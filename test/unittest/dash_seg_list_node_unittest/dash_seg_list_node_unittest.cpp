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
#include "dash_seg_list_node_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing;
using namespace testing::ext;
const static int32_t RET_TEST = 123;
const static int32_t NUM_TEST = 0;
void DashSegListNodeUnittest::SetUpTestCase(void) {}
void DashSegListNodeUnittest::TearDownTestCase(void) {}
void DashSegListNodeUnittest::SetUp(void)
{
    dashPtr_ = std::make_shared<DashSegListNode>();
}
void DashSegListNodeUnittest::TearDown(void)
{
    dashPtr_ = nullptr;
}

/**
 * @tc.name  : Test GetAttr(_, std::string)
 * @tc.number: GetAttr_001
 * @tc.desc  : Test index < DASH_SEG_LIST_ATTR_NUM
 *             Test segListAttr_[index].val_.length() > 0
 *             Test segListAttr_[index].val_.length() == 0
 */
HWTEST_F(DashSegListNodeUnittest, GetAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    std::string sAttrVal = "";
    dashPtr_->segListAttr_[NUM_TEST].val_ = "testVal";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, dashPtr_->segListAttr_[NUM_TEST].val_);

    dashPtr_->segListAttr_[NUM_TEST].val_ = "";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "");
}

/**
 * @tc.name  : Test GetAttr(_, uint32_t)
 * @tc.number: GetAttr_002
 * @tc.desc  : Test segListAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegListNodeUnittest, GetAttr_002, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint32_t uiAttrVal = 0;
    dashPtr_->segListAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(_, int32_t)
 * @tc.number: GetAttr_003
 * @tc.desc  : Test segListAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegListNodeUnittest, GetAttr_003, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    int32_t uiAttrVal = 0;
    dashPtr_->segListAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(_, uint64_t)
 * @tc.number: GetAttr_004
 * @tc.desc  : Test segListAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegListNodeUnittest, GetAttr_004, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint64_t uiAttrVal = 0;
    dashPtr_->segListAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(_, double)
 * @tc.number: GetAttr_005
 * @tc.desc  : Test segListAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegListNodeUnittest, GetAttr_005, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double uiAttrVal = 0;
    dashPtr_->segListAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test ParseNode
 * @tc.number: ParseNode_nullptr
 * @tc.desc  : Test ParseNode xmlParser nullptr, rootElement nullptr
 */
HWTEST_F(DashSegListNodeUnittest, ParseNode_nullptr, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    dashPtr_->ParseNode(nullptr, nullptr);
    dashPtr_->ParseNode(std::make_shared<XmlParser>(), nullptr);
    xmlNodePtr element = xmlNewNode(nullptr, BAD_CAST "root");
    dashPtr_->ParseNode(std::make_shared<XmlParser>(), std::make_shared<XmlElement>(element));
    std::string attrName = "media";
    double uiAttrVal = 0;
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, 0);
}
}
}
}
}