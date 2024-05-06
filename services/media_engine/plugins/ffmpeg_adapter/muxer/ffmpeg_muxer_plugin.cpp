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

#define HST_LOG_TAG "FfmpegMuxerPlugin"

#include "ffmpeg_muxer_plugin.h"

#include <malloc.h>
#include <set>

#include "securec.h"
#include "common/log.h"
#include "ffmpeg_utils.h"
#include "meta/mime_type.h"

namespace {
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

std::map<std::string, std::shared_ptr<AVOutputFormat>> g_pluginOutputFmt;

std::set<std::string> g_supportedMuxer = {"mp4", "ipod", "amr", "mp3"};
constexpr uint8_t START_CODE[] = {0x00, 0x00, 0x01};
constexpr int64_t TIMESTAMP_US = 1000;
constexpr float LATITUDE_MIN = -90.0f;
constexpr float LATITUDE_MAX = 90.0f;
constexpr float LONGITUDE_MIN = -180.0f;
constexpr float LONGITUDE_MAX = 180.0f;

bool IsMuxerSupported(const char *name)
{
    return g_supportedMuxer.count(name) == 1;
}

bool CodecId2Cap(AVCodecID codecId, bool encoder, Capability& cap)
{
    switch (codecId) {
        case AV_CODEC_ID_MP3:
            cap.SetMime(MimeType::AUDIO_MPEG).AppendFixedKey<uint32_t>(Tag::AUDIO_MPEG_VERSION, 1);
            return true;
        case AV_CODEC_ID_AAC:
            cap.SetMime(MimeType::AUDIO_AAC);
            return true;
        case AV_CODEC_ID_MPEG4:
            cap.SetMime(MimeType::VIDEO_MPEG4);
            return true;
        case AV_CODEC_ID_H264:
            cap.SetMime(MimeType::VIDEO_AVC);
            return true;
        case AV_CODEC_ID_HEVC:
            cap.SetMime(MimeType::VIDEO_HEVC);
            return true;
        case AV_CODEC_ID_AMR_NB:
            cap.SetMime(MimeType::AUDIO_AMR_NB);
            return true;
        case AV_CODEC_ID_AMR_WB:
            cap.SetMime(MimeType::AUDIO_AMR_WB);
            return true;
        default:
            break;
    }
    return false;
}

bool FormatName2OutCapability(const std::string& fmtName, MuxerPluginDef& pluginDef)
{
    if (fmtName == "mp4") {
        auto cap = Capability(MimeType::MEDIA_MP4);
        pluginDef.AddOutCaps(cap);
        return true;
    } else if (fmtName == "ipod") {
        auto cap = Capability(MimeType::MEDIA_M4A);
        pluginDef.AddOutCaps(cap);
        return true;
    } else if (fmtName == "amr") {
        auto cap = Capability(MimeType::MEDIA_AMR);
        pluginDef.AddOutCaps(cap);
        return true;
    } else if (fmtName == "mp3") {
        auto cap = Capability(MimeType::MEDIA_MP3);
        pluginDef.AddOutCaps(cap);
        return true;
    }
    return false;
}

bool UpdatePluginInCapability(AVCodecID codecId, MuxerPluginDef& pluginDef)
{
    if (codecId != AV_CODEC_ID_NONE) {
        Capability cap;
        if (!CodecId2Cap(codecId, true, cap)) {
            return false;
        } else {
            pluginDef.AddInCaps(cap);
        }
    }
    return true;
}

bool UpdatePluginCapability(const AVOutputFormat* oFmt, MuxerPluginDef& pluginDef)
{
    if (!FormatName2OutCapability(oFmt->name, pluginDef)) {
        MEDIA_LOG_D(PUBLIC_LOG_S " is not supported now", oFmt->name);
        return false;
    }
    UpdatePluginInCapability(oFmt->audio_codec, pluginDef);
    UpdatePluginInCapability(oFmt->video_codec, pluginDef);
    UpdatePluginInCapability(oFmt->subtitle_codec, pluginDef);
    return true;
}

Status RegisterMuxerPlugins(const std::shared_ptr<Register>& reg)
{
    const AVOutputFormat *outputFormat = nullptr;
    void *ite = nullptr;
    while ((outputFormat = av_muxer_iterate(&ite))) {
        if (!IsMuxerSupported(outputFormat->name)) {
            continue;
        }
        if (outputFormat->long_name != nullptr) {
            if (!strncmp(outputFormat->long_name, "raw ", 4)) { // 4
                continue;
            }
        }
        std::string pluginName = "ffmpegMux_" + std::string(outputFormat->name);
        ReplaceDelimiter(".,|-<> ", '_', pluginName);
        MuxerPluginDef def;
        if (!UpdatePluginCapability(outputFormat, def)) {
            continue;
        }
        def.name = pluginName;
        def.description = "ffmpeg muxer";
        def.rank = 60; // 60
        def.SetCreator([](const std::string& name) -> std::shared_ptr<MuxerPlugin> {
            return std::make_shared<FFmpegMuxerPlugin>(name);
        });
        if (reg->AddPlugin(def) != Status::OK) {
            MEDIA_LOG_W("fail to add plugin " PUBLIC_LOG_S, pluginName.c_str());
            continue;
        }
        g_pluginOutputFmt[pluginName] = std::shared_ptr<AVOutputFormat>(
            const_cast<AVOutputFormat*>(outputFormat), [](AVOutputFormat *ptr) {}); // do not delete
    }
    return Status::OK;
}

PLUGIN_DEFINITION(FFmpegMuxer, LicenseType::LGPL, RegisterMuxerPlugins, [] {g_pluginOutputFmt.clear();})

void ResetCodecParameter(AVCodecParameters *par)
{
    av_freep(&par->extradata);
    (void)memset_s(par, sizeof(*par), 0, sizeof(*par));
    par->codec_type = AVMEDIA_TYPE_UNKNOWN;
    par->codec_id = AV_CODEC_ID_NONE;
    par->format = -1;
    par->profile = FF_PROFILE_UNKNOWN;
    par->level = FF_LEVEL_UNKNOWN;
    par->field_order = AV_FIELD_UNKNOWN;
    par->color_range = AVCOL_RANGE_UNSPECIFIED;
    par->color_primaries = AVCOL_PRI_UNSPECIFIED;
    par->color_trc = AVCOL_TRC_UNSPECIFIED;
    par->color_space = AVCOL_SPC_UNSPECIFIED;
    par->chroma_location = AVCHROMA_LOC_UNSPECIFIED;
    par->sample_aspect_ratio = AVRational {0, 1};
}
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegMuxerPlugin::FFmpegMuxerPlugin(std::string name)
    : MuxerPlugin(std::move(name)), isWriteHeader_(false)
{
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
#ifndef _WIN32
    mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
#endif
    auto pkt = av_packet_alloc();
    cachePacket_ = std::shared_ptr<AVPacket> (pkt, [] (AVPacket *packet) {av_packet_free(&packet);});
    outputFormat_ = g_pluginOutputFmt[pluginName_];
    auto fmt = avformat_alloc_context();
    fmt->pb = nullptr;
    fmt->oformat = outputFormat_.get();
    fmt->flags = static_cast<uint32_t>(fmt->flags) | static_cast<uint32_t>(AVFMT_FLAG_CUSTOM_IO);
    fmt->io_open = IoOpen;
    fmt->io_close = IoClose;
    formatContext_ = std::shared_ptr<AVFormatContext>(fmt, [](AVFormatContext *ptr) {
        if (ptr) {
            DeInitAvIoCtx(ptr->pb);
            avformat_free_context(ptr);
        }
    });
    av_log_set_level(AV_LOG_ERROR);
}

FFmpegMuxerPlugin::~FFmpegMuxerPlugin()
{
    MEDIA_LOG_D("Destroy");
    outputFormat_.reset();
    cachePacket_.reset();
    formatContext_.reset();
    videoTracksInfo_.clear();
    hevcParser_.reset();
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
#ifndef _WIN32
    mallopt(M_FLUSH_THREAD_CACHE, 0);
#endif
}

Status FFmpegMuxerPlugin::SetDataSink(const std::shared_ptr<DataSink> &dataSink)
{
    FALSE_RETURN_V_MSG_E(dataSink != nullptr, Status::ERROR_INVALID_PARAMETER, "data sink is null");
    DeInitAvIoCtx(formatContext_->pb);
    formatContext_->pb = InitAvIoCtx(dataSink, 1);
    FALSE_RETURN_V_MSG_E(formatContext_->pb != nullptr, Status::ERROR_INVALID_OPERATION, "failed to create io");
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetParameter(const std::shared_ptr<Meta> &param)
{
    Status ret = Status::NO_ERROR;
    int32_t dataInt = 0;
    if (param->GetData("fast_start", dataInt) && dataInt == 1) {
        isFastStart_ = true;
        MEDIA_LOG_I("fast start for moov");
    }
    ret = SetRotation(param);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "SetParameter failed");
    ret = SetLocation(param);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "SetParameter failed");
    ret = SetMetaData(param);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "SetParameter failed");
    return ret;
}

Status FFmpegMuxerPlugin::SetRotation(std::shared_ptr<Meta> param)
{
    if (param->Find(Tag::VIDEO_ROTATION) != param->end()) {
        param->Get<Tag::VIDEO_ROTATION>(rotation_); // rotation
        if (rotation_ != VIDEO_ROTATION_0 && rotation_ != VIDEO_ROTATION_90 &&
            rotation_ != VIDEO_ROTATION_180 && rotation_ != VIDEO_ROTATION_270) {
            MEDIA_LOG_W("Invalid rotation: %{public}d, keep default 0", rotation_);
            rotation_ = VIDEO_ROTATION_0;
            return Status::ERROR_INVALID_DATA;
        }
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetLocation(std::shared_ptr<Meta> param)
{
    float latitude = 0.0f;
    float longitude = 0.0f;
    if (param->Find(Tag::MEDIA_LATITUDE) == param->end() &&
        param->Find(Tag::MEDIA_LONGITUDE) == param->end()) {
        return Status::NO_ERROR;
    } else if (param->Find(Tag::MEDIA_LATITUDE) != param->end() &&
        param->Find(Tag::MEDIA_LONGITUDE) != param->end()) {
        param->Get<Tag::MEDIA_LATITUDE>(latitude); // latitude
        param->Get<Tag::MEDIA_LONGITUDE>(longitude); // longitude
    } else {
        MEDIA_LOG_W("the latitude and longitude are all required");
        return Status::ERROR_INVALID_DATA;
    }
    FALSE_RETURN_V_MSG_E(latitude >= LATITUDE_MIN && latitude <= LATITUDE_MAX,
        Status::ERROR_INVALID_DATA, "latitude must be in [-90, 90]!");
    FALSE_RETURN_V_MSG_E(longitude >= LONGITUDE_MIN && longitude <= LONGITUDE_MAX,
        Status::ERROR_INVALID_DATA, "longitude must be in [-180, 180]!");
    std::string location = std::to_string(longitude) + " ";
    location += std::to_string(latitude) + " ";
    location += std::to_string(0.0f);
    av_dict_set(&formatContext_.get()->metadata, "location", location.c_str(), 0);
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetMetaData(std::shared_ptr<Meta> param)
{
    static std::map<TagType, std::string> table = {
        {Tag::MEDIA_TITLE, "title"},
        {Tag::MEDIA_ARTIST, "artist"},
        {Tag::MEDIA_ALBUM, "album"},
        {Tag::MEDIA_ALBUM_ARTIST, "album_artist"},
        {Tag::MEDIA_DATE, "date"},
        {Tag::MEDIA_COMMENT, "comment"},
        {Tag::MEDIA_GENRE, "genre"},
        {Tag::MEDIA_COPYRIGHT, "copyright"},
        {Tag::MEDIA_LANGUAGE, "language"},
        {Tag::MEDIA_DESCRIPTION, "description"},
        {Tag::MEDIA_LYRICS, "lyrics"},
        {Tag::MEDIA_AUTHOR, "author"},
        {Tag::MEDIA_COMPOSER, "composer"},
        {Tag::MEDIA_CREATION_TIME, "creation_time"},
    };
    av_dict_set(&formatContext_->metadata, "creation_time", "now", 0);
    for (auto& key: table) {
        std::string value;
        if (param->GetData(key.first, value)) {
            av_dict_set(&formatContext_->metadata, key.second.c_str(), value.c_str(), 0);
        }
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    std::vector<std::string> keys;
    userMeta->GetKeys(keys);
    FALSE_RETURN_V_MSG_E(keys.size() > 0, Status::ERROR_INVALID_DATA, "user meta is empty!");
    av_dict_set(&formatContext_->metadata, "moov_level_meta_flag", "1", 0);
    for (auto& k: keys) {
        std::string key = "moov_level_meta_key_" + k;
        std::string value = "";
        int32_t dataInt = 0;
        float dataFloat = 0.0f;
        std::string dataStr = "";
        if (userMeta->GetData(k, dataInt)) {
            value = "00000043";
            value += std::to_string(dataInt);
        } else if (userMeta->GetData(k, dataFloat)) {
            value = "00000017";
            value += std::to_string(dataFloat);
        } else if (userMeta->GetData(k, dataStr)) {
            value = "00000001";
            value += dataStr;
        } else {
            MEDIA_LOG_E("the value type of meta key %{public}s is not supported!", k.c_str());
            continue;
        }
        av_dict_set(&formatContext_->metadata, key.c_str(), value.c_str(), 0);
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterOfTrack(AVStream *stream, const std::shared_ptr<Meta> &trackDesc)
{
    AVCodecParameters *par = stream->codecpar;
    if (trackDesc->Find(Tag::MEDIA_BITRATE) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_BITRATE>(par->bit_rate); // bit rate
    }

    VideoSampleInfo sampleInfo;
    videoTracksInfo_[stream->index] = sampleInfo;
    std::vector<uint8_t> codecConfig;
    if (trackDesc->Find(Tag::MEDIA_CODEC_CONFIG) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig); // codec config
        if (par->codec_id == AV_CODEC_ID_H264 || par->codec_id == AV_CODEC_ID_HEVC) {
            videoTracksInfo_[stream->index].extraData_.reserve(codecConfig.size());
            videoTracksInfo_[stream->index].extraData_.assign(codecConfig.begin(), codecConfig.end());
            return SetNalSizeLen(stream, codecConfig);
        }
        return SetCodecParameterExtra(stream, codecConfig.data(), codecConfig.size());
    } else if (par->codec_id == AV_CODEC_ID_AAC) {
        if (trackDesc->Find(Tag::MEDIA_PROFILE) != trackDesc->end()) {
            int32_t profile;
            int32_t sampleRate;
            int32_t channels;
            trackDesc->Get<Tag::MEDIA_PROFILE>(profile);
            trackDesc->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
            trackDesc->Get<Tag::AUDIO_CHANNEL_COUNT>(channels);
            codecConfig = GenerateAACCodecConfig(profile, sampleRate, channels);
            return SetCodecParameterExtra(stream, codecConfig.data(), codecConfig.size());
        }
        MEDIA_LOG_W("missing codec config of aac!");
    } else if (par->codec_id == AV_CODEC_ID_MPEG4) {
        MEDIA_LOG_W("missing codec config of mpeg4!");
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterExtra(AVStream *stream, const uint8_t *extraData, int32_t extraDataSize)
{
    FALSE_RETURN_V_MSG_E(extraDataSize != 0, Status::ERROR_INVALID_DATA, "codec config size is zero!");

    AVCodecParameters *par = stream->codecpar;
    par->extradata = static_cast<uint8_t *>(av_mallocz(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
    FALSE_RETURN_V_MSG_E(par->extradata != nullptr, Status::ERROR_NO_MEMORY, "codec config malloc failed!");
    par->extradata_size = extraDataSize;
    errno_t rc = memcpy_s(par->extradata, par->extradata_size, extraData, extraDataSize);
    FALSE_RETURN_V_MSG_E(rc == EOK, Status::ERROR_UNKNOWN, "memcpy_s failed");

    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterColor(AVStream* stream, const std::shared_ptr<Meta> &trackDesc)
{
    AVCodecParameters* par = stream->codecpar;
    if (trackDesc->Find(Tag::VIDEO_COLOR_PRIMARIES) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_TRC) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_MATRIX_COEFF) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_RANGE) != trackDesc->end()) {
        ColorPrimary colorPrimaries = ColorPrimary::UNSPECIFIED;
        TransferCharacteristic colorTransfer = TransferCharacteristic::UNSPECIFIED;
        MatrixCoefficient colorMatrixCoeff = MatrixCoefficient::UNSPECIFIED;
        bool colorRange = false;
        trackDesc->Get<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);
        trackDesc->Get<Tag::VIDEO_COLOR_TRC>(colorTransfer);
        trackDesc->Get<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrixCoeff);
        trackDesc->Get<Tag::VIDEO_COLOR_RANGE>(colorRange);
        MEDIA_LOG_D("color info.: primary %{public}d, transfer %{public}d, matrix coeff %{public}d,"
            " range %{public}d,", colorPrimaries, colorTransfer, colorMatrixCoeff, colorRange);

        auto colorPri = ColorPrimary2AVColorPrimaries(colorPrimaries);
        FALSE_RETURN_V_MSG_E(colorPri.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color primary %{public}d", colorPrimaries);
        auto colorTrc = ColorTransfer2AVColorTransfer(colorTransfer);
        FALSE_RETURN_V_MSG_E(colorTrc.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color transfer %{public}d", colorTransfer);
        auto colorSpe = ColorMatrix2AVColorSpace(colorMatrixCoeff);
        FALSE_RETURN_V_MSG_E(colorSpe.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color matrix coeff %{public}d", colorMatrixCoeff);
        par->color_primaries = colorPri.second;
        par->color_trc = colorTrc.second;
        par->color_space = colorSpe.second;
        par->color_range = colorRange ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
        isColorSet_ = true;
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterColorByParser(AVStream* stream)
{
    if (!isColorSet_) {
        AVCodecParameters* par = stream->codecpar;
        bool colorRange = hevcParser_->GetColorRange();
        uint8_t colorPrimaries = hevcParser_->GetColorPrimaries();
        uint8_t colorTransfer = hevcParser_->GetColorTransfer();
        uint8_t colorMatrixCoeff = hevcParser_->GetColorMatrixCoeff();
        MEDIA_LOG_D("color info.: primary %{public}d, transfer %{public}d, matrix coeff %{public}d,"
            " range %{public}d,", colorPrimaries, colorTransfer, colorMatrixCoeff, colorRange);

        auto colorPri = ColorPrimary2AVColorPrimaries(static_cast<ColorPrimary>(colorPrimaries));
        FALSE_RETURN_V_MSG_E(colorPri.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color primary %{public}d", colorPrimaries);
        auto colorTrc = ColorTransfer2AVColorTransfer(static_cast<TransferCharacteristic>(colorTransfer));
        FALSE_RETURN_V_MSG_E(colorTrc.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color transfer %{public}d", colorTransfer);
        auto colorSpe = ColorMatrix2AVColorSpace(static_cast<MatrixCoefficient>(colorMatrixCoeff));
        FALSE_RETURN_V_MSG_E(colorSpe.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color matrix coeff %{public}d", colorMatrixCoeff);
        par->color_primaries = colorPri.second;
        par->color_trc = colorTrc.second;
        par->color_space = colorSpe.second;
        par->color_range = colorRange ? AVCOL_RANGE_JPEG : AVCOL_RANGE_MPEG;
        isColorSet_ = true;
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterCuva(AVStream* stream, const std::shared_ptr<Meta> &trackDesc)
{
    if (trackDesc->Find(Tag::VIDEO_IS_HDR_VIVID) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_IS_HDR_VIVID>(isHdrVivid_);
        MEDIA_LOG_D("hdr vivid: %{public}d", isHdrVivid_);
        if (isHdrVivid_) {
            av_dict_set(&stream->metadata, "hdr_type", "hdr_vivid", 0);
        }
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterCuvaByParser(AVStream *stream)
{
    if (!isHdrVivid_) {
        if (hevcParser_->IsHdrVivid()) {
            MEDIA_LOG_D("hdr vivid type by parser");
            av_dict_set(&stream->metadata, "hdr_type", "hdr_vivid", 0);
        }
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetDisplayMatrix(AVStream* stream)
{
    uint32_t displayMatrix[9] = {
        0x00010000, 0, 0, // matrix A B U
        0, 0x00010000, 0, // matrix C D V
        0, 0, 0x40000000 // matrix X Y W
    };
    if (rotation_ == VIDEO_ROTATION_90) {
        displayMatrix[0] = 0; // matrix A, index 0
        displayMatrix[1] = 0x00010000; // matrix B, index 1
        displayMatrix[3] = 0xFFFF0000; // matrix C, index 3
        displayMatrix[4] = 0; // matrix D, index 4
        displayMatrix[6] = static_cast<uint32_t>(stream->codecpar->height) << 16; // matrix X, index 6, shift 16
    } else if (rotation_ == VIDEO_ROTATION_180) {
        displayMatrix[0] = 0xFFFF0000; // matrix A, index 0
        displayMatrix[4] = 0xFFFF0000; // matrix D, index 4
        displayMatrix[6] = static_cast<uint32_t>(stream->codecpar->width) << 16; // matrix X, index 6, shift 16
        displayMatrix[7] = static_cast<uint32_t>(stream->codecpar->height) << 16; // matrix Y, index 7, shift 16
    } else if (rotation_ == VIDEO_ROTATION_270) {
        displayMatrix[0] = 0; // matrix A, index 0
        displayMatrix[1] = 0xFFFF0000; // matrix B, index 1
        displayMatrix[3] = 0x00010000; // matrix C, index 3
        displayMatrix[4] = 0; // matrix D, index 4
        displayMatrix[7] = static_cast<uint32_t>(stream->codecpar->width) << 16; // matrix Y, index 7, shift 16
    }
    uint8_t *data = av_stream_new_side_data(stream, AV_PKT_DATA_DISPLAYMATRIX, sizeof(displayMatrix));
    FALSE_RETURN_V_MSG_E(data != nullptr, Status::ERROR_NO_MEMORY, "alloc failed");
    errno_t rc = memcpy_s(data, sizeof(displayMatrix), displayMatrix, sizeof(displayMatrix));
    FALSE_RETURN_V_MSG_E(rc == EOK, Status::ERROR_UNKNOWN, "memcpy_s failed");
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::AddAudioTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID)
{
    int32_t sampleRate = 0;
    int32_t channels = 0;
    bool ret = trackDesc->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate); // sample rate
    FALSE_RETURN_V_MSG_E(ret && sampleRate > 0, Status::ERROR_MISMATCHED_TYPE,
        "get audio sample_rate failed! sampleRate:%{public}d", sampleRate);
    ret = trackDesc->Get<Tag::AUDIO_CHANNEL_COUNT>(channels); // channels
    FALSE_RETURN_V_MSG_E(ret && channels > 0, Status::ERROR_MISMATCHED_TYPE,
        "get audio channels failed! channels:%{public}d", channels);

    auto st = avformat_new_stream(formatContext_.get(), nullptr);
    FALSE_RETURN_V_MSG_E(st != nullptr, Status::ERROR_NO_MEMORY, "avformat_new_stream failed!");
    ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codecpar->codec_id = codeID;
    st->codecpar->sample_rate = sampleRate;
    st->codecpar->channels = channels;
    int32_t frameSize = 0;
    if (trackDesc->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != trackDesc->end()) {
        trackDesc->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize); // frame size
        FALSE_RETURN_V_MSG_E(frameSize > 0, Status::ERROR_MISMATCHED_TYPE,
            "get audio sample per frame failed! audio sample per frame:%{public}d", frameSize);
        st->codecpar->frame_size = frameSize;
    }
    trackIndex = st->index;
    return SetCodecParameterOfTrack(st, trackDesc);
}

Status FFmpegMuxerPlugin::AddVideoTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc,
                                        AVCodecID codeID, bool isCover)
{
    constexpr int32_t maxLength = 65535;
    constexpr int32_t maxVideoDelay = 16;
    int32_t width = 0;
    int32_t height = 0;
    bool ret = trackDesc->Get<Tag::VIDEO_WIDTH>(width); // width
    FALSE_RETURN_V_MSG_E(ret && width > 0 && width <= maxLength, Status::ERROR_INVALID_PARAMETER,
        "get video width failed! width:%{public}d", width);
    ret =  trackDesc->Get<Tag::VIDEO_HEIGHT>(height); // height
    FALSE_RETURN_V_MSG_E((ret && height > 0 && height <= maxLength), Status::ERROR_INVALID_PARAMETER,
        "get video height failed! height:%{public}d", height);

    auto st = avformat_new_stream(formatContext_.get(), nullptr);
    FALSE_RETURN_V_MSG_E(st != nullptr, Status::ERROR_NO_MEMORY, "avformat_new_stream failed!");
    ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codecpar->codec_id = codeID;
    st->codecpar->codec_tag = codeID == AV_CODEC_ID_HEVC ? MKTAG('h', 'v', 'c', '1') : 0;
    st->codecpar->width = width;
    st->codecpar->height = height;
    int32_t videoDelay = 0;
    if (trackDesc->Find(Tag::VIDEO_DELAY) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_DELAY>(videoDelay); // video delay
        FALSE_RETURN_V_MSG_E(videoDelay >= 0, Status::ERROR_MISMATCHED_TYPE,
            "get video delay failed! video delay:%{public}d", videoDelay);
        st->codecpar->video_delay = videoDelay;
    }

    trackIndex = st->index;
    if (isCover) {
        st->disposition = AV_DISPOSITION_ATTACHED_PIC;
    }
    double frameRate = 0;
    if (trackDesc->Find(Tag::VIDEO_FRAME_RATE) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_FRAME_RATE>(frameRate); // video frame rate
        FALSE_RETURN_V_MSG_E(frameRate > 0, Status::ERROR_MISMATCHED_TYPE,
            "get video frame rate failed! video frame rate:%{public}lf", frameRate);
        st->avg_frame_rate = {static_cast<int32_t>(frameRate), 1};
    }
    FALSE_RETURN_V_MSG_E((videoDelay > 0 && videoDelay <= maxVideoDelay && frameRate > 0) || videoDelay == 0,
        Status::ERROR_MISMATCHED_TYPE, "If the video delayed, the frame rate is required. "
        "The delay is greater than or equal to 0 and less than or equal to 16.");

    auto retColor = SetCodecParameterColor(st, trackDesc);
    FALSE_RETURN_V_MSG_E(retColor == Status::NO_ERROR, retColor, "set color failed!");
    auto retCuva = SetCodecParameterCuva(st, trackDesc);
    FALSE_RETURN_V_MSG_E(retCuva == Status::NO_ERROR, retCuva, "set cuva failed!");
    return SetCodecParameterOfTrack(st, trackDesc);
}

Status FFmpegMuxerPlugin::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    FALSE_RETURN_V_MSG_E(!isWriteHeader_, Status::ERROR_WRONG_STATE, "AddTrack failed! muxer has start!");
    FALSE_RETURN_V_MSG_E(outputFormat_ != nullptr, Status::ERROR_NULL_POINTER, "AVOutputFormat is nullptr");
    constexpr int32_t mimeTypeLen = 5;
    Status ret = Status::NO_ERROR;
    std::string mimeType = {};
    AVCodecID codeID = AV_CODEC_ID_NONE;
    FALSE_RETURN_V_MSG_E(trackDesc->Get<Tag::MIME_TYPE>(mimeType), Status::ERROR_INVALID_PARAMETER,
        "get mimeType failed!"); // mime
    MEDIA_LOG_D("mimeType is %{public}s", mimeType.c_str());
    FALSE_RETURN_V_MSG_E(Mime2CodecId(mimeType, codeID), Status::ERROR_INVALID_DATA,
        "this mimeType do not support! mimeType:%{public}s", mimeType.c_str());

    if (codeID == AV_CODEC_ID_HEVC && hevcParser_ == nullptr) {
        hevcParser_ = StreamParserManager::Create(StreamType::HEVC);
        FALSE_RETURN_V_MSG_E(hevcParser_ != nullptr, Status::ERROR_INVALID_DATA,
            "this mimeType do not support! mimeType:%{public}s", mimeType.c_str());
    }

    if (!mimeType.compare(0, mimeTypeLen, "audio")) {
        ret = AddAudioTrack(trackIndex, trackDesc, codeID);
        FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "AddAudioTrack failed!");
    } else if (!mimeType.compare(0, mimeTypeLen, "video")) {
        ret = AddVideoTrack(trackIndex, trackDesc, codeID, false);
        FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "AddVideoTrack failed!");
    } else if (!mimeType.compare(0, mimeTypeLen, "image")) {
        ret = AddVideoTrack(trackIndex, trackDesc, codeID, true);
        FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "AddCoverTrack failed!");
    } else {
        MEDIA_LOG_D("mimeType %{public}s is unsupported", mimeType.c_str());
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    uint32_t flags = static_cast<uint32_t>(formatContext_->flags);
    formatContext_->flags = static_cast<int32_t>(flags | AVFMT_TS_NONSTRICT);
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::Start()
{
    FALSE_RETURN_V_MSG_E(formatContext_->pb != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    FALSE_RETURN_V_MSG_E(!isWriteHeader_, Status::ERROR_WRONG_STATE, "Start failed! muxer has start!");
    if (rotation_ != VIDEO_ROTATION_0) {
        std::string rotate = std::to_string(rotation_);
        for (uint32_t i = 0; i < formatContext_->nb_streams; i++) {
            if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                av_dict_set(&formatContext_->streams[i]->metadata, "rotate", rotate.c_str(), 0);
                FALSE_LOG_MSG(SetDisplayMatrix(formatContext_->streams[i]) == Status::NO_ERROR,
                    "set rotation failed!");
            }
        }
    }
    if (av_dict_get(formatContext_->metadata, "creation_time", nullptr, 0) == nullptr) {
        av_dict_set(&formatContext_->metadata, "creation_time", "now", 0);
    }
    AVDictionary *options = nullptr;
    if (static_cast<IOContext*>(formatContext_->pb->opaque)->dataSink_->CanRead() && isFastStart_) {
        av_dict_set(&options, "movflags", "faststart", 0);
    }
    int ret = avformat_write_header(formatContext_.get(), &options);
    if (ret < 0) {
        MEDIA_LOG_E("write header failed, %{public}s", AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }
    isWriteHeader_ = true;
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::Stop()
{
    FALSE_RETURN_V_MSG_E(isWriteHeader_, Status::ERROR_WRONG_STATE, "Stop failed! Did not write header!");
    int ret = av_write_frame(formatContext_.get(), nullptr); // flush out cache data
    if (ret < 0) {
        MEDIA_LOG_E("write trailer failed, %{public}s", AVStrError(ret).c_str());
    }
    ret = av_write_trailer(formatContext_.get());
    if (ret != 0) {
        MEDIA_LOG_E("write trailer failed, %{public}s", AVStrError(ret).c_str());
    }
    avio_flush(formatContext_->pb);

    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::Reset()
{
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(isWriteHeader_, Status::ERROR_WRONG_STATE, "WriteSample failed! Did not write header!");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_NULL_POINTER,
        "av_write_frame sample is null!");
    FALSE_RETURN_V_MSG_E(trackIndex < formatContext_->nb_streams, Status::ERROR_INVALID_PARAMETER,
        "track index is invalid!");
    MEDIA_LOG_D("WriteSample track:" PUBLIC_LOG_U32 ", pts:" PUBLIC_LOG_D64 ", size:" PUBLIC_LOG_D32
        ", flags:" PUBLIC_LOG_U32, trackIndex, sample->pts_, sample->memory_->GetSize(), sample->flag_);

    std::lock_guard<std::mutex> lock(mutex_);
    auto st = formatContext_->streams[trackIndex];
    if (st->codecpar->codec_id == AV_CODEC_ID_H264 || st->codecpar->codec_id == AV_CODEC_ID_HEVC) {
        return WriteVideoSample(trackIndex, sample);
    }
    return WriteNormal(trackIndex, sample);
}

Status FFmpegMuxerPlugin::WriteNormal(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    auto st = formatContext_->streams[trackIndex];
    (void)memset_s(cachePacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    cachePacket_->data = sample->memory_->GetAddr();
    cachePacket_->size = sample->memory_->GetSize();
    cachePacket_->stream_index = static_cast<int>(trackIndex);
    cachePacket_->pts = ConvertTimeToFFmpeg(sample->pts_ * TIMESTAMP_US, st->time_base);

    if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        cachePacket_->dts = cachePacket_->pts;
    } else {
        cachePacket_->dts = AV_NOPTS_VALUE;
    }
    cachePacket_->flags = 0;
    if (sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) {
        MEDIA_LOG_D("It is key frame");
        cachePacket_->flags |= AV_PKT_FLAG_KEY;
    }
    if (sample->flag_ & static_cast<uint32_t>(AVBufferFlag::DISPOSABLE)) {
        MEDIA_LOG_D("It is disposable frame");
        cachePacket_->flags |= AV_PKT_FLAG_DISPOSABLE;
    }
    if (sample->flag_ & static_cast<uint32_t>(AVBufferFlag::DISPOSABLE_EXT)) {
        MEDIA_LOG_D("It is disposable_ext frame");
        cachePacket_->flags |= AV_PKT_FLAG_DISPOSABLE_EXT;
    }

    auto ret = av_write_frame(formatContext_.get(), cachePacket_.get());
    av_packet_unref(cachePacket_.get());
    if (ret < 0) {
        MEDIA_LOG_E("write sample buffer failed, " PUBLIC_LOG_S ". track id: " PUBLIC_LOG_U32
            ", pts: " PUBLIC_LOG_D64 ", flag: " PUBLIC_LOG_U32 ", size: " PUBLIC_LOG_D32,
            AVStrError(ret).c_str(), trackIndex, sample->pts_, sample->flag_, sample->memory_->GetSize());
        return Status::ERROR_UNKNOWN;
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::WriteVideoSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    auto st = formatContext_->streams[trackIndex];
    if (videoTracksInfo_[trackIndex].isFirstFrame_) {
        videoTracksInfo_[trackIndex].isFirstFrame_ = false;
        if (!IsAvccSample(sample->memory_->GetAddr(), sample->memory_->GetSize(),
            videoTracksInfo_[trackIndex].nalSizeLen_)) {
            FALSE_RETURN_V_MSG_E(sample->flag_ & static_cast<uint32_t>(AVBufferFlag::CODEC_DATA),
                Status::ERROR_INVALID_DATA,
                "first frame flag of annex-b stream need AVCODEC_BUFFER_FLAGS_CODEC_DATA!");
            if (st->codecpar->codec_id == AV_CODEC_ID_HEVC) {
                uint8_t *extra = nullptr;
                int32_t size = 0;
                videoTracksInfo_[trackIndex].isNeedTransData_ = true;
                hevcParser_->ParseExtraData(sample->memory_->GetAddr(), sample->memory_->GetSize(), &extra, &size);
                FALSE_RETURN_V_MSG_E(SetCodecParameterExtra(st, extra, size) == Status::NO_ERROR,
                    Status::ERROR_INVALID_DATA, "set codec config failed!");
                FALSE_RETURN_V_MSG_E(SetCodecParameterColorByParser(st) == Status::NO_ERROR,
                    Status::ERROR_INVALID_DATA, "set color failed!");
                FALSE_RETURN_V_MSG_E(SetCodecParameterCuvaByParser(st) == Status::NO_ERROR,
                    Status::ERROR_INVALID_DATA, "set cuva flag failed!");
            }
            if (!(sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) &&
                (st->codecpar->codec_id == AV_CODEC_ID_H264)) {
                return SetCodecParameterExtra(st, sample->memory_->GetAddr(), sample->memory_->GetSize());
            } else if (!(sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME))) {
                return Status::NO_ERROR;
            }
        } else {
            FALSE_RETURN_V_MSG_E(!videoTracksInfo_[trackIndex].extraData_.empty(),
                Status::ERROR_INVALID_DATA, "non annexb stream must set codec config!");
            auto &extra = videoTracksInfo_[trackIndex].extraData_;
            SetCodecParameterExtra(st, extra.data(), extra.size());
        }
    }
    if (videoTracksInfo_[trackIndex].isNeedTransData_) {
        std::vector<uint8_t> nonAnnexbData = TransAnnexbToMp4(sample->memory_->GetAddr(), sample->memory_->GetSize());
        int32_t size = static_cast<int32_t>(nonAnnexbData.size());
        FALSE_RETURN_V_MSG_E(size > 0, Status::ERROR_INVALID_DATA, "annexb to mp4 is empty!");
        std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(nonAnnexbData.data(), size, size);
        buffer->pts_ = sample->pts_;
        buffer->flag_ = sample->flag_;
        return WriteNormal(trackIndex, buffer);
    }
    return WriteNormal(trackIndex, sample);
}

std::vector<uint8_t> FFmpegMuxerPlugin::TransAnnexbToMp4(const uint8_t *sample, int32_t size)
{
    std::vector<uint8_t> data;
    uint8_t *nalStart = const_cast<uint8_t *>(sample);
    uint8_t *end = nalStart + size;
    uint8_t *nalEnd = nullptr;
    int32_t startCodeLen = 0;
    uint32_t naluSize = 0;

    nalStart = FindNalStartCode(nalStart, end, startCodeLen);
    nalStart = nalStart + startCodeLen;
    while (nalStart < end) {
        nalEnd = FindNalStartCode(nalStart, end, startCodeLen);
        naluSize = static_cast<uint32_t>(nalEnd - nalStart);
        for (int32_t i = sizeof(naluSize) - 1; i >= 0; --i) {
            data.emplace_back((naluSize >> (i * 0x08)) & 0xFF);
        }
        data.insert(data.end(), nalStart, nalEnd);
        nalStart = nalEnd + startCodeLen;
    }
    return data;
}

uint8_t *FFmpegMuxerPlugin::FindNalStartCode(const uint8_t *buf, const uint8_t *end, int32_t &startCodeLen)
{
    startCodeLen = sizeof(START_CODE);
    auto *iter = std::search(buf, end, START_CODE, START_CODE + startCodeLen);
    if (iter != end) {
        if (iter > buf && *(iter - 1) == 0x00) {
            ++startCodeLen;
            return const_cast<uint8_t *>(iter - 1);
        }
    }

    return const_cast<uint8_t *>(iter);
}

bool FFmpegMuxerPlugin::IsAvccSample(const uint8_t* sample, int32_t size, int32_t nalSizeLen)
{
    if (size < nalSizeLen) {
        return false;
    }
    uint64_t naluSize = 0;
    uint64_t curPos = 0;
    uint64_t cmpSize = static_cast<uint64_t>(size);
    while (curPos + nalSizeLen <= cmpSize) {
        naluSize = 0;
        for (uint64_t i = curPos; i < curPos + nalSizeLen; i++) {
            naluSize = sample[i] + naluSize * 0x100;
        }
        if (naluSize <= 1) {
            return false;
        }
        curPos += (naluSize + nalSizeLen);
    }
    return curPos == cmpSize;
}

Status FFmpegMuxerPlugin::SetNalSizeLen(AVStream *stream, const std::vector<uint8_t> &codecConfig)
{
    const int32_t h264NalSizeIndex = 4;
    const int32_t hevcNalSizeIndex = 21;
    const size_t h264CodecConfigSize = 6;
    const size_t hevcCodecConfigSize = 23;
    AVCodecParameters *par = stream->codecpar;
    if (par->codec_id == AV_CODEC_ID_H264 && codecConfig.size() > h264CodecConfigSize) {
        videoTracksInfo_[stream->index].nalSizeLen_ = static_cast<uint32_t>(codecConfig[h264NalSizeIndex] & 0x03) + 1;
    } else if (par->codec_id == AV_CODEC_ID_HEVC && codecConfig.size() > hevcCodecConfigSize) {
        videoTracksInfo_[stream->index].nalSizeLen_ = static_cast<uint32_t>(codecConfig[hevcNalSizeIndex] & 0x03) + 1;
    } else {
        MEDIA_LOG_E("SetNalSizeLen failed! codec config size is " PUBLIC_LOG_ZU, codecConfig.size());
        return Status::ERROR_INVALID_DATA;
    }
    return Status::NO_ERROR;
}

AVIOContext* FFmpegMuxerPlugin::InitAvIoCtx(const std::shared_ptr<DataSink>& dataSink, int writeFlags)
{
    if (dataSink == nullptr) {
        return nullptr;
    }
    IOContext *ioContext = new IOContext();
    ioContext->dataSink_ = dataSink;
    ioContext->pos_ = 0;
    ioContext->end_ = dataSink->Seek(0ll, SEEK_END);

    constexpr int bufferSize = 4 * 1024; // 4096
    auto buffer = static_cast<unsigned char*>(av_malloc(bufferSize));
    AVIOContext *avioContext = avio_alloc_context(buffer, bufferSize, writeFlags, static_cast<void*>(ioContext),
                                                  IoRead, IoWrite, IoSeek);
    if (avioContext == nullptr) {
        delete ioContext;
        av_free(buffer);
        return nullptr;
    }
    avioContext->seekable = AVIO_SEEKABLE_NORMAL;
    return avioContext;
}

void FFmpegMuxerPlugin::DeInitAvIoCtx(AVIOContext *ptr)
{
    if (ptr != nullptr) {
        delete static_cast<IOContext*>(ptr->opaque);
        ptr->opaque = nullptr;
        av_freep(&ptr->buffer);
        av_opt_free(ptr);
        avio_context_free(&ptr);
    }
}

int32_t FFmpegMuxerPlugin::IoRead(void *opaque, uint8_t *buf, int bufSize)
{
    auto ioCtx = static_cast<IOContext*>(opaque);
    if (ioCtx != nullptr && ioCtx->dataSink_ != nullptr) {
        if (ioCtx->pos_ >= ioCtx->end_) {
            return AVERROR_EOF;
        }
        int64_t ret = ioCtx->dataSink_->Seek(ioCtx->pos_, SEEK_SET);
        if (ret != -1) {
            int32_t size = ioCtx->dataSink_->Read(buf, bufSize);
            if (size == 0) {
                return AVERROR_EOF;
            }
            ioCtx->pos_ += size;
            return size;
        }
        return 0;
    }
    return -1;
}

int32_t FFmpegMuxerPlugin::IoWrite(void *opaque, uint8_t *buf, int bufSize)
{
    auto ioCtx = static_cast<IOContext*>(opaque);
    if (ioCtx != nullptr && ioCtx->dataSink_ != nullptr) {
        int64_t ret = ioCtx->dataSink_->Seek(ioCtx->pos_, SEEK_SET);
        if (ret != -1) {
            int32_t size = ioCtx->dataSink_->Write(buf, bufSize);
            if (size < 0) {
                return -1;
            }
            ioCtx->pos_ += size;
            if (ioCtx->pos_ > ioCtx->end_) {
                ioCtx->end_ = ioCtx->pos_;
            }
            return size;
        }
        return 0;
    }
    return -1;
}

int64_t FFmpegMuxerPlugin::IoSeek(void *opaque, int64_t offset, int whence)
{
    auto ioContext = static_cast<IOContext*>(opaque);
    int64_t newPos = 0;
    switch (whence) {
        case SEEK_SET:
            newPos = offset;
            ioContext->pos_ = newPos;
            break;
        case SEEK_CUR:
            newPos = ioContext->pos_ + offset;
            break;
        case SEEK_END:
        case AVSEEK_SIZE:
            newPos = ioContext->end_ + offset;
            break;
        default:
            break;
    }
    if (whence != AVSEEK_SIZE) {
        ioContext->pos_ = newPos;
    }
    return newPos;
}

int32_t FFmpegMuxerPlugin::IoOpen(AVFormatContext *s, AVIOContext **pb,
                                  const char *url, int flags, AVDictionary **options)
{
    MEDIA_LOG_D("IoOpen flags %{public}d", flags);
    *pb = InitAvIoCtx(static_cast<IOContext*>(s->pb->opaque)->dataSink_, 0);
    if (*pb == nullptr) {
        MEDIA_LOG_E("IoOpen failed");
        return -1;
    }
    return 0;
}

void FFmpegMuxerPlugin::IoClose(AVFormatContext *s, AVIOContext *pb)
{
    avio_flush(pb);
    DeInitAvIoCtx(pb);
}
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
