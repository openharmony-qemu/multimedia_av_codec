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
#include "stream_demuxer.h"
#include "demuxer_plugin_manager.h"

#define LOCAL true

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;

namespace OHOS {
namespace Media {
void DemuxerPluginManagerUnitTest::SetUpTestCase(void) {}

void DemuxerPluginManagerUnitTest::TearDownTestCase(void) {}

void DemuxerPluginManagerUnitTest::SetUp(void)
{
    demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
    std::shared_ptr<StreamDemuxer> streamDemuxer = std::make_shared<StreamDemuxer>();
    int32_t streamId = 0;
    dataSourceImpl_ = std::make_shared<DataSourceImpl>(streamDemuxer, streamId);
}

void DemuxerPluginManagerUnitTest::TearDown(void)
{
    demuxerPluginManager_ = nullptr;
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
    std::shared_ptr<StreamDemuxer> streamDemuxer = std::make_shared<StreamDemuxer>();
    Plugins::MediaInfo mediaInfo;
    Status status = demuxerPluginManager_->LoadCurrentSubtitlePlugin(streamDemuxer, mediaInfo);

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
}
}