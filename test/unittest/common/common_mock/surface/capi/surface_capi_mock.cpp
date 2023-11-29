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

#include "surface_capi_mock.h"
#include "native_window.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<SurfaceMock> SurfaceMockFactory::CreateSurface()
{
    return std::make_shared<SurfaceCapiMock>();
}

std::shared_ptr<SurfaceMock> SurfaceMockFactory::CreateSurface(sptr<Surface> &surface)
{
    OHNativeWindow *window = CreateNativeWindowFromSurface(&surface);
    return std::make_shared<SurfaceCapiMock>(window);
}

SurfaceCapiMock::~SurfaceCapiMock()
{
    if ((nativeWindow_ != nullptr) && (nativeWindow_->GetSptrRefCount() >= 1)) {
        DestoryNativeWindow(nativeWindow_);
        nativeWindow_ = nullptr;
    }
}

OHNativeWindow *SurfaceCapiMock::GetSurface()
{
    return nativeWindow_;
}
} // namespace MediaAVCodec
} // namespace OHOS