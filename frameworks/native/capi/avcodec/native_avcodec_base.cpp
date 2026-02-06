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

const char *OH_AVCODEC_MIMETYPE_VIDEO_MSVIDEO1 = "video/msvideo1";
const char *OH_AVCODEC_MIMETYPE_VIDEO_VC1 = "video/vc1";
const char *OH_AVCODEC_MIMETYPE_VIDEO_VP8 = "video/vp8";
const char *OH_AVCODEC_MIMETYPE_VIDEO_VP9 = "video/vp9";
const char *OH_AVCODEC_MIMETYPE_VIDEO_AV1 = "video/av1";
const char *OH_AVCODEC_MIMETYPE_VIDEO_WVC1 = "video/wvc1";
const char *OH_AVCODEC_MIMETYPE_VIDEO_AVC = "video/avc";
const char *OH_AVCODEC_MIMETYPE_VIDEO_MPEG4 = "video/mp4v-es";
const char *OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2 = "video/mp4v-es";
const char *OH_AVCODEC_MIMETYPE_VIDEO_H263 = "video/h263";
const char *OH_AVCODEC_MIMETYPE_VIDEO_HEVC = "video/hevc";
const char *OH_AVCODEC_MIMETYPE_VIDEO_MPEG2 = "video/mpeg2";
const char *OH_AVCODEC_MIMETYPE_VIDEO_MPEG1 = "video/mpeg";
const char *OH_AVCODEC_MIMETYPE_VIDEO_MJPEG = "video/mjpeg";
const char *OH_AVCODEC_MIMETYPE_VIDEO_RV30 = "video/rv30";
const char *OH_AVCODEC_MIMETYPE_VIDEO_RV40 = "video/rv40";
const char *OH_AVCODEC_MIMETYPE_VIDEO_DVVIDEO = "video/dvvideo";
const char *OH_AVCODEC_MIMETYPE_VIDEO_RAWVIDEO = "video/rawvideo";
const char *OH_AVCODEC_MIMETYPE_VIDEO_CINEPAK = "video/cinepak";
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
const char *OH_AVCODEC_MIMETYPE_AUDIO_APE = "audio/x-ape";
const char *OH_AVCODEC_MIMETYPE_VIDEO_VVC = "video/vvc";
const char *OH_AVCODEC_MIMETYPE_SUBTITLE_SRT = "application/x-subrip";
const char *OH_AVCODEC_MIMETYPE_AUDIO_LBVC = "audio/lbvc";
const char *OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT = "text/vtt";
const char *OH_AVCODEC_MIMETYPE_AUDIO_RAW = "audio/raw";
const char *OH_AVCODEC_MIMETYPE_AUDIO_G711A = "audio/g711a";
const char *OH_AVCODEC_MIMETYPE_AUDIO_EAC3 = "audio/eac3";
const char *OH_AVCODEC_MIMETYPE_AUDIO_AC3 = "audio/ac3";
const char *OH_AVCODEC_MIMETYPE_AUDIO_ALAC = "audio/alac";
const char *OH_AVCODEC_MIMETYPE_AUDIO_GSM_MS = "audio/gsm_ms";
const char *OH_AVCODEC_MIMETYPE_VIDEO_WMV3 = "video/wmv3";
const char *OH_AVCODEC_MIMETYPE_AUDIO_GSM = "audio/gsm";
const char *OH_AVCODEC_MIMETYPE_AUDIO_WMAV1 = "audio/wmav1";
const char *OH_AVCODEC_MIMETYPE_AUDIO_WMAV2 = "audio/wmav2";
const char *OH_AVCODEC_MIMETYPE_AUDIO_WMAPRO = "audio/wmapro";
const char *OH_AVCODEC_MIMETYPE_AUDIO_ILBC = "audio/ilbc";
const char *OH_AVCODEC_MIMETYPE_AUDIO_TRUEHD = "audio/truehd";
const char *OH_AVCODEC_MIMETYPE_AUDIO_TWINVQ = "audio/twinvq";
const char *OH_AVCODEC_MIMETYPE_AUDIO_DVAUDIO = "audio/dvaudio";
const char *OH_AVCODEC_MIMETYPE_AUDIO_DTS = "audio/dts";
const char *OH_AVCODEC_MIMETYPE_AUDIO_COOK = "audio/cook";

const char *OH_ED_KEY_TIME_STAMP = "timeStamp";
const char *OH_ED_KEY_EOS = "endOfStream";
const char *OH_MD_KEY_TRACK_TYPE = "track_type";
const char *OH_MD_KEY_CODEC_MIME = "codec_mime";
const char *OH_MD_KEY_DURATION = "duration";
const char *OH_MD_KEY_BITRATE = "bitrate";
const char *OH_MD_KEY_MAX_BITRATE = "max_bitrate";
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
const char *OH_MD_KEY_VIDEO_TRANSFORM_TYPE = "video_orientation_type";
const char *OH_MD_KEY_CODEC_CONFIG = "codec_config";
const char *OH_MD_KEY_REQUEST_I_FRAME = "req_i_frame";
const char *OH_MD_KEY_RANGE_FLAG = "range_flag";
const char *OH_MD_KEY_COLOR_PRIMARIES = "color_primaries";
const char *OH_MD_KEY_TRANSFER_CHARACTERISTICS = "transfer_characteristics";
const char *OH_MD_KEY_MATRIX_COEFFICIENTS = "matrix_coefficients";
const char *OH_MD_KEY_QUALITY = "quality";
const char *OH_MD_KEY_SQR_FACTOR = "sqr_factor";
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
const char *OH_MD_KEY_BLOCK_ALIGN = "audio_block_align";
const char *OH_MD_KEY_BUFFER_SKIP_SAMPLES_INFO = "buffer_skip_samples_info";
const char *OH_MD_KEY_ENABLE_BUFFER_SKIP_SAMPLES = "enable_buffer_skip_samples";

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
const char *OH_MD_KEY_REFERENCE_TRACK_IDS = "reference_track_ids";
const char *OH_MD_KEY_TRACK_REFERENCE_TYPE = "track_reference_type";
const char *OH_MD_KEY_TRACK_DESCRIPTION = "track_description";

const char *OH_MD_KEY_AUDIO_COMPRESSION_LEVEL = "audio_compression_level";
const char *OH_MD_KEY_AUDIO_OBJECT_NUMBER = "audio_object_number_key";
const char *OH_MD_KEY_AUDIO_VIVID_METADATA = "audio_vivid_metadata_key";

const char *OH_FEATURE_VIDEO_ENCODER_TEMPORAL_SCALABILITY = "feature_video_encoder_temporal_scalability";
const char *OH_FEATURE_VIDEO_ENCODER_LONG_TERM_REFERENCE = "feature_video_encoder_long_term_reference";
const char *OH_FEATURE_VIDEO_LOW_LATENCY = "feature_video_low_latency";

const char *OH_FEATURE_PROPERTY_KEY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT =
    "feature_property_video_encoder_max_ltr_frame_count";
const char *OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY = "video_encoder_enable_temporal_scalability";
const char *OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_SIZE = "video_encoder_temporal_gop_size";
const char *OH_MD_KEY_VIDEO_ENCODER_TEMPORAL_GOP_REFERENCE_MODE = "video_encoder_temporal_gop_reference_mode";
const char *OH_MD_KEY_VIDEO_ENCODER_LTR_FRAME_COUNT = "video_encoder_ltr_frame_count";
const char *OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_MARK_LTR = "video_encoder_per_frame_mark_ltr";
const char *OH_MD_KEY_VIDEO_PER_FRAME_IS_LTR = "video_per_frame_is_ltr";
const char *OH_MD_KEY_VIDEO_PER_FRAME_POC = "video_per_frame_poc";
const char *OH_MD_KEY_VIDEO_ENCODER_PER_FRAME_USE_LTR = "video_encoder_per_frame_use_ltr";
const char *OH_MD_KEY_VIDEO_CROP_TOP = "video_crop_top";
const char *OH_MD_KEY_VIDEO_CROP_BOTTOM = "video_crop_bottom";
const char *OH_MD_KEY_VIDEO_CROP_LEFT = "video_crop_left";
const char *OH_MD_KEY_VIDEO_CROP_RIGHT = "video_crop_right";
const char *OH_MD_KEY_VIDEO_STRIDE = "stride";
const char *OH_MD_KEY_VIDEO_SLICE_HEIGHT = "video_slice_height";
const char *OH_MD_KEY_VIDEO_PIC_WIDTH = "video_picture_width";
const char *OH_MD_KEY_VIDEO_PIC_HEIGHT = "video_picture_height";
const char *OH_MD_KEY_VIDEO_ENABLE_LOW_LATENCY = "video_enable_low_latency";
const char *OH_MD_KEY_VIDEO_ENCODER_QP_MAX = "video_encoder_qp_max";
const char *OH_MD_KEY_VIDEO_ENCODER_QP_MIN = "video_encoder_qp_min";
const char *OH_MD_KEY_VIDEO_ENCODER_QP_AVERAGE = "video_encoder_qp_average";
const char *OH_MD_KEY_VIDEO_ENCODER_MSE = "video_encoder_mse";
const char *OH_MD_KEY_DECODING_TIMESTAMP = "decoding_timestamp";
const char *OH_MD_KEY_BUFFER_DURATION = "buffer_duration";
const char *OH_MD_KEY_VIDEO_SAR = "video_sar";
const char *OH_MD_KEY_START_TIME = "start_time";
const char *OH_MD_KEY_TRACK_START_TIME = "track_start_time";
const char *OH_MD_KEY_VIDEO_DECODER_OUTPUT_COLOR_SPACE = "video_decoder_output_colorspace";
const char *OH_MD_KEY_VIDEO_DECODER_OUTPUT_ENABLE_VRR = "video_decoder_output_enable_vrr";
const char *OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_FRAME_AFTER = "video_encoder_repeat_previous_frame_after";
const char *OH_MD_KEY_VIDEO_ENCODER_REPEAT_PREVIOUS_MAX_COUNT = "video_encoder_repeat_previous_max_count";
const char *OH_MD_KEY_CREATION_TIME = "creation_time";
const char *OH_MD_KEY_VIDEO_ENCODER_ROI_PARAMS = "video_encoder_roi_params";
const char *OH_MD_KEY_ENABLE_MOOV_FRONT = "fast_start";
const char *OH_MD_KEY_VIDEO_ENCODER_ENABLE_PTS_BASED_RATECONTROL = "video_encoder_enable_pts_based_ratecontrol";
const char *OH_MD_KEY_ENABLE_SYNC_MODE = "enable_sync_mode";
const char *OH_MD_KEY_VIDEO_ENCODER_MAX_B_FRAMES = "video_encoder_max_b_frame";
const char *OH_MD_KEY_VIDEO_ENCODER_ENABLE_B_FRAME = "video_encoder_enable_b_frame";
const char *OH_MD_KEY_VIDEO_DECODER_BLANK_FRAME_ON_SHUTDOWN = "video_decoder_blank_frame_on_shutdown";
const char *OH_MD_KEY_VIDEO_NATIVE_BUFFER_FORMAT = "video_graphic_pixel_format";
#ifdef __cplusplus
}
#endif
