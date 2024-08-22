/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#include "avcodec_common.h"
#include "avcodec_audio_common.h"
#include "native_avcodec_audioencoder.h"
#include "common/native_mfmagic.h"
#include "native_avcodec_audiocodec.h"
#include "avcodec_audio_encoder.h"
#include "audio_encoder_reset_demo.h"
#define FUZZ_PROJECT_NAME "audioencoderreset_fuzzer"

using namespace OHOS::MediaAVCodec::AudioAacEncDemo;

namespace OHOS {
bool AudioEncoderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ OH_AudioCodec_CreateByMime
    std::string codecdata(reinterpret_cast<const char*>(data), size);
    OH_AVCodec *source =  OH_AudioCodec_CreateByMime(codecdata.c_str(), true);
    if (source) {
        OH_AudioCodec_Destroy(source);
    }
    OH_AVCodec *sourcename =  OH_AudioCodec_CreateByName(codecdata.c_str());
    if (sourcename) {
        OH_AudioCodec_Destroy(sourcename);
    }
    return true;
}

bool AudioEncoderAACResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ aac
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("aac");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderOPUSResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ opus
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("opus");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderG711ResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ g711
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("g711");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderLBVCResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ lbvc
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("lbvc");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderFLACResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ flac
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("flac");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderAMRNBResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrnb
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("amrnb");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderAMRWBResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrwb
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("amrwb");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderMP3ResetFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ mp3
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("mp3");
    bool ret = aDecBufferDemo->RunCaseReset(data, size);
    delete aDecBufferDemo;
    return ret;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioEncoderFuzzTest(data, size);
    OHOS::AudioEncoderAACResetFuzzTest(data, size);
    OHOS::AudioEncoderG711ResetFuzzTest(data, size);
    OHOS::AudioEncoderOPUSResetFuzzTest(data, size);
    OHOS::AudioEncoderLBVCResetFuzzTest(data, size);
    OHOS::AudioEncoderFLACResetFuzzTest(data, size);
    OHOS::AudioEncoderAMRNBResetFuzzTest(data, size);
    OHOS::AudioEncoderAMRWBResetFuzzTest(data, size);
    OHOS::AudioEncoderMP3ResetFuzzTest(data, size);
    return 0;
}
