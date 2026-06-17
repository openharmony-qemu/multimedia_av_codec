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
#include <unistd.h>

#include "native_buffer.h"
#include "native_buffer_inner.h"
#include "native_avbuffer.h"
#include "surface_buffer.h"

using namespace std;

namespace {
constexpr double K_DEFAULT_INPUT_FRAME_RATE = 30.0;
constexpr int64_t K_US_PER_SECOND = 1000000;
constexpr double K_ROUND_HALF = 0.5;

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

int64_t GetFrameIntervalUs(double frameRate)
{
    double validFrameRate = frameRate > 0.0 ? frameRate : K_DEFAULT_INPUT_FRAME_RATE;
    return static_cast<int64_t>((static_cast<double>(K_US_PER_SECOND) / validFrameRate) + K_ROUND_HALF);
}

int64_t GetFrameTimestampUs(uint32_t frameIndex, double frameRate)
{
    return static_cast<int64_t>(frameIndex) * GetFrameIntervalUs(frameRate);
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr uint32_t K_SURFACE_USAGE = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA;
constexpr uint8_t K_FRAME_PATTERN_BASE = 0x20;
constexpr int32_t K_COLOR_BLOCK_SIZE = 64;
constexpr int32_t K_FRAME_LABEL_X = 8;
constexpr int32_t K_FRAME_LABEL_Y = 8;
constexpr int32_t K_FRAME_LABEL_SCALE = 4;
constexpr int32_t K_FRAME_LABEL_GLYPH_WIDTH = 5;
constexpr int32_t K_FRAME_LABEL_GLYPH_HEIGHT = 7;
constexpr int32_t K_FRAME_LABEL_GLYPH_SPACING = 1;
constexpr int32_t K_FRAME_LABEL_PADDING = 4;
constexpr uint8_t K_TEXT_WHITE_Y = 235;
constexpr uint8_t K_TEXT_BLACK_Y = 16;
constexpr uint8_t K_TEXT_NEUTRAL_UV = 128;
constexpr uint8_t K_BYTE_MAX = 255;
constexpr uint8_t K_FULL_ALPHA_8BIT = 0xFF;
constexpr int32_t K_CHROMA_SAMPLE_STEP = 2;
constexpr int32_t K_RGBA_BYTES_PER_PIXEL = 4;
constexpr int32_t K_RGBA_BLUE_INDEX = 2;
constexpr int32_t K_RGBA_ALPHA_INDEX = 3;
constexpr int32_t K_P010_BYTES_PER_SAMPLE = 2;
constexpr int32_t K_P010_SHIFT = 8;
constexpr int32_t K_RGB10_SHIFT = 2;
constexpr int32_t K_RGB10_GREEN_SHIFT = 10;
constexpr int32_t K_RGB10_BLUE_SHIFT = 20;
constexpr int32_t K_RGB10_ALPHA_SHIFT = 30;
constexpr uint32_t K_RGB10_ALPHA = 0x3;
constexpr uint32_t K_RGB10_TEXT_WHITE = 0x3FC;
constexpr uint32_t K_RGB10_TEXT_BLACK = 0x000;
constexpr useconds_t K_FRAME_INTERVAL_SLEEP_US = 1000;
constexpr uint32_t K_RUNTIME_TRIGGER_DIVISOR = 2;
constexpr int32_t K_YUV_COEFF_SHIFT = 8;
constexpr int32_t K_YUV_OFFSET = 128;
constexpr int32_t K_Y_COEFF_R = 66;
constexpr int32_t K_Y_COEFF_G = 129;
constexpr int32_t K_Y_COEFF_B = 25;
constexpr int32_t K_Y_OFFSET = 16;
constexpr int32_t K_U_COEFF_R = -38;
constexpr int32_t K_U_COEFF_G = -74;
constexpr int32_t K_U_COEFF_B = 112;
constexpr int32_t K_V_COEFF_R = 112;
constexpr int32_t K_V_COEFF_G = -94;
constexpr int32_t K_V_COEFF_B = -18;

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

struct ImageBuffer {
    uint8_t *base;
    int32_t width;
    int32_t height;
    int32_t stride;
    int32_t sliceHeight;
};

struct PixelTarget {
    ImageBuffer image;
    int32_t format;
};

struct RectArea {
    int32_t left;
    int32_t top;
    int32_t width;
    int32_t height;
};

struct FrameRequestInfo {
    int32_t width;
    int32_t height;
    uint8_t pattern;
    int64_t timestampUs;
    uint32_t frameNumber;
};

struct WindowBufferContext {
    OHNativeWindowBuffer *windowBuffer = nullptr;
    OH_NativeBuffer *nativeBuffer = nullptr;
    void *virAddr = nullptr;
    int32_t format = 0;
    int32_t stride = 0;
    int32_t sliceHeight = 0;
};

struct RequestFailureDetail {
    int32_t format = 0;
    int32_t stride = 0;
    int32_t sliceHeight = 0;
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
    if (value > K_BYTE_MAX) {
        return K_BYTE_MAX;
    }
    return static_cast<uint8_t>(value);
}

YuvColor RgbToYuv(const RgbColor &color)
{
    YuvColor yuv {};
    yuv.y = ClampToByte(((K_Y_COEFF_R * color.r + K_Y_COEFF_G * color.g + K_Y_COEFF_B * color.b +
        K_YUV_OFFSET) >> K_YUV_COEFF_SHIFT) + K_Y_OFFSET);
    yuv.u = ClampToByte(((K_U_COEFF_R * color.r + K_U_COEFF_G * color.g + K_U_COEFF_B * color.b +
        K_YUV_OFFSET) >> K_YUV_COEFF_SHIFT) + K_YUV_OFFSET);
    yuv.v = ClampToByte(((K_V_COEFF_R * color.r + K_V_COEFF_G * color.g + K_V_COEFF_B * color.b +
        K_YUV_OFFSET) >> K_YUV_COEFF_SHIFT) + K_YUV_OFFSET);
    return yuv;
}

const RgbColor &GetSpectrumColor(int32_t x, int32_t y, int32_t width, uint8_t pattern)
{
    int32_t blocksPerRow = (width + K_COLOR_BLOCK_SIZE - 1) / K_COLOR_BLOCK_SIZE;
    size_t blockIndex = static_cast<size_t>((y / K_COLOR_BLOCK_SIZE) * blocksPerRow +
                                            (x / K_COLOR_BLOCK_SIZE) + pattern);
    return SPECTRUM_COLORS[blockIndex % SPECTRUM_COLORS.size()];
}

bool FillYuv420Sp(const ImageBuffer &image, uint8_t pattern, bool cbFirst)
{
    if (image.base == nullptr || image.width <= 0 || image.height <= 0 || image.stride < image.width ||
        image.sliceHeight < image.height) {
        return false;
    }

    for (int32_t y = 0; y < image.height; ++y) {
        uint8_t *row = image.base + static_cast<size_t>(y) * image.stride;
        for (int32_t x = 0; x < image.width; ++x) {
            row[x] = RgbToYuv(GetSpectrumColor(x, y, image.width, pattern)).y;
        }
    }

    uint8_t *uvPlane = image.base + static_cast<size_t>(image.stride) * image.sliceHeight;
    for (int32_t y = 0; y < image.height; y += K_CHROMA_SAMPLE_STEP) {
        uint8_t *row = uvPlane + static_cast<size_t>(y / K_CHROMA_SAMPLE_STEP) * image.stride;
        for (int32_t x = 0; x < image.width; x += K_CHROMA_SAMPLE_STEP) {
            YuvColor yuv = RgbToYuv(GetSpectrumColor(x, y, image.width, pattern));
            row[x] = cbFirst ? yuv.u : yuv.v;
            if (x + 1 < image.stride) {
                row[x + 1] = cbFirst ? yuv.v : yuv.u;
            }
        }
    }
    return true;
}

bool FillYuv420P(const ImageBuffer &image, uint8_t pattern)
{
    int32_t chromaWidth = (image.width + 1) / K_CHROMA_SAMPLE_STEP;
    int32_t chromaStride = image.stride / K_CHROMA_SAMPLE_STEP;
    int32_t chromaSliceHeight = (image.sliceHeight + 1) / K_CHROMA_SAMPLE_STEP;
    if (image.base == nullptr || image.width <= 0 || image.height <= 0 || image.stride < image.width ||
        chromaStride < chromaWidth || image.sliceHeight < image.height) {
        return false;
    }

    for (int32_t y = 0; y < image.height; ++y) {
        uint8_t *row = image.base + static_cast<size_t>(y) * image.stride;
        for (int32_t x = 0; x < image.width; ++x) {
            row[x] = RgbToYuv(GetSpectrumColor(x, y, image.width, pattern)).y;
        }
    }

    uint8_t *uPlane = image.base + static_cast<size_t>(image.stride) * image.sliceHeight;
    uint8_t *vPlane = uPlane + static_cast<size_t>(chromaStride) * chromaSliceHeight;
    for (int32_t y = 0; y < image.height; y += K_CHROMA_SAMPLE_STEP) {
        uint8_t *uRow = uPlane + static_cast<size_t>(y / K_CHROMA_SAMPLE_STEP) * chromaStride;
        uint8_t *vRow = vPlane + static_cast<size_t>(y / K_CHROMA_SAMPLE_STEP) * chromaStride;
        for (int32_t x = 0; x < image.width; x += K_CHROMA_SAMPLE_STEP) {
            YuvColor yuv = RgbToYuv(GetSpectrumColor(x, y, image.width, pattern));
            int32_t chromaX = x / K_CHROMA_SAMPLE_STEP;
            uRow[chromaX] = yuv.u;
            vRow[chromaX] = yuv.v;
        }
    }
    return true;
}

bool FillRgba8888(const ImageBuffer &image, uint8_t pattern, bool redFirst)
{
    if (image.base == nullptr || image.width <= 0 || image.height <= 0 ||
        image.stride < image.width * K_RGBA_BYTES_PER_PIXEL) {
        return false;
    }

    for (int32_t y = 0; y < image.height; ++y) {
        uint8_t *row = image.base + static_cast<size_t>(y) * image.stride;
        for (int32_t x = 0; x < image.width; ++x) {
            const RgbColor &color = GetSpectrumColor(x, y, image.width, pattern);
            uint8_t *pixel = row + static_cast<size_t>(x) * K_RGBA_BYTES_PER_PIXEL;
            pixel[0] = redFirst ? color.r : color.b;
            pixel[1] = color.g;
            pixel[K_RGBA_BLUE_INDEX] = redFirst ? color.b : color.r;
            pixel[K_RGBA_ALPHA_INDEX] = K_FULL_ALPHA_8BIT;
        }
    }
    return true;
}

uint16_t ToP010Sample(uint8_t value)
{
    return static_cast<uint16_t>(value) << K_P010_SHIFT;
}

std::string GetFrameLabel(uint32_t frameNumber)
{
    return "F" + std::to_string(frameNumber);
}

std::array<uint8_t, K_FRAME_LABEL_GLYPH_HEIGHT> GetGlyphRows(char value)
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

void DrawYuv420SpPixel(const ImageBuffer &image, int32_t x, int32_t y, bool foreground, bool cbFirst)
{
    if (image.base == nullptr || x < 0 || y < 0 || x >= image.width || y >= image.height || image.stride <= 0 ||
        image.sliceHeight <= 0) {
        return;
    }
    image.base[static_cast<size_t>(y) * image.stride + x] = foreground ? K_TEXT_WHITE_Y : K_TEXT_BLACK_Y;

    uint8_t *uvPlane = image.base + static_cast<size_t>(image.stride) * image.sliceHeight;
    uint8_t *uvRow = uvPlane + static_cast<size_t>(y / K_CHROMA_SAMPLE_STEP) * image.stride;
    int32_t uvX = (x / K_CHROMA_SAMPLE_STEP) * K_CHROMA_SAMPLE_STEP;
    if (uvX + 1 >= image.stride) {
        return;
    }
    uvRow[uvX] = K_TEXT_NEUTRAL_UV;
    uvRow[uvX + 1] = K_TEXT_NEUTRAL_UV;
    (void)cbFirst;
}

void DrawYuv420PPixel(const ImageBuffer &image, int32_t x, int32_t y, bool foreground)
{
    if (image.base == nullptr || x < 0 || y < 0 || x >= image.width || y >= image.height || image.stride <= 0 ||
        image.sliceHeight <= 0) {
        return;
    }
    image.base[static_cast<size_t>(y) * image.stride + x] = foreground ? K_TEXT_WHITE_Y : K_TEXT_BLACK_Y;

    int32_t chromaStride = image.stride / K_CHROMA_SAMPLE_STEP;
    int32_t chromaSliceHeight = (image.sliceHeight + 1) / K_CHROMA_SAMPLE_STEP;
    int32_t chromaX = x / K_CHROMA_SAMPLE_STEP;
    int32_t chromaY = y / K_CHROMA_SAMPLE_STEP;
    if (chromaStride <= 0 || chromaX >= chromaStride || chromaY >= chromaSliceHeight) {
        return;
    }
    uint8_t *uPlane = image.base + static_cast<size_t>(image.stride) * image.sliceHeight;
    uint8_t *vPlane = uPlane + static_cast<size_t>(chromaStride) * chromaSliceHeight;
    uPlane[static_cast<size_t>(chromaY) * chromaStride + chromaX] = K_TEXT_NEUTRAL_UV;
    vPlane[static_cast<size_t>(chromaY) * chromaStride + chromaX] = K_TEXT_NEUTRAL_UV;
}

void DrawYuvP010Pixel(const ImageBuffer &image, int32_t x, int32_t y, bool foreground, bool cbFirst)
{
    int32_t samplesPerRow = image.stride / K_P010_BYTES_PER_SAMPLE;
    if (image.base == nullptr || x < 0 || y < 0 || x >= image.width || y >= image.height ||
        x >= samplesPerRow || image.stride <= 0 || image.sliceHeight <= 0) {
        return;
    }
    uint16_t *row = reinterpret_cast<uint16_t *>(image.base + static_cast<size_t>(y) * image.stride);
    row[x] = ToP010Sample(foreground ? K_TEXT_WHITE_Y : K_TEXT_BLACK_Y);

    uint8_t *uvPlane = image.base + static_cast<size_t>(image.stride) * image.sliceHeight;
    uint16_t *uvRow = reinterpret_cast<uint16_t *>(uvPlane +
        static_cast<size_t>(y / K_CHROMA_SAMPLE_STEP) * image.stride);
    int32_t uvX = (x / K_CHROMA_SAMPLE_STEP) * K_CHROMA_SAMPLE_STEP;
    if (uvX + 1 >= samplesPerRow) {
        return;
    }
    uvRow[uvX] = ToP010Sample(K_TEXT_NEUTRAL_UV);
    uvRow[uvX + 1] = ToP010Sample(K_TEXT_NEUTRAL_UV);
    (void)cbFirst;
}

void DrawRgba8888Pixel(const ImageBuffer &image, int32_t x, int32_t y, bool foreground)
{
    if (image.base == nullptr || x < 0 || y < 0 || x >= image.width || y >= image.height ||
        image.stride < image.width * K_RGBA_BYTES_PER_PIXEL) {
        return;
    }
    uint8_t value = foreground ? K_FULL_ALPHA_8BIT : 0x00;
    uint8_t *pixel = image.base + static_cast<size_t>(y) * image.stride +
        static_cast<size_t>(x) * K_RGBA_BYTES_PER_PIXEL;
    pixel[0] = value;
    pixel[1] = value;
    pixel[K_RGBA_BLUE_INDEX] = value;
    pixel[K_RGBA_ALPHA_INDEX] = K_FULL_ALPHA_8BIT;
}

void DrawRgba1010102Pixel(const ImageBuffer &image, int32_t x, int32_t y, bool foreground)
{
    if (image.base == nullptr || x < 0 || y < 0 || x >= image.width || y >= image.height ||
        image.stride < image.width * K_RGBA_BYTES_PER_PIXEL) {
        return;
    }
    uint32_t value = foreground ? K_RGB10_TEXT_WHITE : K_RGB10_TEXT_BLACK;
    uint32_t *row = reinterpret_cast<uint32_t *>(image.base + static_cast<size_t>(y) * image.stride);
    row[x] = value | (value << K_RGB10_GREEN_SHIFT) | (value << K_RGB10_BLUE_SHIFT) |
        (K_RGB10_ALPHA << K_RGB10_ALPHA_SHIFT);
}

void DrawFrameLabelPixel(const PixelTarget &target, int32_t x, int32_t y, bool foreground)
{
    switch (target.format) {
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP:
            DrawYuv420SpPixel(target.image, x, y, foreground, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP:
            DrawYuv420SpPixel(target.image, x, y, foreground, false);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P:
            DrawYuv420PPixel(target.image, x, y, foreground);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_P010:
            DrawYuvP010Pixel(target.image, x, y, foreground, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_P010:
            DrawYuvP010Pixel(target.image, x, y, foreground, false);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
            DrawRgba8888Pixel(target.image, x, y, foreground);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_1010102:
            DrawRgba1010102Pixel(target.image, x, y, foreground);
            break;
        default:
            break;
    }
}

void DrawSolidRect(const PixelTarget &target, const RectArea &rect, bool foreground)
{
    for (int32_t y = rect.top; y < rect.top + rect.height; ++y) {
        for (int32_t x = rect.left; x < rect.left + rect.width; ++x) {
            DrawFrameLabelPixel(target, x, y, foreground);
        }
    }
}

void DrawFrameGlyph(const PixelTarget &target, const std::array<uint8_t, K_FRAME_LABEL_GLYPH_HEIGHT> &rows,
                    int32_t cursorX, int32_t cursorY)
{
    for (int32_t rowIndex = 0; rowIndex < K_FRAME_LABEL_GLYPH_HEIGHT; ++rowIndex) {
        for (int32_t colIndex = 0; colIndex < K_FRAME_LABEL_GLYPH_WIDTH; ++colIndex) {
            bool isSet = (rows[rowIndex] & (1 << (K_FRAME_LABEL_GLYPH_WIDTH - 1 - colIndex))) != 0;
            if (!isSet) {
                continue;
            }
            int32_t dotX = cursorX + colIndex * K_FRAME_LABEL_SCALE;
            int32_t dotY = cursorY + rowIndex * K_FRAME_LABEL_SCALE;
            DrawSolidRect(target, RectArea {dotX, dotY, K_FRAME_LABEL_SCALE, K_FRAME_LABEL_SCALE}, true);
        }
    }
}

void DrawFrameNumber(const PixelTarget &target, uint32_t frameNumber)
{
    std::string label = GetFrameLabel(frameNumber);
    int32_t glyphAdvance = (K_FRAME_LABEL_GLYPH_WIDTH + K_FRAME_LABEL_GLYPH_SPACING) * K_FRAME_LABEL_SCALE;
    int32_t backgroundWidth = static_cast<int32_t>(label.size()) * glyphAdvance +
        K_FRAME_LABEL_PADDING * K_CHROMA_SAMPLE_STEP;
    int32_t backgroundHeight = K_FRAME_LABEL_GLYPH_HEIGHT * K_FRAME_LABEL_SCALE +
        K_FRAME_LABEL_PADDING * K_CHROMA_SAMPLE_STEP;
    DrawSolidRect(target, RectArea {K_FRAME_LABEL_X, K_FRAME_LABEL_Y, backgroundWidth, backgroundHeight}, false);

    int32_t cursorX = K_FRAME_LABEL_X + K_FRAME_LABEL_PADDING;
    int32_t cursorY = K_FRAME_LABEL_Y + K_FRAME_LABEL_PADDING;
    for (char value : label) {
        DrawFrameGlyph(target, GetGlyphRows(value), cursorX, cursorY);
        cursorX += glyphAdvance;
    }
}

bool FillYuvP010(const ImageBuffer &image, uint8_t pattern, bool cbFirst)
{
    int32_t samplesPerRow = image.stride / K_P010_BYTES_PER_SAMPLE;
    if (image.base == nullptr || image.width <= 0 || image.height <= 0 ||
        image.stride < image.width * K_P010_BYTES_PER_SAMPLE || image.sliceHeight < image.height) {
        return false;
    }

    for (int32_t y = 0; y < image.height; ++y) {
        uint16_t *row = reinterpret_cast<uint16_t *>(image.base + static_cast<size_t>(y) * image.stride);
        for (int32_t x = 0; x < image.width; ++x) {
            row[x] = ToP010Sample(RgbToYuv(GetSpectrumColor(x, y, image.width, pattern)).y);
        }
    }

    uint8_t *uvPlane = image.base + static_cast<size_t>(image.stride) * image.sliceHeight;
    for (int32_t y = 0; y < image.height; y += K_CHROMA_SAMPLE_STEP) {
        uint16_t *row = reinterpret_cast<uint16_t *>(uvPlane +
            static_cast<size_t>(y / K_CHROMA_SAMPLE_STEP) * image.stride);
        for (int32_t x = 0; x < image.width; x += K_CHROMA_SAMPLE_STEP) {
            YuvColor yuv = RgbToYuv(GetSpectrumColor(x, y, image.width, pattern));
            row[x] = ToP010Sample(cbFirst ? yuv.u : yuv.v);
            if (x + 1 < samplesPerRow) {
                row[x + 1] = ToP010Sample(cbFirst ? yuv.v : yuv.u);
            }
        }
    }
    return true;
}

bool FillRgba1010102(const ImageBuffer &image, uint8_t pattern)
{
    if (image.base == nullptr || image.width <= 0 || image.height <= 0 ||
        image.stride < image.width * K_RGBA_BYTES_PER_PIXEL) {
        return false;
    }

    for (int32_t y = 0; y < image.height; ++y) {
        uint32_t *row = reinterpret_cast<uint32_t *>(image.base + static_cast<size_t>(y) * image.stride);
        for (int32_t x = 0; x < image.width; ++x) {
            const RgbColor &color = GetSpectrumColor(x, y, image.width, pattern);
            uint32_t r = static_cast<uint32_t>(color.r) << K_RGB10_SHIFT;
            uint32_t g = static_cast<uint32_t>(color.g) << K_RGB10_SHIFT;
            uint32_t b = static_cast<uint32_t>(color.b) << K_RGB10_SHIFT;
            row[x] = r | (g << K_RGB10_GREEN_SHIFT) | (b << K_RGB10_BLUE_SHIFT) |
                (K_RGB10_ALPHA << K_RGB10_ALPHA_SHIFT);
        }
    }
    return true;
}

bool FillSpectrumBlocks(const PixelTarget &target, uint8_t pattern, uint32_t frameNumber)
{
    bool filled = false;
    switch (target.format) {
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP:
            filled = FillYuv420Sp(target.image, pattern, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP:
            filled = FillYuv420Sp(target.image, pattern, false);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P:
            filled = FillYuv420P(target.image, pattern);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
            filled = FillRgba8888(target.image, pattern, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_RGBA_1010102:
            filled = FillRgba1010102(target.image, pattern);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_P010:
            filled = FillYuvP010(target.image, pattern, true);
            break;
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_P010:
            filled = FillYuvP010(target.image, pattern, false);
            break;
        default:
            return false;
    }
    if (!filled) {
        return false;
    }
    DrawFrameNumber(target, frameNumber);
    return true;
}

void PrintRequestFailure(const FrameRequestInfo &info, const char *step, int32_t err,
                         const RequestFailureDetail &detail = {})
{
    cout << "RequestAndFlushFrame failed, step=" << step << ", ret=" << err << ", width=" << info.width
         << ", height=" << info.height << ", pattern=" << static_cast<int32_t>(info.pattern)
         << ", frameNumber=" << info.frameNumber << ", timestampUs=" << info.timestampUs
         << ", format=" << detail.format << ", stride=" << detail.stride
         << ", sliceHeight=" << detail.sliceHeight << endl;
}

void PrintRequestWarning(const FrameRequestInfo &info, const char *step, int32_t err, int32_t format = 0)
{
    cout << "RequestAndFlushFrame warning, step=" << step << ", ret=" << err;
    if (format != 0) {
        cout << ", fallbackFormat=" << format;
    }
    cout << ", width=" << info.width << ", height=" << info.height
         << ", pattern=" << static_cast<int32_t>(info.pattern) << ", frameNumber=" << info.frameNumber
         << ", timestampUs=" << info.timestampUs << endl;
}

bool RequestWindowBuffer(OHNativeWindow *window, const FrameRequestInfo &info, WindowBufferContext &context)
{
    int fenceFd = -1;
    int32_t ret = OH_NativeWindow_NativeWindowRequestBuffer(window, &context.windowBuffer, &fenceFd);
    if (ret != 0 || context.windowBuffer == nullptr) {
        PrintRequestFailure(info, "request_buffer", ret);
        return false;
    }
    if (fenceFd > 0) {
        close(fenceFd);
    }

    context.nativeBuffer = OH_NativeBufferFromNativeWindowBuffer(context.windowBuffer);
    if (context.nativeBuffer == nullptr) {
        PrintRequestFailure(info, "native_buffer_from_window_buffer", AV_ERR_INVALID_VAL);
        (void)NativeWindowCancelBuffer(window, context.windowBuffer);
        return false;
    }
    return true;
}

bool MapAndDescribeBuffer(OHNativeWindow *window, const FrameRequestInfo &info, WindowBufferContext &context)
{
    int32_t ret = OH_NativeBuffer_Map(context.nativeBuffer, &context.virAddr);
    if (ret != 0 || context.virAddr == nullptr) {
        PrintRequestFailure(info, "map_buffer", ret);
        (void)NativeWindowCancelBuffer(window, context.windowBuffer);
        return false;
    }

    const SurfaceBuffer *surfaceBuffer = SurfaceBuffer::NativeBufferToSurfaceBuffer(context.nativeBuffer);
    if (surfaceBuffer == nullptr) {
        PrintRequestFailure(info, "surface_buffer_from_native_buffer", AV_ERR_INVALID_VAL);
        (void)OH_NativeBuffer_Unmap(context.nativeBuffer);
        (void)NativeWindowCancelBuffer(window, context.windowBuffer);
        return false;
    }

    context.format = surfaceBuffer->GetFormat();
    ret = OH_NativeWindow_NativeWindowHandleOpt(window, GET_FORMAT, &context.format);
    if (ret != 0) {
        PrintRequestWarning(info, "get_format", ret, context.format);
    }

    OH_NativeBuffer_Config config;
    OH_NativeBuffer_GetConfig(context.nativeBuffer, &config);
    context.stride = config.stride;
    context.sliceHeight = surfaceBuffer->GetHeight();
    if (context.stride > 0 && context.sliceHeight > 0) {
        return true;
    }
    PrintRequestFailure(info, "invalid_buffer_config", AV_ERR_INVALID_VAL,
                        {context.format, context.stride, context.sliceHeight});
    (void)OH_NativeBuffer_Unmap(context.nativeBuffer);
    (void)NativeWindowCancelBuffer(window, context.windowBuffer);
    return false;
}

bool FillAndUnmapBuffer(OHNativeWindow *window, const FrameRequestInfo &info, WindowBufferContext &context)
{
    ImageBuffer image {static_cast<uint8_t *>(context.virAddr), info.width, info.height, context.stride,
                       context.sliceHeight};
    if (!FillSpectrumBlocks(PixelTarget {image, context.format}, info.pattern, info.frameNumber)) {
        PrintRequestFailure(info, "fill_spectrum_blocks", AV_ERR_INVALID_VAL,
                            {context.format, context.stride, context.sliceHeight});
        (void)OH_NativeBuffer_Unmap(context.nativeBuffer);
        (void)NativeWindowCancelBuffer(window, context.windowBuffer);
        return false;
    }

    int32_t ret = OH_NativeBuffer_Unmap(context.nativeBuffer);
    if (ret == 0) {
        return true;
    }
    PrintRequestFailure(info, "unmap_buffer", ret, {context.format, context.stride, context.sliceHeight});
    (void)NativeWindowCancelBuffer(window, context.windowBuffer);
    return false;
}

bool FlushWindowBuffer(OHNativeWindow *window, const FrameRequestInfo &info, const WindowBufferContext &context)
{
    Region region {};
    Region::Rect rect {};
    rect.x = 0;
    rect.y = 0;
    rect.w = info.width;
    rect.h = info.height;
    region.rects = &rect;
    region.rectNumber = 1;
    int32_t ret = OH_NativeWindow_NativeWindowFlushBuffer(window, context.windowBuffer, -1, region);
    if (ret != 0) {
        PrintRequestFailure(info, "flush_buffer", ret, {context.format, context.stride, context.sliceHeight});
    }
    return ret == 0;
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
    (void)userData;

    int32_t width = 0;
    int32_t height = 0;
    int32_t cropLeft = 0;
    int32_t cropTop = 0;
    int32_t cropRight = 0;
    int32_t cropBottom = 0;
    int32_t downsamplingWidth = 0;
    int32_t downsamplingHeight = 0;
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_WIDTH, &width);
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_HEIGHT, &height);
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, &cropLeft);
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, &cropTop);
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, &cropRight);
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, &cropBottom);
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, &downsamplingWidth);
    (void)OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, &downsamplingHeight);

    bool cropping = cropLeft != 0 || cropTop != 0 || cropRight != 0 || cropBottom != 0;
    if (cropping) {
        int32_t cropWidth = cropRight - cropLeft + 1;
        int32_t cropHeight = cropBottom - cropTop + 1;
        if (cropWidth >= width || cropHeight >= height) {
            std::cout << "[OnStreamChanged] Turn off crop" << std::endl;
            auto parameter = OH_AVFormat_Create();
            (void)OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, 0);
            (void)OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_TOP, 0);
            (void)OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_RIGHT, 0);
            (void)OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_BOTTOM, 0);
            auto ret = OH_VideoEncoder_SetParameter(codec, parameter);
            if (ret != 0) {
                std::cout << "[OnStreamChanged] SetParameter failed. ret=" << ret << std::endl;
            }
            OH_AVFormat_Destroy(parameter);
            parameter = nullptr;
        }
    }

    if (downsamplingWidth != 0 || downsamplingHeight != 0) {
        std::cout << "[OnStreamChanged] Turn off downsampling" << std::endl;
        auto parameter = OH_AVFormat_Create();
        (void)OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, 0);
        (void)OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_HEIGHT, 0);
        auto ret = OH_VideoEncoder_SetParameter(codec, parameter);
        if (ret != 0) {
            std::cout << "[OnStreamChanged] SetParameter failed. ret=" << ret << std::endl;
        }
        OH_AVFormat_Destroy(parameter);
        parameter = nullptr;
    }
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
    primaryOutputFile_.open(path, std::ios::out | std::ios::binary | std::ios::trunc);
    secondaryOutputPath_ = MakeSecondaryOutputPath(path);
    return primaryOutputFile_.is_open();
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
    if (scenario.profile >= 0 && !OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, scenario.profile)) {
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
    ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_USAGE, K_SURFACE_USAGE);
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
    if (secondaryOutputFile_.is_open()) {
        secondaryOutputFile_.close();
    }
    if (!secondaryOutputPath_.empty()) {
        if (!EnsureOutputDir(secondaryOutputPath_)) {
            return AV_ERR_UNKNOWN;
        }
        secondaryOutputFile_.open(secondaryOutputPath_, std::ios::out | std::ios::binary | std::ios::trunc);
        if (!secondaryOutputFile_.is_open()) {
            return AV_ERR_UNKNOWN;
        }
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
    FrameRequestInfo info {width, height, pattern, timestampUs, frameNumber};
    if (window_ == nullptr) {
        PrintRequestFailure(info, "window_null", AV_ERR_INVALID_VAL);
        return false;
    }

    int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(window_, SET_UI_TIMESTAMP, timestampUs);
    if (ret != 0) {
        PrintRequestWarning(info, "set_ui_timestamp", ret);
    }

    WindowBufferContext context;
    if (!RequestWindowBuffer(window_, info, context)) {
        return false;
    }
    if (!MapAndDescribeBuffer(window_, info, context)) {
        return false;
    }
    if (!FillAndUnmapBuffer(window_, info, context)) {
        return false;
    }
    return FlushWindowBuffer(window_, info, context);
}

int32_t VideoEncPreprocDualOutputSample::ApplyRuntimeOperation(const PreprocDualOutput::ScenarioCase &scenario,
                                                               uint32_t frameIndex, uint32_t frameCount,
                                                               int32_t &frameWidth, int32_t &frameHeight)
{
    if (frameIndex != frameCount / K_RUNTIME_TRIGGER_DIVISOR) {
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
            i >= frameCount / K_RUNTIME_TRIGGER_DIVISOR) {
            geometryUpdated = true;
        }
        int64_t timestampUs = GetFrameTimestampUs(i, scenario.primary.frameRate);
        if (!RequestAndFlushFrame(frameWidth, frameHeight, static_cast<uint8_t>(K_FRAME_PATTERN_BASE + i), timestampUs,
                                  i + 1)) {
            return AV_ERR_UNKNOWN;
        }
        usleep(K_FRAME_INTERVAL_SLEEP_US);
    }
#ifndef SUPPORT_VIDEO_ENCODER_PREPROCESSING
    sleep(1);
#endif
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

int32_t VideoEncPreprocDualOutputSample::SendFramesWithConfiguredPts(const PreprocDualOutput::ScenarioCase &scenario,
                                                                     uint32_t frameCount)
{
    if (window_ == nullptr) {
        return AV_ERR_INVALID_VAL;
    }
    for (uint32_t i = 0; i < frameCount; ++i) {
        int64_t timestampUs = GetFrameTimestampUs(i, scenario.primary.frameRate);
        if (!RequestAndFlushFrame(scenario.width, scenario.height, static_cast<uint8_t>(K_FRAME_PATTERN_BASE + i),
                                  timestampUs, i + 1)) {
            return AV_ERR_UNKNOWN;
        }
        usleep(K_FRAME_INTERVAL_SLEEP_US);
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
