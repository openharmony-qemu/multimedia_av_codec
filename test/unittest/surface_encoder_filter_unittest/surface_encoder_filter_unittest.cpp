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

#include "surface_encoder_filter_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int32_t NUM_0 = 0;

void SurfaceEncoderFilterUnitTest::SetUpTestCase(void) {}

void SurfaceEncoderFilterUnitTest::TearDownTestCase(void) {}

void SurfaceEncoderFilterUnitTest::SetUp(void) {}

void SurfaceEncoderFilterUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test OnLinkedResult
 * @tc.number: SurfaceEncoderFilter_OnLinkedResult_001
 * @tc.desc  : Test SurfaceEncoderFilterLinkCallback_OnUnlinkedResult surfaceEncoderFilter != nullptr
 *             Test SurfaceEncoderFilterLinkCallback_OnUpdatedResult surfaceEncoderFilter != nullptr
 *             Test SurfaceEncoderFilterLinkCallback_OnLinkedResult surfaceEncoderFilter == nullptr
 *             Test SurfaceEncoderFilterLinkCallback_OnUnlinkedResult surfaceEncoderFilter == nullptr
 *             Test SurfaceEncoderFilterLinkCallback_OnUpdatedResult surfaceEncoderFilter == nullptr
 *             Test SurfaceEncoderAdapterCallback_OnError surfaceEncoderFilter == nullptr
 *             Test SurfaceEncoderAdapterKeyFramePtsCallback_OnReportKeyFramePts surfaceEncoderFilter == nullptr
 *             Test SurfaceEncoderAdapterKeyFramePtsCallback_OnReportFirstFramePts surfaceEncoderFilter == nullptr
 */
HWTEST_F(SurfaceEncoderFilterUnitTest, SurfaceEncoderFilter_OnLinkedResult_001, TestSize.Level0)
{
    auto filter = std::make_shared<SurfaceEncoderFilter>("testSurfaceEncoderFilter", FilterType::FILTERTYPE_VDEC);
    ASSERT_NE(filter, nullptr);
    sptr<AVBufferQueueProducer> outputBufferQueue = new MockAVBufferQueueProducer();
    std::shared_ptr<Meta> meta = nullptr;
    filter->mediaCodec_ = std::make_shared<SurfaceEncoderAdapter>();

    // Test SurfaceEncoderFilterLinkCallback_OnUnlinkedResult surfaceEncoderFilter != nullptr
    auto linkCallback = std::make_shared<SurfaceEncoderFilterLinkCallback>(filter);
    linkCallback->OnUnlinkedResult(meta);
    
    // Test SurfaceEncoderFilterLinkCallback_OnUpdatedResult surfaceEncoderFilter != nullptr
    linkCallback->OnUpdatedResult(meta);

    linkCallback->OnLinkedResult(outputBufferQueue, meta);
    EXPECT_NE(filter->mediaCodec_->outputBufferQueueProducer_, nullptr);

    // Test SurfaceEncoderFilterLinkCallback_OnLinkedResult surfaceEncoderFilter == nullptr
    filter = nullptr;
    linkCallback = std::make_shared<SurfaceEncoderFilterLinkCallback>(filter);
    linkCallback->OnLinkedResult(outputBufferQueue, meta);

    // Test SurfaceEncoderFilterLinkCallback_OnUnlinkedResult surfaceEncoderFilter == nullptr
    linkCallback->OnUnlinkedResult(meta);

    // Test SurfaceEncoderFilterLinkCallback_OnUpdatedResult surfaceEncoderFilter == nullptr
    linkCallback->OnUpdatedResult(meta);

    // Test SurfaceEncoderAdapterCallback_OnError surfaceEncoderFilter == nullptr
    auto adapterCallback = std::make_shared<SurfaceEncoderAdapterCallback>(filter);
    adapterCallback->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, NUM_0);

    // Test SurfaceEncoderAdapterKeyFramePtsCallback_OnReportKeyFramePts surfaceEncoderFilter == nullptr
    auto ptsCallback = std::make_shared<SurfaceEncoderAdapterKeyFramePtsCallback>(filter);
    ptsCallback->OnReportKeyFramePts("test");

    // Test SurfaceEncoderAdapterKeyFramePtsCallback_OnReportFirstFramePts surfaceEncoderFilter == nullptr
    ptsCallback->OnReportFirstFramePts(NUM_0);
    
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS