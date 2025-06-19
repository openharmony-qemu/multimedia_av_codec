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

private:
    shared_ptr<ReferenceParserDemo> refParserDemo_ = nullptr;
};

void ReferenceParserTest::SetUpTestCase() {}

void ReferenceParserTest::TearDownTestCase() {}

void ReferenceParserTest::SetUp()
{
    refParserDemo_ = make_shared<ReferenceParserDemo>();
}

void ReferenceParserTest::TearDown()
{
    refParserDemo_ = nullptr;
}
} // namespace

namespace {
constexpr int32_t WAIT_TIME = 5;
constexpr int64_t DEC_COST_TIME_MS = 6L;
/**
 * @tc.number       : RP_FUNC_SEEK_IPB_0_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_IPB_0_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPB_0));
        ASSERT_EQ(true, refParserDemo_->DoAccurateSeek(1566)); // 1566 is frame 99
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_SEEK_IPB_0_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_IPB_1_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPB_1));
        ASSERT_EQ(true, refParserDemo_->DoAccurateSeek(1599)); // 1599 is frame 50
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_SEEK_IPPP_0_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_IPPP_0_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_0));
        ASSERT_EQ(true, refParserDemo_->DoAccurateSeek(500)); // 500 is frame 30
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_SEEK_IPPP_1_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_IPPP_1_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_1));
        ASSERT_EQ(true, refParserDemo_->DoAccurateSeek(27840)); // 27840 is frame 696
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_SEEK_IPPP_SCALA_0_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_IPPP_SCALA_0_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_SCALA_0));
        ASSERT_EQ(true, refParserDemo_->DoAccurateSeek(20040)); // 20040 is frame 501
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_SEEK_IPPP_SCALA_1_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_IPPP_SCALA_1_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_SCALA_1));
        ASSERT_EQ(true, refParserDemo_->DoAccurateSeek(200)); // 200 is frame 5
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_SEEK_SDTP_0100
 * @tc.name         : do accurate seek
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_SEEK_SDTP_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::SDTP));
        ASSERT_EQ(true, refParserDemo_->DoAccurateSeek(10266)); // 10266 is frame 310
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_IPB_0_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_IPB_0_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPB_0));
        ASSERT_EQ(true, refParserDemo_->DoVariableSpeedPlay(416)); // 416 is frame 30
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_IPB_1_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_IPB_1_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPB_1));
        ASSERT_EQ(true, refParserDemo_->DoVariableSpeedPlay(1133)); // 1133 is frame 36
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_IPPP_0_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_IPPP_0_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_0));
        ASSERT_EQ(true, refParserDemo_->DoVariableSpeedPlay(23966)); // 23966 is frame 1438
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_IPPP_1_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_IPPP_1_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_1));
        ASSERT_EQ(true, refParserDemo_->DoVariableSpeedPlay(23880)); // 23880 is frame 597
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_IPPP_SCALA_0_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_IPPP_SCALA_0_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_SCALA_0));
        ASSERT_EQ(true, refParserDemo_->DoVariableSpeedPlay(10680)); // 10680 is frame 267
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_IPPP_SCALA_1_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_IPPP_SCALA_1_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::IPPP_SCALA_1));
        ASSERT_EQ(true, refParserDemo_->DoVariableSpeedPlay(400)); // 400 is frame 10
        sleep(WAIT_TIME);
    }
}

/**
 * @tc.number       : RP_FUNC_VAR_SPEED_SDTP_0100
 * @tc.name         : do var speed
 * @tc.desc         : func test
 */
HWTEST_F(ReferenceParserTest, RP_FUNC_VAR_SPEED_SDTP_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        refParserDemo_->SetDecIntervalMs(DEC_COST_TIME_MS);
        ASSERT_EQ(0, refParserDemo_->InitScene(MP4Scene::SDTP));
        ASSERT_EQ(true, refParserDemo_->DoVariableSpeedPlay(8233)); // 8233 is frame 249
        sleep(WAIT_TIME);
    }
}
} // namespace