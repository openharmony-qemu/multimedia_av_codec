/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef POST_PROCESSING_UTILS_H
#define POST_PROCESSING_UTILS_H

#include <type_traits>
#include "avcodec_log.h"

constexpr OHOS::HiviewDFX::HiLogLabel LogLabel(const char* tag = "PostProcessing")
{
    auto label = OHOS::HiviewDFX::HiLogLabel{LOG_CORE, LOG_DOMAIN_FRAMEWORK, tag};
    return label;
}

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

struct CapabilityInfo {
    int32_t colorSpaceType;
    int32_t metadataType;
    int32_t pixelFormat;
};

template<typename... Types>
struct TypeArray {
    using Array = std::tuple<Types...>;

    template<size_t I>
    using Get = std::tuple_element_t<I, Array>;

    static constexpr size_t size = sizeof...(Types);
};

template<typename T, T V, typename = std::enable_if_t<std::is_enum_v<T>>>
struct EnumerationValue {
    using UnderlyingType = std::underlying_type_t<T>;
    static constexpr UnderlyingType value = static_cast<UnderlyingType>(V);
};

} // namespace PostProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // POST_PROCESSING_UTILS_H