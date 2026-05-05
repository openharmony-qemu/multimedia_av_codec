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
#include <unordered_map>
#include "hitrace_meter.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "FastKitsInterface"};
constexpr const char *FAST_IMAGE_SO_PATH = "libfast_image.so";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

FastKitsInterface &FastKitsInterface::GetInstance()
{
    static FastKitsInterface instance;
    return instance;
}

FastKitsInterface::~FastKitsInterface()
{
    if (handle_ != nullptr) {
        ClearSymbols();
        dlclose(handle_);
        handle_ = nullptr;
    }
}

void FastKitsInterface::Retain()
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

void FastKitsInterface::Release()
{
    std::unique_lock<std::shared_mutex> lock(mutex_);
    if (refCount_ > 0) {
        refCount_--;
    }
    if (refCount_ == 0 && handle_ != nullptr) {
        if (fastScaleClearGPUFunc_ != nullptr) {
            fastScaleClearGPUFunc_();
        }
        ClearSymbols();
        dlclose(handle_);
        handle_ = nullptr;
        AVCODEC_LOGI("fastkit lib unloaded");
    }
}

int32_t FastKitsInterface::ConvertErrorCode(int32_t fastErr)
{
    static const std::unordered_map<int32_t, int32_t> errMap = {
        { FAST_ERR_SUCCESS,          AVCS_ERR_OK },
        { FAST_ERR_SUCCESS_PARTIAL,  AVCS_ERR_OK },
        { FAST_ERR_ILLEGAL_INPUT,    AVCS_ERR_INVALID_VAL },
        { FAST_ERR_INVALID_PTR,      AVCS_ERR_INVALID_VAL },
        { FAST_ERR_UNSURPPORTED,     AVCS_ERR_UNSUPPORT },
    };
    auto it = errMap.find(fastErr);
    return it != errMap.end() ? it->second : AVCS_ERR_UNKNOWN;
}

template <typename FuncPtr>
bool FastKitsInterface::LoadSymbol(void *handle, FuncPtr &func, const char *name)
{
    func = reinterpret_cast<FuncPtr>(dlsym(handle, name));
    if (func == nullptr) {
        AVCODEC_LOGE("Failed to find %{public}s: %{public}s", name, dlerror());
        return false;
    }
    return true;
}

int32_t FastKitsInterface::Init(uint32_t format, FastResizeAlgoType algo)
{
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "DS Init format=%u algo=%d", format, static_cast<int32_t>(algo));
    std::unique_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(handle_ != nullptr, AVCS_ERR_UNKNOWN, "Library not loaded, call Retain first");
    CHECK_AND_RETURN_RET_LOG(initFunc_ != nullptr, AVCS_ERR_INVALID_VAL, "Init function not loaded");
    int32_t ret = initFunc_(format, static_cast<int32_t>(algo));
    int32_t err = ConvertErrorCode(ret);
    CHECK_AND_RETURN_RET_LOG(err == AVCS_ERR_OK, err, "Init %{public}d, fmt %{public}u algo %{public}d",
                             ret, format, static_cast<int32_t>(algo));
    return err;
}

bool FastKitsInterface::OpenLibrary()
{
    handle_ = dlopen(FAST_IMAGE_SO_PATH, RTLD_NOW);
    if (handle_ == nullptr) {
        AVCODEC_LOGE("Failed to load fastkit lib: %{public}s", dlerror());
        return false;
    }
    return true;
}

bool FastKitsInterface::ReadSymbols()
{
    return LoadSymbol(handle_, initFunc_, "FastScaleInit") &&
           LoadSymbol(handle_, downSampleFunc_, "FastScaleDownSample") &&
           LoadSymbol(handle_, cropFunc_, "FastScaleCrop") &&
           LoadSymbol(handle_, createRectFunc_, "CreateCropRect") &&
           LoadSymbol(handle_, destroyRectFunc_, "DestroyCropRect") &&
           LoadSymbol(handle_, fastScaleClearGPUFunc_, "FastScaleClearGPU");
}

void FastKitsInterface::ClearSymbols()
{
    initFunc_ = nullptr;
    downSampleFunc_ = nullptr;
    cropFunc_ = nullptr;
    createRectFunc_ = nullptr;
    destroyRectFunc_ = nullptr;
    fastScaleClearGPUFunc_ = nullptr;
}

bool FastKitsInterface::IsLoaded() const
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    return handle_ != nullptr;
}

int32_t FastKitsInterface::Crop(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                const CropRect &rect)
{
    HITRACE_METER_FMT(HITRACE_TAG_ZMEDIA, "Crop %dx%d->%dx%d rect=[%d,%d,%d,%d]", input->GetWidth(), input->GetHeight(),
                      output->GetWidth(), output->GetHeight(), rect.top, rect.left, rect.bottom, rect.right);
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(cropFunc_ != nullptr && createRectFunc_ != nullptr && destroyRectFunc_ != nullptr,
                             AVCS_ERR_INVALID_VAL, "Crop functions not loaded");

    void *cropRect = createRectFunc_(rect.top, rect.left, rect.bottom, rect.right);
    CHECK_AND_RETURN_RET_LOG(cropRect != nullptr, AVCS_ERR_UNKNOWN, "createCropRect failed");

    int32_t ret = cropFunc_(input.GetRefPtr(), output.GetRefPtr(), cropRect);
    destroyRectFunc_(cropRect);
    int32_t err = ConvertErrorCode(ret);
    CHECK_AND_RETURN_RET_LOG(err == AVCS_ERR_OK, err,
                             "Crop %{public}d, in %{public}dx%{public}d:%{public}d "
                             "out %{public}dx%{public}d:%{public}d "
                             "rect [%{public}d,%{public}d,%{public}d,%{public}d]",
                             ret, input->GetWidth(), input->GetHeight(), input->GetFormat(), output->GetWidth(),
                             output->GetHeight(), output->GetFormat(), rect.top, rect.left, rect.bottom, rect.right);
    return err;
}

int32_t FastKitsInterface::DownSample(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
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
