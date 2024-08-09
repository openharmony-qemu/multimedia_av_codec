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

#ifndef DEMUXER_PLUGIN_MANAGER_H
#define DEMUXER_PLUGIN_MANAGER_H

#include <atomic>
#include <limits>
#include <string>
#include <shared_mutex>

#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "demuxer/type_finder.h"
#include "filter/filter.h"
#include "meta/media_types.h"
#include "osal/task/task.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "plugin/demuxer_plugin.h"
#include "source/source.h"

namespace OHOS {
namespace Media {

class BaseStreamDemuxer;

enum TrackType {
    TRACK_VIDEO = 0,
    TRACK_AUDIO,
    TRACK_SUBTITLE,
    TRACK_INVALID
};

class DataSourceImpl : public Plugins::DataSource {
public:
    explicit DataSourceImpl(const std::shared_ptr<BaseStreamDemuxer>& stream, int32_t streamID);
    ~DataSourceImpl() override = default;
    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override;
    Status GetSize(uint64_t& size) override;
    Plugins::Seekable GetSeekable() override;
    Status SetStreamID(int32_t streamID);
    int32_t GetStreamID() override;
    bool IsDash() override;
    void SetIsDash(bool flag);
private:
    bool IsOffsetValid(int64_t offset) const;
private:
    std::shared_ptr<BaseStreamDemuxer> stream_;
    int32_t streamID_;
    bool isDash_ = false;
};

class MediaStreamInfo {
public:
    int32_t streamID = -1;
    bool activated = false;
    StreamType type;
    uint32_t bitRate;
    std::string pluginName = "";
    std::shared_ptr<Plugins::DemuxerPlugin> plugin = nullptr;
    std::shared_ptr<DataSourceImpl> dataSource = nullptr;
    Plugins::MediaInfo mediaInfo;   // dash中每个streamid只有一个track
};

class MediaTrackMap {
public:
    int32_t trackID = -1;
    int32_t streamID = -1;
    int32_t innerTrackIndex = -1;
};

class DemuxerPluginManager {
public:
    explicit DemuxerPluginManager();
    virtual ~DemuxerPluginManager();

    Status InitDefaultPlay(const std::vector<StreamInfo>& streams);
    std::shared_ptr<Plugins::DemuxerPlugin> GetPluginByStreamID(int32_t streamID);
    void GetTrackInfoByStreamID(int32_t streamID, int32_t& trackId, int32_t& innerTrackId);
    
    int32_t GetTmpStreamIDByTrackID(int32_t trackId);
    int32_t GetTmpInnerTrackIDByTrackID(int32_t trackId);
    void UpdateTempTrackMapInfo(int32_t oldTrackId, int32_t newTrackId, int32_t newInnerTrackIndex);
    void DeleteTempTrackMapInfo(int32_t oldTrackId);

    int32_t GetInnerTrackIDByTrackID(int32_t trackId);
    StreamType GetStreamTypeByTrackID(int32_t trackId);
    int32_t GetStreamIDByTrackID(int32_t trackId);
    
    TrackType GetTrackTypeByTrackID(int32_t trackId);

    Status LoadCurrentAllPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer, MediaInfo& mediaInfo);
    Status LoadCurrentSubtitlePlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer,
        Plugins::MediaInfo& mediaInfo);
    Status LoadDemuxerPlugin(int32_t streamID, std::shared_ptr<BaseStreamDemuxer> streamDemuxer);
    Status Reset();
    Status Start();
    Status Stop();
    Status Flush();
    Status SeekTo(int64_t seekTime, Plugins::SeekMode mode, int64_t& realSeekTime);
    int32_t GetStreamID(int32_t trackId);
    int32_t GetInnerTrackID(int32_t trackId);
    bool IsDash() const;
    bool IsSubtitle() const;
    Status StopPlugin(int32_t streamId, std::shared_ptr<BaseStreamDemuxer> streamDemuxer);
    Status StartPlugin(int32_t streamId, std::shared_ptr<BaseStreamDemuxer> streamDemuxer);
    Status StartAllPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer);
    Status StopAllPlugin();
    Status UpdateDefaultStreamID(Plugins::MediaInfo& mediaInfo, StreamType type, int32_t newStreamID);

    std::shared_ptr<Meta> GetUserMeta();
    uint32_t GetCurrentBitRate();
    size_t GetStreamCount() const;
    void SetResetEosStatus(bool flag);
    bool CheckTrackIsActive(int32_t trackId);
    int32_t AddExternalSubtitle();
    Status localSubtitleSeekTo(int64_t seekTime);
private:
    bool CreatePlugin(std::string pluginName, int32_t id);
    bool InitPlugin(std::shared_ptr<BaseStreamDemuxer> streamDemuxer, const std::string& pluginName, int32_t id);
    void MediaTypeFound(std::shared_ptr<BaseStreamDemuxer> streamDemuxer, const std::string& pluginName, int32_t id);
    void InitAudioTrack(const StreamInfo& info);
    void InitVideoTrack(const StreamInfo& info);
    void InitSubtitleTrack(const StreamInfo& info);
    void AddMediaInfo(int32_t streamID, Plugins::MediaInfo& mediaInfo);
    static Status UpdateGeneralValue(int32_t trackCount, const Meta& format, Meta& formatNew);
    static Status AddGeneral(const Meta& format, Meta& formatNew);

    Status AddTrackMapInfo(int32_t streamID, int32_t trackIndex);
    Status UpdateMediaInfo(int32_t streamID);
    bool IsSubtitleMime(const std::string& mime);
private:
    std::map<int32_t, MediaStreamInfo> streamInfoMap_; // <streamId, MediaStreamInfo>
    std::map<int32_t, MediaTrackMap> trackInfoMap_;    // 保存所有的track信息，使用映射的trackID <trackId, MediaTrackMap>
    std::map<int32_t, MediaTrackMap> temp2TrackInfoMap_;     // 保存正在播放的track和tempTrackInfoMap_索引映射
    int32_t curVideoStreamID_ = -1;
    int32_t curAudioStreamID_ = -1;
    int32_t curSubTitleStreamID_ = -1;

    Plugins::MediaInfo curMediaInfo_;
    bool isDash_ = false;
    bool needResetEosStatus_ = false;
};
} // namespace Media
} // namespace OHOS
#endif // MEDIA_DEMUXER_H
