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


#ifndef MEDIA_AVCODEC_SUSPEND_H
#define MEDIA_AVCODEC_SUSPEND_H

#include <cstdint>
#include <vector>

namespace OHOS {
namespace MediaAVCodec {
class AVCodecSuspend {
public:
    /**
        * @brief Get freeze status message from suspend manager.
        *
        * @param pidList The list of pid to be frozen.
        * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
        * @since 5.1
        */
    static int32_t SuspendFreeze(const std::vector<pid_t> &pidList);

    /**
        * @brief Get active status message from suspend manager.
        *
        * @param pidList The list of pid to be active.
        * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
        * @since 5.1
        */
    static int32_t SuspendActive(const std::vector<pid_t> &pidList);

    /**
        * @brief Reset all frozen pids into active state.
        *
        * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
        * @since 5.1
        */
    static int32_t SuspendActiveAll();

private:
    AVCodecSuspend() = default;
    ~AVCodecSuspend() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_SUSPEND_H
