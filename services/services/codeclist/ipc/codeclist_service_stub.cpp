/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <unistd.h>
#include <string>
#include <map>
#include "avsharedmemory_ipc.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_server_manager.h"
#include "avcodec_xcollie.h"
#include "codeclist_service_stub.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListServiceStub"};

    const std::map<uint32_t, std::string> CODECLIST_FUNC_NAME = {
        { static_cast<uint32_t>(OHOS::MediaAVCodec::AVCodecListServiceInterfaceCode::FIND_DECODER),
            "CodecListServiceStub DoFindDecoder" },
        { static_cast<uint32_t>(OHOS::MediaAVCodec::AVCodecListServiceInterfaceCode::FIND_ENCODER),
            "CodecListServiceStub DoFindEncoder" },
        { static_cast<uint32_t>(OHOS::MediaAVCodec::AVCodecListServiceInterfaceCode::GET_CAPABILITY),
            "CodecListServiceStub DoGetCapability" },
        { static_cast<uint32_t>(OHOS::MediaAVCodec::AVCodecListServiceInterfaceCode::DESTROY),
            "CodecListServiceStub DoDestroyStub" },
    };
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
sptr<CodecListServiceStub> CodecListServiceStub::Create()
{
    sptr<CodecListServiceStub> codecListStub = new (std::nothrow) CodecListServiceStub();
    CHECK_AND_RETURN_RET_LOG(codecListStub != nullptr, nullptr, "Create codeclist service stub failed");

    int32_t ret = codecListStub->Init();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Init codeclist stub failed");
    return codecListStub;
}

CodecListServiceStub::CodecListServiceStub()
{
    AVCODEC_LOGD("Create codeclist service stub instance successful");
}

CodecListServiceStub::~CodecListServiceStub()
{
    AVCODEC_LOGD("Destroy codeclist service stub instance successful");
}

int32_t CodecListServiceStub::Init()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    codecListServer_ = CodecListServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, AVCS_ERR_NO_MEMORY, "Create codecList server failed");
    return AVCS_ERR_OK;
}

int32_t CodecListServiceStub::DestroyStub()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    codecListServer_ = nullptr;
    AVCodecServerManager::GetInstance().DestroyStubObject(AVCodecServerManager::CODECLIST, AsObject());
    return AVCS_ERR_OK;
}

int CodecListServiceStub::OnRemoteRequest(uint32_t code, MessageParcel &data, MessageParcel &reply,
                                          MessageOption &option)
{
    AVCODEC_LOGD("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    if (CodecListServiceStub::GetDescriptor() != remoteDescriptor) {
        AVCODEC_LOGE("Invalid descriptor");
        return AVCS_ERR_INVALID_OPERATION;
    }
    int32_t ret = -1;
    auto itFuncName = CODECLIST_FUNC_NAME.find(code);
    std::string funcName =
        itFuncName != CODECLIST_FUNC_NAME.end() ? itFuncName->second : "CodecListServiceStub OnRemoteRequest";
    switch (code) {
        case static_cast<uint32_t>(AVCodecListServiceInterfaceCode::FIND_DECODER):
            ret = DoFindDecoder(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecListServiceInterfaceCode::FIND_ENCODER):
            ret = DoFindEncoder(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecListServiceInterfaceCode::GET_CAPABILITY):
            ret = DoGetCapability(data, reply);
            break;
        case static_cast<uint32_t>(AVCodecListServiceInterfaceCode::DESTROY):
            ret = DoDestroyStub(data, reply);
            break;
        default:
            AVCODEC_LOGW("CodecListServiceStub: no member func supporting, applying default process");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Failed to call member func %{public}s", funcName.c_str());
    return ret;
}

std::string CodecListServiceStub::FindDecoder(const Format &format)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, "", "Find decoder failed: avcodeclist server is nullptr");
    return codecListServer_->FindDecoder(format);
}

std::string CodecListServiceStub::FindEncoder(const Format &format)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, "", "Find encoder failed: avcodeclist server is nullptr");
    return codecListServer_->FindEncoder(format);
}

int32_t CodecListServiceStub::GetCapability(CapabilityData &capabilityData, const std::string &mime,
                                            const bool isEncoder, const AVCodecCategory &category)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListServer_ != nullptr, AVCS_ERR_NO_MEMORY,
                             "Get capability failed: avcodeclist server is null");
    return codecListServer_->GetCapability(capabilityData, mime, isEncoder, category);
}

int32_t CodecListServiceStub::DoFindDecoder(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);
    reply.WriteString(FindDecoder(format));
    return AVCS_ERR_OK;
}

int32_t CodecListServiceStub::DoFindEncoder(MessageParcel &data, MessageParcel &reply)
{
    Format format;
    (void)AVCodecParcel::Unmarshalling(data, format);
    reply.WriteString(FindEncoder(format));
    return AVCS_ERR_OK;
}

int32_t CodecListServiceStub::DoGetCapability(MessageParcel &data, MessageParcel &reply)
{
    std::string mime = data.ReadString();
    bool isEncoder = data.ReadBool();
    AVCodecCategory category = static_cast<AVCodecCategory>(data.ReadInt32());
    CapabilityData capabilityData;
    (void)GetCapability(capabilityData, mime, isEncoder, category);
    (void)CodecListParcel::Marshalling(reply, capabilityData);
    return AVCS_ERR_OK;
}

int32_t CodecListServiceStub::DoDestroyStub(MessageParcel &data, MessageParcel &reply)
{
    (void)data;
    reply.WriteInt32(DestroyStub());
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS