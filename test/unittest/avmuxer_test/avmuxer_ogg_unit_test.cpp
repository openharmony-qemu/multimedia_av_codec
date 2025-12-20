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
#include <vector>
#include <fcntl.h>
#include "native_avbuffer.h"
#include "native_audio_channel_layout.h"
#include "native_avmuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"


using namespace testing::ext;
using namespace std;
namespace {
const string TEST_FILE_PATH = "/data/test/media/";
const string OPUS_FILE_PATH = "/data/test/media/opus_16000_1.dat";
const string VORBIS_FILE_PATH = "/data/test/media/vorbis_16000_2.dat";

class AVMuxerOggUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    bool WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file);
protected:
    OH_AVMuxer *muxer_ {nullptr};
    std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    int32_t fd_ {-1};
};

void AVMuxerOggUnitTest::SetUpTestCase() {}

void AVMuxerOggUnitTest::TearDownTestCase() {}

void AVMuxerOggUnitTest::SetUp()
{
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

} // namespace