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

#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "demo_log.h"
#include "media_description.h"
#include "securec.h"
#include "ffmpeg_converter.h"
#include "avcodec_audio_encoder_inner_demo.h"

extern "C" {
#include "libavcodec/avcodec.h"
#include "libavformat/avformat.h"
#include "libavutil/imgutils.h"
#include "libavutil/samplefmt.h"
#include "libavutil/timestamp.h"
}

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::InnerAudioDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t BITS_RATE = 199000; // for aac encoding
constexpr uint32_t BITS_PER_CODED_SAMPLE = AudioSampleFormat::SAMPLE_F32P;
constexpr uint32_t DEFAULT_SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_F32P;
constexpr uint32_t DEFAULT_CHANNEL_LAYOUT_COUNT = AudioChannelLayout::STEREO;
constexpr uint32_t DEFAULT_SLEEP_TIME = 30;
} // namespace

void AEnInnerDemo::RunCase()
{
    DEMO_CHECK_AND_RETURN_LOG(CreateDec() == AVCS_ERR_OK, "Fatal: CreateDec fail");

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, BITS_PER_CODED_SAMPLE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, DEFAULT_SAMPLE_FORMAT);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, DEFAULT_CHANNEL_LAYOUT_COUNT);
    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");

    DEMO_CHECK_AND_RETURN_LOG(Start() == AVCS_ERR_OK, "Fatal: Start fail");
    sleep(DEFAULT_SLEEP_TIME);
    DEMO_CHECK_AND_RETURN_LOG(Stop() == AVCS_ERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == AVCS_ERR_OK, "Fatal: Release fail");
}

int32_t AEnInnerDemo::CreateDec()
{
    audioEn_ = AudioEncoderFactory::CreateByName((AVCodecCodecName::AUDIO_ENCODER_AAC_NAME).data());
    DEMO_CHECK_AND_RETURN_RET_LOG(audioEn_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = make_shared<AEnSignal>();

    cb_ = make_unique<AEnDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(audioEn_->SetCallback(cb_) == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                                  "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t AEnInnerDemo::Configure(const Format &format)
{
    return audioEn_->Configure(format);
}

int32_t AEnInnerDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AEnInnerDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&AEnInnerDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return audioEn_->Start();
}

int32_t AEnInnerDemo::Stop()
{
    isRunning_.store(false);

    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inQueue_.push(0);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
        inputLoop_.reset();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outQueue_.push(0);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
        outputLoop_.reset();
    }

    return audioEn_->Stop();
}

int32_t AEnInnerDemo::Flush()
{
    return audioEn_->Flush();
}

int32_t AEnInnerDemo::Reset()
{
    return audioEn_->Reset();
}

int32_t AEnInnerDemo::Release()
{
    return audioEn_->Release();
}

void AEnInnerDemo::InputFunc()
{
    const char *filePath = "/data/test/media/aac_2c_44100hz_199k.pcm";
    int frameBytes = 2 * 1024 * 4;
    std::ifstream inputFile(filePath, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cout << "open file " << filePath << " failed" << std::endl;
        return;
    }

    while (isRunning_.load()) {
        std::unique_lock<std::mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return signal_->inQueue_.size() > 0; });
        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        std::shared_ptr<AVSharedMemory> buffer = signal_->inBufferQueue_.front();
        if (buffer == nullptr) {
            isRunning_.store(false);
            std::cout << "buffer is null:" << index << "\n";
            break;
        }
        inputFile.read(reinterpret_cast<char *>(buffer->GetBase()), frameBytes);
        int readBytes = inputFile.gcount();
        AVCodecBufferInfo attr;
        AVCodecBufferFlag flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
        if (inputFile.eof() || readBytes == 0) {
            flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS;
            (void)audioEn_->QueueInputBuffer(index, attr, flag);
            signal_->inQueue_.pop();
            signal_->inBufferQueue_.pop();
            std::cout << "end buffer\n";
            break;
        }
        auto result = audioEn_->QueueInputBuffer(index, attr, flag);
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        if (result != AVCS_ERR_OK) {
            std::cout << "QueueInputBuffer error:\n";
            isRunning_ = false;
            break;
        }
    }
}

void AEnInnerDemo::OutputFunc()
{
    std::ofstream outputFile("/data/test/media/encode.aac", std::ios::binary);
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return signal_->outQueue_.size() > 0; });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        auto buffer = signal_->outBufferQueue_.front();
        if (buffer == nullptr) {
            cout << "get output buffer failed" << endl;
            isRunning_.store(false);
            break;
        }
        auto attr = signal_->sizeQueue_.front();
        outputFile.write(reinterpret_cast<char *>(buffer->GetBase()), attr.size);
        cout << "output write size = " << attr.size << endl;
        if (audioEn_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
        signal_->sizeQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
}

AEnDemoCallback::AEnDemoCallback(shared_ptr<AEnSignal> signal) : signal_(signal) {}

void AEnDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void AEnDemoCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
    cout << "OnOutputFormatChanged received" << endl;
}

void AEnDemoCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(buffer);
    signal_->inCond_.notify_all();
}

void AEnDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                              std::shared_ptr<AVSharedMemory> buffer)
{
    (void)info;
    (void)flag;
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->sizeQueue_.push(info);
    signal_->outBufferQueue_.push(buffer);
    cout << "**********out info size = " << info.size << endl;
    signal_->outCond_.notify_all();
}