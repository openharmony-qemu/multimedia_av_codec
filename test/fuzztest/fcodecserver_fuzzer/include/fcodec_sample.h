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

#ifndef SERVERDEC_SAMPLE_H
#define SERVERDEC_SAMPLE_H

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
#include "fcodec.h"
namespace OHOS {
namespace MediaAVCodec {

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs) : cs_(cs){};
    ~TestConsumerListener() {}
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

class VDecSignal {
public:
    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<std::shared_ptr<AVBuffer>> inBufferQueue_;
};

class FCodecServerSample : public NoCopyable {
public:
    FCodecServerSample() = default;
    ~FCodecServerSample();

    void RunFCodecDecoder();
    int32_t Configure();
    int32_t SetSurface();
    int32_t SetCallback();
    void GetOutputFormat();
    void Flush();
    void Reset();
    void InputFunc();
    void WaitForEos();
    VDecSignal *signal_;
    const uint8_t *fuzzData;
    size_t fuzzSize;
    int sendFrameIndex = 0;
    int32_t width = 1920;
    int32_t height = 1080;
    int32_t frameRate = 30;
    int32_t frameIndex = 10;
    sptr<Surface> cs = nullptr;
    sptr<Surface> ps = nullptr;

protected:
    sptr<Codec::FCodec> fcodec_;
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    struct CallBack : public MediaCodecCallback {
        explicit CallBack(FCodecServerSample *tester) : tester(tester) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;

    private:
        FCodecServerSample *tester;
    };
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_SAMPLE_H
