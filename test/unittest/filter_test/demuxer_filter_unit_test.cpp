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
#include "demuxer_filter_unit_test.h"
#include "scope_guard.h"
#include "interstitial_controller.h"
#include "interstitial_scheduler.h"
#include "media_sync_manager.h"
#include "common/media_source.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace {
static const std::string MEDIA_ROOT = "file:///data/test/media/";
static const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
static const std::string VIDEO_FILE2 = MEDIA_ROOT + "test_265_B_Gop25_4sec.mp4";
static const std::string MIME_IMAGE = "image";
static const int32_t TEST_VIDEO_WIDTH = 1920;
static const int32_t TEST_VIDEO_HEIGHT = 1080;
}  // namespace

namespace OHOS {
namespace Media {
namespace Pipeline {
class MediaDemuxerMock : public MediaDemuxer {
public:
    MediaDemuxerMock()
    {
        std::cout << "MediaDemuxerMock" << std::endl;
    }

    std::vector<std::shared_ptr<Meta>> GetStreamMetaInfo()
    {
        return metaInfo_;
    }

protected:
    std::vector<std::shared_ptr<Meta>> metaInfo_{};
};

class FilterEventReceiverMock : public EventReceiver {
public:
    void OnEvent(const Event &event)
    {
        (void)event;
    }
};

class TestFilter : public Filter {
public:
    TestFilter():Filter("TestFilter", FilterType::FILTERTYPE_SOURCE) {}
    ~TestFilter() = default;
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
                            const std::shared_ptr<FilterLinkCallback>& callback)
    {
        (void)inType;
        (void)meta;
        (void)callback;
        return onLinked_;
    }
protected:
    Status onLinked_;
};

class FilterLinkCallbackMock : public FilterLinkCallback {
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta)
    {
        (void)queue;
        (void)meta;
        return;
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta)
    {
        (void)meta;
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta)
    {
        (void)meta;
    }
};

class FilterCallbackMock : public FilterCallback {
public:
    Status OnCallback(const std::shared_ptr<Filter> &filter, FilterCallBackCommand cmd, StreamType outType)
    {
        (void)filter;
        (void)cmd;
        (void)outType;
        return status_;
    }

protected:
    Status status_{ Status::OK };
};

class VideoStreamReadyCallbackMock : public VideoStreamReadyCallback {
public:
    bool IsVideoStreamDiscardable(const std::shared_ptr<AVBuffer> buffer)
    {
        (void)buffer;
        return true;
    }

protected:
};

void DemuxerFilterUnitTest::SetUpTestCase(void) {}

void DemuxerFilterUnitTest::TearDownTestCase(void) {}

void DemuxerFilterUnitTest::SetUp(void)
{
    demuxerFilter_ = std::make_shared<DemuxerFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_DEMUXER);
    auto receiver = std::make_shared<FilterEventReceiverMock>();
    demuxerFilter_->receiver_ = receiver;
    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    demuxerFilter_->SetDataSource(mediaSource);
}

void DemuxerFilterUnitTest::TearDown(void)
{
    demuxerFilter_ = nullptr;
}

void DemuxerFilterUnitTest::SetDataSource()
{
    string srtPath = "/data/test/media/camera_info_parser.mp4";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
}

/**
 * @tc.name: SetDataSource
 * @tc.desc: SetDataSource
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, SetDataSource, TestSize.Level1)
{
    auto res = demuxerFilter_->SetDataSource(nullptr);
    EXPECT_NE(res, Status::OK);
    std::cout << static_cast<int32_t>(res) << std::endl;

    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    res = demuxerFilter_->SetDataSource(mediaSource);
    std::cout << static_cast<int32_t>(res) << std::endl;
    EXPECT_EQ(res, Status::OK);
}

/**
 * @tc.name: DrmCallback
 * @tc.desc: DrmCallback
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DrmCallback, TestSize.Level1)
{
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter_->demuxer_ = demuxer;

    auto receiver = std::make_shared<FilterEventReceiverMock>();
    auto callback = std::make_shared<FilterCallbackMock>();
    demuxerFilter_->Init(receiver, callback);
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    demuxer->drmCallback_->OnDrmInfoChanged(drmInfo);

    demuxerFilter_ = nullptr;
    demuxer->drmCallback_->OnDrmInfoChanged(drmInfo);
    std::cout << "DrmCallback drmInfo.size()" << drmInfo.size() << std::endl;
    EXPECT_EQ(drmInfo.size(), 0);
}

/**
 * @tc.name: RegisterVideoStreamReadyCallback
 * @tc.desc: RegisterVideoStreamReadyCallback
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, RegisterVideoStreamReadyCallback, TestSize.Level1)
{
    auto readyCallback = std::make_shared<VideoStreamReadyCallbackMock>();
    demuxerFilter_->RegisterVideoStreamReadyCallback(readyCallback);
    demuxerFilter_->RegisterVideoStreamReadyCallback(nullptr);
    EXPECT_NE(readyCallback, nullptr);
}

/**
 * @tc.name: Init
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, Init, TestSize.Level1)
{
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter_->demuxer_ = demuxer;

    auto receiver = std::make_shared<FilterEventReceiverMock>();
    auto callback = std::make_shared<FilterCallbackMock>();
    demuxerFilter_->Init(receiver, callback);
    EXPECT_NE(demuxerFilter_, nullptr);
}

/**
 * @tc.name: DoPrepare
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DoPrepare, TestSize.Level1)
{
    auto res = demuxerFilter_->DoPrepare();
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;
    EXPECT_NE(res, Status::OK);

    std::vector<std::shared_ptr<Meta>> trackInfos;
    trackInfos.push_back(nullptr);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxer->metaInfo_ = trackInfos;
    demuxerFilter_->demuxer_ = demuxer;
    res = demuxerFilter_->DoPrepare();
    trackInfos.pop_back();
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;
    EXPECT_NE(res, Status::OK);

    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    trackInfos.push_back(meta);
    res = demuxerFilter_->DoPrepare();
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;
    EXPECT_NE(res, Status::OK);
}

/**
 * @tc.name: HandleTrackInfos
 * @tc.desc: HandleTrackInfos
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, HandleTrackInfos, TestSize.Level1)
{
    std::vector<std::shared_ptr<Meta>> trackInfos;
    int32_t successNodeCount = 0;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    trackInfos.push_back(meta);

    meta->Set<Tag::REGULAR_TRACK_ID>(0);
    auto res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MIME_TYPE>("invalid/xxx");
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MIME_TYPE>("audio/xxx");
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    meta->Set<Tag::MIME_TYPE>(MIME_IMAGE);
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::TIMEDMETA);
    meta->Set<Tag::MIME_TYPE>("");
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    meta->Set<Tag::MIME_TYPE>("video/xxx");
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    auto callback = std::make_shared<FilterCallbackMock>();
    demuxerFilter_->callback_ = callback;
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;

    callback->status_ = Status::ERROR_WRONG_STATE;
    res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    EXPECT_NE(res, Status::OK);
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;
}

/**
 * @tc.name: ShouldTrackSkipped
 * @tc.desc: ShouldTrackSkipped
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, ShouldTrackSkipped, TestSize.Level1)
{
    auto res = demuxerFilter_->ShouldTrackSkipped(Plugins::MediaType::VIDEO, "", 0);
    EXPECT_NE(res, true);

    res = demuxerFilter_->ShouldTrackSkipped(Plugins::MediaType::VIDEO, MIME_IMAGE, 0);
    EXPECT_EQ(res, true);

    res = demuxerFilter_->ShouldTrackSkipped(Plugins::MediaType::TIMEDMETA, "", 0);
    EXPECT_EQ(res, true);

    demuxerFilter_->disabledMediaTracks_.insert(Plugins::MediaType::VIDEO);
    res = demuxerFilter_->ShouldTrackSkipped(Plugins::MediaType::VIDEO, "", 0);
    EXPECT_EQ(res, true);

    res = demuxerFilter_->ShouldTrackSkipped(Plugins::MediaType::AUDIO, "", 0);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: CollectVideoAndAudioMime
 * @tc.desc: CollectVideoAndAudioMime
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, CollectVideoAndAudioMime, TestSize.Level1)
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
    demuxerFilter_->demuxer_ = mediaDemuxerMock;

    auto res = demuxerFilter_->CollectVideoAndAudioMime();

    std::cout << "CollectVideoAndAudioMime_0100 " << res << std::endl;
    EXPECT_NE(res, "");
}

/**
 * @tc.name: UpdateTrackIdMap
 * @tc.desc: UpdateTrackIdMap
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, UpdateTrackIdMap, TestSize.Level1)
{
    demuxerFilter_->UpdateTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO, 1);
    demuxerFilter_->UpdateTrackIdMap(StreamType::STREAMTYPE_RAW_AUDIO, 1);
    EXPECT_TRUE(demuxerFilter_->track_id_map_.size() > 0);
}

/**
 * @tc.name: FindTrackId
 * @tc.desc: FindTrackId
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, FindTrackId, TestSize.Level1)
{
    int32_t idTmp = 0;
    demuxerFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, idTmp);

    std::vector<int32_t> idVec;
    idVec.push_back(0);
    demuxerFilter_->track_id_map_.insert({ StreamType::STREAMTYPE_RAW_AUDIO, idVec });
    demuxerFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, idTmp);
    std::cout << "FindTrackId " << idTmp << std::endl;

    idVec.push_back(1);
    demuxerFilter_->track_id_map_.insert({ StreamType::STREAMTYPE_RAW_AUDIO, idVec });
    demuxerFilter_->FindTrackId(StreamType::STREAMTYPE_RAW_AUDIO, idTmp);
    std::cout << "FindTrackId " << idTmp << std::endl;
    EXPECT_TRUE(idTmp >= 0);
}

/**
 * @tc.name: FindStreamType
 * @tc.desc: FindStreamType
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, FindStreamType, TestSize.Level1)
{
    auto audio = StreamType::STREAMTYPE_RAW_AUDIO;
    shared_ptr<Meta> meta = make_shared<Meta>();
    auto res = demuxerFilter_->FindStreamType(audio, Plugins::MediaType::SUBTITLE, "", 0, meta);
    EXPECT_EQ(res, true);
    res = demuxerFilter_->FindStreamType(audio, Plugins::MediaType::AUDIO, std::string(MimeType::AUDIO_RAW), 0, meta);
    EXPECT_EQ(res, true);
    res = demuxerFilter_->FindStreamType(audio, Plugins::MediaType::VIDEO, "", 0, meta);
    EXPECT_EQ(res, true);
    res = demuxerFilter_->FindStreamType(audio, Plugins::MediaType::TIMEDMETA, "", 0, meta);
    EXPECT_EQ(res, false);
}

/**
 * @tc.name: OnLinkedResult
 * @tc.desc: OnLinkedResult
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, OnLinkedResult, TestSize.Level1)
{
    std::shared_ptr<Meta> meta0 = nullptr;
    demuxerFilter_->OnLinkedResult(nullptr, meta0);
    auto res = demuxerFilter_->DoStart();
    std::cout << "OnLinkedResult " << static_cast<int32_t>(res) << std::endl;

    auto meta = std::make_shared<Meta>();
    demuxerFilter_->OnLinkedResult(nullptr, meta);
    res = demuxerFilter_->DoStart();
    std::cout << "OnLinkedResult " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::REGULAR_TRACK_ID>(-1);
    demuxerFilter_->OnLinkedResult(nullptr, meta);
    res = demuxerFilter_->DoStart();
    std::cout << "OnLinkedResult " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::REGULAR_TRACK_ID>(0);
    demuxerFilter_->OnLinkedResult(nullptr, meta);
    res = demuxerFilter_->DoStart();
    std::cout << "OnLinkedResult " << static_cast<int32_t>(res) << std::endl;

    meta->Set<Tag::VIDEO_DECODER_RATE_UPPER_LIMIT>(0);
    demuxerFilter_->OnLinkedResult(nullptr, meta);
    res = demuxerFilter_->DoStart();
    std::cout << "OnLinkedResult " << static_cast<int32_t>(res) << std::endl;

    double frameRate = 2000.0;
    meta->Set<Tag::VIDEO_FRAME_RATE>(frameRate);
    demuxerFilter_->OnLinkedResult(nullptr, meta);
    res = demuxerFilter_->DoStart();
    std::cout << "OnLinkedResult " << static_cast<int32_t>(res) << std::endl;
    EXPECT_EQ(demuxerFilter_->instanceId_, false);
}

/**
 * @tc.name: OnDrmInfoUpdated
 * @tc.desc: OnDrmInfoUpdated
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, OnDrmInfoUpdated, TestSize.Level1)
{
    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    demuxerFilter_->OnDrmInfoUpdated(drmInfo);

    demuxerFilter_->receiver_ = nullptr;
    demuxerFilter_->OnDrmInfoUpdated(drmInfo);
    EXPECT_EQ(demuxerFilter_->isDump_, false);
}

/**
 * @tc.name: PrepareBeforeStart
 * @tc.desc: PrepareBeforeStart
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, PrepareBeforeStart, TestSize.Level1)
{
    auto res = demuxerFilter_->PrepareBeforeStart();
    demuxerFilter_->isLoopStarted = true;
    res = demuxerFilter_->PrepareBeforeStart();
    std::cout << "HandleTrackInfos " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);
}

/**
 * @tc.name: DoStart
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DoStart, TestSize.Level1)
{
    auto res = demuxerFilter_->DoStart();
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);

    demuxerFilter_->isLoopStarted = true;
    res = demuxerFilter_->DoStart();
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);
}

/**
 * @tc.name: PauseForSeek
 * @tc.desc: PauseForSeek
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, PauseForSeek, TestSize.Level1)
{
    // 1. it == nextFiltersMap_.end()
    auto res = demuxerFilter_->PauseForSeek();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);

    // 2. it != nextFiltersMap_.end(), it->second.size() != 1
    std::map<StreamType, std::vector<std::shared_ptr<Filter>>> nextFiltersMap;
    std::vector<std::shared_ptr<Filter>> vec;
    nextFiltersMap.insert({ StreamType::STREAMTYPE_ENCODED_VIDEO, vec });
    res = demuxerFilter_->PauseForSeek();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);

    // 3. it != nextFiltersMap_.end(), it->second.size() == 1, filter is nullptr
    vec.push_back(nullptr);
    res = demuxerFilter_->PauseForSeek();
    vec.pop_back();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);

    // 4. it != nextFiltersMap_.end(), it->second.size() == 1, filter not nullptr
    auto filter = std::make_shared<Filter>("filter", FilterType::FILTERTYPE_MAX);
    vec.push_back(filter);
    res = demuxerFilter_->PauseForSeek();
    std::cout << "PauseForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);
}

/**
 * @tc.name: ResumeForSeek
 * @tc.desc: ResumeForSeek
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, ResumeForSeek, TestSize.Level1)
{
    // 1. it == nextFiltersMap_.end()
    auto res = demuxerFilter_->ResumeForSeek();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);

    // 2. it != nextFiltersMap_.end(), it->second.size() != 1
    std::map<StreamType, std::vector<std::shared_ptr<Filter>>> nextFiltersMap;
    std::vector<std::shared_ptr<Filter>> vec;
    nextFiltersMap.insert({ StreamType::STREAMTYPE_ENCODED_VIDEO, vec });
    res = demuxerFilter_->ResumeForSeek();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);

    // 3. it != nextFiltersMap_.end(), it->second.size() == 1, filter is nullptr
    vec.push_back(nullptr);
    res = demuxerFilter_->ResumeForSeek();
    vec.pop_back();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);

    // 4. it != nextFiltersMap_.end(), it->second.size() == 1, filter not nullptr
    auto filter = std::make_shared<Filter>("filter", FilterType::FILTERTYPE_MAX);
    vec.push_back(filter);
    res = demuxerFilter_->ResumeForSeek();
    std::cout << "ResumeForSeek " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);
}

/**
 * @tc.name: ResumeForSeek
 * @tc.desc: ResumeForSeek
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, SetDumpFlag, TestSize.Level1)
{
    demuxerFilter_->SetDumpFlag(false);
    ASSERT_EQ(demuxerFilter_->isDump_, false);
    demuxerFilter_->demuxer_ = nullptr;
    demuxerFilter_->SetDumpFlag(false);
    ASSERT_NE(demuxerFilter_->isDump_, true);
}

/**
 * @tc.name: LinkNext
 * @tc.desc: LinkNext
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, LinkNext, TestSize.Level1)
{
    std::shared_ptr<Filter> nextFilter = std::make_shared<Filter>("filter", FilterType::FILTERTYPE_MAX);
    StreamType outType = StreamType::STREAMTYPE_RAW_AUDIO;
    auto res = demuxerFilter_->LinkNext(nextFilter, outType);
    std::cout << "LinkNext " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);

    std::vector<int32_t> idVec;
    idVec.push_back(0);
    demuxerFilter_->track_id_map_.insert({ StreamType::STREAMTYPE_RAW_AUDIO, idVec });
    res = demuxerFilter_->LinkNext(nextFilter, outType);
    std::cout << "LinkNext " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);
}

/**
 * @tc.name: GetBitRates
 * @tc.desc: GetBitRates
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, GetBitRates, TestSize.Level1)
{
    std::vector<uint32_t> bitRates;
    auto res = demuxerFilter_->GetBitRates(bitRates);
    std::cout << "GetBitRates " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);

    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    demuxerFilter_->SetDataSource(mediaSource);
    res = demuxerFilter_->GetBitRates(bitRates);
    std::cout << "GetBitRates " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);
}

/**
 * @tc.name: GetDownloadInfo
 * @tc.desc: GetDownloadInfo
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, GetDownloadInfo, TestSize.Level1)
{
    DownloadInfo info;
    auto res = demuxerFilter_->GetDownloadInfo(info);
    std::cout << "GetDownloadInfo " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);

    demuxerFilter_->demuxer_ = nullptr;
    res = demuxerFilter_->GetDownloadInfo(info);
    std::cout << "GetDownloadInfo " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);
}

/**
 * @tc.name: GetPlaybackInfo
 * @tc.desc: GetPlaybackInfo
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, GetPlaybackInfo, TestSize.Level1)
{
    PlaybackInfo info;
    auto res = demuxerFilter_->GetPlaybackInfo(info);
    std::cout << "GetPlaybackInfo " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);;

    demuxerFilter_->demuxer_ = nullptr;
    res = demuxerFilter_->GetPlaybackInfo(info);
    std::cout << "GetPlaybackInfo " << static_cast<int32_t>(res) << std::endl;
    ASSERT_NE(res, Status::OK);
}

/**
 * @tc.name: SelectBitRate
 * @tc.desc: SelectBitRate
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, SelectBitRate, TestSize.Level1)
{
    auto res = demuxerFilter_->SelectBitRate(0);
    std::cout << "SelectBitRate " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);

    demuxerFilter_->mediaSource_ = std::make_shared<MediaSource>("file:////");
    res = demuxerFilter_->SelectBitRate(0);
    demuxerFilter_->mediaSource_ = nullptr;
    res = demuxerFilter_->SelectBitRate(0);

    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    demuxerFilter_->SetDataSource(mediaSource);
    res = demuxerFilter_->SelectBitRate(0);
    std::cout << "SelectBitRate " << static_cast<int32_t>(res) << std::endl;
    ASSERT_EQ(res, Status::OK);
}

/**
 * @tc.name: OnDumpInfo
 * @tc.desc: OnDumpInfo
 * @tc.type: FUNC
*/
HWTEST_F(DemuxerFilterUnitTest, OnDumpInfo, TestSize.Level1)
{
    auto fd = AshmemCreate("DemuxerFilterUnitTest::OnDumpInfo", 4096);
    char* temp = new char[8];
    ON_SCOPE_EXIT(0) {
        if (fd > 0) {
            (void)::close(fd);
        }
        delete [] temp;
    };

    ASSERT_GE(fd, 0);
    demuxerFilter_->OnDumpInfo(fd);
    read(fd, temp, 8);
    std::cout << "OnDumpInfo " << temp << std::endl;
    ASSERT_NE(temp[0], ' ');

    demuxerFilter_->demuxer_ = nullptr;
    demuxerFilter_->SetBundleName("xxxxx");
    demuxerFilter_->OnDumpInfo(0);
    ASSERT_NE(temp[0], ' ');
}

/**
 * @tc.name: GetBitRates_0200
 * @tc.desc: GetBitRates
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, GetBitRates_0200, TestSize.Level1)
{
    std::vector<uint32_t> bitRates;
    demuxerFilter_->mediaSource_ = nullptr;
    demuxerFilter_->GetBitRates(bitRates);
    demuxerFilter_->mediaSource_ = std::make_shared<MediaSource>("file:////");
    demuxerFilter_->GetBitRates(bitRates);
    ASSERT_EQ(demuxerFilter_->isDump_, false);
}

HWTEST_F(DemuxerFilterUnitTest, UnLinkNext_0400, TestSize.Level1)
{
    std::shared_ptr<TestFilter> filter = std::make_shared<TestFilter>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status status = demuxerFilter_->UnLinkNext(filter, streamType);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DemuxerFilterUnitTest, UpdateNext_0400, TestSize.Level1)
{
    std::shared_ptr<TestFilter> filter = std::make_shared<TestFilter>();
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    Status status = demuxerFilter_->UpdateNext(filter, streamType);
    EXPECT_EQ(status, Status::OK);
}

HWTEST_F(DemuxerFilterUnitTest, GetFilterType_0400, TestSize.Level1)
{
    demuxerFilter_->filterType_ = FilterType::FILTERTYPE_DEMUXER;
    FilterType res = demuxerFilter_->GetFilterType();
    EXPECT_EQ(res, FilterType::FILTERTYPE_DEMUXER);
}

HWTEST_F(DemuxerFilterUnitTest, OnLinked_0400, TestSize.Level1)
{
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    Status res = demuxerFilter_->OnLinked(streamType, meta, callback);
    EXPECT_EQ(res, Status::OK);
}

HWTEST_F(DemuxerFilterUnitTest, OnUpdated_0400, TestSize.Level1)
{
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    Status res = demuxerFilter_->OnUpdated(streamType, meta, callback);
    EXPECT_EQ(res, Status::OK);
}

HWTEST_F(DemuxerFilterUnitTest, OnUnLinked_0400, TestSize.Level1)
{
    StreamType streamType = StreamType::STREAMTYPE_ENCODED_VIDEO;
    auto callback = std::make_shared<FilterLinkCallbackMock>();
    Status res = demuxerFilter_->OnUnLinked(streamType, callback);
    EXPECT_EQ(res, Status::OK);
}

HWTEST_F(DemuxerFilterUnitTest, IsLocalFd_0400, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_DEMUXER);
    auto receiver = std::make_shared<FilterEventReceiverMock>();
    demuxerFilter->receiver_ = receiver;
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    demuxerFilter->SetDataSource(mediaSource);
    EXPECT_TRUE(demuxerFilter->IsLocalFd());
}

HWTEST_F(DemuxerFilterUnitTest, Dts2FrameId_0100, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto receiver = std::make_shared<FilterEventReceiverMock>();
    demuxerFilter->receiver_ = receiver;
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE2);
    demuxerFilter->SetDataSource(mediaSource);
    uint32_t frameId = 0;
    Status ret = Status::OK;
    ret = demuxerFilter->Dts2FrameId(-40000, frameId);
    EXPECT_EQ(frameId, 1);
    EXPECT_EQ(ret, Status::OK);
    ret = demuxerFilter->Dts2FrameId(480000, frameId);
    EXPECT_EQ(frameId, 14);
    EXPECT_EQ(ret, Status::OK);
    ret = demuxerFilter->Dts2FrameId(920000, frameId);
    EXPECT_EQ(frameId, 25);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SetMediaMuted
 * @tc.desc: SetMediaMuted
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, SetMediaMuted, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    Status ret = Status::OK;
    ret = demuxerFilter->SetMediaMuted(Media::MediaType::MEDIA_TYPE_VID, true);
    ASSERT_EQ(ret, Status::OK);
}

/**
 * @tc.name: NotifyResumeUnMute_0100
 * @tc.desc: NotifyResumeUnMute
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, NotifyResumeUnMute_0100, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    Status ret = demuxerFilter->NotifyResumeUnMute();
    ASSERT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SetPlayStrategy_0100
 * @tc.desc: Test DemuxerFilter::SetPlayStrategy
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, SetPlayStrategy_0100, TestSize.Level1)
{
    demuxerFilter_->demuxer_ = nullptr;
    Status ret = demuxerFilter_->SetPlayStrategy(nullptr);
    EXPECT_NE(ret, Status::OK);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter_->demuxer_ = demuxer;
    ret = demuxerFilter_->SetPlayStrategy(nullptr);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
    auto playStrategy = std::make_shared<PlayStrategy>();
    playStrategy->width = TEST_VIDEO_WIDTH;
    playStrategy->height = TEST_VIDEO_HEIGHT;
    playStrategy->audioLanguage = "eng";
    playStrategy->subtitleLanguage = "zh_CN";
    ret = demuxerFilter_->SetPlayStrategy(playStrategy);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SelectTrack_0100
 * @tc.desc: Test DemuxerFilter::SelectTrack
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, SelectTrack_0100, TestSize.Level1)
{
    demuxerFilter_->demuxer_ = nullptr;
    Status ret = demuxerFilter_->SelectTrack(0);
    EXPECT_NE(ret, Status::OK);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter_->demuxer_ = demuxer;
    ret = demuxerFilter_->SelectTrack(0);
    EXPECT_NE(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: SetTrackSelectionFilter_0100
 * @tc.desc: Test DemuxerFilter::SetTrackSelectionFilter
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, SetTrackSelectionFilter_0100, TestSize.Level1)
{
    TrackSelectionFilter filter;
    filter.maxVideoBitrate = 8000000;
    filter.minVideoBitrate = 1000000;
    filter.maxVideoFrameRate = 60;
    filter.minVideoFrameRate = 24;
    filter.maxVideoResolution = std::make_pair(TEST_VIDEO_WIDTH, TEST_VIDEO_HEIGHT);
    filter.minVideoResolution = std::make_pair(1280, 720);
    filter.preferredVideoMimeTypes = std::vector<std::string>{"video/avc"};
    filter.maxAudioBitrate = 320000;
    filter.minAudioBitrate = 64000;
    filter.maxAudioChannels = 2;
    filter.preferredAudioMimeTypes = std::vector<std::string>{"audio/mp4a-latm"};
    filter.preferredAudioLanguages = std::vector<std::string>{"eng"};
    filter.preferredSubtitleLanguages = std::vector<std::string>{"zh"};

    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter_->demuxer_ = demuxer;
    demuxerFilter_->SetTrackSelectionFilter(filter);

    EXPECT_EQ(demuxer->trackSelectionFilter_.maxVideoBitrate, filter.maxVideoBitrate);
    EXPECT_EQ(demuxer->trackSelectionFilter_.minVideoBitrate, filter.minVideoBitrate);
    EXPECT_EQ(demuxer->trackSelectionFilter_.maxVideoFrameRate, filter.maxVideoFrameRate);
    EXPECT_EQ(demuxer->trackSelectionFilter_.minVideoFrameRate, filter.minVideoFrameRate);
    EXPECT_EQ(demuxer->trackSelectionFilter_.maxVideoResolution, filter.maxVideoResolution);
    EXPECT_EQ(demuxer->trackSelectionFilter_.minVideoResolution, filter.minVideoResolution);
    EXPECT_EQ(demuxer->trackSelectionFilter_.preferredVideoMimeTypes, filter.preferredVideoMimeTypes);
    EXPECT_EQ(demuxer->trackSelectionFilter_.maxAudioBitrate, filter.maxAudioBitrate);
    EXPECT_EQ(demuxer->trackSelectionFilter_.minAudioBitrate, filter.minAudioBitrate);
    EXPECT_EQ(demuxer->trackSelectionFilter_.maxAudioChannels, filter.maxAudioChannels);
    EXPECT_EQ(demuxer->trackSelectionFilter_.preferredAudioMimeTypes, filter.preferredAudioMimeTypes);
    EXPECT_EQ(demuxer->trackSelectionFilter_.preferredAudioLanguages, filter.preferredAudioLanguages);
    EXPECT_EQ(demuxer->trackSelectionFilter_.preferredSubtitleLanguages, filter.preferredSubtitleLanguages);

    demuxerFilter_->demuxer_ = nullptr;
    demuxerFilter_->SetTrackSelectionFilter(filter);
    SUCCEED();
}

/**
 * @tc.name: GetSeekableRanges_0100
 * @tc.desc: GetSeekableRanges
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, GetSeekableRanges_0100, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    auto ret = demuxerFilter->GetSeekableRanges();
    ASSERT_EQ(ret.size(), 0);
}

/**
 * @tc.name: GetLoadedRanges_0100
 * @tc.desc: GetLoadedRanges
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, GetLoadedRanges_0100, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    auto ret = demuxerFilter->GetLoadedRanges();
    ASSERT_EQ(ret.size(), 0);
}

/**
 * @tc.name: IsAVTrackInvalid_PlayerMode
 * @tc.desc: Test IsAVTrackInvalid in player mode with various parameter combinations
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, IsAVTrackInvalid_PlayerMode, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    demuxerFilter->isPlayerMode_ = true;
    
    // Test Case 1: Both video and audio invalid, demuxer has no video and no audio
    // Expected: (true && true) || (true && true) || (true && true) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    bool result = demuxerFilter->IsAVTrackInvalid(true, true, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 2: Only video invalid, no audio track, demuxer has no video but has audio
    // Expected: (false && false) || (true && true) || (false && false) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 3: Only audio invalid, no video filter, demuxer has video but no audio
    // Expected: (false && true) || (false && false) || (true && true) = true
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, false, true);
    EXPECT_TRUE(result);
    
    // Test Case 4: Both valid, hasVideoFilter=true, hasAudioTrack=true, demuxer has both
    // Expected: (false && false) || (false && false) || (false && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 5: Video invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has video
    // Expected: (false && false) || (false && false) || (false && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 6: Video invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has video but no audio
    // Expected: (false && false) || (false && true) || (false && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 7: Audio invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has no video but has audio
    // Expected: (false && false) || (false && false) || (false && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 8: Both invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has both
    // isVideoInvalid = true && !true = false, isAudioInvalid = true && !true = false
    // isInvalid = (false && false) || (false && !true) || (false && !true) = false || false || false = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 9: Both valid, hasVideoFilter=true, hasAudioTrack=true, demuxer has no video and no audio
    // Expected: (false && false) || (false && true) || (false && true) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 10: Video invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has no video and no audio
    // isVideoInvalid = true && !false = true, isAudioInvalid = false && !false = false
    // isInvalid = (true && false) || (true && !true) || (false && !true) = false || false || false = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 11: Audio invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has no video and no audio
    // isVideoInvalid = false && !false = false, isAudioInvalid = true && !false = true
    // isInvalid = (false && true) || (false && !true) || (true && !true) = false || false || false = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 12: Both invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has no video but has audio
    // isVideoInvalid = true && !false = true, isAudioInvalid = true && !true = false
    // isInvalid = (true && false) || (true && !true) || (false && !true) = false || false || false = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 13: Both valid, hasVideoFilter=true, hasAudioTrack=true, demuxer has no video but has audio
    // Expected: (false && false) || (false && false) || (false && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 14: Video invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has no video but has audio
    // Expected: (false && false) || (true && false) || (false && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 15: Both invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has video but no audio
    // isVideoInvalid = true && !true = false, isAudioInvalid = true && !false = true
    // isInvalid = (false && true) || (false && !true) || (true && !true) = false || false || false = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 16: Both valid, hasVideoFilter=true, hasAudioTrack=true, demuxer has video but no audio
    // Expected: (false && false) || (false && true) || (false && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 17: Audio invalid, hasVideoFilter=true, hasAudioTrack=true, demuxer has video but no audio
    // Expected: (false && false) || (false && true) || (true && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, true, true);
    EXPECT_FALSE(result);
    
    // Test Case 18: Both valid, hasVideoFilter=false, hasAudioTrack=true, demuxer has no video but has audio
    // Expected: (false && false) || (false && false) || (false && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, false, true);
    EXPECT_FALSE(result);
    
    // Test Case 19: Video invalid, hasVideoFilter=false, hasAudioTrack=true, demuxer has no video but has audio
    // Expected: (false && false) || (true && false) || (false && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, false, true);
    EXPECT_FALSE(result);
    
    // Test Case 20: Both invalid, hasVideoFilter=false, hasAudioTrack=true, demuxer has no video but has audio
    // Expected: (true && false) || (true && false) || (true && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, false, true);
    EXPECT_FALSE(result);
    
    // Test Case 21: Both valid, hasVideoFilter=true, hasAudioTrack=false, demuxer has video but no audio
    // Expected: (false && false) || (false && true) || (false && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, true, false);
    EXPECT_FALSE(result);
    
    // Test Case 22: Audio invalid, hasVideoFilter=true, hasAudioTrack=false, demuxer has video but no audio
    // isVideoInvalid = false && !true = false, isAudioInvalid = true && !false = true
    // isInvalid = (false && true) || (false && !false) || (true && !true) = false || false || false = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, true, false);
    EXPECT_FALSE(result);
    
    // Test Case 23: Both invalid, hasVideoFilter=true, hasAudioTrack=false, demuxer has video but no audio
    // isVideoInvalid = true && !true = false, isAudioInvalid = true && !false = true
    // isInvalid = (false && true) || (false && !false) || (true && !true) = false || false || false = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, true, false);
    EXPECT_FALSE(result);
    
    // Test Case 24: Both valid, hasVideoFilter=false, hasAudioTrack=true, demuxer has no video and no audio
    // isVideoInvalid = false && !false = false, isAudioInvalid = false && !false = false
    // isInvalid = (false && false) || (false && !true) || (false && !false) = false || false || false = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, false, true);
    EXPECT_FALSE(result);
    
    // Test Case 25: Video invalid, hasVideoFilter=false, hasAudioTrack=true, demuxer has no video and no audio
    // isVideoInvalid = true && !false = true, isAudioInvalid = false && !false = false
    // isInvalid = (true && false) || (true && !true) || (false && !false) = false || false || false = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, false, true);
    EXPECT_FALSE(result);
    
    // Test Case 26: Both invalid, hasVideoFilter=false, hasAudioTrack=true, demuxer has no video and no audio
    // isVideoInvalid = true && !false = true, isAudioInvalid = true && !false = true
    // isInvalid = (true && true) || (true && !true) || (true && !false) = true || false || true = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, false, true);
    EXPECT_TRUE(result);
    
    // Test Case 27: Both valid, hasVideoFilter=true, hasAudioTrack=false, demuxer has no video and no audio
    // isVideoInvalid = false && !false = false, isAudioInvalid = false && !false = false
    // isInvalid = (false && false) || (false && !false) || (false && !true) = false || false || false = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, true, false);
    EXPECT_FALSE(result);
    
    // Test Case 28: Video invalid, hasVideoFilter=true, hasAudioTrack=false, demuxer has no video and no audio
    // isVideoInvalid = true && !false = true, isAudioInvalid = false && !false = false
    // isInvalid = (true && false) || (true && !false) || (false && !true) = false || true || false = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, true, false);
    EXPECT_TRUE(result);
    
    // Test Case 29: Both invalid, hasVideoFilter=true, hasAudioTrack=false, demuxer has no video and no audio
    // Expected: (true && false) || (true && true) || (true && true) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, true, false);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: IsAVTrackInvalid_NonPlayerMode
 * @tc.desc: Test IsAVTrackInvalid in non-player mode with various parameter combinations
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, IsAVTrackInvalid_NonPlayerMode, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    demuxerFilter->isPlayerMode_ = false;
    
    // Test Case 1: Invalid video, demuxer has no video but has audio
    // Expected: (true && true) || (false && false) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    bool result = demuxerFilter->IsAVTrackInvalid(true, false, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 2: Invalid audio, demuxer has video but no audio
    // Expected: (false && false) || (true && true) = true
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 3: Both invalid, demuxer has no video and no audio
    // Expected: (true && true) || (true && true) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 4: Both valid, demuxer has both video and audio
    // Expected: (false && false) || (false && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 5: Invalid video, demuxer has video and audio
    // Expected: (true && false) || (false && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 6: Invalid audio, demuxer has video and audio
    // Expected: (false && false) || (true && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 7: Both invalid, demuxer has video but no audio
    // Expected: (true && false) || (true && true) = true
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 8: Both invalid, demuxer has no video but has audio
    // Expected: (true && true) || (true && false) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 9: Both valid, demuxer has no video and no audio
    // Expected: (false && true) || (false && true) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 10: Video invalid, demuxer has no video and no audio
    // Expected: (true && true) || (false && true) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 11: Audio invalid, demuxer has no video and no audio
    // Expected: (false && true) || (true && true) = true
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, false, false);
    EXPECT_TRUE(result);
    
    // Test Case 12: Both valid, demuxer has no video but has audio
    // Expected: (false && true) || (false && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 13: Audio invalid, demuxer has no video but has audio
    // Expected: (false && true) || (true && false) = false
    demuxer->videoTrackId_ = -1;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(false, true, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 14: Both valid, demuxer has video but no audio
    // Expected: (false && false) || (false && true) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(false, false, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 15: Video invalid, demuxer has video but no audio
    // Expected: (true && false) || (false && true) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = -1;
    result = demuxerFilter->IsAVTrackInvalid(true, false, false, false);
    EXPECT_FALSE(result);
    
    // Test Case 16: Both invalid, demuxer has both video and audio
    // Expected: (true && false) || (true && false) = false
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = 1;
    result = demuxerFilter->IsAVTrackInvalid(true, true, false, false);
    EXPECT_FALSE(result);
}
/**
 * @tc.name  : Test SetSyncCenter
 * @tc.number: SetSyncCenter_Interstitial_001
 * @tc.desc  : Test SetSyncCenter stores syncCenter_ and propagates to demuxer
 */
HWTEST_F(DemuxerFilterUnitTest, SetSyncCenter_Interstitial_001, TestSize.Level1)
{
    auto syncMgr = std::make_shared<MediaSyncManager>();
    demuxerFilter_->SetSyncCenter(syncMgr);
    EXPECT_EQ(demuxerFilter_->syncCenter_, syncMgr);
}

/**
 * @tc.name  : Test SetSyncCenter
 * @tc.number: SetSyncCenter_Interstitial_002
 * @tc.desc  : Test SetSyncCenter with null demuxer_
 */
HWTEST_F(DemuxerFilterUnitTest, SetSyncCenter_Interstitial_002, TestSize.Level1)
{
    demuxerFilter_->demuxer_ = nullptr;
    auto syncMgr = std::make_shared<MediaSyncManager>();
    demuxerFilter_->SetSyncCenter(syncMgr);
    EXPECT_EQ(demuxerFilter_->syncCenter_, nullptr);
}

/**
 * @tc.name  : Test HandleAdsStop
 * @tc.number: HandleAdsStop_001
 * @tc.desc  : Test HandleAdsStop with no controller and no scheduler
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsStop_001, TestSize.Level1)
{
    demuxerFilter_->interstitialController_ = nullptr;
    demuxerFilter_->HandleAdsStop();
    EXPECT_EQ(demuxerFilter_->interstitialController_, nullptr);
}

/**
 * @tc.name  : Test HandleAdsStop
 * @tc.number: HandleAdsStop_002
 * @tc.desc  : Test HandleAdsStop with controller set
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsStop_002, TestSize.Level1)
{
    auto controller = std::make_shared<InterstitialController>();
    demuxerFilter_->interstitialController_ = controller;
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    controller->AddAdsMediaSource(source, 1000, outId);
    demuxerFilter_->HandleAdsStop();
    EXPECT_FALSE(controller->HasPendingEvents());
}

/**
 * @tc.name  : Test HandleAdsSeek
 * @tc.number: HandleAdsSeek_001
 * @tc.desc  : Test HandleAdsSeek with no controller and no scheduler
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsSeek_001, TestSize.Level1)
{
    demuxerFilter_->interstitialController_ = nullptr;
    demuxerFilter_->HandleAdsSeek(3000);
    EXPECT_EQ(demuxerFilter_->interstitialController_, nullptr);
}

/**
 * @tc.name  : Test HandleAdsSeek
 * @tc.number: HandleAdsSeek_002
 * @tc.desc  : Test HandleAdsSeek with controller set (VOD)
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsSeek_002, TestSize.Level1)
{
    auto controller = std::make_shared<InterstitialController>();
    controller->SetLiveSource(false);
    demuxerFilter_->interstitialController_ = controller;
    auto source1 = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<MediaSource>("http://example.com/ad2.mp4");
    std::string outId1, outId2;
    controller->AddAdsMediaSource(source1, 1000, outId1);
    controller->AddAdsMediaSource(source2, 5000, outId2);
    demuxerFilter_->HandleAdsSeek(3000);
    EXPECT_TRUE(controller->entries_[outId1].played);
    EXPECT_FALSE(controller->entries_[outId2].played);
}

/**
 * @tc.name  : Test AddAdsMediaSource
 * @tc.number: FilterAddAds_001
 * @tc.desc  : Test AddAdsMediaSource with null demuxer_
 */
HWTEST_F(DemuxerFilterUnitTest, FilterAddAds_001, TestSize.Level1)
{
    demuxerFilter_->demuxer_ = nullptr;
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    auto ret = demuxerFilter_->AddAdsMediaSource(source, 1000, false, outId);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test AddAdsMediaSource
 * @tc.number: FilterAddAds_002
 * @tc.desc  : Test AddAdsMediaSource creates controller and scheduler
 */
HWTEST_F(DemuxerFilterUnitTest, FilterAddAds_002, TestSize.Level1)
{
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    auto ret = demuxerFilter_->AddAdsMediaSource(source, 1000, false, outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(demuxerFilter_->interstitialController_, nullptr);
    EXPECT_EQ(outId, "ad_0");
}

/**
 * @tc.name  : Test AddAdsMediaSource
 * @tc.number: FilterAddAds_003
 * @tc.desc  : Test AddAdsMediaSource with live source
 */
HWTEST_F(DemuxerFilterUnitTest, FilterAddAds_003, TestSize.Level1)
{
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    auto ret = demuxerFilter_->AddAdsMediaSource(source, 1000, true, outId);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(demuxerFilter_->interstitialController_->IsLiveSource());
}

/**
 * @tc.name  : Test RemoveAdsMediaSource
 * @tc.number: FilterRemoveAds_001
 * @tc.desc  : Test RemoveAdsMediaSource with null controller
 */
HWTEST_F(DemuxerFilterUnitTest, FilterRemoveAds_001, TestSize.Level1)
{
    demuxerFilter_->interstitialController_ = nullptr;
    auto ret = demuxerFilter_->RemoveAdsMediaSource("ad_0");
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test RemoveAdsMediaSource
 * @tc.number: FilterRemoveAds_002
 * @tc.desc  : Test RemoveAdsMediaSource with valid controller
 */
HWTEST_F(DemuxerFilterUnitTest, FilterRemoveAds_002, TestSize.Level1)
{
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    demuxerFilter_->AddAdsMediaSource(source, 1000, false, outId);
    auto ret = demuxerFilter_->RemoveAdsMediaSource(outId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DisableAllAdsMediaSource
 * @tc.number: FilterDisableAds_001
 * @tc.desc  : Test DisableAllAdsMediaSource with valid controller
 */
HWTEST_F(DemuxerFilterUnitTest, FilterDisableAds_001, TestSize.Level1)
{
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    demuxerFilter_->AddAdsMediaSource(source, 1000, false, outId);
    auto ret = demuxerFilter_->DisableAllAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(demuxerFilter_->interstitialController_->IsAdsDisabled());
}

/**
 * @tc.name  : Test DisableAllAdsMediaSource
 * @tc.number: FilterDisableAds_002
 * @tc.desc  : Test DisableAllAdsMediaSource without adding any ads, auto-creates controller
 */
HWTEST_F(DemuxerFilterUnitTest, FilterDisableAds_002, TestSize.Level1)
{
    auto ret = demuxerFilter_->DisableAllAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(demuxerFilter_->interstitialController_, nullptr);
    EXPECT_TRUE(demuxerFilter_->interstitialController_->IsAdsDisabled());
}

/**
 * @tc.name  : Test DisableAllAdsMediaSource
 * @tc.number: FilterDisableAds_003
 * @tc.desc  : Test DisableAllAdsMediaSource called multiple times is idempotent
 */
HWTEST_F(DemuxerFilterUnitTest, FilterDisableAds_003, TestSize.Level1)
{
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    demuxerFilter_->AddAdsMediaSource(source, 1000, false, outId);
    EXPECT_EQ(demuxerFilter_->DisableAllAdsMediaSource(), Status::OK);
    EXPECT_EQ(demuxerFilter_->DisableAllAdsMediaSource(), Status::OK);
    EXPECT_EQ(demuxerFilter_->DisableAllAdsMediaSource(), Status::OK);
    EXPECT_TRUE(demuxerFilter_->interstitialController_->IsAdsDisabled());
}

/**
 * @tc.name  : Test DisableAllAdsMediaSource
 * @tc.number: FilterDisableAds_004
 * @tc.desc  : Test DisableAllAdsMediaSource with multiple ads, verify not playing after disable
 */
HWTEST_F(DemuxerFilterUnitTest, FilterDisableAds_004, TestSize.Level1)
{
    std::string outId;
    auto source1 = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    auto source2 = std::make_shared<MediaSource>("http://example.com/ad2.mp4");
    auto source3 = std::make_shared<MediaSource>("http://example.com/ad3.mp4");
    demuxerFilter_->AddAdsMediaSource(source1, 1000, false, outId);
    demuxerFilter_->AddAdsMediaSource(source2, 2000, false, outId);
    demuxerFilter_->AddAdsMediaSource(source3, 3000, false, outId);
    auto ret = demuxerFilter_->DisableAllAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(demuxerFilter_->interstitialController_->IsAdsDisabled());
    EXPECT_FALSE(demuxerFilter_->interstitialController_->IsPlayingInterstitial());
}

/**
 * @tc.name  : Test SkipCurrentAdsMediaSource
 * @tc.number: FilterSkipAds_001
 * @tc.desc  : Test SkipCurrentAdsMediaSource with null controller
 */
HWTEST_F(DemuxerFilterUnitTest, FilterSkipAds_001, TestSize.Level1)
{
    demuxerFilter_->interstitialController_ = nullptr;
    auto ret = demuxerFilter_->SkipCurrentAdsMediaSource();
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test SkipCurrentAdsMediaSource
 * @tc.number: FilterSkipAds_002
 * @tc.desc  : Test SkipCurrentAdsMediaSource in IDLE state
 */
HWTEST_F(DemuxerFilterUnitTest, FilterSkipAds_002, TestSize.Level1)
{
    auto source = std::make_shared<MediaSource>("http://example.com/ad1.mp4");
    std::string outId;
    demuxerFilter_->AddAdsMediaSource(source, 1000, false, outId);
    auto ret = demuxerFilter_->SkipCurrentAdsMediaSource();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test EnsureInterstitialController
 * @tc.number: EnsureController_001
 * @tc.desc  : Test EnsureInterstitialController lazy creation
 */
HWTEST_F(DemuxerFilterUnitTest, EnsureController_001, TestSize.Level1)
{
    EXPECT_EQ(demuxerFilter_->interstitialController_, nullptr);
    demuxerFilter_->EnsureInterstitialController();
    EXPECT_NE(demuxerFilter_->interstitialController_, nullptr);
}

/**
 * @tc.name  : Test EnsureInterstitialController
 * @tc.number: EnsureController_002
 * @tc.desc  : Test EnsureInterstitialController idempotent
 */
HWTEST_F(DemuxerFilterUnitTest, EnsureController_002, TestSize.Level1)
{
    demuxerFilter_->EnsureInterstitialController();
    auto first = demuxerFilter_->interstitialController_;
    demuxerFilter_->EnsureInterstitialController();
    EXPECT_EQ(demuxerFilter_->interstitialController_, first);
}

/**
 * @tc.name  : Test EnsureInterstitialScheduler
 * @tc.number: EnsureScheduler_001
 * @tc.desc  : Test EnsureInterstitialScheduler lazy creation
 */
HWTEST_F(DemuxerFilterUnitTest, EnsureScheduler_001, TestSize.Level1)
{
    demuxerFilter_->EnsureInterstitialController();
    demuxerFilter_->EnsureInterstitialScheduler();
    EXPECT_NE(demuxerFilter_->demuxer_->GetInterstitialScheduler(), nullptr);
}

// ==================== HandleAdsStart ====================

/**
 * @tc.name  : Test HandleAdsStart
 * @tc.number: HandleAdsStart_001
 * @tc.desc  : Test HandleAdsStart returns false with no controller
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsStart_001, TestSize.Level1)
{
    demuxerFilter_->interstitialController_ = nullptr;
    EXPECT_FALSE(demuxerFilter_->HandleAdsStart());
}

/**
 * @tc.name  : Test HandleAdsStart
 * @tc.number: HandleAdsStart_002
 * @tc.desc  : Test HandleAdsStart returns false with controller but no pre-roll ad
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsStart_002, TestSize.Level1)
{
    auto controller = std::make_shared<InterstitialController>();
    demuxerFilter_->interstitialController_ = controller;
    EXPECT_FALSE(demuxerFilter_->HandleAdsStart());
}

/**
 * @tc.name  : Test HandleAdsStart
 * @tc.number: HandleAdsStart_003
 * @tc.desc  : Test HandleAdsStart returns false when ads are disabled
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsStart_003, TestSize.Level1)
{
    auto controller = std::make_shared<InterstitialController>();
    controller->DisableAllAdsMediaSource();
    demuxerFilter_->interstitialController_ = controller;
    EXPECT_FALSE(demuxerFilter_->HandleAdsStart());
}

/**
 * @tc.name  : Test HandleAdsStart
 * @tc.number: HandleAdsStart_004
 * @tc.desc  : Test HandleAdsStart returns false when pre-roll ad TryPreRollAd does not start playing
 */
HWTEST_F(DemuxerFilterUnitTest, HandleAdsStart_004, TestSize.Level1)
{
    demuxerFilter_->EnsureInterstitialController();
    demuxerFilter_->interstitialController_->SetLiveSource(false);
    demuxerFilter_->interstitialController_->SetMainContentUri("http://example.com/main.mp4");

    auto source = std::make_shared<MediaSource>("http://example.com/preroll.mp4");
    std::string outId;
    demuxerFilter_->AddAdsMediaSource(source, 0, false, outId);
    EXPECT_EQ(demuxerFilter_->interstitialController_->preRollAdId_, outId);

    EXPECT_FALSE(demuxerFilter_->HandleAdsStart());
}

/**
 * @tc.name  : Test IsDsdFormat
 * @tc.number: IsDsdFormat_001
 * @tc.desc  : Test IsDsdFormat with various mime strings (case-insensitive)
 */
HWTEST_F(DemuxerFilterUnitTest, IsDsdFormat_001, TestSize.Level1)
{
    EXPECT_TRUE(demuxerFilter_->IsDsdFormat("audio/dsd"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("AUDIO/DSD"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("AUDIO_DSD"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("Audio_Dsd"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("audio_dsd"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("audio_x_audio_dsd"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("audio/mp4a-latm"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("audio/raw"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("video/avc"));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat(""));
    EXPECT_FALSE(demuxerFilter_->IsDsdFormat("audio_dsa"));
}
 
HWTEST_F(DemuxerFilterUnitTest, HandleTrackInfos_dsd, TestSize.Level1)
{
    std::vector<std::shared_ptr<Meta>> trackInfos;
    int32_t successNodeCount = 0;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    trackInfos.push_back(meta);
 
    meta->Set<Tag::REGULAR_TRACK_ID>(0);
    meta->Set<Tag::MIME_TYPE>(MimeType::AUDIO_DSD);
    meta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
 
    auto callback = std::make_shared<FilterCallbackMock>();
    demuxerFilter_->callback_ = callback;
    auto res = demuxerFilter_->HandleTrackInfos(trackInfos, successNodeCount);
    std::cout << "HandleTrackInfos_dsd " << static_cast<int32_t>(res) << std::endl;
 
    EXPECT_EQ(res, Status::OK);
}
 
/**
 * @tc.name: DemuxerFilter_SetTimeRange_0100
 * @tc.desc: SetTimeRange sets the start and end time
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DemuxerFilter_SetTimeRange_0100, TestSize.Level1)
{
    int64_t start = 1000;
    int64_t end = 5000;
    demuxerFilter_->SetTimeRange(start, end);
    EXPECT_EQ(demuxerFilter_->startTimeMs_, start);
    EXPECT_EQ(demuxerFilter_->endTimeMs_, end);
}

/**
 * @tc.name: DemuxerFilter_SeekToClipStartTime_0100
 * @tc.desc: SeekToClipStartTime returns error when demuxer is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DemuxerFilter_SeekToClipStartTime_0100, TestSize.Level1)
{
    demuxerFilter_->demuxer_ = nullptr;
    auto res = demuxerFilter_->SeekToClipStartTime();
    EXPECT_NE(res, Status::OK);
}

/**
 * @tc.name: DemuxerFilter_SeekToClipStartTime_0200
 * @tc.desc: SeekToClipStartTime returns error when startTimeMs_ <= 0
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DemuxerFilter_SeekToClipStartTime_0200, TestSize.Level1)
{
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter_->demuxer_ = demuxer;
    demuxerFilter_->startTimeMs_ = 0;
    auto res = demuxerFilter_->SeekToClipStartTime();
    EXPECT_NE(res, Status::OK);
}

/**
 * @tc.name: DemuxerFilter_GetTranscoderStartPts_0100
 * @tc.desc: GetTranscoderStartPts with no track metadata returns 0
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DemuxerFilter_GetTranscoderStartPts_0100, TestSize.Level1)
{
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter_->demuxer_ = demuxer;
    int64_t startPts = demuxerFilter_->GetTranscoderStartPts();
    EXPECT_EQ(startPts, 0);
}

/**
 * @tc.name: DemuxerFilter_GetTranscoderStartPts_0200
 * @tc.desc: GetTranscoderStartPts with both video and audio tracks
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DemuxerFilter_GetTranscoderStartPts_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    videoMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    videoMeta->Set<Tag::MIME_TYPE>("video/avc");
    videoMeta->Set<Tag::MEDIA_START_TIME>(1000000);
    std::shared_ptr<Meta> audioMeta = std::make_shared<Meta>();
    audioMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    audioMeta->Set<Tag::MIME_TYPE>("audio/mp4a-latm");
    audioMeta->Set<Tag::MEDIA_START_TIME>(2000000);
    std::vector<std::shared_ptr<Meta>> trackInfos;
    trackInfos.push_back(videoMeta);
    trackInfos.push_back(audioMeta);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    // GetTranscoderStartPts delegates to MediaDemuxer::transcoderStartPts_, which is computed
    // by TranscoderInitMediaStartPts() (called during DoPrepare via InitMediaMetaData).
    // Populate mediaMetaData_.trackMetas and invoke TranscoderInitMediaStartPts to mirror
    // the canonical computation path.
    demuxer->mediaMetaData_.trackMetas = trackInfos;
    demuxer->TranscoderInitMediaStartPts();
    demuxerFilter_->demuxer_ = demuxer;
    int64_t startPts = demuxerFilter_->GetTranscoderStartPts();
    EXPECT_EQ(startPts, 1000000);  // min(firstVideo 1000000, firstAudio 2000000)
}

/**
 * @tc.name: DemuxerFilter_GetTranscoderStartPts_0300
 * @tc.desc: GetTranscoderStartPts with only video track
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DemuxerFilter_GetTranscoderStartPts_0300, TestSize.Level1)
{
    std::shared_ptr<Meta> videoMeta = std::make_shared<Meta>();
    videoMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::VIDEO);
    videoMeta->Set<Tag::MIME_TYPE>("video/avc");
    videoMeta->Set<Tag::MEDIA_START_TIME>(500000);
    std::vector<std::shared_ptr<Meta>> trackInfos;
    trackInfos.push_back(videoMeta);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxer->mediaMetaData_.trackMetas = trackInfos;
    demuxer->TranscoderInitMediaStartPts();
    demuxerFilter_->demuxer_ = demuxer;
    int64_t startPts = demuxerFilter_->GetTranscoderStartPts();
    EXPECT_EQ(startPts, 500000);
}

/**
 * @tc.name: DemuxerFilter_GetTranscoderStartPts_0400
 * @tc.desc: GetTranscoderStartPts with only audio track
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerFilterUnitTest, DemuxerFilter_GetTranscoderStartPts_0400, TestSize.Level1)
{
    std::shared_ptr<Meta> audioMeta = std::make_shared<Meta>();
    audioMeta->Set<Tag::MEDIA_TYPE>(Plugins::MediaType::AUDIO);
    audioMeta->Set<Tag::MIME_TYPE>("audio/mp4a-latm");
    audioMeta->Set<Tag::MEDIA_START_TIME>(800000);
    std::vector<std::shared_ptr<Meta>> trackInfos;
    trackInfos.push_back(audioMeta);
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxer->mediaMetaData_.trackMetas = trackInfos;
    demuxer->TranscoderInitMediaStartPts();
    demuxerFilter_->demuxer_ = demuxer;
    int64_t startPts = demuxerFilter_->GetTranscoderStartPts();
    EXPECT_EQ(startPts, 800000);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS