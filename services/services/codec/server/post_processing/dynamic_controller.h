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

#ifndef POST_PROCESSING_DYNAMIC_CONTROLLER_H
#define POST_PROCESSING_DYNAMIC_CONTROLLER_H

#include "controller.h"
#include "dynamic_interface.h"
#include "utils.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

class DynamicController : public Controller<DynamicController> {
public:
    ~DynamicController();

    bool LoadInterfacesImpl();
    void UnloadInterfacesImpl();

    bool IsColorSpaceConversionSupportedImpl(const CapabilityInfo& input, const CapabilityInfo& output);
    int32_t CreateImpl();
    void DestroyImpl();
    int32_t SetCallbackImpl(void* callback, void* userData);
    int32_t SetOutputSurfaceImpl(sptr<Surface> surface);
    int32_t CreateInputSurfaceImpl(sptr<Surface>& surface);
    int32_t SetParameterImpl(Media::Format& parameter);
    int32_t GetParameterImpl(Media::Format& parameter);
    int32_t ConfigureImpl(Media::Format& config);
    int32_t PrepareImpl();
    int32_t StartImpl();
    int32_t StopImpl();
    int32_t FlushImpl();
    int32_t GetOutputFormatImpl(Media::Format& format);
    int32_t ResetImpl();
    int32_t ReleaseImpl();
    int32_t ReleaseOutputBufferImpl(uint32_t index, bool render);

private:
    GSError OnProducerBufferReleased(sptr<SurfaceBuffer> &buffer);

    bool ready_{false};
    DynamicInterface interface_;
    DynamicColorSpaceConverterHandle* instance_{nullptr};
    static constexpr HiviewDFX::HiLogLabel LABEL{LogLabel("DynamicController")};
};

} // namespace PostProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // POST_PROCESSING_DYNAMIC_CONTROLLER_H