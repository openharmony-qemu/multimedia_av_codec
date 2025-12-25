/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <gtest/hwext/gtest-multithread.h>
#include "meta/meta_key.h"
#include "unittest_utils.h"
#include "avcodec_monitor.h"
#ifdef VIDEODEC_CAPI_UNIT_TEST
#include "native_avmagic.h"
#include "videodec_capi_mock.h"
#endif
#include "videodec_func_test_suit.h"
#include "surface_type.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace VFTSUIT {
std::atomic<int32_t> g_vdecCount = 0;
std::string g_vdecName = "";

void TEST_SUIT::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    g_vdecName = capability->GetName();
}

void TEST_SUIT::CreateByNameWithParam(int32_t param)
{
    std::string codecName = "";
    switch (param) {
        case VCodecTestCode::SW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
#ifdef SUPPORT_CODEC_RV
        case VCodecTestCode::SW_RV40:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_RV40.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
#endif
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void TEST_SUIT::PrepareSource(int32_t param)
{
    std::string sourcePath = decSourcePathMap_.at(param);
    if (param == VCodecTestCode::HW_HEVC) {
        videoDec_->SetSourceType(false);
    }
    videoDec_->testParam_ = param;
    std::cout << "SourcePath: " << sourcePath << std::endl;
    videoDec_->SetSource(sourcePath);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoDec_->SetOutPath(prefix + fileName);
}

void TEST_SUIT::SetFormatWithParam(int32_t param)
{
    (void)param;
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
}

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(HW_AVC, HW_HEVC, SW_AVC));

/**
 * @tc.name: VideoDecoder_Multithread_Create_001
 * @tc.desc: try create 40 instances
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Multithread_Create_001, TestSize.Level1)
{
    auto func = []() {
        std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
        std::shared_ptr<VDecCallbackTest> adecCallback = std::make_shared<VDecCallbackTest>(vdecSignal);
        ASSERT_NE(nullptr, adecCallback);

        std::shared_ptr<VideoDecSample> videoDec = std::make_shared<VideoDecSample>(vdecSignal);
        ASSERT_NE(nullptr, videoDec);

        EXPECT_LE(g_vdecCount.load(), 40); // 40: max instances supported
        if (videoDec->CreateVideoDecMockByName(g_vdecName)) {
            g_vdecCount++;
            cout << "create successed, num:" << g_vdecCount.load() << endl;
        } else {
            cout << "create failed, num:" << g_vdecCount.load() << endl;
            return;
        }
        sleep(3); // 3: existence time
        videoDec->Release();
        g_vdecCount--;
    };
    g_vdecCount = 0;
    SET_THREAD_NUM(40); // 40: num of thread
    GTEST_RUN_TASK(func);
    cout << "remaining num: " << g_vdecCount.load() << endl;
}

/**
 * @tc.name: VideoDecoder_Multithread_Create_002
 * @tc.desc: try create 40 instances by mime
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Multithread_Create_002, TestSize.Level1)
{
    auto func = []() {
        std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
        std::shared_ptr<VideoDecSample> videoDec = std::make_shared<VideoDecSample>(vdecSignal);
        ASSERT_NE(nullptr, videoDec);
        if (videoDec->CreateVideoDecMockByMime(CodecMimeType::VIDEO_AVC.data())) {
            g_vdecCount++;
            cout << "create successed, num:" << g_vdecCount.load() << endl;
        } else {
            cout << "create failed, num:" << g_vdecCount.load() << endl;
            return;
        }
        sleep(3); // 3: existence time
        videoDec->Release();
        EXPECT_GE(g_vdecCount.load(), 25); // 25: num of instances
    };
    g_vdecCount = 0;
    SET_THREAD_NUM(40); // 40: num of thread
    GTEST_RUN_TASK(func);
    cout << "remaining num: " << g_vdecCount.load() << endl;
}

/**
 * @tc.name: VideoDecoder_Start_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Start_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: VideoDecoder_Configure_001
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());

    format_->PutIntValue("video_decoder_blank_frame_on_shutdown", 1);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: VideoDecoder_Configure_Transform_001
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Configure_Transform_001, TestSize.Level1)
{
    sptr<Surface> consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener =
        new TestConsumerListener(consumer_.GetRefPtr(), "", false);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    sptr<Surface> producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);
    surface->SetTransform(1);
    int32_t transform = -1;
    surface->GetTransform(transform);
    EXPECT_EQ(1, transform);

    CreateByNameWithParam(HW_AVC);
    SetFormatWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    surface->GetTransform(transform);
    EXPECT_EQ(0, transform);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: VideoDecoder_Configure_Transform_002
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Configure_Transform_002, TestSize.Level1)
{
    sptr<Surface> consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener =
        new TestConsumerListener(consumer_.GetRefPtr(), "", false);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    sptr<Surface> producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);

    CreateByNameWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatCfg = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, formatCfg);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    formatCfg->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 1);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(formatCfg));
    videoDec_->SetOutputSurface(surface);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    int32_t transform = -1;
    surface->GetTransform(transform);
    EXPECT_EQ(1, transform);

    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatSep = FormatMockFactory::CreateFormat();
    formatSep->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 2);
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(formatSep));
    surface->GetTransform(transform);
    EXPECT_EQ(2, transform);

    surface->SetTransform(3);
    surface->GetTransform(transform);
    EXPECT_EQ(3, transform);

    videoDec_->Release();
    surface->GetTransform(transform);
    EXPECT_EQ(0, transform);
}

/**
 * @tc.name: VideoDecoder_Configure_Transform_003
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Configure_Transform_003, TestSize.Level1)
{
    sptr<Surface> consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener =
        new TestConsumerListener(consumer_.GetRefPtr(), "", false);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    sptr<Surface> producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);
    surface->SetTransform(1);
    int32_t transform = -1;
    surface->GetTransform(transform);
    EXPECT_EQ(1, transform);

    CreateByNameWithParam(SW_AVC);
    SetFormatWithParam(SW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    surface->GetTransform(transform);
    EXPECT_EQ(0, transform);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: VideoDecoder_Configure_Transform_004
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Configure_Transform_004, TestSize.Level1)
{
    sptr<Surface> consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener =
        new TestConsumerListener(consumer_.GetRefPtr(), "", false);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    sptr<Surface> producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);

    CreateByNameWithParam(SW_AVC);
    PrepareSource(SW_AVC);
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatCfg = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, formatCfg);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    formatCfg->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 1);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(formatCfg));
    videoDec_->SetOutputSurface(surface);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    int32_t transform = -1;
    surface->GetTransform(transform);
    EXPECT_EQ(1, transform);

    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatSep = FormatMockFactory::CreateFormat();
    formatSep->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 2);
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(formatSep));
    surface->GetTransform(transform);
    EXPECT_EQ(2, transform);

    surface->SetTransform(3);
    surface->GetTransform(transform);
    EXPECT_EQ(3, transform);

    videoDec_->Release();
    surface->GetTransform(transform);
    EXPECT_EQ(0, transform);
}
#ifdef HMOS_TEST
/**
 * @tc.name: VideoDecoder_Configure_Transform_005
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Configure_Transform_005, TestSize.Level1)
{
    sptr<Surface> consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener =
        new TestConsumerListener(consumer_.GetRefPtr(), "", false);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    sptr<Surface> producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);
    surface->SetTransform(1);
    int32_t transform = -1;
    surface->GetTransform(transform);
    EXPECT_EQ(1, transform);

    CreateVideoCodecByName("OH.Media.Codec.Decoder.Video.HEVC");
    SetFormatWithParam(0);
    PrepareSource(HW_HEVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface(surface));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    surface->GetTransform(transform);
    EXPECT_EQ(0, transform);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: VideoDecoder_Configure_Transform_006
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_Configure_Transform_006, TestSize.Level1)
{
    sptr<Surface> consumer_ = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener =
        new TestConsumerListener(consumer_.GetRefPtr(), "", false);
    consumer_->RegisterConsumerListener(listener);
    auto p = consumer_->GetProducer();
    sptr<Surface> producer_ = Surface::CreateSurfaceAsProducer(p);
    std::shared_ptr<SurfaceMock> surface = SurfaceMockFactory::CreateSurface(producer_);

    CreateVideoCodecByName("OH.Media.Codec.Decoder.Video.HEVC");
    PrepareSource(HW_HEVC);
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatCfg = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, formatCfg);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    formatCfg->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 1);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(formatCfg));
    videoDec_->SetOutputSurface(surface);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    int32_t transform = -1;
    surface->GetTransform(transform);
    EXPECT_EQ(1, transform);

    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatSep = FormatMockFactory::CreateFormat();
    formatSep->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 2);
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(formatSep));
    surface->GetTransform(transform);
    EXPECT_EQ(2, transform);

    surface->SetTransform(3);
    surface->GetTransform(transform);
    EXPECT_EQ(3, transform);

    videoDec_->Release();
    surface->GetTransform(transform);
    EXPECT_EQ(0, transform);
}
#endif
/**
 * @tc.name: VideoDecoder_Configure_Transform_007
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_Transform_007, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatCfg = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, formatCfg);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    formatCfg->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, -1);
    EXPECT_NE(AV_ERR_OK, videoDec_->Configure(formatCfg));
    videoDec_->Release();
}

/**
 * @tc.name: VideoDecoder_Configure_Transform_008
 * @tc.desc: video codec Configure
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_Configure_Transform_008, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatCfg = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, formatCfg);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    formatCfg->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE,
        static_cast<int32_t>(OHOS::GraphicTransformType::GRAPHIC_ROTATE_BUTT));
    EXPECT_NE(AV_ERR_OK, videoDec_->Configure(formatCfg));
    videoDec_->Release();
}

/**
 * @tc.name: VideoDecoder_SetParameter_Transform_001
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetParameter_Transform_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatCfg = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, formatCfg);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    formatCfg->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 1);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(formatCfg));
    videoDec_->SetOutputSurface();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());

    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatSep = FormatMockFactory::CreateFormat();
    formatSep->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, -1);
    EXPECT_NE(AV_ERR_OK, videoDec_->SetParameter(formatSep));
    videoDec_->Release();
}

/**
 * @tc.name: VideoDecoder_SetParameter_Transform_002
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetParameter_Transform_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatCfg = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, formatCfg);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    formatCfg->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    formatCfg->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE, 1);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(formatCfg));
    videoDec_->SetOutputSurface();
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());

    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> formatSep = FormatMockFactory::CreateFormat();
    formatSep->PutIntValue(OHOS::Media::Tag::VIDEO_ORIENTATION_TYPE,
        static_cast<int32_t>(OHOS::GraphicTransformType::GRAPHIC_ROTATE_BUTT));
    EXPECT_NE(AV_ERR_OK, videoDec_->SetParameter(formatSep));
    videoDec_->Release();
}

/**
 * @tc.name: VideoDecoder_SetParameter_001
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetParameter_001, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_SetParameter_002
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_SetParameter_002, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -2);  // invalid width size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -2); // invalid height size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_GetOutputDescription_001
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_GetOutputDescription_001, TestSize.Level1)
{
    CreateByNameWithParam(HW_AVC);
    SetFormatWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();
    std::cout << format_->DumpInfo() << std::endl;
    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_GetOutputDescription_002
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_GetOutputDescription_002, TestSize.Level1)
{
    CreateByNameWithParam(HW_AVC);
    SetFormatWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();

    std::string dumpInfo = format_->DumpInfo();
    std::cout << "dumpInfo: [" << dumpInfo << "]\n";
    auto checkFunc = [&dumpInfo](const std::string key) {
        std::string keyStr = key + " = ";
        EXPECT_NE(dumpInfo.find(keyStr), string::npos) << "keyStr: [" << keyStr << "]\n"
                                                       << "dumpInfo: [" << dumpInfo << "]\n";
    };
    checkFunc(Media::Tag::VIDEO_CROP_TOP);
    checkFunc(Media::Tag::VIDEO_CROP_BOTTOM);
    checkFunc(Media::Tag::VIDEO_CROP_LEFT);
    checkFunc(Media::Tag::VIDEO_CROP_RIGHT);
    checkFunc(Media::Tag::VIDEO_STRIDE);

    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_GetOutputDescription_003
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_GetOutputDescription_003, TestSize.Level1)
{
    CreateByNameWithParam(HW_AVC);
    SetFormatWithParam(HW_AVC);
    PrepareSource(HW_AVC);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();

    int32_t cropRight = 0;
    int32_t stride = 0;
    int32_t cropBottom = 0;

    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_CROP_RIGHT, cropRight));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_STRIDE, stride));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_CROP_BOTTOM, cropBottom));

    EXPECT_GE(cropRight, DEFAULT_WIDTH - 1);
    EXPECT_GE(stride, DEFAULT_WIDTH);
    EXPECT_GE(cropBottom, DEFAULT_HEIGHT - 1);

    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_GetOutputDescription_004
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_GetOutputDescription_004, TestSize.Level1)
{
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();

    int32_t pictureWidth = 0;
    int32_t pictureHeight = 0;

    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_PIC_WIDTH, pictureWidth));
    EXPECT_TRUE(format_->GetIntValue(Media::Tag::VIDEO_PIC_HEIGHT, pictureHeight));

    if (GetParam() == VCodecTestCode::SW_RV40) {
        EXPECT_GE(pictureWidth, DEFAULT_RV40_WIDTH - 1);
        EXPECT_GE(pictureHeight, DEFAULT_RV40_HEIGHT - 1);
    } else {
        EXPECT_GE(pictureWidth, DEFAULT_WIDTH - 1);
        EXPECT_GE(pictureHeight, DEFAULT_HEIGHT - 1);
    }

    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoDecoder_HDR_Function_001
 * @tc.desc: video decodec hdr function test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HDR_Function_001, TestSize.Level1)
{
    capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                AVCodecCategory::AVCODEC_HARDWARE);
    std::string codecName = capability_->GetName();
    if (codecName != "OMX.rk.video_decoder.hevc") {
        std::cout << "CodecName: " << codecName << "\n";
        ASSERT_TRUE(CreateVideoCodecByName(codecName));

        format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

        VCodecTestCode param = VCodecTestCode::HW_HDR;
        std::string sourcePath = decSourcePathMap_.at(param);
        videoDec_->SetSourceType(false);
        videoDec_->testParam_ = param;
        std::cout << "SourcePath: " << sourcePath << std::endl;
        videoDec_->SetSource(sourcePath);
        const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
        string prefix = "/data/test/media/";
        string fileName = testInfo_->name();
        auto check = [](char it) { return it == '/'; };
        (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
        videoDec_->SetOutPath(prefix + fileName);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    }
}

/**
 * @tc.name: VideoDecoder_HDR_METADATA_Function_001
 * @tc.desc: video decodec hdr metadata function test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HDR_METADATA_Function_001, TestSize.Level1)
{
    capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                AVCodecCategory::AVCODEC_HARDWARE);
    std::string codecName = capability_->GetName();
    if (codecName != "OMX.rk.video_decoder.hevc") {
        std::cout << "CodecName: " << codecName << "\n";
        videoDec_->isAVBufferMode_ = true;
        ASSERT_TRUE(CreateVideoCodecByName(codecName));

        format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

        VCodecTestCode param = VCodecTestCode::HW_HDR;
        std::string sourcePath = decSourcePathMap_.at(param);
        videoDec_->SetSourceType(false);
        videoDec_->testParam_ = param;
        std::cout << "SourcePath: " << sourcePath << std::endl;
        videoDec_->SetSource(sourcePath);
        const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
        string prefix = "/data/test/media/";
        string fileName = testInfo_->name();
        auto check = [](char it) { return it == '/'; };
        (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
        videoDec_->SetOutPath(prefix + fileName);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        sleep(1);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
        EXPECT_EQ(0, vdecSignal_->errorNum);
    }
}

/**
 * @tc.name: VideoDecoder_HDR_METADATA_Function_002
 * @tc.desc: video decodec hdr metadata function test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HDR_METADATA_Function_002, TestSize.Level1)
{
    capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                AVCodecCategory::AVCODEC_HARDWARE);
    std::string codecName = capability_->GetName();
    if (codecName != "OMX.rk.video_decoder.hevc") {
        std::cout << "CodecName: " << codecName << "\n";
        videoDec_->isAVBufferMode_ = true;
        ASSERT_TRUE(CreateVideoCodecByName(codecName));

        format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1920);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 1440);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

        VCodecTestCode param = VCodecTestCode::HW_HDR_HLG_FULL;
        std::string sourcePath = decSourcePathMap_.at(param);
        videoDec_->SetSourceType(false);
        videoDec_->testParam_ = param;
        std::cout << "SourcePath: " << sourcePath << std::endl;
        videoDec_->SetSource(sourcePath);
        const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
        string prefix = "/data/test/media/";
        string fileName = testInfo_->name();
        auto check = [](char it) { return it == '/'; };
        (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
        videoDec_->SetOutPath(prefix + fileName);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        sleep(1);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
        EXPECT_EQ(0, vdecSignal_->errorNum);
    }
}

/**
 * @tc.name: VideoDecoder_HDR_METADATA_Function_003
 * @tc.desc: video decodec hdr metadata function test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_HDR_METADATA_Function_003, TestSize.Level1)
{
    capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                AVCodecCategory::AVCODEC_HARDWARE);
    std::string codecName = capability_->GetName();
    if (codecName != "OMX.rk.video_decoder.hevc") {
        std::cout << "CodecName: " << codecName << "\n";
        videoDec_->isAVBufferMode_ = true;
        ASSERT_TRUE(CreateVideoCodecByName(codecName));

        format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1600);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 900);
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

        VCodecTestCode param = VCodecTestCode::HW_HDR10;
        std::string sourcePath = decSourcePathMap_.at(param);
        videoDec_->SetSourceType(false);
        videoDec_->testParam_ = param;
        std::cout << "SourcePath: " << sourcePath << std::endl;
        videoDec_->SetSource(sourcePath);
        const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
        string prefix = "/data/test/media/";
        string fileName = testInfo_->name();
        auto check = [](char it) { return it == '/'; };
        (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
        videoDec_->SetOutPath(prefix + fileName);

        ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
        EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
        sleep(1);
        EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
        EXPECT_EQ(0, vdecSignal_->errorNum);
    }
}

/**
 * @tc.name: VideoDecoder_SetDecryptionConfig_001
 * @tc.desc: video decodec set decryption config function test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_SetDecryptionConfig_001, TestSize.Level1)
{
    VCodecTestCode param = VCodecTestCode::HW_AVC;
    CreateByNameWithParam(param);
    SetFormatWithParam(param);
    PrepareSource(param);
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetVideoDecryptionConfig());
}

/**
 * @tc.name: VideoDecoder_RenderOutputBufferAtTime_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoDecoder_RenderOutputBufferAtTime_001, TestSize.Level1)
{
    videoDec_->isAVBufferMode_ = true;
    CreateByNameWithParam(GetParam());
    SetFormatWithParam(GetParam());
    PrepareSource(GetParam());
    videoDec_->renderAtTimeFlag_ = true;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: VideoEncoder_GET_SECURE_DECODER_PIDS_001
 * @tc.desc: get secure decoder pids
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_GET_SECURE_DECODER_PIDS_001, TestSize.Level1)
{
    ASSERT_TRUE(CreateVideoCodecByName("OMX.hisi.video.decoder.avc.secure"));
    std::vector<pid_t> pidList;
    auto ret = AVCodecMonitor::GetActiveSecureDecoderPids(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    pid_t pid = getpid();
    ASSERT_TRUE(std::find(pidList.begin(), pidList.end(), pid) != pidList.end());
    videoDec_->Release();

    ret = AVCodecMonitor::GetActiveSecureDecoderPids(pidList);
    ASSERT_EQ(AVCS_ERR_OK, ret);
    ASSERT_TRUE(std::find(pidList.begin(), pidList.end(), pid) == pidList.end());
}

/**
 * @tc.name: VideoDecoder_CODEC_INFO_001
 * @tc.desc: get codec info
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoDecoder_CODEC_INFO_001, TestSize.Level1)
{
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> format = nullptr;
    int32_t isHardware = 0;
    if (CreateVideoCodecByName("OMX.hisi.video.decoder.avc")) {
        format = videoDec_->GetCodecInfo();
        if (format) {
            format->GetIntValue(Media::Tag::MEDIA_IS_HARDWARE, isHardware);
            EXPECT_EQ(isHardware, 1);
        }
        videoDec_->Release();
    }
    if (CreateVideoCodecByName("OMX.rk.video_decoder.hevc")) {
        format = videoDec_->GetCodecInfo();
        if (format) {
            format->GetIntValue(Media::Tag::MEDIA_IS_HARDWARE, isHardware);
            EXPECT_EQ(isHardware, 1);
        }
        videoDec_->Release();
    }
    if (CreateVideoCodecByName("OH.Media.Codec.Decoder.Video.AVC")) {
        format = videoDec_->GetCodecInfo();
        if (format) {
            format->GetIntValue(Media::Tag::MEDIA_IS_HARDWARE, isHardware);
            EXPECT_EQ(isHardware, 0);
        }
        videoDec_->Release();
    }
}
} // namespace

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        cout << argv[i] << endl;
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoDecSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}