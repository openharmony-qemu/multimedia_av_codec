/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <cstddef>
#include <cstdint>
#include "avcodec_common.h"
#include "avcodec_audio_common.h"
#include "native_avcodec_audioencoder.h"
#include "common/native_mfmagic.h"
#include "native_avcodec_audiocodec.h"
#include "avcodec_audio_encoder.h"
#define FUZZ_PROJECT_NAME "audiodecoderConfigure_fuzzer"
namespace OHOS {
bool AudioAACConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *source =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, true);

    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1); //aactest

    OH_AudioCodec_Configure(source, format);
    if (source) {
        OH_AudioCodec_Destroy(source);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioFlacConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_FLAC, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_FLAC, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioMP3ConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_MPEG, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioVorbisConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_VORBIS, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioLBVCConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_LBVC, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_LBVC, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioAMRNBConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioAMRWBConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioAPEConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_APE, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_APE, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioOPUSConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_OPUS, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioG711ConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    OH_AVCodec *encodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_G711MU, false);
    if (encodersource == nullptr) {
        return false;
    }

    format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(encodersource, format);
    if (encodersource) {
        OH_AudioCodec_Destroy(encodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

bool AudioVividConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *decodersource =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_VIVID, true);
    if (decodersource == nullptr) {
        return false;
    }
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);
    OH_AVFormat *format = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);

    OH_AudioCodec_Configure(decodersource, format);
    if (decodersource) {
        OH_AudioCodec_Destroy(decodersource);
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    return true;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioAACConfigureFuzzTest(data, size);
    OHOS::AudioFlacConfigureFuzzTest(data, size);
    OHOS::AudioMP3ConfigureFuzzTest(data, size);
    OHOS::AudioVorbisConfigureFuzzTest(data, size);
    OHOS::AudioLBVCConfigureFuzzTest(data, size);
    OHOS::AudioAMRNBConfigureFuzzTest(data, size);
    OHOS::AudioAMRWBConfigureFuzzTest(data, size);
    OHOS::AudioAPEConfigureFuzzTest(data, size);
    OHOS::AudioOPUSConfigureFuzzTest(data, size);
    OHOS::AudioG711ConfigureFuzzTest(data, size);
    OHOS::AudioVividConfigureFuzzTest(data, size);
    return 0;
}
