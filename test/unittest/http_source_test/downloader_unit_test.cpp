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

#include "download/downloader.h"
#include "monitor/download_monitor.h"
#include "utils/media_cached_buffer.h"
#include "download/network_client/http_curl_client.h"
#include "gtest/gtest.h"

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

void DownloaderUnitTest::SetUpTestCase(void)
{
}

void DownloaderUnitTest::TearDownTestCase(void)
{
}

void DownloaderUnitTest::SetUp(void)
{
    downloader = std::make_shared<Downloader>("test");
}

void DownloaderUnitTest::TearDown(void)
{
    downloader.reset();
}

HWTEST_F(DownloaderUnitTest, Downloader_Construct_nullptr, TestSize.Level1)
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isDestructor_ = false;
    downloader->shouldStartNextRequest = false;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
    };
    std::shared_ptr<DownloadRequest> Request_ =
        std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isDestructor_ = false;
    downloader->shouldStartNextRequest = false;
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

    constexpr int64_t mediaSize = 1024 * 1024 * 3;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);
    size_t writeLen = 1024;
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

HWTEST_F(DownloaderUnitTest, DeleteOtherHasReadFragmentCache, TestSize.Level1)
{
    CacheMediaChunkBuffer cachedMediaBuffer;
    uint64_t totalSize = 128 * 1024;
    uint32_t chunkSize = 128;
    EXPECT_EQ(true, cachedMediaBuffer.Init(totalSize, chunkSize));

    int64_t mediaSize = 1024 * 8;
    std::unique_ptr<uint8_t[]> mp4Data = std::make_unique<uint8_t[]>(mediaSize);
    size_t* data = (size_t*)mp4Data.get();
    for (size_t i = 0; i < mediaSize / sizeof(size_t); i++) {
        data[i] = i;
    }

    size_t writeLen = 128 * 2;
    int64_t offset1 = 0;
    ASSERT_EQ(writeLen, cachedMediaBuffer.Write(mp4Data.get(), offset1, writeLen));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset2 = 128 * 10;
    size_t writeLen2 = 128 * 2;
    ASSERT_EQ(writeLen2, cachedMediaBuffer.Write(mp4Data.get() + offset2, offset2, writeLen2));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset3 = 128 * 15;
    size_t writeLen3 = 128 * 2;
    ASSERT_EQ(writeLen3, cachedMediaBuffer.Write(mp4Data.get() + offset3, offset3, writeLen3));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    uint32_t buffer[1024 * 2] = {0};
    size_t readSize = 128u;
    ASSERT_EQ(readSize, cachedMediaBuffer.Read(buffer, offset2, readSize));
    ASSERT_EQ(writeLen, cachedMediaBuffer.Read(buffer, offset1, writeLen));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset5 = writeLen;
    size_t writeLen5 = 128 * 3;
    ASSERT_EQ(writeLen5, cachedMediaBuffer.Write(mp4Data.get() + offset5, offset5, writeLen5));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    ASSERT_EQ(writeLen, cachedMediaBuffer.Read(buffer, offset1 + writeLen, writeLen));
    EXPECT_EQ(true, cachedMediaBuffer.Check());

    int64_t offset7 = writeLen5 + offset5;
    size_t writeLen7 = 128;
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
    constexpr int64_t mediaSize = 1024 * (chunkNum + 1);
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

    constexpr int64_t mediaSize = 1024 * (chunkNum + 1);
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

    constexpr int64_t mediaSize = 1024 * (chunkNum + 1);
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

    constexpr int64_t mediaSize = 1024 * (chunkNum + 1);
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

    constexpr int64_t mediaSize = 1024 * (chunkNum + 1);
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isAppBackground_ = true;
    downloader->StopBufferring();
    EXPECT_NE(downloader->client_, nullptr);
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->currentRequest_->startPos_ = START_POS;
    downloader->isAppBackground_ = false;
    downloader->shouldStartNextRequest = false;
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
    auto saveData =  [this] (uint8_t*&& data, uint32_t&& len) {
        return true;
    };
    downloader->currentRequest_ = std::make_shared<DownloadRequest>(saveData, realStatusCallback, requestInfo);
    downloader->isAppBackground_ = false;
    downloader->shouldStartNextRequest = false;
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

}
}
}
}