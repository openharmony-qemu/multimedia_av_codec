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

string g_cafPcmPath = TEST_FILE_PATH + string("caf_pcm_s16le.caf");
string g_cafPcmUri = TEST_URI_PATH + string("caf_pcm_s16le.caf");
string g_cafAlacPath = TEST_FILE_PATH + string("caf_alac.caf");
string g_cafAlacUri = TEST_URI_PATH + string("caf_alac.caf");
string g_cafOpusPath = TEST_FILE_PATH + string("caf_opus.caf");
string g_cafOpusUri = TEST_URI_PATH + string("caf_opus.caf");

/**
 * @tc.name: AVSource_CAF_GetFormat_0001
 * @tc.desc: get source format when the file is CAF(PCM_S16LE). Use FD mode.
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetFormat_0001, TestSize.Level1)
{
    fd_ = OpenFile(g_cafPcmPath);
    size_ = GetFileSize(g_cafPcmPath);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr) << "Create CAF(PCM) source with FD failed";

    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr) <<"Get CAF(PCM) source format failed";
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());

    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr) << "Get CAF(PCM) track 0 format failed";
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());

    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));

    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD) << "Track type is not audio";
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.sampleRate, 44100) << "Sample rate should be 44100Hz";
    ASSERT_EQ(formatVal_.channelCount, 2) << "Channel count should be 2(stereo)";
    ASSERT_EQ(formatVal_.bitRate, 1411200) << "Bitrate should be 1411200bps";
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_S16LE) << "Sample format not S16LE";
    ASSERT_EQ(formatVal_.channelLayout, 3) << "Channel layout should be stereo(3)";

    format_->Destroy();
}

/**
 * @tc.name: AVSource_CAF_GetFormat_0002
 * @tc.desc: get source format when the file is CAF(PCM_S16LE). Use URI mode.
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetFormat_0002, TestSize.Level1)
{
    printf("---- %s ------\n", g_cafPcmUri.data());
    
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_cafPcmUri.data()));
    ASSERT_NE(source_, nullptr) << "Create CAF(PCM) source with URI failed";

    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr) << "Get CAF(PCM) source format failed";
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());

    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr) << "Get CAF(PCM) track 0 format failed";
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());

    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));

    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.codecMime, "audio/raw");
    ASSERT_EQ(formatVal_.sampleRate, 44100);

    format_->Destroy();
}

/**
 * @tc.name: AVSource_CAF_GetFormat_0003
 * @tc.desc: get source format when the file is CAF(ALAC). Use FD mode.
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetFormat_0003, TestSize.Level1)
{
    fd_ = OpenFile(g_cafAlacPath);
    size_ = GetFileSize(g_cafAlacPath);
    printf("---- %s ----\n", g_cafAlacPath.c_str());
    
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr) << "Create CAF(ALAC) source with FD failed";

    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr) << "Get CAF(ALAC) source format failed";
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());

    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr) << "Get CAF(ALAC) track 0 format failed";
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());

    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));

    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.codecMime, "audio/alac") << "Codec mime not match ALAC";
    ASSERT_EQ(formatVal_.sampleRate, 44100) << "Sample rate should be 44100Hz";
    ASSERT_EQ(formatVal_.channelCount, 2) << "Channel count should be 2(stereo)";
    ASSERT_GE(formatVal_.bitRate, 700000) << "ALAC bitrate should be ≥700kbps";
    ASSERT_LE(formatVal_.bitRate, 900000) << "ALAC bitrate should be ≤900kbps";

    format_->Destroy();
}

/**
 * @tc.name: AVSource_CAF_GetFormat_0004
 * @tc.desc: get source format when the file is CAF(ALAC). Use URI mode.
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetFormat_0004, TestSize.Level1)
{
    printf("---- %s ------\n", g_cafAlacUri.data());
    
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_cafAlacUri.data()));
    ASSERT_NE(source_, nullptr) << "Create CAF(ALAC) source with URI failed";

    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr) << "Get CAF(ALAC) source format failed";
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());

    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr) << "Get CAF(ALAC) track 0 format failed";
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());

    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));

    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.codecMime, "audio/alac");
    ASSERT_EQ(formatVal_.sampleRate, 44100);

    format_->Destroy();
}

/**
 * @tc.name: AVSource_CAF_GetFormat_0005
 * @tc.desc: get source format when the file is CAF(Opus). Use FD mode.
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetFormat_0005, TestSize.Level1)
{
    fd_ = OpenFile(g_cafOpusPath);
    size_ = GetFileSize(g_cafOpusPath);
    printf("---- %s ----\n", g_cafOpusPath.c_str());
    
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr) << "Create CAF(Opus) source with FD failed";

    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr) << "Get CAF(Opus) source format failed";
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());

    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr) << "Get CAF(Opus) track 0 format failed";
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());

    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_FALSE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, formatVal_.bitRate));

    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.codecMime, "audio/opus") << "Codec mime not match Opus";
    ASSERT_EQ(formatVal_.sampleRate, 48000) << "Sample rate should be 48000Hz";
    ASSERT_EQ(formatVal_.channelCount, 2) << "Channel count should be 2(stereo)";
    ASSERT_EQ(formatVal_.bitRate, 0);

    format_->Destroy();
}

/**
 * @tc.name: AVSource_CAF_GetFormat_0006
 * @tc.desc: get source format when the file is CAF(Opus). Use URI mode.
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetFormat_0006, TestSize.Level1)
{
    printf("---- %s ------\n", g_cafOpusUri.data());
    
    source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char *>(g_cafOpusUri.data()));
    ASSERT_NE(source_, nullptr) << "Create CAF(Opus) source with URI failed";

    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr) << "Get CAF(Opus) source format failed";
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());

    trackIndex_ = 0;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr) << "Get CAF(Opus) track 0 format failed";
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());

    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));

    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    ASSERT_EQ(formatVal_.codecMime, "audio/opus");
    ASSERT_EQ(formatVal_.sampleRate, 48000) << "Sample rate should be 48000Hz";

    format_->Destroy();
}

/**
 * @tc.name: AVSource_CAF_GetFormat_0007
 * @tc.desc: get invalid track index format for CAF file
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetFormat_0007, TestSize.Level1)
{
    fd_ = OpenFile(g_cafOpusPath);
    size_ = GetFileSize(g_cafOpusPath);
    printf("---- %s ----\n", g_cafOpusPath.c_str());
    
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);

    trackIndex_ = 1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_EQ(format_, nullptr) << "Get invalid track index should return nullptr";

    trackIndex_ = -1;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_EQ(format_, nullptr) << "Get negative track index should return nullptr";
}

/**
 * @tc.name: AVSource_CAF_GetSourceFormat_0008
 * @tc.desc: verify CAF source format file type
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_CAF_GetSourceFormat_0008, TestSize.Level1)
{
    fd_ = OpenFile(g_cafPcmPath);
    size_ = GetFileSize(g_cafPcmPath);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 117) << "CAF source file type identifier mismatch";
    format_->Destroy();

    fd_ = OpenFile(g_cafOpusPath);
    size_ = GetFileSize(g_cafOpusPath);
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.fileType, 117) << "CAF source file type identifier mismatch";
    format_->Destroy();
}

} // namespace