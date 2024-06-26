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

#include "codec_server.h"
#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>
#include "avcodec_codec_name.h"
#include "avcodec_dump_utils.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "avcodec_sysevent.h"
#include "avcodec_trace.h"
#include "buffer/avbuffer.h"
#include "codec_factory.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "surface_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "CodecServer"};
enum DumpIndex : uint32_t {
    DUMP_INDEX_FORWARD_CALLER_PID          = 0x01'01'01'00,
    DUMP_INDEX_FORWARD_CALLER_UID          = 0x01'01'02'00,
    DUMP_INDEX_FORWARD_CALLER_PROCESS_NAME = 0x01'01'03'00,
    DUMP_INDEX_CALLER_PID                  = 0x01'01'04'00,
    DUMP_INDEX_CALLER_UID                  = 0x01'01'05'00,
    DUMP_INDEX_CALLER_PROCESS_NAME         = 0x01'01'06'00,
    DUMP_INDEX_STATUS                      = 0x01'01'07'00,
    DUMP_INDEX_LAST_ERROR                  = 0x01'01'08'00,
    DUMP_INDEX_CODEC_INFO_START            = 0x01'02'00'00,
};
constexpr uint32_t DUMP_OFFSET_8 = 8;

const std::map<OHOS::MediaAVCodec::CodecServer::CodecStatus, std::string> CODEC_STATE_MAP = {
    {OHOS::MediaAVCodec::CodecServer::UNINITIALIZED, "uninitialized"},
    {OHOS::MediaAVCodec::CodecServer::INITIALIZED, "initialized"},
    {OHOS::MediaAVCodec::CodecServer::CONFIGURED, "configured"},
    {OHOS::MediaAVCodec::CodecServer::RUNNING, "running"},
    {OHOS::MediaAVCodec::CodecServer::FLUSHED, "flushed"},
    {OHOS::MediaAVCodec::CodecServer::END_OF_STREAM, "end of stream"},
    {OHOS::MediaAVCodec::CodecServer::ERROR, "error"},
};

const std::vector<std::pair<std::string_view, const std::string>> VIDEO_DUMP_TABLE = {
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "Codec_Name"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, "Width"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, "Height"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_FRAME_RATE, "Frame_Rate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "Bit_Rate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, "Pixel_Format"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_SCALE_TYPE, "Scale_Type"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, "Rotation_Angle"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, "Max_Input_Size"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, "Max_Input_Buffer_Count"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, "Max_Output_Buffer_Count"},
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
    {OHOS::ScalingMode::SCALING_MODE_SCALE_TO_WINDOW, "Scale_to_window"},
    {OHOS::ScalingMode::SCALING_MODE_SCALE_CROP, "Scale_crop"},
    {OHOS::ScalingMode::SCALING_MODE_NO_SCALE_CROP, "No_scale_crop"},
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
std::shared_ptr<ICodecService> CodecServer::Create()
{
    std::shared_ptr<CodecServer> server = std::make_shared<CodecServer>();

    int32_t ret = server->InitServer();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Server init failed, ret: %{public}d!", ret);
    return server;
}

CodecServer::CodecServer()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecServer::~CodecServer()
{
    std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(&CodecServer::ExitProcessor, this);
    if (thread->joinable()) {
        thread->join();
    }
    (void)mallopt(M_FLUSH_THREAD_CACHE, 0);
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void CodecServer::ExitProcessor()
{
    codecBase_ = nullptr;
}

int32_t CodecServer::InitServer()
{
    return AVCS_ERR_OK;
}

int32_t CodecServer::Init(AVCodecType type, bool isMimeType, const std::string &name,
                          Meta &callerInfo, API_VERSION apiVersion)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
    codecType_ = type;
    codecName_ = name;
    int32_t ret = isMimeType ? InitByMime(callerInfo, apiVersion) : InitByName(callerInfo, apiVersion);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret,
                             "Init failed. isMimeType:(%{public}d), name:(%{public}s), error:(%{public}d)", isMimeType,
                             name.c_str(), ret);
    SetCallerInfo(callerInfo);

    std::shared_ptr<AVCodecCallback> callback = std::make_shared<CodecBaseCallback>(shared_from_this());
    ret = codecBase_->SetCallback(callback);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "SetCallback failed.");

    std::shared_ptr<MediaCodecCallback> videoCallback = std::make_shared<VCodecBaseCallback>(shared_from_this());
    ret = codecBase_->SetCallback(videoCallback);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "SetCallback failed.");

    StatusChanged(INITIALIZED);
    return AVCS_ERR_OK;
}

int32_t CodecServer::InitByName(Meta &callerInfo, API_VERSION apiVersion)
{
    int32_t ret = GetAudioCodecName(codecType_, codecName_);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "CodecName get failed.");

    codecBase_ = CodecFactory::Instance().CreateCodecByName(codecName_, apiVersion);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "CodecBase is nullptr.");
    return codecBase_->Init(callerInfo);
}

int32_t CodecServer::InitByMime(Meta &callerInfo, API_VERSION apiVersion)
{
    int32_t ret = AVCS_ERR_NO_MEMORY;
    bool isEncoder = (codecType_ == AVCODEC_TYPE_VIDEO_ENCODER) || (codecType_ == AVCODEC_TYPE_AUDIO_ENCODER);
    std::vector<std::string> nameArray = CodecFactory::Instance().GetCodecNameArrayByMime(codecName_, isEncoder);
    std::vector<std::string>::iterator iter;
    for (iter = nameArray.begin(); iter != nameArray.end(); ++iter) {
        ret = AVCS_ERR_NO_MEMORY;
        codecBase_ = CodecFactory::Instance().CreateCodecByName(*iter, apiVersion);
        CHECK_AND_CONTINUE_LOG(codecBase_ != nullptr, "Skip creation failure. name:(%{public}s)", iter->c_str());
        ret = codecBase_->Init(callerInfo);
        CHECK_AND_CONTINUE_LOG(ret == AVCS_ERR_OK, "Skip initialization failure. name:(%{public}s)", iter->c_str());
        codecName_ = *iter;
        break;
    }
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "CodecBase is nullptr.");
    return iter == nameArray.end() ? ret : AVCS_ERR_OK;
}

int32_t CodecServer::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    Format config = format;

    int32_t isSetParameterCb = 0;
    format.GetIntValue(Tag::VIDEO_ENCODER_ENABLE_SURFACE_INPUT_CALLBACK, isSetParameterCb);
    isSetParameterCb_ = isSetParameterCb != 0;

    int32_t paramCheckRet = AVCS_ERR_OK;
    if (codecType_ == AVCODEC_TYPE_VIDEO_ENCODER || codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        auto scenario = CodecParamChecker::CheckCodecScenario(config, codecType_, codecName_);
        CHECK_AND_RETURN_RET_LOG(scenario != std::nullopt, AVCS_ERR_INVALID_VAL, "Failed to get codec scenario");
        scenario_ = scenario.value();
        paramCheckRet = CodecParamChecker::CheckConfigureValid(config, codecType_, codecName_, scenario_);
        CHECK_AND_RETURN_RET_LOG(paramCheckRet == AVCS_ERR_OK || paramCheckRet == AVCS_ERR_CODEC_PARAM_INCORRECT,
            paramCheckRet, "Params in format is not valid.");
        CodecScenarioInit(config);
    }

    int32_t ret = codecBase_->Configure(config);
    CodecStatus newStatus = (ret == AVCS_ERR_OK ? CONFIGURED : ERROR);
    StatusChanged(newStatus);
    return (ret == AVCS_ERR_OK && paramCheckRet == AVCS_ERR_CODEC_PARAM_INCORRECT) ? paramCheckRet : ret;
}

int32_t CodecServer::CodecScenarioInit(Format &config)
{
    switch (scenario_) {
        case CodecScenario::CODEC_SCENARIO_ENC_TEMPORAL_SCALABILITY:
            temporalScalability_ = std::make_shared<TemporalScalability>();
            temporalScalability_->ValidateTemporalGopParam(config);
            break;
        default:
            break;
    }

    return AVCS_ERR_OK;
}

void CodecServer::StartInputParamTask()
{
    inputParamTask_ = std::make_shared<TaskThread>("InputParamTask");
    inputParamTask_->RegisterHandler([this] {
        uint32_t index = temporalScalability_->GetFirstBufferIndex();
        AVCodecBufferInfo info;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
        CHECK_AND_RETURN_LOG(QueueInputBuffer(index, info, flag) == AVCS_ERR_OK, "QueueInputBuffer failed");
    });
    inputParamTask_->Start();
}

int32_t CodecServer::Start()
{
    SetFreeStatus(false);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == FLUSHED || status_ == CONFIGURED, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (temporalScalability_ != nullptr && isCreateSurface_ && !isSetParameterCb_) {
        StartInputParamTask();
    }
    int32_t ret = codecBase_->Start();
    CodecStatus newStatus = (ret == AVCS_ERR_OK ? RUNNING : ERROR);
    StatusChanged(newStatus);
    if (ret == AVCS_ERR_OK) {
        isStarted_ = true;
        isModeConfirmed_ = true;
        CodecDfxInfo codecDfxInfo;
        GetCodecDfxInfo(codecDfxInfo);
        CodecStartEventWrite(codecDfxInfo);
    }
    return ret;
}

int32_t CodecServer::Stop()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM || status_ == FLUSHED,
                             AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Stop();
    CodecStatus newStatus = (ret == AVCS_ERR_OK ? CONFIGURED : ERROR);
    StatusChanged(newStatus);
    if (isStarted_ && ret == AVCS_ERR_OK) {
        isStarted_ = false;
        CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    }
    return ret;
}

int32_t CodecServer::Flush()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Flush();
    CodecStatus newStatus = (ret == AVCS_ERR_OK ? FLUSHED : ERROR);
    StatusChanged(newStatus);
    if (isStarted_ && ret == AVCS_ERR_OK) {
        isStarted_ = false;
        CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    }
    return ret;
}

int32_t CodecServer::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->NotifyEos();
    if (ret == AVCS_ERR_OK) {
        CodecStatus newStatus = END_OF_STREAM;
        StatusChanged(newStatus);
        if (isStarted_) {
            isStarted_ = false;
            CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
        }
    }
    return ret;
}

int32_t CodecServer::Reset()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
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
    lastErrMsg_.clear();
    if (isStarted_ && ret == AVCS_ERR_OK) {
        isStarted_ = false;
        isSurfaceMode_ = false;
        isModeConfirmed_ = false;
        CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    }
    return ret;
}

int32_t CodecServer::Release()
{
    SetFreeStatus(true);
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
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
    std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(&CodecServer::ExitProcessor, this);
    if (thread->joinable()) {
        thread->join();
    }
    if (isStarted_ && ret == AVCS_ERR_OK) {
        isStarted_ = false;
        isSurfaceMode_ = false;
        isModeConfirmed_ = false;
        CodecStopEventWrite(caller_.pid, caller_.uid, FAKE_POINTER(this));
    }
    return ret;
}

sptr<Surface> CodecServer::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == CONFIGURED, nullptr, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, nullptr, "Codecbase is nullptr");
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
    CHECK_AND_RETURN_RET_LOG(status_ == CONFIGURED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (surface != nullptr) {
        isSurfaceMode_ = true;
    }
    return codecBase_->SetInputSurface(surface);
}

int32_t CodecServer::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    bool isBufferMode = isModeConfirmed_ && !isSurfaceMode_;
    CHECK_AND_RETURN_RET_LOG(!isBufferMode, AVCS_ERR_INVALID_OPERATION, "In buffer mode.");

    bool isValidState = isModeConfirmed_ ? isSurfaceMode_ && (status_ == CONFIGURED || status_ == RUNNING ||
                                                              status_ == FLUSHED    || status_ == END_OF_STREAM)
                                         : status_ == CONFIGURED;
    CHECK_AND_RETURN_RET_LOG(isValidState, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    GSError gsRet = surface->SetSurfaceSourceType(OHSurfaceSource::OH_SURFACE_SOURCE_VIDEO);
    EXPECT_AND_LOGW(gsRet != GSERROR_OK, "Set surface source type failed, %{public}s", GSErrorStr(gsRet).c_str());
    int32_t ret = codecBase_->SetOutputSurface(surface);
    isSurfaceMode_ = true;
#ifdef EMULATOR_ENABLED
    Format config_emulator;
    config_emulator.PutIntValue(Tag::VIDEO_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::RGBA));
    codecBase_->SetParameter(config_emulator);
#endif
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
            drmDecryptor_->SetCodecName(codecName_);
            ret = drmDecryptor_->DrmVideoCencDecrypt(decryptVideoBufs_[index].inBuf,
                decryptVideoBufs_[index].outBuf, dataSize);
            decryptVideoBufs_[index].outBuf->memory_->SetSize(dataSize);
        }
    }
    return ret;
}

int32_t CodecServer::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    std::shared_lock<std::shared_mutex> freeLock(freeMutex_);
    if (isFree_) {
        AVCODEC_LOGE("In invalid state, free out");
        return AVCS_ERR_INVALID_STATE;
    }
    int32_t ret = AVCS_ERR_OK;
    if (((codecType_ == AVCODEC_TYPE_VIDEO_ENCODER) || (codecType_ == AVCODEC_TYPE_VIDEO_DECODER)) &&
        !((flag & AVCODEC_BUFFER_FLAG_CODEC_DATA) || (flag & AVCODEC_BUFFER_FLAG_EOS))) {
        AVCodecTrace::TraceBegin("CodecServer::Frame", info.presentationTimeUs);
    }
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

int32_t CodecServer::QueueInputBufferIn(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    int32_t ret = AVCS_ERR_OK;
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
        GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (temporalScalability_ != nullptr) {
        temporalScalability_->ConfigureLTR(index);
    }
    if (videoCb_ != nullptr) {
        ret = DrmVideoCencDecrypt(index);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_DECRYPT_FAILED, "CodecServer decrypt failed");
        ret = codecBase_->QueueInputBuffer(index);
    }
    if (codecCb_ != nullptr) {
        ret = codecBase_->QueueInputBuffer(index, info, flag);
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
    CHECK_AND_RETURN_RET_LOG(status_ != UNINITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->GetOutputFormat(format);
}

#ifdef SUPPORT_DRM
int32_t CodecServer::SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_LOGI("CodecServer::SetDecryptConfig");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (drmDecryptor_ == nullptr) {
        drmDecryptor_ = std::make_shared<CodecDrmDecrypt>();
    }
    CHECK_AND_RETURN_RET_LOG(drmDecryptor_ != nullptr, AVCS_ERR_NO_MEMORY, "drmDecryptor is nullptr");
    drmDecryptor_->SetDecryptionConfig(keySession, svpFlag);
    return AVCS_ERR_OK;
}
#endif

int32_t CodecServer::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> freeLock(freeMutex_);
    if (isFree_) {
        AVCODEC_LOGE("In invalid state, free out");
        return AVCS_ERR_INVALID_STATE;
    }
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret;
    if (render) {
        ret = codecBase_->RenderOutputBuffer(index);
    } else {
        ret = codecBase_->ReleaseOutputBuffer(index);
    }
    return ret;
}

int32_t CodecServer::SetParameter(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != INITIALIZED && status_ != CONFIGURED, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    if (codecType_ == AVCODEC_TYPE_VIDEO_ENCODER || codecType_ == AVCODEC_TYPE_VIDEO_DECODER) {
        Format oldFormat;
        int32_t ret = codecBase_->GetOutputFormat(oldFormat);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "Failed to get codec format");
        ret = CodecParamChecker::CheckParameterValid(format, oldFormat, codecType_, codecName_, scenario_);
        CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Params in format is not valid.");
    }

    return codecBase_->SetParameter(format);
}

int32_t CodecServer::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> cbLock(cbMutex_);
    codecCb_ = callback;
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

int32_t CodecServer::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(
        status_ == CONFIGURED || status_ == RUNNING || status_ == FLUSHED || status_ == END_OF_STREAM,
        AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s", GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->GetInputFormat(format);
}

int32_t CodecServer::DumpInfo(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(fd >= 0, AVCS_ERR_OK, "Get a invalid fd");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    Format codecFormat;
    int32_t ret = codecBase_->GetOutputFormat(codecFormat);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Get codec format failed.");

    AVCodecDumpControler dumpControler;
    auto statusIt = CODEC_STATE_MAP.find(status_);
    if (forwardCaller_.pid != -1 || !forwardCaller_.processName.empty()) {
        dumpControler.AddInfo(DUMP_INDEX_FORWARD_CALLER_PID, "Forward_Caller_Pid", std::to_string(caller_.pid));
        dumpControler.AddInfo(DUMP_INDEX_FORWARD_CALLER_UID, "Forward_Caller_Uid", std::to_string(caller_.uid));
        dumpControler.AddInfo(DUMP_INDEX_FORWARD_CALLER_PROCESS_NAME,
            "Forward_Caller_Process_Name", caller_.processName);
    }
    dumpControler.AddInfo(DUMP_INDEX_CALLER_PID, "Caller_Pid", std::to_string(caller_.pid));
    dumpControler.AddInfo(DUMP_INDEX_CALLER_UID, "Caller_Uid", std::to_string(caller_.uid));
    dumpControler.AddInfo(DUMP_INDEX_CALLER_PROCESS_NAME, "Caller_Process_Name", caller_.processName);
    dumpControler.AddInfo(DUMP_INDEX_STATUS, "Status", statusIt != CODEC_STATE_MAP.end() ? statusIt->second : "");
    dumpControler.AddInfo(DUMP_INDEX_LAST_ERROR, "Last_Error", lastErrMsg_.size() ? lastErrMsg_ : "Null");

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
    dumpString += "\n";
    write(fd, dumpString.c_str(), dumpString.size());
    return AVCS_ERR_OK;
}

void CodecServer::SetCallerInfo(const Meta &callerInfo)
{
    (void)callerInfo.GetData(Tag::AV_CODEC_CALLER_PID, caller_.pid);
    (void)callerInfo.GetData(Tag::AV_CODEC_CALLER_UID, caller_.uid);
    (void)callerInfo.GetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, caller_.processName);
    (void)callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_PID, forwardCaller_.pid);
    (void)callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_UID, forwardCaller_.uid);
    (void)callerInfo.GetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardCaller_.processName);

    EXPECT_AND_LOGI((forwardCaller_.pid >= 0) || (!forwardCaller_.processName.empty()),
        "Forward caller pid: %{public}d, process name: %{public}s",
        forwardCaller_.pid, forwardCaller_.processName.c_str());
    AVCODEC_LOGI("Caller pid: %{public}d, process name: %{public}s", caller_.pid, caller_.processName.c_str());
}

inline const std::string &CodecServer::GetStatusDescription(CodecStatus status)
{
    if (status < UNINITIALIZED || status >= ERROR) {
        return CODEC_STATE_MAP.at(ERROR);
    }
    return CODEC_STATE_MAP.at(status);
}

inline void CodecServer::StatusChanged(CodecStatus newStatus)
{
    if (status_ == newStatus) {
        return;
    }
    AVCODEC_LOGI("Status %{public}s -> %{public}s",
        GetStatusDescription(status_).data(), GetStatusDescription(newStatus).data());
    status_ = newStatus;
}

void CodecServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    lastErrMsg_ = AVCSErrorToString(static_cast<AVCodecServiceErrCode>(errorCode));
    FaultEventWrite(FaultType::FAULT_TYPE_INNER_ERROR, lastErrMsg_, "Codec");
    if (videoCb_ != nullptr) {
        videoCb_->OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
    }
    if (codecCb_ != nullptr) {
        codecCb_->OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
    }
}

void CodecServer::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (videoCb_ != nullptr) {
        videoCb_->OnOutputFormatChanged(format);
    }
    if (codecCb_ != nullptr) {
        codecCb_->OnOutputFormatChanged(format);
    }
}

void CodecServer::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr || (isCreateSurface_ && !isSetParameterCb_)) {
        return;
    }
    codecCb_->OnInputBufferAvailable(index, buffer);
}

void CodecServer::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                          std::shared_ptr<AVSharedMemory> buffer)
{
    if (((codecType_ == AVCODEC_TYPE_VIDEO_ENCODER) || (codecType_ == AVCODEC_TYPE_VIDEO_DECODER)) &&
        !((flag & AVCODEC_BUFFER_FLAG_CODEC_DATA) || (flag & AVCODEC_BUFFER_FLAG_EOS))) {
        AVCodecTrace::TraceEnd("CodecServer::Frame", info.presentationTimeUs);
    }

    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnOutputBufferAvailable(index, info, flag, buffer);
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
            AVCODEC_LOGE("CreateSharedAllocator failed");
            return;
        }
        decryptVideoBuf.inBuf = AVBuffer::CreateAVBuffer(avAllocator,
            static_cast<int32_t>(buffer->memory_->GetCapacity()));
        if (decryptVideoBuf.inBuf == nullptr || decryptVideoBuf.inBuf->memory_ == nullptr ||
            decryptVideoBuf.inBuf->memory_->GetCapacity() != static_cast<int32_t>(buffer->memory_->GetCapacity())) {
            AVCODEC_LOGE("CreateAVBuffer failed");
            return;
        }
        decryptVideoBuf.outBuf = buffer;
        decryptVideoBufs_.insert({index, decryptVideoBuf});
        videoCb_->OnInputBufferAvailable(index, decryptVideoBuf.inBuf);
    } else {
        videoCb_->OnInputBufferAvailable(index, buffer);
    }
}

void CodecServer::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    AVCODEC_LOGD("on output buffer index: %{public}d", index);
    CHECK_AND_RETURN_LOG(buffer != nullptr, "buffer is nullptr!");

    if (((codecType_ == AVCODEC_TYPE_VIDEO_ENCODER) || (codecType_ == AVCODEC_TYPE_VIDEO_DECODER)) &&
        !((buffer->flag_ & AVCODEC_BUFFER_FLAG_CODEC_DATA) || (buffer->flag_ & AVCODEC_BUFFER_FLAG_EOS))) {
        AVCodecTrace::TraceEnd("CodecServer::Frame", buffer->pts_);
    }

    if (temporalScalability_ != nullptr && !(buffer->flag_ == AVCODEC_BUFFER_FLAG_CODEC_DATA)) {
        temporalScalability_->SetDisposableFlag(buffer);
    }

    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    CHECK_AND_RETURN_LOG(videoCb_ != nullptr, "videoCb_ is nullptr!");
    videoCb_->OnOutputBufferAvailable(index, buffer);
}

CodecBaseCallback::CodecBaseCallback(const std::shared_ptr<CodecServer> &codec) : codec_(codec)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

CodecBaseCallback::~CodecBaseCallback()
{
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

VCodecBaseCallback::VCodecBaseCallback(const std::shared_ptr<CodecServer> &codec) : codec_(codec)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VCodecBaseCallback::~VCodecBaseCallback()
{
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
    CHECK_AND_RETURN_RET_LOG(meta != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    CHECK_AND_RETURN_RET_LOG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");

    int32_t ret = codecBase_->Configure(meta);

    CodecStatus newStatus = (ret == AVCS_ERR_OK ? CONFIGURED : ERROR);
    StatusChanged(newStatus);
    return ret;
}
int32_t CodecServer::SetParameter(const std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(parameter != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    CHECK_AND_RETURN_RET_LOG(status_ != INITIALIZED && status_ != CONFIGURED, AVCS_ERR_INVALID_STATE,
                             "In invalid state, %{public}s", GetStatusDescription(status_).data());
    return codecBase_->SetParameter(parameter);
}
int32_t CodecServer::GetOutputFormat(std::shared_ptr<Media::Meta> &parameter)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != UNINITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state, %{public}s",
                             GetStatusDescription(status_).data());
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "codecBase is nullptr");
    return codecBase_->GetOutputFormat(parameter);
}

int32_t CodecServer::SetOutputBufferQueue(const sptr<Media::AVBufferQueueProducer> &bufferQueueProducer)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(bufferQueueProducer != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->SetOutputBufferQueue(bufferQueueProducer);
}
int32_t CodecServer::Prepare()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    return codecBase_->Prepare();
}
sptr<Media::AVBufferQueueProducer> CodecServer::GetInputBufferQueue()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    return codecBase_->GetInputBufferQueue();
}
void CodecServer::ProcessInputBuffer()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    return codecBase_->ProcessInputBuffer();
}

#ifdef SUPPORT_DRM
int32_t CodecServer::SetAudioDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
    const bool svpFlag)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    AVCODEC_LOGI("CodecServer::SetAudioDecryptionConfig");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "codecBase is nullptr");
    return codecBase_->SetAudioDecryptionConfig(keySession, svpFlag);
}
#endif

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
} // namespace MediaAVCodec
} // namespace OHOS
