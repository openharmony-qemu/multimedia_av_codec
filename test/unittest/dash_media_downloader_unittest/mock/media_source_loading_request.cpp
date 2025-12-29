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
#include "media_source_loading_request.h"
namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
MediaSourceLoaderCombinations::MediaSourceLoaderCombinations(std::shared_ptr<IMediaSourceLoader> loader)
{
    loader_ = loader;
}
MediaSourceLoaderCombinations::~MediaSourceLoaderCombinations()
{
}
int64_t MediaSourceLoaderCombinations::Open(const std::string &url,
    const std::map<std::string, std::string> &header, std::shared_ptr<NetworkClient> &client)
{
    return 0;
}

int32_t MediaSourceLoaderCombinations::Close(int64_t uuid)
{
    return 0;
}

void MediaSourceLoaderCombinations::EnableOfflineCache(bool enable)
{
    enable_ = enable;
}

bool MediaSourceLoaderCombinations::GetenableOfflineCache()
{
    return enable_;
}

}
}
}
}