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
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include "avsource.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"

#define FUZZ_PROJECT_NAME "avsource_fuzzer"
using namespace std;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
const char *FILE_PATH = "/data/test/fuzz_create.mp4";
bool DoAVSourceFuncFuzz(const uint8_t *data, size_t size)
{
    std::ofstream file(FILE_PATH, std::ios::binary);
    if (file.is_open()) {
        file.write(reinterpret_cast<const char*>(data), size);
        file.close();
    }
    int32_t fd = open(FILE_PATH, O_RDONLY);
    if (fd < 0) {
        return false;
    }

    std::shared_ptr<AVSource> source = AVSourceFactory::CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        return false;
    }
    Format metaFormat;
    int ret = source->GetUserMeta(metaFormat);
    source = nullptr;
    close(fd);
    if (ret != AVCS_ERR_OK) {
        return false;
    }
    ret = remove(FILE_PATH);
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
    OHOS::DoAVSourceFuncFuzz(data, size);
    return 0;
}