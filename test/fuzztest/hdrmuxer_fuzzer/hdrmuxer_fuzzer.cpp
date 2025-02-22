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
#include "hdrmuxer_sample.h"
#define FUZZ_PROJECT_NAME "hdrmuxer_fuzzer"
using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
namespace OHOS {
const char *HDR_MP4_PATH = "/data/test/media/demuxer_parser_hdr_vivid.mp4";

bool HdrMuxerFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    MuxerSample *muxerSample = new MuxerSample();
    muxerSample->RunHdrMuxer(data, size, HDR_MP4_PATH);
    delete muxerSample;
    return true;
}
} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::HdrMuxerFuzzTest(data, size);
    return 0;
}
