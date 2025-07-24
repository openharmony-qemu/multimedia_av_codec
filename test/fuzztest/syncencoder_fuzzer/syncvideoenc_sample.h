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

#ifndef VIDEOENC_API11_SAMPLE_H
#define VIDEOENC_API11_SAMPLE_H

#include <iostream>
#include <cstdio>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>
#include <unordered_map>
#include "securec.h"
#include "native_avcodec_videoencoder.h"
#include "nocopyable.h"
#include "native_avbuffer.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "window.h"
#include "media_description.h"
#include "av_common.h"
#include "external_window.h"
#include "native_buffer.h"
namespace OHOS {
namespace Media {
class VEncSyncSample : public NoCopyable {
public:
    VEncSyncSample() = default;
    ~VEncSyncSample();
    uint32_t defaultWidth = 1280;
    uint32_t defaultHeight = 720;
    uint32_t defaultBitrate = 5000000;
    uint32_t defaultQuality = 30;
    double defaultFrameRate = 30.0;
    bool isAVCEncoder = true;
    const uint8_t *fuzzData;
    int32_t maxFrameInput = 20;
    size_t fuzzSize;
    OH_HEVCProfile hevcProfile = HEVC_PROFILE_MAIN;
    OH_AVCProfile avcProfile = AVC_PROFILE_BASELINE;
    int32_t defaultQp = 20;
    uint32_t DEFAULT_BITRATE_MODE = CBR;
    OH_AVPixelFormat DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    uint32_t defaultKeyFrameInterval = 1000;
    uint32_t defaultRangeFlag = 0;
    uint32_t DEFAULT_COLOR_PRIMARIES = COLOR_PRIMARY_BT709;
    uint32_t DEFAULT_TRANSFER_CHARACTERISTICS = TRANSFER_CHARACTERISTIC_BT709;
    uint32_t DEFAULT_MATRIX_COEFFICIENTS = MATRIX_COEFFICIENT_BT709;
    int32_t enbleBFrameMode = 0;
    int32_t CreateVideoEncoder(const char *codecName);
    int32_t ConfigureVideoEncoder();
    int32_t ConfigureVideoEncoderFuzz(int32_t data);
    int32_t CreateSurface();
    int64_t GetSystemTimeUs();
    int32_t Start();
    int32_t Stop();
    int32_t Release();
    int32_t SetParameter(int32_t data);
    void SyncInputFuncFuzz();
    void InputFuncSurfaceFuzz();
    void SyncOutputFuncFuzz();
    uint32_t FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer);
    uint32_t errCount = 0;
    int32_t frameCount = 0;
    int32_t codecType = 0;
    bool sleepOnFPS = false;
    bool surfInput = false;
    bool enableColorSpaceParams = false;
    bool enableQP = false;
    bool enableLTR = false;
    bool fuzzMode = false;
    bool enableRepeat = false;
    bool setMaxCount = false;
    int32_t defaultFrameAfter = 1;
    int32_t defaultMaxCount = 1;
    int32_t enbleSyncMode = 0;
    int64_t syncInputWaitTime = -1;
    int64_t syncOutputWaitTime = -1;
    std::atomic<bool> isRunning_ { false };
    int32_t frameIndex_ = 0;
private:
    OH_AVCodec *venc_;
    OHNativeWindow *nativeWindow;
};
} // namespace Media
} // namespace OHOS

#endif // VIDEOENC_API11_SAMPLE_H
