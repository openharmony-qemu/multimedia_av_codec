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

#ifndef SURFACEBUFFER_MOCK_H
#define SURFACEBUFFER_MOCK_H

#include <string>
#include "avbuffer_mock.h"
#include "nocopyable.h"
#include "unittest_log.h"
namespace OHOS {
namespace MediaAVCodec {

class SurfaceBufferMock : public NoCopyable {
public:
    virtual ~SurfaceBufferMock() = default;
    virtual bool GetHDRDynamicMetadata(std::vector<uint8_t> &meta) = 0;
    virtual bool GetHDRStaticMetadata(std::vector<uint8_t> &meta) = 0;
};

class __attribute__((visibility("default"))) SurfaceBufferMockFactory {
public:
    static std::shared_ptr<SurfaceBufferMock> CreateSurfaceBuffer(std::shared_ptr<AVBufferMock> &avBufferMock);

private:
    SurfaceBufferMockFactory() = delete;
    ~SurfaceBufferMockFactory() = delete;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SURFACEBUFFER_MOCK_H