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
const char *SRT_PATH = "/data/test/fuzz_create.srt";
int64_t expectSize = 37;
size_t timeSize = 5;
size_t uriSize = 25;
int64_t uriBufferSize = 20;
char flag = '\0';
size_t trackTypeSize = 26;
size_t durationSize = 27;
size_t heightSize = 28;
size_t frameRateSize = 29;
size_t languageSize = 31;
int64_t languageBufferSize = 2;
size_t codecConfigSize = 32;
size_t sampleRateSize = 33;
size_t channelCount = 34;
size_t videoHeightSize = 35;
size_t videoWidthSize = 36;

bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < expectSize) {
        return false;
    }
    int32_t fd = open(SRT_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size - 36);
    if (len <= 0) {
        return false;
    }
    close(fd);
    struct Params params;
    params.time = data[size - timeSize];
    char *uri = new char[21];
    memcpy(uri, data  + size - uriSize, uriBufferSize);
    uri[uriBufferSize] = flag;
    params.setTrackType = data[size - trackTypeSize];
    params.setDuration = data[size - durationSize];
    params.setHeight = data[size - heightSize];
    params.setFrameRate = data[size - frameRateSize];
    char *setLanguage = new char[3];
    memcpy(setLanguage, data + size - languageSize, languageBufferSize);
    setLanguage[languageBufferSize] = flag;
    params.setCodecConfigSize = data[size - codecConfigSize];
    params.sampleRate = data[size - sampleRateSize];
    params.channelCount = data[size - channelCount];
    params.setVideoHeight = data[size - videoHeightSize];
    params.setVideoWidth = data[size - videoWidthSize];
    uint8_t *dataConver = const_cast<uint8_t *>(data);
    uint32_t *createSize = reinterpret_cast<uint32_t *>(dataConver + size - 4);
    shared_ptr<DemuxerSample> demuxerSample = make_shared<DemuxerSample>();
    demuxerSample->filePath = SRT_PATH;
    demuxerSample->RunNormalDemuxerApi11(*createSize, uri, setLanguage, params);
    delete[] uri;
    delete[] setLanguage;
    int ret = remove(SRT_PATH);
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
