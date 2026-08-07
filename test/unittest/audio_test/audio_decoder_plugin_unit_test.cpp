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
#include <vector>
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
#include "securec.h"

using namespace std;
using namespace testing::ext;

namespace {
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t DEFAULT_AAC_TYPE = 1;
constexpr int32_t AMRNB_SUPPORT_CHANNELS = 1;
constexpr int32_t AMRWB_SUPPORT_CHANNELS = 1;
constexpr int32_t G711MU_SUPPORT_CHANNELS = 1;
constexpr int32_t AMRNB_SUPPORT_SAMPLE_RATE = 8000;
constexpr int32_t AMRWB_SUPPORT_SAMPLE_RATE = 16000;
constexpr int32_t G711MU_SUPPORT_SAMPLE_RATE = 8000;

static std::set<int32_t> supportedSampleRate = {96000, 88200, 64000, 48000, 44100, 32000, 24000, 22050, 16000, 12000,
                                                11025, 8000, 7350};
static std::set<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE,
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE};
static std::set<int32_t> supportedSampleRates = {7350, 8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000,
                                                 64000, 88200, 96000};
const string CODEC_OGG_NAME = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME);
const string INPUT_SOURCE_PATH = "/data/test/media/";
const string INPUT_OGG_FILE_WITH_HEADER[1][5] = {{"OGG_44k_2c_with_header.dat", "44100", "2", "128000", "16"}};
const string INPUT_AAC_FILE = "aac_2c_44100hz_199k.dat";
constexpr string_view OUTPUT_PCM_FILE_PATH = "/data/test/media/out.pcm";
}

namespace OHOS {
namespace MediaAVCodec {
class AudioDecPluginUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t InitOggFile(const string &codecName, string inputTestFile);
    int32_t SetVorbisHeader();
    int32_t InitAacFile(const string &inputTestFile);
    bool ReadAacFrameData(std::vector<uint8_t> &data);
    int32_t SendAacFrame(const std::shared_ptr<AudioBufferInfo> &inBuf, const std::vector<uint8_t> &data,
                         int64_t pts);
    int32_t SendAacEos(const std::shared_ptr<AudioBufferInfo> &inBuf);
    void DrainAacOutput(std::vector<int64_t> &ptsList, uint32_t outCap);
    void SendAndDrain(const std::shared_ptr<AudioBufferInfo> &inBuf, const std::vector<uint8_t> &data,
                      int64_t pts, std::vector<int64_t> &ptsList, uint32_t outCap);
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

int32_t AudioDecPluginUnitTest::InitAacFile(const string &inputTestFile)
{
    // AAC ADTS is self-contained: the .dat file is just [size:int64][pts:int64][data] frames, no header.
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
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

bool AudioDecPluginUnitTest::ReadAacFrameData(std::vector<uint8_t> &data)
{
    data.clear();
    int64_t size = 0;
    inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_.eof() || inputFile_.gcount() != sizeof(size) || size <= 0) {
        return false;
    }
    int64_t pts = 0; // pts read from file is discarded; tests feed their own pts
    inputFile_.read(reinterpret_cast<char *>(&pts), sizeof(pts));
    if (inputFile_.gcount() != sizeof(pts)) {
        return false;
    }
    data.resize(static_cast<size_t>(size));
    inputFile_.read(reinterpret_cast<char *>(data.data()), size);
    if (inputFile_.gcount() != size) {
        return false;
    }
    return true;
}

int32_t AudioDecPluginUnitTest::SendAacFrame(const std::shared_ptr<AudioBufferInfo> &inBuf,
                                             const std::vector<uint8_t> &data, int64_t pts)
{
    if (inBuf == nullptr || data.empty()) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    auto mem = inBuf->GetBuffer();
    if (mem == nullptr || static_cast<size_t>(mem->GetSize()) < data.size()) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    errno_t ret = memcpy_s(mem->GetBase(), static_cast<size_t>(mem->GetSize()), data.data(), data.size());
    if (ret != 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    AVCodecBufferInfo info;
    info.size = static_cast<int32_t>(data.size());
    info.presentationTimeUs = pts;
    inBuf->SetBufferAttr(info);
    return aacDecPlugin_->ProcessSendData(inBuf);
}

int32_t AudioDecPluginUnitTest::SendAacEos(const std::shared_ptr<AudioBufferInfo> &inBuf)
{
    if (inBuf == nullptr) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    inBuf->SetEos(true);
    AVCodecBufferInfo info;
    info.size = 0;
    info.presentationTimeUs = 0;
    inBuf->SetBufferAttr(info);
    return aacDecPlugin_->ProcessSendData(inBuf);
}

void AudioDecPluginUnitTest::DrainAacOutput(std::vector<int64_t> &ptsList, uint32_t outCap)
{
    constexpr int32_t maxDrain = 256; // avoid infinite loop on misbehaving decoder
    for (int32_t i = 0; i < maxDrain; i++) {
        auto outBuf = std::make_shared<AudioBufferInfo>(outCap, "aac_out");
        auto ret = aacDecPlugin_->ProcessRecieveData(outBuf);
        if (ret == AVCodecServiceErrCode::AVCS_ERR_OK) {
            ptsList.push_back(outBuf->GetBufferAttr().presentationTimeUs);
        } else {
            break; // AVCS_ERR_NOT_ENOUGH_DATA / AVCS_ERR_END_OF_STREAM
        }
    }
}

void AudioDecPluginUnitTest::SendAndDrain(const std::shared_ptr<AudioBufferInfo> &inBuf,
                                          const std::vector<uint8_t> &data, int64_t pts,
                                          std::vector<int64_t> &ptsList, uint32_t outCap)
{
    // Send one packet then drain available output. Draining after every send keeps the decoder's
    // internal frame buffer empty so the next send does not fail with EAGAIN (NOT_ENOUGH_DATA).
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, SendAacFrame(inBuf, data, pts));
    DrainAacOutput(ptsList, outCap);
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
HWTEST_F(AudioDecPluginUnitTest, CheckSampleFormat_Aac_007, TestSize.Level1)
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
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRNB_SUPPORT_SAMPLE_RATE);
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
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRNB_SUPPORT_SAMPLE_RATE);
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
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRNB_SUPPORT_SAMPLE_RATE);
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
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRWB_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRWB_SUPPORT_SAMPLE_RATE);
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
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRWB_SUPPORT_SAMPLE_RATE);
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
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, AMRNB_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, AMRWB_SUPPORT_SAMPLE_RATE);
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
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
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitOggFile(CODEC_OGG_NAME, INPUT_OGG_FILE_WITH_HEADER[0][0]));
    auto ret = vorbisDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckInit_g711mu_001
 * @tc.desc: CheckInit success
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckInit_g711mu_001, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, G711MU_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, G711MU_SUPPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    g711muDecPlugin_ = std::make_shared<AudioG711muDecoderPlugin>();
    auto ret = g711muDecPlugin_->Init(format_);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckInit_g711mu_002
 * @tc.desc: unsupported channelcount
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckInit_g711mu_002, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2); // unsupported channelcount
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, G711MU_SUPPORT_SAMPLE_RATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    g711muDecPlugin_ = std::make_shared<AudioG711muDecoderPlugin>();
    auto ret = g711muDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: CheckInit_g711mu_003
 * @tc.desc: unsupported samplerate
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, CheckInit_g711mu_003, TestSize.Level1)
{
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, G711MU_SUPPORT_CHANNELS);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 192000); // unsupported samplerate
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    g711muDecPlugin_ = std::make_shared<AudioG711muDecoderPlugin>();
    auto ret = g711muDecPlugin_->Init(format_);
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, ret);
}

/**
 * @tc.name: AacPtsInterpolation_001
 * @tc.desc: pts interpolation path (cachedFrame pts == AV_NOPTS_VALUE); verify the computed pts equals
 *           cur + distance * (bufferIndex - 1) / bufferNum and stays within int64 range.
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, AacPtsInterpolation_001, TestSize.Level1)
{
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2); // aac_2c_44100hz_199k.dat is 2-channel
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitAacFile(INPUT_AAC_FILE));
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacDecPlugin_->Init(format_));

    constexpr uint32_t inCap = 16384;  // 16 KiB input buffer, large enough for an AAC ADTS frame
    constexpr uint32_t outCap = 65536; // 64 KiB output buffer
    auto inBuf = std::make_shared<AudioBufferInfo>(inCap, "aac_in");
    ASSERT_TRUE(inBuf != nullptr);

    std::vector<int64_t> ptsList;
    std::vector<uint8_t> data;
    // Frame1 pts=0 (valid): pre/cur stay 0, bufferGroupPtsDistance stays 0.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, 0, ptsList, outCap);
    // Frame2 pts=1000 (valid): sets bufferGroupPtsDistance = 1000, bufferNum = 1.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, 1000, ptsList, outCap);
    // Frame3..5 pts=AV_NOPTS_VALUE: interpolate 1000 + 1000 * (idx-1) / 1 -> 2000 / 3000 / 4000.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);

    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, SendAacEos(inBuf));
    DrainAacOutput(ptsList, outCap);

    EXPECT_FALSE(ptsList.empty());
    // Every output pts must be a sane non-negative value within int64 range.
    for (int64_t pts : ptsList) {
        EXPECT_GE(pts, 0);
        EXPECT_LE(pts, INT64_MAX);
    }
    // Interpolation produces evenly spaced pts; normalize by the minimum to stay robust against any
    // constant pts offset the decoder may apply. 1000 + 1000 * 1 / 1 = 2000 must appear.
    int64_t base = ptsList[0];
    for (int64_t p : ptsList) {
        if (p < base) {
            base = p;
        }
    }
    std::set<int64_t> relPts;
    for (int64_t p : ptsList) {
        relPts.insert(p - base);
    }
    EXPECT_NE(relPts.find(2000), relPts.end());
}

/**
 * @tc.name: AacPtsOverflowClamp_001
 * @tc.desc: pts interpolation overflow guard; when distance * (bufferIndex - 1) overflows int64 or the
 *           final addition overflows, the result is clamped to INT64_MAX instead of wrapping.
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, AacPtsOverflowClamp_001, TestSize.Level1)
{
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitAacFile(INPUT_AAC_FILE));
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacDecPlugin_->Init(format_));

    constexpr uint32_t inCap = 16384;
    constexpr uint32_t outCap = 65536;
    auto inBuf = std::make_shared<AudioBufferInfo>(inCap, "aac_in");
    ASSERT_TRUE(inBuf != nullptr);

    std::vector<int64_t> ptsList;
    std::vector<uint8_t> data;
    // Frame1 pts=INT64_MAX (valid): AAC propagates packet pts to frame pts, so curBufferGroupPts_ and
    // bufferGroupPtsDistance both become INT64_MAX.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, INT64_MAX, ptsList, outCap);
    // Frame2 pts=AV_NOPTS_VALUE: distance * 1 fits, but cur + distance overflows -> clamp to INT64_MAX.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);
    // Frame3 pts=AV_NOPTS_VALUE: distance * 2 overflows int64 -> clamp to INT64_MAX (multiplication guard).
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);

    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, SendAacEos(inBuf));
    DrainAacOutput(ptsList, outCap);

    EXPECT_FALSE(ptsList.empty());
    // Without the overflow guard the interpolation would wrap to a negative/garbage value; with the guard
    // every overflowed pts is clamped to INT64_MAX and no value escapes the int64 range.
    for (int64_t pts : ptsList) {
        EXPECT_GE(pts, 0);
        EXPECT_LE(pts, INT64_MAX);
    }
    bool hasClamped = false;
    for (int64_t pts : ptsList) {
        if (pts == INT64_MAX) {
            hasClamped = true;
            break;
        }
    }
    EXPECT_TRUE(hasClamped);
}

/**
 * @tc.name: AacPtsSubtractionOverflow_001
 * @tc.desc: pts group distance subtraction overflow guard. When cur and pre have opposite-sign
 *           extremes, abs(cur - pre) invokes signed-overflow / abs(INT64_MIN) UB and, without the
 *           guard, makes the interpolated pts wrap to a large negative value. With the guard the
 *           distance is clamped to INT64_MAX and the interpolated pts is INT64_MAX.
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, AacPtsSubtractionOverflow_001, TestSize.Level1)
{
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitAacFile(INPUT_AAC_FILE));
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacDecPlugin_->Init(format_));

    constexpr uint32_t inCap = 16384;
    constexpr uint32_t outCap = 65536;
    auto inBuf = std::make_shared<AudioBufferInfo>(inCap, "aac_in");
    ASSERT_TRUE(inBuf != nullptr);

    // INT64_MIN is AV_NOPTS_VALUE; INT64_MIN + 1 is the most-negative *valid* pts.
    constexpr int64_t negExtremePts = INT64_MIN + 1;
    std::vector<int64_t> ptsList;
    std::vector<uint8_t> data;
    // Frame1 pts=negExtremePts (valid): curBufferGroupPts_ = negExtremePts, distance = INT64_MAX.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, negExtremePts, ptsList, outCap);
    // Frame2 pts=1 (valid): preBufferGroupPts_ = negExtremePts, curBufferGroupPts_ = 1.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, 1, ptsList, outCap);
    // Frame3..4 pts=AV_NOPTS_VALUE: abs(1 - (INT64_MIN+1)) overflows to INT64_MIN, abs() is UB and
    // would yield a negative distance -> a large negative interpolated pts. The guard clamps the
    // distance to INT64_MAX so the interpolated pts is INT64_MAX.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);

    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, SendAacEos(inBuf));
    DrainAacOutput(ptsList, outCap);

    EXPECT_FALSE(ptsList.empty());
    for (int64_t pts : ptsList) {
        EXPECT_LE(pts, INT64_MAX);
    }
    // The two AV_NOPTS_VALUE frames interpolate to INT64_MAX with the guard; without the guard they
    // wrap to INT64_MIN + 1 (negative), so this assertion fails if the subtraction guard is removed.
    bool hasClamped = false;
    for (int64_t pts : ptsList) {
        if (pts == INT64_MAX) {
            hasClamped = true;
            break;
        }
    }
    EXPECT_TRUE(hasClamped);
}

/**
 * @tc.name: AacPtsNegativeCur_001
 * @tc.desc: addition overflow guard must not fire for negative curBufferGroupPts_. When cur is negative
 *           cur + increment cannot overflow, so the guard's (INT64_MAX - cur) check (which itself
 *           overflows for negative cur) must be skipped, otherwise the interpolated pts is falsely
 *           clamped to INT64_MAX instead of the correct small value.
 * @tc.type: FUNC
 */
HWTEST_F(AudioDecPluginUnitTest, AacPtsNegativeCur_001, TestSize.Level1)
{
    aacDecPlugin_ = std::make_shared<AudioFFMpegAacDecoderPlugin>();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 2);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 44100);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, DEFAULT_AAC_TYPE);
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, InitAacFile(INPUT_AAC_FILE));
    ASSERT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, aacDecPlugin_->Init(format_));

    constexpr uint32_t inCap = 16384;
    constexpr uint32_t outCap = 65536;
    auto inBuf = std::make_shared<AudioBufferInfo>(inCap, "aac_in");
    ASSERT_TRUE(inBuf != nullptr);

    constexpr int64_t negPts = -1000; // valid negative pts
    std::vector<int64_t> ptsList;
    std::vector<uint8_t> data;
    // Frame1 pts=negPts (valid): curBufferGroupPts_ = -1000, distance = 1000.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, negPts, ptsList, outCap);
    // Frame2..4 pts=AV_NOPTS_VALUE: interpolate -1000 + 1000 * (idx-1) -> 0 / 1000 / 2000.
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);
    ASSERT_TRUE(ReadAacFrameData(data));
    SendAndDrain(inBuf, data, AV_NOPTS_VALUE, ptsList, outCap);

    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, SendAacEos(inBuf));
    DrainAacOutput(ptsList, outCap);

    EXPECT_FALSE(ptsList.empty());
    // With the cur>0 guard the interpolated pts are small (relative 1000 step); without it the
    // (INT64_MAX - cur) check overflows for negative cur and falsely clamps them to INT64_MAX.
    for (int64_t pts : ptsList) {
        EXPECT_NE(pts, INT64_MAX);
    }
    int64_t base = ptsList[0];
    for (int64_t p : ptsList) {
        if (p < base) {
            base = p;
        }
    }
    std::set<int64_t> relPts;
    for (int64_t p : ptsList) {
        relPts.insert(p - base);
    }
    EXPECT_NE(relPts.find(1000), relPts.end());
}
} // namespace MediaAVCodec
} // namespace OHOS