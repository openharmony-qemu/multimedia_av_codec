/*
 * Copyright (c) 2025-2025 Huawei Device Co., Ltd.
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

#include "audio_g711a_decoder_plugin.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"
#include "avcodec_common.h"


namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace G711a;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioG711aDecoderPlugin"};
constexpr int MIN_CHANNELS = 1;
constexpr int MIN_SAMPLE_RATE = 8000;
constexpr int INPUT_BUFFER_SIZE_DEFAULT = 8192; // 20ms:160
constexpr int OUTPUT_BUFFER_SIZE_DEFAULT = 16384; // 20ms:320
constexpr float TIME_ONE_SECOND = 1000000.f;
constexpr int AVCODEC_G711A_SHIFT = 3;
constexpr int AVCODEC_G711A_SHIFT_BASE = 2;
constexpr int AVCODEC_G711A_SEG_SHIFT = 4;
constexpr int AVCODEC_G711A_SEG_MASK = 0x70;
constexpr int AVCODEC_G711A_SIGN_BIT = 0x80;
constexpr int AVCODEC_G711A_MAX_INT32 = 0x7fffffff;

Status RegisterAudioDecoderPlugins(const std::shared_ptr<Register>& reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_G711A_NAME);
    definition.pluginType = PluginType::AUDIO_DECODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioG711aDecoderPlugin>(name);
    });

    Capability cap;
    cap.SetMime(MimeType::AUDIO_G711A);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioG711aDecoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioDecoderPlugin() {}

PLUGIN_DEFINITION(G711aAudioDecoder, LicenseType::APACHE_V2, RegisterAudioDecoderPlugins,
    UnRegisterAudioDecoderPlugin);
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace G711a {
AudioG711aDecoderPlugin::AudioG711aDecoderPlugin(const std::string& name)
    : CodecPlugin(std::move(name)),
      decodeBytes_(0),
      channels_(MIN_CHANNELS),
      sampleRate_(MIN_SAMPLE_RATE),
      pts_(0),
      maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT),
      maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT),
      sampleFormat_(AudioSampleFormat::INVALID_WIDTH)
{
}

AudioG711aDecoderPlugin::~AudioG711aDecoderPlugin()
{
}

Status AudioG711aDecoderPlugin::Init()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    decodeInput_.reserve(maxInputSize_);
    return Status::OK;
}

Status AudioG711aDecoderPlugin::Start()
{
    return Status::OK;
}

int16_t AudioG711aDecoderPlugin::G711aLawDecode(uint8_t aLawValue)
{
    uint16_t tmp = 0;
    // sabc wxyz
    aLawValue ^= 0x55;
    uint8_t offsetNum = (aLawValue & AVCODEC_G711A_SEG_MASK) >> AVCODEC_G711A_SEG_SHIFT;  // 4 -> 0x0000 0abc
    uint16_t offsetValue = offsetNum > 0 ? 0x21 : 1;  // 0x21 -> 0010 0001
    offsetNum = offsetNum > 1 ? offsetNum + AVCODEC_G711A_SHIFT_BASE : AVCODEC_G711A_SHIFT;
    offsetValue |= ((aLawValue & 0xf) << 1);  // 000w xyz0 | 0010 0001
    tmp |= offsetValue << offsetNum;
    return ((aLawValue & AVCODEC_G711A_SIGN_BIT) ? static_cast<int16_t>(tmp) : -static_cast<int16_t>(tmp));
}

Status AudioG711aDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    auto memory = inputBuffer->memory_;
    CHECK_AND_RETURN_RET_LOG(memory != nullptr, Status::ERROR_NO_MEMORY, "inputBuffer is nullptr");
    int32_t size = memory->GetSize();
    CHECK_AND_RETURN_RET_LOG(size >= 0, Status::ERROR_UNKNOWN,
        "SendBuffer buffer size < 0. size : %{public}d", size);
    if (size > memory->GetCapacity()) {
        AVCODEC_LOGE("send input buffer > allocate size. size : %{public}d, allocate size : %{public}d",
            size, memory->GetCapacity());
        return Status::ERROR_UNKNOWN;
    }
    if ((size_t)size > decodeInput_.capacity()) {
        AVCODEC_LOGI("g711a size change form %{public}zu to %{public}d", decodeInput_.capacity(), size);
        decodeInput_.reserve(size);
        maxInputSize_ = size;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        decodeBytes_ = size;
        memory->Read(reinterpret_cast<uint8_t *>(decodeInput_.data()), decodeBytes_, 0);
        pts_ = inputBuffer->pts_;
        dataCallback_->OnInputBufferDone(inputBuffer);
    }
    return Status::OK;
}

void AudioG711aDecoderPlugin::SetOutputBasicInfo(std::shared_ptr<AVBuffer> &outputBuffer)
{
    outputBuffer->pts_ = pts_;
    if (dataCallback_ != nullptr) {
        dataCallback_->OnOutputBufferDone(outputBuffer);
    }
}

Status AudioG711aDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    CHECK_AND_RETURN_RET_LOG(outputBuffer != nullptr && outputBuffer->memory_ != nullptr,
        Status::ERROR_INVALID_PARAMETER, "AudioG711aDecoderPlugin Queue out buffer is null.");
    auto memory = outputBuffer->memory_;
    memory->SetSize(0);

    std::lock_guard<std::mutex> lock(avMutex_);
    if (outputBuffer->flag_ == MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) {
        SetOutputBasicInfo(outputBuffer);
        return Status::END_OF_STREAM;
    }
    if (decodeBytes_ > 0) {
        int32_t outSize = static_cast<int32_t>(sizeof(int16_t)) * decodeBytes_;
        CHECK_AND_RETURN_RET_LOG(memory != nullptr && memory->GetCapacity() >= outSize, Status::ERROR_UNKNOWN,
            "memory not enough, capacity:%{public}d, outSize:%{public}d", memory->GetCapacity(), outSize);
        int16_t *decOutputData = reinterpret_cast<int16_t *>(memory->GetAddr());
        CHECK_AND_RETURN_RET_LOG(decOutputData != nullptr, Status::ERROR_NULL_POINTER, "decOutputData is empty");
        uint8_t *decInputData = reinterpret_cast<uint8_t *>(decodeInput_.data());
        for (int32_t i = 0; i < decodeBytes_ ; ++i) {
            decOutputData[i] = G711aLawDecode(decInputData[i]);
        }
        memory->SetSize(outSize);
        if (sampleFormat_ == SAMPLE_S16LE && sampleRate_ > 0 && channels_ > 0) {
            float usPerSample = TIME_ONE_SECOND / sampleRate_;
            outputBuffer->duration_ = static_cast<uint32_t>((outSize / 2.0f / channels_) * usPerSample); // 2 bytes
        }
    }
    SetOutputBasicInfo(outputBuffer);
    decodeBytes_ = 0;
    return Status::OK;
}

Status AudioG711aDecoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    audioParameter_.Clear();
    return Status::OK;
}

Status AudioG711aDecoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711aDecoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711aDecoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    Status ret = Status::OK;

    if (parameter->Find(Tag::AUDIO_CHANNEL_COUNT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_);
    } else {
        AVCODEC_LOGE("AudioG711aDecoderPlugin no AUDIO_CHANNEL_COUNT");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    } else {
        AVCODEC_LOGE("AudioG711aDecoderPlugin no AUDIO_SAMPLE_RATE");
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (channels_ <= 0 || sampleRate_ <= 0) {
        AVCODEC_LOGE("AudioG711aDecoderPlugin not supported channles:%{public}d or sampleRate:%{public}d",
            channels_, sampleRate_);
        ret = Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
        AVCODEC_LOGI("AudioG711aDecoderPlugin SetParameter maxInputSize_: %{public}d", maxInputSize_);
        if (maxInputSize_ < 0 || maxInputSize_ > static_cast<int32_t>(AVCODEC_G711A_MAX_INT32 / sizeof(int16_t))) {
            maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
        }
        maxOutputSize_ = maxInputSize_ * static_cast<int32_t>(sizeof(int16_t));
        decodeInput_.reserve(maxInputSize_);
    }

    if (parameter->Find(Tag::AUDIO_SAMPLE_FORMAT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat_);
    } else {
        AVCODEC_LOGW("AudioG711aDecoderPlugin no AUDIO_SAMPLE_FORMAT");
    }

    audioParameter_ = *parameter;
    return ret;
}

Status AudioG711aDecoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    AVCODEC_LOGD("AudioG711aDecoderPlugin GetParameter maxInputSize_: %{public}d", maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    *parameter = audioParameter_;
    return Status::OK;
}

Status AudioG711aDecoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioG711aDecoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioG711aDecoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>>& inputBuffers)
{
    return Status::OK;
}

Status AudioG711aDecoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>>& outputBuffers)
{
    return Status::OK;
}

}  // namespace G711mu
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS