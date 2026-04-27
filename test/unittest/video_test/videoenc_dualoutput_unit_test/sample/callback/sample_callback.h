/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef SAMPLE_CALLBACK_H
#define SAMPLE_CALLBACK_H

#include "native_avbuffer.h"
#include "native_avcodec_videoencoder.h"
#include "native_avformat.h"
#include "native_avmemory.h"

namespace OHOS {
namespace MediaAVCodec {

void OnErrorVoid(OH_AVCodec *codec, int32_t errorCode, void *userData);
void OnStreamChangedVoid(OH_AVCodec *codec, OH_AVFormat *format, void *userData);
void InBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
void OutBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
void OutBufferHandleSecondary(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
void InBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

} // namespace MediaAVCodec
} // namespace OHOS
#endif // SAMPLE_CALLBACK_H
