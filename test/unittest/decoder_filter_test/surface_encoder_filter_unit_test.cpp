/*
 * Copyright (c) 2023-2026 Huawei Device Co., Ltd.
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

#include "surface_encoder_filter_unit_test.h"

#include <iostream>
#include <string>

#include "common/log.h"
#include "common/media_core.h"
#include "filter/filter_factory.h"

#include "surface_encoder_filter.h"
#include "surface_encoder_adapter.h"
#include "muxer_filter.h"
#include "avcodec_video_encoder.h"
#include "meta/meta.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
void SurfaceEncoderFilterUnitTest::SetUpTestCase(void)
{
}

void SurfaceEncoderFilterUnitTest::TearDownTestCase(void)
{
}

void SurfaceEncoderFilterUnitTest::SetUp()
{
}

void SurfaceEncoderFilterUnitTest::TearDown()
{
}

HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);

    std::shared_ptr<Meta> format = std::make_shared<Meta>();
    format->Set<Tag::MIME_TYPE>("test");
    format->Set<Tag::MEDIA_END_OF_STREAM>(true);
    EXPECT_EQ(surfaceEncoder->SetCodecFormat(format), Status::OK);

    std::shared_ptr<AVBuffer> waterMarkBuffer = std::make_shared<AVBuffer>();
    EXPECT_EQ(surfaceEncoder->Configure(format), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->SetWatermark(waterMarkBuffer), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->DoStart(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->DoPause(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->DoResume(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->DoStop(), Status::OK);
    EXPECT_EQ(surfaceEncoder->Reset(), Status::OK);
    EXPECT_EQ(surfaceEncoder->DoRelease(), Status::OK);
}


HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);

    std::shared_ptr<Meta> format = std::make_shared<Meta>();
    format->Set<Tag::MIME_TYPE>("test");
    format->Set<Tag::MEDIA_END_OF_STREAM>(true);
    EXPECT_EQ(surfaceEncoder->SetCodecFormat(format), Status::OK);

    std::shared_ptr<AVBuffer> waterMarkBuffer = std::make_shared<AVBuffer>();
    surfaceEncoder->SetParameter(nullptr);
    surfaceEncoder->isUpdateCodecNeeded_ = true;

    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    surfaceEncoder->Init(eventReceive, filterCallback);

    EXPECT_EQ(surfaceEncoder->Configure(format), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->SetWatermark(waterMarkBuffer), Status::ERROR_UNKNOWN);

    EXPECT_EQ(surfaceEncoder->SetInputSurface(nullptr), Status::OK);
    EXPECT_EQ(surfaceEncoder->SetTransCoderMode(), Status::OK);
    EXPECT_EQ(surfaceEncoder->DoPrepare(), Status::OK);

    EXPECT_EQ(surfaceEncoder->DoStart(), Status::ERROR_FRAMEWORK_ERR);
    EXPECT_EQ(surfaceEncoder->DoPause(), Status::OK);
    EXPECT_EQ(surfaceEncoder->DoResume(), Status::OK);
    EXPECT_EQ(surfaceEncoder->DoStop(), Status::OK);
    EXPECT_EQ(surfaceEncoder->Reset(), Status::OK);
    EXPECT_EQ(surfaceEncoder->DoFlush(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->DoRelease(), Status::OK);
    EXPECT_EQ(surfaceEncoder->NotifyEos(UINT32_MAX), Status::ERROR_UNKNOWN);

    surfaceEncoder->SetParameter(format);
    surfaceEncoder->GetParameter(format);
    surfaceEncoder->GetFilterType();

    std::shared_ptr<Meta> meta;
    surfaceEncoder->OnUpdatedResult(meta);
    surfaceEncoder->OnUnlinkedResult(meta);
    surfaceEncoder->SetCallingInfo(1, 1, "111", 1);

    EXPECT_EQ(surfaceEncoder->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(surfaceEncoder->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);

    std::shared_ptr<TestFilterLinkCallback> filterLinkCallback = std::make_shared<TestFilterLinkCallback>();
    EXPECT_EQ(surfaceEncoder->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::OK);
    EXPECT_EQ(surfaceEncoder->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::OK);
    EXPECT_EQ(surfaceEncoder->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, filterLinkCallback), Status::OK);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, OnError_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    surfaceEncoder->eventReceiver_ = std::make_shared<TestEventReceiver>();
    surfaceEncoder->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 11);
    surfaceEncoder->eventReceiver_ = nullptr;
    surfaceEncoder->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, 11);
    EXPECT_EQ(surfaceEncoder->isUpdateCodecNeeded_, false);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, Init_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<TestEventReceiver> receiver = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> callback = std::make_shared<TestFilterCallback>();
    surfaceEncoder->mediaCodec_ = nullptr;
    surfaceEncoder->isUpdateCodecNeeded_ = false;
    surfaceEncoder->Init(receiver, callback);
    surfaceEncoder->isUpdateCodecNeeded_ = true;
    surfaceEncoder->Init(receiver, callback);
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    surfaceEncoder->Init(receiver, callback);
    surfaceEncoder->mediaCodec_->codecServer_ = MediaAVCodec::VideoEncoderFactory::CreateByMime("video/avc");
    surfaceEncoder->mediaCodec_->releaseBufferTask_ = std::make_shared<Task>("SurfaceEncoder");
    EXPECT_NE(surfaceEncoder->mediaCodec_->codecServer_, nullptr);
    EXPECT_EQ(surfaceEncoder->instanceId_, 0);
}

/**
 * @tc.name: SurfaceEncoderFilter_Init_002
 * @tc.desc: Init if#1 TRUE (mediaCodec_ null) + if#2 FALSE + if#3 TRUE (Init succeeds) + if#4 FALSE (ret==OK)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_Init_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<RecordableEventReceiver> receiver = std::make_shared<RecordableEventReceiver>();
    std::shared_ptr<TestFilterCallback> callback = std::make_shared<TestFilterCallback>();
    surfaceEncoder->codecMimeType_ = "video/avc";
    surfaceEncoder->mediaCodec_ = nullptr;
    surfaceEncoder->isUpdateCodecNeeded_ = false;
    surfaceEncoder->Init(receiver, callback);
    EXPECT_NE(surfaceEncoder->mediaCodec_, nullptr);
    EXPECT_EQ(receiver->eventCount_, 0);
    surfaceEncoder->DoRelease();
}

/**
 * @tc.name: SurfaceEncoderFilter_Init_003
 * @tc.desc: Init if#1 TRUE (isUpdateCodecNeeded_) + if#2 TRUE (Release old) + if#3 FALSE + if#4 TRUE (error event)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_Init_003, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<RecordableEventReceiver> receiver = std::make_shared<RecordableEventReceiver>();
    std::shared_ptr<TestFilterCallback> callback = std::make_shared<TestFilterCallback>();
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    surfaceEncoder->isUpdateCodecNeeded_ = true;
    surfaceEncoder->Init(receiver, callback);
    EXPECT_NE(surfaceEncoder->mediaCodec_, nullptr);
    EXPECT_EQ(receiver->eventCount_, 1);
    surfaceEncoder->DoRelease();
}

/**
 * @tc.name: SurfaceEncoderFilter_Init_004
 * @tc.desc: Init if#1 FALSE (mediaCodec_ non-null, isUpdateCodecNeeded_ false) + if#4 FALSE (ret==OK)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_Init_004, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<RecordableEventReceiver> receiver = std::make_shared<RecordableEventReceiver>();
    std::shared_ptr<TestFilterCallback> callback = std::make_shared<TestFilterCallback>();
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    surfaceEncoder->isUpdateCodecNeeded_ = false;
    surfaceEncoder->Init(receiver, callback);
    EXPECT_NE(surfaceEncoder->mediaCodec_, nullptr);
    EXPECT_EQ(receiver->eventCount_, 0);
    surfaceEncoder->DoRelease();
}

/**
 * @tc.name: SurfaceEncoderFilter_Init_005
 * @tc.desc: Init if#4 FALSE via eventReceiver_ == nullptr (ret != OK but receiver is null)
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_Init_005, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<TestFilterCallback> callback = std::make_shared<TestFilterCallback>();
    surfaceEncoder->mediaCodec_ = nullptr;
    surfaceEncoder->isUpdateCodecNeeded_ = false;
    surfaceEncoder->Init(nullptr, callback);
    EXPECT_NE(surfaceEncoder->mediaCodec_, nullptr);
    EXPECT_EQ(surfaceEncoder->isUpdateCodecNeeded_, false);
    surfaceEncoder->DoRelease();
}

/**
 * @tc.name: SurfaceEncoderFilter_Configure_001
 * @tc.desc: Configure if (mediaCodec_ == nullptr) TRUE branch - returns ERROR_UNKNOWN
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_Configure_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    surfaceEncoder->mediaCodec_ = nullptr;
    EXPECT_EQ(surfaceEncoder->Configure(parameter), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SurfaceEncoderFilter_Configure_002
 * @tc.desc: Configure if (mediaCodec_ == nullptr) FALSE branch - mediaCodec_ non-null, calls adapter Configure
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_Configure_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    EXPECT_NE(surfaceEncoder->mediaCodec_, nullptr);
    EXPECT_EQ(surfaceEncoder->Configure(parameter), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceEncoder->configureParameter_, parameter);
    surfaceEncoder->DoRelease();
}

HWTEST_F(SurfaceEncoderFilterUnitTest, GetInputSurface_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    surfaceEncoder->surface_ = nullptr;
    surfaceEncoder->GetInputSurface();
    surfaceEncoder->surface_ = Surface::CreateSurfaceAsConsumer();
    EXPECT_NE(surfaceEncoder->GetInputSurface(), nullptr);
    surfaceEncoder->mediaCodec_ = nullptr;
    surfaceEncoder->GetInputSurface();
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    EXPECT_NE(surfaceEncoder->GetInputSurface(), nullptr);
    EXPECT_EQ(surfaceEncoder->instanceId_, 0);
}

/**
 * @tc.name: SurfaceEncoderFilter_GetInputSurface_002
 * @tc.desc: GetInputSurface if (surface_ == nullptr && mediaCodec_ != nullptr) TRUE branch
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_GetInputSurface_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    surfaceEncoder->surface_ = nullptr;
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    EXPECT_EQ(surfaceEncoder->GetInputSurface(), nullptr);
    surfaceEncoder->DoRelease();
}

/**
 * @tc.name: SurfaceEncoderFilter_GetInputSurface_003
 * @tc.desc: GetInputSurface if FALSE branch - surface_ non-null, returns existing surface
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_GetInputSurface_003, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    sptr<Surface> testSurface = Surface::CreateSurfaceAsConsumer();
    surfaceEncoder->surface_ = testSurface;
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    EXPECT_EQ(surfaceEncoder->GetInputSurface(), testSurface);
    surfaceEncoder->DoRelease();
}

/**
 * @tc.name: SurfaceEncoderFilter_GetInputSurface_004
 * @tc.desc: GetInputSurface if FALSE branch - mediaCodec_ null, returns nullptr
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_GetInputSurface_004, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    surfaceEncoder->surface_ = nullptr;
    surfaceEncoder->mediaCodec_ = nullptr;
    EXPECT_EQ(surfaceEncoder->GetInputSurface(), nullptr);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, SetParameter_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::MEDIA_END_OF_STREAM>(true);
    parameter->Set<Tag::USER_FRAME_PTS>(1);
    surfaceEncoder->SetParameter(parameter);
    parameter->Set<Tag::MEDIA_END_OF_STREAM>(false);
    parameter->Set<Tag::USER_FRAME_PTS>(2);
    surfaceEncoder->SetParameter(parameter);
    EXPECT_EQ(surfaceEncoder->instanceId_, 0);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, OnLinkedResult_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    sptr<AVBufferQueueProducer> outputBufferQueue = new MyAVBufferQueueProducer();
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    surfaceEncoder->onLinkedResultCallback_ = std::make_shared<TestFilterLinkCallback>();
    surfaceEncoder->OnLinkedResult(outputBufferQueue, meta);
    surfaceEncoder->onLinkedResultCallback_ = nullptr;
    surfaceEncoder->OnLinkedResult(outputBufferQueue, meta);
    EXPECT_EQ(surfaceEncoder->instanceId_, 0);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_OnLinkedResult_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    sptr<AVBufferQueueProducer> outputBufferQueue = new MyAVBufferQueueProducer();
    surfaceEncoder->mediaCodec_ = nullptr;
    surfaceEncoder->OnLinkedResult(outputBufferQueue, meta);
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    surfaceEncoder->OnLinkedResult(outputBufferQueue, meta);
    EXPECT_EQ(surfaceEncoder->mediaCodec_->outputBufferQueueProducer_, outputBufferQueue);
    surfaceEncoder->DoRelease();
}

HWTEST_F(SurfaceEncoderFilterUnitTest, OnReportKeyFramePts_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    surfaceEncoder->OnReportKeyFramePts("test");
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    int32_t appUid =  0;
    int32_t appPid = 0;
    const std::string bundleName = "test";
    uint64_t instanceId = 0;
    surfaceEncoder->SetCallingInfo(appUid, appPid, bundleName, instanceId);
    surfaceEncoder->mediaCodec_ = nullptr;
    surfaceEncoder->SetCallingInfo(appUid, appPid, bundleName, instanceId);
    EXPECT_EQ(surfaceEncoder->instanceId_, 0);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, SetWatermarkMode_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VENC);
    surfaceEncoder->mediaCodec_ = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    EXPECT_EQ(Status::OK, surfaceEncoder->SetWatermarkMode());
}

HWTEST_F(SurfaceEncoderFilterUnitTest, OnReportFirstFramePts_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    surfaceEncoder->eventReceiver_ = nullptr;
    int64_t firstFramePts = 1000;
    surfaceEncoder->OnReportFirstFramePts(firstFramePts);
    EXPECT_EQ(surfaceEncoder->eventReceiver_, nullptr);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, OnReportFirstFramePts_002, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<RecordableEventReceiver> receiver = std::make_shared<RecordableEventReceiver>();
    surfaceEncoder->eventReceiver_ = receiver;
    int64_t firstFramePts = 1000;
    surfaceEncoder->OnReportFirstFramePts(firstFramePts);
    EXPECT_EQ(receiver->eventCount_, 1);
    EXPECT_EQ(AnyCast<int64_t>(receiver->lastEvent_.param), firstFramePts);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, OnReportFirstFramePts_003, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    std::shared_ptr<RecordableEventReceiver> receiver = std::make_shared<RecordableEventReceiver>();
    surfaceEncoder->eventReceiver_ = receiver;
    int64_t firstFramePts1 = 1000;
    surfaceEncoder->OnReportFirstFramePts(firstFramePts1);
    int64_t firstFramePts2 = 2000;
    surfaceEncoder->OnReportFirstFramePts(firstFramePts2);
    EXPECT_EQ(receiver->eventCount_, 2);
    EXPECT_EQ(AnyCast<int64_t>(receiver->lastEvent_.param), firstFramePts2);
}

/**
 * @tc.name: SurfaceEncoderFilter_LinkNext_NullNextFilter_0100
 * @tc.desc: Test LinkNext with nullptr nextFilter, should return ERROR_NULL_POINTER
 *           without writing nextFilter_ or nextFiltersMap_
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_LinkNext_NullNextFilter_0100, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder =
        std::make_shared<Pipeline::SurfaceEncoderFilter>("test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);

    surfaceEncoder->configureParameter_ = std::make_shared<Meta>();
    surfaceEncoder->configureParameter_->Set<Tag::MIME_TYPE>("video/avc");

    Status ret = surfaceEncoder->LinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_ENCODED_VIDEO);

    ASSERT_EQ(ret, Status::ERROR_NULL_POINTER);
    EXPECT_EQ(surfaceEncoder->nextFilter_, nullptr);
    EXPECT_EQ(surfaceEncoder->nextFiltersMap_.count(Pipeline::StreamType::STREAMTYPE_ENCODED_VIDEO), 0u);
}

/**
 * @tc.name: SurfaceEncoderFilter_SetParameter_Null_0100
 * @tc.desc: SetParameter with nullptr — covers L411 nullptr guard.
 *           Scenario: caller passes null parameter; guard returns early, no crash, no mediaCodec_ access.
 *           Contrast: non-null parameter goes through mediaCodec_->SetParameter(parameter) path (L426).
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_SetParameter_Null_0100, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder =
        std::make_shared<Pipeline::SurfaceEncoderFilter>("test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    auto adapter = std::make_shared<OHOS::Media::SurfaceEncoderAdapter>();
    auto countingEncoder = std::make_shared<CountingVideoEncoder>();
    adapter->codecServer_ = countingEncoder;
    surfaceEncoder->mediaCodec_ = adapter;
    // guard 路径：parameter=nullptr → SUT 在 L415 提前返回，mediaCodec_->SetParameter 不被调用
    surfaceEncoder->SetParameter(nullptr);
    EXPECT_EQ(countingEncoder->setParameterCount_, 0);
    // 正面对照：parameter 非空非EOS → SUT 走到 L430 mediaCodec_->SetParameter → codecServer_->SetParameter
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->SetData(Tag::MIME_TYPE, std::string("video/avc"));
    surfaceEncoder->SetParameter(param);
    EXPECT_EQ(countingEncoder->setParameterCount_, 1);
    surfaceEncoder->DoRelease();
}

/**
 * @tc.name: SurfaceEncoderFilter_SetInputSurface_NullCodec_0100
 * @tc.desc: SetInputSurface with mediaCodec_ nullptr — covers L254 guard returning ERROR_UNKNOWN.
 *           Scenario: set input surface before Init, guard should return ERROR_UNKNOWN.
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_SetInputSurface_NullCodec_0100, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder =
        std::make_shared<Pipeline::SurfaceEncoderFilter>("test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    sptr<Surface> dummySurface = nullptr;
    EXPECT_EQ(surfaceEncoder->SetInputSurface(dummySurface), Status::ERROR_UNKNOWN);
}
}
