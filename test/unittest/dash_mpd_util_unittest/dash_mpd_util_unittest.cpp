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

#include "dash_mpd_util_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace std;
using namespace testing;
using namespace testing::ext;
static const int32_t NUM_0 = 0;
static const int32_t ERROR = -1;
static const uint32_t MS_OF_TWO_DAYS = 172800000u;

void DashMpdUtilUnitTest::SetUpTestCase(void) {}

void DashMpdUtilUnitTest::TearDownTestCase(void) {}

void DashMpdUtilUnitTest::SetUp(void) {}

void DashMpdUtilUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test DashAppendBaseUrl
 * @tc.number: DashAppendBaseUrl_001
 * @tc.desc  : Test DashUrlIsAbsolute(baseUrl) == true
 */
HWTEST_F(DashMpdUtilUnitTest, DashAppendBaseUrl_001, TestSize.Level0)
{
    std::string srcUrl;
    std::string baseUrl = "http://host/path/file";
    DashAppendBaseUrl(srcUrl, baseUrl);
    EXPECT_EQ(srcUrl, baseUrl);
}

/**
 * @tc.name  : Test DashAppendBaseUrl
 * @tc.number: DashAppendBaseUrl_002
 * @tc.desc  : Test baseUrl.find('/') == 0
 */
HWTEST_F(DashMpdUtilUnitTest, DashAppendBaseUrl_002, TestSize.Level0)
{
    std::string srcUrl;
    std::string baseUrl = "//host/path/file";
    DashAppendBaseUrl(srcUrl, baseUrl);
    EXPECT_EQ(srcUrl, baseUrl);
}

/**
 * @tc.name  : Test BuildSrcUrl
 * @tc.number: BuildSrcUrl_001
 * @tc.desc  : Test DashUrlIsAbsolute(srcUrl) == true
 *             Test urlSchemIndex != std::string::npos
 *             Test urlPathIndex != std::string::npos
 */
HWTEST_F(DashMpdUtilUnitTest, BuildSrcUrl_001, TestSize.Level0)
{
    // Test DashUrlIsAbsolute(srcUrl) == true
    // Test urlSchemIndex != std::string::npos
    // Test urlPathIndex != std::string::npos
    std::string srcUrl = "http://host/abc";
    std::string baseUrl = "/xyz";
    BuildSrcUrl(srcUrl, baseUrl);
    EXPECT_EQ(srcUrl, "http://host/xyz");
}

/**
 * @tc.name  : Test BuildSrcUrl
 * @tc.number: BuildSrcUrl_002
 * @tc.desc  : Test DashUrlIsAbsolute(srcUrl) == false
 */
HWTEST_F(DashMpdUtilUnitTest, BuildSrcUrl_002, TestSize.Level0)
{
    std::string srcUrl;
    std::string baseUrl = "http://host/path/file";
    BuildSrcUrl(srcUrl, baseUrl);
    EXPECT_EQ(srcUrl, baseUrl);
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr
 * @tc.number: DashSubstituteTmpltStr_001
 * @tc.desc  : Test posEnd == std::string::npos
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_001, TestSize.Level0)
{
    std::string segTmpltStr = "$Number%012";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr
 * @tc.number: DashSubstituteTmpltStr_002
 * @tc.desc  : Test (str[0] == '%' && str[1] == '0') == false && (str[0] == '$') == false
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_002, TestSize.Level0)
{
    std::string segTmpltStr = "$NumberABC";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test DashStrToDuration
 * @tc.number: DashStrToDuration_001
 * @tc.desc  : Test case 'D'
 */
HWTEST_F(DashMpdUtilUnitTest, DashStrToDuration_001, TestSize.Level0)
{
    std::string str = "P2D";
    uint32_t duration = NUM_0;
    int32_t ret = DashStrToDuration(str, duration);
    EXPECT_EQ(ret, NUM_0);
    EXPECT_EQ(duration, MS_OF_TWO_DAYS);
}

/**
 * @tc.name  : Test DashStrToDuration
 * @tc.number: DashStrToDuration_002
 * @tc.desc  : Test case default
 */
HWTEST_F(DashMpdUtilUnitTest, DashStrToDuration_002, TestSize.Level0)
{
    std::string str = "P2X";
    uint32_t duration = NUM_0;
    int32_t ret = DashStrToDuration(str, duration);
    EXPECT_EQ(ret, NUM_0);
    EXPECT_EQ(duration, NUM_0);
}

/**
 * @tc.name  : Test DashParseRange
 * @tc.number: DashParseRange_001
 * @tc.desc  : Test separatePosition == std::string::npos
 */
HWTEST_F(DashMpdUtilUnitTest, DashParseRange_001, TestSize.Level0)
{
    std::string rangeStr = "123456";
    int64_t startRange = ERROR;
    int64_t endRange = ERROR;
    DashParseRange(rangeStr, startRange, endRange);
    EXPECT_EQ(startRange, ERROR);
    EXPECT_EQ(endRange, ERROR);
}

/**
 * @tc.name  : Test DashParseRange
 * @tc.number: DashParseRange_002
 * @tc.desc  : Test separatePosition == 0
 */
HWTEST_F(DashMpdUtilUnitTest, DashParseRange_002, TestSize.Level0)
{
    std::string rangeStr = "-123456";
    int64_t startRange = ERROR;
    int64_t endRange = ERROR;
    DashParseRange(rangeStr, startRange, endRange);
    EXPECT_EQ(startRange, ERROR);
    EXPECT_EQ(endRange, ERROR);
}

/**
 * @tc.name  : Test DashStreamIsHdr
 * @tc.number: DashStreamIsHdr_001
 * @tc.desc  : Test default
 */
HWTEST_F(DashMpdUtilUnitTest, DashStreamIsHdr_001, TestSize.Level0)
{
    DashList<DashDescriptor*> essentialPropertyList;
    bool isHdr = DashStreamIsHdr(essentialPropertyList);
    EXPECT_EQ(isHdr, false);
}

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS