/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "native_avcodec_base.h"

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

string g_tsMpeg4Path = TEST_FILE_PATH + string("test_mpeg4_Gop25_4sec.ts");
string g_tsMpeg4Uri = TEST_URI_PATH + string("test_mpeg4_Gop25_4sec.ts");
string g_h264aacPath = TEST_FILE_PATH + string("h264_allmetadata.mov");
string g_h264mp3Path = TEST_FILE_PATH + string("h264_mp3.mov");
string g_h264vorPath = TEST_FILE_PATH + string("h264_vorbis.mov");
string g_mpg4mp2Path = TEST_FILE_PATH + string("MPEG4_mp2.mov");
string g_h264aacUri = TEST_URI_PATH + string("h264_allmetadata.mov");
string g_h264mp3Uri = TEST_URI_PATH + string("h264_mp3.mov");
string g_h264vorUri = TEST_URI_PATH + string("h264_vorbis.mov");
string g_mpg4mp2Uri = TEST_URI_PATH + string("MPEG4_mp2.mov");
string g_aviPath = TEST_FILE_PATH + string("h264_aac_metadata.avi");
string g_aviUri = TEST_URI_PATH + string("h264_aac_metadata.avi");
string g_aviAvcMp3Path = TEST_FILE_PATH + string("h264_mp3.avi");
string g_aviAvcMp3Uri = TEST_URI_PATH + string("h264_mp3.avi");
string g_avi263AacPath = TEST_FILE_PATH + string("test_263_aac_B_Gop25_4sec_cover.avi");
string g_avi263AacUri = TEST_URI_PATH + string("test_263_aac_B_Gop25_4sec_cover.avi");
string g_aviMpeg2Mp2Path = TEST_FILE_PATH + string("test_mpeg2_mp2_B_Gop25_4sec_cover.avi");
string g_aviMpeg2Mp2Uri = TEST_URI_PATH + string("test_mpeg2_mp2_B_Gop25_4sec_cover.avi");
string g_aviMpeg4PcmPath = TEST_FILE_PATH + string("mpeg4_pcm.avi");
string g_aviMpeg4PcmUri = TEST_URI_PATH + string("mpeg4_pcm.avi");
string g_mpg4mp4Path = TEST_FILE_PATH + string("MPEG4.mp4");
string g_mpg4mp4Uri = TEST_URI_PATH + string("MPEG4.mp4");
string g_mpegPath = TEST_FILE_PATH + string("mpeg_h264_mp2.mpeg");
string g_mpegPath2 = TEST_FILE_PATH + string("mpeg_h264_mp3.mpeg");
string g_mpegPath3 = TEST_FILE_PATH + string("mpeg_mpeg2_mp2.mpeg");
string g_mpegPath4 = TEST_FILE_PATH + string("mpeg_mpeg2_mp3.mpeg");
string g_mpegUri = TEST_URI_PATH + string("mpeg_h264_mp2.mpeg");
string g_mpegUri2 = TEST_URI_PATH + string("mpeg_h264_mp3.mpeg");
string g_mpegUri3 = TEST_URI_PATH + string("mpeg_mpeg2_mp2.mpeg");
string g_mpegUri4 = TEST_URI_PATH + string("mpeg_mpeg2_mp3.mpeg");
string g_mp4GltfPath = TEST_FILE_PATH + string("TEST.mp4");
} //namespace

/**********************************source FD**************************************/

namespace {
/**
 * @tc.name: AVSource_GetFormat_1025
 * @tc.desc: get source format when the file is ts(mpeg4, mp2)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1025, TestSize.Level1)
{
    fd_ = OpenFile(g_tsMpeg4Path);
    size_ = GetFileSize(g_tsMpeg4Path);
    printf("---- %s ----\n", g_tsMpeg4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
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
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2025
 * @tc.desc: get source format when the file is ts(mpeg4, aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2025, TestSize.Level1)
{
    printf("---- %s ------\n", g_tsMpeg4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_tsMpeg4Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
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
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2221
 * @tc.desc: get track format (avi)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2221, TestSize.Level1)
{
    fd_ = OpenFile(g_aviAvcMp3Path);
    size_ = GetFileSize(g_aviAvcMp3Path);
    printf("---- %s ------\n", g_aviAvcMp3Path.c_str());
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
    ASSERT_EQ(formatVal_.bitRate, 1175235);
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
    ASSERT_EQ(formatVal_.bitRate, 128332);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2231
 * @tc.desc: get track format when the file is avi (video: h264, audio: mp3)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2231, TestSize.Level1)
{
    printf("---- %s ------\n", g_aviAvcMp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_aviAvcMp3Uri.data()));
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
    ASSERT_EQ(formatVal_.bitRate, 1175235);
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
    ASSERT_EQ(formatVal_.bitRate, 128332);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
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
    ASSERT_EQ(formatVal_.codecMime, OH_AVCODEC_MIMETYPE_AUDIO_RAW);
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
    ASSERT_EQ(formatVal_.codecMime, OH_AVCODEC_MIMETYPE_AUDIO_RAW);
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

/**
 * @tc.name: AVSource_GetFormat_2296
 * @tc.desc: get source format, local (264-aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2296, TestSize.Level1)
{
    fd_ = OpenFile(g_h264aacPath);
    size_ = GetFileSize(g_h264aacPath);
    printf("---- %s ------\n", g_h264aacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_EQ(formatVal_.title, "test");
    ASSERT_EQ(formatVal_.artist, "元数据测试");
    ASSERT_EQ(formatVal_.album, "media");
    ASSERT_EQ(formatVal_.genre, "Lyrical");
    ASSERT_EQ(formatVal_.date, "2024");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.copyright, "copyright");
    ASSERT_EQ(formatVal_.duration, 10067000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 107);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2297
 * @tc.desc: get source format, uri (264-aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2297, TestSize.Level1)
{
    printf("---- %s ------\n", g_h264aacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_h264aacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_EQ(formatVal_.title, "test");
    ASSERT_EQ(formatVal_.artist, "元数据测试");
    ASSERT_EQ(formatVal_.album, "media");
    ASSERT_EQ(formatVal_.genre, "Lyrical");
    ASSERT_EQ(formatVal_.date, "2024");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.copyright, "copyright");
    ASSERT_EQ(formatVal_.duration, 10067000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 107);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2298
 * @tc.desc: get track format, local (264-aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2298, TestSize.Level1)
{
    fd_ = OpenFile(g_h264aacPath);
    size_ = GetFileSize(g_h264aacPath);
    printf("---- %s ------\n", g_h264aacPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.bitRate, 1187898);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 128064);
}

/**
 * @tc.name: AVSource_GetFormat_2299
 * @tc.desc: get track format, uri (264-aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2299, TestSize.Level1)
{
    printf("---- %s ------\n", g_h264aacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_h264aacUri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.bitRate, 1187898);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 128064);
}

/**
 * @tc.name: AVSource_GetFormat_2300
 * @tc.desc: get track format, local (264-mp3)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2300, TestSize.Level1)
{
    fd_ = OpenFile(g_h264mp3Path);
    size_ = GetFileSize(g_h264mp3Path);
    printf("---- %s ------\n", g_h264mp3Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_2301
 * @tc.desc: get track format, uri (264-mp3)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2301, TestSize.Level1)
{
    printf("---- %s ------\n", g_h264mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_h264mp3Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_2302
 * @tc.desc: get track format, local (264-vorbis)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2302, TestSize.Level1)
{
    fd_ = OpenFile(g_h264vorPath);
    size_ = GetFileSize(g_h264vorPath);
    printf("---- %s ------\n", g_h264vorPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/vorbis");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 80435);
}

/**
 * @tc.name: AVSource_GetFormat_2303
 * @tc.desc: get track format, uri (264-vorbis)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2303, TestSize.Level1)
{
    printf("---- %s ------\n", g_h264vorUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_h264vorUri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/vorbis");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 80435);
}

/**
 * @tc.name: AVSource_GetFormat_2304
 * @tc.desc: get track format, local (mpeg4-mp2)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2304, TestSize.Level1)
{
    fd_ = OpenFile(g_mpg4mp2Path);
    size_ = GetFileSize(g_mpg4mp2Path);
    printf("---- %s ------\n", g_mpg4mp2Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 191862);
}

/**
 * @tc.name: AVSource_GetFormat_2305
 * @tc.desc: get track format, local (mpeg4-mp2)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2305, TestSize.Level1)
{
    printf("---- %s ------\n", g_mpg4mp2Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mpg4mp2Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 191862);
}

/**
 * @tc.name: AVSource_GetFormat_2312
 * @tc.desc: get track format, local (mpeg4-mp4)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2312, TestSize.Level1)
{
    fd_ = OpenFile(g_mpg4mp4Path);
    size_ = GetFileSize(g_mpg4mp4Path);
    printf("---- %s ------\n", g_mpg4mp4Path.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 130231);
}

/**
 * @tc.name: AVSource_GetFormat_2313
 * @tc.desc: get  format, uri(mpeg4-mp4)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2313, TestSize.Level1)
{
    printf("---- %s ------\n", g_mpg4mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mpg4mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 130231);
}

/**
 * @tc.name: AVSource_GetFormat_2250
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2250, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegPath);
    size_ = GetFileSize(g_mpegPath);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 384000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_2251
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2251, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegPath2);
    size_ = GetFileSize(g_mpegPath2);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_2252
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2252, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegPath3);
    size_ = GetFileSize(g_mpegPath3);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 384000);
}

/**
 * @tc.name: AVSource_GetFormat_2253
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2253, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegPath4);
    size_ = GetFileSize(g_mpegPath4);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_2260
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2260, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegUri);
    size_ = GetFileSize(g_mpegUri);
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mpegUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 384000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_2261
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2261, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegUri2);
    size_ = GetFileSize(g_mpegUri2);
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mpegUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_2262
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2262, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegUri3);
    size_ = GetFileSize(g_mpegUri3);
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mpegUri3.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 384000);
}

/**
 * @tc.name: AVSource_GetFormat_2263
 * @tc.desc: get source format(mpeg)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2263, TestSize.Level1)
{
    fd_ = OpenFile(g_mpegUri4);
    size_ = GetFileSize(g_mpegUri4);
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mpegUri4.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 106);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 24.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_2270
 * @tc.desc: get source format(gltf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2270, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4GltfPath.data());
    fd_ = OpenFile(g_mp4GltfPath);
    size_ = GetFileSize(g_mp4GltfPath);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_IS_GLTF, formatVal_.isGltf));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_GLTF_OFFSET, formatVal_.gltfOffset));
    ASSERT_EQ(formatVal_.isGltf, 1);
    ASSERT_EQ(formatVal_.gltfOffset, 2408943);
}
} // namespace
