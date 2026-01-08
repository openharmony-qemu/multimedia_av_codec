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

#ifndef AVPACKET_MEMORY_H
#define AVPACKET_MEMORY_H
#include <memory>
#include "buffer/avbuffer.h"
#include "avpacket_wrapper.h"

namespace OHOS {
namespace Media {
namespace Plugins {
class AVPacketMemory : public AVMemory {
public:
    explicit AVPacketMemory(std::shared_ptr<AVPacketWrapper> pktWrapper);
    ~AVPacketMemory() override;

    // AVMemory 接口实现
    MemoryType GetMemoryType() override;
    uint8_t* GetAddr() override;
    int32_t GetCapacity() override;
    int32_t GetSize() override;
    int32_t GetOffset() override;

    int32_t Write(const uint8_t *in, int32_t writeSize, int32_t position = INVALID_POSITION) override;
    void Reset() = delete;
private:
    std::shared_ptr<AVPacketWrapper> pktWrapper_;
};
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // AVPACKET_MEMORY_H
