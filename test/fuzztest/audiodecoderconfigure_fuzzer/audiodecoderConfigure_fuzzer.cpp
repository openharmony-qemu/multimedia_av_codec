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
#include <fuzzer/FuzzedDataProvider.h>
#define FUZZ_PROJECT_NAME "audiodecoderConfigure_fuzzer"
namespace OHOS {
const size_t THRESHOLD = 10;
static const uint8_t* RAW_DATA = nullptr;
static size_t g_dataSize = 0;
static size_t g_pos;
typedef void (*TestFuncs)();

template<class T>
T GetData()
{
    T object {};
    size_t objectSize = sizeof(object);
    if (RAW_DATA == nullptr || objectSize > g_dataSize - g_pos) {
        return object;
    }
    errno_t ret = memcpy_s(&object, objectSize, RAW_DATA + g_pos, objectSize);
    if (ret != EOK) {
        return {};
    }
    g_pos += objectSize;
    return object;
}

template<class T>
uint32_t GetArrLength(T& arr)
{
    if (arr == nullptr) {
        return 0;
    }
    return sizeof(arr) / sizeof(arr[0]);
}
bool AudioAACConfigureFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *source =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, true);

    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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
    FuzzedDataProvider fdp(data, size);
    int32_t intData = fdp.ConsumeIntegral<int32_t>();
    int64_t longData = fdp.ConsumeIntegral<int64_t>();
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

TestFuncs g_testFuncs[] = {
    AudioAACConfigureFuzzTest,
    AudioFlacConfigureFuzzTest,
    AudioMP3ConfigureFuzzTest,
    AudioVorbisConfigureFuzzTest,
    AudioLBVCConfigureFuzzTest,
    AudioAMRNBConfigureFuzzTest,
    AudioAMRWBConfigureFuzzTest,
    AudioAPEConfigureFuzzTest,
    AudioOPUSConfigureFuzzTest,
    AudioG711ConfigureFuzzTest,
    AudioVividConfigureFuzzTest,
};

bool FuzzTest(const uint8_t* rawData, size_t size)
{
    if (rawData == nullptr) {
        return false;
    }

    // initialize data
    RAW_DATA = rawData;
    g_dataSize = size;
    g_pos = 0;

    uint32_t code = GetData<uint32_t>();
    uint32_t len = GetArrLength(g_testFuncs);
    if (len > 0) {
        g_testFuncs[code % len]();
    } else {
        return false;
    }

    return true;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    if (size < OHOS::THRESHOLD) {
        return 0;
    }

    OHOS::FuzzTest(data, size);
    return 0;
}