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
#include "surface/window.h"
#include "media_description.h"
#include "av_common.h"
#include "external_window.h"
#include "native_buffer_inner.h"
namespace OHOS {
namespace Media {
class VEncSignal {
public:
    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
};

class VEncAPI11FuzzSample : public NoCopyable {
public:
    VEncAPI11FuzzSample() = default;
    ~VEncAPI11FuzzSample();
    uint32_t defaultWidth = 1280;
    uint32_t defaultHeight = 720;
    uint32_t defaultBitRate = 5000000;
    uint32_t defaultQuality = 30;
    double defaultFrameRate = 30.0;
    uint32_t maxFrameInput = 20;
    int32_t defaultQP = 20;
    bool fuzzMode = true;
    uint32_t defaultBitrateMode = CBR;
    OH_AVPixelFormat defaultPixFmt = AV_PIXEL_FORMAT_NV12;
    uint32_t defaultKeyFrameInterval = 1000;
    const char *inpDir = "/data/test/media/1280_720_nv.yuv";
    int32_t CreateVideoEncoder();
    int32_t ConfigureVideoEncoderFuzz(int32_t data);
    int32_t ConfigureVideoEncoder();
    int32_t SetVideoEncoderCallback();
    int32_t CreateSurface();
    int32_t StartVideoEncoder();
    int32_t SetParameter(int32_t data);
    void GetStride();
    void WaitForEOS();
    int32_t OpenFile();
    uint32_t ReturnZeroIfEOS(uint32_t expectedSize);
    int64_t GetSystemTimeUs();
    int32_t Start();
    int32_t Flush();
    int32_t Reset();
    int32_t Stop();
    int32_t Release();
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    void InputFunc();
    void InputFuncSurface();
    uint32_t ReadOneFrameYUV420SP(uint8_t *dst);
    void ReleaseInFile();
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    uint32_t FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer);
    void ReleaseSignal();
    int32_t PushData(OH_AVBuffer *buffer, uint32_t index, int32_t &result);
    void StopInloop();
    VEncSignal *signal_;
    uint32_t errCount = 0;
    uint32_t frameCount = 0;
    bool sleepOnFPS = false;
    bool surfInput = false;
private:
    std::atomic<bool> isRunning_ { false };
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::ifstream> inFile_;
    std::unordered_map<uint32_t, OH_AVBuffer *> inBufferMap_;
    OH_AVCodec *venc_;
    OH_AVCodecCallback cb_;
    int64_t timeStamp_ { 0 };
    int64_t lastRenderedTimeUs_ { 0 };
    bool isFirstFrame_ = true;
    OHNativeWindow *nativeWindow;
    int stride_;
};
} // namespace Media
} // namespace OHOS

#endif // VIDEOENC_API11_SAMPLE_H
