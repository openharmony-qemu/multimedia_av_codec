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

#include "avpacket_memory.h"

namespace OHOS {
namespace Media {
namespace Plugins {
AVPacketMemory::AVPacketMemory(std::shared_ptr<AVPacketWrapper> pktWrapper)
    : pktWrapper_(std::move(pktWrapper))
{
    if (pktWrapper_ && pktWrapper_->GetAVPacket()) {
        base_ = pktWrapper_->GetData();
        capacity_ = pktWrapper_->GetSize();
        size_ = pktWrapper_->GetSize();
        offset_ = 0;
    } else {
        base_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        offset_ = 0;
    }
}

AVPacketMemory::~AVPacketMemory() = default;

MemoryType AVPacketMemory::GetMemoryType()
{
    return MemoryType::CUSTOM_MEMORY;
}

uint8_t* AVPacketMemory::GetAddr()
{
    if (!pktWrapper_ || !pktWrapper_->GetAVPacket()) {
        return nullptr;
    }
    return pktWrapper_->GetData();
}

int32_t AVPacketMemory::GetCapacity()
{
    return capacity_;
}

int32_t AVPacketMemory::GetSize()
{
    return size_;
}

int32_t AVPacketMemory::GetOffset()
{
    return offset_;
}

int32_t AVPacketMemory::Write(const uint8_t *in, int32_t writeSize, int32_t position)
{
    return 0;
}
} // namespace Plugins
} // namespace Media
} // namespace OHOS