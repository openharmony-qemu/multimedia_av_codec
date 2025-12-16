/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "mpeg4_muxer_plugin.h"
#include <functional>
#include <sys/types.h>
#include <algorithm>
#include <fcntl.h>
#ifndef _WIN32
#include <unistd.h>
#include "securec.h"
#endif // !_WIN32
#include "avcodec_common.h"
#include "avcodec_mime_type.h"
#include "mpeg4_utils.h"
#include "audio_track.h"
#include "video_track.h"
#include "cover_track.h"
#include "meta/mime_type.h"

namespace {
#ifndef _WIN32
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_MUXER, "Mpeg4MuxerPlugin"};
#endif // !_WIN32
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace Plugins;
using namespace Mpeg4;

std::map<std::string, uint32_t> g_supportedMuxer = {
    {"Mpeg4Mux_mp4", OHOS::MediaAVCodec::OUTPUT_FORMAT_MPEG_4},
    {"Mpeg4Mux_m4a", OHOS::MediaAVCodec::OUTPUT_FORMAT_M4A}
};

Status RegisterMuxerPlugins(const std::shared_ptr<Register>& reg)
{
    for (auto &muxer : g_supportedMuxer) {
        MuxerPluginDef def;
        def.name = muxer.first;
        def.description = "mpeg4 muxer";
        def.rank = 60; // 60  T:100
        def.SetCreator([](const std::string& name) -> std::shared_ptr<MuxerPlugin> {
            return std::make_shared<Mpeg4MuxerPlugin>(name);
        });
        if (reg->AddPlugin(def) != Status::NO_ERROR) {
            MEDIA_LOG_W("fail to add plugin " PUBLIC_LOG_S, muxer.first.c_str());
            continue;
        }
    }
    return Status::NO_ERROR;
}

PLUGIN_DEFINITION(Mpeg4Muxer, LicenseType::APACHE_V2, RegisterMuxerPlugins, [] {})
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
Mpeg4MuxerPlugin::Mpeg4MuxerPlugin(std::string name)
    : MuxerPlugin(std::move(name))
{
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
    outputFormat_ = g_supportedMuxer[pluginName_];
    boxParser_ = std::make_shared<BoxParser>(outputFormat_);
    moov_ = boxParser_->MoovBoxGenerate();
}

Mpeg4MuxerPlugin::~Mpeg4MuxerPlugin()
{
    MEDIA_LOG_D("Destory");
    io_ = nullptr;
    moov_ = nullptr;
    boxParser_ = nullptr;
    tracks_.clear();
    MEDIA_LOG_D("0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

Status Mpeg4MuxerPlugin::SetDataSink(const std::shared_ptr<DataSink> &dataSink)
{
    FALSE_RETURN_V_MSG_E(dataSink != nullptr, Status::ERROR_INVALID_PARAMETER, "data sink is null");
    io_ = std::make_shared<AVIOStream>(dataSink);
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::SetParameter(const std::shared_ptr<Meta> &param)
{
    Status ret = Status::NO_ERROR;
    ret = SetRotation(param);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "SetRotation failed");
    return ret;
}

Status Mpeg4MuxerPlugin::SetRotation(const std::shared_ptr<Meta> &param)
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

Status Mpeg4MuxerPlugin::AddTrack(int32_t &trackIndex, const std::shared_ptr<Meta> &trackDesc)
{
    FALSE_RETURN_V_MSG_E(boxParser_ != nullptr, Status::ERROR_INVALID_OPERATION, "box parser is not set");
    constexpr int32_t mimeTypeLen = 5;  // 5
    Status ret = Status::NO_ERROR;
    std::string mimeType = "";
    std::shared_ptr<BasicTrack> track;
    FALSE_RETURN_V_MSG_E(trackDesc->Get<Tag::MIME_TYPE>(mimeType), Status::ERROR_INVALID_PARAMETER,
        "get mimeType failed!"); // mime
    MEDIA_LOG_D("mimeType is %{public}s", mimeType.c_str());
    if (!mimeType.compare(0, mimeTypeLen, "audio")) {
        track = std::make_shared<AudioTrack>(mimeType, moov_);
    } else if (!mimeType.compare(0, mimeTypeLen, "video")) {
        track = std::make_shared<VideoTrack>(mimeType, moov_);
    } else if (!mimeType.compare(0, mimeTypeLen, "image")) {
        track = std::make_shared<CoverTrack>(mimeType, moov_);
    } else {
        MEDIA_LOG_W("mimeType %{public}s is unsupported", mimeType.c_str());
        return Status::ERROR_UNSUPPORTED_FORMAT;
    }
    ret = track->Init(trackDesc);
    FALSE_RETURN_V_MSG_E(ret == Status::NO_ERROR, ret, "add track failed!");
    if (track->GetMediaType() == OHOS::MediaAVCodec::MEDIA_TYPE_AUD ||
        track->GetMediaType() == OHOS::MediaAVCodec::MEDIA_TYPE_VID) {
        boxParser_->AddTrakBox(track);
        FALSE_RETURN_V_MSG_E(track->GetTrackId() > 0, Status::ERROR_INVALID_DATA, "track id must be > 0");
    }
    if (mimeType.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC) == 0) {
        isH264_ = true;
    }
    trackIndex = static_cast<int32_t>(tracks_.size());
    tracks_.emplace_back(track);
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::Start()
{
    FALSE_RETURN_V_MSG_E(io_ != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    return WriteHeader();
}

Status Mpeg4MuxerPlugin::WriteSample(uint32_t trackIndex, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(io_ != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "sample is null!");
    FALSE_RETURN_V_MSG_E(trackIndex < tracks_.size(), Status::ERROR_INVALID_PARAMETER, "track index is invalid!");
    return tracks_[trackIndex]->WriteSample(io_, sample);
}

Status Mpeg4MuxerPlugin::Stop()
{
    FALSE_RETURN_V_MSG_E(io_ != nullptr, Status::ERROR_INVALID_OPERATION, "data sink is not set");
    FALSE_RETURN_V_MSG_E(moov_ != nullptr, Status::ERROR_INVALID_OPERATION, "moov is not set");
    return WriteTailer();
}

Status Mpeg4MuxerPlugin::WriteHeader()
{
    FALSE_RETURN_V_MSG_E(boxParser_ != nullptr, Status::ERROR_INVALID_OPERATION, "box parser is not set");
    constexpr int32_t typeLen = 4;
    boxParser_->AddUdtaBox();
    for (auto &track : tracks_) {
        track->SetRotation(rotation_);
    }
    io_->Seek(0);
    io_->Write(static_cast<uint32_t>(0));
    io_->Write("ftyp", typeLen);
    // T:更新ftyp, 用新标
    if (outputFormat_ == OUTPUT_FORMAT_M4A) {
        io_->Write("M4A ", typeLen);
    } else {
        io_->Write("mp42", typeLen);
    }
    io_->Write(static_cast<uint32_t>(0));
    if (outputFormat_ == OUTPUT_FORMAT_M4A) {
        io_->Write("M4A ", typeLen);
        io_->Write("isom ", typeLen);
        io_->Write("iso2 ", typeLen);
    } else {
        io_->Write("iso2", typeLen);
        if (isH264_) {
            io_->Write("avc1", typeLen);
        }
        io_->Write("mp42 ", typeLen);
    }
    freePos_ = io_->GetPos();
    io_->Seek(0);
    io_->Write(static_cast<uint32_t>(freePos_));
    io_->Seek(freePos_);
    io_->Write(static_cast<uint32_t>(8));  // 8
    io_->Write("free", typeLen);
    mdatPos_ = io_->GetPos();
    io_->Write(static_cast<uint32_t>(0));
    io_->Write("mdat", typeLen);
    return Status::NO_ERROR;
}

Status Mpeg4MuxerPlugin::WriteTailer()
{
    bool isNeedFree = false;
    int64_t pos = io_->GetPos();
    uint64_t mdatSize = pos - mdatPos_;
    if (mdatSize <= UINT32_MAX) {
        io_->Seek(mdatPos_);
        io_->Write(static_cast<uint32_t>(mdatSize));
    } else {
        isNeedFree = true;
        io_->Seek(freePos_);
        io_->Write(static_cast<uint32_t>(1));
        io_->Write("mdat", 4);  // 4
        io_->Write(mdatSize + 8);  // 8
    }
    io_->Seek(pos);
    for (auto &track : tracks_) {
        track->WriteTailer();
    }
    return MoveMoovBoxToFront(isNeedFree);  // T:默认不前置
}

Status Mpeg4MuxerPlugin::MoveMoovBoxToFront(bool isNeedFree)
{
    uint32_t moovSize = moov_->GetSize();
    uint32_t freeSize = isNeedFree ? 8 : 0; // 需要补齐free box 8
    moovSize += freeSize;
    std::sort(tracks_.begin(), tracks_.end(), BasicTrack::Compare);
    for (auto &track : tracks_) {
        uint32_t  increments = track->MoovIncrements(moovSize);
        if (increments == 0) {
            break;
        }
        moovSize += increments;
    }
    uint32_t offset = moov_->GetSize() + freeSize;
    MEDIA_LOG_I("check moov size %{public}d - %{public}d", moovSize, offset);
    for (auto& track : tracks_) {
        track->SetOffset(offset);
    }
    auto io = std::make_shared<AVIOStream>(*io_.get());
    io->Seek(freePos_);
    io_->Seek(freePos_);

    std::vector<std::pair<uint8_t*, int32_t>> buffer = {{nullptr, 0}, {nullptr, 0}};
    buffer[0].first = new uint8_t[offset];
    buffer[0].second = io->Read(buffer[0].first, offset);
    moov_->Write(io_);
    if (isNeedFree) {
        io_->Write(static_cast<uint32_t>(8));  // 8
        io_->Write("free", 4);  // 4
    }
    tracks_.clear();
    moov_ = nullptr;
    boxParser_ = nullptr;
    buffer[1].first = new uint8_t[offset];

    int32_t i = 0;
    while (buffer[i].second > 0) {
        buffer[(i + 1) % 2].second = io->Read(buffer[(i + 1) % 2].first, offset);  // 2
        io_->Write(buffer[i].first, buffer[i].second);
        i = (i + 1) % 2;  // 2
    }
    delete[] buffer[0].first;
    delete[] buffer[1].first;
    return Status::NO_ERROR;
}
} // Mpeg4
} // namespace Plugins
} // Media
} // OHOS
