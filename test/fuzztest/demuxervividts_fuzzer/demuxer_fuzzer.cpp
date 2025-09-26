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

#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fuzzer/FuzzedDataProvider.h>
#include "securec.h"

#include <iostream>
#include "demuxer_sample.h"


#define FUZZ_PROJECT_NAME "demuxervivid_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const char *TS_PATH = "/data/test/fuzz_create.ts";
const int64_t EXPECT_SIZE = 4;

bool DemuxerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(TS_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    FuzzedDataProvider fdp(data, size);
    uint8_t *pstream = nullptr;
    uint16_t framesize = size - EXPECT_SIZE;
    pstream = (uint8_t *)malloc(framesize * sizeof(uint8_t));
    if (!pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        close(fd);
        return false;
    }
    fdp.ConsumeData(pstream, framesize);
    int len = write(fd, pstream, framesize);
    if (len <= 0) {
        close(fd);
        free(pstream);
        pstream = nullptr;
        return false;
    }
    close(fd);
    free(pstream);
    pstream = nullptr;
    shared_ptr<DemuxerSample> demuxerSample = make_shared<DemuxerSample>();
    demuxerSample->filePath = TS_PATH;
    demuxerSample->RunNormalDemuxerApi11();
    int ret = remove(TS_PATH);
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
    OHOS::DemuxerFuzzTest(data, size);
    return 0;
}
