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

#define MEDIA_PLUGIN
#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <chrono>
#include <limits>
#include "avcodec_trace.h"
#include "securec.h"
#include "mpeg4_demuxer_plugin.h"
#include "mpeg4_sample_helper.h"
#include "mpeg4_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "meta/video_types.h"
#include "avcodec_sysevent.h"
#include "syspara/parameters.h"

// namespace {
// constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4ReferenceParser" };
// }

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

bool MPEG4DemuxerPlugin::IsRefParserSupported()
{
    return true;
}

Status MPEG4DemuxerPlugin::ParserRefUpdatePos(int64_t timeStampMs, bool isForward)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ParserRefInfo()
{
    return Status::ERROR_AGAIN;
}

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::Dts2FrameId(int64_t dts, uint32_t &frameId)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SeekMs2FrameId(int64_t seekMs, uint32_t &frameId)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FrameId2SeekMs(uint32_t frameId, int64_t &seekMs)
{
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    MEDIA_LOG_I("GetIndexByRelativePresentationTimeUs, trackIndex is %{public}u, time is %{public}zu, "
        "index is %{public}u", trackIndex, relativePresentationTimeUs, index)
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    MEDIA_LOG_I("GetRelativePresentationTimeUsByIndex, trackIndex is %{public}u, time is %{public}zu, "
        "index is %{public}u", trackIndex, relativePresentationTimeUs, index)
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    int32_t trackCount = CountTracks();
    FALSE_RETURN_V_MSG_E(trackIndex < trackCount, Status::ERROR_INVALID_DATA, "Track is out of range");3
    // >>>为什么不能等于
    FALSE_RETURN_V_MSG_E(index < UINT32_MAX, Status::ERROR_INVALID_DATA, "Index is out of range");
    uint32_t sampleCount = GetSampleCount(trackIndex);
    // >>>ffmpeg和自研中该上限不同
    FALSE_RETURN_V_MSG_E(sampleCount <= 36000, "Sample count exceeds limit");
    GetPresentationTimeUsByMpeg4BoxParser(trackIndex);


    return Status::OK;
}

uint32_t MPEG4DemuxerPlugin::GetSampleCount(uint32_t trackIndex)
{
    FALSE_RETURN_V_MSG_E(trackIndex < trackCount, 0, "trackIndex is invalid");
    std::shared_ptr<Track> track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, 0, "firstTrack_ is nullptr");
    return track->sampleHelper->CalculateSampleCount();;
}

Status MPEG4DemuxerPlugin::GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode mode, uint32_t trackIndex,
    int64_t absolutePTS, uint32_t sampleIndex)
{
    int32_t timeScale = 0;
    mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(timeScale);
    // >>>时间基的校验是否只需要校验不等于0
    FALSE_RETURN_V_MSG_E(timeScale >= 0, Status::ERROR_INVALID_DATA, "timescale is zero");

    std::shared_ptr<Track> track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, 0, "firstTrack_ is nullptr");

    uint32_t sttsEnrtyCount = track->sampleHelper->GetSttsEntryCount();
    FALSE_RETURN_V_MSG_E(sttsEnrtyCount != 0, Status::ERROR_INVALID_DATA, "sttsEnrtyCount is empty");
    uint32_t cttsEnrtyCount = track->sampleHelper->GetcttsEntryCount();

    return (cttsEnrtyCount == 0) ? ConvertPTSAndIndexOnlyByStts(mode, absolutePTS, sampleIndex) :
        ConvertPTSAndIndexBySttsAndCtts(mode, absolutePTS, sampleIndex);
}

Status MPEG4DemuxerPlugin::ConvertPTSAndIndexOnlyByStts(IndexAndPTSConversionMode mode, int64_t absolutePTS,
    uint32_t sampleIndex)
{

}

Status MPEG4DemuxerPlugin::ConvertPTSAndIndexBySttsAndCtts(IndexAndPTSConversionMode mode, int64_t absolutePTS,
    uint32_t sampleIndex)
{

}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS