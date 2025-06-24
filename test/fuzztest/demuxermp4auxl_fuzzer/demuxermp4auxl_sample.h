/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef DEMUXER_SAMPLE_H
#define DEMUXER_SAMPLE_H

#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

namespace OHOS {
namespace Media {
class DemuxerMp4AuxlSample {
public:
    DemuxerMp4AuxlSample() = default;
    ~DemuxerMp4AuxlSample();
    const char *filePath = "/data/test/fuzz_create.mp4";
    void RunNormalDemuxer(uint32_t createSize, int64_t time);
private:
    int CreateDemuxer();
    void GetFormat(OH_AVFormat* format);
    int fd;
    OH_AVFormat *sourceFormat;
    OH_AVSource *source;
    OH_AVDemuxer *demuxer;
};
} // namespace Media
} // namespace OHOS

#endif // DEMUXER_SAMPLE_H
