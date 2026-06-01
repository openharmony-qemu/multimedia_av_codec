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
#include "dash_seg_template_node_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing;
using namespace testing::ext;
const static int32_t RET_TEST = 123;
const static int32_t NUM_TEST = 0;
void DashSegTemplateNodeUnittest::SetUpTestCase(void) {}
void DashSegTemplateNodeUnittest::TearDownTestCase(void) {}
void DashSegTemplateNodeUnittest::SetUp(void)
{
    dashPtr_ = std::make_shared<DashSegTemplateNode>();
}
void DashSegTemplateNodeUnittest::TearDown(void)
{
    dashPtr_ = nullptr;
}

/**
 * @tc.name  : Test GetAttr(_, uint32_t)
 * @tc.number: GetAttr_001
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint32_t uiAttrVal = 0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(_, int32_t)
 * @tc.number: GetAttr_002
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_002, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    int32_t uiAttrVal = 0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(_, uint64_t)
 * @tc.number: GetAttr_003
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_003, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint64_t uiAttrVal = 0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(_, double)
 * @tc.number: GetAttr_004
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() > 0
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_004, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double uiAttrVal = 0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(RET_TEST);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, RET_TEST);
}

/**
 * @tc.name  : Test GetAttr(string, with empty value)
 * @tc.number: GetAttr_005
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() == 0 for string
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_005, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    std::string sAttrVal = "initial_value";
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "");
}

/**
 * @tc.name  : Test GetAttr(uint32_t, with empty value)
 * @tc.number: GetAttr_006
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() == 0 for uint32_t
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_006, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint32_t uiAttrVal = 999;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "";
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, 0);
}

/**
 * @tc.name  : Test GetAttr(int32_t, with empty value)
 * @tc.number: GetAttr_007
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() == 0 for int32_t
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_007, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    int32_t iAttrVal = 999;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "";
    dashPtr_->GetAttr(attrName, iAttrVal);
    EXPECT_EQ(iAttrVal, 0);
}

/**
 * @tc.name  : Test GetAttr(uint64_t, with empty value)
 * @tc.number: GetAttr_008
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() == 0 for uint64_t
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_008, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint64_t ullAttrVal = 999;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "";
    dashPtr_->GetAttr(attrName, ullAttrVal);
    EXPECT_EQ(ullAttrVal, 0);
}

/**
 * @tc.name  : Test GetAttr(double, with empty value)
 * @tc.number: GetAttr_009
 * @tc.desc  : Test segTemplateAttr_[index].val_.length() == 0 for double
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_009, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double dAttrVal = 999.0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "";
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_EQ(dAttrVal, 0.0);
}

/**
 * @tc.name  : Test GetAttr(string, with unknown attribute name)
 * @tc.number: GetAttr_010
 * @tc.desc  : Test getting unknown attribute name for string
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_010, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "unknown_attribute";
    std::string sAttrVal = "initial_value";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "initial_value");
}

/**
 * @tc.name  : Test GetAttr(uint32_t, with unknown attribute name)
 * @tc.number: GetAttr_011
 * @tc.desc  : Test getting unknown attribute name for uint32_t
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_011, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "unknown_attribute";
    uint32_t uiAttrVal = 999;
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, 999);
}

/**
 * @tc.name  : Test GetAttr(int32_t, with unknown attribute name)
 * @tc.number: GetAttr_012
 * @tc.desc  : Test getting unknown attribute name for int32_t
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_012, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "unknown_attribute";
    int32_t iAttrVal = 999;
    dashPtr_->GetAttr(attrName, iAttrVal);
    EXPECT_EQ(iAttrVal, 999);
}

/**
 * @tc.name  : Test GetAttr(uint64_t, with unknown attribute name)
 * @tc.number: GetAttr_013
 * @tc.desc  : Test getting unknown attribute name for uint64_t
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_013, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "unknown_attribute";
    uint64_t ullAttrVal = 999;
    dashPtr_->GetAttr(attrName, ullAttrVal);
    EXPECT_EQ(ullAttrVal, 999);
}

/**
 * @tc.name  : Test GetAttr(double, with unknown attribute name)
 * @tc.number: GetAttr_014
 * @tc.desc  : Test getting unknown attribute name for double
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_014, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "unknown_attribute";
    double dAttrVal = 999.0;
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_EQ(dAttrVal, 999.0);
}

/**
 * @tc.name  : Test GetAttr(string, with different attribute names)
 * @tc.number: GetAttr_015
 * @tc.desc  : Test getting different attribute names for string
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_015, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "index";
    std::string sAttrVal = "";
    dashPtr_->segTemplateAttr_[1].val_ = "test_index_value";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "test_index_value");
}

/**
 * @tc.name  : Test GetAttr(string, with initialization attribute)
 * @tc.number: GetAttr_016
 * @tc.desc  : Test getting initialization attribute for string
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_016, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "initialization";
    std::string sAttrVal = "";
    dashPtr_->segTemplateAttr_[2].val_ = "init_value";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "init_value");
}

/**
 * @tc.name  : Test GetAttr(string, with bitstreamSwitching attribute)
 * @tc.number: GetAttr_017
 * @tc.desc  : Test getting bitstreamSwitching attribute for string
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_017, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "bitstreamSwitching";
    std::string sAttrVal = "";
    dashPtr_->segTemplateAttr_[3].val_ = "true";
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, "true");
}

/**
 * @tc.name  : Test GetAttr(uint32_t, with large value)
 * @tc.number: GetAttr_018
 * @tc.desc  : Test getting large uint32_t value
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_018, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint32_t uiAttrVal = 0;
    uint32_t largeValue = 4294967295;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(largeValue);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    EXPECT_EQ(uiAttrVal, largeValue);
}

/**
 * @tc.name  : Test GetAttr(int32_t, with negative value)
 * @tc.number: GetAttr_019
 * @tc.desc  : Test getting negative int32_t value
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_019, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    int32_t iAttrVal = 0;
    int32_t negativeValue = -12345;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(negativeValue);
    dashPtr_->GetAttr(attrName, iAttrVal);
    EXPECT_EQ(iAttrVal, negativeValue);
}

/**
 * @tc.name  : Test GetAttr(uint64_t, with large value)
 * @tc.number: GetAttr_020
 * @tc.desc  : Test getting large uint64_t value within int64_t range
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_020, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    uint64_t ullAttrVal = 0;
    uint64_t largeValue = 9223372036854775807ULL; // INT64_MAX
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(largeValue);
    dashPtr_->GetAttr(attrName, ullAttrVal);
    EXPECT_EQ(ullAttrVal, largeValue);
}

/**
 * @tc.name  : Test GetAttr(double, with fractional value)
 * @tc.number: GetAttr_021
 * @tctc.desc  : Test getting fractional double value
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_021, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double dAttrVal = 0.0;
    double fractionalValue = 123.456;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(fractionalValue);
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_NEAR(dAttrVal, fractionalValue, 0.001);
}

/**
 * @tc.name  : Test GetAttrAttr(double, with scientific notation)
 * @tc.number: GetAttr_022
 * @tc.desc  : Test getting double value in scientific notation
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_022, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double dAttrVal = 0.0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "1.23e2";
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_NEAR(dAttrVal, 123.0, 0.001);
}

/**
 * @tc.name  : Test GetAttr(double, with negative value)
 * @tc.number: GetAttr_023
 * @tc.desc  : Test getting negative double value
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_023, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double dAttrVal = 0.0;
    double negativeValue = -123.456;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = std::to_string(negativeValue);
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_NEAR(dAttrVal, negativeValue, 0.001);
}

/**
 * @tc.name  : Test GetAttr(double, with infinity value)
 * @tc.number: GetAttr_024
 * @tc.desc  : Test getting infinity double value
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_024, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double dAttrVal = 0.0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "inf";
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_TRUE(std::isinf(dAttrVal));
}

/**
 * @tc.name  : Test GetAttr(double, with NaN value)
 * @tc.number: GetAttr_025
 * @tc.desc  : Test getting NaN double value
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_025, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    double dAttrVal = 0.0;
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "nan";
    dashPtr_->GetAttr(attrName, dAttrVal);
    EXPECT_TRUE(std::isnan(dAttrVal));
}

/**
 * @tc.name  : Test GetAttr(string, with DASH template)
 * @tc.number: GetAttr_026
 * @tc.desc  : Test getting DASH segment template string
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_026, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    std::string sAttrVal = "";
    std::string templateValue = "segment_$Number$_$Time$.m4s";
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = templateValue;
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, templateValue);
}

/**
 * @tc.name  : Test GetAttr(string, with URL-like value)
 * @tc.number: GetAttr_027
 * @tc.desc  : Test getting URL-like string value
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_027, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    std::string sAttrVal = "";
    std::string urlValue = "http://example.com/segment_$Time$.m4s";
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = urlValue;
    dashPtr_->GetAttr(attrName, sAttrVal);
    EXPECT_EQ(sAttrVal, urlValue);
}

/**
 * @tc.name  : Test Constructor initialization
 * @tc.number: Constructor_001
 * @tc.desc  : Test that constructor properly initializes attributes
 */
HWTEST_F(DashSegTemplateNodeUnittest, Constructor_001, TestSize.Level0)
{
    auto testNode = std::make_shared<DashSegTemplateNode>();
    ASSERT_NE(testNode, nullptr);
    for (int i = 0; i < 4; i++) {
        EXPECT_EQ(testNode->segTemplateAttr_[i].val_, "");
    }
}

/**
 * @tc.name  : Test multiple GetAttr calls
 * @tc.number: GetAttr_028
 * @tc.desc  : Test multiple GetAttr calls with different types
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_028, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "123";
    std::string sAttrVal = "";
    uint32_t uiAttrVal = 0;
    int32_t iAttrVal = 0;
    uint64_t ullAttrVal = 0;
    double dAttrVal = 0.0;
    
    dashPtr_->GetAttr(attrName, sAttrVal);
    dashPtr_->GetAttr(attrName, uiAttrVal);
    dashPtr_->GetAttr(attrName, iAttrVal);
    dashPtr_->GetAttr(attrName, ullAttrVal);
    dashPtr_->GetAttr(attrName, dAttrVal);
    
    EXPECT_EQ(sAttrVal, "123");
    EXPECT_EQ(uiAttrVal, 123);
    EXPECT_EQ(iAttrVal, 123);
    EXPECT_EQ(ullAttrVal, 123);
    EXPECT_NEAR(dAttrVal, 123.0, 0.001);
}

/**
 * @tc.name  : Test GetAttr with all attributes set
 * @tc.number: GetAttr_029
 * @tc.desc  : Test getting all attributes when all are set
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_029, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    
    dashPtr_->segTemplateAttr_[0].val_ = "media_value";
    dashPtr_->segTemplateAttr_[1].val_ = "index_value";
    dashPtr_->segTemplateAttr_[2].val_ = "init_value";
    dashPtr_->segTemplateAttr_[3].val_ = "true";
    
    std::string sAttrVal1 = "";
    std::string sAttrVal2 = "";
    std::string sAttrVal3 = "";
    std::string sAttrVal4 = "";
    
    dashPtr_->GetAttr("media", sAttrVal1);
    dashPtr_->GetAttr("index", sAttrVal2);
    dashPtr_->GetAttr("initialization", sAttrVal3);
    dashPtr_->GetAttr("bitstreamSwitching", sAttrVal4);
    
    EXPECT_EQ(sAttrVal1, "media_value");
    EXPECT_EQ(sAttrVal2, "index_value");
    EXPECT_EQ(sAttrVal3, "init_value");
    EXPECT_EQ(sAttrVal4, "true");
}

/**
 * @tc.name  : Test GetAttr with repeated calls same attribute
 * @tc.number: GetAttr_030
 * @tc.desc  : Test getting same attribute multiple times
 */
HWTEST_F(DashSegTemplateNodeUnittest, GetAttr_030, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    std::string attrName = "media";
    
    dashPtr_->segTemplateAttr_[NUM_TEST].val_ = "test_value";
    
    std::string sAttrVal1 = "";
    std::string sAttrVal2 = "";
    std::string sAttrVal3 = "";
    
    dashPtr_->GetAttr(attrName, sAttrVal1);
    dashPtr_->GetAttr(attrName, sAttrVal2);
    dashPtr_->GetAttr(attrName, sAttrVal3);
    
    EXPECT_EQ(sAttrVal1, "test_value");
    EXPECT_EQ(sAttrVal2, "test_value");
    EXPECT_EQ(sAttrVal3, "test_value");
}
}
}
}
}