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

#ifndef AVCODEC_AUDIO_AVBUFFER_DECODER_DEMO_H
#define AVCODEC_AUDIO_AVBUFFER_DECODER_DEMO_H

#include <atomic>
#include <fstream>
#include <queue>
#include <string>
#include <thread>

#include "native_avcodec_audiocodec.h"
#include "nocopyable.h"
#include "common/native_mfmagic.h"

namespace OHOS {
namespace MediaAVCodec {
namespace AudioBufferDemo {
enum class AudioBufferFormatType : int32_t {
    TYPE_AAC = 0,
    TYPE_FLAC = 1,
    TYPE_MP3 = 2,
    TYPE_VORBIS = 3,
    TYPE_AMRNB = 4,
    TYPE_AMRWB = 5,
    TYPE_G711MU = 6,
    TYPE_APE = 7,
    TYPE_MAX = 8,
};

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

class ADecBufferDemo : public NoCopyable {
public:
    ADecBufferDemo();
    virtual ~ADecBufferDemo();
    void RunCase(AudioBufferFormatType audioType);
    void RunDrmCase(AudioBufferFormatType audioType);

private:
    int32_t CreateDec();
    int32_t Configure(OH_AVFormat *format);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    void InputFunc();
    void OutputFunc();
    void HandleInputEOS(const uint32_t index);
    bool InitFile(AudioBufferFormatType audioType);

    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    OH_AVCodec *audioDec_;
    ADecBufferSignal *signal_;
    struct OH_AVCodecCallback cb_;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
    std::ifstream inputFile_;
    std::ofstream pcmOutputFile_;
    AudioBufferFormatType audioType_;
};
} // namespace AudioBufferDemo
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_AVBUFFER_DECODER_DEMO_H
