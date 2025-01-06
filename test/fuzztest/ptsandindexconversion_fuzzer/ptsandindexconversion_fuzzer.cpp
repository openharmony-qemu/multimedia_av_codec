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
#include <iostream>
#include "ptsandindexconversion_sample.h"


#define FUZZ_PROJECT_NAME "ptsandindexconversion_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
void PtsAndIndexConversionWithFunc(const uint8_t *data, size_t size)
{
    std::shared_ptr<PtsAndIndexConversion>PtsAndIndexConversions = std::make_shared<PtsAndIndexConversion>();
    if (PtsAndIndexConversions->Init(data, size)) {
        PtsAndIndexConversions->RunNormalTimeAndIndexConversions();
    }
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::PtsAndIndexConversionWithFunc(data, size);
    return 0;
}
