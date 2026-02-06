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
const int64_t SOURCE_OFFSET = 0;

string g_auFileName = "au_48000_1_cover.au";

string g_auFilePath = TEST_FILE_PATH + g_auFileName;
string g_auUriPath = TEST_URI_PATH + g_auFileName;

string g_sunAuErrorPath = TEST_FILE_PATH + string("test_error.au");
string g_pcmAlawSunAuPath = TEST_FILE_PATH + string("au_pcm_alaw.au");
string g_pcmMulawSunAuPath = TEST_FILE_PATH + string("au_pcm_mulaw.au");
string g_pcmS16beSunAuPath = TEST_FILE_PATH + string("au_pcm_s16be.au");
string g_pcmAlawSunAuUri = TEST_URI_PATH + string("au_pcm_alaw.au");
string g_pcmF64beSunAuPath = TEST_FILE_PATH + string("au_pcm_f64be.au");
string g_adpcmG722SunAuPath = TEST_FILE_PATH + string("au_adpcm_g722.au");
string g_adpcmG726leSunAuPath = TEST_FILE_PATH + string("au_adpcm_g726le.au");
string g_pcmF32beSunAuPath = TEST_FILE_PATH + string("au_pcm_f32be.au");
string g_pcmS8SunAuPath = TEST_FILE_PATH + string("au_pcm_s8.au");
string g_pcmS24beSunAuPath = TEST_FILE_PATH + string("au_pcm_s24be.au");
string g_pcmS32beSunAuPath = TEST_FILE_PATH + string("au_pcm_s32be.au");

/**
 * @tc.name: AVSource_AU_GetFormat_0001
 * @tc.desc: get source format(au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0001, TestSize.Level1)
{
    ASSERT_EQ(access(g_auFilePath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_auFilePath);
    size_ = GetFileSize(g_auFilePath);
    printf("---- %s ----\n", g_auFilePath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0002
 * @tc.desc: get format when the file is au
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0002, TestSize.Level1)
{
    ASSERT_EQ(access(g_auFilePath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_auFilePath);
    size_ = GetFileSize(g_auFilePath);
    printf("---- %s ------\n", g_auFilePath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 768000);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.channelLayout, 4);
}


// Uri Path
/**
 * @tc.name: AVSource_AU_GetFormat_0003
 * @tc.desc: get source format(au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0003, TestSize.Level1)
{
    printf("---- %s ----\n", g_auUriPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_auUriPath.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 30000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0004
 * @tc.desc: get format when the file is au
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0004, TestSize.Level1)
{
    printf("---- %s ------\n", g_auUriPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_auUriPath.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 768000);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.channelLayout, 4);
}

/**
 * @tc.name: AVSource_CreateSourceWithFD_AU_0001
 * @tc.desc: create source with fd, but file is error
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CreateSourceWithFD_AU_0001, TestSize.Level1)
{
    printf("---- %s ----\n", g_sunAuErrorPath.c_str());
    fd_ = OpenFile(g_sunAuErrorPath);
    size_ = GetFileSize(g_sunAuErrorPath);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_EQ(source_, nullptr);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0005
 * @tc.desc: get source format(pcm_alaw.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0005, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmAlawSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmAlawSunAuPath);
    size_ = GetFileSize(g_pcmAlawSunAuPath);
    printf("---- %s ----\n", g_pcmAlawSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3018594);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}


/**
 * @tc.name: AVSource_AU_GetFormat_0006
 * @tc.desc: get source format(pcm_mulaw.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0006, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmMulawSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmMulawSunAuPath);
    size_ = GetFileSize(g_pcmMulawSunAuPath);
    printf("---- %s ----\n", g_pcmMulawSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3018594);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/g711mu");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0007
 * @tc.desc: get source format(pcm_s16be.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0007, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmS16beSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmS16beSunAuPath);
    size_ = GetFileSize(g_pcmS16beSunAuPath);
    printf("---- %s ----\n", g_pcmS16beSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3018594);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16BE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}


/**
 * @tc.name: AVSource_AU_GetFormat_0008
 * @tc.desc: get source format(pcm_alaw.au url)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0008, TestSize.Level1)
{
    InitResource(g_pcmAlawSunAuUri, URI);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 3018594);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0009
 * @tc.desc: get source format(au_pcm_f64be.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0009, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmF64beSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmF64beSunAuPath);
    size_ = GetFileSize(g_pcmF64beSunAuPath);
    printf("---- %s ----\n", g_pcmF64beSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 5000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 3072000);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_F64BE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0010
 * @tc.desc: get source format(au_adpcm_g722.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0010, TestSize.Level1)
{
    ASSERT_EQ(access(g_adpcmG722SunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_adpcmG722SunAuPath);
    size_ = GetFileSize(g_adpcmG722SunAuPath);
    printf("---- %s ----\n", g_adpcmG722SunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 5000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 192000);
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_g722");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0011
 * @tc.desc: get source format(au_adpcm_g726le.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0011, TestSize.Level1)
{
    ASSERT_EQ(access(g_adpcmG726leSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_adpcmG726leSunAuPath);
    size_ = GetFileSize(g_adpcmG726leSunAuPath);
    printf("---- %s ----\n", g_adpcmG726leSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 5000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_g726le");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0012
 * @tc.desc: get source format(au_pcm_f32be.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0012, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmF32beSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmF32beSunAuPath);
    size_ = GetFileSize(g_pcmF32beSunAuPath);
    printf("---- %s ----\n", g_pcmF32beSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 5000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 1536000);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_F32BE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0013
 * @tc.desc: get source format(au_pcm_s8.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0013, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmS8SunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmS8SunAuPath);
    size_ = GetFileSize(g_pcmS8SunAuPath);
    printf("---- %s ----\n", g_pcmS8SunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 5000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 384000);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S8);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0014
 * @tc.desc: get source format(au_pcm_s24be.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0014, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmS24beSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmS24beSunAuPath);
    size_ = GetFileSize(g_pcmS24beSunAuPath);
    printf("---- %s ----\n", g_pcmS24beSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 5000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 1152000);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S24BE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AU_GetFormat_0015
 * @tc.desc: get source format(au_pcm_s32be.au)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AU_GetFormat_0015, TestSize.Level1)
{
    ASSERT_EQ(access(g_pcmS32beSunAuPath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_pcmS32beSunAuPath);
    size_ = GetFileSize(g_pcmS32beSunAuPath);
    printf("---- %s ----\n", g_pcmS32beSunAuPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 5000000);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AU));
#endif
    trackIndex_ = 0;
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
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.bitRate, 1536000);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S32BE);
    ASSERT_EQ(formatVal_.channelLayout, 4);
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}
}