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

#include "media_codec_server.h"
#include "avcodec_codec_name.h"
#include "avcodec_dump_utils.h"
#include "avcodec_errors.h"
#include "avcodec_log.h"
#include "codec_factory.h"
#include "media_description.h"
#include "surface_type.h"
#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN, "MediaCodecServer"};
constexpr uint32_t DUMP_CODEC_INFO_INDEX = 0x01010000;
constexpr uint32_t DUMP_STATUS_INDEX = 0x01010100;
constexpr uint32_t DUMP_LAST_ERROR_INDEX = 0x01010200;
constexpr uint32_t DUMP_OFFSET_8 = 8;

const std::map<OHOS::MediaAVCodec::MediaCodecServer::CodecStatus, std::string> CODEC_STATE_MAP = {
    {OHOS::MediaAVCodec::MediaCodecServer::UNINITIALIZED, "uninitialized"},
    {OHOS::MediaAVCodec::MediaCodecServer::INITIALIZED, "initialized"},
    {OHOS::MediaAVCodec::MediaCodecServer::CONFIGURED, "configured"},
    {OHOS::MediaAVCodec::MediaCodecServer::RUNNING, "running"},
    {OHOS::MediaAVCodec::MediaCodecServer::FLUSHED, "flushed"},
    {OHOS::MediaAVCodec::MediaCodecServer::END_OF_STREAM, "end of stream"},
    {OHOS::MediaAVCodec::MediaCodecServer::ERROR, "error"},
};

const std::vector<std::pair<std::string_view, const std::string>> DEFAULT_DUMP_TABLE = {
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "Codec_Name"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "Bit_Rate"},
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

const std::vector<std::pair<std::string_view, const std::string>> AUDIO_DUMP_TABLE = {
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CODEC_NAME, "Codec_Name"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, "Channel_Count"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_BITRATE, "Bit_Rate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_SAMPLE_RATE, "Sample_Rate"},
    {OHOS::MediaAVCodec::MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE, "Max_Input_Size"},
};

const std::map<OHOS::MediaAVCodec::MediaCodecServer::CodecType,
               std::vector<std::pair<std::string_view, const std::string>>>
    CODEC_DUMP_TABLE = {
        {OHOS::MediaAVCodec::MediaCodecServer::CodecType::CODEC_TYPE_DEFAULT, DEFAULT_DUMP_TABLE},
        {OHOS::MediaAVCodec::MediaCodecServer::CodecType::CODEC_TYPE_VIDEO, VIDEO_DUMP_TABLE},
        {OHOS::MediaAVCodec::MediaCodecServer::CodecType::CODEC_TYPE_AUDIO, AUDIO_DUMP_TABLE},
};

const std::map<int32_t, const std::string> PIXEL_FORMAT_STRING_MAP = {
    {OHOS::MediaAVCodec::VideoPixelFormat::YUV420P, "YUV420P"},
    {OHOS::MediaAVCodec::VideoPixelFormat::YUVI420, "YUVI420"},
    {OHOS::MediaAVCodec::VideoPixelFormat::NV12, "NV12"},
    {OHOS::MediaAVCodec::VideoPixelFormat::NV21, "NV21"},
    {OHOS::MediaAVCodec::VideoPixelFormat::SURFACE_FORMAT, "SURFACE_FORMAT"},
    {OHOS::MediaAVCodec::VideoPixelFormat::RGBA, "RGBA"},
    {OHOS::MediaAVCodec::VideoPixelFormat::UNKNOWN_FORMAT, "UNKNOWN_FORMAT"},
};

const std::map<int32_t, const std::string> SCALE_TYPE_STRING_MAP = {
    {OHOS::ScalingMode::SCALING_MODE_FREEZE, "Freeze"},
    {OHOS::ScalingMode::SCALING_MODE_SCALE_TO_WINDOW, "Scale_to_window"},
    {OHOS::ScalingMode::SCALING_MODE_SCALE_CROP, "Scale_crop"},
    {OHOS::ScalingMode::SCALING_MODE_NO_SCALE_CROP, "No_scale_crop"},
};
} // namespace

namespace OHOS {
namespace MediaAVCodec {
std::shared_ptr<IMediaCodecService> MediaCodecServer::Create()
{
    std::shared_ptr<MediaCodecServer> server = std::make_shared<MediaCodecServer>();

    int32_t ret = server->InitServer();
    CHECK_AND_RETURN_RET_LOG(ret == AVCS_ERR_OK, nullptr, "Codec server init failed!");
    return server;
}

MediaCodecServer::MediaCodecServer()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecServer::~MediaCodecServer()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

int32_t MediaCodecServer::InitServer()
{
    return AVCS_ERR_OK;
}

int32_t MediaCodecServer::Init(bool isEncoder, bool isMimeType, const std::string &name)
{
    // call init
    AVCODEC_LOGD("Init successfully, name: %{public}s", name.c_str());
    return AVCS_ERR_OK;
}

int32_t MediaCodecServer::Configure(const Format &format)
{
    return 0;
}

int32_t MediaCodecServer::Start()
{
    return 0;
}

int32_t MediaCodecServer::Prepare()
{
    return 0;
}

int32_t MediaCodecServer::Stop()
{
    return 0;
}

int32_t MediaCodecServer::Flush()
{
    return 0;
}

int32_t MediaCodecServer::Reset()
{
    return 0;
}

int32_t MediaCodecServer::Release()
{
    return 0;
}

int32_t MediaCodecServer::GetOutputFormat(Format &format)
{
    return 0;
}

int32_t MediaCodecServer::SetParameter(const Format &format)
{
    return 0;
}

sptr<Media::AVBufferQueueProducer> MediaCodecServer::GetInputBufferQueue()
{
    return nullptr;
}

int32_t MediaCodecServer::SetOutputBufferQueue(sptr<Media::AVBufferQueueProducer> bufferQueue)
{
    return 0;
}

sptr<Surface> MediaCodecServer::CreateInputSurface()
{
    return nullptr;
}

int32_t MediaCodecServer::SetOutputSurface(sptr<Surface> surface)
{
    return 0;
}

int32_t MediaCodecServer::NotifyEos()
{
    return 0;
}

int32_t MediaCodecServer::SurfaceModeReturnData(std::shared_ptr<Meida::AVBuffer> buffer, bool available)
{
    return 0;
}

int32_t MediaCodecServer::SetCallback(const std::shared_ptr<VideoCodecCallback> &callback)
{
    std::lock_guard<std::shared_mutex> cbLock(cbMutex_);
    codecCb_ = callback;
    return 0;
}

int32_t MediaCodecServer::DumpInfo(int32_t fd)
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
            dumpControler.AddInfoFromFormat(DUMP_CODEC_INFO_INDEX + (dumpIndex << DUMP_OFFSET_8), codecFormat,
                                            iter.first, iter.second);
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

int32_t MediaCodecServer::SetClientInfo(int32_t clientPid, int32_t clientUid)
{
    clientPid_ = clientPid;
    clientUid_ = clientUid;
    return 0;
}

const std::string &MediaCodecServer::GetStatusDescription(OHOS::MediaAVCodec::MediaCodecServer::CodecStatus status)
{
    static const std::string ILLEGAL_STATE = "CODEC_STATUS_ILLEGAL";
    if (status < OHOS::MediaAVCodec::CodecServer::UNINITIALIZED || status > OHOS::MediaAVCodec::CodecServer::ERROR) {
        return ILLEGAL_STATE;
    }
    auto iter = CODEC_STATE_MAP.find(status);
    return iter == CODEC_STATE_MAP.end() ? "unknow" : iter->second;
}

void MediaCodecServer::OnError(int32_t errorType, int32_t errorCode)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    lastErrMsg_ = AVCSErrorToString(static_cast<AVCodecServiceErrCode>(errorCode));
    FaultEventWrite(FaultType::FAULT_TYPE_INNER_ERROR, lastErrMsg_, "Codec");
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnError(static_cast<AVCodecErrorType>(errorType), errorCode);
}

void MediaCodecServer::OnStreamChanged(const Format &format)
{
    std::lock_guard<std::shared_mutex> lock(cbMutex_);
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->OnStreamChanged(format);
}

void MediaCodecServer::onSurfaceModeData(std::shared_ptr<Media::AVBuffer> buffer)
{
    if (codecCb_ == nullptr) {
        return;
    }
    codecCb_->onSurfaceModeData(buffer);
}

MediaCodecBaseCallback::MediaCodecBaseCallback(const std::shared_ptr<MediaCodecServer> &codec) : codec_(codec)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

MediaCodecBaseCallback::~MediaCodecBaseCallback()
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

void MediaCodecBaseCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    if (codec_ != nullptr) {
        codec_->OnError(errorType, errorCode);
    }
}

void MediaCodecBaseCallback::OnStreamChanged(const Format &format)
{
    if (codec_ != nullptr) {
        codec_->OnStreamChanged(format);
    }
}

void MediaCodecBaseCallback::onSurfaceModeData(std::shared_ptr<Media::AVBuffer> buffer)
{
    if (codec_ != nullptr) {
        codec_->onSurfaceModeData(buffer);
    }
}

MediaCodecServer::CodecType MediaCodecServer::GetCodecType()
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

int32_t MediaCodecServer::GetCodecDfxInfo(CodecDfxInfo &codecDfxInfo)
{
    if (clientPid_ == 0) {
        clientPid_ = getpid();
        clientUid_ = getuid();
    }
    Format format;
    codecBase_->GetOutputFormat(format);
    int32_t videoPixelFormat = VideoPixelFormat::UNKNOWN_FORMAT;
    format.GetIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, videoPixelFormat);
    videoPixelFormat = PIXEL_FORMAT_STRING_MAP.find(videoPixelFormat) != PIXEL_FORMAT_STRING_MAP.end()
                           ? videoPixelFormat
                           : VideoPixelFormat::UNKNOWN_FORMAT;
    int32_t codecIsVendor = 0;
    codecIsVendor = format.GetIntValue("IS_VENDOR", codecIsVendor);

    codecDfxInfo.clientPid = clientPid_;
    codecDfxInfo.clientUid = clientUid_;
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
} // namespace MediaAVCodec
} // namespace OHOS