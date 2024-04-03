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
#define FUZZ_PROJECT_NAME "audioencoder_fuzzer"
namespace OHOS {
bool AudioEncoderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    OH_AVCodec *source =  OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_AAC, true);
    OH_AVFormat *format = OH_AVFormat_Create();
    int32_t intData = *reinterpret_cast<const int32_t *>(data);
    int64_t longData = *reinterpret_cast<const int64_t *>(data);

    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, intData);
    OH_AVFormat_SetLongValue(format, OH_MD_KEY_BITRATE, longData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITS_PER_CODED_SAMPLE, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_COMPLIANCE_LEVEL, intData);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_AAC_IS_ADTS, 1);//aac

    OH_AudioCodec_SetParameter(source, format);
    if (source) {
        OH_AudioCodec_Destroy(source);
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioEncoderFuzzTest(data, size);
    return 0;
}
