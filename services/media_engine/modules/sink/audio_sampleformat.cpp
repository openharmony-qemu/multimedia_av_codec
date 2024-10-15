/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include "audio_sampleformat.h"
#include <map>

namespace OHOS {
namespace Media {
namespace Pipeline {
const std::map<Plugins::AudioSampleFormat, int32_t> SAMPLEFORMAT_INFOS = {
    {Plugins::SAMPLE_U8, 8},
    {Plugins::SAMPLE_S16LE, 16},
    {Plugins::SAMPLE_S24LE, 24},
    {Plugins::SAMPLE_S32LE, 32},
    {Plugins::SAMPLE_F32LE, 32},
    {Plugins::SAMPLE_U8P, 8},
    {Plugins::SAMPLE_S16P, 16},
    {Plugins::SAMPLE_S24P, 24},
    {Plugins::SAMPLE_S32P, 32},
    {Plugins::SAMPLE_F32P, 32},
    {Plugins::SAMPLE_S8, 8},
    {Plugins::SAMPLE_S8P, 8},
    {Plugins::SAMPLE_U16, 16},
    {Plugins::SAMPLE_U16P, 16},
    {Plugins::SAMPLE_U24, 24},
    {Plugins::SAMPLE_U24P, 24},
    {Plugins::SAMPLE_U32, 32},
    {Plugins::SAMPLE_U32P, 32},
    {Plugins::SAMPLE_S64, 64},
    {Plugins::SAMPLE_U64, 64},
    {Plugins::SAMPLE_S64P, 64},
    {Plugins::SAMPLE_U64P, 64},
    {Plugins::SAMPLE_F64, 64},
    {Plugins::SAMPLE_F64P, 64},
    {Plugins::INVALID_WIDTH, -1},
};

int32_t AudioSampleFormatToBitDepth(Plugins::AudioSampleFormat sampleFormat)
{
    if (SAMPLEFORMAT_INFOS.count(sampleFormat) != 0) {
        return SAMPLEFORMAT_INFOS.at(sampleFormat);
    }
    return -1;
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS