/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "videoenc_preproc_dual_output_sample.h"

#include <chrono>
#include <cstring>
#include <iostream>
#include <time.h>
#include <unistd.h>

#include "native_buffer.h"
#include "native_buffer_inner.h"
#include "securec.h"
#include "surface_buffer.h"

using namespace std;

namespace {
static int64_t GetTimeUs()
{
    struct timespec now {};
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    return static_cast<int64_t>(now.tv_sec) * 1000000 + static_cast<int64_t>(now.tv_nsec / 1000);
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr uint32_t kSurfaceUsage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
constexpr uint8_t kFramePatternBase = 0x20;
} // namespace

VideoEncPreprocDualOutputSample::~VideoEncPreprocDualOutputSample()
{
    if (secondary_ != nullptr) {
        (void)OH_VideoEncoder_Destroy(secondary_);
        secondary_ = nullptr;
    }
    if (primary_ != nullptr) {
        (void)OH_VideoEncoder_Destroy(primary_);
        primary_ = nullptr;
    }
    ReleaseWindow();
}

void VideoEncPreprocDualOutputSample::ResetStates()
{
    primaryState_.codec = nullptr;
    primaryState_.dataCount.store(0);
    primaryState_.gotEos.store(false);
    primaryState_.errorCode.store(0);
    secondaryState_.codec = nullptr;
    secondaryState_.dataCount.store(0);
    secondaryState_.gotEos.store(false);
    secondaryState_.errorCode.store(0);
}

void VideoEncPreprocDualOutputSample::ReleaseWindow()
{
    if (window_ != nullptr) {
        (void)OH_NativeWindow_DestroyNativeWindow(window_);
        window_ = nullptr;
    }
}

void VideoEncPreprocDualOutputSample::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    auto *state = reinterpret_cast<EncoderState *>(userData);
    if (state != nullptr) {
        state->errorCode.store(errorCode);
    }
}

void VideoEncPreprocDualOutputSample::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}

void VideoEncPreprocDualOutputSample::OnNeedOutputData(OH_AVCodec *codec, uint32_t index,
                                                       OH_AVBuffer *buffer, void *userData)
{
    if (!codec || !buffer || !userData) {
        return;
    }
    auto *state = reinterpret_cast<EncoderState *>(userData);
    OH_AVCodecBufferAttr attr;
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if ((attr.flags & AVCODEC_BUFFER_FLAGS_EOS) == 0) {
        state->dataCount.fetch_add(1);
    } else {
        state->gotEos.store(true);
        state->eosCv->notify_all();
        return;
    }
    if (codec != nullptr) {
        (void)OH_VideoEncoder_FreeOutputBuffer(codec, index);
    }
}

int32_t VideoEncPreprocDualOutputSample::RegisterCallback(OH_AVCodec *codec, EncoderState &state)
{
    if (codec == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    OH_AVCodecCallback callback {};
    callback.onError = OnError;
    callback.onStreamChanged = OnStreamChanged;
    callback.onNeedInputBuffer = nullptr;
    callback.onNewOutputBuffer = OnNeedOutputData;
    state.codec = codec;
    state.eosCv = &eosCv_;
    return OH_VideoEncoder_RegisterCallback(codec, callback, &state);
}

bool VideoEncPreprocDualOutputSample::ApplyPreprocConfig(OH_AVFormat *format, const PreprocDualOutput::PreprocConfig &config)
{
    if (format == nullptr) {
        return false;
    }
    if (!OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, config.frameRate)) {
        return false;
    }
    if (config.useDownsampling) {
        if (!OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH,
                                     config.downsamplingWidth) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT,
                                     config.downsamplingHeight)) {
            return false;
        }
    }
    if (config.useCrop) {
        if (!OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, config.cropLeft) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, config.cropTop) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, config.cropRight) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, config.cropBottom)) {
            return false;
        }
    }
    if (config.useDropToFrameRate) {
        if (!OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE,
                                        config.dropToFrameRate)) {
            return false;
        }
    }
    return OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, kWidth_) &&
           OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, kHeight_) &&
           OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
}

int32_t VideoEncPreprocDualOutputSample::BuildAndConfigure(OH_AVCodec *codec,
                                                           const PreprocDualOutput::PreprocConfig &config)
{
    if (codec == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        return AV_ERR_NO_MEMORY;
    }
    int32_t ret = AV_ERR_OK;
    if (!ApplyPreprocConfig(format, config)) {
        ret = AV_ERR_UNKNOWN;
    } else {
        ret = OH_VideoEncoder_Configure(codec, format);
    }
    OH_AVFormat_Destroy(format);
    return ret;
}

bool VideoEncPreprocDualOutputSample::CreatePrimary(const std::string &mime)
{
    ResetStates();
    if (OH_VideoEncoder_CreatePrimaryWithPreproc(mime.c_str(), &primary_) != AV_ERR_OK || primary_ == nullptr) {
        return false;
    }
    return RegisterCallback(primary_, primaryState_) == AV_ERR_OK;
}

int32_t VideoEncPreprocDualOutputSample::ConfigurePrimary(const PreprocDualOutput::ScenarioCase &scenario)
{
    return BuildAndConfigure(primary_, scenario.primary);
}

int32_t VideoEncPreprocDualOutputSample::GetPrimarySurface()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    int32_t ret = OH_VideoEncoder_GetSurface(primary_, &window_);
    if (ret != AV_ERR_OK || window_ == nullptr) {
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (ret != 0) {
        return AV_ERR_UNKNOWN;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_BUFFER_GEOMETRY, kWidth_, kHeight_);
    if (ret != 0) {
        return AV_ERR_UNKNOWN;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_USAGE, kSurfaceUsage);
    if (ret != 0) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VideoEncPreprocDualOutputSample::StartPrimary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Start(primary_);
}

int32_t VideoEncPreprocDualOutputSample::CreateSecondary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    int32_t ret = OH_VideoEncoder_CreateSecondaryFromPrimary(primary_, &secondary_);
    if (ret != AV_ERR_OK || secondary_ == nullptr) {
        return ret;
    }
    return RegisterCallback(secondary_, secondaryState_);
}

int32_t VideoEncPreprocDualOutputSample::ConfigureSecondary(const PreprocDualOutput::ScenarioCase &scenario)
{
    return BuildAndConfigure(secondary_, scenario.secondary);
}

int32_t VideoEncPreprocDualOutputSample::StartSecondary()
{
    if (secondary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Start(secondary_);
}

int32_t VideoEncPreprocDualOutputSample::DuplicateSecondary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    OH_AVCodec *duplicate = nullptr;
    int32_t ret = OH_VideoEncoder_CreateSecondaryFromPrimary(primary_, &duplicate);
    if (duplicate != nullptr) {
        (void)OH_VideoEncoder_Destroy(duplicate);
    }
    return ret;
}

int32_t VideoEncPreprocDualOutputSample::DestroyPrimary()
{
    if (primary_ == nullptr) {
        return AV_ERR_OK;
    }
    int32_t ret = OH_VideoEncoder_Destroy(primary_);
    primary_ = nullptr;
    primaryState_.codec = nullptr;
    return ret;
}

int32_t VideoEncPreprocDualOutputSample::DestroySecondary()
{
    if (secondary_ == nullptr) {
        return AV_ERR_OK;
    }
    int32_t ret = OH_VideoEncoder_Destroy(secondary_);
    secondary_ = nullptr;
    secondaryState_.codec = nullptr;
    return ret;
}

void VideoEncPreprocDualOutputSample::ForgetSecondaryHandle()
{
    secondary_ = nullptr;
    secondaryState_.codec = nullptr;
}

bool VideoEncPreprocDualOutputSample::RequestAndFlushFrame(int32_t width, int32_t height, uint8_t pattern,
                                                           int64_t timestampUs)
{
    if (window_ == nullptr) {
        return false;
    }

    OHNativeWindowBuffer *nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(window_, &nativeWindowBuffer, &fenceFd);
    if (ret != 0 || nativeWindowBuffer == nullptr) {
        return false;
    }
    if (fenceFd > 0) {
        close(fenceFd);
    }

    OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
    if (nativeBuffer == nullptr) {
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    void *virAddr = nullptr;
    ret = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
    if (ret != 0 || virAddr == nullptr) {
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    const SurfaceBuffer *surfaceBuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
    if (surfaceBuffer == nullptr) {
        (void)OH_NativeBuffer_Unmap(nativeBuffer);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    const int32_t stride = surfaceBuffer->GetStride();
    const int32_t sliceHeight = surfaceBuffer->GetHeight();
    if (stride <= 0 || sliceHeight <= 0) {
        (void)OH_NativeBuffer_Unmap(nativeBuffer);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    const size_t fillSize = static_cast<size_t>(stride) * static_cast<size_t>(sliceHeight) * 3 / 2;
    if (memset_s(virAddr, fillSize, pattern, fillSize) != EOK) {
        (void)OH_NativeBuffer_Unmap(nativeBuffer);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    (void)OH_NativeWindow_NativeWindowHandleOpt(window_, SET_UI_TIMESTAMP, timestampUs);
    ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    Region region {};
    Region::Rect rect {};
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    region.rects = &rect;
    region.rectNumber = 1;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(window_, nativeWindowBuffer, -1, region);
    return ret == 0;
}

int32_t VideoEncPreprocDualOutputSample::SendPrimaryFrames(uint32_t frameCount)
{
    if (window_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    for (uint32_t i = 0; i < frameCount; ++i) {
        if (!RequestAndFlushFrame(kWidth_, kHeight_, static_cast<uint8_t>(kFramePatternBase + i), GetTimeUs())) {
            return AV_ERR_UNKNOWN;
        }
        usleep(1000);
    }
    auto ret = OH_VideoEncoder_NotifyEndOfStream(primary_);
    return (ret == AV_ERR_OK && secondary_ != nullptr) ? OH_VideoEncoder_NotifyEndOfStream(secondary_) : ret;
}

bool VideoEncPreprocDualOutputSample::WaitForDualEos(uint32_t timeoutSeconds)
{
    unique_lock<mutex> lock(eosMutex_);
    return eosCv_.wait_for(lock, chrono::seconds(timeoutSeconds), [this]() {
        return primaryState_.gotEos.load() && secondaryState_.gotEos.load();
    });
}

bool VideoEncPreprocDualOutputSample::PrimaryHasData() const
{
    return primaryState_.dataCount.load() > 0;
}

bool VideoEncPreprocDualOutputSample::SecondaryHasData() const
{
    return secondaryState_.dataCount.load() > 0;
}

bool VideoEncPreprocDualOutputSample::PrimaryGotEos() const
{
    return primaryState_.gotEos.load();
}

bool VideoEncPreprocDualOutputSample::SecondaryGotEos() const
{
    return secondaryState_.gotEos.load();
}

int32_t VideoEncPreprocDualOutputSample::PrimaryError() const
{
    return primaryState_.errorCode.load();
}

int32_t VideoEncPreprocDualOutputSample::SecondaryError() const
{
    return secondaryState_.errorCode.load();
}
} // namespace MediaAVCodec
} // namespace OHOS
