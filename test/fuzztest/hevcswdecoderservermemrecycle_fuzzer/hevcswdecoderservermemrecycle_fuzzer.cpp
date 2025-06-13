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
#include "hevcserverdec_sample.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
#define FUZZ_PROJECT_NAME "swdecoderservernormal_fuzzer"
const size_t EXPECT_SIZE = 6;
const size_t WIDTH_SIZE = 1;
const size_t HEIGHT_SIZE = 2;
const size_t FRAME_RATE_SIZE = 3;
const size_t ROTATION_SIZE = 4;
const size_t PIXELFORMAT_SIZE = 5;

namespace OHOS {
bool SwdecoderServerNormalFuzzTest(const uint8_t *data, size_t size)
{
    if (data == nullptr) {
        return false;
    }
    if (size < EXPECT_SIZE) {
        return false;
    }
    VDecServerSample *vDecSample = new VDecServerSample();
    vDecSample->defaultWidth = data[size - WIDTH_SIZE];
    vDecSample->defaultHeight = data[size - HEIGHT_SIZE];
    vDecSample->defaultFrameRate = data[size - FRAME_RATE_SIZE];
    vDecSample->defaultRotation = data[size - ROTATION_SIZE];
    vDecSample->defaultPixelFormat = data[size - PIXELFORMAT_SIZE];
    if (vDecSample->defaultRotation % 2 == 0) {
        vDecSample->inpDir = "/data/test/media/720_1280_25_avcc.h265";
    } else {
        vDecSample->inpDir = "/data/test/media/720_1280_25_avcc.hdr.h265";
    }
    if (vDecSample->defaultPixelFormat % 2 == 0) {
        vDecSample->isSurfMode = true;
    } else {
        vDecSample->isSurfMode = false;
    }
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
    /* Run your code on data */
    OHOS::SwdecoderServerNormalFuzzTest(data, size);
    return 0;
}
