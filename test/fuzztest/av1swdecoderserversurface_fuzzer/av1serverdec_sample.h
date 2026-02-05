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

#ifndef AV1SERVERDEC_SAMPLE_H
#define AV1SERVERDEC_SAMPLE_H

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
#include "codecbase.h"
#include "surface/window.h"
namespace OHOS {
namespace MediaAVCodec {

class VDecSignal {
public:
    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<std::shared_ptr<AVBuffer>> inBufferQueue_;
};

class Av1VDecServerSample : public NoCopyable {
public:
    Av1VDecServerSample() = default;
    ~Av1VDecServerSample();

    void RunVideoServerSurfaceDecoder();
    int32_t ConfigServerDecoder();
    int32_t SetParameter();
    int32_t SetCallback();
    void GetOutputFormat();
    void Flush();
    void Stop();
    void Reset();
    void InputFunc();
    void WaitForEos();
    int32_t SetOutputSurface();
    int32_t InitDecoder();
    std::shared_ptr<VDecSignal> signal_;
    const uint8_t *fuzzData;
    size_t fuzzSize;
    int32_t sendFrameIndex = 0;
    std::vector<sptr<Surface>> cs_vector;
    std::vector<sptr<Surface>> ps_vector;
    const char *outDIR = "/data/test/media/Av1VDecTest.yuv";
protected:
    std::shared_ptr<CodecBase> codec_;
    std::atomic<bool> isRunning_ { false };
    std::unique_ptr<std::thread> inputLoop_;
    struct CallBack : public MediaCodecCallback {
        explicit CallBack(Av1VDecServerSample* tester) : tester(tester) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    private:
        Av1VDecServerSample* tester;
    };
};

class ConsumerListener : public IBufferConsumerListener {
public:
    ConsumerListener(sptr<Surface> cs) : cs_(cs){};
    ~ConsumerListener() {}
    void OnBufferAvailable() override
    {
        sptr<Surface> surface = cs_.promote();
        if (surface == nullptr) {
            return;
        }
        sptr<SurfaceBuffer> buffer;
        int32_t flushFence;
        int32_t ret = surface->AcquireBuffer(buffer, flushFence, timestamp_, damage_);
        if (ret != 0 || buffer == nullptr) {
            return;
        }
        surface->ReleaseBuffer(buffer, -1);
    }

private:
    int64_t timestamp_ = 0;
    Rect damage_ = {};
    wptr<Surface> cs_{nullptr};
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // AV1SERVERDEC_SAMPLE_H
