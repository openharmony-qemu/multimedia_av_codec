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
#include <memory>
#include <fuzzer/FuzzedDataProvider.h>
#include "demuxerplugintype_sample.h"

#define FUZZ_PROJECT_NAME "demuxerpluginwav_fuzzer"
using namespace std;
using namespace OHOS::Media;

namespace OHOS {
const int64_t EXPECT_SIZE = 4;
void DemuxerPluginFuzzWithFunc(const uint8_t *data, size_t dataSize)
{
    if (dataSize < EXPECT_SIZE) {
        return;
    }
    std::shared_ptr<DemuxerPluginTypeTest> demuxerTest = std::make_shared<DemuxerPluginTypeTest>();
    demuxerTest->testFilePath_ = "/data/test/demuxerpluginwav.wav";
    demuxerTest->demuxerPluginName_ = "avdemux_wav";
    FuzzedDataProvider fdp(data, dataSize);
    uint8_t *pstream = nullptr;
    uint16_t framesize = dataSize - EXPECT_SIZE;
    pstream = (uint8_t *)malloc(framesize * sizeof(uint8_t));
    if (!pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        return;
    }
    fdp.ConsumeData(pstream, framesize);
    bool ret = demuxerTest->InitWithData(pstream, framesize);
    free(pstream);
    pstream = nullptr;
    if (ret) {
        demuxerTest->RunDemuxerInterfaceFuzz();
    }
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DemuxerPluginFuzzWithFunc(data, size);
    return 0;
}