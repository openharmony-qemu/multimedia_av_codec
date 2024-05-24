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
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
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
string g_tsHevcAacPath = TEST_FILE_PATH + string("hevc_aac_1920x1080_g30_30fps.ts");
string g_tsHevcAacUri = TEST_URI_PATH + string("hevc_aac_1920x1080_g30_30fps.ts");
string g_flvPath = TEST_FILE_PATH + string("h265_enhanced.flv");
string g_fmp4HevcPath = TEST_FILE_PATH + string("h265_fmp4.mp4");
string g_fmp4HevcUri = TEST_URI_PATH + string("h265_fmp4.mp4");
string g_DoubleVividPath = TEST_FILE_PATH + string("audiovivid_hdrvivid_2s.mp4");
string g_DoubleVividUri = TEST_URI_PATH + string("audiovivid_hdrvivid_2s.mp4");

std::map<std::string, std::map<std::string, int32_t>> infoMap = {
    {"hdrVivid", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN_10)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_4)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT2020)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"mp4Hevc", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_31)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT709)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_BT709)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT709)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"mkvHevcAcc", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_41)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_UNSPECIFIED)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"tsHevcAac", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_4)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_UNSPECIFIED)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"HevcFlv", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN_10)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_31)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT2020)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"Hevcfmp4", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_31)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_UNSPECIFIED)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"doubleVivid", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN_10)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_4)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT2020)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
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
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, streamsCount_));
    for (int i = 0; i < streamsCount_; i++) {
        format_ = source_->GetTrackFormat(i);
        ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        if (formatVal_.trackType == MediaType::MEDIA_TYPE_VID) {
            vTrackIdx_ = i;
        } else if (formatVal_.trackType == MediaType::MEDIA_TYPE_AUD) {
            aTrackIdx_ = i;
        }
    }
}

void AVSourceUnitTest::CheckHevcInfo(const std::string resName)
{
    for (int i = 0; i < streamsCount_; i++) {
        format_ = source_->GetTrackFormat(i);
        string codecMime;
        format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codecMime);
        if (codecMime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) {
            printf("[trackFormat %d]: %s\n", i, format_->DumpInfo());
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, formatVal_.profile));
            ASSERT_EQ(formatVal_.profile, infoMap[resName]["profile"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, formatVal_.colorPri));
            ASSERT_EQ(formatVal_.colorPri, infoMap[resName]["colorPrim"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS,
                formatVal_.colorTrans));
            ASSERT_EQ(formatVal_.colorTrans, infoMap[resName]["colorTrans"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, formatVal_.colorMatrix));
            ASSERT_EQ(formatVal_.colorMatrix, infoMap[resName]["colorMatrix"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, formatVal_.colorRange));
            ASSERT_EQ(formatVal_.colorRange, infoMap[resName]["colorRange"]);
#ifdef AVSOURCE_INNER_UNIT_TEST
            printf("-------input inner--------\n");
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHROMA_LOCATION, formatVal_.chromaLoc));
            ASSERT_EQ(formatVal_.chromaLoc, infoMap[resName]["chromaLoc"]);
            ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_LEVEL, formatVal_.level));
            ASSERT_EQ(formatVal_.level, infoMap[resName]["level"]);
#endif
            if (resName == "hdrVivid" || resName == "doubleVivid") {
                ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID,
                    formatVal_.isHdrVivid));
                printf("isHdrVivid = %d\n", formatVal_.isHdrVivid);
                ASSERT_EQ(formatVal_.isHdrVivid, 1);
            } else {
                ASSERT_FALSE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID,
                    formatVal_.isHdrVivid));
            }
        }
    }
}

namespace {
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
    InitResource(g_hdrVividPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    CheckHevcInfo("hdrVivid");
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
    InitResource(g_hdrVividUri, URI);
    ASSERT_NE(source_, nullptr);
    CheckHevcInfo("hdrVivid");
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
    InitResource(g_mp4HevcPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    CheckHevcInfo("mp4Hevc");
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
    InitResource(g_mp4HevcdUri, URI);
    ASSERT_NE(source_, nullptr);
    CheckHevcInfo("mp4Hevc");
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
    InitResource(g_mkvHevcAccPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    CheckHevcInfo("mkvHevcAcc");
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
    InitResource(g_mkvHevcAccUri, URI);
    ASSERT_NE(source_, nullptr);
    CheckHevcInfo("mkvHevcAcc");
}

/**
 * @tc.name: AVSource_GetFormat_1301
 * @tc.desc: get mkv 264 opus format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1301, TestSize.Level1)
{
    InitResource(g_mkvAvcOpusPath, LOCAL);
    trackIndex_ = vTrackIdx_;
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
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.channelLayout, static_cast<int64_t>(AudioChannelLayout::MONO));
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/opus");
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1302
 * @tc.desc: get mkv 264 mp3 format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1302, TestSize.Level1)
{
    InitResource(g_mkvAvcMp3Path, LOCAL);
    trackIndex_ = vTrackIdx_;
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
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.channelLayout, static_cast<int64_t>(AudioChannelLayout::STEREO));
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1304
 * @tc.desc: get mkv 264 opus format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1304, TestSize.Level1)
{
    InitResource(g_mkvAvcOpusUri, URI);
    trackIndex_ = vTrackIdx_;
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
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.channelLayout, static_cast<int64_t>(AudioChannelLayout::MONO));
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.codecMime, "audio/opus");
    ASSERT_EQ(formatVal_.channelCount, 1);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1305
 * @tc.desc: get mkv 264 mp3 format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1305, TestSize.Level1)
{
    InitResource(g_mkvAvcMp3Uri, URI);
    trackIndex_ = vTrackIdx_;
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
    trackIndex_ = aTrackIdx_;
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, formatVal_.sampleRate));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, formatVal_.channelCount));
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_EQ(formatVal_.channelLayout, static_cast<int64_t>(AudioChannelLayout::STEREO));
    ASSERT_EQ(formatVal_.sampleRate, 44100);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
}

/**
 * @tc.name: AVSource_GetFormat_1306
 * @tc.desc: get hevc format, local (ts)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1306, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_tsHevcAacPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat(); // source
    ASSERT_NE(format_, nullptr);
    format_->DumpInfo();
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 10123222);
    ASSERT_EQ(formatVal_.trackCount, 2);
    CheckHevcInfo("tsHevcAac");
}

/**
 * @tc.name: AVSource_GetFormat_1307
 * @tc.desc: get hevc format, local (ts)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1307, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_tsHevcAacUri, URI);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat(); // source
    ASSERT_NE(format_, nullptr);
    format_->DumpInfo();
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 10123222);
    ASSERT_EQ(formatVal_.trackCount, 2);
    CheckHevcInfo("tsHevcAac");
}

/**
 * @tc.name: AVSource_GetFormat_1312
 * @tc.desc: get fmp4 hevc mp4 format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1312, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_fmp4HevcPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat(); // source
    ASSERT_NE(format_, nullptr);
    format_->DumpInfo();
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 10100000);
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
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/hevc");
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
    ASSERT_EQ(formatVal_.bitRate, 127407);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    CheckHevcInfo("Hevcfmp4");
}

/**
 * @tc.name: AVSource_GetFormat_1313
 * @tc.desc: get fmp4 hevc mp4 format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1313, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_fmp4HevcUri, URI);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat(); // source
    ASSERT_NE(format_, nullptr);
    format_->DumpInfo();
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 10100000);
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
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/hevc");
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
    ASSERT_EQ(formatVal_.bitRate, 127407);
    ASSERT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    CheckHevcInfo("Hevcfmp4");
}

/**
 * @tc.name: AVSource_GetFormat_1314
 * @tc.desc: get audiovivid hdrvivid fmp4 format, local
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1314, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_DoubleVividPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat(); // source
    ASSERT_NE(format_, nullptr);
    format_->DumpInfo();
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 2699349);
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
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/hevc");
    ASSERT_EQ(formatVal_.width, 1280);
    ASSERT_EQ(formatVal_.height, 720);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 28.154937050793496);
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
    ASSERT_EQ(formatVal_.bitRate, 64083);
    ASSERT_EQ(formatVal_.codecMime, "audio/av3a");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::INVALID_WIDTH);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    CheckHevcInfo("doubleVivid");
}

/**
 * @tc.name: AVSource_GetFormat_1315
 * @tc.desc: get audiovivid hdrvivid fmp4 format, uri
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1315, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_DoubleVividUri, URI);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat(); // source
    ASSERT_NE(format_, nullptr);
    format_->DumpInfo();
    ASSERT_TRUE(format_->GetLongValue(MediaDescriptionKey::MD_KEY_DURATION, formatVal_.duration));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    ASSERT_EQ(formatVal_.duration, 2699349);
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
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
    ASSERT_TRUE(format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, formatVal_.codecMime));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, formatVal_.width));
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, formatVal_.height));
    ASSERT_TRUE(format_->GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, formatVal_.frameRate));
    ASSERT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    ASSERT_EQ(formatVal_.codecMime, "video/hevc");
    ASSERT_EQ(formatVal_.width, 1280);
    ASSERT_EQ(formatVal_.height, 720);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 28.154937050793496);
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
    ASSERT_EQ(formatVal_.bitRate, 64083);
    ASSERT_EQ(formatVal_.codecMime, "audio/av3a");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::INVALID_WIDTH);
    ASSERT_EQ(formatVal_.channelLayout, 3);
    CheckHevcInfo("doubleVivid");
}

/**
 * @tc.name: AVSource_GetFormat_1402
 * @tc.desc: get source format(flv)
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1402, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    fd_ = OpenFile(g_flvPath);
    size_ = GetFileSize(g_flvPath);
    printf("---- %s ----\n", g_flvPath.c_str());
    source_ = AVSourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, streamsCount_));
    ASSERT_EQ(streamsCount_, 2);
#ifdef AVSOURCE_INNER_UNIT_TEST
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_VIDEO, formatVal_.hasVideo));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_HAS_AUDIO, formatVal_.hasAudio));
    ASSERT_TRUE(format_->GetIntValue(AVSourceFormat::SOURCE_FILE_TYPE, formatVal_.fileType));
    ASSERT_EQ(formatVal_.hasVideo, 1);
    ASSERT_EQ(formatVal_.hasAudio, 1);
    ASSERT_EQ(formatVal_.fileType, 104);
#endif
    CheckHevcInfo("HevcFlv");
}

/**
 * @tc.name: AVSource_GetFormat_1403
 * @tc.desc: get format when the file is flv
 * @tc.type: FUNC
 */
HWTEST_F(AVSourceUnitTest, AVSource_GetFormat_1403, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    fd_ = OpenFile(g_flvPath);
    size_ = GetFileSize(g_flvPath);
    printf("---- %s ------\n", g_flvPath.c_str());
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
    ASSERT_EQ(formatVal_.codecMime, "video/hevc");
    ASSERT_EQ(formatVal_.width, 720);
    ASSERT_EQ(formatVal_.height, 1280);
    ASSERT_DOUBLE_EQ(formatVal_.frameRate, 30.000000);
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
    ASSERT_EQ(formatVal_.sampleRate, 48000);
    ASSERT_EQ(formatVal_.channelCount, 2);
    ASSERT_EQ(formatVal_.bitRate, 128000);
    ASSERT_EQ(formatVal_.codecMime, "audio/mpeg");
    ASSERT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    ASSERT_EQ(formatVal_.channelLayout, 3);
}
} // namespace