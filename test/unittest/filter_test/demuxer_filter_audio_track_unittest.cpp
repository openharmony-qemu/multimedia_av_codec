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

#include <cstddef>
#include <cstdint>
#include <vector>
#include "demuxer_filter_unit_test.h"
#include "common/media_core.h"
#include "common/media_source.h"
#include "meta/any.h"
#include "meta/mime_type.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
namespace {
const std::string AUDIO_TRACK_TEST_MIME_AAC = "audio/mp4a-latm";
const std::string AUDIO_TRACK_TEST_MIME_DOLBY = "audio/eac3";
const std::string AUDIO_TRACK_TEST_MIME_VIDEO = "video/avc";
}  // namespace

// Fixture for unsupported-audio-track degrade scenarios added by the track rebuild fix.
class DemuxerFilterAudioTrackTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void) override;
    // TearDown: Called after each test cases
    void TearDown(void) override;

protected:
    std::shared_ptr<DemuxerFilter> demuxerFilter_ { nullptr };
};

// Records events dispatched by the filter so that EVENT_WARN / EVENT_ERROR can be asserted.
class AudioTrackEventReceiver : public EventReceiver {
public:
    void OnEvent(const Event &event) override
    {
        events.push_back(event);
    }

    bool HasEvent(EventType type) const
    {
        for (const auto &event : events) {
            if (event.type == type) {
                return true;
            }
        }
        return false;
    }

    std::vector<Event> events;
};

// Returns a configurable link result per stream type: audio link can be forced to fail while
// other tracks still link successfully, which drives the degrade-to-video-only branches.
class AudioTrackFilterCallback : public FilterCallback {
public:
    AudioTrackFilterCallback() = default;
    ~AudioTrackFilterCallback() override = default;

    Status OnCallback(const std::shared_ptr<Filter> &filter, FilterCallBackCommand cmd, StreamType outType) override
    {
        (void)filter;
        (void)cmd;
        if (outType == StreamType::STREAMTYPE_ENCODED_AUDIO) {
            return audioLinkStatus_;
        }
        return otherLinkStatus_;
    }

    std::vector<std::string> GetDolbyListCallback() override
    {
        return dolbyList_;
    }

    bool IsAudioPassthroughCallback(const char *mimeType) override
    {
        (void)mimeType;
        return isPassthrough_;
    }

    Status audioLinkStatus_ { Status::OK };
    Status otherLinkStatus_ { Status::OK };
    std::vector<std::string> dolbyList_ {};
    bool isPassthrough_ { false };
};

void DemuxerFilterAudioTrackTest::SetUpTestCase(void) {}

void DemuxerFilterAudioTrackTest::TearDownTestCase(void) {}

void DemuxerFilterAudioTrackTest::SetUp(void)
{
    demuxerFilter_ = std::make_shared<DemuxerFilter>("testDemuxerFilterAudioTrack", FilterType::FILTERTYPE_DEMUXER);
    demuxerFilter_->receiver_ = std::make_shared<AudioTrackEventReceiver>();
    demuxerFilter_->demuxer_ = std::make_shared<MediaDemuxer>();
}

void DemuxerFilterAudioTrackTest::TearDown(void)
{
    demuxerFilter_ = nullptr;
}

/**
 * @tc.name: DemuxerFilter_IsAudioSupportedOnDevice_0100
 * @tc.desc: Test IsAudioSupportedOnDevice when demuxer_ is nullptr, assume supported
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_IsAudioSupportedOnDevice_0100, TestSize.Level1)
{
    demuxerFilter_->demuxer_ = nullptr;
    // demuxer_ is nullptr: assume supported and keep playback going
    EXPECT_TRUE(demuxerFilter_->IsAudioSupportedOnDevice(AUDIO_TRACK_TEST_MIME_AAC));
}

/**
 * @tc.name: DemuxerFilter_ClearTrackIdMap_0100
 * @tc.desc: Test ClearTrackIdMap removes entries of the given stream type only
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_ClearTrackIdMap_0100, TestSize.Level1)
{
    demuxerFilter_->UpdateTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO, 1);
    demuxerFilter_->UpdateTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO, 2);
    int32_t trackId = 0;
    EXPECT_TRUE(demuxerFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, trackId));
    EXPECT_EQ(trackId, 1);
    // the remaining entry of the stream type is removed by ClearTrackIdMap
    demuxerFilter_->ClearTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO);
    EXPECT_FALSE(demuxerFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, trackId));
    // clearing a stream type without entries does not crash
    demuxerFilter_->ClearTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO);
    EXPECT_FALSE(demuxerFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, trackId));
    // entries of other stream types are kept
    demuxerFilter_->UpdateTrackIdMap(StreamType::STREAMTYPE_ENCODED_VIDEO, 3);
    EXPECT_TRUE(demuxerFilter_->FindTrackId(StreamType::STREAMTYPE_ENCODED_VIDEO, trackId));
    EXPECT_EQ(trackId, 3);
}

/**
 * @tc.name: DemuxerFilter_GetAudioStreamType_0100
 * @tc.desc: Test GetAudioStreamType for raw/encoded audio mimes
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_GetAudioStreamType_0100, TestSize.Level1)
{
    StreamType streamType = StreamType::STREAMTYPE_MAX;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    // little-endian raw audio maps to STREAMTYPE_RAW_AUDIO
    EXPECT_TRUE(demuxerFilter_->GetAudioStreamType(streamType, std::string(MimeType::AUDIO_RAW), 0, meta));
    EXPECT_EQ(streamType, StreamType::STREAMTYPE_RAW_AUDIO);
    // big-endian raw audio falls back to STREAMTYPE_ENCODED_AUDIO
    std::shared_ptr<Meta> beMeta = std::make_shared<Meta>();
    beMeta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16BE);
    EXPECT_TRUE(demuxerFilter_->GetAudioStreamType(streamType, std::string(MimeType::AUDIO_RAW), 0, beMeta));
    EXPECT_EQ(streamType, StreamType::STREAMTYPE_ENCODED_AUDIO);
    // non-raw audio mime maps to STREAMTYPE_ENCODED_AUDIO
    EXPECT_TRUE(demuxerFilter_->GetAudioStreamType(streamType, AUDIO_TRACK_TEST_MIME_AAC, 0, meta));
    EXPECT_EQ(streamType, StreamType::STREAMTYPE_ENCODED_AUDIO);
}

/**
 * @tc.name: DemuxerFilter_GetAudioStreamType_0200
 * @tc.desc: Test GetAudioStreamType returns STREAMTYPE_DOLBY for dolby passthrough mime
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_GetAudioStreamType_0200, TestSize.Level1)
{
    auto callback = std::make_shared<AudioTrackFilterCallback>();
    callback->dolbyList_ = { AUDIO_TRACK_TEST_MIME_DOLBY };
    callback->isPassthrough_ = true;
    demuxerFilter_->callback_ = callback;
    StreamType streamType = StreamType::STREAMTYPE_MAX;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_TRUE(demuxerFilter_->GetAudioStreamType(streamType, AUDIO_TRACK_TEST_MIME_DOLBY, 0, meta));
    EXPECT_EQ(streamType, StreamType::STREAMTYPE_DOLBY);
}

/**
 * @tc.name: DemuxerFilter_UnLinkNext_0100
 * @tc.desc: Test UnLinkNext with nullptr nextFilter returns ERROR_INVALID_OPERATION
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_UnLinkNext_0100, TestSize.Level1)
{
    std::shared_ptr<Filter> prevNext = demuxerFilter_->nextFilter_;
    EXPECT_EQ(demuxerFilter_->UnLinkNext(nullptr, StreamType::STREAMTYPE_ENCODED_AUDIO),
        Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(demuxerFilter_->nextFilter_, prevNext);

    std::shared_ptr<Filter> otherFilter = std::make_shared<Filter>("OtherFilter", FilterType::FILTERTYPE_MAX);
    EXPECT_EQ(demuxerFilter_->UnLinkNext(otherFilter, StreamType::STREAMTYPE_RAW_AUDIO), Status::OK);
    EXPECT_EQ(demuxerFilter_->nextFilter_, prevNext);
}

/**
 * @tc.name: DemuxerFilter_UnLinkNext_0200
 * @tc.desc: Test UnLinkNext with a valid filter clears nextFilter_
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_UnLinkNext_0200, TestSize.Level1)
{
    std::shared_ptr<Filter> nextFilter = std::make_shared<Filter>("AudioTrackNextFilter", FilterType::FILTERTYPE_MAX);
    demuxerFilter_->nextFilter_ = nextFilter;
    EXPECT_EQ(demuxerFilter_->UnLinkNext(nextFilter, StreamType::STREAMTYPE_ENCODED_AUDIO), Status::OK);
    EXPECT_EQ(demuxerFilter_->nextFilter_, nullptr);
}

/**
 * @tc.name: DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0100
 * @tc.desc: Test audio exists but none linked in player mode, EVENT_WARN is reported
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0100, TestSize.Level1)
{
    auto receiver = std::make_shared<AudioTrackEventReceiver>();
    demuxerFilter_->receiver_ = receiver;
    auto callback = std::make_shared<AudioTrackFilterCallback>();
    callback->audioLinkStatus_ = Status::ERROR_UNKNOWN;
    callback->otherLinkStatus_ = Status::OK;
    demuxerFilter_->callback_ = callback;
    demuxerFilter_->isPlayerMode_ = true;

    std::vector<std::shared_ptr<Meta>> trackInfos;
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    videoMeta->Set<Tag::REGULAR_TRACK_ID>(0);
    videoMeta->Set<Tag::MIME_TYPE>(AUDIO_TRACK_TEST_MIME_VIDEO);
    videoMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    trackInfos.push_back(videoMeta);
    std::shared_ptr<Meta> audioMeta = std::make_shared<Meta>();
    audioMeta->Set<Tag::REGULAR_TRACK_ID>(1);
    audioMeta->Set<Tag::MIME_TYPE>(AUDIO_TRACK_TEST_MIME_AAC);
    audioMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    trackInfos.push_back(audioMeta);

    int32_t successNodeCount = 0;
    auto res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    EXPECT_EQ(res, Status::OK);
    EXPECT_EQ(successNodeCount, 1);  // video linked, audio link failed
    bool hasWarn = false;
    for (const auto &event : receiver->events) {
        if (event.type == EventType::EVENT_WARN) {
            hasWarn = true;
            EXPECT_EQ(AnyCast<int32_t>(event.param), static_cast<int32_t>(MSERR_UNSUPPORT_AUD_DEC_TYPE));
        }
    }
    EXPECT_TRUE(hasWarn);
    EXPECT_FALSE(receiver->HasEvent(EventType::EVENT_ERROR));
}

/**
 * @tc.name: DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0200
 * @tc.desc: Test audio disabled, no degrade warning is reported
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0200, TestSize.Level1)
{
    auto receiver = std::make_shared<AudioTrackEventReceiver>();
    demuxerFilter_->receiver_ = receiver;
    auto callback = std::make_shared<AudioTrackFilterCallback>();
    demuxerFilter_->callback_ = callback;
    demuxerFilter_->isPlayerMode_ = true;
    demuxerFilter_->disabledMediaTracks_.insert(Plugins::MediaType::AUDIO);

    std::vector<std::shared_ptr<Meta>> trackInfos;
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    videoMeta->Set<Tag::REGULAR_TRACK_ID>(0);
    videoMeta->Set<Tag::MIME_TYPE>(AUDIO_TRACK_TEST_MIME_VIDEO);
    videoMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    trackInfos.push_back(videoMeta);
    std::shared_ptr<Meta> audioMeta = std::make_shared<Meta>();
    audioMeta->Set<Tag::REGULAR_TRACK_ID>(1);
    audioMeta->Set<Tag::MIME_TYPE>(AUDIO_TRACK_TEST_MIME_AAC);
    audioMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    trackInfos.push_back(audioMeta);

    int32_t successNodeCount = 0;
    auto res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    EXPECT_EQ(res, Status::OK);
    EXPECT_EQ(successNodeCount, 1);  // audio is muted and skipped entirely
    EXPECT_FALSE(receiver->HasEvent(EventType::EVENT_WARN));
    EXPECT_FALSE(receiver->HasEvent(EventType::EVENT_ERROR));
}

/**
 * @tc.name: DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0300
 * @tc.desc: Test audio track linked successfully, no degrade warning is reported
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0300, TestSize.Level1)
{
    auto receiver = std::make_shared<AudioTrackEventReceiver>();
    demuxerFilter_->receiver_ = receiver;
    auto callback = std::make_shared<AudioTrackFilterCallback>();
    demuxerFilter_->callback_ = callback;
    demuxerFilter_->isPlayerMode_ = true;
    demuxerFilter_->demuxer_->audioTrackId_ = 1;

    std::vector<std::shared_ptr<Meta>> trackInfos;
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    videoMeta->Set<Tag::REGULAR_TRACK_ID>(0);
    videoMeta->Set<Tag::MIME_TYPE>(AUDIO_TRACK_TEST_MIME_VIDEO);
    videoMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    trackInfos.push_back(videoMeta);
    std::shared_ptr<Meta> audioMeta = std::make_shared<Meta>();
    audioMeta->Set<Tag::REGULAR_TRACK_ID>(1);
    audioMeta->Set<Tag::MIME_TYPE>(AUDIO_TRACK_TEST_MIME_AAC);
    audioMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    trackInfos.push_back(audioMeta);

    int32_t successNodeCount = 0;
    auto res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    EXPECT_EQ(res, Status::OK);
    EXPECT_EQ(successNodeCount, 2);  // both video and audio linked
    EXPECT_FALSE(receiver->HasEvent(EventType::EVENT_WARN));
    EXPECT_FALSE(receiver->HasEvent(EventType::EVENT_ERROR));
}

/**
 * @tc.name: DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0400
 * @tc.desc: Test no track linked in player mode, no degrade warning is reported
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterAudioTrackTest, DemuxerFilter_HandleTrackInfos_DegradeToVideoOnly_0400, TestSize.Level1)
{
    auto receiver = std::make_shared<AudioTrackEventReceiver>();
    demuxerFilter_->receiver_ = receiver;
    auto callback = std::make_shared<AudioTrackFilterCallback>();
    callback->audioLinkStatus_ = Status::ERROR_UNKNOWN;
    callback->otherLinkStatus_ = Status::ERROR_UNKNOWN;
    demuxerFilter_->callback_ = callback;
    demuxerFilter_->isPlayerMode_ = true;

    std::vector<std::shared_ptr<Meta>> trackInfos;
    std::shared_ptr<Meta> audioMeta = std::make_shared<Meta>();
    audioMeta->Set<Tag::REGULAR_TRACK_ID>(1);
    audioMeta->Set<Tag::MIME_TYPE>(AUDIO_TRACK_TEST_MIME_AAC);
    audioMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    trackInfos.push_back(audioMeta);

    int32_t successNodeCount = 0;
    auto res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    EXPECT_EQ(res, Status::OK);
    EXPECT_EQ(successNodeCount, 0);  // nothing linked, caller DoPrepare handles this case
    EXPECT_FALSE(receiver->HasEvent(EventType::EVENT_WARN));
    EXPECT_FALSE(receiver->HasEvent(EventType::EVENT_ERROR));
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
