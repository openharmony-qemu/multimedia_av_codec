/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef MPEG4_UTILS_H
#define MPEG4_UTILS_H

#include "common/log.h"
#include "plugin/plugin_info.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
constexpr int32_t MS_TO_NS = 1000000;
constexpr uint64_t S_TO_US = 1000000;
constexpr int64_t US_TO_MS = 1000;
constexpr int64_t MS_TO_US = 1000;

const int32_t FOURCC_TYPE_LENGTH = 5;
const int32_t MAJOR_BRAND_LENGTH = 4;
const int32_t THIRD_CHAR = 2;
const int32_t FOURTH_CHAR = 3;

enum TrackType : int32_t {
    AUDIO_TYPE = 0,
    VIDEO_TYPE = 1,
    SUBTITLE_TYPE = 2,
    ATTACHMENT_TYPE = 3,
    TIMEDMETA_TYPE = 5,
    AUXILIARY_TYPE = 6,
    INVALID_TYPE = -1,
};

inline std::string FourccToString(uint32_t fourcc)
{
    return std::string {
        static_cast<char>((fourcc >> 0x18) & 0xFF),
        static_cast<char>((fourcc >> 0x10) & 0xFF),
        static_cast<char>((fourcc >> 0x08) & 0xFF),
        static_cast<char>(fourcc & 0xFF)
    };
}

inline uint16_t GetU16Value(const uint8_t *data)
{
    if (!data) {
        return 0;
    }
    return static_cast<uint16_t>((static_cast<uint16_t>(data[0]) << 0x08) | data[1]);
}

inline uint32_t GetU32Value(const uint8_t *data)
{
    if (!data) {
        return 0;
    }
    return (static_cast<uint32_t>(data[0]) << 0x18) |
           (static_cast<uint32_t>(data[1]) << 0x10) |
           (static_cast<uint32_t>(data[THIRD_CHAR]) << 0x08) |
           static_cast<uint32_t>(data[FOURTH_CHAR]);
}

inline std::string GetCharValue(const uint8_t *data)
{
    if (!data) {
        return "";
    }
    return std::string(reinterpret_cast<const char*>(data), MAJOR_BRAND_LENGTH);
}

inline uint64_t GetU64Value(const uint8_t *data)
{
    if (!data) {
        return 0;
    }
    return (static_cast<uint64_t>(GetU32Value(data)) << 0x20) | GetU32Value(data + 0x04);
}

inline double GetDoubleValue(const uint8_t* data)
{
    uint64_t intVal = GetU64Value(data);
    return *reinterpret_cast<double*>(&intVal);
}

inline Status ConvertTimeScale(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result)
{
    int64_t temp;
    if (__builtin_mul_overflow(multiplicand, multiplier, &temp) || timeScale == 0) {
        return Status::ERROR_INVALID_DATA;
    }
    *result = temp / timeScale;

    return Status::OK;
}

template<size_t N>
constexpr int32_t FourccType(const char (&str)[N])
{
    if (N != FOURCC_TYPE_LENGTH) {
        return -1;
    }
    return static_cast<int32_t>((static_cast<uint32_t>(static_cast<uint8_t>(str[0])) << 0x18) |
                                (static_cast<uint32_t>(static_cast<uint8_t>(str[1])) << 0x10) |
                                (static_cast<uint32_t>(static_cast<uint8_t>(str[THIRD_CHAR])) << 0x08) |
                                static_cast<uint32_t>(static_cast<uint8_t>(str[FOURTH_CHAR])));
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_UTILS_H