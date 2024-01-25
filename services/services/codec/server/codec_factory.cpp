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


#include "codec_factory.h"
#include <limits>
#include <cinttypes>
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "audio_codec_adapter.h"
#include "audio_codec.h"
#include "codeclist_core.h"
#include "codeclist_utils.h"
#include "meta/format.h"
#ifndef CLIENT_SUPPORT_CODEC
#include "fcodec.h"
#include "hcodec_loader.h"
#endif
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecFactory"};
}

namespace OHOS {
namespace MediaAVCodec {
CodecFactory &CodecFactory::Instance()
{
    static CodecFactory inst;
    return inst;
}

CodecFactory::~CodecFactory()
{
}

std::shared_ptr<CodecBase> CodecFactory::CreateCodecByMime(bool isEncoder,
    const std::string &mime, API_VERSION apiVersion)
{
    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    std::string codecname;
    Format format;
    format.PutStringValue("codec_mime", mime);
    if (isEncoder) {
        codecname = codecListCore->FindEncoder(format);
    } else {
        codecname = codecListCore->FindDecoder(format);
    }
    CHECK_AND_RETURN_RET_LOG(!codecname.empty(), nullptr, "Create codec by mime failed: error mime type");
    
    std::shared_ptr<CodecBase> codec = CreateCodecByName(codecname, apiVersion);
    EXPECT_AND_LOGI(codec != nullptr, "Succeed");
    return codec;
}

std::shared_ptr<CodecBase> CodecFactory::CreateCodecByName(const std::string &name, API_VERSION apiVersion)
{
    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    CodecType codecType = codecListCore->FindCodecType(name);
    std::shared_ptr<CodecBase> codec = nullptr;
    switch (codecType) {
#ifndef CLIENT_SUPPORT_CODEC
        case CodecType::AVCODEC_HCODEC:
            codec = HCodecLoader::CreateByName(name);
            break;
        case CodecType::AVCODEC_VIDEO_CODEC:
            codec = std::make_shared<Codec::FCodec>(name);
            break;
#else
        case CodecType::AVCODEC_AUDIO_CODEC:
            if (apiVersion == API_VERSION::API_VERSION_10) {
                codec = std::make_shared<AudioCodecAdapter>(name);
            } else {
                codec = std::make_shared<AudioCodec>();
                auto ret = codec->CreateCodecByName(name);
                CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr,
                                         "Create codec by name:%{public}s failed", name.c_str());
            }
            break;
#endif
        default:
            AVCODEC_LOGE("Create codec %{public}s failed", name.c_str());
            return codec;
    }
    (void)apiVersion;
    AVCODEC_LOGD("Create codec %{public}s successful", name.c_str());
    return codec;
}
} // namespace MediaAVCodec
} // namespace OHOS
