/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <cstdlib>
#include <cstring>
#include "native_avcodec_videobase.h"
#include "native_avformat.h"
#include "securec.h"
#define TEST_SUIT RoiStringTest

using namespace testing::ext;

class TEST_SUIT : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
    void TearDown(void) {};
};

static OH_AVFormat *CreateRoiFormat(int32_t top, int32_t left, int32_t bottom, int32_t right)
{
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, top);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, left);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bottom);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, right);
    return fmt;
}

static void DestroyFormats(OH_AVFormat **formats, uint32_t count)
{
    for (uint32_t i = 0; i < count; i++) {
        OH_AVFormat_Destroy(formats[i]);
    }
}

/**
 * @tc.name: RoiString_AppendRoiString_NullRoiStrInOut_001
 * @tc.desc: Test AppendRoiString with null double pointer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_NullRoiStrInOut_001, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 10, 10);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_AppendRoiString(nullptr, fmt));
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: RoiString_AppendRoiString_NullFormat_002
 * @tc.desc: Test AppendRoiString with null format
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_NullFormat_002, TestSize.Level1)
{
    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_AppendRoiString(&roiStr, nullptr));
}

/**
 * @tc.name: RoiString_AppendRoiString_CoordsOnly_003
 * @tc.desc: Test AppendRoiString with only coordinates
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_CoordsOnly_003, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 10, 10);
    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-10,10;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: RoiString_AppendRoiString_WithDqp_004
 * @tc.desc: Test AppendRoiString with dqp only
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_WithDqp_004, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 100, 100);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, 5);
    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-100,100=dqp:5;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: RoiString_AppendRoiString_WithDqpAndSemLabel_005
 * @tc.desc: Test AppendRoiString with both dqp and semLabel
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_WithDqpAndSemLabel_005, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 50, 60);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, -3);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 1);
    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-50,60=dqp:-3,slb:1;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: RoiString_AppendRoiString_SeparatorProtection_006
 * @tc.desc: Test AppendRoiString when existing string lacks trailing semicolon
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_SeparatorProtection_006, TestSize.Level1)
{
    char *roiStr = static_cast<char *>(malloc(16));
    strcpy_s(roiStr, 16, "0,0-100,100=5");
    OH_AVFormat *fmt = CreateRoiFormat(10, 20, 200, 300);
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-100,100=5;10,20-200,300;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: RoiString_AppendRoiString_NoCountLimit_007
 * @tc.desc: Test that appending more than 6 ROIs succeeds when total length stays under 256
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_NoCountLimit_007, TestSize.Level1)
{
    char *roiStr = nullptr;
    for (int32_t i = 0; i < 7; i++) {
        OH_AVFormat *fmt = CreateRoiFormat(i * 10, 0, (i + 1) * 10, 100);
        ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
        OH_AVFormat_Destroy(fmt);
    }
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount(roiStr, &count));
    EXPECT_EQ(count, 7);
    free(roiStr);
}

/**
 * @tc.name: RoiString_AppendRoiString_ExceedMaxLen_008
 * @tc.desc: Test that appending a ROI which would push total length over 256 is rejected
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_ExceedMaxLen_008, TestSize.Level1)
{
    char *roiStr = nullptr;
    int32_t bigCoord = 2147483647;
    for (int32_t i = 0; i < 4; i++) {
        OH_AVFormat *fmt = OH_AVFormat_Create();
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, bigCoord - 1 - i);
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, bigCoord - 1);
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bigCoord);
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, bigCoord);
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, -51);
        OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 1);
        ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
        OH_AVFormat_Destroy(fmt);
    }
    OH_AVFormat *fmt5 = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt5, OH_MD_KEY_VIDEO_METADATA_ROI_TOP, bigCoord - 1);
    OH_AVFormat_SetIntValue(fmt5, OH_MD_KEY_VIDEO_METADATA_ROI_LEFT, bigCoord - 1);
    OH_AVFormat_SetIntValue(fmt5, OH_MD_KEY_VIDEO_METADATA_ROI_BOTTOM, bigCoord);
    OH_AVFormat_SetIntValue(fmt5, OH_MD_KEY_VIDEO_METADATA_ROI_RIGHT, bigCoord);
    OH_AVFormat_SetIntValue(fmt5, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, -51);
    OH_AVFormat_SetIntValue(fmt5, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 1);
    EXPECT_EQ(AV_ERR_NO_MEMORY, OH_VideoMetadata_AppendRoiString(&roiStr, fmt5));
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount(roiStr, &count));
    EXPECT_EQ(count, 4);
    OH_AVFormat_Destroy(fmt5);
    free(roiStr);
}

/**
 * @tc.name: RoiString_GetRoiCount_NullRoiStr_009
 * @tc.desc: Test GetRoiCount with null roiStr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_NullRoiStr_009, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_GetRoiCount(nullptr, &count));
}

/**
 * @tc.name: RoiString_GetRoiCount_NullOutCount_010
 * @tc.desc: Test GetRoiCount with null outCount
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_NullOutCount_010, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_GetRoiCount("0,0-10,10;", nullptr));
}

/**
 * @tc.name: RoiString_GetRoiCount_EmptyString_011
 * @tc.desc: Test GetRoiCount with empty string
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_EmptyString_011, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_SingleOldFormat_012
 * @tc.desc: Test GetRoiCount with single old-format ROI string
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_SingleOldFormat_012, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("0,0-100,100=5;", &count));
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: RoiString_GetRoiCount_MultipleRois_013
 * @tc.desc: Test GetRoiCount with multiple ROI regions
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_MultipleRois_013, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("0,0-100,100;10,20-200,300=dqp:3;", &count));
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: RoiString_GetRoiCount_InvalidKvpDqpNonInt_014
 * @tc.desc: Test GetRoiCount rejects segment with non-integer dqp value "dqp:abc"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_InvalidKvpDqpNonInt_014, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=dqp:abc;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_InvalidKvpSlbTrailing_015
 * @tc.desc: Test GetRoiCount rejects segment with trailing garbage "slb:1t"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_InvalidKvpSlbTrailing_015, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=slb:1t;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_InvalidKvpOldFormatNonInt_016
 * @tc.desc: Test GetRoiCount rejects old format segment "=abc"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_InvalidKvpOldFormatNonInt_016, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=abc;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_InvalidKvpOldFormatTrailing_017
 * @tc.desc: Test GetRoiCount rejects old format "=5xyz" with trailing garbage
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_InvalidKvpOldFormatTrailing_017, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=5xyz;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_MixedValidAndInvalid_018
 * @tc.desc: Test GetRoiCount only counts valid segments among mixed valid/invalid
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_MixedValidAndInvalid_018, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("0,0-10,10=dqp:5;1,2-3,4=abc;5,6-7,8;", &count));
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: RoiString_GetRoiCount_DuplicateKey_019
 * @tc.desc: Test GetRoiCount rejects segment with duplicate key "=dqp:5,dqp:3"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_DuplicateKey_019, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=dqp:5,dqp:3;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_TrailingComma_020
 * @tc.desc: Test GetRoiCount rejects segment with trailing comma "=dqp:5,"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_TrailingComma_020, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=dqp:5,", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_GarbageBetweenCoordsAndEq_021
 * @tc.desc: Test GetRoiCount rejects segment with garbage between coords and "="
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_GarbageBetweenCoordsAndEq_021, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4,ab=dqp:1;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_ParseRoiString_NullRoiStr_022
 * @tc.desc: Test ParseRoiString with null roiStr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_NullRoiStr_022, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_ParseRoiString(nullptr, formats, 4, &count));
}

/**
 * @tc.name: RoiString_ParseRoiString_OldFormatDeltaQP_023
 * @tc.desc: Test ParseRoiString with old format "=deltaQP"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_OldFormatDeltaQP_023, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString("0,0-100,100=5;", formats, 4, &count));
    ASSERT_EQ(count, 1);
    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 5);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_NewFormatSlbAndDqp_024
 * @tc.desc: Test ParseRoiString with reversed order "=slb:val,dqp:val"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_NewFormatSlbAndDqp_024, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString("0,0-100,100=slb:1,dqp:5;", formats, 4, &count));
    ASSERT_EQ(count, 1);
    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 5);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &val));
    EXPECT_EQ(val, 1);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_NoCrossSegmentParse_025
 * @tc.desc: Test that old-format "=5" is not confused by dqp in later segments
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_NoCrossSegmentParse_025, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    const char *roiStr = "0,0-100,100=5;10,20-200,200=dqp:3;";
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(roiStr, formats, 4, &count));
    ASSERT_EQ(count, 2);
    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 5);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &val));
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_InvalidKvpDqpNonInt_026
 * @tc.desc: Test ParseRoiString skips segment with non-integer dqp value
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_InvalidKvpDqpNonInt_026, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString("1,2-3,4=dqp:abc;", formats, 4, &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_ParseRoiString_InvalidKvpSlbTrailing_027
 * @tc.desc: Test ParseRoiString skips segment with trailing garbage in slb value
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_InvalidKvpSlbTrailing_027, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString("1,2-3,4=dqp:5,slb:1t;", formats, 4, &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_ParseRoiString_MixedValidAndInvalid_028
 * @tc.desc: Test ParseRoiString only parses valid segments from mixed input
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_MixedValidAndInvalid_028, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10=dqp:5;1,2-3,4=abc;5,6-7,8=slb:1;", formats, 4, &count));
    ASSERT_EQ(count, 2);
    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 5);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[1], OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &val));
    EXPECT_EQ(val, 1);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_DuplicateKey_029
 * @tc.desc: Test ParseRoiString skips segment with duplicate key
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_DuplicateKey_029, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString("1,2-3,4=dqp:5,dqp:3;", formats, 4, &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_RoundTrip_AppendAndParse_030
 * @tc.desc: Test round-trip: append multiple ROIs then parse back
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_RoundTrip_AppendAndParse_030, TestSize.Level1)
{
    char *roiStr = nullptr;
    OH_AVFormat *fmt1 = CreateRoiFormat(0, 0, 100, 100);
    OH_AVFormat_SetIntValue(fmt1, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, 5);
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt1));
    OH_AVFormat *fmt2 = CreateRoiFormat(10, 20, 200, 300);
    OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 1);
    OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, -2);
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt2));
    OH_AVFormat *fmt3 = CreateRoiFormat(50, 60, 400, 500);
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt3));

    uint32_t roiCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount(roiStr, &roiCount));
    EXPECT_EQ(roiCount, 3);

    OH_AVFormat *parsed[4] = {};
    uint32_t parsedCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(roiStr, parsed, 4, &parsedCount));
    ASSERT_EQ(parsedCount, 3);

    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(parsed[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 5);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(parsed[1], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, -2);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(parsed[1], OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &val));
    EXPECT_EQ(val, 1);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(parsed[2], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));

    free(roiStr);
    DestroyFormats(parsed, parsedCount);
    OH_AVFormat_Destroy(fmt1);
    OH_AVFormat_Destroy(fmt2);
    OH_AVFormat_Destroy(fmt3);
}

/**
 * @tc.name: RoiString_RoundTrip_ParseThenAppend_031
 * @tc.desc: Test: parse a valid ROI string, then append one more, verify consistency
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_RoundTrip_ParseThenAppend_031, TestSize.Level1)
{
    const char *inputStr = "0,0-100,100=dqp:5;";
    OH_AVFormat *parsed[4] = {};
    uint32_t parsedCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(inputStr, parsed, 4, &parsedCount));
    ASSERT_EQ(parsedCount, 1);

    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, parsed[0]));
    EXPECT_STREQ(roiStr, "0,0-100,100=dqp:5;");

    OH_AVFormat *fmt2 = CreateRoiFormat(10, 20, 200, 300);
    OH_AVFormat_SetIntValue(fmt2, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 1);
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt2));
    EXPECT_STREQ(roiStr, "0,0-100,100=dqp:5;10,20-200,300=slb:1;");

    uint32_t totalCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount(roiStr, &totalCount));
    EXPECT_EQ(totalCount, 2);

    free(roiStr);
    DestroyFormats(parsed, parsedCount);
    OH_AVFormat_Destroy(fmt2);
}

/**
 * @tc.name: RoiString_RoundTrip_ParseInvalidThenAppend_032
 * @tc.desc: Test: parse an invalid ROI string (0 segments), append valid ones, verify result
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_RoundTrip_ParseInvalidThenAppend_032, TestSize.Level1)
{
    const char *invalidStr = "1,2-3,4=dqp:abc;";
    OH_AVFormat *parsed[4] = {};
    uint32_t parsedCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(invalidStr, parsed, 4, &parsedCount));
    EXPECT_EQ(parsedCount, 0);

    char *roiStr = nullptr;
    OH_AVFormat *fmt = CreateRoiFormat(5, 5, 50, 50);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, -10);
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    EXPECT_STREQ(roiStr, "5,5-50,50=dqp:-10;");

    uint32_t totalCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount(roiStr, &totalCount));
    EXPECT_EQ(totalCount, 1);

    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: RoiString_RoundTrip_ParseMixedThenReparse_033
 * @tc.desc: Test: parse mixed valid/invalid string, append parsed results, then re-parse
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_RoundTrip_ParseMixedThenReparse_033, TestSize.Level1)
{
    const char *mixedStr = "0,0-10,10=dqp:5;1,2-3,4=abc;5,6-7,8=slb:1;";
    OH_AVFormat *parsed[4] = {};
    uint32_t parsedCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(mixedStr, parsed, 4, &parsedCount));
    ASSERT_EQ(parsedCount, 2);

    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, parsed[0]));
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, parsed[1]));
    EXPECT_STREQ(roiStr, "0,0-10,10=dqp:5;5,6-7,8=slb:1;");

    OH_AVFormat *reparsed[4] = {};
    uint32_t reparsedCount = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(roiStr, reparsed, 4, &reparsedCount));
    ASSERT_EQ(reparsedCount, 2);

    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(reparsed[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 5);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(reparsed[1], OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &val));
    EXPECT_EQ(val, 1);

    free(roiStr);
    DestroyFormats(parsed, parsedCount);
    DestroyFormats(reparsed, reparsedCount);
}

/**
 * @tc.name: RoiString_ParseRoiString_DuplicateCoords_034
 * @tc.desc: Test DedupFormats: identical coords and kv values are deduplicated
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_DuplicateCoords_034, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10=dqp:5;0,0-10,10=dqp:5;", formats, 4, &count));
    EXPECT_EQ(count, 1);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_DuplicateCoordsNoKv_035
 * @tc.desc: Test DedupFormats: identical coords without kv are deduplicated
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_DuplicateCoordsNoKv_035, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10;0,0-10,10;", formats, 4, &count));
    EXPECT_EQ(count, 1);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_SameCoordsDifferentDqp_036
 * @tc.desc: Test IsSameRoiFormat: same coords with different dqp are NOT deduplicated
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_SameCoordsDifferentDqp_036, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10=dqp:5;0,0-10,10=dqp:3;", formats, 4, &count));
    ASSERT_EQ(count, 2);
    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 5);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[1], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    EXPECT_EQ(val, 3);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_SameCoordsOneWithDqp_037
 * @tc.desc: Test IsSameRoiFormat: same coords, one has dqp the other doesn't → NOT deduplicated
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_SameCoordsOneWithDqp_037, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10=dqp:5;0,0-10,10;", formats, 4, &count));
    ASSERT_EQ(count, 2);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_SameCoordsDifferentSlb_038
 * @tc.desc: Test IsSameRoiFormat: same coords with different slb are NOT deduplicated
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_SameCoordsDifferentSlb_038, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10=slb:1;0,0-10,10=slb:0;", formats, 4, &count));
    ASSERT_EQ(count, 2);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_SameCoordsOneWithSlb_039
 * @tc.desc: Test IsSameRoiFormat: same coords, one has slb the other doesn't → NOT deduplicated
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_SameCoordsOneWithSlb_039, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10=slb:1;0,0-10,10;", formats, 4, &count));
    ASSERT_EQ(count, 2);
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_ParseRoiString_SlbOnly_040
 * @tc.desc: Test ParseRoiString with slb only (no dqp) in new format
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_ParseRoiString_SlbOnly_040, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_ParseRoiString(
        "0,0-10,10=slb:1;", formats, 4, &count));
    ASSERT_EQ(count, 1);
    int32_t val = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, &val));
    EXPECT_EQ(val, 1);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(formats[0], OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, &val));
    DestroyFormats(formats, count);
}

/**
 * @tc.name: RoiString_GetRoiCount_DqpOverRange_041
 * @tc.desc: Test GetRoiCount rejects segment with dqp value over max range (>51)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_DqpOverRange_041, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=dqp:60;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_DqpUnderRange_042
 * @tc.desc: Test GetRoiCount rejects segment with dqp value under min range (<-51)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_DqpUnderRange_042, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=dqp:-60;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_SlbOverRange_043
 * @tc.desc: Test GetRoiCount rejects segment with slb value over max range (>1)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_SlbOverRange_043, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=slb:2;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_SlbUnderRange_044
 * @tc.desc: Test GetRoiCount rejects segment with slb value under min range (<0)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_SlbUnderRange_044, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("1,2-3,4=slb:-1;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_OldFormatDqpOverRange_045
 * @tc.desc: Test GetRoiCount rejects old format segment with dqp > 51
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_OldFormatDqpOverRange_045, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("0,0-10,10=52;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_GetRoiCount_OldFormatDqpUnderRange_046
 * @tc.desc: Test GetRoiCount rejects old format segment with dqp < -51
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_GetRoiCount_OldFormatDqpUnderRange_046, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("0,0-10,10=-52;", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: RoiString_AppendRoiString_InvalidDqpOverRange_047
 * @tc.desc: Test AppendRoiString rejects format with dqp value over range (>51)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_InvalidDqpOverRange_047, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 10, 10);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, 60);
    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: RoiString_AppendRoiString_InvalidSlbOverRange_048
 * @tc.desc: Test AppendRoiString rejects format with slb value over range (>1)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, RoiString_AppendRoiString_InvalidSlbOverRange_048, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 10, 10);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 2);
    char *roiStr = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    OH_AVFormat_Destroy(fmt);
}