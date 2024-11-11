/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "avsource_unit_test.h"
#include "media_data_source.h"
#include "native_avsource.h"

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

string g_aviPath = TEST_FILE_PATH + string("h264_aac_metadata.avi");
string g_aviUri = TEST_URI_PATH + string("h264_aac_metadata.avi");
string g_avi263AacPath = TEST_FILE_PATH + string("test_263_aac_B_Gop25_4sec_cover.avi");
string g_avi263AacUri = TEST_URI_PATH + string("test_263_aac_B_Gop25_4sec_cover.avi");
string g_aviMpeg2Mp2Path = TEST_FILE_PATH + string("test_mpeg2_mp2_B_Gop25_4sec_cover.avi");
string g_aviMpeg2Mp2Uri = TEST_URI_PATH + string("test_mpeg2_mp2_B_Gop25_4sec_cover.avi");
string g_aviMpeg4PcmPath = TEST_FILE_PATH + string("mpeg4_pcm.avi");
string g_aviMpeg4PcmUri = TEST_URI_PATH + string("mpeg4_pcm.avi");
}

/**********************************source FD**************************************/
namespace {
/**
 * @tc.name: AVSource_GetFormat_2221
 * @tc.desc: get track format (avi)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2221, TestSize.Level1)
{
    fd_ = OpenFile(g_aviPath);
    size_ = GetFileSize(g_aviPath);
    printf("---- %s ------\n", g_aviPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.bitRate, 1269765);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 120.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatVal_.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128248);
    ASSERT_EQ(formatVal_.aacIsAdts, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2231
 * @tc.desc: get track format when the file is avi (video: h264, audio:aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2231, TestSize.Level1)
{
    printf("---- %s ------\n", g_aviUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_aviUri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.bitRate, 1269765);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 120.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatVal_.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128248);
    ASSERT_EQ(formatVal_.aacIsAdts, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2222
 * @tc.desc: get source format (avi)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2222, TestSize.Level1)
{
    fd_ = OpenFile(g_aviPath);
    size_ = GetFileSize(g_aviPath);
    printf("---- %s ----\n", g_aviPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.date, "2024");
    ASSERT_EQ(formatVal_.genre, "Lyrical");
    ASSERT_EQ(formatVal_.title, "test");
    ASSERT_EQ(formatVal_.duration, 10025000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 105);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2232
 * @tc.desc: get source format when the file is avi
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2232, TestSize.Level1)
{
    printf("---- %s ----\n", g_aviUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_aviUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.date, "2024");
    ASSERT_EQ(formatVal_.genre, "Lyrical");
    ASSERT_EQ(formatVal_.title, "test");
    ASSERT_EQ(formatVal_.duration, 10025000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 105);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2226
 * @tc.desc: get track format (avi)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2226, TestSize.Level1)
{
    fd_ = OpenFile(g_aviMpeg2Mp2Path);
    size_ = GetFileSize(g_aviMpeg2Mp2Path);
    printf("---- %s ------\n", g_aviMpeg2Mp2Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.bitRate, 3328590);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 384000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2236
 * @tc.desc: get track format when the file is avi (video: mpeg2, audio:mp2)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2236, TestSize.Level1)
{
    printf("---- %s ------\n", g_aviMpeg2Mp2Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_aviMpeg2Mp2Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.bitRate, 3328590);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 384000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2229
 * @tc.desc: get track format (avi)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2229, TestSize.Level1)
{
    fd_ = OpenFile(g_aviMpeg4PcmPath);
    size_ = GetFileSize(g_aviMpeg4PcmPath);
    printf("---- %s ------\n", g_aviMpeg4PcmPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.bitRate, 766523);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 1411200);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2239
 * @tc.desc: get track format when the file is avi (video: mpeg4, audio:pcm)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2239, TestSize.Level1)
{
    printf("---- %s ------\n", g_aviMpeg4PcmUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_aviMpeg4PcmUri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.bitRate, 766523);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 1411200);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2230
 * @tc.desc: get track format (avi)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2230, TestSize.Level1)
{
    fd_ = OpenFile(g_avi263AacPath);
    size_ = GetFileSize(g_avi263AacPath);
    printf("---- %s ------\n", g_avi263AacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/h263");
    ASSERT_EQ(formatVal_.width, 1408);
    ASSERT_EQ(formatVal_.height, 1152);
    ASSERT_EQ(formatVal_.bitRate, 2704586);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2240
 * @tc.desc: get track format when the file is avi (video: h263, audio:aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2240, TestSize.Level1)
{
    printf("---- %s ------\n", g_avi263AacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_avi263AacUri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/h263");
    ASSERT_EQ(formatVal_.width, 1408);
    ASSERT_EQ(formatVal_.height, 1152);
    ASSERT_EQ(formatVal_.bitRate, 2704586);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}
}