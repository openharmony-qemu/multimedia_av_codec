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

#include "codeclist_server.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecListServer"};
}

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
std::shared_ptr<ICodecListService> CodecListServer::Create()
{
    std::shared_ptr<CodecListServer> server = std::make_shared<CodecListServer>();
    if (!server->Init()) {
        AVCODEC_LOGE("failed to init CodecListServer");
        return nullptr;
    }
    return server;
}

CodecListServer::CodecListServer()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecListServer::~CodecListServer()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

bool CodecListServer::Init()
{
    codecListCore_ = std::make_shared<CodecListCore>();
    return true;
}

std::string CodecListServer::FindDecoder(const Format &format)
{
    return codecListCore_->FindDecoder(format);
}

std::string CodecListServer::FindEncoder(const Format &format)
{
    return codecListCore_->FindEncoder(format);
}

int32_t CodecListServer::GetCapability(CapabilityData &capabilityData, const std::string &mime, const bool isEncoder,
                                       const AVCodecCategory &category)
{
    return codecListCore_->GetCapability(capabilityData, mime, isEncoder, category);
}
} // namespace MediaAVCodec
} // namespace OHOS
