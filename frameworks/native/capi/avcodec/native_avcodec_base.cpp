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

#include "native_avcodec_base.h"

#ifdef __cplusplus
extern "C" {
#endif

const char *OH_AVCODEC_MIMETYPE_VIDEO_AVC = "video/avc";
const char *OH_AVCODEC_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
const char *OH_AVCODEC_MIMETYPE_VIDEO_HEVC = "video/hevc";
const char *OH_AVCODEC_MIMETYPE_AUDIO_AAC = "audio/mp4a-latm";
const char *OH_AVCODEC_MIMETYPE_AUDIO_FLAC = "audio/flac";
const char *OH_AVCODEC_MIMETYPE_AUDIO_VORBIS = "audio/vorbis";
const char *OH_AVCODEC_MIMETYPE_AUDIO_MPEG = "audio/mpeg";
const char *OH_AVCODEC_MIMETYPE_IMAGE_JPG = "image/jpeg";
const char *OH_AVCODEC_MIMETYPE_IMAGE_PNG = "image/png";
const char *OH_AVCODEC_MIMETYPE_IMAGE_BMP = "image/bmp";
const char *OH_AVCODEC_MIMETYPE_AUDIO_VIVID = "audio/av3a";
const char *OH_AVCODEC_MIMETYPE_AUDIO_AMR_NB = "audio/3gpp";
const char *OH_AVCODEC_MIMETYPE_AUDIO_AMR_WB = "audio/amr-wb";
const char *OH_AVCODEC_MIMETYPE_AUDIO_OPUS = "audio/opus";
const char *OH_AVCODEC_MIMETYPE_AUDIO_G711MU = "audio/g711mu";

const char *OH_ED_KEY_TIME_STAMP = "timeStamp";
const char *OH_ED_KEY_EOS = "endOfStream";
const char *OH_MD_KEY_TRACK_TYPE = "track_type";
const char *OH_MD_KEY_CODEC_MIME = "codec_mime";
const char *OH_MD_KEY_DURATION = "duration";
const char *OH_MD_KEY_BITRATE = "bitrate";
const char *OH_MD_KEY_MAX_INPUT_SIZE = "max_input_size";
const char *OH_MD_KEY_WIDTH = "width";
const char *OH_MD_KEY_HEIGHT = "height";
const char *OH_MD_KEY_PIXEL_FORMAT = "pixel_format";
const char *OH_MD_KEY_AUDIO_SAMPLE_FORMAT = "audio_sample_format";
const char *OH_MD_KEY_FRAME_RATE = "frame_rate";
const char *OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE = "video_encode_bitrate_mode";
const char *OH_MD_KEY_PROFILE = "codec_profile";
const char *OH_MD_KEY_AUD_CHANNEL_COUNT = "channel_count";
const char *OH_MD_KEY_AUD_SAMPLE_RATE = "sample_rate";
const char *OH_MD_KEY_I_FRAME_INTERVAL = "i_frame_interval";
const char *OH_MD_KEY_ROTATION = "rotation_angle";
const char *OH_MD_KEY_CODEC_CONFIG = "codec_config";
const char *OH_MD_KEY_REQUEST_I_FRAME = "req_i_frame";
const char *OH_MD_KEY_RANGE_FLAG = "range_flag";
const char *OH_MD_KEY_COLOR_PRIMARIES = "color_primaries";
const char *OH_MD_KEY_TRANSFER_CHARACTERISTICS = "transfer_characteristics";
const char *OH_MD_KEY_MATRIX_COEFFICIENTS = "matrix_coefficients";
const char *OH_MD_KEY_QUALITY = "quality";
const char *OH_MD_KEY_CHANNEL_LAYOUT = "channel_layout";
const char *OH_MD_KEY_BITS_PER_CODED_SAMPLE = "bits_per_coded_sample";
const char *OH_MD_KEY_AAC_IS_ADTS = "aac_is_adts";
const char *OH_MD_KEY_SBR = "sbr";
const char *OH_MD_KEY_COMPLIANCE_LEVEL = "compliance_level";
const char *OH_MD_KEY_IDENTIFICATION_HEADER = "identification_header";
const char *OH_MD_KEY_SETUP_HEADER = "setup_header";
const char *OH_MD_KEY_SCALING_MODE = "scale_type";
const char *OH_MD_MAX_INPUT_BUFFER_COUNT = "max_input_buffer_count";
const char *OH_MD_MAX_OUTPUT_BUFFER_COUNT = "max_output_buffer_count";
const char *OH_MD_KEY_VIDEO_IS_HDR_VIVID = "video_is_hdr_vivid";

const char *OH_MD_KEY_TITLE = "title";
const char *OH_MD_KEY_ARTIST = "artist";
const char *OH_MD_KEY_ALBUM = "album";
const char *OH_MD_KEY_ALBUM_ARTIST = "album_artist";
const char *OH_MD_KEY_DATE = "date";
const char *OH_MD_KEY_COMMENT = "comment";
const char *OH_MD_KEY_GENRE = "genre";
const char *OH_MD_KEY_COPYRIGHT = "copyright";
const char *OH_MD_KEY_LANGUAGE = "language";
const char *OH_MD_KEY_DESCRIPTION = "description";
const char *OH_MD_KEY_LYRICS = "lyrics";
const char *OH_MD_KEY_TRACK_COUNT = "track_count";

const char *OH_MD_KEY_AUDIO_COMPRESSION_LEVEL = "audio_compression_level";
const char *OH_MD_KEY_AUDIO_OBJECT_NUMBER = "audio_object_number_key";
const char *OH_MD_KEY_AUDIO_VIVID_METADATA = "audio_vivid_metadata_key";

#ifdef __cplusplus
}
#endif
