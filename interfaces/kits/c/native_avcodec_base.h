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

#ifndef NATIVE_AVCODEC_BASE_H
#define NATIVE_AVCODEC_BASE_H

#include <stdint.h>
#include <stdio.h>
#include "native_avbuffer.h"
#include "native_avmemory.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NativeWindow OHNativeWindow;
typedef struct OH_AVCodec OH_AVCodec;

/**
 * @brief When an error occurs in the running of the OH_AVCodec instance, the function pointer will be called
 * to report specific error information.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec OH_AVCodec instance
 * @param errorCode specific error code
 * @param userData User specific data
 * @since 9
 * @version 1.0
 */
typedef void (*OH_AVCodecOnError)(OH_AVCodec *codec, int32_t errorCode, void *userData);

/**
 * @brief When the output stream changes, the function pointer will be called to report the new stream description
 * information. It should be noted that the life cycle of the OH_AVFormat pointer
 * is only valid when the function pointer is called, and it is forbidden to continue to access after the call ends.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec OH_AVCodec instance
 * @param format New output stream description information
 * @param userData User specific data
 * @since 9
 * @version 1.0
 */
typedef void (*OH_AVCodecOnStreamChanged)(OH_AVCodec *codec, OH_AVFormat *format, void *userData);

/**
 * @brief When OH_AVCodec needs new input data during the running process,
 * the function pointer will be called and carry an available Buffer to fill in the new input data.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec OH_AVCodec instance
 * @param index The index corresponding to the newly available input buffer.
 * @param data New available input buffer.
 * @param userData User specific data
 * @deprecated since 11
 * @useinstead OH_AVCodecOnNeedInputBuffer
 * @since 9
 * @version 1.0
 */
typedef void (*OH_AVCodecOnNeedInputData)(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData);

/**
 * @brief When new output data is generated during the operation of OH_AVCodec, the function pointer will be
 * called and carry a Buffer containing the new output data. It should be noted that the life cycle of the
 * OH_AVCodecBufferAttr pointer is only valid when the function pointer is called. , which prohibits continued
 * access after the call ends.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec OH_AVCodec instance
 * @param index The index corresponding to the new output Buffer.
 * @param data Buffer containing the new output data
 * @param attr The description of the new output Buffer, please refer to {@link OH_AVCodecBufferAttr}
 * @param userData specified data
 * @deprecated since 11
 * @useinstead OH_AVCodecOnNewOutputBuffer
 * @since 9
 * @version 1.0
 */
typedef void (*OH_AVCodecOnNewOutputData)(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data,
                                          OH_AVCodecBufferAttr *attr, void *userData);

/**
 * @brief When OH_AVCodec needs new input data during the running process,
 * the function pointer will be called and carry an available Buffer to fill in the new input data.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec OH_AVCodec instance
 * @param index The index corresponding to the newly available input buffer.
 * @param buffer New available input buffer.
 * @param userData User specific data
 * @since 11
 */
typedef void (*OH_AVCodecOnNeedInputBuffer)(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

/**
 * @brief When new output data is generated during the operation of OH_AVCodec, the function pointer will be
 * called and carry a Buffer containing the new output data.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param codec OH_AVCodec instance
 * @param index The index corresponding to the new output Buffer.
 * @param buffer Buffer containing the new output buffer.
 * @param userData specified data
 * @since 11
 */
typedef void (*OH_AVCodecOnNewOutputBuffer)(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData);

/**
 * @brief A collection of all asynchronous callback function pointers in OH_AVCodec. Register an instance of this
 * structure to the OH_AVCodec instance, and process the information reported through the callback to ensure the
 * normal operation of OH_AVCodec.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param onError Monitor OH_AVCodec operation errors, refer to {@link OH_AVCodecOnError}
 * @param onStreamChanged Monitor codec stream information, refer to {@link OH_AVCodecOnStreamChanged}
 * @param onNeedInputData Monitoring codec requires input data, refer to {@link OH_AVCodecOnNeedInputData}
 * @param onNeedOutputData Monitor codec to generate output data, refer to {@link OH_AVCodecOnNewOutputData}
 * @deprecated since 11
 * @useinstead OH_AVCodecCallback
 * @since 9
 * @version 1.0
 */
typedef struct OH_AVCodecAsyncCallback {
    OH_AVCodecOnError onError;
    OH_AVCodecOnStreamChanged onStreamChanged;
    OH_AVCodecOnNeedInputData onNeedInputData;
    OH_AVCodecOnNewOutputData onNeedOutputData;
} OH_AVCodecAsyncCallback;

/**
 * @brief A collection of all asynchronous callback function pointers in OH_AVCodec. Register an instance of this
 * structure to the OH_AVCodec instance, and process the information reported through the callback to ensure the
 * normal operation of OH_AVCodec.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @param onError Monitor OH_AVCodec operation errors, refer to {@link OH_AVCodecOnError}
 * @param onStreamChanged Monitor codec stream information, refer to {@link OH_AVCodecOnStreamChanged}
 * @param onNeedInputBuffer Monitoring codec requires input buffer, refer to {@link OH_AVCodecOnNeedInputBuffer}
 * @param onNewOutputBuffer Monitor codec to generate output buffer, refer to {@link OH_AVCodecOnNewOutputBuffer}
 * @since 11
 */
typedef struct OH_AVCodecCallback {
    OH_AVCodecOnError onError;
    OH_AVCodecOnStreamChanged onStreamChanged;
    OH_AVCodecOnNeedInputBuffer onNeedInputBuffer;
    OH_AVCodecOnNewOutputBuffer onNewOutputBuffer;
} OH_AVCodecCallback;

/**
 * @brief Enumerates the MIME types of audio and video codecs
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
extern const char *OH_AVCODEC_MIMETYPE_VIDEO_AVC;
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_AAC;

/**
 * @brief Enumerates the MIME types of audio and video codecs
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_FLAC;
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_VORBIS;
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_MPEG;
extern const char *OH_AVCODEC_MIMETYPE_VIDEO_HEVC;

/**
 * @brief Enumerates the types of audio and video muxer
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @deprecated since 11
 * @since 10
 */
extern const char *OH_AVCODEC_MIMETYPE_VIDEO_MPEG4;

/**
 * @brief Enumerates the types of audio and video muxer
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
extern const char *OH_AVCODEC_MIMETYPE_IMAGE_JPG;
extern const char *OH_AVCODEC_MIMETYPE_IMAGE_PNG;
extern const char *OH_AVCODEC_MIMETYPE_IMAGE_BMP;

/**
 * @brief Enumerates the MIME types of audio codecs
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 11
 */
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_VIVID;
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB;
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB;
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_OPUS;
extern const char *OH_AVCODEC_MIMETYPE_AUDIO_G711MU;

/**
 * @brief The extra data's key of surface Buffer
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
/* Key for timeStamp in surface's extraData, value type is int64 */
extern const char *OH_ED_KEY_TIME_STAMP;
/* Key for endOfStream in surface's extraData, value type is bool */
extern const char *OH_ED_KEY_EOS;

/**
 * @brief Provides the uniform key for storing the media description.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
/* Key for track type, value type is uint8_t, see @OH_MediaType. */
extern const char *OH_MD_KEY_TRACK_TYPE;
/* Key for codec mime type, value type is string. */
extern const char *OH_MD_KEY_CODEC_MIME;
/* Key for duration, value type is int64_t. */
extern const char *OH_MD_KEY_DURATION;
/* Key for bitrate, value type is int64_t. */
extern const char *OH_MD_KEY_BITRATE;
/* Key for max input size, value type is uint32_t */
extern const char *OH_MD_KEY_MAX_INPUT_SIZE;
/* Key for video width, value type is uint32_t */
extern const char *OH_MD_KEY_WIDTH;
/* Key for video height, value type is uint32_t */
extern const char *OH_MD_KEY_HEIGHT;
/* Key for video pixel format, value type is int32_t, see @OH_AVPixelFormat */
extern const char *OH_MD_KEY_PIXEL_FORMAT;
/* key for audio raw format, value type is uint32_t , see @AudioSampleFormat */
extern const char *OH_MD_KEY_AUDIO_SAMPLE_FORMAT;
/* Key for video frame rate, value type is double. */
extern const char *OH_MD_KEY_FRAME_RATE;
/* video encode bitrate mode, the value type is int32_t, see @OH_VideoEncodeBitrateMode */
extern const char *OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE;
/* encode profile, the value type is number. see @OH_AVCProfile, OH_HEVCProfile, OH_AACProfile. */
extern const char *OH_MD_KEY_PROFILE;
/* Key for audio channel count, value type is uint32_t */
extern const char *OH_MD_KEY_AUD_CHANNEL_COUNT;
/* Key for audio sample rate, value type is uint32_t */
extern const char *OH_MD_KEY_AUD_SAMPLE_RATE;
/* Key for the interval of key frame. value type is int32_t, the unit is milliseconds. */
extern const char *OH_MD_KEY_I_FRAME_INTERVAL;
/* Key of the surface rotation angle. value type is int32_t: should be {0, 90, 180, 270}, default is 0. */
extern const char *OH_MD_KEY_ROTATION;

/**
 * @brief Provides the uniform key for storing the media description.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
/* Key for video YUV value range flag, value type is boolean */
extern const char *OH_MD_KEY_RANGE_FLAG;
/* Key for video color primaries, value type is int32_t, see @OH_ColorPrimary */
extern const char *OH_MD_KEY_COLOR_PRIMARIES;
/* Key for video transfer characteristics, value type is int32_t, see @OH_TransferCharacteristic */
extern const char *OH_MD_KEY_TRANSFER_CHARACTERISTICS;
/* Key for video matrix coefficients, value type is int32_t, see @OH_MatrixCoefficient */
extern const char *OH_MD_KEY_MATRIX_COEFFICIENTS;
/* Key for the request an I-Frame immediately, value type is boolean */
extern const char *OH_MD_KEY_REQUEST_I_FRAME;
/* Key for the desired encoding quality, value type is uint32_t, this key is only
 * supported for encoders that are configured in constant quality mode */
extern const char *OH_MD_KEY_QUALITY;
/* Key of the codec specific data. value type is a uint8_t pointer */
extern const char *OH_MD_KEY_CODEC_CONFIG;
/* source format Key for title, value type is string */
extern const char *OH_MD_KEY_TITLE;
/* source format Key for artist, value type is string */
extern const char *OH_MD_KEY_ARTIST;
/* source format Key for album, value type is string */
extern const char *OH_MD_KEY_ALBUM;
/* source format Key for album artist, value type is string */
extern const char *OH_MD_KEY_ALBUM_ARTIST;
/* source format Key for date, value type is string */
extern const char *OH_MD_KEY_DATE;
/* source format Key for comment, value type is string */
extern const char *OH_MD_KEY_COMMENT;
/* source format Key for genre, value type is string */
extern const char *OH_MD_KEY_GENRE;
/* source format Key for copyright, value type is string */
extern const char *OH_MD_KEY_COPYRIGHT;
/* source format Key for language, value type is string */
extern const char *OH_MD_KEY_LANGUAGE;
/* source format Key for description, value type is string */
extern const char *OH_MD_KEY_DESCRIPTION;
/* source format Key for lyrics, value type is string */
extern const char *OH_MD_KEY_LYRICS;
/* source format Key for track count, value type is uint32_t */
extern const char *OH_MD_KEY_TRACK_COUNT;
/* Key for the desired encoding channel layout, value type is int64_t, this key is only supported for encoders */
extern const char *OH_MD_KEY_CHANNEL_LAYOUT;
/* Key for bits per coded sample, value type is uint32_t, supported for flac encoder, see @OH_BitsPerSample */
extern const char *OH_MD_KEY_BITS_PER_CODED_SAMPLE;
/* Key for the aac format, value type is uint32_t, supported for aac decoder */
extern const char *OH_MD_KEY_AAC_IS_ADTS;
/* Key for aac sbr mode, value type is uint32_t, supported for aac encoder */
extern const char *OH_MD_KEY_SBR;
/* Key for flac compliance level, value type is int32_t */
extern const char *OH_MD_KEY_COMPLIANCE_LEVEL;
/* Key for vorbis identification header, value type is a uint8_t pointer, supported only for vorbis decoder */
extern const char *OH_MD_KEY_IDENTIFICATION_HEADER;
/* Key for vorbis setup header, value type is a uint8_t pointer, supported only for vorbis decoder */
extern const char *OH_MD_KEY_SETUP_HEADER;
/* Key for video scale type, value type is int32_t, see @OH_ScalingMode */
extern const char *OH_MD_KEY_SCALING_MODE;
/* Key for max input buffer count, value type is int32_t */
extern const char *OH_MD_MAX_INPUT_BUFFER_COUNT;
/* Key for max output buffer count, value type is int32_t */
extern const char *OH_MD_MAX_OUTPUT_BUFFER_COUNT;

/**
 * @brief Provides the uniform key for storing the media description.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 11
 */
/* Key for audio codec compression level, value type is uint32_t */
extern const char *OH_MD_KEY_AUDIO_COMPRESSION_LEVEL;
/* Key of the video is hdr vivid. value type is bool */
extern const char *OH_MD_KEY_VIDEO_IS_HDR_VIVID;
/* Key for number of audio objects. value type is int32_t */
extern const char *OH_MD_KEY_AUDIO_OBJECT_NUMBER;
/* Key for meta data of audio vivid. value type is a uint8_t pointer */
extern const char *OH_MD_KEY_AUDIO_VIVID_METADATA;

/**
 * @brief Media type.
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OH_MediaType {
    /* track is audio. */
    MEDIA_TYPE_AUD = 0,
    /* track is video. */
    MEDIA_TYPE_VID = 1,
} OH_MediaType;

/**
 * @brief AAC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OH_AACProfile {
    AAC_PROFILE_LC = 0,
} OH_AACProfile;

/**
 * @brief AVC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 9
 * @version 1.0
 */
typedef enum OH_AVCProfile {
    AVC_PROFILE_BASELINE = 0,
    AVC_PROFILE_HIGH = 4,
    AVC_PROFILE_MAIN = 8,
} OH_AVCProfile;

/**
 * @brief HEVC Profile
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_HEVCProfile {
    HEVC_PROFILE_MAIN = 0,
    HEVC_PROFILE_MAIN_10 = 1,
    HEVC_PROFILE_MAIN_STILL = 2,
    HEVC_PROFILE_MAIN_10_HDR10 = 3,
    HEVC_PROFILE_MAIN_10_HDR10_PLUS = 4,
} OH_HEVCProfile;

/**
 * @brief Enumerates the muxer output file format
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_AVOutputFormat {
    AV_OUTPUT_FORMAT_DEFAULT = 0,
    AV_OUTPUT_FORMAT_MPEG_4 = 2,
    AV_OUTPUT_FORMAT_M4A = 6,
} OH_AVOutputFormat;

/**
 * @brief Seek Mode
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_AVSeekMode {
    /* seek to sync sample after the time */
    SEEK_MODE_NEXT_SYNC = 0,
    /* seek to sync sample before the time */
    SEEK_MODE_PREVIOUS_SYNC,
    /* seek to sync sample closest to time */
    SEEK_MODE_CLOSEST_SYNC,
} OH_AVSeekMode;

/**
 * @brief Scaling Mode
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_ScalingMode {
    SCALING_MODE_SCALE_TO_WINDOW = 1,
    SCALING_MODE_SCALE_CROP = 2,
} OH_ScalingMode;

/**
 * @brief enum Audio Bits Per Coded Sample
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_BitsPerSample {
    SAMPLE_U8 = 0,
    SAMPLE_S16LE = 1,
    SAMPLE_S24LE = 2,
    SAMPLE_S32LE = 3,
    SAMPLE_F32LE = 4,
    SAMPLE_U8P = 5,
    SAMPLE_S16P = 6,
    SAMPLE_S24P = 7,
    SAMPLE_S32P = 8,
    SAMPLE_F32P = 9,
    INVALID_WIDTH = -1
} OH_BitsPerSample;

/**
 * @brief Color Primary
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_ColorPrimary {
    COLOR_PRIMARY_BT709 = 1,
    COLOR_PRIMARY_UNSPECIFIED = 2,
    COLOR_PRIMARY_BT470_M = 4,
    COLOR_PRIMARY_BT601_625 = 5,
    COLOR_PRIMARY_BT601_525 = 6,
    COLOR_PRIMARY_SMPTE_ST240 = 7,
    COLOR_PRIMARY_GENERIC_FILM = 8,
    COLOR_PRIMARY_BT2020 = 9,
    COLOR_PRIMARY_SMPTE_ST428 = 10,
    COLOR_PRIMARY_P3DCI = 11,
    COLOR_PRIMARY_P3D65 = 12,
} OH_ColorPrimary;

/**
 * @brief Transfer Characteristic
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_TransferCharacteristic {
    TRANSFER_CHARACTERISTIC_BT709 = 1,
    TRANSFER_CHARACTERISTIC_UNSPECIFIED = 2,
    TRANSFER_CHARACTERISTIC_GAMMA_2_2 = 4,
    TRANSFER_CHARACTERISTIC_GAMMA_2_8 = 5,
    TRANSFER_CHARACTERISTIC_BT601 = 6,
    TRANSFER_CHARACTERISTIC_SMPTE_ST240 = 7,
    TRANSFER_CHARACTERISTIC_LINEAR = 8,
    TRANSFER_CHARACTERISTIC_LOG = 9,
    TRANSFER_CHARACTERISTIC_LOG_SQRT = 10,
    TRANSFER_CHARACTERISTIC_IEC_61966_2_4 = 11,
    TRANSFER_CHARACTERISTIC_BT1361 = 12,
    TRANSFER_CHARACTERISTIC_IEC_61966_2_1 = 13,
    TRANSFER_CHARACTERISTIC_BT2020_10BIT = 14,
    TRANSFER_CHARACTERISTIC_BT2020_12BIT = 15,
    TRANSFER_CHARACTERISTIC_PQ = 16,
    TRANSFER_CHARACTERISTIC_SMPTE_ST428 = 17,
    TRANSFER_CHARACTERISTIC_HLG = 18,
} OH_TransferCharacteristic;

/**
 * @brief Matrix Coefficient
 * @syscap SystemCapability.Multimedia.Media.CodecBase
 * @since 10
 */
typedef enum OH_MatrixCoefficient {
    MATRIX_COEFFICIENT_IDENTITY = 0,
    MATRIX_COEFFICIENT_BT709 = 1,
    MATRIX_COEFFICIENT_UNSPECIFIED = 2,
    MATRIX_COEFFICIENT_FCC = 4,
    MATRIX_COEFFICIENT_BT601_625 = 5,
    MATRIX_COEFFICIENT_BT601_525 = 6,
    MATRIX_COEFFICIENT_SMPTE_ST240 = 7,
    MATRIX_COEFFICIENT_YCGCO = 8,
    MATRIX_COEFFICIENT_BT2020_NCL = 9,
    MATRIX_COEFFICIENT_BT2020_CL = 10,
    MATRIX_COEFFICIENT_SMPTE_ST2085 = 11,
    MATRIX_COEFFICIENT_CHROMATICITY_NCL = 12,
    MATRIX_COEFFICIENT_CHROMATICITY_CL = 13,
    MATRIX_COEFFICIENT_ICTCP = 14,
} OH_MatrixCoefficient;

#ifdef __cplusplus
}
#endif

#endif // NATIVE_AVCODEC_BASE_H
