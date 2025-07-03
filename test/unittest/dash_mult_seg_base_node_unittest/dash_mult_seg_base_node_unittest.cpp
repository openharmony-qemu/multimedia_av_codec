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

#include "dash_mult_seg_base_node_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace std;
using namespace testing;
using namespace testing::ext;
static const int32_t NUM_0 = 0;
static const int32_t NUM_1 = 1;

void DashMultSegBaseNodeUnitTest::SetUpTestCase(void) {}

void DashMultSegBaseNodeUnitTest::TearDownTestCase(void) {}

void DashMultSegBaseNodeUnitTest::SetUp(void)
{
    dashMultSegBaseNode_ = std::make_shared<DashMultSegBaseNode>();
}

void DashMultSegBaseNodeUnitTest::TearDown(void)
{
    dashMultSegBaseNode_ = nullptr;
}

/**
 * @tc.name  : Test GetAttr(string, string)
 * @tc.number: GetAttr_001
 * @tc.desc  : Test segBaseNode_ != nullptr
 */
HWTEST_F(DashMultSegBaseNodeUnitTest, GetAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashMultSegBaseNode_, nullptr);
    std::string attrName = "test";
    std::string sAttrVal;
    dashMultSegBaseNode_->segBaseNode_ = new DashMpdNodeImpl();
    dashMultSegBaseNode_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "testVal");
}

/**
 * @tc.name  : Test GetAttr(string, int32_t)
 * @tc.number: GetAttr_002
 * @tc.desc  : Test multSegBaseAttr_[index].val_.length() > 0
 */
HWTEST_F(DashMultSegBaseNodeUnitTest, GetAttr_002, TestSize.Level0)
{
    ASSERT_NE(dashMultSegBaseNode_, nullptr);
    std::string attrName = "duration";
    int32_t iAttrVal;
    dashMultSegBaseNode_->multSegBaseAttr_[NUM_0].val_ = "1";
    dashMultSegBaseNode_->GetAttr(attrName, iAttrVal);
    EXPECT_EQ(iAttrVal, NUM_1);
}

/**
 * @tc.name  : Test GetAttr(string, uint64_t)
 * @tc.number: GetAttr_003
 * @tc.desc  : Test multSegBaseAttr_[index].val_.length() > 0
 */
HWTEST_F(DashMultSegBaseNodeUnitTest, GetAttr_003, TestSize.Level0)
{
    ASSERT_NE(dashMultSegBaseNode_, nullptr);
    std::string attrName = "duration";
    uint64_t ullAttrVal;
    dashMultSegBaseNode_->multSegBaseAttr_[NUM_0].val_ = "1";
    dashMultSegBaseNode_->GetAttr(attrName, ullAttrVal);
    EXPECT_EQ(ullAttrVal, NUM_1);
}

/**
 * @tc.name  : Test GetAttr(string, double)
 * @tc.number: GetAttr_004
 * @tc.desc  : Test multSegBaseAttr_[index].val_.length() > 0
 */
HWTEST_F(DashMultSegBaseNodeUnitTest, GetAttr_004, TestSize.Level0)
{
    ASSERT_NE(dashMultSegBaseNode_, nullptr);
    std::string attrName = "duration";
    double dAttrVal;
    dashMultSegBaseNode_->multSegBaseAttr_[NUM_0].val_ = "1";
    dashMultSegBaseNode_->GetAttr(attrName, dAttrVal);
    EXPECT_EQ(dAttrVal, NUM_1);
}

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS