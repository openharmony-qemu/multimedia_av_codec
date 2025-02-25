/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "video_post_processor_factory.h"

namespace OHOS {
namespace Media {

VideoPostProcessorFactory& VideoPostProcessorFactory::Instance()
{
    static VideoPostProcessorFactory instance;
    return instance;
}

std::shared_ptr<BaseVideoPostProcessor> VideoPostProcessorFactory::CreateVideoPostProcessorPriv(
    const VideoPostProcessorType type)
{
    auto it = generators_.find(type);
    if (it != generators_.end()) {
        return it->second();
    }
    return nullptr;
}

bool VideoPostProcessorFactory::IsPostProcessorSupportedPriv(const VideoPostProcessorType type,
                                                             const std::shared_ptr<Meta>& meta)
{
    auto it = checkers_.find(type);
    if (it != checkers_.end()) {
        return it->second(meta);
    }
    return false;
}
} // namespace Media
} // namespace OHOS
