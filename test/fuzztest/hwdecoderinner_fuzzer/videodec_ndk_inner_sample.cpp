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
#include "meta/meta_key.h"
#include "openssl/crypto.h"
#include "openssl/sha.h"
#include "native_buffer_inner.h"
#include "videodec_inner_sample.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;

namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t START_CODE_SIZE = 4;
constexpr uint8_t START_CODE[START_CODE_SIZE] = {0, 0, 0, 1};
int32_t g_strideSurface = 0;
int32_t g_sliceSurface = 0;
VDecNdkInnerFuzzSample *g_decSample = nullptr;

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}

void clearBufferqueue(std::queue<AVCodecBufferInfo> &q)
{
    std::queue<AVCodecBufferInfo> empty;
    swap(empty, q);
}

void clearFlagqueue(std::queue<AVCodecBufferFlag> &q)
{
    std::queue<AVCodecBufferFlag> empty;
    swap(empty, q);
}
} // namespace

class ConsumerListenerBuffer : public IBufferConsumerListener {
public:
    ConsumerListenerBuffer(sptr<Surface> cs, std::string_view name) : cs(cs)
    {
        outFile_ = std::make_unique<std::ofstream>();
        outFile_->open(name.data(), std::ios::out | std::ios::binary);
    };
    ~ConsumerListenerBuffer()
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

VDecInnerCallback::VDecInnerCallback(std::shared_ptr<VDecInnerSignal> signal) : innersignal_(signal) {}

void VDecInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    g_decSample->isRunning_.store(false);
    innersignal_->inCond_.notify_all();
    innersignal_->outCond_.notify_all();
}

void VDecInnerCallback::OnOutputFormatChanged(const Format& format)
{
    cout << "Format Changed" << endl;
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int32_t stride = 0;
    int32_t sliceHeight = 0;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, currentWidth);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, currentHeight);
    format.GetIntValue(Media::Tag::VIDEO_STRIDE, stride);
    format.GetIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight);
    g_decSample->defaultWidth = currentWidth;
    g_decSample->defaultHeight = currentHeight;
    g_strideSurface = stride;
    g_sliceSurface = sliceHeight;
}

void VDecInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable index:" << index << endl;
    if (innersignal_ == nullptr) {
        std::cout << "buffer is null 1" << endl;
        return;
    }

    unique_lock<mutex> lock(innersignal_->inMutex_);
    innersignal_->inIdxQueue_.push(index);
    innersignal_->inBufferQueue_.push(buffer);
    innersignal_->inCond_.notify_all();
}

void VDecInnerCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info,
    AVCodecBufferFlag flag, std::shared_ptr<AVSharedMemory> buffer)
{
    if (g_decSample->sfOutput) {
        g_decSample->vdec_->ReleaseOutputBuffer(index, true);
    } else {
        g_decSample->vdec_->ReleaseOutputBuffer(index, false);
    }
}

VDecNdkInnerFuzzSample::~VDecNdkInnerFuzzSample()
{
    for (int i = 0; i < static_cast<int>(maxSurfNum); i++) {
        if (nativeWindow[i]) {
            OH_NativeWindow_DestroyNativeWindow(nativeWindow[i]);
            nativeWindow[i] = nullptr;
        }
    }
    if (vdec_ != nullptr) {
        (void)Stop();
        Release();
    }
}

int64_t VDecNdkInnerFuzzSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

void VDecNdkInnerFuzzSample::CreateSurface()
{
    cs[0] = Surface::CreateSurfaceAsConsumer();
    if (cs[0] == nullptr) {
        cout << "Create the surface consummer fail" << endl;
        return;
    }
    GSError err = cs[0]->SetDefaultUsage(BUFFER_USAGE_MEM_DMA | BUFFER_USAGE_VIDEO_DECODER | BUFFER_USAGE_CPU_READ);
    if (err == GSERROR_OK) {
        cout << "set consumer usage succ" << endl;
    } else {
        cout << "set consumer usage failed" << endl;
    }
    sptr<IBufferConsumerListener> listener = new ConsumerListenerBuffer(cs[0], outDir);
    cs[0]->RegisterConsumerListener(listener);
    auto p = cs[0]->GetProducer();
    ps[0] = Surface::CreateSurfaceAsProducer(p);
    nativeWindow[0] = CreateNativeWindowFromSurface(&ps[0]);
    if (autoSwitchSurface)  {
        cs[1] = Surface::CreateSurfaceAsConsumer();
        sptr<IBufferConsumerListener> listener2 = new ConsumerListenerBuffer(cs[1], outDir2);
        cs[1]->RegisterConsumerListener(listener2);
        auto p2 = cs[1]->GetProducer();
        ps[1] = Surface::CreateSurfaceAsProducer(p2);
        nativeWindow[1] = CreateNativeWindowFromSurface(&ps[1]);
    }
}

int32_t VDecNdkInnerFuzzSample::CreateByName(const std::string &name)
{
    vdec_ = VideoDecoderFactory::CreateByName(name);
    g_decSample = this;
    return vdec_ == nullptr ? AVCS_ERR_INVALID_OPERATION : AVCS_ERR_OK;
}

int32_t VDecNdkInnerFuzzSample::Configure()
{
    if (autoSwitchSurface) {
        switchSurfaceFlag = (switchSurfaceFlag == 1) ? 0 : 1;
        if (ps[switchSurfaceFlag] != nullptr) {
            if (vdec_->SetOutputSurface(ps[switchSurfaceFlag]) != AVCS_ERR_INVALID_STATE) {
                errCount++;
            }
        }
    }
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, defaultWidth);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, defaultHeight);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, DEFAULT_FORMAT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, defaultFrameRate);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, defaultColorspace);
    return vdec_->Configure(format);
}

int32_t VDecNdkInnerFuzzSample::Prepare()
{
    return vdec_->Prepare();
}

int32_t VDecNdkInnerFuzzSample::Start()
{
    int32_t ret = vdec_->Start();
    if (ret != AVCS_ERR_OK) {
        cout << "Failed to start codec" << endl;
        isRunning_.store(false);
        Release();
    }
    return ret;
}

int32_t VDecNdkInnerFuzzSample::Stop()
{
    if (signal_ != nullptr) {
        clearIntqueue(signal_->outIdxQueue_);
        clearBufferqueue(signal_->infoQueue_);
        clearFlagqueue(signal_->flagQueue_);
    }

    return vdec_->Stop();
}

int32_t VDecNdkInnerFuzzSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    unique_lock<mutex> outLock(signal_->outMutex_);
    clearIntqueue(signal_->outIdxQueue_);
    clearBufferqueue(signal_->infoQueue_);
    clearFlagqueue(signal_->flagQueue_);
    signal_->outCond_.notify_all();
    outLock.unlock();

    return vdec_->Flush();
}

int32_t VDecNdkInnerFuzzSample::Reset()
{
    isRunning_.store(false);
    return vdec_->Reset();
}

int32_t VDecNdkInnerFuzzSample::Release()
{
    int32_t ret = 0;
    if (vdec_ != nullptr) {
        ret = vdec_->Release();
        vdec_ = nullptr;
    }
    if (signal_ != nullptr) {
        signal_ = nullptr;
    }
    return ret;
}

int32_t VDecNdkInnerFuzzSample::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    return vdec_->QueueInputBuffer(index, info, flag);
}

int32_t VDecNdkInnerFuzzSample::ReleaseOutputBuffer(uint32_t index)
{
    return vdec_->ReleaseOutputBuffer(index, false);
}

int32_t VDecNdkInnerFuzzSample::SetCallback()
{
    signal_ = make_shared<VDecInnerSignal>();
    if (signal_ == nullptr) {
        cout << "Failed to new VEncInnerSignal" << endl;
        return AVCS_ERR_UNKNOWN;
    }

    cb_ = make_shared<VDecInnerCallback>(signal_);
    return vdec_->SetCallback(cb_);
}

int32_t VDecNdkInnerFuzzSample::InputFuncFUZZ(const uint8_t *data, size_t size)
{
    unique_lock<mutex> lock(signal_->inMutex_);
    if (!isRunning_.load()) {
        return AVCS_ERR_NO_MEMORY;
    }
    signal_->inCond_.wait(lock, [this]() {
        if (!isRunning_.load()) {
            return true;
        }
        return signal_->inIdxQueue_.size() > 0;
    });

    if (!isRunning_.load()) {
        return AVCS_ERR_NO_MEMORY;
    }

    uint32_t index = signal_->inIdxQueue_.front();
    auto buffer = signal_->inBufferQueue_.front();
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
    lock.unlock();
    int32_t bufferSize = buffer->GetSize();
    uint8_t *bufferAddr = buffer->GetBase();
    if (size > bufferSize - START_CODE_SIZE) {
        vdec_->QueueInputBuffer(index);
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr, bufferSize, START_CODE, START_CODE_SIZE) != EOK) {
        vdec_->QueueInputBuffer(index);
        return AV_ERR_NO_MEMORY;
    }
    if (memcpy_s(bufferAddr + START_CODE_SIZE, bufferSize - START_CODE_SIZE, data, size) != EOK) {
        vdec_->QueueInputBuffer(index);
        cout << "Fatal: memcpy fail" << endl;
        return AV_ERR_NO_MEMORY;
    }
    AVCodecBufferInfo info;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.presentationTimeUs = GetSystemTimeUs();
    info.size = bufferSize + START_CODE_SIZE;
    info.offset = 0;
    return vdec_->QueueInputBuffer(index, info, flag);
}

int32_t VDecNdkInnerFuzzSample::SetOutputSurface()
{
    CreateSurface();
    return vdec_->SetOutputSurface(ps[0]);
}