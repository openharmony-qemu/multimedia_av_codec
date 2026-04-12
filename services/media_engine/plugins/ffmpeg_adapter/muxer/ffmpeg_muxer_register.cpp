/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "FfmpegMuxerRegister"

#include "ffmpeg_muxer_register.h"
#include "common/log.h"
#include "ffmpeg_utils.h"
#include "ffmpeg_converter.h"
#include "meta/mime_type.h"
#include "ffmpeg_muxer_plugin.h"
#include "ffmpeg_flv_muxer_plugin.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_MUXER, "FFmpegMuxerRegister"};

using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace Ffmpeg;

PLUGIN_DEFINITION(FFmpegMuxer, LicenseType::LGPL,
    FFmpegMuxerRegister::RegisterMuxerPlugins, FFmpegMuxerRegister::UnregisterMuxerPlugins)
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
std::map<std::string, std::shared_ptr<AVOutputFormat>> FFmpegMuxerRegister::pluginOutputFmt_;
std::set<std::string> FFmpegMuxerRegister::supportedMuxer_ = {
    "mp4", "ipod", "amr", "mp3", "wav", "adts", "flac", "ogg", "flv"
};

bool FFmpegMuxerRegister::IsMuxerSupported(const char* name)
{
    return supportedMuxer_.count(name) == 1;
}

bool FFmpegMuxerRegister::CodecId2Cap(AVCodecID codecId, bool encoder, Capability &cap)
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
        case AV_CODEC_ID_PCM_S16LE:
            cap.SetMime(MimeType::AUDIO_RAW);
            return true;
        case AV_CODEC_ID_FLAC:
            cap.SetMime(MimeType::AUDIO_FLAC);
            return true;
        case AV_CODEC_ID_OPUS:
            cap.SetMime(MimeType::AUDIO_OPUS);
            return true;
        case AV_CODEC_ID_VORBIS:
            cap.SetMime(MimeType::AUDIO_VORBIS);
            return true;
        default:
            break;
    }
    return false;
}

bool FFmpegMuxerRegister::FormatName2OutCapability(const std::string &fmtName, MuxerPluginDef &pluginDef)
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
    } else if (fmtName == "wav") {
        auto cap = Capability(MimeType::MEDIA_WAV);
        pluginDef.AddOutCaps(cap);
        return true;
    } else if (fmtName == "adts") {
        auto cap = Capability(MimeType::MEDIA_AAC);
        pluginDef.AddOutCaps(cap);
        return true;
    } else if (fmtName == "flac") {
        auto cap = Capability(MimeType::MEDIA_FLAC);
        pluginDef.AddOutCaps(cap);
        return true;
    } else if (fmtName == "ogg") {
        auto cap = Capability(MimeType::MEDIA_OGG);
        pluginDef.AddOutCaps(cap);
        return true;
    } else if (fmtName == "flv") {
        auto cap = Capability(MimeType::MEDIA_FLV);
        pluginDef.AddOutCaps(cap);
        return true;
    }
    return false;
}

bool FFmpegMuxerRegister::UpdatePluginInCapability(AVCodecID codecId, MuxerPluginDef &pluginDef)
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

bool FFmpegMuxerRegister::UpdatePluginCapability(const AVOutputFormat* oFmt, MuxerPluginDef &pluginDef)
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

Status FFmpegMuxerRegister::RegisterMuxerPlugins(const std::shared_ptr<Register> &reg)
{
    const AVOutputFormat* outputFormat = nullptr;
    void* ite = nullptr;
    while ((outputFormat = av_muxer_iterate(&ite))) {
        if (!IsMuxerSupported(outputFormat->name)) {
            continue;
        }
        if (outputFormat->long_name != nullptr && strncmp(outputFormat->name, "flac ", 4)) {  // 4 not flac
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
        def.SetCreator([](const std::string &name) -> std::shared_ptr<MuxerPlugin> {
            if (name == "ffmpegMux_flv") {
                return std::make_shared<FFmpegFlvMuxerPlugin>(name);
            }
            return std::make_shared<FFmpegMuxerPlugin>(name);
        });
        if (reg->AddPlugin(def) != Status::OK) {
            MEDIA_LOG_W("fail to add plugin " PUBLIC_LOG_S, pluginName.c_str());
            continue;
        }
        pluginOutputFmt_[pluginName] = std::shared_ptr<AVOutputFormat>(
            const_cast<AVOutputFormat*>(outputFormat), [](AVOutputFormat* ptr) {}); // do not delete
    }
    return Status::OK;
}

void FFmpegMuxerRegister::UnregisterMuxerPlugins()
{
    pluginOutputFmt_.clear();
}

void FFmpegMuxerRegister::FfmpegLogPrintWithLevel(void* avcl, int level, const char* fmt, va_list vl)
{
    (void)avcl;
    char buf[500] = {0}; // 500
    int ret = vsnprintf_s(buf, sizeof(buf), sizeof(buf), fmt, vl);
    if (ret < 0) {
        return;
    }
    switch (level) {
        case AV_LOG_WARNING:
            MEDIA_LOG_W("[FFmpeg Log " PUBLIC_LOG_D32 " WARN] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_ERROR:
            MEDIA_LOG_E("[FFmpeg Log " PUBLIC_LOG_D32 " ERROR] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_FATAL:
            MEDIA_LOG_F("[FFmpeg Log " PUBLIC_LOG_D32 " FATAL] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_INFO:
            MEDIA_LOG_I("[FFmpeg Log " PUBLIC_LOG_D32 " INFO] " PUBLIC_LOG_S, level, buf);
            break;
        case AV_LOG_DEBUG:
            MEDIA_LOG_D("[FFmpeg Log " PUBLIC_LOG_D32 " DEBUG] " PUBLIC_LOG_S, level, buf);
            break;
        default:
            break;
    }
}

void FFmpegMuxerRegister::ResetCodecParameter(AVCodecParameters* par)
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

std::shared_ptr<AVOutputFormat> FFmpegMuxerRegister::GetAVOutputFormat(std::string pluginName)
{
    return pluginOutputFmt_[pluginName];
}

AVIOContext* FFmpegMuxerRegister::InitAvIoCtx(const std::shared_ptr<DataSink> &dataSink, int writeFlags)
{
    if (dataSink == nullptr) {
        return nullptr;
    }
    IOContext* ioContext = new IOContext();
    ioContext->dataSink_ = dataSink;
    ioContext->pos_ = 0;
    ioContext->end_ = dataSink->Seek(0ll, SEEK_END);

    constexpr int bufferSize = 4 * 1024; // 4096
    auto buffer = static_cast<unsigned char*>(av_malloc(bufferSize));
    AVIOContext* avioContext = avio_alloc_context(buffer, bufferSize, writeFlags, static_cast<void*>(ioContext),
                                                  IoRead, IoWrite, IoSeek);
    if (avioContext == nullptr) {
        delete ioContext;
        av_free(buffer);
        return nullptr;
    }
    avioContext->seekable = AVIO_SEEKABLE_NORMAL;
    return avioContext;
}

void FFmpegMuxerRegister::DeInitAvIoCtx(AVIOContext* ptr)
{
    if (ptr != nullptr) {
        delete static_cast<IOContext*>(ptr->opaque);
        ptr->opaque = nullptr;
        av_freep(&ptr->buffer);
        av_opt_free(ptr);
        avio_context_free(&ptr);
    }
}

int32_t FFmpegMuxerRegister::IoRead(void* opaque, uint8_t* buf, int bufSize)
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

int32_t FFmpegMuxerRegister::IoWrite(void* opaque, const uint8_t* buf, int bufSize)
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

int64_t FFmpegMuxerRegister::IoSeek(void* opaque, int64_t offset, int whence)
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

int32_t FFmpegMuxerRegister::IoOpen(
    AVFormatContext* s, AVIOContext** pb, const char* url, int flags, AVDictionary** options)
{
    MEDIA_LOG_D("IoOpen flags %{public}d", flags);
    *pb = InitAvIoCtx(static_cast<IOContext*>(s->pb->opaque)->dataSink_, 0);
    if (*pb == nullptr) {
        MEDIA_LOG_E("IoOpen failed");
        return -1;
    }
    return 0;
}

int FFmpegMuxerRegister::IoClose(AVFormatContext* s, AVIOContext* pb)
{
    avio_flush(pb);
    DeInitAvIoCtx(pb);
    return 0;
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS
