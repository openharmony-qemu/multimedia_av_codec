/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AUDIO_ENCODER_DEMO_BASE_H
#define AUDIO_ENCODER_DEMO_BASE_H

#include <atomic>
#include <fstream>
#include <queue>
#include <string>
#include <thread>

#include "native_avcodec_audioencoder.h"
#include "nocopyable.h"
#include "avcodec_audio_common.h"

namespace OHOS {
namespace MediaAVCodec {
namespace AudioEncDemoAuto {
extern void OnError(OH_AVCodec* codec, int32_t errorCode, void* userData);
extern void OnOutputFormatChanged(OH_AVCodec* codec, OH_AVFormat* format, void* userData);
extern void OnInputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data, void* userData);
extern void OnOutputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data,
    OH_AVCodecBufferAttr* attr, void* userData);

enum AudioFormatType : int32_t {
    TYPE_OPUS = 0,
    TYPE_G711MU = 1,
    TYPE_AAC = 2,
    TYPE_FLAC = 3,
    TYPE_MAX = 10,
};

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
    std::queue<OH_AVMemory*> inBufferQueue_;
    std::queue<OH_AVMemory*> outBufferQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
};

class AEncDemoAuto : public NoCopyable {
public:
    AEncDemoAuto();
    virtual ~AEncDemoAuto();

    bool RunCase(const uint8_t *data, size_t size);

    OH_AVCodec* CreateByMime(const char* mime);

    OH_AVCodec* CreateByName(const char* mime);

    OH_AVErrCode Destroy(OH_AVCodec* codec);

    OH_AVErrCode SetCallback(OH_AVCodec* codec);

    OH_AVErrCode Prepare(OH_AVCodec* codec);

    OH_AVErrCode Start(OH_AVCodec* codec);

    OH_AVErrCode Stop(OH_AVCodec* codec);

    OH_AVErrCode Flush(OH_AVCodec* codec);

    OH_AVErrCode Reset(OH_AVCodec* codec);

    OH_AVErrCode PushInputData(OH_AVCodec* codec, uint32_t index, int32_t size, int32_t offset);

    OH_AVErrCode PushInputDataEOS(OH_AVCodec* codec, uint32_t index);

    OH_AVErrCode FreeOutputData(OH_AVCodec* codec, uint32_t index);

    OH_AVErrCode IsValid(OH_AVCodec* codec, bool* isValid);

    uint32_t GetInputIndex();

    uint32_t GetOutputIndex();

    void HandleEOS(const uint32_t& index);
    bool InitFile(std::string inputFile);
private:
    void ClearQueue();
    int32_t CreateEnd();
    int32_t Configure(OH_AVFormat* format);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    void InputFunc();
    void OutputFunc();
    void HandleInputEOS(const uint32_t index);
    void Setformat(OH_AVFormat *format);
    int32_t HandleNormalInput(const uint32_t& index, const int64_t pts, const size_t size);

    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    OH_AVCodec* audioEnc_;
    AEncSignal* signal_;
    struct OH_AVCodecAsyncCallback cb_;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
    size_t inputdatasize;
    std::string inputdata;
    AudioFormatType audioType_;
    OH_AVFormat* format_;
};
} // namespace AudioEncDemoAuto
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AUDIO_DECODER_DEMO_BASE_H
