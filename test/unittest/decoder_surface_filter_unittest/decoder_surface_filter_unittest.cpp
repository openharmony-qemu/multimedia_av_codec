/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "mock/video_post_processor_factory.h"
#include "mock/fdsan_fd.h"
#include "mock/avbuffer.h"
#include "mock/avcodec_list.h"
#include "decoder_surface_filter_unittest.h"

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
const static int32_t INVALID_NUM = -1;
const static int32_t NUM_TEST = 0;
const static int32_t WIDTH_TEST = 10;
const static int32_t RENDER_TIME = 1000;
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
    void OnDfxEvent(const DfxEvent &event) override
    {
        (void)event;
    }
};

void DecoderSurfaceFilterUnittest::SetUpTestCase(void) {}

void DecoderSurfaceFilterUnittest::TearDownTestCase(void) {}

void DecoderSurfaceFilterUnittest::SetUp(void)
{
    decoderSurfaceFilter_ = std::make_shared<DecoderSurfaceFilter>("TestName", FilterType::FILTERTYPE_VIDEODEC);
}

void DecoderSurfaceFilterUnittest::TearDown(void)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_ = nullptr;
}

/**
 * @tc.name: Test OnError  API
 * @tc.number: CallbackOnError_001
 * @tc.desc: Test decoderSurfaceFilter != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, CallbackOnError_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<FilterVideoPostProcessorCallback>(decoderSurfaceFilter_);
    int32_t errcode = INVALID_NUM;
    callbackPtr->OnError(errcode);
    auto testPtr = callbackPtr->decoderSurfaceFilter_.lock();
    decoderSurfaceFilter_->videoSink_ = nullptr;
}

/**
 * @tc.name: Test OnError  API
 * @tc.number: CallbackOnError_002
 * @tc.desc: Test decoderSurfaceFilter == nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, CallbackOnError_002, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<FilterVideoPostProcessorCallback>(nullptr);
    int32_t errcode = INVALID_NUM;
    callbackPtr->OnError(errcode);
    auto testPtr = callbackPtr->decoderSurfaceFilter_.lock();
    EXPECT_EQ(testPtr, nullptr);
}

/**
 * @tc.name: Test OnOutputBufferAvailable  API
 * @tc.number: CallbackOnOutputBufferAvailable_001
 * @tc.desc: Test decoderSurfaceFilter == nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, CallbackOnOutputBufferAvailable_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<FilterVideoPostProcessorCallback>(nullptr);
    int32_t errcode = INVALID_NUM;
    std::shared_ptr<AVBuffer> buffer;
    callbackPtr->OnOutputBufferAvailable(errcode, buffer);
    auto testPtr = callbackPtr->decoderSurfaceFilter_.lock();
    EXPECT_EQ(testPtr, nullptr);
}

/**
 * @tc.name: Test OnOutputBufferAvailable  API
 * @tc.number: CallbackOnOutputBufferAvailable_001
 * @tc.desc: Test decoderSurfaceFilter != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, CallbackOnOutputBufferAvailable_002, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->prerollDone_.store(true);
    decoderSurfaceFilter_->inPreroll_.store(true);
    decoderSurfaceFilter_->isInSeekContinuous_ = false;
    decoderSurfaceFilter_->isSeek_ = false;
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<FilterVideoPostProcessorCallback>(decoderSurfaceFilter_);
    int32_t errcode = INVALID_NUM;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = NUM_TEST;
    callbackPtr->OnOutputBufferAvailable(errcode, buffer);
}

/**
 * @tc.name: Test OnError  API
 * @tc.number: MediaCodeCallbackOnError_001
 * @tc.desc: Test decoderSurfaceFilter != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, MediaCodeCallbackOnError_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->eventReceiver_ = nullptr;
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<FilterMediaCodecCallbackWithPostProcessor>(decoderSurfaceFilter_);
    MediaAVCodec::AVCodecErrorType errorType = MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL;
    int32_t errcode = INVALID_NUM;
    callbackPtr->OnError(errorType, errcode);
    auto callbackPtr2 = std::make_shared<FilterMediaCodecCallbackWithPostProcessor>(nullptr);
    callbackPtr2->OnError(errorType, errcode);
}

/**
 * @tc.name: Test OnOutputFormatChanged  API
 * @tc.number: MediaCodeOnOutputFormatChanged_001
 * @tc.desc: Test all
 */
HWTEST_F(DecoderSurfaceFilterUnittest, MediaCodeOnOutputFormatChanged_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->eventReceiver_ = nullptr;
    decoderSurfaceFilter_->surfaceWidth_ = NUM_TEST;
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<FilterMediaCodecCallbackWithPostProcessor>(decoderSurfaceFilter_);
    MediaAVCodec::Format format;
    format.PutIntValue("video_picture_width", WIDTH_TEST);
    callbackPtr->OnOutputFormatChanged(format);
    auto callbackPtr2 = std::make_shared<FilterMediaCodecCallbackWithPostProcessor>(nullptr);
    callbackPtr2->OnOutputFormatChanged(format);
    auto testPtr = callbackPtr2->decoderSurfaceFilter_.lock();
    EXPECT_EQ(testPtr, nullptr);
}

/**
 * @tc.name: Test OnOutputBufferAvailable  API
 * @tc.number: MediaCodeOnOutputBufferAvailable_001
 * @tc.desc: Test all
 */
HWTEST_F(DecoderSurfaceFilterUnittest, MediaCodeOnOutputBufferAvailable_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->isSeek_ = false;
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_),
        ReleaseOutputBuffer(_, _, _)).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<FilterMediaCodecCallbackWithPostProcessor>(decoderSurfaceFilter_);
    uint32_t index = NUM_TEST;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = NUM_TEST;
    buffer->pts_ = WIDTH_TEST;
    callbackPtr->OnOutputBufferAvailable(index, buffer);
    auto callbackPtr2 = std::make_shared<FilterMediaCodecCallbackWithPostProcessor>(nullptr);
    callbackPtr2->OnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(decoderSurfaceFilter_->prevDecoderPts_, WIDTH_TEST);
}

/**
 * @tc.name: Test OnBufferAvailable  API
 * @tc.number: AVBufferAvailableOnBufferAvailable_001
 * @tc.desc: Test decoderSurfaceFilter == nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, MediaCodeOnBufferAvailable_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto callbackPtr = std::make_shared<AVBufferAvailableListener>(nullptr);
    callbackPtr->OnBufferAvailable();
    EXPECT_EQ(callbackPtr->decoderSurfaceFilter_.lock(), nullptr);
}

/**
 * @tc.name: Test DoInitAfterLink  API
 * @tc.number: DoInitAfterLink_001
 * @tc.desc: Test ret != Status::OK
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoInitAfterLink_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), SetCallingInfo(_, _, _, _)).WillRepeatedly(Return());
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Init(_, _, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Configure(_)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), SetCallback(_)).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->isDrmProtected_ = false;
    decoderSurfaceFilter_->postProcessor_ = nullptr;
    auto mockEventReceiver = std::make_shared<MockEventReceiver>();
    decoderSurfaceFilter_->eventReceiver_ = mockEventReceiver;
    auto ret = decoderSurfaceFilter_->DoInitAfterLink();
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Test DoStart  API
 * @tc.number: DoStart_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoStart_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->isPaused_.store(true);
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Start()).WillRepeatedly(Return(Status::OK));
    auto ret = decoderSurfaceFilter_->DoStart();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test DoPause  API
 * @tc.number: DoPause_001
 * @tc.desc: Test state_ == FilterState::FROZEN
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoPause_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->state_ = FilterState::FROZEN;
    auto ret = decoderSurfaceFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->state_, FilterState::PAUSED);
}

/**
 * @tc.name: Test DoResume  API
 * @tc.number: DoResume_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoResume_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    auto ret = decoderSurfaceFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test DoResumeDragging  API
 * @tc.number: DoResumeDragging_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoResumeDragging_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    auto ret = decoderSurfaceFilter_->DoResumeDragging();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test DoUnFreeze  API
 * @tc.number: DoUnFreeze_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoUnFreeze_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Start()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    decoderSurfaceFilter_->state_ = FilterState::FROZEN;
    auto ret = decoderSurfaceFilter_->DoUnFreeze();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test DoFlush  API
 * @tc.number: DoFlush_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoFlush_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Flush()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Flush()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    decoderSurfaceFilter_->state_ = FilterState::FROZEN;
    auto ret = decoderSurfaceFilter_->DoFlush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test DoPreroll  API
 * @tc.number: DoPreroll_001
 * @tc.desc: Test ret != Status::OK
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoPreroll_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Start()).WillRepeatedly(Return(Status::ERROR_NULL_POINTER));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->isDecoderReleasedForMute_ = false;
    decoderSurfaceFilter_->inPreroll_.store(false);
    decoderSurfaceFilter_->isPaused_.store(false);
    auto mockEventReceiver = std::make_shared<MockEventReceiver>();
    decoderSurfaceFilter_->eventReceiver_ = mockEventReceiver;
    decoderSurfaceFilter_->state_ = FilterState::FROZEN;
    auto ret = decoderSurfaceFilter_->DoPreroll();
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Test SetParameter  API
 * @tc.number: SetParameter_001
 * @tc.desc: Test isDrmProtected_ == true
 *           Test postProcessor_!= nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetParameter_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_),
        SetParameter(_)).WillRepeatedly(Return(MediaAVCodec::AVCS_ERR_INVALID_STATE));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Reset()).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Configure(_)).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), SetOutputSurface(_)).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), SetParameter(_)).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    decoderSurfaceFilter_->isDrmProtected_ = true;
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), SetParameter(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    decoderSurfaceFilter_->SetParameter(parameter);
}

/**
 * @tc.name: Test PostProcessorOnError  API
 * @tc.number: PostProcessorOnError_001
 * @tc.desc: Test eventReceiver_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, PostProcessorOnError_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    auto mockEventReceiver = std::make_shared<MockEventReceiver>();
    decoderSurfaceFilter_->eventReceiver_ = mockEventReceiver;
    int32_t errCode = NUM_TEST;
    decoderSurfaceFilter_->PostProcessorOnError(errCode);
}

/**
 * @tc.name: Test OnUnLinked  API
 * @tc.number: OnUnLinked_001
 * @tc.desc: Test return Status::OK
 */
HWTEST_F(DecoderSurfaceFilterUnittest, OnUnLinked_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    StreamType inType = StreamType::STREAMTYPE_PACKED;
    auto ret = decoderSurfaceFilter_->OnUnLinked(inType, nullptr);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test DoReleaseOutputBuffer  API
 * @tc.number: DoReleaseOutputBuffer_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoReleaseOutputBuffer_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_),
        ReleaseOutputBuffer(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    uint32_t index = NUM_TEST;
    bool render = true;
    int64_t pts = NUM_TEST;
    decoderSurfaceFilter_->DoReleaseOutputBuffer(index, render, pts);
}

/**
 * @tc.name: Test DoRenderOutputBufferAtTime  API
 * @tc.number: DoRenderOutputBufferAtTime_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DoRenderOutputBufferAtTime_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_),
        RenderOutputBufferAtTime(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    uint32_t index = NUM_TEST;
    int64_t render = NUM_TEST;
    int64_t pts = NUM_TEST;
    decoderSurfaceFilter_->DoRenderOutputBufferAtTime(index, render, pts);
}

/**
 * @tc.name: Test DecoderDrainOutputBuffer  API
 * @tc.number: DecoderDrainOutputBuffer_001
 * @tc.desc: Test all
 */
HWTEST_F(DecoderSurfaceFilterUnittest, DecoderDrainOutputBuffer_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_),
        ReleaseOutputBuffer(_, _, _)).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), NotifyEos(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    uint32_t index = NUM_TEST;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::EOS);
    buffer->pts_ = WIDTH_TEST;
    decoderSurfaceFilter_->DecoderDrainOutputBuffer(index, buffer);
    EXPECT_EQ(decoderSurfaceFilter_->prevDecoderPts_, buffer->pts_);
}

/**
 * @tc.name: Test SetVideoSurface  API
 * @tc.number: SetVideoSurface_001
 * @tc.desc: Test err != GSERROR_OK
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetVideoSurface_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), SetOutputSurface(_)).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    sptr<MockSurface> mockVideoSurface = new MockSurface();
    EXPECT_CALL(*(mockVideoSurface), SetScalingMode(_)).WillRepeatedly(Return(GSERROR_INVALID_ARGUMENTS));
    sptr<Surface> videoSurface = mockVideoSurface;
    auto ret = decoderSurfaceFilter_->SetVideoSurface(videoSurface);
    EXPECT_NE(decoderSurfaceFilter_->videoSurface_, nullptr);
    EXPECT_EQ(ret, Status::OK);
    decoderSurfaceFilter_->videoSurface_ = nullptr;
}

/**
 * @tc.name: Test SetDecryptConfig  API
 * @tc.number: SetDecryptConfig_001
 * @tc.desc: Test keySessionProxy == nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetDecryptConfig_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    sptr<DrmStandard::IMediaKeySessionService> keySessionProxy;
    auto ret = decoderSurfaceFilter_->SetDecryptConfig(keySessionProxy, true);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: Test ParseDecodeRateLimit  API
 * @tc.number: ParseDecodeRateLimit_001
 * @tc.desc: Test codeList = nullptr 1
 */
HWTEST_F(DecoderSurfaceFilterUnittest, ParseDecodeRateLimit_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->ParseDecodeRateLimit();
    auto codeList = MediaAVCodec::AVCodecListFactory::CreateAVCodecList();
    EXPECT_EQ(codeList, nullptr);
}

/**
 * @tc.name: Test OnDumpInfo  API
 * @tc.number: OnDumpInfo_001
 * @tc.desc: Test all
 */
HWTEST_F(DecoderSurfaceFilterUnittest, OnDumpInfo_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), OnDumpInfo(_)).WillRepeatedly(Return());
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    int32_t testFd = NUM_TEST;
    decoderSurfaceFilter_->OnDumpInfo(testFd);
}

/**
 * @tc.name: Test SetSeiMessageCbStatus  API
 * @tc.number: SetSeiMessageCbStatus_001
 * @tc.desc: Test producerListener_ == nullptr
 *           Test producerListener_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetSeiMessageCbStatus_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->producerListener_ = nullptr;
    sptr<MockInputBufferQueueProducer> mockInputBufferQueueProducer = new MockInputBufferQueueProducer();
    decoderSurfaceFilter_->inputBufferQueueProducer_ = mockInputBufferQueueProducer;
    bool status = true;
    std::vector<int32_t> payloadTypes;
    auto ret = decoderSurfaceFilter_->SetSeiMessageCbStatus(status, payloadTypes);
    std::cout << "1" << std::endl;
    EXPECT_NE(decoderSurfaceFilter_->producerListener_, nullptr);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Test InitPostProcessor  API
 * @tc.number: InitPostProcessor_001
 * @tc.desc: Test fsanfd != nullptr
 *           Test ret != Status::OK
 */
HWTEST_F(DecoderSurfaceFilterUnittest, InitPostProcessor_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), SetOutputSurface(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), SetEventReceiver(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_),
        SetVideoWindowSize(_, _)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), SetPostProcessorOn(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), SetParameter(_)).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Init()).WillRepeatedly(Return(Status::ERROR_UNKNOWN));
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    auto mockEventReceiver = std::make_shared<MockEventReceiver>();
    decoderSurfaceFilter_->eventReceiver_ = mockEventReceiver;
    decoderSurfaceFilter_->fdsanFd_ = std::make_unique<FdsanFd>();
    auto ret = decoderSurfaceFilter_->InitPostProcessor();
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Test SetPostProcessorType  API
 * @tc.number: SetPostProcessorType_001
 * @tc.desc: Test all
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetPostProcessorType_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->SetPostProcessorType(VideoPostProcessorType::NONE);
    EXPECT_EQ(decoderSurfaceFilter_->postProcessorType_, VideoPostProcessorType::NONE);
}

/**
 * @tc.name: Test CreatePostProcessor  API
 * @tc.number: CreatePostProcessor_001
 * @tc.desc: Test all
 */
HWTEST_F(DecoderSurfaceFilterUnittest, CreatePostProcessor_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), SetOutputSurface(_)).WillRepeatedly(Return(NUM_TEST));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessorType_ = VideoPostProcessorType::SUPER_RESOLUTION;
    auto ret = decoderSurfaceFilter_->CreatePostProcessor();
    EXPECT_NE(ret, nullptr);
    EXPECT_NE(decoderSurfaceFilter_->postProcessor_, nullptr);
}

/**
 * @tc.name: Test SetPostProcessorFd  API
 * @tc.number: SetPostProcessorFd_001
 * @tc.desc: Test fdsanFd_ != nullptr
 *           Test fdsanFd_->Get() = 0
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetPostProcessorFd_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->fdsanFd_ = nullptr;
    const char* testFilePath = "/data/testfile.txt";
    int32_t testFd = open(testFilePath, O_CREAT | O_RDWR, 0666);
    ASSERT_NE(testFd, -1);
    auto ret = decoderSurfaceFilter_->SetPostProcessorFd(testFd);
    EXPECT_EQ(ret, Status::OK);
    close(testFd);
    remove(testFilePath);
}

/**
 * @tc.name: Test SetPostProcessorFd  API
 * @tc.number: SetPostProcessorFd_002
 * @tc.desc: Test fdsanFd_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetPostProcessorFd_002, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    const char* testFilePath = "/data/testfile.txt";
    int32_t testFd = open(testFilePath, O_CREAT | O_RDWR, 0666);
    ASSERT_NE(testFd, -1);
    decoderSurfaceFilter_->fdsanFd_ = std::make_unique<FdsanFd>();
    EXPECT_EQ(decoderSurfaceFilter_->fdsanFd_->fd_, -1);
    auto ret = decoderSurfaceFilter_->SetPostProcessorFd(testFd);
    EXPECT_EQ(ret, Status::OK);
    close(testFd);
    remove(testFilePath);
}

/**
 * @tc.name: Test SetSeekTime  API
 * @tc.number: SetSeekTime_001
 * @tc.desc: Test postProcessor_ != nullptr
 */
HWTEST_F(DecoderSurfaceFilterUnittest, SetSeekTime_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoSink_), ResetSyncInfo()).WillRepeatedly(Return());
    decoderSurfaceFilter_->postProcessor_ = std::make_shared<BaseVideoPostProcessor>();
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_),
        SetSeekTime(_, _)).WillRepeatedly(Return());
    EXPECT_CALL(*(decoderSurfaceFilter_->postProcessor_), Release()).WillRepeatedly(Return(Status::OK));
    int64_t seekTimeUs = NUM_TEST;
    PlayerSeekMode mode = PlayerSeekMode::SEEK_CLOSEST;
    decoderSurfaceFilter_->SetSeekTime(seekTimeUs, mode);
}

/**
 * @tc.name: Test HandleRender API
 * @tc.number: HandleRender_001
 * @tc.desc: Test eventReceiver_== nullptr && lastRenderTimeNs = 0L
 *                isInSeekContinuous_ = false
 */
HWTEST_F(DecoderSurfaceFilterUnittest, HandleRender_001, TestSize.Level0)
{
    ASSERT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->isInSeekContinuous_ = false;
    auto buffer = std::make_shared<AVBuffer>();
    buffer->pts_ = 0;
    buffer->meta_ = std::make_shared<Meta>();

    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_),
        RenderOutputBufferAtTime(_, _, _)).WillRepeatedly(Return(0));
    EXPECT_CALL(*(decoderSurfaceFilter_->videoDecoder_), Release()).WillRepeatedly(Return(Status::OK));

    int index = 0;
    bool render = true;
    int64_t renderTime = RENDER_TIME;
    decoderSurfaceFilter_->eventReceiver_ = nullptr;
    decoderSurfaceFilter_->HandleRender(index, render, buffer, renderTime);
    EXPECT_EQ(decoderSurfaceFilter_->lastRenderTimeNs_, renderTime);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
