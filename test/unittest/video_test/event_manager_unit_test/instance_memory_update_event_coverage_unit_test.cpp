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
#include "avcodec_server.h"
#include "avcodec_server_manager.h"
#include "codec_service_stub.h"
#include "codeclist_service_stub.h"
#include "instance_memory_update_event_handler.h"
#include "iservice_registry.h"
#include "mem_mgr_client.h"
#include "system_ability.h"
#include "system_ability_definition.h"

#define TEST_SUIT InstanceMemoryCoverageUintTest

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace OHOS::Memory;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MediaAVCodec {
constexpr int32_t MEMORY_LEAK_UPLOAD_TIMEOUT = 180; // seconds
constexpr uint32_t DEFAULT_MEMORY = 100 * 1024;
class TEST_SUIT : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    std::shared_ptr<AVCodecServer> CreateAVCodecServer();
    sptr<IRemoteObject> CreateCodecServiceStub(std::shared_ptr<AVCodecServer> &server);

    std::shared_ptr<SystemAbilityMock> saMock_ = nullptr;
    std::shared_ptr<AVCodecServiceStubMock> avcodecStubMock_ = nullptr;
    std::shared_ptr<CodecServiceStubMock> codecStubMock_ = nullptr;
    std::shared_ptr<InstanceMemoryUpdateEventHandler> instanceMemoryHandler_ = nullptr;
    static int32_t instanceId_;
    bool existInstance_ = false;

private:
    std::vector<std::pair<AVCodecServerManager::StubType, sptr<IRemoteObject>>> stubList_;
};

int32_t TEST_SUIT::instanceId_ = 0;

void TEST_SUIT::SetUpTestCase(void) {}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    saMock_ = std::make_shared<SystemAbilityMock>();
    SystemAbility::RegisterMock(saMock_);
    avcodecStubMock_ = std::make_shared<AVCodecServiceStubMock>();
    AVCodecServiceStub::RegisterMock(avcodecStubMock_);
    codecStubMock_ = std::make_shared<CodecServiceStubMock>();
    CodecServiceStub::RegisterMock(codecStubMock_);
    instanceMemoryHandler_ = std::make_shared<InstanceMemoryUpdateEventHandler>();
    existInstance_ = false;
}

void TEST_SUIT::TearDown(void)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    for (auto &val : stubList_) {
        manager.DestroyStubObject(val.first, val.second);
    }
    stubList_.clear();
    saMock_ = nullptr;
    avcodecStubMock_ = nullptr;
    codecStubMock_ = nullptr;
    instanceMemoryHandler_ = nullptr;
}

std::shared_ptr<AVCodecServer> TEST_SUIT::CreateAVCodecServer()
{
    std::shared_ptr<AVCodecServer> server = nullptr;
    EXPECT_CALL(*saMock_, SystemAbilityCtor(AV_CODEC_SERVICE_ID, true)).Times(1);
    EXPECT_CALL(*saMock_, SystemAbilityDtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubCtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubDtor()).Times(1);

    server = std::make_shared<AVCodecServer>(AV_CODEC_SERVICE_ID, true);
    EXPECT_NE(server, nullptr);
    return server;
}

sptr<IRemoteObject> TEST_SUIT::CreateCodecServiceStub(std::shared_ptr<AVCodecServer> &server)
{
    EXPECT_NE(server, nullptr);
    if (server == nullptr) {
        return nullptr;
    }
    sptr<IRemoteObject> listener = sptr<IRemoteObject>(new AVCodecListenerStub());
    sptr<IRemoteObject> codecStub = nullptr;
    EXPECT_CALL(*avcodecStubMock_, SetDeathListener(listener)).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecStubMock_, Create()).Times(AtLeast(1)).WillOnce(Return(new CodecServiceStub()));
    EXPECT_CALL(*codecStubMock_, CodecServiceStubDtor()).Times(AtLeast(1));
    int32_t ret = server->GetSubSystemAbility(IStandardAVCodecService::AVCODEC_CODEC, listener, codecStub);
    EXPECT_NE(codecStub, nullptr);
    EXPECT_EQ(ret, AVCS_ERR_OK);
    stubList_.emplace_back(AVCodecServerManager::StubType::CODEC, codecStub);
    existInstance_ = true;
    return codecStub;
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_001
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_001, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    (void)CreateCodecServiceStub(server);
    pid_t pid = getpid();
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->timerMap_.clear();
    instanceMemoryHandler_->UpdateInstanceMemory(instanceId_, DEFAULT_MEMORY);
    instanceMemoryHandler_->appMemoryExceedThresholdList_.clear();
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
    instanceId_++;
    EXPECT_TRUE(existInstance_);
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
    EXPECT_FALSE(existInstance_);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_003
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_003, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    (void)CreateCodecServiceStub(server);
    pid_t pid = getpid();
    auto timeName = std::string("Pid_") + std::to_string(pid) + " memory exceeded threshold";
    instanceMemoryHandler_->timerMap_.emplace(
        pid, std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT, nullptr));
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->UpdateInstanceMemory(instanceId_, DEFAULT_MEMORY);
    instanceMemoryHandler_->appMemoryExceedThresholdList_.clear();
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
    instanceId_++;
    EXPECT_TRUE(existInstance_);
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
    EXPECT_FALSE(existInstance_);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_005
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_005, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    (void)CreateCodecServiceStub(server);
    pid_t pid = getpid();
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->timerMap_.clear();
    instanceMemoryHandler_->UpdateInstanceMemory(instanceId_, DEFAULT_MEMORY);
    instanceMemoryHandler_->appMemoryExceedThresholdList_.emplace(pid);
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
    instanceId_++;
    EXPECT_TRUE(existInstance_);
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
    EXPECT_FALSE(existInstance_);
}

/**
 * @tc.name: DeterminAppMemoryExceedThresholdAndReport_Test_007
 * @tc.desc: add the coverage of DeterminAppMemoryExceedThresholdAndReport function
 */
HWTEST_F(TEST_SUIT, DeterminAppMemoryExceedThresholdAndReport_Test_007, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    (void)CreateCodecServiceStub(server);
    pid_t pid = getpid();
    auto timeName = std::string("Pid_") + std::to_string(pid) + " memory exceeded threshold";
    instanceMemoryHandler_->timerMap_.emplace(
        pid, std::make_shared<AVCodecXcollieTimer>(timeName, false, MEMORY_LEAK_UPLOAD_TIMEOUT, nullptr));
    instanceMemoryHandler_->appMemoryThreshold_ = 0;
    instanceMemoryHandler_->UpdateInstanceMemory(instanceId_, DEFAULT_MEMORY);
    instanceMemoryHandler_->appMemoryExceedThresholdList_.emplace(pid);
    instanceMemoryHandler_->DeterminAppMemoryExceedThresholdAndReport(pid, pid);
    instanceId_++;
    EXPECT_TRUE(existInstance_);
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
    EXPECT_FALSE(existInstance_);
}
} // namespace MediaAVCodec
} // namespace OHOS