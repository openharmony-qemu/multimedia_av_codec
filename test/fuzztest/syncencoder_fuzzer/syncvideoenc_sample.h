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
class VEncAPI11Signal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
};
struct fileInfo {
    std::string fileDir;
    OH_NativeBuffer_Format format;
    uint32_t width;
    uint32_t height;
};

typedef struct LtrTestParameter {
    uint32_t ltrInterval = 0;
    bool enableUseLtr = false;
    bool useBadLtr = false;
    bool useLtrOnce = false;
    int32_t useLtrIndex = 0;
    bool markAndUseSelf = false;
    int32_t ltrCount = 0;
}LtrTestParameter;

class VEncSyncSample : public NoCopyable {
public:
    VEncSyncSample() = default;
    ~VEncSyncSample();
    const char *inpDir = "/data/test/media/1280_720_nv.yuv";
    const char *outDir = "/data/test/media/VEncTest.h264";
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
    int32_t DEFAULT_PROFILE = HEVC_PROFILE_MAIN;
    OH_AVPixelFormat DEFAULT_PIX_FMT = AV_PIXEL_FORMAT_NV12;
    uint32_t defaultKeyFrameInterval = 1000;
    uint32_t defaultRangeFlag = 0;
    uint32_t DEFAULT_COLOR_PRIMARIES = COLOR_PRIMARY_BT709;
    uint32_t DEFAULT_TRANSFER_CHARACTERISTICS = TRANSFER_CHARACTERISTIC_BT709;
    uint32_t DEFAULT_MATRIX_COEFFICIENTS = MATRIX_COEFFICIENT_BT709;
    uint32_t repeatTime = 0;
    int32_t enbleBFrameMode = 0;
    int32_t CreateVideoEncoder(const char *codecName);
    int32_t ConfigureVideoEncoder();
    int32_t ConfigureVideoEncoderTemporal(int32_t temporalGopSize);
    int32_t ConfigureVideoEncoderFuzz(int32_t data);
    int32_t SetVideoEncoderCallback();
    int32_t CreateSurface();
    int32_t StartVideoEncoder();
    int32_t VideoEncoder();
    int32_t SetParameter(OH_AVFormat *format);
    void SetBufferParameter(OH_AVBuffer *buffer);
    void SetLTRParameter(OH_AVBuffer *buffer);
    void SetForceIDR();
    void GetStride();
    void TestApi();
    void WaitForEOS();
    int32_t SyncOutputFuncEos(uint32_t &lastIndex, uint32_t &outFrames, uint32_t &index,
        OH_AVBuffer *buffer, OH_AVCodecBufferAttr &attr);
    int32_t OpenFile();
    uint32_t ReturnZeroIfEOS(uint32_t expectedSize);
    int64_t GetSystemTimeUs();
    int32_t Start();
    int32_t Flush();
    int32_t Reset();
    int32_t Stop();
    int32_t Release();
    int32_t SetParameter(int32_t data);
    int32_t ReadFile(uint32_t index, OH_AVBuffer *buffer);
    void FlushBuffer();
    void AutoSwitchParam();
    void RepeatStartBeforeEOS();
    bool RandomEOS(uint32_t index);
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    int32_t PushData(OH_AVBuffer *buffer, uint32_t index, int32_t &result);
    int32_t CheckResult(bool isRandomEosSuccess, int32_t pushResult);
    void InputFunc();
    void SyncInputFunc();
    int32_t StateEos();
    void InputFuncSurface();
    uint32_t ReadOneFrameYUV420SP(uint8_t *dst);
    uint32_t ReadOneFrameRGBA8888(uint8_t *dst);
    uint32_t ReadOneFrameYUVP010(uint8_t *dst);
    uint32_t ReadOneFrameFromList(uint8_t *dst, int32_t &fileIndex);
    uint32_t ReadOneFrameByType(uint8_t *dst, OH_NativeBuffer_Format format);
    int32_t OpenFileFail();
    int32_t CheckAttrFlag(OH_AVCodecBufferAttr attr);
    void OutputFuncFail();
    void OutputFunc();
    void SyncOutputFunc();
    uint32_t FlushSurf(OHNativeWindowBuffer *ohNativeWindowBuffer, OH_NativeBuffer *nativeBuffer);
    void ReleaseSignal();
    void ReleaseInFile();
    void StopInloop();
    void StopOutloop();
    void DumpLtrInfo(OH_AVBuffer *buffer);
    void DumpQPInfo(OH_AVBuffer *buffer);
    void DumpInfo(OH_AVCodecBufferAttr attr, OH_AVBuffer *buffer);
    void ReadMultiFilesFunc();
    int32_t InitBuffer(OHNativeWindowBuffer *&ohNativeWindowBuffer, OH_NativeBuffer *&nativeBuffer, uint8_t *&dst);
    void InputEnableRepeatSleep();
    int32_t QueryInputBuffer(uint32_t index, int64_t timeoutUs);
    int32_t QueryOutputBuffer(uint32_t index, int64_t timeoutUs);
    OH_AVBuffer *GetInputBuffer(uint32_t index);
    OH_AVBuffer *GetOutputBuffer(uint32_t index);
    VEncAPI11Signal *signal_;
    uint32_t errCount = 0;
    bool enableForceIDR = false;
    uint32_t outCount = 0;
    int32_t frameCount = 0;
    int32_t codecType = 0;
    int32_t switchParamsTimeSec = 3;
    bool sleepOnFPS = false;
    bool surfInput = false;
    bool enableAutoSwitchParam = false;
    bool enableColorSpaceParams = false;
    bool enableAutoSwitchBufferParam = false;
    bool needResetBitrate = false;
    bool needResetFrameRate = false;
    bool needResetQP = false;
    bool enableQP = false;
    bool repeatRun = false;
    bool showLog = false;
    bool enableLTR = false;
    bool fuzzMode = false;
    bool getInputBufferIndexRepeat = false;
    bool abnormalIndexValue = false;
    bool getOutputBufferIndexNoExisted = false;
    int64_t encodeCount = 0;
    bool enableRandomEos = false;
    uint32_t repeatStartStopBeforeEos = 0;  // 1200 测试用例
    uint32_t repeatStartFlushBeforeEos = 0; // 1300 测试用例
    int64_t startTime = 0;
    int64_t endTime = 0;
    LtrTestParameter ltrParam;
    bool temporalConfig = false;
    bool temporalEnable = false;
    bool temporalJumpMode = false;
    bool temporalDefault = false;
    bool temporalUniformly = false;
    bool getQpMse = false;
    std::vector<fileInfo> fileInfos;
    bool readMultiFiles = false;
    bool setFormatRbgx = false;
    bool configMain = false;
    bool configMain10 = false;
    bool setFormat8Bit = false;
    bool setFormat10Bit = false;
    bool enableRepeat = false;
    bool enableSeekEos = false;
    bool setMaxCount = false;
    int32_t defaultFrameAfter = 1;
    int32_t defaultMaxCount = 1;
    uint32_t inCount = 0;
    int32_t enbleSyncMode = 0;
    int64_t syncInputWaitTime = -1;
    int64_t syncOutputWaitTime = -1;
    bool queryOutputBufferEOS = false;
    bool queryInputBufferEOS = false;
    bool getOutputBufferIndexRepeated = false;
    bool noDestroy = false;
    std::atomic<bool> isRunning_ { false };
    int32_t frameIndex_ = 0;
private:
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::unordered_map<uint32_t, OH_AVBuffer *> inBufferMap_;
    std::unordered_map<uint32_t, OH_AVBuffer *> outBufferMap_;
    OH_AVCodec *venc_;
    OH_AVCodecCallback cb_;
    int64_t timeStamp_ { 0 };
    int64_t lastRenderedTimeUs_ { 0 };
    bool isFirstFrame_ = true;
    OHNativeWindow *nativeWindow;
    int stride_;
    static constexpr uint32_t sampleRatio = 2;
};
} // namespace Media
} // namespace OHOS

#endif // VIDEOENC_API11_SAMPLE_H
