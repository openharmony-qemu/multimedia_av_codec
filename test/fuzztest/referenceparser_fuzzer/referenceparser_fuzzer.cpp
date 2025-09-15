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

bool DoReferenceParserWithDemuxerAPI(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    int32_t fd = open(MP4_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    uint8_t *pstream = nullptr;
    uint16_t framesize = fdp.ConsumeIntegralInRange<uint16_t>(0, 0xfff);
    pstream = (uint8_t *)malloc(framesize * sizeof(uint8_t));
    if (!pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        return false;
    }
    fdp.ConsumeData(pstream, framesize);
    int len = write(fd, data, size);
    if (len <= EXPECT_SIZE) {
        close(fd);
        free(pstream);
        pstream = nullptr;
        return false;
    }
    free(pstream);
    pstream = nullptr;
    close(fd);
    int64_t pts = std::max(fdp.ConsumeIntegral<int64_t>(), static_cast<int64_t>(0));
    int64_t ptsForPtsIndex = fdp.ConsumeIntegral<int64_t>();
    int64_t frameIndex = fdp.ConsumeIntegral<int64_t>();
    uint32_t createSize = fdp.ConsumeIntegral<uint32_t>();
    shared_ptr<ParserSample> parserSample = make_shared<ParserSample>();
    parserSample->filePath = MP4_PATH;
    parserSample->RunReferenceParser(pts, ptsForPtsIndex, frameIndex, createSize);
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
