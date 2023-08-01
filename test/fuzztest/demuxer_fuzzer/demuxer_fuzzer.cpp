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
#include "native_avdemuxer.h"
#include "native_avsource.h"
#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
namespace OHOS {
bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ CreateFD
    int32_t fd = *reinterpret_cast<const int32_t *>(data);
    int64_t offset = *reinterpret_cast<const int64_t *>(data);
    int64_t fileSize = *reinterpret_cast<const int64_t *>(data);
    OH_AVSource *source = OH_AVSource_CreateWithFD(fd, offset, fileSize);
    if (source) {
        OH_AVSource_Destroy(source);
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
