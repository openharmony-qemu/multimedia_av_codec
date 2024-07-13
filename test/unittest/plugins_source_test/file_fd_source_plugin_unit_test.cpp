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
#include "file_fd_source_plugin_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace FileFdSource {
const std::string MEDIA_ROOT = "file:///data/test/media/";
const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
void OHOS::Media::Plugins::FileFdSource::FileFdSourceUnitTest::SetUpTestCase(void)
{
}

void FileFdSourceUnitTest::TearDownTestCase(void)
{
}

void FileFdSourceUnitTest::SetUp(void)
{
    fileFdSourcePlugin_ = std::make_shared<FileFdSourcePlugin>("testPlugin");
}

void FileFdSourceUnitTest::TearDown(void)
{
    fileFdSourcePlugin_ = nullptr;
}

class SourceCallback : public Plugins::Callback {
public:
    void OnEvent(const Plugins::PluginEvent &event) override
    {
        (void)event;
    }

    void SetSelectBitRateFlag(bool flag) override
    {
        (void)flag;
    }

    bool CanDoSelectBitRate() override
    {
        return true;
    }
};

/**
 * @tc.name: FileFdSource_SetSource_0100
 * @tc.desc: SetSource
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetSource_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->SetSource(mediaSource));
}
/**
 * @tc.name: FileFdSource_SubmitBufferingStart_0100
 * @tc.desc: FileFdSource_SubmitBufferingStart_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SubmitBufferingStart_0100, TestSize.Level1)
{
    Plugins::Callback* sourceCallback = new SourceCallback();
    fileFdSourcePlugin_->SubmitBufferingStart();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->SubmitBufferingStart();

    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    fileFdSourcePlugin_->SubmitBufferingStart();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
}
/**
 * @tc.name: FileFdSource_SubmitReadFail_0100
 * @tc.desc: FileFdSource_SubmitReadFail_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SubmitReadFail_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->SubmitReadFail();
    Plugins::Callback* sourceCallback = new SourceCallback();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->SetCallback(sourceCallback));
    fileFdSourcePlugin_->SubmitReadFail();
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
    fileFdSourcePlugin_->SubmitReadFail();
}
/**
 * @tc.name: FileFdSource_read_0100
 * @tc.desc: FileFdSource_read_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->SubmitBufferingStart();
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_read_0200
 * @tc.desc: FileFdSource_read_0200
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_read_0200, TestSize.Level1)
{
    std::shared_ptr<Buffer> buffer = nullptr;
    EXPECT_NE(Status::OK, fileFdSourcePlugin_->Read(buffer, 0, 1024));
}
/**
 * @tc.name: FileFdSource_ParseUriInfo_0100
 * @tc.desc: FileFdSource_ParseUriInfo_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_ParseUriInfo_0100, TestSize.Level1)
{
    std::string uri = "";
    std::shared_ptr<MediaSource> source = std::make_shared<MediaSource>(uri);
    fileFdSourcePlugin_->SetSource(source);
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Reset());
}
/**
 * @tc.name: FileFdSource_Reset_0100
 * @tc.desc: FileFdSource_Reset_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_Reset_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Reset());
}
/**
 * @tc.name: FileFdSource_SetBundleName_0100
 * @tc.desc: FileFdSource_SetBundleName_0100
 * @tc.type: FUNC
 */
HWTEST_F(FileFdSourceUnitTest, FileFdSource_SetBundleName_0100, TestSize.Level1)
{
    fileFdSourcePlugin_->Stop();
    fileFdSourcePlugin_->SetBundleName("TestFileFdSource");
    EXPECT_EQ(Status::OK, fileFdSourcePlugin_->Stop());
}
} // namespace FileSource
} // namespace Plugins
} // namespace Media
} // namespace OHOS