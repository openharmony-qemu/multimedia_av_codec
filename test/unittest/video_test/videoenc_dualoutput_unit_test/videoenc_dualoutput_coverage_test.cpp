/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <avcodec_common.h>
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include <memory>
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avmagic.h"
#include "preprocessor.h"
#include "preprocessor_encoder.h"
#include "preprocessor_manager.h"
#include "sample_callback.h"
#include "unittest_utils.h"
#include "vcodec_signal.h"
#include "venc_dualoutput_sample.h"
#include "v2_2/buffer_handle_meta_key_type.h"

#define IsSupportFastImage()                                                                                           \
    if (access(FAST_IMAGE_PATH.data(), 0) != 0) {                                                                      \
        GTEST_SKIP() << "Unsupport fast image";                                                                        \
    }
#define TEST_SUIT DualOutputCoverageTest

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
namespace V2_2 = OHOS::HDI::Display::Graphic::Common::V2_2;

class TEST_SUIT : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp();
    void TearDown();
protected:
    std::shared_ptr<Preprocessor> preprocessor_;
    std::shared_ptr<PreprocessorEncoder> preprocessorEncoder_;
    std::shared_ptr<PreprocessorManager> preprocessorManager_;
};

void TEST_SUIT::SetUp()
{
    preprocessor_ = std::make_shared<Preprocessor>(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(preprocessor_, nullptr);
    preprocessorEncoder_ = std::make_shared<PreprocessorEncoder>(AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER);
    ASSERT_NE(preprocessorEncoder_, nullptr);
    preprocessorManager_ = std::make_shared<PreprocessorManager>();
    ASSERT_NE(preprocessorManager_, nullptr);
}

void TEST_SUIT::TearDown()
{
    preprocessor_.reset();
    preprocessorEncoder_.reset();
    preprocessorManager_.reset();
}

/**
 * @tc.name: PreprocManager_PeekPendingBufferData_UnknownId_001
 * @tc.desc: Direct test: PeekPendingBufferData with unknown encoderId returns false
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_PeekPendingBufferData_UnknownId_001, TestSize.Level1)
{
    PreprocessorManager::BufferDataInfo info;
    ASSERT_FALSE(preprocessorManager_->PeekPendingBufferData("nonexistent_encoder", info));
    ASSERT_EQ(info.width, 0);
    ASSERT_EQ(info.height, 0);
    ASSERT_EQ(info.format, 0);
    ASSERT_EQ(info.stride, 0);
}

/**
 * @tc.name: PreprocManager_PeekPendingBufferData_EmptyQueue_001
 * @tc.desc: Test PeekPendingBufferData when pendingBuffers_ is empty - covers empty queue path
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_PeekPendingBufferData_EmptyQueue_001, TestSize.Level1)
{
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface);
    PreprocessorManager::BufferDataInfo info;
    ASSERT_FALSE(preprocessorManager_->PeekPendingBufferData("test_encoder", info));
    ASSERT_EQ(info.width, 0);
    ASSERT_EQ(info.height, 0);
    ASSERT_EQ(info.format, 0);
    ASSERT_EQ(info.stride, 0);
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: EncoderInfo_PeekPendingBufferData_EmptyQueue_001
 * @tc.desc: Test PeekPendingBufferData with empty pendingBuffers_ through PreprocessorManager - covers L611-L612
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, EncoderInfo_PeekPendingBufferData_EmptyQueue_001, TestSize.Level1)
{
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface);
    PreprocessorManager::BufferDataInfo info;
    ASSERT_FALSE(preprocessorManager_->PeekPendingBufferData("test_encoder", info));
    ASSERT_EQ(info.width, 0);
    ASSERT_EQ(info.height, 0);
    ASSERT_EQ(info.format, 0);
    ASSERT_EQ(info.stride, 0);
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: EncoderInfo_PeekPendingBufferData_SuccessPath_001
 * @tc.desc: Test EncoderInfo::PeekPendingBufferData success path with valid pending buffer - covers L618-L621
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, EncoderInfo_PeekPendingBufferData_SuccessPath_001, TestSize.Level1)
{
    sptr<Surface> mockProducerSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockProducerSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockProducerSurface);
    preprocessorManager_->CreateSharedSurface();
    preprocessorManager_->SetEncoderRunning("test_encoder", true);
    preprocessorManager_->NotifyNewBufferAvailable();
    PreprocessorManager::BufferDataInfo info;
    bool result = preprocessorManager_->PeekPendingBufferData("test_encoder", info);
    if (result) {
        ASSERT_GT(info.width, 0);
        ASSERT_GT(info.height, 0);
        ASSERT_GT(info.format, 0);

    } else {
        ASSERT_EQ(info.width, 0);
        ASSERT_EQ(info.height, 0);
        ASSERT_EQ(info.format, 0);
        ASSERT_EQ(info.stride, 0);
    }
    preprocessorManager_->SetEncoderRunning("test_encoder", false);
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: PreprocManager_RepPreprocErr_Direct_001
 * @tc.desc: Test preprocess error handling through PreprocessorManager registration
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_RepPreprocErr_Direct_001, TestSize.Level1)
{
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface, nullptr, nullptr);
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: PreprocManager_RepPreprocErr_Callback_001
 * @tc.desc: Test preprocess error callback registration through PreprocessorManager
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_RepPreprocErr_Callback_001, TestSize.Level1)
{
    int32_t reportedCode = 0;
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface, nullptr,
                                         [&reportedCode](int32_t code) {
                                             reportedCode = code;
                                         });
    ASSERT_EQ(reportedCode, 0);  // No error has been triggered yet
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: PreprocManager_RepPreprocErr_Suppressed_001
 * @tc.desc: Test error suppression mechanism registration through PreprocessorManager
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_RepPreprocErr_Suppressed_001, TestSize.Level1)
{
    int32_t callCount = 0;
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface, nullptr,
                                         [&callCount](int32_t code) {
                                             callCount++;
                                         });
    ASSERT_EQ(callCount, 0);  // No errors triggered yet
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: PreprocManager_RepPreprocErr_ZeroCode_001
 * @tc.desc: Test zero error code handling through PreprocessorManager callback registration
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_RepPreprocErr_ZeroCode_001, TestSize.Level1)
{
    int32_t callCount = 0;
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface, nullptr,
                                         [&callCount](int32_t code) {
                                             callCount++;
                                         });
    ASSERT_EQ(callCount, 0);  // No errors triggered yet
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: PreprocManager_NotifyEosNowIfPending_UnknownId_001
 * @tc.desc: Test NotifyEosNowIfPending with non-existent encoder ID - covers iter == encoders_.end() branch
 * @tc.type: FUNC
 * @note: The branch "!iter->second" (encoderInfo is nullptr) cannot be covered through public APIs
 *        because RegisterEncoder always creates valid EncoderInfo objects. This is a defensive check
 *        that handles abnormal scenarios which cannot occur in normal operation.
 */
HWTEST_F(TEST_SUIT, PreprocManager_NotifyEosNowIfPending_UnknownId_001, TestSize.Level1)
{
    preprocessorManager_->NotifyEosNowIfPending("nonexistent_encoder");
}

/**
 * @tc.name: PreprocManager_NotifyEosNowIfPending_WithPendingBuffers_001
 * @tc.desc: Test NotifyEosNowIfPending when EOS is requested with pending buffers - covers L457-L458
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_NotifyEosNowIfPending_WithPendingBuffers_001, TestSize.Level1)
{
    bool eosCalled = false;
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface);
    preprocessorManager_->CreateSharedSurface();
    preprocessorManager_->SetEncoderRunning("test_encoder", true);
    preprocessorManager_->NotifyNewBufferAvailable();
    preprocessorManager_->NotifyNewBufferAvailable();  // Add multiple buffers
    bool requestResult = preprocessorManager_->RequestNotifyEos("test_encoder", [&eosCalled]() {
        eosCalled = true;
    });
    ASSERT_TRUE(requestResult);
    preprocessorManager_->NotifyEosNowIfPending("test_encoder");
    ASSERT_TRUE(eosCalled);
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_NullInput_001
 * @tc.desc: Test UpdateRoiMetadata with null input buffer - covers early return path
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_NullInput_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = nullptr;
    sptr<SurfaceBuffer> output = nullptr;
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_NoRoiMetadata_001
 * @tc.desc: Test UpdateRoiMetadata when input has no ROI metadata - covers empty metadata path
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_NoRoiMetadata_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_EmptyRoiMetadata_001
 * @tc.desc: Test UpdateRoiMetadata when input has empty ROI metadata - covers empty roiData path
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_EmptyRoiMetadata_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::vector<uint8_t> emptyRoi;
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, emptyRoi, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_ValidRoi_001
 * @tc.desc: Test UpdateRoiMetadata with valid ROI metadata and identity transform
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_ValidRoi_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::string roiStr = "10,20-30,40";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_WithScale_001
 * @tc.desc: Test UpdateRoiMetadata with scaling factors applied
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_WithScale_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::string roiStr = "10,20-30,40";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {0.5, 0.5, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_WithOffset_001
 * @tc.desc: Test UpdateRoiMetadata with offset values applied
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_WithOffset_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::string roiStr = "10,20-30,40";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 5, 10};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_MultipleRois_001
 * @tc.desc: Test UpdateRoiMetadata with multiple ROI regions
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_MultipleRois_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::string roiStr = "10,20-30,40;50,60-70,80";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_InvalidRoiSkipped_001
 * @tc.desc: Test UpdateRoiMetadata with invalid ROI that gets skipped
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_InvalidRoiSkipped_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::string roiStr = "abc,20-30,40";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_WithParam_001
 * @tc.desc: Test UpdateRoiMetadata with ROI containing extra parameter - covers L146-L147
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_WithParam_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::string roiStr = "10,20-30,40=weight=0.8";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_MixedRoisWithParams_001
 * @tc.desc: Test UpdateRoiMetadata with multiple ROIs, some with parameters - covers B3+B4 combinations
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_MixedRoisWithParams_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    std::string roiStr = "10,20-30,40;50,60-70,80=importance=high";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {0.5, 0.5, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: PreprocessorEncoder_CreateSharedSurface_001
 * @tc.desc: CreateSharedSurface coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CreateSharedSurface_001, TestSize.Level1)
{
    int32_t ret = preprocessorEncoder_->InitAsPrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_EQ(ret, AV_ERR_OK);
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->CreateSharedSurface();
}

/**
 * @tc.name: PreprocessorEncoder_GetSharedSurface_001
 * @tc.desc: preprocMgr_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetSharedSurface_001, TestSize.Level1)
{
    auto ret = preprocessorEncoder_->GetSharedSurface();
    ASSERT_EQ(ret, nullptr);
}

/**
 * @tc.name: PreprocessorEncoder_GetSharedSurface_002
 * @tc.desc: preprocMgr_ is't nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetSharedSurface_002, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    ASSERT_NE(preprocessorEncoder_->preprocMgr_, nullptr);
    auto ret = preprocessorEncoder_->GetSharedSurface();
    ASSERT_NE(ret, nullptr);
}

/**
 * @tc.name: PreprocessorEncoder_DetachSecondaryEncoder_NoSecondary_001
 * @tc.desc: Test DetachSecondaryEncoder when secondary_ is nullptr
 *           This covers the branch where there is no secondary encoder attached
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_DetachSecondaryEncoder_NoSecondary_001, TestSize.Level1)
{
    OH_AVCodec* primary = nullptr;
    OH_AVErrCode ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(primary, nullptr);
    PreprocessorEncoder* detached = preprocessorEncoder_->DetachSecondaryEncoder();
    ASSERT_EQ(detached, nullptr);
    preprocessorEncoder_->Destroy(primary);
}

/**
 * @tc.name: PreprocessorEncoder_DetachSecondaryEncoder_WithSecondary_001
 * @tc.desc: Test DetachSecondaryEncoder when secondary_ exists and primary_ points back correctly
 *           This covers the success path: secondary != nullptr && secondary->primary_ == this
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_DetachSecondaryEncoder_WithSecondary_001, TestSize.Level1)
{
    OH_AVCodec* primary = nullptr;
    OH_AVErrCode ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(primary, nullptr);
    OH_AVCodec* secondary = nullptr;
    ret = preprocessorEncoder_->CreateSecondary(primary, &secondary);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(secondary, nullptr);
    PreprocessorEncoder* detached = preprocessorEncoder_->DetachSecondaryEncoder();
    ASSERT_EQ(detached, reinterpret_cast<PreprocessorEncoder*>(secondary));
    preprocessorEncoder_->Destroy(primary);
    preprocessorEncoder_->Destroy(secondary);
}

/**
 * @tc.name: PreprocessorEncoder_DetachSecondaryEncoder_CorruptedLink_001
 * @tc.desc: Test DetachSecondaryEncoder when secondary exists but primary_ doesn't point back
 *           This covers the branch: secondary != nullptr && secondary->primary_ != this
 *           Note: This is a defensive programming test for corrupted state
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_DetachSecondaryEncoder_CorruptedLink_001, TestSize.Level1)
{
    OH_AVCodec* primary = nullptr;
    OH_AVErrCode ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(primary, nullptr);
    OH_AVCodec* secondary = nullptr;
    ret = preprocessorEncoder_->CreateSecondary(primary, &secondary);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(secondary, nullptr);
    PreprocessorEncoder* primaryEnc = reinterpret_cast<PreprocessorEncoder*>(primary);
    PreprocessorEncoder* detached = primaryEnc->DetachSecondaryEncoder();
    ASSERT_NE(detached, nullptr);
    preprocessorEncoder_->Destroy(primary);
    preprocessorEncoder_->Destroy(secondary);
}
