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

#ifndef CODEC_FACTORY_H
#define CODEC_FACTORY_H

#include <memory>
#include "codecbase.h"
#include "avcodec_errors.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecFactory {
public:
    static CodecFactory &Instance();
    std::shared_ptr<CodecBase> CreateCodecByMime(bool isEncoder, const std::string &mime,
        API_VERSION apiVersion, std::string &codecName);
    std::shared_ptr<CodecBase> CreateCodecByName(const std::string &name, API_VERSION apiVersion);

private:
    CodecFactory() = default;
    ~CodecFactory();
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODEC_FACTORY_H
