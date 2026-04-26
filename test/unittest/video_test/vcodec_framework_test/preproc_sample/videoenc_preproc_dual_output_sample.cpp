/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "videoenc_preproc_dual_output_sample.h"

#include <array>
#include <cerrno>
#include <chrono>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "native_buffer.h"
#include "native_buffer_inner.h"
#include "native_avbuffer.h"
#include "surface_buffer.h"

using namespace std;

namespace {
static int64_t GetTimeUs()
{
    struct timespec now {};
    (void)clock_gettime(CLOCK_BOOTTIME, &now);
    return static_cast<int64_t>(now.tv_sec) * 1000000 + static_cast<int64_t>(now.tv_nsec / 1000);
}

static bool EnsureOutputDir(const std::string &path)
{
    size_t pos = path.find_last_of("/\\");
    if (pos == std::string::npos) {
        return true;
    }
    std::string dir = path.substr(0, pos);
    if (dir.empty()) {
        return true;
    }
    if (mkdir(dir.c_str(), S_IRWXU | S_IRWXG | S_IRWXO) == 0 || errno == EEXIST) {
        return true;
    }
    return false;
}

static std::string MakeSecondaryOutputPath(const std::string &primaryPath)
{
    size_t dirPos = primaryPath.find_last_of("/\\");
    size_t dotPos = primaryPath.find_last_of('.');
    if (dotPos == std::string::npos || (dirPos != std::string::npos && dotPos < dirPos)) {
        return primaryPath + "_secondary";
    }
    return primaryPath.substr(0, dotPos) + "_secondary" + primaryPath.substr(dotPos);
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr uint32_t kSurfaceUsage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
constexpr uint8_t kFramePatternBase = 0x20;
constexpr int32_t kColorBlockSize = 64;
constexpr int32_t kFrameLabelX = 8;
constexpr int32_t kFrameLabelY = 8;
constexpr int32_t kFrameLabelScale = 4;
constexpr int32_t kFrameLabelGlyphWidth = 5;
constexpr int32_t kFrameLabelGlyphHeight = 7;
constexpr int32_t kFrameLabelGlyphSpacing = 1;
constexpr int32_t kFrameLabelPadding = 4;
constexpr uint8_t kTextWhiteY = 235;
constexpr uint8_t kTextBlackY = 16;
constexpr uint8_t kTextNeutralUv = 128;

struct RgbColor {
    uint8_t r;
    uint8_t g;
    uint8_t b;
};

struct YuvColor {
    uint8_t y;
    uint8_t u;
    uint8_t v;
};

constexpr std::array<RgbColor, 7> SPECTRUM_COLORS = {{
    {255, 0, 0},     // Red
    {255, 127, 0},   // Orange
    {255, 255, 0},   // Yellow
    {0, 255, 0},     // Green
    {0, 255, 255},   // Cyan
    {0, 0, 255},     // Blue
    {139, 0, 255},   // Violet
}};

uint8_t ClampToByte(int32_t value)
{
    if (value < 0) {
        return 0;
    }
    if (value > 255) {
        return 255;
    }
    return static_cast<uint8_t>(value);
}

YuvColor RgbToYuv(const RgbColor &color)
{
    YuvColor yuv {};
    yuv.y = ClampToByte(((66 * color.r + 129 * color.g + 25 * color.b + 128) >> 8) + 16);
    yuv.u = ClampToByte(((-38 * color.r - 74 * color.g + 112 * color.b + 128) >> 8) + 128);
    yuv.v = ClampToByte(((112 * color.r - 94 * color.g - 18 * color.b + 128) >> 8) + 128);
    return yuv;
}

const RgbColor &GetSpectrumColor(int32_t x, int32_t y, int32_t width, uint8_t pattern)
{
    int32_t blocksPerRow = (width + kColorBlockSize - 1) / kColorBlockSize;
    size_t blockIndex = static_cast<size_t>((y / kColorBlockSize) * blocksPerRow + (x / kColorBlockSize) + pattern);
    return SPECTRUM_COLORS[blockIndex % SPECTRUM_COLORS.size()];
}

bool FillYuv420Sp(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                  uint8_t pattern, bool cbFirst)
{
    if (base == nullptr || width <= 0 || height <= 0 || stride < width || sliceHeight < height) {
        return false;
    }

    for (int32_t y = 0; y < height; ++y) {
        uint8_t *row = base + static_cast<size_t>(y) * stride;
        for (int32_t x = 0; x < width; ++x) {
            row[x] = RgbToYuv(GetSpectrumColor(x, y, width, pattern)).y;
        }
    }

    uint8_t *uvPlane = base + static_cast<size_t>(stride) * sliceHeight;
    for (int32_t y = 0; y < height; y += 2) {
        uint8_t *row = uvPlane + static_cast<size_t>(y / 2) * stride;
        for (int32_t x = 0; x < width; x += 2) {
            YuvColor yuv = RgbToYuv(GetSpectrumColor(x, y, width, pattern));
            row[x] = cbFirst ? yuv.u : yuv.v;
            if (x + 1 < stride) {
                row[x + 1] = cbFirst ? yuv.v : yuv.u;
            }
        }
    }
    return true;
}

bool FillYuv420P(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                 uint8_t pattern)
{
    int32_t chromaWidth = (width + 1) / 2;
    int32_t chromaStride = stride / 2;
    int32_t chromaSliceHeight = (sliceHeight + 1) / 2;
    if (base == nullptr || width <= 0 || height <= 0 || stride < width || chromaStride < chromaWidth ||
        sliceHeight < height) {
        return false;
    }

    for (int32_t y = 0; y < height; ++y) {
        uint8_t *row = base + static_cast<size_t>(y) * stride;
        for (int32_t x = 0; x < width; ++x) {
            row[x] = RgbToYuv(GetSpectrumColor(x, y, width, pattern)).y;
        }
    }

    uint8_t *uPlane = base + static_cast<size_t>(stride) * sliceHeight;
    uint8_t *vPlane = uPlane + static_cast<size_t>(chromaStride) * chromaSliceHeight;
    for (int32_t y = 0; y < height; y += 2) {
        uint8_t *uRow = uPlane + static_cast<size_t>(y / 2) * chromaStride;
        uint8_t *vRow = vPlane + static_cast<size_t>(y / 2) * chromaStride;
        for (int32_t x = 0; x < width; x += 2) {
            YuvColor yuv = RgbToYuv(GetSpectrumColor(x, y, width, pattern));
            int32_t chromaX = x / 2;
            uRow[chromaX] = yuv.u;
            vRow[chromaX] = yuv.v;
        }
    }
    return true;
}

bool FillRgba8888(uint8_t *base, int32_t width, int32_t height, int32_t stride, uint8_t pattern, bool redFirst)
{
    constexpr int32_t bytesPerPixel = 4;
    if (base == nullptr || width <= 0 || height <= 0 || stride < width * bytesPerPixel) {
        return false;
    }

    for (int32_t y = 0; y < height; ++y) {
        uint8_t *row = base + static_cast<size_t>(y) * stride;
        for (int32_t x = 0; x < width; ++x) {
            const RgbColor &color = GetSpectrumColor(x, y, width, pattern);
            uint8_t *pixel = row + static_cast<size_t>(x) * bytesPerPixel;
            pixel[0] = redFirst ? color.r : color.b;
            pixel[1] = color.g;
            pixel[2] = redFirst ? color.b : color.r;
            pixel[3] = 0xFF;
        }
    }
    return true;
}

uint16_t ToP010Sample(uint8_t value)
{
    return static_cast<uint16_t>(value) << 8;
}

std::string GetFrameLabel(uint32_t frameNumber)
{
    return "F" + std::to_string(frameNumber);
}

std::array<uint8_t, kFrameLabelGlyphHeight> GetGlyphRows(char value)
{
    switch (value) {
        case 'F':
            return {0x1F, 0x10, 0x10, 0x1E, 0x10, 0x10, 0x10};
        case '0':
            return {0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E};
        case '1':
            return {0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E};
        case '2':
            return {0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F};
        case '3':
            return {0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E};
        case '4':
            return {0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02};
        case '5':
            return {0x1F, 0x10, 0x10, 0x1E, 0x01, 0x01, 0x1E};
        case '6':
            return {0x0E, 0x10, 0x10, 0x1E, 0x11, 0x11, 0x0E};
        case '7':
            return {0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08};
        case '8':
            return {0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E};
        case '9':
            return {0x0E, 0x11, 0x11, 0x0F, 0x01, 0x01, 0x0E};
        default:
            return {0, 0, 0, 0, 0, 0, 0};
    }
}

void DrawYuv420SpPixel(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                       int32_t x, int32_t y, bool foreground, bool cbFirst)
{
    if (base == nullptr || x < 0 || y < 0 || x >= width || y >= height || stride <= 0 || sliceHeight <= 0) {
        return;
    }
    base[static_cast<size_t>(y) * stride + x] = foreground ? kTextWhiteY : kTextBlackY;

    uint8_t *uvPlane = base + static_cast<size_t>(stride) * sliceHeight;
    uint8_t *uvRow = uvPlane + static_cast<size_t>(y / 2) * stride;
    int32_t uvX = (x / 2) * 2;
    if (uvX + 1 >= stride) {
        return;
    }
    uvRow[uvX] = kTextNeutralUv;
    uvRow[uvX + 1] = kTextNeutralUv;
    (void)cbFirst;
}

void DrawYuv420PPixel(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                      int32_t x, int32_t y, bool foreground)
{
    if (base == nullptr || x < 0 || y < 0 || x >= width || y >= height || stride <= 0 || sliceHeight <= 0) {
        return;
    }
    base[static_cast<size_t>(y) * stride + x] = foreground ? kTextWhiteY : kTextBlackY;

    int32_t chromaStride = stride / 2;
    int32_t chromaSliceHeight = (sliceHeight + 1) / 2;
    int32_t chromaX = x / 2;
    int32_t chromaY = y / 2;
    if (chromaStride <= 0 || chromaX >= chromaStride || chromaY >= chromaSliceHeight) {
        return;
    }
    uint8_t *uPlane = base + static_cast<size_t>(stride) * sliceHeight;
    uint8_t *vPlane = uPlane + static_cast<size_t>(chromaStride) * chromaSliceHeight;
    uPlane[static_cast<size_t>(chromaY) * chromaStride + chromaX] = kTextNeutralUv;
    vPlane[static_cast<size_t>(chromaY) * chromaStride + chromaX] = kTextNeutralUv;
}

void DrawYuvP010Pixel(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                      int32_t x, int32_t y, bool foreground, bool cbFirst)
{
    constexpr int32_t bytesPerSample = 2;
    int32_t samplesPerRow = stride / bytesPerSample;
    if (base == nullptr || x < 0 || y < 0 || x >= width || y >= height || x >= samplesPerRow ||
        stride <= 0 || sliceHeight <= 0) {
        return;
    }
    uint16_t *row = reinterpret_cast<uint16_t *>(base + static_cast<size_t>(y) * stride);
    row[x] = ToP010Sample(foreground ? kTextWhiteY : kTextBlackY);

    uint8_t *uvPlane = base + static_cast<size_t>(stride) * sliceHeight;
    uint16_t *uvRow = reinterpret_cast<uint16_t *>(uvPlane + static_cast<size_t>(y / 2) * stride);
    int32_t uvX = (x / 2) * 2;
    if (uvX + 1 >= samplesPerRow) {
        return;
    }
    uvRow[uvX] = ToP010Sample(kTextNeutralUv);
    uvRow[uvX + 1] = ToP010Sample(kTextNeutralUv);
    (void)cbFirst;
}

void DrawRgba8888Pixel(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t x, int32_t y,
                       bool foreground)
{
    constexpr int32_t bytesPerPixel = 4;
    if (base == nullptr || x < 0 || y < 0 || x >= width || y >= height || stride < width * bytesPerPixel) {
        return;
    }
    uint8_t value = foreground ? 0xFF : 0x00;
    uint8_t *pixel = base + static_cast<size_t>(y) * stride + static_cast<size_t>(x) * bytesPerPixel;
    pixel[0] = value;
    pixel[1] = value;
    pixel[2] = value;
    pixel[3] = 0xFF;
}

void DrawRgba1010102Pixel(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t x, int32_t y,
                          bool foreground)
{
    constexpr int32_t bytesPerPixel = 4;
    constexpr uint32_t alpha = 0x3;
    if (base == nullptr || x < 0 || y < 0 || x >= width || y >= height || stride < width * bytesPerPixel) {
        return;
    }
    uint32_t value = foreground ? 0x3FC : 0x000;
    uint32_t *row = reinterpret_cast<uint32_t *>(base + static_cast<size_t>(y) * stride);
    row[x] = value | (value << 10) | (value << 20) | (alpha << 30);
}

void DrawFrameLabelPixel(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                         int32_t format, int32_t x, int32_t y, bool foreground)
{
    switch (format) {
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP:
            DrawYuv420SpPixel(base, width, height, stride, sliceHeight, x, y, foreground, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP:
            DrawYuv420SpPixel(base, width, height, stride, sliceHeight, x, y, foreground, false);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P:
            DrawYuv420PPixel(base, width, height, stride, sliceHeight, x, y, foreground);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_P010:
            DrawYuvP010Pixel(base, width, height, stride, sliceHeight, x, y, foreground, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_P010:
            DrawYuvP010Pixel(base, width, height, stride, sliceHeight, x, y, foreground, false);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
            DrawRgba8888Pixel(base, width, height, stride, x, y, foreground);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_1010102:
            DrawRgba1010102Pixel(base, width, height, stride, x, y, foreground);
            break;
        default:
            break;
    }
}

void DrawSolidRect(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                   int32_t format, int32_t left, int32_t top, int32_t rectWidth, int32_t rectHeight,
                   bool foreground)
{
    for (int32_t y = top; y < top + rectHeight; ++y) {
        for (int32_t x = left; x < left + rectWidth; ++x) {
            DrawFrameLabelPixel(base, width, height, stride, sliceHeight, format, x, y, foreground);
        }
    }
}

void DrawFrameNumber(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                     int32_t format, uint32_t frameNumber)
{
    std::string label = GetFrameLabel(frameNumber);
    int32_t glyphAdvance = (kFrameLabelGlyphWidth + kFrameLabelGlyphSpacing) * kFrameLabelScale;
    int32_t backgroundWidth = static_cast<int32_t>(label.size()) * glyphAdvance + kFrameLabelPadding * 2;
    int32_t backgroundHeight = kFrameLabelGlyphHeight * kFrameLabelScale + kFrameLabelPadding * 2;
    DrawSolidRect(base, width, height, stride, sliceHeight, format, kFrameLabelX, kFrameLabelY, backgroundWidth,
                  backgroundHeight, false);

    int32_t cursorX = kFrameLabelX + kFrameLabelPadding;
    int32_t cursorY = kFrameLabelY + kFrameLabelPadding;
    for (char value : label) {
        auto rows = GetGlyphRows(value);
        for (int32_t rowIndex = 0; rowIndex < kFrameLabelGlyphHeight; ++rowIndex) {
            for (int32_t colIndex = 0; colIndex < kFrameLabelGlyphWidth; ++colIndex) {
                bool isSet = (rows[rowIndex] & (1 << (kFrameLabelGlyphWidth - 1 - colIndex))) != 0;
                if (!isSet) {
                    continue;
                }
                int32_t dotX = cursorX + colIndex * kFrameLabelScale;
                int32_t dotY = cursorY + rowIndex * kFrameLabelScale;
                DrawSolidRect(base, width, height, stride, sliceHeight, format, dotX, dotY, kFrameLabelScale,
                              kFrameLabelScale, true);
            }
        }
        cursorX += glyphAdvance;
    }
}

bool FillYuvP010(uint8_t *base, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                 uint8_t pattern, bool cbFirst)
{
    constexpr int32_t bytesPerSample = 2;
    int32_t samplesPerRow = stride / bytesPerSample;
    if (base == nullptr || width <= 0 || height <= 0 || stride < width * bytesPerSample ||
        sliceHeight < height) {
        return false;
    }

    for (int32_t y = 0; y < height; ++y) {
        uint16_t *row = reinterpret_cast<uint16_t *>(base + static_cast<size_t>(y) * stride);
        for (int32_t x = 0; x < width; ++x) {
            row[x] = ToP010Sample(RgbToYuv(GetSpectrumColor(x, y, width, pattern)).y);
        }
    }

    uint8_t *uvPlane = base + static_cast<size_t>(stride) * sliceHeight;
    for (int32_t y = 0; y < height; y += 2) {
        uint16_t *row = reinterpret_cast<uint16_t *>(uvPlane + static_cast<size_t>(y / 2) * stride);
        for (int32_t x = 0; x < width; x += 2) {
            YuvColor yuv = RgbToYuv(GetSpectrumColor(x, y, width, pattern));
            row[x] = ToP010Sample(cbFirst ? yuv.u : yuv.v);
            if (x + 1 < samplesPerRow) {
                row[x + 1] = ToP010Sample(cbFirst ? yuv.v : yuv.u);
            }
        }
    }
    return true;
}

bool FillRgba1010102(uint8_t *base, int32_t width, int32_t height, int32_t stride, uint8_t pattern)
{
    constexpr int32_t bytesPerPixel = 4;
    constexpr uint32_t alpha = 0x3;
    if (base == nullptr || width <= 0 || height <= 0 || stride < width * bytesPerPixel) {
        return false;
    }

    for (int32_t y = 0; y < height; ++y) {
        uint32_t *row = reinterpret_cast<uint32_t *>(base + static_cast<size_t>(y) * stride);
        for (int32_t x = 0; x < width; ++x) {
            const RgbColor &color = GetSpectrumColor(x, y, width, pattern);
            uint32_t r = static_cast<uint32_t>(color.r) << 2;
            uint32_t g = static_cast<uint32_t>(color.g) << 2;
            uint32_t b = static_cast<uint32_t>(color.b) << 2;
            row[x] = r | (g << 10) | (b << 20) | (alpha << 30);
        }
    }
    return true;
}

bool FillSpectrumBlocks(void *addr, int32_t width, int32_t height, int32_t stride, int32_t sliceHeight,
                        int32_t format, uint8_t pattern, uint32_t frameNumber)
{
    uint8_t *base = static_cast<uint8_t *>(addr);
    bool filled = false;
    switch (format) {
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP:
            filled = FillYuv420Sp(base, width, height, stride, sliceHeight, pattern, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP:
            filled = FillYuv420Sp(base, width, height, stride, sliceHeight, pattern, false);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P:
            filled = FillYuv420P(base, width, height, stride, sliceHeight, pattern);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
            filled = FillRgba8888(base, width, height, stride, pattern, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_1010102:
            filled = FillRgba1010102(base, width, height, stride, pattern);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_P010:
            filled = FillYuvP010(base, width, height, stride, sliceHeight, pattern, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_P010:
            filled = FillYuvP010(base, width, height, stride, sliceHeight, pattern, false);
            break;
        default:
            return false;
    }
    if (!filled) {
        return false;
    }
    DrawFrameNumber(base, width, height, stride, sliceHeight, format, frameNumber);
    return true;
}
} // namespace

VideoEncPreprocDualOutputSample::~VideoEncPreprocDualOutputSample()
{
    if (secondary_ != nullptr) {
        (void)OH_VideoEncoder_Destroy(secondary_);
        secondary_ = nullptr;
    }
    if (primary_ != nullptr) {
        (void)OH_VideoEncoder_Destroy(primary_);
        primary_ = nullptr;
    }
    ReleaseWindow();
    if (primaryOutputFile_.is_open()) {
        primaryOutputFile_.close();
    }
    if (secondaryOutputFile_.is_open()) {
        secondaryOutputFile_.close();
    }
}

void VideoEncPreprocDualOutputSample::ResetStates()
{
    primaryState_.codec = nullptr;
    primaryState_.dataCount.store(0);
    primaryState_.gotEos.store(false);
    primaryState_.errorCode.store(0);
    secondaryState_.codec = nullptr;
    secondaryState_.dataCount.store(0);
    secondaryState_.gotEos.store(false);
    secondaryState_.errorCode.store(0);
}

void VideoEncPreprocDualOutputSample::ReleaseWindow()
{
    if (window_ != nullptr) {
        (void)OH_NativeWindow_DestroyNativeWindow(window_);
        window_ = nullptr;
    }
}

void VideoEncPreprocDualOutputSample::OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    auto *state = reinterpret_cast<EncoderState *>(userData);
    if (state != nullptr) {
        if (state->eosMutex != nullptr) {
            std::lock_guard<std::mutex> lock(*state->eosMutex);
            state->errorCode.store(errorCode);
        } else {
            state->errorCode.store(errorCode);
        }
        if (state->eosCv != nullptr) {
            state->eosCv->notify_all();
        }
    }
}

void VideoEncPreprocDualOutputSample::OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}

void VideoEncPreprocDualOutputSample::OnNeedOutputData(OH_AVCodec *codec, uint32_t index,
                                                       OH_AVBuffer *buffer, void *userData)
{
    if (!codec || !buffer || !userData) {
        return;
    }
    auto *state = reinterpret_cast<EncoderState *>(userData);
    OH_AVCodecBufferAttr attr;
    OH_AVBuffer_GetBufferAttr(buffer, &attr);
    if ((attr.flags & AVCODEC_BUFFER_FLAGS_EOS) == 0) {
        if (attr.flags != AVCODEC_BUFFER_FLAGS_CODEC_DATA) {
            state->dataCount.fetch_add(1);
        }
        DumpOutputBuffer(buffer, attr, *state);
    } else {
        state->gotEos.store(true);
        state->eosCv->notify_all();
        return;
    }
    if (codec != nullptr) {
        (void)OH_VideoEncoder_FreeOutputBuffer(codec, index);
    }
}

void VideoEncPreprocDualOutputSample::DumpOutputBuffer(OH_AVBuffer *buffer, const OH_AVCodecBufferAttr &attr,
                                                       EncoderState &state)
{
    if (buffer == nullptr || attr.size <= 0 || state.outputFile == nullptr) {
        return;
    }
    uint8_t *addr = OH_AVBuffer_GetAddr(buffer);
    if (addr == nullptr) {
        return;
    }
    if (state.outputFile->is_open()) {
        state.outputFile->write(reinterpret_cast<const char *>(addr + attr.offset), attr.size);
        state.outputFile->flush();
    }
}

int32_t VideoEncPreprocDualOutputSample::RegisterCallback(OH_AVCodec *codec, EncoderState &state)
{
    if (codec == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    OH_AVCodecCallback callback {};
    callback.onError = OnError;
    callback.onStreamChanged = OnStreamChanged;
    callback.onNeedInputBuffer = nullptr;
    callback.onNewOutputBuffer = OnNeedOutputData;
    state.codec = codec;
    state.eosMutex = &eosMutex_;
    state.eosCv = &eosCv_;
    if (&state == &primaryState_) {
        state.outputFile = &primaryOutputFile_;
    } else {
        state.outputFile = &secondaryOutputFile_;
    }
    return OH_VideoEncoder_RegisterCallback(codec, callback, &state);
}

bool VideoEncPreprocDualOutputSample::SetOutputFilePath(const std::string &path)
{
    if (primaryOutputFile_.is_open()) {
        primaryOutputFile_.close();
    }
    if (secondaryOutputFile_.is_open()) {
        secondaryOutputFile_.close();
    }
    if (!EnsureOutputDir(path)) {
        return false;
    }
    std::string secondaryPath = MakeSecondaryOutputPath(path);
    if (!EnsureOutputDir(secondaryPath)) {
        return false;
    }
    primaryOutputFile_.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
    secondaryOutputFile_.open(secondaryPath, std::ios::out | std::ios::binary | std::ios::trunc);
    return primaryOutputFile_.is_open() && secondaryOutputFile_.is_open();
}

bool VideoEncPreprocDualOutputSample::ApplyPreprocKeys(OH_AVFormat *format,
                                                       const PreprocDualOutput::PreprocConfig &config)
{
    if (format == nullptr) {
        return false;
    }
    if (config.useDownsampling) {
        if (!OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH,
                                     config.downsamplingWidth) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT,
                                     config.downsamplingHeight)) {
            return false;
        }
    }
    if (config.useCrop) {
        if (!OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, config.cropLeft) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, config.cropTop) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, config.cropRight) ||
            !OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, config.cropBottom)) {
            return false;
        }
    }
    if (config.useDropToFrameRate) {
        if (!OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DROP_TO_FRAME_RATE,
                                        config.dropToFrameRate)) {
            return false;
        }
    }
    return true;
}

bool VideoEncPreprocDualOutputSample::ApplyPreprocConfig(OH_AVFormat *format,
                                                         const PreprocDualOutput::PreprocConfig &config,
                                                         const PreprocDualOutput::ScenarioCase &scenario)
{
    if (format == nullptr) {
        return false;
    }
    if (!OH_AVFormat_SetDoubleValue(format, OH_MD_KEY_FRAME_RATE, config.frameRate)) {
        return false;
    }
    if (!ApplyPreprocKeys(format, config)) {
        return false;
    }
    return OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, scenario.width) &&
           OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, scenario.height) &&
           OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, scenario.codecPixelFormat);
}

int32_t VideoEncPreprocDualOutputSample::BuildAndConfigure(OH_AVCodec *codec,
                                                           const PreprocDualOutput::PreprocConfig &config,
                                                           const PreprocDualOutput::ScenarioCase &scenario)
{
    if (codec == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        return AV_ERR_NO_MEMORY;
    }
    int32_t ret = AV_ERR_OK;
    if (!ApplyPreprocConfig(format, config, scenario)) {
        ret = AV_ERR_UNKNOWN;
    } else {
        ret = OH_VideoEncoder_Configure(codec, format);
    }
    OH_AVFormat_Destroy(format);
    return ret;
}

int32_t VideoEncPreprocDualOutputSample::SetPreprocParameter(OH_AVCodec *codec,
                                                            const PreprocDualOutput::PreprocConfig &config)
{
    if (codec == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    OH_AVFormat *format = OH_AVFormat_Create();
    if (format == nullptr) {
        return AV_ERR_NO_MEMORY;
    }
    int32_t ret = AV_ERR_OK;
    if (!ApplyPreprocKeys(format, config)) {
        ret = AV_ERR_UNKNOWN;
    } else {
        ret = OH_VideoEncoder_SetParameter(codec, format);
    }
    OH_AVFormat_Destroy(format);
    return ret;
}

bool VideoEncPreprocDualOutputSample::CreatePrimary(const std::string &mime)
{
    ResetStates();
    if (OH_VideoEncoder_CreatePrimaryWithPreproc(mime.c_str(), &primary_) != AV_ERR_OK || primary_ == nullptr) {
        return false;
    }
    return RegisterCallback(primary_, primaryState_) == AV_ERR_OK;
}

int32_t VideoEncPreprocDualOutputSample::ConfigurePrimary(const PreprocDualOutput::ScenarioCase &scenario)
{
    return BuildAndConfigure(primary_, scenario.primary, scenario);
}

int32_t VideoEncPreprocDualOutputSample::GetPrimarySurface(const PreprocDualOutput::ScenarioCase &scenario)
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    int32_t ret = OH_VideoEncoder_GetSurface(primary_, &window_);
    if (ret != AV_ERR_OK || window_ == nullptr) {
        return ret;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_FORMAT, scenario.nativeFormat);
    if (ret != 0) {
        return AV_ERR_UNKNOWN;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_BUFFER_GEOMETRY, scenario.width, scenario.height);
    if (ret != 0) {
        return AV_ERR_UNKNOWN;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_USAGE, kSurfaceUsage);
    if (ret != 0) {
        return AV_ERR_UNKNOWN;
    }
    return AV_ERR_OK;
}

int32_t VideoEncPreprocDualOutputSample::StartPrimary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Start(primary_);
}

int32_t VideoEncPreprocDualOutputSample::CreateSecondary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    int32_t ret = OH_VideoEncoder_CreateSecondaryFromPrimary(primary_, &secondary_);
    if (ret != AV_ERR_OK || secondary_ == nullptr) {
        return ret;
    }
    return RegisterCallback(secondary_, secondaryState_);
}

int32_t VideoEncPreprocDualOutputSample::ConfigureSecondary(const PreprocDualOutput::ScenarioCase &scenario)
{
    return BuildAndConfigure(secondary_, scenario.secondary, scenario);
}

int32_t VideoEncPreprocDualOutputSample::StartSecondary()
{
    if (secondary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Start(secondary_);
}

int32_t VideoEncPreprocDualOutputSample::NotifyPrimaryEos()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_NotifyEndOfStream(primary_);
}

int32_t VideoEncPreprocDualOutputSample::NotifySecondaryEos()
{
    if (secondary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_NotifyEndOfStream(secondary_);
}

int32_t VideoEncPreprocDualOutputSample::DuplicateSecondary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    OH_AVCodec *duplicate = nullptr;
    int32_t ret = OH_VideoEncoder_CreateSecondaryFromPrimary(primary_, &duplicate);
    if (duplicate != nullptr) {
        (void)OH_VideoEncoder_Destroy(duplicate);
    }
    return ret;
}

int32_t VideoEncPreprocDualOutputSample::DestroyPrimary()
{
    if (primary_ == nullptr) {
        return AV_ERR_OK;
    }
    int32_t ret = OH_VideoEncoder_Destroy(primary_);
    primary_ = nullptr;
    primaryState_.codec = nullptr;
    return ret;
}

int32_t VideoEncPreprocDualOutputSample::DestroySecondary()
{
    if (secondary_ == nullptr) {
        return AV_ERR_OK;
    }
    int32_t ret = OH_VideoEncoder_Destroy(secondary_);
    secondary_ = nullptr;
    secondaryState_.codec = nullptr;
    return ret;
}

void VideoEncPreprocDualOutputSample::ForgetSecondaryHandle()
{
    secondary_ = nullptr;
    secondaryState_.codec = nullptr;
}

bool VideoEncPreprocDualOutputSample::RequestAndFlushFrame(int32_t width, int32_t height, uint8_t pattern,
                                                           int64_t timestampUs, uint32_t frameNumber)
{
    auto printFailure = [&](const char *step, int32_t err, int32_t format = 0, int32_t stride = 0,
                            int32_t sliceHeight = 0) {
        cout << "RequestAndFlushFrame failed, step=" << step << ", ret=" << err << ", width=" << width
             << ", height=" << height << ", pattern=" << static_cast<int32_t>(pattern)
             << ", frameNumber=" << frameNumber << ", timestampUs=" << timestampUs << ", format=" << format
             << ", stride=" << stride << ", sliceHeight=" << sliceHeight << endl;
    };

    if (window_ == nullptr) {
        printFailure("window_null", AV_ERR_INVALID_VAL);
        return false;
    }

    OHNativeWindowBuffer *nativeWindowBuffer = nullptr;
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(window_, &nativeWindowBuffer, &fenceFd);
    if (ret != 0 || nativeWindowBuffer == nullptr) {
        printFailure("request_buffer", ret);
        return false;
    }
    if (fenceFd > 0) {
        close(fenceFd);
    }

    OH_NativeBuffer *nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(nativeWindowBuffer);
    if (nativeBuffer == nullptr) {
        printFailure("native_buffer_from_window_buffer", AV_ERR_INVALID_VAL);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    void *virAddr = nullptr;
    ret = OH_NativeBuffer_Map(nativeBuffer, &virAddr);
    if (ret != 0 || virAddr == nullptr) {
        printFailure("map_buffer", ret);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    const SurfaceBuffer *surfaceBuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(nativeBuffer);
    if (surfaceBuffer == nullptr) {
        printFailure("surface_buffer_from_native_buffer", AV_ERR_INVALID_VAL);
        (void)OH_NativeBuffer_Unmap(nativeBuffer);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    int32_t format = surfaceBuffer->GetFormat();
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, GET_FORMAT, &format);
    if (ret != 0) {
        cout << "RequestAndFlushFrame warning, step=get_format, ret=" << ret << ", fallbackFormat=" << format
             << ", width=" << width << ", height=" << height << ", pattern=" << static_cast<int32_t>(pattern)
             << ", frameNumber=" << frameNumber << ", timestampUs=" << timestampUs << endl;
    }

    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig(nativeBuffer, &config);
    int32_t stride = config.stride;
    const int32_t sliceHeight = surfaceBuffer->GetHeight();
    if (stride <= 0 || sliceHeight <= 0) {
        printFailure("invalid_buffer_config", AV_ERR_INVALID_VAL, format, stride, sliceHeight);
        (void)OH_NativeBuffer_Unmap(nativeBuffer);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    if (!FillSpectrumBlocks(virAddr, width, height, stride, sliceHeight, format, pattern, frameNumber)) {
        printFailure("fill_spectrum_blocks", AV_ERR_INVALID_VAL, format, stride, sliceHeight);
        (void)OH_NativeBuffer_Unmap(nativeBuffer);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_UI_TIMESTAMP, timestampUs);
    if (ret != 0) {
        cout << "RequestAndFlushFrame warning, step=set_ui_timestamp, ret=" << ret << ", width=" << width
             << ", height=" << height << ", pattern=" << static_cast<int32_t>(pattern)
             << ", frameNumber=" << frameNumber << ", timestampUs=" << timestampUs << ", format=" << format
             << ", stride=" << stride << ", sliceHeight=" << sliceHeight << endl;
    }
    ret = OH_NativeBuffer_Unmap(nativeBuffer);
    if (ret != 0) {
        printFailure("unmap_buffer", ret, format, stride, sliceHeight);
        (void)NativeWindowCancelBuffer(window_, nativeWindowBuffer);
        return false;
    }

    Region region {};
    Region::Rect rect {};
    rect.x = 0;
    rect.y = 0;
    rect.w = width;
    rect.h = height;
    region.rects = &rect;
    region.rectNumber = 1;
    ret = OH_NativeWindow_NativeWindowFlushBuffer(window_, nativeWindowBuffer, -1, region);
    if (ret != 0) {
        printFailure("flush_buffer", ret, format, stride, sliceHeight);
    }
    return ret == 0;
}

int32_t VideoEncPreprocDualOutputSample::ApplyRuntimeOperation(const PreprocDualOutput::ScenarioCase &scenario,
                                                              uint32_t frameIndex, uint32_t frameCount,
                                                              int32_t &frameWidth, int32_t &frameHeight)
{
    if (frameIndex != frameCount / 2) {
        return AV_ERR_OK;
    }
    if (scenario.runtimeOperation == PreprocDualOutput::RuntimeOperation::SET_PREPROC_PARAMETER) {
        int32_t ret = AV_ERR_OK;
        if (primary_ != nullptr) {
            ret = SetPreprocParameter(primary_, scenario.runtimePrimary);
            if (ret != AV_ERR_OK) {
                return ret;
            }
        }
        if (secondary_ != nullptr) {
            ret = SetPreprocParameter(secondary_, scenario.runtimeSecondary);
            if (ret != AV_ERR_OK) {
                return ret;
            }
        }
    } else if (scenario.runtimeOperation == PreprocDualOutput::RuntimeOperation::CHANGE_BUFFER_GEOMETRY) {
        if (window_ == nullptr) {
            return AV_ERR_INVALID_VAL;
        }
        int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_BUFFER_GEOMETRY, scenario.runtimeWidth,
                                                           scenario.runtimeHeight);
        if (ret != 0) {
            return AV_ERR_UNKNOWN;
        }
        frameWidth = scenario.runtimeWidth;
        frameHeight = scenario.runtimeHeight;
    }
    return AV_ERR_OK;
}

int32_t VideoEncPreprocDualOutputSample::SendFrames(const PreprocDualOutput::ScenarioCase &scenario,
                                                   uint32_t frameCount)
{
    if (window_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    bool notifyPrimary = primary_ != nullptr;
    bool notifySecondary = secondary_ != nullptr;
    if (scenario.encoderMode == PreprocDualOutput::EncoderEnableMode::PRIMARY_SECONDARY_SEND_PRIMARY_ONLY) {
        notifySecondary = false;
    } else if (scenario.encoderMode == PreprocDualOutput::EncoderEnableMode::PRIMARY_SECONDARY_SEND_SECONDARY_ONLY) {
        notifyPrimary = false;
    }

    bool geometryUpdated = false;
    for (uint32_t i = 0; i < frameCount; ++i) {
        int32_t frameWidth = scenario.width;
        int32_t frameHeight = scenario.height;
        if (geometryUpdated) {
            frameWidth = scenario.runtimeWidth;
            frameHeight = scenario.runtimeHeight;
        }
        int32_t ret = ApplyRuntimeOperation(scenario, i, frameCount, frameWidth, frameHeight);
        if (ret != AV_ERR_OK) {
            return ret;
        }
        if (scenario.runtimeOperation == PreprocDualOutput::RuntimeOperation::CHANGE_BUFFER_GEOMETRY &&
            i >= frameCount / 2) {
            geometryUpdated = true;
        }
        if (!RequestAndFlushFrame(frameWidth, frameHeight, static_cast<uint8_t>(kFramePatternBase + i), GetTimeUs(),
                                  i + 1)) {
            return AV_ERR_UNKNOWN;
        }
        usleep(1000);
    }
    if (notifyPrimary) {
        int32_t ret = NotifyPrimaryEos();
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    if (notifySecondary) {
        int32_t ret = NotifySecondaryEos();
        if (ret != AV_ERR_OK) {
            return ret;
        }
    }
    return AV_ERR_OK;
}

bool VideoEncPreprocDualOutputSample::WaitForDualEos(uint32_t timeoutSeconds)
{
    unique_lock<mutex> lock(eosMutex_);
    (void)eosCv_.wait_for(lock, chrono::seconds(timeoutSeconds), [this]() {
        return (primaryState_.gotEos.load() && secondaryState_.gotEos.load()) ||
               primaryState_.errorCode.load() != 0 || secondaryState_.errorCode.load() != 0;
    });
    return primaryState_.gotEos.load() && secondaryState_.gotEos.load();
}

bool VideoEncPreprocDualOutputSample::WaitForPrimaryEos(uint32_t timeoutSeconds)
{
    unique_lock<mutex> lock(eosMutex_);
    (void)eosCv_.wait_for(lock, chrono::seconds(timeoutSeconds), [this]() {
        return primaryState_.gotEos.load() || primaryState_.errorCode.load() != 0;
    });
    return primaryState_.gotEos.load();
}

bool VideoEncPreprocDualOutputSample::WaitForSecondaryEos(uint32_t timeoutSeconds)
{
    unique_lock<mutex> lock(eosMutex_);
    (void)eosCv_.wait_for(lock, chrono::seconds(timeoutSeconds), [this]() {
        return secondaryState_.gotEos.load() || secondaryState_.errorCode.load() != 0;
    });
    return secondaryState_.gotEos.load();
}

int32_t VideoEncPreprocDualOutputSample::StopPrimary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Stop(primary_);
}

int32_t VideoEncPreprocDualOutputSample::StopSecondary()
{
    if (secondary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Stop(secondary_);
}

int32_t VideoEncPreprocDualOutputSample::FlushPrimary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Flush(primary_);
}

int32_t VideoEncPreprocDualOutputSample::FlushSecondary()
{
    if (secondary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Flush(secondary_);
}

int32_t VideoEncPreprocDualOutputSample::ResetPrimary()
{
    if (primary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Reset(primary_);
}

int32_t VideoEncPreprocDualOutputSample::ResetSecondary()
{
    if (secondary_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    return OH_VideoEncoder_Reset(secondary_);
}

int32_t VideoEncPreprocDualOutputSample::SetParameterPrimary(const PreprocDualOutput::PreprocConfig &config)
{
    return SetPreprocParameter(primary_, config);
}

int32_t VideoEncPreprocDualOutputSample::SetParameterSecondary(const PreprocDualOutput::PreprocConfig &config)
{
    return SetPreprocParameter(secondary_, config);
}

int32_t VideoEncPreprocDualOutputSample::NotifyEndOfStream()
{
    auto ret = OH_VideoEncoder_NotifyEndOfStream(primary_);
    if (ret != AV_ERR_OK) {
        return ret;
    }
    if (secondary_ != nullptr) {
        ret = OH_VideoEncoder_NotifyEndOfStream(secondary_);
    }
    return ret;
}

int32_t VideoEncPreprocDualOutputSample::SendFramesFixedPts(const PreprocDualOutput::ScenarioCase &scenario,
                                                             uint32_t frameCount, int64_t ptsUs)
{
    if (window_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    for (uint32_t i = 0; i < frameCount; ++i) {
        if (!RequestAndFlushFrame(scenario.width, scenario.height, static_cast<uint8_t>(kFramePatternBase + i),
                                  ptsUs, i + 1)) {
            return AV_ERR_UNKNOWN;
        }
        usleep(1000);
    }
    auto ret = NotifyPrimaryEos();
    if (ret != AV_ERR_OK) {
        return ret;
    }
    if (secondary_ != nullptr) {
        ret = NotifySecondaryEos();
    }
    return ret;
}

void VideoEncPreprocDualOutputSample::ResetDataCounts()
{
    primaryState_.dataCount.store(0);
    primaryState_.gotEos.store(false);
    primaryState_.errorCode.store(0);
    secondaryState_.dataCount.store(0);
    secondaryState_.gotEos.store(false);
    secondaryState_.errorCode.store(0);

}

bool VideoEncPreprocDualOutputSample::PrimaryHasData() const
{
    return primaryState_.dataCount.load() > 0;
}

bool VideoEncPreprocDualOutputSample::SecondaryHasData() const
{
    return secondaryState_.dataCount.load() > 0;
}

bool VideoEncPreprocDualOutputSample::PrimaryGotEos() const
{
    return primaryState_.gotEos.load();
}

bool VideoEncPreprocDualOutputSample::SecondaryGotEos() const
{
    return secondaryState_.gotEos.load();
}

int32_t VideoEncPreprocDualOutputSample::PrimaryError() const
{
    return primaryState_.errorCode.load();
}

int32_t VideoEncPreprocDualOutputSample::SecondaryError() const
{
    return secondaryState_.errorCode.load();
}

int32_t VideoEncPreprocDualOutputSample::PrimaryDataCount() const
{
    return primaryState_.dataCount.load();
}

int32_t VideoEncPreprocDualOutputSample::SecondaryDataCount() const
{
    return secondaryState_.dataCount.load();
}
} // namespace MediaAVCodec
} // namespace OHOS
