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
#include "avs3_decoder_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avs3_decoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
using Avs3Decoder = Codec::Avs3Decoder;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Avs3DecoderLoader"};
const char *AVS3_DECODER_LIB_PATH = "libavs3_decoder.z.so";
const char *AVS3_DECODER_CREATE_FUNC_NAME = "CreateAvs3DecoderByName";
const char *AVS3_DECODER_GETCAPS_FUNC_NAME = "GetAvs3DecoderCapabilityList";
} // namespace

std::shared_ptr<CodecBase> Avs3DecoderLoader::CreateByName(const std::string &name)
{
    Avs3DecoderLoader &loader = GetInstance();

    CodecBase *noDeleterPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, nullptr, "Create codec by name failed: init error");
        noDeleterPtr = loader.Create(name).get();
        CHECK_AND_RETURN_RET_LOG(noDeleterPtr != nullptr, nullptr, "Create avs3decoder by name failed");
        ++(loader.avs3DecoderCount_);
    }
    auto deleter = [&loader](CodecBase *ptr) {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        Avs3Decoder *codec = static_cast<Avs3Decoder*>(ptr);
        codec->DecStrongRef(codec);
        --(loader.avs3DecoderCount_);
        loader.CloseLibrary();
    };
    return std::shared_ptr<CodecBase>(noDeleterPtr, deleter);
}

int32_t Avs3DecoderLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    Avs3DecoderLoader &loader = GetInstance();
    std::lock_guard<std::mutex> lock(loader.mutex_);
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get capability failed: init error");
    int32_t ret = loader.GetCaps(caps);
    loader.CloseLibrary();
    return ret;
}

Avs3DecoderLoader::Avs3DecoderLoader() : VideoCodecLoader(AVS3_DECODER_LIB_PATH,
    AVS3_DECODER_CREATE_FUNC_NAME, AVS3_DECODER_GETCAPS_FUNC_NAME) {}

void Avs3DecoderLoader::CloseLibrary()
{
    if (avs3DecoderCount_ != 0) {
        return;
    }
    Close();
}

Avs3DecoderLoader &Avs3DecoderLoader::GetInstance()
{
    static Avs3DecoderLoader loader;
    return loader;
}
} // namespace MediaAVCodec
} // namespace OHOS