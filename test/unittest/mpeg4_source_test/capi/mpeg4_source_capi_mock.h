/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_SOURCE_CAPI_MOCK_H
#define MPEG4_SOURCE_CAPI_MOCK_H

#include "mpeg4_source_mock.h"
#include "avcodec_common.h"
#include "native_avsource.h"

namespace OHOS {
namespace MediaAVCodec {
class Mpeg4SourceCapiMock : public Mpeg4SourceMock {
public:
    explicit Mpeg4SourceCapiMock(OH_AVSource *source) : source_(source) {}
    ~Mpeg4SourceCapiMock() = default;
    int32_t Destroy() override;
    std::shared_ptr<FormatMock> GetSourceFormat() override;
    std::shared_ptr<FormatMock> GetTrackFormat(uint32_t trackIndex) override;
    std::shared_ptr<FormatMock> GetUserData() override;
    OH_AVSource* GetAVSource();
private:
    OH_AVSource* source_ = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MPEG4_SOURCE_CAPI_MOCK_H