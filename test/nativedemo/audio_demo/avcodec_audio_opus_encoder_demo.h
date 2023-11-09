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

#ifndef AVCODEC_AUDIO_OPUS_ENCODER_DEMO_H
#define AVCODEC_AUDIO_OPUS_ENCODER_DEMO_H

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "nocopyable.h"
#include "native_avcodec_audioencoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace AudioOpusDemo {
class AEncSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::mutex startMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::condition_variable startCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
};

class AEncOpusDemo : public NoCopyable {
public:
    AEncOpusDemo();
    virtual ~AEncOpusDemo();
    void RunCase();

private:
    int32_t CreateEnc();
    int32_t Configure(OH_AVFormat *format);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    void InputFunc();
    void OutputFunc();
    void HandleEOS(const uint32_t &index);

    std::atomic<bool> isRunning_;
    std::unique_ptr<std::ifstream> inputFile_;
    std::unique_ptr<std::ofstream> outputFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    OH_AVCodec *audioEnc_;
    AEncSignal *signal_;
    struct OH_AVCodecAsyncCallback cb_;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
};
} // namespace AudioOpusDemo
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_OPUS_ENCODER_DEMO_H
