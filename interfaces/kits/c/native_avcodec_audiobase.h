/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
/**
 * @addtogroup CodecBase
 * @{
 *
 * @brief The CodecBase module provides variables, properties, and functions
 * for audio and video muxer, demuxer, and basic encoding and decoding functions.
 *
 * @since 9
 */
/**
 * @file native_avcodec_audiobase.h
 *
 * @brief Declare the Native API used for basic audio encoding and decoding functions,
 * as well as audio-specific configurations and parameters.
 *
 * @kit AVCodecKit
 * @library libnative_media_codecbase.so
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 26.2.0
 */

#ifndef NATIVE_AVCODEC_AUDIOBASE_H
#define NATIVE_AVCODEC_AUDIOBASE_H
#include <stdint.h>
#include "native_avformat.h"
#include "native_averrors.h"
#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief DSD decoding key ID. The value is of the int32_t type from {@link OH_AVCodec_DSDCodecType}.
 * This key is optional for the audio decoder.
 * It is used only to identify the DSD decoding type parameter.
 * The default value is OH_AVCODEC_DSD_CODEC_LSBF_PLANAR(0).
 * @since 26.2.0
 */
extern const char *OH_MD_KEY_AUDIO_DSD_CODEC_TYPE;

/**
 * @brief DSD codec type.
 * @since 26.2.0
 */
typedef enum OH_AVCodec_DSDCodecType {
    /**
     * @brief dsf container format for LSB first and flat layout.
     * @since 26.2.0
     */
    OH_AVCODEC_DSD_CODEC_LSBF_PLANAR = 0,
    /**
     * @brief MSB first and flat layout of the dsf container format.
     * @since 26.2.0
     */
    OH_AVCODEC_DSD_CODEC_MSBF_PLANAR = 1,
    /**
     * @brief MSB first and interleaved dff container format.
     * @since 26.2.0
     */
    OH_AVCODEC_DSD_CODEC_MSBF = 2,
} OH_AVCodec_DSDCodecType;

#ifdef __cplusplus
}
#endif
#endif // NATIVE_AVCODEC_AUDIOBASE_H
/** @} */
