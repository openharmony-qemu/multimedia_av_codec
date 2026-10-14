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

#include "download/downloader.h"
#include "monitor/download_monitor.h"
#include "http/http_media_downloader.h"
#include "utils/media_cached_buffer.h"
#include "download/network_client/http_curl_client.h"
#include "download/app_client.h"
#include "gtest/gtest.h"
#include "utils/string_utils.h"
#include <future>
#include <thread>

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class TestMediaDownloaderForMonitor : public MediaDownloader {
public:
    void Init() override {}
    void SetSourceStatisticsDfx(std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr) override
    {
        (void)rpInfoPtr;
    }
    bool Open(const std::string& url, const std::map<std::string, std::string>& httpHeader) override
    {
        (void)url;
        (void)httpHeader;
        return true;
    }
    void Close(bool isAsync) override
    {
        (void)isAsync;
    }
    void Pause() override {}
    void Resume() override {}
    Status Read(unsigned char* buff, ReadDataInfo& readDataInfo) override
    {
        (void)buff;
        (void)readDataInfo;
        return Status::OK;
    }
    uint64_t GetBufferSize() const override
    {
        return 0;
    }
    size_t GetContentLength() const override
    {
        return 0;
    }
    int64_t GetDuration() const override
    {
        return 0;
    }
    Seekable GetSeekable() const override
    {
        return Seekable::UNSEEKABLE;
    }
    void SetCallback(const std::shared_ptr<Callback>& cb) override
    {
        (void)cb;
    }
    void SetStatusCallback(StatusCallbackFunc cb) override
    {
        (void)cb;
    }
    void SetInterruptState(bool isInterruptNeeded) override
    {
        (void)isInterruptNeeded;
    }
    void SetAppUid(int32_t appUid) override
    {
        (void)appUid;
    }

    Status GetStreamInfo(std::vector<StreamInfo>& streams, bool isUpdate = false) override
    {
        (void)streams;
        getStreamInfoCalled_ = true;
        getStreamInfoIsUpdate_ = isUpdate;
        return getStreamInfoReturn_;
    }

    void SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId) override
    {
        setDefaultStreamIdCalled_ = true;
        lastVideoId_ = videoStreamId;
        lastAudioId_ = audioStreamId;
        lastSubtitleId_ = subTitleStreamId;
    }

    std::string GetMaskUrlByReadIndex(int32_t streamId) override
    {
        (void)streamId;
        getMaskedUrlCalled_ = true;
        return maskedUrlResult_;
    }

    bool getStreamInfoCalled_ {false};
    bool getStreamInfoIsUpdate_ {false};
    Status getStreamInfoReturn_ {Status::OK};

    bool setDefaultStreamIdCalled_ {false};
    int32_t lastVideoId_ {-1};
    int32_t lastAudioId_ {-1};
    int32_t lastSubtitleId_ {-1};

    bool getMaskedUrlCalled_ {false};
    std::string maskedUrlResult_;
};

class DownloaderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
protected:
    std::shared_ptr<Downloader> downloader;
};

constexpr int START_POS = 10;
constexpr int ONE_KILO = 1024;
constexpr int FIR_BUFFER = 128;
constexpr long LIVE_CONTENT_LENGTH = 2147483646;
static constexpr uint32_t CHUNK_SIZE = 16 * 1024;
static constexpr uint64_t MAX_CACHE_BUFFER_SIZE = 19 * 1024 * 1024;

void DownloaderUnitTest::SetUpTestCase(void)
{
}

void DownloaderUnitTest::TearDownTestCase(void)
{
}

void DownloaderUnitTest::SetUp(void)
{
    downloader = std::make_shared<Downloader>("test");
    downloader->Init();
}

void DownloaderUnitTest::TearDown(void)
{
    downloader.reset();
}

HWTEST_F(DownloaderUnitTest, DOWNLOADER_MONITOR_GetStreamInfo, TestSize.Level1)
{
    auto innerDownloader = std::make_shared<TestMediaDownloaderForMonitor>();
    std::shared_ptr<DownloadMonitor> monitor = std::make_shared<DownloadMonitor>(innerDownloader);

    std::vector<StreamInfo> streams;
    auto ret = monitor->GetStreamInfo(streams, true);

    EXPECT_EQ(Status::OK, ret);
    EXPECT_TRUE(innerDownloader->getStreamInfoCalled_);
    EXPECT_TRUE(innerDownloader->getStreamInfoIsUpdate_);
}

HWTEST_F(DownloaderUnitTest, DOWNLOADER_MONITOR_SetDefaultStreamId, TestSize.Level1)
{
    auto innerDownloader = std::make_shared<TestMediaDownloaderForMonitor>();
    std::shared_ptr<DownloadMonitor> monitor = std::make_shared<DownloadMonitor>(innerDownloader);

    int32_t videoId = 10;
    int32_t audioId = 20;
    int32_t subtitleId = 30;
    monitor->SetDefaultStreamId(videoId, audioId, subtitleId);

    EXPECT_TRUE(innerDownloader->setDefaultStreamIdCalled_);
    EXPECT_EQ(videoId, innerDownloader->lastVideoId_);
    EXPECT_EQ(audioId, innerDownloader->lastAudioId_);
    EXPECT_EQ(subtitleId, innerDownloader->lastSubtitleId_);
}

/**
 * @tc.name  : Test DownloadMonitor GetMaskUrlByReadIndex API
 * @tc.number: DownloadMonitor_GetMaskUrlByReadIndex_NullDownloader
 * @tc.desc  : Test GetMaskUrlByReadIndex returns empty when inner downloader is nullptr
 */
HWTEST_F(DownloaderUnitTest, DownloadMonitor_GetMaskUrlByReadIndex_NullDownloader, TestSize.Level1)
{
    std::shared_ptr<DownloadMonitor> monitor = std::make_shared<DownloadMonitor>(nullptr);
    EXPECT_EQ(monitor->GetMaskUrlByReadIndex(1), "");
}

/**
 * @tc.name  : Test DownloadMonitor GetMaskUrlByReadIndex API
 * @tc.number: DownloadMonitor_GetMaskUrlByReadIndex_Delegate
 * @tc.desc  : Test GetMaskUrlByReadIndex delegates to inner downloader and returns masked url
 */
HWTEST_F(DownloaderUnitTest, DownloadMonitor_GetMaskUrlByReadIndex_Delegate, TestSize.Level1)
{
    auto innerDownloader = std::make_shared<TestMediaDownloaderForMonitor>();
    innerDownloader->maskedUrlResult_ = "https://host/***/file.ts";
    std::shared_ptr<DownloadMonitor> monitor = std::make_shared<DownloadMonitor>(innerDownloader);

    EXPECT_EQ(monitor->GetMaskUrlByReadIndex(1), "https://host/***/file.ts");
    EXPECT_TRUE(innerDownloader->getMaskedUrlCalled_);
}

HWTEST_F(DownloaderUnitTest, ClearMiddleReadFragment, TestSize.Level0)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE));
    EXPECT_FALSE(cachedMediaBuffer.ClearMiddleReadFragment(10, 1000));
    cachedMediaBuffer.IsReadSplit(100);
}

HWTEST_F(DownloaderUnitTest, Downloader_Construct_nullptr, TestSize.Level0)
{
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_EQ(downloader->name_, std::string("test"));
}

HWTEST_F(DownloaderUnitTest, Retry_1, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    EXPECT_FALSE(downloader->Retry(Request_));
}

HWTEST_F(DownloaderUnitTest, Retry_2, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isAppBackground_ = true;
    EXPECT_TRUE(downloader->Retry(Request_));
}

HWTEST_F(DownloaderUnitTest, Retry_3, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isDestructor_ = true;
    EXPECT_FALSE(downloader->Retry(Request_));
}

HWTEST_F(DownloaderUnitTest, Retry_4, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isDestructor_ = false;
    EXPECT_FALSE(downloader->Retry(Request_));
}

HWTEST_F(DownloaderUnitTest, Retry_5, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isDestructor_ = false;
    downloader->shouldStartNextRequest_ = false;
    EXPECT_TRUE(downloader->Retry(Request_));
}

HWTEST_F(DownloaderUnitTest, Retry_6, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isDestructor_ = false;
    downloader->shouldStartNextRequest_ = false;
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    EXPECT_TRUE(downloader->Retry(Request_));
}

bool TestLastCheck(CacheMediaChunkBuffer& cachedMediaBuffer)
{
    auto& fragmentCacheBuffer = cachedMediaBuffer.impl_->fragmentCacheBuffer_;
    auto pos = fragmentCacheBuffer.begin();
    while (pos != fragmentCacheBuffer.end()) {
        cachedMediaBuffer.impl_->lruCache_.Delete(pos->offsetBegin);
        cachedMediaBuffer.impl_->totalReadSize_ -= pos->totalReadSize;
        cachedMediaBuffer.impl_->freeChunks_.splice(cachedMediaBuffer.impl_->freeChunks_.end(), pos->chunks);
        pos = fragmentCacheBuffer.erase(pos);
    }

    auto success = cachedMediaBuffer.impl_->Check();
    if (!success) {
        cachedMediaBuffer.Dump(0);
    }
    return success;
}

bool TestCheck(CacheMediaChunkBuffer& cachedMediaBuffer)
{
    auto success = cachedMediaBuffer.impl_->Check();
    if (!success) {
        cachedMediaBuffer.Dump(0);
    }
    return success;
}

HWTEST_F(DownloaderUnitTest, cachedMediaBuffer, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE));
    EXPECT_EQ(0u, cachedMediaBuffer.GetBufferSize(0));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(0));

    constexpr int64_t mediaSize = ONE_KILO * ONE_KILO * 3;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);
    size_t writeLen = ONE_KILO;
    size_t* data = (size_t*)mp4Data.get();
    for (size_t i = 0; i < mediaSize / sizeof(size_t); i++) {
        data[i] = i;
    }

    int64_t offset1 = 0;
    ASSERT_EQ(writeLen, cachedMediaBuffer.Write(mp4Data.get(), offset1, writeLen));
    EXPECT_EQ(writeLen, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset1));

    int64_t offset2 = static_cast<int64_t>(writeLen) * 3;
    ASSERT_EQ(writeLen, cachedMediaBuffer.Write(mp4Data.get(), offset2, writeLen));
    EXPECT_EQ(writeLen, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ(static_cast<size_t>(offset2), cachedMediaBuffer.GetNextBufferOffset(offset1));
    EXPECT_EQ(static_cast<size_t>(offset2), cachedMediaBuffer.GetNextBufferOffset(offset1 + writeLen - 3));

    cachedMediaBuffer.Dump(0);
    EXPECT_EQ(static_cast<size_t>(offset2), cachedMediaBuffer.GetNextBufferOffset(offset1 + writeLen));
    EXPECT_EQ(static_cast<size_t>(offset2), cachedMediaBuffer.GetNextBufferOffset(offset1 + writeLen + 1));
    EXPECT_EQ(static_cast<size_t>(offset2), cachedMediaBuffer.GetNextBufferOffset(offset2 - 2));

    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset2));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset2 + writeLen));
}

HWTEST_F(DownloaderUnitTest, LargeOffsetSpan, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    cachedMediaBuffer.SetIsLargeOffsetSpan(true);
    ASSERT_EQ(false, cachedMediaBuffer.Init(0, CHUNK_SIZE));
    ASSERT_EQ(true, cachedMediaBuffer.Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE));
    ASSERT_EQ(false, cachedMediaBuffer.Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE));
    ASSERT_EQ(MAX_CACHE_BUFFER_SIZE, cachedMediaBuffer.GetFreeSize());

    constexpr int64_t mediaSize = ONE_KILO * ONE_KILO * 15;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t offset = 0;
    size_t writeLen = ONE_KILO * 50;
    uint32_t buffer[ONE_KILO * 30] = {0};
    size_t readSize = ONE_KILO * 20;
    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(writeLen, cachedMediaBuffer.Write(mp4Data.get() + offset, offset, writeLen));
        EXPECT_EQ(true, cachedMediaBuffer.Check());
        ASSERT_EQ(readSize, cachedMediaBuffer.Read(buffer, offset, readSize));
        offset += ONE_KILO * 100;
    }
    EXPECT_EQ(true, cachedMediaBuffer.Check());
    ASSERT_EQ(0, cachedMediaBuffer.Read(buffer, offset, readSize));
    EXPECT_EQ(false, cachedMediaBuffer.ClearChunksOfFragment(ONE_KILO * 500));
    EXPECT_EQ(true, cachedMediaBuffer.ClearChunksOfFragment(ONE_KILO * 1133));
    EXPECT_EQ(true, cachedMediaBuffer.ClearFragmentBeforeOffset(ONE_KILO * 1401));
    cachedMediaBuffer.Dump(0);
    EXPECT_EQ(false, cachedMediaBuffer.Seek(ONE_KILO * 1300));
    EXPECT_EQ(true, cachedMediaBuffer.Seek(ONE_KILO * 1700));
    size_t offset1 = ONE_KILO * 1560;
    ASSERT_EQ(CHUNK_SIZE, cachedMediaBuffer.Write(mp4Data.get() + offset1, offset1, CHUNK_SIZE));
    EXPECT_EQ(false, cachedMediaBuffer.ClearMiddleReadFragment(ONE_KILO * 1500, ONE_KILO * 1800));
    EXPECT_EQ(true, cachedMediaBuffer.Seek(ONE_KILO * 1700));
    ASSERT_EQ(true, TestLastCheck(cachedMediaBuffer));
}

HWTEST_F(DownloaderUnitTest, HLSReadSplitChunk, TestSize.Level1)
{
    std::shared_ptr<CacheMediaChunkBufferHlsImpl> cachedMediaBuffer = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    constexpr size_t totalSize = CHUNK_SIZE * 400;
    ASSERT_EQ(true, cachedMediaBuffer->Init(totalSize, CHUNK_SIZE));
    ASSERT_EQ(totalSize, cachedMediaBuffer->GetFreeSize());

    constexpr int64_t mediaSize = CHUNK_SIZE * 400;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t offset = 0;
    size_t writeLen = CHUNK_SIZE * 8;
    uint32_t buffer[CHUNK_SIZE * 2] = {0};
    size_t readSize = CHUNK_SIZE * 3;
    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(writeLen, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen));
        ASSERT_EQ(readSize, cachedMediaBuffer->Read(buffer, offset, readSize));
        ASSERT_EQ(readSize, cachedMediaBuffer->Read(buffer, offset + CHUNK_SIZE * 5, readSize));
        offset += CHUNK_SIZE * 10;
    }
    size_t readSize1 = CHUNK_SIZE * 2;
    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(writeLen, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen));
        ASSERT_EQ(readSize1, cachedMediaBuffer->Read(buffer, offset, readSize1));
        ASSERT_EQ(readSize1, cachedMediaBuffer->Read(buffer, offset + CHUNK_SIZE * 5 + 1, readSize1));
        offset += CHUNK_SIZE * 10;
    }

    size_t offset1 = CHUNK_SIZE * 9;
    size_t writeLen1 = CHUNK_SIZE;
    ASSERT_EQ(writeLen1, cachedMediaBuffer->Write(mp4Data.get() + offset1, offset1, writeLen1));
    EXPECT_EQ(true, cachedMediaBuffer->Check());
}

HWTEST_F(DownloaderUnitTest, HLSFreeChunk, TestSize.Level1)
{
    std::shared_ptr<CacheMediaChunkBufferHlsImpl> cachedMediaBuffer = std::make_shared<CacheMediaChunkBufferHlsImpl>();
    constexpr size_t chunkSize = 512;
    constexpr size_t totalSize = chunkSize * 200;
    ASSERT_EQ(true, cachedMediaBuffer->Init(totalSize, chunkSize));
    ASSERT_EQ(totalSize, cachedMediaBuffer->GetFreeSize());

    constexpr int64_t mediaSize = chunkSize * 400;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t offset = 0;
    size_t writeLen = chunkSize * 10;
    for (int i = 0; i < 20; ++i) {
        ASSERT_EQ(writeLen, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen));
        offset += chunkSize * 11;
    }

    ASSERT_EQ(chunkSize, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, chunkSize));
    offset += chunkSize * 11;
    ASSERT_EQ(0, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen));
    EXPECT_EQ(true, cachedMediaBuffer->Check());
}

HWTEST_F(DownloaderUnitTest, HTTPFreeChunk0, TestSize.Level1)
{
    std::shared_ptr<CacheMediaChunkBufferImpl> cachedMediaBuffer = std::make_shared<CacheMediaChunkBufferImpl>();
    constexpr size_t chunkSize = 512;
    constexpr size_t totalSize = chunkSize * 128;
    ASSERT_EQ(true, cachedMediaBuffer->Init(totalSize, chunkSize));
    ASSERT_EQ(totalSize, cachedMediaBuffer->GetFreeSize());

    constexpr int64_t mediaSize = chunkSize * 400;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t offset = 0;
    size_t writeLen = chunkSize * 20;
    for (int i = 0; i < 10; ++i) {
        ASSERT_EQ(writeLen, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen));
        offset += chunkSize * 21;
    }

    size_t writeLen1 = chunkSize * 9;
    ASSERT_EQ(writeLen1, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen1));
    offset += chunkSize * 11;
    ASSERT_EQ(writeLen1, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen1));
    EXPECT_EQ(true, cachedMediaBuffer->Check());
}

HWTEST_F(DownloaderUnitTest, HTTPFreeChunk1, TestSize.Level1)
{
    std::shared_ptr<CacheMediaChunkBufferImpl> cachedMediaBuffer = std::make_shared<CacheMediaChunkBufferImpl>();
    constexpr size_t chunkSize = 512;
    constexpr size_t totalSize = chunkSize * 128;
    ASSERT_EQ(true, cachedMediaBuffer->Init(totalSize, chunkSize));
    ASSERT_EQ(totalSize, cachedMediaBuffer->GetFreeSize());

    constexpr int64_t mediaSize = chunkSize * 400;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t offset = 0;
    size_t writeLen = chunkSize * 45;
    for (int i = 0; i < 3; ++i) {
        ASSERT_EQ(writeLen, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen));
        offset += chunkSize * 60;
    }

    EXPECT_EQ(true, cachedMediaBuffer->Check());
}

HWTEST_F(DownloaderUnitTest, HTTPFreeChunk2, TestSize.Level1)
{
    std::shared_ptr<CacheMediaChunkBufferImpl> cachedMediaBuffer = std::make_shared<CacheMediaChunkBufferImpl>();
    constexpr size_t chunkSize = 512;
    constexpr size_t totalSize = chunkSize * 128;
    ASSERT_EQ(true, cachedMediaBuffer->Init(totalSize, chunkSize));
    ASSERT_EQ(totalSize, cachedMediaBuffer->GetFreeSize());

    constexpr int64_t mediaSize = chunkSize * 400;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t offset = 0;
    size_t writeLen = chunkSize * 40;
    uint32_t buffer[ONE_KILO * 30] = {0};
    size_t readSize = chunkSize * 10;
    for (int i = 0; i < 3; ++i) {
        ASSERT_EQ(writeLen, cachedMediaBuffer->Write(mp4Data.get() + offset, offset, writeLen));
        ASSERT_EQ(readSize, cachedMediaBuffer->Read(buffer, offset, readSize));
        readSize += chunkSize * 10;
        offset += chunkSize * 60;
    }

    EXPECT_EQ(true, cachedMediaBuffer->Check());
}

HWTEST_F(DownloaderUnitTest, WriteMerger, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(MAX_CACHE_BUFFER_SIZE, CHUNK_SIZE));
    EXPECT_EQ(0u, cachedMediaBuffer.GetBufferSize(0));

    constexpr int64_t mediaSize = ONE_KILO * ONE_KILO * 5;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t writeLen0 = ONE_KILO * 15;
    size_t offset0 = ONE_KILO * 10;
    ASSERT_EQ(writeLen0, cachedMediaBuffer.Write(mp4Data.get() + offset0, offset0, writeLen0));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    size_t writeLen1 = ONE_KILO * 10;
    size_t offset1 = ONE_KILO * 30;
    ASSERT_EQ(writeLen1, cachedMediaBuffer.Write(mp4Data.get() + offset1, offset1, writeLen1));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    size_t writeLen2 = ONE_KILO * 30;
    size_t offset2 = ONE_KILO * 5;
    ASSERT_EQ(writeLen2, cachedMediaBuffer.Write(mp4Data.get() + offset2, offset2, writeLen2));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    auto fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    ASSERT_EQ(offset2, fragmentInfo.offsetBegin);
    ASSERT_EQ(writeLen1 + offset1 - offset2, fragmentInfo.dataLength);
    EXPECT_EQ(true, cachedMediaBuffer.impl_->DumpAndCheckInner());
    ASSERT_EQ(true, TestLastCheck(cachedMediaBuffer));
}

HWTEST_F(DownloaderUnitTest, DeleteOtherHasReadFragmentCache, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    uint64_t totalSize = FIR_BUFFER * ONE_KILO;
    uint32_t chunkSize = FIR_BUFFER;
    EXPECT_EQ(true, cachedMediaBuffer.Init(totalSize, chunkSize));

    int64_t mediaSize = ONE_KILO * 8;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);
    size_t* data = (size_t*)mp4Data.get();
    for (size_t i = 0; i < mediaSize / sizeof(size_t); i++) {
        data[i] = i;
    }

    size_t writeLen = FIR_BUFFER * 2;
    int64_t offset1 = 0;
    ASSERT_EQ(writeLen, cachedMediaBuffer.Write(mp4Data.get(), offset1, writeLen));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset2 = FIR_BUFFER * 10;
    size_t writeLen2 = FIR_BUFFER * 2;
    ASSERT_EQ(writeLen2, cachedMediaBuffer.Write(mp4Data.get() + offset2, offset2, writeLen2));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset3 = FIR_BUFFER * 15;
    size_t writeLen3 = FIR_BUFFER * 2;
    ASSERT_EQ(writeLen3, cachedMediaBuffer.Write(mp4Data.get() + offset3, offset3, writeLen3));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    uint32_t buffer[ONE_KILO * 2] = {0};
    size_t readSize = 128u;
    ASSERT_EQ(readSize, cachedMediaBuffer.Read(buffer, offset2, readSize));
    ASSERT_EQ(writeLen, cachedMediaBuffer.Read(buffer, offset1, writeLen));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset5 = writeLen;
    size_t writeLen5 = FIR_BUFFER * 3;
    ASSERT_EQ(writeLen5, cachedMediaBuffer.Write(mp4Data.get() + offset5, offset5, writeLen5));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    ASSERT_EQ(writeLen, cachedMediaBuffer.Read(buffer, offset1 + writeLen, writeLen));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset7 = writeLen5 + offset5;
    size_t writeLen7 = FIR_BUFFER;
    ASSERT_EQ(writeLen7, cachedMediaBuffer.Write(mp4Data.get() + offset7, offset7, writeLen7));
    EXPECT_EQ(true, cachedMediaBuffer.Check());
}

HWTEST_F(DownloaderUnitTest, ReadSplitChunk_0, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    constexpr uint32_t chunkNum = 2 * 15 * 100;
    constexpr uint32_t chunKSizePer = 512;
    uint64_t totalSize = chunKSizePer * chunkNum;
    uint32_t chunkSize = chunKSizePer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(totalSize, chunkSize));
    EXPECT_EQ(0u, cachedMediaBuffer.GetBufferSize(0));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(0));
    constexpr int64_t mediaSize = ONE_KILO * (chunkNum + 1);
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);
    size_t writeLen1 = chunKSizePer * 20 + 8;
    int64_t offset1 = 0;
    ASSERT_EQ(writeLen1, cachedMediaBuffer.Write(mp4Data.get(), offset1, writeLen1));
    EXPECT_EQ(writeLen1, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset1));
    auto fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    EXPECT_EQ(0, fragmentInfo.chunks.front()->offset);
    size_t readLen2 = chunkSize - 8;
    char buffer[chunKSizePer * chunkNum] = {0};
    EXPECT_EQ(readLen2, cachedMediaBuffer.Read(buffer, offset1, readLen2));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, readLen2));
    cachedMediaBuffer.Dump(0);
    int64_t chunkOffsetReadBegin = 16;
    size_t readLen3 = 512 + 8;
    int64_t offset3 = chunKSizePer * 2 * 6 + chunkOffsetReadBegin;
    EXPECT_EQ(true, cachedMediaBuffer.Seek(offset3));
    EXPECT_EQ(readLen3, cachedMediaBuffer.Read(buffer, offset3, readLen3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, readLen3));

    fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    EXPECT_EQ(offset1 + (int64_t)readLen2, fragmentInfo.accessLength);
    EXPECT_EQ((int64_t)readLen2, fragmentInfo.accessLength);
    EXPECT_EQ(0, fragmentInfo.offsetBegin);
    EXPECT_EQ(offset3, fragmentInfo.dataLength); // changed
    EXPECT_EQ(*fragmentInfo.chunks.begin(), *fragmentInfo.accessPos);

    fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.back();
    EXPECT_EQ((int64_t)readLen3, fragmentInfo.accessLength);
    EXPECT_EQ(offset3, fragmentInfo.offsetBegin);
    EXPECT_EQ((int64_t)writeLen1 - offset3, fragmentInfo.dataLength); // changed
    EXPECT_EQ(*std::next(fragmentInfo.chunks.begin()), *fragmentInfo.accessPos);
    cachedMediaBuffer.Dump(0);
    ASSERT_EQ(true, TestCheck(cachedMediaBuffer));

    EXPECT_EQ((size_t)offset3, cachedMediaBuffer.Read(buffer, offset1, (size_t)offset3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, (size_t)offset3));

    EXPECT_EQ(writeLen1 - (size_t)offset3, cachedMediaBuffer.Read(buffer, offset3, writeLen1 - (size_t)offset3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, writeLen1 - (size_t)offset3));
    ASSERT_EQ(true, TestLastCheck(cachedMediaBuffer));
}

HWTEST_F(DownloaderUnitTest, ReadSplitChunk_1, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    constexpr uint32_t chunkNum = 2 * 15 * 100;
    constexpr uint32_t chunKSizePer = 512;
    uint64_t totalSize = chunKSizePer * chunkNum;
    uint32_t chunkSize = chunKSizePer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(totalSize, chunkSize));
    EXPECT_EQ(0u, cachedMediaBuffer.GetBufferSize(0));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(0));

    constexpr int64_t mediaSize = ONE_KILO * (chunkNum + 1);
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);
    size_t writeLen1 = chunKSizePer * 20 + 8;
    int64_t offset1 = 0;
    ASSERT_EQ(writeLen1, cachedMediaBuffer.Write(mp4Data.get(), offset1, writeLen1));
    EXPECT_EQ(writeLen1, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset1));
    auto fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    EXPECT_EQ(0, fragmentInfo.chunks.front()->offset);

    size_t readLen2 = chunkSize - 8;
    char buffer[chunKSizePer * chunkNum] = {0};
    EXPECT_EQ(readLen2, cachedMediaBuffer.Read(buffer, offset1, readLen2));

    cachedMediaBuffer.Dump(0);

    int64_t chunkOffsetReadBegin = 0;
    size_t readLen3 = 512 + 8;
    int64_t offset3 = chunKSizePer * 2 * 6 + chunkOffsetReadBegin;
    EXPECT_EQ(true, cachedMediaBuffer.Seek(offset3));
    EXPECT_EQ(readLen3, cachedMediaBuffer.Read(buffer, offset3, readLen3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, readLen3));

    fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    EXPECT_EQ(offset1 + (int64_t)readLen2, fragmentInfo.accessLength);
    EXPECT_EQ((int64_t)readLen2, fragmentInfo.accessLength);
    EXPECT_EQ(0, fragmentInfo.offsetBegin);
    EXPECT_EQ(offset3, fragmentInfo.dataLength);
    EXPECT_EQ(*fragmentInfo.chunks.begin(), *fragmentInfo.accessPos);

    fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.back();
    EXPECT_EQ((int64_t)readLen3, fragmentInfo.accessLength);
    EXPECT_EQ(offset3, fragmentInfo.offsetBegin);
    EXPECT_EQ((int64_t)writeLen1 - offset3, fragmentInfo.dataLength);
    EXPECT_EQ(*std::next(fragmentInfo.chunks.begin()), *fragmentInfo.accessPos);
    cachedMediaBuffer.Dump(0);
    ASSERT_EQ(true, TestCheck(cachedMediaBuffer));

    EXPECT_EQ((size_t)offset3, cachedMediaBuffer.Read(buffer, offset1, (size_t)offset3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, (size_t)offset3));

    EXPECT_EQ(writeLen1 - (size_t)offset3, cachedMediaBuffer.Read(buffer, offset3, writeLen1 - (size_t)offset3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, writeLen1 - (size_t)offset3));
    ASSERT_EQ(true, TestLastCheck(cachedMediaBuffer));
}

HWTEST_F(DownloaderUnitTest, ReadSplitChunk_2, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    constexpr uint32_t chunkNum = 2 * 15 * 100;
    constexpr uint32_t chunKSizePer = 512;
    uint64_t totalSize = chunKSizePer * chunkNum;
    uint32_t chunkSize = chunKSizePer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(totalSize, chunkSize));
    EXPECT_EQ(0u, cachedMediaBuffer.GetBufferSize(0));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(0));

    constexpr int64_t mediaSize = ONE_KILO * (chunkNum + 1);
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t writeLen1 = chunKSizePer * 20 + 8;
    int64_t offset1 = 0;
    ASSERT_EQ(writeLen1, cachedMediaBuffer.Write(mp4Data.get(), offset1, writeLen1));
    EXPECT_EQ(writeLen1, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset1));
    auto fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    EXPECT_EQ(0, fragmentInfo.chunks.front()->offset);


    size_t writeLen2 = 10;
    int64_t wrOffset2 = chunKSizePer * 25 + 8;
    ASSERT_EQ(writeLen2, cachedMediaBuffer.Write(mp4Data.get(), wrOffset2, writeLen2));

    size_t readLen2 = chunkSize - 8;
    char buffer[chunKSizePer * chunkNum] = {0};
    EXPECT_EQ(readLen2, cachedMediaBuffer.Read(buffer, offset1, readLen2));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, readLen2));

    cachedMediaBuffer.Dump(0);

    int64_t chunkOffsetReadBegin = 0;
    size_t readLen3 = 512 + 8;
    int64_t offset3 = chunKSizePer * 2 * 6 + chunkOffsetReadBegin;
    EXPECT_EQ(true, cachedMediaBuffer.Seek(offset3));
    EXPECT_EQ(readLen3, cachedMediaBuffer.Read(buffer, offset3, readLen3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, readLen3));

    fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    EXPECT_EQ(offset1 + (int64_t)readLen2, fragmentInfo.accessLength);
    EXPECT_EQ((int64_t)readLen2, fragmentInfo.accessLength);
    EXPECT_EQ(0, fragmentInfo.offsetBegin);
    EXPECT_EQ(offset3, fragmentInfo.dataLength);
    EXPECT_EQ(*fragmentInfo.chunks.begin(), *fragmentInfo.accessPos);

    fragmentInfo = *std::next(cachedMediaBuffer.impl_->fragmentCacheBuffer_.begin()) ;
    EXPECT_EQ((int64_t)readLen3, fragmentInfo.accessLength);
    EXPECT_EQ(offset3, fragmentInfo.offsetBegin);
    EXPECT_EQ((int64_t)writeLen1 - offset3, fragmentInfo.dataLength);
    EXPECT_EQ(*std::next(fragmentInfo.chunks.begin()), *fragmentInfo.accessPos);
    cachedMediaBuffer.Dump(0);
    ASSERT_EQ(true, TestCheck(cachedMediaBuffer));

    EXPECT_EQ((size_t)offset3, cachedMediaBuffer.Read(buffer, offset1, (size_t)offset3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, (size_t)offset3));

    EXPECT_EQ(writeLen1 - (size_t)offset3, cachedMediaBuffer.Read(buffer, offset3, writeLen1 - (size_t)offset3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, writeLen1 - (size_t)offset3));
}

HWTEST_F(DownloaderUnitTest, ReadSplitChunk_3, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    constexpr uint32_t chunkNum = 2 * 15 * 100;
    constexpr uint32_t chunKSizePer = 512;
    uint64_t totalSize = chunKSizePer * chunkNum;
    uint32_t chunkSize = chunKSizePer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(totalSize, chunkSize));
    EXPECT_EQ(0u, cachedMediaBuffer.GetBufferSize(0));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(0));

    constexpr int64_t mediaSize = ONE_KILO * (chunkNum + 1);
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);

    size_t writeLen1 = chunKSizePer * 20 + 8;
    int64_t offset1 = chunKSizePer + 8;
    ASSERT_EQ(writeLen1, cachedMediaBuffer.Write(mp4Data.get() + offset1, offset1, writeLen1));
    EXPECT_EQ(writeLen1, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset1));
    auto fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.front();
    EXPECT_EQ(offset1, fragmentInfo.chunks.front()->offset);

    size_t writeLen2 = 10;
    int64_t wrOffset2 = 0;
    ASSERT_EQ(writeLen2, cachedMediaBuffer.Write(mp4Data.get(), wrOffset2, writeLen2));

    size_t readLen2 = chunkSize - 8;
    char buffer[chunKSizePer * chunkNum] = {0};
    EXPECT_EQ(readLen2, cachedMediaBuffer.Read(buffer, offset1, readLen2));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, readLen2));

    cachedMediaBuffer.Dump(0);

    int64_t chunkOffsetReadBegin = 0;
    size_t readLen3 = 512 + 8;
    int64_t offset3 = chunKSizePer * 2 * 6 + chunkOffsetReadBegin;
    EXPECT_EQ(true, cachedMediaBuffer.Seek(offset3));
    EXPECT_EQ(readLen3, cachedMediaBuffer.Read(buffer, offset3, readLen3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, readLen3));
    cachedMediaBuffer.Dump(0);
    EXPECT_EQ(writeLen1, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(offset1));

    fragmentInfo = cachedMediaBuffer.impl_->fragmentCacheBuffer_.back();
    EXPECT_EQ((int64_t)readLen3, fragmentInfo.accessLength);
    EXPECT_EQ(offset3, fragmentInfo.offsetBegin);
    int64_t splitFirstLeftLenght = offset3 - offset1;
    EXPECT_EQ((int64_t)writeLen1 - splitFirstLeftLenght, fragmentInfo.dataLength);
    cachedMediaBuffer.Dump(0);

    ASSERT_EQ(true, TestCheck(cachedMediaBuffer));

    EXPECT_EQ((size_t)splitFirstLeftLenght, cachedMediaBuffer.Read(buffer, offset1, (size_t)splitFirstLeftLenght));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, (size_t)splitFirstLeftLenght));

    EXPECT_GT(writeLen1, (size_t)splitFirstLeftLenght);
    EXPECT_EQ(writeLen1, cachedMediaBuffer.Read(buffer, offset1, writeLen1));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset1, writeLen1));

    EXPECT_EQ(writeLen1 - (size_t)offset3, cachedMediaBuffer.Read(buffer, offset3, writeLen1 - (size_t)offset3));
    EXPECT_EQ(0, memcmp(buffer, mp4Data.get() + offset3, writeLen1 - (size_t)offset3));
}

HWTEST_F(DownloaderUnitTest, ReadSplitChunk_4, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    constexpr uint32_t chunkNum = 2 * 15 * 100;
    constexpr uint32_t chunKSizePer = 512;
    uint64_t totalSize = chunKSizePer * chunkNum;
    uint32_t chunkSize = chunKSizePer;
    ASSERT_EQ(true, cachedMediaBuffer.Init(totalSize, chunkSize));
    EXPECT_EQ(0u, cachedMediaBuffer.GetBufferSize(0));
    EXPECT_EQ(0u, cachedMediaBuffer.GetNextBufferOffset(0));

    constexpr int64_t mediaSize = ONE_KILO * (chunkNum + 1);
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);
    size_t* data = (size_t*) mp4Data.get();
    for (size_t i = 0; i < mediaSize / sizeof(size_t); ++i) {
        data[i] = i;
    }

    for (uint32_t i = 0; i < 20; ++i) {
        size_t writeLen1 = chunKSizePer - 16;
        int64_t offset1 = chunKSizePer * i + 8 ;
        ASSERT_EQ(writeLen1, cachedMediaBuffer.Write(mp4Data.get() + offset1, offset1, writeLen1));
    }

    for (uint32_t i = 0; i < 30; ++i) {
        size_t writeLen1 = chunKSizePer - 8;
        int64_t offset1 = chunKSizePer * i + 8 ;
        ASSERT_EQ(writeLen1, cachedMediaBuffer.Write(mp4Data.get() + offset1, offset1, writeLen1));
    }
    cachedMediaBuffer.Clear();
    ASSERT_EQ(true, TestLastCheck(cachedMediaBuffer));
}

HWTEST_F(DownloaderUnitTest, StopBufferring_1, TestSize.Level1)
{
    downloader->StopBufferring();
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, StopBufferring_2, TestSize.Level1)
{
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isAppBackground_ = true;
    downloader->StopBufferring();
    EXPECT_EQ(downloader->client_, nullptr);
    EXPECT_EQ(downloader->isAppBackground_.load(), true);
}

HWTEST_F(DownloaderUnitTest, StopBufferring_3, TestSize.Level1)
{
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isAppBackground_ = false;
    downloader->StopBufferring();
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, StopBufferring_4, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isAppBackground_ = false;
    downloader->StopBufferring();
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, StopBufferring_5, TestSize.Level1)
{
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->currentRequest_->startPos_ = START_POS;
    downloader->isAppBackground_ = false;
    downloader->shouldStartNextRequest_ = false;
    downloader->StopBufferring();
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_EQ(downloader->shouldStartNextRequest_.load(), false);
}

HWTEST_F(DownloaderUnitTest, StopBufferring_6, TestSize.Level1)
{
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isAppBackground_ = false;
    downloader->shouldStartNextRequest_ = false;
    downloader->StopBufferring();
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_EQ(downloader->shouldStartNextRequest_.load(), false);
}

HWTEST_F(DownloaderUnitTest, ToString_EmptyList, TestSize.Level1)
{
    list<string> lists;
    string result = ToString(lists);
    EXPECT_EQ(result, "");
}

HWTEST_F(DownloaderUnitTest, ToString_NoEmptyList, TestSize.Level1)
{
    list<string> lists = {"Hello", "World"};
    string result = ToString(lists);
    EXPECT_EQ(result, "Hello,World");
}

HWTEST_F(DownloaderUnitTest, InsertCharBefore_Test1, TestSize.Level1)
{
    string input = "hello world";
    char from = 'x';
    char preChar = 'y';
    char nextChar = 'z';
    string expected = "hello world";
    string actual = InsertCharBefore(input, from, preChar, nextChar);
    EXPECT_EQ(expected, actual);
}

HWTEST_F(DownloaderUnitTest, InsertCharBefore_Test2, TestSize.Level1)
{
    string input = "hello world";
    char from = 'l';
    char preChar = 'y';
    char nextChar = 'o';
    string expected = "heyllo woryld";
    string actual = InsertCharBefore(input, from, preChar, nextChar);
    EXPECT_EQ(expected, actual);
}

HWTEST_F(DownloaderUnitTest, InsertCharBefore_Test3, TestSize.Level1)
{
    string input = "hello world";
    char from = 'l';
    char preChar = 'y';
    char nextChar = 'l';
    string expected = "hello woryld";
    string actual = InsertCharBefore(input, from, preChar, nextChar);
    EXPECT_EQ(expected, actual);
}

HWTEST_F(DownloaderUnitTest, Trim_EmptyString, TestSize.Level1)
{
    string input = "";
    string expected = "";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_LeadingSpaces, TestSize.Level1)
{
    string input = "   Hello";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_TrailingSpaces, TestSize.Level1)
{
    string input = "Hello   ";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_LeadingAndTrailingSpaces, TestSize.Level1)
{
    string input = "  Hello   ";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, Trim_NoSpaces, TestSize.Level1)
{
    string input = "Hello";
    string expected = "Hello";
    EXPECT_EQ(Trim(input), expected);
}

HWTEST_F(DownloaderUnitTest, IsRegexValid_Test1, TestSize.Level1)
{
    string regex = "";
    bool result = IsRegexValid(regex);
    EXPECT_EQ(result, false);
}

HWTEST_F(DownloaderUnitTest, IsRegexValid_Test2, TestSize.Level1)
{
    string regex = "!@#$%^&*()";
    bool result = IsRegexValid(regex);
    EXPECT_EQ(result, false);
}

HWTEST_F(DownloaderUnitTest, IsRegexValid_Test3, TestSize.Level1)
{
    string regex = "abc123";
    bool result = IsRegexValid(regex);
    EXPECT_EQ(result, true);
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_01, TestSize.Level1)
{
    string input = "";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_02, TestSize.Level1)
{
    string input = "abc";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "abc");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_03, TestSize.Level1)
{
    string input = "a.b.c";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "a\\.b\\.c");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_04, TestSize.Level1)
{
    string input = "a\\b\\c";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "a\\b\\c");
}

HWTEST_F(DownloaderUnitTest, ReplaceCharacters_05, TestSize.Level1)
{
    string input = "a\\b.c";
    string output = ReplaceCharacters(input);
    EXPECT_EQ(output, "a\\b\\.c");
}

HWTEST_F(DownloaderUnitTest, IsMatch_01, TestSize.Level1)
{
    string str = "test";
    string patternStr = "";
    bool result = IsMatch(str, patternStr);
    EXPECT_FALSE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_02, TestSize.Level1)
{
    string str = "test";
    string patternStr = "*";
    bool result = IsMatch(str, patternStr);
    EXPECT_TRUE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_03, TestSize.Level1)
{
    string str = "test";
    string patternStr = "test";
    bool result = IsMatch(str, patternStr);
    EXPECT_TRUE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_04, TestSize.Level1)
{
    string str = "test";
    string patternStr = "t.*";
    bool result = IsMatch(str, patternStr);
    EXPECT_FALSE(result);
}

HWTEST_F(DownloaderUnitTest, IsMatch_05, TestSize.Level1)
{
    string str = "test";
    string patternStr = "e.*";
    bool result = IsMatch(str, patternStr);
    EXPECT_FALSE(result);
}

HWTEST_F(DownloaderUnitTest, IsExcluded_01, TestSize.Level1)
{
    string str = "test";
    string exclusions = "";
    string split = ",";
    EXPECT_FALSE(IsExcluded(str, exclusions, split));
}

HWTEST_F(DownloaderUnitTest, IsExcluded_02, TestSize.Level1)
{
    string str = "test";
    string exclusions = "test,example";
    string split = ",";
    EXPECT_TRUE(IsExcluded(str, exclusions, split));
}

HWTEST_F(DownloaderUnitTest, IsExcluded_03, TestSize.Level1)
{
    string str = "test";
    string exclusions = "example,sample";
    string split = ",";
    EXPECT_FALSE(IsExcluded(str, exclusions, split));
}

HWTEST_F(DownloaderUnitTest, IsExcluded_04, TestSize.Level1)
{
    string str = "test";
    string exclusions = "example";
    string split = ",";
    EXPECT_FALSE(IsExcluded(str, exclusions, split));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_01, TestSize.Level1)
{
    string str = "  ++123";
    int32_t value;
    EXPECT_FALSE(StringUtil::SafeStoInt32(str, value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_02, TestSize.Level1)
{
    string str = "  0  ";
    int32_t value;
    EXPECT_TRUE(StringUtil::SafeStoInt32(str, value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_03, TestSize.Level1)
{
    string str = "ABC123";
    int32_t value;
    EXPECT_FALSE(StringUtil::SafeStoInt32(str, value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_04, TestSize.Level1)
{
    string str = "4294967295";
    int32_t value;
    EXPECT_FALSE(StringUtil::SafeStoInt32(str, value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_05, TestSize.Level1)
{
    string str = "123";
    int32_t value;
    EXPECT_TRUE(StringUtil::SafeStoInt32(str, value));
    EXPECT_EQ(value, 123);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_06, TestSize.Level1)
{
    string str = "-123";
    int32_t value;
    EXPECT_TRUE(StringUtil::SafeStoInt32(str, value));
    EXPECT_EQ(value, -123);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_07, TestSize.Level1)
{
    string str = "123abc";
    int32_t value;
    EXPECT_TRUE(StringUtil::SafeStoInt32(str, value));
    EXPECT_EQ(value, 123);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_01, TestSize.Level1)
{
    string str = "-4294967295";
    int64_t value;
    EXPECT_TRUE(StringUtil::SafeStoInt64(str, value));
    EXPECT_EQ(value, -4294967295);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_02, TestSize.Level1)
{
    string str = "9223372036854775807";
    int64_t value;
    EXPECT_TRUE(StringUtil::SafeStoInt64(str, value));
    EXPECT_EQ(value, 9223372036854775807);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_03, TestSize.Level1)
{
    string str = "9223372036854775808";
    int64_t value;
    EXPECT_FALSE(StringUtil::SafeStoInt64(str, value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_04, TestSize.Level1)
{
    string str = "+";
    int64_t value;
    EXPECT_FALSE(StringUtil::SafeStoInt64(str, value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_05, TestSize.Level1)
{
    string str = "--123";
    int64_t value;
    EXPECT_FALSE(StringUtil::SafeStoInt64(str, value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_06, TestSize.Level1)
{
    string str = "";
    int64_t value;
    EXPECT_FALSE(StringUtil::SafeStoInt64(str, value));
    EXPECT_EQ(value, 0);
}

// SafeStoInt32 正常数字测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_NormalNumber, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("123", value));
    EXPECT_EQ(value, 123);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_Zero, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_TRUE(StringUtil::SafeStoInt32("0", value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_NegativeNumber, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("-456", value));
    EXPECT_EQ(value, -456);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_MaxInt32, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32(std::to_string(INT32_MAX), value));
    EXPECT_EQ(value, INT32_MAX);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_MinInt32, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32(std::to_string(INT32_MIN), value));
    EXPECT_EQ(value, INT32_MIN);
}

// SafeStoInt32 前导空格测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_LeadingSpaces, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("   123", value));
    EXPECT_EQ(value, 123);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_MultipleLeadingSpaces, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("     -456", value));
    EXPECT_EQ(value, -456);
}

// SafeStoInt32 前导加号测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_LeadingPlus, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("+789", value));
    EXPECT_EQ(value, 789);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_SpaceAndPlus, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("  +789", value));
    EXPECT_EQ(value, 789);
}

// SafeStoInt32 错误输入测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_EmptyString, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_OnlySpaces, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("   ", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_DoublePlus, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("++123", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_PlusNotFollowedByDigit, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("+ abc", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_InvalidCharacters, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("abc123", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_OnlyPlus, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("+", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_OnlyMinus, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("-", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_DecimalPoint, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_TRUE(StringUtil::SafeStoInt32("123.45", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_LetterAfterNumber, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_TRUE(StringUtil::SafeStoInt32("123abc", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_SpaceAfterNumber, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_TRUE(StringUtil::SafeStoInt32("123 ", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_SpecialCharacters, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("@#$%", value));
}

// SafeStoInt32 边界测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_Overflow, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_FALSE(StringUtil::SafeStoInt32("2147483648", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_Underflow, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_FALSE(StringUtil::SafeStoInt32("-2147483649", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_VeryLargeNumber, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_FALSE(StringUtil::SafeStoInt32("999999999999999999999", value));
}

// SafeStoInt64 正常数字测试
HWTEST_F(DownloaderUnitTest, SafeStoInt64_NormalNumber, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64("1234567890", value));
    EXPECT_EQ(value, 1234567890);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_Zero, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_TRUE(StringUtil::SafeStoInt64("0", value));
    EXPECT_EQ(value, 0);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_NegativeNumber, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64("-9876543210", value));
    EXPECT_EQ(value, -9876543210);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_MaxInt64, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64(std::to_string(INT64_MAX), value));
    EXPECT_EQ(value, INT64_MAX);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_MinInt64, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64(std::to_string(INT64_MIN), value));
    EXPECT_EQ(value, INT64_MIN);
}

// SafeStoInt64 前导空格测试
HWTEST_F(DownloaderUnitTest, SafeStoInt64_LeadingSpaces, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64("   1234567890", value));
    EXPECT_EQ(value, 1234567890);
}

// SafeStoInt64 前导加号测试
HWTEST_F(DownloaderUnitTest, SafeStoInt64_LeadingPlus, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64("+1234567890", value));
    EXPECT_EQ(value, 1234567890);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_SpaceAndPlus, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64("  +9876543210", value));
    EXPECT_EQ(value, 9876543210);
}

// SafeStoInt64 错误输入测试
HWTEST_F(DownloaderUnitTest, SafeStoInt64_EmptyString, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt64("", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_OnlySpaces, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt64("   ", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_DoublePlus, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt64("++123", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_InvalidCharacters, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt64("abc123", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_DecimalPoint, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_TRUE(StringUtil::SafeStoInt64("123.45", value));
}

// SafeStoInt64 边界测试
HWTEST_F(DownloaderUnitTest, SafeStoInt64_Overflow, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_FALSE(StringUtil::SafeStoInt64("9223372036854775808", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_Underflow, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_FALSE(StringUtil::SafeStoInt64("-9223372036854775809", value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_VeryLargeNumber, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_FALSE(StringUtil::SafeStoInt64("999999999999999999999999999999", value));
}

// 混合场景测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_ValueNotModifiedOnFailure, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("invalid", value));
    EXPECT_EQ(value, 999);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_ValueNotModifiedOnFailure, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt64("invalid", value));
    EXPECT_EQ(value, 999);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt32_LongString, TestSize.Level1)
{
    int32_t value = 0;
    std::string longStr(1000, '1');
    EXPECT_FALSE(StringUtil::SafeStoInt32(longStr, value));
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_LongString, TestSize.Level1)
{
    int64_t value = 0;
    std::string longStr(1000, '1');
    EXPECT_FALSE(StringUtil::SafeStoInt64(longStr, value));
}

// 补充：多次空格循环测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_MultipleSpacesWithPlus, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("     +123", value));
    EXPECT_EQ(value, 123);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_MultipleSpacesWithPlus, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64("     +9876543210", value));
    EXPECT_EQ(value, 9876543210);
}

// 补充：加号后大于'9'的字符测试
HWTEST_F(DownloaderUnitTest, SafeStoInt32_PlusFollowedByGreaterThanNine, TestSize.Level1)
{
    int32_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt32("+:", value));  // ':' ASCII 58 > '9' (57)
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_PlusFollowedByGreaterThanNine, TestSize.Level1)
{
    int64_t value = 999;
    EXPECT_FALSE(StringUtil::SafeStoInt64("+@", value));  // '@' ASCII 64 > '9' (57)
}

// 补充：多个空格后跟负号
HWTEST_F(DownloaderUnitTest, SafeStoInt32_MultipleSpacesWithNegative, TestSize.Level1)
{
    int32_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt32("     -456", value));
    EXPECT_EQ(value, -456);
}

HWTEST_F(DownloaderUnitTest, SafeStoInt64_MultipleSpacesWithNegative, TestSize.Level1)
{
    int64_t value = 0;
    EXPECT_TRUE(StringUtil::SafeStoInt64("     -9876543210", value));
    EXPECT_EQ(value, -9876543210);
}

HWTEST_F(DownloaderUnitTest, HandleRetErrorCode001, TestSize.Level1)
{
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->currentRequest_->serverError_ = 500;
    downloader->HandleRetErrorCode();
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_EQ(downloader->currentRequest_->serverError_, 500);
}
HWTEST_F(DownloaderUnitTest, IsChunkedInterrupt, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    bool isInterruptNeeded = true;
    EXPECT_EQ(Request_->IsChunked(isInterruptNeeded), Seekable::INVALID);
}

HWTEST_F(DownloaderUnitTest, IsChunkedIsChunk, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    bool isInterruptNeeded = false;
    Request_->headerInfo_.isChunked = true;
    Request_->headerInfo_.fileContentLen = LIVE_CONTENT_LENGTH;
    EXPECT_EQ(Request_->IsChunked(isInterruptNeeded), Seekable::SEEKABLE);
}

HWTEST_F(DownloaderUnitTest, GetBitRateError01, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    Request_->downloadDoneTime_ = 0;
    EXPECT_EQ(Request_->GetBitRate(), 0);
}

HWTEST_F(DownloaderUnitTest, GetBitRateError02, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    Request_->downloadStartTime_ = 1;
    Request_->downloadDoneTime_ = 1;
    Request_->realRecvContentLen_ = 1;
    EXPECT_EQ(Request_->GetBitRate(), 0);
}

HWTEST_F(DownloaderUnitTest, IsM3u8Request, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    Request_->protocolType_ = RequestProtocolType::HLS;
    EXPECT_TRUE(Request_->IsM3u8Request());
}

HWTEST_F(DownloaderUnitTest, IsServerAcceptRange, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    Request_->headerInfo_.isChunked = true;
    EXPECT_FALSE(Request_->IsServerAcceptRange());
}

HWTEST_F(DownloaderUnitTest, DownloadInterrupt, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    Request_->isInterruptNeeded_ = true;
    int32_t waitMs = 0;
    downloader->Download(Request_, waitMs);
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_TRUE(Request_->isInterruptNeeded_);
}

HWTEST_F(DownloaderUnitTest, Download002, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    Request_->isInterruptNeeded_ = false;
    int32_t waitMs = -1;
    downloader->Download(Request_, waitMs);
    downloader->Pause();
    OSAL::SleepFor(1 * 1000);
    Request_->requestSize_ = Request_->GetFileContentLength() + 1;
    Request_->startPos_ = Request_->GetFileContentLength() - 1;
    downloader->Resume();
    Request_->retryTimes_ = 1;
    downloader->Cancel();

    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_FALSE(downloader->isCancelling_.load());
}

HWTEST_F(DownloaderUnitTest, Seek001, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    downloader->downloadRequestSize_ = 1;
    Request_->retryTimes_ = 1;
    EXPECT_FALSE(downloader->Seek(1));
}

HWTEST_F(DownloaderUnitTest, HttpDownloadLoop001, TestSize.Level1)
{
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);

    downloader->shouldStartNextRequest_.store(true);
    downloader->requestQue_->Push(Request_, static_cast<int>(-1));
    downloader->isInterruptNeeded_ = true;
    EXPECT_NE(downloader->client_, nullptr);
    EXPECT_TRUE(downloader->isInterruptNeeded_);
}

class SourceLoader : public IMediaSourceLoader {
public:
    ~SourceLoader() {};

    int32_t Init(std::shared_ptr<IMediaSourceLoadingRequest> &request)
    {
        return 1;
    };
    
    int64_t Open(const std::string &url, const std::map<std::string, std::string> &header)
    {
        return 1;
    };

    int32_t Read(int64_t uuid, int64_t requestedOffset, int64_t requestedLength)
    {
        return 1;
    };

    int32_t Close(int64_t uuid)
    {
        return 1;
    };
};

HWTEST_F(DownloaderUnitTest, OPEN_APP_URI_001, TestSize.Level1)
{
    std::string testPath = "http://127.0.0.1:46666/test_cbr/720_1M/video_720.m3u8";
    std::shared_ptr<SourceLoader> loader = std::make_shared<SourceLoader>();
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader =
        std::make_shared<MediaSourceLoaderCombinations>(loader);
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<Downloader> downloader1 = std::make_shared<Downloader>("test", sourceLoader);
    downloader1->Init();
    downloader1->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader1->appPreviousRequestUrl_ = testPath;
    EXPECT_NE(downloader1->client_, nullptr);
    EXPECT_NE(downloader1->sourceLoader_, nullptr);
    EXPECT_NE(downloader1->currentRequest_, nullptr);
    EXPECT_EQ(downloader1->sourceId_, 0);
    downloader1->OpenAppUri();
    ASSERT_EQ(downloader1->sourceId_, static_cast<int64_t>(1));
    downloader1->sourceLoader_ = nullptr;
    downloader1->OpenAppUri();
    ASSERT_EQ(downloader1->sourceId_, static_cast<int64_t>(1));
}

HWTEST_F(DownloaderUnitTest, DROP_RETRY_DATA_001, TestSize.Level1)
{
    std::string testPath = "http://127.0.0.1:46666/test_cbr/720_1M/video_720.m3u8";
    std::shared_ptr<SourceLoader> loader = std::make_shared<SourceLoader>();
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader =
        std::make_shared<MediaSourceLoaderCombinations>(loader);
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
                                  std::shared_ptr<DownloadRequest>& request) {
    };
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<Downloader> downloader1 = std::make_shared<Downloader>("test", sourceLoader);
    downloader1->Init();
    downloader1->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader1->appPreviousRequestUrl_ = testPath;
    downloader1->OpenAppUri();
    EXPECT_NE(downloader1->client_, nullptr);
    EXPECT_NE(downloader1->sourceLoader_, nullptr);
    EXPECT_NE(downloader1->currentRequest_, nullptr);
    uint8_t * buffer = new uint8_t[1000];

    downloader1->currentRequest_->startPos_ = 0;
    downloader1->DropRetryData(buffer, 1000, downloader1.get());
    
    downloader1->currentRequest_->startPos_ = 1000;
    EXPECT_EQ(downloader1->DropRetryData(buffer, 500, downloader1.get()), 500);
    EXPECT_EQ(downloader1->DropRetryData(buffer, 500, downloader1.get()), 500);
}

class SourceCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {}
};

HWTEST_F(DownloaderUnitTest, DOWNLOADER_MONITOR_001, TestSize.Level1)
{
    std::shared_ptr<DownloadMonitor> downloader = std::make_shared<DownloadMonitor>
        (std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>("http", 100, nullptr)));
    downloader->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->callback_ = sourceCallback;
    NetworkErrorInfo networkErrorInfo;
    networkErrorInfo.clientErrorCode_ = 52;
    networkErrorInfo.serverErrorCode_ = 403;
    downloader->NotifyError(networkErrorInfo);
    std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>("", nullptr, nullptr,  false);
    request->clientError_ = 992;
    EXPECT_EQ(downloader->NeedRetry(request), false);
    downloader->GetDownloadInfo();
    downloader->GetBufferSize();
    downloader->GetReadTimeOut(true);
    downloader->GetSegmentOffset();
    downloader->GetHLSDiscontinuity();
    downloader->StopBufferring(true);
    int32_t serverCode = 0;
    uint32_t bitRate = 2000;
    downloader->GetServerMediaServiceErrorCode(400, serverCode);
    downloader->GetServerMediaServiceErrorCode(101, serverCode);
    downloader->GetCachedDuration();
    downloader->RestartAndClearBuffer();
    downloader->IsFlvLive();
    downloader->IsHlsFmp4();
    downloader->GetContentType();
    downloader->AutoSelectBitRate(bitRate);
    downloader->SetStartPts(1);
    downloader->SetExtraCache(1);
    downloader->GetMemorySize();
    downloader->SetCurrentBitRate(1, 1);
    downloader->SetPlayStrategy(nullptr);
    DownloadInfo downloadInfo;
    downloader->GetDownloadInfo(downloadInfo);
    PlaybackInfo playbackInfo;
    downloader->GetPlaybackInfo(playbackInfo);
    downloader->SetAppUid(1);
    downloader->GetPlayable();
    EXPECT_NE(downloader->downloader_, nullptr);
}

HWTEST_F(DownloaderUnitTest, DOWNLOADER_MONITOR_002, TestSize.Level1)
{
    std::shared_ptr<DownloadMonitor> downloader = std::make_shared<DownloadMonitor>
        (std::make_shared<DownloadMonitor>(std::make_shared<HttpMediaDownloader>("http", 100, nullptr)));
    downloader->Init();
    auto sourceCallback = std::make_shared<SourceCallback>();
    downloader->downloader_ = nullptr;
    downloader->callback_ = sourceCallback;
    NetworkErrorInfo networkErrorInfo;
    networkErrorInfo.clientErrorCode_ = 52;
    networkErrorInfo.serverErrorCode_ = 403;
    downloader->NotifyError(networkErrorInfo);
    std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>("", nullptr, nullptr,  false);
    request->clientError_ = 992;
    EXPECT_EQ(downloader->NeedRetry(request), false);
    downloader->GetDownloadInfo();
    downloader->GetBufferSize();
    downloader->GetReadTimeOut(true);
    downloader->GetSegmentOffset();
    downloader->GetHLSDiscontinuity();
    downloader->StopBufferring(true);
    int32_t serverCode = 0;
    downloader->GetServerMediaServiceErrorCode(400, serverCode);
    downloader->GetServerMediaServiceErrorCode(101, serverCode);
    downloader->GetCachedDuration();
    downloader->RestartAndClearBuffer();
    downloader->IsFlvLive();
    downloader->SetStartPts(1);
    downloader->SetExtraCache(1);
    downloader->GetMemorySize();
    downloader->SetCurrentBitRate(1, 1);
    downloader->SetPlayStrategy(nullptr);
    DownloadInfo downloadInfo;
    downloader->GetDownloadInfo(downloadInfo);
    PlaybackInfo playbackInfo;
    downloader->GetPlaybackInfo(playbackInfo);
    downloader->SetAppUid(1);
    downloader->GetPlayable();
    EXPECT_EQ(downloader->downloader_, nullptr);
    downloader->callback_.reset();
    networkErrorInfo.clientErrorCode_ = 52;
    networkErrorInfo.serverErrorCode_ = 403;
    downloader->NotifyError(networkErrorInfo);
    networkErrorInfo.serverErrorCode_ = 0;
    downloader->NotifyError(networkErrorInfo);
    networkErrorInfo.clientErrorCode_ = 0;
    downloader->NotifyError(networkErrorInfo);
    EXPECT_TRUE(downloader->callback_.expired());
}

void ThreadFinishLoading(std::shared_ptr<NetworkClient> client, int64_t uuid, LoadingRequestError error)
{
    client->FinishLoading(uuid, error);
}

void ThreadRequestData(std::shared_ptr<NetworkClient> client, long startPos, int len, const RequestInfo& requestInfo,
    HandleResponseCbFunc completeCb, int64_t uuid, LoadingRequestError error)
{
    std::thread threadFinishLoading(ThreadFinishLoading, client, uuid, error);
    client->RequestData(startPos, len, requestInfo, completeCb);

    if (threadFinishLoading.joinable()) {
        threadFinishLoading.join();
    }
}

HWTEST_F(DownloaderUnitTest, APP_CLIENT_001, TestSize.Level1)
{
    std::string testPath = "http://127.0.0.1:46666/test_cbr/710_1M/video_720.m3u8";
    std::shared_ptr<SourceLoader> loader = std::make_shared<SourceLoader>();
    std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader =
        std::make_shared<MediaSourceLoaderCombinations>(loader);
    std::map<std::string, std::string> httpHeader;
    RequestInfo requestInfo;
    requestInfo.url = "http";
    requestInfo.httpHeader = httpHeader;
    auto realStatusCallback = [this] (DownloadStatus&& status, std::shared_ptr<Downloader>& downloader,
        std::shared_ptr<DownloadRequest>& request) {};
    auto saveData = [this] (uint8_t*&& data, uint32_t&& len, bool notblock) {
        return len;
    };
    std::shared_ptr<Downloader> downloader2 = std::make_shared<Downloader>("test", sourceLoader);
    downloader2->Init();
    downloader2->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader2->appPreviousRequestUrl_ = testPath;
    downloader2->OpenAppUri();
    EXPECT_NE(downloader2->client_, nullptr);
    EXPECT_NE(downloader2->sourceId_, static_cast<int64_t>(0));

    uint32_t appUid = 3588;
    int64_t uuid = 1;
    std::string ipAddress = "address";
    httpHeader["Content-Type"] = "application/json";
    httpHeader["Authorization"] = "Bearer your_token_here";
    downloader2->client_->RespondHeader(uuid, httpHeader, " ");
    downloader2->client_->SetUuid(uuid);
    downloader2->client_->SetAppUid(appUid);
    downloader2->client_->GetIp(ipAddress);
    downloader2->client_->GetRedirectUrl();
    for (int i = 0; i < 101; i++) { httpHeader["key" + std::to_string(i)] = "value" + std::to_string(i); }
    downloader2->client_->RespondHeader(uuid, httpHeader, " ");
    std::vector<LoadingRequestError> errors = {
        LoadingRequestError::LOADING_ERROR_SUCCESS,
        LoadingRequestError::LOADING_ERROR_NOT_READY,
    };

    for (const auto& error : errors) {
        std::thread threadRequestData(ThreadRequestData, downloader2->client_, 31819, 4096, requestInfo,
            [](int32_t clientCode, int32_t serverCode, const std::string &ca, Status status) {}, uuid, error);
        if (threadRequestData.joinable()) {
            threadRequestData.join();
        }
    }
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_EmptyString
 * @tc.desc  : Test empty input returns empty string (B1)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_EmptyString, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl(""), "");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_NoScheme
 * @tc.desc  : Test URL without scheme separator returns unchanged (B2)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_NoScheme, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("no-scheme"), "no-scheme");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_HostOnly
 * @tc.desc  : Test URL with no path returns scheme://hostPort (B3)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_HostOnly, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com"), "https://example.com");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_RootPath
 * @tc.desc  : Test root path "/" is stripped, returns scheme://hostPort (B4)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_RootPath, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/"), "https://example.com");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_DirectoryPath
 * @tc.desc  : Test directory path ending with "/" is stripped (B5)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_DirectoryPath, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/a/b/"), "https://example.com");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_MultiSegmentFilePath
 * @tc.desc  : Test multi-segment file path: hidden segments masked with *** (B6)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_MultiSegmentFilePath, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/dir/file"), "https://example.com/***/file");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_SingleSegmentFilePath
 * @tc.desc  : Test single-segment file path: no mask needed (B7)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_SingleSegmentFilePath, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/file.m3u8"), "https://example.com/file.m3u8");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_UserInfoStripped
 * @tc.desc  : Test userinfo (user:pass@) is stripped (B8)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_UserInfoStripped, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("http://user:pass@host/dir/file"), "http://host/***/file");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_QueryStripped
 * @tc.desc  : Test query parameters are stripped (B9)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_QueryStripped, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/a/b/file?q=1"), "https://example.com/***/file");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_FragmentStripped
 * @tc.desc  : Test fragment is stripped (B10)
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_FragmentStripped, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/a/b/file#frag"), "https://example.com/***/file");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_DirWithQueryStripped
 * @tc.desc  : Test directory path with query is stripped entirely
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_DirWithQueryStripped, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/a/b/?q=1"), "https://example.com");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_RootWithQueryStripped
 * @tc.desc  : Test root path with query is stripped entirely
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_RootWithQueryStripped, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/?q=1"), "https://example.com");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_PortPreserved
 * @tc.desc  : Test port number is preserved in output
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_PortPreserved, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("http://host:8080/dir/file"), "http://host:8080/***/file");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_UserInfoAndPort
 * @tc.desc  : Test userinfo stripped and port preserved together
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_UserInfoAndPort, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("http://user:pass@host:8080/dir/file"), "http://host:8080/***/file");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_IPv6Host
 * @tc.desc  : Test IPv6 host address is preserved
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_IPv6Host, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("http://[::1]:8080/dir/file"), "http://[::1]:8080/***/file");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_DeepPath
 * @tc.desc  : Test deeply nested path shows only last segment
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_DeepPath, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/a/b/c/d/e/f/v.mp4"),
              "https://example.com/***/v.mp4");
}

/**
 * @tc.name  : Test MaskUrl API
 * @tc.number: MaskUrl_QueryAndFragmentStripped
 * @tc.desc  : Test both query and fragment are stripped
 */
HWTEST_F(DownloaderUnitTest, MaskUrl_QueryAndFragmentStripped, TestSize.Level1)
{
    EXPECT_EQ(StringUtil::MaskUrl("https://example.com/v/s.mp4?q=1#f"),
              "https://example.com/***/s.mp4");
}

class MockCloseRetryClient : public NetworkClient {
public:
    std::atomic<int> closeCallCount_{0};
    int closeFailTimes_{0};

    MockCloseRetryClient() {}

    Status Close(bool isAsync) override
    {
        closeCallCount_++;
        if (closeCallCount_ <= closeFailTimes_) {
            return Status::ERROR_CLIENT;
        }
        return Status::OK;
    }
    Status Init() override { return Status::OK; }
    Status Open(const std::string&, const std::map<std::string, std::string>&, int32_t) override { return Status::OK; }
    Status RequestData(long, int, const RequestInfo&, HandleResponseCbFunc) override { return Status::OK; }
    Status Deinit() override { return Status::OK; }
    Status GetIp(std::string&) override { return Status::OK; }
    void SetAppUid(int32_t) override {}
};

HWTEST_F(DownloaderUnitTest, Pause_CloseRetry_Succeed, TestSize.Level0)
{
    auto mock = std::make_shared<MockCloseRetryClient>();
    mock->closeFailTimes_ = 2;
    downloader->client_ = mock;

    auto future = std::async(std::launch::async, [&]() { downloader->Pause(); });
    auto status = future.wait_for(std::chrono::seconds(5));
    EXPECT_NE(status, std::future_status::timeout);
    EXPECT_EQ(mock->closeCallCount_, 3);
}

HWTEST_F(DownloaderUnitTest, Pause_CloseRetry_AllFail, TestSize.Level0)
{
    auto mock = std::make_shared<MockCloseRetryClient>();
    mock->closeFailTimes_ = 100;
    downloader->client_ = mock;

    auto future = std::async(std::launch::async, [&]() { downloader->Pause(); });
    auto status = future.wait_for(std::chrono::seconds(30));
    EXPECT_NE(status, std::future_status::timeout);
    EXPECT_EQ(mock->closeCallCount_, 5);
}

HWTEST_F(DownloaderUnitTest, Pause_CloseNoRetry_Normal, TestSize.Level0)
{
    auto mock = std::make_shared<MockCloseRetryClient>();
    mock->closeFailTimes_ = 0;
    downloader->client_ = mock;

    downloader->Pause();
    EXPECT_EQ(mock->closeCallCount_, 1);
}

HWTEST_F(DownloaderUnitTest, HttpCurlClient_HeaderList_Free_In_dtor, TestSize.Level1)
{
    auto client = NetworkClient::GetInstance(
        [](void *buffer, size_t size, size_t nitems, void *userParam) ->
            size_t { (void)buffer; (void)userParam; return size * nitems; },
        [](void *buffer, size_t size, size_t nitems, void *userParam) ->
            size_t { (void)buffer; (void)userParam; return size * nitems; },
        nullptr);
    EXPECT_NE(client, nullptr);
    EXPECT_EQ(client->Init(), Status::OK);
    (void)client->Init();
    std::map<std::string, std::string> httpHeader = {{"Accpet", "*/*"}};
    (void)client->Open("http://127.0.0.1:46666/test", httpHeader, 1000);
    client.reset();
}

/**
 * @tc.name  : Test SafeStoDouble API
 * @tc.number: SafeStoDouble_001
 * @tc.desc  : Test SafeStoDouble with valid double string
 */
HWTEST_F(DownloaderUnitTest, SafeStoDouble_001, TestSize.Level1)
{
    double value = 0.0;
    EXPECT_TRUE(StringUtil::SafeStoDouble("3.14", value));
    EXPECT_DOUBLE_EQ(value, 3.14);
}

/**
 * @tc.name  : Test SafeStoDouble API
 * @tc.number: SafeStoDouble_002
 * @tc.desc  : Test SafeStoDouble with empty string returns false
 */
HWTEST_F(DownloaderUnitTest, SafeStoDouble_002, TestSize.Level1)
{
    double value = -1.0;
    bool result = StringUtil::SafeStoDouble("", value);
    EXPECT_FALSE(result);
    EXPECT_EQ(value, -1.0);
    result = StringUtil::SafeStoDouble("   ", value);
    EXPECT_FALSE(result);
    EXPECT_EQ(value, -1.0);
}

/**
 * @tc.name  : Test SafeStoDouble API
 * @tc.number: SafeStoDouble_003
 * @tc.desc  : Test SafeStoDouble with overflow string returns false
 */
HWTEST_F(DownloaderUnitTest, SafeStoDouble_003, TestSize.Level1)
{
    double value = -1.0;
    bool result = StringUtil::SafeStoDouble("1e999", value);
    EXPECT_FALSE(result);
    EXPECT_EQ(value, -1.0);
    result = StringUtil::SafeStoDouble("-1e999", value);
    EXPECT_FALSE(result);
    EXPECT_EQ(value, -1.0);
}

/**
 * @tc.name  : Test SafeStoDouble API
 * @tc.number: SafeStoDouble_004
 * @tc.desc  : Test SafeStoDouble with non-numeric string returns false
 */
HWTEST_F(DownloaderUnitTest, SafeStoDouble_004, TestSize.Level1)
{
    double value = -1.0;
    bool result = StringUtil::SafeStoDouble("abc", value);
    EXPECT_FALSE(result);
    EXPECT_EQ(value, -1.0);
    result = StringUtil::SafeStoDouble("!@#", value);
    EXPECT_FALSE(result);
    EXPECT_EQ(value, -1.0);
}

/**
 * @tc.name  : Test RetryCore null client
 * @tc.number: RetryCore_NullClient
 * @tc.desc  : Test RetryCore returns false when client_ is null
 */
HWTEST_F(DownloaderUnitTest, RetryCore_NullClient, TestSize.Level1)
{
    auto downloader = std::make_shared<Downloader>("test_retry_null");
    auto request = std::make_shared<DownloadRequest>("http://test.com",
        [](uint8_t*, uint32_t, bool) { return (uint32_t)0; },
        [](DownloadStatus, std::shared_ptr<Downloader>&, std::shared_ptr<DownloadRequest>&) {});
    downloader->client_ = nullptr;
    downloader->shouldStartNextRequest_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    bool result = downloader->RetryCore(request);
    EXPECT_FALSE(result);
}

/**
 * @tc.name  : Test RetryCore null currentRequest
 * @tc.number: RetryCore_NullCurrentRequest
 * @tc.desc  : Test RetryCore with null currentRequest_ skips inner block
 */
HWTEST_F(DownloaderUnitTest, RetryCore_NullCurrentRequest, TestSize.Level1)
{
    auto downloader = std::make_shared<Downloader>("test_retry_nullreq");
    auto request = std::make_shared<DownloadRequest>("http://test.com",
        [](uint8_t*, uint32_t, bool) { return (uint32_t)0; },
        [](DownloadStatus, std::shared_ptr<Downloader>&, std::shared_ptr<DownloadRequest>&) {});
    downloader->client_ = std::make_shared<AppClient>(
        [](void*, size_t, size_t, void*) -> size_t { return 0; },
        [](void*, size_t, size_t, void*) -> size_t { return 0; },
        nullptr);
    downloader->currentRequest_ = nullptr;
    downloader->shouldStartNextRequest_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    bool result = downloader->RetryCore(request);
    EXPECT_TRUE(result);
}

/**
 * @tc.name  : Test RetryCore range request size update
 * @tc.number: RetryCore_RangeSizeUpdate
 * @tc.desc  : Test RetryCore updates requestSize for valid range (endPos > 0, startPos >= 0)
 */
HWTEST_F(DownloaderUnitTest, RetryCore_RangeSizeUpdate, TestSize.Level1)
{
    auto downloader = std::make_shared<Downloader>("test_retry_range");
    auto request = std::make_shared<DownloadRequest>("http://test.com",
        [](uint8_t*, uint32_t, bool) { return (uint32_t)0; },
        [](DownloadStatus, std::shared_ptr<Downloader>&, std::shared_ptr<DownloadRequest>&) {});
    downloader->client_ = std::make_shared<AppClient>(
        [](void*, size_t, size_t, void*) -> size_t { return 0; },
        [](void*, size_t, size_t, void*) -> size_t { return 0; },
        nullptr);
    downloader->requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>("testQue");
    downloader->currentRequest_ = request;
    downloader->shouldStartNextRequest_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    downloader->currentRequest_->startPos_ = 100;
    downloader->currentRequest_->endPos_ = 200;
    downloader->currentRequest_->requestSize_ = 0;
    bool result = downloader->RetryCore(request);
    EXPECT_TRUE(result);
    EXPECT_EQ(downloader->currentRequest_->requestSize_, 101);
}

// Helper: create a DownloadRequest for testing
static std::shared_ptr<DownloadRequest> MakeTestRequest(const std::string& url = "http://test.com")
{
    auto saveData = [](uint8_t*, uint32_t len, bool) { return len; };
    auto statusCb = [](DownloadStatus, std::shared_ptr<Downloader>&, std::shared_ptr<DownloadRequest>&) {};
    RequestInfo ri;
    ri.url = url;
    return std::make_shared<DownloadRequest>(saveData, statusCb, ri);
}

// ---- DownloadRequest method coverage ----

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetBitRate_Valid, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->downloadStartTime_ = 1000;
    req->downloadDoneTime_ = 2000;
    req->realRecvContentLen_ = 8000;
    ASSERT_EQ(req->GetBitRate(), static_cast<uint32_t>(8000 * 1000 * 1 * 8 / 1000));
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_IsChunked_NotChunked, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->headerInfo_.isChunked = false;
    ASSERT_EQ(req->IsChunked(false), Seekable::SEEKABLE);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_IsChunked_ChunkedNotLive, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->headerInfo_.isChunked = true;
    req->headerInfo_.fileContentLen = 100;
    ASSERT_EQ(req->IsChunked(false), Seekable::UNSEEKABLE);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_IsChunkedVod_True, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->headerInfo_.isChunked = true;
    req->headerInfo_.fileContentLen = LIVE_CONTENT_LENGTH;
    ASSERT_TRUE(req->IsChunkedVod());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_IsChunkedVod_False, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->headerInfo_.isChunked = false;
    ASSERT_FALSE(req->IsChunkedVod());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_IsServerAcceptRange_NotChunked, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->headerInfo_.isChunked = false;
    req->headerInfo_.isServerAcceptRange = true;
    ASSERT_TRUE(req->IsServerAcceptRange());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_IsServerAcceptRange_NotChunkedFalse, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->headerInfo_.isChunked = false;
    req->headerInfo_.isServerAcceptRange = false;
    ASSERT_FALSE(req->IsServerAcceptRange());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetStartTimePos_Positive, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetStartTimePos(100);
    ASSERT_FALSE(req->shouldSaveData_.load());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetStartTimePos_Zero, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetStartTimePos(0);
    ASSERT_TRUE(req->shouldSaveData_.load());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetRangePos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetRangePos(10, 100);
    ASSERT_EQ(req->startPos_, 10);
    ASSERT_EQ(req->endPos_, 100);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetDuration, TestSize.Level1)
{
    auto saveData = [](uint8_t*, uint32_t len, bool) { return len; };
    auto statusCb = [](DownloadStatus, std::shared_ptr<Downloader>&, std::shared_ptr<DownloadRequest>&) {};
    auto req = std::make_shared<DownloadRequest>("http://test.com", 5.5, saveData, statusCb);
    ASSERT_EQ(req->GetDuration(), 5.5);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetDownloaderName, TestSize.Level1)
{
    auto dl = std::make_shared<Downloader>("myDownloader");
    dl->Init();
    dl->currentRequest_ = MakeTestRequest();
    dl->currentRequest_->downloaderName_ = "myDownloader";
    ASSERT_EQ(dl->currentRequest_->GetDownloaderName(), "myDownloader");
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetClientError, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->clientError_ = 404;
    ASSERT_EQ(req->GetClientError(), 404);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetServerError, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->serverError_ = 500;
    ASSERT_EQ(req->GetServerError(), 500);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_IsEos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->isEos_.store(true);
    ASSERT_TRUE(req->IsEos());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetRetryTimes, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->retryTimes_.store(3);
    ASSERT_EQ(req->GetRetryTimes(), 3);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_Close_IsClosed, TestSize.Level1)
{
    auto req = MakeTestRequest();
    ASSERT_FALSE(req->IsClosed());
    req->Close();
    ASSERT_TRUE(req->IsClosed());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetRequestType_GetRequestType, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetRequestType(RequestType::HLS_PLAYLIST);
    ASSERT_EQ(req->GetRequestType(), RequestType::HLS_PLAYLIST);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetLocation, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->location_ = "http://redirect.com";
    std::string loc;
    req->GetLocation(loc);
    ASSERT_EQ(loc, "http://redirect.com");
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetBitRateToRequestSize, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetBitRateToRequestSize(1024);
    ASSERT_EQ(req->bitRateToRequestSize_, 256);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_GetFileContentLengthNoWait, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->headerInfo_.fileContentLen = 12345;
    ASSERT_EQ(req->GetFileContentLengthNoWait(), 12345);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetRequestProtocolType, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetRequestProtocolType(RequestProtocolType::HLS);
    ASSERT_TRUE(req->IsM3u8Request());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetUrl, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetUrl("http://newurl.com");
    ASSERT_EQ(req->GetUrl(), "http://newurl.com");
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetIsIndexM3u8Request, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->SetIsIndexM3u8Request(true);
    ASSERT_TRUE(req->IsIndexM3u8Request());
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SetDownloadDoneCb, TestSize.Level1)
{
    auto req = MakeTestRequest();
    bool called = false;
    req->SetDownloadDoneCb([&called](const std::string& url, const std::string& loc) { called = true; });
    req->downloadDoneCallback_("url", "loc");
    ASSERT_TRUE(called);
}

HWTEST_F(DownloaderUnitTest, DownloadRequest_SaveHeader, TestSize.Level1)
{
    auto req = MakeTestRequest();
    HeaderInfo info;
    info.fileContentLen = 9999;
    info.contentLen = 8888;
    info.isChunked = true;
    strncpy(info.contentType, "video/mp4", sizeof(info.contentType) - 1);
    req->SaveHeader(&info);
    ASSERT_TRUE(req->isHeaderUpdated_.load());
    ASSERT_EQ(req->headerInfo_.fileContentLen, 9999);
}

// ---- Downloader method coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_GetContentType_Destructor, TestSize.Level1)
{
    downloader->isDestructor_ = true;
    ASSERT_EQ(downloader->GetContentType(), downloader->contentType_);
}

HWTEST_F(DownloaderUnitTest, Downloader_GetContentType_AlreadyUpdated, TestSize.Level1)
{
    downloader->isContentTypeUpdated_.store(true);
    downloader->contentType_ = "video/mp4";
    ASSERT_EQ(downloader->GetContentType(), "video/mp4");
}

HWTEST_F(DownloaderUnitTest, Downloader_GetIp_WithClient, TestSize.Level1)
{
    std::string ip;
    downloader->GetIp(ip);
    ASSERT_EQ(downloader->name_, std::string("test"));
}

HWTEST_F(DownloaderUnitTest, Downloader_SetAppUid, TestSize.Level1)
{
    downloader->SetAppUid(12345);
    ASSERT_EQ(downloader->name_, std::string("test"));
}

HWTEST_F(DownloaderUnitTest, Downloader_GetCurrentRequest, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    ASSERT_EQ(downloader->GetCurrentRequest(), req);
}

HWTEST_F(DownloaderUnitTest, Downloader_SetInterruptState_Interrupt, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->SetInterruptState(true);
    ASSERT_TRUE(downloader->isInterruptNeeded_.load());
    ASSERT_TRUE(req->isInterruptNeeded_);
}

HWTEST_F(DownloaderUnitTest, Downloader_SetInterruptState_NoInterrupt, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->SetInterruptState(false);
    ASSERT_FALSE(downloader->isInterruptNeeded_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_SetAppState, TestSize.Level1)
{
    downloader->SetAppState(true);
    ASSERT_TRUE(downloader->isAppBackground_.load());
    downloader->SetAppState(false);
    ASSERT_FALSE(downloader->isAppBackground_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_GetCertIssuerNames, TestSize.Level1)
{
    downloader->ca_ = "TestCA";
    ASSERT_EQ(downloader->GetCertIssuerNames(), "TestCA");
}

HWTEST_F(DownloaderUnitTest, Downloader_SetRequestSize, TestSize.Level1)
{
    downloader->SetRequestSize(4096);
    ASSERT_EQ(downloader->downloadRequestSize_, static_cast<size_t>(4096));
}

HWTEST_F(DownloaderUnitTest, Downloader_ReStart, TestSize.Level1)
{
    downloader->shouldStartNextRequest_.store(false);
    downloader->ReStart();
    ASSERT_TRUE(downloader->shouldStartNextRequest_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_PauseLoop_NullTask, TestSize.Level1)
{
    downloader->task_ = nullptr;
    downloader->PauseLoop(true);
    ASSERT_EQ(downloader->loopStatus_.load(), Downloader::LoopStatus::IDLE);
}

HWTEST_F(DownloaderUnitTest, Downloader_PauseLoop_Sync, TestSize.Level1)
{
    downloader->PauseLoop(false);
    ASSERT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, Downloader_PauseLoop_Async, TestSize.Level1)
{
    downloader->PauseLoop(true);
    ASSERT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, Downloader_Download_Cancelled, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->isCancelling_.store(true);
    ASSERT_FALSE(downloader->Download(req, 100));
}

HWTEST_F(DownloaderUnitTest, Downloader_Download_WithTimeout, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->isCancelling_.store(false);
    downloader->Download(req, 100);
    ASSERT_EQ(downloader->isCancelling_.load(), false);
}

HWTEST_F(DownloaderUnitTest, Downloader_Start_Cancelled, TestSize.Level1)
{
    downloader->isCancelling_.store(true);
    downloader->Start();
    ASSERT_TRUE(downloader->isCancelling_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_Seek_NormalOffset, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->isHeaderUpdated_.store(true);
    ASSERT_TRUE(downloader->Seek(100));
    ASSERT_EQ(req->startPos_, 100);
}

HWTEST_F(DownloaderUnitTest, Downloader_Seek_OffsetOutOfRange, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 1000;
    req->isHeaderUpdated_.store(true);
    ASSERT_TRUE(downloader->Seek(2000));
    ASSERT_EQ(req->startPos_, 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_Seek_NegativeOffset, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 1000;
    req->isHeaderUpdated_.store(true);
    ASSERT_TRUE(downloader->Seek(-1));
    ASSERT_EQ(req->startPos_, 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_Seek_WithDownloadRequestSize, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->isHeaderUpdated_.store(true);
    downloader->downloadRequestSize_ = 500;
    ASSERT_TRUE(downloader->Seek(100));
    ASSERT_EQ(req->requestSize_, 500);
    ASSERT_EQ(downloader->downloadRequestSize_, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_Seek_WithRetryTimes, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->isHeaderUpdated_.store(true);
    req->retryTimes_.store(5);
    ASSERT_TRUE(downloader->Seek(100));
    ASSERT_EQ(req->retryTimes_.load(), 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_Seek_NullRequest, TestSize.Level1)
{
    downloader->currentRequest_ = nullptr;
    ASSERT_FALSE(downloader->Seek(100));
}

HWTEST_F(DownloaderUnitTest, Downloader_Seek_Interrupted, TestSize.Level1)
{
    downloader->isInterruptNeeded_ = true;
    ASSERT_FALSE(downloader->Seek(100));
}

HWTEST_F(DownloaderUnitTest, Downloader_RetryCore_NullClient, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->client_ = nullptr;
    downloader->shouldStartNextRequest_ = false;
    ASSERT_FALSE(downloader->RetryCore(req));
}

HWTEST_F(DownloaderUnitTest, Downloader_Retry_Interrupted, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->isInterruptNeeded_ = true;
    ASSERT_FALSE(downloader->Retry(req));
}

HWTEST_F(DownloaderUnitTest, Downloader_Retry_AppBackground, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->isAppBackground_ = true;
    downloader->shouldStartNextRequest_ = false;
    ASSERT_TRUE(downloader->Retry(req));
}

HWTEST_F(DownloaderUnitTest, Downloader_BeginDownload_EmptyUrl, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "";
    downloader->currentRequest_ = req;
    ASSERT_FALSE(downloader->BeginDownload());
}

HWTEST_F(DownloaderUnitTest, Downloader_BeginDownload_WithEndPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://test.com";
    req->endPos_ = 1000;
    req->startPos_ = 100;
    downloader->currentRequest_ = req;
    ASSERT_TRUE(downloader->BeginDownload());
    ASSERT_EQ(req->requestSize_, 901);
}

HWTEST_F(DownloaderUnitTest, Downloader_BeginDownload_NoEndPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://test.com";
    req->endPos_ = -1;
    downloader->currentRequest_ = req;
    ASSERT_TRUE(downloader->BeginDownload());
    ASSERT_EQ(req->requestSize_, 8192);
}

HWTEST_F(DownloaderUnitTest, Downloader_BeginDownload_WithUserAgent, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://test.com";
    req->httpHeader_["User-Agent"] = "MyAgent";
    downloader->currentRequest_ = req;
    ASSERT_TRUE(downloader->BeginDownload());
    ASSERT_EQ(req->httpHeader_.count("User-Agent"), static_cast<size_t>(1));
}

HWTEST_F(DownloaderUnitTest, Downloader_BeginDownload_NoUserAgent, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://test.com";
    downloader->currentRequest_ = req;
    ASSERT_TRUE(downloader->BeginDownload());
    ASSERT_GT(req->httpHeader_.count("User-Agent"), static_cast<size_t>(0));
}

// ---- HandleRetOK coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_ChunkedEos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.isChunked = true;
    req->headerInfo_.fileContentLen = 100;
    req->isHeaderUpdated_.store(true);
    downloader->HandleRetOK();
    ASSERT_TRUE(req->isEos_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_ReachEnd, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 100;
    req->startPos_ = 200;
    req->isHeaderUpdated_.store(true);
    req->downloadDoneCallback_ = [](const std::string&, const std::string&) {};
    downloader->HandleRetOK();
    ASSERT_TRUE(req->isEos_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_ZeroContentLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 0;
    req->startPos_ = 200;
    req->endPos_ = 100;
    req->isHeaderUpdated_.store(true);
    req->downloadDoneCallback_ = [](const std::string&, const std::string&) {};
    downloader->HandleRetOK();
    ASSERT_TRUE(req->isEos_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_Continue, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 100;
    req->isHeaderUpdated_.store(true);
    req->retryTimes_.store(2);
    downloader->HandleRetOK();
    ASSERT_EQ(req->retryTimes_.load(), 0);
    ASSERT_EQ(req->requestSize_, 9900);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_WithEndPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 100;
    req->endPos_ = 500;
    req->isHeaderUpdated_.store(true);
    downloader->HandleRetOK();
    ASSERT_EQ(req->requestSize_, 401);
}

// ---- HandleRetErrorCode coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetErrorCode_NotOften, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->serverError_ = 404;
    downloader->HandleRetErrorCode();
    ASSERT_EQ(req->serverError_, 404);
}

// ---- HandlePlayingFinish coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandlePlayingFinish_WithCallback, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    bool called = false;
    req->downloadDoneCallback_ = [&called](const std::string& url, const std::string& loc) {
        called = true;
        ASSERT_EQ(url, "http://test.com");
    };
    downloader->HandlePlayingFinish();
    ASSERT_TRUE(called);
    ASSERT_FALSE(req->isFirstRangeRequestReady_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_HandlePlayingFinish_NoCallback, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isDestructor_ = true;
    downloader->HandlePlayingFinish();
    ASSERT_TRUE(downloader->shouldStartNextRequest_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_HandlePlayingFinish_QueueNotEmpty, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    auto req2 = MakeTestRequest("http://test2.com");
    downloader->requestQue_->Push(req2, 1000);
    bool called = false;
    req->downloadDoneCallback_ = [&called](const std::string&, const std::string&) { called = true; };
    downloader->HandlePlayingFinish();
    ASSERT_TRUE(called);
}

// ---- HandleFileContentLen coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleFileContentLen_WithContentLen, TestSize.Level1)
{
    HeaderInfo info;
    info.fileContentLen = 0;
    info.contentLen = 5000;
    downloader->HandleFileContentLen(&info);
    ASSERT_EQ(info.fileContentLen, 5000);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleFileContentLen_BothZero, TestSize.Level1)
{
    HeaderInfo info;
    info.fileContentLen = 0;
    info.contentLen = 0;
    downloader->HandleFileContentLen(&info);
    ASSERT_EQ(info.fileContentLen, 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleFileContentLen_NonZero, TestSize.Level1)
{
    HeaderInfo info;
    info.fileContentLen = 1000;
    info.contentLen = 5000;
    downloader->HandleFileContentLen(&info);
    ASSERT_EQ(info.fileContentLen, 1000);
}

// ---- UpdateRequestSize coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_UpdateRequestSize_NoEndPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 1000;
    req->endPos_ = -1;
    downloader->UpdateRequestSize(downloader.get());
    ASSERT_EQ(req->preRequestSize_, 0);
    ASSERT_EQ(req->requestSize_, 9000);
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateRequestSize_WithEndPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->endPos_ = 500;
    req->startPos_ = 100;
    downloader->UpdateRequestSize(downloader.get());
    ASSERT_EQ(req->requestSize_, 401);
}

// ---- HandleResponseCb coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleResponseCb_Ok, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->isHeaderUpdated_.store(true);
    req->startPos_ = 0;
    Status ret = Status::OK;
    downloader->HandleResponseCb(0, 200, "ca_data", ret);
    ASSERT_EQ(req->clientError_, 0);
    ASSERT_EQ(req->serverError_, 200);
    ASSERT_EQ(downloader->ca_, "ca_data");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleResponseCb_Error, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    Status ret = Status::ERROR_CLIENT;
    downloader->HandleResponseCb(500, 404, "", ret);
    ASSERT_EQ(req->clientError_, 500);
    ASSERT_EQ(req->serverError_, 404);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleResponseCb_Destructor, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isDestructor_ = true;
    Status ret = Status::OK;
    downloader->HandleResponseCb(0, 200, "ca", ret);
    ASSERT_EQ(req->clientError_, 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleResponseCb_RangeError, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->preRequestSize_ = 8192;
    req->isFirstRangeRequestReady_ = false;
    req->serverError_ = 416;
    Status ret = Status::ERROR_SERVER;
    downloader->HandleResponseCb(0, 416, "", ret);
    ASSERT_EQ(req->startPos_, 0);
    ASSERT_EQ(req->requestSize_, 2);
    ASSERT_TRUE(req->isFirstRangeRequestReady_.load());
}

// ---- HandleRedirect coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_WithLocation, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com/path/file.mp4";
    req->location_ = "http://newhost.com/newpath";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(ret, Status::ERROR_SERVER);
    ASSERT_EQ(req->serverError_, 302);
    ASSERT_TRUE(req->haveRedirectRetry_.load());
    ASSERT_EQ(req->url_, "http://newhost.com/newpath");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_NoLocation, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(ret, Status::OK);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_AlreadyRetried, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->location_ = "http://newhost.com";
    req->haveRedirectRetry_.store(true);
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(ret, Status::OK);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_RelativePath, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com/dir/file.mp4";
    req->location_ = "other.mp4";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "http://example.com/dir/other.mp4");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_ProtocolRelative, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com/dir/file.mp4";
    req->location_ = "//newhost.com/newpath";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "http://newhost.com/newpath");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_AbsolutePath, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com/dir/file.mp4";
    req->location_ = "/newpath";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "http://example.com/newpath");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_AbsolutePathNoHostEnd, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com";
    req->location_ = "/newpath";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "http://example.com/newpath");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_HttpsLocation, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com/dir/file.mp4";
    req->location_ = "https://secure.com/newpath";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "https://secure.com/newpath");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_QueryBase, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com/dir/file.mp4?q=1";
    req->location_ = "other.mp4";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "http://example.com/dir/other.mp4");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_NoSlash, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://example.com";
    req->location_ = "other.mp4";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "http://other.mp4");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_ProtocolRelativeNoScheme, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "noscheme";
    req->location_ = "//newhost.com/path";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "//newhost.com/path");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRedirect_AbsolutePathNoScheme, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "noscheme";
    req->location_ = "/newpath";
    downloader->currentRequest_ = req;
    Status ret = Status::OK;
    downloader->HandleRedirect(ret);
    ASSERT_EQ(req->url_, "/newpath");
}

// ---- UpdateHeaderInfo coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_UpdateHeaderInfo_AlreadyUpdated, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->isHeaderUpdated_.store(true);
    downloader->UpdateHeaderInfo(downloader.get());
    ASSERT_TRUE(req->isHeaderUpdated_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateHeaderInfo_WithContentLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.contentLen = 5000;
    req->headerInfo_.fileContentLen = 0;
    downloader->UpdateHeaderInfo(downloader.get());
    ASSERT_FALSE(req->headerInfo_.isChunked);
    ASSERT_TRUE(req->isHeaderUpdated_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateHeaderInfo_ZeroContentLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.contentLen = 0;
    req->headerInfo_.fileContentLen = 0;
    downloader->UpdateHeaderInfo(downloader.get());
    ASSERT_TRUE(req->headerInfo_.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateHeaderInfo_ChunkedWithFileLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.contentLen = 0;
    req->headerInfo_.fileContentLen = 5000;
    req->headerInfo_.isChunked = true;
    downloader->UpdateHeaderInfo(downloader.get());
    ASSERT_FALSE(req->headerInfo_.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateHeaderInfo_M3u8Chunked, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->protocolType_ = RequestProtocolType::HLS;
    downloader->currentRequest_ = req;
    req->headerInfo_.contentLen = 0;
    req->headerInfo_.fileContentLen = 0;
    downloader->UpdateHeaderInfo(downloader.get());
    ASSERT_FALSE(req->headerInfo_.isChunked);
    ASSERT_TRUE(req->isHeaderUpdated_.load());
}

// ---- IsDropDataRetryRequest coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_IsDropDataRetryRequest_NotWholeFile, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->requestWholeFile_ = false;
    ASSERT_FALSE(downloader->IsDropDataRetryRequest(downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_IsDropDataRetryRequest_WholeFileNoRetry, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->requestWholeFile_ = true;
    req->shouldSaveData_.store(true);
    req->retryOnGoing_ = false;
    ASSERT_FALSE(downloader->IsDropDataRetryRequest(downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_IsDropDataRetryRequest_WholeFileRetryStartPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->requestWholeFile_ = true;
    req->shouldSaveData_.store(true);
    req->retryOnGoing_ = true;
    req->startPos_ = 100;
    ASSERT_TRUE(downloader->IsDropDataRetryRequest(downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_IsDropDataRetryRequest_WholeFileRetryZeroPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->requestWholeFile_ = true;
    req->shouldSaveData_.store(true);
    req->retryOnGoing_ = true;
    req->startPos_ = 0;
    ASSERT_FALSE(downloader->IsDropDataRetryRequest(downloader.get()));
    ASSERT_FALSE(req->retryOnGoing_);
}

// ---- HandleContentRange coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_Valid, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range: bytes 0-499/1000\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    info.fileContentLen = 0;
    ASSERT_TRUE(downloader->HandleContentRange(&info, key, next, 1, 1));
    ASSERT_EQ(info.fileContentLen, 1000);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_InvalidFormat, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range: invalid\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_NoSeparators, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range: bytes 04991000\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_InvalidFileLen, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range: bytes 0-499/-1\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_FileLenTooLarge, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range: bytes 0-499/99999999999999999\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_NotContentRange, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "other-header: value\r\n";
    char* next = buf + strlen("other-header");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_FileContentLenMismatch, TestSize.Level1)
{
    HeaderInfo info;
    info.fileContentLen = 500;
    char buf[] = "content-range: bytes 0-499/1000\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentRange(&info, key, next, 1, 1));
    ASSERT_EQ(info.fileContentLen, 500);
}

// ---- HandleContentType coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentType_Valid, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-type: video/mp4\r\n";
    char* next = buf + strlen("content-type");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentType(&info, key, next, 1, downloader.get()));
    ASSERT_EQ(std::string(info.contentType), "video/mp4");
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentType_LongType, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-type: very/very/very/long/content/type/string/that/exceeds/buffer\r\n";
    char* next = buf + strlen("content-type");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentType(&info, key, next, 1, downloader.get()));
    ASSERT_EQ(strlen(info.contentType), static_cast<size_t>(31));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentType_NotContentType, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "other-header: value\r\n";
    char* next = buf + strlen("other-header");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentType(&info, key, next, 1, downloader.get()));
}

// ---- HandleContentEncode coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentEncode_Valid, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-encode: gzip\r\n";
    char* next = buf + strlen("content-encode");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentEncode(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentEncode_NotContentEncode, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "other-header: value\r\n";
    char* next = buf + strlen("other-header");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentEncode(&info, key, next, 1, 1));
}

// ---- HandleContentLength coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_Valid, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-length: 1024\r\n";
    char* next = buf + strlen("content-length");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentLength(&info, key, next, downloader.get()));
    ASSERT_EQ(info.contentLen, 1024);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_Chunked, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    HeaderInfo info;
    char buf[] = "content-length: 0\r\n";
    char* next = buf + strlen("content-length");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentLength(&info, key, next, downloader.get()));
    ASSERT_TRUE(info.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_NotContentLength, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "other-header: value\r\n";
    char* next = buf + strlen("other-header");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentLength(&info, key, next, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_InvalidValue, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-length: abc\r\n";
    char* next = buf + strlen("content-length");
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentLength(&info, key, next, downloader.get()));
}

// ---- HandleRange coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleRange_AcceptRanges, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "accept-ranges: bytes\r\n";
    char* next = buf + strlen("accept-ranges");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleRange(&info, key, next, 1, 1));
    ASSERT_TRUE(info.isServerAcceptRange);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRange_AcceptRangesNotBytes, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "accept-ranges: none\r\n";
    char* next = buf + strlen("accept-ranges");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleRange(&info, key, next, 1, 1));
    ASSERT_FALSE(info.isServerAcceptRange);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRange_ContentRange, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range: bytes 0-499/1000\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleRange(&info, key, next, 1, 1));
    ASSERT_TRUE(info.isServerAcceptRange);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRange_NotRange, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "other-header: value\r\n";
    char* next = buf + strlen("other-header");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleRange(&info, key, next, 1, 1));
}

// ---- HandleTransferEncoding coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_HandleTransferEncoding_Chunked, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    HeaderInfo info;
    char buf[] = "transfer-encoding: chunked\r\n";
    char* next = buf + strlen("transfer-encoding");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleTransferEncoding(&info, key, next, downloader.get()));
    ASSERT_TRUE(info.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleTransferEncoding_ChunkedFlv, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    HeaderInfo info;
    char buf[] = "transfer-encoding: chunked\r\n";
    char* next = buf + strlen("transfer-encoding");
    char* key = buf;
    downloader->currentRequest_->url_ = "http://test.com/video.flv";
    ASSERT_TRUE(downloader->HandleTransferEncoding(&info, key, next, downloader.get()));
    ASSERT_TRUE(info.isChunked);
    ASSERT_EQ(info.contentLen, 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleTransferEncoding_NotChunked, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    HeaderInfo info;
    char buf[] = "transfer-encoding: identity\r\n";
    char* next = buf + strlen("transfer-encoding");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleTransferEncoding(&info, key, next, downloader.get()));
    ASSERT_FALSE(info.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleTransferEncoding_ChunkedM3u8, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    HeaderInfo info;
    char buf[] = "transfer-encoding: chunked\r\n";
    char* next = buf + strlen("transfer-encoding");
    char* key = buf;
    downloader->currentRequest_->protocolType_ = RequestProtocolType::HLS;
    ASSERT_TRUE(downloader->HandleTransferEncoding(&info, key, next, downloader.get()));
    ASSERT_FALSE(info.isChunked);
}

// ---- ToLower coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_ToLower_Normal, TestSize.Level1)
{
    char str[] = "HelloWORLD";
    downloader->ToLower(str);
    ASSERT_EQ(std::string(str), "helloworld");
}

HWTEST_F(DownloaderUnitTest, Downloader_ToLower_Null, TestSize.Level1)
{
    downloader->ToLower(nullptr);
    ASSERT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

// ---- StopBufferring coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_StopBufferring_NullTask, TestSize.Level1)
{
    downloader->task_ = nullptr;
    downloader->StopBufferring();
    ASSERT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, Downloader_StopBufferring_AppBackground, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isAppBackground_ = true;
    downloader->StopBufferring();
    ASSERT_EQ(downloader->client_, nullptr);
    ASSERT_EQ(downloader->isAppBackground_.load(), true);
}

HWTEST_F(DownloaderUnitTest, Downloader_StopBufferring_RestartWithStartPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isAppBackground_ = false;
    downloader->client_ = nullptr;
    req->startPos_ = 100;
    req->protocolType_ = RequestProtocolType::HLS;
    downloader->StopBufferring();
    ASSERT_NE(downloader->client_, nullptr);
    ASSERT_TRUE(req->retryOnGoing_);
}

// ---- SetInterruptState with sourceLoader ----

HWTEST_F(DownloaderUnitTest, Downloader_SetInterruptState_WithSourceLoader, TestSize.Level1)
{
    auto loader = std::make_shared<SourceLoader>();
    auto sourceLoader = std::make_shared<MediaSourceLoaderCombinations>(loader);
    auto dl = std::make_shared<Downloader>("test_interrupt", sourceLoader);
    dl->Init();
    dl->currentRequest_ = MakeTestRequest();
    dl->SetInterruptState(true);
    ASSERT_TRUE(dl->isInterruptNeeded_.load());
}

// ---- NotifyLoopPause / WaitLoopPause coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_NotifyLoopPause_Idle, TestSize.Level1)
{
    downloader->loopStatus_ = Downloader::LoopStatus::IDLE;
    downloader->isInterruptNeeded_ = false;
    downloader->NotifyLoopPause();
    ASSERT_EQ(downloader->loopStatus_.load(), Downloader::LoopStatus::IDLE);
}

HWTEST_F(DownloaderUnitTest, Downloader_WaitLoopPause_Idle, TestSize.Level1)
{
    downloader->loopStatus_ = Downloader::LoopStatus::IDLE;
    downloader->WaitLoopPause();
    ASSERT_EQ(downloader->loopStatus_.load(), Downloader::LoopStatus::IDLE);
}

// ---- UpdateDownloadInfo coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_UpdateDownloadInfo_FirstDownload, TestSize.Level1)
{
    downloader->startDownTime_ = 1000;
    downloader->lastDownloadTime_ = 0;
    downloader->downloadCallback_ = nullptr;
    int64_t before = downloader->GetCurrentMillisecond();
    downloader->UpdateDownloadInfo(downloader.get(), 1024);
    int64_t after = downloader->GetCurrentMillisecond();
    ASSERT_EQ(downloader->startDownTime_, 1000);
    ASSERT_GE(downloader->lastDownloadTime_, before);
    ASSERT_LE(downloader->lastDownloadTime_, after);
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateDownloadInfo_SubsequentDownload, TestSize.Level1)
{
    downloader->startDownTime_ = 1000;
    downloader->lastDownloadTime_ = 2000;
    downloader->downloadCallback_ = nullptr;
    downloader->UpdateDownloadInfo(downloader.get(), 1024);
    ASSERT_NE(downloader->lastDownloadTime_, 2000);
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateDownloadInfo_NullDownloader, TestSize.Level1)
{
    downloader->UpdateDownloadInfo(nullptr, 1024);
    ASSERT_EQ(downloader->lastDownloadTime_, 0);
}

// ---- Cancel coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_Cancel_WithRequest, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->retryTimes_.store(5);
    downloader->Cancel();
    ASSERT_FALSE(downloader->isCancelling_.load());
    ASSERT_EQ(req->retryTimes_.load(), 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_Cancel_NullRequest, TestSize.Level1)
{
    downloader->currentRequest_ = nullptr;
    downloader->Cancel();
    ASSERT_FALSE(downloader->isCancelling_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_Cancel_NullClient, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->client_ = nullptr;
    downloader->Cancel();
    ASSERT_FALSE(downloader->isCancelling_.load());
}

// ---- Resume coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_Resume_ClientClosed, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isClientClose_ = true;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    req->headerInfo_.fileContentLen = 10000;
    req->isHeaderUpdated_.store(true);
    req->startPos_ = 100;
    req->requestSize_ = 20000;
    downloader->Resume();
    ASSERT_FALSE(downloader->isClientClose_);
    ASSERT_TRUE(req->requestSize_ <= 10000);
}

HWTEST_F(DownloaderUnitTest, Downloader_Resume_StartPosLessThanFileLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isClientClose_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    req->headerInfo_.fileContentLen = 500;
    req->isHeaderUpdated_.store(true);
    req->startPos_ = 100;
    req->requestSize_ = 1000;
    downloader->Resume();
    ASSERT_EQ(req->requestSize_, 400);
}

HWTEST_F(DownloaderUnitTest, Downloader_Resume_Interrupted, TestSize.Level1)
{
    downloader->isInterruptNeeded_ = true;
    downloader->Resume();
    ASSERT_TRUE(downloader->isInterruptNeeded_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_Resume_Destructor, TestSize.Level1)
{
    downloader->isDestructor_ = true;
    downloader->Resume();
    ASSERT_TRUE(downloader->isDestructor_.load());
}

// ---- RxHeaderData coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_TransferEncoding, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "transfer-encoding: chunked\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
    ASSERT_TRUE(downloader->currentRequest_->headerInfo_.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_Location, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "location: http://redirect.com\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
    ASSERT_EQ(req->location_, "http://redirect.com");
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentLength, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-length: 1024\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
    ASSERT_EQ(req->headerInfo_.contentLen, 1024);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentType, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-type: video/mp4\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
    ASSERT_EQ(std::string(req->headerInfo_.contentType), "video/mp4");
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_AlreadyUpdated, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->isHeaderUpdated_.store(true);
    const char* header = "content-length: 1024\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_NullUserParam, TestSize.Level1)
{
    const char* header = "content-length: 1024\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, nullptr);
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ZeroTotalLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "";
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 0, 0, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

// ---- RxBodyData coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_NullUserParam, TestSize.Level1)
{
    uint8_t buf[100] = {0};
    size_t ret = downloader->RxBodyData(buf, 100, 1, nullptr);
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_OverflowCheck, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    uint8_t buf[100] = {0};
    size_t ret = downloader->RxBodyData(buf, 1, std::numeric_limits<size_t>::max(), downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_WithLocation, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->location_ = "http://redirect.com";
    req->haveRedirectRetry_.store(false);
    uint8_t buf[100] = {0};
    size_t ret = downloader->RxBodyData(buf, 100, 1, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_NotSaveData, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->shouldSaveData_.store(false);
    req->headerInfo_.fileContentLen = 1000;
    req->headerInfo_.contentLen = 1000;
    uint8_t buf[100] = {0};
    size_t ret = downloader->RxBodyData(buf, 100, 1, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(100));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_SaveData, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->shouldSaveData_.store(true);
    req->headerInfo_.fileContentLen = 10000;
    req->headerInfo_.contentLen = 10000;
    uint8_t buf[100] = {0};
    size_t ret = downloader->RxBodyData(buf, 100, 1, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(100));
}


// ---- Additional tests for branch coverage round 2 ----

HWTEST_F(DownloaderUnitTest, Downloader_Pause_Async, TestSize.Level1)
{
    downloader->Pause(true);
    ASSERT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, Downloader_Pause_NullClient, TestSize.Level1)
{
    downloader->client_ = nullptr;
    downloader->Pause(true);
    ASSERT_EQ(downloader->shouldStartNextRequest_.load(), true);
}

HWTEST_F(DownloaderUnitTest, Downloader_Stop_Async, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->Stop(true);
    ASSERT_TRUE(req->isInterruptNeeded_);
}

HWTEST_F(DownloaderUnitTest, Downloader_Stop_SyncWithRequest, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->Stop(false);
    ASSERT_TRUE(req->isInterruptNeeded_);
    ASSERT_TRUE(req->IsClosed());
}

HWTEST_F(DownloaderUnitTest, Downloader_GetIp_NullClient, TestSize.Level1)
{
    downloader->client_ = nullptr;
    std::string ip = "test";
    downloader->GetIp(ip);
    ASSERT_EQ(ip, std::string("test"));
}

HWTEST_F(DownloaderUnitTest, Downloader_SetAppUid_NullClient, TestSize.Level1)
{
    downloader->client_ = nullptr;
    downloader->SetAppUid(12345);
    ASSERT_EQ(downloader->name_, std::string("test"));
}

HWTEST_F(DownloaderUnitTest, Downloader_BeginDownload_NullClient, TestSize.Level1)
{
    auto req = MakeTestRequest();
    req->url_ = "http://test.com";
    downloader->currentRequest_ = req;
    downloader->client_ = nullptr;
    ASSERT_TRUE(downloader->BeginDownload());
    ASSERT_EQ(req->requestSize_, 8192);
}

HWTEST_F(DownloaderUnitTest, Downloader_SetInterruptState_NoRequest, TestSize.Level1)
{
    downloader->currentRequest_ = nullptr;
    downloader->SetInterruptState(true);
    ASSERT_TRUE(downloader->isInterruptNeeded_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_SetInterruptState_NoRequest_NoInterrupt, TestSize.Level1)
{
    downloader->currentRequest_ = nullptr;
    downloader->SetInterruptState(false);
    ASSERT_FALSE(downloader->isInterruptNeeded_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_Download_NotInterrupt, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->isInterruptNeeded_ = false;
    downloader->isCancelling_.store(false);
    bool ret = downloader->Download(req, 100);
    ASSERT_TRUE(ret);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetErrorCode_OftenError_Interrupted, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->serverError_ = 500;
    downloader->isInterruptNeeded_ = true;
    downloader->HandleRetErrorCode();
    ASSERT_EQ(req->serverError_, 500);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_RetryTimes, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 100;
    req->isHeaderUpdated_.store(true);
    req->retryTimes_.store(0);
    downloader->HandleRetOK();
    ASSERT_EQ(req->requestSize_, 9900);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_NotChunked_QueueNotEmpty, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 0;
    req->isHeaderUpdated_.store(true);
    req->headerInfo_.isChunked = true;
    auto req2 = MakeTestRequest("http://test2.com");
    downloader->requestQue_->Push(req2, 1000);
    downloader->HandleRetOK();
    ASSERT_FALSE(req->isEos_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRetOK_EndPosRemaining, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 100;
    req->endPos_ = 200;
    req->isHeaderUpdated_.store(true);
    downloader->HandleRetOK();
    ASSERT_EQ(req->requestSize_, 101);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandlePlayingFinish_NoCallbackDestructor, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isDestructor_ = true;
    downloader->HandlePlayingFinish();
    ASSERT_TRUE(downloader->shouldStartNextRequest_.load());
    ASSERT_TRUE(downloader->isDestructor_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_HandlePlayingFinish_CallbackDestructor, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    bool called = false;
    req->downloadDoneCallback_ = [&called](const std::string&, const std::string&) { called = true; };
    downloader->isDestructor_ = true;
    downloader->HandlePlayingFinish();
    ASSERT_FALSE(called);
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateHeaderInfo_AlreadyContentTypeUpdated, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.contentLen = 0;
    req->headerInfo_.fileContentLen = 0;
    downloader->isContentTypeUpdated_.store(true);
    downloader->UpdateHeaderInfo(downloader.get());
    ASSERT_TRUE(req->isHeaderUpdated_.load());
    ASSERT_TRUE(downloader->isContentTypeUpdated_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateHeaderInfo_LiveContentLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.contentLen = LIVE_CONTENT_LENGTH;
    req->headerInfo_.fileContentLen = 0;
    downloader->UpdateHeaderInfo(downloader.get());
    ASSERT_FALSE(req->headerInfo_.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_SaveBodyData_WithKeyIndex, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    uint8_t buf[100] = {0};
    req->keyIndex_ = 1;
    req->keySaveData_ = [](uint8_t*, uint32_t len, bool, uint64_t) { return len; };
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 100;
    req->endPos_ = -1;
    size_t ret = downloader->SaveBodyData(downloader.get(), buf, 100);
    ASSERT_EQ(ret, static_cast<size_t>(100));
}

HWTEST_F(DownloaderUnitTest, Downloader_SaveBodyData_PartialWrite, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    uint8_t buf[100] = {0};
    req->saveData_ = [](uint8_t*, uint32_t, bool) { return static_cast<uint32_t>(50); };
    req->headerInfo_.fileContentLen = 10000;
    req->startPos_ = 100;
    req->endPos_ = -1;
    size_t ret = downloader->SaveBodyData(downloader.get(), buf, 100);
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentType_NullToken, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-type:\r\n";
    char* next = nullptr;
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentType(&info, key, next, 1, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentType_EmptyType, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-type: \r\n";
    char* next = buf + strlen("content-type");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentType(&info, key, next, 1, downloader.get()));
    ASSERT_EQ(strlen(info.contentType), static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentEncode_NullToken, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-encode:\r\n";
    char* next = nullptr;
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentEncode(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_NullKey, TestSize.Level1)
{
    HeaderInfo info;
    ASSERT_FALSE(downloader->HandleContentLength(&info, nullptr, nullptr, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_NotMatch, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "other: value\r\n";
    char* next = buf + strlen("other");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentLength(&info, key, next, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_M3u8Chunked, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    downloader->currentRequest_->protocolType_ = RequestProtocolType::HLS;
    HeaderInfo info;
    char buf[] = "content-length: 0\r\n";
    char* next = buf + strlen("content-length");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentLength(&info, key, next, downloader.get()));
    ASSERT_FALSE(info.isChunked);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRange_AcceptRanges_NullToken, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "accept-ranges:\r\n";
    char* next = nullptr;
    char* key = buf;
    ASSERT_FALSE(downloader->HandleRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRange_ContentRange_NoBytes, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range: items 0-499/1000\r\n";
    char* next = buf + strlen("content-range");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleRange(&info, key, next, 1, 1));
    ASSERT_FALSE(info.isServerAcceptRange);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleRange_ContentRange_NullToken, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range:\r\n";
    char* next = nullptr;
    char* key = buf;
    ASSERT_FALSE(downloader->HandleRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_ToLower_LongString, TestSize.Level1)
{
    char str[200];
    memset(str, 'A', 199);
    str[199] = '\0';
    downloader->ToLower(str);
    ASSERT_EQ(str[50], 'a');
    ASSERT_EQ(str[99], 'a');
    ASSERT_EQ(str[101], 'A');
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_OtherHeader, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "x-custom-header: value\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_TooLarge, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    std::string bigHeader(20000, 'x');
    size_t ret = downloader->RxHeaderData(const_cast<char*>(bigHeader.c_str()), 1, bigHeader.size(), downloader.get());
    ASSERT_EQ(ret, bigHeader.size());
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_NotDownloading, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->shouldSaveData_.store(true);
    req->headerInfo_.fileContentLen = 10000;
    req->headerInfo_.contentLen = 10000;
    req->isDownloading_.store(true);
    uint8_t buf[100] = {0};
    size_t ret = downloader->RxBodyData(buf, 100, 1, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(100));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_SizeZero, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->shouldSaveData_.store(true);
    req->headerInfo_.fileContentLen = 10000;
    req->headerInfo_.contentLen = 10000;
    uint8_t buf[100] = {0};
    size_t ret = downloader->RxBodyData(buf, 0, 100, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_StopBufferring_AppBackgroundNullClient, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    downloader->isAppBackground_ = true;
    downloader->client_ = nullptr;
    downloader->StopBufferring();
    ASSERT_EQ(downloader->isAppBackground_.load(), true);
}

HWTEST_F(DownloaderUnitTest, Downloader_StopBufferring_ForegroundClientExists, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    downloader->isAppBackground_ = false;
    downloader->StopBufferring();
    ASSERT_NE(downloader->client_, nullptr);
    ASSERT_EQ(downloader->isAppBackground_.load(), false);
}

HWTEST_F(DownloaderUnitTest, Downloader_StopBufferring_StartPosZeroHttp, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    downloader->isAppBackground_ = false;
    downloader->client_ = nullptr;
    req->startPos_ = 100;
    req->protocolType_ = RequestProtocolType::HTTP;
    downloader->StopBufferring();
    ASSERT_NE(downloader->client_, nullptr);
    ASSERT_FALSE(req->retryOnGoing_);
}

HWTEST_F(DownloaderUnitTest, Downloader_StopBufferring_StartPosZeroNonHttp, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->task_ = std::make_shared<Task>(std::string("OS_Downloader"));
    downloader->isAppBackground_ = false;
    downloader->client_ = nullptr;
    req->startPos_ = 0;
    req->protocolType_ = RequestProtocolType::HLS;
    downloader->StopBufferring();
    ASSERT_NE(downloader->client_, nullptr);
    ASSERT_FALSE(req->retryOnGoing_);
}

HWTEST_F(DownloaderUnitTest, Downloader_Resume_ClientCloseNullRequest, TestSize.Level1)
{
    downloader->isClientClose_ = true;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    downloader->currentRequest_ = nullptr;
    downloader->Resume();
    ASSERT_TRUE(downloader->isClientClose_);
}

HWTEST_F(DownloaderUnitTest, Downloader_Resume_NoClientClose, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isClientClose_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    req->headerInfo_.fileContentLen = 10000;
    req->isHeaderUpdated_.store(true);
    req->startPos_ = 100;
    req->requestSize_ = 50;
    downloader->Resume();
    ASSERT_FALSE(downloader->isClientClose_);
}

HWTEST_F(DownloaderUnitTest, Downloader_Resume_StartPosGEFileLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->isClientClose_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    req->headerInfo_.fileContentLen = 100;
    req->isHeaderUpdated_.store(true);
    req->startPos_ = 200;
    req->requestSize_ = 50;
    downloader->Resume();
    ASSERT_EQ(req->requestSize_, 50);
}

HWTEST_F(DownloaderUnitTest, Downloader_UpdateDownloadInfo_WithCallback, TestSize.Level1)
{
    downloader->startDownTime_ = 1000;
    downloader->lastDownloadTime_ = 0;
    auto callback = std::make_shared<DownloadMetricsInfo>();
    downloader->downloadCallback_ = callback;
    int64_t before = downloader->GetCurrentMillisecond();
    downloader->UpdateDownloadInfo(downloader.get(), 1024);
    ASSERT_EQ(downloader->startDownTime_, 1000);
    ASSERT_GE(downloader->lastDownloadTime_, before);
}

HWTEST_F(DownloaderUnitTest, Downloader_RetryCore_ShouldStartNext, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->shouldStartNextRequest_ = true;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    ASSERT_FALSE(downloader->RetryCore(req));
}

HWTEST_F(DownloaderUnitTest, Downloader_Retry_Destructor, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->isDestructor_ = true;
    ASSERT_FALSE(downloader->Retry(req));
}

HWTEST_F(DownloaderUnitTest, Downloader_RetryCore_SameRequest, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->shouldStartNextRequest_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    downloader->requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>("testQue");
    bool result = downloader->RetryCore(req);
    ASSERT_TRUE(result);
    ASSERT_EQ(req->retryTimes_.load(), 1);
}

HWTEST_F(DownloaderUnitTest, Downloader_RetryCore_DifferentRequest, TestSize.Level1)
{
    auto req1 = MakeTestRequest("http://test1.com");
    auto req2 = MakeTestRequest("http://test2.com");
    downloader->currentRequest_ = req1;
    downloader->shouldStartNextRequest_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    downloader->requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>("testQue");
    bool result = downloader->RetryCore(req2);
    ASSERT_TRUE(result);
    ASSERT_EQ(req1->retryTimes_.load(), 0);
}

HWTEST_F(DownloaderUnitTest, Downloader_RetryCore_NoEndPosRange, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->shouldStartNextRequest_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    downloader->requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>("testQue");
    req->endPos_ = -1;
    req->startPos_ = 0;
    bool result = downloader->RetryCore(req);
    ASSERT_TRUE(result);
    ASSERT_EQ(req->requestSize_, 0);
}


HWTEST_F(DownloaderUnitTest, Downloader_Download_WaitMsMinusOne, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->isInterruptNeeded_ = false;
    downloader->isCancelling_.store(false);
    bool ret = downloader->Download(req, -1);
    ASSERT_TRUE(ret);
}

HWTEST_F(DownloaderUnitTest, Downloader_Cancel_WithRetryTimes, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->retryTimes_.store(3);
    downloader->Cancel();
    ASSERT_EQ(req->retryTimes_.load(), 0);
    ASSERT_FALSE(downloader->isCancelling_.load());
}

HWTEST_F(DownloaderUnitTest, Downloader_GetContentType_Normal, TestSize.Level1)
{
    downloader->isDestructor_ = false;
    downloader->isContentTypeUpdated_.store(false);
    downloader->isInterruptNeeded_ = true;
    std::string result = downloader->GetContentType();
    ASSERT_EQ(result, downloader->contentType_);
}

HWTEST_F(DownloaderUnitTest, Downloader_SetDownloadCallback_Test, TestSize.Level1)
{
    auto callback = std::make_shared<DownloadMetricsInfo>();
    downloader->SetDownloadCallback(callback);
    ASSERT_EQ(downloader->downloadCallback_, callback);
}

HWTEST_F(DownloaderUnitTest, Downloader_GetCurrentMillisecond_Test, TestSize.Level1)
{
    int64_t t1 = downloader->GetCurrentMillisecond();
    int64_t t2 = downloader->GetCurrentMillisecond();
    ASSERT_GE(t2, t1);
}


// ---- Round 3: more branch coverage ----

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_NoColon, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-range\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentRangeInvalidStart, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-range: bytes abc-499/1000\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentRangeInvalidEnd, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-range: bytes 0-abc/1000\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentRangeInvalidFileLen, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-range: bytes 0-499/abc\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentRangeNoSeparators, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-range: bytes 04991000\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentRangeMismatch, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->headerInfo_.fileContentLen = 500;
    const char* header = "content-range: bytes 0-499/1000\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
    ASSERT_EQ(req->headerInfo_.fileContentLen, 500);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentLengthNoNext, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-length\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_AcceptRangesNoToken, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "accept-ranges\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentRangeNoToken, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-range\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_LocationNoNext, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "location\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_TransferEncodingNoToken, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "transfer-encoding\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_RxHeaderData_ContentEncode, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    const char* header = "content-encode: gzip\r\n";
    size_t len = strlen(header);
    size_t ret = downloader->RxHeaderData(const_cast<char*>(header), 1, len, downloader.get());
    ASSERT_EQ(ret, len);
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentRange_DirectNoToken, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-range";
    char* next = nullptr;
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentRange(&info, key, next, 1, 1));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_DirectNullNext, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    HeaderInfo info;
    char buf[] = "content-length";
    char* next = nullptr;
    char* key = buf;
    ASSERT_FALSE(downloader->HandleContentLength(&info, key, next, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_DirectNullInfo, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    char buf[] = "content-length: 1024\r\n";
    char* next = buf + strlen("content-length");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentLength(nullptr, key, next, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleContentLength_DirectNullDownloader, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "content-length: 1024\r\n";
    char* next = buf + strlen("content-length");
    char* key = buf;
    ASSERT_TRUE(downloader->HandleContentLength(&info, key, next, nullptr));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleTransferEncoding_NullDownloader, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "transfer-encoding: chunked\r\n";
    char* next = buf + strlen("transfer-encoding");
    char* key = buf;
    ASSERT_FALSE(downloader->HandleTransferEncoding(&info, key, next, nullptr));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleTransferEncoding_NullCurrentRequest, TestSize.Level1)
{
    HeaderInfo info;
    char buf[] = "transfer-encoding: chunked\r\n";
    char* next = buf + strlen("transfer-encoding");
    char* key = buf;
    downloader->currentRequest_ = nullptr;
    ASSERT_FALSE(downloader->HandleTransferEncoding(&info, key, next, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_HandleTransferEncoding_NoToken, TestSize.Level1)
{
    downloader->currentRequest_ = MakeTestRequest();
    HeaderInfo info;
    char buf[] = "transfer-encoding";
    char* next = nullptr;
    char* key = buf;
    ASSERT_FALSE(downloader->HandleTransferEncoding(&info, key, next, downloader.get()));
}

HWTEST_F(DownloaderUnitTest, Downloader_Pause_AsyncWithMockRetry, TestSize.Level1)
{
    auto mock = std::make_shared<MockCloseRetryClient>();
    mock->closeFailTimes_ = 2;
    downloader->client_ = mock;
    downloader->Pause(true);
    ASSERT_EQ(mock->closeCallCount_, 3);
}

HWTEST_F(DownloaderUnitTest, Downloader_DropRetryData_PartialDrop, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->startPos_ = 500;
    req->dropedDataLen_ = 0;
    uint8_t buf[1000] = {0};
    size_t ret = downloader->DropRetryData(buf, 1000, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(1000));
}

HWTEST_F(DownloaderUnitTest, Downloader_DropRetryData_AllDrop, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->startPos_ = 500;
    req->dropedDataLen_ = 0;
    uint8_t buf[300] = {0};
    size_t ret = downloader->DropRetryData(buf, 300, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(300));
}

HWTEST_F(DownloaderUnitTest, Downloader_DropRetryData_NoDropNeeded, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->startPos_ = 100;
    req->dropedDataLen_ = 100;
    uint8_t buf[500] = {0};
    size_t ret = downloader->DropRetryData(buf, 500, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(0));
}

HWTEST_F(DownloaderUnitTest, Downloader_DropRetryData_WithKeyIndex, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->startPos_ = 500;
    req->dropedDataLen_ = 0;
    req->keyIndex_ = 1;
    req->keySaveData_ = [](uint8_t*, uint32_t len, bool, uint64_t) { return len; };
    uint8_t buf[1000] = {0};
    size_t ret = downloader->DropRetryData(buf, 1000, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(1000));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_DropRetryWithIndex, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->requestWholeFile_ = true;
    req->shouldSaveData_.store(true);
    req->retryOnGoing_ = true;
    req->startPos_ = 100;
    req->headerInfo_.fileContentLen = 10000;
    req->headerInfo_.contentLen = 10000;
    req->SetIsIndexM3u8Request(false);
    uint8_t buf[200] = {0};
    size_t ret = downloader->RxBodyData(buf, 200, 1, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(200));
}

HWTEST_F(DownloaderUnitTest, Downloader_RxBodyData_DropRetryIndexM3u8, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    req->requestWholeFile_ = true;
    req->shouldSaveData_.store(true);
    req->retryOnGoing_ = true;
    req->startPos_ = 100;
    req->headerInfo_.fileContentLen = 10000;
    req->headerInfo_.contentLen = 10000;
    req->SetIsIndexM3u8Request(true);
    uint8_t buf[200] = {0};
    size_t ret = downloader->RxBodyData(buf, 200, 1, downloader.get());
    ASSERT_EQ(ret, static_cast<size_t>(200));
}

HWTEST_F(DownloaderUnitTest, Downloader_Retry_ShouldStartNextRequest, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->shouldStartNextRequest_ = true;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    ASSERT_FALSE(downloader->Retry(req));
}

HWTEST_F(DownloaderUnitTest, Downloader_RetryCore_IsSameAndEndPos, TestSize.Level1)
{
    auto req = MakeTestRequest();
    downloader->currentRequest_ = req;
    downloader->shouldStartNextRequest_ = false;
    downloader->isDestructor_ = false;
    downloader->isInterruptNeeded_ = false;
    downloader->requestQue_ = std::make_shared<BlockingQueue<std::shared_ptr<DownloadRequest>>>("testQue");
    req->endPos_ = 200;
    req->startPos_ = 100;
    bool result = downloader->RetryCore(req);
    ASSERT_TRUE(result);
    ASSERT_EQ(req->requestSize_, 101);
}

HWTEST_F(DownloaderUnitTest, Downloader_NotifyLoopPause_Pause, TestSize.Level1)
{
    downloader->loopStatus_ = Downloader::LoopStatus::PAUSE;
    downloader->isInterruptNeeded_ = false;
    downloader->NotifyLoopPause();
    ASSERT_EQ(downloader->loopStatus_.load(), Downloader::LoopStatus::IDLE);
}

HWTEST_F(DownloaderUnitTest, Downloader_NotifyLoopPause_Interrupted, TestSize.Level1)
{
    downloader->loopStatus_ = Downloader::LoopStatus::START;
    downloader->isInterruptNeeded_ = true;
    downloader->NotifyLoopPause();
    ASSERT_EQ(downloader->loopStatus_.load(), Downloader::LoopStatus::IDLE);
}

HWTEST_F(DownloaderUnitTest, Downloader_SetInterruptState_NoRequestTrue, TestSize.Level1)
{
    downloader->currentRequest_ = nullptr;
    downloader->sourceLoader_ = nullptr;
    downloader->SetInterruptState(true);
    ASSERT_TRUE(downloader->isInterruptNeeded_.load());
}

}
}
}
}
