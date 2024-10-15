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

#ifndef AV_CODEC_AUDIO_SAMPLEFORMAT_H
#define AV_CODEC_AUDIO_SAMPLEFORMAT_H

#include "meta/audio_types.h"
#include <cstdint>  // NOLINT: used it

namespace OHOS {
namespace Media {
namespace Pipeline {

__attribute__((visibility("default"))) int32_t AudioSampleFormatToBitDepth(Plugins::AudioSampleFormat sampleFormat);
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // AV_CODEC_AUDIO_SAMPLEFORMAT_H