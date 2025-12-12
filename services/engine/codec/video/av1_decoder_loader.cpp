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
#include "av1_decoder_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "av1decoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
using Av1Decoder = Codec::Av1Decoder;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Av1DecoderLoader"};
const char *AV1_DECODER_LIB_PATH = "libav1_decoder.z.so";
const char *AV1_DECODER_CREATE_FUNC_NAME = "CreateAv1DecoderByName";
const char *AV1_DECODER_GETCAPS_FUNC_NAME = "GetAv1DecoderCapabilityList";
} // namespace

std::shared_ptr<CodecBase> Av1DecoderLoader::CreateByName(const std::string &name)
{
    Av1DecoderLoader &loader = GetInstance();

    CodecBase *noDeleterPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, nullptr, "Create codec by name failed: init error");
        noDeleterPtr = loader.Create(name).get();
        CHECK_AND_RETURN_RET_LOG(noDeleterPtr != nullptr, nullptr, "Create av1decoder by name failed");
        ++(loader.av1DecoderCount_);
    }
    auto deleter = [&loader](CodecBase *ptr) {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        Av1Decoder *codec = static_cast<Av1Decoder*>(ptr);
        codec->DecStrongRef(codec);
        --(loader.av1DecoderCount_);
        loader.CloseLibrary();
    };
    return std::shared_ptr<CodecBase>(noDeleterPtr, deleter);
}

int32_t Av1DecoderLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    Av1DecoderLoader &loader = GetInstance();
    std::lock_guard<std::mutex> lock(loader.mutex_);
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get capability failed: init error");
    int32_t ret = loader.GetCaps(caps);
    loader.CloseLibrary();
    return ret;
}

Av1DecoderLoader::Av1DecoderLoader() : VideoCodecLoader(AV1_DECODER_LIB_PATH,
    AV1_DECODER_CREATE_FUNC_NAME, AV1_DECODER_GETCAPS_FUNC_NAME) {}

void Av1DecoderLoader::CloseLibrary()
{
    if (av1DecoderCount_ != 0) {
        return;
    }
    Close();
}

Av1DecoderLoader &Av1DecoderLoader::GetInstance()
{
    static Av1DecoderLoader loader;
    return loader;
}
} // namespace MediaAVCodec
} // namespace OHOS