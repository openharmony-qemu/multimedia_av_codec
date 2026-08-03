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
    void SetConfig(OH_AVFormat *format);
    const char *INP_DIR = "/data/test/media/1920_1080_10_30Mb.h264";
    const char *OUT_DIR = "/data/test/media/VEncTest.h264";
    const char *OUT_DIR_SEC = "/data/test/media/VEncTest2.h264";
    int32_t DEFAULT_WIDTH = 3840;
    int32_t DEFAULT_HEIGHT = 2160;
    int32_t DEFAULT_PIXEL_FORMAT = AV_PIXEL_FORMAT_NV12;
    double DEFAULT_FRAME_RATE = 30.0;
    std::atomic<uint32_t> frameCountDec {0};
    std::atomic<uint32_t> frameCountEnc {0};
    std::atomic<uint32_t> frameCountEncSecondary {0};
    int32_t DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
    VSignal *decSignal;
    std::unique_ptr<std::thread> inputLoop_;
    OH_AVCodec *vdec_;
    OH_AVCodec *venc_;
    OH_AVCodec *vencSecondary;
    FILE *outFile_ = nullptr;
    FILE *outFileSec_ = nullptr;
    std::atomic<uint32_t> errorCount {0};
    std::atomic<int32_t> inputNum {0};
    std::atomic<bool> dualEos {false};
    bool enableDownsampling = false;
    int32_t downsamplingWidth = 0;
    int32_t downsamplingHeight = 0;

private:
    OHNativeWindow *window = nullptr;
    OH_AVCodecCallback encCb_;
    OH_AVCodecCallback decCb_;
};
} // namespace Media
} // namespace OHOS

#endif
