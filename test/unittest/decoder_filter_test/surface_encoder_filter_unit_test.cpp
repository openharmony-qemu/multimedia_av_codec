/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

    EXPECT_EQ(surfaceEncoder->DoStart(), Status::ERROR_UNKNOWN);
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
    EXPECT_EQ(surfaceEncoder->instanceId_, 0);
}

HWTEST_F(SurfaceEncoderFilterUnitTest, GetInputSurface_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::SurfaceEncoderFilter> surfaceEncoder = std::make_shared<Pipeline::SurfaceEncoderFilter>(
        "test", Pipeline::FilterType::FILTERTYPE_VIDRESIZE);
    surfaceEncoder->surface_ = nullptr;
    surfaceEncoder->GetInputSurface();
    surfaceEncoder->mediaCodec_ = nullptr;
    surfaceEncoder->GetInputSurface();
    EXPECT_EQ(surfaceEncoder->instanceId_, 0);
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
}
