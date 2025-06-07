/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "media_sync_manager_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

void MediaSyncManagerUnitTest::SetUpTestCase(void)
{
}

void MediaSyncManagerUnitTest::TearDownTestCase(void)
{
}

void MediaSyncManagerUnitTest::SetUp(void)
{
    manager_ = std::make_shared<MediaSyncManager>();
}

void MediaSyncManagerUnitTest::TearDown(void)
{
    manager_ = nullptr;
}

/**
 * @tc.name  : Test GetLastVideoBufferAbsPts
 * @tc.number: GetLastVideoBufferAbsPts_001
 * @tc.desc  : Test SetInitialVideoFrameRate
 *             Test ReportEos
 *             Test GetLastVideoBufferAbsPts
 */
HWTEST_F(MediaSyncManagerUnitTest, GetLastVideoBufferAbsPts_001, TestSize.Level0)
{
    ASSERT_NE(manager_, nullptr);
    // Test SetInitialVideoFrameRate
    manager_->SetInitialVideoFrameRate(1);
    // Test ReportEos
    manager_->ReportEos(nullptr);
    // Test GetLastVideoBufferAbsPts
    auto ret = manager_->GetLastVideoBufferAbsPts();
    EXPECT_EQ(ret, HST_TIME_NONE);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS