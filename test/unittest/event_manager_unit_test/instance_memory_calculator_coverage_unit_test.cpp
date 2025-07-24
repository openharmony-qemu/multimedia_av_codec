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
#include "avcodec_list.h"
#include "avcodec_video_encoder.h"
#include "format.h"
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_avcodec_videoencoder.h"
#include "native_avformat.h"

#define TEST_SUIT InstanceMemoryCalculatorCoverageUintTest

using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

namespace {
uint32_t DEFAULT_WIDTH = 1280;
uint32_t DEFAULT_HEIGHT = 720;

class TEST_SUIT : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void){};
    void SetUp(void);
    void TearDown(void);
    void CreateCodec(std::string_view codecType, bool isDecoder, VideoPixelFormat pixelFormat, bool configMain10,
                     bool configPostProcessing);

    std::vector<std::shared_ptr<OH_AVFormat>> format_;
    std::vector<std::shared_ptr<OH_AVCodec>> videoSoftwareEnc_;
    std::vector<std::shared_ptr<OH_AVCodec>> videoHardwareHevcDec_;
    std::vector<std::shared_ptr<OH_AVCodec>> videoHardwareVvcDec_;
};

void TEST_SUIT::SetUpTestCase(void) {}

void TEST_SUIT::SetUp(void) {}

void TEST_SUIT::TearDown(void)
{
    format_.clear();
    videoSoftwareEnc_.clear();
    videoHardwareHevcDec_.clear();
    videoHardwareVvcDec_.clear();
}

void SetFormatBasicParam(bool isDecoder) {}

void TEST_SUIT::CreateCodec(std::string_view codecType, bool isDecoder, VideoPixelFormat pixelFormat, bool configMain10,
                            bool configPostProcessing)
{
    auto format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(pixelFormat)));
    if (configMain10) {
        ASSERT_EQ(true, OH_AVFormat_SetIntValue(format, OH_MD_KEY_PROFILE, OH_HEVCProfile::HEVC_PROFILE_MAIN_10));
    }
    format_.emplace_back(format, [](OH_AVFormat *ptr) { OH_AVFormat_Destroy(ptr); });

    if (codecType == CodecMimeType::VIDEO_HEVC && isDecoder) {
        auto codec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
        ASSERT_NE(nullptr, codec);
        OH_AVErrCode ret = OH_VideoDecoder_Configure(codec, format);
        ASSERT_NE(AV_ERR_OK, ret);
        videoHardwareHevcDec_.emplace_back(codec, [](OH_AVCodec *ptr) { OH_VideoDecoder_Destroy(ptr); });
    } else if (codecType == CodecMimeType::VIDEO_VVC && isDecoder) {
        auto codec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_VVC);
        ASSERT_NE(nullptr, codec);
        OH_AVErrCode ret = OH_VideoDecoder_Configure(codec, format);
        ASSERT_NE(AV_ERR_OK, ret);
        videoHardwareVvcDec_.emplace_back(codec, [](OH_AVCodec *ptr) { OH_VideoDecoder_Destroy(ptr); });
    } else if (codecType == CodecMimeType::VIDEO_AVC && !isDecoder) {
        auto codec = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        ASSERT_NE(nullptr, codec);
        OH_AVErrCode ret = OH_VideoDecoder_Configure(codec, format);
        ASSERT_NE(AV_ERR_OK, ret);
        videoSoftwareEnc_.emplace_back(codec, [](OH_AVCodec *ptr) { OH_VideoEncoder_Destroy(ptr); });
    }
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_001, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                    configPostProcessing);
    }
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_002, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 2; i++) {
            CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_003, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 4; i++) {
            CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderHevc10BitYUV420_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderHevc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevc10BitYUV420_TEST_004, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 15; i++) {
            CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_001, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = true;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                    configPostProcessing);
    }
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_002, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = true;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 2; i++) {
            CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_003, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = true;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 4; i++) {
            CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderHevcYUV420PostProcessing_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderHevcYUV420PostProcessing function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderHevcYUV420PostProcessing_TEST_004, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = true;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 15; i++) {
            CreateCodec(CodecMimeType::VIDEO_HEVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderVvc10BitYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderVvc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvc10BitYUV420_TEST_001, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10, configPostProcessing);
    }
}

/**
 * @tc.name: HardwareDecoderVvc10BitYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderVvc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvc10BitYUV420_TEST_002, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 2; i++) {
            CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderVvc10BitYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderVvc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvc10BitYUV420_TEST_003, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 4; i++) {
            CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderVvc10BitYUV420_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderVvc10BitYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvc10BitYUV420_TEST_004, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = true;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 15; i++) {
            CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderVvcYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderVvcYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvcYUV420_TEST_001, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10, configPostProcessing);
    }
}

/**
 * @tc.name: HardwareDecoderVvcYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderVvcYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvcYUV420_TEST_002, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 2; i++) {
            CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderVvcYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderVvcYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvcYUV420_TEST_003, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 4; i++) {
            CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderVvcYUV420_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderVvcYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderVvcYUV420_TEST_004, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_VVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 15; i++) {
            CreateCodec(CodecMimeType::VIDEO_VVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_001
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_001, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10, configPostProcessing);
    }
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_002
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_002, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 2; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_003
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_003, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 4; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: HardwareDecoderYUV420_TEST_004
 * @tc.desc: add the coverage of HardwareDecoderYUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, HardwareDecoderYUV420_TEST_004, TestSize.Level3)
{
    constexpr bool isDecoder = true;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 15; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_001
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_001, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10, configPostProcessing);
    }
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_002
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_002, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 2; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_003
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_003, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 4; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: SoftwareEncoderH264YUV420_TEST_004
 * @tc.desc: add the coverage of SoftwareEncoderH264YUV420 function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264YUV420_TEST_004, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 15; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::YUVI420, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_001
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_001, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::RGBA, configMain10, configPostProcessing);
    }
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_002
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_002, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 2; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::RGBA, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_003
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_003, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 4; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::RGBA, configMain10,
                        configPostProcessing);
        }
    }
}

/**
 * @tc.name: SoftwareEncoderH264RGBA_TEST_004
 * @tc.desc: add the coverage of SoftwareEncoderH264RGBA function
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, SoftwareEncoderH264RGBA_TEST_004, TestSize.Level3)
{
    constexpr bool isDecoder = false;
    constexpr bool configMain10 = false;
    constexpr bool configPostProcessing = false;
    OH_AVCapability *decoderCapability = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_VIDEO_AVC, false);
    if (decoderCapability != nullptr) {
        for (int32_t i = 0; i < 15; i++) {
            CreateCodec(CodecMimeType::VIDEO_AVC, isDecoder, VideoPixelFormat::RGBA, configMain10,
                        configPostProcessing);
        }
    }
}

} // namespace