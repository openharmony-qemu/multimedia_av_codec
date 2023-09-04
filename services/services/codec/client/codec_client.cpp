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

#include "codec_client.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecClient"};
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<CodecClient> CodecClient::Create(const sptr<IStandardCodecService> &ipcProxy)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, nullptr, "Ipc proxy is nullptr.");

    std::shared_ptr<CodecClient> codec = std::make_shared<CodecClient>(ipcProxy);

    int32_t ret = codec->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Codec client create failed");

    AVCODEC_LOGI("Codec client create successful");
    return codec;
}

CodecClient::CodecClient(const sptr<IStandardCodecService> &ipcProxy)
    : codecProxy_(ipcProxy)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecClient::~CodecClient()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);

    if (codecProxy_ != nullptr) {
        (void)codecProxy_->DestroyStub();
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecClient::AVCodecServerDied()
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

int32_t CodecClient::CreateListenerObject()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");
    
    listenerStub_ = new(std::nothrow) CodecListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener stub create failed");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Listener object is nullptr.");

    int32_t ret = codecProxy_->SetListenerObject(object);
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        AVCODEC_LOGI("Codec client set listener object successful");
    }
    return ret;
}

int32_t CodecClient::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Init(type, isMimeType, name);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client init successful");
    }
    return ret;
}

int32_t CodecClient::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Configure(format);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client configure successful");
    }
    return ret;
}

int32_t CodecClient::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Start();
    if (ret == AVCS_ERR_OK) {
        needUpdateGeneration = true;
        AVCODEC_LOGI("Codec client start successful");
    }
    return ret;
}

int32_t CodecClient::Stop()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Stop();
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        WaitCallbackDone();
        AVCODEC_LOGI("Codec client stop successful");
    }
    return ret;
}

int32_t CodecClient::Flush()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Flush();
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        WaitCallbackDone();
        AVCODEC_LOGI("Codec client flush successful");
    }
    return ret;
}

int32_t CodecClient::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->NotifyEos();
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client notify eos successful");
    }
    return ret;
}

int32_t CodecClient::Reset()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->Reset();
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        WaitCallbackDone();
        AVCODEC_LOGI("Codec client reset successful");
    }
    return ret;
}

int32_t CodecClient::Release()
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

sptr<OHOS::Surface> CodecClient::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, nullptr, "Codec service does not exist.");

    auto ret = codecProxy_->CreateInputSurface();
    if (ret == nullptr) {
        AVCODEC_LOGI("Codec client create input surface successful");
    }
    return ret;
}

int32_t CodecClient::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->SetOutputSurface(surface);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client set output surface successful");
    }
    return ret;
}

int32_t CodecClient::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->QueueInputBuffer(index, info, flag);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGD("Codec client queue input buffer successful");
    }
    return ret;
}

int32_t CodecClient::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->GetOutputFormat(format);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGD("Codec client get output format successful");
    }
    return ret;
}

int32_t CodecClient::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->ReleaseOutputBuffer(index, render);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGD("Codec client release output buffer successful");
    }
    return ret;
}

int32_t CodecClient::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->SetParameter(format);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client set parameter successful");
    }
    return ret;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener stub is nullptr.");

    callback_ = callback;
    listenerStub_->SetCallback(callback);
    AVCODEC_LOGI("Codec client set callback successful");
    return AVCS_ERR_OK;
}

int32_t CodecClient::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec service does not exist.");

    int32_t ret = codecProxy_->GetInputFormat(format);
    if (ret == AVCS_ERR_OK) {
        AVCODEC_LOGI("Codec client get input format successful");
    }
    return ret;
}

void CodecClient::UpdateGeneration()
{
    if (listenerStub_ != nullptr && needUpdateGeneration) {
        listenerStub_->UpdateGeneration();
        needUpdateGeneration = false;
    }
}

void CodecClient::WaitCallbackDone()
{
    if (listenerStub_ != nullptr) {
        listenerStub_->WaitCallbackDone();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS
