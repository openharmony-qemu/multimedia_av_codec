/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
#include "native_avmagic.h"
#include "codeclist_demo.h"
#include "native_avcodec_base.h"

namespace OHOS {
namespace MediaAVCodec {

OH_AVCapability* SelectAudioType()
{
    std::cout << "Please select audio type(default AAC): " << std::endl;
    std::cout << "0:AAC" << std::endl;
    std::cout << "1:Flac" << std::endl;
    std::cout << "2:Vorbis" << std::endl;
    std::cout << "3:MPEG" << std::endl;
    std::cout << "4:G711mu" << std::endl;
    std::cout << "5:AMR-nb" << std::endl;
    std::cout << "6:AMR-wb" << std::endl;
    std::cout << "7:APE" << std::endl;
    std::string mode;
    OH_AVCapability *avCap = nullptr;
    (void)getline(std::cin, mode);
    if (mode == "0" || mode == "") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_AAC, false);
    } else if (mode == "1") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_FLAC, false);
    } else if (mode == "2") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_VORBIS, false);
    } else if (mode == "3") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_MPEG, false);
    } else if (mode == "4") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_G711MU, false);
    } else if (mode == "5") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB, false);
    } else if (mode == "6") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB, false);
    } else if (mode == "7") {
        avCap = OH_AVCodec_GetCapability(OH_AVCODEC_MIMETYPE_AUDIO_APE, false);
    }
    return avCap;
}

void NativeSelectSampleRateRanges()
{
    OH_AVCapability *avCapability = SelectAudioType();
    if (avCapability == nullptr) {
        std::cout << "not support decode" << std::endl;
        return;
    }
    OH_AVRange *sampleRateRanges = nullptr;
    uint32_t rangesNum = 0;
    std::string codecName = avCapability->capabilityData_->codecName;
    OH_AVErrCode ret = OH_AVCapability_GetAudioSupportedSampleRateRanges(avCapability, &sampleRateRanges, &rangesNum);
    if (ret == AV_ERR_OK) {
        std::cout << codecName << " support sample rate range number:" << rangesNum << std::endl;
        for (uint32_t i = 0; i < rangesNum; i++) {
            std::cout << codecName << " support sample rate range[" << i << "]:" << sampleRateRanges[i].minVal
                << "~" << sampleRateRanges[i].maxVal << std::endl;
        }
    } else {
        std::cout << "get " << codecName << " sample rate ranges fail" << std::endl;
    }
}

void NativeSelectSampleRate()
{
    OH_AVCapability *avCapability = SelectAudioType();
    if (avCapability == nullptr) {
        std::cout << "not support decode" << std::endl;
        return;
    }
    const int32_t *sampleRate = nullptr;
    uint32_t rangesNum = 0;
    std::string codecName = avCapability->capabilityData_->codecName;
    OH_AVErrCode ret = OH_AVCapability_GetAudioSupportedSampleRates(avCapability, &sampleRate, &rangesNum);
    if (ret == AV_ERR_OK) {
        std::cout << codecName << " support sample rate number:" << rangesNum << std::endl;
        std::cout << codecName << " support sample rate:" << std::endl;
        for (uint32_t i = 0; i < rangesNum; i++) {
            std::cout << sampleRate[i];
            if (i == rangesNum - 1) {
                std::cout << std::endl;
            } else {
                std::cout << ", ";
            }
        }
    } else {
        std::cout << "get " << codecName << " sample rate fail" << std::endl;
    }
}

void CodecListDemo::RunCase()
{
    std::cout << "===== ============== ======" << std::endl;
    const char *mime = "video/avc";
    OH_AVCapability *cap = OH_AVCodec_GetCapability(mime, false);
    const char *name = OH_AVCapability_GetName(cap);
    std::cout << name << std::endl;
    std::cout << "get caps successful" << std::endl;

    std::cout << "===== ============== ======" << std::endl;
    std::cout << "Please select a capability test(default 0)" << std::endl;
    std::cout << "0:get audio supported sample rate ranges" << std::endl;
    std::cout << "1:get audio supported sample rate" << std::endl;
    std::string mode;
    (void)getline(std::cin, mode);
    if (mode == "0" || mode == "") {
        (void)NativeSelectSampleRateRanges();
    } else {
        (void)NativeSelectSampleRate();
    }
}
} // namespace MediaAVCodec
} // namespace OHOS