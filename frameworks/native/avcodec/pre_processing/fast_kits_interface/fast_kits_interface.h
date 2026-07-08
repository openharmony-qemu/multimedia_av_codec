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

#ifndef MEDIA_AVCODEC_FAST_KITS_INTERFACE_H
#define MEDIA_AVCODEC_FAST_KITS_INTERFACE_H

#include <cstdint>
#include <memory>

#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

enum class FastResizeAlgoType {
    HARDWARE,
    BILINEAR,
    BICUBIC,
    LANCZOS,
};

struct CropRect {
    int32_t top{0};
    int32_t left{0};
    int32_t bottom{0};
    int32_t right{0};
};

class FastKitsInterface {
public:
    static FastKitsInterface &GetInstance();
    static bool IsFormatSupported(int32_t format);

    void Retain();
    void Release();
    int32_t Init(uint32_t format, FastResizeAlgoType algo = FastResizeAlgoType::HARDWARE);
    bool IsLoaded() const;

    int32_t Crop(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output, const CropRect &rect);
    int32_t DownSample(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                       FastResizeAlgoType algo = FastResizeAlgoType::HARDWARE);

    ~FastKitsInterface();

private:
    FastKitsInterface();
    FastKitsInterface(const FastKitsInterface &) = delete;
    FastKitsInterface &operator=(const FastKitsInterface &) = delete;

    class Impl;
    std::unique_ptr<Impl> impl_;
};

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // MEDIA_AVCODEC_FAST_KITS_INTERFACE_H
