/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include "avcodec_common.h"
#include "buffer/avbuffer.h"
#include "common/media_core.h"
#include "common/media_source.h"
#include "demuxer/type_finder.h"
#include "filter/filter.h"
#include "meta/media_types.h"
#include "osal/task/task.h"
#include "plugin/plugin_base.h"
#include "plugin/plugin_info.h"
#include "plugin/plugin_time.h"
#include "osal/task/mutex.h"
#include "osal/task/condition_variable.h"

#include "source.h"
#include "demuxer_plugin.h"
#include "stream_demuxer.h"

namespace OHOS {
namespace Media {
enum TrackType {
    TRACK_VIDEO = 0,
    TRACK_AUDIO,
    TRACK_SUBTITLE,
    TRACK_INVALID
};

enum class DemuxerCallerType : int32_t;

class DemuxerPluginManager {
public:
    DemuxerPluginManager() = default;
    ~DemuxerPluginManager() = default;

    MOCK_METHOD1(InitDefaultPlay, Status(const std::vector<StreamInfo> &streams));
    MOCK_METHOD1(GetPluginByStreamID, std::shared_ptr<Plugins::DemuxerPlugin>(int32_t streamID));

    MOCK_METHOD3(GetTrackInfoByStreamID, void(int32_t streamID, int32_t &trackId, int32_t &innerTrackId));
    MOCK_METHOD4(
        GetTrackInfoByStreamID, void(int32_t streamID, int32_t &trackId, int32_t &innerTrackId, TrackType type));

    MOCK_METHOD1(GetTmpStreamIDByTrackID, int32_t(int32_t trackId));
    MOCK_METHOD1(GetTmpInnerTrackIDByTrackID, int32_t(int32_t trackId));
    MOCK_METHOD3(UpdateTempTrackMapInfo, void(int32_t oldTrackId, int32_t newTrackId, int32_t newInnerTrackIndex));
    MOCK_METHOD3(UpdateTempTrackMapByStreamId, void(int32_t oldTrackId, int32_t newStreamId, TrackType type));
    MOCK_METHOD1(DeleteTempTrackMapInfo, void(int32_t oldTrackId));

    MOCK_METHOD1(GetInnerTrackIDByTrackID, int32_t(int32_t trackId));
    MOCK_METHOD1(GetStreamTypeByTrackID, StreamType(int32_t trackId));
    MOCK_METHOD1(GetStreamIDByTrackID, int32_t(int32_t trackId));
    MOCK_METHOD1(GetStreamIDByTrackType, int32_t(TrackType type));
    MOCK_METHOD2(
        GetStreamDemuxerNewStreamID, int32_t(TrackType trackType, std::shared_ptr<BaseStreamDemuxer> streamDemuxer));

    MOCK_METHOD1(GetTrackTypeByTrackID, TrackType(int32_t trackId));
    MOCK_METHOD1(GetTmpTrackTypeByTrackID, TrackType(int32_t trackId));

    MOCK_METHOD2(LoadCurrentAllPlugin, Status(std::shared_ptr<BaseStreamDemuxer> streamDemuxer, MediaInfo &mediaInfo));
    MOCK_METHOD2(LoadCurrentSubtitlePlugin,
        Status(std::shared_ptr<BaseStreamDemuxer> streamDemuxer, Plugins::MediaInfo &mediaInfo));
    MOCK_METHOD2(LoadDemuxerPlugin, Status(int32_t streamID, std::shared_ptr<BaseStreamDemuxer> streamDemuxer));
    MOCK_METHOD0(Reset, Status());
    MOCK_METHOD0(Start, Status());
    MOCK_METHOD0(Pause, Status());
    MOCK_METHOD0(Stop, Status());
    MOCK_METHOD0(Flush, Status());
    MOCK_METHOD3(SeekTo, Status(int64_t seekTime, Plugins::SeekMode mode, int64_t &realSeekTime));
    MOCK_METHOD4(SeekToKeyFrame, Status(int64_t seekTime, Plugins::SeekMode mode,
        int64_t &realSeekTime, DemuxerCallerType callerType));
    MOCK_METHOD5(SeekToFrameByDts, Status(int32_t streamID, int32_t trackId, int64_t seekTime,
                                   Plugins::SeekMode mode, int64_t& realSeekTime));
    MOCK_METHOD1(GetStreamID, int32_t(int32_t trackId));
    MOCK_METHOD1(GetInnerTrackID, int32_t(int32_t trackId));
    MOCK_CONST_METHOD0(IsDash, bool());
    MOCK_METHOD2(StopPlugin, Status(int32_t streamId, std::shared_ptr<BaseStreamDemuxer> streamDemuxer));
    MOCK_METHOD2(StartPlugin, Status(int32_t streamId, std::shared_ptr<BaseStreamDemuxer> streamDemuxer));
    MOCK_METHOD4(RebootPlugin, Status(int32_t streamId, TrackType trackType,
                                   std::shared_ptr<BaseStreamDemuxer> streamDemuxer, bool &isRebooted));
    MOCK_METHOD3(UpdateDefaultStreamID, Status(Plugins::MediaInfo &mediaInfo, StreamType type, int32_t newStreamID));
    MOCK_METHOD4(
        SingleStreamSeekTo, Status(int64_t seekTime, Plugins::SeekMode mode, int32_t streamID, int64_t &realSeekTime));

    MOCK_METHOD0(GetUserMeta, std::shared_ptr<Meta>());
    MOCK_METHOD0(GetCurrentBitRate, uint32_t());
    MOCK_METHOD1(SetResetEosStatus, void(bool flag));
    MOCK_CONST_METHOD0(GetStreamCount, size_t());
    MOCK_METHOD1(SetCacheLimit, Status(uint32_t limitSize));
    MOCK_METHOD1(SetInterruptState, void(bool isInterruptNeeded));
    MOCK_METHOD1(CheckTrackIsActive, bool(int32_t trackId));
    MOCK_METHOD1(GetPluginName, bool(const std::string& pluginName));
    MOCK_METHOD0(AddExternalSubtitle, int32_t());
    MOCK_METHOD1(localSubtitleSeekTo, Status(int64_t seekTime));
    MOCK_METHOD1(NotifyInitialBufferingEnd, void(bool isInitialBufferingSucc));
    MOCK_METHOD1(SetApiVersion, void(int32_t apiVersion));
    MOCK_METHOD1(SetIsHlsFmp4, void(bool isHlsFmp4));
    MOCK_METHOD2(GetCurrentCacheSize, Status(uint32_t trackId, uint32_t& size));
};
}
}
#endif
