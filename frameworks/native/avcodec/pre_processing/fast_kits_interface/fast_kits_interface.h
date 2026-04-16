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

#include <dlfcn.h>
#include <mutex>

#include "avcodec_errors.h"
#include "avcodec_log_ex.h"
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

// libfast_image.so 返回的错误码
constexpr int32_t FAST_ERR_SUCCESS = 0;
constexpr int32_t FAST_ERR_SUCCESS_PARTIAL = 1;
constexpr int32_t FAST_ERR_ILLEGAL_INPUT = 2;
constexpr int32_t FAST_ERR_INVALID_PTR = 3;

// libfast_image.so 导出符号的函数指针类型
using FastScaleInitFunc = int32_t (*)(uint32_t format, int32_t algo);
using FastScaleDownSampleFunc = int32_t (*)(SurfaceBuffer *input, SurfaceBuffer *output, int32_t algo);
using FastScaleCropFunc = int32_t (*)(SurfaceBuffer *input, SurfaceBuffer *output, void *rect);
using CreateCropRectFunc = void *(*)(int32_t top, int32_t left, int32_t bottom, int32_t right);
using DestroyCropRectFunc = void (*)(void *rect);

enum class FastResizeAlgoType {
    HARDWARE,
    BILINEAR,
    BICUBIC,
    LANCZOS,
};

struct CropRect {
    int32_t left{0};
    int32_t top{0};
    int32_t right{0};
    int32_t bottom{0};
};

class FastKitsInterface {
public:
    static FastKitsInterface &GetInstance();

    void Retain();
    void Release();
    int32_t Init(uint32_t format, FastResizeAlgoType algo = FastResizeAlgoType::HARDWARE);
    bool IsLoaded() const;

    int32_t Crop(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output, const CropRect &rect);
    int32_t DownSample(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                       FastResizeAlgoType algo = FastResizeAlgoType::BICUBIC);

private:
    FastKitsInterface() = default;
    ~FastKitsInterface();
    FastKitsInterface(const FastKitsInterface &) = delete;
    FastKitsInterface &operator=(const FastKitsInterface &) = delete;

    bool OpenLibrary();
    bool ReadSymbols();
    void ClearSymbols();

    template <typename FuncPtr>
    bool LoadSymbol(void *handle, FuncPtr &func, const char *name);

    static int32_t ConvertErrorCode(int32_t fastErr);
    static const char *ErrorCodeToString(int32_t fastErr);

    mutable std::mutex mutex_;
    int32_t refCount_{0};
    void *handle_ = nullptr;
    FastScaleInitFunc initFunc_ = nullptr;
    FastScaleDownSampleFunc downSampleFunc_ = nullptr;
    FastScaleCropFunc cropFunc_ = nullptr;
    CreateCropRectFunc createRectFunc_ = nullptr;
    DestroyCropRectFunc destroyRectFunc_ = nullptr;
};

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS

#endif // MEDIA_AVCODEC_FAST_KITS_INTERFACE_H
