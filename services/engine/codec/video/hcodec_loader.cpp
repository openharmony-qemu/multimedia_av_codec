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
#include "hcodec_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "HCodecLoader"};
const char *HCODEC_LIB_PATH = "libhcodec.z.so";
const char *HCODEC_CREATE_FUNC_NAME = "CreateHCodecByName";
const char *HCODEC_GETCAPS_FUNC_NAME = "GetHCodecCapabilityList";
} // namespace
std::shared_ptr<CodecBase> HCodecLoader::CreateByName(const std::string &name)
{
    HCodecLoader &loader = GetInstance();
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, nullptr, "Create codec by name failed: init error");
    return loader.Create(name);
}

int32_t HCodecLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    HCodecLoader &loader = GetInstance();
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get capability failed: init error");
    return loader.GetCaps(caps);
}

HCodecLoader::HCodecLoader() : VideoCodecLoader(HCODEC_LIB_PATH, HCODEC_CREATE_FUNC_NAME, HCODEC_GETCAPS_FUNC_NAME) {}

HCodecLoader &HCodecLoader::GetInstance()
{
    static HCodecLoader loader;
    return loader;
}
} // namespace MediaAVCodec
} // namespace OHOS