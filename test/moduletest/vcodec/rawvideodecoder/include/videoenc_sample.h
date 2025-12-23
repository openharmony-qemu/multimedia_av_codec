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
#include "window.h"
#include "media_description.h"
#include "av_common.h"
#include "external_window.h"
#include "native_buffer_inner.h"

namespace OHOS {
namespace Media {
class VEncSignal {
public:
    std::mutex inMutex;
    std::mutex outMutex;
    std::mutex flushMutex;
    std::condition_variable inCond;
    std::condition_variable outCond;
    std::queue<uint32_t> inIdxQueue;
    std::queue<uint32_t> outIdxQueue;
    std::queue<OH_AVCodecBufferAttr> attrQueue;
    std::queue<OH_AVMemory *> inBufferQueue;
    std::queue<OH_AVMemory *> outBufferQueue;
};

class VEncNdkSample : public NoCopyable {
public:
    VEncNdkSample() = default;
    ~VEncNdkSample();
    int32_t CreateVideoEncoder(const char *codecName);
    int32_t ConfigureVideoEncoder();
    int32_t ConfigureVideoEncoder_Temporal(int32_t temporal_gop_size);
    int32_t ConfigureVideoEncoder_fuzz(int32_t data);
    int32_t SetVideoEncoderCallback();
    int32_t CreateSurface();
    int32_t StartVideoEncoder();
    int32_t SetParameter(OH_AVFormat *format);
    void SetForceIDR();
    void GetStride();
    void testApi();
    void WaitForEOS();
    int32_t OpenFile();
    uint32_t ReturnZeroIfEOS(uint32_t expectedSize);
    int64_t GetSystemTimeUs();
    int32_t Start();
    int32_t Flush();
    int32_t Reset();
    int32_t Stop();
    int32_t Release();
    void Flush_buffer();
    void AutoSwitchParam();
    void RepeatStartBeforeEOS();
    bool RandomEOS(uint32_t index);
    void SetEOS(uint32_t index);
    int32_t PushData(OH_AVMemory *buffer, uint32_t index, int32_t &result);
    void InputDataNormal(bool &runningFlag, uint32_t index, OH_AVMemory *buffer);
    void InputDataFuzz(bool &runningFlag, uint32_t index);
    int32_t CheckResult(bool isRandomEosSuccess, int32_t pushResult);
    void InputFunc();
    int32_t state_EOS();
    void InputFuncSurface();
    uint32_t ReadOneFrameYUV420SP(uint8_t *dst);
    void ReadOneFrameRGBA8888(uint8_t *dst);
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    void OutputFuncFail();
    void OutputFunc();
    uint32_t FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer);
    void ReleaseSignal();
    void ReleaseInFile();
    void StopInloop();
    void StopOutloop();

    const char *inputDir = "";
    const char *outputDir = "/data/test/media/Mpeg1EncTest.h264";
    uint32_t defaultHeight = 720;
    uint32_t defaultWidth = 480;
    double defaultFrameRate = 30.0;
    VEncSignal *signal = nullptr;
    uint32_t errCount = 0;
    std::atomic<bool> isRunning = false ;
    std::atomic<bool> isFlushing_ = false ;
    bool inputCallbackFlush = false;
    bool inputCallbackStop = false;
    bool outputCallbackFlush = false;
    bool outputCallbackStop = false;
private:
    bool enableForceIDR_ = false;
    uint32_t outCount_ = 0;
    uint32_t frameCount_ = 0;
    uint32_t switchParamsTimeSec_ = 3;
    bool sleepOnFPS_ = false;
    bool SURF_INPUT_ = false;
    bool enableAutoSwitchParam_ = false;
    bool needResetBitrate_ = false;
    bool needResetFrameRate_ = false;
    bool needResetQP_ = false;
    bool repeatRun_ = false;
    bool showLog_ = false;
    bool fuzzMode_ = false;
    int64_t encodeCount_ = 0;
    bool enableRandomEos_ = false;
    uint32_t repeatStartStopBeforeEos_ = 0;
    uint32_t repeatStartFlushBeforeEos_ = 0;
    int64_t startTime_ = 0;
    int64_t endTime_ = 0;
    uint32_t randomEos_ = 0;
    bool temporalConfig_ = false;
    bool temporalEnable = false;
    bool temporalJumpMode_ = false;
    bool temporalDefault = false;
    uint32_t defaultBitrate_ = 5000000;
    uint32_t defaultQuality_ = 30;
    uint32_t defaultFuzzTime_ = 30;
    uint32_t defaultBitrateMode_ = CBR;
    uint32_t defaultRangeFlag_ = 0;
    OH_AVPixelFormat defaultPixFmt_ = AV_PIXEL_FORMAT_NV12;
    uint32_t defaultKeyFrameInterval_ = 1000;
    uint32_t repeatTime_ = 0;
    std::unique_ptr<std::ifstream> inFile_ = nullptr;
    std::unique_ptr<std::thread> inputLoop_ = nullptr;
    std::unique_ptr<std::thread> outputLoop_ = nullptr;
    std::unordered_map<uint32_t, OH_AVMemory *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVMemory *> outBufferMap_;
    OH_AVCodec *venc_ = nullptr;
    OH_AVCodecAsyncCallback cb_;
    int64_t timeStamp_ = 0;
    int64_t lastRenderedTimeUs_ = 0;
    bool isFirstFrame_ = true;
    OHNativeWindow *nativeWindow_ = nullptr;
    int stride_ = 0;
    uint32_t sampleRatio_ = 2;
};
} // namespace Media
} // namespace OHOS

#endif // VIDEODEC_SAMPLE_H
