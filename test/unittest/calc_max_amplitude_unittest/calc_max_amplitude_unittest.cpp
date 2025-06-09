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

#define MEDIA_OHOS 1
#include <cstdint>
#include "calc_max_amplitude_unittest.h"

namespace OHOS {
namespace Media {
namespace CalcMaxAmplitude {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int32_t INT_1 = 1;
static const float FLOAT_1 = 1.0f;

void CalcMaxAmplitudeUnitTest::SetUpTestCase(void)
{
}

void CalcMaxAmplitudeUnitTest::TearDownTestCase(void)
{
}

void CalcMaxAmplitudeUnitTest::SetUp(void)
{
}

void CalcMaxAmplitudeUnitTest::TearDown(void)
{
}

/**
 * @tc.name  : Test CalculateMaxAmplitudeForPCM8Bit
 * @tc.number: CalculateMaxAmplitudeForPCM8Bit_001
 * @tc.desc  : Test value == INT8_MIN
 */
HWTEST_F(CalcMaxAmplitudeUnitTest, CalculateMaxAmplitudeForPCM8Bit_001, TestSize.Level0)
{
    int8_t data[1] = {INT8_MIN};
    float result = CalculateMaxAmplitudeForPCM8Bit(data, INT_1);
    EXPECT_FLOAT_EQ(result, FLOAT_1);
}

/**
 * @tc.name  : Test CalculateMaxAmplitudeForPCM24Bit
 * @tc.number: CalculateMaxAmplitudeForPCM24Bit_001
 * @tc.desc  : Test Test curValue < 0
 */
HWTEST_F(CalcMaxAmplitudeUnitTest, CalculateMaxAmplitudeForPCM24Bit_001, TestSize.Level0)
{
    char frame[3];
    frame[0] = 0x00;
    frame[1] = 0x00;
    frame[2] = static_cast<char>(0x80);

    float result = CalculateMaxAmplitudeForPCM24Bit(frame, INT_1);
    EXPECT_FLOAT_EQ(result, FLOAT_1);
}

/**
 * @tc.name  : Test CalculateMaxAmplitudeForPCM32Bit
 * @tc.number: CalculateMaxAmplitudeForPCM32Bit_001
 * @tc.desc  : Test value == INT32_MIN
 */
HWTEST_F(CalcMaxAmplitudeUnitTest, CalculateMaxAmplitudeForPCM32Bit_001, TestSize.Level0)
{
    int32_t data[1] = {INT32_MIN};
    float result = CalculateMaxAmplitudeForPCM32Bit(data, INT_1);
    EXPECT_FLOAT_EQ(result, FLOAT_1);
}

} // namespace CalcMaxAmplitude
} // namespace Media
} // namespace OHOS