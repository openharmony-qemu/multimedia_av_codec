/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "avmemory_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
uint8_t *AVMemoryCapiMock::GetAddr() const
{
    if (memory_ != nullptr) {
        return OH_AVMemory_GetAddr(memory_);
    }
    return nullptr;
}

int32_t AVMemoryCapiMock::GetSize() const
{
    if (memory_ != nullptr) {
        return OH_AVMemory_GetSize(memory_);
    }
    return -1;
}

uint32_t AVMemoryCapiMock::GetFlags() const
{
    return 0;
}
int32_t AVMemoryCapiMock::Destory()
{
    if (memory_ != nullptr) {
        return OH_AVMemory_Destroy(memory_);
    }
    return AV_ERR_OK;
}
OH_AVMemory* AVMemoryCapiMock::GetAVMemory()
{
    return memory_;
}
}  // namespace MediaAVCodec
}  // namespace OHOS