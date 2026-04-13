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

#include "shared_surface_manager.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "SharedSurfaceManager"};
}

namespace OHOS {
namespace MediaAVCodec {

SharedSurfaceManager::SharedSurfaceManager()
{
    AVCODEC_LOGI("SharedSurfaceManager created");
}

SharedSurfaceManager::~SharedSurfaceManager()
{
    AVCODEC_LOGI("SharedSurfaceManager destroyed");
}

int32_t SharedSurfaceManager::Create(int32_t width, int32_t height, int32_t bufferCount)
{
    consumerSurface_ = Surface::CreateSurfaceAsConsumer("SharedEncoderSurface");
    if (consumerSurface_ == nullptr) {
        AVCODEC_LOGE("Failed to create consumer surface");
        return AVCS_ERR_NO_MEMORY;
    }

    GSError err = consumerSurface_->SetQueueSize(bufferCount);
    if (err != GSERROR_OK) {
        AVCODEC_LOGE("Failed to set queue size, err=%{public}d", err);
        return AVCS_ERR_UNKNOWN;
    }

    producer_ = consumerSurface_->GetProducer();
    if (producer_ == nullptr) {
        AVCODEC_LOGE("Failed to get producer");
        return AVCS_ERR_UNKNOWN;
    }

    producerSurface_ = Surface::CreateSurfaceAsProducer(producer_);
    if (producerSurface_ == nullptr) {
        AVCODEC_LOGE("Failed to create producer surface");
        return AVCS_ERR_NO_MEMORY;
    }

    AVCODEC_LOGI("SharedSurfaceManager created: width=%{public}d, height=%{public}d, bufferCount=%{public}d",
        width, height, bufferCount);
    return AVCS_ERR_OK;
}

sptr<Surface> SharedSurfaceManager::GetProducerSurface()
{
    return producerSurface_;
}

sptr<Surface> SharedSurfaceManager::GetConsumerSurface()
{
    return consumerSurface_;
}

} // namespace MediaAVCodec
} // namespace OHOS