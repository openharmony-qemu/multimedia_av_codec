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
    int32_t newStreamID = 1;
    TrackType trackType = TRACK_AUDIO;
    bool isRebooted;
    demuxerPluginManager_->streamInfoMap_[0] = MediaStreamInfo();
    demuxerPluginManager_->streamInfoMap_[1] = MediaStreamInfo();
    EXPECT_CALL(*streamDemuxer_, ResetCache).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*streamDemuxer_, SetDemuxerState).WillRepeatedly(Return());
    std::string str = "";
    EXPECT_CALL(*streamDemuxer_, SnifferMediaType).WillRepeatedly(Return(str));
    EXPECT_CALL(*streamDemuxer_, GetNewAudioStreamID()).WillRepeatedly(Return(newStreamID));
    EXPECT_EQ(demuxerPluginManager_->RebootPlugin(streamId, trackType, streamDemuxer_, isRebooted),
        Status::OK);
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
}
}