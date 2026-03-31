/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "mpeg4_source_inner_mock.h"

namespace OHOS {
namespace MediaAVCodec {
int32_t Mpeg4SourceInnerMock::Destroy()
{
    if (source_ != nullptr) {
        source_ = nullptr;
        return AV_ERR_OK;
    }
    return AV_ERR_UNKNOWN;
}

std::shared_ptr<FormatMock> Mpeg4SourceInnerMock::GetSourceFormat()
{
    if (source_ != nullptr) {
        Format format;
        source_->GetSourceFormat(format);
        return std::make_shared<AVFormatInnerMock>(format);
    }
    return nullptr;
}

std::shared_ptr<FormatMock> Mpeg4SourceInnerMock::GetTrackFormat(uint32_t trackIndex)
{
    if (source_ != nullptr) {
        Format format;
        source_->GetTrackFormat(format, trackIndex);
        return std::make_shared<AVFormatInnerMock>(format);
    }
    return nullptr;
}

std::shared_ptr<FormatMock> Mpeg4SourceInnerMock::GetUserData()
{
    if (source_ != nullptr) {
        Format format;
        source_->GetUserMeta(format);
        return std::make_shared<AVFormatInnerMock>(format);
    }
    return nullptr;
}

std::shared_ptr<AVSource> Mpeg4SourceInnerMock::GetAVSource()
{
    return source_;
}
} // namespace MediaAVCodec
} // namespace OHOS