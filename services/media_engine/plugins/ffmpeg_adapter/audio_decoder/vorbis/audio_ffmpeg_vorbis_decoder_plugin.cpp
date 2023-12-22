
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
#include "audio_ffmpeg_vorbis_decoder_plugin.h"
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

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "AvCodec-AudioFFmpegVorbisDecoderPlugin"};
constexpr uint8_t EXTRADATA_FIRST_CHAR = 2;
constexpr int COMMENT_HEADER_LENGTH = 16;
constexpr int COMMENT_HEADER_PADDING_LENGTH = 8;
constexpr uint8_t COMMENT_HEADER_FIRST_CHAR = '\x3';
constexpr uint8_t COMMENT_HEADER_LAST_CHAR = '\x1';
constexpr std::string_view VORBIS_STRING = "vorbis";
constexpr int NUMBER_PER_BYTES = 255;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 8192;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4 * 1024 * 8;
constexpr AVSampleFormat DEFAULT_FFMPEG_SAMPLE_FORMAT = AV_SAMPLE_FMT_FLT;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 8;
constexpr int32_t MIN_SAMPLE_RATE = 8000;
constexpr int32_t MAX_SAMPLE_RATE = 192000;
static std::vector<OHOS::MediaAVCodec::AudioSampleFormat> supportedSampleFormats = {
    OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE, OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_F32LE};

void UpdateInCaps(CodecPluginDef &definition)
{
    Capability cap;
    cap.SetMime(MimeType::AUDIO_VORBIS);
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
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100; // 100
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioFFmpegVorbisDecoderPlugin>(name);
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

PLUGIN_DEFINITION(FFmpegAudioVorbisDecoders, LicenseType::APACHE_V2, RegisterAudioDecoderPlugins,
                  UnRegisterAudioDecoderPlugin);

namespace OHOS {
namespace Media {
namespace Plugin {
namespace Ffmpeg {
AudioFFmpegVorbisDecoderPlugin::AudioFFmpegVorbisDecoderPlugin(std::string name)
    : CodecPlugin(name), channels_(0), basePlugin(std::make_unique<AudioFfmpegBaseDecoder>())
{
}

AudioFFmpegVorbisDecoderPlugin::~AudioFFmpegVorbisDecoderPlugin()
{
    basePlugin->Release();
    basePlugin.reset();
    basePlugin = nullptr;
}

Status AudioFFmpegVorbisDecoderPlugin::Init()
{
    return Status::OK;
}

Status AudioFFmpegVorbisDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioFFmpegVorbisDecoderPlugin::Reset()
{
    return basePlugin->Reset();
}

Status AudioFFmpegVorbisDecoderPlugin::Start()
{
    return Status::OK;
}

Status AudioFFmpegVorbisDecoderPlugin::Stop()
{
    return Status::OK;
}

Status AudioFFmpegVorbisDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    if (!CheckFormat(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    Status ret = basePlugin->AllocateContext("vorbis");
    if (ret != Status::OK) {
        AVCODEC_LOGE("AllocateContext failed, ret=%{public}d", ret);
        return ret;
    }
    ret = basePlugin->InitContext(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("InitContext failed, ret=%{public}d", ret);
        return ret;
    }
    auto codecCtx = basePlugin->GetCodecContext();
    if (!basePlugin->HasExtraData()) {
        ret = GenExtradata(parameter);
        if (ret != Status::OK) {
            AVCODEC_LOGE("Generate extradata failed, ret=%{public}d", ret);
            return ret;
        }
    }
    auto format = basePlugin->GetFormat();
    format->SetData(Tag::AUDIO_MAX_INPUT_SIZE, GetInputBufferSize());
    format->SetData(Tag::AUDIO_MAX_OUTPUT_SIZE, GetOutputBufferSize());
    format->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VORBIS);
    return basePlugin->OpenContext();
}

Status AudioFFmpegVorbisDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    parameter = basePlugin->GetFormat();
    return Status::OK;
}

Status AudioFFmpegVorbisDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    return basePlugin->ProcessSendData(inputBuffer);
}

Status AudioFFmpegVorbisDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    return basePlugin->ProcessReceiveData(outputBuffer);
}

Status AudioFFmpegVorbisDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status AudioFFmpegVorbisDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status AudioFFmpegVorbisDecoderPlugin::Flush()
{
    return basePlugin->Flush();
}

Status AudioFFmpegVorbisDecoderPlugin::Release()
{
    return basePlugin->Release();
}

bool AudioFFmpegVorbisDecoderPlugin::CheckSampleFormat(const std::shared_ptr<Meta> &format)
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

bool AudioFFmpegVorbisDecoderPlugin::CheckFormat(const std::shared_ptr<Meta> &format)
{
    if (!CheckChannelCount(format) || !CheckSampleFormat(format) || !CheckSampleRate(format)) {
        return false;
    }
    return true;
}

bool AudioFFmpegVorbisDecoderPlugin::CheckChannelCount(const std::shared_ptr<Meta> &format)
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

bool AudioFFmpegVorbisDecoderPlugin::CheckSampleRate(const std::shared_ptr<Meta> &format) const
{
    int32_t sampleRate;
    if (!format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate)) {
        AVCODEC_LOGE("parameter sample_rate missing");
        return false;
    }

    if (sampleRate < MIN_SAMPLE_RATE || sampleRate > MAX_SAMPLE_RATE) {
        AVCODEC_LOGE("parameter sample_rate invaild");
        return false;
    }
    return true;
}

void AudioFFmpegVorbisDecoderPlugin::GetExtradataSize(size_t idSize, size_t setupSize) const
{
    auto codecCtx = basePlugin->GetCodecContext();
    codecCtx->extradata_size = 1 + (1 + idSize / NUMBER_PER_BYTES + idSize) +
                               (1 + COMMENT_HEADER_LENGTH / NUMBER_PER_BYTES + COMMENT_HEADER_LENGTH) + setupSize;
}

int AudioFFmpegVorbisDecoderPlugin::PutHeaderLength(uint8_t *p, size_t value) const
{
    int n = 0;
    while (value >= 0xff) {
        *p++ = 0xff;
        value -= 0xff;
        n++;
    }
    *p = value;
    n++;
    return n;
}

void AudioFFmpegVorbisDecoderPlugin::PutCommentHeader(int offset) const
{
    auto codecCtx = basePlugin->GetCodecContext();
    auto data = codecCtx->extradata;
    int pos = offset;
    data[pos++] = COMMENT_HEADER_FIRST_CHAR;
    for (size_t i = 0; i < VORBIS_STRING.size(); i++) {
        data[pos++] = VORBIS_STRING[i];
    }
    for (int i = 0; i < COMMENT_HEADER_PADDING_LENGTH; i++) {
        data[pos++] = '\0';
    }
    data[pos] = COMMENT_HEADER_LAST_CHAR;
}

Status AudioFFmpegVorbisDecoderPlugin::GenExtradata(const std::shared_ptr<Meta> &format) const
{
    AVCODEC_LOGD("GenExtradata start");
    std::vector<uint8_t> idHeader;
    if (!format->GetData(Tag::AUDIO_VORBIS_IDENTIFICATION_HEADER, idHeader)) {
        AVCODEC_LOGE("identification header not available.");
        return Status::ERROR_INVALID_DATA;
    }
    std::vector<uint8_t> setupHeader;
    if (!format->GetData(Tag::AUDIO_VORBIS_SETUP_HEADER, setupHeader)) {
        AVCODEC_LOGE("setup header not available.");
        return Status::ERROR_INVALID_DATA;
    }

    GetExtradataSize(idHeader.size(), setupHeader.size());
    auto codecCtx = basePlugin->GetCodecContext();
    codecCtx->extradata = static_cast<uint8_t *>(av_mallocz(codecCtx->extradata_size + AV_INPUT_BUFFER_PADDING_SIZE));

    int offset = 0;
    codecCtx->extradata[0] = EXTRADATA_FIRST_CHAR;
    offset = 1;

    // put identification header size and comment header size
    offset += PutHeaderLength(codecCtx->extradata + offset, idHeader.size());
    codecCtx->extradata[offset++] = COMMENT_HEADER_LENGTH;

    // put identification header
    int ret =
        memcpy_s(codecCtx->extradata + offset, codecCtx->extradata_size - offset, idHeader.data(), idHeader.size());
    if (ret != 0) {
        AVCODEC_LOGE("memory copy failed: %{public}d", ret);
        return Status::ERROR_UNKNOWN;
    }
    offset += idHeader.size();

    // put comment header
    PutCommentHeader(offset);
    offset += COMMENT_HEADER_LENGTH;

    // put setup header
    ret = memcpy_s(codecCtx->extradata + offset, codecCtx->extradata_size - offset, setupHeader.data(),
                   setupHeader.size());
    if (ret != 0) {
        AVCODEC_LOGE("memory copy failed: %{public}d", ret);
        return Status::ERROR_UNKNOWN;
    }
    offset += setupHeader.size();

    if (offset != codecCtx->extradata_size) {
        AVCODEC_LOGW("extradata write length mismatch extradata size");
    }

    return Status::OK;
}

int32_t AudioFFmpegVorbisDecoderPlugin::GetInputBufferSize()
{
    int32_t maxSize = basePlugin->GetMaxInputSize();
    if (maxSize < 0 || maxSize > INPUT_BUFFER_SIZE_DEFAULT) {
        maxSize = INPUT_BUFFER_SIZE_DEFAULT;
    }
    return maxSize;
}

int32_t AudioFFmpegVorbisDecoderPlugin::GetOutputBufferSize()
{
    return OUTPUT_BUFFER_SIZE_DEFAULT;
}
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS