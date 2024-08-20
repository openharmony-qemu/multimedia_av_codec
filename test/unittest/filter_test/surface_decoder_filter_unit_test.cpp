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
    EXPECT_EQ(surfaceDecoderFilter_->Configure(format), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoStart(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoPause(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoResume(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderFilter_->DoStop(), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->DoRelease(), Status::OK);

    surfaceDecoderFilter_->SetParameter(nullptr);

    std::shared_ptr<EventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<FilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    surfaceDecoderFilter_->Init(eventReceive, filterCallback);

    EXPECT_EQ(surfaceDecoderFilter_->Configure(format), Status::ERROR_UNKNOWN);
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
    EXPECT_EQ(surfaceDecoderFilter_->NotifyNextFilterEos(), Status::OK);

    std::shared_ptr<Meta> meta;
    surfaceDecoderFilter_->OnUpdatedResult(meta);
    surfaceDecoderFilter_->OnUnlinkedResult(meta);

    EXPECT_EQ(surfaceDecoderFilter_->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    std::shared_ptr<FilterCallback> filterLinkCallback = std::make_shared<TestFilterCallback>();
    EXPECT_EQ(surfaceDecoderFilter_->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, filterLinkCallback),
        Status::OK);
    EXPECT_EQ(surfaceDecoderFilter_->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, filterLinkCallback),
        Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS