/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "audio_ffmpeg_aac_encoder_plugin.h"
#include "audio_ffmpeg_flac_encoder_plugin.h"
#include <set>
#include <fstream>
#include "media_description.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"
#include "ffmpeg_converter.h"
#include "audio_codec_adapter.h"
#include "meta/format.h"
#include "avcodec_audio_decoder.h"
#include "avcodec_codec_name.h"

using namespace std;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
constexpr int32_t MIN_CHANNELS = 2;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t AAC_SAMPLE_RATE = 44100;
constexpr int32_t AAC_BITRATE = 128000;
constexpr int32_t DEFAULT_AAC_TYPE = 1;
constexpr int32_t BUFFER_SIZE = 6400;
constexpr std::string_view AAC_NAME = "aac_enc_buffer";
constexpr std::string_view FLAC_NAME = "flac_enc_buffer";
constexpr uint32_t META_SIZE = 6400;
constexpr int32_t FLAC_CHANNELS = 2;
constexpr int32_t FLAC_ENC_SAMPLE_RATE = 32000;
constexpr int32_t FLAC_BITRATE = 32000;
constexpr int32_t ONE_FRAME_SIZE = 64000;
constexpr int32_t COMPLIANCE_LEVEL = 2;
}

namespace OHOS {
namespace MediaAVCodec {
class AudioEncPluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    void PrepareFlacEncoder();
    std::shared_ptr<AudioBufferInfo> CreateAudioBuffer(const std::vector<int16_t> &data, size_t offset,
        size_t size, int64_t pts, bool isEos);
    bool ProcessAudioData(const std::vector<int16_t> &pcmData, size_t offset, size_t size, int64_t pts);
    bool FlushEncoder();

protected:
    OHOS::MediaAVCodec::Format aacFormat_;
    OHOS::MediaAVCodec::Format flacFormat_;
    std::shared_ptr<AudioFFMpegAacEncoderPlugin> aacEncPlugin_ = {nullptr};
    std::shared_ptr<AudioFFMpegFlacEncoderPlugin> flacEncPlugin_ = {nullptr};
};

void AudioEncPluginUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioEncPluginUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioEncPluginUnitTest::SetUp(void)
{
    aacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    aacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AAC_SAMPLE_RATE);
    aacFormat_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, AAC_BITRATE);
    aacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    aacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);

    flacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, FLAC_CHANNELS);
    flacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    flacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, FLAC_ENC_SAMPLE_RATE);
    flacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, FLAC_BITRATE);
    flacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE, SAMPLE_S16LE);
    flacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, COMPLIANCE_LEVEL);
    flacFormat_.PutLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, AudioChannelLayout::STEREO);

    aacEncPlugin_ = std::make_shared<AudioFFMpegAacEncoderPlugin>();
    flacEncPlugin_ = std::make_shared<AudioFFMpegFlacEncoderPlugin>();
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioEncPluginUnitTest::TearDown(void)
{
    if (aacEncPlugin_) {
        aacEncPlugin_->Release();
    }
    if (flacEncPlugin_) {
        flacEncPlugin_->Release();
    }
    cout << "[TearDown]: over!!!" << endl;
}

void AudioEncPluginUnitTest::PrepareFlacEncoder()
{
    int32_t ret = flacEncPlugin_->Init(flacFormat_);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        cout << "Failed to initialize FLAC encoder" << endl;
    }
}

std::shared_ptr<AudioBufferInfo> AudioEncPluginUnitTest::CreateAudioBuffer(const std::vector<int16_t> &data,
    size_t offset, size_t size, int64_t pts, bool isEos)
{
    auto buffer = std::make_shared<AudioBufferInfo>(size, FLAC_NAME, ONE_FRAME_SIZE);
    auto memory = buffer->GetBuffer();
    if (!memory) {
        cout << "Failed to allocate memory for audio buffer" << endl;
        return nullptr;
    }

    if (size > 0) {
        if (memcpy_s(memory->GetBase(), memory->GetSize(), data.data() + offset, size) != EOK) {
            cout << "Fatal: memory copy failed (size=" << size
                 << ", dest_size=" << memory->GetSize() << ")" << endl;
        }
    }

    AVCodecBufferInfo attr = {};
    attr.size = size;
    attr.presentationTimeUs = pts;
    buffer->SetBufferAttr(attr);
    buffer->SetEos(isEos);

    return buffer;
}

bool AudioEncPluginUnitTest::ProcessAudioData(const std::vector<int16_t> &pcmData, size_t offset,
    size_t size, int64_t pts)
{
    auto inputBuffer = CreateAudioBuffer(pcmData, offset, size, pts, false);
    if (!inputBuffer) {
        return false;
    }

    int32_t ret = flacEncPlugin_->ProcessSendData(inputBuffer);
    while (true) {
        auto outputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, FLAC_NAME, ONE_FRAME_SIZE);
        ret = flacEncPlugin_->ProcessRecieveData(outputBuffer);
        if (ret == AVCodecServiceErrCode::AVCS_ERR_NOT_ENOUGH_DATA) {
            break;
        }
        if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
            cout <<"Failed to receive encoded data: " << ret << endl;
            return false;
        }
    }
    return true;
}

bool AudioEncPluginUnitTest::FlushEncoder()
{
    auto eosBuffer = CreateAudioBuffer({}, 0, 0, 0, true);
    if (!eosBuffer) {
        return false;
    }

    int32_t ret = flacEncPlugin_->ProcessSendData(eosBuffer);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        cout << "Failed to send EOS: " << ret << endl;
    }

    while (true) {
        auto outputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, FLAC_NAME, ONE_FRAME_SIZE);
        ret = flacEncPlugin_->ProcessRecieveData(outputBuffer);
        if (ret == AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM) {
            break;
        }
        if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
            cout << "Error during flush: " << ret << endl;
            return false;
        }
    }
    return true;
}


HWTEST_F(AudioEncPluginUnitTest, CheckBitRate_Aac_001, TestSize.Level1)
{
    aacFormat_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, -44100); // invalid
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
}

HWTEST_F(AudioEncPluginUnitTest, SendBuffer_Aac_001, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Release());
    auto inputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, AAC_NAME, META_SIZE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->ProcessSendData(inputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, SendBuffer_Aac_002, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    auto inputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, AAC_NAME, META_SIZE);
    inputBuffer->SetEos(false);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->ProcessSendData(inputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, SendBuffer_Aac_003, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    auto inputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, AAC_NAME, META_SIZE);
    inputBuffer->SetEos(false);

    AVCodecBufferInfo attr;
    attr.size = -6400; // invalid
    inputBuffer->SetBufferAttr(attr);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->ProcessSendData(inputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, SendBuffer_Aac_004, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    auto inputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, AAC_NAME, META_SIZE);
    inputBuffer->SetEos(false);

    AVCodecBufferInfo attr;
    attr.size = 9600; // attr.size > 6400
    inputBuffer->SetBufferAttr(attr);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->ProcessSendData(inputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, SendBuffer_Aac_005, TestSize.Level1)
{
    aacFormat_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MIN_CHANNELS);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    auto inputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, AAC_NAME, META_SIZE);
    inputBuffer->SetEos(false);

    AVCodecBufferInfo attr;
    attr.size = 6400; // valid size
    inputBuffer->SetBufferAttr(attr);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->ProcessSendData(inputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, ProcessRecieveData_Aac_001, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    auto outputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, AAC_NAME, META_SIZE);
    outputBuffer = nullptr;
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->ProcessRecieveData(outputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, ProcessRecieveData_Aac_002, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Release());
    auto outputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, AAC_NAME, META_SIZE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->ProcessRecieveData(outputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, CheckResample_Aac_001, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Release());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacEncPlugin_->Init(aacFormat_));
}

HWTEST_F(AudioEncPluginUnitTest, ProcessSendData_Flac_001, TestSize.Level1)
{
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, flacEncPlugin_->Init(flacFormat_));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, flacEncPlugin_->Release());
    auto inputBuffer = std::make_shared<AudioBufferInfo>(BUFFER_SIZE, FLAC_NAME, ONE_FRAME_SIZE);

    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, flacEncPlugin_->ProcessSendData(inputBuffer));
}

HWTEST_F(AudioEncPluginUnitTest, FlacEncodeProcess_001, TestSize.Level1)
{
    PrepareFlacEncoder();

    const int sampleRate = FLAC_ENC_SAMPLE_RATE;
    const int durationSeconds = 5;
    const int numSamples = sampleRate * durationSeconds;
    const float amplitude = 32760.0f;

    std::vector<int16_t> pcmData(numSamples * FLAC_CHANNELS);
    for (int i = 0; i < numSamples; ++i) {
        for (int ch = 0; ch < FLAC_CHANNELS; ++ch) {
            float frequency = 440.0f * (ch + 1);
            float value = amplitude * sin(2.0f * M_PI * frequency * i / sampleRate);
            pcmData[i * FLAC_CHANNELS + ch] = static_cast<int16_t>(value);
        }
    }

    int32_t frameSize = 1024;
    const int bytesPerSample = 2;
    const int bytesPerChunk = frameSize * FLAC_CHANNELS * bytesPerSample;
    const int64_t timeIncrementUs = (frameSize * 1000000LL) / sampleRate;

    int64_t pts = 0;
    for (size_t offset = 0; offset < pcmData.size() * bytesPerSample; offset += bytesPerChunk) {
        size_t chunkSize = std::min(bytesPerChunk, static_cast<int>(pcmData.size() * bytesPerSample - offset));
        if (chunkSize == 0) break;
        if (!ProcessAudioData(pcmData, offset / bytesPerSample, chunkSize, pts)) {
            break;
        }
        pts += timeIncrementUs;
    }
    EXPECT_EQ(FlushEncoder(), true);
}
} // namespace MediaAVCodec
} // namespace OHOS