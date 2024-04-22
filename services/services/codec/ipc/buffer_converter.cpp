/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "buffer_converter.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "meta/meta_key.h"
#include "native_buffer.h"
#include "surface_buffer.h"
#include "surface_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "BufferConverter"};
using AVCodecRect = OHOS::MediaAVCodec::BufferConverter::AVCodecRect;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
constexpr int32_t OFFSET_2 = 0x02;
constexpr int32_t OFFSET_3 = 0x03;
constexpr int32_t OFFSET_15 = 0x0F;
constexpr int32_t OFFSET_16 = 0x10;
VideoPixelFormat TranslateSurfaceFormat(GraphicPixelFormat surfaceFormat)
{
    switch (surfaceFormat) {
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P: {
            return VideoPixelFormat::YUV420P;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888: {
            return VideoPixelFormat::RGBA;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP: {
            return VideoPixelFormat::NV12;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP: {
            return VideoPixelFormat::NV21;
        }
        default:
            return VideoPixelFormat::UNKNOWN;
    }
}

int32_t ConvertYUV420SP(uint8_t *dst, const AVCodecRect &dstRect, uint8_t *src, const AVCodecRect &srcRect,
                        AVCodecRect rect)
{
    // Y
    for (int32_t i = 0; i < rect.height; ++i) {
        (void)memcpy_s(dst, dstRect.stride, src, rect.stride);
        dst += dstRect.stride;
        src += srcRect.stride;
    }
    // padding
    dst += (dstRect.height - rect.height) * dstRect.stride;
    src += (srcRect.height - rect.height) * srcRect.stride;
    rect.height >>= 1;
    // UV
    for (int32_t i = 0; i < rect.height; ++i) {
        (void)memcpy_s(dst, dstRect.stride, src, rect.stride);
        dst += dstRect.stride;
        src += srcRect.stride;
    }
    return (OFFSET_3 * dstRect.stride * dstRect.height) >> 1;
}

int32_t ConvertYUV420P(uint8_t *dst, const AVCodecRect &dstRect, uint8_t *src, const AVCodecRect &srcRect,
                       AVCodecRect rect)
{
    // Y
    for (int32_t i = 0; i < rect.height; ++i) {
        (void)memcpy_s(dst, dstRect.stride, src, rect.stride);
        dst += dstRect.stride;
        src += srcRect.stride;
    }
    // padding
    const int32_t dstWidth = dstRect.stride >> 1;
    const int32_t srcWidth = srcRect.stride >> 1;
    const int32_t dstPadding = (dstRect.height - rect.height) * dstRect.stride;
    const int32_t srcPadding = (srcRect.height - rect.height) * srcRect.stride;
    dst += dstPadding;
    src += srcPadding;
    rect.height >>= 1;
    // U
    for (int32_t i = 0; i < rect.height; ++i) {
        (void)memcpy_s(dst, dstRect.stride, src, rect.stride);
        dst += dstWidth;
        src += srcWidth;
    }
    // padding
    dst += dstPadding >> OFFSET_2;
    src += srcPadding >> OFFSET_2;
    // V
    for (int32_t i = 0; i < rect.height; ++i) {
        (void)memcpy_s(dst, dstRect.stride, src, rect.stride);
        dst += dstWidth;
        src += srcWidth;
    }
    return (OFFSET_3 * dstRect.stride * dstRect.height) >> 1;
}

int32_t ConverteRGBA8888(uint8_t *dst, const AVCodecRect &dstRect, uint8_t *src, const AVCodecRect &srcRect,
                         AVCodecRect rect)
{
    for (int32_t i = 0; i < rect.height; ++i) {
        (void)memcpy_s(dst, dstRect.stride, src, rect.stride);
        dst += dstRect.stride;
        src += srcRect.stride;
    }
    return dstRect.stride * dstRect.height;
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace OHOS;
using namespace OHOS::Media;
std::shared_ptr<BufferConverter> BufferConverter::Create(AVCodecType type)
{
    if (type == AVCODEC_TYPE_VIDEO_ENCODER) {
        return std::make_shared<BufferConverter>(true);
    } else if (type == AVCODEC_TYPE_VIDEO_DECODER) {
        return std::make_shared<BufferConverter>(false);
    }
    return nullptr;
}

BufferConverter::BufferConverter(bool isEncoder)
    : func_(ConvertYUV420SP), isEncoder_(isEncoder), isSharedMemory_(false), needResetFormat_(true)
{
}

int32_t BufferConverter::ReadFromBuffer(std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
{
    if (isSharedMemory_) {
        return AVCS_ERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_INVALID_VAL, "buffer is nullptr");
    if (buffer->memory_ == nullptr) {
        return AVCS_ERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(buffer->memory_->GetAddr() != nullptr, AVCS_ERR_INVALID_VAL, "buffer addr is nullptr");
    CHECK_AND_RETURN_RET_LOG(memory != nullptr && memory->GetBase() != nullptr, AVCS_ERR_INVALID_VAL,
                             "shared memory is nullptr");
    if (isEncoder_) {
        int32_t size = buffer->memory_->GetSize();
        if (size > 0) {
            int32_t ret = buffer->memory_->Read(memory->GetBase(), size, 0);
            CHECK_AND_RETURN_RET_LOG(ret == size, AVCS_ERR_INVALID_VAL, "Read avbuffer's data failed.");
        }
        return AVCS_ERR_OK;
    }
    int32_t usrSize = func_(memory->GetBase(), usrRect_, buffer->memory_->GetAddr(), hwRect_, rect_);
    buffer->memory_->SetSize(usrSize);
    return AVCS_ERR_OK;
}

int32_t BufferConverter::WriteToBuffer(std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
{
    if (isSharedMemory_) {
        return AVCS_ERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->memory_ != nullptr && buffer->memory_->GetAddr() != nullptr,
                             AVCS_ERR_INVALID_VAL, "buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(memory != nullptr && memory->GetBase() != nullptr, AVCS_ERR_INVALID_VAL,
                             "shared memory is nullptr");
    if (!isEncoder_) {
        int32_t size = buffer->memory_->GetSize();
        if (size > 0) {
            (void)buffer->memory_->Write(memory->GetBase(), size, 0);
        }
        return AVCS_ERR_OK;
    }
    int32_t hwSize = func_(buffer->memory_->GetAddr(), hwRect_, memory->GetBase(), usrRect_, rect_);
    buffer->memory_->SetSize(hwSize);
    return AVCS_ERR_OK;
}

void BufferConverter::NeedToResetFormatOnce()
{
    needResetFormat_ = true;
}

void BufferConverter::GetFormat(Format &format)
{
    if (isSharedMemory_ || needResetFormat_) {
        return;
    }
    if (format.ContainKey(Tag::VIDEO_WIDTH)) {
        format.PutIntValue(Tag::VIDEO_WIDTH, usrRect_.stride / pixcelSize_);
    }
    if (format.ContainKey(Tag::VIDEO_HEIGHT)) {
        format.PutIntValue(Tag::VIDEO_HEIGHT, usrRect_.height);
    }
    if (format.ContainKey(Tag::VIDEO_STRIDE)) {
        format.PutIntValue(Tag::VIDEO_STRIDE, usrRect_.stride);
    }
    if (format.ContainKey(Tag::VIDEO_SLICE_HEIGHT)) {
        format.PutIntValue(Tag::VIDEO_SLICE_HEIGHT, usrRect_.height);
    }
}

void BufferConverter::SetFormat(const Format &format)
{
    if (isSharedMemory_) {
        return;
    }
    int32_t width = 0;
    int32_t height = 0;
    int32_t pixelFormat = static_cast<int32_t>(VideoPixelFormat::UNKNOWN);
    if (format.GetIntValue(Tag::VIDEO_PIXEL_FORMAT, pixelFormat)) {
        SetPixFormat(static_cast<VideoPixelFormat>(pixelFormat));
    }
    if (format.GetIntValue(Tag::VIDEO_DISPLAY_WIDTH, width) || format.GetIntValue(Tag::VIDEO_WIDTH, width)) {
        SetWidth(width);
    }
    if (format.GetIntValue(Tag::VIDEO_DISPLAY_HEIGHT, height) || format.GetIntValue(Tag::VIDEO_HEIGHT, height)) {
        SetHeight(height);
    }
    if (!format.GetIntValue(Tag::VIDEO_STRIDE, hwRect_.stride)) {
        SetStride(rect_.stride);
    }
    if (!format.GetIntValue(Tag::VIDEO_SLICE_HEIGHT, hwRect_.height)) {
        SetSliceHeight(rect_.height);
    }
    if (width != 0) {
        const int32_t tempPixcelSize = hwRect_.stride / width;
        pixcelSize_ = tempPixcelSize == 0 ? 1 : tempPixcelSize;
        rect_.stride = width * pixcelSize_;
        usrRect_.stride *= pixcelSize_;
    }
    AVCODEC_LOGI(
        "Actual:(%{public}d x %{public}d), Converter:(%{public}d x %{public}d), Hardware:(%{public}d x %{public}d).",
        width, rect_.height, usrRect_.stride, usrRect_.height, hwRect_.stride, hwRect_.height);
}

void BufferConverter::SetInputBufferFormat(std::shared_ptr<AVBuffer> &buffer)
{
    if (!needResetFormat_ || !isEncoder_) {
        return;
    }
    needResetFormat_ = false;
    SetBufferFormat(buffer);
}

void BufferConverter::SetOutputBufferFormat(std::shared_ptr<AVBuffer> &buffer)
{
    if (!needResetFormat_ || isEncoder_) {
        return;
    }
    needResetFormat_ = false;
    SetBufferFormat(buffer);
}

void BufferConverter::SetPixFormat(const VideoPixelFormat pixelFormat)
{
    switch (pixelFormat) {
        case VideoPixelFormat::YUVI420:
            func_ = ConvertYUV420P;
            break;
        case VideoPixelFormat::NV12:
        case VideoPixelFormat::NV21:
            func_ = ConvertYUV420SP;
            break;
        case VideoPixelFormat::RGBA:
            func_ = ConverteRGBA8888;
            break;
        default:
            AVCODEC_LOGE("Invalid video pix format.");
            break;
    };
}

inline void BufferConverter::SetWidth(const int32_t width)
{
    rect_.stride = width;
    int32_t modVal = width & OFFSET_15;
    if (modVal) {
        usrRect_.stride = width + OFFSET_16 - modVal;
    } else {
        usrRect_.stride = width;
    }
}

inline void BufferConverter::SetHeight(const int32_t height)
{
    rect_.height = height;
    int32_t modVal = height & OFFSET_15;
    if (modVal) {
        usrRect_.height = height + OFFSET_16 - modVal;
    } else {
        usrRect_.height = height;
    }
}

inline void BufferConverter::SetStride(const int32_t stride)
{
    hwRect_.stride = stride;
}

inline void BufferConverter::SetSliceHeight(const int32_t sliceHeight)
{
    hwRect_.height = sliceHeight;
}

void BufferConverter::SetBufferFormat(std::shared_ptr<AVBuffer> &buffer)
{
    CHECK_AND_RETURN_LOG(buffer != nullptr && buffer->memory_ != nullptr, "buffer is nullptr");
    isSharedMemory_ = buffer->memory_->GetMemoryType() == MemoryType::SHARED_MEMORY;

    auto surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
    CHECK_AND_RETURN_LOG(surfaceBuffer != nullptr, "surface buffer is nullptr");
    // pixcelFormat
    VideoPixelFormat pixelFormat = TranslateSurfaceFormat(static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat()));
    SetPixFormat(pixelFormat);
    // width
    int32_t width = surfaceBuffer->GetWidth();
    SetWidth(width);
    // height
    SetHeight(surfaceBuffer->GetHeight());
    // stride
    SetStride(surfaceBuffer->GetStride());
    // sliceHeight
    void *planesInfoPtr = nullptr;
    surfaceBuffer->GetPlanesInfo(&planesInfoPtr);
    CHECK_AND_RETURN_LOG(planesInfoPtr != nullptr, "planes info is nullptr");
    auto planesInfo = static_cast<OH_NativeBuffer_Planes *>(planesInfoPtr);
    CHECK_AND_RETURN_LOG(planesInfo->planeCount > 1, "planes count is %{public}d", planesInfo->planeCount);
    SetSliceHeight(planesInfo->planes[1].offset / planesInfo->planes[1].columnStride);
    // pixcelSize
    if (width != 0) {
        const int32_t tempPixcelSize = hwRect_.stride / width;
        pixcelSize_ = tempPixcelSize == 0 ? 1 : tempPixcelSize;
        rect_.stride = width * pixcelSize_;
        usrRect_.stride *= pixcelSize_;
    }
    AVCODEC_LOGI(
        "Actual:(%{public}d x %{public}d), Converter:(%{public}d x %{public}d), Hardware:(%{public}d x %{public}d).",
        width, rect_.height, usrRect_.stride, usrRect_.height, hwRect_.stride, hwRect_.height);
}
} // namespace MediaAVCodec
} // namespace OHOS
