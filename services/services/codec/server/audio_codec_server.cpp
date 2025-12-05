/*
 * Copyright (C) 2023-2025 Huawei Device Co., Ltd.
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

#include "audio_codec_server.h"
#include <functional>
#include <limits>
#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>
#include "avcodec_codec_name.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "buffer/avbuffer.h"
#include "codec_ability_singleton.h"
#include "codec_factory.h"
#include "media_description.h"
#include "meta/meta_key.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AudioCodecServer"};

const std::map<OHOS::MediaAVCodec::AudioCodecServer::CodecStatus, std::string> CODEC_STATE_MAP = {
    {OHOS::MediaAVCodec::AudioCodecServer::UNINITIALIZED, "uninitialized"},
    {OHOS::MediaAVCodec::AudioCodecServer::INITIALIZED, "initialized"},
    {OHOS::MediaAVCodec::AudioCodecServer::CONFIGURED, "configured"},
    {OHOS::MediaAVCodec::AudioCodecServer::RUNNING, "running"},
    {OHOS::MediaAVCodec::AudioCodecServer::FLUSHED, "flushed"},
    {OHOS::MediaAVCodec::AudioCodecServer::END_OF_STREAM, "EOS"},
    {OHOS::MediaAVCodec::AudioCodecServer::ERROR, "error"},
};

int32_t GetAudioCodecName(const OHOS::MediaAVCodec::AVCodecType type, std::string &name)
{
    using namespace OHOS::MediaAVCodec;
    if (name.compare(AVCodecCodecName::AUDIO_DECODER_API9_AAC_NAME) == 0) {
        name = AVCodecCodecName::AUDIO_DECODER_AAC_NAME;
    } else if (name.compare(AVCodecCodecName::AUDIO_ENCODER_API9_AAC_NAME) == 0) {
        name = AVCodecCodecName::AUDIO_ENCODER_AAC_NAME;
    }
    if (name.find("Audio") != name.npos) {
        if ((name.find("Decoder") != name.npos && type != AVCODEC_TYPE_AUDIO_DECODER) ||
            (name.find("Encoder") != name.npos && type != AVCODEC_TYPE_AUDIO_ENCODER)) {
            AVCODEC_LOGE("AudioCodec name:%{public}s invalid", name.c_str());
            return AVCS_ERR_INVALID_OPERATION;
        }
    }
    return AVCS_ERR_OK;
}

} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
std::shared_ptr<AudioCodecServer> AudioCodecServer::Create()
{
    std::shared_ptr<AudioCodecServer> server = std::make_shared<AudioCodecServer>();

    int32_t ret = server->InitServer();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Server init failed, ret: %{public}d!", ret);
    return server;
}

AudioCodecServer::AudioCodecServer()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioCodecServer::~AudioCodecServer()
{
    codecBase_ = nullptr;
    avBufCallback_ = nullptr;
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);

    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AudioCodecServer::InitServer()
{
    return AVCS_ERR_OK;
}

int32_t AudioCodecServer::Init(AVCodecType type, bool isMimeType, const std::string &name,
                               Meta &callerInfo, API_VERSION apiVersion)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
    CHECK_AND_RETURN_RET_LOG(name != nullptr, 
    codecType_ = type;
    codecName_ = name;
    codecMime_ = isMimeType ? name : CodecAbilitySingleton::GetInstance().GetMimeByCodecName(name);
    
    int32_t ret = isMimeType ? InitByMime(callerInfo, apiVersion) : InitByName(callerInfo, apiVersion);

    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION,  "name is nullptr");
                             "Init failed. isMimeType:(%{public}d), name:(%{public}s), error:(%{public}d)",
                             isMimeType, name.c_str(), ret);
    shareBufCallback_ = std::make_shared<CodecBaseCallback>(shared_from_this());
    ret = codecBase_->SetCallback(shareBufCallback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "SetCallback failed");

    avBufCallback_ = std::make_shared<VCodecBaseCallback>(shared_from_this());
    ret = codecBase_->SetCallback(avBufCallback_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "SetCallback failed");
    AVCODEC_LOGI("Create codec %{public}s by %{public}s success", codecName_.c_str(),
                 (isMimeType ? "mime" : "name"));
    StatusChanged(INITIALIZED);
    return AVCS_ERR_OK;
}

int32_t AudioCodecServer::SetLowPowerPlayerMode(bool isLpp)
{
    return 0;
}

int32_t AudioCodecServer::InitByName(Meta &callerInfo, API_VERSION apiVersion)
{
    int32_t ret = GetAudioCodecName(codecType_, codecName_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "CodecName get failed");

    codecBase_ = CodecFactory::Instance().CreateCodecByName(codecName_, apiVersion);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "CodecBase is nullptr");
    return codecBase_->Init(callerInfo);
}

int32_t AudioCodecServer::InitByMime(Meta &callerInfo, API_VERSION apiVersion)
{
    int32_t ret = AVCS_ERR_NO_MEMORY;
    bool isEncoder = (codecType_ == AVCODEC_TYPE_AUDIO_ENCODER);
    std::vector<std::string> nameArray = CodecFactory::Instance().GetCodecNameArrayByMime(codecName_, isEncoder);
    std::vector<std::string>::iterator iter;
    for (iter = nameArray.begin(); iter != nameArray.end(); ++iter) {
        ret = AVCS_ERR_NO_MEMORY;
        codecBase_ = CodecFactory::Instance().CreateCodecByName(*iter, apiVersion);
        CHECK_AND_CONTINUE_LOG(codecBase_ != nullptr, "Skip creation failure. name:(%{public}s)",
                               iter->c_str());
        ret = codecBase_->Init(callerInfo);
        CHECK_AND_CONTINUE_LOG(ret == AVCS_ERR_OK, "Skip initialization failure. name:(%{public}s)",
                               iter->c_str());
        codecName_ = *iter;
        break;
    }
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "CodecBase is nullptr");
    return iter == nameArray.end() ? ret : AVCS_ERR_OK;
}

int32_t AudioCodecServer::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t paramCheckRet = AVCS_ERR_OK;
    int32_t ret = codecBase_->Configure(format);
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
        return ret;
    }
    StatusChanged(CONFIGURED);
    return paramCheckRet;
}

int32_t AudioCodecServer::SetCustomBuffer(std::shared_ptr<AVBuffer> buffer)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == CONFIGURED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->SetCustomBuffer(buffer);
}

int32_t AudioCodecServer::Start()
{
    SetFreeStatus(false);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == FLUSHED || status_ == CONFIGURED, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Start();
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
    } else {
        StatusChanged(RUNNING);
    }
    return ret;
}

int32_t AudioCodecServer::Stop()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOGW(status_ != CONFIGURED, AVCS_ERR_OK, "Already in %{public}s state",
                              GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM || status_ == FLUSHED,
                             AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    codecBase_->Stop(); // perhaps need check?
    StatusChanged(CONFIGURED);
    return AVCS_ERR_OK;
}

int32_t AudioCodecServer::Flush()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOGW(status_ != FLUSHED, AVCS_ERR_OK, "Already in %{public}s state",
                              GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    codecBase_->Flush();
    StatusChanged(FLUSHED);
    return AVCS_ERR_OK;
}

int32_t AudioCodecServer::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->NotifyEos();
    if (ret == AVCS_ERR_OK) {
        CodecStatus newStatus = END_OF_STREAM;
        StatusChanged(newStatus);
    }
    return ret;
}

int32_t AudioCodecServer::Reset()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret = codecBase_->Reset();
    CodecStatus newStatus = (ret == AVCS_ERR_OK ? INITIALIZED : ERROR);
    StatusChanged(newStatus);
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
    }
    lastErrMsg_.clear();

    return ret;
}

int32_t AudioCodecServer::Release()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret = codecBase_->Release();

    codecBase_ = nullptr;
    avBufCallback_ = nullptr;
    return ret;
}

int32_t AudioCodecServer::GetChannelId(int32_t &channelId)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->GetChannelId(channelId);
    return ret;
}

int32_t AudioCodecServer::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_lock<std::shared_mutex> freeLock(freeMutex_);
    if (isFree_) {
        AVCODEC_LOGE("In invalid state, free out");
        return AVCS_ERR_INVALID_STATE;
    }

    int32_t ret = AVCS_ERR_OK;
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        ret = QueueInputBufferIn(index, info, flag);
        if (ret == AVCS_ERR_OK) {
            CodecStatus newStatus = END_OF_STREAM;
            StatusChanged(newStatus);
        }
    } else {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        ret = QueueInputBufferIn(index, info, flag);
    }
    return ret;
}

int32_t AudioCodecServer::QueueInputBufferIn(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    int32_t ret = AVCS_ERR_OK;
    CHECK_AND_RETURN_RET_LOG(
        status_ == RUNNING || ((inputParamTask_ != nullptr) && status_ == END_OF_STREAM),
        AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    if (codecCb_ != nullptr) {
        ret = codecBase_->QueueInputBuffer(index, info, flag);
    }
    return ret;
}

int32_t AudioCodecServer::QueueInputBuffer(uint32_t index)
{
    (void)index;
    return AVCS_ERR_UNSUPPORT;
}

int32_t AudioCodecServer::QueueInputParameter(uint32_t index)
{
    (void)index;
    return AVCS_ERR_UNSUPPORT;
}

int32_t AudioCodecServer::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != UNINITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    
    return codecBase_->GetOutputFormat(format);
}

int32_t AudioCodecServer::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> freeLock(freeMutex_);
    if (isFree_) {
        AVCODEC_LOGE("In invalid state, free out");
        return AVCS_ERR_INVALID_STATE;
    }
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());

    return ReleaseOutputBufferOfCodec(index, render);
}

int32_t AudioCodecServer::ReleaseOutputBufferOfCodec(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret;
    if (render) {
        ret = codecBase_->RenderOutputBuffer(index);
    } else {
        ret = codecBase_->ReleaseOutputBuffer(index);
    }
    return ret;
}

int32_t AudioCodecServer::RenderOutputBufferAtTime(uint32_t index, [[maybe_unused]]int64_t renderTimestampNs)
{
    return ReleaseOutputBuffer(index, true);
}

int32_t AudioCodecServer::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != INITIALIZED && status_ != CONFIGURED, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    return codecBase_->SetParameter(format);
}

int32_t AudioCodecServer::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> cbLock(cbMutex_);
    codecCb_ = callback;
    return AVCS_ERR_OK;
}

int32_t AudioCodecServer::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> cbLock(cbMutex_);
    videoCb_ = callback;
    return AVCS_ERR_OK;
}

int32_t AudioCodecServer::SetCodecCallback(const std::shared_ptr<MediaCodecCallback> &codecCallback)
{
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    avBufCallback_ = codecCallback;
    return codecBase_->SetCallback(codecCallback);
}

int32_t AudioCodecServer::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    (void)callback;
    return AVCS_ERR_UNSUPPORT;
}

int32_t AudioCodecServer::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    (void)callback;
    return AVCS_ERR_UNSUPPORT;
}

int32_t AudioCodecServer::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(
        status_ == CONFIGURED || status_ == RUNNING || status_ == FLUSHED || status_ == END_OF_STREAM,
        AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->GetInputFormat(format);
}

int32_t AudioCodecServer::ChangePlugin(const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->ChangePlugin(mime, isEncoder, meta);
}

void AudioCodecServer::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    CHECK_AND_RETURN_LOG(codecBase_ != nullptr, "Codecbase is nullptr");
    return codecBase_->SetDumpInfo(isDump, instanceId);
}

inline const std::string &AudioCodecServer::GetStatusDescription(CodecStatus status)
{
    if (status < UNINITIALIZED || status >= ERROR) {
        return CODEC_STATE_MAP.at(ERROR);
    }
    return CODEC_STATE_MAP.at(status);
}

inline void AudioCodecServer::StatusChanged(CodecStatus newStatus)
{
    if (status_ == newStatus) {
        return;
    }
    if (status_ == ERROR && videoCb_ != nullptr) {
        videoCb_->OnError(AVCODEC_ERROR_FRAMEWORK_FAILED, AVCS_ERR_INVALID_STATE);
    }
    AVCODEC_LOGI("Status %{public}s -> %{public}s", GetStatusDescription(status_).data(),
                 GetStatusDescription(newStatus).data());
    status_ = newStatus;
}

void AudioCodecServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    lastErrMsg_ = AVCSErrorToString(static_cast<AVCodecServiceErrCode>(errorCode));
    AVCODEC_LOGW("%{public}s", lastErrMsg_.c_str());
    if (codecCb_ != nullptr) {
        codecCb_->OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
    }
}

void AudioCodecServer::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (videoCb_ != nullptr) {
        videoCb_->OnOutputFormatChanged(format);
    }
    if (codecCb_ != nullptr) {
        codecCb_->OnOutputFormatChanged(format);
    }
}

void AudioCodecServer::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnInputBufferAvailable(index, buffer);
}

void AudioCodecServer::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                               std::shared_ptr<AVSharedMemory> buffer)
{
    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnOutputBufferAvailable(index, info, flag, buffer);
}

void AudioCodecServer::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    std::shared_lock<std::shared_mutex> lock(cbMutex_);
}

void AudioCodecServer::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG(buffer != nullptr, "buffer is nullptr!");

    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    CHECK_AND_RETURN_LOG(videoCb_ != nullptr, "videoCb_ is nullptr!");
    videoCb_->OnOutputBufferAvailable(index, buffer);
}

CodecBaseCallback::CodecBaseCallback(const std::shared_ptr<AudioCodecServer> &codec) : codec_(codec)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecBaseCallback::~CodecBaseCallback()
{
    codec_ = nullptr;
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecBaseCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (codec_ != nullptr) {
        codec_->OnError(errorType, errorCode);
    }
}

void CodecBaseCallback::OnOutputFormatChanged(const Format &format)
{
    if (codec_ != nullptr) {
        codec_->OnOutputFormatChanged(format);
    } else {
        AVCODEC_LOGI("CodecBaseCallback receive output format changed but codec is nullptr");
    }
}

void CodecBaseCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    if (codec_ != nullptr) {
        codec_->OnInputBufferAvailable(index, buffer);
    }
}

void CodecBaseCallback::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                                std::shared_ptr<AVSharedMemory> buffer)
{
    if (codec_ != nullptr) {
        codec_->OnOutputBufferAvailable(index, info, flag, buffer);
    }
}

VCodecBaseCallback::VCodecBaseCallback(const std::shared_ptr<AudioCodecServer> &codec) : codec_(codec)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VCodecBaseCallback::~VCodecBaseCallback()
{
    codec_ = nullptr;
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void VCodecBaseCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (codec_ != nullptr) {
        codec_->OnError(errorType, errorCode);
    }
}

void VCodecBaseCallback::OnOutputFormatChanged(const Format &format)
{
    if (codec_ != nullptr) {
        codec_->OnOutputFormatChanged(format);
    } else {
        AVCODEC_LOGE("receive output format changed, but codec is nullptr");
    }
}

void VCodecBaseCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (codec_ != nullptr) {
        codec_->OnInputBufferAvailable(index, buffer);
    }
}

void VCodecBaseCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    if (codec_ != nullptr) {
        codec_->OnOutputBufferAvailable(index, buffer);
    }
}

int32_t AudioCodecServer::Configure(const std::shared_ptr<Media::Meta> &meta)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(meta != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    CHECK_AND_RETURN_RET_LOG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret = codecBase_->Configure(meta);

    CodecStatus newStatus = (ret == AVCS_ERR_OK ? CONFIGURED : ERROR);
    StatusChanged(newStatus);
    return ret;
}

int32_t AudioCodecServer::SetParameter(const std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(parameter != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    CHECK_AND_RETURN_RET_LOG(status_ != INITIALIZED && status_ != CONFIGURED, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    return codecBase_->SetParameter(parameter);
}

int32_t AudioCodecServer::GetOutputFormat(std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != UNINITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "codecBase is nullptr");
    return codecBase_->GetOutputFormat(parameter);
}

int32_t AudioCodecServer::SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(bufferQueueProducer != nullptr, AVCS_ERR_NO_MEMORY, "bufferQueueProducer is nullptr");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "codecBase_ is nullptr");
    return codecBase_->SetOutputBufferQueue(bufferQueueProducer);
}
int32_t AudioCodecServer::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "codecBase_ is nullptr");
    return codecBase_->Prepare();
}
sptr<Media::AVBufferQueueProducer> AudioCodecServer::GetInputBufferQueue()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, nullptr, "codecBase_ is nullptr");
    return codecBase_->GetInputBufferQueue();
}

sptr<Media::AVBufferQueueConsumer> AudioCodecServer::GetInputBufferQueueConsumer()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    return codecBase_ != nullptr ? codecBase_->GetInputBufferQueueConsumer() : nullptr;
}

sptr<Media::AVBufferQueueProducer> AudioCodecServer::GetOutputBufferQueueProducer()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    return codecBase_ != nullptr ? codecBase_->GetOutputBufferQueueProducer() : nullptr;
}

void AudioCodecServer::ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(codecBase_ != nullptr, "ProcessInputBufferInner codecBase is nullptr");
    return codecBase_->ProcessInputBufferInner(isTriggeredByOutPort, isFlushed, bufferStatus);
}

void AudioCodecServer::ProcessInputBuffer()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(codecBase_ != nullptr, "codecBase_ is nullptr");
    return codecBase_->ProcessInputBuffer();
}

bool AudioCodecServer::CheckRunning()
{
    if (status_ == AudioCodecServer::RUNNING) {
        return true;
    }
    return false;
}

#ifdef SUPPORT_DRM
int32_t AudioCodecServer::SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
                                                   const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_LOGI("AudioCodecServer::SetAudioDecryptionConfig");
    if (codecBase_ == nullptr) {
        AVCODEC_LOGE("codecBase_ is nullptr");
        return AVCS_ERR_NO_MEMORY;
    }
    return codecBase_->SetAudioDecryptionConfig(keySession, svpFlag);
}
#endif

void AudioCodecServer::SetFreeStatus(bool isFree)
{
    std::lock_guard<std::shared_mutex> lock(freeMutex_);
    isFree_ = isFree;
}

} // namespace MediaAVCodec
} // namespace OHOS
