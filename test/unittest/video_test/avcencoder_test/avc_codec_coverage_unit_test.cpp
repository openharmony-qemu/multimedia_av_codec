/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <iostream>
#include <gtest/gtest.h>
#include "avc_encoder.h"
#include "avc_encoder_util.h"
#include "surface/window.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;
using namespace testing;
using namespace testing::ext;

namespace {
constexpr int32_t VIDEO_MAX_WIDTH_SIZE = 2560;
constexpr int32_t VIDEO_MAX_HEIGHT_SIZE = 2560;
constexpr int32_t DEFAULT_VIDEO_WIDTH = 1920;
constexpr int32_t DEFAULT_VIDEO_HEIGHT = 1080;
constexpr int32_t VIDEO_BITRATE_MAX_SIZE = 240000000;
constexpr int32_t DEFAULT_VIDEO_BITRATE = 20000000;
constexpr int32_t VIDEO_FRAMERATE_MAX_SIZE = 60;
constexpr int32_t DEFAULT_VIDEO_FRAMERATE = 30;
constexpr double DEFAULT_VIDEO_FRAMERATE_DOUBLE = 30.0;
constexpr int32_t VIDEO_QUALITY_DEFAULT = 80;
constexpr int32_t VIDEO_QP_MAX = 51;
constexpr int32_t VIDEO_QP_MIN = 4;
constexpr int32_t VIDEO_IFRAME_INTERVAL_MIN_TIME = 1000;
constexpr int32_t VIDEO_IFRAME_INTERVAL_MAX_TIME = 3600000;
constexpr int32_t MOCK_ENCODE_TIEMSTAMPS = 0;
constexpr int32_t MOCK_ENCODE_BYTES = 100;
constexpr int32_t MOCK_ENCODE_FRAMETYPE = 3;
constexpr int32_t FAKE_PTR = 0x1234;
constexpr int32_t RGBA_BUFFER_SIZE = 4;
constexpr int32_t YUV_BUFFER_SIZE = 3;
constexpr int32_t CONVERT_WIDTH = 10;
constexpr int32_t CONVERT_HEIGHT = 10;


class AvcCodecCallback : public MediaCodecCallback {
public:
    explicit AvcCodecCallback(std::shared_ptr<Codec::AvcEncoder> codec)
        : encoder_(codec)
    {
    }

    ~AvcCodecCallback() override = default;

    void OnError(AVCodecErrorType errorType, int32_t errorCode) override
    {
        cout << "AvcCodecCallback OnError" << endl;
    }

    void OnOutputFormatChanged(const Format &format) override
    {
        cout << "AvcCodecCallback OnOutputFormatChanged" << endl;
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<OHOS::Media::AVBuffer> buffer) override
    {
        if (auto codec = encoder_.lock()) {
            if (codec->state_ == Codec::AvcEncoder::State::CONFIGURED) {
                codec->state_ = Codec::AvcEncoder::State::RUNNING;
            }
            if (buffer->memory_ != nullptr) {
                buffer->memory_->SetSize(DEFAULT_VIDEO_WIDTH * DEFAULT_VIDEO_HEIGHT * RGBA_BUFFER_SIZE);
            }

            buffer->meta_ = std::make_shared<Meta>();
            if (setDiscardBufferFlag) {
                buffer->meta_->SetData(OHOS::Media::Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, true);
            }
            codec->QueueInputBuffer(index);
        } else {
            cout << "AvcCodecCallback OnInputBufferAvailable null encoder" << endl;
        }
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<OHOS::Media::AVBuffer> buffer) override
    {
        if (auto codec = encoder_.lock()) {
            codec->ReleaseOutputBuffer(index);
        } else {
            cout << "AvcCodecCallback OnOutputBufferAvailable null encoder" << endl;
        }
    }
public:
    bool setDiscardBufferFlag = false;
private:
    std::weak_ptr<Codec::AvcEncoder> encoder_;
};

static uint32_t EncoderCreate(AVC_ENC_HANDLE *phEncoder, AVC_ENC_INIT_PARAM *pstInitParam)
{
    *phEncoder = reinterpret_cast<AVC_ENC_HANDLE>(FAKE_PTR);
    std::string str = "EncoderCreate";
    pstInitParam->logFxn(0, IHW264VIDEO_ALG_LOG_LEVEL::IHW264VIDEO_ALG_LOG_DEBUG, (uint8_t *)(str.c_str()));
    pstInitParam->logFxn(0, IHW264VIDEO_ALG_LOG_LEVEL::IHW264VIDEO_ALG_LOG_INFO, (uint8_t *)(str.c_str()));
    return 0;
}

static uint32_t EncoderProcess(AVC_ENC_HANDLE phEncoder, AVC_ENC_INARGS *pstInArgs, AVC_ENC_OUTARGS *pstOutArgs)
{
    (void)phEncoder;
    (void)pstInArgs;
    pstOutArgs->bytes = MOCK_ENCODE_BYTES;
    pstOutArgs->timestamp = MOCK_ENCODE_TIEMSTAMPS;
    pstOutArgs->encodedFrameType = MOCK_ENCODE_FRAMETYPE;
    return 0;
}

static uint32_t EncoderDelete(AVC_ENC_HANDLE phEncoder)
{
    (void)phEncoder;
    return 0;
}

static uint32_t EncoderCreateError(AVC_ENC_HANDLE *phEncoder, AVC_ENC_INIT_PARAM *pstInitParam)
{
    *phEncoder = reinterpret_cast<AVC_ENC_HANDLE>(FAKE_PTR);
    std::string str = "EncoderCreate";
    pstInitParam->logFxn(0, IHW264VIDEO_ALG_LOG_LEVEL::IHW264VIDEO_ALG_LOG_WARNING, (uint8_t *)(str.c_str()));
    pstInitParam->logFxn(0, IHW264VIDEO_ALG_LOG_LEVEL::IHW264VIDEO_ALG_LOG_ERROR, (uint8_t *)(str.c_str()));
    return 1;
}

static uint32_t EncoderProcessError(AVC_ENC_HANDLE phEncoder, AVC_ENC_INARGS *pstInArgs, AVC_ENC_OUTARGS *pstOutArgs)
{
    (void)phEncoder;
    (void)pstInArgs;
    pstOutArgs->bytes = MOCK_ENCODE_BYTES;
    pstOutArgs->timestamp = MOCK_ENCODE_TIEMSTAMPS;
    pstOutArgs->encodedFrameType = MOCK_ENCODE_FRAMETYPE;
    return 1;
}

static uint32_t EncoderDeleteError(AVC_ENC_HANDLE phEncoder)
{
    (void)phEncoder;
    return 1;
}

class AvcCodecCoverageUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    std::shared_ptr<Codec::AvcEncoder> avcEncoder_ = nullptr;
    std::shared_ptr<AvcCodecCallback> avcCallback_ = nullptr;
private:
    OHNativeWindow* CreateWindow(sptr<Surface> surface);
    void FlushWindow(OHNativeWindow* nativeWindow);
    Format format_;
};

void AvcCodecCoverageUnitTest::SetUpTestCase(void)
{}

void AvcCodecCoverageUnitTest::TearDownTestCase(void)
{}

void AvcCodecCoverageUnitTest::SetUp(void)
{
    avcEncoder_ = std::make_shared<Codec::AvcEncoder>("OH.Media.Codec.Encoder.Video.AVC");
    EXPECT_NE(avcEncoder_, nullptr);
    avcCallback_ = std::make_shared<AvcCodecCallback>(avcEncoder_);
    EXPECT_NE(avcCallback_, nullptr);
    avcEncoder_->SetCallback(avcCallback_);
}

void AvcCodecCoverageUnitTest::TearDown(void)
{
    avcEncoder_ = nullptr;
    avcCallback_ = nullptr;
}

OHNativeWindow* AvcCodecCoverageUnitTest::CreateWindow(sptr<Surface> surface)
{
    OHNativeWindow* nativeWindow = CreateNativeWindowFromSurface(&surface);
    if (nativeWindow == nullptr) {
        cout << "CreateNativeWindowFromSurface failed!" << endl;
        return nullptr;
    }
    int32_t ret = OH_NativeWindow_NativeWindowHandleOpt(
        nativeWindow, SET_FORMAT, GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    if (ret != AVCS_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_FORMAT fail" << endl;
        return nullptr;
    }
    ret = OH_NativeWindow_NativeWindowHandleOpt(
        nativeWindow, SET_BUFFER_GEOMETRY, DEFAULT_VIDEO_WIDTH, DEFAULT_VIDEO_HEIGHT);
    if (ret != AVCS_ERR_OK) {
        cout << "NativeWindowHandleOpt SET_BUFFER_GEOMETRY fail" << endl;
        return nullptr;
    }
    return nativeWindow;
}

void AvcCodecCoverageUnitTest::FlushWindow(OHNativeWindow* nativeWindow)
{
    if (nativeWindow == nullptr) {
        return;
    }

    int fenceFd = -1;
    OHNativeWindowBuffer* ohNativeWindowBuffer = nullptr;
    int32_t err = OH_NativeWindow_NativeWindowRequestBuffer(
        nativeWindow, &ohNativeWindowBuffer, &fenceFd);
    if (err != 0) {
        cout << "RequestBuffer failed " << endl;
    }

    Region region{nullptr, 0};
    err = OH_NativeWindow_NativeWindowFlushBuffer(
        nativeWindow, ohNativeWindowBuffer, fenceFd, region);
    if (err != 0) {
        cout << "flush failed " << err << endl;
    }
    return;
}

/**
 * @tc.name: Test_Codec_Config_001
 * @tc.desc: codec Configure
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Config_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, VIDEO_IFRAME_INTERVAL_MIN_TIME);

    // CQ & Quality & qp
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(VideoEncodeBitrateMode::CQ));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_QUALITY, VIDEO_QUALITY_DEFAULT);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MIN, VIDEO_QP_MIN);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_QP_MAX, VIDEO_QP_MAX);

    // profile & level
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PROFILE, 0);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_LEVEL, 0);

    format_.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE_DOUBLE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, 1);

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, 1);

    // callback
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, 0);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Config_002
 * @tc.desc: codec Configure Framerate Bitrate .. Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Config_002, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, VIDEO_BITRATE_MAX_SIZE + 1);
    format_.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE,
        static_cast<double>(VIDEO_FRAMERATE_MAX_SIZE + 1));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_I_FRAME_INTERVAL, VIDEO_IFRAME_INTERVAL_MAX_TIME + 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(VideoEncodeBitrateMode::CBR));

    format_.PutIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, -1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, -1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, -1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, -1);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Config_003
 * @tc.desc: codec Configure Width Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Config_003, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, VIDEO_MAX_WIDTH_SIZE + 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(VideoEncodeBitrateMode::VBR));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name: Test_Codec_Config_004
 * @tc.desc: codec Configure Height Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Config_004, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, VIDEO_MAX_HEIGHT_SIZE + 1);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name: Test_Codec_Config_005
 * @tc.desc: codec Configure PixelFormat Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Config_005, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::SURFACE_FORMAT));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Config_006
 * @tc.desc: codec input buffer count error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Config_006, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);

    avcEncoder_->format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, 1);
    avcEncoder_->format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, 1);

    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: Test_Codec_Config_007
 * @tc.desc: codec output buffer count error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Config_007, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);

    avcEncoder_->format_.PutIntValue(MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, 1);

    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: Test_Codec_Encode_NV21_001
 * @tc.desc: codec NV21 encode
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Encode_NV21_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Flush();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Encode_NV12_001
 * @tc.desc: codec NV12 encode
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Encode_NV12_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV12));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Flush();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Encode_YUV_001
 * @tc.desc: codec yuv420 encode
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Encode_YUV_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::YUVI420));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Flush();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Encode_RGBA_001
 * @tc.desc: codec rgba encode
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Encode_RGBA_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::RGBA));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Flush();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Format_Error_001
 * @tc.desc: codec rgba encode
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Format_Error_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::RGBA));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    avcEncoder_->srcPixelFmt_ = VideoPixelFormat::UNKNOWN;
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Start_001
 * @tc.desc: codec Start Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Start_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    avcEncoder_->callback_ = nullptr;
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Test_Codec_Start_002
 * @tc.desc: codec Start Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Start_002, TestSize.Level1)
{
    int32_t ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name: Test_Codec_Reset_001
 * @tc.desc: codec Reset
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Reset_001, TestSize.Level1)
{
    int32_t ret = avcEncoder_->Reset();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Flush_001.
 * @tc.desc: codec Flush Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Flush_001, TestSize.Level1)
{
    int32_t ret = avcEncoder_->Flush();
    EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name: Test_Codec_Start_Stop_Stop_001
 * @tc.desc: codec Start Stop Stop
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Start_Stop_Stop_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Create_Error_001
 * @tc.desc: codec Create Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Create_Error_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreateError;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_INVALID_OPERATION);
}

/**
 * @tc.name: Test_Codec_Process_Error_001
 * @tc.desc: codec Process Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Process_Error_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcessError;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Process_Error_002
 * @tc.desc: codec Process Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Process_Error_002, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = nullptr;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Delete_Error_001
 * @tc.desc: codec Delete Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Delete_Error_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDeleteError;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_Delete_Error_002
 * @tc.desc: codec Delete Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_Delete_Error_002, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDeleteError;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);
    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_GetOutFormat_001
 * @tc.desc: codec Delete Error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_GetOutFormat_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    Format outFormat;
    ret = avcEncoder_->GetOutputFormat(outFormat);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_GetInFormat_001
 * @tc.desc: codec get in format
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_GetInFormat_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, 1);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->SetParameter(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    Format inFormat;
    ret = avcEncoder_->GetInputFormat(inFormat);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->srcPixelFmt_ = VideoPixelFormat::RGBA;
    ret = avcEncoder_->GetInputFormat(inFormat);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Codec_GetInFormat_002
 * @tc.desc: codec get in format
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_GetInFormat_002, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_REQUEST_I_FRAME, 1);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    int32_t ret = avcEncoder_->SetParameter(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    Format inFormat;
    GraphicPixelFormat graphicPixFmt;
    ret = avcEncoder_->GetInputFormat(inFormat);
    inFormat.GetIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, *(int *)&graphicPixFmt);
    EXPECT_EQ(graphicPixFmt, GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP);

    avcEncoder_->srcPixelFmt_ = VideoPixelFormat::RGBA;
    ret = avcEncoder_->GetInputFormat(inFormat);
    inFormat.GetIntValue(OHOS::Media::Tag::VIDEO_GRAPHIC_PIXEL_FORMAT, *(int *)&graphicPixFmt);
    EXPECT_EQ(graphicPixFmt, GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888);
}

/**
 * @tc.name: Test_Codec_GetCapability_001
 * @tc.desc: codec get capability
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_GetCapability_001, TestSize.Level1)
{
    std::vector<CapabilityData> capaArray;
    CapabilityData capsData;
    avcEncoder_->GetCapabilityData(capsData, 0);
    EXPECT_EQ(capsData.isVendor, false);
    int32_t ret = avcEncoder_->GetCodecCapability(capaArray);
    EXPECT_EQ(ret, AVCS_ERR_UNSUPPORT);
}

/**
 * @tc.name: Test_Codec_GetCapability_002
 * @tc.desc: codec get capability
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Codec_GetCapability_002, TestSize.Level1)
{
    std::vector<CapabilityData> capaArray;
    CapabilityData capsData;
    avcEncoder_->GetCapabilityData(capsData, 0);
    EXPECT_EQ(capsData.graphicPixFormat[0], static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_P));
    EXPECT_EQ(capsData.graphicPixFormat[1], static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCBCR_420_SP));
    EXPECT_EQ(capsData.graphicPixFormat[2], static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_YCRCB_420_SP));
    EXPECT_EQ(capsData.graphicPixFormat[3], static_cast<int32_t>(GraphicPixelFormat::GRAPHIC_PIXEL_FMT_RGBA_8888));
}

/**
 * @tc.name: Test_Encoder_Surface_Mod_001
 * @tc.desc: encoder Surface mod, not create window
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Surface_Mod_001, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    sptr<Surface> surface = avcEncoder_->CreateInputSurface();
    EXPECT_NE(surface, nullptr);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);

    ret = avcEncoder_->SetOutputSurface(nullptr);
    EXPECT_EQ(ret, AVCS_ERR_OK);
    ret = avcEncoder_->RenderOutputBuffer(0);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->NotifyEos();
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Encoder_Surface_Mod_002
 * @tc.desc: encoder Surface mod, nv21 create window
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Surface_Mod_002, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);

    sptr<Surface> surface = avcEncoder_->CreateInputSurface();
    EXPECT_NE(surface, nullptr);
    OHNativeWindow* nativeWindow = CreateWindow(surface);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);

    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    FlushWindow(nativeWindow);
    sleep(1);
    ret = avcEncoder_->NotifyEos();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Encoder_Surface_Mod_003
 * @tc.desc: encoder Surface mod, nv12 create window
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Surface_Mod_003, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV12));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, 1);

    sptr<Surface> surface = avcEncoder_->CreateInputSurface();
    EXPECT_NE(surface, nullptr);
    OHNativeWindow* nativeWindow = CreateWindow(surface);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);

    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    FlushWindow(nativeWindow);
    sleep(1);
    ret = avcEncoder_->NotifyEos();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Encoder_Surface_Mod_004
 * @tc.desc: encoder Surface mod, rgba create window
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Surface_Mod_004, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::RGBA));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, 1);

    sptr<Surface> surface = avcEncoder_->CreateInputSurface();
    EXPECT_NE(surface, nullptr);
    OHNativeWindow* nativeWindow = CreateWindow(surface);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);

    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    FlushWindow(nativeWindow);
    sleep(1);
    ret = avcEncoder_->NotifyEos();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Encoder_Surface_Mod_005
 * @tc.desc: encoder Surface mod, set discard buffer flag
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Surface_Mod_005, TestSize.Level1)
{
    format_ = Format();
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_VIDEO_WIDTH);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_VIDEO_HEIGHT);
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT,
        static_cast<int32_t>(VideoPixelFormat::NV21));
    format_.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_VIDEO_FRAMERATE);
    format_.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, DEFAULT_VIDEO_BITRATE);
    format_.PutIntValue(OHOS::Media::Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, 1);

    avcCallback_->setDiscardBufferFlag = true;
    sptr<Surface> surface = avcEncoder_->CreateInputSurface();
    EXPECT_NE(surface, nullptr);
    OHNativeWindow* nativeWindow = CreateWindow(surface);

    int32_t ret = avcEncoder_->Configure(format_);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    avcEncoder_->avcEncoderCreateFunc_ = &EncoderCreate;
    avcEncoder_->avcEncoderFrameFunc_ = &EncoderProcess;
    avcEncoder_->avcEncoderDeleteFunc_ = &EncoderDelete;
    avcEncoder_->handle_ = reinterpret_cast<void *>(FAKE_PTR);

    ret = avcEncoder_->Start();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    FlushWindow(nativeWindow);
    sleep(1);
    ret = avcEncoder_->NotifyEos();
    EXPECT_EQ(ret, AVCS_ERR_OK);
    sleep(1);
    ret = avcEncoder_->Stop();
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = avcEncoder_->Release();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Encoder_Create_Surface_001
 * @tc.desc: encoder create & get surface
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Create_Surface_001, TestSize.Level1)
{
    sptr<Surface> surface = avcEncoder_->CreateInputSurface();
    EXPECT_NE(surface, nullptr);
    sptr<Surface> surface2 = avcEncoder_->CreateInputSurface();
    EXPECT_EQ(surface2, nullptr);
    int32_t ret = avcEncoder_->SetInputSurface(surface);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    ret = avcEncoder_->SetInputSurface(nullptr);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
    sptr<Surface> surface3 = avcEncoder_->inputSurface_;
    ret = avcEncoder_->SetInputSurface(surface3);
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: Test_Encoder_QueueInputBuffer_001
 * @tc.desc: encoder QueueInputBuffer error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_QueueInputBuffer_001, TestSize.Level1)
{
    sptr<Surface> surface = avcEncoder_->CreateInputSurface();
    EXPECT_NE(surface, nullptr);
    int32_t ret = avcEncoder_->QueueInputBuffer(0);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name: Test_Encoder_CheckBufferSize_001
 * @tc.desc: encoder CheckBufferSize error
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_CheckBufferSize_001, TestSize.Level1)
{
    int32_t ret = avcEncoder_->CheckBufferSize(0, 1, 1, VideoPixelFormat::RGBA);
    EXPECT_EQ(ret, AVCS_ERR_UNSUPPORT_SOURCE);
}

/**
 * @tc.name: Test_Encoder_Utils_001
 * @tc.desc: encoder utils coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Utils_001, TestSize.Level1)
{
    int32_t flag = AvcFrameTypeToBufferFlag(1);
    EXPECT_EQ(flag, AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_PARTIAL_FRAME);
    flag = AvcFrameTypeToBufferFlag(0);
    EXPECT_EQ(flag, AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_CODEC_DATA);
}

/**
 * @tc.name: Test_Encoder_Utils_002
 * @tc.desc: encoder utils coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Utils_002, TestSize.Level1)
{
    VideoPixelFormat fmt = TranslateVideoPixelFormat(GRAPHIC_PIXEL_FMT_YCBCR_420_P);
    EXPECT_EQ(fmt, VideoPixelFormat::YUVI420);
    COLOR_FORMAT clr_fmt = TranslateVideoFormatToAvc(fmt);
    EXPECT_EQ(clr_fmt, COLOR_FORMAT::YUV_420P);

    fmt = TranslateVideoPixelFormat(GRAPHIC_PIXEL_FMT_RGBA_8888);
    EXPECT_EQ(fmt, VideoPixelFormat::RGBA);
    clr_fmt = TranslateVideoFormatToAvc(fmt);
    EXPECT_EQ(clr_fmt, COLOR_FORMAT::YUV_420SP_VU);

    fmt = TranslateVideoPixelFormat(GRAPHIC_PIXEL_FMT_YCRCB_420_SP);
    EXPECT_EQ(fmt, VideoPixelFormat::NV21);
    clr_fmt = TranslateVideoFormatToAvc(fmt);
    EXPECT_EQ(clr_fmt, COLOR_FORMAT::YUV_420SP_VU);

    fmt = TranslateVideoPixelFormat(GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    EXPECT_EQ(fmt, VideoPixelFormat::NV12);
    clr_fmt = TranslateVideoFormatToAvc(fmt);
    EXPECT_EQ(clr_fmt, COLOR_FORMAT::YUV_420SP_UV);

    fmt = TranslateVideoPixelFormat(GRAPHIC_PIXEL_FMT_BUTT);
    EXPECT_EQ(fmt, VideoPixelFormat::UNKNOWN);
    clr_fmt = TranslateVideoFormatToAvc(fmt);
    EXPECT_EQ(clr_fmt, COLOR_FORMAT::YUV_420P);
}

/**
 * @tc.name: Test_Encoder_Utils_003
 * @tc.desc: encoder utils coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Utils_003, TestSize.Level1)
{
    COLOR_MATRIX matrix = TranslateMatrix(MatrixCoefficient::MATRIX_COEFFICIENT_BT709);
    EXPECT_EQ(matrix, COLOR_MATRIX::MATRIX_BT709);
    matrix = TranslateMatrix(MatrixCoefficient::MATRIX_COEFFICIENT_FCC);
    EXPECT_EQ(matrix, COLOR_MATRIX::MATRIX_FCC47_73_682);
    matrix = TranslateMatrix(MatrixCoefficient::MATRIX_COEFFICIENT_BT601_525);
    EXPECT_EQ(matrix, COLOR_MATRIX::MATRIX_BT601);
    matrix = TranslateMatrix(MatrixCoefficient::MATRIX_COEFFICIENT_SMPTE_ST240);
    EXPECT_EQ(matrix, COLOR_MATRIX::MATRIX_240M);
    matrix = TranslateMatrix(MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_NCL);
    EXPECT_EQ(matrix, COLOR_MATRIX::MATRIX_BT2020);
    matrix = TranslateMatrix(MatrixCoefficient::MATRIX_COEFFICIENT_BT2020_CL);
    EXPECT_EQ(matrix, COLOR_MATRIX::MATRIX_BT2020_CONSTANT);
    matrix = TranslateMatrix(MatrixCoefficient::MATRIX_COEFFICIENT_ICTCP);
    EXPECT_EQ(matrix, COLOR_MATRIX::MATRIX_UNSPECIFIED);
}

/**
 * @tc.name: Test_Encoder_Utils_004
 * @tc.desc: encoder utils coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Utils_004, TestSize.Level1)
{
    COLOR_RANGE range = TranslateRange(0);
    EXPECT_EQ(range, COLOR_RANGE::RANGE_LIMITED);
    range = TranslateRange(1);
    EXPECT_EQ(range, COLOR_RANGE::RANGE_FULL);
    range = TranslateRange(2);
    EXPECT_EQ(range, COLOR_RANGE::RANGE_UNSPECIFIED);
}

/**
 * @tc.name: Test_Encoder_Utils_005
 * @tc.desc: encoder utils coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Utils_005, TestSize.Level1)
{
    ENC_PROFILE profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_BASELINE);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_BASE);
    profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_HIGH);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_HIGH);
    profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_CONSTRAINED_HIGH);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_HIGH);
    profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_EXTENDED);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_HIGH);
    profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_HIGH_10);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_HIGH);
    profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_HIGH_422);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_HIGH);
    profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_HIGH_444);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_HIGH);
    profile = TranslateEncProfile(AVCProfile::AVC_PROFILE_MAIN);
    EXPECT_EQ(profile, ENC_PROFILE::PROFILE_MAIN);
}

/**
 * @tc.name: Test_Encoder_Utils_006
 * @tc.desc: encoder utils coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Utils_006, TestSize.Level1)
{
    ENC_MODE mode = TranslateEncMode(VideoEncodeBitrateMode::CBR);
    EXPECT_EQ(mode, ENC_MODE::MODE_CBR);
    mode = TranslateEncMode(VideoEncodeBitrateMode::VBR);
    EXPECT_EQ(mode, ENC_MODE::MODE_VBR);
    mode = TranslateEncMode(VideoEncodeBitrateMode::CQ);
    EXPECT_EQ(mode, ENC_MODE::MODE_CQP);
}

/**
 * @tc.name: Test_Encoder_Convert_001
 * @tc.desc: encoder convert coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Convert_001, TestSize.Level1)
{
    int32_t width = CONVERT_WIDTH;
    int32_t height = CONVERT_HEIGHT;
    int32_t dstBufferSize = width * height * YUV_BUFFER_SIZE / 2;
    uint8_t *dst = (uint8_t *)malloc(dstBufferSize);
    uint8_t *src = (uint8_t *)malloc(width * height * RGBA_BUFFER_SIZE);
    RgbImageData data = {
        .data = src,
        .stride = width,
        .matrix = COLOR_MATRIX::MATRIX_BT709,
        .range = COLOR_RANGE::RANGE_FULL,
        .bytesPerPixel = RGBA_BUFFER_SIZE,
    };
    int32_t ret = ConvertRgbToYuv420(dst, width, height, dstBufferSize, data);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = ConvertRgbToYuv420(dst, width, height, dstBufferSize - 1, data);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);

    ret = ConvertRgbToYuv420(nullptr, width, height, dstBufferSize, data);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);

#if defined(ARMV8)
    ret = ConvertRgbToYuv420Neon(dst, width, height, dstBufferSize, data);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = ConvertRgbToYuv420Neon(dst, width, height, dstBufferSize - 1, data);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);

    ret = ConvertRgbToYuv420Neon(nullptr, width, height, dstBufferSize, data);
    EXPECT_EQ(ret, AVCS_ERR_INVALID_VAL);
#endif
}

/**
 * @tc.name: Test_Encoder_Convert_002
 * @tc.desc: encoder convert coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Convert_002, TestSize.Level1)
{
    int32_t width = CONVERT_WIDTH;
    int32_t height = CONVERT_HEIGHT;
    int32_t dstBufferSize = width * height * YUV_BUFFER_SIZE / 2;
    uint8_t *dst = (uint8_t *)malloc(dstBufferSize);
    uint8_t *src = (uint8_t *)malloc(dstBufferSize);
    YuvImageData data = {
        .data = src,
        .stride = width,
        .uvOffset = 0,
    };
    int32_t ret = ConvertNv12ToYuv420(dst, width, height, dstBufferSize, data);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = ConvertNv12ToYuv420(dst, width, height, dstBufferSize - 1, data);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: Test_Encoder_Convert_003
 * @tc.desc: encoder convert coverage
 */
HWTEST_F(AvcCodecCoverageUnitTest, Test_Encoder_Convert_003, TestSize.Level1)
{
    int32_t width = CONVERT_WIDTH;
    int32_t height = CONVERT_HEIGHT;
    int32_t dstBufferSize = width * height * YUV_BUFFER_SIZE / 2;
    uint8_t *dst = (uint8_t *)malloc(dstBufferSize);
    uint8_t *src = (uint8_t *)malloc(dstBufferSize);
    YuvImageData data = {
        .data = src,
        .stride = width,
        .uvOffset = 0,
    };
    int32_t ret = ConvertNv21ToYuv420(dst, width, height, dstBufferSize, data);
    EXPECT_EQ(ret, AVCS_ERR_OK);

    ret = ConvertNv21ToYuv420(dst, width, height, dstBufferSize - 1, data);
    EXPECT_EQ(ret, AVCS_ERR_NO_MEMORY);
}
} // namespace