/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef VIDEOTRANSCODE_API11_SAMPLE_H
#define VIDEOTRANSCODE_API11_SAMPLE_H

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
#include <condition_variable>

#include "native_avcodec_base.h"

#include "securec.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_videoencoder.h"
#include "nocopyable.h"
#include "native_avmemory.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "surface/window.h"


namespace OHOS {
namespace Media {
class VSignal {
public:
    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
};

class VideoTransCodeApi11Sample : public NoCopyable {
public:
    VideoTransCodeApi11Sample() = default;
    ~VideoTransCodeApi11Sample();
    int32_t Configure();
    int32_t Start();
    int32_t Release();
    void WaitForEos();
    void ReleaseInFile();
    void InputFunc();
    int32_t CreateVideocoder(std::string codeName, std::string enCodeName);
    int32_t SendData(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer);
    int32_t StartSecondaryEncoder();
    const char *g_inpDir = "/data/test/media/1920_1080_10_30Mb.h264";
    int32_t g_defaultWidth = 1920;
    int32_t g_defaultHeight = 1080;
    int32_t g_defaultPixelFormat = AV_PIXEL_FORMAT_NV12;
    double g_defaultFrameRate = 30.0;
    uint32_t frameCountDec = 0;
    uint32_t frameCountEnc = 0;
    uint32_t frameCountEncSecondary = 0;
    int32_t g_defaultProfile = HEVC_PROFILE_MAIN;
    VSignal *decSignal;
    std::unique_ptr<std::thread> inputLoop_;
    OH_AVCodec *vdec_;
    OH_AVCodec *venc_;
    OH_AVCodec *vencSecondary;
    uint32_t errorCount = 0;
    int32_t inputNum = 0;
    bool dualEos = false;
    bool enableDownsampling = true;
    int32_t downsamplingWidth = 360;
    int32_t downsamplingHeight = 280;
    int32_t cropLeft = 10;
    int32_t cropRight = 301;
    int32_t cropTop = 10;
    int32_t cropBottom = 201;
    double dropFrameRate = 5.00;

private:
    OHNativeWindow *window = nullptr;
    OH_AVCodecCallback encCb_;
    OH_AVCodecCallback decCb_;
};
} // namespace Media
} // namespace OHOS

#endif
