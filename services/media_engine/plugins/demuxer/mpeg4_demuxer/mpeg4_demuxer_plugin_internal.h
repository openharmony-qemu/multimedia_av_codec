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

#ifndef MPEG4_DEMUXER_PLUGIN_INTERNAL_H
#define MPEG4_DEMUXER_PLUGIN_INTERNAL_H

#include <cstdint>
#include <memory>

#include "buffer/avbuffer.h"
#include "mpeg4_box_parser.h"
#include "mpeg4_sample_helper.h"
#include "plugin/demuxer_plugin.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
namespace detail {

enum class CachePhase : uint8_t {
    BOOTSTRAP,
    RUNTIME,
};

struct ReadCursor {
    bool valid = false;
    int64_t dtsUs = 0;
    int64_t pos = 0;
    int64_t ptsUs = 0;
    bool usePresentationTime = false;
    SeekMode seekMode {SeekMode::SEEK_PREVIOUS_SYNC};
};

struct SampleReadInfo {
    // Keep one resolved view across selection, I/O and attribute output to avoid sparse replay.
    uint32_t trackId = 0;
    uint32_t sampleIndex = 0;
    MPEG4SampleHelper::SampleIndexView view;
    bool found = false;
};

struct TrackDfxInfo {
    int frameIndex = 0;
};

enum DumpMode : unsigned long {
    DUMP_NONE = 0,
    DUMP_READAT_INPUT = 0b001,
    DUMP_AVPACKET_OUTPUT = 0b010,
    DUMP_AVBUFFER_OUTPUT = 0b100,
};

struct DumpParam {
    DumpMode mode;
    uint8_t* buf;
    int trackId;
    int64_t offset;
    int size;
    int index;
    int64_t pts;
    int64_t pos;
};

struct DynamicBaseTrackState {
    int64_t& minPts;
    int32_t& baseTrackTimeBase;
    bool& baseTrackIsVideo;
    int32_t& baseTrackIndex;
};

struct SeekTimeContext {
    bool editListApplied = false;
    int32_t baseTrackTimeScale = 0;
    int64_t relativeSeekTime = 0;
    int64_t trackSeekTime = 0;
    int64_t syncSeekTime = 0;
};

struct SeekSampleIndexContext {
    int64_t seekTime = 0;
    int64_t trackSeekTime = 0;
    SeekMode mode = SeekMode::SEEK_PREVIOUS_SYNC;
    uint32_t sampleIndex = 0;
    bool isBaseTrack = true;
};

struct VideoSyncSeekContext {
    uint32_t sampleIndex;
    bool isBaseTrack;
    bool editListApplied;
    int32_t baseTrackTimeScale;
    int64_t& syncSeekTime;
};

struct SeekToFrameByDtsContext {
    std::shared_ptr<MPEG4AtomParser::Track> track;
    uint32_t sampleIndex = 0;
    int64_t actualDts = 0;
    int32_t trackTimeScale = 0;
};

struct AnnexbBufferConvertContext {
    uint8_t* outputBuffer = nullptr;
    int32_t outputBufferSize = 0;
    int32_t outputDataSize = 0;
    bool handled = false;
    bool shouldConvert = false;
    bool isPrepared = false;
    bool isAvc = false;
};

struct WebVttReadContext {
    Seekable seekable;
    uint32_t trackId = 0;
    std::shared_ptr<MPEG4AtomParser::Track> track;
    std::shared_ptr<AVBuffer> outBuffer;
};

enum class ThreadState : uint32_t {
    NOT_STARTED,
    WAITING,
    READING,
};

enum class ReadMode : uint32_t {
    NONE = 0,
    SYNC = 1U << 0,
    ASYNC = 1U << 1,
};

enum class AVReadPacketStopState : uint32_t {
    UNSET = 0,
    TRUE,
    FALSE,
};

} // namespace detail

bool ShouldClearAudioDiscardForSkipPacket(
    const std::shared_ptr<MPEG4AtomParser::Track>& track, uint32_t sampleIndex,
    const MPEG4SampleHelper::SampleIndexView& currentSample, bool hasPlayableVideoTrack);

uint32_t GetSkipSamplesFromInfo(const std::vector<uint8_t>& skipSamplesInfo);

void RemoveZeroChannelKeys(Meta& meta);

} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS

#endif // MPEG4_DEMUXER_PLUGIN_INTERNAL_H
