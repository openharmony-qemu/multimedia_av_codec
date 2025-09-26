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
#include "native_buffer_inner.h"
#include "syncvideoenc_sample.h"
#include "native_avcapability.h"
#include <random>
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;

namespace {
constexpr int64_t NANOS_IN_SECOND = 1000000000L;
constexpr int64_t NANOS_IN_MICRO = 1000L;
constexpr uint32_t FRAME_INTERVAL = 16666;
constexpr uint32_t MAX_PIXEL_FMT = 5;
constexpr uint32_t DOUBLE = 2;
constexpr uint32_t THREE = 3;
OH_AVCapability *cap = nullptr;
VEncSyncSample *g_encSample = nullptr;
constexpr int32_t TIMESTAMP_BASE = 1000000;
constexpr int32_t DURATION_BASE = 46000;
} // namespace

VEncSyncSample::~VEncSyncSample()
{
    if (surfInput && nativeWindow) {
        OH_NativeWindow_DestroyNativeWindow(nativeWindow);
        nativeWindow = nullptr;
    }
    Stop();
    Release();
}

int64_t VEncSyncSample::GetSystemTimeUs()
{
    struct timespec now;
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    int64_t nanoTime = static_cast<int64_t>(now.tv_sec) * NANOS_IN_SECOND + now.tv_nsec;

    return nanoTime / NANOS_IN_MICRO;
}

int32_t VEncSyncSample::ConfigureVideoEncoder()
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, defaultWidth);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, defaultHeight);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DEFAULT_PIX_FMT);
    (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, defaultFrameRate);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_I_FRAME_INTERVAL, defaultKeyFrameInterval);
    if (isAVCEncoder) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, avcProfile);
    } else {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, hevcProfile);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, HEVC_PROFILE_MAIN);
    if (DEFAULT_BITRATE_MODE == CQ) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_QUALITY, defaultQuality);
    } else {
        (void)OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, defaultBitrate);
    }
    if (enableQP) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MAX, defaultQp);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_QP_MIN, defaultQp);
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, DEFAULT_BITRATE_MODE);
    if (enableColorSpaceParams) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_RANGE_FLAG, defaultRangeFlag);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_COLOR_PRIMARIES, DEFAULT_COLOR_PRIMARIES);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_TRANSFER_CHARACTERISTICS, DEFAULT_TRANSFER_CHARACTERISTICS);
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_MATRIX_COEFFICIENTS, DEFAULT_MATRIX_COEFFICIENTS);
    }
    if (enableRepeat) {
        (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, defaultFrameAfter);
        if (setMaxCount) {
            (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, defaultMaxCount);
        }
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, enbleSyncMode);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME, enbleBFrameMode);
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncSyncSample::ConfigureVideoEncoderFuzz(int32_t data)
{
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        cout << "Fatal: Failed to create format" << endl;
        return AV_ERR_UNKNOWN;
    }
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, data);
    defaultWidth = data;
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, data);
    defaultHeight = data;
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
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, enbleSyncMode);
    int ret = OH_VideoEncoder_Configure(venc_, format);
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VEncSyncSample::CreateSurface()
{
    int32_t ret = 0;
    ret = OH_VideoEncoder_GetSurface(venc_, &nativeWindow);
    if (ret != AV_ERR_OK) {
        cout << "OH_VideoEncoder_GetSurface fail" << endl;
        return ret;
    }
    if (DEFAULT_PIX_FMT == AV_PIXEL_FORMAT_RGBA1010102) {
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, NATIVEBUFFER_PIXEL_FMT_RGBA_1010102);
    } else {
        ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    }
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(nativeWindow, SET_BUFFER_GEOMETRY, defaultWidth, defaultHeight);
    if (ret != AV_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return ret;
    }
    return AV_ERR_OK;
}

int32_t VEncSyncSample::CreateVideoEncoder(const char *codecName)
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (!strcmp(codecName, tmpCodecName)) {
        isAVCEncoder = true;
    } else {
        isAVCEncoder = false;
    }
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    g_encSample = this;
    return venc_ == nullptr ? AV_ERR_UNKNOWN : AV_ERR_OK;
}

uint32_t VEncSyncSample::FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer)
{
    int32_t ret = 0;
    struct Region region;
    struct Region::Rect *rect = new Region::Rect();
    rect->x = 0;
    rect->y = 0;
    rect->w = defaultWidth;
    rect->h = defaultHeight;
    region.rects = rect;
    NativeWindowHandleOpt(nativeWindow, SET_UI_TIMESTAMP, GetSystemTimeUs());
    ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        cout << "OH_NativeBuffer_Unmap failed" << endl;
        delete rect;
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowFlushBuffer(nativeWindow, ohNativeWindowBuffer, -1, region);
    delete rect;
    if (ret != 0) {
        cout << "FlushBuffer failed" << endl;
        return ret;
    }
    return ret;
}

void VEncSyncSample::InputFuncSurfaceFuzz()
{
    OHNativeWindowBuffer *ohNativeWindowBuffer;
    int fenceFd = -1;
    if (nativeWindow == nullptr) {
        return;
    }
    int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(nativeWindow, &ohNativeWindowBuffer, &fenceFd);
    if (err != 0) {
        return;
    }
    if (fenceFd > 0) {
        close(fenceFd);
    }
    OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(ohNativeWindowBuffer);
    void *virAddr = nullptr;
    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig (nativeBuffer, &config);
    err = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
    if (err != 0) {
        return;
    }
    uint8_t *dst = (uint8_t *)virAddr;
    if (dst == nullptr || fuzzData == nullptr) {
        return;
    }
    int32_t frameSize = (config.stride * config.height * THREE) / DOUBLE;
    if (frameSize >= fuzzSize) {
        if (memcpy_s(dst, frameSize, fuzzData, fuzzSize) != EOK) {
            return;
        }
    } else {
        if (memcpy_s(dst, frameSize, fuzzData, frameSize) != EOK) {
            return;
        }
    }
    if (frameCount == maxFrameInput) {
        err = OH_VideoEncoder_NotifyEndOfStream(venc_);
        if (err != 0) {
            isRunning_.store(false);
        }
        return;
    }
    if (FlushSurf(ohNativeWindowBuffer, nativeBuffer)) {
        return;
    }
    usleep(FRAME_INTERVAL);
    frameCount++;
}

void VEncSyncSample::SyncInputFuncFuzz()
{
    uint32_t index;
    if (OH_VideoEncoder_QueryInputBuffer(venc_, &index, syncInputWaitTime) != AV_ERR_OK) {
        return;
    }
    OH_AVBuffer *buffer = OH_VideoEncoder_GetInputBuffer(venc_, index);
    if (buffer == nullptr) {
        cout << "OH_VideoEncoder_GetInputBuffer fail" << endl;
        errCount = errCount + 1;
        return;
    }
    OH_AVCodecBufferAttr attr;
    int32_t bufferSize = OH_AVBuffer_GetCapacity(buffer);
    uint8_t *fileBuffer = OH_AVBuffer_GetAddr(buffer);
    if (fileBuffer == nullptr || fuzzData == nullptr) {
        return;
    }
    if (bufferSize >= fuzzSize) {
        if (memcpy_s(fileBuffer, bufferSize, fuzzData, fuzzSize) != EOK) {
            return;
        }
        attr.size = fuzzSize;
    } else {
        if (memcpy_s(fileBuffer, bufferSize, fuzzData, bufferSize) != EOK) {
            return;
        }
        attr.size = bufferSize;
    }
    attr.pts = TIMESTAMP_BASE + DURATION_BASE * frameIndex_;
    frameIndex_++;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    OH_VideoEncoder_PushInputBuffer(venc_, index);
    frameCount++;
    if (sleepOnFPS) {
        usleep(FRAME_INTERVAL);
    }
}

void VEncSyncSample::SyncOutputFuncFuzz()
{
    uint32_t index = 0;
    int32_t ret = OH_VideoEncoder_QueryOutputBuffer(venc_, &index, syncOutputWaitTime);
    if (ret != AV_ERR_OK) {
        return;
    }
    OH_AVBuffer *buffer = OH_VideoEncoder_GetOutputBuffer(venc_, index);
    if (buffer == nullptr) {
        cout << "OH_VideoEncoder_GetOutputBuffer fail" << endl;
        errCount = errCount + 1;
        return;
    }

    if (OH_VideoEncoder_FreeOutputBuffer(venc_, index) != AV_ERR_OK) {
        cout << "Fatal: ReleaseOutputBuffer fail" << endl;
        errCount = errCount + 1;
    }
}

int32_t VEncSyncSample::Release()
{
    int ret = OH_VideoEncoder_Destroy(venc_);
    venc_ = nullptr;
    return ret;
}

int32_t VEncSyncSample::Stop()
{
    return OH_VideoEncoder_Stop(venc_);
}

int32_t VEncSyncSample::Start()
{
    return OH_VideoEncoder_Start(venc_);
}

int32_t VEncSyncSample::SetParameter(int32_t data, int32_t data1)
{
    if (venc_) {
        OH_AVFormat *format = OH_AVFormat_Create();
        if (format == nullptr) {
            return AV_ERR_UNKNOWN;
        }
        double frameRate = data;
        (void)OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, frameRate);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, data1);
        int ret = OH_VideoEncoder_SetParameter(venc_, format);
        OH_AVFormat_Destroy(format);
        return ret;
    }
    return AV_ERR_UNKNOWN;
}