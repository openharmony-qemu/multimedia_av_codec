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

#ifndef VIDEODEC_API11_SAMPLE_H
#define VIDEODEC_API11_SAMPLE_H

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
#include "native_avcodec_videodecoder.h"
#include "nocopyable.h"
#include "native_avmemory.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "window.h"
#include "iconsumer_surface.h"

namespace OHOS {
namespace Media {

class VDecSyncSample : public NoCopyable {
public:
    VDecSyncSample() = default;
    ~VDecSyncSample();
    const char *outDir = "/data/test/media/VDecTest.yuv";
    const char *outDir2 = "/data/test/media/VDecTest2.yuv";
    bool sfOutput = false;
    bool transferFlag = false;
    bool nV21Flag = false;
    uint32_t defaultWidth = 1920;
    uint32_t defaultHeight = 1080;
    uint32_t originalWidth = 0;
    uint32_t originalHeight = 0;
    int enbleBlankFrame = 0;
    uint32_t defualtPixelFormat = AV_PIXEL_FORMAT_NV12;
    int maxSurfNum = 2;
    int32_t codecType = 0;
    double defaultFrameRate = 30.0;
    // 解码输出数据预期
    bool enableVRR = false;
    bool enableLowLatency = false;
    int32_t enbleSyncMode = 0;
    int64_t syncInputWaitTime = -1;
    int64_t syncOutputWaitTime = -1;
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t ConfigureVideoDecoder();
    int64_t GetSystemTimeUs();
    int32_t CreateVideoDecoder(std::string codeName);
    int32_t Release();
    int32_t SyncInputFuncFuzz(const uint8_t *data, size_t size);
    int32_t SyncOutputFuncFuzz();
    void CreateSurface();
    void ReleaseInFile();
    bool isRenderAttime = false;
    uint32_t errCount = 0;
    int64_t renderTimestampNs = 0;
    int32_t maxInputSize = 0;
    bool autoSwitchSurface = false;
    std::atomic<bool> isFlushing_ { false };
    std::atomic<bool> isRunning_ { false };
    bool useHDRSource = false;
    int32_t DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
    int32_t DecodeSetSurface();
    void SetParameter(int32_t data, int32_t data1);
private:
    std::unique_ptr<std::ifstream> inFile_;
    OH_AVCodec *vdec_;
    OHNativeWindow *nativeWindow[2] = {};
    sptr<Surface> cs[2] = {};
    sptr<Surface> ps[2] = {};
};
} // namespace Media
} // namespace OHOS
#endif // VIDEODEC_SAMPLE_H
