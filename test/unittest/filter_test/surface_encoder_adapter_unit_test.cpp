/*
 * Copyright (C) 2024-2025 Huawei Device Co., Ltd.
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
    surfaceEncoderAdapter_->isStart_ = true;
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->releaseBufferTask_ = nullptr;
    surfaceEncoderAdapter_->encoderAdapterCallback_ = std::make_shared<MyEncoderAdapterCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("test");
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    ret = surfaceEncoderAdapter_->Stop();
    surfaceEncoderAdapter_->isStart_ = true;
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
    surfaceEncoderAdapter_->isStart_ = true;
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::RECORDING;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isStart_ = true;
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
    surfaceEncoderAdapter_->isStart_ = true;
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->curState_ = ProcessStateCode::PAUSED;
    // current frame is not the last frame before the pasue time, wait for stop
    surfaceEncoderAdapter_->currentKeyFramePts_ = 5000000000000;
    surfaceEncoderAdapter_->pauseTime_ = 6000000000000;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ =
        std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    Status ret = surfaceEncoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::OK);
    surfaceEncoderAdapter_->isStart_ = true;
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
    surfaceEncoderAdapter_->ReleaseBuffer();
    EXPECT_EQ(surfaceEncoderAdapter_->startBufferTime_, -1);
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
    meta->SetData(Tag::VIDEO_ENCODE_BITRATE_MODE, 2);
    surfaceEncoderAdapter_->ConfigureAboutRGBA(format, meta);
    EXPECT_NE(meta->Find(Tag::VIDEO_PIXEL_FORMAT), meta->end());
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
 * @tc.desc: ConfigureAboutRGBA with VIDEO_ENCODER_SQR_FACTOR boundary value 0 in Meta
 *           Covers: branch I (Meta has SQR_FACTOR=0 → Format conversion preserves 0)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0300, TestSize.Level1)
{
    // 场景：Meta 中设置 SQR_FACTOR 边界值 0；预期 Format 中写入 0
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
 * @tc.desc: ConfigureAboutRGBA with VIDEO_ENCODER_SQR_FACTOR boundary value 51 in Meta
 *           Covers: branch I (Meta has SQR_FACTOR=51 → Format conversion preserves 51)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_ConfigureAboutRGBA_SqrFactor_0400, TestSize.Level1)
{
    // 场景：Meta 中设置 SQR_FACTOR 边界值 51；预期 Format 中写入 51
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
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 0);
    surfaceEncoderAdapter_->ConfigureAboutEnableTemporalScale(format, meta);
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 2);
    surfaceEncoderAdapter_->ConfigureAboutEnableTemporalScale(format, meta);
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
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
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
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
 * @tc.name: SurfaceEncoderAdapter_TransCoder_100
 * @tc.desc: TransCoder
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_TransCoder_100, TestSize.Level1)
{
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    surfaceEncoderAdapter_->outputBufferQueueProducer_ =
                                new OHOS::Media::Pipeline::SurfaceEncoderAdapterUnitTestAP();
    uint32_t index = 1;
    surfaceEncoderAdapter_->stopTime_ = 0;
    buffer->pts_ = 1;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->startBufferTime_ = -1;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->isResume_ = false;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->isResume_ = true;
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->isResume_, buffer->pts_);
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
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->isResume_ = true;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->isTransCoderMode = true;
    surfaceEncoderAdapter_->startBufferTime_ = -1;
    buffer->pts_ = 1;
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->startBufferTime_, -1);
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
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    buffer->pts_ = 1;
    surfaceEncoderAdapter_->eosPts_ = 0;
    surfaceEncoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoEncoder>();
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(surfaceEncoderAdapter_->startBufferTime_, -1);
}

/**
 * @tc.name: SurfaceEncoderAdapter_OnOutputBufferAvailable_0100
 * @tc.desc: OnOutputBufferAvailable
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_OnOutputBufferAvailable_0100, TestSize.Level1)
{
    surfaceEncoderAdapter_->outputBufferQueueProducer_ =
                                new OHOS::Media::Pipeline::SurfaceEncoderAdapterUnitTestAP();
    uint32_t index = 1;
    surfaceEncoderAdapter_->isTransCoderMode = true;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->stopTime_ = 1;
    buffer->pts_ = 0;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->stopTime_ = -1;
    buffer->pts_ = 0;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    surfaceEncoderAdapter_->stopTime_ = 1;
    buffer->pts_ = 2;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(index, buffer);
    EXPECT_NE(surfaceEncoderAdapter_->startBufferTime_, buffer->pts_);
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
    meta->SetData(Tag::VIDEO_HEIGHT, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    meta->SetData(Tag::VIDEO_CAPTURE_RATE, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    meta->SetData(Tag::MEDIA_BITRATE, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    meta->SetData(Tag::VIDEO_FRAME_RATE, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    meta->SetData(Tag::MIME_TYPE, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    meta->SetData(Tag::VIDEO_H265_PROFILE, 1);
    surfaceEncoderAdapter_->ConfigureGeneralFormat(format, meta);
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
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
    surfaceEncoderAdapter_->ConfigureEnableFormat(format, meta);
    meta->SetData(Tag::VIDEO_ENCODER_ENABLE_WATERMARK, 1);
    surfaceEncoderAdapter_->ConfigureEnableFormat(format, meta);
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
}

/**
 * @tc.name: SurfaceEncoderAdapter_AddStartPts_0100
 * @tc.desc: AddStartPts
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_AddStartPts_0200, TestSize.Level1)
{
    surfaceEncoderAdapter_->isStartKeyFramePts_ = false;
    surfaceEncoderAdapter_->AddStartPts(0);
    surfaceEncoderAdapter_->isStartKeyFramePts_ = true;
    surfaceEncoderAdapter_->AddStartPts(0);
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
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
    surfaceEncoderAdapter_->isStartKeyFramePts_ = false;
    surfaceEncoderAdapter_->AddStopPts();
    surfaceEncoderAdapter_->isStartKeyFramePts_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 0;
    surfaceEncoderAdapter_->stopTime_ = 0;
    surfaceEncoderAdapter_->AddStopPts();
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1;
    surfaceEncoderAdapter_->stopTime_ = 0;
    surfaceEncoderAdapter_->AddStopPts();
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
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
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->currentKeyFramePts_ = 1;
    surfaceEncoderAdapter_->stopTime_ = 0;
    surfaceEncoderAdapter_->AddStopPts();
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTime_, 0);
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
 * @tc.name: SurfaceEncoderAdapter_GetOutputFormat_0100
 * @tc.desc: GetOutputFormat
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SurfaceEncoderAdapter_GetOutputFormat_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> testMeta = surfaceEncoderAdapter_->GetOutputFormat();
    ASSERT_EQ(testMeta, nullptr);
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
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    // shouldDiscard=false, PER_FRAME_DISCARD should be 0
    int32_t discardValue = 1;
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->HandleTranscoderMode(0, nullAttribute, parameter);
    // attribute is null → shouldDiscard=false
    int32_t discardValue = 1;
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    // shouldDiscard=true, PER_FRAME_DISCARD should be 1
    int32_t discardValue = 0;
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 1);
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    // shouldDiscard=false, IDR requested for first non-discard frame
    int32_t discardValue = 1;
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
    int32_t idrValue = 0;
    parameter->GetIntValue(Tag::VIDEO_REQUEST_I_FRAME, idrValue);
    EXPECT_EQ(idrValue, 1);
    EXPECT_TRUE(surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.load());
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(false);
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->HandleTranscoderMode(0, attribute, parameter);
    // shouldDiscard=false, IDR NOT requested (firstNonDiscardFrameProcessed_ already true)
    int32_t discardValue = 1;
    parameter->GetIntValue(Tag::VIDEO_ENCODER_PER_FRAME_DISCARD, discardValue);
    EXPECT_EQ(discardValue, 0);
    // VIDEO_REQUEST_I_FRAME should not be set (default 0)
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(false);
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    EXPECT_TRUE(result); // discard
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
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
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(false);
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
    // IDR not requested — VIDEO_REQUEST_I_FRAME not set
    surfaceEncoderAdapter_->clipStartRelativeUs_ = -1;
    surfaceEncoderAdapter_->firstNonDiscardFrameProcessed_.store(false);
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
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->isTransCoderMode = false;
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
    surfaceEncoderAdapter_->isTransCoderMode = false;
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS