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

#ifndef NATIVE_AVCODEC_VIDEO_CODEC_H
#define NATIVE_AVCODEC_VIDEO_CODEC_H

#include <stdint.h>
#include <stdio.h>
#include "native_averrors.h"
#include "native_avformat.h"
#include "native_avbuffer.h"
#include "native_avcodec_base.h"

#ifdef __cplusplus
extern "C" {
#endif

OH_AVCodec *OH_VideoCodec_CreateByMime(const char *mime, bool isEncoder);
OH_AVCodec *OH_VideoCodec_CreateByName(const char *name);
OH_AVErrCode OH_VideoCodec_Destroy(OH_AVCodec *codec);
OH_AVErrCode OH_VideoCodec_SetCallback(OH_AVCodec *codec, OH_AVCodecCallback callback, void *userData);
OH_AVErrCode OH_VideoCodec_SetOutputBufferQueue(OH_AVCodec *codec, OH_AVBufferQueueProducer *bufferQueue);
OH_AVErrCode OH_VideoCodec_SetSurface(OH_AVCodec *codec, OHNativeWindow *window);
OH_AVErrCode OH_VideoCodec_Configure(OH_AVCodec *codec, OH_AVFormat *format);
OH_AVErrCode OH_VideoCodec_Prepare(OH_AVCodec *codec);
OH_AVBufferQueueProducer *OH_VideoCodec_SetOutputBufferQueue(OH_AVCodec *codec);
OH_AVErrCode OH_VideoCodec_CreateSurface(OH_AVCodec *codec, OHNativeWindow **surface);
OH_AVErrCode OH_VideoCodec_Start(OH_AVCodec *codec);
OH_AVErrCode OH_VideoCodec_Stop(OH_AVCodec *codec);
OH_AVErrCode OH_VideoCodec_Flush(OH_AVCodec *codec);
OH_AVErrCode OH_VideoCodec_Reset(OH_AVCodec *codec);
OH_AVFormat *OH_VideoCodec_GetOutputDescription(OH_AVCodec *codec);
OH_AVErrCode OH_VideoCodec_SetParameter(OH_AVCodec *codec, OH_AVFormat *format);
OH_AVErrCode OH_VideoCodec_NotifyEndOfStream(OH_AVCodec *codec);
OH_AVErrCode OH_VideoCodec_SurfaceModeReturnBuffer(OH_AVCodec *codec, OH_AVBuffer *buffer, bool available);
#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVCODEC_VIDEO_CODEC_H