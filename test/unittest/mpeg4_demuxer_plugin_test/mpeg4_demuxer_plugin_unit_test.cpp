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

#include "mpeg4_demuxer_plugin_unit_test.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cctype>
#include <condition_variable>
#include <cstdlib>
#include <dirent.h>
#include <fcntl.h>
#include <functional>
#include <iostream>
#include <limits>
#include <mutex>
#include <numeric>
#include <regex>
#include <sstream>
#include <sys/stat.h>
#include <tuple>
#include <unistd.h>

#include "demuxer_data_reader.h"
#include "demuxer_utils.h"
#include "demuxer/type_finder.h"
#include "plugin/cached_plugin_package.h"
#include "plugin/plugin_list.h"
#include "file_server_demo.h"
#include "avc_parser_impl.h"
#include "mpeg4_audio_parser.h"
#include "mpeg4_demuxer_plugin_internal.h"
#include "mpeg4_sample_memory.h"
#include "mpeg4_sample_helper.h"
#include "rbsp_context.h"
#include "test_http_port.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
Status ConvertTimeScaleRound(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result);
Status ConvertTimeScaleRoundToTimeBase(int64_t multiplicand, int64_t multiplier, int64_t timeScale, int64_t* result);
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS

namespace {
const std::string TEST_MP4_PATH_H264_B = "/data/test/media/test_264_B_Gop25_4sec_cover.mp4";
// Crafted AAC probe fixture: extreme dts pair that would overflow a signed dts difference.
const std::string TEST_MP4_PATH_AAC_PROBE_DTS_OVERFLOW = "/data/test/media/aac_probe_dts_overflow.mp4";
const std::string TEST_MP4_PATH_H265_B = "/data/test/media/test_265_B_Gop25_4sec.mp4";
const std::string TEST_MP4_PATH_HEVC_AUX = "/data/test/media/muxer_auxl_265.mp4";
const std::string TEST_MP4_PATH_HEVC_AVC_AAC = "/data/test/media/muxer_auxl_265_264_aac.mp4";
const std::string TEST_MP4_PATH_VVC = "/data/test/media/vvc_1280_720_8.mp4";
const std::string TEST_MP4_PATH_WEBVTT = "/data/test/media/webvtt.mp4";
const std::string TEST_MP4_MULTIPLE_VIDEO_TRACKS = "/data/test/media/h264_double_video_audio.mp4";
const std::string ZERO_STTS_HVC1_FIXTURE = "/data/test/media/zero_stts_hvc1_55_samples.mp4";
const std::string TEST_URI_PATH = "http://127.0.0.1:" AVCODEC_TEST_HTTP_PORT_STR "/";
const std::string TEST_RELATIVE_PATH = "/data/test/media/";
const std::string TEST_DEMUXER_RESOURCE_DIR = "/data/test/media";
const std::string FFMPEG_MP4_PLUGIN_NAME = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
const std::string MPEG4_PLUGIN_NAME = "avdemux_mpeg4";
const std::string WEBVTT_MIME = "text/vtt";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
const std::string VVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_vvc_parser.z.so";
constexpr int32_t WAIT_RETRY_TIMES = 20;
constexpr uint32_t INVALID_TRACK_ID = static_cast<uint32_t>(-1);
constexpr uint32_t WEAK_NETWORK_SHORT_TIMEOUT_MS = 10;
constexpr uint32_t WEAK_NETWORK_FAIL_DELAY_MS = 20;
constexpr int64_t WEAK_NETWORK_FIRST_READ_FAIL_OFFSET = 0;
constexpr size_t WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT = 1;
constexpr size_t WEAK_NETWORK_THREAD_RETRY_FAIL_COUNT = 20;
constexpr size_t MAX_WEBVTT_SAMPLE_READ_COUNT = 100000;
constexpr size_t MAX_EOS_SEEK_SAMPLE_COUNT = 128;
const std::vector<uint8_t> H264_AVCC_FIXTURE = {
    0x01, 0x42, 0x00, 0x1E, 0xFF, 0xE1, 0x00, 0x02, 0x67, 0x42, 0x01, 0x00, 0x02, 0x68, 0xCE,
};
const std::vector<uint8_t> H264_REORDER_TWO_AVCC_FIXTURE = {
    0x01, 0x64, 0x00, 0x32, 0xFF, 0xE1, 0x00, 0x19, 0x67, 0x64, 0x00, 0x32, 0xAC, 0xD9, 0x40, 0x28,
    0x00, 0xB5, 0xB0, 0x11, 0x00, 0x00, 0x03, 0x03, 0xE9, 0x00, 0x00, 0xEA, 0x60, 0x0F, 0x18, 0x31,
    0x96, 0x01, 0x00, 0x05, 0x68, 0xEB, 0xEC, 0xB2, 0x2C, 0xFD, 0xF8, 0xF8, 0x00,
};
// video_oddheight_lvxian.mp4 SPS: 136x182 MBs (2176x2912 coded), crop bottom 5 -> 2176x2902.
const std::vector<uint8_t> H264_ODDHEIGHT_CROP_AVCC_FIXTURE = {
    0x01, 0x64, 0x00, 0x34, 0xFF, 0xE1, 0x00, 0x2A, 0x67, 0x64, 0x00, 0x34, 0xAC, 0x2B, 0x20, 0x04,
    0x40, 0x0B, 0x6F, 0xCD, 0x80, 0xB7, 0x01, 0x01, 0x05, 0x40, 0x00, 0x00, 0xFA, 0x00, 0x00, 0x73,
    0x3C, 0x38, 0x00, 0x00, 0x03, 0x00, 0x4B, 0xF2, 0x4C, 0x00, 0x00, 0x04, 0xBF, 0x24, 0xC6, 0xEF,
    0x2E, 0x0A, 0x01, 0x00, 0x04, 0x68, 0xEB, 0x8F, 0x2C,
};
constexpr uint32_t VIDEO_DURATION_READ_BUFFER_SIZE = 64;
constexpr size_t VIDEO_DELAY_TEST_TRACK_COUNT = 1;
constexpr uint32_t VIDEO_DELAY_TEST_SAMPLE_DURATION = 1;
constexpr uint32_t VIDEO_DELAY_TEST_SPS_REORDER = 2;
constexpr uint32_t VIDEO_DELAY_TEST_SUPPORTED_SAMPLE_COUNT = VIDEO_DELAY_TEST_SPS_REORDER + 2;
constexpr uint32_t VIDEO_DELAY_TEST_INSUFFICIENT_SAMPLE_COUNT = VIDEO_DELAY_TEST_SPS_REORDER + 1;
constexpr uint32_t TRADITIONAL_METADATA_LENGTH_SHIFT = 8;
constexpr uint32_t RFPS_LINE77_STTS_EXTEND_COUNT = 40;
constexpr int64_t TEST_EOS_SEEK_TIME_MS = 4000;
constexpr int64_t TEST_MS_TO_US = 1000;
constexpr int64_t TEST_US_TO_NS = 1000;
constexpr uint32_t EOS_WAIT_FOR_CACHE_TEST_TIMEOUT_MS = 5000;
constexpr uint32_t DYNAMIC_SWITCH_PRIMARY_TRACK_ID = 0;
constexpr uint32_t DYNAMIC_SWITCH_SECONDARY_TRACK_ID = 1;
constexpr uint32_t DYNAMIC_SWITCH_NEW_TRACK_ID = 2;
constexpr size_t DYNAMIC_SWITCH_REQUIRED_TRACK_COUNT = 3;
constexpr uint32_t DYNAMIC_SWITCH_PREROLL_SAMPLE_COUNT = 6;
constexpr uint32_t DYNAMIC_SWITCH_COMPARE_SAMPLE_COUNT = 3;
constexpr uint32_t DYNAMIC_SWITCH_RESELECT_SAMPLE_COUNT = 2;
constexpr int64_t DYNAMIC_SWITCH_SEEK_TIME_MS = 5000;
constexpr uint32_t IN_FLIGHT_READ_BLOCK_TIMEOUT_SECONDS = 2;
constexpr int32_t ZERO_LENGTH_AAC_SAMPLE_RATE = 44100;
constexpr int32_t ZERO_LENGTH_AAC_SAMPLES_PER_FRAME = 1024;
constexpr int32_t AAC_TEST_CORE_SAMPLE_RATE = 24000;
constexpr int32_t AAC_TEST_CORE_SAMPLES_PER_FRAME = 1024;
constexpr int32_t AAC_TEST_CHANNEL_COUNT = 2;
constexpr int32_t AAC_SBR_SAMPLE_RATE_MULTIPLIER = 2;
constexpr size_t MP4_ATOM_HEADER_SIZE = 8; // atom header: size(4) + fourcc(4)
constexpr int32_t MULTI_TRACK_SYNC_TEST_INITIAL_SAMPLE_POS = 7;
constexpr int64_t MULTI_TRACK_SYNC_TEST_SEEK_TIME = 100;
constexpr uint32_t AAC_TEST_AUDIO_OBJECT_TYPE_AAC_LC = 2;
constexpr uint32_t AAC_TEST_AUDIO_OBJECT_TYPE_SBR = 5;
constexpr uint8_t AAC_TEST_PS_CONFIG[] = {0xEC, 0x8B, 0x08, 0x00, 0x00};
constexpr uint8_t AAC_TEST_EXPLICIT_STEREO_CONFIG[] = {0x11, 0x90};
constexpr uint8_t AAC_TEST_PCE_CONFIG[] = {
    0x11, 0x80, 0x04, 0xC8, 0x44,
    0x00, 0x20, 0x08, 0x84, 0x00,
};
constexpr uint8_t AAC_TEST_AUDIO_OBJECT_TYPE_BITS = 5;
constexpr uint32_t AAC_TEST_ASC_SYNC_EXTENSION = 0x2B7;
constexpr uint8_t AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS = 4;
constexpr uint8_t AAC_TEST_CHANNEL_CONFIGURATION_BITS = 4;
constexpr uint8_t AAC_TEST_GA_SPECIFIC_CONFIG_FLAGS_BITS = 3;
constexpr uint8_t AAC_TEST_PCE_OBJECT_TYPE_BITS = 2;
constexpr uint8_t AAC_TEST_PCE_ELEMENT_COUNT_BITS = 4;
constexpr uint8_t AAC_TEST_PCE_LFE_COUNT_BITS = 2;
constexpr uint8_t AAC_TEST_PCE_ASSOC_DATA_COUNT_BITS = 3;
constexpr uint8_t AAC_TEST_PCE_COMMENT_COUNT_BITS = 8;
constexpr uint8_t AAC_TEST_PCE_ELEMENT_IS_CPE_BITS = 1;
constexpr uint8_t AAC_TEST_PCE_ELEMENT_TAG_BITS = 4;
constexpr uint8_t AAC_TEST_PCE_FLAG_BITS = 1;
constexpr uint8_t AAC_TEST_PCE_AAC_LC_OBJECT_TYPE = 1;
constexpr uint8_t AAC_TEST_PCE_48KHZ_SAMPLE_RATE_INDEX = 3;
constexpr uint8_t AAC_TEST_PCE_CHANNEL_CONFIGURATION = 0;
constexpr uint8_t AAC_TEST_PCE_FRONT_ELEMENT_COUNT = 2;
constexpr uint8_t AAC_TEST_PCE_SIDE_ELEMENT_COUNT = 1;
constexpr uint8_t AAC_TEST_PCE_BACK_ELEMENT_COUNT = 1;
constexpr uint8_t AAC_TEST_PCE_NO_LFE_ELEMENT = 0;
constexpr uint8_t AAC_TEST_PCE_NO_ASSOC_DATA_ELEMENT = 0;
constexpr uint8_t AAC_TEST_PCE_NO_VALID_CC_ELEMENT = 0;
constexpr uint8_t AAC_TEST_PCE_SINGLE_CHANNEL_ELEMENT = 0;
constexpr uint8_t AAC_TEST_PCE_CHANNEL_PAIR_ELEMENT = 1;
constexpr uint8_t AAC_TEST_PCE_ZERO_VALUE = 0;
constexpr uint8_t AAC_TEST_PCE_SECONDARY_TAG = 1;
constexpr int32_t AAC_TEST_MONO_CHANNEL_COUNT = 1;
constexpr int32_t AAC_TEST_PCE_CHANNEL_COUNT = 6;
constexpr int32_t AAC_TEST_PCE_SAMPLE_RATE = 48000;
constexpr size_t AAC_TEST_TRACK_COUNT = 1;
constexpr uint8_t AAC_TEST_SYNC_EXTENSION_TYPE_BITS = 11;
constexpr uint32_t AAC_TEST_96KHZ_SAMPLE_RATE = 96000;
constexpr uint32_t AAC_TEST_96KHZ_SAMPLES_PER_FRAME = 1024;
constexpr uint32_t AAC_TEST_96KHZ_SBR_SAMPLES_PER_FRAME = 2048;
constexpr int32_t AAC_TEST_ELD_SHORT_SAMPLES_PER_FRAME = 480;
constexpr int32_t AAC_TEST_ELD_LONG_SAMPLES_PER_FRAME = 512;
// ER-AAC-ELD(39) AudioSpecificConfig with samplingFreqIndex=3 (48000),
// channelConfiguration=2 and frameLengthFlag 1/0 -> 480/512 samples per frame.
constexpr uint8_t AAC_TEST_ELD_SHORT_FRAME_CONFIG[] = {0xF8, 0xE6, 0x50};
constexpr uint8_t AAC_TEST_ELD_LONG_FRAME_CONFIG[] = {0xF8, 0xE6, 0x40};
// ASC of AVC_Main_L5.1_3735Kbps_2448x960_25.000fps_AAC_HE_AAC_LC_128Kbps_2channels_96.0_48.0KHz_earlyend.mp4:
// AOT=AAC_LC, samplingFreqIndex=3 (48000), chanConfig=2, 0x2B7 sync extension -> SBR at 96000.
constexpr uint8_t AAC_TEST_SYNC_EXTENSION_SBR_CONFIG[] = {0x11, 0x90, 0x56, 0xE5, 0x80};
constexpr uint8_t AAC_TEST_ELEMENT_CPE = 1;
constexpr uint8_t AAC_TEST_ELEMENT_SCE = 0;
constexpr uint8_t AAC_TEST_ELEMENT_CCE = 2;
constexpr uint8_t AAC_TEST_ELEMENT_LFE = 3;
constexpr uint8_t AAC_TEST_ELEMENT_DSE = 4;
constexpr uint8_t AAC_TEST_ELEMENT_PCE = 5;
constexpr uint8_t AAC_TEST_ELEMENT_FIL = 6;
constexpr uint8_t AAC_TEST_ELEMENT_END = 7;
constexpr uint8_t AAC_TEST_EXTENSION_SBR = 0x0D;
constexpr uint8_t AAC_TEST_EXTENSION_SBR_CRC = 0x0E;
constexpr uint8_t AAC_TEST_EXTENSION_OTHER = 1;
constexpr size_t AVC_PREFIX_NAL_TEST_SCAN_BUFFER_SIZE = 4 * 1024;
constexpr uint8_t AAC_TEST_ELEMENT_TYPE_BITS = 3;
constexpr uint8_t AAC_TEST_INSTANCE_TAG_BITS = 4;
constexpr uint8_t AAC_TEST_FILL_COUNT_BITS = 4;
constexpr uint8_t AAC_TEST_EXTENSION_TYPE_BITS = 4;
constexpr uint8_t AAC_TEST_FILL_COUNT = 1;
constexpr uint8_t AAC_TEST_PAYLOAD_BITS = 8;
constexpr uint8_t AAC_TEST_END_BITS = 3;
constexpr uint8_t AAC_TEST_BITS_PER_BYTE = 8;
constexpr uint8_t AAC_TEST_DSE_ALIGN_FLAG_BITS = 1;
constexpr uint8_t AAC_TEST_DSE_COUNT_BITS = 8;
constexpr uint8_t AAC_TEST_CCE_COUPLING_POINT_BITS = 1;
constexpr uint8_t AAC_TEST_CCE_NUM_COUPLED_BITS = 3;
constexpr uint8_t AAC_TEST_CCE_IS_CPE_BITS = 1;
constexpr uint8_t AAC_TEST_CCE_ID_SELECT_BITS = 4;
constexpr uint8_t AAC_TEST_CCE_CH_SELECT_BITS = 2;
constexpr uint8_t AAC_TEST_CCE_SIGN_BITS = 1;
constexpr uint8_t AAC_TEST_CCE_SCALE_BITS = 2;
constexpr uint32_t SAMPLE_INDEX_TEST_COUNT = 3;
constexpr uint32_t SAMPLE_INDEX_TEST_SECOND = 1;
constexpr uint32_t SAMPLE_INDEX_TEST_THIRD = 2;
constexpr uint32_t SAMPLE_INDEX_TEST_INVALID = SAMPLE_INDEX_TEST_COUNT;
constexpr int64_t SAMPLE_INDEX_TEST_FIRST_POS = 100;
constexpr int64_t SAMPLE_INDEX_TEST_SECOND_POS = 200;
constexpr int64_t SAMPLE_INDEX_TEST_THIRD_POS = 400;
constexpr int64_t SAMPLE_INDEX_TEST_BEFORE_FIRST_POS = 99;
constexpr int64_t SAMPLE_INDEX_TEST_AFTER_LAST_POS = 999;
constexpr int64_t SAMPLE_INDEX_TEST_FIRST_DTS = 10;
constexpr int64_t SAMPLE_INDEX_TEST_FIRST_PTS = 12;
constexpr int64_t SAMPLE_INDEX_TEST_SECOND_DTS = 20;
constexpr int64_t SAMPLE_INDEX_TEST_THIRD_DTS = 30;
constexpr int64_t SAMPLE_INDEX_TEST_THIRD_PTS = 35;
constexpr int64_t SAMPLE_INDEX_TEST_MIDDLE_DTS = 25;
constexpr int64_t SAMPLE_INDEX_TEST_AFTER_LAST_DTS = 31;
constexpr int64_t SAMPLE_INDEX_TEST_DURATION = 10;
constexpr uint32_t SAMPLE_INDEX_TEST_FIRST_SIZE = 5;
constexpr uint32_t SAMPLE_INDEX_TEST_SECOND_SIZE = 6;
constexpr uint32_t SAMPLE_INDEX_TEST_THIRD_SIZE = 7;
constexpr int32_t SPARSE_SAMPLE_TEST_SIZE = 10;
constexpr int64_t SAMPLE_INDEX_TEST_DURATION_OVERRIDE = 8;
constexpr uint32_t DCA3_CONFIG_SIZE = 6;
constexpr uint32_t CODEC_ID_BITS = 4;
constexpr uint32_t SAMPLING_INDEX_BITS = 4;
constexpr uint32_t NN_TYPE_BITS = 3;
constexpr uint32_t RESERVED_BITS = 1;
constexpr uint32_t CONTENT_TYPE_BITS = 4;
constexpr uint32_t CHANNEL_INDEX_BITS = 7;
constexpr uint32_t BITRATE_BITS = 16;
constexpr uint32_t RESOLUTION_BITS = 2;
constexpr uint32_t LOSSY_CODEC_ID = 2;
constexpr uint32_t CHANNEL_BASED_CONTENT = 0;
constexpr uint32_t BASELINE_NN_TYPE = 0;
constexpr uint32_t TEST_BITRATE_KBPS = 128;
constexpr uint32_t TEST_RESOLUTION_INDEX = 1;
constexpr uint32_t RAP_GROUP_ENTRY_COUNT = 1;
constexpr uint32_t RAP_GROUP_SAMPLE_COUNT = 1;
constexpr uint32_t RAP_GROUP_NON_KEY_INDEX = 0;
constexpr uint32_t PARTIAL_SYNC_SAMPLE = 61;
constexpr uint32_t MPEG_AUDIO_TEST_TRACK_ID = 0;
constexpr int32_t MPEG_AUDIO_TEST_SAMPLE_RATE = 16000;
constexpr int32_t MPEG_AUDIO_TEST_SAMPLES_PER_FRAME = 576;
constexpr size_t MPEG_AUDIO_TEST_FRAME_SIZE = 504;
constexpr size_t MPEG_AUDIO_TEST_SPLIT_SIZE = 200;
constexpr size_t MPEG_AUDIO_TEST_HEADER_SPLIT_SIZE = 2;
constexpr size_t MPEG_AUDIO_TEST_PARTIAL_FRAME_SIZE = 298;
constexpr size_t MPEG_AUDIO_TEST_FOLLOWING_PREFIX_SIZE = 249;
constexpr size_t MPEG_AUDIO_TEST_PARTIAL_FRAME_TAIL_SIZE =
    MPEG_AUDIO_TEST_FRAME_SIZE - MPEG_AUDIO_TEST_PARTIAL_FRAME_SIZE;
constexpr size_t MPEG_AUDIO_TEST_FOLLOWING_FRAME_TAIL_SIZE =
    MPEG_AUDIO_TEST_FRAME_SIZE - MPEG_AUDIO_TEST_FOLLOWING_PREFIX_SIZE;
constexpr size_t MPEG_AUDIO_TEST_MIXED_SAMPLE_SIZE =
    MPEG_AUDIO_TEST_PARTIAL_FRAME_TAIL_SIZE + MPEG_AUDIO_TEST_FOLLOWING_PREFIX_SIZE;
constexpr size_t MPEG_AUDIO_TEST_PENDING_LIMIT = 64 * 1024;
constexpr size_t MPEG_AUDIO_TEST_INVALID_SAMPLE_SIZE = 1024;
constexpr size_t MPEG_AUDIO_TEST_SEGMENT_COALESCE_COUNT = 129;
constexpr uint8_t MPEG_AUDIO_TEST_HEADER[] = {0xFF, 0xF3, 0xB8, 0x44};
constexpr int64_t MPEG_AUDIO_TEST_FRAME_DURATION_US = 36000;

void AppendAacTestBits(std::vector<uint8_t>& bits, uint32_t value, uint8_t bitCount)
{
    for (int32_t shift = static_cast<int32_t>(bitCount) - 1; shift >= 0; --shift) {
        bits.push_back(static_cast<uint8_t>((value >> shift) & 0x01));
    }
}

std::vector<uint8_t> BuildAacRawDataBlock(uint8_t extensionType, bool truncated = false)
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_CPE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_FIL, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, truncated ? AAC_TEST_FILL_COUNT + 1 : AAC_TEST_FILL_COUNT,
        AAC_TEST_FILL_COUNT_BITS);
    AppendAacTestBits(bits, extensionType, AAC_TEST_EXTENSION_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_PAYLOAD_BITS - AAC_TEST_EXTENSION_TYPE_BITS);
    if (!truncated) {
        AppendAacTestBits(bits, AAC_TEST_ELEMENT_END, AAC_TEST_END_BITS);
    }
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    std::vector<uint8_t> data(bits.size() / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        data[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    return data;
}

std::vector<uint8_t> BuildAacSceRawDataBlock()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_SCE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_END, AAC_TEST_END_BITS);
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    std::vector<uint8_t> data(bits.size() / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        data[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    return data;
}

std::vector<uint8_t> BuildAacAudioSpecificConfig(bool sbrPresent)
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_AUDIO_OBJECT_TYPE_AAC_LC, AAC_TEST_AUDIO_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS); // 96 kHz sampling frequency index
    AppendAacTestBits(bits, AAC_TEST_CHANNEL_COUNT, AAC_TEST_CHANNEL_CONFIGURATION_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_GA_SPECIFIC_CONFIG_FLAGS_BITS); // GASpecificConfig flags
    AppendAacTestBits(bits, AAC_TEST_ASC_SYNC_EXTENSION, AAC_TEST_SYNC_EXTENSION_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_AUDIO_OBJECT_TYPE_SBR, AAC_TEST_AUDIO_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, sbrPresent ? 1 : 0, 1);
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    std::vector<uint8_t> data(bits.size() / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        data[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    return data;
}

void AppendAacPceChannelElement(std::vector<uint8_t>& bits, uint8_t elementType, uint8_t elementTag)
{
    AppendAacTestBits(bits, elementType, AAC_TEST_PCE_ELEMENT_IS_CPE_BITS);
    AppendAacTestBits(bits, elementTag, AAC_TEST_PCE_ELEMENT_TAG_BITS);
}

std::vector<uint8_t> BuildAacPceSpecificConfig(bool truncated)
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_AUDIO_OBJECT_TYPE_AAC_LC, AAC_TEST_AUDIO_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_48KHZ_SAMPLE_RATE_INDEX, AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_CHANNEL_CONFIGURATION, AAC_TEST_CHANNEL_CONFIGURATION_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_GA_SPECIFIC_CONFIG_FLAGS_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_AAC_LC_OBJECT_TYPE, AAC_TEST_PCE_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_48KHZ_SAMPLE_RATE_INDEX, AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_FRONT_ELEMENT_COUNT, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_SIDE_ELEMENT_COUNT, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_BACK_ELEMENT_COUNT, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_NO_LFE_ELEMENT, AAC_TEST_PCE_LFE_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_NO_ASSOC_DATA_ELEMENT, AAC_TEST_PCE_ASSOC_DATA_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_NO_VALID_CC_ELEMENT, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacPceChannelElement(bits, AAC_TEST_PCE_CHANNEL_PAIR_ELEMENT, AAC_TEST_PCE_ZERO_VALUE);
    AppendAacPceChannelElement(bits, AAC_TEST_PCE_SINGLE_CHANNEL_ELEMENT, AAC_TEST_PCE_ZERO_VALUE);
    AppendAacPceChannelElement(bits, AAC_TEST_PCE_CHANNEL_PAIR_ELEMENT, AAC_TEST_PCE_SECONDARY_TAG);
    AppendAacPceChannelElement(bits, AAC_TEST_PCE_SINGLE_CHANNEL_ELEMENT, AAC_TEST_PCE_SECONDARY_TAG);
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_COMMENT_COUNT_BITS);
    std::vector<uint8_t> config(bits.size() / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        config[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    if (truncated) {
        config.pop_back();
    }
    return config;
}

std::vector<uint8_t> BuildAacPceConfigWithElements(uint8_t frontCount, uint8_t sideCount, uint8_t backCount,
    const std::vector<uint8_t>& frontElements, const std::vector<uint8_t>& sideElements,
    const std::vector<uint8_t>& backElements)
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_AUDIO_OBJECT_TYPE_AAC_LC, AAC_TEST_AUDIO_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_48KHZ_SAMPLE_RATE_INDEX, AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_CHANNEL_CONFIGURATION, AAC_TEST_CHANNEL_CONFIGURATION_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_GA_SPECIFIC_CONFIG_FLAGS_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_AAC_LC_OBJECT_TYPE, AAC_TEST_PCE_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_48KHZ_SAMPLE_RATE_INDEX, AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS);
    AppendAacTestBits(bits, frontCount, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, sideCount, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, backCount, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_NO_LFE_ELEMENT, AAC_TEST_PCE_LFE_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_NO_ASSOC_DATA_ELEMENT, AAC_TEST_PCE_ASSOC_DATA_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_NO_VALID_CC_ELEMENT, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    for (uint8_t isCpe : frontElements) {
        AppendAacPceChannelElement(bits, isCpe, AAC_TEST_PCE_ZERO_VALUE);
    }
    for (uint8_t isCpe : sideElements) {
        AppendAacPceChannelElement(bits, isCpe, AAC_TEST_PCE_ZERO_VALUE);
    }
    for (uint8_t isCpe : backElements) {
        AppendAacPceChannelElement(bits, isCpe, AAC_TEST_PCE_ZERO_VALUE);
    }
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_COMMENT_COUNT_BITS);
    std::vector<uint8_t> config(bits.size() / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        config[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    return config;
}

std::vector<uint8_t> BuildAacLfeRawDataBlock()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_LFE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_END, AAC_TEST_END_BITS);
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    std::vector<uint8_t> data(bits.size() / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        data[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    return data;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareAacConfigTestParser(
    Plugins::MPEG4::MPEG4AtomParser& parser)
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    if (track == nullptr) {
        return nullptr;
    }
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track->sampleHelper == nullptr) {
        return nullptr;
    }
    parser.lastTrack_ = track;
    parser.mediaInfo_.tracks.resize(AAC_TEST_TRACK_COUNT);
    return track;
}

MediaInfo MakeAacFrameMediaInfo()
{
    MediaInfo mediaInfo;
    mediaInfo.tracks.resize(1);
    mediaInfo.tracks[0].Set<Tag::AUDIO_SAMPLE_RATE>(AAC_TEST_CORE_SAMPLE_RATE);
    mediaInfo.tracks[0].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(AAC_TEST_CORE_SAMPLES_PER_FRAME);
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(AAC_TEST_CHANNEL_COUNT);
    return mediaInfo;
}
std::unique_ptr<FileServerDemo> g_server = nullptr;

Plugins::MPEG4::MPEG4SampleHelper::SampleIndexEntry MakeSampleIndexEntry(
    int64_t pos, int64_t dts, int64_t pts, uint32_t size, bool discard = false)
{
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexEntry entry;
    entry.pos = pos;
    entry.dts = dts;
    EXPECT_TRUE(entry.SetPts(pts));
    EXPECT_TRUE(entry.SetSize(size));
    entry.SetDiscard(discard);
    return entry;
}

void PrepareSparseSampleHelper(Plugins::MPEG4::MPEG4SampleHelper& helper, uint32_t sampleCount)
{
    helper.trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper.mimeType_ = Plugins::MimeType::VIDEO_AVC;
    helper.sttsEntry_ = {{sampleCount, 10}};
    helper.stscEntry_ = {{0, sampleCount, 1}};
    helper.stssEntry_ = {{0}, {3}, {sampleCount - 1}};
    helper.sampleSizes_.assign(sampleCount, SPARSE_SAMPLE_TEST_SIZE);
    helper.sampleSizeCount_ = sampleCount;
    helper.chunkOffsets_ = {100};
    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
}

constexpr uint32_t AUDIO_EDIT_TEST_SAMPLE_COUNT = 3;
constexpr uint32_t AUDIO_EDIT_TEST_SAMPLE_DURATION = 1024;
constexpr uint32_t AUDIO_EDIT_TEST_DURATION = 15604;
constexpr int32_t AUDIO_EDIT_TEST_TIME_SCALE = 44100;
constexpr uint32_t AUDIO_EDIT_TEST_STRADDLE_MEDIA_TIME = 495;
constexpr uint32_t AUDIO_EDIT_TEST_BOUNDARY_MEDIA_TIME = 1024;
constexpr uint32_t AUDIO_EDIT_TEST_OUTSIDE_MEDIA_TIME = 2048;

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> BuildAudioEditTrack(uint32_t mediaTime)
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    if (track == nullptr) {
        return nullptr;
    }
    track->codecParms.trackType = Plugins::MPEG4::AUDIO_TYPE;
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track->sampleHelper == nullptr) {
        return nullptr;
    }
    auto& helper = *track->sampleHelper;
    helper.trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper.mimeType_ = MimeType::AUDIO_AAC;
    helper.sttsEntry_ = {{AUDIO_EDIT_TEST_SAMPLE_COUNT, AUDIO_EDIT_TEST_SAMPLE_DURATION}};
    helper.stscEntry_ = {{0, AUDIO_EDIT_TEST_SAMPLE_COUNT, 1}};
    helper.sampleSizes_.assign(AUDIO_EDIT_TEST_SAMPLE_COUNT, 1);
    helper.sampleSizeCount_ = AUDIO_EDIT_TEST_SAMPLE_COUNT;
    helper.chunkOffsets_ = {0};
    helper.SetEditListParams({{mediaTime, AUDIO_EDIT_TEST_DURATION, 1, 0}},
        AUDIO_EDIT_TEST_TIME_SCALE, AUDIO_EDIT_TEST_TIME_SCALE);
    if (helper.FinalizeSampleTables() != Status::OK || helper.EnsureDenseIndex() != Status::OK ||
        !helper.HasAppliedEditList()) {
        return nullptr;
    }
    return track;
}

void VerifySampleIndexProperties(Plugins::MPEG4::MPEG4SampleHelper& helper)
{
    EXPECT_EQ(helper.GetSampleCount(), SAMPLE_INDEX_TEST_COUNT);
    EXPECT_FALSE(helper.IsSampleIndexEmpty());
    EXPECT_TRUE(helper.IsSyncSample(0));
    EXPECT_FALSE(helper.IsSyncSample(SAMPLE_INDEX_TEST_SECOND));
    EXPECT_TRUE(helper.IsSyncSample(SAMPLE_INDEX_TEST_THIRD));
    EXPECT_TRUE(helper.IsDiscardSample(SAMPLE_INDEX_TEST_SECOND));
    EXPECT_FALSE(helper.IsDiscardSample(SAMPLE_INDEX_TEST_INVALID));

    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(helper.GetSampleIndexView(SAMPLE_INDEX_TEST_SECOND, view), Status::OK);
    EXPECT_EQ(view.pos, SAMPLE_INDEX_TEST_SECOND_POS);
    EXPECT_EQ(view.dts, SAMPLE_INDEX_TEST_SECOND_DTS);
    EXPECT_EQ(view.pts, SAMPLE_INDEX_TEST_SECOND_DTS);
    EXPECT_EQ(view.size, SAMPLE_INDEX_TEST_SECOND_SIZE);
    EXPECT_EQ(view.duration, SAMPLE_INDEX_TEST_DURATION);
    EXPECT_EQ(view.flag, Plugins::MPEG4::Sample::SampleFlag::DISCARD);
}

void VerifySampleIndexAccessors(Plugins::MPEG4::MPEG4SampleHelper& helper)
{
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t pos = 0;
    int32_t size = 0;
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    EXPECT_EQ(helper.GetSamplePts(SAMPLE_INDEX_TEST_THIRD, pts), Status::OK);
    EXPECT_EQ(pts, SAMPLE_INDEX_TEST_THIRD_PTS);
    EXPECT_EQ(helper.GetSampleDts(SAMPLE_INDEX_TEST_THIRD, dts), Status::OK);
    EXPECT_EQ(dts, SAMPLE_INDEX_TEST_THIRD_DTS);
    EXPECT_EQ(helper.GetSampleRange(SAMPLE_INDEX_TEST_THIRD, pos, size), Status::OK);
    EXPECT_EQ(pos, SAMPLE_INDEX_TEST_THIRD_POS);
    EXPECT_EQ(size, SAMPLE_INDEX_TEST_THIRD_SIZE);
    EXPECT_EQ(helper.GetSampleIndexView(SAMPLE_INDEX_TEST_INVALID, view), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(helper.GetSampleRange(SAMPLE_INDEX_TEST_INVALID, pos, size), Status::ERROR_INVALID_PARAMETER);
}

void VerifySampleIndexLookup(Plugins::MPEG4::MPEG4SampleHelper& helper)
{
    uint32_t index = 0;
    EXPECT_EQ(helper.FindSampleIndexByPosition(SAMPLE_INDEX_TEST_BEFORE_FIRST_POS, index), Status::OK);
    EXPECT_EQ(index, 0U);
    EXPECT_EQ(helper.FindSampleIndexByPosition(SAMPLE_INDEX_TEST_SECOND_POS, index), Status::OK);
    EXPECT_EQ(index, SAMPLE_INDEX_TEST_SECOND);
    EXPECT_EQ(helper.FindSampleIndexByPosition(SAMPLE_INDEX_TEST_AFTER_LAST_POS, index), Status::OK);
    EXPECT_EQ(index, SAMPLE_INDEX_TEST_THIRD);
    EXPECT_EQ(helper.FindFirstSampleAfterPosition(SAMPLE_INDEX_TEST_SECOND_POS, index), Status::OK);
    EXPECT_EQ(index, SAMPLE_INDEX_TEST_THIRD);
    EXPECT_EQ(helper.FindFirstSampleAfterPosition(SAMPLE_INDEX_TEST_THIRD_POS, index), Status::OK);
    EXPECT_EQ(index, SAMPLE_INDEX_TEST_INVALID);
    EXPECT_EQ(helper.FindFirstSampleByDts(SAMPLE_INDEX_TEST_SECOND_DTS, index), Status::OK);
    EXPECT_EQ(index, SAMPLE_INDEX_TEST_SECOND);
    EXPECT_EQ(helper.FindFirstSampleByDts(SAMPLE_INDEX_TEST_MIDDLE_DTS, index), Status::OK);
    EXPECT_EQ(index, SAMPLE_INDEX_TEST_THIRD);
    EXPECT_EQ(helper.FindFirstSampleByDts(SAMPLE_INDEX_TEST_AFTER_LAST_DTS, index), Status::END_OF_STREAM);
}

static const std::vector<std::string> g_supportFiles = {
    "2obj_16bit_44100Hz_64kbps.mp4",
    "2obj_44100Hz_16bit_32k.m4a",
    "2obj_44100Hz_16bit_32k.mp4",
    "5_1_4_4obj_16bit_48000Hz_1232kbps.mp4",
    "5_1_4_16bit_32000Hz_704kbps.mp4",
    "5_1_4_16bit_96000Hz_704kbps.mp4",
    "5_1_4_24bit_48000Hz_704kbps.mp4",
    "7_1_4_16bit_48000Hz_832kbps.mp4",
    "aac_hdrvivid.mp4",
    "aigc_str.m4a",
    "aigc_str.mp4",
    "avc_aac.mp4",
    "avcc_10sec.mp4",
    "avcc_aac_mp3.mp4",
    "camera.mp4",
    "camera_h265_aac_rotate270.mp4",
    "camera_info_parser.mp4",
    "comment_0010.mp4",
    "comment_0020.mp4",
    "comment_0030.mp4",
    "comment_0040.mp4",
    "comment_0050.mp4",
    "comment_0060.mp4",
    "demuxer_parser_2_layer_frame_avc.mp4",
    "demuxer_parser_2_layer_frame_hevc.mp4",
    "demuxer_parser_3_layer_frame_avc.mp4",
    "demuxer_parser_3_layer_frame_hevc.mp4",
    "demuxer_parser_4_layer_frame_avc.mp4",
    "demuxer_parser_4_layer_frame_hevc.mp4",
    "demuxer_parser_all_i_frame_avc.mp4",
    "demuxer_parser_all_i_frame_hevc.mp4",
    "demuxer_parser_hdr_1_hevc.mp4",
    "demuxer_parser_hdr_2_hevc.mp4",
    "demuxer_parser_hdr_vivid.mp4",
    "demuxer_parser_hdr_vivid_gltf.mp4",
    "demuxer_parser_ipb_frame_avc.mp4",
    "demuxer_parser_ipb_frame_hevc.mp4",
    "demuxer_parser_ltr_frame_avc.mp4",
    "demuxer_parser_ltr_frame_hevc.mp4",
    "demuxer_parser_one_i_frame_avc.mp4",
    "demuxer_parser_one_i_frame_hevc.mp4",
    "demuxer_parser_one_i_frame_no_audio_avc.mp4",
    "demuxer_parser_one_i_frame_no_audio_hevc.mp4",
    "demuxer_parser_recording.mp4",
    "demuxer_parser_sdtp_extended_hevc.mp4",
    "demuxer_parser_sdtp_frame_avc.mp4",
    "demuxer_parser_sdtp_frame_hevc.mp4",
    "double_hevc.mp4",
    "double_vivid.mp4",
    "FLIP_V_ut.mp4",
    "FLIP_V_90_ut.mp4",
    "gb2312.m4a",
    "gb2312.mp4",
    "gb18030.m4a",
    "gb18030.mp4",
    "gbk.m4a",
    "gbk.mp4",
    "h264.mp4",
    "h264_aac_ch5.1.mp4",
    "H264_AAC_multi_track.mp4",
    "h264_double_video_audio.mp4",
    "h265_mp3_gltf.mp4",
    "hdrType_hdr10.mp4",
    "hdrType_hdrVivid.mp4",
    "hdrType_hlg.mp4",
    "hdrvivid_720p_2s.mp4",
    "hevc_aac.mp4",
    "hlg.mp4",
    "hlgHdrVivid.mp4",
    "hoa3_16bit_48000Hz_640kbps.mp4",
    "hvcc.mp4",
    "large_tref.mp4",
    "loci_altitude_100.mp4",
    "loci_altitude_102.mp4",
    "loci_altitude_empty.mp4",
    "M4A_24K_2.m4a",
    "M4A_64K_2_main.m4a",
    "M4A_96K_1_cbr.m4a",
    "M4A_48000_1.m4a",
    "m4a_48000_1_ut.m4a",
    "M4A_48000_aigc.m4a",
    "m4a_m4a.m4a",
    "m4v_h264_noaudio.m4v",
    "m4v_hevc_aac.m4v",
    "metaAlltype.mp4",
    "metaFloatval.mp4",
    "metaIntval.mp4",
    "metaMaxval.mp4",
    "metaStringval.mp4",
    "metaval_01.m4a",
    "metaval_01.mp4",
    "metaval_03.m4a",
    "metaval_03.mp4",
    "metaval_04.m4a",
    "metaval_04.mp4",
    "metaval_05.m4a",
    "metaval_05.mp4",
    "mp3_h264.mp4",
    "mp3_h265.mp4",
    "mp3_h265_fmp4.mp4",
    "Muxer_Auxiliary.mp4",
    "Muxer_Auxiliary_02.mp4",
    "Muxer_Auxiliary_03.mp4",
    "Muxer_Auxiliary_04.mp4",
    "muxer_auxl_265.mp4",
    "muxer_auxl_265_264_aac.mp4",
    "Muxer_SetFormat_Comment_1000.mp4",
    "Muxer_SetFormat_Comment_1100.mp4",
    "Muxer_SetFormat_Comment_1200.mp4",
    "Muxer_SetFormat_Comment_1300.mp4",
    "noframe.mp4",
    "NoTimedmetadataAudio.mp4",
    "pqHdrvivid.mp4",
    "pre_record_parser.mp4",
    "ROTATE_270_ut.mp4",
    "ROTATE_NONE_ut.mp4",
    "sarnot1.mp4",
    "short_audio.mp4",
    "short_video.mp4",
    "single_60.mp4",
    "single_rk.mp4",
    "stereo_16bit_48000Hz_32kbps.mp4",
    "test_264_B_Gop25_4sec.mp4",
    "test_264_B_Gop25_4sec_cover.mp4",
    "test_264_B_Gop25_4sec_gltf.mp4",
    "test_265_B_Gop25_4sec.mp4",
    "test_h264.mp4",
    "test_starttime.mp4",
    "timed_metadata_track.mp4",
    "Timedmetadata1Track0.mp4",
    "Timedmetadata1Track1.mp4",
    "Timedmetadata1Track2.mp4",
    "Timedmetadata2Track2.mp4",
    "TimedmetadataAudio.mp4",
    "TimedmetadataVideo.mp4",
    "unsupport_type.mp4",
    "video_2audio.mp4",
    "video_9audio.mp4",
    "video_audio_one_frame_test.mp4",
    "vvc_1280_720_8.mp4",
    "vvc_1280_720_8_gltf.mp4",
    "webvtt.mp4",
    "wvtt.mp4",
    "xm.mp4",
    "zero_track.mp4",
};

std::vector<SeekExpectedResult> GetExplicitSeekExpectedResults()
{
    return {
        {0, SeekMode::SEEK_NEXT_SYNC, true, {103, 174}},
        {0, SeekMode::SEEK_PREVIOUS_SYNC, true, {103, 174}},
        {0, SeekMode::SEEK_CLOSEST_SYNC, true, {103, 174}},
        {2000, SeekMode::SEEK_NEXT_SYNC, true, {53, 90}},
        {2000, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2000, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {1920, SeekMode::SEEK_NEXT_SYNC, true, {53, 90}},
        {1920, SeekMode::SEEK_PREVIOUS_SYNC, true, {78, 134}},
        {1920, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 90}},
        {2160, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2160, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2160, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2200, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2200, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2200, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2440, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2440, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2440, SeekMode::SEEK_CLOSEST_SYNC, true, {53, 91}},
        {2600, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2600, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2600, SeekMode::SEEK_CLOSEST_SYNC, true, {28, 47}},
        {2700, SeekMode::SEEK_NEXT_SYNC, true, {28, 47}},
        {2700, SeekMode::SEEK_PREVIOUS_SYNC, true, {53, 91}},
        {2700, SeekMode::SEEK_CLOSEST_SYNC, true, {28, 47}},
        {4080, SeekMode::SEEK_NEXT_SYNC, false, {}},
        {4080, SeekMode::SEEK_PREVIOUS_SYNC, true, {3, 5}},
        {4080, SeekMode::SEEK_CLOSEST_SYNC, true, {3, 5}},
        {4100, SeekMode::SEEK_NEXT_SYNC, false, {}},
        {4100, SeekMode::SEEK_PREVIOUS_SYNC, true, {3, 5}},
        {4100, SeekMode::SEEK_CLOSEST_SYNC, true, {3, 5}},
    };
}

struct MetaDumpItem {
    TagType tag;
    std::string name;
};

struct SetDataSourceRunResult {
    Status ret = Status::ERROR_UNKNOWN;
    Status mediaInfoRet = Status::ERROR_UNKNOWN;
    bool hasMediaInfo = false;
    MediaInfo mediaInfo;
};

struct FixedSeekSampleExpected {
    int64_t seekTimeMs = 0;
    SeekMode seekMode = SeekMode::SEEK_PREVIOUS_SYNC;
    int64_t realSeekTimeNs = 0;
    int64_t ptsUs = 0;
    int64_t dtsUs = 0;
    int64_t durationUs = 0;
};

struct KnownSetDataSourceDiffResource {
    std::string filePath;
    std::vector<std::string> issueFields;
};

class ShortReadDataSource : public DataSource {
public:
    ShortReadDataSource(std::vector<uint8_t> data, size_t maxReadSize)
        : data_(std::move(data)), maxReadSize_(maxReadSize)
    {
    }

    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override
    {
        readOffsets.emplace_back(offset);
        readExpectedLens.emplace_back(expectedLen);
        if (offset < 0 || buffer == nullptr || buffer->GetMemory() == nullptr ||
            static_cast<size_t>(offset) >= data_.size()) {
            return Status::END_OF_STREAM;
        }
        size_t available = data_.size() - static_cast<size_t>(offset);
        size_t copySize = std::min({expectedLen, maxReadSize_, available});
        size_t writeSize = buffer->GetMemory()->Write(data_.data() + offset, copySize, 0);
        return writeSize > 0 ? Status::OK : Status::ERROR_UNKNOWN;
    }

    Status GetSize(uint64_t& size) override
    {
        size = data_.size();
        return Status::OK;
    }

    Seekable GetSeekable() override
    {
        return Seekable::SEEKABLE;
    }

    int32_t GetStreamID() override
    {
        return 0;
    }

    bool IsDash() override
    {
        return false;
    }

    std::vector<int64_t> readOffsets;
    std::vector<size_t> readExpectedLens;

private:
    std::vector<uint8_t> data_;
    size_t maxReadSize_ = 0;
};

class BlockingReadAtDataSource final : public DataSource {
public:
    explicit BlockingReadAtDataSource(std::vector<uint8_t> data) : data_(std::move(data)) {}

    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override
    {
        {
            std::unique_lock<std::mutex> lock(mutex_);
            if (armed_ && !blocked_) {
                blocked_ = true;
                condition_.notify_all();
                condition_.wait(lock, [this] { return released_; });
                armed_ = false;
            }
        }
        if (offset < 0 || buffer == nullptr || buffer->GetMemory() == nullptr ||
            static_cast<size_t>(offset) >= data_.size()) {
            return Status::END_OF_STREAM;
        }
        size_t copySize = std::min(expectedLen, data_.size() - static_cast<size_t>(offset));
        size_t writeSize = buffer->GetMemory()->Write(data_.data() + offset, copySize, 0);
        return writeSize > 0 ? Status::OK : Status::ERROR_UNKNOWN;
    }

    Status GetSize(uint64_t& size) override
    {
        size = data_.size();
        return Status::OK;
    }

    Seekable GetSeekable() override
    {
        return Seekable::SEEKABLE;
    }

    int32_t GetStreamID() override
    {
        return 0;
    }

    bool IsDash() override
    {
        return false;
    }

    void Arm()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        blocked_ = false;
        released_ = false;
        armed_ = true;
    }

    bool WaitUntilBlocked()
    {
        std::unique_lock<std::mutex> lock(mutex_);
        return condition_.wait_for(lock, std::chrono::seconds(IN_FLIGHT_READ_BLOCK_TIMEOUT_SECONDS),
            [this] { return blocked_; });
    }

    void Release()
    {
        std::lock_guard<std::mutex> lock(mutex_);
        released_ = true;
        condition_.notify_all();
    }

private:
    std::vector<uint8_t> data_;
    std::mutex mutex_;
    std::condition_variable condition_;
    bool armed_ = false;
    bool blocked_ = false;
    bool released_ = false;
};

std::vector<uint8_t> ReadFileBytes(const std::string& filePath)
{
    struct stat fileStatus {};
    if (stat(filePath.c_str(), &fileStatus) != 0 || fileStatus.st_size <= 0) {
        return {};
    }
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        return {};
    }
    std::vector<uint8_t> data(static_cast<size_t>(fileStatus.st_size));
    size_t bytesRead = 0;
    while (bytesRead < data.size()) {
        ssize_t ret = read(fd, data.data() + bytesRead, data.size() - bytesRead);
        if (ret <= 0) {
            data.clear();
            break;
        }
        bytesRead += static_cast<size_t>(ret);
    }
    close(fd);
    return data;
}

class UnknownSizeDataSource final : public ShortReadDataSource {
public:
    using ShortReadDataSource::ShortReadDataSource;

    Status GetSize(uint64_t& size) override
    {
        size = 0;
        return Status::ERROR_UNKNOWN;
    }
};

class SequentialReadDataSource final : public DataSource {
public:
    explicit SequentialReadDataSource(std::vector<uint8_t> data) : data_(std::move(data)) {}

    Status ReadAt(int64_t offset, std::shared_ptr<Buffer>& buffer, size_t expectedLen) override
    {
        requestedOffsets.emplace_back(offset);
        readSizes.emplace_back(expectedLen);
        actualOffsets.emplace_back(cursor_);
        if (buffer == nullptr || buffer->GetMemory() == nullptr || cursor_ >= data_.size()) {
            return Status::END_OF_STREAM;
        }
        size_t readSize = std::min(expectedLen, data_.size() - cursor_);
        size_t written = buffer->GetMemory()->Write(data_.data() + cursor_, readSize, 0);
        cursor_ += written;
        return written > 0 ? Status::OK : Status::ERROR_UNKNOWN;
    }

    Status GetSize(uint64_t& size) override
    {
        size = data_.size();
        return Status::OK;
    }

    Seekable GetSeekable() override
    {
        return Seekable::UNSEEKABLE;
    }

    int32_t GetStreamID() override
    {
        return 0;
    }

    bool IsDash() override
    {
        return false;
    }

    size_t GetCursor() const
    {
        return cursor_;
    }

    std::vector<int64_t> requestedOffsets;
    std::vector<size_t> readSizes;
    std::vector<size_t> actualOffsets;

private:
    std::vector<uint8_t> data_;
    size_t cursor_ = 0;
};

void AppendU32(std::vector<uint8_t>& data, uint32_t value);

std::vector<uint8_t> BuildFtypWithLateIsomBrand(size_t brandCount, size_t isomBrandIndex)
{
    constexpr uint32_t ftypAtom = 0x66747970;
    constexpr uint32_t unknownBrand = 0x7A7A7A7A;
    constexpr uint32_t isomBrand = 0x69736D6F;
    constexpr size_t ftypAtomMinimumSize = 16;
    std::vector<uint8_t> data;
    AppendU32(data, static_cast<uint32_t>(ftypAtomMinimumSize + brandCount * sizeof(uint32_t)));
    AppendU32(data, ftypAtom);
    AppendU32(data, unknownBrand);
    AppendU32(data, 0);
    for (size_t index = 0; index < brandCount; ++index) {
        AppendU32(data, index == isomBrandIndex ? isomBrand : unknownBrand);
    }
    return data;
}

Status ReadFromSequentialSourceWithPrefixCache(const std::shared_ptr<SequentialReadDataSource>& source,
    const std::shared_ptr<std::vector<uint8_t>>& prefixCache, uint64_t offset, size_t size,
    std::shared_ptr<Buffer>& buffer)
{
    auto outputMemory = buffer == nullptr ? nullptr : buffer->GetMemory();
    if (outputMemory == nullptr) {
        return Status::ERROR_INVALID_PARAMETER;
    }
    if (prefixCache->empty()) {
        Status ret = source->ReadAt(static_cast<int64_t>(offset), buffer, size);
        if (ret == Status::OK) {
            prefixCache->assign(outputMemory->GetReadOnlyData(),
                outputMemory->GetReadOnlyData() + outputMemory->GetSize());
        }
        return ret;
    }
    if (offset >= prefixCache->size()) {
        return source->ReadAt(static_cast<int64_t>(offset), buffer, size);
    }
    size_t cacheOffset = static_cast<size_t>(offset);
    size_t cachedSize = std::min(size, prefixCache->size() - cacheOffset);
    outputMemory->Write(prefixCache->data() + cacheOffset, cachedSize, 0);
    if (cachedSize == size) {
        return Status::OK;
    }
    auto extension = Buffer::CreateDefaultBuffer(size - cachedSize);
    if (extension == nullptr || extension->GetMemory() == nullptr) {
        return Status::ERROR_NO_MEMORY;
    }
    Status ret = source->ReadAt(static_cast<int64_t>(prefixCache->size()), extension, size - cachedSize);
    if (ret != Status::OK) {
        return ret;
    }
    auto extensionMemory = extension->GetMemory();
    prefixCache->insert(prefixCache->end(), extensionMemory->GetReadOnlyData(),
        extensionMemory->GetReadOnlyData() + extensionMemory->GetSize());
    outputMemory->Write(extensionMemory->GetReadOnlyData(), extensionMemory->GetSize(), cachedSize);
    return Status::OK;
}

PluginSnifferFunc GetDemuxerSniffer(const std::string& pluginName)
{
    auto plugins = PluginList::GetInstance().GetPluginsByType(PluginType::DEMUXER);
    auto plugin = std::find_if(plugins.begin(), plugins.end(), [&pluginName](const PluginDescription& description) {
        return description.pluginName == pluginName;
    });
    if (plugin == plugins.end()) {
        return nullptr;
    }
    auto pluginDef = PluginManagerV2::Instance().cachedPluginPackage_->GetPluginDef(*plugin);
    return pluginDef == nullptr ? nullptr : pluginDef->GetSniffer();
}

void AppendU32(std::vector<uint8_t>& data, uint32_t value)
{
    data.emplace_back(static_cast<uint8_t>((value >> 24) & 0xFF));
    data.emplace_back(static_cast<uint8_t>((value >> 16) & 0xFF));
    data.emplace_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    data.emplace_back(static_cast<uint8_t>(value & 0xFF));
}

void AppendU16(std::vector<uint8_t>& data, uint16_t value)
{
    data.emplace_back(static_cast<uint8_t>((value >> 8) & 0xFF));
    data.emplace_back(static_cast<uint8_t>(value & 0xFF));
}

void AppendU64(std::vector<uint8_t>& data, uint64_t value)
{
    constexpr int32_t byteBitWidth = 8;
    constexpr int32_t maxUint64Shift = 56;
    for (int32_t shift = maxUint64Shift; shift >= 0; shift -= byteBitWidth) {
        data.emplace_back(static_cast<uint8_t>((value >> shift) & 0xFF));
    }
}

std::vector<uint8_t> BuildAtomHeader(uint64_t payloadSize, uint32_t atomType, bool isExtendedSize)
{
    constexpr uint64_t normalAtomHeaderSize = 8;
    constexpr uint64_t extendedAtomHeaderSize = 16;
    const uint64_t headerSize = isExtendedSize ? extendedAtomHeaderSize : normalAtomHeaderSize;
    const uint64_t atomSize = payloadSize + headerSize;
    std::vector<uint8_t> data;
    if (isExtendedSize) {
        AppendU32(data, 1);
        AppendU32(data, atomType);
        AppendU64(data, atomSize);
    } else {
        AppendU32(data, static_cast<uint32_t>(atomSize));
        AppendU32(data, atomType);
    }
    return data;
}

std::vector<uint8_t> BuildTraditionalMetadataFile(uint32_t atomType, const std::vector<uint8_t>& value)
{
    EXPECT_LE(value.size(), std::numeric_limits<uint16_t>::max());
    std::vector<uint8_t> payload;
    payload.emplace_back(static_cast<uint8_t>(value.size() >> TRADITIONAL_METADATA_LENGTH_SHIFT));
    payload.emplace_back(static_cast<uint8_t>(value.size()));
    payload.insert(payload.end(), {0, 0}); // language
    payload.insert(payload.end(), value.begin(), value.end());
    auto metadata = BuildAtomHeader(payload.size(), atomType, false);
    metadata.insert(metadata.end(), payload.begin(), payload.end());
    auto udta = BuildAtomHeader(metadata.size(), Plugins::MPEG4::FourccType("udta"), false);
    udta.insert(udta.end(), metadata.begin(), metadata.end());
    auto moov = BuildAtomHeader(udta.size(), Plugins::MPEG4::FourccType("moov"), false);
    moov.insert(moov.end(), udta.begin(), udta.end());
    return moov;
}

int32_t CalculateAvcVideoDelayForTest(const std::vector<uint8_t>& config,
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::CttsEntry>& ctts, uint32_t sampleCount)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    track->sampleHelper->mimeType_ = MimeType::VIDEO_AVC;
    track->sampleHelper->sttsEntry_ = {{sampleCount, VIDEO_DELAY_TEST_SAMPLE_DURATION}};
    track->sampleHelper->cttsEntry_ = ctts;
    track->sampleHelper->sampleTableSummary_.rawSampleCount = sampleCount;
    track->sampleHelper->sampleTableSummary_.logicalSampleCount = sampleCount;
    if (!config.empty()) {
        track->codecParms.data = std::make_unique<uint8_t[]>(config.size());
        std::copy(config.begin(), config.end(), track->codecParms.data.get());
        track->codecParms.extradataSize = static_cast<int64_t>(config.size());
    }
    parser.mediaInfo_.tracks.resize(VIDEO_DELAY_TEST_TRACK_COUNT);
    parser.CalculateVideoDelay(track);
    int32_t videoDelay = -1;
    EXPECT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::VIDEO_DELAY>(videoDelay));
    return videoDelay;
}

// ctts entries [INT32_MAX, -1] make delta - minDelta exceed INT32_MAX: the reference parser must
// widen the operands to int64 before subtracting, otherwise the normalized offset wraps to INT32_MIN.
constexpr int64_t WIDE_CTTS_EXPECTED_MIN_PTS_US = 40000; // sample1: 512 ticks * 1e6 / 12800
std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> BuildWideCttsOffsetTrack()
{
    constexpr int32_t wideCttsTimeScale = 12800;
    constexpr uint32_t wideCttsSampleCount = 2;
    constexpr uint32_t wideCttsSampleDelta = 512;
    constexpr int32_t wideCttsMaxDelta = INT32_MAX;
    constexpr int32_t wideCttsMinDelta = -1;
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    track->trackIndex = 0;
    track->timeScale = wideCttsTimeScale;
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    track->sampleHelper->sttsEntry_ = {{wideCttsSampleCount, wideCttsSampleDelta}};
    track->sampleHelper->cttsEntry_ = {{1, wideCttsMaxDelta}, {1, wideCttsMinDelta}};
    return track;
}

// All ctts entries carry zero sample count: the stts/ctts conversion loop processes no sample, so the
// reference parser keeps absolutePTSOfIndexZero_ at INT64_MAX and the defensive state check must reject.
std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> BuildZeroCountCttsTrack()
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    track->trackIndex = 0;
    track->timeScale = 12800;
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    track->sampleHelper->sttsEntry_ = {{2, 512}};
    track->sampleHelper->cttsEntry_ = {{0, INT32_MAX}, {0, -1}};
    return track;
}

void WriteU16(std::vector<uint8_t>& data, size_t offset, uint16_t value)
{
    data[offset] = static_cast<uint8_t>((value >> 8) & 0xFF);
    data[offset + 1] = static_cast<uint8_t>(value & 0xFF);
}

std::vector<uint8_t> BuildMinimalMp4WithZeroCountSttsAndUlaw()
{
    // NOTE: the fixture omits mvhd/tkhd/mdhd/hdlr/etc. The parser reaches the stsd codec check
    // without requiring those atoms, so the -25 assertion below depends on that parse order;
    // if earlier moov completeness checks are added, this fixture must be completed with them.
    // stts with a single zero-count entry (FFmpeg tolerates it, strict mpeg4 must skip it).
    std::vector<uint8_t> sttsPayload;
    AppendU32(sttsPayload, 0); // version/flags
    AppendU32(sttsPayload, 1); // entry count
    AppendU32(sttsPayload, 0); // sampleCount == 0
    AppendU32(sttsPayload, 1); // delta
    auto stts = BuildAtomHeader(sttsPayload.size(), Plugins::MPEG4::FourccType("stts"), false);
    stts.insert(stts.end(), sttsPayload.begin(), sttsPayload.end());
    // stsd with a single 'ulaw' sample entry; ulaw is not in g_supportedAudioCodecsFourcc, so the
    // codec check must fail with ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT after stts is skipped.
    // The sample entry only carries the size+fourcc header (reserved/data_reference_index and
    // audio fields are omitted on purpose): the parser fails the codec check right after reading
    // that header, so the deeper fields are never accessed.
    constexpr uint32_t sampleEntryHeaderSize = 8; // size(4) + fourcc(4)
    std::vector<uint8_t> ulawEntry;
    AppendU32(ulawEntry, sampleEntryHeaderSize);
    AppendU32(ulawEntry, static_cast<uint32_t>(Plugins::MPEG4::FourccType("ulaw")));
    std::vector<uint8_t> stsdPayload;
    AppendU32(stsdPayload, 0); // version/flags
    AppendU32(stsdPayload, 1); // entry count
    stsdPayload.insert(stsdPayload.end(), ulawEntry.begin(), ulawEntry.end());
    auto stsd = BuildAtomHeader(stsdPayload.size(), Plugins::MPEG4::FourccType("stsd"), false);
    stsd.insert(stsd.end(), stsdPayload.begin(), stsdPayload.end());
    std::vector<uint8_t> stblPayload;
    stblPayload.insert(stblPayload.end(), stts.begin(), stts.end());
    stblPayload.insert(stblPayload.end(), stsd.begin(), stsd.end());
    auto stbl = BuildAtomHeader(stblPayload.size(), Plugins::MPEG4::FourccType("stbl"), false);
    stbl.insert(stbl.end(), stblPayload.begin(), stblPayload.end());
    auto minf = BuildAtomHeader(stbl.size(), Plugins::MPEG4::FourccType("minf"), false);
    minf.insert(minf.end(), stbl.begin(), stbl.end());
    auto mdia = BuildAtomHeader(minf.size(), Plugins::MPEG4::FourccType("mdia"), false);
    mdia.insert(mdia.end(), minf.begin(), minf.end());
    auto trak = BuildAtomHeader(mdia.size(), Plugins::MPEG4::FourccType("trak"), false);
    trak.insert(trak.end(), mdia.begin(), mdia.end());
    auto moov = BuildAtomHeader(trak.size(), Plugins::MPEG4::FourccType("moov"), false);
    moov.insert(moov.end(), trak.begin(), trak.end());
    std::vector<uint8_t> ftypPayload;
    AppendU32(ftypPayload, 0x69736F6D); // "isom"
    AppendU32(ftypPayload, 0); // minor version
    auto ftyp = BuildAtomHeader(ftypPayload.size(), Plugins::MPEG4::FourccType("ftyp"), false);
    ftyp.insert(ftyp.end(), ftypPayload.begin(), ftypPayload.end());
    std::vector<uint8_t> file;
    file.insert(file.end(), ftyp.begin(), ftyp.end());
    file.insert(file.end(), moov.begin(), moov.end());
    return file;
}

std::shared_ptr<DemuxerDataReader> CreateReaderForBytes(const std::vector<uint8_t>& data)
{
    auto dataSource = std::make_shared<ShortReadDataSource>(data, std::max<size_t>(data.size(), 1));
    auto reader = std::make_shared<DemuxerDataReader>();
    EXPECT_EQ(reader->SetDataReader(dataSource), Status::OK);
    return reader;
}

std::vector<uint8_t> BuildDca3Config(uint8_t samplingIndex, uint8_t channelIndex)
{
    std::vector<uint8_t> data(DCA3_CONFIG_SIZE, 0);
    uint32_t bitOffset = 0;
    auto appendBits = [&data, &bitOffset](uint32_t value, uint32_t bits) {
        for (uint32_t i = 0; i < bits; ++i) {
            uint32_t shift = bits - i - 1;
            if (((value >> shift) & 1U) != 0) {
                data[bitOffset / 8] |= static_cast<uint8_t>(1U << (7U - (bitOffset % 8U)));
            }
            ++bitOffset;
        }
    };
    appendBits(LOSSY_CODEC_ID, CODEC_ID_BITS);
    appendBits(samplingIndex, SAMPLING_INDEX_BITS);
    appendBits(BASELINE_NN_TYPE, NN_TYPE_BITS);
    appendBits(0, RESERVED_BITS);
    appendBits(CHANNEL_BASED_CONTENT, CONTENT_TYPE_BITS);
    appendBits(channelIndex, CHANNEL_INDEX_BITS);
    appendBits(0, RESERVED_BITS);
    appendBits(TEST_BITRATE_KBPS, BITRATE_BITS);
    appendBits(TEST_RESOLUTION_INDEX, RESOLUTION_BITS);
    return data;
}

std::shared_ptr<ShortReadDataSource> CreateDataSourceForBytes(const std::vector<uint8_t>& data)
{
    return std::make_shared<ShortReadDataSource>(data, std::max<size_t>(data.size(), 1));
}

std::shared_ptr<UnknownSizeDataSource> CreateUnknownSizeDataSourceForBytes(const std::vector<uint8_t>& data)
{
    return std::make_shared<UnknownSizeDataSource>(data, std::max<size_t>(data.size(), 1));
}

struct AtomParserTestContext {
    std::shared_ptr<DataSource> dataSource;
    std::shared_ptr<DemuxerDataReader> reader;
    Plugins::MPEG4::MPEG4AtomParser parser;

    explicit AtomParserTestContext(const std::vector<uint8_t>& data, bool unknownSize = false)
    {
        if (unknownSize) {
            dataSource = CreateUnknownSizeDataSourceForBytes(data);
        } else {
            dataSource = CreateDataSourceForBytes(data);
        }
        reader = std::make_shared<DemuxerDataReader>();
        EXPECT_EQ(reader->SetDataReader(dataSource), Status::OK);
        parser.dataReader_ = reader;
        parser.dataSource_ = dataSource;
    }
};

std::vector<uint8_t> BuildMinimalMp4WithTrailingBytes(size_t trailingBytes)
{
    // ftyp + moov{mvhd} is enough for MPEG4ParseHeader to mark moovFound_ and then scan to EOF,
    // where fewer than DEFAULT_HEAD_SIZE (8) trailing bytes must be tolerated like FFmpeg.
    constexpr uint32_t identityMatrix[] = {0x00010000, 0, 0, 0, 0x00010000, 0, 0, 0, 0x40000000};
    std::vector<uint8_t> mvhdPayload;
    AppendU32(mvhdPayload, 0); // version/flags
    AppendU32(mvhdPayload, 0); // creation time
    AppendU32(mvhdPayload, 0); // modification time
    AppendU32(mvhdPayload, 1000); // timescale
    AppendU32(mvhdPayload, 0); // duration
    AppendU32(mvhdPayload, 0x00010000); // rate
    AppendU32(mvhdPayload, 0x01000000); // volume(2) + reserved(2)
    AppendU32(mvhdPayload, 0); // reserved
    AppendU32(mvhdPayload, 0); // reserved
    for (uint32_t matrixValue : identityMatrix) {
        AppendU32(mvhdPayload, matrixValue);
    }
    for (uint32_t i = 0; i < 6; ++i) {
        AppendU32(mvhdPayload, 0); // pre_defined
    }
    AppendU32(mvhdPayload, 2); // next track id
    auto mvhd = BuildAtomHeader(mvhdPayload.size(), Plugins::MPEG4::FourccType("mvhd"), false);
    mvhd.insert(mvhd.end(), mvhdPayload.begin(), mvhdPayload.end());
    auto moov = BuildAtomHeader(mvhd.size(), Plugins::MPEG4::FourccType("moov"), false);
    moov.insert(moov.end(), mvhd.begin(), mvhd.end());
    std::vector<uint8_t> ftypPayload;
    AppendU32(ftypPayload, 0x69736F6D); // "isom"
    AppendU32(ftypPayload, 0); // minor version
    auto ftyp = BuildAtomHeader(ftypPayload.size(), Plugins::MPEG4::FourccType("ftyp"), false);
    ftyp.insert(ftyp.end(), ftypPayload.begin(), ftypPayload.end());
    std::vector<uint8_t> file;
    file.insert(file.end(), ftyp.begin(), ftyp.end());
    file.insert(file.end(), moov.begin(), moov.end());
    file.insert(file.end(), trailingBytes, 0x00);
    return file;
}

std::vector<uint8_t> BuildStszData(uint32_t defaultSampleSize, uint32_t sampleCount,
    const std::vector<uint32_t>& sampleSizes = {})
{
    std::vector<uint8_t> data;
    AppendU32(data, 0);
    AppendU32(data, defaultSampleSize);
    AppendU32(data, sampleCount);
    for (uint32_t sampleSize : sampleSizes) {
        AppendU32(data, sampleSize);
    }
    return data;
}

std::vector<uint8_t> BuildStz2Data(uint32_t fieldSize, uint32_t sampleCount, const std::vector<uint8_t>& payload)
{
    std::vector<uint8_t> data;
    AppendU32(data, 0);
    AppendU32(data, fieldSize);
    AppendU32(data, sampleCount);
    data.insert(data.end(), payload.begin(), payload.end());
    return data;
}

std::vector<uint8_t> BuildSttsData(const std::vector<std::pair<uint32_t, uint32_t>>& entries)
{
    std::vector<uint8_t> data;
    AppendU32(data, 0);
    AppendU32(data, static_cast<uint32_t>(entries.size()));
    for (const auto& entry : entries) {
        AppendU32(data, entry.first);
        AppendU32(data, entry.second);
    }
    return data;
}

std::vector<uint8_t> BuildCttsData(const std::vector<std::pair<uint32_t, uint32_t>>& entries)
{
    return BuildSttsData(entries);
}

std::vector<uint8_t> BuildStscData(const std::vector<std::tuple<uint32_t, uint32_t, uint32_t>>& entries)
{
    std::vector<uint8_t> data;
    AppendU32(data, 0);
    AppendU32(data, static_cast<uint32_t>(entries.size()));
    for (const auto& [firstChunk, samplesPerChunk, descriptionId] : entries) {
        AppendU32(data, firstChunk);
        AppendU32(data, samplesPerChunk);
        AppendU32(data, descriptionId);
    }
    return data;
}

std::vector<uint8_t> BuildTrunData(uint32_t flags, uint32_t sampleCount, size_t payloadSize)
{
    constexpr size_t trunHeaderSize = 8;
    constexpr size_t flagsFirstByteIndex = 1;
    constexpr size_t flagsSecondByteIndex = 2;
    constexpr size_t flagsThirdByteIndex = 3;
    constexpr size_t sampleCountFirstByteIndex = 4;
    constexpr size_t sampleCountSecondByteIndex = 5;
    constexpr size_t sampleCountThirdByteIndex = 6;
    constexpr size_t sampleCountFourthByteIndex = 7;
    constexpr uint32_t firstByteShift = 24;
    constexpr uint32_t secondByteShift = 16;
    constexpr uint32_t thirdByteShift = 8;
    std::vector<uint8_t> data(trunHeaderSize + payloadSize, 0);
    data[flagsFirstByteIndex] = static_cast<uint8_t>((flags >> secondByteShift) & 0xFF);
    data[flagsSecondByteIndex] = static_cast<uint8_t>((flags >> thirdByteShift) & 0xFF);
    data[flagsThirdByteIndex] = static_cast<uint8_t>(flags & 0xFF);
    data[sampleCountFirstByteIndex] = static_cast<uint8_t>((sampleCount >> firstByteShift) & 0xFF);
    data[sampleCountSecondByteIndex] = static_cast<uint8_t>((sampleCount >> secondByteShift) & 0xFF);
    data[sampleCountThirdByteIndex] = static_cast<uint8_t>((sampleCount >> thirdByteShift) & 0xFF);
    data[sampleCountFourthByteIndex] = static_cast<uint8_t>(sampleCount & 0xFF);
    return data;
}

std::vector<uint8_t> BuildSidxV0Data(uint8_t version, uint16_t entryCount)
{
    std::vector<uint8_t> data;
    data.resize(24 + static_cast<size_t>(entryCount) * 12, 0);
    data[0] = version;
    WriteU16(data, 22, entryCount);
    data[11] = 1; // timeScale
    for (uint16_t i = 0; i < entryCount; ++i) {
        size_t entryOffset = 24 + static_cast<size_t>(i) * 12;
        data[entryOffset + 3] = 1; // referenced size
        data[entryOffset + 7] = 1; // duration
    }
    return data;
}

std::vector<FixedSeekSampleExpected> GetH264BFfprobeSeekExpectedResults()
{
    return {
        {2440, SeekMode::SEEK_PREVIOUS_SYNC, 2440000000LL, 2000000, 1920000, 40000},
        {2440, SeekMode::SEEK_NEXT_SYNC, 2440000000LL, 3000000, 2920000, 40000},
        {2440, SeekMode::SEEK_CLOSEST_SYNC, 2440000000LL, 2000000, 1920000, 40000},
        {2600, SeekMode::SEEK_CLOSEST_SYNC, 2600000000LL, 3000000, 2920000, 40000},
    };
}

struct MetaCompareDiff {
    std::string scope;
    std::string field;
    std::string ffmpegValue;
    std::string mpeg4Value;
};

const std::vector<MetaDumpItem>& GetSourceDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::MEDIA_TITLE, "title"},
        {Tag::MEDIA_ARTIST, "artist"},
        {Tag::MEDIA_ALBUM, "album"},
        {Tag::MEDIA_ALBUM_ARTIST, "albumArt"},
        {Tag::MEDIA_DATE, "date"},
        {Tag::MEDIA_COMMENT, "comment"},
        {Tag::MEDIA_GENRE, "gnre"},
        {Tag::MEDIA_COPYRIGHT, "copyright"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::MEDIA_DESCRIPTION, "desc"},
        {Tag::MEDIA_LYRICS, "lyrics"},
        {Tag::MEDIA_AUTHOR, "auth"},
        {Tag::MEDIA_COMPOSER, "composer"},
        {Tag::MEDIA_CREATION_TIME, "createTime"},
        {Tag::MEDIA_DURATION, "duration"},
        {Tag::MEDIA_FILE_TYPE, "fileType"},
        {Tag::MEDIA_HAS_AUDIO, "hasAud"},
        {Tag::MEDIA_HAS_AUXILIARY, "hasAuxl"},
        {Tag::MEDIA_HAS_SUBTITLE, "hasSub"},
        {Tag::MEDIA_HAS_TIMEDMETA, "hasTimed"},
        {Tag::MEDIA_HAS_VIDEO, "hasVid"},
        {Tag::MEDIA_CONTAINER_START_TIME, "containStartTime"},
        {Tag::MEDIA_TRACK_COUNT, "trackCnt"},
        {Tag::MEDIA_ENCODER, "encoder"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetVideoTrackDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::MEDIA_BITRATE, "bitRate"},
        {Tag::MEDIA_CODEC_CONFIG, "csd"},
        {Tag::MIME_TYPE, "mime"},
        {Tag::VIDEO_FRAME_RATE, "frmRate"},
        {Tag::VIDEO_HEIGHT, "h"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::ORIGINAL_CODEC_NAME, "codecName"},
        {Tag::VIDEO_ROTATION, "rotate"},
        {Tag::MEDIA_START_TIME, "trackStartTime"},
        {Tag::MEDIA_TYPE, "trackType"},
        {Tag::VIDEO_DELAY, "delay"},
        {Tag::VIDEO_ORIENTATION_TYPE, "orient"},
        {Tag::VIDEO_SAR, "sar"},
        {Tag::VIDEO_WIDTH, "w"},
        {Tag::MEDIA_COVER, "cover"},
        {Tag::TRACK_REFERENCE_TYPE, "refType"},
        {Tag::TRACK_DESCRIPTION, "trackDesc"},
        {Tag::REFERENCE_TRACK_IDS, "refIds"},
        {Tag::IS_GLTF, "isGltf"},
        {Tag::GLTF_OFFSET, "gltfOffset"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetAudioTrackDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::AUDIO_AAC_IS_ADTS, "adts"},
        {Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT, "outChannelLayout"},
        {Tag::AUDIO_OUTPUT_CHANNELS, "outChannelCnt"},
        {Tag::AUDIO_BLOCK_ALIGN, "blockAlign"},
        {Tag::AUDIO_SAMPLE_FORMAT, "smpFmt"},
        {Tag::AUDIO_SAMPLE_PER_FRAME, "smpFrm"},
        {Tag::MEDIA_BITRATE, "bitRate"},
        {Tag::AUDIO_BITS_PER_CODED_SAMPLE, "bitsPerCodedSmp"},
        {Tag::AUDIO_BITS_PER_RAW_SAMPLE, "bitsPerRawSmp"},
        {Tag::AUDIO_CHANNEL_COUNT, "channelCnt"},
        {Tag::AUDIO_CHANNEL_LAYOUT, "channelLayout"},
        {Tag::MEDIA_CODEC_CONFIG, "csd"},
        {Tag::MIME_TYPE, "mime"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::ORIGINAL_CODEC_NAME, "codecName"},
        {Tag::AUDIO_SAMPLE_RATE, "smpRate"},
        {Tag::MEDIA_START_TIME, "trackStartTime"},
        {Tag::MEDIA_TYPE, "trackType"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetTextTrackDumpItems()
{
    static const std::vector<MetaDumpItem> items = {
        {Tag::MEDIA_BITRATE, "bitRate"},
        {Tag::MIME_TYPE, "mime"},
        {Tag::MEDIA_LANGUAGE, "lang"},
        {Tag::ORIGINAL_CODEC_NAME, "codecName"},
        {Tag::MEDIA_START_TIME, "trackStartTime"},
        {Tag::MEDIA_TYPE, "trackType"},
        {Tag::TIMED_METADATA_KEY, "metaKey"},
        {Tag::TIMED_METADATA_SRC_TRACK, "metaSrcTrack"},
        {Tag::TRACK_REFERENCE_TYPE, "refType"},
        {Tag::TRACK_DESCRIPTION, "trackDesc"},
        {Tag::REFERENCE_TRACK_IDS, "refIds"},
        {Tag::IS_GLTF, "isGltf"},
        {Tag::GLTF_OFFSET, "gltfOffset"},
    };
    return items;
}

const std::vector<MetaDumpItem>& GetTrackDumpItems(const Meta& meta)
{
    Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
    if (!meta.GetData(Tag::MEDIA_TYPE, mediaType)) {
        return GetVideoTrackDumpItems();
    }
    if (mediaType == Plugins::MediaType::AUDIO) {
        return GetAudioTrackDumpItems();
    }
    if (mediaType == Plugins::MediaType::SUBTITLE || mediaType == Plugins::MediaType::TIMEDMETA) {
        return GetTextTrackDumpItems();
    }
    return GetVideoTrackDumpItems();
}

template<typename EnumType>
bool AppendEnumValue(const Meta& meta, const TagType& tag, std::stringstream& dumpStr)
{
    EnumType value {};
    if (!meta.GetData(tag, value)) {
        return false;
    }
    dumpStr << static_cast<int64_t>(value);
    return true;
}

bool AppendKnownEnumValue(const Meta& meta, const TagType& tag, std::stringstream& dumpStr)
{
    if (tag == Tag::MEDIA_FILE_TYPE) {
        return AppendEnumValue<Plugins::FileType>(meta, tag, dumpStr);
    }
    if (tag == Tag::MEDIA_TYPE) {
        return AppendEnumValue<Plugins::MediaType>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_ROTATION) {
        return AppendEnumValue<Plugins::VideoRotation>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_ORIENTATION_TYPE) {
        return AppendEnumValue<Plugins::VideoOrientationType>(meta, tag, dumpStr);
    }
    if (tag == Tag::AUDIO_SAMPLE_FORMAT) {
        return AppendEnumValue<Plugins::AudioSampleFormat>(meta, tag, dumpStr);
    }
    if (tag == Tag::AUDIO_CHANNEL_LAYOUT || tag == Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT) {
        return AppendEnumValue<Plugins::AudioChannelLayout>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_COLOR_PRIMARIES) {
        return AppendEnumValue<Plugins::ColorPrimary>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_COLOR_TRC) {
        return AppendEnumValue<Plugins::TransferCharacteristic>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_COLOR_MATRIX_COEFF) {
        return AppendEnumValue<Plugins::MatrixCoefficient>(meta, tag, dumpStr);
    }
    if (tag == Tag::VIDEO_HDR_TYPE) {
        return AppendEnumValue<Plugins::HDRType>(meta, tag, dumpStr);
    }
    return false;
}

bool AppendPrimitiveMetaValue(const Meta& meta, const TagType& tag, std::stringstream& dumpStr)
{
    bool boolValue = false;
    if (meta.GetData(tag, boolValue)) {
        dumpStr << static_cast<int32_t>(boolValue);
        return true;
    }
    int32_t int32Value = 0;
    if (meta.GetData(tag, int32Value)) {
        dumpStr << int32Value;
        return true;
    }
    uint32_t uint32Value = 0;
    if (meta.GetData(tag, uint32Value)) {
        dumpStr << uint32Value;
        return true;
    }
    int64_t int64Value = 0;
    if (meta.GetData(tag, int64Value)) {
        dumpStr << int64Value;
        return true;
    }
    uint64_t uint64Value = 0;
    if (meta.GetData(tag, uint64Value)) {
        dumpStr << uint64Value;
        return true;
    }
    float floatValue = 0.0F;
    if (meta.GetData(tag, floatValue)) {
        dumpStr << std::to_string(floatValue);
        return true;
    }
    double doubleValue = 0.0;
    if (meta.GetData(tag, doubleValue)) {
        dumpStr << std::to_string(doubleValue);
        return true;
    }
    std::string stringValue;
    if (meta.GetData(tag, stringValue)) {
        dumpStr << stringValue;
        return true;
    }
    return AppendKnownEnumValue(meta, tag, dumpStr);
}

std::string GetMetaDumpValue(const Meta& meta, const MetaDumpItem& item)
{
    std::vector<uint8_t> bytes;
    if (meta.GetData(item.tag, bytes)) {
        return "size=" + std::to_string(bytes.size());
    }
    std::vector<uint32_t> uint32Values;
    if (meta.GetData(item.tag, uint32Values)) {
        return "size=" + std::to_string(uint32Values.size());
    }
    std::vector<int32_t> int32Values;
    if (meta.GetData(item.tag, int32Values)) {
        return "size=" + std::to_string(int32Values.size());
    }
    std::vector<int64_t> int64Values;
    if (meta.GetData(item.tag, int64Values)) {
        return "size=" + std::to_string(int64Values.size());
    }
    std::stringstream dumpStr;
    if (!AppendPrimitiveMetaValue(meta, item.tag, dumpStr)) {
        dumpStr << "<unsupported>";
    }
    return dumpStr.str();
}

std::vector<MetaDumpItem> MergeDumpItems(const std::vector<MetaDumpItem>& lhs, const std::vector<MetaDumpItem>& rhs)
{
    std::vector<MetaDumpItem> items = lhs;
    for (const auto& rhsItem : rhs) {
        auto iter = std::find_if(items.begin(), items.end(), [&rhsItem](const MetaDumpItem& item) {
            return item.tag == rhsItem.tag;
        });
        if (iter == items.end()) {
            items.emplace_back(rhsItem);
        }
    }
    return items;
}

std::string GetComparableMetaValue(const Meta& meta, const MetaDumpItem& item)
{
    if (meta.Find(item.tag) == meta.end()) {
        return "<missing>";
    }
    return GetMetaDumpValue(meta, item);
}

bool ContainsIssueField(const std::vector<std::string>& issueFields, const std::string& field)
{
    return std::find(issueFields.begin(), issueFields.end(), field) != issueFields.end();
}

void AppendMetaDiffs(std::vector<MetaCompareDiff>& diffs, const std::string& scope, const Meta& ffmpegMeta,
    const Meta& mpeg4Meta, const std::vector<MetaDumpItem>& items)
{
    for (const auto& item : items) {
        std::string ffmpegValue = GetComparableMetaValue(ffmpegMeta, item);
        std::string mpeg4Value = GetComparableMetaValue(mpeg4Meta, item);
        if (ffmpegValue == mpeg4Value) {
            continue;
        }
        diffs.emplace_back(MetaCompareDiff {scope, item.name, ffmpegValue, mpeg4Value});
    }
}

void PrintSetDataSourceDiff(const MetaCompareDiff& diff)
{
    std::cout << "  " << diff.scope << "." << diff.field << ": ffmpeg=" << diff.ffmpegValue;
    std::cout << ", mpeg4=" << diff.mpeg4Value << std::endl;
}

void PrintSetDataSourceDiffGroupHeader(const std::string& diffType, const std::string& filePath)
{
    std::cout << "[SetDataSourceCompatibility][" << diffType << "] file=" << filePath << std::endl;
}

void ExpectSetDataSourceDiffs(const std::string& filePath, const std::vector<MetaCompareDiff>& diffs,
    const std::vector<std::string>& ignoredFields)
{
    bool hasKnownDiff = false;
    bool hasUnexpectedDiff = false;
    for (const auto& diff : diffs) {
        if (ContainsIssueField(ignoredFields, diff.field)) {
            if (!hasKnownDiff) {
                PrintSetDataSourceDiffGroupHeader("known", filePath);
                hasKnownDiff = true;
            }
            PrintSetDataSourceDiff(diff);
            continue;
        }
        if (!hasUnexpectedDiff) {
            PrintSetDataSourceDiffGroupHeader("unexpected", filePath);
            hasUnexpectedDiff = true;
        }
        PrintSetDataSourceDiff(diff);
        EXPECT_EQ(diff.ffmpegValue, diff.mpeg4Value) << diff.scope << "." << diff.field;
    }
}

void ExpectSetDataSourceCompatible(const std::string& filePath, const SetDataSourceRunResult& ffmpegResult,
    const SetDataSourceRunResult& mpeg4Result, const std::vector<std::string>& ignoredFields)
{
    SCOPED_TRACE(filePath);
    std::vector<MetaCompareDiff> diffs;
    if (ffmpegResult.ret != mpeg4Result.ret) {
        diffs.emplace_back(MetaCompareDiff {"[SetDataSource]", "ret",
            std::to_string(static_cast<int32_t>(ffmpegResult.ret)),
            std::to_string(static_cast<int32_t>(mpeg4Result.ret))});
    }
    if (ffmpegResult.hasMediaInfo != mpeg4Result.hasMediaInfo) {
        diffs.emplace_back(MetaCompareDiff {"[GetMediaInfo]", "available",
            std::to_string(static_cast<int32_t>(ffmpegResult.hasMediaInfo)),
            std::to_string(static_cast<int32_t>(mpeg4Result.hasMediaInfo))});
    }
    ExpectSetDataSourceDiffs(filePath, diffs, ignoredFields);
    if (!ffmpegResult.hasMediaInfo || !mpeg4Result.hasMediaInfo) {
        return;
    }

    diffs.clear();
    AppendMetaDiffs(diffs, "[source]", ffmpegResult.mediaInfo.general, mpeg4Result.mediaInfo.general,
        GetSourceDumpItems());
    if (ffmpegResult.mediaInfo.tracks.size() != mpeg4Result.mediaInfo.tracks.size()) {
        diffs.emplace_back(MetaCompareDiff {"[track]", "track.count",
            std::to_string(ffmpegResult.mediaInfo.tracks.size()),
            std::to_string(mpeg4Result.mediaInfo.tracks.size())});
    }
    size_t trackCount = std::min(ffmpegResult.mediaInfo.tracks.size(), mpeg4Result.mediaInfo.tracks.size());
    for (size_t index = 0; index < trackCount; ++index) {
        const auto& ffmpegTrack = ffmpegResult.mediaInfo.tracks[index];
        const auto& mpeg4Track = mpeg4Result.mediaInfo.tracks[index];
        auto items = MergeDumpItems(GetTrackDumpItems(ffmpegTrack), GetTrackDumpItems(mpeg4Track));
        AppendMetaDiffs(diffs, "[track " + std::to_string(index) + "]", ffmpegTrack, mpeg4Track, items);
    }
    ExpectSetDataSourceDiffs(filePath, diffs, ignoredFields);
}

std::string ToLowerAscii(std::string value)
{
    std::transform(value.begin(), value.end(), value.begin(), [](unsigned char ch) {
        return static_cast<char>(std::tolower(ch));
    });
    return value;
}

bool HasDemuxerResourceSuffix(const std::string& fileName)
{
    static const std::vector<std::string> demuxerSuffixes = {
        ".mov", ".mp4", ".m4a", ".3gp", ".3g2", ".mj2", ".psp", ".m4b", ".ism", ".ismv", ".isma",
        ".f4v", ".avif", ".heic", ".heif"
    };
    std::string lowerName = ToLowerAscii(fileName);
    return std::any_of(demuxerSuffixes.begin(), demuxerSuffixes.end(), [&lowerName](const std::string& suffix) {
        return lowerName.size() >= suffix.size() &&
            lowerName.compare(lowerName.size() - suffix.size(), suffix.size(), suffix) == 0;
    });
}

bool IsSupportedFile(const std::string& fileName)
{
    return (std::find(g_supportFiles.cbegin(), g_supportFiles.cend(), fileName) != g_supportFiles.cend());
}

std::vector<std::string> CollectDemuxerResourceFiles()
{
    std::vector<std::string> filePaths;
    DIR* dir = opendir(TEST_DEMUXER_RESOURCE_DIR.c_str());
    if (dir == nullptr) {
        ADD_FAILURE() << "Failed to open resource dir: " << TEST_DEMUXER_RESOURCE_DIR;
        return filePaths;
    }

    struct dirent* entry = nullptr;
    while ((entry = readdir(dir)) != nullptr) {
        std::string fileName = entry->d_name;
        if (fileName == "." || fileName == ".." || !HasDemuxerResourceSuffix(fileName) || !IsSupportedFile(fileName)) {
            continue;
        }
        std::string filePath = TEST_DEMUXER_RESOURCE_DIR + "/" + fileName;
        struct stat fileStatus {};
        if (stat(filePath.c_str(), &fileStatus) != 0 || !S_ISREG(fileStatus.st_mode)) {
            continue;
        }
        filePaths.emplace_back(filePath);
    }
    closedir(dir);
    std::sort(filePaths.begin(), filePaths.end());
    return filePaths;
}

// FFmpeg may still convert HEVC hvcC to AnnexB through hevc_mp4toannexb when the HEVC parser
// library is unavailable, while MPEG4 demuxer now exposes the original codec config in that case.
static const std::vector<KnownSetDataSourceDiffResource> g_knownSetDataSourceDiffResources = {
    {TEST_DEMUXER_RESOURCE_DIR + "/265_pcm_s16le.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_16bit_44100Hz_64kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_44100Hz_16bit_32k.m4a",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_44100Hz_16bit_32k.mp4", {"outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/2obj_44100Hz_16bit_32k_noftyp.mp4", {"outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_16bit_32000Hz_704kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_16bit_96000Hz_704kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_24bit_48000Hz_704kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout", "bitsPerCodedSmp"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/5_1_4_4obj_16bit_48000Hz_1232kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/7_1_4_16bit_48000Hz_832kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/aac_hdrvivid.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/aigc_str.mov", {"title"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/audiovivid_hdrvivid.mp4", {"csd", "outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/audiovivid_hdrvivid_2s.mp4", {"csd", "outChannelLayout", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/camera.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/camera_h265_aac_rotate270.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h264_double_video_audio.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h265_fmp4.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h265_opus_ch7.1.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/h265_pcm_s16le.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_db_no_image.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_hdr10.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_hdrVivid.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_hlg.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrType_sdr.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hdrvivid_720p_2s.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hevc_aac.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hevc_aac.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/hoa3_16bit_48000Hz_640kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/multi_trk.mov", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/muxer_auxl_265.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/noframe.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/stereo_16bit_48000Hz_32kbps.mp4",
        {"outChannelLayout", "smpFmt", "smpFrm", "bitsPerRawSmp", "channelLayout"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/test_265_B_Gop25_4sec.mp4", {"csd"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/webvtt.mp4", {"lang"}},
    {TEST_DEMUXER_RESOURCE_DIR + "/wvtt.mp4", {"lang"}},
};

const std::vector<KnownSetDataSourceDiffResource>& GetKnownSetDataSourceDiffResources()
{
    return g_knownSetDataSourceDiffResources;
}

std::vector<std::string> GetKnownIssueFields(const std::string& filePath)
{
    // These fields are known long-standing FFmpeg/MPEG4 metadata differences and should not
    // block the resource-wide compatibility guard.
    static const std::vector<std::string> commonIssueFields = {
        "timeScale",
        "encoder",
        "codecName",
        "blockAlign",
    };

    std::vector<std::string> issueFields = commonIssueFields;
    const auto& resources = GetKnownSetDataSourceDiffResources();
    auto iter = std::find_if(resources.begin(), resources.end(),
        [&filePath](const KnownSetDataSourceDiffResource& resource) {
            return resource.filePath == filePath;
        });
    if (iter == resources.end()) {
        return issueFields;
    }
    for (const auto& field : iter->issueFields) {
        if (!ContainsIssueField(issueFields, field)) {
            issueFields.emplace_back(field);
        }
    }
    return issueFields;
}

std::shared_ptr<DataSourceImpl> CreateFileDataSource(const std::string& filePath, int& fd)
{
    struct stat fileStatus {};
    if (stat(filePath.c_str(), &fileStatus) != 0) {
        ADD_FAILURE() << "Failed to stat file: " << filePath;
        return nullptr;
    }
    fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        ADD_FAILURE() << "Failed to open file: " << filePath;
        return nullptr;
    }

    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    if (source->SetSource(mediaSource) != Status::OK) {
        ADD_FAILURE() << "Set source failed";
        return nullptr;
    }
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    if (streamDemuxer->Init(uri) != Status::OK) {
        ADD_FAILURE() << "Init stream demuxer failed";
        return nullptr;
    }
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    return std::make_shared<DataSourceImpl>(streamDemuxer, 0);
}

SetDataSourceRunResult RunSetDataSourceOnly(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const std::string& filePath)
{
    SetDataSourceRunResult result;
    int fd = -1;
    auto dataSource = CreateFileDataSource(filePath, fd);
    if (dataSource == nullptr) {
        if (fd >= 0) {
            close(fd);
        }
        return result;
    }
    result.ret = plugin->SetDataSource(dataSource);
    if (result.ret == Status::OK) {
        result.mediaInfoRet = plugin->GetMediaInfo(result.mediaInfo);
        result.hasMediaInfo = result.mediaInfoRet == Status::OK;
    }
    plugin->Reset();
    close(fd);
    return result;
}

std::shared_ptr<Plugins::MPEG4::Sample> CreateMpeg4Sample(int32_t size, bool hasData)
{
    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    sample->pts = 0;
    sample->dts = 0;
    sample->duration = 0;
    sample->flag = Plugins::MPEG4::Sample::NONE;
    sample->size = size;
    if (hasData && size > 0) {
        sample->data = std::make_unique<uint8_t[]>(size);
        for (int32_t index = 0; index < size; ++index) {
            sample->data[index] = static_cast<uint8_t>(index);
        }
    }
    return sample;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareZeroLengthAacTrack(
    Plugins::MPEG4::MPEG4DemuxerPlugin& plugin)
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    EXPECT_NE(track, nullptr);
    if (track == nullptr) {
        return nullptr;
    }
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    EXPECT_NE(helper, nullptr);
    if (helper == nullptr) {
        return nullptr;
    }
    helper->sampleIndexEntry_.push_back(MakeSampleIndexEntry(0, 0, 0, 0));
    helper->durationOverrides_.push_back({0, 0});
    helper->sttsEntry_ = {{1, 0}};
    helper->allSamplesSync_ = true;
    helper->trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper->mimeType_ = Plugins::MimeType::AUDIO_AAC;
    track->trackIndex = 0;
    track->trackId = 0;
    track->timeScale = ZERO_LENGTH_AAC_SAMPLE_RATE;
    track->codecParms.trackType = Plugins::MPEG4::AUDIO_TYPE;
    track->sampleHelper = helper;
    plugin.firstTrack_ = track;
    plugin.mediaInfo_.tracks.resize(1);
    plugin.mediaInfo_.tracks[0].Set<Tag::AUDIO_SAMPLE_RATE>(ZERO_LENGTH_AAC_SAMPLE_RATE);
    plugin.mediaInfo_.tracks[0].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(ZERO_LENGTH_AAC_SAMPLES_PER_FRAME);
    return track;
}

std::vector<uint8_t> BuildMpegAudioTestFrame()
{
    std::vector<uint8_t> frame(MPEG_AUDIO_TEST_FRAME_SIZE, 0);
    std::copy(std::begin(MPEG_AUDIO_TEST_HEADER), std::end(MPEG_AUDIO_TEST_HEADER), frame.begin());
    for (size_t index = sizeof(MPEG_AUDIO_TEST_HEADER); index < frame.size(); ++index) {
        frame[index] = static_cast<uint8_t>(index);
    }
    return frame;
}

std::vector<uint8_t> BuildMpegAudioBoundaryTestFile()
{
    constexpr size_t frameCount = 5;
    std::vector<uint8_t> file;
    file.reserve(frameCount * MPEG_AUDIO_TEST_FRAME_SIZE);
    for (size_t index = 0; index < frameCount; ++index) {
        auto frame = BuildMpegAudioTestFrame();
        std::fill(frame.begin() + sizeof(MPEG_AUDIO_TEST_HEADER), frame.end(), static_cast<uint8_t>(index));
        file.insert(file.end(), frame.begin(), frame.end());
    }
    return file;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareMpegAudioTestTrack(
    Plugins::MPEG4::MPEG4DemuxerPlugin& plugin, const std::vector<uint32_t>& sampleSizes)
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr || sampleSizes.size() > UINT32_MAX) {
        return nullptr;
    }
    int64_t pos = 0;
    for (size_t index = 0; index < sampleSizes.size(); ++index) {
        int64_t timestamp = static_cast<int64_t>(index) * MPEG_AUDIO_TEST_SAMPLES_PER_FRAME;
        helper->sampleIndexEntry_.emplace_back(MakeSampleIndexEntry(pos, timestamp, timestamp, sampleSizes[index]));
        pos += static_cast<int64_t>(sampleSizes[index]);
    }
    if (!sampleSizes.empty() &&
        helper->AddDurationOverride(static_cast<uint32_t>(sampleSizes.size() - 1),
            MPEG_AUDIO_TEST_SAMPLES_PER_FRAME) != Status::OK) {
        return nullptr;
    }
    helper->allSamplesSync_ = true;
    helper->trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper->mimeType_ = MimeType::AUDIO_MPEG;
    track->trackIndex = MPEG_AUDIO_TEST_TRACK_ID;
    track->trackId = static_cast<int32_t>(MPEG_AUDIO_TEST_TRACK_ID);
    track->timeScale = MPEG_AUDIO_TEST_SAMPLE_RATE;
    // Model the mdhd duration as the STTS total (one MP3 frame per sample), matching real parses
    // where the stream end is always known; tests may override track->duration for tail scenarios.
    track->duration = static_cast<int64_t>(sampleSizes.size()) * MPEG_AUDIO_TEST_SAMPLES_PER_FRAME;
    track->codecParms.trackType = Plugins::MPEG4::AUDIO_TYPE;
    track->sampleHelper = helper;
    track->cache = std::make_shared<Plugins::MPEG4::SampleCache>();
    plugin.firstTrack_ = track;
    plugin.lastTrack_ = track;
    plugin.mediaInfo_.tracks.resize(1);
    plugin.mediaInfo_.general.Set<Tag::MEDIA_TRACK_COUNT>(1);
    plugin.mediaInfo_.tracks[MPEG_AUDIO_TEST_TRACK_ID].Set<Tag::AUDIO_SAMPLE_RATE>(MPEG_AUDIO_TEST_SAMPLE_RATE);
    plugin.mediaInfo_.tracks[MPEG_AUDIO_TEST_TRACK_ID].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(
        MPEG_AUDIO_TEST_SAMPLES_PER_FRAME);
    plugin.trackMtx_[MPEG_AUDIO_TEST_TRACK_ID] = std::make_shared<std::mutex>();
    plugin.inited_ = true;
    plugin.seekable_ = Seekable::SEEKABLE;
    plugin.selectedTrackIds_ = {MPEG_AUDIO_TEST_TRACK_ID};
    return track;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareAacTailTestTrack(
    Plugins::MPEG4::MPEG4DemuxerPlugin& plugin, const std::vector<uint32_t>& sampleSizes,
    int32_t timeScale, int64_t mdhdDuration, int64_t tailDurationTicks, int64_t tailOverrideTicks,
    bool exportSamplePerFrame, int64_t emptyEditTicks, int64_t rawTotalDuration)
{
    constexpr uint32_t testAacTrackId = 0;
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr || sampleSizes.empty() || timeScale <= 0) {
        return nullptr;
    }
    int64_t pos = 0;
    int64_t dts = emptyEditTicks;
    for (size_t index = 0; index < sampleSizes.size(); ++index) {
        helper->sampleIndexEntry_.emplace_back(MakeSampleIndexEntry(pos, dts, dts, sampleSizes[index]));
        dts += tailDurationTicks;
        pos += static_cast<int64_t>(sampleSizes[index]);
    }
    if (helper->AddDurationOverride(static_cast<uint32_t>(sampleSizes.size() - 1),
        static_cast<uint32_t>(tailOverrideTicks)) != Status::OK) {
        return nullptr;
    }
    helper->allSamplesSync_ = true;
    helper->trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper->mimeType_ = MimeType::AUDIO_AAC;
    helper->rawTotalDuration_ = rawTotalDuration;
    track->trackIndex = testAacTrackId;
    track->trackId = static_cast<int32_t>(testAacTrackId);
    track->timeScale = timeScale;
    track->duration = mdhdDuration;
    if (emptyEditTicks > 0) {
        track->elstInitEmptyEdit = emptyEditTicks;
        track->editListEntries = {{-1, emptyEditTicks, 1, 0}};
    }
    track->codecParms.trackType = Plugins::MPEG4::AUDIO_TYPE;
    track->sampleHelper = helper;
    track->cache = std::make_shared<Plugins::MPEG4::SampleCache>();
    plugin.firstTrack_ = track;
    plugin.lastTrack_ = track;
    plugin.mediaInfo_.tracks.resize(1);
    plugin.mediaInfo_.tracks[testAacTrackId].Set<Tag::MIME_TYPE>(MimeType::AUDIO_AAC);
    plugin.mediaInfo_.tracks[testAacTrackId].Set<Tag::AUDIO_SAMPLE_RATE>(timeScale);
    if (exportSamplePerFrame) {
        plugin.mediaInfo_.tracks[testAacTrackId].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(
            AAC_TEST_CORE_SAMPLES_PER_FRAME);
    }
    plugin.trackMtx_[testAacTrackId] = std::make_shared<std::mutex>();
    plugin.inited_ = true;
    plugin.seekable_ = Seekable::SEEKABLE;
    plugin.selectedTrackIds_ = {testAacTrackId};
    return track;
}

std::vector<uint8_t> BuildMpegAudioTestFrames(size_t frameCount)
{
    std::vector<uint8_t> frames;
    frames.reserve(frameCount * MPEG_AUDIO_TEST_FRAME_SIZE);
    for (size_t index = 0; index < frameCount; ++index) {
        auto frame = BuildMpegAudioTestFrame();
        std::fill(frame.begin() + sizeof(MPEG_AUDIO_TEST_HEADER), frame.end(), static_cast<uint8_t>(index));
        frames.insert(frames.end(), frame.begin(), frame.end());
    }
    return frames;
}

std::shared_ptr<Plugins::MPEG4::MPEG4SampleHelper> BuildEmptyStssSyncTestHelper(
    Plugins::MPEG4::TrackType trackType, const std::string& mimeType)
{
    constexpr uint32_t emptyStssTestSampleCount = 4;
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (helper == nullptr) {
        return nullptr;
    }
    helper->emptyStss_ = true;
    helper->trackType_ = trackType;
    helper->mimeType_ = mimeType;
    helper->sttsEntry_ = {{emptyStssTestSampleCount, 1}};
    helper->stscEntry_ = {{0, emptyStssTestSampleCount, 1}};
    helper->sampleSizes_ = {1, 1, 1, 1};
    helper->sampleSizeCount_ = emptyStssTestSampleCount;
    helper->chunkOffsets_ = {0};
    return helper;
}

std::vector<uint8_t> BuildAvcCTestConfig(const std::vector<uint8_t>& sps)
{
    constexpr uint8_t avcConfigVersion = 0x01;
    constexpr uint8_t avcProfile = 0x42;
    constexpr uint8_t avcCompat = 0x00;
    constexpr uint8_t avcLevel = 0x1E;
    constexpr uint8_t avcLengthSize = 0xFF;
    constexpr uint8_t avcSpsCount = 0xE1;
    constexpr uint8_t avcPpsCount = 0x01;
    constexpr uint8_t avcConfigLengthHighByteShift = 8;
    constexpr std::array<uint8_t, 4> pps = {0x68, 0xCE, 0x3C, 0x80};
    std::vector<uint8_t> config = {
        avcConfigVersion, avcProfile, avcCompat, avcLevel, avcLengthSize, avcSpsCount,
        static_cast<uint8_t>((sps.size() >> avcConfigLengthHighByteShift) & 0xFF),
        static_cast<uint8_t>(sps.size() & 0xFF),
    };
    config.insert(config.end(), sps.begin(), sps.end());
    config.push_back(avcPpsCount);
    config.push_back(static_cast<uint8_t>((pps.size() >> avcConfigLengthHighByteShift) & 0xFF));
    config.push_back(static_cast<uint8_t>(pps.size() & 0xFF));
    config.insert(config.end(), pps.begin(), pps.end());
    return config;
}

std::vector<uint8_t> BuildLengthPrefixedNal(const std::vector<uint8_t>& nal)
{
    std::vector<uint8_t> sample;
    AppendU32(sample, static_cast<uint32_t>(nal.size()));
    sample.insert(sample.end(), nal.begin(), nal.end());
    return sample;
}

std::vector<uint8_t> BuildLengthPrefixedNal(const std::vector<uint8_t>& nal, uint8_t lengthSize)
{
    std::vector<uint8_t> sample;
    for (uint8_t i = 0; i < lengthSize; ++i) {
        constexpr uint8_t bitsPerByte = 8;
        const uint8_t shift = static_cast<uint8_t>((lengthSize - 1 - i) * bitsPerByte);
        sample.push_back(static_cast<uint8_t>((nal.size() >> shift) & 0xFF));
    }
    sample.insert(sample.end(), nal.begin(), nal.end());
    return sample;
}

void BuildAvcSampleData(const std::vector<std::vector<uint8_t>>& nals, std::vector<uint8_t>& sampleData,
    std::vector<int32_t>& sampleSizes, bool separateSamples)
{
    if (!separateSamples) {
        for (const auto& nal : nals) {
            std::vector<uint8_t> data = BuildLengthPrefixedNal(nal);
            sampleData.insert(sampleData.end(), data.begin(), data.end());
        }
        sampleSizes.push_back(static_cast<int32_t>(sampleData.size()));
        return;
    }
    sampleSizes.reserve(nals.size());
    for (const auto& nal : nals) {
        std::vector<uint8_t> sample = BuildLengthPrefixedNal(nal);
        sampleSizes.push_back(static_cast<int32_t>(sample.size()));
        sampleData.insert(sampleData.end(), sample.begin(), sample.end());
    }
}

void BuildAvcMultiSampleData(const std::vector<std::vector<std::vector<uint8_t>>>& samples,
    std::vector<uint8_t>& sampleData, std::vector<int32_t>& sampleSizes)
{
    sampleSizes.clear();
    for (const auto& nals : samples) {
        size_t sampleStart = sampleData.size();
        for (const auto& nal : nals) {
            std::vector<uint8_t> packet = BuildLengthPrefixedNal(nal);
            sampleData.insert(sampleData.end(), packet.begin(), packet.end());
        }
        sampleSizes.push_back(static_cast<int32_t>(sampleData.size() - sampleStart));
    }
}

void ActivateVideoDurationTestTrack(Plugins::MPEG4::MPEG4DemuxerPlugin& plugin,
    const std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track>& track, const std::string& mime);

std::shared_ptr<Plugins::MPEG4::MPEG4DemuxerPlugin> BuildAvcKeyHeuristicTestPlugin(
    const std::vector<uint8_t>& spsNal, const std::vector<std::vector<uint8_t>>& nals,
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::StssEntry>& stssIndexes, bool separateSamples = true)
{
    constexpr uint32_t testVideoTrackId = 0;
    constexpr int32_t testTrackTimeScale = 90000;
    constexpr uint32_t testSttsDelta = 3600;
    const std::vector<uint8_t> avcc = BuildAvcCTestConfig(spsNal);
    std::vector<uint8_t> sampleData;
    std::vector<int32_t> sampleSizes;
    BuildAvcSampleData(nals, sampleData, sampleSizes, separateSamples);
    const uint32_t testSampleCount = static_cast<uint32_t>(sampleSizes.size());
    auto source = CreateDataSourceForBytes(sampleData);
    auto reader = std::make_shared<DemuxerDataReader>();
    if (source == nullptr || reader == nullptr || reader->SetDataReader(source) != Status::OK) {
        return nullptr;
    }
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr) {
        return nullptr;
    }
    helper->trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper->mimeType_ = MimeType::VIDEO_AVC;
    helper->sttsEntry_ = {{testSampleCount, testSttsDelta}};
    helper->cttsEntry_ = {{testSampleCount, 0}};
    helper->stscEntry_ = {{0, testSampleCount, 1}};
    helper->stssEntry_ = stssIndexes;
    helper->sampleSizes_ = sampleSizes;
    helper->sampleSizeCount_ = testSampleCount;
    helper->chunkOffsets_ = {0};
    helper->dataReader_ = reader;
    if (helper->FinalizeSampleTables() != Status::OK) {
        return nullptr;
    }
    track->trackIndex = testVideoTrackId;
    track->trackId = static_cast<int32_t>(testVideoTrackId);
    track->timeScale = testTrackTimeScale;
    track->codecParms.trackType = Plugins::MPEG4::VIDEO_TYPE;
    track->codecParms.data = std::make_unique<uint8_t[]>(avcc.size());
    std::copy(avcc.begin(), avcc.end(), track->codecParms.data.get());
    track->codecParms.extradataSize = static_cast<int64_t>(avcc.size());
    track->sampleHelper = helper;
    auto plugin = std::make_shared<Plugins::MPEG4::MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ActivateVideoDurationTestTrack(*plugin, track, MimeType::VIDEO_AVC);
    plugin->dataSource_ = source;
    return plugin;
}

std::shared_ptr<Plugins::MPEG4::MPEG4DemuxerPlugin> BuildAvcMultiNalSampleTestPlugin(
    const std::vector<uint8_t>& spsNal, const std::vector<uint8_t>& sampleData,
    const std::vector<int32_t>& sampleSizes,
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::StssEntry>& stssIndexes)
{
    constexpr uint32_t testVideoTrackId = 0;
    constexpr int32_t testTrackTimeScale = 90000;
    constexpr uint32_t testSttsDelta = 3600;
    const uint32_t testSampleCount = static_cast<uint32_t>(sampleSizes.size());
    const std::vector<uint8_t> avcc = BuildAvcCTestConfig(spsNal);
    auto source = CreateDataSourceForBytes(sampleData);
    auto reader = std::make_shared<DemuxerDataReader>();
    if (source == nullptr || reader == nullptr || reader->SetDataReader(source) != Status::OK) {
        return nullptr;
    }
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr) {
        return nullptr;
    }
    helper->trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper->mimeType_ = MimeType::VIDEO_AVC;
    helper->sttsEntry_ = {{testSampleCount, testSttsDelta}};
    helper->cttsEntry_ = {{testSampleCount, 0}};
    helper->stscEntry_ = {{0, testSampleCount, 1}};
    helper->stssEntry_ = stssIndexes;
    helper->sampleSizes_ = sampleSizes;
    helper->sampleSizeCount_ = testSampleCount;
    helper->chunkOffsets_ = {0};
    helper->dataReader_ = reader;
    if (helper->FinalizeSampleTables() != Status::OK) {
        return nullptr;
    }
    track->trackIndex = testVideoTrackId;
    track->trackId = static_cast<int32_t>(testVideoTrackId);
    track->timeScale = testTrackTimeScale;
    track->codecParms.trackType = Plugins::MPEG4::VIDEO_TYPE;
    track->codecParms.data = std::make_unique<uint8_t[]>(avcc.size());
    std::copy(avcc.begin(), avcc.end(), track->codecParms.data.get());
    track->codecParms.extradataSize = static_cast<int64_t>(avcc.size());
    track->sampleHelper = helper;
    auto plugin = std::make_shared<Plugins::MPEG4::MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ActivateVideoDurationTestTrack(*plugin, track, MimeType::VIDEO_AVC);
    plugin->dataSource_ = source;
    return plugin;
}

// Minimal HEVCDecoderConfigurationRecord (ISO/IEC 14496-15): the fixed 23-byte header stores
// lengthSizeMinusOne in the low 2 bits of byte 21 and numOfArrays at byte 22, the same layout
// FFmpeg reads in hevc_mp4toannexb.c (length_size = (byte & 3) + 1).
std::vector<uint8_t> BuildHevcCTestConfig(const std::vector<uint8_t>& vps,
    const std::vector<uint8_t>& sps, const std::vector<uint8_t>& pps, uint8_t lengthSizeMinusOne)
{
    constexpr uint8_t hevcConfigVersion = 0x01;
    constexpr uint8_t hevcProfileIdc = 0x01; // Main profile, no tier
    constexpr uint8_t hevcLevelIdc = 0x00;
    constexpr uint8_t hevcNumArrays = 0x03;
    constexpr uint8_t hevcReservedByte = 0x00;
    constexpr uint8_t hevcArrayHeaderNalTypeMask = 0x3F;
    constexpr uint8_t hevcVpsNalType = 0x20; // HEVC_NAL_VPS = 32
    constexpr uint8_t hevcSpsNalType = 0x21; // HEVC_NAL_SPS = 33
    constexpr uint8_t hevcPpsNalType = 0x22; // HEVC_NAL_PPS = 34
    constexpr uint16_t hevcNumNalusPerArray = 0x01;
    std::vector<uint8_t> config = {
        hevcConfigVersion, hevcProfileIdc, hevcReservedByte, hevcReservedByte, hevcReservedByte,
        hevcReservedByte, hevcReservedByte, hevcReservedByte, hevcReservedByte, hevcReservedByte,
        hevcReservedByte, hevcReservedByte, hevcLevelIdc, hevcReservedByte, hevcReservedByte,
        hevcReservedByte, hevcReservedByte, hevcReservedByte, hevcReservedByte, hevcReservedByte,
        hevcReservedByte, lengthSizeMinusOne, hevcNumArrays,
    };
    const auto appendArray = [&config](uint8_t nalType, const std::vector<uint8_t>& nal) {
        config.push_back(static_cast<uint8_t>(nalType & hevcArrayHeaderNalTypeMask));
        AppendU16(config, hevcNumNalusPerArray);
        AppendU16(config, static_cast<uint16_t>(nal.size()));
        config.insert(config.end(), nal.begin(), nal.end());
    };
    appendArray(hevcVpsNalType, vps);
    appendArray(hevcSpsNalType, sps);
    appendArray(hevcPpsNalType, pps);
    return config;
}

std::vector<uint8_t> BuildHevcLengthPrefixedNal(const std::vector<uint8_t>& nal)
{
    std::vector<uint8_t> sample;
    AppendU16(sample, static_cast<uint16_t>(nal.size()));
    sample.insert(sample.end(), nal.begin(), nal.end());
    return sample;
}

std::shared_ptr<Plugins::MPEG4::MPEG4DemuxerPlugin> BuildHevcSampleTestPlugin(
    const std::vector<uint8_t>& hvcc, const std::vector<uint8_t>& sampleData,
    const std::vector<int32_t>& sampleSizes,
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::StssEntry>& stssIndexes)
{
    constexpr uint32_t testVideoTrackId = 0;
    constexpr int32_t testTrackTimeScale = 90000;
    constexpr uint32_t testSttsDelta = 3600;
    const uint32_t testSampleCount = static_cast<uint32_t>(sampleSizes.size());
    auto source = CreateDataSourceForBytes(sampleData);
    auto reader = std::make_shared<DemuxerDataReader>();
    if (source == nullptr || reader == nullptr || reader->SetDataReader(source) != Status::OK) {
        return nullptr;
    }
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr) {
        return nullptr;
    }
    helper->trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper->mimeType_ = MimeType::VIDEO_HEVC;
    helper->sttsEntry_ = {{testSampleCount, testSttsDelta}};
    helper->cttsEntry_ = {{testSampleCount, 0}};
    helper->stscEntry_ = {{0, testSampleCount, 1}};
    helper->stssEntry_ = stssIndexes;
    helper->sampleSizes_ = sampleSizes;
    helper->sampleSizeCount_ = testSampleCount;
    helper->chunkOffsets_ = {0};
    helper->dataReader_ = reader;
    if (helper->FinalizeSampleTables() != Status::OK) {
        return nullptr;
    }
    track->trackIndex = testVideoTrackId;
    track->trackId = static_cast<int32_t>(testVideoTrackId);
    track->timeScale = testTrackTimeScale;
    track->codecParms.trackType = Plugins::MPEG4::VIDEO_TYPE;
    track->codecParms.data = std::make_unique<uint8_t[]>(hvcc.size());
    std::copy(hvcc.begin(), hvcc.end(), track->codecParms.data.get());
    track->codecParms.extradataSize = static_cast<int64_t>(hvcc.size());
    track->sampleHelper = helper;
    auto plugin = std::make_shared<Plugins::MPEG4::MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ActivateVideoDurationTestTrack(*plugin, track, MimeType::VIDEO_HEVC);
    plugin->dataSource_ = source;
    return plugin;
}

static void ExpectReadSampleAnnexb(Plugins::MPEG4::MPEG4DemuxerPlugin* plugin,
    const std::vector<uint8_t>& expected)
{
    ASSERT_NE(plugin, nullptr);
    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), static_cast<int32_t>(expected.size()));
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()));
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareVideoCttsTestTrack(
    Plugins::MPEG4::MPEG4DemuxerPlugin& plugin, const std::vector<uint8_t>& sampleData)
{
    constexpr uint32_t testVideoTrackId = 0;
    constexpr int32_t testTrackTimeScale = 90000;
    constexpr uint32_t testSttsDelta = 3600;
    constexpr uint32_t testSampleCount = 2;
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr || sampleData.size() != testSampleCount) {
        return nullptr;
    }
    helper->trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper->mimeType_ = MimeType::VIDEO_AVC;
    helper->sttsEntry_ = {{testSampleCount, testSttsDelta}};
    helper->cttsEntry_ = {{testSampleCount, 0}};
    helper->stscEntry_ = {{0, testSampleCount, 1}};
    helper->stssEntry_ = {{0}};
    helper->sampleSizes_ = {1, 1};
    helper->sampleSizeCount_ = testSampleCount;
    helper->chunkOffsets_ = {0};
    if (helper->FinalizeSampleTables() != Status::OK ||
        helper->AddDurationOverride(testSampleCount - 1, testSttsDelta) != Status::OK) {
        return nullptr;
    }
    track->trackIndex = testVideoTrackId;
    track->trackId = static_cast<int32_t>(testVideoTrackId);
    track->timeScale = testTrackTimeScale;
    track->codecParms.trackType = Plugins::MPEG4::VIDEO_TYPE;
    track->sampleHelper = helper;
    plugin.firstTrack_ = track;
    plugin.lastTrack_ = track;
    plugin.mediaInfo_.tracks.resize(1);
    plugin.mediaInfo_.tracks[testVideoTrackId].Set<Tag::MIME_TYPE>(MimeType::VIDEO_AVC);
    plugin.trackMtx_[testVideoTrackId] = std::make_shared<std::mutex>();
    plugin.inited_ = true;
    plugin.seekable_ = Seekable::SEEKABLE;
    plugin.selectedTrackIds_ = {testVideoTrackId};
    return track;
}

struct VideoDurationTrackConfig {
    std::vector<uint8_t> sampleData;
    std::string mime;
    std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry> sttsEntries;
    bool hasCtts = false;
    int32_t trackTimeScale = 0;
};

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> CreateVideoDurationTestTrack(
    const VideoDurationTrackConfig& config, uint32_t testSampleCount)
{
    constexpr uint32_t testVideoTrackId = 0;
    if (testSampleCount == 0 || config.trackTimeScale <= 0) {
        return nullptr;
    }
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr) {
        return nullptr;
    }
    helper->trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper->mimeType_ = config.mime;
    helper->sttsEntry_ = config.sttsEntries;
    if (config.hasCtts) {
        helper->cttsEntry_ = {{testSampleCount, 0}};
    }
    helper->stscEntry_ = {{0, testSampleCount, 1}};
    helper->stssEntry_ = {{0}};
    helper->sampleSizes_.assign(testSampleCount, 1);
    helper->sampleSizeCount_ = testSampleCount;
    helper->chunkOffsets_ = {0};
    if (helper->FinalizeSampleTables() != Status::OK) {
        return nullptr;
    }
    track->trackIndex = testVideoTrackId;
    track->trackId = static_cast<int32_t>(testVideoTrackId);
    track->timeScale = config.trackTimeScale;
    track->codecParms.trackType = Plugins::MPEG4::VIDEO_TYPE;
    track->sampleHelper = helper;
    return track;
}

void ActivateVideoDurationTestTrack(Plugins::MPEG4::MPEG4DemuxerPlugin& plugin,
    const std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track>& track, const std::string& mime)
{
    constexpr uint32_t testVideoTrackId = 0;
    plugin.firstTrack_ = track;
    plugin.lastTrack_ = track;
    plugin.mediaInfo_.tracks.resize(1);
    plugin.mediaInfo_.tracks[testVideoTrackId].Set<Tag::MIME_TYPE>(mime);
    plugin.trackMtx_[testVideoTrackId] = std::make_shared<std::mutex>();
    plugin.inited_ = true;
    plugin.seekable_ = Seekable::SEEKABLE;
    plugin.selectedTrackIds_ = {testVideoTrackId};
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareVideoDurationTestTrack(
    Plugins::MPEG4::MPEG4DemuxerPlugin& plugin, const VideoDurationTrackConfig& config)
{
    if (config.sampleData.empty() || config.trackTimeScale <= 0) {
        return nullptr;
    }
    auto track = CreateVideoDurationTestTrack(config,
        static_cast<uint32_t>(config.sampleData.size()));
    if (track == nullptr) {
        return nullptr;
    }
    ActivateVideoDurationTestTrack(plugin, track, config.mime);
    return track;
}

std::vector<int64_t> ReadDurationsWithRFrameRate(const std::vector<uint8_t>& sampleData,
    const std::string& mime, const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry>& sttsEntries,
    uint32_t rateNum, uint32_t rateDen)
{
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    VideoDurationTrackConfig config {sampleData, mime, sttsEntries, false, 90000};
    auto track = PrepareVideoDurationTestTrack(plugin, config);
    if (track == nullptr) {
        return {};
    }
    track->rFrameRateParsed = true;
    track->rFrameRateNum = rateNum;
    track->rFrameRateDen = rateDen;
    plugin.dataSource_ = CreateDataSourceForBytes(sampleData);
    if (plugin.dataSource_ == nullptr) {
        return {};
    }
    std::vector<int64_t> durations;
    for (size_t i = 0; i < sampleData.size(); ++i) {
        AVBufferWrapper output(VIDEO_DURATION_READ_BUFFER_SIZE);
        if (plugin.ReadSample(0, output.mediaAVBuffer) != Status::OK) {
            return {};
        }
        durations.push_back(output.mediaAVBuffer->duration_);
    }
    return durations;
}

const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry> RFPS_LINE61_STTS = {
    {4, 3040}, {1, 3039}, {24, 3040}, {1, 3039}, {24, 3040}, {1, 3039}, {24, 3040}, {1, 3039},
    {23, 3040}, {1, 3039}, {25, 3040}, {1, 3039}, {23, 3040}, {1, 3039}, {24, 3040}, {1, 3039},
    {23, 3040}, {1, 3039}, {1, 3041}, {1, 3039}, {23, 3040}, {1, 3039}, {22, 3040}, {1, 3039},
    {1, 3041}, {1, 3039}, {23, 3040}, {1, 3039}, {24, 3040}, {1, 3039}, {14, 3040}, {1, 3000},
};
const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry> RFPS_H265_L4_STTS = {
    {19, 1}, {1, 30011}, {9, 1}, {1, 37528}, {9, 1}, {1, 37529}, {9, 1}, {1, 37528},
    {9, 1}, {1, 37529}, {9, 1}, {1, 37528}, {9, 1}, {1, 37529}, {9, 1}, {1, 37528},
    {9, 1}, {1, 37529}, {9, 1}, {1, 37528}, {9, 1}, {1, 37529}, {9, 1}, {1, 37528},
    {9, 1}, {1, 41282}, {9, 1}, {1, 33775}, {9, 1}, {1, 37529}, {9, 1}, {1, 37528},
    {9, 1}, {1, 37529},
};
const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry> RFPS_AVC_HIGH_STTS = {{60, 512}};

std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry> BuildLine77Stts()
{
    std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry> stts = {{1, 0}, {2, 3031}, {1, 3032}};
    for (uint32_t i = 0; i < RFPS_LINE77_STTS_EXTEND_COUNT; ++i) {
        stts.push_back({4, 3031});
        stts.push_back({1, 3032});
    }
    stts.push_back({4, 3031});
    stts.push_back({1, 3032});
    stts.push_back({1, 3031});
    stts.push_back({1, 3000});
    return stts;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> BuildRFrameRateTestTrack(const std::string& mime,
    int32_t timeScale, const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry>& stts)
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr) {
        return nullptr;
    }
    helper->trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper->mimeType_ = mime;
    helper->sttsEntry_ = stts;
    track->timeScale = timeScale;
    track->codecParms.trackType = Plugins::MPEG4::VIDEO_TYPE;
    track->sampleHelper = helper;
    return track;
}

struct FrameSampleDigest {
    int32_t actualSize = 0;
    int64_t pts = 0;
    int64_t dts = 0;
    int64_t duration = 0;
    uint32_t flag = 0;
    std::vector<uint8_t> payload;
};

class InvalidDataReferenceParser final : public RefParser {
public:
    Status RefParserInit(std::vector<uint32_t>&) override { return Status::OK; }
    Status ParserNalUnits(uint8_t*, int32_t, uint32_t, int64_t) override
    {
        ++parserNalUnitsCalls;
        return Status::ERROR_INVALID_DATA;
    }
    Status ParserExtraData(uint8_t*, int32_t) override { return Status::OK; }
    Status ParserSdtpData(uint8_t*, int32_t) override { return Status::OK; }
    Status GetFrameLayerInfo(uint32_t, FrameLayerInfo&) override { return Status::ERROR_AGAIN; }
    Status GetFrameLayerInfo(int64_t, FrameLayerInfo&) override { return Status::ERROR_AGAIN; }
    Status GetGopLayerInfo(uint32_t, GopLayerInfo&) override { return Status::ERROR_AGAIN; }
    uint32_t parserNalUnitsCalls = 0;
};

void WriteU32(std::vector<uint8_t>& data, size_t offset, uint32_t value)
{
    constexpr size_t byteIndex0 = 0;
    constexpr size_t byteIndex1 = 1;
    constexpr size_t byteIndex2 = 2;
    constexpr size_t byteIndex3 = 3;
    constexpr uint32_t byte0Shift = 24;
    constexpr uint32_t byte1Shift = 16;
    constexpr uint32_t byte2Shift = 8;

    data[offset + byteIndex0] = static_cast<uint8_t>((value >> byte0Shift) & 0xFF);
    data[offset + byteIndex1] = static_cast<uint8_t>((value >> byte1Shift) & 0xFF);
    data[offset + byteIndex2] = static_cast<uint8_t>((value >> byte2Shift) & 0xFF);
    data[offset + byteIndex3] = static_cast<uint8_t>(value & 0xFF);
}

std::shared_ptr<Plugins::MPEG4::MPEG4Sample> MakeWebVttCueSample(
    int64_t pts, int64_t duration, const std::string& payload)
{
    constexpr size_t boxHeaderSize = 8;
    constexpr size_t boxTypeOffset = sizeof(uint32_t);
    uint32_t payloadBoxSize = static_cast<uint32_t>(boxHeaderSize + payload.size());
    uint32_t cueBoxSize = static_cast<uint32_t>(boxHeaderSize + payloadBoxSize);
    std::vector<uint8_t> sampleData(cueBoxSize + 1, 0);
    WriteU32(sampleData, 0, cueBoxSize);
    WriteU32(sampleData, boxTypeOffset, Plugins::MPEG4::FourccType("vttc"));
    WriteU32(sampleData, boxHeaderSize, payloadBoxSize);
    WriteU32(sampleData, boxHeaderSize + boxTypeOffset, Plugins::MPEG4::FourccType("payl"));
    std::copy(payload.begin(), payload.end(), sampleData.begin() + boxHeaderSize + boxHeaderSize);

    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    sample->pts = pts;
    sample->dts = pts;
    sample->duration = duration;
    sample->flag = Plugins::MPEG4::Sample::SYNC_FRAME;
    sample->size = static_cast<int32_t>(cueBoxSize);
    sample->data = std::make_unique<uint8_t[]>(sampleData.size());
    std::copy(sampleData.begin(), sampleData.end(), sample->data.get());

    auto mpeg4Sample = std::make_shared<Plugins::MPEG4::MPEG4Sample>();
    mpeg4Sample->sample = sample;
    return mpeg4Sample;
}

std::shared_ptr<Plugins::MPEG4::MPEG4Sample> MakePendingWebVttSample(int64_t pts, int64_t duration)
{
    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    sample->pts = pts;
    sample->dts = pts;
    sample->duration = duration;
    sample->flag = Plugins::MPEG4::Sample::SYNC_FRAME;
    sample->size = 1;
    auto mpeg4Sample = std::make_shared<Plugins::MPEG4::MPEG4Sample>();
    mpeg4Sample->sample = sample;
    return mpeg4Sample;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> MakeWebVttTrack(uint32_t trackIndex, int64_t duration)
{
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    track->trackIndex = trackIndex;
    track->duration = duration;
    track->cache = std::make_shared<Plugins::MPEG4::SampleCache>();
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    track->sampleHelper->mimeType_ = MimeType::TEXT_WEBVTT;
    return track;
}

uint32_t FindTrackByMime(const MediaInfo& mediaInfo, const std::string& expectedMime)
{
    for (uint32_t trackId = 0; trackId < mediaInfo.tracks.size(); ++trackId) {
        std::string mime;
        if (mediaInfo.tracks[trackId].GetData(Tag::MIME_TYPE, mime) && mime == expectedMime) {
            return trackId;
        }
    }
    return INVALID_TRACK_ID;
}

uint32_t FindFirstVideoTrack(const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds)
{
    for (auto trackId : selectedTrackIds) {
        std::string mime;
        if (trackId < mediaInfo.tracks.size() &&
            mediaInfo.tracks[trackId].GetData(Tag::MIME_TYPE, mime) && mime.find("video/") == 0) {
            return trackId;
        }
    }
    return INVALID_TRACK_ID;
}

const TrackReadDigest* FindTrackDigest(const ReadDigest& digest, uint32_t trackId)
{
    for (const auto& trackDigest : digest.tracks) {
        if (trackDigest.trackId == trackId) {
            return &trackDigest;
        }
    }
    return nullptr;
}

bool SelectSourceSyncSampleByPts(
    const TrackReadDigest& digest, int64_t targetPtsUs, SeekMode seekMode, size_t& sampleIndex)
{
    int64_t previousKeyIndex = -1;
    int64_t nextKeyIndex = -1;
    for (auto keyIndex : digest.keyFrameIndexes) {
        if (keyIndex >= digest.samplePts.size()) {
            return false;
        }
        int64_t keyPts = digest.samplePts[keyIndex];
        if (keyPts <= targetPtsUs) {
            previousKeyIndex = static_cast<int64_t>(keyIndex);
        }
        if (keyPts >= targetPtsUs && nextKeyIndex < 0) {
            nextKeyIndex = static_cast<int64_t>(keyIndex);
        }
    }

    int64_t selectedKeyIndex = -1;
    if (seekMode == SeekMode::SEEK_PREVIOUS_SYNC) {
        selectedKeyIndex = previousKeyIndex;
    } else if (seekMode == SeekMode::SEEK_NEXT_SYNC) {
        selectedKeyIndex = nextKeyIndex;
    } else if (previousKeyIndex >= 0 && nextKeyIndex >= 0) {
        int64_t previousDiff = std::abs(targetPtsUs - digest.samplePts[previousKeyIndex]);
        int64_t nextDiff = std::abs(digest.samplePts[nextKeyIndex] - targetPtsUs);
        selectedKeyIndex = previousDiff <= nextDiff ? previousKeyIndex : nextKeyIndex;
    } else {
        selectedKeyIndex = previousKeyIndex >= 0 ? previousKeyIndex : nextKeyIndex;
    }
    if (selectedKeyIndex < 0) {
        return false;
    }
    sampleIndex = static_cast<size_t>(selectedKeyIndex);
    return true;
}

bool CopySamplePayload(const std::shared_ptr<MediaAVBuffer>& buffer, FrameSampleDigest& sample)
{
    if (buffer == nullptr || buffer->memory_ == nullptr) {
        ADD_FAILURE() << "Sample buffer memory is nullptr";
        return false;
    }
    sample.actualSize = buffer->memory_->GetSize();
    if (sample.actualSize <= 0 || buffer->memory_->GetAddr() == nullptr) {
        ADD_FAILURE() << "Invalid sample memory size: " << sample.actualSize;
        return false;
    }
    auto data = buffer->memory_->GetAddr();
    sample.payload.assign(data, data + sample.actualSize);
    return true;
}

bool ReadSwitchFrame(const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId, ReadMode readMode,
    FrameSampleDigest& sample)
{
    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    if (buffer.mediaAVBuffer == nullptr) {
        ADD_FAILURE() << "Failed to create switch sample buffer";
        return false;
    }
    Status ret = Status::ERROR_WAIT_TIMEOUT;
    if (readMode == ReadMode::ZERO_COPY) {
        for (int32_t attempt = 0; attempt < WAIT_RETRY_TIMES && ret == Status::ERROR_WAIT_TIMEOUT; ++attempt) {
            ret = plugin->ReadSampleZeroCopy(trackId, buffer.mediaAVBuffer,
                MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT);
        }
    } else {
        // Keep timeline parity checks deterministic; async cache races are covered by the ABA test.
        ret = plugin->ReadSample(trackId, buffer.mediaAVBuffer);
    }
    if (ret != Status::OK && ret != Status::NO_ERROR) {
        ADD_FAILURE() << "Switch sample read failed, track=" << trackId << ", status="
                      << static_cast<int32_t>(ret);
        return false;
    }
    if ((buffer.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) != 0) {
        ADD_FAILURE() << "Unexpected EOS while reading switched track " << trackId;
        return false;
    }
    sample.pts = buffer.mediaAVBuffer->pts_;
    sample.dts = buffer.mediaAVBuffer->dts_;
    sample.duration = buffer.mediaAVBuffer->duration_;
    sample.flag = buffer.mediaAVBuffer->flag_;
    return CopySamplePayload(buffer.mediaAVBuffer, sample);
}

void CompareSwitchFrames(const FrameSampleDigest& actual, const FrameSampleDigest& expected, bool compareSize)
{
    if (compareSize) {
        EXPECT_EQ(actual.actualSize, expected.actualSize);
    } else {
        EXPECT_GT(actual.actualSize, 0);
        EXPECT_GT(expected.actualSize, 0);
    }
    EXPECT_EQ(actual.pts, expected.pts);
    EXPECT_EQ(actual.dts, expected.dts);
    EXPECT_EQ(actual.duration, expected.duration);
    EXPECT_EQ(actual.flag, expected.flag);
}

template<typename ReadSampleFunc, typename IsEosFunc>
std::vector<FrameSampleDigest> ReadTrackSamples(const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
    ReadSampleFunc readSample, IsEosFunc isEos)
{
    std::vector<FrameSampleDigest> samples;
    for (size_t sampleCount = 0; sampleCount < MAX_WEBVTT_SAMPLE_READ_COUNT; ++sampleCount) {
        AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
        auto ret = readSample(plugin, trackId, buffer.mediaAVBuffer);
        if (ret == Status::END_OF_STREAM || isEos(buffer.mediaAVBuffer)) {
            return samples;
        }
        if (!(ret == Status::OK || ret == Status::NO_ERROR)) {
            ADD_FAILURE() << "ReadSample failed, trackId=" << trackId
                          << ", ret=" << static_cast<int32_t>(ret);
            return samples;
        }
        FrameSampleDigest sample;
        sample.pts = buffer.mediaAVBuffer->pts_;
        sample.dts = buffer.mediaAVBuffer->dts_;
        sample.duration = buffer.mediaAVBuffer->duration_;
        sample.flag = buffer.mediaAVBuffer->flag_;
        if (!CopySamplePayload(buffer.mediaAVBuffer, sample)) {
            return samples;
        }
        samples.push_back(std::move(sample));
    }
    ADD_FAILURE() << "ReadSample exceeded max WebVTT sample count, trackId=" << trackId;
    return samples;
}

void CompareWebVttFrameSamples(const std::vector<FrameSampleDigest>& actual,
    const std::vector<FrameSampleDigest>& expected)
{
    ASSERT_EQ(actual.size(), expected.size());
    for (size_t index = 0; index < expected.size(); ++index) {
        EXPECT_EQ(actual[index].actualSize, expected[index].actualSize);
        EXPECT_EQ(actual[index].pts, expected[index].pts);
        EXPECT_EQ(actual[index].dts, expected[index].dts);
        if (index + 1 == expected.size()) {
            EXPECT_GT(actual[index].duration, 0);
            EXPECT_LE(actual[index].duration, expected[index].duration);
        } else {
            EXPECT_EQ(actual[index].duration, expected[index].duration);
        }
        EXPECT_EQ(actual[index].flag, expected[index].flag);
        EXPECT_EQ(actual[index].payload, expected[index].payload);
    }
}

}

namespace OHOS {
namespace Media {
struct InFlightAbaTestContext {
    std::shared_ptr<MPEG4DemuxerPlugin> plugin;
    std::function<void()> armSource;
    std::function<bool()> waitUntilBlocked;
    std::function<void()> releaseSource;
    std::thread reader;
    std::atomic<bool> readContinues {false};
    uint64_t oldEpoch = 0;
    uint64_t newEpoch = 0;
    uint32_t inFlightSampleIndex = 0;
    bool hasInFlightSample = false;
    Status unselectStatus = Status::ERROR_UNKNOWN;
    Status selectStatus = Status::ERROR_UNKNOWN;
};

struct EosWaitForCacheContext {
    std::shared_ptr<MPEG4DemuxerPlugin> plugin;
    uint32_t trackId = INVALID_TRACK_ID;
    uint64_t readConfigEpoch = 0;
    std::atomic<Status> enqueueStatus {Status::ERROR_UNKNOWN};
    std::atomic<Status> duplicateEnqueueStatus {Status::ERROR_UNKNOWN};
    std::atomic<uint32_t> eosCacheFrameCount {0};
    std::atomic<Status> waitStatus {Status::ERROR_UNKNOWN};
    std::mutex gateMutex;
    std::condition_variable gateCv;
    std::thread eosWriter;
    std::thread waiter;
    bool allowEosPush = false;
    bool eosPushFinished = false;
};

void Mpeg4DemuxerPluginUnitTest::SetUpTestCase()
{
    if (g_server != nullptr) {
        g_server->StopServer();
        g_server.reset();
    }
    g_server = std::make_unique<FileServerDemo>();
    g_server->StartServer();
}

void Mpeg4DemuxerPluginUnitTest::TearDownTestCase()
{
    if (g_server == nullptr) {
        return;
    }
    g_server->StopServer();
    g_server.reset();
}

void Mpeg4DemuxerPluginUnitTest::TearDown()
{
    ResetPlugin(ffmpegPlugin_, ffmpegFd_);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
}

void Mpeg4DemuxerPluginUnitTest::ResetPlugin(std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd)
{
    if (plugin != nullptr) {
        plugin->Reset();
        plugin = nullptr;
    }
    if (fd >= 0) {
        close(fd);
        fd = -1;
    }
}

void Mpeg4DemuxerPluginUnitTest::InitPlugin(
    const std::string& filePath, const std::shared_ptr<BaseDemuxerPlugin>& plugin, int& fd, MediaInfo& mediaInfo,
    const std::shared_ptr<Meta>& configs)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    fd = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);

    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(mediaSource), Status::OK);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    ASSERT_EQ(streamDemuxer->Init(uri), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(plugin, nullptr);
    ASSERT_EQ(plugin->SetDataSource(dataSource, configs), Status::OK);
    ASSERT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitPluginURI(const std::string& filePath,
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, MediaInfo& mediaInfo)
{
    ASSERT_EQ(filePath.find(TEST_RELATIVE_PATH), 0U);
    auto uri = TEST_URI_PATH + filePath.substr(TEST_RELATIVE_PATH.size());
    auto mediaSource = std::make_shared<MediaSource>(uri);
    auto streamDemuxer = std::make_shared<StreamDemuxer>();
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(mediaSource), Status::OK);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    ASSERT_EQ(streamDemuxer->Init(uri), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(plugin, nullptr);
    ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkPlugin(
    const std::string& filePath, const WeakNetworkPluginContext& context, int64_t failOffset, size_t maxFailCount)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    context.fd = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(context.fd, 0);

    auto uri = "fd://" + std::to_string(context.fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    auto cb = std::make_shared<Mpeg4SourceCallback>(demuxerPluginManager);
    auto source = std::make_shared<Source>();
    source->SetCallback(cb);
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);

    std::shared_ptr<BaseStreamDemuxer> streamDemuxer =
        std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(failOffset, maxFailCount);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    ASSERT_NE(context.plugin, nullptr);
    ASSERT_EQ(context.plugin->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(context.plugin->GetMediaInfo(context.mediaInfo), Status::OK);
}

void Mpeg4DemuxerPluginUnitTest::InitFfmpegPlugin(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    InitPlugin(filePath, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitFfmpegPluginURI(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    InitPluginURI(filePath, ffmpegPlugin_, ffmpegMediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitMpeg4Plugin(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(MPEG4_PLUGIN_NAME);
    auto mpeg4Plugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(mpeg4Plugin);
    InitPlugin(filePath, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitMpeg4PluginURI(const std::string& filePath)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(MPEG4_PLUGIN_NAME);
    auto mpeg4Plugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(mpeg4Plugin);
    InitPluginURI(filePath, mpeg4Plugin_, mpeg4MediaInfo_);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkFfmpegPlugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
    auto ffmpegPlugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    ffmpegPlugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(ffmpegPlugin);
    WeakNetworkPluginContext context {ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_};
    InitWeakNetworkPlugin(filePath, context, failOffset, maxFailCount);
}

void Mpeg4DemuxerPluginUnitTest::InitWeakNetworkMpeg4Plugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(MPEG4_PLUGIN_NAME);
    auto mpeg4Plugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(mpeg4Plugin);
    WeakNetworkPluginContext context {mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_};
    InitWeakNetworkPlugin(filePath, context, failOffset, maxFailCount);
}

void Mpeg4DemuxerPluginUnitTest::InitDeferredWeakNetworkMpeg4Plugin(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount, uint32_t failDelayMs)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    mpeg4Fd_ = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(mpeg4Fd_, 0);

    auto uri = "fd://" + std::to_string(mpeg4Fd_) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    auto cb = std::make_shared<Mpeg4SourceCallback>(demuxerPluginManager);
    auto source = std::make_shared<Source>();
    source->SetCallback(cb);
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);

    auto streamDemuxer = std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(0, 0);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(MPEG4_PLUGIN_NAME);
    auto mpeg4Plugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(mpeg4Plugin);
    ASSERT_EQ(mpeg4Plugin_->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->GetMediaInfo(mpeg4MediaInfo_), Status::OK);
    streamDemuxer->ResetFailState();
    streamDemuxer->SetFailDelay(failDelayMs);
    streamDemuxer->SetFailParams(static_cast<size_t>(failOffset), maxFailCount);
}

bool Mpeg4DemuxerPluginUnitTest::IsSupportedTrack(const Meta& meta) const
{
    Plugins::MediaType trackType = Plugins::MediaType::UNKNOWN;
    if (!meta.Get<Tag::MEDIA_TYPE>(trackType)) {
        return false;
    }
    std::vector<uint8_t> cover;
    if (meta.Get<Tag::MEDIA_COVER>(cover)) {
        return false;
    }
    std::string mime;
    if (meta.Get<Tag::MIME_TYPE>(mime) && mime.find("image/") == 0) {
        return false;
    }
    return trackType == Plugins::MediaType::VIDEO || trackType == Plugins::MediaType::AUDIO ||
        trackType == Plugins::MediaType::SUBTITLE || trackType == Plugins::MediaType::TIMEDMETA;
}

bool Mpeg4DemuxerPluginUnitTest::IsEos(const std::shared_ptr<MediaAVBuffer>& buffer) const
{
    return buffer != nullptr && (buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) != 0;
}

bool Mpeg4DemuxerPluginUnitTest::IsSyncFrame(const std::shared_ptr<MediaAVBuffer>& buffer) const
{
    return buffer != nullptr && (buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0;
}

std::vector<uint32_t> Mpeg4DemuxerPluginUnitTest::SelectSupportedTracks(
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, const MediaInfo& mediaInfo)
{
    std::vector<uint32_t> selectedTrackIds;
    for (uint32_t trackId = 0; trackId < mediaInfo.tracks.size(); ++trackId) {
        if (!IsSupportedTrack(mediaInfo.tracks[trackId])) {
            continue;
        }
        auto ret = plugin->SelectTrack(trackId);
        EXPECT_EQ(ret, Status::OK);
        if (ret == Status::OK) {
            selectedTrackIds.push_back(trackId);
        }
    }
    EXPECT_FALSE(selectedTrackIds.empty());
    return selectedTrackIds;
}

Status Mpeg4DemuxerPluginUnitTest::ReadSampleWithRetry(
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
    const std::shared_ptr<MediaAVBuffer>& buffer, ReadMode readMode)
{
    Status ret = Status::OK;
    for (int32_t retry = 0; retry < WAIT_RETRY_TIMES; ++retry) {
        ret = (readMode == ReadMode::ZERO_COPY) ? plugin->ReadSampleZeroCopy(trackId, buffer,
            MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT) : plugin->ReadSample(trackId, buffer,
            MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT);
        if (ret != Status::ERROR_WAIT_TIMEOUT) {
            return ret;
        }
    }
    return ret;
}

Status Mpeg4DemuxerPluginUnitTest::GetNextSampleSizeWithRetry(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    uint32_t trackId, int32_t& sampleSize)
{
    Status ret = Status::OK;
    for (int32_t retry = 0; retry < WAIT_RETRY_TIMES; ++retry) {
        ret = plugin->GetNextSampleSize(trackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT);
        if (ret != Status::ERROR_WAIT_TIMEOUT) {
            return ret;
        }
    }
    return ret;
}

void Mpeg4DemuxerPluginUnitTest::ReadAndCompareSwitchSample(uint32_t trackId, ReadMode readMode)
{
    FrameSampleDigest ffmpegSample;
    FrameSampleDigest mpeg4Sample;
    ASSERT_TRUE(ReadSwitchFrame(ffmpegPlugin_, trackId, readMode, ffmpegSample));
    ASSERT_TRUE(ReadSwitchFrame(mpeg4Plugin_, trackId, readMode, mpeg4Sample));
    std::string mime;
    bool hasMime = trackId < mpeg4MediaInfo_.tracks.size() &&
        mpeg4MediaInfo_.tracks[trackId].GetData(Tag::MIME_TYPE, mime);
    bool compareSize = !hasMime || mime.find("video/") != 0;
    CompareSwitchFrames(mpeg4Sample, ffmpegSample, compareSize);
}

void Mpeg4DemuxerPluginUnitTest::PrepareDynamicTrackSwitchPlugins()
{
    ffmpegPlugin_ = std::make_shared<FFmpegDemuxerPlugin>(FFMPEG_MP4_PLUGIN_NAME);
    mpeg4Plugin_ = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(ffmpegPlugin_, nullptr);
    ASSERT_NE(mpeg4Plugin_, nullptr);
    InitPlugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
    InitPlugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
    ASSERT_GE(ffmpegMediaInfo_.tracks.size(), DYNAMIC_SWITCH_REQUIRED_TRACK_COUNT);
    ASSERT_GE(mpeg4MediaInfo_.tracks.size(), DYNAMIC_SWITCH_REQUIRED_TRACK_COUNT);
}

void Mpeg4DemuxerPluginUnitTest::SelectInitialDynamicSwitchTracks()
{
    for (uint32_t trackId : {DYNAMIC_SWITCH_PRIMARY_TRACK_ID, DYNAMIC_SWITCH_SECONDARY_TRACK_ID}) {
        ASSERT_EQ(ffmpegPlugin_->SelectTrack(trackId), Status::OK);
        ASSERT_EQ(mpeg4Plugin_->SelectTrack(trackId), Status::OK);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckDynamicTrackSwitch(DynamicSwitchScenario scenario, ReadMode readMode)
{
    ASSERT_NO_FATAL_FAILURE(PrepareDynamicTrackSwitchPlugins());
    ASSERT_NO_FATAL_FAILURE(SelectInitialDynamicSwitchTracks());
    ASSERT_NO_FATAL_FAILURE(ReadDynamicSwitchPreroll(scenario, readMode));

    int64_t lastMpeg4Dts = INT64_MIN;
    ASSERT_NO_FATAL_FAILURE(SwitchAndCompareDynamicTrack(readMode, lastMpeg4Dts));
    ASSERT_NO_FATAL_FAILURE(ReselectAndCompareDynamicTrack(readMode, lastMpeg4Dts));
}

void Mpeg4DemuxerPluginUnitTest::ReadDynamicSwitchPreroll(DynamicSwitchScenario scenario, ReadMode readMode)
{
    if (scenario == DynamicSwitchScenario::GET_NEXT_SIZE) {
        int32_t ffmpegSize = 0;
        int32_t mpeg4Size = 0;
        ASSERT_EQ(ffmpegPlugin_->GetNextSampleSize(DYNAMIC_SWITCH_PRIMARY_TRACK_ID, ffmpegSize), Status::OK);
        ASSERT_EQ(mpeg4Plugin_->GetNextSampleSize(DYNAMIC_SWITCH_PRIMARY_TRACK_ID, mpeg4Size), Status::OK);
        EXPECT_GT(mpeg4Size, 0);
        EXPECT_GT(ffmpegSize, 0);
    } else {
        for (uint32_t index = 0; index < DYNAMIC_SWITCH_PREROLL_SAMPLE_COUNT; ++index) {
            for (uint32_t trackId : {DYNAMIC_SWITCH_PRIMARY_TRACK_ID, DYNAMIC_SWITCH_SECONDARY_TRACK_ID}) {
                FrameSampleDigest ignoredFfmpeg;
                FrameSampleDigest ignoredMpeg4;
                ASSERT_TRUE(ReadSwitchFrame(ffmpegPlugin_, trackId, readMode, ignoredFfmpeg));
                ASSERT_TRUE(ReadSwitchFrame(mpeg4Plugin_, trackId, readMode, ignoredMpeg4));
            }
        }
        if (scenario == DynamicSwitchScenario::PLAYBACK_SEEK) {
            int64_t ffmpegRealSeekTime = 0;
            int64_t mpeg4RealSeekTime = 0;
            ASSERT_EQ(ffmpegPlugin_->SeekTo(DYNAMIC_SWITCH_PRIMARY_TRACK_ID, DYNAMIC_SWITCH_SEEK_TIME_MS,
                SeekMode::SEEK_PREVIOUS_SYNC, ffmpegRealSeekTime), Status::OK);
            ASSERT_EQ(mpeg4Plugin_->SeekTo(DYNAMIC_SWITCH_PRIMARY_TRACK_ID, DYNAMIC_SWITCH_SEEK_TIME_MS,
                SeekMode::SEEK_PREVIOUS_SYNC, mpeg4RealSeekTime), Status::OK);
        } else if (scenario == DynamicSwitchScenario::FLUSH) {
            ASSERT_EQ(ffmpegPlugin_->Flush(), Status::OK);
            ASSERT_EQ(mpeg4Plugin_->Flush(), Status::OK);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::SwitchAndCompareDynamicTrack(ReadMode readMode, int64_t& lastMpeg4Dts)
{
    ASSERT_EQ(ffmpegPlugin_->UnselectTrack(DYNAMIC_SWITCH_PRIMARY_TRACK_ID), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->UnselectTrack(DYNAMIC_SWITCH_PRIMARY_TRACK_ID), Status::OK);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    for (uint32_t index = 0; index < DYNAMIC_SWITCH_COMPARE_SAMPLE_COUNT; ++index) {
        SCOPED_TRACE("after-switch sample=" + std::to_string(index));
        FrameSampleDigest ffmpegSample;
        FrameSampleDigest mpeg4Sample;
        ASSERT_TRUE(ReadSwitchFrame(ffmpegPlugin_, DYNAMIC_SWITCH_NEW_TRACK_ID, readMode, ffmpegSample));
        ASSERT_TRUE(ReadSwitchFrame(mpeg4Plugin_, DYNAMIC_SWITCH_NEW_TRACK_ID, readMode, mpeg4Sample));
        CompareSwitchFrames(mpeg4Sample, ffmpegSample, true);
        lastMpeg4Dts = mpeg4Sample.dts;
    }
}

void Mpeg4DemuxerPluginUnitTest::ReselectAndCompareDynamicTrack(ReadMode readMode, int64_t& lastMpeg4Dts)
{
    ASSERT_EQ(ffmpegPlugin_->UnselectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->UnselectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    for (uint32_t index = 0; index < DYNAMIC_SWITCH_RESELECT_SAMPLE_COUNT; ++index) {
        SCOPED_TRACE("after-reselect sample=" + std::to_string(index));
        FrameSampleDigest ffmpegSample;
        FrameSampleDigest mpeg4Sample;
        ASSERT_TRUE(ReadSwitchFrame(ffmpegPlugin_, DYNAMIC_SWITCH_NEW_TRACK_ID, readMode, ffmpegSample));
        ASSERT_TRUE(ReadSwitchFrame(mpeg4Plugin_, DYNAMIC_SWITCH_NEW_TRACK_ID, readMode, mpeg4Sample));
        if (readMode == ReadMode::ZERO_COPY) {
            // Independent async readers may prefetch different amounts before reselect. Verify
            // the new selection advances past this plugin's previous delivered packet.
            EXPECT_GT(mpeg4Sample.dts, lastMpeg4Dts);
            EXPECT_GT(mpeg4Sample.actualSize, 0);
            lastMpeg4Dts = mpeg4Sample.dts;
            continue;
        }
        CompareSwitchFrames(mpeg4Sample, ffmpegSample, true);
        lastMpeg4Dts = mpeg4Sample.dts;
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckDynamicTrackSwitchInFlightAba()
{
    InFlightAbaTestContext context;
    ASSERT_NO_FATAL_FAILURE(PrepareInFlightAbaTest(context));
    ASSERT_NO_FATAL_FAILURE(RunInFlightAbaRace(context));
}

void Mpeg4DemuxerPluginUnitTest::PrepareInFlightAbaTest(InFlightAbaTestContext& context)
{
    auto data = ReadFileBytes(TEST_MP4_MULTIPLE_VIDEO_TRACKS);
    ASSERT_FALSE(data.empty());
    auto source = std::make_shared<BlockingReadAtDataSource>(std::move(data));
    context.armSource = [source] { source->Arm(); };
    context.waitUntilBlocked = [source] { return source->WaitUntilBlocked(); };
    context.releaseSource = [source] { source->Release(); };
    context.plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(context.plugin, nullptr);
    ASSERT_EQ(context.plugin->SetDataSource(source), Status::OK);
    MediaInfo mediaInfo;
    ASSERT_EQ(context.plugin->GetMediaInfo(mediaInfo), Status::OK);
    ASSERT_EQ(context.plugin->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID), Status::OK);
    std::lock_guard<std::mutex> lock(context.plugin->readConfigMutex_);
    context.plugin->EnterRuntimeLocked();
    context.plugin->ClearTrackCache(DYNAMIC_SWITCH_NEW_TRACK_ID);
    context.plugin->trackId_.store(DYNAMIC_SWITCH_NEW_TRACK_ID, std::memory_order_release);
}

void Mpeg4DemuxerPluginUnitTest::RunInFlightAbaRace(InFlightAbaTestContext& context)
{
    ASSERT_TRUE(context.armSource);
    ASSERT_TRUE(context.waitUntilBlocked);
    ASSERT_TRUE(context.releaseSource);
    context.armSource();
    auto plugin = context.plugin;
    auto* readContinues = &context.readContinues;
    context.reader = std::thread([plugin, readContinues] {
        readContinues->store(plugin->ReadAndCacheSample(), std::memory_order_release);
    });
    if (!context.waitUntilBlocked()) {
        context.releaseSource();
        context.reader.join();
        FAIL() << "Reader did not reach the armed DataSource::ReadAt";
        return;
    }

    {
        std::lock_guard<std::mutex> lock(context.plugin->readConfigMutex_);
        context.oldEpoch = context.plugin->readConfigEpoch_;
        context.hasInFlightSample = context.plugin->hasInFlightSample_;
        if (context.hasInFlightSample) {
            context.inFlightSampleIndex = context.plugin->inFlightSampleInfo_.sampleIndex;
        }
    }
    if (!context.hasInFlightSample) {
        ReleaseInFlightAbaRead(context);
        FAIL() << "Blocked read has no in-flight sample marker";
        return;
    }

    context.unselectStatus = context.plugin->UnselectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID);
    context.selectStatus = context.plugin->SelectTrack(DYNAMIC_SWITCH_NEW_TRACK_ID);
    {
        std::lock_guard<std::mutex> lock(context.plugin->readConfigMutex_);
        context.newEpoch = context.plugin->readConfigEpoch_;
        auto track = context.plugin->FindTrackById(DYNAMIC_SWITCH_NEW_TRACK_ID);
        EXPECT_NE(track, nullptr);
        if (track != nullptr) {
            EXPECT_GT(track->currentSampleIndex, context.inFlightSampleIndex);
        }
    }
    ReleaseInFlightAbaRead(context);
    VerifyInFlightAbaResult(context);
}

void Mpeg4DemuxerPluginUnitTest::ReleaseInFlightAbaRead(InFlightAbaTestContext& context)
{
    if (context.releaseSource) {
        context.releaseSource();
    }
    if (context.reader.joinable()) {
        context.reader.join();
    }
}

void Mpeg4DemuxerPluginUnitTest::VerifyInFlightAbaResult(InFlightAbaTestContext& context)
{
    EXPECT_EQ(context.unselectStatus, Status::OK);
    EXPECT_EQ(context.selectStatus, Status::OK);
    EXPECT_TRUE(context.readContinues.load(std::memory_order_acquire));
    EXPECT_GT(context.newEpoch, context.oldEpoch);
    EXPECT_FALSE(context.plugin->HasTrackCache(DYNAMIC_SWITCH_NEW_TRACK_ID));
    EXPECT_TRUE(context.plugin->ReadAndCacheSample());
    EXPECT_TRUE(context.plugin->HasTrackCache(DYNAMIC_SWITCH_NEW_TRACK_ID));
    auto cachedSample = context.plugin->FrontTrackCache(DYNAMIC_SWITCH_NEW_TRACK_ID);
    ASSERT_NE(cachedSample, nullptr);
    ASSERT_NE(cachedSample->sample, nullptr);
    EXPECT_GT(cachedSample->sampleIndex, context.inFlightSampleIndex);
}

void Mpeg4DemuxerPluginUnitTest::CheckResetAndDataSourceSwitch()
{
    ffmpegPlugin_ = std::make_shared<FFmpegDemuxerPlugin>(FFMPEG_MP4_PLUGIN_NAME);
    mpeg4Plugin_ = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(ffmpegPlugin_, nullptr);
    ASSERT_NE(mpeg4Plugin_, nullptr);
    InitPlugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
    InitPlugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(DYNAMIC_SWITCH_PRIMARY_TRACK_ID), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(DYNAMIC_SWITCH_PRIMARY_TRACK_ID), Status::OK);
    ReadAndCompareSwitchSample(DYNAMIC_SWITCH_PRIMARY_TRACK_ID, ReadMode::COPY);

    ASSERT_EQ(ffmpegPlugin_->Reset(), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->Reset(), Status::OK);
    close(ffmpegFd_);
    close(mpeg4Fd_);
    ffmpegFd_ = -1;
    mpeg4Fd_ = -1;
    InitPlugin(TEST_MP4_PATH_H264_B, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
    InitPlugin(TEST_MP4_PATH_H264_B, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(DYNAMIC_SWITCH_PRIMARY_TRACK_ID), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(DYNAMIC_SWITCH_PRIMARY_TRACK_ID), Status::OK);
    for (uint32_t index = 0; index < DYNAMIC_SWITCH_COMPARE_SAMPLE_COUNT; ++index) {
        ReadAndCompareSwitchSample(DYNAMIC_SWITCH_PRIMARY_TRACK_ID, ReadMode::COPY);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekToFrameByDtsResult(
    const std::string& filePath, const std::string& testLabel)
{
    InitMpeg4Plugin(filePath);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());

    uint32_t videoTrackId = selectedTrackIds.front();
    bool hasVideoTrack = false;
    for (auto trackId : selectedTrackIds) {
        std::string mime;
        if (mpeg4MediaInfo_.tracks[trackId].Get<Tag::MIME_TYPE>(mime) && mime.find("video/") == 0) {
            videoTrackId = trackId;
            hasVideoTrack = true;
            break;
        }
    }
    ASSERT_TRUE(hasVideoTrack);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    std::vector<int64_t> targetDtsUsList;
    constexpr uint32_t readFrameCount = 40;
    constexpr uint32_t targetDtsCount = 3;
    constexpr int64_t usPerMs = 1000;
    for (uint32_t i = 0; i < readFrameCount; ++i) {
        ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, videoTrackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
        int64_t dtsUs = buffer.mediaAVBuffer->dts_;
        if (dtsUs % usPerMs != 0 ||
            std::find(targetDtsUsList.begin(), targetDtsUsList.end(), dtsUs) != targetDtsUsList.end()) {
            continue;
        }
        targetDtsUsList.push_back(dtsUs);
        std::cout << "[" << testLabel << "] source frame index: " << i <<
            ", source dts(us): " << dtsUs << std::endl;
        if (targetDtsUsList.size() == targetDtsCount) {
            break;
        }
    }
    ASSERT_EQ(targetDtsUsList.size(), targetDtsCount);

    for (auto targetDtsUs : targetDtsUsList) {
        int64_t seekTimeMs = targetDtsUs / usPerMs;
        int64_t realSeekTime = 0;
        ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(videoTrackId), seekTimeMs,
            SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
        EXPECT_EQ(realSeekTime, seekTimeMs * 1000000LL);

        ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, videoTrackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
        std::cout << "[" << testLabel << "] target dts(us): " << targetDtsUs
            << ", input seek dts(ms): " << seekTimeMs
            << ", real seek time(ns): " << realSeekTime
            << ", seeked frame dts(us): " << buffer.mediaAVBuffer->dts_ << std::endl;
        EXPECT_EQ(buffer.mediaAVBuffer->dts_, targetDtsUs);
    }
}

void Mpeg4DemuxerPluginUnitTest::InitPluginsForReadTest(const std::string& filePath, bool useUri)
{
    if (useUri) {
        InitFfmpegPluginURI(filePath);
        InitMpeg4PluginURI(filePath);
        return;
    }
    InitFfmpegPlugin(filePath);
    InitMpeg4Plugin(filePath);
}

SampleDigest Mpeg4DemuxerPluginUnitTest::ReadFirstSample(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const MediaInfo& mediaInfo, ReadMode readMode)
{
    SampleDigest digest;
    auto selectedTrackIds = SelectSupportedTracks(plugin, mediaInfo);
    if (selectedTrackIds.empty()) {
        ADD_FAILURE() << "No supported track selected";
        return digest;
    }

    digest.trackId = selectedTrackIds.front();
    mediaInfo.tracks[digest.trackId].GetData(Tag::MIME_TYPE, digest.mime);

    auto sampleSizeRet = GetNextSampleSizeWithRetry(plugin, digest.trackId, digest.nextSampleSize);
    EXPECT_EQ(sampleSizeRet, Status::OK);
    if (sampleSizeRet != Status::OK) {
        return digest;
    }
    EXPECT_GT(digest.nextSampleSize, 0);

    std::shared_ptr<MediaAVBuffer> buffer;
    std::unique_ptr<AVBufferWrapper> copyBuffer;
    if (readMode == ReadMode::ZERO_COPY) {
        buffer = MediaAVBuffer::CreateAVBuffer();
    } else {
        copyBuffer = std::make_unique<AVBufferWrapper>(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
        buffer = copyBuffer->mediaAVBuffer;
    }
    if (buffer == nullptr) {
        ADD_FAILURE() << "Create AVBuffer failed";
        return digest;
    }
    auto readRet = ReadSampleWithRetry(plugin, digest.trackId, buffer, readMode);
    EXPECT_EQ(readRet, Status::OK);
    if (readRet != Status::OK) {
        return digest;
    }
    EXPECT_FALSE(IsEos(buffer));

    digest.actualSize = buffer->memory_ != nullptr ? buffer->memory_->GetSize() : 0;
    digest.pts = buffer->pts_;
    digest.dts = buffer->dts_;
    digest.duration = buffer->duration_;
    digest.flag = buffer->flag_;
    EXPECT_GT(digest.actualSize, 0);
    return digest;
}

ReadDigest Mpeg4DemuxerPluginUnitTest::ReadAllSamples(const std::shared_ptr<BaseDemuxerPlugin>& plugin,
    const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds, ReadMode readMode)
{
    auto digest = CreateReadDigest(mediaInfo, selectedTrackIds);
    std::vector<bool> eosFlags(selectedTrackIds.size(), false);
    std::unique_ptr<AVBufferWrapper> copyBuffer;
    if (readMode == ReadMode::COPY) {
        copyBuffer = std::make_unique<AVBufferWrapper>(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    }

    while (std::find(eosFlags.begin(), eosFlags.end(), false) != eosFlags.end()) {
        for (size_t index = 0; index < selectedTrackIds.size(); ++index) {
            if (eosFlags[index]) {
                continue;
            }
            std::shared_ptr<MediaAVBuffer> buffer =
                (readMode == ReadMode::ZERO_COPY) ? MediaAVBuffer::CreateAVBuffer() : copyBuffer->mediaAVBuffer;
            bool eosReached = false;
            TrackSampleReadContext context {plugin, digest.tracks[index], buffer, readMode};
            if (!ReadTrackSample(context, eosReached)) {
                return digest;
            }
            eosFlags[index] = eosReached;
        }
    }
    return digest;
}

ReadDigest Mpeg4DemuxerPluginUnitTest::CreateReadDigest(
    const MediaInfo& mediaInfo, const std::vector<uint32_t>& selectedTrackIds) const
{
    ReadDigest digest;
    for (auto trackId : selectedTrackIds) {
        TrackReadDigest trackDigest;
        trackDigest.trackId = trackId;
        mediaInfo.tracks[trackId].GetData(Tag::MIME_TYPE, trackDigest.mime);
        digest.tracks.push_back(trackDigest);
    }
    return digest;
}

bool Mpeg4DemuxerPluginUnitTest::ReadTrackSample(const TrackSampleReadContext& context, bool& eosReached)
{
    if (context.buffer == nullptr) {
        ADD_FAILURE() << "Create AVBuffer failed";
        return false;
    }
    auto& trackDigest = context.trackDigest;
    auto ret = ReadSampleWithRetry(context.plugin, trackDigest.trackId, context.buffer, context.readMode);
    if (!(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR)) {
        ADD_FAILURE() << "ReadSample failed, trackId=" << trackDigest.trackId
                      << ", ret=" << static_cast<int32_t>(ret);
        return false;
    }
    eosReached = ret == Status::END_OF_STREAM || IsEos(context.buffer);
    if (eosReached) {
        return true;
    }
    if (trackDigest.frameCount == 0) {
        trackDigest.firstPts = context.buffer->pts_;
    }
    trackDigest.lastPts = context.buffer->pts_;
    if (IsSyncFrame(context.buffer)) {
        trackDigest.keyFrameCount++;
        trackDigest.keyFrameIndexes.push_back(static_cast<uint32_t>(trackDigest.frameCount));
    }
    if ((context.buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::DISCARD)) != 0) {
        trackDigest.discardFrameCount++;
        trackDigest.discardFrameIndexes.push_back(static_cast<uint32_t>(trackDigest.frameCount));
    }
    trackDigest.samplePts.push_back(context.buffer->pts_);
    trackDigest.sampleDts.push_back(context.buffer->dts_);
    trackDigest.sampleDurations.push_back(context.buffer->duration_);
    trackDigest.sampleDiscardFlags.push_back(
        context.buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::DISCARD));
    std::vector<uint8_t> skipInfo;
    if (context.buffer->meta_ != nullptr &&
        context.buffer->meta_->GetData(Tag::BUFFER_SKIP_SAMPLES_INFO, skipInfo)) {
        trackDigest.sampleSkipInfos.push_back(skipInfo);
    } else {
        trackDigest.sampleSkipInfos.emplace_back();
    }
    trackDigest.frameCount++;
    return true;
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekSuccessDigest(const ReadDigest& actualDigest,
    const ReadDigest& expectedDigest)
{
    ASSERT_EQ(actualDigest.tracks.size(), expectedDigest.tracks.size());
    for (size_t index = 0; index < actualDigest.tracks.size(); ++index) {
        EXPECT_EQ(actualDigest.tracks[index].mime, expectedDigest.tracks[index].mime);
        EXPECT_GT(actualDigest.tracks[index].frameCount, 0);
        EXPECT_LE(actualDigest.tracks[index].frameCount, expectedDigest.tracks[index].frameCount);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekSuccessDigestWithExpected(const ReadDigest& ffmpegDigest,
    const ReadDigest& mpeg4Digest, const std::vector<int32_t>& expectedFrames)
{
    ASSERT_EQ(ffmpegDigest.tracks.size(), expectedFrames.size());
    ASSERT_EQ(mpeg4Digest.tracks.size(), expectedFrames.size());
    for (size_t index = 0; index < expectedFrames.size(); ++index) {
        EXPECT_EQ(mpeg4Digest.tracks[index].mime, ffmpegDigest.tracks[index].mime);
        int32_t mpeg4DisplayFrameCount = mpeg4Digest.tracks[index].frameCount -
            mpeg4Digest.tracks[index].discardFrameCount;
        EXPECT_GT(mpeg4DisplayFrameCount, 0);
        EXPECT_LE(mpeg4DisplayFrameCount, expectedFrames[index]);
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekFallbackDigest(const ReadDigest& mpeg4Digest,
    const std::vector<uint32_t>& selectedTrackIds, const std::vector<int32_t>& expectedFrames)
{
    if (expectedFrames.empty()) {
        EXPECT_EQ(mpeg4Digest.tracks.size(), selectedTrackIds.size());
        return;
    }
    ASSERT_EQ(mpeg4Digest.tracks.size(), expectedFrames.size());
    for (size_t index = 0; index < expectedFrames.size(); ++index) {
        int32_t mpeg4DisplayFrameCount = mpeg4Digest.tracks[index].frameCount -
            mpeg4Digest.tracks[index].discardFrameCount;
        EXPECT_GT(mpeg4DisplayFrameCount, 0);
        EXPECT_LE(mpeg4DisplayFrameCount, expectedFrames[index]);
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareReadDigest(const ReadDigest& lhs, const ReadDigest& rhs)
{
    ASSERT_EQ(lhs.tracks.size(), rhs.tracks.size());
    for (size_t index = 0; index < lhs.tracks.size(); ++index) {
        const auto& lhsTrack = lhs.tracks[index];
        const auto& rhsTrack = rhs.tracks[index];
        EXPECT_EQ(lhsTrack.mime, rhsTrack.mime);
        EXPECT_EQ(lhsTrack.frameCount, rhsTrack.frameCount);
        EXPECT_EQ(lhsTrack.keyFrameCount, rhsTrack.keyFrameCount);
        constexpr int64_t timestampToleranceUs = 100;
        EXPECT_NEAR(lhsTrack.firstPts, rhsTrack.firstPts, timestampToleranceUs);
        EXPECT_NEAR(lhsTrack.lastPts, rhsTrack.lastPts, timestampToleranceUs);
        EXPECT_EQ(lhsTrack.keyFrameIndexes, rhsTrack.keyFrameIndexes);
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareTrackPropertiesWithFfmpeg(
    const std::vector<uint32_t>& mpeg4SelectedTrackIds, const std::vector<uint32_t>& ffmpegSelectedTrackIds,
    bool compareBitRate)
{
    ASSERT_EQ(mpeg4SelectedTrackIds.size(), ffmpegSelectedTrackIds.size());
    for (size_t index = 0; index < mpeg4SelectedTrackIds.size(); ++index) {
        SCOPED_TRACE("track index=" + std::to_string(index));
        const auto mpeg4TrackId = mpeg4SelectedTrackIds[index];
        const auto ffmpegTrackId = ffmpegSelectedTrackIds[index];
        ASSERT_LT(mpeg4TrackId, mpeg4MediaInfo_.tracks.size());
        ASSERT_LT(ffmpegTrackId, ffmpegMediaInfo_.tracks.size());
        const auto& mpeg4Track = mpeg4MediaInfo_.tracks[mpeg4TrackId];
        const auto& ffmpegTrack = ffmpegMediaInfo_.tracks[ffmpegTrackId];

        std::string mpeg4Mime;
        std::string ffmpegMime;
        EXPECT_EQ(mpeg4Track.Get<Tag::MIME_TYPE>(mpeg4Mime), ffmpegTrack.Get<Tag::MIME_TYPE>(ffmpegMime));
        EXPECT_EQ(mpeg4Mime, ffmpegMime);

        Plugins::MediaType mpeg4Type = Plugins::MediaType::UNKNOWN;
        Plugins::MediaType ffmpegType = Plugins::MediaType::UNKNOWN;
        EXPECT_EQ(mpeg4Track.Get<Tag::MEDIA_TYPE>(mpeg4Type), ffmpegTrack.Get<Tag::MEDIA_TYPE>(ffmpegType));
        EXPECT_EQ(mpeg4Type, ffmpegType);

        if (compareBitRate) {
            int64_t mpeg4BitRate = 0;
            int64_t ffmpegBitRate = 0;
            bool hasMpeg4BitRate = mpeg4Track.Get<Tag::MEDIA_BITRATE>(mpeg4BitRate);
            bool hasFfmpegBitRate = ffmpegTrack.Get<Tag::MEDIA_BITRATE>(ffmpegBitRate);
            EXPECT_EQ(hasMpeg4BitRate, hasFfmpegBitRate);
            if (hasMpeg4BitRate && hasFfmpegBitRate) {
                EXPECT_EQ(mpeg4BitRate, ffmpegBitRate);
            }
        }

        int64_t mpeg4StartTime = 0;
        int64_t ffmpegStartTime = 0;
        bool hasMpeg4StartTime = mpeg4Track.Get<Tag::MEDIA_START_TIME>(mpeg4StartTime);
        bool hasFfmpegStartTime = ffmpegTrack.Get<Tag::MEDIA_START_TIME>(ffmpegStartTime);
        EXPECT_EQ(hasMpeg4StartTime, hasFfmpegStartTime);
        if (hasMpeg4StartTime && hasFfmpegStartTime) {
            EXPECT_EQ(mpeg4StartTime, ffmpegStartTime);
        }

        if (mpeg4Type == Plugins::MediaType::VIDEO || ffmpegType == Plugins::MediaType::VIDEO) {
            int32_t mpeg4VideoDelay = 0;
            int32_t ffmpegVideoDelay = 0;
            bool hasMpeg4VideoDelay = mpeg4Track.Get<Tag::VIDEO_DELAY>(mpeg4VideoDelay);
            bool hasFfmpegVideoDelay = ffmpegTrack.Get<Tag::VIDEO_DELAY>(ffmpegVideoDelay);
            EXPECT_EQ(hasMpeg4VideoDelay, hasFfmpegVideoDelay);
            if (hasMpeg4VideoDelay && hasFfmpegVideoDelay) {
                EXPECT_EQ(mpeg4VideoDelay, ffmpegVideoDelay);
            }
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareTimelineDigest(const ReadDigest& mpeg4Digest,
    const ReadDigest& ffmpegDigest, bool compareFullTimeline, bool compareDurations)
{
    ASSERT_EQ(mpeg4Digest.tracks.size(), ffmpegDigest.tracks.size());
    for (size_t index = 0; index < mpeg4Digest.tracks.size(); ++index) {
        SCOPED_TRACE("track index=" + std::to_string(index));
        const auto& mpeg4Track = mpeg4Digest.tracks[index];
        const auto& ffmpegTrack = ffmpegDigest.tracks[index];
        size_t compareCount = compareFullTimeline ? ffmpegTrack.samplePts.size() :
            std::min(mpeg4Track.samplePts.size(), ffmpegTrack.samplePts.size());
        ASSERT_GE(mpeg4Track.samplePts.size(), compareCount);
        ASSERT_GE(ffmpegTrack.samplePts.size(), compareCount);
        EXPECT_EQ(mpeg4Track.mime, ffmpegTrack.mime);
        EXPECT_EQ(std::vector<int64_t>(mpeg4Track.samplePts.begin(), mpeg4Track.samplePts.begin() + compareCount),
            std::vector<int64_t>(ffmpegTrack.samplePts.begin(), ffmpegTrack.samplePts.begin() + compareCount));
        EXPECT_EQ(std::vector<int64_t>(mpeg4Track.sampleDts.begin(), mpeg4Track.sampleDts.begin() + compareCount),
            std::vector<int64_t>(ffmpegTrack.sampleDts.begin(), ffmpegTrack.sampleDts.begin() + compareCount));
        if (compareDurations) {
            EXPECT_EQ(std::vector<int64_t>(mpeg4Track.sampleDurations.begin(),
                mpeg4Track.sampleDurations.begin() + compareCount),
                std::vector<int64_t>(ffmpegTrack.sampleDurations.begin(),
                    ffmpegTrack.sampleDurations.begin() + compareCount));
        }
        if (compareFullTimeline) {
            EXPECT_EQ(mpeg4Track.discardFrameCount, ffmpegTrack.discardFrameCount);
            EXPECT_EQ(mpeg4Track.discardFrameIndexes, ffmpegTrack.discardFrameIndexes);
        }
        EXPECT_EQ(std::vector<uint32_t>(mpeg4Track.sampleDiscardFlags.begin(),
            mpeg4Track.sampleDiscardFlags.begin() + compareCount),
            std::vector<uint32_t>(ffmpegTrack.sampleDiscardFlags.begin(),
                ffmpegTrack.sampleDiscardFlags.begin() + compareCount));
        EXPECT_EQ(std::vector<std::vector<uint8_t>>(mpeg4Track.sampleSkipInfos.begin(),
            mpeg4Track.sampleSkipInfos.begin() + compareCount),
            std::vector<std::vector<uint8_t>>(ffmpegTrack.sampleSkipInfos.begin(),
                ffmpegTrack.sampleSkipInfos.begin() + compareCount));
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareMovFixIndexResultWithFfmpeg(const std::string& filePath,
    bool compareBitRate, bool compareFullTimeline, bool compareDurations)
{
    ResetPlugin(ffmpegPlugin_, ffmpegFd_);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    SCOPED_TRACE(filePath);
    InitFfmpegPlugin(filePath);
    InitMpeg4Plugin(filePath);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(mpeg4SelectedTracks.size(), ffmpegSelectedTracks.size());
    CompareTrackPropertiesWithFfmpeg(mpeg4SelectedTracks, ffmpegSelectedTracks, compareBitRate);

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    if (compareFullTimeline) {
        CompareReadDigest(mpeg4Digest, ffmpegDigest);
    } else {
        ASSERT_FALSE(mpeg4Digest.tracks.empty());
        for (const auto& trackDigest : mpeg4Digest.tracks) {
            EXPECT_GT(trackDigest.frameCount, 0);
        }
    }
    CompareTimelineDigest(mpeg4Digest, ffmpegDigest, compareFullTimeline, compareDurations);

    ResetPlugin(ffmpegPlugin_, ffmpegFd_);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
}

void Mpeg4DemuxerPluginUnitTest::CheckReadResultWithExpected(const std::string& filePath,
    const std::vector<int32_t>& expectedFrames, const std::vector<int32_t>& expectedKeyFrames,
    const std::vector<uint32_t>& expectedKeyFrameIndexes, bool useUri)
{
    if (useUri) {
        InitMpeg4PluginURI(filePath);
    } else {
        InitMpeg4Plugin(filePath);
    }
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    auto digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, selectedTrackIds);
    CheckReadDigestWithExpected(digest, expectedFrames, expectedKeyFrames, expectedKeyFrameIndexes);
}

void Mpeg4DemuxerPluginUnitTest::CheckReadDigestWithExpected(const ReadDigest& digest,
    const std::vector<int32_t>& expectedFrames, const std::vector<int32_t>& expectedKeyFrames,
    const std::vector<uint32_t>& expectedKeyFrameIndexes)
{
    ASSERT_EQ(digest.tracks.size(), expectedFrames.size());
    ASSERT_EQ(digest.tracks.size(), expectedKeyFrames.size());
    for (size_t index = 0; index < digest.tracks.size(); ++index) {
        EXPECT_EQ(digest.tracks[index].frameCount, expectedFrames[index]);
        EXPECT_EQ(digest.tracks[index].keyFrameCount, expectedKeyFrames[index]);
        if (index == 0) {
            EXPECT_EQ(digest.tracks[index].keyFrameIndexes, expectedKeyFrameIndexes);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareSampleDigest(const SampleDigest& lhs, const SampleDigest& rhs)
{
    EXPECT_EQ(lhs.mime, rhs.mime);
    if (lhs.mime.find("video/") == 0 &&
        (lhs.flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) != 0) {
        // FFmpeg prepends codec config on every sync frame while MPEG4Demuxer only prepends it on the first one.
        // The zero-copy test validates decodable sample attributes rather than requiring identical advertised size.
        EXPECT_GT(lhs.nextSampleSize, 0);
        EXPECT_GT(rhs.nextSampleSize, 0);
        EXPECT_LE(lhs.actualSize, rhs.nextSampleSize);
        EXPECT_LE(rhs.actualSize, lhs.nextSampleSize);
    } else {
        EXPECT_EQ(lhs.nextSampleSize, rhs.nextSampleSize);
    }
    EXPECT_EQ(lhs.actualSize, rhs.actualSize);
    EXPECT_EQ(lhs.pts, rhs.pts);
    EXPECT_EQ(lhs.dts, rhs.dts);
    EXPECT_EQ(lhs.duration, rhs.duration);
    EXPECT_EQ(lhs.flag, rhs.flag);
}

void Mpeg4DemuxerPluginUnitTest::CompareSeekReadResult(
    const std::string& filePath, const std::vector<int64_t>& seekTimes,
    const std::vector<SeekMode>& seekModes, bool useUri)
{
    for (auto seekTimeTarget : seekTimes) {
        for (auto seekMode : seekModes) {
            InitPluginsForReadTest(filePath, useUri);

            auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
            auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
            ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

            int64_t ffmpegRealSeekTime = 0;
            int64_t mpeg4RealSeekTime = 0;
            auto ffmpegRet = ffmpegPlugin_->SeekTo(-1, seekTimeTarget, seekMode, ffmpegRealSeekTime);
            auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, seekTimeTarget, seekMode, mpeg4RealSeekTime);
            ASSERT_EQ(mpeg4Ret, ffmpegRet);
            if (ffmpegRet != Status::OK) {
                ResetPlugin(ffmpegPlugin_, ffmpegFd_);
                ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
                continue;
            }
            auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekSuccessDigest(mpeg4Digest, ffmpegDigest);

            ResetPlugin(ffmpegPlugin_, ffmpegFd_);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
        }
    }
}

SeekToEosReadResult Mpeg4DemuxerPluginUnitTest::ReadToEosAfterSeek(
    const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId, int64_t seekTimeMs, ReadMode readMode)
{
    SeekToEosReadResult result;
    int64_t realSeekTime = 0;
    result.seekStatus = plugin->SeekTo(static_cast<int32_t>(trackId), seekTimeMs,
        SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    if (result.seekStatus != Status::OK) {
        return result;
    }

    std::unique_ptr<AVBufferWrapper> copyBuffer;
    if (readMode == ReadMode::COPY) {
        copyBuffer = std::make_unique<AVBufferWrapper>(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    }
    for (size_t sampleIndex = 0; sampleIndex < MAX_EOS_SEEK_SAMPLE_COUNT; ++sampleIndex) {
        int32_t sampleSize = 0;
        result.nextSampleSizeStatus = GetNextSampleSizeWithRetry(plugin, trackId, sampleSize);
        if (result.nextSampleSizeStatus != Status::OK &&
            result.nextSampleSizeStatus != Status::END_OF_STREAM) {
            return result;
        }
        auto buffer = (readMode == ReadMode::ZERO_COPY) ? MediaAVBuffer::CreateAVBuffer() : copyBuffer->mediaAVBuffer;
        if (buffer == nullptr) {
            result.readStatus = Status::ERROR_NO_MEMORY;
            return result;
        }
        result.readStatus = ReadSampleWithRetry(plugin, trackId, buffer, readMode);
        result.eosBuffer = IsEos(buffer);
        if (result.nextSampleSizeStatus == Status::END_OF_STREAM || result.readStatus != Status::OK ||
            result.eosBuffer) {
            return result;
        }
        if (result.nextSampleSizeStatus != Status::OK) {
            return result;
        }
    }
    result.nextSampleSizeStatus = Status::ERROR_WAIT_TIMEOUT;
    return result;
}

void Mpeg4DemuxerPluginUnitTest::CheckAsyncEosAfterSeek(ReadMode readMode)
{
    InitPluginsForReadTest(TEST_MP4_PATH_H264_B, false);
    auto ffmpegTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4Tracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    uint32_t ffmpegVideoTrack = FindFirstVideoTrack(ffmpegMediaInfo_, ffmpegTracks);
    uint32_t mpeg4VideoTrack = FindFirstVideoTrack(mpeg4MediaInfo_, mpeg4Tracks);
    ASSERT_NE(ffmpegVideoTrack, INVALID_TRACK_ID);
    ASSERT_NE(mpeg4VideoTrack, INVALID_TRACK_ID);

    auto ffmpegResult = ReadToEosAfterSeek(ffmpegPlugin_, ffmpegVideoTrack, TEST_EOS_SEEK_TIME_MS, readMode);
    auto mpeg4Result = ReadToEosAfterSeek(mpeg4Plugin_, mpeg4VideoTrack, TEST_EOS_SEEK_TIME_MS, readMode);
    EXPECT_EQ(ffmpegResult.seekStatus, Status::OK);
    EXPECT_EQ(mpeg4Result.seekStatus, ffmpegResult.seekStatus);
    EXPECT_EQ(ffmpegResult.nextSampleSizeStatus, Status::END_OF_STREAM);
    EXPECT_EQ(mpeg4Result.nextSampleSizeStatus, ffmpegResult.nextSampleSizeStatus);
    EXPECT_EQ(ffmpegResult.readStatus, Status::OK);
    EXPECT_EQ(mpeg4Result.readStatus, ffmpegResult.readStatus);
    EXPECT_TRUE(ffmpegResult.eosBuffer);
    EXPECT_EQ(mpeg4Result.eosBuffer, ffmpegResult.eosBuffer);
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekFirstVideoSampleWithSourceTimeline(
    const std::string& filePath, const std::vector<int64_t>& seekTimes,
    const std::vector<SeekMode>& seekModes, bool useUri)
{
    useUri ? InitMpeg4PluginURI(filePath) : InitMpeg4Plugin(filePath);
    auto sourceSelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    uint32_t sourceVideoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, sourceSelectedTracks);
    ASSERT_NE(sourceVideoTrackId, INVALID_TRACK_ID);
    int64_t videoStartTimeUs = 0;
    (void)mpeg4MediaInfo_.tracks[sourceVideoTrackId].Get<Tag::MEDIA_START_TIME>(videoStartTimeUs);
    auto sourceDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, sourceSelectedTracks);
    const auto* sourceVideoDigest = FindTrackDigest(sourceDigest, sourceVideoTrackId);
    ASSERT_NE(sourceVideoDigest, nullptr);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);

    for (auto seekTimeTarget : seekTimes) {
        for (auto seekMode : seekModes) {
            SCOPED_TRACE("seekTime=" + std::to_string(seekTimeTarget) +
                ", mode=" + std::to_string(static_cast<int32_t>(seekMode)));
            int64_t targetPtsUs = videoStartTimeUs + seekTimeTarget * TEST_MS_TO_US;
            size_t expectedSampleIndex = 0;
            ASSERT_TRUE(SelectSourceSyncSampleByPts(*sourceVideoDigest, targetPtsUs, seekMode, expectedSampleIndex));

            useUri ? InitMpeg4PluginURI(filePath) : InitMpeg4Plugin(filePath);
            auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
            uint32_t mpeg4VideoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, mpeg4SelectedTracks);
            ASSERT_NE(mpeg4VideoTrackId, INVALID_TRACK_ID);

            int64_t mpeg4RealSeekTime = 0;
            auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, seekTimeTarget, seekMode, mpeg4RealSeekTime);
            ASSERT_EQ(mpeg4Ret, Status::OK);
            EXPECT_EQ(mpeg4RealSeekTime, targetPtsUs * TEST_US_TO_NS);

            AVBufferWrapper mpeg4Buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
            ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, mpeg4VideoTrackId, mpeg4Buffer.mediaAVBuffer,
                ReadMode::COPY), Status::OK);
            EXPECT_FALSE(IsEos(mpeg4Buffer.mediaAVBuffer));
            EXPECT_EQ(mpeg4Buffer.mediaAVBuffer->pts_, sourceVideoDigest->samplePts[expectedSampleIndex]);
            EXPECT_EQ(mpeg4Buffer.mediaAVBuffer->dts_, sourceVideoDigest->sampleDts[expectedSampleIndex]);
            EXPECT_EQ(mpeg4Buffer.mediaAVBuffer->duration_, sourceVideoDigest->sampleDurations[expectedSampleIndex]);
            EXPECT_NE(mpeg4Buffer.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME), 0U);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekReadResultWithSourceTimeline(
    const std::string& filePath, const std::vector<int64_t>& seekTimes, const std::vector<SeekMode>& seekModes)
{
    InitMpeg4Plugin(filePath);
    auto sourceSelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    uint32_t sourceVideoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, sourceSelectedTracks);
    ASSERT_NE(sourceVideoTrackId, INVALID_TRACK_ID);
    int64_t videoStartTimeUs = 0;
    (void)mpeg4MediaInfo_.tracks[sourceVideoTrackId].Get<Tag::MEDIA_START_TIME>(videoStartTimeUs);
    auto sourceDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, sourceSelectedTracks);
    const auto* sourceVideoDigest = FindTrackDigest(sourceDigest, sourceVideoTrackId);
    ASSERT_NE(sourceVideoDigest, nullptr);
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);

    for (auto seekTimeTarget : seekTimes) {
        for (auto seekMode : seekModes) {
            SCOPED_TRACE("seekTime=" + std::to_string(seekTimeTarget) +
                ", mode=" + std::to_string(static_cast<int32_t>(seekMode)));
            size_t expectedSampleIndex = 0;
            int64_t targetPtsUs = videoStartTimeUs + seekTimeTarget * TEST_MS_TO_US;
            ASSERT_TRUE(SelectSourceSyncSampleByPts(*sourceVideoDigest, targetPtsUs, seekMode, expectedSampleIndex));
            InitMpeg4Plugin(filePath);
            auto selectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
            int64_t realSeekTime = 0;
            ASSERT_EQ(mpeg4Plugin_->SeekTo(-1, seekTimeTarget, seekMode, realSeekTime), Status::OK);
            EXPECT_EQ(realSeekTime, targetPtsUs * TEST_US_TO_NS);
            auto seekDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, selectedTracks);
            const auto* seekVideoDigest = FindTrackDigest(seekDigest, sourceVideoTrackId);
            ASSERT_NE(seekVideoDigest, nullptr);
            ASSERT_EQ(seekVideoDigest->samplePts.size(), sourceVideoDigest->samplePts.size() - expectedSampleIndex);
            EXPECT_TRUE(std::equal(sourceVideoDigest->samplePts.begin() + expectedSampleIndex,
                sourceVideoDigest->samplePts.end(), seekVideoDigest->samplePts.begin()));
            EXPECT_TRUE(std::equal(sourceVideoDigest->sampleDts.begin() + expectedSampleIndex,
                sourceVideoDigest->sampleDts.end(), seekVideoDigest->sampleDts.begin()));
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
        }
    }
}

void Mpeg4DemuxerPluginUnitTest::CompareWeakNetworkReadResult(
    const std::string& filePath, int64_t failOffset, size_t maxFailCount)
{
    InitWeakNetworkFfmpegPlugin(filePath, failOffset, maxFailCount);
    InitWeakNetworkMpeg4Plugin(filePath, failOffset, maxFailCount);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

void Mpeg4DemuxerPluginUnitTest::CheckSeekReadResultWithExpected(const std::string& filePath,
    const std::vector<SeekExpectedResult>& expectedResults, bool useUri)
{
    for (const auto& expected : expectedResults) {
        InitPluginsForReadTest(filePath, useUri);

        auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
        auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
        ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

        int64_t ffmpegRealSeekTime = 0;
        int64_t mpeg4RealSeekTime = 0;
        auto ffmpegRet = ffmpegPlugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, ffmpegRealSeekTime);
        auto mpeg4Ret = mpeg4Plugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, mpeg4RealSeekTime);

        if (expected.seekSuccess) {
            ASSERT_EQ(ffmpegRet, Status::OK);
            ASSERT_EQ(mpeg4Ret, Status::OK);
            auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekSuccessDigestWithExpected(ffmpegDigest, mpeg4Digest, expected.expectedFrames);
            ResetPlugin(ffmpegPlugin_, ffmpegFd_);
            ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
            continue;
        }
        EXPECT_NE(ffmpegRet, Status::OK);
        if (mpeg4Ret == Status::OK) {
            auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
            CheckSeekFallbackDigest(mpeg4Digest, mpeg4SelectedTracks, expected.expectedFrames);
        }

        ResetPlugin(ffmpegPlugin_, ffmpegFd_);
        ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_InitSelectTrack_001
 * @tc.desc: Directly create MPEG4DemuxerPlugin and select supported tracks
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InitSelectTrack_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    ASSERT_GT(mpeg4MediaInfo_.tracks.size(), 0U);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    EXPECT_FALSE(selectedTrackIds.empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroDurationMp3EditProjection_001
 * @tc.desc: Verify edit-list projection retains every MP3 sample sharing the same DTS
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ZeroDurationMp3EditProjection_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper.mimeType_ = MimeType::AUDIO_MPEG;
    helper.sttsEntry_ = {{180, 0}, {1, 576}};
    helper.stscEntry_ = {{0, 181, 1}};
    helper.stssEntry_ = {{0}, {1}, {180}};
    helper.sampleSizes_.assign(181, 1);
    helper.sampleSizeCount_ = 181;
    helper.chunkOffsets_ = {0};
    helper.SetEditListParams({{0, 36, 1, 0}}, 1000, 16000);

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    EXPECT_EQ(helper.GetSampleCount(), 181U);
    EXPECT_EQ(helper.GetSyncSamplesIndexFromStssEntry(), std::vector<uint32_t>({0, 1, 180}));
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView firstSample;
    ASSERT_EQ(helper.GetSampleIndexView(0, firstSample), Status::OK);
    EXPECT_EQ(firstSample.pos, 0);
    constexpr uint32_t expectedSyncFlag =
        static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME);
    EXPECT_EQ(firstSample.flag & expectedSyncFlag, expectedSyncFlag);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroDurationVideoEditKeepsRawTimeline_001
 * @tc.desc: Verify malformed zero-duration video media edits keep the native seek timeline.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ZeroDurationVideoEditKeepsRawTimeline_001, TestSize.Level1)
{
    constexpr uint32_t sampleCount = 3;
    constexpr uint32_t sampleDuration = 10;
    constexpr uint32_t sampleSize = 1;
    constexpr int64_t invalidEditMediaTime = 25;
    constexpr int64_t seekPts = 15;
    constexpr uint32_t expectedSyncSample = 2;
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper.mimeType_ = MimeType::VIDEO_AVC;
    helper.sttsEntry_ = {{sampleCount, sampleDuration}};
    helper.cttsEntry_ = {{sampleCount, 0}};
    helper.stscEntry_ = {{0, sampleCount, 1}};
    helper.stssEntry_ = {{0}, {expectedSyncSample}};
    helper.sampleSizes_.assign(sampleCount, static_cast<int32_t>(sampleSize));
    helper.sampleSizeCount_ = sampleCount;
    helper.chunkOffsets_ = {0};
    helper.SetEditListParams({{invalidEditMediaTime, 0, 1, 0}}, 1, 1);

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    EXPECT_TRUE(helper.HasAppliedEditList());
    EXPECT_EQ(helper.GetSampleCount(), static_cast<size_t>(sampleCount));
    EXPECT_EQ(helper.GetSyncSamplesIndexFromStssEntry(), std::vector<uint32_t>({0, expectedSyncSample}));
    uint32_t sampleIndex = 0;
    EXPECT_EQ(helper.FindSyncSampleByPts(seekPts, true, SeekMode::SEEK_CLOSEST_SYNC, sampleIndex), Status::OK);
    EXPECT_EQ(sampleIndex, expectedSyncSample);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroDurationSingleFrameVideoEditKeepsDiscard_001
 * @tc.desc: Verify a zero-timeline video sample remains discardable like FFmpeg mov_fix_index.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ZeroDurationSingleFrameVideoEditKeepsDiscard_001, TestSize.Level1)
{
    constexpr uint32_t sampleCount = 1;
    constexpr uint32_t sampleSize = 1;
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper.mimeType_ = MimeType::VIDEO_AVC;
    helper.sttsEntry_ = {{sampleCount, 0}};
    helper.stscEntry_ = {{0, sampleCount, 1}};
    helper.stssEntry_ = {{0}};
    helper.sampleSizes_.assign(sampleCount, static_cast<int32_t>(sampleSize));
    helper.sampleSizeCount_ = sampleCount;
    helper.chunkOffsets_ = {0};
    helper.SetEditListParams({{0, 0, 1, 0}}, 1, 1);

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView sample;
    ASSERT_EQ(helper.GetSampleIndexView(0, sample), Status::OK);
    constexpr uint32_t discardFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::DISCARD);
    EXPECT_NE(sample.flag & discardFlag, 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroDurationAudioEditKeepsProjection_001
 * @tc.desc: Verify zero-duration media edits on audio keep the existing edit-list projection.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ZeroDurationAudioEditKeepsProjection_001, TestSize.Level1)
{
    constexpr uint32_t sampleCount = 5;
    constexpr uint32_t sampleDuration = 10;
    constexpr uint32_t sampleSize = 1;
    constexpr int64_t editMediaTime = 25;
    constexpr size_t expectedProjectedSampleCount = 1;
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper.mimeType_ = MimeType::AUDIO_MPEG;
    helper.sttsEntry_ = {{sampleCount, sampleDuration}};
    helper.stscEntry_ = {{0, sampleCount, 1}};
    helper.sampleSizes_.assign(sampleCount, static_cast<int32_t>(sampleSize));
    helper.sampleSizeCount_ = sampleCount;
    helper.chunkOffsets_ = {0};
    helper.SetEditListParams({{editMediaTime, 0, 1, 0}}, 1, 1);

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    EXPECT_TRUE(helper.HasAppliedEditList());
    EXPECT_EQ(helper.GetSampleCount(), expectedProjectedSampleCount);
    EXPECT_NE(helper.GetSampleCount(), static_cast<size_t>(sampleCount));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroDurationAudioEditKeepsRawTimeline_001
 * @tc.desc: Verify a zero-origin media edit cannot discard a positive raw AAC timeline.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ZeroDurationAudioEditKeepsRawTimeline_001, TestSize.Level1)
{
    constexpr uint32_t sampleCount = 5;
    constexpr uint32_t sampleDuration = 10;
    constexpr uint32_t sampleSize = 1;
    constexpr uint32_t lastSampleIndex = sampleCount - 1;
    constexpr uint32_t discardFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::DISCARD);
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper.mimeType_ = MimeType::AUDIO_AAC;
    helper.sttsEntry_ = {{sampleCount, sampleDuration}};
    helper.stscEntry_ = {{0, sampleCount, 1}};
    helper.sampleSizes_.assign(sampleCount, static_cast<int32_t>(sampleSize));
    helper.sampleSizeCount_ = sampleCount;
    helper.chunkOffsets_ = {0};
    helper.SetEditListParams({{0, 0, 1, 0}}, 1, 1);

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    ASSERT_TRUE(helper.HasAppliedEditList());
    ASSERT_EQ(helper.GetSampleCount(), static_cast<size_t>(sampleCount));
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView firstSample;
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView lastSample;
    ASSERT_EQ(helper.GetSampleIndexView(0, firstSample), Status::OK);
    ASSERT_EQ(helper.GetSampleIndexView(lastSampleIndex, lastSample), Status::OK);
    EXPECT_EQ(firstSample.dts, 0);
    EXPECT_EQ(firstSample.duration, sampleDuration);
    EXPECT_EQ(firstSample.flag & discardFlag, 0U);
    EXPECT_EQ(lastSample.dts, static_cast<int64_t>(lastSampleIndex) * sampleDuration);
    EXPECT_EQ(lastSample.duration, sampleDuration);
    EXPECT_EQ(lastSample.flag & discardFlag, 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AudioStraddleEditFirstFrameSkipPacket_001
 * @tc.desc: Verify the first audio frame straddling the edit-list start clears DISCARD
 *           through skip_samples, matching FFmpeg mov_fix_index; fully-outside and
 *           exact-boundary frames keep DISCARD.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AudioStraddleEditFirstFrameSkipPacket_001, TestSize.Level1)
{
    auto straddleTrack = BuildAudioEditTrack(AUDIO_EDIT_TEST_STRADDLE_MEDIA_TIME);
    ASSERT_NE(straddleTrack, nullptr);
    const auto* straddleSkipInfo = straddleTrack->sampleHelper->GetSkipSamplesInfo(0);
    ASSERT_NE(straddleSkipInfo, nullptr);
    EXPECT_EQ(Plugins::MPEG4::GetSkipSamplesFromInfo(*straddleSkipInfo), AUDIO_EDIT_TEST_STRADDLE_MEDIA_TIME);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(straddleTrack->sampleHelper->GetSampleIndexView(0, view), Status::OK);
    EXPECT_EQ(view.duration, AUDIO_EDIT_TEST_SAMPLE_DURATION);
    EXPECT_TRUE(Plugins::MPEG4::ShouldClearAudioDiscardForSkipPacket(straddleTrack, 0, view, false));

    auto boundaryTrack = BuildAudioEditTrack(AUDIO_EDIT_TEST_BOUNDARY_MEDIA_TIME);
    ASSERT_NE(boundaryTrack, nullptr);
    ASSERT_EQ(boundaryTrack->sampleHelper->GetSampleIndexView(0, view), Status::OK);
    EXPECT_FALSE(Plugins::MPEG4::ShouldClearAudioDiscardForSkipPacket(boundaryTrack, 0, view, false));

    auto outsideTrack = BuildAudioEditTrack(AUDIO_EDIT_TEST_OUTSIDE_MEDIA_TIME);
    ASSERT_NE(outsideTrack, nullptr);
    EXPECT_EQ(outsideTrack->sampleHelper->GetSkipSamplesInfo(0), nullptr);
    ASSERT_EQ(outsideTrack->sampleHelper->GetSampleIndexView(0, view), Status::OK);
    EXPECT_FALSE(Plugins::MPEG4::ShouldClearAudioDiscardForSkipPacket(outsideTrack, 0, view, false));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_FindPrevClosestIndexSameDts_001
 * @tc.desc: Verify the earliest eligible sync sample is selected for an equal-DTS group.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_FindPrevClosestIndexSameDts_001, TestSize.Level1)
{
    constexpr int64_t equalDts = 0;
    constexpr int64_t nextDts = 10;
    constexpr uint32_t sampleSize = 1;
    Plugins::MPEG4::MPEG4SampleHelper helper;
    std::vector<Plugins::MPEG4::MPEG4SampleHelper::SampleIndexEntry> entries {
        MakeSampleIndexEntry(0, equalDts, equalDts, sampleSize),
        MakeSampleIndexEntry(1, equalDts, equalDts, sampleSize),
        MakeSampleIndexEntry(2, equalDts, equalDts, sampleSize),
        MakeSampleIndexEntry(3, nextDts, nextDts, sampleSize),
    };
    std::vector<Plugins::MPEG4::MPEG4SampleHelper::StssEntry> syncSamples {{1}, {2}, {3}};

    EXPECT_EQ(helper.FindPrevClosestIndex(entries, syncSamples, false, equalDts, false), 1);
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, syncSamples, false, equalDts, true), 0);
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, syncSamples, true, equalDts, false), 0);
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, syncSamples, false, nextDts, false), 3);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_FindPrevClosestIndexBoundary_001
 * @tc.desc: Verify empty, before-first and no-sync inputs return the expected sentinel or fallback.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_FindPrevClosestIndexBoundary_001, TestSize.Level1)
{
    constexpr int64_t noSampleIndex = -1;
    constexpr int64_t firstDts = 10;
    constexpr uint32_t sampleSize = 1;
    constexpr int32_t negativeCttsOffset = -5;
    Plugins::MPEG4::MPEG4SampleHelper helper;
    std::vector<Plugins::MPEG4::MPEG4SampleHelper::SampleIndexEntry> entries {
        MakeSampleIndexEntry(0, firstDts, firstDts, sampleSize),
    };
    std::vector<Plugins::MPEG4::MPEG4SampleHelper::StssEntry> noSyncSamples;

    EXPECT_EQ(helper.FindPrevClosestIndex({}, noSyncSamples, false, firstDts, false), noSampleIndex);
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, noSyncSamples, false, firstDts - 1, true), noSampleIndex);
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, noSyncSamples, false, firstDts, false), noSampleIndex);
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, noSyncSamples, false, firstDts, true), 0);
    const std::vector<uint8_t> ctts = BuildCttsData({{1, static_cast<uint32_t>(negativeCttsOffset)}});
    helper.dataReader_ = CreateReaderForBytes(ctts);
    ASSERT_EQ(helper.SetCompositionTimeToSampleParams(0, ctts.size()), Status::OK);
    helper.CalculateDtsShift();
    std::vector<Plugins::MPEG4::MPEG4SampleHelper::StssEntry> firstSyncSamples {{0}};
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, firstSyncSamples, false, firstDts + 4, true), noSampleIndex);
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, firstSyncSamples, false, firstDts + 5, true), 0);

    std::vector<Plugins::MPEG4::MPEG4SampleHelper::StssEntry> invalidSyncSamples {{1}};
    EXPECT_EQ(helper.FindPrevClosestIndex(entries, invalidSyncSamples, false, firstDts + 5, false), noSampleIndex);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioCompleteSampleDirectRead_001
 * @tc.desc: Verify a complete MP3 frame keeps the direct read path.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioCompleteSampleDirectRead_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(frame.size())}), nullptr);
    auto source = CreateDataSourceForBytes(frame);
    ASSERT_NE(source, nullptr);
    plugin.dataSource_ = source;
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);

    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), frame.size());
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), frame);
    EXPECT_EQ(source->readExpectedLens, std::vector<size_t>({sizeof(MPEG_AUDIO_TEST_HEADER), frame.size()}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioDirectSizeQuery_001
 * @tc.desc: Verify a complete MP3 size query only probes the header and preserves direct sample reading.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioDirectSizeQuery_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(frame.size())}), nullptr);
    auto source = CreateDataSourceForBytes(frame);
    ASSERT_NE(source, nullptr);
    plugin.dataSource_ = source;
    int32_t sampleSize = 0;

    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(frame.size()));
    EXPECT_EQ(source->readExpectedLens, std::vector<size_t>({sizeof(MPEG_AUDIO_TEST_HEADER)}));
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), frame);
    EXPECT_EQ(source->readExpectedLens,
        std::vector<size_t>({sizeof(MPEG_AUDIO_TEST_HEADER), sizeof(MPEG_AUDIO_TEST_HEADER), frame.size()}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioSizeQueryEos_001
 * @tc.desc: Verify an MP3 size query reports EOS after the final complete sample.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioSizeQueryEos_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(frame.size())}), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(frame);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    int32_t sampleSize = 0;

    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::END_OF_STREAM);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioCrossSamplePacket_001
 * @tc.desc: Verify a header-aligned partial MP4 sample preserves its FFmpeg packet boundary.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioCrossSamplePacket_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    uint32_t tailSize = static_cast<uint32_t>(frame.size() - MPEG_AUDIO_TEST_SPLIT_SIZE);
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin,
        {static_cast<uint32_t>(MPEG_AUDIO_TEST_SPLIT_SIZE), tailSize}), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(frame);
    ASSERT_NE(plugin.dataSource_, nullptr);
    int32_t sampleSize = 0;
    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(MPEG_AUDIO_TEST_SPLIT_SIZE));
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), MPEG_AUDIO_TEST_SPLIT_SIZE);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()),
        std::vector<uint8_t>(frame.begin(), frame.begin() + MPEG_AUDIO_TEST_SPLIT_SIZE));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioIrregularSampleBoundary_001
 * @tc.desc: Verify MP3 parser output follows FFmpeg packet boundaries when MP4 samples split two frames.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioIrregularSampleBoundary_001, TestSize.Level1)
{
    const std::vector<uint32_t> sampleSizes = {MPEG_AUDIO_TEST_FRAME_SIZE, MPEG_AUDIO_TEST_PARTIAL_FRAME_SIZE,
        MPEG_AUDIO_TEST_MIXED_SAMPLE_SIZE, MPEG_AUDIO_TEST_FRAME_SIZE,
        MPEG_AUDIO_TEST_FRAME_SIZE - MPEG_AUDIO_TEST_FOLLOWING_PREFIX_SIZE + MPEG_AUDIO_TEST_FRAME_SIZE};
    const std::vector<int32_t> expectedSizes = {MPEG_AUDIO_TEST_FRAME_SIZE, MPEG_AUDIO_TEST_PARTIAL_FRAME_SIZE,
        MPEG_AUDIO_TEST_MIXED_SAMPLE_SIZE + MPEG_AUDIO_TEST_FOLLOWING_FRAME_TAIL_SIZE,
        MPEG_AUDIO_TEST_FRAME_SIZE, MPEG_AUDIO_TEST_FRAME_SIZE};
    auto input = BuildMpegAudioBoundaryTestFile();
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, sampleSizes), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(input);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(MPEG_AUDIO_TEST_MIXED_SAMPLE_SIZE + MPEG_AUDIO_TEST_FOLLOWING_FRAME_TAIL_SIZE);
    std::vector<uint8_t> outputData;

    for (int32_t expectedSize : expectedSizes) {
        int32_t sampleSize = 0;
        ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
        EXPECT_EQ(sampleSize, expectedSize);
        ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), static_cast<size_t>(expectedSize));
        outputData.insert(outputData.end(), output.mediaAVBuffer->memory_->GetAddr(),
            output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize());
    }
    EXPECT_EQ(outputData, input);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioHeaderAcrossSamples_001
 * @tc.desc: Verify the MP3 header cursor reads across MP4 sample boundaries without rescanning prior segments.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioHeaderAcrossSamples_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    uint32_t tailSize = static_cast<uint32_t>(frame.size() - MPEG_AUDIO_TEST_HEADER_SPLIT_SIZE);
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin,
        {static_cast<uint32_t>(MPEG_AUDIO_TEST_HEADER_SPLIT_SIZE), tailSize}), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(frame);
    ASSERT_NE(plugin.dataSource_, nullptr);
    int32_t sampleSize = 0;
    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(frame.size()));
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), frame);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioMultiplePacketsInSample_001
 * @tc.desc: Verify a header-aligned MP4 sample preserves multiple MP3 frames as one FFmpeg packet.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioMultiplePacketsInSample_001, TestSize.Level1)
{
    auto firstFrame = BuildMpegAudioTestFrame();
    auto secondFrame = BuildMpegAudioTestFrame();
    secondFrame.back() ^= 0xFFU;
    std::vector<uint8_t> input = firstFrame;
    input.insert(input.end(), secondFrame.begin(), secondFrame.end());
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(input.size())}), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(input);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(input.size());
    int32_t sampleSize = 0;

    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(input.size()));
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), input);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioInvalidPrefixCursor_001
 * @tc.desc: Verify parser cursor retains an invalid prefix and finds the following MP3 header once.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioInvalidPrefixCursor_001, TestSize.Level1)
{
    constexpr std::array<uint8_t, 3> invalidPrefix = {0x00, 0x11, 0x22};
    auto frame = BuildMpegAudioTestFrame();
    std::vector<uint8_t> input(invalidPrefix.begin(), invalidPrefix.end());
    input.insert(input.end(), frame.begin(), frame.end());
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(input.size())}), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(input);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(input.size());
    int32_t sampleSize = 0;

    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(input.size()));
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), input);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioPendingLimit_001
 * @tc.desc: Verify invalid MP3 data cannot grow per-track parser pending storage beyond its limit.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioPendingLimit_001, TestSize.Level1)
{
    static_assert(MPEG_AUDIO_TEST_PENDING_LIMIT % MPEG_AUDIO_TEST_INVALID_SAMPLE_SIZE == 0);
    size_t fullSampleCount = MPEG_AUDIO_TEST_PENDING_LIMIT / MPEG_AUDIO_TEST_INVALID_SAMPLE_SIZE;
    std::vector<uint32_t> sampleSizes(fullSampleCount, MPEG_AUDIO_TEST_INVALID_SAMPLE_SIZE);
    sampleSizes.emplace_back(1);
    std::vector<uint8_t> input(MPEG_AUDIO_TEST_PENDING_LIMIT + 1, 0);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, sampleSizes), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(input);
    ASSERT_NE(plugin.dataSource_, nullptr);
    int32_t sampleSize = 0;

    EXPECT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioFragmentedPendingCoalesce_001
 * @tc.desc: Verify many tiny invalid MP3 samples are coalesced without changing EOF flush output.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioFragmentedPendingCoalesce_001, TestSize.Level1)
{
    std::vector<uint32_t> sampleSizes(MPEG_AUDIO_TEST_SEGMENT_COALESCE_COUNT, 1);
    std::vector<uint8_t> input(MPEG_AUDIO_TEST_SEGMENT_COALESCE_COUNT, 0);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, sampleSizes), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(input);
    ASSERT_NE(plugin.dataSource_, nullptr);
    int32_t sampleSize = 0;

    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(input.size()));
    AVBufferWrapper output(input.size());
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), input);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioUnseekableCrossSample_001
 * @tc.desc: Verify the unseekable path preserves header-aligned partial MP4 sample boundaries.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioUnseekableCrossSample_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    uint32_t tailSize = static_cast<uint32_t>(frame.size() - MPEG_AUDIO_TEST_SPLIT_SIZE);
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin,
        {static_cast<uint32_t>(MPEG_AUDIO_TEST_SPLIT_SIZE), tailSize}), nullptr);
    plugin.seekable_ = Seekable::UNSEEKABLE;
    plugin.dataSource_ = CreateDataSourceForBytes(frame);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    std::vector<uint8_t> outputData;

    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    outputData.insert(outputData.end(), output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize());
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), MPEG_AUDIO_TEST_SPLIT_SIZE);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    outputData.insert(outputData.end(), output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize());
    EXPECT_EQ(outputData, frame);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioAsyncEofFlush_001
 * @tc.desc: Verify the asynchronous cache path flushes a pending MP3 tail at end of stream.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioAsyncEofFlush_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    std::vector<uint8_t> tail(frame.begin(), frame.begin() + MPEG_AUDIO_TEST_SPLIT_SIZE);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(tail.size())}), nullptr);
    plugin.seekable_ = Seekable::UNSEEKABLE;
    plugin.dataSource_ = CreateDataSourceForBytes(tail);
    ASSERT_NE(plugin.dataSource_, nullptr);

    ASSERT_EQ(plugin.ReadSampleToCache(MPEG_AUDIO_TEST_TRACK_ID, plugin.readConfigEpoch_), Status::OK);
    auto cached = plugin.FrontTrackCache(MPEG_AUDIO_TEST_TRACK_ID);
    ASSERT_NE(cached, nullptr);
    ASSERT_NE(cached->sample, nullptr);
    EXPECT_EQ(cached->sample->size, static_cast<int32_t>(tail.size()));
    EXPECT_EQ(std::vector<uint8_t>(cached->sample->data.get(),
        cached->sample->data.get() + cached->sample->size), tail);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioEofFlushAndReset_001
 * @tc.desc: Verify an incomplete MP3 tail is flushed at EOF and parser state is released by Flush.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioEofFlushAndReset_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    std::vector<uint8_t> tail(frame.begin(), frame.begin() + MPEG_AUDIO_TEST_SPLIT_SIZE);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(tail.size())}), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(tail);
    ASSERT_NE(plugin.dataSource_, nullptr);
    int32_t sampleSize = 0;
    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(tail.size()));
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), tail.size());
    EXPECT_EQ(plugin.Flush(), Status::OK);
    EXPECT_EQ(plugin.mpegAudioParserStates_, nullptr);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioStsdReassemblyTrigger_001
 * @tc.desc: Verify a version 0 MP3 stsd entry switches the stream to MP3 frame reassembly.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioStsdReassemblyTrigger_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    uint32_t tailSize = static_cast<uint32_t>(frame.size() - MPEG_AUDIO_TEST_SPLIT_SIZE);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareMpegAudioTestTrack(plugin,
        {static_cast<uint32_t>(MPEG_AUDIO_TEST_SPLIT_SIZE), tailSize});
    ASSERT_NE(track, nullptr);
    // FFmpeg mov_parse_stsd_audio: a plain version 0 MP3 sample entry cannot be handled as a
    // fixed-size audio unit and forces AVSTREAM_PARSE_FULL.
    track->stsdParsed = true;
    track->stsdVersion = 0;
    track->audioSampleEntryVersion = 0;
    plugin.dataSource_ = CreateDataSourceForBytes(frame);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);

    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), frame.size());
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), frame);
    EXPECT_EQ(output.mediaAVBuffer->pts_, 0);
    EXPECT_EQ(output.mediaAVBuffer->dts_, 0);
    EXPECT_EQ(output.mediaAVBuffer->duration_, MPEG_AUDIO_TEST_FRAME_DURATION_US);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioSttsReassemblyTrigger_001
 * @tc.desc: Verify many short stts runs with mdhd timescale equal to the sample rate trigger reassembly.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioSttsReassemblyTrigger_001, TestSize.Level1)
{
    auto frames = BuildMpegAudioTestFrames(2);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareMpegAudioTestTrack(plugin,
        {MPEG_AUDIO_TEST_FRAME_SIZE / 2, MPEG_AUDIO_TEST_FRAME_SIZE / 2,
         MPEG_AUDIO_TEST_FRAME_SIZE / 2, MPEG_AUDIO_TEST_FRAME_SIZE / 2});
    ASSERT_NE(track, nullptr);
    // FFmpeg mov_read_header: stts has more than three runs and the runs outnumber
    // 1/10 of the samples while mdhd timescale matches the MP3 sample rate.
    track->sampleHelper->sttsEntry_ = {
        {1, MPEG_AUDIO_TEST_SAMPLES_PER_FRAME}, {1, MPEG_AUDIO_TEST_SAMPLES_PER_FRAME},
        {1, MPEG_AUDIO_TEST_SAMPLES_PER_FRAME}, {1, MPEG_AUDIO_TEST_SAMPLES_PER_FRAME},
    };
    plugin.dataSource_ = CreateDataSourceForBytes(frames);
    ASSERT_NE(plugin.dataSource_, nullptr);
    int32_t sampleSize = 0;
    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(MPEG_AUDIO_TEST_FRAME_SIZE));

    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), MPEG_AUDIO_TEST_FRAME_SIZE);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()),
        std::vector<uint8_t>(frames.begin(), frames.begin() + MPEG_AUDIO_TEST_FRAME_SIZE));
    EXPECT_EQ(output.mediaAVBuffer->pts_, 0);
    EXPECT_EQ(output.mediaAVBuffer->duration_, MPEG_AUDIO_TEST_FRAME_DURATION_US);

    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), MPEG_AUDIO_TEST_FRAME_SIZE);
    EXPECT_EQ(output.mediaAVBuffer->pts_, MPEG_AUDIO_TEST_FRAME_DURATION_US * 2);
    EXPECT_EQ(output.mediaAVBuffer->duration_, MPEG_AUDIO_TEST_FRAME_DURATION_US);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioLegacyHandbrakeTrigger_001
 * @tc.desc: Verify HandBrake <= 0.10.2 MP3 timing forces reassembly while newer versions pass through.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioLegacyHandbrakeTrigger_001, TestSize.Level1)
{
    auto frame = BuildMpegAudioTestFrame();
    uint32_t tailSize = static_cast<uint32_t>(frame.size() - MPEG_AUDIO_TEST_SPLIT_SIZE);
    std::vector<uint32_t> sampleSizes = {
        static_cast<uint32_t>(MPEG_AUDIO_TEST_SPLIT_SIZE), tailSize,
    };
    auto readFirstSize = [sampleSizes, frame](const std::string& encoder) -> size_t {
        Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
        EXPECT_NE(PrepareMpegAudioTestTrack(plugin, sampleSizes), nullptr);
        plugin.mediaInfo_.general.SetData(Tag::MEDIA_ENCODER, encoder);
        plugin.dataSource_ = CreateDataSourceForBytes(frame);
        EXPECT_NE(plugin.dataSource_, nullptr);
        AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
        if (plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer) != Status::OK) {
            return 0;
        }
        return output.mediaAVBuffer->memory_->GetSize();
    };

    EXPECT_EQ(readFirstSize("HandBrake 0.10.2"), MPEG_AUDIO_TEST_FRAME_SIZE);
    EXPECT_EQ(readFirstSize("HandBrake 0.9.5"), MPEG_AUDIO_TEST_FRAME_SIZE);
    EXPECT_EQ(readFirstSize("HandBrake 0.10.3"), MPEG_AUDIO_TEST_SPLIT_SIZE);
    EXPECT_EQ(readFirstSize("HandBrake 1.0.0"), MPEG_AUDIO_TEST_SPLIT_SIZE);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioReassembledPtsMonotonic_001
 * @tc.desc: Verify reassembled MP3 frames keep monotonic PTS and per-frame duration.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioReassembledPtsMonotonic_001, TestSize.Level1)
{
    auto frames = BuildMpegAudioTestFrames(4);
    std::vector<uint32_t> sampleSizes(8, MPEG_AUDIO_TEST_FRAME_SIZE / 2);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, sampleSizes), nullptr);
    plugin.mediaInfo_.general.SetData(Tag::MEDIA_ENCODER, "HandBrake 0.10.2");
    plugin.dataSource_ = CreateDataSourceForBytes(frames);
    ASSERT_NE(plugin.dataSource_, nullptr);
    const std::vector<int64_t> expectedPts = {0, MPEG_AUDIO_TEST_FRAME_DURATION_US * 2,
        MPEG_AUDIO_TEST_FRAME_DURATION_US * 4, MPEG_AUDIO_TEST_FRAME_DURATION_US * 6};
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);

    for (size_t index = 0; index < expectedPts.size(); ++index) {
        ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), MPEG_AUDIO_TEST_FRAME_SIZE);
        EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
            output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()),
            std::vector<uint8_t>(frames.begin() + static_cast<ptrdiff_t>(index * MPEG_AUDIO_TEST_FRAME_SIZE),
                frames.begin() + static_cast<ptrdiff_t>((index + 1) * MPEG_AUDIO_TEST_FRAME_SIZE)));
        EXPECT_EQ(output.mediaAVBuffer->pts_, expectedPts[index]);
        EXPECT_EQ(output.mediaAVBuffer->dts_, expectedPts[index]);
        EXPECT_EQ(output.mediaAVBuffer->duration_, MPEG_AUDIO_TEST_FRAME_DURATION_US);
    }
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_TRUE(IsEos(output.mediaAVBuffer));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioMultiFramePassthroughContainerDuration_001
 * @tc.desc: Verify a passed-through sample spanning several MP3 frames keeps the container stts duration.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_MpegAudioMultiFramePassthroughContainerDuration_001, TestSize.Level1)
{
    auto firstFrame = BuildMpegAudioTestFrame();
    auto secondFrame = BuildMpegAudioTestFrame();
    secondFrame.back() ^= 0xFFU;
    std::vector<uint8_t> input = firstFrame;
    input.insert(input.end(), secondFrame.begin(), secondFrame.end());
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(input.size())});
    ASSERT_NE(track, nullptr);
    // The container records two MP3 frames in one sample: duration 2 * 576 ticks at 16 kHz.
    ASSERT_EQ(track->sampleHelper->AddDurationOverride(0,
        MPEG_AUDIO_TEST_SAMPLES_PER_FRAME * 2), Status::OK);
    track->duration = static_cast<int64_t>(MPEG_AUDIO_TEST_SAMPLES_PER_FRAME) * 2;
    plugin.seekable_ = Seekable::UNSEEKABLE;
    plugin.dataSource_ = CreateDataSourceForBytes(input);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(input.size());

    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), input.size());
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), input);
    EXPECT_EQ(output.mediaAVBuffer->pts_, 0);
    EXPECT_EQ(output.mediaAVBuffer->duration_, MPEG_AUDIO_TEST_FRAME_DURATION_US * 2);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioTailFrameStreamEndDuration_001
 * @tc.desc: Verify the last MP3 frame closes against the stream end (mdhd/edit list) instead of the frame size.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioTailFrameStreamEndDuration_001, TestSize.Level1)
{
    auto frames = BuildMpegAudioTestFrames(3);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareMpegAudioTestTrack(plugin,
        {MPEG_AUDIO_TEST_FRAME_SIZE, MPEG_AUDIO_TEST_FRAME_SIZE, MPEG_AUDIO_TEST_FRAME_SIZE});
    ASSERT_NE(track, nullptr);
    // mdhd duration cuts 144 ticks off the last frame: 3 * 576 - 144 = 1584 ticks at 16 kHz.
    track->duration = static_cast<int64_t>(MPEG_AUDIO_TEST_SAMPLES_PER_FRAME) * 3 -
        MPEG_AUDIO_TEST_SAMPLES_PER_FRAME / 4;
    plugin.dataSource_ = CreateDataSourceForBytes(frames);
    ASSERT_NE(plugin.dataSource_, nullptr);
    const std::vector<int64_t> expectedDurations = {
        MPEG_AUDIO_TEST_FRAME_DURATION_US,
        MPEG_AUDIO_TEST_FRAME_DURATION_US,
        MPEG_AUDIO_TEST_FRAME_DURATION_US * 3 / 4,
    };
    const std::vector<int64_t> expectedPts = {
        0, MPEG_AUDIO_TEST_FRAME_DURATION_US, MPEG_AUDIO_TEST_FRAME_DURATION_US * 2,
    };
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    for (size_t index = 0; index < expectedDurations.size(); ++index) {
        ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), MPEG_AUDIO_TEST_FRAME_SIZE);
        EXPECT_EQ(output.mediaAVBuffer->pts_, expectedPts[index]);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedDurations[index]);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioInvalidHeaderNeedsParsing_001
 * @tc.desc: Verify an invalid MP3 sample head switches the stream to full parsing and flushes the tail at EOF.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioInvalidHeaderNeedsParsing_001, TestSize.Level1)
{
    const std::vector<uint8_t> garbage = {0x00, 0x11, 0x22, 0x33};
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {static_cast<uint32_t>(garbage.size())}), nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(garbage);
    ASSERT_NE(plugin.dataSource_, nullptr);
    int32_t sampleSize = 0;
    ASSERT_EQ(plugin.GetNextSampleSize(MPEG_AUDIO_TEST_TRACK_ID, sampleSize), Status::OK);
    EXPECT_EQ(sampleSize, static_cast<int32_t>(garbage.size()));
    AVBufferWrapper output(garbage.size());
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(std::vector<uint8_t>(output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()), garbage);
    EXPECT_EQ(output.mediaAVBuffer->pts_, 0);
    EXPECT_EQ(output.mediaAVBuffer->duration_, MPEG_AUDIO_TEST_FRAME_DURATION_US);
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_TRUE(IsEos(output.mediaAVBuffer));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_AudioTailStreamEndKeepsZeroDuration_001
 * @tc.desc: Verify an AAC tail whose dts equals the stream end keeps duration 0 when no AAC probe
 *           frame size is exported (FFmpeg mov_finalize_packet native 0, no compute_pkt_fields
 *           backfill), instead of borrowing the previous STTS spacing.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AudioTailStreamEndKeepsZeroDuration_001, TestSize.Level1)
{
    constexpr int32_t testTimeScale = 48000;
    constexpr int64_t testMdhdDuration = 199 * 1024;
    constexpr int64_t testFrameTicks = 1024;
    constexpr int64_t expectedFrameUs = 21333;
    std::vector<uint32_t> sampleSizes(200, 1);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareAacTailTestTrack(plugin, sampleSizes, testTimeScale, testMdhdDuration,
        testFrameTicks, 0, false, 0, 0);
    ASSERT_NE(track, nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(std::vector<uint8_t>(200, 0x01));
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(256);
    for (size_t index = 0; index < 199; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
    ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->duration_, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacTrailingZeroTailKeepsZeroDuration_001
 * @tc.desc: Verify a non-zero-size AAC tail with a trailing-zero STTS delta and an initial empty
 *           edit closed at/after the stream end keeps duration 0 when the interleaved probe never
 *           reached the track (AUDIO_SAMPLE_PER_FRAME not exported), matching FFmpeg native 0
 *           without the decoder-probe AAC frame size backfill (dog_tail.mp4 track1 frame230 and
 *           line58.mp4 track1 frame200: ff duration 0).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacTrailingZeroTailKeepsZeroDuration_001,
    TestSize.Level1)
{
    constexpr int32_t testTimeScale = 48000;
    constexpr int64_t testMdhdDuration = 199 * 1024;
    constexpr int64_t testFrameTicks = 1024;
    constexpr int64_t testSampleCount = 200;
    constexpr int64_t testEmptyEditTicks = 624;
    constexpr int64_t expectedFrameUs = 21333;
    std::vector<uint32_t> sampleSizes(testSampleCount, 1);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareAacTailTestTrack(plugin, sampleSizes, testTimeScale, testMdhdDuration,
        testFrameTicks, 0, false, testEmptyEditTicks, testMdhdDuration);
    ASSERT_NE(track, nullptr);
    // Model dog_tail's trailing-zero STTS plus empty edit with the probe unreached (probe window
    // consumed by the large video/meta samples): the last delta is 0, the stream end (mdhd/raw
    // total) sits before the last dts, and AUDIO_SAMPLE_PER_FRAME is absent, so the native 0 stays.
    track->sampleHelper->sttsEntry_ = {{199, 1024}, {1, 0}};
    plugin.dataSource_ = CreateDataSourceForBytes(std::vector<uint8_t>(testSampleCount, 0x01));
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(256);
    for (size_t index = 0; index + 1 < testSampleCount; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
    ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->duration_, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacTrailingZeroTailProbeReachedBackfill_001
 * @tc.desc: Verify a non-zero-size AAC tail with a trailing-zero STTS delta and an initial empty
 *           edit closed at the stream end is backfilled from the probed AAC frame size when the
 *           probe reached the track (AUDIO_SAMPLE_PER_FRAME exported), matching FFmpeg
 *           compute_pkt_fields av_get_audio_frame_duration (video_audio_one_frame_test.mp4 frame5 /
 *           single_rk.mp4 frame60: ff 21333; H264_3_720x1280.mp4 frame668: ff 23219).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacTrailingZeroTailProbeReachedBackfill_001,
    TestSize.Level1)
{
    constexpr int32_t testTimeScale = 48000;
    constexpr int64_t testMdhdDuration = 199 * 1024;
    constexpr int64_t testFrameTicks = 1024;
    constexpr int64_t testSampleCount = 200;
    constexpr int64_t testEmptyEditTicks = 624;
    constexpr int64_t expectedFrameUs = 21333;
    std::vector<uint32_t> sampleSizes(testSampleCount, 1);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareAacTailTestTrack(plugin, sampleSizes, testTimeScale, testMdhdDuration,
        testFrameTicks, 0, true, testEmptyEditTicks, testMdhdDuration);
    ASSERT_NE(track, nullptr);
    // Same container shape as the unreached case above, but the probe reached the track so the
    // exported AAC frame size backfills the native 0 tail, exactly like the demov5 full-set cases.
    track->sampleHelper->sttsEntry_ = {{199, 1024}, {1, 0}};
    plugin.dataSource_ = CreateDataSourceForBytes(std::vector<uint8_t>(testSampleCount, 0x01));
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(256);
    for (size_t index = 0; index + 1 < testSampleCount; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
    ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MpegAudioTailStreamEndKeepsZeroDuration_001
 * @tc.desc: Verify an MP3 tail whose dts equals the stream end keeps duration 0 (FFmpeg
 *           mov_finalize_packet native 0; MP3 has no frame-size backfill for a zero-length
 *           tail) instead of borrowing the previous MP3 frame spacing.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MpegAudioTailStreamEndKeepsZeroDuration_001,
    TestSize.Level1)
{
    constexpr size_t testFrameCount = 3;
    constexpr int64_t testFrameTicks = MPEG_AUDIO_TEST_SAMPLES_PER_FRAME;
    std::vector<uint32_t> sampleSizes(testFrameCount, static_cast<uint32_t>(MPEG_AUDIO_TEST_FRAME_SIZE));
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareMpegAudioTestTrack(plugin, sampleSizes);
    ASSERT_NE(track, nullptr);
    // mdhd duration excludes the final frame: st->duration == last dts.
    track->duration = static_cast<int64_t>(testFrameCount - 1) * testFrameTicks;
    plugin.dataSource_ = CreateDataSourceForBytes(BuildMpegAudioTestFrames(testFrameCount));
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(MPEG_AUDIO_TEST_FRAME_SIZE);
    for (size_t index = 0; index + 1 < testFrameCount; ++index) {
        ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, MPEG_AUDIO_TEST_FRAME_DURATION_US) << "sample " << index;
    }
    ASSERT_EQ(plugin.ReadSample(MPEG_AUDIO_TEST_TRACK_ID, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->duration_, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EditedAudioTailAacFrameBackfill_001
 * @tc.desc: Verify an AAC tail past the raw media end (empty edit) keeps native 0 and is then
 *           backfilled from the probed AAC frame size (1024 ticks) exactly like FFmpeg
 *           compute_pkt_fields, instead of the nominal STTS spacing.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EditedAudioTailAacFrameBackfill_001, TestSize.Level1)
{
    constexpr int32_t testTimeScale = 32000;
    constexpr int64_t testMdhdDuration = 98548;
    constexpr int64_t testFrameTicks = 1024;
    constexpr int64_t testTailOverrideTicks = 627;
    constexpr int64_t testEmptyEditTicks = 2000;
    constexpr int64_t testRawTotalDuration = 94 * 1024 + 627;
    constexpr int64_t expectedFrameUs = 32000;
    std::vector<uint32_t> sampleSizes(95, 1);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareAacTailTestTrack(plugin, sampleSizes, testTimeScale, testMdhdDuration,
        testFrameTicks, testTailOverrideTicks, true, testEmptyEditTicks, testRawTotalDuration);
    ASSERT_NE(track, nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(std::vector<uint8_t>(95, 0x01));
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(256);
    for (size_t index = 0; index < 95; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EditedAudioTailInsideMediaKeepsNativeDuration_001
 * @tc.desc: Verify an AAC tail still inside the raw media (rawTailDuration > 0) keeps the native
 *           st->duration - dts value instead of the stts delta or the probed AAC frame duration,
 *           matching FFmpeg mov_finalize_packet (compute_pkt_fields only backfills duration <= 0).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EditedAudioTailInsideMediaKeepsNativeDuration_001,
    TestSize.Level1)
{
    constexpr int32_t testTimeScale = 48000;
    constexpr int32_t testSampleCount = 501;
    constexpr int64_t testFrameTicks = 1024;
    constexpr int64_t testEmptyEditTicks = 1016;
    constexpr int64_t testRawTotalDuration = testEmptyEditTicks +
        static_cast<int64_t>(testSampleCount - 1) * testFrameTicks + 16;
    constexpr int64_t expectedFrameUs = 21333;
    constexpr int64_t expectedTailUs = 333;
    std::vector<uint32_t> sampleSizes(testSampleCount, 1);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareAacTailTestTrack(plugin, sampleSizes, testTimeScale, testRawTotalDuration,
        testFrameTicks, testFrameTicks, true, testEmptyEditTicks, testRawTotalDuration);
    ASSERT_NE(track, nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(std::vector<uint8_t>(testSampleCount, 0x01));
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(256);
    for (size_t index = 0; index + 1 < testSampleCount; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
    ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->duration_, expectedTailUs);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcSpsFrameRateFromAvcConfig_001
 * @tc.desc: Verify avcC SPS timing_info parsing yields the FFmpeg codec frame rate.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcSpsFrameRateFromAvcConfig_001, TestSize.Level1)
{
    const std::vector<uint8_t> sps25Fps = {0x67, 0x42, 0x00, 0x1E, 0xF4, 0x0A, 0x0F, 0xD0,
        0x80, 0x00, 0x00, 0x03, 0x00, 0x80, 0x00, 0x00, 0x19, 0x20};
    const std::vector<uint8_t> spsNoTiming = {0x67, 0x42, 0x00, 0x1E, 0xF4, 0x0A, 0x0F, 0xD0, 0x00};
    const std::vector<uint8_t> spsBogusRate = {0x67, 0x42, 0x00, 0x1E, 0xF4, 0x0A, 0x0F, 0xD0,
        0x80, 0x00, 0x00, 0x03, 0x00, 0x80, 0x00, 0xAF, 0xC8, 0x40, 0x00};
    const std::vector<uint8_t> spsAvccAacMp3 = {0x67, 0x64, 0x00, 0x1F, 0xAC, 0xD9, 0x40, 0xB4,
        0x3D, 0xA1, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x00, 0x03, 0x00, 0x78, 0x0F, 0x18,
        0x31, 0x96, 0x00};
    const std::vector<uint8_t> spsDenis = {0x67, 0x4D, 0x40, 0x28, 0xEC, 0xA0, 0x58, 0x09,
        0x36, 0x08, 0xD4, 0x14, 0x04, 0x19, 0x00, 0x00, 0x03, 0x00, 0x01, 0x00, 0x02, 0xBF,
        0x20, 0x0F, 0x18, 0x31, 0x96, 0x00};
    AvcParserImpl parser;
    uint32_t frameRateNum = 0;
    uint32_t frameRateDen = 0;

    auto config = BuildAvcCTestConfig(sps25Fps);
    ASSERT_TRUE(parser.ParseSpsFrameRateFromAvcConfig(
        config.data(), static_cast<int32_t>(config.size()), frameRateNum, frameRateDen));
    EXPECT_EQ(frameRateNum, 25U);
    EXPECT_EQ(frameRateDen, 1U);

    config = BuildAvcCTestConfig(spsNoTiming);
    EXPECT_FALSE(parser.ParseSpsFrameRateFromAvcConfig(
        config.data(), static_cast<int32_t>(config.size()), frameRateNum, frameRateDen));

    config = BuildAvcCTestConfig(spsBogusRate);
    ASSERT_TRUE(parser.ParseSpsFrameRateFromAvcConfig(
        config.data(), static_cast<int32_t>(config.size()), frameRateNum, frameRateDen));
    EXPECT_EQ(frameRateNum, 45000U);
    EXPECT_EQ(frameRateDen, 1U);

    config = BuildAvcCTestConfig(spsAvccAacMp3);
    ASSERT_TRUE(parser.ParseSpsFrameRateFromAvcConfig(
        config.data(), static_cast<int32_t>(config.size()), frameRateNum, frameRateDen));
    EXPECT_EQ(frameRateNum, 60U);
    EXPECT_EQ(frameRateDen, 1U);

    config = BuildAvcCTestConfig(spsDenis);
    ASSERT_TRUE(parser.ParseSpsFrameRateFromAvcConfig(
        config.data(), static_cast<int32_t>(config.size()), frameRateNum, frameRateDen));
    EXPECT_EQ(frameRateNum, 90000U);
    EXPECT_EQ(frameRateDen, 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParserDimensionsOverrideContainer_001
 * @tc.desc: Verify SPS dimensions overwrite valid container dimensions like FFmpeg probing.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParserDimensionsOverrideContainer_001,
    TestSize.Level1)
{
    constexpr uint32_t trackId = 0;
    constexpr int32_t containerWidth = 2176;
    constexpr int32_t containerHeight = 2901;
    MPEG4DemuxerPlugin plugin(MPEG4_PLUGIN_NAME);
    plugin.streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin.streamParsers_, nullptr);
    ASSERT_EQ(plugin.streamParsers_->Create(trackId, VideoStreamType::AVC), Status::OK);

    auto config = H264_REORDER_TWO_AVCC_FIXTURE;
    ASSERT_TRUE(plugin.streamParsers_->ConvertExtraDataToAnnexb(
        trackId, config.data(), static_cast<int32_t>(config.size())));
    Plugins::MPEG4::MPEG4AtomParser::Track track;
    track.trackIndex = trackId;
    int32_t spsWidth = static_cast<int32_t>(plugin.streamParsers_->GetPicWidInLumaSamples(trackId));
    int32_t spsHeight = static_cast<int32_t>(plugin.streamParsers_->GetPicHetInLumaSamples(trackId));
    ASSERT_GT(spsWidth, 0);
    ASSERT_GT(spsHeight, 0);

    Meta format;
    format.Set<Tag::VIDEO_WIDTH>(containerWidth);
    format.Set<Tag::VIDEO_HEIGHT>(containerHeight);
    plugin.ParseAVCMetadataInfo(track, format);
    int32_t width = 0;
    int32_t height = 0;
    ASSERT_TRUE(format.Get<Tag::VIDEO_WIDTH>(width));
    ASSERT_TRUE(format.Get<Tag::VIDEO_HEIGHT>(height));
    EXPECT_EQ(width, spsWidth);
    EXPECT_EQ(height, spsHeight);

    Meta missingHeightFormat;
    missingHeightFormat.Set<Tag::VIDEO_WIDTH>(containerWidth);
    plugin.ParseAVCMetadataInfo(track, missingHeightFormat);
    ASSERT_TRUE(missingHeightFormat.Get<Tag::VIDEO_WIDTH>(width));
    ASSERT_TRUE(missingHeightFormat.Get<Tag::VIDEO_HEIGHT>(height));
    EXPECT_EQ(width, spsWidth);
    EXPECT_EQ(height, spsHeight);

    Meta zeroWidthFormat;
    zeroWidthFormat.Set<Tag::VIDEO_WIDTH>(0);
    zeroWidthFormat.Set<Tag::VIDEO_HEIGHT>(containerHeight);
    plugin.ParseAVCMetadataInfo(track, zeroWidthFormat);
    ASSERT_TRUE(zeroWidthFormat.Get<Tag::VIDEO_WIDTH>(width));
    ASSERT_TRUE(zeroWidthFormat.Get<Tag::VIDEO_HEIGHT>(height));
    EXPECT_EQ(width, spsWidth);
    EXPECT_EQ(height, spsHeight);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParserKeepContainerDimensions_001
 * @tc.desc: Verify FFmpeg init_dimensions rule keeps container dims when the SPS frame has
 *           no top/left crop and the container dims fit the SPS cropped frame on the 16-px grid.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParserKeepContainerDimensions_001, TestSize.Level1)
{
    constexpr uint32_t trackId = 0;
    constexpr int32_t containerWidth = 2176;
    constexpr int32_t containerHeight = 2901;
    MPEG4DemuxerPlugin plugin(MPEG4_PLUGIN_NAME);
    plugin.streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin.streamParsers_, nullptr);
    ASSERT_EQ(plugin.streamParsers_->Create(trackId, VideoStreamType::AVC), Status::OK);

    auto config = H264_ODDHEIGHT_CROP_AVCC_FIXTURE;
    ASSERT_TRUE(plugin.streamParsers_->ConvertExtraDataToAnnexb(
        trackId, config.data(), static_cast<int32_t>(config.size())));
    Plugins::MPEG4::MPEG4AtomParser::Track track;
    track.trackIndex = trackId;
    int32_t spsWidth = static_cast<int32_t>(plugin.streamParsers_->GetPicWidInLumaSamples(trackId));
    int32_t spsHeight = static_cast<int32_t>(plugin.streamParsers_->GetPicHetInLumaSamples(trackId));
    ASSERT_GT(spsWidth, 0);
    ASSERT_GT(spsHeight, 0);
    EXPECT_EQ(plugin.streamParsers_->GetSpsCropLeftOffset(trackId), 0U);
    EXPECT_EQ(plugin.streamParsers_->GetSpsCropTopOffset(trackId), 0U);

    Meta format;
    format.Set<Tag::VIDEO_WIDTH>(containerWidth);
    format.Set<Tag::VIDEO_HEIGHT>(containerHeight);
    plugin.ParseAVCMetadataInfo(track, format);
    int32_t width = 0;
    int32_t height = 0;
    ASSERT_TRUE(format.Get<Tag::VIDEO_WIDTH>(width));
    ASSERT_TRUE(format.Get<Tag::VIDEO_HEIGHT>(height));
    EXPECT_EQ(width, containerWidth);
    EXPECT_EQ(height, containerHeight);

    Meta misalignedFormat;
    misalignedFormat.Set<Tag::VIDEO_WIDTH>(1280);
    misalignedFormat.Set<Tag::VIDEO_HEIGHT>(720);
    plugin.ParseAVCMetadataInfo(track, misalignedFormat);
    ASSERT_TRUE(misalignedFormat.Get<Tag::VIDEO_WIDTH>(width));
    ASSERT_TRUE(misalignedFormat.Get<Tag::VIDEO_HEIGHT>(height));
    EXPECT_EQ(width, spsWidth);
    EXPECT_EQ(height, spsHeight);

    Meta oversizedFormat;
    oversizedFormat.Set<Tag::VIDEO_WIDTH>(containerWidth);
    oversizedFormat.Set<Tag::VIDEO_HEIGHT>(2910);
    plugin.ParseAVCMetadataInfo(track, oversizedFormat);
    ASSERT_TRUE(oversizedFormat.Get<Tag::VIDEO_WIDTH>(width));
    ASSERT_TRUE(oversizedFormat.Get<Tag::VIDEO_HEIGHT>(height));
    EXPECT_EQ(width, spsWidth);
    EXPECT_EQ(height, spsHeight);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CttsSpsFrameRatePacketDuration_001
 * @tc.desc: Verify ctts video packets derive duration from the SPS frame rate and keep 0 for bogus rates.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CttsSpsFrameRatePacketDuration_001, TestSize.Level1)
{
    constexpr int64_t expectedPlausibleDurationUs = 40000;
    constexpr int64_t bogusRateDurationUs = 0;
    const std::vector<uint8_t> sampleData = {0x00, 0x01};
    auto readFirstDuration = [sampleData](bool frameRateParsed, uint32_t frameRateNum,
        uint32_t frameRateDen) -> int64_t {
        Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
        auto track = PrepareVideoCttsTestTrack(plugin, sampleData);
        if (track == nullptr) {
            return INT64_MIN;
        }
        if (frameRateParsed) {
            track->codecFrameRateParsed = true;
            track->codecFrameRateNum = frameRateNum;
            track->codecFrameRateDen = frameRateDen;
        }
        plugin.dataSource_ = CreateDataSourceForBytes(sampleData);
        if (plugin.dataSource_ == nullptr) {
            return INT64_MIN;
        }
        AVBufferWrapper output(VIDEO_DURATION_READ_BUFFER_SIZE);
        if (plugin.ReadSample(0, output.mediaAVBuffer) != Status::OK) {
            return INT64_MIN;
        }
        EXPECT_EQ(output.mediaAVBuffer->pts_, 0);
        EXPECT_EQ(output.mediaAVBuffer->dts_, 0);
        return output.mediaAVBuffer->duration_;
    };

    // time_scale / (2 * num_units_in_tick) = 90000/3600 -> 25/1, plausible -> 40 ms.
    EXPECT_EQ(readFirstDuration(true, 25, 1), expectedPlausibleDurationUs);
    // Bogus rate like HandBrake 90000/1 fails the plausibility check and keeps duration 0.
    EXPECT_EQ(readFirstDuration(true, 45000, 1), bogusRateDurationUs);
    // Without parsed SPS timing the nominal stts delta (3600 ticks at 90 kHz) is used.
    EXPECT_EQ(readFirstDuration(false, 0, 0), expectedPlausibleDurationUs);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_VideoRFrameRatePacketDurationCtts_001
 * @tc.desc: Verify ctts H.264/HEVC packets derive duration from the estimated r_frame_rate
 *           (FFmpeg compute_frame_duration branch 1) instead of the nominal stts delta.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_VideoRFrameRatePacketDurationCtts_001, TestSize.Level1)
{
    constexpr uint32_t testSttsDelta = 3600;
    const std::vector<uint8_t> sampleData = {0x00, 0x01};
    auto readFirstDuration = [sampleData](const std::string& mime, int32_t timeScale,
        bool rateParsed, uint32_t rateNum, uint32_t rateDen) -> int64_t {
        Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
        VideoDurationTrackConfig config {sampleData, mime,
            {{static_cast<uint32_t>(sampleData.size()), testSttsDelta}}, true, timeScale};
        auto track = PrepareVideoDurationTestTrack(plugin, config);
        if (track == nullptr) {
            return INT64_MIN;
        }
        if (rateParsed) {
            track->rFrameRateParsed = true;
            track->rFrameRateNum = rateNum;
            track->rFrameRateDen = rateDen;
        }
        plugin.dataSource_ = CreateDataSourceForBytes(sampleData);
        if (plugin.dataSource_ == nullptr) {
            return INT64_MIN;
        }
        AVBufferWrapper output(VIDEO_DURATION_READ_BUFFER_SIZE);
        if (plugin.ReadSample(0, output.mediaAVBuffer) != Status::OK) {
            return INT64_MIN;
        }
        EXPECT_EQ(output.mediaAVBuffer->pts_, 0);
        EXPECT_EQ(output.mediaAVBuffer->dts_, 0);
        return output.mediaAVBuffer->duration_;
    };

    // 90 kHz / (355/12) rounded down to stream ticks: 3042 ticks -> 33800 us (line61 mode).
    EXPECT_EQ(readFirstDuration(MimeType::VIDEO_HEVC, 90000, true, 355, 12), 33800);
    // 90 kHz / (24000/1001): 3753 ticks -> 41700 us (bsover mode).
    EXPECT_EQ(readFirstDuration(MimeType::VIDEO_HEVC, 90000, true, 24000, 1001), 41700);
    // 90 kHz / (48000/1001): 1876 ticks -> 20844 us (H265 broken stts mode).
    EXPECT_EQ(readFirstDuration(MimeType::VIDEO_HEVC, 90000, true, 48000, 1001), 20844);
    // 15360 / (30/1): 512 ticks -> 33333 us (AVC_high mode).
    EXPECT_EQ(readFirstDuration(MimeType::VIDEO_AVC, 15360, true, 30, 1), 33333);
    // Without an estimated rate the nominal stts delta (3600 ticks at 90 kHz) is kept.
    EXPECT_EQ(readFirstDuration(MimeType::VIDEO_HEVC, 90000, false, 0, 0), 40000);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_VideoRFrameRatePacketDurationNoCtts_001
 * @tc.desc: Verify zero-dts video packets (broken edit list like line61/line77) derive duration
 *           from r_frame_rate while packets with a valid stts step keep the stts delta.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_VideoRFrameRatePacketDurationNoCtts_001, TestSize.Level1)
{
    const std::vector<uint8_t> sampleData = {0x00, 0x01, 0x02};

    // AVC: first two samples share dts 0 (zeroed timeline), so the first packet uses the
    // r_frame_rate fallback (3042 ticks -> 33800 us) and the second keeps the stts delta 3040.
    EXPECT_EQ(ReadDurationsWithRFrameRate(sampleData, MimeType::VIDEO_AVC, {{1, 0}, {2, 3040}}, 355, 12),
        std::vector<int64_t>({33800, 33777, 33777}));
    // HEVC: same zero-dts pattern with r_frame_rate 89/3 (3033 ticks -> 33700 us).
    EXPECT_EQ(ReadDurationsWithRFrameRate(sampleData, MimeType::VIDEO_HEVC, {{1, 0}, {2, 3031}}, 89, 3),
        std::vector<int64_t>({33700, 33677, 33677}));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_VideoTailStreamEndKeepsZeroDuration_001
 * @tc.desc: Verify a video tail whose dts equals the stream end keeps duration 0 when the SPS
 *           codec framerate is bogus (FFmpeg compute_frame_duration branch 4 fails), instead of
 *           reusing the nominal STTS spacing.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_VideoTailStreamEndKeepsZeroDuration_001,
    TestSize.Level1)
{
    constexpr int32_t testTimeScale = 1000000;
    constexpr int64_t testFrameTicks = 19330;
    constexpr int64_t expectedFrameUs = 19330;
    const std::vector<uint8_t> sampleData(5, 0x01);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    VideoDurationTrackConfig config {sampleData, MimeType::VIDEO_AVC,
        {{4, testFrameTicks}, {1, 0}}, false, testTimeScale};
    auto track = PrepareVideoDurationTestTrack(plugin, config);
    ASSERT_NE(track, nullptr);
    // mdhd duration equals the STTS total: the tail dts hits the stream end exactly.
    track->duration = 4 * testFrameTicks;
    // HandBrake-style SPS timing (1000000/1) fails the plausibility check, so FFmpeg keeps 0.
    track->codecFrameRateParsed = true;
    track->codecFrameRateNum = 1000000;
    track->codecFrameRateDen = 1;
    plugin.dataSource_ = CreateDataSourceForBytes(sampleData);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(VIDEO_DURATION_READ_BUFFER_SIZE);
    for (size_t index = 0; index < 4; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
    ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->duration_, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_VideoTailPlausibleSpsKeepsNominalSpacing_001
 * @tc.desc: Verify a video tail at the stream end with a plausible SPS codec framerate keeps the
 *           nominal STTS spacing, matching FFmpeg compute_frame_duration (r_frame_rate / codec
 *           framerate equals the stream spacing for constant-rate tracks, with the H.264 parser
 *           repeat_pict quirk also yielding the spacing).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_VideoTailPlausibleSpsKeepsNominalSpacing_001,
    TestSize.Level1)
{
    constexpr int32_t testTimeScale = 90000;
    constexpr int64_t testFrameTicks = 1500;
    constexpr int64_t expectedFrameUs = 16666;
    const std::vector<uint8_t> sampleData(5, 0x01);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    VideoDurationTrackConfig config {sampleData, MimeType::VIDEO_AVC,
        {{4, testFrameTicks}, {1, 0}}, false, testTimeScale};
    auto track = PrepareVideoDurationTestTrack(plugin, config);
    ASSERT_NE(track, nullptr);
    // mdhd duration equals the STTS total: the tail dts hits the stream end exactly.
    track->duration = 4 * testFrameTicks;
    // Plausible SPS timing (60/1) does not block the backfill, so the nominal spacing applies.
    track->codecFrameRateParsed = true;
    track->codecFrameRateNum = 60;
    track->codecFrameRateDen = 1;
    plugin.dataSource_ = CreateDataSourceForBytes(sampleData);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(VIDEO_DURATION_READ_BUFFER_SIZE);
    for (size_t index = 0; index < 5; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AuxVideoTailKeepsNominalSpacing_001
 * @tc.desc: Verify an auxiliary video track tail at the stream end keeps the nominal STTS spacing
 *           (FFmpeg estimates r_frame_rate for auxiliary video streams too; the mpeg4 demuxer has
 *           no per-track rate for auxiliary tracks, so the spacing backfill applies).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AuxVideoTailKeepsNominalSpacing_001,
    TestSize.Level1)
{
    constexpr int32_t testTimeScale = 90000;
    constexpr int64_t testFrameTicks = 3000;
    constexpr int64_t expectedFrameUs = 33333;
    const std::vector<uint8_t> sampleData(5, 0x01);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    VideoDurationTrackConfig config {sampleData, MimeType::VIDEO_HEVC,
        {{4, testFrameTicks}, {1, 0}}, false, testTimeScale};
    auto track = PrepareVideoDurationTestTrack(plugin, config);
    ASSERT_NE(track, nullptr);
    track->codecParms.trackType = Plugins::MPEG4::AUXILIARY_TYPE;
    // mdhd duration equals the STTS total: the tail dts hits the stream end exactly.
    track->duration = 4 * testFrameTicks;
    plugin.dataSource_ = CreateDataSourceForBytes(sampleData);
    ASSERT_NE(plugin.dataSource_, nullptr);
    AVBufferWrapper output(VIDEO_DURATION_READ_BUFFER_SIZE);
    for (size_t index = 0; index < 5; ++index) {
        ASSERT_EQ(plugin.ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ(output.mediaAVBuffer->duration_, expectedFrameUs) << "sample " << index;
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_RFrameRateEstimateFromStts_001
 * @tc.desc: Verify r_frame_rate is estimated from the first 100 raw stts timestamps exactly like
 *           FFmpeg ff_rfps_calculate for H.264/HEVC tracks (gcd and standard-rate branches).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_RFrameRateEstimateFromStts_001, TestSize.Level1)
{
    // line61 HEVC audio-shifted pattern: 355/12 @ 90 kHz.
    auto track0 = BuildRFrameRateTestTrack(MimeType::VIDEO_HEVC, 90000, RFPS_LINE61_STTS);
    // line77 HEVC zeroed-timeline pattern: 89/3 @ 90 kHz.
    auto track1 = BuildRFrameRateTestTrack(MimeType::VIDEO_HEVC, 90000, BuildLine77Stts());
    // H265_Main_L4 broken stts (1-tick runs) pattern: 48000/1001 @ 90 kHz.
    auto track2 = BuildRFrameRateTestTrack(MimeType::VIDEO_HEVC, 90000, RFPS_H265_L4_STTS);
    // AVC_high uniform 30 fps @ 15360: 30/1.
    auto track3 = BuildRFrameRateTestTrack(MimeType::VIDEO_AVC, 15360, RFPS_AVC_HIGH_STTS);
    ASSERT_NE(track0, nullptr);
    ASSERT_NE(track1, nullptr);
    ASSERT_NE(track2, nullptr);
    ASSERT_NE(track3, nullptr);
    track0->next = track1;
    track1->next = track2;
    track2->next = track3;

    auto parser = std::make_shared<Plugins::MPEG4::MPEG4AtomParser>();
    ASSERT_NE(parser, nullptr);
    parser->firstTrack_ = track0;
    parser->lastTrack_ = track3;
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    plugin.InitBaseInfoByAtomParser(parser);

    EXPECT_TRUE(track0->rFrameRateParsed);
    EXPECT_EQ(track0->rFrameRateNum, 355);
    EXPECT_EQ(track0->rFrameRateDen, 12);
    EXPECT_TRUE(track1->rFrameRateParsed);
    EXPECT_EQ(track1->rFrameRateNum, 89);
    EXPECT_EQ(track1->rFrameRateDen, 3);
    EXPECT_TRUE(track2->rFrameRateParsed);
    EXPECT_EQ(track2->rFrameRateNum, 48000);
    EXPECT_EQ(track2->rFrameRateDen, 1001);
    EXPECT_TRUE(track3->rFrameRateParsed);
    EXPECT_EQ(track3->rFrameRateNum, 30);
    EXPECT_EQ(track3->rFrameRateDen, 1);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToStart_ResetEosState_001
 * @tc.desc: Verify SeekToStart resets async EOS state even when sample index is already zero.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToStart_ResetEosState_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    ASSERT_NE(PrepareMpegAudioTestTrack(plugin, {MPEG_AUDIO_TEST_FRAME_SIZE}), nullptr);
    plugin.readToEnd_ = true;
    plugin.readLoopStatus_.store(Status::END_OF_STREAM, std::memory_order_release);

    ASSERT_EQ(plugin.SeekToStartInternal(), Status::OK);
    EXPECT_FALSE(plugin.readToEnd_);
    EXPECT_EQ(plugin.readLoopStatus_.load(std::memory_order_acquire), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_RapSampleGroupSync_001
 * @tc.desc: Verify rap sample groups add random-access samples to sync flags.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_RapSampleGroupSync_001, TestSize.Level1)
{
    std::vector<uint8_t> sbgp;
    AppendU32(sbgp, 0);
    AppendU32(sbgp, static_cast<uint32_t>(Plugins::MPEG4::FourccType("rap ")));
    AppendU32(sbgp, 6);
    for (const auto& entry : std::vector<std::pair<uint32_t, uint32_t>> {
        {60, 0}, {1, 1}, {59, 0}, {1, 1}, {58, 0}, {1, 1}}) {
        AppendU32(sbgp, entry.first);
        AppendU32(sbgp, entry.second);
    }
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.stssEntry_ = {{0}};
    helper.dataReader_ = CreateReaderForBytes(sbgp);
    ASSERT_EQ(helper.SetRapSampleGroupParams(0, sbgp.size()), Status::OK);
    EXPECT_EQ(helper.GetSyncSamplesIndexFromStssEntry(), std::vector<uint32_t>({0, 60, 120, 179}));

    std::vector<uint8_t> sbgpVersionOne;
    AppendU32(sbgpVersionOne, 0x01000000);
    AppendU32(sbgpVersionOne, static_cast<uint32_t>(Plugins::MPEG4::FourccType("rap ")));
    AppendU32(sbgpVersionOne, 0);
    AppendU32(sbgpVersionOne, RAP_GROUP_ENTRY_COUNT);
    AppendU32(sbgpVersionOne, 2);
    AppendU32(sbgpVersionOne, 1);
    Plugins::MPEG4::MPEG4SampleHelper versionOneHelper;
    versionOneHelper.dataReader_ = CreateReaderForBytes(sbgpVersionOne);
    ASSERT_EQ(versionOneHelper.SetRapSampleGroupParams(0, sbgpVersionOne.size()), Status::OK);
    EXPECT_EQ(versionOneHelper.GetSyncSamplesIndexFromStssEntry(), std::vector<uint32_t>({0, 1}));

    sbgpVersionOne.resize(12);
    Plugins::MPEG4::MPEG4SampleHelper truncatedVersionOneHelper;
    truncatedVersionOneHelper.dataReader_ = CreateReaderForBytes(sbgpVersionOne);
    EXPECT_EQ(truncatedVersionOneHelper.SetRapSampleGroupParams(0, sbgpVersionOne.size()),
        Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EmptyStssRapSampleGroupSync_001
 * @tc.desc: Verify rap sample groups override empty-stss first-sample fallback.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EmptyStssRapSampleGroupSync_001, TestSize.Level1)
{
    std::vector<uint8_t> sbgp;
    AppendU32(sbgp, 0);
    AppendU32(sbgp, static_cast<uint32_t>(Plugins::MPEG4::FourccType("rap ")));
    AppendU32(sbgp, 5);
    for (const auto& entry : std::vector<std::pair<uint32_t, uint32_t>> {
        {1, 1}, {59, 0}, {1, 1}, {58, 0}, {1, 1}}) {
        AppendU32(sbgp, entry.first);
        AppendU32(sbgp, entry.second);
    }
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.emptyStss_ = true;
    helper.dataReader_ = CreateReaderForBytes(sbgp);
    ASSERT_EQ(helper.SetRapSampleGroupParams(0, sbgp.size()), Status::OK);
    EXPECT_FALSE(helper.emptyStss_);
    EXPECT_EQ(helper.GetSyncSamplesIndexFromStssEntry(), std::vector<uint32_t>({0, 60, 119}));

    std::vector<uint8_t> nonKeyRap;
    AppendU32(nonKeyRap, 0);
    AppendU32(nonKeyRap, static_cast<uint32_t>(Plugins::MPEG4::FourccType("rap ")));
    AppendU32(nonKeyRap, RAP_GROUP_ENTRY_COUNT);
    AppendU32(nonKeyRap, RAP_GROUP_SAMPLE_COUNT);
    AppendU32(nonKeyRap, RAP_GROUP_NON_KEY_INDEX);
    Plugins::MPEG4::MPEG4SampleHelper fallbackHelper;
    fallbackHelper.emptyStss_ = true;
    fallbackHelper.dataReader_ = CreateReaderForBytes(nonKeyRap);
    ASSERT_EQ(fallbackHelper.SetRapSampleGroupParams(0, nonKeyRap.size()), Status::OK);
    EXPECT_FALSE(fallbackHelper.emptyStss_);
    EXPECT_TRUE(fallbackHelper.GetSyncSamplesIndexFromStssEntry().empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_StpsPartialSync_001
 * @tc.desc: Verify stps partial-sync samples are key and suppress empty-stss fallback.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_StpsPartialSync_001, TestSize.Level1)
{
    std::vector<uint8_t> stps;
    AppendU32(stps, 0);
    AppendU32(stps, 2);
    AppendU32(stps, 0);
    AppendU32(stps, PARTIAL_SYNC_SAMPLE);
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.emptyStss_ = true;
    helper.dataReader_ = CreateReaderForBytes(stps);
    ASSERT_EQ(helper.SetPartialSyncSampleParams(0, stps.size()), Status::OK);
    EXPECT_FALSE(helper.emptyStss_);
    EXPECT_EQ(helper.GetSyncSamplesIndexFromStssEntry(), std::vector<uint32_t>({0, 60}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EmptyStssAudioAllSync_001
 * @tc.desc: Verify an empty stss marks every sample of an audio/aux-audio track as sync (FFmpeg
 *           mov_read_stss keyframe_absent + mov_build_index need_parse_audio_info), while other
 *           tracks keep only the first sample.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EmptyStssAudioAllSync_001, TestSize.Level1)
{
    constexpr uint32_t testSampleCount = 4;
    auto audioHelper = BuildEmptyStssSyncTestHelper(Plugins::MPEG4::AUDIO_TYPE, MimeType::AUDIO_AAC);
    ASSERT_NE(audioHelper, nullptr);
    ASSERT_EQ(audioHelper->FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(audioHelper->EnsureDenseIndex(), Status::OK);
    ASSERT_EQ(audioHelper->GetSampleCount(), testSampleCount);
    for (uint32_t index = 0; index < testSampleCount; ++index) {
        EXPECT_TRUE(audioHelper->IsSyncSample(index));
    }
    auto auxAudioHelper = BuildEmptyStssSyncTestHelper(Plugins::MPEG4::AUXILIARY_TYPE, MimeType::AUDIO_AAC);
    ASSERT_NE(auxAudioHelper, nullptr);
    ASSERT_EQ(auxAudioHelper->FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(auxAudioHelper->EnsureDenseIndex(), Status::OK);
    for (uint32_t index = 0; index < testSampleCount; ++index) {
        EXPECT_TRUE(auxAudioHelper->IsSyncSample(index));
    }
    auto videoHelper = BuildEmptyStssSyncTestHelper(Plugins::MPEG4::VIDEO_TYPE, MimeType::VIDEO_AVC);
    ASSERT_NE(videoHelper, nullptr);
    ASSERT_EQ(videoHelper->FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(videoHelper->EnsureDenseIndex(), Status::OK);
    EXPECT_TRUE(videoHelper->IsSyncSample(0));
    for (uint32_t index = 1; index < testSampleCount; ++index) {
        EXPECT_FALSE(videoHelper->IsSyncSample(index));
    }
    auto videoAudioMimeHelper = BuildEmptyStssSyncTestHelper(Plugins::MPEG4::VIDEO_TYPE, MimeType::AUDIO_AAC);
    ASSERT_NE(videoAudioMimeHelper, nullptr);
    ASSERT_EQ(videoAudioMimeHelper->FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(videoAudioMimeHelper->EnsureDenseIndex(), Status::OK);
    EXPECT_TRUE(videoAudioMimeHelper->IsSyncSample(0));
    for (uint32_t index = 1; index < testSampleCount; ++index) {
        EXPECT_FALSE(videoAudioMimeHelper->IsSyncSample(index));
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SyncSamplesOutsideSampleTable_001
 * @tc.desc: Verify sync entries outside the finalized sample table are ignored
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SyncSamplesOutsideSampleTable_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper.mimeType_ = MimeType::VIDEO_AVC;
    helper.sttsEntry_ = {{2, 1}};
    helper.stscEntry_ = {{0, 2, 1}};
    helper.sampleSizes_ = {1, 1};
    helper.sampleSizeCount_ = 2;
    helper.chunkOffsets_ = {0};
    helper.stssEntry_ = {{2}};

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    EXPECT_TRUE(helper.GetSyncSamplesIndexFromStssEntry().empty());
    EXPECT_FALSE(helper.allSamplesSync_);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_Dca3ConfigValidation_001
 * @tc.desc: Verify AV3A sampling boundary, invalid sampling, invalid channel config and metadata rate.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_Dca3ConfigValidation_001, TestSize.Level1)
{
    auto runParse = [](uint8_t samplingIndex, uint8_t channelIndex, Status expected, int32_t* sampleRate) {
        Plugins::MPEG4::MPEG4AtomParser parser;
        if (parser.InitCurrentTrackForTrak() != Status::OK) {
            return false;
        }
        parser.dataReader_ = CreateReaderForBytes(BuildDca3Config(samplingIndex, channelIndex));
        Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
        atom.type = Plugins::MPEG4::FourccType("dca3");
        atom.dataSize = 6;
        Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
        if (parser.ParseDca3Entry(atom, &ctx) != expected) {
            return false;
        }
        if (sampleRate != nullptr) {
            return parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(*sampleRate);
        }
        return true;
    };

    int32_t sampleRate = 0;
    ASSERT_TRUE(runParse(8, 2, Status::OK, &sampleRate));
    EXPECT_EQ(sampleRate, 8000);
    EXPECT_TRUE(runParse(9, 2, Status::ERROR_INVALID_DATA, nullptr));
    EXPECT_TRUE(runParse(8, 4, Status::ERROR_INVALID_DATA, nullptr));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_FastInitReadSample_001
 * @tc.desc: Verify FAST_INIT keeps decoder metadata and sample reading available
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_FastInitReadSample_001, TestSize.Level1)
{
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(
        std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
    auto configs = std::make_shared<Meta>();
    ASSERT_NE(configs, nullptr);
    configs->Set<Tag::MEDIA_DEMUXER_MODE>(Plugins::DemuxerMode::FAST_INIT);
    InitPlugin(TEST_MP4_PATH_H264_B, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_, configs);

    int32_t trackCount = 0;
    ASSERT_TRUE(mpeg4MediaInfo_.general.Get<Tag::MEDIA_TRACK_COUNT>(trackCount));
    ASSERT_EQ(trackCount, static_cast<int32_t>(mpeg4MediaInfo_.tracks.size()));
    bool videoChecked = false;
    for (const auto& trackMeta : mpeg4MediaInfo_.tracks) {
        std::string mime;
        if (!trackMeta.Get<Tag::MIME_TYPE>(mime) || mime != MimeType::VIDEO_AVC) {
            continue;
        }
        int32_t width = 0;
        int32_t height = 0;
        std::vector<uint8_t> codecConfig;
        EXPECT_TRUE(trackMeta.Get<Tag::VIDEO_WIDTH>(width));
        EXPECT_TRUE(trackMeta.Get<Tag::VIDEO_HEIGHT>(height));
        EXPECT_GT(width, 0);
        EXPECT_GT(height, 0);
        EXPECT_TRUE(trackMeta.Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig));
        EXPECT_FALSE(codecConfig.empty());
        videoChecked = true;
    }
    ASSERT_TRUE(videoChecked);
    auto sample = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::COPY);
    EXPECT_GT(sample.actualSize, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_RbspReadUEBoundary_001
 * @tc.desc: Verify unsigned Exp-Golomb boundary avoids 32-bit shift overflow
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_RbspReadUEBoundary_001, TestSize.Level1)
{
    const uint8_t maxLeadingZeroUe[] = {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x00};
    RbspContext maxContext(maxLeadingZeroUe, static_cast<int32_t>(sizeof(maxLeadingZeroUe)));
    uint32_t value = 0;
    EXPECT_TRUE(maxContext.RbspReadUE(value));
    EXPECT_EQ(value, 0xFFFFFFFFU);

    RbspContext signedOverflowContext(maxLeadingZeroUe, static_cast<int32_t>(sizeof(maxLeadingZeroUe)));
    int32_t signedValue = 0;
    EXPECT_FALSE(signedOverflowContext.RbspReadSE(signedValue));

    const uint8_t overflowUe[] = {0x00, 0x00, 0x00, 0x00, 0x80, 0x00, 0x00, 0x00, 0x80};
    RbspContext overflowContext(overflowUe, static_cast<int32_t>(sizeof(overflowUe)));
    value = 0;
    EXPECT_FALSE(overflowContext.RbspReadUE(value));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_RbspGetBitsBoundary_001
 * @tc.desc: Verify RbspGetBits handles zero, oversized and unaligned 32-bit reads safely
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_RbspGetBitsBoundary_001, TestSize.Level1)
{
    const uint8_t unalignedData[] = {0x0F, 0xFF, 0xFF, 0xFF, 0xF0};
    RbspContext unalignedContext(unalignedData, static_cast<int32_t>(sizeof(unalignedData)));
    EXPECT_EQ(static_cast<uint32_t>(unalignedContext.RbspGetBits<uint8_t>(4)), 0U);
    EXPECT_EQ((unalignedContext.RbspGetBits<uint32_t, uint64_t>(32)), 0xFFFFFFFFU);

    RbspContext invalidContext(unalignedData, static_cast<int32_t>(sizeof(unalignedData)));
    EXPECT_EQ((invalidContext.RbspGetBits<uint32_t, uint32_t>(0)), 0U);
    EXPECT_EQ(static_cast<uint32_t>(invalidContext.RbspGetBits<uint8_t, uint32_t>(9)), 0U);
    EXPECT_EQ((invalidContext.RbspGetBits<uint32_t, uint32_t>(33)), 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_LifecycleMetaAndStubInterfaces_001
 * @tc.desc: Verify MPEG4 demuxer lifecycle, meta, probe, drm and reference-parser interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_LifecycleMetaAndStubInterfaces_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);

    MediaInfo mediaInfo;
    EXPECT_EQ(plugin->GetMediaInfo(mediaInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->Start(), Status::OK);
    EXPECT_EQ(plugin->Stop(), Status::OK);
    EXPECT_EQ(plugin->Pause(), Status::OK);
    EXPECT_EQ(plugin->Flush(), Status::OK);
    EXPECT_EQ(plugin->Reset(), Status::OK);
    plugin->ResetEosStatus();
    plugin->SetCacheLimit(1024);
    plugin->SetInterruptState(false);
    EXPECT_EQ(plugin->SetReadTimeoutForInitSeek(10), Status::OK);
    EXPECT_EQ(plugin->SetAVReadPacketStopState(false), Status::OK);
    EXPECT_EQ(plugin->SetCachePressureCallback(nullptr), Status::OK);
    EXPECT_EQ(plugin->SetTrackCacheLimit(0, 1024), Status::OK);

    int32_t offset = -1;
    int32_t size = -1;
    EXPECT_TRUE(plugin->GetProbeSize(offset, size));
    EXPECT_EQ(offset, 0);
    EXPECT_GT(size, 0);

    std::multimap<std::string, std::vector<uint8_t>> drmInfo;
    EXPECT_EQ(plugin->GetDrmInfo(drmInfo), Status::ERROR_NULL_POINTER);
    EXPECT_FALSE(plugin->IsRefParserSupported());
    EXPECT_EQ(plugin->ParserRefUpdatePos(0), Status::ERROR_UNSUPPORTED_FORMAT);
    EXPECT_EQ(plugin->ParserRefInfo(), Status::ERROR_UNKNOWN);

    auto buffer = MediaAVBuffer::CreateAVBuffer();
    FrameLayerInfo frameLayerInfo;
    GopLayerInfo gopLayerInfo;
    std::vector<uint32_t> iFramePos;
    uint32_t frameId = 0;
    int64_t seekMs = 0;
    EXPECT_EQ(plugin->GetFrameLayerInfo(buffer, frameLayerInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->GetFrameLayerInfo(frameId, frameLayerInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->GetGopLayerInfo(0, gopLayerInfo), Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->GetIFramePos(iFramePos), Status::ERROR_UNKNOWN);
    EXPECT_EQ(plugin->Dts2FrameId(0, frameId), Status::ERROR_OUT_OF_RANGE);
    EXPECT_EQ(plugin->SeekMs2FrameId(0, frameId), Status::ERROR_OUT_OF_RANGE);
    EXPECT_EQ(plugin->FrameId2SeekMs(frameId, seekMs), Status::ERROR_UNKNOWN);

    int64_t realSeekTime = 0;
    EXPECT_EQ(plugin->SeekToKeyFrame(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(plugin->SeekToFrameByDts(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0),
        Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_001
 * @tc.desc: Verify MPEG4 demuxer cache pressure callback and per-track limit controls
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, 1), Status::OK);

    uint32_t initialFrameCount = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, initialFrameCount), Status::OK);
    uint32_t initialCacheSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, initialCacheSize), Status::OK);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    EXPECT_EQ(ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);

    uint32_t frameCount = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, frameCount), Status::OK);
    uint32_t cacheSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, cacheSize), Status::OK);
    EXPECT_GT(callbackCount.load(), 0);
    EXPECT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 0), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_Disable_001
 * @tc.desc: Verify cache pressure notification stops after disabling per-track limit
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_Disable_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, 1), Status::OK);

    int64_t realSeekTime = 0;
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    ASSERT_GT(callbackCount.load(), 0);

    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 0), Status::OK);
    callbackCount.store(0);
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 40,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    EXPECT_EQ(callbackCount.load(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_Throttle_001
 * @tc.desc: Verify cache pressure callback is throttled by per-track notification window
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_Throttle_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    constexpr uint32_t throttleWindowMs = 10000;
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, throttleWindowMs), Status::OK);

    int64_t realSeekTime = 0;
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    int32_t firstCallbackCount = callbackCount.load();
    ASSERT_GT(firstCallbackCount, 0);

    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 40,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 80,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    EXPECT_EQ(callbackCount.load(), firstCallbackCount);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CachePressure_AfterSeek_001
 * @tc.desc: Verify cache pressure and cache query remain valid after DTS seek
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CachePressure_AfterSeek_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    uint32_t trackId = selectedTrackIds.front();

    std::atomic<int32_t> callbackCount {0};
    ASSERT_EQ(mpeg4Plugin_->SetCachePressureCallback(
        [&callbackCount](uint32_t callbackTrackId, uint32_t cachedBytes) {
            (void)callbackTrackId;
            (void)cachedBytes;
            ++callbackCount;
        }), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SetTrackCacheLimit(trackId, 1, 1), Status::OK);

    int64_t realSeekTime = 0;
    ASSERT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(trackId), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
    EXPECT_GT(callbackCount.load(), 0);

    uint32_t frameCount = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheFrameCount(trackId, frameCount), Status::OK);
    EXPECT_GT(frameCount, 0);
    uint32_t cacheSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetCurrentCacheSize(trackId, cacheSize), Status::OK);
    EXPECT_GT(cacheSize, 0);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
    EXPECT_EQ(buffer.mediaAVBuffer->dts_, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToFrameByDts_001
 * @tc.desc: Verify MPEG4 demuxer can seek to the first frame whose DTS reaches the target
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToFrameByDts_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckSeekToFrameByDtsResult(TEST_MP4_PATH_H264_B, "SeekToFrameByDts_001"));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToFrameByDts_InvalidParam_001
 * @tc.desc: Verify MPEG4 demuxer SeekToFrameByDts parameter checks
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToFrameByDts_InvalidParam_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    int64_t realSeekTime = 0;
    EXPECT_EQ(plugin->SeekToFrameByDts(0, 0, SeekMode::SEEK_CLOSEST, realSeekTime, 0),
        Status::ERROR_NULL_POINTER);
    EXPECT_EQ(plugin->SeekToFrameByDts(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0),
        Status::ERROR_NULL_POINTER);

    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(0, 0, SeekMode::SEEK_CLOSEST, realSeekTime, 0),
        Status::ERROR_INVALID_OPERATION);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(-1, 0, SeekMode::SEEK_CLOSEST, realSeekTime, 0),
        Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutFail_001
 * @tc.desc: Verify MPEG4 demuxer init fails when ReadAt keeps returning ERROR_AGAIN beyond timeout
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutFail_001, TestSize.Level1)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(TEST_MP4_PATH_H264_B.c_str(), &fileStatus), 0);
    int fd = open(TEST_MP4_PATH_H264_B.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    auto streamDemuxer = std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(0, 1000);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_EQ(plugin->SetReadTimeoutForInitSeek(100), Status::OK);
    EXPECT_NE(plugin->SetDataSource(dataSource), Status::OK);
    close(fd);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetDataSource_ZeroCountSttsUlawUnsupported_001
 * @tc.desc: A moov whose stts carries sampleCount==0 must not fail at the stts table; parsing
 *           continues to the sample entry where the unsupported 'ulaw' codec surfaces
 *           ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT so the plugin manager falls back to ffmpeg.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetDataSource_ZeroCountSttsUlawUnsupported_001,
    TestSize.Level1)
{
    auto dataSource = CreateDataSourceForBytes(BuildMinimalMp4WithZeroCountSttsAndUlaw());
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    EXPECT_EQ(plugin->SetDataSource(dataSource), Status::ERROR_MPEG4_DEMUXER_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParseHeader_TrailingBytesLessThanAtomHeader_001
 * @tc.desc: A seekable mp4 with fewer than 8 trailing bytes (e.g. "末尾多7个无效字节.mp4") must
 *           init successfully: FFmpeg stops at EOF when a full atom header cannot be read and
 *           keeps the parsed moov, so mpeg4 must not fail the whole init with ERROR_INVALID_DATA.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParseHeader_TrailingBytesLessThanAtomHeader_001,
    TestSize.Level1)
{
    for (size_t trailingBytes = 0; trailingBytes < MP4_ATOM_HEADER_SIZE; ++trailingBytes) {
        AtomParserTestContext context(BuildMinimalMp4WithTrailingBytes(trailingBytes));
        EXPECT_EQ(context.parser.MPEG4ParseHeader(context.dataSource, Seekable::SEEKABLE, -1,
            Plugins::DemuxerMode::FULL_PARSE), Status::OK) << "trailingBytes=" << trailingBytes;
        EXPECT_TRUE(context.parser.moovFound_) << "trailingBytes=" << trailingBytes;
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParseHeader_TrailingAtomHeaderExceedsSource_001
 * @tc.desc: A full 8-byte atom header remains but its declared size exceeds the source: this is a
 *           genuinely truncated file and must fail init instead of being tolerated like trailing
 *           garbage bytes that cannot form an atom header.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParseHeader_TrailingAtomHeaderExceedsSource_001,
    TestSize.Level1)
{
    auto file = BuildMinimalMp4WithTrailingBytes(0);
    auto truncatedHeader = BuildAtomHeader(MP4_ATOM_HEADER_SIZE, Plugins::MPEG4::FourccType("xxxx"), false);
    file.insert(file.end(), truncatedHeader.begin(), truncatedHeader.end());
    AtomParserTestContext context(file);
    EXPECT_EQ(context.parser.MPEG4ParseHeader(context.dataSource, Seekable::SEEKABLE, -1,
        Plugins::DemuxerMode::FULL_PARSE), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutSuccess_001
 * @tc.desc: Verify MPEG4 demuxer init succeeds when read timeout is explicitly disabled
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForInit_TimeoutSuccess_001, TestSize.Level1)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(TEST_MP4_PATH_H264_B.c_str(), &fileStatus), 0);
    int fd = open(TEST_MP4_PATH_H264_B.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
    auto source = std::make_shared<Source>();
    ASSERT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    auto streamDemuxer = std::make_shared<Mpeg4StreamDemuxerPullDataFailMock>(0, 20);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    ASSERT_EQ(streamDemuxer->Init(""), Status::OK);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_EQ(plugin->SetReadTimeoutForInitSeek(0), Status::OK);
    EXPECT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    close(fd);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_001
 * @tc.desc: Verify MPEG4 demuxer DTS seek fails when prefetch read times out
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 1000);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(100), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_NE(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_002
 * @tc.desc: Verify SeekToFrameByDts timeout limits prefetch read even when internal read timeout is longer
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutFail_002, TestSize.Level1)
{
    constexpr uint32_t apiTimeoutMs = 100;
    constexpr uint32_t internalTimeoutMs = 2000;
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 1000, 20);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(internalTimeoutMs), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_NE(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, apiTimeoutMs), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_001
 * @tc.desc: Verify MPEG4 demuxer DTS seek succeeds when read timeout is disabled
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 20);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(0), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, 0), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_002
 * @tc.desc: Verify MPEG4 demuxer DTS seek succeeds when read timeout is long enough
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetReadTimeoutForSeek_TimeoutSuccess_002, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, 0, 20);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    ASSERT_EQ(mpeg4Plugin_->SetReadTimeoutForInitSeek(2000), Status::OK);
    int64_t realSeekTime = 0;
    EXPECT_EQ(mpeg4Plugin_->SeekToFrameByDts(static_cast<int32_t>(selectedTrackIds.front()), 0,
        SeekMode::SEEK_CLOSEST, realSeekTime, 2000), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_InitDrmInfoCache_PsshUuid_001
 * @tc.desc: Verify only PSSH entries with a complete UUID are cached
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InitDrmInfoCache_PsshUuid_001, TestSize.Level1)
{
    constexpr uint32_t psshUuidSize = 16;
    auto parser = std::make_shared<Plugins::MPEG4::MPEG4AtomParser>();
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(parser, nullptr);
    ASSERT_NE(plugin, nullptr);

    Plugins::MPEG4::MPEG4AtomParser::Mpeg4PsshInfo validPssh;
    validPssh.uuidLen = psshUuidSize;
    validPssh.psshData = {0x10, 0x20, 0x30};
    for (uint32_t index = 0; index < psshUuidSize; ++index) {
        validPssh.uuid[index] = static_cast<uint8_t>(index);
    }
    parser->psshList_.push_back(validPssh);

    auto invalidPssh = validPssh;
    invalidPssh.uuidLen = psshUuidSize - 1;
    parser->psshList_.push_back(invalidPssh);
    plugin->InitDrmInfoCacheByAtomParser(parser);

    ASSERT_EQ(plugin->cachedDrmInfo_.size(), 1U);
    auto cachedPssh = plugin->cachedDrmInfo_.find("000102030405060708090a0b0c0d0e0f");
    ASSERT_NE(cachedPssh, plugin->cachedDrmInfo_.end());
    EXPECT_EQ(cachedPssh->second, validPssh.psshData);
    EXPECT_TRUE(plugin->drmInfoCached_.load());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetDataSource_DemuxerMode_001
 * @tc.desc: Verify SetDataSource reads the configured demuxer mode
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetDataSource_DemuxerMode_001, TestSize.Level1)
{
    int fd = -1;
    auto dataSource = CreateFileDataSource(TEST_MP4_PATH_H264_B, fd);
    ASSERT_NE(dataSource, nullptr);
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    auto configs = std::make_shared<Meta>();
    ASSERT_NE(plugin, nullptr);
    ASSERT_NE(configs, nullptr);
    configs->SetData(Tag::MEDIA_DEMUXER_MODE, Plugins::DemuxerMode::FAST_INIT);

    EXPECT_EQ(plugin->SetDataSource(dataSource, configs), Status::OK);
    EXPECT_EQ(plugin->demuxerMode_, Plugins::DemuxerMode::FAST_INIT);
    EXPECT_EQ(plugin->Reset(), Status::OK);
    if (fd >= 0) {
        close(fd);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetDataSourceWithProbSize_001
 * @tc.desc: Verify SetDataSourceWithProbSize parameter checks and normal initialization path
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetDataSourceWithProbSize_001, TestSize.Level1)
{
    int fd = -1;
    auto dataSource = CreateFileDataSource(TEST_MP4_PATH_H264_B, fd);
    ASSERT_NE(dataSource, nullptr);

    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(nullptr, 0), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(dataSource, -1), Status::ERROR_INVALID_PARAMETER);
    auto setDataSourceWithProbeRet = plugin->SetDataSourceWithProbSize(dataSource, 1024);
    if (setDataSourceWithProbeRet != Status::OK) {
        EXPECT_EQ(setDataSourceWithProbeRet, Status::ERROR_INVALID_PARAMETER);
        ASSERT_EQ(plugin->SetDataSource(dataSource), Status::OK);
    }

    MediaInfo mediaInfo;
    EXPECT_EQ(plugin->GetMediaInfo(mediaInfo), Status::OK);
    EXPECT_FALSE(mediaInfo.tracks.empty());
    EXPECT_EQ(plugin->SetDataSourceWithProbSize(dataSource, 1024), Status::ERROR_WRONG_STATE);
    EXPECT_EQ(plugin->Reset(), Status::OK);
    if (fd >= 0) {
        close(fd);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_UnseekableSniff_UsesFfmpeg_001
 * @tc.desc: Verify MPEG4 sniff does not consume sequential data beyond the shared prefix cache
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_UnseekableSniff_UsesFfmpeg_001, TestSize.Level1)
{
    constexpr size_t lateBrandCount = 5000;
    constexpr size_t lateIsomBrandIndex = 4200;
    auto data = BuildFtypWithLateIsomBrand(lateBrandCount, lateIsomBrandIndex);
    auto source = std::make_shared<SequentialReadDataSource>(data);
    auto prefixCache = std::make_shared<std::vector<uint8_t>>();
    auto typeFinder = std::make_shared<TypeFinder>();
    auto checkRange = [](int32_t, uint64_t, size_t) { return Status::OK; };
    auto peekRange = [source, prefixCache](int32_t, uint64_t offset, size_t size,
        std::shared_ptr<Buffer>& buffer, bool) {
        return ReadFromSequentialSourceWithPrefixCache(source, prefixCache, offset, size, buffer);
    };
    typeFinder->Init("sequential.mp4", data.size(), checkRange, peekRange,
        TypeFinder::StreamContext {0, Seekable::UNSEEKABLE});

    constexpr size_t initialPrefixSize = 16U * 1024U;
    std::vector<uint8_t> initialPrefix(initialPrefixSize);
    auto prefixBuffer = std::make_shared<Buffer>();
    ASSERT_NE(prefixBuffer->WrapMemory(initialPrefix.data(), initialPrefixSize, initialPrefixSize), nullptr);
    ASSERT_EQ(typeFinder->ReadAt(0, prefixBuffer, initialPrefixSize), Status::OK);
    EXPECT_EQ(typeFinder->GetSeekable(), Seekable::UNSEEKABLE);
    auto mpeg4Sniffer = GetDemuxerSniffer(MPEG4_PLUGIN_NAME);
    ASSERT_NE(mpeg4Sniffer, nullptr);
    EXPECT_EQ(mpeg4Sniffer(MPEG4_PLUGIN_NAME, typeFinder), 0);
    EXPECT_EQ(source->requestedOffsets.size(), 1U);
    EXPECT_EQ(source->GetCursor(), initialPrefixSize);
    EXPECT_TRUE(std::equal(initialPrefix.begin(), initialPrefix.end(), prefixCache->begin()));

    EXPECT_EQ(typeFinder->FindMediaType(), FFMPEG_MP4_PLUGIN_NAME);
    EXPECT_TRUE(std::equal(data.begin(), data.begin() + 8, prefixCache->begin()));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_UnseekableSniff_SkipCondition_001
 * @tc.desc: Verify MPEG4 sniff remains enabled for seekable and invalid seekability states
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_UnseekableSniff_SkipCondition_001, TestSize.Level1)
{
    constexpr size_t sniffBufferSize = 4096;
    auto data = BuildFtypWithLateIsomBrand(1, 0);
    data.resize(sniffBufferSize, 0);
    auto mpeg4Sniffer = GetDemuxerSniffer(MPEG4_PLUGIN_NAME);
    ASSERT_NE(mpeg4Sniffer, nullptr);
    for (Seekable seekable : {Seekable::SEEKABLE, Seekable::INVALID}) {
        auto source = std::make_shared<SequentialReadDataSource>(data);
        auto prefixCache = std::make_shared<std::vector<uint8_t>>();
        auto typeFinder = std::make_shared<TypeFinder>();
        auto checkRange = [](int32_t, uint64_t, size_t) { return Status::OK; };
        auto peekRange = [source, prefixCache](int32_t, uint64_t offset, size_t size,
            std::shared_ptr<Buffer>& buffer, bool) {
            return ReadFromSequentialSourceWithPrefixCache(source, prefixCache, offset, size, buffer);
        };
        typeFinder->Init("state.mp4", data.size(), checkRange, peekRange, TypeFinder::StreamContext {0, seekable});
        EXPECT_EQ(typeFinder->GetSeekable(), seekable);
        auto prefixBuffer = Buffer::CreateDefaultBuffer(sniffBufferSize);
        ASSERT_NE(prefixBuffer, nullptr);
        ASSERT_EQ(typeFinder->ReadAt(0, prefixBuffer, sniffBufferSize), Status::OK);
        EXPECT_GT(mpeg4Sniffer(MPEG4_PLUGIN_NAME, typeFinder), 0);
        EXPECT_EQ(source->requestedOffsets.size(), 1U);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_UnseekableSniff_NoCandidate_001
 * @tc.desc: Verify unrecognized unseekable data remains unidentified when no sniffer scores it
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_UnseekableSniff_NoCandidate_001, TestSize.Level1)
{
    std::vector<uint8_t> data(4096, 0);
    auto source = std::make_shared<SequentialReadDataSource>(data);
    auto prefixCache = std::make_shared<std::vector<uint8_t>>();
    auto typeFinder = std::make_shared<TypeFinder>();
    auto checkRange = [](int32_t, uint64_t, size_t) { return Status::OK; };
    auto peekRange = [source, prefixCache](int32_t, uint64_t offset, size_t size,
        std::shared_ptr<Buffer>& buffer, bool) {
        return ReadFromSequentialSourceWithPrefixCache(source, prefixCache, offset, size, buffer);
    };
    typeFinder->Init("unknown.bin", data.size(), checkRange, peekRange,
        TypeFinder::StreamContext {0, Seekable::UNSEEKABLE});

    EXPECT_TRUE(typeFinder->FindMediaType().empty());
    EXPECT_EQ(source->requestedOffsets.size(), 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetDataSource_AllDemuxerResources_001
 * @tc.desc: Verify FFmpeg and MPEG4 demuxer SetDataSource initialization coverage for mov/mp4 family resources
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetDataSource_AllDemuxerResources_001, TestSize.Level1)
{
    auto filePaths = CollectDemuxerResourceFiles();
    ASSERT_FALSE(filePaths.empty());

    size_t ffmpegOkCount = 0;
    size_t mpeg4OkCount = 0;
    for (const auto& filePath : filePaths) {
        SCOPED_TRACE(filePath);
        auto ffmpegBasePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(FFMPEG_MP4_PLUGIN_NAME);
        auto ffmpegPlugin = std::static_pointer_cast<BaseDemuxerPlugin>(
            std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(ffmpegBasePlugin));
        ASSERT_NE(ffmpegPlugin, nullptr);
        auto ffmpegResult = RunSetDataSourceOnly(ffmpegPlugin, filePath);
        if (ffmpegResult.ret == Status::OK) {
            ++ffmpegOkCount;
        }

        auto mpeg4Plugin = std::static_pointer_cast<BaseDemuxerPlugin>(
            std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4"));
        ASSERT_NE(mpeg4Plugin, nullptr);
        auto mpeg4Result = RunSetDataSourceOnly(mpeg4Plugin, filePath);
        if (mpeg4Result.ret == Status::OK) {
            ++mpeg4OkCount;
        }
        auto ignoredFields = GetKnownIssueFields(filePath);
        ExpectSetDataSourceCompatible(filePath, ffmpegResult, mpeg4Result, ignoredFields);
    }

    EXPECT_GT(ffmpegOkCount, 0U);
    EXPECT_GT(mpeg4OkCount, 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacImplicitSbrEditListParity_001
 * @tc.desc: Verify FFmpeg and MPEG4 agree on implicit SBR metadata after an edit-list skip.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacImplicitSbrEditListParity_001, TestSize.Level1)
{
    const std::string path = TEST_DEMUXER_RESOURCE_DIR + "/aac_implicit_sbr_editlist.mp4";
    auto getAacTrack = [](const MediaInfo& info) -> const Meta* {
        for (const auto& track : info.tracks) {
            Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
            std::string mime;
            if (track.Get<Tag::MEDIA_TYPE>(mediaType) && mediaType == Plugins::MediaType::AUDIO &&
                track.Get<Tag::MIME_TYPE>(mime) && (mime.find("mp4a") != std::string::npos ||
                mime.find("aac") != std::string::npos)) {
                return &track;
            }
        }
        return nullptr;
    };

    auto ffmpegPlugin = std::make_shared<FFmpegDemuxerPlugin>(FFMPEG_MP4_PLUGIN_NAME);
    auto mpeg4Plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(ffmpegPlugin, nullptr);
    ASSERT_NE(mpeg4Plugin, nullptr);
    auto ffmpeg = RunSetDataSourceOnly(ffmpegPlugin, path);
    auto mpeg4 = RunSetDataSourceOnly(mpeg4Plugin, path);
    ASSERT_EQ(ffmpeg.ret, Status::OK) << path;
    ASSERT_EQ(ffmpeg.mediaInfoRet, Status::OK) << path;
    ASSERT_EQ(mpeg4.ret, Status::OK) << path;
    ASSERT_EQ(mpeg4.mediaInfoRet, Status::OK) << path;
    const auto* ffmpegTrack = getAacTrack(ffmpeg.mediaInfo);
    const auto* mpeg4Track = getAacTrack(mpeg4.mediaInfo);
    ASSERT_NE(ffmpegTrack, nullptr) << path << " FFmpeg AAC track missing";
    ASSERT_NE(mpeg4Track, nullptr) << path << " MPEG4 AAC track missing";
    std::string ffmpegMime;
    std::string mpeg4Mime;
    int32_t ffmpegRate = 0;
    int32_t mpeg4Rate = 0;
    int32_t ffmpegFrameSize = 0;
    int32_t mpeg4FrameSize = 0;
    ASSERT_TRUE(ffmpegTrack->Get<Tag::MIME_TYPE>(ffmpegMime));
    ASSERT_TRUE(mpeg4Track->Get<Tag::MIME_TYPE>(mpeg4Mime));
    ASSERT_TRUE(ffmpegTrack->Get<Tag::AUDIO_SAMPLE_RATE>(ffmpegRate));
    ASSERT_TRUE(mpeg4Track->Get<Tag::AUDIO_SAMPLE_RATE>(mpeg4Rate));
    ASSERT_TRUE(ffmpegTrack->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(ffmpegFrameSize));
    ASSERT_TRUE(mpeg4Track->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(mpeg4FrameSize));
    std::cout << "[AacImplicitSbrEditListParity] file=" << path << " ffmpeg=" << ffmpegMime << ":" <<
        ffmpegRate << "/" << ffmpegFrameSize << " mpeg4=" << mpeg4Mime <<
        ":" << mpeg4Rate << "/" << mpeg4FrameSize << std::endl;
    EXPECT_EQ(ffmpegMime, mpeg4Mime) << path;
    EXPECT_EQ(ffmpegRate, mpeg4Rate) << path;
    EXPECT_EQ(ffmpegFrameSize, mpeg4FrameSize) << path;
    EXPECT_EQ(ffmpegRate, 48000) << path;
    EXPECT_EQ(ffmpegFrameSize, 2048) << path;
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroSttsHvc1Parity_001
 * @tc.desc: Verify MPEG4 and FFmpeg expose identical samples for an HEVC track whose STTS deltas are zero
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ZeroSttsHvc1Parity_001, TestSize.Level1)
{
    ffmpegPlugin_ = std::make_shared<FFmpegDemuxerPlugin>(FFMPEG_MP4_PLUGIN_NAME);
    auto mpeg4 = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    mpeg4Plugin_ = mpeg4;
    InitPlugin(ZERO_STTS_HVC1_FIXTURE, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
    InitPlugin(ZERO_STTS_HVC1_FIXTURE, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
    auto ffmpegTrackIds = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4TrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    uint32_t ffmpegTrackId = FindFirstVideoTrack(ffmpegMediaInfo_, ffmpegTrackIds);
    uint32_t mpeg4TrackId = FindFirstVideoTrack(mpeg4MediaInfo_, mpeg4TrackIds);
    ASSERT_NE(ffmpegTrackId, INVALID_TRACK_ID);
    ASSERT_NE(mpeg4TrackId, INVALID_TRACK_ID);

    auto readSample = [this](const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
        const std::shared_ptr<MediaAVBuffer>& buffer) {
        return ReadSampleWithRetry(plugin, trackId, buffer, ReadMode::COPY);
    };
    auto isEos = [this](const std::shared_ptr<MediaAVBuffer>& buffer) { return IsEos(buffer); };
    auto ffmpegSamples = ReadTrackSamples(ffmpegPlugin_, ffmpegTrackId, readSample, isEos);
    auto mpeg4Samples = ReadTrackSamples(mpeg4Plugin_, mpeg4TrackId, readSample, isEos);
    ASSERT_EQ(ffmpegSamples.size(), 1U);
    ASSERT_EQ(mpeg4Samples.size(), ffmpegSamples.size());
    EXPECT_EQ(mpeg4Samples[0].pts, ffmpegSamples[0].pts);
    EXPECT_EQ(mpeg4Samples[0].dts, ffmpegSamples[0].dts);
    EXPECT_EQ(mpeg4Samples[0].duration, ffmpegSamples[0].duration);
    EXPECT_EQ(mpeg4Samples[0].flag, ffmpegSamples[0].flag);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacImplicitSbrLeadInParity_001
 * @tc.desc: Verify MPEG4 matches FFmpeg when only the pre-roll access units carry implicit SBR.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacImplicitSbrLeadInParity_001, TestSize.Level1)
{
    const std::string path = TEST_DEMUXER_RESOURCE_DIR + "/aac_implicit_sbr_leadin.mp4";
    auto getAacTrack = [](const MediaInfo& info) -> const Meta* {
        for (const auto& track : info.tracks) {
            Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
            std::string mime;
            if (track.Get<Tag::MEDIA_TYPE>(mediaType) && mediaType == Plugins::MediaType::AUDIO &&
                track.Get<Tag::MIME_TYPE>(mime) && (mime.find("mp4a") != std::string::npos ||
                mime.find("aac") != std::string::npos)) {
                return &track;
            }
        }
        return nullptr;
    };

    auto ffmpegPlugin = std::make_shared<FFmpegDemuxerPlugin>(FFMPEG_MP4_PLUGIN_NAME);
    auto mpeg4Plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(ffmpegPlugin, nullptr);
    ASSERT_NE(mpeg4Plugin, nullptr);
    auto ffmpeg = RunSetDataSourceOnly(ffmpegPlugin, path);
    auto mpeg4 = RunSetDataSourceOnly(mpeg4Plugin, path);
    ASSERT_EQ(ffmpeg.ret, Status::OK) << path;
    ASSERT_EQ(ffmpeg.mediaInfoRet, Status::OK) << path;
    ASSERT_EQ(mpeg4.ret, Status::OK) << path;
    ASSERT_EQ(mpeg4.mediaInfoRet, Status::OK) << path;
    const auto* ffmpegTrack = getAacTrack(ffmpeg.mediaInfo);
    const auto* mpeg4Track = getAacTrack(mpeg4.mediaInfo);
    ASSERT_NE(ffmpegTrack, nullptr) << path << " FFmpeg AAC track missing";
    ASSERT_NE(mpeg4Track, nullptr) << path << " MPEG4 AAC track missing";
    int32_t ffmpegRate = 0;
    int32_t mpeg4Rate = 0;
    int32_t ffmpegFrameSize = 0;
    int32_t mpeg4FrameSize = 0;
    ASSERT_TRUE(ffmpegTrack->Get<Tag::AUDIO_SAMPLE_RATE>(ffmpegRate));
    ASSERT_TRUE(mpeg4Track->Get<Tag::AUDIO_SAMPLE_RATE>(mpeg4Rate));
    ASSERT_TRUE(ffmpegTrack->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(ffmpegFrameSize));
    ASSERT_TRUE(mpeg4Track->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(mpeg4FrameSize));
    std::cout << "[AacImplicitSbrLeadInParity] file=" << path << " ffmpeg=" << ffmpegRate << "/" <<
        ffmpegFrameSize << " mpeg4=" << mpeg4Rate << "/" << mpeg4FrameSize << std::endl;
    EXPECT_EQ(ffmpegRate, mpeg4Rate) << path;
    EXPECT_EQ(ffmpegFrameSize, mpeg4FrameSize) << path;
    EXPECT_EQ(mpeg4Rate, 48000) << path;
    EXPECT_EQ(mpeg4FrameSize, 2048) << path;
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacImplicitSbrProbePreservesReadCursor_001
 * @tc.desc: Verify metadata probing does not advance the AAC sample cursor.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacImplicitSbrProbePreservesReadCursor_001,
    TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(plugin);
    ASSERT_NE(plugin, nullptr);
    InitPlugin(TEST_DEMUXER_RESOURCE_DIR + "/aac_implicit_sbr_editlist.mp4", mpeg4Plugin_, mpeg4Fd_,
        mpeg4MediaInfo_);
    auto track = plugin->firstTrack_;
    ASSERT_NE(track, nullptr);
    EXPECT_EQ(track->currentSampleIndex, 0U);

    ASSERT_EQ(plugin->SelectTrack(track->trackIndex), Status::OK);
    AVBufferWrapper buffer(4096);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    EXPECT_EQ(plugin->ReadSample(track->trackIndex, buffer.mediaAVBuffer), Status::OK);
    EXPECT_EQ(track->currentSampleIndex, 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_InvalidParameterAndTrackState_001
 * @tc.desc: Verify invalid parameters and select/unselect state handling
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InvalidParameterAndTrackState_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();
    auto invalidTrackId = static_cast<uint32_t>(mpeg4MediaInfo_.tracks.size());

    EXPECT_EQ(mpeg4Plugin_->SelectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->SelectTrack(invalidTrackId), Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->UnselectTrack(invalidTrackId), Status::ERROR_INVALID_PARAMETER);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    int32_t sampleSize = 0;
    EXPECT_EQ(mpeg4Plugin_->GetNextSampleSize(invalidTrackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(invalidTrackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, nullptr, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->ReadSampleZeroCopy(trackId, nullptr, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);

    ASSERT_EQ(mpeg4Plugin_->UnselectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_PARAMETER);
    EXPECT_EQ(mpeg4Plugin_->UnselectTrack(trackId), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->SelectTrack(trackId), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DynamicTrackSwitch_CompareWithFfmpeg_001
 * @tc.desc: Compare the first sample from a newly selected track after playback has advanced.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_DynamicTrackSwitch_CompareWithFfmpeg_001,
    TestSize.Level1)
{
    CheckDynamicTrackSwitch(DynamicSwitchScenario::PLAYBACK, ReadMode::COPY);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterSeek_CompareWithFfmpeg_001
 * @tc.desc: Compare a newly selected track after seeking before the first sample read.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterSeek_CompareWithFfmpeg_001,
    TestSize.Level1)
{
    ffmpegPlugin_ = std::make_shared<FFmpegDemuxerPlugin>(FFMPEG_MP4_PLUGIN_NAME);
    mpeg4Plugin_ = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(ffmpegPlugin_, nullptr);
    ASSERT_NE(mpeg4Plugin_, nullptr);
    InitPlugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS, ffmpegPlugin_, ffmpegFd_, ffmpegMediaInfo_);
    InitPlugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
    ASSERT_GE(ffmpegMediaInfo_.tracks.size(), DYNAMIC_SWITCH_REQUIRED_TRACK_COUNT);
    ASSERT_GE(mpeg4MediaInfo_.tracks.size(), DYNAMIC_SWITCH_REQUIRED_TRACK_COUNT);

    constexpr uint32_t seekTrackId = DYNAMIC_SWITCH_PRIMARY_TRACK_ID;
    constexpr uint32_t newlySelectedTrackId = DYNAMIC_SWITCH_SECONDARY_TRACK_ID;
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(seekTrackId), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(seekTrackId), Status::OK);
    int64_t ffmpegRealSeekTime = 0;
    int64_t mpeg4RealSeekTime = 0;
    constexpr int64_t seekTimeMs = DYNAMIC_SWITCH_SEEK_TIME_MS;
    ASSERT_EQ(ffmpegPlugin_->SeekTo(seekTrackId, seekTimeMs, SeekMode::SEEK_PREVIOUS_SYNC, ffmpegRealSeekTime),
        Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SeekTo(seekTrackId, seekTimeMs, SeekMode::SEEK_PREVIOUS_SYNC, mpeg4RealSeekTime),
        Status::OK);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(newlySelectedTrackId), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(newlySelectedTrackId), Status::OK);
    for (uint32_t index = 0; index < DYNAMIC_SWITCH_COMPARE_SAMPLE_COUNT; ++index) {
        ReadAndCompareSwitchSample(newlySelectedTrackId, ReadMode::COPY);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterPlaybackSeek_CompareWithFfmpeg_001
 * @tc.desc: Compare track switching after playback and a seek.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterPlaybackSeek_CompareWithFfmpeg_001, TestSize.Level1)
{
    CheckDynamicTrackSwitch(DynamicSwitchScenario::PLAYBACK_SEEK, ReadMode::COPY);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterFlush_CompareWithFfmpeg_001
 * @tc.desc: Compare track switching after Flush invalidates cached samples.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterFlush_CompareWithFfmpeg_001, TestSize.Level1)
{
    CheckDynamicTrackSwitch(DynamicSwitchScenario::FLUSH, ReadMode::COPY);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterGetNextSampleSize_CompareWithFfmpeg_001
 * @tc.desc: Enter runtime through GetNextSampleSize before switching tracks.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_DynamicTrackSwitchAfterGetNextSampleSize_CompareWithFfmpeg_001, TestSize.Level1)
{
    CheckDynamicTrackSwitch(DynamicSwitchScenario::GET_NEXT_SIZE, ReadMode::COPY);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DynamicTrackSwitch_ZeroCopy_CompareWithFfmpeg_001
 * @tc.desc: Compare switched-track samples using zero-copy reads.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_DynamicTrackSwitch_ZeroCopy_CompareWithFfmpeg_001, TestSize.Level1)
{
    CheckDynamicTrackSwitch(DynamicSwitchScenario::PLAYBACK, ReadMode::ZERO_COPY);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ResetAndDataSourceSwitch_CompareWithFfmpeg_001
 * @tc.desc: Verify Reset clears old-source state before binding a different MP4 resource.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ResetAndDataSourceSwitch_CompareWithFfmpeg_001, TestSize.Level1)
{
    CheckResetAndDataSourceSwitch();
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DynamicTrackSwitchInFlightAba_001
 * @tc.desc: Reject an in-flight sample after Unselect and Select create a new track epoch.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_DynamicTrackSwitchInFlightAba_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckDynamicTrackSwitchInFlightAba());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SyncReadInterfaces_001
 * @tc.desc: Verify synchronous GetNextSampleSize and ReadSample interfaces
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SyncReadInterfaces_001, TestSize.Level1)
{
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    auto trackId = selectedTrackIds.front();

    int32_t sampleSize = 0;
    ASSERT_EQ(mpeg4Plugin_->GetNextSampleSize(trackId, sampleSize), Status::OK);
    EXPECT_GT(sampleSize, 0);

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    ASSERT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer), Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
    EXPECT_GT(buffer.mediaAVBuffer->memory_->GetSize(), 0U);

    EXPECT_EQ(mpeg4Plugin_->GetNextSampleSize(trackId, sampleSize, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT),
        Status::ERROR_INVALID_OPERATION);
    EXPECT_EQ(mpeg4Plugin_->ReadSampleZeroCopy(trackId, MediaAVBuffer::CreateAVBuffer(),
        MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_TIMEOUT), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SyncReadEosKeepsCursor_001
 * @tc.desc: Keep the committed read cursor unchanged for repeated sync EOS reads.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SyncReadEosKeepsCursor_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckSyncReadEosKeepsCursor());
}

void Mpeg4DemuxerPluginUnitTest::CheckSyncReadEosKeepsCursor()
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(plugin, nullptr);
    int fd = -1;
    MediaInfo mediaInfo;
    InitPlugin(TEST_MP4_PATH_H264_B, plugin, fd, mediaInfo);
    auto selectedTracks = SelectSupportedTracks(plugin, mediaInfo);
    const uint32_t trackId = FindFirstVideoTrack(mediaInfo, selectedTracks);
    ASSERT_NE(trackId, INVALID_TRACK_ID);
    for (uint32_t id : selectedTracks) {
        if (id != trackId) {
            ASSERT_EQ(plugin->UnselectTrack(id), Status::OK);
        }
    }
    auto track = plugin->FindTrackById(static_cast<int32_t>(trackId));
    ASSERT_NE(track, nullptr);
    const size_t sampleCount = track->sampleHelper->GetSampleCount();
    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_TRUE(ReadUntilSyncEosAndVerifyCursor(plugin, trackId, track, sampleCount, buffer.mediaAVBuffer));
    EXPECT_EQ(plugin->ReadSample(trackId, buffer.mediaAVBuffer), Status::OK);
    EXPECT_TRUE(IsEos(buffer.mediaAVBuffer));

    int64_t realSeekTime = 0;
    ASSERT_EQ(plugin->SeekTo(static_cast<int32_t>(trackId), TEST_EOS_SEEK_TIME_MS,
        SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime), Status::OK);
    EXPECT_TRUE(ReadUntilSyncEosAndVerifyCursor(plugin, trackId, track, sampleCount, buffer.mediaAVBuffer));
}

bool Mpeg4DemuxerPluginUnitTest::ReadUntilSyncEosAndVerifyCursor(
    const std::shared_ptr<MPEG4DemuxerPlugin>& plugin, uint32_t trackId,
    const std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track>& track, size_t sampleCount,
    const std::shared_ptr<MediaAVBuffer>& buffer)
{
    auto getCursor = [plugin] {
        std::lock_guard<std::mutex> lock(plugin->readConfigMutex_);
        return std::make_tuple(plugin->readCursor_.valid, plugin->readCursor_.dtsUs,
            plugin->readCursor_.pos, plugin->readCursor_.ptsUs, plugin->readCursor_.usePresentationTime);
    };
    bool eosReached = false;
    for (size_t index = 0; index <= sampleCount; ++index) {
        auto cursorBefore = getCursor();
        const uint32_t sampleIndexBefore = track->currentSampleIndex;
        EXPECT_EQ(plugin->ReadSample(trackId, buffer), Status::OK);
        if (!IsEos(buffer)) {
            continue;
        }
        eosReached = true;
        EXPECT_EQ(getCursor(), cursorBefore);
        EXPECT_EQ(track->currentSampleIndex, sampleIndexBefore);
        break;
    }
    return eosReached;
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacImplicitSbr_001
 * @tc.desc: Verify an in-band EXT_SBR_DATA extension updates AAC output metadata.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacImplicitSbr_001, TestSize.Level1)
{
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    auto data = BuildAacRawDataBlock(AAC_TEST_EXTENSION_SBR);
    Plugins::MPEG4::MPEG4AudioParser parser;

    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t sampleRate = 0;
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(sampleRate, AAC_TEST_CORE_SAMPLE_RATE * AAC_SBR_SAMPLE_RATE_MULTIPLIER);
    EXPECT_EQ(samplesPerFrame, AAC_TEST_CORE_SAMPLES_PER_FRAME * AAC_SBR_SAMPLE_RATE_MULTIPLIER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacImplicitSbrCrc_001
 * @tc.desc: Verify an in-band EXT_SBR_DATA_CRC extension updates AAC output metadata.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacImplicitSbrCrc_001, TestSize.Level1)
{
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    auto data = BuildAacRawDataBlock(AAC_TEST_EXTENSION_SBR_CRC);
    Plugins::MPEG4::MPEG4AudioParser parser;

    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t sampleRate = 0;
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(sampleRate, AAC_TEST_CORE_SAMPLE_RATE * AAC_SBR_SAMPLE_RATE_MULTIPLIER);
    EXPECT_EQ(samplesPerFrame, AAC_TEST_CORE_SAMPLES_PER_FRAME * AAC_SBR_SAMPLE_RATE_MULTIPLIER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacImplicitSbrFallback_001
 * @tc.desc: Verify ordinary fill data and truncated SBR data preserve ASC metadata.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacImplicitSbrFallback_001, TestSize.Level1)
{
    std::vector<std::vector<uint8_t>> testData = {BuildAacRawDataBlock(AAC_TEST_EXTENSION_OTHER),
        BuildAacRawDataBlock(AAC_TEST_EXTENSION_SBR, true)};
    for (auto& data : testData) {
        MediaInfo mediaInfo = MakeAacFrameMediaInfo();
        Plugins::MPEG4::MPEG4AudioParser parser;
        EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
        int32_t sampleRate = 0;
        int32_t samplesPerFrame = 0;
        ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
        ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
        EXPECT_EQ(sampleRate, AAC_TEST_CORE_SAMPLE_RATE);
        EXPECT_EQ(samplesPerFrame, AAC_TEST_CORE_SAMPLES_PER_FRAME);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacPsConfig_001
 * @tc.desc: Verify a PS ASC reports FFmpeg-compatible stereo output metadata.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacPsConfig_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    ASSERT_NE(track, nullptr);
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    ASSERT_NE(track->sampleHelper, nullptr);
    parser.lastTrack_ = track;
    parser.mediaInfo_.tracks.resize(1);

    uint32_t sampleRate = 0;
    uint32_t channels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(parser.ParseAACConfig(AAC_TEST_PS_CONFIG, sizeof(AAC_TEST_PS_CONFIG), sampleRate, channels, layout));
    EXPECT_EQ(sampleRate, AAC_TEST_CORE_SAMPLE_RATE);
    EXPECT_EQ(channels, static_cast<uint32_t>(AAC_TEST_CHANNEL_COUNT));
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacSyncExtensionSbr_001
 * @tc.desc: Verify an AAC_LC ASC with an 0x2B7 sync extension reports SBR sample rate and
 *           doubled samples per frame, matching ffmpeg ff_mpeg4audio_get_config_gb.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacSyncExtensionSbr_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    ASSERT_NE(track, nullptr);
    track->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    ASSERT_NE(track->sampleHelper, nullptr);
    parser.lastTrack_ = track;
    parser.mediaInfo_.tracks.resize(1);

    uint32_t sampleRate = 0;
    uint32_t channels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(parser.ParseAACConfig(AAC_TEST_SYNC_EXTENSION_SBR_CONFIG,
        sizeof(AAC_TEST_SYNC_EXTENSION_SBR_CONFIG), sampleRate, channels, layout));
    EXPECT_EQ(sampleRate, AAC_TEST_96KHZ_SAMPLE_RATE);
    EXPECT_EQ(channels, static_cast<uint32_t>(AAC_TEST_CHANNEL_COUNT));
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(samplesPerFrame, static_cast<int32_t>(AAC_TEST_96KHZ_SBR_SAMPLES_PER_FRAME));

    ASSERT_TRUE(parser.ParseAACSpecificConfig(AAC_TEST_SYNC_EXTENSION_SBR_CONFIG,
        sizeof(AAC_TEST_SYNC_EXTENSION_SBR_CONFIG)));
    int32_t parsedSampleRate = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(parsedSampleRate));
    EXPECT_EQ(parsedSampleRate, static_cast<int32_t>(AAC_TEST_96KHZ_SAMPLE_RATE));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacPceConfig_001
 * @tc.desc: Verify channelConfiguration zero uses a valid PCE channel count.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacPceConfig_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    ASSERT_NE(PrepareAacConfigTestParser(parser), nullptr);
    auto config = BuildAacPceSpecificConfig(false);
    const std::vector<uint8_t> expected(AAC_TEST_PCE_CONFIG, AAC_TEST_PCE_CONFIG + sizeof(AAC_TEST_PCE_CONFIG));
    ASSERT_EQ(config, expected);

    ASSERT_TRUE(parser.ParseAACSpecificConfig(config.data(), config.size()));
    int32_t channels = 0;
    int32_t outputChannels = 0;
    int32_t sampleRate = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_PCE_CHANNEL_COUNT);
    EXPECT_EQ(outputChannels, AAC_TEST_PCE_CHANNEL_COUNT);
    EXPECT_EQ(sampleRate, AAC_TEST_PCE_SAMPLE_RATE);
    EXPECT_EQ(layout, AudioChannelLayout::CH_5POINT1);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacTruncatedPceFallback_001
 * @tc.desc: Verify a truncated PCE preserves valid sample-entry channel metadata.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacTruncatedPceFallback_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    ASSERT_NE(PrepareAacConfigTestParser(parser), nullptr);
    parser.mediaInfo_.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(AAC_TEST_PCE_CHANNEL_COUNT);
    parser.mediaInfo_.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(AAC_TEST_PCE_CHANNEL_COUNT);
    parser.mediaInfo_.tracks[0].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::CH_5POINT1);
    parser.mediaInfo_.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::CH_5POINT1);
    auto config = BuildAacPceSpecificConfig(true);

    ASSERT_TRUE(parser.ParseAACSpecificConfig(config.data(), config.size()));
    int32_t channels = 0;
    int32_t outputChannels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_PCE_CHANNEL_COUNT);
    EXPECT_EQ(outputChannels, AAC_TEST_PCE_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::CH_5POINT1);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacExplicitChannelConfigOverride_001
 * @tc.desc: Verify an expressible ASC channel configuration overrides sample-entry metadata.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacExplicitChannelConfigOverride_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    ASSERT_NE(PrepareAacConfigTestParser(parser), nullptr);
    parser.mediaInfo_.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(AAC_TEST_PCE_CHANNEL_COUNT);
    parser.mediaInfo_.tracks[0].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::CH_5POINT1);

    ASSERT_TRUE(parser.ParseAACSpecificConfig(AAC_TEST_EXPLICIT_STEREO_CONFIG,
        sizeof(AAC_TEST_EXPLICIT_STEREO_CONFIG)));
    int32_t channels = 0;
    int32_t outputChannels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_CHANNEL_COUNT);
    EXPECT_EQ(outputChannels, AAC_TEST_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacPceOddBackLayout_001
 * @tc.desc: Verify an odd BACK channel count resolves through the position center
 *           slot (BL/BR/BC), matching FFmpeg assign_channels.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacPceOddBackLayout_001, TestSize.Level1)
{
    // FRONT: SCE,CPE -> 3 (FC,FL,FR); BACK: CPE,SCE -> 3 (BL,BR,BC). The combined
    // mask FC|FL|FR|BL|BR|BC equals AudioChannelLayout::HEXAGONAL.
    auto config = BuildAacPceConfigWithElements(2, 0, 2, {AAC_TEST_PCE_SINGLE_CHANNEL_ELEMENT,
        AAC_TEST_PCE_CHANNEL_PAIR_ELEMENT}, {}, {AAC_TEST_PCE_CHANNEL_PAIR_ELEMENT,
        AAC_TEST_PCE_SINGLE_CHANNEL_ELEMENT});
    Plugins::MPEG4::MPEG4AtomParser parser;
    ASSERT_NE(PrepareAacConfigTestParser(parser), nullptr);
    ASSERT_TRUE(parser.ParseAACSpecificConfig(config.data(), config.size()));
    int32_t channels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_PCE_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::HEXAGONAL);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacEldFrameLength_001
 * @tc.desc: Verify ER-AAC-ELD frame length follows frameLengthFlag (480/512),
 *           matching FFmpeg aac_decode_er_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacEldFrameLength_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    ASSERT_NE(PrepareAacConfigTestParser(parser), nullptr);
    ASSERT_TRUE(parser.ParseAACSpecificConfig(AAC_TEST_ELD_SHORT_FRAME_CONFIG,
        sizeof(AAC_TEST_ELD_SHORT_FRAME_CONFIG)));
    int32_t samplesPerFrame = 0;
    int32_t sampleRate = 0;
    int32_t channels = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_TRUE(parser.mediaInfo_.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    EXPECT_EQ(samplesPerFrame, AAC_TEST_ELD_SHORT_SAMPLES_PER_FRAME);
    EXPECT_EQ(sampleRate, AAC_TEST_PCE_SAMPLE_RATE);
    EXPECT_EQ(channels, AAC_TEST_CHANNEL_COUNT);

    Plugins::MPEG4::MPEG4AtomParser longParser;
    ASSERT_NE(PrepareAacConfigTestParser(longParser), nullptr);
    ASSERT_TRUE(longParser.ParseAACSpecificConfig(AAC_TEST_ELD_LONG_FRAME_CONFIG,
        sizeof(AAC_TEST_ELD_LONG_FRAME_CONFIG)));
    ASSERT_TRUE(longParser.mediaInfo_.tracks[0].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(samplesPerFrame, AAC_TEST_ELD_LONG_SAMPLES_PER_FRAME);
}

namespace {
// FFmpeg avformat_find_stream_info default probesize (options_table.h); the
// demuxer keeps it for MP4, so AAC tracks beyond it are never probe-decoded.
constexpr int64_t AAC_PROBE_TEST_FFMPEG_PROBE_SIZE = 5000000;
constexpr uint32_t AAC_PROBE_TEST_VIDEO_TIMESCALE = 90000;
constexpr uint32_t AAC_PROBE_TEST_AUDIO_TIMESCALE = 48000;
constexpr uint32_t AAC_PROBE_TEST_AUDIO_DELTA = 1024;
constexpr uint32_t AAC_PROBE_TEST_VIDEO_DELTA = 1;
constexpr int32_t AAC_PROBE_TEST_AUDIO_SAMPLE_SIZE = 200;
constexpr int32_t AAC_PROBE_TEST_SMALL_VIDEO_SAMPLE_SIZE = 100000;
constexpr int32_t AAC_PROBE_TEST_LARGE_VIDEO_SAMPLE_SIZE = 2000000;
constexpr int32_t AAC_PROBE_TEST_SAMPLES_PER_FRAME = 1024;
constexpr uint32_t AAC_PROBE_TEST_TRACK_COUNT = 2;
// bbcnews-original.mp4 video track: stts (159,0),(1,29),(2442,40) at a 1 kHz time base.
// The FFmpeg probe reaches max_stream_analyze_duration (5 s) through the long zero-delta
// run before the audio track is ever read, so the AAC frame size is not exported.
constexpr uint32_t AAC_PROBE_TEST_R5_VIDEO_TIMESCALE = 1000;
constexpr uint32_t AAC_PROBE_TEST_R5_VIDEO_SAMPLE_COUNT = 2602;
constexpr int32_t AAC_PROBE_TEST_R5_VIDEO_SAMPLE_SIZE = 1000;
constexpr int64_t AAC_PROBE_TEST_R5_AUDIO_OFFSET = 3000000;
// ASC bytes with AOT=AAC-LC and samplingFrequencyIndex=4 (0x12 0x10), used by the
// lightweight probe frame-structure check to pick the NUM_SWB table.
constexpr uint8_t AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4[] = {0x12, 0x10};
constexpr size_t AAC_PROBE_TEST_ASC_SIZE = 2;
constexpr uint8_t AAC_PROBE_TEST_COMMON_WINDOW_BITS = 1;
constexpr uint8_t AAC_PROBE_TEST_ICS_RESERVED_BITS = 1;
constexpr uint8_t AAC_PROBE_TEST_WINDOW_SEQUENCE_BITS = 2;
constexpr uint8_t AAC_PROBE_TEST_WINDOW_SHAPE_BITS = 1;
constexpr uint8_t AAC_PROBE_TEST_MAX_SFB_BITS = 6;
constexpr uint8_t AAC_PROBE_TEST_PREDICTOR_PRESENT_BITS = 1;
constexpr uint8_t AAC_PROBE_TEST_MS_MASK_PRESENT_BITS = 2;
constexpr uint8_t AAC_PROBE_TEST_GLOBAL_GAIN_BITS = 8;
constexpr uint8_t AAC_PROBE_TEST_SECTION_TYPE_BITS = 4;
constexpr uint8_t AAC_PROBE_TEST_BAND_INCR_BITS = 5;
constexpr uint32_t AAC_PROBE_TEST_MAX_SFB = 1;
constexpr uint32_t AAC_PROBE_TEST_INVALID_MAX_SFB = 50;
constexpr uint32_t AAC_PROBE_TEST_SECTION_TYPE = 1;
constexpr uint32_t AAC_PROBE_TEST_BAND_INCR = 1;
// EIGHT_SHORT_SEQUENCE = 2 in FFmpeg's enum WindowSequence (aac.h).
constexpr uint8_t AAC_PROBE_TEST_SHORT_WINDOW_SEQUENCE = 2;
constexpr uint8_t AAC_PROBE_TEST_MAX_SFB_SHORT_BITS = 4;
constexpr uint32_t AAC_PROBE_TEST_SHORT_MAX_SFB = 10;
constexpr uint8_t AAC_PROBE_TEST_GROUPING_BITS = 7;
constexpr uint32_t AAC_PROBE_TEST_WINDOW_GROUP_COUNT = 2;
constexpr uint8_t AAC_PROBE_TEST_MS_MASK_BITS = AAC_PROBE_TEST_WINDOW_GROUP_COUNT *
    AAC_PROBE_TEST_SHORT_MAX_SFB;
constexpr uint8_t AAC_PROBE_TEST_BAND_INCR_SHORT_BITS = 3;
constexpr uint32_t AAC_PROBE_TEST_MAX_SHORT_INCR = 7;
constexpr uint32_t AAC_PROBE_TEST_FINAL_SHORT_INCR = 3;
// scale_factor_grouping 0111111: the first (MSB) zero opens the second window group and
// the six set bits extend it, so FFmpeg decode_ics_info counts two window groups.
constexpr uint32_t AAC_PROBE_TEST_GROUP_EXTEND_MASK = 0x3F;
// DSE/PCE/CCE element payloads mirroring the parser-side probe checks (ISO/IEC 14496-3
// table 4.x): the DSE count escape 255 -> +8 bits, and band type 12 that FFmpeg
// decode_band_types rejects ("invalid band type").
constexpr uint32_t AAC_PROBE_TEST_DSE_PAYLOAD_COUNT = 1;
constexpr uint32_t AAC_PROBE_TEST_DSE_COUNT_ESCAPE = 255;
constexpr uint32_t AAC_PROBE_TEST_DSE_ESCAPE_ADD_COUNT = 2;
constexpr uint32_t AAC_PROBE_TEST_DSE_TRUNCATED_COUNT = 8;
constexpr uint32_t AAC_PROBE_TEST_DSE_TRUNCATED_PAYLOAD_BYTES = 4;
constexpr uint32_t AAC_PROBE_TEST_PCE_FRONT_COUNT = 1;
constexpr uint32_t AAC_PROBE_TEST_PCE_BACK_COUNT = 1;
constexpr uint32_t AAC_PROBE_TEST_PREDICTOR_PRESENT = 1;
constexpr uint32_t AAC_PROBE_TEST_INVALID_SECTION_TYPE = 12;
// Parser-side AAC_MAX_FRAME_VALIDATE_SIZE (16 KiB): a raw-data block beyond this cap is
// rejected before any read so a corrupt stsz cannot force a multi-MB probe allocation.
constexpr int32_t AAC_PROBE_TEST_OVERSIZE_SAMPLE_SIZE = 16 * 1024 + 1;

std::shared_ptr<Plugins::MPEG4::MPEG4SampleHelper> BuildProbeTestSampleHelper(
    Plugins::MPEG4::TrackType trackType, const std::vector<int32_t>& sampleSizes,
    uint32_t sttsDelta, int64_t baseOffset)
{
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (helper == nullptr) {
        return nullptr;
    }
    const uint32_t sampleCount = static_cast<uint32_t>(sampleSizes.size());
    helper->trackType_ = trackType;
    helper->sttsEntry_ = {{sampleCount, sttsDelta}};
    helper->stscEntry_ = {{0, sampleCount, 1}};
    helper->sampleSizes_ = sampleSizes;
    helper->sampleSizeCount_ = sampleCount;
    helper->chunkOffsets_ = {baseOffset};
    if (helper->FinalizeSampleTables() != Status::OK) {
        return nullptr;
    }
    return helper;
}

std::shared_ptr<Plugins::MPEG4::MPEG4SampleHelper> BuildProbeTestSampleHelperWithStts(
    Plugins::MPEG4::TrackType trackType, const std::vector<int32_t>& sampleSizes,
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry>& sttsEntries,
    int64_t baseOffset)
{
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (helper == nullptr) {
        return nullptr;
    }
    const uint32_t sampleCount = static_cast<uint32_t>(sampleSizes.size());
    helper->trackType_ = trackType;
    helper->sttsEntry_ = sttsEntries;
    helper->stscEntry_ = {{0, sampleCount, 1}};
    helper->sampleSizes_ = sampleSizes;
    helper->sampleSizeCount_ = sampleCount;
    helper->chunkOffsets_ = {baseOffset};
    if (helper->FinalizeSampleTables() != Status::OK) {
        return nullptr;
    }
    return helper;
}

std::vector<uint8_t> PackAacTestBits(const std::vector<uint8_t>& bits)
{
    std::vector<uint8_t> data((bits.size() + AAC_TEST_BITS_PER_BYTE - 1) / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        data[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    return data;
}

// Shared CPE core used by the probe frame builders: common_window=1 long window with
// max_sfb/predictor/section type passed by the caller, ms_mask absent, one band section.
void AppendAacProbeCpeBits(std::vector<uint8_t>& bits, uint32_t maxSfb, uint32_t predictorPresent,
    uint32_t sectionType)
{
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_CPE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, 1, AAC_PROBE_TEST_COMMON_WINDOW_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_ICS_RESERVED_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_WINDOW_SEQUENCE_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_WINDOW_SHAPE_BITS);
    AppendAacTestBits(bits, maxSfb, AAC_PROBE_TEST_MAX_SFB_BITS);
    AppendAacTestBits(bits, predictorPresent, AAC_PROBE_TEST_PREDICTOR_PRESENT_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_MS_MASK_PRESENT_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_GLOBAL_GAIN_BITS);
    AppendAacTestBits(bits, sectionType, AAC_PROBE_TEST_SECTION_TYPE_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_BAND_INCR, AAC_PROBE_TEST_BAND_INCR_BITS);
}

std::vector<uint8_t> BuildAacProbeCpeFrame(uint32_t maxSfb, uint32_t predictorPresent, uint32_t sectionType)
{
    std::vector<uint8_t> bits;
    AppendAacProbeCpeBits(bits, maxSfb, predictorPresent, sectionType);
    return PackAacTestBits(bits);
}

// A single CPE with a shared ICS window: element type/tag, common_window=1, reserved=0,
// long window, max_sfb (1 valid / 50 invalid for samplingFrequencyIndex=4), predictor
// absent, ms_mask_present=0, global_gain, then one band-type section.
std::vector<uint8_t> BuildAacProbeCpeRawDataBlock(bool validMaxSfb)
{
    return BuildAacProbeCpeFrame(validMaxSfb ? AAC_PROBE_TEST_MAX_SFB : AAC_PROBE_TEST_INVALID_MAX_SFB,
        0, AAC_PROBE_TEST_SECTION_TYPE);
}

// A DSE raw-data-block element followed by a valid CPE: element type/tag,
// data_byte_align_flag=0, count (255 -> +8 escape when requested), count payload bytes,
// then the valid CPE. Exercises SkipAacProbeDataStreamElement in ValidateAacProbeFrame.
std::vector<uint8_t> BuildAacProbeDsePrefixedFrame(bool escapeCount)
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_DSE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_DSE_ALIGN_FLAG_BITS);
    uint32_t payloadBytes = 0;
    if (escapeCount) {
        AppendAacTestBits(bits, AAC_PROBE_TEST_DSE_COUNT_ESCAPE, AAC_TEST_DSE_COUNT_BITS);
        AppendAacTestBits(bits, AAC_PROBE_TEST_DSE_ESCAPE_ADD_COUNT, AAC_TEST_DSE_COUNT_BITS);
        payloadBytes = AAC_PROBE_TEST_DSE_COUNT_ESCAPE + AAC_PROBE_TEST_DSE_ESCAPE_ADD_COUNT;
    } else {
        AppendAacTestBits(bits, AAC_PROBE_TEST_DSE_PAYLOAD_COUNT, AAC_TEST_DSE_COUNT_BITS);
        payloadBytes = AAC_PROBE_TEST_DSE_PAYLOAD_COUNT;
    }
    for (uint32_t i = 0; i < payloadBytes; ++i) {
        AppendAacTestBits(bits, 0, AAC_TEST_BITS_PER_BYTE);
    }
    AppendAacProbeCpeBits(bits, AAC_PROBE_TEST_MAX_SFB, 0, AAC_PROBE_TEST_SECTION_TYPE);
    return PackAacTestBits(bits);
}

// A DSE element declaring more payload bytes than the packet provides, so the probe
// BitsLeft check fails and the first sample is rejected.
std::vector<uint8_t> BuildAacProbeTruncatedDseFrame()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_DSE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_DSE_ALIGN_FLAG_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_DSE_TRUNCATED_COUNT, AAC_TEST_DSE_COUNT_BITS);
    for (uint32_t i = 0; i < AAC_PROBE_TEST_DSE_TRUNCATED_PAYLOAD_BYTES; ++i) {
        AppendAacTestBits(bits, 0, AAC_TEST_BITS_PER_BYTE);
    }
    return PackAacTestBits(bits);
}

// An unaligned FIL element followed by a DSE with data_byte_align_flag=1: the DSE
// count ends off the byte boundary and AlignToByte() consumes the padding, then the
// count payload and a valid CPE follow. Exercises both SkipAacProbeFillElement and
// the DSE byte-alignment branch in ValidateAacProbeFrame.
std::vector<uint8_t> BuildAacProbeDseAlignedFrame()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_FIL, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_FILL_COUNT, AAC_TEST_FILL_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_EXTENSION_OTHER, AAC_TEST_EXTENSION_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_PAYLOAD_BITS - AAC_TEST_EXTENSION_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_DSE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, 1, AAC_TEST_DSE_ALIGN_FLAG_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_DSE_PAYLOAD_COUNT, AAC_TEST_DSE_COUNT_BITS);
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    AppendAacTestBits(bits, 0, AAC_TEST_BITS_PER_BYTE);
    AppendAacProbeCpeBits(bits, AAC_PROBE_TEST_MAX_SFB, 0, AAC_PROBE_TEST_SECTION_TYPE);
    return PackAacTestBits(bits);
}

// A PCE raw-data-block element (object type/sampling index, one front CPE, one back SCE,
// a mono mixdown tag, empty comment) followed by a valid CPE. Exercises
// SkipAacProbeProgramConfigElement including the relative byte alignment.
std::vector<uint8_t> BuildAacProbePcePrefixedFrame()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_PCE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_AAC_LC_OBJECT_TYPE, AAC_TEST_PCE_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_48KHZ_SAMPLE_RATE_INDEX, AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_PCE_FRONT_COUNT, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_PCE_BACK_COUNT, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_LFE_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ASSOC_DATA_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, 1, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacPceChannelElement(bits, AAC_TEST_PCE_CHANNEL_PAIR_ELEMENT, AAC_TEST_PCE_ZERO_VALUE);
    AppendAacPceChannelElement(bits, AAC_TEST_PCE_SINGLE_CHANNEL_ELEMENT, AAC_TEST_PCE_SECONDARY_TAG);
    // FFmpeg decode_pce aligns the comment field to a byte boundary relative to the
    // raw-data-block start (payload_alignment), so the builder pads before comment_len.
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_COMMENT_COUNT_BITS);
    AppendAacProbeCpeBits(bits, AAC_PROBE_TEST_MAX_SFB, 0, AAC_PROBE_TEST_SECTION_TYPE);
    return PackAacTestBits(bits);
}

// A PCE element with zero channel counts and no comment field: the relative byte
// alignment succeeds but the comment read runs out of bits and rejects the frame.
std::vector<uint8_t> BuildAacProbeTruncatedPceFrame()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_PCE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_AAC_LC_OBJECT_TYPE, AAC_TEST_PCE_OBJECT_TYPE_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_48KHZ_SAMPLE_RATE_INDEX, AAC_TEST_SAMPLING_FREQUENCY_INDEX_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_LFE_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ASSOC_DATA_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_ELEMENT_COUNT_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    AppendAacTestBits(bits, AAC_TEST_PCE_ZERO_VALUE, AAC_TEST_PCE_FLAG_BITS);
    return PackAacTestBits(bits);
}

// A CCE raw-data-block element: coupling_point=0, num_coupled=1 (one CPE couple with
// ch_select, one SCE couple), then a valid first-channel SCE (global_gain/ics/band types).
std::vector<uint8_t> BuildAacProbeCceFrame()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_CCE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_COUPLING_POINT_BITS);
    AppendAacTestBits(bits, 1, AAC_TEST_CCE_NUM_COUPLED_BITS);
    AppendAacTestBits(bits, 1, AAC_TEST_CCE_IS_CPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_ID_SELECT_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_CH_SELECT_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_IS_CPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_ID_SELECT_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_COUPLING_POINT_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_SIGN_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_SCALE_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_GLOBAL_GAIN_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_ICS_RESERVED_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_WINDOW_SEQUENCE_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_WINDOW_SHAPE_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_MAX_SFB, AAC_PROBE_TEST_MAX_SFB_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_PREDICTOR_PRESENT_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_SECTION_TYPE, AAC_PROBE_TEST_SECTION_TYPE_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_BAND_INCR, AAC_PROBE_TEST_BAND_INCR_BITS);
    return PackAacTestBits(bits);
}

// A CCE element with num_coupled=1 but only one per-couple entry: the second iteration
// runs out of bits and the probe rejects the frame.
std::vector<uint8_t> BuildAacProbeTruncatedCceFrame()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_CCE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_COUPLING_POINT_BITS);
    AppendAacTestBits(bits, 1, AAC_TEST_CCE_NUM_COUPLED_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_IS_CPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_CCE_ID_SELECT_BITS);
    return PackAacTestBits(bits);
}

// A CPE with a shared EIGHT_SHORT ICS window and two window groups (scale_factor_grouping
// 0,1,1,1,1,1,1): the ms_mask is num_window_groups * max_sfb bits and the band-type
// sections restart per group with a zero/escaping increment, matching the real frames of
// the 44.1 kHz meta* resources that the previous single-run check wrongly rejected.
std::vector<uint8_t> BuildAacProbeShortCpeRawDataBlock()
{
    std::vector<uint8_t> bits;
    AppendAacTestBits(bits, AAC_TEST_ELEMENT_CPE, AAC_TEST_ELEMENT_TYPE_BITS);
    AppendAacTestBits(bits, 0, AAC_TEST_INSTANCE_TAG_BITS);
    AppendAacTestBits(bits, 1, AAC_PROBE_TEST_COMMON_WINDOW_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_ICS_RESERVED_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_SHORT_WINDOW_SEQUENCE, AAC_PROBE_TEST_WINDOW_SEQUENCE_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_WINDOW_SHAPE_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_SHORT_MAX_SFB, AAC_PROBE_TEST_MAX_SFB_SHORT_BITS);
    AppendAacTestBits(bits, AAC_PROBE_TEST_GROUP_EXTEND_MASK, AAC_PROBE_TEST_GROUPING_BITS);
    AppendAacTestBits(bits, 1, AAC_PROBE_TEST_MS_MASK_PRESENT_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_MS_MASK_BITS);
    AppendAacTestBits(bits, 0, AAC_PROBE_TEST_GLOBAL_GAIN_BITS);
    for (uint32_t group = 0; group < AAC_PROBE_TEST_WINDOW_GROUP_COUNT; ++group) {
        AppendAacTestBits(bits, AAC_PROBE_TEST_SECTION_TYPE, AAC_PROBE_TEST_SECTION_TYPE_BITS);
        AppendAacTestBits(bits, AAC_PROBE_TEST_MAX_SHORT_INCR, AAC_PROBE_TEST_BAND_INCR_SHORT_BITS);
        AppendAacTestBits(bits, AAC_PROBE_TEST_FINAL_SHORT_INCR, AAC_PROBE_TEST_BAND_INCR_SHORT_BITS);
    }
    while (bits.size() % AAC_TEST_BITS_PER_BYTE != 0) {
        bits.push_back(0);
    }
    std::vector<uint8_t> data(bits.size() / AAC_TEST_BITS_PER_BYTE, 0);
    for (size_t index = 0; index < bits.size(); ++index) {
        data[index / AAC_TEST_BITS_PER_BYTE] |= static_cast<uint8_t>(
            bits[index] << (AAC_TEST_BITS_PER_BYTE - 1 - index % AAC_TEST_BITS_PER_BYTE));
    }
    return data;
}

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareProbeTestTracks(
    Plugins::MPEG4::MPEG4AtomParser& parser,
    const std::shared_ptr<Plugins::MPEG4::MPEG4SampleHelper>& videoHelper,
    const std::shared_ptr<Plugins::MPEG4::MPEG4SampleHelper>& audioHelper)
{
    auto video = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto audio = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    if (video == nullptr || audio == nullptr) {
        return nullptr;
    }
    video->trackIndex = 0;
    video->timeScale = AAC_PROBE_TEST_VIDEO_TIMESCALE;
    video->sampleHelper = videoHelper;
    audio->trackIndex = 1;
    audio->timeScale = AAC_PROBE_TEST_AUDIO_TIMESCALE;
    audio->sampleHelper = audioHelper;
    video->next = audio;
    parser.firstTrack_ = video;
    parser.mediaInfo_.tracks.resize(AAC_PROBE_TEST_TRACK_COUNT);
    parser.mediaInfo_.tracks[0].Set<Tag::MIME_TYPE>(MimeType::VIDEO_AVC);
    parser.mediaInfo_.tracks[1].Set<Tag::MIME_TYPE>(MimeType::AUDIO_AAC);
    parser.mediaInfo_.tracks[1].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(AAC_PROBE_TEST_SAMPLES_PER_FRAME);
    return audio;
}

bool RunAacProbeFrameValidation(const std::vector<uint8_t>& payload, int32_t sampleSize)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {sampleSize}, AAC_PROBE_TEST_AUDIO_DELTA, 0);
    if (audioHelper == nullptr) {
        return false;
    }
    auto audio = PrepareProbeTestTracks(parser, nullptr, audioHelper);
    if (audio == nullptr) {
        return false;
    }
    audio->codecParms.data = std::make_unique<uint8_t[]>(AAC_PROBE_TEST_ASC_SIZE);
    std::copy(std::begin(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4),
        std::end(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4), audio->codecParms.data.get());
    audio->codecParms.extradataSize = static_cast<int64_t>(AAC_PROBE_TEST_ASC_SIZE);
    parser.dataReader_ = CreateReaderForBytes(payload);
    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    return parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame);
}
} // namespace

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeWindowReached_KeepsSamplePerFrame_001
 * @tc.desc: Verify AAC keeps audio_samples_per_frame when the FFmpeg probe window
 *           reaches the first audio sample (interleaved dts order).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeWindowReached_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto videoHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::VIDEO_TYPE,
        {AAC_PROBE_TEST_SMALL_VIDEO_SAMPLE_SIZE}, AAC_PROBE_TEST_VIDEO_DELTA, 0);
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {AAC_PROBE_TEST_AUDIO_SAMPLE_SIZE}, AAC_PROBE_TEST_AUDIO_DELTA, AAC_PROBE_TEST_SMALL_VIDEO_SAMPLE_SIZE);
    auto audio = PrepareProbeTestTracks(parser, videoHelper, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(videoHelper, nullptr);
    ASSERT_NE(audioHelper, nullptr);

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(samplesPerFrame, AAC_PROBE_TEST_SAMPLES_PER_FRAME);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeWindowExhausted_RemovesSamplePerFrame_001
 * @tc.desc: Verify AAC drops audio_samples_per_frame when cumulative video packets
 *           before the first audio sample already exhaust the FFmpeg probe window.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeWindowExhausted_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    std::vector<int32_t> videoSizes = {AAC_PROBE_TEST_LARGE_VIDEO_SAMPLE_SIZE,
        AAC_PROBE_TEST_LARGE_VIDEO_SAMPLE_SIZE, AAC_PROBE_TEST_LARGE_VIDEO_SAMPLE_SIZE};
    const int64_t audioOffset = static_cast<int64_t>(AAC_PROBE_TEST_LARGE_VIDEO_SAMPLE_SIZE) * 3;
    auto videoHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::VIDEO_TYPE, videoSizes,
        AAC_PROBE_TEST_VIDEO_DELTA, 0);
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {AAC_PROBE_TEST_AUDIO_SAMPLE_SIZE}, AAC_PROBE_TEST_AUDIO_DELTA, audioOffset);
    auto audio = PrepareProbeTestTracks(parser, videoHelper, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(videoHelper, nullptr);
    ASSERT_NE(audioHelper, nullptr);

    const int64_t videoBytesBeforeAudio = static_cast<int64_t>(AAC_PROBE_TEST_LARGE_VIDEO_SAMPLE_SIZE) * 3;
    EXPECT_GE(videoBytesBeforeAudio, AAC_PROBE_TEST_FFMPEG_PROBE_SIZE);
    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    EXPECT_FALSE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeWindowAudioFirst_KeepsSamplePerFrame_001
 * @tc.desc: Verify an AAC track selected first by the probe window keeps the value.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeWindowAudioFirst_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {AAC_PROBE_TEST_AUDIO_SAMPLE_SIZE}, AAC_PROBE_TEST_AUDIO_DELTA, 0);
    auto audio = PrepareProbeTestTracks(parser, nullptr, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(audioHelper, nullptr);

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(samplesPerFrame, AAC_PROBE_TEST_SAMPLES_PER_FRAME);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeWindowNoSamples_RemovesSamplePerFrame_001
 * @tc.desc: Verify an AAC track without samples never reaches the probe decoder.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeWindowNoSamples_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto audio = PrepareProbeTestTracks(parser, nullptr, nullptr);
    ASSERT_NE(audio, nullptr);

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    EXPECT_FALSE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeFragmentedUnseekable_KeepsSamplePerFrame_001
 * @tc.desc: Verify a fragmented unseekable input keeps the ASC-exported AAC frame size: the
 *           FFmpeg probe interleave model does not apply and the probe window is skipped.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeFragmentedUnseekable_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {AAC_PROBE_TEST_AUDIO_SAMPLE_SIZE}, AAC_PROBE_TEST_AUDIO_DELTA, 0);
    auto audio = PrepareProbeTestTracks(parser, nullptr, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(audioHelper, nullptr);
    parser.hasMoofBox_ = true;
    parser.seekable_ = Seekable::UNSEEKABLE;

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(samplesPerFrame, AAC_PROBE_TEST_SAMPLES_PER_FRAME);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeMaxAnalyzeDuration_RemovesSamplePerFrame_001
 * @tc.desc: Verify the probe stops on max_stream_analyze_duration (5 s) inside a video-first
 *           zero-duration stts run (bbcnews-original.mp4 shape) before the audio track is
 *           reached, so audio_samples_per_frame is not exported.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeMaxAnalyzeDuration_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SttsEntry> sttsEntries = {
        {159, 0}, {1, 29}, {2442, 40}};
    std::vector<int32_t> videoSizes(AAC_PROBE_TEST_R5_VIDEO_SAMPLE_COUNT,
        AAC_PROBE_TEST_R5_VIDEO_SAMPLE_SIZE);
    auto videoHelper = BuildProbeTestSampleHelperWithStts(Plugins::MPEG4::VIDEO_TYPE, videoSizes,
        sttsEntries, 0);
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {AAC_PROBE_TEST_AUDIO_SAMPLE_SIZE}, AAC_PROBE_TEST_AUDIO_DELTA, AAC_PROBE_TEST_R5_AUDIO_OFFSET);
    auto audio = PrepareProbeTestTracks(parser, videoHelper, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(videoHelper, nullptr);
    ASSERT_NE(audioHelper, nullptr);
    parser.firstTrack_->timeScale = AAC_PROBE_TEST_R5_VIDEO_TIMESCALE;

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    EXPECT_FALSE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeFirstSampleValid_KeepsSamplePerFrame_001
 * @tc.desc: Verify the lightweight AAC frame-structure check accepts a structurally valid
 *           first packet and keeps audio_samples_per_frame (R6/R8 shape).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeFirstSampleValid_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    std::vector<uint8_t> payload = BuildAacProbeCpeRawDataBlock(true);
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {static_cast<int32_t>(payload.size())}, AAC_PROBE_TEST_AUDIO_DELTA, 0);
    auto audio = PrepareProbeTestTracks(parser, nullptr, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(audioHelper, nullptr);
    audio->codecParms.data = std::make_unique<uint8_t[]>(AAC_PROBE_TEST_ASC_SIZE);
    std::copy(std::begin(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4),
        std::end(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4), audio->codecParms.data.get());
    audio->codecParms.extradataSize = static_cast<int64_t>(AAC_PROBE_TEST_ASC_SIZE);
    parser.dataReader_ = CreateReaderForBytes(payload);

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(samplesPerFrame, AAC_PROBE_TEST_SAMPLES_PER_FRAME);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeFirstSampleInvalid_RemovesSamplePerFrame_001
 * @tc.desc: Verify a structurally invalid first AAC packet (max_sfb beyond the sampling-rate
 *           table) never reaches the probe decoder and drops audio_samples_per_frame (R7 shape).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeFirstSampleInvalid_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    std::vector<uint8_t> payload = BuildAacProbeCpeRawDataBlock(false);
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {static_cast<int32_t>(payload.size())}, AAC_PROBE_TEST_AUDIO_DELTA, 0);
    auto audio = PrepareProbeTestTracks(parser, nullptr, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(audioHelper, nullptr);
    audio->codecParms.data = std::make_unique<uint8_t[]>(AAC_PROBE_TEST_ASC_SIZE);
    std::copy(std::begin(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4),
        std::end(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4), audio->codecParms.data.get());
    audio->codecParms.extradataSize = static_cast<int64_t>(AAC_PROBE_TEST_ASC_SIZE);
    parser.dataReader_ = CreateReaderForBytes(payload);

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    EXPECT_FALSE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeShortWindowGroups_KeepsSamplePerFrame_001
 * @tc.desc: Verify a real EIGHT_SHORT CPE first packet (meta* resources shape: two window
 *           groups, ms_mask over num_window_groups * max_sfb, band sections per group)
 *           passes the probe check and keeps audio_samples_per_frame = 1024.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeShortWindowGroups_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    Plugins::MPEG4::MPEG4AtomParser parser;
    std::vector<uint8_t> payload = BuildAacProbeShortCpeRawDataBlock();
    auto audioHelper = BuildProbeTestSampleHelper(Plugins::MPEG4::AUDIO_TYPE,
        {static_cast<int32_t>(payload.size())}, AAC_PROBE_TEST_AUDIO_DELTA, 0);
    auto audio = PrepareProbeTestTracks(parser, nullptr, audioHelper);
    ASSERT_NE(audio, nullptr);
    ASSERT_NE(audioHelper, nullptr);
    audio->codecParms.data = std::make_unique<uint8_t[]>(AAC_PROBE_TEST_ASC_SIZE);
    std::copy(std::begin(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4),
        std::end(AAC_PROBE_TEST_ASC_SAMPLE_INDEX_4), audio->codecParms.data.get());
    audio->codecParms.extradataSize = static_cast<int64_t>(AAC_PROBE_TEST_ASC_SIZE);
    parser.dataReader_ = CreateReaderForBytes(payload);

    parser.CalculateAacProbeSamplePerFrame();
    parser.NormalizeAudioAttributes(audio);
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(parser.mediaInfo_.tracks[1].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(samplesPerFrame, AAC_PROBE_TEST_SAMPLES_PER_FRAME);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeDsePrefix_KeepsSamplePerFrame_001
 * @tc.desc: Verify a DSE raw-data-block element before the first channel is skipped
 *           (FFmpeg skip_data_stream_element) and a valid trailing CPE still keeps
 *           audio_samples_per_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeDsePrefix_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeDsePrefixedFrame(false);
    EXPECT_TRUE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeDseEscapeCount_KeepsSamplePerFrame_001
 * @tc.desc: Verify the DSE 255 -> +8 count escape path (FFmpeg skip_data_stream_element)
 *           is accepted when enough payload bytes follow.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeDseEscapeCount_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeDsePrefixedFrame(true);
    EXPECT_TRUE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeDseByteAlign_KeepsSamplePerFrame_001
 * @tc.desc: Verify a DSE with data_byte_align_flag=1 after an unaligned FIL element
 *           skips to the byte boundary and a valid trailing CPE keeps
 *           audio_samples_per_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeDseByteAlign_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeDseAlignedFrame();
    EXPECT_TRUE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeDseTruncated_RemovesSamplePerFrame_001
 * @tc.desc: Verify a DSE payload shorter than its count field fails the probe check
 *           and drops audio_samples_per_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeDseTruncated_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeTruncatedDseFrame();
    EXPECT_FALSE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbePcePrefix_KeepsSamplePerFrame_001
 * @tc.desc: Verify a PCE raw-data-block element (element counts, mixdown tag, channel
 *           maps, relative byte alignment, comment) is skipped and a valid trailing CPE
 *           keeps audio_samples_per_frame. The PCE relative byte alignment always finds
 *           the bit position already past the raw-data-block start, which must use the
 *           signed modulo-8 arithmetic of FFmpeg relative_align_get_bits and never the
 *           unsigned subtraction (UBSan sub-overflow crash guard).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbePcePrefix_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbePcePrefixedFrame();
    EXPECT_TRUE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbePceTruncated_RemovesSamplePerFrame_001
 * @tc.desc: Verify a PCE element missing its comment field fails the probe check and
 *           drops audio_samples_per_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbePceTruncated_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeTruncatedPceFrame();
    EXPECT_FALSE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeCceFirst_KeepsSamplePerFrame_001
 * @tc.desc: Verify a CCE raw-data-block element parsed as the first audio element keeps
 *           audio_samples_per_frame when its first coupled channel decodes cleanly.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeCceFirst_KeepsSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeCceFrame();
    EXPECT_TRUE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeCceTruncated_RemovesSamplePerFrame_001
 * @tc.desc: Verify a CCE whose per-couple entries are cut short fails the probe check
 *           and drops audio_samples_per_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeCceTruncated_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeTruncatedCceFrame();
    EXPECT_FALSE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbePredictorPresent_RemovesSamplePerFrame_001
 * @tc.desc: Verify a long-window CPE with predictor_data_present=1 is rejected like the
 *           AAC-LC decoder ("Prediction is not allowed in AAC-LC") and drops
 *           audio_samples_per_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbePredictorPresent_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeCpeFrame(AAC_PROBE_TEST_MAX_SFB,
        AAC_PROBE_TEST_PREDICTOR_PRESENT, AAC_PROBE_TEST_SECTION_TYPE);
    EXPECT_FALSE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeInvalidSectionType_RemovesSamplePerFrame_001
 * @tc.desc: Verify a band section with sect_band_type=12 (FFmpeg "invalid band type")
 *           fails the probe check and drops audio_samples_per_frame.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeInvalidSectionType_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeCpeFrame(AAC_PROBE_TEST_MAX_SFB, 0,
        AAC_PROBE_TEST_INVALID_SECTION_TYPE);
    EXPECT_FALSE(RunAacProbeFrameValidation(payload, static_cast<int32_t>(payload.size())));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeOversizedSample_RemovesSamplePerFrame_001
 * @tc.desc: Verify a first AAC sample larger than the 16 KiB validation cap is rejected
 *           before any read so a corrupt stsz cannot force a multi-MB probe allocation.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeOversizedSample_RemovesSamplePerFrame_001,
    TestSize.Level1)
{
    std::vector<uint8_t> payload = BuildAacProbeCpeRawDataBlock(true);
    EXPECT_FALSE(RunAacProbeFrameValidation(payload, AAC_PROBE_TEST_OVERSIZE_SAMPLE_SIZE));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacLfeProbeKeepsLayoutUnknown_001
 * @tc.desc: Verify a pure LFE first element only proves a channel count and keeps
 *           the layout UNKNOWN instead of reporting a mono front channel.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacLfeProbeKeepsLayoutUnknown_001, TestSize.Level1)
{
    auto data = BuildAacLfeRawDataBlock();
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    mediaInfo.tracks[0].Remove(Tag::AUDIO_CHANNEL_COUNT);
    Plugins::MPEG4::MPEG4AudioParser parser;
    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t channels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_MONO_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::UNKNOWN);

    MediaInfo stereo = MakeAacFrameMediaInfo();
    stereo.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(AAC_TEST_CHANNEL_COUNT);
    stereo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(AAC_TEST_CHANNEL_COUNT);
    stereo.tracks[0].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    stereo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    Plugins::MPEG4::MPEG4AudioParser stereoParser;
    EXPECT_EQ(stereoParser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, stereo), Status::OK);
    ASSERT_TRUE(stereo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(stereo.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacFrameChannelProbeRaisesMetadata_001
 * @tc.desc: Verify a raw AAC CPE raises an understated ASC channel count, matching
 *           FFmpeg stream-info probing where the decoded frame count wins.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacFrameChannelProbeRaisesMetadata_001, TestSize.Level1)
{
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(AAC_TEST_MONO_CHANNEL_COUNT);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(AAC_TEST_MONO_CHANNEL_COUNT);
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::MONO);
    auto data = BuildAacRawDataBlock(AAC_TEST_EXTENSION_OTHER);
    Plugins::MPEG4::MPEG4AudioParser parser;

    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t channels = 0;
    int32_t outputChannels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_CHANNEL_COUNT);
    EXPECT_EQ(outputChannels, AAC_TEST_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacFrameChannelProbePreservesHigherMetadata_001
 * @tc.desc: Verify a raw AAC CPE never lowers a higher container/ASC channel count
 *           (multi-CPE 5.1 frames probe as stereo from the first element only).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacFrameChannelProbePreservesHigherMetadata_001,
    TestSize.Level1)
{
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(AAC_TEST_PCE_CHANNEL_COUNT);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(AAC_TEST_PCE_CHANNEL_COUNT);
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::CH_5POINT1);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::CH_5POINT1);
    auto data = BuildAacRawDataBlock(AAC_TEST_EXTENSION_OTHER);
    Plugins::MPEG4::MPEG4AudioParser parser;

    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t channels = 0;
    int32_t outputChannels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_PCE_CHANNEL_COUNT);
    EXPECT_EQ(outputChannels, AAC_TEST_PCE_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::CH_5POINT1);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacFrameChannelProbeMonoLowerBound_001
 * @tc.desc: Verify a mono SCE frame is a lower bound: it fills missing channel
 *           metadata but never lowers a declared higher count.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacFrameChannelProbeMonoLowerBound_001, TestSize.Level1)
{
    auto data = BuildAacSceRawDataBlock();
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(AAC_TEST_CHANNEL_COUNT);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(AAC_TEST_CHANNEL_COUNT);
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    Plugins::MPEG4::MPEG4AudioParser parser;

    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t channels = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);

    MediaInfo missing = MakeAacFrameMediaInfo();
    missing.tracks[0].Remove(Tag::AUDIO_CHANNEL_COUNT);
    Plugins::MPEG4::MPEG4AudioParser fillParser;
    EXPECT_EQ(fillParser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, missing), Status::OK);
    ASSERT_TRUE(missing.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(missing.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(channels, AAC_TEST_MONO_CHANNEL_COUNT);
    EXPECT_EQ(layout, AudioChannelLayout::MONO);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacFrameChannelProbeKeepsZeroDeclared_001
 * @tc.desc: Verify a raw AAC frame never fills a channel count explicitly declared as
 *           zero (container channelcount=0 and invalid ASC), matching FFmpeg which does
 *           not export channels when the AAC decoder cannot initialize.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacFrameChannelProbeKeepsZeroDeclared_001, TestSize.Level1)
{
    auto data = BuildAacSceRawDataBlock();
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(0);
    Plugins::MPEG4::MPEG4AudioParser parser;

    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t channels = -1;
    int32_t outputChannels = -1;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    EXPECT_EQ(channels, 0);
    EXPECT_EQ(outputChannels, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ExportDropsZeroChannelKeys_001
 * @tc.desc: Verify the export layer drops 0-valued count keys but keeps the layout keys,
 *           matching FFmpeg ParseAudioTrackInfo which exports count keys only when
 *           nb_channels > 0 while exporting the layout unconditionally.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ExportDropsZeroChannelKeys_001, TestSize.Level1)
{
    MediaInfo mediaInfo;
    mediaInfo.tracks.resize(1);
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(0);
    mediaInfo.tracks[0].Set<Tag::AUDIO_SAMPLE_RATE>(AAC_TEST_CORE_SAMPLE_RATE);
    mediaInfo.tracks[0].Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    mediaInfo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);

    Plugins::MPEG4::RemoveZeroChannelKeys(mediaInfo.tracks[0]);
    int32_t channels = -1;
    int32_t outputChannels = -1;
    int32_t sampleRate = 0;
    AudioChannelLayout layout = AudioChannelLayout::UNKNOWN;
    ASSERT_FALSE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_FALSE(mediaInfo.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    // FFmpeg keeps the layout for a zero-count track (mask lookup falls back to
    // GetDefaultChannelLayout(channels), e.g. MONO for channels=0); only the count keys drop.
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNEL_LAYOUT>(layout));
    EXPECT_EQ(layout, AudioChannelLayout::STEREO);
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    EXPECT_EQ(sampleRate, AAC_TEST_CORE_SAMPLE_RATE);

    // A zero output-channel key is dropped independently, while a positive count is kept.
    MediaInfo zeroOutput = MakeAacFrameMediaInfo();
    zeroOutput.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(0);
    Plugins::MPEG4::RemoveZeroChannelKeys(zeroOutput.tracks[0]);
    ASSERT_TRUE(zeroOutput.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_FALSE(zeroOutput.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));

    MediaInfo stereo = MakeAacFrameMediaInfo();
    stereo.tracks[0].Set<Tag::AUDIO_OUTPUT_CHANNELS>(AAC_TEST_CHANNEL_COUNT);
    Plugins::MPEG4::RemoveZeroChannelKeys(stereo.tracks[0]);
    ASSERT_TRUE(stereo.tracks[0].Get<Tag::AUDIO_CHANNEL_COUNT>(channels));
    ASSERT_TRUE(stereo.tracks[0].Get<Tag::AUDIO_OUTPUT_CHANNELS>(outputChannels));
    EXPECT_EQ(channels, AAC_TEST_CHANNEL_COUNT);
    EXPECT_EQ(outputChannels, AAC_TEST_CHANNEL_COUNT);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacExplicitSbrAbsent_001
 * @tc.desc: Verify an explicit ASC SBR-absent signal prevents in-band SBR metadata promotion.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacExplicitSbrAbsent_001, TestSize.Level1)
{
    MediaInfo mediaInfo = MakeAacFrameMediaInfo();
    mediaInfo.tracks[0].Set<Tag::AUDIO_SAMPLE_RATE>(AAC_TEST_96KHZ_SAMPLE_RATE);
    mediaInfo.tracks[0].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(AAC_TEST_96KHZ_SAMPLES_PER_FRAME);
    mediaInfo.tracks[0].Set<Tag::MEDIA_CODEC_CONFIG>(BuildAacAudioSpecificConfig(false));
    auto data = BuildAacRawDataBlock(AAC_TEST_EXTENSION_SBR);
    Plugins::MPEG4::MPEG4AudioParser parser;

    EXPECT_EQ(parser.ParseAudioFrame(data.data(), data.size(), MimeType::AUDIO_AAC, 0, mediaInfo), Status::OK);
    int32_t sampleRate = 0;
    int32_t samplesPerFrame = 0;
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_TRUE(mediaInfo.tracks[0].Get<Tag::AUDIO_SAMPLE_PER_FRAME>(samplesPerFrame));
    EXPECT_EQ(sampleRate, AAC_TEST_96KHZ_SAMPLE_RATE);
    EXPECT_EQ(samplesPerFrame, AAC_TEST_96KHZ_SAMPLES_PER_FRAME);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleMemoryValidBranch_001
 * @tc.desc: Verify MPEG4SampleMemory valid sample and boundary offset branches
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleMemoryValidBranch_001, TestSize.Level1)
{
    constexpr int32_t sampleSize = 8;
    constexpr int32_t startOffset = 3;
    auto sample = CreateMpeg4Sample(sampleSize, true);
    ASSERT_NE(sample, nullptr);
    ASSERT_NE(sample->data, nullptr);

    Plugins::MPEG4::MPEG4SampleMemory memory(sample, startOffset);
    EXPECT_EQ(memory.GetMemoryType(), MemoryType::CUSTOM_MEMORY);
    EXPECT_EQ(memory.GetAddr(), sample->data.get() + startOffset);
    EXPECT_EQ(memory.GetCapacity(), sampleSize - startOffset);
    EXPECT_EQ(memory.GetSize(), sampleSize - startOffset);
    EXPECT_EQ(memory.GetOffset(), 0);
    EXPECT_EQ(memory.Write(nullptr, sampleSize), 0);

    Plugins::MPEG4::MPEG4SampleMemory endOffsetMemory(sample, sampleSize);
    EXPECT_EQ(endOffsetMemory.GetAddr(), sample->data.get() + sampleSize);
    EXPECT_EQ(endOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(endOffsetMemory.GetSize(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleMemoryInvalidBranch_001
 * @tc.desc: Verify MPEG4SampleMemory invalid sample, data, size and offset branches
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleMemoryInvalidBranch_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleMemory nullSampleMemory(nullptr);
    EXPECT_EQ(nullSampleMemory.GetAddr(), nullptr);
    EXPECT_EQ(nullSampleMemory.GetCapacity(), 0);
    EXPECT_EQ(nullSampleMemory.GetSize(), 0);
    EXPECT_EQ(nullSampleMemory.GetOffset(), 0);

    auto sampleWithoutData = CreateMpeg4Sample(8, false);
    Plugins::MPEG4::MPEG4SampleMemory nullDataMemory(sampleWithoutData);
    EXPECT_EQ(nullDataMemory.GetAddr(), nullptr);
    EXPECT_EQ(nullDataMemory.GetCapacity(), 0);
    EXPECT_EQ(nullDataMemory.GetSize(), 0);

    auto negativeSizeSample = CreateMpeg4Sample(-1, false);
    negativeSizeSample->data = std::make_unique<uint8_t[]>(1);
    Plugins::MPEG4::MPEG4SampleMemory negativeSizeMemory(negativeSizeSample);
    EXPECT_EQ(negativeSizeMemory.GetAddr(), nullptr);
    EXPECT_EQ(negativeSizeMemory.GetCapacity(), 0);
    EXPECT_EQ(negativeSizeMemory.GetSize(), 0);

    auto validSample = CreateMpeg4Sample(8, true);
    Plugins::MPEG4::MPEG4SampleMemory negativeOffsetMemory(validSample, -1);
    EXPECT_EQ(negativeOffsetMemory.GetAddr(), nullptr);
    EXPECT_EQ(negativeOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(negativeOffsetMemory.GetSize(), 0);

    Plugins::MPEG4::MPEG4SampleMemory overflowOffsetMemory(validSample, validSample->size + 1);
    EXPECT_EQ(overflowOffsetMemory.GetAddr(), nullptr);
    EXPECT_EQ(overflowOffsetMemory.GetCapacity(), 0);
    EXPECT_EQ(overflowOffsetMemory.GetSize(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroLengthAacSample_001
 * @tc.desc: Verify a legal zero-length AAC sample does not read payload and uses AAC duration fallback
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ZeroLengthAacSample_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    auto track = PrepareZeroLengthAacTrack(plugin);
    ASSERT_NE(track, nullptr);
    EXPECT_EQ(plugin.CheckReadSampleDataState(0), Status::OK);
    EXPECT_EQ(plugin.CheckReadSampleDataState(-1), Status::ERROR_INVALID_PARAMETER);

    auto dataSource = std::make_shared<ShortReadDataSource>(std::vector<uint8_t> {0x01}, 1);
    ASSERT_NE(dataSource, nullptr);
    plugin.dataSource_ = dataSource;
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(track->sampleHelper->GetSampleIndexView(0, view), Status::OK);
    Plugins::MPEG4::detail::SampleReadInfo sampleInfo {0, 0, view, true};
    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    ASSERT_NE(sample, nullptr);
    ASSERT_EQ(plugin.ReadSampleData(sample, sampleInfo), Status::OK);
    EXPECT_EQ(sample->data, nullptr);
    EXPECT_EQ(sample->size, 0);
    EXPECT_EQ(sample->duration, 23219);
    EXPECT_TRUE(dataSource->readOffsets.empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroLengthAacCopyOutput_001
 * @tc.desc: Verify a reused copy buffer is cleared for a legal zero-length AAC sample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ZeroLengthAacCopyOutput_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin(MPEG4_PLUGIN_NAME);
    ASSERT_NE(PrepareZeroLengthAacTrack(plugin), nullptr);
    auto sample = CreateMpeg4Sample(0, false);
    ASSERT_NE(sample, nullptr);
    sample->duration = 23219;
    auto mpeg4Sample = std::make_shared<Plugins::MPEG4::MPEG4Sample>();
    ASSERT_NE(mpeg4Sample, nullptr);
    mpeg4Sample->trackId = 0;
    mpeg4Sample->sample = sample;

    AVBufferWrapper copyBuffer(64);
    ASSERT_NE(copyBuffer.mediaAVBuffer, nullptr);
    copyBuffer.mediaAVBuffer->memory_->SetSize(64);
    ASSERT_EQ(plugin.WriteBuffer(0, copyBuffer.mediaAVBuffer, mpeg4Sample), Status::OK);
    EXPECT_EQ(copyBuffer.mediaAVBuffer->memory_->GetSize(), 0);
    EXPECT_EQ(copyBuffer.mediaAVBuffer->duration_, 23219);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroLengthAacZeroCopyOutput_001
 * @tc.desc: Verify zero-copy output owns valid zero-length memory for a legal AAC sample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ZeroLengthAacZeroCopyOutput_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin(MPEG4_PLUGIN_NAME);
    ASSERT_NE(PrepareZeroLengthAacTrack(plugin), nullptr);
    auto sample = CreateMpeg4Sample(0, false);
    ASSERT_NE(sample, nullptr);
    sample->duration = 23219;
    auto mpeg4Sample = std::make_shared<Plugins::MPEG4::MPEG4Sample>();
    ASSERT_NE(mpeg4Sample, nullptr);
    mpeg4Sample->trackId = 0;
    mpeg4Sample->sample = sample;
    auto zeroCopyBuffer = MediaAVBuffer::CreateAVBuffer();
    ASSERT_NE(zeroCopyBuffer, nullptr);
    ASSERT_EQ(plugin.WriteSampleMemory(0, zeroCopyBuffer, mpeg4Sample), Status::OK);
    ASSERT_NE(zeroCopyBuffer->memory_, nullptr);
    EXPECT_EQ(zeroCopyBuffer->memory_->GetSize(), 0);
    EXPECT_EQ(zeroCopyBuffer->memory_->GetAddr(), nullptr);
    EXPECT_EQ(zeroCopyBuffer->duration_, 23219);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroLengthAacTrailingIndex_001
 * @tc.desc: Verify a legal trailing zero-length AAC entry is built from synthetic sample tables
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ZeroLengthAacTrailingIndex_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper.mimeType_ = MimeType::AUDIO_AAC;
    helper.sttsEntry_ = {{2, ZERO_LENGTH_AAC_SAMPLES_PER_FRAME}};
    helper.stscEntry_ = {{0, 3, 1}};
    helper.sampleSizes_ = {3, 4, 0};
    helper.sampleSizeCount_ = 3U;
    helper.chunkOffsets_ = {100};

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    ASSERT_EQ(helper.GetSampleCount(), 3U);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(helper.GetSampleIndexView(2, view), Status::OK);
    EXPECT_EQ(view.pos, 107);
    EXPECT_EQ(view.dts, 2048);
    EXPECT_EQ(view.pts, 2048);
    EXPECT_EQ(view.size, 0);
    EXPECT_EQ(view.duration, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_BufferConvert_001
 * @tc.desc: Verify AVC pointer and caller-buffer AnnexB conversions produce identical data
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_BufferConvert_001, TestSize.Level1)
{
    const std::vector<uint8_t> avcc = {
        0x01, 0x42, 0x00, 0x1E, 0xFF, 0xE1, 0x00, 0x02, 0x67, 0x42, 0x01, 0x00, 0x02, 0x68, 0xCE,
    };
    const std::vector<uint8_t> packet = {0x00, 0x00, 0x00, 0x02, 0x65, 0x88};

    Plugins::AvcParserImpl legacyParser;
    auto legacyConfig = avcc;
    ASSERT_TRUE(legacyParser.ConvertExtraDataToAnnexb(legacyConfig.data(), static_cast<int32_t>(legacyConfig.size())));
    auto legacyPacket = packet;
    uint8_t* legacyData = legacyPacket.data();
    int32_t legacySize = static_cast<int32_t>(legacyPacket.size());
    ASSERT_TRUE(legacyParser.ConvertPacketToAnnexb(&legacyData, legacySize, nullptr, 0, false));
    const std::vector<uint8_t> expected(legacyData, legacyData + legacySize);

    Plugins::AvcParserImpl bufferParser;
    auto bufferConfig = avcc;
    ASSERT_TRUE(bufferParser.ConvertExtraDataToAnnexb(bufferConfig.data(), static_cast<int32_t>(bufferConfig.size())));
    std::vector<uint8_t> output(expected.size(), 0);
    int32_t outputSize = 0;
    Plugins::PacketConvertToBufferInfo convertInfo(outputSize);
    convertInfo.srcData = packet.data();
    convertInfo.srcDataSize = static_cast<int32_t>(packet.size());
    convertInfo.outBuffer = output.data();
    convertInfo.outBufferSize = static_cast<int32_t>(output.size());
    ASSERT_TRUE(bufferParser.ConvertPacketToAnnexb(convertInfo));
    EXPECT_EQ(outputSize, legacySize);
    EXPECT_EQ(output, expected);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_BufferConvert_002
 * @tc.desc: Verify an insufficient AVC output buffer does not advance parser state
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_BufferConvert_002, TestSize.Level1)
{
    const std::vector<uint8_t> avcc = {
        0x01, 0x42, 0x00, 0x1E, 0xFF, 0xE1, 0x00, 0x02, 0x67, 0x42, 0x01, 0x00, 0x02, 0x68, 0xCE,
    };
    const std::vector<uint8_t> packet = {0x00, 0x00, 0x00, 0x02, 0x65, 0x88};

    Plugins::AvcParserImpl parser;
    auto config = avcc;
    ASSERT_TRUE(parser.ConvertExtraDataToAnnexb(config.data(), static_cast<int32_t>(config.size())));
    std::vector<uint8_t> smallOutput(1, 0xA5);
    int32_t outputSize = 0;
    Plugins::PacketConvertToBufferInfo smallInfo(outputSize);
    smallInfo.srcData = packet.data();
    smallInfo.srcDataSize = static_cast<int32_t>(packet.size());
    smallInfo.outBuffer = smallOutput.data();
    smallInfo.outBufferSize = static_cast<int32_t>(smallOutput.size());
    EXPECT_FALSE(parser.ConvertPacketToAnnexb(smallInfo));
    EXPECT_GT(outputSize, smallInfo.outBufferSize);
    EXPECT_EQ(smallOutput[0], 0xA5);

    std::vector<uint8_t> output(static_cast<size_t>(outputSize), 0);
    Plugins::PacketConvertToBufferInfo retryInfo(outputSize);
    retryInfo.srcData = packet.data();
    retryInfo.srcDataSize = static_cast<int32_t>(packet.size());
    retryInfo.outBuffer = output.data();
    retryInfo.outBufferSize = static_cast<int32_t>(output.size());
    ASSERT_TRUE(parser.ConvertPacketToAnnexb(retryInfo));
    EXPECT_EQ(outputSize, static_cast<int32_t>(output.size()));
    EXPECT_EQ(output[0], 0x00);
    EXPECT_EQ(output[3], 0x01);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_BufferConvert_003
 * @tc.desc: Verify AVC buffer conversion preserves in-band parameter-set and frame-to-frame state
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_BufferConvert_003, TestSize.Level1)
{
    const std::vector<uint8_t> avcc = {
        0x01, 0x42, 0x00, 0x1E, 0xFF, 0xE1, 0x00, 0x02, 0x67, 0x42, 0x01, 0x00, 0x02, 0x68, 0xCE,
    };
    const std::vector<std::vector<uint8_t>> packets = {
        {0x00, 0x00, 0x00, 0x02, 0x67, 0x4D, 0x00, 0x00, 0x00, 0x02, 0x68, 0xEF,
            0x00, 0x00, 0x00, 0x02, 0x65, 0x88},
        {0x00, 0x00, 0x00, 0x02, 0x41, 0x80},
        {0x00, 0x00, 0x00, 0x02, 0x65, 0x88},
    };

    Plugins::AvcParserImpl legacyParser;
    Plugins::AvcParserImpl bufferParser;
    auto legacyConfig = avcc;
    auto bufferConfig = avcc;
    ASSERT_TRUE(legacyParser.ConvertExtraDataToAnnexb(legacyConfig.data(), static_cast<int32_t>(legacyConfig.size())));
    ASSERT_TRUE(bufferParser.ConvertExtraDataToAnnexb(bufferConfig.data(), static_cast<int32_t>(bufferConfig.size())));

    for (const auto& packet : packets) {
        auto legacyPacket = packet;
        uint8_t* legacyData = legacyPacket.data();
        int32_t legacySize = static_cast<int32_t>(legacyPacket.size());
        ASSERT_TRUE(legacyParser.ConvertPacketToAnnexb(&legacyData, legacySize, nullptr, 0, false));

        std::vector<uint8_t> output(packet.size() + avcc.size(), 0);
        int32_t outputSize = 0;
        Plugins::PacketConvertToBufferInfo convertInfo(outputSize);
        convertInfo.srcData = packet.data();
        convertInfo.srcDataSize = static_cast<int32_t>(packet.size());
        convertInfo.outBuffer = output.data();
        convertInfo.outBufferSize = static_cast<int32_t>(output.size());
        ASSERT_TRUE(bufferParser.ConvertPacketToAnnexb(convertInfo));
        ASSERT_EQ(outputSize, legacySize);
        EXPECT_TRUE(std::equal(output.begin(), output.begin() + outputSize, legacyData));
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_BufferConvert_004
 * @tc.desc: Verify empty and malformed AVC packets fail without writing caller memory
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_BufferConvert_004, TestSize.Level1)
{
    const std::vector<uint8_t> avcc = {
        0x01, 0x42, 0x00, 0x1E, 0xFF, 0xE1, 0x00, 0x02, 0x67, 0x42, 0x01, 0x00, 0x02, 0x68, 0xCE,
    };
    const std::vector<uint8_t> malformedPacket = {0x00, 0x00, 0x00, 0x03, 0x65, 0x88};
    Plugins::AvcParserImpl parser;
    auto config = avcc;
    ASSERT_TRUE(parser.ConvertExtraDataToAnnexb(config.data(), static_cast<int32_t>(config.size())));

    std::vector<uint8_t> output(16, 0xA5);
    int32_t outputSize = 0;
    Plugins::PacketConvertToBufferInfo malformedInfo(outputSize);
    malformedInfo.srcData = malformedPacket.data();
    malformedInfo.srcDataSize = static_cast<int32_t>(malformedPacket.size());
    malformedInfo.outBuffer = output.data();
    malformedInfo.outBufferSize = static_cast<int32_t>(output.size());
    EXPECT_FALSE(parser.ConvertPacketToAnnexb(malformedInfo));
    EXPECT_EQ(outputSize, 0);
    EXPECT_EQ(output, std::vector<uint8_t>(output.size(), 0xA5));

    uint8_t emptyPacket = 0;
    Plugins::PacketConvertToBufferInfo emptyInfo(outputSize);
    emptyInfo.srcData = &emptyPacket;
    emptyInfo.srcDataSize = 0;
    emptyInfo.outBuffer = output.data();
    emptyInfo.outBufferSize = static_cast<int32_t>(output.size());
    EXPECT_FALSE(parser.ConvertPacketToAnnexb(emptyInfo));
    EXPECT_EQ(outputSize, 0);
    EXPECT_EQ(output, std::vector<uint8_t>(output.size(), 0xA5));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_InvalidInbandPps_001
 * @tc.desc: Verify truncated and out-of-range in-band PPS units preserve avcC PPS and are not output
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_InvalidInbandPps_001, TestSize.Level1)
{
    const std::vector<uint8_t> expected = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x00, 0x00,
        0x01, 0x68, 0xCE, 0x00, 0x00, 0x00, 0x01, 0x65, 0x88,
    };
    const std::vector<std::vector<uint8_t>> packets = {
        {0x00, 0x00, 0x00, 0x02, 0x65, 0x88, 0x00, 0x00, 0x00, 0x01, 0x68},
        {0x00, 0x00, 0x00, 0x02, 0x65, 0x88, 0x00, 0x00, 0x00, 0x04, 0x68, 0x00, 0x80, 0xE0},
        {0x00, 0x00, 0x00, 0x02, 0x65, 0x88, 0x00, 0x00, 0x00, 0x03, 0x68, 0x82, 0x18},
    };
    for (const auto& packet : packets) {
        Plugins::AvcParserImpl parser;
        auto config = H264_AVCC_FIXTURE;
        ASSERT_TRUE(parser.ConvertExtraDataToAnnexb(config.data(), static_cast<int32_t>(config.size())));
        std::vector<uint8_t> output(expected.size(), 0);
        int32_t outputSize = 0;
        Plugins::PacketConvertToBufferInfo convertInfo(outputSize);
        convertInfo.srcData = packet.data();
        convertInfo.srcDataSize = static_cast<int32_t>(packet.size());
        convertInfo.outBuffer = output.data();
        convertInfo.outBufferSize = static_cast<int32_t>(output.size());
        ASSERT_TRUE(parser.ConvertPacketToAnnexb(convertInfo));
        EXPECT_EQ(outputSize, static_cast<int32_t>(expected.size()));
        EXPECT_EQ(output, expected);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_ValidInbandPps_001
 * @tc.desc: Verify a complete in-band PPS with valid IDs is retained in AnnexB output
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_ValidInbandPps_001, TestSize.Level1)
{
    const std::vector<uint8_t> packet = {
        0x00, 0x00, 0x00, 0x02, 0x68, 0xE0, 0x00, 0x00, 0x00, 0x02, 0x65, 0x88,
    };
    const std::vector<uint8_t> expected = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x00, 0x00,
        0x01, 0x68, 0xE0, 0x00, 0x00, 0x00, 0x01, 0x65, 0x88,
    };
    Plugins::AvcParserImpl parser;
    auto config = H264_AVCC_FIXTURE;
    ASSERT_TRUE(parser.ConvertExtraDataToAnnexb(config.data(), static_cast<int32_t>(config.size())));
    std::vector<uint8_t> output(expected.size(), 0);
    int32_t outputSize = 0;
    Plugins::PacketConvertToBufferInfo convertInfo(outputSize);
    convertInfo.srcData = packet.data();
    convertInfo.srcDataSize = static_cast<int32_t>(packet.size());
    convertInfo.outBuffer = output.data();
    convertInfo.outBufferSize = static_cast<int32_t>(output.size());
    ASSERT_TRUE(parser.ConvertPacketToAnnexb(convertInfo));
    EXPECT_EQ(outputSize, static_cast<int32_t>(expected.size()));
    EXPECT_EQ(output, expected);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_InvalidPpsCallerBuffer_001
 * @tc.desc: Verify an invalid PPS does not exceed a sample-plus-codec-config caller buffer
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_InvalidPpsCallerBuffer_001, TestSize.Level1)
{
    const std::vector<uint8_t> packet = {
        0x00, 0x00, 0x00, 0x02, 0x65, 0x88, 0x00, 0x00, 0x00, 0x04, 0x68, 0x00, 0x80, 0xE0,
    };
    const std::vector<uint8_t> expectedCodecConfig = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x00, 0x00, 0x01, 0x68, 0xCE,
    };
    const std::vector<uint8_t> expected = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0x00, 0x00, 0x00,
        0x01, 0x68, 0xCE, 0x00, 0x00, 0x00, 0x01, 0x65, 0x88,
    };
    Plugins::AvcParserImpl parser;
    auto config = H264_AVCC_FIXTURE;
    ASSERT_TRUE(parser.ConvertExtraDataToAnnexb(config.data(), static_cast<int32_t>(config.size())));
    std::vector<uint8_t> output(packet.size() + expectedCodecConfig.size(), 0);
    int32_t outputSize = 0;
    Plugins::PacketConvertToBufferInfo convertInfo(outputSize);
    convertInfo.srcData = packet.data();
    convertInfo.srcDataSize = static_cast<int32_t>(packet.size());
    convertInfo.outBuffer = output.data();
    convertInfo.outBufferSize = static_cast<int32_t>(output.size());
    ASSERT_TRUE(parser.ConvertPacketToAnnexb(convertInfo));
    EXPECT_EQ(outputSize, static_cast<int32_t>(expected.size()));
    EXPECT_TRUE(std::equal(expected.begin(), expected.end(), output.begin(), output.begin() + outputSize));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_ZeroLengthNals_001
 * @tc.desc: Verify zero-length AVCC NAL units produce FFmpeg-compatible empty packet.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_ZeroLengthNals_001, TestSize.Level1)
{
    const std::vector<uint8_t> packet(12, 0);
    Plugins::AvcParserImpl parser;
    auto config = H264_AVCC_FIXTURE;
    ASSERT_TRUE(parser.ConvertExtraDataToAnnexb(config.data(), static_cast<int32_t>(config.size())));
    int32_t outputSize = -1;
    Plugins::PacketConvertToBufferInfo convertInfo(outputSize);
    convertInfo.srcData = packet.data();
    convertInfo.srcDataSize = static_cast<int32_t>(packet.size());
    convertInfo.outBuffer = nullptr;
    convertInfo.outBufferSize = 0;
    EXPECT_TRUE(parser.ConvertPacketToAnnexb(convertInfo));
    EXPECT_EQ(outputSize, 0);

    const std::vector<uint8_t> truncatedPacket(2, 0);
    convertInfo.srcData = truncatedPacket.data();
    convertInfo.srcDataSize = static_cast<int32_t>(truncatedPacket.size());
    EXPECT_FALSE(parser.ConvertPacketToAnnexb(convertInfo));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcParser_InvalidPpsOnlyPacket_001
 * @tc.desc: Verify a packet containing only an invalid in-band PPS yields an empty AnnexB packet
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcParser_InvalidPpsOnlyPacket_001, TestSize.Level1)
{
    const std::vector<uint8_t> packet = {
        0x00, 0x00, 0x00, 0x01, 0x68, // in-band PPS with no payload (NAL header only)
    };
    Plugins::AvcParserImpl parser;
    auto config = H264_AVCC_FIXTURE;
    ASSERT_TRUE(parser.ConvertExtraDataToAnnexb(config.data(), static_cast<int32_t>(config.size())));
    int32_t outputSize = -1;
    Plugins::PacketConvertToBufferInfo convertInfo(outputSize);
    convertInfo.srcData = packet.data();
    convertInfo.srcDataSize = static_cast<int32_t>(packet.size());
    convertInfo.outBuffer = nullptr;
    convertInfo.outBufferSize = 0;
    EXPECT_TRUE(parser.ConvertPacketToAnnexb(convertInfo));
    EXPECT_EQ(outputSize, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer full-frame read results on h264 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    ASSERT_NO_FATAL_FAILURE(CompareReadDigest(mpeg4Digest, ffmpegDigest));
}

void Mpeg4DemuxerPluginUnitTest::CheckEosWaitForCache()
{
    EosWaitForCacheContext context;
    ASSERT_NO_FATAL_FAILURE(PrepareEosWaitForCache(context));
    ASSERT_NO_FATAL_FAILURE(RunEosWaitForCacheRace(context));

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    EXPECT_EQ(context.plugin->ReadSample(context.trackId, buffer.mediaAVBuffer,
        EOS_WAIT_FOR_CACHE_TEST_TIMEOUT_MS), Status::OK);
    EXPECT_TRUE(IsEos(buffer.mediaAVBuffer));
}

void Mpeg4DemuxerPluginUnitTest::PrepareEosWaitForCache(EosWaitForCacheContext& context)
{
    context.plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(context.plugin, nullptr);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(context.plugin);
    InitPlugin(TEST_MP4_PATH_H264_B, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());
    context.trackId = selectedTrackIds.front();

    {
        std::lock_guard<std::mutex> lock(context.plugin->readConfigMutex_);
        context.plugin->EnterRuntimeLocked();
        context.readConfigEpoch = context.plugin->readConfigEpoch_;
        context.plugin->trackId_.store(context.trackId, std::memory_order_release);
    }
    context.plugin->readLoopStatus_.store(Status::END_OF_STREAM, std::memory_order_release);
    context.plugin->readThread_ = std::make_unique<std::thread>([] {});
    context.plugin->readThread_->join();
}

void Mpeg4DemuxerPluginUnitTest::RunEosWaitForCacheRace(EosWaitForCacheContext& context)
{
    auto plugin = context.plugin;
    auto timeout = std::chrono::milliseconds(EOS_WAIT_FOR_CACHE_TEST_TIMEOUT_MS);
    StartEosCacheWriter(context);
    StartEosLoopWaiter(context);
    bool readerWaiting = false;
    {
        std::unique_lock<std::mutex> lock(plugin->readSampleMutex_);
        readerWaiting = plugin->readCacheCv_.wait_for(lock, timeout, [plugin] {
            return plugin->isWaitingForReadThread_.load(std::memory_order_acquire);
        });
    }
    if (!readerWaiting) {
        {
            std::lock_guard<std::mutex> lock(context.gateMutex);
            context.allowEosPush = true;
        }
        context.gateCv.notify_all();
        context.waiter.join();
        context.eosWriter.join();
        FAIL() << "Reader did not wait for the EOS cache entry";
        return;
    }
    {
        std::lock_guard<std::mutex> lock(context.gateMutex);
        context.allowEosPush = true;
    }
    context.gateCv.notify_all();
    {
        std::unique_lock<std::mutex> lock(context.gateMutex);
        EXPECT_TRUE(context.gateCv.wait_for(lock, timeout, [&context] { return context.eosPushFinished; }));
    }
    context.waiter.join();
    context.eosWriter.join();
    EXPECT_EQ(context.waitStatus.load(std::memory_order_acquire), Status::OK);
    EXPECT_EQ(context.enqueueStatus.load(std::memory_order_acquire), Status::OK);
    EXPECT_EQ(context.duplicateEnqueueStatus.load(std::memory_order_acquire), Status::OK);
    EXPECT_EQ(context.eosCacheFrameCount.load(std::memory_order_acquire), 1U);
}

void Mpeg4DemuxerPluginUnitTest::StartEosCacheWriter(EosWaitForCacheContext& context)
{
    auto plugin = context.plugin;
    auto trackId = context.trackId;
    context.eosWriter = std::thread([&context, plugin, trackId] {
        {
            std::unique_lock<std::mutex> lock(context.gateMutex);
            context.gateCv.wait(lock, [&context] { return context.allowEosPush; });
        }
        context.enqueueStatus.store(plugin->PushEosToCache(trackId, context.readConfigEpoch),
            std::memory_order_release);
        context.duplicateEnqueueStatus.store(plugin->PushEosToCache(trackId, context.readConfigEpoch),
            std::memory_order_release);
        context.eosCacheFrameCount.store(plugin->GetTrackCacheFrameCount(trackId), std::memory_order_release);
        {
            std::lock_guard<std::mutex> lock(context.gateMutex);
            context.eosPushFinished = true;
        }
        context.gateCv.notify_all();
        std::lock_guard<std::mutex> lock(plugin->readSampleMutex_);
        plugin->readCacheCv_.notify_all();
    });
}

void Mpeg4DemuxerPluginUnitTest::StartEosLoopWaiter(EosWaitForCacheContext& context)
{
    auto plugin = context.plugin;
    context.waiter = std::thread([&context, plugin] {
        context.waitStatus.store(plugin->WaitForLoop(context.trackId, EOS_WAIT_FOR_CACHE_TEST_TIMEOUT_MS),
            std::memory_order_release);
    });
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EosWaitForCache_001
 * @tc.desc: Prefer an EOS cache entry over the read-loop EOS status
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EosWaitForCache_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckEosWaitForCache());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncEosAfterSeek_Copy_CompareWithFfmpeg_001
 * @tc.desc: Compare cached EOS status and output flag after a near-end copy read
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_AsyncEosAfterSeek_Copy_CompareWithFfmpeg_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckAsyncEosAfterSeek(ReadMode::COPY));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncEosAfterSeek_ZeroCopy_CompareWithFfmpeg_001
 * @tc.desc: Compare cached EOS status and output flag after a near-end zero-copy read
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_AsyncEosAfterSeek_ZeroCopy_CompareWithFfmpeg_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckAsyncEosAfterSeek(ReadMode::ZERO_COPY));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_WebVtt_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer WebVTT sample attributes on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_MP4_WebVtt_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_WEBVTT);
    InitMpeg4Plugin(TEST_MP4_PATH_WEBVTT);

    auto ffmpegTrackId = FindTrackByMime(ffmpegMediaInfo_, WEBVTT_MIME);
    auto mpeg4TrackId = FindTrackByMime(mpeg4MediaInfo_, WEBVTT_MIME);
    ASSERT_NE(ffmpegTrackId, INVALID_TRACK_ID);
    ASSERT_NE(mpeg4TrackId, INVALID_TRACK_ID);
    std::string ffmpegTitle;
    std::string mpeg4Title;
    bool hasFfmpegTitle = ffmpegMediaInfo_.tracks[ffmpegTrackId].Get<Tag::MEDIA_TITLE>(ffmpegTitle);
    bool hasMpeg4Title = mpeg4MediaInfo_.tracks[mpeg4TrackId].Get<Tag::MEDIA_TITLE>(mpeg4Title);
    EXPECT_EQ(hasMpeg4Title, hasFfmpegTitle);
    if (hasFfmpegTitle) {
        EXPECT_EQ(mpeg4Title, ffmpegTitle);
    }
    std::vector<uint8_t> ffmpegCodecConfig;
    std::vector<uint8_t> mpeg4CodecConfig;
    ASSERT_TRUE(ffmpegMediaInfo_.tracks[ffmpegTrackId].Get<Tag::MEDIA_CODEC_CONFIG>(ffmpegCodecConfig));
    ASSERT_TRUE(mpeg4MediaInfo_.tracks[mpeg4TrackId].Get<Tag::MEDIA_CODEC_CONFIG>(mpeg4CodecConfig));
    EXPECT_EQ(mpeg4CodecConfig, ffmpegCodecConfig);
    ASSERT_EQ(ffmpegPlugin_->SelectTrack(ffmpegTrackId), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SelectTrack(mpeg4TrackId), Status::OK);

    auto readSample = [this](const std::shared_ptr<BaseDemuxerPlugin>& plugin, uint32_t trackId,
        const std::shared_ptr<MediaAVBuffer>& buffer) {
        return ReadSampleWithRetry(plugin, trackId, buffer, ReadMode::COPY);
    };
    auto isEos = [this](const std::shared_ptr<MediaAVBuffer>& buffer) { return IsEos(buffer); };
    auto ffmpegSamples = ReadTrackSamples(ffmpegPlugin_, ffmpegTrackId, readSample, isEos);
    auto mpeg4Samples = ReadTrackSamples(mpeg4Plugin_, mpeg4TrackId, readSample, isEos);
    ASSERT_FALSE(ffmpegSamples.empty());
    CompareWebVttFrameSamples(mpeg4Samples, ffmpegSamples);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_WebVttPendingCue_001
 * @tc.desc: Verify WebVTT pending cues are flushed by track and keep sample duration priority
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_WebVttPendingCue_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    auto track0 = MakeWebVttTrack(0, 10000);
    auto track1 = MakeWebVttTrack(1, 20000);
    track0->next = track1;
    plugin->firstTrack_ = track0;
    plugin->lastTrack_ = track1;
    plugin->mediaInfo_.tracks.resize(2);
    plugin->mediaInfo_.tracks[0].Set<Tag::MIME_TYPE>(WEBVTT_MIME);
    plugin->mediaInfo_.tracks[1].Set<Tag::MIME_TYPE>(WEBVTT_MIME);
    plugin->mediaInfo_.general.Set<Tag::MEDIA_DURATION>(60000);

    plugin->pendingWebVttSamples_[0] = MakePendingWebVttSample(1000, 2000);
    plugin->pendingWebVttSamples_[1] = MakePendingWebVttSample(5000, 1000);
    plugin->selectedTrackIds_ = {0, 1};
    plugin->trackMtx_[0] = std::make_shared<std::mutex>();
    plugin->trackMtx_[1] = std::make_shared<std::mutex>();
    ASSERT_EQ(plugin->FlushPendingWebVTTSample(0, plugin->readConfigEpoch_), Status::OK);
    EXPECT_EQ(plugin->pendingWebVttSamples_.count(0), 0U);
    EXPECT_EQ(plugin->pendingWebVttSamples_.count(1), 1U);
    ASSERT_NE(track0->cache, nullptr);
    ASSERT_NE(track1->cache, nullptr);
    ASSERT_TRUE(track0->cache->HasCache());
    ASSERT_FALSE(track1->cache->HasCache());
    auto flushedTrack0 = track0->cache->Front();
    ASSERT_NE(flushedTrack0, nullptr);
    ASSERT_NE(flushedTrack0->sample, nullptr);
    EXPECT_EQ(flushedTrack0->sample->duration, 2000);

    ASSERT_EQ(plugin->FlushPendingWebVTTSamples(), Status::OK);
    EXPECT_TRUE(plugin->pendingWebVttSamples_.empty());
    ASSERT_TRUE(track1->cache->HasCache());
    auto flushedTrack1 = track1->cache->Front();
    ASSERT_NE(flushedTrack1, nullptr);
    ASSERT_NE(flushedTrack1->sample, nullptr);
    EXPECT_EQ(flushedTrack1->sample->duration, 1000);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_WebVttContinuousCue_001
 * @tc.desc: Verify continuous WebVTT cues update previous cue duration with next cue pts
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_WebVttContinuousCue_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    auto track = MakeWebVttTrack(0, 10000);
    plugin->firstTrack_ = track;
    plugin->lastTrack_ = track;
    plugin->mediaInfo_.tracks.resize(1);
    plugin->mediaInfo_.tracks[0].Set<Tag::MIME_TYPE>(WEBVTT_MIME);

    auto firstCue = MakeWebVttCueSample(1000, 100, "first");
    auto secondCue = MakeWebVttCueSample(2500, 100, "second");
    plugin->selectedTrackIds_ = {0};
    plugin->trackMtx_[0] = std::make_shared<std::mutex>();
    ASSERT_EQ(plugin->ProcessWebVTTSampleForCache(track, firstCue, plugin->readConfigEpoch_), Status::OK);
    ASSERT_NE(track->cache, nullptr);
    EXPECT_FALSE(track->cache->HasCache());
    ASSERT_EQ(plugin->ProcessWebVTTSampleForCache(track, secondCue, plugin->readConfigEpoch_), Status::OK);
    ASSERT_TRUE(track->cache->HasCache());
    auto flushedFirstCue = track->cache->Front();
    ASSERT_NE(flushedFirstCue, nullptr);
    ASSERT_NE(flushedFirstCue->sample, nullptr);
    EXPECT_EQ(flushedFirstCue->sample->pts, 1000);
    EXPECT_EQ(flushedFirstCue->sample->duration, 1500);
    EXPECT_EQ(plugin->pendingWebVttSamples_.count(0), 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_WebVttSeekAndTimeScaleBoundary_001
 * @tc.desc: Verify WebVTT seek fallback index and rounded timescale boundary handling
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_WebVttSeekAndTimeScaleBoundary_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.mimeType_ = MimeType::TEXT_WEBVTT;
    helper.totalDuration_ = 5000;
    helper.sampleIndexEntry_ = {
        MakeSampleIndexEntry(0, 0, 0, 1),
        MakeSampleIndexEntry(1, 1000, 1000, 1),
    };
    helper.allSamplesSync_ = true;
    int64_t currentIndex = 1;
    uint32_t sampleIndex = 99;
    EXPECT_TRUE(helper.ResolveTextWebVttSample(1000, currentIndex, sampleIndex));
    EXPECT_EQ(sampleIndex, 1U);

    int64_t result = 0;
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRound(std::numeric_limits<int64_t>::min(), 1, 1, &result),
        Status::OK);
    EXPECT_EQ(result, std::numeric_limits<int64_t>::min());
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRoundToTimeBase(std::numeric_limits<int64_t>::min(), 1, 2, &result),
        Status::OK);
    EXPECT_EQ(result, std::numeric_limits<int64_t>::min() / 2);
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRound(std::numeric_limits<int64_t>::max(), 2, 1, &result),
        Status::ERROR_INVALID_DATA);
    EXPECT_EQ(Plugins::MPEG4::ConvertTimeScaleRoundToTimeBase(1, 1, 0, &result), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg edit-list fixed index attributes on h264 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompareMovFixIndexResultWithFfmpeg(TEST_MP4_PATH_H264_B));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_002
 * @tc.desc: Compare MPEG4 and FFmpeg edit-list fixed index attributes on h265 mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MovFixIndex_CompareWithFfmpeg_002, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompareMovFixIndexResultWithFfmpeg(TEST_MP4_PATH_H265_B));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSampleZeroCopy_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer zero-copy first-sample result on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSampleZeroCopy_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegDigest = ReadFirstSample(ffmpegPlugin_, ffmpegMediaInfo_, ReadMode::ZERO_COPY);
    auto mpeg4Digest = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::ZERO_COPY);
    ASSERT_NO_FATAL_FAILURE(CompareSampleDigest(mpeg4Digest, ffmpegDigest));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSampleZeroCopy_Hevc_001
 * @tc.desc: Verify HEVC copy and zero-copy reads produce the same first sample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSampleZeroCopy_Hevc_001, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    InitMpeg4Plugin(TEST_MP4_PATH_H265_B);
    auto copyDigest = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::COPY);

    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H265_B);
    auto zeroCopyDigest = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::ZERO_COPY);
    ASSERT_NO_FATAL_FAILURE(CompareSampleDigest(zeroCopyDigest, copyDigest));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSampleZeroCopy_Vvc_001
 * @tc.desc: Verify VVC copy and zero-copy reads produce the same first sample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSampleZeroCopy_Vvc_001, TestSize.Level1)
{
    if (access(VVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "vvc parser library not found";
    }
    InitMpeg4Plugin(TEST_MP4_PATH_VVC);
    auto copyDigest = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::COPY);

    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_VVC);
    auto zeroCopyDigest = ReadFirstSample(mpeg4Plugin_, mpeg4MediaInfo_, ReadMode::ZERO_COPY);
    ASSERT_NO_FATAL_FAILURE(CompareSampleDigest(zeroCopyDigest, copyDigest));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer seek and remaining-frame read results on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_CompareWithFfmpeg_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {0, 1000, 2000, 3000};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(CompareSeekReadResult(TEST_MP4_PATH_H264_B, seekTimes, seekModes));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_MultiTrackSyncAuxiliarySeekFailure_001
 * @tc.desc: Verify an auxiliary audio seek miss does not fail the resolved primary video seek
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_MultiTrackSyncAuxiliarySeekFailure_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(plugin, nullptr);
    auto videoTrack = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto audioTrack = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    ASSERT_NE(videoTrack, nullptr);
    ASSERT_NE(audioTrack, nullptr);
    videoTrack->trackIndex = 0;
    videoTrack->next = audioTrack;
    audioTrack->trackIndex = 1;
    audioTrack->timeScale = 1000;
    audioTrack->currentSampleIndex = MULTI_TRACK_SYNC_TEST_INITIAL_SAMPLE_POS;
    audioTrack->currentSamplePos = MULTI_TRACK_SYNC_TEST_INITIAL_SAMPLE_POS;
    audioTrack->sampleHelper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    ASSERT_NE(audioTrack->sampleHelper, nullptr);
    audioTrack->sampleHelper->trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    plugin->firstTrack_ = videoTrack;
    plugin->selectedTrackIds_ = {0, 1};

    EXPECT_EQ(plugin->MultiTrackSync(0, true, MULTI_TRACK_SYNC_TEST_SEEK_TIME, SeekMode::SEEK_NEXT_SYNC),
        Status::OK);
    EXPECT_EQ(audioTrack->currentSampleIndex, MULTI_TRACK_SYNC_TEST_INITIAL_SAMPLE_POS);
    EXPECT_EQ(audioTrack->currentSamplePos, MULTI_TRACK_SYNC_TEST_INITIAL_SAMPLE_POS);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_SourceFirstSample_001
 * @tc.desc: Verify first video sample after PTS seek against source sync-sample timeline on B-frame mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_SourceFirstSample_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {1920, 2160, 2200, 2440, 2600, 2700};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(CheckSeekFirstVideoSampleWithSourceTimeline(TEST_MP4_PATH_H264_B, seekTimes, seekModes));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_FfprobeFixedFrames_001
 * @tc.desc: Verify PTS seek against fixed ffprobe packet timestamps on B-frame mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_FfprobeFixedFrames_001, TestSize.Level1)
{
    for (const auto& expected : GetH264BFfprobeSeekExpectedResults()) {
        SCOPED_TRACE("seekTime=" + std::to_string(expected.seekTimeMs) +
            ", mode=" + std::to_string(static_cast<int32_t>(expected.seekMode)));
        InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
        auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
        uint32_t videoTrackId = FindFirstVideoTrack(mpeg4MediaInfo_, selectedTrackIds);
        ASSERT_NE(videoTrackId, INVALID_TRACK_ID);

        int64_t realSeekTime = 0;
        ASSERT_EQ(mpeg4Plugin_->SeekTo(-1, expected.seekTimeMs, expected.seekMode, realSeekTime), Status::OK);
        EXPECT_EQ(realSeekTime, expected.realSeekTimeNs);

        AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
        ASSERT_EQ(ReadSampleWithRetry(mpeg4Plugin_, videoTrackId, buffer.mediaAVBuffer, ReadMode::COPY), Status::OK);
        EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
        EXPECT_EQ(buffer.mediaAVBuffer->pts_, expected.ptsUs);
        EXPECT_EQ(buffer.mediaAVBuffer->dts_, expected.dtsUs);
        EXPECT_EQ(buffer.mediaAVBuffer->duration_, expected.durationUs);
        EXPECT_NE(buffer.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME), 0U);
        ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_ReadDigest_001
 * @tc.desc: Verify MPEG4 remaining video samples after PTS seek against source timeline on B-frame mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_ReadDigest_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {1920, 2160, 2200, 2440, 2600, 2700};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(CheckSeekReadResultWithSourceTimeline(TEST_MP4_PATH_H264_B, seekTimes, seekModes));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToPtsTimeline_Uri_001
 * @tc.desc: Verify first video sample after PTS seek against source sync-sample timeline on B-frame mp4 uri source
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToPtsTimeline_Uri_001, TestSize.Level1)
{
    std::vector<int64_t> seekTimes = {1920, 2160, 2200, 2440, 2600, 2700};
    std::vector<SeekMode> seekModes = {
        SeekMode::SEEK_PREVIOUS_SYNC,
        SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC,
    };
    ASSERT_NO_FATAL_FAILURE(
        CheckSeekFirstVideoSampleWithSourceTimeline(TEST_MP4_PATH_H264_B, seekTimes, seekModes, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekToStart_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer SeekToStart results after reading a sample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekToStart_CompareWithFfmpeg_001, TestSize.Level1)
{
    InitFfmpegPlugin(TEST_MP4_PATH_H264_B);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);

    auto ffmpegSelectedTracks = SelectSupportedTracks(ffmpegPlugin_, ffmpegMediaInfo_);
    auto mpeg4SelectedTracks = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_EQ(ffmpegSelectedTracks.size(), mpeg4SelectedTracks.size());

    auto ffmpegPreSeekDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks);
    auto mpeg4PreSeekDigest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks);
    CompareReadDigest(mpeg4PreSeekDigest, ffmpegPreSeekDigest);

    ASSERT_EQ(ffmpegPlugin_->SeekToStart(), Status::OK);
    ASSERT_EQ(mpeg4Plugin_->SeekToStart(), Status::OK);

    auto ffmpegDigest = ReadAllSamples(ffmpegPlugin_, ffmpegMediaInfo_, ffmpegSelectedTracks, ReadMode::COPY);
    auto mpeg4Digest = ReadAllSamples(mpeg4Plugin_, mpeg4MediaInfo_, mpeg4SelectedTracks, ReadMode::COPY);
    CompareReadDigest(mpeg4Digest, ffmpegDigest);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_001
 * @tc.desc: Compare MPEG4 and FFmpeg demuxer async read results on h264 mp4 under weak network
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_MP4_CompareWithFfmpeg_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CompareWeakNetworkReadResult(TEST_MP4_PATH_H264_B, 2560656, 3));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_ThreadRetryRecover_001
 * @tc.desc: Verify async read thread keeps running when ReadSampleData returns ERROR_AGAIN
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_ThreadRetryRecover_001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CompareWeakNetworkReadResult(TEST_MP4_PATH_H264_B, 2560656, WEAK_NETWORK_THREAD_RETRY_FAIL_COUNT));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_TimeoutThenRecover_001
 * @tc.desc: Async read returns timeout under weak network and the next read can recover
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ReadSample_WeakNetwork_TimeoutThenRecover_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, WEAK_NETWORK_FIRST_READ_FAIL_OFFSET,
        WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT, WEAK_NETWORK_FAIL_DELAY_MS);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());

    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);

    auto trackId = selectedTrackIds.front();
    EXPECT_EQ(mpeg4Plugin_->ReadSample(trackId, buffer.mediaAVBuffer, WEAK_NETWORK_SHORT_TIMEOUT_MS),
        Status::ERROR_WAIT_TIMEOUT);

    auto ret = ReadSampleWithRetry(mpeg4Plugin_, trackId, buffer.mediaAVBuffer, ReadMode::COPY);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_FALSE(IsEos(buffer.mediaAVBuffer));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetNextSampleSize_WeakNetwork_TimeoutThenRecover_001
 * @tc.desc: Return timeout while the cache is empty and read-loop status remains OK, then recover
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_GetNextSampleSize_WeakNetwork_TimeoutThenRecover_001, TestSize.Level1)
{
    InitDeferredWeakNetworkMpeg4Plugin(TEST_MP4_PATH_H264_B, WEAK_NETWORK_FIRST_READ_FAIL_OFFSET,
        WEAK_NETWORK_TIMEOUT_RECOVER_FAIL_COUNT, WEAK_NETWORK_FAIL_DELAY_MS);
    auto selectedTrackIds = SelectSupportedTracks(mpeg4Plugin_, mpeg4MediaInfo_);
    ASSERT_FALSE(selectedTrackIds.empty());

    int32_t sampleSize = 0;
    uint32_t trackId = selectedTrackIds.front();
    auto plugin = std::static_pointer_cast<MPEG4DemuxerPlugin>(mpeg4Plugin_);
    ASSERT_NE(plugin, nullptr);
    EXPECT_EQ(plugin->readLoopStatus_.load(std::memory_order_acquire), Status::OK);
    EXPECT_EQ(mpeg4Plugin_->GetNextSampleSize(trackId, sampleSize, WEAK_NETWORK_SHORT_TIMEOUT_MS),
        Status::ERROR_WAIT_TIMEOUT);
    EXPECT_EQ(plugin->readLoopStatus_.load(std::memory_order_acquire), Status::OK);
    EXPECT_EQ(GetNextSampleSizeWithRetry(mpeg4Plugin_, trackId, sampleSize), Status::OK);
    EXPECT_GT(sampleSize, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected("/data/test/media/h264_double_video_audio.mp4",
        {602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0002, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CheckReadResultWithExpected(TEST_MP4_PATH_H264_B, {103, 174}, {5, 174}, {0, 25, 50, 75, 100}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0004
 * @tc.desc: Explicitly verify hevc mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AUX, {16}, {1}, {0}));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0005
 * @tc.desc: Explicitly verify multi-track hevc mp4 frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_MP4_ExplicitFrames_0005, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    std::vector<uint32_t> numbers(430);
    std::iota(numbers.begin(), numbers.end(), 0);
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AVC_AAC, {430, 601}, {430, 3}, numbers));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0001
 * @tc.desc: Explicitly verify mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected("/data/test/media/h264_double_video_audio.mp4",
        {602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500}, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0002
 * @tc.desc: Explicitly verify mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0002, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CheckReadResultWithExpected(TEST_MP4_PATH_H264_B, {103, 174}, {5, 174}, {0, 25, 50, 75, 100}, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0004
 * @tc.desc: Explicitly verify hevc mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0004, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    ASSERT_NO_FATAL_FAILURE(CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AUX, {16}, {1}, {0}, true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0005
 * @tc.desc: Explicitly verify multi-track hevc mp4 uri frame and key-frame statistics
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSample_URI_MP4_ExplicitFrames_0005, TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    std::vector<uint32_t> numbers(430);
    std::iota(numbers.begin(), numbers.end(), 0);
    ASSERT_NO_FATAL_FAILURE(
        CheckReadResultWithExpected(TEST_MP4_PATH_HEVC_AVC_AAC, {430, 601}, {430, 3}, numbers, true));
}


/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_ExplicitFrames_0001
 * @tc.desc: Explicitly verify remaining frame counts after seek on mp4
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(CheckSeekReadResultWithExpected(TEST_MP4_PATH_H264_B, GetExplicitSeekExpectedResults()));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekTo_URI_ExplicitFrames_0001
 * @tc.desc: Explicitly verify remaining frame counts after seek on mp4 uri source
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekTo_URI_ExplicitFrames_0001, TestSize.Level1)
{
    ASSERT_NO_FATAL_FAILURE(
        CheckSeekReadResultWithExpected(TEST_MP4_PATH_H264_B, GetExplicitSeekExpectedResults(), true));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_IsRefParserSupported_0001
 * @tc.desc: Resource types supported by RefParser
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_IsRefParserSupported_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    bool isSupport = mpeg4Plugin_->IsRefParserSupported();
    EXPECT_EQ(isSupport, true);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_IsRefParserSupported_0002
 * @tc.desc: Resource types supported by RefParser
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_IsRefParserSupported_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS);
    bool isSupport = mpeg4Plugin_->IsRefParserSupported();
    EXPECT_EQ(isSupport, false);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefUpdatePos_0001
 * @tc.desc: ParserRefUpdatePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefUpdatePos_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    Status ret = mpeg4Plugin_->ParserRefUpdatePos(0, true);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefUpdatePos_0002
 * @tc.desc: ParserRefUpdatePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefUpdatePos_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_MULTIPLE_VIDEO_TRACKS);
    Status ret = mpeg4Plugin_->ParserRefUpdatePos(0, true);
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefUpdatePos_0003
 * @tc.desc: ParserRefUpdatePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefUpdatePos_0003, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(MPEG4_PLUGIN_NAME);
    auto mpeg4Plugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(mpeg4Plugin);
    Status ret = mpeg4Plugin_->ParserRefUpdatePos(0, true);
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefInfo_0001
 * @tc.desc: ParserRefInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ParserRefInfo_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    Status ret = mpeg4Plugin_->ParserRefInfo();
    EXPECT_TRUE(ret == Status::ERROR_UNKNOWN || ret == Status::ERROR_AGAIN);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ParserRefInfoInvalidDataStopsAndKeepsCursor_001
 * @tc.desc: Stop retrying invalid reference data without rewinding the playback sample cursor
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ParserRefInfoInvalidDataStopsAndKeepsCursor_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    mpeg4Plugin_ = plugin;
    ASSERT_NE(plugin, nullptr);
    InitPlugin(TEST_MP4_PATH_H264_B, mpeg4Plugin_, mpeg4Fd_, mpeg4MediaInfo_);
    ASSERT_EQ(plugin->EnsureReferenceParserPrepared(), Status::OK);
    ASSERT_GT(plugin->syncSamplesIndex_.size(), 1U);

    auto track = plugin->trackOfRefParser_;
    ASSERT_NE(track, nullptr);
    const uint32_t gopId = 1;
    const uint32_t playbackSampleIndex = plugin->syncSamplesIndex_[gopId] + 2;
    ASSERT_LT(playbackSampleIndex, track->sampleHelper->GetSampleCount());
    ASSERT_EQ(plugin->SelectTrack(static_cast<uint32_t>(plugin->refParserTrackIdx_)), Status::OK);
    AVBufferWrapper buffer(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_NE(buffer.mediaAVBuffer, nullptr);
    for (uint32_t sampleIndex = 0; sampleIndex < playbackSampleIndex; ++sampleIndex) {
        ASSERT_EQ(ReadSampleWithRetry(plugin, static_cast<uint32_t>(plugin->refParserTrackIdx_),
            buffer.mediaAVBuffer, ReadMode::COPY), Status::OK) << "sampleIndex=" << sampleIndex;
    }
    ASSERT_EQ(track->currentSampleIndex, playbackSampleIndex);

    auto parserManager = std::make_shared<ReferenceParserManager>();
    auto parser = std::make_unique<InvalidDataReferenceParser>();
    ASSERT_NE(parserManager, nullptr);
    ASSERT_NE(parser, nullptr);
    auto parserPtr = parser.get();
    parserManager->referenceParser_ = parserPtr;
    plugin->referenceParser_ = parserManager;
    plugin->isInit_ = true;
    plugin->refParserCurGopId_ = static_cast<int32_t>(gopId);
    plugin->refParserStartTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    plugin->refParserState_ = true;

    EXPECT_EQ(plugin->ParserRefInfo(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(parserPtr->parserNalUnitsCalls, 1U);
    EXPECT_EQ(track->currentSampleIndex, playbackSampleIndex);
    EXPECT_EQ(plugin->ParserRefInfo(), Status::ERROR_UNKNOWN);
    EXPECT_EQ(parserPtr->parserNalUnitsCalls, 1U);
    EXPECT_EQ(track->currentSampleIndex, playbackSampleIndex);
    parserManager->referenceParser_ = nullptr;
    plugin->referenceParser_.reset();
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetFrameLayerInfo_0001
 * @tc.desc: GetFrameLayerInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetFrameLayerInfo_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    std::shared_ptr<AVBuffer> videoSample = AVBuffer::CreateAVBuffer();
    videoSample->dts_ = 0;
    FrameLayerInfo frameLayerInfo;
    Status ret = mpeg4Plugin_->GetFrameLayerInfo(videoSample, frameLayerInfo);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetFrameLayerInfo_0002
 * @tc.desc: GetFrameLayerInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetFrameLayerInfo_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    FrameLayerInfo frameLayerInfo;
    Status ret = mpeg4Plugin_->GetFrameLayerInfo(0, frameLayerInfo);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetGopLayerInfo_0001
 * @tc.desc: GetGopLayerInfo
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetGopLayerInfo_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    GopLayerInfo gopLayerInfo;
    Status ret = mpeg4Plugin_->GetGopLayerInfo(0, gopLayerInfo);
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetIFramePos_0001
 * @tc.desc: GetIFramePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetIFramePos_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    std::vector<uint32_t> IFramePos;
    mpeg4Plugin_->GetIFramePos(IFramePos);
    EXPECT_GE(IFramePos.size(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetIFramePos_0002
 * @tc.desc: GetIFramePos
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetIFramePos_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(MPEG4_PLUGIN_NAME);
    auto mpeg4Plugin = std::reinterpret_pointer_cast<FFmpegDemuxerPlugin>(basePlugin);
    mpeg4Plugin_ = std::static_pointer_cast<BaseDemuxerPlugin>(mpeg4Plugin);
    std::vector<uint32_t> IFramePos;
    mpeg4Plugin_->GetIFramePos(IFramePos);
    EXPECT_EQ(IFramePos.size(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_Dts2FrameId_0001
 * @tc.desc: Dts2FrameId
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_Dts2FrameId_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    int64_t dts = 0;
    uint32_t frameId = -1;
    mpeg4Plugin_->Dts2FrameId(dts, frameId);
    EXPECT_EQ(frameId, 2);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_Dts2FrameId_0002
 * @tc.desc: Dts2FrameId
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_Dts2FrameId_0002, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    int64_t dts = -1;
    uint32_t frameId = -1;
    Status ret = mpeg4Plugin_->Dts2FrameId(dts, frameId);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_FrameId2SeekMs_0001
 * @tc.desc: FrameId2SeekMs
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_FrameId2SeekMs_0001, TestSize.Level1)
{
    ResetPlugin(mpeg4Plugin_, mpeg4Fd_);
    InitMpeg4Plugin(TEST_MP4_PATH_H264_B);
    uint32_t frameId = 0;
    int64_t seekMs = -1;
    mpeg4Plugin_->FrameId2SeekMs(frameId, seekMs);
    EXPECT_EQ(seekMs, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetIndexByRelativePresentationTimeUs_0001
 * @tc.desc: Oversized relative PTS and absolute PTS addition overflow return ERROR_INVALID_DATA
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetIndexByRelativePresentationTimeUs_0001,
    TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(plugin, nullptr);
    plugin->firstTrack_ = BuildWideCttsOffsetTrack();
    ASSERT_NE(plugin->firstTrack_, nullptr);

    uint32_t index = UINT32_MAX;
    EXPECT_EQ(plugin->GetIndexByRelativePresentationTimeUs(0, 0, index), Status::OK);
    EXPECT_EQ(index, 0U);
    EXPECT_EQ(plugin->absolutePTSOfIndexZero_, WIDE_CTTS_EXPECTED_MIN_PTS_US);

    index = UINT32_MAX;
    EXPECT_EQ(plugin->GetIndexByRelativePresentationTimeUs(0,
        static_cast<uint64_t>(INT64_MAX) + 1U, index), Status::ERROR_INVALID_DATA);
    EXPECT_EQ(index, UINT32_MAX);

    index = UINT32_MAX;
    EXPECT_EQ(plugin->GetIndexByRelativePresentationTimeUs(0,
        static_cast<uint64_t>(INT64_MAX), index), Status::ERROR_INVALID_DATA);
    EXPECT_EQ(index, UINT32_MAX);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CttsOffsetNormalization_0001
 * @tc.desc: CTTS offset normalization widens to int64, so delta - minDelta above INT32_MAX does not wrap
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CttsOffsetNormalization_0001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(plugin, nullptr);
    plugin->firstTrack_ = BuildWideCttsOffsetTrack();
    ASSERT_NE(plugin->firstTrack_, nullptr);

    EXPECT_EQ(plugin->GetPresentationTimeUsByMpeg4BoxParser(
        MPEG4DemuxerPlugin::GET_FIRST_SAMPLE_PTS, 0, 0, 0), Status::OK);
    // Before the widening fix, INT32_MAX - (-1) wrapped to INT32_MIN and the minimum PTS became negative.
    EXPECT_EQ(plugin->absolutePTSOfIndexZero_, WIDE_CTTS_EXPECTED_MIN_PTS_US);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetIndexByRelativePresentationTimeUsInvalidBasePts_0001
 * @tc.desc: Reject the invalid absolute PTS base state instead of using the INT64_MAX sentinel
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetIndexByRelativePresentationTimeUsInvalidBasePts_0001,
    TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(plugin, nullptr);
    plugin->firstTrack_ = BuildZeroCountCttsTrack();
    ASSERT_NE(plugin->firstTrack_, nullptr);

    uint32_t index = UINT32_MAX;
    EXPECT_EQ(plugin->GetIndexByRelativePresentationTimeUs(0, 0, index), Status::ERROR_INVALID_DATA);
    // No sample went through the stts/ctts conversion, so the base PTS keeps its sentinel value.
    EXPECT_EQ(plugin->absolutePTSOfIndexZero_, INT64_MAX);
    EXPECT_EQ(index, UINT32_MAX);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_GetRelativePresentationTimeUsByIndexEmptyHeap_0001
 * @tc.desc: Reject the empty index-to-relative-pts heap instead of dereferencing it
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_GetRelativePresentationTimeUsByIndexEmptyHeap_0001,
    TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>(MPEG4_PLUGIN_NAME);
    ASSERT_NE(plugin, nullptr);
    plugin->firstTrack_ = BuildZeroCountCttsTrack();
    ASSERT_NE(plugin->firstTrack_, nullptr);

    uint64_t relativePTS = 0;
    EXPECT_EQ(plugin->GetRelativePresentationTimeUsByIndex(0, 0, relativePTS), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DemuxerDataReader_ReadFullyShortRead_0001
 * @tc.desc: Verify DemuxerDataReader continues from the previous short read offset
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_DemuxerDataReader_ReadFullyShortRead_0001,
    TestSize.Level1)
{
    constexpr size_t shortReadSize = 10 * 1024;
    constexpr size_t requestSize = 20 * 1024;
    constexpr int64_t baseOffset = 37;
    std::vector<uint8_t> source(baseOffset + requestSize);
    for (size_t i = 0; i < source.size(); ++i) {
        source[i] = static_cast<uint8_t>((i % 251) + 1);
    }

    auto dataSource = std::make_shared<ShortReadDataSource>(source, shortReadSize);
    DemuxerDataReader reader;
    ASSERT_EQ(reader.SetDataReader(dataSource), Status::OK);
    std::vector<uint8_t> output(requestSize, 0);
    ASSERT_EQ(reader.ReadFully(baseOffset, output.data(), output.size()), Status::OK);

    std::vector<uint8_t> expected(source.begin() + baseOffset, source.begin() + baseOffset + requestSize);
    EXPECT_EQ(output, expected);
    ASSERT_GE(dataSource->readOffsets.size(), 2U);
    EXPECT_EQ(dataSource->readOffsets[0], baseOffset);
    EXPECT_EQ(dataSource->readOffsets[1], baseOffset + static_cast<int64_t>(shortReadSize));
    EXPECT_EQ(dataSource->readExpectedLens[0], requestSize);
    EXPECT_EQ(dataSource->readExpectedLens[1], requestSize - shortReadSize);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_DemuxerDataReader_ReadUintDataEmpty_0001
 * @tc.desc: Verify zero-length ReadUintData succeeds without a buffer or data source access.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_DemuxerDataReader_ReadUintDataEmpty_0001,
    TestSize.Level1)
{
    DemuxerDataReader reader;
    EXPECT_EQ(reader.ReadUintData(0, nullptr, 0), Status::OK);
    EXPECT_EQ(reader.ReadUintData(-1, nullptr, 0), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ReadSampleDataToBuffer_ShortRead_0001
 * @tc.desc: Verify ReadSampleDataToBuffer continues from the previous short read offset
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ReadSampleDataToBuffer_ShortRead_0001, TestSize.Level1)
{
    constexpr size_t shortReadSize = 10 * 1024;
    constexpr size_t requestSize = 20 * 1024;
    constexpr int64_t baseOffset = 53;
    std::vector<uint8_t> source(baseOffset + requestSize);
    for (size_t i = 0; i < source.size(); ++i) {
        source[i] = static_cast<uint8_t>((i % 251) + 1);
    }

    auto dataSource = std::make_shared<ShortReadDataSource>(source, shortReadSize);
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin("avdemux_mpeg4");
    plugin.dataSource_ = dataSource;
    auto sample = std::make_shared<Plugins::MPEG4::Sample>();
    ASSERT_NE(sample, nullptr);
    ASSERT_EQ(plugin.ReadSampleDataToBuffer(sample, baseOffset, requestSize), Status::OK);
    ASSERT_NE(sample->data, nullptr);

    std::vector<uint8_t> actual(sample->data.get(), sample->data.get() + requestSize);
    std::vector<uint8_t> expected(source.begin() + baseOffset, source.begin() + baseOffset + requestSize);
    EXPECT_EQ(actual, expected);
    ASSERT_GE(dataSource->readOffsets.size(), 2U);
    EXPECT_EQ(dataSource->readOffsets[0], baseOffset);
    EXPECT_EQ(dataSource->readOffsets[1], baseOffset + static_cast<int64_t>(shortReadSize));
    EXPECT_EQ(dataSource->readExpectedLens[0], requestSize);
    EXPECT_EQ(dataSource->readExpectedLens[1], requestSize - shortReadSize);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TrunRejectsTruncatedOptionalFields_001
 * @tc.desc: Verify trun optional uint32 fields are rejected before a short payload can be read out of bounds
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TrunRejectsTruncatedOptionalFields_001,
    TestSize.Level1)
{
    const std::vector<std::pair<uint32_t, size_t>> invalidCases = {
        {Plugins::MPEG4::TRUN_DATA_OFFSET, sizeof(uint32_t) - 1},
        {Plugins::MPEG4::TRUN_FIRST_SAMPLE_FLAGS, sizeof(uint32_t) - 1},
        {Plugins::MPEG4::TRUN_DATA_OFFSET | Plugins::MPEG4::TRUN_FIRST_SAMPLE_FLAGS,
            sizeof(uint32_t) * 2 - 1},
    };

    for (const auto& [flags, payloadSize] : invalidCases) {
        auto data = BuildTrunData(flags, 0, payloadSize);
        Plugins::MPEG4::MPEG4SampleHelper helper;
        helper.dataReader_ = CreateReaderForBytes(data);
        helper.hasTfhd_ = true;
        helper.tfhd_.defaultSampleDuration = 1;
        helper.tfhd_.defaultSampleSize = 1;
        helper.tfhd_.flags = 1;
        int64_t moofDts = 0;
        int64_t duration = 0;

        EXPECT_EQ(helper.SetTrackFragmentRunParams(0, static_cast<int64_t>(data.size()), 0, moofDts, duration),
            Status::ERROR_INVALID_DATA) << "flags=" << flags << ", payloadSize=" << payloadSize;
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TrunAcceptsCompleteOptionalFields_001
 * @tc.desc: Verify valid trun optional uint32 fields remain accepted after bounds hardening
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TrunAcceptsCompleteOptionalFields_001,
    TestSize.Level1)
{
    constexpr uint32_t flags = Plugins::MPEG4::TRUN_DATA_OFFSET |
        Plugins::MPEG4::TRUN_FIRST_SAMPLE_FLAGS;
    auto data = BuildTrunData(flags, 0, sizeof(uint32_t) * 2);
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);
    helper.hasTfhd_ = true;
    helper.tfhd_.defaultSampleDuration = 1;
    helper.tfhd_.defaultSampleSize = 1;
    helper.tfhd_.flags = 1;
    int64_t moofDts = 0;
    int64_t duration = 0;

    EXPECT_EQ(helper.SetTrackFragmentRunParams(0, static_cast<int64_t>(data.size()), 0, moofDts, duration),
        Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TrunRejectsInvalidPayloadSize_001
 * @tc.desc: Verify negative or header-only trun data size is rejected by the signed-domain payload size guard
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TrunRejectsInvalidPayloadSize_001, TestSize.Level1)
{
    constexpr size_t expectedPayloadSize = sizeof(uint32_t);
    auto data = BuildTrunData(0, 1, expectedPayloadSize);
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);
    uint32_t trunFlag = 0;
    uint32_t numSamples = 0;
    std::unique_ptr<uint8_t[]> trunPayload;

    EXPECT_EQ(helper.ReadTrunRunData(0, -1, trunFlag, numSamples, trunPayload),
        Status::ERROR_INVALID_BUFFER_SIZE);
    EXPECT_EQ(trunPayload.get(), nullptr);
    EXPECT_EQ(helper.ReadTrunRunData(0, static_cast<int64_t>(sizeof(uint64_t)), trunFlag, numSamples, trunPayload),
        Status::ERROR_INVALID_BUFFER_SIZE);
    EXPECT_EQ(trunPayload.get(), nullptr);
    // Upper bound: 8-byte header + 8 fixed bytes + 16 bytes for each of MAX_TRUN_SAMPLES samples.
    constexpr int64_t trunMaxDataSize = 8 + 8 + 1000000LL * 16;
    EXPECT_EQ(helper.ReadTrunRunData(0, trunMaxDataSize + 1, trunFlag, numSamples, trunPayload),
        Status::ERROR_INVALID_BUFFER_SIZE);
    EXPECT_EQ(trunPayload.get(), nullptr);
    EXPECT_EQ(helper.ReadTrunRunData(0, std::numeric_limits<int64_t>::max(), trunFlag, numSamples, trunPayload),
        Status::ERROR_INVALID_BUFFER_SIZE);
    EXPECT_EQ(trunPayload.get(), nullptr);

    EXPECT_EQ(helper.ReadTrunRunData(0, static_cast<int64_t>(data.size()), trunFlag, numSamples, trunPayload),
        Status::OK);
    ASSERT_NE(trunPayload.get(), nullptr);
    EXPECT_EQ(numSamples, 1U);
    EXPECT_EQ(trunPayload[expectedPayloadSize - 1], 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TimeToSample_ClipNegativeDelta_001
 * @tc.desc: Verify oversized stts delta is clipped like FFmpeg's MOV demuxer correction path
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TimeToSample_ClipNegativeDelta_001, TestSize.Level1)
{
    auto data = BuildSttsData({{1U, 1023U}, {1U, 0xFFFFFC9EU}, {1U, 1023U}});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    ASSERT_EQ(helper.SetTimeToSampleParams(0, data.size()), Status::OK);
    EXPECT_EQ(helper.GetSttsEntryCount(), 3U);
    EXPECT_EQ(helper.GetSampleDeltaOfSttsEntry(1), 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TimeToSample_ZeroCountEntrySkipped_001
 * @tc.desc: Aligned with FFmpeg mov_read_stts(): a zero-count stts entry contributes no samples
 *           and is skipped instead of failing the whole moov parse.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TimeToSample_ZeroCountEntrySkipped_001, TestSize.Level1)
{
    auto data = BuildSttsData({{0U, 1U}});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    ASSERT_EQ(helper.SetTimeToSampleParams(0, data.size()), Status::OK);
    EXPECT_EQ(helper.GetSttsEntryCount(), 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TimeToSample_ZeroCountEntryThenValidEntry_001
 * @tc.desc: A zero-count stts entry followed by a valid entry is skipped without shifting the
 *           valid entry's delta mapping (FFmpeg-aligned re-indexing).
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TimeToSample_ZeroCountEntryThenValidEntry_001,
    TestSize.Level1)
{
    auto data = BuildSttsData({{0U, 1U}, {3U, 2U}});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    ASSERT_EQ(helper.SetTimeToSampleParams(0, data.size()), Status::OK);
    EXPECT_EQ(helper.GetSttsEntryCount(), 1U);
    EXPECT_EQ(helper.GetSampleCountOfSttsEntry(0), 3U);
    EXPECT_EQ(helper.GetSampleDeltaOfSttsEntry(0), 2U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleSize_Stz2HalfByteSingleEntry_001
 * @tc.desc: Verify stz2 half-byte parsing does not append an extra low-nibble sample for an odd count
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleSize_Stz2HalfByteSingleEntry_001, TestSize.Level1)
{
    auto data = BuildStz2Data(4, 1, {0xA0});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    ASSERT_EQ(helper.SetSampleSizeParams(static_cast<uint32_t>(Plugins::MPEG4::FourccType("stz2")), 0, data.size()),
        Status::OK);
    ASSERT_EQ(helper.sampleSizes_.size(), 1U);
    EXPECT_EQ(helper.sampleSizes_[0], 10);
    EXPECT_EQ(helper.sampleSizeCount_, 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleSize_DefaultStszKeepsSampleCount_001
 * @tc.desc: Verify stsz default sample size still records the declared sample count
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleSize_DefaultStszKeepsSampleCount_001, TestSize.Level1)
{
    auto data = BuildStszData(512, 3);
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    ASSERT_EQ(helper.SetSampleSizeParams(static_cast<uint32_t>(Plugins::MPEG4::FourccType("stsz")), 0, data.size()),
        Status::OK);
    ASSERT_EQ(helper.sampleSizes_.size(), 1U);
    EXPECT_EQ(helper.sampleSizes_[0], 512);
    EXPECT_TRUE(helper.hasDefaultStszSize_);
    EXPECT_EQ(helper.sampleSizeCount_, 3U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleSize_RejectOversizedDefaultStsz_001
 * @tc.desc: Verify default stsz sample size must fit in signed sample index entry size
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleSize_RejectOversizedDefaultStsz_001, TestSize.Level1)
{
    auto data = BuildStszData(0x80000000U, 1);
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    EXPECT_EQ(helper.SetSampleSizeParams(static_cast<uint32_t>(Plugins::MPEG4::FourccType("stsz")), 0, data.size()),
        Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleSize_RejectOversizedExplicitStsz_001
 * @tc.desc: Verify explicit stsz sample size must fit in signed sample index entry size
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleSize_RejectOversizedExplicitStsz_001, TestSize.Level1)
{
    auto data = BuildStszData(0, 1, {0x80000000U});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    EXPECT_EQ(helper.SetSampleSizeParams(static_cast<uint32_t>(Plugins::MPEG4::FourccType("stsz")), 0, data.size()),
        Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CompositionTime_RejectZeroSampleCount_001
 * @tc.desc: Verify ctts entries with zero samples are rejected before expansion
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CompositionTime_RejectZeroSampleCount_001, TestSize.Level1)
{
    auto data = BuildCttsData({{0U, 0U}});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    EXPECT_EQ(helper.SetCompositionTimeToSampleParams(0, data.size()), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CompositionTime_TrimExcessSamples_001
 * @tc.desc: Verify trailing ctts samples beyond stts sample count are ignored like FFmpeg mov demuxer
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CompositionTime_TrimExcessSamples_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper.sttsEntry_ = {{3U, 10U}};
    helper.cttsEntry_ = {{2U, 5}, {2U, 9}};
    helper.stscEntry_ = {{0U, 3U, 1U}};
    helper.sampleSizes_ = {1, 1, 1};
    helper.sampleSizeCount_ = 3;
    helper.chunkOffsets_ = {100};

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(helper.GetSampleIndexView(2, view), Status::OK);
    EXPECT_EQ(view.pts - view.dts, 9);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleToChunk_AcceptZeroSampleCount_001
 * @tc.desc: Verify stsc zero sample count is retained for FFmpeg-compatible repair
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleToChunk_AcceptZeroSampleCount_001, TestSize.Level1)
{
    auto data = BuildStscData({{1U, 0U, 1U}});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);

    EXPECT_EQ(helper.SetSampleToChunkParams(0, data.size()), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleToChunk_RepairInvalidMiddleEntry_001
 * @tc.desc: Verify invalid intermediate stsc entry is repaired like FFmpeg mov demuxer
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleToChunk_RepairInvalidMiddleEntry_001,
    TestSize.Level1)
{
    auto data = BuildStscData({{1U, 1U, 1U}, {1U, UINT32_MAX, 0U}, {3U, 1U, 1U}});
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.dataReader_ = CreateReaderForBytes(data);
    ASSERT_EQ(helper.SetSampleToChunkParams(0, data.size()), Status::OK);
    helper.sttsEntry_ = {{3U, 1U}};
    helper.sampleSizes_ = {1, 1, 1};
    helper.sampleSizeCount_ = 3;
    helper.chunkOffsets_ = {100, 101, 102};

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(helper.GetSampleIndexView(1, view), Status::OK);
    EXPECT_EQ(view.pos, 101);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleToChunk_TrimUnmappedSamples_001
 * @tc.desc: Verify samples beyond repaired stsc chunk coverage are not indexed
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleToChunk_TrimUnmappedSamples_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.sttsEntry_ = {{4U, 1U}};
    helper.stscEntry_ = {{0U, 1U, 1U}, {0U, 1U, 0U}, {2U, 1U, 1U}};
    helper.sampleSizes_ = {1, 1, 1, 1};
    helper.sampleSizeCount_ = 4;
    helper.chunkOffsets_ = {100, 101, 102};

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    EXPECT_EQ(helper.GetSampleCount(), 3U);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    EXPECT_EQ(helper.GetSampleIndexView(2, view), Status::OK);
    EXPECT_EQ(helper.GetSampleIndexView(3, view), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_Sidx_RejectUnsupportedVersion_001
 * @tc.desc: Verify sidx version must be version 0 or 1 even when the remaining bytes look parseable
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_Sidx_RejectUnsupportedVersion_001, TestSize.Level1)
{
    auto data = BuildSidxV0Data(2, 1);
    WriteU16(data, 14, 1);
    Plugins::MPEG4::MPEG4AtomParser parser;
    parser.dataReader_ = CreateReaderForBytes(data);
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
    atom.size = data.size() + 8;
    atom.dataSize = data.size();
    atom.dataOffset = 0;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    ctx.offset = 0;
    ctx.dataOffset = 0;

    EXPECT_EQ(parser.ParseSidx(atom, 0, &ctx), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_Sidx_RejectTooManyEntries_001
 * @tc.desc: Verify sidx reference count is capped before entry parsing
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_Sidx_RejectTooManyEntries_001, TestSize.Level1)
{
    auto data = BuildSidxV0Data(0, 8193);
    Plugins::MPEG4::MPEG4AtomParser parser;
    parser.dataReader_ = CreateReaderForBytes(data);
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
    atom.size = data.size() + 8;
    atom.dataSize = data.size();
    atom.dataOffset = 0;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    ctx.offset = 0;
    ctx.dataOffset = 0;

    EXPECT_EQ(parser.ParseSidx(atom, 0, &ctx), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SidxEntries_RejectBufferBoundary_001
 * @tc.desc: Verify sidx entry parsing checks the supplied buffer boundary before reading an entry
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SidxEntries_RejectBufferBoundary_001, TestSize.Level1)
{
    std::vector<uint8_t> header(16, 0);
    Plugins::MPEG4::MPEG4AtomParser parser;
    Plugins::MPEG4::MPEG4AtomParser::SidxParseContext sidxCtx;
    sidxCtx.entryCount = 1;

    EXPECT_EQ(parser.ParseSidxEntries(header.data(), 4, 15, sidxCtx, nullptr), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomHeader_RejectAtomExceedingSource_001
 * @tc.desc: Verify known root atom size cannot exceed the data source size
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomHeader_RejectAtomExceedingSource_001, TestSize.Level1)
{
    std::vector<uint8_t> data;
    AppendU32(data, 32);
    AppendU32(data, static_cast<uint32_t>(Plugins::MPEG4::FourccType("ftyp")));
    auto dataSource = CreateDataSourceForBytes(data);
    auto reader = std::make_shared<DemuxerDataReader>();
    ASSERT_EQ(reader->SetDataReader(dataSource), Status::OK);
    Plugins::MPEG4::MPEG4AtomParser parser;
    parser.dataReader_ = reader;
    parser.dataSource_ = dataSource;
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    std::string typeStr;
    bool isZeroSizeAtom = false;

    EXPECT_EQ(parser.ParseAtomHeader(atom, typeStr, 0, isZeroSizeAtom, &ctx), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomHeader_TrimTrailingUnknownAtom_001
 * @tc.desc: Verify trailing unknown root atom after moov and mdat is trimmed to remaining source bytes
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomHeader_TrimTrailingUnknownAtom_001, TestSize.Level1)
{
    std::vector<uint8_t> data;
    AppendU32(data, 32);
    AppendU32(data, static_cast<uint32_t>(Plugins::MPEG4::FourccType("zzzz")));
    data.resize(12, 0);
    auto dataSource = CreateDataSourceForBytes(data);
    auto reader = std::make_shared<DemuxerDataReader>();
    ASSERT_EQ(reader->SetDataReader(dataSource), Status::OK);
    Plugins::MPEG4::MPEG4AtomParser parser;
    parser.dataReader_ = reader;
    parser.dataSource_ = dataSource;
    parser.moovFound_ = true;
    parser.hasMdatBox_ = true;
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    std::string typeStr;
    bool isZeroSizeAtom = false;

    ASSERT_EQ(parser.ParseAtomHeader(atom, typeStr, 0, isZeroSizeAtom, &ctx), Status::OK);
    EXPECT_EQ(atom.size, 32U);
    EXPECT_EQ(atom.effectiveSize, data.size());
    EXPECT_FALSE(isZeroSizeAtom);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomHeader_RootMdatBoundaries_001
 * @tc.desc: Verify root mdat truncation, ordering, and declared/effective sizes
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomHeader_RootMdatBoundaries_001, TestSize.Level1)
{
    constexpr uint32_t mdatType = static_cast<uint32_t>(Plugins::MPEG4::FourccType("mdat"));
    std::vector<uint8_t> truncated;
    AppendU32(truncated, 32);
    AppendU32(truncated, mdatType);
    truncated.resize(12, 0);
    AtomParserTestContext test(truncated);
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    std::string typeStr;
    bool isZeroSizeAtom = false;

    ASSERT_EQ(test.parser.ParseAtomHeader(atom, typeStr, 0, isZeroSizeAtom, &ctx), Status::OK);
    EXPECT_EQ(atom.size, 32U);
    EXPECT_EQ(atom.effectiveSize, truncated.size());
    ASSERT_EQ(test.parser.MPEG4ParseAtom(0, &ctx), Status::OK);
    EXPECT_FALSE(test.parser.moovFound_);
    EXPECT_TRUE(test.parser.hasMdatBox_);
    EXPECT_EQ(ctx.offset, static_cast<int64_t>(truncated.size()));

    std::vector<uint8_t> emptyMdat;
    AppendU32(emptyMdat, 8);
    AppendU32(emptyMdat, mdatType);
    AtomParserTestContext emptyMdatTest(emptyMdat);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext emptyMdatCtx;
    EXPECT_EQ(emptyMdatTest.parser.MPEG4ParseAtom(0, &emptyMdatCtx), Status::OK);
    EXPECT_FALSE(emptyMdatTest.parser.hasMdatBox_);
    EXPECT_EQ(emptyMdatCtx.offset, static_cast<int64_t>(emptyMdat.size()));

    std::vector<uint8_t> ordered;
    AppendU32(ordered, 12);
    AppendU32(ordered, mdatType);
    ordered.resize(12, 0);
    AppendU32(ordered, 8);
    AppendU32(ordered, static_cast<uint32_t>(Plugins::MPEG4::FourccType("moov")));
    AtomParserTestContext orderedTest(ordered);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext orderedCtx;
    ASSERT_EQ(orderedTest.parser.MPEG4ParseAtom(0, &orderedCtx), Status::OK);
    ASSERT_EQ(orderedTest.parser.MPEG4ParseAtom(0, &orderedCtx), Status::OK);
    EXPECT_TRUE(orderedTest.parser.moovFound_);
    EXPECT_EQ(orderedCtx.offset, static_cast<int64_t>(ordered.size()));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomHeader_ExtendedAndZeroSizeBoundaries_001
 * @tc.desc: Verify extended headers, zero-size atoms, and incomplete headers
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_AtomHeader_ExtendedAndZeroSizeBoundaries_001, TestSize.Level1)
{
    constexpr uint32_t mdatType = static_cast<uint32_t>(Plugins::MPEG4::FourccType("mdat"));
    auto extended = BuildAtomHeader(32, mdatType, true);
    extended.resize(20, 0);
    AtomParserTestContext extendedTest(extended);
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    std::string typeStr;
    bool isZeroSizeAtom = false;
    EXPECT_EQ(extendedTest.parser.ParseAtomHeader(atom, typeStr, 0, isZeroSizeAtom, &ctx), Status::OK);
    EXPECT_EQ(atom.size, 48U);
    EXPECT_EQ(atom.effectiveSize, extended.size());

    std::vector<uint8_t> shortExtended;
    AppendU32(shortExtended, 1);
    AppendU32(shortExtended, mdatType);
    shortExtended.resize(12, 0);
    AtomParserTestContext shortTest(shortExtended);
    EXPECT_EQ(shortTest.parser.ParseAtomHeader(atom, typeStr, 0, isZeroSizeAtom, &ctx), Status::ERROR_INVALID_DATA);

    std::vector<uint8_t> zeroSize;
    AppendU32(zeroSize, 0);
    AppendU32(zeroSize, mdatType);
    zeroSize.resize(12, 0);
    AtomParserTestContext zeroTest(zeroSize);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext zeroCtx;
    EXPECT_EQ(zeroTest.parser.MPEG4ParseAtom(0, &zeroCtx), Status::OK);
    EXPECT_EQ(zeroCtx.offset, static_cast<int64_t>(zeroSize.size()));

    std::vector<uint8_t> nestedZero;
    AppendU32(nestedZero, 0);
    AppendU32(nestedZero, static_cast<uint32_t>(Plugins::MPEG4::FourccType("free")));
    AtomParserTestContext nestedZeroTest(nestedZero);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext nestedCtx;
    EXPECT_EQ(nestedZeroTest.parser.ParseAtomHeader(atom, typeStr, 1, isZeroSizeAtom, &nestedCtx), Status::OK);
    EXPECT_TRUE(isZeroSizeAtom);
    EXPECT_EQ(nestedCtx.offset, 4);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_FtypZeroCompatibleBrandsAccepted_001
 * @tc.desc: Verify a legal ftyp box with only major brand and no compatible brands is accepted
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_FtypZeroCompatibleBrandsAccepted_001, TestSize.Level1)
{
    constexpr uint32_t ftypAtomSize = 16;
    constexpr uint32_t ftypType = static_cast<uint32_t>(Plugins::MPEG4::FourccType("ftyp"));
    constexpr uint32_t mp41Brand = static_cast<uint32_t>(Plugins::MPEG4::FourccType("mp41"));
    std::vector<uint8_t> data;
    AppendU32(data, ftypAtomSize);
    AppendU32(data, ftypType);
    AppendU32(data, mp41Brand);
    AppendU32(data, 0); // minor version, no compatible brands
    AtomParserTestContext test(data);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;

    ASSERT_EQ(test.parser.MPEG4ParseAtom(0, &ctx), Status::OK);
    EXPECT_TRUE(test.parser.IsIsom());
    EXPECT_EQ(ctx.offset, static_cast<int64_t>(data.size()));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomHeader_RejectInvalidBoundaries_001
 * @tc.desc: Verify invalid offsets, overflow, nested truncation, and malformed declared sizes are rejected
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomHeader_RejectInvalidBoundaries_001, TestSize.Level1)
{
    constexpr uint32_t mdatType = static_cast<uint32_t>(Plugins::MPEG4::FourccType("mdat"));
    auto header = BuildAtomHeader(0, mdatType, false);

    AtomParserTestContext negativeTest(header, true);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext negativeCtx;
    negativeCtx.offset = -1;
    EXPECT_EQ(negativeTest.parser.MPEG4ParseAtom(0, &negativeCtx), Status::ERROR_INVALID_PARAMETER);

    AtomParserTestContext beyondTest(header);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext beyondCtx;
    beyondCtx.offset = static_cast<int64_t>(header.size()) + 1;
    EXPECT_EQ(beyondTest.parser.MPEG4ParseAtom(0, &beyondCtx), Status::ERROR_INVALID_DATA);

    AtomParserTestContext overflowTest(header, true);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext overflowCtx;
    overflowCtx.offset = std::numeric_limits<int64_t>::max() - 7;
    EXPECT_EQ(overflowTest.parser.MPEG4ParseAtom(0, &overflowCtx), Status::ERROR_INVALID_DATA);

    std::vector<uint8_t> nestedHeader;
    AppendU32(nestedHeader, 32);
    AppendU32(nestedHeader, mdatType);
    nestedHeader.resize(12, 0);
    AtomParserTestContext nestedTest(nestedHeader);
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom atom;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext nestedCtx;
    std::string typeStr;
    bool isZeroSizeAtom = false;
    EXPECT_EQ(nestedTest.parser.ParseAtomHeader(atom, typeStr, 1, isZeroSizeAtom, &nestedCtx),
        Status::ERROR_INVALID_DATA);

    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom overflowAtom;
    overflowAtom.type = static_cast<int32_t>(mdatType);
    overflowAtom.size = std::numeric_limits<uint64_t>::max();
    overflowAtom.effectiveSize = overflowAtom.size;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext atomCtx;
    atomCtx.offset = 1;
    EXPECT_EQ(nestedTest.parser.ValidateAtomSourceRange(overflowAtom, "mdat", 0, &atomCtx, 16),
        Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomPayloadLimit_AcceptUnknownAtomBoundary_001
 * @tc.desc: Verify unknown non-mdat atom payloads up to 64 MiB are skipped when source size is unknown
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomPayloadLimit_AcceptUnknownAtomBoundary_001,
    TestSize.Level1)
{
    constexpr uint64_t maxPayloadSize = 64ULL * 1024 * 1024;
    constexpr uint64_t normalHeaderSize = 8;
    for (uint64_t payloadSize : {maxPayloadSize - 1, maxPayloadSize}) {
        auto data = BuildAtomHeader(payloadSize, static_cast<uint32_t>(Plugins::MPEG4::FourccType("zzzz")), false);
        auto dataSource = CreateUnknownSizeDataSourceForBytes(data);
        auto reader = std::make_shared<DemuxerDataReader>();
        ASSERT_EQ(reader->SetDataReader(dataSource), Status::OK);
        Plugins::MPEG4::MPEG4AtomParser parser;
        parser.dataReader_ = reader;
        parser.dataSource_ = dataSource;
        Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;

        EXPECT_EQ(parser.MPEG4ParseAtom(0, &ctx), Status::OK);
        EXPECT_EQ(ctx.offset, static_cast<int64_t>(payloadSize + normalHeaderSize));
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomPayloadLimit_RejectUnknownAtomBeforeSkip_001
 * @tc.desc: Verify an oversized unknown atom is rejected before unknown atom skipping
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomPayloadLimit_RejectUnknownAtomBeforeSkip_001,
    TestSize.Level1)
{
    constexpr uint64_t maxPayloadSize = 64ULL * 1024 * 1024;
    auto data = BuildAtomHeader(maxPayloadSize + 1,
        static_cast<uint32_t>(Plugins::MPEG4::FourccType("zzzz")), false);
    auto dataSource = CreateUnknownSizeDataSourceForBytes(data);
    auto reader = std::make_shared<DemuxerDataReader>();
    ASSERT_EQ(reader->SetDataReader(dataSource), Status::OK);
    Plugins::MPEG4::MPEG4AtomParser parser;
    parser.dataReader_ = reader;
    parser.dataSource_ = dataSource;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;

    EXPECT_EQ(parser.MPEG4ParseAtom(0, &ctx), Status::ERROR_INVALID_DATA);
    EXPECT_EQ(ctx.offset, 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomPayloadLimit_ExtendedHeaderBoundary_001
 * @tc.desc: Verify the 64 MiB limit excludes the 16-byte extended atom header
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomPayloadLimit_ExtendedHeaderBoundary_001,
    TestSize.Level1)
{
    constexpr uint64_t maxPayloadSize = 64ULL * 1024 * 1024;
    constexpr uint64_t extendedHeaderSize = 16;
    for (uint64_t payloadSize : {maxPayloadSize, maxPayloadSize + 1}) {
        auto data = BuildAtomHeader(payloadSize, static_cast<uint32_t>(Plugins::MPEG4::FourccType("zzzz")), true);
        auto dataSource = CreateUnknownSizeDataSourceForBytes(data);
        auto reader = std::make_shared<DemuxerDataReader>();
        ASSERT_EQ(reader->SetDataReader(dataSource), Status::OK);
        Plugins::MPEG4::MPEG4AtomParser parser;
        parser.dataReader_ = reader;
        parser.dataSource_ = dataSource;
        Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;

        const Status expected = payloadSize == maxPayloadSize ? Status::OK : Status::ERROR_INVALID_DATA;
        EXPECT_EQ(parser.MPEG4ParseAtom(0, &ctx), expected);
        EXPECT_EQ(ctx.offset, expected == Status::OK ? static_cast<int64_t>(payloadSize + extendedHeaderSize) : 0);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AtomPayloadLimit_ExemptMdat_001
 * @tc.desc: Verify large mdat payloads keep the existing skip behavior
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AtomPayloadLimit_ExemptMdat_001, TestSize.Level1)
{
    constexpr uint64_t maxPayloadSize = 64ULL * 1024 * 1024;
    constexpr uint64_t normalHeaderSize = 8;
    auto data = BuildAtomHeader(maxPayloadSize + 1,
        static_cast<uint32_t>(Plugins::MPEG4::FourccType("mdat")), false);
    auto dataSource = CreateUnknownSizeDataSourceForBytes(data);
    auto reader = std::make_shared<DemuxerDataReader>();
    ASSERT_EQ(reader->SetDataReader(dataSource), Status::OK);
    Plugins::MPEG4::MPEG4AtomParser parser;
    parser.dataReader_ = reader;
    parser.dataSource_ = dataSource;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;

    EXPECT_EQ(parser.MPEG4ParseAtom(0, &ctx), Status::OK);
    EXPECT_TRUE(parser.hasMdatBox_);
    EXPECT_EQ(ctx.offset, static_cast<int64_t>(maxPayloadSize + 1 + normalHeaderSize));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ContainerAtom_RejectTruncatedExtendedChildHeader_001
 * @tc.desc: Verify container parsing rejects a child atom whose extended-size header crosses the container end
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest,
    Mpeg4DemuxerPlugin_ContainerAtom_RejectTruncatedExtendedChildHeader_001, TestSize.Level1)
{
    std::vector<uint8_t> data;
    AppendU32(data, 1);
    AppendU32(data, static_cast<uint32_t>(Plugins::MPEG4::FourccType("free")));
    data.resize(12, 0);
    Plugins::MPEG4::MPEG4AtomParser parser;
    parser.dataReader_ = CreateReaderForBytes(data);
    Plugins::MPEG4::MPEG4AtomParser::MPEG4Atom containerAtom;
    containerAtom.dataOffset = 0;
    containerAtom.dataSize = data.size();
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    ctx.dataOffset = 0;

    EXPECT_EQ(parser.ParseContainerAtom(containerAtom, 0, &ctx), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleIndexCompactLayout_001
 * @tc.desc: Verify compact sample index layout and packed size/discard boundaries
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleIndexCompactLayout_001, TestSize.Level1)
{
    using SampleIndexEntry = Plugins::MPEG4::MPEG4SampleHelper::SampleIndexEntry;
    static_assert(sizeof(SampleIndexEntry) == 24, "SampleIndexEntry layout changed");
    SampleIndexEntry entry;
    EXPECT_TRUE(entry.SetSize(SampleIndexEntry::sizeMask));
    EXPECT_EQ(entry.GetSize(), std::numeric_limits<int32_t>::max());
    entry.SetDiscard(true);
    EXPECT_TRUE(entry.IsDiscard());
    EXPECT_EQ(entry.GetSize(), std::numeric_limits<int32_t>::max());
    entry.SetDiscard(false);
    EXPECT_FALSE(entry.IsDiscard());
    EXPECT_FALSE(entry.SetSize(static_cast<uint32_t>(std::numeric_limits<int32_t>::max()) + 1U));
    EXPECT_EQ(entry.GetSize(), std::numeric_limits<int32_t>::max());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleIndexPtsOffset_001
 * @tc.desc: Verify PTS offset encoding boundaries and overflow detection
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleIndexPtsOffset_001, TestSize.Level1)
{
    using SampleIndexEntry = Plugins::MPEG4::MPEG4SampleHelper::SampleIndexEntry;
    SampleIndexEntry entry;
    entry.dts = 0;
    EXPECT_TRUE(entry.SetPts(static_cast<int64_t>(std::numeric_limits<uint32_t>::max())));
    int64_t pts = 0;
    EXPECT_TRUE(entry.GetPts(pts));
    EXPECT_EQ(pts, static_cast<int64_t>(std::numeric_limits<uint32_t>::max()));
    EXPECT_FALSE(entry.SetPts(-1));
    EXPECT_FALSE(entry.SetPts(static_cast<int64_t>(std::numeric_limits<uint32_t>::max()) + 1));
    entry.dts = std::numeric_limits<int64_t>::max();
    entry.ptsOffset = 1;
    EXPECT_FALSE(entry.GetPts(pts));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleIndexDurationOverride_001
 * @tc.desc: Verify derived and overridden sample durations
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleIndexDurationOverride_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.sampleIndexEntry_ = {
        MakeSampleIndexEntry(0, 0, 0, 10),
        MakeSampleIndexEntry(10, 100, 100, 10),
    };
    ASSERT_EQ(helper.AddDurationOverride(1, 40), Status::OK);
    int64_t duration = 0;
    EXPECT_EQ(helper.GetSampleDurationTicks(0, duration), Status::OK);
    EXPECT_EQ(duration, 100);
    EXPECT_EQ(helper.GetSampleDurationTicks(1, duration), Status::OK);
    EXPECT_EQ(duration, 40);
    EXPECT_EQ(helper.GetSampleDurationTicks(2, duration), Status::ERROR_INVALID_PARAMETER);
    helper.lastPTS_ = 100;
    EXPECT_FALSE(helper.IsAtOrBeyondLastSampleEnd(139));
    EXPECT_TRUE(helper.IsAtOrBeyondLastSampleEnd(140));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleIndexZeroDurationOverride_001
 * @tc.desc: Verify zero-duration samples remain valid after dense sample index construction
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleIndexZeroDurationOverride_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper.mimeType_ = Plugins::MimeType::VIDEO_AVC;
    helper.sttsEntry_ = {{1, 0}, {1, 40}, {1, 0}};
    helper.stscEntry_ = {{0, 3, 1}};
    helper.sampleSizes_ = {1, 1, 1};
    helper.sampleSizeCount_ = 3U;
    helper.chunkOffsets_ = {100};

    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    ASSERT_EQ(helper.sampleIndexEntry_.size(), 3U);
    ASSERT_EQ(helper.durationOverrides_.size(), 2U);
    EXPECT_EQ(helper.durationOverrides_[0].sampleIndex, 0U);
    EXPECT_EQ(helper.durationOverrides_[0].duration, 0);
    EXPECT_EQ(helper.durationOverrides_[1].sampleIndex, 2U);
    EXPECT_EQ(helper.durationOverrides_[1].duration, 0);

    const std::vector<int64_t> expectedDts = {0, 0, 40};
    const std::vector<int64_t> expectedDuration = {0, 40, 0};
    for (uint32_t i = 0; i < 3U; ++i) {
        Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
        ASSERT_EQ(helper.GetSampleIndexView(i, view), Status::OK);
        EXPECT_EQ(view.dts, expectedDts[i]);
        EXPECT_EQ(view.duration, expectedDuration[i]);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_CttsRleValidation_001
 * @tc.desc: Verify CTTS runs remain compressed while sample counts are validated
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_CttsRleValidation_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.cttsEntry_ = {{3, 0}, {2, -1}};
    EXPECT_EQ(helper.ValidateCttsEntries(5), Status::OK);
    EXPECT_EQ(helper.cttsEntry_.size(), 2U);
    EXPECT_EQ(helper.ValidateCttsEntries(4), Status::ERROR_INVALID_DATA);
    helper.cttsEntry_ = {{0, 0}};
    EXPECT_EQ(helper.ValidateCttsEntries(0), Status::ERROR_INVALID_DATA);
    helper.cttsEntry_ = {{std::numeric_limits<uint32_t>::max(), 0}};
    EXPECT_EQ(helper.ValidateCttsEntries(1), Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleIndexAccessAndLookup_001
 * @tc.desc: Verify compact index accessors, sparse sync flags, and lookup boundaries
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleIndexAccessAndLookup_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.sampleIndexEntry_ = {
        MakeSampleIndexEntry(SAMPLE_INDEX_TEST_FIRST_POS, SAMPLE_INDEX_TEST_FIRST_DTS,
            SAMPLE_INDEX_TEST_FIRST_PTS, SAMPLE_INDEX_TEST_FIRST_SIZE),
        MakeSampleIndexEntry(SAMPLE_INDEX_TEST_SECOND_POS, SAMPLE_INDEX_TEST_SECOND_DTS,
            SAMPLE_INDEX_TEST_SECOND_DTS, SAMPLE_INDEX_TEST_SECOND_SIZE, true),
        MakeSampleIndexEntry(SAMPLE_INDEX_TEST_THIRD_POS, SAMPLE_INDEX_TEST_THIRD_DTS,
            SAMPLE_INDEX_TEST_THIRD_PTS, SAMPLE_INDEX_TEST_THIRD_SIZE),
    };
    helper.allSamplesSync_ = false;
    helper.stssEntry_ = {{0}, {SAMPLE_INDEX_TEST_THIRD}};
    ASSERT_EQ(helper.AddDurationOverride(SAMPLE_INDEX_TEST_THIRD, SAMPLE_INDEX_TEST_DURATION_OVERRIDE), Status::OK);
    VerifySampleIndexProperties(helper);
    VerifySampleIndexAccessors(helper);
    VerifySampleIndexLookup(helper);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EmptyStssSyncFlag_001
 * @tc.desc: Verify empty stss marks only the first sample as sync, matching FFmpeg keyframe_absent.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EmptyStssSyncFlag_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.sampleIndexEntry_ = {
        MakeSampleIndexEntry(0, 0, 0, 1),
        MakeSampleIndexEntry(1, 10, 10, 1),
        MakeSampleIndexEntry(2, 20, 20, 1),
    };

    ASSERT_EQ(helper.SetSyncSampleParams(0, 0), Status::OK);
    EXPECT_TRUE(helper.emptyStss_);
    EXPECT_FALSE(helper.IsSyncSample(1));
    EXPECT_TRUE(helper.IsSyncSample(0));
    EXPECT_FALSE(helper.IsSyncSample(2));
    EXPECT_EQ(helper.GetSyncSamplesIndexFromStssEntry(), std::vector<uint32_t>({0}));

    Plugins::MPEG4::MPEG4SampleHelper zeroCountHelper;
    zeroCountHelper.dataReader_ = CreateReaderForBytes(std::vector<uint8_t>(8, 0));
    ASSERT_EQ(zeroCountHelper.SetSyncSampleParams(0, 8), Status::OK);
    EXPECT_TRUE(zeroCountHelper.emptyStss_);

    Plugins::MPEG4::MPEG4SampleHelper duplicateStssHelper;
    duplicateStssHelper.stssEntry_ = {{0}};
    EXPECT_EQ(duplicateStssHelper.SetSyncSampleParams(0, 0), Status::OK);
    EXPECT_TRUE(duplicateStssHelper.emptyStss_);
    std::vector<uint8_t> oneSyncSample;
    AppendU32(oneSyncSample, 0);
    AppendU32(oneSyncSample, 1);
    AppendU32(oneSyncSample, 1);
    duplicateStssHelper.dataReader_ = CreateReaderForBytes(oneSyncSample);
    EXPECT_EQ(duplicateStssHelper.SetSyncSampleParams(0, oneSyncSample.size()), Status::OK);
    EXPECT_FALSE(duplicateStssHelper.stssEntry_.empty());
    EXPECT_FALSE(duplicateStssHelper.IsSyncSample(1));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleIndexSparseSyncAndOverride_001
 * @tc.desc: Verify sparse sync conversion and duration override index maintenance after insertion
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleIndexSparseSyncAndOverride_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.sampleIndexEntry_ = {
        MakeSampleIndexEntry(0, 0, 0, 1),
        MakeSampleIndexEntry(1, 10, 10, 1),
        MakeSampleIndexEntry(2, 20, 20, 1),
        MakeSampleIndexEntry(3, 30, 30, 1),
    };
    helper.allSamplesSync_ = true;
    helper.UpdateSyncSamplesForInsert(0, true);
    EXPECT_TRUE(helper.allSamplesSync_);
    EXPECT_TRUE(helper.stssEntry_.empty());
    ASSERT_EQ(helper.AddDurationOverride(1, 7), Status::OK);
    ASSERT_EQ(helper.AddDurationOverride(3, 9), Status::OK);
    EXPECT_EQ(helper.AddDurationOverride(0, -1), Status::ERROR_INVALID_DATA);

    helper.ShiftDurationOverridesForInsert(2);
    ASSERT_EQ(helper.durationOverrides_.size(), 2U);
    EXPECT_EQ(helper.durationOverrides_[0].sampleIndex, 1U);
    EXPECT_EQ(helper.durationOverrides_[1].sampleIndex, 4U);

    helper.UpdateSyncSamplesForInsert(1, false);
    EXPECT_FALSE(helper.allSamplesSync_);
    ASSERT_EQ(helper.stssEntry_.size(), 3U);
    EXPECT_EQ(helper.stssEntry_[0].index, 0U);
    EXPECT_EQ(helper.stssEntry_[1].index, 2U);
    EXPECT_EQ(helper.stssEntry_[2].index, 3U);
    EXPECT_FALSE(helper.IsSyncSample(1));

    helper.sampleIndexEntry_.insert(helper.sampleIndexEntry_.begin() + 2, MakeSampleIndexEntry(2, 15, 15, 1));
    helper.UpdateSyncSamplesForInsert(2, true);
    ASSERT_EQ(helper.stssEntry_.size(), 4U);
    EXPECT_EQ(helper.stssEntry_[0].index, 0U);
    EXPECT_EQ(helper.stssEntry_[1].index, 2U);
    EXPECT_EQ(helper.stssEntry_[2].index, 3U);
    EXPECT_EQ(helper.stssEntry_[3].index, 4U);
    EXPECT_TRUE(helper.IsSyncSample(2));
    EXPECT_FALSE(helper.IsSyncSample(1));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SparseViewCacheAndCursor_001
 * @tc.desc: Verify repeated sparse access uses the fixed cache and preserves STSS cursor flags
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SparseViewCacheAndCursor_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    PrepareSparseSampleHelper(helper, 8);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(helper.GetSampleIndexView(5, view), Status::OK);
    EXPECT_EQ(view.pos, 150);
    EXPECT_TRUE((view.flag & Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME) == 0);
    EXPECT_EQ(helper.sparseFrontier_.outputIndex, 6U);
    EXPECT_EQ(helper.sparseViewCache_[1].sampleIndex, 5U);

    auto frontier = helper.sparseFrontier_;
    ASSERT_EQ(helper.GetSampleIndexView(5, view), Status::OK);
    EXPECT_EQ(helper.sparseFrontier_.outputIndex, frontier.outputIndex);
    EXPECT_EQ(helper.sparseFrontier_.processedRawSamples, frontier.processedRawSamples);
    ASSERT_EQ(helper.GetSampleIndexView(3, view), Status::OK);
    EXPECT_TRUE((view.flag & Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME) != 0);
    uint32_t sampleIndex = 0;
    EXPECT_EQ(helper.FindSampleIndexByPosition(155, sampleIndex), Status::OK);
    EXPECT_EQ(sampleIndex, 5U);
    EXPECT_EQ(helper.FindFirstSampleAfterPosition(155, sampleIndex), Status::OK);
    EXPECT_EQ(sampleIndex, 6U);
    EXPECT_EQ(helper.FindFirstSampleByDts(35, sampleIndex), Status::OK);
    EXPECT_EQ(sampleIndex, 4U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SparseCheckpointAndDenseRelease_001
 * @tc.desc: Verify direct checkpoint replay and sparse storage release after dense promotion
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SparseCheckpointAndDenseRelease_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    PrepareSparseSampleHelper(helper, 1026);
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexView view;
    ASSERT_EQ(helper.GetSampleIndexView(1025, view), Status::OK);
    ASSERT_EQ(helper.sparseCheckpoints_.size(), 2U);
    ASSERT_EQ(helper.GetSampleIndexView(1024, view), Status::OK);
    EXPECT_EQ(view.pos, 10340);
    EXPECT_EQ(view.dts, 10240);

    ASSERT_EQ(helper.EnsureDenseIndex(), Status::OK);
    EXPECT_EQ(helper.GetSampleIndexState(),
        Plugins::MPEG4::MPEG4SampleHelper::SampleIndexState::DENSE);
    EXPECT_TRUE(helper.sparseCheckpoints_.empty());
    EXPECT_EQ(helper.sparseCheckpoints_.capacity(), 0U);
    EXPECT_EQ(helper.sampleIndexEntry_.size(), 1026U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SampleIndexCttsRleConsume_001
 * @tc.desc: Verify CTTS RLE consumption transitions and compact index duration overflow checks
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SampleIndexCttsRleConsume_001, TestSize.Level1)
{
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.cttsEntry_ = {{2, 10}, {1, 20}};
    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexBuildContext context;
    context.cttsIndex = 0;
    context.cttsSamplesRemaining = 2;
    context.cttsDelta = 10;
    int64_t ptsOffset = -1;
    EXPECT_EQ(helper.ConsumeCttsSamples(context, 2, ptsOffset), Status::OK);
    EXPECT_EQ(ptsOffset, 10);
    EXPECT_EQ(context.cttsSamplesRemaining, 0U);
    EXPECT_EQ(helper.ConsumeCttsSamples(context, 1, ptsOffset), Status::OK);
    EXPECT_EQ(ptsOffset, 20);
    EXPECT_EQ(context.cttsIndex, 1U);
    EXPECT_EQ(context.cttsSamplesRemaining, 0U);
    EXPECT_EQ(helper.ConsumeCttsSamples(context, 1, ptsOffset), Status::ERROR_INVALID_DATA);

    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexBuildContext crossingContext;
    crossingContext.cttsIndex = 0;
    crossingContext.cttsSamplesRemaining = 1;
    crossingContext.cttsDelta = 10;
    EXPECT_EQ(helper.ConsumeCttsSamples(crossingContext, 2, ptsOffset), Status::OK);
    EXPECT_EQ(ptsOffset, 10);
    EXPECT_EQ(crossingContext.cttsIndex, 1U);
    EXPECT_EQ(crossingContext.cttsSamplesRemaining, 0U);

    Plugins::MPEG4::MPEG4SampleHelper::SampleIndexBuildContext emptyContext;
    emptyContext.cttsIndex = 1;
    EXPECT_EQ(helper.ConsumeCttsSamples(emptyContext, 1, ptsOffset), Status::ERROR_INVALID_DATA);

    int64_t duration = 0;
    int64_t currentDts = 100;
    EXPECT_EQ(helper.UpdateSampleDurationAndDts(10, 0, duration, currentDts), Status::ERROR_INVALID_DATA);
    EXPECT_EQ(helper.UpdateSampleDurationAndDts(std::numeric_limits<uint32_t>::max(),
        std::numeric_limits<uint32_t>::max(), duration, currentDts), Status::ERROR_INVALID_DATA);
    currentDts = std::numeric_limits<int64_t>::max();
    EXPECT_EQ(helper.UpdateSampleDurationAndDts(1, 1, duration, currentDts), Status::ERROR_INVALID_DATA);
    currentDts = 100;
    EXPECT_EQ(helper.UpdateSampleDurationAndDts(20, 2, duration, currentDts), Status::OK);
    EXPECT_EQ(duration, 40);
    EXPECT_EQ(currentDts, 140);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcLowRefISliceSyncFlag_001
 * @tc.desc: Verify a non-IDR I-slice with low SPS/PPS ref counts is marked as a sync frame like FFmpeg
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcLowRefISliceSyncFlag_001, TestSize.Level1)
{
    constexpr uint32_t testSampleCount = 5;
    // Baseline profile SPS with max_num_ref_frames == 1 (no extended syntax fields).
    const std::vector<uint8_t> lowRefSps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    // Non-IDR I-slice: NAL header type 1, first_mb_in_slice 0, slice_type 7 (I).
    const std::vector<uint8_t> iSliceNal = {0x61, 0x88, 0xAA, 0xAA, 0xAA, 0xAA};
    // P-slice: slice_type 0.
    const std::vector<uint8_t> pSliceNal = {0x61, 0xC0, 0xAA, 0xAA, 0xAA, 0xAA};
    // B-slice: slice_type 1.
    const std::vector<uint8_t> bSliceNal = {0x61, 0xA0, 0xAA, 0xAA, 0xAA, 0xAA};
    // IDR slice: NAL header type 5.
    const std::vector<uint8_t> idrNal = {0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA};
    // index 0: stss sync frame, index 3: IDR key via NAL type, index 4: non-IDR I-slice key via heuristic.
    auto plugin = BuildAvcKeyHeuristicTestPlugin(lowRefSps,
        {iSliceNal, pSliceNal, bSliceNal, idrNal, iSliceNal}, {{0}});
    ASSERT_NE(plugin, nullptr);

    constexpr uint32_t syncFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME);
    const std::vector<uint32_t> expectedSyncIndexes = {0, 3, 4};
    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    for (uint32_t index = 0; index < testSampleCount; ++index) {
        ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
        bool expectSync = std::find(expectedSyncIndexes.begin(), expectedSyncIndexes.end(), index) !=
            expectedSyncIndexes.end();
        EXPECT_EQ((output.mediaAVBuffer->flag_ & syncFlag) != 0, expectSync) <<
            "sample index " << index << " sync flag mismatch";
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcHighRefISliceNotSync_001
 * @tc.desc: Verify a non-IDR I-slice with SPS max_num_ref_frames > 1 is not marked as a sync frame
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcHighRefISliceNotSync_001, TestSize.Level1)
{
    // Baseline profile SPS with max_num_ref_frames == 2.
    const std::vector<uint8_t> highRefSps = {0x67, 0x42, 0xC0, 0x1E, 0xF6, 0x31, 0xB2};
    const std::vector<uint8_t> iSliceNal = {0x61, 0x88, 0xAA, 0xAA, 0xAA, 0xAA};
    auto plugin = BuildAvcKeyHeuristicTestPlugin(highRefSps, {iSliceNal}, {});
    ASSERT_NE(plugin, nullptr);

    constexpr uint32_t syncFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME);
    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ((output.mediaAVBuffer->flag_ & syncFlag) != 0, false);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcPrefixNalEmbeddedIdrSyncFlag_001
 * @tc.desc: Verify FFmpeg-style key detection for an Annex-B IDR inside an AVC Prefix NAL
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcPrefixNalEmbeddedIdrSyncFlag_001, TestSize.Level1)
{
    constexpr size_t testSampleCount = 4;
    constexpr size_t annexbStartCodeOffset = AVC_PREFIX_NAL_TEST_SCAN_BUFFER_SIZE - 2;
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    const std::vector<uint8_t> idr = {0x6E, 0x00, 0x00, 0x00, 0x01, 0x06, 0x00, 0x00, 0x00, 0x01, 0x65, 0x88};
    const std::vector<uint8_t> sei = {0x6E, 0x00, 0x00, 0x00, 0x01, 0x06, 0x88};
    const std::vector<uint8_t> truncated = {0x6E, 0x00, 0x00, 0x00, 0x01};
    std::vector<uint8_t> crossChunkIdr(AVC_PREFIX_NAL_TEST_SCAN_BUFFER_SIZE + 3, 0xFF);
    crossChunkIdr[0] = 0x6E;
    crossChunkIdr[annexbStartCodeOffset + 1] = 0;
    crossChunkIdr[annexbStartCodeOffset + 2] = 0;
    crossChunkIdr[annexbStartCodeOffset + 3] = 1;
    crossChunkIdr[annexbStartCodeOffset + 4] = 0x65;
    auto plugin = BuildAvcKeyHeuristicTestPlugin(sps, {idr, sei, truncated, crossChunkIdr},
        {{0}, {1}, {2}, {3}});
    ASSERT_NE(plugin, nullptr);

    constexpr uint32_t syncFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME);
    const std::array<bool, testSampleCount> expectedSync = {true, false, false, true};
    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    for (bool expectSync : expectedSync) {
        ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ((output.mediaAVBuffer->flag_ & syncFlag) != 0, expectSync);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcEmptyPrefixThenIdrSyncFlag_001
 * @tc.desc: Verify an empty Prefix NAL does not stop FFmpeg-style scanning for a following IDR
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcEmptyPrefixThenIdrSyncFlag_001, TestSize.Level1)
{
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    const std::vector<uint8_t> emptyPrefix = {0x6E};
    const std::vector<uint8_t> idr = {0x65, 0x88};
    auto plugin = BuildAvcKeyHeuristicTestPlugin(sps, {emptyPrefix, idr}, {}, false);
    ASSERT_NE(plugin, nullptr);

    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
    constexpr uint32_t syncFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME);
    EXPECT_NE(output.mediaAVBuffer->flag_ & syncFlag, 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcRecoveryPointSeiSyncFlag_001
 * @tc.desc: Verify a recovery_point SEI (payload type 6) before a slice marks a sync frame like
 *           FFmpeg h264_parser.c, while a non-recovery SEI or a SEI after the slice does not
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcRecoveryPointSeiSyncFlag_001, TestSize.Level1)
{
    // SEI NAL (type 6): recovery_point (payload type 6) with recovery_frame_cnt ue(0) = "1" -> 0x80.
    const std::vector<uint8_t> recoverySei = {0x06, 0x06, 0x01, 0x80};
    // SEI NAL carrying a non-recovery message (pic_timing, payload type 1).
    const std::vector<uint8_t> otherSei = {0x06, 0x01, 0x00};
    // Non-IDR P-slice so only the SEI heuristic can mark the sample as key.
    const std::vector<uint8_t> pSlice = {0x61, 0xC0, 0xAA, 0xAA};
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    std::vector<uint8_t> sampleData;
    std::vector<int32_t> sampleSizes;
    BuildAvcMultiSampleData({{recoverySei, pSlice}, {otherSei, pSlice}, {pSlice, recoverySei}},
        sampleData, sampleSizes);
    auto plugin = BuildAvcMultiNalSampleTestPlugin(sps, sampleData, sampleSizes, {});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::AVC), Status::OK);

    constexpr uint32_t syncFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME);
    const std::array<bool, 3> expectedSync = {true, false, false};
    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    for (uint32_t index = 0; index < expectedSync.size(); ++index) {
        ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ((output.mediaAVBuffer->flag_ & syncFlag) != 0, expectedSync[index]) <<
            "sample index " << index << " sync flag mismatch";
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcRecoveryPointSeiMultiMessage_001
 * @tc.desc: Verify a recovery_point SEI that follows another sei_message inside the same SEI NAL
 *           still marks the sample as a sync frame, matching FFmpeg ff_h264_sei_decode which
 *           iterates every sei_message instead of only the first one
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcRecoveryPointSeiMultiMessage_001, TestSize.Level1)
{
    // One SEI NAL carrying pic_timing (type 1, size 0) followed by recovery_point (type 6,
    // size 1, payload 0x80 = ue(0)); recovery_point is not the first message, so only the
    // multi-message scan can mark this sample as key.
    const std::vector<uint8_t> recoveryAfterOtherSei = {0x06, 0x01, 0x00, 0x06, 0x01, 0x80};
    // One SEI NAL with two non-recovery messages (pic_timing x2) must not mark the sample.
    const std::vector<uint8_t> noRecoveryMultiSei = {0x06, 0x01, 0x00, 0x01, 0x00};
    // Non-IDR P-slice so only the SEI heuristic can mark the sample as key.
    const std::vector<uint8_t> pSlice = {0x61, 0xC0, 0xAA, 0xAA};
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    std::vector<uint8_t> sampleData;
    std::vector<int32_t> sampleSizes;
    BuildAvcMultiSampleData({{recoveryAfterOtherSei, pSlice}, {noRecoveryMultiSei, pSlice}},
        sampleData, sampleSizes);
    auto plugin = BuildAvcMultiNalSampleTestPlugin(sps, sampleData, sampleSizes, {});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::AVC), Status::OK);

    constexpr uint32_t syncFlag = static_cast<uint32_t>(Plugins::MPEG4::Sample::SampleFlag::SYNC_FRAME);
    const std::array<bool, 2> expectedSync = {true, false};
    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    for (uint32_t index = 0; index < expectedSync.size(); ++index) {
        ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
        EXPECT_EQ((output.mediaAVBuffer->flag_ & syncFlag) != 0, expectedSync[index]) <<
            "sample index " << index << " sync flag mismatch";
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekableVideoFirstFrameUsesFirstStssSync_001
 * @tc.desc: Verify seekable video init probes the first stss sync sample instead of sample 0,
 *           aligned with FFmpeg ParseVideoFirstFramesFull, and rewinds the sample cursor.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekableVideoFirstFrameUsesFirstStssSync_001,
    TestSize.Level1)
{
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    const std::vector<uint8_t> sample0 = {0x61, 0xC0, 0xAA, 0xAA}; // P-slice, 4 bytes.
    const std::vector<uint8_t> sample1 = {0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA}; // IDR, 6 bytes.
    auto plugin = BuildAvcKeyHeuristicTestPlugin(sps, {sample0, sample1}, {{1}});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::AVC), Status::OK);

    ASSERT_EQ(plugin->ParseAVFirstFrameForTrack(0), Status::OK);
    auto firstFrame = plugin->firstFrameMap_.find(0);
    ASSERT_NE(firstFrame, plugin->firstFrameMap_.end());
    ASSERT_NE(firstFrame->second, nullptr);
    EXPECT_EQ(firstFrame->second->size, static_cast<int32_t>(BuildLengthPrefixedNal(sample1).size()));
    EXPECT_EQ(plugin->firstTrack_->currentSampleIndex, 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SeekableVideoFirstFrameFallsBackToSampleZero_001
 * @tc.desc: Verify an out-of-range stss first entry (dropped by FinalizeSampleTables) falls back
 *           to sample 0 for the metadata probe, matching FFmpeg keyframe_absent video fallback.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SeekableVideoFirstFrameFallsBackToSampleZero_001,
    TestSize.Level1)
{
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    const std::vector<uint8_t> sample0 = {0x61, 0xC0, 0xAA, 0xAA}; // P-slice, 4 bytes.
    const std::vector<uint8_t> sample1 = {0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA}; // IDR, 6 bytes.
    auto plugin = BuildAvcKeyHeuristicTestPlugin(sps, {sample0, sample1}, {{5}});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::AVC), Status::OK);

    ASSERT_EQ(plugin->ParseAVFirstFrameForTrack(0), Status::OK);
    auto firstFrame = plugin->firstFrameMap_.find(0);
    ASSERT_NE(firstFrame, plugin->firstFrameMap_.end());
    ASSERT_NE(firstFrame->second, nullptr);
    EXPECT_EQ(firstFrame->second->size, static_cast<int32_t>(BuildLengthPrefixedNal(sample0).size()));
    EXPECT_EQ(plugin->firstTrack_->currentSampleIndex, 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_StssFirstIdrKeepsInBandSpsPps_001
 * @tc.desc: Verify an stss-first IDR metadata probe does not re-insert SPS/PPS before the IDR when
 *           the first read sample (sample 0) already carries in-band SPS/PPS, matching FFmpeg
 *           h264_mp4toannexb which keeps the in-band parameter-set seen state per packet.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_StssFirstIdrKeepsInBandSpsPps_001, TestSize.Level1)
{
    constexpr uint32_t sample0Size = 19;
    constexpr uint32_t sample1Size = 10;
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    // In-band PPS must be byte-identical to the avcC PPS (BuildAvcCTestConfig) so the parser state
    // after sample 0 matches FFmpeg's bsf: the seen in-band parameter sets suppress XPS insertion.
    const std::vector<uint8_t> pps = {0x68, 0xCE, 0x3C, 0x80};
    const std::vector<uint8_t> idr = {0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA};
    std::vector<uint8_t> sampleData;
    std::vector<uint8_t> spsPacket = BuildLengthPrefixedNal(sps);
    std::vector<uint8_t> ppsPacket = BuildLengthPrefixedNal(pps);
    std::vector<uint8_t> idrPacket = BuildLengthPrefixedNal(idr);
    sampleData.insert(sampleData.end(), spsPacket.begin(), spsPacket.end());
    sampleData.insert(sampleData.end(), ppsPacket.begin(), ppsPacket.end());
    sampleData.insert(sampleData.end(), idrPacket.begin(), idrPacket.end());
    // Only sample 1 (IDR) is a sync sample; the metadata probe must read sample 1.
    auto plugin = BuildAvcMultiNalSampleTestPlugin(sps, sampleData, {sample0Size, sample1Size}, {{1}});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::AVC), Status::OK);

    ASSERT_EQ(plugin->ParseAVFirstFrameForTrack(0), Status::OK);
    auto firstFrame = plugin->firstFrameMap_.find(0);
    ASSERT_NE(firstFrame, plugin->firstFrameMap_.end());
    ASSERT_NE(firstFrame->second, nullptr);
    EXPECT_EQ(firstFrame->second->size, static_cast<int32_t>(sample1Size));
    EXPECT_EQ(plugin->firstTrack_->currentSampleIndex, 0U);

    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    // Sample 0: Annex-B SPS+PPS only; the XPS reset at the first read sample must not alter this.
    ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
    const std::vector<uint8_t> expectedSample0 = {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2,
        0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80,
    };
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), static_cast<int32_t>(expectedSample0.size()));
    EXPECT_TRUE(std::equal(expectedSample0.begin(), expectedSample0.end(),
        output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()));

    // Sample 1 (stss-first IDR): the in-band SPS/PPS seen state from sample 0 must suppress the
    // XPS insertion; re-inserting avcC SPS/PPS here would add 24 bytes and regress line58.mp4.
    ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
    const std::vector<uint8_t> expectedSample1 = {
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA,
    };
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), static_cast<int32_t>(expectedSample1.size()));
    EXPECT_TRUE(std::equal(expectedSample1.begin(), expectedSample1.end(),
        output.mediaAVBuffer->memory_->GetAddr(),
        output.mediaAVBuffer->memory_->GetAddr() + output.mediaAVBuffer->memory_->GetSize()));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_NegativeDtsKeepsXpsSeenState_001
 * @tc.desc: Verify with a valid first frame, non-positive DTS on later samples does not re-reset
 *           the XPS send state: a PPS+slice sample (no in-band SPS) keeps the SPS seen state from
 *           sample 0, so no out-of-band SPS is prepended (CVPA1_TOSHIBA_B_err_pic0-1-10-16), while
 *           a later IDR after a slice still re-inserts SPS/PPS like FFmpeg h264_mp4toannexb.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_NegativeDtsKeepsXpsSeenState_001, TestSize.Level1)
{
    const std::vector<uint8_t> sps = {0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2};
    const std::vector<uint8_t> pps = {0x68, 0xCE, 0x3C, 0x80}; // avcC PPS.
    const std::vector<uint8_t> idr = {0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA};
    const std::vector<uint8_t> slice = {0x61, 0xC0, 0xAA, 0xAA};
    std::vector<uint8_t> sampleData;
    std::vector<int32_t> sampleSizes;
    BuildAvcMultiSampleData({{sps, pps, idr}, {pps, slice}, {idr}}, sampleData, sampleSizes);
    // Samples 0 and 2 are sync samples; the stss-first metadata probe reads sample 0.
    auto plugin = BuildAvcMultiNalSampleTestPlugin(sps, sampleData, sampleSizes, {{0}, {2}});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::AVC), Status::OK);
    // Negative DTS like an edit list at stream start (CVPA1_TOSHIBA_B_err_pic0-1-10-16).
    // The default index is sparse (sampleIndexEntry_ empty); materialize the dense index first
    // so the per-sample DTS override below targets real entries instead of an empty vector.
    ASSERT_EQ(plugin->firstTrack_->sampleHelper->EnsureDenseIndex(), Status::OK);
    plugin->firstTrack_->sampleHelper->sampleIndexEntry_[0].dts = -40000;
    plugin->firstTrack_->sampleHelper->sampleIndexEntry_[1].dts = -20000;
    plugin->firstTrack_->sampleHelper->sampleIndexEntry_[2].dts = 20000;

    ASSERT_EQ(plugin->ParseAVFirstFrameForTrack(0), Status::OK);
    auto firstFrame = plugin->firstFrameMap_.find(0);
    ASSERT_NE(firstFrame, plugin->firstFrameMap_.end());
    ASSERT_NE(firstFrame->second, nullptr);
    EXPECT_EQ(firstFrame->second->size, static_cast<int32_t>(sampleSizes[0]));

    // Sample 0 IDR keeps in-band SPS/PPS; the probe-state reset must not leak into later samples.
    ExpectReadSampleAnnexb(plugin.get(), {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2,
        0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80,
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA,
    });
    // Sample 1 (negative DTS): in-band SPS seen state from sample 0 suppresses XPS insertion;
    // re-inserting avcC SPS here would add 13 bytes and regress CVPA1_TOSHIBA.
    ExpectReadSampleAnnexb(plugin.get(), {
        0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80,
        0x00, 0x00, 0x00, 0x01, 0x61, 0xC0, 0xAA, 0xAA,
    });
    // Sample 2 (IDR after a slice): per-slice reset clears the seen state, so SPS/PPS are
    // re-inserted before the IDR, matching FFmpeg h264_mp4toannexb.
    ExpectReadSampleAnnexb(plugin.get(), {
        0x00, 0x00, 0x00, 0x01, 0x67, 0x42, 0xC0, 0x1E, 0xF4, 0x31, 0xB2,
        0x00, 0x00, 0x00, 0x01, 0x68, 0xCE, 0x3C, 0x80,
        0x00, 0x00, 0x00, 0x01, 0x65, 0x88, 0xAA, 0xAA, 0xAA, 0xAA,
    });
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_HevcTwoByteLengthPrefixAnnexbGrowth_001
 * @tc.desc: Verify HEVC samples with 2-byte NAL length prefixes (hvcC lengthSizeMinusOne=1) convert
 *           to Annex-B with a 4-byte start code per NAL plus out-of-band VPS/SPS/PPS, covering
 *           multi-NAL samples; regression guard for the 877Kbps 320x240 playback failure
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_HevcTwoByteLengthPrefixAnnexbGrowth_001,
    TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    constexpr uint8_t hevcLengthSizeMinusOne = 1;
    // Minimal NALs: 2-byte NAL headers (nal_unit_type << 1, layer/temporal id byte) plus payload;
    // the HEVC parser re-frames NAL units and does not decode slice bitstreams.
    const std::vector<uint8_t> vps = {0x40, 0x01, 0xAA, 0xBB};
    const std::vector<uint8_t> sps = {0x42, 0x01, 0xCC, 0xDD};
    const std::vector<uint8_t> pps = {0x44, 0x01, 0xEE, 0xFF};
    // One sample with two NALs (IDR + trailing slice): each 2-byte length prefix grows by 2 bytes.
    const std::vector<uint8_t> idr = {0x26, 0x01, 0x11, 0x22, 0x33, 0x44};
    const std::vector<uint8_t> trailing = {0x02, 0x01, 0x55, 0x66, 0x77, 0x88};
    const std::vector<uint8_t> hvcc = BuildHevcCTestConfig(vps, sps, pps, hevcLengthSizeMinusOne);
    std::vector<uint8_t> sampleData = BuildHevcLengthPrefixedNal(idr);
    const std::vector<uint8_t> trailingPrefixed = BuildHevcLengthPrefixedNal(trailing);
    sampleData.insert(sampleData.end(), trailingPrefixed.begin(), trailingPrefixed.end());
    const std::vector<int32_t> sampleSizes = {static_cast<int32_t>(sampleData.size())};
    auto plugin = BuildHevcSampleTestPlugin(hvcc, sampleData, sampleSizes, {{0}});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::HEVC), Status::OK);
    ASSERT_EQ(plugin->ParseAVFirstFrameForTrack(0), Status::OK);
    ASSERT_TRUE(plugin->streamParsers_->ParserIsInited(0));
    std::vector<uint8_t> expected;
    for (const auto* nal : {&vps, &sps, &pps, &idr, &trailing}) {
        expected.insert(expected.end(), {0x00, 0x00, 0x00, 0x01});
        expected.insert(expected.end(), nal->begin(), nal->end());
    }
    ExpectReadSampleAnnexb(plugin.get(), expected);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_HevcOneAndFourByteLengthPrefixAnnexbGrowth_001
 * @tc.desc: Verify 1-byte (lengthSizeMinusOne=0) and 4-byte (lengthSizeMinusOne=3) NAL length
 *           prefixes convert to Annex-B correctly: +3 bytes per NAL for 1-byte prefixes and no
 *           growth for 4-byte prefixes
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_HevcOneAndFourByteLengthPrefixAnnexbGrowth_001,
    TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    const std::vector<uint8_t> vps = {0x40, 0x01, 0xAA, 0xBB};
    const std::vector<uint8_t> sps = {0x42, 0x01, 0xCC, 0xDD};
    const std::vector<uint8_t> pps = {0x44, 0x01, 0xEE, 0xFF};
    const std::vector<uint8_t> idr = {0x26, 0x01, 0x11, 0x22, 0x33, 0x44};
    for (uint8_t lengthSizeMinusOne : {0, 3}) {
        const uint8_t lengthSize = static_cast<uint8_t>(lengthSizeMinusOne + 1);
        const std::vector<uint8_t> hvcc = BuildHevcCTestConfig(vps, sps, pps, lengthSizeMinusOne);
        const std::vector<uint8_t> sampleData = BuildLengthPrefixedNal(idr, lengthSize);
        const std::vector<int32_t> sampleSizes = {static_cast<int32_t>(sampleData.size())};
        auto plugin = BuildHevcSampleTestPlugin(hvcc, sampleData, sampleSizes, {{0}});
        ASSERT_NE(plugin, nullptr);
        plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
        ASSERT_NE(plugin->streamParsers_, nullptr);
        ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::HEVC), Status::OK);
        ASSERT_EQ(plugin->ParseAVFirstFrameForTrack(0), Status::OK);
        ASSERT_TRUE(plugin->streamParsers_->ParserIsInited(0));
        std::vector<uint8_t> expected;
        for (const auto* nal : {&vps, &sps, &pps, &idr}) {
            expected.insert(expected.end(), {0x00, 0x00, 0x00, 0x01});
            expected.insert(expected.end(), nal->begin(), nal->end());
        }
        ExpectReadSampleAnnexb(plugin.get(), expected);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_HevcOversizedLengthPrefixDelivered_001
 * @tc.desc: Verify a HEVC sample whose NAL length prefix exceeds the remaining bytes is delivered
 *           with Status::OK and an unchanged size, matching the FFmpeg demuxer plugin, which uses
 *           the same libav_codec_hevc_parser and returns OK regardless of the conversion result
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_HevcOversizedLengthPrefixDelivered_001,
    TestSize.Level1)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        GTEST_SKIP() << "hevc parser library not found";
    }
    constexpr uint8_t hevcLengthSizeMinusOne = 1;
    constexpr uint16_t oversizedNalLength = 0xFFFF;
    const std::vector<uint8_t> vps = {0x40, 0x01, 0xAA, 0xBB};
    const std::vector<uint8_t> sps = {0x42, 0x01, 0xCC, 0xDD};
    const std::vector<uint8_t> pps = {0x44, 0x01, 0xEE, 0xFF};
    const std::vector<uint8_t> hvcc = BuildHevcCTestConfig(vps, sps, pps, hevcLengthSizeMinusOne);
    std::vector<uint8_t> sampleData;
    AppendU16(sampleData, oversizedNalLength);
    sampleData.insert(sampleData.end(), {0x26, 0x01, 0x11, 0x22});
    const std::vector<int32_t> sampleSizes = {static_cast<int32_t>(sampleData.size())};
    auto plugin = BuildHevcSampleTestPlugin(hvcc, sampleData, sampleSizes, {{0}});
    ASSERT_NE(plugin, nullptr);
    plugin->streamParsers_ = std::make_shared<MultiStreamParserManager>();
    ASSERT_NE(plugin->streamParsers_, nullptr);
    ASSERT_EQ(plugin->streamParsers_->Create(0, VideoStreamType::HEVC), Status::OK);
    ASSERT_EQ(plugin->ParseAVFirstFrameForTrack(0), Status::OK);
    ASSERT_TRUE(plugin->streamParsers_->ParserIsInited(0));
    AVBufferWrapper output(MPEG4_DEMUXER_PLUGIN_TEST_DEFAULT_BUFFER_SIZE);
    ASSERT_EQ(plugin->ReadSample(0, output.mediaAVBuffer), Status::OK);
    EXPECT_EQ(output.mediaAVBuffer->memory_->GetSize(), static_cast<int32_t>(sampleData.size()));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EditProjectedVideoDelay_001
 * @tc.desc: Verify H.264 delay uses edit-projected output timestamps like FFmpeg mov_fix_index
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EditProjectedVideoDelay_001, TestSize.Level1)
{
    constexpr uint32_t sampleSize = 1;
    constexpr uint32_t testSampleCount = 3;
    constexpr uint32_t sampleDuration = 10;
    constexpr int64_t firstSampleDts = -static_cast<int64_t>(sampleDuration);
    constexpr int64_t reorderedSamplePts = static_cast<int64_t>(sampleDuration) + sampleDuration;
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.sttsEntry_ = {{testSampleCount, sampleDuration}};
    helper.cttsEntry_ = {{testSampleCount, 0}};
    helper.sampleTableSummary_.rawSampleCount = testSampleCount;
    helper.sampleIndexEntry_ = {
        // SampleIndexEntry stores PTS as a non-negative offset from DTS. These normalized DTS values preserve
        // the edit-projected PTS order {0, 20, 10}, which has one frame of display reordering.
        MakeSampleIndexEntry(0, firstSampleDts, 0, sampleSize),
        MakeSampleIndexEntry(1, 0, reorderedSamplePts, sampleSize),
        MakeSampleIndexEntry(2, sampleDuration, sampleDuration, sampleSize),
    };
    helper.editListApplied_ = true;
    EXPECT_EQ(helper.EstimateVideoDelay(), 1U);

    helper.editListApplied_ = false;
    EXPECT_EQ(helper.EstimateVideoDelay(), 0U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SparseEditProjectedVideoDelay_001
 * @tc.desc: Verify sparse edit output delay matches FFmpeg-style projected PTS ordering
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SparseEditProjectedVideoDelay_001, TestSize.Level1)
{
    constexpr uint32_t testSampleCount = 3;
    Plugins::MPEG4::MPEG4SampleHelper helper;
    helper.trackType_ = Plugins::MPEG4::VIDEO_TYPE;
    helper.mimeType_ = MimeType::VIDEO_AVC;
    helper.allSamplesSync_ = true;
    helper.sttsEntry_ = {{testSampleCount, 10}};
    helper.cttsEntry_ = {{1, 0}, {1, 10}, {1, -10}};
    helper.stscEntry_ = {{0, testSampleCount, 1}};
    helper.sampleSizes_.assign(testSampleCount, 1);
    helper.sampleSizeCount_ = testSampleCount;
    helper.chunkOffsets_ = {0};
    helper.SetEditListParams({{0, 30, 1, 0}}, 1, 1);
    ASSERT_EQ(helper.FinalizeSampleTables(), Status::OK);
    EXPECT_EQ(helper.GetSampleIndexState(), Plugins::MPEG4::MPEG4SampleHelper::SampleIndexState::UNBUILT);
    EXPECT_EQ(helper.EstimateVideoDelay(), 1U);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcSpsDelayOverridesZeroCtts_001
 * @tc.desc: Verify codec SPS reorder evidence raises a zero MOV CTTS delay like FFmpeg probing
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcSpsDelayOverridesZeroCtts_001, TestSize.Level1)
{
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::CttsEntry> ctts = {
        {VIDEO_DELAY_TEST_SUPPORTED_SAMPLE_COUNT, 0},
    };
    EXPECT_EQ(CalculateAvcVideoDelayForTest(H264_REORDER_TWO_AVCC_FIXTURE, ctts,
        VIDEO_DELAY_TEST_SUPPORTED_SAMPLE_COUNT), static_cast<int32_t>(VIDEO_DELAY_TEST_SPS_REORDER));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcSpsDelayRaisesLowerCtts_001
 * @tc.desc: Verify FFmpeg FFMAX semantics retain the larger SPS reorder depth when CTTS is nonzero
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcSpsDelayRaisesLowerCtts_001, TestSize.Level1)
{
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::CttsEntry> ctts = {
        {1, 0}, {1, 1}, {1, -1}, {1, 0},
    };
    EXPECT_EQ(CalculateAvcVideoDelayForTest(H264_REORDER_TWO_AVCC_FIXTURE, ctts,
        VIDEO_DELAY_TEST_SUPPORTED_SAMPLE_COUNT), static_cast<int32_t>(VIDEO_DELAY_TEST_SPS_REORDER));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcCttsDelayRetainedWithoutSps_001
 * @tc.desc: Verify a valid MOV CTTS estimate is retained when codec reorder parsing is unavailable
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcCttsDelayRetainedWithoutSps_001, TestSize.Level1)
{
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::CttsEntry> ctts = {
        {1, 0}, {1, 1}, {1, -1}, {1, 0},
    };
    EXPECT_EQ(CalculateAvcVideoDelayForTest({}, ctts, VIDEO_DELAY_TEST_SUPPORTED_SAMPLE_COUNT), 1);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcSpsDelayWithoutCtts_001
 * @tc.desc: Verify codec SPS reorder evidence supplies delay when MOV CTTS is absent
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcSpsDelayWithoutCtts_001, TestSize.Level1)
{
    EXPECT_EQ(CalculateAvcVideoDelayForTest(H264_REORDER_TWO_AVCC_FIXTURE, {},
        VIDEO_DELAY_TEST_SUPPORTED_SAMPLE_COUNT), static_cast<int32_t>(VIDEO_DELAY_TEST_SPS_REORDER));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AvcSpsDelayNeedsEnoughSamples_001
 * @tc.desc: Verify SPS delay is not exposed when the track cannot contain the reorder depth
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AvcSpsDelayNeedsEnoughSamples_001, TestSize.Level1)
{
    EXPECT_EQ(CalculateAvcVideoDelayForTest(H264_REORDER_TWO_AVCC_FIXTURE, {},
        VIDEO_DELAY_TEST_INSUFFICIENT_SAMPLE_COUNT), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AuxEditListMinCorrectedPtsShift_001
 * @tc.desc: Verify auxiliary video-like tracks apply the min corrected PTS shift like FFmpeg need_parse_video_info
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AuxEditListMinCorrectedPtsShift_001, TestSize.Level1)
{
    constexpr uint32_t sampleSize = 100;
    constexpr int64_t emptyEditDuration = 2005;
    constexpr int64_t mediaEditDuration = 1068;
    constexpr int64_t mediaTime = -1;
    constexpr int64_t mediaEditTime = 0;
    constexpr int32_t movieTimeScale = 1000;
    constexpr int32_t trackTimeScale = 90000;
    // Empty edit (2005 ms, mediaTime -1) + media edit starting at media time 0;
    // ctts offsets shift min PTS to 2 ticks. EditListEntry fields are (mediaTime, duration).
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::EditListEntry> editList = {
        {mediaTime, emptyEditDuration, 1, 0},
        {mediaEditTime, mediaEditDuration, 1, 0},
    };
    const std::vector<Plugins::MPEG4::MPEG4SampleHelper::SampleIndexEntry> entries = {
        MakeSampleIndexEntry(0, 0, 2, sampleSize),
        MakeSampleIndexEntry(1, 1, 51021, sampleSize),
        MakeSampleIndexEntry(2, 2, 96039, sampleSize),
    };
    const std::vector<int64_t> expectedDts = {180448, 180449, 180450};
    const std::vector<int64_t> expectedPts = {180450, 231469, 276487};
    for (auto trackType : {Plugins::MPEG4::VIDEO_TYPE, Plugins::MPEG4::AUXILIARY_TYPE}) {
        Plugins::MPEG4::MPEG4SampleHelper helper;
        helper.trackType_ = trackType;
        helper.mimeType_ = MimeType::VIDEO_HEVC;
        helper.allSamplesSync_ = true;
        helper.sampleIndexEntry_ = entries;
        helper.SetEditListParams(editList, movieTimeScale, trackTimeScale);
        ASSERT_EQ(helper.ApplyEditList(), Status::OK);
        for (uint32_t index = 0; index < expectedDts.size(); ++index) {
            int64_t dts = 0;
            int64_t pts = 0;
            ASSERT_EQ(helper.GetSampleDts(index, dts), Status::OK);
            ASSERT_EQ(helper.GetSamplePts(index, pts), Status::OK);
            EXPECT_EQ(dts, expectedDts[index]) << "trackType " << static_cast<int32_t>(trackType) << " dts";
            EXPECT_EQ(pts, expectedPts[index]) << "trackType " << static_cast<int32_t>(trackType) << " pts";
        }
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_IlstStringNulPaddingTrim_001
 * @tc.desc: Verify udta/ilst string metadata is trimmed at the first NUL to align with FFmpeg av_dict_set
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_IlstStringNulPaddingTrim_001, TestSize.Level1)
{
    const std::string expectedComment = "SANYO DIGITAL CAMERA HD2000";
    constexpr size_t nulPaddingCount = 23;

    std::vector<uint8_t> dataPayload;
    AppendU32(dataPayload, 1); // dataType: UTF-8
    AppendU32(dataPayload, 0); // reserved
    dataPayload.insert(dataPayload.end(), expectedComment.begin(), expectedComment.end());
    dataPayload.insert(dataPayload.end(), nulPaddingCount, '\0');
    auto dataBox = BuildAtomHeader(dataPayload.size(), Plugins::MPEG4::FourccType("data"), false);
    dataBox.insert(dataBox.end(), dataPayload.begin(), dataPayload.end());

    auto infBox = BuildAtomHeader(dataBox.size(), Plugins::MPEG4::FourccType("\xA9inf"), false);
    infBox.insert(infBox.end(), dataBox.begin(), dataBox.end());

    auto ilstBox = BuildAtomHeader(infBox.size(), Plugins::MPEG4::FourccType("ilst"), false);
    ilstBox.insert(ilstBox.end(), infBox.begin(), infBox.end());

    std::vector<uint8_t> metaPayload;
    AppendU32(metaPayload, 0); // version/flags
    metaPayload.insert(metaPayload.end(), ilstBox.begin(), ilstBox.end());
    auto metaBox = BuildAtomHeader(metaPayload.size(), Plugins::MPEG4::FourccType("meta"), false);
    metaBox.insert(metaBox.end(), metaPayload.begin(), metaPayload.end());

    auto udtaBox = BuildAtomHeader(metaBox.size(), Plugins::MPEG4::FourccType("udta"), false);
    udtaBox.insert(udtaBox.end(), metaBox.begin(), metaBox.end());

    auto moovBox = BuildAtomHeader(udtaBox.size(), Plugins::MPEG4::FourccType("moov"), false);
    moovBox.insert(moovBox.end(), udtaBox.begin(), udtaBox.end());

    AtomParserTestContext test(moovBox);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    ASSERT_EQ(test.parser.MPEG4ParseAtom(0, &ctx), Status::OK);

    std::string actualComment;
    ASSERT_TRUE(test.parser.mediaInfo_.general.Get<Tag::MEDIA_COMMENT>(actualComment));
    EXPECT_EQ(actualComment, expectedComment);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TraditionalEmptyCopyrightPreserved_001
 * @tc.desc: Verify a recognized zero-length cprt value remains present like FFmpeg metadata
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TraditionalEmptyCopyrightPreserved_001, TestSize.Level1)
{
    auto file = BuildTraditionalMetadataFile(Plugins::MPEG4::FourccType("cprt"), {});
    AtomParserTestContext test(file);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    ASSERT_EQ(test.parser.MPEG4ParseAtom(0, &ctx), Status::OK);

    std::string copyrightValue = "unexpected";
    ASSERT_TRUE(test.parser.mediaInfo_.general.Get<Tag::MEDIA_COPYRIGHT>(copyrightValue));
    EXPECT_TRUE(copyrightValue.empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_TraditionalNulCopyrightPreserved_001
 * @tc.desc: Verify a recognized NUL-only cprt value is trimmed but not removed
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_TraditionalNulCopyrightPreserved_001, TestSize.Level1)
{
    auto file = BuildTraditionalMetadataFile(Plugins::MPEG4::FourccType("cprt"), {'\0'});
    AtomParserTestContext test(file);
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    ASSERT_EQ(test.parser.MPEG4ParseAtom(0, &ctx), Status::OK);

    std::string copyrightValue = "unexpected";
    ASSERT_TRUE(test.parser.mediaInfo_.general.Get<Tag::MEDIA_COPYRIGHT>(copyrightValue));
    EXPECT_TRUE(copyrightValue.empty());
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ZeroLengthMetadataDataType_001
 * @tc.desc: Verify zero-length numeric and unsupported metadata follow FFmpeg type handling
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ZeroLengthMetadataDataType_001, TestSize.Level1)
{
    constexpr uint32_t reservedType = 0;
    constexpr uint32_t utf8Type = 1;
    constexpr uint32_t utf8SortType = 4;
    constexpr uint32_t signedIntegerType = 21;
    constexpr uint32_t unsignedIntegerType = 22;
    constexpr uint32_t floatType = 23;
    constexpr uint32_t signedIntegerAltType = 67;
    Plugins::MPEG4::MPEG4AtomParser parser;
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    std::string key = "copyright";
    ASSERT_EQ(parser.ReadMetadataString(key, 0, 0, signedIntegerType, &ctx), Status::OK);
    std::string value;
    ASSERT_TRUE(parser.mediaInfo_.general.Get<Tag::MEDIA_COPYRIGHT>(value));
    EXPECT_EQ(value, "0");

    Plugins::MPEG4::MPEG4AtomParser unsupportedParser;
    ASSERT_EQ(unsupportedParser.ReadMetadataString(key, 0, 0, floatType, &ctx), Status::OK);
    EXPECT_FALSE(unsupportedParser.mediaInfo_.general.Get<Tag::MEDIA_COPYRIGHT>(value));

    Plugins::MPEG4::MPEG4AtomParser userParser;
    ctx.isUserMeta = true;
    for (uint32_t textType : {reservedType, utf8Type, utf8SortType}) {
        key = "empty_text_" + std::to_string(textType);
        EXPECT_EQ(userParser.ReadMetadataString(key, 0, 0, textType, &ctx), Status::OK);
        EXPECT_FALSE(userParser.userFormat_->GetData(key, value));
    }
    userParser.userFormat_.reset();
    EXPECT_EQ(userParser.ReadMetadataString(key, 0, 0, utf8Type, &ctx), Status::OK);
    userParser.userFormat_ = std::make_shared<Meta>();
    key = "signed";
    ASSERT_EQ(userParser.ReadMetadataString(key, 0, 0, signedIntegerType, &ctx), Status::OK);
    int32_t integerValue = -1;
    ASSERT_TRUE(userParser.userFormat_->GetData(key, integerValue));
    EXPECT_EQ(integerValue, 0);
    key = "unsigned";
    ASSERT_EQ(userParser.ReadMetadataString(key, 0, 0, unsignedIntegerType, &ctx), Status::OK);
    ASSERT_TRUE(userParser.userFormat_->GetData(key, value));
    EXPECT_EQ(value, "0");
    key = "signed_alt";
    ASSERT_EQ(userParser.ReadMetadataString(key, 0, 0, signedIntegerAltType, &ctx), Status::OK);
    ASSERT_TRUE(userParser.userFormat_->GetData(key, integerValue));
    EXPECT_EQ(integerValue, 0);
    key = "float";
    EXPECT_EQ(userParser.ReadMetadataString(key, 0, 0, floatType, &ctx), Status::OK);
    EXPECT_FALSE(userParser.userFormat_->GetData(key, value));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_UserMetaStringNulPaddingTrim_001
 * @tc.desc: Verify moov-level user metadata (mdta keys) strings are trimmed at the first NUL like FFmpeg
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_UserMetaStringNulPaddingTrim_001, TestSize.Level1)
{
    const std::string expectedValue = "Some Artist Name";
    constexpr size_t nulPaddingCount = 12;
    const std::string metaKey = "com.apple.quicktime.artist";

    std::vector<uint8_t> hdlrPayload;
    AppendU32(hdlrPayload, 0); // version/flags
    AppendU32(hdlrPayload, 0); // pre_defined
    AppendU32(hdlrPayload, Plugins::MPEG4::FourccType("mdta"));
    hdlrPayload.insert(hdlrPayload.end(), 12, '\0'); // reserved
    hdlrPayload.insert(hdlrPayload.end(), {'m', 'd', 't', 'a', '\0'});
    auto hdlrBox = BuildAtomHeader(hdlrPayload.size(), Plugins::MPEG4::FourccType("hdlr"), false);
    hdlrBox.insert(hdlrBox.end(), hdlrPayload.begin(), hdlrPayload.end());

    std::vector<uint8_t> keysPayload;
    AppendU32(keysPayload, 0); // version/flags
    AppendU32(keysPayload, 1); // entry_count
    AppendU32(keysPayload, static_cast<uint32_t>(8 + metaKey.size())); // key_size
    AppendU32(keysPayload, Plugins::MPEG4::FourccType("mdta"));
    keysPayload.insert(keysPayload.end(), metaKey.begin(), metaKey.end());
    auto keysBox = BuildAtomHeader(keysPayload.size(), Plugins::MPEG4::FourccType("keys"), false);
    keysBox.insert(keysBox.end(), keysPayload.begin(), keysPayload.end());

    std::vector<uint8_t> dataPayload;
    AppendU32(dataPayload, 1); // dataType: UTF-8
    AppendU32(dataPayload, 0); // reserved
    dataPayload.insert(dataPayload.end(), expectedValue.begin(), expectedValue.end());
    dataPayload.insert(dataPayload.end(), nulPaddingCount, '\0');
    auto dataBox = BuildAtomHeader(dataPayload.size(), Plugins::MPEG4::FourccType("data"), false);
    dataBox.insert(dataBox.end(), dataPayload.begin(), dataPayload.end());

    auto indexBox = BuildAtomHeader(dataBox.size(), 1, false); // 1 = metadata key index (BE)
    indexBox.insert(indexBox.end(), dataBox.begin(), dataBox.end());

    auto ilstBox = BuildAtomHeader(indexBox.size(), Plugins::MPEG4::FourccType("ilst"), false);
    ilstBox.insert(ilstBox.end(), indexBox.begin(), indexBox.end());

    std::vector<uint8_t> metaPayload;
    AppendU32(metaPayload, 0); // version/flags
    metaPayload.insert(metaPayload.end(), hdlrBox.begin(), hdlrBox.end());
    metaPayload.insert(metaPayload.end(), keysBox.begin(), keysBox.end());
    metaPayload.insert(metaPayload.end(), ilstBox.begin(), ilstBox.end());
    auto metaBox = BuildAtomHeader(metaPayload.size(), Plugins::MPEG4::FourccType("meta"), false);
    metaBox.insert(metaBox.end(), metaPayload.begin(), metaPayload.end());

    auto udtaBox = BuildAtomHeader(metaBox.size(), Plugins::MPEG4::FourccType("udta"), false);
    udtaBox.insert(udtaBox.end(), metaBox.begin(), metaBox.end());

    auto moovBox = BuildAtomHeader(udtaBox.size(), Plugins::MPEG4::FourccType("moov"), false);
    moovBox.insert(moovBox.end(), udtaBox.begin(), udtaBox.end());

    AtomParserTestContext test(moovBox);
    test.parser.userFormat_ = std::make_shared<Meta>();
    Plugins::MPEG4::MPEG4AtomParser::ParseContext ctx;
    ASSERT_EQ(test.parser.MPEG4ParseAtom(0, &ctx), Status::OK);

    std::string actualValue;
    ASSERT_TRUE(test.parser.userFormat_->GetData(metaKey, actualValue));
    EXPECT_EQ(actualValue, expectedValue);
}

constexpr uint32_t EOF_STRADDLE_SAMPLE_SIZE = 4;
constexpr uint32_t EOF_STRADDLE_PARTIAL_AVAILABLE_SIZE = 2;
constexpr uint32_t EOF_STRADDLE_BUFFER_CAPACITY = 64;
constexpr int32_t EOF_STRADDLE_TIMESCALE = 32000;
constexpr uint32_t EOF_STRADDLE_SAMPLE_COUNT = 2;

std::shared_ptr<Plugins::MPEG4::MPEG4AtomParser::Track> PrepareEofStraddleTestTrack(
    Plugins::MPEG4::MPEG4DemuxerPlugin& plugin)
{
    constexpr uint32_t testTrackId = 0;
    auto track = std::make_shared<Plugins::MPEG4::MPEG4AtomParser::Track>();
    auto helper = std::make_shared<Plugins::MPEG4::MPEG4SampleHelper>();
    if (track == nullptr || helper == nullptr) {
        return nullptr;
    }
    helper->sampleIndexEntry_ = {
        MakeSampleIndexEntry(0, 0, 0, EOF_STRADDLE_SAMPLE_SIZE),
        MakeSampleIndexEntry(EOF_STRADDLE_SAMPLE_SIZE, AAC_TEST_CORE_SAMPLES_PER_FRAME,
            AAC_TEST_CORE_SAMPLES_PER_FRAME, EOF_STRADDLE_SAMPLE_SIZE),
    };
    helper->allSamplesSync_ = true;
    helper->trackType_ = Plugins::MPEG4::AUDIO_TYPE;
    helper->mimeType_ = MimeType::AUDIO_AAC;
    helper->rawTotalDuration_ = EOF_STRADDLE_SAMPLE_COUNT * AAC_TEST_CORE_SAMPLES_PER_FRAME;
    if (helper->AddDurationOverride(EOF_STRADDLE_SAMPLE_COUNT - 1,
        AAC_TEST_CORE_SAMPLES_PER_FRAME) != Status::OK) {
        return nullptr;
    }
    track->trackIndex = testTrackId;
    track->trackId = static_cast<int32_t>(testTrackId);
    track->timeScale = EOF_STRADDLE_TIMESCALE;
    track->duration = EOF_STRADDLE_SAMPLE_COUNT * AAC_TEST_CORE_SAMPLES_PER_FRAME;
    track->codecParms.trackType = Plugins::MPEG4::AUDIO_TYPE;
    track->sampleHelper = helper;
    plugin.firstTrack_ = track;
    plugin.lastTrack_ = track;
    plugin.mediaInfo_.tracks.resize(1);
    plugin.mediaInfo_.tracks[testTrackId].Set<Tag::MIME_TYPE>(MimeType::AUDIO_AAC);
    plugin.mediaInfo_.tracks[testTrackId].Set<Tag::AUDIO_SAMPLE_RATE>(EOF_STRADDLE_TIMESCALE);
    plugin.mediaInfo_.tracks[testTrackId].Set<Tag::AUDIO_SAMPLE_PER_FRAME>(
        AAC_TEST_CORE_SAMPLES_PER_FRAME);
    plugin.trackMtx_[testTrackId] = std::make_shared<std::mutex>();
    plugin.inited_ = true;
    plugin.seekable_ = Seekable::SEEKABLE;
    plugin.selectedTrackIds_ = {testTrackId};
    return track;
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_EofStraddlePartialReadAndEos_001
 * @tc.desc: Verify a sample extending past EOF is delivered as the available short read (FFmpeg
 *           mov alignment) and a sample fully beyond EOF terminates with the EOS flag instead of
 *           looping on END_OF_STREAM without the flag.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_EofStraddlePartialReadAndEos_001, TestSize.Level1)
{
    // 6-byte source: EOF_STRADDLE_SAMPLE_SIZE bytes for the first sample, then
    // EOF_STRADDLE_PARTIAL_AVAILABLE_SIZE bytes available for the truncated second sample.
    const std::vector<uint8_t> sampleData = {
        0xAA, 0xBB, 0xCC, 0xDD,
        0x11, 0x22,
    };
    const std::vector<uint8_t> firstExpected = {0xAA, 0xBB, 0xCC, 0xDD};
    const std::vector<uint8_t> partialExpected = {0x11, 0x22};
    Plugins::MPEG4::MPEG4DemuxerPlugin plugin(MPEG4_PLUGIN_NAME);
    auto track = PrepareEofStraddleTestTrack(plugin);
    ASSERT_NE(track, nullptr);
    plugin.dataSource_ = CreateDataSourceForBytes(sampleData);
    ASSERT_NE(plugin.dataSource_, nullptr);

    AVBufferWrapper first(EOF_STRADDLE_BUFFER_CAPACITY);
    ASSERT_EQ(plugin.ReadSample(0, first.mediaAVBuffer), Status::OK);
    EXPECT_EQ(first.mediaAVBuffer->memory_->GetSize(), EOF_STRADDLE_SAMPLE_SIZE);
    EXPECT_EQ((first.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)), 0U);
    const uint8_t* firstData = static_cast<const uint8_t*>(first.mediaAVBuffer->memory_->GetAddr());
    EXPECT_EQ(std::vector<uint8_t>(firstData, firstData + EOF_STRADDLE_SAMPLE_SIZE), firstExpected);

    AVBufferWrapper partial(EOF_STRADDLE_BUFFER_CAPACITY);
    ASSERT_EQ(plugin.ReadSample(0, partial.mediaAVBuffer), Status::OK);
    EXPECT_EQ(partial.mediaAVBuffer->memory_->GetSize(), EOF_STRADDLE_PARTIAL_AVAILABLE_SIZE);
    EXPECT_EQ((partial.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)), 0U);
    const uint8_t* partialData = static_cast<const uint8_t*>(partial.mediaAVBuffer->memory_->GetAddr());
    EXPECT_EQ(std::vector<uint8_t>(partialData, partialData + EOF_STRADDLE_PARTIAL_AVAILABLE_SIZE),
        partialExpected);

    AVBufferWrapper eos(EOF_STRADDLE_BUFFER_CAPACITY);
    ASSERT_EQ(plugin.ReadSample(0, eos.mediaAVBuffer), Status::OK);
    EXPECT_NE((eos.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)), 0U);

    AVBufferWrapper eosAgain(EOF_STRADDLE_BUFFER_CAPACITY);
    ASSERT_EQ(plugin.ReadSample(0, eosAgain.mediaAVBuffer), Status::OK);
    EXPECT_NE((eosAgain.mediaAVBuffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)), 0U);
}

namespace {
void ExpectScanMatchesRegex(const std::string& input)
{
    std::regex pattern(R"([+-]\d+\.\d+)");
    std::vector<std::string> regexMatches;
    for (std::sregex_iterator iter(input.cbegin(), input.cend(), pattern), endIter;
        iter != endIter; ++iter) {
        regexMatches.push_back(iter->str());
    }

    std::array<std::string, MAX_LOCATION_FLOAT_COUNT> tokens{};
    uint32_t count = 0;
    ScanSignedFloatTokens(input, tokens, count);

    const uint32_t expectedCount = std::min(static_cast<uint32_t>(regexMatches.size()),
        MAX_LOCATION_FLOAT_COUNT);
    EXPECT_EQ(count, expectedCount);
    for (uint32_t idx = 0; idx < expectedCount; ++idx) {
        EXPECT_EQ(tokens[idx], regexMatches[idx]);
    }
}
} // namespace

/**
 * @tc.name: Mpeg4DemuxerPlugin_ScanSignedFloatTokensMatchesRegex_001
 * @tc.desc: Verify ScanSignedFloatTokens yields the same tokens and count as the original regex
 *           [+\-]\d+\.\d+ for valid, whitespace, sign-cluster and truncated location strings.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ScanSignedFloatTokensMatchesRegex_001, TestSize.Level1)
{
    const std::vector<std::string> inputs = {
        "+120.201302+30.188101/",
        "+30.188101+120.201302+12.34/",
        "-0.5+3.25/",
        "abc+1.2xyz",
        "  +1.2  -3.4  ",
        "+1.2\t-3.4\n",
        "++1.2",
        "--1.2",
        "+-1.2",
        "-+1.2",
        "+1.2.3",
        "+1.2.3.4",
        "+12+3.4",
        "+1.2e-3",
        "+1.",
        "+.5",
        ".5",
        "1.2",
        "+ 1.2",
        "+1 .2",
        "+1. 2",
        "+a-3.4",
        "+1a.2",
        "+1..2",
        "+1.2..",
        "+1.2-",
        "+1.2+3.4+5.6+7.8",
        "+0.0-0.0+0.0-0.0+0.0",
        "+999999.999999",
        "+1234567890.1234567890",
        "",
        "+",
        "-",
        "abc",
        "++++1.2",
    };
    for (const auto& input : inputs) {
        ExpectScanMatchesRegex(input);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_ScanSignedFloatTokensCountCap_001
 * @tc.desc: Verify extraction caps at three matches when the regex finds more, keeping the
 *           lat/lon/altitude count thresholds aligned with the regex result.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_ScanSignedFloatTokensCountCap_001, TestSize.Level1)
{
    const std::vector<std::string> inputs = {
        "+1.2+3.4+5.6+7.8+9.0",
        "+0.0-0.0+0.0-0.0+0.0-0.0+0.0",
        "+1.2+3.4+5.6",
        "+1.2+3.4",
        "+1.2",
        "no location here",
    };
    for (const auto& input : inputs) {
        ExpectScanMatchesRegex(input);
    }
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetLocationMetaLatitudeLongitude_001
 * @tc.desc: Verify two location tokens set latitude and longitude only, and skip altitude.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetLocationMetaLatitudeLongitude_001, TestSize.Level1)
{
    constexpr uint32_t twoTokens = 2;
    constexpr float latitudeValue = 30.188101f;
    constexpr float longitudeValue = 120.201302f;
    std::array<std::string, MAX_LOCATION_FLOAT_COUNT> tokens = {"+30.188101", "+120.201302", "+12.34"};
    Meta format;

    SetLocationMeta(format, tokens, twoTokens);

    float latitude = 0.0f;
    float longitude = 0.0f;
    float altitude = 0.0f;
    ASSERT_TRUE(format.Get<Tag::MEDIA_LATITUDE>(latitude));
    ASSERT_TRUE(format.Get<Tag::MEDIA_LONGITUDE>(longitude));
    EXPECT_FLOAT_EQ(latitude, latitudeValue);
    EXPECT_FLOAT_EQ(longitude, longitudeValue);
    EXPECT_FALSE(format.Get<Tag::MEDIA_ALTITUDE>(altitude));
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_SetLocationMetaAltitude_001
 * @tc.desc: Verify three location tokens additionally set altitude.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_SetLocationMetaAltitude_001, TestSize.Level1)
{
    constexpr uint32_t threeTokens = 3;
    constexpr float altitudeValue = 12.34f;
    std::array<std::string, MAX_LOCATION_FLOAT_COUNT> tokens = {"+30.188101", "+120.201302", "+12.34"};
    Meta format;

    SetLocationMeta(format, tokens, threeTokens);

    float altitude = 0.0f;
    ASSERT_TRUE(format.Get<Tag::MEDIA_ALTITUDE>(altitude));
    EXPECT_FLOAT_EQ(altitude, altitudeValue);
}

} // namespace Media

/**
 * @tc.name: Mpeg4DemuxerPlugin_AacProbeDtsDiffOverflow_001
 * @tc.desc: Regression: the AAC probe tie-break must not wrap on extreme dts. The fixture holds
 *           two audio tracks with dtsUs ~ 9223372036854000000 and -1000000, whose difference
 *           overflows a signed subtraction.
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AacProbeDtsDiffOverflow_001, TestSize.Level1)
{
    auto plugin = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
    ASSERT_NE(plugin, nullptr);
    int32_t fd = -1;
    MediaInfo mediaInfo;

    // Pre-fix code aborts here under a signed-integer-overflow sanitizer.
    InitPlugin(TEST_MP4_PATH_AAC_PROBE_DTS_OVERFLOW, plugin, fd, mediaInfo);
    EXPECT_GE(mediaInfo.tracks.size(), 1);
}

} // namespace OHOS
