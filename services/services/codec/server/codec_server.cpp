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
#include <map>
#include <vector>
#include <malloc.h>
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "codec_factory.h"
#include "avcodec_dump_utils.h"
#include "media_description.h"
#include "surface_type.h"
#include "avcodec_codec_name.h"

namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "CodecServer"};
    constexpr uint32_t DUMP_CODEC_INFO_INDEX = 0x01010000;
    constexpr uint32_t DUMP_STATUS_INDEX = 0x01010100;
    constexpr uint32_t DUMP_LAST_ERROR_INDEX = 0x01010200;
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

    const std::vector<std::pair<std::string_view, const std::string>> DEFAULT_DUMP_TABLE = {
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "Codec_Name" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "Bit_Rate" },
    };

    const std::vector<std::pair<std::string_view, const std::string>> VIDEO_DUMP_TABLE = {
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "Codec_Name" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_WIDTH, "Width" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_HEIGHT, "Height" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_FRAME_RATE, "Frame_Rate" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "Bit_Rate" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, "Pixel_Format" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_SCALE_TYPE, "Scale_Type" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_ROTATION_ANGLE, "Rotation_Angle" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, "Max_Input_Size" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_BUFFER_COUNT, "Max_Input_Buffer_Count" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_OUTPUT_BUFFER_COUNT, "Max_Output_Buffer_Count" },
    };

    const std::vector<std::pair<std::string_view, const std::string>> AUDIO_DUMP_TABLE = {
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "Codec_Name" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, "Channel_Count" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "Bit_Rate" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_SAMPLE_RATE, "Sample_Rate" },
        { OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, "Max_Input_Size" },
    };

    const std::map<OHOS::MediaAVCodec::CodecServer::CodecType,
        std::vector<std::pair<std::string_view, const std::string>>> CODEC_DUMP_TABLE = {
        { OHOS::MediaAVCodec::CodecServer::CodecType::CODEC_TYPE_DEFAULT, DEFAULT_DUMP_TABLE },
        { OHOS::MediaAVCodec::CodecServer::CodecType::CODEC_TYPE_VIDEO, VIDEO_DUMP_TABLE },
        { OHOS::MediaAVCodec::CodecServer::CodecType::CODEC_TYPE_AUDIO, AUDIO_DUMP_TABLE },
    };

    const std::map<int32_t, const std::string> PIXEL_FORMAT_STRING_MAP = {
        { OHOS::MediaAVCodec::VideoPixelFormat::YUV420P, "YUV420P" },
        { OHOS::MediaAVCodec::VideoPixelFormat::YUVI420, "YUVI420" },
        { OHOS::MediaAVCodec::VideoPixelFormat::NV12, "NV12" },
        { OHOS::MediaAVCodec::VideoPixelFormat::NV21, "NV21" },
        { OHOS::MediaAVCodec::VideoPixelFormat::SURFACE_FORMAT, "SURFACE_FORMAT" },
        { OHOS::MediaAVCodec::VideoPixelFormat::RGBA, "RGBA" },
        { OHOS::MediaAVCodec::VideoPixelFormat::UNKNOWN_FORMAT, "UNKNOWN_FORMAT" },
    };

    const std::map<int32_t, const std::string> SCALE_TYPE_STRING_MAP = {
        { OHOS::ScalingMode::SCALING_MODE_FREEZE, "Freeze" },
        { OHOS::ScalingMode::SCALING_MODE_SCALE_TO_WINDOW, "Scale_to_window" },
        { OHOS::ScalingMode::SCALING_MODE_SCALE_CROP, "Scale_crop" },
        { OHOS::ScalingMode::SCALING_MODE_NO_SCALE_CROP, "No_scale_crop" },
    };
}

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<ICodecService> CodecServer::Create()
{
    std::shared_ptr<CodecServer> server = std::make_shared<CodecServer>();

    int32_t ret = server->InitServer();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Codec server init failed!");
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

int32_t CodecServer::Init(AVCodecType type, bool isMimeType, const std::string &name)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    (void)mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    (void)mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
    std::string codecMimeName = name;
    if (isMimeType) {
        bool isEncoder = (type == AVCODEC_TYPE_VIDEO_ENCODER) || (type == AVCODEC_TYPE_AUDIO_ENCODER);
        codecBase_ = CodecFactory::Instance().CreateCodecByMime(isEncoder, codecMimeName);
    } else {
        if (name.compare(AVCodecCodecName::AUDIO_DECODER_API9_AAC_NAME) == 0) {
            codecMimeName = AVCodecCodecName::AUDIO_DECODER_AAC_NAME;
        } else if (name.compare(AVCodecCodecName::AUDIO_ENCODER_API9_AAC_NAME) == 0) {
            codecMimeName = AVCodecCodecName::AUDIO_ENCODER_AAC_NAME;
        }
        if (codecMimeName.find("Audio") != codecMimeName.npos) {
            if ((codecMimeName.find("Decoder") != codecMimeName.npos && type != AVCODEC_TYPE_AUDIO_DECODER) ||
                (codecMimeName.find("Encoder") != codecMimeName.npos && type != AVCODEC_TYPE_AUDIO_ENCODER)) {
                AVCODEC_LOGE("Codec name invalid");
                return AVCS_ERR_INVALID_OPERATION;
            }
        }
        codecBase_ = CodecFactory::Instance().CreateCodecByName(codecMimeName);
    }
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "CodecBase is nullptr");
    codecName_ = codecMimeName;
    std::shared_ptr<AVCodecCallback> callback = std::make_shared<CodecBaseCallback>(shared_from_this());
    int32_t ret = codecBase_->SetCallback(callback);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, AVCS_ERR_INVALID_OPERATION, "CodecBase SetCallback failed");
    status_ = INITIALIZED;
    AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
    return AVCS_ERR_OK;
}

int32_t CodecServer::Configure(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == INITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    config_ = format;
    int32_t ret = codecBase_->Configure(format);

    status_ = (ret == AVCS_ERR_OK ? CONFIGURED : ERROR);
    AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
    return ret;
}

int32_t CodecServer::Start()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == FLUSHED || status_ == CONFIGURED,
        AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Start();

    status_ = (ret == AVCS_ERR_OK ? RUNNING : ERROR);
    AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
    if (isFirstStart_ && ret == AVCS_ERR_OK) {
        isFirstStart_ = false;
        CodecDfxInfo codecDfxInfo;
        GetCodecDfxInfo(codecDfxInfo);
        CodecCreateEventWrite(codecDfxInfo);
    }
    return ret;
}

int32_t CodecServer::Stop()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM ||
        status_ == FLUSHED, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Stop();
    status_ = (ret == AVCS_ERR_OK ? CONFIGURED : ERROR);
    AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
    return ret;
}

int32_t CodecServer::Flush()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM,
        AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Flush();
    status_ = (ret == AVCS_ERR_OK ? FLUSHED : ERROR);
    AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
    return ret;
}

int32_t CodecServer::NotifyEos()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->NotifyEos();
    if (ret == AVCS_ERR_OK) {
        status_ = END_OF_STREAM;
        AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
    }
    return ret;
}

int32_t CodecServer::Reset()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Reset();
    status_ = (ret == AVCS_ERR_OK ? INITIALIZED : ERROR);
    AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
    lastErrMsg_.clear();
    return ret;
}

int32_t CodecServer::Release()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    int32_t ret = codecBase_->Release();
    std::unique_ptr<std::thread> thread = std::make_unique<std::thread>(&CodecServer::ExitProcessor, this);
    if (thread->joinable()) {
        thread->join();
    }
    if (ret == AVCS_ERR_OK) {
        CodecDfxInfo codecDfxInfo;
        GetCodecDfxInfo(codecDfxInfo);
        CodecDestroyEventWrite(codecDfxInfo.clientPid, codecDfxInfo.clientUid, codecDfxInfo.codecInstanceId);
    }
    return ret;
}

sptr<Surface> CodecServer::CreateInputSurface()
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == CONFIGURED, nullptr, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, nullptr, "Codecbase is nullptr");
    sptr<Surface> surface = codecBase_->CreateInputSurface();
    if (surface != nullptr) {
        isSurfaceMode_ = true;
    }
    return surface;
}

int32_t CodecServer::SetOutputSurface(sptr<Surface> surface)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == CONFIGURED, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    if (surface != nullptr) {
        isSurfaceMode_ = true;
    }
    return codecBase_->SetOutputSurface(surface);
}

int32_t CodecServer::QueueInputBuffer(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    int32_t ret = AVCS_ERR_OK;
    if (flag != AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        if (isFirstFrameIn_) {
            AVCodecTrace::TraceBegin("CodecServer::FirstFrame", info.presentationTimeUs);
            isFirstFrameIn_ = false;
        } else {
            AVCodecTrace::TraceBegin("CodecServer::Frame", info.presentationTimeUs);
        }
    }
    {
        std::shared_lock<std::shared_mutex> lock(mutex_);
        CHECK_AND_RETURN_RET_LOG(status_ == RUNNING, AVCS_ERR_INVALID_STATE, "In invalid state");
        CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
        ret = codecBase_->QueueInputBuffer(index, info, flag);
    }
    if (flag & AVCODEC_BUFFER_FLAG_EOS) {
        if (ret == AVCS_ERR_OK) {
            std::unique_lock<std::shared_mutex> lock(mutex_);
            status_ = END_OF_STREAM;
            AVCODEC_LOGI("Codec server in %{public}s status", GetStatusDescription(status_).data());
        }
    }
    return ret;
}

int32_t CodecServer::GetOutputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != UNINITIALIZED, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->GetOutputFormat(format);
}

int32_t CodecServer::ReleaseOutputBuffer(uint32_t index, bool render)
{
    std::shared_lock<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ == RUNNING || status_ == END_OF_STREAM,
        AVCS_ERR_INVALID_STATE, "In invalid state");
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
    CHECK_AND_RETURN_RET_LOG(status_ != INITIALIZED && status_ != CONFIGURED,
        AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->SetParameter(format);
}

int32_t CodecServer::SetCallback(const std::shared_ptr<AVCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> cbLock(cbMutex_);
    codecCb_ = callback;
    return AVCS_ERR_OK;
}

int32_t CodecServer::GetInputFormat(Format &format)
{
    std::lock_guard<std::shared_mutex> lock(mutex_);
    CHECK_AND_RETURN_RET_LOG(status_ != CONFIGURED, AVCS_ERR_INVALID_STATE, "In invalid state");
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    return codecBase_->GetInputFormat(format);
}

int32_t CodecServer::DumpInfo(int32_t fd)
{
    CHECK_AND_RETURN_RET_LOG(codecBase_ != nullptr, AVCS_ERR_NO_MEMORY, "Codecbase is nullptr");
    Format codecFormat;
    int32_t ret = codecBase_->GetOutputFormat(codecFormat);
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, ret, "Get codec format failed.");
    CodecType codecType = GetCodecType();
    auto it = CODEC_DUMP_TABLE.find(codecType);
    const auto &dumpTable = it != CODEC_DUMP_TABLE.end() ? it->second : DEFAULT_DUMP_TABLE;
    AVCodecDumpControler dumpControler;
    std::string codecInfo;
    switch (codecType) {
        case CODEC_TYPE_VIDEO:
            codecInfo = "Video_Codec_Info";
            break;
        case CODEC_TYPE_DEFAULT:
            codecInfo = "Codec_Info";
            break;
        case CODEC_TYPE_AUDIO:
            codecInfo = "Audio_Codec_Info";
            break;
    }
    auto statusIt = CODEC_STATE_MAP.find(status_);
    dumpControler.AddInfo(DUMP_CODEC_INFO_INDEX, codecInfo);
    dumpControler.AddInfo(DUMP_STATUS_INDEX, "Status", statusIt != CODEC_STATE_MAP.end() ? statusIt->second : "");
    dumpControler.AddInfo(DUMP_LAST_ERROR_INDEX, "Last_Error", lastErrMsg_.size() ? lastErrMsg_ : "Null");

    int32_t dumpIndex = 3;
    for (auto iter : dumpTable) {
        if (iter.first == MediaDescriptionKey::MD_KEY_PIXEL_FORMAT) {
            dumpControler.AddInfoFromFormatWithMapping(DUMP_CODEC_INFO_INDEX + (dumpIndex << DUMP_OFFSET_8),
                codecFormat, iter.first, iter.second, PIXEL_FORMAT_STRING_MAP);
        } else if (iter.first == MediaDescriptionKey::MD_KEY_SCALE_TYPE) {
            dumpControler.AddInfoFromFormatWithMapping(DUMP_CODEC_INFO_INDEX + (dumpIndex << DUMP_OFFSET_8),
                codecFormat, iter.first, iter.second, SCALE_TYPE_STRING_MAP);
        } else {
            dumpControler.AddInfoFromFormat(
                DUMP_CODEC_INFO_INDEX + (dumpIndex << DUMP_OFFSET_8), codecFormat, iter.first, iter.second);
        }
        dumpIndex++;
    }
    std::string dumpString;
    dumpControler.GetDumpString(dumpString);
    if (fd != -1) {
        write(fd, dumpString.c_str(), dumpString.size());
    }
    return AVCS_ERR_OK;
}

const std::string &CodecServer::GetStatusDescription(OHOS::MediaAVCodec::CodecServer::CodecStatus status)
{
    static const std::string ILLEGAL_STATE = "CODEC_STATUS_ILLEGAL";
    if (status < OHOS::MediaAVCodec::CodecServer::UNINITIALIZED ||
        status > OHOS::MediaAVCodec::CodecServer::ERROR) {
        return ILLEGAL_STATE;
    }

    return CODEC_STATE_MAP.find(status)->second;
}

void CodecServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    lastErrMsg_ = AVCSErrorToString(static_cast<AVCodecServiceErrCode>(errorCode));
    FaultEventWrite(FaultType::FAULT_TYPE_INNER_ERROR, lastErrMsg_, "Codec");
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
}

void CodecServer::OnOutputFormatChanged(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnOutputFormatChanged(format);
}

void CodecServer::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVSharedMemory> buffer)
{
    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnInputBufferAvailable(index, buffer);
}

void CodecServer::OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag,
                                          std::shared_ptr<AVSharedMemory> buffer)
{
    if (flag != AVCODEC_BUFFER_FLAG_CODEC_DATA) {
        if (isFirstFrameOut_) {
            AVCodecTrace::TraceEnd("CodecServer::FirstFrame", info.presentationTimeUs);
            isFirstFrameOut_ = false;
        } else {
            AVCodecTrace::TraceEnd("CodecServer::Frame", info.presentationTimeUs);
        }
    }

    if (flag == AVCODEC_BUFFER_FLAG_EOS) {
        isFirstFrameIn_ = true;
        isFirstFrameOut_ = true;
    }
    std::shared_lock<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnOutputBufferAvailable(index, info, flag, buffer);
}

CodecBaseCallback::CodecBaseCallback(const std::shared_ptr<CodecServer> &codec)
    : codec_(codec)
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

CodecServer::CodecType CodecServer::GetCodecType()
{
    CodecType codecType;

    if ((codecName_.find("Video") != codecName_.npos) || (codecName_.find("video") != codecName_.npos)) {
        codecType = CodecType::CODEC_TYPE_VIDEO;
    } else if ((codecName_.find("Audio") != codecName_.npos) || (codecName_.find("audio") != codecName_.npos)) {
        codecType = CodecType::CODEC_TYPE_AUDIO;
    } else {
        codecType = CodecType::CODEC_TYPE_DEFAULT;
    }

    return codecType;
}

int32_t CodecServer::GetCodecDfxInfo(CodecDfxInfo &codecDfxInfo)
{
    Format format;
    codecBase_->GetOutputFormat(format);

    codecDfxInfo.clientPid = clientPid_;
    codecDfxInfo.clientUid = clientUid_;
    codecDfxInfo.codecInstanceId = FAKE_POINTER(this);
    format.GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_NAME, codecDfxInfo.codecName);
    // codecDfxInfo.codecIsVendor = ;
    codecDfxInfo.codecMode = isSurfaceMode_ ? "Surface mode" : "Buffer Mode";
    format.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, codecDfxInfo.encoderBitRate);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, codecDfxInfo.videoWidth);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, codecDfxInfo.videoHeight);
    format.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, codecDfxInfo.videoFrameRate);
    format.GetStringValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, codecDfxInfo.videoPixelFormat);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, codecDfxInfo.audioChannelCount);
    format.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, codecDfxInfo.audioSampleRate);
    return 0;
}
} // namespace MediaAVCodec
} // namespace OHOS