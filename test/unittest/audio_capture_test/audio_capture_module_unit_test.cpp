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

    auto createPermissionState = [](const std::string& permissionName) {
        return PermissionStateFull {
            .permissionName = permissionName,
            .isGeneral = true,
            .resDeviceID = { "local" },
            .grantStatus = { PermissionState::PERMISSION_GRANTED },
            .grantFlags = { 1 }
        };
    };
    
    std::vector<PermissionStateFull> permissionList = {
        createPermissionState("ohos.permission.MICROPHONE"),
        createPermissionState("ohos.permission.READ_MEDIA"),
        createPermissionState("ohos.permission.WRITE_MEDIA"),
        createPermissionState("ohos.permission.KEEP_BACKGROUND_RUNNING"),
        createPermissionState("ohos.permission.GET_BUNDLE_INFO"),
        createPermissionState("ohos.permission.GET_BUNDLE_INFO_PRIVILEGED")
    };

    HapPolicyParams policy = {
        .apl = APL_SYSTEM_BASIC,
        .domain = "test.audiocapturermodule",
        .permList = { },
        .permStateList = permissionList
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

    size_t bufferLen;
    ret = audioCapturer->GetBufferSize(bufferLen);
    EXPECT_EQ(SUCCESS, ret);
    
    uint8_t *buffer = (uint8_t *) malloc(bufferLen);
    ASSERT_NE(nullptr, buffer);
    ret = audioCapturer->Read(buffer, bufferLen);

    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Stop();
    ASSERT_TRUE(ret == Status::OK);
    ret = audioCaptureModule_->Deinit();
    ASSERT_TRUE(ret == Status::OK);

    free(buffer);
}
} // namespace OHOS