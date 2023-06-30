/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <arpa/inet.h>
#include <sys/time.h>
#include <utility>
#include "consumer_surface.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "videodec_ndk_sample.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
const string MIME_TYPE = "video/avc";
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;

constexpr int32_t FOUR = 4;
constexpr int32_t EIGHT = 8;
constexpr int32_t SIXTEEN = 16;
constexpr int32_t TWENTY_FOUR = 24;
constexpr uint8_t SPS = 7;
constexpr uint8_t PPS = 8;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint32_t EOS_COUNT = 10;
constexpr uint32_t MAX_WIDTH = 4000;
constexpr uint32_t MAX_HEIGHT = 3000;
VDecNdkSample *dec_sample = nullptr;
char HEX_MAX = 0x1f;
SHA512_CTX c;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
unsigned char md[SHA512_DIGEST_LENGTH];
bool fuzzError = false;

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void clearBufferqueue(std::queue<OH_AVCodecBufferAttr> &q)
{
    std::queue<OH_AVCodecBufferAttr> empty;
    swap(empty, q);
}
} // namespace

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs(cs)
    {
        outFile_ = std::make_unique<std::ofstream>();
        outFile_->open(name.data(), std::ios::out | std::ios::binary);
    };
    ~TestConsumerListener()
    {
        if (outFile_ != nullptr) {
            outFile_->close();
        }
    }
    void OnBufferAvailable() override
    {
        sptr<SurfaceBuffer> buffer;
        int32_t flushFence;
        cs->AcquireBuffer(buffer, flushFence, timestamp, damage);
        cs->ReleaseBuffer(buffer, -1);
    }

private:
    int64_t timestamp = 0;
    Rect damage = {};
    sptr<Surface> cs {nullptr};
    std::unique_ptr<std::ofstream> outFile_;
};
VDecNdkSample::~VDecNdkSample()
{
    Release();
}

void VdecError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    cout << "Error errorCode=" << errorCode << endl;
    fuzzError = true;
    signal->inCond_.notify_all();
}

void VdecFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
    int32_t current_width = 0;
    int32_t current_height = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &current_width);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &current_height);
    dec_sample->DEFAULT_WIDTH = current_width;
    dec_sample->DEFAULT_HEIGHT = current_height;
}

void VdecInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

void VdecOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                         void *userData)
{
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->attrQueue_.push(*attr);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

bool VDecNdkSample::MdCompare(unsigned char *buffer, int len, const char *source[])
{
    bool result = true;
    for (int i = 0; i < len; i++) {
        char std[SHA512_DIGEST_LENGTH] = {0};
        int re = strcmp(source[i], std);
        if (re != 0) {
            result = false;
            break;
        }
    }
    return result;
}

int64_t VDecNdkSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = (int64_t)now.tv_sec * NANOS_IN_SECOND + now.tv_nsec;
    return nanoTime / NANOS_IN_MICRO;
}

int32_t VDecNdkSample::ConfigureVideoDecoder()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, DEFAULT_ROTATION);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIXEL_FORMAT);
    int ret = OH_VideoDecoder_Configure(vdec_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VDecNdkSample::RunVideoDec_Surface(string codeName)
{
    SURFACE_OUTPUT = true;
    int err = AV_ERR_OK;
    cs = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs, OUT_DIR);
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    ps = Surface::CreateSurfaceAsProducer(p);
    OHNativeWindow *nativeWindow = CreateNativeWindowFromSurface(&ps);
    if (!nativeWindow) {
        cout << "Failed to create surface" << endl;
        return AV_ERR_UNKNOWN;
    }
    err = CreateVideoDecoder(codeName);
    if (err != AV_ERR_OK) {
        cout << "Failed to create video decoder" << endl;
        return err;
    }
    err = SetVideoDecoderCallback();
    if (err != AV_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return err;
    }
    err = ConfigureVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return err;
    }
    err = OH_VideoDecoder_SetSurface(vdec_, nativeWindow);
    if (err != AV_ERR_OK) {
        cout << "Failed to set surface" << endl;
        return err;
    }
    err = StartVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return err;
    }
    return err;
}

int32_t VDecNdkSample::RunVideoDec(string codeName)
{
    SURFACE_OUTPUT = false;
    int err = CreateVideoDecoder(codeName);
    if (err != AV_ERR_OK) {
        cout << "Failed to create video decoder" << endl;
        return err;
    }

    err = ConfigureVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to configure video decoder" << endl;
        Release();
        return err;
    }

    err = SetVideoDecoderCallback();
    if (err != AV_ERR_OK) {
        cout << "Failed to setCallback" << endl;
        Release();
        return err;
    }

    err = StartVideoDecoder();
    if (err != AV_ERR_OK) {
        cout << "Failed to start video decoder" << endl;
        Release();
        return err;
    }
    return err;
}

int32_t VDecNdkSample::SetVideoDecoderCallback()
{
    signal_ = new VDecSignal();
    if (signal_ == nullptr) {
        cout << "Failed to new VDecSignal" << endl;
        return AV_ERR_UNKNOWN;
    }

    cb_.onError = VdecError;
    cb_.onStreamChanged = VdecFormatChanged;
    cb_.onNeedInputData = VdecInputDataReady;
    cb_.onNeedOutputData = VdecOutputDataReady;
    return OH_VideoDecoder_SetCallback(vdec_, cb_, static_cast<void *>(signal_));
}

void VDecNdkSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VDecNdkSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        signal_->inCond_.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_.reset();
    }
}

int32_t VDecNdkSample::StartVideoDecoder()
{
    int ret = OH_VideoDecoder_Start(vdec_);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        return ret;
    }

    isRunning_.store(true);

    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(INP_DIR, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }

    inputLoop_ = make_unique<thread>(&VDecNdkSample::InputFunc_AVCC, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }

    outputLoop_ = make_unique<thread>(&VDecNdkSample::OutputFunc, this);

    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VDecNdkSample::CreateVideoDecoder(string codeName)
{
    if (!codeName.empty()) {
        vdec_ = OH_VideoDecoder_CreateByName(codeName.c_str());
    } else {
        vdec_ = OH_VideoDecoder_CreateByMime(MIME_TYPE.c_str());
    }
    dec_sample = this;
    return vdec_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VDecNdkSample::WaitForEOS()
{
    if (outputLoop_) {
        outputLoop_->join();
    }
    if (inputLoop_) {
        inputLoop_->join();
    }
}

void VDecNdkSample::OutputFunc()
{
    SHA512_Init(&c);
    FILE *outFile = fopen(OUT_DIR, "wb");
    if (outFile == nullptr) {
        cout << "dump data fail" << endl;
        return;
    }
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        OH_AVCodecBufferAttr attr;
        uint32_t index;
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                cout << "quit out signal" << endl;
                return true;
            }
            return signal_->outIdxQueue_.size() > 0;
        });
        if (!isRunning_.load()) {
            break;
        }
        index = signal_->outIdxQueue_.front();
        attr = signal_->attrQueue_.front();
        signal_->outIdxQueue_.pop();
        signal_->attrQueue_.pop();
        lock.unlock();
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            signal_->outBufferQueue_.pop();
            SHA512_Final(md, &c);
            OPENSSL_cleanse(&c, sizeof(c));
            MdCompare(md, SHA512_DIGEST_LENGTH, fileSourcesha256);
            if (AFTER_EOS_DESTORY_CODEC) {
                (void)Stop();
            }
            int64_t firstTime = 0;
            int64_t aveTime = 0;
            int64_t sumTime = 0;
            if (firstTime == 0 && outCount > 0) {
                firstTime = outTimeArray[0];
            }
            for (int i = 0; i < outCount; i++) {
                sumTime = sumTime + outTimeArray[i];
            }
            aveTime = sumTime / outCount;
            cout << "dec finish " << INP_DIR << "  firstTime:" << firstTime << "   aveTime:" << aveTime << endl;
            break;
        }
        if (start_time == 0) {
            start_time = GetSystemTimeUs();
        }
        int64_t decTs = GetSystemTimeUs() - attr.pts;
        outTimeArray[outCount] = decTs;
        outCount = outCount + 1;

        if (!SURFACE_OUTPUT) {
            int size = attr.size;
            OH_AVMemory *buffer = signal_->outBufferQueue_.front();
            fwrite(OH_AVMemory_GetAddr(buffer), 1, size, outFile);
            SHA512_Update(&c, OH_AVMemory_GetAddr(buffer), size);
            if (OH_VideoDecoder_FreeOutputData(vdec_, index) != AV_ERR_OK) {
                cout << "Fatal: ReleaseOutputBuffer fail" << endl;
                errCount = errCount + 1;
            }
        } else {
            if (OH_VideoDecoder_RenderOutputData(vdec_, index) != AV_ERR_OK) {
                cout << "Fatal: RenderOutputBuffer fail" << endl;
                errCount = errCount + 1;
            }
        }
        if (errCount > 0) {
            break;
        }
        signal_->outBufferQueue_.pop();
    }
    (void)fclose(outFile);
}
void VDecNdkSample::Flush_buffer()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    std::queue<OH_AVMemory *> empty;
    swap(empty, signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();
}
void VDecNdkSample::InputFunc_AVCC()
{
    frameCount_ = 1;
    errCount = 0;
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        if (frameCount_ % (EOS_COUNT >> 1) == 0) {
            if (REPEAT_START_FLUSH_BEFORE_EOS > 0) {
                REPEAT_START_FLUSH_BEFORE_EOS--;
                OH_VideoDecoder_Flush(vdec_);
                Flush_buffer();
                OH_VideoDecoder_Start(vdec_);
            }
            if (REPEAT_START_STOP_BEFORE_EOS > 0) {
                REPEAT_START_STOP_BEFORE_EOS--;
                OH_VideoDecoder_Stop(vdec_);
                Flush_buffer();
                OH_VideoDecoder_Start(vdec_);
            }
        }

        uint32_t index;
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                cout << "quit signal" << endl;
                return true;
            }
            return signal_->inIdxQueue_.size() > 0;
        });
        if (!isRunning_.load()) {
            break;
        }
        index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        lock.unlock();
        OH_AVCodecBufferAttr attr;
        if (!inFile_->eof()) {
            if (BEFORE_EOS_INPUT && frameCount_ > EOS_COUNT) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                (void)OH_VideoDecoder_PushInputData(vdec_, index, attr);
                break;
            }
            if (BEFORE_EOS_INPUT_INPUT && frameCount_ > EOS_COUNT) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                BEFORE_EOS_INPUT_INPUT = false;
            }
            uint8_t ch[4] = {};

            (void)inFile_->read(reinterpret_cast<char *>(ch), FOUR);
            if (repeatRun && inFile_->eof()) {
                inFile_->clear();
                inFile_->seekg(0, ios::beg);
                cout << "repeat" << endl;
                continue;
            } else if (inFile_->eof()) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                (void)OH_VideoDecoder_PushInputData(vdec_, index, attr);
                cout << "OH_VideoDecoder_PushInputData    EOS" << endl;
                break;
            }
            uint32_t bufferSize = (uint32_t)(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << EIGHT) |
                                             ((ch[1] & 0xFF) << SIXTEEN) | (ch[0] & 0xFF << TWENTY_FOUR));
            if (bufferSize > MAX_WIDTH * MAX_HEIGHT << 1)
                break;
            uint8_t *fileBuffer = new uint8_t[bufferSize];
            uint8_t *frameBuffer = new uint8_t[bufferSize + START_CODE_SIZE];
            if (fileBuffer == nullptr) {
                cout << "Fatal: no memory" << endl;
                continue;
            }
            (void)inFile_->read(reinterpret_cast<char *>(fileBuffer), bufferSize);
            switch (fileBuffer[0] & HEX_MAX) {
                case SPS:
                    memcpy_s(frameBuffer, bufferSize + START_CODE_SIZE, START_CODE, START_CODE_SIZE);
                    memcpy_s(frameBuffer + START_CODE_SIZE, bufferSize, fileBuffer, bufferSize);
                    attr.pts = GetSystemTimeUs();
                    attr.size = bufferSize + START_CODE_SIZE;
                    attr.offset = 0;
                    attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                    break;
                case PPS:
                    memcpy_s(frameBuffer, bufferSize + START_CODE_SIZE, START_CODE, START_CODE_SIZE);
                    memcpy_s(frameBuffer + START_CODE_SIZE, bufferSize, fileBuffer, bufferSize);
                    attr.pts = GetSystemTimeUs();
                    attr.size = bufferSize + START_CODE_SIZE;
                    attr.offset = 0;
                    attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                    break;
                default: {
                    memcpy_s(frameBuffer, bufferSize + START_CODE_SIZE, START_CODE, START_CODE_SIZE);
                    memcpy_s(frameBuffer + START_CODE_SIZE, bufferSize, fileBuffer, bufferSize);
                    attr.pts = GetSystemTimeUs();
                    attr.size = bufferSize + START_CODE_SIZE;
                    attr.offset = 0;
                    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
                }
            }
            int32_t size = OH_AVMemory_GetSize(buffer);
            if (size < attr.size) {
                delete[] fileBuffer;
                delete[] frameBuffer;
                cout << "ERROR:AVMemory not enough, buffer size" << attr.size << "   AVMemory Size " << size << endl;
                isRunning_.store(false);
                StopOutloop();
                return;
            }
            uint8_t *buffer_addr = OH_AVMemory_GetAddr(buffer);
            if (memcpy_s(buffer_addr, size, frameBuffer, attr.size) != EOK) {
                delete[] fileBuffer;
                delete[] frameBuffer;
                cout << "Fatal: memcpy fail" << endl;
                isRunning_.store(false);
                return;
            }
            delete[] fileBuffer;
            delete[] frameBuffer;
            int32_t ret = OH_VideoDecoder_PushInputData(vdec_, index, attr);
            if (ret != AV_ERR_OK) {
                errCount++;
                cout << "push input data failed, error:" << ret << endl;
            }
            frameCount_ = frameCount_ + 1;
            if (inFile_->eof()) {
                isRunning_.store(false);
                StopOutloop();
            }
        }

        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

OH_AVErrCode VDecNdkSample::InputFunc_FUZZ(const uint8_t *data, size_t size)
{
    uint32_t index;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() {
        if (fuzzError) {
            return true;
        }
        return signal_->inIdxQueue_.size() > 0;
    });
    if (fuzzError)
        return AV_ERR_TIMEOUT;
    index = signal_->inIdxQueue_.front();
    auto buffer = signal_->inBufferQueue_.front();
    lock.unlock();
    int32_t buffer_size = OH_AVMemory_GetSize(buffer);
    uint8_t *buffer_addr = OH_AVMemory_GetAddr(buffer);

    if (memcpy_s(buffer_addr, buffer_size, data, size) != EOK) {
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    OH_AVCodecBufferAttr attr;
    attr.pts = GetSystemTimeUs();
    attr.size = buffer_size;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVErrCode ret = OH_VideoDecoder_PushInputData(vdec_, index, attr);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    return ret;
}

int32_t VDecNdkSample::EOS()
{
    OH_AVCodecBufferAttr attr;
    int32_t index = 0;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outCond_.wait(lock, [this]() { return signal_->outIdxQueue_.size() > 0; });
    index = signal_->outIdxQueue_.front();
    attr = signal_->attrQueue_.front();
    lock.unlock();
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    return OH_VideoDecoder_PushInputData(vdec_, index, attr);
}

int32_t VDecNdkSample::state_EOS()
{
    OH_AVCodecBufferAttr attr;
    int32_t index = 0;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    return OH_VideoDecoder_PushInputData(vdec_, index, attr);
}

int32_t VDecNdkSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();

    return OH_VideoDecoder_Flush(vdec_);
}

int32_t VDecNdkSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    StopOutloop();
    ReleaseInFile();
    return OH_VideoDecoder_Reset(vdec_);
}

int32_t VDecNdkSample::Release()
{
    int ret = OH_VideoDecoder_Destroy(vdec_);
    vdec_ = nullptr;
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
    return ret;
}

int32_t VDecNdkSample::Stop()
{
    StopInloop();
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    ReleaseInFile();
    return OH_VideoDecoder_Stop(vdec_);
}

int32_t VDecNdkSample::Start()
{
    return OH_VideoDecoder_Start(vdec_);
}

void VDecNdkSample::StopOutloop()
{
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->attrQueue_);
        signal_->outCond_.notify_all();
        lock.unlock();
    }
}

int32_t VDecNdkSample::SetParameter(OH_AVFormat *format)
{
    return OH_VideoDecoder_SetParameter(vdec_, format);
}