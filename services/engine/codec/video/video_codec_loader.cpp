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
#include "video_codec_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "VideoCodecLoader"};
} // namespace

int32_t VideoCodecLoader::Init()
{
    if (codecHandle_ != nullptr) {
        return AVCS_ERR_OK;
    }
    void *handle = dlopen(libPath_, RTLD_LAZY);
    CHECK_AND_RETURN_RET_LOG(handle != nullptr, AVCS_ERR_UNKNOWN, "Load codec failed: %{public}s", libPath_);
    auto handleSP = std::shared_ptr<void>(handle, dlclose);
    auto createFunc = reinterpret_cast<CreateByNameFuncType>(dlsym(handle, createFuncName_));
    CHECK_AND_RETURN_RET_LOG(createFunc != nullptr, AVCS_ERR_UNKNOWN, "Load createFunc failed: %{public}s",
                             createFuncName_);
    auto getCapsFunc = reinterpret_cast<GetCapabilityFuncType>(dlsym(handle, getCapsFuncName_));
    CHECK_AND_RETURN_RET_LOG(getCapsFunc != nullptr, AVCS_ERR_UNKNOWN, "Load getCapsFunc failed: %{public}s",
                             getCapsFuncName_);
    codecHandle_ = handleSP;
    createFunc_ = createFunc;
    getCapsFunc_ = getCapsFunc;

    AVCODEC_LOGI("Init library:%{public}s", libPath_);
    return AVCS_ERR_OK;
}

void VideoCodecLoader::Close()
{
    codecHandle_ = nullptr;
    createFunc_ = nullptr;
    getCapsFunc_ = nullptr;
    AVCODEC_LOGI("Close library:%{public}s", libPath_);
}

std::shared_ptr<CodecBase> VideoCodecLoader::Create(const std::string &name)
{
    std::shared_ptr<CodecBase> codec;
    (void)createFunc_(name, codec);
    return codec;
}

int32_t VideoCodecLoader::GetCaps(std::vector<CapabilityData> &caps)
{
    return getCapsFunc_(caps);
}
} // namespace MediaAVCodec
} // namespace OHOS