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
#include "audio_encoder_flush_demo.h"
#define FUZZ_PROJECT_NAME "audioencoderflush_fuzzer"

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

bool AudioEncoderAACFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ aac
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("aac");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderOPUSFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ opus
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("opus");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderG711FlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ g711
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("g711");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderLBVCFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ lbvc
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("lbvc");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderFLACFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ flac
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("flac");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderAMRNBFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrnb
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("amrnb");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderAMRWBFlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrwb
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("amrwb");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderMP3FlushFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ mp3
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("mp3");
    bool ret = aDecBufferDemo->RunCaseFlush(data, size);
    delete aDecBufferDemo;
    return ret;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioEncoderFuzzTest(data, size);
    OHOS::AudioEncoderAACFlushFuzzTest(data, size);
    OHOS::AudioEncoderG711FlushFuzzTest(data, size);
    OHOS::AudioEncoderOPUSFlushFuzzTest(data, size);
    OHOS::AudioEncoderLBVCFlushFuzzTest(data, size);
    OHOS::AudioEncoderFLACFlushFuzzTest(data, size);
    OHOS::AudioEncoderAMRNBFlushFuzzTest(data, size);
    OHOS::AudioEncoderAMRWBFlushFuzzTest(data, size);
    OHOS::AudioEncoderMP3FlushFuzzTest(data, size);
    return 0;
}
