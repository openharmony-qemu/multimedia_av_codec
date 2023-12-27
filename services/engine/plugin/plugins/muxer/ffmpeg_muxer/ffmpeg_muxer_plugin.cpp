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

#include "ffmpeg_muxer_plugin.h"
#include <malloc.h>
#include "securec.h"
#include "ffmpeg_utils.h"
#include "avcodec_log.h"
#include "avcodec_common.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN, "FfmpegMuxerPlugin" };
constexpr uint8_t START_CODE[] = {0x00, 0x00, 0x01};
}

namespace {
using namespace OHOS::MediaAVCodec;
using namespace Plugin;
using namespace Ffmpeg;

std::map<std::string, std::shared_ptr<AVOutputFormat>> g_pluginOutputFmt;

std::map<std::string, uint32_t> g_supportedMuxer = {{"mp4", OUTPUT_FORMAT_MPEG_4}, {"ipod", OUTPUT_FORMAT_M4A}};

bool IsMuxerSupported(const char *name)
{
    auto it = g_supportedMuxer.find(name);
    if (it != g_supportedMuxer.end()) {
        return true;
    }
    return false;
}

int32_t Sniff(const std::string& pluginName, uint32_t outputFormat)
{
    constexpr int32_t ffmpegConfidence = 60;
    if (pluginName.empty()) {
        return 0;
    }
    auto plugin = g_pluginOutputFmt[pluginName];
    int32_t confidence = 0;
    auto it = g_supportedMuxer.find(plugin->name);
    if (it != g_supportedMuxer.end() && it->second == outputFormat) {
        confidence = ffmpegConfidence;
    }

    return confidence;
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
        def.name = pluginName;
        def.description = "ffmpeg muxer";
        def.rank = 100; // 100
        def.creator = [](const std::string& name) -> std::shared_ptr<MuxerPlugin> {
            return std::make_shared<FFmpegMuxerPlugin>(name);
        };
        def.sniffer = Sniff;
        if (reg->AddPlugin(def) != Status::NO_ERROR) {
            continue;
        }
        g_pluginOutputFmt[pluginName] = std::shared_ptr<AVOutputFormat>(
            const_cast<AVOutputFormat*>(outputFormat), [](AVOutputFormat *ptr) {}); // do not delete
    }
    return Status::NO_ERROR;
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
namespace MediaAVCodec {
namespace Plugin {
namespace Ffmpeg {
FFmpegMuxerPlugin::FFmpegMuxerPlugin(std::string name)
    : MuxerPlugin(std::move(name)), isWriteHeader_(false)
{
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    mallopt(M_SET_THREAD_CACHE, M_THREAD_CACHE_DISABLE);
    mallopt(M_DELAYED_FREE, M_DELAYED_FREE_DISABLE);
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

    hevcParser_ = HevcParserManager::Create();
}

FFmpegMuxerPlugin::~FFmpegMuxerPlugin()
{
    AVCODEC_LOGD("Destory");
    outputFormat_.reset();
    cachePacket_.reset();
    formatContext_.reset();
    videoTracksInfo_.clear();
    hevcParser_.reset();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    mallopt(M_FLUSH_THREAD_CACHE, 0);
}

Status FFmpegMuxerPlugin::SetDataSink(const std::shared_ptr<DataSink>& dataSink)
{
    CHECK_AND_RETURN_RET_LOG(dataSink != nullptr, Status::ERROR_INVALID_PARAMETER, "data sink is null");
    DeInitAvIoCtx(formatContext_->pb);
    formatContext_->pb = InitAvIoCtx(dataSink, 1);
    CHECK_AND_RETURN_RET_LOG(formatContext_->pb != nullptr, Status::ERROR_INVALID_OPERATION, "failed to create io");
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetRotation(int32_t rotation)
{
    rotation_ = rotation;
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterOfTrack(AVStream *stream, const MediaDescription &trackDesc)
{
    uint8_t *extraData = nullptr;
    size_t extraDataSize = 0;

    AVCodecParameters *par = stream->codecpar;
    if (trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_BITRATE)) {
        trackDesc.GetLongValue(MediaDescriptionKey::MD_KEY_BITRATE, par->bit_rate); // bit rate
    }

    VideoSampleInfo sampleInfo;
    videoTracksInfo_[stream->index] = sampleInfo;
    if (trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_CODEC_CONFIG) &&
        trackDesc.GetBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, &extraData, extraDataSize) &&
        extraDataSize > 0) { // codec config
        if (par->codec_id == AV_CODEC_ID_H264 || par->codec_id == AV_CODEC_ID_HEVC) {
            videoTracksInfo_[stream->index].extraData_.reserve(extraDataSize);
            videoTracksInfo_[stream->index].extraData_.assign(extraData, extraData + extraDataSize);
            return Status::NO_ERROR;
        }
        return SetCodecParameterExtra(stream, extraData, extraDataSize);
    } else if (par->codec_id == AV_CODEC_ID_AAC || par->codec_id == AV_CODEC_ID_MPEG4) {
        AVCODEC_LOGW("missing codec config!");
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterExtra(AVStream *stream, const uint8_t *extraData, int32_t extraDataSize)
{
    CHECK_AND_RETURN_RET_LOG(extraDataSize != 0, Status::ERROR_INVALID_DATA, "codec config size is zero!");

    AVCodecParameters *par = stream->codecpar;
    par->extradata = static_cast<uint8_t *>(av_mallocz(extraDataSize + AV_INPUT_BUFFER_PADDING_SIZE));
    CHECK_AND_RETURN_RET_LOG(par->extradata != nullptr, Status::ERROR_NO_MEMORY, "codec config malloc failed!");
    par->extradata_size = extraDataSize;
    errno_t rc = memcpy_s(par->extradata, par->extradata_size, extraData, extraDataSize);
    CHECK_AND_RETURN_RET_LOG(rc == EOK, Status::ERROR_UNKNOWN, "memcpy_s failed");
    
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterColor(AVStream* stream, const MediaDescription& trackDesc)
{
    AVCodecParameters* par = stream->codecpar;
    if (trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES) ||
        trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS) ||
        trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS) ||
        trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_RANGE_FLAG)) {
        ColorPrimary colorPrimaries = ColorPrimary::COLOR_PRIMARY_UNSPECIFIED;
        TransferCharacteristic colorTransfer = TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED;
        MatrixCoefficient colorMatrixCoeff = MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED;
        int32_t colorRange = 0;
        trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_COLOR_PRIMARIES, *(int*)&colorPrimaries);
        trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_TRANSFER_CHARACTERISTICS, *(int*)&colorTransfer);
        trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_MATRIX_COEFFICIENTS, *(int*)&colorMatrixCoeff);
        trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_RANGE_FLAG, colorRange);
        AVCODEC_LOGD("color info.: primary %{public}d, transfer %{public}d, matrix coeff %{public}d,"
            " range %{public}d,", colorPrimaries, colorTransfer, colorMatrixCoeff, colorRange);

        auto colorPri = ColorPrimary2AVColorPrimaries(colorPrimaries);
        CHECK_AND_RETURN_RET_LOG(colorPri.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color primary %{public}d", colorPrimaries);
        auto colorTrc = ColorTransfer2AVColorTransfer(colorTransfer);
        CHECK_AND_RETURN_RET_LOG(colorTrc.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color transfer %{public}d", colorTransfer);
        auto colorSpe = ColorMatrix2AVColorSpace(colorMatrixCoeff);
        CHECK_AND_RETURN_RET_LOG(colorSpe.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color matrix coeff %{public}d", colorMatrixCoeff);
        par->color_primaries = colorPri.second;
        par->color_trc = colorTrc.second;
        par->color_space = colorSpe.second;
        par->color_range = colorRange == 0 ? AVCOL_RANGE_UNSPECIFIED : AVCOL_RANGE_MPEG;
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
        AVCODEC_LOGD("color info.: primary %{public}d, transfer %{public}d, matrix coeff %{public}d,"
            " range %{public}d,", colorPrimaries, colorTransfer, colorMatrixCoeff, colorRange);
        
        auto colorPri = ColorPrimary2AVColorPrimaries(static_cast<ColorPrimary>(colorPrimaries));
        CHECK_AND_RETURN_RET_LOG(colorPri.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color primary %{public}d", colorPrimaries);
        auto colorTrc = ColorTransfer2AVColorTransfer(static_cast<TransferCharacteristic>(colorTransfer));
        CHECK_AND_RETURN_RET_LOG(colorTrc.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color transfer %{public}d", colorTransfer);
        auto colorSpe = ColorMatrix2AVColorSpace(static_cast<MatrixCoefficient>(colorMatrixCoeff));
        CHECK_AND_RETURN_RET_LOG(colorSpe.first, Status::ERROR_INVALID_PARAMETER,
            "failed to match color matrix coeff %{public}d", colorMatrixCoeff);
        par->color_primaries = colorPri.second;
        par->color_trc = colorTrc.second;
        par->color_space = colorSpe.second;
        par->color_range = colorRange == false ? AVCOL_RANGE_UNSPECIFIED : AVCOL_RANGE_MPEG;
        isColorSet_ = true;
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterCuva(AVStream* stream, const MediaDescription& trackDesc)
{
    if (trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID)) {
        trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_IS_HDR_VIVID, isHdrVivid_);
        AVCODEC_LOGD("hdr vivid: %{public}d", isHdrVivid_);
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
            AVCODEC_LOGD("hdr vivid type by parser");
            av_dict_set(&stream->metadata, "hdr_type", "hdr_vivid", 0);
        }
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::AddAudioTrack(int32_t &trackIndex, const MediaDescription &trackDesc, AVCodecID codeID)
{
    int sampleRate = 0;
    int channels = 0;
    bool ret = trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, sampleRate); // sample rate
    CHECK_AND_RETURN_RET_LOG((ret && sampleRate > 0), Status::ERROR_MISMATCHED_TYPE,
        "get audio sample_rate failed! sampleRate:%{public}d", sampleRate);
    ret = trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, channels); // channels
    CHECK_AND_RETURN_RET_LOG((ret && channels > 0), Status::ERROR_MISMATCHED_TYPE,
        "get audio channels failed! channels:%{public}d", channels);

    auto st = avformat_new_stream(formatContext_.get(), nullptr);
    CHECK_AND_RETURN_RET_LOG(st != nullptr, Status::ERROR_NO_MEMORY, "avformat_new_stream failed!");
    ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codecpar->codec_id = codeID;
    st->codecpar->sample_rate = sampleRate;
    st->codecpar->channels = channels;
    int32_t frameSize = 0;
    if (trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLES_PER_FRAME) &&
        trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_AUDIO_SAMPLES_PER_FRAME, frameSize) &&
        frameSize > 0) {
        st->codecpar->frame_size = frameSize;
    }
    trackIndex = st->index;
    return SetCodecParameterOfTrack(st, trackDesc);
}

Status FFmpegMuxerPlugin::AddVideoTrack(int32_t &trackIndex, const MediaDescription &trackDesc,
                                        AVCodecID codeID, bool isCover)
{
    constexpr int maxLength = 65535;
    constexpr int maxVideoDelay = 16;
    int width = 0;
    int height = 0;
    bool ret = trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_WIDTH, width); // width
    CHECK_AND_RETURN_RET_LOG((ret && width > 0 && width <= maxLength), Status::ERROR_MISMATCHED_TYPE,
        "get video width failed! width:%{public}d", width);
    ret = trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, height); // height
    CHECK_AND_RETURN_RET_LOG((ret && height > 0 && height <= maxLength), Status::ERROR_MISMATCHED_TYPE,
        "get video height failed! height:%{public}d", height);

    auto st = avformat_new_stream(formatContext_.get(), nullptr);
    CHECK_AND_RETURN_RET_LOG(st != nullptr, Status::ERROR_NO_MEMORY, "avformat_new_stream failed!");
    ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codecpar->codec_id = codeID;
    st->codecpar->width = width;
    st->codecpar->height = height;
    int32_t videoDelay = 0;
    if (trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_VIDEO_DELAY) &&
        trackDesc.GetIntValue(MediaDescriptionKey::MD_KEY_VIDEO_DELAY, videoDelay) &&
        videoDelay > 0) {
        st->codecpar->video_delay = videoDelay;
    }

    trackIndex = st->index;
    if (isCover) {
        st->disposition = AV_DISPOSITION_ATTACHED_PIC;
    }
    double frameRate = 0;
    if (trackDesc.ContainKey(MediaDescriptionKey::MD_KEY_FRAME_RATE) &&
        trackDesc.GetDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, frameRate) &&
        frameRate > 0) {
        st->avg_frame_rate = {static_cast<int>(frameRate), 1};
    }
    CHECK_AND_RETURN_RET_LOG((videoDelay > 0 && videoDelay <= maxVideoDelay && frameRate > 0) || videoDelay == 0,
        Status::ERROR_MISMATCHED_TYPE, "If the video delayed, the frame rate is required. "
        "The delay is greater than or equal to 0 and less than or equal to 16.");

    auto retColor = SetCodecParameterColor(st, trackDesc);
    CHECK_AND_RETURN_RET_LOG(retColor == Status::NO_ERROR, retColor, "set color failed!");
    auto retCuva = SetCodecParameterCuva(st, trackDesc);
    CHECK_AND_RETURN_RET_LOG(retCuva == Status::NO_ERROR, retCuva, "set cuva failed!");
    return SetCodecParameterOfTrack(st, trackDesc);
}

Status FFmpegMuxerPlugin::AddTrack(int32_t &trackIndex, const MediaDescription &trackDesc)
{
    CHECK_AND_RETURN_RET_LOG(!isWriteHeader_, Status::ERROR_WRONG_STATE, "AddTrack failed! muxer has start!");
    CHECK_AND_RETURN_RET_LOG(outputFormat_ != nullptr, Status::ERROR_NULL_POINTER, "AVOutputFormat is nullptr");
    constexpr int32_t mimeTypeLen = 5;
    Status ret = Status::NO_ERROR;
    std::string mimeType = {};
    AVCodecID codeID = AV_CODEC_ID_NONE;
    CHECK_AND_RETURN_RET_LOG(trackDesc.GetStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType),
        Status::ERROR_MISMATCHED_TYPE, "get mimeType failed!"); // mime
    AVCODEC_LOGD("mimeType is %{public}s", mimeType.c_str());
    CHECK_AND_RETURN_RET_LOG(Mime2CodecId(mimeType, codeID) && (codeID != AV_CODEC_ID_HEVC || hevcParser_ != nullptr),
        Status::ERROR_INVALID_DATA, "this mimeType do not support! mimeType:%{public}s", mimeType.c_str());
    if (!mimeType.compare(0, mimeTypeLen, "audio")) {
        ret = AddAudioTrack(trackIndex, trackDesc, codeID);
        CHECK_AND_RETURN_RET_LOG(ret == Status::NO_ERROR, ret, "AddAudioTrack failed!");
    } else if (!mimeType.compare(0, mimeTypeLen, "video")) {
        ret = AddVideoTrack(trackIndex, trackDesc, codeID, false);
        CHECK_AND_RETURN_RET_LOG(ret == Status::NO_ERROR, ret, "AddVideoTrack failed!");
    } else if (!mimeType.compare(0, mimeTypeLen, "image")) {
        ret = AddVideoTrack(trackIndex, trackDesc, codeID, true);
        CHECK_AND_RETURN_RET_LOG(ret == Status::NO_ERROR, ret, "AddCoverTrack failed!");
    } else {
        AVCODEC_LOGD("mimeType %{public}s is unsupported", mimeType.c_str());
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    uint32_t flags = static_cast<uint32_t>(formatContext_->flags);
    formatContext_->flags = static_cast<int32_t>(flags | AVFMT_TS_NONSTRICT);
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::Start()
{
    CHECK_AND_RETURN_RET_LOG(formatContext_->pb != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    CHECK_AND_RETURN_RET_LOG(!isWriteHeader_, Status::ERROR_WRONG_STATE, "Start failed! muxer has start!");
    if (rotation_ != 0) {
        std::string rotate = std::to_string(rotation_);
        for (uint32_t i = 0; i < formatContext_->nb_streams; i++) {
            if (formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
                av_dict_set(&formatContext_->streams[i]->metadata, "rotate", rotate.c_str(), 0);
            }
        }
    }
    AVDictionary *options = nullptr;
    av_dict_set(&options, "movflags", "faststart", 0);
    int ret = avformat_write_header(formatContext_.get(), &options);
    if (ret < 0) {
        AVCODEC_LOGE("write header failed, %{public}s", AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }
    isWriteHeader_ = true;
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::Stop()
{
    CHECK_AND_RETURN_RET_LOG(isWriteHeader_, Status::ERROR_WRONG_STATE, "Stop failed! Did not write header!");
    int ret = av_write_frame(formatContext_.get(), nullptr); // flush out cache data
    if (ret < 0) {
        AVCODEC_LOGE("write trailer failed, %{public}s", AVStrError(ret).c_str());
    }
    ret = av_write_trailer(formatContext_.get());
    if (ret != 0) {
        AVCODEC_LOGE("write trailer failed, %{public}s", AVStrError(ret).c_str());
    }
    avio_flush(formatContext_->pb);

    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::WriteSample(uint32_t trackIndex, const uint8_t *sample,
    AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    CHECK_AND_RETURN_RET_LOG(isWriteHeader_, Status::ERROR_WRONG_STATE, "WriteSample failed! Did not write header!");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr, Status::ERROR_NULL_POINTER, "av_write_frame sample is null!");
    CHECK_AND_RETURN_RET_LOG(trackIndex < formatContext_->nb_streams,
        Status::ERROR_INVALID_PARAMETER, "track index is invalid!");

    auto st = formatContext_->streams[trackIndex];
    if (st->codecpar->codec_id == AV_CODEC_ID_H264 || st->codecpar->codec_id == AV_CODEC_ID_HEVC) {
        return WriteVideoSample(trackIndex, sample, info, flag);
    }
    return WriteNormal(trackIndex, sample, info.size, info.presentationTimeUs, flag);
}

Status FFmpegMuxerPlugin::WriteNormal(uint32_t trackIndex, const uint8_t *sample,
    int32_t size, int64_t pts, AVCodecBufferFlag flag)
{
    auto st = formatContext_->streams[trackIndex];
    (void)memset_s(cachePacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    cachePacket_->data = const_cast<uint8_t *>(sample);
    cachePacket_->size = size;
    cachePacket_->stream_index = static_cast<int>(trackIndex);
    cachePacket_->pts = ConvertTimeToFFmpeg(pts, st->time_base);
    
    if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        cachePacket_->dts = cachePacket_->pts;
    } else {
        cachePacket_->dts = AV_NOPTS_VALUE;
    }
    cachePacket_->flags = 0;
    if (flag & AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
        AVCODEC_LOGD("It is key frame");
        cachePacket_->flags = AV_PKT_FLAG_KEY;
    }

    auto ret = av_write_frame(formatContext_.get(), cachePacket_.get());
    av_packet_unref(cachePacket_.get());
    if (ret < 0) {
        AVCODEC_LOGE("write sample buffer failed, %{public}s", AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::WriteVideoSample(uint32_t trackIndex, const uint8_t *sample,
    AVCodecBufferInfo info, AVCodecBufferFlag flag)
{
    auto st = formatContext_->streams[trackIndex];
    if (videoTracksInfo_[trackIndex].isFirstFrame_) {
        videoTracksInfo_[trackIndex].isFirstFrame_ = false;
        if (!IsAvccSample(sample, info.size)) {
            if ((flag & AVCODEC_BUFFER_FLAG_CODEC_DATA) != AVCODEC_BUFFER_FLAG_CODEC_DATA) {
                AVCODEC_LOGE("first frame flag of annex-b stream need AVCODEC_BUFFER_FLAGS_CODEC_DATA!");
                return Status::ERROR_INVALID_DATA;
            }
            if (st->codecpar->codec_id == AV_CODEC_ID_HEVC) {
                uint8_t *extraData = nullptr;
                int32_t extraDataSize = 0;
                videoTracksInfo_[trackIndex].isNeedTransData_ = true;
                hevcParser_->ParseExtraData(sample, info.size, &extraData, &extraDataSize);
                CHECK_AND_RETURN_RET_LOG(extraDataSize != 0, Status::ERROR_INVALID_DATA, "parse codec config failed!");
                Status status = SetCodecParameterExtra(st, extraData, extraDataSize);
                CHECK_AND_RETURN_RET_LOG(status == Status::NO_ERROR, status, "set codec config failed!");
                status = SetCodecParameterColorByParser(st);
                CHECK_AND_RETURN_RET_LOG(status == Status::NO_ERROR, status, "set color failed!");
                status = SetCodecParameterCuvaByParser(st);
                CHECK_AND_RETURN_RET_LOG(status == Status::NO_ERROR, status, "set cuva flag failed!");
            }
            if ((flag & AVCODEC_BUFFER_FLAG_SYNC_FRAME) != AVCODEC_BUFFER_FLAG_SYNC_FRAME &&
                st->codecpar->codec_id == AV_CODEC_ID_H264) {
                return SetCodecParameterExtra(st, sample, info.size);
            } else if ((flag & AVCODEC_BUFFER_FLAG_SYNC_FRAME) != AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
                return Status::NO_ERROR;
            }
        } else {
            if (!videoTracksInfo_[trackIndex].extraData_.empty()) {
                SetCodecParameterExtra(st, videoTracksInfo_[trackIndex].extraData_.data(),
                                       videoTracksInfo_[trackIndex].extraData_.size());
            } else {
                AVCODEC_LOGE("non annexb stream must set codec config!");
                return Status::ERROR_INVALID_DATA;
            }
        }
    }

    if (videoTracksInfo_[trackIndex].isNeedTransData_) {
        std::vector<uint8_t> nonAnnexbData = TransAnnexbToMp4(sample, info.size);
        return WriteNormal(trackIndex, nonAnnexbData.data(), nonAnnexbData.size(), info.presentationTimeUs, flag);
    }
    return WriteNormal(trackIndex, sample, info.size, info.presentationTimeUs, flag);
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

bool FFmpegMuxerPlugin::IsAvccSample(const uint8_t* sample, int32_t size)
{
    constexpr int sizeLen = 4;
    if (size < sizeLen) {
        return false;
    }
    uint64_t naluSize = 0;
    uint64_t curPos = 0;
    uint64_t cmpSize = static_cast<uint64_t>(size);
    while (curPos + sizeLen <= cmpSize) {
        naluSize = 0;
        for (uint64_t i = curPos; i < curPos + sizeLen; i++) {
            naluSize = sample[i] + naluSize * 0x100;
        }
        if (naluSize <= 1) {
            return false;
        }
        curPos += (naluSize + sizeLen);
    }
    return curPos == cmpSize;
}

AVIOContext* FFmpegMuxerPlugin::InitAvIoCtx(std::shared_ptr<DataSink> dataSink, int writeFlags)
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
    uint64_t newPos = 0;
    switch (whence) {
        case SEEK_SET:
            newPos = static_cast<uint64_t>(offset);
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
    AVCODEC_LOGD("IoOpen flags %{public}d", flags);
    *pb = InitAvIoCtx(static_cast<IOContext*>(s->pb->opaque)->dataSink_, 0);
    if (*pb == nullptr) {
        AVCODEC_LOGE("IoOpen failed");
        return -1;
    }
    return 0;
}

void FFmpegMuxerPlugin::IoClose(AVFormatContext *s, AVIOContext *pb)
{
    avio_flush(pb);
    DeInitAvIoCtx(pb);
}
} // Ffmpeg
} // Plugin
} // MediaAVCodec
} // OHOS
