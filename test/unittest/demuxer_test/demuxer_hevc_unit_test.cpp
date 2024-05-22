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
#include <malloc.h>
#include <sys/stat.h>
#include <cinttypes>
#include <fcntl.h>
#include <list>
#include <map>
#include <cmath>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "media_description.h"
#include "file_server_demo.h"
#include "demuxer_unit_test.h"

#define LOCAL true
#define URI false

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace std;

namespace {
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const string TEST_URI_PATH2 = "http://192.168.3.11:8080/share/";

const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
const int64_t SOURCE_OFFSET = 0;
list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};

string g_hdrVividPath = TEST_FILE_PATH + string("hdrvivid_720p_2s.mp4");
string g_hdrVividUri = TEST_URI_PATH + string("hdrvivid_720p_2s.mp4");
string g_mp4HevcPath = TEST_FILE_PATH + string("camera_h265_aac_rotate270.mp4");
string g_mp4HevcUri = TEST_URI_PATH + string("camera_h265_aac_rotate270.mp4");
string g_mkvHevcAccPath = TEST_FILE_PATH + string("h265_aac_4sec.mkv");
string g_mkvHevcAccUri = TEST_URI_PATH + string("h265_aac_4sec.mkv");
string g_mkvAvcOpusPath = TEST_FILE_PATH + string("h264_opus_4sec.mkv");
string g_mkvAvcOpusUri = TEST_URI_PATH + string("h264_opus_4sec.mkv");
string g_mkvAvcMp3Path = TEST_FILE_PATH + string("h264_mp3_4sec.mkv");
string g_mkvAvcMp3Uri = TEST_URI_PATH + string("h264_mp3_4sec.mkv");
string g_tsHevcAacPath = TEST_FILE_PATH + string("hevc_aac_1920x1080_g30_30fps.ts");
string g_tsHevcAacUri = TEST_URI_PATH + string("hevc_aac_1920x1080_g30_30fps.ts");
string g_tsHevcAac4KPath = TEST_FILE_PATH + string("hevc_aac_3840x2160_30frames.ts");
string g_flvPath = TEST_FILE_PATH + string("h265_enhanced.flv");
string g_flvUri = TEST_URI_PATH + string("h265_enhanced.flv");
string g_fmp4HevcPath = TEST_FILE_PATH + string("h265_fmp4.mp4");
string g_fmp4HevcUri = TEST_URI_PATH + string("h265_fmp4.mp4");
string g_DoubleVividPath = TEST_FILE_PATH + string("audiovivid_hdrvivid_2s.mp4");
string g_DoubleVividUri = TEST_URI_PATH + string("audiovivid_hdrvivid_2s.mp4");
string g_hls = TEST_URI_PATH2 + string("index_265.m3u8");

std::map<std::string, std::map<std::string, std::vector<int32_t>>> infoMap = {
    {"hdrVivid",   {{"frames", {76,   125}}, {"kFrames", {3, 125}}}},
    {"mp4Hevc",    {{"frames", {60,   87 }}, {"kFrames", {1, 87 }}}},
    {"mkvHevcAcc", {{"frames", {242,  173}}, {"kFrames", {1, 173}}}},
    {"mkvAvcOpus", {{"frames", {240,  199}}, {"kFrames", {4, 199}}}},
    {"mkvAvcMp3",  {{"frames", {239,  153}}, {"kFrames", {4, 153}}}},
    {"tsHevcAac",  {{"frames", {303,  433}}, {"kFrames", {11, 433}}}},
    {"fmp4Hevc",  {{"frames", {604,  433}}, {"kFrames", {3, 433}}}},
    {"doubleVivid",  {{"frames", {76,  116}}, {"kFrames", {3, 116}}}},
};
} // namespace

void DemuxerUnitTest::InitResource(const std::string &path, bool local)
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
    ASSERT_TRUE(format_->GetIntValue(MediaDescriptionKey::MD_KEY_TRACK_COUNT, nbStreams_));
    demuxer_ = AVDemuxerMockFactory::CreateDemuxer(source_);
    ASSERT_NE(demuxer_, nullptr);
}

void DemuxerUnitTest::ReadSample(const std::string &path, bool local)
{
    InitResource(path, local);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);

    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
}

namespace {
/**
 * @tc.name: Demuxer_ReadSample_1220
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1220, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_hdrVividPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["hdrVivid"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["hdrVivid"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1221
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1221, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_hdrVividUri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["hdrVivid"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["hdrVivid"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1200
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1200, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_mp4HevcPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mp4Hevc"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mp4Hevc"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1201
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1201, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_mp4HevcUri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mp4Hevc"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mp4Hevc"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1210
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1210, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_mkvHevcAccPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mkvHevcAcc"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mkvHevcAcc"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1211
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1211, TestSize.Level1)
{
    ReadSample(g_mkvAvcOpusPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mkvAvcOpus"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mkvAvcOpus"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1212
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1212, TestSize.Level1)
{
    ReadSample(g_mkvAvcMp3Path, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mkvAvcMp3"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mkvAvcMp3"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1213
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1213, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_mkvHevcAccUri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mkvHevcAcc"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mkvHevcAcc"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1214
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1214, TestSize.Level1)
{
    ReadSample(g_mkvAvcOpusUri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mkvAvcOpus"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mkvAvcOpus"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1215
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1215, TestSize.Level1)
{
    ReadSample(g_mkvAvcMp3Uri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["mkvAvcMp3"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["mkvAvcMp3"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1216
 * @tc.desc: copy current sample to buffer, local(ts)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1216, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_tsHevcAacPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["tsHevcAac"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["tsHevcAac"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1217
 * @tc.desc: copy current sample to buffer, uri(ts)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1217, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_tsHevcAacPath, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["tsHevcAac"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["tsHevcAac"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1218
 * @tc.desc: copy current sample to buffer, local(ts)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1218, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    string path = TEST_FILE_PATH + string("hevc_aac_3840x2160_30frames.ts");
    ReadSample(path, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], 30);
        ASSERT_EQ(keyFrames_[idx], 1);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1226
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1226, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_fmp4HevcPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["fmp4Hevc"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["fmp4Hevc"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1227
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1227, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_fmp4HevcUri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["fmp4Hevc"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["fmp4Hevc"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1231
 * @tc.desc: copy current sample to buffer, local
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1231, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_DoubleVividPath, LOCAL);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["doubleVivid"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["doubleVivid"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_ReadSample_1232
 * @tc.desc: copy current sample to buffer, uri
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1232, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    ReadSample(g_DoubleVividUri, URI);
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(frames_[idx], infoMap["doubleVivid"]["frames"][idx]);
        ASSERT_EQ(keyFrames_[idx], infoMap["doubleVivid"]["kFrames"][idx]);
    }
    RemoveValue();
    selectedTrackIds_.clear();
}

/**
 * @tc.name: Demuxer_SeekToTime_1170
 * @tc.desc: seek to the specified time(h265 mp4 fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1170, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4HevcPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1500, 1000, 1740, 1970, 2100}; // ms
    vector<int32_t> videoVals = {60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1171
 * @tc.desc: seek to the specified time(h265 mp4 uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1171, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mp4HevcUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1500, 1000, 1740, 1970, 2100}; // ms
    vector<int32_t> videoVals = {60, 60, 60, 60, 60, 60, 60, 60, 60, 60, 60};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1180
 * @tc.desc: seek to the specified time(h265+aac(mkv) fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1180, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mkvHevcAccPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 2000, 1500, 2160, 3630, 2850, 4017, 4300}; // ms
    vector<int32_t> videoVals = {242, 242, 242, 242, 242, 242, 242, 242, 242, 242,
        242, 242, 242, 242, 242, 242, 242, 242, 242};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1181
 * @tc.desc: seek to the specified time(h264+opus(mkv) fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1181, TestSize.Level1)
{
    InitResource(g_mkvAvcOpusPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000, 1500, 1720, 2700, 3980, 4000, 4100}; // ms
    vector<int32_t> videoVals = {240, 240, 240, 120, 120, 120, 120, 180, 180,
        120, 180, 120, 60, 120, 60, 60, 60, 60, 60, 60, 60};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1182
 * @tc.desc: seek to the specified time(h264+mp3(mkv) fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1182, TestSize.Level1)
{
    InitResource(g_mkvAvcMp3Path, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2025, 1500, 1958, 2600, 3400, 3992, 4100}; // ms
    vector<int32_t> videoVals = {239, 239, 239, 59, 119, 119, 119, 179, 179, 119, 179, 119,
        59, 119, 59, 59, 59, 59, 59, 59, 59};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1183
 * @tc.desc: seek to the specified time(h265+aac(mkv) uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1183, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_mkvHevcAccUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1000, 2000, 1500, 2160, 3630, 2850, 4017, 4300}; // ms
    vector<int32_t> videoVals = {242, 242, 242, 242, 242, 242, 242, 242, 242, 242,
        242, 242, 242, 242, 242, 242, 242, 242, 242};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1184
 * @tc.desc: seek to the specified time(h264+opus(mkv) uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1184, TestSize.Level1)
{
    InitResource(g_mkvAvcOpusUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2000, 1500, 1720, 2700, 3980, 4000, 4100}; // ms
    vector<int32_t> videoVals = {240, 240, 240, 120, 120, 120, 120, 180, 180,
        120, 180, 120, 60, 120, 60, 60, 60, 60, 60, 60, 60};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1185
 * @tc.desc: seek to the specified time(h264+mp3(mkv) uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1185, TestSize.Level1)
{
    InitResource(g_mkvAvcMp3Uri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 2025, 1500, 1958, 2600, 3400, 3992, 4100}; // ms
    vector<int32_t> videoVals = {239, 239, 239, 59, 119, 119, 119, 179, 179, 119, 179, 119,
        59, 119, 59, 59, 59, 59, 59, 59, 59};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1190
 * @tc.desc: seek to the specified time(hdrvivid mp4 fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1190, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_hdrVividPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1166, 2000, 1500, 2666, 2800}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 16, 46, 46, 16, 16, 16, 16, 46, 46, 16, 16};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1191
 * @tc.desc: seek to the specified time(hdrvivid mp4 uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1191, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_hdrVividUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1166, 2000, 1500, 2666, 2800}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 16, 46, 46, 16, 16, 16, 16, 46, 46, 16, 16};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
}

/**
 * @tc.name: Demuxer_SeekToTime_1192
 * @tc.desc: seek to the specified time(h265 ts fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1192, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_tsHevcAacPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
    vector<int32_t> videoVals = {303, 303, 303, 258, 258, 258, 3, 3, 3, 165, 165, 165};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1193
 * @tc.desc: seek to the specified time(h265 ts uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1193, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_tsHevcAacUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1500, 10000, 4600}; // ms
    vector<int32_t> videoVals = {303, 303, 303, 258, 258, 258, 3, 3, 3, 165, 165, 165};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1198
 * @tc.desc: seek to the specified time(h265 fmp4 fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1198, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_fmp4HevcPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {358, 604, 604, 107, 358, 358, 107, 358, 107, 358, 604, 604};
    vector<int32_t> audioVals = {258, 433, 433, 78, 259, 259, 78, 259, 78, 258, 433, 433};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1199
 * @tc.desc: seek to the specified time(h265 fmp4 uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1199, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_fmp4HevcUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 4500, 7000, 2000}; // ms
    vector<int32_t> videoVals = {358, 604, 604, 107, 358, 358, 107, 358, 107, 358, 604, 604};
    vector<int32_t> audioVals = {258, 433, 433, 78, 259, 259, 78, 259, 78, 258, 433, 433};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1207
 * @tc.desc: seek to the specified time(doublevivid fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1207, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_DoubleVividPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1166, 2000, 1500, 2666, 2800}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 46, 46, 46, 16, 46, 16, 16, 46, 46, 16, 16};
    vector<int32_t> audioVals = {116, 116, 116, 65, 66, 65, 22, 66, 22, 22, 66, 66, 23, 23};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1208
 * @tc.desc: seek to the specified time(doublevivid fd)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1208, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_DoubleVividUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1166, 2000, 1500, 2666, 2800}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 46, 46, 46, 16, 46, 16, 16, 46, 46, 16, 16};
    vector<int32_t> audioVals = {116, 116, 116, 65, 66, 65, 22, 66, 22, 22, 66, 66, 23, 23};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_1401
 * @tc.desc: copy current sample to buffer(flv_enhanced)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1401, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_flvPath, LOCAL);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_ReadSample_1402
 * @tc.desc: copy current sample to buffer(flv_enhanced uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1402, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_flvUri, URI);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_1203
 * @tc.desc: seek to the specified time(h265 flv uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1203, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_flvUri, URI);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1500, 1000, 1740, 1970, 2100}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_1204
 * @tc.desc: seek to the specified time(h265 flv local)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1204, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    InitResource(g_flvPath, LOCAL);
    SetInitValue();
    for (auto idx : selectedTrackIds_) {
        ASSERT_EQ(demuxer_->SelectTrackByID(idx), AV_ERR_OK);
    }
    list<int64_t> toPtsList = {0, 1500, 1000, 1740, 1970, 2100}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76, 76};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}

/**
 * @tc.name: Demuxer_ReadSample_1412
 * @tc.desc: copy current sample to buffer(h265 hls uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_ReadSample_1412, TestSize.Level1)
{
    if (g_hls.find(TEST_URI_PATH2) != std::string::npos) {
        return;
    }
    InitResource(g_hls, URI);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    SetInitValue();
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            ASSERT_EQ(demuxer_->ReadSample(idx, sharedMem_, &info_, flag_), AV_ERR_OK);
            CountFrames(idx);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 602);
    ASSERT_EQ(frames_[1], 433);
    ASSERT_EQ(keyFrames_[0], 3);
    ASSERT_EQ(keyFrames_[1], 433);
    RemoveValue();
}

/**
 * @tc.name: Demuxer_SeekToTime_1413
 * @tc.desc: seek to the specified time(h265 hls uri)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerUnitTest, Demuxer_SeekToTime_1413, TestSize.Level1)
{
    if (g_hls.find(TEST_URI_PATH2) != std::string::npos) {
        return;
    }
    InitResource(g_hls, URI);
    ASSERT_NE(source_, nullptr);
    ASSERT_NE(format_, nullptr);
    ASSERT_NE(demuxer_, nullptr);
    ASSERT_EQ(demuxer_->SelectTrackByID(0), AV_ERR_OK);
    ASSERT_EQ(demuxer_->SelectTrackByID(1), AV_ERR_OK);
    list<int64_t> toPtsList = {0, 4500, 7000, 2000, 10000}; // ms
    vector<int32_t> videoVals = {602, 602, 602, 102, 352, 352, 102, 352, 102, 352, 602, 602, 102, 102};
    vector<int32_t> audioVals = {433, 433, 433, 74, 254, 254, 74, 254, 74, 253, 433, 433, 75, 75};
    sharedMem_ = AVMemoryMockFactory::CreateAVMemoryMock(bufferSize_);
    ASSERT_NE(sharedMem_, nullptr);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            ret_ = demuxer_->SeekToTime(*toPts, *mode);
            if (ret_ != AV_ERR_OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d | kFrames[0]=%d\n", *toPts, frames_[0], keyFrames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d | kFrames[1]=%d\n", *toPts, frames_[1], keyFrames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
            selectedTrackIds_.clear();
        }
    }
    ASSERT_NE(demuxer_->SeekToTime(11000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
    ASSERT_NE(demuxer_->SeekToTime(-1000, SeekMode::SEEK_NEXT_SYNC), AV_ERR_OK);
}
} // namespace