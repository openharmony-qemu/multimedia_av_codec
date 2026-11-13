/*
 * Copyright (C) 2024-2026 Huawei Device Co., Ltd.
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
#include <ctime>
#include <thread>
#include <chrono>
#include "surface_encoder_adapter_unit_test.h"
#include "surface_encoder_adapter.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "meta/format.h"
#include "media_description.h"
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"
#include "common/log.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void SurfaceEncoderAdapterUnitTest::SetUpTestCase(void) {}

void SurfaceEncoderAdapterUnitTest::TearDownTestCase(void) {}

void SurfaceEncoderAdapterUnitTest::SetUp(void)
{
    surfaceEncoderAdapter_ = std::make_shared<SurfaceEncoderAdapter>();
}

void SurfaceEncoderAdapterUnitTest::TearDown(void)
{
    surfaceEncoderAdapter_ = nullptr;
}

/**
 * @tc.name: SurfaceEncoderAdapter_Init_0100
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Init_0100, TestSize.Level1)
{
    std::string mime = "SurfaceEncoderAdaptertest";
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Init(mime, true);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    ret = surfaceEncoderAdapter_->Init(mime, true);
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    ret = surfaceEncoderAdapter_->Init(mime, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Configure_0100
 * @tc.desc: Configure
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Configure_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = true;
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetWatermark_0100
 * @tc.desc: SetWatermark
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetWatermark_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    std::shared_ptr<AVBuffer> waterMarkBuffer = AVBuffer::CreateAVBuffer();
    EXPECT_EQ(surfaceEncoderAdapter_->SetWatermark(waterMarkBuffer), Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    EXPECT_EQ(surfaceEncoderAdapter_->SetWatermark(waterMarkBuffer), Status::ERROR_UNKNOWN);
    waterMarkBuffer = nullptr;
    EXPECT_EQ(surfaceEncoderAdapter_->SetWatermark(waterMarkBuffer), Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetEncoderAdapterCallback_0100
 * @tc.desc: SetEncoderAdapterCallback
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetEncoderAdapterCallback_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    std::shared_ptr<EncoderAdapterCallback> encoderAdapterCallback = std::make_shared<MyEncoderAdapterCallback>();
    Status ret = surfaceEncoderAdapter_->SetEncoderAdapterCallback(encoderAdapterCallback);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->SetEncoderAdapterCallback(encoderAdapterCallback);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Configure_NullMeta_0100
 * @tc.desc: Configure returns ERROR_NULL_POINTER when meta is nullptr (null guard added by security remediation)
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Configure_NullMeta_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::IDLE;
    std::shared_ptr<Meta> nullMeta = nullptr;
    Status ret = surfaceEncoderAdapter_->Configure(nullMeta);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
    EXPECT_EQ(surfaceEncoderAdapter_->curState_, ProcessStateCode::IDLE);
    EXPECT_NE(surfaceEncoderAdapter_->curState_, ProcessStateCode::RECORDING);
    EXPECT_FALSE(surfaceEncoderAdapter_->isTransCoderMode);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetEncoderAdapterCallback_Null_0100
 * @tc.desc: SetEncoderAdapterCallback returns ERROR_INVALID_PARAMETER when callback is nullptr
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetEncoderAdapterCallback_Null_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->encoderAdapterCallback_ = std::make_shared<MyEncoderAdapterCallback>();
    std::shared_ptr<EncoderAdapterCallback> nullCallback = nullptr;
    Status ret = surfaceEncoderAdapter_->SetEncoderAdapterCallback(nullCallback);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
    EXPECT_NE(surfaceEncoderAdapter_->encoderAdapterCallback_, nullptr);
    EXPECT_EQ(surfaceEncoderAdapter_->encoderAdapterCallback_.use_count(), 1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetOutputBufferQueue_NullProducer_0100
 * @tc.desc: SetOutputBufferQueue returns ERROR_INVALID_PARAMETER when producer is nullptr
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetOutputBufferQueue_NullProducer_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->outputBufferQueueProducer_ =
        new OHOS::Media::Pipeline::SurfaceEncoderAdapterUnitTestAP();
    sptr<AVBufferQueueProducer> nullProducer = nullptr;
    Status ret = surfaceEncoderAdapter_->SetOutputBufferQueue(nullProducer);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
    EXPECT_NE(surfaceEncoderAdapter_->outputBufferQueueProducer_, nullptr);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Start_0100
 * @tc.desc: Start
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Start_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Start();
    EXPECT_EQ(ret, Status::ERROR_FRAMEWORK_ERR);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    ret = surfaceEncoderAdapter_->Start();
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    ret = surfaceEncoderAdapter_->Start();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_0100
 * @tc.desc: Stop
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    surfaceEncoderAdapter_->encoderAdapterCallback_ = std::make_shared<MyEncoderAdapterCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_0200
 * @tc.desc: recording -> stop, wait for stop and stop directly
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterCallback_ = std::make_shared<MyEncoderAdapterCallback>();
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::RECORDING;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::STOPPED;
    ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_0300
 * @tc.desc: paused -> stop, wait for stop and stop directly
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_0300, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterCallback_ = std::make_shared<MyEncoderAdapterCallback>();
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::PAUSED;
    // current frame is not the last frame before the pasue time, wait for stop
    surfaceEncoderAdapter_->currentKeyFramePts_ = 5000000000000;
    surfaceEncoderAdapter_->pauseTime_ = 6000000000000;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::PAUSED;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 5000000000000;
    surfaceEncoderAdapter_->pauseTime_ = 4000000000000;
    ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Pause_0100
 * @tc.desc: Pause
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Pause_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    Status ret = surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    ret = surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Resume_0100
 * @tc.desc: Resume
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Resume_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    Status ret = surfaceEncoderAdapter_->Resume();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    ret = surfaceEncoderAdapter_->Resume();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Resume_0200
 * @tc.desc: Resume
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Resume_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::RESUME));
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::RESUME));
    Status ret = surfaceEncoderAdapter_->Resume();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Flush_0100
 * @tc.desc: Flush
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Flush_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Flush();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Flush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Reset_0100
 * @tc.desc: Reset
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Reset_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Reset();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Reset();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Release_0100
 * @tc.desc: Release
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Release_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->Release();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Release();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_NotifyEos_0100
 * @tc.desc: NotifyEos
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_NotifyEos_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->NotifyEos(UINT32_MAX);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->NotifyEos(UINT32_MAX);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_NotifyEos_0200
 * @tc.desc: NotifyEos
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_NotifyEos_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->NotifyEos(UINT32_MAX);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->currentPts_ = 0;
    surfaceEncoderAdapter_->eosPts_ = 1;
    ret = surfaceEncoderAdapter_->NotifyEos(UINT32_MAX);
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->currentPts_ = 1;
    surfaceEncoderAdapter_->eosPts_ = 0;
    ret = surfaceEncoderAdapter_->NotifyEos(UINT32_MAX);
    EXPECT_EQ(ret, Status::OK);
}
/**
 * @tc.name: SurfaceEncoderAdapter_SetParameter_0100
 * @tc.desc: SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetParameter_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    Status ret = surfaceEncoderAdapter_->SetParameter(parameter);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->SetParameter(parameter);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ReleaseBuffer_0100
 * @tc.desc: ReleaseBuffer
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ReleaseBuffer_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isThreadExit_ = true;
    // 预置 indexs_ 有待释放的 index，验证 isThreadExit_=true 时 ReleaseBuffer 直接退出不消费 indexs_
    surfaceEncoderAdapter_->indexs_.push_back(1);
    surfaceEncoderAdapter_->ReleaseBuffer();
    // SUT 在 while 首部检查 isThreadExit_ 直接 break，indexs_ 不应被消费
    EXPECT_EQ(surfaceEncoderAdapter_->indexs_.size(), 1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_0200
 * @tc.desc: ConfigureAboutRGBA
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_PIXEL_FORMAT, 2);
    MediaAVCodec::Format format;
    meta->SetData(Tag::VIDEO_PIXEL_FORMAT, 2);
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);
    // 验证 SUT 将 meta 中的 VIDEO_PIXEL_FORMAT 写入 format
    int32_t pixelFormat = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, pixelFormat));
    EXPECT_EQ(pixelFormat, 2);
    meta->SetData(Tag::VIDEO_ENCODE_BITRATE_MODE, Plugins::VideoEncodeBitrateMode::CQ);
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);
    // 验证 SUT 将 meta 中的 VIDEO_ENCODE_BITRATE_MODE 写入 format
    int32_t bitrateMode = -1;
    EXPECT_TRUE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, bitrateMode));
    EXPECT_EQ(bitrateMode, static_cast<int32_t>(Plugins::VideoEncodeBitrateMode::CQ));
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0100
 * @tc.desc: ConfigureAboutRGBA with VIDEO_ENCODER_SQR_FACTOR in Meta
 *           Covers: branch I (Meta has SQR_FACTOR → Format conversion)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    int32_t expectedSqrFactor = 27;
    meta->SetData(Tag::VIDEO_ENCODER_SQR_FACTOR, expectedSqrFactor);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualSqrFactor = -1;
    EXPECT_TRUE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, actualSqrFactor));
    EXPECT_EQ(actualSqrFactor, expectedSqrFactor);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0200
 * @tc.desc: ConfigureAboutRGBA without VIDEO_ENCODER_SQR_FACTOR in Meta
 *           Covers: branch J (Meta has no SQR_FACTOR → skip conversion)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_PIXEL_FORMAT, 2); // 2: NV12
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualSqrFactor = -1;
    EXPECT_FALSE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, actualSqrFactor));
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0300
 * @tc.desc: Test ConfigureAboutRGBA with VIDEO_ENCODER_SQR_FACTOR boundary value 0
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0300, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    int32_t expectedSqrFactor = 0;
    meta->SetData(Tag::VIDEO_ENCODER_SQR_FACTOR, expectedSqrFactor);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualSqrFactor = -1;
    EXPECT_TRUE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, actualSqrFactor));
    EXPECT_EQ(actualSqrFactor, expectedSqrFactor);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0400
 * @tc.desc: Test ConfigureAboutRGBA with VIDEO_ENCODER_SQR_FACTOR boundary value 51
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0400, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    int32_t expectedSqrFactor = 51;
    meta->SetData(Tag::VIDEO_ENCODER_SQR_FACTOR, expectedSqrFactor);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualSqrFactor = -1;
    EXPECT_TRUE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, actualSqrFactor));
    EXPECT_EQ(actualSqrFactor, expectedSqrFactor);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutEnableTemporalScale_0100
 * @tc.desc: ConfigureAboutEnableTemporalScale
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, ConfigureAboutEnableTemporalScale_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    MediaAVCodec::Format format;
    // enableTemporalScale=false 时 SUT 提前 return，不写 format
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, false);
    surfaceEncoderAdapter_->ConfigureAboutEnableTemporalScale(format, meta);
    int32_t temporalScale = -1;
    EXPECT_FALSE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, temporalScale));
    // enableTemporalScale=true 时 SUT 写 format（固定值 1）
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, true);
    surfaceEncoderAdapter_->ConfigureAboutEnableTemporalScale(format, meta);
    EXPECT_TRUE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, temporalScale));
    EXPECT_EQ(temporalScale, 1);
}

/**
 * @tc.name: OnInputParameterWithAttrAvailablee_100
 * @tc.desc: OnInputParameterWithAttrAvailable
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, OnInputParameterWithAttrAvailablee_100, TestSize.Level1)
{
    uint32_t index = 0;
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    // isTransCoderMode=true：SUT 走 HandleTranscoderMode 后 return，不修改 recordVideoFrameCount_
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
    EXPECT_EQ(surfaceEncoderAdapter_->recordVideoFrameCount_, 0);
    // isTransCoderMode=false：SUT 走 HandlePtsMapping + recordVideoFrameCount_++
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckFrames_100
 * @tc.desc: CheckFrames
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckFrames_100, TestSize.Level1)
{
    int64_t currentPts = 50;
    int64_t checkFramesPauseTime = 0;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::PAUSE));
    bool result = surfaceEncoderAdapter_->CheckFrames(currentPts, checkFramesPauseTime);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_100
 * @tc.desc: CheckAndAdjustFrameRate
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->isSupportBoostFrameRate_ = true;
    surfaceEncoderAdapter_->stoppedVideoFrameCount_ = 2;
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecMimeType_ = "video/avc";
    surfaceEncoderAdapter_->videoWidth_ = 1920;
    surfaceEncoderAdapter_->videoHeight_ = 1080;
    surfaceEncoderAdapter_->IsSupportBoostFrameRate();
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_200
 * @tc.desc: CheckAndAdjustFrameRate
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_200, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = false;
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_300
 * @tc.desc: CheckAndAdjustFrameRate
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_300, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->isSupportBoostFrameRate_ = true;
    surfaceEncoderAdapter_->stoppedVideoFrameCount_ = 0;
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_400
 * @tc.desc: CheckAndAdjustFrameRate
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_400, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->isSupportBoostFrameRate_ = true;
    surfaceEncoderAdapter_->stoppedVideoFrameCount_ = 2;
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_500
 * @tc.desc: CheckAndAdjustFrameRate
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_500, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->isSupportBoostFrameRate_ = true;
    surfaceEncoderAdapter_->stoppedVideoFrameCount_ = 2;
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecMimeType_ = "";
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_600
 * @tc.desc: CheckAndAdjustFrameRate
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_600, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->isSupportBoostFrameRate_ = true;
    surfaceEncoderAdapter_->stoppedVideoFrameCount_ = 2;
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecMimeType_ = "video/avc";
    surfaceEncoderAdapter_->videoWidth_ = 0;
    surfaceEncoderAdapter_->videoHeight_ = 0;
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckFrames_200
 * @tc.desc: CheckFrames
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckFrames_200, TestSize.Level1)
{
    int64_t currentPts = 50;
    int64_t checkFramesPauseTime = 0;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::RESUME));
    bool result = surfaceEncoderAdapter_->CheckFrames(currentPts, checkFramesPauseTime);
    ASSERT_EQ(result, true);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckFrames_300
 * @tc.desc: CheckFrames
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckFrames_300, TestSize.Level1)
{
    int64_t currentPts = 500;
    int64_t checkFramesPauseTime = 200;
    surfaceEncoderAdapter_->lastBufferTime_ = 0;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::PAUSE));
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(150, StateCode::RESUME));
    surfaceEncoderAdapter_->CheckFrames(currentPts, checkFramesPauseTime);
    ASSERT_EQ(checkFramesPauseTime, 200 - (500 - 150));
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckFrames_400
 * @tc.desc: CheckFrames
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckFrames_400, TestSize.Level1)
{
    int64_t currentPts = 500;
    int64_t checkFramesPauseTime = 200;
    surfaceEncoderAdapter_->lastBufferTime_ = 0;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::RESUME));
    surfaceEncoderAdapter_->CheckFrames(currentPts, checkFramesPauseTime);
    ASSERT_EQ(checkFramesPauseTime, 200 - (500 - 100));
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckFrames_500
 * @tc.desc: CheckFrames
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckFrames_500, TestSize.Level1)
{
    int64_t currentPts = 500;
    int64_t checkFramesPauseTime = 200;
    surfaceEncoderAdapter_->lastBufferTime_ = 0;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::PAUSE));
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(150, StateCode::RESUME));
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(200, StateCode::PAUSE));
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(250, StateCode::RESUME));
    surfaceEncoderAdapter_->CheckFrames(currentPts, checkFramesPauseTime);
    ASSERT_EQ(checkFramesPauseTime, 200 - (500 - 250));
}

/**
 * @tc.name: SurfaceEncoderAdapter_TransCoder_200
 * @tc.desc: TransCoderOnOutputBufferAvailable
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_TransCoder_200, TestSize.Level1)
{
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    surfaceEncoderAdapter_->outputBufferQueueProducer_ =
                                new OHOS::Media::Pipeline::SurfaceEncoderAdapterUnitTestAP();
    uint32_t index = 1;
    // Mock RequestBuffer 返回 ERROR_UNKNOWN → SUT 提前 return，不会 push 到 indexs_
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->indexs_.size(), 0);
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->indexs_.size(), 0);
    surfaceEncoderAdapter_->isTransCoderMode = true;
    buffer->pts_ = 1;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    // SUT 因 RequestBuffer 失败未消费 index，indexs_ 应为空
    EXPECT_EQ(surfaceEncoderAdapter_->indexs_.size(), 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_TransCoder_300
 * @tc.desc: TransCoderOnOutputBufferAvailable
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_TransCoder_300, TestSize.Level1)
{
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    surfaceEncoderAdapter_->outputBufferQueueProducer_ =
                                new OHOS::Media::Pipeline::SurfaceEncoderAdapterUnitTestAP();
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    uint32_t index = 1;
    buffer->pts_ = 0;
    surfaceEncoderAdapter_->eosPts_ = 1;
    // Mock RequestBuffer 失败 → SUT 提前 return，不修改 indexs_
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->indexs_.size(), 0);
    buffer->pts_ = 1;
    surfaceEncoderAdapter_->eosPts_ = 0;
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    // TransCoderOnOutputBufferAvailable 不读 codecServer_/eosPts_，且 RequestBuffer 失败时不 push indexs_
    EXPECT_EQ(surfaceEncoderAdapter_->indexs_.size(), 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnOutputBufferAvailable_0100
 * @tc.desc: OnOutputBufferAvailable
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnOutputBufferAvailable_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->outputBufferQueueProducer_ =
                                new OHOS::Media::Pipeline::SurfaceEncoderAdapterUnitTestAP();
    uint32_t index = 1;
    surfaceEncoderAdapter_->isTransCoderMode = true;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    // SUT 在所有路径首部均执行 currentPts_ = max(currentPts_, buffer->pts_)
    buffer->pts_ = 5;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->currentPts_.load(), 5);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->stopTime_ = 1;
    buffer->pts_ = 0;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    // currentPts_ 取 max，传入 0 不应覆盖已有 5
    EXPECT_EQ(surfaceEncoderAdapter_->currentPts_.load(), 5);
    surfaceEncoderAdapter_->stopTime_ = -1;
    buffer->pts_ = 0;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->currentPts_.load(), 5);
    surfaceEncoderAdapter_->stopTime_ = 1;
    buffer->pts_ = 10;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    // currentPts_ 取 max，10 > 5 → 更新为 10
    EXPECT_EQ(surfaceEncoderAdapter_->currentPts_.load(), 10);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureGeneralFormat_0100
 * @tc.desc: ConfigureGeneralFormat
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureGeneralFormat_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    MediaAVCodec::Format format;
    meta->SetData(Tag::VIDEO_WIDTH, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    // SUT 将 meta 中的 VIDEO_WIDTH 写入 format 并同步到成员 videoWidth_
    int32_t width = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, width));
    EXPECT_EQ(width, 1);
    EXPECT_EQ(surfaceEncoderAdapter_->videoWidth_, 1);
    meta->SetData(Tag::VIDEO_HEIGHT, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    int32_t height = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, height));
    EXPECT_EQ(height, 1);
    EXPECT_EQ(surfaceEncoderAdapter_->videoHeight_, 1);
    meta->SetData(Tag::VIDEO_CAPTURE_RATE, 1.0);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    double captureRate = -1.0;
    EXPECT_TRUE(format.GetDoubleValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CAPTURE_RATE, captureRate));
    EXPECT_EQ(captureRate, 1.0);
    meta->SetData(Tag::MEDIA_BITRATE, static_cast<int64_t>(1));
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    int64_t bitrate = -1;
    EXPECT_TRUE(format.GetLongValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, bitrate));
    EXPECT_EQ(bitrate, 1);
    meta->SetData(Tag::VIDEO_FRAME_RATE, 1.0);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    EXPECT_EQ(surfaceEncoderAdapter_->videoFrameRate_.load(), 1);
    meta->SetData(Tag::MIME_TYPE, std::string("video/hevc"));
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    std::string mimeType;
    EXPECT_TRUE(format.GetStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType));
    EXPECT_EQ(mimeType, "video/hevc");
    meta->SetData(Tag::VIDEO_H265_PROFILE, Plugins::HEVCProfile::HEVC_PROFILE_MAIN_10);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    int32_t profile = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PROFILE, profile));
    EXPECT_EQ(profile, static_cast<int32_t>(Plugins::HEVCProfile::HEVC_PROFILE_MAIN_10));
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureEnableFormat_0100
 * @tc.desc: ConfigureEnableFormat
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureEnableFormat_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    MediaAVCodec::Format format;
    // meta 无 VIDEO_ENCODER_ENABLE_WATERMARK → SUT 不写 format
    surfaceEncoderAdapter_->ConfigureEnableFormat(format, meta);
    int32_t enableWatermark = -1;
    EXPECT_FALSE(format.GetIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, enableWatermark));
    // meta 有 VIDEO_ENCODER_ENABLE_WATERMARK=true → SUT 写 format（PutIntValue 写入 1）
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, true);
    surfaceEncoderAdapter_->ConfigureEnableFormat(format, meta);
    EXPECT_TRUE(format.GetIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, enableWatermark));
    EXPECT_EQ(enableWatermark, 1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddStartPts_0100
 * @tc.desc: AddStartPts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddStartPts_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
                            std::make_shared<MyEncoderAdapterKeyFramePtsCallback>();
    // isStartKeyFramePts_=false → SUT 不修改 keyFramePts_/isStartKeyFramePts_
    surfaceEncoderAdapter_->isStartKeyFramePts_ = false;
    surfaceEncoderAdapter_->AddStartPts(1000);  // 1000ns = 1us
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "");
    EXPECT_FALSE(surfaceEncoderAdapter_->isStartKeyFramePts_);
    // isStartKeyFramePts_=true → SUT 追加 keyFramePts_ 并置 isStartKeyFramePts_=false
    surfaceEncoderAdapter_->isStartKeyFramePts_ = true;
    surfaceEncoderAdapter_->AddStartPts(1000);
    EXPECT_FALSE(surfaceEncoderAdapter_->isStartKeyFramePts_);
    // SUT 追加 std::to_string(currentPts / NS_PER_US) + "," → "1,"
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "1,");
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddStopPts_0100
 * @tc.desc: AddStopPts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddStopPts_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
                            std::make_shared<MyEncoderAdapterKeyFramePtsCallback>();
    // isStopKeyFramePts_=false → SUT 不修改任何状态
    surfaceEncoderAdapter_->isStopKeyFramePts_ = false;
    surfaceEncoderAdapter_->AddStopPts();
    EXPECT_FALSE(surfaceEncoderAdapter_->isStopKeyFramePts_);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "");
    // isStopKeyFramePts_=true 且 currentKeyFramePts_ <= stopTime_ → 走 else 分支追加 currentKeyFramePts_
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 0;  // 0us
    surfaceEncoderAdapter_->stopTime_ = 0;
    surfaceEncoderAdapter_->AddStopPts();
    // SUT 处理后置 isStopKeyFramePts_=false 并清空 keyFramePts_
    EXPECT_FALSE(surfaceEncoderAdapter_->isStopKeyFramePts_);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "");
    // isStopKeyFramePts_=true 且 currentKeyFramePts_ > stopTime_ → 走 if 分支追加 preKeyFramePts_
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1000;  // 1000ns = 1us
    surfaceEncoderAdapter_->stopTime_ = 0;
    surfaceEncoderAdapter_->preKeyFramePts_ = 2000;  // 2000ns = 2us
    surfaceEncoderAdapter_->AddStopPts();
    EXPECT_FALSE(surfaceEncoderAdapter_->isStopKeyFramePts_);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "");
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddStopPts_0200
 * @tc.desc: AddStopPts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddStopPts_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
                            std::make_shared<MyEncoderAdapterKeyFramePtsCallback>();
    // isStopKeyFramePts_=true 且 currentKeyFramePts_ > stopTime_ → 走 if 分支
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1;
    surfaceEncoderAdapter_->stopTime_ = 0;
    surfaceEncoderAdapter_->preKeyFramePts_ = 0;
    surfaceEncoderAdapter_->AddStopPts();
    // SUT 处理后置 isStopKeyFramePts_=false 并清空 keyFramePts_
    EXPECT_FALSE(surfaceEncoderAdapter_->isStopKeyFramePts_);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "");
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_0100
 * @tc.desc: AddPauseResumePts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_0100, TestSize.Level1)
{
    int64_t currentPts = 0;
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(currentPts);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_0200
 * @tc.desc: AddPauseResumePts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::PAUSE));
    int64_t currentPts = 0;
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(currentPts);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_0300
 * @tc.desc: AddPauseResumePts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_0300, TestSize.Level1)
{
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::RESUME));
    int64_t currentPts = 0;
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(currentPts);
    ASSERT_EQ(result, true);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_0400
 * @tc.desc: AddPauseResumePts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_0400, TestSize.Level1)
{
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::PAUSE));
    int64_t currentPts = 100;
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(currentPts);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_0500
 * @tc.desc: AddPauseResumePts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_0500, TestSize.Level1)
{
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::RESUME));
    int64_t currentPts = 100;
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(currentPts);
    ASSERT_EQ(result, false);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetEncoderAdapterKeyFramePtsCallback_0100
 * @tc.desc: SetEncoderAdapterKeyFramePtsCallback
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest,
    SurfaceEncoderAdapter_SetEncoderAdapterKeyFramePtsCallback_0100,
    TestSize.Level1)
{
    Status ret = surfaceEncoderAdapter_->SetEncoderAdapterKeyFramePtsCallback(
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>());
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetEnableBFrame_0100
 * @tc.desc: SetEnableBFrame Test
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetEnableBFrame_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::AV_TRANSCODER_ENABLE_B_FRAME>(true);
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->enableBFrame_, true);
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandleTranscoderMode_0100
 * @tc.desc: HandleTranscoderMode returns early when parameter is nullptr (null guard)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandleTranscoderMode_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(false);
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    attribute->PutLongValue(Tag::MEDIA_TIME_STAMP, 2000000);
    std::shared_ptr<Format> nullParameter = nullptr;
    // Should return early without crash due to parameter null guard
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, nullParameter);
    // firstNonDiscardFrameProcessed_ should remain false (early return before any logic)
    EXPECT_FALSE(surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.load());
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandleTranscoderMode_0200
 * @tc.desc: HandleTranscoderMode in non-clip mode (clipStartRelativeUs_ <= 0) sets shouldDiscard=false
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandleTranscoderMode_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1; // non-clip mode
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    attribute->PutLongValue(Tag::MEDIA_TIME_STAMP, 2000000);
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    int32_t discardValue = 1;
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandleTranscoderMode_0300
 * @tc.desc: HandleTranscoderMode with null attribute keeps shouldDiscard=false
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandleTranscoderMode_0300, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    std::shared_ptr<Format> nullAttribute = nullptr;
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    int32_t discardValue = 1;
    surfaceEncoderAdapter_->HandleTranscoderMode(0, nullAttribute, parameter);
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandleTranscoderMode_0400
 * @tc.desc: HandleTranscoderMode discards frame when currentPts < clipStartRelativeUs_
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandleTranscoderMode_0400, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    attribute->PutLongValue(Tag::MEDIA_TIME_STAMP, 500000); // pts < clipStartRelativeUs_ → discard
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    int32_t discardValue = 0;
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandleTranscoderMode_0500
 * @tc.desc: HandleTranscoderMode requests IDR for first non-discard frame
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandleTranscoderMode_0500, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(false);
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    attribute->PutLongValue(Tag::MEDIA_TIME_STAMP, 2000000); // pts >= clipStartRelativeUs_ → keep
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    int32_t discardValue = 1;
    int32_t idrValue = 0;
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
    parameter->GetIntValue(Tag::VIDEO_REQUEST_I_FRAME, idrValue);
    EXPECT_EQ(idrValue, 1);
    EXPECT_TRUE(surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.load());
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandleTranscoderMode_0600
 * @tc.desc: HandleTranscoderMode does not request IDR for subsequent non-discard frames
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandleTranscoderMode_0600, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(true); // already processed
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    attribute->PutLongValue(Tag::MEDIA_TIME_STAMP, 3000000); // pts >= clipStartRelativeUs_ → keep
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    int32_t discardValue = 1;
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
    int32_t idrValue = 1;
    EXPECT_FALSE(parameter->GetIntValue(Tag::VIDEO_REQUEST_I_FRAME, idrValue));
}

/**
 * @tc.name: SurfaceEncoderAdapter_DecideFrameDiscardAndRequestIdr_0100
 * @tc.desc: DecideFrameDiscardAndRequestIdr returns true (discard) when pts < clipStart
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_DecideFrameDiscardAndRequestIdr_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    bool result = surfaceEncoderAdapter_->DecideFrameDiscardAndRequestIdr(500000, parameter);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: SurfaceEncoderAdapter_DecideFrameDiscardAndRequestIdr_0200
 * @tc.desc: DecideFrameDiscardAndRequestIdr returns false (keep) and requests IDR for first kept frame
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_DecideFrameDiscardAndRequestIdr_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(false);
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    bool result = surfaceEncoderAdapter_->DecideFrameDiscardAndRequestIdr(2000000, parameter);
    EXPECT_FALSE(result); // keep
    int32_t idrValue = 0;
    parameter->GetIntValue(Tag::VIDEO_REQUEST_I_FRAME, idrValue);
    EXPECT_EQ(idrValue, 1);
    EXPECT_TRUE(surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.load());
}

/**
 * @tc.name: SurfaceEncoderAdapter_DecideFrameDiscardAndRequestIdr_0300
 * @tc.desc: DecideFrameDiscardAndRequestIdr returns false and no IDR for subsequent kept frames
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_DecideFrameDiscardAndRequestIdr_0300, TestSize.Level1)
{
    surfaceEncoderAdapter_->clipStartRelativeUs_ = 1000000;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(true); // already processed
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    bool result = surfaceEncoderAdapter_->DecideFrameDiscardAndRequestIdr(3000000, parameter);
    EXPECT_FALSE(result); // keep
    int32_t idrValue = 1;
    EXPECT_FALSE(parameter->GetIntValue(Tag::VIDEO_REQUEST_I_FRAME, idrValue));
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetClipStartTime_0100
 * @tc.desc: SetClipStartTime sets clipStartRelativeUs_ and calls mediaCodec_->SetClipStartTime
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetClipStartTime_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    int64_t expected = 5000000;
    surfaceEncoderAdapter_->SetClipStartTime(expected);
    EXPECT_EQ(surfaceEncoderAdapter_->clipStartRelativeUs_, expected);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetClipStartTime_0200
 * @tc.desc: SetClipStartTime with null codecServer_ does not crash
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetClipStartTime_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    int64_t expected = 3000000;
    surfaceEncoderAdapter_->SetClipStartTime(expected);
    EXPECT_EQ(surfaceEncoderAdapter_->clipStartRelativeUs_, expected);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0100
 * @tc.desc: TransCoder HDR->HDR with HDRVivid flag sets pixel format to SURFACE_FORMAT (P010)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_PIXEL_FORMAT, static_cast<int32_t>(MediaAVCodec::VideoPixelFormat::NV12));
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, true);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualPixelFormat = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, actualPixelFormat));
    EXPECT_EQ(actualPixelFormat, static_cast<int32_t>(MediaAVCodec::VideoPixelFormat::SURFACE_FORMAT));
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0200
 * @tc.desc: Non-transcoder mode with HDRVivid flag does NOT change pixel format (uses original)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    int32_t originalPixelFormat = static_cast<int32_t>(MediaAVCodec::VideoPixelFormat::NV12);
    meta->SetData(Tag::VIDEO_PIXEL_FORMAT, originalPixelFormat);
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, true);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualPixelFormat = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, actualPixelFormat));
    EXPECT_EQ(actualPixelFormat, originalPixelFormat);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0300
 * @tc.desc: TransCoder mode but HDRVivid flag is false uses original pixel format
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0300, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    int32_t originalPixelFormat = static_cast<int32_t>(MediaAVCodec::VideoPixelFormat::NV12);
    meta->SetData(Tag::VIDEO_PIXEL_FORMAT, originalPixelFormat);
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualPixelFormat = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, actualPixelFormat));
    EXPECT_EQ(actualPixelFormat, originalPixelFormat);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0400
 * @tc.desc: TransCoder mode with HDRVivid flag missing uses original pixel format
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0400, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    int32_t originalPixelFormat = static_cast<int32_t>(MediaAVCodec::VideoPixelFormat::NV12);
    meta->SetData(Tag::VIDEO_PIXEL_FORMAT, originalPixelFormat);
    // No VIDEO_IS_HDR_VIVID set
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualPixelFormat = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, actualPixelFormat));
    EXPECT_EQ(actualPixelFormat, originalPixelFormat);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0500
 * @tc.desc: TransCoder HDR->HDR with no VIDEO_PIXEL_FORMAT in meta still writes SURFACE_FORMAT
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_HdrVivid_0500, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, true);
    // No VIDEO_PIXEL_FORMAT set - default VideoPixelFormat::UNKNOWN will be used in else branch,
    // but HDRVivid path takes precedence
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);

    int32_t actualPixelFormat = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, actualPixelFormat));
    EXPECT_EQ(actualPixelFormat, static_cast<int32_t>(MediaAVCodec::VideoPixelFormat::SURFACE_FORMAT));
}

class CountingAVCodecVideoEncoder : public MyAVCodecVideoEncoder {
public:
    int32_t notifyEosCount_ = 0;
    int32_t NotifyEos() override
    {
        notifyEosCount_++;
        return 0;
    }
};

class ConfigurableAVCodecVideoEncoder : public MyAVCodecVideoEncoder {
public:
    int32_t configureRet = 0;
    int32_t startRet = 0;
    int32_t stopRet = 0;
    int32_t flushRet = 0;
    int32_t resetRet = 0;
    int32_t releaseRet = 0;
    int32_t notifyEosRet = 0;
    int32_t setParameterRet = 0;
    int32_t mediaCodecCbRet = 0;
    int32_t paramWithAttrCbRet = 0;
    int32_t Configure(const Format &format) override { return configureRet; }
    int32_t Start() override { return startRet; }
    int32_t Stop() override { return stopRet; }
    int32_t Flush() override { return flushRet; }
    int32_t Reset() override { return resetRet; }
    int32_t Release() override { return releaseRet; }
    int32_t NotifyEos() override { return notifyEosRet; }
    int32_t SetParameter(const Format &format) override { return setParameterRet; }
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback) override
    {
        mediaCodecCb_ = callback;
        return mediaCodecCbRet;
    }
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecParameterWithAttrCallback> &callback) override
    {
        return paramWithAttrCbRet;
    }
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> mediaCodecCb_;
};

class CallbackCapturingAVCodecVideoEncoder : public MyAVCodecVideoEncoder {
public:
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> capturedMediaCodecCb_;
    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback) override
    {
        capturedMediaCodecCb_ = callback;
        return 0;
    }
};

class RecordingEncoderAdapterCallback : public EncoderAdapterCallback {
public:
    int32_t errorCount = 0;
    int32_t lastErrorCode = -1;
    void OnError(MediaAVCodec::AVCodecErrorType type, int32_t errorCode) override
    {
        errorCount++;
        lastErrorCode = errorCode;
    }
    void OnOutputFormatChanged(const std::shared_ptr<Meta> &format) override {}
};

class SuccessAVBufferQueueProducer : public SurfaceEncoderAdapterUnitTestAP {
public:
    Status RequestBuffer(std::shared_ptr<AVBuffer>& outBuffer,
        const AVBufferConfig& config, int32_t timeoutMs) override
    {
        AVBufferConfig cfg = config;
        if (cfg.size <= 0) {
            cfg.size = 1;
        }
        outBuffer = AVBuffer::CreateAVBuffer(cfg);
        return Status::OK;
    }
};

/**
 * @tc.name: SurfaceEncoderAdapter_OnInputParameter_VideoFrameRateZero_0100
 * @tc.desc: Test OnInputParameterWithAttrAvailable guards division by zero when videoFrameRate_ is 0
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnInputParameter_VideoFrameRateZero_0100, TestSize.Level1)
{
    uint32_t index = 0;
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    auto countingEncoder = std::make_shared<CountingAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecServer_ = countingEncoder;
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->isWatermarkMode_ = false;
    surfaceEncoderAdapter_->videoFrameRate_.store(0);
    surfaceEncoderAdapter_->stopTime_ = 100;
    int64_t recordBefore = surfaceEncoderAdapter_->recordVideoFrameCount_;
    surfaceEncoderAdapter_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
    EXPECT_GT(surfaceEncoderAdapter_->recordVideoFrameCount_, recordBefore);
    EXPECT_EQ(countingEncoder->notifyEosCount_, 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnOutputBufferAvailable_NullBuffer_0100
 * @tc.desc: OnOutputBufferAvailable with buffer=nullptr — covers L687 guard (cherry-pick null guard).
 *           Encoder callback receives nullptr buffer (abnormal): guard returns early,
 *           currentPts_ not updated (L689 skipped), indexs_ not pushed.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnOutputBufferAvailable_NullBuffer_0100,
         TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    int64_t ptsBefore = surfaceEncoderAdapter_->currentPts_.load();
    std::shared_ptr<AVBuffer> nullBuffer = nullptr;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(1, nullBuffer);
    // Guard returns at L687: currentPts_ should stay at initial value
    EXPECT_EQ(surfaceEncoderAdapter_->currentPts_.load(), ptsBefore);
    std::lock_guard<std::mutex> lock(surfaceEncoderAdapter_->releaseBufferMutex_);
    EXPECT_TRUE(surfaceEncoderAdapter_->indexs_.empty());
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnOutputBufferAvailable_NullMemory_0100
 * @tc.desc: OnOutputBufferAvailable with buffer->memory_=nullptr — covers L691 guard.
 *           buffer non-null (pts_ set) but memory_ is null. Guard returns at L691:
 *           currentPts_ updated by L689, but indexs_ not pushed (L717 not executed).
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnOutputBufferAvailable_NullMemory_0100,
         TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    auto buffer = std::make_shared<AVBuffer>();
    ASSERT_NE(buffer, nullptr);
    constexpr int64_t testPts = 5000;
    buffer->pts_ = testPts;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(1, buffer);
    // L689 executed (currentPts_ updated to buffer->pts_), but L691 guard returns before indexs_ push
    EXPECT_EQ(surfaceEncoderAdapter_->currentPts_.load(), testPts);
    std::lock_guard<std::mutex> lock(surfaceEncoderAdapter_->releaseBufferMutex_);
    EXPECT_TRUE(surfaceEncoderAdapter_->indexs_.empty());
}

/**
 * @tc.name: SurfaceEncoderAdapter_TransCoderOnOutputBufferAvailable_NullBuffer_0100
 * @tc.desc: TransCoderOnOutputBufferAvailable with buffer=nullptr — covers L647 guard (cherry-pick null guard).
 *           Transcoder output callback receives nullptr buffer: guard returns at L647,
 *           outputBufferQueueProducer_ not accessed, indexs_ not pushed.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_TransCoderOnOutputBufferAvailable_NullBuffer_0100,
         TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->outputBufferQueueProducer_ = nullptr;  // guard should not access producer
    std::shared_ptr<AVBuffer> nullBuffer = nullptr;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(1, nullBuffer);
    // Guard returns early: indexs_ should not be appended
    std::lock_guard<std::mutex> lock(surfaceEncoderAdapter_->releaseBufferMutex_);
    EXPECT_TRUE(surfaceEncoderAdapter_->indexs_.empty());
}

/**
 * @tc.name: SurfaceEncoderAdapter_TransCoderOnOutputBufferAvailable_NullMemory_0100
 * @tc.desc: TransCoderOnOutputBufferAvailable with buffer->memory_=nullptr — covers L648 guard.
 *           buffer non-null but memory_ is null: guard returns at L648,
 *           outputBufferQueueProducer_ not accessed, indexs_ not pushed.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_TransCoderOnOutputBufferAvailable_NullMemory_0100,
         TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->outputBufferQueueProducer_ = nullptr;  // guard should not access producer
    auto buffer = std::make_shared<AVBuffer>();
    ASSERT_NE(buffer, nullptr);
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(1, buffer);
    // Guard returns early: indexs_ should not be appended
    std::lock_guard<std::mutex> lock(surfaceEncoderAdapter_->releaseBufferMutex_);
    EXPECT_TRUE(surfaceEncoderAdapter_->indexs_.empty());
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetVideoEnableBFrame_0100
 * @tc.desc: SetVideoEnableBFrame null codec returns ERROR_UNKNOWN; valid codec returns OK and sets flag
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetVideoEnableBFrame_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    bool enableBFrame = true;
    EXPECT_EQ(surfaceEncoderAdapter_->SetVideoEnableBFrame(enableBFrame), Status::ERROR_UNKNOWN);
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    enableBFrame = true;
    EXPECT_EQ(surfaceEncoderAdapter_->SetVideoEnableBFrame(enableBFrame), Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->enableBFrame_, true);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetStopTime_0100
 * @tc.desc: SetStopTime records current monotonic time into stopTime_
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetStopTime_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->stopTime_ = -1;
    EXPECT_EQ(surfaceEncoderAdapter_->SetStopTime(), Status::OK);
    EXPECT_NE(surfaceEncoderAdapter_->stopTime_, -1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetOutputBufferQueue_0100
 * @tc.desc: SetOutputBufferQueue stores a non-null producer and returns OK
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetOutputBufferQueue_0100, TestSize.Level1)
{
    sptr<AVBufferQueueProducer> producer = new OHOS::Media::Pipeline::SurfaceEncoderAdapterUnitTestAP();
    EXPECT_EQ(surfaceEncoderAdapter_->SetOutputBufferQueue(producer), Status::OK);
    EXPECT_NE(surfaceEncoderAdapter_->outputBufferQueueProducer_, nullptr);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureFrameRate_Invalid_0100
 * @tc.desc: ConfigureFrameRate logs invalid when videoFrameRate <= 0, still stores the value
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureFrameRate_Invalid_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_FRAME_RATE, 0.0);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureFrameRate(format, meta);
    EXPECT_EQ(surfaceEncoderAdapter_->videoFrameRate_.load(), 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureSQRMode_0100
 * @tc.desc: ConfigureSQRMode uses user-specified SQR_FACTOR when valid (0..51)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureSQRMode_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_ENCODER_SQR_FACTOR, 30);
    MediaAVCodec::Format format;
    format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(MediaAVCodec::VideoEncodeBitrateMode::SQR));
    surfaceEncoderAdapter_->ConfigureSQRMode(format, meta);
    int32_t sqrFactor = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactor));
    EXPECT_EQ(sqrFactor, 30);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureSQRMode_0200
 * @tc.desc: ConfigureSQRMode falls back to default 25 when user SQR_FACTOR exceeds MAX_SQR_FACTOR
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureSQRMode_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_ENCODER_SQR_FACTOR, 100);
    MediaAVCodec::Format format;
    format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(MediaAVCodec::VideoEncodeBitrateMode::SQR));
    surfaceEncoderAdapter_->ConfigureSQRMode(format, meta);
    int32_t sqrFactor = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactor));
    EXPECT_EQ(sqrFactor, 25);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureSQRMode_0300
 * @tc.desc: ConfigureSQRMode falls back to default 25 when SQR_FACTOR absent in meta
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureSQRMode_0300, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    MediaAVCodec::Format format;
    format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(MediaAVCodec::VideoEncodeBitrateMode::SQR));
    surfaceEncoderAdapter_->ConfigureSQRMode(format, meta);
    int32_t sqrFactor = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactor));
    EXPECT_EQ(sqrFactor, 25);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Configure_DroppedFramesCallbackFail_0100
 * @tc.desc: Configure returns ERROR_UNKNOWN when SetCallback(MediaCodecParameterWithAttrCallback) fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Configure_DroppedFramesCallbackFail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->paramWithAttrCbRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    surfaceEncoderAdapter_->isTransCoderMode = false;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Configure_CodecConfigureFail_0100
 * @tc.desc: Configure returns ERROR_UNKNOWN when codecServer_->Configure fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Configure_CodecConfigureFail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->configureRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    surfaceEncoderAdapter_->isTransCoderMode = false;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_EQ(surfaceEncoderAdapter_->Configure(meta), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetEncoderAdapterCallback_SetCallbackFail_0100
 * @tc.desc: SetEncoderAdapterCallback returns ERROR_UNKNOWN when codecServer_->SetCallback fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest,
    SurfaceEncoderAdapter_SetEncoderAdapterCallback_SetCallbackFail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->mediaCodecCbRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    EXPECT_EQ(surfaceEncoderAdapter_->SetEncoderAdapterCallback(
        std::make_shared<MyEncoderAdapterCallback>()), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Start_Fail_0100
 * @tc.desc: Start returns ERROR_VID_ENC_START_FAILED when codecServer_->Start fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Start_Fail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->startRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    EXPECT_EQ(surfaceEncoderAdapter_->Start(), Status::ERROR_VID_ENC_START_FAILED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_Fail_0100
 * @tc.desc: Stop returns ERROR_VID_ENC_STOP_FAILED when codecServer_->Stop fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_Fail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->stopRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    EXPECT_EQ(surfaceEncoderAdapter_->Stop(), Status::ERROR_VID_ENC_STOP_FAILED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Flush_Fail_0100
 * @tc.desc: Flush returns ERROR_UNKNOWN when codecServer_->Flush fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Flush_Fail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->flushRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    EXPECT_EQ(surfaceEncoderAdapter_->Flush(), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Reset_Fail_0100
 * @tc.desc: Reset returns ERROR_UNKNOWN when codecServer_->Reset fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Reset_Fail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->resetRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    EXPECT_EQ(surfaceEncoderAdapter_->Reset(), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Release_Fail_0100
 * @tc.desc: Release returns ERROR_FRAMEWORK_ERR when codecServer_->Release fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Release_Fail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->releaseRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    EXPECT_EQ(surfaceEncoderAdapter_->Release(), Status::ERROR_FRAMEWORK_ERR);
}

/**
 * @tc.name: SurfaceEncoderAdapter_NotifyEos_Fail_0100
 * @tc.desc: NotifyEos returns ERROR_UNKNOWN when codecServer_->NotifyEos fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_NotifyEos_Fail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->notifyEosRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    EXPECT_EQ(surfaceEncoderAdapter_->NotifyEos(100), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetParameter_Fail_0100
 * @tc.desc: SetParameter returns ERROR_UNKNOWN when codecServer_->SetParameter fails
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetParameter_Fail_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<ConfigurableAVCodecVideoEncoder>();
    encoder->setParameterRet = -1;
    surfaceEncoderAdapter_->codecServer_ = encoder;
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    EXPECT_EQ(surfaceEncoderAdapter_->SetParameter(parameter), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Resume_BackPause_0100
 * @tc.desc: Resume with queue back == PAUSE skips the RESUME time update, still returns OK
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Resume_BackPause_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::PAUSE));
    EXPECT_EQ(surfaceEncoderAdapter_->Resume(), Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Resume_PauseTimeSet_0100
 * @tc.desc: Resume accumulates totalPauseTime_ when pauseTime_ != -1, resets pauseTime_ to -1
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Resume_PauseTimeSet_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseTime_ = 1000;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::RESUME));
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::RESUME));
    EXPECT_EQ(surfaceEncoderAdapter_->Resume(), Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->pauseTime_, -1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddStartPts_NullCallback_0100
 * @tc.desc: AddStartPts with null key-frame callback still appends keyFramePts_ (else log branch)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddStartPts_NullCallback_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ = nullptr;
    surfaceEncoderAdapter_->isStartKeyFramePts_ = true;
    surfaceEncoderAdapter_->AddStartPts(1000);
    EXPECT_FALSE(surfaceEncoderAdapter_->isStartKeyFramePts_);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "1,");
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddStopPts_NullCallback_0100
 * @tc.desc: AddStopPts with null key-frame callback skips OnReportKeyFramePts, still clears keyFramePts_
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddStopPts_NullCallback_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ = nullptr;
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1000;
    surfaceEncoderAdapter_->stopTime_ = 0;
    surfaceEncoderAdapter_->keyFramePts_ = "stale";
    surfaceEncoderAdapter_->AddStopPts();
    EXPECT_FALSE(surfaceEncoderAdapter_->isStopKeyFramePts_);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "");
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnError_NonTransCoder_0100
 * @tc.desc: SurfaceEncoderAdapterCallback::OnError forwards to user callback in non-transcoder mode
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnError_NonTransCoder_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<CallbackCapturingAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecServer_ = encoder;
    auto recCb = std::make_shared<RecordingEncoderAdapterCallback>();
    surfaceEncoderAdapter_->SetEncoderAdapterCallback(recCb);
    ASSERT_NE(encoder->capturedMediaCodecCb_, nullptr);
    encoder->capturedMediaCodecCb_->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 100);
    EXPECT_EQ(recCb->errorCount, 1);
    EXPECT_EQ(recCb->lastErrorCode, 100);
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnError_TransCoder_0100
 * @tc.desc: In transcoder mode first OnError is forwarded, second is suppressed by transCoderErrorCbOnce_
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnError_TransCoder_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<CallbackCapturingAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecServer_ = encoder;
    auto recCb = std::make_shared<RecordingEncoderAdapterCallback>();
    surfaceEncoderAdapter_->SetEncoderAdapterCallback(recCb);
    surfaceEncoderAdapter_->isTransCoderMode = true;
    ASSERT_NE(encoder->capturedMediaCodecCb_, nullptr);
    encoder->capturedMediaCodecCb_->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 200);
    EXPECT_EQ(recCb->errorCount, 1);
    encoder->capturedMediaCodecCb_->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 300);
    EXPECT_EQ(recCb->errorCount, 1);
}

class GetCodecInfoFailEncoder : public MyAVCodecVideoEncoder {
public:
    int32_t GetCodecInfo(Format &format) override { return -1; }
};

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureSQRMode_0400
 * @tc.desc: ConfigureSQRMode falls back to default 25 when user SQR_FACTOR < 0 (L286 branch 1)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureSQRMode_0400, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_ENCODER_SQR_FACTOR, -1);
    MediaAVCodec::Format format;
    format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(MediaAVCodec::VideoEncodeBitrateMode::SQR));
    surfaceEncoderAdapter_->ConfigureSQRMode(format, meta);
    int32_t sqrFactor = -1;
    EXPECT_TRUE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactor));
    EXPECT_EQ(sqrFactor, 25);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_0600
 * @tc.desc: AddPauseResumePts RESUME branch with null callback hits L1069 false branch (no report)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_0600, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ = nullptr;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1000;
    surfaceEncoderAdapter_->keyFramePts_ = "";
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::RESUME));
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(100);
    ASSERT_EQ(result, false);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "1,");
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandleWaitforStop_0100
 * @tc.desc: HandleWaitforStop returns directly when hasReceivedEOS_ is true (L1084 true branch), no OnError
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandleWaitforStop_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->hasReceivedEOS_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = -1;
    auto recCb = std::make_shared<RecordingEncoderAdapterCallback>();
    surfaceEncoderAdapter_->encoderAdapterCallback_ = recCb;
    surfaceEncoderAdapter_->HandleWaitforStop();
    EXPECT_EQ(recCb->errorCount, 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_700
 * @tc.desc: CheckAndAdjustFrameRate returns OK immediately when !isSupportBoostFrameRate_
 *           (L1099 branch 2: isStopKeyFramePts_=true, isSupportBoostFrameRate_=false)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_700, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->isSupportBoostFrameRate_ = false;
    surfaceEncoderAdapter_->stoppedVideoFrameCount_ = 5;
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->stoppedVideoFrameCount_, 5);
}

/**
 * @tc.name: SurfaceEncoderAdapter_CheckAndAdjustFrameRate_800
 * @tc.desc: CheckAndAdjustFrameRate returns OK immediately when hasBoostVideoFrameRate_
 *           (L1099 branch 3: isStop=true, isSupport=true, hasBoost=true)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_CheckAndAdjustFrameRate_800, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->isSupportBoostFrameRate_ = true;
    surfaceEncoderAdapter_->hasBoostVideoFrameRate_ = true;
    surfaceEncoderAdapter_->stoppedVideoFrameCount_ = 5;
    Status ret = surfaceEncoderAdapter_->CheckAndAdjustFrameRate();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->stoppedVideoFrameCount_, 5);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_0700
 * @tc.desc: AddPauseResumePts with null callback traverses PAUSE then RESUME;
 *           RESUME branch hits L1069 else (callback null) — no report, no crash
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_0700, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ = nullptr;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 3000;
    surfaceEncoderAdapter_->preKeyFramePts_ = 1000;
    surfaceEncoderAdapter_->keyFramePts_ = "";
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::PAUSE));
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(150, StateCode::RESUME));
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(200);
    ASSERT_EQ(result, false);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "1,3,");
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnInputParameter_WatermarkEarlyReturn_0100
 * @tc.desc: OnInputParameterWithAttrAvailable returns early when isWatermarkMode_ && currentPts <= 0
 *           (L877 true branch: recordVideoFrameCount_ not incremented)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnInputParameter_WatermarkEarlyReturn_0100,
         TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->isWatermarkMode_ = true;
    surfaceEncoderAdapter_->isStopKeyFramePts_ = false;
    surfaceEncoderAdapter_->recordVideoFrameCount_ = 5;
    uint32_t index = 0;
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    surfaceEncoderAdapter_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
    EXPECT_EQ(surfaceEncoderAdapter_->recordVideoFrameCount_, 5);
}

/**
 * @tc.name: SurfaceEncoderAdapter_GetInputSurface_0100
 * @tc.desc: GetInputSurface with codecServer_=nullptr returns nullptr (L426 true guard),
 *           then with codecServer_ set delegates to CreateInputSurface (L426 false, L427).
 *           codecServer_ ptr identity assertion proves the function did not modify the member.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_GetInputSurface_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = nullptr;
    sptr<Surface> surfaceNull = surfaceEncoderAdapter_->GetInputSurface();
    EXPECT_EQ(surfaceNull, nullptr);
    auto encoder = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecServer_ = encoder;
    sptr<Surface> surfaceFromCodec = surfaceEncoderAdapter_->GetInputSurface();
    EXPECT_EQ(surfaceFromCodec, nullptr);
    EXPECT_EQ(surfaceEncoderAdapter_->codecServer_, encoder);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Pause_QueueBackPause_0100
 * @tc.desc: Pause with pauseResumeQueue_ back=PAUSE — covers L535 false (queue not empty AND
 *           NOT(back==RESUME && time<=pauseTime_)). No push to queue.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Pause_QueueBackPause_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(std::make_pair(100, StateCode::PAUSE));
    size_t sizeBefore = surfaceEncoderAdapter_->pauseResumeQueue_.size();
    Status ret = surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->pauseResumeQueue_.size(), sizeBefore);
    EXPECT_EQ(surfaceEncoderAdapter_->curState_, ProcessStateCode::PAUSED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Pause_QueueBackResumeTimeGt_0100
 * @tc.desc: Pause with pauseResumeQueue_ back=RESUME, time > pauseTime_ — covers L535 false
 *           (back==RESUME but time > pauseTime_ → second cond false → outer false). No push.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Pause_QueueBackResumeTimeGt_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back(
        std::make_pair(std::numeric_limits<int64_t>::max(), StateCode::RESUME));
    size_t sizeBefore = surfaceEncoderAdapter_->pauseResumeQueue_.size();
    Status ret = surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->pauseResumeQueue_.size(), sizeBefore);
    EXPECT_EQ(surfaceEncoderAdapter_->curState_, ProcessStateCode::PAUSED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_PausedFrameRateGt0_0100
 * @tc.desc: Stop in PAUSED state with frameRate>0, enableStabilization_=true, hasReceivedEOS_=true —
 *           covers L486 true (PAUSED), L490 true (frameRate>0 && stabilization &&
 *           currentKeyFramePts_ <= pauseTime_ - SEC_TO_NS/frameRate). HandleWaitforStop returns
 *           immediately (hasReceivedEOS_=true, no wait_for).
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_PausedFrameRateGt0_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::PAUSED;
    surfaceEncoderAdapter_->videoFrameRate_.store(30);
    surfaceEncoderAdapter_->enableStabilization_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 0;
    surfaceEncoderAdapter_->pauseTime_ = 1000000000;
    surfaceEncoderAdapter_->stopTime_ = 1000000000;
    surfaceEncoderAdapter_->hasReceivedEOS_ = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->curState_, ProcessStateCode::STOPPED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_RecordingStabilization_0100
 * @tc.desc: Stop in RECORDING state with enableStabilization_=true, hasReceivedEOS_=true —
 *           covers L499 true (RECORDING), L500 true (enableStabilization_). HandleWaitforStop
 *           returns immediately (hasReceivedEOS_=true, no wait_for).
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_RecordingStabilization_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::RECORDING;
    surfaceEncoderAdapter_->enableStabilization_ = true;
    surfaceEncoderAdapter_->stopTime_ = 1000000000;
    surfaceEncoderAdapter_->hasReceivedEOS_ = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->curState_, ProcessStateCode::STOPPED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetTransCoderMode_0100
 * @tc.desc: SetTransCoderMode sets isTransCoderMode=true — covers L397-402.
 *           Prior state false → post state true proves SetTransCoderMode executed.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetTransCoderMode_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    Status ret = surfaceEncoderAdapter_->SetTransCoderMode();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(surfaceEncoderAdapter_->isTransCoderMode);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetWatermarkMode_0100
 * @tc.desc: SetWatermarkMode sets isWatermarkMode_=true — covers L410-415.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetWatermarkMode_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isWatermarkMode_ = false;
    Status ret = surfaceEncoderAdapter_->SetWatermarkMode();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(surfaceEncoderAdapter_->isWatermarkMode_);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetStabilizationMode_0100
 * @tc.desc: SetStabilizationMode toggles enableStabilization_ — covers L417-422.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetStabilizationMode_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->enableStabilization_ = false;
    Status ret = surfaceEncoderAdapter_->SetStabilizationMode(true);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(surfaceEncoderAdapter_->enableStabilization_);
    ret = surfaceEncoderAdapter_->SetStabilizationMode(false);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(surfaceEncoderAdapter_->enableStabilization_);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureSQRMode_NotSqr_0100
 * @tc.desc: ConfigureSQRMode with non-SQR bitrate mode — covers L282 false (isSqrMode=false),
 *           L293 skipped (no SQR_FACTOR put). L281 VIDEO_FRAME_RATE_ADAPTIVE_MODE always set.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureSQRMode_NotSqr_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    MediaAVCodec::Format format;
    format.PutIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE,
        static_cast<int32_t>(MediaAVCodec::VideoEncodeBitrateMode::VBR));
    surfaceEncoderAdapter_->ConfigureSQRMode(format, meta);
    int32_t sqrFactor = -1;
    EXPECT_FALSE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactor));
    int32_t adaptiveMode = -1;
    EXPECT_TRUE(format.GetIntValue(Tag::VIDEO_FRAME_RATE_ADAPTIVE_MODE, adaptiveMode));
    EXPECT_EQ(adaptiveMode, 1);
}


/**
 * @tc.name: SurfaceEncoderAdapter_GetOutputFormat_0100
 * @tc.desc: GetOutputFormat returns nullptr (not supported)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_GetOutputFormat_0100, TestSize.Level1)
{
    auto encoder = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecServer_ = encoder;
    auto format = surfaceEncoderAdapter_->GetOutputFormat();
    EXPECT_EQ(format, nullptr);
    EXPECT_EQ(surfaceEncoderAdapter_->codecServer_, encoder);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetInputSurface_0100
 * @tc.desc: SetInputSurface returns OK (stub implementation always returns OK)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetInputSurface_0100, TestSize.Level1)
{
    sptr<Surface> surface = nullptr;
    Status ret = surfaceEncoderAdapter_->SetInputSurface(surface);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetFaultEvent_0100
 * @tc.desc: SetFaultEvent(string) writes fault info using stored calling info fields
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetFaultEvent_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecMimeType_ = "";
    surfaceEncoderAdapter_->bundleName_ = "";
    surfaceEncoderAdapter_->instanceId_ = 0;
    surfaceEncoderAdapter_->SetCallingInfo(1001, 2002, "test_bundle", 99999);
    surfaceEncoderAdapter_->SetFaultEvent("test_fault_msg");
    EXPECT_EQ(surfaceEncoderAdapter_->bundleName_, "test_bundle");
    EXPECT_EQ(surfaceEncoderAdapter_->appUid_, 1001);
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetFaultEvent_0200
 * @tc.desc: SetFaultEvent(string, int32_t) appends ret code and delegates to SetFaultEvent(string)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetFaultEvent_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->appUid_ = 0;
    surfaceEncoderAdapter_->bundleName_ = "";
    surfaceEncoderAdapter_->instanceId_ = 0;
    surfaceEncoderAdapter_->SetCallingInfo(1001, 2002, "fault_bundle", 12345);
    surfaceEncoderAdapter_->SetFaultEvent("test_fault_with_code", -1);
    EXPECT_EQ(surfaceEncoderAdapter_->appUid_, 1001);
    EXPECT_EQ(surfaceEncoderAdapter_->bundleName_, "fault_bundle");
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetCallingInfo_0100
 * @tc.desc: SetCallingInfo stores appUid, appPid, bundleName, instanceId
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetCallingInfo_0100, TestSize.Level1)
{
    int32_t expectedUid = 1001;
    int32_t expectedPid = 2002;
    std::string expectedBundle = "com.test.app";
    uint64_t expectedInstanceId = 99999;
    surfaceEncoderAdapter_->appUid_ = 0;
    surfaceEncoderAdapter_->appPid_ = 0;
    surfaceEncoderAdapter_->bundleName_ = "";
    surfaceEncoderAdapter_->instanceId_ = 0;
    surfaceEncoderAdapter_->SetCallingInfo(expectedUid, expectedPid, expectedBundle, expectedInstanceId);
    EXPECT_EQ(surfaceEncoderAdapter_->appUid_, expectedUid);
    EXPECT_EQ(surfaceEncoderAdapter_->appPid_, expectedPid);
    EXPECT_EQ(surfaceEncoderAdapter_->bundleName_, expectedBundle);
    EXPECT_EQ(surfaceEncoderAdapter_->instanceId_, expectedInstanceId);
}

/**
 * @tc.name: SurfaceEncoderAdapter_GetIsTransCoderMode_0100
 * @tc.desc: GetIsTransCoderMode returns false before SetTransCoderMode, true after
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_GetIsTransCoderMode_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    EXPECT_FALSE(surfaceEncoderAdapter_->GetIsTransCoderMode());
    surfaceEncoderAdapter_->SetTransCoderMode();
    EXPECT_TRUE(surfaceEncoderAdapter_->GetIsTransCoderMode());
}

/**
 * @tc.name: SurfaceEncoderAdapter_SetTransCoderCapabilityCheckInfo_0100
 * @tc.desc: SetTransCoderCapabilityCheckInfo stores width and height for resolution check
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_SetTransCoderCapabilityCheckInfo_0100, TestSize.Level1)
{
    int32_t expectedWidth = 1920;
    int32_t expectedHeight = 1080;
    surfaceEncoderAdapter_->transCoderWidth_ = 0;
    surfaceEncoderAdapter_->transCoderHeight_ = 0;
    surfaceEncoderAdapter_->SetTransCoderCapabilityCheckInfo(expectedWidth, expectedHeight);
    EXPECT_EQ(surfaceEncoderAdapter_->transCoderWidth_, expectedWidth);
    EXPECT_EQ(surfaceEncoderAdapter_->transCoderHeight_, expectedHeight);
}

/**
 * @tc.name: SurfaceEncoderAdapter_HandlePtsMapping_NegativeMappingTime_0100
 * @tc.desc: OnInputParameterWithAttrAvailable with startBufferTime_ > adjustPts — covers L912
 *           true branch (mappingTime < 0, log warning). RecordVideoFrameCount increments prove
 *           HandlePtsMapping executed past the warning.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_HandlePtsMapping_NegativeMappingTime_0100,
         TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->isWatermarkMode_ = false;
    surfaceEncoderAdapter_->isStopKeyFramePts_ = false;
    surfaceEncoderAdapter_->startBufferTime_ = 10000;
    surfaceEncoderAdapter_->totalPauseTimeQueue_ = {0};
    surfaceEncoderAdapter_->checkFramesPauseTime_ = 0;
    surfaceEncoderAdapter_->videoFrameRate_.store(0);
    uint32_t index = 0;
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    attribute->PutLongValue(Tag::MEDIA_TIME_STAMP, 100);
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    int64_t recordBefore = surfaceEncoderAdapter_->recordVideoFrameCount_;
    surfaceEncoderAdapter_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
    EXPECT_GT(surfaceEncoderAdapter_->recordVideoFrameCount_, recordBefore);
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnInputParameter_NotifyEos_0100
 * @tc.desc: OnInputParameterWithAttrAvailable triggers NotifyEos when currentPts > stopTime_ -
 *           (SEC_TO_NS / frameRate) — covers L890-896 true branch.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnInputParameter_NotifyEos_0100, TestSize.Level1)
{
    auto countingEncoder = std::make_shared<CountingAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->codecServer_ = countingEncoder;
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->isWatermarkMode_ = false;
    surfaceEncoderAdapter_->isStopKeyFramePts_ = false;
    surfaceEncoderAdapter_->videoFrameRate_.store(30);
    surfaceEncoderAdapter_->stopTime_ = 100;
    surfaceEncoderAdapter_->startBufferTime_ = -1;
    surfaceEncoderAdapter_->totalPauseTimeQueue_ = {0};
    surfaceEncoderAdapter_->checkFramesPauseTime_ = 0;
    uint32_t index = 0;
    std::shared_ptr<Format> attribute = std::make_shared<Format>();
    attribute->PutLongValue(Tag::MEDIA_TIME_STAMP, 200000000);
    std::shared_ptr<Format> parameter = std::make_shared<Format>();
    surfaceEncoderAdapter_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
    EXPECT_EQ(countingEncoder->notifyEosCount_, 1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddPauseResumePts_WithCallback_0100
 * @tc.desc: AddPauseResumePts RESUME branch with non-null callback — covers L1069 true branch
 *           (OnReportFirstFramePts called). keyFramePts_ appended with currentKeyFramePts_/NS_PER_US.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddPauseResumePts_WithCallback_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MyEncoderAdapterKeyFramePtsCallback>();
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1000;
    surfaceEncoderAdapter_->keyFramePts_ = "";
    surfaceEncoderAdapter_->pauseResumePts_.push_back(std::make_pair(100, StateCode::RESUME));
    bool result = surfaceEncoderAdapter_->AddPauseResumePts(100);
    ASSERT_EQ(result, false);
    EXPECT_EQ(surfaceEncoderAdapter_->keyFramePts_, "1,");
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_RecordingNoStabilization_0100
 * @tc.desc: Stop in RECORDING state with enableStabilization_=false — covers L500 false branch
 *           (HandleWaitforStop NOT called, AddStopPts called directly).
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_RecordingNoStabilization_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::RECORDING;
    surfaceEncoderAdapter_->enableStabilization_ = false;
    surfaceEncoderAdapter_->stopTime_ = 1000000000;
    surfaceEncoderAdapter_->hasReceivedEOS_ = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->curState_, ProcessStateCode::STOPPED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_Stop_PausedFrameRateGt0_NoWait_0100
 * @tc.desc: Stop in PAUSED state with frameRate>0, enableStabilization_=true, but
 *           currentKeyFramePts_ > pauseTime_ - SEC_TO_NS/frameRate — covers L491 false branch
 *           (HandleWaitforStop NOT called, stop directly).
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_Stop_PausedFrameRateGt0_NoWait_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::PAUSED;
    surfaceEncoderAdapter_->videoFrameRate_.store(30);
    surfaceEncoderAdapter_->enableStabilization_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1000000000;
    surfaceEncoderAdapter_->pauseTime_ = 1000000000;
    surfaceEncoderAdapter_->stopTime_ = 1000000000;
    surfaceEncoderAdapter_->hasReceivedEOS_ = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(surfaceEncoderAdapter_->curState_, ProcessStateCode::STOPPED);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureGeneralFormat_TypeMismatch_0100
 * @tc.desc: ConfigureGeneralFormat with wrong-type tags — covers L159/L166/L173/L179/L186/L192
 *           false branches (key exists via Find, but Get returns false due to type mismatch)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureGeneralFormat_TypeMismatch_0100,
         TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_WIDTH, std::string("x"));
    meta->SetData(Tag::VIDEO_HEIGHT, std::string("x"));
    meta->SetData(Tag::VIDEO_CAPTURE_RATE, std::string("x"));
    meta->SetData(Tag::MEDIA_BITRATE, std::string("x"));
    meta->SetData(Tag::MIME_TYPE, 42);
    meta->SetData(Tag::VIDEO_H265_PROFILE, std::string("x"));
    surfaceEncoderAdapter_->videoWidth_ = 999;
    surfaceEncoderAdapter_->videoHeight_ = 999;
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    int32_t width = -1;
    EXPECT_FALSE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, width));
    EXPECT_EQ(surfaceEncoderAdapter_->videoWidth_, 999);
    int32_t height = -1;
    EXPECT_FALSE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, height));
    EXPECT_EQ(surfaceEncoderAdapter_->videoHeight_, 999);
    double captureRate = -1.0;
    EXPECT_FALSE(format.GetDoubleValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CAPTURE_RATE, captureRate));
    int64_t bitrate = -1;
    EXPECT_FALSE(format.GetLongValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, bitrate));
    std::string mimeType;
    EXPECT_FALSE(format.GetStringValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType));
    int32_t profile = -1;
    EXPECT_FALSE(format.GetIntValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_PROFILE, profile));
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureFrameRate_TypeMismatch_0100
 * @tc.desc: ConfigureFrameRate with wrong-type VIDEO_FRAME_RATE — covers L202 false branch
 *           (Get returns false → videoFrameRate_ NOT updated, PutDoubleValue writes default 0.0)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureFrameRate_TypeMismatch_0100,
         TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_FRAME_RATE, std::string("x"));
    surfaceEncoderAdapter_->videoFrameRate_.store(42);
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureFrameRate(format, meta);
    EXPECT_EQ(surfaceEncoderAdapter_->videoFrameRate_.load(), 42);
    double frameRate = -1.0;
    EXPECT_TRUE(format.GetDoubleValue(MediaAVCodec::MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate));
    EXPECT_EQ(frameRate, 0.0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureEnableFormat_TypeMismatch_0100
 * @tc.desc: ConfigureEnableFormat with wrong-type VIDEO_ENCODER_ENABLE_WATERMARK — covers L218
 *           false branch (Get returns false → format key NOT written)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureEnableFormat_TypeMismatch_0100,
         TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, std::string("x"));
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureEnableFormat(format, meta);
    int32_t enableWatermark = -1;
    EXPECT_FALSE(format.GetIntValue(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, enableWatermark));
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutRGBA_TypeMismatch_0100
 * @tc.desc: ConfigureAboutRGBA with wrong-type VIDEO_ENCODE_BITRATE_MODE and VIDEO_ENCODER_SQR_FACTOR
 *           — covers L800/L808 false branches (Get returns false → format keys NOT written)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_TypeMismatch_0100,
         TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_ENCODE_BITRATE_MODE, std::string("x"));
    meta->SetData(Tag::VIDEO_ENCODER_SQR_FACTOR, std::string("x"));
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);
    int32_t bitrateMode = -1;
    EXPECT_FALSE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODE_BITRATE_MODE, bitrateMode));
    int32_t sqrFactor = -1;
    EXPECT_FALSE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::MD_KEY_VIDEO_ENCODER_SQR_FACTOR, sqrFactor));
}

/**
 * @tc.name: SurfaceEncoderAdapter_ConfigureAboutEnableTemporalScale_TypeMismatch_0100
 * @tc.desc: ConfigureAboutEnableTemporalScale with wrong-type VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY
 *           — covers L820 true branch (Get returns false → early return → format key NOT written)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest,
    SurfaceEncoderAdapter_ConfigureAboutEnableTemporalScale_TypeMismatch_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, std::string("x"));
    MediaAVCodec::Format format;
    surfaceEncoderAdapter_->ConfigureAboutEnableTemporalScale(format, meta);
    int32_t temporalScale = -1;
    EXPECT_FALSE(format.GetIntValue(
        MediaAVCodec::MediaDescriptionKey::OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY,
        temporalScale));
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS