/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include "hevc_decoder_loader.h"
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "hevc_decoder.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
using HevcDecoder = Codec::HevcDecoder;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "HevcDecoderLoader"};
const char *HEVC_DECODER_LIB_PATH = "libhevc_decoder.z.so";
const char *HEVC_DECODER_CREATE_FUNC_NAME = "CreateHevcDecoderByName";
const char *HEVC_DECODER_GETCAPS_FUNC_NAME = "GetHevcDecoderCapabilityList";
} // namespace

std::shared_ptr<CodecBase> HevcDecoderLoader::CreateByName(const std::string &name)
{
    HevcDecoderLoader &loader = GetInstance();

    CodecBase *noDeleterPtr = nullptr;
    {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, nullptr, "Create codec by name failed: init error");
        noDeleterPtr = loader.Create(name).get();
        CHECK_AND_RETURN_RET_LOG(noDeleterPtr != nullptr, nullptr, "Create hevcdecoder by name failed: no memory");
        ++(loader.hevcDecoderCount_);
    }
    auto deleter = [&loader](CodecBase *ptr) {
        std::lock_guard<std::mutex> lock(loader.mutex_);
        HevcDecoder *codec = reinterpret_cast<HevcDecoder*>(ptr);
        codec->DecStrongRef(codec);
        --(loader.hevcDecoderCount_);
        loader.CloseLibrary();
    };
    return std::shared_ptr<CodecBase>(noDeleterPtr, deleter);
}

int32_t HevcDecoderLoader::GetCapabilityList(std::vector<CapabilityData> &caps)
{
    HevcDecoderLoader &loader = GetInstance();

    std::lock_guard<std::mutex> lock(loader.mutex_);
    CHECK_AND_RETURN_RET_LOG(loader.Init() == AVCS_ERR_OK, AVCS_ERR_UNKNOWN, "Get capability failed: init error");
    int32_t ret = loader.GetCaps(caps);
    loader.CloseLibrary();
    return ret;
}

HevcDecoderLoader::HevcDecoderLoader() : VideoCodecLoader(HEVC_DECODER_LIB_PATH,
    HEVC_DECODER_CREATE_FUNC_NAME, HEVC_DECODER_GETCAPS_FUNC_NAME) {}

void HevcDecoderLoader::CloseLibrary()
{
    if (hevcDecoderCount_ != 0) {
        return;
    }
    Close();
}

HevcDecoderLoader &HevcDecoderLoader::GetInstance()
{
    static HevcDecoderLoader loader;
    return loader;
}
} // namespace MediaAVCodec
} // namespace OHOS