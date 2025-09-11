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

#ifndef AVCODEC_INSTANCE_INFO_H
#define AVCODEC_INSTANCE_INFO_H

#include <sys/types.h>
#include <string>
#include "avcodec_log.h"
#include "avcodec_info.h"
#include "meta.h"
#include "meta/meta_key.h"

namespace OHOS {
namespace MediaAVCodec {
using InstanceId = int32_t;
constexpr pid_t INVALID_PID = -1;
constexpr InstanceId INVALID_INSTANCE_ID = -1;

class EventInfoExtentedKey {
public:
    static constexpr std::string_view INSTANCE_ID = "av_codec_event_info_instance_id";
    static constexpr std::string_view CODEC_TYPE = "av_codec_event_info_codec_type";
    static constexpr std::string_view IS_HARDWARE = "IS_VENDOR";
    static constexpr std::string_view BIT_DEPTH = "av_codec_event_info_bit_depth";
    static constexpr std::string_view ENABLE_POST_PROCESSING = "av_codec_event_info_enable_post_processing";
    static constexpr std::string_view PIXEL_FORMAT_STRING = "pixel_format_string";

    static int32_t GetInstanceIdFromMeta(const Media::Meta &meta)
    {
        auto instanceId = INVALID_INSTANCE_ID;
        meta.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId);
        return instanceId;
    }
};

struct CallerInfo {
    pid_t pid = -1;
    uid_t uid = 0;
    std::string processName = "";
};

struct InstanceInfo {
    InstanceId instanceId = INVALID_INSTANCE_ID;
    CallerInfo caller;
    CallerInfo forwardCaller;
    AVCodecType codecType;
    uint32_t memoryUsage = 0;
    std::string codecName = "";

    void Print()
    {
        constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceInfo"};
        AVCODEC_LOGI("InstanceId: %{public}d, Caller: [%{public}d, %{public}s], ForwardCaller: [%{public}d, %{public}s]"
            ", CodecType: %{public}d, MemoryUsage: %{public}u", instanceId, caller.pid, caller.processName.c_str(),
            forwardCaller.pid, forwardCaller.processName.c_str(), codecType, memoryUsage);
    }
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_INSTANCE_INFO_H