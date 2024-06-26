/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "native_avcapability.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
const string MIME_TYPE = "video/avc";
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;

constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
VDecFuzzSample *g_decSample = nullptr;

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
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
    cout << "Error errorCode=" << errorCode << endl;
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

void VdecInputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

void VdecOutputDataReady(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
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
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, 0);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    int ret = OH_VideoDecoder_Configure(vdec_, format);
    OH_AVFormat_Destroy(format);
    return ret;
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
    OH_VideoDecoder_RegisterCallback(vdec_, cb_, static_cast<void *>(signal_));
    return OH_VideoDecoder_RegisterCallback(vdec_, cb_, static_cast<void *>(signal_));
}

int32_t VDecFuzzSample::CreateVideoDecoder()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false, HARDWARE);
    string codecName = OH_AVCapability_GetName(cap);
    vdec_ = OH_VideoDecoder_CreateByName("aabbcc");
    if (vdec_) {
        OH_VideoDecoder_Destroy(vdec_);
        vdec_ = nullptr;
    }
    OH_AVCodec *tmpDec = OH_VideoDecoder_CreateByMime("aabbcc");
    if (tmpDec) {
        OH_VideoDecoder_Destroy(tmpDec);
        tmpDec = nullptr;
    }
    tmpDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    if (tmpDec) {
        OH_VideoDecoder_Destroy(tmpDec);
        tmpDec = nullptr;
    }
    vdec_ = OH_VideoDecoder_CreateByName(codecName.c_str());
    g_decSample = this;
    return vdec_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VDecFuzzSample::WaitForEOS()
{
    if (inputLoop_ && inputLoop_->joinable()) {
        inputLoop_->join();
    }
}

OH_AVErrCode VDecFuzzSample::InputFuncFUZZ(const uint8_t *data, size_t size)
{
    uint32_t index;
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!isRunning_.load()) {
        return AV_ERR_NO_MEMORY;
    }
    signal_->inCond_.wait(lock, [this]() {
        if (!isRunning_.load()) {
            return true;
        }
        return signal_->inIdxQueue_.size() > 0;
    });
    if (!isRunning_.load()) {
        return AV_ERR_NO_MEMORY;
    }
    index = signal_->inIdxQueue_.front();
    auto buffer = signal_->inBufferQueue_.front();
    lock.unlock();
    int32_t bufferSize = OH_AVBuffer_GetCapacity(buffer);
    uint8_t *bufferAddr = OH_AVBuffer_GetAddr(buffer);
    if (size > bufferSize - START_CODE_SIZE) {
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr, bufferSize, START_CODE, START_CODE_SIZE) != EOK) {
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr + START_CODE_SIZE, bufferSize - START_CODE_SIZE, data, size) != EOK) {
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    OH_AVCodecBufferAttr attr;
    attr.pts = GetSystemTimeUs();
    attr.size = size;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    OH_AVErrCode ret = OH_VideoDecoder_PushInputBuffer(vdec_, index);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
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
    cout << "OH_VideoDecoder_PushInputData    EOS   res: " << res << endl;
}

int32_t VDecFuzzSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    clearAvBufferQueue(signal_->inBufferQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    signal_->outCond_.notify_all();
    isRunning_.store(false);
    outLock.unlock();

    return OH_VideoDecoder_Flush(vdec_);
}

int32_t VDecFuzzSample::Reset()
{
    isRunning_.store(false);
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
        delete signal_;
        signal_ = nullptr;
    }
    return ret;
}

int32_t VDecFuzzSample::Stop()
{
    clearIntqueue(signal_->outIdxQueue_);
    isRunning_.store(false);
    return OH_VideoDecoder_Stop(vdec_);
}

int32_t VDecFuzzSample::Start()
{
    int32_t ret = OH_VideoDecoder_Start(vdec_);
    if (ret == AV_ERR_OK) {
        isRunning_.store(true);
    }
    return ret;
}

int32_t VDecFuzzSample::SetParameter(int32_t data)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, data);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, data);
    int32_t ret = OH_VideoDecoder_SetParameter(vdec_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}