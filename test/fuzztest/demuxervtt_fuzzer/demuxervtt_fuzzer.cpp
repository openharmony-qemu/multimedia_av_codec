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
const char *VTT_PATH = "/data/test/fuzz_create.vtt";
const int64_t EXPECT_SIZE = 37;
const size_t TIME_SIZE = 5;
const size_t URI_SIZE = 25;
const size_t URI_BUFFER_SIZE = 21;
const int64_t URI_COUNT = 20;
const char FLAG = '\0';
const size_t TRACK_TYPE_SIZE = 26;
const size_t DURATION_SIZE = 27;
const size_t HEIGHT_SIZE = 28;
const size_t FRAME_RATE_SIZE = 29;
const size_t LANGUAGE_SIZE = 31;
const size_t LANGUAGE_BUFFER_SIZE = 3;
const size_t LANGUAGE_COUNT = 2;
const size_t CODEC_CONFIG_SIZE = 32;
const size_t SAMPLE_RATE_SIZE = 33;
const size_t CHANNEL_COUNT = 34;
const size_t VIDEO_HEIGHT_SIZE = 35;
const size_t VIDEO_WIDTH_SIZE = 36;
bool CheckDataValidity(const uint8_t *data, size_t size)
{
    if (size < EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(VTT_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, size - 36);
    if (len <= 0) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}
bool DemuxerFuzzTest(const uint8_t *data, size_t size)
{
    if (!CheckDataValidity(data, size)) {
        return false;
    }
    struct Params params;
    params.time = data[size - TIME_SIZE];
    char *uri = new char[URI_BUFFER_SIZE];
    if (memcpy_s(uri, URI_BUFFER_SIZE, data  + size - URI_SIZE, URI_COUNT) != 0) {
        delete[] uri;
        return false;
    }
    uri[URI_COUNT] = FLAG;
    params.setTrackType = data[size - TRACK_TYPE_SIZE];
    params.setDuration = data[size - DURATION_SIZE];
    params.setHeight = data[size - HEIGHT_SIZE];
    params.setFrameRate = data[size - FRAME_RATE_SIZE];
    char *setLanguage = new char[LANGUAGE_BUFFER_SIZE];
    if (memcpy_s(setLanguage, LANGUAGE_BUFFER_SIZE, data + size - LANGUAGE_SIZE, LANGUAGE_COUNT) != 0) {
        delete[] uri;
        delete[] setLanguage;
        return false;
    }
    setLanguage[LANGUAGE_COUNT] = FLAG;
    params.setCodecConfigSize = data[size - CODEC_CONFIG_SIZE];
    params.sampleRate = data[size - SAMPLE_RATE_SIZE];
    params.channelCount = data[size - CHANNEL_COUNT];
    params.setVideoHeight = data[size - VIDEO_HEIGHT_SIZE];
    params.setVideoWidth = data[size - VIDEO_WIDTH_SIZE];
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
    OHOS::DemuxerFuzzTest(data, size);
    return 0;
}
