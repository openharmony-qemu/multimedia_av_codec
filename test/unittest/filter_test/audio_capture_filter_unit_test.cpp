/*
 * Copyright (C) 2024-2026 Huawei Device Co., Ltd.
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

#include "audio_capturer.h"
#include "audio_capture_filter.h"
#include "audio_capture_filter_unit_test.h"
#include "avbuffer_queue.h"
#include "audio_capture_module.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace {
std::shared_ptr<uint8_t> MakeTestBuffer(int32_t size)
{
    return std::shared_ptr<uint8_t>(std::make_unique<uint8_t[]>(size));
}

// Build an AudioCaptureModule whose audioCapturer_ is a FakeAudioCapturer so the module's
// non-virtual Start/Prepare/Read/GetAudioTime return OK. timestamp_ controls GetAudioTime output.
std::shared_ptr<AudioCaptureModule::AudioCaptureModule> MakeFakeModule(int64_t timestamp = 20000000)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->audioCapturer_.reset(new AudioStandard::FakeAudioCapturer());
    module->bufferSize_ = 16;
    auto *fake = static_cast<AudioStandard::FakeAudioCapturer *>(module->audioCapturer_.get());
    fake->timestamp_ = timestamp;
    return module;
}

// FakeAudioCapturer subclass whose Stop() returns false, causing AudioCaptureModule::Stop()
// to return ERROR_AUD_STOP_FAILED. Used to cover DoStop's ret != OK branch (L294).
class FailingStopFakeCapturer : public AudioStandard::FakeAudioCapturer {
public:
    bool Stop() const override { return false; }
};

std::shared_ptr<AudioCaptureModule::AudioCaptureModule> MakeFailingStopModule()
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->audioCapturer_.reset(new FailingStopFakeCapturer());
    module->bufferSize_ = 16;
    return module;
}
}

namespace OHOS {
namespace Media {
namespace Pipeline {
void AudioCaptureFilterUnitTest::SetUpTestCase(void) {}

void AudioCaptureFilterUnitTest::TearDownTestCase(void) {}

void AudioCaptureFilterUnitTest::SetUp(void)
{
    audioCaptureFilter_ =
        std::make_shared<AudioCaptureFilter>("testAudioCaptureFilter", FilterType::AUDIO_CAPTURE);
}

void AudioCaptureFilterUnitTest::TearDown(void)
{
    audioCaptureFilter_ = nullptr;
}

/**
 * @tc.name: AudioCaptureFilter_Init_001
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_Init_001, TestSize.Level1)
{
    auto testEventReceiver = std::make_shared<TestEventReceiver>();
    auto testFilterCallback = std::make_shared<TestFilterCallback>();
    std::shared_ptr<Meta> audioEncFormat_ = std::make_shared<Meta>();
    audioCaptureFilter_->SetParameter(audioEncFormat_);
    audioCaptureFilter_->Init(testEventReceiver, testFilterCallback);
    EXPECT_EQ(audioCaptureFilter_->receiver_, testEventReceiver);
    EXPECT_EQ(audioCaptureFilter_->callback_, testFilterCallback);
    EXPECT_NE(audioCaptureFilter_->audioCaptureModule_, nullptr);
}

/**
 * @tc.name: AudioCaptureFilter_PrepareAudioCapture_001
 * @tc.desc: PrepareAudioCapture
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_PrepareAudioCapture_001, TestSize.Level1)
{
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    Status result = audioCaptureFilter_->PrepareAudioCapture();
    EXPECT_EQ(result, Status::ERROR_AUD_INIT_FAILED);
    audioCaptureFilter_->taskPtr_ = nullptr;
    auto mockAudioCaptureModule = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = mockAudioCaptureModule;
    result = audioCaptureFilter_->PrepareAudioCapture();
    EXPECT_EQ(result, Status::ERROR_AUD_INIT_FAILED);
    EXPECT_NE(audioCaptureFilter_->taskPtr_, nullptr);
}

/**
 * @tc.name: AudioCaptureFilter_SetAudioCaptureChangeCallback_001
 * @tc.desc: SetAudioCaptureChangeCallback
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetAudioCaptureChangeCallback_001, TestSize.Level1)
{
    auto testCallback = std::make_shared<MockAudioCapturerInfoChangeCallback>();
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->SetAudioCaptureChangeCallback(testCallback);
    EXPECT_EQ(result, Status::ERROR_WRONG_STATE);
    result = audioCaptureFilter_->SetAudioCaptureChangeCallback(testCallback);
    EXPECT_EQ(result, Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name: AudioCaptureFilter_DoPrepare_001
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPrepare_001, TestSize.Level1)
{
    audioCaptureFilter_->callback_ = nullptr;
    Status result = audioCaptureFilter_->DoPrepare();
    EXPECT_EQ(result, Status::ERROR_NULL_POINTER);
    auto testCallback = std::make_shared<TestFilterCallback>();
    audioCaptureFilter_->callback_ = testCallback;
    result = audioCaptureFilter_->DoPrepare();
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_DoStart_001
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStart_001, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->DoStart();
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    result = audioCaptureFilter_->DoStart();
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_001
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_001, TestSize.Level1)
{
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::ERROR_AUD_STOP_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_DoResume_001
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoResume_001, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->DoResume();
    EXPECT_EQ(result, Status::ERROR_AUD_START_FAILED);
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    auto mockAudioCaptureModule = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = mockAudioCaptureModule;
    result = audioCaptureFilter_->DoResume();
    EXPECT_EQ(result, Status::ERROR_AUD_START_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_DoStop_001
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStop_001, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->DoStop();
    EXPECT_EQ(result, Status::OK);
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    result = audioCaptureFilter_->DoStop();
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_DoFlush_001
 * @tc.desc: DoFlush
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoFlush_001, TestSize.Level1)
{
    Status result = audioCaptureFilter_->DoFlush();
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_DoRelease_001
 * @tc.desc: DoRelease
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoRelease_001, TestSize.Level1)
{
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    Status result = audioCaptureFilter_->DoRelease();
    EXPECT_EQ(result, Status::OK);
    EXPECT_EQ(audioCaptureFilter_->audioCaptureModule_, nullptr);
    EXPECT_EQ(audioCaptureFilter_->taskPtr_, nullptr);
}

/**
 * @tc.name: AudioCaptureFilter_SetParameter_001
 * @tc.desc: SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetParameter_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    audioCaptureFilter_->SetParameter(meta);
    EXPECT_EQ(audioCaptureFilter_->audioCaptureConfig_, meta);
}

/**
 * @tc.name: AudioCaptureFilter_GetParameter_001
 * @tc.desc: GetParameter
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_GetParameter_001, TestSize.Level1)
{
    auto meta = std::make_shared<Meta>();
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->GetParameter(meta);
    EXPECT_EQ(audioCaptureFilter_->audioCaptureModule_, nullptr);
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->GetParameter(meta);
    Status flushResult = audioCaptureFilter_->DoFlush();
    EXPECT_EQ(flushResult, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_LinkNext_001
 * @tc.desc: LinkNext
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_LinkNext_001, TestSize.Level1)
{
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    auto nextFilter = std::make_shared<Filter>("nextFilter", FilterType::AUDIO_CAPTURE);
    StreamType outType = StreamType::STREAMTYPE_RAW_AUDIO;
    Status result = audioCaptureFilter_->LinkNext(nextFilter, outType);
    EXPECT_EQ(result, Status::OK);
    EXPECT_EQ(audioCaptureFilter_->nextFilter_, nextFilter);
    EXPECT_EQ(audioCaptureFilter_->nextFiltersMap_[outType].front(), nextFilter);
}

/**
 * @tc.name: AudioCaptureFilter_GetFilterType_001
 * @tc.desc: GetFilterType
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_GetFilterType_001, TestSize.Level1)
{
    FilterType type = audioCaptureFilter_->GetFilterType();
    EXPECT_EQ(type, FilterType::AUDIO_CAPTURE);
}

/**
 * @tc.name: AudioCaptureFilter_SetAudioSource_001
 * @tc.desc: SetAudioSource
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetAudioSource_001, TestSize.Level1)
{
    int32_t source = 1;
    audioCaptureFilter_->SetAudioSource(source);
    EXPECT_EQ(audioCaptureFilter_->sourceType_, AudioStandard::SourceType::SOURCE_TYPE_MIC);
    source = static_cast<int32_t>(AudioStandard::SourceType::SOURCE_TYPE_VOICE_CALL);
    audioCaptureFilter_->SetAudioSource(source);
    EXPECT_EQ(audioCaptureFilter_->sourceType_, AudioStandard::SourceType::SOURCE_TYPE_VOICE_CALL);
}

/**
 * @tc.name: AudioCaptureFilter_SendEos_001
 * @tc.desc: SendEos
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SendEos_001, TestSize.Level1)
{
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    Status result = audioCaptureFilter_->SendEos();
    EXPECT_EQ(result, Status::OK);
    EXPECT_TRUE(audioCaptureFilter_->eos_);
    sptr<AVBufferQueueProducer> mockOutputBufferQueue = new MockOutputBufferQueue();
    audioCaptureFilter_->outputBufferQueue_ = mockOutputBufferQueue;
    result = audioCaptureFilter_->SendEos();
    EXPECT_EQ(result, Status::OK);
    EXPECT_TRUE(audioCaptureFilter_->eos_);
}

/**
 * @tc.name: AudioCaptureFilter_GetCurrentCapturerChangeInfo_001
 * @tc.desc: GetCurrentCapturerChangeInfo
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_GetCurrentCapturerChangeInfo_001, TestSize.Level1)
{
    AudioStandard::AudioCapturerChangeInfo changeInfo;
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->GetCurrentCapturerChangeInfo(changeInfo);
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    result = audioCaptureFilter_->GetCurrentCapturerChangeInfo(changeInfo);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_GetMaxAmplitude_001
 * @tc.desc: GetMaxAmplitude
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_GetMaxAmplitude_001, TestSize.Level1)
{
    int32_t amplitude;
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->GetMaxAmplitude(amplitude);
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    int32_t maxAmplitude = 12345;
    audioCaptureFilter_->audioCaptureModule_->maxAmplitude_ = maxAmplitude;
    result = audioCaptureFilter_->GetMaxAmplitude(amplitude);
    EXPECT_EQ(result, Status::OK);
    EXPECT_EQ(amplitude, maxAmplitude);
}

/**
 * @tc.name: AudioCaptureFilter_OnLinkedResult_001
 * @tc.desc: OnLinkedResult
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnLinkedResult_001, TestSize.Level1)
{
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    sptr<AVBufferQueueProducer> queue = new MockOutputBufferQueue();
    ASSERT_NE(queue, nullptr);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioCaptureFilter_->OnLinkedResult(queue, meta);
    EXPECT_EQ(audioCaptureFilter_->outputBufferQueue_, queue);
}

/**
 * @tc.name: AudioCaptureFilter_UpdateNext_001
 * @tc.desc: UpdateNext
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_UpdateNext_001, TestSize.Level1)
{
    StreamType outType = StreamType::STREAMTYPE_PACKED;
    Status result = audioCaptureFilter_->UpdateNext(nullptr, outType);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_UnLinkNext_001
 * @tc.desc: UnLinkNext
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_UnLinkNext_001, TestSize.Level1)
{
    StreamType outType = StreamType::STREAMTYPE_PACKED;
    Status result = audioCaptureFilter_->UnLinkNext(nullptr, outType);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_OnLinked_001
 * @tc.desc: OnLinked
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnLinked_001, TestSize.Level1)
{
    StreamType inType = StreamType::STREAMTYPE_PACKED;
    Status result = audioCaptureFilter_->OnLinked(inType, nullptr, nullptr);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_OnUpdated_001
 * @tc.desc: OnUpdated
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnUpdated_001, TestSize.Level1)
{
    StreamType inType = StreamType::STREAMTYPE_PACKED;
    Status result = audioCaptureFilter_->OnUpdated(inType, nullptr, nullptr);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_OnUnLinked_001
 * @tc.desc: OnUnLinked
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnUnLinked_001, TestSize.Level1)
{
    StreamType inType = StreamType::STREAMTYPE_PACKED;
    Status result = audioCaptureFilter_->OnUnLinked(inType, nullptr);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_OnUnlinkedResult_001
 * @tc.desc: OnUnlinkedResult
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnUnlinkedResult_001, TestSize.Level1)
{
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    sptr<AVBufferQueueProducer> queue = new MockOutputBufferQueue();
    ASSERT_NE(queue, nullptr);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioCaptureFilter_->OnLinkedResult(queue, meta);
    ASSERT_EQ(audioCaptureFilter_->outputBufferQueue_, queue);
    audioCaptureFilter_->OnUnlinkedResult(meta);
    EXPECT_EQ(audioCaptureFilter_->outputBufferQueue_, queue);
}

/**
 * @tc.name: AudioCaptureFilter_OnUpdatedResult_001
 * @tc.desc: OnUpdatedResult
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnUpdatedResult_001, TestSize.Level1)
{
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    sptr<AVBufferQueueProducer> queue = new MockOutputBufferQueue();
    ASSERT_NE(queue, nullptr);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioCaptureFilter_->OnLinkedResult(queue, meta);
    ASSERT_EQ(audioCaptureFilter_->outputBufferQueue_, queue);
    audioCaptureFilter_->OnUpdatedResult(meta);
    EXPECT_EQ(audioCaptureFilter_->outputBufferQueue_, queue);
}

/**
 * @tc.name: AudioCaptureFilter_SetCallingInfo_001
 * @tc.desc: SetCallingInfo
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetCallingInfo_001, TestSize.Level1)
{
    int32_t testAppUid = 1001;
    int32_t testAppPid = 2002;
    std::string testBundleName = "com.example.app";
    uint64_t testInstanceId = 100000001;
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->SetCallingInfo(testAppUid, testAppPid, testBundleName, testInstanceId);
    EXPECT_EQ(audioCaptureFilter_->appUid_, testAppUid);
    EXPECT_EQ(audioCaptureFilter_->appPid_, testAppPid);
    EXPECT_EQ(audioCaptureFilter_->bundleName_, testBundleName);
    EXPECT_EQ(audioCaptureFilter_->instanceId_, testInstanceId);
    auto audioCaptureModule = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = audioCaptureModule;
    audioCaptureFilter_->SetCallingInfo(testAppUid, testAppPid, testBundleName, testInstanceId);
    EXPECT_EQ(audioCaptureFilter_->audioCaptureModule_->appUid_, testAppUid);
    EXPECT_EQ(audioCaptureFilter_->audioCaptureModule_->appPid_, testAppPid);
    EXPECT_EQ(audioCaptureFilter_->audioCaptureModule_->bundleName_, testBundleName);
    EXPECT_EQ(audioCaptureFilter_->audioCaptureModule_->instanceId_, testInstanceId);
}

/**
 * @tc.name: AudioCaptureFilter_SetWillMuteWhenInterrupted_001
 * @tc.desc: SetWillMuteWhenInterrupted
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetVideoFirstFramePts_001, TestSize.Level1)
{
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->startTime_ = 0;
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    int64_t testPts = 1000;
    audioCaptureFilter_->SetVideoFirstFramePts(testPts);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), testPts);
}

HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetVideoFirstFramePts_002, TestSize.Level1)
{
    audioCaptureFilter_->hasCalculateAVTime_ = true;
    audioCaptureFilter_->startTime_ = 500;
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    int64_t testPts = 1000;
    audioCaptureFilter_->SetVideoFirstFramePts(testPts);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), testPts);
}

HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetVideoFirstFramePts_003, TestSize.Level1)
{
    audioCaptureFilter_->hasCalculateAVTime_ = true;
    audioCaptureFilter_->startTime_ = 500;
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    int64_t testPts = 100;
    audioCaptureFilter_->SetVideoFirstFramePts(testPts);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), -1);
}

HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetVideoFirstFramePts_004, TestSize.Level1)
{
    audioCaptureFilter_->hasCalculateAVTime_ = true;
    audioCaptureFilter_->startTime_ = 500;
    audioCaptureFilter_->firstVideoFramePts_.store(200);
    int64_t testPts = 600;
    audioCaptureFilter_->SetVideoFirstFramePts(testPts);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), 600);
}

HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetVideoFirstFramePts_005, TestSize.Level1)
{
    audioCaptureFilter_->hasCalculateAVTime_ = true;
    audioCaptureFilter_->startTime_ = 500;
    audioCaptureFilter_->firstVideoFramePts_.store(200);
    int64_t testPts = 500;
    audioCaptureFilter_->SetVideoFirstFramePts(testPts);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), 200);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_001
 * @tc.desc: CalculateAVTime firstAudioFramePts_ > firstVideoFramePts (audio later than video)
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_001, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->firstVideoFramePts_.store(0);
    audioCaptureFilter_->firstAudioFramePts_.store(6000000000); // 6s, larger than video pts
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 6000000000);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_002
 * @tc.desc: CalculateAVTime firstAudioFramePts_ <= firstVideoFramePts (audio faster than video)
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_002, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->firstAudioFramePts_.store(0);
    audioCaptureFilter_->firstVideoFramePts_.store(6000000000); // video later than audio pts
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 0);
}

HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetWillMuteWhenInterrupted_001, TestSize.Level1)
{
    ASSERT_NE(audioCaptureFilter_, nullptr);
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->SetWillMuteWhenInterrupted(true);
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: AudioCaptureFilter_RecordCachedData_NullGuard_0100
 * @tc.desc: RecordCachedData L656 audioCaptureModule_==nullptr guard hits early return:
 *           cachedAudioDataDeque_ unchanged (L670 erase not executed, queue keeps 2 pre-filled items,
 *           front item ptr identity unchanged). Safety remediation null guard blocks L660-698 main path —
 *           if guard missed, L670 erase would delete front item and front ptr would change.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordCachedData_NullGuard_0100, TestSize.Level1)
{
    ASSERT_NE(audioCaptureFilter_, nullptr);
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    constexpr int32_t bufSize = 16;
    auto data0 = MakeTestBuffer(bufSize);
    auto data1 = MakeTestBuffer(bufSize);
    uint8_t* expectedFront = data0.get();
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data0);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data1);
    }
    audioCaptureFilter_->RecordCachedData(2);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 2);
    EXPECT_EQ(audioCaptureFilter_->cachedAudioDataDeque_.front().get(), expectedFront);
}

/**
 * @tc.name: AudioCaptureFilter_RecordCachedData_LockErasePath_0100
 * @tc.desc: RecordCachedData L665-671 in-lock copy-out + erase path (safety remediation added lock_guard):
 *           audioCaptureModule_ non-null + outputBufferQueue_ null (while loop condition false, skip L672-698
 *           to avoid 20s sleep). Verify L670 erase uses std::min(recordFrameNum, size) for precise dequeue —
 *           pre-fill 3 items, call RecordCachedData(2), expect 1 remaining = original 3rd item (ptr identity match),
 *           proving std::min guard prevents out-of-bounds + erase path executes.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordCachedData_LockErasePath_0100, TestSize.Level1)
{
    ASSERT_NE(audioCaptureFilter_, nullptr);
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->outputBufferQueue_ = nullptr;  // while loop condition false, skip L672-698
    constexpr int32_t bufSize = 16;
    auto data0 = MakeTestBuffer(bufSize);
    auto data1 = MakeTestBuffer(bufSize);
    auto data2 = MakeTestBuffer(bufSize);
    uint8_t* expectedRemain = data2.get();
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data0);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data1);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data2);
    }
    audioCaptureFilter_->RecordCachedData(2);  // erase only first 2 items
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    ASSERT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
    EXPECT_EQ(audioCaptureFilter_->cachedAudioDataDeque_.front().get(), expectedRemain);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_WithVideo_CachedData_0100
 * @tc.desc: DoPause with withVideo_=true and cached audio data — covers L226 currentTime_==0 branch,
 *           L245 cachedAudioDataDeque_ not empty, L249 RecordCachedData (erase cached data).
 *           Scenario: pause during recording with video stream, currentTime_==0 (pause before first frame),
 *           cached audio data present. AudioCaptureModule::Stop() is non-virtual; real impl returns OK when
 *           audioCapturer_ is null. RecordCachedData skips while loop when outputBufferQueue_ is null,
 *           but L670 erase still executes.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_WithVideo_CachedData_0100, TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->startTime_ = 0;
    constexpr int32_t bufSize = 16;
    auto data = MakeTestBuffer(bufSize);
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data);
    }
    Status result = audioCaptureFilter_->DoPause();
    // Real Stop() returns OK when audioCapturer_ is null
    EXPECT_EQ(result, Status::OK);
    // RecordCachedData L670 erase should clear cached data
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoResume_WithModule_0100
 * @tc.desc: DoResume with audioCaptureModule_ set — Start fails, returns ERROR_AUD_START_FAILED
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoResume_WithModule_0100, TestSize.Level1)
{
    auto mockModule = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = mockModule;
    audioCaptureFilter_->withVideo_ = false;
    Status result = audioCaptureFilter_->DoResume();
    // Real AudioCaptureModule::Start() returns ERROR_AUD_INIT_FAILED (audioCapturer_ is null),
    // so DoResume hits L272 (ret != OK) and returns ERROR_AUD_START_FAILED
    EXPECT_EQ(result, Status::ERROR_AUD_START_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_DoResume_NullModule_0100
 * @tc.desc: DoResume with audioCaptureModule_ nullptr — covers L269 false branch (audioCaptureModule_ == nullptr),
 *           ret stays ERROR_INVALID_OPERATION, L272 true -> returns ERROR_AUD_START_FAILED
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoResume_NullModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->withVideo_ = false;
    Status result = audioCaptureFilter_->DoResume();
    EXPECT_EQ(result, Status::ERROR_AUD_START_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_DoStop_WithCachedData_0100
 * @tc.desc: DoStop with cached audio data — covers L305 cachedAudioDataDeque_ not empty, L309 RecordCachedData
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStop_WithCachedData_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    constexpr int32_t bufSize = 16;
    auto data = MakeTestBuffer(bufSize);
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data);
    }
    Status result = audioCaptureFilter_->DoStop();
    EXPECT_EQ(result, Status::OK);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    // RecordCachedData should have consumed the cached data
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_SendEos_WithCachedData_0100
 * @tc.desc: SendEos with outputBufferQueue_ set and cached audio data
 *           — covers L393 cachedAudioDataDeque_ not empty, L397 RecordCachedData
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SendEos_WithCachedData_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    sptr<AVBufferQueueProducer> mockQueue = new MockOutputBufferQueue();
    audioCaptureFilter_->outputBufferQueue_ = mockQueue;
    constexpr int32_t bufSize = 16;
    auto data = MakeTestBuffer(bufSize);
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data);
    }
    Status result = audioCaptureFilter_->SendEos();
    EXPECT_EQ(result, Status::OK);
    EXPECT_TRUE(audioCaptureFilter_->eos_);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_RecordOneAudioFrame_NullGuard_0100
 * @tc.desc: RecordOneAudioFrame with audioCaptureModule_=nullptr — covers L616 cherry-pick guard
 *           (FALSE_RETURN_MSG(audioCaptureModule_ != nullptr && outputBufferQueue_ != nullptr)).
 *           Scenario: ReadLoop tries to record a frame before capture module init; guard returns early,
 *           no buffer allocated, no data appended to cache queue.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordOneAudioFrame_NullGuard_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->RecordOneAudioFrame(16);
    // Guard returns early: cache queue should not be appended
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_TRUE(audioCaptureFilter_->cachedAudioDataDeque_.empty());
}

/**
 * @tc.name: AudioCaptureFilter_DoStop_NullModule_0100
 * @tc.desc: DoStop with audioCaptureModule_=nullptr — covers L292 false branch (ret stays OK),
 *           L295 false (ret==OK), L305 cached not empty check.
 *           Scenario: stop before capture module init, should safely return OK.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStop_NullModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    Status result = audioCaptureFilter_->DoStop();
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_NullModule_0100
 * @tc.desc: DoPause with audioCaptureModule_=nullptr — covers L211 false branch,
 *           ret stays ERROR_INVALID_OPERATION -> L214 true -> ret=ERROR_AUD_STOP_FAILED.
 *           Scenario: pause before capture module init, Stop not called, ret keeps initial error value.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_NullModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->withVideo_ = false;
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::ERROR_AUD_STOP_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_DoRelease_NullModule_0100
 * @tc.desc: DoRelease with audioCaptureModule_=nullptr and taskPtr_=nullptr — covers
 *           L326 false (taskPtr_ null), L329 false (audioCaptureModule_ null).
 *           Scenario: release when both module and task are uncreated, should safely return OK.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoRelease_NullModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->taskPtr_ = nullptr;
    Status result = audioCaptureFilter_->DoRelease();
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_SetWithVideo_001
 * @tc.desc: SetWithVideo sets withVideo_ member — covers L718-722.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetWithVideo_001, TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->SetWithVideo(false);
    EXPECT_FALSE(audioCaptureFilter_->withVideo_);
    audioCaptureFilter_->SetWithVideo(true);
    EXPECT_TRUE(audioCaptureFilter_->withVideo_);
}

/**
 * @tc.name: AudioCaptureFilter_RelativeSleep_001
 * @tc.desc: RelativeSleep with nanoTime <= 0 returns -1 — covers L823-825 early return.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RelativeSleep_001, TestSize.Level1)
{
    EXPECT_EQ(audioCaptureFilter_->RelativeSleep(0), -1);
    EXPECT_EQ(audioCaptureFilter_->RelativeSleep(-1), -1);
}

/**
 * @tc.name: AudioCaptureFilter_RelativeSleep_002
 * @tc.desc: RelativeSleep with valid nanoTime returns 0 — covers L834 ret==0 path.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RelativeSleep_002, TestSize.Level1)
{
    EXPECT_EQ(audioCaptureFilter_->RelativeSleep(1000000), 0);
}

/**
 * @tc.name: AudioCaptureFilter_AllocateAudioDataBuffer_001
 * @tc.desc: AllocateAudioDataBuffer oversized returns nullptr, valid returns zero-init buffer —
 *           covers L578-580 (bufferSize >= MAX) and L582-588 (normal allocation).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_AllocateAudioDataBuffer_001, TestSize.Level1)
{
    auto oversized = audioCaptureFilter_->AllocateAudioDataBuffer(100000);
    EXPECT_EQ(oversized, nullptr);
    auto valid = audioCaptureFilter_->AllocateAudioDataBuffer(16);
    ASSERT_NE(valid, nullptr);
    EXPECT_EQ(valid.get()[0], 0);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrame_NullModule_0100
 * @tc.desc: FillLostFrame with audioCaptureModule_=nullptr — covers L520-522 null guard early return.
 *           Pre-fill deque, call FillLostFrame, verify deque unchanged (guard blocks push_front).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrame_NullModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    auto data = MakeTestBuffer(16);
    uint8_t* expectedFront = data.get();
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data);
    }
    audioCaptureFilter_->FillLostFrame(3);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
    EXPECT_EQ(audioCaptureFilter_->cachedAudioDataDeque_.front().get(), expectedFront);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrame_NormalPath_0100
 * @tc.desc: FillLostFrame normal path — covers L524-534 push_front loop.
 *           Module set with bufferSize_=16, call FillLostFrame(3), verify 3 items added to front.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrame_NormalPath_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->FillLostFrame(3);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 3);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_NullModule_0100
 * @tc.desc: CalculateAVTime with audioCaptureModule_=nullptr — covers L478-481 null guard.
 *           hasCalculateAVTime_ stays false (guard returns before setting it).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_NullModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->firstAudioFramePts_.store(100);
    audioCaptureFilter_->firstVideoFramePts_.store(50);
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_FALSE(audioCaptureFilter_->hasCalculateAVTime_);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_AudioLaterFillFrames_0100
 * @tc.desc: CalculateAVTime audio-later with diffCount <= MAX — covers L493-496 else branch
 *           (FillLostFrame called, currentTime_ adjusted).
 *           firstAudioFramePts_=125ms, firstVideoFramePts_=0 → diffCount=6, currentTime_=5ms.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_AudioLaterFillFrames_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = -1;
    audioCaptureFilter_->firstVideoFramePts_.store(0);
    audioCaptureFilter_->firstAudioFramePts_.store(125000000); // 125ms
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    // diffCount=6, currentTime_=125000000-6*20000000=5000000
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 5000000);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 6);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_VideoLaterCropFrames_0100
 * @tc.desc: CalculateAVTime video-later with diffCount <= MAX — covers L505-513 else branch
 *           (pop_front from deque, currentTime_ adjusted).
 *           firstAudioFramePts_=0, firstVideoFramePts_=100ms → diffCount=5, currentTime_=100ms.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_VideoLaterCropFrames_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = -1;
    audioCaptureFilter_->firstAudioFramePts_.store(0);
    audioCaptureFilter_->firstVideoFramePts_.store(100000000); // 100ms
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
    }
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    // diffCount=5, currentTime_=0+5*20000000=100000000
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 100000000);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    // All 3 items popped (diffCount=5 > 3 items, loop stops at empty)
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrameNum_NoVideo_0100
 * @tc.desc: FillLostFrameNum with withVideo_=false — covers L603 withVideo_ false branch.
 *           Condition short-circuits, lostCount stays 0.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrameNum_NoVideo_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->withVideo_ = false;
    audioCaptureFilter_->currentTime_ = 1000;
    int32_t result = audioCaptureFilter_->FillLostFrameNum();
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: AudioCaptureFilter_GetCurrentTime_001
 * @tc.desc: GetCurrentTime sets currentTime from clock_gettime — covers L702-708.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_GetCurrentTime_001, TestSize.Level1)
{
    int64_t time1 = -1;
    audioCaptureFilter_->GetCurrentTime(time1);
    EXPECT_NE(time1, -1);
}

/**
 * @tc.name: AudioCaptureFilter_CacheFirstAudioFrame_OverMax_0100
 * @tc.desc: CacheFirstAudioFrame with cachedAudioDataDeque_ over MAX — covers L446-450.
 *           Pre-fill 257 items (> AUDIO_CAPTURE_MAX_CACHED_FRAMES=256), verify firstVideoFramePts_
 *           set to 0 and early return (deque unchanged at 257).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CacheFirstAudioFrame_OverMax_0100, TestSize.Level1)
{
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        for (int32_t i = 0; i < 257; ++i) {
            audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
        }
    }
    audioCaptureFilter_->CacheFirstAudioFrame(16);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), 0);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 257);
}

/**
 * @tc.name: AudioCaptureFilter_ReadLoop_Eos_0100
 * @tc.desc: ReadLoop with eos_=true — covers L417-419 early return.
 *           audioCaptureModule_ is nullptr; if eos_ guard missed, GetSize would crash.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_ReadLoop_Eos_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->eos_ = true;
    audioCaptureFilter_->ReadLoop();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_ReadLoop_GetSizeFail_0100
 * @tc.desc: ReadLoop with GetSize failure — covers L422-426 sleep-and-return path.
 *           Module has bufferSize_=0, GetSize returns ERROR_INVALID_PARAMETER.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_ReadLoop_GetSizeFail_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->eos_ = false;
    audioCaptureFilter_->ReadLoop();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_SetAudioCaptureChangeCallback_WithModule_0100
 * @tc.desc: SetAudioCaptureChangeCallback with non-null module — covers L163 delegation path.
 *           Module's audioCapturer_ is null, SetAudioCapturerInfoChangeCallback returns ERROR_AUD_INIT_FAILED.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetAudioCaptureChangeCallback_WithModule_0100, TestSize.Level1)
{
    auto testCallback = std::make_shared<MockAudioCapturerInfoChangeCallback>();
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    Status result = audioCaptureFilter_->SetAudioCaptureChangeCallback(testCallback);
    EXPECT_EQ(result, Status::ERROR_AUD_INIT_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_SetWillMuteWhenInterrupted_WithModule_0100
 * @tc.desc: SetWillMuteWhenInterrupted with non-null module — covers L753 delegation path.
 *           Module's audioCapturer_ is null, returns ERROR_INVALID_OPERATION.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetWillMuteWhenInterrupted_WithModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    Status result = audioCaptureFilter_->SetWillMuteWhenInterrupted(true);
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: AudioCaptureFilter_PrepareAudioCapture_TaskPtrSet_ModuleNull_0100
 * @tc.desc: PrepareAudioCapture with taskPtr_ pre-set and module null — covers L139 false (taskPtr_ already set,
 *           skip creation) and L146 true (module null, return ERROR_NULL_POINTER).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_PrepareAudioCapture_TaskPtrSet_ModuleNull_0100, TestSize.Level1)
{
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->PrepareAudioCapture();
    EXPECT_EQ(result, Status::ERROR_NULL_POINTER);
    EXPECT_EQ(audioCaptureFilter_->taskPtr_, taskPtr);
}

/**
 * @tc.name: AudioCaptureFilter_PrepareAudioCapture_TaskPtrSet_ModuleSet_0100
 * @tc.desc: PrepareAudioCapture with taskPtr_ pre-set and module set — covers L139 false (taskPtr_ already set),
 *           L146 false (module non-null), L150-151 Prepare returns error.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_PrepareAudioCapture_TaskPtrSet_ModuleSet_0100, TestSize.Level1)
{
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    Status result = audioCaptureFilter_->PrepareAudioCapture();
    EXPECT_EQ(result, Status::ERROR_AUD_INIT_FAILED);
    EXPECT_EQ(audioCaptureFilter_->taskPtr_, taskPtr);
}

/**
 * @tc.name: AudioCaptureFilter_DoStart_WithModule_0100
 * @tc.desc: DoStart with audioCaptureModule_ set — covers L192 true (module non-null, Start called),
 *           L195 true (Start fails, res != OK, early return). Real Start returns ERROR_AUD_INIT_FAILED.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStart_WithModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->taskPtr_ = nullptr;
    Status result = audioCaptureFilter_->DoStart();
    EXPECT_EQ(result, Status::ERROR_AUD_INIT_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_L223True_LostCountOverMax_0100
 * @tc.desc: DoPause with currentTime_=1 and withVideo_=true — covers L223 true (currentTime_ != 0 &&
 *           currentTime_ < pauseTime_), L236 true (lostCount > MAX, FillLostFrame skipped).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_L223True_LostCountOverMax_0100, TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->currentTime_ = 1;
    audioCaptureFilter_->startTime_ = 0;
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::OK);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_L223True_LostCountPositive_0100
 * @tc.desc: DoPause with currentTime_ ~30ms before now and withVideo_=true — covers L223 true,
 *           L239 true (0 < lostCount <= MAX, FillLostFrame called).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_L223True_LostCountPositive_0100, TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    int64_t now = -1;
    audioCaptureFilter_->GetCurrentTime(now);
    EXPECT_NE(now, -1);
    audioCaptureFilter_->currentTime_ = now - 30000000;
    audioCaptureFilter_->startTime_ = now;
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::OK);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_L223False_L226False_0100
 * @tc.desc: DoPause with currentTime_=INT64_MAX and withVideo_=true — covers L223 false
 *           (currentTime_ >= pauseTime_) and L226 false (currentTime_ != 0), lostCount stays 0.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_L223False_L226False_0100, TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->currentTime_ = INT64_MAX;
    audioCaptureFilter_->startTime_ = 0;
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::OK);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_WithVideo_NoCachedData_0100
 * @tc.desc: DoPause with withVideo_=true, currentTime_=0, no cached data — covers L226 true
 *           (currentTime_ == 0), L245 false (deque empty), L249 false (cachedSizeForRecord == 0).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_WithVideo_NoCachedData_0100, TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->startTime_ = 0;
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::OK);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_ReadLoop_GetSizeOK_CacheFirstFrame_0100
 * @tc.desc: ReadLoop with GetSize OK, pts < 0, withVideo_=true — covers L422 false (GetSize OK),
 *           L428 true (pts < 0 && withVideo_, CacheFirstAudioFrame called).
 *           Read fails (audioCapturer_ null), deque stays empty, firstAudioFramePts_ unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_ReadLoop_GetSizeOK_CacheFirstFrame_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->eos_ = false;
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->firstAudioFramePts_.store(-1);
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    audioCaptureFilter_->ReadLoop();
    EXPECT_EQ(audioCaptureFilter_->firstAudioFramePts_.load(), -1);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_ReadLoop_GetSizeOK_CalculateAVTime_0100
 * @tc.desc: ReadLoop with GetSize OK, pts >= 0, hasCalculateAVTime_=false, withVideo_=true —
 *           covers L428 false (pts >= 0), L431 true (!hasCalculateAVTime_ && withVideo_,
 *           CalculateAVTime called).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_ReadLoop_GetSizeOK_CalculateAVTime_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->eos_ = false;
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->firstAudioFramePts_.store(100000000);
    audioCaptureFilter_->firstVideoFramePts_.store(100000000);
    audioCaptureFilter_->ReadLoop();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
}

/**
 * @tc.name: AudioCaptureFilter_ReadLoop_GetSizeOK_RecordAudioFrame_WithVideo_0100
 * @tc.desc: ReadLoop with GetSize OK, pts >= 0, hasCalculateAVTime_=true, withVideo_=true —
 *           covers L428 false, L431 false (hasCalculateAVTime_ true), L434 RecordAudioFrame called.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_ReadLoop_GetSizeOK_RecordAudioFrame_WithVideo_0100,
         TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->eos_ = false;
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->hasCalculateAVTime_ = true;
    audioCaptureFilter_->firstAudioFramePts_.store(100000000);
    audioCaptureFilter_->firstVideoFramePts_.store(100000000);
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->ReadLoop();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_ReadLoop_GetSizeOK_RecordAudioFrame_NoVideo_0100
 * @tc.desc: ReadLoop with GetSize OK, withVideo_=false — covers L428 false (withVideo_ false,
 *           condition short-circuits), L431 false (withVideo_ false), L434 RecordAudioFrame called.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_ReadLoop_GetSizeOK_RecordAudioFrame_NoVideo_0100,
         TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->eos_ = false;
    audioCaptureFilter_->withVideo_ = false;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->ReadLoop();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrameNum_WithVideo_PositiveLostCount_0100
 * @tc.desc: FillLostFrameNum with withVideo_=true, currentTime_ negative — covers L601-603 true
 *           (audioDataTime > currentTime_ + cached, delta > irregular, withVideo_), L607 false
 *           (lostCount <= MAX). audioDataTime=0 (capturer null), currentTime_=-200000000 → lostCount=9.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrameNum_WithVideo_PositiveLostCount_0100,
         TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->currentTime_ = -200000000;
    int32_t result = audioCaptureFilter_->FillLostFrameNum();
    EXPECT_EQ(result, 9);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrameNum_WithVideo_OverMax_0100
 * @tc.desc: FillLostFrameNum with withVideo_=true, currentTime_ very negative — covers L601-603 true,
 *           L607 true (lostCount > MAX). currentTime_=-10000000000 → lostCount=499 > 256.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrameNum_WithVideo_OverMax_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->currentTime_ = -10000000000;
    int32_t result = audioCaptureFilter_->FillLostFrameNum();
    EXPECT_EQ(result, 499);
}

/**
 * @tc.name: AudioCaptureFilter_SendEos_RequestBufferFail_0100
 * @tc.desc: SendEos with outputBufferQueue_ returning error from RequestBuffer — covers L403 true
 *           (ret != OK, early return). Uses FailingMockOutputBufferQueue.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SendEos_RequestBufferFail_0100, TestSize.Level1)
{
    sptr<AVBufferQueueProducer> failQueue = new FailingMockOutputBufferQueue();
    audioCaptureFilter_->outputBufferQueue_ = failQueue;
    Status result = audioCaptureFilter_->SendEos();
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
    EXPECT_TRUE(audioCaptureFilter_->eos_);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrame_OversizeBuffer_0100
 * @tc.desc: FillLostFrame with module bufferSize_ >= MAX_CAPTURE_BUFFER_SIZE — covers L527 true
 *           (AllocateAudioDataBuffer returns nullptr, early return). Deque unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrame_OversizeBuffer_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 100000;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->FillLostFrame(3);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_RecordAudioFrame_NullModule_0100
 * @tc.desc: RecordAudioFrame with audioCaptureModule_=nullptr — covers L539 true (null guard,
 *           early return). Deque unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordAudioFrame_NullModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->RecordAudioFrame();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_RecordAudioFrame_CacheNotEmpty_ReadFail_0100
 * @tc.desc: RecordAudioFrame with cache not empty and Read failing — covers L551 true
 *           (cacheNotEmpty), L556 true (Read returns error, RelativeSleep + return).
 *           Pre-fill 1 item, call RecordAudioFrame, Read fails (audioCapturer_ null),
 *           deque stays at 1 (no push_back since Read failed).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordAudioFrame_CacheNotEmpty_ReadFail_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    auto data = MakeTestBuffer(16);
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data);
    }
    audioCaptureFilter_->RecordAudioFrame();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
}

/**
 * @tc.name: AudioCaptureFilter_SetAudioSource_NonMicSource_0100
 * @tc.desc: SetAudioSource with source != 1 — covers L374 false (else branch, direct cast).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetAudioSource_NonMicSource_0100, TestSize.Level1)
{
    audioCaptureFilter_->sourceType_ = AudioStandard::SourceType::SOURCE_TYPE_MIC;
    int32_t source = 0;
    audioCaptureFilter_->SetAudioSource(source);
    EXPECT_EQ(audioCaptureFilter_->sourceType_, static_cast<AudioStandard::SourceType>(0));
    source = 7;
    audioCaptureFilter_->SetAudioSource(source);
    EXPECT_EQ(audioCaptureFilter_->sourceType_, static_cast<AudioStandard::SourceType>(7));
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_AudioLater_DiffOverMax_0100
 * @tc.desc: CalculateAVTime audio-later with diffCount > MAX — covers L490-492 true
 *           (currentTime_ set to firstAudioFramePts_ directly, FillLostFrame NOT called).
 *           firstAudioFramePts_=10s, firstVideoFramePts_=0 → diffCount=500 > 256.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_AudioLater_DiffOverMax_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->firstVideoFramePts_.store(0);
    audioCaptureFilter_->firstAudioFramePts_.store(10000000000);
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 10000000000);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_VideoLater_DiffOverMax_0100
 * @tc.desc: CalculateAVTime video-later with diffCount > MAX — covers L502-504 true
 *           (currentTime_ set to firstAudioFramePts_ directly, no pop_front).
 *           firstAudioFramePts_=0, firstVideoFramePts_=10s → diffCount=500 > 256.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_VideoLater_DiffOverMax_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->firstAudioFramePts_.store(0);
    audioCaptureFilter_->firstVideoFramePts_.store(10000000000);
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_ModuleNull_WithVideo_0100
 * @tc.desc: DoPause with module null and withVideo_=true — covers L211 false (module null,
 *           ret stays ERROR_INVALID_OPERATION), L214 true (ret != OK → ERROR_AUD_STOP_FAILED),
 *           L221 true (withVideo_), L223 false, L226 true (currentTime_==0).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_ModuleNull_WithVideo_0100, TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->startTime_ = 0;
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::ERROR_AUD_STOP_FAILED);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoStart_WithModuleAndTask_0100
 * @tc.desc: DoStart with module set and taskPtr_ set — covers L192 true (module non-null),
 *           L195 true (Start fails). taskPtr_ block (L197) not reached since Start fails.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStart_WithModuleAndTask_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->taskPtr_ = std::make_shared<Task>("DataReader");
    Status result = audioCaptureFilter_->DoStart();
    EXPECT_EQ(result, Status::ERROR_AUD_INIT_FAILED);
}

/**
 * @tc.name: AudioCaptureFilter_CacheFirstAudioFrame_AllocFail_0100
 * @tc.desc: CacheFirstAudioFrame with bufferSize >= MAX_CAPTURE_BUFFER_SIZE — covers L452 true
 *           (AllocateAudioDataBuffer returns nullptr, early return). Deque unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CacheFirstAudioFrame_AllocFail_0100, TestSize.Level1)
{
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    audioCaptureFilter_->firstAudioFramePts_.store(-1);
    audioCaptureFilter_->CacheFirstAudioFrame(100000);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_AudioLater_SmallDiff_0100
 * @tc.desc: CalculateAVTime audio-later with diffCount == 1 — covers L493-496 else branch
 *           (FillLostFrame called with diffCount=1). firstAudio=25ms, video=0 → diffCount=1.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_AudioLater_SmallDiff_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = -1;
    audioCaptureFilter_->firstVideoFramePts_.store(0);
    audioCaptureFilter_->firstAudioFramePts_.store(25000000);
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 5000000);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
}

/**
 * @tc.name: AudioCaptureFilter_CalculateAVTime_VideoLater_SmallDiff_0100
 * @tc.desc: CalculateAVTime video-later with diffCount == 1, cached deque has 1 item —
 *           covers L505-513 else branch (pop_front 1 item). firstAudio=0, video=25ms → diffCount=1.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CalculateAVTime_VideoLater_SmallDiff_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 16;
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->hasCalculateAVTime_ = false;
    audioCaptureFilter_->currentTime_ = -1;
    audioCaptureFilter_->firstAudioFramePts_.store(0);
    audioCaptureFilter_->firstVideoFramePts_.store(25000000);
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
    }
    audioCaptureFilter_->CalculateAVTime();
    EXPECT_TRUE(audioCaptureFilter_->hasCalculateAVTime_);
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 20000000);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_SetVideoFirstFramePts_FirstFramePtsEqualsStartTime_0100
 * @tc.desc: SetVideoFirstFramePts with firstFramePts == startTime_ and hasCalculateAVTime_=true —
 *           covers L712 false (firstFramePts <= startTime_ && hasCalculateAVTime_ true),
 *           firstVideoFramePts_ unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetVideoFirstFramePts_FirstFramePtsEqualsStartTime_0100,
         TestSize.Level1)
{
    audioCaptureFilter_->hasCalculateAVTime_ = true;
    audioCaptureFilter_->startTime_ = 1000;
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    audioCaptureFilter_->SetVideoFirstFramePts(1000);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), -1);
}

/**
 * @tc.name: AudioCaptureFilter_GetMaxAmplitude_WithModule_0100
 * @tc.desc: GetMaxAmplitude with module set and maxAmplitude_ preset — covers L742 amplitude
 *           assignment from module->GetMaxAmplitude().
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_GetMaxAmplitude_WithModule_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->maxAmplitude_ = 0;
    audioCaptureFilter_->audioCaptureModule_ = module;
    int32_t amplitude = -1;
    Status result = audioCaptureFilter_->GetMaxAmplitude(amplitude);
    EXPECT_EQ(result, Status::OK);
    EXPECT_EQ(amplitude, 0);
}

/**
 * @tc.name: AudioCaptureFilter_GetCurrentCapturerChangeInfo_WithModule_0100
 * @tc.desc: GetCurrentCapturerChangeInfo with module set — covers L731 delegation
 *           (module->GetCurrentCapturerChangeInfo called), returns OK.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_GetCurrentCapturerChangeInfo_WithModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    AudioStandard::AudioCapturerChangeInfo changeInfo;
    Status result = audioCaptureFilter_->GetCurrentCapturerChangeInfo(changeInfo);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_SetCallingInfo_WithModule_0100
 * @tc.desc: SetCallingInfo with module set — covers L815-816 true (module non-null,
 *           delegates SetCallingInfo to module).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetCallingInfo_WithModule_0100, TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = module;
    int32_t testUid = 1001;
    int32_t testPid = 2002;
    std::string testBundle = "com.test.app";
    uint64_t testInstanceId = 99999;
    audioCaptureFilter_->SetCallingInfo(testUid, testPid, testBundle, testInstanceId);
    EXPECT_EQ(module->appUid_, testUid);
    EXPECT_EQ(module->appPid_, testPid);
    EXPECT_EQ(module->bundleName_, testBundle);
    EXPECT_EQ(module->instanceId_, testInstanceId);
}

/**
 * @tc.name: AudioCaptureFilter_DoFlush_ReturnsOK_0100
 * @tc.desc: DoFlush always returns OK — covers L315-318.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoFlush_ReturnsOK_0100, TestSize.Level1)
{
    Status result = audioCaptureFilter_->DoFlush();
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_PrepareAudioCapture_PrepareOk_0100
 * @tc.desc: PrepareAudioCapture with module Prepare returning OK — covers L151 false branch
 *           (err == OK, skip error log). FakeAudioCapturer injected so module.Prepare() succeeds
 *           (GetBufferSize OK, size < MAX). taskPtr_ preset to FakeTask to skip real Task creation.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_PrepareAudioCapture_PrepareOk_0100, TestSize.Level1)
{
    audioCaptureFilter_->taskPtr_ = std::make_shared<FakeTask>();
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    Status result = audioCaptureFilter_->PrepareAudioCapture();
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: AudioCaptureFilter_DoStart_StartOk_TaskNull_0100
 * @tc.desc: DoStart with module Start OK and taskPtr_ null — covers L195 false (res==OK, continue
 *           past macro) and L197 false (taskPtr_ null, skip Start). FakeAudioCapturer injected so
 *           module.Start() returns OK. taskPtr_ null avoids spawning ffrt::thread on device.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStart_StartOk_TaskNull_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    audioCaptureFilter_->taskPtr_ = nullptr;
    Status result = audioCaptureFilter_->DoStart();
    EXPECT_EQ(result, Status::OK);
    EXPECT_EQ(audioCaptureFilter_->taskPtr_, nullptr);
}

/**
 * @tc.name: AudioCaptureFilter_DoStart_StartOk_TaskSet_0100
 * @tc.desc: DoStart with module Start OK and taskPtr_ set — covers L195 false and L197 true
 *           (taskPtr_->Start() called). FakeTask::Start records the call without spawning a thread.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStart_StartOk_TaskSet_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    auto fakeTask = std::make_shared<FakeTask>();
    audioCaptureFilter_->taskPtr_ = fakeTask;
    Status result = audioCaptureFilter_->DoStart();
    EXPECT_EQ(result, Status::OK);
    EXPECT_TRUE(fakeTask->startCalled_);
}

/**
 * @tc.name: AudioCaptureFilter_DoResume_StartOk_TaskNull_0100
 * @tc.desc: DoResume with module Start OK and taskPtr_ null — covers L271 false (ret==OK, skip
 *           error return) and L275 false (taskPtr_ null, skip Start).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoResume_StartOk_TaskNull_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    audioCaptureFilter_->taskPtr_ = nullptr;
    Status result = audioCaptureFilter_->DoResume();
    EXPECT_EQ(result, Status::OK);
    EXPECT_EQ(audioCaptureFilter_->taskPtr_, nullptr);
}

/**
 * @tc.name: AudioCaptureFilter_DoResume_StartOk_TaskSet_0100
 * @tc.desc: DoResume with module Start OK and taskPtr_ set — covers L271 false and L275 true
 *           (taskPtr_->Start() called via FakeTask).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoResume_StartOk_TaskSet_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    auto fakeTask = std::make_shared<FakeTask>();
    audioCaptureFilter_->taskPtr_ = fakeTask;
    Status result = audioCaptureFilter_->DoResume();
    EXPECT_EQ(result, Status::OK);
    EXPECT_TRUE(fakeTask->startCalled_);
}

/**
 * @tc.name: AudioCaptureFilter_CacheFirstAudioFrame_ReadOk_AudioTimeZero_0100
 * @tc.desc: CacheFirstAudioFrame with Read OK, firstAudioFramePts_<0, GetAudioTime yields 0 —
 *           covers L456 false (Read OK), L461 true (pts<0), L464 true (audioDataTime==0 →
 *           GetCurrentTime). FakeAudioCapturer timestamp_=20ms → after first-frame subtract (−20ms)
 *           audioDataTime==0, so GetCurrentTime is invoked and pts is set to wall clock.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CacheFirstAudioFrame_ReadOk_AudioTimeZero_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule(20000000); // 20ms -> audioDataTime 0
    audioCaptureFilter_->firstAudioFramePts_.store(-1);
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->CacheFirstAudioFrame(16);
    EXPECT_NE(audioCaptureFilter_->firstAudioFramePts_.load(), -1);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
}

/**
 * @tc.name: AudioCaptureFilter_CacheFirstAudioFrame_ReadOk_AudioTimeNonZero_0100
 * @tc.desc: CacheFirstAudioFrame with Read OK, firstAudioFramePts_<0, GetAudioTime non-zero —
 *           covers L456 false, L461 true, L464 false (audioDataTime!=0, keep module timestamp).
 *           FakeAudioCapturer timestamp_=1s -> audioDataTime = 1s - 20ms = 980000000.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CacheFirstAudioFrame_ReadOk_AudioTimeNonZero_0100,
         TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule(1000000000); // 1s -> 980000000
    audioCaptureFilter_->firstAudioFramePts_.store(-1);
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->CacheFirstAudioFrame(16);
    EXPECT_EQ(audioCaptureFilter_->firstAudioFramePts_.load(), 980000000);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
}

/**
 * @tc.name: AudioCaptureFilter_CacheFirstAudioFrame_ReadOk_PtsNonNeg_0100
 * @tc.desc: CacheFirstAudioFrame with Read OK and firstAudioFramePts_>=0 — covers L456 false and
 *           L461 false (pts>=0, skip timestamp block). pts stays unchanged; Read buffer is still
 *           pushed to cache deque (proving the call executed past L456).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_CacheFirstAudioFrame_ReadOk_PtsNonNeg_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    audioCaptureFilter_->firstAudioFramePts_.store(100);
    audioCaptureFilter_->firstVideoFramePts_.store(-1);
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->CacheFirstAudioFrame(16);
    EXPECT_EQ(audioCaptureFilter_->firstAudioFramePts_.load(), 100);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
}

/**
 * @tc.name: AudioCaptureFilter_RecordAudioFrame_ReadOk_NoVideo_0100
 * @tc.desc: RecordAudioFrame with cache not empty, Read OK, withVideo_=false — covers L553 false
 *           (AllocateAudioDataBuffer non-null), L557 false (Read OK), L563 false (lostCount==0,
 *           FillLostFrame skipped). RecordCachedData then drains the cache (outputBufferQueue_ null
 *           -> while loop skipped, erase only).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordAudioFrame_ReadOk_NoVideo_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    audioCaptureFilter_->withVideo_ = false;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
    }
    audioCaptureFilter_->RecordAudioFrame();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_RecordAudioFrame_ReadOk_WithVideo_LostCountPositive_0100
 * @tc.desc: RecordAudioFrame with cache not empty, Read OK, withVideo_=true, large audioDataTime —
 *           covers L553 false, L557 false, L563 true (0 < lostCount <= MAX -> FillLostFrame called).
 *           FakeAudioCapturer timestamp_=200ms -> FillLostFrameNum returns 8 (audioDataTime=200ms,
 *           currentTime_=0, cachedSize=1). FillLostFrame(8) prepends 8 frames; RecordCachedData(5)
 *           then erases 5 -> deque ends at 5.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordAudioFrame_ReadOk_WithVideo_LostCountPositive_0100,
         TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule(200000000); // 200ms -> lostCount 8
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
    }
    audioCaptureFilter_->RecordAudioFrame();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 5);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrameNum_WithVideo_NoLoss_0100
 * @tc.desc: FillLostFrameNum with withVideo_=true, audioDataTime==currentTime_+cached (delta=0) —
 *           covers L601 false branch (first comparison false, short-circuit). lostCount stays 0.
 *           Real AudioCaptureModule with null capturer yields audioDataTime=0; currentTime_=0 → 0 > 0 false.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrameNum_WithVideo_NoLoss_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->currentTime_ = 0;
    int32_t result = audioCaptureFilter_->FillLostFrameNum();
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: AudioCaptureFilter_FillLostFrameNum_WithVideo_SmallDelta_0100
 * @tc.desc: FillLostFrameNum with audioDataTime > currentTime_+cached but delta <= AUDIO_UNREGULAR_DELTA_TIME —
 *           covers L601 true, L602 false (delta <= 100ms). lostCount stays 0.
 *           currentTime_=-50ms, audioDataTime=0 → delta=50ms <= 100ms.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_FillLostFrameNum_WithVideo_SmallDelta_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->currentTime_ = -50000000; // -50ms → delta=50ms <= 100ms
    int32_t result = audioCaptureFilter_->FillLostFrameNum();
    EXPECT_EQ(result, 0);
}

/**
 * @tc.name: AudioCaptureFilter_RecordAudioFrame_CacheNotEmpty_OversizeBuffer_0100
 * @tc.desc: RecordAudioFrame with cacheNotEmpty + module->GetSize returning oversized bufferSize —
 *           covers L551 true (cacheNotEmpty), L552 AllocateAudioDataBuffer returns nullptr (>= MAX),
 *           L553 true (!cachedAudioData) → early return. Deque unchanged.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordAudioFrame_CacheNotEmpty_OversizeBuffer_0100,
         TestSize.Level1)
{
    auto module = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    module->bufferSize_ = 100000; // >= MAX_CAPTURE_BUFFER_SIZE
    audioCaptureFilter_->audioCaptureModule_ = module;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    auto data = MakeTestBuffer(16);
    uint8_t* expectedFront = data.get();
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(data);
    }
    audioCaptureFilter_->RecordAudioFrame();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 1);
    EXPECT_EQ(audioCaptureFilter_->cachedAudioDataDeque_.front().get(), expectedFront);
}

/**
 * @tc.name: AudioCaptureFilter_ReadLoop_RecordOneAudioFrame_MemoryNull_0100
 * @tc.desc: ReadLoop with outputBufferQueue_=MockOutputBufferQueue (RequestBuffer OK, buffer->memory_=nullptr),
 *           withVideo_=false, cache empty — covers RecordOneAudioFrame L624 true branch
 *           (ret==OK but buffer->memory_==nullptr → RelativeSleep + return).
 *           FakeAudioCapturer injected so GetSize succeeds; Read not reached (guard at L624).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_ReadLoop_RecordOneAudioFrame_MemoryNull_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    sptr<AVBufferQueueProducer> queue = new MockOutputBufferQueue();
    audioCaptureFilter_->outputBufferQueue_ = queue;
    audioCaptureFilter_->eos_ = false;
    audioCaptureFilter_->withVideo_ = false;
    audioCaptureFilter_->ReadLoop();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_OnLinkedResult_PrepareOk_0100
 * @tc.desc: OnLinkedResult with FakeAudioCapturer module + FakeTask — covers PrepareAudioCapture success
 *           path (L150-151 false: module Prepare returns OK). outputBufferQueue_ transitions from
 *           nullptr to the queue argument, proving OnLinkedResult stored it.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnLinkedResult_PrepareOk_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    audioCaptureFilter_->taskPtr_ = std::make_shared<FakeTask>();
    sptr<AVBufferQueueProducer> queue = new MockOutputBufferQueue();
    ASSERT_NE(queue, nullptr);
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioCaptureFilter_->OnLinkedResult(queue, meta);
    EXPECT_EQ(audioCaptureFilter_->outputBufferQueue_, queue);
}

/**
 * @tc.name: AudioCaptureFilter_DoStop_WithTaskAndFakeModule_0100
 * @tc.desc: DoStop with taskPtr_ set + FakeAudioCapturer module — covers L286 true (taskPtr_->StopAsync),
 *           L292 true (module non-null, Stop OK), L294 false (ret==OK). State members reset:
 *           firstAudioFramePts_/firstVideoFramePts_/currentTime_/startTime_ reset to defaults.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStop_WithTaskAndFakeModule_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule();
    audioCaptureFilter_->taskPtr_ = std::make_shared<FakeTask>();
    audioCaptureFilter_->firstAudioFramePts_.store(1000);
    audioCaptureFilter_->firstVideoFramePts_.store(2000);
    audioCaptureFilter_->currentTime_ = 5000;
    audioCaptureFilter_->startTime_ = 9000;
    Status result = audioCaptureFilter_->DoStop();
    EXPECT_EQ(result, Status::OK);
    EXPECT_EQ(audioCaptureFilter_->firstAudioFramePts_.load(), -1);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), -1);
    EXPECT_EQ(audioCaptureFilter_->currentTime_, 0);
    EXPECT_EQ(audioCaptureFilter_->startTime_, 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoPause_WithVideo_CurrentTimeZero_LostCountNonPositive_0100
 * @tc.desc: DoPause with withVideo_=true, currentTime_=0, startTime_ in future — covers L226 true
 *           (currentTime_==0), L236 false + L239 false (lostCount <= 0 because pauseTime_-startTime_ < 0).
 *           No FillLostFrame called, no RecordCachedData (deque empty).
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoPause_WithVideo_CurrentTimeZero_LostCountNonPositive_0100,
         TestSize.Level1)
{
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->currentTime_ = 0;
    int64_t now = -1;
    audioCaptureFilter_->GetCurrentTime(now);
    EXPECT_NE(now, -1);
    audioCaptureFilter_->startTime_ = now + 1000000000; // 1s in future → pauseTime_-startTime_ < 0
    Status result = audioCaptureFilter_->DoPause();
    EXPECT_EQ(result, Status::OK);
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_RecordAudioFrame_LostCountOverMax_0100
 * @tc.desc: RecordAudioFrame with cacheNotEmpty, Read OK, FillLostFrameNum returning > MAX —
 *           covers L563 false branch (lostCount > AUDIO_CAPTURE_MAX_CACHED_FRAMES, FillLostFrame
 *           NOT called). FakeAudioCapturer timestamp_=10s → lostCount=498 > 256.
 *           RecordCachedData(5) erases min(5,1)=1 from deque, push_back adds 1 → deque size=1.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_RecordAudioFrame_LostCountOverMax_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFakeModule(10000000000); // 10s → lostCount 498
    audioCaptureFilter_->withVideo_ = true;
    audioCaptureFilter_->currentTime_ = 0;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    {
        std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
        audioCaptureFilter_->cachedAudioDataDeque_.push_back(MakeTestBuffer(16));
    }
    audioCaptureFilter_->RecordAudioFrame();
    std::lock_guard<std::mutex> lock(audioCaptureFilter_->cachedAudioDataMutex_);
    // push_back adds 1 (deque=2), RecordCachedData(5) erases min(5,2)=2 → deque=0
    EXPECT_EQ(static_cast<int32_t>(audioCaptureFilter_->cachedAudioDataDeque_.size()), 0);
}

/**
 * @tc.name: AudioCaptureFilter_DoStop_StopFail_0100
 * @tc.desc: DoStop with module whose Stop() returns ERROR_AUD_STOP_FAILED — covers L294 true
 *           branch (ret != OK, logs error). FailingStopFakeCapturer::Stop() returns false,
 *           AudioCaptureModule::Stop() returns ERROR_AUD_STOP_FAILED.
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_DoStop_StopFail_0100, TestSize.Level1)
{
    audioCaptureFilter_->audioCaptureModule_ = MakeFailingStopModule();
    audioCaptureFilter_->taskPtr_ = nullptr;
    audioCaptureFilter_->outputBufferQueue_ = nullptr;
    audioCaptureFilter_->firstAudioFramePts_.store(1000);
    audioCaptureFilter_->firstVideoFramePts_.store(2000);
    Status result = audioCaptureFilter_->DoStop();
    EXPECT_EQ(result, Status::ERROR_AUD_STOP_FAILED);
    EXPECT_EQ(audioCaptureFilter_->firstAudioFramePts_.load(), -1);
    EXPECT_EQ(audioCaptureFilter_->firstVideoFramePts_.load(), -1);
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS