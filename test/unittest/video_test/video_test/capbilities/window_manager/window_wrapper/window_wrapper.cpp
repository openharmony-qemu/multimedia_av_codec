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

#include "window_wrapper.h"
#include "av_codec_sample_log.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
WindowWrapper::WindowWrapper(SampleWindowType windowType, std::shared_ptr<OHNativeWindow> window)
{
    windowType_ = windowType;
    window_ = window;
}

SampleWindowType WindowWrapper::GetWindowType()
{
    return windowType_;
}

WindowId WindowWrapper::GetWindowId()
{
    return windowId_;
}

void WindowWrapper::SetWindowId(WindowId id)
{
    windowId_ = id;
}

std::shared_ptr<OHNativeWindow> WindowWrapper::GetWindow()
{
    return window_;
}

bool WindowWrapper::SelfCheck()
{
    return (window_ == nullptr) && (windowType_ > SampleWindowType::UNKNOWN) && (windowType_ > SampleWindowType::END);
}
} // Sample
} // MediaAVCodec
} // OHOS