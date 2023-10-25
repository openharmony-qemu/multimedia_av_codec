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
#include "ffmpeg_muxer_converter.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN, "FfmpegMuxerPlugin" };
}

namespace {
using namespace OHOS::Media;
using namespace Plugin;
using namespace Ffmpeg;

std::map<std::string, std::shared_ptr<AVOutputFormat>> g_pluginOutputFmt;

std::map<std::string, OutputFormat> g_supportedMuxer = {
    {"mp4", OutputFormat::MPEG_4}, 
    {"ipod", OutputFormat::M4A}
};

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
    if (it != g_supportedMuxer.end() && it->second == static_cast<OutputFormat>(outputFormat)) {
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
namespace Media {
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
}

FFmpegMuxerPlugin::~FFmpegMuxerPlugin()
{
    AVCODEC_LOGD("Destory");
    outputFormat_.reset();
    cachePacket_.reset();
    formatContext_.reset();
    codecConfigs_.clear();
    AVCODEC_LOGD("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
    mallopt(M_FLUSH_THREAD_CACHE, 0);
}

Status FFmpegMuxerPlugin::SetDataSink(const std::shared_ptr<DataSink> &dataSink)
{
    CHECK_AND_RETURN_RET_LOG(dataSink != nullptr, Status::ERROR_INVALID_PARAMETER, "data sink is null");
    DeInitAvIoCtx(formatContext_->pb);
    formatContext_->pb = InitAvIoCtx(dataSink, 1);
    CHECK_AND_RETURN_RET_LOG(formatContext_->pb != nullptr, Status::ERROR_INVALID_OPERATION, "failed to create io");
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetParameter(const std::shared_ptr<Meta> &param)
{
    if (param->Find(Tag::VIDEO_ROTATION) != param->end()) {
        param->Get<Tag::VIDEO_ROTATION>(rotation_); // rotation
        if (rotation_ != VIDEO_ROTATION_0 && rotation_ != VIDEO_ROTATION_90 &&
            rotation_ != VIDEO_ROTATION_180 && rotation_ != VIDEO_ROTATION_270) {
            AVCODEC_LOGW("Invalid rotation: %{public}d, keep default 0", rotation_);
            rotation_ = VIDEO_ROTATION_0;
        }
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterOfTrack(AVStream *stream, const std::shared_ptr<Meta> &trackDesc)
{
    uint8_t *extraData = nullptr;
    size_t extraDataSize = 0;

    AVCodecParameters *par = stream->codecpar;
    if (trackDesc->Find(Tag::MEDIA_BITRATE) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_BITRATE>(par->bit_rate); // bit rate
    }
    std::vector<uint8_t> codecConfig;
    if (trackDesc->Find(Tag::MEDIA_CODEC_CONFIG) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig); // codec config
    }
    codecConfigs_.insert(
        { stream->index, { true, codecConfig } }
        );
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterColor(AVStream* stream, const std::shared_ptr<Meta> &trackDesc)
{
    AVCodecParameters* par = stream->codecpar;
    if (trackDesc->Find(Tag::VIDEO_COLOR_PRIMARIES) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_TRC) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_MATRIX_COEFF) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_RANGE) != trackDesc->end()) {
        ColorPrimary colorPrimaries = ColorPrimary::COLOR_PRIMARY_UNSPECIFIED;
        TransferCharacteristic colorTransfer = TransferCharacteristic::TRANSFER_CHARACTERISTIC_UNSPECIFIED;
        MatrixCoefficient colorMatrixCoeff = MatrixCoefficient::MATRIX_COEFFICIENT_UNSPECIFIED;
        bool colorRange = false;
        trackDesc->Get<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries);
        trackDesc->Get<Tag::VIDEO_COLOR_TRC>(colorTransfer);
        trackDesc->Get<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrixCoeff);
        trackDesc->Get<Tag::VIDEO_COLOR_RANGE>(colorRange);
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
        par->color_range = !colorRange ? AVCOL_RANGE_UNSPECIFIED : AVCOL_RANGE_MPEG;
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::SetCodecParameterCuva(AVStream* stream, const std::shared_ptr<Meta> &trackDesc)
{
    if (trackDesc->Find(Tag::VIDEO_HDR_TYPE) != trackDesc->end()) {
        VideoHdrType hdrType;
        trackDesc->Get<Tag::VIDEO_HDR_TYPE>(hdrType);
        AVCODEC_LOGD("hdr type: %{public}d", hdrType);

        CHECK_AND_RETURN_RET_LOG(hdrType == VideoHdrType::HDR_VIVID, Status::ERROR_INVALID_PARAMETER,
            "this hdr type do not support! hdr type: %{public}d", hdrType);
        if (hdrType == VideoHdrType::HDR_VIVID) {
            av_dict_set(&stream->metadata, "hdr_type", "hdr_vivid", 0);
        }
    }
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::AddAudioTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc, AVCodecID codeID)
{
    uint32_t sampleRate = 0;
    uint32_t channels = 0;
    bool ret = trackDesc->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate); // sample rate
    CHECK_AND_RETURN_RET_LOG(ret, Status::ERROR_INVALID_PARAMETER, "get audio sample_rate failed!");
    ret = trackDesc->Get<Tag::AUDIO_CHANNEL_COUNT>(channels); // channels
    CHECK_AND_RETURN_RET_LOG(ret, Status::ERROR_INVALID_PARAMETER, "get audio channels failed!");

    auto st = avformat_new_stream(formatContext_.get(), nullptr);
    CHECK_AND_RETURN_RET_LOG(st != nullptr, Status::ERROR_NO_MEMORY, "avformat_new_stream failed!");
    ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_AUDIO;
    st->codecpar->codec_id = codeID;
    st->codecpar->sample_rate = sampleRate;
    st->codecpar->channels = channels;
    uint32_t frameSize = 0;
    if (trackDesc->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != trackDesc->end()) {
        trackDesc->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize); // frame size
        st->codecpar->frame_size = frameSize;
    }
    trackIndex = st->index;
    return SetCodecParameterOfTrack(st, trackDesc);
}

Status FFmpegMuxerPlugin::AddVideoTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc,
                                        AVCodecID codeID, bool isCover)
{
    constexpr uint32_t maxLength = 65535;
    constexpr uint32_t maxVideoDelay = 16;
    uint32_t width = 0;
    uint32_t height = 0;
    bool ret = trackDesc->Get<Tag::VIDEO_WIDTH>(width); // width
    CHECK_AND_RETURN_RET_LOG((ret && width <= maxLength), Status::ERROR_INVALID_PARAMETER,
        "get video width failed! width:%{public}d", width);
    ret =  trackDesc->Get<Tag::VIDEO_HEIGHT>(height); // height
    CHECK_AND_RETURN_RET_LOG((ret && height <= maxLength), Status::ERROR_INVALID_PARAMETER,
        "get video height failed! height:%{public}d", height);

    auto st = avformat_new_stream(formatContext_.get(), nullptr);
    CHECK_AND_RETURN_RET_LOG(st != nullptr, Status::ERROR_NO_MEMORY, "avformat_new_stream failed!");
    ResetCodecParameter(st->codecpar);
    st->codecpar->codec_type = AVMEDIA_TYPE_VIDEO;
    st->codecpar->codec_id = codeID;
    st->codecpar->width = width;
    st->codecpar->height = height;
    uint32_t videoDelay = 0;
    if (trackDesc->Find(Tag::MD_KEY_VIDEO_DELAY) != trackDesc->end()) {
        trackDesc->Get<Tag::MD_KEY_VIDEO_DELAY>(videoDelay); // video delay
        st->codecpar->video_delay = videoDelay;
    }

    trackIndex = st->index;
    if (isCover) {
        st->disposition = AV_DISPOSITION_ATTACHED_PIC;
    }
    uint32_t frameRate = 0;
    if (trackDesc->Find(Tag::VIDEO_FRAME_RATE) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_FRAME_RATE>(frameRate); // video frame rate
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

Status FFmpegMuxerPlugin::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    CHECK_AND_RETURN_RET_LOG(!isWriteHeader_, Status::ERROR_WRONG_STATE, "AddTrack failed! muxer has start!");
    CHECK_AND_RETURN_RET_LOG(outputFormat_ != nullptr, Status::ERROR_NULL_POINTER, "AVOutputFormat is nullptr");
    constexpr int32_t mimeTypeLen = 5;
    Status ret = Status::NO_ERROR;
    std::string mimeType = {};
    AVCodecID codeID = AV_CODEC_ID_NONE;
    CHECK_AND_RETURN_RET_LOG(trackDesc->Get<Tag::CODEC_MIME_TYPE>(mimeType),
        Status::ERROR_INVALID_PARAMETER, "get mimeType failed!"); // mime
    AVCODEC_LOGD("mimeType is %{public}s", mimeType.c_str());
    CHECK_AND_RETURN_RET_LOG(Mime2CodecId(mimeType, codeID), Status::ERROR_INVALID_DATA,
        "this mimeType do not support! mimeType:%{public}s", mimeType.c_str());
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
    if (rotation_ != VIDEO_ROTATION_0) {
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

Status FFmpegMuxerPlugin::Reset()
{
    return Status::NO_ERROR;
}

Status FFmpegMuxerPlugin::WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    CHECK_AND_RETURN_RET_LOG(isWriteHeader_, Status::ERROR_WRONG_STATE, "WriteSample failed! Did not write header!");
    CHECK_AND_RETURN_RET_LOG(sample != nullptr && sample->memory_ != nullptr, Status::ERROR_NULL_POINTER,
        "av_write_frame sample is null!");
    CHECK_AND_RETURN_RET_LOG(trackIndex < formatContext_->nb_streams &&
        codecConfigs_.find(trackIndex) != codecConfigs_.end(), Status::ERROR_INVALID_PARAMETER,
        "track index is invalid!");
    std::lock_guard<std::mutex> lock(mutex_);
    (void)memset_s(cachePacket_.get(), sizeof(AVPacket), 0, sizeof(AVPacket));
    auto st = formatContext_->streams[trackIndex];
    cachePacket_->data = sample->memory_->GetAddr();
    cachePacket_->size = sample->memory_->GetSize();
    cachePacket_->stream_index = static_cast<int>(trackIndex);
    cachePacket_->pts = ConvertTimeToFFmpeg(sample->pts_, st->time_base);
    if (st->codecpar->codec_type == AVMEDIA_TYPE_AUDIO) {
        cachePacket_->dts = cachePacket_->pts;
    } else {
        cachePacket_->dts = AV_NOPTS_VALUE;
    }
    cachePacket_->flags = 0;
    if (sample->flag_ & AVBufferFlag::SYNC_FRAME) {
        AVCODEC_LOGD("It is key frame");
        cachePacket_->flags = AV_PKT_FLAG_KEY;
    }
    if (codecConfigs_[trackIndex].first) {
        auto ret = SetCodecConfigToCodecPar(trackIndex,
            !IsAvccSample(sample->memory_->GetAddr(), sample->memory_->GetSize()));
        CHECK_AND_RETURN_RET_LOG(ret == Status::NO_ERROR, ret, "set codec config failed!");
    }
    auto ret = av_write_frame(formatContext_.get(), cachePacket_.get());
    av_packet_unref(cachePacket_.get());
    if (ret < 0) {
        AVCODEC_LOGE("write sample buffer failed, %{public}s", AVStrError(ret).c_str());
        return Status::ERROR_UNKNOWN;
    }
    return Status::NO_ERROR;
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

Status FFmpegMuxerPlugin::SetCodecConfigToCodecPar(uint32_t trackIndex, bool isAnnexB)
{
    if (codecConfigs_.find(trackIndex) == codecConfigs_.end() || !codecConfigs_[trackIndex].first) {
        return Status::NO_ERROR;
    }

    codecConfigs_[trackIndex].first = false;
    auto size = static_cast<int32_t>(codecConfigs_[trackIndex].second.size());
    auto data = codecConfigs_[trackIndex].second.data();
    auto st = formatContext_->streams[trackIndex];
    auto par = st->codecpar;

    if ((par->codec_type == AVMEDIA_TYPE_AUDIO && size == 0) ||
        (par->codec_type == AVMEDIA_TYPE_VIDEO && (st->disposition == AV_DISPOSITION_ATTACHED_PIC || isAnnexB))) {
        return Status::NO_ERROR;
    }
    if (par->codec_type == AVMEDIA_TYPE_VIDEO && !isAnnexB) {
        CHECK_AND_RETURN_RET_LOG(size > 0, Status::ERROR_INVALID_DATA,
            "the video stream is not annex-b, so the video format need codec config!");
    }
    par->extradata = static_cast<uint8_t*>(av_mallocz(size + AV_INPUT_BUFFER_PADDING_SIZE));
    CHECK_AND_RETURN_RET_LOG(par->extradata != nullptr, Status::ERROR_NO_MEMORY, "codec config malloc failed!");
    par->extradata_size = static_cast<int32_t>(size);
    errno_t rc = memcpy_s(par->extradata, par->extradata_size, data, size);
    CHECK_AND_RETURN_RET_LOG(rc == EOK, Status::ERROR_UNKNOWN, "memcpy_s failed");
    return Status::NO_ERROR;
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
} // namespace Ffmpeg
} // namespace Plugin
} // namespace Media
} // namespace OHOS
