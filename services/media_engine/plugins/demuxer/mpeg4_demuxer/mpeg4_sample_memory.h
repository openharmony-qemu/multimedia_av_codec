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

#ifndef MPEG4_SAMPLE_MEMORY_H
#define MPEG4_SAMPLE_MEMORY_H

#include <memory>
#include "buffer/avbuffer.h"
#include "block_queue_pool.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
class MPEG4SampleMemory : public AVMemory {
public:
    explicit MPEG4SampleMemory(std::shared_ptr<Sample> sample, int32_t offset = 0);
    ~MPEG4SampleMemory() override;

    MemoryType GetMemoryType() override;
    uint8_t* GetAddr() override;
    int32_t GetCapacity() override;
    int32_t GetSize() override;
    int32_t GetOffset() override;

    int32_t Write(const uint8_t *in, int32_t writeSize, int32_t position = INVALID_POSITION) override;
    void Reset() = delete;

private:
    std::shared_ptr<Sample> sample_ {nullptr};
    int32_t startOffset_ {0};
};
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
#endif // MPEG4_SAMPLE_MEMORY_H
