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
string g_mp4CameraUri = TEST_URI_PATH + string("camera.mp4");
string g_mp4FlipPath = TEST_FILE_PATH + string("FLIP_V_90.mp4");
string g_mp4FlipUri = TEST_URI_PATH + string("FLIP_V_90.mp4");
string g_mp4TimedMetadataPath = TEST_FILE_PATH + string("timed_metadata_track.mp4");
string g_mp4TimedMetadataUri = TEST_URI_PATH + string("timed_metadata_track.mp4");
string g_mp4UnsupportOgg = TEST_FILE_PATH + string("mpeg4_unsupport_ogg.mp4");
string g_mp4UnsupportDts = TEST_FILE_PATH + string("mpeg4_unsupport_dts.mp4");
} // namespace

void Mpeg4SourceUnitTest::SetUpTestCase(void)
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}

void Mpeg4SourceUnitTest::TearDownTestCase(void)
{
    server->StopServer();
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
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
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

/**********************************source URI**************************************/
/**
 * @tc.name: mp4Source_GetFormat_1001
 * @tc.desc: get user data
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1001, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4CameraUri.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4CameraUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[source Format]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
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
 * @tc.name: mp4Source_GetFormat_1005
 * @tc.desc: get displaymatrix
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1005, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4FlipUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4FlipUri.data()));
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
 * @tc.name: mp4Source_GetFormat_1006
 * @tc.desc: get format when the file is timed metadata file
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1006, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4TimedMetadataUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4TimedMetadataUri.data()));
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
 * @tc.name: mp4Source_Unsupport_1001
 * @tc.desc: unsupport audio track vorbis, switch ffmpeg demuxer auto
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_Unsupport_1001, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4UnsupportOgg);
    size_ = GetFileSize(g_mp4UnsupportOgg);
    printf("---- %s ----\n", g_mp4UnsupportOgg.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_EQ(formatVal_.codecMime, CodecMimeType::AUDIO_VORBIS);
}

/**
 * @tc.name: mp4Source_Unsupport_1002
 * @tc.desc: unsupport audio track dts, switch ffmpeg demuxer auto
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_Unsupport_1002, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4UnsupportDts);
    size_ = GetFileSize(g_mp4UnsupportDts);
    printf("---- %s ----\n", g_mp4UnsupportDts.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_EQ(formatVal_.codecMime, CodecMimeType::AUDIO_DTS);
}
} // namespace
