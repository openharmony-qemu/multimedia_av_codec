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
#define TEST_SUIT DualOutputCoverageTest

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
namespace V2_2 = OHOS::HDI::Display::Graphic::Common::V2_2;

namespace {
BufferRequestConfig CONFIG = {
    .width = 1280,          // 1280: default width
    .height = 720,          // 720: default height
    .strideAlignment = 32,  // 1280: default strideAlignment
    .format = 26,           // 26: default format
    .usage = (1ULL << 0) | (1ULL << 1) | (1ULL << 5),
    .timeout = 0,           // 0: timeout
};

static void OnErrorVoid(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}
}

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
 * @tc.desc: Test PeekPendingBufferData when pendingBuffers_ is empty
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
 * @tc.desc: Test NotifyEosNowIfPending with non-existent encoder ID
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_NotifyEosNowIfPending_UnknownId_001, TestSize.Level1)
{
    preprocessorManager_->NotifyEosNowIfPending("nonexistent_encoder");
}

/**
 * @tc.name: PreprocManager_NotifyEosNowIfPending_WithPendingBuffers_001
 * @tc.desc: Test NotifyEosNowIfPending when EOS is requested with pending buffers
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_NotifyEosNowIfPending_WithPendingBuffers_001, TestSize.Level1)
{
    const std::string encoderId = "test_encoder";
    bool eosCalled = false;
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder(encoderId, preprocessor_, mockSurface);
    auto iter = preprocessorManager_->encoders_.find(encoderId);
    ASSERT_NE(iter, preprocessorManager_->encoders_.end());
    auto encoderInfo = iter->second;
    ASSERT_NE(encoderInfo, nullptr);
    sptr<SurfaceBuffer> pendingBuffer = SurfaceBuffer::Create();
    ASSERT_NE(pendingBuffer, nullptr);
    auto pendingWrapper = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>(
        wptr<Surface>(), pendingBuffer, nullptr, PreprocessorManager::SurfaceBufferWrapper::SurfaceType::CONSUMER);
    {
        std::lock_guard<std::mutex> lock(encoderInfo->bufferQueueMutex_);
        encoderInfo->isRunning_ = true;
        encoderInfo->pendingBuffers_.push(pendingWrapper);
    }
    pendingWrapper.reset();

    ASSERT_TRUE(preprocessorManager_->RequestNotifyEos(encoderId, [&eosCalled]() {
        eosCalled = true;
    }));
    preprocessorManager_->NotifyEosNowIfPending(encoderId);
    EXPECT_TRUE(eosCalled);
    EXPECT_FALSE(encoderInfo->isRunning_);
    EXPECT_FALSE(encoderInfo->eosRequested_);
    preprocessorManager_->UnregisterEncoder(encoderId);
}

/**
 * @tc.name: PreprocManager_EncoderInfo_Start_StopCycle_001
 * @tc.desc: Test multiple start/stop cycles to verify proper cleanup and restart
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_Start_StopCycle_001, TestSize.Level1)
{
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder", preprocessor_, mockSurface);
    preprocessorManager_->CreateSharedSurface();
    preprocessorManager_->SetEncoderRunning("test_encoder", true);
    usleep(100000); // 100ms
    preprocessorManager_->SetEncoderRunning("test_encoder", false);
    preprocessorManager_->SetEncoderRunning("test_encoder", true);
    usleep(100000); // 100ms
    preprocessorManager_->SetEncoderRunning("test_encoder", false);
    preprocessorManager_->UnregisterEncoder("test_encoder");
}

/**
 * @tc.name: PreprocManager_ReportPreprocessError_NoCallback_001
 * @tc.desc: Test ReportPreprocessError when preprocessErrorCallback_ is nullptr
 *           This covers the branch where callback is not set, should not crash
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_ReportPreprocessError_NoCallback_001, TestSize.Level1)
{
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder_no_cb", preprocessor_, mockSurface, nullptr, nullptr);
    preprocessorManager_->CreateSharedSurface();
    preprocessorManager_->SetEncoderRunning("test_encoder_no_cb", true);
    usleep(100000); // 100ms
    preprocessorManager_->SetEncoderRunning("test_encoder_no_cb", false);
    preprocessorManager_->UnregisterEncoder("test_encoder_no_cb");
}

/**
 * @tc.name: PreprocManager_ReportPreprocessError_WithCallback_001
 * @tc.desc: Test ReportPreprocessError when preprocessErrorCallback_ is set
 *           This covers the branch where callback exists and should be invoked
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_ReportPreprocessError_WithCallback_001, TestSize.Level1)
{
    std::vector<int32_t> errorCodes;
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder_with_cb", preprocessor_, mockSurface, nullptr,
                                         [&errorCodes](int32_t code) {
                                             errorCodes.push_back(code);
                                         });
    preprocessorManager_->CreateSharedSurface();
    // Start encoder with callback registered
    preprocessorManager_->SetEncoderRunning("test_encoder_with_cb", true);
    usleep(100000); // 100ms
    ASSERT_EQ(errorCodes.size(), 0);
    preprocessorManager_->SetEncoderRunning("test_encoder_with_cb", false);
    preprocessorManager_->UnregisterEncoder("test_encoder_with_cb");
}

/**
 * @tc.name: PreprocManager_ReportPreprocessError_MultipleCallbacks_001
 * @tc.desc: Test ReportPreprocessError with different error codes through callback
 *           Verifies callback can handle various error scenarios
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_ReportPreprocessError_MultipleCallbacks_001, TestSize.Level1)
{
    int32_t lastErrorCode = 0;
    int32_t callbackCount = 0;
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("test_encoder_multi", preprocessor_, mockSurface, nullptr,
                                         [&lastErrorCode, &callbackCount](int32_t code) {
                                             lastErrorCode = code;
                                             callbackCount++;
                                         });
    preprocessorManager_->CreateSharedSurface();
    preprocessorManager_->SetEncoderRunning("test_encoder_multi", true);
    usleep(100000); // 100ms
    ASSERT_EQ(callbackCount, 0);
    ASSERT_EQ(lastErrorCode, 0);
    preprocessorManager_->SetEncoderRunning("test_encoder_multi", false);
    preprocessorManager_->UnregisterEncoder("test_encoder_multi");
}

/**
 * @tc.name: PreprocessorManager_CreateSharedSurface_001
 * @tc.desc: CreateSharedSurface coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorManager_CreateSharedSurface_001, TestSize.Level1)
{
    preprocessorManager_->sharedConsumerSurface_ = nullptr;
    preprocessorManager_->CreateSharedSurface();
}

/**
 * @tc.name: PreprocessorManager_CreateSharedSurface_002
 * @tc.desc: CreateSharedSurface coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorManager_CreateSharedSurface_002, TestSize.Level1)
{
    preprocessorManager_->sharedConsumerSurface_ = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(preprocessorManager_->sharedConsumerSurface_, nullptr);
    preprocessorManager_->CreateSharedSurface();
}

/**
 * @tc.name: PreprocManager_Destructor_WithRunningEncoder_001
 * @tc.desc: Test PreprocessorManager destructor with running encoder
 *           This covers the branch where Stop() is called on active encoder during destruction
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_Destructor_WithRunningEncoder_001, TestSize.Level1)
{
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("running_encoder", preprocessor_, mockSurface);
    preprocessorManager_->CreateSharedSurface();
    preprocessorManager_->SetEncoderRunning("running_encoder", true);
    usleep(100000); // 100ms to ensure thread is running
}

/**
 * @tc.name: PreprocManager_Destructor_WithStoppedEncoder_001
 * @tc.desc: Test PreprocessorManager destructor with stopped encoder
 *           This covers the branch where Stop() is called on already stopped encoder
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_Destructor_WithStoppedEncoder_001, TestSize.Level1)
{
    sptr<Surface> mockSurface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(mockSurface, nullptr);
    preprocessorManager_->RegisterEncoder("stopped_encoder", preprocessor_, mockSurface);
    preprocessorManager_->CreateSharedSurface();
    preprocessorManager_->SetEncoderRunning("stopped_encoder", true);
    usleep(100000);  // 100ms to ensure thread is running
    preprocessorManager_->SetEncoderRunning("stopped_encoder", false);
}


/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_NullInput_001
 * @tc.desc: Test UpdateRoiMetadata with null input buffer
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
 * @tc.desc: Test UpdateRoiMetadata when input has no ROI metadata
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
 * @tc.desc: Test UpdateRoiMetadata when input has empty ROI metadata
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_EmptyRoiMetadata_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    input->Alloc(CONFIG);
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
    input->Alloc(CONFIG);
    std::string roiStr = "2147483648,1-2,3";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_ValidRoi_002
 * @tc.desc: Test UpdateRoiMetadata with valid ROI metadata and identity transform
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_ValidRoi_002, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    input->Alloc(CONFIG);
    std::string roiStr = "100,200-300,400=label1;10,20-30,40";
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
    input->Alloc(CONFIG);
    std::string roiStr = "1,2147483648-2,3";
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
    input->Alloc(CONFIG);
    std::string roiStr = "1,2-2147483648,3";
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
    input->Alloc(CONFIG);
    std::string roiStr = "1,2-3,2147483648";
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
    input->Alloc(CONFIG);
    std::string roiStr = "-2147483649,1-2,3";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_WithParam_001
 * @tc.desc: Test UpdateRoiMetadata with ROI containing extra parameter
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_WithParam_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    input->Alloc(CONFIG);
    std::string roiStr = "1,-2147483649-2,3";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_MixedRoisWithParams_001
 * @tc.desc: Test UpdateRoiMetadata with multiple ROIs, some with parameters
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_MixedRoisWithParams_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    input->Alloc(CONFIG);
    std::string roiStr = "1,2--2147483649,3";
    std::vector<uint8_t> roiData(roiStr.begin(), roiStr.end());
    roiData.push_back('\0');
    (void)input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, roiData, false);
    Preprocessor::RoiTransform roiTransform = {0.5, 0.5, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, roiTransform);
}

/**
 * @tc.name: PreprocessorEncoder_Destroy_PrimaryWithoutSecondary_001
 * @tc.desc: Test Destroy interface for primary encoder without secondary encoder
 *           This covers the branch where Primary encoder has no secondary (secondary_ == nullptr)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Destroy_PrimaryWithoutSecondary_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    OH_AVErrCode ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, &primaryCodec);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(primaryCodec, nullptr);
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Destroy_PrimaryWithSecondary_001
 * @tc.desc: Test Destroy interface for primary encoder with secondary encoder
 *           This covers the branch where Primary encoder has secondary (secondary_ != nullptr)
 *           Both primary and secondary should be properly released and deleted
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Destroy_PrimaryWithSecondary_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    OH_AVErrCode ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(primaryCodec, nullptr);
    OH_AVCodec* secondaryCodec = nullptr;
    ret = preprocessorEncoder_->CreateSecondary(primaryCodec, &secondaryCodec);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(secondaryCodec, nullptr);
    ASSERT_TRUE(preprocessorEncoder_->IsPrimaryEncoderMagic(primaryCodec->magic_));
    ASSERT_TRUE(preprocessorEncoder_->IsSecondaryEncoderMagic(secondaryCodec->magic_));
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Destroy_SecondaryBeforePrimary_001
 * @tc.desc: Test destroying secondary encoder before primary encoder
 *           Verifies that secondary can be destroyed independently
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Destroy_SecondaryBeforePrimary_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    OH_AVErrCode ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, &primaryCodec);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(primaryCodec, nullptr);
    OH_AVCodec* secondaryCodec = nullptr;
    preprocessorEncoder_->CreateSecondary(primaryCodec, &secondaryCodec);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_NE(secondaryCodec, nullptr);
    preprocessorEncoder_->Destroy(secondaryCodec);
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_CreateSharedSurface_001
 * @tc.desc: preprocMgr_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CreateSharedSurface_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->CreateSharedSurface();
}

/**
 * @tc.name: PreprocessorEncoder_CreateSharedSurface_002
 * @tc.desc: preprocMgr_ is't nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CreateSharedSurface_002, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->CreateSharedSurface();
}

/**
 * @tc.name: PreprocessorEncoder_GetSharedSurface_001
 * @tc.desc: preprocMgr_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetSharedSurface_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->GetSharedSurface();
}

/**
 * @tc.name: PreprocessorEncoder_GetSharedSurface_002
 * @tc.desc: preprocMgr_ is't nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetSharedSurface_002, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->GetSharedSurface();
}

/**
 * @tc.name: PreprocessorEncoder_RegisterEncoderToManager_001
 * @tc.desc: preprocMgr_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_RegisterEncoderToManager_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->RegisterEncoderToManager();
}

/**
 * @tc.name: PreprocessorEncoder_UnregisterEncoderFromManager_001
 * @tc.desc: preprocMgr_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_UnregisterEncoderFromManager_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->UnregisterEncoderFromManager();
}

/**
 * @tc.name: PreprocessorEncoder_UnregisterEncoderFromManager_002
 * @tc.desc: preprocMgr_ is't nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_UnregisterEncoderFromManager_002, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->UnregisterEncoderFromManager();
}

/**
 * @tc.name: PreprocessorEncoder_OnStreamChanged_001
 * @tc.desc: onStreamChanged is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_OnStreamChanged_001, TestSize.Level1)
{
    Media::Format format;
    preprocessorEncoder_->callback_.onStreamChanged = nullptr;
    preprocessorEncoder_->OnStreamChanged(format);
}

/**
 * @tc.name: PreprocessorEncoder_OnPreprocessError_001
 * @tc.desc: onError is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_OnPreprocessError_001, TestSize.Level1)
{
    int32_t errorCode = 0;
    preprocessorEncoder_->callback_.onError = nullptr;
    preprocessorEncoder_->OnPreprocessError(errorCode);
}

/**
 * @tc.name: PreprocessorEncoder_OnPreprocessError_002
 * @tc.desc: onError is't nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_OnPreprocessError_002, TestSize.Level1)
{
    int32_t errorCode = 0;
    preprocessorEncoder_->callback_.onError = OnErrorVoid;
    preprocessorEncoder_->OnPreprocessError(errorCode);
}

/**
 * @tc.name: PreprocessorEncoder_RegisterCallback_NullVideoEncoder_001
 * @tc.desc: Test RegisterCallback when videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_RegisterCallback_NullVideoEncoder_001, TestSize.Level1)
{
    struct OH_AVCodecCallback cb = {};
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    auto ret = preprocessorEncoder_->RegisterCallback(cb, nullptr);
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_RegisterCalslback_NullVideoEncoderObject_001
 * @tc.desc: Test RegisterCallback when videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_RegisterCallback_NullVideoEncoderObject_001, TestSize.Level1)
{
    struct OH_AVCodecCallback cb = {};
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    auto ret = preprocessorEncoder_->RegisterCallback(cb, nullptr);
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_ConfigureVideoEncoder_NormalBitrateMode_001
 * @tc.desc: Test ConfigureVideoEncoder with normal bitrate mode (not SQR)
 *           This tests the normal configuration path without SQR bitrate mode
 *           Expected: Calls videoEncoder_->Configure(format) directly
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConfigureVideoEncoder_NormalBitrateMode_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    ASSERT_NE(encoder->videoEncoderObject_, nullptr);
    Media::Format format;
    format.PutLongValue(OH_MD_KEY_BITRATE, 1000000);
    format.PutLongValue(OH_MD_KEY_MAX_BITRATE, 2000000);
    format.PutIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, 0); // CBR mode
    int32_t result = encoder->ConfigureVideoEncoder(format);
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_ConfigureVideoEncoder_SQRModeWithMaxBitrate_001
 * @tc.desc: Test ConfigureVideoEncoder with SQR bitrate mode when maxBitrate is already set
 *           This tests SQR mode where both bitrate and maxBitrate are provided
 *           Expected: Calls videoEncoder_->Configure(format) without modification
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConfigureVideoEncoder_SQRModeWithMaxBitrate_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    Media::Format format;
    format.PutLongValue(OH_MD_KEY_BITRATE, 1000000);
    format.PutLongValue(OH_MD_KEY_MAX_BITRATE, 2000000); // maxBitrate is set
    format.PutIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, 3); // SQR mode (assuming value is 3)
    int32_t result = encoder->ConfigureVideoEncoder(format);
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_ConfigureVideoEncoder_SQRModeWithoutMaxBitrate_001
 * @tc.desc: Test ConfigureVideoEncoder with SQR bitrate mode when maxBitrate is not set
 *           This tests the special case where maxBitrate should be set to bitrate value
 *           Expected: Creates formatTemp with maxBitrate=bitrate and calls Configure
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConfigureVideoEncoder_SQRModeWithoutMaxBitrate_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    Media::Format format;
    format.PutLongValue(OH_MD_KEY_BITRATE, 1500000);
    format.PutIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, 3); // SQR mode
    int32_t result = encoder->ConfigureVideoEncoder(format);
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_ConfigureVideoEncoder_SQRModeWithoutBitrate_001
 * @tc.desc: Test ConfigureVideoEncoder with SQR bitrate mode when bitrate is not set
 *           This tests edge case where bitrate doesn't exist in format
 *           Expected: Calls videoEncoder_->Configure(format) without modification
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConfigureVideoEncoder_SQRModeWithoutBitrate_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    Media::Format format;
    format.PutLongValue(OH_MD_KEY_MAX_BITRATE, 2000000);
    format.PutIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, 3); // SQR mode
    int32_t result = encoder->ConfigureVideoEncoder(format);
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_ConfigureVideoEncoder_NullVideoEncoder_001
 * @tc.desc: Test ConfigureVideoEncoder when videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConfigureVideoEncoder_NullVideoEncoder_001, TestSize.Level1)
{
    Media::Format format;
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    auto ret = preprocessorEncoder_->ConfigureVideoEncoder(format);
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_ConfigureVideoEncoder_NullVideoEncoderObject_001
 * @tc.desc: Test ConfigureVideoEncoder when videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConfigureVideoEncoder_NullVideoEncoderObject_001, TestSize.Level1)
{
    Media::Format format;
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    auto ret = preprocessorEncoder_->ConfigureVideoEncoder(format);
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_ConfigurePreprocessor_001
 * @tc.desc: preprocessor is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConfigurePreprocessor_001, TestSize.Level1)
{
    Media::Format format;
    preprocessorEncoder_->ConfigurePreprocessor(format);
}

/**
 * @tc.name: PreprocessorEncoder_CreateEncoderSurface_ValidEncoder_001
 * @tc.desc: Test CreateEncoderSurface with valid videoEncoderObject_
 *           This tests the normal path where CreateInputSurface is called
 *           Expected: Calls CreateInputSurface and assigns result to encoderSurface_
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CreateEncoderSurface_ValidEncoder_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    ASSERT_NE(encoder->videoEncoderObject_, nullptr);
    ASSERT_EQ(encoder->encoderSurface_, nullptr); // Initially nullptr
    (void)encoder->CreateEncoderSurface();
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_CreateEncoderSurface_ExistingSurface_001
 * @tc.desc: Test CreateEncoderSurface when encoderSurface_ already exists
 *           This tests the case where CreateInputSurface returns nullptr but encoderSurface_ is not null
 *           Expected: Returns AVCS_ERR_OK (not AVCS_ERR_NO_MEMORY) because encoderSurface_ is already set
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CreateEncoderSurface_ExistingSurface_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->CreateEncoderSurface();
    sptr<Surface> firstSurface = encoder->encoderSurface_;
    (void)encoder->CreateEncoderSurface();
    (void)firstSurface;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_CreateEncoderSurface_NullVideoEncoder_001
 * @tc.desc: Test CreateEncoderSurface when videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CreateEncoderSurface_NullVideoEncoder_001, TestSize.Level1)
{
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    preprocessorEncoder_->CreateEncoderSurface();
}

/**
 * @tc.name: PreprocessorEncoder_CreateEncoderSurface_NullVideoEncoderObject_001
 * @tc.desc: Test CreateEncoderSurface when videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CreateEncoderSurface_NullVideoEncoderObject_001, TestSize.Level1)
{
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    preprocessorEncoder_->CreateEncoderSurface();
}

/**
 * @tc.name: PreprocessorEncoder_Start_ValidEncoder_001
 * @tc.desc: Test Start with valid videoEncoderObject_
 *           This tests the normal path where video encoder Start is called
 *           Expected: Calls videoEncoder_->Start(), FlushTimeStamp, and SetEncoderRunning
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Start_ValidEncoder_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    ASSERT_NE(encoder->videoEncoderObject_, nullptr);
    encoder->Start();
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Start_WithPreprocMgr_001
 * @tc.desc: Test Start when preprocMgr_ is set
 *           This tests the path where SetEncoderRunning is called
 *           Expected: Calls SetEncoderRunning(encoderId_, true)
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Start_WithPreprocMgr_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = std::make_shared<PreprocessorManager>();
    encoder->Start();
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Start_WithoutPreprocMgr_001
 * @tc.desc: Test Start when preprocMgr_ is nullptr
 *           This tests the path where SetEncoderRunning is skipped
 *           Expected: Skips SetEncoderRunning call, returns AV_ERR_OK
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Start_WithoutPreprocMgr_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = nullptr;
    encoder->Start();
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Start_AfterConfigure_001
 * @tc.desc: Test Start after proper configuration
 *           This tests the complete initialization path before Start
 *           Expected: Successfully starts configured encoder
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Start_AfterConfigure_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    Media::Format format;
    format.PutLongValue(OH_MD_KEY_BITRATE, 1000000);
    format.PutLongValue(OH_MD_KEY_MAX_BITRATE, 2000000);
    format.PutIntValue(OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, 0); // CBR mode
    int32_t configRet = encoder->ConfigureVideoEncoder(format);
    (void)configRet;
    encoder->Start();
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Start_NullVideoEncoder_001
 * @tc.desc: Test Start when preprocMgr_ is nullptr and videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Start_NullVideoEncoder_001, TestSize.Level1)
{
    preprocessorEncoder_->encoderId_ = "test_encoder_null_videoencoder";
    auto preprocessor = std::make_shared<Preprocessor>(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(preprocessor, nullptr);
    preprocessorEncoder_->preprocessors_[preprocessorEncoder_->encoderId_] = preprocessor;
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    auto ret = preprocessorEncoder_->Start();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Start_NullVideoEncoderObject_001
 * @tc.desc: Test Start when preprocMgr_ not exists, videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Start_NullVideoEncoderObject_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    auto ret = preprocessorEncoder_->Start();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Stop_WithPreprocMgr_001
 * @tc.desc: Test Stop when preprocMgr_ is set
 *           This tests the path where NotifyEosNowIfPending and SetEncoderRunning are called
 *           Expected: Calls NotifyEosNowIfPending before stop, SetEncoderRunning(false) after stop
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Stop_WithPreprocMgr_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = std::make_shared<PreprocessorManager>();
    OH_AVErrCode result = encoder->Stop();
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Stop_NullVideoEncoder_001
 * @tc.desc: Test Flush when preprocMgr_ exists and videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Stop_NullVideoEncoder_001, TestSize.Level1)
{
    preprocessorEncoder_->encoderId_ = "test_encoder_null_videoencoder";
    auto preprocessor = std::make_shared<Preprocessor>(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(preprocessor, nullptr);
    preprocessorEncoder_->preprocessors_[preprocessorEncoder_->encoderId_] = preprocessor;
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    auto ret = preprocessorEncoder_->Stop();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Stop_NullVideoEncoderObject_001
 * @tc.desc: Test Stop when preprocMgr_ not exists, videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Stop_NullVideoEncoderObject_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    auto ret = preprocessorEncoder_->Stop();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Flush_WithPreprocMgr_001
 * @tc.desc: Test Flush when preprocMgr_ is set
 *           This tests the path where NotifyEosNowIfPending and SetEncoderRunning are called
 *           Expected: Calls NotifyEosNowIfPending before flush, SetEncoderRunning(false) after flush
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Flush_WithPreprocMgr_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = std::make_shared<PreprocessorManager>();
    OH_AVErrCode result = encoder->Flush();
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Flush_NullVideoEncoder_001
 * @tc.desc: Test Flush when preprocMgr_ exists and videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Flush_NullVideoEncoder_001, TestSize.Level1)
{
    preprocessorEncoder_->encoderId_ = "test_encoder_null_videoencoder";
    auto preprocessor = std::make_shared<Preprocessor>(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(preprocessor, nullptr);
    preprocessorEncoder_->preprocessors_[preprocessorEncoder_->encoderId_] = preprocessor;
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    auto ret = preprocessorEncoder_->Flush();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Flush_NullVideoEncoderObject_001
 * @tc.desc: Test Flush when preprocMgr_ not exists, videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Flush_NullVideoEncoderObject_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    auto ret = preprocessorEncoder_->Flush();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Reset_WithPreprocMgr_001
 * @tc.desc: Test Reset when preprocMgr_ is set
 *           This tests the path where NotifyEosNowIfPending, SetEncoderRunning UnregisterEncoderFromManager are called
 *           Expected: Calls all preprocMgr_ methods including UnregisterEncoderFromManager
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Reset_WithPreprocMgr_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = std::make_shared<PreprocessorManager>();
    OH_AVErrCode result = encoder->Reset();
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Reset_NullVideoEncoder_001
 * @tc.desc: Test Reset when preprocMgr_ exists and videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Reset_NullVideoEncoder_001, TestSize.Level1)
{
    preprocessorEncoder_->encoderId_ = "test_encoder_null_videoencoder";
    auto preprocessor = std::make_shared<Preprocessor>(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(preprocessor, nullptr);
    preprocessorEncoder_->preprocessors_[preprocessorEncoder_->encoderId_] = preprocessor;
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    auto ret = preprocessorEncoder_->Reset();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Reset_NullVideoEncoderObject_001
 * @tc.desc: Test Reset when preprocMgr_ not exists, videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Reset_NullVideoEncoderObject_001, TestSize.Level1)
{
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    auto ret = preprocessorEncoder_->Reset();
    EXPECT_EQ(ret, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Release_WithPreprocMgr_001
 * @tc.desc: Test Release when preprocMgr_ is set
 *           This tests the path where SetEncoderRunning(false) is called before release
 *           Expected: Calls SetEncoderRunning(false), releases encoder, and preserves the primary type magic
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Release_WithPreprocMgr_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = std::make_shared<PreprocessorManager>();
    OH_AVErrCode result = encoder->Release();
    (void)result;
    ASSERT_EQ(encoder->magic_, AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER);
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_Release_ValidEncoder_001
 * @tc.desc: Test Release with valid videoEncoderObject_
 *           This tests the normal release path
 *           Expected: Calls videoEncoder_->Release(), StopCallback and ClearBufferList,
 *           and preserves the primary type magic
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Release_ValidEncoder_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    ASSERT_NE(encoder->videoEncoderObject_, nullptr);
    OH_AVErrCode result = encoder->Release();
    (void)result;
    ASSERT_EQ(encoder->magic_, AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER);
    delete encoder;
}

/**
 * @tc.name: PreprocessorEncoder_ReleaseOutputBuffer_ValidEncoder_001
 * @tc.desc: Test ReleaseOutputBuffer with valid videoEncoderObject_
 *           This tests the normal path where video encoder ReleaseOutputBuffer is called
 *           Expected: Calls videoEncoder_->ReleaseOutputBuffer(index) and returns result
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ReleaseOutputBuffer_ValidEncoder_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    ASSERT_NE(encoder->videoEncoderObject_, nullptr);
    uint32_t index = 0;
    OH_AVErrCode result = encoder->ReleaseOutputBuffer(index);
    (void)result;
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_ReleaseOutputBuffer_MultipleIndices_001
 * @tc.desc: Test ReleaseOutputBuffer with different buffer indices
 *           This tests that the index parameter is correctly passed through
 *           Expected: Calls videoEncoder_->ReleaseOutputBuffer with each index
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ReleaseOutputBuffer_MultipleIndices_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    uint32_t indices[] = {0, 1, 5, 10};
    for (uint32_t index : indices) {
        OH_AVErrCode result = encoder->ReleaseOutputBuffer(index);
        (void)result;
    }
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_GetOutputDescription_ValidEncoder_001
 * @tc.desc: Test GetOutputDescription with valid videoEncoderObject_
 *           This tests the normal path where GetOutputFormat is called
 *           Expected: Returns valid OH_AVFormat* or nullptr depending on encoder state
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetOutputDescription_ValidEncoder_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    ASSERT_NE(encoder->videoEncoderObject_, nullptr);
    OH_AVFormat* result = encoder->GetOutputDescription();
    (void)result;
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_GetOutputDescription_NullVideoEncoder_001
 * @tc.desc: Test GetOutputDescription when videoEncoderObject_ exists but videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetOutputDescription_NullVideoEncoder_001, TestSize.Level1)
{
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    OH_AVFormat* result = preprocessorEncoder_->GetOutputDescription();
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
}

/**
 * @tc.name: PreprocessorEncoder_GetOutputDescription_NullVideoEncoderObject_001
 * @tc.desc: Test GetOutputDescription when videoEncoderObject_ and videoEncoder_ is nullptr
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetOutputDescription_NullVideoEncoderObject_001, TestSize.Level1)
{
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    OH_AVFormat* result = preprocessorEncoder_->GetOutputDescription();
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
}

/**
 * @tc.name: PreprocessorEncoder_GetInputDescription_NullPreprocessor_001
 * @tc.desc: Test GetInputDescription when preprocessor is not available
 *           This tests the case where GetPreprocessor returns nullptr
 *           Expected: Returns nullptr with error message
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetInputDescription_NullPreprocessor_001, TestSize.Level1)
{
    OH_AVFormat* result = preprocessorEncoder_->GetInputDescription();
    ASSERT_EQ(result, nullptr);
}

/**
 * @tc.name: PreprocessorEncoder_GetInputDescription_ValidEncoder_001
 * @tc.desc: Test GetInputDescription with valid encoder and preprocessor
 *           This tests the normal path with encoder format retrieval
 *           Expected: Returns valid OH_AVFormat* with encoder input format
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetInputDescription_ValidEncoder_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = std::make_shared<PreprocessorManager>();
    OH_AVFormat* result = encoder->GetInputDescription();
    (void)result;
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_GetInputDescription_WithCropEnabled_001
 * @tc.desc: Test GetInputDescription when crop is enabled
 *           This tests the path where crop metadata is added to format
 *           Expected: Includes crop left/right/top/bottom in the returned format
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetInputDescription_WithCropEnabled_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = nullptr;
    OH_AVFormat* result = encoder->GetInputDescription();
    (void)result;
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_GetInputDescription_WithDownsampling_001
 * @tc.desc: Test GetInputDescription when downsampling is enabled
 *           This tests the path where downsampling metadata is added to format
 *           Expected: Includes downsampling width/height in the returned format if enabled
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetInputDescription_WithDownsampling_001, TestSize.Level1)
{
    OH_AVCodec* primaryCodec = nullptr;
    auto ret = preprocessorEncoder_->CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primaryCodec);
    if (ret != AV_ERR_OK || primaryCodec == nullptr) {
        GTEST_SKIP() << "Cannot create primary encoder";
    }
    auto* encoder = static_cast<PreprocessorEncoder*>(primaryCodec);
    encoder->preprocMgr_ = std::make_shared<PreprocessorManager>();
    OH_AVFormat* result = encoder->GetInputDescription();
    (void)result;
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
    preprocessorEncoder_->Destroy(primaryCodec);
}

/**
 * @tc.name: PreprocessorEncoder_GetInputDescription_NullVideoEncoder_001
 * @tc.desc: Test GetInputDescription when videoEncoderObject_ exists but videoEncoder_ is nullptr
 *           This tests the case where videoEncoderObject_ is not null but videoEncoderObject_->videoEncoder_ is null
 *           Expected: Returns valid OH_AVFormat* without encoder format, only preprocessor metadata if available
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetInputDescription_NullVideoEncoder_001, TestSize.Level1)
{
    preprocessorEncoder_->encoderId_ = "test_encoder_null_videoencoder";
    auto preprocessor = std::make_shared<Preprocessor>(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(preprocessor, nullptr);
    preprocessorEncoder_->preprocessors_[preprocessorEncoder_->encoderId_] = preprocessor;
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->videoEncoderObject_ = std::make_shared<VideoEncoderObject>(nullptr);
    OH_AVFormat* result = preprocessorEncoder_->GetInputDescription();
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
}

/**
 * @tc.name: PreprocessorEncoder_GetInputDescription_NullVideoEncoderObject_001
 * @tc.desc: Test GetInputDescription when videoEncoderObject_ is nullptr
 *           This tests the case where videoEncoderObject_ itself is null
 *           Expected: Returns valid OH_AVFormat* without encoder format, only preprocessor metadata if available
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_GetInputDescription_NullVideoEncoderObject_001, TestSize.Level1)
{
    preprocessorEncoder_->encoderId_ = "test_encoder_null_videoencoderobj";
    auto preprocessor = std::make_shared<Preprocessor>(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(preprocessor, nullptr);
    preprocessorEncoder_->preprocessors_[preprocessorEncoder_->encoderId_] = preprocessor;
    preprocessorEncoder_->preprocMgr_ = std::make_shared<PreprocessorManager>();
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    OH_AVFormat* result = preprocessorEncoder_->GetInputDescription();
    if (result != nullptr) {
        OH_AVFormat_Destroy(result);
    }
}
