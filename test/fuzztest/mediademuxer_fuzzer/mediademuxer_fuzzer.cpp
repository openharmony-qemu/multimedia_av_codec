/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
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
#include <sys/stat.h>
#include "securec.h"
#include <string>
#include <malloc.h>
#include <sys/stat.h>
#include <cinttypes>
#include <thread>
#include <chrono>
#include "plugin/plugin_event.h"
#include "demuxer/stream_demuxer.h"
#include <iostream>
#include "mediademuxer_mock_fuzz.h"
 
#include "media_demuxer.h"
 
#define FUZZ_PROJECT_NAME "mediademuxer_fuzzer"
using namespace std;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
const int NUMBER_1 = 1;
const int NUMBER_2 = 2;
const int NUMBER_3 = 3;
const int NUMBER_4 = 4;
const int NUMBER_5 = 5;
const int NUMBER_8 = 8;
const int NUMBER_10 = 10;
const int NUMBER_25 = 25;
const int NUMBER_100 = 100;
const int NUMBER_111 = 111;
const int NUMBER_200 = 200;
const float NUMBER_SPEED = 111.5;
const double NUMBER_FRAMERATE_1 = 1.5;
const double NUMBER_FRAMERATE_2 = 15000;
const char *DATA_PATH = "/data/test/fuzz_create.mp4";
void DoFuzzTest();
void FuzzTest1();
void FuzzTest6();
void FuzzTest7();
void FuzzTest8();
void FuzzTest9();
void FuzzTest10();
void FuzzTest11();
void FuzzTest12();
void FuzzTest13();
void FuzzTest14();
void FuzzTest15();
void FuzzTest16();
void FuzzTest17();
void FuzzTest18();
void FuzzTest19();
void FuzzTest20();
void FuzzTest21();
void FuzzTest22();
void FuzzTest23();
void FuzzTest24();
void FuzzTest25();
void FuzzTest27();
void FuzzTest28();
void FuzzTest29();
void FuzzTest30();
void FuzzTest31();
void FuzzTest32();
void FuzzTest33();
void FuzzTest34();
void FuzzTest35();
void FuzzTest37();
void FuzzTest38();
void FuzzTest39();
void FuzzTest40();
void FuzzTest41();
void FuzzTest45();
void FuzzTest46();
void FuzzTest47();
void FuzzTest48();
void FuzzTest49();
void FuzzTest50();
void FuzzTest51();
void FuzzTest52();
void FuzzTest53();
void FuzzTest54();
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
    }

    void OnEvent(const Event &event)
    {
    }
};

class VideoStreamReadyTestCallback : public VideoStreamReadyCallback {
public:
    bool IsVideoStreamDiscardable(const std::shared_ptr<AVBuffer> buffer)
    {
        (void)buffer;
        return true;
    }

protected:
};

bool RunMediaDemuxerFuzz(uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    int32_t fd = open(DATA_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size);
    if (len <= 0) {
        close(fd);
        return false;
    }
    close(fd);

    FuzzTest1();
    FuzzTest6();
    FuzzTest7();
    FuzzTest8();
    FuzzTest9();
    FuzzTest10();
    FuzzTest11();
    FuzzTest12();
    FuzzTest13();
    FuzzTest14();
    FuzzTest15();
    FuzzTest16();
    FuzzTest17();
    FuzzTest18();
    FuzzTest19();
    FuzzTest20();
    FuzzTest21();
    FuzzTest22();
    FuzzTest23();
    FuzzTest24();
    FuzzTest25();
    FuzzTest27();
    FuzzTest28();
    FuzzTest29();
    FuzzTest30();
    DoFuzzTest();
    unlink(DATA_PATH);
    return true;
}

void DoFuzzTest()
{
    FuzzTest31();
    FuzzTest32();
    FuzzTest33();
    FuzzTest34();
    FuzzTest35();
    FuzzTest37();
    FuzzTest38();
    FuzzTest39();
    FuzzTest40();
    FuzzTest41();
    FuzzTest45();
    FuzzTest46();
    FuzzTest47();
    FuzzTest48();
    FuzzTest49();
    FuzzTest50();
    FuzzTest51();
    FuzzTest52();
    FuzzTest53();
    FuzzTest54();
}

void FuzzTest1()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
        return;
    }

    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    Status ret = demuxer->SetDataSource(source);
    if (ret != Status::OK) {
        close(fd);
        return;
    }
    demuxer->OnInterrupted(false);
    demuxer->SetBundleName("fuzzTest");

    std::shared_ptr<AVBufferQueue> inputQueue =
            AVBufferQueue::Create(NUMBER_4, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> producer = inputQueue->GetProducer();
    demuxer->SetOutputBufferQueue(0, producer);
    auto queueMap = demuxer->GetBufferQueueProducerMap();
    close(fd);
    return;
}

void FuzzTest6()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(0, inputBufferQueueProducer);
    demuxer->SetDecoderFramerateUpperLimit(NUMBER_111, 0);
    demuxer->SetSpeed(NUMBER_SPEED);
    demuxer->SetFrameRate(1, NUMBER_25);
    demuxer->DisableMediaTrack(Plugins::MediaType::VIDEO);
    demuxer->IsRenderNextVideoFrameSupported();
    close(fd);
}

void FuzzTest7()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    int32_t invalidTrackId = NUMBER_100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->SetTrackNotifyFlag(invalidTrackId, true);
    demuxer->SetTrackNotifyFlag(trackId, true);
    demuxer->OnBufferAvailable(trackId);
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    close(fd);
}

void FuzzTest8()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    std::shared_ptr<MediaDemuxerTestCallback> callback = std::make_shared<MediaDemuxerTestCallback>();
    demuxer->SetDrmCallback(callback);
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    close(fd);
}

void FuzzTest9()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    int64_t duration = 0;
    demuxer->mediaMetaData_.globalMeta = std::make_shared<Meta>();
    demuxer->GetDuration(duration);
    close(fd);
}

void FuzzTest10()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    close(fd);
}

void FuzzTest11()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->AddDemuxerCopyTask(trackId, TaskType::GLOBAL);
    close(fd);
}

void FuzzTest12()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    int32_t invalidTrackId = NUMBER_100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->bufferQueueMap_.erase(trackId);
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->AddDemuxerCopyTask(invalidTrackId, TaskType::GLOBAL);
    demuxer->bufferQueueMap_.insert(
        std::pair<int32_t, sptr<AVBufferQueueProducer>>(invalidTrackId, inputBufferQueueProducer));
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    close(fd);
}

void FuzzTest13()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->OnDumpInfo(-1);
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    close(fd);
}

void FuzzTest14()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->useBufferQueue_ = true;
    demuxer->SelectTrack(trackId);
    demuxer->UnselectTrack(trackId);
    close(fd);
}

void FuzzTest15()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    int64_t realSeekTime = 0;
    demuxer->SeekTo(0, SeekMode::SEEK_CLOSEST_INNER, realSeekTime);
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->SeekTo(0, SeekMode::SEEK_CLOSEST_INNER, realSeekTime);
    close(fd);
}

void FuzzTest16()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->isSelectBitRate_.store(true);
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->SelectBitRate(0);
    close(fd);
}

void FuzzTest17()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    demuxer->isSelectBitRate_.store(true);
    demuxer->SelectBitRate(0);
    close(fd);
}

void FuzzTest18()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->videoTrackId_ = 0;
    demuxer->audioTrackId_ = NUMBER_1;
    demuxer->subtitleTrackId_ = NUMBER_2;
    demuxer->IsRightMediaTrack(MediaDemuxer::TRACK_ID_INVALID, DemuxerTrackType::VIDEO);
    demuxer->IsRightMediaTrack(0, DemuxerTrackType::VIDEO);
    demuxer->IsRightMediaTrack(NUMBER_1, DemuxerTrackType::AUDIO);
    demuxer->IsRightMediaTrack(NUMBER_2, DemuxerTrackType::SUBTITLE);
}

void FuzzTest19()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    int32_t invalidTrackId = NUMBER_100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->Flush();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->bufferQueueMap_.insert(
        std::pair<int32_t, sptr<AVBufferQueueProducer>>(invalidTrackId, inputBufferQueueProducer));
    demuxer->Flush();
    close(fd);
}

void FuzzTest20()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t invalidTrackId = NUMBER_100;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->taskMap_[invalidTrackId] = nullptr;
    demuxer->PauseAllTask();
    demuxer->source_ = nullptr;
    demuxer->StopAllTask();
    close(fd);
}

void FuzzTest21()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->Resume();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    close(fd);
}

void FuzzTest22()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();

    demuxer->streamDemuxer_ = nullptr;
    demuxer->source_ = nullptr;
    demuxer->inPreroll_.store(true);
    demuxer->Resume();

    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->inPreroll_.store(false);
    demuxer->Resume();
}

void FuzzTest23()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    int32_t invalidTrackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    demuxer->taskMap_[invalidTrackId] = nullptr;
    demuxer->Start();
    close(fd);
}

void FuzzTest24()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    close(fd);
}

void FuzzTest25()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t aTrackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(aTrackId, inputBufferQueueProducer);
    demuxer->isDump_ = true;
    close(fd);
}

void FuzzTest27()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t trackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(trackId, inputBufferQueueProducer);
    std::shared_ptr<Pipeline::EventReceiver> receiver = std::make_shared<TestEventReceiver>();
    demuxer->SetEventReceiver(receiver);
    demuxer->OnEvent({Plugins::PluginEventType::CLIENT_ERROR, "", "CLIENT_ERROR"});
    demuxer->OnEvent({Plugins::PluginEventType::BUFFERING_END, "", "BUFFERING_END"});
    demuxer->OnEvent({Plugins::PluginEventType::BUFFERING_START, "", "BUFFERING_START"});
    demuxer->OnEvent({Plugins::PluginEventType::CACHED_DURATION, "", "CACHED_DURATION"});
    demuxer->OnEvent({Plugins::PluginEventType::SOURCE_BITRATE_START, "", "SOURCE_BITRATE_START"});
    demuxer->OnEvent({Plugins::PluginEventType::EVENT_BUFFER_PROGRESS, "", "EVENT_BUFFER_PROGRESS"});
    demuxer->OnEvent({Plugins::PluginEventType::EVENT_CHANNEL_CLOSED, "", "EVENT_CHANNEL_CLOSED"});
    close(fd);
}

void FuzzTest28()
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
    info1.streamId = NUMBER_1;
    info1.bitRate = 0;
    info1.type = Plugins::VIDEO;
    streams.push_back(info1);
    streams.push_back(info1);

    Plugins::StreamInfo info2;
    info2.streamId = NUMBER_2;
    info2.bitRate = 0;
    info2.type = Plugins::SUBTITLE;
    streams.push_back(info2);
    streams.push_back(info2);

    demuxerPluginManager->InitDefaultPlay(streams);
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

    demuxerPluginManager->AddTrackMapInfo(0, 0);
    demuxerPluginManager->AddTrackMapInfo(NUMBER_1, NUMBER_1);
    demuxerPluginManager->AddTrackMapInfo(NUMBER_2, NUMBER_2);

    demuxerPluginManager->GetTrackInfoByStreamID(0, trackId, innerTrackId);
    demuxerPluginManager->GetInnerTrackIDByTrackID(0);
    demuxerPluginManager->CheckTrackIsActive(-1);
}

void FuzzTest29()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusOK");
    demuxer->audioTrackId_ = NUMBER_1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[0].streamID = 0;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_1].streamID = NUMBER_1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_2].streamID = NUMBER_2;

    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_1].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2].plugin = pluginMock;

    demuxer->isParserTaskEnd_ = false;
    uint32_t frameId = 0;
    std::vector<uint32_t> IFramePos = { NUMBER_100 };

    demuxer->Dts2FrameId(NUMBER_100, frameId);
    demuxer->GetIFramePos(IFramePos);

    demuxer->source_  = nullptr;
    demuxer->Dts2FrameId(NUMBER_100, frameId);
    demuxer->GetIFramePos(IFramePos);

    demuxer->demuxerPluginManager_  = nullptr;
    demuxer->Dts2FrameId(NUMBER_100, frameId);
    demuxer->GetIFramePos(IFramePos);

    demuxer->SetFrameRate(-1.0, 0);
    demuxer->SetFrameRate(1.0, 0);
}

void FuzzTest30()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusOK");
    demuxer->audioTrackId_ = NUMBER_1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[0].streamID = 0;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_1].streamID = NUMBER_1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_2].streamID = NUMBER_2;

    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_1].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2].plugin = pluginMock;

    demuxer->isParserTaskEnd_ = false;
    uint32_t frameId = 0;
    std::vector<uint32_t> IFramePos = { NUMBER_100 };

    demuxer->SeekMs2FrameId(NUMBER_100, frameId);
    demuxer->GetIFramePos(IFramePos);

    demuxer->source_  = nullptr;
    demuxer->SeekMs2FrameId(NUMBER_100, frameId);
    demuxer->GetIFramePos(IFramePos);

    demuxer->demuxerPluginManager_  = nullptr;
    demuxer->SeekMs2FrameId(NUMBER_100, frameId);
    demuxer->GetIFramePos(IFramePos);

    demuxer->SetFrameRate(-1.0, 0);
    demuxer->SetFrameRate(1.0, 0);
}

void FuzzTest31()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusOK");
    demuxer->audioTrackId_ = NUMBER_1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[0].streamID = 0;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_1].streamID = NUMBER_1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_2].streamID = NUMBER_2;

    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_1].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2].plugin = pluginMock;

    demuxer->isParserTaskEnd_ = false;
    int64_t seekMs = 0;
    std::vector<uint32_t> IFramePos = { NUMBER_2 };

    demuxer->FrameId2SeekMs(NUMBER_100, seekMs);
    demuxer->GetIFramePos(IFramePos);

    demuxer->source_  = nullptr;
    demuxer->FrameId2SeekMs(NUMBER_100, seekMs);
    demuxer->GetIFramePos(IFramePos);

    demuxer->demuxerPluginManager_  = nullptr;
    demuxer->FrameId2SeekMs(NUMBER_100, seekMs);
    demuxer->GetIFramePos(IFramePos);

    demuxer->SetFrameRate(-1.0, 0);
    demuxer->SetFrameRate(1.0, 0);
}

void FuzzTest32()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    uint64_t relativePresentationTimeUs;
    demuxer->GetRelativePresentationTimeUsByIndex(0, NUMBER_1, relativePresentationTimeUs);
    uint32_t index;
    demuxer->GetIndexByRelativePresentationTimeUs(0, NUMBER_1, index);

    demuxer->demuxerPluginManager_ = nullptr;
    demuxer->GetRelativePresentationTimeUsByIndex(0, NUMBER_1, relativePresentationTimeUs);
    demuxer->GetIndexByRelativePresentationTimeUs(0, NUMBER_1, index);
}

void FuzzTest33()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = NUMBER_100;

    demuxer->audioTrackId_ = 0;
    demuxer->shouldCheckAudioFramePts_ = false;
    demuxer->CheckDropAudioFrame(sample, 0);
    demuxer->lastAudioPts_ = NUMBER_100 + 1;
    demuxer->CheckDropAudioFrame(sample, 0);
    demuxer->shouldCheckAudioFramePts_ = true;
    demuxer->CheckDropAudioFrame(sample, 0);
    demuxer->lastAudioPts_ = NUMBER_100 - 1;
    demuxer->CheckDropAudioFrame(sample, 0);

    demuxer->subtitleTrackId_ = 1;
    demuxer->shouldCheckSubtitleFramePts_ = false;
    demuxer->CheckDropAudioFrame(sample, 1);
    demuxer->shouldCheckSubtitleFramePts_ = true;
    demuxer->lastSubtitlePts_ = NUMBER_100 + 1;
    demuxer->CheckDropAudioFrame(sample, 1);
    demuxer->lastSubtitlePts_ = NUMBER_100 - 1;
    demuxer->CheckDropAudioFrame(sample, 1);

    demuxer->CheckDropAudioFrame(sample, NUMBER_2);

    demuxer->videoTrackId_ = 1;
    demuxer->isDecodeOptimizationEnabled_ = true;

    uint8_t* data = new uint8_t[NUMBER_100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, NUMBER_100, NUMBER_100);
    demuxer->framerate_ = NUMBER_FRAMERATE_1;
    demuxer->speed_ = 1.0;
    demuxer->decoderFramerateUpperLimit_ = NUMBER_100;
    demuxer->IsBufferDroppable(buffer, 1);
    
    demuxer->framerate_ = NUMBER_FRAMERATE_2;
    demuxer->speed_ = 1.0;
    demuxer->decoderFramerateUpperLimit_ = NUMBER_100;
    demuxer->IsBufferDroppable(buffer, 1);

    buffer->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, true);
    demuxer->IsBufferDroppable(buffer, 1);

    delete[] data;
}

void FuzzTest34()
{
    int32_t fd = open(DATA_PATH, O_RDONLY);
    struct stat statBuffer;
    if (fstat(fd, &statBuffer) == -1) {
        perror("Error getting file status");
        close(fd);
    }
    std::string uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(statBuffer.st_size);

    int32_t aTrackId = 1;
    int32_t vTrackId = 0;
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->SetDataSource(std::make_shared<MediaSource>(uri));
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
        AVBufferQueue::Create(NUMBER_8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    demuxer->SetOutputBufferQueue(aTrackId, inputBufferQueueProducer);

    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->SetDumpInfo(true, 0);
    demuxer->isDecodeOptimizationEnabled_ = true;
    std::shared_ptr<AVBuffer> aBuffer = AVBuffer::CreateAVBuffer();
    std::shared_ptr<AVBuffer> vBuffer = AVBuffer::CreateAVBuffer();
    demuxer->bufferMap_[aTrackId] = aBuffer;
    demuxer->bufferMap_[vTrackId] = vBuffer;
    vBuffer->meta_->SetData(Media::Tag::VIDEO_BUFFER_CAN_DROP, true);
    close(fd);
}

void FuzzTest35()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusErrorUnknown");
    demuxer->audioTrackId_ = 1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[0].streamID = 0;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[1].streamID = 1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_2].streamID = NUMBER_2;

    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2].plugin = pluginMock;

    uint64_t relativePresentationTimeUs = 0;
    demuxer->GetRelativePresentationTimeUsByIndex(0, 0, relativePresentationTimeUs);
}

void FuzzTest37()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::vector<uint8_t> val{0, 0};
    std::multimap<std::string, std::vector<uint8_t>> info;
    info.insert(std::pair<std::string, std::vector<uint8_t>>("key", val));
    demuxer->localDrmInfos_ = info;
    demuxer->HandleSourceDrmInfoEvent(info);
}

void FuzzTest38()
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
    demuxerManager->GetTrackTypeByTrackID(0);
    demuxerManager->GetTrackTypeByTrackID(1);
    demuxerManager->GetTrackTypeByTrackID(NUMBER_2);
    demuxerManager->GetTrackTypeByTrackID(NUMBER_3);

    demuxerManager->IsSubtitleMime("application/x-subrip");
    demuxerManager->IsSubtitleMime("text/vtt");
    demuxerManager->IsSubtitleMime("aaaaa");
}

void FuzzTest39()
{
    std::shared_ptr<DemuxerPluginManager> demuxerManager = std::make_shared<DemuxerPluginManager>();
    Plugins::MediaInfo mediaInfo;
    demuxerManager->UpdateDefaultStreamID(mediaInfo, AUDIO, 1);
    demuxerManager->UpdateDefaultStreamID(mediaInfo, SUBTITLE, 1);
    demuxerManager->UpdateDefaultStreamID(mediaInfo, VIDEO, 1);
}

void FuzzTest40()
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
    demuxerManager->streamInfoMap_[NUMBER_2] = info1;

    demuxerManager->curVideoStreamID_ = 0;
    demuxerManager->curAudioStreamID_ = -1;
    demuxerManager->curSubTitleStreamID_ = -1;
    demuxerManager->Start();
    demuxerManager->Stop();
    demuxerManager->Reset();
    demuxerManager->Flush();
    int64_t realSeekTime;
    demuxerManager->SeekTo(1, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
}

void FuzzTest41()
{
    std::shared_ptr<DemuxerPluginManager> demuxerManager = std::make_shared<DemuxerPluginManager>();
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";

    MediaStreamInfo info;
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock =
        std::make_shared<DemuxerPluginSetDataSourceFailMock<0>>("StatusOK");
    info.plugin = pluginMock;
    info.pluginName = pluginName;
    demuxerManager->streamInfoMap_[0] = info;

    std::shared_ptr<StreamDemuxer> streamDemuxer = std::make_shared<StreamDemuxerMock>();
    std::thread initPluginThread([demuxerManager, streamDemuxer, pluginName]() {
        demuxerManager->InitPlugin(streamDemuxer, pluginName, 0);
    });
    initPluginThread.join();
}

void FuzzTest45()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    
    demuxer->source_->seekToTimeFlag_ = true;
    demuxer->videoTrackId_  = 0;
    demuxer->demuxerPluginManager_->isDash_ = false;

    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = NUMBER_100;
    demuxer->DoSelectTrack(0, MediaDemuxer::TRACK_ID_INVALID);
}

void FuzzTest46()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->HandleDashSelectTrack(0);

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
    demuxer->demuxerPluginManager_->curMediaInfo_.tracks.push_back(metaTmp4);

    demuxer->demuxerPluginManager_->AddTrackMapInfo(0, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(1, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(NUMBER_2, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(NUMBER_3, 0);

    demuxer->audioTrackId_ = 0;
    demuxer->videoTrackId_ = 1;
    demuxer->subtitleTrackId_ = NUMBER_2;

    demuxer->HandleDashSelectTrack(0);
    demuxer->HandleDashSelectTrack(1);
    demuxer->HandleDashSelectTrack(NUMBER_2);
    demuxer->HandleDashSelectTrack(NUMBER_3);
}

void FuzzTest47()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->audioTrackId_ = 0;
    demuxer->videoTrackId_ = 0;
    demuxer->subtitleTrackId_ = 0;

    demuxer->demuxerPluginManager_->isDash_ = false;
    demuxer->SeekToTimeAfter();
    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->isSelectBitRate_ = true;
    demuxer->SeekToTimeAfter();
}

void FuzzTest48()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    sample->pts_ = NUMBER_100;
    demuxer->audioTrackId_ = 0;
    demuxer->subtitleTrackId_ = NUMBER_2;
    demuxer->shouldCheckAudioFramePts_  = true;
    demuxer->lastAudioPts_  = NUMBER_200;
    demuxer->CheckDropAudioFrame(sample, 0);

    demuxer->shouldCheckAudioFramePts_  = false;
    demuxer->CheckDropAudioFrame(sample, NUMBER_2);
    demuxer->lastSubtitlePts_  = NUMBER_200;
    demuxer->shouldCheckAudioFramePts_  = true;
    demuxer->CheckDropAudioFrame(sample, NUMBER_2);

    demuxer->IsVideoEos();
    demuxer->videoTrackId_ = 0;
    demuxer->eosMap_[0] = true;
    demuxer->IsVideoEos();

    demuxer->IsRenderNextVideoFrameSupported();
    demuxer->ResumeDragging();
}

void FuzzTest49()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->streamDemuxer_->SetNewAudioStreamID(0);
    demuxer->streamDemuxer_->SetNewVideoStreamID(1);
    demuxer->streamDemuxer_->SetNewSubtitleStreamID(NUMBER_2);
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusErrorUnknown");

    demuxer->audioTrackId_ = 0;
    demuxer->videoTrackId_ = 1;
    demuxer->subtitleTrackId_ = NUMBER_2;

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
    demuxer->SelectTrackChangeStream(NUMBER_5);

    demuxer->demuxerPluginManager_->AddTrackMapInfo(0, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(1, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(NUMBER_2, 0);

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
    info3.streamID = NUMBER_2;
    info3.type = StreamType::SUBTITLE;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2] = info3;

    demuxer->SelectTrackChangeStream(0);
}

void FuzzTest50()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->source_->plugin_ = std::make_shared<SourcePluginMock>("StatusOK");
    demuxer->demuxerPluginManager_->isDash_ = true;
    demuxer->streamDemuxer_->changeStreamFlag_ = false;
    demuxer->SelectBitRate(1);

    std::shared_ptr<Meta> meta1 = std::make_shared<Meta>();
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);
    demuxer->mediaMetaData_.trackMetas.push_back(meta1);

    demuxer->videoTrackId_ = NUMBER_2;
    demuxer->useBufferQueue_ = true;
    demuxer->SelectBitRate(NUMBER_3);

    std::vector<uint32_t> bitRates;
    demuxer->GetBitRates(bitRates);

    demuxer->source_ = nullptr;
    int64_t durationMs;
    demuxer->GetDuration(durationMs);
}

void FuzzTest51()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    demuxer->videoTrackId_ = 1;
    demuxer->StartReferenceParser(1, false);
    demuxer->demuxerPluginManager_ = nullptr;
    demuxer->StartReferenceParser(1, false);
    demuxer->videoTrackId_ = MediaDemuxer::TRACK_ID_INVALID;
    demuxer->StartReferenceParser(1, false);
    demuxer->source_ = nullptr;
    demuxer->StartReferenceParser(1, false);
    demuxer->StartReferenceParser(-1, false);
}

void FuzzTest52()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock = std::make_shared<DemuxerPluginMock>("StatusErrorUnknown");
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock1 = std::make_shared<DemuxerPluginMock>("StatusOK");
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[0].streamID = 0;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[1].streamID = 1;
    demuxer->demuxerPluginManager_->temp2TrackInfoMap_[NUMBER_2].streamID = NUMBER_2;

    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = nullptr;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].plugin = pluginMock;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2].plugin = pluginMock1;

    demuxer->videoTrackId_ = 0;
    demuxer->StartReferenceParser(1, false);
    
    demuxer->videoTrackId_ = 1;
    demuxer->StartReferenceParser(1, false);

    demuxer->videoTrackId_ = NUMBER_2;
    demuxer->isFirstParser_ = true;
    demuxer->StartReferenceParser(1, false);
}

void FuzzTest53()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();

    FrameLayerInfo frameLayerInfo;
    demuxer->GetFrameLayerInfo(nullptr, frameLayerInfo);
    std::shared_ptr<AVBuffer> sample = std::make_shared<AVBuffer>();
    demuxer->demuxerPluginManager_  = nullptr;
    demuxer->GetFrameLayerInfo(sample, frameLayerInfo);
    demuxer->source_ = nullptr;
    demuxer->GetFrameLayerInfo(sample, frameLayerInfo);
}

void FuzzTest54()
{
    std::shared_ptr<MediaDemuxer> demuxer = std::make_shared<MediaDemuxer>();
    demuxer->streamDemuxer_ = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Plugins::DemuxerPlugin> pluginMock1 = std::make_shared<DemuxerPluginMock>("StatusOK");

    demuxer->demuxerPluginManager_->curVideoStreamID_ = 0;
    demuxer->demuxerPluginManager_->curAudioStreamID_ = 1;
    demuxer->demuxerPluginManager_->curSubTitleStreamID_ = NUMBER_2;
    demuxer->demuxerPluginManager_->streamInfoMap_[0].plugin = pluginMock1;
    demuxer->demuxerPluginManager_->streamInfoMap_[0].type = StreamType::VIDEO;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].plugin = pluginMock1;
    demuxer->demuxerPluginManager_->streamInfoMap_[1].type = StreamType::AUDIO;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2].plugin = pluginMock1;
    demuxer->demuxerPluginManager_->streamInfoMap_[NUMBER_2].type = StreamType::SUBTITLE;

    demuxer->SetCacheLimit(NUMBER_10);

    demuxer->demuxerPluginManager_->AddTrackMapInfo(0, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(1, 0);
    demuxer->demuxerPluginManager_->AddTrackMapInfo(NUMBER_2, 0);
    demuxer->demuxerPluginManager_->GetStreamTypeByTrackID(0);
}
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(uint8_t* data, size_t size)
{
    OHOS::Media::RunMediaDemuxerFuzz(data, size);
    return 0;
}

