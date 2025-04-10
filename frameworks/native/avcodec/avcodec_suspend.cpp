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

#include "avcodec_suspend.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "i_avcodec_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecSuspend"};
}

namespace OHOS {
namespace MediaAVCodec {
int32_t AVCodecSuspend::SuspendFreeze(const std::vector<pid_t> &pidList)
{
    CHECK_AND_RETURN_RET_LOG(!pidList.empty(), AVCS_ERR_INPUT_DATA_ERROR, "Freeze pidlist is empty");
    for (const auto &pid : pidList) {
        if (pid < 0 || static_cast<uint32_t>(pid) > std::numeric_limits<uint32_t>::max()) {
            AVCODEC_LOGE("Invalid pid: %d. Pid must be a non-negative 32-bit unsigned integer.", pid);
            return AVCS_ERR_INPUT_DATA_ERROR;
        }
    }
    return AVCodecServiceFactory::GetInstance().SuspendFreeze(pidList);
}

int32_t AVCodecSuspend::SuspendActive(const std::vector<pid_t> &pidList)
{
    CHECK_AND_RETURN_RET_LOG(!pidList.empty(), AVCS_ERR_INPUT_DATA_ERROR, "Active pidlist is empty");
    for (const auto &pid : pidList) {
        if (pid < 0 || static_cast<uint32_t>(pid) > std::numeric_limits<uint32_t>::max()) {
            AVCODEC_LOGE("Invalid pid: %d. Pid must be a non-negative 32-bit unsigned integer.", pid);
            return AVCS_ERR_INPUT_DATA_ERROR;
        }
    }
    return AVCodecServiceFactory::GetInstance().SuspendActive(pidList);
}

int32_t AVCodecSuspend::SuspendActiveAll()
{
    return AVCodecServiceFactory::GetInstance().SuspendActiveAll();
}
} // namespace MediaAVCodec
} // namespace OHOS

