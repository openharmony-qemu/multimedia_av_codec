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

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace {
static const std::string MEDIA_ROOT = "file:///data/test/media/";
static const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
static const std::string VIDEO_FILE2 = MEDIA_ROOT + "test_265_B_Gop25_4sec.mp4";
static const std::string MIME_IMAGE = "image";
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

HWTEST_F(DemuxerFilterUnitTest, SeekMs2FrameId_0100, TestSize.Level1)
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
    ret = demuxerFilter->SeekMs2FrameId(0, frameId);
    EXPECT_EQ(frameId, 0);
    EXPECT_EQ(ret, Status::OK);
    ret = demuxerFilter->SeekMs2FrameId(480, frameId);
    EXPECT_EQ(frameId, 9);
    EXPECT_EQ(ret, Status::OK);
    ret = demuxerFilter->SeekMs2FrameId(920, frameId);
    EXPECT_EQ(frameId, 19);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(demuxerFilter->SeekMs2FrameId(-4, frameId), Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(DemuxerFilterUnitTest, FrameId2SeekMs_0100, TestSize.Level1)
{
    auto demuxerFilter = std::make_shared<DemuxerFilter>("testDemuxerFilter", FilterType::FILTERTYPE_DEMUXER);
    auto receiver = std::make_shared<FilterEventReceiverMock>();
    demuxerFilter->receiver_ = receiver;
    auto demuxer = std::make_shared<MediaDemuxerMock>();
    demuxerFilter->demuxer_ = demuxer;
    auto mediaSource = std::make_shared<MediaSource>(VIDEO_FILE2);
    demuxerFilter->SetDataSource(mediaSource);
    int64_t seekMs = 0;
    Status ret = Status::OK;
    ret = demuxerFilter->FrameId2SeekMs(0, seekMs);
    EXPECT_EQ(seekMs, 0);
    EXPECT_EQ(ret, Status::OK);
    ret = demuxerFilter->FrameId2SeekMs(10, seekMs);
    EXPECT_EQ(seekMs, 400);
    EXPECT_EQ(ret, Status::OK);
    ret = demuxerFilter->FrameId2SeekMs(20, seekMs);
    EXPECT_EQ(seekMs, 840);
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
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS