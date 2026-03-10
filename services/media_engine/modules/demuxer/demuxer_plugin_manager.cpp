/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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

#define HST_LOG_TAG "DemuxerPluginManager"

#include "demuxer_plugin_manager.h"

#include <algorithm>
#include <map>
#include <memory>

#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "cpp_ext/type_traits_ext.h"
#include "buffer/avallocator.h"
#include "common/event.h"
#include "common/log.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_manager_v2.h"
#include "plugin/plugin_time.h"
#include "base_stream_demuxer.h"
#include "media_demuxer.h"
#include "scoped_timer.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "DemuxerPluginManager" };
constexpr int32_t INVALID_STREAM_OR_TRACK_ID = -1;
constexpr int WAIT_INITIAL_BUFFERING_END_TIME_MS = 3000;
constexpr int32_t API_VERSION_16 = 16;
constexpr int32_t API_VERSION_18 = 18;
constexpr int64_t SNIFF_WARNING_MS = 200;
constexpr int64_t SEEKTOKEYFRAME_WARNING_MS = 0;
constexpr uint32_t SEEKTOFRAMEBYDTS_TIMEOUT_MS = 200;
}

namespace OHOS {
namespace Media {

DataSourceImpl::DataSourceImpl(const std::shared_ptr<BaseStreamDemuxer>& stream, int32_t streamID)
    : stream_(stream),
    streamID_(streamID)
{
}

bool DataSourceImpl::IsOffsetValid(int64_t offset) const
{
    if (stream_->seekable_ == Plugins::Seekable::SEEKABLE) {
        return stream_->mediaDataSize_ == 0 || offset <= static_cast<int64_t>(stream_->mediaDataSize_);
    }
    return true;
}

Status DataSourceImpl::SetStreamID(int32_t streamID)
{
    streamID_ = streamID;
    return Status::OK;
}

/**
* ReadAt Plugins::DataSource::ReadAt implementation.
* @param offset offset in media stream.
* @param buffer caller allocate real buffer.
* @param expectedLen buffer size wanted to read.
* @return read result.
*/
Status DataSourceImpl::ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen)
{
    MediaAVCodec::AVCodecTrace trace("DataSourceImpl::ReadAt");
    std::unique_lock<std::mutex> lock(readMutex_);
    if (!buffer || !IsOffsetValid(offset)) {
        MEDIA_LOG_E("ReadAt failed, buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_D32
                            ", offset: " PUBLIC_LOG_D64, !buffer, static_cast<int>(expectedLen), offset);
        return Status::ERROR_UNKNOWN;
    }
    return stream_->CallbackReadAt(streamID_, offset, buffer, expectedLen);
}

Status DataSourceImpl::GetSize(uint64_t& size)
{
    FALSE_RETURN_V(stream_ != nullptr, Status::ERROR_WRONG_STATE);
    size = stream_->mediaDataSize_;
    return (size > 0) ? Status::OK : Status::ERROR_WRONG_STATE;
}

Plugins::Seekable DataSourceImpl::GetSeekable()
{
    FALSE_RETURN_V(stream_ != nullptr, Plugins::Seekable::INVALID);
    return stream_->seekable_;
}

int32_t DataSourceImpl::GetStreamID()
{
    return streamID_;
}

void DataSourceImpl::SetIsDash(bool flag)
{
    isDash_ = flag;
}

bool DataSourceImpl::IsDash()
{
    return isDash_;
}

DemuxerPluginManager::DemuxerPluginManager()
{
    MEDIA_LOG_D("DemuxerPluginManager called");
}

DemuxerPluginManager::~DemuxerPluginManager()
{
    MEDIA_LOG_D("~DemuxerPluginManager called");
    for (auto& iter : streamInfoMap_) {
        if (iter.second.plugin) {
            iter.second.plugin->Deinit();
        }
        iter.second.plugin = nullptr;
        iter.second.dataSource = nullptr;
    }
}

size_t DemuxerPluginManager::GetStreamCount() const
{
    return streamInfoMap_.size();
}

bool DemuxerPluginManager::GetPluginName(std::string& pluginName)
{
    FALSE_RETURN_V_NOLOG(!pluginName_.empty(), false);
    pluginName = pluginName_;
    return true;
}

void DemuxerPluginManager::InitAudioTrack(const StreamInfo& info)
{
    if (curAudioStreamID_ == -1) {    // 获取第一个音频流
        FALSE_RETURN_MSG(info.streamId >= 0, "Invalid streamId, id = " PUBLIC_LOG_D32, info.streamId);
        curAudioStreamID_ = info.streamId;
        streamInfoMap_[info.streamId].activated = true;
        MEDIA_LOG_I("InitAudioTrack AUDIO");
        isDash_ = true;
    } else {
        Meta format;
        format.Set<Tag::MEDIA_BITRATE>(static_cast<uint32_t>(info.bitRate));
        if (apiVersion_ >= API_VERSION_16) {
            format.Set<Tag::MEDIA_LANGUAGE>(info.lang);
        }
        if (apiVersion_ >= API_VERSION_18) {
            format.Set<Tag::MIME_TYPE>("audio/unknown");
        } else {
            format.Set<Tag::MIME_TYPE>("audio/xxx");
        }
        streamInfoMap_[info.streamId].mediaInfo.tracks.push_back(format);
        streamInfoMap_[info.streamId].mediaInfo.general.Set<Tag::MEDIA_HAS_AUDIO>(true);
        streamInfoMap_[info.streamId].mediaInfo.general.Set<Tag::MEDIA_TRACK_COUNT>(1);
    }
    streamInfoMap_[info.streamId].type = AUDIO;
}

void DemuxerPluginManager::InitVideoTrack(const StreamInfo& info)
{
    if (curVideoStreamID_ == -1) {
        FALSE_RETURN_MSG(info.streamId >= 0, "Invalid streamId, id = " PUBLIC_LOG_D32, info.streamId);
        curVideoStreamID_ = info.streamId; // 获取第一个视频流
        streamInfoMap_[info.streamId].activated = true;
        MEDIA_LOG_I("InitVideoTrack VIDEO");
        isDash_ = true;
    } else {
        Meta format;
        format.Set<Tag::MEDIA_BITRATE>(static_cast<uint32_t>(info.bitRate));
        format.Set<Tag::VIDEO_WIDTH>(static_cast<uint32_t>(info.videoWidth));
        format.Set<Tag::VIDEO_HEIGHT>(static_cast<uint32_t>(info.videoHeight));
        if (apiVersion_ >= API_VERSION_16) {
            format.Set<Tag::VIDEO_IS_HDR_VIVID>(
                static_cast<uint32_t>(info.videoType == VideoType::VIDEO_TYPE_HDR_VIVID));
        }
        if (apiVersion_ >= API_VERSION_18) {
            format.Set<Tag::MIME_TYPE>("video/unknown");
        } else {
            format.Set<Tag::MIME_TYPE>("video/xxx");
        }
        streamInfoMap_[info.streamId].mediaInfo.tracks.push_back(format);
        streamInfoMap_[info.streamId].mediaInfo.general.Set<Tag::MEDIA_HAS_VIDEO>(true);
        streamInfoMap_[info.streamId].mediaInfo.general.Set<Tag::MEDIA_TRACK_COUNT>(1);
    }
    streamInfoMap_[info.streamId].type = VIDEO;
}

void DemuxerPluginManager::InitSubtitleTrack(const StreamInfo& info)
{
    if (curSubTitleStreamID_ == -1) {   // 获取第一个字幕流
        FALSE_RETURN_MSG(info.streamId >= 0, "Invalid streamId, id = " PUBLIC_LOG_D32, info.streamId);
        curSubTitleStreamID_ = info.streamId;
        streamInfoMap_[info.streamId].activated = true;
        MEDIA_LOG_I("InitSubtitleTrack SUBTITLE");
    } else {
        Meta format;
        format.Set<Tag::MIME_TYPE>("text/vtt");
        streamInfoMap_[info.streamId].mediaInfo.tracks.push_back(format);
        streamInfoMap_[info.streamId].mediaInfo.general.Set<Tag::MEDIA_HAS_SUBTITLE>(true);
        streamInfoMap_[info.streamId].mediaInfo.general.Set<Tag::MEDIA_TRACK_COUNT>(1);
    }
    streamInfoMap_[info.streamId].type = SUBTITLE;
    streamInfoMap_[info.streamId].sniffSize = info.sniffSize;
}

Status DemuxerPluginManager::InitDefaultPlay(const std::vector<StreamInfo>& streams)
{
    MEDIA_LOG_D("InitDefaultPlay Begin");
    for (const auto& iter : streams) {
        FALSE_RETURN_V_MSG_E(iter.streamId >= 0, Status::ERROR_INVALID_PARAMETER,
            "Invalid streamId, id = " PUBLIC_LOG_D32, iter.streamId);
        int32_t streamIndex = iter.streamId;
        streamInfoMap_[streamIndex].streamID = streamIndex;
        streamInfoMap_[streamIndex].bitRate = iter.bitRate;
        if (iter.type == MIXED) {  // 存在混合流则只请求该流
            if (isHlsFmp4_) {
                InitVideoTrack(iter);
                continue;
            }
            curVideoStreamID_ = streamIndex;
            streamInfoMap_[streamIndex].activated = true;
            streamInfoMap_[streamIndex].type = MIXED;
            curAudioStreamID_ = -1;
            MEDIA_LOG_D("InitDefaultPlay MIX");
            break;
        } else if (iter.type == AUDIO) {
            InitAudioTrack(iter);
        } else if (iter.type == VIDEO) {
            InitVideoTrack(iter);
        } else if (iter.type == SUBTITLE) {
            InitSubtitleTrack(iter);
        } else {
            MEDIA_LOG_W("streaminfo invalid type");
        }
    }
    MEDIA_LOG_D("InitDefaultPlay End");
    return Status::OK;
}

std::shared_ptr<Plugins::DemuxerPlugin> DemuxerPluginManager::GetPluginByStreamID(int32_t streamID)
{
    if (streamID != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_.find(streamID) != streamInfoMap_.end()) {
        return streamInfoMap_[streamID].plugin;
    }
    return nullptr;
}

void DemuxerPluginManager::GetTrackInfoByStreamID(int32_t streamID, int32_t& trackId, int32_t& innerTrackId)
{
    auto iter = std::find_if(trackInfoMap_.begin(), trackInfoMap_.end(),
        [&](const std::pair<int32_t, MediaTrackMap> &item) {
        return item.second.streamID == streamID;
    });
    if (iter != trackInfoMap_.end()) {
        trackId = iter->first;
        innerTrackId = iter->second.innerTrackIndex;
    }
    return;
}

void DemuxerPluginManager::GetTrackInfoByStreamID(int32_t streamID, int32_t& trackId,
    int32_t& innerTrackId, TrackType type)
{
    auto iter = std::find_if(trackInfoMap_.begin(), trackInfoMap_.end(),
        [&](const std::pair<int32_t, MediaTrackMap> &item) {
        return item.second.streamID == streamID && GetTrackTypeByTrackID(item.first) == type;
    });
    if (iter != trackInfoMap_.end()) {
        trackId = iter->first;
        innerTrackId = iter->second.innerTrackIndex;
    }
    return;
}

Status DemuxerPluginManager::LoadDemuxerPlugin(int32_t streamID, std::shared_ptr<BaseStreamDemuxer> streamDemuxer)
{
    if (streamID == INVALID_STREAM_OR_TRACK_ID) {
        MEDIA_LOG_I("LoadDemuxerPlugin streamid invalid");
        return Status::ERROR_UNKNOWN;
    }

    std::string type;
    {
        StreamInfo streamInfo;
        streamInfo.streamId = streamID;
        streamInfo.type = streamInfoMap_[streamID].type;
        streamInfo.sniffSize = streamInfoMap_[streamID].sniffSize;
        ScopedTimer timer("SnifferMediaType", SNIFF_WARNING_MS);
        type = streamDemuxer->SnifferMediaType(streamInfo);
        if (!type.empty() && pluginName_.empty()) {
            MEDIA_LOG_D("PluginName: " PUBLIC_LOG_S, type.c_str());
            pluginName_ = type;
        }
    }
    FALSE_RETURN_V_MSG(!type.empty(), Status::ERROR_INVALID_PARAMETER, "SnifferMediaType is failed.");
    MediaTypeFound(streamDemuxer, type, streamID);

    FALSE_RETURN_V_MSG_E(streamInfoMap_[streamID].plugin != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set data source failed due to create video demuxer plugin failed.");
    Plugins::MediaInfo mediaInfoTemp;
    Status ret = streamInfoMap_[streamID].plugin->GetMediaInfo(mediaInfoTemp);
    if (ret == Status::OK) {
        streamInfoMap_[streamID].mediaInfo = mediaInfoTemp;
    }
    return ret;
}

Status DemuxerPluginManager::LoadCurrentAllPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
    Plugins::MediaInfo& mediaInfo)
{
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID) {
        MEDIA_LOG_I("LoadCurrentAllPlugin audio plugin");
        Status ret = LoadDemuxerPlugin(curAudioStreamID_, streamDemuxer);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "LoadDemuxerPlugin audio plugin failed.");
    }
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID) {
        MEDIA_LOG_D("LoadCurrentAllPlugin video plugin");
        Status ret = LoadDemuxerPlugin(curVideoStreamID_, streamDemuxer);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "LoadDemuxerPlugin video plugin failed.");
    }

    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID) {
        MEDIA_LOG_I("LoadCurrentAllPlugin subtitle plugin");
        Status ret = LoadDemuxerPlugin(curSubTitleStreamID_, streamDemuxer);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "LoadDemuxerPlugin subtitle plugin failed.");
    }
    for (auto& iter : streamInfoMap_) {
        AddMediaInfo(iter.first, mediaInfo);
    }

    curMediaInfo_ = mediaInfo;
    return Status::OK;
}

Status DemuxerPluginManager::LoadCurrentSubtitlePlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
    Plugins::MediaInfo& mediaInfo)
{
    if (curSubTitleStreamID_ == INVALID_STREAM_OR_TRACK_ID) {
        MEDIA_LOG_I("LoadCurrentSubtitleDemuxerPlugin failed, curSubTitleStreamID_ invalid");
        return Status::ERROR_UNKNOWN;
    }

    mediaInfo = curMediaInfo_;
    MEDIA_LOG_I("LoadCurrentSubtitleDemuxerPlugin begin");
    Status ret = LoadDemuxerPlugin(curSubTitleStreamID_, streamDemuxer);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "LoadDemuxerPlugin subtitle plugin failed.");
    AddMediaInfo(curSubTitleStreamID_, mediaInfo);
    curMediaInfo_ = mediaInfo;
    MEDIA_LOG_I("LoadCurrentSubtitleDemuxerPlugin success");
    return Status::OK;
}

void DemuxerPluginManager::AddMediaInfo(int32_t streamID, Plugins::MediaInfo& mediaInfo)
{
    MEDIA_LOG_D("AddMediaInfo enter");
    AddGeneral(streamInfoMap_[streamID], mediaInfo.general);
    for (uint32_t index = 0; index < streamInfoMap_[streamID].mediaInfo.tracks.size(); index++) {
        auto trackMeta = streamInfoMap_[streamID].mediaInfo.tracks[index];
        mediaInfo.tracks.push_back(trackMeta);
        MEDIA_LOG_D("AddMediaInfo streamID = " PUBLIC_LOG_D32 " index = " PUBLIC_LOG_D32, streamID, index);
        AddTrackMapInfo(streamID, index);
    }
    return;
}

Status DemuxerPluginManager::AddTrackMapInfo(int32_t streamID, int32_t trackIndex)
{
    MEDIA_LOG_D("DemuxerPluginManager::AddTrackMapInfo in");
    for (const auto& iter : trackInfoMap_) {
        if (iter.second.streamID == streamID && iter.second.innerTrackIndex == trackIndex) {
            return Status::OK;
        }
    }
    size_t index = trackInfoMap_.size();
    trackInfoMap_[index].streamID = streamID;
    trackInfoMap_[index].innerTrackIndex = trackIndex;
    return Status::OK;
}

void DemuxerPluginManager::DeleteTempTrackMapInfo(int32_t oldTrackId)
{
    MEDIA_LOG_I("DeleteTempTrackMapInfo oldTrackId =  "  PUBLIC_LOG_D32, oldTrackId);
    temp2TrackInfoMap_.erase(oldTrackId);
}

void DemuxerPluginManager::UpdateTempTrackMapInfo(int32_t oldTrackId, int32_t newTrackId, int32_t newInnerTrackIndex)
{
    temp2TrackInfoMap_[oldTrackId].streamID = trackInfoMap_[newTrackId].streamID;
    temp2TrackInfoMap_[oldTrackId].trackID = newTrackId;
    if (newInnerTrackIndex == -1) {
        MEDIA_LOG_D("UpdateTempTrackMapInfo oldTrackId =  "  PUBLIC_LOG_D32 " newTrackId = " PUBLIC_LOG_D32
            " innerTrackIndex = " PUBLIC_LOG_D32, oldTrackId, newTrackId, trackInfoMap_[newTrackId].innerTrackIndex);
        temp2TrackInfoMap_[oldTrackId].innerTrackIndex = trackInfoMap_[newTrackId].innerTrackIndex;
    } else {
        MEDIA_LOG_I("UpdateTempTrackMapInfo oldTrackId =  "  PUBLIC_LOG_D32 " newTrackId = " PUBLIC_LOG_D32
            "innerTrackIndex = " PUBLIC_LOG_D32, oldTrackId, newTrackId, newInnerTrackIndex);
        temp2TrackInfoMap_[oldTrackId].innerTrackIndex = newInnerTrackIndex;
    }
}

int32_t DemuxerPluginManager::GetTmpInnerTrackIDByTrackID(int32_t trackId)
{
    auto iter = temp2TrackInfoMap_.find(trackId);
    if (iter != temp2TrackInfoMap_.end()) {
        return temp2TrackInfoMap_[trackId].innerTrackIndex;
    }
    return INVALID_STREAM_OR_TRACK_ID;  // default
}

int32_t DemuxerPluginManager::GetTmpStreamIDByTrackID(int32_t trackId)
{
    auto iter = temp2TrackInfoMap_.find(trackId);
    if (iter != temp2TrackInfoMap_.end()) {
        return temp2TrackInfoMap_[trackId].streamID;
    }
    return INVALID_STREAM_OR_TRACK_ID;  // default
}

Status DemuxerPluginManager::UpdateGeneralValue(int32_t trackCount, const Meta& format, Meta& formatNew)
{
    formatNew.Set<Tag::MEDIA_TRACK_COUNT>(trackCount);

    bool hasVideo = false;
    format.Get<Tag::MEDIA_HAS_VIDEO>(hasVideo);
    if (hasVideo) {
        formatNew.Set<Tag::MEDIA_HAS_VIDEO>(hasVideo);
    }

    bool hasAudio = false;
    format.Get<Tag::MEDIA_HAS_AUDIO>(hasAudio);
    if (hasAudio) {
        formatNew.Set<Tag::MEDIA_HAS_AUDIO>(hasAudio);
    }

    bool hasSubtitle = false;
    format.Get<Tag::MEDIA_HAS_SUBTITLE>(hasSubtitle);
    if (hasSubtitle) {
        formatNew.Set<Tag::MEDIA_HAS_SUBTITLE>(hasSubtitle);
    }
    return Status::OK;
}

Status DemuxerPluginManager::AddGeneral(const MediaStreamInfo& info, Meta& formatNew)
{
    FileType fileType = FileType::UNKNOW;
    int32_t curTrackCount = 0;
    formatNew.Get<Tag::MEDIA_TRACK_COUNT>(curTrackCount);

    bool hasVideo = false;
    formatNew.Get<Tag::MEDIA_HAS_VIDEO>(hasVideo);

    bool hasAudio = false;
    formatNew.Get<Tag::MEDIA_HAS_AUDIO>(hasAudio);

    bool hasSubtitle = false;
    formatNew.Get<Tag::MEDIA_HAS_SUBTITLE>(hasSubtitle);

    if (formatNew.Get<Tag::MEDIA_FILE_TYPE>(fileType) == false && info.activated == true) {
        formatNew = info.mediaInfo.general;
    }

    formatNew.Set<Tag::MEDIA_HAS_VIDEO>(hasVideo);
    formatNew.Set<Tag::MEDIA_HAS_AUDIO>(hasAudio);
    formatNew.Set<Tag::MEDIA_HAS_SUBTITLE>(hasSubtitle);

    int32_t newTrackCount = 0;
    if (info.mediaInfo.general.Get<Tag::MEDIA_TRACK_COUNT>(newTrackCount) == false) {
        newTrackCount = 1;
    }
    int32_t totalTrackCount = newTrackCount + curTrackCount;
    UpdateGeneralValue(totalTrackCount, info.mediaInfo.general, formatNew);

    return Status::OK;
}

bool DemuxerPluginManager::CheckTrackIsActive(int32_t trackId)
{
    MEDIA_LOG_D("CheckTrackIsActive enter");
    auto iter = trackInfoMap_.find(trackId);
    if (iter != trackInfoMap_.end()) {
        int32_t streamId = iter->second.streamID;
        return streamInfoMap_[streamId].activated;
    }
    return false;
}

int32_t DemuxerPluginManager::GetInnerTrackIDByTrackID(int32_t trackId)
{
    auto iter = trackInfoMap_.find(trackId);
    if (iter != trackInfoMap_.end()) {
        return trackInfoMap_[trackId].innerTrackIndex;
    }
    return INVALID_STREAM_OR_TRACK_ID;  // default
}

int32_t DemuxerPluginManager::GetStreamIDByTrackID(int32_t trackId)
{
    auto iter = trackInfoMap_.find(trackId);
    if (iter != trackInfoMap_.end()) {
        return trackInfoMap_[trackId].streamID;
    }
    return INVALID_STREAM_OR_TRACK_ID;  // default
}

int32_t DemuxerPluginManager::GetStreamIDByTrackType(TrackType type)
{
    if (type == TRACK_VIDEO) {
        return curVideoStreamID_;
    } else if (type == TRACK_AUDIO) {
        return curAudioStreamID_;
    } else if (type == TRACK_SUBTITLE) {
        return curSubTitleStreamID_;
    } else {
        return INVALID_STREAM_OR_TRACK_ID;
    }
}

bool DemuxerPluginManager::CreatePlugin(std::string pluginName, int32_t id)
{
    if (streamInfoMap_[id].plugin != nullptr) {
        streamInfoMap_[id].plugin->Deinit();
    }
    auto plugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(pluginName);
    if (plugin == nullptr) {
        return false;
    }
    plugin->SetAVReadPacketStopState(false);
    streamInfoMap_[id].plugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(plugin);
    if (!streamInfoMap_[id].plugin || streamInfoMap_[id].plugin->Init() != Status::OK) {
        MEDIA_LOG_E("CreatePlugin " PUBLIC_LOG_S " failed.", pluginName.c_str());
        return false;
    }
    MEDIA_LOG_D("CreatePlugin " PUBLIC_LOG_S " success, id " PUBLIC_LOG_D32, pluginName.c_str(), id);
    streamInfoMap_[id].pluginName = pluginName;
    return true;
}

bool DemuxerPluginManager::InitPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
    const std::string& pluginName, int32_t id)
{
    if (pluginName.empty()) {
        return false;
    }
    if (streamInfoMap_[id].pluginName != pluginName) {
        FALSE_RETURN_V(CreatePlugin(pluginName, id), false);
    } else {
        if (streamInfoMap_[id].plugin == nullptr || streamInfoMap_[id].plugin->Reset() != Status::OK) {
            FALSE_RETURN_V(CreatePlugin(pluginName, id), false);
        }
    }
    MEDIA_LOG_D("InitPlugin, " PUBLIC_LOG_S " used, id " PUBLIC_LOG_D32, pluginName.c_str(), id);
    streamDemuxer->SetDemuxerState(id, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    streamDemuxer->SetIsDash(isDash_);

    streamInfoMap_[id].dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, id);
    streamInfoMap_[id].dataSource->SetIsDash(isDash_);

    Status st = streamInfoMap_[id].plugin->SetDataSource(streamInfoMap_[id].dataSource);
    if (st == Status::ERROR_NOT_ENOUGH_DATA) {
        int32_t offset = 0;
        int32_t size = 0;
        FALSE_RETURN_V(streamInfoMap_[id].plugin->GetProbeSize(offset, size), false);
        WaitForInitialBufferingEnd(streamDemuxer, offset, size);
        if (isInitialBufferingSucc_.load()) {
            st = streamInfoMap_[id].plugin->SetDataSource(streamInfoMap_[id].dataSource);
        }
    }
    return st == Status::OK;
}

void DemuxerPluginManager::WaitForInitialBufferingEnd(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
    int32_t offset, int32_t size)
{
    AutoLock lk(initialBufferingEndMutex_);
    MEDIA_LOG_I("WaitForInitialBufferingEnd");
    isInitialBufferingSucc_.store(false);
    bool isSetSucc = streamDemuxer->SetSourceInitialBufferSize(offset, size);
    // already initial buffering end, set buffer size failed
    if (!isSetSucc) {
        MEDIA_LOG_I("already initial buffering end, set buffer size failed");
        isInitialBufferingSucc_.store(true);
        return;
    }
    int32_t count = 0;
    while (!isInitialBufferingNotified_.load()) {
        initialBufferingEndCond_.WaitFor(lk, WAIT_INITIAL_BUFFERING_END_TIME_MS,
            [&] {return isInitialBufferingNotified_.load();});
        count++;
        MEDIA_LOG_I("WaitForInitialBufferingEnd count " PUBLIC_LOG_D32, count);
    }
    MEDIA_LOG_I("initial buffering success");
}

void DemuxerPluginManager::SetResetEosStatus(bool flag)
{
    needResetEosStatus_ = flag;
}

Status DemuxerPluginManager::StartPlugin(int32_t streamId, std::shared_ptr<BaseStreamDemuxer> streamDemuxer)
{
    MEDIA_LOG_I("StartPlugin begin. id = " PUBLIC_LOG_D32, streamId);
    auto iter = streamInfoMap_.find(streamId);
    if (iter != streamInfoMap_.end()) {
        streamInfoMap_[streamId].activated = true;
        if (streamInfoMap_[streamId].plugin != nullptr) {
            streamInfoMap_[streamId].plugin.reset();
            streamInfoMap_[streamId].pluginName = "";
        }
        streamDemuxer->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
        Status ret = LoadDemuxerPlugin(streamId, streamDemuxer);
        streamDemuxer->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "LoadDemuxerPlugin failed.");
        UpdateMediaInfo(streamId);
    }
    MEDIA_LOG_I("StartPlugin success. id = " PUBLIC_LOG_D32, streamId);
    return Status::OK;
}

Status DemuxerPluginManager::StopPlugin(int32_t streamId, std::shared_ptr<BaseStreamDemuxer> streamDemuxer)
{
    MEDIA_LOG_I("StopPlugin begin. id = " PUBLIC_LOG_D32, streamId);
    auto iter = streamInfoMap_.find(streamId);
    if (iter != streamInfoMap_.end()) {
        streamInfoMap_[streamId].activated = false;
        if (streamInfoMap_[streamId].plugin != nullptr) {
            streamInfoMap_[streamId].plugin.reset();
            streamInfoMap_[streamId].pluginName = "";
        }
    }
    streamDemuxer->ResetCache(streamId);
    MEDIA_LOG_I("StopPlugin success. id = " PUBLIC_LOG_D32, streamId);
    return Status::OK;
}

Status DemuxerPluginManager::RebootPlugin(int32_t streamId, TrackType trackType,
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer, bool& isRebooted)
{
    FALSE_RETURN_V_MSG_E(streamInfoMap_.find(streamId) != streamInfoMap_.end(),
        Status::ERROR_INVALID_PARAMETER, "streamId is invalid");
    FALSE_RETURN_V_MSG_E(streamDemuxer != nullptr, Status::ERROR_NULL_POINTER, "streamDemuxer is nullptr");
    MEDIA_LOG_D("RebootPlugin begin. id = " PUBLIC_LOG_D32, streamId);
    streamDemuxer->ResetCache(streamId);
    streamDemuxer->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_HEADER);

    // Start to reboot demuxer plugin while streamId is not changed
    streamInfoMap_[streamId].activated = true;
    if (streamInfoMap_[streamId].plugin != nullptr) {
        streamInfoMap_[streamId].plugin.reset();
    }
    if (streamInfoMap_[streamId].pluginName.empty()) {
        StreamInfo streamInfo;
        streamInfo.streamId = streamId;
        streamInfo.type = streamInfoMap_[streamId].type;
        streamInfo.sniffSize = streamInfoMap_[streamId].sniffSize;
        streamInfoMap_[streamId].pluginName = streamDemuxer->SnifferMediaType(streamInfo);
    }
    MediaTypeFound(streamDemuxer, streamInfoMap_[streamId].pluginName, streamId);
    FALSE_RETURN_V_MSG_E(streamInfoMap_[streamId].plugin != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set data source failed due to create video demuxer plugin failed");
    Plugins::MediaInfo mediaInfoTemp;
    Status ret = streamInfoMap_[streamId].plugin->GetMediaInfo(mediaInfoTemp);
    isRebooted = true;
    streamDemuxer->SetDemuxerState(streamId, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "GetMediaInfo failed");
    streamInfoMap_[streamId].mediaInfo = mediaInfoTemp;
    UpdateMediaInfo(streamId);
    MEDIA_LOG_D("RebootPlugin success. id = " PUBLIC_LOG_D32, streamId);
    return Status::OK;
}

void DemuxerPluginManager::MediaTypeFound(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
    const std::string& pluginName, int32_t id)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerPluginManager::MediaTypeFound");
    if (!InitPlugin(streamDemuxer, pluginName, id)) {
        MEDIA_LOG_E("Init plugin error.");
    }
}

int32_t DemuxerPluginManager::GetStreamDemuxerNewStreamID(TrackType trackType,
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer)
{
    FALSE_RETURN_V_MSG_E(streamDemuxer != nullptr, INVALID_STREAM_OR_TRACK_ID, "streamDemuxer is nullptr");
    int32_t newStreamID = INVALID_STREAM_OR_TRACK_ID;
    if (trackType == TRACK_AUDIO) {
        newStreamID = streamDemuxer->GetNewAudioStreamID();
    } else if (trackType == TRACK_SUBTITLE) {
        newStreamID = streamDemuxer->GetNewSubtitleStreamID();
    } else if (trackType == TRACK_VIDEO) {
        newStreamID = streamDemuxer->GetNewVideoStreamID();
    } else {
        MEDIA_LOG_W("Invalid trackType " PUBLIC_LOG_U32, trackType);
        return INVALID_STREAM_OR_TRACK_ID;
    }
    return newStreamID;
}

Status DemuxerPluginManager::localSubtitleSeekTo(int64_t seekTime)
{
    FALSE_RETURN_V_MSG_E(curSubTitleStreamID_ != -1 && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr,
                         Status::ERROR_NO_MEMORY, "subtitle seek failed, no subtitle");
    int64_t realSeekTime = 0;
    auto plugin = streamInfoMap_[curSubTitleStreamID_].plugin;
    auto preSeekRes = plugin->SeekTo(-1, seekTime, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    FALSE_RETURN_V(preSeekRes != Status::OK, Status::OK);
    return plugin->SeekTo(-1, seekTime, Plugins::SeekMode::SEEK_NEXT_SYNC, realSeekTime);
}

Status DemuxerPluginManager::localSubtitleSeekToStart(int64_t seekTime)
{
    FALSE_RETURN_V_MSG_E(curSubTitleStreamID_ != -1 && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr,
                         Status::ERROR_NO_MEMORY, "subtitle seek failed, no subtitle");
    int64_t realSeekTime = 0;
    auto plugin = streamInfoMap_[curSubTitleStreamID_].plugin;
    auto seekRes = plugin->SeekToStart();
    FALSE_RETURN_V(seekRes != Status::OK, Status::OK);
    return plugin->SeekTo(-1, seekTime, Plugins::SeekMode::SEEK_NEXT_SYNC, realSeekTime);
}

Status DemuxerPluginManager::SingleStreamSeekTo(int64_t seekTime, Plugins::SeekMode mode, int32_t streamID,
    int64_t& realSeekTime)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerPluginManager::SingleStreamSeekTo");
    Status ret = Status::OK;
    if (streamID >= 0 && streamInfoMap_.find(streamID) != streamInfoMap_.end() &&
        streamInfoMap_[streamID].plugin != nullptr) {
        ret = streamInfoMap_[streamID].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
    }
    return ret;
}

Status DemuxerPluginManager::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (ret != Status::OK && mode != Plugins::SeekMode::SEEK_NEXT_SYNC) {
            ret = streamInfoMap_[curSubTitleStreamID_].plugin->SeekTo(
                -1, seekTime, Plugins::SeekMode::SEEK_NEXT_SYNC, realSeekTime);
        }
    }
    return Status::OK;
}

Status DemuxerPluginManager::SeekToStart(int64_t seekTime, int64_t& realSeekTime)
{
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->SeekToStart();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->SeekToStart();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->SeekToStart();
        if (ret != Status::OK) {
            ret = streamInfoMap_[curSubTitleStreamID_].plugin->SeekTo(
                -1, seekTime, Plugins::SeekMode::SEEK_NEXT_SYNC, realSeekTime);
        }
    }
    return Status::OK;
}

Status DemuxerPluginManager::SeekToKeyFrame(int64_t seekTime, Plugins::SeekMode mode,
    int64_t& realSeekTime, DemuxerCallerType callerType)
{
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (ret != Status::OK && mode != Plugins::SeekMode::SEEK_NEXT_SYNC) {
            ret = streamInfoMap_[curSubTitleStreamID_].plugin->SeekTo(
                -1, seekTime, Plugins::SeekMode::SEEK_NEXT_SYNC, realSeekTime);
        }
    }
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->SeekToKeyFrame(
            -1, seekTime, Plugins::SeekMode::SEEK_NEXT_SYNC, realSeekTime, SEEKTOKEYFRAME_WARNING_MS);
        if (callerType == DemuxerCallerType::AVMETADATA && ret == Status::END_OF_STREAM) {
            return ret;
        }
        if (ret != Status::OK) {
            MEDIA_LOG_W("TS SeekToKeyFrame failed");
        }
    }
    return Status::OK;
}

Status DemuxerPluginManager::SeekToFrameByDts(int32_t streamID, int32_t trackId, int64_t seekTime,
    Plugins::SeekMode mode, int64_t& realSeekTime)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerPluginManager::SeekToFrameByDts");
    Status ret = Status::OK;
    if (streamID >= 0 && streamInfoMap_.find(streamID) != streamInfoMap_.end() &&
        streamInfoMap_[streamID].plugin != nullptr) {
        ret = streamInfoMap_[streamID].plugin->SeekToFrameByDts(trackId, seekTime, mode, realSeekTime,
            SEEKTOFRAMEBYDTS_TIMEOUT_MS);
    }
    return ret;
}

Status DemuxerPluginManager::Flush()
{
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Flush();
        if (needResetEosStatus_) {
            streamInfoMap_[curAudioStreamID_].plugin->ResetEosStatus();
        }
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Flush();
        if (needResetEosStatus_) {
            streamInfoMap_[curVideoStreamID_].plugin->ResetEosStatus();
        }
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->Flush();
        if (needResetEosStatus_) {
            streamInfoMap_[curSubTitleStreamID_].plugin->ResetEosStatus();
        }
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;
}

Status DemuxerPluginManager::Reset()
{
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Reset();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Reset();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->Reset();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;   // todo: 待适配返回值
}

Status DemuxerPluginManager::Stop()
{
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Stop();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Stop();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->Stop();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;   // todo: 待适配返回值
}

Status DemuxerPluginManager::Start()
{
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Start();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Start();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->Start();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;   // todo: 待适配返回值
}

Status DemuxerPluginManager::Pause()
{
    MEDIA_LOG_D("DemuxerPluginManager Pause");
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Pause();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curAudioStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Pause();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curSubTitleStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curSubTitleStreamID_].plugin->Pause();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;
}

Status DemuxerPluginManager::UpdateMediaInfo(int32_t streamID)
{
    Plugins::MediaInfo mediaInfo = curMediaInfo_;
    std::map<int32_t, MediaTrackMap> tempTrackInfoMap = trackInfoMap_;
    for (size_t i = 0; i < streamInfoMap_[streamID].mediaInfo.tracks.size(); i++) {
        auto trackMeta = streamInfoMap_[streamID].mediaInfo.tracks[i];
        size_t j = 0;
        for (j = 0; j < tempTrackInfoMap.size(); j++) {
            if (tempTrackInfoMap[j].streamID == streamID
                && tempTrackInfoMap[j].innerTrackIndex == static_cast<int32_t>(i)
                && j < mediaInfo.tracks.size()) {
                mediaInfo.tracks[j] = trackMeta;     // cover
                break;
            }
        }
        if (j >= tempTrackInfoMap.size()) {   // can not find, add
            AddTrackMapInfo(streamID, static_cast<int32_t>(i));
            mediaInfo.tracks.push_back(trackMeta);
        }
    }

    UpdateGeneralValue(trackInfoMap_.size() - tempTrackInfoMap.size(),
        streamInfoMap_[streamID].mediaInfo.general, mediaInfo.general);

    curMediaInfo_ = mediaInfo;
    return Status::OK;
}

Status DemuxerPluginManager::UpdateDefaultStreamID(Plugins::MediaInfo& mediaInfo, StreamType type, int32_t newStreamID)
{
    MEDIA_LOG_I("UpdateDefaultStreamID plugin");
    if (type == AUDIO) {
        curAudioStreamID_ = newStreamID;
    } else if (type == SUBTITLE) {
        curSubTitleStreamID_ = newStreamID;
    } else if (type == VIDEO) {
        curVideoStreamID_ = newStreamID;
    } else {}

    mediaInfo = curMediaInfo_;

    return Status::OK;
}

std::shared_ptr<Meta> DemuxerPluginManager::GetUserMeta()
{
    if (IsDash()) {
        MEDIA_LOG_W("GetUserMeta dash not support.");
        return nullptr;
    }
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    FALSE_RETURN_V_MSG_E(meta != nullptr, nullptr, "Create meta failed.");
    if (curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID && streamInfoMap_[curVideoStreamID_].plugin) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->GetUserMeta(meta);
        if (ret != Status::OK) {
            MEDIA_LOG_W("No valid user data");
        }
    } else {
        MEDIA_LOG_W("Demuxer plugin is not exist.");
    }
    return meta;
}

uint32_t DemuxerPluginManager::GetCurrentBitRate()
{
    if (IsDash() && curVideoStreamID_ != INVALID_STREAM_OR_TRACK_ID) {
        return streamInfoMap_[curVideoStreamID_].bitRate;
    }
    return 0;
}

StreamType DemuxerPluginManager::GetStreamTypeByTrackID(int32_t trackId)
{
    int32_t streamID = GetStreamIDByTrackID(trackId);
    return streamInfoMap_[streamID].type;
}

bool DemuxerPluginManager::IsSubtitleMime(const std::string& mime)
{
    if (mime == "application/x-subrip" || mime == "text/vtt") {
        return true;
    }
#ifdef SUPPORT_DEMUXER_LRC
    if (mime == "text/plain") {
        return true;
    }
#endif
#ifdef SUPPORT_DEMUXER_SAMI
    if (mime == "application/x-sami") {
        return true;
    }
#endif
#ifdef SUPPORT_DEMUXER_ASS
    if (mime == "text/x-ass") {
        return true;
    }
#endif
    return false;
}

TrackType DemuxerPluginManager::GetTrackTypeByTrackID(int32_t trackId)
{
    if (static_cast<size_t>(trackId) >= curMediaInfo_.tracks.size()) {
        return TRACK_INVALID;
    }
    std::string mimeType = "";
    bool ret = curMediaInfo_.tracks[trackId].Get<Tag::MIME_TYPE>(mimeType);
    if (ret && mimeType.find("audio") == 0) {
        return TRACK_AUDIO;
    } else if (ret && mimeType.find("video") == 0) {
        return TRACK_VIDEO;
    } else if (ret && IsSubtitleMime(mimeType)) {
        return TRACK_SUBTITLE;
    } else {
        return TRACK_INVALID;
    }
}

int32_t DemuxerPluginManager::AddExternalSubtitle()
{
    if (curSubTitleStreamID_ == INVALID_STREAM_OR_TRACK_ID) {
        auto maxKeyIt = streamInfoMap_.rbegin();
        int32_t streamIndex = streamInfoMap_.size() == 0 ? 0 : maxKeyIt->first + 1;
        curSubTitleStreamID_ = streamIndex;
        streamInfoMap_[streamIndex].activated = true;
        streamInfoMap_[streamIndex].type = SUBTITLE;
        MEDIA_LOG_I("InitDefaultPlay SUBTITLE");
        return streamIndex;
    }
    return INVALID_STREAM_OR_TRACK_ID;
}

void DemuxerPluginManager::SetInterruptState(bool isInterruptNeeded)
{
    if (curVideoStreamID_ != -1 && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        streamInfoMap_[curVideoStreamID_].plugin->SetInterruptState(isInterruptNeeded);
    }
    if (curAudioStreamID_ != -1 && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        streamInfoMap_[curAudioStreamID_].plugin->SetInterruptState(isInterruptNeeded);
    }
    if (curSubTitleStreamID_ != -1 && streamInfoMap_[curSubTitleStreamID_].plugin != nullptr) {
        streamInfoMap_[curSubTitleStreamID_].plugin->SetInterruptState(isInterruptNeeded);
    }
}

void DemuxerPluginManager::NotifyInitialBufferingEnd(bool isInitialBufferingSucc)
{
    MEDIA_LOG_D("NotifyInitialBufferingEnd, bufferingEndCond NotifyAll.");
    AutoLock lk(initialBufferingEndMutex_);
    isInitialBufferingNotified_.store(true);
    isInitialBufferingSucc_.store(isInitialBufferingSucc);
    initialBufferingEndCond_.NotifyAll();
}

void DemuxerPluginManager::SetApiVersion(int32_t apiVersion)
{
    apiVersion_ = apiVersion;
}

void DemuxerPluginManager::SetIsHlsFmp4(bool isHlsFmp4)
{
    isHlsFmp4_ = isHlsFmp4;
}

TrackType DemuxerPluginManager::GetTmpTrackTypeByTrackID(int32_t trackId)
{
    auto iter = temp2TrackInfoMap_.find(trackId);
    if (iter != temp2TrackInfoMap_.end()) {
        return GetTrackTypeByTrackID(iter->second.trackID);
    }
    return GetTrackTypeByTrackID(trackId);
}

void DemuxerPluginManager::UpdateTempTrackMapByStreamId(int32_t oldTrackId, int32_t newStreamId, TrackType type)
{
    int32_t newTrackId = -1;
    int32_t newInnerTrackId = -1;
    GetTrackInfoByStreamID(newStreamId, newTrackId, newInnerTrackId, type);
    UpdateTempTrackMapInfo(oldTrackId, newTrackId, newInnerTrackId);
}
} // namespace Media
} // namespace OHOS
