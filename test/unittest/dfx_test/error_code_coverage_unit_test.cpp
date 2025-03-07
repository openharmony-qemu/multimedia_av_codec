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

#include <string>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "native_averrors.h"

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
    constexpr OH_AVErrCode OHAVCODEC_ERROR_CODE = AV_ERR_OK;
    const std::string UNKOWN_ERROR = "unkown error";
};

namespace OHOS::MediaAVCodec {
class ErrorCodeCoverageUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
    void TearDown(void) {};
};

/**
 * @tc.name: OHAVErrCodeToString_Coverage_Unit_Test_001
 * @tc.desc: input valid value
 */
HWTEST_F(ErrorCodeCoverageUnitTest, OHAVErrCodeToString_Coverage_Unit_Test_001, TestSize.Level1)
{
    std::string ret = OHAVErrCodeToString(OHAVCODEC_ERROR_CODE);
    EXPECT_NE(ret, UNKOWN_ERROR);
}

/**
 * @tc.name: OHAVErrCodeToString_Coverage_Unit_Test_002
 * @tc.desc: input invalid value
 */
HWTEST_F(ErrorCodeCoverageUnitTest, OHAVErrCodeToString_Coverage_Unit_Test_002, TestSize.Level1)
{
    constexpr int32_t INVALID_VALUE = -1;
    std::string ret = OHAVErrCodeToString(static_cast<OH_AVErrCode>(INVALID_VALUE));
    EXPECT_EQ(ret, UNKOWN_ERROR);
}

/**
 * @tc.name: AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_001
 * @tc.desc: error code belongs to AVCS_ERRCODE_INFOS not AVCSERRCODE_TO_OHAVCODECERRCODE
 */
HWTEST_F(ErrorCodeCoverageUnitTest, AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_001, TestSize.Level1)
{
    constexpr int32_t ERROR_CODE = AVCS_ERR_VIDEO_UNSUPPORT_COLOR_SPACE_CONVERSION;
    std::string ret = AVCSErrorToOHAVErrCodeString(static_cast<AVCodecServiceErrCode>(ERROR_CODE));
    EXPECT_NE(ret, UNKOWN_ERROR);
}

/**
 * @tc.name: AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_002
 * @tc.desc: error code belongs to AVCS_ERRCODE_INFOS and AVCSERRCODE_TO_OHAVCODECERRCODE
 */
HWTEST_F(ErrorCodeCoverageUnitTest, AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_002, TestSize.Level1)
{
    constexpr int32_t ERROR_CODE = AVCS_ERR_OK;
    std::string ret = AVCSErrorToOHAVErrCodeString(static_cast<AVCodecServiceErrCode>(ERROR_CODE));
    EXPECT_NE(ret, UNKOWN_ERROR);
}

/**
 * @tc.name: AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_003
 * @tc.desc: error code belongs to AVCSERRCODE_TO_OHAVCODECERRCODE not AVCS_ERRCODE_INFOS
 */
HWTEST_F(ErrorCodeCoverageUnitTest, AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_003, TestSize.Level1)
{
    constexpr int32_t ERROR_CODE = OH_AVErrCode::AV_ERR_NO_MEMORY;
    std::string ret = AVCSErrorToOHAVErrCodeString(static_cast<AVCodecServiceErrCode>(ERROR_CODE));
    EXPECT_EQ(ret, UNKOWN_ERROR);
}

/**
 * @tc.name: AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_004
 * @tc.desc: error code not belong to AVCS_ERRCODE_INFOS and AVCSERRCODE_TO_OHAVCODECERRCODE
 */
HWTEST_F(ErrorCodeCoverageUnitTest, AVCSErrorToOHAVErrCodeString_Coverage_Unit_Test_004, TestSize.Level1)
{
    constexpr int32_t INVALID_VALUE = -1;
    std::string ret = AVCSErrorToOHAVErrCodeString(static_cast<AVCodecServiceErrCode>(INVALID_VALUE));
    EXPECT_EQ(ret, UNKOWN_ERROR);
}
}