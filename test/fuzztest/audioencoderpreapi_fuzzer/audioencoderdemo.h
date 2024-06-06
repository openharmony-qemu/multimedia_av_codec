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

/**
  * @test
  * @Status {Create, Configure, Start, Running, EOS, Flush, Stop, Reset, Release}
  * @StatusErrCode AV_ERR_INVALID_STATE
  * @Allow Create -> Configure
  * @Allow Configure -> Start
  * @Allow Start -> {Running, Flush, EOS, Stop}
  * @Allow Flush -> {Start, Stop}
  * @Allow Running -> {Running, EOS, Flush, Stop}
  * @Allow EOS -> {Flush, Stop}
  * @Allow Stop -> Start
  * @Allow Reset -> Configure
  * @Allow {Create, Configure, Start, Running, EOS, Flush, Stop, Reset} -> Reset
  * @Allow {Create, Configure, Start, Running, EOS, Flush, Stop, Reset} -> Release
**/
class AEncDemoAuto : public NoCopyable {
public:
    AEncDemoAuto();
    virtual ~AEncDemoAuto();

    bool RunCase(const uint8_t *data, size_t size);

    OH_AVCodec* CreateByMime(const char* mime);

    OH_AVCodec* CreateByName(const char* mime);

    OH_AVErrCode Destroy(OH_AVCodec* codec);

    OH_AVErrCode SetCallback(OH_AVCodec* codec);

    /**
      * @interfaceTest
      * @Status Configure
      * @after SetCallback
      * @param codec; depend: CreateByMime.return; code: AV_ERR_INVALID_VAL;
      * @param format; default: OH_AVFormat_Create(); code: AV_ERR_INVALID_VAL;
      * @if mime is OH_AVCODEC_MIMETYPE_AUDIO_OPUS
      * @param channel; scope: [1, 2]; default: 2; code: AV_ERR_INVALID_VAL;
      * @param sampleRate; scope: {8000, 12000, 16000, 24000, 48000}; default: 48000;
      *                    code: AV_ERR_INVALID_VAL;
      * @param bitRate; scope: [6000, 510000]; default: 15000; code: AV_ERR_INVALID_VAL;
      * @param sampleFormat; scope: [AudioSampleFormat::SAMPLE_S16LE]; default: AudioSampleFormat::SAMPLE_S16LE; code: AV_ERR_INVALID_VAL;
      * @param sampleBit; scope: [16]; default: 16; code: AV_ERR_INVALID_VAL;
      * @param complexity; scope: [1, 10]; default: 10; code: AV_ERR_INVALID_VAL;
      * @if mime is OH_AVCODEC_MIMETYPE_AUDIO_G711MU
      * @param channel; scope: {1}; default: 1; code: AV_ERR_INVALID_VAL;
      * @param sampleRate; scope: {8000}; default: 8000; code: AV_ERR_INVALID_VAL;
      * @param sampleFormat; scope: [AudioSampleFormat::SAMPLE_S16LE]; default: AudioSampleFormat::SAMPLE_S16LE; code: AV_ERR_INVALID_VAL;
      * @return AV_ERR_OK
    **/
    OH_AVErrCode Configure(OH_AVCodec* codec, OH_AVFormat* format, int32_t channel, int32_t sampleRate, int64_t bitRate, int32_t sampleFormat, int32_t sampleBit, int32_t complexity);

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
