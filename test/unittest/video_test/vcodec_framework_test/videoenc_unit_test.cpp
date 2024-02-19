/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "videoenc_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
std::atomic<int32_t> g_vencCount = 0;
std::string g_vencName = "";

void MultiThreadCreateVEnc()
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    std::shared_ptr<VEncCallbackTest> vencCallback = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback);

    std::shared_ptr<VideoEncSample> videoEnc = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_LE(g_vencCount.load(), 16); // 16: max instances supported
    if (videoEnc->CreateVideoEncMockByName(g_vencName)) {
        g_vencCount++;
        cout << "create successed, num:" << g_vencCount.load() << endl;
    } else {
        cout << "create failed, num:" << g_vencCount.load() << endl;
        return;
    }
    sleep(1);
    videoEnc->Release();
    g_vencCount--;
}

#ifdef VIDEOENC_CAPI_UNIT_TEST
struct OH_AVCodecCallback GetVoidCallback()
{
    struct OH_AVCodecCallback cb;
    cb.onError = [](OH_AVCodec *codec, int32_t errorCode, void *userData) {
        (void)codec;
        (void)errorCode;
        (void)userData;
    };
    cb.onStreamChanged = [](OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
        (void)codec;
        (void)format;
        (void)userData;
    };
    cb.onNeedInputBuffer = [](OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
        (void)codec;
        (void)index;
        (void)buffer;
        (void)userData;
    };
    cb.onNewOutputBuffer = [](OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
        (void)codec;
        (void)index;
        (void)buffer;
        (void)userData;
    };
    return cb;
}
struct OH_AVCodecAsyncCallback GetVoidAsyncCallback()
{
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = [](OH_AVCodec *codec, int32_t errorCode, void *userData) {
        (void)codec;
        (void)errorCode;
        (void)userData;
    };
    cb.onStreamChanged = [](OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
        (void)codec;
        (void)format;
        (void)userData;
    };
    cb.onNeedInputData = [](OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData) {
        (void)codec;
        (void)index;
        (void)data;
        (void)userData;
    };
    cb.onNeedOutputData = [](OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                             void *userData) {
        (void)codec;
        (void)index;
        (void)data;
        (void)attr;
        (void)userData;
    };
    return cb;
}
#endif
} // namespace

void VideoEncUnitTest::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), true,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    g_vencName = capability->GetName();
}

void VideoEncUnitTest::TearDownTestCase(void) {}

void VideoEncUnitTest::SetUp(void)
{
    std::shared_ptr<VEncSignal> vencSignal = std::make_shared<VEncSignal>();
    vencCallback_ = std::make_shared<VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback_);

    vencCallbackExt_ = std::make_shared<VEncCallbackTestExt>(vencSignal);
    ASSERT_NE(nullptr, vencCallbackExt_);

    videoEnc_ = std::make_shared<VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc_);

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);
}

void VideoEncUnitTest::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoEnc_ = nullptr;
#ifdef VIDEOENC_CAPI_UNIT_TEST
    if (codec_ != nullptr) {
        EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(codec_));
        codec_ = nullptr;
    }
#endif
}

bool VideoEncUnitTest::CreateVideoCodecByMime(const std::string &encMime)
{
    if (videoEnc_->CreateVideoEncMockByMime(encMime) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool VideoEncUnitTest::CreateVideoCodecByName(const std::string &name)
{
    if (isAVBufferMode_) {
        if (videoEnc_->CreateVideoEncMockByName(name) == false ||
            videoEnc_->SetCallback(vencCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoEnc_->CreateVideoEncMockByName(name) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}

void VideoEncUnitTest::CreateByNameWithParam(void)
{
    std::string codecName = "";
    switch (GetParam()) {
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
        case VCodecTestCode::HW_HDR:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), true,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void VideoEncUnitTest::PrepareSource(void)
{
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    string prefix = "/data/test/media/";
    string fileName = testInfo_->name();
    auto check = [](char it) { return it == '/'; };
    (void)fileName.erase(std::remove_if(fileName.begin(), fileName.end(), check), fileName.end());
    videoEnc_->SetOutPath(prefix + fileName);
    videoEnc_->SetIsHdrVivid(GetParam() == HW_HDR);
}

void VideoEncUnitTest::SetFormatWithParam(void)
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
                         static_cast<int32_t>(OH_AVPixelFormat::AV_PIXEL_FORMAT_NV12));
    if (GetParam() == HW_HDR) {
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE,
                             static_cast<int32_t>(HEVCProfile::HEVC_PROFILE_MAIN_10));
        format_->PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, 100); // 100: i frame interval
    }
}

namespace {
INSTANTIATE_TEST_SUITE_P(, VideoEncUnitTest, testing::Values(HW_AVC, HW_HEVC));

/**
 * @tc.name: videoEncoder_multithread_create_001
 * @tc.desc: try create 100 instances
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_multithread_create_001, TestSize.Level1)
{
    SET_THREAD_NUM(100);
    g_vencCount = 0;
    GTEST_RUN_TASK(MultiThreadCreateVEnc);
    cout << "remaining num: " << g_vencCount.load() << endl;
}

/**
 * @tc.name: videoEncoder_createWithNull_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_createWithNull_001, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByName(""));
}

/**
 * @tc.name: videoEncoder_createWithNull_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_createWithNull_002, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByMime(""));
}

/**
 * @tc.name: videoEncoder_create_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_create_001, TestSize.Level1)
{
    ASSERT_TRUE(CreateVideoCodecByName(g_vencName));
}

/**
 * @tc.name: videoEncoder_create_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_create_002, TestSize.Level1)
{
    ASSERT_TRUE(CreateVideoCodecByMime((CodecMimeType::VIDEO_AVC).data()));
}

/**
 * @tc.name: videoEncoder_setcallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_setcallback_001, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
}

/**
 * @tc.name: videoEncoder_setcallback_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_setcallback_002, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
}

/**
 * @tc.name: videoEncoder_setcallback_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_setcallback_003, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
}

/**
 * @tc.name: videoEncoder_setcallback_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_setcallback_004, TestSize.Level1)
{
    ASSERT_TRUE(videoEnc_->CreateVideoEncMockByName(g_vencName));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->SetCallback(vencCallbackExt_));
    ASSERT_NE(AV_ERR_OK, videoEnc_->SetCallback(vencCallback_));
}

#ifdef VIDEOENC_CAPI_UNIT_TEST
/**
 * @tc.name: videoEncoder_setcallback_invalid_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_setcallback_invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_RegisterCallback(nullptr, cb, nullptr));
}

/**
 * @tc.name: videoEncoder_setcallback_invalid_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_setcallback_invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNewOutputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
}

/**
 * @tc.name: videoEncoder_setcallback_invalid_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_setcallback_invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
}

/**
 * @tc.name: videoEncoder_push_inputbuffer_invalid_001
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_push_inputbuffer_invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    OH_AVCodecBufferAttr attr = {0, 0, 0, 0};
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_PushInputData(codec_, 0, attr));
}

/**
 * @tc.name: videoEncoder_push_inputbuffer_invalid_002
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_push_inputbuffer_invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_PushInputBuffer(codec_, 0));
}

/**
 * @tc.name: videoEncoder_push_inputbuffer_invalid_003
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_push_inputbuffer_invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_PushInputBuffer(nullptr, 0));
}

/**
 * @tc.name: videoEncoder_push_inputbuffer_invalid_004
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_push_inputbuffer_invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_PushInputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
}

/**
 * @tc.name: videoEncoder_free_buffer_invalid_001
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_free_buffer_invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_FreeOutputData(codec_, 0));
}

/**
 * @tc.name: videoEncoder_free_buffer_invalid_002
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_free_buffer_invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_FreeOutputBuffer(codec_, 0));
}

/**
 * @tc.name: videoEncoder_free_buffer_invalid_003
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_free_buffer_invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_FreeOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: videoEncoder_free_buffer_invalid_004
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoEncUnitTest, videoEncoder_free_buffer_invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoEncoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_FreeOutputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
}
#endif

/**
 * @tc.name: videoEncoder_start_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: videoEncoder_start_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: videoEncoder_start_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: videoEncoder_start_004
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_004, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: videoEncoder_start_005
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_005, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: videoEncoder_start_buffer_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->StartBuffer());
}

/**
 * @tc.name: videoEncoder_start_buffer_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->StartBuffer());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->StartBuffer());
}

/**
 * @tc.name: videoEncoder_start_buffer_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_buffer_003, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->StartBuffer());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->StartBuffer());
}

/**
 * @tc.name: videoEncoder_start_buffer_004
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_start_buffer_004, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    PrepareSource();
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_NE(AV_ERR_OK, videoEnc_->StartBuffer());
}

/**
 * @tc.name: videoEncoder_stop_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_stop_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_stop_002
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_stop_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_stop_003
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_stop_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_NE(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_flush_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_flush_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Flush());
}

/**
 * @tc.name: videoEncoder_flush_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_flush_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Flush());
}

/**
 * @tc.name: videoEncoder_reset_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_reset_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
}

/**
 * @tc.name: videoEncoder_reset_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_reset_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
}

/**
 * @tc.name: videoEncoder_reset_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_reset_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
}

/**
 * @tc.name: videoEncoder_release_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_release_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**
 * @tc.name: videoEncoder_release_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_release_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**
 * @tc.name: videoEncoder_release_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_release_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoEnc_->Release());
}

/**
 * @tc.name: videoEncoder_setsurface_001
 * @tc.desc: video decodec setsurface
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_setsurface_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_setsurface_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_setsurface_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_NE(AV_ERR_OK, videoEnc_->CreateInputSurface());
}

/**
 * @tc.name: videoEncoder_setsurface_003
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_setsurface_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    ASSERT_NE(AV_ERR_OK, videoEnc_->CreateInputSurface());
}

/**
 * @tc.name: videoEncoder_setsurface_buffer_001
 * @tc.desc: video decodec setsurface
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_setsurface_buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoEnc_->CreateInputSurface());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->StartBuffer());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_setsurface_buffer_002
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_setsurface_buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->StartBuffer());
    ASSERT_NE(AV_ERR_OK, videoEnc_->CreateInputSurface());
}

/**
 * @tc.name: videoEncoder_abnormal_001
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_abnormal_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 20); // invalid rotation_angle 20
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, -1); // invalid max input size -1

    videoEnc_->Configure(format_);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Reset());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Flush());
    EXPECT_NE(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_abnormal_002
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_abnormal_002, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_NE(AV_ERR_OK, videoEnc_->Start());
}

/**
 * @tc.name: videoEncoder_abnormal_003
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_abnormal_003, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    EXPECT_NE(AV_ERR_OK, videoEnc_->Flush());
}

/**
 * @tc.name: videoEncoder_abnormal_004
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_abnormal_004, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    EXPECT_NE(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_setParameter_001
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_setParameter_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT_VENC);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_setParameter_002
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_setParameter_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -2);  // invalid width size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -2); // invalid height size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    EXPECT_EQ(AV_ERR_OK, videoEnc_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}

/**
 * @tc.name: videoEncoder_getOutputDescription_001
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncUnitTest, videoEncoder_getOutputDescription_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoEnc_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Start());
    format_ = videoEnc_->GetOutputDescription();
    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoEnc_->Stop());
}
} // namespace