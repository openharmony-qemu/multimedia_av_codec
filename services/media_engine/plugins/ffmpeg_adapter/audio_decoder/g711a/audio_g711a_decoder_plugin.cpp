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
    decodeResult_.reserve(OUTPUT_BUFFER_SIZE_DEFAULT);
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
    uint8_t offsetNum = (aLawValue & 0x70) >> 4;  // 4 -> abc
    uint16_t offsetValue = offsetNum > 0 ? 0x21 : 1;  // 0x21 -> 0010 0001
    offsetNum = offsetNum > 1 ? offsetNum + 2 : 3;
    offsetValue |= ((aLawValue & 0xf) << 1);  // 000w xyz0 | offsetValue
    tmp |= offsetValue << offsetNum;
    return ((aLawValue & 0x80) ? static_cast<int16_t>(tmp) : -static_cast<int16_t>(tmp));
}

Status AudioG711aDecoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    auto memory = inputBuffer->memory_;
    CHECK_AND_RETURN_RET_LOG(memory->GetSize() >= 0, Status::ERROR_UNKNOWN,
        "SendBuffer buffer size < 0. size : %{public}d", memory->GetSize());
    if (memory->GetSize() > memory->GetCapacity()) {
        AVCODEC_LOGE("send input buffer > allocate size. size : %{public}d, allocate size : %{public}d",
            memory->GetSize(), memory->GetCapacity());
        return Status::ERROR_UNKNOWN;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        int32_t decodeNum = memory->GetSize();
        uint8_t *aValueToDecode = reinterpret_cast<uint8_t *>(memory->GetAddr());
        CHECK_AND_RETURN_RET_LOG(aValueToDecode != nullptr, Status::ERROR_NULL_POINTER, "aValueToDecode is empty");
        decodeResult_.clear();
        for (int32_t i = 0; i < decodeNum ; ++i) {
            decodeResult_.push_back(G711aLawDecode(aValueToDecode[i]));
        }
        pts_ = inputBuffer->pts_;
        dataCallback_->OnInputBufferDone(inputBuffer);
    }
    return Status::OK;
}

Status AudioG711aDecoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    if (!outputBuffer) {
        AVCODEC_LOGE("AudioG711aDecoderPlugin Queue out buffer is null.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        auto memory = outputBuffer->memory_;
        auto outSize = sizeof(int16_t) * decodeResult_.size();

        memory->Write(reinterpret_cast<const uint8_t *>(decodeResult_.data()), outSize, 0);
        memory->SetSize(outSize);
        outputBuffer->pts_ = pts_;
        if (sampleFormat_ == SAMPLE_S16LE && sampleRate_ > 0 && channels_ > 0) {
            float usPerSample = TIME_ONE_SECOND / sampleRate_;
            outputBuffer->duration_ = static_cast<uint32_t>((outSize / 2.0f / channels_) * usPerSample); // 2 bytes
        }
        dataCallback_->OnOutputBufferDone(outputBuffer);
    }
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

    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
        AVCODEC_LOGD("AudioG711aDecoderPlugin SetParameter maxInputSize_: %{public}d", maxInputSize_);
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
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
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