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

#ifndef AVPACKET_WRAPPER_H
#define AVPACKET_WRAPPER_H
#include <memory>

#ifdef __cplusplus
extern "C" {
#endif
#include "libavcodec/avcodec.h"
#ifdef __cplusplus
}
#endif

/*
 * AVPacketWrapper
 *  - RAII holder owning AVPacket*
 *  - Share ownership via std::shared_ptr<AVPacketWrapper>
 */
namespace OHOS {
namespace Media {
namespace Plugins {
class AVPacketWrapper {
public:
    /**
     * @brief Construct an empty AVPacketWrapper and allocate AVPacket.
     *        The allocated packet is always valid unless allocation fails.
     */
    AVPacketWrapper();

    /**
     * @brief Construct wrapper from an existing AVPacket*.
     *        Ownership is transferred to this wrapper.
     */
    explicit AVPacketWrapper(AVPacket* pkt) noexcept;

    /**
     * @brief Destructor: free AVPacket.
     */
    ~AVPacketWrapper();

    /**
     * @brief Get raw AVPacket pointer.
     */
    AVPacket* GetAVPacket() const noexcept;

    int64_t GetPts() const noexcept;
    int64_t GetDts() const noexcept;
    int GetSize() const noexcept;
    uint8_t* GetData() const noexcept;
    int GetStreamIndex() const noexcept;
    int GetFlags() const noexcept;
    int64_t GetDuration() const noexcept;
    int64_t GetPos() const noexcept;
    AVRational GetTimeBase() const noexcept;

    // Copy is disabled to avoid double freeing AVPacket
    AVPacketWrapper(const AVPacketWrapper&) = delete;
    AVPacketWrapper& operator=(const AVPacketWrapper&) = delete;

    // Move operations transfer ownership
    AVPacketWrapper(AVPacketWrapper&& other) noexcept;
    AVPacketWrapper& operator=(AVPacketWrapper&& other) noexcept;

private:
    AVPacket* pkt_{nullptr};
};

using AVPacketWrapperPtr = std::shared_ptr<AVPacketWrapper>;
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVPACKET_WRAPPER_H