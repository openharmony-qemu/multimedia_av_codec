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

#ifndef POST_PROCESSING_CONFIGURATION_H
#define POST_PROCESSING_CONFIGURATION_H

#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PostProcessing {

struct Configuration {
    int32_t width;
    int32_t height;
    int32_t inputColorSpaceType;
    int32_t inputMetadataType;
    int32_t inputPixelFormat;
    sptr<Surface> inputSurface;
    int32_t outputColorSpaceType;
    int32_t outputMetadataType;
    int32_t outputPixelFormat;
    sptr<Surface> outputSurface;
    int32_t rotation;
    int32_t scalingMode;
};

} // namespace PostProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // POST_PROCESSING_CONFIGURATION_H