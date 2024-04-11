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

#include "codec_ability_singleton.h"
#define PRINT_HILOG
#include "unittest_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecAbilitySingletonMock"};
const OHOS::MediaAVCodec::Range DEFAULT_RANGE = {96, 4096};
const std::vector<int32_t> DEFALUT_PIXFORMAT = {1, 2, 3};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
CodecAbilitySingleton &CodecAbilitySingleton::GetInstance()
{
    static CodecAbilitySingleton instance;
    return instance;
}

CodecAbilitySingleton::CodecAbilitySingleton()
{
    UNITTEST_INFO_LOG("Succeed");
}

CodecAbilitySingleton::~CodecAbilitySingleton()
{
    UNITTEST_INFO_LOG("Succeed");
}

std::optional<CapabilityData> CodecAbilitySingleton::GetCapabilityByName(std::string name)
{
    (void)name;
    CapabilityData data;
    data.width = DEFAULT_RANGE;
    data.height = DEFAULT_RANGE;
    data.pixFormat = DEFALUT_PIXFORMAT;
    return data;
}
} // namespace MediaAVCodec
} // namespace OHOS
