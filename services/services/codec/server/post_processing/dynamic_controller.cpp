/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "dynamic_controller.h"
#include "avcodec_errors.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

DynamicController::~DynamicController()
{
    if (instance_ != nullptr) {
        AVCODEC_LOGW("Instance is not correctly destroyed.");
    }

    UnloadInterfacesImpl();
}

bool DynamicController::LoadInterfacesImpl()
{
    if (ready_) {
        AVCODEC_LOGD("Controller is ready.");
        return true;
    }
    if (interface_.Load()) {
        ready_ = true;
        return true;
    }
    return false;
}

void DynamicController::UnloadInterfacesImpl()
{
    interface_.Unload();
    ready_ = false;
}

bool DynamicController::IsColorSpaceConversionSupportedImpl(const CapabilityInfo& input, const CapabilityInfo& output)
{
    auto inputPtr = static_cast<const void*>(&input);
    auto outputPtr = static_cast<const void*>(&output);
    return interface_.Invoke<DynamicInterfaceName::IS_COLORSPACE_CONVERSION_SUPPORTED>(inputPtr, outputPtr);
}

int32_t DynamicController::CreateImpl()
{
    instance_ = static_cast<DynamicColorSpaceConverterHandle*>(interface_.Invoke<DynamicInterfaceName::CREATE>());
    CHECK_AND_RETURN_RET_LOG(instance_ != nullptr, AVCS_ERR_UNKNOWN, "Create VPE instance failed.");
    return AVCS_ERR_OK;
}

void DynamicController::DestroyImpl()
{
    interface_.Invoke<DynamicInterfaceName::DESTROY>(instance_);
    instance_ = nullptr;
}

int32_t DynamicController::SetCallbackImpl(void* callback, void* userData)
{
    auto ret = interface_.Invoke<DynamicInterfaceName::SET_CALLBACK>(instance_, callback, userData);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
        "Set callback for video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::SetOutputSurfaceImpl(sptr<Surface> surface)
{
    void* sf = static_cast<void*>(&surface);
    auto ret = interface_.Invoke<DynamicInterfaceName::SET_OUTPUT_SURFACE>(instance_, sf);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
        "Set output surface for video processing failed.");
    surface->UnRegisterReleaseListener();
    surface->RegisterReleaseListener([this](sptr<SurfaceBuffer> &buffer) -> GSError {
        return OnProducerBufferReleased(buffer);
    });
    return AVCS_ERR_OK;
}

int32_t DynamicController::CreateInputSurfaceImpl(sptr<Surface>& surface)
{
    void* sf = static_cast<void*>(&surface);
    auto ret = interface_.Invoke<DynamicInterfaceName::CREATE_INPUT_SURFACE>(instance_, sf);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK && surface != nullptr, AVCS_ERR_UNKNOWN,
        "Create input surface for video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::SetParameterImpl(Media::Format& parameter)
{
    void* parameterPtr = static_cast<void*>(&parameter);
    auto ret = interface_.Invoke<DynamicInterfaceName::CREATE_INPUT_SURFACE>(instance_, parameterPtr);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Set parameter for VPE failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::GetParameterImpl(Media::Format& parameter)
{
    void* parameterPtr = static_cast<void*>(&parameter);
    auto ret = interface_.Invoke<DynamicInterfaceName::CREATE_INPUT_SURFACE>(instance_, parameterPtr);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get parameter for VPE failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::ConfigureImpl(Media::Format& config)
{
    void* configPtr = static_cast<void*>(&config);
    auto ret = interface_.Invoke<DynamicInterfaceName::CONFIGURE>(instance_, configPtr);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Configure video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::PrepareImpl()
{
    auto ret = interface_.Invoke<DynamicInterfaceName::PREPARE>(instance_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Prepare video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::StartImpl()
{
    auto ret = interface_.Invoke<DynamicInterfaceName::START>(instance_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Start video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::StopImpl()
{
    auto ret = interface_.Invoke<DynamicInterfaceName::STOP>(instance_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Stop video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::FlushImpl()
{
    auto ret = interface_.Invoke<DynamicInterfaceName::FLUSH>(instance_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Flush video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::GetOutputFormatImpl(Media::Format &format)
{
    void *formatPtr = static_cast<void *>(&format);
    auto ret = interface_.Invoke<DynamicInterfaceName::GET_OUTPUT_FORMAT>(instance_, formatPtr);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                             "GetOutputFormat video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::ResetImpl()
{
    auto ret = interface_.Invoke<DynamicInterfaceName::RESET>(instance_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Reset video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::ReleaseImpl()
{
    auto ret = interface_.Invoke<DynamicInterfaceName::RELEASE>(instance_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Release video processing failed.");
    return AVCS_ERR_OK;
}

int32_t DynamicController::ReleaseOutputBufferImpl(uint32_t index, bool render)
{
    auto ret = interface_.Invoke<DynamicInterfaceName::RELEASE_OUPUT_BUFFER>(instance_, index, render);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
        "Release output buffer of video processing failed.");
    return AVCS_ERR_OK;
}

GSError DynamicController::OnProducerBufferReleased([[maybe_unused]] sptr<SurfaceBuffer> &buffer)
{
    auto ret = interface_.Invoke<DynamicInterfaceName::ON_PRODUCER_BUFFER_RELEASED>(instance_);
    return static_cast<GSError>(ret);
}

} // namespace PostProcessing
} // namespace MediaAVCodec
} // namespace OHOS