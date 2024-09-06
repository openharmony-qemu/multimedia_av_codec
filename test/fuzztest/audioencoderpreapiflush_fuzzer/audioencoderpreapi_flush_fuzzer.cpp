/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "audioencoderdemo.h"
#define FUZZ_PROJECT_NAME "audioencoderpreapiflush_fuzzer"

using namespace OHOS::MediaAVCodec::AudioEncDemoAuto;

namespace OHOS {
bool AudioEncoderAACFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ aac
    AEncDemoAuto* aDecBufferDemo = new AEncDemoAuto();
    aDecBufferDemo->InitFile("aac");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderOPUSFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ opus
    AEncDemoAuto* aDecBufferDemo = new AEncDemoAuto();
    aDecBufferDemo->InitFile("opus");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderG711FlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ g711
    AEncDemoAuto* aDecBufferDemo = new AEncDemoAuto();
    aDecBufferDemo->InitFile("g711");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderFLACFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ flac
    AEncDemoAuto* aDecBufferDemo = new AEncDemoAuto();
    aDecBufferDemo->InitFile("flac");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioEncoderAACFlushFuzzTest(data, size);
    OHOS::AudioEncoderG711FlushFuzzTest(data, size);
    OHOS::AudioEncoderOPUSFlushFuzzTest(data, size);
    OHOS::AudioEncoderFLACFlushFuzzTest(data, size);
    return 0;
}
