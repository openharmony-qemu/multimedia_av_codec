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

#ifndef POST_PROCESSING_CONTROLLER_H
#define POST_PROCESSING_CONTROLLER_H

#include <type_traits>
#include "surface.h"
#include "avcodec_errors.h"
#include "meta/format.h"
#include "utils.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

template<typename T>
class Controller {
public:
    virtual ~Controller() = default;

    bool LoadInterfaces()
    {
        return This()->LoadInterfacesImpl();
    }

    void UnloadInterfaces()
    {
        return This()->UnloadInterfacesImpl();
    }

    bool IsColorSpaceConversionSupported(const CapabilityInfo& input, const CapabilityInfo& output)
    {
        return This()->IsColorSpaceConversionSupportedImpl(input, output);
    }

    int32_t Create()
    {
        return This()->CreateImpl();
    }

    void Destroy()
    {
        return This()->DestroyImpl();
    }

    int32_t SetCallback(void* callback, void* userData)
    {
        return This()->SetCallbackImpl(callback, userData);
    }

    int32_t SetOutputSurface(sptr<Surface> surface)
    {
        return This()->SetOutputSurfaceImpl(surface);
    }

    int32_t CreateInputSurface(sptr<Surface>& surface)
    {
        return This()->CreateInputSurfaceImpl(surface);
    }

    int32_t SetParameter(Media::Format& parameter)
    {
        return This()->SetParameterImpl(parameter);
    }

    int32_t GetParameter(Media::Format& parameter)
    {
        return This()->GetParameterImpl(parameter);
    }

    int32_t Configure(Media::Format& config)
    {
        return This()->ConfigureImpl(config);
    }

    int32_t Prepare()
    {
        return This()->PrepareImpl();
    }

    int32_t Start()
    {
        return This()->StartImpl();
    }

    int32_t Stop()
    {
        return This()->StopImpl();
    }

    int32_t Flush()
    {
        return This()->FlushImpl();
    }

    int32_t GetOutputFormat(Media::Format& format)
    {
        return This()->GetOutputFormatImpl(format);
    }

    int32_t Reset()
    {
        return This()->ResetImpl();
    }

    int32_t Release()
    {
        return This()->ReleaseImpl();
    }

    int32_t ReleaseOutputBuffer(uint32_t index, bool render)
    {
        return This()->ReleaseOutputBufferImpl(index, render);
    }
private:
    T* This()
    {
        return static_cast<T*>(this);
    }

    static constexpr HiviewDFX::HiLogLabel LABEL{LogLabel("Controller")};
};

template<typename T>
using IsDerivedController = std::enable_if_t<
    std::conjunction_v<
        std::is_base_of<Controller<T>, T>,
        std::is_convertible<const volatile T*, const volatile Controller<T>*>
    >
>;

} // namespace OHOS
} // namespace MediaAVCodec
} // namespace PostProcessing

#endif // POST_PROCESSING_CONTROLLER_H