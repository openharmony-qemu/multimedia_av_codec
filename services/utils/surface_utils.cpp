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

#include "surface_utils.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "SurfaceUtils"};
}

namespace OHOS {
namespace MediaAVCodec {
SurfaceUtils &SurfaceUtils::GetInstance()
{
    static SurfaceUtils instance;
    return instance;
}

bool SurfaceUtils::RegisterReleaseListener(std::string producerName, sptr<Surface> surface, OnReleaseFunc callback,
    OHSurfaceSource type)
{
    CHECK_AND_RETURN_RET_LOGW(!producerName.empty() && surface != nullptr, false, "Unexpected param");

    uint64_t id = surface->GetUniqueId();
    std::lock_guard<std::mutex> lock(mutex_);
    GSError err = surface->RegisterReleaseListener(callback);
    if (err != GSERROR_OK) {
        AVCODEC_LOGE("Register Listener failed, GSError=%{public}d, producerName=%{public}s, id=%{public}" PRIu64,
            err, producerName.c_str(), id);
        return false;
    }
    AVCODEC_LOGI("producerName=%{public}s register listener to surface id=%{public}" PRIu64,
        producerName.c_str(), id);
    surface->SetSurfaceSourceType(type);
    surfaceProducerMap_[id] = producerName;
    return true;
}

void SurfaceUtils::CleanCache(std::string producerName, sptr<Surface> surface, bool cleanAll)
{
    if (producerName.empty() || surface == nullptr) {
        return;
    }
    uint64_t id = surface->GetUniqueId();
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = surfaceProducerMap_.find(id);
    if (iter != surfaceProducerMap_.end() && iter->second == producerName) {
        surface->CleanCache(cleanAll);
        AVCODEC_LOGI("producerName=%{public}s CleanCache to surface id=%{public}" PRIu64,
            producerName.c_str(), id);
    }
}

void SurfaceUtils::ReleaseSurface(std::string producerName, sptr<Surface> surface, bool cleanAll)
{
    if (producerName.empty() || surface == nullptr) {
        return;
    }
    uint64_t id = surface->GetUniqueId();
    std::lock_guard<std::mutex> lock(mutex_);
    auto iter = surfaceProducerMap_.find(id);
    if (iter != surfaceProducerMap_.end() && iter->second == producerName) {
        surface->CleanCache(cleanAll);
        AVCODEC_LOGI("producerName=%{public}s CleanCache to surface id=%{public}" PRIu64,
            producerName.c_str(), id);
        surface->UnRegisterReleaseListener();
        AVCODEC_LOGI("producerName=%{public}s UnRegisterReleaseListener to surface id=%{public}" PRIu64,
            producerName.c_str(), id);
        surface->SetSurfaceSourceType(OHSurfaceSource::OH_SURFACE_SOURCE_DEFAULT);
        surfaceProducerMap_.erase(iter);
    }
}
} // namespace MediaAVCodec
} // namespace OHOS