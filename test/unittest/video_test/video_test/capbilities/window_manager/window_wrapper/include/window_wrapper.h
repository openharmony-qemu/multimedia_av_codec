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

#ifndef AVCODEC_SAMPLE_WINDOW_WRAPPER_H
#define AVCODEC_SAMPLE_WINDOW_WRAPPER_H

#include <memory>

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
using OHNativeWindow = struct NativeWindow;
using WindowId = int32_t;
enum class SampleWindowType : int32_t {
    UNKNOWN,
    NATIVE_IMAGE,
    ENCODER,
#ifdef SAMPLE_BUILD_TO_EXECUTOR
    ROSEN,
#endif
#ifdef SAMPLE_BUILD_TO_HAP
    XCOMPONENT,
#endif
    END,
};

class WindowWrapper {
public:
    WindowWrapper() {}
    WindowWrapper(SampleWindowType windowType, std::shared_ptr<OHNativeWindow> window);
    SampleWindowType GetWindowType();
    WindowId GetWindowId();
    void SetWindowId(WindowId id);
    std::shared_ptr<OHNativeWindow> GetWindow();
    bool SelfCheck();

private:
    SampleWindowType windowType_;
    WindowId windowId_ = -1;
    std::shared_ptr<OHNativeWindow> window_;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_WINDOW_WRAPPER_H