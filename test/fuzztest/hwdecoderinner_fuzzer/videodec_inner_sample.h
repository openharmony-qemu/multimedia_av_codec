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

#ifndef VIDEODEC_INNER_SAMPLE_H
#define VIDEODEC_INNER_SAMPLE_H

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
#include "avcodec_video_decoder.h"
#include "nocopyable.h"
#include "buffer/avsharedmemory.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "window.h"
#include "iconsumer_surface.h"

namespace OHOS {
namespace MediaAVCodec {
class VDecInnerSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<uint32_t> outIdxQueue_;
    std::queue<AVCodecBufferInfo> infoQueue_;
    std::queue<AVCodecBufferFlag> flagQueue_;
    std::queue<std::shared_ptr<AVSharedMemory>> inBufferQueue_;
    std::queue<std::shared_ptr<AVSharedMemory>> outBufferQueue_;
};

class VDecInnerCallback : public AVCodecCallback, public NoCopyable {
public:
    explicit VDecInnerCallback(std::shared_ptr<VDecInnerSignal> signal);
    ~VDecInnerCallback() = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format& format) override;
    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
        std::shared_ptr<AVSharedMemory> buffer) override;

private:
    std::shared_ptr<VDecInnerSignal> innersignal_;
};

class VDecNdkInnerFuzzSample : public NoCopyable {
public:
    VDecNdkInnerFuzzSample() = default;
    ~VDecNdkInnerFuzzSample();
    
    int64_t GetSystemTimeUs();
    int32_t CreateByName(const std::string &name);
    int32_t Configure();
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    int32_t ReleaseOutputBuffer(uint32_t index);
    int32_t SetCallback();
    void CreateSurface();
    int32_t InputFuncFUZZ(const uint8_t *data, size_t size);
    int32_t SetOutputSurface();

    const char *outDir = "/data/test/media/VDecTest.yuv";
    const char *outDir2 = "/data/test/media/VDecTest2.yuv";
    uint32_t defaultWidth = 1920;
    uint32_t defaultHeight = 1080;
    double defaultFrameRate = 30.0;
    uint32_t maxSurfNum = 2;
    bool sfOutput = false;
    bool autoSwitchSurface = false;
    int32_t switchSurfaceFlag = 0;
    bool isP3Full = false;
    int32_t DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV12);
    int32_t defaultColorspace = 0;
    
    uint32_t errCount = 0;
    std::atomic<bool> isRunning_ { false };
    NativeWindow *nativeWindow[2] = {};
    std::shared_ptr<AVCodecVideoDecoder> vdec_;
    
private:
    std::shared_ptr<VDecInnerSignal> signal_;
    std::shared_ptr<VDecInnerCallback> cb_;
    sptr<Surface> cs[2] = {};
    sptr<Surface> ps[2] = {};
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // VIDEODEC_INNER_SAMPLE_H
