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
#define HST_LOG_TAG "MPEG4DemuxerPlugin"
#include <unistd.h>
#include <algorithm>
#include <cmath>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <chrono>
#include <limits>
#include "avcodec_trace.h"
#include "securec.h"
#include "converter.h"
#include "mpeg4_sample_helper.h"
#include "mpeg4_demuxer_plugin.h"
#include "mpeg4_utils.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "meta/video_types.h"
#include "demuxer_log_compressor.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4DemuxerPlugin" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
constexpr uint32_t DEFAULT_CACHE_LIMIT = 50 * 1024 * 1024;
constexpr int32_t DEF_PROBE_SCORE_LIMIT = 50;
constexpr uint32_t PROBE_SIZE = 5000000;
constexpr int32_t FIRST_LEVEL_RANK = 100;
constexpr int32_t SECOND_LEVEL_RANK = 95;
constexpr int32_t THIRD_LEVEL_RANK = 50;
constexpr int32_t RANK_MIN = 5;
constexpr int32_t RANK_MAX = 100 + 1; // 适配自研优先
constexpr uint8_t READ_RETRY_TIMES = 10;
constexpr uint32_t READ_RETRY_SLEEP_TIME_US = 5000;
constexpr int32_t READ_LEN = 8;
constexpr int32_t MIN_ATOM_SIZE = 16;
constexpr int64_t SNIFF_DATA_SIZE = 2048;
constexpr int32_t BOX_HEADER_SIZE = 8;
constexpr int32_t BOX_TYPE_OFFSET = 4;

static const std::map<std::string, VideoStreamType> g_streamParserMap = {
    { "video/avc",  VideoStreamType::AVC },
    { "video/hevc", VideoStreamType::HEVC },
    { "video/vvc",  VideoStreamType::VVC },
};

bool HaveValidParser(const std::string mime)
{
    return g_streamParserMap.count(mime) != 0;
}

bool NeedFfmpegDurationFallback(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    return track != nullptr && track->sampleHelper != nullptr &&
        track->codecParms.trackType == VIDEO_TYPE && !track->sampleHelper->cttsEntry_.empty();
}

bool GetFfmpegFallbackDurationUs(
    const MediaInfo& mediaInfo, uint32_t trackId, int32_t trackTimeScale, int64_t& durationUs)
{
    double frameRate = 0.0;
    if (!mediaInfo.tracks[trackId].Get<Tag::VIDEO_FRAME_RATE>(frameRate) ||
        !std::isfinite(frameRate) || frameRate <= 0 || trackTimeScale <= 0) {
        return false;
    }

    // FFmpeg MOV leaves AVPacket.duration unset when CTTS exists. Then demux.c derives duration from
    // r_frame_rate and rounds down in stream time_base. Mirror that behavior for frame attribute parity.
    double durationTicks = static_cast<double>(trackTimeScale) / frameRate;
    if (!std::isfinite(durationTicks) || durationTicks <= 0 ||
        durationTicks > static_cast<double>(std::numeric_limits<int64_t>::max())) {
        return false;
    }
    int64_t fallbackDurationTicks = static_cast<int64_t>(durationTicks);
    if (fallbackDurationTicks <= 0) {
        return false;
    }
    return ConvertTimeScale(fallbackDurationTicks, static_cast<int64_t>(S_TO_US), trackTimeScale, &durationUs) ==
        Status::OK;
}

int32_t GetSampleSizeWithCodecConfig(const MediaInfo& mediaInfo,
    const std::shared_ptr<Sample>& sample, uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr, 0, "Sample is nullptr");
    int32_t sampleSize = sample->size;
    std::string mime;
    std::vector<uint8_t> extradata {};
    if (mediaInfo.tracks[trackId].Get<Tag::MIME_TYPE>(mime) && HaveValidParser(mime) &&
        mediaInfo.tracks[trackId].Get<Tag::MEDIA_CODEC_CONFIG>(extradata) &&
        (sample->flag & Sample::SampleFlag::SYNC_FRAME)) {
        sampleSize += static_cast<int32_t>(extradata.size());
    }
    return sampleSize;
}

enum AtomLevel {
    LEVEL_0 = 0,
    LEVEL_1 = 1,
    LEVEL_2 = 2,
    LEVEL_3 = 3,
    LEVEL_4 = 4,
    LEVEL_DEFAULT = -1
};

struct AtomInfo {
    uint64_t size = 0;
    uint32_t type = 0;
};

struct MoovInfo {
    bool found = false;
    int64_t endOffset = -1;
    int64_t offset = -1;
};

namespace {
std::mutex g_mtx;

int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> dataSource);

Status RegisterMpeg4Plugin(const std::shared_ptr<Register>& reg);

static const std::vector<TrackType> g_streamMediaTypeVec = {
    AUDIO_TYPE,
    VIDEO_TYPE,
    SUBTITLE_TYPE,
    TIMEDMETA_TYPE,
    AUXILIARY_TYPE,
};

bool IsSupportedTrackType(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    return (std::find(g_streamMediaTypeVec.cbegin(), g_streamMediaTypeVec.cend(),
        track->codecParms.trackType) != g_streamMediaTypeVec.cend());
}


bool IsSupportedTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    FALSE_RETURN_V_MSG_E(track != nullptr, false, "track is nullptr");
    if (!IsSupportedTrackType(track)) {
        MEDIA_LOG_E("Unsupport track type: " PUBLIC_LOG_D32, track->codecParms.trackType);
        return false;
    }
    if (track->codecParms.trackType == ATTACHMENT_TYPE || track->sampleHelper->sampleIndexEntry_.size() <= 0) {
        MEDIA_LOG_E("Unsupport image track or empty SampleIndexEntry");
        return false;
    }
    return true;
}
} // namespace

MPEG4DemuxerPlugin::MPEG4DemuxerPlugin(std::string name)
    : DemuxerPlugin(std::move(name)),
      firstTrack_(nullptr),
      lastTrack_(nullptr),
      isIsom_(false),
      hasSidxBox_(false),
      hasMoofBox_(false),
      seekable_(Seekable::SEEKABLE),
      selectedTrackIds_(),
      cacheQueue_("cacheQueue")
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("In");
}

MPEG4DemuxerPlugin::~MPEG4DemuxerPlugin()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("In");
    avReadPacketStopState_.store(AVReadPacketStopState::TRUE, std::memory_order_release);
    ReleaseReadLoop();

    streamParsers_ = nullptr;
    firstTrack_ = nullptr;
    lastTrack_ = nullptr;
    userformat_ = nullptr;
    dataSource_ = nullptr;
    dataReader_ = nullptr;
    selectedTrackIds_.clear();
    trackMtx_.clear();
    fragmentEntry_.clear();
    firstFrameMap_.clear();
    cacheSamples_.clear();
    avTrackIds_.clear();
    inited_ = false;
    readModeFlags_.store(0, std::memory_order_release);
    MEDIA_LOG_D("Out");
}

void MPEG4DemuxerPlugin::ResetParam()
{
    ReleaseReadLoop();
    for (const auto& selectedTrackId : selectedTrackIds_) {
        cacheQueue_.RemoveTrackQueue(selectedTrackId);
    }
    // Remove cacheQueue for all video and audio tracks
    for (const auto& avTrackId: avTrackIds_) {
        cacheQueue_.RemoveTrackQueue(avTrackId);
    }
    firstTrack_.reset();
    lastTrack_.reset();
    streamParsers_.reset();
    cacheSamples_.clear();
    
    hasMoofBox_ = false;
    hasSidxBox_ = false;
    readToEnd_ = false;
    
    selectedTrackIds_.clear();
    trackMtx_.clear();
    mediaInfo_ = MediaInfo();
    readModeFlags_.store(0, std::memory_order_release);
    readLoopStatus_.store(Status::OK, std::memory_order_release);
    readToEnd_ = false;
}

Status MPEG4DemuxerPlugin::Reset()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("In");
    ResetParam();
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SeekToKeyFrame(int32_t trackId, int64_t seekTime,
    SeekMode mode, int64_t& realSeekTime, uint32_t timeoutMs)
{
    (void)trackId;
    (void)seekTime;
    (void)mode;
    (void)realSeekTime;
    (void)timeoutMs;
    MEDIA_LOG_E("File type is not MPEGTS");
    return Status::ERROR_INVALID_PARAMETER;
}

Status MPEG4DemuxerPlugin::Flush()
{
    Status ret = Status::OK;
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("In");
    ReleaseReadLoop();
    for (const auto& selectedTrackId : selectedTrackIds_) {
        ret = cacheQueue_.RemoveTrackQueue(selectedTrackId);
        ret = cacheQueue_.AddTrackQueue(selectedTrackId);
    }
    cacheSamples_.clear();
    readToEnd_ = false;
    return ret;
}

Status MPEG4DemuxerPlugin::SetDataSource(const std::shared_ptr<DataSource>& source,
    const std::shared_ptr<Media::Meta>& configs)
{
    (void)configs;
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(!inited_, Status::ERROR_WRONG_STATE, "Plugin has been initialized");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "DataSource is nullptr");
    
    seekable_ = source->IsDash() ? Seekable::UNSEEKABLE : source->GetSeekable();
    dataSource_ = source;
    auto parser = std::make_shared<MPEG4AtomParser>();
    FALSE_RETURN_V_MSG_E(parser != nullptr, Status::ERROR_NO_MEMORY, "parser allocation failed");
    Status ret = parser->MPEG4ParseHeader(source, seekable_);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "MPEG4ParseHeader failed");
    
    mediaInfo_ = parser->GetMediaInfo();
    firstTrack_ = parser->GetFirstTrack();
    userformat_ = parser->GetUserFormat();
    fragmentEntry_.clear();
    parser->GetFragmentEntry(fragmentEntry_);
    hasSidxBox_ = parser->GetHasSidx();
    hasMoofBox_ = parser->GetHasMoof();

    InitParser();

    GetMediaInfo();

    if (HasCodecParameters() != Status::OK) {
        ResetParam();
        MEDIA_LOG_E("SetDataSource failed cause not enough data");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }

    cachelimitSize_ = DEFAULT_CACHE_LIMIT;
    inited_ = true;
    MEDIA_LOG_I("Out");
    return Status::OK;
}

bool MPEG4DemuxerPlugin::FirstFrameValid(uint32_t trackIndex)
{
    return firstFrameMap_.count(trackIndex) > 0 && firstFrameMap_[trackIndex] != nullptr;
}

Status MPEG4DemuxerPlugin::ConvertElstTimescaleToMs()
{
    auto track = firstTrack_;
    if (track == nullptr) {
        MEDIA_LOG_W("This file has no track");
        return Status::OK;
    }
    int32_t fileTimeScale;
    if (!mediaInfo_.general.Get<Tag::MEDIA_TIME_SCALE>(fileTimeScale)) {
        MEDIA_LOG_E("Get mvhd timescale failed");
        return Status::ERROR_INVALID_DATA;
    }
    int32_t trackTimeScale;
    for (uint32_t trackIndex = 0; track != nullptr; ++trackIndex) {
        if (track->codecParms.trackType == ATTACHMENT_TYPE) {
            MEDIA_LOG_D("Skip attachment track " PUBLIC_LOG_U32, trackIndex);
            track = track->next;
            continue;
        }
        if (mediaInfo_.tracks[trackIndex].Get<Tag::MEDIA_TIME_SCALE>(trackTimeScale)) {
            int64_t tempEmptyEdit = 0;
            FALSE_RETURN_V_MSG_E(
                ConvertTimeScale(track->elstInitEmptyEdit, 1000LL, fileTimeScale, &tempEmptyEdit) == Status::OK,
                Status::ERROR_INVALID_PARAMETER, "mul overflow");
            track->elstInitEmptyEdit = tempEmptyEdit;
        } else {
            MEDIA_LOG_E("Get mdhd timescale failed");
            return Status::ERROR_INVALID_PARAMETER;
        }
        track = track->next;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::HasCodecParameters()
{
    int32_t param;
    int32_t trackCount;
    auto track = firstTrack_;
    Status ret = ConvertElstTimescaleToMs();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ConvertElstTimescaleToMs is failed");
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_TRACK_COUNT>(trackCount), Status::ERROR_INVALID_DATA,
        "GetData is failed");
    FALSE_RETURN_V_MSG_E(trackCount == static_cast<int32_t>(mediaInfo_.tracks.size()), Status::ERROR_INVALID_DATA,
        "mediaInfo is error");

    for (int32_t trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "track is nullptr");
        Meta &format = mediaInfo_.tracks[trackIndex];
        std::string mime;
        FALSE_RETURN_V_MSG_E(format.Get<Tag::MIME_TYPE>(mime), Status::ERROR_INVALID_DATA, "Get mime type failed");
        bool flag = !HaveValidParser(mime) || (HaveValidParser(mime) && streamParsers_ != nullptr);
        if (IsAudioType(*track)) {
            FALSE_RETURN_V_MSG_E(format.Get<Tag::AUDIO_CHANNEL_COUNT>(param), Status::ERROR_INVALID_DATA,
                "unspecified channel_count");
            FALSE_RETURN_V_MSG_E(format.Get<Tag::AUDIO_SAMPLE_RATE>(param), Status::ERROR_INVALID_DATA,
                "unspecified sample_rate");
        } else if (IsVideoType(*track)) {
            FALSE_RETURN_V_MSG_E(flag && format.Get<Tag::VIDEO_WIDTH>(param), Status::ERROR_INVALID_DATA,
                "unspecified width");
            FALSE_RETURN_V_MSG_E(flag && format.Get<Tag::VIDEO_HEIGHT>(param), Status::ERROR_INVALID_DATA,
                "unspecified height");
        }
        track = track->next;
    }
    return Status::OK;
}

void MPEG4DemuxerPlugin::InitParser()
{
    auto track = firstTrack_;
    FALSE_RETURN_MSG(track != nullptr, "track is nullptr");
    streamParsers_ = std::make_shared<MultiStreamParserManager>();
    for (uint32_t trackIndex = 0; track != nullptr; ++trackIndex) {
        std::string mime;
        mediaInfo_.tracks[trackIndex].Get<Tag::MIME_TYPE>(mime);
        if (HaveValidParser(mime) && streamParsers_ != nullptr) {
            Status ret = streamParsers_->Create(trackIndex, g_streamParserMap.at(mime));
            if (ret != Status::OK) {
                MEDIA_LOG_W("Init failed");
            } else {
                MEDIA_LOG_D("Track " PUBLIC_LOG_D32 " will be converted", trackIndex);
            }
        }
        track = track->next;
    }
}

void MPEG4DemuxerPlugin::GetAVTrackIds()
{
    avTrackIds_.clear();
    auto track = firstTrack_;
    while (track) {
        if ((IsAudioType(*track) || IsVideoType(*track))
            && track->sampleHelper->sampleIndexEntry_.size() > 0) {
            avTrackIds_.emplace_back(track->trackIndex);
            if (seekable_ == Seekable::UNSEEKABLE) {
                trackMtx_[track->trackIndex] = std::make_shared<std::mutex>();
                cacheQueue_.AddTrackQueue(track->trackIndex);
            }
        }
        track = track->next;
    }
}

Status MPEG4DemuxerPlugin::ParseAVFirstFrames()
{
    int32_t trackCount;
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_TRACK_COUNT>(trackCount), Status::ERROR_INVALID_DATA,
        "Get track count failed");
    GetAVTrackIds();
    for (const auto& avTrackId : avTrackIds_) {
        auto currentTrack = FindTrackById(avTrackId);
        FALSE_RETURN_V_MSG_E(currentTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE, "ParseAVFirstFrames interrupt");
        auto mpeg4Sample = std::make_shared<MPEG4Sample>();
        FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate mpeg4Sample");
        
        // 使用GetSampleBySeekableStatus获取首帧，传入空AVBuffer
        Status ret = GetSampleBySeekableStatus(seekable_, avTrackId, mpeg4Sample, nullptr);
        FALSE_CONTINUE_LOGD(ret == Status::OK && mpeg4Sample->sample != nullptr && mpeg4Sample->sample->data != nullptr,
            "Failed to get first frame for track:" PUBLIC_LOG_U32, avTrackId);

        // Process video track
        if (IsVideoType(*currentTrack) && streamParsers_ != nullptr) {
            if (streamParsers_->ParserIsCreated(avTrackId) && !streamParsers_->ParserIsInited(avTrackId)) {
                bool convertRet = streamParsers_->ConvertExtraDataToAnnexb(avTrackId,
                    currentTrack->codecParms.data.get(), currentTrack->codecParms.extradataSize);
                FALSE_CONTINUE_LOGD(convertRet, "ConvertExtraDataToAnnexb failed in track:" PUBLIC_LOG_U32, avTrackId);
            }
            firstFrameMap_[avTrackId] = mpeg4Sample->sample;
        }

        // Process audio track
        if (IsAudioType(*currentTrack)) {
            auto audioParser = std::make_shared<MPEG4AudioParser>();
            FALSE_RETURN_V_MSG_E(audioParser != nullptr, Status::ERROR_NO_MEMORY, "Failed create audioParser");
            ret = audioParser->ParseAudioFrame(mpeg4Sample->sample->data.get(), mpeg4Sample->sample->size,
                currentTrack->sampleHelper->mimeType_, avTrackId, mediaInfo_);
            FALSE_CONTINUE_LOGD(ret == Status::OK, "Parse audio failed in track:" PUBLIC_LOG_U32, avTrackId);
        }

        // Unseekable cannot reset SampleIndex
        if (seekable_ == Seekable::SEEKABLE) {
            currentTrack->currentSampleIndex = 0;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetMediaInfo(MediaInfo& mediaInfo)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    mediaInfo = mediaInfo_;
    return Status::OK;
}

int32_t MPEG4DemuxerPlugin::CountTracks()
{
    int32_t count = 0;
    auto track = firstTrack_;
    while (track) {
        ++count;
        track = track->next;
    }
    return count;
}

void MPEG4DemuxerPlugin::GetSingleTrackInfo(const MPEG4AtomParser::Track& track, const uint32_t trackIndex)
{
    Meta meta = mediaInfo_.tracks[trackIndex];
    std::string mime;
    mediaInfo_.tracks[trackIndex].Get<Tag::MIME_TYPE>(mime);
    if (mime == "video/hevc") {
        if (streamParsers_ != nullptr && streamParsers_->ParserIsInited(trackIndex) &&
            FirstFrameValid(trackIndex)) {
            auto firstFrame = firstFrameMap_[trackIndex];
            uint8_t* dataPtr = firstFrame->data.get();
            PacketConvertInfo convertInfo {nullptr, 0, false};
            streamParsers_->ConvertPacketToAnnexb(trackIndex, &dataPtr, firstFrame->size, convertInfo);
            streamParsers_->ParseAnnexbExtraData(trackIndex, dataPtr, firstFrame->size);
            // Parser only sends xps info when first call ConvertPacketToAnnexb
            // readSample will call ConvertPacketToAnnexb again, so rest here
            streamParsers_->ResetXPSSendStatus(trackIndex);
            ParseHEVCMetadataInfo(track, meta);
        } else {
            MEDIA_LOG_W("Parse hevc info failed");
        }
    }
    if (HaveValidParser(mime)) {
        ConvertCsdToAnnexb(track, meta, trackIndex);
    }
    mediaInfo_.tracks[trackIndex] = meta;
    DemuxerLogCompressor::StringifyMeta(meta, trackIndex);
}

Status MPEG4DemuxerPlugin::GetMediaInfo()
{
    MEDIA_LOG_D("In");
    MediaAVCodec::AVCodecTrace trace("MPEG4DemuxerPlugin::GetMediaInfo");
    int32_t trackCount = CountTracks();
    mediaInfo_.general.Set<Tag::MEDIA_TRACK_COUNT>(trackCount);
    Status ret = ParseAVFirstFrames();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Parse first frames failed");
    DemuxerLogCompressor::StringifyMeta(mediaInfo_.general, -1);
    auto track = firstTrack_;
    for (uint32_t trackIndex = 0; track != nullptr; ++trackIndex, track = track->next) {
        GetSingleTrackInfo(*track, trackIndex);
    }
    UpdateReferenceIds();
    return Status::OK;
}

void MPEG4DemuxerPlugin::UpdateReferenceIds()
{
    auto track = firstTrack_;
    for (uint32_t trackIndex = 0; track != nullptr; ++trackIndex, track = track->next) {
        if (trackIndex >= mediaInfo_.tracks.size()) {
            break;
        }
        std::vector<int32_t> referenceIds;
        mediaInfo_.tracks[trackIndex].Get<Tag::REFERENCE_TRACK_IDS>(referenceIds);
        for (auto id : referenceIds) {
            if (referenceIdsMap_.count(id) == 0) {
                referenceIdsMap_[id] = std::vector<int32_t>();
            }
            if (!std::any_of(referenceIdsMap_[id].begin(), referenceIdsMap_[id].end(),
                [trackIndex](int32_t i) { return static_cast<uint32_t>(i) == trackIndex; })) {
                referenceIdsMap_[id].push_back(trackIndex);
            }
            
            if (referenceIdsMap_.count(trackIndex) == 0) {
                referenceIdsMap_[trackIndex] = std::vector<int32_t>();
            }
            if (!std::any_of(referenceIdsMap_[trackIndex].begin(), referenceIdsMap_[trackIndex].end(),
                [id](int32_t i) { return i == id; })) {
                referenceIdsMap_[trackIndex].push_back(id);
            }
        }
    }
    for (auto item : referenceIdsMap_) {
        if (item.second.size() == 0 || static_cast<size_t>(item.first) >= mediaInfo_.tracks.size()) {
            continue;
        }
        for (auto id : item.second) {
            MEDIA_LOG_D("UpdateReferenceIds Track " PUBLIC_LOG_D32 " ref " PUBLIC_LOG_D32, item.first, id);
        }
        mediaInfo_.tracks[item.first].Set<Tag::REFERENCE_TRACK_IDS>(item.second);
    }
}

Status MPEG4DemuxerPlugin::GetUserMeta(std::shared_ptr<Meta> userformat)
{
    MediaAVCodec::AVCodecTrace trace("MPEG4DemuxerPlugin::GetUserMeta");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(userformat != nullptr, Status::ERROR_NULL_POINTER, "userformat is nullptr");
    *userformat = *userformat_;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetDrmInfo(std::multimap<std::string, std::vector<uint8_t>>& drmInfo)
{
    MEDIA_LOG_D("In");
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    // pass
    return Status::OK;
}

void MPEG4DemuxerPlugin::ConvertCsdToAnnexb(const MPEG4AtomParser::Track& track, Meta &format, uint32_t trackIndex)
{
    uint8_t *extradata = track.codecParms.data.get();
    int32_t extradataSize = track.codecParms.extradataSize;
    std::string mime;
    mediaInfo_.tracks[trackIndex].Get<Tag::MIME_TYPE>(mime);
    if (HaveValidParser(mime) && streamParsers_ != nullptr &&
        streamParsers_->ParserIsInited(trackIndex)) {
        PacketConvertInfo convertInfo {nullptr, 0, true};
        streamParsers_->ConvertPacketToAnnexb(trackIndex, &extradata, extradataSize, convertInfo);
    }
    if (extradata != nullptr && extradataSize > 0) {
        std::vector<uint8_t> extra(extradataSize);
        extra.assign(extradata, extradata + extradataSize);
        format.Set<Tag::MEDIA_CODEC_CONFIG>(extra);
    }
}

void MPEG4DemuxerPlugin::ParseHEVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta& format)
{
    HevcParseFormat parse;
    uint32_t trackIndex = track.trackIndex;
    MultiStreamParserManager::ParseMetadataInfo(trackIndex, streamParsers_, parse);
    if (parse.isHdrVivid) {
        format.Set<Tag::VIDEO_IS_HDR_VIVID>(true);
    }
    Converter::ParseColorBoxInfo(parse, format);
    HEVCProfile profile = Converter::ConvertToOHHEVCProfile(static_cast<int>(parse.profile));
    if (profile != HEVCProfile::HEVC_PROFILE_UNKNOW) {
        format.Set<Tag::VIDEO_H265_PROFILE>(profile);
        format.Set<Tag::MEDIA_PROFILE>(profile);
    } else {
        MEDIA_LOG_D("Parse hevc profile failed: " PUBLIC_LOG_D32, profile);
    }
    HEVCLevel level = Converter::ConvertToOHHEVCLevel(static_cast<int>(parse.level));
    if (level != HEVCLevel::HEVC_LEVEL_UNKNOW) {
        format.Set<Tag::VIDEO_H265_LEVEL>(level);
        format.Set<Tag::MEDIA_LEVEL>(level);
    } else {
        MEDIA_LOG_D("Parse hevc level failed: " PUBLIC_LOG_D32, level);
    }
    Converter::ParseHdrTypeInfo(track.codecParms.hdrBoxInfo, format, parse);
}

bool MPEG4DemuxerPlugin::TrackIsSelected(const uint32_t trackId)
{
    return std::find(selectedTrackIds_.begin(), selectedTrackIds_.end(), trackId) != selectedTrackIds_.end();
}

std::shared_ptr<MPEG4AtomParser::Track> MPEG4DemuxerPlugin::FindTrackById(int32_t trackId)
{
    auto track = firstTrack_;
    while (track && static_cast<int32_t>(track->trackIndex) != trackId) {
        track = track->next;
    }
    return track;
}

Status MPEG4DemuxerPlugin::SelectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Select " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    int32_t trackCount;
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_TRACK_COUNT>(trackCount),
        Status::ERROR_INVALID_DATA, "GetData is failed");
    if (trackId >= static_cast<uint32_t>(trackCount)) {
        MEDIA_LOG_E("Track is Invalid, just have " PUBLIC_LOG_D32 "tracks in file", trackCount);
        return Status::ERROR_INVALID_PARAMETER;
    }

    auto currentTrack = FindTrackById(trackId);
    FALSE_RETURN_V_MSG_E(currentTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    if (!IsSupportedTrack(currentTrack)) {
        MEDIA_LOG_E("Track type is unsupport");
        return Status::ERROR_INVALID_PARAMETER;
    }

    if (!TrackIsSelected(trackId)) {
        selectedTrackIds_.emplace_back(trackId);
        trackMtx_[trackId] = std::make_shared<std::mutex>();
        return cacheQueue_.AddTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " has been selected", trackId);
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::UnselectTrack(uint32_t trackId)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("Unselect " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    int32_t trackCount;
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_TRACK_COUNT>(trackCount),
        Status::ERROR_INVALID_DATA, "GetData is failed");
    if (trackId >= static_cast<uint32_t>(trackCount)) {
        MEDIA_LOG_E("Track is Invalid, just have " PUBLIC_LOG_D32 "tracks in file", trackCount);
        return Status::ERROR_INVALID_PARAMETER;
    }

    auto index = std::find(selectedTrackIds_.begin(), selectedTrackIds_.end(), trackId);
    if (index != selectedTrackIds_.end()) {
        selectedTrackIds_.erase(index);
        trackMtx_.erase(trackId);
        return cacheQueue_.RemoveTrackQueue(trackId);
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " is not in selected list", trackId);
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::DynamicBaseTrackSelect(int64_t seekTime, int64_t &maxPts, int32_t &baseTrackIndex)
{
    int64_t minPts = INT64_MAX;
    int64_t convertedMinPts = INT64_MAX;
    int32_t baseTrackTimeBase = 0;
    bool baseTrackIsVideo = false;
    for (const auto& currentTrackId : selectedTrackIds_) {
        auto track = FindTrackById(currentTrackId);
        FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        FALSE_RETURN_V_MSG_E(track->sampleHelper != nullptr, Status::ERROR_NULL_POINTER, "sampleHelper is nullptr");
        int32_t trackTimeBase;
        if (!mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(trackTimeBase)) {
            return Status::ERROR_INVALID_PARAMETER;
        }
        int64_t timeDelay = track->elstInitEmptyEdit - minElstInitEmptyEdit_;
        int64_t timeDelayToPts;
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(timeDelay, static_cast<int64_t>(trackTimeBase), 1000LL,
            &timeDelayToPts) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        int64_t seekTimeToPts;
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(seekTime - timeDelay, static_cast<int64_t>(trackTimeBase),
            1000LL, &seekTimeToPts) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        seekTimeToPts += track->elstShiftStartTime;

        int64_t firstPts = track->sampleHelper->sampleIndexEntry_[0].pts;  // 获取首帧pts
        int64_t lastPts = track->sampleHelper->lastPTS_;
        if (seekTimeToPts >= firstPts && seekTimeToPts < lastPts) {
            int64_t adjustedFirstPts = firstPts + timeDelayToPts;
            if (minPts != INT64_MAX) {
                FALSE_RETURN_V_MSG_E(ConvertTimeScale(minPts, static_cast<int64_t>(trackTimeBase),
                    static_cast<int64_t>(baseTrackTimeBase), &convertedMinPts) == Status::OK,
                    Status::ERROR_INVALID_PARAMETER, "mul overflow");
            }
            
            bool isVideoTrack = (IsVideoType(*track));
            int64_t trackPtsWithDelay = firstPts + timeDelayToPts;
            // 1. Current track is video, base track is not, or its PTS is below the minimum of selected tracks.
            // 2. Current track is not video, base track also not, and its PTS is below the minimum of selected tracks.
            bool shouldUpdate = isVideoTrack ? (!baseTrackIsVideo || trackPtsWithDelay < convertedMinPts) :
                                               (!baseTrackIsVideo && trackPtsWithDelay < convertedMinPts);

            if (shouldUpdate) {
                baseTrackIndex = currentTrackId;
                minPts = adjustedFirstPts;
                baseTrackTimeBase = trackTimeBase;
                baseTrackIsVideo = isVideoTrack;
            }
        }
        int64_t tempMaxPts = 0;
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(lastPts + timeDelayToPts, 1000LL, static_cast<int64_t>(trackTimeBase),
            &tempMaxPts) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        maxPts = std::max(maxPts, tempMaxPts);
    }
    MEDIA_LOG_D("Selected track " PUBLIC_LOG_D32 " with smallest PTS:" PUBLIC_LOG_D64,
        baseTrackIndex, minPts);
    return Status::OK;
}

// find base track & calculate absolute delay time
Status MPEG4DemuxerPlugin::FindBaseTrackAndCalcDelay(int64_t seekTime, SeekMode mode, int32_t &baseTrackIndex)
{
    for (const auto& currentTrackId : selectedTrackIds_) {
        auto track = FindTrackById(currentTrackId);
        FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        // calculate the absolute delay time of selected tracks
        minElstInitEmptyEdit_ = (track->elstInitEmptyEdit > 0) ?
            std::min(minElstInitEmptyEdit_, track->elstInitEmptyEdit) : 0;
    }

    int64_t maxPts = 0; // ms
    FALSE_RETURN_V_MSG_E(DynamicBaseTrackSelect(seekTime, maxPts, baseTrackIndex) == Status::OK,
        Status::ERROR_INVALID_PARAMETER, "Find base track failed");
    relativeEndPts_ = maxPts;
    if (seekTime > static_cast<int64_t>(relativeEndPts_) && mode == SeekMode::SEEK_NEXT_SYNC) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    MEDIA_LOG_D("Find base track ID: " PUBLIC_LOG_D32 ", minElstInitEmptyEdit_=" PUBLIC_LOG_D64,
        baseTrackIndex, minElstInitEmptyEdit_);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FindFragmentSyncSample(uint32_t fragmentIndex, std::shared_ptr<MPEG4AtomParser::Track> track)
{
    int64_t moofOffset = fragmentEntry_[fragmentIndex].moofOffset;
    for (const auto& selectedTrackId : selectedTrackIds_) {
        uint32_t currentTrackId = selectedTrackId;
        auto currentTrack = FindTrackById(currentTrackId);
        FALSE_RETURN_V_MSG_E(currentTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        auto& entry = currentTrack->sampleHelper->sampleIndexEntry_;
        auto it = std::upper_bound(entry.begin(), entry.end(), moofOffset,
            [](int64_t b, const MPEG4SampleHelper::SampleIndexEntry& a) { return b < a.pos; });

        uint32_t sampleIndex = std::distance(entry.begin(), it);
        if (sampleIndex < entry.size() - 1) {
            currentTrack->currentSampleIndex = sampleIndex;
        } else {
            return Status::ERROR_UNKNOWN;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FindFragmentAtTime(int64_t fragmentSeekTime, SeekMode flag,
    std::shared_ptr<MPEG4AtomParser::Track> track, bool &findFragmentSync)
{
    if (fragmentSeekTime < fragmentEntry_.front().firstDts) {
        return Status::OK;
    }
    if (fragmentSeekTime > fragmentEntry_.back().firstDts + fragmentEntry_.back().duration) {  // seek超限
        return Status::ERROR_INVALID_PARAMETER;
    }

    auto& entry = fragmentEntry_;
    auto it = std::lower_bound(entry.begin(), entry.end(), fragmentSeekTime,
        [](const MPEG4AtomParser::FragmentEntry& a, int64_t b) { return a.firstDts < b; });
    uint32_t fragmentIndex = std::distance(entry.begin(), it);
    if (fragmentSeekTime != fragmentEntry_[fragmentIndex].firstDts) {
        fragmentIndex = fragmentIndex ? fragmentIndex - 1 : 0;
    }
    currentFragment_ = fragmentIndex;
    MEDIA_LOG_D("Find fragment " PUBLIC_LOG_U32, fragmentIndex);
    if (!entry[fragmentIndex].hasRead) {
        int64_t moofOffset = entry[fragmentIndex].moofOffset;
        FALSE_RETURN_V_MSG_E(ParseMoof(moofOffset) == Status::OK, Status::ERROR_INVALID_PARAMETER, "ParseMoof failed");
    }
    bool hasVideoTrack = false;
    mediaInfo_.general.Get<Tag::MEDIA_HAS_VIDEO>(hasVideoTrack);
    if (hasVideoTrack && FindFragmentSyncSample(fragmentIndex, track) == Status::OK) {
        findFragmentSync = true;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FragmentSeek(int64_t seekTime, SeekMode mode, int32_t trackIndex, bool &findFragmentSync)
{
    Status ret = Status::OK;
    auto track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    if (!fragmentEntry_.empty()) {
        ret = FindFragmentAtTime(seekTime, mode, track, findFragmentSync);
    }
    return ret;
}

Status MPEG4DemuxerPlugin::DoSeekInternal(int32_t trackIndex, int64_t seekTime, SeekMode mode, int64_t& realSeekTime)
{
    auto track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    auto& entry = track->sampleHelper->sampleIndexEntry_;
    int64_t relativeSeekTime = seekTime - (track->elstInitEmptyEdit - minElstInitEmptyEdit_);
    int32_t baseTrackTimeScale;
    if (!mediaInfo_.tracks[trackIndex].Get<Tag::MEDIA_TIME_SCALE>(baseTrackTimeScale)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    int64_t startTime = 0;
    FALSE_RETURN_V_MSG_E(ConvertTimeScale(track->elstShiftStartTime, 1000LL, baseTrackTimeScale,
        &startTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    realSeekTime = relativeSeekTime + startTime;  // ms
    MEDIA_LOG_I("Time [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "] mode [" PUBLIC_LOG_D32 "]", seekTime,
        realSeekTime, static_cast<int32_t>(mode));

    int64_t trackSeekTime = 0;
    FALSE_RETURN_V_MSG_E(ConvertTimeScale(relativeSeekTime, static_cast<int64_t>(baseTrackTimeScale), 1000LL,
        &trackSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    trackSeekTime += track->elstShiftStartTime;

    bool findFragmentSync = false;
    Status ret = FragmentSeek(trackSeekTime, mode, trackIndex, findFragmentSync);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "FindFragmentAtTime failed");
    if (findFragmentSync) {
        return ret;
    }

    bool isVideo = IsVideoType(*track);
    uint32_t sampleIndex = 0;
    bool isBaseTrack = true;
    ret = track->sampleHelper->FindSyncSampleAtTime(trackSeekTime, isBaseTrack, mode, sampleIndex);
    // If seektime > last I-frame and within the track range, and the SeekMode is SEEK_NEXT_SYNC, seek to the EOS frame
    sampleIndex = (ret != Status::OK && mode == SeekMode::SEEK_NEXT_SYNC) ? entry.size() : sampleIndex;
    if (isVideo && seekTime - minElstInitEmptyEdit_ < static_cast<int64_t>(relativeEndPts_) &&
        sampleIndex == entry.size()) {
        isBaseTrack = false;
    }
    MEDIA_LOG_I("Find sync sample " PUBLIC_LOG_D32 " in track " PUBLIC_LOG_D32, sampleIndex, track->trackIndex);

    int64_t syncSeekTime = seekTime;
    if (isVideo && !isBaseTrack) {
        // Use the pts of the last frame as the sync time
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(track->sampleHelper->lastPTS_ - track->elstShiftStartTime, 1000LL,
            baseTrackTimeScale, &syncSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        syncSeekTime += track->elstInitEmptyEdit;
    } else if (isVideo) {
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(entry[sampleIndex].pts - track->elstShiftStartTime, 1000LL,
            baseTrackTimeScale, &syncSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        syncSeekTime += track->elstInitEmptyEdit;
    }
    ret = MultiTrackSync(trackIndex, !isBaseTrack, syncSeekTime, mode);  // 多轨同步
    track->currentSampleIndex = ret == Status::OK ? sampleIndex : track->currentSampleIndex;
    return ret;
}

Status MPEG4DemuxerPlugin::MultiTrackSync(int32_t trackIndex, bool isBaseTrack, int64_t syncSeekTime, SeekMode mode)
{
    Status ret = Status::OK;
    for (const auto& selectedTrackId : selectedTrackIds_) {
        int32_t currentTrackId = static_cast<int32_t>(selectedTrackId);
        if (currentTrackId == trackIndex) {
            continue;
        }
        auto otherTrack = FindTrackById(currentTrackId);
        FALSE_RETURN_V_MSG_E(otherTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");

        int64_t adjustSeekTime = 0;
        uint32_t otherSampleIndex = 0;
        adjustSeekTime = syncSeekTime - otherTrack->elstInitEmptyEdit;
        int32_t otherTrackTimeScale;
        if (!mediaInfo_.tracks[currentTrackId].Get<Tag::MEDIA_TIME_SCALE>(otherTrackTimeScale)) {
            return Status::ERROR_INVALID_PARAMETER;
        }

        int64_t otherTrackSeekTime = 0;
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(adjustSeekTime, static_cast<int64_t>(otherTrackTimeScale), 1000LL,
            &otherTrackSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        otherTrackSeekTime += otherTrack->elstShiftStartTime;
        ret = otherTrack->sampleHelper->FindSyncSampleAtTime(otherTrackSeekTime, isBaseTrack, mode, otherSampleIndex);
        if (ret == Status::OK) {
            otherTrack->currentSampleIndex = otherSampleIndex;
            MEDIA_LOG_D("Sync track " PUBLIC_LOG_D32 " to sample " PUBLIC_LOG_D32 " at time " PUBLIC_LOG_D64,
                currentTrackId, otherSampleIndex, adjustSeekTime);
        } else {
            MEDIA_LOG_W("Failed to sync track " PUBLIC_LOG_D32 " to time " PUBLIC_LOG_D64,
                currentTrackId, adjustSeekTime);
        }
    }
    return ret;
}

int MPEG4DemuxerPlugin::SelectSeekTrack()
{
    int trackIndex = static_cast<int>(selectedTrackIds_[0]);
    for (size_t i = 1; i < selectedTrackIds_.size(); i++) {
        int idx = static_cast<int>(selectedTrackIds_[i]);
        auto track = FindTrackById(idx);
        if (track == nullptr) {
            continue;
        }
        if (IsVideoType(*track)) {
            return idx;
        }
    }
    return trackIndex;
}

Status MPEG4DemuxerPlugin::SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t &realSeekTime)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("SeekTo");
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(seekTime >= 0 && seekTime <= INT64_MAX / MS_TO_NS, Status::ERROR_INVALID_PARAMETER,
        "Seek time " PUBLIC_LOG_D64 " is not supported", seekTime);

    ReleaseReadLoop();
    for (const auto& selectedTrackId : selectedTrackIds_) {
        cacheQueue_.RemoveTrackQueue(selectedTrackId);
        cacheQueue_.AddTrackQueue(selectedTrackId);
    }
    cacheSamples_.clear();
    readToEnd_ = false;

    trackId = SelectSeekTrack();
    Status ret = FindBaseTrackAndCalcDelay(seekTime, mode, trackId); // find base track
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Find basetrack failed");
    return DoSeekInternal(trackId, seekTime, mode, realSeekTime);
}

Status MPEG4DemuxerPlugin::SeekToStartInternal()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");

    ReleaseReadLoop();
    bool needSeek = !cacheSamples_.empty();
    for (const auto& selectedTrackId : selectedTrackIds_) {
        auto track = FindTrackById(static_cast<int32_t>(selectedTrackId));
        FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        if (track->currentSampleIndex != 0 || cacheQueue_.HasCache(selectedTrackId)) {
            needSeek = true;
            break;
        }
    }
    if (!needSeek) {
        MEDIA_LOG_I("Already at start, do not seek");
        return Status::OK;
    }

    for (const auto& selectedTrackId : selectedTrackIds_) {
        auto track = FindTrackById(static_cast<int32_t>(selectedTrackId));
        FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        track->currentSampleIndex = 0;
        cacheQueue_.RemoveTrackQueue(selectedTrackId);
        cacheQueue_.AddTrackQueue(selectedTrackId);
    }
    currentFragment_ = 0;
    minElstInitEmptyEdit_ = INT64_MAX;
    relativeEndPts_ = 0;
    cacheSamples_.clear();
    readToEnd_ = false;
    readLoopStatus_.store(Status::OK, std::memory_order_release);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SeekToStart()
{
    MEDIA_LOG_D("In");
    MediaAVCodec::AVCodecTrace trace("SeekToStart");
    auto ret = SeekToStartInternal();
    if (ret != Status::OK) {
        MEDIA_LOG_I("Use default seekto");
        int64_t realSeekTime = 0;
        ret = SeekTo(-1, 0, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SeekToStart failed");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ParseMoof(int64_t offset)
{
    auto parser = std::make_shared<MPEG4AtomParser>();
    FALSE_RETURN_V_MSG_E(parser != nullptr, Status::ERROR_NO_MEMORY, "parser allocation failed");
    auto dataReader = std::make_shared<DemuxerDataReader>();
    FALSE_RETURN_V_MSG_E(dataReader != nullptr, Status::ERROR_NO_MEMORY, "dataReader allocation failed");
    dataReader->SetDataReader(dataSource_);
    parser->SetDataReader(dataReader);
    parser->SetSeekable(seekable_);
    parser->GetFragmentEntry(fragmentEntry_);
    Status ret = parser->ParseMoof(firstTrack_, offset);
    parser->GetFragmentEntry(fragmentEntry_);

    return ret;
}

Status MPEG4DemuxerPlugin::FindValidSample(std::shared_ptr<MPEG4AtomParser::Track> &track,
    std::shared_ptr<MPEG4SampleHelper::SampleIndexEntry> &sample)
{
    std::vector<uint32_t> trackIds = selectedTrackIds_.empty() ? avTrackIds_ : selectedTrackIds_;
    for (const auto& trackId : trackIds) {
        auto currentTrack = FindTrackById(trackId);
        FALSE_RETURN_V_MSG_E(currentTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");

        uint32_t index = currentTrack->currentSampleIndex;
        uint32_t numSamples = currentTrack->sampleHelper->sampleIndexEntry_.size();
        if (index >= numSamples) {
            continue;
        }
        auto& entry = currentTrack->sampleHelper->sampleIndexEntry_;
        if (!sample) {
            track = currentTrack;
            sample = std::make_shared<MPEG4SampleHelper::SampleIndexEntry>(entry[index]);
        } else if (entry[index].pos < sample->pos) {
            while (entry[index].flag == Sample::DISCARD && index < numSamples) {
                ++index;
            }
            currentTrack->currentSampleIndex = index;
            if (index < numSamples && entry[index].pos < sample->pos &&
                entry[index].flag != Sample::DISCARD) {  // If find valid Sample, update
                track = currentTrack;
                sample = std::make_shared<MPEG4SampleHelper::SampleIndexEntry>(entry[index]);
            }
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetUnseekableSampleInfo(uint32_t &trackId, int64_t &pos, int32_t &size)
{
    FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE, "ReadSample interrupt");
    bool hasFoundSample = false;
    bool tryNextMoof = false;
    while (!hasFoundSample) {
        std::shared_ptr<MPEG4AtomParser::Track> track = nullptr;
        std::shared_ptr<MPEG4SampleHelper::SampleIndexEntry> sample = nullptr;
        FALSE_RETURN_V(FindValidSample(track, sample) == Status::OK, Status::ERROR_NULL_POINTER);
        if (!sample && !fragmentEntry_.empty() && !tryNextMoof && !readToEnd_) {
            auto ret = ParseMoof(fragmentEntry_[currentFragment_].nextAtomOffset);
            FALSE_RETURN_V_MSG_E(ret != Status::ERROR_AGAIN, Status::ERROR_AGAIN, "Parse next moof failed, try again");
            FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::END_OF_STREAM, "Parse next moof failed");
            tryNextMoof = true;
            ++currentFragment_;
            FALSE_RETURN_V(currentFragment_ < fragmentEntry_.size(), Status::END_OF_STREAM);
            continue;
        } else if (!sample) {
            return Status::END_OF_STREAM;
        }

        pos = sample->pos;
        size = sample->size;
        trackId = track->trackIndex;
        MEDIA_LOG_D("Sample: pos=" PUBLIC_LOG_D64 ", size=" PUBLIC_LOG_D32 ", trackId=" PUBLIC_LOG_U32,
            pos, size, trackId);
        hasFoundSample = true;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetSeekableSampleInfo(uint32_t trackId, int64_t &pos, int32_t &size)
{
    FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE, "ReadSample interrupt");
    auto track = FindTrackById(trackId);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    bool hasFoundSample = false;
    bool checkedFragment = false;

    while (!hasFoundSample) {
        uint32_t numSamples = track->sampleHelper->sampleIndexEntry_.size();
        uint32_t index = track->currentSampleIndex;
        MPEG4SampleHelper::SampleIndexEntry sample;
        if (index < numSamples) {
            sample = track->sampleHelper->sampleIndexEntry_[index];
        }
        if (hasSidxBox_ && hasMoofBox_ && !checkedFragment) {
            int64_t nextMoofOffset = (currentFragment_ < fragmentEntry_.size() - 1) ?
                fragmentEntry_[currentFragment_ + 1].moofOffset : -1;
            checkedFragment = true;
            if (index >= numSamples || (sample.pos > nextMoofOffset && nextMoofOffset != -1)) {
                Status ret = ParseMoof(nextMoofOffset);
                FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ParseMoof failed");
                track = FindTrackById(trackId);
                FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
                sample = track->sampleHelper->sampleIndexEntry_[index];
                ++currentFragment_;
            } else if (index >= numSamples && nextMoofOffset == -1) {
                return Status::END_OF_STREAM;
            }
        } else {
            if (index < numSamples && !(sample.flag & Sample::DISCARD)) {
                pos = sample.pos;
                size = sample.size;
                MEDIA_LOG_D("next sample pos=" PUBLIC_LOG_D64 ", size=" PUBLIC_LOG_D32, pos, size);
                hasFoundSample = true;
            } else if (index < numSamples && (sample.flag & Sample::DISCARD)) {
                ++track->currentSampleIndex;
                continue;
            } else {
                return Status::END_OF_STREAM;
            }
        }
    }

    return Status::OK;
}

Status MPEG4DemuxerPlugin::WriteSampleAttr(uint32_t trackId, std::shared_ptr<Sample> sample)
{
    auto track = FindTrackById(trackId);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    const uint32_t index = track->currentSampleIndex;
    auto entry = track->sampleHelper->sampleIndexEntry_.data();
    auto currentSample = entry[index];
    int64_t samplePts = 0;
    int64_t sampleDts = 0;
    int32_t trackTimeScale;
    if (!mediaInfo_.tracks[trackId].Get<Tag::MEDIA_TIME_SCALE>(trackTimeScale)) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    FALSE_RETURN_V_MSG_E(
        ConvertTimeScale(currentSample.pts, static_cast<int64_t>(S_TO_US), trackTimeScale, &samplePts)
        == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    sample->pts = samplePts; // us
    FALSE_RETURN_V_MSG_E(
        ConvertTimeScale(currentSample.dts, static_cast<int64_t>(S_TO_US), trackTimeScale, &sampleDts)
        == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    sample->dts = sampleDts; // us
    sample->flag = currentSample.flag;
    sample->size = currentSample.size;
    int64_t sampleDuration = 0;
    bool useFfmpegDuration = NeedFfmpegDurationFallback(track) &&
        GetFfmpegFallbackDurationUs(mediaInfo_, trackId, trackTimeScale, sampleDuration);
    if (!useFfmpegDuration) {
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(
            currentSample.duration, static_cast<int64_t>(S_TO_US), trackTimeScale, &sampleDuration)
            == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    }
    sample->duration = sampleDuration;
    // When the sample is copied in the seekable case, add it up again
    if (seekable_ == Seekable::UNSEEKABLE) {
        ++track->currentSampleIndex;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ProcessWebVTTSample(std::shared_ptr<MPEG4Sample> mpeg4Sample)
{
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, Status::ERROR_NULL_POINTER, "Mpeg4Sample is nullptr");
    std::shared_ptr<Sample> sample = mpeg4Sample->sample;
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->data != nullptr, Status::ERROR_NULL_POINTER, "Sample is nullptr");
    if (sample->size < BOX_HEADER_SIZE) {
        return Status::OK;
    }

    uint8_t* data = sample->data.get();
    uint32_t boxType = GetU32Value(data + BOX_TYPE_OFFSET);
    // vtte box and other box
    if (boxType != static_cast<uint32_t>(FourccType("vttc"))) {
        return Status::OK;
    }
    // vttc box
    mpeg4Sample->isVttc = true;
    int32_t payloadSize = sample->size - BOX_HEADER_SIZE;
    uint8_t* payloadData = data + BOX_HEADER_SIZE;
    while (payloadSize >= BOX_HEADER_SIZE) {
        uint32_t tempSize = GetU32Value(payloadData);
        if (tempSize < BOX_HEADER_SIZE || tempSize > payloadSize) {
            break;
        }
        uint32_t tempType = GetU32Value(payloadData + BOX_TYPE_OFFSET);
        if (tempType == static_cast<uint32_t>(FourccType("payl"))) {
            payloadData += BOX_HEADER_SIZE;
            payloadSize -= BOX_HEADER_SIZE;
            if (payloadSize > 0) {
                int32_t moveSize = (sample->size < payloadSize) ? sample->size : payloadSize;
                errno_t res = memmove_s(data, moveSize, payloadData, moveSize);
                FALSE_RETURN_V_MSG_E(res == EOK, Status::OK, "memmove data err");
                sample->size = payloadSize;
                data[sample->size] = '\0';
            } else {
                sample->size = 0;
            }
            break;
        } else {
            payloadData += tempSize;
            if (tempSize <= INT32_MAX && payloadSize >= INT32_MIN + static_cast<int32_t>(tempSize)) {
                payloadSize -= tempSize;
            }
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ReadSampleData(std::shared_ptr<Sample> sample, uint64_t pos, int32_t size, uint32_t trackId)
{
    MEDIA_LOG_D("In");
    FALSE_RETURN_V(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE);
    FALSE_RETURN_V_MSG_E(!stopReadThread_.load(std::memory_order_acquire),
        Status::ERROR_WRONG_STATE, "Read thread stopped");
    FALSE_RETURN_V_MSG_E(avReadPacketStopState_.load(std::memory_order_acquire) != AVReadPacketStopState::TRUE,
        Status::ERROR_WRONG_STATE, "AVReadPacket stopped");
    FALSE_RETURN_V_MSG_E(size > 0, Status::ERROR_INVALID_PARAMETER, "Invalid size");
    size_t bufferSize = static_cast<size_t>(size);

    auto buffer = std::make_shared<Buffer>();
    FALSE_RETURN_V_MSG_E(buffer != nullptr, Status::ERROR_NO_MEMORY, "Buffer allocation failed");
    sample->data = std::make_unique<uint8_t[]>(size);
    FALSE_RETURN_V_MSG_E(sample->data != nullptr, Status::ERROR_NO_MEMORY, "Memory allocation failed");
    buffer->WrapMemory(sample->data.get(), size, size);
    auto memory = buffer->GetMemory();
    FALSE_RETURN_V_MSG_E(memory != nullptr, Status::ERROR_UNKNOWN, "Memory is nullptr");

    Status ret = Status::ERROR_AGAIN;
    uint32_t count = 0;
    while (ret == Status::ERROR_AGAIN && count < READ_RETRY_TIMES) {
        if (stopReadThread_.load(std::memory_order_acquire) ||
            avReadPacketStopState_.load(std::memory_order_acquire) == AVReadPacketStopState::TRUE) {
            MEDIA_LOG_I("ReadSampleData stopped");
            sample->data.reset();
            return Status::ERROR_WRONG_STATE;
        }
        ret = dataSource_->ReadAt(pos, buffer, bufferSize);
        memory = buffer->GetMemory();
        FALSE_RETURN_V_MSG_E(memory != nullptr, Status::ERROR_UNKNOWN, "Memory is nullptr");
        if (memory->GetSize() == bufferSize) {
            break;
        }
        count++;
        usleep(READ_RETRY_SLEEP_TIME_US);
    }
    memory = buffer->GetMemory();
    FALSE_RETURN_V_MSG_E(memory != nullptr, Status::ERROR_UNKNOWN, "Memory is nullptr");
    int32_t dataSize = static_cast<int32_t>(memory->GetSize());
    MEDIA_LOG_D("Want:" PUBLIC_LOG_D32 ", Get:" PUBLIC_LOG_D32 ", offset:" PUBLIC_LOG_D64, size, dataSize, pos);
    FALSE_RETURN_V_MSG_E(dataSize > 0, Status::ERROR_AGAIN, "No enough data");
    
    if (ret == Status::OK) {
        return WriteSampleAttr(trackId, sample);
    } else {
        sample->data.reset();
        return ret;
    }
}

Status MPEG4DemuxerPlugin::ReadSampleData(std::shared_ptr<MPEG4Sample>& mpeg4Sample, uint64_t pos, int32_t size,
    uint32_t trackId)
{
    auto it = cacheSamples_.find(trackId);
    if (it != cacheSamples_.end()) {
        mpeg4Sample = it->second;
        return Status::OK;
    }
    auto sample = std::make_shared<Sample>();
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate sample");
    mpeg4Sample->sample = sample;
    
    return ReadSampleData(sample, pos, size, trackId);
}

Status MPEG4DemuxerPlugin::AddSampleToCacheQueue(std::shared_ptr<Sample> sample, const uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NULL_POINTER, "sample is nullptr");

    Status ret = Status::OK;
    std::shared_ptr<MPEG4Sample> cacheSample = std::make_shared<MPEG4Sample>();
    FALSE_RETURN_V_MSG_E(cacheSample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate MPEG4Sample");
    if (cacheSample != nullptr) {
        cacheSample->sample = sample;
        cacheSample->offset = 0;
        cacheQueue_.Push(trackId, cacheSample);
        ret = CheckCacheDataLimit(trackId);
    }
    return ret;
}

Status MPEG4DemuxerPlugin::CheckCacheDataLimit(uint32_t trackId)
{
    if (!outOfLimit_) {
        auto cacheDataSize = cacheQueue_.GetCacheDataSize(trackId);
        if (cacheDataSize > cachelimitSize_) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " cache out of limit: " PUBLIC_LOG_U32 "/" PUBLIC_LOG_U32 ", by user "
                PUBLIC_LOG_D32, trackId, cacheDataSize, cachelimitSize_, static_cast<int32_t>(setLimitByUser_));
            outOfLimit_ = true;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ConvertSampleToAnnexb(std::shared_ptr<MPEG4Sample> dstMpeg4Sample, const uint32_t trackIndex)
{
    Status ret = Status::OK;
    if (dstMpeg4Sample->isAnnexb) {
        MEDIA_LOG_D("Has converted");
        return ret;
    }
    std::string mime;
    mediaInfo_.tracks[trackIndex].Get<Tag::MIME_TYPE>(mime);
    if (HaveValidParser(mime) && streamParsers_ != nullptr &&
        streamParsers_->ParserIsInited(trackIndex)) {
        auto srcSample = dstMpeg4Sample->sample;
        uint8_t* dataPtr = srcSample->data.get();
        PacketConvertInfo convertInfo {nullptr, 0, false};
        streamParsers_->ConvertPacketToAnnexb(trackIndex, &dataPtr, srcSample->size, convertInfo);
        
        if (dataPtr != srcSample->data.get()) {
            std::unique_ptr<uint8_t[]> newData = std::make_unique<uint8_t[]>(srcSample->size);
            if (memcpy_s(newData.get(), srcSample->size, dataPtr, srcSample->size) != EOK) {
                MEDIA_LOG_E("Copy annexb data failed");
                return Status::ERROR_UNKNOWN;
            }
            srcSample->data = std::move(newData);
        }
    }
    dstMpeg4Sample->isAnnexb = true;
    return ret;
}

Status MPEG4DemuxerPlugin::SetEosSample(std::shared_ptr<AVBuffer> outBuffer)
{
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_NULL_POINTER, "Eos buffer is nullptr");
    outBuffer->pts_ = 0;
    outBuffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    MEDIA_LOG_D("Read to end");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetSampleByMimeType(const Seekable seekable, uint32_t trackId,
    std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer)
{
    auto track = FindTrackById(trackId);
    if (track == nullptr || track->sampleHelper == nullptr || track->sampleHelper->mimeType_ != MimeType::TEXT_WEBVTT) {
        return GetSampleBySeekableStatus(seekable, trackId, mpeg4Sample, outBuffer);
    }

    Status ret = Status::OK;
    // get vttc+vtte
    while (true) {
        std::shared_ptr<MPEG4Sample> tempSample = std::make_shared<MPEG4Sample>();
        ret = GetSampleBySeekableStatus(seekable, trackId, tempSample, outBuffer);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "GetSampleBySeekableStatus failed");
        if (!(outBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS))) {
            ret = ProcessWebVTTSample(tempSample);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ProcessWebVTTSample failed");
            // push all vttc to cache
            if (tempSample->isVttc) {
                ret = AddSampleToCacheQueue(tempSample->sample, trackId);
                FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add cache failed");
                track->currentSampleIndex++;
                continue;
            }
            // single vtte
            if (!cacheQueue_.HasCache(trackId)) {
                track->currentSampleIndex++;
                continue;
            }
            // compute vttc duration by vtte
            std::shared_ptr<MPEG4Sample> cacheSample  = cacheQueue_.Back(trackId);
            FALSE_RETURN_V_MSG_E(cacheSample != nullptr && cacheSample->sample != nullptr,
                Status::ERROR_NULL_POINTER, "Cache sample is nullptr");
            if (tempSample->sample->pts > cacheSample->sample->pts) {
                cacheSample->sample->duration = tempSample->sample->pts - cacheSample->sample->pts;
            }
        }
        break;
    }
    mpeg4Sample = cacheQueue_.Front(trackId);
    cacheQueue_.Pop(trackId);
    return ret;
}

Status MPEG4DemuxerPlugin::GetSampleBySeekableStatus(const Seekable seekable, uint32_t trackId,
    std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer)
{
    Status ret = Status::OK;
    int64_t pos = 0;
    int32_t size = 0;
    uint32_t trackIndex = trackId;
    if (seekable == Seekable::SEEKABLE) {
        ret = GetSeekableSampleInfo(trackIndex, pos, size);
        if (ret == Status::END_OF_STREAM) {
            return outBuffer != nullptr ? SetEosSample(outBuffer) : Status::END_OF_STREAM;
        } else if (ret != Status::OK) {
            return ret;
        } else {
            std::unique_lock<std::mutex> sLock(syncMutex_);
            ret = ReadSampleData(mpeg4Sample, pos, size, trackIndex);
            sLock.unlock();
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ReadSampleData failed");
        }
    } else if (seekable == Seekable::UNSEEKABLE) {
        while (!cacheQueue_.HasCache(trackId)) {
            ret = GetUnseekableSampleInfo(trackIndex, pos, size);
            if (ret == Status::END_OF_STREAM) {
                readToEnd_ = true;
                return outBuffer != nullptr ? SetEosSample(outBuffer) : Status::END_OF_STREAM;
            } else if (ret != Status::OK) {
                return ret;
            } else {
                std::unique_lock<std::mutex> sLock(syncMutex_);
                auto sample = std::make_shared<Sample>();
                FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate sample");
                ret = ReadSampleData(sample, pos, size, trackIndex);
                sLock.unlock();
                FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ReadSampleData failed");
                ret = AddSampleToCacheQueue(sample, trackIndex);
                FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add cache failed");
            }
        }
        std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
        mpeg4Sample = cacheQueue_.Front(trackId);
        FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, Status::ERROR_NULL_POINTER, "Cache sample is nullptr");
    }
    return Status::OK;
}

uint32_t MPEG4DemuxerPlugin::GetFlags(const std::shared_ptr<Sample> sample, bool memoryNotEnough)
{
    uint32_t flags = static_cast<uint32_t>(AVBufferFlag::NONE);
    if (sample->flag & Sample::SampleFlag::SYNC_FRAME) {
        flags |= static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME);
        flags |= static_cast<uint32_t>(AVBufferFlag::CODEC_DATA);
    }
    if (sample->flag & Sample::SampleFlag::DISCARD) {
        flags |= static_cast<uint32_t>(AVBufferFlag::DISCARD);
    }
    if (memoryNotEnough) {
        flags |= static_cast<uint32_t>(AVBufferFlag::PARTIAL_FRAME);
    }
    return flags;
}

Status MPEG4DemuxerPlugin::WriteBuffer(uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer,
    std::shared_ptr<MPEG4Sample> mpeg4Sample)
{
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_NULL_POINTER, "Buffer is nullptr");

    // copy parameters
    outBuffer->pts_ = mpeg4Sample->sample->pts;
    outBuffer->dts_ = mpeg4Sample->sample->dts;
    outBuffer->meta_->Set<Tag::BUFFER_DECODING_TIMESTAMP>(mpeg4Sample->sample->dts);
    outBuffer->duration_ = mpeg4Sample->sample->duration;
    outBuffer->meta_->Set<Tag::BUFFER_DURATION>(mpeg4Sample->sample->duration);
    MEDIA_LOG_D("CurrentBuffer: [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "], track: "
        PUBLIC_LOG_U32, outBuffer->pts_, outBuffer->dts_, outBuffer->duration_, trackIndex);
    
    CheckResetXPSSendStatus(trackIndex, mpeg4Sample->sample);
    if (cacheQueue_.ResetInfo(mpeg4Sample) == false) {
        MEDIA_LOG_D("Reset info failed");
    }
    Status ret = ConvertSampleToAnnexb(mpeg4Sample, trackIndex);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert annexb failed");
    if (cacheQueue_.SetInfo(mpeg4Sample) == false) {
        MEDIA_LOG_D("Set info failed");
    }

    int32_t remainSize = mpeg4Sample->sample->size - mpeg4Sample->offset;
    int32_t copySize = remainSize < outBuffer->memory_->GetCapacity() ? remainSize : outBuffer->memory_->GetCapacity();
    MEDIA_LOG_D("Convert size [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "]",
        mpeg4Sample->sample->size, remainSize, copySize, mpeg4Sample->offset);
    outBuffer->flag_ = GetFlags(mpeg4Sample->sample, copySize != remainSize);

    // copy data
    const uint8_t *writeData = mpeg4Sample->sample->data.get() + mpeg4Sample->offset;
    if (writeData != nullptr && copySize > 0) {
        FALSE_RETURN_V_MSG_E(outBuffer->memory_ != nullptr, Status::ERROR_NULL_POINTER, "Memory is nullptr");
        int32_t writeResult = outBuffer->memory_->Write(writeData, copySize, 0);
        FALSE_RETURN_V_MSG_E(writeResult >= 0, Status::ERROR_INVALID_OPERATION, "Memory write failed");
    }

    if (copySize < remainSize) {
        FALSE_RETURN_V_MSG_E(mpeg4Sample->offset <= INT32_MAX - copySize, Status::ERROR_INVALID_DATA,
            "Invalid offset [" PUBLIC_LOG_D32 "] copySize [" PUBLIC_LOG_D32 "]", mpeg4Sample->offset, copySize);
        mpeg4Sample->offset += copySize;
        cacheSamples_[trackIndex] = mpeg4Sample;
        MEDIA_LOG_D("Buffer is not enough, next buffer to copy remain data");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    return Status::OK;
}

void MPEG4DemuxerPlugin::CheckResetXPSSendStatus(uint32_t trackIndex, std::shared_ptr<Sample> sample)
{
    if (sample == nullptr || streamParsers_ == nullptr) {
        return;
    }
    auto track = FindTrackById(trackIndex);
    if (track == nullptr || !IsVideoType(*track)) {
        return;
    }
    if (FirstFrameValid(trackIndex) && sample->dts == firstFrameMap_[trackIndex]->dts) {
        if (streamParsers_ != nullptr) {
            streamParsers_->ResetXPSSendStatus(trackIndex);
        }
    }
}

Status MPEG4DemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> outBuffer)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("ReadSample");
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr && outBuffer->memory_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "AVBuffer or memory is nullptr");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::ASYNC)) == 0,
        Status::ERROR_INVALID_OPERATION, "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::SYNC), std::memory_order_acq_rel);
    
    std::shared_ptr<MPEG4Sample> mpeg4Sample = nullptr;
    Status ret = Status::OK;
    bool getFromCache = false;
    if (cacheQueue_.HasCache(trackId)) {
        mpeg4Sample = cacheQueue_.Front(trackId);
        getFromCache = true;
        FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate mpeg4Sample");
    } else {
        mpeg4Sample = std::make_shared<MPEG4Sample>();
        FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate mpeg4Sample");
        ret = GetSampleByMimeType(seekable_, trackId, mpeg4Sample, outBuffer);
        if (ret != Status::OK) {
            return ret;
        }
    }
    if (mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr) {
        ret = WriteBuffer(trackId, outBuffer, mpeg4Sample);
    }
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        return Status::OK;
    } else if (ret == Status::OK) {
        MEDIA_LOG_D("Sample has been copied");
        if (seekable_ == Seekable::UNSEEKABLE || getFromCache) {
            cacheQueue_.Pop(trackId);
        } else {
            auto track = FindTrackById(trackId);
            cacheSamples_.erase(trackId);
            ++track->currentSampleIndex;
        }
    }
    return ret;
}

Status MPEG4DemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t &sampleSize)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("GetNextSampleSize");
    MEDIA_LOG_D("In, track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_UNKNOWN, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::ASYNC)) == 0,
        Status::ERROR_INVALID_OPERATION, "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::SYNC), std::memory_order_acq_rel);

    auto sample = std::make_shared<Sample>();
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate sample");
    uint32_t sampleTrackId = trackId;
    if (cacheQueue_.HasCache(trackId)) {
        std::shared_ptr<MPEG4Sample> mpeg4Sample = cacheQueue_.Front(trackId);
        FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr,
            Status::ERROR_NULL_POINTER, "Cache sample is nullptr");
        sample = mpeg4Sample->sample;
    } else {
        int64_t pos = 0;
        uint32_t trackIndex = trackId;
        Status ret = Status::OK;
        if (seekable_ == Seekable::SEEKABLE) {
            ret = GetSeekableSampleInfo(trackIndex, pos, sampleSize);
        } else if (seekable_ == Seekable::UNSEEKABLE) {
            ret = GetUnseekableSampleInfo(trackIndex, pos, sampleSize);
        }
        if (ret == Status::END_OF_STREAM) {
            return ret;
        }
        auto track = FindTrackById(trackIndex);
        FALSE_RETURN_V_MSG_E(track != nullptr && track->sampleHelper != nullptr,
            Status::ERROR_NULL_POINTER, "Track is nullptr");
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get next sample info failed");
        uint32_t currentSampleIndex = track->currentSampleIndex;
        FALSE_RETURN_V_MSG_E(currentSampleIndex < track->sampleHelper->sampleIndexEntry_.size(),
            Status::END_OF_STREAM, "Sample index out of range");
        sample->size = sampleSize;
        sample->flag = track->sampleHelper->sampleIndexEntry_[currentSampleIndex].flag;
        sampleTrackId = trackIndex;
    }

    sampleSize = GetSampleSizeWithCodecConfig(mediaInfo_, sample, sampleTrackId);
    return Status::OK;
}

void MPEG4DemuxerPlugin::SetCacheLimit(uint32_t limitSize)
{
    setLimitByUser_ = true;
    cachelimitSize_ = limitSize;
}

Status MPEG4DemuxerPlugin::GetCurrentCacheSize(uint32_t trackId, uint32_t& size)
{
    MEDIA_LOG_D("TrackId " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    uint32_t dataSize = cacheQueue_.GetCacheDataSize(trackId);
    FALSE_RETURN_V_MSG_E(dataSize < UINT32_MAX, Status::ERROR_WRONG_STATE, "CacheSize is invalid");
    size = dataSize;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetCurrentCacheFrameCount(uint32_t trackId, uint32_t& frameCount)
{
    MEDIA_LOG_D("TrackId " PUBLIC_LOG_U32, trackId);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");

    uint32_t cachedFrames = cacheQueue_.GetCacheFrameCount(trackId); // 返回对应缓存的samplePacket个数
    FALSE_RETURN_V_MSG_E(cachedFrames <= UINT32_MAX, Status::ERROR_WRONG_STATE, "FrameCount is invalid");
    frameCount = cachedFrames;
    return Status::OK;
}

bool MPEG4DemuxerPlugin::GetProbeSize(int32_t &offset, int32_t &size)
{
    offset = 0;
    size = PROBE_SIZE; // 5000000 init probe size
    return true;
}

void MPEG4DemuxerPlugin::SetInterruptState(bool isInterruptNeeded)
{
    MEDIA_LOG_I("SetInterruptState %{public}d", isInterruptNeeded);
    isInterruptNeeded_ = isInterruptNeeded;
}

Status MPEG4DemuxerPlugin::SetDataSourceWithProbSize(const std::shared_ptr<DataSource>& source,
    const int32_t probSize)
{
    std::unique_lock<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(!inited_, Status::ERROR_WRONG_STATE, "Plugin has been initialized");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "DataSource is nullptr");
    FALSE_RETURN_V_MSG_E(probSize >= 0, Status::ERROR_INVALID_PARAMETER, "probSize is invalid");
    
    int32_t probScore = Sniff("avdemux_mpeg4", source);
    FALSE_RETURN_V_MSG_E(probScore >= DEF_PROBE_SCORE_LIMIT, Status::ERROR_INVALID_PARAMETER,
        "source and inputformat mismatch");
    lock.unlock();

    return SetDataSource(source);
}

bool MPEG4DemuxerPlugin::IsVideoType(const MPEG4AtomParser::Track &track)
{
    FALSE_RETURN_V_MSG_E(track.sampleHelper != nullptr, track.codecParms.trackType == TrackType::VIDEO_TYPE,
        "SampleHelper is null");
    return track.codecParms.trackType == TrackType::VIDEO_TYPE ||
        (track.codecParms.trackType == TrackType::AUXILIARY_TYPE &&
        track.sampleHelper->mimeType_.find("video") != std::string::npos);
}

bool MPEG4DemuxerPlugin::IsAudioType(const MPEG4AtomParser::Track &track)
{
    FALSE_RETURN_V_MSG_E(track.sampleHelper != nullptr, track.codecParms.trackType == TrackType::AUDIO_TYPE,
        "SampleHelper is null");
    return track.codecParms.trackType == TrackType::AUDIO_TYPE ||
        (track.codecParms.trackType == TrackType::AUXILIARY_TYPE &&
        track.sampleHelper->mimeType_.find("audio") != std::string::npos);
}

namespace {
static bool IsJPEGBrand(uint32_t fourcc)
{
    static const uint32_t jpegBrands[] = {
        FourccType("jp2 "),
        FourccType("jpx "),
        FourccType("jxl "),
    };
    for (size_t i = 0; i < sizeof(jpegBrands) / sizeof(jpegBrands[0]); ++i) {
        if (jpegBrands[i] == fourcc) {
            return true;
        }
    }
    return false;
}

static bool IsBrand(uint32_t fourcc)
{
    static const uint32_t compatibleBrands[] = {
        FourccType("isom"),
        FourccType("iso2"),
        FourccType("avc1"),
        FourccType("hvc1"),
        FourccType("hev1"),
        FourccType("vvc1"),
        FourccType("vvi1"),
        FourccType("av01"),
        FourccType("vp09"),
        FourccType("3gp4"),
        FourccType("mp41"),
        FourccType("mp42"),
        FourccType("dash"),
        FourccType("nvr1"),

        FourccType("qt  "),
        FourccType("MSNV"),
        FourccType("wmf "),
        FourccType("3g2a"),
        FourccType("3g2b"),
        FourccType("mif1"),
        FourccType("heic"),
        FourccType("msf1"),
        FourccType("hevc"),
        FourccType("avif"),
        FourccType("avis"),
        FourccType("iso4"),
    };
    for (size_t i = 0; i < sizeof(compatibleBrands) / sizeof(compatibleBrands[0]); ++i) {
        if (compatibleBrands[i] == fourcc) {
            return true;
        }
    }
    return false;
}

void IsMPEGPS(int64_t offset, int64_t kMaxOffset, const std::shared_ptr<DemuxerDataReader>& dataReader,
    int32_t& confidence)
{
    const int32_t readSize = 16;
    const int32_t minDataStep = 2;
    const uint8_t secondTagOffset = 2;
    const uint8_t thirdTagOffset = 3;
    while (offset < kMaxOffset - readSize) {
        uint8_t tag[16];
        uint32_t tagType[4];
        FALSE_RETURN_MSG(dataReader != nullptr &&
            dataReader->ReadUintData(offset, tag, sizeof(tag)) == Status::OK, "Read probe data failed");
        for (uint32_t i = 0; i < sizeof(tagType) / sizeof(tagType[0]); ++i) {
            tagType[i] = GetU32Value(&tag[sizeof(uint32_t) * i]);
        }
        if (tagType[0] == FourccType("hdlr") && tagType[secondTagOffset] == FourccType("mhlr") &&
            tagType[thirdTagOffset] == FourccType("MPEG")) {
            MEDIA_LOG_D("MPEG-PS");
            confidence = RANK_MIN;
            break;
        }
        else {
            offset += minDataStep;
        }
    }
}

int32_t GetFtypConfidence(std::shared_ptr<DemuxerDataReader> dataReader_,
    int64_t chunkDataOffset, int64_t chunkDataSize, int32_t confidence)
{
    const int32_t readLengHalf = 4;
    FALSE_RETURN_V_MSG_E(chunkDataSize >= READ_LEN, 0, "ftyp data size error");
    uint32_t numBrands = (chunkDataSize - READ_LEN) / readLengHalf;
    uint32_t numBrandsIteration = 0;
    uint32_t offSetMore = 2;
    FALSE_RETURN_V_MSG_E(!(__builtin_add_overflow(numBrands, offSetMore, &numBrandsIteration)), 0,
        "numBrands has add overflow");
    for (size_t i = 0; i < numBrandsIteration; ++i) {
        if (i == 1) {
            continue;
        }
        uint32_t brand = 0;
        uint8_t brandData[4];
        FALSE_RETURN_V_MSG_E(dataReader_->ReadUintData(
            chunkDataOffset + readLengHalf * i, brandData, sizeof(brandData)) == Status::OK,
            0, "Read brand failed");
        brand = GetU32Value(brandData);
        if (i == 0 && !IsJPEGBrand(brand)) {
            confidence = FIRST_LEVEL_RANK;
            return confidence;
        }
        if (i == 0) {
            confidence = std::max(confidence, RANK_MIN);
        } else if (IsBrand(brand)) {
            confidence = FIRST_LEVEL_RANK;
            return confidence;
        }
    }
    return confidence;
}

bool UpdateAtomSize(
    AtomInfo& atom, int64_t offset, int64_t& chunkDataOffset, std::shared_ptr<DemuxerDataReader> dataReader)
{
    if (atom.size == 1) {
        uint8_t sizeData[8];
        FALSE_RETURN_V_MSG_E(
            dataReader->ReadUintData(offset + READ_LEN, sizeData, sizeof(sizeData)) == Status::OK, false,
            "Read atom size(version 1) failed");
        atom.size = GetU64Value(sizeData);
        chunkDataOffset += READ_LEN;
        FALSE_RETURN_V_MSG_E(atom.size >= MIN_ATOM_SIZE && atom.size <= INT64_MAX, false, "atom size error");
    } else if (atom.size < READ_LEN) {
        MEDIA_LOG_E("atom.size error");
        return false;
    }
    return true;
}

AtomLevel GetAtomLevel(AtomInfo atom)
{
    switch (atom.type) {
        case FourccType("ftyp"): {
            return AtomLevel::LEVEL_0;
        }
        case FourccType("mdat"):
        case FourccType("pnot"):
        case FourccType("udta"): {
            return AtomLevel::LEVEL_1;
        }
        case FourccType("moov"): {
            return AtomLevel::LEVEL_2;
        }
        // case FourccType("ediw"):
        // case FourccType("junk"):
        // case FourccType("pict"):
        case FourccType("wide"):
        case FourccType("free"): {
            return AtomLevel::LEVEL_3;
        }
        // case 0x82827f7d:
        // case FourccType("prfl"):
        case FourccType("skip"):
        case FourccType("uuid"): {
            return AtomLevel::LEVEL_4;
        }
        default:
            return AtomLevel::LEVEL_DEFAULT;
    }
}

int32_t GetConfidence(std::shared_ptr<DemuxerDataReader> dataReader)
{
    MoovInfo moov {false, -1, -1};
    int64_t offset = 0;
    int32_t confidence = 0;

    while (!moov.found && offset < SNIFF_DATA_SIZE) {
        uint8_t atomInfoData[8];
        FALSE_RETURN_V_MSG_E(dataReader->ReadUintData(offset, atomInfoData, sizeof(atomInfoData)) == Status::OK, 0,
            "Read atom failed");
        AtomInfo atom {GetU32Value(&atomInfoData[0]), GetU32Value(&atomInfoData[4])};
        int64_t chunkDataOffset = offset + READ_LEN;
        FALSE_RETURN_V_NOLOG(UpdateAtomSize(atom, offset, chunkDataOffset, dataReader), 0);
        int64_t chunkDataSize = atom.size - (chunkDataOffset - offset);
        FALSE_RETURN_V_MSG_E(chunkDataSize >= 0, 0, "atom data size error");
        switch (GetAtomLevel(atom)) {
            case AtomLevel::LEVEL_0: {
                int32_t originalConfidence = confidence;
                confidence = GetFtypConfidence(dataReader, chunkDataOffset, chunkDataSize, confidence);
                FALSE_RETURN_V_NOLOG(!(confidence != FIRST_LEVEL_RANK || confidence == originalConfidence), confidence);
                break;
            }
            case AtomLevel::LEVEL_1: {
                return FIRST_LEVEL_RANK;
            }
            case AtomLevel::LEVEL_2: {
                FALSE_RETURN_V_MSG_E(!(__builtin_add_overflow(offset, atom.size, &moov.endOffset)), 0, "Add overflow");
                moov.offset = offset + sizeof(atom.type);
                moov.found = true;
                confidence = FIRST_LEVEL_RANK;
                break;
            }
            case AtomLevel::LEVEL_3: {
                confidence = std::max(confidence, SECOND_LEVEL_RANK);
                break;
            }
            case AtomLevel::LEVEL_4: {
                confidence = std::max(confidence, THIRD_LEVEL_RANK);
                break;
            }
            default:
                break;
        }
        FALSE_RETURN_V_MSG_E(!(__builtin_add_overflow(offset, atom.size, &offset)), 0, "Add overflow");
    }
    if (confidence > THIRD_LEVEL_RANK && moov.offset != -1) {
        offset = moov.offset;
        IsMPEGPS(offset, SNIFF_DATA_SIZE, dataReader, confidence);
    }
    return confidence;
}

int Sniff(const std::string& pluginName, std::shared_ptr<DataSource> source)
{
    std::string debug = OHOS::system::GetParameter("Mpeg4DemuxerPlugin.debug", "0");
    FALSE_RETURN_V_MSG_W(debug == "0", 0, "Skip Mpeg4Demuxer, debug");
    std::string enable = OHOS::system::GetParameter("Mpeg4DemuxerPlugin.enable", "0");
    FALSE_RETURN_V_MSG_W(enable == "1", 0, "Skip Mpeg4Demuxer");
    FALSE_RETURN_V_MSG_E(!pluginName.empty(), 0, "Plugin name is empty");
    FALSE_RETURN_V_MSG_E(pluginName == "avdemux_mpeg4", 0, "Plugin name is invalid");
    FALSE_RETURN_V_MSG_E(source != nullptr, 0, "source is nullptr");
    auto dataReader = std::make_shared<DemuxerDataReader>();
    FALSE_RETURN_V_MSG_E(dataReader != nullptr, 0, "dataReader allocation failed");
    dataReader->SetDataReader(source);
    int32_t confidence = GetConfidence(dataReader);
    MEDIA_LOG_I("Sniff score = " PUBLIC_LOG_D32, confidence);
    return confidence == 0 ? 0 : confidence + 1; // 适配自研优先
}

Status RegisterMpeg4Plugin(const std::shared_ptr<Register>& reg)
{
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(reg != nullptr, Status::ERROR_INVALID_PARAMETER, "Register is nullptr");
    std::lock_guard<std::mutex> lock(g_mtx);

    std::string pluginName = "avdemux_mpeg4";

    DemuxerPluginDef regInfo;
    regInfo.name = pluginName;
    regInfo.description = "mpeg4 demuxer plugin";
    regInfo.rank = RANK_MAX;
    regInfo.AddExtensions(
        {"mov", "mp4", "m4a", "3gp", "3g2", "mj2", "psp", "m4b", "ism", "ismv", "isma", "f4v", "avif", "heic", "heif"});
    auto func = [](const std::string& name) -> std::shared_ptr<DemuxerPlugin> {
        return std::make_shared<MPEG4DemuxerPlugin>(name);
    };
    regInfo.SetCreator(func);
    regInfo.SetSniffer(Sniff);
    auto ret = reg->AddPlugin(regInfo);
    if (ret != Status::OK) {
        MEDIA_LOG_E("Add plugin failed, err = " PUBLIC_LOG_D32, static_cast<int>(ret));
    } else {
        MEDIA_LOG_D("Add plugin " PUBLIC_LOG_S, pluginName.c_str());
    }
    
    return Status::OK;
}
} // namespace
PLUGIN_DEFINITION(Mpeg4Demuxer, LicenseType::LGPL, RegisterMpeg4Plugin, [] {});
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
