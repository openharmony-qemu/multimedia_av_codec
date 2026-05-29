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

#include <array>
#include <cstring>
#include <gtest/gtest.h>
#include <vector>
#include "avcodec_errors.h"
#include "cpu_image_processor.h"
#include "surface.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {
using namespace testing::ext;

namespace {
constexpr uint8_t SENTINEL = 0xCC;
constexpr int32_t SRC_WIDTH = 8;
constexpr int32_t SRC_HEIGHT = 6;
#ifdef SUPPORT_VIDEO_ENCODER_PREPROCESSING
constexpr int32_t CROP_LEFT = 2;
constexpr int32_t CROP_TOP = 2;
constexpr int32_t CROP_WIDTH = 4;
constexpr int32_t CROP_HEIGHT = 2;
constexpr int32_t DS_WIDTH = 4;
constexpr int32_t DS_HEIGHT = 4;
#endif
constexpr int32_t BUFFER_STRIDE_ALIGNMENT = 64;
constexpr int32_t BUFFER_REQUEST_TIMEOUT = 0;
constexpr int32_t DEFAULT_PLANE_BYTES_PER_PIXEL = 1;
constexpr int32_t NO_SUBSAMPLE = 1;
constexpr int32_t YUV_SUBSAMPLE = 2;
constexpr int32_t P010_LUMA_BYTES_PER_PIXEL = 2;
constexpr int32_t P010_CHROMA_BYTES_PER_PIXEL = 4;
constexpr int32_t RGBA_BYTES_PER_PIXEL = 4;
constexpr size_t PATTERN_PLANE_FACTOR = 53;
constexpr int32_t PATTERN_ROW_FACTOR = 17;
constexpr uint8_t BYTE_MASK = 0xFF;
constexpr int32_t NON_YUV_ODD_SRC_WIDTH = 5;
constexpr int32_t NON_YUV_ODD_SRC_HEIGHT = 3;
constexpr int32_t ODD_OUTPUT_WIDTH = 3;
constexpr int32_t ODD_OUTPUT_HEIGHT = 3;
constexpr int32_t ODD_CROP_LEFT = 1;
constexpr int32_t ODD_CROP_TOP = 1;
constexpr int32_t ODD_CROP_WIDTH = 3;
constexpr int32_t ODD_CROP_HEIGHT = 3;

struct FormatParam {
    const char *name;
    int32_t format;
    bool yuv420;
};

#ifdef SUPPORT_VIDEO_ENCODER_PREPROCESSING
const FormatParam FORMAT_PARAMS[] = {
    {"NV12", GRAPHIC_PIXEL_FMT_YCBCR_420_SP, true},
    {"NV21", GRAPHIC_PIXEL_FMT_YCRCB_420_SP, true},
    {"YCBCR_P010", GRAPHIC_PIXEL_FMT_YCBCR_P010, true},
    {"YCRCB_P010", GRAPHIC_PIXEL_FMT_YCRCB_P010, true},
    {"YCBCR_420_P", GRAPHIC_PIXEL_FMT_YCBCR_420_P, true},
    {"RGBA8888", GRAPHIC_PIXEL_FMT_RGBA_8888, false},
    {"RGBA1010102", GRAPHIC_PIXEL_FMT_RGBA_1010102, false},
};
#else
const FormatParam FORMAT_PARAMS[] = {
    {"NV12", GRAPHIC_PIXEL_FMT_YCBCR_420_SP, true},
    {"NV21", GRAPHIC_PIXEL_FMT_YCRCB_420_SP, true},
    {"YCBCR_420_P", GRAPHIC_PIXEL_FMT_YCBCR_420_P, true},
};
#endif

struct Plane {
    size_t offset;
    int32_t width;
    int32_t height;
    int32_t rowBytes;
    int32_t stride;
    int32_t bytesPerPixel;
    int32_t xSubsample;
    int32_t ySubsample;
};

sptr<SurfaceBuffer> AllocBuffer(int32_t width, int32_t height, int32_t format)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    if (buffer == nullptr) {
        return nullptr;
    }
    BufferRequestConfig config = {
        .width = width,
        .height = height,
        .strideAlignment = BUFFER_STRIDE_ALIGNMENT,
        .format = format,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = BUFFER_REQUEST_TIMEOUT,
    };
    if (buffer->Alloc(config) != GSERROR_OK) {
        return nullptr;
    }
    return buffer;
}

bool BuildPlanes(const sptr<SurfaceBuffer> &buffer, std::vector<Plane> &planes)
{
    planes.clear();
    if (!buffer || buffer->GetStride() <= 0) {
        return false;
    }
    int32_t width = buffer->GetWidth();
    int32_t height = buffer->GetHeight();
    int32_t stride = buffer->GetStride();
    size_t offset = 0;
    auto add = [&](int32_t w, int32_t h, int32_t rowBytes, int32_t rowStride, int32_t bpp,
                   int32_t xSubsample, int32_t ySubsample) {
        if (rowStride < rowBytes) {
            return false;
        }
        planes.push_back({offset, w, h, rowBytes, rowStride, bpp, xSubsample, ySubsample});
        offset += static_cast<size_t>(h) * static_cast<size_t>(rowStride);
        return true;
    };

    switch (buffer->GetFormat()) {
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
            return add(width, height, width, stride, DEFAULT_PLANE_BYTES_PER_PIXEL, NO_SUBSAMPLE, NO_SUBSAMPLE) &&
                   add(width / YUV_SUBSAMPLE, height / YUV_SUBSAMPLE, width, stride, YUV_SUBSAMPLE, YUV_SUBSAMPLE,
                       YUV_SUBSAMPLE);
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
            return add(width, height, width * P010_LUMA_BYTES_PER_PIXEL, stride, P010_LUMA_BYTES_PER_PIXEL,
                       NO_SUBSAMPLE, NO_SUBSAMPLE) &&
                   add(width / YUV_SUBSAMPLE, height / YUV_SUBSAMPLE, width * P010_LUMA_BYTES_PER_PIXEL, stride,
                       P010_CHROMA_BYTES_PER_PIXEL, YUV_SUBSAMPLE, YUV_SUBSAMPLE);
        case GRAPHIC_PIXEL_FMT_YCBCR_420_P:
            return add(width, height, width, stride, DEFAULT_PLANE_BYTES_PER_PIXEL, NO_SUBSAMPLE, NO_SUBSAMPLE) &&
                   add(width / YUV_SUBSAMPLE, height / YUV_SUBSAMPLE, width / YUV_SUBSAMPLE,
                       stride / YUV_SUBSAMPLE, DEFAULT_PLANE_BYTES_PER_PIXEL, YUV_SUBSAMPLE, YUV_SUBSAMPLE) &&
                   add(width / YUV_SUBSAMPLE, height / YUV_SUBSAMPLE, width / YUV_SUBSAMPLE,
                       stride / YUV_SUBSAMPLE, DEFAULT_PLANE_BYTES_PER_PIXEL, YUV_SUBSAMPLE, YUV_SUBSAMPLE);
        case GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
            return add(width, height, width * RGBA_BYTES_PER_PIXEL, stride, RGBA_BYTES_PER_PIXEL, NO_SUBSAMPLE,
                       NO_SUBSAMPLE);
        default:
            return false;
    }
}

void FillVisiblePattern(const sptr<SurfaceBuffer> &buffer)
{
    auto *base = static_cast<uint8_t *>(buffer->GetVirAddr());
    ASSERT_NE(base, nullptr);
    std::memset(base, SENTINEL, static_cast<size_t>(buffer->GetSize()));
    std::vector<Plane> planes;
    ASSERT_TRUE(BuildPlanes(buffer, planes));
    for (size_t planeIndex = 0; planeIndex < planes.size(); ++planeIndex) {
        const Plane &plane = planes[planeIndex];
        for (int32_t y = 0; y < plane.height; ++y) {
            for (int32_t x = 0; x < plane.rowBytes; ++x) {
                base[plane.offset + static_cast<size_t>(y) * plane.stride + x] =
                    static_cast<uint8_t>((planeIndex * PATTERN_PLANE_FACTOR + y * PATTERN_ROW_FACTOR + x) &
                                         BYTE_MASK);
            }
        }
    }
}

std::vector<uint8_t> ReadVisibleBytes(const sptr<SurfaceBuffer> &buffer)
{
    auto *base = static_cast<uint8_t *>(buffer->GetVirAddr());
    std::vector<Plane> planes;
    EXPECT_TRUE(BuildPlanes(buffer, planes));
    std::vector<uint8_t> data;
    for (const auto &plane : planes) {
        for (int32_t y = 0; y < plane.height; ++y) {
            const uint8_t *row = base + plane.offset + static_cast<size_t>(y) * plane.stride;
            data.insert(data.end(), row, row + plane.rowBytes);
        }
    }
    return data;
}

#ifdef SUPPORT_VIDEO_ENCODER_PREPROCESSING
std::vector<uint8_t> ExpectedCrop(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output,
                                  int32_t left, int32_t top)
{
    auto *base = static_cast<uint8_t *>(input->GetVirAddr());
    std::vector<Plane> srcPlanes;
    std::vector<Plane> dstPlanes;
    EXPECT_TRUE(BuildPlanes(input, srcPlanes));
    EXPECT_TRUE(BuildPlanes(output, dstPlanes));
    std::vector<uint8_t> data;
    for (size_t i = 0; i < srcPlanes.size(); ++i) {
        const Plane &src = srcPlanes[i];
        const Plane &dst = dstPlanes[i];
        int32_t srcLeft = left / src.xSubsample;
        int32_t srcTop = top / src.ySubsample;
        int32_t bytes = dst.width * dst.bytesPerPixel;
        for (int32_t y = 0; y < dst.height; ++y) {
            const uint8_t *row = base + src.offset + static_cast<size_t>(srcTop + y) * src.stride +
                                 static_cast<size_t>(srcLeft) * src.bytesPerPixel;
            data.insert(data.end(), row, row + bytes);
        }
    }
    return data;
}

std::vector<uint8_t> ExpectedDownsample(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output)
{
    auto *base = static_cast<uint8_t *>(input->GetVirAddr());
    std::vector<Plane> srcPlanes;
    std::vector<Plane> dstPlanes;
    EXPECT_TRUE(BuildPlanes(input, srcPlanes));
    EXPECT_TRUE(BuildPlanes(output, dstPlanes));
    std::vector<uint8_t> data;
    for (size_t i = 0; i < srcPlanes.size(); ++i) {
        const Plane &src = srcPlanes[i];
        const Plane &dst = dstPlanes[i];
        int32_t srcW = input->GetWidth() / src.xSubsample;
        int32_t srcH = input->GetHeight() / src.ySubsample;
        int32_t dstW = output->GetWidth() / dst.xSubsample;
        int32_t dstH = output->GetHeight() / dst.ySubsample;
        for (int32_t y = 0; y < dstH; ++y) {
            int32_t srcY = static_cast<int32_t>((static_cast<int64_t>(y) * srcH) / dstH);
            for (int32_t x = 0; x < dstW; ++x) {
                int32_t srcX = static_cast<int32_t>((static_cast<int64_t>(x) * srcW) / dstW);
                const uint8_t *pixel = base + src.offset + static_cast<size_t>(srcY) * src.stride +
                                       static_cast<size_t>(srcX) * src.bytesPerPixel;
                data.insert(data.end(), pixel, pixel + dst.bytesPerPixel);
            }
        }
    }
    return data;
}
#endif

void ExpectPaddingUntouched(const sptr<SurfaceBuffer> &buffer)
{
    auto *base = static_cast<uint8_t *>(buffer->GetVirAddr());
    std::vector<Plane> planes;
    ASSERT_TRUE(BuildPlanes(buffer, planes));
    for (const auto &plane : planes) {
        for (int32_t y = 0; y < plane.height; ++y) {
            for (int32_t x = plane.rowBytes; x < plane.stride; ++x) {
                EXPECT_EQ(base[plane.offset + static_cast<size_t>(y) * plane.stride + x], SENTINEL);
            }
        }
    }
}
} // namespace

class CpuImageProcessorTest : public testing::TestWithParam<FormatParam> {};

#ifdef SUPPORT_VIDEO_ENCODER_PREPROCESSING
HWTEST_P(CpuImageProcessorTest, CropSuccess, TestSize.Level1)
{
    const auto param = GetParam();
    auto input = AllocBuffer(SRC_WIDTH, SRC_HEIGHT, param.format);
    auto output = AllocBuffer(CROP_WIDTH, CROP_HEIGHT, param.format);
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    FillVisiblePattern(input);
    FillVisiblePattern(output);

    CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = CpuImageOperation::CROP;
    request.crop = {CROP_LEFT, CROP_TOP, CROP_WIDTH, CROP_HEIGHT};

    EXPECT_EQ(CpuImageProcessor::Process(request), AVCS_ERR_OK);
    EXPECT_EQ(ReadVisibleBytes(output), ExpectedCrop(input, output, CROP_LEFT, CROP_TOP));
    ExpectPaddingUntouched(output);
}

HWTEST_P(CpuImageProcessorTest, DownsampleSuccess, TestSize.Level1)
{
    const auto param = GetParam();
    auto input = AllocBuffer(SRC_WIDTH, SRC_HEIGHT, param.format);
    auto output = AllocBuffer(DS_WIDTH, DS_HEIGHT, param.format);
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    FillVisiblePattern(input);
    FillVisiblePattern(output);

    CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = CpuImageOperation::DOWNSAMPLE;

    EXPECT_EQ(CpuImageProcessor::Process(request), AVCS_ERR_OK);
    EXPECT_EQ(ReadVisibleBytes(output), ExpectedDownsample(input, output));
    ExpectPaddingUntouched(output);
}
#endif

HWTEST_P(CpuImageProcessorTest, CopySuccess, TestSize.Level1)
{
    const auto param = GetParam();
    auto input = AllocBuffer(SRC_WIDTH, SRC_HEIGHT, param.format);
    auto output = AllocBuffer(SRC_WIDTH, SRC_HEIGHT, param.format);
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    FillVisiblePattern(input);
    FillVisiblePattern(output);

    CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = CpuImageOperation::COPY;

    EXPECT_EQ(CpuImageProcessor::Process(request), AVCS_ERR_OK);
    EXPECT_EQ(ReadVisibleBytes(output), ReadVisibleBytes(input));
    ExpectPaddingUntouched(output);
}

HWTEST_P(CpuImageProcessorTest, FormatMismatchFails, TestSize.Level1)
{
    const auto param = GetParam();
    auto input = AllocBuffer(SRC_WIDTH, SRC_HEIGHT, param.format);
    auto output = AllocBuffer(SRC_WIDTH, SRC_HEIGHT, GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    FillVisiblePattern(input);
    FillVisiblePattern(output);

    CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = CpuImageOperation::COPY;

    if (param.format == GRAPHIC_PIXEL_FMT_RGBA_8888) {
        EXPECT_EQ(CpuImageProcessor::Process(request), AVCS_ERR_OK);
    } else {
        EXPECT_NE(CpuImageProcessor::Process(request), AVCS_ERR_OK);
    }
}

HWTEST_P(CpuImageProcessorTest, OddGeometryPolicy, TestSize.Level1)
{
    const auto param = GetParam();
    auto input = AllocBuffer(param.yuv420 ? SRC_WIDTH : NON_YUV_ODD_SRC_WIDTH,
                             param.yuv420 ? SRC_HEIGHT : NON_YUV_ODD_SRC_HEIGHT, param.format);
    auto output = AllocBuffer(ODD_OUTPUT_WIDTH, ODD_OUTPUT_HEIGHT, param.format);
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    FillVisiblePattern(input);
    FillVisiblePattern(output);

    CpuImageProcessRequest request;
    request.input = input;
    request.output = output;
    request.operation = CpuImageOperation::CROP;
    request.crop = {ODD_CROP_LEFT, ODD_CROP_TOP, ODD_CROP_WIDTH, ODD_CROP_HEIGHT};

    int32_t ret = CpuImageProcessor::Process(request);
    if (param.yuv420) {
        EXPECT_NE(ret, AVCS_ERR_OK);
    } else {
        EXPECT_EQ(ret, AVCS_ERR_OK);
    }
}

INSTANTIATE_TEST_SUITE_P(SupportedFormats, CpuImageProcessorTest, testing::ValuesIn(FORMAT_PARAMS),
                         [](const testing::TestParamInfo<FormatParam> &info) { return info.param.name; });

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
