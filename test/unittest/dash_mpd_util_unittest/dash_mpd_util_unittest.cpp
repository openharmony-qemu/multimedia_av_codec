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
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_003
 * @tc.desc  : Test GetSubstitutionStr with format width 5, substitutionStr "5" -> "00005"
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_003, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%05d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_00005.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_004
 * @tc.desc  : Test GetSubstitutionStr with format width equals string length
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_004, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%05d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "12345";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_12345.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_005
 * @tc.desc  : Test GetSubstitutionStr with format width less than string length
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_005, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%03d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "12345";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_12345.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_006
 * @tc.desc  : Test GetSubstitutionStr with format width 1
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_006, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%01d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_5.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_007
 * @tc.desc  : Test GetSubstitutionStr with format width equals MAX_SUBSTITUTION_FORMAT_WIDTH (32)
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_007, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%032d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_00000000000000000000000000000005.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_008
 * @tc.desc  : Test GetSubstitutionStr with format width exceeds MAX_SUBSTITUTION_FORMAT_WIDTH (32)
 *             Should use MAX_SUBSTITUTION_FORMAT_WIDTH
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_008, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%033d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_00000000000000000000000000000005.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_009
 * @tc.desc  : Test GetSubstitutionStr with format width 0
 *             Should not apply padding
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_009, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%00d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_5.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_010
 * @tc.desc  : Test GetSubstitutionStr with negative format width
 *             Should not apply padding
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_010, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%0-5d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_5.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_011
 * @tc.desc  : Test GetSubstitutionStr with empty substitutionStr
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_011, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%05d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_00000.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_012
 * @tc.desc  : Test GetSubstitutionStr with long substitutionStr
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_012, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%05d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "1234567890";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_1234567890.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_013
 * @tc.desc  : Test GetSubstitutionStr with multiple identifiers
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_013, TestSize.Level0)
{
    std::string segTmpltStr = "$RepresentationID_$Number%05d$.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "123";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "$RepresentationID_00123.m4s");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_014
 * @tc.desc  : Test GetSubstitutionStr with identifier at the end
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_014, TestSize.Level0)
{
    std::string segTmpltStr = "segment_$Number%05d$";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "segment_00005");
}

/**
 * @tc.name  : Test DashSubstituteTmpltStr with GetSubstitutionStr
 * @tc.number: DashSubstituteTmpltStr_015
 * @tc.desc  : Test GetSubstitutionStr with identifier at the beginning
 */
HWTEST_F(DashMpdUtilUnitTest, DashSubstituteTmpltStr_015, TestSize.Level0)
{
    std::string segTmpltStr = "$Number%05d$_segment.m4s";
    std::string segTmpltIdentifier = "$Number";
    std::string substitutionStr = "5";
    int32_t ret = DashSubstituteTmpltStr(segTmpltStr, segTmpltIdentifier, substitutionStr);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(segTmpltStr, "00005_segment.m4s");
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