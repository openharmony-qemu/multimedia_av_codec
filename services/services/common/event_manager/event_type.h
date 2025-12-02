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

#ifndef AVCODEC_EVENT_TYPE_H
#define AVCODEC_EVENT_TYPE_H
#include <cstdint>

namespace OHOS {
namespace MediaAVCodec {
/*   EventType description
 *   +-------+-------------+-------------+-------------+-------------+
 *   |  Bit  |    31-24    |    23-16    |    15-08    |    07-00    |
 *   +-------+-------------+-------------+-------------+-------------+
 *   | Field |  EventType  |  MainEvent  |   SubEvent  | EventDetail |
 *   +-------+-------------+-------------+-------------+-------------+
 */
enum class EventType : uint32_t {
    UNKNOWN                             = 0,
    INSTANCE_INIT                       = (1 << 24),
    INSTANCE_RELEASE                    = (2 << 24),
    INSTANCE_MEMORY_UPDATE              = (3 << 24),
    INSTANCE_MEMORY_RESET               = (4 << 24),
    INSTANCE_ENCODE_BEGIN               = (5 << 24),
    INSTANCE_ENCODE_END                 = (6 << 24),
    STATISTICS_EVENT                    = (7 << 24),
    STATISTICS_EVENT_SUBMIT             = (8 << 24),
    STATISTICS_EVENT_REGISTER_SUBMIT    = (9 << 24),
    END,
    MASK                                = 0xFF000000,
};

inline constexpr EventType operator&(const EventType a, const EventType b)
{
    return static_cast<EventType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
inline constexpr EventType operator|(const EventType a, const EventType b)
{
    return static_cast<EventType>(static_cast<uint32_t>(a) | static_cast<uint32_t>(b));
}
inline constexpr uint32_t operator&(const EventType a, const uint32_t b)
{
    return static_cast<uint32_t>(a) & static_cast<uint32_t>(b);
}
inline constexpr uint32_t operator|(const EventType a, const uint32_t b)
{
    return static_cast<uint32_t>(a) | static_cast<uint32_t>(b);
}

enum class StatisticsEventType : uint32_t {
    BASIC_INFO                                          = EventType::STATISTICS_EVENT | (0 << 16) | (0 << 8) | 0,
    BASIC_QUERY_CAP_INFO                                = EventType::STATISTICS_EVENT | (0 << 16) | (0 << 8) | 1,
    BASIC_CREATE_CODEC_INFO                             = EventType::STATISTICS_EVENT | (0 << 16) | (0 << 8) | 2,
    BASIC_CREATE_CODEC_SPEC_INFO                        = EventType::STATISTICS_EVENT | (0 << 16) | (0 << 8) | 3,

    APP_SPECIFICATIONS_INFO                             = EventType::STATISTICS_EVENT | (1 << 16) | (0 << 8) | 0,
    CAP_UNSUPPORTED_INFO                                = EventType::STATISTICS_EVENT | (1 << 16) | (1 << 8) | 0,
    CAP_UNSUPPORTED_QUERY_CAP_INFO                      = EventType::STATISTICS_EVENT | (1 << 16) | (1 << 8) | 1,
    CAP_UNSUPPORTED_CREATE_CODEC_INFO                   = EventType::STATISTICS_EVENT | (1 << 16) | (1 << 8) | 2,

    APP_BEHAVIORS_INFO                                  = EventType::STATISTICS_EVENT | (2 << 16) | (0 << 8) | 0,
    APP_BEHAVIORS_RELEASE_HDEC_INFO                     = EventType::STATISTICS_EVENT | (2 << 16) | (1 << 8) | 1,
    DEC_ABNORMAL_OCCUPATION_INFO                        = EventType::STATISTICS_EVENT | (2 << 16) | (2 << 8) | 0,
    DEC_ABNORMAL_OCCUPATION_HDEC_LIMIT_EXCEEDED_INFO    = EventType::STATISTICS_EVENT | (2 << 16) | (2 << 8) | 1,
    DEC_ABNORMAL_OCCUPATION_LONG_TIME_IN_BG_INFO        = EventType::STATISTICS_EVENT | (2 << 16) | (2 << 8) | 2,
    SPEED_DECODING_INFO                                 = EventType::STATISTICS_EVENT | (2 << 16) | (3 << 8) | 0,

    CODEC_ABNORMAL_INFO                                 = EventType::STATISTICS_EVENT | (3 << 16) | (0 << 8) | 0,
    CODEC_ERROR_INFO                                    = EventType::STATISTICS_EVENT | (3 << 16) | (1 << 8) | 0,

    MAIN_EVENT_TYPE_MASK                                = EventType::STATISTICS_EVENT | 0x00FF0000,
    SUB_EVENT_TYPE_MASK                                 = EventType::STATISTICS_EVENT | 0x00FFFF00,
};

inline constexpr StatisticsEventType operator&(StatisticsEventType a, StatisticsEventType b)
{
    return static_cast<StatisticsEventType>(static_cast<uint32_t>(a) & static_cast<uint32_t>(b));
}
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_EVENT_TYPE_H