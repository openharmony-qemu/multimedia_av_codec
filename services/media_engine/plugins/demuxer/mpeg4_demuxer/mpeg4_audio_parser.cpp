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
#define HST_LOG_TAG "MPEG4AudioParser"
#include "common/log.h"
#include "demuxer_data_reader.h"
#include "mpeg4_audio_parser.h"
#include "mpeg4_utils.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4AudioParser" };
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
constexpr uint32_t DTS_SYNC_WORD = 0x7FFE8001;
constexpr uint8_t MAX_DTS_AUDIO_MODE = 10;
constexpr uint32_t MPEG_AUDIO_SYNC = 0x7FF;
constexpr uint8_t MPEG_AUDIO_VERSION_RESERVED = 0x01;
constexpr uint8_t MPEG_AUDIO_LAYER_RESERVED = 0x00;
constexpr uint8_t MPEG_AUDIO_LAYER_I = 0x03;
constexpr uint8_t MPEG_AUDIO_LAYER_II = 0x02;
constexpr uint8_t MPEG_AUDIO_BITRATE_FREE = 0x00;
constexpr uint8_t MPEG_AUDIO_BITRATE_BAD = 0x0F;
constexpr uint8_t MPEG_AUDIO_SAMPLE_RATE_RESERVED = 0x03;
constexpr uint8_t MPEG_AUDIO_VERSION_1 = 0x03;
constexpr int32_t MPEG_AUDIO_LAYER_I_SAMPLES = 384;
constexpr int32_t MPEG_AUDIO_LAYER_II_SAMPLES = 1152;
constexpr int32_t MPEG_AUDIO_LAYER_III_MPEG1_SAMPLES = 1152;
constexpr int32_t MPEG_AUDIO_LAYER_III_LSF_SAMPLES = 576;
constexpr uint8_t AAC_ELEMENT_TYPE_SCE = 0;
constexpr uint8_t AAC_ELEMENT_TYPE_CPE = 1;
constexpr uint8_t AAC_ELEMENT_TYPE_FIL = 6;
constexpr uint8_t AAC_FIL_ESCAPE_COUNT = 15;

struct MpegAudioFrameInfo {
    bool valid = false;
    int32_t samplesPerFrame = 0;
    AudioSampleFormat sampleFormat = AudioSampleFormat::INVALID_WIDTH;
    int32_t channels = 0;
};

const std::vector<std::pair<int32_t, AudioChannelLayout>> g_channelLayoutDefaultMap = {
    {2, AudioChannelLayout::STEREO},             // 2: STEREO
    {4, AudioChannelLayout::CH_4POINT0},         // 4: CH_4POINT0
    {6, AudioChannelLayout::CH_5POINT1},         // 6: CH_5POINT1
    {8, AudioChannelLayout::CH_5POINT1POINT2},   // 8: CH_5POINT1POINT2
    {9, AudioChannelLayout::HOA_ORDER2_ACN_N3D}, // 9: HOA_ORDER2_ACN_N3D
    {10, AudioChannelLayout::CH_7POINT1POINT2},  // 10: CH_7POINT1POINT2 or CH_5POINT1POINT4 ?
    {12, AudioChannelLayout::CH_7POINT1POINT4},  // 12: CH_7POINT1POINT4
    {14, AudioChannelLayout::CH_9POINT1POINT4},  // 14: CH_9POINT1POINT4
    {16, AudioChannelLayout::CH_9POINT1POINT6},  // 16: CH_9POINT1POINT6
    {24, AudioChannelLayout::CH_22POINT2},       // 24: CH_22POINT2
};

const std::vector<AudioChannelLayout> g_supportChannelLayout = {
    AudioChannelLayout::UNKNOWN,
    AudioChannelLayout::MONO,
    AudioChannelLayout::STEREO,
    AudioChannelLayout::CH_2POINT1,
    AudioChannelLayout::CH_2_1,
    AudioChannelLayout::SURROUND,
    AudioChannelLayout::CH_3POINT1,
    AudioChannelLayout::CH_4POINT0,
    AudioChannelLayout::CH_4POINT1,
    AudioChannelLayout::CH_2_2,
    AudioChannelLayout::QUAD,
    AudioChannelLayout::CH_5POINT0,
    AudioChannelLayout::CH_5POINT1,
    AudioChannelLayout::CH_5POINT0_BACK,
    AudioChannelLayout::CH_5POINT1_BACK,
    AudioChannelLayout::CH_6POINT0,
    AudioChannelLayout::CH_6POINT0_FRONT,
    AudioChannelLayout::HEXAGONAL,
    AudioChannelLayout::CH_6POINT1,
    AudioChannelLayout::CH_6POINT1_BACK,
    AudioChannelLayout::CH_6POINT1_FRONT,
    AudioChannelLayout::CH_7POINT0,
    AudioChannelLayout::CH_7POINT0_FRONT,
    AudioChannelLayout::CH_7POINT1,
    AudioChannelLayout::CH_7POINT1_WIDE,
    AudioChannelLayout::CH_7POINT1_WIDE_BACK,
    AudioChannelLayout::CH_3POINT1POINT2,
    AudioChannelLayout::CH_5POINT1POINT2,
    AudioChannelLayout::CH_5POINT1POINT4,
    AudioChannelLayout::CH_7POINT1POINT2,
    AudioChannelLayout::CH_7POINT1POINT4,
    AudioChannelLayout::CH_9POINT1POINT4,
    AudioChannelLayout::CH_9POINT1POINT6,
    AudioChannelLayout::CH_10POINT2,
    AudioChannelLayout::CH_22POINT2,
    AudioChannelLayout::OCTAGONAL,
    AudioChannelLayout::HEXADECAGONAL,
    AudioChannelLayout::STEREO_DOWNMIX,
    AudioChannelLayout::CH_2POINT0POINT2,
    AudioChannelLayout::CH_2POINT1POINT2,
    AudioChannelLayout::CH_3POINT0POINT2
};

const std::vector<AudioChannelLayout> g_dtsAudioMode2DtsChannelLayout = {
    AudioChannelLayout::MONO,
    AudioChannelLayout::STEREO,
    AudioChannelLayout::STEREO,
    AudioChannelLayout::STEREO,
    AudioChannelLayout::STEREO,
    AudioChannelLayout::SURROUND,
    AudioChannelLayout::CH_2_1,
    AudioChannelLayout::CH_4POINT0,
    AudioChannelLayout::CH_2_2,
    AudioChannelLayout::CH_5POINT0,
};

MpegAudioFrameInfo ParseMpegAudioFrameHeader(const uint8_t* data, uint32_t size)
{
    MpegAudioFrameInfo info;
    constexpr uint32_t headerSize = 4;
    constexpr uint8_t channelModeMono = 3;
    if (data == nullptr || size < headerSize) {
        return info;
    }
    uint32_t header = GetU32Value(data);
    uint32_t sync = (header >> 21) & 0x7FF;
    uint8_t version = static_cast<uint8_t>((header >> 19) & 0x03);
    uint8_t layer = static_cast<uint8_t>((header >> 17) & 0x03);
    uint8_t bitrateIndex = static_cast<uint8_t>((header >> 12) & 0x0F);
    uint8_t sampleRateIndex = static_cast<uint8_t>((header >> 10) & 0x03);
    uint8_t channelMode = static_cast<uint8_t>((header >> 6) & 0x03);
    if (sync != MPEG_AUDIO_SYNC || version == MPEG_AUDIO_VERSION_RESERVED ||
        layer == MPEG_AUDIO_LAYER_RESERVED || bitrateIndex == MPEG_AUDIO_BITRATE_FREE ||
        bitrateIndex == MPEG_AUDIO_BITRATE_BAD || sampleRateIndex == MPEG_AUDIO_SAMPLE_RATE_RESERVED) {
        return info;
    }

    info.valid = true;
    info.channels = (channelMode == channelModeMono) ? 1 : 2;
    if (layer == MPEG_AUDIO_LAYER_I) {
        info.samplesPerFrame = MPEG_AUDIO_LAYER_I_SAMPLES;
        info.sampleFormat = AudioSampleFormat::SAMPLE_S16P;
    } else if (layer == MPEG_AUDIO_LAYER_II) {
        info.samplesPerFrame = MPEG_AUDIO_LAYER_II_SAMPLES;
        info.sampleFormat = AudioSampleFormat::SAMPLE_S16P;
    } else {
        info.samplesPerFrame = (version == MPEG_AUDIO_VERSION_1) ?
            MPEG_AUDIO_LAYER_III_MPEG1_SAMPLES : MPEG_AUDIO_LAYER_III_LSF_SAMPLES;
        info.sampleFormat = AudioSampleFormat::SAMPLE_F32P;
    }
    return info;
}

bool SkipAacFillElementPayload(DemuxerBitReader& bitReader)
{
    FALSE_RETURN_V(bitReader.HasBits(4), false);
    uint32_t fillBytes = bitReader.ReadBits(4);
    if (fillBytes == AAC_FIL_ESCAPE_COUNT) {
        FALSE_RETURN_V(bitReader.HasBits(8), false);
        fillBytes += bitReader.ReadBits(8) - 1;
    }
    while (fillBytes > 0) {
        FALSE_RETURN_V(bitReader.HasBits(8), false);
        FALSE_RETURN_V(bitReader.SkipBits(8), false);
        --fillBytes;
    }
    return true;
}

bool ParseAacRawDataBlockChannels(const uint8_t* data, uint32_t size, int32_t& channels, AudioChannelLayout& layout)
{
    if (data == nullptr || size == 0) {
        return false;
    }
    DemuxerBitReader bitReader(data, size);
    while (bitReader.HasBits(7)) {
        uint8_t elementType = bitReader.ReadBits(3);
        if (elementType == AAC_ELEMENT_TYPE_FIL) {
            FALSE_RETURN_V(SkipAacFillElementPayload(bitReader), false);
            continue;
        }
        FALSE_RETURN_V(bitReader.HasBits(4), false);
        bitReader.SkipBits(4);
        if (elementType == AAC_ELEMENT_TYPE_SCE) {
            return false;
        }
        if (elementType == AAC_ELEMENT_TYPE_CPE) {
            channels = 2;
            layout = AudioChannelLayout::STEREO;
            return true;
        }
        return false;
    }
    return false;
}

MPEG4AudioParser::MPEG4AudioParser()
{
    MEDIA_LOG_D("In");
}

MPEG4AudioParser::~MPEG4AudioParser()
{
    MEDIA_LOG_D("In");
}

AudioChannelLayout MPEG4AudioParser::FindValidChannelLayout(uint64_t layoutMask)
{
    auto it = std::find(g_supportChannelLayout.begin(), g_supportChannelLayout.end(),
        static_cast<AudioChannelLayout>(layoutMask));
    if (it != g_supportChannelLayout.end()) {
        return *it;
    }
    return AudioChannelLayout::UNKNOWN;
}

AudioChannelLayout MPEG4AudioParser::GetDefaultChannelLayout(int32_t channels)
{
    AudioChannelLayout layout = AudioChannelLayout::MONO;
    auto ite = std::find_if(g_channelLayoutDefaultMap.begin(), g_channelLayoutDefaultMap.end(),
                            [&channels](const auto &item) -> bool { return item.first == channels; });
    if (ite != g_channelLayoutDefaultMap.end()) {
        layout = ite->second;
    }
    return layout;
}

Status MPEG4AudioParser::ParseAudioFrame(uint8_t* data, uint32_t size, std::string mime, uint32_t trackIndex,
    MediaInfo& mediaInfo)
{
    MEDIA_LOG_D("In");
    audioData_ = data;
    dataSize_ = size;
    index_ = trackIndex;
    auto ret = Status::OK;
    if (mime == MimeType::AUDIO_MPEG) {
        ret = ParseMpegAudio(mediaInfo);
    } else if (mime == MimeType::AUDIO_AAC) {
        ret = ParseAacAudio(mediaInfo);
    } else if (mime == "audio/dts") {
        ret = ParseDtsAudio(mediaInfo);
    }

    return ret;
}

Status MPEG4AudioParser::ParseMpegAudio(MediaInfo& mediaInfo)
{
    MEDIA_LOG_D("In");
    MpegAudioFrameInfo frameInfo = ParseMpegAudioFrameHeader(audioData_, dataSize_);
    if (!frameInfo.valid) {
        MEDIA_LOG_W("Invalid MPEG audio frame header");
        mediaInfo.tracks[index_].Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32P);
        return Status::OK;
    }
    AudioChannelLayout layout = GetDefaultChannelLayout(frameInfo.channels);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_CHANNEL_COUNT>(frameInfo.channels);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_OUTPUT_CHANNELS>(frameInfo.channels);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
    if (frameInfo.samplesPerFrame > 0) {
        mediaInfo.tracks[index_].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(frameInfo.samplesPerFrame);
    }
    mediaInfo.tracks[index_].Set<Tag::AUDIO_SAMPLE_FORMAT>(frameInfo.sampleFormat);
    return Status::OK;
}

Status MPEG4AudioParser::ParseAacAudio(MediaInfo& mediaInfo)
{
    MEDIA_LOG_D("In");
    int32_t channels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    if (!ParseAacRawDataBlockChannels(audioData_, dataSize_, channels, layout)) {
        return Status::OK;
    }
    int32_t parsedChannels = 0;
    if (mediaInfo.tracks[index_].Get<Tag::AUDIO_CHANNEL_COUNT>(parsedChannels) && parsedChannels > 0 &&
        channels <= parsedChannels) {
        return Status::OK;
    }
    mediaInfo.tracks[index_].Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_OUTPUT_CHANNELS>(channels);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
    return Status::OK;
}

Status MPEG4AudioParser::ParseDtsAudio(MediaInfo& mediaInfo)
{
    MEDIA_LOG_D("In");
    constexpr uint32_t syncWordSize = 4;
    constexpr uint32_t modeFirstByteOffset = 3;
    constexpr uint32_t modeSecondByteOffset = 4;
    constexpr uint32_t minDtsHeaderSize = syncWordSize + modeSecondByteOffset + 1;
    uint32_t currentOffset = 0;
    if (dataSize_ < minDtsHeaderSize || GetU32Value(&audioData_[currentOffset]) != DTS_SYNC_WORD) {
        return Status::ERROR_INVALID_DATA;
    }
    currentOffset += syncWordSize;
    uint8_t mode = ((audioData_[currentOffset + modeFirstByteOffset] & 0xF) << 4) +
        (audioData_[currentOffset + modeSecondByteOffset] >> 6);
    if (mode >= MAX_DTS_AUDIO_MODE) {
        return Status::ERROR_INVALID_DATA;
    }
    AudioChannelLayout layout = g_dtsAudioMode2DtsChannelLayout[mode];
    mediaInfo.tracks[index_].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
    return Status::OK;
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
