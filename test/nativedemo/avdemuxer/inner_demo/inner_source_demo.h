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

#ifndef INNER_SOURCE_DEMO_H
#define INNER_SOURCE_DEMO_H

#include <iostream>
#include "avsource.h"

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
class InnerSourceDemo {
public:
    InnerSourceDemo();
    ~InnerSourceDemo();
    int32_t CreateWithURI(const std::string &uri);
    size_t GetFileSize(const std::string& fileName);
    int32_t CreateWithFD(int32_t fd, int64_t offset, int64_t size);
    Format GetSourceFormat();
    Format GetTrackFormat(uint32_t trackIndex);
    std::shared_ptr<AVSource> avsource_ = nullptr;
private:
    Format source_format_;
    Format track_format_;
};
}  // namespace MediaAVCodec
}  // namespace OHOS

#endif
