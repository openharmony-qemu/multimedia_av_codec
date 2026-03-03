/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
static const string TEST_TIMED_METAASFA = "com.openharmony.timed_metadata.test";
const int64_t SOURCE_OFFSET = 0;
string g_datPath1 = TEST_FILE_PATH + string("test_msmpeg4_wma.asf");
string g_datPath2 = TEST_FILE_PATH + string("test_mpeg4_wma.asf");
string g_datUri1 = TEST_URI_PATH + string("test_msmpeg4_wma.asf");
string g_datUri2 = TEST_URI_PATH + string("test_mpeg4_wma.asf");
string g_asfPath2 = TEST_FILE_PATH + string("h263_aac.asf");
string g_asfPath3 = TEST_FILE_PATH + string("h264_aac.asf");
string g_asfPath4 = TEST_FILE_PATH + string("h264_ac3.asf");
string g_asfPath5 = TEST_FILE_PATH + string("h264_adpcm_g722.asf");
string g_asfPath6 = TEST_FILE_PATH + string("h264_adpcm_g726.asf");
string g_asfPath7 = TEST_FILE_PATH + string("h264_adpcm_ima_wav.asf");
string g_asfPath8 = TEST_FILE_PATH + string("h264_adpcm_ms.asf");
string g_asfPath10 = TEST_FILE_PATH + string("h264_adpcm_yamaha.asf");
string g_asfPath11 = TEST_FILE_PATH + string("h264_amr_nb.asf");
string g_asfPath12 = TEST_FILE_PATH + string("h264_amr_wb.asf");
string g_asfPath13 = TEST_FILE_PATH + string("h264_eac3.asf");
string g_asfPath14 = TEST_FILE_PATH + string("h264_av1_aac.asf");
string g_asfPath15 = TEST_FILE_PATH + string("h264_flac.asf");
string g_asfPath16 = TEST_FILE_PATH + string("h264_gsm_ms.asf");
string g_asfPath17 = TEST_FILE_PATH + string("h264_mp2.asf");
string g_asfPath18 = TEST_FILE_PATH + string("h264_mp3.asf");
string g_asfPath19 = TEST_FILE_PATH + string("h264_pcm_alaw.asf");
string g_asfPath20 = TEST_FILE_PATH + string("h264_pcm_f32le.asf");
string g_asfPath21 = TEST_FILE_PATH + string("h264_pcm_f64le.asf");
string g_asfPath22 = TEST_FILE_PATH + string("h264_pcm_mulaw.asf");
string g_asfPath23 = TEST_FILE_PATH + string("h264_pcm_s16le.asf");
string g_asfPath24 = TEST_FILE_PATH + string("h264_pcm_s24le.asf");
string g_asfPath25 = TEST_FILE_PATH + string("h264_pcm_s32le.asf");
string g_asfPath26 = TEST_FILE_PATH + string("h264_pcm_s64le.asf");
string g_asfPath27 = TEST_FILE_PATH + string("h264_pcm_u8.asf");
string g_asfPath28 = TEST_FILE_PATH + string("h264_vorbis.asf");
string g_asfPath29 = TEST_FILE_PATH + string("h264_wmav1.asf");
string g_asfPath30 = TEST_FILE_PATH + string("h264_wmav2.asf");
string g_asfPath31 = TEST_FILE_PATH + string("mjpeg_aac.asf");
string g_asfPath32 = TEST_FILE_PATH + string("mpeg2_aac.asf");
string g_asfPath33 = TEST_FILE_PATH + string("mpeg4_aac.asf");
string g_asfPath34 = TEST_FILE_PATH + string("msvideo1_aac.asf");
string g_asfPath35 = TEST_FILE_PATH + string("vp9_aac.asf");
string g_asfPath36 = TEST_FILE_PATH + string("test_error.asf");
string g_asfPath37 = TEST_FILE_PATH + string("av1_mp3.asf");
string g_asfPath38 = TEST_FILE_PATH + string("DVCNTSC_720x480_25_422_dv5n.asf");
string g_asfPath39 = TEST_FILE_PATH + string("wmv3_wmapro.asf");
string g_asfPath40 = TEST_FILE_PATH + string("DVCPAL_720x576_25_411_dvpp.asf");
string g_asfPath41 = TEST_FILE_PATH + string("DVCPROHD_1280x1080_29_422_dvh6.asf");
string g_asfPath42 = TEST_FILE_PATH + string("vc1.asf");
string g_asfPath43 = TEST_FILE_PATH + string("vp8_aac.asf");
/**
 * @tc.name: AVSource_ASF_GetFormat_0001
 * @tc.desc: get source format when the file is asf(mpeg4, aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0001, TestSize.Level1)
{
    fd_ = OpenFile(g_datPath1);
    size_ = GetFileSize(g_datPath1);
    printf("---- %s ----\n", g_datPath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
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
    printf("[ formatVal_.codecMime ]: %s\n", formatVal_.codecMime.c_str());
    ASSERT_EQ(formatVal_.width, 640);
    ASSERT_EQ(formatVal_.height, 360);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
                    formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 96000);
    ASSERT_EQ(formatVal_.codecMime, "audio/wmav2");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0002
 * @tc.desc: get source format when the file is asf(mpeg4, aac)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0002, TestSize.Level1)
{
    printf("---- %s ------\n", g_datUri1.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_datUri1.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
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
    printf("[ formatVal_.codecMime ]: %s\n", formatVal_.codecMime.c_str());
    ASSERT_EQ(formatVal_.width, 640);
    ASSERT_EQ(formatVal_.height, 360);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
                formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT,
                formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 96000);
    ASSERT_EQ(formatVal_.codecMime, "audio/wmav2");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0003
 * @tc.desc: get source format when the file is asf(h264, mp3)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0003, TestSize.Level1)
{
    fd_ = OpenFile(g_datPath2);
    size_ = GetFileSize(g_datPath2);
    printf("---- %s ----\n", g_datPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
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
    printf("[ formatVal_.codecMime ]: %s\n", formatVal_.codecMime.c_str());
    ASSERT_EQ(formatVal_.width, 640);
    ASSERT_EQ(formatVal_.height, 360);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
                formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 96000);
    ASSERT_EQ(formatVal_.codecMime, "audio/wmav2");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0004
 * @tc.desc: get source format when the file is asf(h264, mp3)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0004, TestSize.Level1)
{
    printf("---- %s ------\n", g_datUri2.data());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_datUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
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
    printf("[ formatVal_.codecMime ]: %s\n", formatVal_.codecMime.c_str());
    ASSERT_EQ(formatVal_.width, 640);
    ASSERT_EQ(formatVal_.height, 360);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
                formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 96000);
    ASSERT_EQ(formatVal_.codecMime, "audio/wmav2");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0005
 * @tc.desc: get source format(h263_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0005, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath2.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath2);
    size_ = GetFileSize(g_asfPath2);
    printf("---- %s ----\n", g_asfPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3141000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0006
 * @tc.desc: get format when the file is h263_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0006, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath2.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath2);
    size_ = GetFileSize(g_asfPath2);
    printf("---- %s ------\n", g_asfPath2.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/h263");
    ASSERT_EQ(formatVal_.width, 176);
    ASSERT_EQ(formatVal_.height, 144);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 29.970029970029969);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0007
 * @tc.desc: get source format(h264_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0007, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath3.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath3);
    size_ = GetFileSize(g_asfPath3);
    printf("---- %s ----\n", g_asfPath3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0008
 * @tc.desc: get format when the file is h264_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0008, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath3.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath3);
    size_ = GetFileSize(g_asfPath3);
    printf("---- %s ------\n", g_asfPath3.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0009
 * @tc.desc: get source format(h264_ac3.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0009, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath4.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath4);
    size_ = GetFileSize(g_asfPath4);
    printf("---- %s ----\n", g_asfPath4.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0010
 * @tc.desc: get format when the file is h264_ac3.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0010, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath4.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath4);
    size_ = GetFileSize(g_asfPath4);
    printf("---- %s ------\n", g_asfPath4.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 192000);
    ASSERT_EQ(formatVal_.codecMime, "audio/ac3");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0011
 * @tc.desc: get source format(h264_adpcm_g722.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0011, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath5.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath5);
    size_ = GetFileSize(g_asfPath5);
    printf("---- %s ----\n", g_asfPath5.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0012
 * @tc.desc: get format when the file is h264_adpcm_g722.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0012, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath5.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath5);
    size_ = GetFileSize(g_asfPath5);
    printf("---- %s ------\n", g_asfPath5.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_g722");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0013
 * @tc.desc: get source format(h264_adpcm_g726.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0013, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath6.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath6);
    size_ = GetFileSize(g_asfPath6);
    printf("---- %s ----\n", g_asfPath6.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0014
 * @tc.desc: get format when the file is h264_adpcm_g726.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0014, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath6.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath6);
    size_ = GetFileSize(g_asfPath6);
    printf("---- %s ------\n", g_asfPath6.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.sampleRate, 8000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 32000);
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_g726");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0015
 * @tc.desc: get source format(h264_adpcm_ima_wav.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0015, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath7.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath7);
    size_ = GetFileSize(g_asfPath7);
    printf("---- %s ----\n", g_asfPath7.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0016
 * @tc.desc: get format when the file is h264_adpcm_ima_wav.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0016, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath7.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath7);
    size_ = GetFileSize(g_asfPath7);
    printf("---- %s ------\n", g_asfPath7.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_ima_wav");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0017
 * @tc.desc: get source format(h264_adpcm_ms.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0017, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath8.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath8);
    size_ = GetFileSize(g_asfPath8);
    printf("---- %s ----\n", g_asfPath8.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0018
 * @tc.desc: get format when the file is h264_adpcm_ms.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0018, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath8.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath8);
    size_ = GetFileSize(g_asfPath8);
    printf("---- %s ------\n", g_asfPath8.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_ms");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0021
 * @tc.desc: get source format(h264_adpcm_yamaha.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0021, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath10.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath10);
    size_ = GetFileSize(g_asfPath10);
    printf("---- %s ----\n", g_asfPath10.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0022
 * @tc.desc: get format when the file is h264_adpcm_yamaha.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0022, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath10.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath10);
    size_ = GetFileSize(g_asfPath10);
    printf("---- %s ------\n", g_asfPath10.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_yamaha");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0023
 * @tc.desc: get source format(h264_amr_nb.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0023, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath11.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath11);
    size_ = GetFileSize(g_asfPath11);
    printf("---- %s ----\n", g_asfPath11.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0024
 * @tc.desc: get format when the file is h264_amr_nb.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0024, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath11.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath11);
    size_ = GetFileSize(g_asfPath11);
    printf("---- %s ------\n", g_asfPath11.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 8000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/3gpp");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0025
 * @tc.desc: get source format(h264_amr_wb.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0025, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath12.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath12);
    size_ = GetFileSize(g_asfPath12);
    printf("---- %s ----\n", g_asfPath12.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0026
 * @tc.desc: get format when the file is h264_amr_wb.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0026, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath12.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath12);
    size_ = GetFileSize(g_asfPath12);
    printf("---- %s ------\n", g_asfPath12.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 16000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.codecMime, "audio/amr-wb");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

#ifdef SUPPORT_DEMUXER_EAC3
/**
 * @tc.name: AVSource_ASF_GetFormat_0027
 * @tc.desc: get source format(h264_eac3.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0027, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath13.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath13);
    size_ = GetFileSize(g_asfPath13);
    printf("---- %s ----\n", g_asfPath13.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0028
 * @tc.desc: get format when the file is h264_eac3.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0028, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath13.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath13);
    size_ = GetFileSize(g_asfPath13);
    printf("---- %s ------\n", g_asfPath13.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 191559);
    ASSERT_EQ(formatVal_.codecMime, "audio/eac3");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}
#endif

/**
 * @tc.name: AVSource_ASF_GetFormat_0029
 * @tc.desc: get source format(h264_av1_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0029, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath14.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath14);
    size_ = GetFileSize(g_asfPath14);
    printf("---- %s ----\n", g_asfPath14.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3018000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0030
 * @tc.desc: get format when the file is h264_av1_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0030, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath14.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath14);
    size_ = GetFileSize(g_asfPath14);
    printf("---- %s ------\n", g_asfPath14.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/av1");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 129208);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0031
 * @tc.desc: get source format(h264_flac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0031, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath15.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath15);
    size_ = GetFileSize(g_asfPath15);
    printf("---- %s ----\n", g_asfPath15.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0032
 * @tc.desc: get format when the file is h264_flac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0032, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath15.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath15);
    size_ = GetFileSize(g_asfPath15);
    printf("---- %s ------\n", g_asfPath15.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/flac");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S32LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0033
 * @tc.desc: get source format(h264_gsm_ms.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0033, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath16.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath16);
    size_ = GetFileSize(g_asfPath16);
    printf("---- %s ----\n", g_asfPath16.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0034
 * @tc.desc: get format when the file is h264_gsm_ms.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0034, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath16.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath16);
    size_ = GetFileSize(g_asfPath16);
    printf("---- %s ------\n", g_asfPath16.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.sampleRate, 8000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 13000);
    ASSERT_EQ(formatVal_.codecMime, "audio/gsm_ms");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0035
 * @tc.desc: get source format(h264_mp2.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0035, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath17.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath17);
    size_ = GetFileSize(g_asfPath17);
    printf("---- %s ----\n", g_asfPath17.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0036
 * @tc.desc: get format when the file is h264_mp2.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0036, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath17.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath17);
    size_ = GetFileSize(g_asfPath17);
    printf("---- %s ------\n", g_asfPath17.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0037
 * @tc.desc: get source format(h264_mp3.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0037, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath18.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath18);
    size_ = GetFileSize(g_asfPath18);
    printf("---- %s ----\n", g_asfPath18.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0038
 * @tc.desc: get format when the file is h264_mp3.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0038, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath18.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath18);
    size_ = GetFileSize(g_asfPath18);
    printf("---- %s ------\n", g_asfPath18.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0039
 * @tc.desc: get source format(h264_pcm_alaw.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0039, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath19.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath19);
    size_ = GetFileSize(g_asfPath19);
    printf("---- %s ----\n", g_asfPath19.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0040
 * @tc.desc: get format when the file is h264_pcm_alaw.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0040, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath19.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath19);
    size_ = GetFileSize(g_asfPath19);
    printf("---- %s ------\n", g_asfPath19.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 705600);
    ASSERT_EQ(formatVal_.codecMime, "audio/g711a");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0041
 * @tc.desc: get source format(h264_pcm_f32le.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0041, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath20.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath20);
    size_ = GetFileSize(g_asfPath20);
    printf("---- %s ----\n", g_asfPath20.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0042
 * @tc.desc: get format when the file is h264_pcm_f32le.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0042, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath20.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath20);
    size_ = GetFileSize(g_asfPath20);
    printf("---- %s ------\n", g_asfPath20.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 2822400);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0043
 * @tc.desc: get source format(h264_pcm_f64le.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0043, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath21.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath21);
    size_ = GetFileSize(g_asfPath21);
    printf("---- %s ----\n", g_asfPath21.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0044
 * @tc.desc: get format when the file is h264_pcm_f64le.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0044, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath21.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath21);
    size_ = GetFileSize(g_asfPath21);
    printf("---- %s ------\n", g_asfPath21.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 5644800);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_F64LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0045
 * @tc.desc: get source format(h264_pcm_mulaw.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0045, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath22.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath22);
    size_ = GetFileSize(g_asfPath22);
    printf("---- %s ----\n", g_asfPath22.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3066000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0046
 * @tc.desc: get format when the file is h264_pcm_mulaw.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0046, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath22.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath22);
    size_ = GetFileSize(g_asfPath22);
    printf("---- %s ------\n", g_asfPath22.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.sampleRate, 8000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 64000);
    ASSERT_EQ(formatVal_.codecMime, "audio/g711mu");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0047
 * @tc.desc: get source format(h264_pcm_s16le.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0047, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath23.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath23);
    size_ = GetFileSize(g_asfPath23);
    printf("---- %s ----\n", g_asfPath23.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0048
 * @tc.desc: get format when the file is h264_pcm_s16le.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0048, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath23.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath23);
    size_ = GetFileSize(g_asfPath23);
    printf("---- %s ------\n", g_asfPath23.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0049
 * @tc.desc: get source format(h264_pcm_s24le.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0049, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath24.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath24);
    size_ = GetFileSize(g_asfPath24);
    printf("---- %s ----\n", g_asfPath24.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0050
 * @tc.desc: get format when the file is h264_pcm_s24le.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0050, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath24.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath24);
    size_ = GetFileSize(g_asfPath24);
    printf("---- %s ------\n", g_asfPath24.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 2116800);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S24LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0051
 * @tc.desc: get source format(h264_pcm_s32le.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0051, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath25.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath25);
    size_ = GetFileSize(g_asfPath25);
    printf("---- %s ----\n", g_asfPath25.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0052
 * @tc.desc: get format when the file is h264_pcm_s32le.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0052, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath25.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath25);
    size_ = GetFileSize(g_asfPath25);
    printf("---- %s ------\n", g_asfPath25.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 2822400);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S32LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0053
 * @tc.desc: get source format(h264_pcm_s64le.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0053, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath26.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath26);
    size_ = GetFileSize(g_asfPath26);
    printf("---- %s ----\n", g_asfPath26.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0054
 * @tc.desc: get format when the file is h264_pcm_s64le.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0054, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath26.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath26);
    size_ = GetFileSize(g_asfPath26);
    printf("---- %s ------\n", g_asfPath26.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 5644800);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S64LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0055
 * @tc.desc: get source format(h264_pcm_u8.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0055, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath27.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath27);
    size_ = GetFileSize(g_asfPath27);
    printf("---- %s ----\n", g_asfPath27.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0056
 * @tc.desc: get format when the file is h264_pcm_u8.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0056, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath27.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath27);
    size_ = GetFileSize(g_asfPath27);
    printf("---- %s ------\n", g_asfPath27.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.bitRate, 705600);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_U8);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0057
 * @tc.desc: get source format(h264_vorbis.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0057, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath28.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath28);
    size_ = GetFileSize(g_asfPath28);
    printf("---- %s ----\n", g_asfPath28.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3100000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0058
 * @tc.desc: get format when the file is h264_vorbis.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0058, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath28.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath28);
    size_ = GetFileSize(g_asfPath28);
    printf("---- %s ------\n", g_asfPath28.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.codecMime, "audio/vorbis");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0059
 * @tc.desc: get source format(h264_wmav1.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0059, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath29.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath29);
    size_ = GetFileSize(g_asfPath29);
    printf("---- %s ----\n", g_asfPath29.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3113000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0060
 * @tc.desc: get format when the file is h264_wmav1.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0060, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath29.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath29);
    size_ = GetFileSize(g_asfPath29);
    printf("---- %s ------\n", g_asfPath29.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/wmav1");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0061
 * @tc.desc: get source format(h264_wmav2.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0061, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath30.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath30);
    size_ = GetFileSize(g_asfPath30);
    printf("---- %s ----\n", g_asfPath30.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3113000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0062
 * @tc.desc: get format when the file is h264_wmav2.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0062, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath30.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath30);
    size_ = GetFileSize(g_asfPath30);
    printf("---- %s ------\n", g_asfPath30.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/avc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 62.500000);
    trackIndex_ = 1;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/wmav2");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0063
 * @tc.desc: get source format(mjpeg_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0063, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath31.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath31);
    size_ = GetFileSize(g_asfPath31);
    printf("---- %s ----\n", g_asfPath31.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3124000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0064
 * @tc.desc: get format when the file is mjpeg_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0064, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath31.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath31);
    size_ = GetFileSize(g_asfPath31);
    printf("---- %s ------\n", g_asfPath31.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "image/jpeg");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0065
 * @tc.desc: get source format(mpeg2_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0065, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath32.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath32);
    size_ = GetFileSize(g_asfPath32);
    printf("---- %s ----\n", g_asfPath32.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3090000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0066
 * @tc.desc: get format when the file is mpeg2_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0066, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath32.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath32);
    size_ = GetFileSize(g_asfPath32);
    printf("---- %s ------\n", g_asfPath32.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mpeg2");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0067
 * @tc.desc: get source format(mpeg4_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0067, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath33.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath33);
    size_ = GetFileSize(g_asfPath33);
    printf("---- %s ----\n", g_asfPath33.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3124000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0068
 * @tc.desc: get format when the file is mpeg4_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0068, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath33.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath33);
    size_ = GetFileSize(g_asfPath33);
    printf("---- %s ------\n", g_asfPath33.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/mp4v-es");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0069
 * @tc.desc: get source format(msvideo1_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0069, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath34.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath34);
    size_ = GetFileSize(g_asfPath34);
    printf("---- %s ----\n", g_asfPath34.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3124000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0070
 * @tc.desc: get format when the file is msvideo1_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0070, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath34.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath34);
    size_ = GetFileSize(g_asfPath34);
    printf("---- %s ------\n", g_asfPath34.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/msvideo1");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0071
 * @tc.desc: get source format(vp9_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0071, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath35.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath35);
    size_ = GetFileSize(g_asfPath35);
    printf("---- %s ----\n", g_asfPath35.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3124000);
    ASSERT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0072
 * @tc.desc: get format when the file is vp9_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0072, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath35.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath35);
    size_ = GetFileSize(g_asfPath35);
    printf("---- %s ------\n", g_asfPath35.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/vp9");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 480);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
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
 * @tc.name: AVSource_ASF_GetFormat_0073
 * @tc.desc: get source format(av1_mp3.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0073, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath37.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath37);
    size_ = GetFileSize(g_asfPath37);
    printf("---- %s ----\n", g_asfPath37.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 3084000);
    EXPECT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0074
 * @tc.desc: get format when the file is av1_mp3.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0074, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath37.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath37);
    size_ = GetFileSize(g_asfPath37);
    printf("---- %s ------\n", g_asfPath37.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/vp9");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0075
 * @tc.desc: get source format(DVCNTSC_720x480_25_422_dv5n.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0075, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath38.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath38);
    size_ = GetFileSize(g_asfPath38);
    printf("---- %s ----\n", g_asfPath38.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 1000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 0);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0076
 * @tc.desc: get format when the file is DVCNTSC_720x480_25_422_dv5n.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0076, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath38.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath38);
    size_ = GetFileSize(g_asfPath38);
    printf("---- %s ------\n", g_asfPath38.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/dvvideo");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 29.970029970029969);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0077
 * @tc.desc: get source format(wmv3_wmapro.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0077, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath39.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath39);
    size_ = GetFileSize(g_asfPath39);
    printf("---- %s ----\n", g_asfPath39.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 4999000);
    EXPECT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0078
 * @tc.desc: get format when the file is wmv3_wmapro.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0078, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath39.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath39);
    size_ = GetFileSize(g_asfPath39);
    printf("---- %s ------\n", g_asfPath39.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/wmv3");
    EXPECT_EQ(formatVal_.width, 1280);
    EXPECT_EQ(formatVal_.height, 720);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 24.000000);
    trackIndex_ = 1;
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 6);
    EXPECT_EQ(formatVal_.bitRate, 384000);
    EXPECT_EQ(formatVal_.codecMime, "audio/wmapro");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 63);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0079
 * @tc.desc: get source format(DVCPAL_720x576_25_411_dvpp.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0079, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath40.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath40);
    size_ = GetFileSize(g_asfPath40);
    printf("---- %s ----\n", g_asfPath40.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 1080000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 0);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0080
 * @tc.desc: get format when the file is DVCPAL_720x576_25_411_dvpp.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0080, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath40.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath40);
    size_ = GetFileSize(g_asfPath40);
    printf("---- %s ------\n", g_asfPath40.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/dvvideo");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 576);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0081
 * @tc.desc: get source format(DVCPROHD_1280x1080_29_422_dvh6.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0081, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath41.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath41);
    size_ = GetFileSize(g_asfPath41);
    printf("---- %s ----\n", g_asfPath41.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 1001000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 0);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0082
 * @tc.desc: get format when the file is DVCPROHD_1280x1080_29_422_dvh6.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0082, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath41.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath41);
    size_ = GetFileSize(g_asfPath41);
    printf("---- %s ------\n", g_asfPath41.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/dvvideo");
    EXPECT_EQ(formatVal_.width, 1280);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 29.970029970029969);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0083
 * @tc.desc: get source format(vc1.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0083, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath42.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath42);
    size_ = GetFileSize(g_asfPath42);
    printf("---- %s ----\n", g_asfPath42.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 1001000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 0);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0084
 * @tc.desc: get format when the file is vc1.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0084, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath42.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath42);
    size_ = GetFileSize(g_asfPath42);
    printf("---- %s ------\n", g_asfPath42.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "invalid");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 59.940059940059939);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0085
 * @tc.desc: get source format(vp8_aac.asf)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0085, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath43.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath43);
    size_ = GetFileSize(g_asfPath43);
    printf("---- %s ----\n", g_asfPath43.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 3169000);
    EXPECT_EQ(formatVal_.trackCount, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 1);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::WMV));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0086
 * @tc.desc: get format when the file is vp8_aac.asf
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0086, TestSize.Level1)
{
    ASSERT_EQ(access(g_asfPath43.c_str(), F_OK), 0);
    fd_ = OpenFile(g_asfPath43);
    size_ = GetFileSize(g_asfPath43);
    printf("---- %s ------\n", g_asfPath43.c_str());
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
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/vp8");
    EXPECT_EQ(formatVal_.width, 176);
    EXPECT_EQ(formatVal_.height, 144);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 29.970029970029969);
    trackIndex_ = 1;
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_ASF_GetFormat_0087
 * @tc.desc: create source with fd, but file is error
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_ASF_GetFormat_0087, TestSize.Level1)
{
    printf("---- %s ----\n", g_asfPath36.c_str());
    fd_ = OpenFile(g_asfPath36);
    size_ = GetFileSize(g_asfPath36);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
}
} // namespace