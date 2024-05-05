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
#include "audio_opus_decoder_plugin.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioOpusDecoderPlugin"};
constexpr std::string_view AUDIO_CODEC_NAME = "opus";
constexpr int32_t INITVAL = -1;
constexpr int64_t TIME_US = 20000;
constexpr int32_t MIN_CHANNELS = 1;
constexpr int32_t MAX_CHANNELS = 2;

static const int32_t OPUS_DECODER_SAMPLE_RATE_TABLE[] = {
    8000, 12000, 16000, 24000, 48000
};
}

namespace OHOS {
namespace MediaAVCodec {
AudioOpusDecoderPlugin::AudioOpusDecoderPlugin()
    : PluginCodecPtr(nullptr), fbytes(nullptr), len(-1), codeData(nullptr), channels(-1), sampleRate(-1)
{
    ret = 0;
    void *handle = dlopen("/system/lib64/libav_codec_ext_base.z.so", 1);
    if (!handle) {
        ret = -1;
        AVCODEC_LOGE("AudioOpusDecoderPlugin dlopen error, check .so file exist");
    }
    OpusPluginClassCreateFun *PluginCodecCreate =
        (OpusPluginClassCreateFun *)dlsym(handle, "OpusPluginClassDecoderCreate");
    if (!PluginCodecCreate) {
        ret = -1;
        AVCODEC_LOGE("AudioOpusDecoderPlugin dlsym error, check .so file has this function");
    }
    if (ret == 0) {
        AVCODEC_LOGD("AudioOpusDecoderPlugin dlopen and dlsym success");
        ret = PluginCodecCreate(&PluginCodecPtr);
    }
}

AudioOpusDecoderPlugin::~AudioOpusDecoderPlugin()
{
    Release();
}

static bool CheckSampleRate(int32_t sampleR)
{
    for (auto i : OPUS_DECODER_SAMPLE_RATE_TABLE) {
        if (i == sampleR) {
            return true;
        }
    }
    return false;
}

int32_t AudioOpusDecoderPlugin::CheckSampleFormat()
{
    if (channels < MIN_CHANNELS || channels > MAX_CHANNELS) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin channels not supported");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (!CheckSampleRate(sampleRate)) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin sampleRate not supported");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusDecoderPlugin::Init(const Format &format)
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin Init dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    channels = INITVAL;
    sampleRate = INITVAL;

    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);

    format_ = format;
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate);
    format_.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS);

    AVCODEC_LOGD("AudioOpusDecoderPlugin parameter channels:%{public}d samplerate:%{public}d", channels, sampleRate);
    ret = CheckSampleFormat();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin parameter not supported");
        return ret;
    }
    ret = PluginCodecPtr->SetPluginParameter(CHANNEL, channels);
    ret = PluginCodecPtr->SetPluginParameter(SAMPLE_RATE, sampleRate);
    ret = PluginCodecPtr->Init();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return ret;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusDecoderPlugin::ProcessSendData(const std::shared_ptr<AudioBufferInfo> &inputBuffer)
{
    if (!inputBuffer) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin inputBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin ProcessSendData dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        auto attr = inputBuffer->GetBufferAttr();
        bool isEos = inputBuffer->CheckIsEos();
        len = attr.size;

        if ((len <= 0 || (uint32_t) len > inputBuffer->GetBufferSize()) && !isEos) {
            AVCODEC_LOGE("SendBuffer error buffer size:%{public}d", len);
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
        AVCODEC_LOGD("PCM buffer size:%{public}d", len);
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusDecoderPlugin::ProcessRecieveData(std::shared_ptr<AudioBufferInfo> &outBuffer)
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin ProcessRecieveData dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    if (!outBuffer) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin outBuffer is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    {
        std::lock_guard<std::mutex> lock(avMutext_);
        if (len > 0) {
            PluginCodecPtr->ProcessRecieveData(&codeData, len);
            auto memory = outBuffer->GetBuffer();
            memory->Write(codeData, len * channels * sizeof(short));
            auto attr = outBuffer->GetBufferAttr();
            attr.size = len * channels * sizeof(short);
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

int32_t AudioOpusDecoderPlugin::Reset()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin Reset dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    ret = PluginCodecPtr->Reset();
    if (ret != 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusDecoderPlugin::Release()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (!PluginCodecPtr) {
        AVCODEC_LOGD("AudioOpusDecoderPlugin Release dlopen or dlsym error. release");
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

int32_t AudioOpusDecoderPlugin::Flush()
{
    std::lock_guard<std::mutex> lock(avMutext_);
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin Flush dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    ret = PluginCodecPtr->Flush();
    if (ret != 0) {
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioOpusDecoderPlugin::GetInputBufferSize() const
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin GetInputBufferSize dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return PluginCodecPtr->GetInputBufferSize();
}

int32_t AudioOpusDecoderPlugin::GetOutputBufferSize() const
{
    if (!PluginCodecPtr) {
        AVCODEC_LOGE("AudioOpusDecoderPlugin GetOutputBufferSize dlopen or dlsym error");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    return PluginCodecPtr->GetOutputBufferSize();
}

Format AudioOpusDecoderPlugin::GetFormat() const noexcept
{
    return format_;
}

std::string_view AudioOpusDecoderPlugin::GetCodecType() const noexcept
{
    return AUDIO_CODEC_NAME;
}
} // namespace MediaAVCodec
} // namespace OHOS