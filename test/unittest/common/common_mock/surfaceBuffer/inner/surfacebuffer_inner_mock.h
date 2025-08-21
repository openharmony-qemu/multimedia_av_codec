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

#ifndef SURFACEBUFFER_INNER_MOCK_H
#define SURFACEBUFFER_INNER_MOCK_H

#include "surfaceBuffer_mock.h"
#include "surface_buffer.h"

namespace OHOS {
namespace MediaAVCodec {
class SurfaceBufferInnerMock : public SurfaceBufferMock {
public:
    explicit SurfaceBufferInnerMock(std::shared_ptr<AVBufferMock> &avBufferMock) {
        surfaceBuffer_ = avBufferMock->GetNativeBuffer();
    }
    SurfaceBufferInnerMock() = default;
    ~SurfaceBufferInnerMock();

    bool GetHDRDynamicMetadata(std::vector<uint8_t> &meta) override;
    bool GetHDRStaticMetadata(std::vector<uint8_t> &meta) override;
    bool GetHDRMetadataType(int &hdrType) override;
private:
    sptr<SurfaceBuffer> surfaceBuffer_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SURFACEBUFFER_INNER_MOCK_H