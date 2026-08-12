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

#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <fcntl.h>
#include <fuzzer/FuzzedDataProvider.h>
#include <iostream>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include "demuxer_sample.h"
#include "securec.h"

#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const char *THREE_G2_PATH = "/data/test/fuzz_create.3g2";
const int64_t EXPECT_SIZE = 90;
const size_t URI_BUFFER_SIZE = 21;
const int64_t URI_COUNT = 20;
const char FLAG = '\0';
const size_t LANGUAGE_BUFFER_SIZE = 3;
const size_t LANGUAGE_COUNT = 2;
const uint32_t MIN_BUFFER_SIZE = 1;
const uint32_t MAX_BUFFER_SIZE = 10 * 1024 * 1024;

bool CheckDataValidity(FuzzedDataProvider *fdp, size_t dataSize)
{
    if (dataSize <= EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(THREE_G2_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    uint8_t *pstream = nullptr;
    size_t framesize = dataSize - EXPECT_SIZE;
    pstream = (uint8_t *)malloc(framesize * sizeof(uint8_t));
    if (!pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        close(fd);
        return false;
    }
    fdp->ConsumeData(pstream, framesize);
    int len = write(fd, pstream, framesize);
    close(fd);
    free(pstream);
    pstream = nullptr;
    if (len <= 0) {
        return false;
    }
    return true;
}

bool DemuxerFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (!CheckDataValidity(&fdp, size)) {
        return false;
    }
    struct Params params;
    params.time = fdp.ConsumeIntegral<int64_t>();
    std::unique_ptr<char[]> uri(new char[URI_BUFFER_SIZE]);
    if (strncpy_s(uri.get(), URI_BUFFER_SIZE, fdp.ConsumeBytesAsString(URI_COUNT).c_str(), URI_COUNT) != 0) {
        return false;
    }
    uri[URI_COUNT] = FLAG;
    params.setTrackType = fdp.ConsumeIntegral<int64_t>();
    params.setDuration = fdp.ConsumeIntegral<int64_t>();
    params.setHeight = fdp.ConsumeIntegral<int64_t>();
    params.setFrameRate = fdp.ConsumeIntegral<int64_t>();
    std::unique_ptr<char[]> setLanguage(new char[LANGUAGE_BUFFER_SIZE]);
    if (strncpy_s(setLanguage.get(), LANGUAGE_BUFFER_SIZE, fdp.ConsumeBytesAsString(LANGUAGE_COUNT).c_str(),
        LANGUAGE_COUNT) != 0) {
        return false;
    }
    setLanguage[LANGUAGE_COUNT] = FLAG;
    params.setCodecConfigSize = fdp.ConsumeIntegral<int64_t>();
    params.sampleRate = fdp.ConsumeIntegral<int32_t>();
    params.channelCount = fdp.ConsumeIntegral<int32_t>();
    params.setVideoHeight = fdp.ConsumeIntegral<int32_t>();
    params.setVideoWidth = fdp.ConsumeIntegral<int32_t>();
    uint32_t rawCreateSize = fdp.ConsumeIntegral<uint32_t>();
    uint32_t createSize = std::max(MIN_BUFFER_SIZE, std::min(rawCreateSize, MAX_BUFFER_SIZE));
    shared_ptr<DemuxerSample> demuxerSample = make_shared<DemuxerSample>();
    demuxerSample->filePath = THREE_G2_PATH;
    demuxerSample->RunNormalDemuxerApi11(createSize, uri.get(), setLanguage.get(), params);
    int ret = remove(THREE_G2_PATH);
    if (ret != 0) {
        std::cerr << "Failed tp remove file: " << THREE_G2_PATH << std::endl;
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
