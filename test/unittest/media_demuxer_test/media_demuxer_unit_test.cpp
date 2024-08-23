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

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Stop_005, TestSize.Level1)
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

    EXPECT_EQ(demuxer->Flush(), Status::OK);
    EXPECT_EQ(demuxer->Start(), Status::OK);
    EXPECT_EQ(demuxer->Pause(), Status::OK);
    EXPECT_EQ(demuxer->Resume(), Status::OK);
    EXPECT_EQ(demuxer->Stop(), Status::OK);
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
    demuxer->isSeekError_ = false;
    EXPECT_EQ(time, demuxer->ReadLoop(trackId));
    demuxer->streamDemuxer_->SetIsIgnoreParse(false);
    demuxer->isStopped_ = false;
    demuxer->isPaused_ = false;
    demuxer->isSeekError_ = true;
    EXPECT_EQ(time, demuxer->ReadLoop(trackId));
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
    streams.push_back(info);

    Plugins::StreamInfo info1;
    info1.streamId = 1;
    info1.bitRate = 0;
    info1.type = Plugins::VIDEO;
    streams.push_back(info1);
    streams.push_back(info1);

    Plugins::StreamInfo info2;
    info2.streamId = 2;
    info2.bitRate = 0;
    info2.type = Plugins::SUBTITLE;
    streams.push_back(info2);
    streams.push_back(info2);

    EXPECT_EQ(demuxerPluginManager->InitDefaultPlay(streams), Status::OK);
    demuxerPluginManager->GetStreamCount();

    demuxerPluginManager->LoadDemuxerPlugin(-1, nullptr);
    demuxerPluginManager->curSubTitleStreamID_  = -1;
    Plugins::MediaInfo mediaInfo;
    demuxerPluginManager->LoadCurrentSubtitlePlugin(nullptr, mediaInfo);
    demuxerPluginManager->GetTmpInnerTrackIDByTrackID(-1);
    demuxerPluginManager->GetInnerTrackIDByTrackID(-1);

    int32_t trackId = -1;
    int32_t innerTrackId = -1;
    demuxerPluginManager->GetTrackInfoByStreamID(0, trackId, innerTrackId);
    EXPECT_EQ(trackId, -1);
    EXPECT_EQ(innerTrackId, -1);

    demuxerPluginManager->AddTrackMapInfo(0, 0);
    demuxerPluginManager->AddTrackMapInfo(1, 1);
    demuxerPluginManager->AddTrackMapInfo(2, 2);

    demuxerPluginManager->GetTrackInfoByStreamID(0, trackId, innerTrackId);
    EXPECT_EQ(trackId, 0);
    EXPECT_EQ(innerTrackId, 0);

    EXPECT_EQ(demuxerPluginManager->GetInnerTrackIDByTrackID(0), 0);
    EXPECT_EQ(demuxerPluginManager->CheckTrackIsActive(-1), false);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Dts2FrameId_012, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusOK");
    demuxer->audioTrackId_ = 1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[0].streamID = 0;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[1].streamID = 1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[2].streamID = 2;

    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[2].plugin = pluginMock;

    demuxer->isParserTaskEnd_ = false;
    uint32_t frameId = 0;
    std::vector<uint32_t> IFramePos = { 100 };

    EXPECT_EQ(demuxer->Dts2FrameId(100, frameId, 0), Status::OK);
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
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();

    demuxer->RegisterVideoStreamReadyCallback(nullptr);

    demuxer->isStopped_ = false;
    demuxer->OnBufferAvailable(0);
    EXPECT_EQ(demuxer->AddDemuxerCopyTask(0, TaskType::GLOBAL), Status::ERROR_UNKNOWN);

    demuxer->AccelerateTrackTask(-1);
    demuxer->SetTrackNotifyFlag(0, false);
    demuxer->AccelerateTrackTask(0);
    
    demuxer->OnDumpInfo(-1);
    demuxer->OnDumpInfo(123);
    demuxer->OptimizeDecodeSlow(false);
    demuxer->DeregisterVideoStreamReadyCallback();
    EXPECT_EQ(demuxer->HasVideo(), false);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_CheckDropAudioFrame_013, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = 100;

    demuxer->audioTrackId_ = 0;
    demuxer->shouldCheckAudioFramePts_ = false;
    demuxer->CheckDropAudioFrame(sample, 0);
    demuxer->lastAudioPts_ = 101;
    demuxer->CheckDropAudioFrame(sample, 0);
    demuxer->shouldCheckAudioFramePts_ = true;
    demuxer->CheckDropAudioFrame(sample, 0);

    demuxer->subtitleTrackId_ = 1;
    demuxer->shouldCheckSubtitleFramePts_ = false;
    demuxer->CheckDropAudioFrame(sample, 1);
    demuxer->shouldCheckSubtitleFramePts_ = true;
    demuxer->lastSubtitlePts_ = 101;
    demuxer->CheckDropAudioFrame(sample, 1);
    demuxer->lastSubtitlePts_ = 99;
    demuxer->CheckDropAudioFrame(sample, 1);

    demuxer->videoTrackId_ = 1;
    demuxer->isDecodeOptimizationEnabled_ = true;

    uint8_t* data = new uint8_t[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, 100, 100);
    demuxer->framerate_ = 1.5;
    demuxer->speed_ = 1.0;
    demuxer->decoderFramerateUpperLimit_ = 100;
    EXPECT_EQ(demuxer->IsBufferDroppable(buffer, 1), false);
    
    demuxer->framerate_ = 15000;
    demuxer->speed_ = 1.0;
    demuxer->decoderFramerateUpperLimit_ = 100;
    EXPECT_EQ(demuxer->IsBufferDroppable(buffer, 1), false);

    buffer->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, true);
    EXPECT_EQ(demuxer->IsBufferDroppable(buffer, 1), true);

    delete[] data;
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_IsBufferDroppable_001,
    TestSize.Level1)
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
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], aTrackId));
    EXPECT_EQ(true, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], vTrackId));
    demuxer->SetSpeed(0);
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], aTrackId));
    EXPECT_EQ(true, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], vTrackId));
    demuxer->bufferMap_[vTrackId]->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, false);
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[aTrackId], aTrackId));
    EXPECT_EQ(false, demuxer->IsBufferDroppable(demuxer->bufferMap_[vTrackId], vTrackId));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_HandleSourceDrmInfoEvent_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::vector<uint8_t> val{0, 0};
    std::multimap<std::string, std::vector<uint8_t>> info;
    info.insert(std::pair<std::string, std::vector<uint8_t>>("key", val));
    demuxer->localDrmInfos_ = info;
    demuxer->HandleSourceDrmInfoEvent(info);
    EXPECT_EQ(1, demuxer->localDrmInfos_.size());
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetTrackTypeByTrackID_016, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerManager = std::make_shared<DemuxerPluginManager>();
    demuxerManager->curSubTitleStreamID_ = 0;
    demuxerManager->AddExternalSubtitle();

    Meta metaTmp1;
    metaTmp1.Set<Tag::MIME_TYPE>("audio/xxx");
    demuxerManager->curMediaInfo_.tracks.push_back(metaTmp1);
    Meta metaTmp2;
    metaTmp2.Set<Tag::MIME_TYPE>("video/xxx");
    demuxerManager->curMediaInfo_.tracks.push_back(metaTmp2);
    Meta metaTmp3;
    metaTmp3.Set<Tag::MIME_TYPE>("text/vtt");
    demuxerManager->curMediaInfo_.tracks.push_back(metaTmp3);
    Meta metaTmp4;
    metaTmp4.Set<Tag::MIME_TYPE>("aaaa");
    demuxerManager->curMediaInfo_.tracks.push_back(metaTmp4);
    EXPECT_EQ(demuxerManager->GetTrackTypeByTrackID(0), TRACK_AUDIO);
    EXPECT_EQ(demuxerManager->GetTrackTypeByTrackID(1), TRACK_VIDEO);
    EXPECT_EQ(demuxerManager->GetTrackTypeByTrackID(2), TRACK_SUBTITLE);
    EXPECT_EQ(demuxerManager->GetTrackTypeByTrackID(3), TRACK_INVALID);

    EXPECT_EQ(demuxerManager->IsSubtitleMime("application/x-subrip"), true);
    EXPECT_EQ(demuxerManager->IsSubtitleMime("text/vtt"), true);
    EXPECT_EQ(demuxerManager->IsSubtitleMime("aaaaa"), false);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_UpdateDefaultStreamID_016, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerManager = std::make_shared<DemuxerPluginManager>();
    Plugins::MediaInfo mediaInfo;
    EXPECT_EQ(demuxerManager->UpdateDefaultStreamID(mediaInfo, AUDIO, 1), Status::OK);
    EXPECT_EQ(demuxerManager->UpdateDefaultStreamID(mediaInfo, SUBTITLE, 1), Status::OK);
    EXPECT_EQ(demuxerManager->UpdateDefaultStreamID(mediaInfo, VIDEO, 1), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, DemuxerPluginManager_Start_016, TestSize.Level1)
{
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusErrorUnknown");
    std::shared_ptr<DemuxerPluginManager> demuxerManager = std::make_shared<DemuxerPluginManager>();
    demuxerManager->needResetEosStatus_ = true;

    MediaStreamInfo info1;
    info1.plugin = pluginMock;
    demuxerManager->streamInfoMap_[0] = info1;
    MediaStreamInfo info2;
    info2.plugin = pluginMock;
    demuxerManager->streamInfoMap_[1] = info1;
    MediaStreamInfo info3;
    info3.plugin = pluginMock;
    demuxerManager->streamInfoMap_[2] = info1;

    demuxerManager->curVideoStreamID_ = 0;
    demuxerManager->curAudioStreamID_ = -1;
    demuxerManager->curSubTitleStreamID_ = -1;
    EXPECT_EQ(demuxerManager->Start(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Stop(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Reset(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Flush(), Status::ERROR_UNKNOWN);
    int64_t realSeekTime;
    EXPECT_EQ(demuxerManager->SeekTo(1, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime), Status::ERROR_UNKNOWN);
    demuxerManager->curVideoStreamID_ = -1;
    demuxerManager->curAudioStreamID_ = 1;
    demuxerManager->curSubTitleStreamID_ = -1;
    EXPECT_EQ(demuxerManager->Start(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Stop(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Reset(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Flush(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->SeekTo(1, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime), Status::ERROR_UNKNOWN);
    demuxerManager->curVideoStreamID_ = -1;
    demuxerManager->curAudioStreamID_ = -1;
    demuxerManager->curSubTitleStreamID_ =2;
    EXPECT_EQ(demuxerManager->Start(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Stop(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Reset(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->Flush(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(demuxerManager->SeekTo(1, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_ProcessVideoStartTime_016, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    
    demuxer->source_->seekToTimeFlag_ = true;
    demuxer->videoTrackId_  = 0;
    demuxer->demuxerPluginManager_->isDash_ = false;

    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = 100;
    EXPECT_EQ(demuxer->DoSelectTrack(0, TRACK_ID_DUMMY), Status::ERROR_UNKNOWN);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_HandleDashSelectTrack_016, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->HandleDashSelectTrack(0), Status::ERROR_UNKNOWN);

    Meta metaTmp1;
    metaTmp1.Set<Tag::MIME_TYPE>("audio/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp1);
    Meta metaTmp2;
    metaTmp2.Set<Tag::MIME_TYPE>("video/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp2);
    Meta metaTmp3;
    metaTmp3.Set<Tag::MIME_TYPE>("text/vtt");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp3);
    Meta metaTmp4;
    metaTmp4.Set<Tag::MIME_TYPE>("aaaaa");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp3);

    demuxer->demuxerPluginManager_->AddTrackMapInfo(0, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(1, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(2, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(3, 0);

    demuxer->audioTrackId_ = 0;
    demuxer->videoTrackId_ = 1;
    demuxer->subtitleTrackId_ = 2;

    EXPECT_EQ(demuxer->HandleDashSelectTrack(0), Status::OK);
    EXPECT_EQ(demuxer->HandleDashSelectTrack(1), Status::OK);
    EXPECT_EQ(demuxer->HandleDashSelectTrack(2), Status::OK);
    EXPECT_EQ(demuxer->HandleDashSelectTrack(3), Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SeekToTimePre_016, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->audioTrackId_ = 0;
    demuxer->videoTrackId_ = 0;
    demuxer->subtitleTrackId_ = 0;

    Meta metaTmp1;
    metaTmp1.Set<Tag::MIME_TYPE>("audio/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp1);
    Meta metaTmp2;
    metaTmp2.Set<Tag::MIME_TYPE>("video/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp2);
    Meta metaTmp3;
    metaTmp3.Set<Tag::MIME_TYPE>("text/vtt");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp3);

    demuxer->demuxerPluginManager_->isDash_ = false;
    EXPECT_EQ(demuxer->SeekToTimePre(), Status::OK);
    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->isSelectBitRate_ = true;
    EXPECT_EQ(demuxer->SeekToTimePre(), Status::OK);

    demuxer->isSelectBitRate_ = false;
    demuxer->isSelectTrack_ = true;
    demuxer->selectTrackTrackID_ = 0;
    EXPECT_EQ(demuxer->SeekToTimePre(), Status::OK);
    demuxer->selectTrackTrackID_ = 1;
    EXPECT_EQ(demuxer->SeekToTimePre(), Status::OK);
    demuxer->selectTrackTrackID_ = 2;
    EXPECT_EQ(demuxer->SeekToTimePre(), Status::OK);

    demuxer->isSelectBitRate_ = false;
    demuxer->isSelectTrack_ = false;
    EXPECT_EQ(demuxer->SeekToTimePre(), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SeekToTimeAfter_016, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->audioTrackId_ = 0;
    demuxer->videoTrackId_ = 0;
    demuxer->subtitleTrackId_ = 0;

    Meta metaTmp1;
    metaTmp1.Set<Tag::MIME_TYPE>("audio/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp1);
    Meta metaTmp2;
    metaTmp2.Set<Tag::MIME_TYPE>("video/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp2);
    Meta metaTmp3;
    metaTmp3.Set<Tag::MIME_TYPE>("text/vtt");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp3);

    demuxer->demuxerPluginManager_->isDash_ = false;
    EXPECT_EQ(demuxer->SeekToTimeAfter(), Status::OK);
    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->isSelectBitRate_ = true;
    EXPECT_EQ(demuxer->SeekToTimeAfter(), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_CheckDropAudioFrame_016, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = 100;
    demuxer->audioTrackId_ = 0;
    demuxer->subtitleTrackId_ = 2;
    demuxer->shouldCheckAudioFramePts_  = true;
    demuxer->lastAudioPts_  = 200;
    demuxer->CheckDropAudioFrame(sample, 0);

    demuxer->shouldCheckAudioFramePts_  = false;
    demuxer->CheckDropAudioFrame(sample, 2);
    demuxer->lastSubtitlePts_  = 200;
    demuxer->shouldCheckAudioFramePts_  = true;
    demuxer->CheckDropAudioFrame(sample, 2);

    EXPECT_EQ(demuxer->IsVideoEos(), true);
    demuxer->videoTrackId_ = 0;
    demuxer->eosMap_[0] = true;
    EXPECT_EQ(demuxer->IsVideoEos(), true);

    EXPECT_EQ(demuxer->IsRenderNextVideoFrameSupported(), true);
    EXPECT_EQ(demuxer->ResumeDragging(), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SelectTrackChangeStream_016, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->streamDemuxer_->SetNewAudioStreamID(0);
    demuxer->streamDemuxer_->SetNewVideoStreamID(1);
    demuxer->streamDemuxer_->SetNewSubtitleStreamID(2);
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusErrorUnknown");

    demuxer->audioTrackId_ = 0;
    demuxer->videoTrackId_ = 1;
    demuxer->subtitleTrackId_ = 2;

    Meta metaTmp1;
    metaTmp1.Set<Tag::MIME_TYPE>("audio/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp1);
    Meta metaTmp2;
    metaTmp2.Set<Tag::MIME_TYPE>("video/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp2);
    Meta metaTmp3;
    metaTmp3.Set<Tag::MIME_TYPE>("text/vtt");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp3);
    Meta metaTmp4;
    metaTmp4.Set<Tag::MIME_TYPE>("aaaa");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp4);

    demuxer->selectTrackTrackID_ = 0;
    EXPECT_EQ(demuxer->SelectTrackChangeStream(5), false);
    demuxer->selectTrackTrackID_ = 1;
    EXPECT_EQ(demuxer->SelectTrackChangeStream(5), false);
    demuxer->selectTrackTrackID_ = 2;
    EXPECT_EQ(demuxer->SelectTrackChangeStream(5), false);
    demuxer->selectTrackTrackID_ = 3;
    EXPECT_EQ(demuxer->SelectTrackChangeStream(5), false);

    demuxer->demuxerPluginManager_->AddTrackMapInfo(0, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(1, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(2, 0);

    MediaStreamInfo info1;
    info1.plugin = pluginMock;
    info1.streamID = 0;
    info1.type = StreamType::AUDIO;
    demuxer->demuxerPluginManager_->streamInfoMap_[0] = info1;
    MediaStreamInfo info2;
    info2.plugin = pluginMock;
    info2.streamID = 1;
    info2.type = StreamType::VIDEO;
    demuxer->demuxerPluginManager_->streamInfoMap_[1] = info2;
    MediaStreamInfo info3;
    info3.plugin = pluginMock;
    info3.streamID = 2;
    info3.type = StreamType::SUBTITLE;
    demuxer->demuxerPluginManager_->streamInfoMap_[2] = info3;

    EXPECT_EQ(demuxer->SelectTrackChangeStream(0), false);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_PrepareFrame_002, TestSize.Level1)
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
    demuxer->isStopped_ = true;
    demuxer->useBufferQueue_ = false;
    EXPECT_EQ(Status::ERROR_WRONG_STATE, demuxer->PrepareFrame(true));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_PrepareFrame_003, TestSize.Level1)
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
    demuxer->waitForDataFail_ = true;
    EXPECT_EQ(Status::OK, demuxer->PrepareFrame(true));
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SelectBitRate_016, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->source_->plugin_ = std::make_shared<SourcePluginMock>("StatusOK");
    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->streamDemuxer_->changeStreamFlag_ = false;
    EXPECT_EQ(demuxer->SelectBitRate(1), Status::OK);

    std::shared_ptr<Meta> meta1 = std::make_shared<Meta>();
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);

    demuxer->videoTrackId_ = 2;
    demuxer->useBufferQueue_ = true;
    EXPECT_EQ(demuxer->SelectBitRate(3), Status::OK);

    std::vector<uint32_t> bitRates;
    EXPECT_EQ(demuxer->GetBitRates(bitRates), Status::OK);

    demuxer->source_ = nullptr;
    int64_t durationMs;
    EXPECT_EQ(demuxer->GetDuration(durationMs), false);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_StartReferenceParser_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->videoTrackId_ = 1;
    EXPECT_EQ(demuxer->StartReferenceParser(1, false), Status::ERROR_NULL_POINTER);
    demuxer->demuxerPluginManager_ = nullptr;
    EXPECT_EQ(demuxer->StartReferenceParser(1, false), Status::ERROR_NULL_POINTER);
    demuxer->videoTrackId_ = TRACK_ID_DUMMY;
    EXPECT_EQ(demuxer->StartReferenceParser(1, false), Status::ERROR_UNKNOWN);
    demuxer->source_ = nullptr;
    EXPECT_EQ(demuxer->StartReferenceParser(1, false), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(demuxer->StartReferenceParser(-1, false), Status::ERROR_UNKNOWN);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_StartReferenceParser_002, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusErrorUnknown");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock1 = std::make_shared<DemuxerPluginMock>("StatusOK");
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[0].streamID = 0;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[1].streamID = 1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[2].streamID = 2;

    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = nullptr;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[2].plugin = pluginMock1;

    demuxer->videoTrackId_ = 0;
    EXPECT_EQ(demuxer->StartReferenceParser(1, false), Status::ERROR_NULL_POINTER);
    
    demuxer->videoTrackId_ = 1;
    EXPECT_EQ(demuxer->StartReferenceParser(1, false), Status::ERROR_UNKNOWN);

    demuxer->videoTrackId_ = 2;
    demuxer->isFirstParser_ = true;
    EXPECT_EQ(demuxer->StartReferenceParser(1, false), Status::ERROR_INVALID_OPERATION);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetFrameLayerInfo_002, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();

    FrameLayerInfo frameLayerInfo;
    EXPECT_EQ(demuxer->GetFrameLayerInfo(nullptr, frameLayerInfo), Status::ERROR_NULL_POINTER);
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    demuxer->demuxerPluginManager_  = nullptr;
    EXPECT_EQ(demuxer->GetFrameLayerInfo(sample, frameLayerInfo), Status::ERROR_NULL_POINTER);
    demuxer->source_ = nullptr;
    EXPECT_EQ(demuxer->GetFrameLayerInfo(sample, frameLayerInfo), Status::ERROR_NULL_POINTER);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_SetCacheLimit_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock1 = std::make_shared<DemuxerPluginMock>("StatusOK");

    demuxer->demuxerPluginManager_->curVideoStreamID_ = 0;
    demuxer->demuxerPluginManager_->curAudioStreamID_ = 1;
    demuxer->demuxerPluginManager_->curSubTitleStreamID_ = 2;
    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = pluginMock1;
    demuxer->demuxerPluginManager_->streamInfoMap_[0].type = StreamType::VIDEO;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].plugin = pluginMock1;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].type = StreamType::AUDIO;
    demuxer->demuxerPluginManager_->streamInfoMap_[2].plugin = pluginMock1;
    demuxer->demuxerPluginManager_->streamInfoMap_[2].type = StreamType::SUBTITLE;

    demuxer->SetCacheLimit(10);

    demuxer->demuxerPluginManager_->AddTrackMapInfo(0, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(1, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(2, 0);
    EXPECT_EQ(demuxer->demuxerPluginManager_->GetStreamTypeByTrackID(0), StreamType::VIDEO);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_ResumeDemuxerReadLoop_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->isDemuxerLoopExecuting_ = true;
    EXPECT_EQ(demuxer->ResumeDemuxerReadLoop(), Status::OK);
    EXPECT_EQ(demuxer->PauseDemuxerReadLoop(), Status::OK);
    EXPECT_EQ(demuxer->ResumeDemuxerReadLoop(), Status::OK);
    demuxer->isDemuxerLoopExecuting_ = false;
    EXPECT_EQ(demuxer->PauseDemuxerReadLoop(), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_PauseTaskByTrackId_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();

    demuxer->GetStreamMetaInfo();
    demuxer->GetGlobalMetaInfo();
    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->GetUserMeta();

    PlaybackInfo playbackInfo;
    demuxer->GetPlaybackInfo(playbackInfo);
    demuxer->source_ = nullptr;
    demuxer->GetPlaybackInfo(playbackInfo);

    EXPECT_EQ(demuxer->AddDemuxerCopyTask(0, TaskType::VIDEO), Status::OK);
    EXPECT_EQ(demuxer->PauseTaskByTrackId(0), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_ReadSample_001, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();

    std::shared_ptr<AVBuffer> sample;
    demuxer->useBufferQueue_ = true;
    EXPECT_EQ(demuxer->ReadSample(0, sample), Status::ERROR_WRONG_STATE);
    demuxer->useBufferQueue_ = false;
    EXPECT_EQ(demuxer->ReadSample(0, sample), Status::ERROR_INVALID_OPERATION);
    demuxer->eosMap_[0] = true;
    EXPECT_EQ(demuxer->ReadSample(0, sample), Status::ERROR_INVALID_PARAMETER);
    uint8_t* data = new uint8_t[100];
    sample = AVBuffer::CreateAVBuffer(data, 100, 100);
    sample->pts_ = 100;
    EXPECT_EQ(demuxer->ReadSample(0, sample), Status::END_OF_STREAM);
    delete[] data;
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_StartTask_002, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->streamDemuxer_->isIgnoreParse_ = true;
    demuxer->isSeekError_ = true;
    demuxer->source_ = nullptr;

    Meta metaTmp1;
    metaTmp1.Set<Tag::MIME_TYPE>("audio/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp1);
    Meta metaTmp2;
    metaTmp2.Set<Tag::MIME_TYPE>("video/xxx");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp2);
    Meta metaTmp3;
    metaTmp3.Set<Tag::MIME_TYPE>("text/vtt");
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp3);
    EXPECT_EQ(demuxer->StartTask(0), Status::OK);
    EXPECT_EQ(demuxer->StartTask(1), Status::OK);
    EXPECT_EQ(demuxer->StartTask(2), Status::OK);
    EXPECT_EQ(demuxer->StopAllTask(), Status::OK);
}

HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_CheckChangeStreamID_002, TestSize.Level1)
{
    std::shared_ptr<StreamDemuxer> streamDemuxer = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    streamDemuxer->isDash_ = true;
    streamDemuxer->newVideoStreamID_ = 0;
    streamDemuxer->newAudioStreamID_ = 1;
    streamDemuxer->newSubtitleStreamID_ = 2;

    buffer->streamID = 3;
    EXPECT_EQ(streamDemuxer->CheckChangeStreamID(0, buffer), Status::END_OF_STREAM);
    streamDemuxer->newVideoStreamID_ = 0;
    EXPECT_EQ(streamDemuxer->CheckChangeStreamID(1, buffer), Status::END_OF_STREAM);
    streamDemuxer->newAudioStreamID_ = 1;
    EXPECT_EQ(streamDemuxer->CheckChangeStreamID(2, buffer), Status::END_OF_STREAM);
    streamDemuxer->newSubtitleStreamID_ = 2;
    EXPECT_EQ(streamDemuxer->CheckChangeStreamID(4, buffer), Status::END_OF_STREAM);

    std::shared_ptr<Buffer> bufferPtr = nullptr;
    streamDemuxer->isInterruptNeeded_ = false;
    EXPECT_EQ(streamDemuxer->GetPeekRange(0, 0, 100, bufferPtr), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(streamDemuxer->Start(), Status::OK);

    streamDemuxer->pluginStateMap_[0] = DemuxerState::DEMUXER_STATE_NULL;
    EXPECT_EQ(streamDemuxer->CallbackReadAt(0, 0, bufferPtr, 100), Status::ERROR_WRONG_STATE);

    CacheData cacheTmp;
    streamDemuxer->cacheDataMap_[0] = cacheTmp;
    EXPECT_EQ(streamDemuxer->ResetCache(0), Status::OK);
}

/**
 * @tc.name: MediaDemuxer_GetProtocolByUri_0100
 * @tc.desc: MediaDemuxer_GetProtocolByUri_0100
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetProtocolByUri_0100, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    EXPECT_EQ(demuxer->PauseForPrepareFrame(), Status::OK);
    demuxer->source_ = std::shared_ptr<Source>();
    EXPECT_EQ(demuxer->PauseForPrepareFrame(), Status::OK);
    demuxer->taskMap_ = std::map<uint32_t, std::unique_ptr<Task>>();
    EXPECT_EQ(demuxer->PauseForPrepareFrame(), Status::OK);
}
/**
 * @tc.name: MediaDemuxer_Resume_0100
 * @tc.desc: MediaDemuxer_Resume_0100
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_Resume_0100, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    EXPECT_EQ(demuxer->Resume(), Status::OK);
    demuxer->source_ = std::shared_ptr<Source>();
    EXPECT_EQ(demuxer->Resume(), Status::OK);
    demuxer->doPrepareFrame_ = false;
    EXPECT_EQ(demuxer->Resume(), Status::OK);
    demuxer->doPrepareFrame_ = true;
    EXPECT_EQ(demuxer->Resume(), Status::OK);
}
/**
 * @tc.name: MediaDemuxer_ResumeDragging_0100
 * @tc.desc: MediaDemuxer_ResumeDragging_0100
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_ResumeDragging_0100, TestSize.Level1)
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->source_ = std::shared_ptr<Source>();
    demuxer->taskMap_ = std::map<uint32_t, std::unique_ptr<Task>>();
    EXPECT_EQ(demuxer->ResumeDragging(), Status::OK);
}

/**
 * @tc.name: MediaDemuxer_InitAudioTrack_0100
 * @tc.desc: test InitAudioTrack
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_InitAudioTrack_0100, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    StreamInfo info;
    demuxerPluginManager->curAudioStreamID_ = 1;
    demuxerPluginManager->InitAudioTrack(info);
    demuxerPluginManager->curAudioStreamID_ = -1;
    demuxerPluginManager->InitAudioTrack(info);
    EXPECT_EQ(demuxerPluginManager->isDash_, true);
}
/**
 * @tc.name: MediaDemuxer_InitVideoTrack_0100
 * @tc.desc: test InitVideoTrack
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_InitVideoTrack_0100, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    StreamInfo info;
    demuxerPluginManager->curAudioStreamID_ = 1;
    demuxerPluginManager->InitVideoTrack(info);
    demuxerPluginManager->curAudioStreamID_ = -1;
    demuxerPluginManager->InitVideoTrack(info);
    EXPECT_EQ(demuxerPluginManager->isDash_, true);
}
/**
 * @tc.name: MediaDemuxer_InitSubtitleTrack_0100
 * @tc.desc: test InitSubtitleTrack
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_InitSubtitleTrack_0100, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    StreamInfo info;
    demuxerPluginManager->curAudioStreamID_ = -1;
    demuxerPluginManager->InitSubtitleTrack(info);
    demuxerPluginManager->curAudioStreamID_ = 0;
    demuxerPluginManager->InitSubtitleTrack(info);
    EXPECT_EQ(demuxerPluginManager->curAudioStreamID_, info.streamId);
}
/**
 * @tc.name: MediaDemuxer_LoadCurrentSubtitlePlugin_0100
 * @tc.desc: test LoadCurrentSubtitlePlugin
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_LoadCurrentSubtitlePlugin_0100, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    demuxerPluginManager->curSubTitleStreamID_ = -1;
    Plugins::MediaInfo mediaInfo;
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer = std::shared_ptr<BaseStreamDemuxer>();
    Status ret =  demuxerPluginManager->LoadCurrentSubtitlePlugin(streamDemuxer, mediaInfo);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
/**
 * @tc.name: MediaDemuxer_UpdateGeneralValue_0100
 * @tc.desc: test UpdateGeneralValue
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_UpdateGeneralValue_0100, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    int32_t trackCount = 0;
    Meta format;
    Meta formatNew;
    format.Set<Tag::MEDIA_HAS_VIDEO>(true);
    format.Set<Tag::MEDIA_HAS_AUDIO>(true);
    format.Set<Tag::MEDIA_HAS_SUBTITLE>(true);
    Status ret = demuxerPluginManager->UpdateGeneralValue(trackCount, format, formatNew);
    EXPECT_EQ(ret, Status::OK);
}
/**
 * @tc.name: MediaDemuxer_GetInnerTrackIDByTrackID_0100
 * @tc.desc: test GetInnerTrackIDByTrackID
 * @tc.type: FUNC
 */
HWTEST_F(MediaDemuxerUnitTest, MediaDemuxer_GetInnerTrackIDByTrackID_0100, TestSize.Level1)
{
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    int32_t trackId = 1;
    int32_t expectedInnerTrackIndex = 2;
    demuxerPluginManager->trackInfoMap_[trackId].innerTrackIndex = expectedInnerTrackIndex;
    int32_t actualInnerTrackIndex = demuxerPluginManager->GetInnerTrackIDByTrackID(trackId);
    ASSERT_EQ(actualInnerTrackIndex, expectedInnerTrackIndex);
}
}