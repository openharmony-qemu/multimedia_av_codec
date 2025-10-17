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

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "FFmpegDecoderPlugin"};

static const std::vector<std::string_view> codecVec = {
    AVCodecCodecName::AUDIO_DECODER_MP3_NAME,              //  0: mp3
    AVCodecCodecName::AUDIO_DECODER_AAC_NAME,              //  1: aac
    AVCodecCodecName::AUDIO_DECODER_FLAC_NAME,             //  2: flac
    AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME,           //  3: vorbis
    AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME,            //  4: amrnb
    AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME,            //  5: amrwb
    AVCodecCodecName::AUDIO_DECODER_APE_NAME,              //  6: ape
    AVCodecCodecName::AUDIO_DECODER_AC3_NAME,              //  7: ac3
    AVCodecCodecName::AUDIO_DECODER_GSM_NAME,              //  8: gsm
    AVCodecCodecName::AUDIO_DECODER_GSM_MS_NAME,           //  9: gsm_ms
    AVCodecCodecName::AUDIO_DECODER_ADPCM_MS_NAME,         // 10: adpcm_ms
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_QT_NAME,     // 11: adpcm_ima_qt
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_WAV_NAME,    // 12: adpcm_ima_wav
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DK3_NAME,    // 13: adpcm_ima_dk3
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DK4_NAME,    // 14: adpcm_ima_dk4
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_WS_NAME,     // 15: adpcm_ima_ws
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_SMJPEG_NAME, // 16: adpcm_ima_smjpeg
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DAT4_NAME,   // 17: adpcm_ima_dat4
    AVCodecCodecName::AUDIO_DECODER_ADPCM_MTAF_NAME,       // 18: adpcm_mtaf
    AVCodecCodecName::AUDIO_DECODER_ADPCM_ADX_NAME,        // 19: adpcm_adx
    AVCodecCodecName::AUDIO_DECODER_ADPCM_AFC_NAME,        // 20: adpcm_afc
    AVCodecCodecName::AUDIO_DECODER_ADPCM_AICA_NAME,       // 21: adpcm_aica
    AVCodecCodecName::AUDIO_DECODER_ADPCM_CT_NAME,         // 22: adpcm_ct
    AVCodecCodecName::AUDIO_DECODER_ADPCM_DTK_NAME,        // 23: adpcm_dtk
    AVCodecCodecName::AUDIO_DECODER_ADPCM_G722_NAME,       // 24: adpcm_g722
    AVCodecCodecName::AUDIO_DECODER_ADPCM_G726_NAME,       // 25: adpcm_g726
    AVCodecCodecName::AUDIO_DECODER_ADPCM_G726LE_NAME,     // 26: adpcm_g726le
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_AMV_NAME,    // 27: adpcm_ima_amv
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_APC_NAME,    // 28: adpcm_ima_apc
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_ISS_NAME,    // 29: adpcm_ima_iss
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_OKI_NAME,    // 30: adpcm_ima_oki
    AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_RAD_NAME,    // 31: adpcm_ima_rad
    AVCodecCodecName::AUDIO_DECODER_ADPCM_PSX_NAME,        // 32: adpcm_psx
    AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_2_NAME,    // 33: adpcm_sbpro_2
    AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_3_NAME,    // 34: adpcm_sbpro_3
    AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_4_NAME,    // 35: adpcm_sbpro_4
    AVCodecCodecName::AUDIO_DECODER_ADPCM_THP_NAME,        // 36: adpcm_thp
    AVCodecCodecName::AUDIO_DECODER_ADPCM_THP_LE_NAME,     // 37: adpcm_thp_le
    AVCodecCodecName::AUDIO_DECODER_ADPCM_XA_NAME,         // 38: adpcm_xa
    AVCodecCodecName::AUDIO_DECODER_ADPCM_YAMAHA_NAME,     // 39: adpcm_yamaha
    AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME,            // 40: wmav1
    AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME,            // 41: wmav2
    AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME,           // 42: wmapro
	AVCodecCodecName::AUDIO_DECODER_ALAC_NAME			   // 43: alac
};

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
        case 0: // 0: mp3
            InitDefinition<FFmpegMp3DecoderPlugin>(MimeType::AUDIO_MPEG,
                AVCodecCodecName::AUDIO_DECODER_MP3_NAME, definition, cap);
            break;
        case 1: // 1: aac
            InitDefinition<FFmpegAACDecoderPlugin>(MimeType::AUDIO_AAC,
                AVCodecCodecName::AUDIO_DECODER_AAC_NAME, definition, cap);
            break;
        case 2: // 2: flac
            InitDefinition<FFmpegFlacDecoderPlugin>(MimeType::AUDIO_FLAC,
                AVCodecCodecName::AUDIO_DECODER_FLAC_NAME, definition, cap);
            break;
        case 3: // 3: vorbis
            InitDefinition<FFmpegVorbisDecoderPlugin>(MimeType::AUDIO_VORBIS,
                AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME, definition, cap);
            break;
        case 4: // 4: amrnb
            InitDefinition<FFmpegAmrnbDecoderPlugin>(MimeType::AUDIO_AMR_NB,
                AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME, definition, cap);
            break;
        case 5: // 5: amrwb
            InitDefinition<FFmpegAmrWbDecoderPlugin>(MimeType::AUDIO_AMR_WB,
                AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME, definition, cap);
            break;
        case 6: // 6: ape
            InitDefinition<FFmpegAPEDecoderPlugin>(MimeType::AUDIO_APE,
                AVCodecCodecName::AUDIO_DECODER_APE_NAME, definition, cap);
            break;
        case 7: // 7: ac3
            InitDefinition<FFmpegAC3DecoderPlugin>(MimeType::AUDIO_AC3,
                AVCodecCodecName::AUDIO_DECODER_AC3_NAME, definition, cap);
            break;
        case 8: // 8: gsm
            InitDefinition<FFmpegGSMDecoderPlugin>(MimeType::AUDIO_GSM,
                AVCodecCodecName::AUDIO_DECODER_GSM_NAME, definition, cap);
            break;
        case 9: // 9: gsm_ms
            InitDefinition<FFmpegGsmMsDecoderPlugin>(MimeType::AUDIO_GSM_MS,
                AVCodecCodecName::AUDIO_DECODER_GSM_MS_NAME, definition, cap);
            break;
        case 10: // 10: adpcm_ms
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_MS,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_MS_NAME, definition, cap);
            break;
        case 11: // 11: adpcm_ima_qt
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_QT,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_QT_NAME, definition, cap);
            break;
        case 12: // 12: adpcm_ima_wav
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_WAV,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_WAV_NAME, definition, cap);
            break;
        case 13: // 13: adpcm_ima_dk3
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_DK3,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DK3_NAME, definition, cap);
            break;
        case 14: // 14: adpcm_ima_dk4
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_DK4,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DK4_NAME, definition, cap);
            break;
        case 15: // 15: adpcm_ima_ws
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_WS,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_WS_NAME, definition, cap);
            break;
        case 16: // 16: adpcm_ima_smjpeg
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_SMJPEG,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_SMJPEG_NAME, definition, cap);
            break;
        case 17: // 17: adpcm_ima_dat4
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_DAT4,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DAT4_NAME, definition, cap);
            break;
        case 18: // 18: adpcm_mtaf
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_MTAF,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_MTAF_NAME, definition, cap);
            break;
        case 19: // 19: adpcm_adx
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_ADX,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_ADX_NAME, definition, cap);
            break;
        case 20: // 20: adpcm_afc
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_AFC,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_AFC_NAME, definition, cap);
            break;
        case 21: // 21: adpcm_aica
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_AICA,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_AICA_NAME, definition, cap);
            break;
        case 22: // 22: adpcm_ct
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_CT,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_CT_NAME, definition, cap);
            break;
        case 23: // 23: adpcm_dtk
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_DTK,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_DTK_NAME, definition, cap);
            break;
        case 24: // 24: adpcm_g722
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_G722,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_G722_NAME, definition, cap);
            break;
        case 25: // 25: adpcm_g726
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_G726,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_G726_NAME, definition, cap);
            break;
        case 26: // 26: adpcm_g726le
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_G726LE,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_G726LE_NAME, definition, cap);
            break;
        case 27: // 27: adpcm_ima_amv
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_AMV,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_AMV_NAME, definition, cap);
            break;
        case 28: // 28: adpcm_ima_apc
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_APC,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_APC_NAME, definition, cap);
            break;
        case 29: // 29: adpcm_ima_iss
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_ISS,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_ISS_NAME, definition, cap);
            break;
        case 30: // 30: adpcm_ima_oki
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_OKI,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_OKI_NAME, definition, cap);
            break;
        case 31: // 31: adpcm_ima_rad
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_IMA_RAD,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_RAD_NAME, definition, cap);
            break;
        case 32: // 32: adpcm_psx
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_PSX,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_PSX_NAME, definition, cap);
            break;
        case 33: // 33: adpcm_sbpro_2
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_SBPRO_2,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_2_NAME, definition, cap);
            break;
        case 34: // 34: adpcm_sbpro_3
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_SBPRO_3,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_3_NAME, definition, cap);
            break;
        case 35: // 35: adpcm_sbpro_4
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_SBPRO_4,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_4_NAME, definition, cap);
            break;
        case 36: // 36: adpcm_thp
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_THP,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_THP_NAME, definition, cap);
            break;
        case 37: // 37: adpcm_thp_le
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_THP_LE,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_THP_LE_NAME, definition, cap);
            break;
        case 38: // 38: adpcm_xa
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_XA,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_XA_NAME, definition, cap);
            break;
        case 39: // 39: adpcm_yamaha
            InitDefinition<FFmpegADPCMDecoderPlugin>(MimeType::AUDIO_ADPCM_YAMAHA,
                AVCodecCodecName::AUDIO_DECODER_ADPCM_YAMAHA_NAME, definition, cap);
            break;
        case 40: // 40: wmav1
            InitDefinition<FFmpegWMADecoderPlugin>(MimeType::AUDIO_WMAV1,
                AVCodecCodecName::AUDIO_DECODER_WMAV1_NAME, definition, cap);
            break;
        case 41: // 41: wmav2
            InitDefinition<FFmpegWMADecoderPlugin>(MimeType::AUDIO_WMAV2,
                AVCodecCodecName::AUDIO_DECODER_WMAV2_NAME, definition, cap);
            break;
        case 42: // 42: wmapro
            InitDefinition<FFmpegWMADecoderPlugin>(MimeType::AUDIO_WMAPRO,
                AVCodecCodecName::AUDIO_DECODER_WMAPRO_NAME, definition, cap);
            break;
        case 43: // 43:alac
            InitDefinition<FFmpegAlacDecoderPlugin>(MimeType::AUDIO_ALAC,
                AVCodecCodecName::AUDIO_DECODER_ALAC_NAME, definition, cap);
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