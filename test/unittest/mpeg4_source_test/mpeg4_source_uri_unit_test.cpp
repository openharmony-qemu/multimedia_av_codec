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
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_audio_common.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "mpeg4_source_unit_test.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const string TEST_TIMED_METADATA = "com.openharmony.timed_metadata.test";
string g_mp4CameraUri = TEST_URI_PATH + string("camera.mp4");
string g_multiTrackUri = TEST_URI_PATH + string("multi_trk.mov");
string g_fmp4SidxUri = TEST_URI_PATH + string("sidx.mp4");
string g_mp4FlipUri = TEST_URI_PATH + string("FLIP_V_90.mp4");
string g_mp4TimedMetadataUri = TEST_URI_PATH + string("timed_metadata_track.mp4");
string g_mp4AudiovividUri = TEST_URI_PATH + string("audiovivid_hdrvivid.mp4");
string g_mp4G711muUri = TEST_URI_PATH + string("mpeg4_g711mu.mov");
string g_mp4MPEG4Uri = TEST_URI_PATH + string("mpeg4_aac.mp4");
string g_mp4MPEG4Uri2 = TEST_URI_PATH + string("mpeg4_flac.mp4");
string g_mp4MPEG4Uri3 = TEST_URI_PATH + string("mpeg4_mp3.mp4");
string g_mp4MPEG4Uri4 = TEST_URI_PATH + string("mpeg4_opus.mp4");
string g_mp4MPEG4Uri5 = TEST_URI_PATH + string("mpeg4_amr_nb.3gp");
string g_mp4MPEG4Uri6 = TEST_URI_PATH + string("mpeg4_flac_ch7.1.mp4");
string g_mp4MPEG4Uri7 = TEST_URI_PATH + string("mpeg4_pcm_s32be_ch7.1.mov");
string g_mp4MPEG4Uri8 = TEST_URI_PATH + string("mpeg4_aac_ch7.1wide.mp4");
string g_mp4MPEG4Uri9 = TEST_URI_PATH + string("mpeg4_aac_ch4.0_eld.mp4");
string g_mp4MPEG4Uri10 = TEST_URI_PATH + string("mpeg4_f32le.mov");
string g_mp4MPEG4Uri11 = TEST_URI_PATH + string("mpeg4_f32le_lpcm.mov");
string g_mp4MPEG4Uri12 = TEST_URI_PATH + string("mpeg4_f64be.mov");
string g_mp4MPEG4Uri13 = TEST_URI_PATH + string("mpeg4_s16be_twos.mov");
string g_mp4MPEG4Uri14 = TEST_URI_PATH + string("mpeg4_s16le_lpcm.mov");
string g_mp4MPEG4Uri15 = TEST_URI_PATH + string("mpeg4_s24le.mov");
string g_mp4MPEG4Uri16 = TEST_URI_PATH + string("mpeg4_s24le_lpcm.mov");
string g_mp4MPEG4Uri17 = TEST_URI_PATH + string("mpeg4_s32le.mov");
string g_mp4MPEG4Uri18 = TEST_URI_PATH + string("mpeg4_s32le_lpcm.mov");
string g_mp4MPEG4Uri19 = TEST_URI_PATH + string("mpeg4_u8.mov");
string g_mp4H263Uri = TEST_URI_PATH + string("h263_amr_wb.3gp");
string g_mp4MPEG2Uri = TEST_URI_PATH + string("mpeg2_aac_test.mp4");
string g_mp4MPEG2Uri2 = TEST_URI_PATH + string("mpeg2_alac.mp4");
string g_mp4MPEG2Uri3 = TEST_URI_PATH + string("mpeg2_dts.mp4");
string g_mp4MPEG2Uri4 = TEST_URI_PATH + string("mpeg2_aac.mov");

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
 * @tc.name: mp4Source_GetFormat_1002
 * @tc.desc: get format when the file is mov
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1002, TestSize.Level1)
{
    printf("---- %s ------\n", g_multiTrackUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_multiTrackUri.data()));
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
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
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
 * @tc.name: mp4Source_GetFormat_1003
 * @tc.desc: get format when the file is mov
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1003, TestSize.Level1)
{
    printf("---- %s ------\n", g_multiTrackUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_multiTrackUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.duration, 2000000);
    EXPECT_EQ(formatVal_.trackCount, 4);
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.fileType, 107);
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
 * @tc.name: mp4Source_GetFormat_1004
 * @tc.desc: get format when the file is fmp4 and has sidx box
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1004, TestSize.Level1)
{
    printf("---- %s ------\n", g_fmp4SidxUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_fmp4SidxUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_START_TIME, formatVal_.startTime));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.startTime, 0);
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
 * @tc.name: mp4Source_GetFormat_1007
 * @tc.desc: get format when the file is audiovivid
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1007, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4AudiovividUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4AudiovividUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_TIME_SCALE, formatVal_.timeScale));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.duration, 2699349);
    EXPECT_EQ(formatVal_.trackCount, 2);
    EXPECT_EQ(formatVal_.timeScale, 1000);
    EXPECT_EQ(formatVal_.fileType, 101);
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
 * @tc.name: mp4Source_GetFormat_1008
 * @tc.desc: get format when the file is MP4(mpeg4 + g711mu)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1008, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4G711muUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4G711muUri.data()));
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
 * @tc.name: mp4Source_GetFormat_1009
 * @tc.desc: get format when the file is MP4(mpeg4 + g711mu)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1009, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4G711muUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4G711muUri.data()));
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
 * @tc.name: mp4Source_GetFormat_1010
 * @tc.desc: get format when the file is MP4(mpeg4 + aac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1010, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri.data()));
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
 * @tc.name: mp4Source_GetFormat_1011
 * @tc.desc: get format when the file is MP4(mpeg4 + aac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1011, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri.data()));
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
 * @tc.name: mp4Source_GetFormat_1012
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1012, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri2.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri2.data()));
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
 * @tc.name: mp4Source_GetFormat_1013
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1013, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri2.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri2.data()));
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
 * @tc.name: mp4Source_GetFormat_1014
 * @tc.desc: get format when the file is MP4(mpeg4 + mp3)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1014, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri3.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri3.data()));
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
 * @tc.name: mp4Source_GetFormat_1015
 * @tc.desc: get format when the file is MP4(mpeg4 + mp3)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1015, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri3.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri3.data()));
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
 * @tc.name: mp4Source_GetFormat_1016
 * @tc.desc: get format when the file is MP4(mpeg4 + aac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1016, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG2Uri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG2Uri.data()));
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
 * @tc.name: mp4Source_GetFormat_1017
 * @tc.desc: get format when the file is MP4(mpeg4 + opus)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1017, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri4.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri4.data()));
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
 * @tc.name: mp4Source_GetFormat_1018
 * @tc.desc: get format when the file is MP4(mpeg4 + opus)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1018, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri4.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri4.data()));
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
 * @tc.name: mp4Source_GetFormat_1019
 * @tc.desc: get format when the file is MP4(mpeg4 + amr_nb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1019, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri5.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri5.data()));
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
 * @tc.name: mp4Source_GetFormat_1020
 * @tc.desc: get format when the file is MP4(mpeg4 + amr_nb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1020, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri5.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri5.data()));
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
 * @tc.name: mp4Source_GetFormat_1021
 * @tc.desc: get format when the file is MP4(h263 + amr_wb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1021, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4H263Uri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4H263Uri.data()));
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
 * @tc.name: mp4Source_GetFormat_1022
 * @tc.desc: get format when the file is MP4(h263 + amr_wb)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1022, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4H263Uri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4H263Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 1;
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
 * @tc.name: mp4Source_GetFormat_1023
 * @tc.desc: get format when the file is MP4(mpeg2 + alac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1023, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG2Uri2.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG2Uri2.data()));
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
 * @tc.name: mp4Source_GetFormat_1024
 * @tc.desc: get format when the file is MP4(mpeg2 + alac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1024, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG2Uri2.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG2Uri2.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 1;
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
 * @tc.name: mp4Source_GetFormat_1025
 * @tc.desc: get format when the file is MP4(mpeg2 + dts)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1025, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG2Uri3.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG2Uri3.data()));
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
 * @tc.name: mp4Source_GetFormat_1026
 * @tc.desc: get format when the file is MP4(mpeg2 + dts)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1026, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG2Uri3.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG2Uri3.data()));
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
 * @tc.name: mp4Source_GetFormat_1027
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1027, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4MPEG4Uri6.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri6.data()));
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
 * @tc.name: mp4Source_GetFormat_1028
 * @tc.desc: get format when the file is MP4(mpeg4 + flac)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1028, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4MPEG4Uri6.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri6.data()));
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
 * @tc.name: mp4Source_GetFormat_1029
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1029, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4MPEG4Uri7.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri7.data()));
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
 * @tc.name: mp4Source_GetFormat_1030
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1030, TestSize.Level1)
{
    printf("---- %s ----\n", g_mp4MPEG4Uri7.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri7.data()));
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
 * @tc.name: mp4Source_GetFormat_1031
 * @tc.desc: get format when the file is MP4(mpeg4 + aac-pce 7.1 wide)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1031, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri8.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri8.data()));
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
 * @tc.name: mp4Source_GetFormat_1032
 * @tc.desc: get format when the file is MP4(mpeg4 + aac-pce 7.1 wide)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1032, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri8.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri8.data()));
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
 * @tc.name: mp4Source_GetFormat_1033
 * @tc.desc: get format when the file is MP4(mpeg4 + aac_eld 4.0)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1033, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri9.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri9.data()));
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
 * @tc.name: mp4Source_GetFormat_1034
 * @tc.desc: get format when the file is MP4(mpeg4 + aac_eld 4.0)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1034, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri9.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri9.data()));
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
 * @tc.name: mp4Source_GetFormat_1035
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_f32le)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1035, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri10.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri10.data()));
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
 * @tc.name: mp4Source_GetFormat_1036
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_f32le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1036, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri11.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri11.data()));
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
 * @tc.name: mp4Source_GetFormat_1037
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_f64be)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1037, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri12.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri12.data()));
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
 * @tc.name: mp4Source_GetFormat_1038
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s16be_twos)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1038, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri13.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri13.data()));
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
 * @tc.name: mp4Source_GetFormat_1039
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s16le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1039, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri14.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri14.data()));
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
 * @tc.name: mp4Source_GetFormat_1040
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s24le)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1040, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri15.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri15.data()));
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
 * @tc.name: mp4Source_GetFormat_1041
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s24le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1041, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri16.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri16.data()));
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
 * @tc.name: mp4Source_GetFormat_1042
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s32le)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1042, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri17.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri17.data()));
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
 * @tc.name: mp4Source_GetFormat_1043
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_s32le_lpcm)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1043, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri18.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri18.data()));
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
 * @tc.name: mp4Source_GetFormat_1044
 * @tc.desc: get format when the file is MP4(mpeg4 + pcm_u8)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1044, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG4Uri19.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG4Uri19.data()));
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
 * @tc.name: mp4Source_GetFormat_1045
 * @tc.desc: get format when the file is mov
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_1045, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4MPEG2Uri4.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4MPEG2Uri4.data()));
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