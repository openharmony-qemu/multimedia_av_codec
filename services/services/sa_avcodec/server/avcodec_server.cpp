/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "avcodec_server.h"
#include <sys/time.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_sysevent.h"
#include "avcodec_trace.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"
#include "background_event_handler.h"
#include "event_manager.h"
#include "codec_ability_singleton.h"
#include "avcodec_xcollie.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecServer"};
constexpr uint32_t SERVER_MAX_IPC_THREAD_NUM = 64;
constexpr int32_t RESO_144P_THRESHOLD = 129024;
constexpr int32_t RESO_480P_THRESHOLD = 665760;
constexpr int32_t RESO_720P_THRESHOLD = 1497600;
constexpr int32_t BITRATE_300K_THRESHOLD = 300000;
constexpr int32_t BITRATE_480K_THRESHOLD = 480000;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
REGISTER_SYSTEM_ABILITY_BY_ID(AVCodecServer, AV_CODEC_SERVICE_ID, true)
AVCodecServer::AVCodecServer(int32_t systemAbilityId, bool runOnCreate) : SystemAbility(systemAbilityId, runOnCreate)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecServer::~AVCodecServer()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void AVCodecServer::OnDump()
{
    AVCODEC_LOGD("AVCodecServer OnDump");
}

void AVCodecServer::OnStart()
{
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    AVCODEC_LOGI("In");
    Publish(this);
    IPCSkeleton::SetMaxWorkThreadNum(SERVER_MAX_IPC_THREAD_NUM);
    AddSystemAbilityListener(MEMORY_MANAGER_SA_ID);
    EventManager::GetInstance().OnInstanceEvent(EventType::STATISTICS_EVENT_REGISTER_SUBMIT);
}

int32_t AVCodecServer::OnIdle([[maybe_unused]] const SystemAbilityOnDemandReason &idleReason)
{
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    AVCODEC_LOGI("In");
    auto instanceCount = AVCodecServerManager::GetInstance().GetInstanceCount();
    CHECK_AND_RETURN_RET_LOG(instanceCount == 0,
        -1, // -1: refuse to switch to idle state
        "The transition to the idle state is rejected, because %{public}u instances is not released", instanceCount);
    return 0;
}

void AVCodecServer::OnStop()
{
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    AVCODEC_LOGI("In");
    AVCodecServerManager::GetInstance().NotifyProcessStatus(0);
}

void AVCodecServer::OnAddSystemAbility(int32_t systemAbilityId, const std::string &deviceId)
{
    AVCODEC_LOGI("systemAbilityId:%{public}d, deviceId:%s", systemAbilityId, deviceId.c_str());
    if (systemAbilityId == MEMORY_MANAGER_SA_ID) {
        AVCodecServerManager::GetInstance().NotifyProcessStatus(1);
    }
}

std::optional<AVCodecServerManager::StubType> AVCodecServer::SwitchSystemId(
    IStandardAVCodecService::AVCodecSystemAbility subSystemId)
{
    switch (subSystemId) {
#ifdef SUPPORT_CODECLIST
        case AVCodecSystemAbility::AVCODEC_CODECLIST: {
            return AVCodecServerManager::CODECLIST;
        }
#endif
#ifdef SUPPORT_CODEC
        case AVCodecSystemAbility::AVCODEC_CODEC: {
            return AVCodecServerManager::CODEC;
        }
#endif
        default: {
            AVCODEC_LOGE("subSystemId is invalid");
            return std::nullopt;
        }
    }
}

int32_t AVCodecServer::GetSubSystemAbility(IStandardAVCodecService::AVCodecSystemAbility subSystemId,
                                           sptr<IRemoteObject> &stubObject)
{
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    bool saInActiveState = (GetAbilityState() != SystemAbilityState::IDLE) || CancelIdle();
    CHECK_AND_RETURN_RET_LOG(saInActiveState,
        AVCS_ERR_INVALID_STATE, "AVCodec service in idle state, but cancel idle failed");

    std::optional<AVCodecServerManager::StubType> stubType = SwitchSystemId(subSystemId);
    CHECK_AND_RETURN_RET_LOG(stubType != std::nullopt, AVCS_ERR_INVALID_OPERATION, "Get sub system type failed");

    int32_t ret = AVCodecServerManager::GetInstance().CreateStubObject(stubType.value(), stubObject);
    CHECK_AND_RETURN_RET_LOG(stubObject != nullptr, ret, "Create sub system failed, err: %{public}d", ret);

    return AVCS_ERR_OK;
}

int32_t AVCodecServer::SuspendFreeze(const std::vector<pid_t> &pidList)
{
    BackGroundEventHandler::GetInstance().NotifyFreeze(pidList);
    return AVCS_ERR_OK;
}

int32_t AVCodecServer::SuspendActive(const std::vector<pid_t> &pidList)
{
    BackGroundEventHandler::GetInstance().NotifyActive(pidList);
    return AVCS_ERR_OK;
}

int32_t AVCodecServer::SuspendActiveAll()
{
    BackGroundEventHandler::GetInstance().NotifyActiveAll();
    return AVCS_ERR_OK;
}

int32_t AVCodecServer::GetActiveSecureDecoderPids(std::vector<pid_t> &pidList)
{
    pidList = AVCodecServerManager::GetInstance().GetActiveSecureDecoderPids();
    return AVCS_ERR_OK;
}

void AVCodecServer::ReportStatisticsEvent(StatisticsEventType type, Media::Meta &meta)
{
    EventManager::GetInstance().OnInstanceEvent(type, meta);
}

int32_t AVCodecServer::Dump(int32_t fd, const std::vector<std::u16string> &args)
{
    if (fd <= 0) {
        AVCODEC_LOGW("Failed to check fd");
        return OHOS::INVALID_OPERATION;
    }
    if (args.size() == 0) {
        AVCodecServerManager::GetInstance().Dump(fd, args);
        return OHOS::NO_ERROR;
    }

    auto firstArg = args[0];
    if (firstArg == u"codec") {
        AVCodecServerManager::GetInstance().Dump(fd, args);
    } else if (firstArg == u"query_capability") {
        CodecAbilitySingleton::GetInstance().Dump(fd, args);
    } else if (firstArg == u"collie") {
        AVCodecXCollie::GetInstance().Dump(fd);
    } else if (firstArg == u"report_statistics_event") {
        EventManager::GetInstance().OnInstanceEvent(EventType::STATISTICS_EVENT_SUBMIT);
    }
    return OHOS::NO_ERROR;
}

int32_t AVCodecServer::VideoCallNotifyPiPState(const std::string &bundleName, const bool isFloatingWindow)
{
    auto codecParam = AVCodecServerManager::GetInstance().GetAndUpdateCodecParmInfoByProcessName(
        bundleName, isFloatingWindow);
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    if (camRegisterFlag_) {
        int32_t codecDecision = 0;
        if (!codecParam.has_value())
            codecDecision = 0;
        else
            codecDecision = DecidePowerMode(*codecParam);
        AVCODEC_LOGD("AVCodecServer VideoCallNotifyPiPState: bundleName is %{public}s, isFloatingWindow is %{public}d, "
                     "codecDecision is %{public}d", bundleName.c_str(), isFloatingWindow, codecDecision);
        return NotifyListenerByProcessName(bundleName, codecDecision);
    }
    return AVCS_ERR_OK;
}

int32_t AVCodecServer::DecidePowerMode(const CodecParamInfo &codecParam)
{
    int32_t codecDecision = 0;
    if (codecParam.encPowermode != 1)
        codecDecision = 0;
    else if (codecParam.encWidth <= 0 || codecParam.encHeight <= 0)
        codecDecision = 0;
    else {
        int64_t resolution = static_cast<int64_t>(codecParam.encHeight) *
                             static_cast<int64_t>(codecParam.encWidth);
        if (resolution <= RESO_144P_THRESHOLD)
            codecDecision = 1;
        else if (resolution <= RESO_480P_THRESHOLD)
            codecDecision = codecParam.encBitrate <= BITRATE_480K_THRESHOLD ? 1 : 0;
        else if (resolution <= RESO_720P_THRESHOLD)
            codecDecision = codecParam.encBitrate <= BITRATE_300K_THRESHOLD ? 1 : 0;
    }
    return codecDecision;
}

void AVCodecServer::OnOutputParamInfo(const CodecParamInfo &codecParam, const std::string &forwardCallerName,
                                      const std::string &callerName)
{
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    if (camRegisterFlag_) {
    int32_t codecDecision = 0;
    codecDecision = DecidePowerMode(codecParam);
    std::string bundleName = forwardCallerName.empty() ? callerName : forwardCallerName;
    NotifyListenerByProcessName(bundleName, codecDecision);
    }
}

int32_t AVCodecServer::NotifyCamRegister(const bool camRegisterFlag)
{
    std::lock_guard<std::mutex> stateLock(stateMutex_);
    camRegisterFlag_ = camRegisterFlag;
    AVCODEC_LOGI("Server:camera notify update camregister status: %{public}s",
                 camRegisterFlag ? "registered" : "unregistered");
    if (camRegisterFlag) {
        wptr<AVCodecServer> weakThis = this;
        AVCodecServerManager::GetInstance().RegisterCallback(
            [weakThis](const CodecParamInfo &codecParam, const std::string &forwardCallerName,
                       const std::string &callerName) {
                sptr<AVCodecServer> strongThis = weakThis.promote();
                if (strongThis != nullptr) {
                    strongThis->OnOutputParamInfo(codecParam, forwardCallerName, callerName);
                }
            });
    } else {
        AVCodecServerManager::GetInstance().UnRegisterCallback();
    }
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS
