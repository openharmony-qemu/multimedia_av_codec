/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include <cmath>
#include <gtest/gtest.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "buffer_converter.h"
#include "meta/meta_key.h"
#include "native_buffer.h"
#include "surface_buffer.h"
#include "surface_type.h"
#include "unittest_utils.h"

#include "unittest_log.h"

namespace {
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
constexpr int32_t DEFAULT_WIDTH = 2520;
constexpr int32_t DEFAULT_HEIGHT = 1080;
constexpr int32_t DEFAULT_WIDTH_64_ALIGN = 2560;
constexpr int32_t DEFAULT_WIDTH_16_ALIGN = 2528;

class BufferConverterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

private:
    std::shared_ptr<BufferConverter> converter_ = nullptr;
    std::shared_ptr<AVBuffer> buffer_ = nullptr;
    std::shared_ptr<AVSharedMemory> memory_ = nullptr;
};

void BufferConverterUnitTest::SetUpTestCase(void) {}

void BufferConverterUnitTest::TearDownTestCase(void) {}

void BufferConverterUnitTest::SetUp(void) {}

void BufferConverterUnitTest::TearDown(void)
{
    converter_ = nullptr;
    buffer_ = nullptr;
    memory_ = nullptr;
}

std::shared_ptr<AVBuffer> CreateSurfaceAVBuffer(const int32_t width, const int32_t height,
                                                const GraphicPixelFormat pixFormat)
{
    const BufferRequestConfig config = {
        .width = width,
        .height = height,
        .strideAlignment = 0x40,
        .format = pixFormat,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };
    auto allocator = AVAllocatorFactory::CreateSurfaceAllocator(config);
    EXPECT_NE(allocator, nullptr);
    return AVBuffer::CreateAVBuffer(allocator, 0, 0);
}

std::shared_ptr<AVBuffer> CreateSharedAVBuffer(const int32_t size)
{
    auto allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    EXPECT_NE(allocator, nullptr);
    return AVBuffer::CreateAVBuffer(allocator, size, 0);
}

std::shared_ptr<AVSharedMemory> CreateAVSharedMemory(const int32_t size)
{
    return AVSharedMemoryBase::CreateFromLocal(size, AVSharedMemory::Flags::FLAGS_READ_WRITE, "buffer_converter_test");
}

/**
 * @tc.name: Create_Encoder_Converter_001
 * @tc.desc: create encoder buffer converter
 */
HWTEST_F(BufferConverterUnitTest, Create_Encoder_Converter_001, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_ENCODER);
    EXPECT_NE(converter_, nullptr);
    EXPECT_EQ(converter_->isEncoder_, true);
}

/**
 * @tc.name: Create_Decoder_Converter_001
 * @tc.desc: create decoder buffer converter
 */
HWTEST_F(BufferConverterUnitTest, Create_Decoder_Converter_001, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    EXPECT_NE(converter_, nullptr);
    EXPECT_EQ(converter_->isEncoder_, false);
}

/**
 * @tc.name: Create_Invalid_Converter_001
 * @tc.desc: create decoder buffer converter
 */
HWTEST_F(BufferConverterUnitTest, Create_Invalid_Converter_001, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    EXPECT_NE(converter_, nullptr);
    converter_ = BufferConverter::Create(static_cast<AVCodecType>(-1));
    EXPECT_EQ(converter_, nullptr);
}

/**
 * @tc.name: SetInputBufferFormat_001
 * @tc.desc: set input buffer format, YUV 8 bit
 */
HWTEST_F(BufferConverterUnitTest, SetInputBufferFormat_001, TestSize.Level1)
{
    const std::vector<GraphicPixelFormat> testList = {GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P,
                                                      GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP,
                                                      GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP};
    for (const auto &pixcelFormat : testList) {
        converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_ENCODER);
        buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, pixcelFormat);
        UNITTEST_CHECK_AND_CONTINUE_LOG(buffer_ != nullptr, "not support this pixcel format:%d",
                                        static_cast<int32_t>(pixcelFormat));
        UNITTEST_INFO_LOG("pixcel format:%d", static_cast<int32_t>(pixcelFormat));
        converter_->SetInputBufferFormat(buffer_);
        EXPECT_EQ(converter_->rect_.wStride, DEFAULT_WIDTH);
        EXPECT_EQ(converter_->rect_.hStride, DEFAULT_HEIGHT);
        EXPECT_LE(converter_->hwRect_.wStride, DEFAULT_WIDTH_64_ALIGN);
        EXPECT_EQ(converter_->hwRect_.hStride, DEFAULT_HEIGHT);
        EXPECT_EQ(converter_->usrRect_.wStride, DEFAULT_WIDTH_16_ALIGN);
        EXPECT_EQ(converter_->usrRect_.hStride, DEFAULT_HEIGHT);
        EXPECT_EQ(converter_->needResetFormat_, false);
    }
}

/**
 * @tc.name: SetInputBufferFormat_002
 * @tc.desc: set input buffer format, YUV 10 bit
 */
HWTEST_F(BufferConverterUnitTest, SetInputBufferFormat_002, TestSize.Level1)
{
    const std::vector<GraphicPixelFormat> testList = {GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_P010,
                                                      GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_P010};
    for (const auto &pixcelFormat : testList) {
        converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_ENCODER);
        buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, pixcelFormat);
        UNITTEST_CHECK_AND_CONTINUE_LOG(buffer_ != nullptr, "not support this pixcel format:%d",
                                        static_cast<int32_t>(pixcelFormat));
        UNITTEST_INFO_LOG("pixcel format:%d", static_cast<int32_t>(pixcelFormat));
        converter_->SetInputBufferFormat(buffer_);
        EXPECT_EQ(converter_->rect_.wStride, DEFAULT_WIDTH * 2); // 2: pixcelSize
        EXPECT_EQ(converter_->rect_.hStride, DEFAULT_HEIGHT);
        EXPECT_LE(converter_->hwRect_.wStride, DEFAULT_WIDTH_64_ALIGN * 2); // 2: pixcelSize
        EXPECT_EQ(converter_->hwRect_.hStride, DEFAULT_HEIGHT);
        EXPECT_EQ(converter_->usrRect_.wStride, DEFAULT_WIDTH_16_ALIGN * 2); // 2: pixcelSize
        EXPECT_EQ(converter_->usrRect_.hStride, DEFAULT_HEIGHT);
        EXPECT_EQ(converter_->needResetFormat_, false);
    }
}

/**
 * @tc.name: SetInputBufferFormat_003
 * @tc.desc: set input buffer format, RGBA
 */
HWTEST_F(BufferConverterUnitTest, SetInputBufferFormat_003, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_ENCODER);
    GraphicPixelFormat pixcelFormat = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
    buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, pixcelFormat);
    UNITTEST_CHECK_AND_RETURN_LOG(buffer_ != nullptr, "not support this pixcel format:%d",
                                  static_cast<int32_t>(pixcelFormat));
    UNITTEST_INFO_LOG("pixcel format:%d", static_cast<int32_t>(pixcelFormat));
    converter_->SetInputBufferFormat(buffer_);
    EXPECT_EQ(converter_->rect_.wStride, DEFAULT_WIDTH * 4); // 4: pixcelSize
    EXPECT_EQ(converter_->rect_.hStride, DEFAULT_HEIGHT);
    EXPECT_LE(converter_->hwRect_.wStride, DEFAULT_WIDTH_64_ALIGN * 4); // 4: pixcelSize
    EXPECT_EQ(converter_->hwRect_.hStride, DEFAULT_HEIGHT);
    EXPECT_EQ(converter_->usrRect_.wStride, DEFAULT_WIDTH_16_ALIGN * 4); // 4: pixcelSize
    EXPECT_EQ(converter_->usrRect_.hStride, DEFAULT_HEIGHT);
    EXPECT_EQ(converter_->needResetFormat_, false);
}

/**
 * @tc.name: SetInputBufferFormat_004
 * @tc.desc: set input buffer format, shared memory
 */
HWTEST_F(BufferConverterUnitTest, SetInputBufferFormat_004, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_ENCODER);
    buffer_ = CreateSharedAVBuffer(1);
    converter_->SetInputBufferFormat(buffer_);
    EXPECT_EQ(converter_->rect_.wStride, 0);
    EXPECT_EQ(converter_->rect_.hStride, 0);
    EXPECT_EQ(converter_->hwRect_.wStride, 0);
    EXPECT_EQ(converter_->hwRect_.hStride, 0);
    EXPECT_EQ(converter_->usrRect_.wStride, 0);
    EXPECT_EQ(converter_->usrRect_.hStride, 0);
    EXPECT_EQ(converter_->needResetFormat_, false);
}

/**
 * @tc.name: Invalid_SetInputBufferFormat_001
 * @tc.desc: video decoder set input buffer format
 */
HWTEST_F(BufferConverterUnitTest, Invalid_SetInputBufferFormat_001, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    GraphicPixelFormat pixcelFormat = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
    buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, pixcelFormat);
    UNITTEST_CHECK_AND_RETURN_LOG(buffer_ != nullptr, "not support this pixcel format:%d",
                                  static_cast<int32_t>(pixcelFormat));
    converter_->SetInputBufferFormat(buffer_);
    EXPECT_EQ(converter_->rect_.wStride, 0);
    EXPECT_EQ(converter_->rect_.hStride, 0);
    EXPECT_EQ(converter_->hwRect_.wStride, 0);
    EXPECT_EQ(converter_->hwRect_.hStride, 0);
    EXPECT_EQ(converter_->usrRect_.wStride, 0);
    EXPECT_EQ(converter_->usrRect_.hStride, 0);
    EXPECT_EQ(converter_->needResetFormat_, true);
}

/**
 * @tc.name: Invalid_SetInputBufferFormat_002
 * @tc.desc: video encoder set input buffer format, but already set format.
 */
HWTEST_F(BufferConverterUnitTest, Invalid_SetInputBufferFormat_002, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_ENCODER);
    GraphicPixelFormat pixcelFormat = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
    buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, pixcelFormat);
    UNITTEST_CHECK_AND_RETURN_LOG(buffer_ != nullptr, "not support this pixcel format:%d",
                                  static_cast<int32_t>(pixcelFormat));
    converter_->needResetFormat_ = false;
    converter_->SetInputBufferFormat(buffer_);
    EXPECT_EQ(converter_->rect_.wStride, 0);
    EXPECT_EQ(converter_->rect_.hStride, 0);
    EXPECT_EQ(converter_->hwRect_.wStride, 0);
    EXPECT_EQ(converter_->hwRect_.hStride, 0);
    EXPECT_EQ(converter_->usrRect_.wStride, 0);
    EXPECT_EQ(converter_->usrRect_.hStride, 0);
    EXPECT_EQ(converter_->needResetFormat_, false);
}

/**
 * @tc.name: SetOutputBufferFormat_001
 * @tc.desc: set input buffer format, shared memory
 */
HWTEST_F(BufferConverterUnitTest, SetOutputBufferFormat_001, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    buffer_ = CreateSharedAVBuffer(1);
    converter_->SetOutputBufferFormat(buffer_);
    EXPECT_EQ(converter_->rect_.wStride, 0);
    EXPECT_EQ(converter_->rect_.hStride, 0);
    EXPECT_EQ(converter_->hwRect_.wStride, 0);
    EXPECT_EQ(converter_->hwRect_.hStride, 0);
    EXPECT_EQ(converter_->usrRect_.wStride, 0);
    EXPECT_EQ(converter_->usrRect_.hStride, 0);
    EXPECT_EQ(converter_->needResetFormat_, false);
}

/**
 * @tc.name: Invalid_SetOutputBufferFormat_001
 * @tc.desc: video decoder set output buffer format
 */
HWTEST_F(BufferConverterUnitTest, Invalid_SetOutputBufferFormat_001, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_ENCODER);
    GraphicPixelFormat pixcelFormat = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
    buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, pixcelFormat);
    UNITTEST_CHECK_AND_RETURN_LOG(buffer_ != nullptr, "not support this pixcel format:%d",
                                  static_cast<int32_t>(pixcelFormat));
    converter_->SetOutputBufferFormat(buffer_);
    EXPECT_EQ(converter_->rect_.wStride, 0);
    EXPECT_EQ(converter_->rect_.hStride, 0);
    EXPECT_EQ(converter_->hwRect_.wStride, 0);
    EXPECT_EQ(converter_->hwRect_.hStride, 0);
    EXPECT_EQ(converter_->usrRect_.wStride, 0);
    EXPECT_EQ(converter_->usrRect_.hStride, 0);
    EXPECT_EQ(converter_->needResetFormat_, true);
}

/**
 * @tc.name: Invalid_SetOutputBufferFormat_002
 * @tc.desc: video decoder set output buffer format
 */
HWTEST_F(BufferConverterUnitTest, Invalid_SetOutputBufferFormat_002, TestSize.Level1)
{
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    GraphicPixelFormat pixcelFormat = GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888;
    buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, pixcelFormat);
    UNITTEST_CHECK_AND_RETURN_LOG(buffer_ != nullptr, "not support this pixcel format:%d",
                                  static_cast<int32_t>(pixcelFormat));
    converter_->needResetFormat_ = false;
    converter_->SetOutputBufferFormat(buffer_);
    EXPECT_EQ(converter_->rect_.wStride, 0);
    EXPECT_EQ(converter_->rect_.hStride, 0);
    EXPECT_EQ(converter_->hwRect_.wStride, 0);
    EXPECT_EQ(converter_->hwRect_.hStride, 0);
    EXPECT_EQ(converter_->usrRect_.wStride, 0);
    EXPECT_EQ(converter_->usrRect_.hStride, 0);
    EXPECT_EQ(converter_->needResetFormat_, false);
}

/**
 * @tc.name: ReadFromBuffer_001
 * @tc.desc: buffer->memory_ is nullptr
 */
HWTEST_F(BufferConverterUnitTest, ReadFromBuffer_001, TestSize.Level1)
{
    int32_t testSize = DEFAULT_HEIGHT * DEFAULT_WIDTH * 3 / 2; // 3: nom, 2: denom
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    buffer_ = AVBuffer::CreateAVBuffer();
    memory_ = CreateAVSharedMemory(testSize);
    int32_t ret = converter_->ReadFromBuffer(buffer_, memory_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: ReadFromBuffer_002
 * @tc.desc: buffer->memory_ is shared memory
 */
HWTEST_F(BufferConverterUnitTest, ReadFromBuffer_002, TestSize.Level1)
{
    int32_t testSize = DEFAULT_HEIGHT * DEFAULT_WIDTH * 3 / 2; // 3: nom, 2: denom
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    buffer_ = CreateSharedAVBuffer(testSize);
    memory_ = CreateAVSharedMemory(testSize);
    converter_->isSharedMemory_ = true;
    int32_t ret = converter_->ReadFromBuffer(buffer_, memory_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: ReadFromBuffer_003
 * @tc.desc: buffer->memory_ size is 0
 */
HWTEST_F(BufferConverterUnitTest, ReadFromBuffer_003, TestSize.Level1)
{
    int32_t testSize = DEFAULT_HEIGHT * DEFAULT_WIDTH * 4; // 4: pixcelSize
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888);
    memory_ = CreateAVSharedMemory(testSize);
    buffer_->memory_->SetSize(0);
    int32_t ret = converter_->ReadFromBuffer(buffer_, memory_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: WriteToBuffer_001
 * @tc.desc: buffer->memory_ is nullptr
 */
HWTEST_F(BufferConverterUnitTest, WriteToBuffer_001, TestSize.Level1)
{
    int32_t testSize = DEFAULT_HEIGHT * DEFAULT_WIDTH * 3 / 2; // 3: nom, 2: denom
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    buffer_ = AVBuffer::CreateAVBuffer();
    memory_ = CreateAVSharedMemory(testSize);
    int32_t ret = converter_->WriteToBuffer(buffer_, memory_);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name: WriteToBuffer_002
 * @tc.desc: buffer->memory_ is shared memory
 */
HWTEST_F(BufferConverterUnitTest, WriteToBuffer_002, TestSize.Level1)
{
    int32_t testSize = DEFAULT_HEIGHT * DEFAULT_WIDTH * 3 / 2; // 3: nom, 2: denom
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    buffer_ = CreateSharedAVBuffer(testSize);
    memory_ = CreateAVSharedMemory(testSize);
    converter_->isSharedMemory_ = true;
    int32_t ret = converter_->WriteToBuffer(buffer_, memory_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: WriteToBuffer_003
 * @tc.desc: buffer->memory_ size is 0
 */
HWTEST_F(BufferConverterUnitTest, WriteToBuffer_003, TestSize.Level1)
{
    int32_t testSize = DEFAULT_HEIGHT * DEFAULT_WIDTH * 4; // 4: pixcelSize
    converter_ = BufferConverter::Create(AVCODEC_TYPE_VIDEO_DECODER);
    buffer_ = CreateSurfaceAVBuffer(DEFAULT_WIDTH, DEFAULT_HEIGHT, GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888);
    memory_ = CreateAVSharedMemory(testSize);
    buffer_->memory_->SetSize(0);
    int32_t ret = converter_->WriteToBuffer(buffer_, memory_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}
} // namespace
