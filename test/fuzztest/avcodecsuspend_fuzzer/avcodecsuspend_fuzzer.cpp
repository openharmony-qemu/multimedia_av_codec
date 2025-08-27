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
const int64_t ONE = 1;
const int64_t TWO = 2;
const int64_t THREE = 3;
namespace OHOS {
uint8_t *g_pstream = nullptr;
uint16_t g_framesize = 0;
size_t g_length = 0;
size_t g_copyLength = 0;
bool GetData(FuzzedDataProvider *fdp)
{
    g_framesize = fdp->ConsumeIntegralInRange<uint16_t>(0, 0xfff);
    if (g_framesize < sizeof(pid_t)) {
        return false;
    }
    g_pstream = (uint8_t *)malloc(g_framesize * sizeof(uint8_t));
    if (!g_pstream) {
        std::cerr << "Memory alloction failed" << std::endl;
        return false;
    }
    fdp->ConsumeData(g_pstream, g_framesize);
    g_length = g_framesize / sizeof(pid_t);
    g_copyLength = g_length * sizeof(pid_t);
    return true;
}
void ReleaseData()
{
    free(g_pstream);
    g_pstream = nullptr;
}
bool FreezeAvitive(FuzzedDataProvider *fdp)
{
    if (!GetData(fdp)) {
        return false;
    }
    std::vector<pid_t> pidFuzz(g_length);
    errno_t result = memcpy_s(pidFuzz.data(), g_copyLength, g_pstream, g_copyLength);
    ReleaseData();
    if (result != 0) {
        return false;
    }
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    return true;
}
bool FreezeAvitiveRepeat(FuzzedDataProvider *fdp, std::vector<pid_t> pid)
{
    if (!GetData(fdp)) {
        return false;
    }
    std::vector<pid_t> pidFuzz(g_length);
    errno_t result = memcpy_s(pidFuzz.data(), g_copyLength, g_pstream, g_copyLength);
    ReleaseData();
    if (result != 0) {
        return false;
    }
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pid);
    return true;
}
bool FreezeRepeat(FuzzedDataProvider *fdp, std::vector<pid_t> pid)
{
    if (!GetData(fdp)) {
        return false;
    }
    std::vector<pid_t> pidFuzz(g_length);
    errno_t result = memcpy_s(pidFuzz.data(), g_copyLength, g_pstream, g_copyLength);
    ReleaseData();
    if (result != 0) {
        return false;
    }
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    return true;
}
bool AvcodecSuspend001FuzzTest(FuzzedDataProvider *fdp)
{
    std::vector<pid_t> pid;
    pid_t pid0 = getpid();
    pid.push_back(pid0);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pid);
    if (!GetData(fdp)) {
        return false;
    }
    std::vector<pid_t> pidFuzz1(g_length);
    errno_t result = memcpy_s(pidFuzz1.data(), g_copyLength, g_pstream, g_copyLength);
    ReleaseData();
    if (result != 0) {
        return false;
    }
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz1);
    if (!GetData(fdp)) {
        return false;
    }
    std::vector<pid_t> pidFuzz2(g_length);
    result = memcpy_s(pidFuzz2.data(), g_copyLength, g_pstream, g_copyLength);
    ReleaseData();
    if (result != 0) {
        return false;
    }
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz2);
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pid);
    if (!GetData(fdp)) {
        return false;
    }
    std::vector<pid_t> pidFuzz3(g_length);
    result = memcpy_s(pidFuzz3.data(), g_copyLength, g_pstream, g_copyLength);
    ReleaseData();
    if (result != 0) {
        return false;
    }
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz3);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    return true;
}

bool AvcodecSuspend002FuzzTest(FuzzedDataProvider *fdp)
{
    if (!GetData(fdp)) {
        return false;
    }
    std::vector<pid_t> pidFuzz1(g_length);
    errno_t result = memcpy_s(pidFuzz1.data(), g_copyLength, g_pstream, g_copyLength);
    ReleaseData();
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
    MediaAVCodec::AVCodecSuspend::SuspendFreeze(pidFuzz1);
    MediaAVCodec::AVCodecSuspend::SuspendActive(pidFuzz1);
    MediaAVCodec::AVCodecSuspend::SuspendActiveAll();
    vDecSample->SetVideoDecoderCallback();
    vDecSample->StartVideoDecoder();
    if (!FreezeRepeat(fdp, pid)) {
        return false;
    }
    vDecSample->Flush();
    if (!FreezeAvitiveRepeat(fdp, pid)) {
        return false;
    }
    vDecSample->Stop();
    if (!FreezeAvitive(fdp)) {
        return false;
    }
    vDecSample->Release();
    delete vDecSample;
    vDecSample = nullptr;
    return true;
}

bool AvcodecSuspend003FuzzTest(FuzzedDataProvider *fdp)
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
    auto remaining_data = fdp->ConsumeRemainingBytes<uint8_t>();
    vDecSample->InputFuncFUZZ(remaining_data.data(), remaining_data.size());
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
    FuzzedDataProvider fdp(data, size);
    int32_t choose = fdp.ConsumeIntegralInRange<int32_t>(ONE, THREE);
    if (choose == ONE) {
        OHOS::AvcodecSuspend001FuzzTest(&fdp);
    } else if (choose == TWO) {
        OHOS::AvcodecSuspend002FuzzTest(&fdp);
    } else {
        OHOS::AvcodecSuspend003FuzzTest(&fdp);
    }
    return 0;
 }