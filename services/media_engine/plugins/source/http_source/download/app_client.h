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
 
#ifndef HISTREAMER_APP_CLIENT_H
#define HISTREAMER_APP_CLIENT_H
 
#include <string>
#include "network/network_client.h"
#include <memory>
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "plugin/plugin_buffer.h"
#include "osal/task/mutex.h"
#include "osal/task/condition_variable.h"
#include "common/status.h"
#include "common/log.h"
 
namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class AppClient : public NetworkClient {
public:
    AppClient(RxHeader headCallback, RxBody bodyCallback, void* userParam);
 
    ~AppClient() override;
 
    Status Init() override;
 
    Status Open(const std::string& url, const std::map<std::string, std::string>& httpHeader,
                int32_t timeoutMs) override;
 
    Status RequestData(long startPos, int len, const RequestInfo& requestInfo,
        HandleResponseCbFunc completedCb) override;
 
    Status Close(bool isAsync) override;
 
    Status Deinit() override;
 
    Status GetIp(std::string &ip) override;
 
    void SetAppUid(int32_t appUid) override;
 
    void SetLoader(std::shared_ptr<IMediaSourceLoader> sourceLoader) override;
 
    int32_t RespondHeader(int64_t uuid, const std::map<std::string, std::string>& httpHeader,
                       std::string redirectUrl) override;
 
    int32_t RespondData(int64_t uuid, int64_t offset, const std::shared_ptr<AVSharedMemory> memory) override;
 
    int32_t FinishLoading(int64_t uuid, LoadingRequestError state) override;
 
    void SetUuid(int64_t uuid) override;
 
    std::string GetRedirectUrl() override;
 
private:
    void NotifyResponseDataEnd(LoadingRequestError state);
    void MapToString(std::map<std::string, std::string> httpHeader);
 
private:
    RxHeader rxHeader_;
    RxBody rxBody_;
    void *userParam_;
 
    std::shared_ptr<IMediaSourceLoader> sourceLoader_;
    int64_t uuid_ {0};
    std::atomic<bool> isResponseCompleted_ {false};
    ConditionVariable responseCondition_{};
    LoadingRequestError requestState_ = LoadingRequestError::LOADING_ERROR_SUCCESS;
    mutable FairMutex mutex_;
    int dataInFlight_ {0};
    long startPos_ {0};
    int len_ {0};
    std::string redirectUrl_ {""};
    int64_t curOffset_ {-2};
};
}
}
}
}
#endif