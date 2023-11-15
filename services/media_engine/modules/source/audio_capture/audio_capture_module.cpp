/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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
#include "audio_capture_module.h"
#include "common/log.h"
#include "osal/task/autolock.h"
#include "common/status.h"
#include "audio_type_translate.h"
#include "audio_capturer.h"

namespace OHOS {
namespace Media {
namespace Plugin {
namespace AudioCaptureModule {
#define FAIL_LOG_RETURN(exec, msg) \
do { \
    auto ret = exec; \
    if (ret != 0) { \
        MEDIA_LOG_E(msg " failed return " PUBLIC_LOG_D32, ret); \
        return Error2Status(ret); \
    } \
} while (0)

#define HST_NSECOND ((int64_t)1)
constexpr size_t TIME_SEC_TO_NS = 1000000000;
constexpr size_t MAX_CAPTURE_BUFFER_SIZE = 100000;

AudioCaptureModule::AudioCaptureModule() {}
 
AudioCaptureModule::~AudioCaptureModule()
{
    DoDeinit();
}

Status AudioCaptureModule::Init()
{
    AutoLock lock(captureMutex_);
    if (audioCapturer_ == nullptr) {
        AudioStandard::AppInfo appInfo;
        appInfo.appTokenId = appTokenId_;
        appInfo.appUid = appUid_;
        appInfo.appPid = appPid_;
        audioCapturer_ = AudioStandard::AudioCapturer::Create(AudioStandard::AudioStreamType::STREAM_MUSIC, appInfo);
        if (audioCapturer_ == nullptr) {
            MEDIA_LOG_E("Create audioCapturer fail");
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

Status AudioCaptureModule::DoDeinit()
{
    AutoLock lock(captureMutex_);
    if (audioCapturer_) {
        if (audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING) {
            if (!audioCapturer_->Stop()) {
                MEDIA_LOG_E("Stop audioCapturer fail");
            }
        }
        if (!audioCapturer_->Release()) {
            MEDIA_LOG_E("Release audioCapturer fail");
        }
        audioCapturer_ = nullptr;
    }
    return Status::OK;
}

Status AudioCaptureModule::Deinit()
{
    MEDIA_LOG_E("Deinit");
    return DoDeinit();
}

Status AudioCaptureModule::Prepare()
{
    MEDIA_LOG_E("IN");
    AudioStandard::AudioEncodingType audioEncoding = AudioStandard::ENCODING_INVALID;
    auto supportedEncodingTypes = OHOS::AudioStandard::AudioCapturer::GetSupportedEncodingTypes();
    for (auto& supportedEncodingType : supportedEncodingTypes) {
        if (supportedEncodingType == AudioStandard::ENCODING_PCM) {
            audioEncoding = AudioStandard::ENCODING_PCM;
            break;
        }
    }

    if (audioEncoding != AudioStandard::ENCODING_PCM) {
        MEDIA_LOG_E("audioCapturer do not support pcm encoding");
        return Status::ERROR_UNKNOWN;
    }
    capturerParams_.audioEncoding = AudioStandard::ENCODING_PCM;
    size_t size;
    {
        AutoLock lock (captureMutex_);
        FALSE_RETURN_V_MSG_E(audioCapturer_ != nullptr, Status::ERROR_WRONG_STATE, "no available audio capture");
        FAIL_LOG_RETURN(audioCapturer_->SetParams(capturerParams_), "audioCapturer SetParam");
        FAIL_LOG_RETURN(audioCapturer_->GetBufferSize(size), "audioCapturer GetBufferSize");
    }
    if (size >= MAX_CAPTURE_BUFFER_SIZE) {
        MEDIA_LOG_E("bufferSize is too big: " PUBLIC_LOG_ZU, size);
        return Status::ERROR_INVALID_PARAMETER;
    }
    bufferSize_ = size;
    MEDIA_LOG_E("bufferSize is: " PUBLIC_LOG_ZU, bufferSize_);
    return Status::OK;
}

Status AudioCaptureModule::Reset()
{
    MEDIA_LOG_E("Reset");
    {
        AutoLock lock (captureMutex_);
        FALSE_RETURN_V_MSG_E(audioCapturer_ != nullptr, Status::ERROR_WRONG_STATE, "no available audio capture");
        if (audioCapturer_->GetStatus() == AudioStandard::CapturerState::CAPTURER_RUNNING) {
            if (!audioCapturer_->Stop()) {
                MEDIA_LOG_E("Stop audioCapturer fail");
            }
        }
    }
    bufferSize_ = 0;
    bitRate_ = 0;
    capturerParams_ = AudioStandard::AudioCapturerParams();
    return Status::OK;
}

Status AudioCaptureModule::Start()
{
    MEDIA_LOG_E("start capture");
    AutoLock lock (captureMutex_);
    FALSE_RETURN_V_MSG_E(audioCapturer_ != nullptr, Status::ERROR_WRONG_STATE, "no available audio capture");
    if (audioCapturer_->GetStatus() != AudioStandard::CapturerState::CAPTURER_RUNNING) {
        if (!audioCapturer_->Start()) {
            MEDIA_LOG_E("audioCapturer start failed");
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

Status AudioCaptureModule::Stop()
{
    MEDIA_LOG_E("stop capture");
    AutoLock lock (captureMutex_);
    if (audioCapturer_ && audioCapturer_->GetStatus() == AudioStandard::CAPTURER_RUNNING) {
        if (!audioCapturer_->Stop()) {
            MEDIA_LOG_E("Stop audioCapturer fail");
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

Status AudioCaptureModule::GetParameter(std::shared_ptr<Meta>& meta)
{
    MEDIA_LOG_E("GetParameter");
    AudioStandard::AudioCapturerParams params;
    {
        AutoLock lock (captureMutex_);
        if (!audioCapturer_) {
            return Status::ERROR_WRONG_STATE;
        }
        FAIL_LOG_RETURN(audioCapturer_->GetParams(params), "audioCapturer GetParams");
    }
    
    if (params.samplingRate != capturerParams_.samplingRate) {
        MEDIA_LOG_W("samplingRate has changed from " PUBLIC_LOG_U32 " to " PUBLIC_LOG_U32,
                    capturerParams_.samplingRate, params.samplingRate);
    }
    FALSE_LOG(meta->Set<Tag::AUDIO_SAMPLE_RATE>(params.samplingRate));
    
    if (params.audioChannel != capturerParams_.audioChannel) {
        MEDIA_LOG_W("audioChannel has changed from " PUBLIC_LOG_U32 " to " PUBLIC_LOG_U32,
                    capturerParams_.audioChannel, params.audioChannel);
    }
    FALSE_LOG(meta->Set<Tag::AUDIO_CHANNEL_COUNT>(params.audioChannel));
    
    if (params.audioSampleFormat != capturerParams_.audioSampleFormat) {
        MEDIA_LOG_W("audioSampleFormat has changed from " PUBLIC_LOG_U32 " to " PUBLIC_LOG_U32,
                    capturerParams_.audioSampleFormat, params.audioSampleFormat);
    }
    FALSE_LOG(meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(static_cast<AudioSampleFormat>(params.audioSampleFormat)));

    meta->Set<Tag::MEDIA_BITRATE>(bitRate_);
        
    return Status::OK;
}

Status AudioCaptureModule::SetParameter(const std::shared_ptr<Meta>& meta)
{
    std::vector<TagType> keys;
    meta->GetKeys(keys);
    for (const auto& tag : keys) {
        ValueType value;
        if (tag == Tag::APP_TOKEN_ID) {
            MEDIA_LOG_I("APP_TOKEN_ID");
            meta->Get<Tag::APP_TOKEN_ID>(appTokenId_);
        }
        if (tag == Tag::APP_UID) {
            MEDIA_LOG_I("APP_UID");
            meta->Get<Tag::APP_UID>(appUid_);
        }
        if (tag == Tag::APP_PID) {
            MEDIA_LOG_I("APP_PID");
            meta->Get<Tag::APP_PID>(appPid_);
        }
        if (tag == Tag::MEDIA_BITRATE) {
            MEDIA_LOG_I("MEDIA_BITRATE");
            meta->Get<Tag::MEDIA_BITRATE>(bitRate_);
        }
        if (tag == Tag::AUDIO_SAMPLE_RATE) {
            MEDIA_LOG_I("AUDIO_SAMPLE_RAT");
            int32_t sampleRate = 0;
            meta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
            FALSE_RETURN_V_MSG_E(AssignSampleRateIfSupported(sampleRate), Status::ERROR_INVALID_PARAMETER,
                                "SampleRate is unsupported by audiocapturer");
        }
        if (tag == Tag::AUDIO_CHANNEL_COUNT) {
            MEDIA_LOG_I("AUDIO_CHANNEL_COUNT");
            int32_t channelCount = 0;
            meta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount);
            FALSE_RETURN_V_MSG_E(AssignChannelNumIfSupported(channelCount), Status::ERROR_INVALID_PARAMETER,
                                "ChannelNum is unsupported by audiocapturer");
        }
        if (tag == Tag::AUDIO_SAMPLE_FORMAT) {
            MEDIA_LOG_I("AUDIO_SAMPLE_FORMAT");
            AudioSampleFormat sampleFormat;
            meta->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat);
            FALSE_RETURN_V_MSG_E(AssignSampleFmtIfSupported(sampleFormat), Status::ERROR_INVALID_PARAMETER,
                                "SampleFormat is unsupported by audiocapturer");
        }
    }
    return Status::OK;
}

Status AudioCaptureModule::SetParameterByTag(TagType tag, const ValueType &value)
{
    return Status::OK;
}

bool AudioCaptureModule::AssignSampleRateIfSupported(const int32_t value)
{
    uint32_t sampleRate = (uint32_t)value;
    AudioStandard::AudioSamplingRate aRate = AudioStandard::SAMPLE_RATE_8000;
    FALSE_RETURN_V_MSG_E(SampleRateNum2Enum(sampleRate, aRate), false, "sample rate " PUBLIC_LOG_U32
                         "not supported", sampleRate);
    for (const auto& rate : AudioStandard::AudioCapturer::GetSupportedSamplingRates()) {
        if (rate == sampleRate) {
            capturerParams_.samplingRate = rate;
            return true;
        }
    }
    return false;
}

bool AudioCaptureModule::AssignChannelNumIfSupported(const int32_t value)
{
    uint32_t channelNum = (uint32_t)value;
    if (channelNum > 2) { // 2
        MEDIA_LOG_E("Unsupported channelNum: " PUBLIC_LOG_U32, channelNum);
        return false;
    }
    AudioStandard::AudioChannel aChannel = AudioStandard::MONO;
    FALSE_RETURN_V_MSG_E(ChannelNumNum2Enum(channelNum, aChannel), false, "Channel num "
                         PUBLIC_LOG_U32 "not supported", channelNum);
    for (const auto& channel : AudioStandard::AudioCapturer::GetSupportedChannels()) {
        if (channel == channelNum) {
            capturerParams_.audioChannel = channel;
            return true;
        }
    }
    return false;
}

bool AudioCaptureModule::AssignSampleFmtIfSupported(const AudioSampleFormat value)
{
    FALSE_RETURN_V_MSG_E(Any::IsSameTypeWith<AudioSampleFormat>(value), false,
                         "Check sample format value fail.");
    AudioSampleFormat sampleFormat = value;
    AudioStandard::AudioSampleFormat aFmt = AudioStandard::AudioSampleFormat::INVALID_WIDTH;
    FALSE_RETURN_V_MSG_E(Plugin::AudioCaptureModule::PluginFmt2SampleFmt(sampleFormat, aFmt), false,
                         "sample format " PUBLIC_LOG_U8 " not supported", static_cast<uint8_t>(sampleFormat));
    for (const auto& fmt : AudioStandard::AudioCapturer::GetSupportedFormats()) {
        if (fmt == aFmt) {
            capturerParams_.audioSampleFormat = fmt;
            return true;
        }
    }
    return false;
}

Status AudioCaptureModule::GetAudioTimeLocked(int64_t& audioTimeNs)
{
    OHOS::AudioStandard::Timestamp timeStamp;
    auto timeBase = OHOS::AudioStandard::Timestamp::Timestampbase::MONOTONIC;
    if (!audioCapturer_->GetAudioTime(timeStamp, timeBase)) {
        MEDIA_LOG_E("audioCapturer GetAudioTimeLocked() fail");
        return Status::ERROR_UNKNOWN;
    }
    if (timeStamp.time.tv_sec < 0 || timeStamp.time.tv_nsec < 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    if ((UINT64_MAX - timeStamp.time.tv_nsec) / TIME_SEC_TO_NS < static_cast<uint64_t>(timeStamp.time.tv_sec)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    audioTimeNs = timeStamp.time.tv_sec * TIME_SEC_TO_NS + timeStamp.time.tv_nsec;
    return Status::OK;
}

Status AudioCaptureModule::Read(std::shared_ptr<AVBuffer>& buffer, size_t expectedLen)
{
    auto bufferMeta = buffer->meta_;
    if (!bufferMeta) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    std::shared_ptr<AVMemory> bufData = buffer->memory_;
    if (bufData->GetCapacity() <= 0) {
        return Status::ERROR_NO_MEMORY;
    }
    auto size = 0;
    Status ret = Status::OK;
    int64_t timestampNs = 0;
    {
        AutoLock lock(captureMutex_);
        if (audioCapturer_->GetStatus() != AudioStandard::CAPTURER_RUNNING) {
            return Status::ERROR_AGAIN;
        }
        size = audioCapturer_->Read(*bufData->GetAddr(), expectedLen, true);
        ret = GetAudioTimeLocked(timestampNs);
    }
    if (size < 0) {
        MEDIA_LOG_E("audioCapturer Read() fail");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    if (ret != Status::OK) {
        MEDIA_LOG_E("Get audio timestamp fail");
        return ret;
    }
    buffer->pts_ = timestampNs * HST_NSECOND;
    return ret;
}

Status AudioCaptureModule::GetSize(uint64_t& size)
{
    if (bufferSize_ == 0) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    size = bufferSize_;
    MEDIA_LOG_E("BufferSize_: " PUBLIC_LOG_U64, size);
    return Status::OK;
}
} // namespace AuCapturePlugin
} // namespace Plugin
} // namespace Media
} // namespace OHOS
