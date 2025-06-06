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

#ifndef DEMUXER_INNER_FUNC_SAMPLE_H
#define DEMUXER_INNER_FUNC_SAMPLE_H

#include "native_avformat.h"
#include "native_avmemory.h"
#include "avsource.h"
#include "avdemuxer.h"

namespace OHOS {
namespace Media {
using namespace OHOS::MediaAVCodec;
class DemuxerInnerFuncSample {
public:
    DemuxerInnerFuncSample() = default;
    ~DemuxerInnerFuncSample();
    bool Init(const uint8_t *data, size_t size);
    void GetCurrentCacheSize(uint32_t trackIndex);
private:
    int32_t fd_ = -1;
    std::shared_ptr<AVSource> source_ = nullptr;
    std::shared_ptr<AVDemuxer> demuxer_ = nullptr;
};
} // namespace Media
} // namespace OHOS

#endif // DEMUXER_INNER_FUNC_SAMPLE_H
