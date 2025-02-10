/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "videodec_sample.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint8_t START_CODE_SIZE = 4;
constexpr uint8_t MPEG2_FRAME_HEAD[] = {0x00, 0x00, 0x01, 0x00};
constexpr uint8_t MPEG2_SEQUENCE_HEAD[] = {0x00, 0x00, 0x01, 0xb3};
constexpr uint32_t PREREAD_BUFFER_SIZE = 0.1 * 1024 * 1024;
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint32_t EOS_COUNT = 10;
constexpr uint32_t MAX_WIDTH = 4000;
constexpr uint32_t MAX_HEIGHT = 3000;
constexpr uint32_t MAX_NALU_SIZE = MAX_WIDTH * MAX_HEIGHT << 1;
VDecNdkSample *dec_sample = nullptr;

SHA512_CTX g_c;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
unsigned char g_md[SHA512_DIGEST_LENGTH];
bool g_fuzzError = false;

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
    g_fuzzError = true;
    signal->inCond_.notify_all();
}

void VdecFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
    int32_t currentWidht = 0;
    int32_t currentHeight = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &currentWidht);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &currentHeight);
    dec_sample->DEFAULT_WIDTH = currentWidht;
    dec_sample->DEFAULT_HEIGHT = currentHeight;
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
    if (maxInputSize > 0) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MAX_INPUT_SIZE, maxInputSize);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    std::cout << "DEFAULT_WIDTH: " << DEFAULT_WIDTH << "DEFAULT_HEIGHT: " << DEFAULT_HEIGHT << std::endl;
    originalHeight = DEFAULT_HEIGHT;
    originalWidth = DEFAULT_WIDTH;
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, DEFAULT_ROTATION);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIXEL_FORMAT);
    int ret = OH_VideoDecoder_Configure(vdec_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

void VDecNdkSample::CheckOutputDescription()
{
    OH_AVFormat *newFormat = OH_VideoDecoder_GetOutputDescription(vdec_);
    if (newFormat != nullptr) {
        int32_t picWidth = 0;
        int32_t picHeight = 0;
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_PIC_WIDTH, &picWidth);
        OH_AVFormat_GetIntValue(newFormat, OH_MD_KEY_VIDEO_PIC_HEIGHT, &picHeight);
        if (picWidth != DEFAULT_WIDTH || picHeight != DEFAULT_HEIGHT) {
            std::cout << "picWidth: " << picWidth << " picHeight: " << picHeight << std::endl;
            std::cout << "DEFAULT_WIDTH: " << DEFAULT_WIDTH << "DEFAULT_HEIGHT: " << DEFAULT_HEIGHT << std::endl;
            std::cout << "originalWidth: " << originalWidth << "originalHeight: " << originalHeight << std::endl;
            std::cout << "errCount  !=: " << errCount << std::endl;
            errCount++;
        }
    } else {
        std::cout << "errCount  newFormat == nullptr:" << errCount << std::endl;
        errCount++;
    }
    OH_AVFormat_Destroy(newFormat);
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
        mpegUnit_.reset();
        mpegUnit_ = nullptr;
        prereadBuffer_.reset();
        prereadBuffer_ = nullptr;
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
    prereadBuffer_ = std::make_unique<uint8_t []>(PREREAD_BUFFER_SIZE + START_CODE_SIZE);
    mpegUnit_ = std::make_unique<std::vector<uint8_t>>(MAX_NALU_SIZE);

    if (inFile_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        return AV_ERR_UNKNOWN;
    }
    inFile_->open(INP_DIR, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "open input file failed" << endl;
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
        vdec_ = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_MPEG2);
    }
    dec_sample = this;
    return vdec_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VDecNdkSample::WaitForEOS()
{
    if (!AFTER_EOS_DESTORY_CODEC && inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }

    if (outputLoop_ && outputLoop_->joinable()) {
        outputLoop_->join();
    }
}

void VDecNdkSample::WriteOutputFrame(uint32_t index, OH_AVMemory *buffer, OH_AVCodecBufferAttr attr, FILE *outFile)
{
    if (!SURFACE_OUTPUT) {
        uint8_t *tmpBuffer = new uint8_t[attr.size];
        if (memcpy_s(tmpBuffer, attr.size, OH_AVMemory_GetAddr(buffer), attr.size) != EOK) {
            cout << "Fatal: memory copy failed" << endl;
        }
        fwrite(tmpBuffer, 1, attr.size, outFile);
        SHA512_Update(&g_c, tmpBuffer, attr.size);
        delete[] tmpBuffer;
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
}

void VDecNdkSample::OutputFunc()
{
    SHA512_Init(&g_c);
    FILE *outFile = fopen(OUT_DIR, "wb");
    if (outFile == nullptr) {
        return;
    }
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
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

        uint32_t index = signal_->outIdxQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *buffer = signal_->outBufferQueue_.front();
        signal_->outBufferQueue_.pop();
        signal_->outIdxQueue_.pop();
        signal_->attrQueue_.pop();
        lock.unlock();

        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            SHA512_Final(g_md, &g_c);
            OPENSSL_cleanse(&g_c, sizeof(g_c));
            MdCompare(g_md, SHA512_DIGEST_LENGTH, fileSourcesha256);
            if (AFTER_EOS_DESTORY_CODEC) {
                (void)Stop();
                Release();
            }
            break;
        }
        if (dec_sample->checkOutPut) {
            CheckOutputDescription();
        }
        WriteOutputFrame(index, buffer, attr, outFile);
        if (errCount > 0) {
            break;
        }
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

void VDecNdkSample::PtrStep(uint32_t &bufferSize, unsigned char **pBuffer, uint32_t size)
{
    pPrereadBuffer_ += size;
    bufferSize += size;
    *pBuffer += size;
}

void VDecNdkSample::PtrStepExtraRead(uint32_t &bufferSize, unsigned char **pBuffer)
{
    bufferSize -= START_CODE_SIZE;
    *pBuffer -= START_CODE_SIZE;
    pPrereadBuffer_ = 0;
}

void VDecNdkSample::GetBufferSize()
{
    auto pBuffer = mpegUnit_->data();
    uint32_t bufferSize = 0;
    mpegUnit_->resize(MAX_NALU_SIZE);
    do {
        auto pos1 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + START_CODE_SIZE,
            prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
        uint32_t size1 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos1);
        auto pos2 = std::search(prereadBuffer_.get() + pPrereadBuffer_, prereadBuffer_.get() +
            pPrereadBuffer_ + size1, std::begin(MPEG2_SEQUENCE_HEAD), std::end(MPEG2_SEQUENCE_HEAD));
        uint32_t size = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos2);
        if (size == 0) {
            auto pos3 = std::search(prereadBuffer_.get() + pPrereadBuffer_ + size1 + START_CODE_SIZE,
            prereadBuffer_.get() + prereadBufferSize_, std::begin(MPEG2_FRAME_HEAD), std::end(MPEG2_FRAME_HEAD));
            uint32_t size2 = std::distance(prereadBuffer_.get() + pPrereadBuffer_, pos3);
            if (memcpy_s(pBuffer, size2, prereadBuffer_.get() + pPrereadBuffer_, size2) != EOK) {
                return;
            }
            PtrStep(bufferSize, &pBuffer, size2);
            if (!((pPrereadBuffer_ == prereadBufferSize_) && !inFile_->eof())) {
                break;
            }
        } else if (size1 > size) {
            if (memcpy_s(pBuffer, size, prereadBuffer_.get() + pPrereadBuffer_, size) != EOK) {
                return;
            }
            PtrStep(bufferSize, &pBuffer, size);
            if (!((pPrereadBuffer_ == prereadBufferSize_) && !inFile_->eof())) {
                break;
            }
        } else {
            if (memcpy_s(pBuffer, size1, prereadBuffer_.get() + pPrereadBuffer_, size1) != EOK) {
                return;
            }
            PtrStep(bufferSize, &pBuffer, size1);
            if (!((pPrereadBuffer_ == prereadBufferSize_) && !inFile_->eof())) {
                break;
            }
        }
        inFile_->read(reinterpret_cast<char *>(prereadBuffer_.get() + START_CODE_SIZE), PREREAD_BUFFER_SIZE);
        prereadBufferSize_ = inFile_->gcount() + START_CODE_SIZE;
        pPrereadBuffer_ = START_CODE_SIZE;
        if (memcpy_s(prereadBuffer_.get(), START_CODE_SIZE, pBuffer - START_CODE_SIZE, START_CODE_SIZE) != EOK) {
            return;
        }
        PtrStepExtraRead(bufferSize, &pBuffer);
    } while (pPrereadBuffer_ != prereadBufferSize_);
    mpegUnit_->resize(bufferSize);
}

int32_t VDecNdkSample::ReadData(uint32_t index, OH_AVMemory *buffer)
{
    OH_AVCodecBufferAttr attr;
    uint32_t bufferSize = 0;
    if (BEFORE_EOS_INPUT && frameCount_ > EOS_COUNT) {
        SetEOS(index);
        return 1;
    }
    if (BEFORE_EOS_INPUT_INPUT && frameCount_ > EOS_COUNT) {
        memset_s(&attr, sizeof(OH_AVCodecBufferAttr), 0, sizeof(OH_AVCodecBufferAttr));
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
        BEFORE_EOS_INPUT_INPUT = false;
    }
    if (inFile_->tellg() == 0) {
        inFile_->read(reinterpret_cast<char *>(prereadBuffer_.get() + START_CODE_SIZE), PREREAD_BUFFER_SIZE);
        prereadBufferSize_ = inFile_->gcount() + START_CODE_SIZE;
        pPrereadBuffer_ = START_CODE_SIZE;
    }

    if (finishLastPush) {
        SetEOS(index);
        mpegUnit_->resize(0);
        return 1;
    }
    
    GetBufferSize();
    bufferSize = mpegUnit_->size();
    if (bufferSize > MAX_NALU_SIZE) {
        return 1;
    }
    return SendData(bufferSize, index, buffer);
}

uint32_t VDecNdkSample::SendData(uint32_t bufferSize, uint32_t index, OH_AVMemory *buffer)
{
    OH_AVCodecBufferAttr attr;
    uint8_t *frameBuffer = nullptr;
    int32_t ret = 0;
    if (bufferSize > 0) {
        frameBuffer = new uint8_t[bufferSize];
    } else {
        delete[] frameBuffer;
        return 0;
    }
    memcpy_s(frameBuffer, bufferSize, mpegUnit_->data(), bufferSize);
    attr.pts = GetSystemTimeUs();
    attr.size = bufferSize;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    if (pPrereadBuffer_ == prereadBufferSize_ && inFile_->eof()) {
        finishLastPush = true;
    }
    int32_t size = OH_AVMemory_GetSize(buffer);
    if (size < attr.size) {
        delete[] frameBuffer;
        cout << "ERROR:AVMemory not enough, buffer size" << attr.size << "   AVMemory Size " << size << endl;
        isRunning_.store(false);
        StopOutloop();
        return 1;
    }
    uint8_t *buffer_addr = OH_AVMemory_GetAddr(buffer);
    if (memcpy_s(buffer_addr, size, frameBuffer, attr.size) != EOK) {
        delete[] frameBuffer;
        cout << "Fatal: memcpy fail" << endl;
        isRunning_.store(false);
        return 1;
    }
    delete[] frameBuffer;
    ret = OH_VideoDecoder_PushInputData(vdec_, index, attr);
    if (ret != AV_ERR_OK) {
        errCount++;
        cout << "push input data failed, error:" << ret << endl;
    }
    frameCount_ = frameCount_ + 1;
    if (finishLastPush && repeatRun) {
        inFile_->clear();
        inFile_->seekg(0, ios::beg);
        finishLastPush = false;
        cout << "repeat" << endl;
        return 0;
    }
    return 0;
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
        uint32_t index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        lock.unlock();
        int ret = ReadData(index, buffer);
        if (ret == 1) {
            break;
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
        if (!isRunning_.load() && g_fuzzError) {
            return true;
        }
        return signal_->inIdxQueue_.size() > 0;
    });
    if (g_fuzzError)
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

void VDecNdkSample::SetEOS(uint32_t index)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    int32_t res = OH_VideoDecoder_PushInputData(vdec_, index, attr);
    cout << "OH_VideoDecoder_PushInputData    EOS   res: " << res << endl;
}

int32_t VDecNdkSample::state_EOS()
{
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() {
        if (!isRunning_.load()) {
            return true;
        }
        return signal_->inIdxQueue_.size() > 0;
    });
    uint32_t index = signal_->inIdxQueue_.front();
    signal_->inIdxQueue_.pop();
    OH_AVCodecBufferAttr attr;
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
    int ret = 0;
    if (vdec_ != nullptr) {
        ret = OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }

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
    int32_t ret = OH_VideoDecoder_Start(vdec_);
    if (ret == AV_ERR_OK) {
        isRunning_.store(true);
    }
    return ret;
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