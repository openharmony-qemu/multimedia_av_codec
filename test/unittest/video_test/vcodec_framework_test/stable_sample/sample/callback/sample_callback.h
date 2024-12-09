/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "codec_buffer_queue.h"
#include "vcodec_signal.h"
#include "native_avbuffer.h"
#include "native_avformat.h"
#include "native_avmemory.h"

namespace OHOS {
namespace MediaAVCodec {
constexpr uint64_t TEST_FREQUENCY = 29;
void OnErrorVoid(OH_AVCodec *codec, int32_t errorCode, void *userData);
void OnStreamChangedVoid(OH_AVCodec *codec, OH_AVFormat *format, void *userData);

void InDataVoid(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
void OutDataVoid(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData);
void InBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
void OutBufferVoid(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

void InDataHandle(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
void OutDataHandle(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData);
void InBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
void OutBufferHandle(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

void InDataOperate(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
void OutDataOperate(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData);
void InBufferOperate(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
void OutBufferOperate(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

void InDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);
void OutDataQueue(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData);
void InBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);
void OutBufferQueue(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

void InputBufferLoop(std::shared_ptr<VCodecSignal> &signal);
void OutputBufferLoop(std::shared_ptr<VCodecSignal> &signal);
} // namespace MediaAVCodec
} // namespace OHOS
#endif