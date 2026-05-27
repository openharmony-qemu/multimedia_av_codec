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
bool CheckDataValidity(const uint8_t *data, size_t dataSize)
{
    if (dataSize <= EXPECT_SIZE) {
        return false;
    }
    int32_t fd = open(CAF_PATH, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR);
    if (fd < 0) {
        return false;
    }
    int len = write(fd, data, dataSize);
    if (len <= 0) {
        close(fd);
        return false;
    }
    close(fd);
    return true;
}
bool EncodeDualFuzzTest(const uint8_t *data, size_t size)
{
    cout << "EncodeDualFuzzTest in" << endl;
    if (!CheckDataValidity(data, size)) {
        cout << "EncodeDualFuzzTest out1" << endl;
        return false;
    }
    shared_ptr<VideoTransCodeApi11Sample> sample = make_shared<VideoTransCodeApi11Sample>();
    sample->INP_DIR = "/data/test/fuzz_create.h265";
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory("video/hevc", false, HARDWARE);
    string g_codecNameDecHEVC = OH_AVCapability_GetName(cap);
    sample->CreateVideocoder(g_codecNameDecHEVC, OH_AVCODEC_MIMETYPE_VIDEO_HEVC);
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
