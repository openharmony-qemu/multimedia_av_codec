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
#include <vector>
#include <fcntl.h>
#include <fstream>
#include "avmuxer_sample.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue_producer.h"
#include "nocopyable.h"
#include "avmuxer.h"
#include "native_avbuffer.h"
#include "avcodec_audio_channel_layout.h"
#ifdef AVMUXER_UNITTEST_CAPI
#include "native_avmuxer.h"
#include "native_avformat.h"
#endif

using namespace testing::ext;
using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
const std::string TEST_FILE_PATH = "/data/test/media/";

class AVMuxerFlacUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    int32_t WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file, bool &eosFlag, uint32_t flag);

protected:
    std::shared_ptr<AVMuxerSample> avmuxer_ {nullptr};
    std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    int32_t fd_ {-1};
};

void AVMuxerFlacUnitTest::SetUpTestCase()
{}

void AVMuxerFlacUnitTest::TearDownTestCase()
{}

void AVMuxerFlacUnitTest::SetUp()
{
    avmuxer_ = std::make_shared<AVMuxerSample>();
}

void AVMuxerFlacUnitTest::TearDown()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }

    if (inputFile_ != nullptr) {
        if (inputFile_->is_open()) {
            inputFile_->close();
        }
    }

    if (avmuxer_ != nullptr) {
        avmuxer_->Destroy();
        avmuxer_ = nullptr;
    }
}

int32_t AVMuxerFlacUnitTest::WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file,
                                         bool &eosFlag, uint32_t flag)
{
    OH_AVCodecBufferAttr info;

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    file->read(reinterpret_cast<char*>(&info.pts), sizeof(info.pts));

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    file->read(reinterpret_cast<char*>(&info.flags), sizeof(info.flags));

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }
    file->read(reinterpret_cast<char*>(&info.size), sizeof(info.size));

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    OH_AVBuffer *buffer = nullptr;
    if (info.size == 0) {
        buffer = OH_AVBuffer_Create(1);
    } else {
        buffer = OH_AVBuffer_Create(info.size);
        file->read(reinterpret_cast<char*>(OH_AVBuffer_GetAddr(buffer)), info.size);
    }
    
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    int32_t ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    return ret;
}

/**
 * @tc.name: Muxer_FLAC_001
 * @tc.desc: supported muxer flac
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlacUnitTest, Muxer_FLAC_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLAC_48000_1.flac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_FLAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 48000); // 48000 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>("/data/test/media/flac_48000_1_s16le.dat", std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    ASSERT_EQ(extSize, 34);  // 34 -> flac codecConfig len
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
        audioParams->PutBuffer(OH_MD_KEY_CODEC_CONFIG, buffer.data(), buffer.size());
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_FLAC_002
 * @tc.desc: supported muxer flac with no codec config
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlacUnitTest, Muxer_FLAC_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLAC_48000_1.flac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_FLAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 48000); // 48000 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>("/data/test/media/flac_48000_1_s16le.dat", std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    ASSERT_EQ(extSize, 34);  // 34 -> flac codecConfig len
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: AddTrack_001
 * @tc.desc: test muxer flac add track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlacUnitTest, AddTrack_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLAC_48000_1.flac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_FLAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 48000); // 48000 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 9); // 9 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 0); // 9 channels
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 2); // 2 channels
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: AddTrack_002
 * @tc.desc: test muxer flac add track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlacUnitTest, AddTrack_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLAC_48000_1.flac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_FLAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, -1); // -1 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 0); // 48000 sample rate
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 96000); // 96000 sample rate
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_EQ(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 192000); // 192000 sample rate
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_EQ(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 384000); // 384000 sample rate
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_EQ(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 768000); // 768000 sample rate
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name: AddTrack_003
 * @tc.desc: test muxer flac add track
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerFlacUnitTest, AddTrack_003, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_FLAC_48000_1.flac");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_FLAC;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_FLAC);
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, 48000); // 48000 sample rate
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1); // 1 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, static_cast<int32_t>(0xff));
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, 0);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, static_cast<int32_t>(-1));
    ret = avmuxer_->AddTrack(trackId, audioParams);
    EXPECT_NE(ret, 0);
}
} // namespace MediaAVCodec
} // namespace OHOS