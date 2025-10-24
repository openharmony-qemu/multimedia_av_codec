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
#include "videodec_sample.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;

VDecFuzzSample *g_decSample = nullptr;

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

void clearAvBufferQueue(std::queue<OH_AVBuffer *> &q)
{
    std::queue<OH_AVBuffer *> empty;
    swap(empty, q);
}
} // namespace

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs) : cs(cs) {};
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

VDecFuzzSample::~VDecFuzzSample()
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
    g_decSample->isRunning_.store(false);
    signal->inCond_.notify_all();
}

void VdecFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &currentWidth);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &currentHeight);
    g_decSample->defaultWidth = currentWidth;
    g_decSample->defaultHeight = currentHeight;
}

void VdecInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
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

void VdecOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
    int32_t ret = 0;
    if (g_decSample->isSurfMode) {
        ret = OH_VideoDecoder_RenderOutputBuffer(codec, index);
    } else {
        ret = OH_VideoDecoder_FreeOutputBuffer(codec, index);
    }
    if (ret != AV_ERR_OK) {
        g_decSample->Flush();
        g_decSample->Start();
    }
}

int64_t VDecFuzzSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;
    return nanoTime / NANOS_IN_MICRO;
}

int32_t VDecFuzzSample::ConfigureVideoDecoder()
{
    if (isSurfMode) {
        cs = Surface::CreateSurfaceAsConsumer();
        sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs);
        cs->RegisterConsumerListener(listener);
        auto p = cs->GetProducer();
        ps = Surface::CreateSurfaceAsProducer(p);
        nativeWindow = CreateNativeWindowFromSurface(&ps);
        OH_VideoDecoder_SetSurface(vdec_, nativeWindow);
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, defaultPixelFormat);
    int ret = OH_VideoDecoder_Configure(vdec_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VDecFuzzSample::RunVideoDec(string codeName)
{
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

int32_t VDecFuzzSample::SetVideoDecoderCallback()
{
    signal_ = new VDecSignal();
    if (signal_ == nullptr) {
        cout << "Failed to new VDecSignal" << endl;
        return AV_ERR_UNKNOWN;
    }

    cb_.onError = VdecError;
    cb_.onStreamChanged = VdecFormatChanged;
    cb_.onNeedInputBuffer = VdecInputDataReady;
    cb_.onNewOutputBuffer = VdecOutputDataReady;
    return OH_VideoDecoder_RegisterCallback(vdec_, cb_, static_cast<void *>(signal_));
}

void VDecFuzzSample::ReleaseInFile()
{
    if (inFile_ != nullptr) {
        if (inFile_->is_open()) {
            inFile_->close();
        }
        inFile_.reset();
        inFile_ = nullptr;
    }
}

void VDecFuzzSample::StopInloop()
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

int32_t VDecFuzzSample::StartVideoDecoder()
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
    inFile_->open(inpDir, ios::in | ios::binary);
    if (!inFile_->is_open()) {
        cout << "open input file failed" << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        inFile_->close();
        inFile_.reset();
        inFile_ = nullptr;
        return AV_ERR_UNKNOWN;
    }

    inputLoop_ = make_unique<thread>(&VDecFuzzSample::InputFuncAVCC, this);
    if (inputLoop_ == nullptr) {
        cout << "Failed to create input loop" << endl;
        isRunning_.store(false);
        (void)OH_VideoDecoder_Stop(vdec_);
        ReleaseInFile();
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VDecFuzzSample::CreateVideoDecoder(string codeName)
{
    vdec_ = OH_VideoDecoder_CreateByName(randomName.c_str());
    if (vdec_) {
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
    OH_AVCodec *tmpDec = OH_VideoDecoder_CreateByMime(randomMime.c_str());
    if (tmpDec) {
        OH_VideoDecoder_Destroy(tmpDec);
        tmpDec = nullptr;
    }
    tmpDec = OH_VideoDecoder_CreateByMime("video/mjpeg");
    if (tmpDec) {
        OH_VideoDecoder_Destroy(tmpDec);
        tmpDec = nullptr;
    }
    vdec_ = OH_VideoDecoder_CreateByName(codeName.c_str());
    g_decSample = this;
    return vdec_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VDecFuzzSample::WaitForEOS()
{
    if (inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }
}

void VDecFuzzSample::CopyStartCode(uint8_t *frameBuffer, uint32_t bufferSize, OH_AVCodecBufferAttr &attr)
{
    attr.pts = GetSystemTimeUs();
    attr.size = bufferSize;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
}

int32_t VDecFuzzSample::ReadData(uint32_t index, OH_AVBuffer *buffer)
{
    uint32_t bufferSize = 0;
    inFile_->read(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), OH_AVBuffer_GetCapacity(buffer));
    bufferSize = static_cast<uint32_t>(inFile_->gcount());
    if (bufferSize == 0) {
        return -1;
    }
    if (inFile_->eof()) {
        SetEOS(buffer, index);
        return 1;
    }
    OH_AVCodecBufferAttr attr;
    attr.pts = GetSystemTimeUs();
    attr.size = bufferSize;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    return SendData(bufferSize, index, buffer);
}

uint32_t VDecFuzzSample::SendData(uint32_t bufferSize, uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    if (bufferSize <= 0) {
        return 0;
    }
    uint8_t *frameBuffer = new uint8_t[bufferSize];
    (void)inFile_->read(reinterpret_cast<char *>(frameBuffer), bufferSize);
    int32_t size = OH_AVBuffer_GetCapacity(buffer);
    if (size < bufferSize) {
        delete[] frameBuffer;
        cout << "ERROR:AVBuffer not enough, buffer size" << bufferSize << "   AVBuffer Size " << size << endl;
        isRunning_.store(false);
        return 1;
    }
    uint8_t *bufferAddr = OH_AVBuffer_GetAddr(buffer);
    if (memcpy_s(bufferAddr, size, frameBuffer, bufferSize) != EOK) {
        delete[] frameBuffer;
        cout << "Fatal: memcpy fail" << endl;
        isRunning_.store(false);
        return 1;
    }
    delete[] frameBuffer;
    attr.pts = GetSystemTimeUs();
    attr.size = bufferSize;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t ret = OH_VideoDecoder_PushInputBuffer(vdec_, index);
    if (ret != AV_ERR_OK) {
        errCount++;
        cout << "push input data failed, error:" << ret << endl;
    }
    frameCount_ = frameCount_ + 1;
    if (inFile_->eof()) {
        isRunning_.store(false);
    }
    return 0;
}

void VDecFuzzSample::InputFuncAVCC()
{
    frameCount_ = 1;
    errCount = 0;
    while (isRunning_.load()) {
        if (!isRunning_.load()) {
            break;
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
        if (!inFile_->eof()) {
            int ret = ReadData(index, buffer);
            if (ret == 1) {
                break;
            }
        }
    }
}

OH_AVErrCode VDecFuzzSample::InputFuncFUZZ(const uint8_t *data, size_t size)
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
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    lock.unlock();
    int32_t bufferSize = OH_AVBuffer_GetCapacity(buffer);
    uint8_t *bufferAddr = OH_AVBuffer_GetAddr(buffer);

    if (memcpy_s(bufferAddr, bufferSize, data, size) != EOK) {
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    OH_AVCodecBufferAttr attr;
    attr.pts = GetSystemTimeUs();
    attr.size = bufferSize;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    OH_AVErrCode ret = OH_VideoDecoder_PushInputBuffer(vdec_, index);
    return ret;
}

void VDecFuzzSample::SetEOS(OH_AVBuffer *buffer, uint32_t index)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t res = OH_VideoDecoder_PushInputBuffer(vdec_, index);
    cout << "OH_VideoDecoder_PushInputBuffer    EOS   res: " << res << endl;
}

int32_t VDecFuzzSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    signal_->outCond_.notify_all();
    isRunning_.store(false);
    outLock.unlock();

    return OH_VideoDecoder_Flush(vdec_);
}

int32_t VDecFuzzSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    ReleaseInFile();
    return OH_VideoDecoder_Reset(vdec_);
}

int32_t VDecFuzzSample::Release()
{
    int ret = 0;
    if (vdec_ != nullptr) {
        ret = OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
    if (signal_ != nullptr) {
        clearAvBufferQueue(signal_->inBufferQueue_);
        clearAvBufferQueue(signal_->outBufferQueue_);
        delete signal_;
        signal_ = nullptr;
    }
    return ret;
}

int32_t VDecFuzzSample::Stop()
{
    StopInloop();
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->attrQueue_);
    ReleaseInFile();
    return OH_VideoDecoder_Stop(vdec_);
}

int32_t VDecFuzzSample::Start()
{
    int32_t ret = 0;
    ret = OH_VideoDecoder_Start(vdec_);
    if (ret == AV_ERR_OK) {
        isRunning_.store(true);
    }
    return ret;
}

int32_t VDecFuzzSample::SetParameter(OH_AVFormat *format)
{
    return OH_VideoDecoder_SetParameter(vdec_, format);
}