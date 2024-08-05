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

#include <iostream>
#include <unistd.h>
#include <chrono>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "demo_log.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "securec.h"
#include "audiodecoderdemo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioDemoAuto;
using namespace std;

namespace OHOS {
namespace MediaAVCodec {
namespace AudioDemoAuto {
    constexpr uint32_t CHANNEL_COUNT = 2;
    constexpr uint32_t CHANNEL_COUNT1 = 1;
    constexpr uint32_t SAMPLE_RATE = 44100;
    constexpr uint32_t DEFAULT_AAC_TYPE = 1;
    constexpr uint32_t AMRWB_SAMPLE_RATE = 16000;
    constexpr uint32_t AMRNB_SAMPLE_RATE = 8000;

    void OnError(OH_AVCodec* codec, int32_t errorCode, void* userData)
    {
        (void)codec;
        (void)errorCode;
        (void)userData;
    }

    void OnOutputFormatChanged(OH_AVCodec* codec, OH_AVFormat* format, void* userData)
    {
        (void)codec;
        (void)format;
        (void)userData;
        cout << "OnOutputFormatChanged received" << endl;
    }

    void OnInputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data, void* userData)
    {
        (void)codec;
        ADecSignal* signal = static_cast<ADecSignal*>(userData);
        unique_lock<mutex> lock(signal->inMutex_);
        signal->inQueue_.push(index);
        signal->inBufferQueue_.push(data);
        signal->inCond_.notify_all();
    }

    void OnOutputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data, OH_AVCodecBufferAttr* attr,
        void* userData)
    {
        (void)codec;
        ADecSignal* signal = static_cast<ADecSignal*>(userData);
        unique_lock<mutex> lock(signal->outMutex_);
        signal->outQueue_.push(index);
        signal->outBufferQueue_.push(data);
        if (attr) {
            signal->attrQueue_.push(*attr);
        } else {
            cout << "OnOutputBufferAvailable, attr is nullptr!" << endl;
        }
        signal->outCond_.notify_all();
    }
}
}
}

OH_AVCodec* ADecDemoAuto::CreateByMime(const char* mime)
{
    return OH_AudioDecoder_CreateByMime(mime);
}

OH_AVCodec* ADecDemoAuto::CreateByName(const char* name)
{
    return OH_AudioDecoder_CreateByName(name);
}

OH_AVErrCode ADecDemoAuto::Destroy(OH_AVCodec* codec)
{
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
    OH_AVErrCode ret = OH_AudioDecoder_Destroy(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode ADecDemoAuto::SetCallback(OH_AVCodec* codec)
{
    cb_ = { &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable };
    return OH_AudioDecoder_SetCallback(codec, cb_, signal_);
}

OH_AVErrCode ADecDemoAuto::Prepare(OH_AVCodec* codec)
{
    return OH_AudioDecoder_Prepare(codec);
}

OH_AVErrCode ADecDemoAuto::Start(OH_AVCodec* codec)
{
    return OH_AudioDecoder_Start(codec);
}

OH_AVErrCode ADecDemoAuto::Stop(OH_AVCodec* codec)
{
    OH_AVErrCode ret = OH_AudioDecoder_Stop(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode ADecDemoAuto::Flush(OH_AVCodec* codec)
{
    OH_AVErrCode ret = OH_AudioDecoder_Flush(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode ADecDemoAuto::Reset(OH_AVCodec* codec)
{
    return OH_AudioDecoder_Reset(codec);
}

OH_AVFormat* ADecDemoAuto::GetOutputDescription(OH_AVCodec* codec)
{
    return OH_AudioDecoder_GetOutputDescription(codec);
}

OH_AVErrCode ADecDemoAuto::PushInputData(OH_AVCodec* codec, uint32_t index, int32_t size, int32_t offset)
{
    OH_AVCodecBufferAttr info;
    info.size = size;
    info.offset = offset;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;

    return OH_AudioDecoder_PushInputData(codec, index, info);
}

OH_AVErrCode ADecDemoAuto::PushInputDataEOS(OH_AVCodec* codec, uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;

    return OH_AudioDecoder_PushInputData(codec, index, info);
}

OH_AVErrCode ADecDemoAuto::FreeOutputData(OH_AVCodec* codec, uint32_t index)
{
    return OH_AudioDecoder_FreeOutputData(codec, index);
}

OH_AVErrCode ADecDemoAuto::IsValid(OH_AVCodec* codec, bool* isValid)
{
    return OH_AudioDecoder_IsValid(codec, isValid);
}

uint32_t ADecDemoAuto::GetInputIndex()
{
    int32_t sleepTime = 0;
    uint32_t index;
    int32_t condTime = 5;
    while (signal_->inQueue_.empty() && sleepTime < condTime) {
        sleep(1);
        sleepTime++;
    }
    if (sleepTime >= condTime) {
        return 0;
    } else {
        index = signal_->inQueue_.front();
        signal_->inQueue_.pop();
    }
    return index;
}

uint32_t ADecDemoAuto::GetOutputIndex()
{
    int32_t sleepTime = 0;
    uint32_t index;
    int32_t condTime = 5;
    while (signal_->outQueue_.empty() && sleepTime < condTime) {
        sleep(1);
        sleepTime++;
    }
    if (sleepTime >= condTime) {
        return 0;
    } else {
        index = signal_->outQueue_.front();
        signal_->outQueue_.pop();
    }
    return index;
}

void ADecDemoAuto::ClearQueue()
{
    while (!signal_->inQueue_.empty()) {
        signal_->inQueue_.pop();
    }
    while (!signal_->outQueue_.empty()) {
        signal_->outQueue_.pop();
    }
    while (!signal_->inBufferQueue_.empty()) {
        signal_->inBufferQueue_.pop();
    }
    while (!signal_->outBufferQueue_.empty()) {
        signal_->outBufferQueue_.pop();
    }
    while (!signal_->attrQueue_.empty()) {
        signal_->attrQueue_.pop();
    }
}

bool ADecDemoAuto::InitFile(string inputFile)
{
    if (inputFile.find("aac") != std::string::npos) {
        audioType_ = TYPE_AAC;
    } else if (inputFile.find("flac") != std::string::npos) {
        audioType_ = TYPE_FLAC;
    } else if (inputFile.find("mp3") != std::string::npos) {
        audioType_ = TYPE_MP3;
    } else if (inputFile.find("vorbis") != std::string::npos) {
        audioType_ = TYPE_VORBIS;
    } else if (inputFile.find("amrnb") != std::string::npos) {
        audioType_ = TYPE_AMRNB;
    } else if (inputFile.find("amrwb") != std::string::npos) {
        audioType_ = TYPE_AMRWB;
    } else if (inputFile.find("opus") != std::string::npos) {
        audioType_ = TYPE_OPUS;
    } else if (inputFile.find("g711mu") != std::string::npos) {
        audioType_ = TYPE_G711MU;
    } else {
        audioType_ = TYPE_AAC;
    }
    return true;
}

bool ADecDemoAuto::InitFormat(OH_AVFormat *format)
{
    int32_t channelCount = CHANNEL_COUNT;
    int32_t sampleRate = SAMPLE_RATE;
    if (audioType_ == TYPE_AAC) {
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(),
            DEFAULT_AAC_TYPE);
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
            OH_BitsPerSample::SAMPLE_S16LE);
    } else if (audioType_ == TYPE_AMRNB || audioType_ == TYPE_G711MU || audioType_ == TYPE_OPUS) {
        channelCount = CHANNEL_COUNT1;
        sampleRate = AMRNB_SAMPLE_RATE;
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
            OH_BitsPerSample::SAMPLE_S16LE);
    } else if (audioType_ == TYPE_AMRWB) {
        channelCount = CHANNEL_COUNT1;
        sampleRate = AMRWB_SAMPLE_RATE;
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
            OH_BitsPerSample::SAMPLE_S16LE);
    }
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false,
        "Fatal: Configure fail");
    return true;
}

bool ADecDemoAuto::RunCase(const uint8_t *data, size_t size)
{
    std::string codecdata(reinterpret_cast<const char*>(data), size);
    inputdata = codecdata;
    inputdatasize = size;
    DEMO_CHECK_AND_RETURN_RET_LOG(CreateDec() == AVCS_ERR_OK, false, "Fatal: CreateDec fail");

    OH_AVFormat* format = OH_AVFormat_Create();
    auto res = InitFormat(format);
    if (res == false) {
        return false;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(Start() == AVCS_ERR_OK, false, "Fatal: Start fail");
    auto start = chrono::steady_clock::now();

    unique_lock<mutex> lock(signal_->startMutex_);
    signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });

    auto end = chrono::steady_clock::now();
    std::cout << "Encode finished, time = " << std::chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << std::endl;
    DEMO_CHECK_AND_RETURN_RET_LOG(Stop() == AVCS_ERR_OK, false, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(Release() == AVCS_ERR_OK, false, "Fatal: Release fail");
    sleep(1);
    return true;
}

ADecDemoAuto::ADecDemoAuto() : audioDec_(nullptr), signal_(nullptr), audioType_(TYPE_AAC)
{
    signal_ = new ADecSignal();
    DEMO_CHECK_AND_RETURN_LOG(signal_ != nullptr, "Fatal: No memory");
}

ADecDemoAuto::~ADecDemoAuto()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t ADecDemoAuto::CreateDec()
{
    if (audioType_ == TYPE_AAC) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_AAC_NAME).data());
    } else if (audioType_ == TYPE_FLAC) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_FLAC_NAME).data());
    } else if (audioType_ == TYPE_MP3) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_MP3_NAME).data());
    } else if (audioType_ == TYPE_VORBIS) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME).data());
    } else if (audioType_ == TYPE_AMRNB) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME).data());
    } else if (audioType_ == TYPE_AMRWB) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME).data());
    } else if (audioType_ == TYPE_OPUS) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_OPUS_NAME).data());
    } else if (audioType_ == TYPE_G711MU) {
        audioDec_ = OH_AudioDecoder_CreateByName((AVCodecCodecName::AUDIO_DECODER_G711MU_NAME).data());
    } else {
        return AVCS_ERR_INVALID_VAL;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(audioDec_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    cb_ = { &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable };
    int32_t ret = OH_AudioDecoder_SetCallback(audioDec_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t ADecDemoAuto::Configure(OH_AVFormat* format)
{
    return OH_AudioDecoder_Configure(audioDec_, format);
}

int32_t ADecDemoAuto::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&ADecDemoAuto::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&ADecDemoAuto::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AudioDecoder_Start(audioDec_);
}

int32_t ADecDemoAuto::Stop()
{
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
        inputLoop_ = nullptr;
        while (!signal_->inQueue_.empty()) {
            signal_->inQueue_.pop();
        }
        while (!signal_->inBufferQueue_.empty()) {
            signal_->inBufferQueue_.pop();
        }
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
        outputLoop_ = nullptr;
        while (!signal_->outQueue_.empty()) {
            signal_->outQueue_.pop();
        }
        while (!signal_->attrQueue_.empty()) {
            signal_->attrQueue_.pop();
        }
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
    }
    return OH_AudioDecoder_Stop(audioDec_);
}

int32_t ADecDemoAuto::Flush()
{
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
        inputLoop_ = nullptr;
        while (!signal_->inQueue_.empty()) {
            signal_->inQueue_.pop();
        }
        while (!signal_->inBufferQueue_.empty()) {
            signal_->inBufferQueue_.pop();
        }
        std::cout << "clear input buffer!\n";
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
        outputLoop_ = nullptr;
        while (!signal_->outQueue_.empty()) {
            signal_->outQueue_.pop();
        }
        while (!signal_->attrQueue_.empty()) {
            signal_->attrQueue_.pop();
        }
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
        std::cout << "clear output buffer!\n";
    }
    return OH_AudioDecoder_Flush(audioDec_);
}

int32_t ADecDemoAuto::Reset()
{
    return OH_AudioDecoder_Reset(audioDec_);
}

int32_t ADecDemoAuto::Release()
{
    return OH_AudioDecoder_Destroy(audioDec_);
}

void ADecDemoAuto::HandleInputEOS(const uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AudioDecoder_PushInputData(audioDec_, index, info);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

int32_t ADecDemoAuto::HandleNormalInput(const uint32_t& index, const int64_t pts, const size_t size)
{
    OH_AVCodecBufferAttr info;
    info.size = size;
    info.offset = 0;
    info.pts = pts;

    int32_t ret = AVCS_ERR_OK;
    if (isFirstFrame_) {
        info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
        isFirstFrame_ = false;
    } else {
        info.flags = AVCODEC_BUFFER_FLAGS_NONE;
        ret = OH_AudioDecoder_PushInputData(audioDec_, index, info);
    }
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
    frameCount_++;
    return ret;
}

void ADecDemoAuto::InputFunc()
{
    size_t frameBytes = 1152;
    if (audioType_ == TYPE_OPUS) {
        size_t opussize = 320;
        frameBytes = opussize;
    } else if (audioType_ == TYPE_G711MU) {
        size_t gmusize = 320;
        frameBytes = gmusize;
    } else if (audioType_ == TYPE_AAC) {
        size_t aacsize = 1024;
        frameBytes = aacsize;
    }
    size_t currentSize = inputdatasize < frameBytes ? inputdatasize : frameBytes;
    int64_t pts = 0;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");

        if (isFirstFrame_ == false || currentSize <= 0) {
            HandleInputEOS(index);
            std::cout << "end buffer\n";
            isRunning_.store(false);
            break;
        }

        strncpy_s((char *)OH_AVMemory_GetAddr(buffer), currentSize, inputdata.c_str(), currentSize);
        int32_t ret = HandleNormalInput(index, pts, currentSize);
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal, exit" << endl;
            isRunning_.store(false);
            break;
        }
    }
    signal_->startCond_.notify_all();
}

void ADecDemoAuto::OutputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }
        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioDecoder_FreeOutputData(audioDec_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
    }
    signal_->startCond_.notify_all();
}
