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

#include "media_codec_client.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avbuffer_queue_producer.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodecClient"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<MediaCodecClient> MediaCodecClient::Create(const sptr<IStandardMediaCodecService> &ipcProxy)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "Ipc proxy is nullptr.");

    std::shared_ptr<MediaCodecClient> codec = std::make_shared<MediaCodecClient>(ipcProxy);

    int32_t ret = codec->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Codec client create failed");

    AVCODEC_LOGI("Codec client create successful");
    return codec;
}

MediaCodecClient::MediaCodecClient(const sptr<IStandardCodecService> &ipcProxy)
    : codecProxy_(ipcProxy)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecClient::~MediaCodecClient()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);

    if (codecProxy_ != nullptr) {
        (void)codecProxy_->DestroyStub();
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void MediaCodecClient::AVCodecServerDied()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    codecProxy_ = nullptr;
    listenerStub_ = nullptr;

    if (callback_ != nullptr) {
        callback_->OnError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_SERVICE_DIED);
    } else {
        AVCODEC_LOGD("Callback OnError is nullptr");
    }
}

int32_t MediaCodecClient::CreateListenerObject()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");
    
    listenerStub_ = new(std::nothrow) MediaCodecListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener stub create failed");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Listener object is nullptr.");

    int32_t ret = codecProxy_->SetListenerObject(object);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client set listener object successful");
    }
    return ret;
}

int32_t MediaCodecClient::Init(bool isEncoder, bool isMimeType, const std::string &name)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Init(isEncoder, isMimeType, name);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client init successful");
    }
    return ret;
}

int32_t MediaCodecClient::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    Format format_ = format;
    format_.PutStringValue("process_name", program_invocation_name);

    int32_t ret = codecProxy_->Configure(format_);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client configure successful");
    }
    return ret;
}

int32_t MediaCodecClient::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Start();
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client start successful");
    }
    return ret;
}

int32_t MediaCodecClient::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Prepare();
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client prepare successful");
    }
    return ret;
}

int32_t MediaCodecClient::Stop()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");
    
    int32_t ret = codecProxy_->Stop();
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client stop successful");
    }
    return ret;
}

int32_t MediaCodecClient::Flush()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");
    
    int32_t ret = codecProxy_->Flush();
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client flush successful");
    }
    return ret;
}

int32_t MediaCodecClient::Reset()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");
    
    int32_t ret = codecProxy_->Reset();
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client reset successful");
    }
    return ret;
}

int32_t MediaCodecClient::Release()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Release();
    (void)codecProxy_->DestroyStub();
    codecProxy_ = nullptr;
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client release successful");
    }
    return ret;
}

int32_t MediaCodecClient::SetCallback(const std::shared_ptr<AVCodecVideoCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener stub is nullptr.");

    callback_ = callback;
    listenerStub_->SetCallback(callback);
    AVCODEC_LOGI("Codec client set callback successful");
    return AVCS_ERR_OK;
}

int32_t MediaCodecClient::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->GetOutputFormat(format);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGD("Codec client get output format successful");
    }
    return ret;
}

int32_t MediaCodecClient::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->SetParameter(format);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client set parameter successful");
    }
    return ret;
}

sptr<Media::AVBufferQueueProducer> MediaCodecClient::GetInputBufferQueue()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->GetInputBufferQueue(bufferQueue);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client get input buffer queue successful");
    }
    return ret;
}

int32_t MediaCodecClient::SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->SetOutputBufferQueue(bufferQueue);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client set output buffer queue successful");
    }
    return ret;
}

sptr<OHOS::Surface> MediaCodecClient::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, nullptr, "Codec service does not exist.");

    auto ret = codecProxy_->CreateInputSurface();
    if (ret != nullptr) {
        AVCODEC_LOGI("Codec client create input surface successful");
    }
    return ret;
}

int32_t MediaCodecClient::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->SetOutputSurface(surface);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client set output surface successful");
    }
    return ret;
}

int32_t MediaCodecClient::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->NotifyEos();
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client notify eos successful");
    }
    return ret;
}

int32_t MediaCodecClient::SurfaceModeReturnData(std::shared_ptr<Meida::AVBuffer> buffer, bool available)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_INVALID_VAL, "Buffer is null.");
    uint64_t index = buffer->GetUniqueId();
    bool flag = listenerStub_->FindBufferFromIndex(index, nullptr);
    CHECK_AND_RETURN_RET_LOG(flag != false, AVCS_ERR_INVALID_VAL, "Buffer is invalid.");
    int32_t ret = codecProxy_->SurfaceModeReturnData(index, available);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client notify eos successful");
    }
    return ret;
}
} // namespace MediaAVCodec
} // namespace OHOS
