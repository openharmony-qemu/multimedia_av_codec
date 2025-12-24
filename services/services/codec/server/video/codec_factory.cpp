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

#include "codec_factory.h"
#include <cinttypes>
#include <dlfcn.h>
#include <limits>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "codec_ability_singleton.h"
#include "codeclist_core.h"
#include "meta/format.h"

#include "fcodec_loader.h"
#include "hevc_decoder_loader.h"
#include "hcodec_loader.h"
#include "avc_encoder_loader.h"

#ifdef SUPPORT_CODEC_VP8
#include "vp8_decoder_loader.h"
#endif
#ifdef SUPPORT_CODEC_VP9
#include "vp9_decoder_loader.h"
#endif
#ifdef SUPPORT_CODEC_AV1
#include "av1_decoder_loader.h"
#endif

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecFactory"};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
CodecFactory &CodecFactory::Instance()
{
    static CodecFactory inst;
    return inst;
}

CodecFactory::~CodecFactory() {}

std::vector<std::string> CodecFactory::GetCodecNameArrayByMime(const AVCodecType type, const std::string &mime)
{
    auto codecListCore = std::make_shared<CodecListCore>();
    auto nameArray = codecListCore->FindCodecNameArray(type, mime);
    auto checkFunc = [](const std::string &str) { return str.find("secure") != std::string::npos; };
    nameArray.erase(std::remove_if(nameArray.begin(), nameArray.end(), checkFunc), nameArray.end());
    return nameArray;
}

std::shared_ptr<CodecBase> CodecFactory::CreateCodecByName(const std::string &name)
{
    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    CodecType codecType = codecListCore->FindCodecType(name);
    std::shared_ptr<CodecBase> codec = nullptr;
    switch (codecType) {
        case CodecType::AVCODEC_HCODEC:
            codec = HCodecLoader::CreateByName(name);
            break;
        case CodecType::AVCODEC_VIDEO_CODEC:
            codec = FCodecLoader::CreateByName(name);
            break;
        case CodecType::AVCODEC_VIDEO_HEVC_DECODER:
            codec = HevcDecoderLoader::CreateByName(name);
            break;
        case CodecType::AVCODEC_VIDEO_AVC_ENCODER:
            codec = AvcEncoderLoader::CreateByName(name);
            break;
#ifdef SUPPORT_CODEC_AV1
        case CodecType::AVCODEC_VIDEO_AV1_DECODER:
            codec = Av1DecoderLoader::CreateByName(name);
            break;
#endif
#ifdef SUPPORT_CODEC_VP8
        case CodecType::AVCODEC_VIDEO_VP8_DECODER:
            codec = Vp8DecoderLoader::CreateByName(name);
            break;
#endif
#ifdef SUPPORT_CODEC_VP9
        case CodecType::AVCODEC_VIDEO_VP9_DECODER:
            codec = Vp9DecoderLoader::CreateByName(name);
            break;
#endif
        default:
            AVCODEC_LOGE("Create codec %{public}s failed", name.c_str());
            break;
    }
    return codec;
}
} // namespace MediaAVCodec
} // namespace OHOS
