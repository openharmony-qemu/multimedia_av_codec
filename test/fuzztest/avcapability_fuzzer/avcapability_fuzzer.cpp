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
#include "native_avcapability.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "avcapability_fuzzer"

namespace OHOS {
bool AvcapabilityFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    size_t maxSize = std::numeric_limits<size_t>::max();
    string mimeStr = fdp.ConsumRandomLengthString(maxSize);
    bool isEncoder = data[0] >> sizeof(uint8_t);
    OH_AVCodecCategory category = static_cast<OH_AVCodecCategory>(data[0] >> sizeof(uint8_t));
    OH_AVCodec_GetCapability(mime.c_str(), isEncoder);
    OH_AVCodec_GetCapabilityByCategory(mime.c_str(), isEncoder, category);
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AvcapabilityFuzzTest(data, size);
    return 0;
}
