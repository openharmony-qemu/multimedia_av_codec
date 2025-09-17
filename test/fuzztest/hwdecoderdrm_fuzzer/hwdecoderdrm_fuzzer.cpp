/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "securec.h"
#include <cstddef>
#include <cstdint>
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_drm_object.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
#define FUZZ_PROJECT_NAME "hwdecoderdrm_fuzzer"

bool HwdecoderDrmFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    size_t maxSize = std::numeric_limits<size_t>::max();
    string mimeStr = fdp.ConsumeRandomLengthString(maxSize);
    bool secureVideoPath = fdp.ConsumeBool();

    OH_AVCodec *codec = nullptr;
    codec = OH_VideoDecoder_CreateByName(mimeStr.c_str());
    MediaKeySession *mediaKeySession = nullptr;
    OH_VideoDecoder_SetDecryptionConfig(codec, mediaKeySession, secureVideoPath);
    return true;
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    HwdecoderDrmFuzzTest(data, size);
    return 0;
}
