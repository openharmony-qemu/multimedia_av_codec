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
 
#define HST_LOG_TAG "AppClient"
 
#include <fstream>
#include <algorithm>
#include <fcntl.h>
#include <map>
#include "app_client.h"
#include "avcodec_trace.h"
#include "osal/task/task.h"
 
 
namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "HiStreamer" };
    constexpr size_t MAX_MAP_SIZE = 100;
    constexpr int DROP_APP_DATA = -2;
    constexpr int64_t DEFAULT_CURRENT_OFFSET = -2;
    constexpr int BUFFER_FULL = -3;
    constexpr int RETRY_SLEEP_TIME = 500; // ms
    constexpr int FINISHLOADING_SLEEP_TIME = 10; // ms
}
 
AppClient::AppClient(RxHeader headCallback, RxBody bodyCallback, void *userParam)
    : rxHeader_(headCallback), rxBody_(bodyCallback), userParam_(userParam)
{
    MEDIA_LOG_I("0x%{public}06" PRIXPTR " AppClient create", FAKE_POINTER(this));
}
 
AppClient::~AppClient()
{
}
 
void AppClient::MapToString(std::map<std::string, std::string> httpHeader)
{
    std::string headerStr = "";
    if (httpHeader.size() > MAX_MAP_SIZE) { // The length of map exceeds the limit
        rxHeader_(nullptr, 0, 0, userParam_);
        return;
    }
    std::map<std::string, std::string> httpHeaderTmp = httpHeader;
    for (std::map<std::string, std::string>::iterator iter = httpHeaderTmp.begin();
        iter != httpHeaderTmp.end(); iter++) {
        headerStr = iter->first + " : " + iter->second;
        char* headerTmp = (char*)headerStr.c_str();
        MEDIA_LOG_D("0x%{public}06" PRIXPTR " AppClient header: " PUBLIC_LOG_S,
            FAKE_POINTER(this), (char*)headerStr.c_str());
        void* buffer = reinterpret_cast<void*>(headerTmp);
        rxHeader_(buffer, 1, 1, userParam_);
    }
}
 
Status AppClient::Init()
{
    return Status::OK;
}
 
Status AppClient::Open(const std::string& url, const std::map<std::string, std::string>& httpHeader,
    int32_t timeoutMs)
{
    (void)url;
    (void)httpHeader;
    (void)timeoutMs;
    return Status::OK;
}
 
Status AppClient::RequestData(long startPos, int len, const RequestInfo& requestInfo,
    HandleResponseCbFunc completedCb)
{
    MediaAVCodec::AVCodecTrace trace("AppClient RequestData, startPos: " +
        std::to_string(startPos) + ", len: " + std::to_string(len));
    (void)requestInfo;
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "AppClient RequestData in, startPos: "
        PUBLIC_LOG_D32 " len: " PUBLIC_LOG_D32, FAKE_POINTER(this), static_cast<int>(startPos), len);
 
    if (startPos == -1) {
        len = -1;
        startPos = 0;
    }
    len_ = len;
    startPos_ = startPos;
    curOffset_ = static_cast<int64_t>(startPos);
    dataInFlight_ = len;
 
    int32_t clientCode = 0;
    int32_t serverCode = 0;
    LoadingRequestError requestState;
    {
        AutoLock lock(mutex_);
        isResponseCompleted_.store(false);
        int32_t res = sourceLoader_->Read(uuid_, static_cast<int64_t>(startPos), static_cast<int64_t>(len));
        FALSE_LOG_MSG(res == 0, "sourceLoader read fail.");
 
        responseCondition_.Wait(lock, [this] {
            return isResponseCompleted_.load();
        });
        requestState = requestState_;
    }
    clientCode = static_cast<int32_t>(requestState) * (-1);
 
    Status ret = Status::OK;
    if (requestState_ == LoadingRequestError::LOADING_ERROR_SUCCESS) {
        MEDIA_LOG_I("0x%{public}06" PRIXPTR "AppClient RequestData success", FAKE_POINTER(this));
        completedCb(clientCode, serverCode, ret);
    } else {
        MEDIA_LOG_I("0x%{public}06" PRIXPTR "AppClient RequestData fail, clientCode: " PUBLIC_LOG_D32
            " requestState_: " PUBLIC_LOG_D32, FAKE_POINTER(this), clientCode, requestState_);
 
        Task::SleepInTask(RETRY_SLEEP_TIME);
        ret = Status::ERROR_CLIENT;
        completedCb(clientCode, serverCode, ret);
    }
    requestState_ = LoadingRequestError::LOADING_ERROR_SUCCESS;
    return Status::OK;
}
 
Status AppClient::Close(bool isAsync)
{
    (void)isAsync;
    NotifyResponseDataEnd(LoadingRequestError::LOADING_ERROR_SUCCESS);
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "AppClient Close.", FAKE_POINTER(this));
    return Status::OK;
}
 
Status AppClient::Deinit()
{
    return Status::OK;
}
 
Status AppClient::GetIp(std::string &ip)
{
    (void)ip;
    return Status::OK;
}
 
void AppClient::SetAppUid(int32_t appUid)
{
}
 
std::shared_ptr<NetworkClient> NetworkClient::GetAppInstance(RxHeader headCallback,
    RxBody bodyCallback, void *userParam)
{
    return std::make_shared<AppClient>(headCallback, bodyCallback, userParam);
}
 
void AppClient::SetLoader(std::shared_ptr<IMediaSourceLoader> sourceLoader)
{
    sourceLoader_ = sourceLoader;
}
 
int32_t AppClient::RespondHeader(int64_t uuid, const std::map<std::string, std::string>& httpHeader,
    std::string redirectUrl)
{
    MediaAVCodec::AVCodecTrace trace("AppClient RespondHeader, uuid: " + std::to_string(uuid));
    FALSE_LOG_MSG(uuid > 0, "sourceLoader read fail.");
    std::map<std::string, std::string> httpHeaderTmp = httpHeader;
    if (!redirectUrl.empty()) {
        httpHeaderTmp.emplace("location", redirectUrl);
    }
    MapToString(httpHeaderTmp);
    return 0;
}
 
void AppClient::NotifyResponseDataEnd(LoadingRequestError state)
{
    {
        AutoLock lock(mutex_);
        requestState_ = state;
        isResponseCompleted_.store(true);
        responseCondition_.NotifyOne();
        if (state == LoadingRequestError::LOADING_ERROR_SUCCESS) {
            curOffset_ = DEFAULT_CURRENT_OFFSET;
        }
    }
    MEDIA_LOG_D("0x%{public}06" PRIXPTR "AppClient NotifyResponseDataEnd state: " PUBLIC_LOG_D32,
        FAKE_POINTER(this), static_cast<int32_t>(state));
}

int32_t AppClient::RespondData(int64_t uuid, int64_t offset, const std::shared_ptr<AVSharedMemory> memory)
{
    FALSE_RETURN_V(memory != nullptr, 0);
    if (uuid != uuid_) {
        MEDIA_LOG_E("0x%{public}06" PRIXPTR " AppClient respondData uuid invalid, uuid: " PUBLIC_LOG_D64,
            FAKE_POINTER(this), uuid);
        NotifyResponseDataEnd(LoadingRequestError::LOADING_ERROR_NOT_READY);
        return DROP_APP_DATA;
    }
 
    if (curOffset_ != offset) {
        MEDIA_LOG_E("0x%{public}06" PRIXPTR " AppClient respondData offset invalid, offset: " PUBLIC_LOG_D64
            " curOffset_: " PUBLIC_LOG_D32, FAKE_POINTER(this), offset, static_cast<int32_t>(curOffset_));
        NotifyResponseDataEnd(LoadingRequestError::LOADING_ERROR_NOT_READY);
        return DROP_APP_DATA;
    }
 
    void* buffer = reinterpret_cast<void*>(memory->GetBase());
    size_t res = rxBody_(buffer, memory->GetSize(), 1, userParam_);
    curOffset_ += static_cast<int64_t>(res);
    if (res == 0) {
        MEDIA_LOG_W("0x%{public}06" PRIXPTR " AppClient respondData buffer full, can not write, uuid: " PUBLIC_LOG_D64,
            FAKE_POINTER(this), uuid);
        NotifyResponseDataEnd(LoadingRequestError::LOADING_ERROR_SUCCESS);
        return BUFFER_FULL;
    }
    
    int32_t receiveDataSize = memory->GetSize();
    MediaAVCodec::AVCodecTrace trace("AppClient RespondData, uuid: " + std::to_string(uuid) +
        ", offset: " + std::to_string(offset) + ", receiveDataSize: " + std::to_string(receiveDataSize));
    dataInFlight_ -= receiveDataSize;
 
    if (len_ > 0 && dataInFlight_ <= 0) {
        NotifyResponseDataEnd(LoadingRequestError::LOADING_ERROR_SUCCESS);
    }
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " AppClient RespondData, uuid " PUBLIC_LOG_D64 " offset " PUBLIC_LOG_D64
        " dataInFlight_ " PUBLIC_LOG_D32 " size " PUBLIC_LOG_D32, FAKE_POINTER(this), uuid, offset,
        dataInFlight_, receiveDataSize);
    return receiveDataSize;
}
 
int32_t AppClient::FinishLoading(int64_t uuid, LoadingRequestError state)
{
    FALSE_RETURN_V_MSG_E(uuid == uuid_, 0, "FinishLoading uuid invalid.");
    MEDIA_LOG_I("0x%{public}06" PRIXPTR "AppClient FinishLoading uuid " PUBLIC_LOG_D64, FAKE_POINTER(this), uuid);
    Task::SleepInTask(FINISHLOADING_SLEEP_TIME);
    NotifyResponseDataEnd(state);
    return 0;
}
 
void AppClient::SetUuid(int64_t uuid)
{
    FALSE_RETURN_MSG(uuid > 0, "SetUuid uuid invalid.");
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " AppClient SetUuid " PUBLIC_LOG_D64, FAKE_POINTER(this), uuid);
    uuid_ = uuid;
}
 
std::string AppClient::GetRedirectUrl()
{
    return redirectUrl_;
}
}
}
}
}