/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "avc_encoder_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avc_encoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
using AvcEncoder = Codec::AvcEncoder;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AvcEncoderLoader"};
const char *AVC_ENCODER_LIB_PATH = "libavc_encoder.z.so";
const char *AVC_ENCODER_CREATE_FUNC_NAME = "CreateAvcEncoderByName";
const char *AVC_ENCODER_GETCAPS_FUNC_NAME = "GetAvcEncoderCapabilityList";
} // namespace

std::shared_ptr<CodecBase> AvcEncoderLoader::CreateByName(const std::string &name)
{
    AvcEncoderLoader &loader = GetInstance();

    std::lock_guard<std::mutex> lock(loader.mutex_);
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, nullptr, "Create codec by name failed: init error");
    std::shared_ptr<CodecBase> noDeletePtr = loader.Create(name);
    if (noDeletePtr == nullptr) {
        AVCODEC_LOGE("Loader create coder by name failed!");
        loader.CloseLibrary();
    }
    return noDeletePtr;
}

int32_t AvcEncoderLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    AvcEncoderLoader &loader = GetInstance();

    std::lock_guard<std::mutex> lock(loader.mutex_);
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get capability failed: init error");
    int32_t ret = loader.GetCaps(caps);
    if (ret != AVCS_ERR_OK) {
        AVCODEC_LOGE("Loader get caps failed!");
        loader.CloseLibrary();
    }
    return ret;
}

AvcEncoderLoader::AvcEncoderLoader() : VideoCodecLoader(AVC_ENCODER_LIB_PATH,
    AVC_ENCODER_CREATE_FUNC_NAME, AVC_ENCODER_GETCAPS_FUNC_NAME) {}

void AvcEncoderLoader::CloseLibrary()
{
    Close();
}

AvcEncoderLoader &AvcEncoderLoader::GetInstance()
{
    static AvcEncoderLoader loader;
    return loader;
}
} // namespace MediaAVCodec
} // namespace OHOS