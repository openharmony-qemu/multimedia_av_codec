/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <iostream>
#include "demuxer_sample.h"


#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const char *VTT_PATH = "/data/test/fuzz_create.vtt";

bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < 37) {
        return false;
    }
    int32_t fd = open(VTT_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size - 36);
    if (len <= 0) {
        return false;
    }
    close(fd);
    struct Params params;
    params.time = data[size - 5];
    char *uri = new char[21];
    memcpy(uri, data  + size - 25, 20);
    uri[20] = '\0';
    params.setTrackType = data[size - 26];
    params.setDuration = data[size - 27];
    params.setHeight = data[size - 28];
    params.setFrameRate = data[size - 29];
    char *setLanguage = new char[3];
    memcpy(setLanguage, data + size - 31, 2);
    setLanguage[2] = '\0';
    params.setCodecConfigSize = data[size - 32];
    params.sampleRate = data[size - 33];
    params.channelCount = data[size - 34];
    params.setVideoHeight = data[size - 35];
    params.setVideoWidth = data[size - 36];
    uint8_t *dataConver = const_cast<uint8_t *>(data);
    uint32_t *createSize = reinterpret_cast<uint32_t *>(dataConver + size - 4);
    shared_ptr<DemuxerSample> demuxerSample = make_shared<DemuxerSample>();
    demuxerSample->filePath = VTT_PATH;
    demuxerSample->RunNormalDemuxer(*createSize, uri, setLanguage, params);
    delete[] uri;
    delete[] setLanguage;
    int ret = remove(VTT_PATH);
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
    OHOS::DoSomethingInterestingWithMyAPI(data, size);
    return 0;
}
