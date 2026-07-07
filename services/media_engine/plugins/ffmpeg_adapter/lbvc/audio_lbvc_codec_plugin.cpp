/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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
#include "audio_lbvc_decoder_plugin.h"
#include "audio_lbvc_encoder_plugin.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "plugin/plugin_definition.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Lbvc;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioLbvcCodecPlugin"};

Status RegisterAudioCodecPlugins(const std::shared_ptr<Register>& reg)
{
    CodecPluginDef decoderDef;
    decoderDef.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_LBVC_NAME);
    decoderDef.pluginType = PluginType::AUDIO_DECODER;
    decoderDef.rank = 100;  // 100
    decoderDef.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioLbvcDecoderPlugin>(name);
    });

    Capability decoderCap;
    decoderCap.SetMime(MimeType::AUDIO_LBVC);
    decoderCap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    decoderDef.AddInCaps(decoderCap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(decoderDef) != Status::OK) {
        AVCODEC_LOGE("AudioLbvcDecoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    CodecPluginDef encoderDef;
    encoderDef.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_LBVC_NAME);
    encoderDef.pluginType = PluginType::AUDIO_ENCODER;
    encoderDef.rank = 100;  // 100
    encoderDef.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioLbvcEncoderPlugin>(name);
    });

    Capability encoderCap;
    encoderCap.SetMime(MimeType::AUDIO_LBVC);
    encoderCap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    encoderDef.AddInCaps(encoderCap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(encoderDef) != Status::OK) {
        AVCODEC_LOGE("AudioLbvcEncoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioCodecPlugin() {}

PLUGIN_DEFINITION(LbvcAudioCodec, LicenseType::VENDOR, RegisterAudioCodecPlugins,
    UnRegisterAudioCodecPlugin);
} // namespace