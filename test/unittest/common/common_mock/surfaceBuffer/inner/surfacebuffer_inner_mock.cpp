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

#include "surfacebuffer_inner_mock.h"
#include "ui/rs_surface_node.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<SurfaceBufferMock> SurfaceBufferMockFactory::CreateSurfaceBuffer(
    std::shared_ptr<AVBufferMock> &avBufferMock)
{
    return std::make_shared<SurfaceBufferInnerMock>(avBufferMock);
}

SurfaceBufferInnerMock::~SurfaceBufferInnerMock() {}

bool SurfaceBufferInnerMock::GetHDRDynamicMetadata(std::vector<uint8_t> &meta)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, false, "surfaceBuffer_ is nullptr!");
    if (surfaceBuffer_->GetMetadata(ATTRKEY_HDR_DYNAMIC_METADATA, meta) == 0) {
        return true;
    } else {
        return false;
    }
}

bool SurfaceBufferInnerMock::GetHDRStaticMetadata(std::vector<uint8_t> &meta)
{
    UNITTEST_CHECK_AND_RETURN_RET_LOG(surfaceBuffer_ != nullptr, false, "surfaceBuffer_ is nullptr!");
    if (surfaceBuffer_->GetMetadata(ATTRKEY_HDR_STATIC_METADATA, meta) == 0) {
        return true;
    } else {
        return false;
    }
}
} // namespace MediaAVCodec
} // namespace OHOS