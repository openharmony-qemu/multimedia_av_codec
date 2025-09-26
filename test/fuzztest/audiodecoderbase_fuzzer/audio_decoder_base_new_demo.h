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
 
#ifndef AVCODEC_AUDIO_AVBUFFER_DECODER_BASE_NEW_DEMO_H
#define AVCODEC_AUDIO_AVBUFFER_DECODER_BASE_NEW_DEMO_H

#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <iomanip>
#include <cctype>
#include <atomic>
#include <queue>
#include <string>
#include <thread>
#include "native_avcodec_audiocodec.h"
#include "nocopyable.h"
#include "common/native_mfmagic.h"
#include "avcodec_audio_common.h"

namespace OHOS {
namespace MediaAVCodec {
namespace AudioBufferNewDemo {

using namespace std;
using namespace OHOS::Media;

class ADecBufferSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::mutex startMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::condition_variable startCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
};

class BaseFuzzDemo : public NoCopyable {
public:
    const uint8_t *data_;
    size_t size_;
    ADecBufferSignal *signal_;
    OH_AVFormat *format;
    OH_AVCodec *audioDec_;
    struct OH_AVCodecCallback cb_;
    OH_AVCodecCallback GetCbFunc();
    void InputFunc();
    void OutputFunc();
    OH_AVErrCode Start();
    OH_AVErrCode Stop();
    int32_t Release();
private:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
};

} // namespace AudioBufferNewDemo
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_AVBUFFER_DECODER_BASE_NEW_DEMO_H