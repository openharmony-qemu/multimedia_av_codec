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

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr uint32_t CHANNELS = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t UNSUPPORTED_CHANNELS_MIN = 0;
constexpr uint32_t UNSUPPORTED_CHANNELS_MAX = 256;
constexpr uint32_t UNSUPPORTED_MAX_INPUT_SIZE = 80 * 1024 * 1024 + 1;
constexpr uint32_t UNSUPPORTED_SAMPLE_RATE = 0;
constexpr int32_t MAX_INPUT_SIZE = 4096;
constexpr int32_t MIN_INPUT_SIZE = 16;
constexpr AudioSampleFormat SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16LE;
constexpr AudioSampleFormat RAW_SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16BE;

class RawDecoderUnitTest : public testing::Test, public DataCallback {
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
        decoder_->GetParameter(para);
    }

    void OnOutputBufferDone(const shared_ptr<Media::AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
        std::shared_ptr<Meta> para = std::make_shared<Meta>();
        // for mutex test
        decoder_->GetParameter(para);
    }

    void OnEvent(const shared_ptr<Plugins::PluginEvent> event) override
    {
        (void)event;
    }

protected:
    shared_ptr<Media::Meta> meta_ = nullptr;
    shared_ptr<CodecPlugin> decoder_ = nullptr;
};

void RawDecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void RawDecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void RawDecoderUnitTest::SetUp(void)
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName("OH.Media.Codec.Decoder.Audio.Raw");
    if (plugin == nullptr) {
        cout << "not support raw" << endl;
        return;
    }
    decoder_ = reinterpret_pointer_cast<CodecPlugin>(plugin);
    decoder_->SetDataCallback(this);
    meta_ = make_shared<Media::Meta>();
}

void RawDecoderUnitTest::TearDown(void)
{
    if (decoder_ != nullptr) {
        decoder_ = nullptr;
    }
}

HWTEST_F(RawDecoderUnitTest, InputBuffer_Nullptr_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    decoder_->SetParameter(meta_);
    EXPECT_EQ(Status::OK, decoder_->Init());
    EXPECT_EQ(Status::OK, decoder_->Prepare());
    EXPECT_EQ(Status::OK, decoder_->Start());
    EXPECT_EQ(Status::OK, decoder_->Stop()); // set inputBuffer_ nullptr
    EXPECT_EQ(Status::OK, decoder_->Stop());
    EXPECT_EQ(Status::OK, decoder_->Flush());
    EXPECT_EQ(Status::OK, decoder_->Reset());
    EXPECT_EQ(Status::OK, decoder_->Release());
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE); // no AUDIO_CHANNEL_COUNT
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS); // no AUDIO_SAMPLE_RATE
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE); // no AUDIO_SAMPLE_FORMAT
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_004, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT); // no AUDIO_RAW_SAMPLE_FORMAT
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(UNSUPPORTED_CHANNELS_MIN); // check channels fail
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(UNSUPPORTED_SAMPLE_RATE); // check sampleRate fail
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(INVALID_WIDTH); // check sampleFormat fail
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_004, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(INVALID_WIDTH); // check sampleFormat fail
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_005, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(UNSUPPORTED_CHANNELS_MAX); // check channels fail
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, SetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(MAX_INPUT_SIZE);
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, SetParameter_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(MIN_INPUT_SIZE);
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, SetParameter_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(UNSUPPORTED_MAX_INPUT_SIZE);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetFormatBytes_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    EXPECT_EQ(Status::OK, decoder_->Start());
    std::shared_ptr<Meta> config = std::make_shared<Meta>();
    EXPECT_EQ(Status::OK, decoder_->GetParameter(config));
}

HWTEST_F(RawDecoderUnitTest, GetFormatBytes_002, TestSize.Level1)
{
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
    EXPECT_EQ(Status::OK, decoder_->Start());
    std::shared_ptr<Meta> config = std::make_shared<Meta>();
    EXPECT_EQ(Status::OK, decoder_->GetParameter(config));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F64BE); // SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 64000; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(64000); // size % bytesSize == 0 && size > maxInputSize_
    EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    EXPECT_NE(Status::OK, decoder_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F64BE); // SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 9; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(9); // size % bytesSize != 0
    EXPECT_EQ(Status::ERROR_UNKNOWN, decoder_->QueueInputBuffer(inputBuffer));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32BE); // not SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 16000; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(16000); // size % bytesSize == 0
    EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    EXPECT_NE(Status::OK, decoder_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_004, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32BE); // not SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 16; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(16); // size % bytesSize == 0 && size < maxInputSize_
    EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    EXPECT_EQ(Status::OK, decoder_->QueueOutputBuffer(outputBuffer));
}
} // namespace Plugins
}
}