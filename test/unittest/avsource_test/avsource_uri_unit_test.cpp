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
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_audio_common.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "avsource_unit_test.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
string g_mp4Uri = TEST_URI_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_mp4Uri3 = TEST_URI_PATH + string("test_mpeg2_B_Gop25_4sec.mp4");
string g_mp4Uri5 = TEST_URI_PATH + string("test_suffix_mismatch.mp4");
string g_mp4Uri6 = TEST_URI_PATH + string("test_empty_file.mp4");
string g_mp4Uri7 = TEST_URI_PATH + string("test_error.mp4");
string g_mp4Uri8 = TEST_URI_PATH + string("zero_track.mp4");
string g_mkvUri2 = TEST_URI_PATH + string("h264_opus_4sec.mkv");
string g_tsUri = TEST_URI_PATH + string("test_mpeg2_Gop25_4sec.ts");
string g_aacUri = TEST_URI_PATH + string("audio/aac_44100_1.aac");
string g_flacUri = TEST_URI_PATH + string("audio/flac_48000_1_cover.flac");
string g_m4aUri = TEST_URI_PATH + string("audio/m4a_48000_1.m4a");
string g_mp3Uri = TEST_URI_PATH + string("audio/mp3_48000_1_cover.mp3");
string g_oggUri = TEST_URI_PATH + string("audio/ogg_48000_1.ogg");
string g_wavUri = TEST_URI_PATH + string("audio/wav_48000_1.wav");
string g_amrUri = TEST_URI_PATH + string("audio/amr_nb_8000_1.amr");
string g_amrUri2 = TEST_URI_PATH + string("audio/amr_wb_16000_1.amr");
string g_audioVividUri = TEST_URI_PATH + string("2obj_44100Hz_16bit_32k.m4a");
string g_fmp4AvcUri = TEST_URI_PATH + string("h264_fmp4.mp4");
string g_fmp4m4vUri = TEST_URI_PATH + string("h264_fmp4.m4v");
string g_fmp4m4aUri = TEST_URI_PATH + string("audio/h264_fmp4.m4a");
string g_srt = TEST_URI_PATH + string("subtitle.srt");

/**********************************source URI**************************************/
/**
 * @tc.name: AVSource_CreateSourceWithURI_1000
 * @tc.desc: create source with uri, mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1000, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1020
 * @tc.desc: create source with uri, but file is abnormal
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1020, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri5.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri5.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1030
 * @tc.desc: create source with uri, but file is empty
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1030, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri6.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri6.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1040
 * @tc.desc: create source with uri, but file is error
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1040, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri7.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri7.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1050
 * @tc.desc: create source with uri, but track is zero
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1050, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri8.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri8.data()));
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1060
 * @tc.desc: create source with invalid uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1060, TestSize.Level1)
{
    string uri = "http://127.0.0.1:46666/asdffafafaf";
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(uri.data()));
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1070
 * @tc.desc: Create source repeatedly
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1070, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
}

/**
 * @tc.name: AVSource_CreateSourceWithURI_1080
 * @tc.desc: destroy source
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithURI_1080, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    printf("---- %s ------\n", g_mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    shared_ptr<AVSourceMock> source2 = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp3Uri.data()));
    ASSERT_NE(source2, nullptr);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
    ASSERT_EQ(source2->Destroy(), AV_ERR_OK);
    source2 = nullptr;
}

/**
 * @tc.name: AVSource_GetFormat_2000
 * @tc.desc: get source format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2000, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.duration, 4120000);
    ASSERT_EQ(formatVal_.trackCount, 3);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 101);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2010
 * @tc.desc: get track format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2010, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
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
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.bitRate, 7782407);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 128563);
    ASSERT_EQ(formatVal_.aacIsAdts, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2011
 * @tc.desc: get track format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2011, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 2;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
#ifdef AVSOURCE_INNER_UNIT_TEST
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/test_264_B_Gop25_4sec_cover_uri.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &addr_, buffSize_));
    fwrite(addr_, sizeof(uint8_t), buffSize_, saveFile);
    fclose(saveFile);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2020
 * @tc.desc: get source format when the file is ts
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2020, TestSize.Level1)
{
    printf("---- %s ------\n", g_tsUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_tsUri.data()));
    ASSERT_NE(source_, nullptr);
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
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 127103);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2030
 * @tc.desc: get source format when the file is mp4
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2030, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri3.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri3.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 101);
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.author, "author");
#endif
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.bitRate, 3889231);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128563);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2050
 * @tc.desc: get format when the file is mkv (video: h264, audio: opus)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2050, TestSize.Level1)
{
    printf("---- %s ------\n", g_mkvUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mkvUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.albumArtist, "album_artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "copyRight");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.duration, 4001000);
    ASSERT_EQ(formatVal_.trackCount, 2);
    ASSERT_EQ(formatVal_.language, "language");
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 103);
    ASSERT_EQ(formatVal_.author, "author");
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2060
 * @tc.desc: get format when the file is mkv (video: h264, audio: opus)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2060, TestSize.Level1)
{
    printf("---- %s ------\n", g_mkvUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mkvUri2.data()));
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
    ASSERT_EQ(formatVal_.width, 1920);
    ASSERT_EQ(formatVal_.height, 1080);
    ASSERT_EQ(formatVal_.frameRate, 60.000000);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/opus");
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_2100
 * @tc.desc: get format when the file is aac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2100, TestSize.Level1)
{
    printf("---- %s ------\n", g_aacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_aacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30023469);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 202);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, formatVal_.aacIsAdts));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 126800);
    ASSERT_EQ(formatVal_.aacIsAdts, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}


/**
 * @tc.name: AVSource_GetFormat_2110
 * @tc.desc: get format when the file is flac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2110, TestSize.Level1)
{
    printf("---- %s ------\n", g_flacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_flacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.duration, 30000000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.author, "author");
    ASSERT_EQ(formatVal_.fileType, 204);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2111
 * @tc.desc: get format when the file is flac
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2111, TestSize.Level1)
{
    printf("---- %s ------\n", g_flacUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_flacUri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/flac");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
#ifdef AVSOURCE_INNER_UNIT_TEST
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/flac_48000_1_uri.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &addr_, buffSize_));
    fwrite(addr_, sizeof(uint8_t), buffSize_, saveFile);
    fclose(saveFile);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2120
 * @tc.desc: get format when the file is m4a
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2120, TestSize.Level1)
{
    printf("---- %s ------\n", g_m4aUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "lyrics");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.duration, 30016000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.fileType, 206);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2121
 * @tc.desc: get format when the file is m4a
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2121, TestSize.Level1)
{
    printf("---- %s ------\n", g_m4aUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 69594);
}

/**
 * @tc.name: AVSource_GetFormat_2130
 * @tc.desc: get format when the file is mp3
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2130, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp3Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM_ARTIST, formatVal_.albumArtist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LYRICS, formatVal_.lyrics));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DESCRIPTION, formatVal_.description));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.albumArtist, "album artist");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.lyrics, "SLT");
    ASSERT_EQ(formatVal_.description, "description");
    ASSERT_EQ(formatVal_.language, "language");
    ASSERT_EQ(formatVal_.duration, 30024000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMPOSER, formatVal_.composer));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_AUTHOR, formatVal_.author));
    ASSERT_EQ(formatVal_.author, "author");
    ASSERT_EQ(formatVal_.composer, "composer");
    ASSERT_EQ(formatVal_.fileType, 203);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2131
 * @tc.desc: get format when the file is mp3
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2131, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp3Uri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp3Uri.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 64000);
#ifdef AVSOURCE_INNER_UNIT_TEST
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    const char* outFile = "/data/test/mp3_48000_1_cover_uri.bin";
    FILE* saveFile = fopen(outFile, "wb");
    ASSERT_TRUE(format_->GetBuffer(MediaDescriptionKey::MD_KEY_COVER, &addr_, buffSize_));
    fwrite(addr_, sizeof(uint8_t), buffSize_, saveFile);
    fclose(saveFile);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2140
 * @tc.desc: get format when the file is ogg
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2140, TestSize.Level1)
{
    printf("---- %s ------\n", g_oggUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_oggUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/vorbis");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.bitRate, 80000);
}

/**
 * @tc.name: AVSource_GetFormat_2150
 * @tc.desc: get format when the file is wav
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2150, TestSize.Level1)
{
    printf("---- %s ------\n", g_wavUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_wavUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_TITLE, formatVal_.title));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ARTIST, formatVal_.artist));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_ALBUM, formatVal_.album));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_DATE, formatVal_.date));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COMMENT, formatVal_.comment));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_GENRE, formatVal_.genre));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_COPYRIGHT, formatVal_.copyright));
    ASSERT_TRUE(format_->GetStringValue(AVSourceFormat::SOURCE_LANGUAGE, formatVal_.language));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.title, "title");
    ASSERT_EQ(formatVal_.artist, "artist");
    ASSERT_EQ(formatVal_.album, "album");
    ASSERT_EQ(formatVal_.date, "2023");
    ASSERT_EQ(formatVal_.comment, "comment");
    ASSERT_EQ(formatVal_.genre, "genre");
    ASSERT_EQ(formatVal_.copyright, "Copyright");
    ASSERT_EQ(formatVal_.language, "language");
    ASSERT_EQ(formatVal_.duration, 30037333);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 207);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
}

/**
 * @tc.name: AVSource_GetFormat_2151
 * @tc.desc: get format when the file is wav
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2151, TestSize.Level1)
{
    printf("---- %s ------\n", g_wavUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_wavUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.bitRate, 768000);
}

/**
 * @tc.name: AVSource_GetFormat_2160
 * @tc.desc: get format when the file is amr
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2160, TestSize.Level1)
{
    printf("---- %s ------\n", g_amrUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_amrUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30988375);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 201);
#endif
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 8000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/3gpp");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_2170
 * @tc.desc: get format when the file is amr
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2170, TestSize.Level1)
{
    printf("---- %s ------\n", g_amrUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_amrUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30937500);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_EQ(formatVal_.fileType, 201);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.hasVideo, 0);
#endif
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(formatVal_.sampleRate, 16000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/amr-wb");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
}

/**
 * @tc.name: AVSource_GetFormat_2180
 * @tc.desc: get format when the file is audio vivid (m4a)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2180, TestSize.Level1)
{
    printf("---- %s ------\n", g_audioVividUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_audioVividUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 32044000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_EQ(formatVal_.fileType, 206);
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
#endif
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.codecMime, CodecMimeType::AUDIO_AVS3DA);
    ASSERT_EQ(formatVal_.bitRate, 64082);
}

/**
 * @tc.name: AVSource_GetFormat_1317
 * @tc.desc: get fmp4 264 mp4 format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1317, TestSize.Level1)
{
    printf("---- %s ------\n", g_fmp4AvcUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_fmp4AvcUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 10066666);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 101);
#endif
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
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1320
 * @tc.desc: get fmp4 m4v format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1320, TestSize.Level1)
{
    printf("---- %s ------\n", g_fmp4m4vUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_fmp4m4vUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 10033333);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 0);
    ASSERT_EQ(formatVal_.fileType, 206);
#endif
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
}

/**
 * @tc.name: AVSource_GetFormat_1321
 * @tc.desc: get fmp4 m4a format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1321, TestSize.Level1)
{
    printf("---- %s ------\n", g_fmp4m4aUri.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_fmp4m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 10064354);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 206);
#endif
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
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_3001
 * @tc.desc: get format when the file is srt
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_3001, TestSize.Level1)
{
    printf("---- %s ------\n", g_srt.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_srt.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_SUBTITLE, formatVal_.hasSubtitle));
    ASSERT_EQ(formatVal_.fileType, 301);
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 0);
    ASSERT_EQ(formatVal_.hasSubtitle, 1);
#endif

    printf("---- %s ----\n", g_srt.c_str());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_SUBTITLE);
    ASSERT_EQ(formatVal_.codecMime, "application/x-subrip");
}
} // namespace