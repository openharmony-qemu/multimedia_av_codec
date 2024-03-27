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

#include "codeclist_service_proxy.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_parcel.h"
#include "codeclist_parcel.h"


namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListServiceProxy"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
CodecListServiceProxy::CodecListServiceProxy(const sptr<IRemoteObject> &impl)
    : IRemoteProxy<IStandardCodecListService>(impl)
{
    AVCODEC_LOGD("Create codeclist proxy successful");
}

CodecListServiceProxy::~CodecListServiceProxy()
{
    AVCODEC_LOGD("Destroy codecList proxy successful");
}

std::string CodecListServiceProxy::FindDecoder(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool token = data.WriteInterfaceToken(CodecListServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, "", "Failed to write descriptor!");
    (void)AVCodecParcel::Marshalling(data, format);
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(AVCodecListServiceInterfaceCode::FIND_DECODER), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, "", "Find decoder failed");
    return reply.ReadString();
}

std::string CodecListServiceProxy::FindEncoder(const Format &format)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool token = data.WriteInterfaceToken(CodecListServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, "", "Failed to write descriptor!");
    (void)AVCodecParcel::Marshalling(data, format);
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(AVCodecListServiceInterfaceCode::FIND_ENCODER), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, "", "Find encoder failed");
    return reply.ReadString();
}

int32_t CodecListServiceProxy::GetCapability(CapabilityData &capabilityData, const std::string &mime,
                                             const bool isEncoder, const AVCodecCategory &category)
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    Format profileFormat;
    bool token = data.WriteInterfaceToken(CodecListServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_UNKNOWN, "Write descriptor failed!");
    (void)data.WriteString(mime);
    (void)data.WriteBool(isEncoder);
    (void)data.WriteInt32(static_cast<int32_t>(category));
    int32_t ret = Remote()->SendRequest(static_cast<uint32_t>(AVCodecListServiceInterfaceCode::GET_CAPABILITY), data,
                                        reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_UNKNOWN,
                             "GetCodecCapabilityInfos failed, send request error");
    CHECK_AND_RETURN_RET_LOG(CodecListParcel::Unmarshalling(reply, capabilityData), AVCS_ERR_UNKNOWN,
                             "GetCodecCapabilityInfos failed, Unmarshalling error");
    return AVCS_ERR_OK;
}

int32_t CodecListServiceProxy::DestroyStub()
{
    MessageParcel data;
    MessageParcel reply;
    MessageOption option;
    bool token = data.WriteInterfaceToken(CodecListServiceProxy::GetDescriptor());
    CHECK_AND_RETURN_RET_LOG(token, AVCS_ERR_INVALID_OPERATION, "Write descriptor failed!");
    int32_t ret =
        Remote()->SendRequest(static_cast<uint32_t>(AVCodecListServiceInterfaceCode::DESTROY), data, reply, option);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,
                             "Destroy codeclist stub failed, send request error");
    return reply.ReadInt32();
}
} // namespace MediaAVCodec
} // namespace OHOS