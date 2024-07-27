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
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "source_unit_test.h"

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
const std::string MEDIA_ROOT = "file:///data/test/media/";
const std::string VIDEO_FILE1 = MEDIA_ROOT + "camera_info_parser.mp4";
const std::string BUNDLE_NAME_FIRST = "com";
const std::string BUNDLE_NAME_SECOND = "wei.hmos.photos";
void SourceUnitTest::SetUpTestCase(void)
{
}

void SourceUnitTest::TearDownTestCase(void)
{
}

void SourceUnitTest::SetUp(void)
{
    source_ = std::make_shared<Source>();
}

void SourceUnitTest::TearDown(void)
{
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

    bool CanAutoSelectBitRate() override
    {
        return true;
    }
};
/**
 * @tc.name: Source_SetCallback_0100
 * @tc.desc: Set callback
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SetCallback_0100, TestSize.Level1)
{
    source_->SetCallback(nullptr);
}
/**
 * @tc.name: Source_SetBundleName_0100
 * @tc.desc: Set callback
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SetBundleName_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    source_->SetBundleName("TEST_SOURCE");
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_SetBundleName_0200
 * @tc.desc: Set callback
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SetBundleName_0200, TestSize.Level1)
{
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    source_->SetBundleName(BUNDLE_NAME_FIRST+BUNDLE_NAME_SECOND);
    EXPECT_EQ(Status::OK, source_->Prepare());
    EXPECT_EQ(Status::OK, source_->Start());
    uint32_t time = 1; // 1 sleep
    sleep(time);
    std::shared_ptr<Buffer> buffer = std::make_shared<Buffer>();
    size_t expectedLen = 1024; // 1024 expectedLen
    source_->Read(0, buffer, 0, expectedLen);
    sleep(time);
    EXPECT_EQ(Status::OK, source_->Pause());
    sleep(time);
    EXPECT_EQ(Status::OK, source_->Resume());
    sleep(time);
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_SetBundleName_0300
 * @tc.desc: Source_SetBundleName_0300
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SetBundleName_0300, TestSize.Level1)
{
    source_->SetBundleName("testSource");
}
/**
 * @tc.name: Source_Prepare_0100
 * @tc.desc: Source_Prepare_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_Prepare_0100, TestSize.Level1)
{
    source_->Prepare();
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    source_->Prepare();
    EXPECT_EQ(Status::OK, source_->Stop());
}

/**
 * @tc.name: Source_Start_0100
 * @tc.desc: Source_Start_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_Start_0100, TestSize.Level1)
{
    EXPECT_NE(Status::OK, source_->Start());
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    EXPECT_EQ(Status::OK, source_->Start());
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_GetBitRate_0100
 * @tc.desc: Source_GetBitRate_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_GetBitRate_0100, TestSize.Level1)
{
    std::vector<uint32_t> bitrate;
    EXPECT_NE(Status::OK, source_->GetBitRates(bitrate));
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    EXPECT_EQ(Status::OK, source_->Prepare());
    EXPECT_EQ(Status::OK, source_->GetBitRates(bitrate));
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_SelectBitRate_0100
 * @tc.desc: Source_SelectBitRate_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SelectBitRate_0100, TestSize.Level1)
{
    uint32_t bitrate = 0;
    EXPECT_NE(Status::OK, source_->SelectBitRate(bitrate));
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    EXPECT_EQ(Status::OK, source_->Prepare());
    EXPECT_EQ(Status::OK, source_->SelectBitRate(bitrate));
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_GetDownloadInfo_0100
 * @tc.desc: Source_GetDownloadInfo_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_GetDownloadInfo_0100, TestSize.Level1)
{
    DownloadInfo downloadInfo;
    EXPECT_NE(Status::OK, source_->GetDownloadInfo(downloadInfo));
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    EXPECT_EQ(Status::OK, source_->Prepare());
    source_->GetDownloadInfo(downloadInfo);
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_IsNeedPreDownload_0100
 * @tc.desc: Source_IsNeedPreDownload_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_IsNeedPreDownload_0100, TestSize.Level1)
{
    EXPECT_NE(true, source_->IsNeedPreDownload());
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    EXPECT_EQ(Status::OK, source_->Prepare());
    source_->IsNeedPreDownload();
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_Pause_0100
 * @tc.desc: Source_Pause_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_Pause_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, source_->Pause());
}
/**
 * @tc.name: Source_Resume_0100
 * @tc.desc: Source_Resume_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_Resume_0100, TestSize.Level1)
{
    EXPECT_EQ(Status::OK, source_->Resume());
}
/**
 * @tc.name: Source_SetReadBlockingFlag_0100
 * @tc.desc: Source_SetReadBlockingFlag_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SetReadBlockingFlag_0100, TestSize.Level1)
{
    bool isReadBlockingAllowed = false;
    EXPECT_EQ(Status::OK, source_->SetReadBlockingFlag(isReadBlockingAllowed));
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    EXPECT_EQ(Status::OK, source_->Prepare());
    source_->SetReadBlockingFlag(isReadBlockingAllowed);
    EXPECT_EQ(Status::OK, source_->Stop());
}
/**
 * @tc.name: Source_GetDuration_0100
 * @tc.desc: Source_GetDuration_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_GetDuration_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(VIDEO_FILE1);
    EXPECT_EQ(Status::OK, source_->SetSource(mediaSource));
    EXPECT_EQ(Status::OK, source_->Prepare());
    source_->GetDuration();
    EXPECT_EQ(Status::OK, source_->Stop());
}
} // namespace Media
} // namespace OHOS
