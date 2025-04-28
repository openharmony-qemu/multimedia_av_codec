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

#include "super_resolution_post_processor_unit_test.h"

#include "gmock/gmock.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {

void SuperResolutionPostProcessorUnitTest::SetUpTestCase(void) {}

void SuperResolutionPostProcessorUnitTest::TearDownTestCase(void) {}

void SuperResolutionPostProcessorUnitTest::SetUp(void)
{
    postProcessor_ = std::make_shared<SuperResolutionPostProcessor>();
    EXPECT_NE(postProcessor_, nullptr);
    callback_ = std::make_shared<MockPostProcessorCallback>();
    postProcessor_->SetCallback(callback_);
    EXPECT_NE(postProcessor_->filterCallback_, nullptr);
}

void SuperResolutionPostProcessorUnitTest::TearDown(void)
{
    postProcessor_ = nullptr;
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Supported_001, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();

    meta->SetData(Tag::VIDEO_WIDTH, 1280);
    meta->SetData(Tag::VIDEO_HEIGHT, 720);
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    meta->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(isSuperResolutionSupported(meta), true);

    meta->SetData(Tag::VIDEO_WIDTH, 1920);
    meta->SetData(Tag::VIDEO_HEIGHT, 1080);
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    meta->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(isSuperResolutionSupported(meta), true);

    meta->SetData(Tag::VIDEO_WIDTH, 0);
    meta->SetData(Tag::VIDEO_HEIGHT, 0);
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    meta->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(isSuperResolutionSupported(meta), false);

    meta->SetData(Tag::VIDEO_WIDTH, 720);
    meta->SetData(Tag::VIDEO_HEIGHT, 480);
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, true);
    meta->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, false);
    EXPECT_EQ(isSuperResolutionSupported(meta), false);

    meta->SetData(Tag::VIDEO_WIDTH, 720);
    meta->SetData(Tag::VIDEO_HEIGHT, 480);
    meta->SetData(Tag::VIDEO_IS_HDR_VIVID, false);
    meta->SetData(Tag::AV_PLAYER_IS_DRM_PROTECTED, true);
    EXPECT_EQ(isSuperResolutionSupported(meta), false);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_VpeCallback_001, TestSize.Level1)
{
    Format format;
    VpeBufferSize bufferSize;
    VpeBufferInfo bufferInfo;
    VPECallback vpeCb(nullptr);

    vpeCb.OnError(VPEAlgoErrCode::VPE_ALGO_ERR_OK);
    vpeCb.OnEffectChange(0);
    format.PutBuffer(ParameterKey::DETAIL_ENHANCER_TARGET_SIZE, reinterpret_cast<uint8_t*>(&bufferSize),
        sizeof(bufferSize));
    vpeCb.OnOutputFormatChanged(format);
    vpeCb.OnOutputBufferAvailable(0, bufferInfo);
    EXPECT_EQ(vpeCb.postProcessor_.lock(), nullptr);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_VpeCallback_002, TestSize.Level1)
{
    Format format;
    VpeBufferSize bufferSize;
    VpeBufferInfo bufferInfo;
    VPECallback vpeCb(postProcessor_);

    vpeCb.OnError(VPEAlgoErrCode::VPE_ALGO_ERR_OK);
    vpeCb.OnEffectChange(0);
    vpeCb.OnOutputFormatChanged(format);
    bufferSize.width = 1920;
    bufferSize.height = 1080;
    format.PutBuffer(ParameterKey::DETAIL_ENHANCER_TARGET_SIZE, reinterpret_cast<uint8_t*>(&bufferSize),
        sizeof(bufferSize));
    vpeCb.OnOutputFormatChanged(format);
    bufferSize.width = 1920;
    bufferSize.height = -1;
    format.PutBuffer(ParameterKey::DETAIL_ENHANCER_TARGET_SIZE, reinterpret_cast<uint8_t*>(&bufferSize),
        sizeof(bufferSize));
    vpeCb.OnOutputFormatChanged(format);
    bufferSize.width = -1;
    bufferSize.height = 1080;
    format.PutBuffer(ParameterKey::DETAIL_ENHANCER_TARGET_SIZE, reinterpret_cast<uint8_t*>(&bufferSize),
        sizeof(bufferSize));
    vpeCb.OnOutputFormatChanged(format);
    vpeCb.OnOutputBufferAvailable(0, bufferInfo);
    EXPECT_NE(vpeCb.postProcessor_.lock(), nullptr);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Init_001, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        SetParameter(_)).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        RegisterCallback(_)).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->Init(), Status::OK);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Init_002, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        SetParameter(_)).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        RegisterCallback(_)).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->Init(), Status::ERROR_INVALID_PARAMETER);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Init_003, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        SetParameter(_)).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        RegisterCallback(_)).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_EQ(postProcessor_->Init(), Status::ERROR_INVALID_STATE);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Flush_001, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Flush()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->Flush(), Status::OK);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Flush_002, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Flush()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_EQ(postProcessor_->Flush(), Status::ERROR_INVALID_STATE);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Stop_001, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Stop()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Release()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->Stop(), Status::OK);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Stop_002, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Stop()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Release()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->Stop(), Status::ERROR_INVALID_STATE);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Start_001, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Start()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->Start(), Status::OK);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Start_002, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Start()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_EQ(postProcessor_->Start(), Status::ERROR_INVALID_STATE);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Release_001, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Release()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->Release(), Status::OK);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_Release_002, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Release()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_EQ(postProcessor_->Release(), Status::ERROR_INVALID_STATE);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_OnOutputBufferAvailable_001, TestSize.Level1)
{
    uint32_t index = 0;
    VpeBufferFlag flag = VPE_BUFFER_FLAG_NONE;
    postProcessor_->OnOutputBufferAvailable(index, flag);
    EXPECT_FALSE(callback_->buffer_->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS));
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_OnOutputBufferAvailable_002, TestSize.Level1)
{
    uint32_t index = 0;
    VpeBufferFlag flag = VPE_BUFFER_FLAG_EOS;
    postProcessor_->OnOutputBufferAvailable(index, flag);
    EXPECT_TRUE(callback_->buffer_->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS));
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_OnOutputBufferAvailable_003, TestSize.Level1)
{
    uint32_t index = 0;
    VpeBufferInfo bufferInfo;
    bufferInfo.flag = VPE_BUFFER_FLAG_NONE;
    postProcessor_->OnOutputBufferAvailable(index, bufferInfo);
    EXPECT_FALSE(callback_->buffer_->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS));
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_OnOutputBufferAvailable_004, TestSize.Level1)
{
    uint32_t index = 0;
    VpeBufferInfo bufferInfo;
    bufferInfo.flag = VPE_BUFFER_FLAG_EOS;
    postProcessor_->OnOutputBufferAvailable(index, bufferInfo);
    EXPECT_TRUE(callback_->buffer_->flag_ & static_cast<uint32_t>(Plugins::AVBufferFlag::EOS));
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_SetPostProcessorOn_001, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Enable()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Disable()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_OK));
    EXPECT_EQ(postProcessor_->SetPostProcessorOn(true), Status::OK);
    EXPECT_EQ(postProcessor_->SetPostProcessorOn(false), Status::OK);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_SetPostProcessorOn_002, TestSize.Level1)
{
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Enable()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_CALL(*(postProcessor_->postProcessor_),
        Disable()).WillRepeatedly(Return(VPEAlgoErrCode::VPE_ALGO_ERR_UNKNOWN));
    EXPECT_EQ(postProcessor_->SetPostProcessorOn(true), Status::ERROR_INVALID_STATE);
    EXPECT_EQ(postProcessor_->SetPostProcessorOn(false), Status::ERROR_INVALID_STATE);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_OnSuperResolutionChanged_001, TestSize.Level1)
{
    postProcessor_->eventReceiver_ = nullptr;
    postProcessor_->OnSuperResolutionChanged(false);
    EXPECT_EQ(postProcessor_->isPostProcessorOn_, false);
    postProcessor_->OnSuperResolutionChanged(true);
    EXPECT_EQ(postProcessor_->isPostProcessorOn_, true);
}

HWTEST_F(SuperResolutionPostProcessorUnitTest, SuperResolution_OnSuperResolutionChanged_002, TestSize.Level1)
{
    postProcessor_->eventReceiver_ = std::make_shared<MockEventReceiver>();
    postProcessor_->OnSuperResolutionChanged(false);
    EXPECT_EQ(postProcessor_->isPostProcessorOn_, false);
    postProcessor_->OnSuperResolutionChanged(true);
    EXPECT_EQ(postProcessor_->isPostProcessorOn_, true);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
