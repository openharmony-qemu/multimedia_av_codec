/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "audio_mp3_encoder_plugin.h"
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_mime_type.h"
#include "lame.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_definition.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Mp3;
using namespace std;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioMp3EncoderPlugin"};
constexpr int32_t ONE_CHANNEL = 1;
constexpr int32_t TWO_CHANNELS = 2;
constexpr int32_t SUPPORT_SAMPLE_RATE = 9;
constexpr int32_t SUPPORT_BIT_RATE = 16;

constexpr int32_t OUTPUT_BUFFER_SIZE_DEFAULT = 4096;
constexpr int32_t LAME_BUFFER_SIZE_DEFAULT = 8640;         // 1152*1.25+7200=8640
constexpr int32_t LAME_INPUT_BUFFER_SIZE_ONE_CHAN = 2304;  // 1152*2=2304
constexpr int32_t LAME_INPUT_BUFFER_SIZE_TWO_CHAN = 4608;  // 1152*2*2=4608
constexpr int32_t INPUT_BUFFER_SIZE_DEFAULT = 5120;        // 1152*2*2=4608

constexpr int32_t BUFFER_FLAG_EOS = 0x00000001;

constexpr int32_t BIT_RATE_PICK[SUPPORT_BIT_RATE] = {8000, 16000, 32000, 40000, 48000, 56000,
                                                     64000, 80000, 96000, 112000, 128000, 160000,
                                                     192000, 224000, 256000, 320000};
constexpr int32_t SAMPLE_RATE_PICK[SUPPORT_SAMPLE_RATE] = {8000, 11025, 12000, 16000, 22050,
                                                           24000, 32000, 44100, 48000};

Status RegisterAudioEncoderPlugins(const std::shared_ptr<Register>& reg)
{
    CodecPluginDef definition;
    definition.name = std::string(OHOS::MediaAVCodec::AVCodecCodecName::AUDIO_ENCODER_MP3_NAME);
    definition.pluginType = PluginType::AUDIO_ENCODER;
    definition.rank = 100;  // 100
    definition.SetCreator([](const std::string& name) -> std::shared_ptr<CodecPlugin> {
        return std::make_shared<AudioMp3EncoderPlugin>(name);
    });

    Capability cap;

    cap.SetMime(MimeType::AUDIO_MPEG);
    cap.AppendFixedKey<CodecMode>(Tag::MEDIA_CODEC_MODE, CodecMode::SOFTWARE);

    definition.AddInCaps(cap);
    // do not delete the codec in the deleter
    if (reg->AddPlugin(definition) != Status::OK) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Register Failure");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

void UnRegisterAudioEncoderPlugin() {}

PLUGIN_DEFINITION(Mp3AudioEncoder, LicenseType::APACHE_V2, RegisterAudioEncoderPlugins, UnRegisterAudioEncoderPlugin);
}  // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mp3 {

struct AudioMp3EncoderPlugin::LameInfo {
    lame_global_flags* gfp;
};

AudioMp3EncoderPlugin::AudioMp3EncoderPlugin(const std::string& name)
    : CodecPlugin(std::move(name)),
      audioSampleFormat_(AudioSampleFormat::SAMPLE_S16LE),
      lameInitFlag(0),
      channels_(ONE_CHANNEL),
      bitrate_(0),
      sampleRate_(SUPPORT_SAMPLE_RATE),
      maxInputSize_(INPUT_BUFFER_SIZE_DEFAULT),
      maxOutputSize_(OUTPUT_BUFFER_SIZE_DEFAULT),
      outputSize_(-1)
{
    std::lock_guard<std::mutex> lock(avMutex_);

    unsigned char* tmp = new unsigned char[LAME_BUFFER_SIZE_DEFAULT];
    lameInfo = new LameInfo();

    if (tmp == nullptr || lameInfo == nullptr) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin new LAME buffer or lameInfo failed");
    }
    lameMp3Buffer = tmp;
    lameInfo->gfp = nullptr;
}

AudioMp3EncoderPlugin::~AudioMp3EncoderPlugin()
{
    if (lameMp3Buffer) {
    delete[] lameMp3Buffer;
        lameMp3Buffer = nullptr;
    }

    if (lameInfo) {
        delete lameInfo;
        lameInfo = nullptr;
    }
}

bool AudioMp3EncoderPlugin::CheckFormat()
{
    if (channels_ != ONE_CHANNEL && channels_ != TWO_CHANNELS) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin channels not supported");
        return false;
    }

    for (int32_t i = 0; i < SUPPORT_BIT_RATE; i++) {
        if (bitrate_ == BIT_RATE_PICK[i]) {
            break;
        } else if (i == SUPPORT_BIT_RATE - 1) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin bitRate not supported");
            return false;
        }
    }

    for (int32_t i = 0; i < SUPPORT_SAMPLE_RATE; i++) {
        if (sampleRate_ == SAMPLE_RATE_PICK[i]) {
            break;
        } else if (i == SUPPORT_SAMPLE_RATE - 1) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin sampleRate not supported");
            return false;
        }
    }

    if (audioSampleFormat_ != AudioSampleFormat::SAMPLE_S16LE) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin sampleFmt not supported");
        return false;
    }
    return true;
}

Status AudioMp3EncoderPlugin::Init()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    lameInfo->gfp = lame_init();
    lameInitFlag = 0;
    if (lameInfo->gfp == nullptr) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin LAME initialization error");
        return Status::ERROR_UNKNOWN;
    }

    if (lameMp3Buffer == nullptr) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin lameMp3Buffer allocation failed");
        return Status::ERROR_UNKNOWN;
    }

    return Status::OK;
}

Status AudioMp3EncoderPlugin::Start()
{
    if (!CheckFormat()) {
        AVCODEC_LOGE("Format check failed.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    return Status::OK;
}

Status AudioMp3EncoderPlugin::QueueInputBuffer(const std::shared_ptr<AVBuffer>& inputBuffer)
{
    auto memory = inputBuffer->memory_;
    auto inputSize = memory->GetSize();
    int32_t sampleNumTmp = -1;
    if (inputSize < 0) {
        AVCODEC_LOGE("SendBuffer buffer is less than zero.  size: %{public}d", inputSize);
        return Status::ERROR_UNKNOWN;
    }
    if (inputSize == 0 && !(inputBuffer->flag_ & BUFFER_FLAG_EOS)) {
        AVCODEC_LOGE("size is 0, but eos flag is not 1");
        return Status::ERROR_INVALID_DATA;
    }
    if ((channels_ == ONE_CHANNEL && inputSize > LAME_INPUT_BUFFER_SIZE_ONE_CHAN) ||
        (channels_ == TWO_CHANNELS && inputSize > LAME_INPUT_BUFFER_SIZE_TWO_CHAN)) {
        AVCODEC_LOGE("SendBuffer size > max InputBufferSize(1 channel: 2304bytes; 2 channels: 4608bytes)\
            size: %{public}d", inputSize);
        return Status::ERROR_UNKNOWN;
    }
    if (inputSize > memory->GetCapacity()) {
        AVCODEC_LOGE("send input buffer is > allocate size. size : %{public}d, allocate size : %{public}d",
                     inputSize, memory->GetCapacity());
        return Status::ERROR_UNKNOWN;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        sampleNumTmp = channels_ == ONE_CHANNEL ? inputSize / sizeof(int16_t) : inputSize / sizeof(int16_t) / 2; // 2ch
        unsigned char* lamePcmBuffer = memory->GetAddr();
        int outputSize = 0;

        const int sampleNum = static_cast<const int>(sampleNumTmp);
        const short* inputPcmBuffer = reinterpret_cast<const short*>(lamePcmBuffer);
        if (sampleNumTmp > 0) {
            if (channels_ == 1) {
                outputSize = lame_encode_buffer(lameInfo->gfp, inputPcmBuffer, inputPcmBuffer, sampleNum, lameMp3Buffer,
                                                LAME_BUFFER_SIZE_DEFAULT);
            } else {
                outputSize = lame_encode_buffer_interleaved(lameInfo->gfp, reinterpret_cast<short*>(lamePcmBuffer),
                                                            sampleNumTmp, lameMp3Buffer, LAME_BUFFER_SIZE_DEFAULT);
            }
        } else if (sampleNumTmp == 0) {
            outputSize = lame_encode_flush(lameInfo->gfp, lameMp3Buffer, LAME_BUFFER_SIZE_DEFAULT);
        }

        if (outputSize < 0) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin lame encode error.");
            return Status::ERROR_UNKNOWN;
        }
        outputSize_ = outputSize;
        dataCallback_->OnInputBufferDone(inputBuffer);
    }

    return Status::OK;
}

Status AudioMp3EncoderPlugin::QueueOutputBuffer(std::shared_ptr<AVBuffer>& outputBuffer)
{
    if (!outputBuffer) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Queue out buffer is null.");
        return Status::ERROR_INVALID_PARAMETER;
    }
    {
        std::lock_guard<std::mutex> lock(avMutex_);
        auto memory = outputBuffer->memory_;

        if (outputSize_ > 0) {
            memory->Write(const_cast<const uint8_t*>(lameMp3Buffer), outputSize_, 0);
        }
        memory->SetSize(outputSize_);
        dataCallback_->OnOutputBufferDone(outputBuffer);
    }

    return Status::OK;
}

Status AudioMp3EncoderPlugin::Reset()
{
    AVCODEC_LOGD("Reset begins");
    auto ret = Release();
    if (ret != Status::OK) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Reset Release error.");
        return ret;
    }
    ret = Init();
    if (ret != Status::OK) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Reset Init error.");
        return ret;
    }

    return Status::OK;
}

Status AudioMp3EncoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (lameInfo != nullptr && lameInfo->gfp != nullptr) {
        if (lameInitFlag == 0) {
            if (lame_init_params(lameInfo->gfp) < 0) {
                AVCODEC_LOGE("AudioMp3EncoderPlugin Release LAME parameter initialization error");
                return Status::ERROR_UNKNOWN;
            }
            lameInitFlag = 1;
        }
        int ret = lame_encode_flush(lameInfo->gfp, lameMp3Buffer, LAME_BUFFER_SIZE_DEFAULT);
        if (ret < 0) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin Release lame_encode_flush error.");
            return Status::ERROR_UNKNOWN;
        }
        ret = lame_close(lameInfo->gfp);
        if (ret < 0) {
            AVCODEC_LOGE("AudioMp3EncoderPlugin lame_close error.");
            return Status::ERROR_UNKNOWN;
        }
        lameInfo->gfp = nullptr;
        lameInitFlag = 0;
    }
    return Status::OK;
}

Status AudioMp3EncoderPlugin::Flush()
{
    return Status::OK;
}

Status AudioMp3EncoderPlugin::SetParameter(const std::shared_ptr<Meta>& parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);

    if (parameter->Find(Tag::AUDIO_CHANNEL_COUNT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_);
    } else {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no AUDIO_CHANNEL_COUNT");
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (parameter->Find(Tag::AUDIO_SAMPLE_RATE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    } else {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no AUDIO_SAMPLE_RATE");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_SAMPLE_FORMAT) != parameter->end()) {
        parameter->Get<Tag::AUDIO_SAMPLE_FORMAT>(audioSampleFormat_);
    } else {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no AUDIO_SAMPLE_FORMAT");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::MEDIA_BITRATE) != parameter->end()) {
        parameter->Get<Tag::MEDIA_BITRATE>(bitrate_);
    } else {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. no MEDIA_BITRATE of mp3 encoder CBR");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (parameter->Find(Tag::AUDIO_MAX_INPUT_SIZE) != parameter->end()) {
        parameter->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
        AVCODEC_LOGD("AudioMp3EncoderPlugin SetParameter maxInputSize_: %{public}d", maxInputSize_);
    }

    if (!CheckFormat()) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin SetParameter error. CheckFormat fail");
        return Status::ERROR_INVALID_PARAMETER;
    }
    audioParameter_ = *parameter;

    lame_set_in_samplerate(lameInfo->gfp, sampleRate_);

    lame_set_num_channels(lameInfo->gfp, channels_);

    if (channels_ == 1) {
        lame_set_mode(lameInfo->gfp, MPEG_mode_e::MONO);
    }

    lame_set_brate(lameInfo->gfp, bitrate_);
    if (lame_init_params(lameInfo->gfp) < 0) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin LAME parameter initialization error");
        return Status::ERROR_UNKNOWN;
    }
    lameInitFlag = 1;
    return Status::OK;
}

Status AudioMp3EncoderPlugin::GetParameter(std::shared_ptr<Meta>& parameter)
{
    std::lock_guard<std::mutex> lock(avMutex_);
    if (maxInputSize_ <= 0 || maxInputSize_ > INPUT_BUFFER_SIZE_DEFAULT) {
        maxInputSize_ = INPUT_BUFFER_SIZE_DEFAULT;
    }
    maxOutputSize_ = OUTPUT_BUFFER_SIZE_DEFAULT;
    AVCODEC_LOGD("AudioMp3EncoderPlugin GetParameter maxInputSize_: %{public}d", maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    audioParameter_.Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize_);
    audioParameter_.Set<Tag::AUDIO_MAX_OUTPUT_SIZE>(maxOutputSize_);
    audioParameter_.Set<Tag::MEDIA_BITRATE>(bitrate_);
    *parameter = audioParameter_;
    return Status::OK;
}

Status AudioMp3EncoderPlugin::Prepare()
{
    return Status::OK;
}

Status AudioMp3EncoderPlugin::Stop()
{
    std::lock_guard<std::mutex> lock(avMutex_);
    int result = lame_encode_flush(lameInfo->gfp, lameMp3Buffer, LAME_BUFFER_SIZE_DEFAULT);
    if (result < 0) {
        AVCODEC_LOGE("AudioMp3EncoderPlugin Stop lame_encode_flush error.");
        return Status::ERROR_UNKNOWN;
    }
    return Status::OK;
}

Status AudioMp3EncoderPlugin::GetInputBuffers(std::vector<std::shared_ptr<AVBuffer>>& inputBuffers)
{
    return Status::OK;
}

Status AudioMp3EncoderPlugin::GetOutputBuffers(std::vector<std::shared_ptr<AVBuffer>>& outputBuffers)
{
    return Status::OK;
}

}  // namespace Mp3
}  // namespace Plugins
}  // namespace Media
}  // namespace OHOS