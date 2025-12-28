/*
* Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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
 
#include <iremote_broker.h>
#include "common/log.h"
#include "iservice_registry.h"
#include "bundle_mgr_interface.h"
#include "bundle_info.h"
#include "http_media_utils.h"
 
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };
}
using OHOS::AppExecFwk::GetBundleInfoFlag;
 
namespace OHOS {
namespace Media {
namespace {
    const int32_t DEFAULT_ID = 1003;
}
std::string HttpMediaUtils::GetClientBundleName(int32_t uid)
{
    if (uid == DEFAULT_ID) { // 1003 is bootanimation uid
        return "bootanimation";
    }
 
    std::string bundleName = "";
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (samgr == nullptr) {
        MEDIA_LOG_E("Get ability manager failed, uid is %{public}d", uid);
        return bundleName;
    }
 
    sptr<IRemoteObject> object = samgr->GetSystemAbility(BUNDLE_MGR_SERVICE_SYS_ABILITY);
    if (object == nullptr) {
        MEDIA_LOG_E("object is NULL. uid is %{public}d", uid);
        return bundleName;
    }
 
    sptr<OHOS::AppExecFwk::IBundleMgr> bms = iface_cast<OHOS::AppExecFwk::IBundleMgr>(object);
    if (bms == nullptr) {
        MEDIA_LOG_E("bundle manager service is NULL. uid is %{public}d", uid);
        return bundleName;
    }
 
    auto result = bms->GetNameForUid(uid, bundleName);
    if (result != 0) {
        MEDIA_LOG_E("Error GetBundleNameForUid fail, uid is %{public}d", uid);
        return "";
    }
 
    MEDIA_LOG_I("bundle name is %{public}s, uid is %{public}d", bundleName.c_str(), uid);
    return bundleName;
}
 
}  // namespace Media
}  // namespace OHOS