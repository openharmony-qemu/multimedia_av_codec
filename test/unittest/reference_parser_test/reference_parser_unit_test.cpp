/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "securec.h"
#include "reference_parser_demo.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
class ReferenceParserTest : public testing::Test {
public:
    // SetUpTestCase: CALLed before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: CALLed after all test cases
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases;
    void TearDown(void);
};

void ReferenceParserTest::SetUpTestCase() {}
void ReferenceParserTest::TearDownTestCase() {}
void ReferenceParserTest::SetUp() {}
void ReferenceParserTest::TearDown() {}
} // namespace

namespace {
constexpr int64_t DEC_COST_TIME_MS = 6L;
/**
 * @tc.number       : RP_FUNC_SEEK_IPBBB_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_IPB_0_0100, TestSize.Level1)
{
    shared_ptr<ReferenceParserDemo> refParserDemo = make_shared<ReferenceParserDemo>();
    refParserDemo->SetDecIntervalMs(DEC_COST_TIME_MS);
    ASSERT_EQ(0, refParserDemo->InitScene(MP4Scene::IPB_0));
    ASSERT_EQ(true, refParserDemo->DoAccurateSeek(1566)); // 1566 is frame 99
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_IPBBB_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_IPB_0_0100, TestSize.Level1)
{
    shared_ptr<ReferenceParserDemo> refParserDemo = make_shared<ReferenceParserDemo>();
    refParserDemo->SetDecIntervalMs(DEC_COST_TIME_MS);
    ASSERT_EQ(0, refParserDemo->InitScene(MP4Scene::IPB_0));
    ASSERT_EQ(true, refParserDemo->DoVariableSpeedPlay(416)); // 416 is frame 30
}

} // namespace