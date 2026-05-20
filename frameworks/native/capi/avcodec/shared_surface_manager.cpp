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

int32_t SharedSurfaceManager::Create()
{
    consumerSurface_ = Surface::CreateSurfaceAsConsumer("SharedEncoderSurface");
    CHECK_AND_RETURN_RET_LOG(consumerSurface_, AVCS_ERR_NO_MEMORY, "Failed to create consumer surface");

    producer_ = consumerSurface_->GetProducer();
    CHECK_AND_RETURN_RET_LOG(producer_, AVCS_ERR_UNKNOWN, "Failed to get producer");

    producerSurface_ = Surface::CreateSurfaceAsProducer(producer_);
    CHECK_AND_RETURN_RET_LOG(producerSurface_, AVCS_ERR_NO_MEMORY, "Failed to create producer surface");

    auto ret = producerSurface_->SetDefaultUsage(BUFFER_USAGE_CPU_READ);
    CHECK_AND_RETURN_RET_LOG(ret == GSERROR_OK, AVCS_ERR_UNKNOWN, "Set producer surface usage failed");

    AVCODEC_LOGI("SharedSurfaceManager created");
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
