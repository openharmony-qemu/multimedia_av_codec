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
#include <cmath>
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
            return VideoPixelFormat::YUVI420;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888: {
            return VideoPixelFormat::RGBA;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP: {
            return VideoPixelFormat::NV12;
        }
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010:
        case GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP: {
            return VideoPixelFormat::NV21;
        }
        default:
            AVCODEC_LOGE("Invalid graphic pixcel format:%{public}d", static_cast<int32_t>(surfaceFormat));
            return VideoPixelFormat::UNKNOWN;
    }
}

int32_t ConvertYUV420SP(uint8_t *dst, uint8_t *src, AVCodecRect *rects, int32_t capacity)
{
    AVCodecRect &dstRect = rects[0];
    AVCodecRect &srcRect = rects[1];
    AVCodecRect &rect = rects[2]; // 2: index
    int32_t dstSize = (OFFSET_3 * dstRect.wStride * dstRect.hStride) >> 1;
    int32_t ret;
    CHECK_AND_RETURN_RET_LOG(dstSize <= capacity, 0, "No memory. dstSize:%{public}d, capacity:%{public}d", dstSize,
                             capacity);
    // Y
    for (int32_t i = 0; i < rect.hStride; ++i) {
        ret = memcpy_s(dst, dstRect.wStride, src, rect.wStride);
        EXPECT_AND_LOGW(ret != 0, "memcpy failed");
        dst += dstRect.wStride;
        src += srcRect.wStride;
    }
    // padding
    dst += (dstRect.hStride - rect.hStride) * dstRect.wStride;
    src += (srcRect.hStride - rect.hStride) * srcRect.wStride;
    rect.hStride >>= 1;
    // UV
    for (int32_t i = 0; i < rect.hStride; ++i) {
        ret = memcpy_s(dst, dstRect.wStride, src, rect.wStride);
        EXPECT_AND_LOGW(ret != 0, "memcpy failed");
        dst += dstRect.wStride;
        src += srcRect.wStride;
    }
    return dstSize;
}

int32_t ConvertYUV420P(uint8_t *dst, uint8_t *src, AVCodecRect *rects, int32_t capacity)
{
    AVCodecRect &dstRect = rects[0];
    AVCodecRect &srcRect = rects[1];
    AVCodecRect &rect = rects[2]; // 2: index
    int32_t dstSize = (OFFSET_3 * dstRect.wStride * dstRect.hStride) >> 1;
    int32_t ret;
    CHECK_AND_RETURN_RET_LOG(dstSize <= capacity, 0, "No memory. dstSize:%{public}d, capacity:%{public}d", dstSize,
                             capacity);
    // Y
    for (int32_t i = 0; i < rect.hStride; ++i) {
        ret = memcpy_s(dst, dstRect.wStride, src, rect.wStride);
        EXPECT_AND_LOGW(ret != 0, "memcpy failed");
        dst += dstRect.wStride;
        src += srcRect.wStride;
    }
    // padding
    const int32_t dstWidth = dstRect.wStride >> 1;
    const int32_t srcWidth = srcRect.wStride >> 1;
    const int32_t dstPadding = (dstRect.hStride - rect.hStride) * dstRect.wStride;
    const int32_t srcPadding = (srcRect.hStride - rect.hStride) * srcRect.wStride;
    rect.hStride >>= 1;
    rect.wStride >>= 1;
    dst += dstPadding;
    src += srcPadding;
    // U
    for (int32_t i = 0; i < rect.hStride; ++i) {
        ret = memcpy_s(dst, dstWidth, src, rect.wStride);
        EXPECT_AND_LOGW(ret != 0, "memcpy failed");
        dst += dstWidth;
        src += srcWidth;
    }
    // padding
    dst += dstPadding >> OFFSET_2;
    src += srcPadding >> OFFSET_2;
    // V
    for (int32_t i = 0; i < rect.hStride; ++i) {
        ret = memcpy_s(dst, dstWidth, src, rect.wStride);
        EXPECT_AND_LOGW(ret != 0, "memcpy failed");
        dst += dstWidth;
        src += srcWidth;
    }
    return dstSize;
}

int32_t ConverteRGBA8888(uint8_t *dst, uint8_t *src, AVCodecRect *rects, int32_t capacity)
{
    AVCodecRect &dstRect = rects[0];
    AVCodecRect &srcRect = rects[1];
    AVCodecRect &rect = rects[2]; // 2: index
    int32_t dstSize = dstRect.wStride * dstRect.hStride;
    int32_t ret;
    CHECK_AND_RETURN_RET_LOG(dstSize <= capacity, 0, "No memory. dstSize:%{public}d, capacity:%{public}d", dstSize,
                             capacity);
    for (int32_t i = 0; i < rect.hStride; ++i) {
        ret = memcpy_s(dst, dstRect.wStride, src, rect.wStride);
        EXPECT_AND_LOGW(ret != 0, "memcpy failed");
        dst += dstRect.wStride;
        src += srcRect.wStride;
    }
    return dstSize;
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
    std::shared_lock<std::shared_mutex> lock(mutex_);
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
    int32_t size = buffer->memory_->GetSize();
    if (size <= 0) {
        return AVCS_ERR_OK;
    }
    if (isEncoder_) {
        int32_t ret = buffer->memory_->Read(memory->GetBase(), size, 0);
        CHECK_AND_RETURN_RET_LOG(ret == size, AVCS_ERR_INVALID_VAL, "Read avbuffer's data failed.");
        return AVCS_ERR_OK;
    }
    AVCodecRect rects[3] = {usrRect_, hwRect_, rect_}; // 1:dstRect, 2:srcRect, 3:rect
    int32_t usrSize = func_(memory->GetBase(), buffer->memory_->GetAddr(), rects, memory->GetSize());
    buffer->memory_->SetSize(usrSize);
    return AVCS_ERR_OK;
}

int32_t BufferConverter::WriteToBuffer(std::shared_ptr<AVBuffer> &buffer, std::shared_ptr<AVSharedMemory> &memory)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (isSharedMemory_) {
        return AVCS_ERR_OK;
    }
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->memory_ != nullptr && buffer->memory_->GetAddr() != nullptr,
                             AVCS_ERR_INVALID_VAL, "buffer is nullptr");
    CHECK_AND_RETURN_RET_LOG(memory != nullptr && memory->GetBase() != nullptr, AVCS_ERR_INVALID_VAL,
                             "shared memory is nullptr");
    int32_t size = buffer->memory_->GetSize();
    if (size <= 0) {
        return AVCS_ERR_OK;
    }
    if (!isEncoder_) {
        (void)buffer->memory_->Write(memory->GetBase(), size, 0);
        return AVCS_ERR_OK;
    }
    AVCodecRect rects[3] = {hwRect_, usrRect_, rect_}; // 1:dstRect, 2:srcRect, 3:rect
    int32_t hwSize = func_(buffer->memory_->GetAddr(), memory->GetBase(), rects, buffer->memory_->GetCapacity());
    buffer->memory_->SetSize(hwSize);
    return AVCS_ERR_OK;
}

void BufferConverter::NeedToResetFormatOnce()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    needResetFormat_ = true;
}

void BufferConverter::GetFormat(Format &format)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    if (isSharedMemory_ || needResetFormat_) {
        return;
    }
    if (!isEncoder_ && format.ContainKey(Tag::VIDEO_WIDTH)) {
        format.PutIntValue(Tag::VIDEO_WIDTH, rect_.wStride / pixcelSize_);
    }
    if (!isEncoder_ && format.ContainKey(Tag::VIDEO_HEIGHT)) {
        format.PutIntValue(Tag::VIDEO_HEIGHT, rect_.hStride);
    }
    if (format.ContainKey(Tag::VIDEO_STRIDE)) {
        format.PutIntValue(Tag::VIDEO_STRIDE, usrRect_.wStride);
    }
    if (format.ContainKey(Tag::VIDEO_SLICE_HEIGHT)) {
        format.PutIntValue(Tag::VIDEO_SLICE_HEIGHT, usrRect_.hStride);
    }
}

void BufferConverter::SetFormat(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    if (isSharedMemory_) {
        return;
    }
    int32_t width = 0;
    int32_t height = 0;
    int32_t wStride = 0;
    int32_t hStride = 0;
    int32_t pixelFormat = static_cast<int32_t>(VideoPixelFormat::UNKNOWN);
    if (format.GetIntValue(Tag::VIDEO_PIXEL_FORMAT, pixelFormat)) {
        SetPixFormat(static_cast<VideoPixelFormat>(pixelFormat));
    }
    if (format.GetIntValue(Tag::VIDEO_WIDTH, width)) {
        SetWidth(width);
    }
    if (format.GetIntValue(Tag::VIDEO_HEIGHT, height)) {
        SetHeight(height);
    }
    if (!format.GetIntValue(Tag::VIDEO_STRIDE, wStride)) {
        SetWidthStride(rect_.wStride);
    } else {
        hwRect_.wStride = wStride;
    }
    if (!format.GetIntValue(Tag::VIDEO_SLICE_HEIGHT, hStride)) {
        SetHeightStride(rect_.hStride);
    } else {
        hwRect_.hStride = hStride;
    }
    // check if the converter needs to reset the format.
    bool isHeightStrideValid = (hStride != 0 && !isEncoder_) || isEncoder_;
    needResetFormat_ = width == 0 || height == 0 || wStride == 0 || !isHeightStrideValid ||
                       pixelFormat == static_cast<int32_t>(VideoPixelFormat::UNKNOWN);
    if (needResetFormat_) {
        AVCODEC_LOGW("Invalid format:%{public}s", format.Stringify().c_str());
        return;
    }
    SetFormatInner(width);
    AVCODEC_LOGI(
        "Actual:(%{public}d x %{public}d), Converter:(%{public}d x %{public}d), Hardware:(%{public}d x %{public}d).",
        width, rect_.hStride, usrRect_.wStride, usrRect_.hStride, hwRect_.wStride, hwRect_.hStride);
}

void BufferConverter::SetInputBufferFormat(std::shared_ptr<AVBuffer> &buffer)
{
    if (!isEncoder_) {
        return;
    }
    std::lock_guard<std::shared_mutex> lock(mutex_);
    if (!needResetFormat_) {
        return;
    }
    needResetFormat_ = !SetBufferFormat(buffer);
}

void BufferConverter::SetOutputBufferFormat(std::shared_ptr<AVBuffer> &buffer)
{
    if (isEncoder_) {
        return;
    }
    std::lock_guard<std::shared_mutex> lock(mutex_);
    if (!needResetFormat_) {
        return;
    }
    needResetFormat_ = !SetBufferFormat(buffer);
}

void BufferConverter::SetPixFormat(const VideoPixelFormat pixelFormat)
{
    switch (pixelFormat) {
        case VideoPixelFormat::YUV420P:
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
            AVCODEC_LOGE("Invalid video pix format:%{public}d", static_cast<int32_t>(pixelFormat));
            break;
    };
}

inline void BufferConverter::SetWidth(const int32_t width)
{
    rect_.wStride = width;
    int32_t modVal = width & OFFSET_15;
    if (modVal) {
        usrRect_.wStride = width + OFFSET_16 - modVal;
    } else {
        usrRect_.wStride = width;
    }
}

inline void BufferConverter::SetHeight(const int32_t height)
{
    rect_.hStride = height;
    int32_t modVal = height & OFFSET_15;
    if (modVal) {
        usrRect_.hStride = height + OFFSET_16 - modVal;
    } else {
        usrRect_.hStride = height;
    }
}

inline void BufferConverter::SetWidthStride(const int32_t wStride)
{
    hwRect_.wStride = wStride;
}

inline void BufferConverter::SetHeightStride(const int32_t hStride)
{
    hwRect_.hStride = hStride;
}

bool BufferConverter::SetBufferFormat(std::shared_ptr<AVBuffer> &buffer)
{
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr && buffer->memory_ != nullptr, false, "buffer is nullptr");
    isSharedMemory_ = buffer->memory_->GetMemoryType() == MemoryType::SHARED_MEMORY;
    if (isSharedMemory_) {
        AVCODEC_LOGW("AVBuffer is shared memory");
        return true;
    }

    auto surfaceBuffer = buffer->memory_->GetSurfaceBuffer();
    CHECK_AND_RETURN_RET_LOG(surfaceBuffer != nullptr, false, "surface buffer is nullptr");
    // pixcelFormat
    VideoPixelFormat pixelFormat = TranslateSurfaceFormat(static_cast<GraphicPixelFormat>(surfaceBuffer->GetFormat()));
    SetPixFormat(pixelFormat);
    // width
    int32_t width = surfaceBuffer->GetWidth();
    SetWidth(width);
    // hStride
    int32_t height = surfaceBuffer->GetHeight();
    SetHeight(height);
    // wStride
    SetWidthStride(surfaceBuffer->GetStride());
    // sliceHeight
    if (isEncoder_) {
        SetHeightStride(height);
    } else {
        void *planesInfoPtr = nullptr;
        surfaceBuffer->GetPlanesInfo(&planesInfoPtr);
        CHECK_AND_RETURN_RET_LOG(planesInfoPtr != nullptr, false, "planes info is nullptr");
        auto planesInfo = static_cast<OH_NativeBuffer_Planes *>(planesInfoPtr);
        CHECK_AND_RETURN_RET_LOG(planesInfo->planeCount > 1, false, "planes count is %{public}d",
                                 planesInfo->planeCount);
        SetHeightStride(planesInfo->planes[1].offset / planesInfo->planes[1].columnStride);
    }
    // pixcelSize
    CHECK_AND_RETURN_RET_LOG(width != 0, false, "width is 0");
    SetFormatInner(width);
    AVCODEC_LOGI(
        "Actual:(%{public}d x %{public}d), Converter:(%{public}d x %{public}d), Hardware:(%{public}d x %{public}d).",
        width, rect_.hStride, usrRect_.wStride, usrRect_.hStride, hwRect_.wStride, hwRect_.hStride);
    return true;
}

void BufferConverter::SetFormatInner(const int32_t &width)
{
    CHECK_AND_RETURN_LOG(width != 0, "width is 0");
    const int32_t tempPixcelSize = hwRect_.wStride / width;
    pixcelSize_ = tempPixcelSize == 0 ? 1 : tempPixcelSize;

    rect_.wStride = width * pixcelSize_;
    usrRect_.wStride *= pixcelSize_;

    usrRect_.wStride = std::min(usrRect_.wStride, hwRect_.wStride);
    usrRect_.hStride = std::min(usrRect_.hStride, hwRect_.hStride);
}
} // namespace MediaAVCodec
} // namespace OHOS
