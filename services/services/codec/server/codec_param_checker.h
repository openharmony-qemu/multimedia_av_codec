/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef CODEC_PARAM_CHECKER_H
#define CODEC_PARAM_CHECKER_H

#include "meta/format.h"
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecParamChecker {
public:
    static int32_t CheckParamValid(Media::Format &format, AVCodecType codecType, std::string codecName);
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_PARAM_CHECKER_H