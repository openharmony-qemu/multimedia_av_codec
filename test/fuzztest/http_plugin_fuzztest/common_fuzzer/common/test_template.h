/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#ifndef FUZZTEST_COMMON_GET_DATA_HPP
#define FUZZTEST_COMMON_GET_DATA_HPP

#include "securec.h"
#include "hilog/log_cpp.h"

namespace OHOS {
    inline uint8_t *g_baseFuzzData = nullptr;
    inline size_t g_baseFuzzSize = 0;
    inline size_t g_baseFuzzPos = 0;
    template<typename T> T GetData()
    {
        T object{};
        size_t objectSize = sizeof(object);
        if (g_baseFuzzData == nullptr || objectSize > (g_baseFuzzSize - g_baseFuzzPos)) {
            return object;
        }
        errno_t ret = memcpy_s(&object, objectSize, g_baseFuzzData + g_baseFuzzPos, objectSize);
        if (ret != EOK) {
            return {};
        }
        g_baseFuzzPos += objectSize;
        return object;
    }
}
#endif