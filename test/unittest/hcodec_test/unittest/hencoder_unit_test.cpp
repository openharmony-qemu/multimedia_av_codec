/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hencoder_unit_test.h"
#include <string>
#include "av_common.h"
#include "avcodec_info.h" // foundation/multimedia/player_framework/interfaces/inner_api/native
#include "hcodec_log.h"
#include "media_description.h" // foundation/multimedia/player_framework/interfaces/inner_api/native
#include "tester_common.h"

namespace OHOS::MediaAVCodec {
using namespace std;
using namespace testing::ext;

/*========================================================*/
/*                     HEncoderCallback                   */
/*========================================================*/
void HEncoderCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
}

void HEncoderCallback::OnOutputFormatChanged(const Format &format)
{
}

void HEncoderCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
}

void HEncoderCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
}

/*========================================================*/
/*               HEncoderPreparingUnitTest                */
/*========================================================*/
void HEncoderPreparingUnitTest::SetUpTestCase(void)
{
}

void HEncoderPreparingUnitTest::TearDownTestCase(void)
{
}

void HEncoderPreparingUnitTest::SetUp(void)
{
    TLOGI("----- %s -----", ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

void HEncoderPreparingUnitTest::TearDown(void)
{
}

sptr<Surface> HEncoderPreparingUnitTest::CreateProducerSurface()
{
    sptr<Surface> consumerSurface  = Surface::CreateSurfaceAsConsumer();
    if (consumerSurface == nullptr) {
        TLOGE("Create the surface consummer fail");
        return nullptr;
    }

    sptr<IBufferProducer> producer = consumerSurface->GetProducer();
    if (producer == nullptr) {
        TLOGE("Get the surface producer fail");
        return nullptr;
    }

    sptr<Surface> producerSurface  = Surface::CreateSurfaceAsProducer(producer);
    if (producerSurface == nullptr) {
        TLOGE("CreateSurfaceAsProducer fail");
        return nullptr;
    }
    return producerSurface;
}

sptr<Surface> HEncoderPreparingUnitTest::CreateConsumerSurface()
{
    sptr<Surface> consumerSurface  = Surface::CreateSurfaceAsConsumer();
    return consumerSurface;
}

static int32_t GetEncoderCapabilityForMime(CapabilityData &cap,
                                           const std::string &targetMimeType)
{
    vector<CapabilityData> caps;
    int32_t ret = GetHCodecCapabilityList(caps);
    if (ret != AVCS_ERR_OK) {
        return ret;
    }

    auto it = std::find_if(caps.begin(), caps.end(), [&](const CapabilityData& one) {
        return one.codecType == AVCODEC_TYPE_VIDEO_ENCODER && one.mimeType == targetMimeType;
    });
    if (it != caps.end()) {
        cap = *it;
    }
    return AVCS_ERR_OK;
}

/* ============== CREATION ============== */
HWTEST_F(HEncoderPreparingUnitTest, create_by_avc_name, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj != nullptr);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
}

HWTEST_F(HEncoderPreparingUnitTest, create_by_hevc_name, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
}

HWTEST_F(HEncoderPreparingUnitTest, create_by_empty_name, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create("");
    ASSERT_FALSE(testObj);
}

/* ============== SET_CALLBACK ============== */
HWTEST_F(HEncoderPreparingUnitTest, set_empty_callback, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    int32_t ret = testObj->SetCallback(nullptr);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, set_valid_callback, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    shared_ptr<HEncoderCallback> callback = make_shared<HEncoderCallback>();
    int32_t ret = testObj->SetCallback(callback);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/* ============== CREATE_INPUT_SURFACE ============== */
HWTEST_F(HEncoderPreparingUnitTest, create_input_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
}

HWTEST_F(HEncoderPreparingUnitTest, create_redundant_input_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    sptr<Surface> inputSurface = CreateConsumerSurface();
    int32_t ret = testObj->SetInputSurface(inputSurface);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    sptr<Surface> surface = testObj->CreateInputSurface();
    ASSERT_FALSE(surface);
}

/* ============== SET_INPUT_SURFACE ============== */
HWTEST_F(HEncoderPreparingUnitTest, set_empty_input_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    int32_t ret = testObj->SetInputSurface(nullptr);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, set_redundant_input_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    int32_t ret = testObj->SetInputSurface(inputSurface);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, set_producer_input_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    sptr<Surface> inputSurface = CreateProducerSurface();
    int32_t ret = testObj->SetInputSurface(inputSurface);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, set_consumer_input_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    sptr<Surface> inputSurface = CreateConsumerSurface();
    int32_t ret = testObj->SetInputSurface(inputSurface);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/* ============== SET_OUTPUT_SURFACE ============== */
HWTEST_F(HEncoderPreparingUnitTest, unsupported_set_output_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    int32_t ret = testObj->SetOutputSurface(nullptr);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT, ret);
}

/* ============== CONFIGURE ============== */
HWTEST_F(HEncoderPreparingUnitTest, configure_avc_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000); // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, AVC_PROFILE_BASELINE);
    format.PutIntValue("max-bframes", 1);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_high_profile, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000); // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, AVC_PROFILE_HIGH);
    format.PutIntValue("max-bframes", 1);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_profile_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10); // 10 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000);  // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, HEVC_PROFILE_MAIN);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_profile_ok_with_zero_i_frame_interval, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 0); // 0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000);  // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, HEVC_PROFILE_MAIN);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_no_width, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_width, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 0); // 0 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_maxwidth, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 10001); // out of the width
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_pixelfmt, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::UNKNOWN));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_height, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 0); // invalid hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_with_invalid_maxheight, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 10001); // out of the height
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_NE(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_color_primaries1, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, -1);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_color_primaries2, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000); // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, AVC_PROFILE_BASELINE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, 256); // 1-12
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_characteristics1, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, -1);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}
 
HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_characteristics2, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000); // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, AVC_PROFILE_BASELINE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, 256);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_coefficients1, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, -1);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_coefficients2, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000); // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, AVC_PROFILE_BASELINE);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, 256); // 1-12
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

#ifdef HMOS_TEST
HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_invalid_qprange, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, 0); // invalid MinQp
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, 52); // invalid MaxQp
    int32_t ret = testObj->Configure(format);
    ASSERT_NE(AVCS_ERR_OK, ret);
}
#endif
 
HWTEST_F(HEncoderPreparingUnitTest, configure_avc_no_height, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_no_color_format, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_no_frame_rate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_double_frame_rate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, -30.0); // -30.0 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_invalid_int_frame_rate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, -60); // -60 frame rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_cbr_bitrate_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0); // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000);  // 3000000 bit rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_vbr_bitrate_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0); // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000);  // 3000000 bit rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_vbr_bitrate_without_bitrate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0); // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_vbr_bitrate_with_invalid_bitrate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0); // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 0);  // invalid bit rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_cq_bitrate_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, 10);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_cq_bitrate_without_quality, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_cq_bitrate_invalid_quality, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CQ);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, -1);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_CRF_bitrate_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CRF);
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TARGET_QP, 30);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}
 
HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_CRF_bitrate_invalid_targetQp, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CRF);
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TARGET_QP, -1);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

#ifdef HMOS_TEST
HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_invalid_sqr_bitrate_with_quality, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, 30);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_sqr_bitrate_with_sqrfactor_maxbitrate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, 30);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, 12000000);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_sqr_bitrate_with_sqrfactor_without_maxbitrate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, 30);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_sqr_bitrate_with_maxbitrate_without_sqrfactor, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, 12000000);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_sqr_bitrate_with_nothing, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_sqr_bitrate_with_bitrate_without_maxbitrate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 10000);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_sqr_bitrate_with_maxbitrate_with_invalid_bitrate, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, SQR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_MAX_BITRATE, 12000000);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 10000);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}
#endif

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_invalid_bitrate_mode, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0); // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, 10); // 10 is invalid bitrate mode
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000);  // 3000000 bit rate
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_no_bitrate_mode, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0); // 10.0 I-Frame interval
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_LTR_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);

    auto ltrCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
    if (ltrCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2); // 2: ltr frame count
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_OK, ret);
    }
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_invalid_LTR_count, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);

    auto ltrCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
    if (ltrCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 11); // 11: ltr frame count
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
    }
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_temporal_scale_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);
    
    auto TSVCCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    if (TSVCCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 2); // 2: gop mode
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4); // 4: temporal gop size
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_OK, ret);
    }
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_invalid_temporal_GOP_mode, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);

    auto TSVCCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    if (TSVCCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 1); // 1: gop mode
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4); // 4: temporal gop size
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
    }
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_invalid_temporal_GOP_size, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);

    auto TSVCCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    if (TSVCCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 2); // 2: gop mode
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 8); // 8: invalid temporal gop size
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
    }
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_no_temporal_GOP_size, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);
 
    auto TSVCCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    if (TSVCCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 2); // 2: gop mode
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
    }
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_invalid_temporal_scale_and_LTR, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);

    auto TSVCCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    auto LTRCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
    if (TSVCCap != cap.featuresMap.end() && LTRCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 1);
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE, 2); // 2: gop mode
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE, 4); // 4: invalid temporal gop size
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_LTR_FRAME_COUNT, 2); // 2: ltr frame count
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
    }
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_repeat_encode_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, 30); // after 30ms repeat
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, 10); // max repeat 10 frames
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}
 
HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_repeat_encode_invalid_after, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, 0); // after 0ms repeat
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, 10); // max repeat 10 frames
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
}
 
HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_repeat_encode_invalid_maxCount, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, 30); // after 30ms repeat
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT, 0); // max repeat 10 frames
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
}
 
HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_repeat_encode_no_maxCount, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER, 30); // after 30ms repeat
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_enable_qp_map_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/avc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);
 
    auto qpMapCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_QP_MAP));
    if (qpMapCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, 1);
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_OK, ret);
    }
}
 
HWTEST_F(HEncoderPreparingUnitTest, configure_HEVC_enable_qp_map_ok, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, "video/hevc");
    ASSERT_TRUE(ret == AVCS_ERR_OK);
 
    auto qpMapCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_QP_MAP));
    if (qpMapCap != cap.featuresMap.end()) {
        Media::Meta meta{};
        int32_t err = testObj->Init(meta);
        ASSERT_TRUE(err == AVCS_ERR_OK);
        Format format;
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_HEVC);
        format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
        format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
        format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
        format.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_QP_MAP, 1);
        ret = testObj->Configure(format);
        ASSERT_EQ(AVCS_ERR_OK, ret);
    }
}

static void ConfigureGopModeForBFrameTest(const std::string &targetType, const std::string_view &targetMimeType)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, targetType));
    ASSERT_TRUE(testObj);
    CapabilityData cap;
    int32_t ret = GetEncoderCapabilityForMime(cap, targetType);
    ASSERT_TRUE(ret == AVCS_ERR_OK);

    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, targetMimeType);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1920);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 1080);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0);
    ASSERT_EQ(AVCS_ERR_OK, ret);

#ifdef HMOS_TEST
    auto bEncodeCap = cap.featuresMap.find(static_cast<int32_t>(AVCapabilityFeature::VIDEO_ENCODER_B_FRAME));
    bool supportB = (bEncodeCap != cap.featuresMap.end());
    TLOGI("%s b frame, support B %d", targetType.c_str(), supportB);

    format.PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
                       Media::Plugins::VIDEO_ENCODE_GOP_ADAPTIVE_B_MODE);
    ret = testObj->Configure(format);
    ASSERT_EQ(supportB ? AVCS_ERR_OK : AVCS_ERR_UNSUPPORT, ret);

    format.PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE,
                       Media::Plugins::VIDEO_ENCODE_GOP_H3B_MODE);
    ret = testObj->Configure(format);
    ASSERT_EQ(supportB ? AVCS_ERR_OK : AVCS_ERR_UNSUPPORT, ret);

    format.PutIntValue(Media::Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE, 1000);
    ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_UNSUPPORT, ret);
#endif
}

HWTEST_F(HEncoderPreparingUnitTest, configure_avc_gop_mode_for_b_frame, TestSize.Level1)
{
    ConfigureGopModeForBFrameTest("video/avc", CodecMimeType::VIDEO_AVC);
}

HWTEST_F(HEncoderPreparingUnitTest, configure_hevc_gop_mode_for_b_frame, TestSize.Level1)
{
    ConfigureGopModeForBFrameTest("video/hevc", CodecMimeType::VIDEO_HEVC);
}

/* ============== GET_OUTPUT_FORMAT ============== */
HWTEST_F(HEncoderPreparingUnitTest, get_output_format_after_configure, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000); // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, AVC_PROFILE_HIGH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, 1);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, TRANSFER_CHARACTERISTIC_LINEAR);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, COLOR_PRIMARY_BT601_625);
    int32_t ret = testObj->Configure(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Format outputFormat;
    ret = testObj->GetOutputFormat(outputFormat);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t width = 0;
    ASSERT_TRUE(outputFormat.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width));
    ASSERT_EQ(1024, width);
}

/* ============================ MULTIPLE_INSTANCE ============================ */
HWTEST_F(HEncoderPreparingUnitTest, create_multiple_instance, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj1 = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj1);
    Media::Meta meta{};
    int32_t err = testObj1->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    std::shared_ptr<HCodec> testObj2 = HCodec::Create(GetCodecName(true, "video/hevc"));
    ASSERT_TRUE(testObj2);
    err = testObj2->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    testObj1.reset();
    ASSERT_TRUE(testObj2);
}

/*========================================================*/
/*              HEncoderUserCallingUnitTest               */
/*========================================================*/
void HEncoderUserCallingUnitTest::SetUpTestCase(void)
{
}

void HEncoderUserCallingUnitTest::TearDownTestCase(void)
{
}

void HEncoderUserCallingUnitTest::SetUp(void)
{
    TLOGI("----- %s -----", ::testing::UnitTest::GetInstance()->current_test_info()->name());
}

void HEncoderUserCallingUnitTest::TearDown(void)
{
}

bool HEncoderUserCallingUnitTest::ConfigureAvcEncoder(std::shared_ptr<HCodec>& encoder)
{
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000);  // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, CBR);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue("profile", OMX_VIDEO_AVCProfileHigh);
    int32_t ret = encoder->Configure(format);
    return (ret == AVCS_ERR_OK);
}

bool HEncoderUserCallingUnitTest::ConfigureHevcEncoder(std::shared_ptr<HCodec>& encoder)
{
    Format format;
    format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1024); // 1024 width of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 768); // 768 hight of the video
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, 30.0); // 30.0 frame rate
    format.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 3000000);  // 3000000 bit rate
    format.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 10.0);  // 10.0 I-Frame interval
    format.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, VBR);
    int32_t ret = encoder->Configure(format);
    return (ret == AVCS_ERR_OK);
}

bool HEncoderUserCallingUnitTest::SetCallbackToEncoder(std::shared_ptr<HCodec>& encoder)
{
    shared_ptr<HEncoderCallback> callback = make_shared<HEncoderCallback>();
    int32_t ret = encoder->SetCallback(callback);
    return (ret == AVCS_ERR_OK);
}

/* ============================ SETTINGS ============================ */
HWTEST_F(HEncoderUserCallingUnitTest, create_input_surface_when_codec_is_running, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    sptr<Surface> p = testObj->CreateInputSurface();
    EXPECT_FALSE(p);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

/* ============================ START ============================ */
HWTEST_F(HEncoderUserCallingUnitTest, notify_eos_in_buffer_mode, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));
    int32_t ret = testObj->Start();
    ASSERT_EQ(ret, AVCS_ERR_OK);
    ret = testObj->NotifyEos();
    EXPECT_NE(ret, AVCS_ERR_OK);
    ret = testObj->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_normal, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, 1);
    ret = testObj->SetParameter(format);
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_without_setting_callback, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_NE(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_without_setting_input_surface, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_without_setting_configure, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_NE(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_stop_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Stop();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_release_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Start();
    EXPECT_NE(AVCS_ERR_OK, ret);
}

/* ============================ STOP ============================ */
HWTEST_F(HEncoderUserCallingUnitTest, stop_without_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Stop();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, stop_without_configure_and_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    int32_t ret = testObj->Stop();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

/* ============================ RELEASE ============================ */
HWTEST_F(HEncoderUserCallingUnitTest, release_without_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, release_without_configure_and_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    int32_t ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

/* ============================ FLUSH ============================ */
HWTEST_F(HEncoderUserCallingUnitTest, start_flush, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Flush();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

/* ============================ RESET ============================ */
HWTEST_F(HEncoderUserCallingUnitTest, reset_without_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Reset();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, reset_after_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Reset();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_reset_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Reset();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Start();
    EXPECT_NE(AVCS_ERR_OK, ret);

    ASSERT_TRUE(ConfigureAvcEncoder(testObj));
    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, start_reset_configure_start, TestSize.Level1)
{
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Reset();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

/* ============================ COMBO OP ============================ */
HWTEST_F(HEncoderUserCallingUnitTest, combo_op_1, TestSize.Level1)
{
    // start - stop - start - stop - start - release
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Stop();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    EXPECT_TRUE(ConfigureAvcEncoder(testObj));
    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Stop();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    EXPECT_TRUE(ConfigureAvcEncoder(testObj));
    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, combo_op_2, TestSize.Level1)
{
    // start - Reset - start - Reset - start - stop - start - release
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(SetCallbackToEncoder(testObj));
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    int32_t ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Reset();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Reset();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Stop();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    EXPECT_TRUE(ConfigureAvcEncoder(testObj));
    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}

HWTEST_F(HEncoderUserCallingUnitTest, combo_op_3, TestSize.Level1)
{
    // start - create_input_surface - start - set_callback - start - stop
    // - start - start - reset - start - release
    std::shared_ptr<HCodec> testObj = HCodec::Create(GetCodecName(true, "video/avc"));
    ASSERT_TRUE(testObj);
    Media::Meta meta{};
    int32_t err = testObj->Init(meta);
    ASSERT_TRUE(err == AVCS_ERR_OK);

    int32_t ret = testObj->Start();
    EXPECT_NE(AVCS_ERR_OK, ret);

    sptr<Surface> inputSurface = testObj->CreateInputSurface();
    ASSERT_TRUE(inputSurface);
    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    ret = testObj->Start();
    EXPECT_NE(AVCS_ERR_OK, ret);

    ASSERT_TRUE(SetCallbackToEncoder(testObj));

    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Stop();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    EXPECT_TRUE(ConfigureAvcEncoder(testObj));
    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Reset();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ASSERT_TRUE(ConfigureAvcEncoder(testObj));

    ret = testObj->Start();
    EXPECT_EQ(AVCS_ERR_OK, ret);

    ret = testObj->Release();
    EXPECT_EQ(AVCS_ERR_OK, ret);
}
}