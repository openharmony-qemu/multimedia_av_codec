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
    // FUZZ OH_AudioCodec_CreateByMime
    std::string codecdata((const char*) data, size);
    OH_AVCodec *source =  OH_AudioCodec_CreateByMime(codecdata.c_str(), true);
    if (source) {
        OH_AudioCodec_Destroy(source);
    }
    OH_AVCodec *sourcename =  OH_AudioCodec_CreateByName(codecdata.c_str());
    if (sourcename) {
        OH_AudioCodec_Destroy(sourcename);
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
