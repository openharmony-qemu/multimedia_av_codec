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
#include "avcodec_audio_codec_inner_impl.h"
#include "i_avcodec_service.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "avcodec_codec_name.h"
#include "codec_server.h"
#include "drm_i_keysession_service.h"

namespace OHOS {
namespace MediaAVCodec {
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "AVCodecAudioCodecInnerImpl"};
constexpr size_t DEFAULT_OUTPUT_BUFFER_NUM = 4;
}

class OutputBufferAvailableListener : public Media::IConsumerListener {
public:
    explicit OutputBufferAvailableListener(std::shared_ptr<IConsumerListener> consumerListener)
        : consumerListener_(consumerListener)
    {}

    void OnBufferAvailable() override
    {
        auto realPtr = consumerListener_.lock();
        if (realPtr != nullptr) {
            realPtr->OnBufferAvailable();
        } else {
            AVCODEC_LOGW("AVCodecAudioCodecSyncInnerImpl was released, can not callback OnBufferAvailable");
        }
    }

private:
    std::weak_ptr<IConsumerListener> consumerListener_;
};

std::shared_ptr<AVCodecAudioCodec> AudioCodecFactory::CreateByMime(const std::string &mime, bool isEncoder)
{
    AVCODEC_SYNC_TRACE;
    std::shared_ptr<AVCodecAudioCodecInnerImpl> impl = std::make_shared<AVCodecAudioCodecInnerImpl>();
    AVCodecType type = AVCODEC_TYPE_AUDIO_DECODER;
    if (isEncoder) {
        type = AVCODEC_TYPE_AUDIO_ENCODER;
    }
    int32_t ret = impl->Init(type, true, mime);
    CHECK_AND_RETURN_RET_LOG(ret == AVCodecServiceErrCode::AVCS_ERR_OK,
        nullptr, "failed to init AVCodecAudioCodecInnerImpl");
    return impl;
}

std::shared_ptr<AVCodecAudioCodec> AudioCodecFactory::CreateByName(const std::string &name)
{
    AVCODEC_SYNC_TRACE;
    std::shared_ptr<AVCodecAudioCodecInnerImpl> impl = std::make_shared<AVCodecAudioCodecInnerImpl>();
    std::string codecMimeName = name;
    if (name.compare(AVCodecCodecName::AUDIO_DECODER_API9_AAC_NAME) == 0) {
        codecMimeName = AVCodecCodecName::AUDIO_DECODER_AAC_NAME;
    } else if (name.compare(AVCodecCodecName::AUDIO_ENCODER_API9_AAC_NAME) == 0) {
        codecMimeName = AVCodecCodecName::AUDIO_ENCODER_AAC_NAME;
    }
    AVCodecType type = AVCODEC_TYPE_AUDIO_ENCODER;
    if (codecMimeName.find("Encoder") != codecMimeName.npos) {
        type = AVCODEC_TYPE_AUDIO_ENCODER;
    } else {
        type = AVCODEC_TYPE_AUDIO_DECODER;
    }
    int32_t ret = impl->Init(type, false, name);
    CHECK_AND_RETURN_RET_LOG(ret == AVCodecServiceErrCode::AVCS_ERR_OK,
        nullptr, "failed to init AVCodecAudioCodecInnerImpl");
    return impl;
}

AVCodecAudioCodecInnerImpl::AVCodecAudioCodecInnerImpl()
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecAudioCodecInnerImpl::~AVCodecAudioCodecInnerImpl()
{
    codecService_ = nullptr;
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t AVCodecAudioCodecInnerImpl::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    AVCODEC_SYNC_TRACE;
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Init");
    Format format;
    codecService_ = CodecServer::Create();
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_NO_MEMORY,
                             "failed to create codec service");
    int32_t ret = codecService_->Init(type, isMimeType, name, *format.GetMeta(), API_VERSION::API_VERSION_11);
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::Configure(const std::shared_ptr<Media::Meta> &meta)
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Configure");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    bool enableSyncMode = false;
    meta->GetData(Tag::AV_CODEC_ENABLE_SYNC_MODE, enableSyncMode);
    if (enableSyncMode) {
        AVCODEC_LOGI("AVCodecAudioCodecInnerImpl create SyncCodecAdapter");
        syncCodecAdapter_ = std::make_shared<SyncCodecAdapter>(DEFAULT_OUTPUT_BUFFER_NUM);
        int32_t ret = codecService_->SetOutputBufferQueue(syncCodecAdapter_->GetProducer());
        if (ret != static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK)) {
            AVCODEC_LOGW("AVCodecAudioCodecInnerImpl set sync mode set failed, ret %{public}d", ret);
            syncCodecAdapter_.reset();
            return ret;
        }
    }
    int32_t ret = codecService_->Configure(meta);
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::SetOutputBufferQueue(
    const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl SetOutputBufferQueue");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    if (syncCodecAdapter_ != nullptr) {
        return static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK);
    }
    int32_t ret = codecService_->SetOutputBufferQueue(bufferQueueProducer);
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::Prepare()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Prepare");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->Prepare();
    if (syncCodecAdapter_ != nullptr && ret == static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK)) {
        ret = syncCodecAdapter_->Prepare(codecService_->GetInputBufferQueue());
    }
    return ret;
}

sptr<Media::AVBufferQueueProducer> AVCodecAudioCodecInnerImpl::GetInputBufferQueue()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl GetInputBufferQueue");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, nullptr,
                             "service died");
    return codecService_->GetInputBufferQueue();
}

sptr<Media::AVBufferQueueConsumer> AVCodecAudioCodecInnerImpl::GetInputBufferQueueConsumer()
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl GetInputBufferQueueConsumer");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, nullptr, "GetInputBufferQueueConsumer service died");
    return codecService_->GetInputBufferQueueConsumer();
}

sptr<Media::AVBufferQueueProducer> AVCodecAudioCodecInnerImpl::GetOutputBufferQueueProducer()
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl GetOutputBufferQueueProducer");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, nullptr, "GetOutputBufferQueueProducer service died");
    return codecService_->GetOutputBufferQueueProducer();
}

void AVCodecAudioCodecInnerImpl::ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed,
    uint32_t &bufferStatus)
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl ProcessInputBufferInner");
    CHECK_AND_RETURN_LOG(codecService_ != nullptr, "ProcessInputBufferInner service died");
    codecService_->ProcessInputBufferInner(isTriggeredByOutPort, isFlushed, bufferStatus);
}

int32_t AVCodecAudioCodecInnerImpl::Start()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Start");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->Start();
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::Stop()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Stop");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->Stop();
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::Flush()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Flush");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->Flush();
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::Reset()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Reset");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->Reset();
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::Release()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl Release");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->Release();
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::NotifyEos()
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl NotifyEos");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->NotifyEos();
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::SetParameter(const std::shared_ptr<Media::Meta> &parameter)
{
    AVCODEC_LOGI("AVCodecAudioCodecInnerImpl SetParameter");
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->SetParameter(parameter);
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::GetOutputFormat(std::shared_ptr<Media::Meta> &parameter)
{
    AVCODEC_LOGD_LIMIT(10, "AVCodecAudioCodecInnerImpl GetOutputFormat");   // limit10
    CHECK_AND_RETURN_RET_LOG(codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION,
                             "service died");
    int32_t ret = codecService_->GetOutputFormat(parameter);
    return ret;
}

int32_t AVCodecAudioCodecInnerImpl::ChangePlugin(
    const std::string &mime, bool isEncoder, const std::shared_ptr<Media::Meta> &meta)
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl ChangePlugin");
    CHECK_AND_RETURN_RET_LOG(
        codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->ChangePlugin(mime, isEncoder, meta);
}

int32_t AVCodecAudioCodecInnerImpl::SetCodecCallback(const std::shared_ptr<MediaCodecCallback> &codecCallback)
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl SetCodecCallback");
    CHECK_AND_RETURN_RET_LOG(
        codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->SetCodecCallback(codecCallback);
}

int32_t AVCodecAudioCodecInnerImpl::SetAudioDecryptionConfig(
    const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl SetAudioDecryptionConfig");
    CHECK_AND_RETURN_RET_LOG(
        codecService_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION, "service died");
    return codecService_->SetAudioDecryptionConfig(keySession, svpFlag);
}

void AVCodecAudioCodecInnerImpl::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl SetDumpInfo");
    CHECK_AND_RETURN_LOG(codecService_ != nullptr, "service died");
    return codecService_->SetDumpInfo(isDump, instanceId);
}

void AVCodecAudioCodecInnerImpl::ProcessInputBuffer()
{
    AVCODEC_LOGD("AVCodecAudioCodecInnerImpl ProcessInputBuffer");
    CHECK_AND_RETURN_LOG(codecService_ != nullptr, "service died");
    codecService_->ProcessInputBuffer();
}

int32_t AVCodecAudioCodecInnerImpl::QueryInputBuffer(uint32_t *index, int32_t bufferSize, int64_t timeoutUs)
{
    return syncCodecAdapter_ != nullptr ? syncCodecAdapter_->QueryInputBuffer(index, bufferSize, timeoutUs)
                                        : AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
}

std::shared_ptr<AVBuffer> AVCodecAudioCodecInnerImpl::GetInputBuffer(uint32_t index)
{
    return syncCodecAdapter_ != nullptr ? syncCodecAdapter_->GetInputBuffer(index) : nullptr;
}

std::shared_ptr<AVBuffer> AVCodecAudioCodecInnerImpl::GetOutputBuffer(int64_t timeoutUs)
{
    return syncCodecAdapter_ != nullptr ? syncCodecAdapter_->GetOutputBuffer(timeoutUs) : nullptr;
}

int32_t AVCodecAudioCodecInnerImpl::PushInputBuffer(uint32_t index, bool available)
{
    return syncCodecAdapter_ != nullptr ? syncCodecAdapter_->PushInputBuffer(index, available)
                                        : AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
}

int32_t AVCodecAudioCodecInnerImpl::ReleaseOutputBuffer(const std::shared_ptr<AVBuffer> &buffer)
{
    return syncCodecAdapter_ != nullptr ? syncCodecAdapter_->ReleaseOutputBuffer(buffer)
                                        : AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL;
}

AVCodecAudioCodecInnerImpl::SyncCodecAdapter::SyncCodecAdapter(size_t outputBufferSize)
    : init_(false), inputIndex_(0), outputAvaliableNum_(0)
{
    AVCODEC_LOGI("SyncCodecAdapter Create, outputBufferSize %zu", outputBufferSize);

    innerBufferQueue_ = Media::AVBufferQueue::Create(
        outputBufferSize, Media::MemoryType::SHARED_MEMORY, "AVCodecAudioCodecSyncInnerImpl");
    if (innerBufferQueue_ != nullptr) {
        bufferQueueConsumer_ = innerBufferQueue_->GetConsumer();
    } else {
        AVCODEC_LOGE("SyncCodecAdapter Create innerBufferQueue failed");
    }
}

AVCodecAudioCodecInnerImpl::SyncCodecAdapter::~SyncCodecAdapter()
{
    AVCODEC_LOGI("~SyncCodecAdapter");
    int32_t index = 0;
    if (bufferQueueProducer_ != nullptr) {
        for (auto &it : inputBuffers_) {
            if (it == nullptr) {
                continue;
            }
            Status ret = bufferQueueProducer_->PushBuffer(it, false);
            if (ret != Status::OK) {
                AVCODEC_LOGE("~SyncCodecAdapter release inputBuffer %{public}d failed, ret = %{public}d", index, ret);
            }
            ++index;
        }
    }
    inputBuffers_.clear();

    index = 0;
    if (bufferQueueConsumer_ != nullptr) {
        for (auto &it : outputBuffers_) {
            Status ret = bufferQueueConsumer_->ReleaseBuffer(it.second);
            if (ret != Status::OK) {
                AVCODEC_LOGE("~SyncCodecAdapter release outputBuffer %{public}d failed, ret = %{public}d", index, ret);
            }
            ++index;
        }
    }
    outputBuffers_.clear();
}

int32_t AVCodecAudioCodecInnerImpl::SyncCodecAdapter::Prepare(
    const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    AVCODEC_LOGI("SyncCodecAdapter Prepare, init %{public}d", static_cast<int32_t>(init_));

    if (!init_) {
        if (bufferQueueConsumer_ != nullptr && bufferQueueProducer != nullptr) {
            bufferQueueProducer_ = bufferQueueProducer;
            inputBuffers_.resize(bufferQueueProducer_->GetQueueSize());

            sptr<IConsumerListener> listener = new OutputBufferAvailableListener(shared_from_this());
            Status ret = bufferQueueConsumer_->SetBufferAvailableListener(listener);
            if (ret != Status::OK) {
                AVCODEC_LOGE(
                    "SyncCodecAdapter SetBufferAvailableListener failed, ret %{public}d", static_cast<int32_t>(ret));
                return StatusToAVCodecServiceErrCode(ret);
            }

            init_ = true;
        } else {
            AVCODEC_LOGE("SyncCodecAdapter Prepare failed, init %{public}d, bufferQueueConsumer %{public}d, "
                         "bufferQueueProducer %{public}d",
                static_cast<int32_t>(init_),
                static_cast<int32_t>(bufferQueueConsumer_ != nullptr),
                static_cast<int32_t>(bufferQueueProducer_ != nullptr));
            return AVCodecServiceErrCode::AVCS_ERR_UNKNOWN;
        }
    }
    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

int32_t AVCodecAudioCodecInnerImpl::SyncCodecAdapter::QueryInputBuffer(
    uint32_t *index, int32_t bufferSize, int64_t timeoutUs)
{
    if (!init_) {
        AVCODEC_LOGW("SyncCodecAdapter QueryInputBuffer do not work before prepare");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }

    if (index == nullptr) {
        AVCODEC_LOGW("SyncCodecAdapter QueryInputBuffer failed, input indexPtr is nullptr");
        return AVCodecServiceErrCode::AVCS_ERR_INPUT_DATA_ERROR;
    }

    avBufferConfig_.size = bufferSize;
    Status ret = bufferQueueProducer_->RequestBufferWaitUs(inputBuffers_[inputIndex_], avBufferConfig_, timeoutUs);
    if (ret != Status::OK) {
        AVCODEC_LOGE("SyncCodecAdapter RequestBuffer failed, index %{public}u is invaild", *index);
        return StatusToAVCodecServiceErrCode(ret);
    }

    *index = inputIndex_;
    if (++inputIndex_ >= inputBuffers_.size()) {
        inputIndex_ = 0;
    }

    return AVCodecServiceErrCode::AVCS_ERR_OK;
}

std::shared_ptr<AVBuffer> AVCodecAudioCodecInnerImpl::SyncCodecAdapter::GetInputBuffer(uint32_t index)
{
    if (index >= inputBuffers_.size() || inputBuffers_[index] == nullptr) {
        AVCODEC_LOGD("SyncCodecAdapter GetInputBuffer failed, index %{public}u is invaild", index);
        return nullptr;
    }

    return inputBuffers_[index];
}

std::shared_ptr<AVBuffer> AVCodecAudioCodecInnerImpl::SyncCodecAdapter::GetOutputBuffer(int64_t timeoutUs)
{
    if (!init_) {
        AVCODEC_LOGW("SyncCodecAdapter GetOutputBuffer do not work before prepare");
        return nullptr;
    }

    std::shared_ptr<AVBuffer> outputBuffer;
    std::unique_lock<std::mutex> lock(outputMutex_);

    if (outputAvaliableNum_ == 0) {
        if (!WaitFor(lock, timeoutUs)) {
            return nullptr;
        }
    }

    Status ret = bufferQueueConsumer_->AcquireBuffer(outputBuffer);
    --outputAvaliableNum_;

    if (ret != Status::OK) {
        AVCODEC_LOGE(
            "AVCodecAudioCodecSyncInnerImpl Consumer AcquireBuffer fail, ret= %{public}d", static_cast<int32_t>(ret));
        return nullptr;
    }

    if (outputBuffer == nullptr) {
        AVCODEC_LOGE("AVCodecAudioCodecSyncInnerImpl outputBuffer is nullptr");
        return nullptr;
    }
    outputBuffers_.emplace(std::make_pair(outputBuffer.get(), outputBuffer));

    return outputBuffer;
}

int32_t AVCodecAudioCodecInnerImpl::SyncCodecAdapter::PushInputBuffer(uint32_t index, bool available)
{
    if (!init_) {
        AVCODEC_LOGW("SyncCodecAdapter PushInputBuffer do not work before prepare");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }

    if (index >= inputBuffers_.size() || inputBuffers_[index] == nullptr) {
        return AVCodecServiceErrCode::AVCS_ERR_INPUT_DATA_ERROR;
    }

    Status ret = bufferQueueProducer_->PushBuffer(inputBuffers_[index], available);
    if (ret != Status::OK) {
        AVCODEC_LOGE("SyncCodecAdapter PushInputBuffer failed, ret = %{public}d", ret);
    }
    inputBuffers_[index].reset();
    return StatusToAVCodecServiceErrCode(ret);
}

int32_t AVCodecAudioCodecInnerImpl::SyncCodecAdapter::ReleaseOutputBuffer(const std::shared_ptr<AVBuffer> &buffer)
{
    if (!init_) {
        AVCODEC_LOGW("SyncCodecAdapter ReleaseOutputBuffer do not work before prepare");
        return AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE;
    }

    {
        std::unique_lock<std::mutex> lock(outputMutex_);
        auto it = outputBuffers_.find(buffer.get());
        if (it == outputBuffers_.end()) {
            return AVCodecServiceErrCode::AVCS_ERR_INPUT_DATA_ERROR;
        }
        outputBuffers_.erase(it);
    }

    return StatusToAVCodecServiceErrCode(bufferQueueConsumer_->ReleaseBuffer(buffer));
}

void AVCodecAudioCodecInnerImpl::SyncCodecAdapter::OnBufferAvailable()
{
    {
        std::unique_lock<std::mutex> lock(outputMutex_);
        ++outputAvaliableNum_;
    }
    outputCV_.notify_all();
}

sptr<Media::AVBufferQueueProducer> AVCodecAudioCodecInnerImpl::SyncCodecAdapter::GetProducer()
{
    return innerBufferQueue_ != nullptr ? innerBufferQueue_->GetProducer() : nullptr;
}

bool AVCodecAudioCodecInnerImpl::SyncCodecAdapter::WaitFor(std::unique_lock<std::mutex> &lock, int64_t timeoutUs)
{
    if (timeoutUs < 0) {
        outputCV_.wait(lock, [this] { return outputAvaliableNum_ > 0; });
    } else {
        return outputCV_.wait_for(
            lock, std::chrono::microseconds(timeoutUs), [this] { return outputAvaliableNum_ > 0; });
    }
    return true;
}
} // namespace MediaAVCodec
} // namespace OHOS