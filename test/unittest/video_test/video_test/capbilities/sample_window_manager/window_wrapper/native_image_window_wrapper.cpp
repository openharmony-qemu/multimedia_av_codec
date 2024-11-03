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

#include "native_image_window_wrapper.h"
#include "native_image.h"
#include "external_window.h"
#include "av_codec_sample_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, "NativeImageWindowWrapper"};
}

namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
void OnFrameAvailable(void *context) {
    CHECK_AND_RETURN_LOG(context != nullptr, "Context is nullptr");
    OH_NativeImage *image = static_cast<NativeImageWindowWrapper *>(context)->GetNativeImage().get();

    int32_t fenceId = -1;
    OHNativeWindowBuffer *nativeWindowBuffer = nullptr;
    OH_NativeImage_AcquireNativeWindowBuffer(image, &nativeWindowBuffer, &fenceId);

    OH_NativeImage_ReleaseNativeWindowBuffer(image, nativeWindowBuffer, fenceId);
};

NativeImageWindowWrapper::NativeImageWindowWrapper()
{
    windowType_ = SampleWindowType::NATIVE_IMAGE;

    image_ = std::shared_ptr<OH_NativeImage>(OH_ConsumerSurface_Create(),
        [](OH_NativeImage *image) -> void {
            OH_NativeImage_Destroy(&image);
        }
    );
    window_ = std::shared_ptr<OHNativeWindow>(
        OH_NativeImage_AcquireNativeWindow(image_.get()), OH_NativeWindow_DestroyNativeWindow);

    OH_OnFrameAvailableListener listener {this, OnFrameAvailable};
    OH_NativeImage_SetOnFrameAvailableListener(image_.get(), listener);
}

std::shared_ptr<OH_NativeImage> NativeImageWindowWrapper::GetNativeImage()
{
    return image_;
}
} // Sample
} // MediaAVCodec
} // OHOS