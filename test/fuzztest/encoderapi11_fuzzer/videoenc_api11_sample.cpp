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
#include "native_buffer_inner.h"
#include "videoenc_api11_sample.h"
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint32_t MAX_PIXEL_FMT = 5;
constexpr uint32_t DEFAULT_BITRATE = 10000000;
sptr<Surface> cs = nullptr;
sptr<Surface> ps = nullptr;
VEncAPI11FuzzSample *enc_sample = nullptr;

void clearIntqueue(std::queue<uint32_t> &q)
{
    std::queue<uint32_t> empty;
    swap(empty, q);
}
} // namespace

VEncAPI11FuzzSample::~VEncAPI11FuzzSample()
{
    if (SURF_INPUT && nativeWindow) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow);
        nativeWindow = nullptr;
    }
    Release();
}

static void VencError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
}

static void VencFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    cout << "Format Changed" << endl;
}

static void onEncInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(buffer);
    signal->inCond_.notify_all();
}

static void onEncOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    OH_VideoEncoder_FreeOutputBuffer(codec, index);
}

static void onEncInputParam(OH_AVCodec *codec, uint32_t index, OH_AVFormat *parameter, void *userData)
{
    OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_BITRATE, DEFAULT_BITRATE);
    OH_VideoEncoder_PushInputParameter(codec, index);
    return;
}

int64_t VEncAPI11FuzzSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = (int64_t)now.tv_sec * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncAPI11FuzzSample::ConfigureVideoEncoderFuzz(int32_t data)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, data);
    DEFAULT_WIDTH = data;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, data);
    DEFAULT_HEIGHT = data;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, data % MAX_PIXEL_FMT);
    double frameRate = data;
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, data);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, data);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, data);

    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncAPI11FuzzSample::SetVideoEncoderCallback()
{
    signal_ = new VEncSignal();
    if (signal_ == nullptr) {
        cout << "Failed to new VEncSignal" << endl;
        return AV_ERR_UNKNOWN;
    }
    if (SURF_INPUT) {
        int32_t ret = OH_VideoEncoder_RegisterParameterCallback(venc_, onEncInputParam, static_cast<void *>(this));
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    cb_.onError = VencError;
    cb_.onStreamChanged = VencFormatChanged;
    cb_.onNeedInputBuffer = onEncInputBufferAvailable;
    cb_.onNewOutputBuffer = onEncOutputBufferAvailable;
    return OH_VideoEncoder_RegisterCallback(venc_, cb_, static_cast<void *>(signal_));
}

void VEncAPI11FuzzSample::StopInloop()
{
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        clearIntqueue(signal_->inIdxQueue_);
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        lock.unlock();

        inputLoop_->join();
        inputLoop_ = nullptr;
    }
}
int32_t VEncAPI11FuzzSample::CreateSurface()
{
    int32_t ret = 0;
    ret = OH_VideoEncoder_GetSurface(venc_, &nativeWindow);
    if (ret != AV_ERR_OK) {
        cout << "OH_VideoEncoder_GetSurface fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, DEFAULT_WIDTH, DEFAULT_HEIGHT);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }
    return AV_ERR_OK;
}

void VEncAPI11FuzzSample::GetStride()
{
    OH_AVFormat *format = OH_VideoEncoder_GetInputDescription(venc_);
    int32_t inputStride = 0;
    OH_AVFormat_GetIntValue(format, "stride", &inputStride);
    stride_ = inputStride;
    OH_AVFormat_Destroy(format);
}

int32_t VEncAPI11FuzzSample::StartVideoEncoder()
{
    isRunning_.store(true);
    int32_t ret = 0;
    if (SURF_INPUT) {
        ret = CreateSurface();
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    ret = OH_VideoEncoder_Start(venc_);
    GetStride();
    if (ret != AV_ERR_OK) {
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        return ret;
    }
    if (SURF_INPUT) {
        inputLoop_ = make_unique<thread>(&VEncAPI11FuzzSample::InputFuncSurface, this);
    } else {
        inputLoop_ = make_unique<thread>(&VEncAPI11FuzzSample::InputFunc, this);
    }
    if (inputLoop_ == nullptr) {
        isRunning_.store(false);
        (void)OH_VideoEncoder_Stop(venc_);
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VEncAPI11FuzzSample::CreateVideoEncoder(const char *codecName)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    enc_sample = this;
    return venc_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

void VEncAPI11FuzzSample::WaitForEOS()
{
    if (inputLoop_)
        inputLoop_->join();
    inputLoop_ = nullptr;
}

uint32_t VEncAPI11FuzzSample::FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer)
{
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = DEFAULT_WIDTH;
    rect->h = DEFAULT_HEIGHT;
    region.rects = rect;
    NativeWindowHandleOpt(nativeWindow, SET_UI_TIMESTAMP, GetSystemTimeUs());
    int32_t err = OH_NativeBuffer_Unmap(nativeBuffer);
    if (err != 0) {
        return 1;
    }
    err = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (err != 0) {
        return 1;
    }
    return 0;
}

void VEncAPI11FuzzSample::InputFuncSurface()
{
    while (true) {
        OHNativeWindowBuffer *ohNativeWindowBuffer;
        int fenceFd = -1;
        if (nativeWindow == nullptr) {
            cout << "nativeWindow == nullptr" << endl;
            isRunning_.store(false);
            break;
        }

        int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &ohNativeWindowBuffer, &fenceFd);
        if (err != 0) {
            cout << "RequestBuffer failed, GSError=" << err << endl;
            isRunning_.store(false);
            break;
        }
        if (fenceFd > 0) {
            close(fenceFd);
        }
        OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
        void *virAddr = nullptr;
        err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
        if (err != 0) {
            cout << "OH_NativeBuffer_Map failed, GSError=" << err << endl;
            isRunning_.store(false);
            break;
        }
        uint8_t *dst = (uint8_t *)virAddr;
        const SurfaceBuffer *sbuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
        int stride = sbuffer->GetStride();
        if (dst == nullptr || stride < DEFAULT_WIDTH) {
            cout << "invalid va or stride=" << stride << endl;
            err = NativeWindowCancelBuffer(nativeWindow, ohNativeWindowBuffer);
            isRunning_.store(false);
            break;
        }
        stride_ = stride;
        if (frameCount == maxFrameInput) {
            err = OH_VideoEncoder_NotifyEndOfStream(venc_);
            if (err != 0) {
                cout << "OH_VideoEncoder_NotifyEndOfStream failed" << endl;
                isRunning_.store(false);
            }
            break;
        }
        if (FlushSurf(ohNativeWindowBuffer, nativeBuffer)) {
            break;
        }
        usleep(FRAME_INTERVAL);
        frameCount++;
    }
}

void VEncAPI11FuzzSample::SetEOS(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t res = OH_VideoEncoder_PushInputBuffer(venc_, index);
    cout << "OH_VideoEncoder_PushInputBuffer    EOS   res: " << res << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inIdxQueue_.pop();
    signal_->inBufferQueue_.pop();
}

void VEncAPI11FuzzSample::InputFunc()
{
    errCount = 0;
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() {
            if (!isRunning_.load()) {
                return true;
            }
            return signal_->inIdxQueue_.size() > 0;
        });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inIdxQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();

        lock.unlock();
        OH_AVCodecBufferAttr attr;
        uint8_t *fileBuffer = OH_AVBuffer_GetAddr(buffer);
        if (fileBuffer == nullptr) {
            break;
        }
        attr.size = OH_AVBuffer_GetCapacity(buffer);
        if (frameCount == maxFrameInput) {
            SetEOS(index, buffer);
            break;
        }
        attr.pts = GetSystemTimeUs();
        attr.offset = 0;
        attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
        OH_AVBuffer_SetBufferAttr(buffer, &attr);
        OH_VideoEncoder_PushInputBuffer(venc_, index);
        frameCount++;
        signal_->inIdxQueue_.pop();
        signal_->inBufferQueue_.pop();
        if (sleepOnFPS) {
            usleep(FRAME_INTERVAL);
        }
    }
}

int32_t VEncAPI11FuzzSample::CheckAttrFlag(OH_AVCodecBufferAttr attr)
{
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        cout << "attr.flags == AVCODEC_BUFFER_FLAGS_EOS" << endl;
        unique_lock<mutex> inLock(signal_->inMutex_);
        isRunning_.store(false);
        signal_->inCond_.notify_all();
        inLock.unlock();
        return -1;
    }
    if (attr.flags == AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
        cout << "enc AVCODEC_BUFFER_FLAGS_CODEC_DATA" << attr.pts << endl;
    }
    return 0;
}

int32_t VEncAPI11FuzzSample::Flush()
{
    unique_lock<mutex> inLock(signal_->inMutex_);
    clearIntqueue(signal_->inIdxQueue_);
    signal_->inCond_.notify_all();
    inLock.unlock();
    return OH_VideoEncoder_Flush(venc_);
}

int32_t VEncAPI11FuzzSample::Reset()
{
    isRunning_.store(false);
    StopInloop();
    return OH_VideoEncoder_Reset(venc_);
}

int32_t VEncAPI11FuzzSample::Release()
{
    int ret = OH_VideoEncoder_Destroy(venc_);
    venc_ = nullptr;
    if (signal_ != nullptr) {
        delete signal_;
        signal_ = nullptr;
    }
    return ret;
}

int32_t VEncAPI11FuzzSample::Stop()
{
    StopInloop();
    return OH_VideoEncoder_Stop(venc_);
}

int32_t VEncAPI11FuzzSample::Start()
{
    return OH_VideoEncoder_Start(venc_);
}

int32_t VEncAPI11FuzzSample::SetParameter(int32_t data)
{
    if (venc_) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (format == nullptr) {
            return AV_ERR_UNKNOWN;
        }
        double frameRate = data;
        (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, data);
        int ret = OH_VideoEncoder_SetParameter(venc_, format);
        OH_AVFormat_Destroy(format);
        return ret;
    }
    return AV_ERR_UNKNOWN;
}