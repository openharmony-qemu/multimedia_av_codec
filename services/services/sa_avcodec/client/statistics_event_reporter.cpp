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

#include "statistics_event_reporter.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "ipc_skeleton.h"
#include "iservice_registry.h"
#include "i_standard_avcodec_service.h"
#include "system_ability_definition.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "StatisticsEventReporter"};
}

namespace OHOS {
namespace MediaAVCodec {
void StatisticsEventReporter::Report(StatisticsEventType type, Media::Meta &meta)
{
    sptr<ISystemAbilityManager> samgr = nullptr;
    samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    CHECK_AND_RETURN_LOG(samgr != nullptr, "system ability manager is nullptr");

    sptr<IRemoteObject> object = samgr->CheckSystemAbility(OHOS::AV_CODEC_SERVICE_ID);
    CHECK_AND_RETURN_LOG(object != nullptr, "AVCodec service object is nullptr");

    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    bool parcelRet = data.WriteInterfaceToken(u"IStandardAVCodecServiceInterface");
    parcelRet = parcelRet && data.WriteUint32(static_cast<uint32_t>(type));
    parcelRet = parcelRet && meta.ToParcel(data);
    CHECK_AND_RETURN_LOG(parcelRet, "Write statistics event parcel failed");

    int error = object->SendRequest(static_cast<uint32_t>(
        AVCodecServiceInterfaceCode::REPORT_STATISTICS_EVENT), data, reply, option);
    CHECK_AND_RETURN_LOG(error == 0, "Report statistics event failed, error: %{public}d", error);
}

void StatisticsEventReporter::Report(StatisticsEventType type, const Media::Meta &meta)
{
    Media::Meta metaCopy = meta;
    Report(type, metaCopy);
}
} // namespace MediaAVCodec
} // namespace OHOS