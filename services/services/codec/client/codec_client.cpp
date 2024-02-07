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
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "codec_service_proxy.h"

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

    AVCODEC_LOGI("Succeed");
    return codec;
}

CodecClient::CodecClient(const sptr<IStandardCodecService> &ipcProxy) : codecProxy_(ipcProxy)
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
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        codecProxy_ = nullptr;
        listenerStub_ = nullptr;
    }

    if (callback_ != nullptr) {
        callback_->OnError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_SERVICE_DIED);
    }
    EXPECT_AND_LOGD(callback_ == nullptr, "Callback OnError is nullptr");
}

int32_t CodecClient::CreateListenerObject()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    listenerStub_ = new (std::nothrow) CodecListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener stub create failed");

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG(object != nullptr, AVCS_ERR_NO_MEMORY, "Listener object is nullptr.");

    int32_t ret = codecProxy_->SetListenerObject(object);
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        AVCODEC_LOGI("Succeed");
    }
    static_cast<CodecServiceProxy *>(codecProxy_.GetRefPtr())->SetListener(listenerStub_);
    return ret;
}

int32_t CodecClient::Init(AVCodecType type, bool isMimeType, const std::string &name, API_VERSION apiVersion)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->Init(type, isMimeType, name);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    Format format_ = format;
    format_.PutStringValue("process_name", program_invocation_name);

    int32_t ret = codecProxy_->Configure(format_);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->Start();
    if (ret == AVCS_ERR_OK) {
        needUpdateGeneration = true;
        AVCODEC_LOGI("Succeed");
    }
    return ret;
}

int32_t CodecClient::Stop()
{
    int32_t ret;
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        ret = codecProxy_->Stop();
    }
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        WaitCallbackDone();
        AVCODEC_LOGI("Succeed");
    }
    return ret;
}

int32_t CodecClient::Flush()
{
    int32_t ret;
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        ret = codecProxy_->Flush();
    }
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        WaitCallbackDone();
        AVCODEC_LOGI("Succeed");
    }
    return ret;
}

int32_t CodecClient::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->NotifyEos();
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::Reset()
{
    int32_t ret;
    {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        ret = codecProxy_->Reset();
    }
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        WaitCallbackDone();
        AVCODEC_LOGI("Succeed");
    }
    return ret;
}

int32_t CodecClient::Release()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->Release();
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    (void)codecProxy_->DestroyStub();
    codecProxy_ = nullptr;
    return ret;
}

sptr<OHOS::Surface> CodecClient::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, nullptr, "Server not exist");

    auto ret = codecProxy_->CreateInputSurface();
    EXPECT_AND_LOGI(ret != nullptr, "Succeed");
    return ret;
}

int32_t CodecClient::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->SetOutputSurface(surface);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->QueueInputBuffer(index, info, flag);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::QueueInputBuffer(uint32_t index)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->QueueInputBuffer(index);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->GetOutputFormat(format);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

#ifdef SUPPORT_DRM
int32_t CodecClient::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(keySession != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->SetDecryptConfig(keySession, svpFlag);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}
#endif

int32_t CodecClient::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->ReleaseOutputBuffer(index, render);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->SetParameter(format);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener stub is nullptr.");

    callback_ = callback;
    const std::shared_ptr<AVCodecCallback> &stubCallback = shared_from_this();
    listenerStub_->SetCallback(stubCallback);
    AVCODEC_LOGI("AVSharedMemory callback");
    return AVCS_ERR_OK;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener stub is nullptr.");

    videoCallback_ = callback;
    const std::shared_ptr<MediaCodecCallback> &stubCallback = shared_from_this();
    listenerStub_->SetCallback(stubCallback);
    AVCODEC_LOGI("AVBuffer callback");
    return AVCS_ERR_OK;
}

int32_t CodecClient::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->GetInputFormat(format);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
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

void CodecClient::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (callback_ != nullptr) {
        callback_->OnError(errorType, errorCode);
    } else if (videoCallback_ != nullptr) {
        videoCallback_->OnError(errorType, errorCode);
    }
}

void CodecClient::OnOutputFormatChanged(const Format &format)
{
    if (callback_ != nullptr) {
        callback_->OnOutputFormatChanged(format);
    } else if (videoCallback_ != nullptr) {
        videoCallback_->OnOutputFormatChanged(format);
    }
}

void CodecClient::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    callback_->OnInputBufferAvailable(index, buffer);
}

void CodecClient::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                          std::shared_ptr<AVSharedMemory> buffer)
{
    callback_->OnOutputBufferAvailable(index, info, flag, buffer);
}

void CodecClient::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    videoCallback_->OnInputBufferAvailable(index, buffer);
}

void CodecClient::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    videoCallback_->OnOutputBufferAvailable(index, buffer);
}

} // namespace MediaAVCodec
} // namespace OHOS
