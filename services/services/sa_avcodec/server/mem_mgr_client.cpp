/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "mem_mgr_client.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "system_ability_definition.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "MemMgrClient"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS::Memory;
static MemMgrClient g_MemMgrClientInstance;
MemMgrClient &MemMgrClient::GetInstance()
{
    return g_MemMgrClientInstance;
}

MemMgrClient::MemMgrClient() noexcept
{
    pid_ = getpid();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MemMgrClient::~MemMgrClient()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool MemMgrClient::IsAlived()
{
    sptr<ISystemAbilityManager> samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_RET_LOG(samgr != nullptr, false, "system ability manager is nullptr.");

    sptr<IRemoteObject> object = samgr->GetSystemAbility(OHOS::MEMORY_MANAGER_SA_ID);
    CHECK_AND_RETURN_RET_LOG(object != nullptr, false, "memmgr object is nullptr.");

    memMgrProxy_ = iface_cast<Memory::MemMgrProxy>(object);
    return memMgrProxy_ != nullptr;
}

int32_t MemMgrClient::NotifyProcessStatus(int32_t status)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(IsAlived(), AVCS_ERR_INVALID_OPERATION, "memmgr proxy is not alived. status:%{public}d",
                             status);

    int32_t ret = memMgrProxy_->NotifyProcessStatus(pid_, 1, status, AV_CODEC_SERVICE_ID);
    memMgrProxy_ = nullptr;
    CHECK_AND_RETURN_RET_LOG(ret == 0, AVCS_ERR_UNKNOWN, "notify process status failed. status:%{public}d", status);

    AVCODEC_LOGI("notify memory manager to %{public}d success.", status);
    return AVCS_ERR_OK;
}

int32_t MemMgrClient::SetCritical(bool isKeyService)
{
    if (isKeyService_ == isKeyService) {
        AVCODEC_LOGD("critical is already is %{public}d.", isKeyService);
        return AVCS_ERR_INVALID_OPERATION;
    }
    CHECK_AND_RETURN_RET_LOG(IsAlived(), AVCS_ERR_INVALID_OPERATION,
                             "memmgr proxy is not alived. isKeyService:%{public}d", isKeyService);

    int32_t ret = memMgrProxy_->SetCritical(pid_, isKeyService, AV_CODEC_SERVICE_ID);
    memMgrProxy_ = nullptr;
    CHECK_AND_RETURN_RET_LOG(ret == 0, AVCS_ERR_UNKNOWN, "set critical failed. isKeyService:%{public}d", isKeyService);

    isKeyService_ = isKeyService;
    AVCODEC_LOGI("set critical to %{public}d success.", isKeyService);
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS
