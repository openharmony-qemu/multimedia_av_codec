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

#ifndef MEDIA_AVCODEC_DESCRIPTION_H
#define MEDIA_AVCODEC_DESCRIPTION_H

#include "meta/format.h"

namespace OHOS {
namespace MediaAVCodec {
/**
 * @brief Provides the uniform key for storing the media description.
 */
using MediaDescription = Media::Format;

/**
 * @brief Provides the key's definition for MediaDescription.
 */
class MediaDescriptionKey {
public:
    /**
     * Key for track index, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_TRACK_INDEX = "track_index";

    /**
     * Key for track type, value type is uint8_t, see {link @MediaTrackType}
     */
    static constexpr std::string_view MD_KEY_TRACK_TYPE = "track_type";

    /**
     * Key for codec mime type, value type is string
     */
    static constexpr std::string_view MD_KEY_CODEC_MIME = "codec_mime";

    /**
     * Key for codec name, value type is string
     */
    static constexpr std::string_view MD_KEY_CODEC_NAME = "codec_name";

    /**
     * Key for duration, value type is int64_t
     */
    static constexpr std::string_view MD_KEY_DURATION = "duration";

    /**
     * Key for bitrate, value type is int64_t
     */
    static constexpr std::string_view MD_KEY_BITRATE = "bitrate";

    /**
     * Key for max input size, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_MAX_INPUT_SIZE = "max_input_size";

    /**
     * Key for max input buffer count, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_MAX_INPUT_BUFFER_COUNT = "max_input_buffer_count";

    /**
     * Key for max output buffer count, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_MAX_OUTPUT_BUFFER_COUNT = "max_output_buffer_count";

    /**
     * Key for video width, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_WIDTH = "width";

    /**
     * Key for video height, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_HEIGHT = "height";

    /**
     * Key for video pixelformat, value type is int32_t, see {link @MediaPixelFormat}
     */
    static constexpr std::string_view MD_KEY_PIXEL_FORMAT = "pixel_format";

    /**
     * Key for video scale type, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_SCALE_TYPE = "scale_type";

    /**
     * Key for video rotation angle, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_ROTATION_ANGLE = "rotation_angle";

    /**
     * Key for video frame rate, value type is double.
     */
    static constexpr std::string_view MD_KEY_FRAME_RATE = "frame_rate";

    /**
     * Key for video capture rate, value type is double
     */
    static constexpr std::string_view MD_KEY_CAPTURE_RATE = "capture_rate";

    /**
     * Key for the interval of key frame. value type is int32_t, the unit is milliseconds.
     * A negative value means no key frames are requested after the first frame. A zero
     * value means a stream containing all key frames is requested.
     */
    static constexpr std::string_view MD_KEY_I_FRAME_INTERVAL = "i_frame_interval";

    /**
     * Key for the request a I-Frame immediately. value type is boolean
     */
    static constexpr std::string_view MD_KEY_REQUEST_I_FRAME = "req_i_frame";

    /**
     * Key for video YUV value range flag, value type is bool
     */
    static constexpr std::string_view MD_KEY_RANGE_FLAG = "range_flag";

    /**
     * Key for video color primaries, value type is int32_t, see {link @ColorPrimary}
     */
    static constexpr std::string_view MD_KEY_COLOR_PRIMARIES = "color_primaries";

    /**
     * Key for video transfer characteristics, value type is int32_t, see {link @TransferCharacteristic}
     */
    static constexpr std::string_view MD_KEY_TRANSFER_CHARACTERISTICS = "transfer_characteristics";

    /**
     * Key for video maxtrix coefficients, value type is int32_t, see {link @MaxtrixCoefficient}
     */
    static constexpr std::string_view MD_KEY_MATRIX_COEFFICIENTS = "matrix_coefficients";

    /**
     * Key for video encode bitrate mode, the value type is int32_t, see {link @VideoEncodeBitrateMode}
     */
    static constexpr std::string_view MD_KEY_VIDEO_ENCODE_BITRATE_MODE = "video_encode_bitrate_mode";

    /**
     * Key for video encode temporal scalability, the value type is int32_t
     */
    static constexpr std::string_view OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY =
        "video_encoder_enable_temporal_scalability";

    /**
     * Key for encode profile, the value type is int32_t
     */
    static constexpr std::string_view MD_KEY_PROFILE = "codec_profile";

    /**
     * key for the desired encoding quality, the value type is uint32_t, this key is only supported for encoders that
     * are configured in constant quality mode
     */
    static constexpr std::string_view MD_KEY_QUALITY = "quality";

    /**
     * Key for audio channel count, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_CHANNEL_COUNT = "channel_count";

    /**
     * Key for audio sample rate, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_SAMPLE_RATE = "sample_rate";

    /**
     * Key for track count, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_TRACK_COUNT = "track_count";

    /**
     * custom key prefix, media service will pass through to HAL.
     */
    static constexpr std::string_view MD_KEY_CUSTOM_PREFIX = "vendor.custom";

    /**
     * Key for codec specific data buffer, vlaue type is uint8_t*
     */
    static constexpr std::string_view MD_KEY_CODEC_CONFIG = "codec_config";

    /**
     * Key for audio channel layout, value type is int64_t
     */
    static constexpr std::string_view MD_KEY_CHANNEL_LAYOUT = "channel_layout";

    /**
     * Key for audio sample format, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_AUDIO_SAMPLE_FORMAT = "audio_sample_format";

    /**
     * Key for the number of bits used to encode each sample, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_BITS_PER_CODED_SAMPLE = "bits_per_coded_sample";

    /**
     * Key for aac type, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_AAC_IS_ADTS = "aac_is_adts";

    /**
     * Key for aac sbr, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_SBR = "sbr";

    /**
     * Key for compliance level, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_COMPLIANCE_LEVEL = "compliance_level";

    /**
     * Key for vorbis identification header, value type is uint8_t*
     */
    static constexpr std::string_view MD_KEY_IDENTIFICATION_HEADER = "identification_header";

    /**
     * Key for vorbis setup header, value type is uint8_t*
     */
    static constexpr std::string_view MD_KEY_SETUP_HEADER = "setup_header";

    /**
     * Key for audio frame size, means samples per frame, value type is int32_t*
     */
    static constexpr std::string_view MD_KEY_AUDIO_SAMPLES_PER_FRAME = "audio_samples_per_frame";

    /**
     * Key for Number of delayed video frames, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_VIDEO_DELAY = "video_delay";

    /**
     * Key for codec compression leve, value type is uint32_t
     */
    static constexpr std::string_view MD_KEY_COMPRESSION_LEVEL = "audio_compression_level";
    
    /**
     * Key for encode level, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_LEVEL = "codec_level";

    /**
     * Key for video chroma location, value type is int32_t
     */
    static constexpr std::string_view MD_KEY_CHROMA_LOCATION = "chroma_location";

    /**
     * Key for cover of file, value type is uint8_t pointer
     */
    static constexpr std::string_view MD_KEY_COVER = "cover";

    /**
     * Key for video is hdr vivid, value type is bool
     */
    static constexpr std::string_view MD_KEY_VIDEO_IS_HDR_VIVID = "video_is_hdr_vivid";

    /**
     * Key for hdr vivid video cuvv configuration box, value type is uint8_t*
     */
    static constexpr std::string_view MD_KEY_VIDEO_CUVV_CONFIG_BOX = "cuvv_config_box";
private:
    MediaDescriptionKey() = delete;
    ~MediaDescriptionKey() = delete;
};

/**
 * @brief
 *
 * @since 4.0
 * @version 4.0
 */
enum ColorPrimary : int32_t {
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
};

/**
 * @brief
 *
 * @since 4.0
 * @version 4.0
 */
enum TransferCharacteristic : int32_t {
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
};

/**
 * @brief
 *
 * @since 4.0
 * @version 4.0
 */
enum MatrixCoefficient : int32_t {
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
};

/**
 * @brief
 *
 * @since 4.0
 * @version 4.0
 */
enum ChromaLocation {
    CHROMA_LOC_UNSPECIFIED = 0,
    CHROMA_LOC_LEFT = 1, ///< MPEG-2/4 4:2:0, H.264 default for 4:2:0
    CHROMA_LOC_CENTER = 2, ///< MPEG-1 4:2:0, JPEG 4:2:0, H.263 4:2:0
    CHROMA_LOC_TOPLEFT = 3, ///< ITU-R 601, SMPTE 274M 296M S314M(DV 4:1:1), mpeg2 4:2:2
    CHROMA_LOC_TOP = 4,
    CHROMA_LOC_BOTTOMLEFT = 5,
    CHROMA_LOC_BOTTOM = 6,
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_DESCRIPTION_H
