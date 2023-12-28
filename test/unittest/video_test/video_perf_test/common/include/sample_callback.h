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

#ifndef AVCODEC_SAMPLE_SAMPLE_CALLBACK_H
#define AVCODEC_SAMPLE_SAMPLE_CALLBACK_H

#include "sample_info.h"
namespace OHOS {
namespace MediaAVCodec {
namespace Sample {
class SampleCallback {
public:
    static void OnCodecError(OH_AVCodec *codec, int32_t errorCode, void *userData);
    static void OnCodecFormatChange(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
    static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
    static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
                                OH_AVCodecBufferAttr *attr, void *userData);
    static void OnNeedInputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
    static void OnNewOutputBuffer(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
};
} // Sample
} // MediaAVCodec
} // OHOS
#endif // AVCODEC_SAMPLE_SAMPLE_CALLBACK_H