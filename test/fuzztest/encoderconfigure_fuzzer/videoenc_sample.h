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

#ifndef VIDEOENC_SAMPLE_H
#define VIDEOENC_SAMPLE_H

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
#include "native_avmemory.h"
#include "native_avformat.h"
#include "native_averrors.h"
#include "media_description.h"
#include "av_common.h"

namespace OHOS {
namespace Media {
class VEncSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
};

class VEncFuzzSample : public NoCopyable {
public:
    VEncFuzzSample() = default;
    ~VEncFuzzSample();
    const char *inpDir = "/data/test/media/1280_720_nv.yuv";
    const char *outDir = "/data/test/media/VEncTest.h264";
    uint32_t defaultWidth = 1280;
    uint32_t defaultHeight = 720;
    uint32_t defaultBitrate = 5000000;
    uint32_t defaultQuality = 30;
    double defaultFrameRate = 30.0;
    uint32_t defaultFuzzTime = 30;
    uint32_t defaultBitrateMode = CBR;
    OH_AVPixelFormat defaultPixFmt = AV_PIXEL_FORMAT_NV12;
    uint32_t defaultKeyFrameInterval = 1000;
    int32_t CreateVideoEncoder(const char *codecName);
    int32_t ConfigureVideoEncoder();
    int32_t ConfigureVideoEncoderFuzz(int32_t data);
    int32_t SetVideoEncoderCallback();
    int32_t StartVideoEncoder();
    int32_t SetParameter(OH_AVFormat *format);
    void SetForceIDR();
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
    bool RandomEOS(uint32_t index);
    void SetEOS(uint32_t index);
    int32_t PushData(OH_AVMemory *buffer, uint32_t index, int32_t &result);
    void InputDataFuzz(bool &runningFlag, uint32_t index);
    int32_t CheckResult(bool isRandomEosSuccess, int32_t pushResult);
    void InputFunc();
    uint32_t ReadOneFrameYUV420SP(uint8_t *dst);
    void ReadOneFrameRGBA8888(uint8_t *dst);
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    void OutputFuncFail();
    void OutputFunc();
    void ReleaseSignal();
    void ReleaseInFile();
    void StopInloop();
    void StopOutloop();

    VEncSignal *signal_;
    uint32_t errCount = 0;
    bool enableForceIDR = false;
    uint32_t outCount = 0;
    uint32_t frameCount = 0;
    uint32_t switchParamsTimeSec = 3;
    bool sleepOnFPS = false;
    bool enableAutoSwitchParam = false;
    bool needResetBitrate = false;
    bool needResetFrameRate = false;
    bool needResetQP = false;
    bool repeatRun = false;
    bool showLog = false;
    bool fuzzMode = false;

private:
    std::atomic<bool> isRunning_ { false };
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::unordered_map<uint32_t, OH_AVMemory *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVMemory *> outBufferMap_;
    OH_AVCodec *venc_;
    OH_AVCodecAsyncCallback cb_;
    int64_t timeStamp_ { 0 };
    int64_t lastRenderedTimeUs_ { 0 };
    bool isFirstFrame_ = true;
    OHNativeWindow *nativeWindow;
    int stride_;
    static constexpr uint32_t sampleRatio = 2;
};
} // namespace Media
} // namespace OHOS

#endif // VIDEODEC_SAMPLE_H
