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

#include <fcntl.h>
#include <gtest/gtest.h>
#include <iostream>
#include <memory>
#include <vector>
#include "avcodec_errors.h"
#include "avcodec_server.h"
#include "avcodec_server_manager.h"
#include "codec_service_stub.h"
#include "codeclist_service_stub.h"
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
    std::vector<std::u16string> args = {u"Codec"};
    int32_t fileFd = open(DUMP_FILE_PATH.c_str(), O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP);
    EXPECT_CALL(*codecStubMock_, Dump).Times(AtLeast(1)).WillRepeatedly(Return(OHOS::NO_ERROR));

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
} // namespace