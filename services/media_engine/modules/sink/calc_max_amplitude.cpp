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
#include "common/log.h"
#include "calc_max_amplitude.h"
#include <cstdint>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "CalcMaxAmplitude" };
}
namespace OHOS {
namespace Media {
namespace CalcMaxAmplitude {
constexpr int32_t SAMPLE_S24_BYTE_NUM = 3;
constexpr int32_t ONE_BYTE_BITS = 8;
constexpr int32_t MAX_VALUE_OF_SIGNED_24_BIT = 0x7FFFFF;
constexpr int32_t MAX_VALUE_OF_SIGNED_32_BIT = 0x7FFFFFFF;
constexpr int32_t SAMPLE_U8_C = 0;
constexpr int32_t SAMPLE_S16_C = 1;
constexpr int32_t SAMPLE_S24_C = 2;
constexpr int32_t SAMPLE_S32_C = 3;

float CalculateMaxAmplitudeForPCM8Bit(int8_t *frame, uint64_t nSamples)
{
    FALSE_RETURN_V(frame != nullptr && nSamples != 0, 0.0f);
    int curMaxAmplitude = 0;
    for (uint64_t i = nSamples; i > 0; --i) {
        int8_t value = *frame++;
        if (value == INT8_MIN) {
            value = INT8_MAX;
        }
        if (value < 0) {
            value = -value;
        }
        if (curMaxAmplitude < value) {
            curMaxAmplitude = value;
        }
    }
    return float(curMaxAmplitude) / SCHAR_MAX;
}

float CalculateMaxAmplitudeForPCM16Bit(int16_t *frame, uint64_t nSamples)
{
    FALSE_RETURN_V(frame != nullptr && nSamples != 0, 0.0f);
    int curMaxAmplitude = 0;
    for (uint64_t i = nSamples; i > 0; --i) {
        int16_t value = *frame++;
        if (value == INT16_MIN) {
            value = INT16_MAX;
        }
        if (value < 0) {
            value = -value;
        }
        if (curMaxAmplitude < value) {
            curMaxAmplitude = value;
        }
    }
    return float(curMaxAmplitude) / SHRT_MAX;
}

float CalculateMaxAmplitudeForPCM24Bit(char *frame, uint64_t nSamples)
{
    FALSE_RETURN_V(frame != nullptr && nSamples != 0, 0.0f);
    int curMaxAmplitude = 0;
    for (uint64_t i = 0; i < nSamples; ++i) {
        char *curPos = frame + (i * SAMPLE_S24_BYTE_NUM);
        int32_t curValue = 0;
        for (uint32_t j = 0; j < SAMPLE_S24_BYTE_NUM; ++j) {
            curValue += (*(curPos + j) << (ONE_BYTE_BITS * j));
        }
        if (curValue == INT32_MIN) {
            curValue = INT32_MAX;
        }
        if (curValue < 0) {
            curValue = -curValue;
        }
        if (curMaxAmplitude < curValue) {
            curMaxAmplitude = curValue;
        }
    }
    return float(curMaxAmplitude) / MAX_VALUE_OF_SIGNED_24_BIT;
}

float CalculateMaxAmplitudeForPCM32Bit(int32_t *frame, uint64_t nSamples)
{
    FALSE_RETURN_V(frame != nullptr && nSamples != 0, 0.0f);
    int curMaxAmplitude = 0;
    for (uint64_t i = nSamples; i > 0; --i) {
        int32_t value = *frame++;
        if (value == INT32_MIN) {
            value = INT32_MAX;
        }
        if (value < 0) {
            value = -value;
        }
        if (curMaxAmplitude < value) {
            curMaxAmplitude = value;
        }
    }
    return float(curMaxAmplitude) / float(MAX_VALUE_OF_SIGNED_32_BIT);
}

float UpdateMaxAmplitude(char *frame, uint64_t replyBytes, int32_t adapterFormat)
{
    FALSE_RETURN_V(frame != nullptr && replyBytes != 0, 0.0f);
    switch (adapterFormat) {
        case SAMPLE_U8_C: {
            return CalculateMaxAmplitudeForPCM8Bit(reinterpret_cast<int8_t *>(frame), replyBytes);
        }
        case SAMPLE_S16_C: {
            return CalculateMaxAmplitudeForPCM16Bit(reinterpret_cast<int16_t *>(frame),
                (replyBytes / sizeof(int16_t)));
        }
        case SAMPLE_S24_C: {
            return CalculateMaxAmplitudeForPCM24Bit(frame, (replyBytes / SAMPLE_S24_BYTE_NUM));
        }
        case SAMPLE_S32_C: {
            return CalculateMaxAmplitudeForPCM32Bit(reinterpret_cast<int32_t *>(frame),
                (replyBytes / sizeof(int32_t)));
        }
        default: {
            MEDIA_LOG_I("getMaxAmplitude: Unsupported audio format: %{public}d", adapterFormat);
            return 0;
        }
    }
}
} // namespace CalcMaxAmpiitude
} // namespace Media
} // namespace OHOS