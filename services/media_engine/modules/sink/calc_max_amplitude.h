/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_CALC_MAX_AMPLITUDE_H
#define HISTREAMER_CALC_MAX_AMPLITUDE_H
#include <string>

namespace OHOS {
namespace Media {
namespace CalcMaxAmplitude {
enum ConvertHdiFormat {
    SAMPLE_U8_C = 0,
    SAMPLE_S16_C = 1,
    SAMPLE_S24_C = 2,
    SAMPLE_S32_C = 3,
    SAMPLE_F32_C = 4,
    INVALID_WIDTH_C = -1
}; // same with HdiAdapterFormat
float CalculateMaxAmplitudeForPCM8Bit(int8_t *frame, uint64_t nSamples);
float CalculateMaxAmplitudeForPCM16Bit(int16_t *frame, uint64_t nSamples);
float CalculateMaxAmplitudeForPCM24Bit(char *frame, uint64_t nSamples);
float CalculateMaxAmplitudeForPCM32Bit(int32_t *frame, uint64_t nSamples);
float UpdateMaxAmplitude(ConvertHdiFormat adapterFormat, char *frame, uint64_t replyBytes);
} // namespace CalcMaxAmplitude
} // namespace Media
} // namespace OHOS
#endif // HISTREAMER_CALC_MAX_AMPLITUDE_H