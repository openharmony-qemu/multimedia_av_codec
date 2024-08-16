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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <string>
#include <sys/stat.h>
#include "subtitle_sink_filter_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace {
static const std::string MEDIA_ROOT = "file:///data/test/media/";
static const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
static const std::string MIME_IMAGE = "image";
}  // namespace

namespace OHOS {
namespace Media {
namespace Pipeline {
class FilterLinkCallbackMock : public FilterLinkCallback {
public:
    void OnLinkedResult(const sptr<AVBufferQueueProducer>& queue, std::shared_ptr<Meta>& meta)
    {
        (void)queue;
        (void)meta;
    }

    void OnUnlinkedResult(std::shared_ptr<Meta>& meta)
    {
        (void)meta;
    }

    void OnUpdatedResult(std::shared_ptr<Meta>& meta)
    {
        (void)meta;
    }
}
void SubtitleSinkFilterUnitTest::SetUpTestCase(void) {}

void SubtitleSinkFilterUnitTest::TearDownTestCase(void) {}

void SubtitleSinkFilterUnitTest::SetUp(void)
{
    subtitleSinkFilter_ = std::make_shared<SubtitleSinkFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_DEMUXER);
    auto receiver = std::make_shared<FilterEventReceiverMock>();
    subtitleSinkFilter_->receiver_ = receiver;
    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    subtitleSinkFilter_->SetDataSource(mediaSource);
}

void SubtitleSinkFilterUnitTest::TearDown(void)
{
    subtitleSinkFilter_ = nullptr;
}

/**
 * @tc.name: SetDataSource
 * @tc.desc: SetDataSource
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoPrepare, TestSize.Level1)
{ 
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    subtitleSinkFilter_->onLinkedResultCallback_ = callback;
    res = subtitleSinkFilter_->DoPrepare();
}

/**
 * @tc.name: DoStart
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoStart, TestSize.Level1)
{
    subtitleSinkFilter_->state_ = FilterState::RUNNING;
    res = subtitleSinkFilter_->DoStart();

    subtitleSinkFilter_->state_ = FilterState::READY;
    res = subtitleSinkFilter_->DoStart();

    subtitleSinkFilter_->state_ = FilterState::PAUSED;
    res = subtitleSinkFilter_->DoStart();

    subtitleSinkFilter_->state_ = FilterState::ERROR;
    res = subtitleSinkFilter_->DoStart();
    
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    subtitleSinkFilter_->onLinkedResultCallback_ = callback;
    res = subtitleSinkFilter_->DoStart();
}

/**
 * @tc.name: DoPause
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoPause, TestSize.Level1)
{
    subtitleSinkFilter_->state_ = FilterState::PAUSED;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::STOPPED;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::READY;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::RUNNING;
    res = subtitleSinkFilter_->DoPause();

    subtitleSinkFilter_->state_ = FilterState::ERROR;
    res = subtitleSinkFilter_->DoPause();
    
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    subtitleSinkFilter_->onLinkedResultCallback_ = callback;
    res = subtitleSinkFilter_->DoPause();
}

/**
 * @tc.name: DoResume
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoResume, TestSize.Level1)
{
    subtitleSinkFilter_->state_ = FilterState::PAUSED;
    res = subtitleSinkFilter_->DoResume();

    subtitleSinkFilter_->frameCnt_ = 1;
    res = subtitleSinkFilter_->DoResume();

    subtitleSinkFilter_->state_ = FilterState::STOPPED;
    res = subtitleSinkFilter_->DoResume();
}

/**
 * @tc.name: DoFlush
 * @tc.desc: DoFlush
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoFlush, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->DoFlush();

    subtitleSinkFilter_subtitleSink_ = nullptr;
    res = subtitleSinkFilter_->DoFlush();
}

/**
 * @tc.name: DoStop
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoStop, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->DoStop();

    subtitleSinkFilter_subtitleSink_ = nullptr;
    res = subtitleSinkFilter_->DoStop();
}

/**
 * @tc.name: RegisterVideoStreamReadyCallback
 * @tc.desc: RegisterVideoStreamReadyCallback
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, RegisterVideoStreamReadyCallback, TestSize.Level1)
{
    auto readyCallback = std::make_shared<VideoStreamReadyCallbackMock>();
    subtitleSinkFilter_->RegisterVideoStreamReadyCallback(readyCallback);
    subtitleSinkFilter_->RegisterVideoStreamReadyCallback(nullptr);
}

/**
 * @tc.name: Init
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, Init, TestSize.Level1)
{
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    subtitleSinkFilter_->demuxer_ = demuxer;

    auto receiver = std::make_shared<FilterEventReceiverMock>();
    auto callback = std::make_shared<FilterCallbackMock>();
    subtitleSinkFilter_->Init(receiver, callback);
}

/**
 * @tc.name: DoPrepare
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoPrepare, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->DoPrepare();
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;

    std::vector<std::shared_ptr<Meta>> trackInfos;
    trackInfos.push_back(nullptr);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxer->metaInfo_ = trackInfos;
    subtitleSinkFilter_->demuxer_ = demuxer;
    res = subtitleSinkFilter_->DoPrepare();
    trackInfos.pop_back();
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;

    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    trackInfos.push_back(meta);
    res = subtitleSinkFilter_->DoPrepare();
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: HandleTrackInfos
 * @tc.desc: HandleTrackInfos
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, HandleTrackInfos, TestSize.Level1)
{
    std::vector<std::shared_ptr<Meta>> trackInfos;
    int32_t successNodeCount = 0;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    trackInfos.push_back(meta);

    auto res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MIME_TYPE>("invalid/xxx");
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MIME_TYPE>("audio/xxx");
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    meta->Set<Tag::MIME_TYPE>(MIME_IMAGE);
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::TIMEDMETA);
    meta->Set<Tag::MIME_TYPE>("");
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    meta->Set<Tag::MIME_TYPE>("video/xxx");
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    auto callback = std::make_shared<FilterCallbackMock>();
    subtitleSinkFilter_->callback_ = callback;
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    callback->status_ = Status::ERROR_WRONG_STATE;
    res = subtitleSinkFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: ShouldTrackSkipped
 * @tc.desc: ShouldTrackSkipped
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, ShouldTrackSkipped, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->ShouldTrackSkipped(Plugins::MediaType::VIDEO, "", 0);
    EXPECT_NE(res, true);

    res = subtitleSinkFilter_->ShouldTrackSkipped(Plugins::MediaType::VIDEO, MIME_IMAGE, 0);
    EXPECT_EQ(res, true);

    res = subtitleSinkFilter_->ShouldTrackSkipped(Plugins::MediaType::TIMEDMETA, "", 0);
    EXPECT_EQ(res, true);

    subtitleSinkFilter_->disabledMediaTracks_.insert(Plugins::MediaType::VIDEO);
    res = subtitleSinkFilter_->ShouldTrackSkipped(Plugins::MediaType::VIDEO, "", 0);
    EXPECT_EQ(res, true);

    res = subtitleSinkFilter_->ShouldTrackSkipped(Plugins::MediaType::AUDIO, "", 0);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CollectVideoAndAudioMime
 * @tc.desc: CollectVideoAndAudioMime
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, CollectVideoAndAudioMime, TestSize.Level1)
{
    std::vector<std::shared_ptr<Meta>> trackInfos1;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();  // size 1, no mimeType
    meta->Set<Tag::MIME_TYPE>("xxxx/xxx");
    trackInfos1.push_back(meta);

    std::shared_ptr<Meta> meta1 = std::make_shared<Meta>();  // size 1, no mimeType
    meta1->Set<Tag::MIME_TYPE>("audio/xxx");
    trackInfos1.push_back(meta1);

    std::shared_ptr<Meta> meta2 = std::make_shared<Meta>();  // size 1, no mimeType
    meta2->Set<Tag::MIME_TYPE>("video/xxx");
    trackInfos1.push_back(meta2);

    std::shared_ptr<Meta> meta3 = std::make_shared<Meta>();  // size 1, no mimeType
    trackInfos1.push_back(meta3);

    auto mediaDemuxerMock = std::make_shared<MediaDemuxerMock>();
    mediaDemuxerMock->metaInfo_ = trackInfos1;
    subtitleSinkFilter_->demuxer_ = mediaDemuxerMock;

    auto res = subtitleSinkFilter_->CollectVideoAndAudioMime();

    std::cout << "CollectVideoAndAudioMime_0100 " << res << std::endl;
}

/**
 * @tc.name: UpdateTrackIdMap
 * @tc.desc: UpdateTrackIdMap
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, UpdateTrackIdMap, TestSize.Level1)
{
    subtitleSinkFilter_->UpdateTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO, 1);
    subtitleSinkFilter_->UpdateTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO, 1);
    EXPECT_TRUE(subtitleSinkFilter_->track_id_map_.size() > 0);
}

/**
 * @tc.name: FindTrackId
 * @tc.desc: FindTrackId
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, FindTrackId, TestSize.Level1)
{
    int32_t idTmp = 0;
    subtitleSinkFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, idTmp);

    std::vector<int32_t> idVec;
    idVec.push_back(0);
    subtitleSinkFilter_->track_id_map_.insert({ StreamType::STREAMTYPE_RAW_AUDIO, idVec });
    subtitleSinkFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, idTmp);

    idVec.push_back(1);
    subtitleSinkFilter_->track_id_map_.insert({ StreamType::STREAMTYPE_RAW_AUDIO, idVec });
    subtitleSinkFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, idTmp);
}

/**
 * @tc.name: FindStreamType
 * @tc.desc: FindStreamType
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, FindStreamType, TestSize.Level1)
{
    auto audio = StreamType::STREAMTYPE_RAW_AUDIO;
    auto res = subtitleSinkFilter_->FindStreamType(audio, Plugins::MediaType::SUBTITLE, "", 0);
    EXPECT_EQ(res, true);
    res = subtitleSinkFilter_->FindStreamType(audio, Plugins::MediaType::AUDIO, std::string(MimeType::AUDIO_RAW), 0);
    EXPECT_EQ(res, true);
    res = subtitleSinkFilter_->FindStreamType(audio, Plugins::MediaType::VIDEO, "", 0);
    EXPECT_EQ(res, true);
    res = subtitleSinkFilter_->FindStreamType(audio, Plugins::MediaType::TIMEDMETA, "", 0);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: OnLinkedResult
 * @tc.desc: OnLinkedResult
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, OnLinkedResult, TestSize.Level1)
{
    std::shared_ptr<Meta> meta0 = nullptr;
    subtitleSinkFilter_->OnLinkedResult(nullptr, meta0);

    auto meta = std::make_shared<Meta>();
    subtitleSinkFilter_->OnLinkedResult(nullptr, meta);

    meta->Set<Tag::REGULAR_TRACK_ID>(-1);
    subtitleSinkFilter_->OnLinkedResult(nullptr, meta);

    meta->Set<Tag::REGULAR_TRACK_ID>(0);
    subtitleSinkFilter_->OnLinkedResult(nullptr, meta);

    meta->Set<Tag::VIDEO_DECODER_RATE_UPPER_LIMIT>(0);
    subtitleSinkFilter_->OnLinkedResult(nullptr, meta);

    double frameRate = 2000.0;
    meta->Set<Tag::VIDEO_FRAME_RATE>(frameRate);
    subtitleSinkFilter_->OnLinkedResult(nullptr, meta);
}

/**
 * @tc.name: OnDrmInfoUpdated
 * @tc.desc: OnDrmInfoUpdated
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, OnDrmInfoUpdated, TestSize.Level1)
{
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    subtitleSinkFilter_->OnDrmInfoUpdated(drmInfo);

    subtitleSinkFilter_->receiver_ = nullptr;
    subtitleSinkFilter_->OnDrmInfoUpdated(drmInfo);
}

/**
 * @tc.name: DoPrepareFrame
 * @tc.desc: DoPrepareFrame
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoPrepareFrame, TestSize.Level1)
{
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    subtitleSinkFilter_->demuxer_ = demuxer;
    subtitleSinkFilter_->DoPrepareFrame(true);
    demuxer->prepareFrame_ = Status::ERROR_WRONG_STATE;
    subtitleSinkFilter_->DoPrepareFrame(true);
}

/**
 * @tc.name: PrepareBeforeStart
 * @tc.desc: PrepareBeforeStart
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, PrepareBeforeStart, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->PrepareBeforeStart();
    subtitleSinkFilter_->isLoopStarted = true;
    res = subtitleSinkFilter_->PrepareBeforeStart();
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: DoStart
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, DoStart, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->DoStart();
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;

    subtitleSinkFilter_->isLoopStarted = true;
    res = subtitleSinkFilter_->DoStart();
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;

    subtitleSinkFilter_->isLoopStarted = false;
    subtitleSinkFilter_->isPrepareFramed = true;
    res = subtitleSinkFilter_->DoStart();
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: PauseForSeek
 * @tc.desc: PauseForSeek
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, PauseForSeek, TestSize.Level1)
{
    // 1. it == nextFiltersMap_.end()
    auto res = subtitleSinkFilter_->PauseForSeek();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;

    // 2. it != nextFiltersMap_.end(), it->second.size() != 1
    std::map<StreamType, std::vector<std::shared_ptr<Filter>>> nextFiltersMap;
    std::vector<std::shared_ptr<Filter>> vec;
    nextFiltersMap.insert({ StreamType::STREAMTYPE_ENCODED_VIDEO, vec });
    res = subtitleSinkFilter_->PauseForSeek();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;

    // 3. it != nextFiltersMap_.end(), it->second.size() == 1, filter is nullptr
    vec.push_back(nullptr);
    res = subtitleSinkFilter_->PauseForSeek();
    vec.pop_back();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;

    // 4. it != nextFiltersMap_.end(), it->second.size() == 1, filter not nullptr
    auto filter = std::make_shared<Filter>("filter", FilterType::FILTERTYPE_MAX);
    vec.push_back(filter);
    res = subtitleSinkFilter_->PauseForSeek();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: ResumeForSeek
 * @tc.desc: ResumeForSeek
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, ResumeForSeek, TestSize.Level1)
{
    // 1. it == nextFiltersMap_.end()
    auto res = subtitleSinkFilter_->ResumeForSeek();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;

    // 2. it != nextFiltersMap_.end(), it->second.size() != 1
    std::map<StreamType, std::vector<std::shared_ptr<Filter>>> nextFiltersMap;
    std::vector<std::shared_ptr<Filter>> vec;
    nextFiltersMap.insert({ StreamType::STREAMTYPE_ENCODED_VIDEO, vec });
    res = subtitleSinkFilter_->ResumeForSeek();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;

    // 3. it != nextFiltersMap_.end(), it->second.size() == 1, filter is nullptr
    vec.push_back(nullptr);
    res = subtitleSinkFilter_->ResumeForSeek();
    vec.pop_back();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;

    // 4. it != nextFiltersMap_.end(), it->second.size() == 1, filter not nullptr
    auto filter = std::make_shared<Filter>("filter", FilterType::FILTERTYPE_MAX);
    vec.push_back(filter);
    res = subtitleSinkFilter_->ResumeForSeek();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: ResumeForSeek
 * @tc.desc: ResumeForSeek
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, SetDumpFlag, TestSize.Level1)
{
    subtitleSinkFilter_->SetDumpFlag(false);
    subtitleSinkFilter_->demuxer_ = nullptr;
    subtitleSinkFilter_->SetDumpFlag(false);
}

/**
 * @tc.name: LinkNext
 * @tc.desc: LinkNext
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, LinkNext, TestSize.Level1)
{
    std::shared_ptr<Filter> nextFilter = std::make_shared<Filter>("filter", FilterType::FILTERTYPE_MAX);
    StreamType outType = StreamType::STREAMTYPE_RAW_AUDIO;
    auto res = subtitleSinkFilter_->LinkNext(nextFilter, outType);
    std::cout << "LinkNext " << static_cast<int32_t>(res) << std::endl;

    std::vector<int32_t> idVec;
    idVec.push_back(0);
    subtitleSinkFilter_->track_id_map_.insert({ StreamType::STREAMTYPE_RAW_AUDIO, idVec });
    res = subtitleSinkFilter_->LinkNext(nextFilter, outType);
    std::cout << "LinkNext " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: GetBitRates
 * @tc.desc: GetBitRates
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, GetBitRates, TestSize.Level1)
{
    std::vector<uint32_t> bitRates;
    auto res = subtitleSinkFilter_->GetBitRates(bitRates);
    std::cout << "GetBitRates " << static_cast<int32_t>(res) << std::endl;

    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    subtitleSinkFilter_->SetDataSource(mediaSource);
    res = subtitleSinkFilter_->GetBitRates(bitRates);
    std::cout << "GetBitRates " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: GetDownloadInfo
 * @tc.desc: GetDownloadInfo
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, GetDownloadInfo, TestSize.Level1)
{
    DownloadInfo info;
    auto res = subtitleSinkFilter_->GetDownloadInfo(info);
    std::cout << "GetDownloadInfo " << static_cast<int32_t>(res) << std::endl;

    subtitleSinkFilter_->demuxer_ = nullptr;
    res = subtitleSinkFilter_->GetDownloadInfo(info);
    std::cout << "GetDownloadInfo " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: GetPlaybackInfo
 * @tc.desc: GetPlaybackInfo
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, GetPlaybackInfo, TestSize.Level1)
{
    PlaybackInfo info;
    auto res = subtitleSinkFilter_->GetPlaybackInfo(info);
    std::cout << "GetDownloadInfo " << static_cast<int32_t>(res) << std::endl;

    subtitleSinkFilter_->demuxer_ = nullptr;
    res = subtitleSinkFilter_->GetPlaybackInfo(info);
    std::cout << "GetDownloadInfo " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: SelectBitRate
 * @tc.desc: SelectBitRate
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, SelectBitRate, TestSize.Level1)
{
    auto res = subtitleSinkFilter_->SelectBitRate(0);
    std::cout << "GetBitRates " << static_cast<int32_t>(res) << std::endl;

    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    subtitleSinkFilter_->SetDataSource(mediaSource);
    res = subtitleSinkFilter_->SelectBitRate(0);
    std::cout << "GetBitRates " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: OnDumpInfo
 * @tc.desc: OnDumpInfo
 * @tc.type: FUNC
 */
HWTEST_F(SubtitleSinkFilterUnitTest, OnDumpInfo, TestSize.Level1)
{
    subtitleSinkFilter_->OnDumpInfo(0);
    subtitleSinkFilter_->demuxer_ = nullptr;
    subtitleSinkFilter_->OnDumpInfo(0);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS