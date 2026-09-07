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

#include "cpu_image_processor.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "native_buffer.h"
#include "securec.h"

namespace {
using namespace OHOS;

constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CpuImageProcessor"};
constexpr int32_t RGBA_BYTES_PER_PIXEL = 4;
constexpr int32_t P010_LUMA_BYTES_PER_PIXEL = 2;
constexpr int32_t P010_CHROMA_BYTES_PER_PIXEL = 4;
constexpr int32_t YUV_SUBSAMPLE = 2;
constexpr int32_t DEFAULT_PLANE_BYTES_PER_PIXEL = 1;
constexpr int32_t NO_SUBSAMPLE = 1;
constexpr int32_t INVALID_BUFFER_VALUE = -1;
constexpr size_t MAX_PLANE_COUNT = 3;

struct Plane {
    size_t offset{0};
    int32_t width{0};
    int32_t height{0};
    int32_t rowBytes{0};
    int32_t stride{0};
    int32_t bytesPerPixel{DEFAULT_PLANE_BYTES_PER_PIXEL};
    int32_t xSubsample{NO_SUBSAMPLE};
    int32_t ySubsample{NO_SUBSAMPLE};
};

struct BufferView {
    uint8_t *addr{nullptr};
    int32_t format{0};
    int32_t width{0};
    int32_t height{0};
    int32_t stride{0};
    int32_t sliceHeight{0};
    size_t size{0};
    std::array<Plane, MAX_PLANE_COUNT> planes{};
    size_t planeCount{0};
    bool isYuv420{false};
};

struct PlaneInfo {
    int32_t width{0};
    int32_t height{0};
    int32_t rowBytes{0};
    int32_t stride{0};
    int32_t bytesPerPixel{DEFAULT_PLANE_BYTES_PER_PIXEL};
    int32_t xSubsample{NO_SUBSAMPLE};
    int32_t ySubsample{NO_SUBSAMPLE};
};

bool IsYuv420(int32_t format)
{
    return format == GRAPHIC_PIXEL_FMT_YCBCR_420_SP || format == GRAPHIC_PIXEL_FMT_YCRCB_420_SP ||
           format == GRAPHIC_PIXEL_FMT_YCBCR_P010 || format == GRAPHIC_PIXEL_FMT_YCRCB_P010 ||
           format == GRAPHIC_PIXEL_FMT_YCBCR_420_P;
}

bool IsEven(int32_t value)
{
    return (value % YUV_SUBSAMPLE) == 0;
}

int32_t GetSliceHeightFromBuffer(const OHOS::sptr<OHOS::SurfaceBuffer> &buffer)
{
    int32_t stride = buffer->GetStride();
    if (stride <= 0) {
        return buffer->GetHeight();
    }
    OH_NativeBuffer_Planes *planes = nullptr;
    GSError err = buffer->GetPlanesInfo(reinterpret_cast<void **>(&planes));
    if (err != GSERROR_OK || planes == nullptr || planes->planeCount <= 1) {
        return buffer->GetHeight();
    }
    uint64_t sliceHeight = planes->planes[1].offset / static_cast<uint64_t>(stride);
    if (sliceHeight > static_cast<uint64_t>(INT32_MAX)) {
        return INT32_MAX;
    }
    return std::max(static_cast<int32_t>(sliceHeight), buffer->GetHeight());
}

size_t PlaneBytes(int32_t height, int32_t stride)
{
    return static_cast<size_t>(height) * static_cast<size_t>(stride);
}

PlaneInfo LumaInfo(int32_t w, int32_t h, int32_t rowBytes, int32_t stride, int32_t bpp = DEFAULT_PLANE_BYTES_PER_PIXEL)
{
    return {w, h, rowBytes, stride, bpp, NO_SUBSAMPLE, NO_SUBSAMPLE};
}

PlaneInfo ChromaInfo(int32_t w, int32_t h, int32_t rowBytes, int32_t stride,
                     int32_t bpp = DEFAULT_PLANE_BYTES_PER_PIXEL)
{
    return {w, h, rowBytes, stride, bpp, YUV_SUBSAMPLE, YUV_SUBSAMPLE};
}

bool AddPlane(BufferView &view, const PlaneInfo &planeInfo, size_t offset)
{
    if (view.planeCount >= view.planes.size() || planeInfo.width <= 0 || planeInfo.height <= 0 ||
        planeInfo.rowBytes <= 0 || planeInfo.stride < planeInfo.rowBytes || offset > view.size) {
        AVCODEC_LOGE("Invalid plane metadata: planeCount=%{public}zu width=%{public}d height=%{public}d "
                     "rowBytes=%{public}d stride=%{public}d offset=%{public}zu size=%{public}zu",
                     view.planeCount, planeInfo.width, planeInfo.height, planeInfo.rowBytes, planeInfo.stride,
                     offset, view.size);
        return false;
    }
    size_t lastRowOffset = static_cast<size_t>(planeInfo.height - 1) * static_cast<size_t>(planeInfo.stride);
    size_t end = offset + lastRowOffset + static_cast<size_t>(planeInfo.rowBytes);
    if (end > view.size) {
        AVCODEC_LOGE("Plane exceeds buffer size: end=%{public}zu size=%{public}zu "
                     "height=%{public}d rowBytes=%{public}d stride=%{public}d",
                     end, view.size, planeInfo.height, planeInfo.rowBytes, planeInfo.stride);
        return false;
    }
    view.planes[view.planeCount++] = {offset, planeInfo.width, planeInfo.height, planeInfo.rowBytes, planeInfo.stride,
                                      planeInfo.bytesPerPixel, planeInfo.xSubsample, planeInfo.ySubsample};
    return true;
}

bool IsValidBuffer(const OHOS::sptr<OHOS::SurfaceBuffer> &buffer)
{
    if (!buffer || buffer->GetVirAddr() == nullptr || buffer->GetWidth() <= 0 || buffer->GetHeight() <= 0 ||
        buffer->GetStride() <= 0 || buffer->GetSize() <= 0) {
        AVCODEC_LOGE("Invalid buffer. width=%{public}d, height=%{public}d, stride=%{public}d, size=%{public}d",
                     buffer ? buffer->GetWidth() : INVALID_BUFFER_VALUE,
                     buffer ? buffer->GetHeight() : INVALID_BUFFER_VALUE,
                     buffer ? buffer->GetStride() : INVALID_BUFFER_VALUE,
                     buffer ? buffer->GetSize() : INVALID_BUFFER_VALUE);
        return false;
    }
    return true;
}

void InitBufferView(const OHOS::sptr<OHOS::SurfaceBuffer> &buffer, BufferView &view)
{
    view.addr = static_cast<uint8_t *>(buffer->GetVirAddr());
    view.format = buffer->GetFormat();
    view.width = buffer->GetWidth();
    view.height = buffer->GetHeight();
    view.stride = buffer->GetStride();
    view.sliceHeight = GetSliceHeightFromBuffer(buffer);
    view.size = static_cast<size_t>(buffer->GetSize());
    view.isYuv420 = IsYuv420(view.format);
    view.planeCount = 0;
}

bool BuildNV12Layout(BufferView &view)
{
    size_t uvOffset = PlaneBytes(view.sliceHeight, view.stride);
    return AddPlane(view, LumaInfo(view.width, view.height, view.width, view.stride), 0) &&
           AddPlane(view, ChromaInfo(view.width / YUV_SUBSAMPLE, view.height / YUV_SUBSAMPLE,
                                     view.width, view.stride, YUV_SUBSAMPLE), uvOffset);
}

bool BuildP010Layout(BufferView &view)
{
    if (view.width > INT32_MAX / P010_LUMA_BYTES_PER_PIXEL) {
        AVCODEC_LOGE("Invalid plane width for P010 format: width=%{public}d", view.width);
        return false;
    }
    size_t uvOffset = PlaneBytes(view.sliceHeight, view.stride);
    return AddPlane(view, LumaInfo(view.width, view.height, view.width * P010_LUMA_BYTES_PER_PIXEL,
                                    view.stride, P010_LUMA_BYTES_PER_PIXEL), 0) &&
           AddPlane(view, ChromaInfo(view.width / YUV_SUBSAMPLE, view.height / YUV_SUBSAMPLE,
                                     view.width * P010_LUMA_BYTES_PER_PIXEL, view.stride,
                                     P010_CHROMA_BYTES_PER_PIXEL), uvOffset);
}

bool BuildI420Layout(BufferView &view)
{
    if (!IsEven(view.stride)) {
        return false;
    }
    int32_t chromaStride = view.stride / YUV_SUBSAMPLE;
    size_t uOffset = PlaneBytes(view.sliceHeight, view.stride);
    size_t vOffset = uOffset + PlaneBytes(view.sliceHeight / YUV_SUBSAMPLE, chromaStride);
    return AddPlane(view, LumaInfo(view.width, view.height, view.width, view.stride), 0) &&
           AddPlane(view, ChromaInfo(view.width / YUV_SUBSAMPLE, view.height / YUV_SUBSAMPLE,
                                     view.width / YUV_SUBSAMPLE, chromaStride), uOffset) &&
           AddPlane(view, ChromaInfo(view.width / YUV_SUBSAMPLE, view.height / YUV_SUBSAMPLE,
                                     view.width / YUV_SUBSAMPLE, chromaStride), vOffset);
}

bool BuildPlaneLayout(BufferView &view)
{
    if (view.isYuv420 && (!IsEven(view.width) || !IsEven(view.height))) {
        AVCODEC_LOGE("YUV420 buffer geometry is not even: width=%{public}d height=%{public}d format=%{public}d",
                     view.width, view.height, view.format);
        return false;
    }
    switch (view.format) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
            return BuildNV12Layout(view);
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
            return BuildP010Layout(view);
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
            return BuildI420Layout(view);
        case GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
            if (view.width > INT32_MAX / RGBA_BYTES_PER_PIXEL) {
                AVCODEC_LOGE("Invalid plane width for RGBA format: width=%{public}d", view.width);
                return false;
            }
            return AddPlane(view, LumaInfo(view.width, view.height, view.width * RGBA_BYTES_PER_PIXEL,
                                           view.stride, RGBA_BYTES_PER_PIXEL), 0);
        default:
            AVCODEC_LOGE("Unsupported buffer format: %{public}d", view.format);
            return false;
    }
}

bool BuildView(const OHOS::sptr<OHOS::SurfaceBuffer> &buffer, BufferView &view)
{
    if (!IsValidBuffer(buffer)) {
        return false;
    }
    InitBufferView(buffer, view);
    return BuildPlaneLayout(view);
}

int32_t CopyPlaneRows(const BufferView &input, const BufferView &output)
{
    for (size_t i = 0; i < input.planeCount; ++i) {
        const Plane &srcPlane = input.planes[i];
        const Plane &dstPlane = output.planes[i];
        if (srcPlane.rowBytes > dstPlane.rowBytes || srcPlane.height > dstPlane.height) {
            AVCODEC_LOGE("Copy plane exceeds output plane: plane=%{public}zu, bytesToCopy=%{public}d, "
                         "dstRowBytes=%{public}d, planeHeight=%{public}d, dstHeight=%{public}d",
                         i, srcPlane.rowBytes, dstPlane.rowBytes, srcPlane.height, dstPlane.height);
            return OHOS::MediaAVCodec::AVCS_ERR_INVALID_VAL;
        }
        const uint8_t *src = input.addr + srcPlane.offset;
        uint8_t *dst = output.addr + dstPlane.offset;
        for (int32_t row = 0; row < srcPlane.height; ++row) {
            auto ret = memcpy_s(dst + static_cast<size_t>(row) * dstPlane.stride, srcPlane.rowBytes,
                                src + static_cast<size_t>(row) * srcPlane.stride, srcPlane.rowBytes);
            if (ret != EOK) {
                AVCODEC_LOGE("Copy plane row failed: plane=%{public}zu, row=%{public}d, ret=%{public}d", i, row, ret);
                return OHOS::MediaAVCodec::AVCS_ERR_INVALID_VAL;
            }
        }
    }
    return OHOS::MediaAVCodec::AVCS_ERR_OK;
}

} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {

bool CpuImageProcessor::IsFormatSupported(int32_t format)
{
    switch (format) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
        case GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
            return true;
        default:
            return false;
    }
}

int32_t CpuImageProcessor::Process(const CpuImageProcessRequest &request)
{
    BufferView input;
    BufferView output;
    if (!BuildView(request.input, input) || !BuildView(request.output, output)) {
        AVCODEC_LOGE("Invalid input or output buffer metadata");
        return AVCS_ERR_INVALID_VAL;
    }
    if (input.format != output.format || !IsFormatSupported(input.format)) {
        AVCODEC_LOGE("Unsupported format or format mismatch: input=%{public}d output=%{public}d",
                     input.format, output.format);
        return AVCS_ERR_INVALID_VAL;
    }
    if (input.planeCount != output.planeCount) {
        AVCODEC_LOGE("Plane count mismatch: input=%{public}zu output=%{public}zu",
                     input.planeCount, output.planeCount);
        return AVCS_ERR_INVALID_VAL;
    }

    switch (request.operation) {
        case CpuImageOperation::COPY:
            if (input.width != output.width || input.height != output.height) {
                AVCODEC_LOGE("Copy size mismatch: input=%{public}dx%{public}d output=%{public}dx%{public}d",
                             input.width, input.height, output.width, output.height);
                return AVCS_ERR_INVALID_VAL;
            }
            return CopyPlaneRows(input, output);
        default:
            return AVCS_ERR_INVALID_VAL;
    }
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
