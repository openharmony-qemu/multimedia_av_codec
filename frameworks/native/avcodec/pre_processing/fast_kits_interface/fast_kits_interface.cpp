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

#include "fast_kits_interface.h"

#include <dlfcn.h>
#include <shared_mutex>
#include <unordered_map>
#include <unordered_set>

#include "avcodec_errors.h"
#include "avcodec_log_ex.h"
#include "hitrace_meter.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "FastKitsInterface"};
constexpr const char *FAST_IMAGE_SO_PATH = "libfast_image.so";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

namespace {
enum FastErrCode {
    FAST_ERR_SUCCESS = 0,
    FAST_ERR_SUCCESS_PARTIAL = 1,
    FAST_ERR_ILLEGAL_INPUT = 2,
    FAST_ERR_INVALID_PTR = 3,
    FAST_ERR_UNSURPPORTED = 4,
};

using FastScaleInitFunc = int32_t (*)(uint32_t format, int32_t algo);
using FastScaleDownSampleFunc = int32_t (*)(SurfaceBuffer *input, SurfaceBuffer *output, int32_t algo);
using FastScaleCropFunc = int32_t (*)(SurfaceBuffer *input, SurfaceBuffer *output, void *rect);
using CreateCropRectFunc = void *(*)(int32_t top, int32_t left, int32_t bottom, int32_t right);
using DestroyCropRectFunc = void (*)(void *rect);
using FastScaleClearGPUFunc = void (*)();

template <typename FuncPtr>
bool LoadSymbol(void *handle, FuncPtr &func, const char *name)
{
    func = reinterpret_cast<FuncPtr>(dlsym(handle, name));
    if (func == nullptr) {
        AVCODEC_LOGE("Failed to find %{public}s: %{public}s", name, dlerror());
        return false;
    }
    return true;
}

int32_t ConvertErrorCode(int32_t fastErr)
{
    static const std::unordered_map<int32_t, int32_t> errMap = {
        {FAST_ERR_SUCCESS, AVCS_ERR_OK},
        {FAST_ERR_SUCCESS_PARTIAL, AVCS_ERR_OK},
        {FAST_ERR_ILLEGAL_INPUT, AVCS_ERR_INVALID_VAL},
        {FAST_ERR_INVALID_PTR, AVCS_ERR_INVALID_VAL},
        {FAST_ERR_UNSURPPORTED, AVCS_ERR_UNSUPPORT},
    };
    auto it = errMap.find(fastErr);
    return it != errMap.end() ? it->second : AVCS_ERR_UNKNOWN;
}
} // namespace

class FastKitsInterface::Impl {
public:
    ~Impl();
    void Retain();
    void Release();
    int32_t Init(uint32_t format, FastResizeAlgoType algo);
    bool IsLoaded() const;
    int32_t Crop(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output, const CropRect &rect);
    int32_t DownSample(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output, FastResizeAlgoType algo);

private:
    bool OpenLibrary();
    bool ReadSymbols();
    void ClearSymbols();

    mutable std::shared_mutex mutex_;
    int32_t refCount_ = 0;
    void *handle_ = nullptr;
    std::unordered_set<uint32_t> initializedFormats_;
    FastScaleInitFunc initFunc_ = nullptr;
    FastScaleDownSampleFunc downSampleFunc_ = nullptr;
    FastScaleClearGPUFunc clearGPUFunc_ = nullptr;
    FastScaleCropFunc cropFunc_ = nullptr;
    CreateCropRectFunc createCropRectFunc_ = nullptr;
    DestroyCropRectFunc destroyCropRectFunc_ = nullptr;
};

FastKitsInterface::FastKitsInterface() : impl_(std::make_unique<Impl>()) {}

FastKitsInterface::~FastKitsInterface() = default;

FastKitsInterface &FastKitsInterface::GetInstance()
{
    static FastKitsInterface instance;
    return instance;
}

bool FastKitsInterface::IsFormatSupported(int32_t format)
{
    switch (format) {
        case GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
            return true;
        default:
            return false;
    }
}

void FastKitsInterface::Retain()
{
    impl_->Retain();
}

void FastKitsInterface::Release()
{
    impl_->Release();
}

int32_t FastKitsInterface::Init(uint32_t format, FastResizeAlgoType algo)
{
    return impl_->Init(format, algo);
}

bool FastKitsInterface::IsLoaded() const
{
    return impl_->IsLoaded();
}

int32_t FastKitsInterface::Crop(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                const CropRect &rect)
{
    return impl_->Crop(input, output, rect);
}

int32_t FastKitsInterface::DownSample(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                      FastResizeAlgoType algo)
{
    return impl_->DownSample(input, output, algo);
}

FastKitsInterface::Impl::~Impl()
{
    if (handle_ != nullptr) {
        ClearSymbols();
        dlclose(handle_);
        handle_ = nullptr;
    }
}

void FastKitsInterface::Impl::Retain()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    refCount_++;
    if (refCount_ == 1 && handle_ == nullptr) {
        if (OpenLibrary() && ReadSymbols()) {
            AVCODEC_LOGI("fastkit lib loaded");
        } else {
            if (handle_ != nullptr) {
                ClearSymbols();
                dlclose(handle_);
                handle_ = nullptr;
            }
            AVCODEC_LOGW("fastkit lib not available");
        }
    }
}

void FastKitsInterface::Impl::Release()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (refCount_ > 0) {
        refCount_--;
    }
    if (refCount_ == 0 && handle_ != nullptr) {
        if (clearGPUFunc_ != nullptr) {
            clearGPUFunc_();
        }
        ClearSymbols();
        dlclose(handle_);
        handle_ = nullptr;
        AVCODEC_LOGI("fastkit lib unloaded");
    }
}

bool FastKitsInterface::Impl::OpenLibrary()
{
    handle_ = dlopen(FAST_IMAGE_SO_PATH, RTLD_NOW);
    if (handle_ == nullptr) {
        AVCODEC_LOGE("Failed to load fastkit lib: %{public}s", dlerror());
        return false;
    }
    return true;
}

bool FastKitsInterface::Impl::ReadSymbols()
{
    return LoadSymbol(handle_, initFunc_, "FastScaleInit") &&
           LoadSymbol(handle_, downSampleFunc_, "FastScaleDownSample") &&
           LoadSymbol(handle_, clearGPUFunc_, "FastScaleClearGPU") &&
           LoadSymbol(handle_, cropFunc_, "FastScaleCrop") &&
           LoadSymbol(handle_, createCropRectFunc_, "CreateCropRect") &&
           LoadSymbol(handle_, destroyCropRectFunc_, "DestroyCropRect");
}

void FastKitsInterface::Impl::ClearSymbols()
{
    initFunc_ = nullptr;
    downSampleFunc_ = nullptr;
    clearGPUFunc_ = nullptr;
    cropFunc_ = nullptr;
    createCropRectFunc_ = nullptr;
    destroyCropRectFunc_ = nullptr;
    initializedFormats_.clear();
}

bool FastKitsInterface::Impl::IsLoaded() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return handle_ != nullptr;
}

int32_t FastKitsInterface::Impl::Init(uint32_t format, FastResizeAlgoType algo)
{
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "DS Init format=%u algo=%d", format, static_cast<int32_t>(algo));
    std::unique_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, AVCS_ERR_UNKNOWN, "Library not loaded, call Retain first");
    CHECK_AND_RETURN_RET_LOG(initFunc_ != nullptr, AVCS_ERR_INVALID_VAL, "Init function not loaded");
    if (initializedFormats_.count(format) > 0) {
        AVCODEC_LOGD("Format %{public}u already initialized, skip", format);
        return AVCS_ERR_OK;
    }
    int32_t ret = initFunc_(format, static_cast<int32_t>(algo));
    int32_t err = ConvertErrorCode(ret);
    CHECK_AND_RETURN_RET_LOG(err == AVCS_ERR_OK, err, "Init %{public}d, fmt %{public}u algo %{public}d", ret, format,
                             static_cast<int32_t>(algo));
    initializedFormats_.insert(format);
    return err;
}

int32_t FastKitsInterface::Impl::Crop(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                      const CropRect &rect)
{
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "Crop %dx%d->%dx%d rect=[%d,%d,%d,%d]", input->GetWidth(), input->GetHeight(),
                      output->GetWidth(), output->GetHeight(), rect.top, rect.left, rect.bottom, rect.right);
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(cropFunc_ != nullptr && createCropRectFunc_ != nullptr && destroyCropRectFunc_ != nullptr,
                             AVCS_ERR_INVALID_VAL, "Crop functions not loaded");

    void *cropRect = createCropRectFunc_(rect.top, rect.left, rect.bottom, rect.right);
    CHECK_AND_RETURN_RET_LOG(cropRect != nullptr, AVCS_ERR_UNKNOWN, "createCropRect failed");

    int32_t ret = cropFunc_(input.GetRefPtr(), output.GetRefPtr(), cropRect);
    destroyCropRectFunc_(cropRect);
    int32_t err = ConvertErrorCode(ret);
    CHECK_AND_RETURN_RET_LOG(err == AVCS_ERR_OK, err,
                             "Crop %{public}d, in %{public}dx%{public}d:%{public}d "
                             "out %{public}dx%{public}d:%{public}d "
                             "rect [%{public}d,%{public}d,%{public}d,%{public}d]",
                             ret, input->GetWidth(), input->GetHeight(), input->GetFormat(), output->GetWidth(),
                             output->GetHeight(), output->GetFormat(), rect.top, rect.left, rect.bottom, rect.right);
    return err;
}

int32_t FastKitsInterface::Impl::DownSample(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                            FastResizeAlgoType algo)
{
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "DS %dx%d->%dx%d algo=%d", input->GetWidth(), input->GetHeight(),
                      output->GetWidth(), output->GetHeight(), static_cast<int32_t>(algo));
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(downSampleFunc_ != nullptr, AVCS_ERR_INVALID_VAL, "DownSample function not loaded");

    int32_t ret = downSampleFunc_(input.GetRefPtr(), output.GetRefPtr(), static_cast<int32_t>(algo));
    int32_t err = ConvertErrorCode(ret);
    CHECK_AND_RETURN_RET_LOG(err == AVCS_ERR_OK, err,
                             "DownSample %{public}d, in %{public}dx%{public}d:%{public}d "
                             "out %{public}dx%{public}d:%{public}d",
                             ret, input->GetWidth(), input->GetHeight(), input->GetFormat(), output->GetWidth(),
                             output->GetHeight(), output->GetFormat());
    return err;
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
