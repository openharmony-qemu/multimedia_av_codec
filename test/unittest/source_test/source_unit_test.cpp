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
#include "source_unit_test.h"

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

namespace OHOS {
namespace Media {
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

/**
 * @tc.name: Source_SetCallback_0100
 * @tc.desc: Set callback
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SetCallback_0100, TestSize.Level1)
{
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>();
    source_->SetSource(mediaSource);
}
/**
 * @tc.name: Source_SetBundleName_0100
 * @tc.desc: Source_SetBundleName_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SetBundleName_0100, TestSize.Level1)
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
}
/**
 * @tc.name: Source_Start_0100
 * @tc.desc: Source_Start_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_Start_0100, TestSize.Level1)
{
    EXPECT_NE(Status::OK, source_->Start());
}
/**
 * @tc.name: Source_GetBitRate_0100
 * @tc.desc: Source_GetBitRate_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_GetBitRate_0100, TestSize.Level1)
{
    std::vector<uint32_t> bitrate;
    EXPECT_NE(Status::OK, source_->GetBitRate(bitrate));
}
/**
 * @tc.name: Source_SelectBitRate_0100
 * @tc.desc: Source_SelectBitRate_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SelectBitRate_0100, TestSize.Level1)
{
    uint32_t bitrate;
    EXPECT_NE(Status::OK, source_->SelectBitRate(bitrate));
}
/**
 * @tc.name: Source_SeekToTime_0100
 * @tc.desc: Source_SeekToTime_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_SeekToTime_0100, TestSize.Level1)
{
    int64_t seekTime = 10000000;
    EXPECT_NE(Status::OK, source_->SeekToTime(SeekTime, Plugins::SeekMode::SEEK_NEXT_SYNC));
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
}
/**
 * @tc.name: Source_IsNeedPreDownload_0100
 * @tc.desc: Source_IsNeedPreDownload_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_IsNeedPreDownload_0100, TestSize.Level1)
{
    DownloadInfo downloadInfo;
    EXPECT_NE(true, source_->IsNeedPreDownload());
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
    EXPECT_NE(Status::OK, source_->SetReadBlockingFlag());
}
/**
 * @tc.name: Source_GetDuration_0100
 * @tc.desc: Source_GetDuration_0100
 * @tc.type: FUNC
 */
HWTEST_F(SourceUnitTest, Source_GetDuration_0100, TestSize.Level1)
{
    EXPECT_NE(Status::OK, source_->GetDuration());
}
} // namespace Media
} // namespace OHOS
