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

namespace OHOS {
namespace MediaAVCodec {

class VDecSignal {
public:
    std::mutex inMutex_;
    std::condition_variable inCond_;
    std::queue<uint32_t> inIdxQueue_;
    std::queue<std::shared_ptr<AVBuffer>> inBufferQueue_;
};

class VDecServerSample : public NoCopyable {
public:
    VDecServerSample() = default;
    ~VDecServerSample();

    void RunVideoServerDecoder();
    int32_t ConfigServerDecoder();
    int32_t SetCallback();
    void GetOutputFormat();
    void Flush();
    void Reset();
    void InputFunc();
    void WaitForEos();
    
    int32_t kWidth = 1920;
    int32_t kHeight = 1080;
    int32_t kFormat = 2;
    int32_t kFormatRate = 30;
    int32_t kRotation = 0;
    int32_t kAngle = 0;
    
    VDecSignal *signal_;
    const uint8_t *fuzzData = nullptr;
    size_t fuzzSize = 0;
    int32_t sendFrameIndex;

protected:
    std::shared_ptr<CodecBase> codec_;
    std::atomic<bool> isRunning_ { false };
    std::unique_ptr<std::thread> inputLoop_;
    
    struct CallBack : public MediaCodecCallback {
        explicit CallBack(VDecServerSample* tester) : tester(tester) {}
        ~CallBack() override = default;
        void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
        void OnOutputFormatChanged(const Format &format) override;
        void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override;
        
    private:
        VDecServerSample* tester;
    };
};

} // namespace MediaAVCodec
} // namespace OHOS

#endif // VP9SERVERDEC_SAMPLE_H
