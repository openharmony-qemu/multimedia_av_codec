/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef RS_INTERFACE_ADAPTER
#define RS_INTERFACE_ADAPTER
#include "rs_interface_adapter.h"

#define OHOS_ROSEN_RS_CONNECT_TOKEN u"ohos.rosen.RenderService"
#define RS_CREATE_CONNECTION 0
#define OHOS_ROSEN_RS_SERVICE_CONNECT_TOKEN u"ohos.rosen.ClientToServiceConnection"
#define NOTIFY_VIDEO_DEC_INFO 0x10002

namespace {
constexpr uint32_t MAX_VIDEO_INFO_SIZE = 32;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "DBA"};
}

namespace OHOS {
namespace MediaAVCodec {
RSAdapterDeathRecipient::RSAdapterDeathRecipient(
    std::function<void(const wptr<IRemoteObject> &object)> fun)
{
    mDeathFun = fun;
}

RSAdapterDeathRecipient::~RSAdapterDeathRecipient()
{}

void RSAdapterDeathRecipient::OnRemoteDied(const wptr<IRemoteObject> &object)
{
    if (mDeathFun) {
        mDeathFun(object);
    }
}

RSInterfaceAdapter::RSInterfaceAdapter()
{
    auto resetTask = [](const wptr<IRemoteObject> &object) { RSInterfaceAdapter::GetInstance().ResetConnect(); };
    mRSInterfaceAdapterDeathRecipient = new RSAdapterDeathRecipient(resetTask);
}

RSInterfaceAdapter &RSInterfaceAdapter::GetInstance()
{
    static RSInterfaceAdapter instance;
    return instance;
}

bool RSInterfaceAdapter::ConnectRenderService()
{
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    if (mHgmRemoteObject != nullptr) {
        return true;
    }
    if (mHgmToken == nullptr) {
        mHgmToken = new IRemoteStub<RSIConnectionToken>();
    }
    auto samgr = SystemAbilityManagerClient::GetInstance().GetSystemAbilityManager();
    if (!samgr) {
        AVCODEC_LOGE("RSInterfaceAdapter::ConnectRenderService samgr is null");
        return false;
    }
    mRenderServiceRemoteObject = samgr->GetSystemAbility(RENDER_SERVICE);
    if (mRenderServiceRemoteObject == nullptr || !mRenderServiceRemoteObject->IsProxyObject()) {
        AVCODEC_LOGE("RSInterfaceAdapter::ConnectRenderService mRenderServiceRemoteObject is nullptr");
        return false;
    }
    if (!mRenderServiceRemoteObject->AddDeathRecipient(mRSInterfaceAdapterDeathRecipient)) {
        AVCODEC_LOGE("RSInterfaceAdapter::ConnectRenderService Failed to add death recipient");
        mRenderServiceRemoteObject = nullptr;
        return false;
    }
    mHgmRemoteObject = CreateConnection();
    return mHgmRemoteObject != nullptr;
}

sptr<IRemoteObject> RSInterfaceAdapter::CreateConnection()
{
    if (mHgmToken == nullptr || mRenderServiceRemoteObject == nullptr) {
        AVCODEC_LOGE("RSInterfaceAdapter::CreateConnection mRenderServiceRemoteObject is null");
        return nullptr;
    }
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    option.SetFlags(MessageOption::TF_SYNC);
    if (!data.WriteInterfaceToken(OHOS_ROSEN_RS_CONNECT_TOKEN)) {
        AVCODEC_LOGE("RSInterfaceAdapter::CreateConnection WriteInterfaceToken failed.");
        return nullptr;
    }

    data.WriteRemoteObject(mHgmToken->AsObject());
    int32_t err = mRenderServiceRemoteObject->SendRequest(RS_CREATE_CONNECTION, data, reply, option);
    if (err != NO_ERROR) {
        AVCODEC_LOGE("RSInterfaceAdapter::CreateConnection SendRequest failed, err is %{public}d.", err);
        return nullptr;
    }

    auto remoteObj = reply.ReadRemoteObject();
    if (remoteObj == nullptr || !remoteObj->IsProxyObject()) {
        AVCODEC_LOGE("RSInterfaceAdapter::CreateConnection Reply is not valid.");
        return nullptr;
    }
    return remoteObj;
}

void RSInterfaceAdapter::ResetConnect()
{
    if (mRenderServiceRemoteObject && mRenderServiceRemoteObject->IsProxyObject()) {
        mRenderServiceRemoteObject->RemoveDeathRecipient(mRSInterfaceAdapterDeathRecipient);
    }
    mRenderServiceRemoteObject = nullptr;
    mHgmRemoteObject = nullptr;
}

void RSInterfaceAdapter::NotifyFrameRateEvent(const std::unordered_map<std::string, std::string>& videoRateInfo)
{
    if (!ConnectRenderService()) {
        AVCODEC_LOGE("RSInterfaceAdapter::NotifyFrameRateEvent connect faild.");
        return;
    }
    auto mapSize = videoRateInfo.size();
    if (mapSize <= 0 || mapSize > MAX_VIDEO_INFO_SIZE) {
        AVCODEC_LOGE("RSInterfaceAdapter: map size err.");
        return;
    }
 
    MessageParcel data;
    MessageParcel reply;
    MessageOption option(MessageOption::TF_ASYNC);
    if (!data.WriteInterfaceToken(OHOS_ROSEN_RS_SERVICE_CONNECT_TOKEN)) {
        AVCODEC_LOGE("%{public}s: Write InterfaceToken val err.", __func__);
        return;
    }
 
    if (!data.WriteUint32(mapSize)) {
        AVCODEC_LOGE("%{public}s: Write UInt32 val err.", __func__);
        return;
    }
 
    for (auto const &it : videoRateInfo) {
        if (!data.WriteString(it.first) || !data.WriteString(it.second)) {
            AVCODEC_LOGE("%{public}s: write key value failed!", __func__);
            return;
        }
    }
    int ret = mHgmRemoteObject->SendRequest(NOTIFY_VIDEO_DEC_INFO, data, reply, option);
    if (ret != 0) {
        AVCODEC_LOGE("%{public}s: SendRequest failed. err:%{public}d.", __func__, ret);
        return;
    }
    return;
}

}  // namespace Rosen
}  // namespace OHOS
#endif  // !RS_INTERFACE_ADAPTER
