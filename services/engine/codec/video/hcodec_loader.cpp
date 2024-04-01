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
#include <dlfcn.h>
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "hcodec_loader.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "codecLoader"};
const char *HCODEC_CREATE_FUNC_NAME = "CreateHCodecByName";
const char *HCODEC_GETCAPS_FUNC_NAME = "GetHCodecCapabilityList";
const char *HCODEC_LIB_PATH = "libhcodec.z.so";
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

HCodecLoader &HCodecLoader::GetInstance()
{
    static HCodecLoader loader;
    return loader;
}

int32_t HCodecLoader::Init()
{
    if (codecHandle_ != nullptr) {
        return AVCS_ERR_OK;
    }
    void *handle = dlopen(HCODEC_LIB_PATH, RTLD_LAZY);
    CHECK_AND_RETURN_RET_LOG(handle != nullptr, AVCS_ERR_UNKNOWN, "Load codec failed: %{public}s", HCODEC_LIB_PATH);
    auto handleSP = std::shared_ptr<void>(handle, dlclose);
    auto createFunc = reinterpret_cast<CreateByNameFuncType>(dlsym(handle, HCODEC_CREATE_FUNC_NAME));
    CHECK_AND_RETURN_RET_LOG(createFunc != nullptr, AVCS_ERR_UNKNOWN, "Load createFunc failed: %{public}s",
                             HCODEC_CREATE_FUNC_NAME);
    auto getCapsFunc = reinterpret_cast<GetCapabilityFuncType>(dlsym(handle, HCODEC_GETCAPS_FUNC_NAME));
    CHECK_AND_RETURN_RET_LOG(getCapsFunc != nullptr, AVCS_ERR_UNKNOWN, "Load getCapsFunc failed: %{public}s",
                             HCODEC_GETCAPS_FUNC_NAME);
    codecHandle_ = handleSP;
    createFunc_ = createFunc;
    getCapsFunc_ = getCapsFunc;
    return AVCS_ERR_OK;
}

std::shared_ptr<CodecBase> HCodecLoader::Create(const std::string &name)
{
    std::shared_ptr<CodecBase> codec;
    createFunc_(name, codec);
    return codec;
}

int32_t HCodecLoader::GetCaps(std::vector<CapabilityData> &caps)
{
    return getCapsFunc_(caps);
}
} // namespace MediaAVCodec
} // namespace OHOS