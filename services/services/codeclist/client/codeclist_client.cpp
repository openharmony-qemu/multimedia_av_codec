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

#include "codeclist_client.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListClient"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
std::shared_ptr<CodecListClient> CodecListClient::Create(const sptr<IStandardCodecListService> &ipcProxy)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "Create codeclist client failed: ipcProxy is nullptr");
    std::shared_ptr<CodecListClient> codecList = std::make_shared<CodecListClient>(ipcProxy);
    return codecList;
}

CodecListClient::CodecListClient(const sptr<IStandardCodecListService> &ipcProxy) : codecListProxy_(ipcProxy)
{
    AVCODEC_LOGD("Create CodecListClient instances successful");
}

CodecListClient::~CodecListClient()
{
    std::lock_guard<std::mutex> lock(mutex_);
    if (codecListProxy_ != nullptr) {
        (void)codecListProxy_->DestroyStub();
    }
    AVCODEC_LOGD("Destroy codecListClient instances successful");
}

void CodecListClient::AVCodecServerDied()
{
    std::lock_guard<std::mutex> lock(mutex_);
    codecListProxy_ = nullptr;
}

std::string CodecListClient::FindDecoder(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListProxy_ != nullptr, "", "Find decoder failed: codeclist service does not exist.");
    return codecListProxy_->FindDecoder(format);
}

std::string CodecListClient::FindEncoder(const Format &format)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListProxy_ != nullptr, "", "Find encoder failed: codeclist service does not exist.");
    return codecListProxy_->FindEncoder(format);
}

int32_t CodecListClient::GetCapability(CapabilityData &capabilityData, const std::string &mime, const bool isEncoder,
                                       const AVCodecCategory &category)
{
    std::lock_guard<std::mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecListProxy_ != nullptr, AVCS_ERR_NO_MEMORY,
                             "Get capability failed: codeclist service does not exist.");
    return codecListProxy_->GetCapability(capabilityData, mime, isEncoder, category);
}
} // namespace MediaAVCodec
} // namespace OHOS