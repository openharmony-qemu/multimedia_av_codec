/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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
#include "avcodec_trace.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

// -------------------------------LoadingRequestElements----------------------------------------
LoadingRequestElements::~LoadingRequestElements()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " Elements dtor, uuid " PUBLIC_LOG_D64, FAKE_POINTER(this), uuid_);
    if (client_ != nullptr) {
        client_.reset();
    }
}

int32_t LoadingRequestElements::RespondData(int64_t uuid, int64_t offset, const std::shared_ptr<AVSharedMemory> &data)
{
    if (client_ != nullptr) {
        return client_->RespondData(uuid, offset, data);
    }
    return 0;
}

int32_t LoadingRequestElements::RespondHeader(int64_t uuid, std::map<std::string, std::string> header,
    std::string redirectUrl)
{
    if (client_ != nullptr) {
        return client_->RespondHeader(uuid, header, redirectUrl);
    }
    return 0;
}

int32_t LoadingRequestElements::FinishLoading(int64_t uuid, LoadingRequestError state)
{
    if (client_ != nullptr) {
        return client_->FinishLoading(uuid, state);
    }
    return 0;
}

// -------------------------------MediaSourceLoadingRequest----------------------------------------
int64_t MediaSourceLoadingRequest::Open(int64_t uuid, const std::shared_ptr<NetworkClient> &client)
{
    MediaAVCodec::AVCodecTrace trace("MediaSourceLoadingRequest Open, uuid: " + std::to_string(uuid));
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest Open, uuid: " PUBLIC_LOG_D64,
        FAKE_POINTER(this), uuid);
    AutoLock lock(clientMutex_);
    auto it = requestMap_.find(uuid);
    if (it != requestMap_.end()) {
        MEDIA_LOG_W("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest Open, uuid has opened: " PUBLIC_LOG_D64,
            FAKE_POINTER(this), uuid);
        return 0;
    }
    auto element = std::make_shared<LoadingRequestElements>(uuid, client);
    FALSE_RETURN_V_MSG(element != nullptr, 0, "MediaSourceLoadingRequest Open, no enough memory.");
    requestMap_.emplace(uuid, element);
    return 0;
}

int32_t MediaSourceLoadingRequest::Close(int64_t uuid)
{
    MediaAVCodec::AVCodecTrace trace("MediaSourceLoadingRequest Close, uuid: " + std::to_string(uuid));
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "MediaSOurceLoadingRequest Close, uuid: " PUBLIC_LOG_D64,
        FAKE_POINTER(this), uuid);
    AutoLock lock(clientMutex_);
    auto it = requestMap_.find(uuid);
    if (it != requestMap_.end()) {
        requestMap_.erase(it);
    } else {
        MEDIA_LOG_W("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest Close, invalid uuid: " PUBLIC_LOG_D64,
            FAKE_POINTER(this), uuid);
    }
    return 0;
}

MediaSourceLoadingRequest::~MediaSourceLoadingRequest()
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest dtor in.", FAKE_POINTER(this));
    AutoLock lock(clientMutex_);
    requestMap_.clear();
}

int32_t MediaSourceLoadingRequest::RespondData(int64_t uuid, int64_t offset,
    const std::shared_ptr<AVSharedMemory> &request)
{
    MediaAVCodec::AVCodecTrace trace("MediaSourceLoadingRequest RespondData, uuid: " + std::to_string(uuid) +
        ", offset: " + std::to_string(offset));
    MEDIA_LOG_D("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest RespondData, uuid: " PUBLIC_LOG_D64
        " offset: " PUBLIC_LOG_D64, FAKE_POINTER(this), uuid, offset);
    AutoLock lock(clientMutex_);
    auto it = requestMap_.find(uuid);
    if (it != requestMap_.end()) {
        return it->second->RespondData(uuid, offset, request);
    } else {
        MEDIA_LOG_E("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest RespondData, invalid uuid: " PUBLIC_LOG_D64,
            FAKE_POINTER(this), uuid);
    }
    return 0;
}

int32_t MediaSourceLoadingRequest::RespondHeader(int64_t uuid, std::map<std::string, std::string> header,
    std::string redirectUrl)
{
    MediaAVCodec::AVCodecTrace trace("MediaSourceLoadingRequest RespondHeader, uuid: " + std::to_string(uuid));
    MEDIA_LOG_D("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest RespondHeader, uuid: " PUBLIC_LOG_D64,
        FAKE_POINTER(this), uuid);
    AutoLock lock(clientMutex_);
    auto it = requestMap_.find(uuid);
    if (it != requestMap_.end()) {
        return it->second->RespondHeader(uuid, header, redirectUrl);
    } else {
        MEDIA_LOG_E("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest RespondHeader, invalid uuid: " PUBLIC_LOG_D64,
            FAKE_POINTER(this), uuid);
    }
    return 0;
}

int32_t MediaSourceLoadingRequest::FinishLoading(int64_t uuid, LoadingRequestError state)
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest FinishLoading, uuid: " PUBLIC_LOG_D64
        " state: " PUBLIC_LOG_D32, FAKE_POINTER(this), uuid, static_cast<int32_t>(state));
    AutoLock lock(clientMutex_);
    auto it = requestMap_.find(uuid);
    if (it != requestMap_.end()) {
        return it->second->FinishLoading(uuid, state);
    } else {
        MEDIA_LOG_E("0x%{public}06" PRIXPTR "MediaSourceLoadingRequest FinishLoading, invalid uuid: " PUBLIC_LOG_D64,
            FAKE_POINTER(this), uuid);
    }
    return 0;
}

// -------------------------------MediaSourceLoaderCombinations----------------------------------------
int64_t MediaSourceLoaderCombinations::Open(const std::string &url, const std::map<std::string, std::string> &header,
    std::shared_ptr<NetworkClient> &client)
{
    FALSE_RETURN_V_MSG(loader_ != nullptr, 0, "Open no loader!");
    if (request_ == nullptr) {
        request_ = std::make_shared<MediaSourceLoadingRequest>();
        FALSE_RETURN_V_MSG(request_ != nullptr, 0, "MediaSourceLoaderCombinations Open, no enough memory.");
        std::shared_ptr<IMediaSourceLoadingRequest> request =
            std::static_pointer_cast<IMediaSourceLoadingRequest>(request_);
        loader_->Init(request);
    }
    int64_t uuid = loader_->Open(url, header);
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "MediaSourceLoaderCombinations Open, uuid: " PUBLIC_LOG_D64,
        FAKE_POINTER(this), uuid);
    request_->Open(uuid, client);
    return uuid;
}

int32_t MediaSourceLoaderCombinations::Close(int64_t uuid)
{
    FALSE_RETURN_V_MSG(loader_ != nullptr, 0, "Close no loader!");
    request_->Close(uuid);
    int32_t ret = loader_->Close(uuid);
    return ret;
}

void MediaSourceLoaderCombinations::enableOfflineCache(bool enable)
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