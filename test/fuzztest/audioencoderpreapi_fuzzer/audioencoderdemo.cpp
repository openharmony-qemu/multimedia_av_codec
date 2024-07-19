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
#include "audioencoderdemo.h"
#include "avcodec_audio_channel_layout.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioEncDemoAuto;
using namespace std;

namespace OHOS {
namespace MediaAVCodec {
namespace AudioEncDemoAuto {
    constexpr uint32_t CHANNEL_COUNT = 2;
    constexpr uint32_t SAMPLE_RATE = 48000;
    constexpr uint32_t SAMPLE_RATE_8000 = 8000;
    constexpr uint32_t BIT_RATE_64000 = 64000;
    constexpr int32_t CHANNEL_COUNT1 = 1;
    constexpr uint32_t DEFAULT_AAC_TYPE = 1;
    constexpr int32_t BIT_PER_CODE_COUNT = 16;
    constexpr int32_t COMPLEXITY_COUNT = 10;
    constexpr int32_t CHANNEL_1 = 1;
    constexpr int32_t CHANNEL_2 = 2;
    constexpr int32_t CHANNEL_3 = 3;
    constexpr int32_t CHANNEL_4 = 4;
    constexpr int32_t CHANNEL_5 = 5;
    constexpr int32_t CHANNEL_6 = 6;
    constexpr int32_t CHANNEL_7 = 7;
    constexpr int32_t CHANNEL_8 = 8;

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
        AEncSignal* signal = static_cast<AEncSignal*>(userData);
        unique_lock<mutex> lock(signal->inMutex_);
        signal->inQueue_.push(index);
        signal->inBufferQueue_.push(data);
        signal->inCond_.notify_all();
    }

    void OnOutputBufferAvailable(OH_AVCodec* codec, uint32_t index, OH_AVMemory* data, OH_AVCodecBufferAttr* attr,
        void* userData)
    {
        (void)codec;
        AEncSignal* signal = static_cast<AEncSignal*>(userData);
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

static uint64_t GetChannelLayout(int32_t channel)
{
    switch (channel) {
        case CHANNEL_1:
            return MONO;
        case CHANNEL_2:
            return STEREO;
        case CHANNEL_3:
            return CH_2POINT1;
        case CHANNEL_4:
            return CH_3POINT1;
        case CHANNEL_5:
            return CH_4POINT1;
        case CHANNEL_6:
            return CH_5POINT1;
        case CHANNEL_7:
            return CH_6POINT1;
        case CHANNEL_8:
            return CH_7POINT1;
        default:
            return UNKNOWN_CHANNEL_LAYOUT;
    }
}

void AEncDemoAuto::HandleEOS(const uint32_t& index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AudioEncoder_PushInputData(audioEnc_, index, info);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

OH_AVCodec* AEncDemoAuto::CreateByMime(const char* mime)
{
    return OH_AudioEncoder_CreateByMime(mime);
}

OH_AVCodec* AEncDemoAuto::CreateByName(const char* name)
{
    return OH_AudioEncoder_CreateByName(name);
}

OH_AVErrCode AEncDemoAuto::Destroy(OH_AVCodec* codec)
{
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
    OH_AVErrCode ret = OH_AudioEncoder_Destroy(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode AEncDemoAuto::SetCallback(OH_AVCodec* codec)
{
    cb_ = { &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable };
    return OH_AudioEncoder_SetCallback(codec, cb_, signal_);
}

OH_AVErrCode AEncDemoAuto::Prepare(OH_AVCodec* codec)
{
    return OH_AudioEncoder_Prepare(codec);
}

OH_AVErrCode AEncDemoAuto::Start(OH_AVCodec* codec)
{
    return OH_AudioEncoder_Start(codec);
}

OH_AVErrCode AEncDemoAuto::Stop(OH_AVCodec* codec)
{
    OH_AVErrCode ret = OH_AudioEncoder_Stop(codec);
    ClearQueue();
    return ret;
}

OH_AVErrCode AEncDemoAuto::Flush(OH_AVCodec* codec)
{
    OH_AVErrCode ret = OH_AudioEncoder_Flush(codec);
    std::cout << "Flush ret:"<< ret <<endl;
    ClearQueue();
    return ret;
}

OH_AVErrCode AEncDemoAuto::Reset(OH_AVCodec* codec)
{
    return OH_AudioEncoder_Reset(codec);
}

OH_AVErrCode AEncDemoAuto::PushInputData(OH_AVCodec* codec, uint32_t index, int32_t size, int32_t offset)
{
    OH_AVCodecBufferAttr info;
    info.size = size;
    info.offset = offset;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_NONE;
    return OH_AudioEncoder_PushInputData(codec, index, info);
}

OH_AVErrCode AEncDemoAuto::PushInputDataEOS(OH_AVCodec* codec, uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;

    return OH_AudioEncoder_PushInputData(codec, index, info);
}

OH_AVErrCode AEncDemoAuto::FreeOutputData(OH_AVCodec* codec, uint32_t index)
{
    return OH_AudioEncoder_FreeOutputData(codec, index);
}

OH_AVErrCode AEncDemoAuto::IsValid(OH_AVCodec* codec, bool* isValid)
{
    return OH_AudioEncoder_IsValid(codec, isValid);
}

uint32_t AEncDemoAuto::GetInputIndex()
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

uint32_t AEncDemoAuto::GetOutputIndex()
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

void AEncDemoAuto::ClearQueue()
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

bool AEncDemoAuto::InitFile(string inputFile)
{
    if (inputFile.find("opus") != std::string::npos) {
        audioType_ = TYPE_OPUS;
    } else if (inputFile.find("g711") != std::string::npos) {
        audioType_ = TYPE_G711MU;
    } else if (inputFile.find("flac") != std::string::npos) {
        audioType_ = TYPE_FLAC;
    } else {
        audioType_ = TYPE_AAC;
    }
    return true;
}

bool AEncDemoAuto::RunCase(const uint8_t *data, size_t size)
{
    std::string codecdata(reinterpret_cast<const char*>(data), size);
    inputdata = codecdata;
    inputdatasize = size;
    DEMO_CHECK_AND_RETURN_RET_LOG(CreateEnd() == AVCS_ERR_OK, false, "Fatal: CreateEnd fail");
    int32_t channelCount = CHANNEL_COUNT;
    int32_t sampleRate = SAMPLE_RATE;
    OH_AVFormat* format = OH_AVFormat_Create();
    if (audioType_ == TYPE_OPUS) {
        channelCount = CHANNEL_COUNT1;
        sampleRate = SAMPLE_RATE_8000;
        OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BIT_RATE_64000);
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BIT_PER_CODE_COUNT);
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLEXITY_COUNT);
    } else if (audioType_ == TYPE_G711MU) {
        channelCount = CHANNEL_COUNT1;
        sampleRate = SAMPLE_RATE_8000;
        OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BIT_RATE_64000);
    } else if (audioType_ == TYPE_FLAC) {
        uint64_t channelLayout = GetChannelLayout(CHANNEL_COUNT);
        OH_AVFormat_SetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, channelLayout);
        OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BIT_RATE_64000);
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, OH_BitsPerSample::SAMPLE_S16LE);
    } else if (audioType_ == TYPE_AAC) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    }
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
        AudioSampleFormat::SAMPLE_S16LE);

    DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false, "Fatal: Configure fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(Start() == AVCS_ERR_OK, false, "Fatal: Start fail");
    sleep(1);
    auto start = chrono::steady_clock::now();

    unique_lock<mutex> lock(signal_->startMutex_);
    signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });

    auto end = chrono::steady_clock::now();
    std::cout << "Encode finished, time = " << std::chrono::duration_cast<chrono::milliseconds>(end - start).count()
        << " ms" << std::endl;
    DEMO_CHECK_AND_RETURN_RET_LOG(Stop() == AVCS_ERR_OK, false, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(Release() == AVCS_ERR_OK, false, "Fatal: Release fail");
    OH_AVFormat_Destroy(format);
    sleep(1);
    return true;
}


AEncDemoAuto::AEncDemoAuto()
{
    audioEnc_ = nullptr;
    signal_ = new AEncSignal();
    DEMO_CHECK_AND_RETURN_LOG(signal_ != nullptr, "Fatal: No memory");
    format_ = nullptr;
    audioType_ = TYPE_OPUS;
}


AEncDemoAuto::~AEncDemoAuto()
{
    isRunning_.store(false);
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t AEncDemoAuto::CreateEnd()
{
    if (audioType_ == TYPE_AAC) {
        audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).data());
    } else if (audioType_ == TYPE_FLAC) {
        audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME).data());
    } else if (audioType_ == TYPE_OPUS) {
        audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME).data());
    } else if (audioType_ == TYPE_G711MU) {
        audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME).data());
    } else {
        return AVCS_ERR_INVALID_VAL;
    }

    if (signal_ == nullptr) {
        signal_ = new AEncSignal();
    }
    if (signal_ == nullptr) {
        return AVCS_ERR_UNKNOWN;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(audioEnc_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    cb_ = { &OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable };
    int32_t ret = OH_AudioEncoder_SetCallback(audioEnc_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t AEncDemoAuto::Configure(OH_AVFormat* format)
{
    return OH_AudioEncoder_Configure(audioEnc_, format);
}

int32_t AEncDemoAuto::Start()
{
    isRunning_.store(false);
    signal_->inCond_.notify_all();
    signal_->outCond_.notify_all();
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        inputLoop_->join();
        inputLoop_.reset();
        inputLoop_ = nullptr;
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        outputLoop_->join();
        outputLoop_.reset();
        outputLoop_ = nullptr;
    }
    sleep(1);
    {
        unique_lock<mutex> lock(signal_->inMutex_);
        while (!signal_->inQueue_.empty()) {
            signal_->inQueue_.pop();
        }
        while (!signal_->inBufferQueue_.empty()) {
            signal_->inBufferQueue_.pop();
        }
    }
    {
        unique_lock<mutex> lock(signal_->outMutex_);
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
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AEncDemoAuto::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AEncDemoAuto::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    if (audioEnc_ == nullptr) {
        std::cout << "audioEnc_ is nullptr " << std::endl;
    }
    int32_t ret = OH_AudioEncoder_Start(audioEnc_);
    return ret;
}

int32_t AEncDemoAuto::Stop()
{
    return OH_AudioEncoder_Stop(audioEnc_);
}

int32_t AEncDemoAuto::Flush()
{
    OH_AVErrCode ret = OH_AudioEncoder_Flush(audioEnc_);
    return ret;
}

int32_t AEncDemoAuto::Release()
{
    isRunning_.store(false);
    signal_->startCond_.notify_all();
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
        inputLoop_.reset();
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
        outputLoop_.reset();
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
    if (signal_) {
        ClearQueue();
        delete signal_;
        signal_ = nullptr;
        std::cout << "signal_Release" <<endl;
    }
    int32_t ret = OH_AudioEncoder_Destroy(audioEnc_);
    audioEnc_ = nullptr;
    return ret;
}

int32_t AEncDemoAuto::Reset()
{
    return OH_AudioEncoder_Reset(audioEnc_);
}

void AEncDemoAuto::HandleInputEOS(const uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVErrCode ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
    std::cout << "HandleInputEOS->ret:"<< ret <<endl;
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

int32_t AEncDemoAuto::HandleNormalInput(const uint32_t& index, const int64_t pts, const size_t size)
{
    OH_AVCodecBufferAttr info;
    info.size = size;
    info.offset = 0;
    info.pts = pts;

    int32_t ret = AVCS_ERR_OK;
    if (isFirstFrame_) {
        info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
        ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
        isFirstFrame_ = false;
    } else {
        info.flags = AVCODEC_BUFFER_FLAGS_NONE;
        ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
    }
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
    return ret;
}


void AEncDemoAuto::InputFunc()
{
    int64_t pts = 0;
    size_t frameBytes = 1152;
    if (audioType_ == TYPE_OPUS) {
        size_t opussize = 960;
        frameBytes = opussize;
    } else if (audioType_ == TYPE_G711MU) {
        size_t gmusize = 320;
        frameBytes = gmusize;
    } else if (audioType_ == TYPE_AAC) {
        size_t aacsize = 1024;
        frameBytes = aacsize;
    }
    size_t currentSize = inputdatasize < frameBytes ? inputdatasize : frameBytes;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        strncpy_s((char *)OH_AVMemory_GetAddr(buffer), currentSize, inputdata.c_str(), currentSize);
        if (isFirstFrame_ == false || currentSize <= 0) {
            HandleInputEOS(index);
            std::cout << "end buffer\n";
            isRunning_.store(false);
            break;
        }
        int32_t ret = HandleNormalInput(index, pts, frameBytes);
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal, exit:" <<ret << endl;
            isRunning_.store(false);
            break;
        }
    }
    signal_->startCond_.notify_all();
}

void AEncDemoAuto::OutputFunc()
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

        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioEncoder_FreeOutputData(audioEnc_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "encode eos" << endl;
            break;
        }
    }
    isRunning_.store(false);
    signal_->startCond_.notify_all();
}
