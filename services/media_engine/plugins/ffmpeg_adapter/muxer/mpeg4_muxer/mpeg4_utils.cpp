/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "mpeg4_utils.h"
#include <regex>
#include "common/log.h"
#include "avcodec_audio_common.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "Mpeg4Utils" };
// find nal
constexpr uint32_t SHORT_STARTCODELEN = 3;
constexpr uint32_t LONG_STARTCODELEN = 4;
constexpr uint32_t EDGE_PROTECT_THREE = 3;
constexpr uint32_t EDGE_PROTECT_FOUR = 4;
constexpr uint32_t MAIN_MATCH_STRIDE = 4;
constexpr uint32_t NAL_START_PATTERN = 0x01000100;
constexpr uint32_t BITWISE_NOT_NAL_START_PATTERN = ~0x01000100;
constexpr uint32_t NAL_MATCH_MASK = 0x80008000U;
}
#endif // !_WIN32

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
/* time scale microsecond */
constexpr int32_t TIME_SCALE_US = 1000000;


int64_t ConvertTimeFromMpeg4(int64_t time, int32_t timeScale,
    RoundingType type)
{
    return ConvertTime(time, timeScale, TIME_SCALE_US, type);
}

int64_t ConvertTimeToMpeg4(int64_t time, int32_t timeScale,
    RoundingType type)
{
    return ConvertTime(time, TIME_SCALE_US, timeScale, type);
}

int64_t ConvertTime(int64_t time, int32_t timeScaleA, int32_t timeScaleB,
    RoundingType type)
{
    FALSE_RETURN_V_MSG_E(timeScaleA > 0, INT64_MIN, "time scale a %{public}d must be > 0", timeScaleA);
    FALSE_RETURN_V_MSG_E(timeScaleB > 0, INT64_MIN, "time scale b %{public}d must be > 0", timeScaleB);

    if (time < 0 && type == RoundingType::UP) {
        return -ConvertTime(-time, timeScaleA, timeScaleB, RoundingType::DOWN);
    } else if (time < 0 && type == RoundingType::DOWN) {
        return -ConvertTime(-time, timeScaleA, timeScaleB, RoundingType::UP);
    } else if (time < 0) {
        return -ConvertTime(-time, timeScaleA, timeScaleB, type);
    }

    int64_t timeC = 0;
    if (type == RoundingType::NEAR_INF) {
        timeC = timeScaleA / 2;  // 2
    } else if (type == RoundingType::UP) {
        timeC = timeScaleA - 1;
    }

    int64_t timeD = time / timeScaleA;
    int64_t timeE = (time % timeScaleA * timeScaleB + timeC) / timeScaleA;
    FALSE_RETURN_V_MSG_E(timeD < INT32_MAX || timeD <= (INT64_MAX - timeE) / timeScaleB, INT64_MIN,
        "time after conversion must be <= INT64_MAX, time " PUBLIC_LOG_D64 " * " PUBLIC_LOG_D32 " / " PUBLIC_LOG_D32,
        time, timeScaleB, timeScaleA);
    return timeD * timeScaleB + timeE;
}

std::vector<uint8_t> GenerateAACCodecConfig(int32_t profile, int32_t sampleRate, int32_t channels)
{
    const std::unordered_map<MediaAVCodec::AACProfile, int32_t> profiles = {
        {MediaAVCodec::AAC_PROFILE_LC, 1},
        {MediaAVCodec::AAC_PROFILE_ELD, 38},
        {MediaAVCodec::AAC_PROFILE_ERLC, 1},
        {MediaAVCodec::AAC_PROFILE_HE, 4},
        {MediaAVCodec::AAC_PROFILE_HE_V2, 28},
        {MediaAVCodec::AAC_PROFILE_LD, 22},
        {MediaAVCodec::AAC_PROFILE_MAIN, 0},
    };
    const std::unordered_map<uint32_t, uint32_t> sampleRates = {
        {96000, 0}, {88200, 1}, {64000, 2}, {48000, 3},
        {44100, 4}, {32000, 5}, {24000, 6}, {22050, 7},
        {16000, 8}, {12000, 9}, {11025, 10}, {8000,  11},
        {7350,  12},
    };
    uint32_t profileVal = 1;  // AAC_PROFILE_LC
    auto it1 = profiles.find(static_cast<MediaAVCodec::AACProfile>(profile));
    if (it1 != profiles.end()) {
        profileVal = it1->second;
    }
    uint32_t sampleRateIndex = 0x10;
    uint32_t baseIndex = 0xF;
    auto it2 = sampleRates.find(sampleRate);
    if (it2 != sampleRates.end()) {
        sampleRateIndex = it2->second;
    }
    it2 = sampleRates.find(sampleRate / 2); // 2: HE-AAC require divide base sample rate
    if (it2 != sampleRates.end()) {
        baseIndex = it2->second;
    }
    std::vector<uint8_t> codecConfig;
    if (profile == MediaAVCodec::AAC_PROFILE_HE || profile == MediaAVCodec::AAC_PROFILE_HE_V2) {
        // HE-AAC v2 only support stereo and only one channel exist
        uint32_t realCh = (profile == MediaAVCodec::AAC_PROFILE_HE_V2) ? 1 : static_cast<uint32_t>(channels);
        codecConfig = {0, 0, 0, 0, 0};
        // 5 bit AOT(0x03:left 3 bits for sample rate) + 4 bit sample rate idx(0x01: 4 - 0x03)
        codecConfig[0] = ((profileVal + 1) << 0x03) | ((baseIndex & 0x0F) >> 0x01);
        // 0x07: left 7bits for other, 4 bit channel cfg,0x03:left for other
        codecConfig[1] = ((baseIndex & 0x01) << 0x07) | ((realCh & 0x0F) << 0x03) | ((sampleRateIndex & 0x0F) >> 1) ;
        // 4 bit ext sample rate idx(0x07: left 7 bits for other) + 4 bit aot(2: LC-AAC, 0x02: left for other)
        codecConfig[2] = ((sampleRateIndex & 0x01) << 0x07) | (2 << 0x02);
    } else {
        codecConfig = {0, 0, 0x56, 0xE5, 0};
        codecConfig[0] = ((profileVal + 1) << 0x03) | ((sampleRateIndex & 0x0F) >> 0x01);
        codecConfig[1] = ((sampleRateIndex & 0x01) << 0x07) | ((static_cast<uint32_t>(channels) & 0x0F) << 0x03);
    }
    return codecConfig;
}

static uint8_t *FastFindNalStartCode(const uint8_t *p, const uint8_t *buf, int32_t &startCodeLen)
{
    if (p[1] == 0) {
        if (p[0] == 0 && p[2] == 1) { // 2 is the second byte index
            startCodeLen = (p > buf && *(p - 1) == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p - (startCodeLen - SHORT_STARTCODELEN));
        }
        if (p[2] == 0 && p[3] == 1) { // 2， 3 is byte index
            startCodeLen = (p[0] == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p + 1 - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    if (p[3] == 0) { // 3 is the third byte index
        if (p[2] == 0 && p[4] == 1) { // 2， 4 is byte index
            startCodeLen = (p[1] == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p + 2 - (startCodeLen - SHORT_STARTCODELEN));
        }
        if (p[4] == 0 && p[5] == 1) { // 4， 5 is byte index
            startCodeLen = (p[2] == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN; // 2 is the second byte index
            return const_cast<uint8_t*>(p + 3 - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    return nullptr;
}

uint8_t *FindNalStartCode(const uint8_t *buf, const uint8_t *end, int32_t &startCodeLen)
{
    if (end - buf < EDGE_PROTECT_THREE) {
        startCodeLen = SHORT_STARTCODELEN;
        return const_cast<uint8_t*>(end);
    }
    const uint8_t *p = buf;
    // 0x3: fetch the last 2 bits
    const uint8_t *alignedP = p + (EDGE_PROTECT_FOUR - (reinterpret_cast<uintptr_t>(p) & 0x3));
    for (end -= EDGE_PROTECT_THREE; p < alignedP && p < end; ++p) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1) { // 2 is the second byte index
            startCodeLen = (p > buf && *(p - 1) == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    for (end -= EDGE_PROTECT_THREE; p < end; p += MAIN_MATCH_STRIDE) {
        uint32_t x = *static_cast<const uint32_t*>(static_cast<const void*>(p));
        uint32_t y = x < NAL_START_PATTERN ? (x + (BITWISE_NOT_NAL_START_PATTERN + 1)) : (x - NAL_START_PATTERN);
        if ((y & (~x) & NAL_MATCH_MASK) == 0) {
            continue;
        }
        auto ptr = FastFindNalStartCode(p, buf, startCodeLen);
        if (ptr != nullptr) {
            return ptr;
        }
    }
    for (end += EDGE_PROTECT_THREE; p < end; ++p) {
        if (p[0] == 0 && p[1] == 0 && p[2] == 1) { // 2 is the second byte index
            startCodeLen = (p > buf && *(p - 1) == 0) ? LONG_STARTCODELEN : SHORT_STARTCODELEN;
            return const_cast<uint8_t*>(p - (startCodeLen - SHORT_STARTCODELEN));
        }
    }
    startCodeLen = SHORT_STARTCODELEN;
    return const_cast<uint8_t*>(end + SHORT_STARTCODELEN); // reset and return original end
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS