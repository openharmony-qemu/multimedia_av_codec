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

#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include <fstream>
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_audio_common.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "mpeg4_source_unit_test.h"
#include "media_data_source.h"
#include "native_avsource.h"
#include "native_avcodec_base.h"
#include "syspara/parameters.h"

#define LOCAL true
#define URI false

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const string TEST_TIMED_METADATA = "com.openharmony.timed_metadata.test";
const int64_t SOURCE_OFFSET = 0;
string g_mp4CameraPath = TEST_FILE_PATH + string("camera.mp4");
string g_multiTrackPath = TEST_FILE_PATH + string("multi_trk.mov");
string g_fmp4SidxPath = TEST_FILE_PATH + string("sidx.mp4");
string g_mp4FlipPath = TEST_FILE_PATH + string("FLIP_V_90.mp4");
string g_mp4TimedMetadataPath = TEST_FILE_PATH + string("timed_metadata_track.mp4");
string g_mp4AudiovividPath = TEST_FILE_PATH + string("audiovivid_hdrvivid.mp4");
string g_mp4G711muPath = TEST_FILE_PATH + string("mpeg4_g711mu.mov");
string g_mp4MPEG4Path = TEST_FILE_PATH + string("mpeg4_aac.mp4");
string g_mp4MPEG4Path2 = TEST_FILE_PATH + string("mpeg4_flac.mp4");
string g_mp4MPEG4Path3 = TEST_FILE_PATH + string("mpeg4_mp3.mp4");
string g_mp4MPEG4Path4 = TEST_FILE_PATH + string("mpeg4_opus.mp4");
string g_mp4MPEG4Path5 = TEST_FILE_PATH + string("mpeg4_amr_nb.3gp");
string g_mp4MPEG4Path6 = TEST_FILE_PATH + string("mpeg4_flac_ch7.1.mp4");
string g_mp4MPEG4Path7 = TEST_FILE_PATH + string("mpeg4_pcm_s32be_ch7.1.mov");
string g_mp4MPEG4Path8 = TEST_FILE_PATH + string("mpeg4_aac_ch7.1wide.mp4");
string g_mp4MPEG4Path9 = TEST_FILE_PATH + string("mpeg4_aac_ch4.0_eld.mp4");
string g_mp4MPEG4Path10 = TEST_FILE_PATH + string("mpeg4_f32le.mov");
string g_mp4MPEG4Path11 = TEST_FILE_PATH + string("mpeg4_f32le_lpcm.mov");
string g_mp4MPEG4Path12 = TEST_FILE_PATH + string("mpeg4_f64be.mov");
string g_mp4MPEG4Path13 = TEST_FILE_PATH + string("mpeg4_s16be_twos.mov");
string g_mp4MPEG4Path14 = TEST_FILE_PATH + string("mpeg4_s16le_lpcm.mov");
string g_mp4MPEG4Path15 = TEST_FILE_PATH + string("mpeg4_s24le.mov");
string g_mp4MPEG4Path16 = TEST_FILE_PATH + string("mpeg4_s24le_lpcm.mov");
string g_mp4MPEG4Path17 = TEST_FILE_PATH + string("mpeg4_s32le.mov");
string g_mp4MPEG4Path18 = TEST_FILE_PATH + string("mpeg4_s32le_lpcm.mov");
string g_mp4MPEG4Path19 = TEST_FILE_PATH + string("mpeg4_u8.mov");
string g_mp4H263Path = TEST_FILE_PATH + string("h263_amr_wb.3gp");
string g_mp4MPEG2Path = TEST_FILE_PATH + string("mpeg2_aac_test.mp4");
string g_mp4MPEG2Path2 = TEST_FILE_PATH + string("mpeg2_alac.mp4");
string g_mp4MPEG2Path3 = TEST_FILE_PATH + string("mpeg2_dts.mp4");
string g_mp4MPEG2Path4 = TEST_FILE_PATH + string("mpeg2_aac.mov");
} // namespace

void Mpeg4SourceUnitTest::SetUpTestCase(void)
{
    OHOS::system::SetParameter("Mpeg4DemuxerPlugin.enable", "1");
    server = make_unique<FileServerDemo>();
    server->StartServer();
}

void Mpeg4SourceUnitTest::TearDownTestCase(void)
{
    server->StopServer();
    OHOS::system::SetParameter("Mpeg4DemuxerPlugin.enable", "0");
}

void Mpeg4SourceUnitTest::SetUp(void) {}

void Mpeg4SourceUnitTest::TearDown(void)
{
    if (source_ != nullptr) {
        source_->Destroy();
        source_ = nullptr;
    }
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    if (format_ != nullptr) {
        format_->Destroy();
        format_ = nullptr;
    }
    trackIndex_ = 0;
    size_ = 0;
    addr_ = nullptr;
    buffSize_ = 0;
    initStatus_ = false;
    ResetFormatValue();
}

int64_t Mpeg4SourceUnitTest::GetFileSize(const string &fileName)
{
    int64_t fileSize = 0;
    if (!fileName.empty()) {
        struct stat fileStatus {};
        if (stat(fileName.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

int32_t Mpeg4SourceUnitTest::OpenFile(const string &fileName)
{
    int32_t fd = open(fileName.c_str(), O_RDONLY);
    return fd;
}

void Mpeg4SourceUnitTest::ResetFileFormatValue()
{
    formatVal_.title = "";
    formatVal_.artist = "";
    formatVal_.album = "";
    formatVal_.albumArtist = "";
    formatVal_.date = "";
    formatVal_.comment = "";
    formatVal_.genre = "";
    formatVal_.copyright = "";
    formatVal_.description = "";
    formatVal_.language = "";
    formatVal_.lyrics = "";
    formatVal_.duration = 0;
    formatVal_.trackCount = 0;
    formatVal_.author = "";
    formatVal_.composer = "";
    formatVal_.hasVideo = -1;
    formatVal_.hasAudio = -1;
    formatVal_.fileType = 0;
}

void Mpeg4SourceUnitTest::ResetFormatValue()
{
    ResetFileFormatValue();
    formatVal_.timeScale = 0;
    formatVal_.codecMime = "";
    formatVal_.trackType = 0;
    formatVal_.width = 0;
    formatVal_.height = 0;
    formatVal_.aacIsAdts = -1;
    formatVal_.sampleRate = 0;
    formatVal_.channelCount = 0;
    formatVal_.bitRate = 0;
    formatVal_.startTime = 0;
    formatVal_.audioSampleFormat = 0;
    formatVal_.frameRate = 0;
    formatVal_.videoSar = 0;
    formatVal_.longitude = 0;
    formatVal_.latitude = 0;
    formatVal_.rotationAngle = 0;
    formatVal_.orientationType = 0;
    formatVal_.channelLayout = 0;
    formatVal_.hdrType = 0;
    formatVal_.codecProfile = 0;
    formatVal_.codecLevel = 0;
    formatVal_.colorPrimaries = 0;
    formatVal_.transferCharacteristics = 0;
    formatVal_.rangeFlag = 0;
    formatVal_.matrixCoefficients = 0;
    formatVal_.chromaLocation = 0;
    formatVal_.profile = 0;
    formatVal_.level = 0;
    formatVal_.colorPri = 0;
    formatVal_.colorTrans = 0;
    formatVal_.colorMatrix = 0;
    formatVal_.colorRange = 0;
    formatVal_.chromaLoc = 0;
    formatVal_.isHdrVivid = 0;
}

/**********************************source FD**************************************/
namespace {
/**
 * @tc.name: mp4Source_GetFormat_0001
 * @tc.desc: get user data
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0001, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4CameraPath);
    size_ = GetFileSize(g_mp4CameraPath);
    printf("---- %s ------\n", g_mp4CameraPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[source Format]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3367000);
#ifdef MPEG4_SOURCE_INNER_UNIT_TEST
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_TIMEDMETA, formatVal_.hasTimedMeta));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.hasTimedMeta, 1);
#endif
    format_ = source_->GetUserData();
    ASSERT_NE(format_, nullptr);
    printf("[User Meta]: %s\n", format_->DumpInfo());
    string isWaterMark = "0";
    EXPECT_TRUE(format_->GetStringValue("com.openharmony.isWaterMark", isWaterMark));
    EXPECT_EQ(isWaterMark, "false"); // test user string data watermark
    string flag = "";
    EXPECT_TRUE(format_->GetStringValue("com.openharmony.deferredVideoEnhanceFlag", flag));
    EXPECT_EQ(flag, "0"); // test user int flag

    string timeStamp = "0";
    EXPECT_TRUE(format_->GetStringValue("com.openharmony.recorder.timestamp", timeStamp));
    EXPECT_EQ(timeStamp, "11236330712,11239663416"); // test user string data timestamp
}

/**
 * @tc.name: mp4Source_GetFormat_0002
 * @tc.desc: get format when the file is mov
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0002, TestSize.Level1)
{
    fd_ = OpenFile(g_multiTrackPath);
    size_ = GetFileSize(g_multiTrackPath);
    printf("---- %s ----\n", g_multiTrackPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_EQ(formatVal_.duration, 2000000);
    EXPECT_EQ(formatVal_.timeScale, 1000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/hevc");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.frameRate, 30.000000);
    EXPECT_EQ(formatVal_.bitRate, 304504);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.width, 352);
    EXPECT_EQ(formatVal_.height, 288);
    EXPECT_EQ(formatVal_.bitRate, 544636);
}

/**
 * @tc.name: mp4Source_GetFormat_0003
 * @tc.desc: get format when the file is mov
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0003, TestSize.Level1)
{
    fd_ = OpenFile(g_multiTrackPath);
    size_ = GetFileSize(g_multiTrackPath);
    printf("---- %s ----\n", g_multiTrackPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_EQ(formatVal_.duration, 2000000);
    EXPECT_EQ(formatVal_.trackCount, 4);
    EXPECT_EQ(formatVal_.timeScale, 1000);
    trackIndex_ = 2;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 125574);
    trackIndex_ = 3;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.sampleRate, 32000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 48412);
}

/**
 * @tc.name: mp4Source_GetFormat_0004
 * @tc.desc: get format when the file is fmp4 and has sidx box
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0004, TestSize.Level1)
{
    fd_ = OpenFile(g_fmp4SidxPath);
    size_ = GetFileSize(g_fmp4SidxPath);
    printf("---- %s ----\n", g_fmp4SidxPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 10100000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_TRACK_START_TIME, formatVal_.startTime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.startTime, 33333);
    EXPECT_EQ(formatVal_.bitRate, 1076608);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 123423);
}

/**
 * @tc.name: mp4Source_GetFormat_0005
 * @tc.desc: get displaymatrix
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0005, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4FlipPath);
    size_ = GetFileSize(g_mp4FlipPath);
    printf("---- %s ----\n", g_mp4FlipPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_ORIENTATION_TYPE, formatVal_.orientationType));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_ROTATION, formatVal_.rotationAngle));
    EXPECT_EQ(formatVal_.orientationType, 7);
    EXPECT_EQ(formatVal_.rotationAngle, 270);
}

/**
 * @tc.name: mp4Source_GetFormat_0006
 * @tc.desc: get format when the file is timed metadata file
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0006, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4TimedMetadataPath);
    size_ = GetFileSize(g_mp4TimedMetadataPath);
    printf("---- %s ----\n", g_mp4TimedMetadataPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.trackCount, 2);
#ifdef MPEG4_SOURCE_INNER_UNIT_TEST
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_TIMEDMETA, formatVal_.hasTimedMeta));
    EXPECT_EQ(formatVal_.fileType, 101);
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 0);
    EXPECT_EQ(formatVal_.hasTimedMeta, 1);
#endif
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_TIMED_METADATA_KEY, formatVal_.timedMetadataKey));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TIMED_METADATA_SRC_TRACK_ID, formatVal_.srcTrackID));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_TIMED_METADATA);
    EXPECT_EQ(formatVal_.codecMime, "meta/timed-metadata");
    EXPECT_EQ(formatVal_.timedMetadataKey, TEST_TIMED_METADATA.c_str());
    EXPECT_EQ(formatVal_.srcTrackID, 0);
}

/**
 * @tc.name: mp4Source_GetFormat_0007
 * @tc.desc: get format when the file is audiovivid
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0007, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4AudiovividPath);
    size_ = GetFileSize(g_mp4AudiovividPath);
    printf("---- %s ----\n", g_mp4AudiovividPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_EQ(formatVal_.duration, 2699349);
    EXPECT_EQ(formatVal_.trackCount, 2);
    EXPECT_EQ(formatVal_.timeScale, 1000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/hevc");
    EXPECT_EQ(formatVal_.width, 1280);
    EXPECT_EQ(formatVal_.height, 720);
    EXPECT_EQ(formatVal_.frameRate, 28.154937050793496);
    EXPECT_EQ(formatVal_.bitRate, 24913240);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/av3a");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 64083);
}

/**
 * @tc.name: mp4Source_GetFormat_0008
 * @tc.desc: get format when the file is MP4(mpeg4 + g711mu)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0008, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4G711muPath);
    size_ = GetFileSize(g_mp4G711muPath);
    printf("---- %s ----\n", g_mp4G711muPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 1500000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 3104213);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/g711mu");
    EXPECT_EQ(formatVal_.bitRate, 64000);
}

/**
 * @tc.name: mp4Source_GetFormat_0009
 * @tc.desc: get format when the file is MP4(mpeg4 + g711mu)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0009, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4G711muPath);
    size_ = GetFileSize(g_mp4G711muPath);
    printf("---- %s ----\n", g_mp4G711muPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 1500000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.frameRate, 30.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 8000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: mp4Source_GetFormat_0010
 * @tc.desc: get format when the file is MP4(mpeg4 + aac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0010, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path);
    size_ = GetFileSize(g_mp4MPEG4Path);
    printf("---- %s ----\n", g_mp4MPEG4Path.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 1243454);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.bitRate, 256189);
}

/**
 * @tc.name: mp4Source_GetFormat_0011
 * @tc.desc: get format when the file is MP4(mpeg4 + aac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0011, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path);
    size_ = GetFileSize(g_mp4MPEG4Path);
    printf("---- %s ----\n", g_mp4MPEG4Path.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 640);
    EXPECT_EQ(formatVal_.height, 352);
    EXPECT_EQ(formatVal_.frameRate, 28.787878787878789);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0012
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0012, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path2);
    size_ = GetFileSize(g_mp4MPEG4Path2);
    printf("---- %s ----\n", g_mp4MPEG4Path2.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/flac");
    EXPECT_EQ(formatVal_.bitRate, 1147385);
}

/**
 * @tc.name: mp4Source_GetFormat_0013
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0013, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path2);
    size_ = GetFileSize(g_mp4MPEG4Path2);
    printf("---- %s ----\n", g_mp4MPEG4Path2.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.width, 640);
    EXPECT_EQ(formatVal_.height, 352);
    EXPECT_EQ(formatVal_.bitRate, 1243454);
    EXPECT_EQ(formatVal_.frameRate, 28.787878787878789);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0014
 * @tc.desc: get format when the file is MP4(mpeg4 + mp3)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0014, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path3);
    size_ = GetFileSize(g_mp4MPEG4Path3);
    printf("---- %s ----\n", g_mp4MPEG4Path3.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 1243454);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.bitRate, 128142);
}

/**
 * @tc.name: mp4Source_GetFormat_0015
 * @tc.desc: get format when the file is MP4(mpeg4 + mp3)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0015, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path3);
    size_ = GetFileSize(g_mp4MPEG4Path3);
    printf("---- %s ----\n", g_mp4MPEG4Path3.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 640);
    EXPECT_EQ(formatVal_.height, 352);
    EXPECT_EQ(formatVal_.frameRate, 28.787878787878789);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0016
 * @tc.desc: get format when the file is MP4(mpeg2 + aac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0016, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG2Path);
    size_ = GetFileSize(g_mp4MPEG2Path);
    printf("---- %s ----\n", g_mp4MPEG2Path.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_TITLE, formatVal_.title));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_ARTIST, formatVal_.artist));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_ALBUM, formatVal_.album));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_ALBUM_ARTIST, formatVal_.albumArtist));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_DATE, formatVal_.date));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_COMMENT, formatVal_.comment));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_GENRE, formatVal_.genre));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_COPYRIGHT, formatVal_.copyright));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_LYRICS, formatVal_.lyrics));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_DESCRIPTION, formatVal_.description));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.title, "title");
    EXPECT_EQ(formatVal_.artist, "artist");
    EXPECT_EQ(formatVal_.album, "album");
    EXPECT_EQ(formatVal_.albumArtist, "album artist");
    EXPECT_EQ(formatVal_.date, "2023");
    EXPECT_EQ(formatVal_.comment, "comment");
    EXPECT_EQ(formatVal_.genre, "genre");
    EXPECT_EQ(formatVal_.copyright, "Copyright");
    EXPECT_EQ(formatVal_.lyrics, "lyrics");
    EXPECT_EQ(formatVal_.description, "description");
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 4120000);
    EXPECT_EQ(formatVal_.trackCount, 2);
#ifdef MPEG4_SOURCE_INNER_UNIT_TEST
    EXPECT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    EXPECT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.author, "author");
    EXPECT_EQ(formatVal_.composer, "composer");
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, 101);
#endif
}

/**
 * @tc.name: mp4Source_GetFormat_0017
 * @tc.desc: get format when the file is MP4(mpeg4 + opus)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0017, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path4);
    size_ = GetFileSize(g_mp4MPEG4Path4);
    printf("---- %s ----\n", g_mp4MPEG4Path4.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 1243454);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/opus");
    EXPECT_EQ(formatVal_.bitRate, 96757);
}

/**
 * @tc.name: mp4Source_GetFormat_0018
 * @tc.desc: get format when the file is MP4(mpeg4 + opus)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0018, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path4);
    size_ = GetFileSize(g_mp4MPEG4Path4);
    printf("---- %s ----\n", g_mp4MPEG4Path4.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 640);
    EXPECT_EQ(formatVal_.height, 352);
    EXPECT_EQ(formatVal_.frameRate, 28.787878787878789);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0019
 * @tc.desc: get format when the file is MP4(mpeg4 + amr_nb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0019, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path5);
    size_ = GetFileSize(g_mp4MPEG4Path5);
    printf("---- %s ----\n", g_mp4MPEG4Path5.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 1243454);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/3gpp");
    EXPECT_EQ(formatVal_.bitRate, 12832);
}

/**
 * @tc.name: mp4Source_GetFormat_0020
 * @tc.desc: get format when the file is MP4(mpeg4 + amr_nb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0020, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path5);
    size_ = GetFileSize(g_mp4MPEG4Path5);
    printf("---- %s ----\n", g_mp4MPEG4Path5.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 640);
    EXPECT_EQ(formatVal_.height, 352);
    EXPECT_EQ(formatVal_.frameRate, 28.787878787878789);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 8000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: mp4Source_GetFormat_0021
 * @tc.desc: get format when the file is MP4(h263 + amr_wb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0021, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4H263Path);
    size_ = GetFileSize(g_mp4H263Path);
    printf("---- %s ----\n", g_mp4H263Path.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/h263");
    EXPECT_EQ(formatVal_.bitRate, 508906);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/amr-wb");
    EXPECT_EQ(formatVal_.bitRate, 24509);
}

/**
 * @tc.name: mp4Source_GetFormat_0022
 * @tc.desc: get format when the file is MP4(h263 + amr_wb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0022, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4H263Path);
    size_ = GetFileSize(g_mp4H263Path);
    printf("---- %s ----\n", g_mp4H263Path.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 352);
    EXPECT_EQ(formatVal_.height, 288);
    EXPECT_EQ(formatVal_.frameRate, 30.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 16000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: mp4Source_GetFormat_0023
 * @tc.desc: get format when the file is MP4(mpeg2 + alac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0023, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG2Path2);
    size_ = GetFileSize(g_mp4MPEG2Path2);
    printf("---- %s ----\n", g_mp4MPEG2Path2.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.bitRate, 1344761);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.bitRate, 1222454);
}

/**
 * @tc.name: mp4Source_GetFormat_0024
 * @tc.desc: get format when the file is MP4(mpeg2 + alac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0024, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG2Path2);
    size_ = GetFileSize(g_mp4MPEG2Path2);
    printf("---- %s ----\n", g_mp4MPEG2Path2.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 640);
    EXPECT_EQ(formatVal_.height, 352);
    EXPECT_EQ(formatVal_.frameRate, 28.787878787878789);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0025
 * @tc.desc: get format when the file is MP4(mpeg2 + dts)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0025, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG2Path3);
    size_ = GetFileSize(g_mp4MPEG2Path3);
    printf("---- %s ----\n", g_mp4MPEG2Path3.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.bitRate, 1344761);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.bitRate, 1413017);
}

/**
 * @tc.name: mp4Source_GetFormat_0026
 * @tc.desc: get format when the file is MP4(mpeg2 + dts)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0026, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG2Path3);
    size_ = GetFileSize(g_mp4MPEG2Path3);
    printf("---- %s ----\n", g_mp4MPEG2Path3.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3300000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 640);
    EXPECT_EQ(formatVal_.height, 352);
    EXPECT_EQ(formatVal_.frameRate, 28.787878787878789);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0027
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0027, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path6);
    size_ = GetFileSize(g_mp4MPEG4Path6);
    printf("---- %s ----\n", g_mp4MPEG4Path6.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/flac");
    EXPECT_EQ(formatVal_.bitRate, 393832);
}

/**
 * @tc.name: mp4Source_GetFormat_0028
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0028, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path6);
    size_ = GetFileSize(g_mp4MPEG4Path6);
    printf("---- %s ----\n", g_mp4MPEG4Path6.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.bitRate, 981576);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 8);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 1599);
}

/**
 * @tc.name: mp4Source_GetFormat_0029
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0029, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path7);
    size_ = GetFileSize(g_mp4MPEG4Path7);
    printf("---- %s ----\n", g_mp4MPEG4Path7.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 981576);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.bitRate, 11289600);
}

/**
 * @tc.name: mp4Source_GetFormat_0030
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0030, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path7);
    size_ = GetFileSize(g_mp4MPEG4Path7);
    printf("---- %s ----\n", g_mp4MPEG4Path7.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 8);
    EXPECT_EQ(formatVal_.audioSampleFormat, 26);
    EXPECT_EQ(formatVal_.channelLayout, 1599);
}

/**
 * @tc.name: mp4Source_GetFormat_0031
 * @tc.desc: get format when the file is MP4(mpeg4 + aac-pce 7.1 wide)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0031, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path8);
    size_ = GetFileSize(g_mp4MPEG4Path8);
    printf("---- %s ----\n", g_mp4MPEG4Path8.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 981576);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.bitRate, 268264);
}

/**
 * @tc.name: mp4Source_GetFormat_0032
 * @tc.desc: get format when the file is MP4(mpeg4 + aac-pce 7.1 wide)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0032, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path8);
    size_ = GetFileSize(g_mp4MPEG4Path8);
    printf("---- %s ----\n", g_mp4MPEG4Path8.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 8);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 206158431759);
}

/**
 * @tc.name: mp4Source_GetFormat_0033
 * @tc.desc: get format when the file is MP4(mpeg4 + aac_eld 4.0)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0033, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path9);
    size_ = GetFileSize(g_mp4MPEG4Path9);
    printf("---- %s ----\n", g_mp4MPEG4Path9.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_TRACK_START_TIME, formatVal_.startTime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.bitRate, 981325);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.bitRate, 320851);
}

/**
 * @tc.name: mp4Source_GetFormat_0034
 * @tc.desc: get format when the file is MP4(mpeg4 + aac_eld 4.0)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0034, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path9);
    size_ = GetFileSize(g_mp4MPEG4Path9);
    printf("---- %s ----\n", g_mp4MPEG4Path9.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_TRACK_START_TIME, formatVal_.startTime));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 4);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 263);
}

/**
 * @tc.name: mp4Source_GetFormat_0035
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_f32le)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0035, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path10);
    size_ = GetFileSize(g_mp4MPEG4Path10);
    printf("---- %s ----\n", g_mp4MPEG4Path10.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0036
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_f32le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0036, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path11);
    size_ = GetFileSize(g_mp4MPEG4Path11);
    printf("---- %s ----\n", g_mp4MPEG4Path11.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 128000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0037
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_f64be)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0037, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path12);
    size_ = GetFileSize(g_mp4MPEG4Path12);
    printf("---- %s ----\n", g_mp4MPEG4Path12.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, 28);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0038
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s16be_twos)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0038, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path13);
    size_ = GetFileSize(g_mp4MPEG4Path13);
    printf("---- %s ----\n", g_mp4MPEG4Path13.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, 24);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0039
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s16le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0039, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path14);
    size_ = GetFileSize(g_mp4MPEG4Path14);
    printf("---- %s ----\n", g_mp4MPEG4Path14.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 128000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0040
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s24le)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0040, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path15);
    size_ = GetFileSize(g_mp4MPEG4Path15);
    printf("---- %s ----\n", g_mp4MPEG4Path15.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S24LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0041
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s24le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0041, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path16);
    size_ = GetFileSize(g_mp4MPEG4Path16);
    printf("---- %s ----\n", g_mp4MPEG4Path16.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 128000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S24LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0042
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s32le)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0042, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path17);
    size_ = GetFileSize(g_mp4MPEG4Path17);
    printf("---- %s ----\n", g_mp4MPEG4Path17.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0043
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s32le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0043, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path18);
    size_ = GetFileSize(g_mp4MPEG4Path18);
    printf("---- %s ----\n", g_mp4MPEG4Path18.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 128000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0044
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_u8)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0044, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG4Path19);
    size_ = GetFileSize(g_mp4MPEG4Path19);
    printf("---- %s ----\n", g_mp4MPEG4Path19.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.duration, 2000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_U8);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: mp4Source_GetFormat_0045
 * @tc.desc: get format when the file is mov
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_0045, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4MPEG2Path4);
    size_ = GetFileSize(g_mp4MPEG2Path4);
    printf("---- %s ----\n", g_mp4MPEG2Path4.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_EQ(formatVal_.duration, 10067000);
    EXPECT_EQ(formatVal_.timeScale, 1000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.timeScale, 15360);
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    EXPECT_EQ(formatVal_.bitRate, 1011380);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128356);
}
} // namespace