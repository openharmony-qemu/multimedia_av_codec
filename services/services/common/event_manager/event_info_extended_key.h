/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#ifndef AVCODEC_EVENT_INFO_EXTENDED_KEY_H
#define AVCODEC_EVENT_INFO_EXTENDED_KEY_H

#include <string>
#include "meta.h"
#include "instance_info.h"

namespace OHOS {
namespace MediaAVCodec {
class EventInfoExtentedKey {
public:
    static constexpr std::string_view INSTANCE_ID = "av_codec_event_info_instance_id";
    static constexpr std::string_view CODEC_TYPE = "av_codec_event_info_codec_type";
    static constexpr std::string_view IS_HARDWARE = "av_codec_event_info_is_hardware";
    static constexpr std::string_view BIT_DEPTH = "av_codec_event_info_bit_depth";
    static constexpr std::string_view ENABLE_POST_PROCESSING = "av_codec_event_info_enable_post_processing";
};

static inline int32_t GetInstanceIdFromMeta(const Media::Meta &meta)
{
    auto instanceId = INVALID_INSTANCE_ID;
    meta.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId);
    return instanceId;
}
static inline int32_t GetCodecTypeFromMeta(const Media::Meta &meta)
{
    int32_t codecType = AVCodecType::AVCODEC_TYPE_NONE;
    meta.GetData(EventInfoExtentedKey::CODEC_TYPE.data(), codecType);
    return codecType;
}
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_EVENT_INFO_EXTENDED_KEY_H