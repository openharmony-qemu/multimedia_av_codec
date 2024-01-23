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
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "demo_log.h"
#include "media_description.h"
#include "securec.h"
#include "avcodec_audio_decoder_inner_demo.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::InnerAudioDemo;
using namespace std;
namespace {
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t BITS_RATE = 320000;
constexpr string_view INPUT_FILE_PATH = "/data/test/media/vorbis_2c_44100hz_320k.dat";
constexpr string_view OUTPUT_FILE_PATH = "/data/test/media/decode_ogg.pcm";
constexpr uint32_t TMP_BUFFER_SIZE = 4096;
} // namespace

void ADecInnerDemo::RunCase()
{
    inputFile_.open(INPUT_FILE_PATH, std::ios::binary);
    DEMO_CHECK_AND_RETURN_LOG(inputFile_.is_open(), "Fatal: open file failed");

    DEMO_CHECK_AND_RETURN_LOG(CreateDec() == AVCS_ERR_OK, "Fatal: CreateDec fail");

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);

    // extradata for vorbis
    char buffer[TMP_BUFFER_SIZE]; // 临时buffer，仅测试vorbis时需要
    int64_t extradataSize;
    inputFile_.read((char *)&extradataSize, sizeof(int64_t));
    DEMO_CHECK_AND_RETURN_LOG(inputFile_.gcount() == sizeof(int64_t), "Fatal: read extradataSize bytes error");

    DEMO_CHECK_AND_RETURN_LOG(extradataSize <= TMP_BUFFER_SIZE, "Fatal: buffer not large enough");
    inputFile_.read(buffer, extradataSize);
    DEMO_CHECK_AND_RETURN_LOG(inputFile_.gcount() == extradataSize, "Fatal: read extradata bytes error");
    format.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), (uint8_t *)buffer, extradataSize);

    DEMO_CHECK_AND_RETURN_LOG(Configure(format) == AVCS_ERR_OK, "Fatal: Configure fail");

    DEMO_CHECK_AND_RETURN_LOG(Start() == AVCS_ERR_OK, "Fatal: Start fail");

    unique_lock<mutex> lock(signal_->startMutex_);
    signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });

    DEMO_CHECK_AND_RETURN_LOG(Stop() == AVCS_ERR_OK, "Fatal: Stop fail");
    DEMO_CHECK_AND_RETURN_LOG(Release() == AVCS_ERR_OK, "Fatal: Release fail");
}

int32_t ADecInnerDemo::CreateDec()
{
    audioDec_ = AudioDecoderFactory::CreateByName((AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME).data());
    DEMO_CHECK_AND_RETURN_RET_LOG(audioDec_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: CreateByName fail");

    signal_ = make_shared<ADecSignal>();

    cb_ = make_unique<ADecDemoCallback>(signal_);
    DEMO_CHECK_AND_RETURN_RET_LOG(cb_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");
    DEMO_CHECK_AND_RETURN_RET_LOG(audioDec_->SetCallback(cb_) == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                                  "Fatal: SetCallback fail");

    return AVCS_ERR_OK;
}

int32_t ADecInnerDemo::Configure(const Format &format)
{
    return audioDec_->Configure(format);
}

int32_t ADecInnerDemo::Start()
{
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&ADecInnerDemo::InputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(inputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    outputLoop_ = make_unique<thread>(&ADecInnerDemo::OutputFunc, this);
    DEMO_CHECK_AND_RETURN_RET_LOG(outputLoop_ != nullptr, AVCS_ERR_UNKNOWN, "Fatal: No memory");

    return audioDec_->Start();
}

int32_t ADecInnerDemo::Stop()
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

    return audioDec_->Stop();
}

int32_t ADecInnerDemo::Flush()
{
    return audioDec_->Flush();
}

int32_t ADecInnerDemo::Reset()
{
    return audioDec_->Reset();
}

int32_t ADecInnerDemo::Release()
{
    return audioDec_->Release();
}

void ADecInnerDemo::HandleInputEOS(const uint32_t &index)
{
    AVCodecBufferInfo attr;
    AVCodecBufferFlag flag;
    attr.presentationTimeUs = 0;
    attr.size = 0;
    attr.offset = 0;
    flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS;
    (void)audioDec_->QueueInputBuffer(index, attr, flag);
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
    std::cout << "end buffer\n";
}

int32_t ADecInnerDemo::HandleNormalInput(const uint32_t &index, const int64_t &pts, const size_t &size)
{
    AVCodecBufferInfo attr;
    AVCodecBufferFlag flag;
    attr.presentationTimeUs = pts;
    attr.size = size;
    attr.offset = 0;
    flag = AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE;
    frameCount_++;
    auto result = audioDec_->QueueInputBuffer(index, attr, flag);
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();
    return result;
}

void ADecInnerDemo::InputFunc()
{
    int64_t size;
    int64_t pts;
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

        inputFile_.read((char *)&size, sizeof(size));
        if (inputFile_.eof() || inputFile_.gcount() == 0) {
            std::cout << "eof reached" << std::endl;
            HandleInputEOS(index);
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == sizeof(size), "Fatal: read size fail");

        inputFile_.read((char *)&pts, sizeof(pts));
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == sizeof(pts), "Fatal: read pts fail");
        inputFile_.read((char *)buffer->GetBase(), size);
        DEMO_CHECK_AND_BREAK_LOG(inputFile_.gcount() == size, "Fatal: read buffer fail");

        auto result = HandleNormalInput(index, pts, size);
        if (result != AVCS_ERR_OK) {
            std::cout << "QueueInputBuffer error:\n";
            isRunning_ = false;
            break;
        }
    }
}

void ADecInnerDemo::OutputFunc()
{
    std::ofstream outputFile(OUTPUT_FILE_PATH.data(), std::ios::binary);
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
        auto attr = signal_->infoQueue_.front();
        auto flag = signal_->flagQueue_.front();
        if (flag == AVCODEC_BUFFER_FLAG_EOS) {
            cout << "decode eos" << endl;
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        outputFile.write(reinterpret_cast<char *>(buffer->GetBase()), attr.size);
        if (audioDec_->ReleaseOutputBuffer(index) != AVCS_ERR_OK) {
            cout << "Fatal: ReleaseOutputBuffer fail" << endl;
            break;
        }

        signal_->outQueue_.pop();
        signal_->infoQueue_.pop();
        signal_->flagQueue_.pop();
        signal_->outBufferQueue_.pop();
    }
}

ADecDemoCallback::ADecDemoCallback(shared_ptr<ADecSignal> signal) : signal_(signal) {}

void ADecDemoCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    cout << "Error received, errorType:" << errorType << " errorCode:" << errorCode << endl;
}

void ADecDemoCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
    cout << "OnOutputFormatChanged received" << endl;
}

void ADecDemoCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(buffer);
    signal_->inCond_.notify_all();
}

void ADecDemoCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                               std::shared_ptr<AVSharedMemory> buffer)
{
    (void)info;
    (void)flag;
    cout << "OnOutputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->outMutex_);
    signal_->outQueue_.push(index);
    signal_->infoQueue_.push(info);
    signal_->flagQueue_.push(flag);
    signal_->outBufferQueue_.push(buffer);
    signal_->outCond_.notify_all();
}