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
#include <cstddef>
#include <cstdint>
#include <fuzzer/FuzzedDataProvider.h>
#include "vp8serverdec_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "vp8swdecoderservermemrecycle_fuzzer"
const size_t EXPECT_SIZE = 26;

namespace OHOS {
bool Vp8SwdecoderServerMemrecycleFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    Vp8VDecServerSample *vDecSample = new Vp8VDecServerSample();
    vDecSample->defaultWidth = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultHeight = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultFrameRate = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultRotation = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->defaultPixelFormat = fdp.ConsumeIntegral<uint32_t>();
    vDecSample->inpDir = "/data/test/media/test.vp8";
    vDecSample->isSurfMode = fdp.ConsumeBool();
    auto remaining_data = fdp.ConsumeRemainingBytes<uint8_t>();
    vDecSample->fuzzData = remaining_data.data();
    vDecSample->fuzzSize = remaining_data.size();
    vDecSample->RunVideoServerDecoder();
    vDecSample->NotifyMemoryRecycle();
    vDecSample->NotifyMemoryRecycle();
    vDecSample->NotifyMemoryWriteBack();
    vDecSample->NotifyMemoryWriteBack();
    vDecSample->WaitForEos();
    delete vDecSample;
    return false;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
#ifdef SUPPORT_CODEC_VP8
    /* Run your code on data */
    OHOS::Vp8SwdecoderServerMemrecycleFuzzTest(data, size);
#else
    (void)data;
    (void)size;
#endif
    return 0;
}
