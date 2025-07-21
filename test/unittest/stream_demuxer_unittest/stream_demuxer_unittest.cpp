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

#include "stream_demuxer_unittest.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;
namespace OHOS {
namespace Media {
const static int32_t ID_TEST = 1;
const static int32_t NUM_TEST = 0;
const static int32_t INVALID_TEST = 5;
constexpr uint64_t CONTENT_LENGTH = 2147483646;
// const static int32_t INVALID_MODE = 4;

void StreamDemuxerUnitTest::SetUpTestCase(void)
{
}

void StreamDemuxerUnitTest::TearDownTestCase(void)
{
}

void StreamDemuxerUnitTest::SetUp()
{
    streamDemuxer_ = std::make_shared<StreamDemuxer>();
}

void StreamDemuxerUnitTest::TearDown()
{
    streamDemuxer_ = nullptr;
}

/**
 * @tc.name: Test ReadFrameData  API
 * @tc.number: ReadFrameData_001
 * @tc.desc: Test (IsDash() || GetIsDataSrcNoSeek()) == true
 *           Test cacheDataMap_.find(streamID) != cacheDataMap_.end()
 *           && cacheDataMap_[streamID].CheckCacheExist(offset)
 *           Test memory != nullptr && memory->GetSize() > 0
 */
HWTEST_F(StreamDemuxerUnitTest, ReadFrameData_001, TestSize.Level0)
{
    ASSERT_NE(streamDemuxer_, nullptr);
    streamDemuxer_->isDash_ = true;
    streamDemuxer_->isDataSrcNoSeek_ = true;
    EXPECT_EQ(streamDemuxer_->IsDash(), true);
    EXPECT_EQ(streamDemuxer_->GetIsDataSrcNoSeek(), true);
    int32_t streamID = NUM_TEST;
    uint64_t offset = NUM_TEST;
    size_t size = NUM_TEST;
    auto mockTest = std::make_shared<MockBuffer>();
    EXPECT_CALL(*(mockTest), GetMemory()).WillRepeatedly(Return(nullptr));
    std::shared_ptr<Buffer> bufferPtr = mockTest;
    CacheData cacheDataTest;
    auto mockBuffer = std::make_shared<MockBuffer>();
    auto mockMemory = std::make_shared<MockMemory>(NUM_TEST);
    EXPECT_CALL(*(mockBuffer), GetMemory()).WillRepeatedly(Return(mockMemory));
    EXPECT_CALL(*(mockMemory), GetSize()).WillRepeatedly(Return(ID_TEST));
    cacheDataTest.data = mockBuffer;
    streamDemuxer_->cacheDataMap_[streamID] = cacheDataTest;
    auto ret = streamDemuxer_->ReadFrameData(streamID, offset, size, bufferPtr);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: Test PullDataWithCache  API
 * @tc.number: PullDataWithCache_001
 * @tc.desc: Test tempBuffer == nullptr
 */
HWTEST_F(StreamDemuxerUnitTest, PullDataWithCache_001, TestSize.Level0)
{
    ASSERT_NE(streamDemuxer_, nullptr);
    const uint8_t numTest = NUM_TEST;
    int32_t streamID = NUM_TEST;
    uint64_t offset = NUM_TEST;
    size_t size = ID_TEST;
    auto mockMemory1 = std::make_shared<MockMemory>(NUM_TEST);
    auto mockTest = std::make_shared<MockBuffer>();
    EXPECT_CALL(*(mockTest), GetMemory()).WillRepeatedly(Return(mockMemory1));
    EXPECT_CALL(*(mockMemory1), Write(_, _, _)).WillRepeatedly(Return(NUM_TEST));
    std::shared_ptr<Buffer> bufferPtr = mockTest;
    CacheData cacheDataTest;
    auto mockBuffer = std::make_shared<MockBuffer>();
    auto mockMemory2 = std::make_shared<MockMemory>(NUM_TEST);
    EXPECT_CALL(*(mockBuffer), GetMemory()).WillRepeatedly(Return(mockMemory2));
    EXPECT_CALL(*(mockMemory2), GetSize()).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(mockMemory2), GetReadOnlyData(_)).WillRepeatedly(Return(&numTest));
    cacheDataTest.data = mockBuffer;
    streamDemuxer_->cacheDataMap_[streamID] = cacheDataTest;
    auto ret = streamDemuxer_->PullDataWithCache(streamID, offset, size, bufferPtr);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: Test ProcInnerDash  API
 * @tc.number: ProcInnerDash_001
 * @tc.desc: Test tempBuffer == nullptr
 */
HWTEST_F(StreamDemuxerUnitTest, ProcInnerDash_001, TestSize.Level0)
{
    ASSERT_NE(streamDemuxer_, nullptr);
    streamDemuxer_->isDash_ = true;
    EXPECT_EQ(streamDemuxer_->IsDash(), true);
    int32_t streamID = NUM_TEST;
    uint64_t offset = NUM_TEST;
    auto mockTest = std::make_shared<MockBuffer>();
    EXPECT_CALL(*(mockTest), GetMemory()).WillRepeatedly(Return(nullptr));
    std::shared_ptr<Buffer> bufferPtr = mockTest;
    CacheData cacheDataTest;
    auto mockBuffer = nullptr;
    cacheDataTest.data = mockBuffer;
    streamDemuxer_->cacheDataMap_[streamID] = cacheDataTest;
    auto ret = streamDemuxer_->ProcInnerDash(streamID, offset, bufferPtr);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: Test PullData  API
 * @tc.number: PullData_001
 * @tc.desc: Test source_ == nullptr
 */
HWTEST_F(StreamDemuxerUnitTest, PullData_001, TestSize.Level0)
{
    ASSERT_NE(streamDemuxer_, nullptr);
    streamDemuxer_->source_ = nullptr;
    int32_t streamID = ID_TEST;
    uint64_t offset = ID_TEST;
    size_t size = ID_TEST;
    auto mockTest = std::make_shared<MockBuffer>();
    EXPECT_CALL(*(mockTest), GetMemory()).WillRepeatedly(Return(nullptr));
    std::shared_ptr<Buffer> bufferPtr = mockTest;
    bufferPtr->streamID = ID_TEST;
    auto ret = streamDemuxer_->PullData(streamID, offset, size, bufferPtr);
    EXPECT_EQ(ret, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: Test HandleReadHeader  API
 * @tc.number: HandleReadHeader_001
 * @tc.desc: Test ret != Status::OK
 *           Test (offset + readSize) > totalSize
 */
HWTEST_F(StreamDemuxerUnitTest, HandleReadHeader_001, TestSize.Level0)
{
    ASSERT_NE(streamDemuxer_, nullptr);
    streamDemuxer_->isDash_ = false;
    int32_t streamID = ID_TEST;
    uint64_t offset = ID_TEST;
    size_t expectedLen = ID_TEST;
    std::shared_ptr<Buffer> bufferPtr = nullptr;
    streamDemuxer_->getRange_ = [](int32_t param1, uint64_t param2, size_t param3, std::shared_ptr<Buffer>& buffer) {
        return Status::END_OF_STREAM;
    };
    streamDemuxer_->mediaDataSize_ = CONTENT_LENGTH;
    auto ret = streamDemuxer_->HandleReadHeader(streamID, offset, bufferPtr, expectedLen);
    EXPECT_EQ(ret, Status::OK);
    streamDemuxer_->mediaDataSize_ = ID_TEST;
    ret = streamDemuxer_->HandleReadHeader(streamID, offset, bufferPtr, expectedLen);
    EXPECT_EQ(ret, Status::END_OF_STREAM);
}

/**
 * @tc.name: Test HandleReadPacket  API
 * @tc.number: HandleReadPacket_001
 * @tc.desc: Test ret == Status::OK
 *           Test buffer != nullptr && buffer->GetMemory() != nullptr &&
 *           buffer->GetMemory()->GetSize() == 0
 */
HWTEST_F(StreamDemuxerUnitTest, HandleReadPacket_001, TestSize.Level0)
{
    ASSERT_NE(streamDemuxer_, nullptr);
    streamDemuxer_->isDash_ = false;
    int32_t streamID = ID_TEST;
    uint64_t offset = ID_TEST;
    size_t expectedLen = ID_TEST;
    streamDemuxer_->getRange_ = [](int32_t param1, uint64_t param2, size_t param3, std::shared_ptr<Buffer>& buffer) {
        return Status::OK;
    };
    streamDemuxer_->mediaDataSize_ = CONTENT_LENGTH;
    auto mockMemory1 = std::make_shared<MockMemory>(NUM_TEST);
    auto mockTest = std::make_shared<MockBuffer>();
    EXPECT_CALL(*(mockTest), GetMemory()).WillRepeatedly(Return(mockMemory1));
    EXPECT_CALL(*(mockMemory1), GetSize()).WillRepeatedly(Return(NUM_TEST));
    std::shared_ptr<Buffer> bufferPtr = mockTest;
    streamDemuxer_->HandleReadPacket(streamID, offset, bufferPtr, expectedLen);
    EXPECT_EQ(bufferPtr->GetMemory()->GetSize(), 0);
}

/**
 * @tc.name: Test CallbackReadAt  API
 * @tc.number: CallbackReadAt_001
 * @tc.desc: Test case default
 */
HWTEST_F(StreamDemuxerUnitTest, CallbackReadAt_001, TestSize.Level0)
{
    ASSERT_NE(streamDemuxer_, nullptr);
    streamDemuxer_->isInterruptNeeded_.store(false);
    int32_t streamID = NUM_TEST;
    uint64_t offset = ID_TEST;
    size_t expectedLen = ID_TEST;
    std::shared_ptr<Buffer> bufferPtr = nullptr;
    streamDemuxer_->pluginStateMap_[0] = static_cast<DemuxerState>(INVALID_TEST);
    auto ret = streamDemuxer_->CallbackReadAt(streamID, offset, bufferPtr, expectedLen);
    EXPECT_EQ(ret, Status::OK);
}
} // Media
} // OHOS