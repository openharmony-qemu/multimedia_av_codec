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
#include <fcntl.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <iostream>
#include "demuxerinner_sample.h"

#define FUZZ_PROJECT_NAME "demuxerinner_fuzzer"
using namespace std;
namespace OHOS {
namespace MediaAVCodec {
const char *MP4_PATH = "/data/test/fuzz_create.mp4";
const int64_t EXPECT_SIZE = 90;
bool CheckDataValidity(FuzzedDataProvider *fdp, size_t size)
{
    if (size <= EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(MP4_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    uint8_t *pstream = nullptr;
    uint16_t framesize = size - EXPECT_SIZE;
    pstream = (uint8_t *)malloc(framesize * sizeof(uint8_t));
    if (!pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        close(fd);
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
bool DemuxerFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (!CheckDataValidity(&fdp, size)) {
        return false;
    }
    shared_ptr<DemuxerInnerSample> demuxerInnerSample = make_shared<DemuxerInnerSample>();
    demuxerInnerSample->filePath = MP4_PATH;
    uint32_t createSize = fdp.ConsumeIntegral<uint32_t>();
    demuxerInnerSample->RunNormalDemuxerInner(createSize);
    int ret = remove(MP4_PATH);
    if (ret != 0) {
        return false;
    }
    return true;
}
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::MediaAVCodec::DemuxerFuzzTest(data, size);
    return 0;
}

