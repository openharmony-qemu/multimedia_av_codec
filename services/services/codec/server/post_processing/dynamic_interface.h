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

#ifndef POST_PROCESSING_DYNAMIC_INTERFACE_H
#define POST_PROCESSING_DYNAMIC_INTERFACE_H

#include <array>
#include <type_traits>
#include "avcodec_errors.h"
#include "utils.h"
#include "dynamic_interface_types.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

class DynamicInterface {
public:
    ~DynamicInterface();

    bool Load();
    void Unload();

    template<DynamicInterfaceName E, typename... Args, typename RetT = DynamicInterfaceReturnType<E, Args...>>
    RetT Invoke(Args&& ... args)
    {
        constexpr DynamicInterfaceIndexType<E> I = DynamicInterfaceIndexValue<E>;
        AVCODEC_LOGD("Invoke %{public}s", DYNAMIC_INTERFACE_SYMBOLS[I]);
        auto interface = reinterpret_cast<DynamicInterfaceFuncTypes::Get<I>>(interfaces_[I]);
        if constexpr (std::is_void_v<RetT>) {
            CHECK_AND_RETURN_LOG(interface != nullptr, "Interface not found.");
            interface(std::forward<Args>(args)...);
        } else if constexpr (std::is_pointer_v<RetT>) {
            CHECK_AND_RETURN_RET_LOG(interface != nullptr, nullptr, "Interface not found.");
            return interface(std::forward<Args>(args)...);
        } else if constexpr (std::is_integral_v<RetT>) {
            CHECK_AND_RETURN_RET_LOG(interface != nullptr, AVCS_ERR_INVALID_VAL, "Interface not found.");
            return interface(std::forward<Args>(args)...);
        }
    }
private:
    bool OpenLibrary();
    void CloseLibrary();
    bool ReadSymbols();
    void ClearSymbols();

    void* lib_{nullptr};
    std::array<void*, DYNAMIC_INTERFACE_NUM> interfaces_{nullptr};

    static constexpr HiviewDFX::HiLogLabel LABEL{LogLabel("DynamicInterface")};
};

} // namespace PostProcessing
} // namespace MediaAVCodec
} // OHOS

#endif // POST_PROCESSING_DYNAMIC_INTERFACE_H