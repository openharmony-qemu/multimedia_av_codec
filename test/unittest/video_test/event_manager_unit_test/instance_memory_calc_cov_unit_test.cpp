/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <gtest/gtest.h>
#include <vector>
#include "av_common.h"
#include "avcodec_info.h"
#include "codeclist_mock.h"
#include "instance_memory_update_event_handler.h"
#include "meta.h"
#include "meta/meta_key.h"
#include "meta/video_types.h"

#define TEST_SUIT InstanceMemoryCalculatorCoverageUintTest

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

namespace {
class TEST_SUIT : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    void UpdateMetaData(int32_t pixelFormat, int32_t bitDepth, AVCodecType codecType, int32_t isHardware,
                        bool enablePostProcessing);
    void IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat pixelFormat);

    std::shared_ptr<OHOS::MediaAVCodec::CodecListMock> capability_ = nullptr;
    std::shared_ptr<Meta> meta_ = nullptr;
    std::shared_ptr<InstanceMemoryUpdateEventHandler> instanceMemoryHandler_ = nullptr;
};

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    meta_ = std::make_shared<Meta>();
    EXPECT_NE(meta_, nullptr);
    instanceMemoryHandler_ = std::make_shared<InstanceMemoryUpdateEventHandler>();
    EXPECT_NE(instanceMemoryHandler_, nullptr);
}

void TEST_SUIT::TearDown(void)
{
    instanceMemoryHandler_ = nullptr;
    meta_->Clear();
}

void TEST_SUIT::IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat pixelFormat)
{
    capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                AVCodecCategory::AVCODEC_HARDWARE);
    auto pixelFormats = capability_->GetVideoSupportedPixelFormats();
    if (std::find(pixelFormats.begin(), pixelFormats.end(), static_cast<int32_t>(pixelFormat)) == pixelFormats.end()) {
        GTEST_SKIP() << "Unsupport pixel format = " << static_cast<int32_t>(pixelFormat);
    }
}

void TEST_SUIT::UpdateMetaData(int32_t pixelFormat, int32_t bitDepth, AVCodecType codecType, int32_t isHardware,
                               bool enablePostProcessing)
{
    SetMetaData(*meta_, Media::Tag::VIDEO_PIXEL_FORMAT, pixelFormat);
    SetMetaData(*meta_, EventInfoExtentedKey::BIT_DEPTH.data(), bitDepth);
    meta_->SetData(EventInfoExtentedKey::CODEC_TYPE.data(), codecType);
    meta_->SetData(EventInfoExtentedKey::IS_HARDWARE.data(), isHardware);
    meta_->SetData(EventInfoExtentedKey::ENABLE_POST_PROCESSING.data(), enablePostProcessing);
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_001, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 1, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12_10bit");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 1000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 39212;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_002, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 1, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12_10bit");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 5000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 89987;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_003, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 1, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12_10bit");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 10000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 149023;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_004, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 1, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12_10bit");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 50000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 595178;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_001, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, true);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 1000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 75791;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_002, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, true);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 5000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 133940;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_003, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, true);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 10000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 201870;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_004, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, true);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_HEVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 50000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 720018;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderVvc10BitYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderVvc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvc10BitYUV420_TEST_001, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 1, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_VVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12_10bit");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 1000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 19218;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderVvc10BitYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderVvc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvc10BitYUV420_TEST_002, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 1, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_VVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12_10bit");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 5000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 50271;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderVvc10BitYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderVvc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvc10BitYUV420_TEST_003, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 1, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_VVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12_10bit");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 10000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 92178;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderVvcYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderVvcYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvcYUV420_TEST_001, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_VVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 1000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 16273;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderVvcYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderVvcYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvcYUV420_TEST_002, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_VVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 5000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 36299;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderVvcYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderVvcYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvcYUV420_TEST_003, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_VVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 10000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 64802;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_001, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 1000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 34636;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_002, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 5000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 69883;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_003, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 10000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 109830;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_004, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, 1, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 50000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 403649;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_001
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_001, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 1000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 13416;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_002
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_002, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 6000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 34783;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_003
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_003, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 10000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 62246;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_004
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_004, TestSize.Level3)
{
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "NV12");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 50000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 183683;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_001
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_001, TestSize.Level3)
{
    IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "RGBA");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 1000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 15001;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_002
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_002, TestSize.Level3)
{
    IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "RGBA");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 6000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 42631;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_003
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_003, TestSize.Level3)
{
    IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "RGBA");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 10000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 75232;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_004
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_004, TestSize.Level3)
{
    IsPixelFormatSupported(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    int32_t pixelFormat = static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::RGBA);
    UpdateMetaData(pixelFormat, 0, AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER, 0, false);
    meta_->SetData(Media::Tag::MIME_TYPE, MimeType::VIDEO_AVC);
    meta_->SetData(EventInfoExtentedKey::PIXEL_FORMAT_STRING.data(), "RGBA");
    auto calculator = instanceMemoryHandler_->GetCalculator(*meta_);
    EXPECT_NE(calculator, std::nullopt);
    uint32_t blockCount = 50000;
    auto instanceMemory = calculator.value()(blockCount);
    constexpr int32_t INSTANCE_MEMORY = 242744;
    EXPECT_EQ(instanceMemory, INSTANCE_MEMORY);
}
} // namespace