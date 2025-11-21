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

#include "surface_inner_mock.h"
#include "ui/rs_surface_node.h"

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<SurfaceMock> SurfaceMockFactory::CreateSurface(sptr<Surface> &surface)
{
    return std::make_shared<SurfaceInnerMock>(surface);
}

SurfaceInnerMock::~SurfaceInnerMock() {}
sptr<Surface> SurfaceInnerMock::GetSurface()
{
    return surface_;
}

void SurfaceInnerMock::SetTransform(int32_t transform)
{
    surface_->SetTransform(static_cast<GraphicTransformType>(transform));
}

void SurfaceInnerMock::GetTransform(int32_t &transform)
{
    transform = static_cast<int32_t>(surface_->GetTransform());
}
} // namespace MediaAVCodec
} // namespace OHOS