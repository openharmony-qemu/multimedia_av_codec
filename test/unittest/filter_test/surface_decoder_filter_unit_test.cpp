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

#include "surface_decoder_filter_unit_test.h"
#include "surface_decoder_filter_unit_test.h"
#include "surface_decoder_filter.h"
#include "surface_encoder_filter.h"
#include "filter/filter_factory.h"
#include "surface_decoder_adapter.h"
#include "meta/format.h"
#include "common/media_core.h"
#include "surface/native_buffer.h"
#include "media_description.h"
#include "av_common.h"
#include <iostream>
#include <string>

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void SurfaceDecoderFilterUnitTest::SetUpTestCase(void) {}

void SurfaceDecoderFilterUnitTest::TearDownTestCase(void) {}

void SurfaceDecoderFilterUnitTest::SetUp(void)
{
    surfaceDecoderFilter_ =
        std::make_shared<SurfaceDecoderFilter>("testSurfaceDecoderFilter", FilterType::FILTERTYPE_VDEC);
    mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    filterCallback_ = std::make_shared<TestFilterCallback>();
}

void SurfaceDecoderFilterUnitTest::TearDown(void)
{
    surfaceDecoderFilter_ = nullptr;
    mediaCodec_  = nullptr;
    filterCallback_ = nullptr;
}

/**
 * @tc.name: First
 * @tc.desc: First
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, First, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
    std::shared_ptr<Meta> format = std::make_shared<Meta>();
    format->Set<Tag::MIME_TYPE>("test");
    format->Set<Tag::MEDIA_END_OF_STREAM>(true);
    EXPECT_EQ(surfaceDecoderFilter_->Configure(format), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(surfaceDecoderFilter_->DoStart(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoPause(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoResume(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoStop(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoRelease(), Status::OK);

    surfaceDecoderFilter_->SetParameter(nullptr);

    std::shared_ptr<EventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<FilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    surfaceDecoderFilter_->Init(eventReceive, filterCallback);

    EXPECT_EQ(surfaceDecoderFilter_->Configure(format), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(surfaceDecoderFilter_->SetOutputSurface(nullptr), Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: SECOND
 * @tc.desc: SECOND
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, SECOND, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
    std::shared_ptr<Meta> format = std::make_shared<Meta>();
    format->Set<Tag::MIME_TYPE>("test");
    format->Set<Tag::MEDIA_END_OF_STREAM>(true);
    surfaceDecoderFilter_->SetParameter(format);
    surfaceDecoderFilter_->GetParameter(format);
    surfaceDecoderFilter_->GetFilterType();

    EXPECT_EQ(surfaceDecoderFilter_->DoPrepare(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoStart(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoPause(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoResume(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoStop(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoFlush(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoRelease(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->NotifyNextFilterEos(UINT32_MAX, UINT32_MAX), Status::OK);

    std::shared_ptr<Meta> meta;
    surfaceDecoderFilter_->OnUpdatedResult(meta);
    surfaceDecoderFilter_->OnUnlinkedResult(meta);

    EXPECT_EQ(surfaceDecoderFilter_->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback = std::make_shared<TestFilterLinkCallback>();
    EXPECT_EQ(surfaceDecoderFilter_->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, filterLinkCallback),
        Status::OK);
}

/**
 * @tc.name: Configure
 * @tc.desc: Configure
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, Configure, TestSize.Level1)
{
    surfaceDecoderFilter_->mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    Status ret = surfaceDecoderFilter_->Configure(parameter);
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    ret = surfaceDecoderFilter_->Configure(parameter);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->Configure(parameter);
    EXPECT_NE(ret, Status::OK);
}

HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_Configure_0100, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    surfaceDecoderFilter_->mediaCodec_ = mediaCodec_;
    auto ret = surfaceDecoderFilter_->Configure(parameter);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
 
HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_Configure_0200, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    parameter->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(8);
    surfaceDecoderFilter_->mediaCodec_ = mediaCodec_;
    auto ret = surfaceDecoderFilter_->Configure(parameter);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
 
HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_Configure_0300, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::VIDEO_IS_HDR_VIVID>(false);
    parameter->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(8);
    surfaceDecoderFilter_->mediaCodec_ = mediaCodec_;
    auto ret = surfaceDecoderFilter_->Configure(parameter);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
 
HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_Configure_0400, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    parameter->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(0);
    surfaceDecoderFilter_->mediaCodec_ = mediaCodec_;
    auto ret = surfaceDecoderFilter_->Configure(parameter);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}
 
/**
 * @tc.name: ConfigureMediaCodecByMimeType
 * @tc.desc: ConfigureMediaCodecByMimeType
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, ConfigureMediaCodecByMimeType_0100, TestSize.Level1)
{
    std::string codecMimeType = Plugins::MimeType::VIDEO_AVC;
    bool isHdrVivid = true;
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    EXPECT_EQ(ret, Status::OK);
}
 
HWTEST_F(SurfaceDecoderFilterUnitTest, ConfigureMediaCodecByMimeType_0200, TestSize.Level1)
{
    std::string codecMimeType = Plugins::MimeType::VIDEO_AVC;
    bool isHdrVivid = false;
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    EXPECT_EQ(ret, Status::OK);
}
 
HWTEST_F(SurfaceDecoderFilterUnitTest, ConfigureMediaCodecByMimeType_0300, TestSize.Level1)
{
    std::string codecMimeType = "test";
    bool isHdrVivid = true;
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    EXPECT_NE(ret, Status::OK);
}
 
HWTEST_F(SurfaceDecoderFilterUnitTest, ConfigureMediaCodecByMimeType_0400, TestSize.Level1)
{
    std::string codecMimeType = "test";
    bool isHdrVivid = false;
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->ConfigureMediaCodecByMimeType(codecMimeType, isHdrVivid);
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name: SetOutputSurface
 * @tc.desc: SetOutputSurface
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, SetOutputSurface, TestSize.Level1)
{
    sptr<Surface> surface = nullptr;
    surfaceDecoderFilter_->mediaCodec_ = nullptr;
    surfaceDecoderFilter_->SetOutputSurface(surface);
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->SetOutputSurface(surface);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->SetOutputSurface(surface);
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name: DoPrepare
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, DoPrepare, TestSize.Level1)
{
    surfaceDecoderFilter_->filterCallback_ = std::make_shared<TestFilterCallback>();
    Status ret = surfaceDecoderFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoStart
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, DoStart, TestSize.Level1)
{
    surfaceDecoderFilter_->mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->DoStart();
    EXPECT_NE(ret, Status::OK);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->DoStart();
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name: DoPause
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, DoPause, TestSize.Level1)
{
    surfaceDecoderFilter_->mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoResume
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, DoResume, TestSize.Level1)
{
    surfaceDecoderFilter_->mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoStop
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, DoStop, TestSize.Level1)
{
    surfaceDecoderFilter_->mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    Status ret = surfaceDecoderFilter_->DoStop();
    EXPECT_EQ(ret, Status::OK);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    ret = surfaceDecoderFilter_->DoStop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: SetParameter
 * @tc.desc: SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, SetParameter, TestSize.Level1)
{
    surfaceDecoderFilter_->mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    surfaceDecoderFilter_->eventReceiver_ = std::make_shared<MyEventReceiver>();
    std::shared_ptr<Meta> parameter= std::make_shared<Meta>();
    surfaceDecoderFilter_->SetParameter(parameter);
    surfaceDecoderFilter_->eventReceiver_ = nullptr;
    surfaceDecoderFilter_->SetParameter(parameter);
    EXPECT_EQ(surfaceDecoderFilter_->nextFilter_, nullptr);
}

/**
 * @tc.name: OnLinkedResult
 * @tc.desc: OnLinkedResult
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, OnLinkedResult, TestSize.Level1)
{
    sptr<AVBufferQueueProducer> outputBufferQueue = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    surfaceDecoderFilter_->mediaCodec_ = std::make_shared<SurfaceDecoderAdapter>();
    surfaceDecoderFilter_->onLinkedResultCallback_ = std::make_shared<MyFilterLinkCallback>();
    surfaceDecoderFilter_->OnLinkedResult(outputBufferQueue, meta);
    surfaceDecoderFilter_->onLinkedResultCallback_ = nullptr;
    surfaceDecoderFilter_->OnLinkedResult(outputBufferQueue, meta);
    EXPECT_EQ(surfaceDecoderFilter_->nextFilter_, nullptr);
}

HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_SetOutputSurface_0100, TestSize.Level1)
{
    EXPECT_NE(surfaceDecoderFilter_, nullptr);
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    surfaceDecoderFilter_->mediaCodec_ = mediaCodec_;
    sptr<Surface> surface;
    EXPECT_EQ(surfaceDecoderFilter_->SetOutputSurface(surface), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoPrepare(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoStart(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoPause(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoResume(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoStop(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoFlush(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoRelease(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->NotifyNextFilterEos(UINT32_MAX, UINT32_MAX), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    std::shared_ptr<FilterLinkCallback> filterLinkCallback = std::make_shared<TestFilterLinkCallback>();
    std::shared_ptr<Meta> format = std::make_shared<Meta>();
    format->Set<Tag::MIME_TYPE>("test");
    format->Set<Tag::MEDIA_END_OF_STREAM>(true);
    EXPECT_EQ(surfaceDecoderFilter_->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::OK);
    EXPECT_EQ(
        surfaceDecoderFilter_->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, filterLinkCallback), Status::OK);
}

/**
 * @tc.name: SetCodecFormat
 * @tc.desc: SetCodecFormat
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_SetCodecFormat_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::VIDEO_IS_HDR_VIVID>(false);
    meta->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(8);
    surfaceDecoderFilter_->SetCodecFormat(meta);
    EXPECT_EQ(surfaceDecoderFilter_->transcoderIsHdrVivid_, false);
    EXPECT_EQ(surfaceDecoderFilter_->colorSpace_, 8);
}
 
/**
 * @tc.name: SetCodecFormat
 * @tc.desc: SetCodecFormat
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_SetCodecFormat_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    meta->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(12);
    surfaceDecoderFilter_->SetCodecFormat(meta);
    EXPECT_EQ(surfaceDecoderFilter_->transcoderIsHdrVivid_, true);
    EXPECT_EQ(surfaceDecoderFilter_->colorSpace_, 12);
}
 
/**
 * @tc.name: SetCodecFormat
 * @tc.desc: SetCodecFormat
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderFilterUnitTest, SurfaceDecoderFilter_SetCodecFormat_0300, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    meta->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(0);
    surfaceDecoderFilter_->SetCodecFormat(meta);
    EXPECT_EQ(surfaceDecoderFilter_->transcoderIsHdrVivid_, true);
    EXPECT_EQ(surfaceDecoderFilter_->colorSpace_, 0);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS