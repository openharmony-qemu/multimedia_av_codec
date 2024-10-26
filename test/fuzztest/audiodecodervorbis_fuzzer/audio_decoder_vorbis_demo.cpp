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

#include "audio_decoder_demo.h"
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
constexpr uint32_t AMRWB_SAMPLE_RATE = 16000;
constexpr uint32_t AMRNB_SAMPLE_RATE = 8000;
constexpr string_view INPUT_VORBIS_FILE_PATH = "/data/test/media/vorbis_2c_44100hz_320k.dat";
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

bool ADecBufferDemo::ConfigVorbisExtraData(OH_AVFormat *format)
{
    std::ifstream inputFile_(INPUT_VORBIS_FILE_PATH, std::ios::binary);
    int64_t extradataSize;
    DEMO_CHECK_AND_RETURN_RET_LOG(inputFile_.is_open(), false, "Fatal: file is not open");
    inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
    DEMO_CHECK_AND_RETURN_RET_LOG(inputFile_.gcount() == sizeof(int64_t), false,
                                  "Fatal: read extradataSize bytes error");
    if (extradataSize < 0) {
        return false;
    }
    char buffer[extradataSize];
    inputFile_.read(buffer, extradataSize);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputFile_.gcount() == extradataSize, false, "Fatal: read extradata bytes error");
    OH_AVFormat_SetBuffer(format, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), reinterpret_cast<uint8_t *>(buffer),
                          extradataSize);
    inputFile_.close();
    return true;
}

bool ADecBufferDemo::InitFormat(OH_AVFormat *format)
{
    int32_t channelCount = CHANNEL_COUNT;
    int32_t sampleRate = SAMPLE_RATE;
    if (audioType_ != AudioBufferFormatType::TYPE_OPUS) {
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
    }
    if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), DEFAULT_AAC_TYPE);
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRNB || audioType_ == AudioBufferFormatType::TYPE_G711MU) {
        channelCount = 1;
        sampleRate = AMRNB_SAMPLE_RATE;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRWB || audioType_ == AudioBufferFormatType::TYPE_LBVC) {
        channelCount = 1;
        sampleRate = AMRWB_SAMPLE_RATE;
    } else if (audioType_ == AudioBufferFormatType::TYPE_OPUS) {
        int32_t channelCounttmp = 1;
        int32_t sampleRatetmp = 8000;
        channelCount = channelCounttmp;
        sampleRate = sampleRatetmp;
    } else if (audioType_ == AudioBufferFormatType::TYPE_MP3) {
        OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(),
                                OH_BitsPerSample::SAMPLE_S16LE);
    }
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channelCount);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    if (audioType_ == AudioBufferFormatType::TYPE_VORBIS) {
        if (ConfigVorbisExtraData(format)) {
            DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false, "Fatal: Configure fail");
        }
    } else if (audioType_ == AudioBufferFormatType::TYPE_VIVID) {
        DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false,
            "Fatal: TYPE_VIVID Configure fail");
    } else if (audioType_ == AudioBufferFormatType::TYPE_APE) {
        DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false,
            "Fatal: TYPE_APE Configure fail");
    } else {
        DEMO_CHECK_AND_RETURN_RET_LOG(Configure(format) == AVCS_ERR_OK, false,
            "Fatal: Configure fail");
    }
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
    if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AAC_NAME).data());
        std::cout << "CreateDec TYPE_AAC!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_FLAC) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_FLAC_NAME).data());
        std::cout << "CreateDec TYPE_FLAC!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_MP3) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_MP3_NAME).data());
        std::cout << "CreateDec TYPE_MP3!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_VORBIS) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME).data());
        std::cout << "CreateDec TYPE_VORBIS!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRNB) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME).data());
        std::cout << "CreateDec TYPE_AMRNB!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AMRWB) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME).data());
        std::cout << "CreateDec TYPE_AMRWB!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_VIVID) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_VIVID_NAME).data());
        std::cout << "CreateDec TYPE_VIVID!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_OPUS) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_OPUS_NAME).data());
        std::cout << "CreateDec TYPE_OPUS!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_G711MU) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_G711MU_NAME).data());
        std::cout << "CreateDec TYPE_G711MU!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_APE) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_APE_NAME).data());
        std::cout << "CreateDec TYPE_APE!" << endl;
    } else if (audioType_ == AudioBufferFormatType::TYPE_LBVC) {
        audioDec_ = OH_AudioCodec_CreateByName((AVCodecCodecName::AUDIO_DECODER_LBVC_NAME).data());
        std::cout << "CreateDec TYPE_LBVC!" << endl;
    } else {
        return AVCS_ERR_INVALID_VAL;
    }
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
    size_t frameBytes = 1152;
    if (audioType_ == AudioBufferFormatType::TYPE_OPUS) {
        size_t opussize = 960;
        frameBytes = opussize;
    } else if (audioType_ == AudioBufferFormatType::TYPE_G711MU) {
        size_t gmusize = 320;
        frameBytes = gmusize;
    } else if (audioType_ == AudioBufferFormatType::TYPE_LBVC) {
        size_t lbvcsize = 640;
        frameBytes = lbvcsize;
    } else if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
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
        int ret;
        strncpy_s(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), currentSize, inputdata.c_str(), currentSize);
        buffer->buffer_->memory_->SetSize(currentSize);
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
            isRunning_.store(false);
            break;
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
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

OH_AVErrCode ADecBufferDemo::SetCallback(OH_AVCodec* codec)
{
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    return OH_AudioCodec_RegisterCallback(codec, cb_, signal_);
}

OH_AVCodec* ADecBufferDemo::CreateByMime(const char* mime)
{
    if (mime != nullptr) {
        if (strcmp(mime, "audio/mp4a-latm") == 0) {
            audioType_ = AudioBufferFormatType::TYPE_AAC;
        } else if (strcmp(mime, "audio/flac") == 0) {
            audioType_ = AudioBufferFormatType::TYPE_FLAC;
        } else if (strcmp(mime, "audio/x-ape") == 0) {
            audioType_ = AudioBufferFormatType::TYPE_APE;
        } else if (strcmp(mime, OH_AVCODEC_MIMETYPE_AUDIO_LBVC) == 0) {
            audioType_ = AudioBufferFormatType::TYPE_LBVC;
        } else {
            audioType_ = AudioBufferFormatType::TYPE_VIVID;
        }
    }
    return OH_AudioCodec_CreateByMime(mime, false);
}

OH_AVCodec* ADecBufferDemo::CreateByName(const char* name)
{
    return OH_AudioCodec_CreateByName(name);
}

OH_AVErrCode ADecBufferDemo::Destroy(OH_AVCodec* codec)
{
    OH_AVErrCode ret = OH_AudioCodec_Destroy(codec);
    return ret;
}

OH_AVErrCode ADecBufferDemo::IsValid(OH_AVCodec* codec, bool* isValid)
{
    return OH_AudioCodec_IsValid(codec, isValid);
}

OH_AVErrCode ADecBufferDemo::Prepare(OH_AVCodec* codec)
{
    return OH_AudioCodec_Prepare(codec);
}

OH_AVErrCode ADecBufferDemo::Start(OH_AVCodec* codec)
{
    return OH_AudioCodec_Start(codec);
}

OH_AVErrCode ADecBufferDemo::Stop(OH_AVCodec* codec)
{
    OH_AVErrCode ret = OH_AudioCodec_Stop(codec);
    return ret;
}

OH_AVErrCode ADecBufferDemo::Flush(OH_AVCodec* codec)
{
    OH_AVErrCode ret = OH_AudioCodec_Flush(codec);
    return ret;
}

OH_AVErrCode ADecBufferDemo::Reset(OH_AVCodec* codec)
{
    return OH_AudioCodec_Reset(codec);
}

OH_AVFormat* ADecBufferDemo::GetOutputDescription(OH_AVCodec* codec)
{
    return OH_AudioCodec_GetOutputDescription(codec);
}

OH_AVErrCode ADecBufferDemo::FreeOutputData(OH_AVCodec* codec, uint32_t index)
{
    return OH_AudioCodec_FreeOutputBuffer(codec, index);
}

OH_AVErrCode ADecBufferDemo::PushInputData(OH_AVCodec* codec, uint32_t index)
{
    OH_AVCodecBufferAttr info;
    if (!eosFlag) {
        if (!signal_->inBufferQueue_.empty()) {
            int32_t size = 100;
            unique_lock<mutex> lock(signal_->inMutex_);
            auto buffer = signal_->inBufferQueue_.front();
            info.size = size;
            info.pts = 0;
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            OH_AVErrCode ret = OH_AVBuffer_SetBufferAttr(buffer, &info);
            std::cout <<"info.size:" << info.size <<"   ADecBufferDemo::PushInputData : = "<< (int32_t)ret<<std::endl;
            if (ret != AV_ERR_OK) {
                return ret;
            }
        }
    }
    return OH_AudioCodec_PushInputBuffer(codec, index);
}

uint32_t ADecBufferDemo::GetInputIndex()
{
    int32_t sleepTime = 0;
    int32_t condTime = 5;
    uint32_t index;
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

uint32_t ADecBufferDemo::GetOutputIndex()
{
    int32_t sleepTime = 0;
    int32_t condTime = 5;
    uint32_t index;
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

OH_AVErrCode ADecBufferDemo::PushInputDataEOS(OH_AVCodec* codec, uint32_t index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    eosFlag = true;
    if (!signal_->inBufferQueue_.empty()) {
        auto buffer = signal_->inBufferQueue_.front();
        OH_AVBuffer_SetBufferAttr(buffer, &info);
    }
    return OH_AudioCodec_PushInputBuffer(codec, index);
}

OH_AVErrCode ADecBufferDemo::Configure(OH_AVCodec* codec, OH_AVFormat* format, int32_t channel, int32_t sampleRate)
{
    if (format == nullptr) {
        std::cout<<" Configure format nullptr"<< std::endl;
        return OH_AudioCodec_Configure(codec, format);
    }
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channel);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    } else if (audioType_ == AudioBufferFormatType::TYPE_FLAC) {
    }
    std::cout<<" Configure format :"<< format << std::endl;
    if (format == nullptr) {
        std::cout<<" Configure format end is nullptr"<< std::endl;
    }
    OH_AVErrCode ret = OH_AudioCodec_Configure(codec, format);
    return ret;
}

OH_AVErrCode ADecBufferDemo::SetParameter(OH_AVCodec* codec, OH_AVFormat* format, int32_t channel, int32_t sampleRate)
{
    if (format == nullptr) {
        std::cout<<" SetParameter format nullptr"<< std::endl;
        return OH_AudioCodec_SetParameter(codec, format);
    }
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, channel);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    if (audioType_ == AudioBufferFormatType::TYPE_AAC) {
        OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);
    } else if (audioType_ == AudioBufferFormatType::TYPE_FLAC) {
    }
    std::cout<<" SetParameter format :"<< format << std::endl;
    if (format == nullptr) {
        std::cout<<" SetParameter format end is nullptr"<< std::endl;
    }
    OH_AVErrCode ret = OH_AudioCodec_SetParameter(codec, format);
    return ret;
}