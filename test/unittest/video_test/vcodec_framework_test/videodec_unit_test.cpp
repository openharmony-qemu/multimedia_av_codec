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

#include "videodec_unit_test.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;
using namespace OHOS::MediaAVCodec::VCodecTestParam;

namespace {
std::atomic<int32_t> g_vdecCount = 0;
std::string g_vdecName = "";

void MultiThreadCreateVDec()
{
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    std::shared_ptr<VDecCallbackTest> adecCallback = std::make_shared<VDecCallbackTest>(vdecSignal);
    ASSERT_NE(nullptr, adecCallback);

    std::shared_ptr<VideoDecSample> videoDec = std::make_shared<VideoDecSample>(vdecSignal);
    ASSERT_NE(nullptr, videoDec);

    EXPECT_LE(g_vdecCount.load(), 16); // 16: max instances supported
    if (videoDec->CreateVideoDecMockByName(g_vdecName)) {
        g_vdecCount++;
        cout << "create successed, num:" << g_vdecCount.load() << endl;
    } else {
        cout << "create failed, num:" << g_vdecCount.load() << endl;
        return;
    }
    sleep(1);
    videoDec->Release();
    g_vdecCount--;
}

#ifdef VIDEODEC_CAPI_UNIT_TEST
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

void VideoDecUnitTest::SetUpTestCase(void)
{
    auto capability = CodecListMockFactory::GetCapabilityByCategory((CodecMimeType::VIDEO_AVC).data(), false,
                                                                    AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capability) << (CodecMimeType::VIDEO_AVC).data() << " can not found!" << std::endl;
    g_vdecName = capability->GetName();
}

void VideoDecUnitTest::TearDownTestCase(void) {}

void VideoDecUnitTest::SetUp(void)
{
    std::shared_ptr<VDecSignal> vdecSignal = std::make_shared<VDecSignal>();
    vdecCallback_ = std::make_shared<VDecCallbackTest>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallback_);

    vdecCallbackExt_ = std::make_shared<VDecCallbackTestExt>(vdecSignal);
    ASSERT_NE(nullptr, vdecCallbackExt_);

    videoDec_ = std::make_shared<VideoDecSample>(vdecSignal);
    ASSERT_NE(nullptr, videoDec_);

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);
}

void VideoDecUnitTest::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoDec_ = nullptr;
#ifdef VIDEODEC_CAPI_UNIT_TEST
    if (codec_ != nullptr) {
        EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(codec_));
        codec_ = nullptr;
    }
#endif
}

bool VideoDecUnitTest::CreateVideoCodecByMime(const std::string &decMime)
{
    if (videoDec_->CreateVideoDecMockByMime(decMime) == false || videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool VideoDecUnitTest::CreateVideoCodecByName(const std::string &decName)
{
    if (isAVBufferMode_) {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}

void VideoDecUnitTest::CreateByNameWithParam(void)
{
    std::string codecName = "";
    switch (GetParam()) {
        case VCodecTestCode::SW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
        case VCodecTestCode::HW_AVC:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        case VCodecTestCode::HW_HEVC:
        case VCodecTestCode::HW_HDR:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_HEVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_HARDWARE);
            break;
        default:
            capability_ = CodecListMockFactory::GetCapabilityByCategory(CodecMimeType::VIDEO_AVC.data(), false,
                                                                        AVCodecCategory::AVCODEC_SOFTWARE);
            break;
    }
    codecName = capability_->GetName();
    std::cout << "CodecName: " << codecName << "\n";
    ASSERT_TRUE(CreateVideoCodecByName(codecName));
}

void VideoDecUnitTest::PrepareSource(void)
{
    VCodecTestCode param = static_cast<VCodecTestCode>(GetParam());
    std::string sourcePath = decSourcePathMap_.at(param);
    if (param == VCodecTestCode::HW_HEVC || param == VCodecTestCode::HW_HDR) {
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

void VideoDecUnitTest::SetFormatWithParam(void)
{
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
}

namespace {
INSTANTIATE_TEST_SUITE_P(, VideoDecUnitTest, testing::Values(HW_AVC, HW_HEVC, HW_HDR, SW_AVC));

/**
 * @tc.name: videoDecoder_multithread_create_001
 * @tc.desc: try create 100 instances
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_multithread_create_001, TestSize.Level1)
{
    SET_THREAD_NUM(100);
    g_vdecCount = 0;
    GTEST_RUN_TASK(MultiThreadCreateVDec);
    cout << "remaining num: " << g_vdecCount.load() << endl;
}

/**
 * @tc.name: videoDecoder_createWithNull_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_createWithNull_001, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByName(""));
}

/**
 * @tc.name: videoDecoder_createWithNull_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_createWithNull_002, TestSize.Level1)
{
    ASSERT_FALSE(CreateVideoCodecByMime(""));
}

/**
 * @tc.name: videoDecoder_setcallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_001, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
}

/**
 * @tc.name: videoDecoder_setcallback_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_002, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
}

/**
 * @tc.name: videoDecoder_setcallback_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_003, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
    ASSERT_NE(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
}

/**
 * @tc.name: videoDecoder_setcallback_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_004, TestSize.Level1)
{
    ASSERT_TRUE(videoDec_->CreateVideoDecMockByName(g_vdecName));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetCallback(vdecCallbackExt_));
    ASSERT_NE(AV_ERR_OK, videoDec_->SetCallback(vdecCallback_));
}

#ifdef VIDEODEC_CAPI_UNIT_TEST
/**
 * @tc.name: videoDecoder_setcallback_invalid_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(nullptr, cb, nullptr));
}

/**
 * @tc.name: videoDecoder_setcallback_invalid_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNeedInputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
}

/**
 * @tc.name: videoDecoder_setcallback_invalid_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNewOutputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
}

/**
 * @tc.name: videoDecoder_setcallback_invalid_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_setcallback_invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: videoDecoder_push_inputbuffer_invalid_001
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_push_inputbuffer_invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    OH_AVCodecBufferAttr attr = {0, 0, 0, 0};
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputData(codec_, 0, attr));
}

/**
 * @tc.name: videoDecoder_push_inputbuffer_invalid_002
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_push_inputbuffer_invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputBuffer(codec_, 0));
}

/**
 * @tc.name: videoDecoder_push_inputbuffer_invalid_003
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_push_inputbuffer_invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_PushInputBuffer(nullptr, 0));
}

/**
 * @tc.name: videoDecoder_push_inputbuffer_invalid_004
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_push_inputbuffer_invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_PushInputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: videoDecoder_free_buffer_invalid_001
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_free_buffer_invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_FreeOutputData(codec_, 0));
}

/**
 * @tc.name: videoDecoder_free_buffer_invalid_002
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_free_buffer_invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_FreeOutputBuffer(codec_, 0));
}

/**
 * @tc.name: videoDecoder_free_buffer_invalid_003
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_free_buffer_invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_FreeOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: videoDecoder_free_buffer_invalid_004
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_free_buffer_invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_FreeOutputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: videoDecoder_render_buffer_invalid_001
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_render_buffer_invalid_001, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_RenderOutputData(codec_, 0));
}

/**
 * @tc.name: videoDecoder_render_buffer_invalid_002
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_render_buffer_invalid_002, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(codec_, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_RenderOutputBuffer(codec_, 0));
}

/**
 * @tc.name: videoDecoder_render_buffer_invalid_003
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_render_buffer_invalid_003, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RenderOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: videoDecoder_render_buffer_invalid_004
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoDecUnitTest, videoDecoder_render_buffer_invalid_004, TestSize.Level1)
{
    codec_ = OH_VideoDecoder_CreateByMime((CodecMimeType::VIDEO_AVC).data());
    ASSERT_NE(nullptr, codec_);
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RenderOutputBuffer(codec_, 0));
    codec_->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}
#endif

/**
 * @tc.name: videoDecoder_create_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_create_001, TestSize.Level1)
{
    CreateByNameWithParam();
}

/**
 * @tc.name: videoDecoder_configure_001
 * @tc.desc: correct key input.
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_configure_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 0);     // set rotation_angle 0
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, 15000); // set max input size 15000
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
}

/**
 * @tc.name: videoDecoder_configure_002
 * @tc.desc: correct key input with redundancy key input
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_configure_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 0);     // set rotation_angle 0
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, 15000); // set max input size 15000
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_AAC_IS_ADTS, 1);        // redundancy key
    EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
}

/**
 * @tc.name: videoDecoder_configure_003
 * @tc.desc: correct key input with wrong value type input
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_configure_003, TestSize.Level1)
{
    CreateByNameWithParam();
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -2); // invalid width size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    if (GetParam() == VCodecTestCode::SW_AVC) {
        EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    } else {
        EXPECT_NE(AV_ERR_OK, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: videoDecoder_configure_004
 * @tc.desc: correct key input with wrong value type input
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_configure_004, TestSize.Level1)
{
    CreateByNameWithParam();
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -2); // invalid height size -2
    if (GetParam() == VCodecTestCode::SW_AVC) {
        EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    } else {
        EXPECT_NE(AV_ERR_OK, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: videoDecoder_configure_005
 * @tc.desc: empty format input
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_configure_005, TestSize.Level1)
{
    CreateByNameWithParam();
    if (GetParam() == VCodecTestCode::SW_AVC) {
        EXPECT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    } else {
        EXPECT_NE(AV_ERR_OK, videoDec_->Configure(format_));
    }
}

/**
 * @tc.name: videoDecoder_start_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: videoDecoder_start_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: videoDecoder_start_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: videoDecoder_start_004
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_004, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: videoDecoder_start_005
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_005, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: videoDecoder_start_buffer_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    videoDec_->needCheckSHA_ = GetParam() != VCodecTestCode::HW_HDR;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->StartBuffer());
}

/**
 * @tc.name: videoDecoder_start_buffer_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    videoDec_->needCheckSHA_ = GetParam() != VCodecTestCode::HW_HDR;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->StartBuffer());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->StartBuffer());
}

/**
 * @tc.name: videoDecoder_start_buffer_003
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_buffer_003, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    videoDec_->needCheckSHA_ = GetParam() != VCodecTestCode::HW_HDR;
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->StartBuffer());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->StartBuffer());
}

/**
 * @tc.name: videoDecoder_start_buffer_004
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_start_buffer_004, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    PrepareSource();
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_NE(AV_ERR_OK, videoDec_->StartBuffer());
}

/**
 * @tc.name: videoDecoder_stop_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_stop_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_stop_002
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_stop_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_stop_003
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_stop_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_NE(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_flush_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_flush_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Flush());
}

/**
 * @tc.name: videoDecoder_flush_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_flush_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
    EXPECT_NE(AV_ERR_OK, videoDec_->Flush());
}

/**
 * @tc.name: videoDecoder_reset_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_reset_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
}

/**
 * @tc.name: videoDecoder_reset_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_reset_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
}

/**
 * @tc.name: videoDecoder_reset_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_reset_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
}

/**
 * @tc.name: videoDecoder_release_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_release_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: videoDecoder_release_002
 * @tc.desc: correct flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_release_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: videoDecoder_release_003
 * @tc.desc: correct flow 3
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_release_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    EXPECT_EQ(AV_ERR_OK, videoDec_->Release());
}

/**
 * @tc.name: videoDecoder_setsurface_001
 * @tc.desc: video decodec setsurface
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_setsurface_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_setsurface_002
 * @tc.desc: wrong flow 1
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_setsurface_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_NE(AV_ERR_OK, videoDec_->SetOutputSurface());
}

/**
 * @tc.name: videoDecoder_setsurface_003
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_setsurface_003, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    ASSERT_NE(AV_ERR_OK, videoDec_->SetOutputSurface());
}

/**
 * @tc.name: videoDecoder_setsurface_buffer_001
 * @tc.desc: video decodec setsurface
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_setsurface_buffer_001, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    ASSERT_EQ(AV_ERR_OK, videoDec_->SetOutputSurface());
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->StartBuffer());
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_setsurface_buffer_002
 * @tc.desc: wrong flow 2
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_setsurface_buffer_002, TestSize.Level1)
{
    isAVBufferMode_ = true;
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->StartBuffer());
    ASSERT_NE(AV_ERR_OK, videoDec_->SetOutputSurface());
}

/**
 * @tc.name: videoDecoder_abnormal_001
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_abnormal_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, 20); // invalid rotation_angle 20
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, -1); // invalid max input size -1

    videoDec_->Configure(format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Reset());
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
    EXPECT_NE(AV_ERR_OK, videoDec_->Flush());
    EXPECT_NE(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_abnormal_002
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_abnormal_002, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_NE(AV_ERR_OK, videoDec_->Start());
}

/**
 * @tc.name: videoDecoder_abnormal_003
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_abnormal_003, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    EXPECT_NE(AV_ERR_OK, videoDec_->Flush());
}

/**
 * @tc.name: videoDecoder_abnormal_004
 * @tc.desc: video codec abnormal func
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_abnormal_004, TestSize.Level1)
{
    CreateByNameWithParam();
    PrepareSource();
    EXPECT_NE(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_setParameter_001
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_setParameter_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_setParameter_002
 * @tc.desc: video codec SetParameter
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_setParameter_002, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    format_ = FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    format_->PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, -2);  // invalid width size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, -2); // invalid height size -2
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUV420P));
    format_->PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);
    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    EXPECT_EQ(AV_ERR_OK, videoDec_->SetParameter(format_));
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}

/**
 * @tc.name: videoDecoder_getOutputDescription_001
 * @tc.desc: video codec GetOutputDescription
 * @tc.type: FUNC
 */
HWTEST_P(VideoDecUnitTest, videoDecoder_getOutputDescription_001, TestSize.Level1)
{
    CreateByNameWithParam();
    SetFormatWithParam();
    PrepareSource();
    ASSERT_EQ(AV_ERR_OK, videoDec_->Configure(format_));

    UNITTEST_CHECK_AND_RETURN_LOG(GetParam() != VCodecTestCode::HW_HDR, "hdr not support buffer mode");
    EXPECT_EQ(AV_ERR_OK, videoDec_->Start());
    format_ = videoDec_->GetOutputDescription();
    EXPECT_NE(nullptr, format_);
    EXPECT_EQ(AV_ERR_OK, videoDec_->Stop());
}
} // namespace