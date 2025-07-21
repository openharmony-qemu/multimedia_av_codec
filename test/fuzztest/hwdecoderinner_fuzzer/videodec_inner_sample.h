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
    int32_t CreateByMime(const std::string &mime);
    int32_t CreateByName(const std::string &name);
    int32_t Configure();
    int32_t Prepare();
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag);
    int32_t GetOutputFormat(Format &format);
    int32_t ReleaseOutputBuffer(uint32_t index);
    int32_t SetParameter(const Format &format);
    int32_t SetCallback();

    int32_t StartVideoDecoder();
    int32_t RunVideoDecoder(const std::string &codeName);
    int32_t RunVideoDecSurface(const std::string &codeName);
    int32_t PushData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index);
    int32_t SendData(uint32_t bufferSize, uint32_t index, std::shared_ptr<AVSharedMemory> buffer);
    int32_t StateEOS();
    void RepeatStartBeforeEOS();
    void SetEOS(uint32_t index);
    void WaitForEOS();
    void OpenFileFail();
    void InputFunc();
    void OutputFunc();
    void SwitchSurface();
    void ProcessOutputData(std::shared_ptr<AVSharedMemory> buffer, uint32_t index);
    void FlushBuffer();
    void StopInloop();
    void StopOutloop();
    void ReleaseInFile();
    void SetRunning();
    void GetStride();
    void CreateSurface();
    int32_t InputFuncFUZZ(const uint8_t *data, size_t size);
    int32_t SetOutputSurface();
    bool MdCompare(unsigned char *buffer, int len, const char *source[]);
    bool prepareFlag = true;
    bool p3FullFlag = false;
    bool bT709LimitFlag = false;
    bool bT709FullFlag = false;

    const char *inpDir = "/data/test/media/1920_1080_10_30Mb.h264";
    const char *outDir = "/data/test/media/VDecTest.yuv";
    const char *outDir2 = "/data/test/media/VDecTest2.yuv";
    uint32_t defaultWidth = 1920;
    uint32_t defaultHeight = 1080;
    uint32_t defaultBitrate = 10000000;
    double defaultFrameRate = 30.0;
    uint32_t maxSurfNum = 2;
    uint32_t repeatStartStopBeforeEos = 0;  // 1200 测试用例
    uint32_t repeatStartFlushBeforeEos = 0; // 1300 测试用例
    bool sfOutput = false;
    bool beforeEosInput = false;              // 0800 测试用例
    bool beforeEosInputInput = false;        // 0900 测试用例
    bool afterEosDestoryCodec = true;        // 1000 测试用例 结束不销毁codec
    bool autoSwitchSurface = false;
    int32_t switchSurfaceFlag = 0;
    bool isP3Full = false;
    int32_t DEFAULT_FORMAT = static_cast<int32_t>(VideoPixelFormat::NV12);
    int32_t defaultColorspace = 0;
    
    uint32_t errCount = 0;
    uint32_t outCount = 0;
    uint32_t frameCount = 0;
    bool sleepOnFPS = false;
    bool repeatRun = false;
    bool enableRandomEos = false;
    bool hdR2Sdr = false;
    bool outputYuvSurface = false;
    std::atomic<bool> isRunning_ { false };
    NativeWindow *nativeWindow[2] = {};
    std::shared_ptr<AVCodecVideoDecoder> vdec_;
    const char *fileSourcesha256[64] = {"27", "6D", "A2", "D4", "18", "21", "A5", "CD", "50", "F6", "DD", "CA", "46",
                                        "32", "C3", "FE", "58", "FC", "BC", "51", "FD", "70", "C7", "D4", "E7", "4D",
                                        "5C", "76", "E7", "71", "8A", "B3", "C0", "51", "84", "0A", "FA", "AF", "FA",
                                        "DC", "7B", "C5", "26", "D1", "9A", "CA", "00", "DE", "FC", "C8", "4E", "34",
                                        "C5", "9A", "43", "59", "85", "DC", "AC", "97", "A3", "FB", "23", "51"};
    
private:
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    std::shared_ptr<VDecInnerSignal> signal_;
    std::shared_ptr<VDecInnerCallback> cb_;
    sptr<Surface> cs[2] = {};
    sptr<Surface> ps[2] = {};
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // VIDEODEC_INNER_SAMPLE_H
