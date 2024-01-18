/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include <unistd.h>
#include <string>
#include <string_view>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include "securec.h"
#include "demo_log.h"
#include "meta/audio_types.h"
#include "inner_api/native/avcodec_audio_codec.h"
#include "avcodec_audio_avbuffer_decoder_inner_demo.h"
#include "avcodec_errors.h"
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::InnerAudioDemo;

namespace {
enum class MusicClass { MP3, AAC_LC, AAC_HE, AAC_HE2, AAC_LOAS, M4A, WAV, APE, FLAC, OGG, AVS3DA, AVS3GP, TS };

constexpr std::string_view INPUT_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.dat";
constexpr std::string_view OUTPUT_PCM_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.pcm";

constexpr int32_t INPUT_FRAME_BYTES = 2 * 1024 * 4;
constexpr int32_t TIME_OUT_MS = 8;

typedef enum OH_AVCodecBufferFlags {
    AVCODEC_BUFFER_FLAGS_NONE = 0,
    /* Indicates that the Buffer is an End-of-Stream frame */
    AVCODEC_BUFFER_FLAGS_EOS = 1 << 0,
    /* Indicates that the Buffer contains keyframes */
    AVCODEC_BUFFER_FLAGS_SYNC_FRAME = 1 << 1,
    /* Indicates that the data contained in the Buffer is only part of a frame */
    AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME = 1 << 2,
    /* Indicates that the Buffer contains Codec-Specific-Data */
    AVCODEC_BUFFER_FLAGS_CODEC_DATA = 1 << 3,
} OH_AVCodecBufferFlags;
} // namespace

static int32_t GetFileSize(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file:" << filePath << std::endl;
        return -1;
    }

    std::streampos fileSize = file.tellg(); // 获取文件大小
    file.close();

    return (int32_t)fileSize;
}

AudioCodecConsumerListener::AudioCodecConsumerListener(AudioDecInnerAvBufferDemo *demo)
{
    demo_ = demo;
}

void AudioCodecConsumerListener::OnBufferAvailable()
{
    demo_->OutputFunc();
}

void AudioDecInnerAvBufferDemo::RunCase()
{
    innerBufferQueue_ = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "InnerDemo");  // 4

    audiocodec_ = AudioCodecFactory::CreateByName("OH.Media.Codec.Decoder.Audio.Mpeg");
    if (audiocodec_ == nullptr) {
        std::cout << "codec == nullptr" << std::endl;
        return;
    }
    auto meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(2);    // CHANNEL_COUNT is 2
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(44100);    // SAMPLE_RATE is 44100
    meta->Set<Tag::MEDIA_BITRATE>(60000);    // BITRATE is 60000
    audiocodec_->Configure(meta);

    audiocodec_->SetOutputBufferQueue(innerBufferQueue_->GetProducer());
    audiocodec_->Prepare();

    implConsumer_ = innerBufferQueue_->GetConsumer();
    sptr<Media::IConsumerListener> comsumerListener = new AudioCodecConsumerListener(this);
    implConsumer_->SetBufferAvailableListener(comsumerListener);
    mediaCodecProducer_ = audiocodec_->GetInputBufferQueue();

    audiocodec_->Start();
    isRunning_.store(true);

    fileSize_ = GetFileSize(INPUT_FILE_PATH.data());
    inputFile_ = std::make_unique<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(OUTPUT_PCM_FILE_PATH, std::ios::binary);
    InputFunc();
    inputFile_->close();
    outputFile_->close();
    return;
}

int32_t AudioDecInnerAvBufferDemo::GetInputBufferSize()
{
    int32_t capacity = 0;
    DEMO_CHECK_AND_RETURN_RET_LOG(audiocodec_ != nullptr, capacity, "audiocodec_ is nullptr");
    std::shared_ptr<Media::Meta> bufferConfig = std::make_shared<Media::Meta>();
    DEMO_CHECK_AND_RETURN_RET_LOG(bufferConfig != nullptr, capacity, "bufferConfig is nullptr");
    int32_t ret = audiocodec_->GetOutputFormat(bufferConfig);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCodecServiceErrCode::AVCS_ERR_OK, capacity, "GetOutputFormat fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(bufferConfig->Get<Media::Tag::AUDIO_MAX_INPUT_SIZE>(capacity),
        capacity, "get max input buffer size fail");
    return capacity;
}

void AudioDecInnerAvBufferDemo::InputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(inputFile_ != nullptr && inputFile_->is_open(), "Fatal: open file fail");
    int32_t sumReadSize = 0;
    Media::Status ret;
    int64_t size;
    int64_t pts;
    Media::AVBufferConfig avBufferConfig;
    avBufferConfig.size = GetInputBufferSize();
    while (isRunning_) {
        std::shared_ptr<AVBuffer> inputBuffer = nullptr;
        DEMO_CHECK_AND_BREAK_LOG(mediaCodecProducer_ != nullptr, "mediaCodecProducer_ is nullptr");
        ret = mediaCodecProducer_->RequestBuffer(inputBuffer, avBufferConfig, TIME_OUT_MS);
        if (ret != Media::Status::OK) {
            std::cout << "produceInputBuffer RequestBuffer fail,ret=" << (int32_t)ret << std::endl;
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputBuffer != nullptr, "buffer is nullptr");
        inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
        if (inputFile_->eof() || inputFile_->gcount() == 0 || size == 0) {
            inputBuffer->memory_->SetSize(1);
            inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            sumReadSize += 0;
            mediaCodecProducer_->PushBuffer(inputBuffer, true);
            sumReadSize += inputFile_->gcount();
            std::cout << "InputFunc, INPUT_FRAME_BYTES:" << INPUT_FRAME_BYTES << " flag:" << inputBuffer->flag_
                      << " sumReadSize:" << sumReadSize << " fileSize_:" << fileSize_
                      << " process:" << 100 * sumReadSize / fileSize_ << "%" << std::endl;    // 100
            std::cout << "end buffer\n";
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == sizeof(size), "Fatal: read size fail");
        sumReadSize += inputFile_->gcount();
        inputFile_->read(reinterpret_cast<char *>(&pts), sizeof(pts));
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == sizeof(pts), "Fatal: read pts fail");
        sumReadSize += inputFile_->gcount();
        inputFile_->read(reinterpret_cast<char *>(inputBuffer->memory_->GetAddr()), size);
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == size, "Fatal: read buffer fail");
        inputBuffer->memory_->SetSize(size);
        inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
        sumReadSize += inputFile_->gcount();
        mediaCodecProducer_->PushBuffer(inputBuffer, true);
        std::cout << "InputFunc, INPUT_FRAME_BYTES:" << INPUT_FRAME_BYTES << " flag:" << inputBuffer->flag_
                  << " sumReadSize:" << sumReadSize << " fileSize_:" << fileSize_
                  << " process:" << 100 * sumReadSize / fileSize_ << "%" << std::endl;   // 100
    }
}

void AudioDecInnerAvBufferDemo::OutputFunc()
{
    bufferConsumerAvailableCount_++;
    Media::Status ret = Media::Status::OK;
    while (isRunning_ && (bufferConsumerAvailableCount_ > 0)) {
        std::cout << "/**********ImplConsumerOutputBuffer while**********/" << std::endl;
        std::shared_ptr<AVBuffer> outputBuffer;
        ret = implConsumer_->AcquireBuffer(outputBuffer);
        if (ret != Media::Status::OK) {
            std::cout << "Consumer AcquireBuffer fail,ret=" << (int32_t)ret << std::endl;
            break;
        }
        if (outputBuffer == nullptr) {
            std::cout << "OutputFunc OH_AVBuffer is nullptr" << std::endl;
            continue;
        }
        outputFile_->write(reinterpret_cast<char *>(outputBuffer->memory_->GetAddr()),
                           outputBuffer->memory_->GetSize());
        if (outputBuffer->flag_ == AVCODEC_BUFFER_FLAGS_EOS || outputBuffer->memory_->GetSize() == 0) {
            std::cout << "out eos" << std::endl;
            isRunning_.store(false);
        }
        implConsumer_->ReleaseBuffer(outputBuffer);
        bufferConsumerAvailableCount_--;
    }
}
