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

#ifndef HDRCODEC_SAMPLE_H
#define HDRCODEC_SAMPLE_H

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
#include "avcodec_video_encoder.h"
#include "nocopyable.h"
#include "buffer/avsharedmemory.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "surface/window.h"
#include "media_description.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "external_window.h"
#include "avcodec_video_decoder.h"

namespace OHOS {
namespace MediaAVCodec {
class InnerSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<std::shared_ptr<AVSharedMemory>> inBufferQueue_;
    std::queue<std::shared_ptr<AVSharedMemory>> outBufferQueue_;
};

class HdrEncInnerCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit HdrEncInnerCallback(std::shared_ptr<InnerSignal> signal);
    ~HdrEncInnerCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format& format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
        std::shared_ptr<AVSharedMemory> buffer) override;

private:
    std::shared_ptr<InnerSignal> encInnersignal_;
};

class HdrDecInnerCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit HdrDecInnerCallback(std::shared_ptr<InnerSignal> signal);
    ~HdrDecInnerCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format& format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
        std::shared_ptr<AVSharedMemory> buffer) override;

private:
    std::shared_ptr<InnerSignal> decInnersignal_;
};

class HDRCodecInnderNdkSample : public NoCopyable {
public:
    HDRCodecInnderNdkSample() = default;
    ~HDRCodecInnderNdkSample();
    int32_t CreateCodec();
    int32_t Configure();
    int32_t Start();
    void WaitForEos();
    void ReleaseInFile();
    void StopInloop();
    void InputFunc();
    void FlushBuffer();
    void SwitchInputFile();
    int32_t ReConfigure();
    int32_t RepeatCall();
    int32_t DecSetCallback();
    int32_t EncSetCallback();
    int32_t RepeatCallStartFlush();
    int32_t RepeatCallStartStop();
    int32_t RepeatCallStartFlushStop();
    void Release();
    int32_t SendData(std::shared_ptr<AVCodecVideoDecoder> codec, uint32_t index, std::shared_ptr<AVSharedMemory> buffer);
    const char *INP_DIR = "/data/test/media/1920_1080_10_30Mb.h264";
    bool needEncode = false;
    bool needTransCode = false;
    uint32_t DEFAULT_WIDTH = 3840;
    uint32_t DEFAULT_HEIGHT = 2160;
    double DEFAULT_FRAME_RATE = 30.0;

    uint32_t REPEAT_START_STOP_BEFORE_EOS = 0;  // 1200 测试用例
    uint32_t REPEAT_START_FLUSH_BEFORE_EOS = 0; // 1300 测试用例
    uint32_t REPEAT_START_FLUSH_STOP_BEFORE_EOS = 0;
    uint32_t frameCount_ = 0;
    uint32_t repeat_time = 0;
    uint32_t frameCountDec = 0;
    uint32_t frameCountEnc = 0;
    int32_t DEFAULT_PROFILE = HEVC_PROFILE_MAIN_10;
    bool needOutputFile;
    std::unique_ptr<std::thread> inputLoop_;
    std::shared_ptr<AVCodecVideoDecoder> vdec_;
    std::shared_ptr<AVCodecVideoEncoder> venc_;
    uint32_t errorCount = 0;
private:
    OHNativeWindow *window;
    std::shared_ptr<HdrEncInnerCallback> encCb_;
    std::shared_ptr<HdrDecInnerCallback> decCb_;
    std::shared_ptr<InnerSignal> signal_;
};
} // namespace Media
} // namespace OHOS

#endif
