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
#include "audio_raw_decoder_plugin.h"
#include "avcodec_log.h"
#include "avcodec_codec_name.h"
#include "avcodec_info.h"
#include "mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Audio;
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioRawDecoderPlugin"};
Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register> &reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_RAW_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string &name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioRawDecoderPlugin>(name);
    });

    Capability cap;
    cap.SetMime(MimeType::AUDIO_RAW);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioRawDecoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioDecoderPlugin() {}

PLUGIN_DEFINITION(RawAudioDecoder, LicenseType::APACHE_V2, RegisterAudioDecoderPlugins,
    UnRegisterAudioDecoderPlugin);
}
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Audio {
constexpr int32_t AUDIO_FRAME_LENGHT_DEFAULT = 1024;
constexpr uint32_t BUFFER_FLAG_EOS = 1;
constexpr float TIME_SPAN_MILLSECOND = 1000000.f;
constexpr int32_t BYTE_LENGHT_S16 = 2;
constexpr int32_t BYTE_LENGHT_S24 = 3;
constexpr int32_t BYTE_LENGHT_S32_F32 = 4;
constexpr int32_t BYTE_LENGHT_DOUBLE = 8;
constexpr int32_t BYTE_LENGHT_DOUBLE_INDEX = 7;
constexpr int32_t MAX_CHANNELS = 255;
constexpr int32_t MAX_INPUT_SIZE_LIMIT = 80 * 1024 * 1024;  // 80 MB

static std::vector<AudioSampleFormat> supportedSampleFormats = {
    AudioSampleFormat::SAMPLE_S16BE,
    AudioSampleFormat::SAMPLE_S24BE,
    AudioSampleFormat::SAMPLE_S32BE,
    AudioSampleFormat::SAMPLE_F32BE,
    AudioSampleFormat::SAMPLE_F64BE
};

static std::unordered_map<AudioSampleFormat, AudioSampleFormat> formatMap = {
    {AudioSampleFormat::SAMPLE_S16BE, AudioSampleFormat::SAMPLE_S16LE},
    {AudioSampleFormat::SAMPLE_S24BE, AudioSampleFormat::SAMPLE_S24LE},
    {AudioSampleFormat::SAMPLE_S32BE, AudioSampleFormat::SAMPLE_S32LE},
    {AudioSampleFormat::SAMPLE_F32BE, AudioSampleFormat::SAMPLE_F32LE},
    {AudioSampleFormat::SAMPLE_F64BE, AudioSampleFormat::SAMPLE_F32LE},
};

AudioRawDecoderPlugin::AudioRawDecoderPlugin(const std::string &name)
    : CodecPlugin(std::move(name)), audioSampleFormat_(AudioSampleFormat::INVALID_WIDTH),
      srcSampleFormat_(AudioSampleFormat::INVALID_WIDTH), pts_(0), channels_(0), sampleRate_(0), offset_(0),
      frameSize_(0), maxInputSize_(0), maxOutputSize_(0), durationTime_(0), eosFlag_(false), dataCallback_(nullptr),
      format_(nullptr), inputBuffer_(0)
{
    AVCODEC_LOGI("AudioRawDecoderPlugin init");
}

AudioRawDecoderPlugin::~AudioRawDecoderPlugin()
{
    if (inputBuffer_.size() > 0) {
        inputBuffer_.clear();
    }
}

Status AudioRawDecoderPlugin::Init()
{
    return Status::OK;
}

Status AudioRawDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioRawDecoderPlugin::Reset()
{
    if (inputBuffer_.size() > 0) {
        inputBuffer_.clear();
    }
    return Status::OK;
}

Status AudioRawDecoderPlugin::Flush()
{
    if (inputBuffer_.size() > 0) {
        inputBuffer_.clear();
    }
    return Status::OK;
}

Status AudioRawDecoderPlugin::Start()
{
    eosFlag_ = false;
    return Status::OK;
}

Status AudioRawDecoderPlugin::Stop()
{
    if (inputBuffer_.size() > 0) {
        inputBuffer_.clear();
    }
    return Status::OK;
}

Status AudioRawDecoderPlugin::Release()
{
    if (inputBuffer_.size() > 0) {
        inputBuffer_.clear();
    }
    return Status::OK;
}

Status AudioRawDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    Status ret = GetMetaData(parameter);
    if (ret != Status::OK) {
        AVCODEC_LOGE("Raw GetMetaData Fail");
        return ret;
    }

    if (!CheckFormat()) {
        AVCODEC_LOGE("Raw CheckFormat Fail");
        return Status::ERROR_INVALID_PARAMETER;
    }
    durationTime_ = TIME_SPAN_MILLSECOND / sampleRate_;
    if (format_ == nullptr) {
        format_ = std::make_shared<Meta>();
    }
    *format_ = *parameter;
    int32_t bytesSize = GetFormatBytes(srcSampleFormat_);
    int32_t desBytesSize = GetFormatBytes(audioSampleFormat_);
    if (format_->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_)) {
        if (maxInputSize_ > MAX_INPUT_SIZE_LIMIT) {
            AVCODEC_LOGE(
                "maxInputSize:%{public}d is over %{public}d,not supported", maxInputSize_, MAX_INPUT_SIZE_LIMIT);
            return Status::ERROR_INVALID_PARAMETER;
        }

        if (maxInputSize_ < channels_ * AUDIO_FRAME_LENGHT_DEFAULT * bytesSize) {
            maxInputSize_ = channels_ * AUDIO_FRAME_LENGHT_DEFAULT * bytesSize;
        }
    } else {
        maxInputSize_ = channels_ * AUDIO_FRAME_LENGHT_DEFAULT * bytesSize;
    }
    maxOutputSize_ = static_cast<uint32_t>(channels_ * AUDIO_FRAME_LENGHT_DEFAULT * desBytesSize);
    inputBuffer_.resize(maxInputSize_);
    AVCODEC_LOGI("input size:%{public}d, output size:%{public}d, format:%{public}d,dest format:%{public}d",
        maxInputSize_, maxOutputSize_, srcSampleFormat_, audioSampleFormat_);
    format_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(static_cast<int32_t>(maxInputSize_));
    format_->Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(static_cast<int32_t>(maxOutputSize_));
    return Status::OK;
}

int32_t AudioRawDecoderPlugin::GetFormatBytes(AudioSampleFormat format)
{
    int32_t bytesSize = BYTE_LENGHT_S16;
    switch (format) {
        case AudioSampleFormat::SAMPLE_S16BE:
        case AudioSampleFormat::SAMPLE_S16LE:
            bytesSize = BYTE_LENGHT_S16;
            break;
        case AudioSampleFormat::SAMPLE_S24BE:
        case AudioSampleFormat::SAMPLE_S24LE:
            bytesSize = BYTE_LENGHT_S24;
            break;
        case AudioSampleFormat::SAMPLE_S32BE:
        case AudioSampleFormat::SAMPLE_S32LE:
            bytesSize = BYTE_LENGHT_S32_F32;
            break;
        case AudioSampleFormat::SAMPLE_F32BE:
        case AudioSampleFormat::SAMPLE_F32LE:
            bytesSize = BYTE_LENGHT_S32_F32;
            break;
        case AudioSampleFormat::SAMPLE_F64BE:
            bytesSize = BYTE_LENGHT_DOUBLE;
            break;
        default:
            break;
    }
    return bytesSize;
}

Status AudioRawDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    if (format_ == nullptr) {
        AVCODEC_LOGW("format is not initialize");
        return Status::OK;
    }
    format_->SetData(Tag::MIME_TYPE, MediaAVCodec::CodecMimeType::AUDIO_RAW);
    parameter = format_;
    return Status::OK;
}

double AudioRawDecoderPlugin::F64BEToDouble(const uint8_t *src)
{
    double val = 0.0;
    uint64_t value = 0;
    if (src == nullptr) {
        AVCODEC_LOGE("src is null");
        return val;
    }
    for (size_t i = 0; i < BYTE_LENGHT_DOUBLE; i++) {
        value |= (static_cast<uint64_t>(src[i]) << (BYTE_LENGHT_DOUBLE * (BYTE_LENGHT_DOUBLE_INDEX - i)));
    }
    if (memcpy_s(&val, sizeof(double), &value, sizeof(double)) != EOK) {
        AVCODEC_LOGE("memcpy_s failed");
    }
    return val;
}

Status AudioRawDecoderPlugin::F64BEToF32LE(const uint8_t *src, float *des)
{
    double val = F64BEToDouble(src);
    *des = static_cast<float>(val);
    return Status::OK;
}

Status AudioRawDecoderPlugin::ConvertF64BEToF32LE(const uint8_t *ptr, int32_t &size)
{
    constexpr int32_t bytesF64BESize = sizeof(double);
    constexpr int32_t bytesF32LESize = sizeof(float);
    float *inputData = reinterpret_cast<float *>(inputBuffer_.data());
    for (int32_t i = 0; i < size / bytesF64BESize; i++) {
        F64BEToF32LE(ptr + i * bytesF64BESize, inputData + i);
    }
    size /= (bytesF64BESize / bytesF32LESize);
    return Status::OK;
}

Status AudioRawDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    if (inputBuffer->flag_ == BUFFER_FLAG_EOS) {
        eosFlag_ = true;
        return Status::OK;
    }

    auto bytesSize = GetFormatBytes(srcSampleFormat_);
    auto memory = inputBuffer->memory_;
    int32_t size = memory->GetSize();
    uint8_t *ptr = memory->GetAddr();
    if (size % bytesSize != 0) {
        AVCODEC_LOGE("size:%{public}d is invalid, bytesSize:%{public}d", size, bytesSize);
        return Status::ERROR_UNKNOWN;
    }
    if (size > maxInputSize_) {
        AVCODEC_LOGI("size changed from :%{public}d to %{public}d", maxInputSize_, size);
        inputBuffer_.resize(size);
        maxInputSize_ = size;
    }
    if (srcSampleFormat_ == AudioSampleFormat::SAMPLE_F64BE) {
        ConvertF64BEToF32LE(ptr, size);
    } else {
        for (int32_t i = 0; i < size / bytesSize; i++) {
            for (int32_t j = 0; j < bytesSize; j++) {
                inputBuffer_[i * bytesSize + bytesSize - j - 1] = ptr[i * bytesSize + j];
            }
        }
    }
    frameSize_ = size;
    offset_ = 0;
    pts_ = inputBuffer->pts_;
    dataCallback_->OnInputBufferDone(inputBuffer);
    return Status::OK;
}

Status AudioRawDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    if (eosFlag_) {
        outputBuffer->flag_ = BUFFER_FLAG_EOS;
        outputBuffer->memory_->SetSize(0);
        SetOutputBasicInfo(outputBuffer);
        return Status::END_OF_STREAM;
    }
    int32_t bytesSize = GetFormatBytes(audioSampleFormat_);
    int32_t size = 0;
    if (frameSize_ / (bytesSize * channels_) >= AUDIO_FRAME_LENGHT_DEFAULT) {
        size = bytesSize * channels_ * AUDIO_FRAME_LENGHT_DEFAULT;
        auto memory = outputBuffer->memory_;
        memory->Write(inputBuffer_.data() + offset_, size, 0);
        offset_ += size;
        frameSize_ -= size;
    } else if (frameSize_ > 0) {
        size = frameSize_;
        AVCODEC_LOGE("frame size:%{public}d", frameSize_);
        auto memory = outputBuffer->memory_;
        memory->Write(inputBuffer_.data() + offset_, frameSize_, 0);
        frameSize_ = 0;
        offset_ += size;
    } else {
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    outputBuffer->duration_ = durationTime_ * (size / (bytesSize * channels_));
    SetOutputBasicInfo(outputBuffer);
    pts_ += durationTime_ * (size / (bytesSize * channels_));
    if (frameSize_ > 0) {
        return Status::ERROR_AGAIN;
    }
    return Status::OK;
}

Status AudioRawDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status AudioRawDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status AudioRawDecoderPlugin::GetMetaData(const std::shared_ptr<Meta> &meta)
{
    if (!meta->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_)) {
        AVCODEC_LOGE("AudioRawDecoderPlugin no AUDIO_CHANNEL_COUNT");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!meta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_)) {
        AVCODEC_LOGE("AudioRawDecoderPlugin no AUDIO_SAMPLE_RATE");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!meta->Get<Tag::AUDIO_SAMPLE_FORMAT>(audioSampleFormat_)) {
        AVCODEC_LOGE("AudioRawDecoderPlugin no AUDIO_SAMPLE_FORMAT");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!meta->Get<Tag::AUDIO_RAW_SAMPLE_FORMAT>(srcSampleFormat_)) {
        AVCODEC_LOGE("AudioRawDecoderPlugin no AUDIO_RAW_SAMPLE_FORMAT");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

bool AudioRawDecoderPlugin::CheckFormat()
{
    if (channels_ <= 0 || channels_ > MAX_CHANNELS) {
        AVCODEC_LOGE("channels:%{public}d not supported", channels_);
        return false;
    }

    if (sampleRate_ <= 0) {
        AVCODEC_LOGE("sampleRate :%{public}d not supported", sampleRate_);
        return false;
    }

    if (std::find(supportedSampleFormats.begin(), supportedSampleFormats.end(), srcSampleFormat_) ==
        supportedSampleFormats.end()) {
        AVCODEC_LOGE("sample format:%{public}d not supported", srcSampleFormat_);
        return false;
    }

    if (formatMap.find(srcSampleFormat_) == formatMap.end()) {
        AVCODEC_LOGE("sample format:%{public}d not in map", srcSampleFormat_);
        return false;
    }

    if (formatMap[srcSampleFormat_] != audioSampleFormat_) {
        AVCODEC_LOGE("format:%{public}d --> %{public}d not supported", srcSampleFormat_, audioSampleFormat_);
        return false;
    }
    return true;
}

void AudioRawDecoderPlugin::SetOutputBasicInfo(std::shared_ptr<AVBuffer> &outputBuffer)
{
    outputBuffer->pts_ = pts_;
    if (dataCallback_ != nullptr) {
        dataCallback_->OnOutputBufferDone(outputBuffer);
    }
}
} // namespace OHOS
} // namespace Media
} // namespace Plugins
} // namespace Audio