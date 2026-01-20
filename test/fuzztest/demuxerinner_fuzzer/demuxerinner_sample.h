/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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


#ifndef DEMUXER_INNER_SAMPLE_H
#define DEMUXER_INNER_SAMPLE_H

#include "avdemuxer.h"
#include "avsource.h"

namespace OHOS {
namespace MediaAVCodec {    
class DemuxerInnerSample {
public:
    DemuxerInnerSample() = default;
    ~DemuxerInnerSample();
    const char *filePath = "/data/test/fuzz_create.mp4";
    void RunNormalDemuxerInner(uint32_t createSize);
    int CreateDemuxer();
    std::shared_ptr<AVDemuxer> demuxer_ = nullptr;
    std::shared_ptr<OHOS::Media::AVBuffer> avBuffer;
private:
    std::shared_ptr<AVSource> avsource_ = nullptr;
    int32_t fd = -1;
    int32_t trackCount;
    int32_t hdrType = -2;


};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // DEMUXER_INNER_SAMPLE_H

