/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>
#include <fstream>
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_codec_name.h"
#include "ffmpeg_converter.h"

#define BUFFER_FLAG_EOS 1

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr int32_t FLAC_CHANNELS = 2;
constexpr int32_t FLAC_BIT_RATE = 32000;
constexpr int32_t COMPLIANCE_LEVEL = 2;
constexpr int32_t FLAC_ENCODER_SAMPLE_RATE = 32000;
constexpr int32_t ONE_FRAME_SIZE = 64000;

class FfmpegBaseCodec : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    void OnInputBufferDone(const shared_ptr<Media::AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
        std::shared_ptr<Meta> para = std::make_shared<Meta>();
        // for mutex test
        flacDec_->GetParameter(para);
    }

    void OnOutputBufferDone(const shared_ptr<Media::AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
        std::shared_ptr<Meta> para = std::make_shared<Meta>();
        // for mutex test
        flacDec_->GetParameter(para);
    }

    void OnEvent(const shared_ptr<Plugins::PluginEvent> event) override
    {
        (void)event;
    }

protected:
    shared_ptr<Media::Meta> meta_ = nullptr;
    shared_ptr<CodecPlugin> flacEnc_ = nullptr;
    shared_ptr<CodecPlugin> flacDec_ = nullptr;
};

void FfmpegBaseCodec::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void FfmpegBaseCodec::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void FfmpegBaseCodec::SetUp(void)
{
    auto encPlugin = PluginManagerV2::Instance().CreatePluginByName("OH.Media.Codec.Encoder.Audio.Flac");
    auto decPlugin = PluginManagerV2::Instance().CreatePluginByName("OH.Media.Codec.Decoder.Audio.Flac");
    if (encPlugin == nullptr) {
        cout << "not support flacEnc" << endl;
        return;
    }
    if (decPlugin == nullptr) {
        cout << "not support flacDec" << endl;
        return;
    }
    flacEnc_ = reinterpret_pointer_cast<CodecPlugin>(encPlugin);
    flacDec_ = reinterpret_pointer_cast<CodecPlugin>(decPlugin);
    meta_ = make_shared<Media::Meta>();
}

void FfmpegBaseCodec::TearDown(void)
{
    if (flacEnc_ != nullptr) {
        flacEnc_ = nullptr;
    }
    if (flacDec_ != nullptr) {
        flacDec_ = nullptr;
    }
}

HWTEST_F(FfmpegBaseCodec, FlacCheckFormat_001, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, flacEnc_->Init());
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(FLAC_ENCODER_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(FLAC_BIT_RATE);
    meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_FLAC_COMPLIANCE_LEVEL>(COMPLIANCE_LEVEL);
    meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, flacEnc_->SetParameter(meta_));
}

HWTEST_F(FfmpegBaseCodec, FlacCheckFormat_002, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, flacEnc_->Init());
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(FLAC_CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(FLAC_BIT_RATE);
    meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_FLAC_COMPLIANCE_LEVEL>(COMPLIANCE_LEVEL);
    meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, flacEnc_->SetParameter(meta_));
}

HWTEST_F(FfmpegBaseCodec, ProcessSendData_001, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, flacEnc_->Init());
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(FLAC_CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(FLAC_ENCODER_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(FLAC_BIT_RATE);
    meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_FLAC_COMPLIANCE_LEVEL>(COMPLIANCE_LEVEL);
    meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    EXPECT_EQ(Status::OK, flacEnc_->SetParameter(meta_));
    EXPECT_EQ(Status::OK, flacEnc_->Start());
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, ONE_FRAME_SIZE);
    inputBuffer->memory_ = nullptr;
    EXPECT_NE(Status::OK, flacEnc_->QueueInputBuffer(inputBuffer));
}

HWTEST_F(FfmpegBaseCodec, ProcessSendData_002, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, flacEnc_->Init());
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(FLAC_CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(FLAC_ENCODER_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(FLAC_BIT_RATE);
    meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_FLAC_COMPLIANCE_LEVEL>(COMPLIANCE_LEVEL);
    meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    EXPECT_EQ(Status::OK, flacEnc_->SetParameter(meta_));
    EXPECT_EQ(Status::OK, flacEnc_->Start());
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, ONE_FRAME_SIZE);
    inputBuffer->memory_->SetSize(ONE_FRAME_SIZE);
    inputBuffer->meta_ = nullptr;
    EXPECT_NE(Status::OK, flacEnc_->QueueInputBuffer(inputBuffer));
}

HWTEST_F(FfmpegBaseCodec, ProcessSendData_003, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, flacEnc_->Init());
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(FLAC_CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(FLAC_ENCODER_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(FLAC_BIT_RATE);
    meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_FLAC_COMPLIANCE_LEVEL>(COMPLIANCE_LEVEL);
    meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    EXPECT_EQ(Status::OK, flacEnc_->SetParameter(meta_));
    EXPECT_EQ(Status::OK, flacEnc_->Start());
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, ONE_FRAME_SIZE);
    inputBuffer->memory_->SetSize(1600); // valid size
    inputBuffer->meta_ = nullptr;
    inputBuffer->flag_ = BUFFER_FLAG_EOS;
    EXPECT_NE(Status::OK, flacEnc_->QueueInputBuffer(inputBuffer));
}

HWTEST_F(FfmpegBaseCodec, ProcessReceiveData_001, TestSize.Level1)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, ONE_FRAME_SIZE);
    outputBuffer = nullptr;
    EXPECT_NE(Status::OK, flacEnc_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(FfmpegBaseCodec, ProcessReceiveData_002, TestSize.Level1)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, ONE_FRAME_SIZE);
    EXPECT_EQ(Status::OK, flacEnc_->Reset());
    EXPECT_NE(Status::OK, flacEnc_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(FfmpegBaseCodec, ReceivePacketSucc_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(FLAC_CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(FLAC_ENCODER_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::MEDIA_BITRATE>(FLAC_BIT_RATE);
    meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_FLAC_COMPLIANCE_LEVEL>(COMPLIANCE_LEVEL);
    meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    EXPECT_EQ(Status::OK, flacEnc_->SetParameter(meta_));

    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, 640);
    outputBuffer->memory_->SetSize(640);
    EXPECT_NE(Status::OK, flacEnc_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(FfmpegBaseCodec, Stop_001, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, flacEnc_->Stop()); // set outBuffer_ nullptr
    EXPECT_EQ(Status::OK, flacEnc_->Stop());
}

HWTEST_F(FfmpegBaseCodec, Stop_002, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, flacEnc_->Reset()); // set avCodecContext_ nullptr
    EXPECT_EQ(Status::OK, flacEnc_->Stop());
}
}
}
}