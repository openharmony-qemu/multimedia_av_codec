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

    int64_t realSeekTime = 10000;
    EXPECT_EQ(demuxer->SeekTo(10000, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime), Status::OK);
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

}
