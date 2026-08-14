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
#include <fuzzer/FuzzedDataProvider.h>
#include <iostream>
#include "videotranscode_api11_sample.h"
#include "native_avcapability.h"

#define FUZZ_PROJECT_NAME "encodedual_fuzzer"
using namespace std;
using namespace OHOS::Media;
namespace OHOS {
const char *CAF_PATH = "/data/test/fuzz_create.h265";
const int64_t EXPECT_SIZE = 4;
bool CheckDataValidity(FuzzedDataProvider *fdp, size_t dataSize)
{
    if (dataSize <= EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(CAF_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    uint8_t *pstream = nullptr;
    uint16_t framesize = fdp->ConsumeIntegralInRange<uint16_t>(0, static_cast<uint16_t>(dataSize - EXPECT_SIZE));
    if (framesize == 0) {
        return false;
    }
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
bool EncodeDualFuzzTest(const uint8_t *data, size_t size)
{
    cout << "EncodeDualFuzzTest in" << endl;
    FuzzedDataProvider fdp(data, size);
    if (!CheckDataValidity(&fdp, size)) {
        cout << "EncodeDualFuzzTest out1" << endl;
        return false;
    }
    shared_ptr<VideoTransCodeApi11Sample> sample = make_shared<VideoTransCodeApi11Sample>();
    sample->inpDir = "/data/test/fuzz_create.h265";
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/hevc", false, HARDWARE);
    string codecNameDecHevc = OH_AVCapability_GetName(cap);
    sample->CreateVideocoder(codecNameDecHevc, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
    sample->Configure();
    sample->StartSecondaryEncoder();
    sample->Start();
    sample->WaitForEos();
    cout << "EncodeDualFuzzTest out2" << endl;
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::EncodeDualFuzzTest(data, size);
    return 0;
}
