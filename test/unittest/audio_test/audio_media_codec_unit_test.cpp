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

#include <gtest/gtest.h>
#include <iostream>
#include <string>
#include "plugin/plugin_manager_v2.h"
#include "media_codec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr uint32_t CHANNEL_COUNT_MONO = 1;
constexpr uint32_t CHANNEL_COUNT_STEREO = 2;
constexpr uint32_t SAMPLE_RATE_48k = 48000;
constexpr uint32_t SAMPLE_RATE_8k = 8000;
constexpr uint32_t BIT_RATE_16k = 16000;
constexpr uint32_t AUDIO_AAC_IS_ADTS = 1;
constexpr uint32_t DEFAULT_BUFFER_NUM = 1;
constexpr int32_t BUFFER_CAPACITY_SAMLL = 100;
constexpr int32_t BUFFER_CAPACITY_DEFAULT = 5120;
const std::string AAC_MIME_TYPE = "audio/mp4a-latm";
const std::string UNKNOW_MIME_TYPE = "audio/unknow";
const std::string AAC_DEC_CODEC_NAME = "OH.Media.Codec.Decoder.Audio.AAC";
}  // namespace

namespace OHOS {
namespace MediaAVCodec {

class AudioCodecCallback : public Media::AudioBaseCodecCallback {
public:
    AudioCodecCallback()
    {}
    virtual ~AudioCodecCallback()
    {}

    void OnError(Media::CodecErrorType errorType, int32_t errorCode) override;

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override;
};

void AudioCodecCallback::OnError(Media::CodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
}

void AudioCodecCallback::OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer)
{
    (void)outputBuffer;
}

class TestCodecCallback : public Media::CodecCallback {
public:
    TestCodecCallback()
    {}
    virtual ~TestCodecCallback()
    {}

    void OnError(Media::CodecErrorType errorType, int32_t errorCode) override;

    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) override;
};

void TestCodecCallback::OnError(Media::CodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    (void)errorCode;
}

void TestCodecCallback::OnOutputFormatChanged(const std::shared_ptr<Meta> &format)
{
    (void)format;
}

class AudioMediaCodecUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();

    std::shared_ptr<Plugins::CodecPlugin> CreatePlugin(const std::string &codecName);
};

void AudioMediaCodecUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioMediaCodecUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioMediaCodecUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioMediaCodecUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

std::shared_ptr<Plugins::CodecPlugin> AudioMediaCodecUnitTest::CreatePlugin(const std::string &codecName)
{
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(codecName);
    if (plugin == nullptr) {
        return nullptr;
    }
    return std::reinterpret_pointer_cast<Plugins::CodecPlugin>(plugin);
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Init_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, true));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Init_02, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, true));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Init_03, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Init_04, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_NE(0, mediaCodec->Init(UNKNOW_MIME_TYPE, true));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Init_05, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_NE(0, mediaCodec->Init(UNKNOW_MIME_TYPE, false));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Init_06, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    EXPECT_NE(0, mediaCodec->Init(UNKNOW_MIME_TYPE, false));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Init_07, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_NE(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Configure_08, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_NE(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_Prepare_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_NE(0, mediaCodec->Prepare());
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_SetDumpInfo_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    mediaCodec->SetDumpInfo(false, 0);
    mediaCodec->SetDumpInfo(true, 0);
    mediaCodec->SetDumpInfo(false, 0);
    mediaCodec->SetDumpInfo(true, 1); // 1:fd param
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_OnDumpInfo_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_AAC_IS_ADTS>(AUDIO_AAC_IS_ADTS);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48k);
    EXPECT_EQ(0, mediaCodec->Configure(meta));
    auto implBufferQueue_ =
        Media::AVBufferQueue::Create(DEFAULT_BUFFER_NUM, Media::MemoryType::SHARED_MEMORY, "UT-TEST");
    EXPECT_EQ(0, mediaCodec->SetOutputBufferQueue(implBufferQueue_->GetProducer()));
    EXPECT_EQ(0, mediaCodec->Prepare());
    mediaCodec->OnDumpInfo(0);
    mediaCodec->OnDumpInfo(1); // 1:fd param
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_ProcessInputBuffer_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_MIME_TYPE, false));
    mediaCodec->ProcessInputBuffer();
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_SetCodecCallback_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    std::shared_ptr<Media::CodecCallback> codecCallback = std::make_shared<TestCodecCallback>();
    EXPECT_EQ(0, mediaCodec->SetCodecCallback(codecCallback));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_SetCodecCallback_02, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    std::shared_ptr<Media::AudioBaseCodecCallback> mediaCallback = std::make_shared<AudioCodecCallback>();
    EXPECT_EQ(0, mediaCodec->SetCodecCallback(mediaCallback));
    mediaCodec = nullptr;
}
HWTEST_F(AudioMediaCodecUnitTest, Test_SetOutputSurface_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    sptr<Surface> surface = nullptr;
    EXPECT_EQ(0, mediaCodec->SetOutputSurface(surface));
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, Test_GetInputSurface_01, TestSize.Level1)
{
    auto mediaCodec = std::make_shared<MediaCodec>();
    EXPECT_EQ(0, mediaCodec->Init(AAC_DEC_CODEC_NAME));
    EXPECT_EQ(nullptr, mediaCodec->GetInputSurface());
    mediaCodec = nullptr;
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_01, TestSize.Level1)
{
    // AudioSampleFormat2AVSampleFormat fail branch
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S24P);
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
    EXPECT_NE(Status::OK, plugin->Start());
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_02, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    std::shared_ptr<AVAllocator> avAllocator =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_SAMLL);
    inputBuffer->memory_ = nullptr;
    EXPECT_NE(Status::OK, plugin->QueueInputBuffer(inputBuffer));
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_03, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    std::shared_ptr<AVAllocator> avAllocator =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_SAMLL);
    inputBuffer->memory_->SetSize(0);
    inputBuffer->flag_ = 0;
    EXPECT_NE(Status::OK, plugin->QueueInputBuffer(inputBuffer));
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_04, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_SAMLL);
    inputBuffer->memory_->SetSize(1); // 1:data size
    inputBuffer->flag_ = 0;
    EXPECT_NE(Status::OK, plugin->QueueInputBuffer(inputBuffer));
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_05, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_SAMLL);
    plugin->QueueOutputBuffer(outputBuffer);
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_06, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    plugin->Init();

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_AAC_IS_ADTS>(AUDIO_AAC_IS_ADTS);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48k);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Prepare());
    EXPECT_EQ(Status::OK, plugin->Start());
    EXPECT_EQ(Status::OK, plugin->Flush());
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_07, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    plugin->Init();

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_AAC_IS_ADTS>(AUDIO_AAC_IS_ADTS);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_8k);
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Prepare());
    EXPECT_EQ(Status::OK, plugin->Start());
    EXPECT_EQ(Status::OK, plugin->Flush());
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_08, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    plugin->Init();

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_AAC_IS_ADTS>(0);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48k);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(100); // 100: input buffer size
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Prepare());
    EXPECT_EQ(Status::OK, plugin->Start());
}

HWTEST_F(AudioMediaCodecUnitTest, FFmpegAACEncoderPlugin_09, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.AAC";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    plugin->Init();

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_AAC_IS_ADTS>(0);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48k);
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Start());
    EXPECT_EQ(Status::OK, plugin->Stop());
}

HWTEST_F(AudioMediaCodecUnitTest, Mp3EncoderPlugin_01, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.Mp3";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48k);
    meta->Set<Tag::MEDIA_BITRATE>(320000);  // 320000: valid param
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(100); // 100: input buffer size
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
}

HWTEST_F(AudioMediaCodecUnitTest, Mp3EncoderPlugin_02, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.Mp3";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(8000);  // sampleRate:8000 bitRate:80000 invalid
    meta->Set<Tag::MEDIA_BITRATE>(80000);     // sampleRate:8000 bitRate:80000 invalid
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(22050);  // sampleRate:22050 bitRate:192000 invalid
    meta->Set<Tag::MEDIA_BITRATE>(192000);     // sampleRate:22050 bitRate:192000 invalid
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(44100);  // sampleRate:44100 bitRate:16000 invalid
    meta->Set<Tag::MEDIA_BITRATE>(16000);      // sampleRate:44100 bitRate:16000 invalid
    EXPECT_NE(Status::OK, plugin->SetParameter(meta));
}

HWTEST_F(AudioMediaCodecUnitTest, Mp3EncoderPlugin_03, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.Mp3";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ(Status::OK, plugin->Init());
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_DEFAULT);
    inputBuffer->memory_->SetSize(-1);  // -1: invalid param
    inputBuffer->flag_ = 0;
    EXPECT_NE(Status::OK, plugin->QueueInputBuffer(inputBuffer));
}

HWTEST_F(AudioMediaCodecUnitTest, Mp3EncoderPlugin_04, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.Mp3";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ(Status::OK, plugin->Init());
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_DEFAULT);
    inputBuffer->memory_->SetSize(0);  // 0: invalid param
    inputBuffer->flag_ = 0;
    EXPECT_NE(Status::OK, plugin->QueueInputBuffer(inputBuffer));
}

HWTEST_F(AudioMediaCodecUnitTest, Mp3EncoderPlugin_05, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.Mp3";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_MONO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48k);
    meta->Set<Tag::MEDIA_BITRATE>(64000);  // 64000: valid param
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_DEFAULT);
    inputBuffer->memory_->SetSize(3000);  // 3000: invalid param
    inputBuffer->flag_ = 0;
    EXPECT_NE(Status::OK, plugin->QueueInputBuffer(inputBuffer));
}

HWTEST_F(AudioMediaCodecUnitTest, Mp3EncoderPlugin_06, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.Mp3";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48k);
    meta->Set<Tag::MEDIA_BITRATE>(64000);  // 64000: valid param
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_DEFAULT);
    inputBuffer->memory_->SetSize(5000);  // 5000: invalid param
    inputBuffer->flag_ = 0;
    EXPECT_NE(Status::OK, plugin->QueueInputBuffer(inputBuffer));
}

HWTEST_F(AudioMediaCodecUnitTest, Mp3EncoderPlugin_07, TestSize.Level1)
{
    std::string codecName = "OH.Media.Codec.Encoder.Audio.Mp3";
    auto plugin = CreatePlugin(codecName);
    EXPECT_NE(nullptr, plugin);
    EXPECT_EQ(Status::OK, plugin->Init());

    auto meta = std::make_shared<Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_MONO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(16000); // 16000: sample rate
    meta->Set<Tag::MEDIA_BITRATE>(BIT_RATE_16k);
    EXPECT_EQ(Status::OK, plugin->SetParameter(meta));
    EXPECT_EQ(Status::OK, plugin->Prepare());
    std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, BUFFER_CAPACITY_DEFAULT);
    outputBuffer->memory_->SetSize(0);  // invalid param
    outputBuffer->flag_ = 0;
    EXPECT_NE(Status::OK, plugin->QueueOutputBuffer(outputBuffer));
    EXPECT_EQ(Status::OK, plugin->Flush());
    EXPECT_EQ(Status::OK, plugin->Reset());
}
}  // namespace MediaAVCodec
}  // namespace OHOS