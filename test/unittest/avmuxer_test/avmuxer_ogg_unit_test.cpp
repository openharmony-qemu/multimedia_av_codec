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

#include <gtest/gtest.h>
#include <string>
#include <fstream>
#include <iterator>
#include <vector>
#include <fcntl.h>
#include "native_avbuffer.h"
#include "native_audio_channel_layout.h"
#include "native_avmuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avcodec_base.h"
#include "avcodec_common.h"
#include "native_avcodec_audiocodec.h"


using namespace testing::ext;
using namespace std;
namespace {
const string TEST_FILE_PATH = "/data/test/media/";
const string INPUT_8000 = "/data/test/media/g711mu_8kHz_10s.pcm";
const string INPUT_48000 = "/data/test/media/flac_2c_44100hz_261k.pcm";
const string OPUS_FILE_PATH = "/data/test/media/opus_16000_1.dat";
const string VORBIS_FILE_PATH = "/data/test/media/vorbis_16000_2.dat";
const string OPUS_SO_FILE_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_ext_base.z.so";
constexpr int32_t OPUS_FRAME_SAMPLE_SIZES = 960 * 2 * 2; // 48k, 20ms, 2ch, s16le

// Parses the ogg physical bitstream page-by-page starting from offset 0 (following
// each page's segment table to the next page boundary, not by scanning for "OggS"
// which can appear inside packet data) and returns the header_type_flag byte
// (page offset 5) of the last page. Returns -1 if no page was parsed.
int32_t GetLastOggPageFlag(const string &path)
{
    std::ifstream f(path, std::ios::binary);
    if (!f.is_open()) {
        return -1;
    }
    std::vector<uint8_t> data((std::istreambuf_iterator<char>(f)), std::istreambuf_iterator<char>());
    int64_t size = static_cast<int64_t>(data.size());
    int64_t off = 0;
    int32_t lastFlag = -1;
    while (off + 27 <= size) { // 27: fixed ogg page header size
        if (data[off] != 'O' || data[off + 1] != 'g' || data[off + 2] != 'g' || data[off + 3] != 'S') {  // 2 3
            break;
        }
        int32_t nseg = data[off + 26]; // 26: page_segments offset
        if (off + 27 + nseg > size) {  // 27
            break;
        }
        lastFlag = data[off + 5]; // 5: header_type_flag offset (bit 0x04 = EOS)
        int64_t body = 0;
        for (int32_t i = 0; i < nseg; i++) {
            body += data[off + 27 + i]; // 27: segment table start
        }
        off += 27 + nseg + body;  // 27
    }
    return lastFlag;
}

class AVMuxerOggUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    bool WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file);

    // run with encoder
    bool CheckOpusSoFunc(void);
    void EncoderRun(void);
    void EncoderConfig(int32_t sampleRate, int32_t channel);

protected:
    OH_AVMuxer *muxer_ {nullptr};
    std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    int32_t fd_ {-1};

    // run with encoder
    int32_t trackId_ = -1;
    OH_AVCodec *codec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    int32_t channels_ = 2;
    int32_t sampleRate_ = 48000;
    uint32_t inputFrameBytes_ = 8192;
    int64_t inTimeout_ = 20000; // 20000us: 20ms
    int64_t outTimeout_ = 20000; // 20000us: 20ms
};

void AVMuxerOggUnitTest::SetUpTestCase() {}

void AVMuxerOggUnitTest::TearDownTestCase() {}

void AVMuxerOggUnitTest::SetUp()
{
    format_ = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
    inputFrameBytes_ = OPUS_FRAME_SAMPLE_SIZES;
}

void AVMuxerOggUnitTest::TearDown()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }
    if (inputFile_) {
        if (inputFile_->is_open()) {
            inputFile_->close();
        }
    }
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
    if (muxer_) {
        ASSERT_EQ(OH_AVMuxer_Destroy(muxer_), AV_ERR_OK);
        muxer_ = nullptr;
    }
}

bool AVMuxerOggUnitTest::WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file)
{
    OH_AVCodecBufferAttr info;

    if (file->eof()) {
        return false;
    }
    file->read(reinterpret_cast<char *>(&info.pts), sizeof(info.pts));
    if (file->gcount() < sizeof(info.pts) || file->eof()) {
        return false;
    }

    file->read(reinterpret_cast<char *>(&info.flags), sizeof(info.flags));
    if (file->gcount() < sizeof(info.flags) || file->eof()) {
        return false;
    }

    file->read(reinterpret_cast<char *>(&info.size), sizeof(info.size));
    if (file->gcount() < sizeof(info.size) || file->eof() || info.size <= 0) {
        // size 0 maybe eos
        return false;
    }

    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    file->read(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), info.size);
    if (file->gcount() < info.size) {
        return false;
    }
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    OH_AVMuxer_WriteSampleBuffer(muxer_, trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    return true;
}

bool AVMuxerOggUnitTest::CheckOpusSoFunc()
{
    std::unique_ptr<std::ifstream> soFile = std::make_unique<std::ifstream>(OPUS_SO_FILE_PATH, std::ios::binary);
    if (!soFile->is_open()) {
        cout << "Fatal: Open opus so file failed" << endl;
        return false;
    }
    soFile->close();
    return true;
}

void AVMuxerOggUnitTest::EncoderConfig(int32_t sampleRate, int32_t channel)
{
    OH_AVFormat_SetLongValue(format_, OH_MD_KEY_BITRATE, 150000);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_SAMPLE_RATE, sampleRate);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_COMPLIANCE_LEVEL, 10);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, OH_BitsPerSample::SAMPLE_S16LE);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_AUD_CHANNEL_COUNT, channel);

    codec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS, true);
    ASSERT_NE(codec_, nullptr);
    ASSERT_EQ(OH_AudioCodec_Configure(codec_, format_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Prepare(codec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(codec_), AV_ERR_OK);
    OH_AVFormat *audioFormat = OH_AudioCodec_GetOutputDescription(codec_);
    ASSERT_NE(audioFormat, nullptr);
    ASSERT_EQ(OH_AVMuxer_AddTrack(muxer_, &trackId_, audioFormat), AV_ERR_OK);
    ASSERT_EQ(OH_AVMuxer_Start(muxer_), AV_ERR_OK);
}

void AVMuxerOggUnitTest::EncoderRun()
{
    bool inputEnd = false;
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCodecBufferAttr attr;
    int64_t pts = 0;
    for (;;) {
        uint32_t index = 0;
        if (!inputEnd) {
            ret = OH_AudioCodec_QueryInputBuffer(codec_, &index, inTimeout_);
            if (ret == AV_ERR_TRY_AGAIN_LATER) {
                continue;
            }
            ASSERT_EQ(ret, AV_ERR_OK);
            OH_AVBuffer *inputBuf = OH_AudioCodec_GetInputBuffer(codec_, index);
            ASSERT_NE(inputBuf, nullptr);
            memset_s(&attr, sizeof(attr), 0, sizeof(attr));
            attr.pts = pts;
            if (!inputFile_->eof()) {
                inputFile_->read(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(inputBuf)), inputFrameBytes_);
                int32_t readSize = inputFile_->gcount();
                attr.size = readSize;
                attr.flags = readSize != 0 ? AVCODEC_BUFFER_FLAGS_NONE : AVCODEC_BUFFER_FLAGS_EOS;
            } else {
                inputEnd = true;
                attr.size = 0;
                attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
            }
            pts += 20000; // 20ms per frame
            EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, &attr), AV_ERR_OK);
            EXPECT_EQ(OH_AudioCodec_PushInputBuffer(codec_, index), AV_ERR_OK);
        }
        ret = OH_AudioCodec_QueryOutputBuffer(codec_, &index, outTimeout_);
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            continue;
        }
        EXPECT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *outputBuf = OH_AudioCodec_GetOutputBuffer(codec_, index);
        ASSERT_NE(outputBuf, nullptr);
        ASSERT_EQ(OH_AVBuffer_GetBufferAttr(outputBuf, &attr), AV_ERR_OK);
        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        OH_AVMuxer_WriteSampleBuffer(muxer_, trackId_, outputBuf);
        ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(codec_, index), AV_ERR_OK);
    }
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_Create_001, TestSize.Level0)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_Create_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    OH_AVFormat *audioFormat = OH_AVFormat_Create();
    ASSERT_NE(audioFormat, nullptr);
    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    inputFile_ = std::make_shared<std::ifstream>(OPUS_FILE_PATH, std::ios::binary);
    // read codec config
    uint32_t codecCfgLen = 0;
    inputFile_->read(reinterpret_cast<char *>(&codecCfgLen), sizeof(codecCfgLen));
    ASSERT_EQ(inputFile_->eof(), false);
    if (codecCfgLen > 0) {
        vector<uint8_t> codecConfig(codecCfgLen);
        inputFile_->read(reinterpret_cast<char *>(codecConfig.data()), codecCfgLen);
        ASSERT_EQ(inputFile_->eof(), false);
        OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, codecConfig.data(), codecCfgLen);
    }
    int32_t trackId = -1;
    ASSERT_EQ(OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat), AV_ERR_OK);
    ASSERT_NE(trackId, -1);
    OH_AVFormat_Destroy(audioFormat);
    ASSERT_EQ(OH_AVMuxer_Start(muxer_), AV_ERR_OK);

    while (WriteSample(trackId, inputFile_)) {
        // CONTINUE
    }
    ASSERT_EQ(OH_AVMuxer_Stop(muxer_), AV_ERR_OK);
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_Create_002, TestSize.Level0)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_Create_002.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);
    OH_AVFormat *fileFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(fileFormat, "com.openharmony.testString", "string test");
    // not support
    OH_AVFormat_SetIntValue(fileFormat, "com.openharmony.testInt", 1024);
    ASSERT_EQ(OH_AVMuxer_SetFormat(muxer_, fileFormat), AV_ERR_OK);

    OH_AVFormat *audioFormat = OH_AVFormat_Create();
    ASSERT_NE(audioFormat, nullptr);
    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_VORBIS);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 2);
    inputFile_ = std::make_shared<std::ifstream>(VORBIS_FILE_PATH, std::ios::binary);
    // read codec config
    uint32_t codecCfgLen = 0;
    inputFile_->read(reinterpret_cast<char *>(&codecCfgLen), sizeof(codecCfgLen));
    ASSERT_EQ(inputFile_->eof(), false);
    if (codecCfgLen > 0) {
        vector<uint8_t> codecConfig(codecCfgLen);
        inputFile_->read(reinterpret_cast<char *>(codecConfig.data()), codecCfgLen);
        ASSERT_EQ(inputFile_->eof(), false);
        OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, codecConfig.data(), codecCfgLen);
    }
    int32_t trackId = -1;
    ASSERT_EQ(OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat), AV_ERR_OK);
    ASSERT_NE(trackId, -1);
    OH_AVFormat_Destroy(audioFormat);
    ASSERT_EQ(OH_AVMuxer_Start(muxer_), AV_ERR_OK);

    while (WriteSample(trackId, inputFile_)) {
        // CONTINUE
    }
    ASSERT_EQ(OH_AVMuxer_Stop(muxer_), AV_ERR_OK);
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_WithOpusEncoder_001, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_WithOpusEncoder_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    if (CheckOpusSoFunc()) {
        inputFile_ = std::make_unique<std::ifstream>(INPUT_48000, std::ios::binary);
        ASSERT_EQ(inputFile_->is_open(), true);
        EncoderConfig(48000, 2);
        EncoderRun();
        ASSERT_EQ(OH_AVMuxer_Stop(muxer_), AV_ERR_OK);
    }
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_WithOpusEncoder_002, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_WithOpusEncoder_002.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    if (CheckOpusSoFunc()) {
        inputFrameBytes_ = 80 * 2 * 2; // 20ms
        inputFile_ = std::make_unique<std::ifstream>(INPUT_8000, std::ios::binary);
        ASSERT_EQ(inputFile_->is_open(), true);
        EncoderConfig(8000, 1);
        EncoderRun();
        ASSERT_EQ(OH_AVMuxer_Stop(muxer_), AV_ERR_OK);
    }
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_InvalidSampleRate_001, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_InvalidSampleRate_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    OH_AVFormat *audioFormat = OH_AVFormat_Create();
    ASSERT_NE(audioFormat, nullptr);
    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 0);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);

    int32_t trackId = -1;
    int32_t ret = OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat);
    EXPECT_NE(ret, AV_ERR_OK);

    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, -1);
    ret = OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat);
    EXPECT_NE(ret, AV_ERR_OK);

    OH_AVFormat_Destroy(audioFormat);
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_InvalidChannelCount_001, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_InvalidChannelCount_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    OH_AVFormat *audioFormat = OH_AVFormat_Create();
    ASSERT_NE(audioFormat, nullptr);
    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 0);

    int32_t trackId = -1;
    int32_t ret = OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat);
    EXPECT_NE(ret, AV_ERR_OK);

    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 99);
    ret = OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat);
    EXPECT_NE(ret, AV_ERR_OK);

    OH_AVFormat_Destroy(audioFormat);
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_UnsupportedMimeType_001, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_UnsupportedMimeType_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    OH_AVFormat *audioFormat = OH_AVFormat_Create();
    ASSERT_NE(audioFormat, nullptr);
    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, "audio/mpeg");
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 16000);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);

    int32_t trackId = -1;
    int32_t ret = OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat);
    EXPECT_NE(ret, AV_ERR_OK);

    OH_AVFormat_Destroy(audioFormat);
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_StartBeforeAddTrack_001, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_StartBeforeAddTrack_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    int32_t ret = OH_AVMuxer_Start(muxer_);
    EXPECT_NE(ret, AV_ERR_OK);
}

HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_Destroy_Lifecycle_001, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_Destroy_Lifecycle_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    OH_AVMuxer *tmpMuxer = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(tmpMuxer, nullptr);

    int32_t ret = OH_AVMuxer_Destroy(tmpMuxer);
    ASSERT_EQ(ret, AV_ERR_OK);
}

// Regression for the ogg muxer not writing the EOS page. The muxer's Stop() used to
// call av_write_frame(NULL) before av_write_trailer; for the ogg muxer that NULL flush
// routes through ffmpeg's ogg_write_pages(flush=2), which emits every buffered page
// (including the last one) WITHOUT the EOS flag, leaving av_write_trailer nothing to
// flush -- so the file ended with header_type_flag 0x00 and no EOS page anywhere.
// After the fix the last page must carry EOS (0x04).
HWTEST_F(AVMuxerOggUnitTest, OGG_Muxer_LastPageHasEos_001, TestSize.Level1)
{
    string fileName = TEST_FILE_PATH + string("OGG_Muxer_LastPageHasEos_001.ogg");
    int32_t fd = open(fileName.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    ASSERT_GE(fd, 0);
    muxer_ = OH_AVMuxer_Create(fd, AV_OUTPUT_FORMAT_OGG);
    ASSERT_NE(muxer_, nullptr);

    OH_AVFormat *audioFormat = OH_AVFormat_Create();
    ASSERT_NE(audioFormat, nullptr);
    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_OPUS);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 16000); // 16000: 16 kHz opus
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    inputFile_ = std::make_shared<std::ifstream>(OPUS_FILE_PATH, std::ios::binary);
    // read codec config baked into the .dat
    uint32_t codecCfgLen = 0;
    inputFile_->read(reinterpret_cast<char *>(&codecCfgLen), sizeof(codecCfgLen));
    ASSERT_EQ(inputFile_->eof(), false);
    if (codecCfgLen > 0) {
        vector<uint8_t> codecConfig(codecCfgLen);
        inputFile_->read(reinterpret_cast<char *>(codecConfig.data()), codecCfgLen);
        ASSERT_EQ(inputFile_->eof(), false);
        OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, codecConfig.data(), codecCfgLen);
    }
    int32_t trackId = -1;
    ASSERT_EQ(OH_AVMuxer_AddTrack(muxer_, &trackId, audioFormat), AV_ERR_OK);
    ASSERT_NE(trackId, -1);
    OH_AVFormat_Destroy(audioFormat);
    ASSERT_EQ(OH_AVMuxer_Start(muxer_), AV_ERR_OK);

    while (WriteSample(trackId, inputFile_)) {
        // CONTINUE
    }
    ASSERT_EQ(OH_AVMuxer_Stop(muxer_), AV_ERR_OK);

    // The muxer flushed everything via avio_flush inside Stop(), so the on-disk file
    // already contains the final (EOS) page. The last page's header_type_flag must have
    // bit 0x04 set.
    int32_t lastFlag = GetLastOggPageFlag(fileName);
    ASSERT_GE(lastFlag, 0) << "failed to parse any ogg page from " << fileName;
    EXPECT_TRUE(lastFlag & 0x04) << "last ogg page should carry EOS (0x04), got 0x" << lastFlag;
}

} // namespace