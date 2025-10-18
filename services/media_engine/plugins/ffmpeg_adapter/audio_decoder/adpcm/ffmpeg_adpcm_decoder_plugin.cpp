/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "ffmpeg_adpcm_decoder_plugin.h"
#include <algorithm>
#include <unordered_map>
#include "avcodec_log.h"
#include "avcodec_codec_name.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_audio_common.h"
#include "meta/mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::Ffmpeg;
using namespace OHOS::MediaAVCodec;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {
    LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-FFmpegADPCMDecoderPlugin"
};

constexpr int MIN_CHANNELS = 1;
constexpr int MAX_CHANNELS = 8;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT  = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 2 * 4 * 2048;

static const std::unordered_map<std::string_view, const char*> kAdpcmName2Ff = {
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_MS_NAME,         "adpcm_ms"         },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_QT_NAME,     "adpcm_ima_qt"     },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_WAV_NAME,    "adpcm_ima_wav"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DK3_NAME,    "adpcm_ima_dk3"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DK4_NAME,    "adpcm_ima_dk4"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_WS_NAME,     "adpcm_ima_ws"     },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_SMJPEG_NAME, "adpcm_ima_smjpeg" },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_DAT4_NAME,   "adpcm_ima_dat4"   },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_MTAF_NAME,       "adpcm_mtaf"       },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_ADX_NAME,        "adpcm_adx"        },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_AFC_NAME,        "adpcm_afc"        },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_AICA_NAME,       "adpcm_aica"       },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_CT_NAME,         "adpcm_ct"         },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_DTK_NAME,        "adpcm_dtk"        },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_G722_NAME,       "adpcm_g722"       },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_G726_NAME,       "adpcm_g726"       },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_G726LE_NAME,     "adpcm_g726le"     },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_AMV_NAME,    "adpcm_ima_amv"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_APC_NAME,    "adpcm_ima_apc"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_ISS_NAME,    "adpcm_ima_iss"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_OKI_NAME,    "adpcm_ima_oki"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_IMA_RAD_NAME,    "adpcm_ima_rad"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_PSX_NAME,        "adpcm_psx"        },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_2_NAME,    "adpcm_sbpro_2"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_3_NAME,    "adpcm_sbpro_3"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_SBPRO_4_NAME,    "adpcm_sbpro_4"    },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_THP_NAME,        "adpcm_thp"        },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_THP_LE_NAME,     "adpcm_thp_le"     },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_XA_NAME,         "adpcm_xa"         },
    { AVCodecCodecName::AUDIO_DECODER_ADPCM_YAMAHA_NAME,     "adpcm_yamaha"     },
};
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {

FFmpegADPCMDecoderPlugin::FFmpegADPCMDecoderPlugin(const std::string& name)
    : CodecPlugin(name), base_(std::make_unique<FfmpegBaseDecoder>())
{
    auto it = kAdpcmName2Ff.find(name);
    if (it == kAdpcmName2Ff.end()) {
        AVCODEC_LOGE("unsupported name for ADPCM: %{public}s", name.c_str());
    }
    ffCodecName_ = it->second;
    AVCODEC_LOGI("create adpcm decoder plugin by %{public}s", name.c_str());
}

FFmpegADPCMDecoderPlugin::~FFmpegADPCMDecoderPlugin()
{
    base_->Release();
    base_.reset();
}

Status FFmpegADPCMDecoderPlugin::Init()
{
    return Status::OK;
}
Status FFmpegADPCMDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status FFmpegADPCMDecoderPlugin::Start()
{
    return Status::OK;
}

Status FFmpegADPCMDecoderPlugin::Stop()
{
    return Status::OK;
}

Status FFmpegADPCMDecoderPlugin::Reset()
{
    return base_->Reset();
}

Status FFmpegADPCMDecoderPlugin::Flush()
{
    return base_->Flush();
}

Status FFmpegADPCMDecoderPlugin::Release()
{
    return base_->Release();
}

Status FFmpegADPCMDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &meta)
{
    if (!CheckFormat(meta)) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    auto ret = base_->AllocateContext(ffCodecName_);
    if (ret != Status::OK) {
        AVCODEC_LOGE("AllocateContext failed, ret=%{public}d", ret);
        return ret;
    }
    if (!CheckSampleFormat(meta)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    ret = base_->InitContext(meta);
    if (ret != Status::OK) {
        AVCODEC_LOGE("InitContext failed, ret=%{public}d", ret);
        return ret;
    }

    auto fmt = base_->GetFormat();
    fmt->SetData(Tag::AUDIO_MAX_INPUT_SIZE,  GetInputBufferSize());
    fmt->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());

    // optional: echo MIME back into format for consistency
    std::string mime;
    if (meta->GetData(Tag::MIME_TYPE, mime)) {
        fmt->SetData(Tag::MIME_TYPE, mime);
    }
    int32_t blockAlign = 1;
    if (meta->GetData(Tag::AUDIO_BLOCK_ALIGN, blockAlign)) {
        base_->SetBlockAlignContext(blockAlign);
        AVCODEC_LOGI("set bolck align1:%{public}d", blockAlign);
    } else {
        AVCODEC_LOGW("set block align error");
    }

    int32_t bitsPerSample = 1;
    if (meta->GetData(Tag::AUDIO_BITS_PER_CODED_SAMPLE, bitsPerSample)) {
        base_->SetBitsPerSampleContext(bitsPerSample);
        AVCODEC_LOGI("set bolck align1:%{public}d", bitsPerSample);
    } else {
        AVCODEC_LOGW("set block align error");
    }

    return base_->OpenContext();
}

Status FFmpegADPCMDecoderPlugin::GetParameter(std::shared_ptr<Meta> &meta)
{
    meta = base_->GetFormat();
    return Status::OK;
}

Status FFmpegADPCMDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &in)
{
    return base_->ProcessSendData(in);
}

Status FFmpegADPCMDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &out)
{
    return base_->ProcessReceiveData(out);
}

Status FFmpegADPCMDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &)
{
    return Status::OK;
}

Status FFmpegADPCMDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &)
{
    return Status::OK;
}

bool FFmpegADPCMDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &meta)
{
    return CheckChannelCount(meta) && CheckSampleRate(meta);
}

bool FFmpegADPCMDecoderPlugin::CheckChannelCount(const std::shared_ptr<Meta> &meta)
{
    if (!meta->GetData(Tag::AUDIO_CHANNEL_COUNT, channels_)) {
        AVCODEC_LOGE("parameter channel_count missing");
        return false;
    }
    if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
        AVCODEC_LOGE("parameter channel_count invalid: %{public}d", channels_);
        return false;
    }
    return true;
}

bool FFmpegADPCMDecoderPlugin::CheckSampleRate(const std::shared_ptr<Meta> &meta) const
{
    int32_t sr = 0;
    if (!meta->GetData(Tag::AUDIO_SAMPLE_RATE, sr) || sr <= 0) {
        AVCODEC_LOGE("parameter sample_rate missing/invalid");
        return false;
    }
    return true;
}

bool FFmpegADPCMDecoderPlugin::CheckSampleFormat(const std::shared_ptr<Meta> &meta)
{
    return base_->CheckSampleFormat(meta, channels_);
}

int32_t FFmpegADPCMDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = base_->GetMaxInputSize();
    constexpr int32_t MAX_INPUT_SIZE = 1 << 20;
    if (maxSize > 0 && maxSize <= MAX_INPUT_SIZE) {
        return maxSize;
    }
    return INPUT_BUFFER_SIZE_DEFAULT;
}


int32_t FFmpegADPCMDecoderPlugin::GetOutputBufferSize()
{
    int32_t inMax = base_->GetMaxInputSize();
    const int32_t MIN_INPUT_SIZE = 1;
    const int32_t MAX_INPUT_SIZE = 1 << 20;
    const int32_t MAX_OUTPUT_SIZE = 1 << 22;

    if (inMax >= MIN_INPUT_SIZE && inMax < MAX_INPUT_SIZE) {
        int64_t outMax = static_cast<int64_t>(inMax) * 4;
        if (outMax >= MIN_INPUT_SIZE && outMax < MAX_OUTPUT_SIZE) {
            return static_cast<int32_t>(outMax);
        }
    }
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS