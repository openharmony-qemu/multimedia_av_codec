/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef VP9SERVERDEC_SAMPLE_H
#define VP9SERVERDEC_SAMPLE_H

#include <atomic>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <unordered_map>
#include "vpx_decoder.h"
#include "codecbase.h"
#include "surface/window.h"
namespace OHOS {
namespace MediaAVCodec {

constexpr uint32_t MAX_BUFFER_SIZE = 4 * 1024 * 1024;

class VDecSignal {
public:
    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::mutex outMutex_;
    std::condition_variable endCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<std::shared_ptr<AVBuffer>> inBufferQueue_;
};

class Vp9VDecServerSample : public NoCopyable {
public:
    Vp9VDecServerSample() = default;
    ~Vp9VDecServerSample();

    void RunVideoServerDecoder();
    const char *inpDir = "/data/test/media/test.vp9";
    const char *outDir = "/data/test/media/Vp9VDecTest.yuv";
    uint32_t defaultWidth = 1920;
    uint32_t defaultHeight = 1080;
    uint32_t defaultFrameRate = 30;
    uint32_t defaultRotation = 0;
    uint32_t defaultPixelFormat = 1;
    uint32_t frameCount_ = 0;
    uint32_t errCount = 0;
    int32_t ConfigServerDecoder();
    int32_t SetCallback();
    void GetOutputFormat();
    void Flush();
    void Stop();
    void Reset();
    void InputFunc();
    void WaitForEos();
    void NotifyMemoryRecycle();
    void NotifyMemoryWriteBack();
    int32_t SetOutputSurface();
    int32_t InitDecoder();
    std::shared_ptr<VDecSignal> signal_;
    bool repeatRun = true;
    bool isSurfMode = false;
    int32_t sendFrameIndex = 0;
    std::atomic<bool> isEOS_ { false };
    std::vector<sptr<Surface>> cs_vector;
    std::vector<sptr<Surface>> ps_vector;
    const uint8_t *fuzzData = nullptr;
    size_t fuzzSize = 0;
protected:
    std::shared_ptr<CodecBase> codec_;
    std::atomic<bool> isRunning_ { false };
    std::unique_ptr<std::ifstream> inFile_;
    std::unique_ptr<std::thread> inputLoop_;
    struct CallBack : public MediaCodecCallback {
        explicit CallBack(Vp9VDecServerSample* tester) : tester(tester) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    private:
        Vp9VDecServerSample* tester;
    };
private:
    int64_t GetSystemTimeUs();
    void ReleaseInFile();
    void StopInloop();
    void SetEOS(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    int32_t ReadData(uint32_t index, std::shared_ptr<AVBuffer> buffer);
    int32_t SendData(uint32_t bufferSize, uint32_t index, std::shared_ptr<AVBuffer> buffer);
    int32_t SendFuzzData(uint32_t index, std::shared_ptr<AVBuffer> buffer);
};

class ConsumerListener : public IBufferConsumerListener {
public:
    ConsumerListener(sptr<Surface> cs) : cs_(cs){};
    ~ConsumerListener() {}
    void OnBufferAvailable() override
    {
        sptr<SurfaceBuffer> buffer;
        int32_t flushFence;
        cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);
        cs_->ReleaseBuffer(buffer, -1);
    }

private:
    int64_t timestamp_ = 0;
    Rect damage_ = {};
    sptr<Surface> cs_{nullptr};
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // VP9SERVERDEC_SAMPLE_H
