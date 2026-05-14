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

struct CopyRegion {
    int32_t left{0};
    int32_t top{0};
    int32_t width{0};
    int32_t height{0};
};

struct DownsamplePlaneInfo {
    const BufferView *input{nullptr};
    BufferView *output{nullptr};
    const Plane *srcPlane{nullptr};
    const Plane *dstPlane{nullptr};
    size_t planeIndex{0};
    int32_t srcVisibleWidth{0};
    int32_t srcVisibleHeight{0};
    int32_t dstVisibleWidth{0};
    int32_t dstVisibleHeight{0};
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
    OH_NativeBuffer_Planes *planes = nullptr;
    GSError err = buffer->GetPlanesInfo(reinterpret_cast<void **>(&planes));
    if (err != GSERROR_OK || planes == nullptr || planes->planeCount <= 1) {
        return buffer->GetHeight();
    }
    int32_t stride = buffer->GetStride();
    if (stride <= 0) {
        return buffer->GetHeight();
    }
    int32_t sliceHeight = static_cast<int32_t>(static_cast<int64_t>(planes->planes[1].offset) / stride);
    return std::max(sliceHeight, buffer->GetHeight());
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
        AVCODEC_LOGE("Invalid buffer: buffer=%{public}p, addr=%{public}p, width=%{public}d, "
                     "height=%{public}d, stride=%{public}d, size=%{public}d",
                     buffer.GetRefPtr(), buffer ? buffer->GetVirAddr() : nullptr,
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

bool ValidateCropRect(const BufferView &input, const OHOS::MediaAVCodec::PreProcessing::CpuImageCropRect &crop,
                      const BufferView &output)
{
    if (crop.left < 0 || crop.top < 0 || crop.width <= 0 || crop.height <= 0 ||
        crop.left > input.width - crop.width || crop.top > input.height - crop.height ||
        output.width != crop.width || output.height != crop.height) {
        AVCODEC_LOGE("Invalid crop rect: left=%{public}d top=%{public}d width=%{public}d height=%{public}d, "
                     "input=%{public}dx%{public}d output=%{public}dx%{public}d",
                     crop.left, crop.top, crop.width, crop.height, input.width, input.height, output.width,
                     output.height);
        return false;
    }
    if (input.isYuv420 &&
        (!IsEven(crop.left) || !IsEven(crop.top) || !IsEven(crop.width) || !IsEven(crop.height))) {
        AVCODEC_LOGE("YUV420 crop rect is not even-aligned: left=%{public}d top=%{public}d "
                     "width=%{public}d height=%{public}d format=%{public}d",
                     crop.left, crop.top, crop.width, crop.height, input.format);
        return false;
    }
    return true;
}

bool ValidateDownsample(const BufferView &input, const BufferView &output)
{
    if (output.width <= 0 || output.height <= 0 || output.width > input.width || output.height > input.height) {
        return false;
    }
    if (input.isYuv420 && (!IsEven(input.width) || !IsEven(input.height) || !IsEven(output.width) ||
                           !IsEven(output.height))) {
        return false;
    }
    return true;
}

int32_t CopyPlaneRows(const BufferView &input, const BufferView &output, const CopyRegion &region)
{
    for (size_t i = 0; i < input.planeCount; ++i) {
        const Plane &srcPlane = input.planes[i];
        const Plane &dstPlane = output.planes[i];
        int32_t srcLeft = region.left / srcPlane.xSubsample;
        int32_t srcTop = region.top / srcPlane.ySubsample;
        int32_t planeWidth = region.width / srcPlane.xSubsample;
        int32_t planeHeight = region.height / srcPlane.ySubsample;
        int32_t bytesToCopy = planeWidth * srcPlane.bytesPerPixel;
        if (bytesToCopy > dstPlane.rowBytes || planeHeight > dstPlane.height) {
            AVCODEC_LOGE("Crop plane copy exceeds output plane: plane=%{public}zu, bytesToCopy=%{public}d, "
                         "dstRowBytes=%{public}d, planeHeight=%{public}d, dstHeight=%{public}d",
                         i, bytesToCopy, dstPlane.rowBytes, planeHeight, dstPlane.height);
            return OHOS::MediaAVCodec::AVCS_ERR_INVALID_VAL;
        }
        const uint8_t *src = input.addr + srcPlane.offset + static_cast<size_t>(srcTop) * srcPlane.stride +
                             static_cast<size_t>(srcLeft) * srcPlane.bytesPerPixel;
        uint8_t *dst = output.addr + dstPlane.offset;
        for (int32_t row = 0; row < planeHeight; ++row) {
            auto ret = memcpy_s(dst + static_cast<size_t>(row) * dstPlane.stride, static_cast<size_t>(bytesToCopy),
                                src + static_cast<size_t>(row) * srcPlane.stride, static_cast<size_t>(bytesToCopy));
            if (ret != EOK) {
                AVCODEC_LOGE("Copy plane row failed: plane=%{public}zu, row=%{public}d, ret=%{public}d", i, row, ret);
                return OHOS::MediaAVCodec::AVCS_ERR_INVALID_VAL;
            }
        }
    }
    return OHOS::MediaAVCodec::AVCS_ERR_OK;
}

bool InitDownsamplePlaneInfo(const BufferView &input, BufferView &output, size_t planeIndex,
                             DownsamplePlaneInfo &info)
{
    const Plane &srcPlane = input.planes[planeIndex];
    const Plane &dstPlane = output.planes[planeIndex];
    info = {&input, &output, &srcPlane, &dstPlane, planeIndex,
            input.width / srcPlane.xSubsample, input.height / srcPlane.ySubsample,
            output.width / dstPlane.xSubsample, output.height / dstPlane.ySubsample};
    if (info.dstVisibleWidth <= 0 || info.dstVisibleHeight <= 0 || info.dstVisibleWidth > dstPlane.width ||
        info.dstVisibleHeight > dstPlane.height) {
        AVCODEC_LOGE("Invalid downsample plane size: plane=%{public}zu, dstVisible=%{public}dx%{public}d, "
                     "dstPlane=%{public}dx%{public}d",
                     planeIndex, info.dstVisibleWidth, info.dstVisibleHeight, dstPlane.width, dstPlane.height);
        return false;
    }
    return true;
}

int32_t MapDownsampleCoordinate(int32_t dst, int32_t srcVisible, int32_t dstVisible)
{
    return static_cast<int32_t>((static_cast<int64_t>(dst) * static_cast<int64_t>(srcVisible)) / dstVisible);
}

const uint8_t *GetPixelAddress(const BufferView &view, const Plane &plane, int32_t x, int32_t y)
{
    return view.addr + plane.offset + static_cast<size_t>(y) * plane.stride +
           static_cast<size_t>(x) * plane.bytesPerPixel;
}

uint8_t *GetPixelAddress(BufferView &view, const Plane &plane, int32_t x, int32_t y)
{
    return view.addr + plane.offset + static_cast<size_t>(y) * plane.stride +
           static_cast<size_t>(x) * plane.bytesPerPixel;
}

int32_t CopyDownsamplePixel(const DownsamplePlaneInfo &info, int32_t dstX, int32_t dstY)
{
    CHECK_AND_RETURN_RET_LOG(info.dstVisibleWidth != 0, OHOS::MediaAVCodec::AVCS_ERR_INVALID_VAL,
        "dstVisibleWidth is 0");
    int32_t srcX = MapDownsampleCoordinate(dstX, info.srcVisibleWidth, info.dstVisibleWidth);
    int32_t srcY = MapDownsampleCoordinate(dstY, info.srcVisibleHeight, info.dstVisibleHeight);
    const uint8_t *src = GetPixelAddress(*info.input, *info.srcPlane, srcX, srcY);
    uint8_t *dst = GetPixelAddress(*info.output, *info.dstPlane, dstX, dstY);
    auto ret = memcpy_s(dst, static_cast<size_t>(info.dstPlane->bytesPerPixel),
                        src, static_cast<size_t>(info.dstPlane->bytesPerPixel));
    if (ret != EOK) {
        AVCODEC_LOGE("Downsample pixel copy failed: plane=%{public}zu, x=%{public}d, y=%{public}d, ret=%{public}d",
                     info.planeIndex, dstX, dstY, ret);
        return OHOS::MediaAVCodec::AVCS_ERR_INVALID_VAL;
    }
    return OHOS::MediaAVCodec::AVCS_ERR_OK;
}

int32_t DownsamplePlaneRow(const DownsamplePlaneInfo &info, int32_t dstY)
{
    for (int32_t dstX = 0; dstX < info.dstVisibleWidth; ++dstX) {
        int32_t ret = CopyDownsamplePixel(info, dstX, dstY);
        if (ret != OHOS::MediaAVCodec::AVCS_ERR_OK) {
            return ret;
        }
    }
    return OHOS::MediaAVCodec::AVCS_ERR_OK;
}

int32_t DownsamplePlane(const DownsamplePlaneInfo &info)
{
    for (int32_t dstY = 0; dstY < info.dstVisibleHeight; ++dstY) {
        int32_t ret = DownsamplePlaneRow(info, dstY);
        if (ret != OHOS::MediaAVCodec::AVCS_ERR_OK) {
            return ret;
        }
    }
    return OHOS::MediaAVCodec::AVCS_ERR_OK;
}

int32_t DownsamplePlanes(const BufferView &input, BufferView &output)
{
    for (size_t i = 0; i < input.planeCount; ++i) {
        DownsamplePlaneInfo info;
        if (!InitDownsamplePlaneInfo(input, output, i, info)) {
            return OHOS::MediaAVCodec::AVCS_ERR_INVALID_VAL;
        }
        int32_t ret = DownsamplePlane(info);
        if (ret != OHOS::MediaAVCodec::AVCS_ERR_OK) {
            return ret;
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
            return CopyPlaneRows(input, output, {0, 0, input.width, input.height});
        case CpuImageOperation::CROP:
            if (!ValidateCropRect(input, request.crop, output)) {
                return AVCS_ERR_INVALID_VAL;
            }
            return CopyPlaneRows(input, output,
                                 {request.crop.left, request.crop.top, request.crop.width, request.crop.height});
        case CpuImageOperation::DOWNSAMPLE:
            if (!ValidateDownsample(input, output)) {
                return AVCS_ERR_INVALID_VAL;
            }
            return DownsamplePlanes(input, output);
        default:
            return AVCS_ERR_INVALID_VAL;
    }
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
