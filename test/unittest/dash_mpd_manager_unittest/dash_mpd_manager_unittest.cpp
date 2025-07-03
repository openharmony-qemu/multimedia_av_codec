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

#include <iostream>
#include <algorithm>
#include "dash_mpd_manager_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing;
using namespace testing::ext;
const static int32_t ID_TEST = 1;
const static int32_t NUM_TEST = 0;
void DashMpdManagerUnittest::SetUpTestCase(void) {}
void DashMpdManagerUnittest::TearDownTestCase(void) {}
void DashMpdManagerUnittest::SetUp(void)
{
    mpdManager_ = std::make_shared<DashMpdManager>();
}
void DashMpdManagerUnittest::TearDown(void)
{
    mpdManager_ = nullptr;
}

/**
 * @tc.name  : Test Reset
 * @tc.number: Reset_001
 * @tc.desc  : Test all
 */
HWTEST_F(DashMpdManagerUnittest, Reset_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    auto mpdInfo = new DashMpdInfo();
    mpdManager_->mpdInfo_ = mpdInfo;
    mpdManager_->mpdUrl_ = "testUrl";
    ASSERT_NE(mpdManager_->mpdInfo_, nullptr);
    mpdManager_->Reset();
    EXPECT_EQ(mpdManager_->mpdInfo_, nullptr);
    EXPECT_EQ(mpdManager_->mpdUrl_, "");
    delete mpdInfo;
}

/**
 * @tc.name  : Test GetFirstPeriod
 * @tc.number: GetFirstPeriod_001
 * @tc.desc  : Test this->mpdInfo_ == nullptr || this->mpdInfo_->periodInfoList_.size() == 0
 */
HWTEST_F(DashMpdManagerUnittest, GetFirstPeriod_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    mpdManager_->mpdInfo_ = nullptr;
    auto ret = mpdManager_->GetFirstPeriod();
    EXPECT_EQ(ret, nullptr);
}

/**
 * @tc.name  : Test GetNextPeriod
 * @tc.number: GetNextPeriod_001
 * @tc.desc  : Test this->mpdInfo_ == nullptr || this->mpdInfo_->periodInfoList_.size() == 0
 *             Test periodInfoList_.size() != 0 && (*it) != period
 */
HWTEST_F(DashMpdManagerUnittest, GetNextPeriod_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    DashPeriodInfo *period;
    mpdManager_->mpdInfo_ = nullptr;
    auto ret = mpdManager_->GetNextPeriod(period);
    EXPECT_EQ(ret, nullptr);

    auto mpdInfo = new DashMpdInfo();
    mpdManager_->mpdInfo_ = mpdInfo;
    DashPeriodInfo *period2 = new DashPeriodInfo();
    mpdManager_->mpdInfo_->periodInfoList_.push_back(period2);
    ret = mpdManager_->GetNextPeriod(period);
    EXPECT_EQ(ret, nullptr);
    delete period2;
    delete mpdInfo;
    mpdManager_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetBaseUrlList
 * @tc.number: GetBaseUrlList_001
 * @tc.desc  : Test this->mpdInfo_ == nullptr || this->mpdInfo_->periodInfoList_.size() == 0
 *             Test periodInfoList_.size() != 0 && (*it) != period
 */
HWTEST_F(DashMpdManagerUnittest, GetBaseUrlList_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    mpdManager_->mpdInfo_ = nullptr;
    std::list<std::string> baseUrlList;
    mpdManager_->GetBaseUrlList(baseUrlList);

    auto mpdInfo = new DashMpdInfo();
    mpdManager_->mpdInfo_ = mpdInfo;
    mpdManager_->mpdUrl_ = "mpd/manager?testUrl";
    mpdManager_->GetBaseUrlList(baseUrlList);
    EXPECT_EQ(baseUrlList.empty(), false);
    EXPECT_EQ(mpdManager_->mpdInfo_->baseUrl_, baseUrlList);
    delete mpdInfo;
}

/**
 * @tc.name  : Test GetBaseUrl
 * @tc.number: GetBaseUrl_001
 * @tc.desc  : Test sepCharIndex != std::string::npos
 */
HWTEST_F(DashMpdManagerUnittest, GetBaseUrl_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    auto mpdInfo = new DashMpdInfo();
    mpdManager_->mpdInfo_ = mpdInfo;
    mpdManager_->mpdUrl_ = "mpd/manager?testUrl";
    auto ret = mpdManager_->GetBaseUrl();
    EXPECT_EQ(ret, "mpd/");
    delete mpdInfo;
    mpdManager_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test MakeBaseUrl
 * @tc.number: MakeBaseUrl_001
 * @tc.desc  : Test DashUrlIsAbsolute(baseUrl) == false
 *             Test baseUrl.find('/') == 0
 *             Test beginPathIndex != std::string::npos
 */
HWTEST_F(DashMpdManagerUnittest, MakeBaseUrl_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    auto mpdInfo = new DashMpdInfo();
    mpdManager_->mpdInfo_ = mpdInfo;
    mpdManager_->mpdInfo_->baseUrl_.push_back("test/Url");
    std::string mpdUrlBase = "test/Url";
    std::string urlSchem = "";
    mpdManager_->MakeBaseUrl(mpdUrlBase, urlSchem);
    EXPECT_EQ(mpdUrlBase, "test/Urltest/Url");
    delete mpdInfo;
    mpdManager_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test Update
 * @tc.number: Update_001
 * @tc.desc  : Test all
 */
HWTEST_F(DashMpdManagerUnittest, Update_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    DashMpdInfo *newMpdInfo = new DashMpdInfo();
    mpdManager_->Update(newMpdInfo);
    EXPECT_EQ(mpdManager_->mpdInfo_, newMpdInfo);
    delete newMpdInfo;
    mpdManager_->mpdInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetDuration
 * @tc.number: GetDuration_001
 * @tc.desc  : Test dur == 0
 */
HWTEST_F(DashMpdManagerUnittest, GetDuration_001, TestSize.Level0)
{
    ASSERT_NE(mpdManager_, nullptr);
    uint32_t *duration = nullptr;
    mpdManager_->GetDuration(duration);

    uint32_t testNum = ID_TEST;
    duration = &testNum;
    auto mpdInfo = new DashMpdInfo();
    mpdManager_->mpdInfo_ = mpdInfo;
    mpdManager_->mpdInfo_->mediaPresentationDuration_ = NUM_TEST;
    mpdManager_->GetDuration(duration);
    EXPECT_EQ(*duration, NUM_TEST);
    delete mpdInfo;
    mpdManager_->mpdInfo_ = nullptr;
}
}
}
}
}