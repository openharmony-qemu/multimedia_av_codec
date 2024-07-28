/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include "media_demuxer_unit_test.h"
#include "http_server_demo.h"
#include "plugin/plugin_event.h"
#include "demuxer/stream_demuxer.h"

#define LOCAL true
namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
std::unique_ptr<MediaAVCodec::HttpServerDemo> g_server = nullptr;
void MediaDemuxerUnitTest::SetUpTestCase(void)
{
    g_server = std::make_unique<MediaAVCodec::HttpServerDemo>();
    g_server->StartServer();
    std::cout << "start" << std::endl;
}

void MediaDemuxerUnitTest::TearDownTestCase(void)
{
}

void MediaDemuxerUnitTest::SetUp()
{
}

void MediaDemuxerUnitTest::TearDown()
{
}

class MediaDemuxerTestCallback : public OHOS::MediaAVCodec::AVDemuxerCallback {
public:
    explicit MediaDemuxerTestCallback()
    {
    }

    ~MediaDemuxerTestCallback()
    {
    }

    void OnDrmInfoChanged(const std::multimap<std::string, std::vector<uint8_t>> &drmInfo) override
    {
    }
};

class TestEventReceiver : public Pipeline::EventReceiver {
public:
    explicit TestEventReceiver()
    {
        std::cout << "event receiver constructor" << std::endl;
    }

    void OnEvent(const Event &event)
    {
        std::cout << event.srcFilter << std::endl;
    }
};

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SetSubtitleSource_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/subtitle.srt";
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
    EXPECT_EQ(demuxer->SetSubtitleSource(std::make_shared<MediaSource>(uri)), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SetDataSource_002, TestSize.Level1)
{
    string srtPath = "/data/test/media/h264_fmp4.mp4";
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
    demuxer->SetInterruptState(false);
    demuxer->SetBundleName("test");

    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    std::map<uint32_t, sptr<AVBufferQueueProducer>> curBufferQueueProducerMap = demuxer->GetBufferQueueProducerMap();
    EXPECT_EQ(curBufferQueueProducerMap.size(), 1);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SelectTrack_003, TestSize.Level1)
{
    string srtPath = "/data/test/media/H264_AAC_multi_track.mp4";
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
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    EXPECT_EQ(demuxer->SelectTrack(2), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Seek_004, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Stop_005, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);

    EXPECT_EQ(demuxer->Flush(), Status::OK);
    EXPECT_EQ(demuxer->Start(), Status::OK);
    EXPECT_EQ(demuxer->Pause(), Status::OK);
    EXPECT_EQ(demuxer->Resume(), Status::OK);
    EXPECT_EQ(demuxer->Stop(), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_OnBufferAvailable_006, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);

    demuxer->OnBufferAvailable(0);

    DownloadInfo downloadInfo;
    EXPECT_EQ(demuxer->GetDownloadInfo(downloadInfo), Status::OK);

    demuxer->SetPlayerId("1");

    demuxer->SetDumpInfo(false, 111);
    demuxer->SetDumpInfo(true, 111);
    std::shared_ptr<Meta> userMeta = demuxer->GetUserMeta();

    int64_t durationMs;
    EXPECT_EQ(demuxer->GetDuration(durationMs), true);
    EXPECT_EQ(demuxer->PauseTaskByTrackId(0), Status::OK);
    EXPECT_EQ(demuxer->Reset(), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SelectBitRate_007, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);

    std::vector<uint32_t> bitRates;
    EXPECT_EQ(demuxer->GetBitRates(bitRates), Status::OK);
    EXPECT_GT(bitRates.size(), 0);

    EXPECT_EQ(demuxer->SelectBitRate(bitRates[0]), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetDuration_008, TestSize.Level1)
{
    string srtPath = "/data/test/media/h264_fmp4.mp4";
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
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);

    int64_t durationMs;
    EXPECT_EQ(demuxer->GetDuration(durationMs), true);
    EXPECT_EQ(demuxer->Start(), Status::OK);
    demuxer->OnBufferAvailable(0);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SetDecoderFramerateUpperLimit_009, TestSize.Level1)
{
    string srtPath = "/data/test/media/h264_fmp4.mp4";
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
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);

    EXPECT_EQ(demuxer->SetDecoderFramerateUpperLimit(111, 0), Status::OK);
    EXPECT_EQ(demuxer->SetSpeed(111.5), Status::OK);
    EXPECT_EQ(demuxer->SetFrameRate(1, 25), Status::OK);
    EXPECT_EQ(demuxer->DisableMediaTrack(Plugins::MediaType::VIDEO), Status::OK);
    EXPECT_EQ(demuxer->IsRenderNextVideoFrameSupported(), false);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_OnBufferAvailable_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/h264_fmp4.mp4";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    int32_t invalidTrackId = 100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    demuxer->SetTrackNotifyFlag(invalidTrackId, true);
    demuxer->SetTrackNotifyFlag(trackId, true);
    demuxer->OnBufferAvailable(trackId);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SetDrmCallback_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    std::shared_ptr<MediaDemuxerTestCallback> callback = std::make_shared<MediaDemuxerTestCallback>();
    demuxer->SetDrmCallback(callback);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetDuration_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
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
    int64_t duration = 0;
    demuxer->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    EXPECT_EQ(false, demuxer->GetDuration(duration));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_ProcessVideoStartTime_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    EXPECT_EQ(Status::ERROR_INVALID_PARAMETER, demuxer->HandleRead(trackId));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_AddDemuxerCopyTask_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    EXPECT_EQ(Status::ERROR_UNKNOWN, demuxer->AddDemuxerCopyTask(trackId, TaskType::GLOBAL));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SetInterruptState_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
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
    std::string bundleName = "test";
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->SetBundleName(bundleName);
    demuxer->SetInterruptState(true);
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    demuxer->streamDemuxer_ = nullptr;
    demuxer->SetInterruptState(true);
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    demuxer->SetInterruptState(true);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SetOutputBufferQueue_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    int32_t invalidTrackId = 100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(Status::OK, demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer));
    demuxer->bufferQueueMap_.erase(trackId);
    EXPECT_EQ(Status::OK, demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer));
    EXPECT_EQ(Status::ERROR_UNKNOWN, demuxer->AddDemuxerCopyTask(invalidTrackId, TaskType::GLOBAL));
    demuxer->bufferQueueMap_.insert(
        std::pair<uint32_t, sptr<AVBufferQueueProducer>>(invalidTrackId, inputBufferQueueProducer));
    EXPECT_EQ(Status::OK, demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_OnDumpInfo_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    demuxer->OnDumpInfo(-1);
    demuxer->OnDumpInfo(fd);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_UnselectTrack_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/webvtt_test.vtt";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    demuxer->useBufferQueue_ = true;
    demuxer->SelectTrack(trackId);
    demuxer->UnselectTrack(trackId);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SeekTo_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/test_dash/segment_base/media-video-2.mp4";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    int64_t realSeekTime = 0;
    EXPECT_EQ(Status::OK, demuxer->SeekTo(0, SeekMode::SEEK_CLOSEST_INNER, realSeekTime));
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    EXPECT_EQ(Status::OK, demuxer->SeekTo(0, SeekMode::SEEK_CLOSEST_INNER, realSeekTime));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SelectBitRate_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/test_dash/segment_base/media-video-2.mp4";
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
    demuxer->isSelectBitRate_.store(true);
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_EQ(Status::ERROR_INVALID_OPERATION, demuxer->SelectBitRate(0));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SelectBitRate_002, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
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
    demuxer->isSelectBitRate_.store(true);
    EXPECT_EQ(Status::OK, demuxer->SelectBitRate(0));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Flush_002, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    int32_t invalidTrackId = 100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(Status::OK, demuxer->Flush());
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    demuxer->bufferQueueMap_.insert(
        std::pair<uint32_t, sptr<AVBufferQueueProducer>>(invalidTrackId, inputBufferQueueProducer));
    EXPECT_EQ(Status::OK, demuxer->Flush());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_StopAllTask_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t invalidTrackId = 100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->taskMap_[invalidTrackId] = nullptr;
    EXPECT_EQ(Status::OK, demuxer->PauseAllTask());
    demuxer->source_ = nullptr;
    EXPECT_EQ(Status::OK, demuxer->StopAllTask());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_PauseForPrepareFrame_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(Status::OK, demuxer->PauseForPrepareFrame());
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    EXPECT_EQ(Status::OK, demuxer->PauseForPrepareFrame());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Resume_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_EQ(Status::OK, demuxer->Resume());
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    std::shared_ptr<Pipeline::EventReceiver> receiver = std::make_shared<TestEventReceiver>();
    demuxer->SetEventReceiver(receiver);
    EXPECT_EQ(Status::OK, demuxer->PrepareFrame(true));
    demuxer->taskMap_[trackId] = nullptr;
    demuxer->doPrepareFrame_ = true;
    EXPECT_EQ(Status::OK, demuxer->Resume());
    std::unique_ptr<Task> task = std::make_unique<Task>("test", demuxer->playerId_, TaskType::VIDEO);
    demuxer->taskMap_[trackId] = std::move(task);
    EXPECT_EQ(Status::OK, demuxer->Resume());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Resume_002, TestSize.Level1)
{
    string srtPath = "/data/test/media/H264_AAC_multi_track.mp4";
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
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    demuxer->streamDemuxer_ = nullptr;
    demuxer->source_ = nullptr;
    demuxer->doPrepareFrame_ = true;
    demuxer->taskMap_[0] = nullptr;
    EXPECT_EQ(Status::OK, demuxer->Resume());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Start_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    int32_t invalidTrackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    demuxer->taskMap_[invalidTrackId] = nullptr;
    EXPECT_EQ(Status::OK, demuxer->Start());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Start_002, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    std::shared_ptr<Pipeline::EventReceiver> receiver = std::make_shared<TestEventReceiver>();
    demuxer->SetEventReceiver(receiver);
    EXPECT_EQ(Status::OK, demuxer->PrepareFrame(true));
    EXPECT_EQ(Status::OK, demuxer->Start());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_DumpBufferToFile_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t aTrackId = 0;
    int32_t vTrackId = 1;
    int32_t invalidTrackId = 1;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(aTrackId, inputBufferQueueProducer), Status::OK);
    demuxer->isDump_ = true;
    demuxer->DumpBufferToFile(aTrackId, demuxer->bufferMap_[aTrackId]);
    demuxer->DumpBufferToFile(invalidTrackId, demuxer->bufferMap_[aTrackId]);
    demuxer->DumpBufferToFile(vTrackId, demuxer->bufferMap_[vTrackId]);
    demuxer->DumpBufferToFile(invalidTrackId, demuxer->bufferMap_[vTrackId]);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_ReadLoop_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/test_dash/segment_base/media-video-2.mp4";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    
    int32_t time = 6000;
    demuxer->streamDemuxer_->SetIsIgnoreParse(true);
    demuxer->isStopped_ = false;
    demuxer->isPaused_ = false;
    demuxer->isSeekError_ = false;
    EXPECT_EQ(time, demuxer->ReadLoop(trackId));

    demuxer->streamDemuxer_->SetIsIgnoreParse(false);
    demuxer->isStopped_ = true;
    demuxer->isPaused_ = false;
    demuxer->isSeekError_ = false;
    EXPECT_EQ(time, demuxer->ReadLoop(trackId));

    demuxer->streamDemuxer_->SetIsIgnoreParse(false);
    demuxer->isStopped_ = false;
    demuxer->isPaused_ = true;

    demuxer->streamDemuxer_->SetIsIgnoreParse(false);
    demuxer->isStopped_ = false;
    demuxer->isPaused_ = false;
    demuxer->isSeekError_ = true;
    EXPECT_EQ(time, demuxer->ReadLoop(trackId));
    uint32_t RETRY_DELAY_TIME_US = 100000;
    demuxer->streamDemuxer_->SetIsIgnoreParse(false);
    demuxer->isStopped_ = false;
    demuxer->isPaused_ = false;
    demuxer->isSeekError_ = false;
    EXPECT_EQ(RETRY_DELAY_TIME_US, demuxer->ReadLoop(trackId));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_IsContainIdrFrame_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/h264_fmp4.mp4";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    EXPECT_EQ(false, demuxer->IsContainIdrFrame(nullptr, 0));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_OnEvent_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/test_dash/segment_base/media-video-2.mp4";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer), Status::OK);
    std::shared_ptr<Pipeline::EventReceiver> receiver = std::make_shared<TestEventReceiver>();
    demuxer->SetEventReceiver(receiver);
    demuxer->OnEvent({Plugins::PluginEventType::CLIENT_ERROR, "", "CLIENT_ERROR"});
    demuxer->OnEvent({Plugins::PluginEventType::BUFFERING_END, "", "BUFFERING_END"});
    demuxer->OnEvent({Plugins::PluginEventType::BUFFERING_START, "", "BUFFERING_START"});
    demuxer->OnEvent({Plugins::PluginEventType::CACHED_DURATION, "", "CACHED_DURATION"});
    demuxer->OnEvent({Plugins::PluginEventType::SOURCE_BITRATE_START, "", "SOURCE_BITRATE_START"});
    demuxer->OnEvent({Plugins::PluginEventType::EVENT_BUFFER_PROGRESS, "", "EVENT_BUFFER_PROGRESS"});
    demuxer->OnEvent({Plugins::PluginEventType::EVENT_CHANNEL_CLOSED, "", "EVENT_CHANNEL_CLOSED"});
}

HWTEST_F(MediaDemuxerUnitTest, DemuxerPluginManager_InitDefaultPlay_011, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    std::vector<StreamInfo> streams;
    Plugins::StreamInfo info;
    info.streamId = 0;
    info.bitRate = 0;
    info.type = Plugins::AUDIO;
    streams.push_back(info);

    Plugins::StreamInfo info1;
    info.streamId = 1;
    info.bitRate = 0;
    info.type = Plugins::AUDIO;
    streams.push_back(info1);

    Plugins::StreamInfo info2;
    info.streamId = 2;
    info.bitRate = 0;
    info.type = Plugins::SUBTITLE;
    streams.push_back(info2);

    EXPECT_EQ(demuxerPluginManager->InitDefaultPlay(streams), Status::OK);
    demuxerPluginManager->GetStreamCount();

    demuxerPluginManager->LoadDemuxerPlugin(-1, nullptr);
    demuxerPluginManager->curSubTitleStreamID_  = -1;
    Plugins::MediaInfo mediaInfo;
    demuxerPluginManager->LoadCurrentSubtitlePlugin(nullptr, mediaInfo);
    demuxerPluginManager->GetTmpInnerTrackIDByTrackID(-1);
    demuxerPluginManager->GetInnerTrackIDByTrackID(-1);

    int32_t trackId;
    int32_t innerTrackId;
    demuxerPluginManager->GetTrackInfoByStreamID(0, trackId, innerTrackId);
    EXPECT_EQ(trackId, 0);
    EXPECT_EQ(innerTrackId, 0);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Dts2FrameId_012, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    uint32_t frameId = 0;
    std::vector<uint32_t> IFramePos = { 100 };

    EXPECT_EQ(demuxer->Dts2FrameId(100, frameId, 0), Status::ERROR_NULL_POINTER);
    demuxer->GetIFramePos(IFramePos);

    demuxer->source_  = nullptr;
    EXPECT_EQ(demuxer->Dts2FrameId(100, frameId, 0), Status::ERROR_NULL_POINTER);
    demuxer->GetIFramePos(IFramePos);

    demuxer->demuxerPluginManager_  = nullptr;
    EXPECT_EQ(demuxer->Dts2FrameId(100, frameId, 0), Status::ERROR_NULL_POINTER);
    demuxer->GetIFramePos(IFramePos);

    EXPECT_EQ(demuxer->SetFrameRate(-1.0, 0), Status::OK);
    EXPECT_EQ(demuxer->SetFrameRate(1.0, 0), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_RegisterVideoStreamReadyCallback_010, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);

    demuxer->RegisterVideoStreamReadyCallback(nullptr);

    demuxer->OnBufferAvailable(0);
    demuxer->AccelerateTrackTask(0);
    demuxer->SetTrackNotifyFlag(0, false);
    EXPECT_EQ(demuxer->AddDemuxerCopyTask(0, TaskType::GLOBAL), Status::ERROR_UNKNOWN);
    demuxer->OnDumpInfo(-1);
    demuxer->OnDumpInfo(123);
    demuxer->OptimizeDecodeSlow(false);
    demuxer->DeregisterVideoStreamReadyCallback();
    EXPECT_EQ(demuxer->HasVideo(), true);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_IsBufferDroppable_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
    int64_t fileSize = 0;
    if (!srtPath.empty()) {
        struct stat fileStatus {};
        if (stat(srtPath.c_str(), &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    int32_t fd = open(srtPath.c_str(), O_RDONLY);
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    int32_t aTrackId = 0;
    int32_t vTrackId = 1;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(aTrackId, inputBufferQueueProducer), Status::OK);

    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->SetDumpInfo(true, 0);
    demuxer->isDecodeOptimizationEnabled_ = true;
    std::shared_ptr<AVBuffer> aBuffer = AVBuffer::CreateAVBuffer();
    std::shared_ptr<AVBuffer> vBuffer = AVBuffer::CreateAVBuffer();
    demuxer->bufferMap_[aTrackId] = aBuffer;
    demuxer->bufferMap_[vTrackId] = vBuffer;
    vBuffer->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, true);
    ASSERT_NE(nullptr, demuxer->bufferMap_[aTrackId]);
    ASSERT_NE(nullptr, demuxer->bufferMap_[vTrackId]);
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], aTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], aTrackId));
    int32_t speed = 200;
    demuxer->SetSpeed(speed);
    demuxer->videoTrackId_ = vTrackId;
    demuxer->framerate_ = 1.0;
    demuxer->bufferMap_[vTrackId]->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, true);
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], aTrackId));
    EXPECT_EQ(true, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], aTrackId));
    demuxer->SetSpeed(0);
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], aTrackId));
    EXPECT_EQ(true, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], aTrackId));
    demuxer->bufferMap_[vTrackId]->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, false);
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], aTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], vTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], aTrackId));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_DemuxerReadLoop_001, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);

    EXPECT_EQ(Status::OK, demuxer->Start());

    EXPECT_EQ(Status::OK, demuxer->PauseDemuxerReadLoop());
    EXPECT_EQ(Status::OK, demuxer->PauseDemuxerReadLoop());
    EXPECT_EQ(Status::OK, demuxer->ResumeDemuxerReadLoop());
    EXPECT_EQ(Status::OK, demuxer->ResumeDemuxerReadLoop());
    EXPECT_EQ(Status::OK, demuxer->StopAllTask());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetPresentation_001, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    int64_t presentationTimeUs = 0;
    EXPECT_EQ(Status::OK, demuxer->GetPresentationTimeUsByFrameIndex(0, 0, presentationTimeUs));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetFrameIndex_001, TestSize.Level1)
{
    string srtPath = "/data/test/media/drm/sm4c.ts";
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
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    sleep(1);
    uint32_t frameIndex = 0;
    EXPECT_EQ(Status::ERROR_MISMATCHED_TYPE, demuxer->GetFrameIndexByPresentationTimeUs(0, 0, frameIndex));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_CheckDropAudioFrame_001, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    sleep(1);
    int32_t aTrackId = 0;
    int32_t vTrackId = 1;
    std::shared_ptr<AVBuffer> aBuffer = AVBuffer::CreateAVBuffer();
    std::shared_ptr<AVBuffer> vBuffer = AVBuffer::CreateAVBuffer();
    demuxer->bufferMap_[aTrackId] = aBuffer;
    demuxer->bufferMap_[vTrackId] = vBuffer;
    vBuffer->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, true);
    demuxer->CheckDropAudioFrame(demuxer->bufferMap_[vTrackId], vTrackId);
    EXPECT_EQ(false, demuxer->shouldCheckAudioFramePts_);
    demuxer->shouldCheckAudioFramePts_ = false;
    demuxer->CheckDropAudioFrame(demuxer->bufferMap_[aTrackId], aTrackId);
    EXPECT_EQ(false, demuxer->shouldCheckAudioFramePts_);

    demuxer->shouldCheckAudioFramePts_ = true;
    demuxer->lastAudioPts_ = -1;
    demuxer->CheckDropAudioFrame(demuxer->bufferMap_[aTrackId], aTrackId);
    EXPECT_EQ(false, demuxer->shouldCheckAudioFramePts_);

    demuxer->lastAudioPts_ = demuxer->bufferMap_[aTrackId]->pts_ + 1;
    demuxer->CheckDropAudioFrame(demuxer->bufferMap_[aTrackId], aTrackId);
    EXPECT_EQ(false, demuxer->shouldCheckAudioFramePts_);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_HandleSourceDrmInfoEvent_001, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    sleep(1);
    std::vector<uint8_t> val{0, 0};
    std::multimap<std::string, std::vector<uint8_t>> info;
    info.insert(std::pair<std::string, std::vector<uint8_t>>("key", val));
    demuxer->localDrmInfos_ = info;
    demuxer->HandleSourceDrmInfoEvent(info);
    EXPECT_EQ(1, demuxer->localDrmInfos_.size());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_CopyFrameToUserQueue_001, TestSize.Level1)
{
    string srtPath = "http://127.0.0.1:46666/test_dash/segment_base/index.mpd";
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->SetDataSource(std::make_shared<MediaSource>(srtPath)), Status::OK);
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    EXPECT_EQ(demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer), Status::OK);
    int32_t vTrackId = 1;
    int32_t aTrackId = 0;
    demuxer->isSelectBitRate_ = true;
    EXPECT_EQ(Status::ERROR_UNKNOWN, demuxer->CopyFrameToUserQueue(vTrackId));
    EXPECT_EQ(Status::ERROR_NO_CONSUMER_LISTENER, demuxer->CopyFrameToUserQueue(aTrackId));

    demuxer->isSelectBitRate_ = false;
    EXPECT_EQ(Status::ERROR_UNKNOWN, demuxer->CopyFrameToUserQueue(vTrackId));
    EXPECT_EQ(Status::ERROR_NO_CONSUMER_LISTENER, demuxer->CopyFrameToUserQueue(aTrackId));
}

}