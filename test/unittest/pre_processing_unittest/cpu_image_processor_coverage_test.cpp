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

#include <gtest/gtest.h>
#include "avcodec_errors.h"
#include "cpu_image_processor.h"
#include "surface.h"
#define TEST_SUIT CpuImageProcessorCoverageTest

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {
using namespace testing::ext;

namespace {
constexpr int32_t DEFAULT_WIDTH = 640;
constexpr int32_t DEFAULT_HEIGHT = 480;
constexpr int32_t DEFAULT_STRIDE = 640;
constexpr size_t BUFFER_SIZE = static_cast<size_t>(DEFAULT_STRIDE * DEFAULT_HEIGHT * 3 / 2);
constexpr int32_t BUFFER_STRIDE_ALIGNMENT = 64;
constexpr int32_t BUFFER_REQUEST_TIMEOUT = 0;

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
}

class TEST_SUIT : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(){};
    void TearDown(){};
};

/**
 * @tc.name AddPlane_InvalidWidth_Test_001
 * @tc.desc Test AddPlane validation when planeInfo.width <= 0
 *          This test triggers the width validation branch by creating a buffer
 *          with zero width, which causes BuildPlaneLayout to fail when calling AddPlane.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_InvalidWidth_Test_001, TestSize.Level1)
{
    constexpr int32_t width = 0;
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);

        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name AddPlane_NegativeWidth_Test_002
 * @tc.desc Test AddPlane validation when planeInfo.width is negative
 *          This test explicitly tests the width <= 0 validation branch with negative value.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_NegativeWidth_Test_002, TestSize.Level1)
{
    constexpr int32_t width = -10; // width: -10
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);

        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name AddPlane_InvalidHeight_Test_003
 * @tc.desc Test AddPlane validation when planeInfo.height <= 0
 *          This test triggers the height validation branch by creating a buffer
 *          with zero height.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_InvalidHeight_Test_003, TestSize.Level1)
{
    constexpr int32_t height = 0;
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);

        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name AddPlane_NegativeHeight_Test_004
 * @tc.desc Test AddPlane validation when planeInfo.height is negative
 *          This test explicitly tests the height <= 0 validation branch with negative value.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_NegativeHeight_Test_004, TestSize.Level1)
{
    constexpr int32_t height = -10; // height: -10
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);

        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name AddPlane_StrideLessThanRowBytes_Test_005
 * @tc.desc Test AddPlane validation when planeInfo.stride < planeInfo.rowBytes
 *          For NV12 format, rowBytes equals width. By creating a buffer where
 *          stride < width, we trigger the stride < rowBytes validation.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_StrideLessThanRowBytes_Test_005, TestSize.Level1)
{
    constexpr int32_t stride = 320; // stride: 320
    constexpr size_t bufferSize = static_cast<size_t>(stride * DEFAULT_HEIGHT * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);
    inputBuffer->GetBufferHandle()->stride = stride;

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);
    outputBuffer->GetBufferHandle()->stride = stride;

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;

    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name AddPlane_OffsetExceedsSize_Test_006
 * @tc.desc Test AddPlane validation when offset < view.size
 *          For NV12 format, the UV plane offset is at stride * sliceHeight.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_OffsetExceedsSize_Test_006, TestSize.Level1)
{
    constexpr int32_t sliceHeight = 480; // sliceHeight: 480
    constexpr size_t bufferSize = static_cast<size_t>(DEFAULT_STRIDE * sliceHeight);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_P);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_P);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name AddPlane_ValidPath_NV12_Test_007
 * @tc.desc Test AddPlane successful path with valid NV12 format buffer
 *          This validates that AddPlane returns true when all conditions are met,
 *          covering both luma and chroma planes for NV12 format.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_ValidPath_NV12_Test_007, TestSize.Level1)
{
    constexpr size_t bufferSize = static_cast<size_t>(DEFAULT_STRIDE * DEFAULT_HEIGHT * 3 / 2); // Full NV12 size

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name AddPlane_ValidPath_RGBA_Test_008
 * @tc.desc Test AddPlane successful path with valid RGBA format buffer
 *          This validates AddPlane with single plane layout for RGBA_8888 format.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_ValidPath_RGBA_Test_008, TestSize.Level1)
{
    constexpr int32_t stride = 2560; // stride: 2560
    constexpr size_t bufferSize = static_cast<size_t>(stride * DEFAULT_HEIGHT);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name AddPlane_ValidPath_I420_Test_009
 * @tc.desc Test AddPlane successful path with valid I420 format buffer
 *          This validates AddPlane being called three times successfully for I420 format
 *          (Y, U, V planes), ensuring all plane metadata is valid.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, AddPlane_ValidPath_I420_Test_009, TestSize.Level1)
{
    constexpr size_t bufferSize = static_cast<size_t>(DEFAULT_STRIDE * DEFAULT_HEIGHT * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_P);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_P);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;

    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name IsValidBuffer_NullBuffer_Test_011
 * @tc.desc Test IsValidBuffer validation when buffer is nullptr
 *          This test triggers the !buffer validation branch by passing null pointer
 *          as input buffer to Process function.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsValidBuffer_NullBuffer_Test_011, TestSize.Level1)
{
    constexpr size_t bufferSize = static_cast<size_t>(DEFAULT_STRIDE * DEFAULT_HEIGHT * 3 / 2);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* virAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    CpuImageProcessRequest request;
    request.input = nullptr;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;

    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name IsValidBuffer_NullOutputBuffer_Test_012
 * @tc.desc Test IsValidBuffer validation when output buffer is nullptr
 *          This test triggers the !buffer validation branch for output buffer.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsValidBuffer_NullOutputBuffer_Test_012, TestSize.Level1)
{
    constexpr size_t bufferSize = static_cast<size_t>(DEFAULT_STRIDE * DEFAULT_HEIGHT * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = nullptr;
    request.operation = CpuImageOperation::COPY;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name IsValidBuffer_ZeroWidth_Test_013
 * @tc.desc Test IsValidBuffer validation when buffer width <= 0
 *          This test triggers the GetWidth() <= 0 validation branch.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsValidBuffer_ZeroWidth_Test_013, TestSize.Level1)
{
    constexpr int32_t width = 0;
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);
        
        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name IsValidBuffer_NegativeWidth_Test_014
 * @tc.desc Test IsValidBuffer validation when buffer width is negative
 *          This test explicitly tests the GetWidth() <= 0 validation with negative value.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsValidBuffer_NegativeWidth_Test_014, TestSize.Level1)
{
    constexpr int32_t width = -10; // width: -10
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);

        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name IsValidBuffer_ZeroHeight_Test_015
 * @tc.desc Test IsValidBuffer validation when buffer height <= 0
 *          This test triggers the GetHeight() <= 0 validation branch.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsValidBuffer_ZeroHeight_Test_015, TestSize.Level1)
{
    constexpr int32_t height = 0;
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);

        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name IsValidBuffer_NegativeHeight_Test_016
 * @tc.desc Test IsValidBuffer validation when buffer height is negative
 *          This test explicitly tests the GetHeight() <= 0 validation with negative value.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsValidBuffer_NegativeHeight_Test_016, TestSize.Level1)
{
    constexpr int32_t height = -10; // height: -10
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (inputBuffer != nullptr) {
        sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
        ASSERT_NE(outputBuffer, nullptr);

        CpuImageProcessRequest request;
        request.input = inputBuffer;
        request.output = outputBuffer;
        request.operation = CpuImageOperation::COPY;
        int32_t ret = CpuImageProcessor::Process(request);
        EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    }
}

/**
 * @tc.name IsValidBuffer_ValidBuffer_Test_019
 * @tc.desc Test IsValidBuffer successful path with valid buffer parameters
 *          This validates that IsValidBuffer returns true when all conditions are met,
 *          allowing Process to continue execution successfully.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsValidBuffer_ValidBuffer_Test_019, TestSize.Level1)
{
    constexpr size_t bufferSize = static_cast<size_t>(DEFAULT_STRIDE * DEFAULT_HEIGHT * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name ValidateCropRect_NegativeLeft_Test_020
 * @tc.desc Test ValidateCropRect validation when crop.left < 0
 *          This test triggers the left boundary validation branch by setting
 *          negative left value in crop rectangle.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateCropRect_NegativeLeft_Test_020, TestSize.Level1)
{
    constexpr int32_t cropWidth = 320;  // cropWidth: 320
    constexpr int32_t cropHeight = 240; // cropHeight: 240
    constexpr size_t outputBufferSize = static_cast<size_t>(cropWidth * cropHeight * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(cropWidth, cropHeight, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, outputBufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::CROP;
    request.crop.left = -10; // Negative left triggers validation
    request.crop.top = 0;
    request.crop.width = cropWidth;
    request.crop.height = cropHeight;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateCropRect_NegativeTop_Test_021
 * @tc.desc Test ValidateCropRect validation when crop.top < 0
 *          This test triggers the top boundary validation branch by setting
 *          negative top value in crop rectangle.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateCropRect_NegativeTop_Test_021, TestSize.Level1)
{
    constexpr int32_t cropWidth = 320;  // cropWidth:320
    constexpr int32_t cropHeight = 240; // cropHeight:320
    constexpr size_t outputBufferSize = static_cast<size_t>(cropWidth * cropHeight * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(cropWidth, cropHeight, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, outputBufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::CROP;
    request.crop.left = 0;
    request.crop.top = -10; // Negative top triggers validation
    request.crop.width = cropWidth;
    request.crop.height = cropHeight;

    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateCropRect_ZeroWidth_Test_022
 * @tc.desc Test ValidateCropRect validation when crop.width == 0
 *          This test triggers the width boundary validation branch by setting
 *          width to 0 in crop rectangle.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateCropRect_ZeroWidth_Test_022, TestSize.Level1)
{
    constexpr int32_t cropWidth = 320;  // cropWidth: 320
    constexpr int32_t cropHeight = 240; // cropHeight: 240
    constexpr size_t outputBufferSize = static_cast<size_t>(cropWidth * cropHeight * 3 / 2);
    
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(cropWidth, cropHeight, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, outputBufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::CROP;
    request.crop.left = 0;
    request.crop.top = 0;
    request.crop.width = 0;
    request.crop.height = cropHeight;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateCropRect_ZeroHeight_Test_023
 * @tc.desc Test ValidateCropRect validation when crop.height == 0
 *          This test triggers the height boundary validation branch by setting
 *          height to 0 in crop rectangle.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateCropRect_ZeroHeight_Test_023, TestSize.Level1)
{
    constexpr int32_t cropWidth = 320;  // cropWidth: 320
    constexpr int32_t cropHeight = 240; // cropHeight: 240
    constexpr size_t outputBufferSize = static_cast<size_t>(cropWidth * cropHeight * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(cropWidth, cropHeight, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, outputBufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::CROP;
    request.crop.left = 0;
    request.crop.top = 0;
    request.crop.width = cropWidth;
    request.crop.height = 0;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateCropRect_OutsideWidth_Test_024
 * @tc.desc Test ValidateCropRect validation when crop.left + crop.width > input.width
 *          This test triggers the width boundary validation branch by setting
 *          left + width to be greater than input width in crop rectangle.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateCropRect_OutsideWidth_Test_024, TestSize.Level1)
{
    constexpr int32_t cropWidth = 320;  // cropWidth: 320
    constexpr int32_t cropHeight = 240; // cropHeight: 240
    constexpr size_t outputBufferSize = static_cast<size_t>(cropWidth * cropHeight * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(cropWidth, cropHeight, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, outputBufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::CROP;
    request.crop.left = 310;  // left: 310
    request.crop.top = 0;
    request.crop.width = 350; // width: 350
    request.crop.height = cropHeight;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateCropRect_OutsideHeight_Test_025
 * @tc.desc Test ValidateCropRect validation when crop.top + crop.height > input.height
 *          This test triggers the height boundary validation branch by setting
 *          top + height to be greater than input height in crop rectangle.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateCropRect_OutsideHeight_Test_025, TestSize.Level1)
{
    constexpr int32_t cropWidth = 320;  // cropWidth: 320
    constexpr int32_t cropHeight = 240; // cropHeight: 240
    constexpr size_t outputBufferSize = static_cast<size_t>(cropWidth * cropHeight * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(cropWidth, cropHeight, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, outputBufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::CROP;
    request.crop.left = 0;
    request.crop.top = 230;         // top: 230
    request.crop.width = cropWidth;
    request.crop.height = 250;      // height: 250
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateCropRect_ValidCropRect_Test_026
 * @tc.desc Test ValidateCropRect successful path with valid crop rectangle
 *          This validates that ValidateCropRect returns true when all conditions are met,
 *          allowing Process to continue execution successfully.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateCropRect_ValidCropRect_Test_026, TestSize.Level1)
{
    constexpr int32_t cropWidth = 320;  // cropWidth: 320
    constexpr int32_t cropHeight = 240; // cropHeight: 240
    constexpr size_t outputBufferSize = static_cast<size_t>(cropWidth * cropHeight * 3 / 2);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(cropWidth, cropHeight, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, outputBufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::CROP;
    request.crop.left = 0;
    request.crop.top = 0;
    request.crop.width = cropWidth;
    request.crop.height = cropHeight;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name ValidateOperation_NegativeOperation_Test_038
 * @tc.desc Test ValidateOperation validation when operation < 0
 *          This test triggers the operation boundary validation branch by setting
 *          negative operation value.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateOperation_NegativeOperation_Test_038, TestSize.Level1)
{
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, BUFFER_SIZE);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = static_cast<CpuImageOperation>(-1);
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateOperation_ValidOperation_Test_039
 * @tc.desc Test ValidateOperation successful path with valid operation
 *          This validates that ValidateOperation returns true when all conditions are met,
 *          allowing Process to continue execution successfully.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateOperation_ValidOperation_Test_039, TestSize.Level1)
{
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, BUFFER_SIZE);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name ValidateOperation_UnsupportedOperation_Test_040
 * @tc.desc Test ValidateOperation returning false for unsupported operations
 *          This test triggers the "default: return false" branch in ValidateOperation by using
 *          an operation that is not in the supported list
 *          Using an unsupported operation like CpuImageOperation::UNKNOWN will cause
 *          ValidateOperation to return false, leading to Process returning AVCS_ERR_INVALID_VAL
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateOperation_UnsupportedOperation_Test_040, TestSize.Level1)
{
    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, BUFFER_SIZE);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, BUFFER_SIZE);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = static_cast<CpuImageOperation>(-1);
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name IsFormatSupported_UnsupportedFormat_Test_047
 * @tc.desc Test IsFormatSupported returning false for unsupported pixel formats
 *          This test triggers the "default: return false" branch in IsFormatSupported by using
 *          a pixel format that is not in the supported list
 *          Using an unsupported format like GRAPHIC_PIXEL_FMT_BGRA_8888 will cause
 *          IsFormatSupported to return false, leading to Process returning AVCS_ERR_INVALID_VAL.
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, IsFormatSupported_UnsupportedFormat_Test_047, TestSize.Level1)
{
    constexpr int32_t stride = 640; // stride: 640
    constexpr size_t bufferSize = static_cast<size_t>(stride * DEFAULT_HEIGHT * 4);

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_BGRA_8888);
    ASSERT_NE(inputBuffer, nullptr);

    void* virAddr = inputBuffer->GetVirAddr();
    ASSERT_NE(virAddr, nullptr);
    memset(virAddr, 0, bufferSize);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_BGRA_8888);
    ASSERT_NE(outputBuffer, nullptr);

    void* outVirAddr = outputBuffer->GetVirAddr();
    ASSERT_NE(outVirAddr, nullptr);
    memset(outVirAddr, 0, bufferSize);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::COPY;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateDownsample_Invalid_Test_001
 * @tc.desc output.width > 0, output.height > 0, output.width > input.width, output.height > input.height
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateDownsample_Invalid_Test_001, TestSize.Level1)
{
    constexpr int32_t width = 652; // width: 652
    constexpr int32_t height = 492; // height: 492

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(width, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::DOWNSAMPLE;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateDownsample_Invalid_Test_002
 * @tc.desc output.width > 0, output.height > 0, output.width > input.width, output.height < input.height
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateDownsample_Invalid_Test_002, TestSize.Level1)
{
    constexpr int32_t width = 652; // width: 652
    constexpr int32_t height = 492; // height: 492

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::DOWNSAMPLE;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateDownsample_Invalid_Test_003
 * @tc.desc output.width > 0, output.height > 0, output.width < input.width, output.height > input.height
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateDownsample_Invalid_Test_003, TestSize.Level1)
{
    constexpr int32_t width = 652; // width: 652
    constexpr int32_t height = 492; // height: 492

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(width, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::DOWNSAMPLE;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name ValidateDownsample_Valid_Test_001
 * @tc.desc output.width > 0, output.height > 0, output.width < input.width, output.height < input.height
 *          input.isYuv420 is true,
 *          input.width is even, input.height is even, output.width is even, output.height is even
 * @tc.type FUNC
 */
HWTEST_F(TEST_SUIT, ValidateDownsample_Valid_Test_001, TestSize.Level1)
{
    constexpr int32_t width = 652; // width: 652
    constexpr int32_t height = 492; // height: 492

    sptr<SurfaceBuffer> inputBuffer = AllocBuffer(width, height, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(inputBuffer, nullptr);

    sptr<SurfaceBuffer> outputBuffer = AllocBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    ASSERT_NE(outputBuffer, nullptr);

    CpuImageProcessRequest request;
    request.input = inputBuffer;
    request.output = outputBuffer;
    request.operation = CpuImageOperation::DOWNSAMPLE;
    int32_t ret = CpuImageProcessor::Process(request);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}
} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
