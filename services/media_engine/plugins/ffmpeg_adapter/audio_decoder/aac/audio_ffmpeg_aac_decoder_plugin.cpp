
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
#include "audio_ffmpeg_aac_decoder_plugin.h"
#include "avcodec_codec_name.h"
#include "plugin_caps_builder.h"
#include "avcodec_dfx.h"
#include "avcodec_log.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"
#include "demuxer/ffmpeg_converter.h"
#include "avcodec_audio_common.h"
#include <algorithm>
namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugin;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AvCodec-AudioFFmpegAACDecoderPlugin"};
constexpr int MIN_CHANNELS = 1;
constexpr int MAX_CHANNELS = 8;
constexpr AVSampleFormat DEFAULT_FFMPEG_SAMPLE_FORMAT = AV_SAMPLE_FMT_FLT;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
static std::vector<int32_t> supportedSampleRate = {96000, 88200, 64000, 48000, 44100, 32000, 24000,
                                                   22050, 16000, 12000, 11025, 8000,  7350};
static std::vector<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE, OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE};
static std::vector<int32_t> supportedSampleRates = {7350,  8000,  11025, 12000, 16000, 22050, 24000,
                                                    32000, 44100, 48000, 64000, 88200, 96000};
void UpdateInCaps(CodecPluginDef &definition)
{
    Capability cap;
    cap.SetMime(MimeType::AUDIO_AAC);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    // if (codec->supported_samplerates != nullptr) {
    //     DiscreteCapability<uint32_t> values;
    //     size_t index{0};
    //     for (; index < SUPPORT_SAMPLE_RATE; ++index) {
    //         values.push_back(sampleRatePick[index]);
    //     }
    //     if (index) {
    //         //    cap.SetAudioSampleRateList(values);
    //     }
    // }
    // todo:audio channel layout
    definition.AddInCaps(cap);
}

Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register> &reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_AAC_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100; // 100
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioFFmpegAACDecoderPlugin>(name);
    });

    UpdateInCaps(definition);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        std::cout << "register plugin " << definition.name.c_str() << " failed" << std::endl;
    }
    return Status::OK;
}

void UnRegisterAudioDecoderPlugin() {}
} // namespace

PLUGIN_DEFINITION(FFmpegAudioAACDecoders, LicenseType::APACHE_V2, RegisterAudioDecoderPlugins,
                  UnRegisterAudioDecoderPlugin);

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
AudioFFmpegAACDecoderPlugin::AudioFFmpegAACDecoderPlugin(std::string name)
    : CodecPlugin(name), channels_(0), basePlugin(std::make_unique<AudioFfmpegBaseDecoder>())
{
}

AudioFFmpegAACDecoderPlugin::~AudioFFmpegAACDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status AudioFFmpegAACDecoderPlugin::Init()
{
    return Status::OK;
}

Status AudioFFmpegAACDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioFFmpegAACDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status AudioFFmpegAACDecoderPlugin::Start()
{
    return Status::OK;
}

Status AudioFFmpegAACDecoderPlugin::Stop()
{
    return Status::OK;
}

Status AudioFFmpegAACDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    if (!CheckFormat(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    Status ret = basePlugin->AllocateContext(aacName_);
    if (ret != Status::OK) {
        AVCODEC_LOGE("AllocateContext failed, ret=%{public}d", ret);
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("InitContext failed, ret=%{public}d", ret);
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC);
    return basePlugin->OpenContext();
}

Status AudioFFmpegAACDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status AudioFFmpegAACDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status AudioFFmpegAACDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status AudioFFmpegAACDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status AudioFFmpegAACDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status AudioFFmpegAACDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status AudioFFmpegAACDecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool AudioFFmpegAACDecoderPlugin::CheckAdts(const std::shared_ptr<Meta> &format)
{
    int type;
    if (format->GetData(Tag::AUDIO_AAC_IS_ADTS, type)) {
        if (type != 1 && type != 0) {
            AVCODEC_LOGE("aac_is_adts value invalid");
            return false;
        }
    } else {
        AVCODEC_LOGW("aac_is_adts parameter is missing");
        type = 1;
    }
    aacName_ = (type == 1 ? "aac" : "aac_latm");

    return true;
}

bool AudioFFmpegAACDecoderPlugin::CheckSampleFormat(const std::shared_ptr<Meta> &format)
{
    int32_t sampleFormat;
    if (!format->GetData(Tag::AUDIO_SAMPLE_FORMAT, sampleFormat)) {
        AVCODEC_LOGW("Sample format missing, set to default f32le");
        if (channels_ != 1) {
            basePlugin->EnableResample(DEFAULT_FFMPEG_SAMPLE_FORMAT);
        }
        return true;
    }

    if (std::find(supportedSampleFormats.begin(), supportedSampleFormats.end(),
                  static_cast<OHOS::MediaAVCodec::AudioSampleFormat>(sampleFormat)) == supportedSampleFormats.end()) {
        AVCODEC_LOGE("Output sample format not support");
        return false;
    }
    if (channels_ == 1 && sampleFormat == AudioSampleFormat::SAMPLE_F32LE) {
        return true;
    }
    auto destFmt = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(static_cast<AudioSampleFormat>(sampleFormat));
    if (destFmt == AV_SAMPLE_FMT_NONE) {
        AVCODEC_LOGE("Convert format failed, avSampleFormat not found");
        return false;
    }
    basePlugin->EnableResample(destFmt);
    return true;
}

bool AudioFFmpegAACDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    if (!CheckAdts(format) || !CheckChannelCount(format) || !CheckSampleFormat(format) || !CheckSampleRate(format)) {
        return false;
    }
    return true;
}

bool AudioFFmpegAACDecoderPlugin::CheckChannelCount(const std::shared_ptr<Meta> &format)
{
    if (!format->GetData(Tag::AUDIO_CHANNEL_COUNT, channels_)) {
        AVCODEC_LOGE("parameter channel_count missing");
        return false;
    }
    if (channels_ < MIN_CHANNELS || channels_ > MAX_CHANNELS) {
        AVCODEC_LOGE("parameter channel_count invaild");
        return false;
    }
    return true;
}

bool AudioFFmpegAACDecoderPlugin::CheckSampleRate(const std::shared_ptr<Meta> &format) const
{
    int32_t sampleRate;
    if (!format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate)) {
        AVCODEC_LOGE("parameter sample_rate missing");
        return false;
    }

    if (std::find(supportedSampleRates.begin(), supportedSampleRates.end(), sampleRate) == supportedSampleRates.end()) {
        AVCODEC_LOGE("parameter sample_rate invalid, %{public}d", sampleRate);
        return false;
    }
    return true;
}

int32_t AudioFFmpegAACDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t AudioFFmpegAACDecoderPlugin::GetOutputBufferSize()
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS