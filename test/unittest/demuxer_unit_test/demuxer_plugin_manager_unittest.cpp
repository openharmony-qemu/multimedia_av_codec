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

#include "demuxer_plugin_manager_unittest.h"
#include "demuxer_plugin.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;

namespace OHOS {
namespace Media {
static const int32_t INVALID_NUM = 0;
static const int32_t ID_TEST = 0;
static const int32_t INVALID_ID = -1;
static const int32_t INVALID_TYPE = 5;
constexpr int32_t API_VERSION_18 = 18;
constexpr int32_t VIDEO_STREAM_ID_TEST = 1;
constexpr int32_t TEST_BIT_RATE = 1000;
constexpr int32_t TEST_VIDEO_WIDTH = 1920;
constexpr int32_t TEST_VIDEO_HEIGHT = 1080;
constexpr int32_t SUBTITLE_STREAM_ID_TEST = 2;
constexpr int64_t TEST_SEEK_TIME = 100;

void DemuxerPluginManagerUnittest::SetUpTestCase(void) {}

void DemuxerPluginManagerUnittest::TearDownTestCase(void) {}

void DemuxerPluginManagerUnittest::SetUp(void)
{
    demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
    streamDemuxer_ = std::make_shared<MockBaseStreamDemuxer>();
    int32_t streamId = ID_TEST;
    dataSourceImpl_ = std::make_shared<DataSourceImpl>(streamDemuxer_, streamId);
    dataSourceImpl_->stream_ = streamDemuxer_;
}

void DemuxerPluginManagerUnittest::TearDown(void)
{
    demuxerPluginManager_ = nullptr;
    streamDemuxer_ = nullptr;
    dataSourceImpl_ = nullptr;
}

 /**
 * @tc.name  : Test IsOffsetValid  API
 * @tc.number: IsOffsetValid_001
 * @tc.desc  : Test stream_->seekable_ != Plugins::Seekable::SEEKABLE
 */
HWTEST_F(DemuxerPluginManagerUnittest, IsOffsetValid_001, TestSize.Level1)
{
    ASSERT_NE(dataSourceImpl_, nullptr);
    dataSourceImpl_->stream_->seekable_ = Plugins::Seekable::INVALID;
    size_t offset = INVALID_NUM;
    auto ret = dataSourceImpl_->IsOffsetValid(offset);
    EXPECT_EQ(ret, true);
}

 /**
 * @tc.name  : Test ReadAt  API
 * @tc.number: ReadAt_001
 * @tc.desc  : Test return Status::ERROR_UNKNOWN
 */
HWTEST_F(DemuxerPluginManagerUnittest, ReadAt_001, TestSize.Level1)
{
    ASSERT_NE(dataSourceImpl_, nullptr);
    dataSourceImpl_->stream_->seekable_ = Plugins::Seekable::INVALID;
    int64_t offset = INVALID_NUM;
    std::shared_ptr<Buffer> buffer;
    size_t expectedLen = INVALID_NUM;
    auto ret = dataSourceImpl_->ReadAt(offset, buffer, expectedLen);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
* @tc.name  : Test InitAudioTrack  API
* @tc.number: InitAudioTrack_001
* @tc.desc  : Test apiVersion_ >= API_VERSION_16
*             Test apiVersion_ >= API_VERSION_18
*/
HWTEST_F(DemuxerPluginManagerUnittest, InitAudioTrack_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    StreamInfo info;
    info.streamId = ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = INVALID_NUM;
    demuxerPluginManager_->apiVersion_ = API_VERSION_18;
    MediaStreamInfo mediaInfo;
    demuxerPluginManager_->streamInfoMap_[0] = mediaInfo;
    demuxerPluginManager_->InitAudioTrack(info);
    EXPECT_EQ(demuxerPluginManager_->streamInfoMap_[0].mediaInfo.tracks.empty(), false);
}

/**
* @tc.name  : Test InitVideoTrack  API
* @tc.number: InitVideoTrack_001
* @tc.desc  : Test apiVersion_ >= API_VERSION_16
*             Test apiVersion_ >= API_VERSION_18
*/
HWTEST_F(DemuxerPluginManagerUnittest, InitVideoTrack_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    StreamInfo info;
    info.streamId = ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = INVALID_NUM;
    demuxerPluginManager_->apiVersion_ = API_VERSION_18;
    MediaStreamInfo mediaInfo;
    demuxerPluginManager_->streamInfoMap_[0] = mediaInfo;
    demuxerPluginManager_->InitVideoTrack(info);
    EXPECT_EQ(demuxerPluginManager_->streamInfoMap_[0].type, VIDEO);
}

/**
* @tc.name  : Test InitDefaultPlay  API
* @tc.number: InitDefaultPlay_001
* @tc.desc  : Test isHlsFmp4_ == true
*             Test info2.type = INVALID_TYPE
*/
HWTEST_F(DemuxerPluginManagerUnittest, InitDefaultPlay_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    StreamInfo info;
    info.type = MIXED;
    info.streamId = INVALID_NUM;
    demuxerPluginManager_->isHlsFmp4_ = true;
    demuxerPluginManager_->curVideoStreamID_ = INVALID_ID;
    MediaStreamInfo mediaInfo;
    demuxerPluginManager_->streamInfoMap_[0] = mediaInfo;
    std::vector<StreamInfo> streams;
    streams.push_back(info);
    StreamInfo info2;
    info2.type = static_cast<OHOS::Media::Plugins::StreamType>(INVALID_TYPE);
    info2.streamId = INVALID_NUM;
    streams.push_back(info2);
    auto ret = demuxerPluginManager_->InitDefaultPlay(streams);
    EXPECT_EQ(demuxerPluginManager_->curVideoStreamID_, info.streamId);
    EXPECT_EQ(demuxerPluginManager_->isDash_, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test GetTrackInfoByStreamID  API
* @tc.number: GetTrackInfoByStreamID_001
* @tc.desc  : Test all
*/
HWTEST_F(DemuxerPluginManagerUnittest, GetTrackInfoByStreamID_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    int32_t streamID = ID_TEST;
    int32_t trackId = ID_TEST;
    int32_t innerTrackId = ID_TEST;
    TrackType type = TrackType::TRACK_AUDIO;
    MediaTrackMap mediaTrackMap;
    mediaTrackMap.streamID = ID_TEST;
    mediaTrackMap.innerTrackIndex = INVALID_ID;
    demuxerPluginManager_->trackInfoMap_[0] = mediaTrackMap;
    Meta meta;
    meta.SetData(Tag::MIME_TYPE, "audio_test");
    demuxerPluginManager_->curMediaInfo_.tracks.push_back(meta);
    demuxerPluginManager_->GetTrackInfoByStreamID(streamID, trackId, innerTrackId, type);
    EXPECT_EQ(innerTrackId, INVALID_ID);
}

/**
* @tc.name  : Test LoadCurrentAllPlugin  API
* @tc.number: LoadCurrentAllPlugin_001
* @tc.desc  : Test curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID
*             Test curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID
*/
HWTEST_F(DemuxerPluginManagerUnittest, LoadCurrentAllPlugin_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    auto mockStreamDemuxer = std::make_shared<MockBaseStreamDemuxer>();
    EXPECT_CALL(*(mockStreamDemuxer), SnifferMediaType(_)).WillRepeatedly(testing::Return(""));
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer = mockStreamDemuxer;
    Plugins::MediaInfo mediaInfo;
    demuxerPluginManager_->curAudioStreamID_ = ID_TEST;
    auto ret = demuxerPluginManager_->LoadCurrentAllPlugin(streamDemuxer, mediaInfo);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
    demuxerPluginManager_->curAudioStreamID_ = INVALID_ID;
    demuxerPluginManager_->curSubTitleStreamID_ = ID_TEST;
    ret = demuxerPluginManager_->LoadCurrentAllPlugin(streamDemuxer, mediaInfo);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
* @tc.name  : Test UpdateTempTrackMapInfo  API
* @tc.number: UpdateTempTrackMapInfo_001
* @tc.desc  : Test newInnerTrackIndex != -1
*/
HWTEST_F(DemuxerPluginManagerUnittest, UpdateTempTrackMapInfo_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    MediaTrackMap mediaTrackMap;
    mediaTrackMap.streamID = ID_TEST;
    demuxerPluginManager_->trackInfoMap_[0] = mediaTrackMap;
    demuxerPluginManager_->temp2TrackInfoMap_[0] = mediaTrackMap;
    int32_t oldTrackId = ID_TEST;
    int32_t newTrackId = ID_TEST;
    int32_t newInnerTrackIndex = ID_TEST;
    demuxerPluginManager_->UpdateTempTrackMapInfo(oldTrackId, newTrackId, newInnerTrackIndex);
    EXPECT_EQ(demuxerPluginManager_->temp2TrackInfoMap_[0].innerTrackIndex, newInnerTrackIndex);
}

/**
* @tc.name  : Test GetStreamIDByTrackType  API
* @tc.number: GetStreamIDByTrackType_001
* @tc.desc  : Test type = TRACK_VIDEO && type == TRACK_SUBTITLE && type == INVALID_TYPE
*/
HWTEST_F(DemuxerPluginManagerUnittest, GetStreamIDByTrackType_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    TrackType type = TrackType::TRACK_VIDEO;
    auto ret = demuxerPluginManager_->GetStreamIDByTrackType(type);
    EXPECT_EQ(ret, INVALID_ID);
    type = TrackType::TRACK_SUBTITLE;
    ret = demuxerPluginManager_->GetStreamIDByTrackType(type);
    EXPECT_EQ(ret, INVALID_ID);
    type = static_cast<TrackType>(INVALID_TYPE);
    ret = demuxerPluginManager_->GetStreamIDByTrackType(type);
    EXPECT_EQ(ret, INVALID_ID);
}

/**
* @tc.name  : Test WaitForInitialBufferingEnd  API
* @tc.number: WaitForInitialBufferingEnd_001
* @tc.desc  : Test isSetSucc == false
*/
HWTEST_F(DemuxerPluginManagerUnittest, WaitForInitialBufferingEnd_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    auto mockStreamDemuxer = std::make_shared<MockBaseStreamDemuxer>();
    EXPECT_CALL(*(mockStreamDemuxer), SetSourceInitialBufferSize(_, _)).WillRepeatedly(testing::Return(false));
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer = mockStreamDemuxer;
    int32_t offset = ID_TEST;
    int32_t size = ID_TEST;
    demuxerPluginManager_->WaitForInitialBufferingEnd(streamDemuxer, offset, size);
    EXPECT_EQ(demuxerPluginManager_->isInitialBufferingSucc_.load(), true);
}

/**
* @tc.name  : Test StartPlugin  API
* @tc.number: StartPlugin_001
* @tc.desc  : Test plugin == nullptr
*             Test iter != streamInfoMap_.end()
*             Test iter == streamInfoMap_.end()
*/
HWTEST_F(DemuxerPluginManagerUnittest, StartPlugin_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    auto mockStreamDemuxer = std::make_shared<MockBaseStreamDemuxer>();
    EXPECT_CALL(*(mockStreamDemuxer), SetDemuxerState(_, _)).WillRepeatedly(testing::Return());
    EXPECT_CALL(*(mockStreamDemuxer), SnifferMediaType(_)).WillRepeatedly(testing::Return(""));
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer = mockStreamDemuxer;
    
    MediaStreamInfo mediaInfo;
    demuxerPluginManager_->streamInfoMap_[0] = mediaInfo;
    int32_t streamId = ID_TEST;
    auto ret = demuxerPluginManager_->StartPlugin(streamId, streamDemuxer);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(demuxerPluginManager_->streamInfoMap_[ID_TEST].activated, true);
    ret = demuxerPluginManager_->StartPlugin(++streamId, streamDemuxer);
    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test StopPlugin  API
* @tc.number: StopPlugin_001
* @tc.desc  : Test plugin == nullptr && iter != streamInfoMap_.end()
*/
HWTEST_F(DemuxerPluginManagerUnittest, StopPlugin_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    auto mockStreamDemuxer = std::make_shared<MockBaseStreamDemuxer>();
    EXPECT_CALL(*(mockStreamDemuxer), ResetCache(_)).WillRepeatedly(testing::Return(Status::OK));
    EXPECT_CALL(*(mockStreamDemuxer), SnifferMediaType(_)).WillRepeatedly(testing::Return(""));
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer = mockStreamDemuxer;

    MediaStreamInfo mediaInfo;
    demuxerPluginManager_->streamInfoMap_[0] = mediaInfo;
    int32_t streamId = ID_TEST;
    auto ret = demuxerPluginManager_->StopPlugin(streamId, streamDemuxer);
    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test RebootPlugin  API
* @tc.number: RebootPlugin_001
* @tc.desc  : Test plugin == nullptr
*             Test if (streamInfoMap_[streamId].pluginName.empty())
*/
HWTEST_F(DemuxerPluginManagerUnittest, RebootPlugin_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    auto mockStreamDemuxer = std::make_shared<MockBaseStreamDemuxer>();
    EXPECT_CALL(*(mockStreamDemuxer), ResetCache(_)).WillRepeatedly(testing::Return(Status::OK));
    EXPECT_CALL(*(mockStreamDemuxer), SetDemuxerState(_, _)).WillRepeatedly(testing::Return());
    EXPECT_CALL(*(mockStreamDemuxer), SnifferMediaType(_)).WillRepeatedly(testing::Return(""));
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer = mockStreamDemuxer;

    MediaStreamInfo mediaInfo;
    demuxerPluginManager_->streamInfoMap_[0] = mediaInfo;
    int32_t streamId = ID_TEST;
    TrackType trackType = TrackType::TRACK_VIDEO;
    bool isRebooted = false;
    auto ret = demuxerPluginManager_->RebootPlugin(streamId, trackType, streamDemuxer, isRebooted);
    EXPECT_EQ(demuxerPluginManager_->streamInfoMap_[ID_TEST].activated, true);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
* @tc.name  : Test GetStreamDemuxerNewStreamID  API
* @tc.number: GetStreamDemuxerNewStreamID_001
* @tc.desc  : Test all
*/
HWTEST_F(DemuxerPluginManagerUnittest, GetStreamDemuxerNewStreamID_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    auto mockStreamDemuxer = std::make_shared<MockBaseStreamDemuxer>();
    EXPECT_CALL(*(mockStreamDemuxer), GetNewAudioStreamID()).WillRepeatedly(testing::Return(ID_TEST));
    EXPECT_CALL(*(mockStreamDemuxer), GetNewSubtitleStreamID()).WillRepeatedly(testing::Return(ID_TEST));
    EXPECT_CALL(*(mockStreamDemuxer), GetNewVideoStreamID()).WillRepeatedly(testing::Return(ID_TEST));
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer = mockStreamDemuxer;

    TrackType trackType = TrackType::TRACK_VIDEO;
    auto ret = demuxerPluginManager_->GetStreamDemuxerNewStreamID(trackType, streamDemuxer);
    EXPECT_EQ(ret, ID_TEST);
    trackType = TrackType::TRACK_SUBTITLE;
    ret = demuxerPluginManager_->GetStreamDemuxerNewStreamID(trackType, streamDemuxer);
    EXPECT_EQ(ret, ID_TEST);
    trackType = TrackType::TRACK_AUDIO;
    ret = demuxerPluginManager_->GetStreamDemuxerNewStreamID(trackType, streamDemuxer);
    EXPECT_EQ(ret, ID_TEST);
    trackType = static_cast<TrackType>(INVALID_TYPE);
    ret = demuxerPluginManager_->GetStreamDemuxerNewStreamID(trackType, streamDemuxer);
    EXPECT_EQ(ret, INVALID_ID);
}

/**
* @tc.name  : Test SingleStreamSeekTo  API
* @tc.number: SingleStreamSeekTo_001
* @tc.desc  : Test plugin == nullptr
*/
HWTEST_F(DemuxerPluginManagerUnittest, SingleStreamSeekTo_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    int64_t seekTime = ID_TEST;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;
    int32_t streamID = ID_TEST;
    int64_t realSeekTime = ID_TEST;
    auto ret = demuxerPluginManager_->SingleStreamSeekTo(seekTime, mode, streamID, realSeekTime);
    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test SeekToFrameByDts  API
* @tc.number: SeekToFrameByDts_001
* @tc.desc  : Test streamID < 0, guard fails at first condition
*/
HWTEST_F(DemuxerPluginManagerUnittest, SeekToFrameByDts_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    int32_t invalidStreamId = INVALID_ID;
    int32_t trackId = ID_TEST;
    int64_t seekTime = TEST_SEEK_TIME;
    int64_t realSeekTime = 0;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;

    auto ret = demuxerPluginManager_->SeekToFrameByDts(invalidStreamId, trackId, seekTime, mode, realSeekTime);

    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test SeekToFrameByDts  API
* @tc.number: SeekToFrameByDts_002
* @tc.desc  : Test streamID valid but not found in streamInfoMap_
*/
HWTEST_F(DemuxerPluginManagerUnittest, SeekToFrameByDts_002, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    int32_t streamId = ID_TEST;
    int32_t trackId = ID_TEST;
    int64_t seekTime = TEST_SEEK_TIME;
    int64_t realSeekTime = 0;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;

    auto ret = demuxerPluginManager_->SeekToFrameByDts(streamId, trackId, seekTime, mode, realSeekTime);

    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test SeekToFrameByDts  API
* @tc.number: SeekToFrameByDts_003
* @tc.desc  : Test streamID found but plugin is nullptr
*/
HWTEST_F(DemuxerPluginManagerUnittest, SeekToFrameByDts_003, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    int32_t streamId = ID_TEST;
    MediaStreamInfo mediaInfo;
    mediaInfo.plugin = nullptr;
    demuxerPluginManager_->streamInfoMap_[streamId] = mediaInfo;

    int32_t trackId = ID_TEST;
    int64_t seekTime = TEST_SEEK_TIME;
    int64_t realSeekTime = 0;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;

    auto ret = demuxerPluginManager_->SeekToFrameByDts(streamId, trackId, seekTime, mode, realSeekTime);

    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test SeekToFrameByDts  API
* @tc.number: SeekToFrameByDts_004
* @tc.desc  : Test plugin exists and SeekToFrameByDts returns OK
*/
HWTEST_F(DemuxerPluginManagerUnittest, SeekToFrameByDts_004, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    int32_t streamId = ID_TEST;
    MediaStreamInfo mediaInfo;
    auto plugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK, Status::OK);
    mediaInfo.plugin = plugin;
    demuxerPluginManager_->streamInfoMap_[streamId] = mediaInfo;

    int32_t trackId = ID_TEST;
    int64_t seekTime = TEST_SEEK_TIME;
    int64_t realSeekTime = 0;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;

    auto ret = demuxerPluginManager_->SeekToFrameByDts(streamId, trackId, seekTime, mode, realSeekTime);

    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(plugin->GetSeekToFrameByDtsCallCount(), 1);
}

/**
* @tc.name  : Test SeekToFrameByDts  API
* @tc.number: SeekToFrameByDts_005
* @tc.desc  : Test plugin exists and SeekToFrameByDts returns error
*/
HWTEST_F(DemuxerPluginManagerUnittest, SeekToFrameByDts_005, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    int32_t streamId = ID_TEST;
    MediaStreamInfo mediaInfo;
    auto plugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK, Status::ERROR_UNKNOWN);
    mediaInfo.plugin = plugin;
    demuxerPluginManager_->streamInfoMap_[streamId] = mediaInfo;

    int32_t trackId = ID_TEST;
    int64_t seekTime = TEST_SEEK_TIME;
    int64_t realSeekTime = 0;
    Plugins::SeekMode mode = Plugins::SeekMode::SEEK_NEXT_SYNC;

    auto ret = demuxerPluginManager_->SeekToFrameByDts(streamId, trackId, seekTime, mode, realSeekTime);

    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_EQ(plugin->GetSeekToFrameByDtsCallCount(), 1);
}

/**
* @tc.name  : Test Pause  API
* @tc.number: Pause_001
* @tc.desc  : Test all current stream IDs invalid, no plugin called
*/
HWTEST_F(DemuxerPluginManagerUnittest, Pause_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    demuxerPluginManager_->curVideoStreamID_ = INVALID_ID;
    demuxerPluginManager_->curAudioStreamID_ = INVALID_ID;
    demuxerPluginManager_->curSubTitleStreamID_ = INVALID_ID;

    auto ret = demuxerPluginManager_->Pause();

    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test Pause  API
* @tc.number: Pause_002
* @tc.desc  : Test only video plugin exists and Pause returns OK
*/
HWTEST_F(DemuxerPluginManagerUnittest, Pause_002, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    demuxerPluginManager_->curVideoStreamID_ = VIDEO_STREAM_ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = INVALID_ID;
    demuxerPluginManager_->curSubTitleStreamID_ = INVALID_ID;

    MediaStreamInfo videoInfo;
    auto videoPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    videoInfo.plugin = videoPlugin;
    demuxerPluginManager_->streamInfoMap_[VIDEO_STREAM_ID_TEST] = videoInfo;

    auto ret = demuxerPluginManager_->Pause();

    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoPlugin->GetPauseCallCount(), 1);
}

/**
* @tc.name  : Test Pause  API
* @tc.number: Pause_003
* @tc.desc  : Test video Pause returns error causes early return
*/
HWTEST_F(DemuxerPluginManagerUnittest, Pause_003, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    demuxerPluginManager_->curVideoStreamID_ = VIDEO_STREAM_ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = ID_TEST;
    demuxerPluginManager_->curSubTitleStreamID_ = SUBTITLE_STREAM_ID_TEST;

    MediaStreamInfo videoInfo;
    auto videoPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK, Status::OK,
        Status::ERROR_UNKNOWN);
    videoInfo.plugin = videoPlugin;
    demuxerPluginManager_->streamInfoMap_[VIDEO_STREAM_ID_TEST] = videoInfo;

    MediaStreamInfo audioInfo;
    auto audioPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    audioInfo.plugin = audioPlugin;
    demuxerPluginManager_->streamInfoMap_[ID_TEST] = audioInfo;

    MediaStreamInfo subtitleInfo;
    auto subtitlePlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    subtitleInfo.plugin = subtitlePlugin;
    demuxerPluginManager_->streamInfoMap_[SUBTITLE_STREAM_ID_TEST] = subtitleInfo;

    auto ret = demuxerPluginManager_->Pause();

    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_EQ(videoPlugin->GetPauseCallCount(), 1);
    EXPECT_EQ(audioPlugin->GetPauseCallCount(), 0);
    EXPECT_EQ(subtitlePlugin->GetPauseCallCount(), 0);
}

/**
* @tc.name  : Test Pause  API
* @tc.number: Pause_004
* @tc.desc  : Test video OK, audio OK, subtitle OK
*/
HWTEST_F(DemuxerPluginManagerUnittest, Pause_004, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    demuxerPluginManager_->curVideoStreamID_ = VIDEO_STREAM_ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = ID_TEST;
    demuxerPluginManager_->curSubTitleStreamID_ = SUBTITLE_STREAM_ID_TEST;

    MediaStreamInfo videoInfo;
    auto videoPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    videoInfo.plugin = videoPlugin;
    demuxerPluginManager_->streamInfoMap_[VIDEO_STREAM_ID_TEST] = videoInfo;

    MediaStreamInfo audioInfo;
    auto audioPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    audioInfo.plugin = audioPlugin;
    demuxerPluginManager_->streamInfoMap_[ID_TEST] = audioInfo;

    MediaStreamInfo subtitleInfo;
    auto subtitlePlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    subtitleInfo.plugin = subtitlePlugin;
    demuxerPluginManager_->streamInfoMap_[SUBTITLE_STREAM_ID_TEST] = subtitleInfo;

    auto ret = demuxerPluginManager_->Pause();

    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoPlugin->GetPauseCallCount(), 1);
    EXPECT_EQ(audioPlugin->GetPauseCallCount(), 1);
    EXPECT_EQ(subtitlePlugin->GetPauseCallCount(), 1);
}

/**
* @tc.name  : Test Pause  API
* @tc.number: Pause_005
* @tc.desc  : Test video OK, audio Pause returns error
*/
HWTEST_F(DemuxerPluginManagerUnittest, Pause_005, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    demuxerPluginManager_->curVideoStreamID_ = VIDEO_STREAM_ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = ID_TEST;
    demuxerPluginManager_->curSubTitleStreamID_ = SUBTITLE_STREAM_ID_TEST;

    MediaStreamInfo videoInfo;
    auto videoPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    videoInfo.plugin = videoPlugin;
    demuxerPluginManager_->streamInfoMap_[VIDEO_STREAM_ID_TEST] = videoInfo;

    MediaStreamInfo audioInfo;
    auto audioPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK, Status::OK,
        Status::ERROR_UNKNOWN);
    audioInfo.plugin = audioPlugin;
    demuxerPluginManager_->streamInfoMap_[ID_TEST] = audioInfo;

    MediaStreamInfo subtitleInfo;
    auto subtitlePlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    subtitleInfo.plugin = subtitlePlugin;
    demuxerPluginManager_->streamInfoMap_[SUBTITLE_STREAM_ID_TEST] = subtitleInfo;

    auto ret = demuxerPluginManager_->Pause();

    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_EQ(videoPlugin->GetPauseCallCount(), 1);
    EXPECT_EQ(audioPlugin->GetPauseCallCount(), 1);
    EXPECT_EQ(subtitlePlugin->GetPauseCallCount(), 0);
}

/**
* @tc.name  : Test Pause  API
* @tc.number: Pause_006
* @tc.desc  : Test video OK, audio OK, subtitle Pause returns error
*/
HWTEST_F(DemuxerPluginManagerUnittest, Pause_006, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    demuxerPluginManager_->curVideoStreamID_ = VIDEO_STREAM_ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = ID_TEST;
    demuxerPluginManager_->curSubTitleStreamID_ = SUBTITLE_STREAM_ID_TEST;

    MediaStreamInfo videoInfo;
    auto videoPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    videoInfo.plugin = videoPlugin;
    demuxerPluginManager_->streamInfoMap_[VIDEO_STREAM_ID_TEST] = videoInfo;

    MediaStreamInfo audioInfo;
    auto audioPlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK);
    audioInfo.plugin = audioPlugin;
    demuxerPluginManager_->streamInfoMap_[ID_TEST] = audioInfo;

    MediaStreamInfo subtitleInfo;
    auto subtitlePlugin = std::make_shared<Plugins::TestDemuxerPlugin>(Status::OK, Status::OK, Status::OK,
        Status::ERROR_UNKNOWN);
    subtitleInfo.plugin = subtitlePlugin;
    demuxerPluginManager_->streamInfoMap_[SUBTITLE_STREAM_ID_TEST] = subtitleInfo;

    auto ret = demuxerPluginManager_->Pause();

    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_EQ(videoPlugin->GetPauseCallCount(), 1);
    EXPECT_EQ(audioPlugin->GetPauseCallCount(), 1);
    EXPECT_EQ(subtitlePlugin->GetPauseCallCount(), 1);
}

/**
* @tc.name  : Test Reset  API
* @tc.number: Reset_001
* @tc.desc  : Test return Status::OK
*/
HWTEST_F(DemuxerPluginManagerUnittest, Reset_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    demuxerPluginManager_->curVideoStreamID_ = ID_TEST;
    demuxerPluginManager_->curAudioStreamID_ = ID_TEST;
    demuxerPluginManager_->curSubTitleStreamID_ = ID_TEST;
    auto ret = demuxerPluginManager_->Reset();
    EXPECT_EQ(ret, Status::OK);
}

/**
* @tc.name  : Test GetCurrentBitRate  API
* @tc.number: GetCurrentBitRate_001
* @tc.desc  : Test return Status::OK
*/
HWTEST_F(DemuxerPluginManagerUnittest, GetCurrentBitRate_001, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);
    demuxerPluginManager_->curVideoStreamID_ = ID_TEST;
    MediaStreamInfo mediaInfo;
    mediaInfo.bitRate = INVALID_TYPE;
    demuxerPluginManager_->streamInfoMap_[0] = mediaInfo;
    demuxerPluginManager_->isDash_ = true;
    auto ret = demuxerPluginManager_->GetCurrentBitRate();
    EXPECT_EQ(ret, INVALID_TYPE);
}

/**
* @tc.name  : Test InitVideoTrack  API
* @tc.number: InitVideoTrack_002
* @tc.desc  : curVideoStreamID_ != -1, apiVersion_ >= API_VERSION_16/18
*             -> set VIDEO_IS_HDR_VIVID and MIME "video/unknown"
*/
HWTEST_F(DemuxerPluginManagerUnittest, InitVideoTrack_002, TestSize.Level1)
{
    ASSERT_NE(demuxerPluginManager_, nullptr);

    StreamInfo info;
    info.streamId = VIDEO_STREAM_ID_TEST;
    info.bitRate = TEST_BIT_RATE;
    info.videoWidth = TEST_VIDEO_WIDTH;
    info.videoHeight = TEST_VIDEO_HEIGHT;
    info.videoType = Plugins::VideoType::VIDEO_TYPE_HDR_VIVID;

    demuxerPluginManager_->curVideoStreamID_ = ID_TEST; // simulate first video stream already set
    demuxerPluginManager_->apiVersion_ = API_VERSION_18;

    MediaStreamInfo mediaInfo;
    demuxerPluginManager_->streamInfoMap_[info.streamId] = mediaInfo;

    demuxerPluginManager_->InitVideoTrack(info);

    ASSERT_FALSE(demuxerPluginManager_->streamInfoMap_[info.streamId].mediaInfo.tracks.empty());
    auto &trackMeta = demuxerPluginManager_->streamInfoMap_[info.streamId].mediaInfo.tracks[0];

    std::string mime;
    EXPECT_TRUE(trackMeta.Get<Tag::MIME_TYPE>(mime));
    EXPECT_EQ(mime, "video/unknown");

    bool isHdrVivid = false;
    EXPECT_TRUE(trackMeta.Get<Tag::VIDEO_IS_HDR_VIVID>(isHdrVivid));
    EXPECT_TRUE(isHdrVivid);
}
} // namespace Media
} // namespace OHOS
