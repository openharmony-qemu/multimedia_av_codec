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
#include "vp9_decoder_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "vpxDecoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
using VpxDecoder = Codec::VpxDecoder;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "Vp9DecoderLoader"};
const char *VP9_DECODER_LIB_PATH = "libvpx_decoder.z.so";
const char *VP9_DECODER_CREATE_FUNC_NAME = "CreateVpxDecoderByName";
const char *VP9_DECODER_GETCAPS_FUNC_NAME = "GetVpxDecoderCapabilityList";
} // namespace

std::shared_ptr<CodecBase> Vp9DecoderLoader::CreateByName(const std::string &name)
{
    Vp9DecoderLoader &loader = GetInstance();

    CodecBase *noDeleterPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, nullptr, "Create codec by name failed: init error");
        noDeleterPtr = loader.Create(name).get();
        CHECK_AND_RETURN_RET_LOG(noDeleterPtr != nullptr, nullptr, "Create vpxdecoder by name failed");
        ++(loader.vp9DecoderCount_);
    }
    auto deleter = [&loader](CodecBase *ptr) {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        VpxDecoder *codec = static_cast<VpxDecoder*>(ptr);
        codec->DecStrongRef(codec);
        --(loader.vp9DecoderCount_);
        loader.CloseLibrary();
    };
    return std::shared_ptr<CodecBase>(noDeleterPtr, deleter);
}

int32_t Vp9DecoderLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    Vp9DecoderLoader &loader = GetInstance();
    std::lock_guard<std::mutex> lock(loader.mutex_);
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get capability failed: init error");
    int32_t ret = loader.GetCaps(caps);
    loader.CloseLibrary();
    return ret;
}

Vp9DecoderLoader::Vp9DecoderLoader() : VideoCodecLoader(VP9_DECODER_LIB_PATH,
    VP9_DECODER_CREATE_FUNC_NAME, VP9_DECODER_GETCAPS_FUNC_NAME) {}

void Vp9DecoderLoader::CloseLibrary()
{
    if (vp9DecoderCount_ != 0) {
        return;
    }
    Close();
}

Vp9DecoderLoader &Vp9DecoderLoader::GetInstance()
{
    static Vp9DecoderLoader loader;
    return loader;
}
} // namespace MediaAVCodec
} // namespace OHOS