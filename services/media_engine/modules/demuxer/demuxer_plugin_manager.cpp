/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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
#include "frame_detector.h"
#include "meta/media_types.h"
#include "meta/meta.h"
#include "osal/utils/dump_buffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_manager.h"
#include "plugin/plugin_manager_v2.h"
#include "plugin/plugin_time.h"
#include "base_stream_demuxer.h"
#include "media_demuxer.h"

namespace OHOS {
namespace Media {

DataSourceImpl::DataSourceImpl(std::shared_ptr<BaseStreamDemuxer>& stream, int32_t streamID)
{
    stream_ = stream;
    streamID_ = streamID;
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
    if (!buffer || !IsOffsetValid(offset)) {
        MEDIA_LOG_E("ReadAt failed, buffer empty: " PUBLIC_LOG_D32 ", expectedLen: " PUBLIC_LOG_D32
                            ", offset: " PUBLIC_LOG_D64, !buffer, static_cast<int>(expectedLen), offset);
        return Status::ERROR_UNKNOWN;
    }
    return stream_->CallbackReadAt(streamID_, offset, buffer, expectedLen);
}

Status DataSourceImpl::GetSize(uint64_t& size)
{
    size = stream_->mediaDataSize_;
    return (size > 0) ? Status::OK : Status::ERROR_WRONG_STATE;
}

Plugins::Seekable DataSourceImpl::GetSeekable()
{
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
    MEDIA_LOG_I("DemuxerPluginManager called");
}

DemuxerPluginManager::~DemuxerPluginManager()
{
    MEDIA_LOG_I("~DemuxerPluginManager called");
    for (auto& iter : streamInfoMap_) {
        if (iter.second.plugin) {
            iter.second.plugin->Deinit();
        }
        iter.second.plugin = nullptr;
        iter.second.dataSource = nullptr;
    }
}

Status DemuxerPluginManager::InitDefaultPlay(const std::vector<StreamInfo>& streams)
{
    MEDIA_LOG_I("InitDefaultPlay begin");
    for (auto& iter : streams) {
        int32_t streamIndex = iter.streamId;
        streamInfoMap_[streamIndex].streamID = streamIndex;
        streamInfoMap_[streamIndex].bitRate = iter.bitRate;
        if (iter.type == MIXED) {  // 存在混合流则只请求该流
            curVideoStreamID_ = streamIndex;
            streamInfoMap_[streamIndex].activated = true;
            streamInfoMap_[streamIndex].type = MIXED;
            curAudioStreamID_ = -1;
            MEDIA_LOG_I("InitDefaultPlay MIX");
            break;
        } else if (iter.type == AUDIO) {
            if (curAudioStreamID_ == -1) {    // 获取第一个音频流
                curAudioStreamID_ = streamIndex;
                streamInfoMap_[streamIndex].activated = true;
                MEDIA_LOG_I("InitDefaultPlay AUDIO");
                isDash_ = true;
            }
            streamInfoMap_[streamIndex].type = AUDIO;
        } else if (iter.type == VIDEO) {
            if (curVideoStreamID_ == -1) {
                curVideoStreamID_ = streamIndex; // 获取第一个视频流
                streamInfoMap_[streamIndex].activated = true;
                MEDIA_LOG_I("InitDefaultPlay VIDEO");
                isDash_ = true;
            }
            streamInfoMap_[streamIndex].type = VIDEO;
        } else if (iter.type == SUBTITLE) {
            if (curSubTitleStreamID_ == -1) {   // 获取第一个字幕流
                curSubTitleStreamID_ = streamIndex;
                streamInfoMap_[streamIndex].activated = true;
                MEDIA_LOG_I("InitDefaultPlay SUBTITLE");
            }
            streamInfoMap_[streamIndex].type = SUBTITLE;
        } else {
            MEDIA_LOG_W("streaminfo invalid type");
        }
    }
    MEDIA_LOG_I("InitDefaultPlay end");
    return Status::OK;
}

std::shared_ptr<Plugins::DemuxerPlugin> DemuxerPluginManager::GetCurVideoPlugin()
{
    if (curVideoStreamID_ != -1) {
        return streamInfoMap_[curVideoStreamID_].plugin;
    }
    return nullptr;
}

std::shared_ptr<Plugins::DemuxerPlugin> DemuxerPluginManager::GetCurAudioPlugin()
{
    if (curAudioStreamID_ != -1) {
        return streamInfoMap_[curAudioStreamID_].plugin;
    }
    return nullptr;
}

Status DemuxerPluginManager::LoadDemuxerPlugin(int32_t streamID, std::shared_ptr<BaseStreamDemuxer> streamDemuxer)
{
    if (streamID == -1) {
        MEDIA_LOG_I("LoadDemuxerPlugin streamid invalid");
        return Status::ERROR_UNKNOWN;
    }

    std::string type = streamDemuxer->SnifferMediaType(streamID);
    MediaTypeFound(streamDemuxer, type, streamID);

    Plugins::MediaInfo mediaInfoTemp;
    Status ret = Status::ERROR_UNKNOWN;

    FALSE_RETURN_V_MSG_E(streamInfoMap_[streamID].plugin != nullptr, Status::ERROR_INVALID_PARAMETER,
        "Set data source failed due to create video demuxer plugin failed.");
    ret = streamInfoMap_[streamID].plugin->GetMediaInfo(mediaInfoTemp);
    streamInfoMap_[streamID].mediaInfo = mediaInfoTemp;
    return ret;
}

Status DemuxerPluginManager::LoadCurrentAllPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
    Plugins::MediaInfo& mediaInfo)
{
    if (curAudioStreamID_ != -1) {
        MEDIA_LOG_I("LoadCurrentAllPlugin audio plugin");
        Status ret = LoadDemuxerPlugin(curAudioStreamID_, streamDemuxer);
        AddMediaInfo(ret, curAudioStreamID_, mediaInfo, true, true);   // todo: 合并mediaInfo元数据
    }
    if (curVideoStreamID_ != -1) {
        MEDIA_LOG_I("LoadCurrentAllPlugin video plugin");
        Status ret = LoadDemuxerPlugin(curVideoStreamID_, streamDemuxer);
        AddMediaInfo(ret, curVideoStreamID_, mediaInfo, true, true);   // todo: 合并mediaInfo元数据
    }
    return Status::OK;
}

Status DemuxerPluginManager::AddTrackMapInfo(int32_t streamID, int32_t trackIndex)
{
    MEDIA_LOG_I("DemuxerPluginManager::AddTrackMapInfo in");
    for (auto& iter : trackInfoMap_) {
        if (iter.second.streamID == streamID && iter.second.innerTrackIndex == trackIndex) {
            return Status::OK;
        }
    }
    int32_t index = trackInfoMap_.size();
    trackInfoMap_[index].streamID = streamID;
    trackInfoMap_[index].innerTrackIndex = trackIndex;
    return Status::OK;
}

void DemuxerPluginManager::ClearTempTrackInfo()
{
    tempTrackInfoMap_.clear();
}

void DemuxerPluginManager::UpdateTempTrackMapInfo(int32_t oldTrackId, int32_t newTrackId)
{
    MEDIA_LOG_I("UpdateTempTrackMapInfo oldTrackId =  "  PUBLIC_LOG_D32 " newTrackId = " PUBLIC_LOG_D32,
        oldTrackId, newTrackId);
    temp2TrackInfoMap_[oldTrackId].trackID = tempTrackInfoMap_[newTrackId].trackID;
    temp2TrackInfoMap_[oldTrackId].streamID = tempTrackInfoMap_[newTrackId].streamID;
    temp2TrackInfoMap_[oldTrackId].innerTrackIndex = tempTrackInfoMap_[newTrackId].innerTrackIndex;
}

Status DemuxerPluginManager::AddGeneral(const Meta& format, Meta& formatNew)
{
    formatNew = format;   // todo: 二阶段实现合并功能，当前先这么处理
    return Status::OK;
}

Status DemuxerPluginManager::AddTempTrackInfo(const Plugins::MediaInfo& mediaInfo, int32_t streamId)
{
    for (uint32_t index = 0; index < mediaInfo.tracks.size(); index++) {
        auto trackMeta = mediaInfo.tracks[index];
        for (auto& iter : tempTrackInfoMap_) {
            if (iter.second.streamID == streamId && iter.second.innerTrackIndex == index) {
                continue;
            }
        }
        int32_t tempIndex = tempTrackInfoMap_.size();
        MEDIA_LOG_I("AddTempTrackInfo index =  "  PUBLIC_LOG_D32 " id = " PUBLIC_LOG_D32
            " innerIndex = " PUBLIC_LOG_D32, tempIndex, streamId, index);
        tempTrackInfoMap_[tempIndex].streamID = streamId;
        tempTrackInfoMap_[tempIndex].innerTrackIndex = index;
    }
    return Status::OK;
}


Status DemuxerPluginManager::AddMediaInfo(Status ret, int32_t streamID, Plugins::MediaInfo& mediaInfo,
    bool isAddTrack, bool isAddTempTrack)
{
    MEDIA_LOG_I("AddMediaInfo enter");
    if (ret != Status::OK) {
        MEDIA_LOG_I("AddMediaInfo failed");
        return ret;
    }
    AddGeneral(streamInfoMap_[streamID].mediaInfo.general, mediaInfo.general);
    for (uint32_t index = 0; index < streamInfoMap_[streamID].mediaInfo.tracks.size(); index++) {
        auto trackMeta = streamInfoMap_[streamID].mediaInfo.tracks[index];
        mediaInfo.tracks.push_back(trackMeta);
        MEDIA_LOG_I("AddMediaInfo streamID = " PUBLIC_LOG_D32 " index = " PUBLIC_LOG_D32, streamID, index);
        if (isAddTrack) {
            AddTrackMapInfo(streamID, index);
        }
    }
    if (isAddTempTrack) {
        AddTempTrackInfo(streamInfoMap_[streamID].mediaInfo, streamID);
    }
    return ret;
}

std::shared_ptr<Plugins::DemuxerPlugin> DemuxerPluginManager::SelectPlugin(int32_t trackId)
{
    auto iter = temp2TrackInfoMap_.find(trackId);
    if (iter != temp2TrackInfoMap_.end()) {
        int32_t streamID = temp2TrackInfoMap_[trackId].streamID;
        return streamInfoMap_[streamID].plugin;
    }
    return nullptr;
}

int32_t DemuxerPluginManager::GetInnerTrackID(int32_t trackId)
{
    auto iter = temp2TrackInfoMap_.find(trackId);
    if (iter != temp2TrackInfoMap_.end()) {
        return temp2TrackInfoMap_[trackId].innerTrackIndex;
    }
    return 0;  // default
}

int32_t DemuxerPluginManager::GetStreamID(int32_t trackId)
{
    auto iter = temp2TrackInfoMap_.find(trackId);
    if (iter != temp2TrackInfoMap_.end()) {
        return temp2TrackInfoMap_[trackId].streamID;
    }
    return 0;  // default
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
    streamInfoMap_[id].plugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(plugin);
    if (!streamInfoMap_[id].plugin || streamInfoMap_[id].plugin->Init() != Status::OK) {
        MEDIA_LOG_E("CreatePlugin " PUBLIC_LOG_S " failed.", pluginName.c_str());
        return false;
    }
    MEDIA_LOG_I("CreatePlugin " PUBLIC_LOG_S " success, id " PUBLIC_LOG_D32, pluginName.c_str(), id);
    streamInfoMap_[id].pluginName = pluginName;
    return true;
}

bool DemuxerPluginManager::InitPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer, std::string pluginName,
    int32_t id)
{
    if (pluginName.empty()) {
        return false;
    }
    if (streamInfoMap_[id].pluginName != pluginName) {
        FALSE_RETURN_V(CreatePlugin(pluginName, id), false);
    } else {
        if (streamInfoMap_[id].plugin->Reset() != Status::OK) {
            FALSE_RETURN_V(CreatePlugin(pluginName, id), false);
        }
    }
    MEDIA_LOG_I("InitPlugin, " PUBLIC_LOG_S " used, id " PUBLIC_LOG_D32, pluginName.c_str(), id);
    streamDemuxer->SetDemuxerState(id, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    streamDemuxer->SetIsDash(isDash_);

    streamInfoMap_[id].dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, id);
    streamInfoMap_[id].dataSource->SetIsDash(isDash_);

    Status st = streamInfoMap_[id].plugin->SetDataSource(streamInfoMap_[id].dataSource);
    return st == Status::OK;
}

bool DemuxerPluginManager::IsDash()
{
    return isDash_;
}

Status DemuxerPluginManager::StartAllPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer)
{
    MEDIA_LOG_I("StartAllPlugin begin.");
    if (curAudioStreamID_ != -1) {
        Status ret = StartPlugin(curAudioStreamID_, streamDemuxer);
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curVideoStreamID_ != -1) {
        Status ret = StartPlugin(curVideoStreamID_, streamDemuxer);
        if (ret != Status::OK) {
            return ret;
        }
    }
    MEDIA_LOG_I("StartAllPlugin success.");
    return Status::OK;
}

Status DemuxerPluginManager::StopAllPlugin()
{
    MEDIA_LOG_I("StopAllPlugin begin.");
    if (curAudioStreamID_ != -1) {
        Status ret = StopPlugin(curAudioStreamID_);
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curVideoStreamID_ != -1) {
        Status ret = StopPlugin(curVideoStreamID_);
        if (ret != Status::OK) {
            return ret;
        }
    }
    MEDIA_LOG_I("StopAllPlugin success.");
    return Status::OK;
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
        Status ret = LoadDemuxerPlugin(streamId, streamDemuxer);
        if (ret != Status::OK) {
            MEDIA_LOG_E("StartPlugin failed. id = " PUBLIC_LOG_D32, streamId);
            return ret;
        }
    }
    MEDIA_LOG_I("StartPlugin success. id = " PUBLIC_LOG_D32, streamId);
    return Status::OK;
}

Status DemuxerPluginManager::StopPlugin(int32_t streamId)
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
    MEDIA_LOG_I("StopPlugin success. id = " PUBLIC_LOG_D32, streamId);
    return Status::OK;
}

void DemuxerPluginManager::MediaTypeFound(std::shared_ptr<BaseStreamDemuxer> streamDemuxer, std::string pluginName,
    int32_t id)
{
    MediaAVCodec::AVCodecTrace trace("DemuxerPluginManager::MediaTypeFound");
    if (!InitPlugin(streamDemuxer, pluginName, id)) {
        MEDIA_LOG_E("MediaTypeFound init plugin error.");
    }
}

Status DemuxerPluginManager::SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime)
{
    if (curAudioStreamID_ != -1) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curVideoStreamID_ != -1) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->SeekTo(-1, seekTime, mode, realSeekTime);
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;
}

Status DemuxerPluginManager::Flush()
{
    if (curAudioStreamID_ != -1 && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Flush();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curVideoStreamID_ != -1 && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Flush();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;
}

Status DemuxerPluginManager::Reset()
{
    if (curVideoStreamID_ != -1 && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Reset();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curAudioStreamID_ != -1 && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Reset();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;   // todo: 待适配返回值
}

Status DemuxerPluginManager::Stop()
{
    if (curVideoStreamID_ != -1 && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Stop();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curAudioStreamID_ != -1 && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Stop();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;   // todo: 待适配返回值
}

Status DemuxerPluginManager::Start()
{
    if (curVideoStreamID_ != -1 && streamInfoMap_[curVideoStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curVideoStreamID_].plugin->Start();
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (curAudioStreamID_ != -1 && streamInfoMap_[curAudioStreamID_].plugin != nullptr) {
        Status ret = streamInfoMap_[curAudioStreamID_].plugin->Start();
        if (ret != Status::OK) {
            return ret;
        }
    }
    return Status::OK;   // todo: 待适配返回值
}

Status DemuxerPluginManager::UpdateDefaultVideoStreamID(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
    Plugins::MediaInfo& mediaInfo)
{
    int32_t newStreamID = streamDemuxer->GetNewVideoStreamID();
    if (curVideoStreamID_ == newStreamID) {
        MEDIA_LOG_E("UpdateDefaultVideoStreamID failed, streamid is same. newStreamID = " PUBLIC_LOG_D32,
            newStreamID);
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (streamInfoMap_.find(newStreamID) == streamInfoMap_.end()) {
        MEDIA_LOG_E("UpdateDefaultVideoStreamID failed, streamid not exist. newStreamID = " PUBLIC_LOG_D32,
            newStreamID);
        return Status::ERROR_INVALID_PARAMETER;
    }

    ClearTempTrackInfo();

    // 更新streamInfoMap_
    MEDIA_LOG_I("subenhui UpdateDefaultVideoStreamID, stop video plugin");
    StopPlugin(curVideoStreamID_);
    
    Status ret = Status::OK;
    AddMediaInfo(ret, curAudioStreamID_, mediaInfo, false, true);

    MEDIA_LOG_I("subenhui UpdateDefaultVideoStreamID, start video plugin");
    ret = StartPlugin(newStreamID, streamDemuxer);
    AddMediaInfo(ret, newStreamID, mediaInfo, true, true);

    curVideoStreamID_ = newStreamID;

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
    if (curVideoStreamID_ != -1 && streamInfoMap_[curVideoStreamID_].plugin) {
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
    if (IsDash()) {
        return streamInfoMap_[curVideoStreamID_].bitRate;
    }
    return 0;
}

} // namespace Media
} // namespace OHOS
