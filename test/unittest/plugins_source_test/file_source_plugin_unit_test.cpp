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
    fileSourcePlugin_->GetParameter(meta);
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
/**
 * @tc.name: FileSource_GetAllocator_0100
 * @tc.desc: FileSource_GetAllocator_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileSourceUnitTest, FileSource_GetAllocator_0100, TestSize.Level1)
{
    fileSourcePlugin_->GetAllocator();
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
    fileSourcePlugin_->SetSource(source);
    uri = "file://#test";
    source = std::make_shared<MediaSource>(uri);
    fileSourcePlugin_->SetSource(source);
    uri = "file://#";
    source = std::make_shared<MediaSource>(uri);
    fileSourcePlugin_->SetSource(source);
    uri = "file://#";
    source = std::make_shared<MediaSource>(uri);
    fileSourcePlugin_->SetSource(source);
    uri = "file:///////////#";
    source = std::make_shared<MediaSource>(uri);
    fileSourcePlugin_->SetSource(source);
    EXPECT_EQ(Status::OK, fileSourcePlugin_->Deinit());
}
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS