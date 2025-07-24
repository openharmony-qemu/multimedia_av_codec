/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file expect in compliance with the License.
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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "avcodec_errors.h"
#include "instance_memory_update_event_handler.h"
#define TEST_SUIT InstanceMemoryCoverageUintTest

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MediaAVCodec {
constexpr int32_t MEMORY_LEAK_UPLOAD_TIMEOUT = 180; // seconds
class TEST_SUIT : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<InstanceMemoryUpdateEventHandler> instanceMemoryHandler_ = nullptr;
};

void TEST_SUIT::SetUpTestCase(void) {}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    instanceMemoryHandler_ = std::make_shared<InstanceMemoryUpdateEventHandler>();
}

void TEST_SUIT::TearDown(void)
{
    instanceMemoryHandler_ = nullptr;
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_001
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_001, TestSize.Level1)
{
    pid_t pid = getpid();
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->timerMap_.clear();
    instanceMemoryHandler_->appMemoryExceedThresholdList_.clear();
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_002
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_002, TestSize.Level1)
{
    pid_t pid = getpid();
    instanceMemoryHandler_->appMemoryThreshold_ = INT32_MAX;
    instanceMemoryHandler_->timerMap_.clear();
    instanceMemoryHandler_->appMemoryExceedThresholdList_.clear();
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_003
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_003, TestSize.Level1)
{
    pid_t pid = getpid();
    auto timeName = std::string("Pid_") + std::to_string(pid) + " memory exceeded threshold";
    instanceMemoryHandler_->timerMap_.emplace(
        pid, std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT, nullptr));
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->appMemoryExceedThresholdList_.clear();
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_004
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_004, TestSize.Level1)
{
    pid_t pid = getpid();
    auto timeName = std::string("Pid_") + std::to_string(pid) + " memory exceeded threshold";
    instanceMemoryHandler_->timerMap_.emplace(
        pid, std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT, nullptr));
    instanceMemoryHandler_->appMemoryThreshold_ = INT32_MAX;
    instanceMemoryHandler_->appMemoryExceedThresholdList_.clear();
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_005
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_005, TestSize.Level1)
{
    pid_t pid = getpid();
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->timerMap_.clear();
    instanceMemoryHandler_->appMemoryExceedThresholdList_.emplace(pid);
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_006
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_006, TestSize.Level1)
{
    pid_t pid = getpid();
    instanceMemoryHandler_->appMemoryThreshold_ = INT32_MAX;
    instanceMemoryHandler_->timerMap_.clear();
    instanceMemoryHandler_->appMemoryExceedThresholdList_.emplace(pid);
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_007
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_007, TestSize.Level1)
{
    pid_t pid = getpid();
    auto timeName = std::string("Pid_") + std::to_string(pid) + " memory exceeded threshold";
    instanceMemoryHandler_->timerMap_.emplace(
        pid, std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT, nullptr));
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->appMemoryExceedThresholdList_.emplace(pid);
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_008
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_008, TestSize.Level1)
{
    pid_t pid = getpid();
    auto timeName = std::string("Pid_") + std::to_string(pid) + " memory exceeded threshold";
    instanceMemoryHandler_->timerMap_.emplace(
        pid, std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT, nullptr));
    instanceMemoryHandler_->appMemoryThreshold_ = INT32_MAX;
    instanceMemoryHandler_->appMemoryExceedThresholdList_.emplace(pid);
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
}
} // namespace MediaAVCodec
} // namespace OHOS