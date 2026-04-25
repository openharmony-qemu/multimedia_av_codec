/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#ifndef CPU_IMAGE_PROCESSOR_H
#define CPU_IMAGE_PROCESSOR_H

#include <cstdint>
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

enum class CpuImageOperation {
    COPY,
    CROP,
    DOWNSAMPLE,
};

struct CpuImageCropRect {
    int32_t left{0};
    int32_t top{0};
    int32_t width{0};
    int32_t height{0};
};

struct CpuImageProcessRequest {
    sptr<SurfaceBuffer> input;
    sptr<SurfaceBuffer> output;
    CpuImageOperation operation{CpuImageOperation::COPY};
    CpuImageCropRect crop;
};

class CpuImageProcessor {
public:
    static bool IsFormatSupported(int32_t format);
    static int32_t Process(const CpuImageProcessRequest &request);
};

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // CPU_IMAGE_PROCESSOR_H
