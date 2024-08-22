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
#include <cmath>
#include "avcodec_errors.h"
#include "codec_service_proxy.h"
#include "meta/meta_key.h"

using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
int32_t CodecClient::Create(const sptr<IStandardCodecService> &ipcProxy, std::shared_ptr<ICodecService> &codec)
{
    OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecClient"};
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, AVCS_ERR_INVALID_VAL, "Ipc proxy is nullptr.");

    codec = std::make_shared<CodecClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(codec != nullptr && std::static_pointer_cast<CodecClient>(codec)->syncMutex_ != nullptr,
        AVCS_ERR_UNKNOWN, "Codec client is nullptr");

    int32_t ret = std::static_pointer_cast<CodecClient>(codec)->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Codec client create failed");

    AVCODEC_LOGI("Succeed");
    return AVCS_ERR_OK;
}

CodecClient::CodecClient(const sptr<IStandardCodecService> &ipcProxy)
    : codecProxy_(ipcProxy), syncMutex_(std::make_shared<std::recursive_mutex>())
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecClient::~CodecClient()
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    if (codecProxy_ != nullptr) {
        (void)codecProxy_->DestroyStub();
        SetNeedListen(false);
    }
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecClient::AVCodecServerDied()
{
    {
        std::scoped_lock lock(mutex_, *syncMutex_);
        codecProxy_ = nullptr;
        listenerStub_ = nullptr;
    }
    OnError(AVCODEC_ERROR_INTERNAL, AVCS_ERR_SERVICE_DIED);
}

int32_t CodecClient::CreateListenerObject()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    listenerStub_ = new (std::nothrow) CodecListenerStub();
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Codec listener stub create failed");
    listenerStub_->SetMutex(syncMutex_);

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

void CodecClient::InitLabel(AVCodecType type)
{
    static std::mutex g_mutex;
    static uint64_t g_uid = 0;
    {
        std::lock_guard<std::mutex> lock(g_mutex);
        uid_ = ++g_uid;
    }
    auto &label = const_cast<OHOS::HiviewDFX::HiLogLabel &>(LABEL);
    switch (type) {
        case AVCODEC_TYPE_VIDEO_ENCODER:
            tag_ = "EncClient[";
            break;
        case AVCODEC_TYPE_VIDEO_DECODER:
            tag_ = "DecClient[";
            break;
        default:
            tag_ = "CodecClient[";
            break;
    }
    tag_ += std::to_string(uid_) + "]";
    label.tag = tag_.c_str();
    if (codecProxy_ != nullptr) {
        static_cast<CodecServiceProxy *>(codecProxy_.GetRefPtr())->InitLabel(uid_);
    }
    if (listenerStub_ != nullptr) {
        listenerStub_->InitLabel(uid_);
    }
    type_ = type;
}

int32_t CodecClient::Init(AVCodecType type, bool isMimeType, const std::string &name,
                          Meta &callerInfo, API_VERSION apiVersion)
{
    (void)apiVersion;
    using namespace OHOS::Media;
    InitLabel(type);
    callerInfo.SetData(Tag::AV_CODEC_CALLER_PID, getprocpid());
    callerInfo.SetData(Tag::AV_CODEC_CALLER_UID, getuid());
    callerInfo.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, std::string(program_invocation_name));

    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    converter_ = BufferConverter::Create(type);
    CHECK_AND_RETURN_RET_LOG(converter_ != nullptr, AVCS_ERR_NO_MEMORY, "failed to create converter");
    if (listenerStub_ != nullptr) {
        listenerStub_->SetConverter(converter_);
    }

    int32_t ret = codecProxy_->Init(type, isMimeType, name, callerInfo);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t isSetParameterCb = (codecMode_ & CODEC_SET_PARAMETER_CALLBACK) != 0;
    const_cast<Format &>(format).PutIntValue(Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, isSetParameterCb);

    int32_t ret = codecProxy_->Configure(format);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    if (!hasOnceConfigured_) {
        hasOnceConfigured_ = ret == AVCS_ERR_OK;
    }
    return ret;
}

int32_t CodecClient::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->Prepare();
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");

    return ret;
}

int32_t CodecClient::SetCustomBuffer(std::shared_ptr<AVBuffer> buffer)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, AVCS_ERR_INVALID_VAL, "buffer is nullptr");

    int32_t ret = codecProxy_->SetCustomBuffer(buffer);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(codecMode_ != CODEC_SET_PARAMETER_CALLBACK, AVCS_ERR_INVALID_STATE,
                             "Not get input surface.");

    SetNeedListen(true);
    int32_t ret = codecProxy_->Start();
    if (ret == AVCS_ERR_OK) {
        needUpdateGeneration_ = true;
        AVCODEC_LOGI("Succeed");
    } else {
        SetNeedListen(needUpdateGeneration_); // is in running state = needUpdateGeneration_
    }
    return ret;
}

int32_t CodecClient::Stop()
{
    int32_t ret;
    {
        std::scoped_lock lock(mutex_, *syncMutex_);
        CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        ret = codecProxy_->Stop();
        SetNeedListen(false);
    }
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
        AVCODEC_LOGI("Succeed");
    }
    return ret;
}

int32_t CodecClient::Flush()
{
    int32_t ret;
    {
        std::scoped_lock lock(mutex_, *syncMutex_);
        CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        ret = codecProxy_->Flush();
        SetNeedListen(false);
    }
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
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
        std::scoped_lock lock(mutex_, *syncMutex_);
        CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        ret = codecProxy_->Reset();
        SetNeedListen(false);
    }
    if (ret == AVCS_ERR_OK) {
        hasOnceConfigured_ = false;
        if (converter_ != nullptr) {
            converter_->NeedToResetFormatOnce();
        }
        UpdateGeneration();
        AVCODEC_LOGI("Succeed");
    }
    return ret;
}

int32_t CodecClient::Release()
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->Release();
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    (void)codecProxy_->DestroyStub();
    SetNeedListen(false);
    codecProxy_ = nullptr;
    listenerStub_ = nullptr;
    return ret;
}

sptr<OHOS::Surface> CodecClient::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, nullptr, "Server not exist");

    auto ret = codecProxy_->CreateInputSurface();
    EXPECT_AND_LOGI(ret != nullptr, "Succeed");
    codecMode_ |= CODEC_SURFACE_MODE;
    return ret;
}

int32_t CodecClient::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->SetOutputSurface(surface);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    codecMode_ = CODEC_SURFACE_MODE;
    return ret;
}

int32_t CodecClient::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(callbackMode_ == MEMORY_CALLBACK, AVCS_ERR_INVALID_STATE,
                             "The callback of AVSharedMemory is invalid!");
    int32_t ret = codecProxy_->QueueInputBuffer(index, info, flag);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed. index:%{public}u", index);
    return ret;
}

int32_t CodecClient::QueueInputBuffer(uint32_t index)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(callbackMode_ == BUFFER_CALLBACK, AVCS_ERR_INVALID_STATE,
                             "The callback of AVBuffer is invalid!");

    int32_t ret = codecProxy_->QueueInputBuffer(index);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed. index:%{public}u", index);
    return ret;
}

int32_t CodecClient::QueueInputParameter(uint32_t index)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(codecMode_ == CODEC_SURFACE_MODE_WITH_SETPARAMETER, AVCS_ERR_INVALID_STATE,
                             "Is in invalid state!");

    int32_t ret = codecProxy_->QueueInputParameter(index);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed. index:%{public}u", index);
    return ret;
}

int32_t CodecClient::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->GetOutputFormat(format);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
    if (callbackMode_ == MEMORY_CALLBACK && converter_ != nullptr) {
        converter_->SetFormat(format);
        converter_->GetFormat(format);
    } else {
        UpdateFormat(format);
    }
    return ret;
}

#ifdef SUPPORT_DRM
int32_t CodecClient::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(keySession != nullptr, AVCS_ERR_INVALID_OPERATION, "Server not exist");

    int32_t ret = codecProxy_->SetDecryptConfig(keySession, svpFlag);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}
#endif

int32_t CodecClient::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(callbackMode_ != INVALID_CALLBACK, AVCS_ERR_INVALID_STATE, "The callback is invalid!");

    int32_t ret = codecProxy_->ReleaseOutputBuffer(index, render);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed. index:%{public}u", index);
    return ret;
}

int32_t CodecClient::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG(callbackMode_ != INVALID_CALLBACK, AVCS_ERR_INVALID_STATE, "The callback is invalid!");
    CHECK_AND_RETURN_RET_LOG(renderTimestampNs >= 0, AVCS_ERR_INVALID_VAL,
                             "The renderTimestamp:%{public}" PRId64 " value error", renderTimestampNs);

    int32_t ret = codecProxy_->RenderOutputBufferAtTime(index, renderTimestampNs);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed. index:%{public}u, renderTimestamp:%{public}" PRId64, index,
                    renderTimestampNs);
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
    CHECK_AND_RETURN_RET_LOG(callbackMode_ == MEMORY_CALLBACK || callbackMode_ == INVALID_CALLBACK,
                             AVCS_ERR_INVALID_STATE, "The callback of AVBuffer is already set!");
    callbackMode_ = MEMORY_CALLBACK;

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
    CHECK_AND_RETURN_RET_LOG(callbackMode_ == BUFFER_CALLBACK || callbackMode_ == INVALID_CALLBACK,
                             AVCS_ERR_INVALID_STATE, "The callback of AVSharedMemory is already set!");
    callbackMode_ = BUFFER_CALLBACK;

    videoCallback_ = callback;
    const std::shared_ptr<MediaCodecCallback> &stubCallback = shared_from_this();
    listenerStub_->SetCallback(stubCallback);
    AVCODEC_LOGI("AVBuffer callback");
    return AVCS_ERR_OK;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener stub is nullptr.");
    CHECK_AND_RETURN_RET_LOG(!hasOnceConfigured_, AVCS_ERR_INVALID_STATE, "Need to configure encoder!");
    CHECK_AND_RETURN_RET_LOG(paramWithAttrCallback_ == nullptr, AVCS_ERR_INVALID_STATE,
                             "Already set parameter with atrribute callback!");
    codecMode_ |= CODEC_SET_PARAMETER_CALLBACK;

    paramCallback_ = callback;
    const std::shared_ptr<MediaCodecParameterCallback> &stubCallback = shared_from_this();
    listenerStub_->SetCallback(stubCallback);
    AVCODEC_LOGI("Parameter callback");
    return AVCS_ERR_OK;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr.");
    CHECK_AND_RETURN_RET_LOG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY, "Listener stub is nullptr.");
    CHECK_AND_RETURN_RET_LOG(!hasOnceConfigured_, AVCS_ERR_INVALID_STATE, "Need to configure encoder!");
    CHECK_AND_RETURN_RET_LOG(paramCallback_ == nullptr, AVCS_ERR_INVALID_STATE, "Already set parameter callback!");
    codecMode_ |= CODEC_SET_PARAMETER_CALLBACK;

    paramWithAttrCallback_ = callback;
    const std::shared_ptr<MediaCodecParameterWithAttrCallback> &stubCallback = shared_from_this();
    listenerStub_->SetCallback(stubCallback);
    AVCODEC_LOGI("Parameter callback");
    return AVCS_ERR_OK;
}

int32_t CodecClient::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->GetInputFormat(format);
    EXPECT_AND_LOGD(ret == AVCS_ERR_OK, "Succeed");
    if (callbackMode_ == MEMORY_CALLBACK && converter_ != nullptr) {
        converter_->SetFormat(format);
        converter_->GetFormat(format);
    } else {
        UpdateFormat(format);
    }
    return ret;
}

void CodecClient::UpdateGeneration()
{
    if (listenerStub_ != nullptr && needUpdateGeneration_) {
        listenerStub_->UpdateGeneration();
        needUpdateGeneration_ = false;
    }
}

void CodecClient::UpdateFormat(Format &format)
{
    if (format.ContainKey(Tag::VIDEO_STRIDE) || format.ContainKey(Tag::VIDEO_SLICE_HEIGHT)) {
        int32_t width = 0;
        int32_t height = 0;
        switch (type_) {
            case AVCODEC_TYPE_VIDEO_ENCODER:
                format.GetIntValue(Tag::VIDEO_WIDTH, width);
                format.GetIntValue(Tag::VIDEO_HEIGHT, height);
                break;
            case AVCODEC_TYPE_VIDEO_DECODER:
                format.GetIntValue(Tag::VIDEO_PIC_WIDTH, width);
                format.GetIntValue(Tag::VIDEO_PIC_HEIGHT, height);
                break;
            default:
                return;
        }
        int32_t wStride = 0;
        int32_t hStride = 0;
        format.GetIntValue(Tag::VIDEO_STRIDE, wStride);
        format.GetIntValue(Tag::VIDEO_SLICE_HEIGHT, hStride);
        format.PutIntValue(Tag::VIDEO_STRIDE, std::max(width, wStride));
        format.PutIntValue(Tag::VIDEO_SLICE_HEIGHT, std::max(height, hStride));
    }
}

void CodecClient::SetNeedListen(const bool needListen)
{
    if (listenerStub_ != nullptr) {
        listenerStub_->SetNeedListen(needListen);
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
        if (converter_ != nullptr) {
            converter_->SetFormat(format);
            converter_->GetFormat(const_cast<Format &>(format));
        }
        callback_->OnOutputFormatChanged(format);
    } else if (videoCallback_ != nullptr) {
        UpdateFormat(const_cast<Format &>(format));
        videoCallback_->OnOutputFormatChanged(format);
    }
}

void CodecClient::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    AVCODEC_LOGD("index:%{public}u", index);
    callback_->OnInputBufferAvailable(index, buffer);
}

void CodecClient::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                          std::shared_ptr<AVSharedMemory> buffer)
{
    AVCODEC_LOGD("index:%{public}u", index);
    callback_->OnOutputBufferAvailable(index, info, flag, buffer);
}

void CodecClient::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    AVCODEC_LOGD("index:%{public}u", index);
    videoCallback_->OnInputBufferAvailable(index, buffer);
}

void CodecClient::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    AVCODEC_LOGD("index:%{public}u", index);
    videoCallback_->OnOutputBufferAvailable(index, buffer);
}

void CodecClient::OnInputParameterAvailable(uint32_t index, std::shared_ptr<Format> parameter)
{
    AVCODEC_LOGD("index:%{public}u", index);
    paramCallback_->OnInputParameterAvailable(index, parameter);
}

void CodecClient::OnInputParameterWithAttrAvailable(uint32_t index, std::shared_ptr<Format> attribute,
                                                    std::shared_ptr<Format> parameter)
{
    AVCODEC_LOGD("index:%{public}u", index);
    paramWithAttrCallback_->OnInputParameterWithAttrAvailable(index, attribute, parameter);
}
} // namespace MediaAVCodec
} // namespace OHOS
