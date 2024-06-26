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

#ifndef MEDIA_AVCODEC_INFO_H
#define MEDIA_AVCODEC_INFO_H

#include <cstdint>
#include <memory>
#include <vector>
#include "av_common.h"
#include "nocopyable.h"
#include "avcodec_audio_common.h"

namespace OHOS {
namespace MediaAVCodec {
/**
 * @brief AVCodec Type
 *
 * @since 3.1
 * @version 4.0
 */
enum AVCodecType : int32_t {
    AVCODEC_TYPE_NONE = -1,
    AVCODEC_TYPE_VIDEO_ENCODER = 0,
    AVCODEC_TYPE_VIDEO_DECODER,
    AVCODEC_TYPE_AUDIO_ENCODER,
    AVCODEC_TYPE_AUDIO_DECODER,
};

/**
 * @brief AVCodec Category
 *
 * @since 3.1
 * @version 4.0
 */
enum class AVCodecCategory : int32_t {
    AVCODEC_NONE = -1,
    AVCODEC_HARDWARE = 0,
    AVCODEC_SOFTWARE,
};

/**
 * @brief The enum of optional features that can be used in specific codec seenarios.
 *
 * @since 5.0
 * @version 5.0
 */
enum class AVCapabilityFeature : int32_t {
    VIDEO_ENCODER_TEMPORAL_SCALABILITY = 0,
    VIDEO_ENCODER_LONG_TERM_REFERENCE = 1,
    VIDEO_LOW_LATENCY = 2,
    MAX_VALUE
};

/**
 * @brief Range contain min and max value
 *
 * @since 3.1
 * @version 5.0
 */
struct Range {
    int32_t minVal;
    int32_t maxVal;
    Range() : minVal(0), maxVal(0) {}
    Range(const int32_t &min, const int32_t &max)
    {
        if (min <= max) {
            this->minVal = min;
            this->maxVal = max;
        } else {
            this->minVal = 0;
            this->maxVal = 0;
        }
    }

    Range Create(const int32_t &min, const int32_t &max)
    {
        return Range(min, max);
    }

    Range Intersect(const int32_t &min, const int32_t &max)
    {
        int32_t minCmp = this->minVal > min ? this->minVal : min;
        int32_t maxCmp = this->maxVal < max ? this->maxVal : max;
        return this->Create(minCmp, maxCmp);
    }

    Range Intersect(const Range &range)
    {
        int32_t minCmp = this->minVal > range.minVal ? this->minVal : range.minVal;
        int32_t maxCmp = this->maxVal < range.maxVal ? this->maxVal : range.maxVal;
        return this->Create(minCmp, maxCmp);
    }

    bool InRange(int32_t value)
    {
        return (value >= minVal && value <= maxVal);
    }
};

/**
 * @brief ImgSize contain width and height
 *
 * @since 3.1
 * @version 4.0
 */
struct ImgSize {
    int32_t width;
    int32_t height;

    ImgSize() : width(0), height(0) {}

    ImgSize(const int32_t &width, const int32_t &height)
    {
        this->width = width;
        this->height = height;
    }

    bool operator<(const ImgSize &p) const
    {
        return (width < p.width) || (width == p.width && height < p.height);
    }
};

/**
 * @brief Capability Data struct of Codec, parser from config file
 *
 * @since 3.1
 * @version 4.0
 */
struct CapabilityData {
    std::string codecName = "";
    int32_t codecType = AVCODEC_TYPE_NONE;
    std::string mimeType = "";
    bool isVendor = false;
    int32_t maxInstance = 0;
    Range bitrate;
    Range channels;
    Range complexity;
    ImgSize alignment;
    Range width;
    Range height;
    Range frameRate;
    Range encodeQuality;
    Range blockPerFrame;
    Range blockPerSecond;
    ImgSize blockSize;
    std::vector<int32_t> sampleRate;
    std::vector<int32_t> pixFormat;
    std::vector<int32_t> bitDepth;
    std::vector<int32_t> profiles;
    std::vector<int32_t> bitrateMode;
    std::map<int32_t, std::vector<int32_t>> profileLevelsMap;
    std::map<ImgSize, Range> measuredFrameRate;
    bool supportSwapWidthHeight = false;
    std::map<int32_t, Format> featuresMap;
};

struct LevelParams {
    int32_t maxBlockPerFrame = 0;
    int32_t maxBlockPerSecond = 0;
    int32_t maxFrameRate = 0;
    int32_t maxWidth = 0;
    int32_t maxHeight = 0;
    LevelParams(const int32_t &blockPerSecond, const int32_t &blockPerFrame, const int32_t &frameRate,
                const int32_t &width, const int32_t height)
    {
        this->maxBlockPerFrame = blockPerFrame;
        this->maxBlockPerSecond = blockPerSecond;
        this->maxFrameRate = frameRate;
        this->maxWidth = width;
        this->maxHeight = height;
    }
    LevelParams(const int32_t &blockPerSecond, const int32_t &blockPerFrame)
    {
        this->maxBlockPerFrame = blockPerFrame;
        this->maxBlockPerSecond = blockPerSecond;
    }
};

class __attribute__((visibility("default"))) AVCodecInfo {
public:
    explicit AVCodecInfo(CapabilityData *capabilityData);
    ~AVCodecInfo();

    /**
     * @brief Get name of this codec, used to create the codec instance.
     * @return Returns codec name.
     * @since 3.1
     * @version 4.0
     */
    std::string GetName();

    /**
     * @brief Get type of this codec
     * @return Returns codec type, see {@link AVCodecType}
     * @since 3.1
     * @version 4.0
     */
    AVCodecType GetType();

    /**
     * @brief Get mime type of this codec
     * @return Returns codec mime type, see {@link CodecMimeType}
     * @since 3.1
     * @version 4.0
     */
    std::string GetMimeType();

    /**
     * @brief Check whether the codec is accelerated by hardware.
     * @return Returns true if the codec is hardware accelerated; false otherwise.
     * @since 3.1
     * @version 4.0
     */
    bool IsHardwareAccelerated();

    /**
     * @brief Check whether the codec is accelerated by hardware.
     * @return Returns true if the codec is hardware accelerated; false otherwise.
     * @since 3.1
     * @version 4.0
     */
    int32_t GetMaxSupportedInstances();

    /**
     * @brief Check whether the codec is software implemented only.
     * @return Returns true if the codec is software implemented only; false otherwise.
     * @since 3.1
     * @version 4.0
     */
    bool IsSoftwareOnly();

    /**
     * @brief Check whether the codec is provided by vendor.
     * @return Returns true if the codec is provided by vendor; false otherwise.
     * @since 3.1
     * @version 4.0
     */
    bool IsVendor();

    /**
     * @brief Get supported codec profile number.
     * @return Returns an array of supported codec profile number. For details, see {@link AACProfile}.
     * @since 3.1
     * @version 4.0
     */
    std::map<int32_t, std::vector<int32_t>> GetSupportedLevelsForProfile();

    /**
     * @brief Check if the codec supports a specified feature.
     * @param feature Feature enum, refer to {@link AVCapabilityFeature} for details
     * @return Returns true if the feature is supported, false if it is not supported
     * @since 5.0
     * @version 5.0
     */
    bool IsFeatureSupported(AVCapabilityFeature feature);
    
    /**
     * @brief Get the properties of a specified feature.
     * @param feature Feature enum, refer to {@link AVCapabilityFeature} for details
     * @param format Output parameter, get parametr of specified feature
     * @return Returns {@link AVCS_ERR_OK} if success, returns an error code otherwise
     * @since 5.0
     * @version 5.0
     */
    int32_t GetFeatureProperties(AVCapabilityFeature feature, Format &format);

private:
    bool IsFeatureValid(AVCapabilityFeature feature);
    CapabilityData *data_;
};

class __attribute__((visibility("default"))) VideoCaps {
public:
    explicit VideoCaps(CapabilityData *capabilityData);
    ~VideoCaps();

    /**
     * @brief Get codec information,  such as the codec name, codec type,
     * whether hardware acceleration is supported, whether only software is supported,
     * and whether the codec is provided by the vendor.
     * @return Returns the pointer of {@link AVCodecInfo}.
     * @since 3.1
     * @version 4.0
     */
    std::shared_ptr<AVCodecInfo> GetCodecInfo();

    /**
     * @brief Get supported bitrate range.
     * @return Returns the range of supported bitrates.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedBitrate();

    /**
     * @brief Get supported video raw formats.
     * @return Returns an array of supported formats. For Details, see {@link VideoPixelFormat}.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedFormats();

    /**
     * @brief Get supported alignment of video height, only used for video codecs.
     * @return Returns the supported alignment of video height (in pixels).
     * @since 3.1
     * @version 4.0
     */
    int32_t GetSupportedHeightAlignment();

    /**
     * @brief Get supported alignment of video width, only used for video codecs.
     * @return Returns the supported alignment of video width (in pixels).
     * @since 3.1
     * @version 4.0
     */
    int32_t GetSupportedWidthAlignment();

    /**
     * @brief Get supported width range of video.
     * @return Returns the supported width range of video.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedWidth();

    /**
     * @brief Get supported height range of video.
     * @return Returns the supported height range of video.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedHeight();

    /**
     * @brief Get supported profiles of this codec.
     * @return Returns an array of supported profiles:
     * returns {@link H263Profile} array if codec is h263,
     * returns {@link AVCProfile} array if codec is h264,
     * returns {@link HEVCProfile} array if codec is h265,
     * returns {@link MPEG2Profile} array if codec is mpeg2,
     * returns {@link MPEG4Profile} array if codec is mpeg4,
     * returns {@link VP8Profile} array if codec is vp8.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedProfiles();

    /**
     * @brief Get supported codec level array.
     * @return Returns an array of supported codec level number.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedLevels();

    /**
     * @brief Get supported video encode quality Range.
     * @return Returns an array of supported video encode quality Range.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedEncodeQuality();

    /**
     * @brief Check whether the width and height is supported.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @return Returns true if the codec supports {@link width} * {@link height} size video, false otherwise.
     * @since 3.1
     * @version 4.0
     */
    bool IsSizeSupported(int32_t width, int32_t height);

    /**
     * @brief Get supported frameRate.
     * @return Returns the supported frameRate range of video.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedFrameRate();

    /**
     * @brief Get supported frameRate range for the specified width and height.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @return Returns the supported frameRate range for the specified width and height.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedFrameRatesFor(int32_t width, int32_t height);

    /**
     * @brief Check whether the size and frameRate is supported.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @param frameRate Indicates the specified video frameRate.
     * @return Returns true if the codec supports the specified size and frameRate; false otherwise.
     * @since 3.1
     * @version 4.0
     */
    bool IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate);

    /**
     * @brief Get preferred frameRate range for the specified width and height,
     * these framerates can be reach the performance.
     * @param width Indicates the specified video width (in pixels).
     * @param height Indicates the specified video height (in pixels).
     * @return Returns preferred frameRate range for the specified width and height.
     * @since 3.1
     * @version 4.0
     */
    Range GetPreferredFrameRate(int32_t width, int32_t height);

    /**
     * @brief Get supported encode bitrate mode.
     * @return Returns an array of supported encode bitrate mode. For details, see {@link VideoEncodeBitrateMode}.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedBitrateMode();

    /**
     * @brief Get supported encode qualit range.
     * @return Returns supported encode qualit range.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedQuality();

    /**
     * @brief Get supported encode complexity range.
     * @return Returns supported encode complexity range.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedComplexity();

    /**
     * @brief Check video encoder wether support request key frame dynamicly.
     * @return Returns true if support, false not support.
     * @since 3.1
     * @version 4.0
     */
    bool IsSupportDynamicIframe();

    Range GetVideoHeightRangeForWidth(int32_t width);
    Range GetVideoWidthRangeForHeight(int32_t height);

private:
    CapabilityData *data_;
    int32_t blockWidth_;
    int32_t blockHeight_;
    Range horizontalBlockRange_;
    Range verticalBlockRange_;
    Range blockPerFrameRange_;
    Range blockPerSecondRange_;
    Range widthRange_;
    Range heightRange_;
    Range frameRateRange_;
    void InitParams();
    void UpdateParams();
    void LoadLevelParams();
    void LoadAVCLevelParams();
    void LoadMPEGLevelParams(const std::string &mime);
    ImgSize MatchClosestSize(const ImgSize &imgSize);
    int32_t DivCeil(const int32_t &dividend, const int32_t &divisor);
    Range DivRange(const Range &range, const int32_t &divisor);
    void UpdateBlockParams(const int32_t &blockWidth, const int32_t &blockHeight, Range &blockPerFrameRange,
                           Range &blockPerSecondRange);
};

constexpr uint32_t MAX_MAP_SIZE = 20;

class __attribute__((visibility("default"))) AudioCaps {
public:
    explicit AudioCaps(CapabilityData *capabilityData);
    ~AudioCaps();

    /**
     * @brief Get codec information,  such as the codec name, codec type,
     * whether hardware acceleration is supported, whether only software is supported,
     * and whether the codec is provided by the vendor.
     * @return Returns the pointer of {@link AVCodecInfo}
     * @since 3.1
     * @version 4.0
     */
    std::shared_ptr<AVCodecInfo> GetCodecInfo();

    /**
     * @brief Get supported bitrate range.
     * @return Returns the range of supported bitrates.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedBitrate();

    /**
     * @brief Get supported channel range.
     * @return Returns the range of supported channel.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedChannel();

    /**
     * @brief Get supported audio raw format range.
     * @return Returns the range of supported audio raw format. For details, see {@link AudioSampleFormat}.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedFormats();

    /**
     * @brief Get supported audio samplerates.
     * @return Returns an array of supported samplerates.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedSampleRates();

    /**
     * @brief Get supported codec profile number.
     * @return Returns an array of supported codec profile number. For details, see {@link AACProfile}.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedProfiles();

    /**
     * @brief Get supported codec level array.
     * @return Returns an array of supported codec level number.
     * @since 3.1
     * @version 4.0
     */
    std::vector<int32_t> GetSupportedLevels();

    /**
     * @brief Get supported encode complexity range.
     * @return Returns supported encode complexity range.
     * @since 3.1
     * @version 4.0
     */
    Range GetSupportedComplexity();

private:
    CapabilityData *data_;
};

/**
 * @brief Enumerates the codec mime type.
 */
class CodecMimeType {
public:
    static constexpr std::string_view VIDEO_H263 = "video/h263";
    static constexpr std::string_view VIDEO_AVC = "video/avc";
    static constexpr std::string_view VIDEO_MPEG2 = "video/mpeg2";
    static constexpr std::string_view VIDEO_HEVC = "video/hevc";
    static constexpr std::string_view VIDEO_MPEG4 = "video/mp4v-es";
    static constexpr std::string_view VIDEO_VP8 = "video/x-vnd.on2.vp8";
    static constexpr std::string_view VIDEO_VP9 = "video/x-vnd.on2.vp9";
    static constexpr std::string_view AUDIO_AMR_NB = "audio/3gpp";
    static constexpr std::string_view AUDIO_AMR_WB = "audio/amr-wb";
    static constexpr std::string_view AUDIO_MPEG = "audio/mpeg";
    static constexpr std::string_view AUDIO_AAC = "audio/mp4a-latm";
    static constexpr std::string_view AUDIO_VORBIS = "audio/vorbis";
    static constexpr std::string_view AUDIO_OPUS = "audio/opus";
    static constexpr std::string_view AUDIO_FLAC = "audio/flac";
    static constexpr std::string_view AUDIO_RAW = "audio/raw";
    static constexpr std::string_view AUDIO_G711MU = "audio/g711mu";
    static constexpr std::string_view IMAGE_JPG = "image/jpeg";
    static constexpr std::string_view IMAGE_PNG = "image/png";
    static constexpr std::string_view IMAGE_BMP = "image/bmp";
    static constexpr std::string_view AUDIO_AVS3DA = "audio/av3a";
    static constexpr std::string_view AUDIO_LBVC = "audio/lbvc";
    static constexpr std::string_view AUDIO_APE = "audio/x-ape";
};

/**
 * @brief AVC Profile
 *
 * @since 3.1
 * @version 4.0
 */
enum AVCProfile : int32_t {
    AVC_PROFILE_BASELINE = 0,
    AVC_PROFILE_CONSTRAINED_BASELINE = 1,
    AVC_PROFILE_CONSTRAINED_HIGH = 2,
    AVC_PROFILE_EXTENDED = 3,
    AVC_PROFILE_HIGH = 4,
    AVC_PROFILE_HIGH_10 = 5,
    AVC_PROFILE_HIGH_422 = 6,
    AVC_PROFILE_HIGH_444 = 7,
    AVC_PROFILE_MAIN = 8,
};

/**
 * @brief HEVC Profile
 *
 * @since 3.1
 * @version 4.0
 */
enum HEVCProfile : int32_t {
    HEVC_PROFILE_MAIN = 0,
    HEVC_PROFILE_MAIN_10 = 1,
    HEVC_PROFILE_MAIN_STILL = 2,
    HEVC_PROFILE_MAIN_10_HDR10 = 3,
    HEVC_PROFILE_MAIN_10_HDR10_PLUS = 4,
    HEVC_PROFILE_UNKNOW = -1,
};

/**
 * @brief MPEG2 Profile
 *
 * @since 3.1
 * @version 4.0
 */
enum MPEG2Profile : int32_t {
    MPEG2_PROFILE_422 = 0,
    MPEG2_PROFILE_HIGH = 1,
    MPEG2_PROFILE_MAIN = 2,
    MPEG2_PROFILE_SNR = 3,
    MPEG2_PROFILE_SIMPLE = 4,
    MPEG2_PROFILE_SPATIAL = 5,
};

/**
 * @brief MPEG4 Profile
 *
 * @since 3.1
 * @version 4.0
 */
enum MPEG4Profile : int32_t {
    MPEG4_PROFILE_ADVANCED_CODING = 0,
    MPEG4_PROFILE_ADVANCED_CORE = 1,
    MPEG4_PROFILE_ADVANCED_REAL_TIME = 2,
    MPEG4_PROFILE_ADVANCED_SCALABLE = 3,
    MPEG4_PROFILE_ADVANCED_SIMPLE = 4,
    MPEG4_PROFILE_BASIC_ANIMATED = 5,
    MPEG4_PROFILE_CORE = 6,
    MPEG4_PROFILE_CORE_SCALABLE = 7,
    MPEG4_PROFILE_HYBRID = 8,
    MPEG4_PROFILE_MAIN = 9,
    MPEG4_PROFILE_NBIT = 10,
    MPEG4_PROFILE_SCALABLE_TEXTURE = 11,
    MPEG4_PROFILE_SIMPLE = 12,
    MPEG4_PROFILE_SIMPLE_FBA = 13,
    MPEG4_PROFILE_SIMPLE_FACE = 14,
    MPEG4_PROFILE_SIMPLE_SCALABLE = 15,
};

/**
 * @brief H263 Profile
 *
 * @since 3.1
 * @version 4.0
 */
enum H263Profile : int32_t {
    H263_PROFILE_BACKWARD_COMPATIBLE = 0,
    H263_PROFILE_BASELINE = 1,
    H263_PROFILE_H320_CODING = 2,
    H263_PROFILE_HIGH_COMPRESSION = 3,
    H263_PROFILE_HIGH_LATENCY = 4,
    H263_PROFILE_ISW_V2 = 5,
    H263_PROFILE_ISW_V3 = 6,
    H263_PROFILE_INTERLACE = 7,
    H263_PROFILE_INTERNET = 8,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 4.0
 */
enum VP8Profile : int32_t {
    VP8_PROFILE_MAIN = 0,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 4.0
 */
enum AVCLevel : int32_t {
    AVC_LEVEL_1 = 0,
    AVC_LEVEL_1b = 1,
    AVC_LEVEL_11 = 2,
    AVC_LEVEL_12 = 3,
    AVC_LEVEL_13 = 4,
    AVC_LEVEL_2 = 5,
    AVC_LEVEL_21 = 6,
    AVC_LEVEL_22 = 7,
    AVC_LEVEL_3 = 8,
    AVC_LEVEL_31 = 9,
    AVC_LEVEL_32 = 10,
    AVC_LEVEL_4 = 11,
    AVC_LEVEL_41 = 12,
    AVC_LEVEL_42 = 13,
    AVC_LEVEL_5 = 14,
    AVC_LEVEL_51 = 15,
    AVC_LEVEL_52 = 16,
    AVC_LEVEL_6 = 17,
    AVC_LEVEL_61 = 18,
    AVC_LEVEL_62 = 19,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 4.0
 */
enum HEVCLevel : int32_t {
    HEVC_LEVEL_1 = 0,
    HEVC_LEVEL_2 = 1,
    HEVC_LEVEL_21 = 2,
    HEVC_LEVEL_3 = 3,
    HEVC_LEVEL_31 = 4,
    HEVC_LEVEL_4 = 5,
    HEVC_LEVEL_41 = 6,
    HEVC_LEVEL_5 = 7,
    HEVC_LEVEL_51 = 8,
    HEVC_LEVEL_52 = 9,
    HEVC_LEVEL_6 = 10,
    HEVC_LEVEL_61 = 11,
    HEVC_LEVEL_62 = 12,
    HEVC_LEVEL_UNKNOW = -1,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 4.0
 */
enum MPEG2Level : int32_t {
    MPEG2_LEVEL_LL = 0,
    MPEG2_LEVEL_ML = 1,
    MPEG2_LEVEL_H14 = 2,
    MPEG2_LEVEL_HL = 3,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 4.0
 */
enum MPEG4Level : int32_t {
    MPEG4_LEVEL_0 = 0,
    MPEG4_LEVEL_0B = 1,
    MPEG4_LEVEL_1 = 2,
    MPEG4_LEVEL_2 = 3,
    MPEG4_LEVEL_3 = 4,
    MPEG4_LEVEL_4 = 5,
    MPEG4_LEVEL_4A = 6,
    MPEG4_LEVEL_5 = 7,
};

/**
 * @brief
 *
 * @since 3.1
 * @version 4.0
 */
enum VideoEncodeBitrateMode : int32_t {
    /**
     * constant bit rate mode.
     */
    CBR = 0,
    /**
     * variable bit rate mode.
     */
    VBR = 1,
    /**
     * constant quality mode.
     */
    CQ = 2,
};

/**
 * @brief File type
 *
 * @since 4.0
 * @version 4.0
 */
enum FileType : int32_t {
    FILE_TYPE_UNKNOW = 0,
    FILE_TYPE_MP4 = 101,
    FILE_TYPE_MPEGTS = 102,
    FILE_TYPE_MKV = 103,
    FILE_TYPE_AMR = 201,
    FILE_TYPE_AAC = 202,
    FILE_TYPE_MP3 = 203,
    FILE_TYPE_FLAC = 204,
    FILE_TYPE_OGG = 205,
    FILE_TYPE_M4A = 206,
    FILE_TYPE_WAV = 207,
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // MEDIA_AVCODEC_INFO_H