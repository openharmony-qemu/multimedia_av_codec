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

#ifndef FCODEC_UTILS_H
#define FCODEC_UTILS_H

#include "codec_utils.h"

namespace OHOS {
namespace MediaAVCodec {
namespace Codec {
bool IsValidRawVideoPixelFormat(int32_t val)
{
    return val >= static_cast<int32_t>(RawVideoPixelFormat::YUV420P) &&
           val <= static_cast<int32_t>(RawVideoPixelFormat::RGBA);
}
} // namespace Codec
} // namespace MediaAVCodec
} // namespace OHOS
#endif // FCODEC_UTILS_H
