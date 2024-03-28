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

#include <memory>
#include "avformat_capi_mock.h"
#include "avsource_capi_mock.h"

namespace OHOS {
namespace MediaAVCodec {
int32_t AVSourceCapiMock::Destroy()
{
    if (source_ != nullptr) {
        int32_t ret = OH_AVSource_Destroy(source_);
        source_ = nullptr;
        return ret;
    }
    return AV_ERR_UNKNOWN;
}

std::shared_ptr<FormatMock> AVSourceCapiMock::GetSourceFormat()
{
    if (source_ != nullptr) {
        OH_AVFormat *format = OH_AVSource_GetSourceFormat(source_);
        return std::make_shared<AVFormatCapiMock>(format);
    }
    return nullptr;
}

std::shared_ptr<FormatMock> AVSourceCapiMock::GetTrackFormat(uint32_t trackIndex)
{
    if (source_ != nullptr) {
        OH_AVFormat *format = OH_AVSource_GetTrackFormat(source_, trackIndex);
        if (format != nullptr) {
            return std::make_shared<AVFormatCapiMock>(format);
        }
        return nullptr;
    }
    return nullptr;
}

std::shared_ptr<FormatMock> AVSourceCapiMock::GetUserData()
{
    return nullptr;
}

OH_AVSource* AVSourceCapiMock::GetAVSource()
{
    return source_;
}
} // namespace MediaAVCodec
} // namespace OHOS