/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <chrono>
#include <fcntl.h>
#include <future>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <thread>
#include <vector>
#include "avcodec_errors.h"
#include "avcodec_server.h"
#include "avcodec_server_manager.h"
#include "codec_service_stub.h"
#include "codeclist_service_stub.h"
#include "event_info_extented_key.h"
#include "event_manager.h"
#include "iservice_registry.h"
#include "mem_mgr_client.h"
#include "statistics_event_handler.h"
#include "system_ability.h"
#include "system_ability_definition.h"

using namespace OHOS;
using namespace OHOS::Memory;
using namespace OHOS::MediaAVCodec;
using namespace testing;
using namespace testing::ext;

namespace {
const std::string DUMP_FILE_PATH = "/data/test/media/dump.txt";
constexpr int32_t INVALID_NUM = -1;
} // namespace

namespace {
class SaAVCodecUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<AVCodecServer> CreateAVCodecServer();
    sptr<IRemoteObject> CreateCodecServiceStub(std::shared_ptr<AVCodecServer> &server);

    std::shared_ptr<SystemAbilityMock> saMock_;
    std::shared_ptr<MemMgrClientMock> memMgrMock_;
    std::shared_ptr<AVCodecServiceStubMock> avcodecStubMock_;
    std::shared_ptr<CodecServiceStubMock> codecStubMock_;
    std::shared_ptr<CodecListServiceStubMock> codeclistStubMock_;

private:
    std::vector<std::pair<AVCodecServerManager::StubType, sptr<IRemoteObject>>> stubList_;
};

void SaAVCodecUnitTest::SetUpTestCase(void) {}

void SaAVCodecUnitTest::TearDownTestCase(void)
{
    StatisticsEventInfo::GetInstance().timer_ = nullptr;
}

void SaAVCodecUnitTest::SetUp(void)
{
    saMock_ = std::make_shared<SystemAbilityMock>();
    SystemAbility::RegisterMock(saMock_);
    memMgrMock_ = std::make_shared<MemMgrClientMock>();
    MemMgrClient::RegisterMock(memMgrMock_);
    avcodecStubMock_ = std::make_shared<AVCodecServiceStubMock>();
    AVCodecServiceStub::RegisterMock(avcodecStubMock_);
    codecStubMock_ = std::make_shared<CodecServiceStubMock>();
    CodecServiceStub::RegisterMock(codecStubMock_);
    codeclistStubMock_ = std::make_shared<CodecListServiceStubMock>();
    CodecListServiceStub::RegisterMock(codeclistStubMock_);
}

void SaAVCodecUnitTest::TearDown(void)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    for (auto &val : stubList_) {
        manager.DestroyStubObject(val.first, val.second);
    }
    stubList_.clear();
    saMock_ = nullptr;
    codecStubMock_ = nullptr;
    codeclistStubMock_ = nullptr;
}

std::shared_ptr<AVCodecServer> SaAVCodecUnitTest::CreateAVCodecServer()
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

sptr<IRemoteObject> SaAVCodecUnitTest::CreateCodecServiceStub(std::shared_ptr<AVCodecServer> &server)
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
    return codecStub;
}

/**
 * @tc.name: AVCodec_Server_Constructor_001
 * @tc.desc: id and bool is matched
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_Constructor_001, TestSize.Level1)
{
    EXPECT_CALL(*saMock_, SystemAbilityCtor(AV_CODEC_SERVICE_ID, true)).Times(1);
    EXPECT_CALL(*saMock_, SystemAbilityDtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubCtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubDtor()).Times(1);

    auto server = std::make_shared<AVCodecServer>(AV_CODEC_SERVICE_ID, true);
    EXPECT_NE(server, nullptr);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_Constructor_002
 * @tc.desc: id and bool is matched
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_Constructor_002, TestSize.Level1)
{
    EXPECT_CALL(*saMock_, SystemAbilityCtor(AV_CODEC_SERVICE_ID, false)).Times(1);
    EXPECT_CALL(*saMock_, SystemAbilityDtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubCtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubDtor()).Times(1);

    auto server = std::make_shared<AVCodecServer>(AV_CODEC_SERVICE_ID, false);
    EXPECT_NE(server, nullptr);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_Constructor_003
 * @tc.desc: id and bool is matched
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_Constructor_003, TestSize.Level1)
{
    EXPECT_CALL(*saMock_, SystemAbilityCtor(1, false)).Times(1);
    EXPECT_CALL(*saMock_, SystemAbilityDtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubCtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubDtor()).Times(1);

    auto server = std::make_shared<AVCodecServer>(1, false);
    EXPECT_NE(server, nullptr);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_OnStart_001
 * @tc.desc: 1. will once publish success
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnStart_001, TestSize.Level1)
{
    EXPECT_CALL(*saMock_, AddSystemAbilityListener(MEMORY_MANAGER_SA_ID)).Times(1);
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    EXPECT_CALL(*saMock_, Publish(server.get())).Times(1).WillOnce(Return(true));
    server->OnStart();
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_OnStart_002
 * @tc.desc: 1. will once publish fail
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnStart_002, TestSize.Level1)
{
    EXPECT_CALL(*saMock_, AddSystemAbilityListener(MEMORY_MANAGER_SA_ID)).Times(1);
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    EXPECT_CALL(*saMock_, Publish(server.get())).Times(1).WillOnce(Return(false));
    server->OnStart();
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_OnStop_001
 * @tc.desc: 1. will once NotifyProcessStatus = 0 success
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnStop_001, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    server->OnStop();
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_OnAddSystemAbility_001
 * @tc.desc: 1. will once NotifyProcessStatus = 1 success
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnAddSystemAbility_001, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    server->OnAddSystemAbility(MEMORY_MANAGER_SA_ID, "testId");
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_GetSubSystemAbility_001
 * @tc.desc: GetSubSystemAbility will twice SetDeathListener succuss
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_GetSubSystemAbility_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    IStandardAVCodecService::AVCodecSystemAbility subSystemId;
    sptr<IRemoteObject> listener = sptr<IRemoteObject>(new AVCodecListenerStub());

    EXPECT_CALL(*avcodecStubMock_, SetDeathListener(listener)).Times(2).WillRepeatedly(Return(AVCS_ERR_OK));
    EXPECT_CALL(*codecStubMock_, Create()).Times(1).WillOnce(Return(new CodecServiceStub()));
    EXPECT_CALL(*codecStubMock_, CodecServiceStubDtor()).Times(1);
    EXPECT_CALL(*codeclistStubMock_, Create()).Times(1).WillOnce(Return(new CodecListServiceStub()));
    EXPECT_CALL(*codeclistStubMock_, CodecListServiceStubDtor()).Times(1);

    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    subSystemId = IStandardAVCodecService::AVCODEC_CODEC;
    sptr<IRemoteObject> codecStub = nullptr;
    int32_t ret = server->GetSubSystemAbility(subSystemId, listener, codecStub);
    EXPECT_NE(codecStub, nullptr);
    EXPECT_EQ(ret, AVCS_ERR_OK);
    manager.DestroyStubObject(AVCodecServerManager::StubType::CODEC, codecStub);

    subSystemId = IStandardAVCodecService::AVCODEC_CODECLIST;
    sptr<IRemoteObject> codeclistStub = nullptr;
    ret = server->GetSubSystemAbility(subSystemId, listener, codeclistStub);
    EXPECT_NE(codeclistStub, nullptr);
    EXPECT_EQ(ret, AVCS_ERR_OK);
    manager.DestroyStubObject(AVCodecServerManager::StubType::CODECLIST, codeclistStub);

    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_GetSubSystemAbility_002
 * @tc.desc: GetSubSystemAbility will twice SetDeathListener fail
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_GetSubSystemAbility_002, TestSize.Level1)
{
    IStandardAVCodecService::AVCodecSystemAbility subSystemId;
    sptr<IRemoteObject> listener = sptr<IRemoteObject>(new AVCodecListenerStub());
    EXPECT_CALL(*avcodecStubMock_, SetDeathListener(listener))
        .Times(2)
        .WillRepeatedly(Return(AVCS_ERR_INVALID_OPERATION));

    EXPECT_CALL(*codecStubMock_, Create()).Times(1).WillOnce(Return(new CodecServiceStub()));
    EXPECT_CALL(*codecStubMock_, CodecServiceStubDtor()).Times(1);
    EXPECT_CALL(*codeclistStubMock_, Create()).Times(1).WillOnce(Return(new CodecListServiceStub()));
    EXPECT_CALL(*codeclistStubMock_, CodecListServiceStubDtor()).Times(1);

    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    subSystemId = IStandardAVCodecService::AVCODEC_CODEC;
    sptr<IRemoteObject> codecStub = nullptr;
    int32_t ret = server->GetSubSystemAbility(subSystemId, listener, codecStub);
    EXPECT_EQ(codecStub, nullptr);
    EXPECT_NE(ret, AVCS_ERR_OK);

    subSystemId = IStandardAVCodecService::AVCODEC_CODECLIST;
    sptr<IRemoteObject> codeclistStub = nullptr;
    ret = server->GetSubSystemAbility(subSystemId, listener, codeclistStub);
    EXPECT_EQ(codeclistStub, nullptr);
    EXPECT_NE(ret, AVCS_ERR_OK);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_GetSubSystemAbility_003
 * @tc.desc: GetSubSystemAbility will twice Stub create fail
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_GetSubSystemAbility_003, TestSize.Level1)
{
    IStandardAVCodecService::AVCodecSystemAbility subSystemId;
    sptr<IRemoteObject> listener = sptr<IRemoteObject>(new AVCodecListenerStub());

    EXPECT_CALL(*codecStubMock_, Create()).Times(1).WillOnce(nullptr);
    EXPECT_CALL(*codeclistStubMock_, Create()).Times(1).WillOnce(nullptr);

    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    subSystemId = IStandardAVCodecService::AVCODEC_CODEC;
    sptr<IRemoteObject> codecStub = nullptr;
    int32_t ret = server->GetSubSystemAbility(subSystemId, listener, codecStub);
    EXPECT_EQ(codecStub, nullptr);
    EXPECT_NE(ret, AVCS_ERR_OK);

    subSystemId = IStandardAVCodecService::AVCODEC_CODECLIST;
    sptr<IRemoteObject> codeclistStub = nullptr;
    ret = server->GetSubSystemAbility(subSystemId, listener, codeclistStub);
    EXPECT_EQ(codeclistStub, nullptr);
    EXPECT_NE(ret, AVCS_ERR_OK);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_GetSubSystemAbility_004
 * @tc.desc: GetSubSystemAbility invalid subSystemId
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_GetSubSystemAbility_004, TestSize.Level1)
{
    auto subSystemId = static_cast<IStandardAVCodecService::AVCodecSystemAbility>(INVALID_NUM);
    sptr<IRemoteObject> listener = sptr<IRemoteObject>(new AVCodecListenerStub());

    EXPECT_CALL(*codecStubMock_, Create()).Times(0);
    EXPECT_CALL(*codeclistStubMock_, Create()).Times(0);

    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);

    sptr<IRemoteObject> codecStub = nullptr;
    int32_t ret = server->GetSubSystemAbility(subSystemId, listener, codecStub);
    EXPECT_EQ(codecStub, nullptr);
    EXPECT_NE(ret, AVCS_ERR_OK);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_GetSubSystemAbility_005
 * @tc.desc: server manager createobject
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_GetSubSystemAbility_005, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    sptr<IRemoteObject> codecStub = nullptr;
    EXPECT_NE(manager.CreateStubObject(static_cast<AVCodecServerManager::StubType>(INVALID_NUM), codecStub),
              AVCS_ERR_OK);
    EXPECT_EQ(codecStub, nullptr);
}

void PrintAndCloseFd(int32_t fd)
{
    int32_t fileSize = lseek(fd, 0, SEEK_END);
    std::string str = std::string(fileSize + 1, ' ');
    lseek(fd, 0, SEEK_SET);
    read(fd, str.data(), fileSize);
    std::cout << "dump str: \n==========\n" << str << "\n==========\n";
    close(fd);
}

/**
 * @tc.name: AVCodec_Server_Dump_001
 * @tc.desc: DumpInfo will once Dump success
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_Dump_001, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    std::vector<std::u16string> args = {u"codec"};
    int32_t fileFd = open(DUMP_FILE_PATH.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);

    (void)CreateCodecServiceStub(server);
    EXPECT_EQ(server->Dump(fileFd, args), OHOS::NO_ERROR);
    PrintAndCloseFd(fileFd);
}

/**
 * @tc.name: AVCodec_ServerManager_DestroyStubObjectForPid_001
 * @tc.desc:
 *     1. server manager DestroyStubObjectForPid
 *     2. video decoder without forward caller
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_DestroyStubObjectForPid_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 123;
    InstanceInfo vdecInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = callerId},
    };
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, vdecInfo));
    manager.DestroyStubObjectForPid(INVALID_NUM);
    manager.DestroyStubObjectForPid(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_DestroyStubObjectForPid_002
 * @tc.desc:
 *     1. server manager DestroyStubObjectForPid
 *     2. video decoder with forward caller
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_DestroyStubObjectForPid_002, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 123;
    const int32_t fowardCallerId = 1234;
    InstanceInfo vdecInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = callerId},
        .forwardCaller = {.pid = fowardCallerId},
    };
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, vdecInfo));
    manager.DestroyStubObjectForPid(INVALID_NUM);
    manager.DestroyStubObjectForPid(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_DestroyStubObjectForPid_003
 * @tc.desc:
 *     1. server manager DestroyStubObjectForPid
 *     2. video encoder without forward caller
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_DestroyStubObjectForPid_003, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 123;
    InstanceInfo vdecInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId},
    };
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, vdecInfo));
    manager.DestroyStubObjectForPid(INVALID_NUM);
    manager.DestroyStubObjectForPid(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_DestroyStubObjectForPid_004
 * @tc.desc:
 *     1. server manager DestroyStubObjectForPid
 *     2. video encoder with forward caller
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_DestroyStubObjectForPid_004, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 123;
    const int32_t fowardCallerId = 1234;
    InstanceInfo vdecInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId},
        .forwardCaller = {.pid = fowardCallerId},
    };
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, vdecInfo));
    manager.DestroyStubObjectForPid(INVALID_NUM);
    manager.DestroyStubObjectForPid(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_SetCritical_SkipDuplicate_001
 * @tc.desc: SetCritical skip duplicate call when same value and last succeeded
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_SetCritical_SkipDuplicate_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    manager.memMgrStarted_ = true;
    manager.setCriticalFunc_ = MockSetCriticalForFuncPtr;
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(-1)); // -1: mock set critical failure
    manager.SetCritical(true);
    EXPECT_TRUE(manager.setCriticalFailed_);
    EXPECT_TRUE(manager.lastSetCriticalValue_);
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(0));
    manager.SetCritical(true);
    EXPECT_FALSE(manager.setCriticalFailed_);
    manager.SetCritical(true);
    EXPECT_CALL(*memMgrMock_, SetCritical(_, false, _)).Times(1).WillOnce(Return(0));
    manager.SetCritical(false);
    EXPECT_FALSE(manager.setCriticalFailed_);
    manager.SetCritical(false);
}

/**
 * @tc.name: AVCodec_ServerManager_SetCritical_FailedState_001
 * @tc.desc: SetCritical retry when last set failed
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_SetCritical_FailedState_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    manager.memMgrStarted_ = true;
    manager.setCriticalFunc_ = MockSetCriticalForFuncPtr;
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(-1)); // -1: mock set critical failure
    manager.SetCritical(true);
    EXPECT_TRUE(manager.setCriticalFailed_);
    EXPECT_TRUE(manager.lastSetCriticalValue_);
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(0));
    manager.SetCritical(true);
    EXPECT_FALSE(manager.setCriticalFailed_);
}

/**
 * @tc.name: AVCodec_ServerManager_SetCritical_FailedState_002
 * @tc.desc: SetCritical mark failed when setCriticalFunc_ fails
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_SetCritical_FailedState_002, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    manager.memMgrStarted_ = true;
    manager.setCriticalFunc_ = MockSetCriticalForFuncPtr;
    EXPECT_CALL(*memMgrMock_, SetCritical(_, false, _)).Times(1).WillOnce(Return(0));
    manager.SetCritical(false);
    EXPECT_FALSE(manager.setCriticalFailed_);
    EXPECT_FALSE(manager.lastSetCriticalValue_);
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(-1)); // -1: mock set critical failure
    manager.SetCritical(true);
    EXPECT_TRUE(manager.setCriticalFailed_);
    EXPECT_TRUE(manager.lastSetCriticalValue_);
}

/**
 * @tc.name: AVCodec_ServerManager_NotifyProcessStatus_Retry_001
 * @tc.desc: NotifyProcessStatus retry failed SetCritical(true) when status=1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_NotifyProcessStatus_Retry_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    manager.memMgrStarted_ = true;
    manager.notifyProcessStatusFunc_ = MockNotifyProcessStatusForFuncPtr;
    manager.setCriticalFunc_ = MockSetCriticalForFuncPtr;
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(-1)); // -1: mock set critical failure
    manager.SetCritical(true);
    EXPECT_TRUE(manager.setCriticalFailed_);
    EXPECT_TRUE(manager.lastSetCriticalValue_);
    EXPECT_CALL(*memMgrMock_, NotifyProcessStatus(_, _, 1, _)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(0));
    manager.NotifyProcessStatus(1);
    EXPECT_FALSE(manager.setCriticalFailed_);
}

/**
 * @tc.name: AVCodec_ServerManager_NotifyProcessStatus_Retry_002
 * @tc.desc: NotifyProcessStatus not retry SetCritical when status=0
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_NotifyProcessStatus_Retry_002, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    manager.memMgrStarted_ = true;
    manager.notifyProcessStatusFunc_ = MockNotifyProcessStatusForFuncPtr;
    manager.setCriticalFunc_ = MockSetCriticalForFuncPtr;
    manager.setCriticalFailed_ = false;
    manager.lastSetCriticalValue_ = false;
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(-1)); // -1: mock set critical failure
    manager.SetCritical(true);
    EXPECT_TRUE(manager.setCriticalFailed_);
    EXPECT_TRUE(manager.lastSetCriticalValue_);
    EXPECT_CALL(*memMgrMock_, NotifyProcessStatus(_, _, 0, _)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*memMgrMock_, SetCritical(_, _, _)).Times(0);
    manager.NotifyProcessStatus(0);
    EXPECT_TRUE(manager.setCriticalFailed_);
}

/**
 * @tc.name: AVCodec_ServerManager_NotifyProcessStatus_Retry_003
 * @tc.desc: NotifyProcessStatus not retry when last value was false
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_NotifyProcessStatus_Retry_003, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    manager.memMgrStarted_ = true;
    manager.notifyProcessStatusFunc_ = MockNotifyProcessStatusForFuncPtr;
    manager.setCriticalFunc_ = MockSetCriticalForFuncPtr;
    manager.setCriticalFailed_ = true;
    manager.lastSetCriticalValue_ = true;
    EXPECT_CALL(*memMgrMock_, SetCritical(_, false, _)).Times(1).WillOnce(Return(-1)); // -1: mock set critical failure
    manager.SetCritical(false);
    EXPECT_TRUE(manager.setCriticalFailed_);
    EXPECT_FALSE(manager.lastSetCriticalValue_);
    EXPECT_CALL(*memMgrMock_, NotifyProcessStatus(_, _, 1, _)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*memMgrMock_, SetCritical(_, _, _)).Times(0);
    manager.NotifyProcessStatus(1);
    EXPECT_TRUE(manager.setCriticalFailed_);
}

/**
 * @tc.name: AVCodec_ServerManager_NotifyProcessStatus_Retry_004
 * @tc.desc: NotifyProcessStatus not retry when last set succeeded
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_NotifyProcessStatus_Retry_004, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    manager.memMgrStarted_ = true;
    manager.notifyProcessStatusFunc_ = MockNotifyProcessStatusForFuncPtr;
    manager.setCriticalFunc_ = MockSetCriticalForFuncPtr;
    manager.setCriticalFailed_ = false;
    manager.lastSetCriticalValue_ = false;
    EXPECT_CALL(*memMgrMock_, SetCritical(_, true, _)).Times(1).WillOnce(Return(0));
    manager.SetCritical(true);
    EXPECT_FALSE(manager.setCriticalFailed_);
    EXPECT_TRUE(manager.lastSetCriticalValue_);
    EXPECT_CALL(*memMgrMock_, NotifyProcessStatus(_, _, 1, _)).Times(1).WillOnce(Return(0));
    EXPECT_CALL(*memMgrMock_, SetCritical(_, _, _)).Times(0);
    manager.NotifyProcessStatus(1);
    EXPECT_FALSE(manager.setCriticalFailed_);
}

// ========== VideoCallNotifyPiPState Tests ==========

/**
 * @tc.name: AVCodec_Server_VideoCallNotifyPiPState_001
 * @tc.desc: VideoCallNotifyPiPState returns AVCS_ERR_OK when no matching instance is found
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_VideoCallNotifyPiPState_001, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    int32_t ret = server->VideoCallNotifyPiPState("com.nonexist.app", false);
    EXPECT_EQ(ret, AVCS_ERR_OK);
    server = nullptr;
}

// ========== DecidePowerMode Tests ==========

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_001
 * @tc.desc: DecidePowerMode returns 0 when encPowermode is not 1 (low power disabled)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_001, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 192, .encHeight = 144, .encBitrate = 100000, .encPowermode = 0};
    EXPECT_EQ(server->DecidePowerMode(codecParam), 0);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_002
 * @tc.desc: DecidePowerMode returns 1 for 144P resolution with encPowermode=1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_002, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 192, .encHeight = 144, .encBitrate = 100000, .encPowermode = 1};
    // 192 * 144 = 27648 <= reso144PThreshold(129024)
    EXPECT_EQ(server->DecidePowerMode(codecParam), 1);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_003
 * @tc.desc: DecidePowerMode returns 1 for 480P resolution with bitrate <= 480K and encPowermode=1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_003, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 480000, .encPowermode = 1};
    // 640 * 480 = 307200 <= reso480PThreshold(665760), 480000 <= bitRate480KThreshold(480000)
    EXPECT_EQ(server->DecidePowerMode(codecParam), 1);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_004
 * @tc.desc: DecidePowerMode returns 0 for 480P resolution with bitrate > 480K and encPowermode=1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_004, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 600000, .encPowermode = 1};
    // 640 * 480 = 307200 <= reso480PThreshold(665760), 600000 > bitRate480KThreshold(480000)
    EXPECT_EQ(server->DecidePowerMode(codecParam), 0);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_005
 * @tc.desc: DecidePowerMode returns 1 for 720P resolution with bitrate <= 300K and encPowermode=1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_005, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 1280, .encHeight = 720, .encBitrate = 300000, .encPowermode = 1};
    // 1280 * 720 = 921600 <= reso720PThreshold(1497600), 300000 <= bitRate300KThreshold(300000)
    EXPECT_EQ(server->DecidePowerMode(codecParam), 1);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_006
 * @tc.desc: DecidePowerMode returns 0 for 720P resolution with bitrate > 300K and encPowermode=1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_006, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 1280, .encHeight = 720, .encBitrate = 500000, .encPowermode = 1};
    // 1280 * 720 = 921600 <= reso720PThreshold(1497600), 500000 > bitRate300KThreshold(300000)
    EXPECT_EQ(server->DecidePowerMode(codecParam), 0);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_007
 * @tc.desc: DecidePowerMode returns 0 for 1080P resolution (above 720P threshold) with encPowermode=1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_007, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 1920, .encHeight = 1080, .encBitrate = 200000, .encPowermode = 1};
    // 1920 * 1080 = 2073600 > reso720PThreshold(1497600)
    EXPECT_EQ(server->DecidePowerMode(codecParam), 0);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_008
 * @tc.desc: DecidePowerMode returns 0 when encPowermode=1 but encWidth<=0
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_008, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 0, .encHeight = 720, .encBitrate = 300000, .encPowermode = 1};
    // encWidth <= 0 triggers early return 0 even with encPowermode=1
    EXPECT_EQ(server->DecidePowerMode(codecParam), 0);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_DecidePowerMode_009
 * @tc.desc: DecidePowerMode returns 0 when encPowermode=1 but encHeight<=0
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_DecidePowerMode_009, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 1280, .encHeight = -1, .encBitrate = 300000, .encPowermode = 1};
    // encHeight <= 0 triggers early return 0 even with encPowermode=1
    EXPECT_EQ(server->DecidePowerMode(codecParam), 0);
    server = nullptr;
}

// ========== OnOutputParamInfo Tests ==========

/**
 * @tc.name: AVCodec_Server_OnOutputParamInfo_001
 * @tc.desc: OnOutputParamInfo uses forwardCaller.processName
 *           when forwardCaller.processName is not empty
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnOutputParamInfo_001, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 300000, .encPowermode = 1};
    // forwardCallerName is not empty, so it should be used
    server->OnOutputParamInfo(codecParam, "com.test.forward", "com.test.caller");
    // No crash, function processes the instance info
    EXPECT_NE(server, nullptr);
    server = nullptr;
}

/**
 * @tc.name: AVCodec_Server_OnOutputParamInfo_002
 * @tc.desc: OnOutputParamInfo uses caller.processName
 *           when forwardCaller.processName is empty
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnOutputParamInfo_002, TestSize.Level1)
{
    auto server = CreateAVCodecServer();
    ASSERT_NE(server, nullptr);
    CodecParamInfo codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 300000, .encPowermode = 1};
    // forwardCallerName is empty, so callerName should be used
    server->OnOutputParamInfo(codecParam, "", "com.test.caller");
    // No crash, function processes the instance info
    EXPECT_NE(server, nullptr);
    server = nullptr;
}

// ========== GetAndUpdateCodecParmInfoByProcessName Tests ==========

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_001
 * @tc.desc: GetAndUpdateCodecParmInfoByProcessName finds matching encoder instance and sets encPowermode=1
 *           when isFloatingWindow=true
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 3001;
    InstanceInfo encInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.call"},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, encInfo));

    // countEnc=1, countDec=0, isFloatingWindow=true → condition fails
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.call", true);
    EXPECT_FALSE(result.has_value());

    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_002
 * @tc.desc: GetAndUpdateCodecParmInfoByProcessName returns nullopt when processName does not match any instance
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_002, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.nonexist.call", true);
    EXPECT_FALSE(result.has_value());
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_003
 * @tc.desc: GetAndUpdateCodecParmInfoByProcessName returns nullopt when only encoder exists
 *           without decoder and isFloatingWindow=true
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_003, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 3003;
    InstanceInfo encInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.onlyenc"},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, encInfo));

    // countEnc=1, countDec=0, isFloatingWindow=true → condition fails
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.onlyenc", true);
    EXPECT_FALSE(result.has_value());

    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_004
 * @tc.desc: GetAndUpdateCodecParmInfoByProcessName returns instance
 *           when both encoder and decoder exist for the same processName
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_004, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encCallerId = 3004;
    const int32_t decCallerId = 3005;
    InstanceInfo encInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encCallerId, .processName = "com.test.encdec"},
    };
    InstanceInfo decInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = decCallerId, .processName = "com.test.encdec"},
    };
    manager.codecStubMap_.emplace(encCallerId, std::make_pair(nullptr, encInfo));
    manager.codecStubMap_.emplace(decCallerId, std::make_pair(nullptr, decInfo));

    // countEnc=1, countDec=1, isFloatingWindow=true → (countEnc==1 && countDec==1) passes
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.encdec", true);
    EXPECT_TRUE(result.has_value());

    manager.codecStubMap_.erase(encCallerId);
    manager.codecStubMap_.erase(decCallerId);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_005
 * @tc.desc: GetAndUpdateCodecParmInfoByProcessName returns instance
 *           when isFloatingWindow=false with encoder only (!isFloatingWindow branch)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_005, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 3006;
    InstanceInfo encInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.floating"},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, encInfo));

    // isFloatingWindow=false → !isFloatingWindow makes condition pass
    // even with only encoder (countEnc=1, countDec=0)
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName(
        "com.test.floating", false);
    EXPECT_TRUE(result.has_value());
    // isFloatingWindow=false → encPowermode set to 0
    EXPECT_EQ(result->encPowermode, 0);

    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_006
 * @tc.desc: GetAndUpdateCodecParmInfoByProcessName counts decoder instances (codecType==1)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_006, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t decCallerId = 3007;
    InstanceInfo decInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = decCallerId, .processName = "com.test.deconly"},
    };
    manager.codecStubMap_.emplace(decCallerId,
        std::make_pair(nullptr, decInfo));

    // Only decoder, no encoder → no instanceInfo
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName(
        "com.test.deconly", true);
    EXPECT_FALSE(result.has_value());

    manager.codecStubMap_.erase(decCallerId);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_007
 * @tc.desc: GetAndUpdateCodecParmInfoByProcessName returns instance when
 *           both encoder and decoder exist with isFloatingWindow=false
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_007, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encCallerId = 3008;
    const int32_t decCallerId = 3009;
    InstanceInfo encInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encCallerId, .processName = "com.test.encdec2"},
    };
    InstanceInfo decInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = decCallerId, .processName = "com.test.encdec2"},
    };
    manager.codecStubMap_.emplace(encCallerId,
        std::make_pair(nullptr, encInfo));
    manager.codecStubMap_.emplace(decCallerId,
        std::make_pair(nullptr, decInfo));

    // isFloatingWindow=false → !isFloatingWindow branch
    // countEnc=1, countDec=1, condition passes
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName(
        "com.test.encdec2", false);
    EXPECT_TRUE(result.has_value());
    // isFloatingWindow=false → encPowermode set to 0
    EXPECT_EQ(result->encPowermode, 0);

    manager.codecStubMap_.erase(encCallerId);
    manager.codecStubMap_.erase(decCallerId);
}

// ========== UpdateCodecParmInfoByInstanceId Tests ==========

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_001
 * @tc.desc: UpdateCodecParmInfoByInstanceId updates codecParam fields
 *           when encPowermode != -1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 4003;
    InstanceInfo initInfo = {
        .instanceId = 4003,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.update"},
        .codecParam = {.encWidth = 320, .encHeight = 240,
                       .encBitrate = 100000, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, initInfo));

    InstanceInfo updateInfo = {
        .instanceId = 4003,
        .caller = {.pid = callerId, .processName = "com.test.update"},
        .codecParam = {.encWidth = 640, .encHeight = 480,
                       .encBitrate = 300000, .encPowermode = 1},
    };
    // encPowermode=1 != -1 → encPowermode field is updated
    manager.UpdateCodecParmInfoByInstanceId(4003, updateInfo);

    auto it = manager.codecStubMap_.find(callerId);
    ASSERT_NE(it, manager.codecStubMap_.end());
    auto &stored = it->second.second;
    EXPECT_EQ(stored.codecParam.encWidth, 640);
    EXPECT_EQ(stored.codecParam.encHeight, 480);
    EXPECT_EQ(stored.codecParam.encBitrate, 300000);
    // encPowermode != -1 → updated to 1
    EXPECT_EQ(stored.codecParam.encPowermode, 1);

    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_002
 * @tc.desc: UpdateCodecParmInfoByInstanceId preserves encPowermode
 *           when encPowermode == -1 (skip update branch)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_002, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 4004;
    InstanceInfo initInfo = {
        .instanceId = 4004,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.preserve"},
        .codecParam = {.encWidth = 320, .encHeight = 240,
                       .encBitrate = 100000, .encPowermode = 1},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, initInfo));

    InstanceInfo updateInfo = {
        .instanceId = 4004,
        .caller = {.pid = callerId, .processName = "com.test.preserve"},
        .codecParam = {.encWidth = 1280, .encHeight = 720,
                       .encBitrate = 500000, .encPowermode = -1},
    };
    // encPowermode=-1 → encPowermode field NOT updated, stays at 1
    manager.UpdateCodecParmInfoByInstanceId(4004, updateInfo);

    auto it = manager.codecStubMap_.find(callerId);
    ASSERT_NE(it, manager.codecStubMap_.end());
    auto &stored = it->second.second;
    EXPECT_EQ(stored.codecParam.encWidth, 1280);
    EXPECT_EQ(stored.codecParam.encHeight, 720);
    EXPECT_EQ(stored.codecParam.encBitrate, 500000);
    // encPowermode=-1 → preserved old value 1
    EXPECT_EQ(stored.codecParam.encPowermode, 1);

    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_003
 * @tc.desc: UpdateCodecParmInfoByInstanceId triggers callback
 *           when isFloatingWindow = true (encPowermode==1)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_003, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    std::promise<void> promise;
    auto future = promise.get_future();
    manager.RegisterCallback(
        [&promise](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
               const std::string &callerName) { promise.set_value(); });

    const int32_t callerId = 4005;
    InstanceInfo initInfo = {
        .instanceId = 4005,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.cb2"},
        .codecParam = {.encWidth = 320, .encHeight = 240,
                       .encBitrate = 100000, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, initInfo));

    InstanceInfo updateInfo = {
        .instanceId = 4005,
        .caller = {.pid = callerId, .processName = "com.test.cb2"},
        .codecParam = {.encWidth = 640, .encHeight = 480,
                       .encBitrate = 200000, .encPowermode = 1},
    };
    // encPowermode=1 → isFloatingWindow=true → callback triggered
    manager.UpdateCodecParmInfoByInstanceId(4005, updateInfo);
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);

    manager.UnRegisterCallback();
    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_004
 * @tc.desc: UpdateCodecParmInfoByInstanceId does not trigger callback
 *           when countEnc > 1 and encPowermode != 0
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_004, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    std::promise<void> promise;
    auto future = promise.get_future();
    manager.RegisterCallback(
        [&promise](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
               const std::string &callerName) { promise.set_value(); });

    const int32_t callerId1 = 4006;
    const int32_t callerId2 = 4007;
    InstanceInfo enc1 = {
        .instanceId = 4006,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId1, .processName = "com.test.multi"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    InstanceInfo enc2 = {
        .instanceId = 4007,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId2, .processName = "com.test.multi"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(callerId1, std::make_pair(nullptr, enc1));
    manager.codecStubMap_.emplace(callerId2, std::make_pair(nullptr, enc2));

    InstanceInfo updateInfo = {
        .instanceId = 4006,
        .caller = {.pid = callerId1, .processName = "com.test.multi"},
        .codecParam = {.encWidth = 640, .encHeight = 480,
                       .encBitrate = 200000, .encPowermode = 1},
    };
    // countEnc=2 (>1) and encPowermode=1 (≠0) → no callback
    manager.UpdateCodecParmInfoByInstanceId(4006, updateInfo);
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(500)), std::future_status::timeout);

    manager.UnRegisterCallback();
    manager.codecStubMap_.erase(callerId1);
    manager.codecStubMap_.erase(callerId2);
}

// ========== RegisterCallback / UnRegisterCallback Tests ==========

/**
 * @tc.name: AVCodec_ServerManager_RegisterCallback_001
 * @tc.desc: RegisterCallback stores callback and UnRegisterCallback clears it
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_RegisterCallback_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    std::promise<void> promise;
    auto future = promise.get_future();

    manager.RegisterCallback([&promise](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
               const std::string &callerName) { promise.set_value(); });

    const int32_t callerId = 4001;
    InstanceInfo encInfo = {
        .instanceId = 4001,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.cb"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encBitrate = 200000, .encPowermode = 1},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, encInfo));
    manager.UpdateCodecParmInfoByInstanceId(4001, encInfo);

    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);

    manager.UnRegisterCallback();
    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_UnRegisterCallback_001
 * @tc.desc: UnRegisterCallback prevents callback from being invoked after unregister
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UnRegisterCallback_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    std::promise<void> promise;
    auto future = promise.get_future();

    manager.RegisterCallback([&promise](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
               const std::string &callerName) { promise.set_value(); });
    manager.UnRegisterCallback();

    const int32_t callerId = 4002;
    InstanceInfo encInfo = {
        .instanceId = 4002,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId},
        .codecParam = {.encPowermode = 1},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, encInfo));
    manager.UpdateCodecParmInfoByInstanceId(4002, encInfo);

    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(500)), std::future_status::timeout);

    manager.codecStubMap_.erase(callerId);
}

// ========== IPC Stub VideoCallNotifyPiPState Tests ==========

/**
 * @tc.name: AVCodec_Server_OnVideoCall_001
 * @tc.desc: AVCodecServiceStub delegates VideoCallNotifyPiPState to mock and returns AVCS_ERR_OK
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnVideoCall_001, TestSize.Level1)
{
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubCtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubDtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, VideoCallNotifyPiPState("com.test.bundle", true))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    AVCodecServiceStub stub;
    int32_t ret = stub.VideoCallNotifyPiPState("com.test.bundle", true);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: AVCodec_Server_OnVideoCall_002
 * @tc.desc: AVCodecServiceStub delegates VideoCallNotifyPiPState and returns error code when mock fails
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_Server_OnVideoCall_002, TestSize.Level1)
{
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubCtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, AVCodecServiceStubDtor()).Times(1);
    EXPECT_CALL(*avcodecStubMock_, VideoCallNotifyPiPState("com.test.bundle", false))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_INVALID_OPERATION));

    AVCodecServiceStub stub;
    int32_t ret = stub.VideoCallNotifyPiPState("com.test.bundle", false);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

// ========== CodecParamInfo Initialization Tests ==========

/**
 * @tc.name: AVCodec_ServerManager_CodecParamInfo_Init_001
 * @tc.desc: CodecParamInfo default construction initializes encPowermode to -1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_CodecParamInfo_Init_001, TestSize.Level1)
{
    CodecParamInfo param;
    EXPECT_EQ(param.encPowermode, -1);
    EXPECT_EQ(param.encHeight, 0);
    EXPECT_EQ(param.encWidth, 0);
    EXPECT_EQ(param.encBitrate, 0);
    param.encPowermode = 0;
    param.encHeight = 1080;
    param.encWidth = 720;
    param.encBitrate = 300000;
    EXPECT_EQ(param.encPowermode, 0);
    EXPECT_EQ(param.encHeight, 1080);
    EXPECT_EQ(param.encWidth, 720);
    EXPECT_EQ(param.encBitrate, 300000);
}

// ========== EventManager INSTANCE_UPDATE_INFO Dispatch Tests ==========

/**
 * @tc.name: AVCodec_ServerManager_OnInstanceUpdateInfoEvent_001
 * @tc.desc: OnInstanceUpdateInfoEvent updates codecParam when instance exists in AVCodecServerManager
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_OnInstanceUpdateInfoEvent_001, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 5001;
    InstanceInfo initInfo = {
        .instanceId = 5001,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.update"},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, initInfo));

    Media::Meta meta;
    meta.SetData(EventInfoExtentedKey::INSTANCE_ID.data(), 5001);
    meta.SetData(Media::Tag::VIDEO_PIC_WIDTH, 640);
    meta.SetData(Media::Tag::VIDEO_PIC_HEIGHT, 480);
    meta.SetData(Media::Tag::MEDIA_BITRATE, 300000);
    meta.SetData(Media::Tag::VIDEOCALL_LOWPOWER_MODE, 1);

    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_UPDATE_INFO, meta);

    // Verify codecParam was updated in codecStubMap_
    auto it = manager.codecStubMap_.find(callerId);
    ASSERT_NE(it, manager.codecStubMap_.end());
    auto &storedInfo = it->second.second;
    EXPECT_EQ(storedInfo.codecParam.encWidth, 640);
    EXPECT_EQ(storedInfo.codecParam.encHeight, 480);
    EXPECT_EQ(storedInfo.codecParam.encBitrate, 300000);
    EXPECT_EQ(storedInfo.codecParam.encPowermode, 1);

    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_OnInstanceUpdateInfoEvent_002
 * @tc.desc: OnInstanceUpdateInfoEvent early returns when instance does not exist in AVCodecServerManager
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_OnInstanceUpdateInfoEvent_002, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 5002;
    InstanceInfo existingInfo = {
        .instanceId = 5002,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.exist"},
        .codecParam = {.encWidth = 1280, .encHeight = 720, .encBitrate = 1000000, .encPowermode = 0},
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, existingInfo));

    Media::Meta meta;
    meta.SetData(EventInfoExtentedKey::INSTANCE_ID.data(), 9999); // Non-existent instanceId
    meta.SetData(Media::Tag::VIDEO_PIC_WIDTH, 640);
    meta.SetData(Media::Tag::VIDEOCALL_LOWPOWER_MODE, 1);

    // Should not crash or modify any existing data
    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_UPDATE_INFO, meta);

    // Verify existing entry was NOT modified
    auto it = manager.codecStubMap_.find(callerId);
    ASSERT_NE(it, manager.codecStubMap_.end());
    auto &storedInfo = it->second.second;
    EXPECT_EQ(storedInfo.codecParam.encWidth, 1280);
    EXPECT_EQ(storedInfo.codecParam.encHeight, 720);
    EXPECT_EQ(storedInfo.codecParam.encBitrate, 1000000);
    EXPECT_EQ(storedInfo.codecParam.encPowermode, 0);

    manager.codecStubMap_.erase(callerId);
}

/**
 * @tc.name: AVCodec_ServerManager_OnInstanceUpdateInfoEvent_003
 * @tc.desc: OnInstanceUpdateInfoEvent correctly updates only codecParam fields while preserving other InstanceInfo
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_OnInstanceUpdateInfoEvent_003, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t callerId = 5003;
    InstanceInfo initInfo = {
        .instanceId = 5003,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = callerId, .processName = "com.test.preserve"},
        .codecName = "test_encoder",
    };
    manager.codecStubMap_.emplace(callerId, std::make_pair(nullptr, initInfo));

    Media::Meta meta;
    meta.SetData(EventInfoExtentedKey::INSTANCE_ID.data(), 5003);
    meta.SetData(Media::Tag::VIDEO_PIC_WIDTH, 1920);
    meta.SetData(Media::Tag::VIDEO_PIC_HEIGHT, 1080);
    meta.SetData(Media::Tag::MEDIA_BITRATE, 500000);
    meta.SetData(Media::Tag::VIDEOCALL_LOWPOWER_MODE, 0);

    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_UPDATE_INFO, meta);

    auto it2 = manager.codecStubMap_.find(callerId);
    ASSERT_NE(it2, manager.codecStubMap_.end());
    auto &storedInfo = it2->second.second;
    EXPECT_EQ(storedInfo.codecParam.encWidth, 1920);
    EXPECT_EQ(storedInfo.codecParam.encHeight, 1080);
    EXPECT_EQ(storedInfo.codecParam.encBitrate, 500000);
    EXPECT_EQ(storedInfo.codecParam.encPowermode, 0);
    // Other fields should be preserved
    EXPECT_EQ(storedInfo.caller.pid, callerId);
    EXPECT_EQ(storedInfo.caller.processName, "com.test.preserve");
    EXPECT_EQ(storedInfo.codecName, "test_encoder");

    manager.codecStubMap_.erase(callerId);
}

// ========== GetAndUpdateCodecParmInfoByProcessName Additional Branch Coverage Tests ==========

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_008
 * @tc.desc: Only non-matching processName entries (no encoder/decoder match)
 *           covers line 456: instanceInfo.has_value() = false → short-circuit return nullopt
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_008, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t otherPid = 8014;
    InstanceInfo otherInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = otherPid, .processName = "com.test.unrelated"},
    };
    manager.codecStubMap_.emplace(otherPid, std::make_pair(nullptr, otherInfo));

    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.nonexist", true);
    EXPECT_FALSE(result.has_value());

    manager.codecStubMap_.erase(otherPid);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_009
 * @tc.desc: Matching encoder + non-matching processName entry in same loop
 *           covers line 445 A_false / line 449 C_false (processName mismatch)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_009, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encPid = 8002;
    const int32_t otherPid = 8003;
    InstanceInfo encInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid, .processName = "com.test.target"},
    };
    InstanceInfo otherInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = otherPid, .processName = "com.test.other"},
    };
    manager.codecStubMap_.emplace(encPid, std::make_pair(nullptr, encInfo));
    manager.codecStubMap_.emplace(otherPid, std::make_pair(nullptr, otherInfo));

    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.target", false);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->encPowermode, 0);

    manager.codecStubMap_.erase(encPid);
    manager.codecStubMap_.erase(otherPid);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_010
 * @tc.desc: Multiple encoders (countEnc > 1) + isFloatingWindow=false
 *           covers line 456: countEnc != 1, !isFloatingWindow = true → returns instanceInfo
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_010, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encPid1 = 8004;
    const int32_t encPid2 = 8005;
    InstanceInfo encInfo1 = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid1, .processName = "com.test.multienc"},
    };
    InstanceInfo encInfo2 = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid2, .processName = "com.test.multienc"},
    };
    manager.codecStubMap_.emplace(encPid1, std::make_pair(nullptr, encInfo1));
    manager.codecStubMap_.emplace(encPid2, std::make_pair(nullptr, encInfo2));

    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.multienc", false);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->encPowermode, 0);

    manager.codecStubMap_.erase(encPid1);
    manager.codecStubMap_.erase(encPid2);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_011
 * @tc.desc: Multiple encoders (countEnc > 1) + isFloatingWindow=true
 *           covers line 456: (countEnc==1 && countDec==1) false, !isFloatingWindow false → nullopt
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_011, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encPid1 = 8006;
    const int32_t encPid2 = 8007;
    InstanceInfo encInfo1 = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid1, .processName = "com.test.multienc2"},
    };
    InstanceInfo encInfo2 = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid2, .processName = "com.test.multienc2"},
    };
    manager.codecStubMap_.emplace(encPid1, std::make_pair(nullptr, encInfo1));
    manager.codecStubMap_.emplace(encPid2, std::make_pair(nullptr, encInfo2));

    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.multienc2", true);
    EXPECT_FALSE(result.has_value());

    manager.codecStubMap_.erase(encPid1);
    manager.codecStubMap_.erase(encPid2);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_012
 * @tc.desc: Multiple encoders + decoder + isFloatingWindow=true (countEnc > 1, countDec = 1)
 *           covers line 456: (countEnc==1 && countDec==1) false, !isFloatingWindow false → nullopt
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_012, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encPid1 = 8008;
    const int32_t encPid2 = 8009;
    const int32_t decPid = 8010;
    InstanceInfo encInfo1 = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid1, .processName = "com.test.multi3"},
    };
    InstanceInfo encInfo2 = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid2, .processName = "com.test.multi3"},
    };
    InstanceInfo decInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = decPid, .processName = "com.test.multi3"},
    };
    manager.codecStubMap_.emplace(encPid1, std::make_pair(nullptr, encInfo1));
    manager.codecStubMap_.emplace(encPid2, std::make_pair(nullptr, encInfo2));
    manager.codecStubMap_.emplace(decPid, std::make_pair(nullptr, decInfo));

    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.multi3", true);
    EXPECT_FALSE(result.has_value());

    manager.codecStubMap_.erase(encPid1);
    manager.codecStubMap_.erase(encPid2);
    manager.codecStubMap_.erase(decPid);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_013
 * @tc.desc: Encoder + decoder + audio codec with matching processName
 *           covers all sub-condition branches of line 445/449 in single loop
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_013, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encPid = 8011;
    const int32_t decPid = 8012;
    const int32_t audioPid = 8013;
    InstanceInfo encInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid, .processName = "com.test.mix"},
    };
    InstanceInfo decInfo = {
        .codecType = AVCODEC_TYPE_VIDEO_DECODER,
        .caller = {.pid = decPid, .processName = "com.test.mix"},
    };
    InstanceInfo audioInfo = {
        .codecType = AVCODEC_TYPE_AUDIO_ENCODER,
        .caller = {.pid = audioPid, .processName = "com.test.mix"},
    };
    manager.codecStubMap_.emplace(encPid, std::make_pair(nullptr, encInfo));
    manager.codecStubMap_.emplace(decPid, std::make_pair(nullptr, decInfo));
    manager.codecStubMap_.emplace(audioPid, std::make_pair(nullptr, audioInfo));

    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.mix", true);
    EXPECT_TRUE(result.has_value());
    EXPECT_EQ(result->encPowermode, 1);

    manager.codecStubMap_.erase(encPid);
    manager.codecStubMap_.erase(decPid);
    manager.codecStubMap_.erase(audioPid);
}

// ========== UpdateCodecParmInfoByInstanceId Additional Branch Coverage Tests ==========

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_005
 * @tc.desc: Matching instanceId + non-matching processName entry
 *           covers line 468 false branch, line 477 C_false (processName mismatch)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_005, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t targetPid = 8015;
    const int32_t otherPid = 8016;
    InstanceInfo targetInfo = {
        .instanceId = 8015,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = targetPid, .processName = "com.test.target"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encBitrate = 100000, .encPowermode = -1},
    };
    InstanceInfo otherInfo = {
        .instanceId = 8016,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = otherPid, .processName = "com.test.other"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(targetPid, std::make_pair(nullptr, targetInfo));
    manager.codecStubMap_.emplace(otherPid, std::make_pair(nullptr, otherInfo));

    InstanceInfo updateInfo = {
        .instanceId = 8015,
        .caller = {.pid = targetPid, .processName = "com.test.target"},
        .codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 300000, .encPowermode = 0},
    };
    manager.UpdateCodecParmInfoByInstanceId(8015, updateInfo);

    auto it = manager.codecStubMap_.find(targetPid);
    ASSERT_NE(it, manager.codecStubMap_.end());
    EXPECT_EQ(it->second.second.codecParam.encWidth, 640);
    EXPECT_EQ(it->second.second.codecParam.encPowermode, 0);

    manager.codecStubMap_.erase(targetPid);
    manager.codecStubMap_.erase(otherPid);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_006
 * @tc.desc: callback_ true, countEnc == 1, isFloatingWindow = false (encPowermode = 0)
 *           covers line 481: callback_ && (countEnc==1 || false) → triggers callback
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_006, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    std::promise<void> promise;
    auto future = promise.get_future();
    manager.RegisterCallback([&promise](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
               const std::string &callerName) { promise.set_value(); });

    const int32_t encPid = 8019;
    InstanceInfo initInfo = {
        .instanceId = 8019,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid, .processName = "com.test.singlenofloat"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encBitrate = 100000, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(encPid, std::make_pair(nullptr, initInfo));

    InstanceInfo updateInfo = {
        .instanceId = 8019,
        .caller = {.pid = encPid, .processName = "com.test.singlenofloat"},
        .codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 200000, .encPowermode = 0},
    };
    manager.UpdateCodecParmInfoByInstanceId(8019, updateInfo);
    EXPECT_EQ(future.wait_for(std::chrono::milliseconds(500)), std::future_status::ready);

    manager.UnRegisterCallback();
    manager.codecStubMap_.erase(encPid);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_007
 * @tc.desc: callback_ true, countEnc > 1, encPowermode = 0
 *           covers line 481: callback_ && (false || true) → callback triggered
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_007, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    bool callbackCalled = false;
    manager.RegisterCallback([&callbackCalled](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
               const std::string &callerName) { callbackCalled = true; });

    const int32_t encPid1 = 8020;
    const int32_t encPid2 = 8021;
    InstanceInfo enc1 = {
        .instanceId = 8020,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid1, .processName = "com.test.multinofloat"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    InstanceInfo enc2 = {
        .instanceId = 8021,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid2, .processName = "com.test.multinofloat"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(encPid1, std::make_pair(nullptr, enc1));
    manager.codecStubMap_.emplace(encPid2, std::make_pair(nullptr, enc2));

    InstanceInfo updateInfo = {
        .instanceId = 8020,
        .caller = {.pid = encPid1, .processName = "com.test.multinofloat"},
        .codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 200000, .encPowermode = 0},
    };
    // countEnc=2 (>1) and encPowermode=0 → (false || true) → callback triggered
    manager.UpdateCodecParmInfoByInstanceId(8020, updateInfo);
    EXPECT_TRUE(callbackCalled);

    manager.UnRegisterCallback();
    manager.codecStubMap_.erase(encPid1);
    manager.codecStubMap_.erase(encPid2);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_008
 * @tc.desc: callback_ true, countEnc > 1, isFloatingWindow = true (encPowermode = 1)
 *           covers line 481: callback_ && (false || true) → callback triggered
 *           This exercises the isFloatingWindow=true branch when countEnc != 1
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_008, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    bool callbackCalled = false;
    manager.RegisterCallback([&callbackCalled](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
               const std::string &callerName) { callbackCalled = true; });

    const int32_t encPid1 = 8022;
    const int32_t encPid2 = 8023;
    InstanceInfo enc1 = {
        .instanceId = 8022,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid1, .processName = "com.test.multifloat"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    InstanceInfo enc2 = {
        .instanceId = 8023,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid2, .processName = "com.test.multifloat"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(encPid1, std::make_pair(nullptr, enc1));
    manager.codecStubMap_.emplace(encPid2, std::make_pair(nullptr, enc2));

    InstanceInfo updateInfo = {
        .instanceId = 8022,
        .caller = {.pid = encPid1, .processName = "com.test.multifloat"},
        .codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 200000, .encPowermode = 1},
    };
    manager.UpdateCodecParmInfoByInstanceId(8022, updateInfo);
    EXPECT_FALSE(callbackCalled);

    manager.UnRegisterCallback();
    manager.codecStubMap_.erase(encPid1);
    manager.codecStubMap_.erase(encPid2);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_009
 * @tc.desc: No matching instanceId in map (all entries have different instanceId)
 *           covers line 468 false branch exclusively, line 477 with non-matching processName
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_009, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t existingPid = 8024;
    InstanceInfo existingInfo = {
        .instanceId = 8024,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = existingPid, .processName = "com.test.nomatchid"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encPowermode = -1},
    };
    manager.codecStubMap_.emplace(existingPid, std::make_pair(nullptr, existingInfo));
    InstanceInfo updateInfo = {
        .instanceId = 9999,
        .caller = {.pid = 9999, .processName = "com.test.different"},
        .codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 200000, .encPowermode = 1},
    };
    manager.UpdateCodecParmInfoByInstanceId(9999, updateInfo);
    auto it = manager.codecStubMap_.find(existingPid);
    ASSERT_NE(it, manager.codecStubMap_.end());
    EXPECT_EQ(it->second.second.codecParam.encWidth, 320);
    manager.codecStubMap_.erase(existingPid);
}

/**
 * @tc.name: AVCodec_ServerManager_GetInstanceInfoByProcessName_014
 * @tc.desc: Audio codec with matching processName (codecType != 0 and != 1)
 *           covers line 445 (processName match, codecType != 0 → A_true B_false)
 *           and line 449 (processName match, codecType != 1 → C_true D_false)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_GetInstanceInfoByProcessName_014, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t audioPid = 8001;
    InstanceInfo audioInfo = {
        .codecType = AVCODEC_TYPE_AUDIO_ENCODER,
        .caller = {.pid = audioPid, .processName = "com.test.audio"},
    };
    manager.codecStubMap_.emplace(audioPid, std::make_pair(nullptr, audioInfo));
    auto result = manager.GetAndUpdateCodecParmInfoByProcessName("com.test.audio", true);
    EXPECT_FALSE(result.has_value());
    manager.codecStubMap_.erase(audioPid);
}

/**
 * @tc.name: AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_010
 * @tc.desc: Audio codec entry with matching processName (codecType != 0)
 *           covers line 477: C_true D_false (processName match, codecType != 0)
 */
HWTEST_F(SaAVCodecUnitTest, AVCodec_ServerManager_UpdateInstanceInfoByInstanceId_010, TestSize.Level1)
{
    AVCodecServerManager &manager = AVCodecServerManager::GetInstance();
    const int32_t encPid = 8017;
    const int32_t audioPid = 8018;
    InstanceInfo encInfo = {
        .instanceId = 8017,
        .codecType = AVCODEC_TYPE_VIDEO_ENCODER,
        .caller = {.pid = encPid, .processName = "com.test.audiomix"},
        .codecParam = {.encWidth = 320, .encHeight = 240, .encBitrate = 100000, .encPowermode = -1},
    };
    InstanceInfo audioInfo = {
        .instanceId = 8018,
        .codecType = AVCODEC_TYPE_AUDIO_ENCODER,
        .caller = {.pid = audioPid, .processName = "com.test.audiomix"},
    };
    manager.codecStubMap_.emplace(encPid, std::make_pair(nullptr, encInfo));
    manager.codecStubMap_.emplace(audioPid, std::make_pair(nullptr, audioInfo));

    InstanceInfo updateInfo = {
        .instanceId = 8017,
        .caller = {.pid = encPid, .processName = "com.test.audiomix"},
        .codecParam = {.encWidth = 640, .encHeight = 480, .encBitrate = 300000, .encPowermode = 0},
    };
    manager.UpdateCodecParmInfoByInstanceId(8017, updateInfo);

    auto it = manager.codecStubMap_.find(encPid);
    ASSERT_NE(it, manager.codecStubMap_.end());
    EXPECT_EQ(it->second.second.codecParam.encWidth, 640);
    manager.codecStubMap_.erase(encPid);
    manager.codecStubMap_.erase(audioPid);
}
} // namespace