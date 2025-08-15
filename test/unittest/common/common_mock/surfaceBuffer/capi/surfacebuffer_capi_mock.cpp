/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "surfacebuffer_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<SurfaceBufferMock> SurfaceBufferMockFactory::CreateSurfaceBuffer(
    std::shared_ptr<AVBufferMock> &avBufferMock)
{
    return std::make_shared<SurfaceBufferCapiMock>(avBufferMock);
}

SurfaceBufferCapiMock::~SurfaceBufferCapiMock()
{
    if (nativeBuffer_ != nullptr) {
        OH_NativeBuffer_Unreference(nativeBuffer_);
        nativeBuffer_ = nullptr;
    }
}

bool SurfaceBufferCapiMock::GetHDRDynamicMetadata(std::vector<uint8_t> &meta)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nativeBuffer_ != nullptr, false, "nativeBuffer_ is nullptr!");
    int32_t size = 0;
    uint8_t **metadata = nullptr;
    if (OH_NativeBuffer_GetMetadataValue(nativeBuffer_, OH_HDR_DYNAMIC_METADATA, &size, metadata) != 0) {
        return false;
    }
    meta.resize(size);
    int32_t ret = memcpy_s(&meta[0], size, *metadata, size);
    delete[] *metadata;
    *metadata = nullptr;
    if (ret == 0) {
        return true;
    }
    return false;
}

bool SurfaceBufferCapiMock::GetHDRStaticMetadata(std::vector<uint8_t> &meta)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(nativeBuffer_ != nullptr, false, "nativeBuffer_ is nullptr!");
    int32_t size = 0;
    uint8_t **metadata = nullptr;
    if (OH_NativeBuffer_GetMetadataValue(nativeBuffer_, OH_HDR_STATIC_METADATA, &size, metadata) != 0) {
        return false;
    }
    meta.resize(size);
    int32_t ret = memcpy_s(&meta[0], size, *metadata, size);
    delete[] *metadata;
    *metadata = nullptr;
    if (ret == 0) {
        return true;
    }
    return false;
}
} // namespace MediaAVCodec
} // namespace OHOS