/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include <algorithm>
#include <cstring>
#include <vector>
#include <iostream>
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "dsd_encode_data.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace {
const string CODEC_DSD_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_DSD_NAME);
// DSD64: the raw rate is 64 * 44100. Following the demuxer convention (libavformat/dsfdec.c),
// the decoder is configured with the raw rate / 8.
constexpr int32_t DSD_SAMPLE_RATE = 352800;
constexpr int32_t DSD_CHANNEL_COUNT = 1;
constexpr int32_t DSD_MAX_INPUT_SIZE = 8192;
// The dsd2pcm decimation filter spans 5 bytes, keep a margin when checking segment edges.
constexpr uint32_t DSD_FILTER_MARGIN = 16;
constexpr int16_t DSD_DC_CHECK_LIMIT = 20000;
// Same as SAMPLES * MAX_CHANNELS in the plugin: the default max input buffer size.
constexpr int32_t DSD_DEFAULT_MAX_INPUT_SIZE = 24576;
// Values of OH_AVCodec_DSDCodecType, defined in interfaces/kits/c/native_avcodec_audiobase.h.
constexpr int32_t DSD_CODEC_LSBF_PLANAR = 0;
constexpr int32_t DSD_CODEC_MSBF_PLANAR = 1;
constexpr int32_t DSD_CODEC_MSBF = 2;
} // namespace

class AudioDSDUnitTest : public testing::Test, public DataCallback {
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
    shared_ptr<Media::Meta> CreateMeta();
    shared_ptr<AVBuffer> CreateAVBuffer(size_t capacity, size_t size);
    void QueueInput(const uint8_t *data, size_t size, uint32_t flag = 0);
    void DrainOutput(vector<int16_t> &pcm, bool &eosReached);
    void DecodeStream(const vector<uint8_t> &stream, vector<int16_t> &pcm);

    shared_ptr<CodecPlugin> plugin_ = nullptr;
    shared_ptr<Media::Meta> meta_ = nullptr;
};

void AudioDSDUnitTest::SetUpTestCase(void) {}

void AudioDSDUnitTest::TearDownTestCase(void) {}

void AudioDSDUnitTest::SetUp(void)
{
    plugin_ = CreatePlugin();
    ASSERT_NE(plugin_, nullptr);
    meta_ = CreateMeta();
    ASSERT_NE(meta_, nullptr);
}

void AudioDSDUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
    }
}

shared_ptr<CodecPlugin> AudioDSDUnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_DSD_DEC_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<Media::Meta> AudioDSDUnitTest::CreateMeta()
{
    auto meta = make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(DSD_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(DSD_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_LSBF_PLANAR);
    return meta;
}

shared_ptr<AVBuffer> AudioDSDUnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    avBuffer->memory_->SetSize(size);
    return avBuffer;
}

void AudioDSDUnitTest::QueueInput(const uint8_t *data, size_t size, uint32_t flag)
{
    auto inputBuffer = CreateAVBuffer(size > 0 ? size : 1024, size);  // 1024
    if (data != nullptr && size > 0) {
        if (memcpy_s(inputBuffer->memory_->GetAddr(), size, data, size) != EOK) {
            std::cout << "QueueInput memcpy_s err" << std::endl;
            return;
        }
    }
    inputBuffer->flag_ = flag;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
}

void AudioDSDUnitTest::DrainOutput(vector<int16_t> &pcm, bool &eosReached)
{
    while (true) {
        auto outputBuffer = CreateAVBuffer(DSD_MAX_INPUT_SIZE * 4, 0);  // 4
        Status status = plugin_->QueueOutputBuffer(outputBuffer);
        size_t bytes = outputBuffer->memory_->GetSize();
        if (bytes > 0) {
            auto samples = reinterpret_cast<const int16_t *>(outputBuffer->memory_->GetAddr());
            pcm.insert(pcm.end(), samples, samples + bytes / sizeof(int16_t));
        }
        if (status == Status::END_OF_STREAM) {
            eosReached = true;
            break;
        }
        if (status == Status::ERROR_NOT_ENOUGH_DATA) {
            break;
        }
        ASSERT_TRUE(status == Status::OK || status == Status::ERROR_AGAIN);
    }
}

void AudioDSDUnitTest::DecodeStream(const vector<uint8_t> &stream, vector<int16_t> &pcm)
{
    bool eosReached = false;
    constexpr size_t chunk = 512;  // 512
    for (size_t pos = 0; pos < stream.size(); pos += chunk) {
        size_t size = min(chunk, stream.size() - pos);
        QueueInput(stream.data() + pos, size);
        DrainOutput(pcm, eosReached);
    }
    QueueInput(nullptr, 0, MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS);
    DrainOutput(pcm, eosReached);
    ASSERT_TRUE(eosReached);
}

/**
 * @tc.name: DSD_SetParameter_001
 * @tc.desc: init success, SetParameter success
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_002
 * @tc.desc: init success, SetParameter failed when channel count == 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_003
 * @tc.desc: init success, SetParameter failed when channel count < 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(-999);  // -999
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_004
 * @tc.desc: init success, SetParameter failed when sample rate == 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_004, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_005
 * @tc.desc: init success, SetParameter failed when sample rate < 0
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_005, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(-999);  // -999
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_006
 * @tc.desc: init success, SetParameter failed when remove channel count
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_006, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_007
 * @tc.desc: init success, SetParameter failed when remove sample rate
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_007, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_008
 * @tc.desc: init success, SetParameter failed when channel count > 6 (dsf/dff container limit)
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_008, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(7);  // 7
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_009
 * @tc.desc: init success, DSD1024 rate (5644800) accepted, no upper limit like ffmpeg dsddec
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_009, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(5644800);  // 5644800
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_010
 * @tc.desc: init success, codec type MSB interleaved (DFF) accepted
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_010, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_MSBF);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_011
 * @tc.desc: init success, remove codec type key, default LSBF_PLANAR is used
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_011, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_DSD_CODEC_TYPE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_Lifecycle_001
 * @tc.desc: test plugin lifecycle
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Lifecycle_001, TestSize.Level1)
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
 * @tc.name: DSD_Eos_001
 * @tc.desc: test End of Stream
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Eos_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(1024, 0);  // 1024
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    auto outputBuffer = CreateAVBuffer(DSD_MAX_INPUT_SIZE * 4, 0);  // 4
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

/**
 * @tc.name: DSD_GetParameter_001
 * @tc.desc: test GetParameter (channel count, sample rate, max input size)
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_GetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, DSD_CHANNEL_COUNT);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_EQ(sampleRate, DSD_SAMPLE_RATE);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

/**
 * @tc.name: DSD_Decode_001
 * @tc.desc: decode the encoded test stream, check sample count and per-segment amplitude.
 * One DSD byte per channel decodes to exactly one PCM sample. The dsd2pcm decimation filter
 * spans 5 bytes, so segment edge samples are skipped. A sustained 0x00 byte decodes to
 * full-scale negative DC and a sustained 0xFF byte to full-scale positive DC.
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Decode_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);

    vector<int16_t> pcm;
    DecodeStream(DsdTest::BuildTestStream(), pcm);

    ASSERT_EQ(pcm.size(), static_cast<size_t>(DsdTest::DSD_TOTAL_SIZE));

    // Segment layout: [0] silence(0x69), [1] negative DC(0x00), [2] positive DC(0xFF), [3] square.
    for (uint32_t i = DSD_FILTER_MARGIN; i < DsdTest::DSD_SEGMENT_SIZE - DSD_FILTER_MARGIN; i++) {
        ASSERT_LT(pcm[DsdTest::DSD_SEGMENT_SIZE + i], -DSD_DC_CHECK_LIMIT);
        ASSERT_GT(pcm[2 * DsdTest::DSD_SEGMENT_SIZE + i], DSD_DC_CHECK_LIMIT);  // 2
    }
    int16_t squareMax = pcm[3 * DsdTest::DSD_SEGMENT_SIZE];
    int16_t squareMin = squareMax;
    for (uint32_t i = DSD_FILTER_MARGIN; i < DsdTest::DSD_SEGMENT_SIZE - DSD_FILTER_MARGIN; i++) {
        int16_t value = pcm[3 * DsdTest::DSD_SEGMENT_SIZE + i];  // 3
        squareMax = max(squareMax, value);
        squareMin = min(squareMin, value);
    }
    ASSERT_GT(squareMax, DSD_DC_CHECK_LIMIT);
    ASSERT_LT(squareMin, -DSD_DC_CHECK_LIMIT);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

/**
 * @tc.name: DSD_Decode_002
 * @tc.desc: arbitrary bytes are all valid DSD data, decode must not fail and the output
 * sample count must match the input byte count
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Decode_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);

    vector<uint8_t> garbage(1024);
    for (size_t i = 0; i < garbage.size(); i++) {
        garbage[i] = static_cast<uint8_t>((i * 7 + 3) & 0xFF); // 7 3
    }
    vector<int16_t> pcm;
    DecodeStream(garbage, pcm);

    ASSERT_EQ(pcm.size(), garbage.size());
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_012
 * @tc.desc: init success, codec type MSB planar accepted
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_012, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_MSBF_PLANAR);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_013
 * @tc.desc: init success, unknown codec type value falls back to the default LSBF_PLANAR
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_013, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(99);  // 99
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_014
 * @tc.desc: init success, remove sample format key, default S16LE is used
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_014, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_SAMPLE_FORMAT);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_015
 * @tc.desc: sample format S24LE is not supported, falls back to S16LE: SetParameter succeeds
 * and the output format metadata reports S16LE after decoding
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_015, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S24LE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> pcm;
    bool eosReached = false;
    auto stream = DsdTest::BuildTestStream();
    QueueInput(stream.data(), DsdTest::DSD_SEGMENT_SIZE);
    DrainOutput(pcm, eosReached);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    AudioSampleFormat sampleFormat = AudioSampleFormat::INVALID_WIDTH;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat));
    ASSERT_EQ(sampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_016
 * @tc.desc: init success, explicit stereo channel layout with 2 channels accepted
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_016, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(2);  // 2
    meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_017
 * @tc.desc: init success, 6 channels (container upper bound) without layout uses default layout
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_017, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(6);  // 6
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_SetParameter_018
 * @tc.desc: init success, minimal positive sample rate accepted (no lower limit besides > 0)
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_SetParameter_018, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(1);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

/**
 * @tc.name: DSD_GetParameter_002
 * @tc.desc: max input size negotiation: small value is kept, large/negative values fall
 * back to the plugin default (SAMPLES * MAX_CHANNELS)
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_GetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    int32_t maxInputSize = 0;
    int32_t smallInputSize = 4096;  // 4096
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(smallInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, smallInputSize);
    int32_t largeInputSize = 200000;  // 200000
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(largeInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, DSD_DEFAULT_MAX_INPUT_SIZE);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, DSD_DEFAULT_MAX_INPUT_SIZE);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_DEFAULT_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, DSD_DEFAULT_MAX_INPUT_SIZE);
}

/**
 * @tc.name: DSD_Flush_001
 * @tc.desc: flush in the middle of decoding, the following data still decodes 1:1
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Flush_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);

    auto stream = DsdTest::BuildTestStream();
    vector<int16_t> pcm;
    bool eosReached = false;
    QueueInput(stream.data(), DsdTest::DSD_SEGMENT_SIZE);
    DrainOutput(pcm, eosReached);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    QueueInput(stream.data() + DsdTest::DSD_SEGMENT_SIZE, DsdTest::DSD_SEGMENT_SIZE);
    DrainOutput(pcm, eosReached);
    ASSERT_EQ(pcm.size(), static_cast<size_t>(2 * DsdTest::DSD_SEGMENT_SIZE));
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

/**
 * @tc.name: DSD_Decode_003
 * @tc.desc: output buffer smaller than one frame: the frame is split across output buffers
 * (againIndex path) and the total sample count still matches the input byte count
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Decode_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);

    auto stream = DsdTest::BuildTestStream();
    QueueInput(stream.data(), DsdTest::DSD_SEGMENT_SIZE);

    // 600 bytes < one frame (512 samples * 2 bytes), forces the split path.
    constexpr size_t smallCapacity = 600;  // 600
    vector<int16_t> pcm;
    while (true) {
        auto outputBuffer = CreateAVBuffer(smallCapacity, 0);
        Status status = plugin_->QueueOutputBuffer(outputBuffer);
        size_t bytes = outputBuffer->memory_->GetSize();
        if (bytes > 0) {
            auto samples = reinterpret_cast<const int16_t *>(outputBuffer->memory_->GetAddr());
            pcm.insert(pcm.end(), samples, samples + bytes / sizeof(int16_t));
        }
        if (status == Status::END_OF_STREAM || status == Status::ERROR_NOT_ENOUGH_DATA) {
            break;
        }
        ASSERT_TRUE(status == Status::OK || status == Status::ERROR_AGAIN);
    }
    ASSERT_EQ(pcm.size(), static_cast<size_t>(DsdTest::DSD_SEGMENT_SIZE));
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

/**
 * @tc.name: DSD_Decode_004
 * @tc.desc: bit order must be honored. Decoding the bit-reversed stream with the MSBF
 * decoder produces exactly the same output as decoding the original stream with the LSBF
 * decoder (ctables_lsbf[i][w] == ctables_msbf[i][reverse(w)]), while decoding the same
 * stream with both decoders must differ (the stream carries non-palindromic edge bytes).
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Decode_004, TestSize.Level1)
{
    auto stream = DsdTest::BuildBitOrderStream();
    auto reversed = DsdTest::BitReverseBytes(stream);

    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);

    // pass 1: LSBF_PLANAR on the original stream
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> lsbfOut;
    DecodeStream(stream, lsbfOut);
    ASSERT_EQ(plugin_->Stop(), Status::OK);

    // pass 2: MSBF_PLANAR on the bit-reversed stream
    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_MSBF_PLANAR);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> msbfRevOut;
    DecodeStream(reversed, msbfRevOut);
    ASSERT_EQ(plugin_->Stop(), Status::OK);

    // pass 3: MSBF_PLANAR on the original stream
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> msbfOut;
    DecodeStream(stream, msbfOut);
    ASSERT_EQ(plugin_->Stop(), Status::OK);

    ASSERT_EQ(lsbfOut.size(), msbfRevOut.size());
    ASSERT_EQ(lsbfOut.size(), msbfOut.size());
    int32_t maxDiff = 0;
    for (size_t k = 0; k < lsbfOut.size(); k++) {
        // exact cross identity between LSBF(original) and MSBF(bit-reversed)
        ASSERT_EQ(lsbfOut[k], msbfRevOut[k]);
        int32_t diff = (lsbfOut[k] > msbfOut[k]) ? (lsbfOut[k] - msbfOut[k]) : (msbfOut[k] - lsbfOut[k]);
        maxDiff = max(maxDiff, diff);
    }
    ASSERT_GT(maxDiff, 0);
}

/**
 * @tc.name: DSD_Decode_005
 * @tc.desc: for mono streams the planar and interleaved sample addressing coincide, so
 * MSBF_PLANAR and MSBF (interleaved) must produce identical output
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Decode_005, TestSize.Level1)
{
    auto stream = DsdTest::BuildTestStream();

    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);

    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_MSBF_PLANAR);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> planarOut;
    DecodeStream(stream, planarOut);
    ASSERT_EQ(plugin_->Stop(), Status::OK);

    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_MSBF);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> interleavedOut;
    DecodeStream(stream, interleavedOut);
    ASSERT_EQ(plugin_->Stop(), Status::OK);

    ASSERT_EQ(planarOut.size(), interleavedOut.size());
    for (size_t k = 0; k < planarOut.size(); k++) {
        ASSERT_EQ(planarOut[k], interleavedOut[k]);
    }
}

/**
 * @tc.name: DSD_Decode_006
 * @tc.desc: stereo decoding: the planar stream ([L plane][R plane], MSBF_PLANAR) and the
 * interleaved stream (L,R,L,R..., MSBF) must decode to the same per-channel output, the
 * channels must stay separated (left full-scale positive, right full-scale negative).
 * @tc.type: FUNC
 */
HWTEST_F(AudioDSDUnitTest, DSD_Decode_006, TestSize.Level1)
{
    // 256 bytes per plane: the whole planar stream (512 bytes) fits in one 512-byte chunk,
    // so each queued packet carries a complete plane pair as the planar decoder expects.
    constexpr uint32_t planeSize = 256;  // 256
    auto left = DsdTest::EncodeFlat(0xFF, planeSize);
    auto right = DsdTest::EncodeFlat(0x00, planeSize);
    auto interleaved = DsdTest::InterleaveChannels(left, right);
    auto planar = DsdTest::MakePlanar(left, right);

    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(DSD_MAX_INPUT_SIZE);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(2);

    // pass 1: MSBF (interleaved layout)
    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_MSBF);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> interleavedOut;
    DecodeStream(interleaved, interleavedOut);
    ASSERT_EQ(plugin_->Stop(), Status::OK);

    // pass 2: MSBF_PLANAR (planar layout)
    meta_->Set<Tag::AUDIO_DSD_CODEC_TYPE>(DSD_CODEC_MSBF_PLANAR);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    vector<int16_t> planarOut;
    DecodeStream(planar, planarOut);
    ASSERT_EQ(plugin_->Stop(), Status::OK);

    ASSERT_EQ(interleavedOut.size(), static_cast<size_t>(2 * planeSize));
    ASSERT_EQ(planarOut.size(), interleavedOut.size());
    for (uint32_t k = 0; k < planeSize; k++) {
        // The decoded output is packed interleaved S16LE ([L0, R0, L1, R1, ...]), for both
        // the interleaved and the planar input stream.
        int16_t interL = interleavedOut[2 * k];  // 2
        int16_t interR = interleavedOut[2 * k + 1];  // 2
        int16_t planarL = planarOut[2 * k];  // 2
        int16_t planarR = planarOut[2 * k + 1];  // 2
        // planar and interleaved decode to the same per-channel output
        ASSERT_EQ(interL, planarL);
        ASSERT_EQ(interR, planarR);
        if (k >= DSD_FILTER_MARGIN) {
            // left channel is 0xFF data (positive DC), right channel is 0x00 data (negative DC)
            ASSERT_GT(interL, DSD_DC_CHECK_LIMIT);
            ASSERT_LT(interR, -DSD_DC_CHECK_LIMIT);
        }
    }
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS
