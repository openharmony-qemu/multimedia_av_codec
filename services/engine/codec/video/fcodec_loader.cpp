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
#include "fcodec_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "FCodecLoader"};
const char *FCODEC_LIB_PATH = "libfcodec.z.so";
const char *FCODEC_CREATE_FUNC_NAME = "CreateFCodecByName";
const char *FCODEC_GETCAPS_FUNC_NAME = "GetFCodecCapabilityList";

} // namespace
std::shared_ptr<CodecBase> FCodecLoader::CreateByName(const std::string &name)
{
    FCodecLoader &loader = GetInstance();

    CodecBase *noDeleterPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, nullptr, "Create codec by name failed: init error");
        noDeleterPtr = loader.Create(name).get();
        CHECK_AND_RETURN_RET_LOG(noDeleterPtr != nullptr, nullptr, "Create fcodec by name failed: no memory");
        ++(loader.fcodecCount_);
    }
    auto deleter = [&loader](CodecBase *ptr) {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        delete ptr;
        --(loader.fcodecCount_);
        loader.CloseLibrary();
    };
    return std::shared_ptr<CodecBase>(noDeleterPtr, deleter);
}

int32_t FCodecLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    FCodecLoader &loader = GetInstance();

    std::lock_guard<std::mutex> lock(loader.mutex_);
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get capability failed: init error");
    int32_t ret = loader.GetCaps(caps);
    loader.CloseLibrary();
    return ret;
}

FCodecLoader::FCodecLoader() : VideoCodecLoader(FCODEC_LIB_PATH, FCODEC_CREATE_FUNC_NAME, FCODEC_GETCAPS_FUNC_NAME) {}

void FCodecLoader::CloseLibrary()
{
    if (fcodecCount_) {
        return;
    }
    Close();
}

FCodecLoader &FCodecLoader::GetInstance()
{
    static FCodecLoader loader;
    return loader;
}
} // namespace MediaAVCodec
} // namespace OHOS