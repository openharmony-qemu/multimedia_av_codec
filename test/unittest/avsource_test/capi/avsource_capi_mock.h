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

#ifndef AVSOURCE_CAPI_MOCK_H
#define AVSOURCE_CAPI_MOCK_H

#include "avsource_mock.h"
#include "avcodec_common.h"
#include "native_avsource.h"

namespace OHOS {
namespace MediaAVCodec {
class AVSourceCapiMock : public AVSourceMock {
public:
    explicit AVSourceCapiMock(OH_AVSource *source) : source_(source) {}
    ~AVSourceCapiMock() = default;
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
#endif // AVSOURCE_CAPI_MOCK_H