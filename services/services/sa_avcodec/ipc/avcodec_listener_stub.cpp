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
#include "avcodec_listener_stub.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "i_avcodec_service.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecListenerStub"};
}

namespace OHOS {
namespace MediaAVCodec {
AVCodecListenerStub::AVCodecListenerStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecListenerStub::~AVCodecListenerStub()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecListenerStub::OnRemoteRequest(uint32_t code, MessageParcel &data,
                                             MessageParcel &reply, MessageOption &option)
{
    AVCODEC_LOGD("Stub: OnRemoteRequest of code: %{public}u is received", code);

    auto remoteDescriptor = data.ReadInterfaceToken();
    CHECK_AND_RETURN_RET_LOG(AVCodecListenerStub::GetDescriptor() == remoteDescriptor, AVCS_ERR_INVALID_OPERATION,
                             "Invalid descriptor");
    int32_t ret = -1;
    switch (code) {
        case static_cast<uint32_t>(CodecListenerInterfaceCode::ON_OUTPUT_STRATEGY_INFO):
            ret = HandleOnOutputStrategyInfo(data, reply);
            break;
        default:
            AVCODEC_LOGW("AVCodecListenerStub: no member func supporting, applying default process");
            return IPCObjectStub::OnRemoteRequest(code, data, reply, option);
    }
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Failed to handle OnOutputStrategyInfo");
    return ret;
}

int32_t AVCodecListenerStub::HandleOnOutputStrategyInfo(MessageParcel &data, MessageParcel &reply)
{
    std::string bundleName;
    int32_t codecDecision;
    bool readRet = data.ReadString(bundleName);
    CHECK_AND_RETURN_RET_LOG(readRet, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Failed to read bundleName from data");
    readRet = data.ReadInt32(codecDecision);
    CHECK_AND_RETURN_RET_LOG(readRet, AVCS_ERR_IPC_GET_SUB_SYSTEM_ABILITY_FAILED,
        "Failed to read codecDecision from data");
    return OnOutputStrategyInfo(bundleName, codecDecision);
}

int32_t AVCodecListenerStub::OnOutputStrategyInfo(const std::string &bundleName, int32_t codecDecision)
{
    AVCODEC_LOGD("Stub OnOutputStrategyInfo: bundleName=%{public}s, codecDecision=%{public}d",
                 bundleName.c_str(), codecDecision);
    return AVCodecServiceFactory::GetInstance().OnOutputStrategyInfo(bundleName, codecDecision);
}
} // namespace MediaAVCodec
} // namespace OHOS
