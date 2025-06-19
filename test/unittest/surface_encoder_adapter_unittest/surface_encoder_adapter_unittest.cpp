/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "surface_encoder_adapter_unittest.h"
#include "mock/mock_avbuffer_queue_producer.h"
#include "mock/mock_encoder_adapter_key_frame_pts_callback.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media;

const static int32_t TIMES_TWO = 2;
const static int32_t TEST_ERR_CODE = -1;
const static int32_t TEST_SUCCESS_CODE = 0;
const static int64_t TEST_PAUSE_TIME = -2;
const static int64_t TEST_PAUSE_TIME_DEFAULT = -1;
const static int64_t TEST_CURRENT_PTS = 1;
const static uint32_t TEST_INDEX = 1;

void SurfaceEncoderAdapterUnitTest::SetUpTestCase(void) {}

void SurfaceEncoderAdapterUnitTest::TearDownTestCase(void) {}

void SurfaceEncoderAdapterUnitTest::SetUp(void)
{
    mockCodecServer_ = std::make_shared<MockAVCodecVideoEncoder>();
    surfaceEncoderAdapter_ = std::make_shared<SurfaceEncoderAdapter>();
}

void SurfaceEncoderAdapterUnitTest::TearDown(void)
{
    mockCodecServer_ = nullptr;
    surfaceEncoderAdapter_ = nullptr;
}

/**
 * @tc.name  : Test SetInputSurface
 * @tc.number: SetInputSurface_001
 * @tc.desc  : Test SetInputSurface codecServer_ != nullptr
 *             Test SetInputSurface codecServerPtr->SetInputSurface == 0
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SetInputSurface_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = mockCodecServer_;
    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(Return(TEST_SUCCESS_CODE));
    Status status = surfaceEncoderAdapter_->SetInputSurface(nullptr);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test Pause
 * @tc.number: Pause_001
 * @tc.desc  : Test Pause !pauseResumeQueue_.empty()
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, Pause_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back({TEST_PAUSE_TIME, StateCode::PAUSE});
    surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(surfaceEncoderAdapter_->pauseResumeQueue_.back().second, StateCode::PAUSE);
}

/**
 * @tc.name  : Test Pause
 * @tc.number: Pause_002
 * @tc.desc  : Test Pause pauseResumeQueue_.back().second == StateCode::RESUME
 *             && pauseResumeQueue_.back().first <= pauseTime_
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, Pause_002, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back({TEST_PAUSE_TIME, StateCode::RESUME});
    surfaceEncoderAdapter_->Pause();
    EXPECT_EQ(surfaceEncoderAdapter_->pauseResumeQueue_.back().second, StateCode::RESUME);
}

/**
 * @tc.name  : Test Resume
 * @tc.number: Resume_001
 * @tc.desc  : Test Resume pauseResumeQueue_.back().second != StateCode::RESUME
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, Resume_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back({TEST_PAUSE_TIME, StateCode::PAUSE});
    surfaceEncoderAdapter_->Resume();
    EXPECT_EQ(surfaceEncoderAdapter_->pauseResumeQueue_.back().first, TEST_PAUSE_TIME);
}

/**
 * @tc.name  : Test Resume
 * @tc.number: Resume_002
 * @tc.desc  : Test Resume pauseTime_ == -1
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, Resume_002, TestSize.Level1)
{
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->pauseTime_ = TEST_PAUSE_TIME_DEFAULT;
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back({TEST_PAUSE_TIME, StateCode::PAUSE});
    surfaceEncoderAdapter_->Resume();
    EXPECT_EQ(surfaceEncoderAdapter_->totalPauseTimeQueue_.size(), 1);
}

/**
 * @tc.name  : Test Flush
 * @tc.number: Flush_001
 * @tc.desc  : Test Flush codecServer_->Flush() != 0
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, Flush_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = mockCodecServer_;
    EXPECT_CALL(*(mockCodecServer_), Flush()).WillRepeatedly(Return(TEST_ERR_CODE));
    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(Return(TEST_SUCCESS_CODE));
    Status status = surfaceEncoderAdapter_->Flush();
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test Reset
 * @tc.number: Reset_001
 * @tc.desc  : Test Reset codecServer_->Reset() != 0
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, Reset_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = mockCodecServer_;
    EXPECT_CALL(*(mockCodecServer_), Reset()).WillRepeatedly(Return(TEST_ERR_CODE));
    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(Return(TEST_SUCCESS_CODE));
    Status status = surfaceEncoderAdapter_->Reset();
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test Release
 * @tc.number: Release_001
 * @tc.desc  : Test Release codecServer_->Release() != 0
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, Release_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = mockCodecServer_;
    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(Return(TEST_ERR_CODE));
    Status status = surfaceEncoderAdapter_->Release();
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test NotifyEos
 * @tc.number: NotifyEos_001
 * @tc.desc  : Test NotifyEos codecServer_->NotifyEos() != 0
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, NotifyEos_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = mockCodecServer_;
    EXPECT_CALL(*(mockCodecServer_), NotifyEos()).WillRepeatedly(Return(TEST_ERR_CODE));
    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(Return(TEST_SUCCESS_CODE));
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->currentPts_ = UINT32_MAX;
    Status status = surfaceEncoderAdapter_->NotifyEos(UINT32_MAX);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test SetParameter
 * @tc.number: SetParameter_001
 * @tc.desc  : Test SetParameter codecServer_->SetParameter() != 0
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, SetParameter_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->codecServer_ = mockCodecServer_;
    EXPECT_CALL(*(mockCodecServer_), SetParameter(_)).WillRepeatedly(Return(TEST_ERR_CODE));
    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(Return(TEST_SUCCESS_CODE));
    Status status = surfaceEncoderAdapter_->SetParameter(nullptr);
    EXPECT_EQ(status, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test TransCoderOnOutputBufferAvailable
 * @tc.number: TransCoderOnOutputBufferAvailable_001
 * @tc.desc  : Test TransCoderOnOutputBufferAvailable outputBufferQueueProducer_->RequestBuffer != Status::OK
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, TransCoderOnOutputBufferAvailable_001, TestSize.Level1)
{
    AVBufferConfig config;
    config.memoryType = MemoryType::VIRTUAL_MEMORY;
    auto avbuffer = AVBuffer::CreateAVBuffer(config);
    auto mockProducer = std::make_shared<MockAVBufferQueueProducer>();
    EXPECT_CALL(*(mockProducer), RequestBuffer(_, _, _)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    surfaceEncoderAdapter_->outputBufferQueueProducer_ = OHOS::sptr<AVBufferQueueProducer>(mockProducer.get());
    surfaceEncoderAdapter_->TransCoderOnOutputBufferAvailable(TEST_INDEX, avbuffer);
    EXPECT_TRUE(surfaceEncoderAdapter_->indexs_.empty());
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_001
 * @tc.desc  : Test OnOutputBufferAvailable (stopTime_ != -1 && buffer->pts_ >
 *             stopTime_ - (SEC_TO_NS / videoFrameRate_)) == true
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, OnOutputBufferAvailable_001, TestSize.Level1)
{
    auto avbuffer = AVBuffer::CreateAVBuffer();
    avbuffer->pts_ = UINT32_MAX;
    surfaceEncoderAdapter_->stopTime_ = 1;
    surfaceEncoderAdapter_->isTransCoderMode = false;
    surfaceEncoderAdapter_->OnOutputBufferAvailable(TEST_INDEX, avbuffer);
    EXPECT_TRUE(surfaceEncoderAdapter_->indexs_.empty());
}

/**
 * @tc.name  : Test CheckFrames
 * @tc.number: CheckFrames_001
 * @tc.desc  : Test CheckFrames totalPauseTimeQueue_.empty() == true
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, CheckFrames_001, TestSize.Level1)
{
    surfaceEncoderAdapter_->pauseResumeQueue_.push_back({TEST_PAUSE_TIME, StateCode::RESUME});
    surfaceEncoderAdapter_->totalPauseTimeQueue_.clear();
    int64_t pauseTime = static_cast<int64_t>(TEST_PAUSE_TIME_DEFAULT);
    bool ret = surfaceEncoderAdapter_->CheckFrames(TEST_PAUSE_TIME_DEFAULT, pauseTime);
    EXPECT_TRUE(!ret);
}

/**
 * @tc.name  : Test AddStartPts
 * @tc.number: AddStartPts_001
 * @tc.desc  : Test AddStartPts encoderAdapterKeyFramePtsCallback_ != nullptr
 *             Test AddStopPts encoderAdapterKeyFramePtsCallback_ == nullptr
 *             Test AddPauseResumePts encoderAdapterKeyFramePtsCallback_ != nullptr
 */
HWTEST_F(SurfaceEncoderAdapterUnitTest, AddStartPts_001, TestSize.Level1)
{
    // Test AddStartPts encoderAdapterKeyFramePtsCallback_ != nullptr
    surfaceEncoderAdapter_->isStartKeyFramePts_ = true;
    auto callback = std::make_shared<MockEncoderAdapterKeyFramePtsCallback>();
    EXPECT_CALL(*(callback.get()), OnReportFirstFramePts(_)).Times(TIMES_TWO);
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ = callback;
    surfaceEncoderAdapter_->AddStartPts(TEST_CURRENT_PTS);

    // Test AddStopPts encoderAdapterKeyFramePtsCallback_ == nullptr
    surfaceEncoderAdapter_->isStopKeyFramePts_ = true;
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ = nullptr;
    surfaceEncoderAdapter_->AddStopPts();

    // Test AddPauseResumePts encoderAdapterKeyFramePtsCallback_ != nullptr
    surfaceEncoderAdapter_->pauseResumePts_.push_back({TEST_PAUSE_TIME, StateCode::RESUME});
    surfaceEncoderAdapter_->encoderAdapterKeyFramePtsCallback_ = callback;
    bool ret = surfaceEncoderAdapter_->AddPauseResumePts(TEST_PAUSE_TIME_DEFAULT);
    EXPECT_TRUE(!ret);
}