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

#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "codeclist_builder.h"
#ifndef CLIENT_SUPPORT_CODEC
#include "hcodec_loader.h"
#endif
#include "codec_ability_singleton.h"

namespace OHOS {
namespace MediaAVCodec {
CodecAbilitySingleton &CodecAbilitySingleton::GetInstance()
{
    static std::shared_ptr<CodecAbilitySingleton> instance = nullptr;
    instance = std::make_shared<CodecAbilitySingleton>();
    return *instance;
}

void CodecAbilitySingleton::RegisterCapabilityArray(std::vector<CapabilityData> &capaArray, CodecType codecType)
{
    CodecAbilitySingletonImpl::RegisterCapabilityArray(capaArray, codecType);
}

std::vector<CapabilityData> CodecAbilitySingleton::GetCapabilityArray()
{
    return CodecAbilitySingletonImpl::GetCapabilityArray();
}

std::optional<CapabilityData> CodecAbilitySingleton::GetCapabilityByName(const std::string &name)
{
    return CodecAbilitySingletonImpl::GetCapabilityByName(name);
}

std::unordered_map<std::string, CodecType> CodecAbilitySingleton::GetNameCodecTypeMap()
{
    return CodecAbilitySingletonImpl::GetNameCodecTypeMap();
}

std::unordered_map<std::string, std::vector<size_t>> CodecAbilitySingleton::GetMimeCapIdxMap()
{
    return CodecAbilitySingletonImpl::GetMimeCapIdxMap();
}
} // namespace MediaAVCodec
} // namespace OHOS
