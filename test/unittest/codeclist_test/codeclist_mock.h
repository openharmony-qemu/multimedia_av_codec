/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License") = 0;
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

#ifndef CODECLIST_MOCK_H
#define CODECLIST_MOCK_H

#include <iostream>
#include <string>
#include <vector>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "avformat_mock.h"
#include "media_description.h"
#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "native_averrors.h"
#include "nocopyable.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecListMock : public NoCopyable {
public:
    virtual ~CodecListMock() = default;
    virtual bool IsHardware() = 0;
    virtual std::string GetName() = 0;
    virtual int32_t GetMaxSupportedInstances() = 0; // return is not errcode
    virtual Range GetEncoderBitrateRange() = 0;
    virtual bool IsEncoderBitrateModeSupported(OH_BitrateMode bitrateMode) = 0;
    virtual Range GetEncoderQualityRange() = 0;
    virtual Range GetEncoderComplexityRange() = 0;
    virtual std::vector<int32_t> GetAudioSupportedSampleRates() = 0;
    virtual Range GetAudioChannelsRange() = 0;
    virtual int32_t GetVideoWidthAlignment() = 0;
    virtual int32_t GetVideoHeightAlignment() = 0;
    virtual Range GetVideoWidthRangeForHeight(int32_t height) = 0;
    virtual Range GetVideoHeightRangeForWidth(int32_t width) = 0;
    virtual Range GetVideoWidthRange() = 0;
    virtual Range GetVideoHeightRange() = 0;
    virtual bool IsVideoSizeSupported(int32_t width, int32_t height) = 0;
    virtual Range GetVideoFrameRateRange() = 0;
    virtual Range GetVideoFrameRateRangeForSize(int32_t width, int32_t height) = 0;
    virtual bool AreVideoSizeAndFrameRateSupported(int32_t width, int32_t height, int32_t frameRate) = 0;
    virtual std::vector<int32_t> GetVideoSupportedPixelFormats() = 0;
    virtual std::vector<int32_t> GetSupportedProfiles() = 0;
    virtual std::vector<int32_t> GetSupportedLevelsForProfile(int32_t profile) = 0;
    virtual bool AreProfileAndLevelSupported(int32_t profile, int32_t level) = 0;
};

class __attribute__((visibility("default"))) CodecListMockFactory {
public:
    static std::shared_ptr<CodecListMock> GetCapability(const std::string &mime, bool isEncoder);
    static std::shared_ptr<CodecListMock> GetCapabilityByCategory(const std::string &mime, bool isEncoder,
                                                                  AVCodecCategory category);

private:
    CodecListMockFactory() = delete;
    ~CodecListMockFactory() = delete;
};

namespace CodecListTestParam {
const std::map<std::string, std::string> CAPABILITY_DECODER_NAME = {
    {std::string(CodecMimeType::AUDIO_MPEG), std::string(AVCodecCodecName::AUDIO_DECODER_MP3_NAME)},
    {std::string(CodecMimeType::AUDIO_AAC), std::string(AVCodecCodecName::AUDIO_DECODER_AAC_NAME)},
    {std::string(CodecMimeType::AUDIO_VORBIS), std::string(AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME)},
    {std::string(CodecMimeType::AUDIO_FLAC), std::string(AVCodecCodecName::AUDIO_DECODER_FLAC_NAME)},
    {std::string(CodecMimeType::VIDEO_AVC), std::string(AVCodecCodecName::VIDEO_DECODER_AVC_NAME)}};

const std::map<std::string, std::string> CAPABILITY_ENCODER_NAME = {
    {std::string(CodecMimeType::AUDIO_FLAC), std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME)},
    {std::string(CodecMimeType::AUDIO_AAC), std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME)}};

const std::map<std::string, std::string> CAPABILITY_DECODER_HARD_NAME = {
    {std::string(CodecMimeType::VIDEO_AVC), "OMX.hisi.video.decoder.avc"},
    {std::string(CodecMimeType::VIDEO_HEVC), "OMX.hisi.video.decoder.hevc"}};

const std::map<std::string, std::string> CAPABILITY_ENCODER_HARD_NAME = {
    {std::string(CodecMimeType::VIDEO_AVC), "OMX.hisi.video.encoder.avc"},
    {std::string(CodecMimeType::VIDEO_HEVC), "OMX.hisi.video.encoder.hevc"}};

const std::string DEFAULT_AUDIO_MIME = std::string(CodecMimeType::AUDIO_AAC);
const std::string DEFAULT_VIDEO_MIME = std::string(CodecMimeType::VIDEO_AVC);
constexpr int32_t MAX_SURPPORT_ACODEC = 16;
constexpr int32_t MAX_SURPPORT_VCODEC = 64;

constexpr OH_AVRange DEFAULT_BITRATE_RANGE = {8000, 448000};
constexpr OH_AVRange DEFAULT_QUALITY_RANGE = {0, 0};
constexpr OH_AVRange DEFAULT_COMPLEXITY_RANGE = {0, 0};
constexpr OH_AVRange DEFAULT_CHANNELCOUNT_RANGE = {1, 8};
constexpr OH_AVRange DEFAULT_HEIGHT_RANGE_OF_WIDTH = {96, 4096};
constexpr OH_AVRange DEFAULT_WIDTH_RANGE_OF_HEIGHT = {96, 2304};
constexpr OH_AVRange DEFAULT_HEIGHT_RANGE = {96, 4096};
constexpr OH_AVRange DEFAULT_WIDTH_RANGE = {96, 4096};
constexpr OH_AVRange DEFAULT_HEIGHT_RANGE_ENC = {144, 2160};
constexpr OH_AVRange DEFAULT_WIDTH_RANGE_ENC = {176, 4096};
constexpr OH_AVRange DEFAULT_FRAMERATE_RANGE = {0, 120};
constexpr OH_AVRange DEFAULT_BITRATE_RANGE_ENC = {10000, 100000000};
constexpr OH_AVRange DEFAULT_VIDEO_QUALITY_RANGE = {0, 100};

const std::vector<int32_t> DEFAULT_AUDIO_ACC_SAMPLES = {8000,  11025, 12000, 16000, 22050, 24000,
                                                        32000, 44100, 48000, 64000, 88200, 96000};
const std::vector<int32_t> DEFAULT_VIDEO_AVC_PIXFORMATS = {
    static_cast<int32_t>(VideoPixelFormat::YUVI420), static_cast<int32_t>(VideoPixelFormat::NV12),
    static_cast<int32_t>(VideoPixelFormat::NV21), static_cast<int32_t>(VideoPixelFormat::RGBA)};
const std::vector<int32_t> DEFAULT_VIDEO_AVC_PROFILES = {AVC_PROFILE_BASELINE, AVC_PROFILE_HIGH, AVC_PROFILE_MAIN};
const std::vector<int32_t> DEFAULT_VIDEO_AVC_LEVELS = {
    AVC_LEVEL_1, AVC_LEVEL_1b, AVC_LEVEL_11, AVC_LEVEL_12, AVC_LEVEL_13, AVC_LEVEL_2,  AVC_LEVEL_21, AVC_LEVEL_22,
    AVC_LEVEL_3, AVC_LEVEL_31, AVC_LEVEL_32, AVC_LEVEL_4,  AVC_LEVEL_41, AVC_LEVEL_42, AVC_LEVEL_5,  AVC_LEVEL_51};

constexpr int32_t DEFAULT_WIDTH_ALIGNMENT = 2;
constexpr int32_t DEFAULT_HEIGHT_ALIGNMENT = 2;
constexpr int32_t DEFAULT_ALIGNMENT_ENC = 4;
constexpr int32_t DEFAULT_BITRATEMODE_ENC = 3;

constexpr int32_t DEFAULT_FRAMERATE = 1;
constexpr int32_t DEFAULT_WIDTH = 1920;
constexpr int32_t DEFAULT_HEIGHT = 1080;
constexpr int32_t DEFAULT_VIDEO_AVC_PROFILE = AVC_PROFILE_HIGH;
constexpr int32_t DEFAULT_LEVEL = AVC_LEVEL_1;
constexpr int32_t ERROR_FRAMERATE = 121;
constexpr int32_t ERROR_WIDTH = 95;
constexpr int32_t ERROR_HEIGHT = 95;
constexpr int32_t ERROR_VIDEO_AVC_PROFILE = -1;
constexpr int32_t ERROR_LEVEL = -1;

constexpr uint32_t MAX_VIDEO_BITRATE = 300000000;
constexpr uint32_t MAX_AUDIO_BITRATE = 320000;
constexpr uint32_t MIN_AUDIO_BITRATE = 32000;
constexpr uint32_t MAX_AUDIO_BITRATE_AAC = 448000;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 8000;
constexpr uint32_t DEFAULT_SAMPLE_RATE_SIZE = 12;
constexpr uint32_t MAX_CHANNEL_COUNT = 8;
constexpr uint32_t MAX_CHANNEL_COUNT_MP3 = 2;
constexpr int AUDIO_MIN_BIT_RATE_VIVID_DECODER = 16000;
constexpr int AUDIO_MAX_BIT_RATE_VIVID_DECODER = 3075000;
constexpr int AUDIO_MAX_CHANNEL_COUNT_VIVID = 16;
constexpr int AUDIO_SAMPLE_RATE_COUNT_VIVID = 5;

constexpr int32_t STACK_BUF_OVERWRITE_MAX_BUF_SIZE = 100;
constexpr int32_t STACK_BUF_OVERWRITE_TIMES = 1000;

const std::vector<std::string> videoDecoderList = {std::string(CodecMimeType::VIDEO_AVC)};

const std::vector<std::string> videoEncoderList = {std::string(CodecMimeType::VIDEO_AVC)};

const std::vector<std::string> audioDecoderList = {
    std::string(CodecMimeType::AUDIO_MPEG), std::string(CodecMimeType::AUDIO_AAC),
    std::string(CodecMimeType::AUDIO_VORBIS), std::string(CodecMimeType::AUDIO_FLAC)};

const std::vector<std::string> audioEncoderList = {std::string(CodecMimeType::AUDIO_AAC)};
} // namespace CodecListTestParam
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_MOCK_H