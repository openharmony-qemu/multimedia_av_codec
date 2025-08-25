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
static const string TEST_TIMED_METADATA = "com.openharmony.timed_metadata.test";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";

const int64_t SOURCE_OFFSET = 0;

string g_mtsPath1 = TEST_FILE_PATH + string("mpeg2.mts");
string g_mtsUri1 = TEST_URI_PATH + string("mpeg2.mts");
string g_mtsPath2 = TEST_FILE_PATH + string("mpeg4.mts");
string g_mtsUri2 = TEST_URI_PATH + string("mpeg4.mts");
string g_mtsPath3 = TEST_FILE_PATH + string("h264_mp3.mts");
string g_mtsUri3 = TEST_URI_PATH + string("h264_mp3.mts");
string g_mtsPath4 = TEST_FILE_PATH + string("hevc_aac.mts");
string g_mtsUri4 = TEST_URI_PATH + string("hevc_aac.mts");
string g_m2tsPath1 = TEST_FILE_PATH + string("mpeg2.m2ts");
string g_m2tsUri1 = TEST_URI_PATH + string("mpeg2.m2ts");
string g_m2tsPath2 = TEST_FILE_PATH + string("h264_mp3.m2ts");
string g_m2tsUri2 = TEST_URI_PATH + string("h264_mp3.m2ts");
string g_m2tsPath3 = TEST_FILE_PATH + string("hevc_aac.m2ts");
string g_m2tsUri3 = TEST_URI_PATH + string("hevc_aac.m2ts");
string g_trpPath1 = TEST_FILE_PATH + string("mpeg2.trp");
string g_trpUri1 = TEST_URI_PATH + string("mpeg2.trp");
string g_trpPath2 = TEST_FILE_PATH + string("mpeg4.trp");
string g_trpUri2 = TEST_URI_PATH + string("mpeg4.trp");
string g_trpPath3 = TEST_FILE_PATH + string("h264_mp3.trp");
string g_trpUri3 = TEST_URI_PATH + string("h264_mp3.trp");
string g_trpPath4 = TEST_FILE_PATH + string("hevc_aac.trp");
string g_trpUri4 = TEST_URI_PATH + string("hevc_aac.trp");
} //namespace

/**********************************source FD**************************************/

namespace {
/**
 * @tc.name: AVSource_GetFormat_2400
 * @tc.desc: get source format when the file is mts(mpeg2), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2400, TestSize.Level1)
{
    fd_ = OpenFile(g_mtsUri1);
    size_ = GetFileSize(g_mtsUri1);
    printf("---- %s ----\n", g_mtsUri1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mtsUri1.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2401
 * @tc.desc: get source format when the file is mts(mpeg2), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2401, TestSize.Level1)
{
    fd_ = OpenFile(g_mtsPath1);
    size_ = GetFileSize(g_mtsPath1);
    printf("---- %s ----\n", g_mtsPath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2402
 * @tc.desc: get source format when the file is mts(mpeg4), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2402, TestSize.Level1)
{
    fd_ = OpenFile(g_mtsUri2);
    size_ = GetFileSize(g_mtsUri2);
    printf("---- %s ----\n", g_mtsUri2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mtsUri2.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2403
 * @tc.desc: get source format when the file is mts(mpeg4), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2403, TestSize.Level1)
{
    fd_ = OpenFile(g_mtsPath2);
    size_ = GetFileSize(g_mtsPath2);
    printf("---- %s ----\n", g_mtsPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2404
 * @tc.desc: get source format when the file is mts(h264, mp3), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2404, TestSize.Level1)
{
    fd_ = OpenFile(g_mtsUri3);
    size_ = GetFileSize(g_mtsUri3);
    printf("---- %s ----\n", g_mtsUri3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mtsUri3.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10056355);
    EXPECT_EQ(formatVal_.trackCount, 2);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
        formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2405
 * @tc.desc: get source format when the file is mts(h264, mp3), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2405, TestSize.Level1)
{
    fd_ = OpenFile(g_mtsPath3);
    size_ = GetFileSize(g_mtsPath3);
    printf("---- %s ----\n", g_mtsPath3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10056355);
    EXPECT_EQ(formatVal_.trackCount, 2);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
        formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2406
 * @tc.desc: get source format when the file is mts(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2406, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        fd_ = OpenFile(g_mtsUri4);
        size_ = GetFileSize(g_mtsUri4);
        printf("---- %s ----\n", g_mtsUri4.c_str());
        source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mtsUri4.c_str()));
        EXPECT_NE(source_, nullptr);
        format_ = source_->GetSourceFormat();
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
        EXPECT_EQ(formatVal_.duration, 10156556);
        EXPECT_EQ(formatVal_.trackCount, 2);
        trackIndex_ = 0;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
        EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
        EXPECT_EQ(formatVal_.codecMime, "video/hevc");
        EXPECT_EQ(formatVal_.width, 1920);
        EXPECT_EQ(formatVal_.height, 1080);
        EXPECT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
        trackIndex_ = 1;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
            formatVal_.audioSampleFormat));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
        EXPECT_EQ(formatVal_.sampleRate, 44100);
        EXPECT_EQ(formatVal_.channelCount, 2);
        EXPECT_EQ(formatVal_.bitRate, 100192);
        EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
        EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
        EXPECT_EQ(formatVal_.channelLayout, 3);
    }
}

/**
 * @tc.name: AVSource_GetFormat_2407
 * @tc.desc: get source format when the file is mts(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2407, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        fd_ = OpenFile(g_mtsPath4);
        size_ = GetFileSize(g_mtsPath4);
        printf("---- %s ----\n", g_mtsPath4.c_str());
        source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
        EXPECT_NE(source_, nullptr);
        format_ = source_->GetSourceFormat();
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
        EXPECT_EQ(formatVal_.duration, 10156556);
        EXPECT_EQ(formatVal_.trackCount, 2);
        trackIndex_ = 0;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
        EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
        EXPECT_EQ(formatVal_.codecMime, "video/hevc");
        EXPECT_EQ(formatVal_.width, 1920);
        EXPECT_EQ(formatVal_.height, 1080);
        EXPECT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
        trackIndex_ = 1;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
            formatVal_.audioSampleFormat));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
        EXPECT_EQ(formatVal_.sampleRate, 44100);
        EXPECT_EQ(formatVal_.channelCount, 2);
        EXPECT_EQ(formatVal_.bitRate, 100192);
        EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
        EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
        EXPECT_EQ(formatVal_.channelLayout, 3);
    }
}

/**
 * @tc.name: AVSource_GetFormat_2408
 * @tc.desc: get source format when the file is m2ts(mpeg2), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2408, TestSize.Level1)
{
    fd_ = OpenFile(g_m2tsUri1);
    size_ = GetFileSize(g_m2tsUri1);
    printf("---- %s ----\n", g_m2tsUri1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m2tsUri1.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2409
 * @tc.desc: get source format when the file is m2ts(mpeg2), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2409, TestSize.Level1)
{
    fd_ = OpenFile(g_m2tsPath1);
    size_ = GetFileSize(g_m2tsPath1);
    printf("---- %s ----\n", g_m2tsPath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2410
 * @tc.desc: get source format when the file is m2ts(h264, mp3), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2410, TestSize.Level1)
{
    fd_ = OpenFile(g_m2tsUri2);
    size_ = GetFileSize(g_m2tsUri2);
    printf("---- %s ----\n", g_m2tsUri2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m2tsUri2.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10056355);
    EXPECT_EQ(formatVal_.trackCount, 2);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
        formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2411
 * @tc.desc: get source format when the file is m2ts(h264, mp3), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2411, TestSize.Level1)
{
    fd_ = OpenFile(g_m2tsPath2);
    size_ = GetFileSize(g_m2tsPath2);
    printf("---- %s ----\n", g_m2tsPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10056355);
    EXPECT_EQ(formatVal_.trackCount, 2);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
        formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2412
 * @tc.desc: get source format when the file is m2ts(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2412, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        fd_ = OpenFile(g_m2tsUri3);
        size_ = GetFileSize(g_m2tsUri3);
        printf("---- %s ----\n", g_m2tsUri3.c_str());
        source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m2tsUri3.c_str()));
        EXPECT_NE(source_, nullptr);
        format_ = source_->GetSourceFormat();
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
        EXPECT_EQ(formatVal_.duration, 10156556);
        EXPECT_EQ(formatVal_.trackCount, 2);
        trackIndex_ = 0;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
        EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
        EXPECT_EQ(formatVal_.codecMime, "video/hevc");
        EXPECT_EQ(formatVal_.width, 1920);
        EXPECT_EQ(formatVal_.height, 1080);
        EXPECT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
        trackIndex_ = 1;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
            formatVal_.audioSampleFormat));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
        EXPECT_EQ(formatVal_.sampleRate, 44100);
        EXPECT_EQ(formatVal_.channelCount, 2);
        EXPECT_EQ(formatVal_.bitRate, 100192);
        EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
        EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
        EXPECT_EQ(formatVal_.channelLayout, 3);
    }
}

/**
 * @tc.name: AVSource_GetFormat_2413
 * @tc.desc: get source format when the file is m2ts(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2413, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        fd_ = OpenFile(g_m2tsPath3);
        size_ = GetFileSize(g_m2tsPath3);
        printf("---- %s ----\n", g_m2tsPath3.c_str());
        source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
        EXPECT_NE(source_, nullptr);
        format_ = source_->GetSourceFormat();
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
        EXPECT_EQ(formatVal_.duration, 10156556);
        EXPECT_EQ(formatVal_.trackCount, 2);
        trackIndex_ = 0;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
        EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
        EXPECT_EQ(formatVal_.codecMime, "video/hevc");
        EXPECT_EQ(formatVal_.width, 1920);
        EXPECT_EQ(formatVal_.height, 1080);
        EXPECT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
        trackIndex_ = 1;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
            formatVal_.audioSampleFormat));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
        EXPECT_EQ(formatVal_.sampleRate, 44100);
        EXPECT_EQ(formatVal_.channelCount, 2);
        EXPECT_EQ(formatVal_.bitRate, 100192);
        EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
        EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
        EXPECT_EQ(formatVal_.channelLayout, 3);
    }
}

/**
 * @tc.name: AVSource_GetFormat_2414
 * @tc.desc: get source format when the file is trp(mpeg2), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2414, TestSize.Level1)
{
    fd_ = OpenFile(g_trpUri1);
    size_ = GetFileSize(g_trpUri1);
    printf("---- %s ----\n", g_trpUri1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_trpUri1.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2415
 * @tc.desc: get source format when the file is trp(mpeg2), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2415, TestSize.Level1)
{
    fd_ = OpenFile(g_trpPath1);
    size_ = GetFileSize(g_trpPath1);
    printf("---- %s ----\n", g_trpPath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mpeg2");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2416
 * @tc.desc: get source format when the file is trp(mpeg4), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2416, TestSize.Level1)
{
    fd_ = OpenFile(g_trpUri2);
    size_ = GetFileSize(g_trpUri2);
    printf("---- %s ----\n", g_trpUri2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_trpUri2.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2417
 * @tc.desc: get source format when the file is trp(mpeg4), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2417, TestSize.Level1)
{
    fd_ = OpenFile(g_trpPath2);
    size_ = GetFileSize(g_trpPath2);
    printf("---- %s ----\n", g_trpPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/mp4v-es");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 25.000000);
}

/**
 * @tc.name: AVSource_GetFormat_2418
 * @tc.desc: get source format when the file is trp(h264, mp3), uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2418, TestSize.Level1)
{
    fd_ = OpenFile(g_trpUri3);
    size_ = GetFileSize(g_trpUri3);
    printf("---- %s ----\n", g_trpUri3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_trpUri3.c_str()));
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10056355);
    EXPECT_EQ(formatVal_.trackCount, 2);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
        formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2419
 * @tc.desc: get source format when the file is trp(h264, mp3), fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2419, TestSize.Level1)
{
    fd_ = OpenFile(g_trpPath3);
    size_ = GetFileSize(g_trpPath3);
    printf("---- %s ----\n", g_trpPath3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    EXPECT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 10056355);
    EXPECT_EQ(formatVal_.trackCount, 2);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_DOUBLE_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    EXPECT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
        formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128000);
    EXPECT_EQ(formatVal_.codecMime, "audio/mpeg");
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_GetFormat_2420
 * @tc.desc: get source format when the file is trp(hevc, aac), Uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2420, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        fd_ = OpenFile(g_trpUri4);
        size_ = GetFileSize(g_trpUri4);
        printf("---- %s ----\n", g_trpUri4.c_str());
        source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_trpUri4.c_str()));
        EXPECT_NE(source_, nullptr);
        format_ = source_->GetSourceFormat();
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
        EXPECT_EQ(formatVal_.duration, 10156556);
        EXPECT_EQ(formatVal_.trackCount, 2);
        trackIndex_ = 0;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
        EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
        EXPECT_EQ(formatVal_.codecMime, "video/hevc");
        EXPECT_EQ(formatVal_.width, 1920);
        EXPECT_EQ(formatVal_.height, 1080);
        EXPECT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
        trackIndex_ = 1;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
            formatVal_.audioSampleFormat));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
        EXPECT_EQ(formatVal_.sampleRate, 44100);
        EXPECT_EQ(formatVal_.channelCount, 2);
        EXPECT_EQ(formatVal_.bitRate, 100192);
        EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
        EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
        EXPECT_EQ(formatVal_.channelLayout, 3);
    }
}

/**
 * @tc.name: AVSource_GetFormat_2421
 * @tc.desc: get source format when the file is trp(hevc, aac), Fd
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_2421, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        fd_ = OpenFile(g_trpPath4);
        size_ = GetFileSize(g_trpPath4);
        printf("---- %s ----\n", g_trpPath4.c_str());
        source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
        EXPECT_NE(source_, nullptr);
        format_ = source_->GetSourceFormat();
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
        EXPECT_EQ(formatVal_.duration, 10156556);
        EXPECT_EQ(formatVal_.trackCount, 2);
        trackIndex_ = 0;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
        EXPECT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
        EXPECT_EQ(formatVal_.codecMime, "video/hevc");
        EXPECT_EQ(formatVal_.width, 1920);
        EXPECT_EQ(formatVal_.height, 1080);
        EXPECT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
        trackIndex_ = 1;
        format_ = source_->GetTrackFormat(trackIndex_);
        EXPECT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
        EXPECT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT,
            formatVal_.audioSampleFormat));
        EXPECT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
        EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
        EXPECT_EQ(formatVal_.sampleRate, 44100);
        EXPECT_EQ(formatVal_.channelCount, 2);
        EXPECT_EQ(formatVal_.bitRate, 100192);
        EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
        EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
        EXPECT_EQ(formatVal_.channelLayout, 3);
    }
}
} // namespace