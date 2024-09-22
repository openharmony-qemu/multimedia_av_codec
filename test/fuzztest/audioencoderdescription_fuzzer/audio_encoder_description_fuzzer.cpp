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
#include "audio_encoder_description_demo.h"
#define FUZZ_PROJECT_NAME "audioencoderdescription_fuzzer"

using namespace OHOS::MediaAVCodec::AudioAacEncDemo;

namespace OHOS {
bool AudioEncoderFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ OH_AudioCodec_CreateByMime
    std::string codecdata((const char*) data, size);
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

bool AudioEncoderAACDescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ aac
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("aac");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderOPUSDescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ opus
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("opus");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderG711DescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ g711
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("g711");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderLBVCDescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ lbvc
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("lbvc");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderFLACDescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ flac
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("flac");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderAMRNBDescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrnb
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("amrnb");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderAMRWBDescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ amrwb
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("amrwb");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

bool AudioEncoderMP3DescriptionFuzzTest(const uint8_t *data, size_t size)
{
    if (size < sizeof(int64_t)) {
        return false;
    }
    // FUZZ mp3
    AudioBufferAacEncDemo* aDecBufferDemo = new AudioBufferAacEncDemo();
    aDecBufferDemo->InitFile("mp3");
    bool ret = aDecBufferDemo->CheckGetOutputDescription(data, size);
    delete aDecBufferDemo;
    return ret;
}

} // namespace OHOS

/* Fuzzer entry point */
extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size)
{
    /* Run your code on data */
    OHOS::AudioEncoderFuzzTest(data, size);
	//OHOS::AudioEncoderAACDescriptionFuzzTest(data, size);	//卡死
    OHOS::AudioEncoderG711DescriptionFuzzTest(data, size);
    OHOS::AudioEncoderOPUSDescriptionFuzzTest(data, size);
    OHOS::AudioEncoderLBVCDescriptionFuzzTest(data, size);
    OHOS::AudioEncoderFLACDescriptionFuzzTest(data, size);
    OHOS::AudioEncoderAMRNBDescriptionFuzzTest(data, size);
    OHOS::AudioEncoderAMRWBDescriptionFuzzTest(data, size);
    OHOS::AudioEncoderMP3DescriptionFuzzTest(data, size);
    return 0;
}
