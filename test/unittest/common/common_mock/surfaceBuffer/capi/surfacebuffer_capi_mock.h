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

#ifndef SURFACEBUFFER_CAPI_MOCK_H
#define SURFACEBUFFER_CAPI_MOCK_H

#include "surfaceBuffer_mock.h"
#include "native_avcodec_base.h"
#include "native_buffer.h"
#include "native_avbuffer.h"
#include "avbuffer_capi_mock.h"
namespace OHOS {
namespace MediaAVCodec {
class SurfaceBufferCapiMock : public SurfaceBufferMock {
public:
    explicit SurfaceBufferCapiMock(std::shared_ptr<AVBufferMock> &avBufferMock) {
        nativeBuffer_ = OH_AVBuffer_GetNativeBuffer(std::static_pointer_cast<AVBufferCapiMock>(
            avBufferMock)->GetAVBuffer());
    }
    SurfaceBufferCapiMock() = default;
    ~SurfaceBufferCapiMock();

    bool GetHDRDynamicMetadata(std::vector<uint8_t> &meta) override;
    bool GetHDRStaticMetadata(std::vector<uint8_t> &meta) override;

private:
    OH_NativeBuffer *nativeBuffer_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SURFACEBUFFER_CAPI_MOCK_H