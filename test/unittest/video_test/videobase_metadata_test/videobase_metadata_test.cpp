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
#include <stdlib.h>
#include <string.h>
#include "native_avcodec_videobase.h"
#include "native_avformat.h"
#define TEST_SUIT VideoMetadataRoiTest

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
 * @tc.name: VideoMetadataRoi_AppendRoiString_NullRoiStrInOut_001
 * @tc.desc: Test AppendRoiString with null double pointer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_AppendRoiString_NullRoiStrInOut_001, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 10, 10);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_AppendRoiString(NULL, fmt));
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: VideoMetadataRoi_AppendRoiString_NullFormat_001
 * @tc.desc: Test AppendRoiString with null format
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_AppendRoiString_NullFormat_001, TestSize.Level1)
{
    char *roiStr = NULL;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_AppendRoiString(&roiStr, NULL));
}

/**
 * @tc.name: VideoMetadataRoi_AppendRoiString_CoordsOnly_001
 * @tc.desc: Test AppendRoiString with only coordinates
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_AppendRoiString_CoordsOnly_001, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 10, 10);
    char *roiStr = NULL;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-10,10;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: VideoMetadataRoi_AppendRoiString_WithDqp_001
 * @tc.desc: Test AppendRoiString with dqp only
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_AppendRoiString_WithDqp_001, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 100, 100);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, 5);
    char *roiStr = NULL;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-100,100=dqp:5;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: VideoMetadataRoi_AppendRoiString_WithDqpAndSemLabel_001
 * @tc.desc: Test AppendRoiString with both dqp and semLabel
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_AppendRoiString_WithDqpAndSemLabel_001, TestSize.Level1)
{
    OH_AVFormat *fmt = CreateRoiFormat(0, 0, 50, 60);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_DELTA_QP, -3);
    OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_METADATA_ROI_SEM_LABEL, 1);
    char *roiStr = NULL;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-50,60=dqp:-3,slb:1;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: VideoMetadataRoi_AppendRoiString_SeparatorProtection_001
 * @tc.desc: Test AppendRoiString when existing string lacks trailing semicolon
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_AppendRoiString_SeparatorProtection_001, TestSize.Level1)
{
    char *roiStr = (char *)malloc(16);
    strcpy(roiStr, "0,0-100,100=5");
    OH_AVFormat *fmt = CreateRoiFormat(10, 20, 200, 300);
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_AppendRoiString(&roiStr, fmt));
    ASSERT_NE(roiStr, nullptr);
    EXPECT_STREQ(roiStr, "0,0-100,100=5;10,20-200,300;");
    free(roiStr);
    OH_AVFormat_Destroy(fmt);
}

/**
 * @tc.name: VideoMetadataRoi_GetRoiCount_NullRoiStr_001
 * @tc.desc: Test GetRoiCount with null roiStr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_GetRoiCount_NullRoiStr_001, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_GetRoiCount(NULL, &count));
}

/**
 * @tc.name: VideoMetadataRoi_GetRoiCount_EmptyString_001
 * @tc.desc: Test GetRoiCount with empty string
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_GetRoiCount_EmptyString_001, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("", &count));
    EXPECT_EQ(count, 0);
}

/**
 * @tc.name: VideoMetadataRoi_GetRoiCount_SingleOldFormat_001
 * @tc.desc: Test GetRoiCount with single old-format ROI string
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_GetRoiCount_SingleOldFormat_001, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("0,0-100,100=5;", &count));
    EXPECT_EQ(count, 1);
}

/**
 * @tc.name: VideoMetadataRoi_GetRoiCount_MultipleRois_001
 * @tc.desc: Test GetRoiCount with multiple ROI regions
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_GetRoiCount_MultipleRois_001, TestSize.Level1)
{
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_OK, OH_VideoMetadata_GetRoiCount("0,0-100,100;10,20-200,300=dqp:3;", &count));
    EXPECT_EQ(count, 2);
}

/**
 * @tc.name: VideoMetadataRoi_ParseRoiString_NullRoiStr_001
 * @tc.desc: Test ParseRoiString with null roiStr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_ParseRoiString_NullRoiStr_001, TestSize.Level1)
{
    OH_AVFormat *formats[4] = {};
    uint32_t count = 0;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoMetadata_ParseRoiString(NULL, formats, 4, &count));
}

/**
 * @tc.name: VideoMetadataRoi_ParseRoiString_OldFormatDeltaQP_001
 * @tc.desc: Test ParseRoiString with old format "=deltaQP"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_ParseRoiString_OldFormatDeltaQP_001, TestSize.Level1)
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
 * @tc.name: VideoMetadataRoi_ParseRoiString_NewFormatSlbAndDqp_001
 * @tc.desc: Test ParseRoiString with reversed order "=slb:val,dqp:val"
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_ParseRoiString_NewFormatSlbAndDqp_001, TestSize.Level1)
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
 * @tc.name: VideoMetadataRoi_ParseRoiString_NoCrossSegmentParse_001
 * @tc.desc: Test that old-format "=5" is not confused by dqp in later segments
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_ParseRoiString_NoCrossSegmentParse_001, TestSize.Level1)
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
 * @tc.name: VideoMetadataRoi_RoundTrip_AppendAndParse_001
 * @tc.desc: Test round-trip: append multiple ROIs then parse back
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoMetadataRoi_RoundTrip_AppendAndParse_001, TestSize.Level1)
{
    char *roiStr = NULL;
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