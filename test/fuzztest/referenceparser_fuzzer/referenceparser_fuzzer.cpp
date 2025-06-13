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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <iostream>
#include "parser_sample.h"

#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
namespace OHOS {
const char *MP4_PATH = "/data/test/fuzz_create.mp4";
const int64_t EXPECT_SIZE = 7;
const int64_t STRIDE = 7;

bool DoReferenceParserWithDemuxerAPI(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    int32_t fd = open(MP4_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size);
    if (len <= EXPECT_SIZE) {
        close(fd);
        return false;
    }
    close(fd);
    FuzzedDataProvider fdp(data, size);
    int64_t pts = fdp.ConsumeIntegral<int64_t>();
    int64_t ptsForPtsIndex = fdp.ConsumeIntegral<int64_t>();
    int64_t frameIndex = fdp.ConsumeIntegral<int64_t>();
    uint8_t *dataConver = const_cast<uint8_t *>(data);
    uint32_t *createSize = reinterpret_cast<uint32_t *>(dataConver + size - STRIDE);
    shared_ptr<ParserSample> parserSample = make_shared<ParserSample>();
    parserSample->filePath = MP4_PATH;
    parserSample->RunReferenceParser(pts, ptsForPtsIndex, frameIndex, *createSize);
    int ret = remove(MP4_PATH);
    if (ret != 0) {
        return false;
    }
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DoReferenceParserWithDemuxerAPI(data, size);
    return 0;
}
