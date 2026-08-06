/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "avcodec_videocall.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "i_avcodec_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecVideoCall"};
}

namespace OHOS {
namespace MediaAVCodec {
int32_t AVCodecVideoCall::VideoCallNotifyPiPState(const std::string &bundleName, const bool isFloatingWindow)
{
    CHECK_AND_RETURN_RET_LOG(!bundleName.empty(), AVCS_ERR_INPUT_DATA_ERROR, "bundleName is empty");
    AVCODEC_LOGI("VideoCallNotifyPiPState: bundleName=%{public}s, isFloatingWindow=%{public}s",
                 bundleName.c_str(), isFloatingWindow ? "true" : "false");
    return AVCodecServiceFactory::GetInstance().VideoCallNotifyPiPState(bundleName, isFloatingWindow);
}

int32_t AVCodecVideoCall::RegisterCallback(const std::shared_ptr<MediaCodecLowPowerConsumptionCallback> &callback)
{
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_INPUT_DATA_ERROR, "callback object is nullptr");
    return AVCodecServiceFactory::GetInstance().RegisterCallback(callback);
}

void AVCodecVideoCall::UnRegisterCallback()
{
    AVCodecServiceFactory::GetInstance().UnRegisterCallback();
}
} // namespace MediaAVCodec
} // namespace OHOS

