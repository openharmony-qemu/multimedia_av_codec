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
    AVCodecCodecName::AUDIO_DECODER_ALAC_NAME,			   // 43: alac
    AVCodecCodecName::AUDIO_DECODER_ILBC_NAME              // 44: ilbc
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

static const std::vector<std::string> codecMimeMap = {
    MimeType::AUDIO_MPEG,             // 0: mp3
    MimeType::AUDIO_AAC,              // 1: aac
    MimeType::AUDIO_FLAC,             // 2: flac
    MimeType::AUDIO_VORBIS,           // 3: vorbis
    MimeType::AUDIO_AMR_NB,           // 4: amrnb
    MimeType::AUDIO_AMR_WB,           // 5: amrwb
    MimeType::AUDIO_APE,              // 6: ape
    MimeType::AUDIO_AC3,              // 7: ac3
    MimeType::AUDIO_GSM,              // 8: gsm
    MimeType::AUDIO_GSM_MS,           // 9: gsm_ms
    MimeType::AUDIO_ADPCM_MS,         // 10: adpcm_ms
    MimeType::AUDIO_ADPCM_IMA_QT,     // 11: adpcm_ima_qt
    MimeType::AUDIO_ADPCM_IMA_WAV,    // 12: adpcm_ima_wav
    MimeType::AUDIO_ADPCM_IMA_DK3,    // 13: adpcm_ima_dk3
    MimeType::AUDIO_ADPCM_IMA_DK4,    // 14: adpcm_ima_dk4
    MimeType::AUDIO_ADPCM_IMA_WS,     // 15: adpcm_ima_ws
    MimeType::AUDIO_ADPCM_IMA_SMJPEG, // 16: adpcm_ima_smjpeg
    MimeType::AUDIO_ADPCM_IMA_DAT4,   // 17: adpcm_ima_dat4
    MimeType::AUDIO_ADPCM_MTAF,       // 18: adpcm_mtaf
    MimeType::AUDIO_ADPCM_ADX,        // 19: adpcm_adx
    MimeType::AUDIO_ADPCM_AFC,        // 20: adpcm_afc
    MimeType::AUDIO_ADPCM_AICA,       // 21: adpcm_aica
    MimeType::AUDIO_ADPCM_CT,         // 22: adpcm_ct
    MimeType::AUDIO_ADPCM_DTK,        // 23: adpcm_dtk
    MimeType::AUDIO_ADPCM_G722,       // 24: adpcm_g722
    MimeType::AUDIO_ADPCM_G726,       // 25: adpcm_g726
    MimeType::AUDIO_ADPCM_G726LE,     // 26: adpcm_g726le
    MimeType::AUDIO_ADPCM_IMA_AMV,    // 27: adpcm_ima_amv
    MimeType::AUDIO_ADPCM_IMA_APC,    // 28: adpcm_ima_apc
    MimeType::AUDIO_ADPCM_IMA_ISS,    // 29: adpcm_ima_iss
    MimeType::AUDIO_ADPCM_IMA_OKI,    // 30: adpcm_ima_oki
    MimeType::AUDIO_ADPCM_IMA_RAD,    // 31: adpcm_ima_rad
    MimeType::AUDIO_ADPCM_PSX,        // 32: adpcm_psx
    MimeType::AUDIO_ADPCM_SBPRO_2,    // 33: adpcm_sbpro_2
    MimeType::AUDIO_ADPCM_SBPRO_3,    // 34: adpcm_sbpro_3
    MimeType::AUDIO_ADPCM_SBPRO_4,    // 35: adpcm_sbpro_4
    MimeType::AUDIO_ADPCM_THP,        // 36: adpcm_thp
    MimeType::AUDIO_ADPCM_THP_LE,     // 37: adpcm_thp_le
    MimeType::AUDIO_ADPCM_XA,         // 38: adpcm_xa
    MimeType::AUDIO_ADPCM_YAMAHA,     // 39: adpcm_yamaha
    MimeType::AUDIO_WMAV1,            // 40: wmav1
    MimeType::AUDIO_WMAV2,            // 41: wmav2
    MimeType::AUDIO_WMAPRO,           // 42: wmapro
    MimeType::AUDIO_ALAC,             // 43: alac
    MimeType::AUDIO_ILBC              // 44: ilbc
};

static const std::vector<void(*)(const std::string&, const std::string_view&,
                                 CodecPluginDef&, Capability&)> codecInitMap = {
    InitDefinition<FFmpegMp3DecoderPlugin>,    // 0: mp3
    InitDefinition<FFmpegAACDecoderPlugin>,    // 1: aac
    InitDefinition<FFmpegFlacDecoderPlugin>,   // 2: flac
    InitDefinition<FFmpegVorbisDecoderPlugin>, // 3: vorbis
    InitDefinition<FFmpegAmrnbDecoderPlugin>,  // 4: amrnb
    InitDefinition<FFmpegAmrWbDecoderPlugin>,  // 5: amrwb
    InitDefinition<FFmpegAPEDecoderPlugin>,    // 6: ape
    InitDefinition<FFmpegAC3DecoderPlugin>,    // 7: ac3
    InitDefinition<FFmpegGSMDecoderPlugin>,    // 8: gsm
    InitDefinition<FFmpegGsmMsDecoderPlugin>,  // 9: gsm_ms
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 10: adpcm_ms
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 11: adpcm_ima_qt
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 12: adpcm_ima_wav
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 13: adpcm_ima_dk3
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 14: adpcm_ima_dk4
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 15: adpcm_ima_ws
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 16: adpcm_ima_smjpeg
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 17: adpcm_ima_dat4
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 18: adpcm_mtaf
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 19: adpcm_adx
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 20: adpcm_afc
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 21: adpcm_aica
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 22: adpcm_ct
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 23: adpcm_dtk
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 24: adpcm_g722
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 25: adpcm_g726
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 26: adpcm_g726le
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 27: adpcm_ima_amv
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 28: adpcm_ima_apc
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 29: adpcm_ima_iss
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 30: adpcm_ima_oki
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 31: adpcm_ima_rad
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 32: adpcm_psx
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 33: adpcm_sbpro_2
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 34: adpcm_sbpro_3
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 35: adpcm_sbpro_4
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 36: adpcm_thp
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 37: adpcm_thp_le
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 38: adpcm_xa
    InitDefinition<FFmpegADPCMDecoderPlugin>,  // 39: adpcm_yamaha
    InitDefinition<FFmpegWMADecoderPlugin>,    // 40: wmav1
    InitDefinition<FFmpegWMADecoderPlugin>,    // 41: wmav2
    InitDefinition<FFmpegWMADecoderPlugin>,    // 42: wmapro
    InitDefinition<FFmpegAlacDecoderPlugin>,   // 43: alac
    InitDefinition<FFmpegILBCDecoderPlugin>    // 44: ilbc
};

void SetDefinition(size_t index, CodecPluginDef &definition, Capability &cap)
{
    if (index >= codecVec.size() || index >= codecMimeMap.size() || index >= codecInitMap.size()) {
        MEDIA_LOG_I("codec is not supported right now");
        return;
    }

    codecInitMap[index](codecMimeMap[index], codecVec[index], definition, cap);
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