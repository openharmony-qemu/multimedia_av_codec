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
#include "event_info_extented_key.h"

namespace OHOS {
namespace MediaAVCodec {
using InstanceId = int32_t;
constexpr pid_t INVALID_PID = -1;
constexpr InstanceId INVALID_INSTANCE_ID = -1;

enum class VideoCodecType : int16_t {
    UNKNOWN,
    DECODER_HARDWARE,
    DECODER_SOFTWARE,
    ENCODER_HARDWARE,
    ENCODER_SOFTWARE,
    END,
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
    VideoCodecType videoCodecType = VideoCodecType::UNKNOWN;

    void Print()
    {
        constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "InstanceInfo"};
        AVCODEC_LOGI("InstanceId: %{public}d, Caller: [%{public}d, %{public}s], ForwardCaller: [%{public}d, %{public}s]"
            ", CodecType: %{public}d, MemoryUsage: %{public}u", instanceId, caller.pid, caller.processName.c_str(),
            forwardCaller.pid, forwardCaller.processName.c_str(), codecType, memoryUsage);
    }

    pid_t GetActualCallerPid() const
    {
        if (forwardCaller.pid != INVALID_INSTANCE_ID) {
            return forwardCaller.pid;
        } else {
            return caller.pid;
        }
    }

    std::string GetActualCallerProcessName() const
    {
        if (forwardCaller.pid != INVALID_INSTANCE_ID) {
            return forwardCaller.processName;
        } else {
            return caller.processName;
        }
    }
};

[[maybe_unused]] static int32_t GetInstanceIdFromMeta(const Media::Meta &meta)
{
    auto instanceId = INVALID_INSTANCE_ID;
    meta.GetData(EventInfoExtentedKey::INSTANCE_ID.data(), instanceId);
    return instanceId;
}
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVCODEC_INSTANCE_INFO_H