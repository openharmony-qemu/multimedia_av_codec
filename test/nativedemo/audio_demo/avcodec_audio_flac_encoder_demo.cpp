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

#include <iostream>
#include <unistd.h>
#include "securec.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "native_avformat.h"
#include "demo_log.h"
#include "avcodec_codec_name.h"
#include "ffmpeg_converter.h"
#include "avcodec_audio_flac_encoder_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::AudioFlacDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t BITS_RATE = 261000;
constexpr uint32_t BITS_PER_CODED_SAMPLE = AudioSampleFormat::SAMPLE_S16LE;
constexpr uint32_t FRAME_DURATION_US = 33000;
constexpr uint32_t CHANNEL_LAYOUT = AudioChannelLayout::STEREO;
constexpr int32_t SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t COMPLIANCE_LEVEL = 0;
constexpr int32_t DEFAULT_FRAME_BYTES = 1152;
static std::map<uint32_t, uint32_t> FRAME_BYTES_MAP = {{8000, 576}, {16000, 1152}, {22050, 2304}, {24000, 2304},
                                                       {32000, 2304}, {44100, 4608}, {48000, 4608}, {88200, 8192},
                                                       {96000, 8192}};

constexpr string_view INPUT_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view OUTPUT_FILE_PATH = "/data/test/media/flac_encoder_test.flac";
} // namespace

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
    cout << "Error received, errorCode:" << errorCode << endl;
}

static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
    cout << "OnOutputFormatChanged received" << endl;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                    void *userData)
{
    (void)codec;
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    if (attr) {
        signal->attrQueue_.push(*attr);
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
    signal->outCond_.notify_all();
}

void AEncFlacDemo::RunCase()
{
    DEMO_CHECK_AND_RETURN_LOG(CreateEnc() == AVCS_ERR_OK, "Fatal: CreateEnc fail");

    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");

    DEMO_CHECK_AND_RETURN_LOG(Start() == AVCS_ERR_OK, "Fatal: Start fail");

    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }

    DEMO_CHECK_AND_RETURN_LOG(Stop() == AVCS_ERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == AVCS_ERR_OK, "Fatal: Release fail");
}

AEncFlacDemo::AEncFlacDemo() : isRunning_(false), audioEnc_(nullptr), signal_(nullptr), frameCount_(0)
{
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_FILE_PATH, std::ios::binary);
}

AEncFlacDemo::~AEncFlacDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}

int32_t AEncFlacDemo::CreateEnc()
{
    audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME).data());
    DEMO_CHECK_AND_RETURN_RET_LOG(audioEnc_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = new AEncSignal();
    DEMO_CHECK_AND_RETURN_RET_LOG(signal_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioEncoder_SetCallback(audioEnc_, cb_, signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t AEncFlacDemo::Configure(OH_AVFormat *format)
{
    return OH_AudioEncoder_Configure(audioEnc_, format);
}

int32_t AEncFlacDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AEncFlacDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AEncFlacDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return OH_AudioEncoder_Start(audioEnc_);
}

int32_t AEncFlacDemo::Stop()
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

    return OH_AudioEncoder_Stop(audioEnc_);
}

int32_t AEncFlacDemo::Flush()
{
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
    return OH_AudioEncoder_Flush(audioEnc_);
}

int32_t AEncFlacDemo::Reset()
{
    return OH_AudioEncoder_Reset(audioEnc_);
}

int32_t AEncFlacDemo::Release()
{
    return OH_AudioEncoder_Destroy(audioEnc_);
}

void AEncFlacDemo::HandleEOS(const uint32_t &index)
{
    OH_AVCodecBufferAttr info;
    info.size = 0;
    info.offset = 0;
    info.pts = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AudioEncoder_PushInputData(audioEnc_, index, info);
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
}

static int32_t GetFrameBytes()
{
    auto bitsPerSamples = (SAMPLE_FORMAT == AudioSampleFormat::SAMPLE_S16LE) ? 2 : 4;
    auto iter = FRAME_BYTES_MAP.find(SAMPLE_RATE);
    uint32_t frameSize = DEFAULT_FRAME_BYTES;
    if (iter != FRAME_BYTES_MAP.end()) {
        frameSize = iter->second;
    }
    int32_t frameBytes = CHANNEL_COUNT * bitsPerSamples * frameSize;
    return frameBytes;
}

void AEncFlacDemo::InputFunc()
{
    auto frameBytes = GetFrameBytes();
    DEMO_CHECK_AND_RETURN_LOG(inputFile_ != nullptr && inputFile_->is_open(), "Fatal: open file fail");
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        if (!inputFile_->eof()) {
            inputFile_->read((char *)OH_AVMemory_GetAddr(buffer), frameBytes);
        } else {
            HandleEOS(index);
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        OH_AVCodecBufferAttr info;
        info.size = frameBytes;
        info.offset = 0;

        int32_t ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
            isFirstFrame_ = false;
        } else {
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
        }

        timeStamp_ += FRAME_DURATION_US;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            isRunning_ = false;
            break;
        }
    }
    inputFile_->close();
}

void AEncFlacDemo::OutputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(outputFile_ != nullptr && outputFile_->is_open(), "Fatal: open output file fail");
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();

        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *data = signal_->outBufferQueue_.front();
        if (data != nullptr) {
            outputFile_->write(reinterpret_cast<char *>(OH_AVMemory_GetAddr(data)), attr.size);
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS || attr.size == 0) {
            cout << "encode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }

        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioEncoder_FreeOutputData(audioEnc_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
    }
    outputFile_->close();
}
