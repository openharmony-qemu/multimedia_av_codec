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

#include "dash_mpd_node_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace std;
using namespace testing;
using namespace testing::ext;
static const int32_t NUM_0 = 0;
static const int32_t NUM_1 = 1;

void DashMpdNodeUnitTest::SetUpTestCase(void) {}

void DashMpdNodeUnitTest::TearDownTestCase(void) {}

void DashMpdNodeUnitTest::SetUp(void)
{
    dashMpdNode_ = std::make_shared<DashMpdNode>();
}

void DashMpdNodeUnitTest::TearDown(void)
{
    dashMpdNode_ = nullptr;
}

/**
 * @tc.name  : Test GetAttr(string, uint32_t)
 * @tc.number: GetAttr_001
 * @tc.desc  : Test mpdAttr_[index].val_.length() > 0
 */
HWTEST_F(DashMpdNodeUnitTest, GetAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashMpdNode_, nullptr);
    std::string attrName = "id";
    uint32_t uiAttrVal;
    dashMpdNode_->mpdAttr_[NUM_0].val_ = "1";
    dashMpdNode_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, NUM_1);
}

/**
 * @tc.name  : Test GetAttr(string, int32_t)
 * @tc.number: GetAttr_002
 * @tc.desc  : Test mpdAttr_[index].val_.length() > 0
 */
HWTEST_F(DashMpdNodeUnitTest, GetAttr_002, TestSize.Level0)
{
    ASSERT_NE(dashMpdNode_, nullptr);
    std::string attrName = "id";
    int32_t iAttrVal;
    dashMpdNode_->mpdAttr_[NUM_0].val_ = "1";
    dashMpdNode_->GetAttr(attrName, iAttrVal);
    EXPECT_EQ(iAttrVal, NUM_1);
}

/**
 * @tc.name  : Test GetAttr(string, uint64_t)
 * @tc.number: GetAttr_003
 * @tc.desc  : Test mpdAttr_[index].val_.length() > 0
 */
HWTEST_F(DashMpdNodeUnitTest, GetAttr_003, TestSize.Level0)
{
    ASSERT_NE(dashMpdNode_, nullptr);
    std::string attrName = "id";
    uint64_t ullAttrVal;
    dashMpdNode_->mpdAttr_[NUM_0].val_ = "1";
    dashMpdNode_->GetAttr(attrName, ullAttrVal);
    EXPECT_EQ(ullAttrVal, NUM_1);
}

/**
 * @tc.name  : Test GetAttr(string, double)
 * @tc.number: GetAttr_004
 * @tc.desc  : Test mpdAttr_[index].val_.length() > 0
 */
HWTEST_F(DashMpdNodeUnitTest, GetAttr_004, TestSize.Level0)
{
    ASSERT_NE(dashMpdNode_, nullptr);
    std::string attrName = "id";
    double dAttrVal;
    dashMpdNode_->mpdAttr_[NUM_0].val_ = "1";
    dashMpdNode_->GetAttr(attrName, dAttrVal);
    EXPECT_EQ(dAttrVal, NUM_1);
}

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS