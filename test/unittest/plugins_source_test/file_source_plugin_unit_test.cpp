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
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "file_source_plugin_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileSource {
const int64_t DURATION = 1000000; // 1s
void OHOS::Media::Plugins::FileSource::FileSourceUnitTest::SetUpTestCase(void)
{
}

void FileSourceUnitTest::TearDownTestCase(void)
{
}

void FileSourceUnitTest::SetUp(void)
{
    fileSourcePlugin_ = std::make_shared<FileSourcePlugin>("SourceTest");
}

void FileSourceUnitTest::TearDown(void)
{
    fileSourcePlugin_ = nullptr;
}

/**
 * @tc.name  : Alloc_001
 * @tc.number: Alloc_001
 * @tc.desc  : Test when size is zero, Alloc should return nullptr
 */
HWTEST_F(FileSourceUnitTest, Alloc_001, TestSize.Level1)
{
    FileSourceAllocator allocator;
    void* result = allocator.Alloc(0);
    EXPECT_EQ(result, nullptr);
}

/**
 * @tc.name  : Alloc_002
 * @tc.number: AllocTest_002
 * @tc.desc  : Test when size is non-zero, Alloc should return valid memory
 */
HWTEST_F(FileSourceUnitTest, Alloc_002, TestSize.Level1)
{
    FileSourceAllocator allocator;
    void* result = allocator.Alloc(10);
    EXPECT_NE(result, nullptr);
    delete[] static_cast<uint8_t*>(result);
}

/**
 * @tc.name: FileSource_Prepare_0100
 * @tc.desc: FileSource_Prepare_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_Prepare_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Prepare());
}
/**
 * @tc.name: FileSource_Reset_0100
 * @tc.desc: FileSource_Reset_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_Reset_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Reset());
}
/**
 * @tc.name: FileSource_GetParameter_0100
 * @tc.desc: FileSource_GetParameter_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_GetParameter_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    auto ret = fileSourcePlugin_->GetParameter(meta);
    EXPECT_EQ(Status::ERROR_UNIMPLEMENTED, ret);
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileSource_GetAllocator_0100
 * @tc.desc: FileSource_GetAllocator_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_GetAllocator_0100, TestSize.Level1)
{
    auto allocator = fileSourcePlugin_->GetAllocator();
    EXPECT_EQ(nullptr, allocator);
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileSource_SetSource_0100
 * @tc.desc: FileSource_SetSource_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_SetSource_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>("file:////");
    EXPECT_NE(Status::OK, fileSourcePlugin_->SetSource(source));
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileSource_ParseFileName_0100
 * @tc.desc: FileSource_ParseFileName_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_ParseFileName_0100, TestSize.Level1)
{
    std::string uri = "";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    EXPECT_NE(Status::OK, fileSourcePlugin_->SetSource(source));
    uri = "file://#test";
    source = std::make_shared<MediaSource>(uri);
    EXPECT_NE(Status::OK, fileSourcePlugin_->SetSource(source));
    uri = "file://#";
    source = std::make_shared<MediaSource>(uri);
    EXPECT_NE(Status::OK, fileSourcePlugin_->SetSource(source));
    uri = "file://#";
    source = std::make_shared<MediaSource>(uri);
    EXPECT_NE(Status::OK, fileSourcePlugin_->SetSource(source));
    uri = "file:///////////#";
    source = std::make_shared<MediaSource>(uri);
    EXPECT_NE(Status::OK, fileSourcePlugin_->SetSource(source));
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}

/**
 * @tc.name: FileSource_IsLocalFd_0100
 * @tc.desc: FileSource_IsLocalFd_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_IsLocalFd_0100, TestSize.Level1)
{
    ASSERT_TRUE(fileSourcePlugin_->IsLocalFd());
}

/**
 * @tc.name: FileSource_GetSeekableRanges_0100
 * @tc.desc: Test GetSeekableRanges when seekable_ is not SEEKABLE
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_GetSeekableRanges_0100, TestSize.Level1)
{
    fileSourcePlugin_->seekable_ = Seekable::UNSEEKABLE;
    auto ranges = fileSourcePlugin_->GetSeekableRanges();
    ASSERT_TRUE(ranges.empty());
}

/**
 * @tc.name: FileSource_GetSeekableRanges_0200
 * @tc.desc: Test GetSeekableRanges when durationUs_ is 0
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_GetSeekableRanges_0200, TestSize.Level1)
{
    fileSourcePlugin_->seekable_ = Seekable::SEEKABLE;
    fileSourcePlugin_->durationUs_ = 0;
    auto ranges = fileSourcePlugin_->GetSeekableRanges();
    ASSERT_TRUE(ranges.empty());
}

/**
 * @tc.name: FileSource_GetSeekableRanges_0300
 * @tc.desc: Test GetSeekableRanges when durationUs_ is negative
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_GetSeekableRanges_0300, TestSize.Level1)
{
    fileSourcePlugin_->seekable_ = Seekable::SEEKABLE;
    fileSourcePlugin_->durationUs_ = -1;
    auto ranges = fileSourcePlugin_->GetSeekableRanges();
    ASSERT_TRUE(ranges.empty());
}

/**
 * @tc.name: FileSource_GetSeekableRanges_0400
 * @tc.desc: Test GetSeekableRanges success case
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_GetSeekableRanges_0400, TestSize.Level1)
{
    fileSourcePlugin_->seekable_ = Seekable::SEEKABLE;
    fileSourcePlugin_->durationUs_ = 1000000;
    auto ranges = fileSourcePlugin_->GetSeekableRanges();
    ASSERT_EQ(ranges.size(), 1);
    ASSERT_EQ(ranges[0].start, 0);
    ASSERT_GT(ranges[0].end, 0);
}

/**
 * @tc.name: FileSource_SetMediaDuration_0100
 * @tc.desc: Test SetMediaDuration when seekable_ is not SEEKABLE
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_SetMediaDuration_0100, TestSize.Level1)
{
    ASSERT_EQ(Status::OK, fileSourcePlugin_->SetMediaDuration(DURATION));
    ASSERT_EQ(DURATION, fileSourcePlugin_->durationUs_);
}
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS