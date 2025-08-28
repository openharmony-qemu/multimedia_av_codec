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

#include <fcntl.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <iostream>
#include "demuxersetdatasourcewithprobsize_sample.h"

#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const int64_t EXPECT_SIZE = 64;
const char* TEST_FILE_PATH = "/data/test/demuxersetdatasourcewithprobsizefuzztest.mp4";

bool CheckDataValidity(FuzzedDataProvider *fdp, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(TEST_FILE_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    uint8_t *pstream = nullptr;
    uint16_t framesize = fdp->ConsumeIntegralInRange<uint16_t>(0, 0xfff);
    pstream = (uint8_t *)malloc(framesize * sizeof(uint8_t));
    if (!pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        return false;
    }
    fdp->ConsumeData(pstream, framesize);
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
    return true;
}

void DemuxerSetDataSourceWithProbSizeFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider provider(data, size);
    if (!CheckDataValidity(&provider, size)) {
        return;
    }
    std::string typeName = provider.ConsumeRandomLengthString();
    int probSize = provider.ConsumeIntegral<int32_t>();
    DemuxerPluginTest test;
    test.Run(typeName, TEST_FILE_PATH, probSize);
    (void)remove(TEST_FILE_PATH);
    return;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DemuxerSetDataSourceWithProbSizeFuzzTest(data, size);
    return 0;
}