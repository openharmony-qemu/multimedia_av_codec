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

#ifndef AVCODEC_EVENT_INFO_EXTENTED_KEY_H
#define AVCODEC_EVENT_INFO_EXTENTED_KEY_H

#include <string>

namespace OHOS {
namespace MediaAVCodec {
class EventInfoExtentedKey {
public:
    static constexpr std::string_view INSTANCE_ID = "av_codec_event_info_instance_id";
    static constexpr std::string_view CODEC_TYPE = "av_codec_event_info_codec_type";
    static constexpr std::string_view IS_HARDWARE = "IS_VENDOR";
    static constexpr std::string_view BIT_DEPTH = "av_codec_event_info_bit_depth";
    static constexpr std::string_view ENABLE_POST_PROCESSING = "av_codec_event_info_enable_post_processing";
    static constexpr std::string_view PIXEL_FORMAT_STRING = "pixel_format_string";
    static constexpr std::string_view IS_ENCODER = "is_encoder";
    static constexpr std::string_view CODEC_ERROR_CODE = "codec_error_code";
    static constexpr std::string_view VIDEO_CODEC_TYPE = "av_codec_event_info_video_codec_type";
    static constexpr std::string_view APP_ELAPSED_TIME_IN_BG = "app_elapsed_time_in_bg";
    static constexpr std::string_view TOTAL_DECODING_DURATION = "total_decoding_duration";
    static constexpr std::string_view TOTAL_DECODING_CNT = "total_decoding_cnt";
    static constexpr std::string_view SPEED_DECODING_INFO_TOTAL = "speed_decoding_info_total";
    static constexpr std::string_view SPEED_DECODING_INFO_0_75X = "speed_decoding_info_0_75x";
    static constexpr std::string_view SPEED_DECODING_INFO_1_00X = "speed_decoding_info_1_00x";
    static constexpr std::string_view SPEED_DECODING_INFO_1_25X = "speed_decoding_info_1_25x";
    static constexpr std::string_view SPEED_DECODING_INFO_1_50X = "speed_decoding_info_1_50x";
    static constexpr std::string_view SPEED_DECODING_INFO_2_00X = "speed_decoding_info_2_00x";
    static constexpr std::string_view SPEED_DECODING_INFO_3_00X = "speed_decoding_info_3_00x";
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_EVENT_INFO_EXTENTED_KEY_H