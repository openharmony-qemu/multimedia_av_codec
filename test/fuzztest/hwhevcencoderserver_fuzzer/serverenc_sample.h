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
#include "hcodec.h"
namespace OHOS {
namespace MediaAVCodec {

class VEncSignal {
public:
    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<std::shared_ptr<AVBuffer>> inBufferQueue_;
};

class VEncServerSample : public NoCopyable {
public:
    VEncServerSample() = default;
    ~VEncServerSample();

    void RunVideoServerDecoder();
    int32_t ConfigServerDecoder();
    int32_t SetCallback();
    void GetOutputFormat();
    void Flush();
    void Reset();
    int32_t SetParameter();
    void InputFunc();
    void WaitForEos();
    VEncSignal *signal_;
    const uint8_t *fuzzData;
    size_t fuzzSize;
    int sendFrameIndex = 0;
    int32_t defaultWidth = 1920;
    int32_t defaultHeight = 1080;
    int32_t defaultFrameRate = 30;
    int32_t defaultPixelFormat = 1;
    int32_t frameIndex = 10;
protected:
    std::shared_ptr<CodecBase> codec_;
    std::atomic<bool> isRunning_ { false };
    std::unique_ptr<std::thread> inputLoop_;
    struct CallBack : public MediaCodecCallback {
        explicit CallBack(VEncServerSample* tester) : tester(tester) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
    private:
        VEncServerSample* tester;
    };
};
} // namespace Media
} // namespace OHOS

#endif // SERVERDEC_SAMPLE_H
