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
    buffer->flag_ = 1;
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
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
