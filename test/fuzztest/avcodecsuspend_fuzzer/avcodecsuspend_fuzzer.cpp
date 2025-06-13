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
#include "avcodec_suspend.h"
#include "native_avcodec_videodecoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include <unistd.h>
#include "videodec_sample.h"
#include <fuzzer/FuzzedDataProvider.h>
using namespace std;
using namespace OHOS::Media;
#define FUZZ_PROJECT_NAME "avcodecsuspend_fuzzer"

namespace OHOS {
bool AvcodecSuspend001FuzzTest(const uint8_t *data, size_t size)
{
    std::vector<pid_t> pid;
    pid_t pid0 = getpid();
    pid.push_back(pid0);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pid);

    if (size < sizeof(pid_t)) {
        return false;
    }
    size_t length = size / sizeof(pid_t);
    size_t copyLength = length * sizeof(pid_t);
    std::vector<pid_t> pidFuzz(length);
    errno_t result = memcpy_s(pidFuzz.data(), copyLength, data, copyLength);
    if (result != 0) {
        return false;
    }
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    return true;
}

bool AvcodecSuspend002FuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(pid_t)) {
        return false;
    }
    size_t length = size / sizeof(pid_t);
    size_t copyLength = length * sizeof(pid_t);
    std::vector<pid_t> pidFuzz(length);
    errno_t result = memcpy_s(pidFuzz.data(), copyLength, data, copyLength);
    if (result != 0) {
        return false;
    }
    std::vector<pid_t> pid;
    pid_t pid0 = getpid();
    pid.push_back(pid0);

    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    int32_t ret = vDecSample->CreateVideoDecoder();
    if (ret != 0) {
        delete vDecSample;
        vDecSample = nullptr;
        return false;
    }
    vDecSample->ConfigureVideoDecoder();
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->StartVideoDecoder();
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    vDecSample->Flush();
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pid);
    vDecSample->Stop();
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    vDecSample->Release();
    delete vDecSample;
    vDecSample = nullptr;
    return true;
}

bool AvcodecSuspend003FuzzTest(const uint8_t *data, size_t size)
{
    std::vector<pid_t> pid;
    pid_t pid0 = getpid();
    pid.push_back(pid0);
    
    VDecFuzzSample *vDecSample = new VDecFuzzSample();
    int32_t ret = vDecSample->CreateVideoDecoder();
    if (ret != 0) {
        delete vDecSample;
        vDecSample = nullptr;
        return false;
    }
    vDecSample->ConfigureVideoDecoder();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->Start();
    vDecSample->InputFuncFUZZ(data, size);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    vDecSample->Flush();
    vDecSample->Stop();
    vDecSample->Reset();
    delete vDecSample;
    vDecSample = nullptr;
    return true;
}
}

 /* Fuzzer entry point */
 extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
 {
     /* Run your code on data */
     OHOS::AvcodecSuspend001FuzzTest(data, size);
     OHOS::AvcodecSuspend002FuzzTest(data, size);
     OHOS::AvcodecSuspend003FuzzTest(data, size);
     return 0;
 }
 