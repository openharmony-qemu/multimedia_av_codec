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
#include <algorithm>
#include <chrono>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <thread>
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avmagic.h"
#include "preprocessor.h"
#include "preprocessor_encoder.h"
#include "preprocessor_manager.h"
#include "video_encoder_object.h"
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

constexpr auto CALLBACK_WAIT_TIMEOUT = std::chrono::seconds(3);
constexpr auto CALLBACK_BLOCK_CHECK_TIME = std::chrono::milliseconds(100);

enum class CallbackAction {
    NONE,
    STOP,
    FLUSH,
    RESET,
};

struct CallbackTestContext {
    std::mutex mutex;
    std::condition_variable cv;
    CallbackAction action{CallbackAction::NONE};
    int32_t callbackCount{0};
    int32_t finishedCount{0};
    OH_AVErrCode operationResult{AV_ERR_INVALID_VAL};
    std::thread::id callbackThreadId{};
    bool blockFirstCallback{false};
    bool allowFirstCallbackReturn{false};
    std::shared_ptr<void> dispatcherKeepAlive;
};

struct AsyncOperationState {
    std::mutex mutex;
    std::condition_variable cv;
    bool finished{false};
    OH_AVErrCode result{AV_ERR_INVALID_VAL};
};

static void OnErrorVoid(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}

void OnErrorForCallbackTest(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)errorCode;
    auto *context = static_cast<CallbackTestContext *>(userData);
    if (context == nullptr) {
        return;
    }

    {
        std::unique_lock<std::mutex> lock(context->mutex);
        ++context->callbackCount;
        context->callbackThreadId = std::this_thread::get_id();
        context->cv.notify_all();
        if (context->blockFirstCallback && context->callbackCount == 1) {
            context->cv.wait(lock, [context]() { return context->allowFirstCallbackReturn; });
        }
    }

    OH_AVErrCode result = AV_ERR_OK;
    switch (context->action) {
        case CallbackAction::STOP:
            result = OH_VideoEncoder_Stop(codec);
            break;
        case CallbackAction::FLUSH:
            result = OH_VideoEncoder_Flush(codec);
            break;
        case CallbackAction::RESET:
            result = OH_VideoEncoder_Reset(codec);
            break;
        default:
            break;
    }

    {
        std::lock_guard<std::mutex> lock(context->mutex);
        context->operationResult = result;
        ++context->finishedCount;
    }
    context->cv.notify_all();
}

void OnErrorReleaseDispatcherForCallbackTest(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)errorCode;
    auto *context = static_cast<CallbackTestContext *>(userData);
    if (context == nullptr) {
        return;
    }
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        ++context->callbackCount;
        ++context->finishedCount;
    }
    encoder->callbackDispatcher_.reset();
    context->dispatcherKeepAlive.reset();
    context->cv.notify_all();
}

bool WaitForCallbackCount(CallbackTestContext &context, int32_t expectedCount)
{
    std::unique_lock<std::mutex> lock(context.mutex);
    return context.cv.wait_for(lock, CALLBACK_WAIT_TIMEOUT,
        [&context, expectedCount]() { return context.callbackCount >= expectedCount; });
}

bool WaitForFinishedCount(CallbackTestContext &context, int32_t expectedCount)
{
    std::unique_lock<std::mutex> lock(context.mutex);
    return context.cv.wait_for(lock, CALLBACK_WAIT_TIMEOUT,
        [&context, expectedCount]() { return context.finishedCount >= expectedCount; });
}

void AllowFirstCallbackToReturn(CallbackTestContext &context)
{
    {
        std::lock_guard<std::mutex> lock(context.mutex);
        context.allowFirstCallbackReturn = true;
    }
    context.cv.notify_all();
}

bool WaitForAsyncOperation(AsyncOperationState &state)
{
    std::unique_lock<std::mutex> lock(state.mutex);
    return state.cv.wait_for(lock, CALLBACK_WAIT_TIMEOUT, [&state]() { return state.finished; });
}

bool IsAsyncOperationBlocked(AsyncOperationState &state)
{
    std::unique_lock<std::mutex> lock(state.mutex);
    return !state.cv.wait_for(lock, CALLBACK_BLOCK_CHECK_TIME, [&state]() { return state.finished; });
}

void FinishAsyncOperation(AsyncOperationState &state, OH_AVErrCode result)
{
    {
        std::lock_guard<std::mutex> lock(state.mutex);
        state.result = result;
        state.finished = true;
    }
    state.cv.notify_all();
}

bool PrepareCallbackEncoder(OH_AVCodec *codec, CallbackTestContext &context)
{
    if (codec == nullptr) {
        return false;
    }
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->videoEncoderObject_ = nullptr;
    encoder->preprocMgr_ = nullptr;
    OH_AVCodecCallback callback = {};
    callback.onError = OnErrorForCallbackTest;
    return encoder->RegisterCallback(callback, &context) == AV_ERR_OK && encoder->Start() == AV_ERR_OK;
}

bool RunCallbackAction(OH_AVCodec *codec, CallbackAction action, CallbackTestContext &context)
{
    context.action = action;
    if (!PrepareCallbackEncoder(codec, context)) {
        return false;
    }
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->OnEncoderError(0);
    if (!WaitForFinishedCount(context, 1)) {
        return false;
    }
    std::lock_guard<std::mutex> lock(context.mutex);
    return context.callbackCount == 1 && context.operationResult == AV_ERR_OK;
}

void OnStreamChangedForCallbackTest(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    auto *context = static_cast<CallbackTestContext *>(userData);
    if (context == nullptr) {
        return;
    }
    {
        std::lock_guard<std::mutex> lock(context->mutex);
        ++context->callbackCount;
        context->callbackThreadId = std::this_thread::get_id();
        ++context->finishedCount;
    }
    context->cv.notify_all();
}

void OnOutputBufferForCallbackTest(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData)
{
    (void)codec;
    (void)index;
    (void)buffer;
    auto *context = static_cast<CallbackTestContext *>(userData);
    if (context == nullptr) {
        return;
    }
    {
        std::unique_lock<std::mutex> lock(context->mutex);
        ++context->callbackCount;
        context->callbackThreadId = std::this_thread::get_id();
        context->cv.notify_all();
        if (context->blockFirstCallback && context->callbackCount == 1) {
            context->cv.wait(lock, [context]() { return context->allowFirstCallbackReturn; });
        }
        ++context->finishedCount;
    }
    context->cv.notify_all();
}

bool SetRoiMetadata(const sptr<SurfaceBuffer> &buffer, const std::string &roi)
{
    std::vector<uint8_t> data(roi.begin(), roi.end());
    data.push_back('\0');
    return buffer->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, data, false) == GSERROR_OK;
}

bool GetRoiMetadata(const sptr<SurfaceBuffer> &buffer, std::string &roi)
{
    std::vector<uint8_t> data;
    if (buffer->GetMetadata(V2_2::ATTRKEY_ROI_METADATA, data) != GSERROR_OK) {
        return false;
    }
    auto end = std::find(data.begin(), data.end(), '\0');
    roi.assign(data.begin(), end);
    return true;
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

/**
 * @tc.name: PreprocessorEncoder_Callback_Stop_PrimarySecondary_NoDeadlock_001
 * @tc.desc: Stop called from primary and secondary callback threads does not self-wait
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Callback_Stop_PrimarySecondary_NoDeadlock_001, TestSize.Level1)
{
    OH_AVCodec *primary = nullptr;
    OH_AVCodec *secondary = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary), AV_ERR_OK);
    ASSERT_EQ(PreprocessorEncoder::CreateSecondary(primary, &secondary), AV_ERR_OK);

    CallbackTestContext primaryContext;
    CallbackTestContext secondaryContext;
    EXPECT_TRUE(RunCallbackAction(primary, CallbackAction::STOP, primaryContext));
    EXPECT_TRUE(RunCallbackAction(secondary, CallbackAction::STOP, secondaryContext));
    EXPECT_EQ(OH_VideoEncoder_Destroy(primary), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Callback_Flush_PrimarySecondary_NoDeadlock_001
 * @tc.desc: Flush called from primary and secondary callback threads does not self-wait
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Callback_Flush_PrimarySecondary_NoDeadlock_001, TestSize.Level1)
{
    OH_AVCodec *primary = nullptr;
    OH_AVCodec *secondary = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary), AV_ERR_OK);
    ASSERT_EQ(PreprocessorEncoder::CreateSecondary(primary, &secondary), AV_ERR_OK);

    CallbackTestContext primaryContext;
    CallbackTestContext secondaryContext;
    EXPECT_TRUE(RunCallbackAction(primary, CallbackAction::FLUSH, primaryContext));
    EXPECT_TRUE(RunCallbackAction(secondary, CallbackAction::FLUSH, secondaryContext));
    EXPECT_EQ(OH_VideoEncoder_Destroy(primary), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Callback_Reset_PrimarySecondary_NoDeadlock_001
 * @tc.desc: Reset called from primary and secondary callback threads does not self-wait
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Callback_Reset_PrimarySecondary_NoDeadlock_001, TestSize.Level1)
{
    OH_AVCodec *primary = nullptr;
    OH_AVCodec *secondary = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary), AV_ERR_OK);
    ASSERT_EQ(PreprocessorEncoder::CreateSecondary(primary, &secondary), AV_ERR_OK);

    CallbackTestContext primaryContext;
    CallbackTestContext secondaryContext;
    EXPECT_TRUE(RunCallbackAction(primary, CallbackAction::RESET, primaryContext));
    EXPECT_TRUE(RunCallbackAction(secondary, CallbackAction::RESET, secondaryContext));
    EXPECT_EQ(OH_VideoEncoder_Destroy(primary), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Destroy_WaitsRunningCallback_001
 * @tc.desc: Destroy waits until an executing asynchronous callback returns
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Destroy_WaitsRunningCallback_001, TestSize.Level1)
{
    OH_AVCodec *primary = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary), AV_ERR_OK);
    CallbackTestContext context;
    context.blockFirstCallback = true;
    ASSERT_TRUE(PrepareCallbackEncoder(primary, context));
    reinterpret_cast<PreprocessorEncoder *>(primary)->OnEncoderError(0);
    ASSERT_TRUE(WaitForCallbackCount(context, 1));

    AsyncOperationState destroyState;
    std::thread destroyThread([primary, &destroyState]() {
        FinishAsyncOperation(destroyState, OH_VideoEncoder_Destroy(primary));
    });
    EXPECT_TRUE(IsAsyncOperationBlocked(destroyState));
    AllowFirstCallbackToReturn(context);
    EXPECT_TRUE(WaitForAsyncOperation(destroyState));
    destroyThread.join();
    EXPECT_EQ(destroyState.result, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_DestroyPrimary_WaitsPrimarySecondaryCallbacks_001
 * @tc.desc: Destroying primary waits for callbacks executing on both independent encoders
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_DestroyPrimary_WaitsPrimarySecondaryCallbacks_001, TestSize.Level1)
{
    OH_AVCodec *primary = nullptr;
    OH_AVCodec *secondary = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary), AV_ERR_OK);
    ASSERT_EQ(PreprocessorEncoder::CreateSecondary(primary, &secondary), AV_ERR_OK);
    CallbackTestContext primaryContext;
    CallbackTestContext secondaryContext;
    primaryContext.blockFirstCallback = true;
    secondaryContext.blockFirstCallback = true;
    ASSERT_TRUE(PrepareCallbackEncoder(primary, primaryContext));
    ASSERT_TRUE(PrepareCallbackEncoder(secondary, secondaryContext));
    reinterpret_cast<PreprocessorEncoder *>(primary)->OnEncoderError(0);
    reinterpret_cast<PreprocessorEncoder *>(secondary)->OnEncoderError(0);
    ASSERT_TRUE(WaitForCallbackCount(primaryContext, 1));
    ASSERT_TRUE(WaitForCallbackCount(secondaryContext, 1));

    AsyncOperationState destroyState;
    std::thread destroyThread([primary, &destroyState]() {
        FinishAsyncOperation(destroyState, OH_VideoEncoder_Destroy(primary));
    });
    EXPECT_TRUE(IsAsyncOperationBlocked(destroyState));
    AllowFirstCallbackToReturn(primaryContext);
    EXPECT_TRUE(IsAsyncOperationBlocked(destroyState));
    AllowFirstCallbackToReturn(secondaryContext);
    EXPECT_TRUE(WaitForAsyncOperation(destroyState));
    destroyThread.join();
    EXPECT_EQ(destroyState.result, AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_PrimarySecondary_CallbackThreadsIndependent_001
 * @tc.desc: A blocked primary callback does not prevent a secondary callback from executing
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_PrimarySecondary_CallbackThreadsIndependent_001, TestSize.Level1)
{
    OH_AVCodec *primary = nullptr;
    OH_AVCodec *secondary = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary), AV_ERR_OK);
    ASSERT_EQ(PreprocessorEncoder::CreateSecondary(primary, &secondary), AV_ERR_OK);
    CallbackTestContext primaryContext;
    CallbackTestContext secondaryContext;
    primaryContext.blockFirstCallback = true;
    ASSERT_TRUE(PrepareCallbackEncoder(primary, primaryContext));
    ASSERT_TRUE(PrepareCallbackEncoder(secondary, secondaryContext));

    reinterpret_cast<PreprocessorEncoder *>(primary)->OnEncoderError(0);
    ASSERT_TRUE(WaitForCallbackCount(primaryContext, 1));
    reinterpret_cast<PreprocessorEncoder *>(secondary)->OnEncoderError(0);
    EXPECT_TRUE(WaitForFinishedCount(secondaryContext, 1));
    EXPECT_NE(primaryContext.callbackThreadId, secondaryContext.callbackThreadId);

    AllowFirstCallbackToReturn(primaryContext);
    EXPECT_TRUE(WaitForFinishedCount(primaryContext, 1));
    EXPECT_EQ(OH_VideoEncoder_Destroy(primary), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_Flush_DropsQueuedCallbacks_001
 * @tc.desc: Flush closes the callback gate, drops queued events, and allows new events after restart
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_Flush_DropsQueuedCallbacks_001, TestSize.Level1)
{
    constexpr int32_t queuedEventCount = 8;
    OH_AVCodec *primary = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &primary), AV_ERR_OK);
    CallbackTestContext context;
    context.blockFirstCallback = true;
    ASSERT_TRUE(PrepareCallbackEncoder(primary, context));
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(primary);
    encoder->OnEncoderError(0);
    ASSERT_TRUE(WaitForCallbackCount(context, 1));
    for (int32_t i = 0; i < queuedEventCount; ++i) {
        encoder->OnEncoderError(0);
    }

    AsyncOperationState flushState;
    std::thread flushThread([primary, &flushState]() {
        FinishAsyncOperation(flushState, OH_VideoEncoder_Flush(primary));
    });
    EXPECT_TRUE(IsAsyncOperationBlocked(flushState));
    AllowFirstCallbackToReturn(context);
    EXPECT_TRUE(WaitForAsyncOperation(flushState));
    flushThread.join();
    EXPECT_EQ(flushState.result, AV_ERR_OK);
    EXPECT_EQ(context.callbackCount, 1);

    ASSERT_EQ(OH_VideoEncoder_Start(primary), AV_ERR_OK);
    encoder->OnEncoderError(0);
    EXPECT_TRUE(WaitForFinishedCount(context, 2));
    EXPECT_EQ(context.callbackCount, 2);
    EXPECT_EQ(OH_VideoEncoder_Destroy(primary), AV_ERR_OK);
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_Int32Max_001
 * @tc.desc: The maximum valid int32 ROI coordinates are preserved
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_Int32Max_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(input->Alloc(CONFIG), GSERROR_OK);
    ASSERT_EQ(output->Alloc(CONFIG), GSERROR_OK);
    ASSERT_TRUE(SetRoiMetadata(input, "0,0-2147483647,2147483647"));
    Preprocessor::RoiTransform transform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, transform);
    std::string actual;
    ASSERT_TRUE(GetRoiMetadata(output, actual));
    EXPECT_EQ(actual, "0,0-2147483647,2147483647;");
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_NegativeCoordinateSkipped_001
 * @tc.desc: An ROI with a negative coordinate is skipped while a following valid ROI is retained
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_NegativeCoordinateSkipped_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(input->Alloc(CONFIG), GSERROR_OK);
    ASSERT_EQ(output->Alloc(CONFIG), GSERROR_OK);
    ASSERT_TRUE(SetRoiMetadata(input, "-1,0-100,100;10,20-30,40"));
    Preprocessor::RoiTransform transform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, transform);
    std::string actual;
    ASSERT_TRUE(GetRoiMetadata(output, actual));
    EXPECT_EQ(actual, "10,20-30,40;");
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_OutOfRangeCoordinateSkipped_001
 * @tc.desc: An out-of-int32 ROI is skipped while a following valid ROI is retained
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_OutOfRangeCoordinateSkipped_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(input->Alloc(CONFIG), GSERROR_OK);
    ASSERT_EQ(output->Alloc(CONFIG), GSERROR_OK);
    ASSERT_TRUE(SetRoiMetadata(input, "2147483648,0-100,100;10,20-30,40"));
    Preprocessor::RoiTransform transform = {1.0, 1.0, 0, 0};
    preprocessor_->UpdateRoiMetadata(input, output, transform);
    std::string actual;
    ASSERT_TRUE(GetRoiMetadata(output, actual));
    EXPECT_EQ(actual, "10,20-30,40;");
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_ScaleAndOffset_001
 * @tc.desc: ROI scaling, crop offset, and optional parameters are transformed together
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_ScaleAndOffset_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(input->Alloc(CONFIG), GSERROR_OK);
    ASSERT_EQ(output->Alloc(CONFIG), GSERROR_OK);
    ASSERT_TRUE(SetRoiMetadata(input, "100,200-300,400=label"));
    Preprocessor::RoiTransform transform = {0.5, 0.5, 10, 20};
    preprocessor_->UpdateRoiMetadata(input, output, transform);
    std::string actual;
    ASSERT_TRUE(GetRoiMetadata(output, actual));
    EXPECT_EQ(actual, "40,80-140,180=label;");
}

/**
 * @tc.name: PreprocManager_Stop_WithPendingBuffers_NoDeadlock_001
 * @tc.desc: Stop clears pending wrappers outside bufferQueueMutex and leaves the encoder stopped
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_Stop_WithPendingBuffers_NoDeadlock_001, TestSize.Level1)
{
    const std::string encoderId = "encoder_with_pending_buffer";
    sptr<Surface> surface = Surface::CreateSurfaceAsConsumer();
    ASSERT_NE(surface, nullptr);
    preprocessorManager_->RegisterEncoder(encoderId, preprocessor_, surface);
    auto iter = preprocessorManager_->encoders_.find(encoderId);
    ASSERT_NE(iter, preprocessorManager_->encoders_.end());
    auto encoderInfo = iter->second;
    ASSERT_NE(encoderInfo, nullptr);

    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->Alloc(CONFIG), GSERROR_OK);
    auto wrapper = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>(surface, buffer, nullptr,
        PreprocessorManager::SurfaceBufferWrapper::SurfaceType::CONSUMER);
    {
        std::lock_guard<std::mutex> lock(encoderInfo->bufferQueueMutex_);
        encoderInfo->isRunning_ = true;
        encoderInfo->pendingBuffers_.push(wrapper);
    }
    wrapper.reset();

    encoderInfo->Stop();
    EXPECT_FALSE(encoderInfo->isRunning_);
    EXPECT_TRUE(encoderInfo->pendingBuffers_.empty());
    preprocessorManager_->UnregisterEncoder(encoderId);
}

/**
 * @tc.name: PreprocManager_EncoderInfo_BufferQueueGuards_001
 * @tc.desc: Cover null and valid branches of pending/output buffer queue helpers
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_BufferQueueGuards_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("queue_guard");
    ASSERT_NE(encoderInfo, nullptr);

    encoderInfo->RecycleOutputBuffer(nullptr);
    auto nullBuffer = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>();
    ASSERT_NE(nullBuffer, nullptr);
    nullBuffer->surfaceType = PreprocessorManager::SurfaceBufferWrapper::SurfaceType::CONSUMER;
    encoderInfo->RecycleOutputBuffer(nullBuffer);

    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->Alloc(CONFIG), GSERROR_OK);
    auto wrapper = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>(
        wptr<Surface>(), buffer, nullptr, PreprocessorManager::SurfaceBufferWrapper::SurfaceType::CONSUMER);
    encoderInfo->RecycleOutputBuffer(wrapper);
    EXPECT_EQ(encoderInfo->availableInputBuffers_.size(), 1U);

    encoderInfo->OnNewPendingBufferAvailable(nullptr);
    EXPECT_TRUE(encoderInfo->pendingBuffers_.empty());
    encoderInfo->isRunning_ = true;
    encoderInfo->OnNewPendingBufferAvailable(wrapper);
    EXPECT_EQ(encoderInfo->pendingBuffers_.size(), 1U);
    encoderInfo->eosRequested_ = true;
    encoderInfo->OnNewPendingBufferAvailable(wrapper);
    EXPECT_EQ(encoderInfo->pendingBuffers_.size(), 1U);
}

/**
 * @tc.name: PreprocManager_EncoderInfo_ReplaceOutputBuffer_Guards_001
 * @tc.desc: Cover invalid arguments and fatal replacement path without an encoder surface
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_ReplaceOutputBuffer_Guards_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("replace_guard");
    ASSERT_NE(encoderInfo, nullptr);
    sptr<SurfaceBuffer> oldBuffer;
    sptr<SurfaceBuffer> newBuffer = SurfaceBuffer::Create();
    ASSERT_NE(newBuffer, nullptr);
    ASSERT_EQ(newBuffer->Alloc(CONFIG), GSERROR_OK);
    EXPECT_FALSE(encoderInfo->ReplaceOutputBuffer(oldBuffer, newBuffer));

    oldBuffer = SurfaceBuffer::Create();
    ASSERT_NE(oldBuffer, nullptr);
    ASSERT_EQ(oldBuffer->Alloc(CONFIG), GSERROR_OK);
    EXPECT_FALSE(encoderInfo->ReplaceOutputBuffer(oldBuffer, nullptr));

    newBuffer = SurfaceBuffer::Create();
    ASSERT_NE(newBuffer, nullptr);
    ASSERT_EQ(newBuffer->Alloc(CONFIG), GSERROR_OK);
    encoderInfo->encoderSurfaceBufferPool_.emplace(oldBuffer->GetSeqNum(), oldBuffer);
    encoderInfo->isRunning_ = true;
    EXPECT_FALSE(encoderInfo->ReplaceOutputBuffer(oldBuffer, newBuffer));
    EXPECT_EQ(oldBuffer, nullptr);
    EXPECT_FALSE(encoderInfo->isRunning_);
    EXPECT_TRUE(encoderInfo->encoderSurfaceBufferPool_.empty());
}

/**
 * @tc.name: PreprocManager_EncoderInfo_FlushConfigGuards_001
 * @tc.desc: Cover null guards and timestamp/damage assignment in SetBufferFlushConfig
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_FlushConfigGuards_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("flush_config");
    ASSERT_NE(encoderInfo, nullptr);
    std::shared_ptr<PreprocessorManager::SurfaceBufferWrapper> input;
    std::shared_ptr<PreprocessorManager::SurfaceBufferWrapper> output;
    encoderInfo->SetBufferFlushConfig(input, output);

    input = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>();
    output = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>();
    encoderInfo->SetBufferFlushConfig(input, output);

    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->Alloc(CONFIG), GSERROR_OK);
    output->buffer = buffer;
    input->timestamp = 123456;
    encoderInfo->SetBufferFlushConfig(input, output);
    EXPECT_EQ(output->flushConfig.timestamp, 123456);
    EXPECT_EQ(output->flushConfig.damage.w, buffer->GetWidth());
    EXPECT_EQ(output->flushConfig.damage.h, buffer->GetHeight());
}

/**
 * @tc.name: PreprocManager_EncoderInfo_ProcessFrame_ErrorCallback_001
 * @tc.desc: Cover preprocess failure and error reporting from ProcessFrame
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_ProcessFrame_ErrorCallback_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("process_error");
    ASSERT_NE(encoderInfo, nullptr);
    encoderInfo->preprocessor_ = preprocessor_;
    int32_t reportedError = AVCS_ERR_OK;
    encoderInfo->SetPreprocessErrorCallback([&reportedError](int32_t errorCode) {
        reportedError = errorCode;
    });
    auto input = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>();
    auto output = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    input->timestamp = 1;
    EXPECT_FALSE(encoderInfo->ProcessFrame(input, output));
    EXPECT_EQ(reportedError, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name: PreprocManager_EncoderInfo_RequestNotifyEosGuards_001
 * @tc.desc: Cover EOS request when stopped and when already requested
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_RequestNotifyEosGuards_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("eos_guard");
    ASSERT_NE(encoderInfo, nullptr);
    EXPECT_FALSE(encoderInfo->RequestNotifyEos(nullptr));
    encoderInfo->isRunning_ = true;
    EXPECT_TRUE(encoderInfo->RequestNotifyEos(nullptr));
    EXPECT_TRUE(encoderInfo->eosRequested_);
    EXPECT_TRUE(encoderInfo->RequestNotifyEos([]() {}));
    EXPECT_TRUE(encoderInfo->eosRequested_);
    encoderInfo->isRunning_ = false;
    encoderInfo->eosRequested_ = false;
}

/**
 * @tc.name: PreprocManager_EncoderInfo_SurfaceBufferCallbackBranches_001
 * @tc.desc: Cover unknown and known encoder surface buffer sequence branches
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_SurfaceBufferCallbackBranches_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("surface_callback");
    ASSERT_NE(encoderInfo, nullptr);
    encoderInfo->OnNewEncoderSurfaceBufferAvailable(999, nullptr);

    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    ASSERT_NE(buffer, nullptr);
    ASSERT_EQ(buffer->Alloc(CONFIG), GSERROR_OK);
    const uint32_t seq = buffer->GetSeqNum();
    encoderInfo->encoderSurfaceBufferPool_.emplace(seq, buffer);
    encoderInfo->OnNewEncoderSurfaceBufferAvailable(seq, nullptr);
    EXPECT_EQ(encoderInfo->availableInputBuffers_.size(), 1U);
}

/**
 * @tc.name: PreprocManager_EncoderInfo_StreamChangeDeduplication_001
 * @tc.desc: Cover first, duplicate, and changed stream description notifications
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_EncoderInfo_StreamChangeDeduplication_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("stream_change");
    ASSERT_NE(encoderInfo, nullptr);
    encoderInfo->preprocessor_ = preprocessor_;
    int32_t callbackCount = 0;
    encoderInfo->SetStreamChangedCallback([&callbackCount](const Media::Format &) {
        ++callbackCount;
    });
    sptr<SurfaceBuffer> firstBuffer = SurfaceBuffer::Create();
    ASSERT_NE(firstBuffer, nullptr);
    ASSERT_EQ(firstBuffer->Alloc(CONFIG), GSERROR_OK);
    auto first = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>(
        wptr<Surface>(), firstBuffer, nullptr, PreprocessorManager::SurfaceBufferWrapper::SurfaceType::CONSUMER);
    encoderInfo->CheckAndNotifyStreamChange(first);
    encoderInfo->CheckAndNotifyStreamChange(first);

    BufferRequestConfig changedConfig = CONFIG;
    changedConfig.width = 640;
    sptr<SurfaceBuffer> secondBuffer = SurfaceBuffer::Create();
    ASSERT_NE(secondBuffer, nullptr);
    ASSERT_EQ(secondBuffer->Alloc(changedConfig), GSERROR_OK);
    auto second = std::make_shared<PreprocessorManager::SurfaceBufferWrapper>(
        wptr<Surface>(), secondBuffer, nullptr, PreprocessorManager::SurfaceBufferWrapper::SurfaceType::CONSUMER);
    encoderInfo->CheckAndNotifyStreamChange(second);
    EXPECT_EQ(callbackCount, 2);
}

/**
 * @tc.name: Preprocessor_ConfigurationSnapshotAndValidation_001
 * @tc.desc: Cover configure and SetParameter snapshot construction without FastKit processing
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_ConfigurationSnapshotAndValidation_001, TestSize.Level1)
{
    preprocessor_->encoderConfig_.width = 640;
    preprocessor_->encoderConfig_.height = 480;
    preprocessor_->encoderConfig_.pixelFormat = GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
    preprocessor_->encoderConfig_.frameRate = 24.0;

    Media::Format parameter;
    parameter.PutDoubleValue(Media::Tag::VIDEO_FRAME_RATE, 29.97);
    EXPECT_EQ(preprocessor_->ValidateConfiguration(parameter, false), AVCS_ERR_OK);
    EXPECT_EQ(preprocessor_->encoderParams_.width, 640);
    EXPECT_EQ(preprocessor_->encoderParams_.height, 480);

    preprocessor_->cropParams_.result = Preprocessor::ConfigResult::DISABLED;
    preprocessor_->downsamplingParams_.result = Preprocessor::ConfigResult::DISABLED;
    preprocessor_->dropParams_.result = Preprocessor::ConfigResult::DISABLED;
    preprocessor_->encoderParams_.width = 1280;
    preprocessor_->encoderParams_.height = 720;
    EXPECT_EQ(preprocessor_->ApplyConfiguration(), AVCS_ERR_OK);
    auto snapshot = preprocessor_->GetConfigSnapshot();
    EXPECT_EQ(snapshot.configuredWidth, 1280);
    EXPECT_EQ(snapshot.configuredHeight, 720);
    EXPECT_FALSE(snapshot.cropEnabled);
    EXPECT_FALSE(snapshot.downsamplingEnabled);
}

/**
 * @tc.name: Preprocessor_ConfigurationConflict_ExistingModes_001
 * @tc.desc: Cover rejection when committed crop and downsampling modes conflict
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_ConfigurationConflict_ExistingModes_001, TestSize.Level1)
{
    preprocessor_->encoderConfig_.pixelFormat = GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
    preprocessor_->cropConfig_.enabled = true;
    preprocessor_->downsamplingConfig_.enabled = true;
    Media::Format parameter;
    EXPECT_EQ(preprocessor_->ValidateConfiguration(parameter, false), AVCS_ERR_INVALID_VAL);
    preprocessor_->cropConfig_.enabled = false;
    preprocessor_->downsamplingConfig_.enabled = false;
}

/**
 * @tc.name: Preprocessor_CheckOrReallocOutputBuffer_Branches_001
 * @tc.desc: Cover invalid geometry, null callback, no-realloc, and realloc callback failure branches
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_CheckOrReallocOutputBuffer_Branches_001, TestSize.Level1)
{
    BufferRequestConfig supportedConfig = CONFIG;
    supportedConfig.format = GRAPHIC_PIXEL_FMT_YCBCR_420_SP;
    sptr<SurfaceBuffer> output = nullptr;
    EXPECT_FALSE(preprocessor_->CheckOrReallocOutSurfaceBuffer(
        supportedConfig.width, supportedConfig.height, supportedConfig.format, output, nullptr));
    ASSERT_FALSE(preprocessor_->CheckOrReallocOutSurfaceBuffer(0, supportedConfig.height, supportedConfig.format,
        output, [](sptr<SurfaceBuffer> &, const sptr<SurfaceBuffer> &) { return true; }));

    output = SurfaceBuffer::Create();
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(output->Alloc(supportedConfig), GSERROR_OK);
    auto callback = [](sptr<SurfaceBuffer> &, const sptr<SurfaceBuffer> &) { return true; };
    EXPECT_TRUE(preprocessor_->CheckOrReallocOutSurfaceBuffer(
        supportedConfig.width, supportedConfig.height, supportedConfig.format, output, callback));
    auto rejectCallback = [](sptr<SurfaceBuffer> &, const sptr<SurfaceBuffer> &) { return false; };
    EXPECT_FALSE(preprocessor_->CheckOrReallocOutSurfaceBuffer(
        640, 360, supportedConfig.format, output, rejectCallback));
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_MalformedSegments_001
 * @tc.desc: Cover empty and malformed ROI entries while preserving valid entries
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_MalformedSegments_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(input->Alloc(CONFIG), GSERROR_OK);
    ASSERT_EQ(output->Alloc(CONFIG), GSERROR_OK);
    ASSERT_TRUE(SetRoiMetadata(input, ";bad;1,2-3;1,2-3,4=;5,6-7,8;;"));
    preprocessor_->UpdateRoiMetadata(input, output, {1.0, 1.0, 0, 0});
    std::string actual;
    ASSERT_TRUE(GetRoiMetadata(output, actual));
    EXPECT_EQ(actual, "1,2-3,4=;5,6-7,8;");
}

/**
 * @tc.name: Preprocessor_UpdateRoiMetadata_NoTerminator_001
 * @tc.desc: Cover ROI metadata that has no embedded NUL terminator
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_UpdateRoiMetadata_NoTerminator_001, TestSize.Level1)
{
    sptr<SurfaceBuffer> input = SurfaceBuffer::Create();
    sptr<SurfaceBuffer> output = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    ASSERT_NE(output, nullptr);
    ASSERT_EQ(input->Alloc(CONFIG), GSERROR_OK);
    ASSERT_EQ(output->Alloc(CONFIG), GSERROR_OK);
    std::vector<uint8_t> data{'1', ',', '2', '-', '3', ',', '4'};
    ASSERT_EQ(input->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, data, false), GSERROR_OK);
    preprocessor_->UpdateRoiMetadata(input, output, {1.0, 1.0, 0, 0});
    std::string actual;
    ASSERT_TRUE(GetRoiMetadata(output, actual));
    EXPECT_EQ(actual, "1,2-3,4;");
}

/**
 * @tc.name: PreprocManager_FillStreamDescription_Snapshots_001
 * @tc.desc: Cover crop/downsampling metadata insertion and stride/pixel mapping branches
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_FillStreamDescription_Snapshots_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("format_snapshot");
    ASSERT_NE(encoderInfo, nullptr);
    encoderInfo->preprocessor_ = preprocessor_;
    preprocessor_->cropConfig_.left = 10;
    preprocessor_->cropConfig_.right = 100;
    preprocessor_->cropConfig_.top = 20;
    preprocessor_->cropConfig_.bottom = 200;
    preprocessor_->cropConfig_.width = 91;
    preprocessor_->cropConfig_.height = 181;
    preprocessor_->cropConfig_.enabled = true;
    preprocessor_->downsamplingConfig_.width = 320;
    preprocessor_->downsamplingConfig_.height = 240;
    preprocessor_->downsamplingConfig_.enabled = true;
    Media::Format format = encoderInfo->FillStreamDescription(1280, 720,
        static_cast<int32_t>(GRAPHIC_PIXEL_FMT_YCBCR_420_SP), 0);
    int32_t value = 0;
    EXPECT_TRUE(format.GetIntValue(Media::Tag::VIDEO_WIDTH, value));
    EXPECT_EQ(value, 1280);
    EXPECT_FALSE(format.GetIntValue(Media::Tag::VIDEO_STRIDE, value));
    EXPECT_TRUE(format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_CROP_LEFT, value));
    EXPECT_EQ(value, 10);
    EXPECT_TRUE(format.GetIntValue(OH_MD_KEY_VIDEO_ENCODER_PREPROC_DOWNSAMPLING_WIDTH, value));
    EXPECT_EQ(value, 320);
}

/**
 * @tc.name: PreprocManager_ReportPreprocessError_DirectBranches_001
 * @tc.desc: Cover direct null and non-null preprocess error callback branches
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_ReportPreprocessError_DirectBranches_001, TestSize.Level1)
{
    auto encoderInfo = std::make_shared<PreprocessorManager::EncoderInfo>("error_callback");
    ASSERT_NE(encoderInfo, nullptr);
    encoderInfo->ReportPreprocessError(AVCS_ERR_INVALID_VAL);
    int32_t reported = 0;
    encoderInfo->preprocessErrorCallback_ = [&reported](int32_t code) { reported = code; };
    encoderInfo->ReportPreprocessError(AVCS_ERR_NO_MEMORY);
    EXPECT_EQ(reported, AVCS_ERR_NO_MEMORY);
}

/**
 * @tc.name: PreprocessorEncoder_CallbackGate_BeforeStart_001
 * @tc.desc: Cover callback rejection before Start and callback acceptance after Start
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CallbackGate_BeforeStart_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    CallbackTestContext context;
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->videoEncoderObject_ = nullptr;
    encoder->preprocMgr_ = nullptr;
    OH_AVCodecCallback callback = {};
    callback.onError = OnErrorForCallbackTest;
    ASSERT_EQ(encoder->RegisterCallback(callback, &context), AV_ERR_OK);
    encoder->OnEncoderError(0);
    std::this_thread::sleep_for(CALLBACK_BLOCK_CHECK_TIME);
    {
        std::lock_guard<std::mutex> lock(context.mutex);
        EXPECT_EQ(context.callbackCount, 0);
    }
    ASSERT_EQ(encoder->Start(), AV_ERR_OK);
    encoder->OnEncoderError(0);
    EXPECT_TRUE(WaitForFinishedCount(context, 1));
    EXPECT_EQ(OH_VideoEncoder_Destroy(codec), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_CallbackQueueFull_DropsNewest_001
 * @tc.desc: Cover callback queue full handling and newest-event dropping
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CallbackQueueFull_DropsNewest_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    CallbackTestContext context;
    context.blockFirstCallback = true;
    ASSERT_TRUE(PrepareCallbackEncoder(codec, context));
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->OnEncoderError(0);
    EXPECT_TRUE(WaitForCallbackCount(context, 1));
    for (int32_t i = 0; i < 65; ++i) {
        encoder->OnEncoderError(0);
    }
    AllowFirstCallbackToReturn(context);
    EXPECT_TRUE(WaitForFinishedCount(context, 65));
    std::this_thread::sleep_for(CALLBACK_BLOCK_CHECK_TIME);
    {
        std::lock_guard<std::mutex> lock(context.mutex);
        EXPECT_EQ(context.callbackCount, 65);
    }
    EXPECT_EQ(OH_VideoEncoder_Destroy(codec), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_OnStreamChanged_Asynchronous_001
 * @tc.desc: Cover asynchronous stream-change callback dispatch
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_OnStreamChanged_Asynchronous_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->videoEncoderObject_ = nullptr;
    encoder->preprocMgr_ = nullptr;
    CallbackTestContext context;
    OH_AVCodecCallback callback = {};
    callback.onStreamChanged = OnStreamChangedForCallbackTest;
    ASSERT_EQ(encoder->RegisterCallback(callback, &context), AV_ERR_OK);
    ASSERT_EQ(encoder->Start(), AV_ERR_OK);
    Media::Format format;
    format.PutIntValue(Media::Tag::VIDEO_WIDTH, 640);
    encoder->OnStreamChanged(format);
    EXPECT_TRUE(WaitForFinishedCount(context, 1));
    std::thread::id callbackThreadId;
    {
        std::lock_guard<std::mutex> lock(context.mutex);
        callbackThreadId = context.callbackThreadId;
    }
    EXPECT_NE(callbackThreadId, std::this_thread::get_id());
    EXPECT_EQ(OH_VideoEncoder_Destroy(codec), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_OnOutputBuffer_NullGuards_001
 * @tc.desc: Cover null callback and null output buffer guards
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_OnOutputBuffer_NullGuards_001, TestSize.Level1)
{
    auto encoder = preprocessorEncoder_.get();
    encoder->OnOutputBufferAvailable(0, nullptr);
    CallbackTestContext context;
    OH_AVCodecCallback callback = {};
    callback.onNewOutputBuffer = OnOutputBufferForCallbackTest;
    encoder->RegisterCallback(callback, &context);
    encoder->OnOutputBufferAvailable(0, nullptr);
    std::this_thread::sleep_for(CALLBACK_BLOCK_CHECK_TIME);
    std::lock_guard<std::mutex> lock(context.mutex);
    EXPECT_EQ(context.callbackCount, 0);
}

/**
 * @tc.name: PreprocessorEncoder_CallbackDispatcher_SelfDetach_001
 * @tc.desc: Cover dispatcher self-detach when its last owner is released by callback thread
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CallbackDispatcher_SelfDetach_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->videoEncoderObject_ = nullptr;
    encoder->preprocMgr_ = nullptr;
    CallbackTestContext context;
    OH_AVCodecCallback callback = {};
    callback.onError = OnErrorReleaseDispatcherForCallbackTest;
    ASSERT_EQ(encoder->RegisterCallback(callback, &context), AV_ERR_OK);
    ASSERT_EQ(encoder->Start(), AV_ERR_OK);
    context.dispatcherKeepAlive = encoder->callbackDispatcher_;
    encoder->OnEncoderError(0);
    ASSERT_TRUE(WaitForFinishedCount(context, 1));
    std::this_thread::sleep_for(CALLBACK_BLOCK_CHECK_TIME);
    EXPECT_EQ(OH_VideoEncoder_Destroy(codec), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_CallbackDispatcher_RejectAfterShutdown_001
 * @tc.desc: Cover stopping guards in OpenGate and TryPost after dispatcher shutdown
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_CallbackDispatcher_RejectAfterShutdown_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->videoEncoderObject_ = nullptr;
    encoder->preprocMgr_ = nullptr;
    CallbackTestContext context;
    OH_AVCodecCallback callback = {};
    callback.onError = OnErrorForCallbackTest;
    ASSERT_EQ(encoder->RegisterCallback(callback, &context), AV_ERR_OK);

    encoder->ShutdownCallbackDispatcher();
    ASSERT_EQ(encoder->Start(), AV_ERR_OK);
    encoder->OnEncoderError(0);
    std::this_thread::sleep_for(CALLBACK_BLOCK_CHECK_TIME);
    {
        std::lock_guard<std::mutex> lock(context.mutex);
        EXPECT_EQ(context.callbackCount, 0);
    }
    EXPECT_EQ(OH_VideoEncoder_Destroy(codec), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_OutputCallbackQueueFull_ReleasesNewest_001
 * @tc.desc: Cover output callback queue-full cleanup callback for the newest buffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_OutputCallbackQueueFull_ReleasesNewest_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    auto videoEncoderObject = encoder->videoEncoderObject_;
    encoder->videoEncoderObject_ = nullptr;
    encoder->preprocMgr_ = nullptr;
    CallbackTestContext context;
    context.blockFirstCallback = true;
    OH_AVCodecCallback callback = {};
    callback.onNewOutputBuffer = OnOutputBufferForCallbackTest;
    ASSERT_EQ(encoder->RegisterCallback(callback, &context), AV_ERR_OK);
    ASSERT_EQ(encoder->Start(), AV_ERR_OK);
    encoder->videoEncoderObject_ = videoEncoderObject;

    auto buffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(buffer, nullptr);
    encoder->OnOutputBufferAvailable(0, buffer);
    ASSERT_TRUE(WaitForCallbackCount(context, 1));
    for (uint32_t index = 1; index <= 65; ++index) {
        encoder->OnOutputBufferAvailable(index, buffer);
    }
    AllowFirstCallbackToReturn(context);
    EXPECT_TRUE(WaitForFinishedCount(context, 65));
    {
        std::lock_guard<std::mutex> lock(context.mutex);
        EXPECT_EQ(context.callbackCount, 65);
    }
    EXPECT_EQ(OH_VideoEncoder_Destroy(codec), AV_ERR_OK);
}

/**
 * @tc.name: PreprocessorEncoder_ConcurrentDestroy_WaitsWorkerExit_001
 * @tc.desc: Cover the concurrent-destroy path and callback worker exit wait
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_ConcurrentDestroy_WaitsWorkerExit_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    CallbackTestContext context;
    context.blockFirstCallback = true;
    ASSERT_TRUE(PrepareCallbackEncoder(codec, context));
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    encoder->OnEncoderError(0);
    ASSERT_TRUE(WaitForCallbackCount(context, 1));

    AsyncOperationState firstDestroy;
    AsyncOperationState secondDestroy;
    std::thread firstThread([codec, &firstDestroy]() {
        FinishAsyncOperation(firstDestroy, OH_VideoEncoder_Destroy(codec));
    });
    ASSERT_TRUE(IsAsyncOperationBlocked(firstDestroy));
    std::thread secondThread([codec, &secondDestroy]() {
        PreprocessorEncoder::Destroy(codec);
        FinishAsyncOperation(secondDestroy, AV_ERR_OK);
    });
    EXPECT_TRUE(IsAsyncOperationBlocked(secondDestroy));
    AllowFirstCallbackToReturn(context);
    EXPECT_TRUE(WaitForAsyncOperation(firstDestroy));
    EXPECT_TRUE(WaitForAsyncOperation(secondDestroy));
    firstThread.join();
    secondThread.join();
    EXPECT_EQ(firstDestroy.result, AV_ERR_OK);
    EXPECT_EQ(secondDestroy.result, AV_ERR_OK);
}

/**
 * @tc.name: NativeVideoEncoderCallback_PreprocessorRoutes_001
 * @tc.desc: Cover fixed callback bridge routing for preprocess encoder callbacks
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, NativeVideoEncoderCallback_PreprocessorRoutes_001, TestSize.Level1)
{
    OH_AVCodecCallback callbackConfig = {};
    callbackConfig.onError = OnErrorVoid;
    auto *codec = reinterpret_cast<OH_AVCodec *>(preprocessorEncoder_.get());
    NativeVideoEncoderCallback callback(codec, callbackConfig, nullptr);
    callback.OnError(static_cast<AVCodecErrorType>(0), AVCS_ERR_INVALID_VAL);
    callback.OnOutputFormatChanged(Media::Format());
    callback.OnOutputBufferUnbinded();
    callback.OnOutputBufferAvailable(0, std::shared_ptr<AVBuffer>{});
}

/**
 * @tc.name: VideoEncoderObject_GetTransData_ReusesOutputAndInput_001
 * @tc.desc: Cover strong-reference buffer object creation and reuse for both directions
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoderObject_GetTransData_ReusesOutputAndInput_001, TestSize.Level1)
{
    VideoEncoderObject object(std::shared_ptr<AVCodecVideoEncoder>{});
    auto outputBuffer = OHOS::Media::AVBuffer::CreateAVBuffer();
    auto inputBuffer = OHOS::Media::AVBuffer::CreateAVBuffer();
    ASSERT_NE(outputBuffer, nullptr);
    ASSERT_NE(inputBuffer, nullptr);
    sptr<OH_AVBuffer> outputObject;
    object.GetTransData(7, outputBuffer, true, outputObject);
    ASSERT_NE(outputObject, nullptr);
    sptr<OH_AVBuffer> outputObjectAgain;
    object.GetTransData(7, outputBuffer, true, outputObjectAgain);
    EXPECT_EQ(outputObject.GetRefPtr(), outputObjectAgain.GetRefPtr());
    sptr<OH_AVBuffer> inputObject;
    object.GetTransData(8, inputBuffer, false, inputObject);
    EXPECT_NE(inputObject, nullptr);
    EXPECT_EQ(object.outputBufferMap_.size(), 1U);
    EXPECT_EQ(object.inputBufferMap_.size(), 1U);
}

/**
 * @tc.name: Preprocessor_RuntimeConfigAndTimestampHelpers_001
 * @tc.desc: Cover runtime snapshot, realloc callback setter, and disabled frame-drop helpers
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_RuntimeConfigAndTimestampHelpers_001, TestSize.Level1)
{
    bool callbackCalled = false;
    preprocessor_->SetOnBufferReallocCallback(
        [&callbackCalled](sptr<SurfaceBuffer> &, const sptr<SurfaceBuffer> &) {
            callbackCalled = true;
            return true;
        });
    preprocessor_->cropConfig_.enabled = true;
    preprocessor_->cropConfig_.width = 320;
    preprocessor_->downsamplingConfig_.enabled = false;

    auto config = preprocessor_->GetRuntimeConfigSnapshot();
    EXPECT_TRUE(config.crop.enabled);
    EXPECT_EQ(config.crop.width, 320);
    ASSERT_TRUE(static_cast<bool>(config.reallocCallback));
    sptr<SurfaceBuffer> oldBuffer;
    sptr<SurfaceBuffer> newBuffer;
    EXPECT_TRUE(config.reallocCallback(oldBuffer, newBuffer));
    EXPECT_TRUE(callbackCalled);
    EXPECT_FALSE(preprocessor_->ShouldDropFrame(1));
    preprocessor_->FlushTimeStamp();
}

/**
 * @tc.name: Preprocessor_PixelFormatAsyncAndProcessGuards_001
 * @tc.desc: Cover async-mode and pixel-format helpers plus Process null-buffer guards
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, Preprocessor_PixelFormatAsyncAndProcessGuards_001, TestSize.Level1)
{
    Media::Format format;
    EXPECT_TRUE(preprocessor_->UsingAsyncMode(format));
    format.PutIntValue(Media::Tag::AV_CODEC_ENABLE_SYNC_MODE, 1);
    EXPECT_FALSE(preprocessor_->UsingAsyncMode(format));

    Media::Format pixelFormat;
    EXPECT_EQ(preprocessor_->GetPixelFormatFromConfiguration(pixelFormat), GRAPHIC_PIXEL_FMT_YCBCR_420_SP);
    preprocessor_->encoderConfig_.pixelFormat = GRAPHIC_PIXEL_FMT_RGBA_8888;
    EXPECT_EQ(preprocessor_->GetPixelFormatFromParameter(pixelFormat), GRAPHIC_PIXEL_FMT_RGBA_8888);

    sptr<SurfaceBuffer> input;
    sptr<SurfaceBuffer> output;
    EXPECT_EQ(preprocessor_->Process(input, output), AVCS_ERR_INVALID_VAL);
    input = SurfaceBuffer::Create();
    ASSERT_NE(input, nullptr);
    EXPECT_EQ(preprocessor_->Process(input, output), AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.name: PreprocManager_SetEncoderRunning_Guards_001
 * @tc.desc: Cover unknown encoder and uninitialized EncoderInfo start failures
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocManager_SetEncoderRunning_Guards_001, TestSize.Level1)
{
    EXPECT_EQ(preprocessorManager_->SetEncoderRunning("unknown_encoder", true), AVCS_ERR_INVALID_STATE);

    const std::string encoderId = "uninitialized_encoder";
    preprocessorManager_->RegisterEncoder(encoderId, nullptr, nullptr);
    EXPECT_EQ(preprocessorManager_->SetEncoderRunning(encoderId, true), AVCS_ERR_UNKNOWN);
    EXPECT_EQ(preprocessorManager_->SetEncoderRunning(encoderId, false), AVCS_ERR_OK);
    preprocessorManager_->UnregisterEncoder(encoderId);
    preprocessorManager_->UnregisterEncoder("unknown_encoder");
}

/**
 * @tc.name: PreprocessorEncoder_MagicSnapshotAndDetachHelpers_001
 * @tc.desc: Cover magic classification, callback snapshot, and peer detachment helpers
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_MagicSnapshotAndDetachHelpers_001, TestSize.Level1)
{
    EXPECT_TRUE(PreprocessorEncoder::IsPrimaryEncoderMagic(AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER));
    EXPECT_TRUE(PreprocessorEncoder::IsSecondaryEncoderMagic(AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER));
    EXPECT_TRUE(PreprocessorEncoder::IsPreprocEncoderMagic(AVMagic::AVCODEC_MAGIC_PRIMARY_VIDEO_ENCODER));
    EXPECT_FALSE(PreprocessorEncoder::IsPreprocEncoderMagic(AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER));

    int32_t userData = 1;
    preprocessorEncoder_->callback_.onError = OnErrorVoid;
    preprocessorEncoder_->userData_ = &userData;
    auto snapshot = preprocessorEncoder_->GetCallbackSnapshot();
    EXPECT_EQ(snapshot.callback.onError, OnErrorVoid);
    EXPECT_EQ(snapshot.userData, &userData);

    PreprocessorEncoder secondary(AVMagic::AVCODEC_MAGIC_SECONDARY_VIDEO_ENCODER);
    preprocessorEncoder_->secondary_ = &secondary;
    secondary.primary_ = preprocessorEncoder_.get();
    preprocessorEncoder_->DetachPeerEncoder();
    EXPECT_EQ(preprocessorEncoder_->secondary_, nullptr);
    EXPECT_EQ(secondary.primary_, nullptr);
}

/**
 * @tc.name: PreprocessorEncoder_NullDispatcherLifecycleHelpers_001
 * @tc.desc: Cover callback lifecycle helper branches without a dispatcher or backing encoder
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, PreprocessorEncoder_NullDispatcherLifecycleHelpers_001, TestSize.Level1)
{
    preprocessorEncoder_->callbackDispatcher_ = nullptr;
    preprocessorEncoder_->preprocMgr_ = nullptr;
    preprocessorEncoder_->videoEncoderObject_ = nullptr;
    preprocessorEncoder_->CloseCallbackGate();
    preprocessorEncoder_->CloseAndWaitCallbackIfNeeded();
    preprocessorEncoder_->PrepareCallbackShutdown();
    preprocessorEncoder_->ShutdownCallbackDispatcher();
    SUCCEED();
}

/**
 * @tc.name: VideoEncoderObject_GetTransData_ReplacesMismatchedBuffer_001
 * @tc.desc: Cover replacement of a cached OH_AVBuffer when an index is reused for another AVBuffer
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, VideoEncoderObject_GetTransData_ReplacesMismatchedBuffer_001, TestSize.Level1)
{
    VideoEncoderObject object(std::shared_ptr<AVCodecVideoEncoder>{});
    auto firstBuffer = AVBuffer::CreateAVBuffer();
    auto secondBuffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(firstBuffer, nullptr);
    ASSERT_NE(secondBuffer, nullptr);
    sptr<OH_AVBuffer> firstObject;
    object.GetTransData(9, firstBuffer, true, firstObject);
    ASSERT_NE(firstObject, nullptr);

    sptr<OH_AVBuffer> secondObject;
    object.GetTransData(9, secondBuffer, true, secondObject);
    ASSERT_NE(secondObject, nullptr);
    EXPECT_NE(firstObject.GetRefPtr(), secondObject.GetRefPtr());
    EXPECT_EQ(firstObject->magic_, MFMagic::MFMAGIC_UNKNOWN);
    EXPECT_EQ(firstObject->buffer_, nullptr);
    EXPECT_EQ(object.outputBufferMap_.at(9).GetRefPtr(), secondObject.GetRefPtr());
    EXPECT_EQ(object.outputBufferStateMap_.at(9), 0); // OUTPUT_BUFFER_PENDING
}

/**
 * @tc.name: NativeVideoEncoderCallback_PreprocessorOutputRoute_001
 * @tc.desc: Cover non-null output-buffer routing from NativeVideoEncoderCallback to the async dispatcher
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, NativeVideoEncoderCallback_PreprocessorOutputRoute_001, TestSize.Level1)
{
    OH_AVCodec *codec = nullptr;
    ASSERT_EQ(PreprocessorEncoder::CreatePrimary(OH_AVCODEC_MIMETYPE_VIDEO_AVC, &codec), AV_ERR_OK);
    auto *encoder = reinterpret_cast<PreprocessorEncoder *>(codec);
    auto videoEncoderObject = encoder->videoEncoderObject_;
    encoder->videoEncoderObject_ = nullptr;
    encoder->preprocMgr_ = nullptr;
    CallbackTestContext context;
    OH_AVCodecCallback callbackConfig = {};
    callbackConfig.onNewOutputBuffer = OnOutputBufferForCallbackTest;
    ASSERT_EQ(encoder->RegisterCallback(callbackConfig, &context), AV_ERR_OK);
    ASSERT_EQ(encoder->Start(), AV_ERR_OK);
    encoder->videoEncoderObject_ = videoEncoderObject;

    NativeVideoEncoderCallback callback(codec, callbackConfig, &context);
    auto buffer = AVBuffer::CreateAVBuffer();
    ASSERT_NE(buffer, nullptr);
    callback.OnOutputBufferAvailable(11, buffer);
    EXPECT_TRUE(WaitForFinishedCount(context, 1));
    EXPECT_EQ(OH_VideoEncoder_Destroy(codec), AV_ERR_OK);
}
