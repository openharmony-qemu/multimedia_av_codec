/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include <nativetoken_kit.h>
#include <token_setproc.h>
#include <accesstoken_kit.h>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "audio_capture_module_unit_test.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;
using namespace Security::AccessToken;
namespace {
} // namespace

/**********************************source FD**************************************/
namespace OHOS {

void AudioCaptureModuleUnitTest::SetUpTestCase(void)
{
    HapInfoParams info = {
        .userID = 100, // 100 user ID
        .bundleName = "com.ohos.test.audiocapturertdd",
        .instIndex = 0, // 0 index
        .appIDDesc = "com.ohos.test.audiocapturertdd",
        .isSystemApp = true
    };

    HapPolicyParams policy = {
        .apl = APL_SYSTEM_BASIC,
        .domain = "test.audiocapturermodule",
        .permList = { },
        .permStateList = {
            {
                .permissionName = "ohos.permission.MICROPHONE",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            },
            {
                .permissionName = "ohos.permission.READ_MEDIA",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            },
            {
                .permissionName = "ohos.permission.WRITE_MEDIA",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            },
            {
                .permissionName = "ohos.permission.KEEP_BACKGROUND_RUNNING",
                .isGeneral = true,
                .resDeviceID = { "local" },
                .grantStatus = { PermissionState::PERMISSION_GRANTED },
                .grantFlags = { 1 }
            }
        }
    };
    AccessTokenIDEx tokenIdEx = { 0 };
    tokenIdEx = AccessTokenKit::AllocHapToken(info, policy);
    int ret = SetSelfTokenID(tokenIdEx.tokenIDEx);
    if (ret != 0) {
        std::cout<<"Set hap token failed"<<std::endl;
    }
}

void AudioCaptureModuleUnitTest::TearDownTestCase(void)
{
}

void AudioCaptureModuleUnitTest::SetUp(void)
{
    audioCaptureParameter_ = std::make_shared<Meta>();
    audioCaptureParameter_->Set<Tag::APP_TOKEN_ID>(appTokenId_);
    audioCaptureParameter_->Set<Tag::APP_UID>(appUid_);
    audioCaptureParameter_->Set<Tag::APP_PID>(appPid_);
    audioCaptureParameter_->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId_);
    audioCaptureParameter_->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    audioCaptureParameter_->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    audioCaptureParameter_->Set<Tag::AUDIO_CHANNEL_COUNT>(channel_);
    audioCaptureParameter_->Set<Tag::MEDIA_BITRATE>(bitRate_);
    audioCaptureModule_ = std::make_shared<AudioCaptureModule::AudioCaptureModule>();
}

void AudioCaptureModuleUnitTest::TearDown(void)
{
    audioCaptureParameter_ = nullptr;
    audioCaptureModule_ = nullptr;
}
class CapturerInfoChangeCallback : public AudioStandard::AudioCapturerInfoChangeCallback {
public:
    explicit CapturerInfoChangeCallback() { }
    void OnStateChange(const AudioStandard::AudioCapturerChangeInfo &info)
    {
        (void)info;
        std::cout<<"AudioCapturerInfoChangeCallback"<<std::endl;
    }
};

class AudioCaptureModuleCallbackTest : public AudioCaptureModule::AudioCaptureModuleCallback {
public:
    explicit AudioCaptureModuleCallbackTest() { }
    void OnInterrupt(const std::string &interruptInfo) override
    {
        std::cout<<"AudioCaptureModuleCallback interrupt"<<interruptInfo<<std::endl;
    }
};

/**
 * @tc.name: AVSource_CreateSourceWithFD_1000
 * @tc.desc: create source with fd, mp4
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureInit_1000, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    std::shared_ptr<Meta> audioCaptureFormat = std::make_shared<Meta>();
    audioCaptureFormat->Set<Tag::APP_TOKEN_ID>(appTokenId_);
    audioCaptureFormat->Set<Tag::APP_UID>(appUid_);
    audioCaptureFormat->Set<Tag::APP_PID>(appPid_);
    audioCaptureFormat->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId_);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    audioCaptureFormat->Set<Tag::AUDIO_CHANNEL_COUNT>(channel_);
    audioCaptureFormat->Set<Tag::MEDIA_BITRATE>(bitRate_);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureFormat);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}

HWTEST_F(AudioCaptureModuleUnitTest, AudioCapturePrepare_1000, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    std::shared_ptr<Meta> audioCaptureFormat = std::make_shared<Meta>();
    audioCaptureFormat->Set<Tag::APP_TOKEN_ID>(appTokenId_);
    audioCaptureFormat->Set<Tag::APP_UID>(appUid_);
    audioCaptureFormat->Set<Tag::APP_PID>(appPid_);
    audioCaptureFormat->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId_);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    audioCaptureFormat->Set<Tag::AUDIO_CHANNEL_COUNT>(channel_);
    audioCaptureFormat->Set<Tag::MEDIA_BITRATE>(bitRate_);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureFormat);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}

HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureStart_1000, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    std::shared_ptr<Meta> audioCaptureFormat = std::make_shared<Meta>();
    audioCaptureFormat->Set<Tag::APP_TOKEN_ID>(appTokenId_);
    audioCaptureFormat->Set<Tag::APP_UID>(appUid_);
    audioCaptureFormat->Set<Tag::APP_PID>(appPid_);
    audioCaptureFormat->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId_);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    audioCaptureFormat->Set<Tag::AUDIO_CHANNEL_COUNT>(channel_);
    audioCaptureFormat->Set<Tag::MEDIA_BITRATE>(bitRate_);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureFormat);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Start();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Stop();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}

HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureRead_0100, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    std::shared_ptr<Meta> audioCaptureFormat = std::make_shared<Meta>();
    audioCaptureFormat->Set<Tag::APP_TOKEN_ID>(appTokenId_);
    audioCaptureFormat->Set<Tag::APP_UID>(appUid_);
    audioCaptureFormat->Set<Tag::APP_PID>(appPid_);
    audioCaptureFormat->Set<Tag::APP_FULL_TOKEN_ID>(appFullTokenId_);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    audioCaptureFormat->Set<Tag::AUDIO_CHANNEL_COUNT>(channel_);
    audioCaptureFormat->Set<Tag::MEDIA_BITRATE>(bitRate_);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureFormat);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Start();
    ASSERT_TRUE(ret == Status::OK);
    uint64_t bufferSize = 0;
    ret = audioCaptureModule_->GetSize(bufferSize);
    ASSERT_TRUE(ret == Status::OK);
    std::shared_ptr<AVAllocator> avAllocator =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    int32_t capacity = 1024;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    ret = audioCaptureModule_->Read(buffer, bufferSize);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Stop();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureRead_0200, TestSize.Level1)
{
    std::shared_ptr<AVAllocator> avAllocator =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator);
    size_t bufferSize = 1024;
    audioCaptureModule_->Read(buffer, bufferSize);
}
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureRead_0300, TestSize.Level1)
{
    std::shared_ptr<AVAllocator> avAllocator =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator);
    buffer->meta_ = nullptr;
    size_t bufferSize = 1024;
    audioCaptureModule_->Read(buffer, bufferSize);
}
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureRead_0400, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    uint64_t bufferSize = 0;
    ret = audioCaptureModule_->GetSize(bufferSize);
    ASSERT_TRUE(ret == Status::OK);
    std::shared_ptr<AVAllocator> avAllocator =
    AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    int32_t capacity = 1024;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    EXPECT_NE(Status::OK, audioCaptureModule_->Read(buffer, bufferSize));
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioCaptureGetCurrentChangeInfo_0100
 * @tc.desc: test GetCurrentChangeInfo
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureGetCurrentChangeInfo_0100, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    AudioStandard::AudioCapturerChangeInfo changeInfo;
    audioCaptureModule_->GetCurrentCapturerChangeInfo(changeInfo);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}

/**
 * @tc.name: AudioCaptureGetCurrentChangeInfo_0200
 * @tc.desc: test GetCurrentChangeInfo
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureGetCurrentChangeInfo_0200, TestSize.Level1)
{
    AudioStandard::AudioCapturerChangeInfo changeInfo;
    audioCaptureModule_->GetCurrentCapturerChangeInfo(changeInfo);
    Status ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}

/**
 * @tc.name: AudioCaptureSetCallingInfo_0100
 * @tc.desc: test SetCallingInfo
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureSetCallingInfo_0100, TestSize.Level1)
{
    audioCaptureModule_->SetCallingInfo(appUid_, appPid_, bundleName_, instanceId_);
    Status ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}

/**
 * @tc.name: AudioCaptureGetMaxAmplitude_0100
 * @tc.desc: test GetMaxAmplitude
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureGetMaxAmplitude_0100, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Start();
    ASSERT_TRUE(ret == Status::OK);
    uint64_t bufferSize = 0;
    ret = audioCaptureModule_->GetSize(bufferSize);
    ASSERT_TRUE(ret == Status::OK);
    audioCaptureModule_->GetMaxAmplitude();
    audioCaptureModule_->GetMaxAmplitude();
    std::shared_ptr<AVAllocator> avAllocator =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    int32_t capacity = 1024;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    ret = audioCaptureModule_->Read(buffer, bufferSize);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Stop();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioSetAudioCapturerInfoChangeCallback_0100
 * @tc.desc: test SetAudioCapturerInfoChangeCallback
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioSetAudioCapturerInfoChangeCallback_0100, TestSize.Level1)
{
    EXPECT_NE(Status::OK, audioCaptureModule_->SetAudioCapturerInfoChangeCallback(nullptr));
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    EXPECT_NE(Status::OK, audioCaptureModule_->SetAudioCapturerInfoChangeCallback(nullptr));
    std::shared_ptr<AudioStandard::AudioCapturerInfoChangeCallback> callback =
        std::make_shared<CapturerInfoChangeCallback>();
    EXPECT_NE(Status::OK, audioCaptureModule_->SetAudioCapturerInfoChangeCallback(nullptr));
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioSetAudioInterruptListener_0100
 * @tc.desc: test SetAudioInterruptListener
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioSetAudioInterruptListener_0100, TestSize.Level1)
{
    EXPECT_NE(Status::OK, audioCaptureModule_->SetAudioInterruptListener(nullptr));
    std::shared_ptr<AudioCaptureModule::AudioCaptureModuleCallback> callback =
        std::make_shared<AudioCaptureModuleCallbackTest>();
    EXPECT_EQ(Status::OK, audioCaptureModule_->SetAudioInterruptListener(callback));
    Status ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioGetSize_0100
 * @tc.desc: test GetSize
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioGetSize_0100, TestSize.Level1)
{
    uint64_t size = 0;
    EXPECT_NE(Status::OK, audioCaptureModule_->GetSize(size));
    Status ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioGetSize_0200
 * @tc.desc: test GetSize
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioGetSize_0200, TestSize.Level1)
{
    uint64_t size = 0;
    EXPECT_NE(Status::OK, audioCaptureModule_->GetSize(size));
    Status ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioGetSize_0300
 * @tc.desc: test GetSize
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioGetSize_0300, TestSize.Level1)
{
    uint64_t size = 0;
    EXPECT_NE(Status::OK, audioCaptureModule_->GetSize(size));
    Status ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioSetParameter_0100
 * @tc.desc: test SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioSetParameter_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> audioCaptureFormat = std::make_shared<Meta>();
    audioCaptureModule_->SetParameter(audioCaptureFormat);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate_);
    audioCaptureModule_->SetParameter(audioCaptureFormat);
    audioCaptureFormat->Set<Tag::AUDIO_CHANNEL_COUNT>(channel_);
    audioCaptureModule_->SetParameter(audioCaptureFormat);
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_S16LE);
    audioCaptureModule_->SetParameter(audioCaptureFormat);
}
/**
 * @tc.name: AudioSetParameter_0200
 * @tc.desc: test SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioSetParameter_0200, TestSize.Level1)
{
    std::shared_ptr<Meta> audioCaptureFormat = std::make_shared<Meta>();
    audioCaptureFormat->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_F32P);
    audioCaptureModule_->SetParameter(audioCaptureFormat);
}
/**
 * @tc.name: AudioSetParameter_0300
 * @tc.desc: test SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioSetParameter_0300, TestSize.Level1)
{
    int32_t channel = 3;
    audioCaptureParameter_->Set<Tag::AUDIO_CHANNEL_COUNT>(channel);
    audioCaptureModule_->SetParameter(audioCaptureParameter_);
}
/**
 * @tc.name: AudioGetParameter_0100
 * @tc.desc: test GetParameter
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioGetParameter_0100, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    std::shared_ptr<Meta> audioCaptureParameterTest_ = std::make_shared<Meta>();
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    EXPECT_NE(Status::OK, audioCaptureModule_->GetParameter(audioCaptureParameterTest_));
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    audioCaptureModule_->GetParameter(audioCaptureParameterTest_);

    audioCaptureParameter_->Set<Tag::AUDIO_SAMPLE_FORMAT>(Plugins::AudioSampleFormat::SAMPLE_U8);
    audioCaptureModule_->SetParameter(audioCaptureParameter_);
    audioCaptureModule_->GetParameter(audioCaptureParameterTest_);
    int32_t channel = 2;
    audioCaptureParameter_->Set<Tag::AUDIO_CHANNEL_COUNT>(channel);
    audioCaptureModule_->SetParameter(audioCaptureParameter_);
    audioCaptureModule_->GetParameter(audioCaptureParameterTest_);
    int32_t sampleRate = 64000;
    audioCaptureParameter_->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    audioCaptureModule_->SetParameter(audioCaptureParameter_);
    audioCaptureModule_->GetParameter(audioCaptureParameterTest_);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioReset_0100
 * @tc.desc: test Reset
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioReset_0100, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Start();
    ASSERT_TRUE(ret == Status::OK);
    uint64_t bufferSize = 0;
    ret = audioCaptureModule_->GetSize(bufferSize);
    ASSERT_TRUE(ret == Status::OK);
    std::shared_ptr<AVAllocator> avAllocator =
        AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    int32_t capacity = 1024;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    ret = audioCaptureModule_->Read(buffer, bufferSize);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Stop();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Reset();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: AudioReset_0200
 * @tc.desc: test Reset
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioReset_0200, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Prepare();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Reset();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: Audioinit_0100
 * @tc.desc: test init
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, Audioinit_0100, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    Status ret = audioCaptureModule_->SetParameter(audioCaptureParameter_);
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Init();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
/**
 * @tc.name: Audioinit_0200
 * @tc.desc: test init
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, Audioinit_0200, TestSize.Level1)
{
    audioCaptureModule_->SetAudioSource(AudioStandard::SourceType::SOURCE_TYPE_MIC);
    EXPECT_NE(Status::OK, audioCaptureModule_->Init());
    Status ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);
}
} // namespace OHOS