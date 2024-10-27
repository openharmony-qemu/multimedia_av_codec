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

#ifndef AVCODEC_SAMPLE_WINDOW_MANAGER_H
#define AVCODEC_SAMPLE_WINDOW_MANAGER_H

#include <memory>
#include <unordered_map>
#include "window_wrapper.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class WindowManager {
public:
    static WindowManager &GetInstance();
    std::shared_ptr<WindowWrapper> CreateWindowWrapper(SampleWindowType windowType);
    std::shared_ptr<WindowWrapper> CreateWindowWrapper(
        SampleWindowType windowType, std::shared_ptr<OHNativeWindow> window);
    std::shared_ptr<WindowWrapper> GetWindowWrapper(WindowId id);
    void DestroyWindowWrapper(WindowId id);
    void DestroyWindowWrapper(std::shared_ptr<WindowWrapper> wrapper);

private:
    std::unordered_map<WindowId, std::shared_ptr<WindowWrapper>> map_;
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_WINDOW_MANAGER_H