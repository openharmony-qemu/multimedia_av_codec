/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "decoder_surface_filter_unit_test.h"

#include "gmock/gmock.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;
using namespace MediaAVCodec;

namespace OHOS {
namespace Media {
namespace Pipeline {
class AVCodecList {
public:
    std::string FindDecoder(const Format &format)
    {
        return "VideoDecoderMock";
    };
};

class MockEventReceiver : public EventReceiver {
public:
    MockEventReceiver() = default;
    void OnEvent(const Event &event){};
};

void DecoderSurfaceFilterUnitTest::SetUpTestCase(void) {}

void DecoderSurfaceFilterUnitTest::TearDownTestCase(void) {}

void DecoderSurfaceFilterUnitTest::SetUp(void)
{
    decoderSurfaceFilter_ = GenerateCommonFilter();
    EXPECT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_NE(decoderSurfaceFilter_->meta_, nullptr);
    EXPECT_NE(decoderSurfaceFilter_->eventReceiver_, nullptr);
}

void DecoderSurfaceFilterUnitTest::TearDown(void)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->Stop();
    decoderSurfaceFilter_ = nullptr;
}

std::shared_ptr<DecoderSurfaceFilter> DecoderSurfaceFilterUnitTest::GenerateCommonFilter()
{
    auto decoderSurfaceFilter =
        std::make_shared<DecoderSurfaceFilter>("DecoderSurfaceFilterUnitTest", FilterType::FILTERTYPE_VIDEODEC);
    auto eventReceiver = std::make_shared<MockEventReceiver>();
    decoderSurfaceFilter->eventReceiver_ = eventReceiver;
    auto meta = std::make_shared<Meta>();
    decoderSurfaceFilter->meta_ = meta;
    decoderSurfaceFilter->LinkPipeLine("DecoderSurfaceFilterUnitTest");
    return decoderSurfaceFilter;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, FilterMediaCodecCallback_Functions_test, TestSize.Level1)
{
    FilterMediaCodecCallback callback(decoderSurfaceFilter_);

    callback.OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 0);
    const Format format;
    callback.OnOutputFormatChanged(format);
    callback.OnInputBufferAvailable(0, nullptr);

    std::shared_ptr<AVBuffer> targetAvBuffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(targetAvBuffer, nullptr);
    callback.OnOutputBufferAvailable(0, targetAvBuffer);

    callback.decoderSurfaceFilter_.reset();

    callback.OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 0);
    callback.OnOutputFormatChanged(format);
    callback.OnInputBufferAvailable(0, nullptr);

    ASSERT_NE(targetAvBuffer, nullptr);
    callback.OnOutputBufferAvailable(0, targetAvBuffer);

    decoderSurfaceFilter_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 0);

    std::shared_ptr<VideoDecoderAdapter> videoDecoderAdapter = std::make_shared<VideoDecoderAdapter>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderAdapter;
    EXPECT_CALL(*videoDecoderAdapter, Configure(_)).WillRepeatedly(Return(Status::OK));
    const std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    auto configureRes = decoderSurfaceFilter_->Configure(parameter);

    decoderSurfaceFilter_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 0);
    ASSERT_EQ(configureRes, Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoInitAfterLink, TestSize.Level1)
{
    decoderSurfaceFilter_->videoDecoder_ = nullptr;
    auto linkRes = decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_EQ(linkRes, Status::ERROR_UNKNOWN);

    std::shared_ptr<VideoDecoderAdapter> videoDecoderAdapter = std::make_shared<VideoDecoderAdapter>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderAdapter;
    linkRes = decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_NE(linkRes, Status::ERROR_UNKNOWN);

    decoderSurfaceFilter_->svpFlag_ = true;
    decoderSurfaceFilter_->isDrmProtected_ = false;
    linkRes = decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_NE(linkRes, Status::ERROR_UNKNOWN);

    decoderSurfaceFilter_->svpFlag_ = false;
    linkRes = decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_NE(linkRes, Status::ERROR_UNKNOWN);

    decoderSurfaceFilter_->svpFlag_ = true;
    decoderSurfaceFilter_->isDrmProtected_ = true;
    EXPECT_CALL(*videoDecoderAdapter, Init(_, _, _)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    linkRes = decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_NE(linkRes, Status::ERROR_UNKNOWN);

    decoderSurfaceFilter_->eventReceiver_ = std::make_shared<MockEventReceiver>();
    linkRes = decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_NE(linkRes, Status::ERROR_UNKNOWN);

    EXPECT_CALL(*videoDecoderAdapter, Init(_, _, _)).WillRepeatedly(Return(Status::OK));
    linkRes = decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_NE(linkRes, Status::ERROR_UNKNOWN);

    EXPECT_CALL(*videoDecoderAdapter, Configure(_)).WillOnce(Return(Status::OK));
    const std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    auto configureRes = decoderSurfaceFilter_->Configure(parameter);
    ASSERT_EQ(configureRes, Status::OK);

    decoderSurfaceFilter_->DoInitAfterLink();
    ASSERT_NE(linkRes, Status::ERROR_UNKNOWN);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoPreroll, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoDecoderAdapter = std::make_shared<VideoDecoderAdapter>();
    EXPECT_CALL(*videoDecoderAdapter, Start()).WillRepeatedly(Return(Status::OK));
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderAdapter;
    ASSERT_NE(decoderSurfaceFilter_->videoDecoder_, nullptr);

    decoderSurfaceFilter_->isPaused_ = false;
    auto preRollRes = decoderSurfaceFilter_->DoPreroll();
    ASSERT_EQ(preRollRes, Status::OK);

    decoderSurfaceFilter_->isPaused_ = true;
    preRollRes = decoderSurfaceFilter_->DoPreroll();
    ASSERT_EQ(preRollRes, Status::OK);

    EXPECT_CALL(*videoDecoderAdapter, Start()).WillRepeatedly(Return(Status::ERROR_UNKNOWN));

    decoderSurfaceFilter_->isPaused_ = false;
    preRollRes = decoderSurfaceFilter_->DoPreroll();
    ASSERT_EQ(preRollRes, Status::OK);

    decoderSurfaceFilter_->isPaused_ = true;
    preRollRes = decoderSurfaceFilter_->DoPreroll();
    ASSERT_EQ(preRollRes, Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoWaitPrerollDone, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoDecoderAdapter = std::make_shared<VideoDecoderAdapter>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderAdapter;
    ASSERT_NE(decoderSurfaceFilter_->videoDecoder_, nullptr);

    decoderSurfaceFilter_->prerollDone_ = false;
    decoderSurfaceFilter_->isInterruptNeeded_ = false;
    auto prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->prerollDone_ = false;
    decoderSurfaceFilter_->isInterruptNeeded_ = true;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->prerollDone_ = true;
    decoderSurfaceFilter_->isInterruptNeeded_ = false;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->prerollDone_ = true;
    decoderSurfaceFilter_->isInterruptNeeded_ = true;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->prerollDone_ = true;
    decoderSurfaceFilter_->isInterruptNeeded_ = true;
    decoderSurfaceFilter_->eosNext_ = true;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(true);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->eosNext_ = false;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(true);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->eosNext_ = false;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->eosNext_ = true;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(buffer, nullptr);
    decoderSurfaceFilter_->outputBuffers_.push_back(make_pair(0, buffer));

    decoderSurfaceFilter_->eosNext_ = true;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(true);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->eosNext_ = false;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(true);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->eosNext_ = false;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);

    decoderSurfaceFilter_->eosNext_ = true;
    prerollDoneRes = decoderSurfaceFilter_->DoWaitPrerollDone(false);
    ASSERT_EQ(prerollDoneRes, Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_MediaCodecCallback, TestSize.Level1)
{
    std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilterTmp = nullptr;
    auto filterMediaCodecCallback = std::make_shared<FilterMediaCodecCallback>(decoderSurfaceFilterTmp);
    filterMediaCodecCallback->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 0);

    Format format;
    filterMediaCodecCallback->OnOutputFormatChanged(format);

    auto buffer = AVBuffer::CreateAVBuffer();
    filterMediaCodecCallback->OnOutputBufferAvailable(0, buffer);

    filterMediaCodecCallback->decoderSurfaceFilter_ = decoderSurfaceFilter_;

    filterMediaCodecCallback->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 0);
    filterMediaCodecCallback->OnOutputFormatChanged(format);
    filterMediaCodecCallback->OnOutputBufferAvailable(0, buffer);
    EXPECT_FALSE(decoderSurfaceFilter_->outputBuffers_.empty());
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetParameter, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    decoderSurfaceFilter_->SetParameter(meta);

    meta->Set<Tag::VIDEO_SCALE_TYPE>(0);
    decoderSurfaceFilter_->SetParameter(meta);

    meta->Set<Tag::Tag::VIDEO_FRAME_RATE>(20);
    decoderSurfaceFilter_->SetParameter(meta);

    meta->Set<Tag::Tag::VIDEO_FRAME_RATE>(-1);
    decoderSurfaceFilter_->SetParameter(meta);

    decoderSurfaceFilter_->configFormat_.PutDoubleValue(Tag::VIDEO_FRAME_RATE, 20);
    decoderSurfaceFilter_->SetParameter(meta);

    std::shared_ptr<VideoDecoderAdapter> videoDecoderAdapter = std::make_shared<VideoDecoderAdapter>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderAdapter;

    EXPECT_CALL(*videoDecoderAdapter, SetParameter).WillRepeatedly(Return(MediaAVCodec::AVCS_ERR_INVALID_STATE));

    decoderSurfaceFilter_->SetParameter(meta);
    decoderSurfaceFilter_->isThreadExit_ = true;
    decoderSurfaceFilter_->SetParameter(meta);

    auto stopRes = decoderSurfaceFilter_->Stop();
    ASSERT_EQ(stopRes, Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_AcquireNextRenderBuffer, TestSize.Level1)
{
    uint32_t index = 0u;
    int64_t renderTime = 1000;
    auto buffer = AVBuffer::CreateAVBuffer();
    bool acquireRes = decoderSurfaceFilter_->AcquireNextRenderBuffer(true, index, buffer, renderTime);
    EXPECT_EQ(acquireRes, false);

    acquireRes = decoderSurfaceFilter_->AcquireNextRenderBuffer(false, index, buffer, renderTime);
    EXPECT_EQ(acquireRes, false);

    decoderSurfaceFilter_->isFirstFrameAfterResume_ = false;
    decoderSurfaceFilter_->outputBuffers_.push_back(std::make_pair(0, buffer));
    acquireRes = decoderSurfaceFilter_->AcquireNextRenderBuffer(false, index, buffer, renderTime);
    EXPECT_EQ(acquireRes, true);

    decoderSurfaceFilter_->isFirstFrameAfterResume_ = true;
    decoderSurfaceFilter_->outputBuffers_.push_back(std::make_pair(0, buffer));
    acquireRes = decoderSurfaceFilter_->AcquireNextRenderBuffer(false, index, buffer, renderTime);
    EXPECT_EQ(acquireRes, true);

    auto buffer1 = AVBuffer::CreateAVBuffer();
    EXPECT_NE(buffer1, nullptr);
    decoderSurfaceFilter_->isFirstFrameAfterResume_ = false;
    decoderSurfaceFilter_->outputBuffers_.push_back(std::make_pair(0, buffer));
    decoderSurfaceFilter_->outputBuffers_.push_back(std::make_pair(1, buffer1));
    acquireRes = decoderSurfaceFilter_->AcquireNextRenderBuffer(false, index, buffer, renderTime);
    EXPECT_EQ(acquireRes, true);

    auto stopRes = decoderSurfaceFilter_->Stop();
    ASSERT_EQ(stopRes, Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_ReleaseOutputBuffer, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoDecoderAdapter = std::make_shared<VideoDecoderAdapter>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderAdapter;
    std::shared_ptr<VideoSink> videoSink = std::make_shared<VideoSink>();
    decoderSurfaceFilter_->videoSink_ = videoSink;
    auto buffer = AVBuffer::CreateAVBuffer();

    decoderSurfaceFilter_->isRenderStarted_ = false;
    decoderSurfaceFilter_->isInSeekContinous_ = false;
    buffer->flag_ = 0;
    decoderSurfaceFilter_->playRangeEndTime_ = 1;
    buffer->pts_ = 2000;
    Status ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, buffer, 0L);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->isRenderStarted_, true);

    decoderSurfaceFilter_->isRenderStarted_ = true;
    decoderSurfaceFilter_->isInSeekContinous_ = false;
    buffer->flag_ |= static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
    decoderSurfaceFilter_->playRangeEndTime_ = -1;
    decoderSurfaceFilter_->lastRenderTimeNs_ = HST_TIME_NONE;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, buffer, 1000L);
    EXPECT_EQ(decoderSurfaceFilter_->seekTimeUs_, 0);
    EXPECT_EQ(ret, Status::OK);

    decoderSurfaceFilter_->lastRenderTimeNs_ = 900L;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, buffer, 1000L);
    EXPECT_EQ(ret, Status::OK);

    buffer->flag_ = 0;
    buffer->pts_ = -1;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, buffer, 1000L);
    EXPECT_EQ(ret, Status::OK);

    buffer->pts_ = 2000;
    decoderSurfaceFilter_->lastRenderTimeNs_ = 900L;
    decoderSurfaceFilter_->enableRenderAtTimeDfx_ = true;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, buffer, 1000L);
    EXPECT_EQ(ret, Status::OK);

    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, buffer, 0L);
    EXPECT_EQ(ret, Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_IsPostProcessorSupported_None, TestSize.Level1)
{
    decoderSurfaceFilter_->postProcessorType_ = VideoPostProcessorType::NONE;
    EXPECT_EQ(decoderSurfaceFilter_->IsPostProcessorSupported(), false);
}

#ifdef USE_VIDEO_PROCESSING_ENGINE
HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_IsPostProcessorSupported_SuperResolution, TestSize.Level1)
{
    decoderSurfaceFilter_->postProcessorType_ = VideoPostProcessorType::SUPER_RESOLUTION;

    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_WIDTH, 1280);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_HEIGHT, 720);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    decoderSurfaceFilter_->meta_->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(decoderSurfaceFilter_->IsPostProcessorSupported(), true);

    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_WIDTH, 1920);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_HEIGHT, 1080);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    decoderSurfaceFilter_->meta_->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(decoderSurfaceFilter_->IsPostProcessorSupported(), true);

    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_WIDTH, 0);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_HEIGHT, 0);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    decoderSurfaceFilter_->meta_->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(decoderSurfaceFilter_->IsPostProcessorSupported(), false);

    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_WIDTH, 720);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_HEIGHT, 480);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_IS_HDR_VIVID, true);
    decoderSurfaceFilter_->meta_->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(decoderSurfaceFilter_->IsPostProcessorSupported(), false);

    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_WIDTH, 720);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_HEIGHT, 480);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    decoderSurfaceFilter_->meta_->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, true);
    EXPECT_EQ(decoderSurfaceFilter_->IsPostProcessorSupported(), false);
}
#endif

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetPostProcessorOn, TestSize.Level1)
{
    decoderSurfaceFilter_->postProcessorType_ = VideoPostProcessorType::SUPER_RESOLUTION;

    decoderSurfaceFilter_->isPostProcessorSupported_ = false;
    EXPECT_EQ(decoderSurfaceFilter_->SetPostProcessorOn(true), Status::ERROR_UNSUPPORTED_FORMAT);

    decoderSurfaceFilter_->isPostProcessorSupported_ = true;
    EXPECT_EQ(decoderSurfaceFilter_->SetPostProcessorOn(true), Status::OK);

    std::shared_ptr<BaseVideoPostProcessor> postProcessor = std::make_shared<BaseVideoPostProcessor>();
    decoderSurfaceFilter_->postProcessor_ = postProcessor;
    EXPECT_CALL(*postProcessor, SetPostProcessorOn(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_EQ(decoderSurfaceFilter_->SetPostProcessorOn(true), Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetVideoWindowSize, TestSize.Level1)
{
    decoderSurfaceFilter_->postProcessorType_ = VideoPostProcessorType::SUPER_RESOLUTION;
    int32_t width = 1080;
    int32_t height = 720;

    decoderSurfaceFilter_->isPostProcessorSupported_ = false;
    EXPECT_EQ(decoderSurfaceFilter_->SetVideoWindowSize(width, height), Status::ERROR_UNSUPPORTED_FORMAT);

    decoderSurfaceFilter_->isPostProcessorSupported_ = true;
    EXPECT_EQ(decoderSurfaceFilter_->SetVideoWindowSize(width, height), Status::OK);

    std::shared_ptr<BaseVideoPostProcessor> postProcessor = std::make_shared<BaseVideoPostProcessor>();
    decoderSurfaceFilter_->postProcessor_ = postProcessor;
    EXPECT_CALL(*postProcessor, SetVideoWindowSize(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_EQ(decoderSurfaceFilter_->SetVideoWindowSize(width, height), Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetVideoSurface, TestSize.Level1)
{
    decoderSurfaceFilter_->videoDecoder_ = nullptr;
    decoderSurfaceFilter_->postProcessor_ = nullptr;
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer("TestSurface");
    Status ret = Status::OK;

    ret = decoderSurfaceFilter_->SetVideoSurface(surface);
    EXPECT_EQ(ret, Status::OK);

    std::shared_ptr<VideoDecoderAdapter> videoDecoderAdapter = std::make_shared<VideoDecoderAdapter>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderAdapter;
    EXPECT_CALL(*videoDecoderAdapter, SetOutputSurface(_)).WillRepeatedly(Return(MediaAVCodec::AVCS_ERR_INVALID_STATE));
    ret = decoderSurfaceFilter_->SetVideoSurface(surface);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    EXPECT_CALL(*videoDecoderAdapter, SetOutputSurface(_)).WillRepeatedly(Return(MediaAVCodec::AVCS_ERR_OK));
    ret = decoderSurfaceFilter_->SetVideoSurface(surface);
    EXPECT_EQ(ret, Status::OK);

    std::shared_ptr<BaseVideoPostProcessor> postProcessor = std::make_shared<BaseVideoPostProcessor>();
    decoderSurfaceFilter_->postProcessor_ = postProcessor;
    EXPECT_CALL(*postProcessor, SetOutputSurface(_)).WillRepeatedly(Return(Status::OK));
    ret = decoderSurfaceFilter_->SetVideoSurface(surface);
    EXPECT_EQ(ret, Status::OK);

    EXPECT_CALL(*postProcessor, SetOutputSurface(_)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    ret = decoderSurfaceFilter_->SetVideoSurface(surface);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetPostProcessorFd, TestSize.Level1)
{
    int32_t postProcessorFd = -1;
    auto ret = decoderSurfaceFilter_->SetPostProcessorFd(postProcessorFd);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetSpeed, TestSize.Level1)
{
    float speed = 1.0;
    decoderSurfaceFilter_->postProcessor_ = nullptr;
    auto ret = decoderSurfaceFilter_->SetSpeed(speed);
    EXPECT_EQ(ret, Status::OK);

    std::shared_ptr<BaseVideoPostProcessor> postProcessor = std::make_shared<BaseVideoPostProcessor>();
    decoderSurfaceFilter_->postProcessor_ = postProcessor;
    EXPECT_CALL(*postProcessor, SetSpeed(_)).WillRepeatedly(Return(Status::OK));
    ret = decoderSurfaceFilter_->SetSpeed(speed);
    EXPECT_EQ(ret, Status::OK);
    
    EXPECT_CALL(*postProcessor, SetSpeed(_)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    ret = decoderSurfaceFilter_->SetSpeed(speed);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetCameraPostprocessing, TestSize.Level1)
{
    auto ret = decoderSurfaceFilter_->SetCameraPostprocessing(true);
    EXPECT_EQ(ret, Status::OK);

    ret = decoderSurfaceFilter_->SetCameraPostprocessing(false);
    EXPECT_EQ(ret, Status::OK);
}

#ifdef SUPPORT_CAMERA_POST_PROCESSOR
HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_LoadCameraPostProcessorLib, TestSize.Level1)
{
    EXPECT_EQ(decoderSurfaceFilter->cameraPostProcessorLibHandle_, nullptr);
    decoderSurfaceFilter->SetPostProcessorType(VideoPostProcessorType::CAMERA_INSERT_FRAME);
    EXPECT_EQ(decoderSurfaceFilter->postProcessor_, nullptr);
    decoderSurfaceFilter->CreatePostProcessor();
    EXPECT_EQ(decoderSurfaceFilter->postProcessor_, nullptr);
    decoderSurfaceFilter->LoadCameraPostProcessorLib();
    decoderSurfaceFilter->CreatePostProcessor();
    EXPECT_NE(decoderSurfaceFilter->cameraPostProcessorLibHandle_, nullptr);
    EXPECT_NE(decoderSurfaceFilter->postProcessor_, nullptr);
}
#endif
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
