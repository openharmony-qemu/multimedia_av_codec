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

#ifndef AVCODEC_AUDIO_AVBUFFER_ENCODER_RESET_DEMO_H
#define AVCODEC_AUDIO_AVBUFFER_ENCODER_RESET_DEMO_H

#include <atomic>
#include <condition_variable>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "nocopyable.h"
#include "common/native_mfmagic.h"
#include "native_avcodec_audiocodec.h"

namespace OHOS {
namespace MediaAVCodec {
namespace AudioAacEncDemo {

enum class AudioBufferFormatType : int32_t {
    TYPE_AAC = 0,
    TYPE_FLAC = 1,
    TYPE_MP3 = 2,
    TYPE_VORBIS = 3,
    TYPE_AMRNB = 4,
    TYPE_AMRWB = 5,
    TYPE_VIVID = 6,
    TYPE_OPUS = 7,
    TYPE_G711MU = 8,
    TYPE_LBVC = 10,
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
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
};

class AudioBufferAacEncDemo : public NoCopyable {
public:
    AudioBufferAacEncDemo();
    virtual ~AudioBufferAacEncDemo();
	
    bool RunCaseReset(const uint8_t *data, size_t size);
	
    OH_AVCodec* CreateByMime(const char* mime);

    OH_AVCodec* CreateByName(const char* name);

    OH_AVErrCode Destroy(OH_AVCodec* codec);

    OH_AVErrCode SetCallback(OH_AVCodec* codec);

    OH_AVErrCode Prepare(OH_AVCodec* codec);

    OH_AVErrCode Start(OH_AVCodec* codec);

    OH_AVErrCode Stop(OH_AVCodec* codec);

    OH_AVErrCode Flush(OH_AVCodec* codec);

    OH_AVErrCode Reset(OH_AVCodec* codec);

    OH_AVFormat* GetOutputDescription(OH_AVCodec* codec);

    OH_AVErrCode PushInputData(OH_AVCodec* codec, uint32_t index);

    OH_AVErrCode PushInputDataEOS(OH_AVCodec* codec, uint32_t index);

    OH_AVErrCode FreeOutputData(OH_AVCodec* codec, uint32_t index);

    OH_AVErrCode IsValid(OH_AVCodec* codec, bool* isValid);

    uint32_t GetInputIndex();

    uint32_t GetOutputIndex();

    bool InitFile(const std::string& inputFile);
private:
    void ClearQueue();
    int32_t CreateEnc();
    int32_t Configure(OH_AVFormat *format);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    void InputFunc();
    void OutputFunc();
    void Setformat(OH_AVFormat *format);
    void HandleEOS(const uint32_t &index);
    int32_t GetFileSize(const std::string &filePath);
    
    std::atomic<bool> isRunning_;
    std::ifstream inputFile_;
    std::ofstream outputFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    OH_AVCodec *audioEnc_;
    AEncSignal *signal_;
    struct OH_AVCodecCallback cb_;
    bool isFirstFrame_ = true;
    int64_t timeStamp_ = 0;
    uint32_t frameCount_ = 0;
    size_t inputdatasize = 0;
    std::string inputdata;
    AudioBufferFormatType audioType_ = AudioBufferFormatType::TYPE_AAC;
};
} // namespace AudioAacEncDemo
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_AUDIO_AVBUFFER_ENCODER_RESET_DEMO_H
