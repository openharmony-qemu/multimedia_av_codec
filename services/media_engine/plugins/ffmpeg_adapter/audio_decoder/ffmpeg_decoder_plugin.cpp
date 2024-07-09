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
#include "ffmpeg_decoder_plugin.h"
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
    AVCodecCodecName::AUDIO_DECODER_MP3_NAME, AVCodecCodecName::AUDIO_DECODER_AAC_NAME,
    AVCodecCodecName::AUDIO_DECODER_FLAC_NAME, AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME,
    AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME, AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME,
    AVCodecCodecName::AUDIO_DECODER_APE_NAME
};

void SetDefinition(size_t index, CodecPluginDef &definition, Capability &cap);

template <class T>
void InitDefinition(const std::string &mimetype, const std::string_view &codecName,
                    CodecPluginDef &definition, Capability &cap)
{
    cap.SetMime(mimetype);
    definition.name = codecName;
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<T>(name);
    });
}

void SetDefinition(size_t index, CodecPluginDef &definition, Capability &cap)
{
    switch (index) {
        case 0: // 0:mp3
            InitDefinition<FFmpegMp3DecoderPlugin>(MimeType::AUDIO_MPEG,
                AVCodecCodecName::AUDIO_DECODER_MP3_NAME, definition, cap);
            break;
        case 1: // 1:aac
            InitDefinition<FFmpegAACDecoderPlugin>(MimeType::AUDIO_AAC,
                AVCodecCodecName::AUDIO_DECODER_AAC_NAME, definition, cap);
            break;
        case 2: // 2:flac
            InitDefinition<FFmpegFlacDecoderPlugin>(MimeType::AUDIO_FLAC,
                AVCodecCodecName::AUDIO_DECODER_FLAC_NAME, definition, cap);
            break;
        case 3: // 3:vorbis
            InitDefinition<FFmpegVorbisDecoderPlugin>(MimeType::AUDIO_VORBIS,
                AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME, definition, cap);
            break;
        case 4: // 4:amrnb
            InitDefinition<FFmpegAmrnbDecoderPlugin>(MimeType::AUDIO_AMR_NB,
                AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME, definition, cap);
            break;
        case 5: // 5:amrwb
            InitDefinition<FFmpegAmrWbDecoderPlugin>(MimeType::AUDIO_AMR_WB,
                AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME, definition, cap);
            break;
        case 6: // 6:ape
            InitDefinition<FFmpegAPEDecoderPlugin>(MimeType::AUDIO_APE,
                AVCodecCodecName::AUDIO_DECODER_APE_NAME, definition, cap);
            break;
        default:
            MEDIA_LOG_I("codec is not supported right now");
    }
}

Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register> &reg)
{
    for (size_t i = 0; i < codecVec.size(); i++) {
        CodecPluginDef definition;
        definition.pluginType = PluginType::AUDIO_DECODER;
        definition.rank = 100; // 100:rank
        Capability cap;
        SetDefinition(i, definition, cap);
        cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);
        definition.AddInCaps(cap);
        // do not delete the codec in the deleter
        if (reg->AddPlugin(definition) != Status::OK) {
            AVCODEC_LOGD("register dec-plugin codecName:%{public}s failed", definition.name.c_str());
        }
        AVCODEC_LOGD("register dec-plugin codecName:%{public}s", definition.name.c_str());
    }
    return Status::OK;
}

void UnRegisterAudioDecoderPlugin() {}

PLUGIN_DEFINITION(FFmpegAudioDecoders, LicenseType::LGPL, RegisterAudioDecoderPlugins, UnRegisterAudioDecoderPlugin);
} // namespace