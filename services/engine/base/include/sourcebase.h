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
#ifndef SOURCEBASE_H
#define SOURCEBASE_H

#include <string>
#include "meta/format.h"

namespace OHOS {
namespace MediaAVCodec {
class SourceBase {
public:
    virtual ~SourceBase() = default;
    virtual int32_t Init(std::string& uri) = 0;
    virtual int32_t GetTrackCount(uint32_t &trackCount) = 0;
    virtual int32_t GetSourceFormat(Media::Format &format)  = 0;
    virtual int32_t GetTrackFormat(Media::Format &format, uint32_t trackIndex) = 0;
    virtual uintptr_t GetSourceAddr() = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // SOURCEBASE_H