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
#include "gtest/gtest.h"
#include "utils/string_utils.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
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

    int64_t offset2 = (int64_t)writeLen * 3;
    ASSERT_EQ(writeLen, cachedMediaBuffer.Write(mp4Data.get(), offset2, writeLen));
    EXPECT_EQ(writeLen, cachedMediaBuffer.GetBufferSize(offset1));
    EXPECT_EQ((size_t)offset2, cachedMediaBuffer.GetNextBufferOffset(offset1));
    EXPECT_EQ((size_t)offset2, cachedMediaBuffer.GetNextBufferOffset(offset1 + writeLen - 3));

    cachedMediaBuffer.Dump(0);
    EXPECT_EQ((size_t)offset2, cachedMediaBuffer.GetNextBufferOffset(offset1 + writeLen));
    EXPECT_EQ((size_t)offset2, cachedMediaBuffer.GetNextBufferOffset(offset1 + writeLen + 1));
    EXPECT_EQ((size_t)offset2, cachedMediaBuffer.GetNextBufferOffset(offset2 - 2));

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
    EXPECT_FALSE(StringUtil::SafeStoInt32(str, value));
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
    EXPECT_NE(downloader1->sourceId_, 0);
    downloader1->sourceLoader_ = nullptr;
    downloader1->OpenAppUri();
    EXPECT_NE(downloader1->sourceId_, 0);
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
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->callback_ = sourceCallback;
    downloader->NotifyError(52, 403);
    std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>("", nullptr, nullptr,  false);
    request->clientError_ = 992;
    EXPECT_EQ(downloader->NeedRetry(request), false);
    downloader->GetDownloadInfo();
    downloader->GetBufferSize();
    downloader->GetBufferingTimeOut();
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
    downloader->GetStartedStatus();
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
    Plugins::Callback* sourceCallback = new SourceCallback();
    downloader->downloader_ = nullptr;
    downloader->callback_ = sourceCallback;
    downloader->NotifyError(52, 403);
    std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>("", nullptr, nullptr,  false);
    request->clientError_ = 992;
    EXPECT_EQ(downloader->NeedRetry(request), false);
    downloader->GetDownloadInfo();
    downloader->GetBufferSize();
    downloader->GetBufferingTimeOut();
    downloader->GetReadTimeOut(true);
    downloader->GetSegmentOffset();
    downloader->GetHLSDiscontinuity();
    downloader->StopBufferring(true);
    downloader->WaitForBufferingEnd();
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
    downloader->callback_ = nullptr;
    downloader->NotifyError(52, 403);
    downloader->NotifyError(52, 0);
    downloader->NotifyError(0, 0);
    EXPECT_EQ(downloader->callback_, nullptr);
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
            [](int32_t clientCode, int32_t serverCode, Status status) {}, uuid, error);
        if (threadRequestData.joinable()) {
            threadRequestData.join();
        }
    }
}
}
}
}
}