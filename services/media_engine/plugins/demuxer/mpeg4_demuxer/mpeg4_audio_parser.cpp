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
#include <limits>
#include <vector>

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
constexpr int32_t MPEG_AUDIO_MONO_CHANNELS = 1;
constexpr int32_t MPEG_AUDIO_STEREO_CHANNELS = 2;
constexpr uint8_t AAC_ELEMENT_TYPE_SCE = 0;
constexpr uint8_t AAC_ELEMENT_TYPE_CPE = 1;
constexpr uint8_t AAC_ELEMENT_TYPE_FIL = 6;
constexpr uint8_t AAC_ELEMENT_TYPE_END = 7;
constexpr uint8_t AAC_EXTENSION_TYPE_SBR = 0x0D;
constexpr uint8_t AAC_EXTENSION_TYPE_SBR_CRC = 0x0E;
constexpr uint8_t AAC_FIL_ESCAPE_COUNT = 15;
constexpr uint8_t AAC_FILL_COUNT_BITS = 4;
constexpr uint8_t AAC_BITS_PER_BYTE = 8;
constexpr uint8_t AAC_FILL_BYTE_BITS = AAC_BITS_PER_BYTE;
constexpr uint8_t AAC_FILL_ESCAPE_ADJUSTMENT = 1;
constexpr uint8_t AAC_ELEMENT_HEADER_BITS = 7;
constexpr uint8_t AAC_ELEMENT_TYPE_BITS = 3;
constexpr uint8_t AAC_ELEMENT_INSTANCE_TAG_BITS = 4;
constexpr uint8_t AAC_ELEMENT_END_BITS = 3;
constexpr uint8_t AAC_EXTENSION_TYPE_BITS = 4;
constexpr uint8_t AAC_PADDING_BITS_MAX = 7;
constexpr uint8_t AAC_SBR_SAMPLE_RATE_MULTIPLIER = 2;
constexpr int32_t AAC_DEFAULT_SAMPLES_PER_FRAME = 1024;
constexpr int32_t AAC_STEREO_CHANNELS = 2;
constexpr uint8_t AAC_AUDIO_OBJECT_TYPE_BITS = 5;
constexpr uint8_t AAC_AUDIO_OBJECT_TYPE_ESCAPE = 0x1F;
constexpr uint8_t AAC_AUDIO_OBJECT_TYPE_ESCAPE_OFFSET = 0x20;
constexpr uint8_t AAC_AUDIO_OBJECT_TYPE_EXTENSION_BITS = 6;
constexpr uint8_t AAC_AUDIO_OBJECT_TYPE_SBR = 5;
constexpr uint8_t AAC_AUDIO_OBJECT_TYPE_PS = 29;
constexpr uint8_t AAC_AAC_LC_OBJECT_TYPE = 2;
constexpr uint8_t AAC_SAMPLING_FREQUENCY_INDEX_BITS = 4;
constexpr uint8_t AAC_EXPLICIT_SAMPLING_FREQUENCY_INDEX = 0x0F;
constexpr uint8_t AAC_EXPLICIT_SAMPLING_FREQUENCY_BITS = 24;
constexpr uint8_t AAC_CHANNEL_CONFIGURATION_BITS = 4;
constexpr uint8_t AAC_LC_INITIAL_GA_FLAGS_BITS = 2;
constexpr uint8_t AAC_EXTENSION_FLAG_BITS = 1;
constexpr uint8_t AAC_CORE_CODER_DELAY_BITS = 14;
constexpr uint8_t AAC_SYNC_EXTENSION_TYPE_BITS = 11;
constexpr uint16_t AAC_SYNC_EXTENSION_TYPE = 0x2B7;
constexpr uint8_t AAC_SYNC_EXTENSION_SBR_BITS = 17;
constexpr uint8_t AAC_SYNC_EXTENSION_SBR_SIGNAL_BITS = 1;

enum class AacSbrSignalState : uint8_t {
    UNSPECIFIED,
    PRESENT,
    ABSENT,
};

struct AacRawDataBlockInfo {
    bool hasSbr = false;
};

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
    info.channels = (channelMode == channelModeMono) ? MPEG_AUDIO_MONO_CHANNELS : MPEG_AUDIO_STEREO_CHANNELS;
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
    FALSE_RETURN_V(bitReader.HasBits(AAC_FILL_COUNT_BITS), false);
    uint32_t fillBytes = bitReader.ReadBits(AAC_FILL_COUNT_BITS);
    if (fillBytes == AAC_FIL_ESCAPE_COUNT) {
        FALSE_RETURN_V(bitReader.HasBits(AAC_FILL_BYTE_BITS), false);
        uint32_t escapedFillBytes = bitReader.ReadBits(AAC_FILL_BYTE_BITS);
        FALSE_RETURN_V(escapedFillBytes > 0, false);
        fillBytes += escapedFillBytes - AAC_FILL_ESCAPE_ADJUSTMENT;
    }
    while (fillBytes > 0) {
        FALSE_RETURN_V(bitReader.HasBits(AAC_FILL_BYTE_BITS), false);
        FALSE_RETURN_V(bitReader.SkipBits(AAC_FILL_BYTE_BITS), false);
        --fillBytes;
    }
    return true;
}

bool ReadAacBitsAt(const uint8_t* data, size_t totalBits, size_t bitOffset, uint8_t bitCount, uint32_t& value)
{
    if (data == nullptr || bitCount == 0 || bitCount > sizeof(value) * AAC_BITS_PER_BYTE || bitOffset > totalBits ||
        bitCount > totalBits - bitOffset) {
        return false;
    }
    value = 0;
    for (uint8_t i = 0; i < bitCount; ++i) {
        size_t currentBit = bitOffset + i;
        uint8_t currentByte = data[currentBit / AAC_BITS_PER_BYTE];
        uint8_t shift = static_cast<uint8_t>(AAC_BITS_PER_BYTE - 1 - currentBit % AAC_BITS_PER_BYTE);
        value = (value << 1) | ((currentByte >> shift) & 0x01);
    }
    return true;
}

bool ReadAacAudioObjectType(const uint8_t* data, size_t totalBits, size_t& bitOffset, uint32_t& audioObjectType)
{
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_AUDIO_OBJECT_TYPE_BITS, audioObjectType)) {
        return false;
    }
    bitOffset += AAC_AUDIO_OBJECT_TYPE_BITS;
    if (audioObjectType != AAC_AUDIO_OBJECT_TYPE_ESCAPE) {
        return true;
    }

    uint32_t escapedObjectType = 0;
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_AUDIO_OBJECT_TYPE_EXTENSION_BITS, escapedObjectType)) {
        return false;
    }
    audioObjectType = AAC_AUDIO_OBJECT_TYPE_ESCAPE_OFFSET + escapedObjectType;
    bitOffset += AAC_AUDIO_OBJECT_TYPE_EXTENSION_BITS;
    return true;
}

bool SkipAacSamplingFrequency(const uint8_t* data, size_t totalBits, size_t& bitOffset)
{
    uint32_t samplingFrequencyIndex = 0;
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_SAMPLING_FREQUENCY_INDEX_BITS, samplingFrequencyIndex)) {
        return false;
    }
    bitOffset += AAC_SAMPLING_FREQUENCY_INDEX_BITS;
    if (samplingFrequencyIndex != AAC_EXPLICIT_SAMPLING_FREQUENCY_INDEX) {
        return true;
    }

    if (bitOffset > totalBits || AAC_EXPLICIT_SAMPLING_FREQUENCY_BITS > totalBits - bitOffset) {
        return false;
    }
    bitOffset += AAC_EXPLICIT_SAMPLING_FREQUENCY_BITS;
    return true;
}

bool SkipAacLcSpecificConfig(const uint8_t* data, size_t totalBits, size_t& bitOffset)
{
    if (!SkipAacSamplingFrequency(data, totalBits, bitOffset)) {
        return false;
    }

    uint32_t channelConfiguration = 0;
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_CHANNEL_CONFIGURATION_BITS, channelConfiguration) ||
        channelConfiguration == 0) {
        return false;
    }
    bitOffset += AAC_CHANNEL_CONFIGURATION_BITS;

    uint32_t initialGaFlags = 0;
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_LC_INITIAL_GA_FLAGS_BITS, initialGaFlags)) {
        return false;
    }
    bitOffset += AAC_LC_INITIAL_GA_FLAGS_BITS;
    if ((initialGaFlags & 0x01) != 0) {
        if (bitOffset > totalBits || AAC_CORE_CODER_DELAY_BITS > totalBits - bitOffset) {
            return false;
        }
        bitOffset += AAC_CORE_CODER_DELAY_BITS;
    }

    uint32_t extensionFlag = 0;
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_EXTENSION_FLAG_BITS, extensionFlag)) {
        return false;
    }
    bitOffset += AAC_EXTENSION_FLAG_BITS;
    if (extensionFlag == 0) {
        return true;
    }

    if (bitOffset >= totalBits) {
        return false;
    }
    ++bitOffset;
    return true;
}

AacSbrSignalState FindAacSyncExtensionSbrSignal(const uint8_t* data, size_t totalBits, size_t bitOffset)
{
    if (totalBits < AAC_SYNC_EXTENSION_SBR_BITS || bitOffset > totalBits - AAC_SYNC_EXTENSION_SBR_BITS) {
        return AacSbrSignalState::UNSPECIFIED;
    }
    const size_t lastSyncExtensionOffset = totalBits - AAC_SYNC_EXTENSION_SBR_BITS;
    for (; bitOffset <= lastSyncExtensionOffset; ++bitOffset) {
        uint32_t syncExtensionType = 0;
        uint32_t extensionAudioObjectType = 0;
        uint32_t sbrPresent = 0;
        if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_SYNC_EXTENSION_TYPE_BITS, syncExtensionType) ||
            syncExtensionType != AAC_SYNC_EXTENSION_TYPE ||
            !ReadAacBitsAt(data, totalBits, bitOffset + AAC_SYNC_EXTENSION_TYPE_BITS, AAC_AUDIO_OBJECT_TYPE_BITS,
                extensionAudioObjectType) ||
            extensionAudioObjectType != AAC_AUDIO_OBJECT_TYPE_SBR ||
            !ReadAacBitsAt(data, totalBits, bitOffset + AAC_SYNC_EXTENSION_TYPE_BITS +
                AAC_AUDIO_OBJECT_TYPE_BITS, AAC_SYNC_EXTENSION_SBR_SIGNAL_BITS, sbrPresent)) {
            continue;
        }
        return sbrPresent ? AacSbrSignalState::PRESENT : AacSbrSignalState::ABSENT;
    }
    return AacSbrSignalState::UNSPECIFIED;
}

AacSbrSignalState ParseAacSbrSignalState(const std::vector<uint8_t>& codecConfig)
{
    if (codecConfig.empty()) {
        return AacSbrSignalState::UNSPECIFIED;
    }
    size_t totalBits = 0;
    if (__builtin_mul_overflow(codecConfig.size(), static_cast<size_t>(AAC_BITS_PER_BYTE), &totalBits)) {
        return AacSbrSignalState::UNSPECIFIED;
    }

    size_t bitOffset = 0;
    uint32_t audioObjectType = 0;
    if (!ReadAacAudioObjectType(codecConfig.data(), totalBits, bitOffset, audioObjectType)) {
        return AacSbrSignalState::UNSPECIFIED;
    }
    if (audioObjectType == AAC_AUDIO_OBJECT_TYPE_SBR || audioObjectType == AAC_AUDIO_OBJECT_TYPE_PS) {
        return AacSbrSignalState::PRESENT;
    }
    if (audioObjectType != AAC_AAC_LC_OBJECT_TYPE || !SkipAacLcSpecificConfig(codecConfig.data(), totalBits,
        bitOffset)) {
        return AacSbrSignalState::UNSPECIFIED;
    }
    return FindAacSyncExtensionSbrSignal(codecConfig.data(), totalBits, bitOffset);
}

bool IsAacZeroPadding(const uint8_t* data, size_t totalBits, size_t bitOffset)
{
    if (data == nullptr || bitOffset > totalBits || totalBits - bitOffset > AAC_PADDING_BITS_MAX) {
        return false;
    }
    for (size_t currentBit = bitOffset; currentBit < totalBits; ++currentBit) {
        uint32_t value = 0;
        if (!ReadAacBitsAt(data, totalBits, currentBit, 1, value) || value != 0) {
            return false;
        }
    }
    return true;
}

bool ParseAacFillElementAt(const uint8_t* data, size_t totalBits, size_t bitOffset, size_t& payloadEnd,
    uint8_t& extensionType)
{
    uint32_t elementType = 0;
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_ELEMENT_TYPE_BITS, elementType) ||
        elementType != AAC_ELEMENT_TYPE_FIL) {
        return false;
    }
    size_t currentOffset = bitOffset + AAC_ELEMENT_TYPE_BITS;
    uint32_t fillBytes = 0;
    if (!ReadAacBitsAt(data, totalBits, currentOffset, AAC_FILL_COUNT_BITS, fillBytes)) {
        return false;
    }
    currentOffset += AAC_FILL_COUNT_BITS;
    if (fillBytes == AAC_FIL_ESCAPE_COUNT) {
        uint32_t escapedFillBytes = 0;
        if (!ReadAacBitsAt(data, totalBits, currentOffset, AAC_FILL_BYTE_BITS, escapedFillBytes) ||
            escapedFillBytes == 0) {
            return false;
        }
        fillBytes += escapedFillBytes - AAC_FILL_ESCAPE_ADJUSTMENT;
        currentOffset += AAC_FILL_BYTE_BITS;
    }
    if (fillBytes == 0) {
        return false;
    }
    size_t payloadBits = 0;
    if (__builtin_mul_overflow(static_cast<size_t>(fillBytes),
        static_cast<size_t>(AAC_BITS_PER_BYTE), &payloadBits) ||
        currentOffset > totalBits || payloadBits > totalBits - currentOffset) {
        return false;
    }
    uint32_t extension = 0;
    if (!ReadAacBitsAt(data, totalBits, currentOffset, AAC_EXTENSION_TYPE_BITS, extension)) {
        return false;
    }
    extensionType = static_cast<uint8_t>(extension);
    payloadEnd = currentOffset + payloadBits;
    return true;
}

bool IsAacFillChainToEnd(const uint8_t* data, size_t totalBits, size_t bitOffset, uint8_t depth)
{
    constexpr uint8_t maxFillChainDepth = 4;
    if (depth >= maxFillChainDepth) {
        return false;
    }
    uint32_t elementType = 0;
    if (!ReadAacBitsAt(data, totalBits, bitOffset, AAC_ELEMENT_TYPE_BITS, elementType)) {
        return false;
    }
    if (elementType == AAC_ELEMENT_TYPE_END) {
        return IsAacZeroPadding(data, totalBits, bitOffset + AAC_ELEMENT_END_BITS);
    }
    if (elementType != AAC_ELEMENT_TYPE_FIL) {
        return false;
    }
    size_t payloadEnd = 0;
    uint8_t extensionType = 0;
    if (!ParseAacFillElementAt(data, totalBits, bitOffset, payloadEnd, extensionType)) {
        return false;
    }
    return IsAacFillChainToEnd(data, totalBits, payloadEnd, static_cast<uint8_t>(depth + 1));
}

bool DetectAacSbrFillElement(const uint8_t* data, uint32_t size, AacRawDataBlockInfo& info)
{
    if (data == nullptr || size == 0) {
        return false;
    }
    size_t totalBits = 0;
    if (__builtin_mul_overflow(static_cast<size_t>(size), static_cast<size_t>(AAC_BITS_PER_BYTE), &totalBits)) {
        return false;
    }
    constexpr size_t minFillElementBits = AAC_ELEMENT_TYPE_BITS + AAC_FILL_COUNT_BITS + AAC_EXTENSION_TYPE_BITS +
        AAC_ELEMENT_END_BITS;
    for (size_t bitOffset = 0; bitOffset + minFillElementBits <= totalBits; ++bitOffset) {
        size_t payloadEnd = 0;
        uint8_t extensionType = 0;
        if (!ParseAacFillElementAt(data, totalBits, bitOffset, payloadEnd, extensionType)) {
            continue;
        }
        if ((extensionType == AAC_EXTENSION_TYPE_SBR || extensionType == AAC_EXTENSION_TYPE_SBR_CRC) &&
            IsAacFillChainToEnd(data, totalBits, payloadEnd, 0)) {
            info.hasSbr = true;
            return true;
        }
    }
    return false;
}

bool ParseAacRawDataBlockChannels(const uint8_t* data, uint32_t size, int32_t& channels, AudioChannelLayout& layout,
    AacRawDataBlockInfo& info)
{
    if (data == nullptr || size == 0) {
        return false;
    }
    DemuxerBitReader bitReader(data, size);
    while (bitReader.HasBits(AAC_ELEMENT_HEADER_BITS)) {
        uint8_t elementType = bitReader.ReadBits(AAC_ELEMENT_TYPE_BITS);
        if (elementType == AAC_ELEMENT_TYPE_FIL) {
            FALSE_RETURN_V(SkipAacFillElementPayload(bitReader), false);
            continue;
        }
        FALSE_RETURN_V(bitReader.HasBits(AAC_ELEMENT_INSTANCE_TAG_BITS), false);
        bitReader.SkipBits(AAC_ELEMENT_INSTANCE_TAG_BITS);
        if (elementType == AAC_ELEMENT_TYPE_SCE) {
            return false;
        }
        if (elementType == AAC_ELEMENT_TYPE_CPE) {
            channels = AAC_STEREO_CHANNELS;
            layout = AudioChannelLayout::STEREO;
            DetectAacSbrFillElement(data, size, info);
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
    AacRawDataBlockInfo frameInfo;
    if (!ParseAacRawDataBlockChannels(audioData_, dataSize_, channels, layout, frameInfo)) {
        return Status::OK;
    }

    std::vector<uint8_t> codecConfig;
    AacSbrSignalState sbrSignalState = AacSbrSignalState::UNSPECIFIED;
    if (mediaInfo.tracks[index_].Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig)) {
        sbrSignalState = ParseAacSbrSignalState(codecConfig);
    }
    if (frameInfo.hasSbr && sbrSignalState != AacSbrSignalState::ABSENT) {
        int32_t sampleRate = 0;
        int32_t samplesPerFrame = 0;
        if (mediaInfo.tracks[index_].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate) && sampleRate > 0 &&
            mediaInfo.tracks[index_].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame) &&
            samplesPerFrame == AAC_DEFAULT_SAMPLES_PER_FRAME &&
            sampleRate <= std::numeric_limits<int32_t>::max() / AAC_SBR_SAMPLE_RATE_MULTIPLIER) {
            sampleRate *= AAC_SBR_SAMPLE_RATE_MULTIPLIER;
            samplesPerFrame *= AAC_SBR_SAMPLE_RATE_MULTIPLIER;
            mediaInfo.tracks[index_].Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
            mediaInfo.tracks[index_].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame);
            MEDIA_LOG_I("AAC raw frame SBR extension detected, output sample rate=" PUBLIC_LOG_D32
                ", samples per frame=" PUBLIC_LOG_D32, sampleRate, samplesPerFrame);
        }
    }

    int32_t parsedChannels = 0;
    // Align with FFmpeg MOV: AAC channel metadata comes from sample entry/ASC,
    // and a raw frame probe may only fill missing metadata, never override it.
    if (mediaInfo.tracks[index_].Get<Tag::AUDIO_CHANNEL_COUNT>(parsedChannels) && parsedChannels > 0) {
        return Status::OK;
    }
    mediaInfo.tracks[index_].Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_OUTPUT_CHANNELS>(channels);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_CHANNEL_LAYOUT>(layout);
    mediaInfo.tracks[index_].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout);
    return Status::OK;
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
