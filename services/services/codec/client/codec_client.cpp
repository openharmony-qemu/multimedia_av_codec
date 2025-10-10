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
#include "avcodec_trace.h"
#include "codec_service_proxy.h"
#include "meta/meta_key.h"

using namespace OHOS::Media;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecClient"};
inline std::string ErrorToStr(int32_t ret)
{
    return AVCSErrorToString(static_cast<OHOS::MediaAVCodec::AVCodecServiceErrCode>((ret)));
}
constexpr int64_t  LOG_INTERVAL_MS = 2000;  // 2s
constexpr uint32_t LOG_MAX_COUNT   = 5;     // max 5 times in logIntervalMs
} // namespace
namespace OHOS {
namespace MediaAVCodec {
int32_t CodecClient::Create(const sptr<IStandardCodecService> &ipcProxy, std::shared_ptr<ICodecService> &codec)
{
    CHECK_AND_RETURN_RET_LOG(ipcProxy != nullptr, AVCS_ERR_INVALID_VAL, "Ipc proxy is nullptr");

    std::shared_ptr<CodecClient> codecClient = std::make_shared<CodecClient>(ipcProxy);
    CHECK_AND_RETURN_RET_LOG(codecClient != nullptr, AVCS_ERR_UNKNOWN, "Codec client is nullptr");

    int32_t ret = codecClient->CreateListenerObject();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Codec client create failed");
    codec = codecClient;

    AVCODEC_LOGI("%{public}s", ErrorToStr(ret).c_str());
    return AVCS_ERR_OK;
}

CodecClient::CodecClient(const sptr<IStandardCodecService> &ipcProxy)
    : codecProxy_(ipcProxy),
      converter_(std::make_shared<BufferConverter>()),
      syncMutex_(std::make_shared<std::recursive_mutex>())
{
    AVCODEC_LOGI_WITH_TAG("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecClient::~CodecClient()
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    if (codecProxy_ != nullptr) {
        (void)codecProxy_->DestroyStub();
        SetNeedListen(false);
        circular_.SetIsRunning(false);
    }
    AVCODEC_LOGI_WITH_TAG("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
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
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    listenerStub_ = new (std::nothrow) CodecListenerStub();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(listenerStub_ != nullptr, AVCS_ERR_NO_MEMORY,
                                      "Codec listener stub create failed");
    const std::shared_ptr<MediaCodecCallback> callback = shared_from_this();
    listenerStub_->SetCallback(callback);
    listenerStub_->SetMutex(syncMutex_);

    sptr<IRemoteObject> object = listenerStub_->AsObject();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(object != nullptr, AVCS_ERR_NO_MEMORY, "Listener object is nullptr");

    int32_t ret = codecProxy_->SetListenerObject(object);
    if (ret == AVCS_ERR_OK) {
        UpdateGeneration();
    }
    auto codecProxy = static_cast<CodecServiceProxy *>(codecProxy_.GetRefPtr());
    codecProxy->SetListener(listenerStub_);
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::Init(AVCodecType type, bool isMimeType, const std::string &name, Meta &callerInfo,
                          API_VERSION apiVersion)
{
    (void)apiVersion;
    using namespace OHOS::Media;
    callerInfo.SetData(Tag::AV_CODEC_CALLER_PID, getprocpid());
    callerInfo.SetData(Tag::AV_CODEC_CALLER_UID, getuid());
    callerInfo.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, std::string(program_invocation_name));

    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr && listenerStub_ != nullptr && converter_ != nullptr,
                                      AVCS_ERR_NO_MEMORY, "Server not exist");
    auto codecProxy = static_cast<CodecServiceProxy *>(codecProxy_.GetRefPtr());
    int32_t ret = codecProxy->Init(type, isMimeType, name, callerInfo);
    const std::string tag = CreateVideoLogTag(callerInfo);
    this->SetTag(tag);
    converter_->SetTag(tag);
    codecProxy->SetTag(tag);
    listenerStub_->SetTag(tag);
    circular_.SetTag(tag);
    circular_.SetConverter(converter_);

    converter_->Init(type);
    listenerStub_->Init();
    type_ = type;
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!isConfigured_, AVCS_ERR_INVALID_STATE, "Is configured");
    // check sync mode
    int32_t enableSyncMode = 0;
    (void)format.GetIntValue(Tag::AV_CODEC_ENABLE_SYNC_MODE, enableSyncMode);
    if (enableSyncMode) {
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(circular_.CanEnableSyncMode(), AVCS_ERR_INVALID_OPERATION,
                                          "Can not enable sync mode");
        codecMode_ &= ~CODEC_ENABLE_PARAMETER;
    }
    if (codecMode_ & CODEC_ENABLE_PARAMETER) {
        const_cast<Format &>(format).PutIntValue(Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, 1);
    }
    // notify service to configure
    int32_t ret = codecProxy_->Configure(format);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "%{public}s", ErrorToStr(ret).c_str());
    // update client flag
    enableSyncMode ? circular_.EnableSyncMode() : circular_.EnableAsyncMode();
    isConfigured_ = true;
    AVCODEC_LOGI_WITH_TAG("%{public}s", format.Stringify().c_str());
    AVCODEC_LOGI_WITH_TAG("success. %{public}s mode", enableSyncMode ? "Sync" : "Async");
    return AVCS_ERR_OK;
}

int32_t CodecClient::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->Prepare();
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());

    return ret;
}

int32_t CodecClient::SetCustomBuffer(std::shared_ptr<AVBuffer> buffer)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(buffer != nullptr, AVCS_ERR_INVALID_VAL, "buffer is nullptr");

    int32_t ret = codecProxy_->SetCustomBuffer(buffer);
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::NotifyMemoryExchange(const bool exchangeFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->NotifyMemoryExchange(exchangeFlag);
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecMode_ != CODEC_ENABLE_PARAMETER, AVCS_ERR_INVALID_STATE,
                                      "Not get input surface");

    SetNeedListen(true);
    circular_.SetIsRunning(true);
    int32_t ret = codecProxy_->Start();
    if (ret == AVCS_ERR_OK) {
        needUpdateGeneration_ = true;
    } else {
        bool isRunning = needUpdateGeneration_;
        SetNeedListen(isRunning);
        circular_.SetIsRunning(isRunning);
    }
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::Stop()
{
    {
        std::scoped_lock lock(mutex_, *syncMutex_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        int32_t ret = codecProxy_->Stop();
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "%{public}s", ErrorToStr(ret).c_str());
        SetNeedListen(false);
        circular_.SetIsRunning(false);
        circular_.ClearCaches();
    }
    UpdateGeneration();
    AVCODEC_LOGI_WITH_TAG("success");
    return AVCS_ERR_OK;
}

int32_t CodecClient::Flush()
{
    {
        std::scoped_lock lock(mutex_, *syncMutex_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        int32_t ret = codecProxy_->Flush();
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "%{public}s", ErrorToStr(ret).c_str());
        SetNeedListen(false);
        circular_.SetIsRunning(false); // current state: FLUSHED
        circular_.FlushCaches();
    }
    UpdateGeneration();
    AVCODEC_LOGI_WITH_TAG("success");
    return AVCS_ERR_OK;
}

int32_t CodecClient::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->NotifyEos();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "%{public}s", ErrorToStr(ret).c_str());
    circular_.NotifyEos();
    AVCODEC_LOGI_WITH_TAG("success");
    return AVCS_ERR_OK;
}

int32_t CodecClient::Reset()
{
    {
        std::scoped_lock lock(mutex_, *syncMutex_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
        int32_t ret = codecProxy_->Reset();
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "%{public}s", ErrorToStr(ret).c_str());
        SetNeedListen(false);
        circular_.SetIsRunning(false);
        circular_.ClearCaches();
        circular_.ResetFlag();
        codecMode_ &= ~CODEC_SURFACE_OUTPUT;
        isConfigured_ = false;
        if (converter_ != nullptr) {
            converter_->NeedToResetFormatOnce();
        }
    }
    UpdateGeneration();
    AVCODEC_LOGI_WITH_TAG("success");
    return AVCS_ERR_OK;
}

int32_t CodecClient::Release()
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->Release();
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    (void)codecProxy_->DestroyStub();
    SetNeedListen(false);
    codecProxy_ = nullptr;
    listenerStub_ = nullptr;
    return ret;
}

int32_t CodecClient::GetChannelId(int32_t &channelId)
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->GetChannelId(channelId);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

sptr<OHOS::Surface> CodecClient::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, nullptr, "Server not exist");

    auto ret = codecProxy_->CreateInputSurface();
    AVCODEC_LOGI_WITH_TAG("%{public}s", (ret != nullptr) ? "succeed" : "failed");
    if (ret != nullptr) {
        codecMode_ |= CODEC_SURFACE_INPUT;
    }
    return ret;
}

int32_t CodecClient::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->SetOutputSurface(surface);
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    if (ret == AVCS_ERR_OK) {
        codecMode_ |= CODEC_SURFACE_OUTPUT;
    }
    return ret;
}

int32_t CodecClient::SetLowPowerPlayerMode(bool isLpp)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->SetLowPowerPlayerMode(isLpp);
    EXPECT_AND_LOGI(ret == AVCS_ERR_OK, "Succeed");
    return ret;
}

int32_t CodecClient::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callbackMode_ == MEMORY_CALLBACK, AVCS_ERR_INVALID_STATE,
                                      "The callback of AVSharedMemory is invalid!");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(codecMode_ & CODEC_SURFACE_INPUT), AVCS_ERR_INVALID_OPERATION,
                                      "Input is the surface");
    int32_t ret = circular_.HandleInputBuffer(index, info, flag);
    if (ret == AVCS_ERR_OK) {
        ret = codecProxy_->QueueInputBuffer(index, info, flag);
    }
    CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(ret == AVCS_ERR_OK, ret, LOG_INTERVAL_MS, LOG_MAX_COUNT,
        "%{public}s.idx:%{public}u", ErrorToStr(ret).c_str(), index);
    circular_.QueueInputBufferDone(index);
    return AVCS_ERR_OK;
}

int32_t CodecClient::QueueInputBuffer(uint32_t index)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callbackMode_ == BUFFER_CALLBACK || circular_.IsSyncMode(),
                                      AVCS_ERR_INVALID_STATE, "The callback of AVBuffer is invalid in async mode");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(codecMode_ & CODEC_SURFACE_INPUT), AVCS_ERR_INVALID_OPERATION,
                                      "Input is the surface");
    int32_t ret = circular_.HandleInputBuffer(index);
    if (ret == AVCS_ERR_OK) {
        ret = codecProxy_->QueueInputBuffer(index);
    }
    CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(ret == AVCS_ERR_OK, ret, LOG_INTERVAL_MS, LOG_MAX_COUNT,
        "%{public}s.idx:%{public}u", ErrorToStr(ret).c_str(), index);
    circular_.QueueInputBufferDone(index);
    return AVCS_ERR_OK;
}

int32_t CodecClient::QueueInputParameter(uint32_t index)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecMode_ == CODEC_SURFACE_MODE_WITH_PARAMETER, AVCS_ERR_INVALID_OPERATION,
                                      "Need to enable input parameter");
    int32_t ret = circular_.HandleInputBuffer(index);
    if (ret == AVCS_ERR_OK) {
        ret = codecProxy_->QueueInputParameter(index);
    }
    CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(ret == AVCS_ERR_OK, ret, LOG_INTERVAL_MS, LOG_MAX_COUNT,
        "%{public}s.idx:%{public}u", ErrorToStr(ret).c_str(), index);
    circular_.QueueInputBufferDone(index);
    return AVCS_ERR_OK;
}

int32_t CodecClient::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    int32_t ret = codecProxy_->GetOutputFormat(format);
    UpdateFormat(format);
    AVCODEC_LOGD_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

#ifdef SUPPORT_DRM
int32_t CodecClient::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_INVALID_OPERATION, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(keySession != nullptr, AVCS_ERR_INVALID_OPERATION, "Server not exist");

    int32_t ret = codecProxy_->SetDecryptConfig(keySession, svpFlag);
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}
#endif

int32_t CodecClient::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callbackMode_ != INVALID_CALLBACK || circular_.IsSyncMode(),
                                      AVCS_ERR_INVALID_STATE, "The callback is invalid!");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!render || (codecMode_ & CODEC_SURFACE_OUTPUT), AVCS_ERR_INVALID_OPERATION,
                                      "Need to set output surface");
    int32_t ret = circular_.HandleOutputBuffer(index);
    if (ret == AVCS_ERR_OK) {
        ret = codecProxy_->ReleaseOutputBuffer(index, render);
    }
    CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(ret == AVCS_ERR_OK, ret, LOG_INTERVAL_MS, LOG_MAX_COUNT,
        "%{public}s.idx:%{public}u", ErrorToStr(ret).c_str(), index);
    circular_.ReleaseOutputBufferDone(index);
    return AVCS_ERR_OK;
}

int32_t CodecClient::RenderOutputBufferAtTime(uint32_t index, int64_t renderTimestampNs)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callbackMode_ != INVALID_CALLBACK || circular_.IsSyncMode(),
                                      AVCS_ERR_INVALID_STATE, "The callback is invalid!");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecMode_ & CODEC_SURFACE_OUTPUT, AVCS_ERR_INVALID_OPERATION,
                                      "Need to set output surface");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(renderTimestampNs >= 0, AVCS_ERR_INVALID_VAL,
                                      "The renderTimestamp:%{public}" PRId64 " value error", renderTimestampNs);
    int32_t ret = circular_.HandleOutputBuffer(index);
    if (ret == AVCS_ERR_OK) {
        ret = codecProxy_->RenderOutputBufferAtTime(index, renderTimestampNs);
    }
    CHECK_AND_RETURN_RET_LOG_LIMIT_IN_TIME_WITH_TAG(ret == AVCS_ERR_OK, ret, LOG_INTERVAL_MS, LOG_MAX_COUNT,
        "%{public}s.idx:%{public}u", ErrorToStr(ret).c_str(), index);
    circular_.ReleaseOutputBufferDone(index);
    return AVCS_ERR_OK;
}

int32_t CodecClient::QueryInputBuffer(uint32_t &index, int64_t timeoutUs)
{
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(codecMode_ & CODEC_SURFACE_INPUT), AVCS_ERR_INVALID_OPERATION,
                                          "Input is the surface");
    }
    return circular_.QueryInputBuffer(index, timeoutUs);
}

int32_t CodecClient::QueryOutputBuffer(uint32_t &index, int64_t timeoutUs)
{
    return circular_.QueryOutputBuffer(index, timeoutUs);
}

std::shared_ptr<AVBuffer> CodecClient::GetInputBuffer(uint32_t index)
{
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(!(codecMode_ & CODEC_SURFACE_INPUT), nullptr, "Input is the surface");
    }
    return circular_.GetInputBuffer(index);
}

std::shared_ptr<AVBuffer> CodecClient::GetOutputBuffer(uint32_t index)
{
    return circular_.GetOutputBuffer(index);
}

int32_t CodecClient::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");

    int32_t ret = codecProxy_->SetParameter(format);
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callbackMode_ == MEMORY_CALLBACK || callbackMode_ == INVALID_CALLBACK,
                                      AVCS_ERR_INVALID_STATE, "The callback of AVBuffer is already set!");

    int32_t ret = circular_.SetCallback(callback);
    if (ret == AVCS_ERR_OK) {
        callbackMode_ = MEMORY_CALLBACK;
    }
    AVCODEC_LOGI_WITH_TAG("AVSharedMemory callback.%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callbackMode_ == BUFFER_CALLBACK || callbackMode_ == INVALID_CALLBACK,
                                      AVCS_ERR_INVALID_STATE, "The callback of AVSharedMemory is already set!");

    int32_t ret = circular_.SetCallback(callback);
    if (ret == AVCS_ERR_OK) {
        callbackMode_ = BUFFER_CALLBACK;
    }
    AVCODEC_LOGI_WITH_TAG("AVBuffer callback.%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!isConfigured_, AVCS_ERR_INVALID_STATE, "Need to be configured before!");

    int32_t ret = circular_.SetCallback(callback);
    if (ret == AVCS_ERR_OK) {
        codecMode_ |= CODEC_ENABLE_PARAMETER;
    }
    AVCODEC_LOGI_WITH_TAG("Parameter callback.%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    std::scoped_lock lock(mutex_, *syncMutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(callback != nullptr, AVCS_ERR_NO_MEMORY, "Callback is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!isConfigured_, AVCS_ERR_INVALID_STATE, "Need to configure encoder!");

    int32_t ret = circular_.SetCallback(callback);
    if (ret == AVCS_ERR_OK) {
        codecMode_ |= CODEC_ENABLE_PARAMETER;
    }
    AVCODEC_LOGI_WITH_TAG("Parameter callback.%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    int32_t ret = codecProxy_->GetInputFormat(format);
    UpdateFormat(format);
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
    return ret;
}

int32_t CodecClient::GetCodecInfo(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecProxy_ != nullptr, AVCS_ERR_NO_MEMORY, "Server not exist");
    int32_t ret;
    if (codecInfo_.ContainKey(Tag::MEDIA_IS_HARDWARE)) {
        format = codecInfo_;
        ret = AVCS_ERR_OK;
    } else {
        ret = codecProxy_->GetCodecInfo(codecInfo_);
        if (ret == AVCS_ERR_OK) {
            format = codecInfo_;
        }
    }
    AVCODEC_LOGI_WITH_TAG("%{public}s", format.Stringify().c_str());
    AVCODEC_LOGI_WITH_TAG("%{public}s", ErrorToStr(ret).c_str());
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
    if (callbackMode_ == MEMORY_CALLBACK && converter_ != nullptr) {
        converter_->SetFormat(format);
        converter_->GetFormat(format);
        return;
    }
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
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    AVCODEC_LOGW_WITH_TAG("%{public}s", AVCSErrorToString(static_cast<AVCodecServiceErrCode>(errorCode)).c_str());
    circular_.OnError(errorType, errorCode);
}

void CodecClient::OnOutputFormatChanged(const Format &format)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    UpdateFormat(const_cast<Format &>(format));
    AVCODEC_LOGI_WITH_TAG("%{public}s", format.Stringify().c_str());
    circular_.OnOutputFormatChanged(format);
}

void CodecClient::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    AVCODEC_LOGD_WITH_TAG("index:%{public}u", index);
    circular_.OnInputBufferAvailable(index, buffer);
}

void CodecClient::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    AVCODEC_LOGD_WITH_TAG("index:%{public}u", index);
    circular_.OnOutputBufferAvailable(index, buffer);
}

void CodecClient::OnOutputBufferBinded(std::map<uint32_t, sptr<SurfaceBuffer>> &bufferMap)
{
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    circular_.OnOutputBufferBinded(bufferMap);
}

void CodecClient::OnOutputBufferUnbinded()
{
    AVCODEC_FUNC_TRACE_WITH_TAG_CLIENT;
    circular_.OnOutputBufferUnbinded();
}
} // namespace MediaAVCodec
} // namespace OHOS
