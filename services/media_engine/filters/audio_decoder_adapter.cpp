/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#define MEDIA_PIPELINE

#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>
#include "avcodec_audio_decoder.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "media_description.h"
#include "surface_type.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "audio_decoder_adapter.h"
#include "media_core.h"
#include "avcodec_sysevent.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "AudioDecoderAdapter"};
}

namespace OHOS {
namespace Media {

using namespace MediaAVCodec;

AudioDecoderAdapter::AudioDecoderAdapter()
{
    state_ = CodecState::UNINITIALIZED;
    MEDIA_LOG_I("AudioDecoderAdapter instances create.");
}

AudioDecoderAdapter::~AudioDecoderAdapter()
{
    state_ = CodecState::UNINITIALIZED;
    MEDIA_LOG_I("~AudioDecoderAdapter.");
    FALSE_RETURN_MSG(audiocodec_ != nullptr, "audiocodec_ is nullptr");
    audiocodec_->Release();
}

Status AudioDecoderAdapter::Init(bool isMimeType, const std::string &name)
{
    MEDIA_LOG_I("AudioDecoderAdapter::Init enter: " PUBLIC_LOG_S, name.c_str());
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V_MSG(state_ == CodecState::UNINITIALIZED,
        Status::ERROR_INVALID_STATE,
        "Init failed, state = %{public}s .",
        StateToString(state_).data());

    MEDIA_LOG_I("state from %{public}s to INITIALIZING", StateToString(state_).data());
    if (isMimeType) {
        audiocodec_ = MediaAVCodec::AudioCodecFactory::CreateByMime(name, false);
        MEDIA_LOG_I("AudioDecoderAdapter::Init CreateByMime");
    } else {
        audiocodec_ = MediaAVCodec::AudioCodecFactory::CreateByName(name);
        MEDIA_LOG_I("AudioDecoderAdapter::Init CreateByName");
    }
    FALSE_RETURN_V_MSG(audiocodec_ != nullptr, Status::ERROR_INVALID_STATE, "audiocodec_ is nullptr");
    state_ = CodecState::INITIALIZED;
    return Status::OK;
}

Status AudioDecoderAdapter::Configure(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("Configure enter.");
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::INITIALIZED, Status::ERROR_INVALID_STATE);
    int32_t ret = audiocodec_->Configure(parameter);
    state_ = CodecState::CONFIGURED;
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter enter.");
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(parameter != nullptr, Status::ERROR_INVALID_PARAMETER);
    FALSE_RETURN_V(
        state_ != CodecState::UNINITIALIZED && state_ != CodecState::INITIALIZED && state_ != CodecState::PREPARED,
        Status::ERROR_INVALID_STATE);
    int32_t ret = audiocodec_->SetParameter(parameter);
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::Prepare()
{
    MEDIA_LOG_I("Prepare enter.");
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V_MSG_W(state_ != CodecState::FLUSHED, Status::ERROR_AGAIN, "state is flushed, no need prepare");
    FALSE_RETURN_V(state_ != CodecState::PREPARED, Status::OK);
    FALSE_RETURN_V(state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE);
    int32_t ret = audiocodec_->Prepare();
    state_ = CodecState::PREPARED;
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::Start()
{
    MEDIA_LOG_I("Start enter.");
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ != CodecState::RUNNING, Status::OK);
    FALSE_RETURN_V(state_ == CodecState::PREPARED || state_ == CodecState::FLUSHED, Status::ERROR_INVALID_STATE);
    state_ = CodecState::STARTING;
    int32_t ret = audiocodec_->Start();
    state_ = CodecState::RUNNING;
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::Stop()
{
    MEDIA_LOG_I("Stop enter.");
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ != CodecState::PREPARED, Status::OK);
    FALSE_RETURN_V_MSG_W(
        state_ != CodecState::UNINITIALIZED && state_ != CodecState::STOPPING && state_ != CodecState::RELEASING,
        Status::OK,
        "Stop, state_=%{public}s",
        StateToString(state_).data());

    FALSE_RETURN_V(
        state_ == CodecState::RUNNING || state_ == CodecState::END_OF_STREAM || state_ == CodecState::FLUSHED,
        Status::ERROR_INVALID_STATE);
    state_ = CodecState::STOPPING;
    int32_t ret = audiocodec_->Stop();
    state_ = CodecState::PREPARED;
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::Flush()
{
    MEDIA_LOG_I("Flush enter.");
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V_MSG_W(state_ != CodecState::FLUSHED,
        Status::OK,
        "Flush, state is already flushed, state_=%{public}s .",
        StateToString(state_).data());

    if (state_ != CodecState::RUNNING && state_ != CodecState::END_OF_STREAM) {
        MEDIA_LOG_E("Flush failed, state =%{public}s", StateToString(state_).data());
        return Status::ERROR_INVALID_STATE;
    }
    MEDIA_LOG_I("Flush, state from %{public}s to FLUSHING", StateToString(state_).data());
    state_ = CodecState::FLUSHING;
    int32_t ret = audiocodec_->Flush();
    state_ = CodecState::FLUSHED;
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::Reset()
{
    AutoLock lock(stateMutex_);
    MEDIA_LOG_I("Reset enter.");
    FALSE_RETURN_V_MSG_W(state_ != CodecState::UNINITIALIZED && state_ != CodecState::RELEASING,
        Status::OK,
        "reset, state is already released, state =%{public}s .",
        StateToString(state_).data());

    if (state_ == CodecState::INITIALIZING) {
        MEDIA_LOG_W("adapter reset, state is initialized, state =%{public}s .", StateToString(state_).data());
        state_ = CodecState::INITIALIZED;
        return Status::OK;
    }
    int32_t ret = audiocodec_->Reset();
    state_ = CodecState::INITIALIZED;
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::Release()
{
    AutoLock lock(stateMutex_);
    MEDIA_LOG_I("Release enter.");
    FALSE_RETURN_V_MSG_W(state_ != CodecState::UNINITIALIZED && state_ != CodecState::RELEASING,
        Status::OK,
        "Release, state isnot completely correct, state =%{public}s .",
        StateToString(state_).data());

    if (state_ == CodecState::INITIALIZING) {
        MEDIA_LOG_W("Release, state isnot completely correct, state =%{public}s .", StateToString(state_).data());
        state_ = CodecState::RELEASING;
        return Status::OK;
    }
    MEDIA_LOG_I("state from %{public}s to RELEASING", StateToString(state_).data());
    state_ = CodecState::RELEASING;
    int32_t ret = audiocodec_->Release();
    state_ = CodecState::UNINITIALIZED;
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

Status AudioDecoderAdapter::SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    AutoLock lock(stateMutex_);
    MEDIA_LOG_I("SetOutputBufferQueue enter.");
    FALSE_RETURN_V(state_ == CodecState::INITIALIZED || state_ == CodecState::CONFIGURED, Status::ERROR_INVALID_STATE);
    outputBufferQueueProducer_ = bufferQueueProducer;
    int32_t ret = audiocodec_->SetOutputBufferQueue(bufferQueueProducer);
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

sptr<Media::AVBufferQueueProducer> AudioDecoderAdapter::GetInputBufferQueue()
{
    MEDIA_LOG_I("GetInputBufferQueue enter.");
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ == CodecState::PREPARED, sptr<AVBufferQueueProducer>());
    inputBufferQueueProducer_ = audiocodec_->GetInputBufferQueue();
    return inputBufferQueueProducer_;
}

int32_t AudioDecoderAdapter::GetOutputFormat(std::shared_ptr<Meta> &parameter)
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V_MSG_E(state_ != CodecState::UNINITIALIZED,
        (int32_t)Status::ERROR_INVALID_STATE,
        "status incorrect,get output format failed.");
    FALSE_RETURN_V(parameter != nullptr, (int32_t)Status::ERROR_INVALID_PARAMETER);
    return audiocodec_->GetOutputFormat(parameter);
}

Status AudioDecoderAdapter::ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta)
{
    MEDIA_LOG_I("ChangePlugin enter.");
    int32_t ret = audiocodec_->ChangePlugin(mime, isEncoder, meta);
    return ret == AVCodecServiceErrCode::AVCS_ERR_OK ? Status::OK : Status::ERROR_INVALID_STATE;
}

void AudioDecoderAdapter::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    if (isDump && instanceId == 0) {
        MEDIA_LOG_W("Cannot dump with instanceId 0.");
        return;
    }
    dumpPrefix_ = std::to_string(instanceId);
    isDump_ = isDump;
}

void AudioDecoderAdapter::OnDumpInfo(int32_t fd)
{
    MEDIA_LOG_D("AudioDecoderAdapter::OnDumpInfo called.");
    if (fd < 0) {
        MEDIA_LOG_E("AudioDecoderAdapter::OnDumpInfo fd is invalid.");
        return;
    }
    std::string dumpString;
    dumpString +=
        "AudioDecoderAdapter inputBufferQueueProducer_ size is:" + std::to_string(inputBufferQueueProducer_->GetQueueSize()) + "\n";
    dumpString +=
        "AudioDecoderAdapter outputBufferQueueProducer_ size is:" + std::to_string(outputBufferQueueProducer_->GetQueueSize()) + "\n";
    dumpString +=
        "state_ is:" + StateToString(state_) + "\n";
    int ret = write(fd, dumpString.c_str(), dumpString.size());
    if (ret < 0) {
        MEDIA_LOG_E("AudioDecoderAdapter::OnDumpInfo write failed.");
        return;
    }
}

int32_t AudioDecoderAdapter::NotifyEos()
{
    AutoLock lock(stateMutex_);
    FALSE_RETURN_V(state_ != CodecState::END_OF_STREAM, (int32_t)Status::OK);
    FALSE_RETURN_V(state_ == CodecState::RUNNING, (int32_t)Status::ERROR_INVALID_STATE);
    state_ = CodecState::END_OF_STREAM;
    int ret = audiocodec_->NotifyEos();
    return ret;
}

int32_t AudioDecoderAdapter::SetCodecCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &codecCallback)
{
    MEDIA_LOG_I("SetCodecCallback enter.");
    return audiocodec_->SetCodecCallback(codecCallback);
}

int32_t AudioDecoderAdapter::SetAudioDecryptionConfig(
    const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    MEDIA_LOG_I("SetAudioDecryptionConfig enter.");
    return audiocodec_->SetAudioDecryptionConfig(keySession, svpFlag);
}

std::string AudioDecoderAdapter::StateToString(CodecState state)
{
    std::map<CodecState, std::string> stateStrMap = {
        {CodecState::UNINITIALIZED, " UNINITIALIZED"},
        {CodecState::INITIALIZED, " INITIALIZED"},
        {CodecState::FLUSHED, " FLUSHED"},
        {CodecState::RUNNING, " RUNNING"},
        {CodecState::INITIALIZING, " INITIALIZING"},
        {CodecState::STARTING, " STARTING"},
        {CodecState::STOPPING, " STOPPING"},
        {CodecState::FLUSHING, " FLUSHING"},
        {CodecState::RESUMING, " RESUMING"},
        {CodecState::RELEASING, " RELEASING"},
    };
    return stateStrMap[state];
}
}  // namespace Media
}  // namespace OHOS
