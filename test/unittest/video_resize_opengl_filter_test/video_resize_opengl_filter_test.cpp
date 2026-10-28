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

#include <memory>
#include "video_resize_opengl_filter_test.h"
#include "meta/video_types.h"
#include "surface/native_buffer.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;
using OHOS::Media::Pipeline::StreamType;
using OHOS::Media::Status;

const static int32_t TEST_TIMES_ONE = 1;

namespace OHOS {
namespace Media {
namespace Pipeline {

void VideoResizeOpenGLFilterTest::SetUpTestCase(void) {}

void VideoResizeOpenGLFilterTest::TearDownTestCase(void) {}

void VideoResizeOpenGLFilterTest::SetUp(void)
{
    videoResizeFilter_ = std::make_shared<VideoResizeOpenGLFilter>(
        "videoResizeOpenGLFilter", FilterType::FILTERTYPE_VIDRESIZE_OPENGL);
    ASSERT_TRUE(videoResizeFilter_ != nullptr);

    eventReceiver_ = std::make_shared<OHOS::Media::Pipeline::MockEventReceiver>();
    ASSERT_TRUE(eventReceiver_ != nullptr);

    filterCallback_ = std::make_shared<MockFilterCallback>();
    ASSERT_TRUE(filterCallback_ != nullptr);

    onLinkedResultCallback_ = std::make_shared<MockFilterLinkCallback>();
    ASSERT_TRUE(onLinkedResultCallback_ != nullptr);

    nextFilter_ = std::make_shared<MockFilter>("test", FilterType::FILTERTYPE_FSINK, false);
    ASSERT_TRUE(nextFilter_ != nullptr);

    videoResizeFilter_->eventReceiver_ = eventReceiver_;
    videoResizeFilter_->filterCallback_ = filterCallback_;
    videoResizeFilter_->onLinkedResultCallback_ = onLinkedResultCallback_;
    videoResizeFilter_->nextFilter_ = nextFilter_;
}

void VideoResizeOpenGLFilterTest::TearDown(void)
{
    videoResizeFilter_ = nullptr;
    eventReceiver_ = nullptr;
    filterCallback_ = nullptr;
    onLinkedResultCallback_ = nullptr;
    nextFilter_ = nullptr;
}

// ---- Construction & filter type ----

/**
 * @tc.name  : Test Construction
 * @tc.number: Construction_001
 * @tc.desc  : VideoResizeOpenGLFilter constructs with correct filter type
 */
HWTEST_F(VideoResizeOpenGLFilterTest, Construction_001, TestSize.Level1)
{
    auto filter = std::make_shared<VideoResizeOpenGLFilter>(
        "testFilter", FilterType::FILTERTYPE_VIDRESIZE_OPENGL);
    ASSERT_TRUE(filter != nullptr);
    EXPECT_EQ(filter->GetFilterType(), FilterType::FILTERTYPE_VIDRESIZE_OPENGL);
}

/**
 * @tc.name  : Test GetFilterType
 * @tc.number: GetFilterType_001
 * @tc.desc  : GetFilterType returns FILTERTYPE_VIDRESIZE_OPENGL
 */
HWTEST_F(VideoResizeOpenGLFilterTest, GetFilterType_001, TestSize.Level1)
{
    FilterType ret = videoResizeFilter_->GetFilterType();
    EXPECT_EQ(ret, FilterType::FILTERTYPE_VIDRESIZE_OPENGL);
}

// ---- OnSetCodecFormat: SDR vs HDR color space detection ----

/**
 * @tc.name  : Test OnSetCodecFormat SDR
 * @tc.number: OnSetCodecFormat_001
 * @tc.desc  : OnSetCodecFormat with SDR BT709 color space sets isHdrMode_ = false
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_001, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
    EXPECT_EQ(videoResizeFilter_->dstColorSpace_,
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT));
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR BT2020 HLG
 * @tc.number: OnSetCodecFormat_002
 * @tc.desc  : OnSetCodecFormat with BT2020 HLG FULL sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_002, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_FULL));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR BT2020 HLG LIMIT
 * @tc.number: OnSetCodecFormat_003
 * @tc.desc  : OnSetCodecFormat with BT2020 HLG LIMIT sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_003, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR BT2020 PQ FULL
 * @tc.number: OnSetCodecFormat_004
 * @tc.desc  : OnSetCodecFormat with BT2020 PQ FULL sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_004, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_FULL));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR BT2020 PQ LIMIT
 * @tc.number: OnSetCodecFormat_005
 * @tc.desc  : OnSetCodecFormat with BT2020 PQ LIMIT sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_005, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_LIMIT));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR P3 HLG FULL
 * @tc.number: OnSetCodecFormat_006
 * @tc.desc  : OnSetCodecFormat with P3 HLG FULL sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_006, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_HLG_FULL));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR P3 HLG LIMIT
 * @tc.number: OnSetCodecFormat_007
 * @tc.desc  : OnSetCodecFormat with P3 HLG LIMIT sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_007, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_HLG_LIMIT));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR P3 PQ FULL
 * @tc.number: OnSetCodecFormat_008
 * @tc.desc  : OnSetCodecFormat with P3 PQ FULL sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_008, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_PQ_FULL));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat HDR P3 PQ LIMIT
 * @tc.number: OnSetCodecFormat_009
 * @tc.desc  : OnSetCodecFormat with P3 PQ LIMIT sets isHdrMode_ = true
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_009, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_PQ_LIMIT));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat SDR BT709 FULL
 * @tc.number: OnSetCodecFormat_010
 * @tc.desc  : OnSetCodecFormat with BT709 FULL sets isHdrMode_ = false
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_010, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_FULL));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat SDR BT601 EBU LIMIT
 * @tc.number: OnSetCodecFormat_011
 * @tc.desc  : OnSetCodecFormat with BT601 EBU LIMIT sets isHdrMode_ = false
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_011, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT601_EBU_LIMIT));
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test OnSetCodecFormat default (0)
 * @tc.number: OnSetCodecFormat_012
 * @tc.desc  : OnSetCodecFormat with colorSpace=0 (NONE) sets isHdrMode_ = false
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_012, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(0);
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
    EXPECT_EQ(videoResizeFilter_->dstColorSpace_, 0);
}

/**
 * @tc.name  : Test OnSetCodecFormat null format
 * @tc.number: OnSetCodecFormat_013
 * @tc.desc  : OnSetCodecFormat with null format returns early (no crash, no state change)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_013, TestSize.Level1)
{
    videoResizeFilter_->isHdrMode_ = false;
    videoResizeFilter_->dstColorSpace_ = 0;
    std::shared_ptr<Meta> format = nullptr;
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
    EXPECT_EQ(videoResizeFilter_->dstColorSpace_, 0);
}

/**
 * @tc.name  : Test OnSetCodecFormat no color space key
 * @tc.number: OnSetCodecFormat_014
 * @tc.desc  : OnSetCodecFormat with format missing AV_TRANSCODER_DST_COLOR_SPACE leaves default state
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnSetCodecFormat_014, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    // No AV_TRANSCODER_DST_COLOR_SPACE set
    videoResizeFilter_->OnSetCodecFormat(format);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
}

// ---- SetCodecFormat (base class wrapper) ----

/**
 * @tc.name  : Test SetCodecFormat
 * @tc.number: SetCodecFormat_001
 * @tc.desc  : SetCodecFormat delegates to OnSetCodecFormat and returns OK
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetCodecFormat_001, TestSize.Level1)
{
    auto format = std::make_shared<Meta>();
    ASSERT_TRUE(format != nullptr);
    format->Set<Tag::AV_TRANSCODER_DST_COLOR_SPACE>(
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_FULL));
    Status ret = videoResizeFilter_->SetCodecFormat(format);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test SetCodecFormat null
 * @tc.number: SetCodecFormat_002
 * @tc.desc  : SetCodecFormat with null format returns OK
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetCodecFormat_002, TestSize.Level1)
{
    std::shared_ptr<Meta> format = nullptr;
    Status ret = videoResizeFilter_->SetCodecFormat(format);
    EXPECT_EQ(ret, Status::OK);
}

// ---- IsHdrColorSpace: all 8 HDR color spaces + SDR cases ----

/**
 * @tc.name  : Test IsHdrColorSpace BT2020 HLG FULL
 * @tc.number: IsHdrColorSpace_001
 * @tc.desc  : IsHdrColorSpace returns true for BT2020 HLG FULL
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_001, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_FULL);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace BT2020 HLG LIMIT
 * @tc.number: IsHdrColorSpace_002
 * @tc.desc  : IsHdrColorSpace returns true for BT2020 HLG LIMIT
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_002, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace BT2020 PQ FULL
 * @tc.number: IsHdrColorSpace_003
 * @tc.desc  : IsHdrColorSpace returns true for BT2020 PQ FULL
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_003, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_FULL);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace BT2020 PQ LIMIT
 * @tc.number: IsHdrColorSpace_004
 * @tc.desc  : IsHdrColorSpace returns true for BT2020 PQ LIMIT
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_004, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_LIMIT);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace P3 HLG FULL
 * @tc.number: IsHdrColorSpace_005
 * @tc.desc  : IsHdrColorSpace returns true for P3 HLG FULL
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_005, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_HLG_FULL);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace P3 HLG LIMIT
 * @tc.number: IsHdrColorSpace_006
 * @tc.desc  : IsHdrColorSpace returns true for P3 HLG LIMIT
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_006, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_HLG_LIMIT);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace P3 PQ FULL
 * @tc.number: IsHdrColorSpace_007
 * @tc.desc  : IsHdrColorSpace returns true for P3 PQ FULL
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_007, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_PQ_FULL);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace P3 PQ LIMIT
 * @tc.number: IsHdrColorSpace_008
 * @tc.desc  : IsHdrColorSpace returns true for P3 PQ LIMIT
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_008, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_P3_PQ_LIMIT);
    EXPECT_TRUE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace SDR BT709
 * @tc.number: IsHdrColorSpace_009
 * @tc.desc  : IsHdrColorSpace returns false for SDR BT709 LIMIT
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_009, TestSize.Level1)
{
    int32_t cs = static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT709_LIMIT);
    EXPECT_FALSE(videoResizeFilter_->IsHdrColorSpace(cs));
}

/**
 * @tc.name  : Test IsHdrColorSpace zero
 * @tc.number: IsHdrColorSpace_010
 * @tc.desc  : IsHdrColorSpace returns false for 0 (OH_COLORSPACE_NONE)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_010, TestSize.Level1)
{
    EXPECT_FALSE(videoResizeFilter_->IsHdrColorSpace(0));
}

/**
 * @tc.name  : Test IsHdrColorSpace negative
 * @tc.number: IsHdrColorSpace_011
 * @tc.desc  : IsHdrColorSpace returns false for negative value
 */
HWTEST_F(VideoResizeOpenGLFilterTest, IsHdrColorSpace_011, TestSize.Level1)
{
    EXPECT_FALSE(videoResizeFilter_->IsHdrColorSpace(-1));
}

// ---- DoPrepare ----

/**
 * @tc.name  : Test DoPrepare with null callback
 * @tc.number: DoPrepare_001
 * @tc.desc  : DoPrepare with null filterCallback_ returns ERROR_UNKNOWN
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoPrepare_001, TestSize.Level1)
{
    videoResizeFilter_->filterCallback_ = nullptr;
    Status ret = videoResizeFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test DoPrepare with valid callback
 * @tc.number: DoPrepare_002
 * @tc.desc  : DoPrepare with valid filterCallback_ invokes OnCallback with NEXT_FILTER_NEEDED
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoPrepare_002, TestSize.Level1)
{
    EXPECT_CALL(*(filterCallback_), OnCallback(testing::_, testing::_, testing::_))
        .WillOnce(testing::Return(Status::OK));
    Status ret = videoResizeFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}

// ---- ConfigureInputSurface / ConfigureOutputSurface ----

/**
 * @tc.name  : Test ConfigureInputSurface SDR
 * @tc.number: ConfigureInputSurface_001
 * @tc.desc  : ConfigureInputSurface in SDR mode with null nativeImageWindow_ returns early (no crash)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, ConfigureInputSurface_001, TestSize.Level1)
{
    videoResizeFilter_->isHdrMode_ = false;
    videoResizeFilter_->nativeImageWindow_ = nullptr;
    videoResizeFilter_->ConfigureInputSurface();
    // SDR mode: FALSE_RETURN_NOLOG guard skips GL calls; window must remain unchanged
    EXPECT_EQ(videoResizeFilter_->nativeImageWindow_, nullptr);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test ConfigureInputSurface HDR null window
 * @tc.number: ConfigureInputSurface_002
 * @tc.desc  : ConfigureInputSurface in HDR mode with null nativeImageWindow_ returns early (no crash)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, ConfigureInputSurface_002, TestSize.Level1)
{
    videoResizeFilter_->isHdrMode_ = true;
    videoResizeFilter_->nativeImageWindow_ = nullptr;
    videoResizeFilter_->dstColorSpace_ =
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_PQ_LIMIT);
    videoResizeFilter_->ConfigureInputSurface();
    // HDR mode with null window: FALSE_RETURN_NOLOG guard skips GL calls; state unchanged
    EXPECT_EQ(videoResizeFilter_->nativeImageWindow_, nullptr);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test ConfigureOutputSurface SDR
 * @tc.number: ConfigureOutputSurface_001
 * @tc.desc  : ConfigureOutputSurface in SDR mode with null nativeWindow_ returns early (no crash)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, ConfigureOutputSurface_001, TestSize.Level1)
{
    videoResizeFilter_->isHdrMode_ = false;
    videoResizeFilter_->nativeWindow_ = nullptr;
    videoResizeFilter_->ConfigureOutputSurface();
    // SDR mode: FALSE_RETURN_NOLOG guard skips GL calls; window must remain unchanged
    EXPECT_EQ(videoResizeFilter_->nativeWindow_, nullptr);
    EXPECT_FALSE(videoResizeFilter_->isHdrMode_);
}

/**
 * @tc.name  : Test ConfigureOutputSurface HDR null window
 * @tc.number: ConfigureOutputSurface_002
 * @tc.desc  : ConfigureOutputSurface in HDR mode with null nativeWindow_ returns early (no crash)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, ConfigureOutputSurface_002, TestSize.Level1)
{
    videoResizeFilter_->isHdrMode_ = true;
    videoResizeFilter_->nativeWindow_ = nullptr;
    videoResizeFilter_->dstColorSpace_ =
        static_cast<int32_t>(OH_NativeBuffer_ColorSpace::OH_COLORSPACE_BT2020_HLG_LIMIT);
    videoResizeFilter_->ConfigureOutputSurface();
    // HDR mode with null window: FALSE_RETURN_NOLOG guard skips GL calls; state unchanged
    EXPECT_EQ(videoResizeFilter_->nativeWindow_, nullptr);
    EXPECT_TRUE(videoResizeFilter_->isHdrMode_);
}

// ---- OnReleaseGpuResources ----

/**
 * @tc.name  : Test OnReleaseGpuResources default state
 * @tc.number: OnReleaseGpuResources_001
 * @tc.desc  : OnReleaseGpuResources with default state (no GL resources allocated) does not crash
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnReleaseGpuResources_001, TestSize.Level1)
{
    videoResizeFilter_->outputFBO_ = 0;
    videoResizeFilter_->outputTexId_ = DEFINED_VALUE;
    videoResizeFilter_->yuvConvertProgram_ = nullptr;
    videoResizeFilter_->OnReleaseGpuResources();
    EXPECT_EQ(videoResizeFilter_->outputFBO_, 0);
    EXPECT_EQ(videoResizeFilter_->outputTexId_, DEFINED_VALUE);  // sentinel value preserved (not deleted)
    EXPECT_EQ(videoResizeFilter_->yuvConvertProgram_, nullptr);
}

/**
 * @tc.name  : Test OnReleaseGpuResources idempotency
 * @tc.number: OnReleaseGpuResources_002
 * @tc.desc  : OnReleaseGpuResources called twice does not crash (idempotent)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnReleaseGpuResources_002, TestSize.Level1)
{
    videoResizeFilter_->outputFBO_ = 0;
    videoResizeFilter_->outputTexId_ = 0;
    videoResizeFilter_->yuvConvertProgram_ = nullptr;
    videoResizeFilter_->OnReleaseGpuResources();
    videoResizeFilter_->OnReleaseGpuResources();
    // Idempotent: double release resets FBO/texture to 0 and program to nullptr
    EXPECT_EQ(videoResizeFilter_->outputFBO_, 0);
    EXPECT_EQ(videoResizeFilter_->outputTexId_, 0);
    EXPECT_EQ(videoResizeFilter_->yuvConvertProgram_, nullptr);
}

// ---- Base class lifecycle methods (inherited) ----

/**
 * @tc.name  : Test DoStart
 * @tc.number: DoStart_001
 * @tc.desc  : DoStart returns OK (base class no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoStart_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->DoStart();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoPause
 * @tc.number: DoPause_001
 * @tc.desc  : DoPause returns OK (base class no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoPause_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoResume
 * @tc.number: DoResume_001
 * @tc.desc  : DoResume returns OK (base class no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoResume_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoStop
 * @tc.number: DoStop_001
 * @tc.desc  : DoStop returns OK (base class no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoStop_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->DoStop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoFlush
 * @tc.number: DoFlush_001
 * @tc.desc  : DoFlush returns OK (base class no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoFlush_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->DoFlush();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoRelease
 * @tc.number: DoRelease_001
 * @tc.desc  : DoRelease with no GL/EGL resources allocated returns OK
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoRelease_001, TestSize.Level1)
{
    videoResizeFilter_->nativeImage_ = nullptr;
    videoResizeFilter_->nativeWindow_ = nullptr;
    Status ret = videoResizeFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoRelease idempotency
 * @tc.number: DoRelease_002
 * @tc.desc  : DoRelease called twice returns OK both times (hasReleased_ guard)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, DoRelease_002, TestSize.Level1)
{
    videoResizeFilter_->nativeImage_ = nullptr;
    videoResizeFilter_->nativeWindow_ = nullptr;
    Status ret1 = videoResizeFilter_->DoRelease();
    EXPECT_EQ(ret1, Status::OK);
    Status ret2 = videoResizeFilter_->DoRelease();
    EXPECT_EQ(ret2, Status::OK);
}

// ---- SetVideoResize (base class) ----

/**
 * @tc.name  : Test SetVideoResize valid
 * @tc.number: SetVideoResize_001
 * @tc.desc  : SetVideoResize with valid dimensions sets video/target dimensions and returns OK
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetVideoResize_001, TestSize.Level1)
{
    videoResizeFilter_->nativeImage_ = nullptr;  // Avoid InitOpenGl creating native image
    Status ret = videoResizeFilter_->SetVideoResize(1920, 1080);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoResizeFilter_->videoWidth_.load(), 1920);
    EXPECT_EQ(videoResizeFilter_->videoHeight_.load(), 1080);
    EXPECT_EQ(videoResizeFilter_->targetWidth_.load(), 1920);
    EXPECT_EQ(videoResizeFilter_->targetHeight_.load(), 1080);
}

/**
 * @tc.name  : Test SetVideoResize invalid width
 * @tc.number: SetVideoResize_002
 * @tc.desc  : SetVideoResize with zero width returns ERROR_INVALID_PARAMETER
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetVideoResize_002, TestSize.Level1)
{
    Status ret = videoResizeFilter_->SetVideoResize(0, 1080);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name  : Test SetVideoResize invalid height
 * @tc.number: SetVideoResize_003
 * @tc.desc  : SetVideoResize with negative height returns ERROR_INVALID_PARAMETER
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetVideoResize_003, TestSize.Level1)
{
    Status ret = videoResizeFilter_->SetVideoResize(1920, -1);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

// ---- Init / SetCallingInfo / SetFaultEvent ----

/**
 * @tc.name  : Test Init
 * @tc.number: Init_001
 * @tc.desc  : Init stores eventReceiver and filterCallback
 */
HWTEST_F(VideoResizeOpenGLFilterTest, Init_001, TestSize.Level1)
{
    auto receiver = std::make_shared<MockEventReceiver>();
    auto callback = std::make_shared<MockFilterCallback>();
    ASSERT_TRUE(receiver != nullptr);
    ASSERT_TRUE(callback != nullptr);
    videoResizeFilter_->Init(receiver, callback);
    EXPECT_EQ(videoResizeFilter_->eventReceiver_, receiver);
    EXPECT_EQ(videoResizeFilter_->filterCallback_, callback);
}

/**
 * @tc.name  : Test SetCallingInfo
 * @tc.number: SetCallingInfo_001
 * @tc.desc  : SetCallingInfo stores app uid/pid/bundleName/instanceId
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetCallingInfo_001, TestSize.Level1)
{
    videoResizeFilter_->SetCallingInfo(100, 200, "test_bundle", 300);
    EXPECT_EQ(videoResizeFilter_->appUid_, 100);
    EXPECT_EQ(videoResizeFilter_->appPid_, 200);
    EXPECT_EQ(videoResizeFilter_->bundleName_, "test_bundle");
    EXPECT_EQ(videoResizeFilter_->instanceId_, 300);
}

/**
 * @tc.name  : Test SetFaultEvent
 * @tc.number: SetFaultEvent_001
 * @tc.desc  : SetFaultEvent with message does not crash and leaves caller state unchanged
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetFaultEvent_001, TestSize.Level1)
{
    videoResizeFilter_->SetCallingInfo(100, 200, "test_bundle", 300);
    videoResizeFilter_->SetFaultEvent("test error");
    // SetFaultEvent only writes a fault event; verify caller state is unchanged
    EXPECT_EQ(videoResizeFilter_->appUid_, 100);
    EXPECT_EQ(videoResizeFilter_->bundleName_, "test_bundle");
}

/**
 * @tc.name  : Test SetFaultEvent with ret
 * @tc.number: SetFaultEvent_002
 * @tc.desc  : SetFaultEvent with message and ret does not crash and leaves caller state unchanged
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetFaultEvent_002, TestSize.Level1)
{
    videoResizeFilter_->SetCallingInfo(100, 200, "test_bundle", 300);
    videoResizeFilter_->SetFaultEvent("test error", 500);
    EXPECT_EQ(videoResizeFilter_->appUid_, 100);
    EXPECT_EQ(videoResizeFilter_->instanceId_, 300);
}

// ---- Configure / SetParameter / GetParameter ----

/**
 * @tc.name  : Test Configure
 * @tc.number: Configure_001
 * @tc.desc  : Configure stores parameter in configureParameter_
 */
HWTEST_F(VideoResizeOpenGLFilterTest, Configure_001, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    parameter->Set<Tag::VIDEO_WIDTH>(1280);
    Status ret = videoResizeFilter_->Configure(parameter);
    EXPECT_EQ(ret, Status::OK);
    ASSERT_NE(videoResizeFilter_->configureParameter_, nullptr);
    int32_t width = 0;
    videoResizeFilter_->configureParameter_->Get<Tag::VIDEO_WIDTH>(width);
    EXPECT_EQ(width, 1280);
}

/**
 * @tc.name  : Test SetParameter EOS
 * @tc.number: SetParameter_001
 * @tc.desc  : SetParameter with USER_FRAME_PTS stores pts and sets eosReceived_
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetParameter_001, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    parameter->Set<Tag::USER_FRAME_PTS>(12345);
    parameter->Set<Tag::USER_PUSH_DATA_TIME>(10);
    videoResizeFilter_->SetParameter(parameter);
    EXPECT_EQ(videoResizeFilter_->eosPts_.load(), 12345);
    EXPECT_EQ(videoResizeFilter_->frameNum_, 10);
    EXPECT_TRUE(videoResizeFilter_->eosReceived_.load());
}

/**
 * @tc.name  : Test SetParameter null
 * @tc.number: SetParameter_002
 * @tc.desc  : SetParameter with nullptr does not crash and does not set eosReceived_
 */
HWTEST_F(VideoResizeOpenGLFilterTest, SetParameter_002, TestSize.Level1)
{
    videoResizeFilter_->eosReceived_.store(false);
    std::shared_ptr<Meta> parameter = nullptr;
    videoResizeFilter_->SetParameter(parameter);
    EXPECT_FALSE(videoResizeFilter_->eosReceived_.load());
}

/**
 * @tc.name  : Test GetParameter
 * @tc.number: GetParameter_001
 * @tc.desc  : GetParameter is a no-op and leaves the meta unchanged
 */
HWTEST_F(VideoResizeOpenGLFilterTest, GetParameter_001, TestSize.Level1)
{
    auto parameter = std::make_shared<Meta>();
    ASSERT_TRUE(parameter != nullptr);
    parameter->Set<Tag::USER_FRAME_PTS>(999);
    videoResizeFilter_->GetParameter(parameter);
    // GetParameter is a base no-op; verify meta is not modified
    int64_t pts = 0;
    EXPECT_TRUE(parameter->Get<Tag::USER_FRAME_PTS>(pts));
    EXPECT_EQ(pts, 999);
}

// ---- OnLinked / OnUpdated / OnUnLinked / OnLinkedResult ----

/**
 * @tc.name  : Test OnLinked
 * @tc.number: OnLinked_001
 * @tc.desc  : OnLinked stores callback in onLinkedResultCallback_
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnLinked_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    Status ret = videoResizeFilter_->OnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, meta, onLinkedResultCallback_);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(videoResizeFilter_->onLinkedResultCallback_, onLinkedResultCallback_);
}

/**
 * @tc.name  : Test OnUpdated
 * @tc.number: OnUpdated_001
 * @tc.desc  : OnUpdated returns OK (base no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnUpdated_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    Status ret = videoResizeFilter_->OnUpdated(StreamType::STREAMTYPE_ENCODED_VIDEO, meta, onLinkedResultCallback_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test OnUnLinked
 * @tc.number: OnUnLinked_001
 * @tc.desc  : OnUnLinked returns OK (base no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnUnLinked_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->OnUnLinked(StreamType::STREAMTYPE_ENCODED_VIDEO, onLinkedResultCallback_);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test OnLinkedResult null meta
 * @tc.number: OnLinkedResult_001
 * @tc.desc  : OnLinkedResult with null meta returns early (no crash, callback not invoked)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnLinkedResult_001, TestSize.Level1)
{
    auto outputBufferQueue = sptr<AVBufferQueueProducer>(nullptr);
    std::shared_ptr<Meta> meta = nullptr;
    // No EXPECT_CALL on onLinkedResultCallback_ — verify null meta early-returns without invoking it
    videoResizeFilter_->OnLinkedResult(outputBufferQueue, meta);
    EXPECT_EQ(videoResizeFilter_->onLinkedResultCallback_, onLinkedResultCallback_);
}

/**
 * @tc.name  : Test OnLinkedResult with callback
 * @tc.number: OnLinkedResult_002
 * @tc.desc  : OnLinkedResult with callback invokes OnLinkedResult on the callback
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnLinkedResult_002, TestSize.Level1)
{
    auto outputBufferQueue = sptr<AVBufferQueueProducer>(nullptr);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    EXPECT_CALL(*(onLinkedResultCallback_), OnLinkedResult(testing::_, testing::_)).Times(TEST_TIMES_ONE);
    videoResizeFilter_->OnLinkedResult(outputBufferQueue, meta);
}

/**
 * @tc.name  : Test OnUpdatedResult
 * @tc.number: OnUpdatedResult_001
 * @tc.desc  : OnUpdatedResult does not crash (base no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnUpdatedResult_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    videoResizeFilter_->OnUpdatedResult(meta);
    SUCCEED();
}

/**
 * @tc.name  : Test OnUnlinkedResult
 * @tc.number: OnUnlinkedResult_001
 * @tc.desc  : OnUnlinkedResult does not crash (base no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnUnlinkedResult_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    videoResizeFilter_->OnUnlinkedResult(meta);
    SUCCEED();
}

// ---- NotifyNextFilterEos / UpdateNext / UnLinkNext ----

/**
 * @tc.name  : Test NotifyNextFilterEos empty
 * @tc.number: NotifyNextFilterEos_001
 * @tc.desc  : NotifyNextFilterEos with empty nextFiltersMap returns OK
 */
HWTEST_F(VideoResizeOpenGLFilterTest, NotifyNextFilterEos_001, TestSize.Level1)
{
    videoResizeFilter_->nextFiltersMap_.clear();
    Status ret = videoResizeFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test NotifyNextFilterEos null filter
 * @tc.number: NotifyNextFilterEos_002
 * @tc.desc  : NotifyNextFilterEos with null filter returns ERROR_UNKNOWN
 */
HWTEST_F(VideoResizeOpenGLFilterTest, NotifyNextFilterEos_002, TestSize.Level1)
{
    videoResizeFilter_->nextFiltersMap_[StreamType::STREAMTYPE_RAW_VIDEO].push_back(nullptr);
    videoResizeFilter_->eosPts_.store(100);
    Status ret = videoResizeFilter_->NotifyNextFilterEos();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test UpdateNext
 * @tc.number: UpdateNext_001
 * @tc.desc  : UpdateNext returns OK (base no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, UpdateNext_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->UpdateNext(nextFilter_, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test UnLinkNext
 * @tc.number: UnLinkNext_001
 * @tc.desc  : UnLinkNext returns OK (base no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, UnLinkNext_001, TestSize.Level1)
{
    Status ret = videoResizeFilter_->UnLinkNext(nextFilter_, StreamType::STREAMTYPE_ENCODED_VIDEO);
    EXPECT_EQ(ret, Status::OK);
}

// ---- GetInputSurface ----

/**
 * @tc.name  : Test GetInputSurface null window
 * @tc.number: GetInputSurface_001
 * @tc.desc  : GetInputSurface with null nativeImageWindow_ returns nullptr
 */
HWTEST_F(VideoResizeOpenGLFilterTest, GetInputSurface_001, TestSize.Level1)
{
    videoResizeFilter_->nativeImageWindow_ = nullptr;
    sptr<Surface> ret = videoResizeFilter_->GetInputSurface();
    EXPECT_EQ(ret, nullptr);
}

// ---- OnOutputBufferAvailable ----

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_001
 * @tc.desc  : OnOutputBufferAvailable does not crash (base no-op)
 */
HWTEST_F(VideoResizeOpenGLFilterTest, OnOutputBufferAvailable_001, TestSize.Level1)
{
    videoResizeFilter_->OnOutputBufferAvailable(1, 0);
    SUCCEED();
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:video_resize_opengl_filter_test.xml";
    testing::InitGoogleTest(&argc, argv);
    int ret = RUN_ALL_TESTS();
    _exit(ret);
}
