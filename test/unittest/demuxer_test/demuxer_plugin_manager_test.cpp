/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "demuxer_plugin_manager_test.h"

#include "gtest/gtest.h"
#include "demuxer_plugin_manager.h"

#define LOCAL true

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;

namespace OHOS {
namespace Media {
static const int BUFFER_PADDING_SIZE = 1024;
static const int DEF_PROB_SIZE = 16 * 1024;

static const std::string DEMUXER_PLUGIN_NAME_AAC = "avdemux_aac";
static const std::string DEMUXER_PLUGIN_NAME_AMR = "avdemux_amr";
static const std::string DEMUXER_PLUGIN_NAME_AMRNB = "avdemux_amrnb";
static const std::string DEMUXER_PLUGIN_NAME_AMRWB = "avdemux_amrwb";
static const std::string DEMUXER_PLUGIN_NAME_APE = "avdemux_ape";
static const std::string DEMUXER_PLUGIN_NAME_FLAC = "avdemux_flac";
static const std::string DEMUXER_PLUGIN_NAME_FLV = "avdemux_flv";
static const std::string DEMUXER_PLUGIN_NAME_MATROSKA = "avdemux_matroska,webm";
static const std::string DEMUXER_PLUGIN_NAME_MOV_S = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
static const std::string DEMUXER_PLUGIN_NAME_MP3 = "avdemux_mp3";
static const std::string DEMUXER_PLUGIN_NAME_MPEG = "avdemux_mpeg";
static const std::string DEMUXER_PLUGIN_NAME_MPEGTS = "avdemux_mpegts";
static const std::string DEMUXER_PLUGIN_NAME_AVI = "avdemux_avi";
static const std::string DEMUXER_PLUGIN_NAME_SRT = "avdemux_srt";
static const std::string DEMUXER_PLUGIN_NAME_WEBVTT = "avdemux_webvtt";
static const std::string DEMUXER_PLUGIN_NAME_OGG = "avdemux_ogg";
static const std::string DEMUXER_PLUGIN_NAME_WAV = "avdemux_wav";
static const std::string DEMUXER_PLUGIN_NAME_RM = "avdemux_rm";
static const std::string DEMUXER_PLUGIN_NAME_AC3 = "avdemux_ac3";
static const std::string DEMUXER_PLUGIN_NAME_ERR = "avdemux_err";
static const std::string DEMUXER_PLUGIN_NAME_ASF = "avdemux_asf";

static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_FILE_URI_AAC = TEST_FILE_PATH + "audio/aac_44100_1.aac";
static const string TEST_FILE_URI_AMR = TEST_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_FILE_URI_AMRNB = TEST_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_FILE_URI_AMRWB = TEST_FILE_PATH + "audio/amr_wb_16000_1.amr";
static const string TEST_FILE_URI_APE = TEST_FILE_PATH + "ape_test.ape";
static const string TEST_FILE_URI_FLAC = TEST_FILE_PATH + "audio/flac_48000_1_cover.flac";
static const string TEST_FILE_URI_FLV = TEST_FILE_PATH + "h264.flv";
static const string TEST_FILE_URI_MATROSKA = TEST_FILE_PATH + "h264_opus_4sec.mkv";
static const string TEST_FILE_URI_MOV = TEST_FILE_PATH + "h264_aac.mov";
static const string TEST_FILE_URI_MP4 = TEST_FILE_PATH + "5_1_4_16bit_32000Hz_704kbps.mp4";
static const string TEST_FILE_URI_FMP4 = TEST_FILE_PATH + "h265_fmp4.mp4";
static const string TEST_FILE_URI_M4A = TEST_FILE_PATH + "audio/h264_fmp4.m4a";
static const string TEST_FILE_URI_MP3 = TEST_FILE_PATH + "audio/mp3_48000_1_cover.mp3";
static const string TEST_FILE_URI_MPEG = TEST_FILE_PATH + "mpeg_h264_mp2.mpeg";
static const string TEST_FILE_URI_MPEGTS = TEST_FILE_PATH + "test_mpeg2_Gop25_4sec.ts";
static const string TEST_FILE_URI_AVI = TEST_FILE_PATH + "h264_aac.avi";
static const string TEST_FILE_URI_SRT = TEST_FILE_PATH + "subtitle.srt";
static const string TEST_FILE_URI_WEBVTT = TEST_FILE_PATH + "webvtt_test_ut.vtt";
static const string TEST_FILE_URI_OGG = TEST_FILE_PATH + "audio/ogg_48000_1_ut.ogg";
static const string TEST_FILE_URI_WAV = TEST_FILE_PATH + "audio/wav_48000_1_ut.wav";
static const string TEST_FILE_URI_RM = TEST_FILE_PATH + "rv40_cook.rmvb";
static const string TEST_FILE_URI_AC3 = TEST_FILE_PATH + "audio/ac3_test.ac3";
static const string TEST_FILE_URI_MP4_2 = TEST_FILE_PATH + "noframe.mp4";
static const string TEST_FILE_URI_ASF = TEST_FILE_PATH + "wmv_wmv3_wmapro.wmv";
static const string TEST_FILE_URI_3GP = TEST_FILE_PATH + "3gp_h264_aac.3gp";
static const string TEST_FILE_URI_3G2 = TEST_FILE_PATH + "3g2_mp4v_mp4a.3g2";
static const string TEST_FILE_URI_VOB = TEST_FILE_PATH + "vob_mpeg2_mp2.vob";
static const string TEST_FILE_URI_MPEGTS_2 = TEST_FILE_PATH + "gop250.ts";
static const string TEST_FILE_URI_MPEGTS_3 = TEST_FILE_PATH + "test_ts.ts";

typedef struct TestInfo {
    string pluginName;
    string testFile;
    vector<uint32_t> frameCnt;
    vector<uint32_t> keyFrameCnt;
    TestInfo(string name, string file, vector<uint32_t> &&cnt, vector<uint32_t> &&keyCnt)
        : pluginName(name), testFile(file), frameCnt(std::move(cnt)), keyFrameCnt(std::move(keyCnt)) {}
} TestInfo;

static std::vector<TestInfo> TEST_LIST = {
    {DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, {1293, 0}, {1293, 0}},
    {DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMR, {1501, 0, 1501, 0}, {1501, 0, 1501, 0}},
    {DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRNB, {1501, 0, 1501, 0}, {1501, 0, 1501, 0}},
    {DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRWB, {1500, 0, 1500, 0}, {1500, 0, 1500, 0}},
    {DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_APE, {7, 0}, {7}},
    {DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, {313}, {313}},
    {DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, {76, 113}, {1, 113}},
    {DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MATROSKA, {240, 199}, {4, 199}},
    {DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MOV, {602, 434}, {3, 434}},
    {DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, {1875, 0}, {1875, 0}},
    {DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FMP4, {604, 433}, {3, 433}},
    {DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_M4A, {433, 0}, {433, 0}},
    {DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, {1251, 0}, {1251, 0}},
    {DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPEG, {1253, 2164}, {19, 2164}},
    {DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, {103, 174}, {5, 174}},
    {DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_AVI, {602, 433}, {3, 433}},
    {DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_SRT, {5, 0}, {5, 0}},
    {DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_WEBVTT, {4, 0}, {4, 0}},
    {DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, {1598, 0}, {1598, 0}},
    {DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_WAV, {704, 0}, {704, 0}},
    {DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_ASF, {122, 480}, {122, 8}},
    {DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_3GP, {1116, 2186}, {23, 2186}},
    {DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_3G2, {937, 1121}, {81, 1121}},
    {DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_VOB, {300, 417}, {25, 417}},
};

static std::vector<TestInfo> TEST_LIST2 = {
    {DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_PATH + "aigc_str.flv", {1800, 0}, {9, 0}},
    {DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_PATH + "vp8_vorbis.webm", {602, 594}, {5, 594}},
    {DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_PATH + "vp9_opus.webm", {598, 996}, {5, 996}},
    {DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_PATH + "wmv_h264_wmav1.wmv", {602, 218}, {3, 218}}
};


void DemuxerPluginManagerUnitTest::SetUpTestCase(void) {}

void DemuxerPluginManagerUnitTest::TearDownTestCase(void) {}

void DemuxerPluginManagerUnitTest::SetUp(void)
{
    demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
    streamDemuxer_ = std::make_shared<MockBaseStreamDemuxer>();
    int32_t streamId = 0;
    dataSourceImpl_ = std::make_shared<DataSourceImpl>(streamDemuxer_, streamId);
    dataSourceImpl_->stream_ = streamDemuxer_;
}

void DemuxerPluginManagerUnitTest::TearDown(void)
{
    demuxerPluginManager_ = nullptr;
    streamDemuxer_ = nullptr;
    dataSourceImpl_ = nullptr;
}

HWTEST_F(DemuxerPluginManagerUnitTest, SetStreamID_001, TestSize.Level1)
{
    // 1. Set up the test environment
    ASSERT_NE(dataSourceImpl_, nullptr);

    // 2. Call the function to be tested
    Status status = dataSourceImpl_->SetStreamID(1);

    // 3. Verify the result
    EXPECT_EQ(status, Status::OK);
    EXPECT_EQ(dataSourceImpl_->streamID_, 1);
    EXPECT_EQ(dataSourceImpl_->GetStreamID(), 1);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetStreamCount_001, TestSize.Level1)
{
    size_t streamCount = demuxerPluginManager_->GetStreamCount();

    EXPECT_EQ(streamCount, demuxerPluginManager_->streamInfoMap_.size());
}

HWTEST_F(DemuxerPluginManagerUnitTest, LoadCurrentSubtitlePlugin_001, TestSize.Level1)
{
    demuxerPluginManager_->curSubTitleStreamID_ = -1;
    Plugins::MediaInfo mediaInfo;
    Status status = demuxerPluginManager_->LoadCurrentSubtitlePlugin(streamDemuxer_, mediaInfo);

    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetInnerTrackIDByTrackID_001, TestSize.Level1)
{
    int32_t trackId = 1;
    demuxerPluginManager_->trackInfoMap_[trackId].innerTrackIndex = 2;

    // 2. Call the function to be tested
    int32_t innerTrackId = demuxerPluginManager_->GetInnerTrackIDByTrackID(trackId);

    // 3. Verify the result
    EXPECT_EQ(innerTrackId, 2);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetInnerTrackIDByTrackID_002, TestSize.Level1)
{
    int32_t trackId = 1;

    // 2. Call the function to be tested
    int32_t innerTrackId = demuxerPluginManager_->GetInnerTrackIDByTrackID(trackId);

    // 3. Verify the result
    EXPECT_EQ(innerTrackId, -1);
}

HWTEST_F(DemuxerPluginManagerUnitTest, AddExternalSubtitle_001, TestSize.Level1)
{
    // 1. Set up the test environment
    demuxerPluginManager_->curSubTitleStreamID_ = -1;
    // 2. Call the function to be tested
    int32_t result = demuxerPluginManager_->AddExternalSubtitle();
    // 3. Verify the result
    EXPECT_EQ(result, 0);
    EXPECT_EQ(demuxerPluginManager_->curSubTitleStreamID_, 0);

    // 1. Set up the test environment
    demuxerPluginManager_->curSubTitleStreamID_ = 0;
    // 2. Call the function to be tested
    result = demuxerPluginManager_->AddExternalSubtitle();
}

HWTEST_F(DemuxerPluginManagerUnitTest, IsSubtitleMime_001, TestSize.Level1)
{
    // 1. Set up the test environment
    std::string mime = "application/x-subrip";
    // 2. Call the function to be tested
    bool result = demuxerPluginManager_->IsSubtitleMime(mime);
    // 3. Verify the result
    EXPECT_EQ(result, true);

    // 1. Set up the test environment
    mime = "text/vtt";
    // 2. Call the function to be tested
    result = demuxerPluginManager_->IsSubtitleMime(mime);
    // 3. Verify the result
    EXPECT_EQ(result, true);

    // 1. Set up the test environment
    mime = "video/mp4";
    // 2. Call the function to be tested
    result = demuxerPluginManager_->IsSubtitleMime(mime);
    // 3. Verify the result
    EXPECT_EQ(result, false);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetTrackTypeByTrackID_001, TestSize.Level1)
{
    // 1. Set up the test environment
    int32_t trackId = 0;
    std::vector<Meta> tracks;
    Meta meta;
    meta.SetData(Tag::MIME_TYPE, "audio/mpeg");
    tracks.push_back(meta);
    Plugins::MediaInfo mediaInfo;
    mediaInfo.tracks = tracks;
    demuxerPluginManager_->curMediaInfo_ = mediaInfo;

    // 2. Call the function to be tested
    TrackType result = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);

    // 3. Verify the result
    EXPECT_EQ(result, TRACK_AUDIO);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetTrackTypeByTrackID_002, TestSize.Level1)
{
    // 1. Set up the test environment
    int32_t trackId = 0;
    std::vector<Meta> tracks;
    Meta meta;
    meta.SetData(Tag::MIME_TYPE, "video/mpeg");
    tracks.push_back(meta);
    Plugins::MediaInfo mediaInfo;
    mediaInfo.tracks = tracks;
    demuxerPluginManager_->curMediaInfo_ = mediaInfo;

    // 2. Call the function to be tested
    TrackType result = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);

    // 3. Verify the result
    EXPECT_EQ(result, TRACK_VIDEO);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetTrackTypeByTrackID_003, TestSize.Level1)
{
    // 1. Set up the test environment
    int32_t trackId = 0;
    std::vector<Meta> tracks;
    Meta meta;
    meta.SetData(Tag::MIME_TYPE, "application/x-subrip");
    tracks.push_back(meta);
    Plugins::MediaInfo mediaInfo;
    mediaInfo.tracks = tracks;
    demuxerPluginManager_->curMediaInfo_ = mediaInfo;

    // 2. Call the function to be tested
    TrackType result = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);

    // 3. Verify the result
    EXPECT_EQ(result, TRACK_SUBTITLE);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetTrackTypeByTrackID_004, TestSize.Level1)
{
    // 1. Set up the test environment
    int32_t trackId = 0;
    std::vector<Meta> tracks;
    Meta meta;
    meta.SetData(Tag::MIME_TYPE, "unknown/type");
    tracks.push_back(meta);
    Plugins::MediaInfo mediaInfo;
    mediaInfo.tracks = tracks;
    demuxerPluginManager_->curMediaInfo_ = mediaInfo;

    // 2. Call the function to be tested
    TrackType result = demuxerPluginManager_->GetTrackTypeByTrackID(trackId);

    // 3. Verify the result
    EXPECT_EQ(result, TRACK_INVALID);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetStreamTypeByTrackID_001, TestSize.Level1)
{
    int32_t trackId = 1;
    std::map<int32_t, MediaTrackMap> mediaTrackMap;
    mediaTrackMap[1].streamID = 1;
    demuxerPluginManager_->trackInfoMap_ = mediaTrackMap;
    demuxerPluginManager_->streamInfoMap_[1].type = StreamType::VIDEO;
    // 2. Call the function to be tested
    StreamType result = demuxerPluginManager_->GetStreamTypeByTrackID(trackId);

    // 3. Verify the result
    EXPECT_EQ(result, StreamType::VIDEO);
}

HWTEST_F(DemuxerPluginManagerUnitTest, UpdateDefaultStreamID_001, TestSize.Level1)
{
    // 1. Set up the test environment
    Plugins::MediaInfo mediaInfo;
    int32_t newStreamID = 1; // Assuming streamID 1 is valid

    // 2. Call the function to be tested
    Status result = demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, StreamType::AUDIO, newStreamID);

    // 3. Verify the result
    EXPECT_EQ(demuxerPluginManager_->curAudioStreamID_, newStreamID);
    EXPECT_EQ(result, Status::OK);
}

HWTEST_F(DemuxerPluginManagerUnitTest, UpdateDefaultStreamID_002, TestSize.Level1)
{
    // 1. Set up the test environment
    Plugins::MediaInfo mediaInfo;
    int32_t newStreamID = 1; // Assuming streamID 1 is valid

    // 2. Call the function to be tested
    Status result = demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, StreamType::SUBTITLE, newStreamID);

    // 3. Verify the result
    EXPECT_EQ(demuxerPluginManager_->curSubTitleStreamID_, newStreamID);
    EXPECT_EQ(result, Status::OK);
}

HWTEST_F(DemuxerPluginManagerUnitTest, UpdateDefaultStreamID_003, TestSize.Level1)
{
    // 1. Set up the test environment
    Plugins::MediaInfo mediaInfo;
    int32_t newStreamID = 1; // Assuming streamID 1 is valid

    // 2. Call the function to be tested
    Status result = demuxerPluginManager_->UpdateDefaultStreamID(mediaInfo, StreamType::VIDEO, newStreamID);

    // 3. Verify the result
    EXPECT_EQ(demuxerPluginManager_->curVideoStreamID_, newStreamID);
    EXPECT_EQ(result, Status::OK);
}

HWTEST_F(DemuxerPluginManagerUnitTest, SetResetEosStatus_001, TestSize.Level1)
{
    // 1. Set up the test environment
    bool flag = true;

    // 2. Call the function to be tested
    demuxerPluginManager_->SetResetEosStatus(flag);

    // 3. Verify the result
    EXPECT_EQ(demuxerPluginManager_->needResetEosStatus_, flag);
}

HWTEST_F(DemuxerPluginManagerUnitTest, ReadAt_001, TestSize.Level1)
{
    int64_t offset = 1;
    std::shared_ptr<Buffer> buffer = nullptr;
    size_t expectedLen = 0;
    dataSourceImpl_->stream_->seekable_ = Plugins::Seekable::UNSEEKABLE;
    dataSourceImpl_->stream_->mediaDataSize_  = 0;
    EXPECT_CALL(*streamDemuxer_, CallbackReadAt).WillRepeatedly(Return(Status::OK));
    EXPECT_EQ(dataSourceImpl_->ReadAt(offset, buffer, expectedLen), Status::ERROR_UNKNOWN);
    buffer = std::shared_ptr<Buffer>();
    EXPECT_EQ(dataSourceImpl_->ReadAt(offset, buffer, expectedLen), Status::ERROR_UNKNOWN);
    buffer = nullptr;
    dataSourceImpl_->stream_->seekable_ = Plugins::Seekable::SEEKABLE;
    dataSourceImpl_->stream_->mediaDataSize_  = 0;
    offset = 1;
    EXPECT_EQ(dataSourceImpl_->ReadAt(offset, buffer, expectedLen), Status::ERROR_UNKNOWN);
}

HWTEST_F(DemuxerPluginManagerUnitTest, UpdateTempTrackMapInfo_001, TestSize.Level1)
{
    int32_t oldTrackId = 0;
    int32_t newTrackId = 0;
    int32_t newInnerTrackIndex = 0;
    demuxerPluginManager_->temp2TrackInfoMap_[oldTrackId] = MediaTrackMap();
    demuxerPluginManager_->temp2TrackInfoMap_[oldTrackId].streamID = 0;
    demuxerPluginManager_->temp2TrackInfoMap_[oldTrackId].innerTrackIndex = 0;
    demuxerPluginManager_->temp2TrackInfoMap_[newTrackId] = MediaTrackMap();
    demuxerPluginManager_->temp2TrackInfoMap_[newTrackId].streamID = 0;
    demuxerPluginManager_->temp2TrackInfoMap_[newTrackId].innerTrackIndex = 0;
    demuxerPluginManager_->UpdateTempTrackMapInfo(oldTrackId, newTrackId, newInnerTrackIndex);
    EXPECT_EQ(demuxerPluginManager_->temp2TrackInfoMap_[oldTrackId].innerTrackIndex, newInnerTrackIndex);
}

HWTEST_F(DemuxerPluginManagerUnitTest, SingleStreamSeekTo_001, TestSize.Level1)
{
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;
    int64_t seekTime = 0;
    int32_t streamID = 0;
    int64_t realSeekTime = 0;
    demuxerPluginManager_->streamInfoMap_[0] = MediaStreamInfo();
    demuxerPluginManager_->streamInfoMap_[0].plugin = nullptr;
    // 110
    EXPECT_EQ(demuxerPluginManager_->SingleStreamSeekTo(seekTime, mode, streamID, realSeekTime), Status::OK);
    // 100
    streamID = 2;
    EXPECT_EQ(demuxerPluginManager_->SingleStreamSeekTo(seekTime, mode, streamID, realSeekTime), Status::OK);
    // 000
    streamID = -1;
    EXPECT_EQ(demuxerPluginManager_->SingleStreamSeekTo(seekTime, mode, streamID, realSeekTime), Status::OK);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetCurrentBitRate_001, TestSize.Level1)
{
    uint32_t bitRate = 10;
    uint32_t ret = 0;
    demuxerPluginManager_->curVideoStreamID_ = 1;
    demuxerPluginManager_->streamInfoMap_[demuxerPluginManager_->curVideoStreamID_] = MediaStreamInfo();
    demuxerPluginManager_->streamInfoMap_[demuxerPluginManager_->curVideoStreamID_].bitRate = bitRate;
    demuxerPluginManager_->isDash_ = true;
    // 11
    EXPECT_EQ(demuxerPluginManager_->GetCurrentBitRate(), bitRate);
    // 10
    demuxerPluginManager_->curVideoStreamID_ = -1;
    EXPECT_EQ(demuxerPluginManager_->GetCurrentBitRate(), ret);
    // 00
    demuxerPluginManager_->isDash_ = false;
    EXPECT_EQ(demuxerPluginManager_->GetCurrentBitRate(), ret);
    // 01
    demuxerPluginManager_->curVideoStreamID_ = 1;
    EXPECT_EQ(demuxerPluginManager_->GetCurrentBitRate(), ret);
}

HWTEST_F(DemuxerPluginManagerUnitTest, RebootPlugin_001, TestSize.Level1)
{
    int32_t streamId = 0;
    TrackType trackType = TRACK_AUDIO;
    bool isRebooted;
    demuxerPluginManager_->streamInfoMap_[0] = MediaStreamInfo();
    demuxerPluginManager_->streamInfoMap_[1] = MediaStreamInfo();
    EXPECT_CALL(*streamDemuxer_, ResetCache).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*streamDemuxer_, SetDemuxerState).WillRepeatedly(Return());
    std::string str = "";
    EXPECT_CALL(*streamDemuxer_, SnifferMediaType).WillRepeatedly(Return(str));
    streamId = 1;
    EXPECT_EQ(demuxerPluginManager_->RebootPlugin(streamId, trackType, streamDemuxer_, isRebooted),
        Status::ERROR_INVALID_PARAMETER);
    trackType = TRACK_INVALID;
    EXPECT_EQ(demuxerPluginManager_->RebootPlugin(streamId, trackType, streamDemuxer_, isRebooted),
        Status::ERROR_INVALID_PARAMETER);
    streamId = 0;
    EXPECT_EQ(demuxerPluginManager_->RebootPlugin(streamId, trackType, streamDemuxer_, isRebooted),
        Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetStreamDemuxerNewStreamID_001, TestSize.Level1)
{
    int32_t newStreamID = 1;
    EXPECT_CALL(*streamDemuxer_, GetNewAudioStreamID()).WillRepeatedly(Return(newStreamID));
    EXPECT_CALL(*streamDemuxer_, GetNewSubtitleStreamID()).WillRepeatedly(Return(newStreamID));
    EXPECT_CALL(*streamDemuxer_, GetNewVideoStreamID()).WillRepeatedly(Return(newStreamID));
    EXPECT_EQ(streamDemuxer_->GetNewAudioStreamID(), newStreamID);
    TrackType trackType = TRACK_AUDIO;
    EXPECT_EQ(demuxerPluginManager_->GetStreamDemuxerNewStreamID(trackType, streamDemuxer_), newStreamID);
    trackType = TRACK_SUBTITLE;
    EXPECT_EQ(demuxerPluginManager_->GetStreamDemuxerNewStreamID(trackType, streamDemuxer_), newStreamID);
    trackType = TRACK_VIDEO;
    EXPECT_EQ(demuxerPluginManager_->GetStreamDemuxerNewStreamID(trackType, streamDemuxer_), newStreamID);
}

HWTEST_F(DemuxerPluginManagerUnitTest, StartPlugin_001, TestSize.Level1)
{
    int32_t streamId = 0;
    EXPECT_EQ(demuxerPluginManager_->StartPlugin(streamId, streamDemuxer_), Status::OK);
}

HWTEST_F(DemuxerPluginManagerUnitTest, StopPlugin_001, TestSize.Level1)
{
    int32_t streamId = 0;
    EXPECT_CALL(*streamDemuxer_, ResetCache).WillRepeatedly(Return(Status::OK));
    EXPECT_EQ(demuxerPluginManager_->StopPlugin(streamId, streamDemuxer_), Status::OK);
    demuxerPluginManager_->streamInfoMap_[streamId] = MediaStreamInfo();
    demuxerPluginManager_->streamInfoMap_[streamId].plugin = nullptr;
    EXPECT_EQ(demuxerPluginManager_->StopPlugin(streamId, streamDemuxer_), Status::OK);
}

HWTEST_F(DemuxerPluginManagerUnitTest, GetStreamIDByTrackType_001, TestSize.Level1)
{
    TrackType trackType = TRACK_VIDEO;
    demuxerPluginManager_->curVideoStreamID_ = 1;
    EXPECT_EQ(demuxerPluginManager_->GetStreamIDByTrackType(trackType), demuxerPluginManager_->curVideoStreamID_);
    trackType = TRACK_AUDIO;
    demuxerPluginManager_->curAudioStreamID_ = 2;
    EXPECT_EQ(demuxerPluginManager_->GetStreamIDByTrackType(trackType), demuxerPluginManager_->curAudioStreamID_);
    trackType = TRACK_SUBTITLE;
    demuxerPluginManager_->curSubTitleStreamID_ = 3;
    EXPECT_EQ(demuxerPluginManager_->GetStreamIDByTrackType(trackType), demuxerPluginManager_->curSubTitleStreamID_);
    trackType = TRACK_INVALID;
    EXPECT_EQ(demuxerPluginManager_->GetStreamIDByTrackType(trackType), -1);
}

HWTEST_F(DemuxerPluginManagerUnitTest, SnifferMediaType_Timeout, TestSize.Level1)
{
    int32_t streamId = 0;
    EXPECT_CALL(*streamDemuxer_, SnifferMediaType(_))
        .WillRepeatedly([](int32_t streamID) {
            std::this_thread::sleep_for(std::chrono::milliseconds(300));
            return "DEMUXER";
        });
    ASSERT_NE(demuxerPluginManager_->LoadDemuxerPlugin(streamId, streamDemuxer_), Status::OK);
}

bool DemuxerPluginManagerUnitTest::CreateDataSource(const std::string& filePath)
{
    mediaSource_ = std::make_shared<MediaSource>(filePath);
    realSource_ = std::make_shared<Source>();
    realSource_->SetSource(mediaSource_);

    realStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    realStreamDemuxer_->SetSourceType(Plugins::SourceType::SOURCE_TYPE_URI);
    realStreamDemuxer_->SetSource(realSource_);
    realStreamDemuxer_->Init(filePath);

    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    dataSourceImpl_ = std::make_shared<DataSourceImpl>(realStreamDemuxer_, streamId_);
    realSource_->NotifyInitSuccess();

    return true;
}

bool DemuxerPluginManagerUnitTest::CreateDemuxerPluginByName(
    const std::string& typeName, const std::string& filePath, int probSize)
{
    if (!CreateDataSource(filePath)) {
        printf("CreateDataSource failed for file: %s\n", filePath.c_str());
        return false;
    }
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(typeName);
    if (pluginBase_ == nullptr) {
        printf("CreatePluginByName failed for type: %s\n", typeName.c_str());
        return false;
    }
    demuxerPlugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (demuxerPlugin_->SetDataSourceWithProbSize(dataSourceImpl_, probSize) != Status::OK) {
        printf("SetDataSourceWithProbSize failed for type: %s\n", typeName.c_str());
        return false;
    }

    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);

    return true;
}

bool DemuxerPluginManagerUnitTest::PluginSelectTracks()
{
    MediaInfo mediaInfo;
    if (demuxerPlugin_->GetMediaInfo(mediaInfo) != Status::OK) {
        printf("GetMediaInfo failed for plugin\n");
        return false;
    }

    for (size_t i = 0; i < mediaInfo.tracks.size(); i++) {
        std::string mime;
        mediaInfo.tracks[i].GetData(Tag::MIME_TYPE, mime);
        if (mime.find("video/") == 0 || mime.find("audio/") == 0 ||
            mime.find("application/") == 0 || mime.find("text/vtt") == 0) {
            demuxerPlugin_->SelectTrack(static_cast<uint32_t>(i));
            selectedTrackIds_.push_back(static_cast<uint32_t>(i));
            frames_[i] = 0;
            keyFrames_[i] = 0;
            eosFlag_[i] = false;
        }
    }

    return true;
}

bool DemuxerPluginManagerUnitTest::PluginReadSample(uint32_t idx, uint32_t& flag)
{
    int bufSize = 0;
    demuxerPlugin_->GetNextSampleSize(idx, bufSize);
    if (static_cast<uint32_t>(bufSize) > buffer_.size()) {
        buffer_.resize(bufSize + BUFFER_PADDING_SIZE);
    }

    auto avBuf = AVBuffer::CreateAVBuffer(buffer_.data(), bufSize, bufSize);
    if (avBuf == nullptr) {
        printf("CreateAVBuffer failed for index: %u\n", idx);
        return false;
    }
    
    demuxerPlugin_->ReadSample(idx, avBuf);
    flag = avBuf->flag_;

    return true;
}

void DemuxerPluginManagerUnitTest::CountFrames(uint32_t index, uint32_t flag)
{
    if (flag & static_cast<uint32_t>(AVBufferFlag::EOS)) {
        eosFlag_[index] = true;
        return;
    }

    if (flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) {
        keyFrames_[index]++;
        frames_[index]++;
    } else if ((flag & static_cast<uint32_t>(AVBufferFlag::NONE)) == static_cast<uint32_t>(AVBufferFlag::NONE)) {
        frames_[index]++;
    } else {
        SetEosValue();
        printf("flag is unknown, read sample break");
    }
}

bool DemuxerPluginManagerUnitTest::isEOS(map<uint32_t, bool>& countFlag)
{
    auto ret = std::find_if(countFlag.begin(), countFlag.end(), [](const pair<uint32_t, bool>& p) {
        return p.second == false;
    });

    return ret == countFlag.end();
}

void DemuxerPluginManagerUnitTest::SetEosValue()
{
    std::for_each(selectedTrackIds_.begin(), selectedTrackIds_.end(), [this](uint32_t idx) {
        eosFlag_[idx] = true;
    });
}

void DemuxerPluginManagerUnitTest::RemoveValue()
{
    if (!frames_.empty()) {
        frames_.clear();
    }
    if (!keyFrames_.empty()) {
        keyFrames_.clear();
    }
    if (!eosFlag_.empty()) {
        eosFlag_.clear();
    }
    if (!selectedTrackIds_.empty()) {
        selectedTrackIds_.clear();
    }
}

bool DemuxerPluginManagerUnitTest::ResultAssert(
    uint32_t frames0, uint32_t frames1, uint32_t keyFrames0, uint32_t keyFrames1)
{
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    if (frames_[0] != frames0) {
        printf("Expected frames_[0] = %d, but got %d\n", frames0, frames_[0]);
        return false;
    }
    if (frames_[1] != frames1) {
        printf("Expected frames_[1] = %d, but got %d\n", frames1, frames_[1]);
        return false;
    }
    if (keyFrames_[0] != keyFrames0) {
        printf("Expected keyFrames_[0] = %d, but got %d\n", keyFrames0, keyFrames_[0]);
        return false;
    }
    if (keyFrames_[1] != keyFrames1) {
        printf("Expected keyFrames_[1] = %d, but got %d\n", keyFrames1, keyFrames_[1]);
        return false;
    }

    return true;
}

bool DemuxerPluginManagerUnitTest::ResultAssert(std::vector<uint32_t> &framesExpect,
    std::vector<uint32_t> &keyFramesExpect)
{
    for (size_t i = 0; i < framesExpect.size(); i++) {
        if (frames_.find(i) == frames_.end() || keyFrames_.find(i) == keyFrames_.end()) {
            printf("Index %zu not found in frames_ or keyFrames_\n", i);
            continue;
        }
        printf("frames_[%zu]=%d | kFrames_[%zu]=%d\n", i, frames_[i], i, keyFrames_[i]);
        if (frames_[i] != framesExpect[i]) {
            printf("Expected frames_[%zu] = %u, but got %d\n", i, framesExpect[i], frames_[i]);
            return false;
        }
        if (keyFrames_[i] != keyFramesExpect[i]) {
            printf("Expected keyFrames_[%zu] = %u, but got %d\n", i, keyFramesExpect[i], keyFrames_[i]);
            return false;
        }
    }
    return true;
}

bool DemuxerPluginManagerUnitTest::PluginReadAllSample()
{
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            uint32_t flag = 0;
            if (!PluginReadSample(idx, flag)) {
                printf("PluginReadSample failed for index: %u\n", idx);
                return false;
            }
            CountFrames(idx, flag);
        }
    }

    return true;
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0001, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1293, 0, 1293, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0002, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMR, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1501, 0, 1501, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0003, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1501, 0, 1501, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0004, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1500, 0, 1500, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0005, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(7, 0, 7, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0006, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(313, 0, 313, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0007, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(76, 113, 1, 113), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0008, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(240, 199, 4, 199), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0009, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(602, 434, 3, 434), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0010, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1251, 0, 1251, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0011, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1253, 2164, 19, 2164), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0012, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(103, 174, 5, 174), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0013, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(602, 433, 3, 433), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0014, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(5, 0, 5, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0015, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(4, 0, 4, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0016, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1598, 0, 1598, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0017, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(704, 0, 704, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0018, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_RM, TEST_FILE_URI_RM, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AC3, TEST_FILE_URI_AC3, DEF_PROB_SIZE), true);
}


HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0019, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1875, 0, 1875, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0020, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(604, 433, 3, 433), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0021, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(433, 0, 433, 0), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0022, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, 0), false);
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, 4), false);
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, 512), true);
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0023, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AMR, DEF_PROB_SIZE), false);
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_APE, DEF_PROB_SIZE), false);
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP4, DEF_PROB_SIZE), false);
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FLV, DEF_PROB_SIZE), false);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0024, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_ERR, TEST_FILE_URI_AAC, DEF_PROB_SIZE), false);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, CreateDemuxerPluginByName_0025, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(demuxerPlugin_->SetDataSourceWithProbSize(dataSourceImpl_, DEF_PROB_SIZE), Status::ERROR_WRONG_STATE);
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, Demuxer_Mp4InitCheck_0001, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_2, DEF_PROB_SIZE), true);
}

HWTEST_F(DemuxerPluginManagerUnitTest, Demuxer_SeekToStart_0001, TestSize.Level1)
{
    for (auto &item : TEST_LIST) {
        printf("#####pluginName: %s, testFile: %s#####\n", item.pluginName.c_str(), item.testFile.c_str());
        ASSERT_EQ(CreateDemuxerPluginByName(item.pluginName.c_str(), item.testFile.c_str(), DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(PluginReadAllSample(), true);
        ASSERT_EQ(ResultAssert(item.frameCnt, item.keyFrameCnt), true);
        RemoveValue();

        printf("SeekToStart:\n");
        ASSERT_EQ(demuxerPlugin_->SeekToStart(), Status::OK);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(PluginReadAllSample(), true);
        ASSERT_EQ(ResultAssert(item.frameCnt, item.keyFrameCnt), true);
        RemoveValue();
    }
}

HWTEST_F(DemuxerPluginManagerUnitTest, Demuxer_SeekToStart_0002, TestSize.Level1)
{
    for (auto &item : TEST_LIST2) {
        printf("#####pluginName: %s, testFile: %s#####\n", item.pluginName.c_str(), item.testFile.c_str());
        ASSERT_EQ(CreateDemuxerPluginByName(item.pluginName.c_str(), item.testFile.c_str(), DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(PluginReadAllSample(), true);
        EXPECT_EQ(ResultAssert(item.frameCnt, item.keyFrameCnt), true);
        RemoveValue();

        printf("SeekToStart:\n");
        ASSERT_EQ(demuxerPlugin_->SeekToStart(), Status::OK);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(PluginReadAllSample(), true);
        EXPECT_EQ(ResultAssert(item.frameCnt, item.keyFrameCnt), true);
        RemoveValue();
    }
}

HWTEST_F(DemuxerPluginManagerUnitTest, Demuxer_SeekToStart_0003, TestSize.Level1)
{
    TEST_LIST.insert(TEST_LIST.end(), TEST_LIST2.begin(), TEST_LIST2.end());
    for (auto &item : TEST_LIST) {
        printf("#####pluginName: %s, testFile: %s#####\n", item.pluginName.c_str(), item.testFile.c_str());
        ASSERT_EQ(CreateDemuxerPluginByName(item.pluginName.c_str(), item.testFile.c_str(), DEF_PROB_SIZE), true);

        printf("SeekToStart:\n");
        ASSERT_EQ(demuxerPlugin_->SeekToStart(), Status::OK);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(PluginReadAllSample(), true);
        EXPECT_EQ(ResultAssert(item.frameCnt, item.keyFrameCnt), true);
        RemoveValue();
    }
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0001, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    uint32_t flag = 0;
    std::vector<int64_t> seekTimes = {0, 10, 50, 100};
    std::vector<int64_t> expReadSeekTimes = {1400000000, 2400000000, 2400000000, 2400000000};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            Status::OK);
        ASSERT_EQ(PluginReadSample(0, flag), true);
        ASSERT_EQ(flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME),
            static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME));
        ASSERT_EQ(realSeekTime, expReadSeekTimes[i]);
    }
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0002, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    std::vector<int64_t> seekTimes = {4000, 4010, 4080, 4500};
    std::vector<Status> expStatuses = {Status::OK, Status::END_OF_STREAM, Status::END_OF_STREAM, Status::END_OF_STREAM};
    std::vector<int64_t> expReadSeekTimes = {5400000000, 5480000000, 5480000000, 5480000000};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            expStatuses[i]);
        ASSERT_EQ(realSeekTime, expReadSeekTimes[i]);
    }
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0003, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    std::vector<int64_t> seekTimes = {4000, 3900, 3900, 3800, 3800, 3700, 3700, 3600, 3600, 3500, 3500, 3400, 3400};
    int64_t expReadSeekTime = 5400000000;
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            Status::OK);
        ASSERT_EQ(realSeekTime, expReadSeekTime);
    }
    RemoveValue();
}
HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0004, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    std::vector<int64_t> seekTimes = {0, 200, 1000, 2000, 3000, 3100, 3320, 3480, 3640, 4120, 5520};
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    std::vector<Status> expStatuses = {Status::OK, Status::OK, Status::OK, Status::OK, Status::OK, Status::OK, Status::OK, Status::OK, Status::OK,
        Status::END_OF_STREAM, Status::END_OF_STREAM};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            expStatuses[i]);
    }
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0005, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    std::vector<int64_t> seekTimes = {4120, 4120};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            Status::END_OF_STREAM);
    }
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0006, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    std::vector<int64_t> seekTimes = {1000, 2000};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime, timeout),
            Status::ERROR_INVALID_PARAMETER);
    }
    RemoveValue();
}
HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0007, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    uint32_t flag = 0;
    std::vector<int64_t> seekTimes = {0, 10, 50, 100, 3320, 3200};
    std::vector<int64_t> expReadSeekTimes = {1400000000, 2400000000, 2400000000, 2400000000, 5400000000, 5400000000};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            Status::OK);
        ASSERT_EQ(PluginReadSample(0, flag), true);
        ASSERT_EQ(flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME),
            static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME));
        ASSERT_EQ(realSeekTime, expReadSeekTimes[i]);
    }
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0008, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    std::vector<int64_t> seekTimes = {4000, 4010, 4080, 4500};
    std::vector<Status> expStatuses = {Status::OK, Status::END_OF_STREAM, Status::END_OF_STREAM, Status::END_OF_STREAM};
    std::vector<int64_t> expReadSeekTimes = {5400000000, 5480000000, 5480000000, 5480000000};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            expStatuses[i]);
        ASSERT_EQ(realSeekTime, expReadSeekTimes[i]);
    }
    RemoveValue();
}
HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0009, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS_3, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t timeout = 100;
    std::vector<int64_t> seekTimes = {27000, 26000, 25500};
    std::vector<Status> expStatuses = {Status::END_OF_STREAM, Status::END_OF_STREAM, Status::END_OF_STREAM};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout),
            expStatuses[i]);
    }
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0010, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS_3, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    std::vector<uint32_t> timeout = {1, 1, 1, 1, 0};
    std::vector<int64_t> seekTimes = {100, 100, 100, 100, 100};
    std::vector<Status> expStatuses = {Status::ERROR_WAIT_TIMEOUT, Status::ERROR_WAIT_TIMEOUT,
        Status::ERROR_WAIT_TIMEOUT, Status::ERROR_WAIT_TIMEOUT, Status::OK};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout[i]),
            expStatuses[i]);
    }
    RemoveValue();
}

HWTEST_F(DemuxerPluginManagerUnitTest, SeekToKeyFrame_0011, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS_3, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    std::vector<uint32_t> timeout = {1, 1, 1, 1, 0};
    std::vector<int64_t> seekTimes = {170, 160, 150, 140, 100};
    std::vector<Status> expStatuses = {Status::ERROR_WAIT_TIMEOUT, Status::ERROR_WAIT_TIMEOUT,
        Status::ERROR_WAIT_TIMEOUT, Status::ERROR_WAIT_TIMEOUT, Status::OK};
    for (size_t i = 0; i < seekTimes.size(); ++i) {
        ASSERT_EQ(demuxerPlugin->SeekToKeyFrame(0, seekTimes[i], SeekMode::SEEK_NEXT_SYNC, realSeekTime, timeout[i]),
            expStatuses[i]);
    }
    RemoveValue();
}
}
}