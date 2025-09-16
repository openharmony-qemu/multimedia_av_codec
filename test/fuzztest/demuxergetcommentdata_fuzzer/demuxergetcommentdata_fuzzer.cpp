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

#include <iostream>
#include <unistd.h>
#include <fcntl.h>
#include <fuzzer/FuzzedDataProvider.h>
#include "native_avsource.h"
#include "native_avformat.h"
#include "native_avcodec_base.h"

#define FUZZ_PROJECT_NAME "demuxer_fuzzer"
using namespace std;
namespace OHOS {
const int64_t EXPECT_SIZE = 64;
const char* TEST_FILE_PATH = "/data/test/demuxergetcommentdatafuzztest.mp4";

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

void DemuxerGetCommentDataFuzzTest(const uint8_t *data, size_t size)
{
    FuzzedDataProvider fdp(data, size);
    if (!CheckDataValidity(&fdp, size)) {
        return;
    }
    const char* metaStringValue = nullptr;
    auto source = OH_AVSource_CreateWithURI(const_cast<char*>(TEST_FILE_PATH));
    if (source == nullptr) {
        return;
    }
    auto metaFormat = OH_AVSource_GetSourceFormat(source);
    if (metaFormat == nullptr) {
        OH_AVSource_Destroy(source);
        return;
    }
    OH_AVFormat_GetStringValue(metaFormat, OH_MD_KEY_COMMENT, &metaStringValue);
    OH_AVSource_Destroy(source);
    OH_AVFormat_Destroy(metaFormat);
    (void)remove(TEST_FILE_PATH);
    return;
}
}

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::DemuxerGetCommentDataFuzzTest(data, size);
    return 0;
}