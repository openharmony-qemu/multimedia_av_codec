/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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
#include "mpeg4_source_unit_test.h"

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
string g_mp4Path = TEST_FILE_PATH + string("test_h264.mp4");
string g_mp4Uri = TEST_URI_PATH + string("test_h264.mp4");
string g_mp4Path2 = TEST_FILE_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_mp4Uri2 = TEST_URI_PATH + string("test_264_B_Gop25_4sec_cover.mp4");
string g_mp4InfoPath = TEST_FILE_PATH + string("camera_info_parser.mp4");
string g_mp4InfoUri = TEST_URI_PATH + string("camera_info_parser.mp4");
string g_m4aPath = TEST_FILE_PATH + string("audio/h264_fmp4.m4a");
string g_m4aUri = TEST_URI_PATH + string("audio/h264_fmp4.m4a");
string g_mp4HdrPath = TEST_FILE_PATH + string("aac_hdrvivid.mp4");
string g_mp4AvcAacPath = TEST_FILE_PATH + string("avc_aac.mp4");
string g_mp4AvcAacUri = TEST_URI_PATH + string("avc_aac.mp4");
string g_mp4AvcAacPath2 = TEST_FILE_PATH + string("h264_aac_ch5.1.mp4");
string g_mp4AvcAacUri2 = TEST_URI_PATH + string("h264_aac_ch5.1.mp4");
string g_mp4HevcAacPath2 = TEST_FILE_PATH + string("hevc_aac.mp4");
string g_mp4HevcAacUri2 = TEST_URI_PATH + string("hevc_aac.mp4");

std::map<std::string, std::map<std::string, int32_t>> infoMap = {
    {"hdrVivid", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN_10)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_5)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_HLG)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_BT2020)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
    {"mp4Hevc", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_4)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_UNSPECIFIED)},
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
    {"Hevcmov", {
        {"profile", static_cast<int32_t>(OH_HEVCProfile::HEVC_PROFILE_MAIN)},
        {"level", static_cast<int32_t>(HEVCLevel::HEVC_LEVEL_4)},
        {"colorRange", 0}, {"colorMatrix", static_cast<int32_t>(OH_MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED)},
        {"colorTrans", static_cast<int32_t>(OH_TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED)},
        {"colorPrim", static_cast<int32_t>(OH_ColorPrimary::COLOR_PRIMARY_UNSPECIFIED)},
        {"chromaLoc", static_cast<int32_t>(ChromaLocation::CHROMA_LOC_LEFT)},
    }},
};
} // namespace

void Mpeg4SourceUnitTest::InitResource(const std::string &path, bool local)
{
    printf("---- %s ------\n", path.c_str());
    if (local) {
        fd_ = OpenFile(path);
        int64_t size = GetFileSize(path);
        source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size);
        ASSERT_NE(source_, nullptr);
    } else {
        source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(path.data()));
        ASSERT_NE(source_, nullptr);
    }
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, streamsCount_));
    for (int i = 0; i < streamsCount_; i++) {
        format_ = source_->GetTrackFormat(i);
        ASSERT_NE(format_, nullptr);
        EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_TYPE, formatVal_.trackType));
        if (formatVal_.trackType == MediaType::MEDIA_TYPE_VID) {
            vTrackIdx_ = i;
        } else if (formatVal_.trackType == MediaType::MEDIA_TYPE_AUD) {
            aTrackIdx_ = i;
        }
    }
    initStatus_ = true;
}

void Mpeg4SourceUnitTest::CheckAvcInfo(const std::string resName)
{
    for (int i = 0; i < streamsCount_; i++) {
        format_ = source_->GetTrackFormat(i);
        string codecMime;
        format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codecMime);
        if (codecMime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC) {
            printf("[trackFormat %d]: %s\n", i, format_->DumpInfo());
            // pass
        }
    }
}

void Mpeg4SourceUnitTest::CheckHevcInfo(const std::string resName)
{
    for (int i = 0; i < streamsCount_; i++) {
        format_ = source_->GetTrackFormat(i);
        string codecMime;
        format_->GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, codecMime);
        if (codecMime == AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) {
            printf("[trackFormat %d]: %s\n", i, format_->DumpInfo());
            EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_PROFILE, formatVal_.profile));
            EXPECT_EQ(formatVal_.profile, infoMap[resName]["profile"]);
            EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, formatVal_.colorPri));
            EXPECT_EQ(formatVal_.colorPri, infoMap[resName]["colorPrim"]);
            EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS,
                formatVal_.colorTrans));
            EXPECT_EQ(formatVal_.colorTrans, infoMap[resName]["colorTrans"]);
            EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, formatVal_.colorMatrix));
            EXPECT_EQ(formatVal_.colorMatrix, infoMap[resName]["colorMatrix"]);
            EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, formatVal_.colorRange));
            EXPECT_EQ(formatVal_.colorRange, infoMap[resName]["colorRange"]);
#ifdef MPEG4_SOURCE_INNER_UNIT_TEST
            printf("-------input inner--------\n");
            EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_CHROMA_LOCATION, formatVal_.chromaLoc));
            EXPECT_EQ(formatVal_.chromaLoc, infoMap[resName]["chromaLoc"]);
            EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_LEVEL, formatVal_.level));
            EXPECT_EQ(formatVal_.level, infoMap[resName]["level"]);
#endif
            if (resName == "hdrVivid" || resName == "doubleVivid") {
                EXPECT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID,
                    formatVal_.isHdrVivid));
                printf("isHdrVivid = %d\n", formatVal_.isHdrVivid);
                EXPECT_EQ(formatVal_.isHdrVivid, 1);
            } else {
                EXPECT_FALSE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID,
                    formatVal_.isHdrVivid));
            }
        }
    }
}

namespace {
/**
 * @tc.name: mp4Source_GetFormat_2001
 * @tc.desc: get format when the file is mp4, local
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2001, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4Path);
    size_ = GetFileSize(g_mp4Path);
    printf("---- %s ----\n", g_mp4Path.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_TITLE, formatVal_.title));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_ARTIST, formatVal_.artist));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_COPYRIGHT, formatVal_.copyright));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.title, "Sintel Trailer");
    EXPECT_EQ(formatVal_.artist, "Durian Open Movie Team");
    EXPECT_EQ(formatVal_.copyright, "(c) copyright Blender Foundation | durian.blender.org");
    EXPECT_EQ(formatVal_.duration, 52209000);
    EXPECT_EQ(formatVal_.trackCount, 2);
}

/**
 * @tc.name: mp4Source_GetFormat_3001
 * @tc.desc: get format when the file is mp4, uri
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3001, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_TITLE, formatVal_.title));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_ARTIST, formatVal_.artist));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_COPYRIGHT, formatVal_.copyright));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_EQ(formatVal_.title, "Sintel Trailer");
    EXPECT_EQ(formatVal_.artist, "Durian Open Movie Team");
    EXPECT_EQ(formatVal_.copyright, "(c) copyright Blender Foundation | durian.blender.org");
    EXPECT_EQ(formatVal_.duration, 52209000);
    EXPECT_EQ(formatVal_.trackCount, 2);
}

/**
 * @tc.name: mp4Source_GetFormat_2002
 * @tc.desc: get format when the file is mp4, local
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2002, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4Path2);
    size_ = GetFileSize(g_mp4Path2);
    printf("---- %s ----\n", g_mp4Path2.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 4120000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.bitRate, 7782407);
    EXPECT_EQ(formatVal_.frameRate, 25.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128563);
}

/**
 * @tc.name: mp4Source_GetFormat_3002
 * @tc.desc: get format when the file is mp4, uri
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3002, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4Uri2.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4Uri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 4120000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.bitRate, 7782407);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 128563);
}

/**
 * @tc.name: mp4Source_GetFormat_2003
 * @tc.desc: get format when the file is mp4, local
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2003, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4InfoPath);
    size_ = GetFileSize(g_mp4InfoPath);
    printf("---- %s ------\n", g_mp4InfoPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[source Format]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetFloatValue(Media::Tag::MEDIA_LONGITUDE, formatVal_.longitude));
    EXPECT_TRUE(format_->GetFloatValue(Media::Tag::MEDIA_LATITUDE, formatVal_.latitude));
    EXPECT_EQ(formatVal_.longitude, float(22.670000)); // longitude test
    EXPECT_EQ(formatVal_.latitude, float(114.059998)); // latitude test
}

/**
 * @tc.name: mp4Source_GetFormat_3003
 * @tc.desc: get format when the file is mp4, uri
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3003, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4InfoUri.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4InfoUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[source Format]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetFloatValue(Media::Tag::MEDIA_LONGITUDE, formatVal_.longitude));
    EXPECT_TRUE(format_->GetFloatValue(Media::Tag::MEDIA_LATITUDE, formatVal_.latitude));
    EXPECT_EQ(formatVal_.longitude, float(22.670000)); // longitude test
    EXPECT_EQ(formatVal_.latitude, float(114.059998)); // latitude test
}

/**
 * @tc.name: mp4Source_GetFormat_2005
 * @tc.desc: get format when the file is m4a, local
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2005, TestSize.Level1)
{
    fd_ = OpenFile(g_m4aPath);
    size_ = GetFileSize(g_m4aPath);
    printf("---- %s ----\n", g_m4aPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.duration, 10064354);
    EXPECT_EQ(formatVal_.trackCount, 1);
    EXPECT_EQ(formatVal_.fileType, 206);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 127829);
}

/**
 * @tc.name: mp4Source_GetFormat_3005
 * @tc.desc: get format when the file is m4a, uri
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3005, TestSize.Level1)
{
    printf("---- %s ------\n", g_m4aUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_m4aUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_COUNT, formatVal_.trackCount));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::MEDIA_FILE_TYPE, formatVal_.fileType));
    EXPECT_EQ(formatVal_.duration, 10064354);
    EXPECT_EQ(formatVal_.trackCount, 1);
    EXPECT_EQ(formatVal_.fileType, 206);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 127829);
}

/**
 * @tc.name: mp4Source_GetFormat_2008
 * @tc.desc: get format when the file is HDRvivid, local
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2008, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4HdrPath, LOCAL);
    ASSERT_TRUE(initStatus_);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 7334000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/hevc");
    EXPECT_EQ(formatVal_.width, 2336);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.bitRate, 15100929);
    EXPECT_EQ(formatVal_.frameRate, 29.998636425617018);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 192987);
    CheckHevcInfo("hdrVivid");
}

/**
 * @tc.name: mp4Source_GetFormat_3008
 * @tc.desc: get format when the file is HDRvivid
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3008, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4HdrPath, URI);
    ASSERT_TRUE(initStatus_);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 7334000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/hevc");
    EXPECT_EQ(formatVal_.width, 2336);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.bitRate, 15100929);
    EXPECT_EQ(formatVal_.frameRate, 29.998636425617018);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 192987);
    CheckHevcInfo("hdrVivid");
}

/**
 * @tc.name: mp4Source_GetFormat_2010
 * @tc.desc: get format when the file is MP4 with AAC audio track
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2010, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4AvcAacPath);
    size_ = GetFileSize(g_mp4AvcAacPath);
    printf("---- %s ----\n", g_mp4AvcAacPath.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 6135011);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.bitRate, 2152665);
    EXPECT_EQ(formatVal_.frameRate, 29.991850040749796);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 130517);
}

/**
 * @tc.name: mp4Source_GetFormat_3010
 * @tc.desc: get format when the file is MP4 with AAC audio track
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3010, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4AvcAacUri.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4AvcAacUri.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 6135011);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.bitRate, 2152665);
    EXPECT_EQ(formatVal_.frameRate, 29.991850040749796);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.bitRate, 130517);
}

/**
 * @tc.name: mp4Source_GetFormat_2014
 * @tc.desc: get format when the file is MP4(h265 + AAC)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2014, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4HevcAacPath2, LOCAL);
    ASSERT_TRUE(initStatus_);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 52209000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.frameRate, 24.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
    CheckHevcInfo("mp4Hevc");
}

/**
 * @tc.name: mp4Source_GetFormat_3014
 * @tc.desc: get format when the file is MP4(h265 + AAC)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3014, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4HevcAacUri2, URI);
    ASSERT_TRUE(initStatus_);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 52209000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.width, 1920);
    EXPECT_EQ(formatVal_.height, 1080);
    EXPECT_EQ(formatVal_.frameRate, 24.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 48000);
    EXPECT_EQ(formatVal_.channelCount, 2);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 3);
    CheckHevcInfo("mp4Hevc");
}

/**
 * @tc.name: mp4Source_GetFormat_2023
 * @tc.desc: get format when the file is MP4(h264 + aac 5.1)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2023, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4AvcAacPath2);
    size_ = GetFileSize(g_mp4AvcAacPath2);
    printf("---- %s ----\n", g_mp4AvcAacPath2.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.bitRate, 876853);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.bitRate, 230723);
}

/**
 * @tc.name: mp4Source_GetFormat_3023
 * @tc.desc: get format when the file is MP4(h264 + aac 5.1)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3023, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4AvcAacUri2.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4AvcAacUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_VID);
    EXPECT_EQ(formatVal_.codecMime, "video/avc");
    EXPECT_EQ(formatVal_.bitRate, 876853);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_TRACK_TYPE, formatVal_.trackType));
    EXPECT_TRUE(format_->GetStringValue(OH_MD_KEY_CODEC_MIME, formatVal_.codecMime));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_BITRATE, formatVal_.bitRate));
    EXPECT_EQ(formatVal_.trackType, MediaType::MEDIA_TYPE_AUD);
    EXPECT_EQ(formatVal_.codecMime, "audio/mp4a-latm");
    EXPECT_EQ(formatVal_.bitRate, 230723);
}

/**
 * @tc.name: mp4Source_GetFormat_2024
 * @tc.desc: get format when the file is MP4(h264 + acc 5.1)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_2024, TestSize.Level1)
{
    fd_ = OpenFile(g_mp4AvcAacPath2);
    size_ = GetFileSize(g_mp4AvcAacPath2);
    printf("---- %s ----\n", g_mp4AvcAacPath2.c_str());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithFD(fd_, SOURCE_OFFSET, size_);
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 6);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 63);
}

/**
 * @tc.name: mp4Source_GetFormat_3024
 * @tc.desc: get format when the file is MP4(h264 + aac 5.1)
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4SourceUnitTest, mp4Source_GetFormat_3024, TestSize.Level1)
{
    printf("---- %s ------\n", g_mp4AvcAacUri2.data());
    source_ = Mpeg4SourceMockFactory::CreateSourceWithURI(const_cast<char*>(g_mp4AvcAacUri2.data()));
    ASSERT_NE(source_, nullptr);
    format_ = source_->GetSourceFormat();
    ASSERT_NE(format_, nullptr);
    printf("[ sourceFormat ]: %s\n", format_->DumpInfo());
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_DURATION, formatVal_.duration));
    EXPECT_EQ(formatVal_.duration, 3000000);
    trackIndex_ = 0;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_WIDTH, formatVal_.width));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_HEIGHT, formatVal_.height));
    EXPECT_TRUE(format_->GetDoubleValue(OH_MD_KEY_FRAME_RATE, formatVal_.frameRate));
    EXPECT_EQ(formatVal_.width, 720);
    EXPECT_EQ(formatVal_.height, 480);
    EXPECT_EQ(formatVal_.frameRate, 60.000000);
    trackIndex_ = 1;
    format_->Destroy();
    format_ = source_->GetTrackFormat(trackIndex_);
    ASSERT_NE(format_, nullptr);
    printf("[ trackFormat %d]: %s\n", trackIndex_, format_->DumpInfo());
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, formatVal_.sampleRate));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, formatVal_.channelCount));
    EXPECT_TRUE(format_->GetIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, formatVal_.audioSampleFormat));
    EXPECT_TRUE(format_->GetLongValue(OH_MD_KEY_CHANNEL_LAYOUT, formatVal_.channelLayout));
    EXPECT_EQ(formatVal_.sampleRate, 44100);
    EXPECT_EQ(formatVal_.channelCount, 6);
    EXPECT_EQ(formatVal_.audioSampleFormat, AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(formatVal_.channelLayout, 63);
}

} // namespace
