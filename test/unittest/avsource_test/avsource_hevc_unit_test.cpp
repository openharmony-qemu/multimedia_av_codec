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
#include <map>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_audio_common.h"
#include "avcodec_info.h"
#include "avcodec_audio_channel_layout.h"
#include "avcodec_mime_type.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "avsource_unit_test.h"

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
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PLUGIN_PATH) + "/libav_codec_plugin_HevcParser.z.so";
const int64_t SOURCE_OFFSET = 0;
string g_hdrVividPath = TEST_FILE_PATH + string("hdrvivid_720p_2s.mp4");
string g_hdrVividUri = TEST_URI_PATH + string("hdrvivid_720p_2s.mp4");
string g_mp4HevcPath = TEST_FILE_PATH + string("camera_h265_aac_rotate270.mp4");
string g_mp4HevcdUri = TEST_URI_PATH + string("camera_h265_aac_rotate270.mp4");
string g_mkvHevcAccPath = TEST_FILE_PATH + string("h265_aac_4sec.mkv");
string g_mkvHevcAccUri = TEST_URI_PATH + string("h265_aac_4sec.mkv");
string g_mkvAvcOpusPath = TEST_FILE_PATH + string("h264_opus_4sec.mkv");
string g_mkvAvcOpusUri = TEST_URI_PATH + string("h264_opus_4sec.mkv");
string g_mkvAvcMp3Path = TEST_FILE_PATH + string("h264_mp3_4sec.mkv");
string g_mkvAvcMp3Uri = TEST_URI_PATH + string("h264_mp3_4sec.mkv");

struct FormatValue {
    // source format
    string title = "";
    string artist = "";
    string album = "";
    string albumArtist = "";
    string date = "";
    string comment = "";
    string genre = "";
    string copyright = "";
    string description = "";
    string language = "";
    string lyrics = "";
    int64_t duration = 0;
    int32_t trackCount = 0;
    string author = "";
    string composer = "";
    int32_t hasVideo = -1;
    int32_t hasAudio = -1;
    int32_t fileType = 0;
    // track format
    string codecMime = "";
    int32_t trackType = 0;
    int32_t width = 0;
    int32_t height = 0;
    int32_t aacIsAdts = -1;
    int32_t sampleRate = 0;
    int32_t channelCount = 0;
    int64_t bitRate = 0;
    int32_t audioSampleFormat = 0;
    double frameRate = 0;
    int32_t rotationAngle = 0;
    int64_t channelLayout = 0;
    int32_t hdrType = 0;
    int32_t codecProfile = 0;
    int32_t codecLevel = 0;
    int32_t colorPrimaries = 0;
    int32_t transferCharacteristics = 0;
    int32_t rangeFlag = 0;
    int32_t matrixCoefficients = 0;
    int32_t chromaLocation = 0;
    // hevc format
    int32_t profile = 0;
    int32_t level = 0;
    int32_t colorPri = 0;
    int32_t colorTrans = 0;
    int32_t colorMatrix = 0;
    int32_t colorRange = 0;
    int32_t chromaLoc = 0;
    int32_t isHdrVivid = 0;
};

std::map<std::string, std::map<std::string, int32_t>> infoMap = {
    {"hdrVivid", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN_10)},
        {"level", static_cast<int32_t>(OH_HEVCLevel::HEVC_LEVEL_4)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT2020)},
        {"chromaLoc", static_cast<int32_t>(OH_ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"mp4Hevc", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(OH_HEVCLevel::HEVC_LEVEL_31)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT709)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT709)},
        {"chromaLoc", static_cast<int32_t>(OH_ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"mkvHevcAcc", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(OH_HEVCLevel::HEVC_LEVEL_41)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_UNSPECIFIED)},
        {"chromaLoc", static_cast<int32_t>(OH_ChromaLocation::CHROMA_LOC_LEFT)},
    }}
};
} // namespace

void AVSourceUnitTest::InitResource(const std::string &path, bool local)
{
    printf("---- %s ------\n", path.c_str());
    if (local) {
        fd_ = OpenFile(path);
        int64_t size = GetFileSize(path);
        source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
        ASSERT_NE(source_, nullptr);
    } else {
        source_ = AVSourceMockFactory::CreateSourceWithURI(const_cast<char*>(path.data()));
        ASSERT_NE(source_, nullptr);
    }
    FormatValue formatValue;
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, streamsCount_));
    for (int i = 0; i < streamsCount_; i++) {
        format_ = source_->GetTrackFormat(i);
        ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
        if (formatValue.trackType == MediaType::MEDIA_TYPE_VID) {
            vTrackIdx_ = i;
        } else if (formatValue.trackType == MediaType::MEDIA_TYPE_AUD) {
            aTrackIdx_ = i;
        }
    }
}

void AVSourceUnitTest::CheckHevcInfo(const std::string &path, const std::string resName, bool local)
{
    InitResource(path, local);
    FormatValue formatValue;
    for (int i = 0; i < streamsCount_; i++) {
        format_ = source_->GetTrackFormat(i);
        string codec_mime;
        format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codec_mime);
        if (codec_mime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) {
            printf("[trackFormat %d]: %s\n", i, format_->DumpInfo());
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, formatValue.profile));
            ASSERT_EQ(formatValue.profile, infoMap[resName]["profile"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_LEVEL, formatValue.level));
            ASSERT_EQ(formatValue.level, infoMap[resName]["level"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, formatValue.colorPri));
            ASSERT_EQ(formatValue.colorPri, infoMap[resName]["colorPrim"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS,
                formatValue.colorTrans));
            ASSERT_EQ(formatValue.colorTrans, infoMap[resName]["colorTrans"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, formatValue.colorMatrix));
            ASSERT_EQ(formatValue.colorMatrix, infoMap[resName]["colorMatrix"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, formatValue.colorRange));
            ASSERT_EQ(formatValue.colorRange, infoMap[resName]["colorRange"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHROMA_LOCATION, formatValue.chromaLoc));
            ASSERT_EQ(formatValue.chromaLoc, infoMap[resName]["chromaLoc"]);
            if (resName == "hdrVivid" || resName == "hdrVividPq") {
                ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID,
                    formatValue.isHdrVivid));
                printf("isHdrVivid = %d\n", formatValue.isHdrVivid);
                ASSERT_EQ(formatValue.isHdrVivid, 1);
            } else {
                ASSERT_FALSE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID,
                    formatValue.isHdrVivid));
            }
        }
    }
}

/**
 * @tc.name: AVSource_GetFormat_1190
 * @tc.desc: get HDRVivid format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1190, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    CheckHevcInfo(g_hdrVividPath, "hdrVivid", LOCAL);
}

/**
 * @tc.name: AVSource_GetFormat_1120
 * @tc.desc: get HDRVivid format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1120, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    CheckHevcInfo(g_hdrVividUri, "hdrVivid", URI);
}

/**
 * @tc.name: AVSource_GetFormat_1200
 * @tc.desc: get mp4 265 format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1200, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    CheckHevcInfo(g_mp4HevcPath, "mp4Hevc", LOCAL);
}

/**
 * @tc.name: AVSource_GetFormat_1201
 * @tc.desc: get mp4 265 format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1201, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    CheckHevcInfo(g_mp4HevcdUri, "mp4Hevc", URI);
}

/**
 * @tc.name: AVSource_GetFormat_1300
 * @tc.desc: get mkv 265 aac format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1300, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    CheckHevcInfo(g_mkvHevcAccPath, "mkvHevcAcc", LOCAL);
}

/**
 * @tc.name: AVSource_GetFormat_1303
 * @tc.desc: get mkv 265 aac format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1303, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    CheckHevcInfo(g_mkvHevcAccUri, "mkvHevcAcc", URI);
}

/**
 * @tc.name: AVSource_GetFormat_1301
 * @tc.desc: get mkv 264 opus format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1301, TestSize.Level1)
{
    InitResource(g_mkvAvcOpusPath, LOCAL);
    FormatValue formatValue;
    trackIndex_ = vTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.frameRate, 60.000000);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.channelLayout, static_cast<int64_t>(AudioChannelLayout::MONO));
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.codecMime, "audio/opus");
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1302
 * @tc.desc: get mkv 264 mp3 format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1302, TestSize.Level1)
{
    InitResource(g_mkvAvcMp3Path, LOCAL);
    FormatValue formatValue;
    trackIndex_ = vTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.frameRate, 60.000000);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.channelLayout, static_cast<int64_t>(AudioChannelLayout::STEREO));
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.codecMime, "audio/mpeg");
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1304
 * @tc.desc: get mkv 264 opus format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1304, TestSize.Level1)
{
    InitResource(g_mkvAvcOpusUri, URI);
    FormatValue formatValue;
    trackIndex_ = vTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.frameRate, 60.000000);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.channelLayout, static_cast<int64_t>(AudioChannelLayout::MONO));
    ASSERT_EQ(formatValue.sampleRate, 48000);
    ASSERT_EQ(formatValue.codecMime, "audio/opus");
    ASSERT_EQ(formatValue.channelCount, 1);
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1305
 * @tc.desc: get mkv 264 mp3 format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1305, TestSize.Level1)
{
    InitResource(g_mkvAvcMp3Uri, URI);
    FormatValue formatValue;
    trackIndex_ = vTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatValue.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatValue.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatValue.frameRate));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.codecMime, "video/avc");
    ASSERT_EQ(formatValue.width, 1920);
    ASSERT_EQ(formatValue.height, 1080);
    ASSERT_EQ(formatValue.frameRate, 60.000000);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_VID);
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatValue.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatValue.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatValue.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatValue.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatValue.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatValue.trackType));
    ASSERT_EQ(formatValue.channelLayout, static_cast<int64_t>(AudioChannelLayout::STEREO));
    ASSERT_EQ(formatValue.sampleRate, 44100);
    ASSERT_EQ(formatValue.codecMime, "audio/mpeg");
    ASSERT_EQ(formatValue.channelCount, 2);
    ASSERT_EQ(formatValue.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatValue.trackType, MediaType::MEDIA_TYPE_AUD);
}