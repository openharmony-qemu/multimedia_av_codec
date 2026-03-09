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

#include "audio_capturer.h"
#include "audio_capture_filter.h"
#include "audio_capture_filter_unit_test.h"
#include "avbuffer_queue.h"
#include "audio_capture_module.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

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
    EXPECT_EQ(result, Status::ERROR_WRONG_STATE);
    audioCaptureFilter_->taskPtr_ = nullptr;
    auto mockAudioCaptureModule = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = mockAudioCaptureModule;
    result = audioCaptureFilter_->PrepareAudioCapture();
    EXPECT_EQ(result, Status::ERROR_WRONG_STATE);
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
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
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
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
    auto taskPtr = std::make_shared<Task>("DataReader");
    audioCaptureFilter_->taskPtr_ = taskPtr;
    auto mockAudioCaptureModule = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->audioCaptureModule_ = mockAudioCaptureModule;
    result = audioCaptureFilter_->DoResume();
    EXPECT_EQ(result, Status::ERROR_WRONG_STATE);
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
    audioCaptureFilter_->audioCaptureModule_ = std::make_shared<MockAudioCaptureModule>();
    audioCaptureFilter_->GetParameter(meta);
    EXPECT_NE(meta, nullptr);
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
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioCaptureFilter_->OnUnlinkedResult(meta);
    EXPECT_NE(audioCaptureFilter_, nullptr);
    SUCCEED();
}

/**
 * @tc.name: AudioCaptureFilter_OnUpdatedResult_001
 * @tc.desc: OnUpdatedResult
 * @tc.type: FUNC
 */
 HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_OnUpdatedResult_001, TestSize.Level1)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    audioCaptureFilter_->OnUpdatedResult(meta);
    EXPECT_NE(audioCaptureFilter_, nullptr);
    SUCCEED();
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
HWTEST_F(AudioCaptureFilterUnitTest, AudioCaptureFilter_SetWillMuteWhenInterrupted_001, TestSize.Level1)
{
    ASSERT_NE(audioCaptureFilter_, nullptr);
    audioCaptureFilter_->audioCaptureModule_ = nullptr;
    Status result = audioCaptureFilter_->SetWillMuteWhenInterrupted(true);
    EXPECT_EQ(result, Status::ERROR_INVALID_OPERATION);
}

}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS