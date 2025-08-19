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

#include "audio_decoder_aac_demo.h"
#include <iostream>
#include <fstream>
#include <unistd.h>
#include <sys/stat.h>
#include <chrono>
#include <fcntl.h>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "demo_log.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avbuffer.h"
#include "native_avmemory.h"
#include "securec.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioBufferDemo;

using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t DEFAULT_AAC_TYPE = 1;
} // namespace

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}

static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
    cout << "OnOutputFormatChanged received" << endl;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    ADecBufferSignal *signal = static_cast<ADecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    ADecBufferSignal *signal = static_cast<ADecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
}

vector<string> SplitStringFully(const string& str, const string& separator)
{
    vector<string> dest;
    string substring;
    string::size_type start = 0;
    string::size_type index = str.find_first_of(separator, start);

    while (index != string::npos) {
        substring = str.substr(start, index - start);
        dest.push_back(substring);
        start = str.find_first_not_of(separator, index);
        if (start == string::npos) {
            return dest;
        }
        index = str.find_first_of(separator, start);
    }
    substring = str.substr(start);
    dest.push_back(substring);

    return dest;
}

void StringReplace(std::string& strBig, const std::string& strsrc, const std::string& strdst)
{
    std::string::size_type pos = 0;
    std::string::size_type srclen = strsrc.size();
    std::string::size_type dstlen = strdst.size();

    while ((pos = strBig.find(strsrc, pos)) != std::string::npos) {
        strBig.replace(pos, srclen, strdst);
        pos += dstlen;
    }
}

bool ADecBufferDemo::RunCase(const uint8_t *data, size_t size)
{
    std::string codecdata(reinterpret_cast<const char *>(data), size);
    inputdata = codecdata;
    inputdatasize = size;
    DEMO_CHECK_AND_RETURN_RET_LOG(CreateDec() == AVCS_ERR_OK, false, "Fatal: CreateDec fail");
    
    OH_AVFormat *format = OH_AVFormat_Create();
    auto res = InitFormat(format);
    if (res == false) {
        return false;
    }
    DEMO_CHECK_AND_RETURN_RET_LOG(Start() == AVCS_ERR_OK, false, "Fatal: Start fail");
    auto start = chrono::steady_clock::now();
    unique_lock<mutex> lock(signal_->startMutex_);
    signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    auto end = chrono::steady_clock::now();
    std::cout << "decode finished, time = " << std::chrono::duration_cast<chrono::milliseconds>(end - start).count()
              << " ms" << std::endl;
    DEMO_CHECK_AND_RETURN_RET_LOG(Stop() == AVCS_ERR_OK, false, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(Release() == AVCS_ERR_OK, false, "Fatal: Release fail");
    OH_AVFormat_Destroy(format);
    sleep(1);
    return true;
}

bool ADecBufferDemo::InitFormat(OH_AVFormat *format)
{
    // AAC_PROFILE_HE
    int32_t channelCount = CHANNEL_COUNT;
    int32_t sampleRate = SAMPLE_RATE;
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, AAC_PROFILE_HE_V2);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false, "Fatal: Configure fail");
    return true;
}

bool ADecBufferDemo::InitFile(const std::string& inputFile)
{
    if (inputFile.find("mp4") != std::string::npos || inputFile.find("m4a") != std::string::npos ||
        inputFile.find("vivid") != std::string::npos || inputFile.find("ts") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_VIVID;
    } else if (inputFile.find("aac") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_AAC;
    } else if (inputFile.find("flac") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_FLAC;
    } else if (inputFile.find("mp3") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_MP3;
    } else if (inputFile.find("vorbis") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_VORBIS;
    } else if (inputFile.find("amrnb") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_AMRNB;
    } else if (inputFile.find("amrwb") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_AMRWB;
    } else if (inputFile.find("opus") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_OPUS;
    } else if (inputFile.find("g711mu") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_G711MU;
    } else if (inputFile.find("ape") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_APE;
    } else if (inputFile.find("lbvc") != std::string::npos) {
        audioType_ = AudioBufferFormatType::TYPE_LBVC;
    } else {
        audioType_ = AudioBufferFormatType::TYPE_AAC;
    }
    return true;
}

ADecBufferDemo::ADecBufferDemo() : audioDec_(nullptr), signal_(nullptr), audioType_(AudioBufferFormatType::TYPE_AAC)
{
    signal_ = new ADecBufferSignal();
    DEMO_CHECK_AND_RETURN_LOG(signal_ != nullptr, "Fatal: No memory");
}

ADecBufferDemo::~ADecBufferDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t ADecBufferDemo::CreateDec()
{
    audioDec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false);
    DEMO_CHECK_AND_RETURN_RET_LOG(audioDec_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");
    if (audioDec_ == nullptr) {
        return AVCS_ERR_UNKNOWN;
    }
    if (signal_ == nullptr) {
        signal_ = new ADecBufferSignal();
        DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    }
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioCodec_RegisterCallback(audioDec_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");
    return AVCS_ERR_OK;
}

int32_t ADecBufferDemo::Configure(OH_AVFormat *format)
{
    return OH_AudioCodec_Configure(audioDec_, format);
}

int32_t ADecBufferDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&ADecBufferDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&ADecBufferDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AudioCodec_Start(audioDec_);
}

int32_t ADecBufferDemo::Stop()
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
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
    }
    std::cout << "start stop!\n";
    return OH_AudioCodec_Stop(audioDec_);
}

int32_t ADecBufferDemo::Flush()
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
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
        std::cout << "clear output buffer!\n";
    }
    return OH_AudioCodec_Flush(audioDec_);
}

int32_t ADecBufferDemo::Reset()
{
    return OH_AudioCodec_Reset(audioDec_);
}

int32_t ADecBufferDemo::Release()
{
    return OH_AudioCodec_Destroy(audioDec_);
}

void ADecBufferDemo::HandleInputEOS(const uint32_t index)
{
    OH_AudioCodec_PushInputBuffer(audioDec_, index);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

void ADecBufferDemo::InputFunc()
{
    size_t frameBytes = 1024;
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        size_t currentSize = inputdatasize < frameBytes ? inputdatasize : frameBytes;
        if (currentSize == 0) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            OH_AudioCodec_PushInputBuffer(audioDec_, index);
            break;
        }
        std::string currentStr = inputdata.substr(inputOffset, currentSize);
        int ret;
        strncpy_s(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), currentSize, currentStr.c_str(), currentSize);
        buffer->buffer_->memory_->SetSize(currentSize);
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        inputdatasize -= currentSize;
        inputOffset += currentSize;
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            isRunning_.store(false);
            break;
        }
    }
    signal_->startCond_.notify_all();
}

void ADecBufferDemo::OutputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }
        
        uint32_t index = signal_->outQueue_.front();
        OH_AVBuffer *data = signal_->outBufferQueue_.front();
        cout << "OutputFunc index:" << index << endl;
        if (data == nullptr) {
            cout << "OutputFunc OH_AVBuffer is nullptr" << endl;
            continue;
        }
        if (data != nullptr &&
            (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS || data->buffer_->memory_->GetSize() == 0)) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        signal_->outBufferQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioCodec_FreeOutputBuffer(audioDec_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
        if (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
    }
    signal_->startCond_.notify_all();
}