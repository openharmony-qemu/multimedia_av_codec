/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <set>
#include <dlfcn.h>
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_log.h"
#include "media_description.h"
#include "avcodec_mime_type.h"
#include "ffmpeg_converter.h"
#include "avcodec_audio_common.h"
#include "securec.h"
#include "audio_opus_encoder_plugin.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioOpusEncoderPlugin"};
constexpr std::string_view AUDIO_CODEC_NAME = "opus";
constexpr int32_t INITVAL = -1;
constexpr float TIME_S = 0.02;
constexpr int64_t TIME_US = 20000;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 2;
constexpr int32_t MIN_COMPLEX = 1;
constexpr int32_t MAX_COMPLEX = 10;
constexpr int32_t MIN_BITRATE = 6000;
constexpr int32_t MAX_BITRATE = 510000;
static const int32_t OPUS_ENCODER_SAMPLE_RATE_TABLE[] = {
    8000, 12000, 16000, 24000, 48000
};
}

namespace OHOS {
namespace MediaAVCodec {
AudioOpusEncoderPlugin::AudioOpusEncoderPlugin()
    : PluginCodecPtr(nullptr), fbytes(nullptr), len(-1), codeData(nullptr), sampleFmt(-1),
      channels(-1), sampleRate(-1), bitRate(-1), complexity(-1)
{
    ret = 0;
    void* handle = dlopen("/system/lib64/libav_codec_ext_base.z.so", 1);
    if (!handle) {
        ret = -1;
        AVCODEC_LOGE("AudioOpusEncoderPlugin dlopen error, check .so file exist");
    }
    OpusPluginClassCreateFun* PluginCodecCreate = (OpusPluginClassCreateFun *)dlsym(handle,
        "OpusPluginClassEncoderCreate");
    if (!PluginCodecCreate) {
        ret = -1;
        AVCODEC_LOGE("AudioOpusEncoderPlugin dlsym error, check .so file has this function");
    }
    if (ret == 0) {
        AVCODEC_LOGD("AudioOpusEncoderPlugin dlopen and dlsym success");
        ret = PluginCodecCreate(&PluginCodecPtr);
    }
}

AudioOpusEncoderPlugin::~AudioOpusEncoderPlugin()
{
    Release();
}

static bool CheckSampleRate(int32_t sampleR)
{
    for (auto i : OPUS_ENCODER_SAMPLE_RATE_TABLE) {
        if (i == sampleR) {
            return true;
        }
    }
    return false;
}

int32_t AudioOpusEncoderPlugin::CheckSampleFormat()
{
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin channels not supported");
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_MISMATCH_CHANNEL_COUNT;
    }
    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin sampleRate not supported");
        return AVCodecServiceErrCode::AVCS_ERR_MISMATCH_SAMPLE_RATE;
    }
    if (bitRate < MIN_BITRATE || bitRate > MAX_BITRATE) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin bitRate not supported");
        return AVCodecServiceErrCode::AVCS_ERR_MISMATCH_BIT_RATE;
    }
    if (complexity < MIN_COMPLEX || complexity > MAX_COMPLEX) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin complexity not supported");
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_ERROR;
    }
    if (sampleFmt != OHOS::MediaAVCodec::AudioSampleFormat::SAMPLE_S16LE) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin sampleFmt not supported");
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_ERROR;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusEncoderPlugin::Init(const Format &format)
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin Init dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    channels = INITVAL;
    sampleRate = INITVAL;
    sampleFmt = INITVAL;
    bitRate = (int64_t) INITVAL;
    complexity = INITVAL;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFmt);
    format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, complexity);

    format_ = format;
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT, sampleFmt);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL, complexity);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS);

    ret = CheckSampleFormat();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin parameter not supported");
        return ret;
    }
    ret = PluginCodecPtr->SetPluginParameter(CHANNEL, channels);
    ret = PluginCodecPtr->SetPluginParameter(SAMPLE_RATE, sampleRate);
    ret = PluginCodecPtr->SetPluginParameter(BIT_RATE, (int32_t) bitRate);
    ret = PluginCodecPtr->SetPluginParameter(COMPLEXITY, complexity);
    ret = PluginCodecPtr->Init();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return ret;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusEncoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin inputBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin ProcessSendData dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto attr = inputBuffer->GetBufferAttr();
        bool isEos = inputBuffer->CheckIsEos();
        AVCODEC_LOGD("SendBuffer buffer size:%{public}d", attr.size);
        if (attr.size != ((int32_t) (sampleRate*TIME_S))*channels*sizeof(short) && !isEos) {
            AVCODEC_LOGE("SendBuffer buffer size:%{public}d, expect:%{public}f", attr.size,
                ((int32_t) sampleRate*TIME_S*sizeof(short)*channels));
            return AVCodecServiceErrCode::AVCS_ERR_INVALID_DATA;
        }
        if (!isEos) {
            auto memory = inputBuffer->GetBuffer();
            fbytes = memory->GetBase();
            PluginCodecPtr->ProcessSendData(fbytes, &len);
        } else {
            AVCODEC_LOGD("SendBuffer EOS buffer");
            len = -1;
            return AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM;
        }
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusEncoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin ProcessRecieveData dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (!outBuffer) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin outBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        if (len > 0) {
            PluginCodecPtr->ProcessRecieveData(&codeData, len);
            auto memory = outBuffer->GetBuffer();
            memory->Write(codeData, len);
            auto attr = outBuffer->GetBufferAttr();
            attr.size = len;
            attr.presentationTimeUs = TIME_US;
            outBuffer->SetBufferAttr(attr);
            ret = AVCodecServiceErrCode::AVCS_ERR_OK;
        } else {
            outBuffer->SetEos(true);
            ret = AVCodecServiceErrCode::AVCS_ERR_END_OF_STREAM;
        }
    }
    return ret;
}

int32_t AudioOpusEncoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin Reset dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    ret = PluginCodecPtr->Reset();
    if (ret != 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusEncoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (!PluginCodecPtr) {
        AVCODEC_LOGD("AudioOpusEncoderPlugin Release dlopen or dlsym error. release");
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }
    ret = PluginCodecPtr->Release();
    if (ret != 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    free(PluginCodecPtr);
    PluginCodecPtr = nullptr;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusEncoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin Flush dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    ret = PluginCodecPtr->Flush();
    if (ret != 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusEncoderPlugin::GetInputBufferSize() const
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin GetInputBufferSize dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return PluginCodecPtr->GetInputBufferSize();
}

int32_t AudioOpusEncoderPlugin::GetOutputBufferSize() const
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusEncoderPlugin GetOutputBufferSize dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return PluginCodecPtr->GetOutputBufferSize();
}

Format AudioOpusEncoderPlugin::GetFormat() const noexcept
{
    return format_;
}

std::string_view AudioOpusEncoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS