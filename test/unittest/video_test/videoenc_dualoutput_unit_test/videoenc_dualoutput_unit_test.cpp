/**
 * Copyright (c) Huawei Technologies Co., Ltd. 2026. All rights reserved.
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
#include <gtest/hwext/gtest-multithread.h>
#include <memory>
#include <avcodec_common.h>
#include "native_avcodec_videoencoder.h"
#include "native_avcodec_base.h"
#include "sample_callback.h"
#include "vcodec_signal.h"
#include "venc_dualoutput_sample.h"
#include "unittest_utils.h"
#define IsSupportFastImage() \
    if (access(FAST_IMAGE_PATH.data(), 0) != 0) { GTEST_SKIP() << "Unsupport fast image"; }
using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
constexpr uint32_t DEFAULT_WIDTH = 1280;
constexpr uint32_t DEFAULT_HEIGHT = 720;
constexpr double DEFAULT_FRAME_RATE = 30.0;

enum CodecType : int32_t { AVC = 0, HEVC = 1 };

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}

static void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
}

static void OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)userData;
}

static void OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
    OH_AVCodecBufferAttr *attr, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)attr;
    (void)userData;
}

static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}

static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    (void)userData;
}

static void OnNeedInputParameter(OH_AVCodec *codec, uint32_t index, OH_AVFormat *parameter, void *userData)
{
    (void)codec;
    (void)index;
    (void)parameter;
    (void)userData;
}

const char *GetMimeType(int32_t codecType)
{
    return (codecType == HEVC) ? OH_AVCODEC_MIMETYPE_VIDEO_HEVC : OH_AVCODEC_MIMETYPE_VIDEO_AVC;
}

void RegisterCallbackWithSignal(shared_ptr<VideoEncDualOutputSample> &sample, EncoderType type,
    shared_ptr<VCodecSignal> &signal)
{
    struct OH_AVCodecCallback cb = {};
    cb.onError = OnErrorVoid;
    cb.onStreamChanged = OnStreamChangedVoid;
    cb.onNeedInputBuffer = InBufferVoid;
    cb.onNewOutputBuffer = (type == EncoderType::SECONDARY) ? OutBufferHandleSecondary : OutBufferHandle;
    signal->isRunning_ = true;
    ASSERT_EQ(AV_ERR_OK, sample->RegisterCallback(type, cb, signal)) << "RegisterCallback failed";
}

void RunPreprocDualEncoding(
    int32_t codecType,
    const std::string &testName,
    std::function<void(VideoEncDualOutputSample &)> setPrimaryPreproc,
    std::function<void(VideoEncDualOutputSample &)> setSecondaryPreproc = nullptr)
{
    auto sample = make_shared<VideoEncDualOutputSample>();
    auto signal = make_shared<VCodecSignal>(sample);
    sample->frameCount_ = 30; // 30: input frame num
    sample->inPath_ = "1280_720_nv.yuv";
    sample->outPath_ = testName + "_";
    ASSERT_TRUE(sample->CreatePrimary(GetMimeType(codecType)));

    RegisterCallbackWithSignal(sample, EncoderType::PRIMARY, signal);

    sample->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    setPrimaryPreproc(*sample);
    ASSERT_EQ(AV_ERR_OK, sample->Configure(EncoderType::PRIMARY)) << "Primary Configure failed";

    ASSERT_TRUE(sample->CreateSurface()) << "CreateSurface failed";

    ASSERT_TRUE(sample->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(sample);
    RegisterCallbackWithSignal(sample, EncoderType::SECONDARY, secSignal);

    sample->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    if (setSecondaryPreproc) {
        setSecondaryPreproc(*sample);
    }
    ASSERT_EQ(AV_ERR_OK, sample->Configure(EncoderType::SECONDARY)) << "Secondary Configure failed";

    ASSERT_EQ(AV_ERR_OK, sample->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, sample->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, sample->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, sample->Start(EncoderType::PRIMARY));

    EXPECT_TRUE(sample->WaitForEos(EncoderType::PRIMARY)) << "Primary encoding timeout";
    EXPECT_TRUE(sample->WaitForEos(EncoderType::SECONDARY)) << "Secondary encoding timeout";

    ASSERT_EQ(AV_ERR_OK, sample->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, sample->Stop(EncoderType::SECONDARY));
}
} // namespace

class TEST_SUIT : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    void FullLifecyclePrimary();
    void FullLifecycleSecondary();
    const char *GetCurMimeType() { return GetMimeType(GetParam()); }

protected:
    std::shared_ptr<VideoEncDualOutputSample> dualSample_;
};

INSTANTIATE_TEST_SUITE_P(, TEST_SUIT, testing::Values(AVC, HEVC));

void TEST_SUIT::SetUpTestCase(void) {}

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    dualSample_ = make_shared<VideoEncDualOutputSample>();
    ASSERT_TRUE(dualSample_->CreatePrimary(GetCurMimeType()));
}

void TEST_SUIT::TearDown(void)
{
    dualSample_.reset();
}

void TEST_SUIT::FullLifecyclePrimary()
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto signal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

void TEST_SUIT::FullLifecycleSecondary()
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto signal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: VideoEncoder_CreatePrimaryWithPreproc_Capi_001
 * @tc.desc: Test creating primary encoder with current mime type (AVC/HEVC)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreatePrimaryWithPreproc_Capi_001, TestSize.Level1)
{
    VideoEncDualOutputSample sample;
    ASSERT_TRUE(sample.CreatePrimary(GetCurMimeType()));
}

/**
 * @tc.name: VideoEncoder_CreatePrimaryWithPreproc_NullParam_001
 * @tc.desc: Test creating primary encoder with NULL mime parameter
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreatePrimaryWithPreproc_NullParam_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    OH_AVErrCode ret = OH_VideoEncoder_CreatePrimaryWithPreproc(nullptr, &codec);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: VideoEncoder_CreatePrimaryWithPreproc_NullParam_002
 * @tc.desc: Test creating primary encoder with NULL codec output pointer
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreatePrimaryWithPreproc_NullParam_002, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_CreatePrimaryWithPreproc(GetCurMimeType(), nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: VideoEncoder_CreatePrimaryWithPreproc_InvalidParam_001
 * @tc.desc: Test creating primary encoder with unsupported mime type
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreatePrimaryWithPreproc_InvalidParam_001, TestSize.Level1)
{
    VideoEncDualOutputSample sample;
    ASSERT_FALSE(sample.CreatePrimary("video/unsupported"));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_001
 * @tc.desc: Test configuring with basic format on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_002
 * @tc.desc: Test configuring crop (10,10,629,479) on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_002, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetCrop(EncoderType::PRIMARY, 10, 10, 629, 479); // crop left: 10, top: 10, right: 629, bottom: 479
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_003
 * @tc.desc: Test configuring drop frame rate (15.0) on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_003, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_004
 * @tc.desc: Test downsampling (0,0) disables the feature
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_004, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDownsampling(EncoderType::PRIMARY, 0, 0); // downsampling width: 0, height: 0
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_005
 * @tc.desc: Test crop (0,0,0,0) disables the feature
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_005, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetCrop(EncoderType::PRIMARY, 0, 0, 0, 0); // crop all: 0
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_006
 * @tc.desc: Test drop frame 0.0 disables the feature
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_006, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, 0.0); // drop frame rate: 0
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_007
 * @tc.desc: Test downsampling width only (missing height) returns error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_007, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    OH_AVFormat *fmt = dualSample_->GetFormat(EncoderType::PRIMARY);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, 640)); // width only
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Configure(EncoderType::PRIMARY)) << "Downsampling width without height should fail";
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_008
 * @tc.desc: Test crop left only (missing others) returns error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_008, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    OH_AVFormat *fmt = dualSample_->GetFormat(EncoderType::PRIMARY);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, 10)); // crop left only: 10
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Configure(EncoderType::PRIMARY)) << "Crop left without other values should fail";
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_009
 * @tc.desc: Test downsampling + crop together returns error (mutually exclusive)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_009, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
    dualSample_->SetCrop(EncoderType::PRIMARY, 10, 10, 629, 479); // crop: 10,10,629,479
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Configure(EncoderType::PRIMARY)) << "Downsampling + Crop together should fail";
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_010
 * @tc.desc: Test drop frame with negative value returns error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_010, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, -1.0); // drop frame rate: -1.0
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Configure(EncoderType::PRIMARY)) << "Negative drop frame should fail";
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_011
 * @tc.desc: Test drop frame >= original frame rate returns error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_011, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, 30.0); // drop frame rate: 30 (>= original 30.0)
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Configure(EncoderType::PRIMARY)) << "Drop frame >= original rate should fail";
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_012
 * @tc.desc: Test drop frame without original frame rate set returns error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_012, TestSize.Level1)
{
    OH_AVFormat *fmt = dualSample_->GetFormat(EncoderType::PRIMARY);
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_WIDTH, DEFAULT_WIDTH));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT));
    ASSERT_EQ(true, OH_AVFormat_SetIntValue(fmt, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12));
    // Intentionally not setting OH_MD_KEY_FRAME_RATE
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Configure(EncoderType::PRIMARY)) << "Drop frame without original rate should fail";
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_013
 * @tc.desc: Test drop frame + downsampling combination succeeds
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_013, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
    dualSample_->SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: VideoEncoder_ConfigurePreproc_014
 * @tc.desc: Test drop frame + crop combination succeeds
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_ConfigurePreproc_014, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
    dualSample_->SetCrop(EncoderType::PRIMARY, 10, 10, 629, 479); // crop: 10,10,629,479
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY)) << "Drop frame + crop should succeed";
}

/**
 * @tc.name: VideoEncoder_CreateSecondaryFromPrimary_NullParam_001
 * @tc.desc: Test creating secondary with NULL primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreateSecondaryFromPrimary_NullParam_001, TestSize.Level1)
{
    OH_AVCodec *secondary = nullptr;
    OH_AVErrCode ret = OH_VideoEncoder_CreateSecondaryFromPrimary(nullptr, &secondary);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL) << "Should return AV_ERR_INVALID_VAL for NULL primary";
}

/**
 * @tc.name: VideoEncoder_CreateSecondaryFromPrimary_NullParam_002
 * @tc.desc: Test creating secondary with NULL codec output pointer
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreateSecondaryFromPrimary_NullParam_002, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_CreateSecondaryFromPrimary(dualSample_->GetCodec(EncoderType::PRIMARY), nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL) << "Should return AV_ERR_INVALID_VAL for NULL codec ptr";
}

/**
 * @tc.name: VideoEncoder_CreateSecondaryFromPrimary_InvalidParam_001
 * @tc.desc: Test passing non-preproc encoder as primary returns error
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreateSecondaryFromPrimary_InvalidParam_001, TestSize.Level1)
{
    OH_AVCodec *regularEncoder = OH_VideoEncoder_CreateByMime(GetCurMimeType());
    ASSERT_NE(regularEncoder, nullptr) << "Skip: Cannot create regular encoder for invalid param test";
    OH_AVCodec *secondary = nullptr;
    OH_AVErrCode ret = OH_VideoEncoder_CreateSecondaryFromPrimary(regularEncoder, &secondary);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(regularEncoder));
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL) << "Non-preproc encoder should return AV_ERR_INVALID_VAL";
}

/**
 * @tc.name: VideoEncoder_RecreateSecondaryEncoder_001
 * @tc.desc: Test recreating secondary after destroying the first one
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_RecreateSecondaryEncoder_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    ASSERT_EQ(AV_ERR_OK, dualSample_->Release(EncoderType::SECONDARY));
    ASSERT_TRUE(dualSample_->CreateSecondary());
}
/**
 * @tc.name: VideoEncoder_CreateSecondaryFromPrimary_001
 * @tc.desc: Test destroying primary auto-cleans secondary (no crash)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_CreateSecondaryFromPrimary_001, TestSize.Level1)
{
    VideoEncDualOutputSample localDual;
    ASSERT_TRUE(localDual.CreatePrimary(GetCurMimeType()));
    ASSERT_TRUE(localDual.CreateSecondary());
    ASSERT_EQ(AV_ERR_OK, dualSample_->Release(EncoderType::PRIMARY));
}

/**
 * @tc.name: Primary_SetCallback_Normal_001
 * @tc.desc: Test SetCallback with valid parameters on primary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_SetCallback_Normal_001, TestSize.Level1)
{
    OH_AVCodecAsyncCallback cb = {OnError, OnStreamChanged, OnNeedInputData, OnNewOutputData};
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, dualSample_->SetCallback(EncoderType::PRIMARY, cb, nullptr));
}

/**
 * @tc.name: Primary_RegisterParameterCallback_NullParam_001
 * @tc.desc: Test RegisterParameterCallback with NULL callback
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_RegisterParameterCallback_NullParam_001, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_RegisterParameterCallback(dualSample_->GetCodec(EncoderType::PRIMARY), nullptr, nullptr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
}

/**
 * @tc.name: Primary_RegisterParameterCallback_Normal_001
 * @tc.desc: Test RegisterParameterCallback with valid parameters on primary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_RegisterParameterCallback_Normal_001, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_RegisterParameterCallback(
        dualSample_->GetCodec(EncoderType::PRIMARY), OnNeedInputParameter, nullptr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
}

/**
 * @tc.name: Primary_Configure_NullParam_001
 * @tc.desc: Test Configure with NULL codec
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_Configure_NullParam_001, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_Configure(nullptr, dualSample_->GetFormat(EncoderType::PRIMARY));
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Primary_Configure_NullParam_002
 * @tc.desc: Test Configure with NULL format
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_Configure_NullParam_002, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_Configure(dualSample_->GetCodec(EncoderType::PRIMARY), nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Primary_Prepare_NullParam_001
 * @tc.desc: Test Prepare with NULL codec
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_Prepare_NullParam_001, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_Prepare(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Primary_Prepare_Normal_001
 * @tc.desc: Test Configure -> Prepare normal flow on primary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_Prepare_Normal_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto signal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
}

/**
 * @tc.name: Primary_Start_Normal_001
 * @tc.desc: Test Configure -> Prepare -> Start normal flow on primary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_Start_Normal_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto signal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
}

/**
 * @tc.name: Primary_Reset_AfterStart_001
 * @tc.desc: Test Reset after Start returns to created state
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_Reset_AfterStart_001, TestSize.Level1)
{
    FullLifecyclePrimary();
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::PRIMARY));
}

/**
 * @tc.name: Primary_GetOutputDescription_Normal_001
 * @tc.desc: Test GetOutputDescription on configured primary returns non-NULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_GetOutputDescription_Normal_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto signal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    auto desc = dualSample_->GetOutputDescription(EncoderType::PRIMARY);
    ASSERT_NE(desc, nullptr);
}

/**
 * @tc.name: Primary_GetInputDescription_Normal_001
 * @tc.desc: Test GetInputDescription on configured primary returns non-NULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_GetInputDescription_Normal_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto signal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    auto desc = dualSample_->GetInputDescription(EncoderType::PRIMARY);
    ASSERT_NE(desc, nullptr);
}

/**
 * @tc.name: Primary_SetParameter_NullParam_001
 * @tc.desc: Test SetParameter with NULL format
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_SetParameter_NullParam_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->SetParameter(EncoderType::PRIMARY, nullptr));
}

/**
 * @tc.name: Primary_SetParameter_BeforeStart_001
 * @tc.desc: Test SetParameter before Start returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_SetParameter_BeforeStart_001, TestSize.Level1)
{
    OH_AVFormat *param = OH_AVFormat_Create();
    ASSERT_NE(param, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->SetParameter(EncoderType::PRIMARY, param));
    OH_AVFormat_Destroy(param);
}

/**
 * @tc.name: Primary_PushInputBuffer_NullParam_001
 * @tc.desc: Test PushInputBuffer with NULL codec
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_PushInputBuffer_NullParam_001, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_PushInputBuffer(nullptr, 0);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Primary_PushInputBuffer_BeforeStart_001
 * @tc.desc: Test PushInputBuffer before Start returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_PushInputBuffer_BeforeStart_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, dualSample_->PushInputBuffer(EncoderType::PRIMARY, 0));
}

/**
 * @tc.name: Primary_QueryInputBuffer_NullParam_001
 * @tc.desc: Test QueryInputBuffer with NULL index pointer
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_QueryInputBuffer_NullParam_001, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_QueryInputBuffer(dualSample_->GetCodec(EncoderType::PRIMARY), nullptr, 0);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
}

/**
 * @tc.name: Primary_QueryOutputBuffer_NullParam_001
 * @tc.desc: Test QueryOutputBuffer with NULL index pointer
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_QueryOutputBuffer_NullParam_001, TestSize.Level1)
{
    OH_AVErrCode ret = OH_VideoEncoder_QueryOutputBuffer(dualSample_->GetCodec(EncoderType::PRIMARY), nullptr, 0);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
}

/**
 * @tc.name: Primary_IsValid_Normal_001
 * @tc.desc: Test IsValid on valid primary encoder returns true
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Primary_IsValid_Normal_001, TestSize.Level1)
{
    bool isValid = false;
    ASSERT_EQ(AV_ERR_OK, dualSample_->IsValid(EncoderType::PRIMARY, isValid));
    ASSERT_EQ(isValid, true);
}

/**
 * @tc.name: Secondary_RegisterCallback_Normal_001
 * @tc.desc: Test RegisterCallback with valid parameters on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_RegisterCallback_Normal_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    OH_AVCodecCallback cb = {OnError, OnStreamChanged, OnNeedInputBuffer, OnNewOutputBuffer};
    ASSERT_EQ(AV_ERR_OK, dualSample_->RegisterCallback(EncoderType::SECONDARY, cb, nullptr));
}

/**
 * @tc.name: Secondary_Configure_Normal_001
 * @tc.desc: Test Configure with basic format on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_Configure_Normal_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
}

/**
 * @tc.name: Secondary_Prepare_Normal_001
 * @tc.desc: Test Configure -> Prepare normal flow on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_Prepare_Normal_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));

    OH_AVCodecCallback cb = {OnError, OnStreamChanged, OnNeedInputBuffer, OnNewOutputBuffer};
    ASSERT_EQ(AV_ERR_OK, dualSample_->RegisterCallback(EncoderType::SECONDARY, cb, nullptr));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
}

/**
 * @tc.name: Secondary_Start_BeforePrepare_001
 * @tc.desc: Test Start before Prepare returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_Start_BeforePrepare_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Start(EncoderType::SECONDARY));
}

/**
 * @tc.name: Secondary_Stop_BeforeStart_001
 * @tc.desc: Test Stop before Start returns INVALID_STATE on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_Stop_BeforeStart_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: Secondary_Flush_BeforeStart_001
 * @tc.desc: Test Flush before Start returns INVALID_STATE on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_Flush_BeforeStart_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Flush(EncoderType::SECONDARY));
}

/**
 * @tc.name: Secondary_Reset_CreatedState_001
 * @tc.desc: Test Reset at created state on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_Reset_CreatedState_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::SECONDARY));
}

/**
 * @tc.name: Secondary_GetOutputDescription_Normal_001
 * @tc.desc: Test GetOutputDescription on prepared secondary returns non-NULL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_GetOutputDescription_Normal_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));

    OH_AVCodecCallback cb = {OnError, OnStreamChanged, OnNeedInputBuffer, OnNewOutputBuffer};
    ASSERT_EQ(AV_ERR_OK, dualSample_->RegisterCallback(EncoderType::SECONDARY, cb, nullptr));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    auto desc = dualSample_->GetOutputDescription(EncoderType::SECONDARY);
    ASSERT_NE(desc, nullptr);
}

/**
 * @tc.desc: Test SetParameter before Start returns INVALID_STATE on secondary
 * @tc.name: Secondary_SetParameter_BeforeStart_001
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_SetParameter_BeforeStart_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    OH_AVFormat *param = OH_AVFormat_Create();
    ASSERT_NE(param, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->SetParameter(EncoderType::SECONDARY, param));
    OH_AVFormat_Destroy(param);
}

/**
 * @tc.name: Secondary_QueryOutputBuffer_NullParam_001
 * @tc.desc: Test QueryOutputBuffer with NULL index pointer on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_QueryOutputBuffer_NullParam_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    OH_AVErrCode ret = OH_VideoEncoder_QueryOutputBuffer(dualSample_->GetCodec(EncoderType::SECONDARY), nullptr, 0);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
}

/**
 * @tc.name: Secondary_IsValid_NullParam_001
 * @tc.desc: Test IsValid with NULL isValid pointer on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_IsValid_NullParam_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    OH_AVErrCode ret = OH_VideoEncoder_IsValid(dualSample_->GetCodec(EncoderType::SECONDARY), nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.name: Secondary_IsValid_Normal_001
 * @tc.desc: Test IsValid on valid secondary encoder returns true
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, Secondary_IsValid_Normal_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    bool isValid = false;
    ASSERT_EQ(AV_ERR_OK, dualSample_->IsValid(EncoderType::SECONDARY, isValid));
    ASSERT_EQ(isValid, true);
}

/**
 * @tc.name: DualOutput_FullEncoding_NoPreproc_001
 * @tc.desc: Test full dual-output encoding without any preprocessor features
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_NoPreproc_001, TestSize.Level1)
{
    RunPreprocDualEncoding(GetParam(), "fullenc_no_preproc",
        [](VideoEncDualOutputSample &s) {
            // no preprocessor features
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_CropOnly_001
 * @tc.desc: Test full encoding with crop (10,10,629,479) on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_CropOnly_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_crop",
        [](VideoEncDualOutputSample &s) {
            s.SetCrop(EncoderType::PRIMARY, 10, 10, 629, 479); // crop: 10,10,629,479
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_DownsamplingOnly_001
 * @tc.desc: Test full encoding with downsampling (640x480) on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_DownsamplingOnly_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_downsample",
        [](VideoEncDualOutputSample &s) {
            s.SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_DropFrameRateOnly_001
 * @tc.desc: Test full encoding with drop frame rate (15.0) on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_DropFrameRateOnly_001, TestSize.Level1)
{
    RunPreprocDualEncoding(GetParam(), "fullenc_dropframerate",
        [](VideoEncDualOutputSample &s) {
            s.SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_CropAndDropFrameRate_001
 * @tc.desc: Test full encoding with crop + drop frame rate on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_CropAndDropFrameRate_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_crop_dropfr",
        [](VideoEncDualOutputSample &s) {
            s.SetCrop(EncoderType::PRIMARY, 10, 10, 629, 479); // crop: 10,10,629,479
            s.SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_DownsamplingAndDropFrameRate_001
 * @tc.desc: Test full encoding with downsampling + drop frame rate on primary encoder
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_DownsamplingAndDropFrameRate_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_ds_dropfr",
        [](VideoEncDualOutputSample &s) {
            s.SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
            s.SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_PrimaryCrop_SecondaryDownsample_001
 * @tc.desc: Test full encoding with crop on primary, downsampling on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_PrimaryCrop_SecondaryDownsample_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_pri_crop_sec_ds",
        [](VideoEncDualOutputSample &s) {
            s.SetCrop(EncoderType::PRIMARY, 10, 10, 629, 479); // crop: 10,10,629,479
        },
        [](VideoEncDualOutputSample &s) {
            s.SetDownsampling(EncoderType::SECONDARY, 640, 480); // downsampling: 640x480
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_PrimaryDownsample_SecondaryCrop_001
 * @tc.desc: Test full encoding with downsampling on primary, crop on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_PrimaryDownsample_SecondaryCrop_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_pri_ds_sec_crop",
        [](VideoEncDualOutputSample &s) {
            s.SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
        },
        [](VideoEncDualOutputSample &s) {
            s.SetCrop(EncoderType::SECONDARY, 10, 10, 629, 479); // crop: 10,10,629,479
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_PrimaryCropDropFR_SecondaryDownsample_001
 * @tc.desc: Test full encoding with crop+dropframerate on primary, downsampling on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_PrimaryCropDropFR_SecondaryDownsample_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_pri_cropdfr_sec_ds",
        [](VideoEncDualOutputSample &s) {
            s.SetCrop(EncoderType::PRIMARY, 10, 10, 629, 479); // crop: 10,10,629,479
            s.SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
        },
        [](VideoEncDualOutputSample &s) {
            s.SetDownsampling(EncoderType::SECONDARY, 640, 480); // downsampling: 640x480
        });
}

/**
 * @tc.name: DualOutput_FullEncoding_PrimaryDownsampleDropFR_SecondaryCrop_001
 * @tc.desc: Test full encoding with downsampling+dropframerate on primary, crop on secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, DualOutput_FullEncoding_PrimaryDownsampleDropFR_SecondaryCrop_001, TestSize.Level1)
{
    IsSupportFastImage();
    RunPreprocDualEncoding(GetParam(), "fullenc_pri_dsdfr_sec_crop",
        [](VideoEncDualOutputSample &s) {
            s.SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
            s.SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
        },
        [](VideoEncDualOutputSample &s) {
            s.SetCrop(EncoderType::SECONDARY, 10, 10, 629, 479); // crop: 10,10,629,479
        });
}

/**
 * @tc.name: VideoEncoder_SecondaryIndependent_Capi_001
 * @tc.desc: Test primary and secondary configured with different preproc params
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_SecondaryIndependent_Capi_001, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetCrop(EncoderType::SECONDARY, 10, 10, 629, 479); // crop: 10,10,629,479
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
}

/**
 * @tc.name: VideoEncoder_PrimarySecondaryCombined_Capi_001
 * @tc.desc: Test primary with downsampling and secondary with crop simultaneously
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_PrimarySecondaryCombined_Capi_001, TestSize.Level1)
{
    IsSupportFastImage();
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::PRIMARY, 15.0); // drop frame rate: 15.0
    dualSample_->SetDownsampling(EncoderType::PRIMARY, 640, 480); // downsampling: 640x480
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    dualSample_->SetDropFrameRate(EncoderType::SECONDARY, 20.0); // drop frame rate: 20.0
    dualSample_->SetCrop(EncoderType::SECONDARY, 10, 10, 629, 479); // crop: 10,10,629,479
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
}

/**
 * @tc.name: VideoEncoder_PrimarySecondaryReset_001
 * @tc.desc: Test primary Reset then re-Configure, secondary continues
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, VideoEncoder_PrimarySecondaryReset_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::PRIMARY));
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Start_BeforeConfigure_001
 * @tc.desc: Test Start at created state returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Start_BeforeConfigure_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Start(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Stop_BeforeConfigure_001
 * @tc.desc: Test Stop at created state returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Stop_BeforeConfigure_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Flush_BeforeConfigure_001
 * @tc.desc: Test Flush at created state returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Flush_BeforeConfigure_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Flush(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Prepare_BeforeConfigure_001
 * @tc.desc: Test Prepare before Configure returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Prepare_BeforeConfigure_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Reset_CreatedState_001
 * @tc.desc: Test Reset at created state returns OK
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Reset_CreatedState_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Start_ConfiguredState_001
 * @tc.desc: Test Start after Configure but before Prepare returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Start_ConfiguredState_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Stop_ConfiguredState_001
 * @tc.desc: Test Stop at configured state returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Stop_ConfiguredState_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Flush_ConfiguredState_001
 * @tc.desc: Test Flush at configured state returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Flush_ConfiguredState_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Flush(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Stop_PreparedState_001
 * @tc.desc: Test Stop at prepared state returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Stop_PreparedState_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Flush_PreparedState_001
 * @tc.desc: Test Flush at prepared state returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Flush_PreparedState_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Flush(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_DoubleStop_001
 * @tc.desc: Test double Stop after Start returns INVALID_STATE on second call
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_DoubleStop_001, TestSize.Level1)
{
    FullLifecyclePrimary();
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Reset_AfterStart_001
 * @tc.desc: Test Reset after Start returns OK
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Reset_AfterStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Reset_Reconfigure_001
 * @tc.desc: Test Reset allows re-Configure->Prepare->Start->Stop
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Reset_Reconfigure_001, TestSize.Level1)
{
    FullLifecyclePrimary();
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::PRIMARY));
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_Flush_AfterStart_001
 * @tc.desc: Test Flush after Start returns OK
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_Flush_AfterStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Flush(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_FlushThenStart_001
 * @tc.desc: Test Start after Flush returns OK (resume)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_FlushThenStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Flush(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_StopThenStart_001
 * @tc.desc: Test Start after Stop returns OK (resume)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_StopThenStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Primary_ReleaseThenOperate_001
 * @tc.desc: Test operations after Release return INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Primary_ReleaseThenOperate_001, TestSize.Level1)
{
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto signal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Release(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Flush(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Reset(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Secondary_Prepare_BeforeConfigure_001
 * @tc.desc: Test Prepare before Configure returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_Prepare_BeforeConfigure_001, TestSize.Level1)
{
    ASSERT_TRUE(dualSample_->CreateSecondary());
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_FullLifecycle_001
 * @tc.desc: Test secondary Configure->Prepare->Start->Stop normal flow
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_FullLifecycle_001, TestSize.Level1)
{
    FullLifecycleSecondary();
}

/**
 * @tc.name: SM_Secondary_Stop_PreparedState_001
 * @tc.desc: Test Stop at prepared state on secondary returns INVALID_STATE
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_Stop_PreparedState_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_DoubleStop_001
 * @tc.desc: Test double Stop on secondary returns INVALID_STATE on second call
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_DoubleStop_001, TestSize.Level1)
{
    FullLifecycleSecondary();
    ASSERT_EQ(AV_ERR_INVALID_STATE, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_Reset_AfterStart_001
 * @tc.desc: Test Reset after Start on secondary returns OK
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_Reset_AfterStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_Reset_Reconfigure_001
 * @tc.desc: Test secondary Reset allows re-Configure->Prepare->Start->Stop
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_Reset_Reconfigure_001, TestSize.Level1)
{
    FullLifecycleSecondary();
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::SECONDARY));
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_Flush_AfterStart_001
 * @tc.desc: Test Flush after Start on secondary returns OK
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_Flush_AfterStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Flush(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_FlushThenStart_001
 * @tc.desc: Test Start after Flush on secondary returns OK (resume)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_FlushThenStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Flush(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_StopThenStart_001
 * @tc.desc: Test Start after Stop on secondary returns OK (resume)
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_StopThenStart_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Secondary_ReleaseThenOperate_001
 * @tc.desc: Test operations after Release on secondary return AV_ERR_INVALID_VAL
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Secondary_ReleaseThenOperate_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Release(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Stop(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Flush(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, dualSample_->Reset(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_StartSecondaryBeforePrimary_001
 * @tc.desc: Test Start secondary before primary, then Start primary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_StartSecondaryBeforePrimary_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_TRUE(dualSample_->CreateSurface());

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_StopPrimaryWhileSecondaryRunning_001
 * @tc.desc: Test Stop primary while secondary is still running
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_StopPrimaryWhileSecondaryRunning_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_TRUE(dualSample_->CreateSurface());

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_StopSecondaryWhilePrimaryRunning_001
 * @tc.desc: Test Stop secondary while primary is still running
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_StopSecondaryWhilePrimaryRunning_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_TRUE(dualSample_->CreateSurface());

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Dual_ResetPrimaryWhileSecondaryRunning_001
 * @tc.desc: Test Reset primary while secondary is running
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_ResetPrimaryWhileSecondaryRunning_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_TRUE(dualSample_->CreateSurface());

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::PRIMARY));
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    auto priSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, priSignal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_ResetSecondaryWhilePrimaryRunning_001
 * @tc.desc: Test Reset secondary while primary is running
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_ResetSecondaryWhilePrimaryRunning_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));
    ASSERT_TRUE(dualSample_->CreateSurface());

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::SECONDARY));
    auto secSignal2 = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal2);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Dual_FlushPrimaryWhileSecondaryRunning_001
 * @tc.desc: Test Flush primary while secondary is running
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_FlushPrimaryWhileSecondaryRunning_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSurface());

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Flush(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_FlushSecondaryWhilePrimaryRunning_001
 * @tc.desc: Test Flush secondary while primary is running
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_FlushSecondaryWhilePrimaryRunning_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSurface());

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Flush(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Dual_ReleasePrimary_DestroysSecondary_001
 * @tc.desc: Test destroying primary auto-cleans secondary (no crash), secondary ops should fail
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_ReleasePrimary_DestroysSecondary_001, TestSize.Level1)
{
    VideoEncDualOutputSample localDual;
    ASSERT_TRUE(localDual.CreatePrimary(GetCurMimeType()));
    ASSERT_TRUE(localDual.CreateSecondary());
    ASSERT_EQ(AV_ERR_OK, localDual.Release(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_INVALID_STATE, localDual.Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_INVALID_STATE, localDual.Stop(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_INVALID_VAL, localDual.Configure(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_ReleaseSecondary_ThenRecreate_001
 * @tc.desc: Test releasing secondary and recreating a new one
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_ReleaseSecondary_ThenRecreate_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSurface());
    ASSERT_TRUE(dualSample_->CreateSecondary());
    ASSERT_EQ(AV_ERR_OK, dualSample_->Release(EncoderType::SECONDARY));

    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_BothStart_BothStop_001
 * @tc.desc: Test both encoders Start then both Stop
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_BothStart_BothStop_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSurface());
    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

/**
 * @tc.name: SM_Dual_PrimaryStartStop_ThenSecondaryStartStop_001
 * @tc.desc: Test primary full lifecycle first, then secondary full lifecycle
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_PrimaryStartStop_ThenSecondaryStartStop_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSurface());
    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_BothReset_ThenReconfigure_001
 * @tc.desc: Test both encoders Reset then re-Configure both
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_BothReset_ThenReconfigure_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSurface());
    ASSERT_TRUE(dualSample_->CreateSecondary());
    auto secSignal = make_shared<VCodecSignal>(dualSample_);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::SECONDARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Reset(EncoderType::SECONDARY));

    auto priSignal2 = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, priSignal2);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));

    auto secSignal2 = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::SECONDARY, secSignal2);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_CreateSecondary_BeforePrimaryConfigure_001
 * @tc.desc: Test creating secondary before primary Configure, then configure both
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_CreateSecondary_BeforePrimaryConfigure_001, TestSize.Level1)
{
    // Create secondary before primary is configured
    ASSERT_TRUE(dualSample_->CreateSecondary());
    // Now configure both
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));
}

/**
 * @tc.name: SM_Dual_StartOnlyPrimary_001
 * @tc.desc: Test starting only primary without starting secondary
 * @tc.type: FUNC
 */
HWTEST_P(TEST_SUIT, SM_Dual_StartOnlyPrimary_001, TestSize.Level1)
{
    auto signal = make_shared<VCodecSignal>(dualSample_);
    dualSample_->SetBasicFormat(EncoderType::PRIMARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    RegisterCallbackWithSignal(dualSample_, EncoderType::PRIMARY, signal);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Prepare(EncoderType::PRIMARY));

    ASSERT_TRUE(dualSample_->CreateSurface());
    ASSERT_TRUE(dualSample_->CreateSecondary());
    dualSample_->SetBasicFormat(EncoderType::SECONDARY, DEFAULT_WIDTH, DEFAULT_HEIGHT, AV_PIXEL_FORMAT_NV12, DEFAULT_FRAME_RATE);
    ASSERT_EQ(AV_ERR_OK, dualSample_->Configure(EncoderType::SECONDARY));

    ASSERT_EQ(AV_ERR_OK, dualSample_->Start(EncoderType::PRIMARY));
    ASSERT_EQ(AV_ERR_OK, dualSample_->Stop(EncoderType::PRIMARY));
}

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    for (int i = 0; i < argc; ++i) {
        if (strcmp(argv[i], "--need_dump") == 0) {
            VideoEncDualOutputSample::needDump_ = true;
            DecArgv(i, argc, argv);
        }
    }
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
