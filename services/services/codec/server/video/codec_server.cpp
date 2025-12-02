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

#include "codec_server.h"
#include <functional>
#include <limits>
#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>
#include "avcodec_codec_name.h"
#include "avcodec_dump_utils.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_sysevent.h"
#include "buffer/avbuffer.h"
#include "codec_ability_singleton.h"
#include "vcodec_factory.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "surface_type.h"
#include "surface_tools.h"
#include "surface_utils.h"
#include "codeclist_core.h"
#ifdef SUPPORT_DRM
#include "imedia_key_session_service.h"
#endif
#include "event_manager.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecServer"};
enum DumpIndex : uint32_t {
    DUMP_INDEX_FORWARD_CALLER_PID          = 0x01'01'01'00,
    DUMP_INDEX_FORWARD_CALLER_UID          = 0x01'01'02'00,
    DUMP_INDEX_FORWARD_CALLER_PROCESS_NAME = 0x01'01'03'00,
    DUMP_INDEX_CALLER_PID                  = 0x01'01'04'00,
    DUMP_INDEX_CALLER_UID                  = 0x01'01'05'00,
    DUMP_INDEX_CALLER_PROCESS_NAME         = 0x01'01'06'00,
    DUMP_INDEX_INSTANCE_ID                 = 0x01'01'07'00,
    DUMP_INDEX_STATUS                      = 0x01'01'08'00,
    DUMP_INDEX_LAST_ERROR                  = 0x01'01'09'00,
    DUMP_INDEX_CODEC_INFO_START            = 0x01'02'00'00,
};
constexpr uint32_t DUMP_OFFSET_8 = 8;

const std::map<OHOS::MediaAVCodec::CodecServer::CodecStatus, std::string> CODEC_STATE_MAP = {
    {OHOS::MediaAVCodec::CodecServer::UNINITIALIZED, "uninitialized"},
    {OHOS::MediaAVCodec::CodecServer::INITIALIZED, "initialized"},
    {OHOS::MediaAVCodec::CodecServer::CONFIGURED, "configured"},
    {OHOS::MediaAVCodec::CodecServer::RUNNING, "running"},
    {OHOS::MediaAVCodec::CodecServer::FLUSHED, "flushed"},
    {OHOS::MediaAVCodec::CodecServer::END_OF_STREAM, "EOS"},
    {OHOS::MediaAVCodec::CodecServer::ERROR, "error"},
};

const std::vector<std::pair<std::string_view, const std::string>> VIDEO_DUMP_TABLE = {
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "CodecName"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, "Width"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, "Height"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_FRAME_RATE, "FrameRate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "BitRate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, "PixelFormat"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_SCALE_TYPE, "ScaleType"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, "RotationAngle"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, "MaxInputSize"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, "MaxInputBufferCount"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, "MaxOutputBufferCount"},
};

const std::map<int32_t, const std::string> PIXEL_FORMAT_STRING_MAP = {
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::YUV420P), "YUV420P"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::YUVI420), "YUVI420"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV12), "NV12"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::NV21), "NV21"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::SURFACE_FORMAT), "SURFACE_FORMAT"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::RGBA), "RGBA"},
    {static_cast<int32_t>(OHOS::MediaAVCodec::VideoPixelFormat::UNKNOWN), "UNKNOWN_FORMAT"},
};

const std::map<int32_t, const std::string> SCALE_TYPE_STRING_MAP = {
    {OHOS::ScalingMode::SCALING_MODE_FREEZE, "Freeze"},
    {OHOS::ScalingMode::SCALING_MODE_SCALE_TO_WINDOW, "Scale to window"},
    {OHOS::ScalingMode::SCALING_MODE_SCALE_CROP, "Scale crop"},
    {OHOS::ScalingMode::SCALING_MODE_NO_SCALE_CROP, "No scale crop"},
};

struct PostProcessingCallbackUserData {
    std::shared_ptr<OHOS::MediaAVCodec::CodecServer> codecServer;
};

void PostProcessingCallbackOnError(int32_t errorCode, void* userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "Post processing callback's userData is nullptr");
    auto callbackUserData = static_cast<PostProcessingCallbackUserData*>(userData);
    auto codecServer = callbackUserData->codecServer;
    CHECK_AND_RETURN_LOG(codecServer != nullptr, "Codec server dose not exit");
    codecServer->PostProcessingOnError(errorCode);
}

void PostProcessingCallbackOnOutputBufferAvailable(uint32_t index, int32_t flag, void* userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "Post processing callback's userData is nullptr");
    auto callbackUserData = static_cast<PostProcessingCallbackUserData*>(userData);
    auto codecServer = callbackUserData->codecServer;
    CHECK_AND_RETURN_LOG(codecServer != nullptr, "Codec server dose not exit");
    codecServer->PostProcessingOnOutputBufferAvailable(index, flag);
}

void PostProcessingCallbackOnOutputFormatChanged(const OHOS::Media::Format& format, void* userData)
{
    CHECK_AND_RETURN_LOG(userData != nullptr, "Post processing callback's userData is nullptr");
    auto callbackUserData = static_cast<PostProcessingCallbackUserData *>(userData);
    auto codecServer = callbackUserData->codecServer;
    CHECK_AND_RETURN_LOG(codecServer != nullptr, "Codec server dose not exit");
    codecServer->PostProcessingOnOutputFormatChanged(format);
}
} // namespace

namespace OHOS {
namespace MediaAVCodec {
using namespace Media;
std::shared_ptr<ICodecService> CodecServer::Create(int32_t instanceId)
{
    std::shared_ptr<CodecServer> server = std::make_shared<CodecServer>();

    int32_t ret = server->InitServer(instanceId);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Server init failed, ret: %{public}d!", ret);
    return server;
}

CodecServer::CodecServer()
{
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
    AVCODEC_LOGD_WITH_TAG("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecServer::~CodecServer()
{
    codecBase_ = nullptr;
    codecBaseCb_ = nullptr;
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);

    AVCODEC_LOGD_WITH_TAG("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t CodecServer::InitServer(int32_t instanceId)
{
    instanceId_ = instanceId;
    return AVCS_ERR_OK;
}

int32_t CodecServer::Init(AVCodecType type, bool isMimeType, const std::string &name, Meta &callerInfo)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    EventManager::GetInstance().OnInstanceEvent(StatisticsEventType::BASIC_CREATE_CODEC_INFO);

    int32_t ret = isMimeType ? InitByMime(type, name, callerInfo) : InitByName(name, callerInfo);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret,
        "Init failed, ret: %{public}d, %{public}s, %{public}s: %{public}s",
        ret, type == AVCODEC_TYPE_VIDEO_ENCODER ? "enc" : "dec", isMimeType ? "mime" : "name", name.c_str());

    codecBaseCb_ = std::make_shared<CodecBaseCallback>(shared_from_this());
    ret = codecBase_->SetCallback(codecBaseCb_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "SetCallback failed");
    AVCODEC_LOGI_WITH_TAG("Create %{public}s by %{public}s success", codecName_.c_str(),
                          (isMimeType ? "mime" : "name"));
    StatusChanged(INITIALIZED, false);
    InitFramerateCalculator(callerInfo);
    SetInstanceInfo(type, isMimeType, name, callerInfo);
    return ret;
}

int32_t CodecServer::InitByName(const std::string &codecName, Meta &callerInfo)
{
    codecBase_ = VCodecFactory::Instance().CreateCodecByName(codecName);
    if (codecBase_ == nullptr) {
        return AVCS_ERR_NO_MEMORY;
    }
    codecName_ = codecName;
    auto ret = codecBase_->Init(callerInfo);
    if (ret == AVCS_ERR_INSUFFICIENT_HARDWARE_RESOURCES) {
        EventManager::GetInstance().OnInstanceEvent(
            StatisticsEventType::DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO, callerInfo);
    }
    return ret;
}

int32_t CodecServer::InitByMime(const AVCodecType type, const std::string &codecMime, Meta &callerInfo)
{
    int32_t ret = AVCS_ERR_OK;
    auto nameArray = VCodecFactory::Instance().GetCodecNameArrayByMime(type, codecMime);
    if (nameArray.empty()) {
        Media::Meta eventMeta;
        eventMeta.SetData(Tag::MIME_TYPE, codecMime);
        eventMeta.SetData(EventInfoExtentedKey::IS_ENCODER.data(), type == AVCODEC_TYPE_VIDEO_ENCODER);
        EventManager::GetInstance().OnInstanceEvent(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO, eventMeta);
        return AVCS_ERR_NO_MEMORY;
    }

    for (const auto &name : nameArray) {
        ret = InitByName(name, callerInfo);
        CHECK_AND_CONTINUE_LOG_WITH_TAG(ret == AVCS_ERR_OK, "Skip init failure. name: %{public}s", name.c_str());
        break;
    }
    return ret;
}

int32_t CodecServer::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    Format config = format;
    if (codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        format.GetIntValue(Tag::VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN, pushBlankBufferOnShutdown_);
    }

    int32_t isSetParameterCb = 0;
    format.GetIntValue(Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, isSetParameterCb);
    isSetParameterCb_ = isSetParameterCb != 0;

    int32_t paramCheckRet = AVCS_ERR_OK;
    if (codecType_ == AVCODEC_TYPE_VIDEO_ENCODER || codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        auto scenario = CodecParamChecker::CheckCodecScenario(config, codecType_, codecName_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(scenario != std::nullopt, AVCS_ERR_INVALID_VAL,
                                          "Failed to get codec scenario");
        scenario_ = scenario.value();
        paramCheckRet = CodecParamChecker::CheckConfigureValid(config, codecName_, scenario_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(paramCheckRet == AVCS_ERR_OK ||
                                              paramCheckRet == AVCS_ERR_CODEC_PARAM_INCORRECT,
                                          paramCheckRet, "Params in format is not valid");
        CodecScenarioInit(config);
    }

    if (framerateCalculator_) {
        auto framerate = 0.0;
        if (format.GetDoubleValue(Tag::VIDEO_FRAME_RATE, framerate)) {
            framerateCalculator_->SetConfiguredFramerate(framerate);
        }
    }

    int32_t ret = codecBase_->Configure(config);
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
        return ret;
    }
    ret = CreatePostProcessing(config);
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
        return ret;
    }
    StatusChanged(CONFIGURED);
    return paramCheckRet;
}

int32_t CodecServer::SetCustomBuffer(std::shared_ptr<AVBuffer> buffer)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == CONFIGURED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->SetCustomBuffer(buffer);
}

int32_t CodecServer::NotifyMemoryExchange(const bool /* exchangeFlag */)
{
    return AVCS_ERR_OK;
}

int32_t CodecServer::CodecScenarioInit(Format &config)
{
    switch (scenario_) {
        case CodecScenario::CODEC_SCENARIO_ENC_TEMPORAL_SCALABILITY:
            temporalScalability_ = std::make_shared<TemporalScalability>(codecName_);
            temporalScalability_->ValidateTemporalGopParam(config);
            if (!temporalScalability_->svcLTR_) {
                temporalScalability_ = nullptr;
            } else {
                temporalScalability_->SetTag(this->GetTag());
            }
            config.RemoveKey(Tag::VIDEO_ENCODE_B_FRAME_GOP_MODE);
            break;
        case CodecScenario::CODEC_SCENARIO_ENC_ENABLE_B_FRAME:
            config.RemoveKey(Tag::VIDEO_ENCODER_LTR_FRAME_COUNT);
            config.RemoveKey(Tag::VIDEO_ENCODER_TEMPORAL_GOP_SIZE);
            config.RemoveKey(Tag::VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE);
            config.RemoveKey(Tag::VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY);
            break;
        default:
            break;
    }

    return AVCS_ERR_OK;
}

void CodecServer::StartInputParamTask()
{
    if (inputParamTask_ == nullptr) {
        inputParamTask_ = std::make_shared<TaskThread>("InputParamTask");
        std::weak_ptr<CodecServer> weakThis = weak_from_this();
        inputParamTask_->RegisterHandler([weakThis] {
            std::shared_ptr<CodecServer> cs = weakThis.lock();
            if (cs) {
                uint32_t index = cs->temporalScalability_->GetFirstBufferIndex();
                AVCodecBufferInfo info;
                AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
                CHECK_AND_RETURN_LOG(cs->QueueInputBuffer(index, info, flag) == AVCS_ERR_OK, "QueueInputBuffer failed");
            }
        });
    }
    inputParamTask_->Start();
}

int32_t CodecServer::Start()
{
    SetFreeStatus(false);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == FLUSHED || status_ == CONFIGURED, AVCS_ERR_INVALID_STATE,
                                      "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (temporalScalability_ != nullptr && isCreateSurface_ && !isSetParameterCb_) {
        StartInputParamTask();
    }
    int32_t ret = StartPostProcessing();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Start post processing failed");
    ret = codecBase_->Start();
    if (ret != AVCS_ERR_OK) {
        (void)StopPostProcessing();
        StatusChanged(ERROR);
    } else {
        StatusChanged(RUNNING);
        isModeConfirmed_ = true;
        CodecDfxInfo codecDfxInfo;
        GetCodecDfxInfo(codecDfxInfo);
        CodecStartEventWrite(codecDfxInfo);
    }
    OnInstanceMemoryUpdateEvent();
    OnInstanceEncodeBeginEvent();
    return ret;
}

int32_t CodecServer::Stop()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOGW_WITH_TAG(status_ != CONFIGURED, AVCS_ERR_OK, "Already in %{public}s state",
                                       GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == RUNNING || status_ == END_OF_STREAM || status_ == FLUSHED,
                                      AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (temporalScalability_ != nullptr && inputParamTask_ != nullptr) {
        temporalScalability_->SetBlockQueueActive();
        inputParamTask_->Stop();
    }
    int32_t retPostProcessing = StopPostProcessing();
    int32_t retCodec = codecBase_->Stop();
    CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    if ((retPostProcessing + retCodec) != AVCS_ERR_OK) {
        StatusChanged(ERROR);
        return (retCodec == AVCS_ERR_OK) ? retPostProcessing : retCodec;
    }
    if (isSurfaceMode_ && codecType_ == AVCODEC_TYPE_VIDEO_DECODER && pushBlankBufferOnShutdown_) {
        SurfaceTools::GetInstance().CleanCache(instanceId_,
            SurfaceUtils::GetInstance()->GetSurface(surfaceId_), true);
    }
    StatusChanged(CONFIGURED);
    OnInstanceMemoryResetEvent();
    OnInstanceEncodeEndEvent();
    if (framerateCalculator_) {
        framerateCalculator_->OnStopped();
    }
    return AVCS_ERR_OK;
}

int32_t CodecServer::Flush()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOGW_WITH_TAG(status_ != FLUSHED, AVCS_ERR_OK, "Already in %{public}s state",
                                       GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == RUNNING || status_ == END_OF_STREAM, AVCS_ERR_INVALID_STATE,
                                      "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (temporalScalability_ != nullptr && inputParamTask_ != nullptr) {
        temporalScalability_->SetBlockQueueActive();
        inputParamTask_->Stop();
    }
    int32_t retPostProcessing = FlushPostProcessing();
    int32_t retCodec = codecBase_->Flush();
    CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    if (retPostProcessing + retCodec != AVCS_ERR_OK) {
        StatusChanged(ERROR);
        return (retPostProcessing != AVCS_ERR_OK) ? retPostProcessing : retCodec;
    }
    StatusChanged(FLUSHED);
    if (framerateCalculator_) {
        framerateCalculator_->OnStopped();
    }
    return AVCS_ERR_OK;
}

int32_t CodecServer::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == RUNNING, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->NotifyEos();
    if (ret == AVCS_ERR_OK) {
        CodecStatus newStatus = END_OF_STREAM;
        StatusChanged(newStatus);
        CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
        if (framerateCalculator_) {
            framerateCalculator_->OnStopped();
        }
    }
    return ret;
}

int32_t CodecServer::Reset()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    drmDecryptor_ = nullptr;
    if (temporalScalability_ != nullptr) {
        if (inputParamTask_ != nullptr) {
            temporalScalability_->SetBlockQueueActive();
            inputParamTask_->Stop();
            inputParamTask_ = nullptr;
        }
        temporalScalability_ = nullptr;
    }
    int32_t ret = codecBase_->Reset();
    CodecStatus newStatus = (ret == AVCS_ERR_OK ? INITIALIZED : ERROR);
    StatusChanged(newStatus);
    ret = ReleasePostProcessing();
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
    }
    CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    lastErrMsg_.clear();
    if (ret == AVCS_ERR_OK) {
        isSurfaceMode_ = false;
        isModeConfirmed_ = false;
        pushBlankBufferOnShutdown_ = false;
    }
    OnInstanceMemoryResetEvent();
    OnInstanceEncodeEndEvent();
    if (framerateCalculator_) {
        framerateCalculator_->OnStopped();
    }
    return ret;
}

int32_t CodecServer::Release()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    drmDecryptor_ = nullptr;
    if (temporalScalability_ != nullptr) {
        if (inputParamTask_ != nullptr) {
            temporalScalability_->SetBlockQueueActive();
            inputParamTask_->Stop();
            inputParamTask_ = nullptr;
        }
        temporalScalability_ = nullptr;
    }
    int32_t ret = codecBase_->Release();
    if (isSurfaceMode_ && codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        SurfaceTools::GetInstance().ReleaseSurface(instanceId_,
            SurfaceUtils::GetInstance()->GetSurface(surfaceId_), pushBlankBufferOnShutdown_, true);
    }
    CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    OnInstanceEncodeEndEvent();
    codecBase_ = nullptr;
    codecBaseCb_ = nullptr;
    (void)ReleasePostProcessing();
    if (ret == AVCS_ERR_OK) {
        isSurfaceMode_ = false;
        isModeConfirmed_ = false;
    }
    if (framerateCalculator_) {
        framerateCalculator_->OnStopped();
    }
    return ret;
}

int32_t CodecServer::GetChannelId(int32_t &channelId)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->GetChannelId(channelId);
    return ret;
}

sptr<Surface> CodecServer::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == CONFIGURED, nullptr, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, nullptr, "Codecbase is nullptr");
    sptr<Surface> surface = codecBase_->CreateInputSurface();
    if (surface != nullptr) {
        isSurfaceMode_ = true;
        isCreateSurface_ = true;
    }
    return surface;
}

int32_t CodecServer::SetInputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == CONFIGURED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->SetInputSurface(surface);
    isSurfaceMode_ = (ret == AVCS_ERR_OK);
    return ret;
}

int32_t CodecServer::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    bool isBufferMode = isModeConfirmed_ && !isSurfaceMode_;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(!isBufferMode, AVCS_ERR_INVALID_OPERATION, "In buffer mode");

    bool isValidState = isModeConfirmed_ ? isSurfaceMode_ && (status_ == CONFIGURED || status_ == RUNNING ||
                                                              status_ == FLUSHED || status_ == END_OF_STREAM)
                                         : status_ == CONFIGURED;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(isValidState, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(surface != nullptr, AVCS_ERR_NO_MEMORY, "Surface is nullptr");
    GSError gsRet;
    if (isLpp_) {
        gsRet = surface->SetSurfaceSourceType(OHSurfaceSource::OH_SURFACE_SOURCE_LOWPOWERVIDEO);
    } else {
        gsRet = surface->SetSurfaceSourceType(OHSurfaceSource::OH_SURFACE_SOURCE_VIDEO);
    }
    EXPECT_AND_LOGW_WITH_TAG(gsRet != GSERROR_OK, "Set surface source type failed, %{public}s",
                             GSErrorStr(gsRet).c_str());
    int32_t ret = AVCS_ERR_OK;
    if (postProcessing_) {
        ret = SetOutputSurfaceForPostProcessing(surface);
    } else {
        ret = codecBase_->SetOutputSurface(surface);
    }
    surfaceId_ = surface->GetUniqueId();
    isSurfaceMode_ = (ret == AVCS_ERR_OK);
#ifdef EMULATOR_ENABLED
    Format config_emulator;
    config_emulator.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    codecBase_->SetParameter(config_emulator);
#endif
    return ret;
}

int32_t CodecServer::SetLowPowerPlayerMode(bool isLpp)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    isLpp_ = isLpp;
    AVCODEC_LOGI("CodecServer::SetLowPowerPlayerMode: %{public}d", isLpp_);
    int32_t ret = codecBase_->SetLowPowerPlayerMode(isLpp);
    if (ret != AVCS_ERR_OK) {
        isLpp_ = false;
    }
    return ret;
}

int32_t CodecServer::DrmVideoCencDecrypt(uint32_t index)
{
    int32_t ret = AVCS_ERR_OK;
    if (drmDecryptor_ != nullptr) {
        if (decryptVideoBufs_.find(index) != decryptVideoBufs_.end()) {
            uint32_t dataSize = static_cast<uint32_t>(decryptVideoBufs_[index].inBuf->memory_->GetSize());
            decryptVideoBufs_[index].outBuf->pts_ = decryptVideoBufs_[index].inBuf->pts_;
            decryptVideoBufs_[index].outBuf->dts_ = decryptVideoBufs_[index].inBuf->dts_;
            decryptVideoBufs_[index].outBuf->duration_ = decryptVideoBufs_[index].inBuf->duration_;
            decryptVideoBufs_[index].outBuf->flag_ = decryptVideoBufs_[index].inBuf->flag_;
            if (decryptVideoBufs_[index].inBuf->meta_ != nullptr) {
                *(decryptVideoBufs_[index].outBuf->meta_) = *(decryptVideoBufs_[index].inBuf->meta_);
            }
            if (dataSize == 0) {
                decryptVideoBufs_[index].outBuf->memory_->SetSize(dataSize);
                return ret;
            }
            // LCOV_EXCL_START
            drmDecryptor_->SetCodecName(codecName_);
            ret = drmDecryptor_->DrmVideoCencDecrypt(decryptVideoBufs_[index].inBuf, decryptVideoBufs_[index].outBuf,
                                                     dataSize);
            decryptVideoBufs_[index].outBuf->memory_->SetSize(dataSize);
            // LCOV_EXCL_STOP
        }
    }
    return ret;
}

int32_t CodecServer::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_lock<std::shared_mutex> freeLock(freeMutex_);
    if (isFree_) {
        AVCODEC_LOGE_WITH_TAG("In invalid state, free out");
        return AVCS_ERR_INVALID_STATE;
    }

    (void)info;
    int32_t ret = AVCS_ERR_OK;
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        std::lock_guard<std::shared_mutex> lock(mutex_);
        ret = QueueInputBufferIn(index);
        if (ret == AVCS_ERR_OK) {
            if (framerateCalculator_) {
                framerateCalculator_->OnStopped();
            }
            CodecStatus newStatus = END_OF_STREAM;
            StatusChanged(newStatus);
        }
    } else {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        ret = QueueInputBufferIn(index);
    }
    return ret;
}

int32_t CodecServer::QueueInputBufferIn(uint32_t index)
{
    int32_t ret = AVCS_ERR_OK;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(
        status_ == RUNNING || ((isSetParameterCb_ || inputParamTask_ != nullptr) && status_ == END_OF_STREAM),
        AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (temporalScalability_ != nullptr) {
        temporalScalability_->ConfigureLTR(index);
    }
    if (videoCb_ != nullptr) {
        ret = DrmVideoCencDecrypt(index);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, AVCS_ERR_DECRYPT_FAILED, "CodecServer decrypt failed");
        ret = codecBase_->QueueInputBuffer(index);
    }
    return ret;
}

int32_t CodecServer::QueueInputBuffer(uint32_t index)
{
    (void)index;
    return AVCS_ERR_UNSUPPORT;
}

int32_t CodecServer::QueueInputParameter(uint32_t index)
{
    (void)index;
    return AVCS_ERR_UNSUPPORT;
}

int32_t CodecServer::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ != UNINITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (postProcessing_) {
        int32_t ret = codecBase_->GetOutputFormat(format);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "GetOutputFormat failed");
        return GetPostProcessingOutputFormat(format);
    } else {
        return codecBase_->GetOutputFormat(format);
    }
}

// LCOV_EXCL_START
int32_t CodecServer::CheckDrmSvpConsistency(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    bool svpFlag)
{
    AVCODEC_LOGI_WITH_TAG("CheckDrmSvpConsistency");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(keySession != nullptr, AVCS_ERR_INVALID_VAL, "keySession is nullptr");
    std::string tmpName = codecName_;
    transform(tmpName.begin(), tmpName.end(), tmpName.begin(), ::tolower);

    // check codec name when secure video path is false
    if (svpFlag == false) {
        if (tmpName.find(".secure") != std::string::npos) {
            AVCODEC_LOGE_WITH_TAG("CheckDrmSvpConsistency failed, svpFlag is false but the decoder is secure!");
            return AVCS_ERR_INVALID_VAL;
        }
        return AVCS_ERR_OK;
    }

    // check codec name when secure video path is true
    if (tmpName.find(".secure") == std::string::npos) {
        AVCODEC_LOGE_WITH_TAG("CheckDrmSvpConsistency failed, svpFlag is true but the decoder is not secure!");
        return AVCS_ERR_INVALID_VAL;
    }

    // check session level when secure video path is true
#ifdef SUPPORT_DRM
    DrmStandard::ContentProtectionLevel sessionLevel;
    int ret = keySession->GetContentProtectionLevel(sessionLevel);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == 0, AVCS_ERR_INVALID_VAL, "GetContentProtectionLevel failed");
    if (sessionLevel <
        DrmStandard::ContentProtectionLevel::CONTENT_PROTECTION_LEVEL_HW_CRYPTO) {
        AVCODEC_LOGE("CheckDrmSvpConsistency failed, key session's content protection level is too low!");
        return AVCS_ERR_INVALID_VAL;
    }
#endif

    return AVCS_ERR_OK;
}
// LCOV_EXCL_STOP

#ifdef SUPPORT_DRM
int32_t CodecServer::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_LOGI_WITH_TAG("CodecServer::SetDecryptConfig");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret = CheckDrmSvpConsistency(keySession, svpFlag);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_VAL, "check svp failed");

    if (drmDecryptor_ == nullptr) {
        drmDecryptor_ = std::make_shared<CodecDrmDecrypt>();
    }
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(drmDecryptor_ != nullptr, AVCS_ERR_NO_MEMORY, "drmDecryptor is nullptr");
    drmDecryptor_->SetDecryptionConfig(keySession, svpFlag);
    return AVCS_ERR_OK;
}
#endif

int32_t CodecServer::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> freeLock(freeMutex_);
    if (isFree_) {
        AVCODEC_LOGE_WITH_TAG("In invalid state, free out");
        return AVCS_ERR_INVALID_STATE;
    }
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == RUNNING || status_ == END_OF_STREAM, AVCS_ERR_INVALID_STATE,
                                      "In invalid state, %{public}s", GetStatusDescription(status_).data());

    if (framerateCalculator_ && status_ == RUNNING) {
        framerateCalculator_->OnFrameConsumed();
    }
    if (postProcessing_) {
        return ReleaseOutputBufferOfPostProcessing(index, render);
    } else {
        return ReleaseOutputBufferOfCodec(index, render);
    }
}

int32_t CodecServer::ReleaseOutputBufferOfCodec(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret;
    if (render) {
        ret = codecBase_->RenderOutputBuffer(index);
    } else {
        ret = codecBase_->ReleaseOutputBuffer(index);
    }
    return ret;
}

void CodecServer::OnInstanceMemoryUpdateEvent(std::shared_ptr<Media::Meta> meta)
{
    if (meta == nullptr) {
        Format format;
        (void)(codecType_ == AVCODEC_TYPE_VIDEO_DECODER ? codecBase_->GetOutputFormat(format)
                                                        : codecBase_->GetInputFormat(format));
        meta = format.GetMeta();
    }
    if (meta == nullptr) {
        return;
    }

    meta->SetData(EventInfoExtentedKey::CODEC_TYPE.data(), codecType_);
    meta->SetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId_);
    meta->SetData(EventInfoExtentedKey::ENABLE_POST_PROCESSING.data(), postProcessing_ != nullptr);
    meta->SetData(Media::Tag::MIME_TYPE, codecMime_);
    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_MEMORY_UPDATE, *meta);
}

void CodecServer::OnInstanceMemoryResetEvent(std::shared_ptr<Media::Meta> meta)
{
    if (meta == nullptr) {
        meta = std::make_shared<Media::Meta>();
    }
    meta->SetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId_);
    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_MEMORY_RESET, *meta);
}

void CodecServer::SetInstanceEncodeEventInfo(std::shared_ptr<Media::Meta> &meta)
{
    bool isForwardCaller = forwardCaller_.pid >= 0 || !forwardCaller_.processName.empty();
    const pid_t &pid = isForwardCaller ? forwardCaller_.pid : caller_.pid;
    const uid_t &uid = isForwardCaller ? forwardCaller_.uid : caller_.uid;
    const std::string &processName = isForwardCaller ? forwardCaller_.processName : caller_.processName;
    meta->SetData(Tag::AV_CODEC_CALLER_PID, pid);
    meta->SetData(Tag::AV_CODEC_CALLER_UID, uid);
    meta->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, processName);
    meta->SetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId_);
}

void CodecServer::OnInstanceEncodeBeginEvent(std::shared_ptr<Media::Meta> meta)
{
    if (codecType_ != AVCODEC_TYPE_VIDEO_ENCODER) {
        return;
    }
    meta = (meta == nullptr) ? std::make_shared<Media::Meta>() : meta;
    SetInstanceEncodeEventInfo(meta);
    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_ENCODE_BEGIN, *meta);
}

void CodecServer::OnInstanceEncodeEndEvent(std::shared_ptr<Media::Meta> meta)
{
    if (codecType_ != AVCODEC_TYPE_VIDEO_ENCODER) {
        return;
    }
    meta = (meta == nullptr) ? std::make_shared<Media::Meta>() : meta;
    SetInstanceEncodeEventInfo(meta);
    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_ENCODE_END, *meta);
}

void CodecServer::InitFramerateCalculator(Meta &callerInfo)
{
    if (codecType_ == AVCODEC_TYPE_VIDEO_ENCODER || codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        framerateCalculator_ = std::make_shared<FramerateCalculator>(instanceId_,
            [weakCodecBase = std::weak_ptr<CodecBase>(codecBase_)](double framerate) {
                auto codecBase = weakCodecBase.lock();
                if (!codecBase) {
                    return;
                }
                Format format;
                format.PutDoubleValue(Tag::VIDEO_OPERATING_RATE, framerate);
                codecBase->SetParameter(format);
            }
        );
        if (framerateCalculator_) {
            framerateCalculator_->SetTag(CreateVideoLogTag(callerInfo));
        }
    }
}

int32_t CodecServer::RenderOutputBufferAtTime(uint32_t index, [[maybe_unused]]int64_t renderTimestampNs)
{
    return ReleaseOutputBuffer(index, true);
}

int32_t CodecServer::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ != INITIALIZED && status_ != CONFIGURED, AVCS_ERR_INVALID_STATE,
                                      "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    if (codecType_ == AVCODEC_TYPE_VIDEO_ENCODER || codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        Format oldFormat;
        int32_t ret = codecBase_->GetOutputFormat(oldFormat);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Failed to get codec format");
        ret = CodecParamChecker::CheckParameterValid(format, oldFormat, codecName_, scenario_);
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Params in format is not valid");
    }

    if (framerateCalculator_) {
        auto framerate = 0.0;
        if (format.GetDoubleValue(Tag::VIDEO_FRAME_RATE, framerate)) {
            framerateCalculator_->SetConfiguredFramerate(framerate);
        }
    }

    return codecBase_->SetParameter(format);
}

int32_t CodecServer::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    (void)callback;
    return AVCS_ERR_OK;
}

int32_t CodecServer::SetCallback(const std::shared_ptr<MediaCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> cbLock(cbMutex_);
    videoCb_ = callback;
    return AVCS_ERR_OK;
}

int32_t CodecServer::SetCallback(const std::shared_ptr<MediaCodecParameterCallback> &callback)
{
    (void)callback;
    return AVCS_ERR_UNSUPPORT;
}

int32_t CodecServer::SetCallback(const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback)
{
    (void)callback;
    return AVCS_ERR_UNSUPPORT;
}

int32_t CodecServer::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(
        status_ == CONFIGURED || status_ == RUNNING || status_ == FLUSHED || status_ == END_OF_STREAM,
        AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->GetInputFormat(format);
}

int32_t CodecServer::GetCodecInfo(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ != UNINITIALIZED,
        AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    std::shared_ptr<CodecListCore> codecListCore = std::make_shared<CodecListCore>();
    CodecType codecType = codecListCore->FindCodecType(codecName_);
    format.PutIntValue(Tag::MEDIA_IS_HARDWARE, codecType == CodecType::AVCODEC_HCODEC ? 1 : 0);

    return AVCS_ERR_OK;
}

void CodecServer::SetDumpInfo(bool isDump, uint64_t instanceId)
{
    CHECK_AND_RETURN_LOG(codecBase_ != nullptr, "Codecbase is nullptr");
    return codecBase_->SetDumpInfo(isDump, instanceId);
}

int32_t CodecServer::DumpInfo(int32_t fd)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(fd >= 0, AVCS_ERR_OK, "Get a invalid fd");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    Format codecFormat;
    int32_t ret = codecBase_->GetOutputFormat(codecFormat);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Get codec format failed");

    AVCodecDumpControler dumpControler;
    auto statusIt = CODEC_STATE_MAP.find(status_);
    if (forwardCaller_.pid != -1 || !forwardCaller_.processName.empty()) {
        dumpControler.AddInfo(DUMP_INDEX_FORWARD_CALLER_PID, "ForwardCallerPid", std::to_string(forwardCaller_.pid));
        dumpControler.AddInfo(DUMP_INDEX_FORWARD_CALLER_PROCESS_NAME,
            "ForwardCallerProcessName", forwardCaller_.processName);
    }
    dumpControler.AddInfo(DUMP_INDEX_CALLER_PID, "CallerPid", std::to_string(caller_.pid));
    dumpControler.AddInfo(DUMP_INDEX_CALLER_PROCESS_NAME, "CallerProcessName", caller_.processName);
    dumpControler.AddInfo(DUMP_INDEX_INSTANCE_ID, "InstanceId", std::to_string(instanceId_));
    dumpControler.AddInfo(DUMP_INDEX_STATUS, "Status", statusIt != CODEC_STATE_MAP.end() ? statusIt->second : "");
    dumpControler.AddInfo(DUMP_INDEX_LAST_ERROR, "LastError", lastErrMsg_.size() ? lastErrMsg_ : "Null");

    uint32_t infoIndex = 1;
    for (auto iter : VIDEO_DUMP_TABLE) {
        uint32_t dumpIndex = DUMP_INDEX_CODEC_INFO_START + (infoIndex++ << DUMP_OFFSET_8);
        if (iter.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
            dumpControler.AddInfoFromFormatWithMapping(dumpIndex, codecFormat,
                                                       iter.first, iter.second, PIXEL_FORMAT_STRING_MAP);
        } else if (iter.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
            dumpControler.AddInfoFromFormatWithMapping(dumpIndex, codecFormat,
                                                       iter.first, iter.second, SCALE_TYPE_STRING_MAP);
        } else {
            dumpControler.AddInfoFromFormat(dumpIndex, codecFormat, iter.first, iter.second);
        }
    }
    std::string dumpString;
    dumpControler.GetDumpString(dumpString);
    dumpString += codecBase_->GetHidumperInfo();
    write(fd, dumpString.c_str(), dumpString.size());
    return AVCS_ERR_OK;
}

void CodecServer::SetInstanceInfo(AVCodecType type, bool isMimeType, const std::string &name, Meta &callerInfo)
{
    callerInfo.GetData(Tag::AV_CODEC_CALLER_PID, caller_.pid);
    callerInfo.GetData(Tag::AV_CODEC_CALLER_UID, caller_.uid);
    callerInfo.GetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, caller_.processName);
    callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_PID, forwardCaller_.pid);
    callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_UID, forwardCaller_.uid);
    callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardCaller_.processName);
    if (caller_.pid == -1) {
        caller_.pid = getprocpid();
        caller_.uid = getuid();
    }

    codecType_ = type;
    codecMime_ = isMimeType ? name : CodecAbilitySingleton::GetInstance().GetMimeByCodecName(name);
    vcodecType_ = static_cast<VideoCodecType>(
        CodecAbilitySingleton::GetInstance().GetVideoCodecTypeByCodecName(codecName_));
    callerInfo.SetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId_);
    callerInfo.SetData(Tag::MEDIA_CODEC_NAME, codecName_);
    callerInfo.SetData(Tag::MIME_TYPE, codecMime_);
    callerInfo.SetData(EventInfoExtentedKey::CODEC_TYPE.data(), type);
    callerInfo.SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), vcodecType_);
    EventManager::GetInstance().OnInstanceEvent(EventType::INSTANCE_INIT, callerInfo);
    EventManager::GetInstance().OnInstanceEvent(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, callerInfo);

    EXPECT_AND_LOGI_WITH_TAG((forwardCaller_.pid >= 0) || (!forwardCaller_.processName.empty()),
                             "Forward caller pid: %{public}d, process name: %{public}s", forwardCaller_.pid,
                             forwardCaller_.processName.c_str());
    AVCODEC_LOGI_WITH_TAG("Caller pid: %{public}d, process name: %{public}s", caller_.pid, caller_.processName.c_str());
}

std::shared_ptr<Media::Meta> CodecServer::GetCallerInfo()
{
    auto meta = std::make_shared<Media::Meta>();
    meta->SetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId_);
    meta->SetData(Tag::AV_CODEC_CALLER_PID, caller_.pid);
    meta->SetData(Tag::AV_CODEC_FORWARD_CALLER_PID, forwardCaller_.pid);
    return meta;
}

inline const std::string &CodecServer::GetStatusDescription(CodecStatus status)
{
    if (status < UNINITIALIZED || status >= ERROR) {
        return CODEC_STATE_MAP.at(ERROR);
    }
    return CODEC_STATE_MAP.at(status);
}

inline void CodecServer::StatusChanged(CodecStatus newStatus, bool printLog)
{
    if (status_ == newStatus) {
        return;
    }
    if (newStatus == ERROR && videoCb_ != nullptr &&
        (codecType_ == AVCODEC_TYPE_VIDEO_ENCODER || codecType_ == AVCODEC_TYPE_VIDEO_DECODER)) {
        videoCb_->OnError(AVCODEC_ERROR_FRAMEWORK_FAILED, AVCS_ERR_INVALID_STATE);
    }
    EXPECT_AND_LOGI_WITH_TAG(printLog, "%{public}s -> %{public}s", GetStatusDescription(status_).data(),
                             GetStatusDescription(newStatus).data());
    status_ = newStatus;
}

void CodecServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    Media::Meta meta;
    meta.SetData(Tag::MIME_TYPE, codecMime_);
    meta.SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), errorCode);
    meta.SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), vcodecType_);
    EventManager::GetInstance().OnInstanceEvent(StatisticsEventType::CODEC_ERROR_INFO, meta);
    lastErrMsg_ = AVCSErrorToString(static_cast<AVCodecServiceErrCode>(errorCode));
    AVCODEC_LOGW_WITH_TAG("%{public}s", lastErrMsg_.c_str());
    if (videoCb_ != nullptr) {
        videoCb_->OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
    }
}

void CodecServer::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (postProcessing_) {
        outputFormatChanged_ = format;
        return;
    }
    if (videoCb_ != nullptr) {
        videoCb_->OnOutputFormatChanged(format);
    }
    auto formatTemp = format;
    OnInstanceMemoryUpdateEvent(formatTemp.GetMeta());
}

void CodecServer::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    if (temporalScalability_ != nullptr) {
        temporalScalability_->StoreAVBuffer(index, buffer);
    }
    if (videoCb_ == nullptr || (isCreateSurface_ && !isSetParameterCb_)) {
        return;
    }
    if (drmDecryptor_ != nullptr) {
        if (decryptVideoBufs_.find(index) != decryptVideoBufs_.end()) {
            videoCb_->OnInputBufferAvailable(index, decryptVideoBufs_[index].inBuf);
            return;
        }
        DrmDecryptVideoBuf decryptVideoBuf;
        MemoryFlag memFlag = MEMORY_READ_WRITE;
        std::shared_ptr<AVAllocator> avAllocator = AVAllocatorFactory::CreateSharedAllocator(memFlag);
        if (avAllocator == nullptr) {
            AVCODEC_LOGE_WITH_TAG("CreateSharedAllocator failed");
            return;
        }
        decryptVideoBuf.inBuf = AVBuffer::CreateAVBuffer(avAllocator,
            static_cast<int32_t>(buffer->memory_->GetCapacity()));
        if (decryptVideoBuf.inBuf == nullptr || decryptVideoBuf.inBuf->memory_ == nullptr ||
            decryptVideoBuf.inBuf->memory_->GetCapacity() != static_cast<int32_t>(buffer->memory_->GetCapacity())) {
            AVCODEC_LOGE_WITH_TAG("CreateAVBuffer failed");
            return;
        }
        decryptVideoBuf.outBuf = buffer;
        decryptVideoBufs_.insert({index, decryptVideoBuf});
        videoCb_->OnInputBufferAvailable(index, decryptVideoBuf.inBuf);
    } else {
        videoCb_->OnInputBufferAvailable(index, buffer);
    }
}

void CodecServer::OnOutputBufferBinded(std::map<uint32_t, sptr<SurfaceBuffer>> &bufferMap)
{
    CHECK_AND_RETURN_LOG(videoCb_ != nullptr, "videoCb_ is nullptr!");
    videoCb_->OnOutputBufferBinded(bufferMap);
}
 
void CodecServer::OnOutputBufferUnbinded()
{
    CHECK_AND_RETURN_LOG(videoCb_ != nullptr, "videoCb_ is nullptr!");
    videoCb_->OnOutputBufferUnbinded();
}

void CodecServer::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "buffer is nullptr!");

    if (temporalScalability_ != nullptr && !(buffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA)) {
        temporalScalability_->SetDisposableFlag(buffer);
    }

    if (postProcessing_) {
        /*
            If post processing is configured, this callback flow is intercepted here. Just push the decoded buffer info
            {index, buffer} into the decodedBufferInfoQueue_ which is monitored by another task thread. Once the queue
            has data, the thread will pop the data from the queue and calls "CodecServer::ReleaseOutputBuffer" to flush
            it into video processing engine. The video processing engine will automatically processing the frame
            according to the index. The callback ipc proxy's function "videoCb_->OnOutputBufferAvailable" is called
            later in "PostProcessingOnOutputBufferAvailable" by video processing engine when the frame is processed
            to notify application that the frame is ready. At last, application calls
            "OH_VideoDecoder_RenderOutputBuffer" or "OH_VideoDecoder_FreeOutputBuffer" to flush the frame.
        */
        (void)PushDecodedBufferInfo(index, buffer);
    } else {
        std::shared_lock<std::shared_mutex> lock(cbMutex_);
        CHECK_AND_RETURN_LOG_WITH_TAG(videoCb_ != nullptr, "videoCb_ is nullptr!");
        videoCb_->OnOutputBufferAvailable(index, buffer);
    }
}

CodecBaseCallback::CodecBaseCallback(const std::shared_ptr<CodecServer> &codec) : weakCodec_(codec)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecBaseCallback::~CodecBaseCallback()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecBaseCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    std::shared_ptr<CodecServer> codec = weakCodec_.lock();
    CHECK_AND_RETURN_LOG(codec != nullptr, "codec is nullptr!");
    codec->OnError(errorType, errorCode);
}

void CodecBaseCallback::OnOutputFormatChanged(const Format &format)
{
    std::shared_ptr<CodecServer> codec = weakCodec_.lock();
    CHECK_AND_RETURN_LOG(codec != nullptr, "codec is nullptr!");
    codec->OnOutputFormatChanged(format);
}

void CodecBaseCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    std::shared_ptr<CodecServer> codec = weakCodec_.lock();
    CHECK_AND_RETURN_LOG(codec != nullptr, "codec is nullptr!");
    codec->OnInputBufferAvailable(index, buffer);
}

void CodecBaseCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    std::shared_ptr<CodecServer> codec = weakCodec_.lock();
    CHECK_AND_RETURN_LOG(codec != nullptr, "codec is nullptr!");
    codec->OnOutputBufferAvailable(index, buffer);
}

void CodecBaseCallback::OnOutputBufferBinded(std::map<uint32_t, sptr<SurfaceBuffer>> &bufferMap)
{
    std::shared_ptr<CodecServer> codec = weakCodec_.lock();
    CHECK_AND_RETURN_LOG(codec != nullptr, "codec is nullptr!");
    codec->OnOutputBufferBinded(bufferMap);
}

void CodecBaseCallback::OnOutputBufferUnbinded()
{
    std::shared_ptr<CodecServer> codec = weakCodec_.lock();
    CHECK_AND_RETURN_LOG(codec != nullptr, "codec is nullptr!");
    codec->OnOutputBufferUnbinded();
}

int32_t CodecServer::GetCodecDfxInfo(CodecDfxInfo &codecDfxInfo)
{
    Format format;
    codecBase_->GetOutputFormat(format);
    int32_t videoPixelFormat = static_cast<int32_t>(VideoPixelFormat::UNKNOWN);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, videoPixelFormat);
    videoPixelFormat = PIXEL_FORMAT_STRING_MAP.find(videoPixelFormat) != PIXEL_FORMAT_STRING_MAP.end()
                           ? videoPixelFormat
                           : static_cast<int32_t>(VideoPixelFormat::UNKNOWN);
    int32_t codecIsVendor = 0;
    format.GetIntValue("IS_VENDOR", codecIsVendor);

    codecDfxInfo.clientPid = caller_.pid;
    codecDfxInfo.clientUid = caller_.uid;
    codecDfxInfo.codecInstanceId = FAKE_POINTER(this);
    format.GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, codecDfxInfo.codecName);
    codecDfxInfo.codecIsVendor = codecIsVendor == 1 ? "True" : "False";
    codecDfxInfo.codecMode = isSurfaceMode_ ? "Surface mode" : "Buffer Mode";
    format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, codecDfxInfo.encoderBitRate);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, codecDfxInfo.videoWidth);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, codecDfxInfo.videoHeight);
    format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, codecDfxInfo.videoFrameRate);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, codecDfxInfo.audioChannelCount);
    codecDfxInfo.videoPixelFormat =
        codecDfxInfo.audioChannelCount == 0 ? PIXEL_FORMAT_STRING_MAP.at(videoPixelFormat) : "";
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, codecDfxInfo.audioSampleRate);
    return 0;
}

int32_t CodecServer::Configure(const std::shared_ptr<Media::Meta> &meta)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(meta != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret = codecBase_->Configure(meta);

    CodecStatus newStatus = (ret == AVCS_ERR_OK ? CONFIGURED : ERROR);
    StatusChanged(newStatus);
    return ret;
}
int32_t CodecServer::SetParameter(const std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(parameter != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ != INITIALIZED && status_ != CONFIGURED, AVCS_ERR_INVALID_STATE,
                                      "In invalid state, %{public}s", GetStatusDescription(status_).data());
    return codecBase_->SetParameter(parameter);
}
int32_t CodecServer::GetOutputFormat(std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(status_ != UNINITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                                      GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "codecBase is nullptr");
    return codecBase_->GetOutputFormat(parameter);
}

int32_t CodecServer::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "codecBase_ is nullptr");
    switch (codecType_) {
        case AVCODEC_TYPE_VIDEO_DECODER:
            // Post processing is only available for video decoder.
            return PreparePostProcessing();
        case AVCODEC_TYPE_VIDEO_ENCODER:
            return AVCS_ERR_OK;
        default:
            // Audio's interface "Prepare"
            return codecBase_->Prepare();
    }
}

void CodecServer::ProcessInputBufferInner(bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(codecBase_ != nullptr, "ProcessInputBufferInner codecBase is nullptr");
    return codecBase_->ProcessInputBufferInner(isTriggeredByOutPort, isFlushed, bufferStatus);
}

void CodecServer::ProcessInputBuffer()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_LOG(codecBase_ != nullptr, "codecBase_ is nullptr");
    return codecBase_->ProcessInputBuffer();
}

bool CodecServer::CheckRunning()
{
    if (status_ == CodecServer::RUNNING) {
        return true;
    }
    return false;
}

void CodecServer::SetFreeStatus(bool isFree)
{
    std::lock_guard<std::shared_mutex> lock(freeMutex_);
    isFree_ = isFree;
}

int32_t CodecServer::CreatePostProcessing(const Format& format)
{
    if (codecType_ != AVCODEC_TYPE_VIDEO_DECODER) {
        return AVCS_ERR_OK;
    }
    int32_t colorSpaceType;
    if (!format.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE, colorSpaceType)) {
        return AVCS_ERR_OK;
    }
    auto capData = CodecAbilitySingleton::GetInstance().GetCapabilityByName(codecName_);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(capData != std::nullopt && capData->isVendor, AVCS_ERR_UNKNOWN,
                                      "Get codec capability from codec list failed");
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(codecBase_, AVCS_ERR_UNKNOWN, "Decoder is not found");
    int32_t ret;
    postProcessing_ = PostProcessingType::Create(codecBase_, format, ret);
    EXPECT_AND_LOGI_WITH_TAG(postProcessing_, "Post processing is configured");
    return ret;
}

int32_t CodecServer::SetCallbackForPostProcessing()
{
    using namespace std::placeholders;
    postProcessingCallback_.onError = std::bind(PostProcessingCallbackOnError, _1, _2);
    postProcessingCallback_.onOutputBufferAvailable =
        std::bind(PostProcessingCallbackOnOutputBufferAvailable, _1, _2, _3);
    postProcessingCallback_.onOutputFormatChanged = std::bind(PostProcessingCallbackOnOutputFormatChanged, _1, _2);
    auto userData = new PostProcessingCallbackUserData;
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(userData != nullptr, AVCS_ERR_NO_MEMORY,
                                      "Failed to create post processing callback userdata");
    userData->codecServer = shared_from_this();
    postProcessingUserData_ = static_cast<void *>(userData);
    return postProcessing_->SetCallback(postProcessingCallback_, postProcessingUserData_);
}

void CodecServer::ClearCallbackForPostProcessing()
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    postProcessingCallback_.onError = nullptr;
    postProcessingCallback_.onOutputBufferAvailable = nullptr;
}

int32_t CodecServer::SetOutputSurfaceForPostProcessing(sptr<Surface> surface)
{
    int32_t ret = postProcessing_->SetOutputSurface(surface);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Set output surface failed");
    return ret;
}

int32_t CodecServer::PreparePostProcessing()
{
    if (!postProcessing_) {
        return AVCS_ERR_OK;
    }

    int32_t ret{AVCS_ERR_OK};
    if (postProcessingUserData_ == nullptr) {
        std::lock_guard<std::shared_mutex> lock(cbMutex_);
        ret = SetCallbackForPostProcessing();
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Set callback for post post processing failed");
    }

    if (decodedBufferInfoQueue_ == nullptr) {
        decodedBufferInfoQueue_ = DecodedBufferInfoQueue::Create("DecodedBufferInfoQueue");
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(decodedBufferInfoQueue_, AVCS_ERR_NO_MEMORY,
                                          "Create decoded buffer info queue failed");
    }

    if (postProcessingInputBufferInfoQueue_ == nullptr) {
        postProcessingInputBufferInfoQueue_ =
            PostProcessingBufferInfoQueue::Create("PostProcessingInputBufferInfoQueue");
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(postProcessingInputBufferInfoQueue_, AVCS_ERR_NO_MEMORY,
                                          "Create post processing input buffer info queue failed");
    }

    ret = postProcessing_->Prepare();
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Prepare post processing failed");

    AVCODEC_LOGI_WITH_TAG("Post processing is prepared");
    return AVCS_ERR_OK;
}

int32_t CodecServer::StartPostProcessing()
{
    if (!postProcessing_) {
        return AVCS_ERR_OK;
    }
    int32_t ret = postProcessing_->Start();
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
        return ret;
    }
    ret = StartPostProcessingTask();
    if (ret != AVCS_ERR_OK) {
        StatusChanged(ERROR);
        return ret;
    }
    AVCODEC_LOGI_WITH_TAG("Post processing is started");
    return ret;
}

int32_t CodecServer::StopPostProcessing()
{
    DeactivatePostProcessingQueue();
    if (postProcessingTask_) {
        postProcessingTask_->Stop();
    }
    if (postProcessing_) {
        int32_t ret = postProcessing_->Stop();
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Stop post processing failed");
        AVCODEC_LOGI_WITH_TAG("Post processing is stopped");
    }
    if (decodedBufferInfoQueue_) {
        decodedBufferInfoQueue_->Clear();
    }
    if (postProcessingInputBufferInfoQueue_) {
        postProcessingInputBufferInfoQueue_->Clear();
    }

    return AVCS_ERR_OK;
}

int32_t CodecServer::FlushPostProcessing()
{
    if (!postProcessing_) {
        return AVCS_ERR_OK;
    }
    DeactivatePostProcessingQueue();
    if (postProcessingTask_) {
        postProcessingTask_->Pause();
    }
    auto ret = postProcessing_->Flush();
    if (decodedBufferInfoQueue_) {
        decodedBufferInfoQueue_->Clear();
    }
    if (postProcessingInputBufferInfoQueue_) {
        postProcessingInputBufferInfoQueue_->Clear();
    }

    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == AVCS_ERR_OK, ret, "Flush post processing failed");
    AVCODEC_LOGI_WITH_TAG("Post processing is flushed");
    return AVCS_ERR_OK;
}

int32_t CodecServer::ResetPostProcessing()
{
    if (postProcessing_) {
        DeactivatePostProcessingQueue();
        if (postProcessingTask_) {
            postProcessingTask_->Stop();
        }
        postProcessing_->Reset();
        CleanPostProcessingResource();
        postProcessing_.reset();
        AVCODEC_LOGI_WITH_TAG("Post processing is reset");
    }
    return AVCS_ERR_OK;
}

int32_t CodecServer::ReleasePostProcessing()
{
    if (postProcessing_) {
        DeactivatePostProcessingQueue();
        if (postProcessingTask_) {
            postProcessingTask_->Stop();
        }
        postProcessing_->Release();
        CleanPostProcessingResource();
        postProcessing_.reset();
        AVCODEC_LOGI_WITH_TAG("Post processing is released");
    }

    if (postProcessingUserData_ != nullptr) {
        auto p = static_cast<PostProcessingCallbackUserData*>(postProcessingUserData_);
        p->codecServer.reset();
        delete p;
        postProcessingUserData_ = nullptr;
    }

    return AVCS_ERR_OK;
}

int32_t CodecServer::ReleaseOutputBufferOfPostProcessing(uint32_t index, bool render)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(postProcessing_, AVCS_ERR_UNKNOWN, "Post processing is null");
    return postProcessing_->ReleaseOutputBuffer(index, render);
}

int32_t CodecServer::GetPostProcessingOutputFormat(Format& format)
{
    postProcessing_->GetOutputFormat(format);
    return AVCS_ERR_OK;
}

int32_t CodecServer::PushDecodedBufferInfo(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(buffer != nullptr, AVCS_ERR_UNKNOWN, "buffer is nullptr!");
    auto info = DecodedBufferInfo(index, *buffer);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(decodedBufferInfoQueue_, AVCS_ERR_UNKNOWN, "Queue is null");
    auto ret = decodedBufferInfoQueue_->PushWait(info);
    CHECK_AND_RETURN_RET_LOG_WITH_TAG(ret == QueueResult::OK, AVCS_ERR_UNKNOWN, "Push data failed, %{public}s",
                                      QUEUE_RESULT_DESCRIPTION[static_cast<int32_t>(ret)]);
    return AVCS_ERR_OK;
}

void CodecServer::PostProcessingOnError(int32_t errorCode)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (videoCb_ == nullptr) {
        AVCODEC_LOGD_WITH_TAG("Missing video callback");
        return;
    }
    int32_t ret = VPEErrorToAVCSError(errorCode);
    AVCODEC_LOGW_WITH_TAG("PostProcessingOnError, errorCodec:%{public}d -> %{public}d", errorCode, ret);
    videoCb_->OnError(AVCodecErrorType::AVCODEC_ERROR_INTERNAL, ret);
}

void CodecServer::PostProcessingOnOutputBufferAvailable(uint32_t index, int32_t flag)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (videoCb_ == nullptr) {
        AVCODEC_LOGD_WITH_TAG("Missing video callback");
        return;
    }
    CHECK_AND_RETURN_LOG_WITH_TAG(postProcessingInputBufferInfoQueue_, "Queue is null");
    DecodedBufferInfo info;
    auto ret = postProcessingInputBufferInfoQueue_->PopWait(info);
    CHECK_AND_RETURN_LOG_WITH_TAG(ret == QueueResult::OK, "Get data failed, %{public}s",
                                  QUEUE_RESULT_DESCRIPTION[static_cast<int32_t>(ret)]);
    if (flag == 1) { // 1: EOS flag
        info.flag = AVCODEC_BUFFER_FLAG_EOS;
        AVCODEC_LOGI_WITH_TAG("Catch EOS frame");
    }
    auto buffer = info.CreateAVBuffer();
    CHECK_AND_RETURN_LOG_WITH_TAG(buffer != nullptr, "Create AVBuffer failed");
    videoCb_->OnOutputBufferAvailable(index, buffer);
}

void CodecServer::PostProcessingOnOutputFormatChanged(const Format& format)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (videoCb_ == nullptr) {
        AVCODEC_LOGD_WITH_TAG("Missing video callback");
        return;
    }
    int32_t width = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_WIDTH, width)) {
        outputFormatChanged_.PutIntValue(Media::Tag::VIDEO_WIDTH, width);
        outputFormatChanged_.PutIntValue(Media::Tag::VIDEO_PIC_WIDTH, width);
    }
    int32_t height = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_HEIGHT, height)) {
        outputFormatChanged_.PutIntValue(Media::Tag::VIDEO_HEIGHT, height);
        outputFormatChanged_.PutIntValue(Media::Tag::VIDEO_PIC_HEIGHT, height);
    }
    int32_t stride = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_STRIDE, stride)) {
        outputFormatChanged_.PutIntValue(Media::Tag::VIDEO_STRIDE, stride);
    }
    int32_t sliceHeight = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight)) {
        outputFormatChanged_.PutIntValue(Media::Tag::VIDEO_SLICE_HEIGHT, sliceHeight);
    }
    int32_t outputColorSpace = 0;
    if (format.GetIntValue(Media::Tag::VIDEO_DECODER_OUTPUT_COLOR_SPACE, outputColorSpace)) {
        outputFormatChanged_.PutIntValue(Media::Tag::VIDEO_DECODER_OUTPUT_COLOR_SPACE, outputColorSpace);
    }
    videoCb_->OnOutputFormatChanged(outputFormatChanged_);
}

int32_t CodecServer::StartPostProcessingTask()
{
    if (!postProcessingTask_) {
        postProcessingTask_ = std::make_unique<TaskThread>("PostProcessing");
        CHECK_AND_RETURN_RET_LOG_WITH_TAG(postProcessingTask_, AVCS_ERR_UNKNOWN, "Create task post processing failed");
        std::function<void()> task = std::bind(&CodecServer::PostProcessingTask, this);
        postProcessingTask_->RegisterHandler(task);
    }
    if (decodedBufferInfoQueue_) {
        decodedBufferInfoQueue_->Activate();
    }
    if (postProcessingInputBufferInfoQueue_) {
        postProcessingInputBufferInfoQueue_->Activate();
    }
    postProcessingTask_->Start();

    return AVCS_ERR_OK;
}

void CodecServer::PostProcessingTask()
{
    CHECK_AND_RETURN_LOG_WITH_TAG(decodedBufferInfoQueue_ && postProcessingInputBufferInfoQueue_, "Queue is null");
    DecodedBufferInfo info;
    auto ret = decodedBufferInfoQueue_->PopWait(info);
    CHECK_AND_RETURN_LOG_WITH_TAG(ret == QueueResult::OK, "Get data failed, %{public}s",
                                  QUEUE_RESULT_DESCRIPTION[static_cast<int32_t>(ret)]);
    ret = postProcessingInputBufferInfoQueue_->PushWait(info);
    CHECK_AND_RETURN_LOG_WITH_TAG(ret == QueueResult::OK, "Push data failed, %{public}s",
                                  QUEUE_RESULT_DESCRIPTION[static_cast<int32_t>(ret)]);
    if (info.flag == AVCODEC_BUFFER_FLAG_EOS) {
        AVCODEC_LOGI_WITH_TAG("Catch EOS frame, notify post processing eos");
        postProcessing_->NotifyEos();
    }
    (void)ReleaseOutputBufferOfCodec(info.index, true);
}

void CodecServer::DeactivatePostProcessingQueue()
{
    if (decodedBufferInfoQueue_) {
        decodedBufferInfoQueue_->Deactivate();
    }
    if (postProcessingInputBufferInfoQueue_) {
        postProcessingInputBufferInfoQueue_->Deactivate();
    }
}

void CodecServer::CleanPostProcessingResource()
{
    ClearCallbackForPostProcessing();
    if (postProcessingTask_) {
        postProcessingTask_.reset();
    }
    if (decodedBufferInfoQueue_) {
        decodedBufferInfoQueue_.reset();
    }
    if (postProcessingInputBufferInfoQueue_) {
        postProcessingInputBufferInfoQueue_.reset();
    }
}

int32_t CodecServer::NotifyMemoryRecycle()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == FLUSHED || status_ == END_OF_STREAM,
        AVCS_ERR_INVALID_STATE, "No need to recycle memory, status:%{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->NotifyMemoryRecycle();
}

int32_t CodecServer::NotifyMemoryWriteBack()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->NotifyMemoryWriteBack();
}

int32_t CodecServer::NotifySuspend()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == FLUSHED || status_ == END_OF_STREAM,
        AVCS_ERR_INVALID_STATE, "No need to suspend, status:%{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (framerateCalculator_) {
        framerateCalculator_->OnStopped();
    }
    return codecBase_->NotifySuspend();
}

int32_t CodecServer::NotifyResume()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (framerateCalculator_) {
        framerateCalculator_->SetFramerate2ConfiguredFramerate();
    }
    return codecBase_->NotifyResume();
}

CodecServer::DecodedBufferInfo::DecodedBufferInfo(uint32_t index, AVBuffer &buffer)
{
    this->index = index;
    this->pts   = buffer.pts_;
    this->flag  = buffer.flag_;
}

std::shared_ptr<AVBuffer> CodecServer::DecodedBufferInfo::CreateAVBuffer()
{
    auto buffer = AVBuffer::CreateAVBuffer();
    CHECK_AND_RETURN_RET_LOG(buffer != nullptr, nullptr, "Create AVBuffer failed");
    buffer->pts_  = this->pts;
    buffer->flag_ = this->flag;
    return buffer;
}
} // namespace MediaAVCodec
} // namespace OHOS
