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
#include "avcodec_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace {
const string CODEC_DTS_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_DTS_NAME);
constexpr int32_t DTS_SAMPLE_RATE = 44100;
constexpr int32_t DTS_CHANNEL_COUNT = 2;
constexpr int64_t DTS_BIT_RATE = 192000;
constexpr int32_t DTS_MAX_INPUT_SIZE = 8192;
} // namespace

class AudioDTSUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    void OnInputBufferDone(const shared_ptr<Media::AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
    }
    void OnOutputBufferDone(const shared_ptr<Media::AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
    }
    void OnEvent(const shared_ptr<Plugins::PluginEvent> event) override
    {
        (void)event;
    }

protected:
    shared_ptr<CodecPlugin> CreatePlugin();
    void DestroyPlugin();
    shared_ptr<Media::Meta> CreateMeta();
    shared_ptr<AVBuffer> CreateAVBuffer(size_t capacity, size_t size);
    bool ReadOneFrame(ifstream &file, shared_ptr<AVBuffer> &buffer);
    void ProcessInputFrames(ifstream &file, int &inputFrameCount, int &outputFrameCount, bool &sentEos);
    void ProcessOutputFrames(int &outputFrameCount);
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    shared_ptr<Media::Meta> meta_ = nullptr;
};

void AudioDTSUnitTest::SetUpTestCase(void) {}

void AudioDTSUnitTest::TearDownTestCase(void) {}

void AudioDTSUnitTest::SetUp(void)
{
    plugin_ = CreatePlugin();
    ASSERT_NE(plugin_, nullptr);
    meta_ = CreateMeta();
    ASSERT_NE(meta_, nullptr);
}

void AudioDTSUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
    }
}

shared_ptr<CodecPlugin> AudioDTSUnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_DTS_DEC_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<Media::Meta> AudioDTSUnitTest::CreateMeta()
{
    auto meta = make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(DTS_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DTS_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    meta->Set<Tag::MEDIA_BITRATE>(DTS_BIT_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    return meta;
}

shared_ptr<AVBuffer> AudioDTSUnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    avBuffer->memory_->SetSize(size);
    return avBuffer;
}

/**
 * @tc.name: DTS_SetParameter_001
 * @tc.desc: init success, SetParameter success
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_SetParameter_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DTS_SetParameter_002
 * @tc.desc: init success, SetParameter failed when channel count == 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_SetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DTS_SetParameter_003
 * @tc.desc: init success, SetParameter failed when channel count < 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_SetParameter_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(-999);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DTS_SetParameter_004
 * @tc.desc: init success, SetParameter failed when sample rate == 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_SetParameter_004, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DTS_SetParameter_005
 * @tc.desc: init success, SetParameter failed when sample rate < 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_SetParameter_005, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(-999);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DTS_SetParameter_006
 * @tc.desc: init success, SetParameter failed when remove channel count
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_SetParameter_006, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DTS_SetParameter_007
 * @tc.desc: init success, SetParameter failed when remove channel count
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_SetParameter_007, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DTS_Lifecycle_001
 * @tc.desc: test plugin lifecycle
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_Lifecycle_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->Release(), Status::OK);
}

/**
 * @tc.name: DTS_Eos_001
 * @tc.desc: test End of Stream
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_Eos_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(1024, 0);
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    auto outputBuffer = CreateAVBuffer(DTS_MAX_INPUT_SIZE * 4, 0);
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

/**
 * @tc.name: DTS_GetParameter_001
 * @tc.desc: test GetParameter (channel count, sample rate, max input size)
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_GetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DTS_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, DTS_CHANNEL_COUNT);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_EQ(sampleRate, DTS_SAMPLE_RATE);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

/**
 * @tc.name: DTS_GetParameter_002
 * @tc.desc: test GetParameter, Get<Tag::AUDIO_MAX_INPUT_SIZE>()
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_GetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    int32_t defaultInputSize = 8192;
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    int32_t maxInputSize = 0;
    int32_t smallInputSize = 4096;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(smallInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, smallInputSize);
    int32_t largeInputSize = 200000;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(largeInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, defaultInputSize);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, defaultInputSize);
}

/**
 * @tc.name: DTS_Decoder_With_Invalid_File_001
 * @tc.desc: test
 * @tc.type: FUNC
 */
HWTEST_F(AudioDTSUnitTest, DTS_Decoder_With_Invalid_File_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DTS_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(DTS_MAX_INPUT_SIZE, 1024);
    auto outputBuffer = CreateAVBuffer(DTS_MAX_INPUT_SIZE * 4, 0);
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::ERROR_INVALID_DATA);
    Status status = plugin_->QueueOutputBuffer(outputBuffer);
    ASSERT_TRUE(status == Status::OK || status == Status::ERROR_NOT_ENOUGH_DATA || status == Status::ERROR_UNKNOWN);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS
