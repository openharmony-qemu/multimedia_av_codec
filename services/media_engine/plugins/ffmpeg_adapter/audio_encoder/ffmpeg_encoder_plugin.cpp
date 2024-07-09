/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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

#include "ffmpeg_encoder_plugin.h"
#include <cstring>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <utility>
#include <string_view>
#include "osal/utils/util.h"
#include "common/log.h"
#include "avcodec_log.h"
#include "avcodec_codec_name.h"
#include "meta/mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::Ffmpeg;
using namespace OHOS::MediaAVCodec;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "FFmpegEncoderPlugin"};

std::vector<std::string_view> codecVec = {
    AVCodecCodecName::AUDIO_ENCODER_AAC_NAME,
    AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME,
};

void SetDefinition(size_t index, CodecPluginDef &definition, Capability &cap);

void SetDefinition(size_t index, CodecPluginDef &definition, Capability &cap)
{
    switch (index) {
        case 0: // 0:aac
            cap.SetMime(MimeType::AUDIO_AAC);
            definition.name = AVCodecCodecName::AUDIO_ENCODER_AAC_NAME;
            definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
                return std::make_shared<FFmpegAACEncoderPlugin>(name);
            });
            break;
        case 1: // 1:flac
            cap.SetMime(MimeType::AUDIO_FLAC);
            definition.name = AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME;
            definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
                return std::make_shared<FFmpegFlacEncoderPlugin>(name);
            });
            break;
        default:
            MEDIA_LOG_I("codec is not supported right now");
    }
}

Status RegisterAudioEncoderPlugins(const std::shared_ptr<Register> &reg)
{
    for (size_t i = 0; i < codecVec.size(); i++) {
        CodecPluginDef definition;
        definition.pluginType = PluginType::AUDIO_ENCODER;
        definition.rank = 100; // 100:rank
        Capability cap;
        SetDefinition(i, definition, cap);
        cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);
        definition.AddInCaps(cap);
        // do not delete the codec in the deleter
        if (reg->AddPlugin(definition) != Status::OK) {
            AVCODEC_LOGD("register enc-plugin codecName:%{public}s failed", definition.name.c_str());
        }
        AVCODEC_LOGD("register enc-plugin codecName:%{public}s", definition.name.c_str());
    }
    return Status::OK;
}

void UnRegisterAudioEncoderPlugin() {}

PLUGIN_DEFINITION(FFmpegAudioEncoders, LicenseType::LGPL, RegisterAudioEncoderPlugins, UnRegisterAudioEncoderPlugin);
} // namespace