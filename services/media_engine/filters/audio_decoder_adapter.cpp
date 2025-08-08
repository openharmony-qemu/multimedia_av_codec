/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#define AUDIO_DECODER_ADAPTER_CPP

#include "avcodec_errors.h"
#include "common/log.h"
#include "meta/meta.h"
#include "audio_decoder_adapter.h"
#include "scoped_timer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioDecoderAdapter"};
constexpr int64_t AUDIOCODEC_START_WARNING_MS = 50;
}

namespace OHOS {
namespace Media {

using namespace MediaAVCodec;

AudioDecoderAdapter::AudioDecoderAdapter()
{
}

AudioDecoderAdapter::~AudioDecoderAdapter()
{
    MEDIA_LOG_I("~AudioDecoderAdapter.");
    FALSE_RETURN_MSG(audiocodec_ != nullptr, "audiocodec_ is nullptr");
    audiocodec_->Release();
}

Status AudioDecoderAdapter::Init(bool isMimeType, const std::string &name)
{
    MEDIA_LOG_D("isMimeType is %{public}i, name is %{public}s", isMimeType, name.c_str());
    if (isMimeType) {
        audiocodec_ = MediaAVCodec::AudioCodecFactory::CreateByMime(name, false);
    } else {
        audiocodec_ = MediaAVCodec::AudioCodecFactory::CreateByName(name);
    }
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    return Status::OK;
}

Status AudioDecoderAdapter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->Configure(parameter);
    if (ret == AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL) {
        MEDIA_LOG_E("Configure ret is %{public}d", ret);
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    FALSE_RETURN_V(ret == AVCodecServiceErrCode::AVCS_ERR_OK, Status::ERROR_INVALID_STATE);
    MEDIA_LOG_D("out");
    return Status::OK;
}

Status AudioDecoderAdapter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    FALSE_RETURN_V(parameter != nullptr, Status::ERROR_INVALID_PARAMETER);
    int32_t ret = audiocodec_->SetParameter(parameter);
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::Prepare()
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->Prepare();
    FALSE_RETURN_V(ret == AVCodecServiceErrCode::AVCS_ERR_OK, Status::ERROR_INVALID_STATE);
    isRunning_ = false;
    MEDIA_LOG_D("out");
    return Status::OK;
}

Status AudioDecoderAdapter::Start()
{
    MEDIA_LOG_D("In");
    if (isRunning_.load()) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    int32_t ret;
    {
        ScopedTimer timer("AudioCodec Start", AUDIOCODEC_START_WARNING_MS);
        ret = audiocodec_->Start();
    }
    FALSE_RETURN_V(ret == AVCodecServiceErrCode::AVCS_ERR_OK, Status::ERROR_INVALID_STATE);
    isRunning_ = true;
    MEDIA_LOG_D("out");
    return Status::OK;
}

Status AudioDecoderAdapter::Stop()
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::OK, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->Stop();
    FALSE_RETURN_V(ret == AVCodecServiceErrCode::AVCS_ERR_OK, Status::ERROR_INVALID_STATE);
    isRunning_ = false;
    MEDIA_LOG_D("out");
    return Status::OK;
}

Status AudioDecoderAdapter::Flush()
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->Flush();
    FALSE_RETURN_V(ret == AVCodecServiceErrCode::AVCS_ERR_OK, Status::ERROR_INVALID_STATE);
    isRunning_ = false;
    MEDIA_LOG_D("out");
    return Status::OK;
}

Status AudioDecoderAdapter::Reset()
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::OK, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->Reset();
    FALSE_RETURN_V(ret == AVCodecServiceErrCode::AVCS_ERR_OK, Status::ERROR_INVALID_STATE);
    isRunning_ = false;
    MEDIA_LOG_D("out");
    return Status::OK;
}

Status AudioDecoderAdapter::Release()
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::OK, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->Release();
    FALSE_RETURN_V(ret == AVCodecServiceErrCode::AVCS_ERR_OK, Status::ERROR_INVALID_STATE);
    isRunning_ = false;
    MEDIA_LOG_D("out");
    return Status::OK;
}

Status AudioDecoderAdapter::SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    FALSE_RETURN_V(bufferQueueProducer != nullptr, Status::ERROR_INVALID_PARAMETER);
    outputBufferQueueProducer_ = bufferQueueProducer;
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->SetOutputBufferQueue(bufferQueueProducer);
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

sptr<Media::AVBufferQueueProducer> AudioDecoderAdapter::GetInputBufferQueue()
{
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, nullptr, "audiocodec_ is nullptr");
    inputBufferQueueProducer_ = audiocodec_->GetInputBufferQueue();
    return inputBufferQueueProducer_;
}

sptr<Media::AVBufferQueueConsumer> AudioDecoderAdapter::GetInputBufferQueueConsumer()
{
    return audiocodec_ != nullptr ? audiocodec_->GetInputBufferQueueConsumer() : nullptr;
}

sptr<Media::AVBufferQueueProducer> AudioDecoderAdapter::GetOutputBufferQueueProducer()
{
    return audiocodec_ != nullptr ? audiocodec_->GetOutputBufferQueueProducer() : nullptr;
}

void AudioDecoderAdapter::ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus)
{
    FALSE_RETURN_MSG(audiocodec_ != nullptr, "ProcessInputBufferInner audiocodec_ is nullptr");
    audiocodec_->ProcessInputBufferInner(isTriggeredByOutPort, isFlushed, bufferStatus);
}

int32_t AudioDecoderAdapter::GetOutputFormat(std::shared_ptr<Meta> &parameter)
{
    FALSE_RETURN_V(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    return audiocodec_->GetOutputFormat(parameter);
}

Status AudioDecoderAdapter::ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta)
{
    FALSE_RETURN_V(meta != nullptr, Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    int32_t ret = audiocodec_->ChangePlugin(mime, isEncoder, meta);
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

void AudioDecoderAdapter::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    FALSE_RETURN_MSG(audiocodec_ != nullptr, "audiocodec_ is nullptr");
    audiocodec_->SetDumpInfo(isDump, instanceId);
}

void AudioDecoderAdapter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("OnDumpInfo called.");
    FALSE_RETURN_MSG(fd >= 0, "OnDumpInfo fd is invalid.");
    std::string dumpString;
    FALSE_RETURN_MSG(inputBufferQueueProducer_ != nullptr, "inputBufferQueueProducer_ is nullptr");
    FALSE_RETURN_MSG(outputBufferQueueProducer_ != nullptr, "outputBufferQueueProducer_ is nullptr");
    dumpString += "AudioDecoderAdapter inputBufferQueueProducer_ size is:" +
                  std::to_string(inputBufferQueueProducer_->GetQueueSize()) + "\n";
    dumpString += "AudioDecoderAdapter outputBufferQueueProducer_ size is:" +
                  std::to_string(outputBufferQueueProducer_->GetQueueSize()) + "\n";
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    FALSE_RETURN_MSG(ret >= 0, "AudioDecoderAdapter::OnDumpInfo write failed.");
}

int32_t AudioDecoderAdapter::NotifyEos()
{
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    int ret = audiocodec_->NotifyEos();
    return ret;
}

int32_t AudioDecoderAdapter::SetCodecCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &codecCallback)
{
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    return audiocodec_->SetCodecCallback(codecCallback);
}

int32_t AudioDecoderAdapter::SetAudioDecryptionConfig(
    const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, (int32_t)Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    return audiocodec_->SetAudioDecryptionConfig(keySession, svpFlag);
}

}  // namespace Media
}  // namespace OHOS
