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
#include "ffmpeg_muxer_register.h"

#include <malloc.h>
#include <set>

#include "securec.h"
#include "common/log.h"
#include "ffmpeg_utils.h"
#include "ffmpeg_converter.h"
#include "meta/mime_type.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_MUXER, "FFmpegMuxerPlugin"};

using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

constexpr float LATITUDE_MIN = -90.0f;
constexpr float LATITUDE_MAX = 90.0f;
constexpr float LONGITUDE_MIN = -180.0f;
constexpr float LONGITUDE_MAX = 180.0f;
constexpr int32_t MIN_HE_AAC_SAMPLE_RATE = 16000;
constexpr int32_t MAX_USERMETA_STRING_LENGTH = 256;
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
FFmpegMuxerPlugin::FFmpegMuxerPlugin(std::string name)
    : MuxerPlugin(std::move(name)), isWriteHeader_(false)
{
    MEDIA_LOG_D("%{public}" PRIu64 " Instances create", avcodecUniqueId_);
#ifndef _WIN32
    mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
#endif
    av_log_set_callback(FFmpegMuxerRegister::FfmpegLogPrintWithLevel);
    auto pkt = av_packet_alloc();
    cachePacket_ = std::shared_ptr<AVPacket> (pkt, [] (AVPacket *packet) {av_packet_free(&packet);});
    outputFormat_ = FFmpegMuxerRegister::GetAVOutputFormat(pluginName_);
    auto fmt = avformat_alloc_context();
    if (fmt) {
        fmt->pb = nullptr;
        fmt->oformat = outputFormat_.get();
        fmt->flags = static_cast<uint32_t>(fmt->flags) | static_cast<uint32_t>(AVFMT_FLAG_CUSTOM_IO);
        fmt->io_open = FFmpegMuxerRegister::IoOpen;
        fmt->io_close2 = FFmpegMuxerRegister::IoClose;
    } else {
        MEDIA_LOG_E("Failed to allocate AVFormatContext, fmt is null.");
    }
    formatContext_ = std::shared_ptr<AVFormatContext>(fmt, [](AVFormatContext *ptr) {
        if (ptr) {
            FFmpegMuxerRegister::DeInitAvIoCtx(ptr->pb);
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
    MEDIA_LOG_D("%{public}" PRIu64 " Instances destroy", avcodecUniqueId_);
#ifndef _WIN32
    mallopt(M_FLUSH_THREAD_CACHE, 0);
#endif
}

Status FFmpegMuxerPlugin::SetDataSink(const std::shared_ptr<DataSink> &dataSink)
{
    FALSE_RETURN_V_MSG_E(dataSink != nullptr, Status::ERROR_INVALID_PARAMETER, "data sink is null");
    FFmpegMuxerRegister::DeInitAvIoCtx(formatContext_->pb);
    formatContext_->pb = FFmpegMuxerRegister::InitAvIoCtx(dataSink, 1);
    FALSE_RETURN_V_MSG_E(formatContext_->pb != nullptr, Status::ERROR_INVALID_OPERATION, "failed to create io");
    canReadFile_ = dataSink->CanRead();
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetParameter(const std::shared_ptr<Meta> &param)
{
    Status ret = Status::NO_ERROR;
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
        VideoRotation rotation = VIDEO_ROTATION_0;
        param->Get<Tag::VIDEO_ROTATION>(rotation); // rotation
        if (rotation != VIDEO_ROTATION_0 && rotation != VIDEO_ROTATION_90 &&
            rotation != VIDEO_ROTATION_180 && rotation != VIDEO_ROTATION_270) {
            MEDIA_LOG_W("Invalid rotation: %{public}d, keep default 0", rotation);
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
    std::string location = std::to_string(latitude) + " ";
    location += std::to_string(longitude) + " ";
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
        {Tag::MEDIA_AIGC, "aigc"},
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

Status FFmpegMuxerPlugin::SetOggUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    std::vector<std::string> keys;
    userMeta->GetKeys(keys);
    FALSE_RETURN_V_MSG_E(keys.size() > 0, Status::ERROR_INVALID_DATA, "ogg user meta is empty!");

    for (auto& k: keys) {
        if (k.compare(0, 16, "com.openharmony.") != 0) { // 16 "com.openharmony." length
            MEDIA_LOG_W("the meta key %{public}s must com.openharmony.xxx!", k.c_str());
            continue;
        }
        std::string value = "";
        if (!userMeta->GetData(k, value)) {
            MEDIA_LOG_W("ogg meta key must be string, %{public}s invalid type", k.c_str());
            continue;
        }
        if (value.length() > MAX_USERMETA_STRING_LENGTH) {
            MEDIA_LOG_E("the usermeta key %{public}s string value length %{public}zu more than 256 characters.",
                k.c_str(), value.length());
            return Status::ERROR_INVALID_DATA;
        }
        av_dict_set(&formatContext_->metadata, k.c_str(), value.c_str(), 0);
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    if (pluginName_ == "ffmpegMux_ogg") {
        return SetOggUserMeta(userMeta);
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterOfAudioTrack(AVStream *stream, const std::shared_ptr<Meta> &trackDesc)
{
    AVCodecParameters *par = stream->codecpar;
    if (trackDesc->Find(Tag::MEDIA_BITRATE) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_BITRATE>(par->bit_rate); // bit rate
    }

    if (pluginName_ == "ffmpegMux_adts" && CheckAacParam(trackDesc) != Status::NO_ERROR) {
        MEDIA_LOG_E("aac is adts, but CheckAacParam failed");
        return Status::ERROR_MISMATCHED_TYPE;
    }

    int32_t isAacAdts = 0;
    if (pluginName_ == "ffmpegMux_adts" && trackDesc->Get<Tag::AUDIO_AAC_IS_ADTS>(isAacAdts) && isAacAdts == 1) {
        MEDIA_LOG_I("aac is adts");
        return Status::NO_ERROR;
    }

    std::vector<uint8_t> codecConfig;
    if (trackDesc->Find(Tag::MEDIA_CODEC_CONFIG) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig); // codec config
        return SetCodecParameterExtra(stream, codecConfig.data(), codecConfig.size());
    } else if (par->codec_id == AV_CODEC_ID_AAC) {
        if (trackDesc->Find(Tag::MEDIA_PROFILE) != trackDesc->end()) {
            int32_t profile;
            int32_t sampleRate;
            int32_t channels;
            trackDesc->Get<Tag::MEDIA_PROFILE>(profile);
            trackDesc->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
            if ((profile == AAC_PROFILE_HE || profile == AAC_PROFILE_HE_V2) &&
                sampleRate < MIN_HE_AAC_SAMPLE_RATE) {
                MEDIA_LOG_E("HE-AAC only support sample rate >= 16k, input rate:%{public}d", sampleRate);
                return Status::ERROR_INVALID_PARAMETER;
            }
            trackDesc->Get<Tag::AUDIO_CHANNEL_COUNT>(channels);
            codecConfig = GenerateAACCodecConfig(profile, sampleRate, channels);
            return SetCodecParameterExtra(stream, codecConfig.data(), codecConfig.size());
        }
        MEDIA_LOG_W("missing codec config of aac!");
    } else if (par->codec_id == AV_CODEC_ID_FLAC && PluginBase::GetName() == "ffmpegMux_flac") {
        flacCodecConfig_ = std::make_shared<FlacCodecConfig>();
        if (!flacCodecConfig_->GenerateCodecConfig(trackDesc)) {
            flacCodecConfig_ = nullptr;
            MEDIA_LOG_E("flac GenerateCodecConfig failed!");
            return Status::ERROR_INVALID_PARAMETER;
        }
        codecConfig = flacCodecConfig_->mCodecConfig;
        return SetCodecParameterExtra(stream, codecConfig.data(), codecConfig.size());
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

Status FFmpegMuxerPlugin::CheckAacParam(const std::shared_ptr<Meta> &trackDesc)
{
    int32_t profile = 0;
    int32_t adts = 0;
    Status ret = Status::NO_ERROR;

    trackDesc->Get<Tag::MEDIA_PROFILE>(profile);
    if (profile != AAC_PROFILE_LC && profile != AAC_PROFILE_HE && profile != AAC_PROFILE_HE_V2) {
        ret = Status::ERROR_MISMATCHED_TYPE;
        MEDIA_LOG_E("invalid AAC profile, profile: %{public}d", profile);
    }

    trackDesc->Get<Tag::AUDIO_AAC_IS_ADTS>(adts);
    if (adts != 0 && adts != 1) {
        ret = Status::ERROR_MISMATCHED_TYPE;
        MEDIA_LOG_E("invalid AAC adts, adts: %{public}d", adts);
    }
    return ret;
}

void FFmpegMuxerPlugin::CheckChannelLayout(uint64_t channelLayout, const std::shared_ptr<Meta> &trackDesc)
{
    if (channelLayout == AudioChannelLayout::STEREO_DOWNMIX) {
        formatContext_->strict_std_compliance = FF_COMPLIANCE_UNOFFICIAL;
        MEDIA_LOG_W("channelLayout is STEREO_DOWNMIX, set compliance level: " PUBLIC_LOG_D32,
                    FF_COMPLIANCE_UNOFFICIAL);
    }
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
    if (codeID == AV_CODEC_ID_PCM_U8) {
        AudioSampleFormat sampleFormat = INVALID_WIDTH;
        ret = trackDesc->Get<Tag::AUDIO_SAMPLE_FORMAT>(sampleFormat); // sampleFormat
        FALSE_RETURN_V_MSG_E(ret, Status::ERROR_MISMATCHED_TYPE, "get audio sample format failed!");
        ret = Raw2CodecId(sampleFormat, codeID);
        FALSE_RETURN_V_MSG_E(ret, Status::ERROR_INVALID_DATA,
            "this mimeType do not support! mimeType:%{public}s, sampleFormat:%{public}d",
            MimeType::AUDIO_RAW, sampleFormat);
    }

    auto st = avformat_new_stream(formatContext_.get(), nullptr);
    FALSE_RETURN_V_MSG_E(st != nullptr, Status::ERROR_NO_MEMORY, "avformat_new_stream failed!");
    FFmpegMuxerRegister::ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codecpar->codec_id = codeID;
    st->codecpar->sample_rate = sampleRate;
    if (trackDesc->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != trackDesc->end()) {
        int32_t frameSize = 0;
        trackDesc->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize); // frame size
        FALSE_RETURN_V_MSG_E(frameSize > 0, Status::ERROR_MISMATCHED_TYPE,
            "get audio sample per frame failed! audio sample per frame:%{public}d", frameSize);
        st->codecpar->frame_size = frameSize;
    }
    if (trackDesc->Find(Tag::AUDIO_CHANNEL_LAYOUT) != trackDesc->end()) {
        AudioChannelLayout channelLayout = UNKNOWN;
        trackDesc->Get<Tag::AUDIO_CHANNEL_LAYOUT>(channelLayout);
        auto ffChannelLayout = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(channelLayout);
        CheckChannelLayout(ffChannelLayout, trackDesc);
        MEDIA_LOG_D("channelLayout:" PUBLIC_LOG_D64 ", ffChannelLayout:" PUBLIC_LOG_U64,
            channelLayout, ffChannelLayout);
        FALSE_RETURN_V_MSG_E(ffChannelLayout, Status::ERROR_INVALID_DATA,
            "the value of channelLayout is not supported, " PUBLIC_LOG_D64, channelLayout);
        av_channel_layout_from_mask(&st->codecpar->ch_layout, ffChannelLayout);
    } else {
        av_channel_layout_default(&st->codecpar->ch_layout, channels);
    }
    trackIndex = st->index;
    return SetCodecParameterOfAudioTrack(st, trackDesc);
}

Status FFmpegMuxerPlugin::AddVideoTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc,
                                        AVCodecID codeID, bool isCover)
{
    constexpr int32_t maxLength = 65535;
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
    FFmpegMuxerRegister::ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codecpar->codec_id = codeID;
    st->codecpar->codec_tag = 0;
    st->codecpar->width = width;
    st->codecpar->height = height;

    trackIndex = st->index;
    if (isCover) {
        st->disposition = AV_DISPOSITION_ATTACHED_PIC;
    }
    return Status::NO_ERROR;
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
    FALSE_RETURN_V_MSG_E(Mime2CodecId(mimeType, codeID), Status::ERROR_INVALID_DATA,
        "this mimeType do not support! mimeType:%{public}s", mimeType.c_str());

    if (!mimeType.compare(0, mimeTypeLen, "audio")) {
        ret = AddAudioTrack(trackIndex, trackDesc, codeID);
        FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "AddAudioTrack failed!");
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
    if (av_dict_get(formatContext_->metadata, "creation_time", nullptr, 0) == nullptr) {
        av_dict_set(&formatContext_->metadata, "creation_time", "now", 0);
    }
    AVDictionary *options = nullptr;
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
    if (flacCodecConfig_ != nullptr) {
        FALSE_RETURN_V_MSG_E(flacCodecConfig_->Update(), Status::ERROR_UNKNOWN, "flac codec config update failed!");
        UpdateExtraData(flacCodecConfig_->mCodecConfig.data(), flacCodecConfig_->mCodecConfig.size());
    }
    int ret = av_write_frame(formatContext_.get(), nullptr); // flush out cache data
    if (ret < 0) {
        MEDIA_LOG_E("write trailer failed, %{public}s", AVStrError(ret).c_str());
    }
    ret = av_write_trailer(formatContext_.get());
    if (ret != 0) {
        MEDIA_LOG_E("write trailer failed, %{public}s", AVStrError(ret).c_str());
    }
    avio_flush(formatContext_->pb);
    MEDIA_LOG_I("write trailer successfully");

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
    if (st->codecpar->codec_id == AV_CODEC_ID_FLAC && flacCodecConfig_ != nullptr &&
        sample->flag_ == static_cast<uint32_t>(AVBufferFlag::CODEC_DATA) &&
        static_cast<size_t>(sample->memory_->GetSize()) == flacCodecConfig_->mCodecConfig.size()) {
            flacCodecConfig_->UpdateNewConfig(sample->memory_->GetAddr(), sample->memory_->GetSize());
            return Status::NO_ERROR;
    }
    return WriteNormal(trackIndex, sample, 0);
}

Status FFmpegMuxerPlugin::UpdateExtraData(uint8_t *data, size_t size)
{
    (void)memset_s(cachePacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    cachePacket_->side_data =
        static_cast<AVPacketSideData *>(av_malloc(sizeof(AVPacketSideData) + AV_INPUT_BUFFER_PADDING_SIZE));
    FALSE_RETURN_V_MSG_E(cachePacket_->side_data != nullptr, Status::ERROR_UNKNOWN, "sideData av_mallocz failed!");
    cachePacket_->side_data_elems = 0;
    cachePacket_->side_data[0].data = static_cast<uint8_t *>(av_malloc(size + AV_INPUT_BUFFER_PADDING_SIZE));
    if (cachePacket_->side_data[0].data == nullptr) {
        av_packet_unref(cachePacket_.get());
        MEDIA_LOG_E("sideData data av_mallocz failed!");
        return Status::ERROR_UNKNOWN;
    }
    cachePacket_->side_data_elems = 1;
    cachePacket_->side_data[0].size = size;
    cachePacket_->side_data[0].type = AV_PKT_DATA_NEW_EXTRADATA;
    if (memcpy_s(cachePacket_->side_data[0].data, size, data, size) != EOK) {
        av_packet_unref(cachePacket_.get());
        MEDIA_LOG_E("sideData memcpy_s failed!");
        return Status::ERROR_UNKNOWN;
    }
    auto ret = av_write_frame(formatContext_.get(), cachePacket_.get());
    av_packet_unref(cachePacket_.get());
    if (ret < 0) {
        MEDIA_LOG_E("UpdateExtraData failed!");
        return Status::ERROR_UNKNOWN;
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::WriteNormal(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample, int32_t dataOffset)
{
    auto st = formatContext_->streams[trackIndex];
    FALSE_RETURN_V_MSG_E((dataOffset >= 0 && dataOffset <= sample->memory_->GetSize()),
        Status::ERROR_INVALID_DATA, "error: data offset is invalid");
    uint8_t *data = sample->memory_->GetAddr() + dataOffset;
    int32_t size = sample->memory_->GetSize() - dataOffset;
    (void)memset_s(cachePacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    cachePacket_->data = data;
    cachePacket_->size = size;
    cachePacket_->stream_index = static_cast<int>(trackIndex);
    cachePacket_->pts = ConvertTimeToFFmpegByUs(sample->pts_, st->time_base);

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
    if (flacCodecConfig_ != nullptr && st->codecpar->codec_id == AV_CODEC_ID_FLAC) {
        flacCodecConfig_->UpdatePerFrame(data, size);
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

} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
