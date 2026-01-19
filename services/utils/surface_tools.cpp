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

#include "surface_tools.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "SurfaceTools"};
}

namespace OHOS {
namespace MediaAVCodec {
SurfaceTools &SurfaceTools::GetInstance()
{
    static SurfaceTools instance;
    return instance;
}

bool SurfaceTools::RegisterReleaseListener(int32_t instanceId, sptr<Surface> surface, OnReleaseFunc callback,
    OHSurfaceSource type)
{
    CHECK_AND_RETURN_RET_LOGW(surface != nullptr, false, "Unexpected param");

    uint64_t id = surface->GetUniqueId();
    std::lock_guard<std::mutex> lock(mutex_);
    GSError err = surface->RegisterReleaseListener(callback);
    if (err != GSERROR_OK) {
        AVCODEC_LOGE("Register Listener failed, GSError=%{public}d, instanceId=%{public}d, id=%{public}" PRIu64,
            err, instanceId, id);
        return false;
    }
    AVCODEC_LOGI("instanceId=%{public}d surface id=%{public}" PRIu64,
        instanceId, id);
    surface->SetSurfaceSourceType(type);
    surfaceProducerMap_[id] = instanceId;
    return true;
}

bool SurfaceTools::RegisterReleaseListener(int32_t instanceId, sptr<Surface> surface,
    OnReleaseFuncWithSequenceAndFence callback, OHSurfaceSource type)
{
    CHECK_AND_RETURN_RET_LOGW(surface != nullptr, false, "Unexpected param");
 
    uint64_t id = surface->GetUniqueId();
    std::lock_guard<std::mutex> lock(mutex_);
    GSError err = surface->RegisterReleaseListener(callback);
    if (err != GSERROR_OK) {
        AVCODEC_LOGE("Register Listener failed, GSError=%{public}d, instanceId=%{public}d, id=%{public}" PRIu64,
            err, instanceId, id);
        return false;
    }
    AVCODEC_LOGI("instanceId=%{public}d register listener to surface id=%{public}" PRIu64,
        instanceId, id);
    surface->SetSurfaceSourceType(type);
    surfaceProducerMap_[id] = instanceId;
    return true;
}

void SurfaceTools::CleanCache(int32_t instanceId, sptr<Surface> surface, bool cleanAll)
{
    if (surface == nullptr) {
        return;
    }
    uint64_t id = surface->GetUniqueId();
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = surfaceProducerMap_.find(id);
    if (iter != surfaceProducerMap_.end() && iter->second == instanceId) {
        surface->CleanCache(cleanAll);
        AVCODEC_LOGI("instanceId=%{public}d surface id=%{public}" PRIu64,
            instanceId, id);
    }
}

void SurfaceTools::ReleaseSurface(int32_t instanceId, sptr<Surface> surface, bool cleanAll, bool abadon)
{
    if (surface == nullptr) {
        return;
    }
    uint64_t id = surface->GetUniqueId();
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = surfaceProducerMap_.find(id);
    if (iter != surfaceProducerMap_.end() && iter->second == instanceId) {
        surface->CleanCache(cleanAll);
        AVCODEC_LOGI("instanceId=%{public}d CleanCache to surface id=%{public}" PRIu64,
            instanceId, id);
        surface->UnRegisterReleaseListener();
        AVCODEC_LOGI("instanceId=%{public}d UnRegisterReleaseListener to surface id=%{public}" PRIu64,
            instanceId, id);
        surface->SetSurfaceSourceType(OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT);
        if (abadon) {
            surfaceProducerMap_.erase(iter);
        }
    }
}
} // namespace MediaAVCodec
} // namespace OHOS