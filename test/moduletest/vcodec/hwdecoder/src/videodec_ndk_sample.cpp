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
#include "iconsumer_surface.h"
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
constexpr int32_t TWO = 2;
constexpr int32_t THREE = 3;
constexpr int32_t FOUR = 4;
constexpr int32_t EIGHT = 8;
constexpr int32_t TEN = 10;
constexpr int32_t SIXTEEN = 16;
constexpr int32_t TWENTY_FOUR = 24;
constexpr uint32_t FRAME_INTERVAL = 1; // 16666
char HEX_ZERO = 0x00;
char HEX_ONE = 0x01;
char HEX_MAX = 0x1f;
char HEX_SEVEN = 0x07;
SHA512_CTX c;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
unsigned char md[SHA512_DIGEST_LENGTH];
VDecNdkSample *dec_sample = nullptr;

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
    TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs(cs) {};
    ~TestConsumerListener() {}
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
};
VDecNdkSample::~VDecNdkSample()
{
    Release();
}

void VdecError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
    dec_sample->StopInloop();
    dec_sample->StopOutloop();
    dec_sample->ReleaseInFile();
    dec_sample->Release();
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
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

void VdecOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                         void *userData)
{
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->attrQueue_.push(*attr);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
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

bool VDecNdkSample::MdCompare(unsigned char buffer[], int len, const char *source[])
{
    bool result = true;
    for (int i = 0; i < len; i++) {
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
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
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

int32_t VDecNdkSample::CreateVideoDecoder(string codeName)
{
    vdec_ = OH_VideoDecoder_CreateByMime(MIME_TYPE.c_str());
    dec_sample = this;
    return vdec_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

int32_t VDecNdkSample::StartVideoDecoder()
{
    isRunning_.store(true);
    inFile_ = make_unique<ifstream>();
    if (inFile_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(INP_DIR, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "failed open file " << INP_DIR << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }
    inputLoop_ = make_unique<thread>(&VDecNdkSample::InputFuncTest, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&VDecNdkSample::OutputFuncTest, this);
    if (outputLoop_ == nullptr) {
        cout << "Failed to create output loop" << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        StopInloop();
        Release();
        return AV_ERR_UNKNOWN;
    }
    int ret = OH_VideoDecoder_Start(vdec_);
    if (ret != AV_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning_.store(false);
        ReleaseInFile();
        StopInloop();
        StopOutloop();
        Release();
        return ret;
    }
    return AV_ERR_OK;
}

void VDecNdkSample::testAPI()
{
    cs = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs, OUT_DIR);
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    ps = Surface::CreateSurfaceAsProducer(p);
    OHNativeWindow *nativeWindow = CreateNativeWindowFromSurface(&ps);
    OH_VideoDecoder_SetSurface(vdec_, nativeWindow);

    OH_VideoDecoder_Prepare(vdec_);
    OH_VideoDecoder_Start(vdec_);

    OH_AVFormat *format = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    OH_VideoDecoder_SetParameter(vdec_, format);
    OH_AVFormat_Destroy(format);
    OH_VideoDecoder_GetOutputDescription(vdec_);
    OH_VideoDecoder_Flush(vdec_);
    OH_VideoDecoder_Stop(vdec_);
    OH_VideoDecoder_Reset(vdec_);
    bool isvalid = false;
    OH_VideoDecoder_IsValid(vdec_, &isvalid);
}

void VDecNdkSample::WaitForEOS()
{
    if (inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }
    if (outputLoop_ && outputLoop_->joinable()) {
        outputLoop_->join();
    }
}

void VDecNdkSample::InputFuncTest()
{
    frameCount_ = 0;
    errCount = 0;
    uint32_t repeat_count = 0;
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
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
        uint32_t index;
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return signal_->inIdxQueue_.size() > 0; });
        if (!isRunning_.load()) {
            break;
        }
        index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();

        lock.unlock();
        OH_AVCodecBufferAttr attr;
        if (!inFile_->eof()) {
            if (BEFORE_EOS_INPUT && frameCount_ > TEN) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                (void)OH_VideoDecoder_PushInputData(vdec_, index, attr);
                signal_->inIdxQueue_.pop();
                signal_->inBufferQueue_.pop();
                break;
            }
            if (BEFORE_EOS_INPUT_INPUT && frameCount_ > TEN) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                BEFORE_EOS_INPUT_INPUT = false;
            }
            char ch[4] = {};
            (void)inFile_->read(ch, FOUR);
            if (repeatRun && inFile_->eof()) {
                inFile_->clear();
                inFile_->seekg(0, ios::beg);
                cout << "repeat run " << repeat_count << endl;
                repeat_count++;
                continue;
            } else if (repeat_time > 0 && inFile_->eof() && frameCount_ < repeat_time) {
                inFile_->clear();
                inFile_->seekg(0, ios::beg);
                continue;
            }

            if (inFile_->eof()) {
                attr.pts = 0;
                attr.size = 0;
                attr.offset = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
                (void)OH_VideoDecoder_PushInputData(vdec_, index, attr);
                cout << "OH_VideoDecoder_PushInputData    EOS" << endl;
                signal_->inIdxQueue_.pop();
                signal_->inBufferQueue_.pop();
                break;
            }
            uint32_t bufferSize = (uint32_t)(((ch[3] & 0xFF)) | ((ch[2] & 0xFF) << EIGHT) |
                                             ((ch[1] & 0xFF) << SIXTEEN) | (ch[0] & 0xFF << TWENTY_FOUR));
            if (bufferSize >= DEFAULT_WIDTH * DEFAULT_HEIGHT * THREE >> 1) {
                cout << "read bufferSize abnormal. buffersize = " << bufferSize << endl;
                break;
            }
            uint8_t *fileBuffer = new uint8_t[bufferSize + FOUR];
            if (fileBuffer == nullptr) {
                cout << "Fatal: no memory" << endl;
                continue;
            }
            fileBuffer[0] = HEX_ZERO;
            fileBuffer[1] = HEX_ZERO;
            fileBuffer[TWO] = HEX_ZERO;
            fileBuffer[THREE] = HEX_ONE;
            (void)inFile_->read((char *)fileBuffer + FOUR, bufferSize);
            if ((fileBuffer[FOUR] & HEX_MAX) == HEX_SEVEN || (fileBuffer[FOUR] & HEX_MAX) == 0x08) {
                attr.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            } else {
                attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            }
            int32_t size = OH_AVMemory_GetSize(buffer);
            if (size < bufferSize + FOUR) {
                delete[] fileBuffer;
                cout << "buffer size not enough." << endl;
                continue;
            }
            uint8_t *avBuffer = OH_AVMemory_GetAddr(buffer);
            if (avBuffer == nullptr) {
                cout << "avBuffer == nullptr" << endl;
                inFile_->clear();
                inFile_->seekg(0, ios::beg);
                continue;
            }
            if (memcpy_s(avBuffer, size, fileBuffer, bufferSize + FOUR) != EOK) {
                delete[] fileBuffer;
                cout << "Fatal: memcpy fail" << endl;
                continue;
            }
            int64_t startPts = GetSystemTimeUs();
            attr.pts = startPts;
            attr.size = bufferSize + FOUR;
            attr.offset = 0;
            int32_t result = OH_VideoDecoder_PushInputData(vdec_, index, attr);
            signal_->inIdxQueue_.pop();
            signal_->inBufferQueue_.pop();
            if (result != AV_ERR_OK) {
                errCount = errCount + 1;
                cout << "push input data failed,error:" << result << endl;
            }
            delete[] fileBuffer;
            frameCount_ = frameCount_ + 1;
        }
        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

void VDecNdkSample::OutputFuncTest()
{
    SHA512_Init(&c);
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        OH_AVCodecBufferAttr attr;
        uint32_t index;
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return signal_->outIdxQueue_.size() > 0; });
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
            bool result = MdCompare(md, SHA512_DIGEST_LENGTH, fileSourcesha256);
            int64_t firstTime = 0;
            int64_t aveTime = 0;
            int64_t sumTime = 0;
            if (outCount > 0) {
                firstTime = outTimeArray[0];
            }
            for (int i = 0; i < outCount; i++) {
                sumTime = sumTime + outTimeArray[i];
            }
            aveTime = sumTime / outCount;
            if (end_time == 0) {
                end_time = GetSystemTimeUs();
            }
            double fps = outCount / ((end_time - start_time) / 1000000.00);
            cout << "dec finish " << INP_DIR << " MdCompare result:" << result << "  firstTime:" << firstTime
                 << "   aveTime:" << aveTime << "  fps:" << fps << endl;
            if (AFTER_EOS_DESTORY_CODEC) {
                (void)Stop();
                Release();
            }
            break;
        }
        if (!repeatRun) {
            if (start_time == 0) {
                start_time = GetSystemTimeUs();
            }
            int64_t decTs = GetSystemTimeUs() - attr.pts;
            outTimeArray[outCount] = decTs;
            outCount = outCount + 1;
        }
        if (!SURFACE_OUTPUT) {
            OH_AVMemory *buffer = signal_->outBufferQueue_.front();
            uint32_t size = OH_AVMemory_GetSize(buffer);
            if (size >= DEFAULT_WIDTH * DEFAULT_HEIGHT * THREE >> 1) {
                uint8_t *cropBuffer = new uint8_t[size];
                memcpy_s(cropBuffer, DEFAULT_WIDTH * DEFAULT_HEIGHT, OH_AVMemory_GetAddr(buffer),
                         DEFAULT_WIDTH * DEFAULT_HEIGHT);
                // copy UV
                uint32_t uvSize = size - DEFAULT_WIDTH * DEFAULT_HEIGHT;
                (void)memcpy_s(cropBuffer + DEFAULT_WIDTH * DEFAULT_HEIGHT, (DEFAULT_WIDTH * DEFAULT_HEIGHT >> 1),
                               OH_AVMemory_GetAddr(buffer) + DEFAULT_WIDTH * DEFAULT_HEIGHT,
                               uvSize);
                SHA512_Update(&c, cropBuffer, DEFAULT_WIDTH * DEFAULT_HEIGHT * THREE >> 1);
                delete[] cropBuffer;
            }
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
        signal_->outBufferQueue_.pop();
        if (errCount > 0) {
            break;
        }
    }
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
    int32_t index = 1;
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