
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
#include "audio_ffmpeg_amrwb_decoder_plugin.h"
#include "avcodec_codec_name.h"
#include "plugin_caps_builder.h"
#include "avcodec_dfx.h"
#include "avcodec_log.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugin;
using namespace Ffmpeg;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AvCodec-AudioFFMpegAmrWbDecoderPlugin"};
constexpr int MIN_CHANNELS = 1;
constexpr int MAX_CHANNELS = 1;
constexpr int SUPPORT_SAMPLE_RATE = 1;
constexpr int MIN_OUTBUF_SIZE = 1280;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 150;
int sampleRatePick[SUPPORT_SAMPLE_RATE] = { 16000 };

void UpdateInCaps(CodecPluginDef &definition)
{
    Capability cap;
    cap.SetMime(MimeType::AUDIO_AMR_WB);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);
    definition.AddInCaps(cap);
}

Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register> &reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100;
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioFFmpegAmrWbDecoderPlugin>(name);
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

PLUGIN_DEFINITION(FFmpegAudioAmrWbDecoders, LicenseType::APACHE_V2, RegisterAudioDecoderPlugins,
                  UnRegisterAudioDecoderPlugin);

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
AudioFFmpegAmrWbDecoderPlugin::AudioFFmpegAmrWbDecoderPlugin(std::string name)
    : CodecPlugin(name), channels(0), sampleRate(0), basePlugin(std::make_unique<AudioFfmpegBaseDecoder>())
{
}

AudioFFmpegAmrWbDecoderPlugin::~AudioFFmpegAmrWbDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status AudioFFmpegAmrWbDecoderPlugin::Init()
{
    return Status::OK;
}

Status AudioFFmpegAmrWbDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioFFmpegAmrWbDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status AudioFFmpegAmrWbDecoderPlugin::Start()
{
    return Status::OK;
}

Status AudioFFmpegAmrWbDecoderPlugin::Stop()
{
    return Status::OK;
}

Status AudioFFmpegAmrWbDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = basePlugin->AllocateContext("amrwb");
    Status checkresult = CheckInit(parameter);
    if (checkresult != Status::OK) {
        return checkresult;
    }
    if (ret != Status::OK) {
        AVCODEC_LOGE("amrwb init error.");
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("amrwb init error.");
        return ret;
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    return basePlugin->OpenContext();
}

Status AudioFFmpegAmrWbDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB);
    parameter = format;
    return Status::OK;
}

Status AudioFFmpegAmrWbDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status AudioFFmpegAmrWbDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status AudioFFmpegAmrWbDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status AudioFFmpegAmrWbDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status AudioFFmpegAmrWbDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status AudioFFmpegAmrWbDecoderPlugin::Release()
{
    return basePlugin->Release();
}

Status AudioFFmpegAmrWbDecoderPlugin::CheckInit(const std::shared_ptr<Meta> &format)
{
    format->GetData(Tag::AUDIO_CHANNEL_COUNT, channels);
    format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate);
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS) {
        return Status::ERROR_INVALID_PARAMETER;
    }

    for (int i = 0; i < SUPPORT_SAMPLE_RATE; i++) {
        if (sampleRate == sampleRatePick[i]) {
            break;
        } else if (i == SUPPORT_SAMPLE_RATE - 1) {
            return Status::ERROR_INVALID_PARAMETER;
        }
    }
    return Status::OK;
}

int32_t AudioFFmpegAmrWbDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t AudioFFmpegAmrWbDecoderPlugin::GetOutputBufferSize()
{
    return MIN_OUTBUF_SIZE;
}
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
