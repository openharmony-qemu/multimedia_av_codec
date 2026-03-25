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
#include "dash_descriptor_node_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing;
using namespace testing::ext;

// "schemeIdUri" is index 0, "value" is index 1, "default_KID" is index 2
const static int32_t RET_TEST = 123;
const static int32_t NUM_TEST_SCHEME = 0; // schemeIdUri
const static int32_t NUM_TEST_VALUE  = 1; // value
const static int32_t NUM_TEST_KID    = 2; // default_KID

void DashDescriptorNodeUnittest::SetUpTestCase(void) {}
void DashDescriptorNodeUnittest::TearDownTestCase(void) {}
void DashDescriptorNodeUnittest::SetUp(void)
{
    dashPtr_ = std::make_shared<DashDescriptorNode>();
}
void DashDescriptorNodeUnittest::TearDown(void)
{
    dashPtr_ = nullptr;
}

/**
 * @tc.name  : Test GetAttr(_, std::string) with non-empty val
 * @tc.number: GetAttr_001
 * @tc.desc  : Test descriptorAttr_[index].val_.length() > 0 -> sAttrVal is set
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "schemeIdUri";
    std::string sAttrVal = "";
    dashPtr_->descriptorAttr_[NUM_TEST_SCHEME].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, std::to_string(RET_TEST));
}

/**
 * @tc.name  : Test GetAttr(_, std::string) with empty val
 * @tc.number: GetAttr_001_empty
 * @tc.desc  : Test descriptorAttr_[index].val_.length() == 0 -> sAttrVal is ""
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_001_empty, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "schemeIdUri";
    std::string sAttrVal = "pre-set";
    dashPtr_->descriptorAttr_[NUM_TEST_SCHEME].val_ = "";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "");
}

/**
 * @tc.name  : Test GetAttr(_, uint32_t) with non-empty val
 * @tc.number: GetAttr_002
 * @tc.desc  : Test descriptorAttr_[index].val_.length() > 0 -> uiAttrVal is set
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_002, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "value";
    uint32_t uiAttrVal = 0;
    dashPtr_->descriptorAttr_[NUM_TEST_VALUE].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, static_cast<uint32_t>(RET_TEST));
}

/**
 * @tc.name  : Test GetAttr(_, uint32_t) with empty val
 * @tc.number: GetAttr_002_empty
 * @tc.desc  : Test descriptorAttr_[index].val_.length() == 0 -> uiAttrVal is 0
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_002_empty, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "value";
    uint32_t uiAttrVal = 99; // 99: pre-set sentinel
    dashPtr_->descriptorAttr_[NUM_TEST_VALUE].val_ = "";
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, 0u);
}

/**
 * @tc.name  : Test GetAttr(_, int32_t) with non-empty val
 * @tc.number: GetAttr_003
 * @tc.desc  : Test descriptorAttr_[index].val_.length() > 0 -> iAttrVal is set
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_003, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "default_KID";
    int32_t iAttrVal = 0;
    dashPtr_->descriptorAttr_[NUM_TEST_KID].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, iAttrVal);
    EXPECT_EQ(iAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(_, int32_t) with empty val
 * @tc.number: GetAttr_003_empty
 * @tc.desc  : Test descriptorAttr_[index].val_.length() == 0 -> iAttrVal is 0
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_003_empty, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "default_KID";
    int32_t iAttrVal = 99; // 99: pre-set sentinel
    dashPtr_->descriptorAttr_[NUM_TEST_KID].val_ = "";
    dashPtr_->GetAttr(attrName, iAttrVal);
    EXPECT_EQ(iAttrVal, 0);
}

/**
 * @tc.name  : Test GetAttr(_, uint64_t) with non-empty val
 * @tc.number: GetAttr_004
 * @tc.desc  : Test descriptorAttr_[index].val_.length() > 0 -> ullAttrVal is set
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_004, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "schemeIdUri";
    uint64_t ullAttrVal = 0;
    dashPtr_->descriptorAttr_[NUM_TEST_SCHEME].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, ullAttrVal);
    EXPECT_EQ(ullAttrVal, static_cast<uint64_t>(RET_TEST));
}

/**
 * @tc.name  : Test GetAttr(_, uint64_t) with empty val
 * @tc.number: GetAttr_004_empty
 * @tc.desc  : Test descriptorAttr_[index].val_.length() == 0 -> ullAttrVal is 0
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_004_empty, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "schemeIdUri";
    uint64_t ullAttrVal = 99; // 99: pre-set sentinel
    dashPtr_->descriptorAttr_[NUM_TEST_SCHEME].val_ = "";
    dashPtr_->GetAttr(attrName, ullAttrVal);
    EXPECT_EQ(ullAttrVal, 0u);
}

/**
 * @tc.name  : Test GetAttr(_, double) with non-empty val
 * @tc.number: GetAttr_005
 * @tc.desc  : Test descriptorAttr_[index].val_.length() > 0 -> dAttrVal is set
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_005, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "value";
    double dAttrVal = 0.0;
    dashPtr_->descriptorAttr_[NUM_TEST_VALUE].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_DOUBLE_EQ(dAttrVal, static_cast<double>(RET_TEST));
}

/**
 * @tc.name  : Test GetAttr(_, double) with empty val
 * @tc.number: GetAttr_005_empty
 * @tc.desc  : Test descriptorAttr_[index].val_.length() == 0 -> dAttrVal is 0.0
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_005_empty, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "value";
    double dAttrVal = 99.0; // 99.0: pre-set sentinel
    dashPtr_->descriptorAttr_[NUM_TEST_VALUE].val_ = "";
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_DOUBLE_EQ(dAttrVal, 0.0);
}

/**
 * @tc.name  : Test GetAttr with unknown attrName
 * @tc.number: GetAttr_unknown
 * @tc.desc  : index >= DASH_DESCRIPTOR_ATTR_NUM -> output value unchanged
 */
HWTEST_F(DashDescriptorNodeUnittest, GetAttr_unknown, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "unknownAttr";
    std::string sAttrVal = "unchanged";
    uint32_t uiAttrVal = 77;   // 77: pre-set sentinel
    int32_t  iAttrVal  = 77;   // 77: pre-set sentinel
    uint64_t ullAttrVal = 77;  // 77: pre-set sentinel
    double   dAttrVal  = 77.0; // 77.0: pre-set sentinel
    dashPtr_->GetAttr(attrName, sAttrVal);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    dashPtr_->GetAttr(attrName, iAttrVal);
    dashPtr_->GetAttr(attrName, ullAttrVal);
    dashPtr_->GetAttr(attrName, dAttrVal);
    // all outputs unchanged since index is out of range
    EXPECT_EQ(sAttrVal, "unchanged");
    EXPECT_EQ(uiAttrVal, 77u);
    EXPECT_EQ(iAttrVal, 77);
    EXPECT_EQ(ullAttrVal, 77u);
    EXPECT_DOUBLE_EQ(dAttrVal, 77.0);
}

/**
 * @tc.name  : Test ParseNode with nullptr xmlParser
 * @tc.number: ParseNode_nullptr
 * @tc.desc  : Test ParseNode when xmlParser is nullptr -> no crash, attrs remain empty
 */
HWTEST_F(DashDescriptorNodeUnittest, ParseNode_nullptr, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    dashPtr_->ParseNode(nullptr, nullptr);
    dashPtr_->ParseNode(std::make_shared<XmlParser>(), nullptr);
    xmlNodePtr element = xmlNewNode(nullptr, BAD_CAST "root");
    dashPtr_->ParseNode(std::make_shared<XmlParser>(), std::make_shared<XmlElement>(element));
    std::string attrName = "schemeIdUri";
    std::string sAttrVal = "";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_TRUE(sAttrVal.empty());
}
}
}
}
}
