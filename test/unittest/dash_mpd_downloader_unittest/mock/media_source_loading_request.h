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

#ifndef HISTREAMER_MEDIA_SOURCE_LOADING_REQUEST_H
#define HISTREAMER_MEDIA_SOURCE_LOADING_REQUEST_H

#include <string>
#include <iostream>
#include "app_client.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_STREAM_SOURCE, "HiStreamer" };

class LoadingRequestElements {
public:
    LoadingRequestElements(int64_t uuid, const std::shared_ptr<NetworkClient> &client) : uuid_(uuid),
        client_(client) {}
    ~LoadingRequestElements();

    int32_t RespondData(int64_t uuid, int64_t offset, const std::shared_ptr<AVSharedMemory> &data);

    int32_t RespondHeader(int64_t uuid, std::map<std::string, std::string> header, std::string redirectUrl);

    int32_t FinishLoading(int64_t uuid, LoadingRequestError state);

    int64_t uuid_ {0};
    std::shared_ptr<NetworkClient> client_ {};
};

class MediaSourceLoadingRequest : public IMediaSourceLoadingRequest {
public:
    int64_t Open(int64_t uuid, const std::shared_ptr<NetworkClient> &client);
    
    int32_t Close(int64_t uuid);

    ~MediaSourceLoadingRequest() override;

    int32_t RespondData(int64_t uuid, int64_t offset, const std::shared_ptr<AVSharedMemory> &data) override;

    int32_t RespondHeader(int64_t uuid, std::map<std::string, std::string> header, std::string redirectUrl) override;

    int32_t FinishLoading(int64_t uuid, LoadingRequestError state) override;

private:
    std::map<int64_t, std::shared_ptr<LoadingRequestElements>> requestMap_ {};
    FairMutex clientMutex_{};
};

class MediaSourceLoaderCombinations {
public:
    explicit MediaSourceLoaderCombinations(std::shared_ptr<IMediaSourceLoader> loader);
    ~MediaSourceLoaderCombinations();

    int64_t Open(const std::string &url, const std::map<std::string, std::string> &header,
                 std::shared_ptr<NetworkClient> &client);
    int32_t Close(int64_t uuid);
    void EnableOfflineCache(bool enable);

    bool GetenableOfflineCache();
    std::shared_ptr<MediaSourceLoadingRequest> request_ {nullptr};
    std::shared_ptr<IMediaSourceLoader> loader_  {nullptr};
    bool enable_;
};
}
}
}
}
#endif