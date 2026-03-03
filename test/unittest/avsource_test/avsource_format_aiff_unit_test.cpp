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
#include "meta/audio_types.h"
#include "meta/media_types.h"
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

string g_aiffFileName = "pcmS16be_30s_44.1k.aiff";
string g_aifcFileName = "adpcmImaQt_20s_44.1k.aifc";

string g_aiffFilePath = TEST_FILE_PATH + g_aiffFileName;
string g_aiffUriPath = TEST_URI_PATH + g_aiffFileName;

string g_aifcFilePath = TEST_FILE_PATH + g_aifcFileName;
string g_aifcUriPath = TEST_URI_PATH + g_aifcFileName;

string g_aiffFilePath1 = TEST_FILE_PATH + string("pcm_s16be.aiff");
string g_aiffFilePath2 = TEST_FILE_PATH + string("pcm_s24be.aiff");
string g_aiffUriPath1 = TEST_URI_PATH + string("pcm_s16be.aiff");
string g_aiffUriPath2 = TEST_URI_PATH + string("pcm_s24be.aiff");

string g_aifcFilePath1 = TEST_FILE_PATH + string("pcm_mulaw.aifc");
string g_aifcFilePath2 = TEST_FILE_PATH + string("pcm_alaw.aifc");
string g_aifcFilePath3 = TEST_FILE_PATH + string("aifc_pcm_s16le.aifc");
string g_aifcFilePath4 = TEST_FILE_PATH + string("aifc_pcm_f64be.aifc");
string g_aifcFilePath5 = TEST_FILE_PATH + string("aifc_pcm_f32be.aifc");
string g_aifcFilePath6 = TEST_FILE_PATH + string("aifc_adpcm_ima_ws.aifc");
string g_aifcFilePath7 = TEST_FILE_PATH + string("aifc_adpcm_ima_qt.aifc");
string g_aifcUriPath1 = TEST_URI_PATH + string("pcm_mulaw.aifc");
string g_aifcUriPath2 = TEST_URI_PATH + string("pcm_alaw.aifc");
string g_aifcUriPath3 = TEST_URI_PATH + string("aifc_pcm_s16le.aifc");
string g_aifcUriPath4 = TEST_URI_PATH + string("aifc_pcm_f64be.aifc");
string g_aifcUriPath5 = TEST_URI_PATH + string("aifc_pcm_f32be.aifc");
string g_aifcUriPath6 = TEST_URI_PATH + string("aifc_adpcm_ima_ws.aifc");
string g_aifcUriPath7 = TEST_URI_PATH + string("aifc_adpcm_ima_qt.aifc");

// File Path
/**
 * @tc.name: AVSource_AIFC_GetFormat_0001
 * @tc.desc: get source format(aifc)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFC_GetFormat_0001, TestSize.Level1)
{
    ASSERT_EQ(access(g_aifcFilePath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_aifcFilePath);
    size_ = GetFileSize(g_aifcFilePath);
    printf("---- %s ----\n", g_aifcFilePath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 20001088);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFC_GetFormat_0002
 * @tc.desc: get format when the file is aifc
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFC_GetFormat_0002, TestSize.Level1)
{
    ASSERT_EQ(access(g_aifcFilePath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_aifcFilePath);
    size_ = GetFileSize(g_aifcFilePath);
    printf("---- %s ------\n", g_aifcFilePath.c_str());
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 374850);
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_ima_qt");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_0003
 * @tc.desc: get source format aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_0003, TestSize.Level1)
{
    ASSERT_EQ(access(g_aiffFilePath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_aiffFilePath);
    size_ = GetFileSize(g_aiffFilePath);
    printf("---- %s ----\n", g_aifcFilePath.c_str());
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
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_0004
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_0004, TestSize.Level1)
{
    ASSERT_EQ(access(g_aiffFilePath.c_str(), F_OK), 0);
    fd_ = OpenFile(g_aiffFilePath);
    size_ = GetFileSize(g_aiffFilePath);
    printf("---- %s ------\n", g_aiffFilePath.c_str());
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 1411200);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16BE); // 24
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

// Uri Path
/**
 * @tc.name: AVSource_AIFC_GetFormat_0005
 * @tc.desc: get source format(aifc)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFC_GetFormat_0005, TestSize.Level1)
{
    printf("---- %s ----\n", g_aifcUriPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 20001088);
    ASSERT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 0);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFC_GetFormat_0006
 * @tc.desc: get format when the file is aifc
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFC_GetFormat_0006, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath.data()));
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 374850);
    ASSERT_EQ(formatVal_.codecMime, "audio/adpcm_ima_qt");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_0007
 * @tc.desc: get source format rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_0007, TestSize.Level1)
{
    printf("---- %s ----\n", g_aiffUriPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aiffUriPath.data()));
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
    ASSERT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
#endif
    ASSERT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_0008
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_0008, TestSize.Level1)
{
    printf("---- %s ------\n", g_aiffUriPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aiffUriPath.data()));
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 1411200);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16BE);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_0009
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_0009, TestSize.Level1)
{
    printf("---- %s ------\n", g_aiffFilePath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aiffFilePath1.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 768000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00010
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00010, TestSize.Level1)
{
    printf("---- %s ------\n", g_aiffUriPath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aiffUriPath1.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    EXPECT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 768000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_011
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_011, TestSize.Level1)
{
    printf("---- %s ------\n", g_aiffFilePath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aiffFilePath2.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 1152000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S24BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00012
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00012, TestSize.Level1)
{
    printf("---- %s ------\n", g_aiffUriPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aiffUriPath2.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 1152000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S24BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00013
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00013, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcFilePath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcFilePath1.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 8000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 64000);
    EXPECT_EQ(formatVal_.codecMime, "audio/g711mu");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00014
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00014, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath1.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath1.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 8000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 64000);
    EXPECT_EQ(formatVal_.codecMime, "audio/g711mu");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00015
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00015, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcFilePath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcFilePath2.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 384000);
    EXPECT_EQ(formatVal_.codecMime, "audio/g711a");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00016
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00016, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath2.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath2.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 384000);
    EXPECT_EQ(formatVal_.codecMime, "audio/g711a");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00017
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00017, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcFilePath3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcFilePath3.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 768000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00018
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00018, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath3.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath3.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 768000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00019
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00019, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcFilePath4.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcFilePath4.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 3072000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_F64BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00020
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00020, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath4.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath4.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 3072000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_F64BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00021
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00021, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcFilePath5.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcFilePath5.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 1536000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_F32BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00022
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00022, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath5.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath5.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 1536000);
    EXPECT_EQ(formatVal_.codecMime, "audio/raw");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_F32BE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00023
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00023, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcFilePath6.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcFilePath6.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 192000);
    EXPECT_EQ(formatVal_.codecMime, "audio/adpcm_ima_ws");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00024
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00024, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath6.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath6.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 192000);
    EXPECT_EQ(formatVal_.codecMime, "audio/adpcm_ima_ws");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00025
 * @tc.desc: get format when the file is rm
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00025, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcFilePath7.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcFilePath7.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 204000);
    EXPECT_EQ(formatVal_.codecMime, "audio/adpcm_ima_qt");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16P);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}

/**
 * @tc.name: AVSource_AIFF_GetFormat_00026
 * @tc.desc: get format when the file is aiff
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_AIFF_GetFormat_00026, TestSize.Level1)
{
    printf("---- %s ------\n", g_aifcUriPath7.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_aifcUriPath7.data()));
    ASSERT_NE(source_, nullptr);
    trackIndex_ = 0;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.duration, 5000000);
    EXPECT_EQ(formatVal_.trackCount, 1);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.hasVideo, 0);
    EXPECT_EQ(formatVal_.hasAudio, 1);
    EXPECT_EQ(formatVal_.fileType, static_cast<int>(OHOS::Media::Plugins::FileType::AIFF));
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
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 1);
    EXPECT_EQ(formatVal_.bitRate, 204000);
    EXPECT_EQ(formatVal_.codecMime, "audio/adpcm_ima_qt");
    EXPECT_EQ(formatVal_.audioSampleFormat, OHOS::Media::Plugins::AudioSampleFormat::SAMPLE_S16P);
    EXPECT_EQ(formatVal_.channelLayout, 4);
    EXPECT_EQ(source_->Destroy(), AV_ERR_OK);
}
}