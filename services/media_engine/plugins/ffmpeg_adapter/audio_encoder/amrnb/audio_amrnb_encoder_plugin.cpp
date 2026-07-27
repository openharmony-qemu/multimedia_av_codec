/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "audio_amrnb_encoder_plugin.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "avcodec_log.h"
#include "common/pcm_convert.h"
#include <cstring>
#include <iostream>
#include <set>

enum ParaType : int32_t {
    CHANNEL = 0,
    SAMPLE_RATE = 1,
    BIT_RATE = 2,
    COMPLEXITY = 3,
};

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace AmrNb;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioAmrNbEncoderPlugin"};
constexpr int32_t SUPPORT_CHANNELS = 1;
constexpr int32_t SUPPORT_SAMPLE_RATE = 8000;
constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 150;
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 160 * 2;
constexpr enum Mode ENCODE_MODE_DEFAULT = MR122;
constexpr int32_t AMR_FRAME_LEN = 160;
constexpr int32_t MAX_INPUT_BUFFER_SIZE_LIMIT = 10 * 1024 * 1024; // 10MB
constexpr uint32_t BUFFER_FLAG_EOS = 0x1;
static const std::map<int32_t, enum Mode> BITRATE_MODE_MAP = {
    { 4750, MR475 },
    { 5150, MR515 },
    { 5900, MR59 },
    { 6700, MR67 },
    { 7400, MR74 },
    { 7950, MR795 },
    { 10200, MR102 },
    { 12200, MR122 }
};
static std::set<AudioSampleFormat> extendedSampleFormats = {
    AudioSampleFormat::SAMPLE_U8,
    AudioSampleFormat::SAMPLE_S16LE,
    AudioSampleFormat::SAMPLE_S24LE,
    AudioSampleFormat::SAMPLE_S32LE,
    AudioSampleFormat::SAMPLE_F32LE,
};

Status RegisterAudioEncoderPlugins(const std::shared_ptr<Register>& reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_AMRNB_NAME);
    definition.pluginType = PluginType::AUDIO_ENCODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioAmrNbEncoderPlugin>(name);
    });

    Capability cap;

    cap.SetMime(MimeType::AUDIO_AMR_NB);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioAmrNbEncoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioEncoderPlugin() {}
} // namespace

PLUGIN_DEFINITION(AmrNbAudioEncoder, LicenseType::APACHE_V2, RegisterAudioEncoderPlugins, UnRegisterAudioEncoderPlugin);

namespace OHOS {
namespace Media {
namespace Plugins {
namespace AmrNb {
AudioAmrNbEncoderPlugin::AudioAmrNbEncoderPlugin(std::string name) : CodecPlugin(std::move(name)),
    channels_(SUPPORT_CHANNELS), sampleRate_(SUPPORT_SAMPLE_RATE), bitrate_(0),
    maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT), maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT), packetSize_(0),
    encMode_(ENCODE_MODE_DEFAULT), srcFmt_(AudioSampleFormat::SAMPLE_S16LE)
{
}

AudioAmrNbEncoderPlugin::~AudioAmrNbEncoderPlugin()
{
}

Status AudioAmrNbEncoderPlugin::Init()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    auto state = Encoder_Interface_init(0);
    if (state == nullptr) {
        AVCODEC_LOGE("amrnb encoder init failed.");
        return Status::ERROR_INVALID_OPERATION;
    }
    amrState_ = std::shared_ptr<void>(state, [](void *ptr) { Encoder_Interface_exit(ptr); });
    amrPacket_.reserve(OUTPUT_BUFFER_SIZE_DEFAULT);
    return Status::OK;
}

bool AudioAmrNbEncoderPlugin::CheckChannel(const std::shared_ptr<Meta> &parameter)
{
    if (!parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_)) {
        AVCODEC_LOGE("AudioAmrNbEncoderPlugin SetParameter channels missing, please check");
        return false;
    }
    if (channels_ != SUPPORT_CHANNELS) {
        AVCODEC_LOGE("AudioAmrNbEncoderPlugin not support channel:%{public}d", channels_);
        return false;
    }
    return true;
}

bool AudioAmrNbEncoderPlugin::CheckFormat(const std::shared_ptr<Meta> &parameter)
{
    if (parameter->Get<Tag::AUDIO_ENCODER_ENABLE_SAMPLE_FORMAT_CONVERT>(isEnableFormatConvert_)) {
        AVCODEC_LOGI("set format convert:%{public}d", static_cast<int32_t>(isEnableFormatConvert_));
    }
    if (!parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(srcFmt_)) {
        AVCODEC_LOGE("AudioAmrNbEncoderPlugin SetParameter AUDIO_SAMPLE_FORMAT missing please check");
        return false;
    }
    if (!isEnableFormatConvert_ && srcFmt_ != AudioSampleFormat::SAMPLE_S16LE) {
        AVCODEC_LOGE("AudioAmrNbEncoderPlugin SetParameter AUDIO_SAMPLE_FORMAT wrong, please check");
        return false;
    }
    if (isEnableFormatConvert_ && extendedSampleFormats.find(srcFmt_) == extendedSampleFormats.end()) {
        AVCODEC_LOGE("AudioAmrNbEncoderPlugin SetParameter AUDIO_SAMPLE_FORMAT not support, please check");
        return false;
    }
    return true;
}

bool AudioAmrNbEncoderPlugin::CheckBitrate(const std::shared_ptr<Meta> &parameter)
{
    int64_t bitrate;
    if (!parameter->Get<Tag::MEDIA_BITRATE>(bitrate)) {
        AVCODEC_LOGE("AudioAmrNbEncoderPlugin SetParameter birate_ missing, please check");
        return false;
    }
    auto iter = BITRATE_MODE_MAP.find(bitrate);
    if (iter == BITRATE_MODE_MAP.end()) {
        AVCODEC_LOGE("Parameter bitrate not support, %{public}" PRId64, bitrate);
        return false;
    }
    bitrate_ = bitrate;
    encMode_ = iter->second;
    return true;
}

bool AudioAmrNbEncoderPlugin::CheckSampleRate(const std::shared_ptr<Meta> &parameter)
{
    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
        if (sampleRate_ != SUPPORT_SAMPLE_RATE) {
            AVCODEC_LOGE("AudioAmrNbEncoderPlugin SetParameter sample rate wrong, please check");
            return false;
        }
    }
    return true;
}

Status AudioAmrNbEncoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    isFirstInputPts_ = true;
    prevPts_ = 0;
    ptsMode_ = DEFAULT_ENCODE_PTS_MODE;
    pcmCache_.clear();
    maxInputBufferSize_ = -1;
    enableCache_ = false;
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::Start()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    if (!parameter) {
        AVCODEC_LOGE("parameter is nullptr");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(paramMutex_);
    if (!CheckChannel(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!CheckFormat(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!CheckBitrate(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (!CheckSampleRate(parameter)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    int32_t ptsMode = DEFAULT_ENCODE_PTS_MODE;
    if (parameter->Get<Tag::AUDIO_ENCODE_PTS_MODE>(sampleRate_)) {
        ptsMode_ = ptsMode;
    }
    AVCODEC_LOGI("AudioAmrNbEncoderPlugin SetParameter success");
    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    } else {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    if (parameter->Find(Tag::AUDIO_MAX_OUTPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    } else {
        maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    }
    maxInputBufferSize_ = -1;
    enableCache_ = false;
    parameter->Get<Tag::AUDIO_MAX_INPUT_BUFFER_SIZE>(maxInputBufferSize_);
    if (maxInputBufferSize_ > 0) {
        if (maxInputBufferSize_ > MAX_INPUT_BUFFER_SIZE_LIMIT) {
            AVCODEC_LOGW("maxInputBufferSize exceeds limit, clamped to %{public}d", MAX_INPUT_BUFFER_SIZE_LIMIT);
            maxInputBufferSize_ = MAX_INPUT_BUFFER_SIZE_LIMIT;
        }
        enableCache_ = true;
        AVCODEC_LOGI("AudioAmrNbEncoderPlugin cache enabled, maxInputBufferSize: %{public}d", maxInputBufferSize_);
    }
    audioParameter_ = *parameter;
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::GetParameter(std::shared_ptr<Meta> &parameter)
{
    if (!parameter) {
        AVCODEC_LOGE("parameter is nullptr");
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::lock_guard<std::mutex> lock(paramMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    if (maxOutputSize_ <= 0 || maxOutputSize_ > OUTPUT_BUFFER_SIZE_DEFAULT) {
        maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    }
    if (maxInputBufferSize_ > 0) {
        audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputBufferSize_);
    } else {
        audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    }
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    audioParameter_.Set<Tag::MIME_TYPE>(MimeType::AUDIO_AMR_NB);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);

    *parameter = audioParameter_;
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("inputBuffer is nullptr");
        return Status::ERROR_INVALID_DATA;
    }
    auto buffer = inputBuffer->memory_;
    if (!buffer) {
        AVCODEC_LOGE("buffer is nullptr");
        return Status::ERROR_INVALID_DATA;
    }
    if (inputBuffer->flag_ & BUFFER_FLAG_EOS) {
        SafeCallInputBufferDone(dataCallback_, inputBuffer);
        return Status::OK;
    }
    if (buffer->GetSize() == 0) {
        return Status::ERROR_INVALID_DATA;
    }
    Status ret = Status::OK;
    std::lock_guard<std::mutex> lock(avMutex_);
    size_t sampleCount = static_cast<size_t>(buffer->GetSize() / GetPcmBytesPerSample(srcFmt_));
    const uint8_t *srcBuffer = reinterpret_cast<const uint8_t *>(buffer->GetAddr());
    if (!srcBuffer) {
        AVCODEC_LOGE("srcBuffer is nullptr");
        return Status::ERROR_INVALID_DATA;
    }
    if (ptsMode_ == FIRST_INPUT_START_ENCODE_PTS_MODE && isFirstInputPts_) {
        prevPts_ = inputBuffer->pts_;
        isFirstInputPts_ = false;
    }
    const short *encodeBuffer = reinterpret_cast<const short *>(srcBuffer);
    if (isEnableFormatConvert_ && srcFmt_ != AudioSampleFormat::SAMPLE_S16LE) {
        convertBuf_.resize(sampleCount * sizeof(int16_t));
        ConvertPcmSampleFormat(
            srcBuffer, sampleCount, srcFmt_, AudioSampleFormat::SAMPLE_S16LE, convertBuf_.data());
        encodeBuffer = reinterpret_cast<const short *>(convertBuf_.data());
    }
    if (!enableCache_) {
        packetSize_ = Encoder_Interface_Encode(amrState_.get(), encMode_, encodeBuffer, amrPacket_.data(), 0);
    } else {
        pcmCache_.insert(pcmCache_.end(), reinterpret_cast<const uint8_t *>(encodeBuffer),
            (reinterpret_cast<const uint8_t *>(encodeBuffer) + sampleCount * sizeof(int16_t)));
        if (pcmCache_.size() < AMR_FRAME_LEN * sizeof(int16_t)) {
            AVCODEC_LOGD("pcmCache size %{public}zu less than frameSize", pcmCache_.size());
            SafeCallInputBufferDone(dataCallback_, inputBuffer);
            return Status::OK;
        }
        const short *pcmToEncode = reinterpret_cast<const short *>(pcmCache_.data());
        packetSize_ = Encoder_Interface_Encode(amrState_.get(), encMode_, pcmToEncode, amrPacket_.data(), 0);
        pcmCache_.erase(pcmCache_.begin(), pcmCache_.begin() + AMR_FRAME_LEN * sizeof(int16_t));
    }
    if (packetSize_ > 0) {
        bufferMeta_ = inputBuffer->meta_;
        SafeCallInputBufferDone(dataCallback_, inputBuffer);
    } else {
        ret = Status::ERROR_INVALID_DATA;
        AVCODEC_LOGE("amrnb encode failed");
    }
    return ret;
}

Status AudioAmrNbEncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer> &outputBuffer)
{
    if (!outputBuffer) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (outputBuffer->flag_ & BUFFER_FLAG_EOS) {
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
        return Status::END_OF_STREAM;
    }
    std::lock_guard<std::mutex> lock(avMutex_);
    auto memory = outputBuffer->memory_;
    if (!memory) {
        AVCODEC_LOGE("memory is nullptr");
        return Status::ERROR_NO_MEMORY;
    }
    if (memory->GetCapacity() < packetSize_) {
        AVCODEC_LOGE("Output buffer size not enough, buffer size:%{public}d", memory->GetCapacity());
        return Status::ERROR_NO_MEMORY;
    }
    if (enableCache_ && packetSize_ <= 0) {
        AVCODEC_LOGD("cache not enough: %{public}d", packetSize_);
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    auto lenOut = memory->Write(amrPacket_.data(), packetSize_, 0);
    if (lenOut != packetSize_) {
        AVCODEC_LOGE("Output buffer write failed, packetSize: %{public}d, write size:%{public}d", packetSize_, lenOut);
        return Status::ERROR_NO_MEMORY;
    }
    packetSize_ = 0;
    int64_t frameDuration = 20000; // 20000: amr编码固定一帧时长
    if (ptsMode_ != DEFAULT_ENCODE_PTS_MODE) {
        outputBuffer->pts_ = prevPts_;
        outputBuffer->duration_ = frameDuration;
        if (prevPts_ < 0) {
            prevPts_ += frameDuration;
        } else {
            prevPts_ = ((INT64_MAX - prevPts_) < frameDuration) ? (frameDuration - (INT64_MAX - prevPts_))
                       : (prevPts_ + frameDuration);
        }
    }
    outputBuffer->meta_ = bufferMeta_;
    if (enableCache_ && pcmCache_.size() >= AMR_FRAME_LEN * sizeof(int16_t)) {
        const short *pcmToEncode = reinterpret_cast<const short *>(pcmCache_.data());
        packetSize_ = Encoder_Interface_Encode(amrState_.get(), encMode_, pcmToEncode, amrPacket_.data(), 0);
        pcmCache_.erase(pcmCache_.begin(), pcmCache_.begin() + AMR_FRAME_LEN * sizeof(int16_t));
        SafeCallOutputBufferDone(dataCallback_, outputBuffer);
        return Status::ERROR_AGAIN;
    }
    SafeCallOutputBufferDone(dataCallback_, outputBuffer);
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>> &inputBuffers)
{
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>> &outputBuffers)
{
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    pcmCache_.clear();
    auto state = Encoder_Interface_init(0);
    if (state == nullptr) {
        AVCODEC_LOGE("amrnb encoder init failed.");
    }
    isFirstInputPts_ = true;
    prevPts_ = 0;
    amrState_ = std::shared_ptr<void>(state, [](void *ptr) { Encoder_Interface_exit(ptr); });
    return Status::OK;
}

Status AudioAmrNbEncoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    return Status::OK;
}

}  // namespace AmrNb
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS