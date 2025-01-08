/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef PTS_AND_INDEX_CONVERSION_SAMPLE_H
#define PTS_AND_INDEX_CONVERSION_SAMPLE_H

#include "native_avformat.h"
#include "native_avmemory.h"
#include "pts_and_index_conversion.h"

namespace OHOS {
namespace Media {
class PtsAndIndexConversion {
public:
    PtsAndIndexConversion() = default;
    ~PtsAndIndexConversion();
    const char *filePath = "/data/test/out.mp4";
    bool Init(const uint8_t *data, size_t size);
    void RunNormalTimeAndIndexConversions();
private:
    int32_t fd_ = -1;
    uint32_t index_ = 0;
    uint32_t trackIndex_ = 0;
    uint64_t relativePresentationTimeUs_ = 0;
    std::shared_ptr<TimeAndIndexConversion> timeandindexConversions_ = nullptr;
};
} // namespace Media
} // namespace OHOS

#endif // PTS_AND_INDEX_CONVERSION_SAMPLE_H
