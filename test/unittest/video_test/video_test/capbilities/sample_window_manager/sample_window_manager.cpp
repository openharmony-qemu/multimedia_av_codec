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

#include "sample_window_manager.h"
#include <atomic>
#include <string>
#include "av_codec_sample_log.h"
#include "sample_utils.h"

#include "native_image_window_wrapper.h"
#ifdef SAMPLE_USE_ROSEN_WINDOW
#include "rosen_window_wrapper.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "WindowManager"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
const std::unordered_map<int32_t, std::string> SAMPLE_WINDOW_TYPE_TO_STRING = {
    {static_cast<int32_t>(SampleWindowType::NATIVE_IMAGE), "NATIVE_IMAGE"},
    {static_cast<int32_t>(SampleWindowType::ENCODER),      "ENCODER"},
#ifdef SAMPLE_BUILD_TO_EXECUTOR
    {static_cast<int32_t>(SampleWindowType::ROSEN),        "ROSEN"},
#endif
#ifdef SAMPLE_BUILD_TO_HAP
    {static_cast<int32_t>(SampleWindowType::XCOMPONENT),   "XCOMPONENT"},
#endif
};

WindowManager &OHOS::MediaAVCodec::Sample::WindowManager::GetInstance()
{
    static WindowManager manager;
    return manager;
}

std::shared_ptr<WindowWrapper> WindowManager::CreateWindowWrapper(SampleWindowType windowType)
{
    return CreateWindowWrapper(windowType, nullptr);
}

std::shared_ptr<WindowWrapper> WindowManager::CreateWindowWrapper(SampleWindowType windowType,
                                                                  std::shared_ptr<OHNativeWindow> window)
{
    std::shared_ptr<WindowWrapper> windowWrappper = nullptr;
    switch (windowType) {
        case SampleWindowType::NATIVE_IMAGE:
            windowWrappper = std::static_pointer_cast<WindowWrapper>(std::make_shared<NativeImageWindowWrapper>());
            break;
        case SampleWindowType::ENCODER:
            windowWrappper = std::make_shared<WindowWrapper>(windowType, window);
            break;
#ifdef SAMPLE_BUILD_TO_EXECUTOR
#ifdef SAMPLE_USE_ROSEN_WINDOW
        case SampleWindowType::ROSEN:
            windowWrappper = std::static_pointer_cast<WindowWrapper>(std::make_shared<RosenWindowWrapper>());
            break;
#endif
#endif
#ifdef SAMPLE_BUILD_TO_HAP
        case SampleWindowType::XCOMPONENT:
            break;
#endif
        default:
            AVCODEC_LOGE("Unsupported window type: %{public}d", windowType);
            break;
    }
    CHECK_AND_RETURN_RET_LOG((windowWrappper != nullptr) && windowWrappper->SelfCheck(), nullptr,
        "Window wrapper create failed, window type: %{public}s",
        ToString(static_cast<int32_t>(windowType), SAMPLE_WINDOW_TYPE_TO_STRING).c_str());

    static std::atomic<int32_t> windowIdCount{0};
    WindowId windowId = windowIdCount++;
    windowWrappper->SetWindowId(windowId);
    AVCODEC_LOGI("Succeed, window type: %{public}s, window id: %{public}d",
        ToString(static_cast<int32_t>(windowType), SAMPLE_WINDOW_TYPE_TO_STRING).c_str(), windowId);
    return windowWrappper;
}

std::shared_ptr<WindowWrapper> WindowManager::GetWindowWrapper(WindowId id)
{
    CHECK_AND_RETURN_RET_LOG(map_.find(id) != map_.end(), nullptr, "Unexpected window id: %{public}d", id);
    return std::shared_ptr<WindowWrapper>();
}

void WindowManager::DestroyWindowWrapper(WindowId id)
{
    CHECK_AND_RETURN_LOG(map_.find(id) != map_.end(), "Unexpected window id: %{public}d", id);
    map_.erase(id);
}

void WindowManager::DestroyWindowWrapper(std::shared_ptr<WindowWrapper> wrapper)
{
    DestroyWindowWrapper(wrapper->GetWindowId());
}
} // Sample
} // MediaAVCodec
} // OHOS