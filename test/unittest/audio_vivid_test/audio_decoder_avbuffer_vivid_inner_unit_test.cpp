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
#include "avcodec_audio_decoder.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "buffer/avbuffer_queue_define.h"
#include "inner_api/native/avcodec_audio_codec.h"
#include "media_description.h"
#include "meta/audio_types.h"
#include "meta/format.h"
#include <atomic>
#include <fstream>
#include <gtest/gtest.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;

namespace {
constexpr std::string_view INPUT_FILE_PATH = "/data/test/media/vivid_2c_44100hz_320k.dat";
constexpr std::string_view OUTPUT_PCM_FILE_PATH = "/data/test/media/vivid_2c_44100hz_320k.pcm";
constexpr int32_t TIME_OUT_MS = 8;
constexpr uint32_t DEFAULT_WIDTH = 0;
constexpr uint32_t DEFAULT_CHANNEL_COUNT = 2;
constexpr uint32_t DEFAULT_BUFFER_COUNT = 4;
constexpr uint32_t MAX_CHANNEL_COUNT = 17;
constexpr uint32_t INVALID_CHANNEL_COUNT = -1;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 48000;
constexpr uint32_t INVALID_SAMPLE_RATE = 9075000;
constexpr uint32_t DEFAULT_BITRATE = 128000;

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

#define LOG_MAX_SIZE 200
} // namespace

namespace OHOS {
namespace MediaAVCodec {
class ADecVividInnerDemoApiEleven : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    ADecVividInnerDemoApiEleven() = default;
    ~ADecVividInnerDemoApiEleven();
    void SetUp();
    void TearDown();

    int32_t CreateVividCodec();
    int32_t ProcessVivid();
    int32_t ProcessVivid24Bit();
    void InputFunc();
    void OutputFunc();

protected:
    std::shared_ptr<Media::Meta> meta;
    std::shared_ptr<AVCodecAudioCodec> audiocodec_;
    std::shared_ptr<Media::AVBufferQueue> innerBufferQueue_;
    sptr<Media::AVBufferQueueConsumer> implConsumer_;
    sptr<Media::AVBufferQueueProducer> mediaCodecProducer_;
    std::unique_ptr<std::ifstream> inputFile_;
    std::unique_ptr<std::ofstream> outputFile_;
private:
    int32_t GetInputBufferSize();
    int32_t GetFileSize(const std::string &filePath);
    int32_t fileSize_;
    std::atomic<int32_t> bufferConsumerAvailableCount_ = 0;
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
};

class AudioCodecConsumerListener : public OHOS::Media::IConsumerListener {
public:
    explicit AudioCodecConsumerListener(ADecVividInnerDemoApiEleven *demo);
    ~AudioCodecConsumerListener();
    void OnBufferAvailable() override;

private:
    ADecVividInnerDemoApiEleven *demo_;
};

AudioCodecConsumerListener::~AudioCodecConsumerListener()
{
    demo_ = nullptr;
}

AudioCodecConsumerListener::AudioCodecConsumerListener(ADecVividInnerDemoApiEleven *demo)
{
    demo_ = demo;
}

void AudioCodecConsumerListener::OnBufferAvailable()
{
    demo_->OutputFunc();
}

void ADecVividInnerDemoApiEleven::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void ADecVividInnerDemoApiEleven::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void ADecVividInnerDemoApiEleven::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void ADecVividInnerDemoApiEleven::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
    sleep(1);
}

ADecVividInnerDemoApiEleven::~ADecVividInnerDemoApiEleven()
{
    implConsumer_ = nullptr;
    mediaCodecProducer_ = nullptr;
}

int32_t ADecVividInnerDemoApiEleven::CreateVividCodec()
{
    audiocodec_ = AudioCodecFactory::CreateByName(std::string(AVCodecCodecName::AUDIO_DECODER_VIVID_NAME));
    if (audiocodec_ == nullptr) {
        std::cout << "codec == nullptr" << std::endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    innerBufferQueue_ = Media::AVBufferQueue::Create(DEFAULT_BUFFER_COUNT, Media::MemoryType::SHARED_MEMORY,
                                                     "Vivid_InnerDemo");
    meta = std::make_shared<Media::Meta>();
    return  AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t ADecVividInnerDemoApiEleven::ProcessVivid()
{
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(DEFAULT_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
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
    return  AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t ADecVividInnerDemoApiEleven::ProcessVivid24Bit()
{
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(DEFAULT_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S24LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
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
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t ADecVividInnerDemoApiEleven::GetFileSize(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file: " << filePath << std::endl;
        return -1;
    }

    std::streampos fileSize = file.tellg();
    file.close();

    return (int32_t)fileSize;
}

int32_t ADecVividInnerDemoApiEleven::GetInputBufferSize()
{
    int32_t capacity = 0;
    if (audiocodec_ == nullptr) {
        std::cout << "audiocodec_ is nullptr\n";
        return capacity;
    }
    std::shared_ptr<Media::Meta> bufferConfig = std::make_shared<Media::Meta>();
    if (bufferConfig == nullptr) {
        std::cout << "bufferConfig is nullptr\n";
        return capacity;
    }
    int32_t ret = audiocodec_->GetOutputFormat(bufferConfig);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        std::cout << "GetOutputFormat fail\n";
        return capacity;
    }
    if (!bufferConfig->Get<Media::Tag::AUDIO_MAX_INPUT_SIZE>(capacity)) {
        std::cout << "get max input buffer size fail\n";
        return capacity;
    }
    return capacity;
}

void ADecVividInnerDemoApiEleven::InputFunc()
{
    if (!(inputFile_ != nullptr && inputFile_->is_open())) {
        std::cout << "Fatal: open file fail\n";
        return;
    }
    Media::Status ret;
    int64_t size;
    int64_t pts;
    Media::AVBufferConfig avBufferConfig;
    avBufferConfig.size = GetInputBufferSize();
    while (isRunning_) {
        std::shared_ptr<AVBuffer> inputBuffer = nullptr;
        if (mediaCodecProducer_ == nullptr) {
            break;
        }
        ret = mediaCodecProducer_->RequestBuffer(inputBuffer, avBufferConfig, TIME_OUT_MS);
        if (ret != Media::Status::OK) {
            std::cout << "produceInputBuffer RequestBuffer fail,ret=" << (int32_t)ret << std::endl;
            break;
        }
        if (inputBuffer == nullptr) {
            break;
        }
        inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
        if (inputFile_->eof() || inputFile_->gcount() == 0 || size == 0) {
            inputBuffer->memory_->SetSize(1);
            inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            mediaCodecProducer_->PushBuffer(inputBuffer, true);
            std::cout << "end buffer\n";
            break;
        }
        if (inputFile_->gcount() != sizeof(size)) {
            return;
        }
        inputFile_->read(reinterpret_cast<char *>(&pts), sizeof(pts));
        if (inputFile_->gcount() != sizeof(pts)) {
            return;
        }
        inputFile_->read((char *)inputBuffer->memory_->GetAddr(), size);
        if (inputFile_->gcount() != size) {
            return;
        }
        inputBuffer->memory_->SetSize(size);
        inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
        mediaCodecProducer_->PushBuffer(inputBuffer, true);
    }
}

void ADecVividInnerDemoApiEleven::OutputFunc()
{
    bufferConsumerAvailableCount_++;
    Media::Status ret = Media::Status::OK;
    while (isRunning_ && (bufferConsumerAvailableCount_ > 0)) {
        std::shared_ptr<AVBuffer> outputBuffer;
        ret = implConsumer_->AcquireBuffer(outputBuffer);
        if (ret != Media::Status::OK) {
            isRunning_.store(false);
            std::cout << "Consumer AcquireBuffer fail,ret=" << (int32_t)ret << std::endl;
            break;
        }
        if (outputBuffer == nullptr) {
            isRunning_.store(false);
            std::cout << "OutputFunc OH_AVBuffer is nullptr" << std::endl;
            break;
        }
        outputFile_->write(reinterpret_cast<char *>(outputBuffer->memory_->GetAddr()),
            outputBuffer->memory_->GetSize());
        if (outputBuffer != nullptr &&
            (outputBuffer->flag_ == AVCODEC_BUFFER_FLAGS_EOS || outputBuffer->memory_->GetSize() == 0)) {
            std::cout << "out eos" << std::endl;
            isRunning_.store(false);
        }
        implConsumer_->ReleaseBuffer(outputBuffer);
        bufferConsumerAvailableCount_--;
    }
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_01, TestSize.Level1)
{
    // lack of correct key
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::VIDEO_WIDTH>(DEFAULT_WIDTH);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_02, TestSize.Level1)
{
    // correct key input
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MAX_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(DEFAULT_BITRATE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_03, TestSize.Level1)
{
    // correct key input with redundancy key input
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(DEFAULT_BITRATE);
    meta->Set<Tag::VIDEO_WIDTH>(DEFAULT_WIDTH);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_04, TestSize.Level1)
{
    // correct key input with wrong value type input
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MAX_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(DEFAULT_BITRATE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_05, TestSize.Level1)
{
    // correct key input with wrong value type input
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(INVALID_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(INVALID_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(DEFAULT_BITRATE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_06, TestSize.Level1)
{
    // empty format input
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_07, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_08, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S24LE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Configure_09, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Start_01, TestSize.Level1)
{
    // correct flow 1
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Start_02, TestSize.Level1)
{
    // correct flow 2
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Start());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Start_03, TestSize.Level1)
{
    // wrong flow 1
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Start());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Start_04, TestSize.Level1)
{
    // wrong flow 2
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Start());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Start_05, TestSize.Level1)
{
    // wrong flow 2
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid24Bit());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Stop_01, TestSize.Level1)
{
    // correct flow 1
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Stop_02, TestSize.Level1)
{
    // correct flow 2
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MAX_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
    meta->Set<Tag::MEDIA_BITRATE>(DEFAULT_BITRATE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Stop_03, TestSize.Level1)
{
    // correct flow 2
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid24Bit());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Flush_01, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Flush());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Flush_02, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Release());
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Flush());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Reset_01, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Reset());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Reset_02, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MAX_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Reset());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Reset_03, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Reset());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Release_01, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Stop());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Release());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Release_02, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MAX_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Configure(meta));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Release());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_Release_03, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->Release());
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_SetParameter_01, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(MAX_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DEFAULT_SAMPLE_RATE);
    int32_t ret = audiocodec_->SetParameter(meta);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE, ret);
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_SetParameter_02, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S24LE);
    int32_t ret = audiocodec_->SetParameter(meta);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE, ret);
}

HWTEST_F(ADecVividInnerDemoApiEleven, audioDecoder_Vivid_GetOutputmeta01, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, CreateVividCodec());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ProcessVivid());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, audiocodec_->GetOutputFormat(meta));
}
} // namespace MediaAVCodec
} // namespace OHOS