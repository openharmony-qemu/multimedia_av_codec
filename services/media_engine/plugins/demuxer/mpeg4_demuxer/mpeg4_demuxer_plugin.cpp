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
#include <algorithm>
#include <string>
#include <sstream>
#include <iomanip>
#include <fstream>
#include <limits>

#include "avcodec_trace.h"
#include "demuxer_utils.h"
#include "securec.h"
#include "converter.h"
#include "mpeg4_sample_helper.h"
#include "mpeg4_demuxer_plugin.h"
#include "mpeg4_format_checker.h"
#include "mpeg4_utils.h"
#include "meta/mime_type.h"
#include "meta/video_types.h"
#include "demuxer_log_compressor.h"
#include "syspara/parameters.h"
#include "xcollie/xcollie.h"
#include "xcollie/xcollie_define.h"

constexpr uint32_t HEX_CHARS_PER_BYTE = 2;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4DemuxerPlugin" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
const uint32_t SETTIMER_TIMEOUT = 5;
constexpr uint32_t DEFAULT_CACHE_LIMIT = 50 * 1024 * 1024;
constexpr int32_t DEF_PROBE_SCORE_LIMIT = 50;
constexpr uint32_t PROBE_SIZE = 5000000;
constexpr int32_t FIRST_LEVEL_RANK = 100;
constexpr int32_t SECOND_LEVEL_RANK = 95;
constexpr int32_t THIRD_LEVEL_RANK = 50;
constexpr int32_t RANK_MIN = 5;
constexpr int32_t RANK_MAX = 100 + 1; // 适配自研优先
constexpr int32_t READ_LEN = 8;
constexpr int32_t MIN_ATOM_SIZE = 16;
constexpr int64_t SNIFF_DATA_SIZE = 2048;

static const std::set<SeekMode>  g_seekMode = {
    SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_NEXT_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC,
};

static const std::map<std::string, VideoStreamType> g_streamParserMap = {
    { "video/avc",  VideoStreamType::AVC },
    { "video/hevc", VideoStreamType::HEVC },
    { "video/vvc",  VideoStreamType::VVC },
};

bool HaveValidParser(const std::string mime)
{
    return g_streamParserMap.count(mime) != 0;
}

bool NeedReadPostProcessing(const std::string& mime)
{
    return mime == MimeType::VIDEO_AVC || mime == MimeType::VIDEO_HEVC || mime == MimeType::VIDEO_VVC ||
        mime == MimeType::TEXT_WEBVTT;
}

constexpr int32_t PSSH_UUID_SIZE = 16;
constexpr int64_t US_TO_NS = 1000;
constexpr int64_t NS_TIME_BASE = 1000000000LL;
constexpr int64_t MS_TIME_SCALE = 1000LL;
constexpr uint32_t AAC_SAMPLES_PER_FRAME = 1024;
constexpr uint32_t SINGLE_SAMPLE_RUN_COUNT = 1;
constexpr uint32_t EDIT_LIST_ROUNDING_TOLERANCE_TICKS = 2;
constexpr uint32_t SHORT_TAIL_STTS_ENTRY_COUNT = 2;
constexpr uint32_t EDIT_START_DISCARD_STTS_ENTRY_COUNT = 3;
constexpr uint32_t MIN_TAIL_PACKET_COUNT = 2;
constexpr int64_t ROUND_HALF_DIVISOR = 2;
constexpr int32_t BOX_HEADER_SIZE = 8;
constexpr int32_t BOX_TYPE_OFFSET = 4;
constexpr uint8_t AVC_NAL_LENGTH_SIZE_OFFSET = 4;
constexpr uint8_t AVC_NAL_LENGTH_SIZE_MASK = 0x03;
constexpr uint8_t AVC_NAL_TYPE_MASK = 0x1F;
constexpr uint8_t AVC_IDR_NAL_TYPE = 5;
constexpr uint8_t AVC_MIN_NAL_LENGTH_SIZE = 1;
constexpr uint8_t AVC_MAX_NAL_LENGTH_SIZE = 4;
constexpr uint8_t BITS_PER_BYTE = 8;
constexpr int64_t MAX_AVC_IDR_SCAN_SIZE = 64 * 1024 * 1024;

struct ReadSamplePacketAttr {
    int64_t ptsUs = 0;
    int64_t dtsUs = 0;
    int64_t durationUs = 0;
    uint32_t flag = 0;
    std::vector<uint8_t> skipSamplesInfo;
    bool emit = true;
    uint32_t sourceSampleIndex = 0;
};

struct ReadSamplePacketContext {
    const MediaInfo& mediaInfo;
    const std::shared_ptr<MPEG4AtomParser::Track>& track;
    const MPEG4SampleHelper::SampleIndexView& currentSample;
    std::unordered_map<uint32_t, std::vector<int64_t>>& rawDtsPrefix;
    uint32_t sampleIndex = 0;
    int32_t trackTimeScale = 0;
    int32_t movieTimeScale = 1;
    bool hasPlayableVideoTrack = false;
};

struct ReadSampleEntryContext {
    const ReadSamplePacketContext& packet;
    const MPEG4SampleHelper::SampleIndexView& currentSample;
};

struct WebVttTailDurationContext {
    const MediaInfo& mediaInfo;
    const std::shared_ptr<MPEG4AtomParser::Track>& track;
    uint32_t trackId = 0;
    uint32_t nextSampleIndex = 0;
    int32_t movieTimeScale = 1;
};

bool SafeAddInt64(int64_t left, int64_t right, int64_t& result)
{
    return !__builtin_add_overflow(left, right, &result);
}

bool SafeSubInt64(int64_t left, int64_t right, int64_t& result)
{
    return !__builtin_sub_overflow(left, right, &result);
}

bool IsValidTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    return track != nullptr && track->sampleHelper != nullptr;
}

bool IsValidMpeg4Sample(const std::shared_ptr<MPEG4Sample>& mpeg4Sample)
{
    return mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr;
}

bool GetSampleView(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex,
    MPEG4SampleHelper::SampleIndexView& view)
{
    return IsValidTrack(track) &&
        track->sampleHelper->GetSampleIndexView(sampleIndex, view) == Status::OK;
}

Status RescaleRoundNearest(int64_t value, int64_t multiplier, int64_t divisor, int64_t& result)
{
    if (divisor <= 0 || multiplier <= 0) {
        return Status::ERROR_INVALID_DATA;
    }
    bool negative = value < 0;
    uint64_t absValue = negative ? static_cast<uint64_t>(-(value + 1)) + 1 : static_cast<uint64_t>(value);
    uint64_t divisorAbs = static_cast<uint64_t>(divisor);
    uint64_t multiplierAbs = static_cast<uint64_t>(multiplier);
    uint64_t quotient = absValue / divisorAbs;
    uint64_t remainder = absValue % divisorAbs;
    FALSE_RETURN_V_NOLOG(quotient <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) / multiplierAbs,
        Status::ERROR_INVALID_DATA);
    // Scale quotient and remainder separately to avoid overflowing value * multiplier.
    uint64_t quotientScaled = quotient * multiplierAbs;
    uint64_t remainderScaled = remainder * multiplierAbs;
    uint64_t roundedRemainder = (remainderScaled + divisorAbs / static_cast<uint64_t>(ROUND_HALF_DIVISOR)) /
        divisorAbs;
    uint64_t total = quotientScaled + roundedRemainder;
    if (total < quotientScaled) {
        return Status::ERROR_INVALID_DATA;
    }
    constexpr uint64_t int64MinAbs = static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) + 1;
    if ((!negative && total > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) ||
        (negative && total > int64MinAbs)) {
        return Status::ERROR_INVALID_DATA;
    }
    if (negative && total == int64MinAbs) {
        result = std::numeric_limits<int64_t>::min();
    } else {
        result = negative ? -static_cast<int64_t>(total) : static_cast<int64_t>(total);
    }
    return Status::OK;
}

Status RescaleTrackTimeToNsRoundNearest(int64_t value, int64_t timeScale, int64_t& result)
{
    FALSE_RETURN_V_NOLOG(timeScale > 0 && timeScale <= std::numeric_limits<int32_t>::max(),
        Status::ERROR_INVALID_DATA);
    return RescaleRoundNearest(value, NS_TIME_BASE, timeScale, result);
}

Status RescaleTrackTimeToUsRoundNearest(int64_t value, int64_t timeScale, int64_t& result)
{
    int64_t ns = 0;
    FALSE_RETURN_V_MSG_E(RescaleTrackTimeToNsRoundNearest(value, timeScale, ns) == Status::OK,
        Status::ERROR_INVALID_DATA, "Rescale to round-nearest ns failed");
    result = ns / US_TO_NS;
    return Status::OK;
}

bool IsVideoLikeTrackForDuration(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    FALSE_RETURN_V_NOLOG(IsValidTrack(track), false);
    return track->codecParms.trackType == VIDEO_TYPE ||
        (track->codecParms.trackType == AUXILIARY_TYPE && IsVideoMimeType(track->sampleHelper->mimeType_));
}

bool NeedsCttsFrameRateDuration(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    return IsVideoLikeTrackForDuration(track) && !track->sampleHelper->cttsEntry_.empty();
}

bool HasUntimedTrailingZeroAacSample(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    if (!IsValidTrack(track) || track->codecParms.trackType != AUDIO_TYPE ||
        track->sampleHelper->mimeType_ != MimeType::AUDIO_AAC ||
        track->sampleHelper->GetSampleCount() <= static_cast<size_t>(track->sampleHelper->GetRawSampleCount())) {
        return false;
    }
    MPEG4SampleHelper::SampleIndexView trailingSample;
    return GetSampleView(track, static_cast<uint32_t>(track->sampleHelper->GetSampleCount() - 1), trailingSample) &&
        trailingSample.size == 0 && trailingSample.duration == 0;
}

bool GetCttsFrameRateDurationUs(
    const MediaInfo& mediaInfo, uint32_t trackId, int32_t trackTimeScale, int64_t& durationUs)
{
    double frameRate = 0.0;
    FALSE_RETURN_V_NOLOG(mediaInfo.tracks[trackId].Get<Tag::VIDEO_FRAME_RATE>(frameRate) &&
        std::isfinite(frameRate) && frameRate > 0 && trackTimeScale > 0, false);

    // CTTS packet durations can be unavailable after sample table processing. Derive duration from r_frame_rate
    // and round down in stream time_base to keep ReadSample packet attributes stable.
    double durationTicks = static_cast<double>(trackTimeScale) / frameRate;
    FALSE_RETURN_V_NOLOG(std::isfinite(durationTicks) && durationTicks > 0 &&
        durationTicks <= static_cast<double>(std::numeric_limits<int64_t>::max()), false);
    int64_t fallbackDurationTicks = static_cast<int64_t>(durationTicks);
    FALSE_RETURN_V_NOLOG(fallbackDurationTicks > 0, false);
    return RescaleTrackTimeToUsRoundNearest(fallbackDurationTicks, trackTimeScale, durationUs) == Status::OK;
}

int64_t GetEffectiveStreamDurationTicks(
    const std::shared_ptr<MPEG4AtomParser::Track>& track, int32_t trackTimeScale, int32_t movieTimeScale)
{
    bool isVideoLikeTrack = IsVideoLikeTrackForDuration(track);
    bool hasInitialEmptyEdit = isVideoLikeTrack && !track->editListEntries.empty() &&
        track->editListEntries.front().mediaTime == -1;
    int64_t streamDuration = 0;
    if (track->duration > 0) {
        streamDuration = track->duration;
    }
    if (!HasUntimedTrailingZeroAacSample(track)) {
        int64_t sampleDuration = hasInitialEmptyEdit && track->sampleHelper->GetRawTotalDuration() > 0 ?
            track->sampleHelper->GetRawTotalDuration() : track->sampleHelper->totalDuration_;
        if (sampleDuration > 0) {
            streamDuration = streamDuration > 0 ? std::min(streamDuration, sampleDuration) : sampleDuration;
        }
        int64_t rawSampleDuration = track->sampleHelper->GetRawTotalDuration();
        if (rawSampleDuration > 0) {
            streamDuration = streamDuration > 0 ? std::min(streamDuration, rawSampleDuration) : rawSampleDuration;
        }
    }
    // Initial empty edit shifts the visible video range; remove it from packet tail duration.
    if (streamDuration > 0 && isVideoLikeTrack && !hasInitialEmptyEdit && track->elstInitEmptyEdit > 0) {
        int64_t emptyEditDuration = 0;
        // LCOV_EXCL_START
        if (!track->editListEntries.empty() && track->editListEntries.front().mediaTime == -1 &&
            movieTimeScale > 0) {
            (void)ConvertTimeScale(track->editListEntries.front().duration, trackTimeScale, movieTimeScale,
                &emptyEditDuration);
        }
        if (emptyEditDuration <= 0) {
            (void)ConvertTimeScale(track->elstInitEmptyEdit, trackTimeScale, MS_TIME_SCALE, &emptyEditDuration);
        }
        if (emptyEditDuration > 0 && streamDuration > emptyEditDuration) {
            streamDuration -= emptyEditDuration;
        }
        // LCOV_EXCL_STOP
    }
    return streamDuration;
}

bool GetPacketDurationUs(const ReadSamplePacketContext& context, int64_t& durationUs)
{
    const auto& track = context.track;
    uint32_t sampleIndex = context.sampleIndex;
    int32_t trackTimeScale = context.trackTimeScale;
    FALSE_RETURN_V_NOLOG(IsValidTrack(track), false);
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    const auto& currentSample = context.currentSample;
    FALSE_RETURN_V_NOLOG(sampleCount > 0, false);
    if (NeedsCttsFrameRateDuration(track)) {
        return GetCttsFrameRateDurationUs(context.mediaInfo, track->trackIndex, trackTimeScale, durationUs);
    }

    int64_t duration = 0;
    if (sampleIndex + 1 < sampleCount) {
        // For normal packets, duration follows the distance to the next decode timestamp.
        MPEG4SampleHelper::SampleIndexView nextSample;
        FALSE_RETURN_V_NOLOG(GetSampleView(track, sampleIndex + 1, nextSample), false);
        duration = nextSample.dts - currentSample.dts;
        FALSE_RETURN_V_NOLOG(duration > 0, false);
    } else {
        // The final packet has no next DTS, so close it against the effective stream duration.
        int64_t streamDuration = GetEffectiveStreamDurationTicks(track, trackTimeScale, context.movieTimeScale);
        if (streamDuration < currentSample.dts && currentSample.duration != 0) {
            durationUs = 0;
            return true;
        }
        if (streamDuration <= currentSample.dts) {
            FALSE_RETURN_V_NOLOG(sampleIndex != 0, false);
            MPEG4SampleHelper::SampleIndexView previousSample;
            FALSE_RETURN_V_NOLOG(GetSampleView(track, sampleIndex - 1, previousSample), false);
            duration = currentSample.dts - previousSample.dts;
            FALSE_RETURN_V_NOLOG(duration > 0, false);
        } else {
            duration = streamDuration - currentSample.dts;
        }
    }
    return RescaleTrackTimeToUsRoundNearest(duration, trackTimeScale, durationUs) == Status::OK;
}

bool GetMpegAudioFrameDurationUs(const ReadSamplePacketContext& context, int64_t& durationUs)
{
    const auto& mediaInfo = context.mediaInfo;
    const auto& track = context.track;
    uint32_t trackId = track == nullptr ? 0 : track->trackIndex;
    if (track == nullptr || track->codecParms.trackType != AUDIO_TYPE ||
        track->sampleHelper == nullptr || track->sampleHelper->mimeType_ != MimeType::AUDIO_MPEG ||
        trackId >= mediaInfo.tracks.size()) {
        return false;
    }
    int32_t sampleRate = 0;
    int32_t samplesPerFrame = 0;
    if (!mediaInfo.tracks[trackId].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate) || sampleRate <= 0 ||
        !mediaInfo.tracks[trackId].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame) || samplesPerFrame <= 0) {
        return false;
    }
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    const auto& currentSample = context.currentSample;
    uint32_t sampleIndex = context.sampleIndex;
    FALSE_RETURN_V_NOLOG(sampleCount > 0, false);
    // A short final STTS run means the container already records a real tail duration.
    bool hasExplicitShortTailStts = track->sampleHelper->GetSttsEntryCount() == SHORT_TAIL_STTS_ENTRY_COUNT &&
        track->sampleHelper->GetSampleCountOfSttsEntry(1) == SINGLE_SAMPLE_RUN_COUNT &&
        track->sampleHelper->GetSampleDeltaOfSttsEntry(0) > track->sampleHelper->GetSampleDeltaOfSttsEntry(1);
    if (hasExplicitShortTailStts && sampleIndex + 1 == sampleCount && currentSample.duration > 0 &&
        currentSample.duration != samplesPerFrame) {
        return false;
    }
    return ConvertTimeScale(samplesPerFrame, static_cast<int64_t>(S_TO_US), sampleRate, &durationUs) == Status::OK;
}

bool GetAacFrameDurationUs(const ReadSamplePacketContext& context, int64_t& durationUs)
{
    const auto& track = context.track;
    uint32_t trackId = track == nullptr ? 0 : track->trackIndex;
    if (track == nullptr || track->codecParms.trackType != AUDIO_TYPE || track->sampleHelper == nullptr ||
        track->sampleHelper->mimeType_ != MimeType::AUDIO_AAC || trackId >= context.mediaInfo.tracks.size() ||
        context.trackTimeScale <= 0) {
        return false;
    }
    int32_t sampleRate = 0;
    int32_t samplesPerFrame = 0;
    if (!context.mediaInfo.tracks[trackId].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate) || sampleRate <= 0 ||
        !context.mediaInfo.tracks[trackId].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame) ||
        samplesPerFrame <= 0) {
        return false;
    }

    // Match the stream time base first, then use the packet timestamp conversion used by other durations.
    int64_t durationTicks = 0;
    FALSE_RETURN_V_NOLOG(ConvertTimeScale(samplesPerFrame, context.trackTimeScale, sampleRate, &durationTicks) ==
        Status::OK && durationTicks > 0, false);
    return RescaleTrackTimeToUsRoundNearest(durationTicks, context.trackTimeScale, durationUs) == Status::OK &&
        durationUs > 0;
}

bool IsSingleEditShortTailAudio(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    FALSE_RETURN_V_NOLOG(IsValidTrack(track) && track->codecParms.trackType == AUDIO_TYPE &&
        track->editListEntries.size() == 1 &&
        track->editListEntries.front().mediaTime > 0 &&
        track->sampleHelper->GetSttsEntryCount() == SHORT_TAIL_STTS_ENTRY_COUNT, false);
    uint32_t firstCount = track->sampleHelper->GetSampleCountOfSttsEntry(0);
    uint32_t secondCount = track->sampleHelper->GetSampleCountOfSttsEntry(1);
    uint32_t firstDelta = track->sampleHelper->GetSampleDeltaOfSttsEntry(0);
    uint32_t secondDelta = track->sampleHelper->GetSampleDeltaOfSttsEntry(1);
    FALSE_RETURN_V_NOLOG(secondCount == SINGLE_SAMPLE_RUN_COUNT && firstDelta > secondDelta &&
        firstDelta - secondDelta <= EDIT_LIST_ROUNDING_TOLERANCE_TICKS, false);
    int64_t rawSampleCount = track->sampleHelper->GetRawSampleCount();
    uint64_t totalCount = static_cast<uint64_t>(firstCount) + static_cast<uint64_t>(secondCount);
    return rawSampleCount > 0 && totalCount == static_cast<uint64_t>(rawSampleCount);
}

bool HasAacShortTailStts(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    if (track == nullptr || track->codecParms.trackType != AUDIO_TYPE || track->sampleHelper == nullptr ||
        track->sampleHelper->GetSttsEntryCount() != SHORT_TAIL_STTS_ENTRY_COUNT) {
        return false;
    }
    uint32_t firstCount = track->sampleHelper->GetSampleCountOfSttsEntry(0);
    uint32_t secondCount = track->sampleHelper->GetSampleCountOfSttsEntry(1);
    uint32_t firstDelta = track->sampleHelper->GetSampleDeltaOfSttsEntry(0);
    uint32_t secondDelta = track->sampleHelper->GetSampleDeltaOfSttsEntry(1);
    if (secondCount != SINGLE_SAMPLE_RUN_COUNT || firstDelta != AAC_SAMPLES_PER_FRAME || firstDelta <= secondDelta ||
        firstDelta - secondDelta > EDIT_LIST_ROUNDING_TOLERANCE_TICKS) {
        return false;
    }
    int64_t rawSampleCount = track->sampleHelper->GetRawSampleCount();
    uint64_t totalCount = static_cast<uint64_t>(firstCount) + static_cast<uint64_t>(secondCount);
    return rawSampleCount > 0 && totalCount == static_cast<uint64_t>(rawSampleCount);
}

bool HasEditStartDiscardAudioStts(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    FALSE_RETURN_V_NOLOG(!HasAacShortTailStts(track), true);
    if (track == nullptr || track->codecParms.trackType != AUDIO_TYPE || track->sampleHelper == nullptr ||
        track->sampleHelper->GetSttsEntryCount() != EDIT_START_DISCARD_STTS_ENTRY_COUNT) {
        return false;
    }
    uint32_t firstCount = track->sampleHelper->GetSampleCountOfSttsEntry(0);
    uint32_t secondCount = track->sampleHelper->GetSampleCountOfSttsEntry(1);
    uint32_t thirdCount = track->sampleHelper->GetSampleCountOfSttsEntry(2);
    uint32_t firstDelta = track->sampleHelper->GetSampleDeltaOfSttsEntry(0);
    uint32_t secondDelta = track->sampleHelper->GetSampleDeltaOfSttsEntry(1);
    uint32_t thirdDelta = track->sampleHelper->GetSampleDeltaOfSttsEntry(2);
    if (firstCount != SINGLE_SAMPLE_RUN_COUNT || secondCount != SINGLE_SAMPLE_RUN_COUNT ||
        firstDelta != AAC_SAMPLES_PER_FRAME || thirdDelta != AAC_SAMPLES_PER_FRAME || secondDelta <= firstDelta) {
        return false;
    }
    int64_t rawSampleCount = track->sampleHelper->GetRawSampleCount();
    uint64_t totalCount = static_cast<uint64_t>(firstCount) + static_cast<uint64_t>(secondCount) +
        static_cast<uint64_t>(thirdCount);
    return rawSampleCount > 0 && totalCount == static_cast<uint64_t>(rawSampleCount);
}

bool IsWebVttCueSample(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex)
{
    FALSE_RETURN_V_NOLOG(IsValidTrack(track) &&
        track->sampleHelper->mimeType_ == MimeType::TEXT_WEBVTT && track->sampleHelper->dataReader_ != nullptr, false);
    int64_t samplePos = 0;
    int32_t sampleSize = 0;
    FALSE_RETURN_V_NOLOG(track->sampleHelper->GetSampleRange(sampleIndex, samplePos, sampleSize) == Status::OK &&
        sampleSize >= BOX_HEADER_SIZE, false);
    // WebVTT sample payload starts with an ISO BMFF box header; cue samples use vttc.
    uint8_t header[BOX_HEADER_SIZE] = {0};
    if (track->sampleHelper->dataReader_->ReadUintData(samplePos, header, sizeof(header)) !=
        Status::OK) {
        return false;
    }
    return GetU32Value(header + BOX_TYPE_OFFSET) == static_cast<uint32_t>(FourccType("vttc"));
}

bool HasFutureWebVttCueSample(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t startIndex)
{
    FALSE_RETURN_V_NOLOG(IsValidTrack(track) &&
        track->sampleHelper->mimeType_ == MimeType::TEXT_WEBVTT, false);
    for (uint32_t index = startIndex; index < track->sampleHelper->GetSampleCount(); ++index) {
        FALSE_RETURN_V_NOLOG(!IsWebVttCueSample(track, index), true);
    }
    return false;
}

bool TryUpdateWebVttTailDurationFromMedia(const WebVttTailDurationContext& context,
    const std::shared_ptr<Sample>& sample)
{
    int64_t mediaDurationUs = 0;
    if (context.mediaInfo.general.Get<Tag::MEDIA_DURATION>(mediaDurationUs) && mediaDurationUs > sample->pts) {
        int64_t tailDuration = mediaDurationUs - sample->pts;
        if (tailDuration > sample->duration) {
            sample->duration = tailDuration;
            return true;
        }
    }
    return false;
}

bool TryUpdateWebVttTailDurationFromTrack(const WebVttTailDurationContext& context,
    const std::shared_ptr<Sample>& sample)
{
    const auto& track = context.track;
    int32_t trackTimeScale = track->timeScale;
    FALSE_RETURN_V_NOLOG(context.trackId < context.mediaInfo.tracks.size() && trackTimeScale > 0, false);
    int64_t trackDuration = GetEffectiveStreamDurationTicks(track, trackTimeScale, context.movieTimeScale);
    int64_t trackDurationUs = 0;
    FALSE_RETURN_V_NOLOG(trackDuration > 0 &&
        RescaleTrackTimeToUsRoundNearest(trackDuration, trackTimeScale, trackDurationUs) == Status::OK &&
        trackDurationUs > sample->pts, false);
    int64_t tailDuration = trackDurationUs - sample->pts;
    if (tailDuration > sample->duration) {
        sample->duration = tailDuration;
        return true;
    }
    return false;
}

bool UpdateWebVttTailDurationFromStream(const WebVttTailDurationContext& context, const std::shared_ptr<Sample>& sample)
{
    const auto& track = context.track;
    if (track == nullptr || track->sampleHelper == nullptr || sample == nullptr ||
        track->sampleHelper->mimeType_ != MimeType::TEXT_WEBVTT ||
        HasFutureWebVttCueSample(track, context.nextSampleIndex)) {
        return false;
    }
    return TryUpdateWebVttTailDurationFromMedia(context, sample) ||
        TryUpdateWebVttTailDurationFromTrack(context, sample);
}

void UpdateWebVttFinalPacketDurationFromMedia(const MediaInfo& mediaInfo,
    const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex, ReadSamplePacketAttr& attr)
{
    if (track == nullptr || track->sampleHelper == nullptr ||
        track->sampleHelper->mimeType_ != MimeType::TEXT_WEBVTT ||
        sampleIndex + 1 != track->sampleHelper->GetSampleCount()) {
        return;
    }
    int64_t mediaDurationUs = 0;
    FALSE_RETURN_NOLOG(mediaInfo.general.Get<Tag::MEDIA_DURATION>(mediaDurationUs) && mediaDurationUs > attr.ptsUs);

    // MOV/MP4's WebVTT stream duration is expressed in the text stream timebase (1 ms here).
    mediaDurationUs = ((mediaDurationUs + MS_TO_US / ROUND_HALF_DIVISOR) / MS_TO_US) * MS_TO_US;
    int64_t tailDuration = mediaDurationUs - attr.ptsUs;
    if (tailDuration > attr.durationUs) {
        attr.durationUs = tailDuration;
    }
}

bool GetSingleEditShortTailAudioDurationUs(const ReadSamplePacketContext& context, int64_t& durationUs)
{
    const auto& track = context.track;
    if (!IsSingleEditShortTailAudio(track)) {
        return false;
    }
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    uint32_t sampleIndex = context.sampleIndex;
    if (sampleCount < MIN_TAIL_PACKET_COUNT || sampleIndex + 1 != sampleCount) {
        return false;
    }
    MPEG4SampleHelper::SampleIndexView previousSample;
    FALSE_RETURN_V_NOLOG(GetSampleView(track, sampleIndex - 1, previousSample), false);
    const auto& currentSample = context.currentSample;
    int64_t previousDuration = currentSample.dts - previousSample.dts;
    FALSE_RETURN_V_NOLOG(currentSample.duration > 0 && previousDuration > 0, false);
    int64_t duration = context.hasPlayableVideoTrack ? currentSample.duration :
        currentSample.duration + previousDuration;
    return RescaleTrackTimeToUsRoundNearest(duration, context.trackTimeScale, durationUs) == Status::OK;
}

bool GetTrailingZeroEditedAudioDurationUs(const ReadSampleEntryContext& context, int64_t& durationUs)
{
    const auto& packet = context.packet;
    const auto& track = packet.track;
    const auto& currentSample = context.currentSample;
    uint32_t sampleIndex = packet.sampleIndex;
    if (!track->sampleHelper->HasTrailingZeroSttsDuration() || sampleIndex == 0) {
        return false;
    }
    MPEG4SampleHelper::SampleIndexView previousSample;
    FALSE_RETURN_V_NOLOG(GetSampleView(track, sampleIndex - 1, previousSample), false);
    // Trailing zero STTS entries borrow the previous packet spacing unless stream tail is shorter.
    int64_t previousDuration = currentSample.dts - previousSample.dts;
    FALSE_RETURN_V_NOLOG(previousDuration > 0, false);
    int64_t streamDuration = GetEffectiveStreamDurationTicks(track, packet.trackTimeScale, packet.movieTimeScale);
    int64_t tailDuration = streamDuration - currentSample.dts;
    FALSE_RETURN_V_NOLOG(tailDuration <= 0 || tailDuration >= previousDuration,
        RescaleTrackTimeToUsRoundNearest(tailDuration, packet.trackTimeScale, durationUs) == Status::OK);
    return RescaleTrackTimeToUsRoundNearest(previousDuration, packet.trackTimeScale, durationUs) == Status::OK;
}

bool GetSkipInfoEditedAudioDurationUs(
    const ReadSampleEntryContext& context, int64_t rawTailDuration, int64_t& durationUs)
{
    const auto& currentSample = context.currentSample;
    const auto& track = context.packet.track;
    uint32_t sampleIndex = context.packet.sampleIndex;
    // Edit-list audio with skip samples may need to merge the previous packet spacing into the tail.
    if (rawTailDuration != currentSample.duration || track == nullptr || track->sampleHelper == nullptr ||
        !track->sampleHelper->HasSkipSamplesInfo(0) || sampleIndex == 0) {
        return false;
    }
    MPEG4SampleHelper::SampleIndexView previousSample;
    FALSE_RETURN_V_NOLOG(GetSampleView(track, sampleIndex - 1, previousSample), false);
    int64_t previousDuration = currentSample.dts - previousSample.dts;
    FALSE_RETURN_V_NOLOG(previousDuration > 0, false);
    int64_t duration = currentSample.duration + previousDuration;
    return RescaleTrackTimeToUsRoundNearest(duration, context.packet.trackTimeScale, durationUs) == Status::OK;
}

bool GetEditedAudioTailDurationUs(const ReadSamplePacketContext& context, int64_t& durationUs)
{
    const auto& track = context.track;
    if (track == nullptr || track->codecParms.trackType != AUDIO_TYPE || track->sampleHelper == nullptr ||
        track->sampleHelper->mimeType_ == MimeType::AUDIO_MPEG || track->elstInitEmptyEdit <= 0) {
        return false;
    }
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    if (sampleCount == 0 || context.sampleIndex + 1 != sampleCount) {
        return false;
    }

    const auto& currentSample = context.currentSample;
    ReadSampleEntryContext entryContext { context, currentSample };
    FALSE_RETURN_V_NOLOG(!(GetTrailingZeroEditedAudioDurationUs(entryContext, durationUs)), true);
    FALSE_RETURN_V_NOLOG(currentSample.duration > 0, false);
    int64_t rawTotalDuration = track->sampleHelper->GetRawTotalDuration();
    int64_t rawTailDuration = rawTotalDuration - currentSample.dts;
    FALSE_RETURN_V_NOLOG(!(GetSkipInfoEditedAudioDurationUs(entryContext, rawTailDuration, durationUs)), true);
    FALSE_RETURN_V_NOLOG(rawTailDuration <= 0, false);
    int64_t duration = currentSample.duration;
    if (context.sampleIndex > 0) {
        MPEG4SampleHelper::SampleIndexView previousSample;
        FALSE_RETURN_V_NOLOG(GetSampleView(track, context.sampleIndex - 1, previousSample), false);
        int64_t previousDuration = currentSample.dts - previousSample.dts;
        if (previousDuration > 0 && rawTailDuration < 0) {
            duration = previousDuration;
        }
    }
    return RescaleTrackTimeToUsRoundNearest(duration, context.trackTimeScale, durationUs) == Status::OK;
}

bool GetTimedMetaTailDurationUs(
    const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex, int64_t& durationUs)
{
    if (track == nullptr || track->codecParms.trackType != TIMEDMETA_TYPE || track->sampleHelper == nullptr ||
        track->sampleHelper->mimeType_ == MimeType::TEXT_WEBVTT) {
        return false;
    }
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    if (sampleCount == 0 || sampleIndex + 1 != sampleCount ||
        !track->sampleHelper->HasTrailingZeroSttsDuration()) {
        return false;
    }
    int64_t rawDuration = track->sampleHelper->GetRawTotalDuration();
    FALSE_RETURN_V_NOLOG(rawDuration <= 0 || track->duration <= 0 || rawDuration <= track->duration, false);
    durationUs = 0;
    return true;
}

bool GetNominalSttsFrameDurationUs(const std::shared_ptr<MPEG4AtomParser::Track>& track, int32_t trackTimeScale,
    int64_t& durationUs)
{
    if (track == nullptr || track->sampleHelper == nullptr || trackTimeScale <= 0 ||
        !track->sampleHelper->HasTrailingZeroSttsDuration()) {
        return false;
    }

    // Use the smallest non-zero STTS delta as the nominal frame spacing for zero-duration tails.
    int64_t minDeltaTicks = 0;
    uint32_t entryCount = track->sampleHelper->GetSttsEntryCount();
    for (uint32_t i = 0; i < entryCount; ++i) {
        int64_t deltaTicks = static_cast<int64_t>(track->sampleHelper->GetSampleDeltaOfSttsEntry(i));
        if (deltaTicks == 0) {
            continue;
        }
        minDeltaTicks = minDeltaTicks == 0 ? deltaTicks : std::min(minDeltaTicks, deltaTicks);
    }
    FALSE_RETURN_V_NOLOG(minDeltaTicks != 0, false);

    int64_t timeScale = static_cast<int64_t>(trackTimeScale);
    int64_t frameRate = (timeScale + minDeltaTicks / ROUND_HALF_DIVISOR) / minDeltaTicks;
    FALSE_RETURN_V_NOLOG(frameRate != 0, false);
    int64_t nominalDuration = timeScale / frameRate;
    FALSE_RETURN_V_NOLOG(nominalDuration > 0, false);
    return RescaleTrackTimeToUsRoundNearest(nominalDuration, timeScale, durationUs) == Status::OK;
}

bool GetVideoTailDurationUs(const ReadSamplePacketContext& context, int64_t& durationUs)
{
    const auto& track = context.track;
    if (!IsVideoLikeTrackForDuration(track) || !track->sampleHelper->cttsEntry_.empty()) {
        return false;
    }
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    uint32_t sampleIndex = context.sampleIndex;
    if (sampleCount < MIN_TAIL_PACKET_COUNT || sampleIndex + 1 != sampleCount) {
        return false;
    }

    MPEG4SampleHelper::SampleIndexView previousSample;
    FALSE_RETURN_V_NOLOG(GetSampleView(track, sampleIndex - 1, previousSample), false);
    const auto& currentSample = context.currentSample;
    int64_t streamDuration = GetEffectiveStreamDurationTicks(track, context.trackTimeScale, context.movieTimeScale);
    if (GetNominalSttsFrameDurationUs(track, context.trackTimeScale, durationUs)) {
        return true;
    }
    if (currentSample.duration != 0 && streamDuration > currentSample.dts) {
        return false;
    }
    int64_t duration = currentSample.duration;
    if (duration <= 0 || streamDuration <= currentSample.dts) {
        duration = currentSample.dts - previousSample.dts;
    }
    FALSE_RETURN_V_NOLOG(duration > 0, false);
    return RescaleTrackTimeToUsRoundNearest(duration, context.trackTimeScale, durationUs) == Status::OK;
}

uint32_t ReadNalLength(const uint8_t* data, uint8_t nalLengthSize)
{
    uint32_t nalLength = 0;
    for (uint8_t i = 0; i < nalLengthSize; ++i) {
        nalLength = (nalLength << BITS_PER_BYTE) | data[i];
    }
    return nalLength;
}

uint8_t GetAvcNalLengthSize(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    FALSE_RETURN_V_NOLOG(track != nullptr && track->codecParms.data != nullptr &&
        track->codecParms.extradataSize > AVC_NAL_LENGTH_SIZE_OFFSET, 0);
    uint8_t nalLengthSize = (track->codecParms.data[AVC_NAL_LENGTH_SIZE_OFFSET] & AVC_NAL_LENGTH_SIZE_MASK) + 1;
    return nalLengthSize >= AVC_MIN_NAL_LENGTH_SIZE && nalLengthSize <= AVC_MAX_NAL_LENGTH_SIZE ? nalLengthSize : 0;
}

bool AvcSampleContainsIdr(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    const MPEG4SampleHelper::SampleIndexView& sample)
{
    if (track == nullptr || track->sampleHelper == nullptr || track->sampleHelper->dataReader_ == nullptr ||
        track->sampleHelper->mimeType_ != MimeType::VIDEO_AVC || sample.pos < 0 || sample.size <= 0 ||
        sample.size > MAX_AVC_IDR_SCAN_SIZE) {
        return false;
    }
    uint8_t nalLengthSize = GetAvcNalLengthSize(track);
    FALSE_RETURN_V_NOLOG(nalLengthSize != 0 && sample.size >= nalLengthSize, false);
    int64_t sampleEnd = 0;
    FALSE_RETURN_V_NOLOG(SafeAddInt64(sample.pos, sample.size, sampleEnd), false);
    // AVC samples are length-prefixed; read only NAL size and header to avoid keeping large samples in memory.
    int64_t offset = sample.pos;
    uint8_t lengthData[AVC_MAX_NAL_LENGTH_SIZE] = {0};
    uint8_t nalHeader = 0;
    while (offset <= sampleEnd - nalLengthSize) {
        FALSE_RETURN_V_NOLOG(
            track->sampleHelper->dataReader_->ReadUintData(offset, lengthData, nalLengthSize) == Status::OK, false);
        uint32_t nalLength = ReadNalLength(lengthData, nalLengthSize);
        FALSE_RETURN_V_NOLOG(SafeAddInt64(offset, nalLengthSize, offset), false);
        FALSE_RETURN_V_NOLOG(nalLength != 0 && nalLength <= static_cast<uint64_t>(sampleEnd - offset), false);
        FALSE_RETURN_V_NOLOG(
            track->sampleHelper->dataReader_->ReadUintData(offset, &nalHeader, sizeof(nalHeader)) == Status::OK, false);
        FALSE_RETURN_V_NOLOG((nalHeader & AVC_NAL_TYPE_MASK) != AVC_IDR_NAL_TYPE, true);
        FALSE_RETURN_V_NOLOG(SafeAddInt64(offset, static_cast<int64_t>(nalLength), offset), false);
    }
    return false;
}

bool ShouldApplyMovEditTimelineOffset(const std::shared_ptr<MPEG4AtomParser::Track>& track);

bool ShouldMarkAvcIdrSampleKey(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    const MPEG4SampleHelper::SampleIndexView& currentSample)
{
    if (track == nullptr || track->codecParms.trackType != VIDEO_TYPE ||
        (currentSample.flag & Sample::SampleFlag::SYNC_FRAME) != 0) {
        return false;
    }
    return AvcSampleContainsIdr(track, currentSample);
}

uint32_t GetSkipSamplesFromInfo(const std::vector<uint8_t>& skipSamplesInfo)
{
    FALSE_RETURN_V_NOLOG(skipSamplesInfo.size() >= sizeof(uint32_t), 0);
    uint32_t skipSamples = 0;
    for (size_t i = 0; i < sizeof(uint32_t); ++i) {
        // Skip samples side data is stored as little-endian 32-bit sample count.
        skipSamples |= static_cast<uint32_t>(skipSamplesInfo[i]) << (i * BITS_PER_BYTE);
    }
    return skipSamples;
}

bool ShouldClearAudioDiscardForSkipPacket(
    const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex,
    const MPEG4SampleHelper::SampleIndexView& currentSample, bool hasPlayableVideoTrack)
{
    const auto* skipSamplesInfo = IsValidTrack(track) ?
        track->sampleHelper->GetSkipSamplesInfo(sampleIndex) : nullptr;
    if (track == nullptr || track->codecParms.trackType != AUDIO_TYPE || track->sampleHelper == nullptr ||
        skipSamplesInfo == nullptr) {
        return false;
    }
    uint32_t skipSamples = GetSkipSamplesFromInfo(*skipSamplesInfo);
    FALSE_RETURN_V_NOLOG((skipSamples != 0 && currentSample.duration > 0), false);
    if (sampleIndex == 0 && skipSamples >= static_cast<uint64_t>(currentSample.duration) &&
        IsSingleEditShortTailAudio(track) && !hasPlayableVideoTrack) {
        return true;
    }
    if (skipSamples >= static_cast<uint64_t>(currentSample.duration)) {
        return false;
    }
    return sampleIndex > 0;
}

uint32_t GetPacketSampleFlag(const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex,
    const MPEG4SampleHelper::SampleIndexView& currentSample, bool hasPlayableVideoTrack)
{
    uint32_t flag = currentSample.flag;
    if (ShouldMarkAvcIdrSampleKey(track, currentSample)) {
        flag |= static_cast<uint32_t>(Sample::SampleFlag::SYNC_FRAME);
    }
    if (sampleIndex == 0 && IsValidTrack(track) && track->codecParms.trackType == AUDIO_TYPE &&
        track->editListEntries.size() == 1 &&
        track->editListEntries.front().mediaTime > 0) {
        if (track->sampleHelper->mimeType_ == MimeType::AUDIO_MPEG &&
            !ShouldApplyMovEditTimelineOffset(track) && !HasEditStartDiscardAudioStts(track)) {
            flag &= ~static_cast<uint32_t>(Sample::SampleFlag::DISCARD);
        } else {
            flag |= static_cast<uint32_t>(Sample::SampleFlag::DISCARD);
        }
    }
    if (ShouldClearAudioDiscardForSkipPacket(track, sampleIndex, currentSample, hasPlayableVideoTrack)) {
        flag &= ~static_cast<uint32_t>(Sample::SampleFlag::DISCARD);
    }
    if (IsValidTrack(track) && track->codecParms.trackType == AUDIO_TYPE &&
        ShouldApplyMovEditTimelineOffset(track) && sampleIndex + 1 == track->sampleHelper->GetSampleCount()) {
        flag &= ~static_cast<uint32_t>(Sample::SampleFlag::DISCARD);
    }
    return flag;
}

// LCOV_EXCL_START
bool GetMovEditTimelineOffsetTicks(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    int32_t trackTimeScale, int32_t movieTimeScale, int64_t& timeOffset)
{
    timeOffset = 0;
    FALSE_RETURN_V_NOLOG(track != nullptr && !(track->editListEntries.empty()) && trackTimeScale > 0, true);
    FALSE_RETURN_V_NOLOG(movieTimeScale > 0, false);

    int64_t emptyDuration = 0;
    int64_t startTime = 0;
    bool foundStart = false;
    // Translate edit-list empty edit plus media start into a track-time timeline offset.
    for (size_t i = 0; i < track->editListEntries.size(); ++i) {
        const auto& edit = track->editListEntries[i];
        if (i == 0 && edit.mediaTime == -1) {
            FALSE_RETURN_V_NOLOG(
                ConvertTimeScale(edit.duration, trackTimeScale, movieTimeScale, &emptyDuration) == Status::OK, false);
            continue;
        }
        if (!foundStart && edit.mediaTime >= 0) {
            startTime = edit.mediaTime;
            foundStart = true;
        }
    }
    FALSE_RETURN_V_NOLOG(SafeSubInt64(startTime, emptyDuration, timeOffset), false);
    return true;
}
// LCOV_EXCL_STOP

bool ShouldApplyMovEditTimelineOffset(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    if (track == nullptr || track->codecParms.trackType != AUDIO_TYPE || track->sampleHelper == nullptr ||
        track->editListEntries.empty()) {
        return false;
    }
    int64_t rawDuration = track->sampleHelper->GetRawTotalDuration();
    return rawDuration > 0 && track->editListEntries.front().mediaTime > rawDuration;
}

// LCOV_EXCL_START
bool GetRawAudioDtsTicks(std::unordered_map<uint32_t, std::vector<int64_t>>& rawDtsPrefix,
    const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex, int64_t& rawDts)
{
    FALSE_RETURN_V_NOLOG(IsValidTrack(track), false);
    uint32_t trackIndex = track->trackIndex;
    auto iter = rawDtsPrefix.find(trackIndex);
    if (iter == rawDtsPrefix.end()) {
        size_t sampleCount = track->sampleHelper->GetSampleCount();
        std::vector<int64_t> prefix(sampleCount + 1, 0);
        // Prefix sums keep raw decode timestamps available in O(1) without repeated scans.
        for (uint32_t i = 0; i < sampleCount; ++i) {
            uint32_t duration = 0;
            FALSE_RETURN_V_NOLOG(track->sampleHelper->GetSampleDuration(i, duration) == Status::OK &&
                SafeAddInt64(prefix[i], static_cast<int64_t>(duration), prefix[i + 1]), false);
        }
        iter = rawDtsPrefix.emplace(trackIndex, std::move(prefix)).first;
    }
    FALSE_RETURN_V_NOLOG(sampleIndex < iter->second.size(), false);
    rawDts = iter->second[sampleIndex];
    return true;
}
// LCOV_EXCL_STOP

Status SetPacketTimelineUs(const ReadSamplePacketContext& context, int64_t& dts, ReadSamplePacketAttr& attr)
{
    const auto& track = context.track;
    uint32_t sampleIndex = context.sampleIndex;
    const auto& currentSample = context.currentSample;
    int64_t timelineOffset = 0;
    if (ShouldApplyMovEditTimelineOffset(track)) {
        FALSE_RETURN_V_MSG_E(GetMovEditTimelineOffsetTicks(track, context.trackTimeScale, context.movieTimeScale,
            timelineOffset),
            Status::ERROR_INVALID_DATA, "Get MOV edit-list time offset failed");
    }
    int64_t pts = 0;
    FALSE_RETURN_V_MSG_E(SafeSubInt64(currentSample.pts, timelineOffset, pts),
        Status::ERROR_INVALID_DATA, "Calculate packet pts failed");
    FALSE_RETURN_V_MSG_E(SafeSubInt64(currentSample.dts, timelineOffset, dts),
        Status::ERROR_INVALID_DATA, "Calculate packet dts failed");
    if (ShouldApplyMovEditTimelineOffset(track)) {
        // For edit lists beyond raw media duration, packet PTS/DTS follows the raw decode timeline.
        int64_t rawDts = 0;
        FALSE_RETURN_V_MSG_E(GetRawAudioDtsTicks(context.rawDtsPrefix, track, sampleIndex, rawDts),
            Status::ERROR_INVALID_DATA, "Get raw audio dts failed");
        FALSE_RETURN_V_MSG_E(SafeSubInt64(rawDts, timelineOffset, pts),
            Status::ERROR_INVALID_DATA, "Calculate raw packet pts failed");
        FALSE_RETURN_V_MSG_E(SafeSubInt64(rawDts, timelineOffset, dts),
            Status::ERROR_INVALID_DATA, "Calculate raw packet dts failed");
    }
    FALSE_RETURN_V_MSG_E(RescaleTrackTimeToUsRoundNearest(pts, context.trackTimeScale, attr.ptsUs) == Status::OK,
        Status::ERROR_INVALID_DATA, "Convert pts to read sample packet us failed");
    FALSE_RETURN_V_MSG_E(RescaleTrackTimeToUsRoundNearest(dts, context.trackTimeScale, attr.dtsUs) == Status::OK,
        Status::ERROR_INVALID_DATA, "Convert dts to read sample packet us failed");
    return Status::OK;
}

bool UpdatePacketDurationUs(const ReadSamplePacketContext& context, ReadSamplePacketAttr& attr)
{
    const auto& track = context.track;
    uint32_t sampleIndex = context.sampleIndex;
    int32_t trackTimeScale = context.trackTimeScale;
    const auto& currentSample = context.currentSample;
    // Duration sources are ordered from container-specific corrections to generic sample-table fallback.
    bool hasDuration = GetSingleEditShortTailAudioDurationUs(context, attr.durationUs) ||
        GetMpegAudioFrameDurationUs(context, attr.durationUs) ||
        GetEditedAudioTailDurationUs(context, attr.durationUs) ||
        GetTimedMetaTailDurationUs(track, sampleIndex, attr.durationUs) ||
        GetVideoTailDurationUs(context, attr.durationUs) ||
        GetPacketDurationUs(context, attr.durationUs);
    if (!hasDuration && NeedsCttsFrameRateDuration(track)) {
        hasDuration = GetCttsFrameRateDurationUs(context.mediaInfo, track->trackIndex, trackTimeScale,
            attr.durationUs);
    }
    if (!hasDuration) {
        hasDuration = RescaleTrackTimeToUsRoundNearest(currentSample.duration, trackTimeScale, attr.durationUs) ==
            Status::OK;
    }
    if (attr.durationUs == 0 && GetAacFrameDurationUs(context, attr.durationUs)) {
        hasDuration = true;
    }
    return hasDuration;
}

void UpdateMovEditTailDurationUs(const ReadSamplePacketContext& context, int64_t dts, ReadSamplePacketAttr& attr)
{
    const auto& track = context.track;
    if (!ShouldApplyMovEditTimelineOffset(track) ||
        context.sampleIndex + 1 != track->sampleHelper->GetSampleCount()) {
        return;
    }
    int64_t streamDuration = GetEffectiveStreamDurationTicks(track, context.trackTimeScale, context.movieTimeScale);
    int64_t duration = streamDuration - dts;
    if (duration > 0) {
        (void)RescaleTrackTimeToUsRoundNearest(duration, context.trackTimeScale, attr.durationUs);
    }
}

Status BuildReadSamplePacketAttr(const ReadSamplePacketContext& context, ReadSamplePacketAttr& attr)
{
    const auto& track = context.track;
    FALSE_RETURN_V_NOLOG(IsValidTrack(track), Status::ERROR_NULL_POINTER);
    FALSE_RETURN_V_NOLOG(!track->sampleHelper->IsSampleIndexEmpty() &&
        context.sampleIndex < track->sampleHelper->GetSampleCount(), Status::ERROR_INVALID_PARAMETER);
    int64_t dts = 0;
    FALSE_RETURN_V_MSG_E(SetPacketTimelineUs(context, dts, attr) == Status::OK,
        Status::ERROR_INVALID_DATA, "Set read sample packet timeline failed");
    FALSE_RETURN_V_MSG_E(UpdatePacketDurationUs(context, attr),
        Status::ERROR_INVALID_DATA, "Set read sample packet duration failed");
    UpdateMovEditTailDurationUs(context, dts, attr);
    UpdateWebVttFinalPacketDurationFromMedia(context.mediaInfo, track, context.sampleIndex, attr);

    attr.flag = GetPacketSampleFlag(track, context.sampleIndex, context.currentSample,
        context.hasPlayableVideoTrack);
    const auto* skipSamplesInfo = IsValidTrack(track) ?
        track->sampleHelper->GetSkipSamplesInfo(context.sampleIndex) : nullptr;
    if (skipSamplesInfo != nullptr && !skipSamplesInfo->empty()) {
        attr.skipSamplesInfo = *skipSamplesInfo;
    }
    attr.sourceSampleIndex = context.sampleIndex;
    return Status::OK;
}

bool HasPlayableVideoTrack(const std::shared_ptr<MPEG4AtomParser::Track>& firstTrack)
{
    for (auto track = firstTrack; track != nullptr; track = track->next) {
        if (track->codecParms.trackType == VIDEO_TYPE && track->sampleHelper != nullptr &&
            !track->sampleHelper->IsSampleIndexEmpty()) {
            return true;
        }
    }
    return false;
}

bool CheckSeekTimeInFileDuration(const MediaInfo& mediaInfo, int64_t seekTime)
{
    int64_t fileDuration = 0;
    FALSE_RETURN_V_NOLOG(mediaInfo.general.Get<Tag::MEDIA_DURATION>(fileDuration) && fileDuration > 0, true);
    int64_t fileDurationMs = fileDuration / MS_TO_US; // MEDIA_DURATION is us, seekTime is ms.
    if (seekTime > fileDurationMs) {
        MEDIA_LOG_E("Seek to timestamp=" PUBLIC_LOG_D64 " ms failed, max=" PUBLIC_LOG_D64 " ms",
            seekTime, fileDurationMs);
        return false;
    }
    return true;
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
    if (track->codecParms.trackType == VIDEO_TYPE) {
        FALSE_RETURN_V_MSG_E(track->sampleHelper != nullptr && !IsImageMimeType(track->sampleHelper->mimeType_), false,
            "Unsupport image track");
    }
    return true;
}
} // namespace

std::atomic<int> MPEG4DemuxerPlugin::readatIndex_ = 0;

MPEG4DemuxerPlugin::MPEG4DemuxerPlugin(std::string name)
    : DemuxerPlugin(std::move(name)),
      firstTrack_(nullptr),
      lastTrack_(nullptr),
      isIsom_(false),
      hasSidxBox_(false),
      hasMoofBox_(false),
      seekable_(Seekable::SEEKABLE),
      selectedTrackIds_()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_D("In");
#ifdef BUILD_ENG_VERSION
    std::string dumpModeStr = OHOS::system::GetParameter("Mpeg4DemuxerPlugin.dump", "0");
    dumpMode_ = static_cast<DumpMode>(strtoul(dumpModeStr.c_str(), nullptr, 2)); // 2 is binary
    MEDIA_LOG_D("Dump mode = %s(%lu)", dumpModeStr.c_str(), dumpMode_);
#endif
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
    pendingWebVttSamples_.clear();
    rawDtsPrefix_.clear();
    avTrackIds_.clear();
    trackDfxInfoMap_.clear();
    inited_ = false;
    readModeFlags_.store(0, std::memory_order_release);
    MEDIA_LOG_D("Out");
}

bool MPEG4DemuxerPlugin::IsValidBuffer(const std::shared_ptr<AVBuffer>& outBuffer, bool replaceMemory)
{
    bool outBufferValid = outBuffer != nullptr && outBuffer->meta_ != nullptr;
    FALSE_RETURN_V_NOLOG(!replaceMemory, outBufferValid);
    return outBufferValid && outBuffer->memory_ != nullptr &&
        outBuffer->memory_->GetCapacity() >= 0 && outBuffer->memory_->GetAddr() != nullptr;
}

// LCOV_EXCL_START
void MPEG4DemuxerPlugin::Dump(const DumpParam &dumpParam)
{
    std::string path;
    switch (dumpParam.mode) {
        case DUMP_READAT_INPUT:
            path = "Readat_index." + std::to_string(dumpParam.index) + "_offset." +
                std::to_string(dumpParam.offset) + "_size." + std::to_string(dumpParam.size);
            break;
        case DUMP_AVPACKET_OUTPUT:
            path = "AVPacket_index." + std::to_string(dumpParam.index) + "_track." +
                std::to_string(dumpParam.trackId) + "_pts." + std::to_string(dumpParam.pts) + "_pos." +
                std::to_string(dumpParam.pos);
            break;
        case DUMP_AVBUFFER_OUTPUT:
            path = "AVBuffer_track." + std::to_string(dumpParam.trackId) + "_index." +
                std::to_string(dumpParam.index) + "_pts." + std::to_string(dumpParam.pts);
            break;
        default:
            return;
    }
    std::ofstream ofs;
    path = "/data/ff_dump/" + path;
    ofs.open(path, std::ios::out); //  | std::ios::app
    if (ofs.is_open()) {
        ofs.write(reinterpret_cast<char*>(dumpParam.buf), dumpParam.size);
        ofs.close();
    }
    MEDIA_LOG_D("Dump path:" PUBLIC_LOG_S, path.c_str());
}
// LCOV_EXCL_STOP

void MPEG4DemuxerPlugin::ReleaseReferenceParserState()
{
    std::lock_guard<std::mutex> lock(refPrepareMutex_);
    referenceParser_ = nullptr;
    isInit_ = false;
    refPrepareAttempted_ = false;
    refPrepareStatus_ = Status::OK;
    std::vector<uint32_t>().swap(refPtsOrder_);
    refPtsOrderReady_ = false;
    refParserTrackIdx_ = -1;
    trackOfRefParser_.reset();
    std::vector<uint32_t>().swap(syncSamplesIndex_);
    refParserStartTime_ = -1;
    refParserPendingSeekMsTime_ = -1;
    refParserCurGopId_ = 0;
    refParserIsForward_ = true;
    std::list<uint32_t>().swap(processingSyncSamplesIndex_);
    std::unordered_map<int32_t, int64_t>().swap(gopId2DecodedSamplePts_);
    mimeTypeOfVideoStream_ = MimeType::INVALID_TYPE;
    startPts_ = 0;
    updatePosIsForward_ = true;
    isSdtpExist_ = false;
    cacheSample_.reset();
    isReadSampleDataFromCache_ = false;
    refParserState_.store(true, std::memory_order_release);
}

void MPEG4DemuxerPlugin::ResetParam()
{
    ReleaseReadLoop();
    for (const auto& selectedTrackId : selectedTrackIds_) {
        ClearTrackCache(selectedTrackId);
    }
    // Clear cache for all video and audio tracks.
    for (const auto& avTrackId: avTrackIds_) {
        ClearTrackCache(avTrackId);
    }
    firstTrack_.reset();
    lastTrack_.reset();
    streamParsers_.reset();
    cacheSamples_.clear();
    pendingWebVttSamples_.clear();
    rawDtsPrefix_.clear();
    trackDfxInfoMap_.clear();
    ReleaseReferenceParserState();
    
    hasMoofBox_ = false;
    hasSidxBox_ = false;
    readToEnd_ = false;
    
    selectedTrackIds_.clear();
    trackMtx_.clear();
    mediaInfo_ = MediaInfo();
    movieTimeScale_ = 1;
    readatIndex_ = 0;
    avpacketIndex_ = 0;
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
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("In");
    ReleaseReadLoop();
    for (const auto& selectedTrackId : selectedTrackIds_) {
        ClearTrackCache(selectedTrackId);
    }
    cacheSamples_.clear();
    pendingWebVttSamples_.clear();
    readToEnd_ = false;
    return Status::OK;
}

void MPEG4DemuxerPlugin::InitBaseInfoByAtomParser(std::shared_ptr<MPEG4AtomParser> parser)
{
    mediaInfo_ = parser->GetMediaInfo();
    movieTimeScale_ = parser->GetMovieTimeScale();
    firstTrack_ = parser->GetFirstTrack();
    userformat_ = parser->GetUserFormat();
    hasSidxBox_ = parser->GetHasSidx();
    hasMoofBox_ = parser->GetHasMoof();
    fragmentEntry_.clear();
    parser->GetFragmentEntry(fragmentEntry_);
    mediaInfo_.general.Set<Tag::MEDIA_TRACK_COUNT>(CountTracks());
}

void MPEG4DemuxerPlugin::InitDrmInfoCacheByAtomParser(std::shared_ptr<MPEG4AtomParser> parser)
{
    std::lock_guard<std::mutex> drmLock(cachedDrmInfoMutex_);
    cachedDrmInfo_.clear();
    const auto& psshList = parser->GetPsshList();
    for (const auto& pssh : psshList) {
        if (pssh.uuidLen == PSSH_UUID_SIZE) {
            std::stringstream ss;
            for (uint32_t i = 0; i < pssh.uuidLen; ++i) {
                ss << std::hex << std::setfill('0') << std::setw(HEX_CHARS_PER_BYTE)
                << static_cast<int32_t>(pssh.uuid[i]);
            }
            //ss.str() is uuid
            cachedDrmInfo_.emplace(ss.str(), pssh.psshData);
        }
    }
    drmInfoCached_.store(!cachedDrmInfo_.empty());
}

Status MPEG4DemuxerPlugin::PrepareTrackMetadata()
{
    if (firstTrack_ == nullptr) {
        // FFmpeg accepts a valid MP4 container without tracks and exposes an empty track list.
        MEDIA_LOG_W("No track found in MPEG4 source");
        return Status::OK;
    }
    InitStreamParsers();
    GetMediaInfo();
    if (avTrackIds_.empty()) {
        // A track without playable AV samples does not require first-frame codec parameters.
        MEDIA_LOG_W("No playable AV samples found in MPEG4 source");
        return Status::OK;
    }
    Status ret = ConvertElstTimescaleToMs();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ConvertElstTimescaleToMs is failed");
    if (HasCodecParameters() != Status::OK) {
        ResetParam();
        MEDIA_LOG_E("SetDataSource failed cause not enough data");
        return Status::ERROR_NOT_ENOUGH_DATA;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SetDataSource(const std::shared_ptr<DataSource>& source,
    const std::shared_ptr<Media::Meta>& configs)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MEDIA_LOG_I("In");
    FALSE_RETURN_V_MSG_E(!inited_, Status::ERROR_WRONG_STATE, "Plugin has been initialized");
    FALSE_RETURN_V_MSG_E(source != nullptr, Status::ERROR_INVALID_PARAMETER, "DataSource is nullptr");

    seekable_ = source->IsDash() ? Seekable::UNSEEKABLE : source->GetSeekable();
    dataSource_ = source;
    demuxerMode_ = Plugins::DemuxerMode::FULL_PARSE;
    if (configs != nullptr && configs->Get<Tag::MEDIA_DEMUXER_MODE>(demuxerMode_)) {
        MEDIA_LOG_I("DemuxerMode: " PUBLIC_LOG_U32, static_cast<uint32_t>(demuxerMode_));
    }
    Status ret = Status::OK;
    auto atomParser = std::make_shared<MPEG4AtomParser>();
    FALSE_RETURN_V_MSG_E(atomParser != nullptr, Status::ERROR_NO_MEMORY, "parser allocation failed");
    {
        auto id = HiviewDFX::XCollie::GetInstance().SetTimer("av_codec::demuxer_setdatasource", SETTIMER_TIMEOUT,
            nullptr, nullptr, HiviewDFX::XCOLLIE_FLAG_LOG);
        ret = atomParser->MPEG4ParseHeader(
            source, seekable_, readTimeoutMs_.load(std::memory_order_acquire), demuxerMode_);
        atomParser->NotifyInitializationCompleted();
        FALSE_RETURN_V_MSG_E(ret != Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT, ret, "MPEG4Demuxer unsupport type");
        FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "MPEG4ParseHeader failed");
        HiviewDFX::XCollie::GetInstance().CancelTimer(id);
    }

    InitBaseInfoByAtomParser(atomParser);
    InitDrmInfoCacheByAtomParser(atomParser);
    ret = PrepareTrackMetadata();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "PrepareTrackMetadata failed");

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
    int32_t fileTimeScale = movieTimeScale_;
    FALSE_RETURN_V_MSG_E(fileTimeScale > 0, Status::ERROR_INVALID_DATA, "Get mvhd timescale failed");
    int32_t trackTimeScale;
    for (uint32_t trackIndex = 0; track != nullptr; ++trackIndex) {
        if (track->codecParms.trackType == VIDEO_TYPE &&
            (track->sampleHelper == nullptr || IsImageMimeType(track->sampleHelper->mimeType_))) {
            MEDIA_LOG_D("Skip cover track " PUBLIC_LOG_U32, trackIndex);
            track = track->next;
            continue;
        }
        trackTimeScale = track->timeScale;
        if (trackTimeScale > 0) {
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
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_TRACK_COUNT>(trackCount), Status::ERROR_INVALID_DATA,
        "GetData is failed");
    FALSE_RETURN_V_MSG_E(trackCount == static_cast<int32_t>(mediaInfo_.tracks.size()), Status::ERROR_INVALID_DATA,
        "mediaInfo is error");

    for (int32_t trackIndex = 0; trackIndex < trackCount; ++trackIndex) {
        FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "track is nullptr");
        Meta &format = mediaInfo_.tracks[trackIndex];
        std::string mime;
        FALSE_RETURN_V_MSG_E(format.Get<Tag::MIME_TYPE>(mime), Status::ERROR_INVALID_DATA,
            "Get mime type failed");
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

void MPEG4DemuxerPlugin::InitStreamParsers()
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
                MEDIA_LOG_W("Init parser failed, track " PUBLIC_LOG_U32 ", mime " PUBLIC_LOG_S,
                    trackIndex, mime.c_str());
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
            && !track->sampleHelper->IsSampleIndexEmpty()) {
            avTrackIds_.emplace_back(track->trackIndex);
            if (seekable_ == Seekable::UNSEEKABLE) {
                trackMtx_[track->trackIndex] = std::make_shared<std::mutex>();
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
        mpeg4Sample->trackId = avTrackId;
        
        // 使用GetSampleBySeekableStatus获取首帧，传入空AVBuffer
        Status ret = GetSampleBySeekableStatus(avTrackId, mpeg4Sample, nullptr);
        FALSE_CONTINUE_LOGD(ret == Status::OK && mpeg4Sample->sample != nullptr && mpeg4Sample->sample->data != nullptr,
            "Failed to get first frame for track:" PUBLIC_LOG_U32, avTrackId);

        FALSE_CONTINUE_LOGD(ProcessVideoFirstFrame(currentTrack, avTrackId, mpeg4Sample),
            "ConvertExtraDataToAnnexb failed in track:" PUBLIC_LOG_U32, avTrackId);

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

bool MPEG4DemuxerPlugin::ProcessVideoFirstFrame(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    uint32_t trackId, const std::shared_ptr<MPEG4Sample>& mpeg4Sample)
{
    if (!IsVideoType(*track) || streamParsers_ == nullptr) {
        return true;
    }
    if (streamParsers_->ParserIsCreated(trackId) && !streamParsers_->ParserIsInited(trackId)) {
        const auto& codecParms = track->codecParms;
        if (codecParms.data != nullptr && codecParms.extradataSize > 0) {
            bool convertRet = streamParsers_->ConvertExtraDataToAnnexb(trackId, codecParms.data.get(),
                codecParms.extradataSize);
            if (!convertRet) {
                return false;
            }
        }
    }
    firstFrameMap_[trackId] = mpeg4Sample->sample;
    return true;
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

void MPEG4DemuxerPlugin::ParseAVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta &format)
{
    uint32_t trackIndex = track.trackIndex;
    uint32_t w = streamParsers_->GetPicWidInLumaSamples(trackIndex);
    uint32_t h = streamParsers_->GetPicHetInLumaSamples(trackIndex);
    double sar = streamParsers_->GetSar(trackIndex);
    if (w > 0 && h > 0) {
        format.Set<Tag::VIDEO_WIDTH>(w);
        format.Set<Tag::VIDEO_HEIGHT>(h);
    }
    if (sar > 0) {
        format.Set<Tag::VIDEO_SAR>(sar);
    }
}

void MPEG4DemuxerPlugin::GetSingleTrackInfo(MPEG4AtomParser::Track& track, const uint32_t trackIndex)
{
    Meta meta = mediaInfo_.tracks[trackIndex];
    std::string mime;
    mediaInfo_.tracks[trackIndex].Get<Tag::MIME_TYPE>(mime);
    if (mime == "video/hevc" || mime == "video/avc") {
        if (streamParsers_ != nullptr && streamParsers_->ParserIsInited(trackIndex) &&
            FirstFrameValid(trackIndex)) {
            auto firstFrame = firstFrameMap_[trackIndex];
            uint8_t* dataPtr = firstFrame->data.get();
            int32_t dataSize = firstFrame->size;
            PacketConvertInfo convertInfo {nullptr, 0, false};
            streamParsers_->ConvertPacketToAnnexb(trackIndex, &dataPtr, dataSize, convertInfo);
            streamParsers_->ParseAnnexbExtraData(trackIndex, dataPtr, dataSize);
            // Parser only sends xps info when first call ConvertPacketToAnnexb
            // readSample will call ConvertPacketToAnnexb again, so rest here
            streamParsers_->ResetXPSSendStatus(trackIndex);
            if (mime == "video/hevc") {
                ParseHEVCMetadataInfo(track, meta);
            } else if (mime == "video/avc") {
                ParseAVCMetadataInfo(track, meta);
            }
        } else {
            MEDIA_LOG_W("Parse avc/hevc info failed");
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
    MediaAVCodec::AVCodecTrace trace("GetMediaInfo");
    
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
    MediaAVCodec::AVCodecTrace trace("GetUserMeta");
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
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    std::lock_guard<std::mutex> drmLock(cachedDrmInfoMutex_);
    drmInfo = cachedDrmInfo_;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SetDrmCencInfo(uint32_t trackId, std::shared_ptr<AVBuffer> sample, int32_t sampleSize,
    int32_t oldSampleSize, uint32_t sampleIndex)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_INVALID_OPERATION, "Sample is nullptr");
    FALSE_RETURN_V_MSG_E(sample->memory_ != nullptr, Status::ERROR_INVALID_OPERATION, "Memory is nullptr");
    auto track = FindTrackById(trackId);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    if (!track->drmInfo.isEncrypted) {
        return Status::OK;
    }
    // LCOV_EXCL_START
    uint32_t localSampleIndex = (sampleIndex != UINT32_MAX) ? sampleIndex : track->currentSampleIndex;
    if (localSampleIndex >= track->drmInfo.sampleEncryptionEntries.size()) {
        if (track->drmInfo.defaultCencInfo.keyIdLen == 0) {
            return Status::OK;
        }
    }
    MetaDrmCencInfo cencInfo;
    errno_t err = memset_s(&cencInfo, sizeof(MetaDrmCencInfo), 0, sizeof(MetaDrmCencInfo));
    FALSE_RETURN_V_MSG_E(err == EOK, Status::ERROR_INVALID_OPERATION, "memset cencInfo failed");

    Status ret = BuildDrmCencBaseInfo(track, cencInfo);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build DRM CENC base info failed");

    ret = FillDrmSampleEncryption(track, cencInfo, localSampleIndex, sampleSize, oldSampleSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Fill DRM sample encryption failed");

    std::vector<uint8_t> drmCencVec(reinterpret_cast<uint8_t*>(&cencInfo),
        reinterpret_cast<uint8_t*>(&cencInfo) + sizeof(MetaDrmCencInfo));
    sample->meta_->SetData(Media::Tag::DRM_CENC_INFO, std::move(drmCencVec));
    return Status::OK;
}

Status MPEG4DemuxerPlugin::BuildDrmCencBaseInfo(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    MetaDrmCencInfo& cencInfo)
{
    switch (track->drmInfo.schemeType) {
        case FourccType("cenc"):
        case FourccType("cens"):
            cencInfo.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CTR;
            break;
        case FourccType("cbc1"):
        case FourccType("cbcs"):
            cencInfo.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_AES_CBC;
            break;
        case FourccType("sm4c"):
        case FourccType("sm4s"):
            cencInfo.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_SM4_CBC;
            break;
        default:
            cencInfo.algo = MetaDrmCencAlgorithm::META_DRM_ALG_CENC_UNENCRYPTED;
            break;
    }
    if (track->drmInfo.defaultCencInfo.keyIdLen > 0) {
        errno_t err = memcpy_s(cencInfo.keyId, sizeof(cencInfo.keyId),
            track->drmInfo.defaultCencInfo.keyId, track->drmInfo.defaultCencInfo.keyIdLen);
        FALSE_RETURN_V_MSG_E(err == EOK, Status::ERROR_INVALID_OPERATION, "Copy keyId failed");
        cencInfo.keyIdLen = track->drmInfo.defaultCencInfo.keyIdLen;
    }
    cencInfo.encryptBlocks = track->drmInfo.defaultCencInfo.encryptBlocks;
    cencInfo.skipBlocks = track->drmInfo.defaultCencInfo.skipBlocks;
    cencInfo.mode = MetaDrmCencInfoMode::META_DRM_CENC_INFO_KEY_IV_SUBSAMPLES_SET;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FillDrmSampleEncryption(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    MetaDrmCencInfo& cencInfo, uint32_t localSampleIndex, int32_t sampleSize, int32_t oldSampleSize)
{
    if (localSampleIndex >= track->drmInfo.sampleEncryptionEntries.size()) {
        cencInfo.subSampleNum = 1;
        cencInfo.subSamples[0].clearHeaderLen = 0;
        cencInfo.subSamples[0].payLoadLen = static_cast<uint32_t>(sampleSize);
        return Status::OK;
    }
    const auto& entry = track->drmInfo.sampleEncryptionEntries[localSampleIndex];
    if (!entry.iv.empty() && entry.iv.size() <= sizeof(cencInfo.iv)) {
        errno_t err = memcpy_s(cencInfo.iv, sizeof(cencInfo.iv), entry.iv.data(), entry.iv.size());
        FALSE_RETURN_V_MSG_E(err == EOK, Status::ERROR_INVALID_OPERATION, "Copy iv failed");
        cencInfo.ivLen = static_cast<uint32_t>(entry.iv.size());
    } else if (track->drmInfo.defaultCencInfo.ivLen > 0) {
        errno_t err = memcpy_s(cencInfo.iv, sizeof(cencInfo.iv),
            track->drmInfo.defaultCencInfo.iv, track->drmInfo.defaultCencInfo.ivLen);
        FALSE_RETURN_V_MSG_E(err == EOK, Status::ERROR_INVALID_OPERATION, "Copy default iv failed");
        cencInfo.ivLen = track->drmInfo.defaultCencInfo.ivLen;
    }
    return FillDrmSubsamples(cencInfo, entry, sampleSize, oldSampleSize);
}

Status MPEG4DemuxerPlugin::FillDrmSubsamples(MetaDrmCencInfo& cencInfo,
    const MPEG4AtomParser::Mpeg4SampleEncryptionEntry& entry, int32_t sampleSize, int32_t oldSampleSize)
{
    if (entry.subSamples.empty()) {
        cencInfo.subSampleNum = 1;
        cencInfo.subSamples[0].clearHeaderLen = 0;
        cencInfo.subSamples[0].payLoadLen = static_cast<uint32_t>(sampleSize);
        return Status::OK;
    }
    uint32_t subsampleCount = static_cast<uint32_t>(entry.subSamples.size());
    constexpr uint32_t maxSubsampleNum = 64;
    subsampleCount = std::min(subsampleCount, maxSubsampleNum);
    cencInfo.subSampleNum = subsampleCount;
    for (uint32_t i = 0; i < subsampleCount; ++i) {
        cencInfo.subSamples[i].clearHeaderLen = entry.subSamples[i].clearHeaderLen;
        cencInfo.subSamples[i].payLoadLen = entry.subSamples[i].payLoadLen;
    }
    if (oldSampleSize <= 0 || oldSampleSize == sampleSize) {
        return Status::OK;
    }
    int32_t diff = sampleSize - oldSampleSize;
    if (diff > 0) {
        cencInfo.subSamples[0].clearHeaderLen += static_cast<uint32_t>(diff);
        return Status::OK;
    }
    uint32_t diffAbs = static_cast<uint32_t>(-diff);
    for (uint32_t i = 0; i < subsampleCount; ++i) {
        if (cencInfo.subSamples[i].clearHeaderLen < diffAbs) {
            continue;
        }
        cencInfo.subSamples[i].clearHeaderLen -= diffAbs;
        break;
    }
    return Status::OK;
}
// LCOV_EXCL_STOP

void MPEG4DemuxerPlugin::ConvertCsdToAnnexb(MPEG4AtomParser::Track& track, Meta &format, uint32_t trackIndex)
{
    track.codecParms.hasValidExtradata = false;
    track.codecParms.annexbExtradataSize = 0;
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
        track.codecParms.annexbExtradataSize = extradataSize;
        track.codecParms.hasValidExtradata = true;
    }
}

void MPEG4DemuxerPlugin::ParseHEVCMetadataInfo(const MPEG4AtomParser::Track& track, Meta& format)
{
    HevcParseFormat parse;
    uint32_t trackIndex = track.trackIndex;
    if (streamParsers_ != nullptr) {
        streamParsers_->ParseMetadataInfo(trackIndex, parse);
    }
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
        return Status::OK;
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
        pendingWebVttSamples_.erase(trackId);
        ClearTrackCache(trackId);
        return Status::OK;
    } else {
        MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " is not in selected list", trackId);
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::DynamicBaseTrackSelect(int64_t seekTime, int64_t &maxPts, int32_t &baseTrackIndex)
{
    int64_t minPts = INT64_MAX;
    int32_t baseTrackTimeBase = 0;
    bool baseTrackIsVideo = false;
    for (const auto& currentTrackId : selectedTrackIds_) {
        auto track = FindTrackById(currentTrackId);
        FALSE_RETURN_V_MSG_E(IsValidTrack(track), Status::ERROR_NULL_POINTER, "Track is nullptr");
        int32_t trackTimeBase = track->timeScale;
        FALSE_RETURN_V_NOLOG(trackTimeBase > 0, Status::ERROR_INVALID_PARAMETER);
        bool editListApplied = track->sampleHelper->HasAppliedEditList();
        int64_t timeDelay = editListApplied ? 0 : track->elstInitEmptyEdit - minElstInitEmptyEdit_;
        int64_t timeDelayToPts = 0;
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(timeDelay, static_cast<int64_t>(trackTimeBase), 1000LL,
            &timeDelayToPts) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        int64_t seekTimeWithStart = seekTime;
        if (editListApplied) {
            FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(seekTime, track->elstInitEmptyEdit, &seekTimeWithStart),
                Status::ERROR_INVALID_PARAMETER, "Seek time overflow");
        }
        int64_t seekTimeToPts;
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(seekTimeWithStart - timeDelay, static_cast<int64_t>(trackTimeBase),
            1000LL, &seekTimeToPts) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        if (!editListApplied) {
            seekTimeToPts += track->elstShiftStartTime;
        }

        FALSE_RETURN_V_MSG_E(!track->sampleHelper->IsSampleIndexEmpty(), Status::ERROR_NULL_POINTER,
            "sampleIndexEntry_ is empty");
        int64_t firstPts = 0;
        FALSE_RETURN_V_MSG_E(track->sampleHelper->GetSamplePts(0, firstPts) == Status::OK,
            Status::ERROR_INVALID_DATA, "Get first sample pts failed");
        int64_t lastPts = track->sampleHelper->lastPTS_;
        if (seekTimeToPts >= firstPts && seekTimeToPts < lastPts) {
            DynamicBaseTrackState state {minPts, baseTrackTimeBase, baseTrackIsVideo, baseTrackIndex};
            Status ret = UpdateDynamicBaseTrack(track, currentTrackId, timeDelayToPts, state);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Update dynamic base track failed");
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

Status MPEG4DemuxerPlugin::UpdateDynamicBaseTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    uint32_t trackId, int64_t timeDelayToPts, DynamicBaseTrackState& state)
{
    int32_t trackTimeBase = track->timeScale;
    FALSE_RETURN_V_MSG_E(trackTimeBase > 0, Status::ERROR_INVALID_PARAMETER, "Get track time scale failed");
    int64_t convertedMinPts = INT64_MAX;
    if (state.minPts != INT64_MAX) {
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(state.minPts, static_cast<int64_t>(trackTimeBase),
            static_cast<int64_t>(state.baseTrackTimeBase), &convertedMinPts) == Status::OK,
            Status::ERROR_INVALID_PARAMETER, "mul overflow");
    }
    bool isVideoTrack = IsVideoType(*track);
    int64_t firstPts = 0;
    FALSE_RETURN_V_MSG_E(track->sampleHelper->GetSamplePts(0, firstPts) == Status::OK,
        Status::ERROR_INVALID_DATA, "Get first sample pts failed");
    int64_t trackPtsWithDelay = firstPts + timeDelayToPts;
    bool shouldUpdate = isVideoTrack ? (!state.baseTrackIsVideo || trackPtsWithDelay < convertedMinPts) :
        (!state.baseTrackIsVideo && trackPtsWithDelay < convertedMinPts);
    if (shouldUpdate) {
        state.baseTrackIndex = static_cast<int32_t>(trackId);
        state.minPts = trackPtsWithDelay;
        state.baseTrackTimeBase = trackTimeBase;
        state.baseTrackIsVideo = isVideoTrack;
    }
    return Status::OK;
}

// find base track & calculate absolute delay time
Status MPEG4DemuxerPlugin::FindBaseTrackAndCalcDelay(int64_t seekTime, SeekMode mode, int32_t &baseTrackIndex)
{
    for (const auto& currentTrackId : selectedTrackIds_) {
        auto track = FindTrackById(currentTrackId);
        FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        if (track->sampleHelper != nullptr && track->sampleHelper->HasAppliedEditList()) {
            continue;
        }
        // calculate the absolute delay time of selected tracks
        minElstInitEmptyEdit_ = (track->elstInitEmptyEdit > 0) ?
            std::min(minElstInitEmptyEdit_, track->elstInitEmptyEdit) : 0;
    }
    if (minElstInitEmptyEdit_ == INT64_MAX) {
        minElstInitEmptyEdit_ = 0;
    }

    int64_t maxPts = 0; // ms
    FALSE_RETURN_V_MSG_E(DynamicBaseTrackSelect(seekTime, maxPts, baseTrackIndex) == Status::OK,
        Status::ERROR_INVALID_PARAMETER, "Find base track failed");
    relativeEndPts_ = static_cast<uint64_t>(maxPts);
    if (seekTime > static_cast<int64_t>(relativeEndPts_) && mode == SeekMode::SEEK_NEXT_SYNC) {
        MEDIA_LOG_W("Seek time " PUBLIC_LOG_D64 " exceeds stream end " PUBLIC_LOG_U64
            ", clamp to stream duration", seekTime, relativeEndPts_);
    }
    MEDIA_LOG_D("Find base track ID: " PUBLIC_LOG_D32 ", minElstInitEmptyEdit_=" PUBLIC_LOG_D64,
        baseTrackIndex, minElstInitEmptyEdit_);
    return Status::OK;
}

// LCOV_EXCL_START
Status MPEG4DemuxerPlugin::FindFragmentSyncSample(uint32_t fragmentIndex, std::shared_ptr<MPEG4AtomParser::Track> track)
{
    int64_t moofOffset = fragmentEntry_[fragmentIndex].moofOffset;
    for (const auto& selectedTrackId : selectedTrackIds_) {
        uint32_t currentTrackId = selectedTrackId;
        auto currentTrack = FindTrackById(currentTrackId);
        FALSE_RETURN_V_MSG_E(currentTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        uint32_t sampleIndex = 0;
        FALSE_RETURN_V_MSG_E(currentTrack->sampleHelper->FindFirstSampleAfterPosition(moofOffset, sampleIndex) ==
            Status::OK, Status::ERROR_INVALID_DATA, "Find fragment sample failed");
        FALSE_RETURN_V_NOLOG(sampleIndex + 1 < currentTrack->sampleHelper->GetSampleCount(), Status::ERROR_UNKNOWN);
        currentTrack->currentSampleIndex = sampleIndex;
        currentTrack->currentSamplePos = 0;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FindFragmentAtTime(int64_t fragmentSeekTime, SeekMode flag,
    std::shared_ptr<MPEG4AtomParser::Track> track, bool &findFragmentSync)
{
    FALSE_RETURN_V_NOLOG(fragmentSeekTime >= fragmentEntry_.front().firstDts, Status::OK);
    FALSE_RETURN_V_NOLOG(fragmentSeekTime <= fragmentEntry_.back().firstDts + fragmentEntry_.back().duration,
        Status::ERROR_INVALID_PARAMETER);

    auto& entry = fragmentEntry_;
    auto it = std::lower_bound(entry.begin(), entry.end(), fragmentSeekTime,
        [](const MPEG4AtomParser::FragmentEntry& a, int64_t b) { return a.firstDts < b; });
    if (it == entry.end()) {
        it = entry.end() - 1;
    }
    uint32_t fragmentIndex = static_cast<uint32_t>(std::distance(entry.begin(), it));
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
// LCOV_EXCL_STOP

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

Status MPEG4DemuxerPlugin::CalculateRealSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    int64_t seekTime, int64_t& realSeekTime)
{
    int64_t trackStartTimeUs = 0;
    FALSE_RETURN_V_MSG_E(mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_START_TIME>(trackStartTimeUs),
        Status::ERROR_INVALID_DATA, "Get track start time failed");
    int64_t seekTimeNs = 0;
    int64_t trackStartTimeNs = 0;
    FALSE_RETURN_V_MSG_E(!__builtin_mul_overflow(seekTime, static_cast<int64_t>(MS_TO_NS), &seekTimeNs) &&
        !__builtin_mul_overflow(trackStartTimeUs, US_TO_NS, &trackStartTimeNs) &&
        !__builtin_add_overflow(seekTimeNs, trackStartTimeNs, &realSeekTime),
        Status::ERROR_INVALID_PARAMETER, "Real seek time overflow");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ValidateElstShiftStartTimeScale(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    const SeekTimeContext& context) const
{
    FALSE_RETURN_V_NOLOG(!context.editListApplied, Status::OK);
    int64_t startTime = 0;
    FALSE_RETURN_V_MSG_E(ConvertTimeScale(track->elstShiftStartTime, 1000LL, context.baseTrackTimeScale,
        &startTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ClampSeekTimeToStreamDuration(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    SeekTimeContext& context)
{
    int64_t streamDuration = track->duration;
    if (context.editListApplied && track->sampleHelper != nullptr) {
        streamDuration = std::max(track->sampleHelper->totalDuration_, track->sampleHelper->lastPTS_);
    }
    // Align with MOV/MP4: when timestamp is out of stream duration but inside file duration, seek to end of stream.
    if (streamDuration <= 0 || context.trackSeekTime <= streamDuration) {
        return Status::OK;
    }
    MEDIA_LOG_W("Out of stream, seek to " PUBLIC_LOG_D64, context.trackSeekTime);
    context.trackSeekTime = streamDuration;
    int64_t syncTrackTime = context.trackSeekTime;
    if (!context.editListApplied) {
        syncTrackTime -= track->elstShiftStartTime;
        if (syncTrackTime < 0) {
            MEDIA_LOG_W("Clamp negative sync track time " PUBLIC_LOG_D64 " to 0", syncTrackTime);
            syncTrackTime = 0;
        }
    }
    FALSE_RETURN_V_MSG_E(ConvertTimeScale(syncTrackTime, 1000LL, context.baseTrackTimeScale,
        &context.syncSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    if (!context.editListApplied) {
        context.syncSeekTime += track->elstInitEmptyEdit;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::PrepareSeekTime(const std::shared_ptr<MPEG4AtomParser::Track>& track, int64_t seekTime,
    int64_t& realSeekTime, SeekTimeContext& context)
{
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    context.syncSeekTime = seekTime;
    context.editListApplied = track->sampleHelper != nullptr && track->sampleHelper->HasAppliedEditList();
    if (context.editListApplied) {
        FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(seekTime, track->elstInitEmptyEdit, &context.relativeSeekTime),
            Status::ERROR_INVALID_PARAMETER, "Seek time overflow");
    } else {
        context.relativeSeekTime = seekTime - (track->elstInitEmptyEdit - minElstInitEmptyEdit_);
    }
    context.baseTrackTimeScale = track->timeScale;
    FALSE_RETURN_V_NOLOG(context.baseTrackTimeScale > 0, Status::ERROR_INVALID_PARAMETER);
    Status ret = ValidateElstShiftStartTimeScale(track, context);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Validate elst shift start time scale failed");
    ret = CalculateRealSeekTime(track, seekTime, realSeekTime);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Calculate real seek time failed");

    FALSE_RETURN_V_MSG_E(ConvertTimeScale(context.relativeSeekTime, static_cast<int64_t>(context.baseTrackTimeScale),
        1000LL, &context.trackSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    if (!context.editListApplied) {
        context.trackSeekTime += track->elstShiftStartTime;
    }
    return ClampSeekTimeToStreamDuration(track, context);
}

Status MPEG4DemuxerPlugin::FindSeekSampleIndex(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    SeekSampleIndexContext& context)
{
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    Status ret = track->sampleHelper->FindSyncSampleByPts(
        context.trackSeekTime, context.isBaseTrack, context.mode, context.sampleIndex);
    // If seektime > last I-frame and within the track range, and the SeekMode is SEEK_NEXT_SYNC, seek to the EOS frame
    context.sampleIndex = (ret != Status::OK && context.mode == SeekMode::SEEK_NEXT_SYNC) ?
        sampleCount : context.sampleIndex;
    bool isVideo = IsVideoType(*track);
    if (isVideo && context.seekTime - minElstInitEmptyEdit_ < static_cast<int64_t>(relativeEndPts_) &&
        context.sampleIndex == sampleCount) {
        context.isBaseTrack = false;
    }
    MEDIA_LOG_I("Find sync sample " PUBLIC_LOG_D32 " in track " PUBLIC_LOG_D32,
        context.sampleIndex, track->trackIndex);
    return ret;
}

Status MPEG4DemuxerPlugin::DoSeekInternal(int32_t trackIndex, int64_t seekTime, SeekMode mode, int64_t& realSeekTime)
{
    auto track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    SeekTimeContext seekContext;
    Status ret = PrepareSeekTime(track, seekTime, realSeekTime, seekContext);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare seek time failed");
    MEDIA_LOG_I("Time [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "] mode [" PUBLIC_LOG_D32 "]", seekTime,
        realSeekTime, static_cast<int32_t>(mode));

    bool findFragmentSync = false;
    ret = FragmentSeek(seekContext.trackSeekTime, mode, trackIndex, findFragmentSync);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "FindFragmentAtTime failed");
    FALSE_RETURN_V_NOLOG(!findFragmentSync, ret);

    bool isVideo = IsVideoType(*track);
    SeekSampleIndexContext sampleContext {seekContext.syncSeekTime, seekContext.trackSeekTime, mode};
    ret = FindSeekSampleIndex(track, sampleContext);
    if (isVideo && mode == SeekMode::SEEK_NEXT_SYNC && ret != Status::OK) {
        MEDIA_LOG_E("Seek failed, err: Not next key frame");
        return Status::ERROR_OUT_OF_RANGE;
    }

    int64_t syncSeekTime = seekContext.syncSeekTime;
    if (isVideo) {
        VideoSyncSeekContext context {
            sampleContext.sampleIndex, sampleContext.isBaseTrack, seekContext.editListApplied,
            seekContext.baseTrackTimeScale, syncSeekTime
        };
        ret = GetVideoSyncSeekTime(track, context);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get video sync seek time failed");
    }
    SeekMode multiTrackMode = mode;
    // Keep exact closest hits in NEXT mode to match current MOV/MP4-parity count expectations.
    if (mode == SeekMode::SEEK_CLOSEST_SYNC && syncSeekTime < seekTime) {
        multiTrackMode = SeekMode::SEEK_PREVIOUS_SYNC;
    } else if (mode == SeekMode::SEEK_CLOSEST_SYNC) {
        multiTrackMode = SeekMode::SEEK_NEXT_SYNC;
    }
    ret = MultiTrackSync(trackIndex, !sampleContext.isBaseTrack, syncSeekTime, multiTrackMode);  // 多轨同步
    if (ret == Status::OK) {
        track->currentSampleIndex = sampleContext.sampleIndex;
        track->currentSamplePos = 0;
    }
    return ret;
}

Status MPEG4DemuxerPlugin::GetVideoSyncSeekTime(
    const std::shared_ptr<MPEG4AtomParser::Track>& track, VideoSyncSeekContext& context)
{
    size_t sampleCount = track->sampleHelper->GetSampleCount();
    int64_t syncPts = 0;
    if (!context.isBaseTrack) {
        FALSE_RETURN_V_MSG_E(sampleCount > 0, Status::ERROR_INVALID_DATA, "Sample index entry is empty");
        FALSE_RETURN_V_MSG_E(track->sampleHelper->GetSamplePts(static_cast<uint32_t>(sampleCount - 1), syncPts) ==
            Status::OK, Status::ERROR_INVALID_DATA, "Get last sample pts failed");
    } else {
        FALSE_RETURN_V_MSG_E(context.sampleIndex < sampleCount, Status::ERROR_INVALID_DATA,
            "Invalid sync sample index");
        FALSE_RETURN_V_MSG_E(track->sampleHelper->GetSamplePts(context.sampleIndex, syncPts) == Status::OK,
            Status::ERROR_INVALID_DATA, "Get sync sample pts failed");
    }
    if (!context.editListApplied) {
        syncPts -= track->elstShiftStartTime;
    }
    FALSE_RETURN_V_MSG_E(ConvertTimeScale(syncPts, 1000LL, context.baseTrackTimeScale,
        &context.syncSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
    if (!context.editListApplied) {
        context.syncSeekTime += track->elstInitEmptyEdit;
    }
    return Status::OK;
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
        int32_t otherTrackTimeScale = otherTrack->timeScale;
        FALSE_RETURN_V_NOLOG(otherTrackTimeScale > 0, Status::ERROR_INVALID_PARAMETER);

        int64_t otherTrackSeekTime = 0;
        bool editListApplied = otherTrack->sampleHelper != nullptr && otherTrack->sampleHelper->HasAppliedEditList();
        if (editListApplied) {
            adjustSeekTime = syncSeekTime;
        }
        FALSE_RETURN_V_MSG_E(ConvertTimeScale(adjustSeekTime, static_cast<int64_t>(otherTrackTimeScale), 1000LL,
            &otherTrackSeekTime) == Status::OK, Status::ERROR_INVALID_PARAMETER, "mul overflow");
        if (!editListApplied) {
            otherTrackSeekTime += otherTrack->elstShiftStartTime;
        }
        ret = otherTrack->sampleHelper->FindSyncSampleByPts(otherTrackSeekTime, isBaseTrack, mode, otherSampleIndex);
        if (ret == Status::OK) {
            otherTrack->currentSampleIndex = otherSampleIndex;
            otherTrack->currentSamplePos = 0;
            MEDIA_LOG_D("Sync track " PUBLIC_LOG_D32 " to sample " PUBLIC_LOG_D32 " at time " PUBLIC_LOG_D64
                ", mode " PUBLIC_LOG_D32,
                currentTrackId, otherSampleIndex, adjustSeekTime, static_cast<int32_t>(mode));
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

Status MPEG4DemuxerPlugin::CheckSeekParams(int64_t seekTime, SeekMode mode) const
{
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(seekTime >= 0 && seekTime <= INT64_MAX / MS_TO_NS,
        Status::ERROR_INVALID_PARAMETER, "Seek time " PUBLIC_LOG_D64 " is not supported", seekTime);
    FALSE_RETURN_V_MSG_E(g_seekMode.find(mode) != g_seekMode.end(),
        Status::ERROR_INVALID_PARAMETER,
        "Seek mode " PUBLIC_LOG_U32 " is not supported", static_cast<uint32_t>(mode));
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SeekTo(int32_t trackId, int64_t seekTime, SeekMode mode, int64_t &realSeekTime)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace("SeekTo");

    Status check = CheckSeekParams(seekTime, mode);
    FALSE_RETURN_V_MSG_E(check == Status::OK, check, "CheckSeekParams failed");
    FALSE_RETURN_V_MSG_E(CheckSeekTimeInFileDuration(mediaInfo_, seekTime), Status::ERROR_INVALID_OPERATION,
        "Seek time out of file duration");

    Status denseRet = EnsureSelectedTracksDense();
    FALSE_RETURN_V_MSG_E(denseRet == Status::OK, denseRet, "Build dense sample index failed");
    for (const auto& selectedTrackId : selectedTrackIds_) {
        ClearTrackCache(selectedTrackId);
    }
    cacheSamples_.clear();
    pendingWebVttSamples_.clear();
    readToEnd_ = false;

    trackId = SelectSeekTrack();
    Status ret = FindBaseTrackAndCalcDelay(seekTime, mode, trackId); // find base track
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Find basetrack failed");
    return DoSeekInternal(trackId, seekTime, mode, realSeekTime);
}

Status MPEG4DemuxerPlugin::EnsureSelectedTracksDense()
{
    // Dense index construction mutates sample-helper state, so no async reader may access it concurrently.
    ReleaseReadLoop();
    for (uint32_t trackId : selectedTrackIds_) {
        auto track = FindTrackById(static_cast<int32_t>(trackId));
        FALSE_RETURN_V_NOLOG(track != nullptr && track->sampleHelper != nullptr, Status::ERROR_INVALID_DATA);
        Status ret = track->sampleHelper->EnsureDenseIndex();
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build dense index failed for track " PUBLIC_LOG_U32,
            trackId);
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SeekToStart()
{
    MEDIA_LOG_D("In");
    MediaAVCodec::AVCodecTrace trace("SeekToStart");
    auto ret = EnsureSelectedTracksDense();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build dense sample index failed");
    ret = SeekToStartInternal();
    if (ret != Status::OK) {
        MEDIA_LOG_I("Use default seekto");
        int64_t realSeekTime = 0;
        ret = SeekTo(-1, 0, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "SeekToStart failed");
    return Status::OK;
}

// LCOV_EXCL_START
Status MPEG4DemuxerPlugin::ParseMoof(int64_t offset)
{
    auto parser = std::make_shared<MPEG4AtomParser>();
    FALSE_RETURN_V_MSG_E(parser != nullptr, Status::ERROR_NO_MEMORY, "parser allocation failed");
    auto dataReader = std::make_shared<DemuxerDataReader>();
    FALSE_RETURN_V_MSG_E(dataReader != nullptr, Status::ERROR_NO_MEMORY, "dataReader allocation failed");
    dataReader->SetDataReader(dataSource_);
    int64_t timeoutMs = readTimeoutMs_.load(std::memory_order_acquire);
    if (timeoutMs >= 0) {
        dataReader->SetReadTimeoutForInitSeek(timeoutMs);
    }
    parser->SetDataReader(dataReader);
    parser->SetSeekable(seekable_);
    parser->GetFragmentEntry(fragmentEntry_);
    Status ret = parser->ParseMoof(firstTrack_, offset);
    parser->GetFragmentEntry(fragmentEntry_);

    return ret;
}
// LCOV_EXCL_STOP

Status MPEG4DemuxerPlugin::FindValidSample(SampleReadInfo& sampleInfo)
{
    // Keep a stable snapshot because the asynchronous read loop does not hold sharedMutex_.
    const std::vector<uint32_t> trackIds = selectedTrackIds_.empty() ? avTrackIds_ : selectedTrackIds_;
    for (const auto& trackId : trackIds) {
        auto currentTrack = FindTrackById(trackId);
        FALSE_RETURN_V_MSG_E(currentTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");

        uint32_t index = currentTrack->currentSampleIndex;
        uint32_t numSamples = static_cast<uint32_t>(currentTrack->sampleHelper->GetSampleCount());
        if (index >= numSamples) {
            continue;
        }
        MPEG4SampleHelper::SampleIndexView currentView;
        FALSE_RETURN_V_MSG_E(GetSampleView(currentTrack, index, currentView), Status::ERROR_INVALID_DATA,
            "Get sample view failed");
        if (!sampleInfo.found || currentView.pos < sampleInfo.view.pos) {
            sampleInfo.trackId = currentTrack->trackIndex;
            sampleInfo.sampleIndex = index;
            sampleInfo.view = currentView;
            sampleInfo.found = true;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetUnseekableSampleInfo(SampleReadInfo& sampleInfo)
{
    FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE, "ReadSample interrupt");
    bool hasFoundSample = false;
    bool tryNextMoof = false;
    while (!hasFoundSample) {
        sampleInfo = SampleReadInfo {};
        FALSE_RETURN_V(FindValidSample(sampleInfo) == Status::OK, Status::ERROR_NULL_POINTER);
        if (!sampleInfo.found && !fragmentEntry_.empty() && !tryNextMoof && !readToEnd_) {
            // All moof should be parsed during initialization now
            // If still no sample, we've reached end of all fragments
            return Status::END_OF_STREAM;
        } else if (!sampleInfo.found) {
            return Status::END_OF_STREAM;
        }

        MEDIA_LOG_D("Sample: pos=" PUBLIC_LOG_D64 ", size=" PUBLIC_LOG_D32 ", trackId=" PUBLIC_LOG_U32,
            sampleInfo.view.pos, sampleInfo.view.size, sampleInfo.trackId);
        hasFoundSample = true;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetSeekableSampleInfo(uint32_t trackId, SampleReadInfo& sampleInfo)
{
    FALSE_RETURN_V_MSG_E(!isInterruptNeeded_.load(), Status::ERROR_WRONG_STATE, "ReadSample interrupt");
    auto track = FindTrackById(trackId);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    bool hasFoundSample = false;
    bool checkedFragment = false;

    while (!hasFoundSample) {
        uint32_t numSamples = static_cast<uint32_t>(track->sampleHelper->GetSampleCount());
        uint32_t index = track->currentSampleIndex;
        MPEG4SampleHelper::SampleIndexView sample;
        if (index < numSamples) {
            FALSE_RETURN_V_MSG_E(GetSampleView(track, index, sample), Status::ERROR_INVALID_DATA,
                "Get sample view failed");
        }
        if (hasMoofBox_ && !checkedFragment) {
            int64_t nextMoofOffset = (currentFragment_ < fragmentEntry_.size() - 1) ?
                fragmentEntry_[currentFragment_ + 1].moofOffset : -1;
            checkedFragment = true;
            if (index >= numSamples || (sample.pos > nextMoofOffset && nextMoofOffset != -1)) {
                Status ret = ParseMoof(nextMoofOffset);
                FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ParseMoof failed");
                track = FindTrackById(trackId);
                FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
                numSamples = static_cast<uint32_t>(track->sampleHelper->GetSampleCount());
                FALSE_RETURN_V_MSG_E(index < numSamples, Status::ERROR_INVALID_DATA,
                    "ParseMoof did not produce enough samples");
                FALSE_RETURN_V_MSG_E(GetSampleView(track, index, sample), Status::ERROR_INVALID_DATA,
                    "Get sample view failed");
                ++currentFragment_;
            } else if (index >= numSamples && nextMoofOffset == -1) {
                return Status::END_OF_STREAM;
            }
        } else {
            if (index < numSamples) {
                sampleInfo.trackId = trackId;
                sampleInfo.sampleIndex = index;
                sampleInfo.view = sample;
                sampleInfo.found = true;
                MEDIA_LOG_D("next sample pos=" PUBLIC_LOG_D64 ", size=" PUBLIC_LOG_D32,
                    sampleInfo.view.pos, sampleInfo.view.size);
                hasFoundSample = true;
            } else {
                return Status::END_OF_STREAM;
            }
        }
    }

    return Status::OK;
}

Status MPEG4DemuxerPlugin::WriteSampleAttr(const SampleReadInfo& sampleInfo, std::shared_ptr<Sample> sample)
{
    auto track = FindTrackById(sampleInfo.trackId);
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    const auto& currentSample = sampleInfo.view;
    int32_t trackTimeScale = track->timeScale;
    FALSE_RETURN_V_NOLOG(trackTimeScale > 0, Status::ERROR_INVALID_PARAMETER);

    ReadSamplePacketAttr attr;
    ReadSamplePacketContext context { mediaInfo_, track, currentSample, rawDtsPrefix_, sampleInfo.sampleIndex,
        trackTimeScale, movieTimeScale_,
        HasPlayableVideoTrack(firstTrack_) };
    Status ret = BuildReadSamplePacketAttr(context, attr);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build read sample packet attr failed");
    sample->pts = attr.ptsUs;
    sample->dts = attr.dtsUs;
    sample->duration = attr.durationUs;
    sample->pos = currentSample.pos;
    sample->flag = attr.flag;
    sample->size = currentSample.size;
    sample->skipSamplesInfo = std::move(attr.skipSamplesInfo);
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
    FALSE_RETURN_V_NOLOG(sample->size >= BOX_HEADER_SIZE, Status::OK);

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
        if (tempSize < BOX_HEADER_SIZE || tempSize > static_cast<uint32_t>(payloadSize)) {
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
                payloadSize -= static_cast<int32_t>(tempSize);
            }
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ReadSampleData(
    std::shared_ptr<MPEG4Sample>& mpeg4Sample, const SampleReadInfo& sampleInfo)
{
    auto it = cacheSamples_.find(sampleInfo.trackId);
    if (it != cacheSamples_.end()) {
        mpeg4Sample = it->second;
        return Status::OK;
    }
    auto sample = std::make_shared<Sample>();
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate sample");
    mpeg4Sample->sample = sample;

    return ReadSampleData(sample, sampleInfo);
}

void MPEG4DemuxerPlugin::ClearTrackCache(uint32_t trackId)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    if (track != nullptr && track->cache != nullptr) {
        track->cache->Clear();
    }
}

bool MPEG4DemuxerPlugin::HasTrackCache(uint32_t trackId)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    return HasTrackCache(track);
}

bool MPEG4DemuxerPlugin::HasTrackCache(const std::shared_ptr<MPEG4AtomParser::Track>& track) const
{
    return track != nullptr && track->cache != nullptr && track->cache->HasCache();
}

uint32_t MPEG4DemuxerPlugin::GetTrackCacheDataSize(uint32_t trackId)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    return track != nullptr && track->cache != nullptr ? track->cache->GetCacheDataSize() : 0;
}

uint32_t MPEG4DemuxerPlugin::GetTrackCacheFrameCount(uint32_t trackId)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    return track != nullptr && track->cache != nullptr ? track->cache->GetCacheFrameCount() : 0;
}

bool MPEG4DemuxerPlugin::ResetTrackCacheInfo(const std::shared_ptr<MPEG4Sample>& mpeg4Sample)
{
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, false, "MPEG4 sample is nullptr");
    auto track = FindTrackById(static_cast<int32_t>(mpeg4Sample->trackId));
    return track != nullptr && track->cache != nullptr && track->cache->ResetInfo(mpeg4Sample);
}

bool MPEG4DemuxerPlugin::SetTrackCacheInfo(const std::shared_ptr<MPEG4Sample>& mpeg4Sample)
{
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, false, "MPEG4 sample is nullptr");
    auto track = FindTrackById(static_cast<int32_t>(mpeg4Sample->trackId));
    return track != nullptr && track->cache != nullptr && track->cache->SetInfo(mpeg4Sample);
}

bool MPEG4DemuxerPlugin::PushTrackCache(uint32_t trackId, const std::shared_ptr<MPEG4Sample>& mpeg4Sample)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    if (track == nullptr || track->cache == nullptr) {
        return false;
    }
    return track->cache->Push(mpeg4Sample);
}

std::shared_ptr<MPEG4Sample> MPEG4DemuxerPlugin::PopTrackCache(uint32_t trackId)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    return track != nullptr && track->cache != nullptr ? track->cache->Pop() : nullptr;
}

std::shared_ptr<MPEG4Sample> MPEG4DemuxerPlugin::FrontTrackCache(uint32_t trackId)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    return track != nullptr && track->cache != nullptr ? track->cache->Front() : nullptr;
}

std::shared_ptr<MPEG4Sample> MPEG4DemuxerPlugin::BackTrackCache(uint32_t trackId)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    return track != nullptr && track->cache != nullptr ? track->cache->Back() : nullptr;
}

Status MPEG4DemuxerPlugin::GetTrackCacheLastPTS(uint32_t trackId, int64_t& maxPts)
{
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    if (track == nullptr || track->cache == nullptr) {
        maxPts = INT64_MIN;
        return Status::ERROR_NOT_EXISTED;
    }
    return track->cache->GetLastPTS(maxPts);
}

Status MPEG4DemuxerPlugin::AddMPEG4SampleToCacheQueue(
    std::shared_ptr<MPEG4Sample> cacheSample, const uint32_t trackId, uint32_t sampleIndex)
{
    Status ret = Status::OK;
    cacheSample->offset = 0;
    cacheSample->trackId = trackId;
    if (sampleIndex == UINT32_MAX) {
        auto track = FindTrackById(static_cast<int32_t>(trackId));
        cacheSample->sampleIndex = track ? track->currentSampleIndex : 0;
    } else {
        cacheSample->sampleIndex = sampleIndex;
    }
    (void)PushTrackCache(trackId, cacheSample);
    ret = CheckCacheDataLimit(trackId);
    uint32_t cacheBytes = GetTrackCacheDataSize(trackId);
    if (cacheBytes < UINT32_MAX) {
        MaybeNotifyCachePressure(trackId, cacheBytes);
    }
    return ret;
}

Status MPEG4DemuxerPlugin::AddSampleToCacheQueue(std::shared_ptr<Sample> sample,
    const uint32_t trackId, uint32_t sampleIndex)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NULL_POINTER, "sample is nullptr");

    std::shared_ptr<MPEG4Sample> cacheSample = std::make_shared<MPEG4Sample>();
    FALSE_RETURN_V_MSG_E(cacheSample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate MPEG4Sample");
    cacheSample->sample = sample;
    return AddMPEG4SampleToCacheQueue(cacheSample, trackId, sampleIndex);
}

Status MPEG4DemuxerPlugin::CheckCacheDataLimit(uint32_t trackId)
{
    if (!outOfLimit_) {
        auto cacheDataSize = GetTrackCacheDataSize(trackId);
        if (cacheDataSize > cachelimitSize_) {
            MEDIA_LOG_W("Track " PUBLIC_LOG_U32 " cache out of limit: " PUBLIC_LOG_U32 "/" PUBLIC_LOG_U32 ", by user "
                PUBLIC_LOG_D32, trackId, cacheDataSize, cachelimitSize_, static_cast<int32_t>(setLimitByUser_));
            outOfLimit_ = true;
        }
    }
    return Status::OK;
}

bool MPEG4DemuxerPlugin::GetAnnexbOutputBufferSize(uint32_t trackId, const std::shared_ptr<Sample>& sample,
    int32_t& outputBufferSize)
{
    outputBufferSize = 0;
    if (sample == nullptr || sample->size < 0) {
        return false;
    }
    auto track = FindTrackById(trackId);
    if (track == nullptr) {
        return false;
    }
    int32_t codecConfigSize = 0;
    if (track->codecParms.hasValidExtradata) {
        if (track->codecParms.annexbExtradataSize < 0 || track->codecParms.annexbExtradataSize > INT32_MAX) {
            return false;
        }
        codecConfigSize = static_cast<int32_t>(track->codecParms.annexbExtradataSize);
    }
    if (sample->size > INT32_MAX - codecConfigSize) {
        MEDIA_LOG_E("Annexb output buffer size overflow, sample=" PUBLIC_LOG_D32 ", config=" PUBLIC_LOG_D32,
            sample->size, codecConfigSize);
        return false;
    }
    outputBufferSize = sample->size + codecConfigSize;
    return true;
}

bool MPEG4DemuxerPlugin::CanConvertSampleToAnnexbDirectly(const std::shared_ptr<MPEG4Sample>& mpeg4Sample,
    uint32_t trackId, bool& isAvc)
{
    isAvc = false;
    if (mpeg4Sample == nullptr || mpeg4Sample->sample == nullptr || mpeg4Sample->isAnnexb ||
        mpeg4Sample->offset != 0 || mpeg4Sample->sample->data == nullptr) {
        return false;
    }
    auto track = FindTrackById(trackId);
    if (track == nullptr || track->sampleHelper == nullptr) {
        return false;
    }
    const std::string& mime = track->sampleHelper->mimeType_;
    isAvc = mime == MimeType::VIDEO_AVC;
    return HaveValidParser(mime) && streamParsers_ != nullptr && streamParsers_->ParserIsInited(trackId);
}

Status MPEG4DemuxerPlugin::PrepareAnnexbBufferConversion(const std::shared_ptr<MPEG4Sample>& mpeg4Sample,
    uint32_t trackId, AnnexbBufferConvertContext& context)
{
    if (context.isPrepared) {
        return Status::OK;
    }
    context.handled = false;
    context.shouldConvert = false;
    context.outputDataSize = 0;
    context.isAvc = false;
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr,
        Status::ERROR_NULL_POINTER, "Sample is nullptr");
    if (context.outputBuffer == nullptr || context.outputBufferSize < 0 ||
        !CanConvertSampleToAnnexbDirectly(mpeg4Sample, trackId, context.isAvc)) {
        return Status::OK;
    }

    int32_t requiredBufferSize = 0;
    FALSE_RETURN_V_MSG_E(GetAnnexbOutputBufferSize(trackId, mpeg4Sample->sample, requiredBufferSize),
        Status::ERROR_INVALID_DATA, "Invalid annexb output buffer size");
    context.shouldConvert = context.outputBufferSize >= requiredBufferSize;
    context.isPrepared = true;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::CopyOriginalSampleToAnnexbBuffer(const std::shared_ptr<Sample>& srcSample,
    AnnexbBufferConvertContext& context)
{
    FALSE_RETURN_V_MSG_E(srcSample->size <= context.outputBufferSize &&
        memcpy_s(context.outputBuffer, context.outputBufferSize, srcSample->data.get(), srcSample->size) == EOK,
        Status::ERROR_UNKNOWN, "Copy original packet failed");
    context.outputDataSize = srcSample->size;
    context.handled = true;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::HandleAnnexbBufferConversionResult(const std::shared_ptr<Sample>& srcSample,
    AnnexbBufferConvertContext& context, bool convertRet)
{
    FALSE_RETURN_V_MSG_E(context.outputDataSize >= 0, Status::ERROR_PACKET_CONVERT_FAILED,
        "Invalid annexb output size:" PUBLIC_LOG_D32, context.outputDataSize);
    if (convertRet && context.outputDataSize > 0) {
        FALSE_RETURN_V_MSG_E(context.outputDataSize <= context.outputBufferSize, Status::ERROR_PACKET_CONVERT_FAILED,
            "Invalid annexb output size:" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32,
            context.outputDataSize, context.outputBufferSize);
        context.handled = true;
        return Status::OK;
    }
    if (!convertRet && context.isAvc && context.outputDataSize > context.outputBufferSize) {
        return Status::OK;
    }
    FALSE_RETURN_V_MSG_E(context.outputDataSize <= context.outputBufferSize, Status::ERROR_PACKET_CONVERT_FAILED,
        "Invalid annexb output size:" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32,
        context.outputDataSize, context.outputBufferSize);
    FALSE_RETURN_V_MSG_E(!context.isAvc, Status::ERROR_PACKET_CONVERT_FAILED,
        "Convert avc packet to annexb failed");
    FALSE_RETURN_V_MSG_E(convertRet || context.outputDataSize == 0, Status::ERROR_PACKET_CONVERT_FAILED,
        "Annexb conversion failed after producing output");
    return CopyOriginalSampleToAnnexbBuffer(srcSample, context);
}

Status MPEG4DemuxerPlugin::TryConvertSampleToAnnexbToBuffer(const std::shared_ptr<MPEG4Sample>& mpeg4Sample,
    uint32_t trackId, AnnexbBufferConvertContext& context)
{
    Status ret = PrepareAnnexbBufferConversion(mpeg4Sample, trackId, context);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare annexb buffer conversion failed");
    if (!context.shouldConvert) {
        return Status::OK;
    }

    auto srcSample = mpeg4Sample->sample;
    PacketConvertToBufferInfo convertInfo(context.outputDataSize);
    convertInfo.srcData = srcSample->data.get();
    convertInfo.srcDataSize = srcSample->size;
    convertInfo.outBuffer = context.outputBuffer;
    convertInfo.outBufferSize = context.outputBufferSize;
    bool convertRet = streamParsers_->ConvertPacketToAnnexb(trackId, convertInfo);
    return HandleAnnexbBufferConversionResult(srcSample, context, convertRet);
}

Status MPEG4DemuxerPlugin::ConvertSampleToAnnexb(std::shared_ptr<MPEG4Sample> dstMpeg4Sample, const uint32_t trackIndex)
{
    Status ret = Status::OK;
    FALSE_RETURN_V_MSG_D(!dstMpeg4Sample->isAnnexb, ret, "Has converted");
    auto track = FindTrackById(trackIndex);
    if (track == nullptr || track->sampleHelper == nullptr) {
        dstMpeg4Sample->isAnnexb = true;
        return ret;
    }
    const std::string& mime = track->sampleHelper->mimeType_;
    if (HaveValidParser(mime) && streamParsers_ != nullptr &&
        streamParsers_->ParserIsInited(trackIndex)) {
        auto srcSample = dstMpeg4Sample->sample;
        uint8_t* dataPtr = srcSample->data.get();
        PacketConvertInfo convertInfo {nullptr, 0, false};
        bool convertRet = streamParsers_->ConvertPacketToAnnexb(trackIndex, &dataPtr, srcSample->size, convertInfo);
        if (mime == MimeType::VIDEO_AVC) { // only avc will return ERROR_PACKET_CONVERT_FAILED
            FALSE_RETURN_V_MSG_E(convertRet, Status::ERROR_PACKET_CONVERT_FAILED, "Convert packet to annexb failed");
        }
        if (dataPtr != srcSample->data.get()) {
            std::unique_ptr<uint8_t[]> newData = std::make_unique<uint8_t[]>(srcSample->size);
            FALSE_RETURN_V_MSG_E(memcpy_s(newData.get(), srcSample->size, dataPtr, srcSample->size) == EOK,
                Status::ERROR_UNKNOWN, "Copy annexb data failed");
            srcSample->data = std::move(newData);
        }
    }
    dstMpeg4Sample->isAnnexb = true;
    return ret;
}

Status MPEG4DemuxerPlugin::SetEosSample(std::shared_ptr<AVBuffer> outBuffer)
{
    outBuffer->pts_ = 0;
    outBuffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    MEDIA_LOG_D("Read to end");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::UpdateWebVttCachedSampleDuration(const WebVttReadContext& context,
    const std::shared_ptr<MPEG4Sample>& tempSample, uint32_t nextSampleIndex)
{
    FALSE_RETURN_V_NOLOG(HasTrackCache(context.track), Status::OK);
    std::shared_ptr<MPEG4Sample> cacheSample = BackTrackCache(context.trackId);
    FALSE_RETURN_V_NOLOG(!(tempSample == nullptr && (cacheSample == nullptr || cacheSample->sample == nullptr)),
        Status::OK);
    FALSE_RETURN_V_MSG_E(cacheSample != nullptr && cacheSample->sample != nullptr,
        Status::ERROR_NULL_POINTER, "Cache sample is nullptr");
    if (tempSample != nullptr && tempSample->sample != nullptr && tempSample->sample->pts > cacheSample->sample->pts) {
        cacheSample->sample->duration = tempSample->sample->pts - cacheSample->sample->pts;
    }
    WebVttTailDurationContext tailContext {
        mediaInfo_, context.track, context.trackId, nextSampleIndex, movieTimeScale_
    };
    (void)UpdateWebVttTailDurationFromStream(tailContext, cacheSample->sample);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ReadAndCacheWebVttSample(const WebVttReadContext& context, bool& continueRead)
{
    continueRead = false;
    std::shared_ptr<MPEG4Sample> tempSample = std::make_shared<MPEG4Sample>();
    FALSE_RETURN_V_MSG_E(tempSample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate MPEG4Sample");
    tempSample->trackId = context.trackId;
    Status ret = GetSampleBySeekableStatus(context.trackId, tempSample, context.outBuffer);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "GetSampleBySeekableStatus failed");
    if (context.outBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) {
        return UpdateWebVttCachedSampleDuration(context, nullptr, context.track->currentSampleIndex);
    }
    ret = ProcessWebVTTSample(tempSample);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ProcessWebVTTSample failed");
    if (tempSample->isVttc) {
        ret = AddSampleToCacheQueue(tempSample->sample, context.trackId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add cache failed");
        WebVttTailDurationContext tailContext {
            mediaInfo_, context.track, context.trackId, context.track->currentSampleIndex + 1, movieTimeScale_
        };
        (void)UpdateWebVttTailDurationFromStream(tailContext, tempSample->sample);
    } else {
        ret = UpdateWebVttCachedSampleDuration(context, tempSample, context.track->currentSampleIndex + 1);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Update WebVTT cache duration failed");
    }
    continueRead = tempSample->isVttc || !HasTrackCache(context.track);
    context.track->currentSampleIndex += continueRead ? 1 : 0;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::WriteBufferAttr(const SampleReadInfo& sampleInfo,
    std::shared_ptr<AVBuffer> outBuffer, int32_t dataSize, bool memoryNotEnough)
{
    auto track = FindTrackById(sampleInfo.trackId);
    FALSE_RETURN_V_MSG_E(dataSize >= 0 && dataSize <= outBuffer->memory_->GetCapacity(),
        Status::ERROR_INVALID_BUFFER_SIZE, "Invalid buffer data size");
    int32_t trackTimeScale = track->timeScale;
    FALSE_RETURN_V_NOLOG(trackTimeScale > 0, Status::ERROR_INVALID_PARAMETER);
    ReadSamplePacketAttr attr;
    ReadSamplePacketContext context {
        mediaInfo_, track, sampleInfo.view, rawDtsPrefix_, sampleInfo.sampleIndex,
        trackTimeScale, movieTimeScale_, HasPlayableVideoTrack(firstTrack_)};
    Status ret = BuildReadSamplePacketAttr(context, attr);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Build sample attr failed");
    outBuffer->pts_ = attr.ptsUs;
    outBuffer->dts_ = attr.dtsUs;
    outBuffer->duration_ = attr.durationUs;
    outBuffer->memory_->SetSize(dataSize);
    outBuffer->meta_->Set<Tag::BUFFER_DECODING_TIMESTAMP>(attr.dtsUs);
    outBuffer->meta_->Set<Tag::BUFFER_DURATION>(attr.durationUs);
    MEDIA_LOG_D("[%{public}u]CurrentBuffer: [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "]",
        sampleInfo.trackId, outBuffer->pts_, outBuffer->dts_, outBuffer->duration_);
    outBuffer->flag_ = GetFlags(attr.flag, memoryNotEnough);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetSeekableSampleDirectly(uint32_t trackId, std::shared_ptr<AVBuffer> outBuffer)
{
    std::unique_lock<std::mutex> sLock(syncMutex_);
    auto track = FindTrackById(trackId);
    SampleReadInfo sampleInfo;
    Status ret = GetSeekableSampleInfo(trackId, sampleInfo);
    if (ret == Status::END_OF_STREAM) {
        track->currentSamplePos = 0;
        return SetEosSample(outBuffer);
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get sample info failed");
    int64_t pos = sampleInfo.view.pos;
    int32_t sampleSize = sampleInfo.view.size;
    FALSE_RETURN_V_MSG_E(pos >= 0 && track->currentSamplePos <= INT64_MAX - pos,
        Status::ERROR_INVALID_PARAMETER, "Invalid sample pos");
    FALSE_RETURN_V_MSG_E(track->currentSamplePos >= 0 && track->currentSamplePos <= sampleSize,
        Status::ERROR_INVALID_PARAMETER, "Invalid current pos");

    int32_t capacity = outBuffer->memory_->GetCapacity();
    int32_t remainSize = sampleSize - track->currentSamplePos;
    int32_t readSize = std::min(remainSize, capacity);
    int64_t readPos = pos + track->currentSamplePos;
    bool capacityEnough = readSize >= remainSize;
    ret = CheckReadSampleDataState(readSize);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Invalid read state");
    if (readSize > 0) {
        ret = FillBufferDirectly(outBuffer, static_cast<uint64_t>(readPos), readSize);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read sample data failed");
    }
    ret = WriteBufferAttr(sampleInfo, outBuffer, readSize, !capacityEnough);
    FALSE_RETURN_V(ret == Status::OK, ret);
    MEDIA_LOG_D("Convert size [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "]",
        sampleSize, remainSize, readSize, track->currentSamplePos);
    if (readSize > 0) {
        DumpAVBufferData(outBuffer->memory_->GetAddr(), trackId, readSize, outBuffer->pts_);
    }
    if (!capacityEnough) {
        track->currentSamplePos += readSize;
        MEDIA_LOG_D("Buffer is not enough");
    } else {
        track->currentSamplePos = 0;
        track->currentSampleIndex++;
    }
    return ret;
}

Status MPEG4DemuxerPlugin::GetSampleForWebvtt(uint32_t trackId,
    std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer)
{
    auto track = FindTrackById(trackId);
    FALSE_RETURN_V_MSG_E(IsValidTrack(track), Status::ERROR_NULL_POINTER, "Track is nullptr");
    WebVttReadContext context { seekable_, trackId, track, outBuffer };
    bool continueRead = false;
    do {
        Status ret = ReadAndCacheWebVttSample(context, continueRead);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Read WebVTT sample failed");
    } while (continueRead);
    mpeg4Sample = PopTrackCache(trackId);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetSeekableSample(
    uint32_t trackId, std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer)
{
    SampleReadInfo sampleInfo;
    Status ret = GetSeekableSampleInfo(trackId, sampleInfo);
    if (ret == Status::END_OF_STREAM) {
        return outBuffer != nullptr ? SetEosSample(outBuffer) : Status::END_OF_STREAM;
    } else if (ret != Status::OK) {
        return ret;
    }
    mpeg4Sample->sampleIndex = sampleInfo.sampleIndex;
    std::unique_lock<std::mutex> sLock(syncMutex_);
    ret = ReadSampleData(mpeg4Sample, sampleInfo);
    sLock.unlock();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ReadSampleData failed");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetUnseekableSample(
    uint32_t trackId, std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer)
{
    Status ret = Status::OK;
    while (!HasTrackCache(trackId)) {
        SampleReadInfo sampleInfo;
        ret = GetUnseekableSampleInfo(sampleInfo);
        if (ret == Status::END_OF_STREAM) {
            readToEnd_ = true;
            return outBuffer != nullptr ? SetEosSample(outBuffer) : Status::END_OF_STREAM;
        } else if (ret != Status::OK) {
            return ret;
        }
        auto track = FindTrackById(sampleInfo.trackId);
        std::unique_lock<std::mutex> sLock(syncMutex_);
        auto sample = std::make_shared<Sample>();
        FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate sample");
        ret = ReadSampleData(sample, sampleInfo);
        sLock.unlock();
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ReadSampleData failed");
        ret = AddSampleToCacheQueue(sample, sampleInfo.trackId, sampleInfo.sampleIndex);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add cache failed");
        if (track) {
            track->currentSampleIndex++;
        }
    }
    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    mpeg4Sample = FrontTrackCache(trackId);
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, Status::ERROR_NULL_POINTER, "Cache sample is nullptr");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetSampleBySeekableStatus(
    uint32_t trackId, std::shared_ptr<MPEG4Sample> &mpeg4Sample, std::shared_ptr<AVBuffer> outBuffer)
{
    return (seekable_ == Seekable::SEEKABLE) ? GetSeekableSample(trackId, mpeg4Sample, outBuffer) :
                                               GetUnseekableSample(trackId, mpeg4Sample, outBuffer);
}

void MPEG4DemuxerPlugin::DumpAVBufferData(
    const uint8_t* data [[maybe_unused]], uint32_t trackId [[maybe_unused]],
    int32_t size [[maybe_unused]], int64_t pts [[maybe_unused]])
{
#ifdef BUILD_ENG_VERSION
    DumpParam dumpParam {DumpMode(DUMP_AVBUFFER_OUTPUT & dumpMode_), const_cast<uint8_t*>(data),
        static_cast<int>(trackId), -1, size, trackDfxInfoMap_[trackId].frameIndex++, pts, -1};
    Dump(dumpParam);
#endif
}

uint32_t MPEG4DemuxerPlugin::GetFlags(uint32_t sampleFlag, bool memoryNotEnough)
{
    uint32_t flags = static_cast<uint32_t>(AVBufferFlag::NONE);
    if (sampleFlag & Sample::SampleFlag::SYNC_FRAME) {
        flags |= static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME);
        flags |= static_cast<uint32_t>(AVBufferFlag::CODEC_DATA);
    }
    if (sampleFlag & Sample::SampleFlag::DISCARD) {
        flags |= static_cast<uint32_t>(AVBufferFlag::DISCARD);
    }
    if (memoryNotEnough) {
        flags |= static_cast<uint32_t>(AVBufferFlag::PARTIAL_FRAME);
    }
    return flags;
}

void MPEG4DemuxerPlugin::SetOutputBufferTiming(uint32_t trackIndex, const std::shared_ptr<AVBuffer>& outBuffer,
    const std::shared_ptr<Sample>& sample)
{
    outBuffer->pts_ = sample->pts;
    outBuffer->dts_ = sample->dts;
    outBuffer->meta_->Set<Tag::BUFFER_DECODING_TIMESTAMP>(sample->dts);
    outBuffer->duration_ = sample->duration;
    outBuffer->meta_->Set<Tag::BUFFER_DURATION>(sample->duration);
    MEDIA_LOG_D("CurrentBuffer: [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "], track: "
        PUBLIC_LOG_U32, outBuffer->pts_, outBuffer->dts_, outBuffer->duration_, trackIndex);
}

void MPEG4DemuxerPlugin::FinishDirectAnnexbBufferWrite(uint32_t trackIndex, const std::shared_ptr<AVBuffer>& outBuffer,
    const std::shared_ptr<MPEG4Sample>& mpeg4Sample, int32_t oldSize, const AnnexbBufferConvertContext& context)
{
    outBuffer->memory_->SetSize(context.outputDataSize);
    outBuffer->flag_ = GetFlags(mpeg4Sample->sample->flag, false);
    DumpAVBufferData(outBuffer->memory_->GetAddr(), trackIndex, context.outputDataSize, outBuffer->pts_);
    SetDrmCencInfo(trackIndex, outBuffer, context.outputDataSize, oldSize, mpeg4Sample->sampleIndex);
}

Status MPEG4DemuxerPlugin::WriteConvertedSampleToBuffer(uint32_t trackIndex, const std::shared_ptr<AVBuffer>& outBuffer,
    const std::shared_ptr<MPEG4Sample>& mpeg4Sample, int32_t oldSize)
{
    if (ResetTrackCacheInfo(mpeg4Sample) == false) {
        MEDIA_LOG_D("Reset info failed");
    }
    Status ret = ConvertSampleToAnnexb(mpeg4Sample, trackIndex);
    if (SetTrackCacheInfo(mpeg4Sample) == false) {
        MEDIA_LOG_D("Set info failed");
    }
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert annexb failed");

    int32_t remainSize = mpeg4Sample->sample->size - mpeg4Sample->offset;
    int32_t copySize = remainSize < outBuffer->memory_->GetCapacity() ? remainSize : outBuffer->memory_->GetCapacity();
    MEDIA_LOG_D("Convert size [" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "/" PUBLIC_LOG_D32 "]",
        mpeg4Sample->sample->size, remainSize, copySize, mpeg4Sample->offset);
    outBuffer->flag_ = GetFlags(mpeg4Sample->sample->flag, copySize != remainSize);

    // copy data
    if (copySize > 0) {
        FALSE_RETURN_V_MSG_E(mpeg4Sample->sample->data != nullptr && mpeg4Sample->offset >= 0 &&
            mpeg4Sample->offset <= mpeg4Sample->sample->size &&
            copySize <= mpeg4Sample->sample->size - mpeg4Sample->offset,
            Status::ERROR_INVALID_DATA, "Invalid sample data range");
        const uint8_t *writeData = mpeg4Sample->sample->data.get() + mpeg4Sample->offset;
        int32_t writeResult = outBuffer->memory_->Write(writeData, copySize, 0);
        FALSE_RETURN_V_MSG_E(writeResult >= 0, Status::ERROR_INVALID_OPERATION, "Memory write failed");
        DumpAVBufferData(writeData, trackIndex, copySize, outBuffer->pts_);
    }
    outBuffer->memory_->SetSize(copySize);
    SetDrmCencInfo(trackIndex, outBuffer, mpeg4Sample->sample->size, oldSize, mpeg4Sample->sampleIndex);
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

Status MPEG4DemuxerPlugin::WriteBuffer(uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer,
    std::shared_ptr<MPEG4Sample> mpeg4Sample)
{
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_NULL_POINTER, "Buffer is nullptr");
    SetOutputBufferTiming(trackIndex, outBuffer, mpeg4Sample->sample);
    CheckResetXPSSendStatus(trackIndex, mpeg4Sample->sample);
    FALSE_RETURN_V_MSG_E(outBuffer->memory_ != nullptr, Status::ERROR_NULL_POINTER, "Memory is nullptr");
    int32_t oldSize = mpeg4Sample->sample->size;
    AnnexbBufferConvertContext context;
    context.outputBuffer = outBuffer->memory_->GetAddr();
    context.outputBufferSize = outBuffer->memory_->GetCapacity();
    Status ret = TryConvertSampleToAnnexbToBuffer(mpeg4Sample, trackIndex, context);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert annexb directly failed");
    if (context.handled) {
        FinishDirectAnnexbBufferWrite(trackIndex, outBuffer, mpeg4Sample, oldSize, context);
        return Status::OK;
    }
    return WriteConvertedSampleToBuffer(trackIndex, outBuffer, mpeg4Sample, oldSize);
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
    if (FirstFrameValid(trackIndex)) {
        const auto& firstFrame = firstFrameMap_[trackIndex];
        if (sample->pos >= 0 && firstFrame->pos >= 0) {
            if (sample->pos == firstFrame->pos) {
                streamParsers_->ResetXPSSendStatus(trackIndex);
            }
            return;
        }
    }
    if (sample->dts != INT64_MIN && sample->dts <= 0) {
        streamParsers_->ResetXPSSendStatus(trackIndex);
    }
}

Status MPEG4DemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> outBuffer)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace(std::string("ReadSample_") + std::to_string(trackId));
    MEDIA_LOG_D("In");
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(IsValidBuffer(outBuffer, false), Status::ERROR_INVALID_PARAMETER, "AVBuffer is invalid");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::ASYNC)) == 0,
        Status::ERROR_INVALID_OPERATION, "Cannot use sync and async Read together");
    auto track = FindTrackById(trackId);
    FALSE_RETURN_V_MSG_E(IsValidTrack(track), Status::ERROR_INVALID_PARAMETER, "Track is invalid");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::SYNC), std::memory_order_acq_rel);
    bool useCache = HasTrackCache(track);
    if (!useCache && seekable_ == Seekable::SEEKABLE && !NeedReadPostProcessing(track->sampleHelper->mimeType_) &&
        !track->drmInfo.isEncrypted && outBuffer->memory_->GetCapacity() > 0) {
        return GetSeekableSampleDirectly(trackId, outBuffer);
    }
    std::shared_ptr<MPEG4Sample> mpeg4Sample = useCache ? FrontTrackCache(trackId) : std::make_shared<MPEG4Sample>();
    FALSE_RETURN_V_MSG_E(
        mpeg4Sample != nullptr, Status::ERROR_NO_MEMORY, "Get mpeg4Sample failed[%{public}d]", useCache);
    mpeg4Sample->trackId = trackId;
    Status ret = Status::OK;
    if (!useCache) {
        if (track->sampleHelper->mimeType_ == MimeType::TEXT_WEBVTT) {
            ret = GetSampleForWebvtt(trackId, mpeg4Sample, outBuffer);
        } else {
            ret = GetSampleBySeekableStatus(trackId, mpeg4Sample, outBuffer);
        }
        FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    }
    if (IsValidMpeg4Sample(mpeg4Sample)) {
        ret = WriteBuffer(trackId, outBuffer, mpeg4Sample);
    }
    FALSE_RETURN_V_NOLOG(ret != Status::ERROR_NOT_ENOUGH_DATA, Status::OK);
    FALSE_RETURN_V_NOLOG(ret == Status::OK || ret == Status::ERROR_PACKET_CONVERT_FAILED, ret);
    if (seekable_ == Seekable::UNSEEKABLE || useCache) {
        PopTrackCache(trackId);
    } else {
        cacheSamples_.erase(trackId);
        ++track->currentSampleIndex;
    }
    return ret;
}

Status MPEG4DemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t &sampleSize)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace(std::string("GetNextSampleSize_") + std::to_string(trackId));
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
    if (HasTrackCache(trackId)) {
        std::shared_ptr<MPEG4Sample> mpeg4Sample = FrontTrackCache(trackId);
        FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr,
            Status::ERROR_NULL_POINTER, "Cache sample is nullptr");
        sample = mpeg4Sample->sample;
    } else {
        SampleReadInfo sampleInfo;
        Status ret = Status::OK;
        if (seekable_ == Seekable::SEEKABLE) {
            ret = GetSeekableSampleInfo(trackId, sampleInfo);
        } else if (seekable_ == Seekable::UNSEEKABLE) {
            ret = GetUnseekableSampleInfo(sampleInfo);
        }
        if (ret == Status::END_OF_STREAM) {
            return ret;
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Get next sample info failed");
        FALSE_RETURN_V_MSG_E(sampleInfo.found, Status::ERROR_INVALID_DATA, "Sample info is invalid");
        sample->size = sampleInfo.view.size;
        sample->flag = sampleInfo.view.flag;
        sampleTrackId = sampleInfo.trackId;
    }

    sampleSize = GetSampleSizeWithCodecConfig(sample, sampleTrackId);
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
    uint32_t dataSize = GetTrackCacheDataSize(trackId);
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

    uint32_t cachedFrames = GetTrackCacheFrameCount(trackId); // 返回对应缓存的samplePacket个数
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
        if (tagType[0] == static_cast<uint32_t>(FourccType("hdlr")) &&
            tagType[secondTagOffset] == static_cast<uint32_t>(FourccType("mhlr")) &&
            tagType[thirdTagOffset] == static_cast<uint32_t>(FourccType("MPEG"))) {
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
    uint32_t numBrands = static_cast<uint32_t>((chunkDataSize - READ_LEN) / readLengHalf);
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
        brand = GetU32Value(brandData, sizeof(brandData));
        if (IsSupportedBrand(brand)) {
            return FIRST_LEVEL_RANK;
        }
        if (IsUnsupportedBrand(brand)) {
            return 0;
        }
        if (i == 0) {
            confidence = std::max(confidence, RANK_MIN);
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
        atom.size = GetU64Value(sizeData, sizeof(sizeData));
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
    bool isFirstAtom = true;
    while (!moov.found && offset < SNIFF_DATA_SIZE) {
        uint8_t atomInfoData[8];
        FALSE_RETURN_V_MSG_E(dataReader->ReadUintData(offset, atomInfoData, sizeof(atomInfoData)) == Status::OK, 0,
            "Read atom failed");
        AtomInfo atom {GetU32Value(&atomInfoData[0]), GetU32Value(&atomInfoData[4])};
        int64_t chunkDataOffset = offset + READ_LEN;
        FALSE_RETURN_V_NOLOG(UpdateAtomSize(atom, offset, chunkDataOffset, dataReader), 0);
        int64_t chunkDataSize = static_cast<int64_t>(atom.size) - (chunkDataOffset - offset);
        FALSE_RETURN_V_MSG_E(chunkDataSize >= 0, 0, "atom data size error");
        FALSE_RETURN_V_MSG_E((!isFirstAtom || atom.type == static_cast<uint32_t>(FourccType("ftyp"))),
                             0, "no ftyp atom");
        isFirstAtom = false;
        switch (GetAtomLevel(atom)) {
            case AtomLevel::LEVEL_0: {
                int32_t originalConfidence = confidence;
                FALSE_RETURN_V_MSG_E(atom.size < MPEG4_MAX_BOX_DATA_SIZE, 0, "atom data size too large");
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
    std::string skip = OHOS::system::GetParameter("Mpeg4DemuxerPlugin.skip", "0");
    FALSE_RETURN_V_MSG_W(skip == "0", 0, "Skip Mpeg4Demuxer");
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
