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
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "audio_capture_module_unit_test.h"

#define LOCAL true

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

namespace {
} // namespace

void AudioCaptureModuleUnitTest::SetUpTestCase(void)
{
}

void AudioCaptureModuleUnitTest::TearDownTestCase(void)
{
}

void AudioCaptureModuleUnitTest::SetUp(void) {}

void AudioCaptureModuleUnitTest::TearDown(void)
{
}

/**********************************source FD**************************************/
namespace {
/**
 * @tc.name: AVSource_CreateSourceWithFD_1000
 * @tc.desc: create source with fd, mp4
 * @tc.type: FUNC
 */
HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureInit_1000, TestSize.Level1)
{
    std::shared_ptr<AudioCaptureModule::AudioCaptureModule> audioCaptureModule_ =
        std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureModule_->SetAudioSource(OHOS::AudioStandard::SourceType::SOURCE_TYPE_MIC);
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
    std::shared_ptr<AudioCaptureModule::AudioCaptureModule> audioCaptureModule_ =
        std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureModule_->SetAudioSource(OHOS::AudioStandard::SourceType::SOURCE_TYPE_MIC);
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
    std::shared_ptr<AudioCaptureModule::AudioCaptureModule> audioCaptureModule_ =
        std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureModule_->SetAudioSource(OHOS::AudioStandard::SourceType::SOURCE_TYPE_MIC);
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

HWTEST_F(AudioCaptureModuleUnitTest, AudioCaptureRead_1000, TestSize.Level1)
{
    std::shared_ptr<AudioCaptureModule::AudioCaptureModule> audioCaptureModule_ =
        std::make_shared<AudioCaptureModule::AudioCaptureModule>();
    audioCaptureModule_->SetAudioSource(OHOS::AudioStandard::SourceType::SOURCE_TYPE_MIC);
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

} // namespace