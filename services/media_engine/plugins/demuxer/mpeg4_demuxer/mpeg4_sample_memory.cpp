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

#include "mpeg4_sample_memory.h"

#include <utility>

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
MPEG4SampleMemory::MPEG4SampleMemory(std::shared_ptr<Sample> sample, int32_t offset)
    : sample_(std::move(sample)), startOffset_(offset)
{
    if (sample_ != nullptr && sample_->data != nullptr && sample_->size >= 0 &&
        startOffset_ >= 0 && startOffset_ <= sample_->size) {
        base_ = sample_->data.get() + startOffset_;
        capacity_ = sample_->size - startOffset_;
        size_ = sample_->size - startOffset_;
        offset_ = 0;
    } else {
        base_ = nullptr;
        capacity_ = 0;
        size_ = 0;
        offset_ = 0;
    }
}

MPEG4SampleMemory::~MPEG4SampleMemory() = default;

MemoryType MPEG4SampleMemory::GetMemoryType()
{
    return MemoryType::CUSTOM_MEMORY;
}

uint8_t* MPEG4SampleMemory::GetAddr()
{
    return base_;
}

int32_t MPEG4SampleMemory::GetCapacity()
{
    return capacity_;
}

int32_t MPEG4SampleMemory::GetSize()
{
    return size_;
}

int32_t MPEG4SampleMemory::GetOffset()
{
    return offset_;
}

int32_t MPEG4SampleMemory::Write(const uint8_t *in, int32_t writeSize, int32_t position)
{
    (void)in;
    (void)writeSize;
    (void)position;
    return 0;
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
