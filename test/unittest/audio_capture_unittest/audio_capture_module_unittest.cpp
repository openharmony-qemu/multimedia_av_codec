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

#include "audio_capture_module_unittest.h"
#include "buffer/avbuffer_common.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::AudioCaptureModule;
using namespace testing;
using namespace testing::ext;

const static int32_t CONFIG_SIZE = 1024;
const static int32_t CONFIG_CAPACITY = 1024;
const static int32_t READ_LEN = 0;
const static int32_t TIMES_TWO = 2;
const static int32_t EXPECTED_LEN = 1024;
const static int32_t STATUS_OK = 0;
const static int32_t STATUS_UNKNOWN = -1;

namespace OHOS {
namespace Media {
void AudioCaptureModuleUnitTest::SetUpTestCase(void)
{
}

void AudioCaptureModuleUnitTest::TearDownTestCase(void)
{
}

void AudioCaptureModuleUnitTest::SetUp(void)
{
    mockAudioCapturer_ = std::make_unique<MockAudioCapturer>();
    audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
}

void AudioCaptureModuleUnitTest::TearDown(void)
{
    mockAudioCapturer_ = nullptr;
    audioCaptureModule_ = nullptr;
}

void AudioCaptureModuleUnitTest::InitMeta(std::shared_ptr<Meta> meta)
{
    int32_t appTokenId = 0;
    int32_t appUid = 0;
    int32_t appPid = 0;
    int64_t appFullTokenId = 0;
    int32_t sampleRate = 48000;
    int32_t channel = 1;
    int64_t bitRate = 48000;

    meta->Set<Tag::APP_TOKEN_ID>(appTokenId);
    meta->Set<Tag::APP_UID>(appUid);
    meta->Set<Tag::APP_PID>(appPid);
    meta->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channel);
    meta->Set<Tag::MEDIA_BITRATE>(bitRate);
}

/**
 * @tc.name  : Test DoDeinit
 * @tc.number: DoDeinit_001
 * @tc.desc  : Test DoDeinit audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING
 */
HWTEST_F(AudioCaptureModuleUnitTest, DoDeinit_001, TestSize.Level1)
{
    EXPECT_CALL(*mockAudioCapturer_, GetStatus())
        .WillOnce(Return(AudioStandard::CapturerState::CAPTURER_RUNNING));
    EXPECT_CALL(*mockAudioCapturer_, Stop())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockAudioCapturer_, Release())
        .WillOnce(Return(true));
    EXPECT_CALL(*mockAudioCapturer_, RemoveAudioCapturerInfoChangeCallback(_))
        .WillOnce(Return(true));
    audioCaptureModule_->audioCapturer_ = std::move(mockAudioCapturer_);
    EXPECT_EQ(audioCaptureModule_->DoDeinit(), Status::OK);
}

/**
 * @tc.name  : Test Reset
 * @tc.number: Reset_001
 * @tc.desc  : Test Reset audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING
 */
HWTEST_F(AudioCaptureModuleUnitTest, Reset_001, TestSize.Level1)
{
    EXPECT_CALL(*mockAudioCapturer_, GetStatus())
        .WillOnce(Return(AudioStandard::CapturerState::CAPTURER_RUNNING));
    EXPECT_CALL(*mockAudioCapturer_, Stop())
        .WillOnce(Return(true));
    audioCaptureModule_->audioCapturer_ = std::move(mockAudioCapturer_);
    audioCaptureModule_->Reset();
    audioCaptureModule_->audioCapturer_.reset();
}

/**
 * @tc.name  : Test Start
 * @tc.number: Start_001
 * @tc.desc  : Test Start audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING
 */
HWTEST_F(AudioCaptureModuleUnitTest, Start_001, TestSize.Level1)
{
    EXPECT_CALL(*mockAudioCapturer_, GetStatus())
        .WillOnce(Return(AudioStandard::CapturerState::CAPTURER_RUNNING));
    audioCaptureModule_->audioCapturer_ = std::move(mockAudioCapturer_);
    audioCaptureModule_->Start();
    audioCaptureModule_->audioCapturer_.reset();
}

/**
 * @tc.name  : Test Start
 * @tc.number: Start_002
 * @tc.desc  : Test Start audioCapturer_->Start() == false
 */
HWTEST_F(AudioCaptureModuleUnitTest, Start_002, TestSize.Level1)
{
    EXPECT_CALL(*mockAudioCapturer_, GetStatus())
        .WillOnce(Return(AudioStandard::CapturerState::CAPTURER_INVALID));
    EXPECT_CALL(*mockAudioCapturer_, Start())
        .WillOnce(Return(false));
    audioCaptureModule_->audioCapturer_ = std::move(mockAudioCapturer_);
    audioCaptureModule_->Start();
    audioCaptureModule_->audioCapturer_.reset();
}

/**
 * @tc.name  : Test Stop
 * @tc.number: Stop_001
 * @tc.desc  : Test Stop audioCapturer_->Stop() == false
 */
HWTEST_F(AudioCaptureModuleUnitTest, Stop_001, TestSize.Level1)
{
    EXPECT_CALL(*mockAudioCapturer_, GetStatus())
        .WillOnce(Return(AudioStandard::CapturerState::CAPTURER_RUNNING));
    EXPECT_CALL(*mockAudioCapturer_, Stop())
        .WillOnce(Return(false));
    audioCaptureModule_->audioCapturer_ = std::move(mockAudioCapturer_);
    audioCaptureModule_->Stop();
    audioCaptureModule_->audioCapturer_.reset();
}

/**
 * @tc.name  : Test Read
 * @tc.number: Read_001
 * @tc.desc  : Test Read audioCapturer_ == nullptr
 */
HWTEST_F(AudioCaptureModuleUnitTest, Read_001, TestSize.Level1)
{
    uint8_t *ptr = new uint8_t[EXPECTED_LEN];
    int32_t capacity = CONFIG_CAPACITY;
    int32_t size = CONFIG_SIZE;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(ptr, capacity, size);
    size_t expectedLen = EXPECTED_LEN;
    audioCaptureModule_->audioCapturer_ = nullptr;
    EXPECT_EQ(audioCaptureModule_->Read(buffer, expectedLen), Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name  : Test Read
 * @tc.number: Read_002
 * @tc.desc  : Test Read isTrackMaxAmplitude == true
 */
HWTEST_F(AudioCaptureModuleUnitTest, Read_002, TestSize.Level1)
{
    EXPECT_CALL(*mockAudioCapturer_, GetStatus())
        .WillOnce(Return(AudioStandard::CapturerState::CAPTURER_RUNNING));
    EXPECT_CALL(*mockAudioCapturer_, Read(_, _, _))
        .WillOnce(Return(READ_LEN));
    audioCaptureModule_->isTrackMaxAmplitude = true;
    audioCaptureModule_->audioCapturer_ = std::move(mockAudioCapturer_);
    uint8_t *cacheAudioData = new uint8_t[EXPECTED_LEN];
    size_t expectedLen = EXPECTED_LEN;
    audioCaptureModule_->Read(cacheAudioData, expectedLen);
    audioCaptureModule_->audioCapturer_.reset();
}

/**
 * @tc.name  : Test SetWillMuteWhenInterrupted
 * @tc.number: SetWillMuteWhenInterrupted_001
 * @tc.desc  : Test SetWillMuteWhenInterrupted audioCapturer_->SetInterruptStrategy
 */
HWTEST_F(AudioCaptureModuleUnitTest, SetWillMuteWhenInterrupted_001, TestSize.Level1)
{
    EXPECT_CALL(*mockAudioCapturer_, SetInterruptStrategy(_))
        .Times(TIMES_TWO)
        .WillOnce(Return(STATUS_OK))
        .WillOnce(Return(STATUS_UNKNOWN));
    bool muteWhenInterrupted = true;
    audioCaptureModule_->audioCapturer_ = std::move(mockAudioCapturer_);
    EXPECT_EQ(audioCaptureModule_->SetWillMuteWhenInterrupted(muteWhenInterrupted), Status::OK);
    EXPECT_EQ(audioCaptureModule_->SetWillMuteWhenInterrupted(muteWhenInterrupted), Status::ERROR_UNKNOWN);
    audioCaptureModule_->audioCapturer_.reset();
}

/**
 * @tc.name  : Test OnInterrupt
 * @tc.number: OnInterrupt_001
 * @tc.desc  : Test OnInterrupt interruptEvent.hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_MUTE
 *             Test OnInterrupt interruptEvent.hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_UNMUTE
 *             Test OnInterrupt interruptEvent.hintType != AudioStandard::InterruptHint::INTERRUPT_HINT_NONE
 *             && interruptEvent.hintType != AudioStandard::InterruptHint::INTERRUPT_HINT_UNMUTE
 */
HWTEST_F(AudioCaptureModuleUnitTest, OnInterrupt_001, TestSize.Level1)
{
    auto audioCaptureParameter_ = std::make_shared<Meta>();
    InitMeta(audioCaptureParameter_);
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status status = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    status = audioCaptureModule_->Init();

    // Test OnInterrupt interruptEvent.hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_MUTE
    AudioStandard::InterruptEvent interruptEvent = {};
    interruptEvent.hintType = AudioStandard::InterruptHint::INTERRUPT_HINT_MUTE;
    audioCaptureModule_->audioInterruptCallback_->OnInterrupt(interruptEvent);

    // Test OnInterrupt interruptEvent.hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_UNMUTE
    interruptEvent.hintType = AudioStandard::InterruptHint::INTERRUPT_HINT_UNMUTE;
    audioCaptureModule_->audioInterruptCallback_->OnInterrupt(interruptEvent);

    // Test OnInterrupt interruptEvent.hintType == AudioStandard::InterruptHint::INTERRUPT_HINT_NONE
    interruptEvent.hintType = AudioStandard::InterruptHint::INTERRUPT_HINT_NONE;
    audioCaptureModule_->audioInterruptCallback_->OnInterrupt(interruptEvent);

    EXPECT_EQ(status, Status::OK);
}

/**
 * @tc.name  : Test OnInterrupt
 * @tc.number: OnInterrupt_002
 * @tc.desc  : Test OnInterrupt audioCaptureModuleCallback_ == nullptr
 *             Test OnInterrupt audioCaptureModuleCallback_ != nullptr
 */
HWTEST_F(AudioCaptureModuleUnitTest, OnInterrupt_002, TestSize.Level1)
{
    auto audioCaptureParameter_ = std::make_shared<Meta>();
    InitMeta(audioCaptureParameter_);
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status status = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    status = audioCaptureModule_->Init();

    // Test OnInterrupt audioCaptureModuleCallback_ == nullptr
    audioCaptureModule_->audioCaptureModuleCallback_ = nullptr;
    AudioStandard::InterruptEvent interruptEvent = {};
    interruptEvent.hintType = AudioStandard::InterruptHint::INTERRUPT_HINT_NONE;
    audioCaptureModule_->audioInterruptCallback_->OnInterrupt(interruptEvent);

    // Test OnInterrupt audioCaptureModuleCallback_ != nullptr
    std::shared_ptr<MockAudioCaptureModuleCallback> mockCallback =
        std::make_shared<MockAudioCaptureModuleCallback>();
    audioCaptureModule_->audioCapturer_.reset(nullptr);
    audioCaptureModule_->audioCaptureModuleCallback_ = mockCallback;
    status = audioCaptureModule_->Init();
    audioCaptureModule_->audioInterruptCallback_->OnInterrupt(interruptEvent);
    EXPECT_EQ(status, Status::OK);
}
} // namespace Media
} // namespace OHOS