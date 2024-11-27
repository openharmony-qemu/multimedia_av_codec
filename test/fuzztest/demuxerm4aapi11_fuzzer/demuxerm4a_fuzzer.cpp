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
#include "securec.h"

#include <iostream>
#include "demuxer_sample.h"


#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const char *M4A_PATH = "/data/test/fuzz_create.m4a";
int64_t g_expectSize = 37;
size_t g_timeSize = 5;
size_t g_uriSize = 25;
size_t g_uriBufferSize = 21;
int64_t g_uriCount = 20;
char g_flag = '\0';
size_t g_trackTypeSize = 26;
size_t g_durationSize = 27;
size_t g_heightSize = 28;
size_t g_frameRateSize = 29;
size_t g_languageSize = 31;
size_t g_languageBufferSize = 3;
size_t g_languageCount = 2;
size_t g_codecConfigSize = 32;
size_t g_sampleRateSize = 33;
size_t g_channelCount = 34;
size_t g_videoHeightSize = 35;
size_t g_videoWidthSize = 36;

bool DoSomethingInterestingWithMyAPI(const uint8_t *data, size_t size)
{
    if (size < g_expectSize) {
        return false;
    }
    int32_t fd = open(M4A_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size - 36);
    if (len <= 0) {
        return false;
    }
    close(fd);
    struct Params params;
    params.time = data[size - g_timeSize];
    char *uri = new char[g_uriBufferSize];
    if (memcpy_s(uri, g_uriBufferSize, data  + size - g_uriSize, g_uriCount) != 0) {
        return false;
    }
    uri[g_uriCount] = g_flag;
    params.setTrackType = data[size - g_trackTypeSize];
    params.setDuration = data[size - g_durationSize];
    params.setHeight = data[size - g_heightSize];
    params.setFrameRate = data[size - g_frameRateSize];
    char *setLanguage = new char[g_languageBufferSize];
    if (memcpy_s(setLanguage, g_languageBufferSize, data + size - g_languageSize, g_languageCount) != 0) {
        return false;
    }
    setLanguage[g_languageCount] = g_flag;
    params.setCodecConfigSize = data[size - g_codecConfigSize];
    params.sampleRate = data[size - g_sampleRateSize];
    params.channelCount = data[size - g_channelCount];
    params.setVideoHeight = data[size - g_videoHeightSize];
    params.setVideoWidth = data[size - g_videoWidthSize];
    uint8_t *dataConver = const_cast<uint8_t *>(data);
    uint32_t *createSize = reinterpret_cast<uint32_t *>(dataConver + size - 4);
    shared_ptr<DemuxerSample> demuxerSample = make_shared<DemuxerSample>();
    demuxerSample->filePath = M4A_PATH;
    demuxerSample->RunNormalDemuxerApi11(*createSize, uri, setLanguage, params);
    delete[] uri;
    delete[] setLanguage;
    int ret = remove(M4A_PATH);
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
