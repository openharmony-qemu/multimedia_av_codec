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
#include "audio_ffmpeg_aac_decoder_plugin.h"
#include "audio_ffmpeg_amrnb_decoder_plugin.h"
#include "audio_ffmpeg_amrwb_decoder_plugin.h"
#include "audio_ffmpeg_vorbis_decoder_plugin.h"
#include "audio_g711mu_decoder_plugin.h"
#include <set>
#include <fstream>
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "avcodec_audio_common.h"
#include "ffmpeg_converter.h"
#include "audio_codec_adapter.h"
#include "audio_codec_name.h"
#include "meta/format.h"
#include "avcodec_audio_decoder.h"

using namespace std;
using namespace testing::test;

namespace {
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t DEFAULT_AAC_TYPE = 1;
constexpr int32_t AMRNB_SUPORT_CHANNELS = 1;
constexpr int32_t AMRWB_SUPORT_CHANNELS = 1;
constexpr int32_t G711MU_SUPORT_CHANNELS = 1;
constexpr int32_t AMRNB_SUPORT_SAMPLE_RATE = 8000;
constexpr int32_t AMRWB_SUPORT_SAMPLE_RATE = 16000;
constexpr int32_t G711MU_SUPORT_SAMPLE_RATE = 8000;

static std::set<int32_t> supportedSampleRate = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
                                                11025, 8000, 7350};
static std::set<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE};
static std::set<int32_t> supportedSampleRates = {7350, 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
                                                 64000, 88200, 96000};
const string CODEC_OGG_NAME = std::string(AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME);
const string INPUT_SOURCE_PATH = "/data/test/media/";
const string INPUT_OGG_FILE_WITH_HEADER[1][5] = {{"OGG_44k_2c_with_header.dat", "44100", "2", "128000", "16"}};
constexpr string_view OUTPUT_PCM_FILE_PATH = "/data/test/media/out.pcm";
}

namespace OHOS {
namespace MediaAVCodec {
class AudioDecPluginUnitTest : public testing::test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t InitOggFile(const string &codecName, string inputTestFile);
    int32_t SetVorbisHeader();
protected:
    OHOS::MediaAVCodec::Format format_;
    std::shared_ptr<AudioFFMpegAacDecoderPlugin> aacDecPlugin_ = {nullptr};
    std::shared_ptr<AudioFFMpegAmrnbDecoderPlugin> amrnbDecPlugin_ = {nullptr};
    std::shared_ptr<AudioFFMpegAmrwbDecoderPlugin> amrwbDecPlugin_ = {nullptr};
    std::shared_ptr<AudioFFMpegVorbisDecoderPlugin> vorbisDecPlugin_ = {nullptr};
    std::shared_ptr<AudioG711muDecoderPlugin> g711muDecPlugin_ = {nullptr};
    std::ifstream inputFile_;
    std::ofstream pcmOutputFile_;
    bool vorbisHasExtradata_ = true;
};

void AudioDecPluginUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioDecPluginUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioDecPluginUnitTest::SetUp(void)
{
    audioResample_ = std::make_shared<AudioResample>();
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioDecPluginUnitTest::TearDown(void)
{
    if (aacDecPlugin_) {
        aacDecPlugin_->Release();
    }
    if (amrnbDecPlugin_) {
        amrnbDecPlugin_->Release();
    }
    if (amrwbDecPlugin_) {
        amrwbDecPlugin_->Release();
    }
    if (vorbisDecPlugin_) {
        vorbisDecPlugin_->Release();
    }
    if (g711muDecPlugin_) {
        g711muDecPlugin_->Release();
    }
    cout << "[TearDown]: over!!!" << endl;
}

int32_t AudioDecPluginUnitTest::InitOggFile(const string &codecName, string inputTestFile)
{
    if (vorbisDecPlugin_ == nullptr) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    inputFile_.open(INPUT_SOURCE_PATH + inputTestFile, std::ios::binary);
    pcmOutputFile_.open(OUTPUT_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    if (!inputFile_.is_open()) {
        cout << "Fatal: open input file failed" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    if (codecName.compare(CODEC_OGG_NAME) == 0) {
        if (!vorbisHasExtradata_) {
            return SetVorbisHeader();
        }
        int64_t extradataSize;
        inputFile_.read(reinterpret_cast<char *>(&extradataSize), sizeof(int64_t));
        if (extradataSize < 0) {
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
        char buffer[extradataSize];
        inputFile_.read(buffer, extradataSize);
        if (inputFile_.gcount() != extradataSize) {
            cout << "read extradata bytes error" << endl;
        }
        format_.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extradataSize);
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioDecPluginUnitTest::SetVorbisHeader()
{
    // set identification header
    int64_t headerSize;
    inputFile_.read(reinterpret_cast<char *>(&headerSize), sizeof(int64_t));
    if (headerSize < 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    char idBuffer[headerSize];
    inputFile_.read(idBuffer, headerSize);
    if (inputFile_.gcount() != headerSize) {
        cout << "read extradata bytes error" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    format_.PutBuffer(MediaDescriptionKey::MD_KEY_IDENTIFICATION_HEADER, (uint8_t *)idBuffer, headerSize);

    // set setup header
    inputFile_.read(reinterpret_cast<char *>(&headerSize), sizeof(int64_t));
    if (headerSize < 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    char setupBuffer[headerSize];
    inputFile_.read(setupBuffer, headerSize);
    if (inputFile_.gcount() != headerSize) {
        cout << "read extradata bytes error" << endl;
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    format_.PutBuffer(MediaDescriptionKey::MD_KEY_SETUP_HEADER, (uint8_t *)setupBuffer, headerSize);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

/**
 * @tc.name: CheckSampleFormat_Aac_001
 * @tc.desc: init success
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Aac_002
 * @tc.desc: unsupported aac samplerate
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_002, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44444); // unsupported aac samplerate
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Aac_003
 * @tc.desc: unsupported aac sampleformat
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_003, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16P);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Aac_004
 * @tc.desc: channels_ == 1 && sampleFormat == SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_004, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MIN_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Aac_005
 * @tc.desc: channels_ == 1 && sampleFormat != SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_005, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MIN_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Aac_006
 * @tc.desc: channels_ != 1 && sampleFormat != SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_006, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Aac_007
 * @tc.desc: channels_ != 1 && sampleFormat == SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_006, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckChannelCount_Aac_001
 * @tc.desc: channel_count missing
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckChannelCount_Aac_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    auto ret = aacDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Amrnb_001
 * @tc.desc: sampleformat not missing and EQ SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Amrnb_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRNB_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    amrnbDecPlugin_ = std::make_shared<AudioFFMpegAmrnbDecoderPlugin>();
    auto ret = amrnbDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Amrnb_002
 * @tc.desc: sampleformat not missing and EQ SAMPLE_S16LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Amrnb_002, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRNB_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    amrnbDecPlugin_ = std::make_shared<AudioFFMpegAmrnbDecoderPlugin>();
    auto ret = amrnbDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Amrnb_003
 * @tc.desc: sampleformat not missing but unsupported
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Amrnb_003, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRNB_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16P);
    amrnbDecPlugin_ = std::make_shared<AudioFFMpegAmrnbDecoderPlugin>();
    auto ret = amrnbDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Amrwb_001
 * @tc.desc: sampleformat not missing and EQ SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Amrwb_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRWB_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRWB_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    amrwbDecPlugin_ = std::make_shared<AudioFFMpegAmrwbDecoderPlugin>();
    auto ret = amrwbDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Amrwb_002
 * @tc.desc: sampleformat not missing and EQ SAMPLE_S16LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Amrwb_002, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRWB_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    amrwbDecPlugin_ = std::make_shared<AudioFFMpegAmrwbDecoderPlugin>();
    auto ret = amrwbDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Amrwb_003
 * @tc.desc: sampleformat not missing but unsupported
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Amrwb_003, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRWB_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16P);
    amrwbDecPlugin_ = std::make_shared<AudioFFMpegAmrwbDecoderPlugin>();
    auto ret = amrwbDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Vorbis_001
 * @tc.desc: channels_ == 1 && sampleFormat == SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Vorbis_001, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][1]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MIN_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Vorbis_002
 * @tc.desc: channels_ == 1 && sampleFormat != SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Vorbis_002, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][1]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MIN_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Vorbis_003
 * @tc.desc: channels_ != 1 && sampleFormat == SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Vorbis_003, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][1]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_Vorbis_004
 * @tc.desc: channels_ != 1 && sampleFormat != SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Vorbis_004, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][1]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckChannelCount_Vorbis_001
 * @tc.desc: missing channelCount
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckChannelCount_Vorbis_001, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][1]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleRate_Vorbis_001
 * @tc.desc: missing samplerate
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleRate_Vorbis_001, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleRate_Vorbis_002
 * @tc.desc: LT min samplerate
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleRate_Vorbis_002, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 7000); // invalid samplerate
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleRate_Vorbis_003
 * @tc.desc: GT max samplerate
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleRate_Vorbis_003, TestSize.Level1)
{
    vorbisHasExtradata_ = false;
    vorbisDecPlugin_ = std::make_shared<AudioFFMpegVorbisDecoderPlugin>();

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 200000); // invalid samplerate
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, stoi(INPUT_OGG_FILE_WITH_HEADER[0][3]));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, MAX_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_G711mu_001
 * @tc.desc: CheckInit success
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_G711mu_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, G711MU_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, G711MU_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    g711muDecPlugin_ = std::make_shared<AudioG711muDecoderPlugin>();
    auto ret = g711muDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_G711mu_002
 * @tc.desc: unsupported channelcount
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_G711mu_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2); // unsupported channelcount
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, G711MU_SUPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    g711muDecPlugin_ = std::make_shared<AudioG711muDecoderPlugin>();
    auto ret = g711muDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckSampleFormat_G711mu_001
 * @tc.desc: unsupported samplerate
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_G711mu_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, G711MU_SUPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 192000); // unsupported samplerate
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    g711muDecPlugin_ = std::make_shared<AudioG711muDecoderPlugin>();
    auto ret = g711muDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}
} // namespace MediaAVCodec
} // namespace OHOS