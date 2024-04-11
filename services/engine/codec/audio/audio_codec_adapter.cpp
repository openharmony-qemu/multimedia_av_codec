/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#include "audio_codec_adapter.h"
#include <malloc.h>
#include "avcodec_trace.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "media_description.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AvCodec-AudioCodecAdapter"};
constexpr uint8_t LOGD_FREQUENCY = 5;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
AudioCodecAdapter::AudioCodecAdapter(const std::string &name) : state_(CodecState::RELEASED), name_(name) {}

AudioCodecAdapter::~AudioCodecAdapter()
{
    if (worker_) {
        worker_->Release();
        worker_.reset();
        worker_ = nullptr;
    }
    callback_ = nullptr;
    if (audioCodec) {
        audioCodec->Release();
        audioCodec.reset();
        audioCodec = nullptr;
    }
    state_ = CodecState::RELEASED;
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
}

int32_t AudioCodecAdapter::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    AVCODEC_SYNC_TRACE;
    if (state_ != CodecState::RELEASED && state_ != CodecState::INITIALIZED && state_ != CodecState::INITIALIZING) {
        AVCODEC_LOGE("SetCallback failed, state = %{public}s .", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    if (!callback) {
        AVCODEC_LOGE("SetCallback failed, callback is nullptr.");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    callback_ = callback;
    AVCODEC_LOGD("SetCallback success");
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::Configure(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("state %{public}s to INITIALIZING then INITIALIZED, name:%{public}s",
        stateToString(state_).data(), name_.data());
    if (!format.ContainKey(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT)) {
        AVCODEC_LOGE("Configure failed, missing channel count key in format.");
        return AVCodecServiceErrCode::AVCS_ERR_CONFIGURE_MISMATCH_CHANNEL_COUNT;
    }

    if (!format.ContainKey(MediaDescriptionKey::MD_KEY_SAMPLE_RATE)) {
        AVCODEC_LOGE("Configure failed,missing sample rate key in format.");
        return AVCodecServiceErrCode::AVCS_ERR_MISMATCH_SAMPLE_RATE;
    }

    if (state_ != CodecState::RELEASED) {
        AVCODEC_LOGE("Configure failed, state = %{public}s .", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    
    state_ = CodecState::INITIALIZING;
    auto ret = doInit();
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        return ret;
    }

    if (state_ != CodecState::INITIALIZED) {
        AVCODEC_LOGE("Configure failed, state =%{public}s", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }

    ret = doConfigure(format);
    AVCODEC_LOGD("Configure exit");
    return ret;
}

int32_t AudioCodecAdapter::Start()
{
    AVCODEC_LOGD("Start enter");
    if (!callback_) {
        AVCODEC_LOGE("adapter start error, callback not initialized .");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    if (!audioCodec) {
        AVCODEC_LOGE("adapter start error, audio codec not initialized .");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    if (state_ == CodecState::FLUSHED) {
        AVCODEC_LOGI("Start, doResume");
        return doResume();
    }

    if (state_ != CodecState::INITIALIZED) {
        AVCODEC_LOGE("Start is incorrect, state = %{public}s .", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    AVCODEC_LOGI("state %{public}s to STARTING then RUNNING", stateToString(state_).data());
    state_ = CodecState::STARTING;
    auto ret = doStart();
    return ret;
}

int32_t AudioCodecAdapter::Stop()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("stop enter");
    if (!callback_) {
        AVCODEC_LOGE("Stop failed, call back not initialized.");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    if (state_ == CodecState::INITIALIZED || state_ == CodecState::RELEASED || state_ == CodecState::STOPPING ||
        state_ == CodecState::RELEASING) {
        AVCODEC_LOGD("Stop, state_=%{public}s", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }
    state_ = CodecState::STOPPING;
    auto ret = doStop();
    AVCODEC_LOGI("state %{public}s to INITIALIZED", stateToString(state_).data());
    state_ = CodecState::INITIALIZED;
    return ret;
}

int32_t AudioCodecAdapter::Flush()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("adapter Flush enter");
    if (!callback_) {
        AVCODEC_LOGE("adapter flush error, call back not initialized .");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    if (state_ == CodecState::FLUSHED) {
        AVCODEC_LOGW("Flush, state is already flushed, state_=%{public}s .", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }
    if (state_ != CodecState::RUNNING) {
        callback_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE);
        AVCODEC_LOGE("Flush failed, state =%{public}s", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    AVCODEC_LOGI("state %{public}s to FLUSHING then FLUSHED", stateToString(state_).data());
    state_ = CodecState::FLUSHING;
    auto ret = doFlush();
    return ret;
}

int32_t AudioCodecAdapter::Reset()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("adapter Reset enter");
    if (state_ == CodecState::RELEASED || state_ == CodecState::RELEASING) {
        AVCODEC_LOGW("adapter reset, state is already released, state =%{public}s .", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }
    if (state_ == CodecState::INITIALIZING) {
        AVCODEC_LOGW("adapter reset, state is initialized, state =%{public}s .", stateToString(state_).data());
        state_ = CodecState::RELEASED;
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }
    int32_t status = AVCodecServiceErrCode::AVCS_ERR_OK;
    if (worker_) {
        worker_->Release();
        worker_.reset();
        worker_ = nullptr;
    }
    if (audioCodec) {
        status = audioCodec->Reset();
        audioCodec.reset();
        audioCodec = nullptr;
    }
    state_ = CodecState::RELEASED;
    AVCODEC_LOGI("state %{public}s to INITIALIZED",  stateToString(state_).data());
    return status;
}

int32_t AudioCodecAdapter::Release()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("adapter Release enter");
    if (state_ == CodecState::RELEASED || state_ == CodecState::RELEASING) {
        AVCODEC_LOGW("adapter Release, state isnot completely correct, state =%{public}s .",
                     stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }

    if (state_ == CodecState::INITIALIZING) {
        AVCODEC_LOGW("adapter Release, state isnot completely correct, state =%{public}s .",
                     stateToString(state_).data());
        state_ = CodecState::RELEASING;
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }

    if (state_ == CodecState::STARTING || state_ == CodecState::RUNNING || state_ == CodecState::STOPPING) {
        AVCODEC_LOGE("adapter Release, state is running, state =%{public}s .", stateToString(state_).data());
    }
    AVCODEC_LOGI("state %{public}s to RELEASING then RELEASED", stateToString(state_).data());
    state_ = CodecState::RELEASING;
    auto ret = doRelease();
    return ret;
}

int32_t AudioCodecAdapter::NotifyEos()
{
    AVCODEC_SYNC_TRACE;
    Flush();
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::SetParameter(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    (void)format;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::GetOutputFormat(Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (audioCodec == nullptr) {
        AVCODEC_LOGE("Codec not init or nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }
    format = audioCodec->GetFormat();
    if (!format.ContainKey(MediaDescriptionKey::MD_KEY_CODEC_NAME)) {
        format.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, name_);
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::QueueInputBuffer(uint32_t index, const AVCodecBufferInfo &info, AVCodecBufferFlag flag)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "adapter %{public}s queue buffer enter,index:%{public}u",
        name_.data(), index);
    if (!audioCodec) {
        AVCODEC_LOGE("adapter QueueInputBuffer error, audio codec not initialized .");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    if (!callback_) {
        AVCODEC_LOGE("adapter queue input buffer error,index:%{public}u, call back not initialized .", index);
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    if (info.size < 0) {
        AVCODEC_LOGE("size could not less than 0,size value:%{public}d.", info.size);
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }

    if (info.offset < 0) {
        AVCODEC_LOGE("offset could not less than 0,offset value:%{public}d.", info.offset);
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }
    auto result = worker_->GetInputBufferInfo(index);
    if (result == nullptr) {
        AVCODEC_LOGE("getMemory failed");
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }

    if (result->GetStatus() != BufferStatus::OWEN_BY_CLIENT) {
        AVCODEC_LOGE("GetStatus failed");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    if (result->CheckIsUsing()) {
        AVCODEC_LOGE("input buffer now is already QueueInputBuffer,please don't do it again.");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    if ((uint32_t) info.size > result->GetBufferSize()) {
        AVCODEC_LOGE("Size could not lager than buffersize, please check input size %{public}d.", info.size);
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
    }

    result->SetUsing();
    result->SetBufferAttr(info);
    if (flag == AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
        result->SetEos(true);
    }
    worker_->PushInputData(index);
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::ReleaseOutputBuffer(uint32_t index)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "adapter %{public}s release buffer,index:%{public}u", name_.data(), index);
    if (!callback_) {
        AVCODEC_LOGE("adapter release buffer error,index:%{public}u, call back not initialized .", index);
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    if (!audioCodec) {
        AVCODEC_LOGE("adapter release buffer error, audio codec not initialized .");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    auto outBufferInfo = worker_->GetOutputBufferInfo(index);
    if (outBufferInfo == nullptr) {
        AVCODEC_LOGE("release buffer failed,index=%{public}u error", index);
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }

    if (outBufferInfo->GetStatus() != BufferStatus::OWEN_BY_CLIENT) {
        AVCODEC_LOGE("output buffer status now is IDLE, could not released");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    auto outBuffer = worker_->GetOutputBuffer();
    if (outBuffer == nullptr) {
        AVCODEC_LOGE("release buffer failed,index=%{public}u error", index);
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }
    bool result = outBuffer->ReleaseBuffer(index);
    if (!result) {
        AVCODEC_LOGE("release buffer failed");
        return AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY;
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::doInit()
{
    AVCODEC_SYNC_TRACE;
    if (name_.empty()) {
        state_ = CodecState::RELEASED;
        AVCODEC_LOGE("doInit failed, because name is empty");
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }

    AVCODEC_LOGD("adapter doInit, codec name:%{public}s", name_.data());
    audioCodec = AudioBaseCodec::make_sharePtr(name_);
    if (audioCodec == nullptr) {
        state_ = CodecState::RELEASED;
        AVCODEC_LOGE("Initlize failed, because create codec failed. name: %{public}s.", name_.data());
        return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
    }
    AVCODEC_LOGD("adapter doInit, state from %{public}s to INITIALIZED",
        stateToString(state_).data());
    state_ = CodecState::INITIALIZED;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::doConfigure(const Format &format)
{
    AVCODEC_SYNC_TRACE;
    if (state_ != CodecState::INITIALIZED) {
        AVCODEC_LOGE("adapter configure failed because state is incorrect,state:%{public}d.",
                     static_cast<int>(state_.load()));
        state_ = CodecState::RELEASED;
        AVCODEC_LOGE("state_=%{public}s", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
    int32_t ret = audioCodec->Init(format);
    if (ret != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("configure failed, because codec init failed,error:%{public}d.", static_cast<int>(ret));
        state_ = CodecState::RELEASED;
        return ret;
    }
    return ret;
}

int32_t AudioCodecAdapter::doStart()
{
    AVCODEC_SYNC_TRACE;
    if (state_ != CodecState::STARTING) {
        AVCODEC_LOGE("doStart failed, state = %{public}s", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }

    AVCODEC_LOGD("adapter doStart, state from %{public}s to RUNNING", stateToString(state_).data());
    state_ = CodecState::RUNNING;
    worker_ = std::make_shared<AudioCodecWorker>(audioCodec, callback_);
    worker_->Start();
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::doResume()
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGD("adapter doResume, state from %{public}s to RESUMING", stateToString(state_).data());
    state_ = CodecState::RESUMING;
    worker_->Resume();
    AVCODEC_LOGD("adapter doResume, state from %{public}s to RUNNING", stateToString(state_).data());
    state_ = CodecState::RUNNING;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::doStop()
{
    AVCODEC_SYNC_TRACE;
    if (state_ == CodecState::RELEASING) {
        AVCODEC_LOGW("adapter doStop, state is already release, state_=%{public}s .", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }

    if (state_ != CodecState::STOPPING) {
        AVCODEC_LOGE("doStop failed, state =%{public}s", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    worker_->Stop();
    int32_t status = audioCodec->Flush();
    if (status != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("flush status=%{public}d", static_cast<int>(status));
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::doFlush()
{
    AVCODEC_SYNC_TRACE;
    if (state_ != CodecState::FLUSHING) {
        AVCODEC_LOGE("doFlush failed, state_=%{public}s", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    worker_->Pause();
    int32_t status = audioCodec->Flush();
    state_ = CodecState::FLUSHED;
    if (status != AVCodecServiceErrCode::AVCS_ERR_OK) {
        AVCODEC_LOGE("status=%{public}d", static_cast<int>(status));
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AudioCodecAdapter::doRelease()
{
    AVCODEC_SYNC_TRACE;
    if (state_ == CodecState::RELEASED) {
        AVCODEC_LOGW("adapter doRelease, state already released, state_=%{public}s.", stateToString(state_).data());
        return AVCodecServiceErrCode::AVCS_ERR_OK;
    }
    if (worker_ != nullptr) {
        worker_->Release();
        worker_.reset();
        worker_ = nullptr;
    }
    if (audioCodec != nullptr) {
        audioCodec->Release();
        audioCodec.reset();
        audioCodec = nullptr;
    }
    AVCODEC_LOGD_LIMIT(LOGD_FREQUENCY, "adapter doRelease, state from %{public}s to RELEASED",
        stateToString(state_).data());
    state_ = CodecState::RELEASED;
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

std::string_view AudioCodecAdapter::stateToString(CodecState state)
{
    std::map<CodecState, std::string_view> stateStrMap = {
        {CodecState::RELEASED, " RELEASED"},         {CodecState::INITIALIZED, " INITIALIZED"},
        {CodecState::FLUSHED, " FLUSHED"},           {CodecState::RUNNING, " RUNNING"},
        {CodecState::INITIALIZING, " INITIALIZING"}, {CodecState::STARTING, " STARTING"},
        {CodecState::STOPPING, " STOPPING"},         {CodecState::FLUSHING, " FLUSHING"},
        {CodecState::RESUMING, " RESUMING"},         {CodecState::RELEASING, " RELEASING"},
    };
    return stateStrMap[state];
}
} // namespace MediaAVCodec
} // namespace OHOS