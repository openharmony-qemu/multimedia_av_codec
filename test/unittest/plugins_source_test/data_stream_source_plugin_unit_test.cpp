/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "data_stream_source_plugin_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugin {
namespace DataStreamSource {
constexpr size_t DEFAULT_PREDOWNLOAD_SIZE_BYTE = 10 * 1024 * 1024;

void DataStreamSourceUnitTest::SetUpTestCase(void)
{
}

void DataStreamSourceUnitTest::TearDownTestCase(void)
{
}

void DataStreamSourceUnitTest::SetUp(void)
{
    plugin_ = std::make_shared<DataStreamSourcePlugin>("testPlugin");
    mockDataSrc_ = std::make_shared<MockMediaDataSource>();
    plugin_->dataSrc_ = mockDataSrc_;
}

void DataStreamSourceUnitTest::TearDown(void)
{
    plugin_ = nullptr;
    mockDataSrc_ = nullptr;
}


/**
* @tc.name  : Read_001
* @tc.number: 1
* @tc.desc  : test isInterrupted_ true
*/
HWTEST_F(DataStreamSourceUnitTest, Read_001, TestSize.Level1)
{
    plugin_->isInterrupted_ = true;
    std::shared_ptr<Plugins::Buffer> buffer;
    uint64_t offset = 0;
    size_t expectedLen = 1024;

    Status result = plugin_->Read(buffer, offset, expectedLen);

    EXPECT_EQ(result, Status::OK);
}

/**
* @tc.name  : Read_002
* @tc.number: 2
* @tc.desc  : Test isInterrupted_ false isExitRead_ true
*/
HWTEST_F(DataStreamSourceUnitTest, Read_002, TestSize.Level1)
{
    plugin_->isInterrupted_ = false;
    plugin_->isExitRead_ = true;
    uint64_t offset = 0;
    size_t expectedLen = 1024;
    std::shared_ptr<Plugins::Buffer> buffer = Plugins::Buffer::CreateDefaultBuffer(expectedLen);
    Status result = plugin_->Read(buffer, offset, expectedLen);

    EXPECT_EQ(result, Status::OK);
}

/**
* @tc.name  : Read_003
* @tc.number: 3
* @tc.desc  : Test seekable is true
*/
HWTEST_F(DataStreamSourceUnitTest, Read_003, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::SEEKABLE;
    plugin_->offset_ = 0;
    plugin_->size_ = 2048;
    uint64_t offset = 0;
    size_t expectedLen = 1024;
    std::shared_ptr<Plugins::Buffer> buffer = Plugins::Buffer::CreateDefaultBuffer(expectedLen);
    EXPECT_CALL(*mockDataSrc_, ReadAt(0, 1024, testing::_))
        .Times(1)
        .WillOnce(testing::Return(1024));

    Status result = plugin_->Read(buffer, offset, expectedLen);

    EXPECT_EQ(result, Status::OK);
}

/**
* @tc.name  : Read_004
* @tc.number: 4
* @tc.desc  : Test seekable is false
*/
HWTEST_F(DataStreamSourceUnitTest, Read_004, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::UNSEEKABLE;
    plugin_->offset_ = 0;
    plugin_->size_ = 2048;
    uint64_t offset = 0;
    size_t expectedLen = 1024;
    std::shared_ptr<Plugins::Buffer> buffer = Plugins::Buffer::CreateDefaultBuffer(expectedLen);
    EXPECT_CALL(*mockDataSrc_, ReadAt(1024, testing::_))
        .Times(1)
        .WillOnce(testing::Return(1024));

    Status result = plugin_->Read(buffer, offset, expectedLen);

    EXPECT_EQ(result, Status::OK);
}

/**
* @tc.name  : Read_005
* @tc.number: 5
* @tc.desc  : Test realLen is zero
*/
HWTEST_F(DataStreamSourceUnitTest, Read_005, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::SEEKABLE;
    plugin_->offset_ = 0;
    plugin_->size_ = 2048;
    uint64_t offset = 0;
    size_t expectedLen = 1024;
    std::shared_ptr<Plugins::Buffer> buffer = Plugins::Buffer::CreateDefaultBuffer(expectedLen);
    EXPECT_CALL(*mockDataSrc_, ReadAt(0, 1024, testing::_))
        .WillOnce(testing::Return(0))
        .WillOnce(testing::Return(1024));

    Status result = plugin_->Read(buffer, offset, expectedLen);

    EXPECT_EQ(result, Status::OK);
}

/**
* @tc.name  : Read_006
* @tc.number: 6
* @tc.desc  : Test buffer is nullptr
*/
HWTEST_F(DataStreamSourceUnitTest, Read_006, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::UNSEEKABLE;
    plugin_->offset_ = 0;
    plugin_->size_ = 2048;
    uint64_t offset = 0;
    size_t expectedLen = 1024;
    std::shared_ptr<Plugins::Buffer> buffer = nullptr;
    EXPECT_CALL(*mockDataSrc_, ReadAt(1024, testing::_))
        .Times(1)
        .WillOnce(testing::Return(1024));

    Status result = plugin_->Read(buffer, offset, expectedLen);

    EXPECT_EQ(result, Status::OK);
}

/**
* @tc.name  : HandleBufferingStart_001
* @tc.number: 1
* @tc.desc  : isBufferingStart is false, callback_ is nullptr
*/
HWTEST_F(DataStreamSourceUnitTest, HandleBufferingStart_001, TestSize.Level1)
{
    plugin_->isBufferingStart = false;
    plugin_->callback_.reset();

    plugin_->HandleBufferingStart();
    EXPECT_TRUE(plugin_->isBufferingStart);
}

/**
* @tc.name  : HandleBufferingStart_002
* @tc.number: 2
* @tc.desc  : isBufferingStart is false, callback_ is not nullptr
*/
HWTEST_F(DataStreamSourceUnitTest, HandleBufferingStart_002, TestSize.Level1)
{
    plugin_->isBufferingStart = false;
    auto callback = std::make_shared<TestCallback>();
    plugin_->callback_ = callback;

    plugin_->HandleBufferingStart();
    EXPECT_TRUE(plugin_->isBufferingStart);
}

/**
* @tc.name  : HandleBufferingStart_003
* @tc.number: 3
* @tc.desc  : isBufferingStart is true
*/
HWTEST_F(DataStreamSourceUnitTest, HandleBufferingStart_003, TestSize.Level1)
{
    plugin_->isBufferingStart = true;
    plugin_->HandleBufferingStart();
    EXPECT_TRUE(plugin_->isBufferingStart);
}


/**
* @tc.name  : HandleBufferingEnd_001
* @tc.number: 1
* @tc.desc  : isBufferingStart is true, callback_ is nullptr
*/
HWTEST_F(DataStreamSourceUnitTest, HandleBufferingEnd_001, TestSize.Level1)
{
    plugin_->isBufferingStart = true;
    plugin_->callback_.reset();

    plugin_->HandleBufferingEnd();
    EXPECT_FALSE(plugin_->isBufferingStart);
}

/**
* @tc.name  : HandleBufferingEnd_002
* @tc.number: 2
* @tc.desc  : isBufferingStart is true, callback_ is not nullptr
*/
HWTEST_F(DataStreamSourceUnitTest, HandleBufferingEnd_002, TestSize.Level1)
{
    plugin_->isBufferingStart = true;
    auto callback = std::make_shared<TestCallback>();
    plugin_->callback_ = callback;

    plugin_->HandleBufferingEnd();
    EXPECT_FALSE(plugin_->isBufferingStart);
}

/**
* @tc.name  : HandleBufferingEnd_003
* @tc.number: 3
* @tc.desc  : isBufferingStart is false
*/
HWTEST_F(DataStreamSourceUnitTest, HandleBufferingEnd_003, TestSize.Level1)
{
    plugin_->isBufferingStart = false;
    plugin_->HandleBufferingEnd();
    EXPECT_FALSE(plugin_->isBufferingStart);
}

/**
* @tc.name  : GetSize_001
* @tc.number: 1
* @tc.desc  : Test GetSize function when seekable_ is SEEKABLE.
*/
HWTEST_F(DataStreamSourceUnitTest, GetSize_001, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::SEEKABLE;
    plugin_->size_ = 100;
    uint64_t size = 0;
    EXPECT_EQ(plugin_->GetSize(size), Status::OK);
    EXPECT_EQ(size, 100);
}

/**
* @tc.name  : GetSize_002
* @tc.number: 2
* @tc.desc  : Test GetSize function when seekable_ is not SEEKABLE.
*/
HWTEST_F(DataStreamSourceUnitTest, GetSize_002, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::UNSEEKABLE;
    plugin_->offset_ = 50;
    uint64_t size = 0;
    EXPECT_EQ(plugin_->GetSize(size), Status::OK);
    EXPECT_EQ(size, DEFAULT_PREDOWNLOAD_SIZE_BYTE);
}

/**
* @tc.name  : SeekTo_001
* @tc.number: 1
* @tc.desc  : Test SeekTo function when source is unseekable
*/
HWTEST_F(DataStreamSourceUnitTest, SeekTo_001, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::UNSEEKABLE;
    EXPECT_EQ(plugin_->SeekTo(0), Status::ERROR_INVALID_OPERATION);
}

/**
* @tc.name  : SeekTo_002
* @tc.number: 2
* @tc.desc  : Test SeekTo function when offset is invalid
*/
HWTEST_F(DataStreamSourceUnitTest, SeekTo_002, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::SEEKABLE;
    plugin_->size_ = 100;
    EXPECT_EQ(plugin_->SeekTo(101), Status::ERROR_INVALID_PARAMETER);
}

/**
* @tc.name  : SeekTo_003
* @tc.number: 3
* @tc.desc  : Test SeekTo function when offset is valid
*/
HWTEST_F(DataStreamSourceUnitTest, SeekTo_003, TestSize.Level1)
{
    plugin_->seekable_ = Plugins::Seekable::SEEKABLE;
    plugin_->size_ = 100;
    EXPECT_EQ(plugin_->SeekTo(50), Status::OK);
    EXPECT_EQ(plugin_->offset_, 50);
    EXPECT_FALSE(plugin_->isExitRead_);
}
} // namespace DataStreamSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS