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


#ifndef MEDIA_AVCODEC_VIDEOCALL_H
#define MEDIA_AVCODEC_VIDEOCALL_H

#include <cstdint>
#include <string>

namespace OHOS {
namespace MediaAVCodec {
class AVCodecVideoCall {
public:
    /**
     * @brief RSS notify video call PIP status for low power mode adjustment.
     * @param bundleName The name of the HAP package.
     * @param isFloatingWindow True if the call is in floating window mode,
     *        false otherwise.
     * @return Returns {@link AVCS_ERR_OK} if success; returns an error code otherwise.
     * @since 7.0
     * @version 1.0
     */
    static int32_t VideoCallNotifyPiPState(const std::string &bundleName, const bool isFloatingWindow);

private:
    AVCodecVideoCall() = default;
    ~AVCodecVideoCall() = default;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_VIDEOCALL_H
