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

#ifndef PARSER_SAMPLE_H
#define PARSER_SAMPLE_H

#include "avdemuxer.h"
#include "avsource.h"

namespace OHOS {
namespace MediaAVCodec {
using AVBuffer = OHOS::Media::AVBuffer;
using AVAllocator = OHOS::Media::AVAllocator;
using AVAllocatorFactory = OHOS::Media::AVAllocatorFactory;
using MemoryFlag = OHOS::Media::MemoryFlag;
using FormatDataType = OHOS::Media::FormatDataType;
class ParserSample {
public:
    ParserSample() = default;
    ~ParserSample();
    const char *filePath = "/data/test/fuzz_create.mp4";
    void RunReferenceParser(const uint8_t *data, size_t size);
private:
    int CreateDemuxer(uint32_t buffersize);
    int fd;
    std::shared_ptr<OHOS::Media::AVBuffer> avBuffer;
    std::shared_ptr<AVSource> source = nullptr;
    std::shared_ptr<AVDemuxer> demuxer = nullptr;
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif // PARSER_SAMPLE_H