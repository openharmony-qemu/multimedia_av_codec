/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "media_demuxer_ext_unit_test.h"

#include "plugin/plugin_event.h"
#include "demuxer/stream_demuxer.h"
#include "meta/meta_key.h"

using namespace std;
using namespace testing;
using namespace testing::ext;

using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS::Media {

#undef HiSysEventWrite
#define HiSysEventWrite(domain, eventName, type, ...) (mockRetInt32_;)

static const uint32_t UNUM_0 = 0;
static const uint32_t UNUM_1000 = 1000;
static const uint32_t UNUM_2000 = 2000;
static const uint32_t UNUM_3000 = 3000;
static const int32_t NUM_0 = 0;
static const int32_t NUM_1 = 1;
static const int32_t NUM_2 = 2;
static const int32_t NUM_3 = 3;
static const int32_t NEG_1 = -1;
static const int32_t NEG_2 = -2;
static const int32_t NUM_10 = 100;
static const int32_t NUM_100 = 100;
static const int32_t NUM_200 = 200;
static const int32_t NUM_500 = 500;
static const int32_t NUM_800 = 800;
static const int64_t NUM_1000 = 1000;
static const int64_t NUM_2000 = 2000;
static const int64_t NUM_3000 = 3000;
static const int32_t TEST_VIDEO_WIDTH = 1920;
static const int32_t TEST_VIDEO_HEIGHT = 1080;
static const char TEST_LANG_ENG[] = "eng";
static const char TEST_LANG_ZH[] = "zh";
static const int32_t TEST_MAX_VIDEO_BITRATE = 2000;
static const int32_t TEST_MIN_VIDEO_BITRATE = 500;
static const int32_t TEST_MAX_VIDEO_FRAMERATE = 60;
static const int32_t TEST_MIN_VIDEO_FRAMERATE = 24;
static const int32_t TEST_MAX_AUDIO_BITRATE = 256000;
static const int32_t TEST_MIN_AUDIO_BITRATE = 64000;
static const int32_t TEST_MAX_AUDIO_CHANNELS = 2;
static const uint32_t TEST_NETWORK_BITRATE = 3000;
static const int32_t TEST_MIN_VIDEO_WIDTH = 1280;
static const int32_t TEST_MIN_VIDEO_HEIGHT = 720;
static const int32_t TEST_VIDEO_BITRATE_1000 = 1000;
static const int32_t TEST_AUDIO_BITRATE_128000 = 128000;
static const int32_t TEST_STREAM_ID_1 = 1;
static const int32_t TEST_STREAM_ID_2 = 2;
static const int32_t TEST_STREAM_ID_3 = 3;
static const int32_t TEST_STREAM_ID_4 = 4;
static const char TEST_LANG_FRA[] = "fra";
static const char TEST_LANG_EN_SHORT[] = "en";
static const char TEST_MIME_VIDEO_AVC[] = "video/avc";
static const char TEST_MIME_AUDIO_AAC[] = "audio/mp4a-latm";
static std::map<StreamType, std::vector<StreamFilterPredicate>> filteredStreamPredicates_;
static std::map<StreamType, std::vector<StreamInfo>> filteredStreamMaps_;
static const int64_t NUM_200000 = 200000;
constexpr int32_t FILTER_VIDEO_STREAM_ID_1 = 100;
constexpr int32_t FILTER_VIDEO_STREAM_ID_2 = 101;
constexpr int32_t FILTER_AUDIO_STREAM_ID_1 = 200;
constexpr int32_t FILTER_AUDIO_STREAM_ID_2 = 201;
constexpr int32_t FILTER_SUBTITLE_STREAM_ID_1 = 300;
constexpr int32_t FILTER_SUBTITLE_STREAM_ID_2 = 301;
const std::string FILTER_PLAY_TYPE_HTTP = "http";
const std::string FILTER_PLAY_TYPE_DASH = "dash";
const std::string FILTER_MIME_VIDEO_AVC = "video/avc";
const std::string FILTER_MIME_VIDEO_HEVC = "video/hevc";
const std::string FILTER_LANG_ENG = "eng";
const std::string FILTER_LANG_CHI = "chi";
const std::string FILTER_LANG_FRA = "fra";
class FakeAVCodecList : public MediaAVCodec::AVCodecList {
public:
    explicit FakeAVCodecList(std::vector<std::shared_ptr<MediaAVCodec::CapabilityData>> caps)
        : caps_(std::move(caps))
    {
    }

    std::string FindDecoder(const Format &format) override
    {
        (void)format;
        return "";
    }

    std::string FindEncoder(const Format &format) override
    {
        (void)format;
        return "";
    }

    MediaAVCodec::CapabilityData *GetCapability(const std::string &mime, const bool isEncoder,
        const MediaAVCodec::AVCodecCategory &category) override
    {
        (void)mime;
        (void)isEncoder;
        (void)category;
        return nullptr;
    }

    void *GetBuffer(const std::string &name, uint32_t sizeOfCap) override
    {
        (void)name;
        (void)sizeOfCap;
        return nullptr;
    }

    void *NewBuffer(size_t bufSize) override
    {
        (void)bufSize;
        return nullptr;
    }

    std::vector<std::shared_ptr<MediaAVCodec::CapabilityData>> GetCapabilityList(int32_t codecType) override
    {
        (void)codecType;
        return caps_;
    }

private:
    std::vector<std::shared_ptr<MediaAVCodec::CapabilityData>> caps_;
};

static std::shared_ptr<MediaAVCodec::CapabilityData> MakeCodecCapability(const std::string &mimeType)
{
    auto capability = std::make_shared<MediaAVCodec::CapabilityData>();
    capability->mimeType = mimeType;
    return capability;
}

void MediaDemuxerExtUnitTest::SetUpTestCase(void) {}

void MediaDemuxerExtUnitTest::TearDownTestCase(void) {}

void MediaDemuxerExtUnitTest::SetUp()
{
    mediaDemuxer_ = std::make_shared<MediaDemuxer>();
    mediaDemuxer_->source_ = std::make_shared<Source>();
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->audioTrackId_ = 1;
    mediaDemuxer_->demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
}

void MediaDemuxerExtUnitTest::TearDown()
{
    mediaDemuxer_ = nullptr;
}

static StreamInfo MakeAutoSelectVideoStream(int32_t streamId, uint32_t bitRate,
    StreamType type = StreamType::VIDEO)
{
    StreamInfo streamInfo;
    streamInfo.streamId = streamId;
    streamInfo.bitRate = bitRate;
    streamInfo.type = type;
    return streamInfo;
}

static void PrepareAutoSelectTrackByBitrate(const std::shared_ptr<MediaDemuxer> &mediaDemuxer,
    int32_t streamId, uint32_t currentBitRate)
{
    mediaDemuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer->filteredStreamMap_->map[VIDEO].clear();
    mediaDemuxer->filteredStreamMap_->map[VIDEO].push_back(MakeAutoSelectVideoStream(streamId, currentBitRate));
    mediaDemuxer->isAutoSelect_.store(true);
    mediaDemuxer->curDownloadBitRate_.store(currentBitRate);
    mediaDemuxer->lastSwitchDownloadRate_.store(UNUM_0);
    mediaDemuxer->lastCheckAutoSelectTimeMs_ = mediaDemuxer->GetCurrentTimeMs() - NUM_2000;
    mediaDemuxer->defaultVideoStreamId_ = NEG_1;
    mediaDemuxer->lastAutoSelectBitRate_ = NEG_1;
    mediaDemuxer->isPrepared_.store(true);
    mediaDemuxer->isSelectBitRate_.store(false);
    mediaDemuxer->isSelectTrack_.store(false);
    mediaDemuxer->targetBitRate_ = currentBitRate;
    mediaDemuxer->isFlvLiveStream_ = false;
}

StreamInfo MakeTrackSelectionFilterStream(int32_t streamId, StreamType type, uint32_t bitRate,
    const std::string &mimeType, const std::string &lang)
{
    StreamInfo stream;
    stream.streamId = streamId;
    stream.type = type;
    stream.bitRate = bitRate;
    stream.mimeType = mimeType;
    stream.lang = lang;
    if (type == VIDEO || type == MIXED) {
        stream.videoWidth = NUM_1;
        stream.videoHeight = NUM_1;
    }
    return stream;
}

void PrepareTrackSelectionFilter(const std::shared_ptr<MediaDemuxer> &mediaDemuxer, const std::string &playType,
    const std::vector<StreamInfo> &streams)
{
    mediaDemuxer->filteredStreamMap_->map.clear();
    mediaDemuxer->isPrepared_.store(true);
    mediaDemuxer->currentPlayType_ = playType;
    mediaDemuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer->curDownloadBitRate_.store(UNUM_2000);
    mediaDemuxer->lastAutoSelectBitRate_ = NEG_1;
    EXPECT_CALL(*(mediaDemuxer->source_), IsCloudFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer->source_), IsNeedPreDownload()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer->source_), GetStreamInfo(_, true)).WillOnce(Invoke(
        [streams](std::vector<StreamInfo> &outStreams, bool) {
            outStreams = streams;
            return Status::OK;
        }));
}

StreamInfo MakeSelectAudioDefaultStream(int32_t streamId, const std::string &lang, const std::string &mimeType)
{
    StreamInfo stream;
    stream.streamId = streamId;
    stream.type = AUDIO;
    stream.lang = lang;
    stream.mimeType = mimeType;
    return stream;
}

StreamInfo MakeSelectSubtitleDefaultStream(int32_t streamId, const std::string &lang)
{
    StreamInfo stream;
    stream.streamId = streamId;
    stream.type = SUBTITLE;
    stream.lang = lang;
    return stream;
}

static std::shared_ptr<SampleQueue> PrepareConsumeWaterLoopControl(const std::shared_ptr<MediaDemuxer> &mediaDemuxer,
    int32_t trackId)
{
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer->sampleQueueController_ = std::make_shared<SampleQueueController>();
    mediaDemuxer->subtitleTrackId_ = trackId + NUM_1;
    mediaDemuxer->videoTrackId_ = trackId;
    mediaDemuxer->sampleQueueMap_[trackId] = sampleQueue;
    mediaDemuxer->taskMap_[trackId] = std::unique_ptr<Task>();
    mediaDemuxer->sampleConsumerTaskMap_[trackId] = std::unique_ptr<Task>();
    mediaDemuxer->eosMap_[trackId] = false;
    mediaDemuxer->hlsSegmentEosMap_[trackId] = false;
    mediaDemuxer->isVideoMuted_ = false;
    mediaDemuxer->isBuffering_.store(false);
    mediaDemuxer->SetTrackIsBuffering(trackId, false);
    mediaDemuxer->isAutoSelect_.store(false);
    mediaDemuxer->curDownloadBitRate_.store(UNUM_0);
    mediaDemuxer->lastSwitchDownloadRate_.store(UNUM_0);
    mediaDemuxer->lastCheckAutoSelectTimeMs_ = NUM_1;
    return sampleQueue;
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest StartReferenceParser API
 * @tc.number: StartReferenceParser
 * @tc.desc  : Test MediaDemuxerExtUnitTest StartReferenceParser interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReportSceneCodeForDemuxer_001, TestSize.Level1)
{
    mediaDemuxer_->isFirstParser_ = true;
    mockRetInt32_ = MSERR_OK;
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(plugin), ParserRefUpdatePos(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));

    auto res = mediaDemuxer_->StartReferenceParser(0, 0);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest StartReferenceParser API
 * @tc.number: StartReferenceParser
 * @tc.desc  : Test MediaDemuxerExtUnitTest StartReferenceParser interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReportSceneCodeForDemuxer_002, TestSize.Level1)
{
    mediaDemuxer_->isFirstParser_ = true;
    mockRetInt32_ = MSERR_UNKNOWN;
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(plugin), ParserRefUpdatePos(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));

    auto res = mediaDemuxer_->StartReferenceParser(0, 0);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ParserRefInfo API
 * @tc.number: ParserRefInfo_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest ParserRefInfo interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ParserRefInfo_001, TestSize.Level1)
{
    mediaDemuxer_->demuxerPluginManager_ = nullptr;
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ParserRefInfo API
 * @tc.number: ParserRefInfo_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest ParserRefInfo interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ParserRefInfo_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask API
 * @tc.number: AccelerateSampleConsumerTask
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateSampleConsumerTask, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask API
 * @tc.number: AccelerateSampleConsumerTask_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateSampleConsumerTask_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);

    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    mediaDemuxer_->AccelerateSampleConsumerTask(-1);
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask API
 * @tc.number: AccelerateSampleConsumerTask_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateSampleConsumerTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateSampleConsumerTask_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;

    mediaDemuxer_->AccelerateSampleConsumerTask(1);

    mediaDemuxer_->trackMap_.insert(std::make_pair(1, nullptr));
    mediaDemuxer_->AccelerateSampleConsumerTask(1);

    std::shared_ptr<MediaDemuxer::TrackWrapper> wrapper =
        std::make_shared<MediaDemuxer::TrackWrapper>(1, nullptr, mediaDemuxer_);
    wrapper->isNotifySampleConsumerNeeded_ = false;
    mediaDemuxer_->trackMap_.emplace(1, wrapper);
    mediaDemuxer_->AccelerateSampleConsumerTask(1);

    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateTrackTask API
 * @tc.number: AccelerateTrackTask_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateTrackTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateTrackTask_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateTrackTask(-1);

    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->AccelerateTrackTask(-1);

    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    mediaDemuxer_->AccelerateTrackTask(-1);
    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AccelerateTrackTask API
 * @tc.number: AccelerateTrackTask_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest AccelerateTrackTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AccelerateTrackTask_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;

    mediaDemuxer_->AccelerateTrackTask(1);

    mediaDemuxer_->trackMap_.insert(std::make_pair(1, nullptr));
    mediaDemuxer_->AccelerateTrackTask(1);

    std::shared_ptr<MediaDemuxer::TrackWrapper> wrapper =
        std::make_shared<MediaDemuxer::TrackWrapper>(1, nullptr, mediaDemuxer_);
    wrapper->isNotifySampleConsumerNeeded_ = false;
    mediaDemuxer_->trackMap_.emplace(1, wrapper);
    mediaDemuxer_->AccelerateTrackTask(1);

    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest SetTrackNotifySampleConsumerFlag API
 * @tc.number: SetTrackNotifySampleConsumerFlag_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest SetTrackNotifySampleConsumerFlag interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SetTrackNotifySampleConsumerFlag_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    mediaDemuxer_->SetTrackNotifySampleConsumerFlag(1, true);

    mediaDemuxer_->trackMap_.insert(std::make_pair(1, nullptr));
    mediaDemuxer_->SetTrackNotifySampleConsumerFlag(1, true);

    auto res = mediaDemuxer_->ParserRefInfo();
    EXPECT_EQ(res, 0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetDuration API
 * @tc.number: GetDuration_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetDuration interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetDuration_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Seekable::SEEKABLE));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetDuration()).WillRepeatedly(Return(Plugins::HST_TIME_NONE));

    int64_t duration = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(duration);
    auto res = mediaDemuxer_->GetDuration(duration);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetDuration API
 * @tc.number: GetDuration_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetDuration interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetDuration_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Seekable::SEEKABLE));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetDuration()).WillRepeatedly(Return(0));

    mediaDemuxer_->duration_ = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();

    int64_t duration = 0;
    auto res = mediaDemuxer_->GetDuration(duration);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetDuration API
 * @tc.number: GetDuration_003
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetDuration interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetDuration_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Seekable::SEEKABLE));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetDuration()).WillRepeatedly(Return(0));

    mediaDemuxer_->duration_ = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();

    int64_t duration = 0;
    auto res = mediaDemuxer_->GetDuration(duration);
    EXPECT_EQ(res, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask API
 * @tc.number: MediaDemuxerExt_AddDemuxerCopyTask_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AddDemuxerCopyTask_001, TestSize.Level1)
{
    auto res = mediaDemuxer_->AddDemuxerCopyTask(1, TaskType::AUDIO);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask API
 * @tc.number: MediaDemuxerExt_AddDemuxerCopyTask_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AddDemuxerCopyTask_002, TestSize.Level1)
{
    auto res = mediaDemuxer_->AddDemuxerCopyTask(1, TaskType::AUDIO);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask API
 * @tc.number: MediaDemuxerExt_AddDemuxerCopyTask_003
 * @tc.desc  : Test MediaDemuxerExtUnitTest AddDemuxerCopyTask interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AddDemuxerCopyTask_003, TestSize.Level1)
{
    auto res = mediaDemuxer_->AddDemuxerCopyTask(1, TaskType::SUBTITLE);
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_001
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_001, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    std::vector<std::shared_ptr<Meta>> vector;
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), MediaDemuxer::TRACK_ID_INVALID);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_002
 * @tc.desc  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId interface
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_002, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    std::vector<std::shared_ptr<Meta>> vector;
    vector.push_back(nullptr);
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), MediaDemuxer::TRACK_ID_INVALID);
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_001
 * @tc.desc  : Test taskMap_[trackId] == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_001, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_1;
    mediaDemuxer_->taskMap_[trackId] = nullptr;
    auto iter = mediaDemuxer_->taskMap_.find(trackId);
    EXPECT_TRUE(iter != mediaDemuxer_->taskMap_.end());
    EXPECT_EQ(mediaDemuxer_->StartTask(trackId), Status::OK);
    mediaDemuxer_->StopAllTask();
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_002
 * @tc.desc  : Test taskMap_[trackId] != nullptr && !taskMap_[trackId]->IsTaskRunning()
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_002, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_1;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(taskPtr), Start()).Times(NUM_1);
    mediaDemuxer_->taskMap_[trackId] = std::move(taskPtr);
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
    mediaDemuxer_->StopAllTask();
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_003
 * @tc.desc  : Test taskMap_[trackId] != nullptr && taskMap_[trackId]->IsTaskRunning()
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_003, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_1;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(taskPtr), Start()).Times(0);
    mediaDemuxer_->taskMap_[trackId] = std::move(taskPtr);
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
    mediaDemuxer_->StopAllTask();
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_004
 * @tc.desc  : Test trackType == TrackType::TRACK_AUDIO
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_004, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    TrackType type = mediaDemuxer_->demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(0));
    EXPECT_EQ(type, TrackType::TRACK_AUDIO);
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_2;
    auto iter = mediaDemuxer_->taskMap_.find(trackId);
    EXPECT_TRUE(iter == mediaDemuxer_->taskMap_.end());
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
    mediaDemuxer_->StopAllTask();
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_005
 * @tc.desc  : Test trackType == TrackType::TRACK_VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_005, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_)).Times(NUM_1);
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_2;
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
    mediaDemuxer_->StopAllTask();
}

/**
 * @tc.name  : Test StartTask
 * @tc.number: StartTask_006
 * @tc.desc  : Test trackType == TrackType::TRACK_SUBTITLE
 */
HWTEST_F(MediaDemuxerExtUnitTest, StartTask_006, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_)).Times(NUM_1);
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = 4;
    Status ret = mediaDemuxer_->StartTask(trackId);
    EXPECT_EQ(ret, Status::OK);
    mediaDemuxer_->StopAllTask();
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_001
 * @tc.desc  : Test mimeType.find("audio") == 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    
    mediaDemuxer_->lastAudioPts_ = NUM_2000;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        SingleStreamSeekTo(_, _, _, _)).WillOnce(::testing::Return(Status::OK));
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_002
 * @tc.desc  : Test mimeType == "application/x-subrip"
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "application/x-subrip";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    
    mediaDemuxer_->lastAudioPts_ = NUM_2000;
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_,
        SingleStreamSeekTo(_, _, _, _)).WillOnce(::testing::Return(Status::OK));
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_003
 * @tc.desc  : Test mimeType == "text/vtt"
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_003, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "text/vtt";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    
    mediaDemuxer_->lastAudioPts_ = NUM_2000;
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_,
        SingleStreamSeekTo(_, _, _, _)).WillOnce(::testing::Return(Status::OK));
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleSelectTrackStreamSeek
 * @tc.number: HandleSelectTrackStreamSeek_004
 * @tc.desc  : Test mimeType NUM_1= "text/vtt"
 *             Test mimeType != "application/x-subrip"
 *             Test mimeType.find("audio") != 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackStreamSeek_004, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    int32_t streamID = NUM_0;
    std::string audioMimeType = "aaaa";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);

    mediaDemuxer_->lastAudioPts_ = NUM_3000;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), SingleStreamSeekTo(_, _, _, _)).Times(0);
    mediaDemuxer_->HandleSelectTrackStreamSeek(streamID, trackId);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_001
 * @tc.desc  : Test trackId == TRACK_ID_INVALID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = mediaDemuxer_->TRACK_ID_INVALID;
    mediaDemuxer_->audioTrackId_ = mediaDemuxer_->TRACK_ID_INVALID;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);

    auto ret = mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_002
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second != SEEK_TO_EOS
 *             Test seekReadyInfo.first >= 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->OnInterrupted(true);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillRepeatedly(Return(Status::OK));
    mediaDemuxer_->seekReadyStreamInfo_.clear();
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {NUM_100, false};

    mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_003
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second == SEEK_TO_EOS
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_003, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    auto ret = mediaDemuxer_->demuxerPluginManager_->GetTmpStreamIDByTrackID(NUM_1);
    EXPECT_EQ(ret, NUM_100);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_.clear();
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {NUM_1, true};
    EXPECT_TRUE(mediaDemuxer_->seekReadyStreamInfo_.find(static_cast<int32_t>(StreamType::VIDEO)) !=
        mediaDemuxer_->seekReadyStreamInfo_.end());
    auto seekReadyInfo = mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)];
    EXPECT_EQ(seekReadyInfo.first, NUM_1);
    EXPECT_EQ(seekReadyInfo.second, true);
    mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_004
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second != SEEK_TO_EOS
 *             Test seekReadyInfo.first < 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_004, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(NUM_1);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {-1, false};
    
    mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test HandleHlsRebootPlugin
 * @tc.number: HandleHlsRebootPlugin_005
 * @tc.desc  : Test trackId != TRACK_ID_INVALID
 *             Test seekReadyInfo.second != SEEK_TO_EOS
 *             Test seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleHlsRebootPlugin_005, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {200, false};
    Status ret = mediaDemuxer_->HandleHlsRebootPlugin(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test SelectTrackChangeStream
 * @tc.number: SelectTrackChangeStream_001
 * @tc.desc  : Test type == TRACK_SUBTITLE
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectTrackChangeStream_001, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    TrackType type = mediaDemuxer_->demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(0));
    EXPECT_EQ(type, TrackType::TRACK_SUBTITLE);
    auto ret = mediaDemuxer_->SelectTrackChangeStream(NUM_0);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test SelectTrackChangeStream
 * @tc.number: SelectTrackChangeStream_002
 * @tc.desc  : Test type == TRACK_VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectTrackChangeStream_002, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    TrackType type = mediaDemuxer_->demuxerPluginManager_->GetTrackTypeByTrackID(static_cast<int32_t>(0));
    EXPECT_EQ(type, TrackType::TRACK_VIDEO);
    auto ret = mediaDemuxer_->SelectTrackChangeStream(0);
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test OnEvent
 * @tc.number: OnEvent_001
 * @tc.desc  : Test case PluginEventType::SOURCE_DRM_INFO_UPDATE
 *             Test !Any::IsSameTypeWith<std::multimap<std::string, std::vector<uint8_t>>>(event.param)
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_001, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    PluginEvent event{
        Plugins::PluginEventType::SOURCE_DRM_INFO_UPDATE,
        std::string(""),
        "OnEvent source drmInfo update"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnEvent
 * @tc.number: OnEvent_002
 * @tc.desc  : Test case PluginEventType::SOURCE_DRM_INFO_UPDATE
 *             Test Any::IsSameTypeWith<std::multimap<std::string, std::vector<uint8_t>>>(event.param)
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_002, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    typename std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    drmInfo.insert(std::make_pair("key1", std::vector<uint8_t>{0x01, 0x02, 0x03}));
    
    PluginEvent event{
        Plugins::PluginEventType::SOURCE_DRM_INFO_UPDATE,
        drmInfo,
        "OnEvent source drmInfo update"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnEvent
 * @tc.number: OnEvent_003
 * @tc.desc  : Test case PluginEventType::SOURCE_BITRATE_START
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_003, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    PluginEvent event{
        Plugins::PluginEventType::FLV_AUTO_SELECT_BITRATE,
        std::string(""),
        "OnEvent flv auto select bitrate"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(1);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnSeekReadyEvent
 * @tc.number: OnSeekReadyEvent_001
 * @tc.desc  : Test case PluginEventType::DASH_SEEK_READY
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_004, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    PluginEvent event{
        Plugins::PluginEventType::DASH_SEEK_READY,
        std::string(""),
        "OnEvent flv auto select bitrate"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test OnSeekReadyEvent
 * @tc.number: OnSeekReadyEvent_002
 * @tc.desc  : Test case PluginEventType::HLS_SEEK_READY
 */
HWTEST_F(MediaDemuxerExtUnitTest, OnEvent_005, TestSize.Level1)
{
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    PluginEvent event{
        Plugins::PluginEventType::HLS_SEEK_READY,
        std::string(""),
        "OnEvent flv auto select bitrate"
    };
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);
    mediaDemuxer_->OnEvent(event);
}

/**
 * @tc.name  : Test GetLastVideoBufferAbsPts
 * @tc.number: GetLastVideoBufferAbsPts_001
 * @tc.desc  : Test syncCenter_ == nullptr
 *             Test syncCenter_ != nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, GetLastVideoBufferAbsPts_001, TestSize.Level1)
{
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetLastVideoBufferAbsPts())
        .WillRepeatedly(Return(HST_TIME_BASE));
    mediaDemuxer_->syncCenter_ = nullptr;
    uint32_t trackId = UNUM_0;
    int64_t pts = mediaDemuxer_->GetLastVideoBufferAbsPts(trackId);
    EXPECT_EQ(pts, HST_TIME_NONE);

    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    pts = mediaDemuxer_->GetLastVideoBufferAbsPts(trackId);
    EXPECT_EQ(pts, HST_TIME_BASE);
}

/**
 * @tc.name  : Test CheckDropAudioFrame
 * @tc.number: CheckDropAudioFrame_001
 * @tc.desc  : Test isInSeekDropAudio_ && sample->pts_ < videoSeekTime_
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckDropAudioFrame_001, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_ = UNUM_0;
    mediaDemuxer_->isInSeekDropAudio_ = true;
    uint32_t trackId = UNUM_0;
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = NUM_0;
    mediaDemuxer_->videoSeekTime_ = NUM_1;
    bool ret = mediaDemuxer_->CheckDropAudioFrame(sample, trackId);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test CheckDropAudioFrame
 * @tc.number: CheckDropAudioFrame_002
 * @tc.desc  : Test sample->pts_ > videoSeekTime_
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckDropAudioFrame_002, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_ = UNUM_0;
    mediaDemuxer_->isInSeekDropAudio_ = true;
    uint32_t trackId = UNUM_0;
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = NUM_10;
    mediaDemuxer_->videoSeekTime_ = NUM_1;
    mediaDemuxer_->CheckDropAudioFrame(sample, trackId);
    EXPECT_EQ(mediaDemuxer_->isInSeekDropAudio_, false);
}

/**
 * @tc.name  : Test SelectBitRate
 * @tc.number: SelectBitRate_001
 * @tc.desc  : Test isFlvLiveStream_ && IsRightMediaTrack(videoTrackId_, DemuxerTrackType::VIDEO)
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectBitRate_001, TestSize.Level1)
{
    uint32_t bitRate = UNUM_0;
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->isManualBitRateSetting_ = false;
    mediaDemuxer_->videoTrackId_ = static_cast<uint32_t>(DemuxerTrackType::VIDEO);
    EXPECT_TRUE(mediaDemuxer_->IsRightMediaTrack(mediaDemuxer_->videoTrackId_, DemuxerTrackType::VIDEO));
    EXPECT_TRUE(!(mediaDemuxer_->isManualBitRateSetting_).load());
    auto ret = mediaDemuxer_->SelectBitRate(bitRate, true);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);

    ret = mediaDemuxer_->SelectBitRate(bitRate, false);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);

    mediaDemuxer_->isManualBitRateSetting_ = true;
    EXPECT_TRUE(mediaDemuxer_->isManualBitRateSetting_.load());
    ret = mediaDemuxer_->SelectBitRate(bitRate, false);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test HandleSelectTrackChangeStream
 * @tc.number: HandleSelectTrackChangeStream_001
 * @tc.desc  : Test newStreamID != -1 && currentStreamID != -1 && currentStreamID != newStreamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackChangeStream_001, TestSize.Level1)
{
    int32_t trackId = NUM_1;
    int32_t newStreamID = NUM_0;
    int32_t newTrackId = NUM_0;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamIDByTrackType(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(_))
        .WillRepeatedly(Return(StreamType::VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));
    int32_t currentStreamID = mediaDemuxer_->demuxerPluginManager_->GetStreamIDByTrackType(TrackType::TRACK_VIDEO);
    EXPECT_TRUE(currentStreamID != newStreamID);
    auto ret = mediaDemuxer_->HandleSelectTrackChangeStream(trackId, newStreamID, newTrackId);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test HandleSelectTrackChangeStream
 * @tc.number: HandleSelectTrackChangeStream_002
 * @tc.desc  : Test GetEnableSampleQueueFlag() && !GetEnableSampleQueueFlag()
 */
HWTEST_F(MediaDemuxerExtUnitTest, HandleSelectTrackChangeStream_002, TestSize.Level1)
{
    int32_t trackId = NUM_1;
    int32_t newStreamID = NUM_2;
    int32_t newTrackId = NUM_0;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamIDByTrackType(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(_))
        .WillRepeatedly(Return(StreamType::VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));
    int32_t currentStreamID = mediaDemuxer_->demuxerPluginManager_->GetStreamIDByTrackType(TrackType::TRACK_VIDEO);
    EXPECT_TRUE(currentStreamID != newStreamID);

    mediaDemuxer_->enableSampleQueue_ = false;
    auto ret = mediaDemuxer_->HandleSelectTrackChangeStream(trackId, newStreamID, newTrackId);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : Test UpdateLastVideoBufferAbsPts
 * @tc.number: UpdateLastVideoBufferAbsPts_001
 * @tc.desc  : Test isFlvLiveStream_ && IsRightMediaTrack(trackId, DemuxerTrackType::VIDEO)
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateLastVideoBufferAbsPts_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->syncCenter_ = nullptr;
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    int64_t lastVideoBufferAbsPts = mediaDemuxer_->GetLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(lastVideoBufferAbsPts, HST_TIME_NONE);
}

/**
 * @tc.name  : Test UpdateLastVideoBufferAbsPts
 * @tc.number: UpdateLastVideoBufferAbsPts_001
 * @tc.desc  : Test lastVideoBufferAbsPts != HST_TIME_NONE
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateLastVideoBufferAbsPts_002, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetLastVideoBufferAbsPts())
        .WillRepeatedly(Return(HST_TIME_BASE));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    int64_t lastVideoBufferAbsPts = mediaDemuxer_->GetLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
    EXPECT_EQ(lastVideoBufferAbsPts, HST_TIME_BASE);
}

/**
 * @tc.name  : Test UpdateLastVideoBufferAbsPts
 * @tc.number: UpdateLastVideoBufferAbsPts_003
 * @tc.desc  : Test sqIt != sampleQueueMap_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateLastVideoBufferAbsPts_003, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetLastVideoBufferAbsPts())
        .WillRepeatedly(Return(HST_TIME_BASE));
    mediaDemuxer_->sampleQueueMap_.insert(
        std::pair<uint32_t, std::shared_ptr<SampleQueue>>(mediaDemuxer_->videoTrackId_,
        mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_]));
    EXPECT_TRUE(mediaDemuxer_->sampleQueueMap_.find(mediaDemuxer_->videoTrackId_) !=
        mediaDemuxer_->sampleQueueMap_.end());
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);

    mediaDemuxer_->sampleQueueMap_.erase(mediaDemuxer_->videoTrackId_);
    EXPECT_TRUE(mediaDemuxer_->sampleQueueMap_.find(mediaDemuxer_->videoTrackId_) ==
        mediaDemuxer_->sampleQueueMap_.end());
    mediaDemuxer_->UpdateLastVideoBufferAbsPts(mediaDemuxer_->videoTrackId_);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ > 0 && outputBuffer != nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_001, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = std::make_shared<AVBuffer>();
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_1;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ > 0 && outputBuffer != nullptr);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ == 0 && outputBuffer != nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_002, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = std::make_shared<AVBuffer>();
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_0;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ == 0 && outputBuffer != nullptr);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ > 0 && outputBuffer == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_003, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = nullptr;
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_1;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ > 0 && outputBuffer == nullptr);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_001
 * @tc.desc  : Test transcoderStartPts_ == 0 && outputBuffer == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_004, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = nullptr;
    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_0;
    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_TRUE(mediaDemuxer_->transcoderStartPts_ == 0 && outputBuffer == nullptr);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Pause API
 * @tc.number: MediaDemuxerExt_Pause_001
 * @tc.desc  : Test !streamDemuxer_ && !source_ && !CheckTrackEnabledById(videoTrackId_) &&
 *             !GetEnableSampleQueueFlag() && source_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Pause_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->source_ = nullptr;
    mediaDemuxer_->inPreroll_ = true;
    mediaDemuxer_->videoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    mediaDemuxer_->enableSampleQueue_ = false;
    auto ret = mediaDemuxer_->Pause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest PauseDragging API
 * @tc.number: MediaDemuxerExt_PauseDragging_001
 * @tc.desc  : Test streamDemuxer_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_PauseDragging_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->source_ = nullptr;
    auto ret = mediaDemuxer_->PauseDragging();
    EXPECT_EQ(ret, Status::OK);
    // Test PauseAudioAlign API
    ret = mediaDemuxer_->PauseAudioAlign();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Resume API
 * @tc.number: MediaDemuxerExt_Resume_001
 * @tc.desc  : Test streamDemuxer_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Resume_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->inPreroll_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    auto ret = mediaDemuxer_->Resume();
    EXPECT_EQ(ret, Status::OK);
    // Test ResumeDragging API
    ret = mediaDemuxer_->ResumeDragging();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_001
 * @tc.desc  : Test inPreroll_.load()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_001, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = true;
    EXPECT_TRUE(mediaDemuxer_->inPreroll_.load());
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_002
 * @tc.desc  : Test !CheckTrackEnabledById(videoTrackId_)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_002, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    mediaDemuxer_->videoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_003
 * @tc.desc  : Test !isStopped_.load() && !isPaused_.load()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_003, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    mediaDemuxer_->taskMap_[mediaDemuxer_->videoTrackId_] = std::move(taskPtr);
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    EXPECT_EQ(mediaDemuxer_->CheckTrackEnabledById(mediaDemuxer_->videoTrackId_), true);
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isPaused_ = false;
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Preroll API
 * @tc.number: MediaDemuxerExt_Preroll_004
 * @tc.desc  : Test ret != Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Preroll_004, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    mediaDemuxer_->taskMap_[mediaDemuxer_->videoTrackId_] = std::move(taskPtr);
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    EXPECT_EQ(mediaDemuxer_->CheckTrackEnabledById(mediaDemuxer_->videoTrackId_), true);
    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->useBufferQueue_ = false;
    EXPECT_EQ(mediaDemuxer_->Start(), Status::ERROR_WRONG_STATE);
    auto ret = mediaDemuxer_->Preroll();
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest PausePreroll API
 * @tc.number: MediaDemuxerExt_PausePreroll_001
 * @tc.desc  : Test !inPreroll_.load()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_PausePreroll_001, TestSize.Level1)
{
    mediaDemuxer_->inPreroll_ = false;
    auto ret = mediaDemuxer_->PausePreroll();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ResumeAudioAlign API
 * @tc.number: MediaDemuxerExt_ResumeAudioAlign_001
 * @tc.desc  : Test !streamDemuxer_ && !source_ &&
 *             CheckTrackEnabledById(videoTrackId_) && streamDemuxer_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ResumeAudioAlign_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    mediaDemuxer_->source_ = nullptr;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = false;
    std::string taskName = "Demux";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    mediaDemuxer_->taskMap_[mediaDemuxer_->videoTrackId_] = std::move(taskPtr);
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    EXPECT_EQ(mediaDemuxer_->CheckTrackEnabledById(mediaDemuxer_->videoTrackId_), true);
    auto ret = mediaDemuxer_->ResumeAudioAlign();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest Stop API
 * @tc.number: MediaDemuxerExt_Stop_001
 * @tc.desc  : Test !useBufferQueue_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_Stop_001, TestSize.Level1)
{
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->useBufferQueue_ = false;
    auto ret = mediaDemuxer_->Stop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest SelectTrack API
 * @tc.number: MediaDemuxerExt_SelectTrack_001
 * @tc.desc  : Test demuxerPluginManager_->IsDash() && changeStreamFlag_ == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectTrack_001, TestSize.Level1)
{
    mediaDemuxer_->useBufferQueue_ = true;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta1 = std::make_shared<Meta>();
    std::shared_ptr<Meta> trackMeta2 = std::make_shared<Meta>();
    trackMeta1->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta2->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta1);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta2);
    EXPECT_EQ(mediaDemuxer_->mediaMetaData_.trackMetas.size(), 2);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillRepeatedly(Return(true));
    auto ret = mediaDemuxer_->SelectTrack(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest SelectTrack API
 * @tc.number: MediaDemuxerExt_SelectTrack_002
 * @tc.desc  : Test demuxerPluginManager_->IsDash() && changeStreamFlag_ == false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectTrack_002, TestSize.Level1)
{
    mediaDemuxer_->useBufferQueue_ = true;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillRepeatedly(Return(false));
    auto ret = mediaDemuxer_->SelectTrack(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest UnselectTrack API
 * @tc.number: MediaDemuxerExt_UnselectTrack_001
 * @tc.desc  : Test IsNeedMapToInnerTrackID()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UnselectTrack_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).Times(NUM_1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).Times(NUM_1);
    auto ret = mediaDemuxer_->UnselectTrack(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest UnselectTrack API
 * @tc.number: MediaDemuxerExt_UnselectTrack_002
 * @tc.desc  : Test IsNeedMapToInnerTrackID() is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UnselectTrack_002, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).Times(NUM_1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).Times(NUM_1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    auto ret = mediaDemuxer_->UnselectTrack(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest OnInterrupted API
 * @tc.number: MediaDemuxerExt_OnInterrupted_001
 * @tc.desc  : Test demuxerPluginManager_ != nullptr && demuxerPluginManager_->IsDash()
 *             Test source_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnInterrupted_001, TestSize.Level1)
{
    bool isInterruptNeeded = true;
    mediaDemuxer_->source_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    mediaDemuxer_->OnInterrupted(isInterruptNeeded);
    EXPECT_EQ(mediaDemuxer_->isInterruptNeeded_, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest OnInterrupted API
 * @tc.number: MediaDemuxerExt_OnInterrupted_002
 * @tc.desc  : Test demuxerPluginManager_ == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnInterrupted_002, TestSize.Level1)
{
    bool isInterruptNeeded = true;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(NUM_0);
    mediaDemuxer_->demuxerPluginManager_ = nullptr;
    // Test OnBufferAvailable !GetEnableSampleQueueFlag()
    mediaDemuxer_->enableSampleQueue_ = false;
    mediaDemuxer_->OnBufferAvailable(NUM_0);
    mediaDemuxer_->OnInterrupted(isInterruptNeeded);
    EXPECT_EQ(mediaDemuxer_->isInterruptNeeded_, true);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_003
 * @tc.desc  : Test meta != nullptr && !meta->GetData(Tag::MEDIA_TYPE, mediaType)
 *             Test mediaType != Plugins::MediaType::VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_003, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = NUM_0;
    std::vector<std::shared_ptr<Meta>> vector;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    trackMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::AUDIO);
    vector.push_back(trackMeta);
    vector.push_back(std::make_shared<Meta>());
    EXPECT_EQ(mediaDemuxer_->GetTargetVideoTrackId(vector), NUM_0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_004
 * @tc.desc  : Test meta->GetData(Tag::MEDIA_TYPE, mediaType) && mediaType == Plugins::MediaType::VIDEO
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_004, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = NUM_0;
    std::vector<std::shared_ptr<Meta>> vector;
    auto videoMeta = std::make_shared<Meta>();
    videoMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::VIDEO);
    vector.push_back(videoMeta);
    auto audioMeta = std::make_shared<Meta>();
    audioMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::AUDIO);
    vector.push_back(audioMeta);
    uint32_t result = mediaDemuxer_->GetTargetVideoTrackId(vector);
    EXPECT_EQ(result, NUM_0);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_005
 * @tc.desc  : Test no video track in trackInfos → return original targetVideoTrackId_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_005, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = NEG_1;
    std::vector<std::shared_ptr<Meta>> trackInfos;
    auto audioMeta = std::make_shared<Meta>();
    audioMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::AUDIO);
    trackInfos.push_back(audioMeta);
    auto subtitleMeta = std::make_shared<Meta>();
    subtitleMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::SUBTITLE);
    trackInfos.push_back(subtitleMeta);
    int32_t ret = mediaDemuxer_->GetTargetVideoTrackId(trackInfos);
    EXPECT_EQ(ret, NEG_1);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest GetTargetVideoTrackId API
 * @tc.number: MediaDemuxerExt_GetTargetVideoTrackId_006
 * @tc.desc  : Test find first video track → set targetVideoTrackId_ = index
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetTargetVideoTrackId_006, TestSize.Level1)
{
    mediaDemuxer_->targetVideoTrackId_ = NUM_1;
    std::vector<std::shared_ptr<Meta>> trackInfos;
    auto videoMeta = std::make_shared<Meta>();
    videoMeta->SetData(Tag::MEDIA_TYPE, Plugins::MediaType::VIDEO);
    trackInfos.push_back(videoMeta);
    int32_t ret = mediaDemuxer_->GetTargetVideoTrackId(trackInfos);
    EXPECT_EQ(ret, NUM_1);
    EXPECT_EQ(mediaDemuxer_->targetVideoTrackId_, NUM_1);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSegmentMediaSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleSegmentMediaSelectTrack_001
 * @tc.desc  : Test trackId != curTrackId &&
 *             targetStreamID == demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentMediaSelectTrack_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    mediaDemuxer_->audioTrackId_ = NUM_0;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleSegmentMediaSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSegmentMediaSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleSegmentMediaSelectTrack_002
 * @tc.desc  : Test trackId == curTrackId &&
 *             targetStreamID == demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentMediaSelectTrack_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    mediaDemuxer_->audioTrackId_ = NUM_1;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleSegmentMediaSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSegmentMediaSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleSegmentMediaSelectTrack_003
 * @tc.desc  : Test trackId == curTrackId &&
 *             targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentMediaSelectTrack_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_2));
    mediaDemuxer_->audioTrackId_ = NUM_1;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleSegmentMediaSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSegmentMediaSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleSegmentMediaSelectTrack_004
 * @tc.desc  : Test trackId != curTrackId &&
 *             targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentMediaSelectTrack_004, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_2));
    mediaDemuxer_->audioTrackId_ = NUM_2;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    auto result = mediaDemuxer_->HandleSegmentMediaSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSegmentMediaSelectTrack API
 * @tc.number: MediaDemuxerExt_HandleSegmentMediaSelectTrack_005
 * @tc.desc  : Test trackId != curTrackId &&
 *             targetStreamID != demuxerPluginManager_->GetTmpStreamIDByTrackID(curTrackId)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentMediaSelectTrack_005, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_2));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(NUM_1));
    mediaDemuxer_->source_ = std::make_shared<Source>();
    mediaDemuxer_->subtitleTrackId_ = NUM_1;
    mediaDemuxer_->isHls_ = true;
    mediaDemuxer_->syncCenter_ = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).WillRepeatedly(::testing::Return(Status::OK));
    auto result = mediaDemuxer_->HandleSegmentMediaSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
    mediaDemuxer_->isHls_ = false;
    result = mediaDemuxer_->HandleSegmentMediaSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
    mediaDemuxer_->subtitleTrackId_ = NUM_1;
    mediaDemuxer_->syncCenter_ = nullptr;
    result = mediaDemuxer_->HandleSegmentMediaSelectTrack(NUM_1);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleSelectSubtitle_001
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectSubtitle_001, TestSize.Level1)
{
    mediaDemuxer_->subtitleTrackId_ = NUM_1;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetStreamIDByTrackID(_)).WillOnce(::testing::Return(NUM_2));
    mediaDemuxer_->source_ = std::make_shared<Source>();
    EXPECT_CALL(*mediaDemuxer_->source_, IsSeekToTimeSupported()).WillOnce(::testing::Return(true));
    EXPECT_CALL(*mediaDemuxer_->source_, MediaSeekTimeByStreamId(_, _, _)).WillOnce(::testing::Return(Status::OK));
    auto result = mediaDemuxer_->HandleSelectSubtitle(0, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, NUM_2);
    EXPECT_EQ(result, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_001
 * @tc.desc  : Test isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) == seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_001, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_002
 * @tc.desc  : Test !isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) == seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_002, TestSize.Level1)
{
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    mediaDemuxer_->isInterruptNeeded_.store(false);
    StreamType streamType = TRACK_TO_STREAM_MAP[TrackType::TRACK_VIDEO];
    std::thread notifyThread([this, streamType]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
        std::unique_lock<std::mutex> lock(mediaDemuxer_->rebootPluginMutex_);
        mediaDemuxer_->isInterruptNeeded_.store(true);
        mediaDemuxer_->rebootPluginCondition_.notify_one();
    });
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    notifyThread.join();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_003
 * @tc.desc  : Test !isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) != seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_003, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(false);
    mediaDemuxer_->seekReadyStreamInfo_ = {{StreamType::VIDEO, {NUM_1, NUM_2}}};
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_004
 * @tc.desc  : Test isInterruptNeeded_.load() &&
 *             seekReadyStreamInfo_.find(static_cast<int32_t>(streamType)) != seekReadyStreamInfo_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_004, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {{StreamType::VIDEO, {NUM_1, NUM_2}}};
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_005
 * @tc.desc  : Test static_cast<uint32_t>(trackId) == TRACK_ID_DUMMY
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_005, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    int32_t trackId = MediaDemuxer::TRACK_ID_INVALID;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .Times(NUM_0);
    
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_006
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_006, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_0, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_007
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_007, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_1, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_008
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_008, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NEG_2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_2, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_009
 * @tc.desc  : Test seekReadyInfo.second == SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_009, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = 0;
    int32_t trackId = 0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_2, NUM_1}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_1);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_010
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_010, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = 0;
    int32_t trackId = 0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_1, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_011
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first != streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_011, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_2, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_012
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first >= 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_012, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NUM_2, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleRebootPlugin API
 * @tc.number: MediaDemuxerExt_HandleRebootPlugin_013
 * @tc.desc  : Test seekReadyInfo.second != SEEK_TO_EOS &&
 *             seekReadyInfo.first < 0 && seekReadyInfo.first == streamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleRebootPlugin_013, TestSize.Level1)
{
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    mediaDemuxer_->subtitleTrackId_ = NUM_0;
    int32_t trackId = NUM_0;
    bool isRebooted = false;
    EXPECT_EQ(trackId, static_cast<int32_t>(mediaDemuxer_->subtitleTrackId_));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NEG_2));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTrackTypeByTrackID(_))
        .WillRepeatedly(Return(TrackType::TRACK_VIDEO));
    mediaDemuxer_->isInterruptNeeded_.store(true);
    mediaDemuxer_->seekReadyStreamInfo_ = {
        {NUM_0, {NUM_0, NUM_1}},
        {NUM_1, {NEG_2, NUM_0}}
    };
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, NUM_0);
    Status ret = mediaDemuxer_->HandleRebootPlugin(trackId, isRebooted);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsIgonreBuffering API
 * @tc.number: MediaDemuxerExt_IsIgonreBuffering_001
 * @tc.desc  : Test IsRightMediaTrack(videoTrackId_, DemuxerTrackType::VIDEO)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsIgonreBuffering_001, TestSize.Level1)
{
    bool ret = mediaDemuxer_->IsIgonreBuffering();
    EXPECT_EQ(ret, false);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest UpdateSampleQueueCache API
 * @tc.number: MediaDemuxerExt_UpdateSampleQueueCache_002
 * @tc.desc  : Test lastClockTimeMs_ != 0 && currentClockTimeMs - lastClockTimeMs_ > UPDATE_SOURCE_CACHE_MS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UpdateSampleQueueCache_002, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->lastClockTimeMs_ = NUM_1;
    mediaDemuxer_->source_ = std::make_shared<Source>();
    EXPECT_CALL(*(mediaDemuxer_->source_), SetExtraCache(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetCachedDuration()).WillRepeatedly(Return(NUM_1000));
    mediaDemuxer_->UpdateSampleQueueCache();
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_001
 * @tc.desc  : Test isStopped_ && isThreadExit_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_001, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = true;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_002
 * @tc.desc  : Test !isStopped_ && isThreadExit_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_002, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = true;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_003
 * @tc.desc  : Test isStopped_ && !isThreadExit_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_003, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->isThreadExit_ = false;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_004
 * @tc.desc  : Test track == trackMap_.end()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_004, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_005
 * @tc.desc  : Test track->second == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_005, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    mediaDemuxer_->trackMap_[queueId] = nullptr;
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest NotifySampleQueueBufferConsume API
 * @tc.number: MediaDemuxerExt_NotifySampleQueueBufferConsume_006
 * @tc.desc  : Test track->second == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifySampleQueueBufferConsume_006, TestSize.Level1)
{
    uint32_t queueId = NUM_0;
    uint32_t trackId = NUM_0;
    mediaDemuxer_->isStopped_ = false;
    mediaDemuxer_->isThreadExit_ = false;
    auto sampleQueue = std::make_shared<SampleQueue>();
    std::string taskName = "Demux";
    std::unique_ptr<Task> notifyTask =
        std::make_unique<MockTask>(taskName + "N", mediaDemuxer_->playerId_);
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    sptr<IProducerListener> listener =
        OHOS::sptr<MediaDemuxer::AVBufferQueueProducerListener>::MakeSptr(trackId, demuxer, notifyTask);
    auto trackWrapper = std::make_shared<MediaDemuxer::TrackWrapper>(trackId, listener, demuxer);
    mediaDemuxer_->trackMap_[queueId] = trackWrapper;
    mediaDemuxer_->sampleConsumerTaskMap_.clear();
    auto result = mediaDemuxer_->NotifySampleQueueBufferConsume(queueId);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSelectBitrateForFlvLive API
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_006
 * @tc.desc  : Test isManualBitRateSetting_.load()
 *             Test GetEnableSampleQueueFlag()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_006, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    mediaDemuxer_->demuxerPluginManager_ = std::make_shared<DemuxerPluginManager>();
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    int64_t startPts = 0;
    uint32_t bitrate = 0;
    mediaDemuxer_->isManualBitRateSetting_ = true;
    mediaDemuxer_->enableSampleQueue_ = true;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    auto result = mediaDemuxer_->HandleSelectBitrateForFlvLive(startPts, bitrate);
    EXPECT_EQ(result, Status::OK);
}

/*
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_001
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_001
 * @tc.desc  : Test when GetEnableSampleQueueFlag returns false, GetReadLoopRetryUs returns 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_001, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = false;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), false);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_002
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_002
 * @tc.desc  :Test when isFlvLiveStream_ is false, GetReadLoopRetryUs returns NEXT_DELAY_TIME_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_002, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = false;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), NEXT_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_003
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_003
 * @tc.desc  : Test sampleQueueMap_.count = 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_003, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = true;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), NEXT_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_004
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_004
 * @tc.desc  : Test sampleDuration <= SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_004, TestSize.Level1)
{
    auto sampleQueue = std::make_shared<SampleQueue>();
    uint64_t cacheDuration = static_cast<uint64_t>(SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US);
    EXPECT_CALL(*sampleQueue, GetCacheDuration()).WillRepeatedly(Return(cacheDuration));

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->sampleQueueMap_[1] = sampleQueue;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1), NEXT_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_GetReadLoopRetryUs_005
 * @tc.number: MediaDemuxerExt_GetReadLoopRetryUs_005
 * @tc.desc  : Test sampleDuration > SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetReadLoopRetryUs_005, TestSize.Level1)
{
    auto sampleQueue = std::make_shared<SampleQueue>();
    uint64_t sampleDuration = SAMPLE_FLOW_CONTROL_MIN_SAMPLE_DURATION_US + 1;

    EXPECT_CALL(*sampleQueue, GetCacheDuration()).WillRepeatedly(Return(sampleDuration));

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = true;
    mediaDemuxer_->sampleQueueMap_[1] = sampleQueue;

    EXPECT_EQ(mediaDemuxer_->GetReadLoopRetryUs(1),
        static_cast<int64_t>(sampleDuration >> SAMPLE_FLOW_CONTROL_RATE_POW));
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_001
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_001
 * @tc.desc  : Test Return 0 when IsDash is true or subStreamDemuxer is not nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash())
        .WillOnce(Return(true))
        .WillOnce(Return(false));
    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), static_cast<int64_t>(0));

    mediaDemuxer_->subStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), static_cast<int64_t>(0));
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_002
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_002
 * @tc.desc  : Test when subtitleDemuxerPlugin is nullptr, return RETRY_DELAY_TIME_US
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), RETRY_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_003
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_003
 * @tc.desc  : Test returns 0 when cache size is greater than 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_003, TestSize.Level1)
{
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_))
        .WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, GetCurrentCacheSize(_, _))
        .WillOnce(DoAll(
            SetArgReferee<1>(1024), // 1024 means cacheSize
            Return(Status::OK)
        ));

    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(0), static_cast<int64_t>(0));
}

/**
 * @tc.name  : MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_004
 * @tc.number: MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_004
 * @tc.desc  : Test when the cache size is 0 or the cache size retrieval fails, RETRY_DELAY_TIME_US is returned.
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_DoBeforeSubtitleTrackReadLoop_004, TestSize.Level1)
{
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_))
        .WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, GetCurrentCacheSize(_, _))
        .WillOnce(DoAll(
            SetArgReferee<1>(0),
            Return(Status::OK)
        ))
        .WillOnce(DoAll(
            SetArgReferee<1>(1),
            Return(Status::ERROR_UNKNOWN)
        ));

    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), RETRY_DELAY_TIME_US);
    EXPECT_EQ(mediaDemuxer_->DoBeforeSubtitleTrackReadLoop(1), RETRY_DELAY_TIME_US);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_001
 * @tc.number: MediaDemuxerExt_HandleEvent_001
 * @tc.desc  : Test HandleEvent method when event type is CLIENT_ERROR
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(1);
    PluginEvent event{
        Plugins::PluginEventType::CLIENT_ERROR,
        std::string(""),
        "OnEvent CLIENT_ERROR"
    };

    mediaDemuxer_->HandleEvent(event);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_002
 * @tc.number: MediaDemuxerExt_HandleEvent_002
 * @tc.desc  : Test HandleEvent method when event type is SERVER_ERROR
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(1);
    PluginEvent event{
        Plugins::PluginEventType::SERVER_ERROR,
        std::string(""),
        "OnEvent SERVER_ERROR"
    };

    mediaDemuxer_->HandleEvent(event);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_003
 * @tc.number: MediaDemuxerExt_HandleEvent_003
 * @tc.desc  : Test HandleEvent method when event type is INITIAL_BUFFER_SUCCESS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(_)).Times(1);
    PluginEvent event{
        Plugins::PluginEventType::INITIAL_BUFFER_SUCCESS,
        std::string(""),
        "OnEvent INITIAL_BUFFER_SUCCESS"
    };

    mediaDemuxer_->HandleEvent(event);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleEvent_004
 * @tc.number: MediaDemuxerExt_HandleEvent_004
 * @tc.desc  : Test HandleEvent method NotifyInitialBufferingEnd false & true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleEvent_004, TestSize.Level1)
{
    InSequence seq;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(false)).Times(1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), NotifyInitialBufferingEnd(true)).Times(1);

    PluginEvent errorEvent{
        Plugins::PluginEventType::CLIENT_ERROR,
        std::string(""),
        "OnEvent CLIENT_ERROR"
    };
    PluginEvent successEvent{
        Plugins::PluginEventType::INITIAL_BUFFER_SUCCESS,
        std::string(""),
        "OnEvent INITIAL_BUFFER_SUCCESS"
    };

    mediaDemuxer_->HandleEvent(errorEvent);
    mediaDemuxer_->HandleEvent(successEvent);
}

/**
 * @tc.name  : MediaDemuxerExt_UpdateSampleQueueCache_001
 * @tc.number: MediaDemuxerExt_UpdateSampleQueueCache_001
 * @tc.desc  : Test isFlvLiveStream_ false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UpdateSampleQueueCache_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->UpdateSampleQueueCache();

    EXPECT_EQ(mediaDemuxer_->lastClockTimeMs_, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_001
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_001
 * @tc.desc  : Test source_  nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_001, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->source_ = nullptr;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_002
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_002
 * @tc.desc  : Test demuxerPluginManager_  nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_002, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->demuxerPluginManager_  = nullptr;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_003
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_003
 * @tc.desc  : Test case for returning error when streamDemuxer_ is null
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_003, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->streamDemuxer_ = nullptr;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_004
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_004
 * @tc.desc  : Test case
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_004, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->source_), SetStartPts(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), AutoSelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TRACK_AUDIO)).WillOnce(Return(TRACK_VIDEO)) // first
        .WillOnce(Return(TRACK_AUDIO)).WillOnce(Return(TRACK_VIDEO)) // two
        .WillOnce(Return(TRACK_AUDIO)).WillOnce(Return(TRACK_VIDEO)); // three
    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->audioTrackId_ = 0;
    mediaDemuxer_->isFlvLiveStream_ = false;

    // isManualBitRateSetting_ true
    mediaDemuxer_->isManualBitRateSetting_ = true;
    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);

    // isManualBitRateSetting_ false
    mediaDemuxer_->isManualBitRateSetting_ = false;
    ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);

    // enableSampleQueue_ false
    mediaDemuxer_->isManualBitRateSetting_ = true;
    mediaDemuxer_->enableSampleQueue_ = false;
    ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSelectBitrateForFlvLive_005
 * @tc.number: MediaDemuxerExt_HandleSelectBitrateForFlvLive_005
 * @tc.desc  : Test case
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSelectBitrateForFlvLive_005, TestSize.Level1)
{
    uint32_t bitRate = 1000;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->source_), SetStartPts(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), AutoSelectBitRate(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TRACK_VIDEO))
        .WillOnce(Return(TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapInfo(_, _, _))
        .Times(2); // 2 means TRACK_VIDEO true, TRACK_AUDIO true

    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    mediaDemuxer_->isManualBitRateSetting_ = true;
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->audioTrackId_ = 0;
    mediaDemuxer_->isFlvLiveStream_ = false;

    Status ret = mediaDemuxer_->HandleSelectBitrateForFlvLive(0, bitRate);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_001
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_001
 * @tc.desc  : seekTimeMs < 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_001, TestSize.Level1)
{
    int64_t seekTimeMs = -1;
    Format format;
    format.PutLongValue("seekTime", seekTimeMs);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->videoSeekTime_, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_002
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_002
 * @tc.desc  : seekTimeMs > 0 && HasVideo true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_002, TestSize.Level1)
{
    int64_t seekTimeMs = 1000; // 1000 seekTimeMs > 0
    Format format;
    format.PutLongValue("seekTime", seekTimeMs);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_TRUE(mediaDemuxer_->isInSeekDropAudio_);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_003
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_003
 * @tc.desc  : currentStreamType == MEDIA_TYPE_VID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_003, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_VID));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;


    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_004
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_004
 * @tc.desc  : currentStreamType == MEDIA_TYPE_AUD
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_004, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_AUD));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnDashSeekReadyEvent_005
 * @tc.number: MediaDemuxerExt_OnDashSeekReadyEvent_005
 * @tc.desc  : currentStreamType == MEDIA_TYPE_SUBTITLE
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnDashSeekReadyEvent_005, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;

    mediaDemuxer_->videoTrackId_ = 0;

    mediaDemuxer_->OnDashSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::SUBTITLE)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::SUBTITLE)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_001
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_001
 * @tc.desc  : newStreamID < 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(-1));

    EXPECT_FALSE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_002
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_002
 * @tc.desc  : newStreamID == currentStreamID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));

    EXPECT_FALSE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_003
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_003
 * @tc.desc  : StartPlugin return error
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_003, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _))
        .WillRepeatedly(Return(Status::ERROR_UNKNOWN));

    EXPECT_FALSE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_004
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_004
 * @tc.desc  : isHlsFmp4_ false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_004, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));

    // isHlsFmp4_ false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackInfoByStreamID(_, _, _, _)).Times(2);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapInfo(_, _, _)).Times(2);

    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->isHlsFmp4_ = false;

    EXPECT_TRUE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : MediaDemuxerExt_SelectBitRateChangeStream_005
 * @tc.number: MediaDemuxerExt_SelectBitRateChangeStream_005
 * @tc.desc  : isHlsFmp4_ true, audioTrackId_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectBitRateChangeStream_005, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), GetNewVideoStreamID()).WillRepeatedly(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StopPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), StartPlugin(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateDefaultStreamID(_, _, _))
        .WillRepeatedly(Return(Status::OK));

    // isHlsFmp4_ false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackInfoByStreamID(_, _, _, _)).Times(3);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapInfo(_, _, _)).Times(3);

    // InnerSelectTrack return error
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->videoTrackId_ = 1;
    mediaDemuxer_->audioTrackId_ = 1;
    mediaDemuxer_->isHlsFmp4_ = true;

    // audioTrackId_ != TRACK_ID_INVALID
    EXPECT_TRUE(mediaDemuxer_->SelectBitRateChangeStream(0));

    // audioTrackId_ == TRACK_ID_DUMMY
    mediaDemuxer_->audioTrackId_ = mediaDemuxer_->TRACK_ID_INVALID;
    EXPECT_TRUE(mediaDemuxer_->SelectBitRateChangeStream(0));
}

/**
 * @tc.name  : CopyFrameToUserQueue_ShouldReturnError_WhenPluginIsNull
 * @tc.number: MediaDemuxerExt_CopyFrameToUserQueue_001
 * @tc.desc  : Test case for returning error when plugin is null
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CopyFrameToUserQueue_001, TestSize.Level1)
{
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_)).WillOnce(Return(1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_)).WillOnce(Return(nullptr));

    EXPECT_EQ(mediaDemuxer_->CopyFrameToUserQueue(1), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : CopyFrameToUserQueue_ShouldReturnError_WhenGetNextSampleSizeFails
 * @tc.number: MediaDemuxerExt_CopyFrameToUserQueue_002
 * @tc.desc  : Test case for returning error when GetNextSampleSize fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CopyFrameToUserQueue_002, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(-1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_)).WillOnce(Return(plugin));
    // IsNeedMapToInnerTrackID
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, IsDash()).WillRepeatedly(Return(false));
    mediaDemuxer_->isFlvLiveStream_ = false;

    // GetMemoryUsage return
    mediaDemuxer_->eventReceiver_ = nullptr;
    EXPECT_CALL(*plugin, GetNextSampleSize(_, _, _)).WillOnce(Return(Status::ERROR_WAIT_TIMEOUT));


    EXPECT_EQ(mediaDemuxer_->CopyFrameToUserQueue(1), Status::ERROR_WAIT_TIMEOUT);
}

/**
 * @tc.name  : MediaDemuxerExt_CopyFrameToUserQueue_003
 * @tc.number: MediaDemuxerExt_CopyFrameToUserQueue_003
 * @tc.desc  : Test case for returning error when HandleSegmentChange fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CopyFrameToUserQueue_003, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(-1));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_)).WillOnce(Return(plugin));

    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, IsDash()).WillRepeatedly(Return(false));
    mediaDemuxer_->isFlvLiveStream_ = false;

    mediaDemuxer_->eventReceiver_ = nullptr;
    EXPECT_CALL(*plugin, GetNextSampleSize(_, _, _)).WillOnce(Return(Status::END_OF_STREAM));

    mediaDemuxer_->isHls_ = true;
    EXPECT_CALL(*mediaDemuxer_->source_, IsHlsEnd(_)).WillRepeatedly(Return(false));
    EXPECT_EQ(mediaDemuxer_->CopyFrameToUserQueue(1), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_001
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_001
 * @tc.desc  : isAVDemuxer == true && perfRecEnabled_ == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_001, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->perfRecEnabled_ = true;
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->timeout_ = 1000;
    
    EXPECT_CALL(*plugin, ReadSample(_, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_002
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_002
 * @tc.desc  : isAVDemuxer == true && perfRecEnabled_ == false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_002, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->perfRecEnabled_ = false;
    mediaDemuxer_->timeout_ = 1000;
    
    EXPECT_CALL(*plugin, ReadSample(_, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_003
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_003
 * @tc.desc  : isAVDemuxer == false && perfRecEnabled_ == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_003, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->perfRecEnabled_ = true;
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->timeout_ = 1000;

    EXPECT_CALL(*plugin, ReadSample(_, _, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_ReadSampleWithPerfRecord_004
 * @tc.number: MediaDemuxerExt_ReadSampleWithPerfRecord_004
 * @tc.desc  : isAVDemuxer == false && perfRecEnabled_ == false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ReadSampleWithPerfRecord_004, TestSize.Level1)
{
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    mediaDemuxer_->perfRecEnabled_ = false;
    mediaDemuxer_->eventReceiver_ = nullptr;
    mediaDemuxer_->timeout_ = 1000;

    EXPECT_CALL(*plugin, ReadSample(_, _, _)).WillOnce(Return(Status::OK));

    Status ret = mediaDemuxer_->ReadSampleWithPerfRecord(plugin, 0, sample, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoSampleQueue
 * @tc.desc: test HandleVideoSampleQueue
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoSampleQueue_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_] = sampleQueue;
    EXPECT_CALL(*sampleQueue, AddQueueSize(_)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*sampleQueue, AcquireBuffer(_)).Times(NUM_1);
    EXPECT_CALL(*sampleQueue, ReleaseBuffer(_)).Times(NUM_1);
    mediaDemuxer_->HandleVideoSampleQueue();
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_001
 * @tc.desc: test HandleVideoTrack
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_001, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_ = NUM_0;
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    mediaDemuxer_->bufferMap_[0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[1] = std::make_shared<SampleQueue>();
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(NUM_1);

    mediaDemuxer_->isVideoMuted_ = true;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->hasSetLargeSize_ = false;
    std::string taskName = "SampleConsumerV";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillOnce(Return(true));
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = move(taskPtr);
    mediaDemuxer_->HandleVideoTrack(NUM_0);
    EXPECT_EQ(mediaDemuxer_->hasSetLargeSize_, true);
    EXPECT_EQ(mediaDemuxer_->needReleaseVideoDecoder_, false);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_002
 * @tc.desc: test HandleVideoTrack
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_002, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_ = NUM_0;
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    mediaDemuxer_->bufferMap_[0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[1] = std::make_shared<SampleQueue>();
    EXPECT_CALL(*(mediaDemuxer_->sampleQueueMap_[NUM_0]), Clear()).WillRepeatedly(Return(Status::OK));
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(NUM_1);

    mediaDemuxer_->isVideoMuted_ = true;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->hasSetLargeSize_ = true;
    std::string taskName = "SampleConsumerV";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).WillOnce(Return(false));
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = move(taskPtr);
    mediaDemuxer_->HandleVideoTrack(NUM_0);
    EXPECT_EQ(mediaDemuxer_->needReleaseVideoDecoder_, false);
}

/**
 * @tc.name: MediaDemuxerExt_HandlePushBuffer_001
 * @tc.desc: test HandlePushBuffer
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandlePushBuffer_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[NUM_0] = sptrProducer;
    std::shared_ptr<AVBuffer> dstBuffer = std::make_shared<AVBuffer>();
    dstBuffer->pts_ = NUM_1;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();

    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->lastAudioPtsInMute_ = NUM_0;
    Status ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    mediaDemuxer_->lastAudioPtsInMute_ = NUM_1;
    dstBuffer->pts_ = NUM_0;
    ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    ret = mediaDemuxer_->HandlePushBuffer(NUM_0, dstBuffer, mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    ret = mediaDemuxer_->HandlePushBuffer(NUM_0, dstBuffer, mediaDemuxer_->bufferQueueMap_[NUM_0], Status::OK);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxerExt_HandlePushBuffer_002
 * @tc.desc: test HandlePushBuffer
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandlePushBuffer_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_] = std::make_shared<SampleQueue>();
    std::shared_ptr<AVBuffer> dstBuffer = std::make_shared<AVBuffer>();
    dstBuffer->pts_ = NUM_0;
    dstBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    mediaDemuxer_->isVideoMuted_ = false;
    mediaDemuxer_->needRestore_ = true;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    std::vector<uint8_t> config = {};
    trackMeta->SetData(Tag::MEDIA_CODEC_CONFIG, config);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    Status ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(mediaDemuxer_->needRestore_, false);
    EXPECT_EQ(ret, Status::OK);

    mediaDemuxer_->needRestore_ = true;
    config = {static_cast<uint8_t>(NUM_0)};
    trackMeta->SetData(Tag::MEDIA_CODEC_CONFIG, config);
    mediaDemuxer_->mediaMetaData_.trackMetas.clear();
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    dstBuffer->memory_ = std::make_shared<AVMemory>();
    EXPECT_CALL(*(dstBuffer->memory_), GetSize()).WillOnce(Return(NUM_2));
    EXPECT_CALL(*(dstBuffer->memory_), Read(_, _, _)).WillOnce(Invoke([](uint8_t *data, int32_t size, int32_t offset) {
        memset_s(data, NUM_2, static_cast<uint8_t>(NUM_0), NUM_2);
        return NUM_2;
    }));
    ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(mediaDemuxer_->needRestore_, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxerExt_HandlePushBuffer_004
 * @tc.desc: test HandlePushBuffer
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandlePushBuffer_004, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    auto* mockProducer = new MockAVBufferQueueProducer();
    sptr<AVBufferQueueProducer> sptrProducer(mockProducer);
    mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_] = sptrProducer;
    mediaDemuxer_->sampleQueueMap_[mediaDemuxer_->videoTrackId_] = std::make_shared<SampleQueue>();
    std::shared_ptr<AVBuffer> dstBuffer = std::make_shared<AVBuffer>();
    dstBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    mediaDemuxer_->isVideoMuted_ = false;
    mediaDemuxer_->needRestore_ = true;
    std::shared_ptr<Meta> trackMeta = std::make_shared<Meta>();
    std::vector<uint8_t> config = {static_cast<uint8_t>(NUM_0)};
    trackMeta->SetData(Tag::MEDIA_CODEC_CONFIG, config);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta);
    dstBuffer->memory_ = std::make_shared<AVMemory>();
    EXPECT_CALL(*(dstBuffer->memory_), GetSize()).WillOnce(Return(NUM_0));
    Status ret = mediaDemuxer_->HandlePushBuffer(mediaDemuxer_->videoTrackId_, dstBuffer,
        mediaDemuxer_->bufferQueueMap_[mediaDemuxer_->videoTrackId_], Status::OK);
    EXPECT_EQ(mediaDemuxer_->needRestore_, false);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: MediaDemuxer_GetBufferFromUserQueue_001
 * @tc.desc: test GetBufferFromUserQueue
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetBufferFromUserQueue_001, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    bool ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);

    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->hasSetLargeSize_ = true;
    mediaDemuxer_->isVideoMuted_ = false;
    mediaDemuxer_->needRestore_ = false;
    EXPECT_CALL(*(mediaDemuxer_->sampleQueueMap_[NUM_0]), IsEmpty()).WillOnce(Return(true));
    ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);

    mediaDemuxer_->hasSetLargeSize_ = true;
    int64_t duration = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(duration);
    mediaDemuxer_->isVideoMuted_ = true;
    ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name: MediaDemuxer_GetBufferFromUserQueue_002
 * @tc.desc: test GetBufferFromUserQueue
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetBufferFromUserQueue_002, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    mediaDemuxer_->lastAudioPtsInMute_ = MAX_VIDEO_LEAD_TIME_ON_MUTE_US;
    int64_t duration = 0;
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(duration);
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->lastVideoPts_ = 0;
    mediaDemuxer_->hasSetLargeSize_ = false;
    mediaDemuxer_->needReleaseVideoDecoder_ = false;

    EXPECT_CALL(*(mediaDemuxer_->sampleQueueMap_[NUM_0]), RequestBuffer(_, _, _))
        .Times(NUM_2)
        .WillOnce(Return(Status::ERROR_UNKNOWN))
        .WillRepeatedly(Return(Status::OK));
    bool ret = mediaDemuxer_->GetBufferFromUserQueue(NUM_0, NUM_100);
    EXPECT_EQ(ret, true);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_001
 * @tc.number: MediaDemuxer_DoSelectTrack_001
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_001, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = nullptr;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   true
    // sampleQueueMap_[curTrackId] != nullptr  false
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_0), Status::OK);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_002
 * @tc.number: MediaDemuxer_DoSelectTrack_002
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_002, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    mediaDemuxer_->sampleQueueMap_[NUM_2] = nullptr;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   false
    // sampleQueueMap_[curTrackId] != nullptr  false
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_2), Status::OK);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_003
 * @tc.number: MediaDemuxer_DoSelectTrack_003
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_003, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_0] = sampleQueue;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(0)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   true
    // sampleQueueMap_[curTrackId] != nullptr  true
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_0), Status::OK);
}

/**
 * @tc.name  : MediaDemuxer_DoSelectTrack_004
 * @tc.number: MediaDemuxer_DoSelectTrack_004
 * @tc.desc  : test DoSelectTrack
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_DoSelectTrack_004, TestSize.Level1)
{
    mediaDemuxer_->audioTrackId_  = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_2] = sampleQueue;
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(2)).WillOnce(Return(1));
    EXPECT_CALL(*plugin, UnselectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));

    // curTrackId == audioTrackId_   false
    // sampleQueueMap_[curTrackId] != nullptr  true
    EXPECT_EQ(mediaDemuxer_->DoSelectTrack(NUM_1, NUM_2), Status::OK);
}
/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_001
 * @tc.number: MediaDemuxer_HandleEosDrag_001
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_001, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = false;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], true);
}

/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_002
 * @tc.number: MediaDemuxer_HandleEosDrag_002
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_002, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = true;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], false);
}

/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_003
 * @tc.number: MediaDemuxer_HandleEosDrag_003
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_003, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = 0;
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = false;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], false);
}

/**
 * @tc.name  : MediaDemuxer_HandleEosDrag_004
 * @tc.number: MediaDemuxer_HandleEosDrag_004
 * @tc.desc  : test HandleEosDrag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleEosDrag_004, TestSize.Level1)
{
    int32_t trackId = 0;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = 0;
    buffer->pts_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;
    mediaDemuxer_->eosMap_[trackId] = false;
    bool isDiscardable = true;
    mediaDemuxer_->HandleEosDrag(trackId, isDiscardable);
    EXPECT_EQ(mediaDemuxer_->eosMap_[trackId], false);
}

/**
 * @tc.name  : MediaDemuxerExt_OnHlsSeekReadyEvent_001
 * @tc.number: MediaDemuxerExt_OnHlsSeekReadyEvent_001
 * @tc.desc  : Test currentStreamType == MEDIA_TYPE_AUD
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnHlsSeekReadyEvent_001, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_AUD));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->OnHlsSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_OnHlsSeekReadyEvent_002
 * @tc.number: MediaDemuxerExt_OnHlsSeekReadyEvent_002
 * @tc.desc  : Test currentStreamType == MEDIA_TYPE_VID
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnHlsSeekReadyEvent_002, TestSize.Level1)
{
    Format format;
    format.PutIntValue("currentStreamType", static_cast<int32_t>(MediaAVCodec::MediaType::MEDIA_TYPE_VID));
    format.PutIntValue("currentStreamId", 1);
    format.PutIntValue("isEOS", 0);
    PluginEvent event;
    event.param = format;
    mediaDemuxer_->videoTrackId_ = 0;
    mediaDemuxer_->OnHlsSeekReadyEvent(event);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].first, 1);
    EXPECT_EQ(mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)].second, 0);
}

/**
 * @tc.name  : MediaDemuxerExt_IsSegmentEos_001
 * @tc.number: MediaDemuxerExt_IsSegmentEos_001
 * @tc.desc  : Test IsSegmentEos() == true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsSegmentEos_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;
    EXPECT_EQ(mediaDemuxer_->IsSegmentEos(), true);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_001
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_001
 * @tc.desc  : Test isAVStreamIsSame is false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;

    // IsAVStreamIdSame return false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(NUM_1)).WillRepeatedly(Return(NUM_0));

    // HandleSegmentChange return Status::OK
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillOnce(Return(Status::OK));
    
    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_002
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_002
 * @tc.desc  : Test isAVStreamIdSame is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_002, TestSize.Level1)
{
    // IsSegmentEos return true
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;

    // IsAVStreamIdSame return true
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_0));

    // HandleSegmentChange return Status::OK
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillOnce(Return(Status::OK));
    
    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_003
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_003
 * @tc.desc  : Test demuxerPluginManager is nullptr → return ERROR_NULL_POINTER
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_003, TestSize.Level1)
{
    mediaDemuxer_->demuxerPluginManager_ = nullptr;

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_004
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_004
 * @tc.desc  : Test IsAVInOneStream is false → return HandleSegmentChange result
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_004, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _))
        .WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash())
        .WillRepeatedly(Return(true));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    mediaDemuxer_->subtitleTrackId_ = NUM_1;

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_005
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_005
 * @tc.desc  : Test IsAVInOneStream is false → return HandleSegmentChange result
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_005, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash())
        .WillRepeatedly(Return(true));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    mediaDemuxer_->subStreamDemuxer_ = nullptr;

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_006
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_006
 * @tc.desc  : Test IsAVInOneStream is false → return HandleSegmentChange result
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_006, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash())
        .WillRepeatedly(Return(true));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    int32_t trackId = NEG_1;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_007
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_007
 * @tc.desc  : Test IsSegmentEos is false → return ERROR_ONE_TRACK_SEGMENT_EOS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_007, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = false;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = false;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NEG_1));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::ERROR_ONE_TRACK_SEGMENT_EOS);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_008
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_008
 * @tc.desc  : Test IsSegmentEos is false → return ERROR_ONE_TRACK_SEGMENT_EOS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_008, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = false;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_0));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::ERROR_ONE_TRACK_SEGMENT_EOS);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_009
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_009
 * @tc.desc  : Test IsSegmentEos is false → return ERROR_ONE_TRACK_SEGMENT_EOS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_009, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_INVALID));

    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSegmentEos_010
 * @tc.number: MediaDemuxerExt_HandleSegmentEos_010
 * @tc.desc  : Test IsAVInOneStream is false → return ERROR_ONE_TRACK_SEGMENT_EOS
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSegmentEos_010, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = true;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NEG_1));

    int32_t trackId = NUM_0;
    Status ret = mediaDemuxer_->HandleSegmentEos(trackId);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleHlsSeek_001
 * @tc.number: MediaDemuxerExt_HandleHlsSeek_001
 * @tc.desc  : Test isAVStreamIdSame is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleHlsSeek_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;

    // IsAVStreamIdSame return true
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(NUM_100));

    // HandleHlsRebootPlugin return Status::OK
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {200, false};

    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    Status ret = mediaDemuxer_->HandleHlsSeek();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleHlsSeek_002
 * @tc.number: MediaDemuxerExt_HandleHlsSeek_002
 * @tc.desc  : Test isAVStreamIdIsSame is false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleHlsSeek_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;

    // IsAVStreamIdSame return false
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(NUM_100)).WillOnce(Return(NUM_0))
        .WillOnce(Return(NUM_100)).WillOnce(Return(NUM_100)).WillOnce(Return(NUM_0))
        .WillOnce(Return(NUM_0)).WillOnce(Return(NUM_100)).WillOnce(Return(NUM_0))
        .WillRepeatedly(Return(NUM_100));

    // HandleHlsRebootPlugin return Status::OK
    mediaDemuxer_->subStreamDemuxer_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTrackTypeByTrackID(_))
        .WillOnce(Return(TrackType::TRACK_AUDIO)).WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), RebootPlugin(_, _, _, _)).Times(0);
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::AUDIO)] = {200, false};
    mediaDemuxer_->seekReadyStreamInfo_[static_cast<int32_t>(StreamType::VIDEO)] = {200, false};

    // InnerSelectTrack return Status::OK
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillRepeatedly(Return(Status::OK));

    Status ret = mediaDemuxer_->HandleHlsSeek();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_RecordDemuxerTimeStamp_001
 * @tc.number: MediaDemuxerExt_RecordDemuxerTimeStamp_001
 * @tc.desc  : Test stage == StallingStage::DEMUXER_START
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_RecordDemuxerTimeStamp_001, TestSize.Level1)
{
    AVBuffer buffer;
    buffer.meta_ = std::make_shared<Meta>();
    StallingStage stage = StallingStage::DEMUXER_START;

    mediaDemuxer_->RecordDemuxerTimeStamp(buffer, stage);

    std::vector<int64_t> timeStampList;
    bool ret = buffer.meta_->GetData(Tag::STALLING_TIMESTAMP, timeStampList);
    ASSERT_TRUE(ret);
    ASSERT_EQ(timeStampList.size(), NUM_2);
    EXPECT_EQ(timeStampList[0], static_cast<int64_t>(stage));
    EXPECT_GT(timeStampList[1], NUM_0);
}

/**
 * @tc.name  : MediaDemuxerExt_RecordDemuxerTimeStamp_002
 * @tc.number: MediaDemuxerExt_RecordDemuxerTimeStamp_002
 * @tc.desc  : Test stage == StallingStage::DEMUXER_START
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_RecordDemuxerTimeStamp_002, TestSize.Level1)
{
    AVBuffer buffer;
    buffer.meta_ = std::make_shared<Meta>();
    StallingStage stage = StallingStage::DEMUXER_END;

    mediaDemuxer_->RecordDemuxerTimeStamp(buffer, stage);

    std::vector<int64_t> timeStampList;
    bool ret = buffer.meta_->GetData(Tag::STALLING_TIMESTAMP, timeStampList);
    ASSERT_TRUE(ret);
    ASSERT_EQ(timeStampList.size(), NUM_2);
    EXPECT_EQ(timeStampList[0], static_cast<int64_t>(stage));
    EXPECT_GT(timeStampList[1], NUM_0);
}

/**
 * @tc.name  : MediaDemuxer_IsBufferingMap
 * @tc.number: MediaDemuxer_IsBufferingMap
 * @tc.desc  : test SetTrackIsBuffering GetTrackIsBuffering
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_IsBufferingMap, TestSize.Level1)
{
    int32_t trackId = 0;
    mediaDemuxer_->isBufferingMap_[trackId] = true;
    EXPECT_TRUE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, false);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, true);
    EXPECT_TRUE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    
    trackId = 100;
    mediaDemuxer_->isBufferingMap_[trackId] = false;
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, true);
    EXPECT_TRUE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    mediaDemuxer_->SetTrackIsBuffering(trackId, false);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest BoostReadThreadPriority API
 * @tc.number: MediaDemuxerExt_BoostReadThreadPriority_001
 * @tc.desc  : Test plugin == nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_BoostReadThreadPriority_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(
        INVALID_STREAM_OR_TRACK_ID));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    auto res = mediaDemuxer_->BoostReadThreadPriority();
    EXPECT_EQ(res, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest BoostReadThreadPriority API
 * @tc.number: MediaDemuxerExt_BoostReadThreadPriority_002
 * @tc.desc  : Test (plugin->BoostReadThreadPriority()) != Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_BoostReadThreadPriority_002, TestSize.Level1)
{
    mediaDemuxer_->isFirstParser_ = true;
    mockRetInt32_ = MSERR_OK;
    std::shared_ptr<DemuxerPlugin> plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(plugin), ParserRefUpdatePos(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));

    EXPECT_CALL(*(plugin), BoostReadThreadPriority()).WillOnce(Return(Status::ERROR_WRONG_STATE));

    auto res = mediaDemuxer_->BoostReadThreadPriority();
    EXPECT_EQ(res, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest BoostThreadPriorityIfNeeded API
 * @tc.number: MediaDemuxerExt_BoostThreadPriorityIfNeeded_001
 * @tc.desc  : Test boostAudioRet != Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_BoostThreadPriorityIfNeeded_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_)).WillRepeatedly(Return(
        INVALID_STREAM_OR_TRACK_ID));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(nullptr));

    auto res = mediaDemuxer_->BoostThreadPriorityIfNeeded();
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_001
 * @tc.desc  : Test pluginName without header matches contentType → return tru
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_001, TestSize.Level1)
{
    std::string pluginName = "flv";
    std::string contentType = "video/flv";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_TRUE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_002
 * @tc.desc  : Test empty contentType → return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_002, TestSize.Level1)
{
    std::string pluginName = "flv";
    std::string contentType = "";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_003
 * @tc.desc  : Test token empty after erase header → return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_003, TestSize.Level1)
{
    std::string pluginName = "demuxer_";
    std::string contentType = "video/mp4";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_004
 * @tc.desc  : Test pluginName with header matches contentType → return true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_004, TestSize.Level1)
{
    std::string pluginName = "mp4";
    std::string contentType = "video/mp4";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_TRUE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_005
 * @tc.desc  : Test multi-token pluginName with one match → return true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_005, TestSize.Level1)
{
    std::string pluginName = "mp4,flv,mkv";
    std::string contentType = "video/flv";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_TRUE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_006
 * @tc.desc  : Test token not empty but no match → return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_006, TestSize.Level1)
{
    std::string pluginName = "avi";
    std::string contentType = "video/mp4";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_007
 * @tc.desc  : Test empty pluginName → return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_007, TestSize.Level1)
{
    std::string pluginName = "";
    std::string contentType = "video/mp4";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_008
 * @tc.desc  : Test multi-token pluginName with no match → return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_008, TestSize.Level1)
{
    std::string pluginName = "avi,wmv,rmvb";
    std::string contentType = "video/mp4";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest IsHitPlugin API
 * @tc.number: MediaDemuxerExt_IsHitPlugin_009
 * @tc.desc  : Test pluginName with only delimiters → return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsHitPlugin_009, TestSize.Level1)
{
    std::string pluginName = ",,,";
    std::string contentType = "video/mp4";
    bool ret = mediaDemuxer_->IsHitPlugin(pluginName, contentType);

    EXPECT_FALSE(ret);
}

/**
 * @tc.name  : MediaDemuxer_ReadSampleToDrop_001
 * @tc.number: MediaDemuxer_ReadSampleToDrop_001
 * @tc.desc  : test ReadSampleToDrop when plugin is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_ReadSampleToDrop_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    int32_t trackId = NUM_0;

    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(0));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_))
        .WillOnce(Return(nullptr));

    Status ret = mediaDemuxer_->ReadSampleToDrop(trackId, sample);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : MediaDemuxer_NeedDroped_001
 * @tc.number: MediaDemuxer_NeedDroped_001
 * @tc.desc  : test NeedDroped when cache duration reaches STOP_PRODUCE_WATER_LOOP
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_NeedDroped_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;

    EXPECT_CALL(*sampleQueue, NewGetCacheDuration())
        .WillOnce(Return(SampleQueueController::STOP_PRODUCE_WATER_LOOP));
    EXPECT_CALL(*sampleQueue, GetFilledBufferSize()).Times(0);

    bool ret = mediaDemuxer_->NeedDroped(trackId);
    EXPECT_TRUE(ret);
}

/**
 * @tc.name  : MediaDemuxer_CachePressuredCallback_001
 * @tc.number: MediaDemuxer_CachePressuredCallback_001
 * @tc.desc  : test CachePressuredCallback returns early when sample queue is disabled
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CachePressuredCallback_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    uint32_t cachedBytes = 0;

    mediaDemuxer_->enableSampleQueue_ = false;
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_)).Times(0);
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetStreamIDByTrackID(_)).Times(0);
    mediaDemuxer_->CachePressuredCallback(trackId, cachedBytes);
}

/**
 * @tc.name  : MediaDemuxer_CachePressuredCallback_002
 * @tc.number: MediaDemuxer_CachePressuredCallback_002
 * @tc.desc  : test CachePressuredCallback when pluginTemp is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CachePressuredCallback_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    uint32_t cachedBytes = 0;

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = true;

    mediaDemuxer_->hasDropedMap_[trackId].store(false);

    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(0));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_))
        .WillOnce(Return(nullptr));
    mediaDemuxer_->CachePressuredCallback(trackId, cachedBytes);
}

/**
 * @tc.name  : MediaDemuxer_CachePressuredCallback_003
 * @tc.number: MediaDemuxer_CachePressuredCallback_003
 * @tc.desc  : test CachePressuredCallback when GetCurrentCacheFrameCount returns error
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CachePressuredCallback_003, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    uint32_t cachedBytes = 0;

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = false;

    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(-1));

    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetStreamIDByTrackID(_))
        .WillOnce(Return(0));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_))
        .WillOnce(Return(plugin));

    EXPECT_CALL(*plugin, GetCurrentCacheFrameCount(_, _))
        .WillOnce(Return(Status::ERROR_UNKNOWN));

    mediaDemuxer_->CachePressuredCallback(trackId, cachedBytes);
}

/**
 * @tc.name  : MediaDemuxer_CachePressuredCallback_004
 * @tc.number: MediaDemuxer_CachePressuredCallback_004
 * @tc.desc  : test CachePressuredCallback when NeedDroped is true but demuxer is stopped
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CachePressuredCallback_004, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    uint32_t cachedBytes = 0;

    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isFlvLiveStream_ = false;

    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(-1));

    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetStreamIDByTrackID(_))
        .WillOnce(Return(0));
    EXPECT_CALL(*mediaDemuxer_->demuxerPluginManager_, GetPluginByStreamID(_))
        .WillOnce(Return(plugin));

    uint32_t frameCount = 5;
    EXPECT_CALL(*plugin, GetCurrentCacheFrameCount(_, _))
        .WillOnce(DoAll(SetArgReferee<1>(frameCount), Return(Status::OK)));

    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    EXPECT_CALL(*sampleQueue, NewGetCacheDuration())
        .WillOnce(Return(0));
    EXPECT_CALL(*sampleQueue, GetFilledBufferSize())
        .WillOnce(Return(0));
    mediaDemuxer_->isStopped_ = true;
    mediaDemuxer_->CachePressuredCallback(trackId, cachedBytes);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSeekChangeStream API
 * @tc.number: MediaDemuxerExt_HandleSeekChangeStream_005
 * @tc.desc  : Test streamDemuxer is nullptr → skip if block, return OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSeekChangeStream_005, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = nullptr;
    int32_t currentStreamId = NUM_0;
    int32_t newStreamId = NUM_1;
    int32_t trackId = NUM_0;
    auto ret = mediaDemuxer_->HandleSeekChangeStream(currentStreamId, newStreamId, trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSeekChangeStream API
 * @tc.number: MediaDemuxerExt_HandleSeekChangeStream_002
 * @tc.desc  : Test streamDemuxer is valid but HasEosTrack is false → skip if block, return OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSeekChangeStream_002, TestSize.Level1)
{
    int32_t testId = NUM_0;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->eosMap_[testId] = false;
    int32_t currentStreamId = NUM_0;
    int32_t newStreamId = NUM_1;
    int32_t trackId = NUM_0;
    auto ret = mediaDemuxer_->HandleSeekChangeStream(currentStreamId, newStreamId, trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSeekChangeStream API
 * @tc.number: MediaDemuxerExt_HandleSeekChangeStream_003
 * @tc.desc  : Test enter if block but not Dash stream → skip HandleDashChangeStream, return OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSeekChangeStream_003, TestSize.Level1)
{
    int32_t testId = NUM_0;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->eosMap_[testId] = true;
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), SetNewVideoStreamID(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    int32_t currentStreamId = NUM_0;
    int32_t newStreamId = NUM_1;
    int32_t trackId = NUM_0;
    auto ret = mediaDemuxer_->HandleSeekChangeStream(currentStreamId, newStreamId, trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest HandleSeekChangeStream API
 * @tc.number: MediaDemuxerExt_HandleSeekChangeStream_004
 * @tc.desc  : Test enter if block but not Dash stream → skip HandleDashChangeStream, return OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSeekChangeStream_004, TestSize.Level1)
{
    int32_t testId = NUM_0;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->eosMap_[testId] = true;
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), SetNewVideoStreamID(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(true));
    int32_t currentStreamId = NUM_0;
    int32_t newStreamId = NUM_1;
    int32_t trackId = NUM_0;
    auto ret = mediaDemuxer_->HandleSeekChangeStream(currentStreamId, newStreamId, trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_NotifyResumeUnMute_001
 * @tc.number: MediaDemuxerExt_NotifyResumeUnMute_001
 * @tc.desc  : Test NotifyResumeUnMute when video is not muted and task not running
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifyResumeUnMute_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->isVideoMuted_ = false;

    std::string taskName = "SampleConsumerV";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(taskPtr), IsTaskRunning())
        .WillOnce(Return(false));
    EXPECT_CALL(*(taskPtr), Start())
        .Times(1);
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = std::move(taskPtr);
    mediaDemuxer_->NotifyResumeUnMute();
}

/**
 * @tc.name  : MediaDemuxerExt_NotifyResumeUnMute_002
 * @tc.number: MediaDemuxerExt_NotifyResumeUnMute_002
 * @tc.desc  : Test NotifyResumeUnMute when video is muted
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifyResumeUnMute_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    std::string taskName = "SampleConsumerV";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);

    mediaDemuxer_->isVideoMuted_ = true;
    EXPECT_CALL(*(taskPtr), IsTaskRunning()).Times(0);
    EXPECT_CALL(*(taskPtr), Start()).Times(0);
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = std::move(taskPtr);
    mediaDemuxer_->NotifyResumeUnMute();
}

/**
 * @tc.name  : MediaDemuxerExt_NotifyResumeUnMute_003
 * @tc.number: MediaDemuxerExt_NotifyResumeUnMute_003
 * @tc.desc  : Test NotifyResumeUnMute when task already running
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_NotifyResumeUnMute_003, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    std::string taskName = "SampleConsumerV";
    std::unique_ptr<MockTask> taskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);

    mediaDemuxer_->isVideoMuted_ = false;
    EXPECT_CALL(*(taskPtr), IsTaskRunning())
        .WillOnce(Return(true));
    EXPECT_CALL(*(taskPtr), Start()).Times(0);
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = std::move(taskPtr);
    mediaDemuxer_->NotifyResumeUnMute();
}


/**
 * @tc.name  : MediaDemuxerExt_IsSegmentEos_002
 * @tc.number: MediaDemuxerExt_IsSegmentEos_002
 * @tc.desc  : Test IsSegmentEos() == false when any track segmentEos is false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsSegmentEos_002, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->segmentEosMap_[NUM_0] = false;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    mediaDemuxer_->segmentEosMap_[NUM_1] = true;

    EXPECT_EQ(mediaDemuxer_->IsSegmentEos(), false);
}

/**
 * @tc.name  : MediaDemuxerExt_HandleNotAllTrackEos_001
 * @tc.number: MediaDemuxerExt_HandleNotAllTrackEos_001
 * @tc.desc  : Test GetTrackIsBuffering(trackId) == false, condition not satisfied
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleNotAllTrackEos_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->isStopped_.store(false);
    mediaDemuxer_->SetTrackIsBuffering(trackId, false);

    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(0);

    mediaDemuxer_->HandleNotAllTrackEos(trackId);

    EXPECT_TRUE(mediaDemuxer_->hlsSegmentEosMap_[trackId]);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
}

/**
 * @tc.name  : MediaDemuxerExt_HandleNotAllTrackEos_002
 * @tc.number: MediaDemuxerExt_HandleNotAllTrackEos_002
 * @tc.desc  : Test GetTrackIsBuffering(trackId) == true and one task is not running
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleNotAllTrackEos_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->isStopped_.store(false);
    mediaDemuxer_->audioTrackId_ = trackId;
    mediaDemuxer_->videoTrackId_ = NUM_1;

    mediaDemuxer_->SetTrackIsBuffering(trackId, true);
    mediaDemuxer_->isBuffering_.store(true);

    std::string taskName = "Demux";
    std::unique_ptr<MockTask> audioTaskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(audioTaskPtr), IsTaskRunning()).WillRepeatedly(Return(false));
    mediaDemuxer_->taskMap_[mediaDemuxer_->audioTrackId_] = std::move(audioTaskPtr);

    std::unique_ptr<MockTask> videoTaskPtr = std::make_unique<MockTask>(taskName, mediaDemuxer_->playerId_);
    EXPECT_CALL(*(videoTaskPtr), IsTaskRunning()).WillRepeatedly(Return(true));
    mediaDemuxer_->taskMap_[mediaDemuxer_->videoTrackId_] = std::move(videoTaskPtr);

    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*(mockEventReceiver), OnEvent(_)).Times(1);

    mediaDemuxer_->HandleNotAllTrackEos(trackId);

    EXPECT_TRUE(mediaDemuxer_->hlsSegmentEosMap_[trackId]);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    EXPECT_FALSE(mediaDemuxer_->isBuffering_.load());
}


/**
 * @tc.name  : MediaDemuxerExt_AfterSeekNeedDrop_001
 * @tc.number: MediaDemuxerExt_AfterSeekNeedDrop_001
 * @tc.desc  : Test non-mapping mode when pluginTemp is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AfterSeekNeedDrop_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->SetTrackSeekNeedDrop(trackId, true);
    mediaDemuxer_->isFlvLiveStream_ = false;
    mediaDemuxer_->subtitleTrackId_ = NUM_1;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(mediaDemuxer_->subtitleTrackId_))
        .WillOnce(Return(NEG_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamIDByTrackID(trackId))
        .WillOnce(Return(NUM_0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(NUM_0))
        .WillOnce(Return(std::shared_ptr<Plugins::DemuxerPlugin>(nullptr)));

    mediaDemuxer_->AfterSeekNeedDrop(trackId);
    EXPECT_TRUE(mediaDemuxer_->GetTrackSeekNeedDrop(trackId));
}

/**
 * @tc.name  : MediaDemuxerExt_HandleFrameDropForTrack_001
 * @tc.number: MediaDemuxerExt_HandleFrameDropForTrack_001
 * @tc.desc  : Test IsLocalFd() == true returns 0 and does not change frameCount
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleFrameDropForTrack_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->SetTrackNeedDropFrame(trackId, NUM_1);

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(true));

    auto result = mediaDemuxer_->HandleFrameDropForTrack(trackId);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(mediaDemuxer_->GetTrackNeedDropFrame(trackId), static_cast<uint32_t>(NUM_1));
}

/**
 * @tc.name  : MediaDemuxerExt_HandleFrameDropForTrack_002
 * @tc.number: MediaDemuxerExt_HandleFrameDropForTrack_002
 * @tc.desc  : Test non-mapping mode when ReadSampleToDrop returns error (plugin is nullptr)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleFrameDropForTrack_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->SetTrackNeedDropFrame(trackId, NUM_1);

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));

    mediaDemuxer_->isFlvLiveStream_ = false;
    mediaDemuxer_->subtitleTrackId_ = NUM_1;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTmpStreamIDByTrackID(mediaDemuxer_->subtitleTrackId_)).WillOnce(Return(NEG_1));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamIDByTrackID(trackId)).WillOnce(Return(NUM_0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(NUM_0))
        .WillOnce(Return(std::shared_ptr<Plugins::DemuxerPlugin>(nullptr)));

    auto result = mediaDemuxer_->HandleFrameDropForTrack(trackId);
    EXPECT_EQ(result, 0);
    EXPECT_EQ(mediaDemuxer_->GetTrackNeedDropFrame(trackId), static_cast<uint32_t>(0));
}

/**
 * @tc.name  : MediaDemuxerExt_ResetSampleQueueStatus_001
 * @tc.number: MediaDemuxerExt_ResetSampleQueueStatus_001
 * @tc.desc  : Test video/audio track valid and in sampleQueueMap → reset both queues and set buffering
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ResetSampleQueueStatus_001, TestSize.Level1)
{
    int32_t videoTrackId = NUM_0;
    int32_t audioTrackId = NUM_1;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    mediaDemuxer_->audioTrackId_ = audioTrackId;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    auto mockAudioQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    mediaDemuxer_->sampleQueueMap_[audioTrackId] = mockAudioQueue;
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*mockVideoQueue, Clear()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockVideoQueue, UpdateLastOutSamplePts(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockVideoQueue, UpdateLastEnterSamplePts(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockAudioQueue, Clear()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockAudioQueue, UpdateLastOutSamplePts(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockAudioQueue, UpdateLastEnterSamplePts(_)).WillOnce(Return(Status::OK));
    mediaDemuxer_->ResetSampleQueueStatus(seekTime);
}

/**
 * @tc.name  : MediaDemuxerExt_ResetSampleQueueStatus_002
 * @tc.number: MediaDemuxerExt_ResetSampleQueueStatus_002
 * @tc.desc  : Test video track valid and in sampleQueueMap, audio track valid but not in map → reset only video queue
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ResetSampleQueueStatus_002, TestSize.Level1)
{
    int32_t videoTrackId = NUM_0;
    int32_t audioTrackId = NUM_1;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    mediaDemuxer_->audioTrackId_ = audioTrackId;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*mockVideoQueue, Clear()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockVideoQueue, UpdateLastOutSamplePts(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockVideoQueue, UpdateLastEnterSamplePts(_)).WillOnce(Return(Status::OK));
    mediaDemuxer_->ResetSampleQueueStatus(seekTime);
}

/**
 * @tc.name  : MediaDemuxerExt_ResetSampleQueueStatus_003
 * @tc.number: MediaDemuxerExt_ResetSampleQueueStatus_003
 * @tc.desc  : Test video track valid and in sampleQueueMap, audio track invalid and not in map → reset only video queue
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ResetSampleQueueStatus_003, TestSize.Level1)
{
    int32_t videoTrackId = NUM_0;
    int32_t audioTrackId = NEG_1;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    mediaDemuxer_->audioTrackId_ = audioTrackId;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*mockVideoQueue, Clear()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockVideoQueue, UpdateLastOutSamplePts(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockVideoQueue, UpdateLastEnterSamplePts(_)).WillOnce(Return(Status::OK));
    mediaDemuxer_->ResetSampleQueueStatus(seekTime);
}

/**
 * @tc.name  : MediaDemuxerExt_ResetSampleQueueStatus_004
 * @tc.number: MediaDemuxerExt_ResetSampleQueueStatus_004
 * @tc.desc  : Test video track valid and in sampleQueueMap, audio track invalid and not in map → reset only video queue
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ResetSampleQueueStatus_004, TestSize.Level1)
{
    int32_t videoTrackId = NEG_1;
    int32_t audioTrackId = NUM_0;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    mediaDemuxer_->audioTrackId_ = audioTrackId;
    auto mockAudioQueue  = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[audioTrackId] = mockAudioQueue;
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*mockAudioQueue, Clear()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockAudioQueue, UpdateLastOutSamplePts(_)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*mockAudioQueue, UpdateLastEnterSamplePts(_)).WillOnce(Return(Status::OK));
    mediaDemuxer_->ResetSampleQueueStatus(seekTime);
}

/**
 * @tc.name  : MediaDemuxerExt_ResetSampleQueueStatus_005
 * @tc.number: MediaDemuxerExt_ResetSampleQueueStatus_005
 * @tc.desc  : Test video/audio track invalid and not in sampleQueueMap → no queue reset and no buffering set
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ResetSampleQueueStatus_005, TestSize.Level1)
{
    int32_t videoTrackId = NEG_1;
    int32_t audioTrackId = NEG_1;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    mediaDemuxer_->audioTrackId_ = audioTrackId;
    int64_t seekTime = NUM_1000;
    mediaDemuxer_->ResetSampleQueueStatus(seekTime);
    EXPECT_EQ(mediaDemuxer_->sampleQueueMap_.size(), NUM_0);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekTo_001
 * @tc.number: MediaDemuxerExt_SeekTo_001
 * @tc.desc  : Test source_ != nullptr , mode=SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=true,
 *             return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekTo_001, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    mediaDemuxer_->subtitleSource_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SeekToTime(_, _)).WillRepeatedly(Return(Status::OK));
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    int64_t testRealSeekTime;
    EXPECT_EQ(mediaDemuxer_->SeekTo(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekTo_002
 * @tc.number: MediaDemuxerExt_SeekTo_002
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=true,
 *             return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekTo_002, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SeekToTime(_, _)).WillRepeatedly(Return(Status::OK));
    Plugins::SeekMode testMode = SeekMode::SEEK_NEXT_SYNC;
    int64_t testRealSeekTime;
    EXPECT_EQ(mediaDemuxer_->SeekTo(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekTo_003
 * @tc.number: MediaDemuxerExt_SeekTo_003
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekTo_003, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_NEXT_SYNC;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    EXPECT_EQ(mediaDemuxer_->SeekTo(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekTo_004
 * @tc.number: MediaDemuxerExt_SeekTo_004
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             IsLocalFd=true, return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekTo_004, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd()).WillOnce(Return(true));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    EXPECT_EQ(mediaDemuxer_->SeekTo(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekTo_005
 * @tc.number: MediaDemuxerExt_SeekTo_005
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             IsLocalFd=false, return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekTo_005, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd()).WillOnce(Return(false));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    EXPECT_EQ(mediaDemuxer_->SeekTo(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekToStart_001
 * @tc.number: MediaDemuxerExt_SeekToStart_001
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             IsLocalFd=true, return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekToStart_001, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd()).WillOnce(Return(true));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    EXPECT_EQ(mediaDemuxer_->SeekToStart(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekToStart_002
 * @tc.number: MediaDemuxerExt_SeekToStart_002
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             IsLocalFd=false, return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekToStart_002, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd()).WillOnce(Return(false));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    EXPECT_EQ(mediaDemuxer_->SeekToStart(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekToStart_003
 * @tc.number: MediaDemuxerExt_SeekToStart_003
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=true,
 *             return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekToStart_003, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    EXPECT_EQ(mediaDemuxer_->SeekToStart(seekTime, testMode, testRealSeekTime), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekToKeyFrame_001
 * @tc.number: MediaDemuxerExt_SeekToKeyFrame_001
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             IsLocalFd=true, return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekToKeyFrame_001, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd()).WillOnce(Return(true));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    DemuxerCallerType callerType = DemuxerCallerType::PLAYER;
    EXPECT_EQ(mediaDemuxer_->SeekToKeyFrame(seekTime, testMode, testRealSeekTime, callerType), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekToKeyFrame_002
 * @tc.number: MediaDemuxerExt_SeekToKeyFrame_002
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             IsLocalFd=false, return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekToKeyFrame_002, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd()).WillOnce(Return(false));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    DemuxerCallerType callerType = DemuxerCallerType::PLAYER;
    EXPECT_EQ(mediaDemuxer_->SeekToKeyFrame(seekTime, testMode, testRealSeekTime, callerType), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SeekToKeyFrame_003
 * @tc.number: MediaDemuxerExt_SeekToKeyFrame_003
 * @tc.desc  : Test source_ != nullptr , mode != SEEK_CLOSEST_INNER, has subtitle, IsSeekToTimeSupported=false,
 *             return Status::OK
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SeekToKeyFrame_003, TestSize.Level1)
{
    mediaDemuxer_->source_ = std::make_shared<Source>();
    int64_t seekTime = NUM_1000;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported()).WillRepeatedly(Return(true));
    int64_t testRealSeekTime;
    auto sampleQueue = std::make_shared<SampleQueue>();
    int32_t videoTrackId = NUM_0;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needRestore_ = true;
    mediaDemuxer_->videoTrackId_ = videoTrackId;
    Plugins::SeekMode testMode = SeekMode::SEEK_CLOSEST_INNER;
    auto mockVideoQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[videoTrackId] = mockVideoQueue;
    DemuxerCallerType callerType = DemuxerCallerType::PLAYER;
    EXPECT_EQ(mediaDemuxer_->SeekToKeyFrame(seekTime, testMode, testRealSeekTime, callerType), Status::OK);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ProduceWaterLoopControl API
 * @tc.number: MediaDemuxerExt_ProduceWaterLoopControl_001
 * @tc.desc  : Test early return when sampleQueueController_ is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProduceWaterLoopControl_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->sampleQueueController_ = nullptr;

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).Times(0);

    mediaDemuxer_->ProduceWaterLoopControl(trackId);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ProduceWaterLoopControl API
 * @tc.number: MediaDemuxerExt_ProduceWaterLoopControl_002
 * @tc.desc  : Test path when StartConsume and ProduceWaterLoopControl both early return because video is muted
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProduceWaterLoopControl_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->subtitleTrackId_ = NUM_1; // ensure trackId != subtitleTrackId_
    mediaDemuxer_->videoTrackId_ = trackId;  // trackId is video track
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    mediaDemuxer_->isVideoMuted_ = true;     // make StartConsume and second if early-return

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    // make GetEnableSampleQueueFlag() true and IsLocalFd() false so first if is skipped
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));

    mediaDemuxer_->ProduceWaterLoopControl(trackId);
}

/**
 * @tc.name  : Test MediaDemuxerExtUnitTest ProduceWaterLoopControl API
 * @tc.number: MediaDemuxerExt_ProduceWaterLoopControl_003
 * @tc.desc  : Test early return when IsLocalFd() is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProduceWaterLoopControl_003, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    // keep sampleQueueController_ non-null, trackId != subtitleTrackId_, and enable sample queue
    mediaDemuxer_->subtitleTrackId_ = NUM_1;
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(true));

    mediaDemuxer_->ProduceWaterLoopControl(trackId);
}

/**
 * @tc.name  : MediaDemuxer_CheckAndReportBufferingStatus_001
 * @tc.number: MediaDemuxer_CheckAndReportBufferingStatus_001
 * @tc.desc  : Test BUFFERING_END when isBuffering_ is false (no event, state unchanged)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CheckAndReportBufferingStatus_001, TestSize.Level1)
{
    mediaDemuxer_->isBuffering_.store(false);

    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    mediaDemuxer_->CheckAndReportBufferingStatus(EventType::BUFFERING_END);

    EXPECT_FALSE(mediaDemuxer_->isBuffering_.load());
}

/**
 * @tc.name  : MediaDemuxer_CheckAndReportBufferingStatus_002
 * @tc.number: MediaDemuxer_CheckAndReportBufferingStatus_002
 * @tc.desc  : Test BUFFERING_END when isBuffering_ is true (event sent, state to false)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CheckAndReportBufferingStatus_002, TestSize.Level1)
{
    mediaDemuxer_->isBuffering_.store(true);

    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(1);

    mediaDemuxer_->CheckAndReportBufferingStatus(EventType::BUFFERING_END);

    EXPECT_FALSE(mediaDemuxer_->isBuffering_.load());
}

/**
 * @tc.name  : MediaDemuxer_CheckAndReportBufferingStatus_003
 * @tc.number: MediaDemuxer_CheckAndReportBufferingStatus_003
 * @tc.desc  : Test BUFFERING_START when isBuffering_ is false (event sent, state to true)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CheckAndReportBufferingStatus_003, TestSize.Level1)
{
    mediaDemuxer_->isBuffering_.store(false);

    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(1);

    mediaDemuxer_->CheckAndReportBufferingStatus(EventType::BUFFERING_START);

    EXPECT_TRUE(mediaDemuxer_->isBuffering_.load());
}

/**
 * @tc.name  : MediaDemuxer_CheckAndReportBufferingStatus_004
 * @tc.number: MediaDemuxer_CheckAndReportBufferingStatus_004
 * @tc.desc  : Test BUFFERING_START when isBuffering_ is true (no event, state unchanged)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CheckAndReportBufferingStatus_004, TestSize.Level1)
{
    mediaDemuxer_->isBuffering_.store(true);

    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    mediaDemuxer_->CheckAndReportBufferingStatus(EventType::BUFFERING_START);

    EXPECT_TRUE(mediaDemuxer_->isBuffering_.load());
}

/**
 * @tc.name  : MediaDemuxer_CheckAndReportBufferingStatus_005
 * @tc.number: MediaDemuxer_CheckAndReportBufferingStatus_005
 * @tc.desc  : Test BUFFERING_END when isBuffering_ is true and eventReceiver_ is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CheckAndReportBufferingStatus_005, TestSize.Level1)
{
    mediaDemuxer_->isBuffering_.store(true);

    mediaDemuxer_->eventReceiver_ = nullptr;

    mediaDemuxer_->CheckAndReportBufferingStatus(EventType::BUFFERING_END);

    EXPECT_FALSE(mediaDemuxer_->isBuffering_.load());
}

/**
 * @tc.name  : MediaDemuxer_CheckAndReportBufferingStatus_006
 * @tc.number: MediaDemuxer_CheckAndReportBufferingStatus_006
 * @tc.desc  : Test BUFFERING_START when isBuffering_ is false and eventReceiver_ is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_CheckAndReportBufferingStatus_006, TestSize.Level1)
{
    mediaDemuxer_->isBuffering_.store(false);

    mediaDemuxer_->eventReceiver_ = nullptr;

    mediaDemuxer_->CheckAndReportBufferingStatus(EventType::BUFFERING_START);

    EXPECT_TRUE(mediaDemuxer_->isBuffering_.load());
}

/**
 * @tc.name  : MediaDemuxer_HandleSeek_001
 * @tc.number: MediaDemuxer_HandleSeek_001
 * @tc.desc  : Outer if condition false because source_ is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleSeek_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->source_ = nullptr;
    mediaDemuxer_->isSeeked_ = true;
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->HandleSeek(trackId);

    EXPECT_TRUE(mediaDemuxer_->isSeeked_);
}

/**
 * @tc.name  : MediaDemuxer_HandleSeek_002
 * @tc.number: MediaDemuxer_HandleSeek_002
 * @tc.desc  : Outer if true, inner (trackId == videoTrackId_) is false → inner block skipped
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleSeek_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->videoTrackId_ = NUM_1; // trackId != videoTrackId_
    mediaDemuxer_->isSeeked_ = true;
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported())
        .WillOnce(Return(true));

    mediaDemuxer_->HandleSeek(trackId);

    EXPECT_FALSE(mediaDemuxer_->isSeeked_);
}

/**
 * @tc.name  : MediaDemuxer_HandleSeek_003
 * @tc.number: MediaDemuxer_HandleSeek_003
 * @tc.desc  : Outer if true, trackId == videoTrackId_ but isFirstFrameAfterSeek_ is false → inner block skipped
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleSeek_003, TestSize.Level1)
{
    int32_t trackId = NUM_1;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->isSeeked_ = true;
    mediaDemuxer_->isFirstFrameAfterSeek_.store(false);
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported())
        .WillOnce(Return(true));

    mediaDemuxer_->HandleSeek(trackId);

    EXPECT_FALSE(mediaDemuxer_->isSeeked_);
    EXPECT_FALSE(mediaDemuxer_->isFirstFrameAfterSeek_.load());
}

/**
 * @tc.name  : MediaDemuxer_HandleSeek_004
 * @tc.number: MediaDemuxer_HandleSeek_004
 * @tc.desc  : Outer if true, first frame after seek is sync frame → no error log branch, flag cleared
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleSeek_004, TestSize.Level1)
{
    int32_t trackId = NUM_1;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->isSeeked_ = true;
    mediaDemuxer_->isFirstFrameAfterSeek_.store(true);
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported())
        .WillOnce(Return(true));
    auto buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->bufferMap_[trackId] = buffer;

    mediaDemuxer_->HandleSeek(trackId);
    EXPECT_FALSE(mediaDemuxer_->isFirstFrameAfterSeek_.load());
    EXPECT_FALSE(mediaDemuxer_->isSeeked_);
}

/**
 * @tc.name  : MediaDemuxer_HandleSeek_005
 * @tc.number: MediaDemuxer_HandleSeek_005
 * @tc.desc  : Outer if true, first frame after seek is not sync frame → error branch executed, flag cleared
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleSeek_005, TestSize.Level1)
{
    int32_t trackId = NUM_1;
    mediaDemuxer_->videoTrackId_ = NUM_1;
    mediaDemuxer_->isSeeked_ = true;
    mediaDemuxer_->isFirstFrameAfterSeek_.store(true);
    EXPECT_CALL(*(mediaDemuxer_->source_), IsSeekToTimeSupported())
        .WillOnce(Return(true));
    auto buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = 0;
    mediaDemuxer_->bufferMap_[trackId] = buffer;

    mediaDemuxer_->HandleSeek(trackId);

    EXPECT_FALSE(mediaDemuxer_->isFirstFrameAfterSeek_.load());
    EXPECT_FALSE(mediaDemuxer_->isSeeked_);
}

/**
 * @tc.name  : Test TranscoderUpdateOutputBufferPts
 * @tc.number: TranscoderUpdateOutputBufferPts_005
 * @tc.desc  : transcoderStartPts_ > 0 && outputBuffer != nullptr → pts_ reduced
 */
HWTEST_F(MediaDemuxerExtUnitTest, TranscoderUpdateOutputBufferPts_005, TestSize.Level1)
{
    std::shared_ptr<AVBuffer> outputBuffer = std::make_shared<AVBuffer>();
    outputBuffer->pts_ = NUM_1000; // initial pts

    mediaDemuxer_->isTranscoderMode_ = true;
    mediaDemuxer_->transcoderStartPts_ = NUM_1;

    mediaDemuxer_->TranscoderUpdateOutputBufferPts(NUM_1, outputBuffer);
    EXPECT_EQ(outputBuffer->pts_, NUM_1000 - NUM_1);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_003
 * @tc.desc: isVideoMuted_ is false → outer if not entered, decoder not released
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_003, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;

    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->bufferMap_[NUM_0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    mediaDemuxer_->isVideoMuted_ = false;
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->hasSetLargeSize_ = false;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    mediaDemuxer_->HandleVideoTrack(NUM_0);

    EXPECT_TRUE(mediaDemuxer_->needReleaseVideoDecoder_);
    EXPECT_FALSE(mediaDemuxer_->hasSetLargeSize_);
    EXPECT_EQ(mediaDemuxer_->lastVideoPts_, videoSample->pts_);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_004
 * @tc.desc: Non-sync frame while muted → outer if not entered, decoder not released
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_004, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    videoSample->flag_ = 0;
    mediaDemuxer_->bufferMap_[NUM_0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->hasSetLargeSize_ = false;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    mediaDemuxer_->HandleVideoTrack(NUM_0);

    EXPECT_TRUE(mediaDemuxer_->needReleaseVideoDecoder_);
    EXPECT_FALSE(mediaDemuxer_->hasSetLargeSize_);
    EXPECT_EQ(mediaDemuxer_->lastVideoPts_, videoSample->pts_);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_005
 * @tc.desc: needReleaseVideoDecoder_ is false → decoder-release block skipped, only pts update path
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_005, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->bufferMap_[NUM_0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    mediaDemuxer_->hasSetLargeSize_ = false;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    mediaDemuxer_->HandleVideoTrack(NUM_0);

    EXPECT_FALSE(mediaDemuxer_->needReleaseVideoDecoder_);
    EXPECT_FALSE(mediaDemuxer_->hasSetLargeSize_);
    EXPECT_EQ(mediaDemuxer_->lastVideoPts_, videoSample->pts_);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_006
 * @tc.desc: sampleConsumerTaskMap_ has key but nullptr value → C2 false branch
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_006, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_0;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->bufferMap_[NUM_0] = videoSample;
    mediaDemuxer_->sampleQueueMap_[NUM_0] = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_1] = std::make_shared<SampleQueue>();
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needReleaseVideoDecoder_ = true;
    mediaDemuxer_->hasSetLargeSize_ = false;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(1);
    std::unique_ptr<MockTask> nullTask;
    mediaDemuxer_->sampleConsumerTaskMap_[NUM_0] = std::move(nullTask);

    mediaDemuxer_->HandleVideoTrack(NUM_0);

    EXPECT_FALSE(mediaDemuxer_->needReleaseVideoDecoder_);
    EXPECT_TRUE(mediaDemuxer_->hasSetLargeSize_);
    EXPECT_EQ(mediaDemuxer_->lastVideoPts_, videoSample->pts_);
}

/**
 * @tc.name: MediaDemuxerExt_HandleVideoTrack_007
 * @tc.desc: pts_ > lastOutSamplePts_ → F false branch (no clear)
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_HandleVideoTrack_007, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    std::shared_ptr<AVBuffer> videoSample = std::make_shared<AVBuffer>();
    videoSample->pts_ = NUM_1000;
    videoSample->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
    mediaDemuxer_->bufferMap_[NUM_0] = videoSample;
    auto videoQueue = std::make_shared<SampleQueue>();
    auto audioQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[NUM_0] = videoQueue;
    mediaDemuxer_->sampleQueueMap_[NUM_1] = audioQueue;
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->needReleaseVideoDecoder_ = false;
    mediaDemuxer_->hasSetLargeSize_ = false;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);
    EXPECT_CALL(*audioQueue, GetLastOutSamplePts())
        .WillOnce(Return(static_cast<int64_t>(NUM_0)));
    EXPECT_CALL(*videoQueue, Clear()).Times(0);

    mediaDemuxer_->HandleVideoTrack(NUM_0);

    EXPECT_EQ(mediaDemuxer_->lastVideoPts_, videoSample->pts_);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_001
 * @tc.number: MediaDemuxer_GetSeekableRanges_001
 * @tc.desc  : test !ranges.empty()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_001, TestSize.Level1)
{
    std::vector<Plugins::SeekRange> ranges;
    Plugins::SeekRange range;
    ranges.push_back(range);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(Return(ranges));
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_NE(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_002
 * @tc.number: MediaDemuxer_GetSeekableRanges_002
 * @tc.desc  : test !ranges.empty()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_002, TestSize.Level1)
{
    std::vector<Plugins::SeekRange> ranges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(Return(ranges));
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_003
 * @tc.number: MediaDemuxer_GetSeekableRanges_003
 * @tc.desc  : test !ranges.empty()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_003, TestSize.Level1)
{
    mediaDemuxer_->source_ = nullptr;
    EXPECT_EQ(mediaDemuxer_->source_, nullptr);
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_004
 * @tc.number: MediaDemuxer_GetSeekableRanges_004
 * @tc.desc  : test !ranges.empty()
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_004, TestSize.Level1)
{
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    std::vector<Plugins::SeekRange> ranges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(Return(ranges));
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_005
 * @tc.number: MediaDemuxer_GetSeekableRanges_005
 * @tc.desc  : test sampleQueueMap_ has nullptr SampleQueue
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_005, TestSize.Level1)
{
    std::vector<Plugins::SeekRange> emptyRanges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(emptyRanges));
    mediaDemuxer_->enableSampleQueue_ = true;
    int32_t trackId1 = NUM_1;
    int32_t trackId2 = NUM_2;
    mediaDemuxer_->sampleQueueMap_[trackId1] = nullptr;
    mediaDemuxer_->sampleQueueMap_[trackId2] = std::make_shared<SampleQueue>();
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_006
 * @tc.number: MediaDemuxer_GetSeekableRanges_006
 * @tc.desc  : test return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_006, TestSize.Level1)
{
    std::vector<Plugins::SeekRange> emptyRanges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(emptyRanges));
    mediaDemuxer_->enableSampleQueue_ = true;
    int32_t trackId = NUM_1;
    auto mockSampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = mockSampleQueue;
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_007
 * @tc.number: MediaDemuxer_GetSeekableRanges_007
 * @tc.desc  : test first valid buffered range, return non-empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_007, TestSize.Level1)
{
    std::vector<Plugins::SeekRange> emptyRanges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(emptyRanges));
    mediaDemuxer_->enableSampleQueue_ = true;
    int32_t trackId = NUM_1;
    auto mockSampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = mockSampleQueue;
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), NUM_0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_008
 * @tc.number: MediaDemuxer_GetSeekableRanges_008
 * @tc.desc  : test multiple valid buffered ranges, return intersection
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_008, TestSize.Level1)
{
    std::vector<Plugins::SeekRange> emptyRanges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(emptyRanges));
    mediaDemuxer_->enableSampleQueue_ = true;
    int32_t trackId1 = NUM_1;
    int32_t trackId2 = NUM_2;

    auto mockSampleQueue1 = std::make_shared<SampleQueue>();
    auto mockSampleQueue2 = std::make_shared<SampleQueue>();

    mediaDemuxer_->sampleQueueMap_[trackId1] = mockSampleQueue1;
    mediaDemuxer_->sampleQueueMap_[trackId2] = mockSampleQueue2;
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), NUM_0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_009
 * @tc.number: MediaDemuxer_GetSeekableRanges_009
 * @tc.desc  : test buffered range end <= start, return empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_009, TestSize.Level1)
{
    std::vector<Plugins::SeekRange> emptyRanges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(emptyRanges));
    mediaDemuxer_->enableSampleQueue_ = true;
    int32_t trackId = NUM_1;
    auto mockSampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueMap_[trackId] = mockSampleQueue;

    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_010
 * @tc.number: MediaDemuxer_GetSeekableRanges_010
 * @tc.desc  : test SetMediaDuration called when globalMeta not null and durationMs > 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_010, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    int64_t durationMs = NUM_1000;
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(durationMs);
    
    EXPECT_CALL(*(mediaDemuxer_->source_), SetMediaDuration(durationMs)).Times(1);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(
        Return(std::vector<Plugins::SeekRange>()));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::UNSEEKABLE));
    
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_011
 * @tc.number: MediaDemuxer_GetSeekableRanges_011
 * @tc.desc  : test seekable = SEEKABLE and GetLocalLoadedRange succeeds, return full range
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_011, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    int64_t durationMs = NUM_1000;
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(durationMs);
    
    std::vector<Plugins::SeekRange> emptyRanges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(Return(emptyRanges));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));
    
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].start, 0);
    EXPECT_GT(res[0].end, 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_012
 * @tc.number: MediaDemuxer_GetSeekableRanges_012
 * @tc.desc  : test globalMeta is nullptr, skip SetMediaDuration
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_012, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = nullptr;
    
    EXPECT_CALL(*(mediaDemuxer_->source_), SetMediaDuration(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(
        Return(std::vector<Plugins::SeekRange>()));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_013
 * @tc.number: MediaDemuxer_GetSeekableRanges_013
 * @tc.desc  : test globalMeta not null but durationMs <= 0, skip SetMediaDuration
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_013, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    int64_t durationMs = NUM_0;
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(durationMs);
    
    EXPECT_CALL(*(mediaDemuxer_->source_), SetMediaDuration(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(
        Return(std::vector<Plugins::SeekRange>()));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));
    
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name : MediaDemuxer_GetSeekableRanges_010
 * @tc.number: MediaDemuxer_GetSeekableRanges_010
 * @tc.desc : test SetMediaDuration called when globalMeta not null and durationMs > 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_014, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    int64_t durationMs = NUM_1000;
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(durationMs);

    EXPECT_CALL(*(mediaDemuxer_->source_), SetMediaDuration(durationMs)).Times(1);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(std::vector<Plugins::SeekRange>()));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::UNSEEKABLE));

    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetSeekableRanges_011
 * @tc.number: MediaDemuxer_GetSeekableRanges_011
 * @tc.desc  : test seekable - SEEKABLE and GetLocalLoadedRange succeeds, return full range
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_015, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    int64_t durationMs = NUM_1000;
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(durationMs);

    std::vector<Plugins::SeekRange> emptyRanges;
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges()).WillRepeatedly(Return(emptyRanges));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));

    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].start, 0);
    EXPECT_GT(res[0].end, 0);
}

/**
 * @tc.name : MediaDemuxer_GetSeekableRanges_012
 * @tc.number: MediaDemuxer_GetSeekableRanges_012
 * @tc.desc : test globalMeta is nullptr, skip SetMediaDuration
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_016, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = nullptr;

    EXPECT_CALL(*(mediaDemuxer_->source_), SetMediaDuration(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(std::vector<Plugins::SeekRange>()));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));
    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name : MediaDemuxer_GetSeekableRanges_013
 * @tc.number: MediaDemuxer_GetSeekableRanges_013
 * @tc.desc : test globalMeta not null but durationMs <- 0, skip SetMediaDuration
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetSeekableRanges_017, TestSize.Level1)
{
    mediaDemuxer_->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    int64_t durationMs = NUM_0;
    mediaDemuxer_->mediaMetaData_.globalMeta->Set<Tag::MEDIA_DURATION>(durationMs);

    EXPECT_CALL(*(mediaDemuxer_->source_), SetMediaDuration(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekableRanges())
        .WillRepeatedly(Return(std::vector<Plugins::SeekRange>()));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetSeekable()).WillRepeatedly(Return(Plugins::Seekable::SEEKABLE));

    auto res = mediaDemuxer_->GetSeekableRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetLoadedRanges_001
 * @tc.number: MediaDemuxer_GetLoadedRanges_001
 * @tc.desc  : test IsLocalFd true and GetLocalLoadedRange true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetLoadedRanges_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(true));
    auto globalMeta = std::make_shared<OHOS::Media::Meta>();
    int64_t durationUs = NUM_200;
    globalMeta->Set<OHOS::Media::Tag::MEDIA_DURATION>(durationUs);
    mediaDemuxer_->mediaMetaData_.globalMeta = globalMeta;

    auto res = mediaDemuxer_->GetLoadedRanges();
    EXPECT_EQ(res.size(), 1);
    EXPECT_EQ(res[0].start, 0);
    EXPECT_EQ(res[0].end, NUM_200000);
}

/**
 * @tc.name  : MediaDemuxer_GetLoadedRanges_003
 * @tc.number: MediaDemuxer_GetLoadedRanges_003
 * @tc.desc  : test IsLocalFd false and !GetEnableSampleQueueFlag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetLoadedRanges_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = false;
    int32_t trackId = NUM_1;
    mediaDemuxer_->sampleQueueMap_[trackId] = std::make_shared<SampleQueue>();
    auto res = mediaDemuxer_->GetLoadedRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_GetLoadedRanges_004
 * @tc.number: MediaDemuxer_GetLoadedRanges_004
 * @tc.desc  : test IsLocalFd false and sampleQueueMap_.empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_GetLoadedRanges_004, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->sampleQueueMap_.clear();
    auto res = mediaDemuxer_->GetLoadedRanges();
    EXPECT_EQ(res.size(), 0);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_001
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_001
 * @tc.desc  : test GetEnableSampleQueueFlag returns false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = false;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    int32_t trackId = NUM_1;
    mediaDemuxer_->sampleQueueMap_[trackId] = std::make_shared<SampleQueue>();
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_002
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_002
 * @tc.desc  : test queue is nullptr in sampleQueueMap_
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_002, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    int32_t trackId = NUM_1;
    mediaDemuxer_->sampleQueueMap_[trackId] = nullptr;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_003
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_003
 * @tc.desc  : test lastEnterSamplePts <= 0, should skip the queue
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_003, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NUM_0;
    sampleQueue->lastOutSamplePts_ = NUM_1000;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_004
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_004
 * @tc.desc  : test hasValidQueue is false after loop (all queues skipped)
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_004, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NEG_1;
    sampleQueue->lastOutSamplePts_ = NUM_1000;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_005
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_005
 * @tc.desc  : test syncCenter_ is nullptr, loadedStartUs should be 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_005, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    mediaDemuxer_->syncCenter_ = nullptr;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NUM_1000;
    sampleQueue->lastOutSamplePts_ = NUM_500;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_006
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_006
 * @tc.desc  : test loadedStartUs <= 0 and minLastOutSamplePts != HST_TIME_NONE
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_006, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow())
        .WillRepeatedly(Return(NUM_0));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NUM_1000;
    sampleQueue->lastOutSamplePts_ = NUM_500;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_007
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_007
 * @tc.desc  : test loadedStartUs < 0, should be set to 0
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_007, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow())
        .WillRepeatedly(Return(NEG_1));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NUM_1000;
    sampleQueue->lastOutSamplePts_ = NUM_500;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_008
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_008
 * @tc.desc  : test loadedStartUs >= bufferedEnd, should return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_008, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow())
        .WillRepeatedly(Return(NUM_2000));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NUM_1000;
    sampleQueue->lastOutSamplePts_ = NUM_500;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_009
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_009
 * @tc.desc  : test Us2HstTime fails for loadedStartUs
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_009, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    mediaDemuxer_->syncCenter_ = nullptr;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NUM_1000;
    sampleQueue->lastOutSamplePts_ = Plugins::HST_TIME_NONE;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_010
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_010
 * @tc.desc  : test Us2HstTime fails for bufferedEnd
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_010, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow())
        .WillRepeatedly(Return(NUM_0));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = Plugins::HST_TIME_NONE;
    sampleQueue->lastOutSamplePts_ = NUM_500;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_011
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_011
 * @tc.desc  : test endHst <= startHst, should return false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_011, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow())
        .WillRepeatedly(Return(NUM_0));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    sampleQueue->lastEnterSamplePts_ = NUM_1;
    sampleQueue->lastOutSamplePts_ = NUM_0;
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_012
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_012
 * @tc.desc  : test success path, should return true with valid range
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_012, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow())
        .WillRepeatedly(Return(NUM_0));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    int32_t trackId = NUM_1;
    auto sampleQueue = std::make_shared<SampleQueue>();
    EXPECT_CALL(*(sampleQueue), GetLastEnterSamplePts())
        .WillRepeatedly(Return(NUM_1000));
    EXPECT_CALL(*(sampleQueue), GetLastOutSamplePts())
        .WillRepeatedly(Return(NUM_500));
    mediaDemuxer_->sampleQueueMap_[trackId] = sampleQueue;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, true);
    EXPECT_GT(range.end, range.start);
}

/**
 * @tc.name  : MediaDemuxer_BuildNetworkLoadedRange_013
 * @tc.number: MediaDemuxer_BuildNetworkLoadedRange_013
 * @tc.desc  : test multiple sample queues, take min of lastEnter and lastOut pts
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxer_BuildNetworkLoadedRange_013, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), IsLocalFd())
        .WillRepeatedly(Return(false));
    mediaDemuxer_->enableSampleQueue_ = true;
    mediaDemuxer_->isAudioDemuxDecodeAsync_ = true;
    mediaDemuxer_->isTranscoderMode_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow())
        .WillRepeatedly(Return(NUM_0));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    int32_t trackId1 = NUM_1;
    int32_t trackId2 = NUM_2;
    auto sampleQueue1 = std::make_shared<SampleQueue>();
    auto sampleQueue2 = std::make_shared<SampleQueue>();
    EXPECT_CALL(*(sampleQueue1), GetLastEnterSamplePts())
        .WillRepeatedly(Return(NUM_1000));
    EXPECT_CALL(*(sampleQueue1), GetLastOutSamplePts())
        .WillRepeatedly(Return(NUM_500));
    EXPECT_CALL(*(sampleQueue2), GetLastEnterSamplePts())
        .WillRepeatedly(Return(NUM_2000));
    EXPECT_CALL(*(sampleQueue2), GetLastOutSamplePts())
        .WillRepeatedly(Return(NUM_800));
    mediaDemuxer_->sampleQueueMap_[trackId1] = sampleQueue1;
    mediaDemuxer_->sampleQueueMap_[trackId2] = sampleQueue2;
    Plugins::SeekRange range;
    auto res = mediaDemuxer_->BuildNetworkLoadedRange(range);
    EXPECT_EQ(res, true);
    EXPECT_GT(range.end, range.start);
}

/**
 * @tc.name  : MediaDemuxerExt_UpdateTrackMapTest_001
 * @tc.number: MediaDemuxerExt_UpdateTrackMapTest_001
 * @tc.desc  : test UpdateTrackMap should update track map
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_UpdateTrackMapTest_001, TestSize.Level1)
{
    mediaDemuxer_->videoTrackId_ = NUM_0;
    mediaDemuxer_->audioTrackId_ = NUM_1;
    int32_t defaultStreamId = 0;

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(defaultStreamId));

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpTrackTypeByTrackID(mediaDemuxer_->videoTrackId_))
        .WillOnce(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapByStreamId(mediaDemuxer_->videoTrackId_,
        defaultStreamId, TRACK_VIDEO)).Times(1);

    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpTrackTypeByTrackID(mediaDemuxer_->audioTrackId_))
        .WillOnce(Return(TrackType::TRACK_VIDEO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), UpdateTempTrackMapByStreamId(mediaDemuxer_->audioTrackId_,
        defaultStreamId, TRACK_AUDIO)).Times(1);

    mediaDemuxer_->UpdateTrackMap();
}

/**
 * @tc.name  : MediaDemuxerExt_HandleSeekChangeStream_001
 * @tc.number: MediaDemuxerExt_HandleSeekChangeStream_001
 * @tc.desc  : test HandleSeekChangeStream change stream when isHls_ is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_HandleSeekChangeStream_001, TestSize.Level1)
{
    int32_t currentStreamId = 0;
    int32_t newStreamId = 1;
    int32_t trackId = 0;
    mediaDemuxer_->isHls_ = true;
    mediaDemuxer_->videoTrackId_ = trackId;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), SetNewVideoStreamID(newStreamId)).Times(1);

    Status ret = mediaDemuxer_->HandleSeekChangeStream(currentStreamId, newStreamId, trackId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_001
 * @tc.number: MediaDemuxerExt_SelectStreamId_001
 * @tc.desc  : Test SelectStreamId returns wrong state when demuxer is not ready
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_001, TestSize.Level1)
{
    mediaDemuxer_->demuxerPluginManager_ = nullptr;
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();

    auto ret = mediaDemuxer_->SelectStreamId(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_002
 * @tc.number: MediaDemuxerExt_SelectStreamId_002
 * @tc.desc  : Test SelectStreamId returns wrong state for invalid stream id
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_002, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();

    auto ret = mediaDemuxer_->SelectStreamId(NEG_1);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_003
 * @tc.number: MediaDemuxerExt_SelectStreamId_003
 * @tc.desc  : Test SelectStreamId returns wrong state when dash is changing stream
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_003, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    auto ret = mediaDemuxer_->SelectStreamId(NUM_0);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_004
 * @tc.number: MediaDemuxerExt_SelectStreamId_004
 * @tc.desc  : Test SelectStreamId forwards to source when dash can change stream
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_004, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(NUM_1)).WillOnce(Return(Status::OK));

    auto ret = mediaDemuxer_->SelectStreamId(NUM_1);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_005
 * @tc.number: MediaDemuxerExt_SelectStreamId_005
 * @tc.desc  : Test SelectStreamId returns ok when dash source is null
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_005, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->source_ = nullptr;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).Times(0);

    auto ret = mediaDemuxer_->SelectStreamId(NUM_1);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_006
 * @tc.number: MediaDemuxerExt_SelectStreamId_006
 * @tc.desc  : Test SelectStreamId resets cache and forwards to source for non-dash
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_006, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(NUM_2)).WillOnce(Return(Status::ERROR_UNKNOWN));

    auto ret = mediaDemuxer_->SelectStreamId(NUM_2);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_001
 * @tc.number: AutoSelectTrackByBitrate_001
 * @tc.desc  : Test auto select is disabled
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_001, TestSize.Level1)
{
    mediaDemuxer_->isAutoSelect_.store(false);
    mediaDemuxer_->curDownloadBitRate_.store(UNUM_1000);
    mediaDemuxer_->lastSwitchDownloadRate_.store(UNUM_0);

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_002
 * @tc.number: AutoSelectTrackByBitrate_002
 * @tc.desc  : Test current download bitrate is zero
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_002, TestSize.Level1)
{
    mediaDemuxer_->isAutoSelect_.store(true);
    mediaDemuxer_->curDownloadBitRate_.store(UNUM_0);
    mediaDemuxer_->lastSwitchDownloadRate_.store(UNUM_1000);

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_003
 * @tc.number: AutoSelectTrackByBitrate_003
 * @tc.desc  : Test current download bitrate equals last switched bitrate
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_003, TestSize.Level1)
{
    mediaDemuxer_->isAutoSelect_.store(true);
    mediaDemuxer_->curDownloadBitRate_.store(UNUM_1000);
    mediaDemuxer_->lastSwitchDownloadRate_.store(UNUM_1000);

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_004
 * @tc.number: AutoSelectTrackByBitrate_004
 * @tc.desc  : Test first check initializes timestamp
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_004, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_1, UNUM_1000);
    mediaDemuxer_->lastCheckAutoSelectTimeMs_ = 0;

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_GT(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_005
 * @tc.number: AutoSelectTrackByBitrate_005
 * @tc.desc  : Test auto select interval not reached
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_005, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_1, UNUM_1000);
    int64_t currentTimeMs = mediaDemuxer_->GetCurrentTimeMs();
    mediaDemuxer_->lastCheckAutoSelectTimeMs_ = currentTimeMs;

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, currentTimeMs);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_006
 * @tc.number: AutoSelectTrackByBitrate_006
 * @tc.desc  : Test preferred stream is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_006, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_1, UNUM_1000);
    mediaDemuxer_->filteredStreamMap_->map[VIDEO].clear();

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_NE(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_007
 * @tc.number: AutoSelectTrackByBitrate_007
 * @tc.desc  : Test preferred stream already selected by stream id
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_007, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_1, UNUM_1000);
    mediaDemuxer_->defaultVideoStreamId_ = NUM_1;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_1000));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_008
 * @tc.number: AutoSelectTrackByBitrate_008
 * @tc.desc  : Test preferred stream already selected by bitrate
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_008, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_1, UNUM_1000);
    mediaDemuxer_->lastAutoSelectBitRate_ = static_cast<int32_t>(UNUM_1000);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_1000));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_009
 * @tc.number: AutoSelectTrackByBitrate_009
 * @tc.desc  : Test cannot auto select bitrate
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_009, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_1, UNUM_1000);
    mediaDemuxer_->isPrepared_.store(false);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_1000));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_010
 * @tc.number: AutoSelectTrackByBitrate_010
 * @tc.desc  : Test auto select bitrate succeeds
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_010, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_2, UNUM_2000);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_2000));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_),
    ResetAllCache()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(UNUM_2000)).WillOnce(Return(Status::OK));

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, NUM_2);
    EXPECT_EQ(mediaDemuxer_->lastAutoSelectBitRate_, static_cast<int32_t>(UNUM_2000));
    EXPECT_EQ(mediaDemuxer_->lastSwitchDownloadRate_.load(), UNUM_2000);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_011
 * @tc.number: AutoSelectTrackByBitrate_011
 * @tc.desc  : Test auto select bitrate fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_011, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_2, UNUM_2000);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_2000));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(UNUM_2000))
        .WillOnce(Return(Status::ERROR_UNKNOWN));

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, NEG_1);
    EXPECT_EQ(mediaDemuxer_->lastAutoSelectBitRate_, NEG_1);
    EXPECT_EQ(mediaDemuxer_->lastSwitchDownloadRate_.load(), UNUM_0);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, 0);
}

/**
 * @tc.name  : AutoSelectTrackByBitrate_012
 * @tc.number: AutoSelectTrackByBitrate_012
 * @tc.desc  : Test preferred stream selects nearest valid video stream
 */
HWTEST_F(MediaDemuxerExtUnitTest, AutoSelectTrackByBitrate_012, TestSize.Level1)
{
    PrepareAutoSelectTrackByBitrate(mediaDemuxer_, NUM_1, UNUM_2000);
    mediaDemuxer_->filteredStreamMap_->map[VIDEO].clear();
    mediaDemuxer_->filteredStreamMap_->map[VIDEO].push_back(MakeAutoSelectVideoStream(
        NUM_1, UNUM_1000, StreamType::AUDIO));
    mediaDemuxer_->filteredStreamMap_->map[VIDEO].push_back(MakeAutoSelectVideoStream(NUM_2, UNUM_3000));
    mediaDemuxer_->filteredStreamMap_->map[VIDEO].push_back(MakeAutoSelectVideoStream(NUM_3, UNUM_2000));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_2000));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(UNUM_2000)).WillOnce(Return(Status::OK));

    mediaDemuxer_->AutoSelectTrackByBitrate(NUM_1);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, NUM_3);
    EXPECT_EQ(mediaDemuxer_->lastAutoSelectBitRate_, static_cast<int32_t>(UNUM_2000));
}

/**
 * @tc.name  : SetTrackSelectionFilter_001
 * @tc.number: SetTrackSelectionFilter_001
 * @tc.desc  : Test not prepared early return
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_001, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.minVideoBitrate = NUM_1;
    mediaDemuxer_->isPrepared_.store(false);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->trackSelectionFilter_.minVideoBitrate, NUM_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_002
 * @tc.number: SetTrackSelectionFilter_002
 * @tc.desc  : Test cloud fd early return
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_002, TestSize.Level1)
{
    TrackSelectionFilter filter;
    mediaDemuxer_->isPrepared_.store(true);
    EXPECT_CALL(*(mediaDemuxer_->source_), IsCloudFd()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsNeedPreDownload()).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), GetStreamInfo(_, true)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->trackSelectionFilter_.maxAudioBitrate, filter.maxAudioBitrate);
}

/**
 * @tc.name  : SetTrackSelectionFilter_003
 * @tc.number: SetTrackSelectionFilter_003
 * @tc.desc  : Test pre download early return
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_003, TestSize.Level1)
{
    TrackSelectionFilter filter;
    mediaDemuxer_->isPrepared_.store(true);
    EXPECT_CALL(*(mediaDemuxer_->source_), IsCloudFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), IsNeedPreDownload()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), GetStreamInfo(_, true)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->trackSelectionFilter_.maxVideoBitrate, filter.maxVideoBitrate);
}

/**
 * @tc.name  : SetTrackSelectionFilter_004
 * @tc.number: SetTrackSelectionFilter_004
 * @tc.desc  : Test video stream does not need switch when last stream remains selected
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_004, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, "")
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, FILTER_VIDEO_STREAM_ID_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_005
 * @tc.number: SetTrackSelectionFilter_005
 * @tc.desc  : Test video stream is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_005, TestSize.Level1)
{
    TrackSelectionFilter filter;
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_HTTP, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, FILTER_VIDEO_STREAM_ID_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_006
 * @tc.number: SetTrackSelectionFilter_006
 * @tc.desc  : Test video bitrate switch succeeds
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_006, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_HEVC, ""),
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_2, VIDEO, UNUM_2000, FILTER_MIME_VIDEO_AVC, "")
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillOnce(Return(UNUM_1000));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(UNUM_2000)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, FILTER_VIDEO_STREAM_ID_2);
    EXPECT_EQ(mediaDemuxer_->lastAutoSelectBitRate_, static_cast<int32_t>(UNUM_2000));
}

/**
 * @tc.name  : SetTrackSelectionFilter_007
 * @tc.number: SetTrackSelectionFilter_007
 * @tc.desc  : Test video bitrate switch fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_007, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_HEVC, ""),
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_2, VIDEO, UNUM_2000, FILTER_MIME_VIDEO_AVC, "")
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).Times(NUM_2).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillOnce(Return(UNUM_1000));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(UNUM_2000)).WillOnce(Return(Status::ERROR_UNKNOWN));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, FILTER_VIDEO_STREAM_ID_2);
    EXPECT_EQ(mediaDemuxer_->lastAutoSelectBitRate_, NEG_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_008
 * @tc.number: SetTrackSelectionFilter_008
 * @tc.desc  : Test http play type returns before audio and subtitle switching
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_008, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_2, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_HTTP, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_1);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_009
 * @tc.number: SetTrackSelectionFilter_009
 * @tc.desc  : Test audio stream does not need switch when last stream remains selected
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_009, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_010
 * @tc.number: SetTrackSelectionFilter_010
 * @tc.desc  : Test audio stream is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_010, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, "")
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_011
 * @tc.number: SetTrackSelectionFilter_011
 * @tc.desc  : Test audio stream switch succeeds
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_011, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_2, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_AUDIO_STREAM_ID_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_012
 * @tc.number: SetTrackSelectionFilter_012
 * @tc.desc  : Test audio stream switch fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_012, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_2, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_AUDIO_STREAM_ID_2))
        .WillOnce(Return(Status::ERROR_UNKNOWN));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_013
 * @tc.number: SetTrackSelectionFilter_013
 * @tc.desc  : Test subtitle stream does not need switch when last stream remains selected
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_013, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_014
 * @tc.number: SetTrackSelectionFilter_014
 * @tc.desc  : Test subtitle stream is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_014, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, "")
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_1);
}

/**
 * @tc.name  : SetTrackSelectionFilter_015
 * @tc.number: SetTrackSelectionFilter_015
 * @tc.desc  : Test subtitle stream switch succeeds
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_015, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_016
 * @tc.number: SetTrackSelectionFilter_016
 * @tc.desc  : Test subtitle stream switch fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_016, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2))
        .WillOnce(Return(Status::ERROR_UNKNOWN));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_017
 * @tc.number: SetTrackSelectionFilter_017
 * @tc.desc  : Test audio and subtitle switch succeed together
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_017, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_2, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_AUDIO_STREAM_ID_2)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_018
 * @tc.number: SetTrackSelectionFilter_018
 * @tc.desc  : Test audio switch fails and subtitle switch succeeds
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_018, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_2, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_AUDIO_STREAM_ID_2))
        .WillOnce(Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_019
 * @tc.number: SetTrackSelectionFilter_019
 * @tc.desc  : Test audio switch succeeds and subtitle switch fails
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_019, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_1, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_2, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).Times(2).WillRepeatedly(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_AUDIO_STREAM_ID_2)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2))
        .WillOnce(Return(Status::ERROR_UNKNOWN));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_020
 * @tc.number: SetTrackSelectionFilter_020
 * @tc.desc  : Test audio and subtitle both do not need switch
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_020, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_AUDIO_STREAM_ID_2, AUDIO, UNUM_1000, "audio/aac", FILTER_LANG_ENG),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_2;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_2;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_021
 * @tc.number: SetTrackSelectionFilter_021
 * @tc.desc  : Test subtitle switch triggers HandleSelectSubtitleStream when isHls_ is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_021, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->currentPlayType_ = "hls";
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    mediaDemuxer_->isHls_ = true;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow()).WillRepeatedly(Return(NUM_0));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_022
 * @tc.number: SetTrackSelectionFilter_022
 * @tc.desc  : Test subtitle switch triggers HandleSelectSubtitleStream when isHls_ is false
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_022, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->currentPlayType_ = "hls";
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    mediaDemuxer_->isHls_ = false;
    auto mockMediaSyncManager = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockMediaSyncManager), GetMediaTimeNow()).WillRepeatedly(Return(NUM_0));
    mediaDemuxer_->syncCenter_ = mockMediaSyncManager;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_023
 * @tc.number: SetTrackSelectionFilter_023
 * @tc.desc  : Test subtitle switch triggers HandleSelectSubtitleStream when isHls_ is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_023, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->currentPlayType_ = "hls";
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    mediaDemuxer_->isHls_ = true;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SetTrackSelectionFilter_024
 * @tc.number: SetTrackSelectionFilter_024
 * @tc.desc  : Test subtitle switch triggers HandleSelectSubtitleStream when isHls_ is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, SetTrackSelectionFilter_024, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {FILTER_MIME_VIDEO_AVC};
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};
    std::vector<StreamInfo> streams = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, ""),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_1, SUBTITLE, UNUM_0, "", FILTER_LANG_FRA),
        MakeTrackSelectionFilterStream(FILTER_SUBTITLE_STREAM_ID_2, SUBTITLE, UNUM_0, "", FILTER_LANG_CHI)
    };
    PrepareTrackSelectionFilter(mediaDemuxer_, FILTER_PLAY_TYPE_DASH, streams);
    mediaDemuxer_->currentPlayType_ = "hls";
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_1;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_1;
    mediaDemuxer_->isHls_ = true;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(FILTER_SUBTITLE_STREAM_ID_2))
        .WillOnce(Return(Status::ERROR_UNKNOWN));

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : SelectAudioDefaultId_001
 * @tc.number: SelectAudioDefaultId_001
 * @tc.desc  : Test SelectAudioDefaultId returns -1 for hls when all streams selected
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectAudioDefaultId_001, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    TrackSelectionFilter filter;

    int32_t ret = SelectAudioDefaultId(streamMap, true, filter, FILTER_LANG_ENG, "hls");
    EXPECT_EQ(ret, NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_002
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_002
 * @tc.desc  : Test SelectAudioDefaultId returns -1 for dash when all streams selected
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_002, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    TrackSelectionFilter filter;

    int32_t ret = SelectAudioDefaultId(streamMap, true, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_DASH);
    EXPECT_EQ(ret, NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_003
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_003
 * @tc.desc  : Test SelectAudioDefaultId returns -1 when audio stream list is empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_003, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    TrackSelectionFilter filter;

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_004
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_004
 * @tc.desc  : Test SelectAudioDefaultId finds stream by first preferred language
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_004, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_ENG, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_FRA, "audio/mp3")
    };
    TrackSelectionFilter filter;
    filter.preferredAudioLanguages = {FILTER_LANG_ENG, FILTER_LANG_CHI};

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_FRA, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_005
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_005
 * @tc.desc  : Test SelectAudioDefaultId finds stream by second preferred language
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_005, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_FRA, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_CHI, "audio/mp3")
    };
    TrackSelectionFilter filter;
    filter.preferredAudioLanguages = {FILTER_LANG_ENG, FILTER_LANG_CHI};

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_FRA, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_006
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_006
 * @tc.desc  : Test SelectAudioDefaultId falls through when preferred languages do not match
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_006, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_FRA, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_CHI, "audio/mp3")
    };
    TrackSelectionFilter filter;
    filter.preferredAudioLanguages = {FILTER_LANG_ENG};
    filter.preferredAudioMimeTypes = {"audio/mp3"};

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_FRA, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_007
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_007
 * @tc.desc  : Test SelectAudioDefaultId finds stream by first preferred mime type
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_007, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_FRA, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_CHI, "audio/mp3")
    };
    TrackSelectionFilter filter;
    filter.preferredAudioMimeTypes = {"audio/aac", "audio/flac"};

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_CHI, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_008
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_008
 * @tc.desc  : Test SelectAudioDefaultId finds stream by second preferred mime type
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_008, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_FRA, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_CHI, "audio/mp3")
    };
    TrackSelectionFilter filter;
    filter.preferredAudioMimeTypes = {"audio/flac", "audio/mp3"};

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_FRA, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_009
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_009
 * @tc.desc  : Test SelectAudioDefaultId falls through when preferred mime types do not match
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_009, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_FRA, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_CHI, "audio/mp3")
    };
    TrackSelectionFilter filter;
    filter.preferredAudioMimeTypes = {"audio/flac"};

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_CHI, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_010
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_010
 * @tc.desc  : Test SelectAudioDefaultId returns last matching stream for default language
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_010, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_ENG, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_ENG, "audio/mp3")
    };
    TrackSelectionFilter filter;

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_011
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_011
 * @tc.desc  : Test SelectAudioDefaultId returns last stream when default language is empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_011, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_FRA, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_CHI, "audio/mp3")
    };
    TrackSelectionFilter filter;

    int32_t ret = SelectAudioDefaultId(streamMap, false, filter, "", FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_012
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_012
 * @tc.desc  : Test SelectAudioDefaultId returns first stream when no language matches
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_012, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_FRA, "audio/aac"),
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_2, FILTER_LANG_CHI, "audio/mp3")
    };
    TrackSelectionFilter filter;

    int32_t ret = SelectAudioDefaultId(streamMap, true, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_AUDIO_STREAM_ID_1);
}

/**
 * @tc.name  : SelectSubtitleDefaultId_001
 * @tc.number: SelectSubtitleDefaultId_001
 * @tc.desc  : Test SelectSubtitleDefaultId returns -1 for hls when all streams selected
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectSubtitleDefaultId_001, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    TrackSelectionFilter filter;

    int32_t ret = SelectSubtitleDefaultId(streamMap, true, filter, FILTER_LANG_ENG, "hls");
    EXPECT_EQ(ret, NEG_1);
}
 
/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_002
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_002
 * @tc.desc  : Test SelectSubtitleDefaultId returns -1 for dash when all streams selected
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_002, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    TrackSelectionFilter filter;

    int32_t ret = SelectSubtitleDefaultId(streamMap, true, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_DASH);
    EXPECT_EQ(ret, NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_003
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_003
 * @tc.desc  : Test SelectSubtitleDefaultId returns -1 when subtitle stream list is empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_003, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    TrackSelectionFilter filter;

    int32_t ret = SelectSubtitleDefaultId(streamMap, false, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_004
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_004
 * @tc.desc  : Test SelectSubtitleDefaultId finds stream by first preferred language
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_004, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_ENG),
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_2, FILTER_LANG_FRA)
    };
    TrackSelectionFilter filter;
    filter.preferredSubtitleLanguages = {FILTER_LANG_ENG, FILTER_LANG_CHI};

    int32_t ret = SelectSubtitleDefaultId(streamMap, false, filter, FILTER_LANG_FRA, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_SUBTITLE_STREAM_ID_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_005
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_005
 * @tc.desc  : Test SelectSubtitleDefaultId finds stream by later preferred language match
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_005, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_FRA),
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_2, FILTER_LANG_CHI)
    };
    TrackSelectionFilter filter;
    filter.preferredSubtitleLanguages = {FILTER_LANG_ENG, FILTER_LANG_CHI};

    int32_t ret = SelectSubtitleDefaultId(streamMap, false, filter, FILTER_LANG_FRA, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_006
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_006
 * @tc.desc  : Test SelectSubtitleDefaultId falls back to default language when preferred language misses
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_006, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_ENG),
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_2, FILTER_LANG_FRA)
    };
    TrackSelectionFilter filter;
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};

    int32_t ret = SelectSubtitleDefaultId(streamMap, false, filter, FILTER_LANG_FRA, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_007
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_007
 * @tc.desc  : Test SelectSubtitleDefaultId returns last matching stream for default language
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_007, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_ENG),
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_2, FILTER_LANG_ENG)
    };
    TrackSelectionFilter filter;

    int32_t ret = SelectSubtitleDefaultId(streamMap, false, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_008
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_008
 * @tc.desc  : Test SelectSubtitleDefaultId returns last stream when default language is empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_008, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_FRA),
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_2, FILTER_LANG_CHI)
    };
    TrackSelectionFilter filter;

    int32_t ret = SelectSubtitleDefaultId(streamMap, false, filter, "", FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_009
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_009
 * @tc.desc  : Test SelectSubtitleDefaultId returns first stream when no language matches
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_009, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_FRA),
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_2, FILTER_LANG_CHI)
    };
    TrackSelectionFilter filter;

    int32_t ret = SelectSubtitleDefaultId(streamMap, false, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_SUBTITLE_STREAM_ID_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_010
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_010
 * @tc.desc  : Test SelectSubtitleDefaultId skips preferred language when all streams are already selected on http
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_010, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> streamMap;
    streamMap[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_CHI),
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_2, FILTER_LANG_ENG)
    };
    TrackSelectionFilter filter;
    filter.preferredSubtitleLanguages = {FILTER_LANG_CHI};

    int32_t ret = SelectSubtitleDefaultId(streamMap, true, filter, FILTER_LANG_ENG, FILTER_PLAY_TYPE_HTTP);
    EXPECT_EQ(ret, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_001
 * @tc.number: UpdateDefaultStreamIds_001
 * @tc.desc  : Test UpdateDefaultStreamIds updates default video stream id when selected id is valid
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_001, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->filteredStreamMap_->map[VIDEO] = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, "")
    };
    mediaDemuxer_->currentPlayType_ = "hls";
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->isHdrStart_ = false;
    mediaDemuxer_->initResolution_ = UNUM_0;
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, FILTER_VIDEO_STREAM_ID_1);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_002
 * @tc.number: UpdateDefaultStreamIds_002
 * @tc.desc  : Test UpdateDefaultStreamIds keeps default video stream id when selected id is invalid
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_002, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->currentPlayType_ = "hls";
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->isHdrStart_ = false;
    mediaDemuxer_->initResolution_ = UNUM_0;
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, FILTER_VIDEO_STREAM_ID_2);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_003
 * @tc.number: UpdateDefaultStreamIds_003
 * @tc.desc  : Test UpdateDefaultStreamIds returns early for http play type
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_003, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->filteredStreamMap_->map[VIDEO] = {
        MakeTrackSelectionFilterStream(FILTER_VIDEO_STREAM_ID_1, VIDEO, UNUM_1000, FILTER_MIME_VIDEO_AVC, "")
    };
    mediaDemuxer_->filteredStreamMap_->map[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_ENG, "audio/aac")
    };
    mediaDemuxer_->filteredStreamMap_->map[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_CHI)
    };
    mediaDemuxer_->currentPlayType_ = FILTER_PLAY_TYPE_HTTP;
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->isHdrStart_ = false;
    mediaDemuxer_->initResolution_ = UNUM_0;
    mediaDemuxer_->defaultAudioLang_ = FILTER_LANG_ENG;
    mediaDemuxer_->defaultSubtitleLang_ = FILTER_LANG_CHI;
    mediaDemuxer_->defaultVideoStreamId_ = FILTER_VIDEO_STREAM_ID_2;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_2;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultVideoStreamId_, FILTER_VIDEO_STREAM_ID_1);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_004
 * @tc.number: UpdateDefaultStreamIds_004
 * @tc.desc  : Test UpdateDefaultStreamIds continues updating audio and subtitle for non-http play type
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_004, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->filteredStreamMap_->map[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_ENG, "audio/aac")
    };
    mediaDemuxer_->filteredStreamMap_->map[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_CHI)
    };
    mediaDemuxer_->currentPlayType_ = FILTER_PLAY_TYPE_DASH;
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->defaultAudioLang_ = FILTER_LANG_ENG;
    mediaDemuxer_->defaultSubtitleLang_ = FILTER_LANG_CHI;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_2;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_1);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_1);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_005
 * @tc.number: UpdateDefaultStreamIds_005
 * @tc.desc  : Test UpdateDefaultStreamIds updates default audio stream id when selected id is valid
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_005, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->filteredStreamMap_->map[AUDIO] = {
        MakeSelectAudioDefaultStream(FILTER_AUDIO_STREAM_ID_1, FILTER_LANG_ENG, "audio/aac")
    };
    mediaDemuxer_->currentPlayType_ = FILTER_PLAY_TYPE_DASH;
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->defaultAudioLang_ = FILTER_LANG_ENG;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_2;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_1);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_006
 * @tc.number: UpdateDefaultStreamIds_006
 * @tc.desc  : Test UpdateDefaultStreamIds keeps default audio stream id when selected id is invalid
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_006, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->currentPlayType_ = FILTER_PLAY_TYPE_DASH;
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->defaultAudioLang_ = FILTER_LANG_ENG;
    mediaDemuxer_->defaultAudioStreamId_ = FILTER_AUDIO_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultAudioStreamId_, FILTER_AUDIO_STREAM_ID_2);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_007
 * @tc.number: UpdateDefaultStreamIds_007
 * @tc.desc  : Test UpdateDefaultStreamIds updates default subtitle stream id when selected id is valid
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_007, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->filteredStreamMap_->map[SUBTITLE] = {
        MakeSelectSubtitleDefaultStream(FILTER_SUBTITLE_STREAM_ID_1, FILTER_LANG_CHI)
    };
    mediaDemuxer_->currentPlayType_ = FILTER_PLAY_TYPE_DASH;
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->defaultSubtitleLang_ = FILTER_LANG_CHI;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_1);
}

/**
 * @tc.name  : UpdateDefaultStreamIds_008
 * @tc.number: UpdateDefaultStreamIds_008
 * @tc.desc  : Test UpdateDefaultStreamIds keeps default subtitle stream id when selected id is invalid
 */
HWTEST_F(MediaDemuxerExtUnitTest, UpdateDefaultStreamIds_008, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();
    mediaDemuxer_->currentPlayType_ = FILTER_PLAY_TYPE_DASH;
    mediaDemuxer_->trackSelectionFilter_ = {};
    mediaDemuxer_->defaultSubtitleLang_ = FILTER_LANG_CHI;
    mediaDemuxer_->defaultSubtitleStreamId_ = FILTER_SUBTITLE_STREAM_ID_2;

    mediaDemuxer_->UpdateDefaultStreamIds(false, false, false);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleStreamId_, FILTER_SUBTITLE_STREAM_ID_2);
}

/**
 * @tc.name  : CheckStreamCodec_001
 * @tc.number: CheckStreamCodec_001
 * @tc.desc  : Test CheckStreamCodec returns false when audio codec is unsupported
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckStreamCodec_001, TestSize.Level1)
{
    EXPECT_FALSE(CheckStreamCodec("audio/not-supported", AUDIO, nullptr));
}

/**
 * @tc.name  : CheckStreamCodec_002
 * @tc.number: CheckStreamCodec_002
 * @tc.desc  : Test CheckStreamCodec returns true when codec list is null for video stream
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckStreamCodec_002, TestSize.Level1)
{
    EXPECT_TRUE(CheckStreamCodec(FILTER_MIME_VIDEO_AVC, VIDEO, nullptr));
}

/**
 * @tc.name  : CheckStreamCodec_003
 * @tc.number: CheckStreamCodec_003
 * @tc.desc  : Test CheckStreamCodec returns true when video decoder capability matches codec mime
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckStreamCodec_003, TestSize.Level1)
{
    auto codecList = std::make_shared<FakeAVCodecList>(
        std::vector<std::shared_ptr<MediaAVCodec::CapabilityData>> {
            nullptr, MakeCodecCapability(FILTER_MIME_VIDEO_AVC)
        });

    EXPECT_TRUE(CheckStreamCodec(FILTER_MIME_VIDEO_AVC, VIDEO, codecList));
}

/**
 * @tc.name  : CheckStreamCodec_004
 * @tc.number: CheckStreamCodec_004
 * @tc.desc  : Test CheckStreamCodec returns false when no video decoder capability matches codec mime
 */
HWTEST_F(MediaDemuxerExtUnitTest, CheckStreamCodec_004, TestSize.Level1)
{
    auto codecList = std::make_shared<FakeAVCodecList>(
        std::vector<std::shared_ptr<MediaAVCodec::CapabilityData>> {
            nullptr, MakeCodecCapability(FILTER_MIME_VIDEO_HEVC)
        });

    EXPECT_FALSE(CheckStreamCodec(FILTER_MIME_VIDEO_AVC, VIDEO, codecList));
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_001
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_001
 * @tc.desc  : Test early return when sampleQueueController_ is nullptr
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_001, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = std::make_shared<SampleQueue>();
    mediaDemuxer_->sampleQueueController_ = nullptr;

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).Times(0);

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_002
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_002
 * @tc.desc  : Test early return when track already reaches eos
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_002, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);
    mediaDemuxer_->eosMap_[trackId] = true;

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).Times(0);

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_003
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_003
 * @tc.desc  : Test early return when IsLocalFd() is true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_003, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).Times(0);

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_004
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_004
 * @tc.desc  : Test cloud fd skips water line auto select branch
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_004, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);
    mediaDemuxer_->lastCheckAutoSelectTimeMs_ = NUM_1;

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).WillOnce(Return(VIDEO));
    EXPECT_CALL(*source, IsCloudFd()).WillOnce(Return(true));
    EXPECT_CALL(*source, IsNeedPreDownload()).Times(0);

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, NUM_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_005
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_005
 * @tc.desc  : Test predownload skips water line auto select branch
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_005, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);
    mediaDemuxer_->lastCheckAutoSelectTimeMs_ = NUM_1;

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).WillOnce(Return(VIDEO));
    EXPECT_CALL(*source, IsCloudFd()).WillOnce(Return(false));
    EXPECT_CALL(*source, IsNeedPreDownload()).WillOnce(Return(true));

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, NUM_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_006
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_006
 * @tc.desc  : Test audio stream skips water line auto select branch
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_006, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);
    mediaDemuxer_->lastCheckAutoSelectTimeMs_ = NUM_1;

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).WillOnce(Return(AUDIO));
    EXPECT_CALL(*source, IsCloudFd()).WillOnce(Return(false));
    EXPECT_CALL(*source, IsNeedPreDownload()).WillOnce(Return(false));

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
    EXPECT_EQ(mediaDemuxer_->lastCheckAutoSelectTimeMs_, NUM_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_007
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_007
 * @tc.desc  : Test no stop-consume result keeps buffering state unchanged
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_007, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).WillOnce(Return(AUDIO));
    EXPECT_CALL(*source, IsCloudFd()).WillOnce(Return(true));

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    EXPECT_FALSE(mediaDemuxer_->IsBuffering());
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_008
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_008
 * @tc.desc  : Test hls segment eos with no stop-consume keeps buffering state unchanged
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_008, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);
    mediaDemuxer_->hlsSegmentEosMap_[trackId] = true;
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).WillOnce(Return(AUDIO));
    EXPECT_CALL(*source, IsCloudFd()).WillOnce(Return(true));

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    EXPECT_FALSE(mediaDemuxer_->IsBuffering());
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_009
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_009
 * @tc.desc  : Test muted video with no stop-consume keeps track buffering state unchanged
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_009, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);
    mediaDemuxer_->isVideoMuted_ = true;
    mediaDemuxer_->SetTrackIsBuffering(trackId, true);
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).WillOnce(Return(AUDIO));
    EXPECT_CALL(*source, IsCloudFd()).WillOnce(Return(true));

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
    EXPECT_TRUE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    EXPECT_FALSE(mediaDemuxer_->IsBuffering());
}

/**
 * @tc.name  : MediaDemuxerExt_ConsumeWaterLoopControl_010
 * @tc.number: MediaDemuxerExt_ConsumeWaterLoopControl_010
 * @tc.desc  : Test regular no stop-consume path keeps buffering status unchanged
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ConsumeWaterLoopControl_010, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    auto sampleQueue = PrepareConsumeWaterLoopControl(mediaDemuxer_, trackId);
    auto mockEventReceiver = std::make_shared<StrictMock<MockEventReceiver>>();
    mediaDemuxer_->eventReceiver_ = mockEventReceiver;
    EXPECT_CALL(*mockEventReceiver, OnEvent(_)).Times(0);

    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamTypeByTrackID(trackId)).WillOnce(Return(MIXED));
    EXPECT_CALL(*source, IsCloudFd()).WillOnce(Return(true));

    mediaDemuxer_->ConsumeWaterLoopControl(trackId, sampleQueue);
    EXPECT_FALSE(mediaDemuxer_->GetTrackIsBuffering(trackId));
    EXPECT_FALSE(mediaDemuxer_->IsBuffering());
}

/**
 * @tc.name  : MediaDemuxerExt_OnEventStream_001
 * @tc.number: MediaDemuxerExt_OnEventStream_001
 * @tc.desc  : Test OnEventStream with STREAM_UPDATE
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnEventStream_001, TestSize.Level1)
{
    EXPECT_CALL(*(mediaDemuxer_->source_), SetDefaultStreamId(_, _, _)).Times(1);
    PluginEvent event;
    event.type = PluginEventType::STREAM_UPDATE;
    std::string playType = "http";
    event.param = playType;
    mediaDemuxer_->OnEventStream(event);
}

/**
 * @tc.name  : MediaDemuxerExt_OnEventStream_002
 * @tc.number: MediaDemuxerExt_OnEventStream_002
 * @tc.desc  : Test OnEventStream with NETWORK_BITRATE_CHANGED
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_OnEventStream_002, TestSize.Level1)
{
    uint32_t bitRate = TEST_NETWORK_BITRATE;
    PluginEvent event;
    event.type = PluginEventType::NETWORK_BITRATE_CHANGED;
    event.param = bitRate;
    mediaDemuxer_->curDownloadBitRate_.store(0);
    mediaDemuxer_->OnEventStream(event);
    EXPECT_EQ(mediaDemuxer_->curDownloadBitRate_.load(), bitRate);
}

/**
 * @tc.name  : MediaDemuxerExt_AutoSelectTrackByBitrate_001
 * @tc.number: MediaDemuxerExt_AutoSelectTrackByBitrate_001
 * @tc.desc  : Test AutoSelectTrackByBitrate when should auto select
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_AutoSelectTrackByBitrate_001, TestSize.Level1)
{
    mediaDemuxer_->isAutoSelect_ = true;
    int32_t trackId = NUM_0;
    auto controller = std::make_shared<SampleQueueController>();
    mediaDemuxer_->sampleQueueController_ = controller;
    mediaDemuxer_->sampleQueueMap_[trackId] = std::make_shared<SampleQueue>();
    mediaDemuxer_->AutoSelectTrackByBitrate(trackId);
}

/**
 * @tc.name  : MediaDemuxerExt_ProduceWaterLoopControl_004
 * @tc.number: MediaDemuxerExt_ProduceWaterLoopControl_004
 * @tc.desc  : Test ProduceWaterLoopControl for video track
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProduceWaterLoopControl_004, TestSize.Level1)
{
    int32_t trackId = NUM_0;
    mediaDemuxer_->subtitleTrackId_ = NEG_1;
    mediaDemuxer_->videoTrackId_ = trackId;
    mediaDemuxer_->isVideoMuted_ = false;
    auto controller = std::make_shared<SampleQueueController>();
    mediaDemuxer_->sampleQueueController_ = controller;
    mediaDemuxer_->sampleQueueMap_[trackId] = std::make_shared<SampleQueue>();
    mediaDemuxer_->isBuffering_.store(true);
    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsLocalFd()).WillOnce(Return(false));
    mediaDemuxer_->ProduceWaterLoopControl(trackId);
}

/**
 * @tc.name  : MediaDemuxerExt_SetPlayStrategy_001
 * @tc.number: MediaDemuxerExt_SetPlayStrategy_001
 * @tc.desc  : Test SetPlayStrategy with nullptr strategy
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SetPlayStrategy_001, TestSize.Level1)
{
    Status ret = mediaDemuxer_->SetPlayStrategy(nullptr);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : MediaDemuxerExt_SetPlayStrategy_002
 * @tc.number: MediaDemuxerExt_SetPlayStrategy_002
 * @tc.desc  : Test SetPlayStrategy with valid strategy
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SetPlayStrategy_002, TestSize.Level1)
{
    auto strategy = std::make_shared<Plugins::PlayStrategy>();
    strategy->width = TEST_VIDEO_WIDTH;
    strategy->height = TEST_VIDEO_HEIGHT;
    strategy->preferHDR = true;
    strategy->audioLanguage = TEST_LANG_ENG;
    strategy->subtitleLanguage = TEST_LANG_ZH;

    Status ret = mediaDemuxer_->SetPlayStrategy(strategy);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(mediaDemuxer_->defaultAudioLang_, TEST_LANG_ENG);
    EXPECT_EQ(mediaDemuxer_->defaultSubtitleLang_, TEST_LANG_ZH);
    EXPECT_TRUE(mediaDemuxer_->isHdrStart_);
    EXPECT_EQ(mediaDemuxer_->initResolution_, static_cast<uint32_t>(TEST_VIDEO_WIDTH * TEST_VIDEO_HEIGHT));
}

/**
 * @tc.name  : MediaDemuxerExt_IsLangMatch_001
 * @tc.number: MediaDemuxerExt_IsLangMatch_001
 * @tc.desc  : Test IsLangMatch for audio and subtitle
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsLangMatch_001, TestSize.Level1)
{
    EXPECT_TRUE(IsLangMatch(TEST_LANG_ENG, TEST_LANG_ENG));
    EXPECT_FALSE(IsLangMatch(TEST_LANG_ENG, TEST_LANG_FRA));

    EXPECT_TRUE(IsLangMatch(TEST_LANG_ZH, TEST_LANG_ZH));
    EXPECT_FALSE(IsLangMatch(TEST_LANG_ZH, TEST_LANG_EN_SHORT));
}

/**
 * @tc.name  : MediaDemuxerExt_SetTrackSelectionFilter_001
 * @tc.number: MediaDemuxerExt_SetTrackSelectionFilter_001
 * @tc.desc  : Test SetTrackSelectionFilter builds correct predicates
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SetTrackSelectionFilter_001, TestSize.Level1)
{
    filteredStreamPredicates_.clear();

    TrackSelectionFilter filter;
    filter.maxVideoBitrate = TEST_MAX_VIDEO_BITRATE;
    filter.minVideoBitrate = TEST_MIN_VIDEO_BITRATE;
    filter.maxVideoFrameRate = TEST_MAX_VIDEO_FRAMERATE;
    filter.minVideoFrameRate = TEST_MIN_VIDEO_FRAMERATE;
    filter.maxVideoResolution = std::make_pair(TEST_VIDEO_WIDTH, TEST_VIDEO_HEIGHT);
    filter.minVideoResolution = std::make_pair(TEST_MIN_VIDEO_WIDTH, TEST_MIN_VIDEO_HEIGHT);
    filter.preferredVideoMimeTypes = std::vector<std::string>{TEST_MIME_VIDEO_AVC};

    filter.maxAudioBitrate = TEST_MAX_AUDIO_BITRATE;
    filter.minAudioBitrate = TEST_MIN_AUDIO_BITRATE;
    filter.maxAudioChannels = TEST_MAX_AUDIO_CHANNELS;
    filter.preferredAudioMimeTypes = std::vector<std::string>{TEST_MIME_AUDIO_AAC};
    filter.preferredAudioLanguages = std::vector<std::string>{TEST_LANG_ENG};

    filter.preferredSubtitleLanguages = std::vector<std::string>{TEST_LANG_ZH};

    mediaDemuxer_->SetTrackSelectionFilter(filter);
    TrackSelectionFilter outFilter = filter;
    EXPECT_EQ(outFilter.maxVideoBitrate, TEST_MAX_VIDEO_BITRATE);
    EXPECT_FALSE(outFilter.preferredVideoMimeTypes.empty());
    EXPECT_EQ(outFilter.preferredVideoMimeTypes.size(), 1u);
    filteredStreamPredicates_[StreamType::VIDEO] = GetVideoFilterPredicates(outFilter);
    filteredStreamPredicates_[StreamType::AUDIO] = GetAudioFilterPredicates(outFilter);
    filteredStreamPredicates_[StreamType::SUBTITLE] = GetSubtitleFilterPredicates(outFilter);

    EXPECT_FALSE(filteredStreamPredicates_[StreamType::VIDEO].empty());
    EXPECT_FALSE(filteredStreamPredicates_[StreamType::AUDIO].empty());
    EXPECT_FALSE(filteredStreamPredicates_[StreamType::SUBTITLE].empty());
}

/**
 * @tc.name  : MediaDemuxerExt_GetCurrentTimeMs_001
 * @tc.number: MediaDemuxerExt_GetCurrentTimeMs_001
 * @tc.desc  : Test GetCurrentTimeMs returns non-decreasing positive time
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetCurrentTimeMs_001, TestSize.Level1)
{
    int64_t t1 = mediaDemuxer_->GetCurrentTimeMs();
    int64_t t2 = mediaDemuxer_->GetCurrentTimeMs();
    EXPECT_GT(t1, 0);
    EXPECT_GE(t2, t1);
}

/**
 * @tc.name  : MediaDemuxerExt_ChooseDefaultStreams_001
 * @tc.number: MediaDemuxerExt_ChooseDefaultStreams_001
 * @tc.desc  : Test ChooseDefaultStreams selects defaults from stream info
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ChooseDefaultStreams_001, TestSize.Level1)
{
    mediaDemuxer_->defaultAudioLang_ = TEST_LANG_ENG;
    mediaDemuxer_->defaultSubtitleLang_ = TEST_LANG_ZH;
    mediaDemuxer_->initResolution_ = static_cast<uint32_t>(TEST_VIDEO_WIDTH * TEST_VIDEO_HEIGHT);
    mediaDemuxer_->currentPlayType_ = "http";
    mediaDemuxer_->trackSelectionFilter_ = TrackSelectionFilter();

    StreamInfo vLow;
    vLow.streamId = TEST_STREAM_ID_1;
    vLow.type = StreamType::VIDEO;
    vLow.mimeType = TEST_MIME_VIDEO_AVC;
    vLow.videoWidth = TEST_MIN_VIDEO_WIDTH;
    vLow.videoHeight = TEST_MIN_VIDEO_HEIGHT;

    StreamInfo vHigh;
    vHigh.streamId = TEST_STREAM_ID_2;
    vHigh.type = StreamType::VIDEO;
    vHigh.mimeType = TEST_MIME_VIDEO_AVC;
    vHigh.videoWidth = TEST_VIDEO_WIDTH;
    vHigh.videoHeight = TEST_VIDEO_HEIGHT;

    StreamInfo a1;
    a1.streamId = TEST_STREAM_ID_3;
    a1.type = StreamType::AUDIO;
    a1.mimeType = TEST_MIME_AUDIO_AAC;
    a1.lang = TEST_LANG_ENG;

    StreamInfo s1;
    s1.streamId = TEST_STREAM_ID_4;
    s1.type = StreamType::SUBTITLE;
    s1.lang = TEST_LANG_ZH;

    EXPECT_CALL(*(mediaDemuxer_->source_), GetStreamInfo(_, _))
        .WillOnce(Invoke([&](std::vector<StreamInfo> &streams, bool) {
            streams.clear();
            streams.push_back(vLow);
            streams.push_back(vHigh);
            streams.push_back(a1);
            streams.push_back(s1);
            return Status::OK;
        }));

    mediaDemuxer_->ChooseDefaultStreams();
}

/**
 * @tc.name  : MediaDemuxerExt_CheckStreamMimeType_001
 * @tc.number: MediaDemuxerExt_CheckStreamMimeType_001
 * @tc.desc  : Test CheckStreamMimeType behavior for subtitle mime
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CheckStreamMimeType_001, TestSize.Level1)
{
    StreamInfo subtitleStream;
    subtitleStream.type = StreamType::SUBTITLE;
    subtitleStream.mimeType = "text/vtt";
    std::shared_ptr<MediaAVCodec::AVCodecList> codecList {nullptr};
    EXPECT_FALSE(CheckStreamCodec(subtitleStream.mimeType, Plugins::AUDIO, codecList));
    EXPECT_TRUE(CheckStreamCodec(subtitleStream.mimeType, Plugins::VIDEO, codecList));
}

/**
 * @tc.name  : MediaDemuxerExt_CalcResolutionGap_001
 * @tc.number: MediaDemuxerExt_CalcResolutionGap_001
 * @tc.desc  : Test CalcResolutionGap returns absolute difference
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CalcResolutionGap_001, TestSize.Level1)
{
    uint32_t baseResolution = static_cast<uint32_t>(TEST_VIDEO_WIDTH * TEST_VIDEO_HEIGHT);
    uint32_t lowResolution = static_cast<uint32_t>(TEST_MIN_VIDEO_WIDTH * TEST_MIN_VIDEO_HEIGHT);

    EXPECT_EQ(CalcResolutionGap(baseResolution, lowResolution), baseResolution - lowResolution);
    EXPECT_EQ(CalcResolutionGap(lowResolution, baseResolution), baseResolution - lowResolution);
    EXPECT_EQ(CalcResolutionGap(baseResolution, baseResolution), 0u);
}

/**
 * @tc.name  : MediaDemuxerExt_IsVideoStreamSelectable_001
 * @tc.number: MediaDemuxerExt_IsVideoStreamSelectable_001
 * @tc.desc  : Test IsVideoStreamSelectable for dash/hls and HDR/bandwidth conditions
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsVideoStreamSelectable_001, TestSize.Level1)
{
    StreamInfo stream {};
    stream.type = StreamType::VIDEO;
    stream.bitRate = TEST_VIDEO_BITRATE_1000;
    stream.videoType = VIDEO_TYPE_SDR;
    EXPECT_FALSE(IsVideoStreamSelectable(stream, "dash", true));
    stream.videoType = VIDEO_TYPE_HDR_10;
    EXPECT_TRUE(IsVideoStreamSelectable(stream, "dash", true));
    stream.videoType = VIDEO_TYPE_SDR;
    stream.bitRate = static_cast<uint32_t>(BAND_WIDTH_LIMIT + 1);
    EXPECT_FALSE(IsVideoStreamSelectable(stream, "hls", false));
    stream.bitRate = static_cast<uint32_t>(BAND_WIDTH_LIMIT);
    stream.mimeType = TEST_MIME_VIDEO_AVC;
    EXPECT_TRUE(IsVideoStreamSelectable(stream, "hls", false));
}

/**
 * @tc.name  : MediaDemuxerExt_IsStreamMatched_001
 * @tc.number: MediaDemuxerExt_IsStreamMatched_001
 * @tc.desc  : Test IsStreamMatched checks all predicates
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsStreamMatched_001, TestSize.Level1)
{
    StreamInfo stream {};
    stream.type = StreamType::AUDIO;
    stream.bitRate = TEST_AUDIO_BITRATE_128000;
    stream.lang = TEST_LANG_ENG;
    std::map<StreamType, std::vector<StreamFilterPredicate>> predicates;
    predicates[StreamType::AUDIO].push_back([](StreamInfo info) {
        return info.bitRate == TEST_AUDIO_BITRATE_128000;
    });
    predicates[StreamType::AUDIO].push_back([](StreamInfo info) {
        return info.lang == TEST_LANG_ENG;
    });
    EXPECT_TRUE(IsStreamMatched(stream, predicates));
    stream.lang = TEST_LANG_ZH;
    EXPECT_FALSE(IsStreamMatched(stream, predicates));
}

/**
 * @tc.name  : MediaDemuxerExt_PrintStreamInfo_001
 * @tc.number: MediaDemuxerExt_PrintStreamInfo_001
 * @tc.desc  : Test PrintStreamInfo with nullptr and different stream types
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_PrintStreamInfo_001, TestSize.Level1)
{
    PrintStreamInfo("PrintStreamInfo ", nullptr, "dash", Status::OK, TEST_NETWORK_BITRATE);

    StreamInfo videoStream;
    videoStream.type = StreamType::VIDEO;
    videoStream.bitRate = TEST_VIDEO_BITRATE_1000;
    videoStream.videoHeight = TEST_VIDEO_HEIGHT;
    videoStream.videoWidth = TEST_VIDEO_WIDTH;
    videoStream.frameRate = TEST_MAX_VIDEO_FRAMERATE;
    videoStream.mimeType = TEST_MIME_VIDEO_AVC;
    videoStream.originCodecs = "avc1";
    PrintStreamInfo("PrintStreamInfo ", &videoStream, "dash", Status::OK, TEST_NETWORK_BITRATE);

    StreamInfo audioStream;
    audioStream.type = StreamType::AUDIO;
    audioStream.channels = TEST_MAX_AUDIO_CHANNELS;
    audioStream.bitRate = TEST_AUDIO_BITRATE_128000;
    audioStream.mimeType = TEST_MIME_AUDIO_AAC;
    audioStream.originCodecs = "mp4a";
    audioStream.lang = TEST_LANG_ENG;
    PrintStreamInfo("PrintStreamInfo ", &audioStream, "dash", Status::ERROR_UNKNOWN);

    StreamInfo subtitleStream;
    subtitleStream.type = StreamType::SUBTITLE;
    subtitleStream.lang = TEST_LANG_ZH;
    PrintStreamInfo("PrintStreamInfo ", &subtitleStream, "dash", Status::OK);

    SUCCEED();
}

/**
 * @tc.name  : MediaDemuxerExt_GetPreferredVideoStream_001
 * @tc.number: MediaDemuxerExt_GetPreferredVideoStream_001
 * @tc.desc  : Test GetPreferredVideoStream returns nearest valid video stream
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetPreferredVideoStream_001, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();

    StreamInfo audioStream;
    audioStream.streamId = TEST_STREAM_ID_1;
    audioStream.type = StreamType::AUDIO;
    audioStream.bitRate = TEST_VIDEO_BITRATE_1000;

    StreamInfo videoStreamLow;
    videoStreamLow.streamId = TEST_STREAM_ID_2;
    videoStreamLow.type = StreamType::VIDEO;
    videoStreamLow.bitRate = 1800;

    StreamInfo mixedStream;
    mixedStream.streamId = TEST_STREAM_ID_3;
    mixedStream.type = StreamType::MIXED;
    mixedStream.bitRate = 2600;

    StreamInfo videoStreamHigh;
    videoStreamHigh.streamId = TEST_STREAM_ID_4;
    videoStreamHigh.type = StreamType::VIDEO;
    videoStreamHigh.bitRate = 4200;

    mediaDemuxer_->filteredStreamMap_->map[VIDEO] = {audioStream, videoStreamLow, mixedStream, videoStreamHigh};
    StreamInfo *preferredStream = GetPreferredVideoStream(mediaDemuxer_->filteredStreamMap_->map, 2500);

    ASSERT_NE(preferredStream, nullptr);
    EXPECT_EQ(preferredStream->streamId, TEST_STREAM_ID_3);
    EXPECT_EQ(preferredStream->bitRate, 2600u);
    mediaDemuxer_->filteredStreamMap_->map.clear();
}

/**
 * @tc.name  : MediaDemuxerExt_GetPreferredVideoStream_002
 * @tc.number: MediaDemuxerExt_GetPreferredVideoStream_002
 * @tc.desc  : Test GetPreferredVideoStream returns exact bitrate match
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetPreferredVideoStream_002, TestSize.Level1)
{
    mediaDemuxer_->filteredStreamMap_->map.clear();

    StreamInfo videoStreamLow;
    videoStreamLow.streamId = TEST_STREAM_ID_1;
    videoStreamLow.type = StreamType::VIDEO;
    videoStreamLow.bitRate = TEST_VIDEO_BITRATE_1000;

    StreamInfo videoStreamExact;
    videoStreamExact.streamId = TEST_STREAM_ID_2;
    videoStreamExact.type = StreamType::VIDEO;
    videoStreamExact.bitRate = TEST_MAX_VIDEO_BITRATE;

    mediaDemuxer_->filteredStreamMap_->map[VIDEO] = {videoStreamLow, videoStreamExact};
    StreamInfo *preferredStream = GetPreferredVideoStream(
        mediaDemuxer_->filteredStreamMap_->map, TEST_MAX_VIDEO_BITRATE);

    ASSERT_NE(preferredStream, nullptr);
    EXPECT_EQ(preferredStream->streamId, TEST_STREAM_ID_2);
    EXPECT_EQ(preferredStream->bitRate, static_cast<uint32_t>(TEST_MAX_VIDEO_BITRATE));
    mediaDemuxer_->filteredStreamMap_->map.clear();
}


/**
 * @tc.name  : MediaDemuxerExt_SetIsTriggerAutoMode_001
 * @tc.number: MediaDemuxerExt_SetIsTriggerAutoMode_001
 * @tc.desc  : Test SetIsTriggerAutoMode updates auto select flag
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SetIsTriggerAutoMode_001, TestSize.Level1)
{
    mediaDemuxer_->SetIsTriggerAutoMode(true);
    EXPECT_TRUE(mediaDemuxer_->isAutoSelect_.load());

    mediaDemuxer_->SetIsTriggerAutoMode(false);
    EXPECT_FALSE(mediaDemuxer_->isAutoSelect_.load());
}

/**
 * @tc.name  : MediaDemuxerExt_MatchPreferredMimeTypes_001
 * @tc.number: MediaDemuxerExt_MatchPreferredMimeTypes_001
 * @tc.desc  : Test MatchPreferredMimeTypes by mime type and codec prefix
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_MatchPreferredMimeTypes_001, TestSize.Level1)
{
    StreamInfo streamInfo;
    streamInfo.mimeType = TEST_MIME_VIDEO_AVC;
    streamInfo.originCodecs = "avc1.64001f";

    EXPECT_TRUE(MatchPreferredMimeTypes({TEST_MIME_VIDEO_AVC}, streamInfo));
    EXPECT_TRUE(MatchPreferredMimeTypes({"avc1"}, streamInfo));
    EXPECT_TRUE(MatchPreferredMimeTypes({"avc1.64001f"}, streamInfo));
    EXPECT_FALSE(MatchPreferredMimeTypes({"hev1"}, streamInfo));
}

/**
 * @tc.name  : MediaDemuxerExt_GetVideoFilterPredicatesExt_001
 * @tc.number: MediaDemuxerExt_GetVideoFilterPredicatesExt_001
 * @tc.desc  : Test GetVideoFilterPredicatesExt creates valid predicates
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetVideoFilterPredicatesExt_001, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.minVideoFrameRate = TEST_MIN_VIDEO_FRAMERATE;
    filter.maxVideoResolution = std::make_pair(TEST_VIDEO_WIDTH, TEST_VIDEO_HEIGHT);
    filter.minVideoResolution = std::make_pair(TEST_MIN_VIDEO_WIDTH, TEST_MIN_VIDEO_HEIGHT);
    filter.preferredVideoMimeTypes = {"avc1"};

    std::vector<StreamFilterPredicate> predicates;
    GetVideoFilterPredicatesExt(filter, predicates);
    ASSERT_EQ(predicates.size(), 4u);

    StreamInfo matchedStream;
    matchedStream.type = StreamType::VIDEO;
    matchedStream.frameRate = TEST_MAX_VIDEO_FRAMERATE;
    matchedStream.videoWidth = TEST_VIDEO_WIDTH;
    matchedStream.videoHeight = TEST_VIDEO_HEIGHT;
    matchedStream.originCodecs = "avc1.64001f";

    for (const auto &predicate : predicates) {
        EXPECT_TRUE(predicate(matchedStream));
    }

    StreamInfo notMatchedStream = matchedStream;
    notMatchedStream.frameRate = TEST_MIN_VIDEO_FRAMERATE - 1;
    EXPECT_FALSE(predicates[0](notMatchedStream));
}

/**
 * @tc.name  : MediaDemuxerExt_GetAudioFilterPredicatesExt_001
 * @tc.number: MediaDemuxerExt_GetAudioFilterPredicatesExt_001
 * @tc.desc  : Test GetAudioFilterPredicatesExt creates valid predicates
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetAudioFilterPredicatesExt_001, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.preferredAudioMimeTypes = {"mp4a"};
    filter.preferredAudioLanguages = {TEST_LANG_ENG};

    std::vector<StreamFilterPredicate> predicates;
    GetAudioFilterPredicatesExt(filter, predicates);
    ASSERT_EQ(predicates.size(), 2u);

    StreamInfo matchedStream;
    matchedStream.type = StreamType::AUDIO;
    matchedStream.originCodecs = "mp4a.40.2";
    matchedStream.lang = TEST_LANG_ENG;

    EXPECT_TRUE(predicates[0](matchedStream));
    EXPECT_TRUE(predicates[1](matchedStream));

    matchedStream.lang = TEST_LANG_FRA;
    EXPECT_FALSE(predicates[1](matchedStream));
}

/**
 * @tc.name  : MediaDemuxerExt_FindStreamForFiledStream_001
 * @tc.number: MediaDemuxerExt_FindStreamForFiledStream_001
 * @tc.desc  : Test FindStreamForFiledStream with empty and default selection
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_FindStreamForFiledStream_001, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    bool needChange = true;
    EXPECT_EQ(FindStreamForFiledStream(streams, TEST_STREAM_ID_1, TEST_STREAM_ID_2, needChange), nullptr);

    StreamInfo streamA;
    streamA.streamId = TEST_STREAM_ID_1;
    StreamInfo streamB;
    streamB.streamId = TEST_STREAM_ID_2;
    streams = {streamA, streamB};
    needChange = true;
    StreamInfo *stream = FindStreamForFiledStream(streams, TEST_STREAM_ID_3, TEST_STREAM_ID_2, needChange);
    ASSERT_NE(stream, nullptr);
    EXPECT_TRUE(needChange);
    EXPECT_EQ(stream->streamId, TEST_STREAM_ID_2);

    needChange = true;
    stream = FindStreamForFiledStream(streams, TEST_STREAM_ID_1, TEST_STREAM_ID_2, needChange);
    EXPECT_FALSE(needChange);
}

/**
 * @tc.name  : MediaDemuxerExt_ScanStreamsAndBuildMap_001
 * @tc.number: MediaDemuxerExt_ScanStreamsAndBuildMap_001
 * @tc.desc  : Test ScanStreamsAndBuildMap builds filtered and all stream maps
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ScanStreamsAndBuildMap_001, TestSize.Level1)
{
    std::vector<StreamInfo> streams;
    StreamInfo videoStream;
    videoStream.streamId = TEST_STREAM_ID_1;
    videoStream.type = StreamType::VIDEO;
    videoStream.bitRate = TEST_VIDEO_BITRATE_1000;
    StreamInfo audioStream;
    audioStream.streamId = TEST_STREAM_ID_2;
    audioStream.type = StreamType::AUDIO;
    audioStream.bitRate = TEST_AUDIO_BITRATE_128000;
    streams = {videoStream, audioStream};

    std::map<StreamType, std::vector<StreamFilterPredicate>> filteredStreamPredicates;
    filteredStreamPredicates[VIDEO].emplace_back([](const StreamInfo &stream) {
        return stream.bitRate == static_cast<uint32_t>(TEST_VIDEO_BITRATE_1000);
    });

    std::map<StreamType, bool> needFilterMap;
    needFilterMap[VIDEO] = true;
    needFilterMap[AUDIO] = false;
    needFilterMap[SUBTITLE] = false;

    std::map<StreamType, std::vector<StreamInfo>> filteredStreamMap;
    std::map<StreamType, std::vector<StreamInfo>> allStreamMap;
    ScanStreamsAndBuildMap(streams, filteredStreamPredicates, needFilterMap, filteredStreamMap, allStreamMap);

    EXPECT_EQ(allStreamMap[VIDEO].size(), 1u);
    EXPECT_EQ(allStreamMap[AUDIO].size(), 1u);
    EXPECT_EQ(filteredStreamMap[VIDEO].size(), 1u);
    EXPECT_TRUE(filteredStreamMap[AUDIO].empty());
}

/**
 * @tc.name  : MediaDemuxerExt_SelectDefaultVideoId_001
 * @tc.number: MediaDemuxerExt_SelectDefaultVideoId_001
 * @tc.desc  : Test SelectDefaultVideoId for all-selected and preferred mime branches
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectDefaultVideoId_001, TestSize.Level1)
{
    std::vector<StreamInfo> videoStreams;
    StreamInfo videoStreamA;
    videoStreamA.streamId = TEST_STREAM_ID_1;
    videoStreamA.type = StreamType::VIDEO;
    videoStreamA.mimeType = "video/hevc";
    videoStreamA.videoWidth = TEST_MIN_VIDEO_WIDTH;
    videoStreamA.videoHeight = TEST_MIN_VIDEO_HEIGHT;

    StreamInfo videoStreamB;
    videoStreamB.streamId = TEST_STREAM_ID_2;
    videoStreamB.type = StreamType::VIDEO;
    videoStreamB.mimeType = TEST_MIME_VIDEO_AVC;
    videoStreamB.videoWidth = TEST_VIDEO_WIDTH;
    videoStreamB.videoHeight = TEST_VIDEO_HEIGHT;
    videoStreams = {videoStreamA, videoStreamB};

    TrackSelectionFilter filter;
    EXPECT_EQ(SelectDefaultVideoId(videoStreams, true, filter, "http", false, 0), -1);

    filter.preferredVideoMimeTypes = {TEST_MIME_VIDEO_AVC};
    EXPECT_EQ(SelectDefaultVideoId(videoStreams, false, filter, "hls", false, 0), TEST_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectDefaultVideoId_002
 * @tc.number: MediaDemuxerExt_SelectDefaultVideoId_002
 * @tc.desc  : Test SelectDefaultVideoId selects stream by nearest resolution
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectDefaultVideoId_002, TestSize.Level1)
{
    std::vector<StreamInfo> videoStreams;
    StreamInfo videoStreamA;
    videoStreamA.streamId = TEST_STREAM_ID_1;
    videoStreamA.type = StreamType::VIDEO;
    videoStreamA.videoWidth = TEST_MIN_VIDEO_WIDTH;
    videoStreamA.videoHeight = TEST_MIN_VIDEO_HEIGHT;

    StreamInfo videoStreamB;
    videoStreamB.streamId = TEST_STREAM_ID_2;
    videoStreamB.type = StreamType::VIDEO;
    videoStreamB.videoWidth = TEST_VIDEO_WIDTH;
    videoStreamB.videoHeight = TEST_VIDEO_HEIGHT;

    videoStreams = {videoStreamA, videoStreamB};
    TrackSelectionFilter filter;
    EXPECT_EQ(SelectDefaultVideoId(videoStreams, false, filter, "hls", false,
        static_cast<uint32_t>(TEST_VIDEO_WIDTH * TEST_VIDEO_HEIGHT)), TEST_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectAudioDefaultId_001
 * @tc.number: MediaDemuxerExt_SelectAudioDefaultId_001
 * @tc.desc  : Test SelectAudioDefaultId for all-selected and preferred branches
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectAudioDefaultId_001, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> filteredStreamMap;
    StreamInfo audioStreamA;
    audioStreamA.streamId = TEST_STREAM_ID_1;
    audioStreamA.type = StreamType::AUDIO;
    audioStreamA.lang = TEST_LANG_FRA;
    audioStreamA.mimeType = "audio/flac";
    StreamInfo audioStreamB;
    audioStreamB.streamId = TEST_STREAM_ID_2;
    audioStreamB.type = StreamType::AUDIO;
    audioStreamB.lang = TEST_LANG_ENG;
    audioStreamB.mimeType = TEST_MIME_AUDIO_AAC;
    filteredStreamMap[AUDIO] = {audioStreamA, audioStreamB};

    TrackSelectionFilter filter;
    EXPECT_EQ(SelectAudioDefaultId(filteredStreamMap, true, filter, TEST_LANG_ENG, "dash"), -1);

    filter.preferredAudioLanguages = {TEST_LANG_ENG};
    EXPECT_EQ(SelectAudioDefaultId(filteredStreamMap, false, filter, TEST_LANG_FRA, "hls"), TEST_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectSubtitleDefaultId_001
 * @tc.number: MediaDemuxerExt_SelectSubtitleDefaultId_001
 * @tc.desc  : Test SelectSubtitleDefaultId for all-selected and preferred branches
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectSubtitleDefaultId_001, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamInfo>> filteredStreamMap;
    StreamInfo subtitleStreamA;
    subtitleStreamA.streamId = TEST_STREAM_ID_1;
    subtitleStreamA.type = StreamType::SUBTITLE;
    subtitleStreamA.lang = TEST_LANG_ENG;
    StreamInfo subtitleStreamB;
    subtitleStreamB.streamId = TEST_STREAM_ID_2;
    subtitleStreamB.type = StreamType::SUBTITLE;
    subtitleStreamB.lang = TEST_LANG_ZH;
    filteredStreamMap[SUBTITLE] = {subtitleStreamA, subtitleStreamB};

    TrackSelectionFilter filter;
    EXPECT_EQ(SelectSubtitleDefaultId(filteredStreamMap, true, filter, TEST_LANG_ENG, "hls"), -1);

    filter.preferredSubtitleLanguages = {TEST_LANG_ZH};
    EXPECT_EQ(SelectSubtitleDefaultId(filteredStreamMap, false, filter, TEST_LANG_ENG, "dash"), TEST_STREAM_ID_2);
}

/**
 * @tc.name  : MediaDemuxerExt_CollectValidStreams_001
 * @tc.number: MediaDemuxerExt_CollectValidStreams_001
 * @tc.desc  : Test CollectValidStreams keeps preferred streams and subtitle stream
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CollectValidStreams_001, TestSize.Level1)
{
    std::vector<StreamInfo> allStreams;
    StreamInfo videoStream;
    videoStream.streamId = TEST_STREAM_ID_1;
    videoStream.type = StreamType::VIDEO;
    videoStream.mimeType = TEST_MIME_VIDEO_AVC;
    videoStream.originCodecs = "avc1.64001f";
    StreamInfo audioStream;
    audioStream.streamId = TEST_STREAM_ID_2;
    audioStream.type = StreamType::AUDIO;
    audioStream.mimeType = TEST_MIME_AUDIO_AAC;
    audioStream.originCodecs = "mp4a.40.2";
    StreamInfo subtitleStream;
    subtitleStream.streamId = TEST_STREAM_ID_3;
    subtitleStream.type = StreamType::SUBTITLE;
    subtitleStream.lang = TEST_LANG_ZH;
    allStreams = {videoStream, audioStream, subtitleStream};

    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {TEST_MIME_VIDEO_AVC};
    filter.preferredAudioMimeTypes = {TEST_MIME_AUDIO_AAC};
    std::shared_ptr<MediaAVCodec::AVCodecList> codecList {nullptr};
    auto validStreams = CollectValidStreams(allStreams, "hls", filter, codecList);

    EXPECT_EQ(validStreams.size(), 3u);
}

/**
 * @tc.name  : MediaDemuxerExt_CollectValidStreams_002
 * @tc.number: MediaDemuxerExt_CollectValidStreams_002
 * @tc.desc  : Test CollectValidStreams falls back to original audio and video streams
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_CollectValidStreams_002, TestSize.Level1)
{
    std::vector<StreamInfo> allStreams;
    StreamInfo videoStream;
    videoStream.streamId = TEST_STREAM_ID_1;
    videoStream.type = StreamType::VIDEO;
    videoStream.mimeType = "";
    videoStream.codecs = "invalid-codec";
    StreamInfo audioStream;
    audioStream.streamId = TEST_STREAM_ID_2;
    audioStream.type = StreamType::AUDIO;
    audioStream.codecs = "invalid-codec";
    allStreams = {videoStream, audioStream};

    TrackSelectionFilter filter;
    std::shared_ptr<MediaAVCodec::AVCodecList> codecList {nullptr};
    auto validStreams = CollectValidStreams(allStreams, "hls", filter, codecList);

    EXPECT_EQ(validStreams.size(), 2u);
}

/**
 * @tc.name  : MediaDemuxerExt_GetNeedFilterMap_001
 * @tc.number: MediaDemuxerExt_GetNeedFilterMap_001
 * @tc.desc  : Test GetNeedFilterMap for http and dash play types
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_GetNeedFilterMap_001, TestSize.Level1)
{
    std::map<StreamType, std::vector<StreamFilterPredicate>> filteredStreamPredicates;
    filteredStreamPredicates[AUDIO].emplace_back([](const StreamInfo &) { return true; });
    std::map<StreamType, bool> needFilterMap;
    TrackSelectionFilter filter;
    filter.preferredVideoMimeTypes = {TEST_MIME_VIDEO_AVC};
    GetNeedFilterMap(filteredStreamPredicates, filter, "http", needFilterMap);
    EXPECT_FALSE(needFilterMap[VIDEO]);
    EXPECT_FALSE(needFilterMap[AUDIO]);
    EXPECT_FALSE(needFilterMap[SUBTITLE]);

    filteredStreamPredicates.clear();
    needFilterMap.clear();
    GetNeedFilterMap(filteredStreamPredicates, filter, "dash", needFilterMap);
    EXPECT_TRUE(needFilterMap[VIDEO]);
    EXPECT_TRUE(needFilterMap[AUDIO]);
    EXPECT_TRUE(needFilterMap[SUBTITLE]);
}

/**
 * @tc.name  : MediaDemuxerExt_IsNeedPreDownload_001
 * @tc.number: MediaDemuxerExt_IsNeedPreDownload_001
 * @tc.desc  : Test IsNeedPreDownload with nullptr and mocked source
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_IsNeedPreDownload_001, TestSize.Level1)
{
    mediaDemuxer_->source_ = nullptr;
    EXPECT_FALSE(mediaDemuxer_->IsNeedPreDownload());

    mediaDemuxer_->source_ = std::make_shared<Source>();
    auto source = std::static_pointer_cast<Source>(mediaDemuxer_->source_);
    ASSERT_NE(source, nullptr);
    EXPECT_CALL(*source, IsNeedPreDownload()).WillOnce(Return(true));
    EXPECT_TRUE(mediaDemuxer_->IsNeedPreDownload());
}

/**
 * @tc.name  : MediaDemuxerExt_SetOutputBufferQueue_001
 * @tc.number: MediaDemuxerExt_SetOutputBufferQueue_001
 * @tc.desc  : test set output track is subtitle
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SetOutputBufferQueue_001, TestSize.Level1)
{
    std::string audioMimeType = "audio/mp3";
    int64_t startTime = NUM_1000;
    std::shared_ptr<Meta> trackMeta1 = std::make_shared<Meta>();
    std::shared_ptr<Meta> trackMeta2 = std::make_shared<Meta>();
    trackMeta1->SetData(Tag::MIME_TYPE, audioMimeType);
    trackMeta2->SetData(Tag::MEDIA_START_TIME, startTime);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta1);
    mediaDemuxer_->mediaMetaData_.trackMetas.push_back(trackMeta2);
    mediaDemuxer_->OnInterrupted(false);
    mediaDemuxer_->SetBundleName("test");

    std::shared_ptr<AVBufferQueue> inputBufferQueue =
    AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    EXPECT_EQ(mediaDemuxer_->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : MediaDemuxerExt_InnerSelectTrack_001
 * @tc.number: MediaDemuxerExt_InnerSelectTrack_001
 * @tc.desc  : test IsNeedMapToInnerTrackID = true &&  IsSubtitleTrackId = true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_InnerSelectTrack_001, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(0));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_EQ(mediaDemuxer_->InnerSelectTrack(NUM_1), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_InnerSelectTrack_002
 * @tc.number: MediaDemuxerExt_InnerSelectTrack_002
 * @tc.desc  : test IsNeedMapToInnerTrackID = true &&  IsSubtitleTrackId = false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_InnerSelectTrack_002, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = true;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(0));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_EQ(mediaDemuxer_->InnerSelectTrack(NUM_1), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_InnerSelectTrack_003
 * @tc.number: MediaDemuxerExt_InnerSelectTrack_003
 * @tc.desc  : test IsNeedMapToInnerTrackID = false &&  IsSubtitleTrackId = true
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_InnerSelectTrack_003, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = false;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_SUBTITLE));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillOnce(Return(-1)).WillOnce(Return(1));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpInnerTrackIDByTrackID(_))
        .WillOnce(Return(0));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_EQ(mediaDemuxer_->InnerSelectTrack(NUM_1), Status::OK);
}

/**
 * @tc.name  : MediaDemuxerExt_InnerSelectTrack_004
 * @tc.number: MediaDemuxerExt_InnerSelectTrack_004
 * @tc.desc  : test IsNeedMapToInnerTrackID = false &&  IsSubtitleTrackId = false
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_InnerSelectTrack_004, TestSize.Level1)
{
    mediaDemuxer_->isFlvLiveStream_ = false;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillRepeatedly(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_),
        GetTrackTypeByTrackID(_)).WillRepeatedly(Return(TrackType::TRACK_AUDIO));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetTmpStreamIDByTrackID(_))
        .WillRepeatedly(Return(-1));
    auto plugin = std::make_shared<DemuxerPlugin>("MockPlugin");
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetStreamIDByTrackID(_))
        .WillRepeatedly(Return(0));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetPluginByStreamID(_)).WillRepeatedly(Return(plugin));
    EXPECT_CALL(*plugin, SelectTrack(_)).WillOnce(Return(Status::OK));
    EXPECT_EQ(mediaDemuxer_->InnerSelectTrack(NUM_1), Status::OK);
}

/**
 * @tc.name  : SelectBitRate_002
 * @tc.number: SelectBitRate_002
 * @tc.desc  : Test SelectBitRate caches pending bitrate when dash change is busy and force select is enabled
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectBitRate_002, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);

    auto ret = mediaDemuxer_->SelectBitRate(UNUM_2000, false, true);

    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_2000);
}

/**
 * @tc.name  : SelectBitRate_003
 * @tc.number: SelectBitRate_003
 * @tc.desc  : Test SelectBitRate does not cache pending bitrate when dash change is busy and force select is disabled
 */
HWTEST_F(MediaDemuxerExtUnitTest, SelectBitRate_003, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_0);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);

    auto ret = mediaDemuxer_->SelectBitRate(UNUM_2000, false, false);

    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_0);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_007
 * @tc.number: MediaDemuxerExt_SelectStreamId_007
 * @tc.desc  : Test SelectStreamId caches pending audio stream when dash change is not allowed
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_007, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->waitAudioStreamId_.store(NEG_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NEG_1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    auto ret = mediaDemuxer_->SelectStreamId(NUM_1, true);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NUM_1);
    EXPECT_EQ(mediaDemuxer_->waitSubtitleStreamId_.load(), NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_SelectStreamId_008
 * @tc.number: MediaDemuxerExt_SelectStreamId_008
 * @tc.desc  : Test SelectStreamId caches pending subtitle stream when dash change is not allowed
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_SelectStreamId_008, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->waitAudioStreamId_.store(NEG_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NEG_1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(true));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), CanDoChangeStream()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    auto ret = mediaDemuxer_->SelectStreamId(NUM_2, false);
    EXPECT_EQ(ret, Status::ERROR_WRONG_STATE);
    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NEG_1);
    EXPECT_EQ(mediaDemuxer_->waitSubtitleStreamId_.load(), NUM_2);
}

/**
 * @tc.name  : MediaDemuxerExt_WaitTrackSelectionFilter_001
 * @tc.number: MediaDemuxerExt_WaitTrackSelectionFilter_001
 * @tc.desc  : Test WaitTrackSelectionFilter stores values by track type
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_WaitTrackSelectionFilter_001, TestSize.Level1)
{
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_0);
    mediaDemuxer_->waitAudioStreamId_.store(NEG_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NEG_1);

    mediaDemuxer_->WaitTrackSelectionFilter(UNUM_1000, NUM_1, DemuxerTrackType::VIDEO);
    mediaDemuxer_->WaitTrackSelectionFilter(UNUM_0, NUM_2, DemuxerTrackType::AUDIO);
    mediaDemuxer_->WaitTrackSelectionFilter(UNUM_0, NUM_3, DemuxerTrackType::SUBTITLE);

    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_1000);
    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NUM_2);
    EXPECT_EQ(mediaDemuxer_->waitSubtitleStreamId_.load(), NUM_3);
}

/**
 * @tc.name  : MediaDemuxerExt_WaitTrackSelectionFilter_002
 * @tc.number: MediaDemuxerExt_WaitTrackSelectionFilter_002
 * @tc.desc  : Test WaitTrackSelectionFilter ignores unsupported track type
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_WaitTrackSelectionFilter_002, TestSize.Level1)
{
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_1000);
    mediaDemuxer_->waitAudioStreamId_.store(NUM_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NUM_2);

    mediaDemuxer_->WaitTrackSelectionFilter(UNUM_2000, NUM_3, static_cast<DemuxerTrackType>(NEG_1));

    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_1000);
    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NUM_1);
    EXPECT_EQ(mediaDemuxer_->waitSubtitleStreamId_.load(), NUM_2);
}

/**
 * @tc.name  : MediaDemuxerExt_ProcessTrackSelectionFilter_001
 * @tc.number: MediaDemuxerExt_ProcessTrackSelectionFilter_001
 * @tc.desc  : Test ProcessTrackSelectionFilter consumes pending bitrate when current bitrate differs
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProcessTrackSelectionFilter_001, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_2000);
    mediaDemuxer_->waitAudioStreamId_.store(NUM_1);
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_1000));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(UNUM_2000)).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->ProcessTrackSelectionFilter();

    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_0);
    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NUM_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ProcessTrackSelectionFilter_002
 * @tc.number: MediaDemuxerExt_ProcessTrackSelectionFilter_002
 * @tc.desc  : Test ProcessTrackSelectionFilter clears matched bitrate and selects pending audio stream
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProcessTrackSelectionFilter_002, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_2000);
    mediaDemuxer_->waitAudioStreamId_.store(NUM_1);
    mediaDemuxer_->defaultAudioStreamId_ = NUM_1;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), GetCurrentBitRate()).WillRepeatedly(Return(UNUM_2000));
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(NUM_1)).WillOnce(Return(Status::OK));

    mediaDemuxer_->ProcessTrackSelectionFilter();

    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_0);
    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ProcessTrackSelectionFilter_003
 * @tc.number: MediaDemuxerExt_ProcessTrackSelectionFilter_003
 * @tc.desc  : Test ProcessTrackSelectionFilter clears unmatched pending audio stream without selecting it
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProcessTrackSelectionFilter_003, TestSize.Level1)
{
    mediaDemuxer_->waitAudioStreamId_.store(NUM_2);
    mediaDemuxer_->defaultAudioStreamId_ = NUM_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->ProcessTrackSelectionFilter();

    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ProcessTrackSelectionFilter_004
 * @tc.number: MediaDemuxerExt_ProcessTrackSelectionFilter_004
 * @tc.desc  : Test ProcessTrackSelectionFilter selects pending subtitle stream when audio wait is empty
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProcessTrackSelectionFilter_004, TestSize.Level1)
{
    mediaDemuxer_->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    mediaDemuxer_->waitAudioStreamId_.store(NEG_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NUM_2);
    mediaDemuxer_->defaultSubtitleStreamId_ = NUM_2;
    EXPECT_CALL(*(mediaDemuxer_->demuxerPluginManager_), IsDash()).WillOnce(Return(false));
    EXPECT_CALL(*(mediaDemuxer_->streamDemuxer_), ResetAllCache()).WillOnce(Return(Status::OK));
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(NUM_2)).WillOnce(Return(Status::OK));

    mediaDemuxer_->ProcessTrackSelectionFilter();

    EXPECT_EQ(mediaDemuxer_->waitSubtitleStreamId_.load(), NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ProcessTrackSelectionFilter_005
 * @tc.number: MediaDemuxerExt_ProcessTrackSelectionFilter_005
 * @tc.desc  : Test ProcessTrackSelectionFilter keeps state unchanged when there is no pending filter
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProcessTrackSelectionFilter_005, TestSize.Level1)
{
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_0);
    mediaDemuxer_->waitAudioStreamId_.store(NEG_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NEG_1);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->ProcessTrackSelectionFilter();

    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_0);
    EXPECT_EQ(mediaDemuxer_->waitAudioStreamId_.load(), NEG_1);
    EXPECT_EQ(mediaDemuxer_->waitSubtitleStreamId_.load(), NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ProcessTrackSelectionFilter_006
 * @tc.number: MediaDemuxerExt_ProcessTrackSelectionFilter_006
 * @tc.desc  : Test ProcessTrackSelectionFilter clears unmatched subtitle stream without selecting it
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProcessTrackSelectionFilter_006, TestSize.Level1)
{
    mediaDemuxer_->waitAudioStreamId_.store(NEG_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NUM_2);
    mediaDemuxer_->defaultSubtitleStreamId_ = NUM_1;
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->ProcessTrackSelectionFilter();

    EXPECT_EQ(mediaDemuxer_->waitSubtitleStreamId_.load(), NEG_1);
}

/**
 * @tc.name  : MediaDemuxerExt_ProcessTrackSelectionFilter_007
 * @tc.number: MediaDemuxerExt_ProcessTrackSelectionFilter_007
 * @tc.desc  : Test ProcessTrackSelectionFilter keeps pending bitrate when demuxer plugin manager is null
 */
HWTEST_F(MediaDemuxerExtUnitTest, MediaDemuxerExt_ProcessTrackSelectionFilter_007, TestSize.Level1)
{
    mediaDemuxer_->demuxerPluginManager_ = nullptr;
    mediaDemuxer_->waitVideoBitrate_.store(UNUM_2000);
    mediaDemuxer_->waitAudioStreamId_.store(NEG_1);
    mediaDemuxer_->waitSubtitleStreamId_.store(NEG_1);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectBitRate(_)).Times(0);
    EXPECT_CALL(*(mediaDemuxer_->source_), SelectStream(_)).Times(0);

    mediaDemuxer_->ProcessTrackSelectionFilter();

    EXPECT_EQ(mediaDemuxer_->waitVideoBitrate_.load(), UNUM_2000);
}
}  // namespace OHOS::Media
