/*
 * Copyright (C) 2021 Huawei Device Co., Ltd.
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

#include <cmath>
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, "AVCodecInfo"};
constexpr int32_t FRAME_RATE_30 = 30;
constexpr int32_t BLOCK_SIZE_MIN = 2;
constexpr int32_t BASE_BLOCK_PER_FRAME = 99;
constexpr int32_t BASE_BLOCK_PER_SECOND = 1485;
} // namespace
namespace OHOS {
namespace MediaAVCodec {
const std::map<int32_t, LevelParams> AVC_PARAMS_MAP = {
    {AVC_LEVEL_1, LevelParams(1485, 99)},      {AVC_LEVEL_1b, LevelParams(1485, 99)},
    {AVC_LEVEL_11, LevelParams(3000, 396)},    {AVC_LEVEL_12, LevelParams(6000, 396)},
    {AVC_LEVEL_13, LevelParams(11880, 396)},   {AVC_LEVEL_2, LevelParams(11880, 396)},
    {AVC_LEVEL_21, LevelParams(19800, 792)},   {AVC_LEVEL_22, LevelParams(20250, 1620)},
    {AVC_LEVEL_3, LevelParams(40500, 1620)},   {AVC_LEVEL_31, LevelParams(108000, 3600)},
    {AVC_LEVEL_32, LevelParams(216000, 5120)}, {AVC_LEVEL_4, LevelParams(245760, 8192)},
    {AVC_LEVEL_41, LevelParams(245760, 8192)}, {AVC_LEVEL_42, LevelParams(522240, 8704)},
    {AVC_LEVEL_5, LevelParams(589824, 22080)}, {AVC_LEVEL_51, LevelParams(983040, 36864)},
    {AVC_LEVEL_52, LevelParams(2073600, 36864)}, {AVC_LEVEL_6, LevelParams(4177920, 139264)},
    {AVC_LEVEL_61, LevelParams(8355840, 139264)}, {AVC_LEVEL_62, LevelParams(16711680, 139264)},
};

const std::map<int32_t, LevelParams> MPEG2_SIMPLE_PARAMS_MAP = {
    {MPEG2_LEVEL_ML, LevelParams(40500, 1620, 30, 45, 36)},
};

const std::map<int32_t, LevelParams> MPEG2_MAIN_PARAMS_MAP = {
    {MPEG2_LEVEL_LL, LevelParams(11880, 396, 30, 22, 18)},
    {MPEG2_LEVEL_ML, LevelParams(40500, 1620, 30, 45, 36)},
    {MPEG2_LEVEL_H14, LevelParams(183600, 6120, 60, 90, 68)},
    {MPEG2_LEVEL_HL, LevelParams(244800, 8160, 60, 120, 68)},
};

const std::map<int32_t, LevelParams> MPEG4_ADVANCED_SIMPLE_PARAMS_MAP = {
    {MPEG4_LEVEL_0, LevelParams(2970, 99, 30, 11, 9)},     {MPEG4_LEVEL_1, LevelParams(2970, 99, 30, 11, 9)},
    {MPEG4_LEVEL_2, LevelParams(5940, 396, 30, 22, 18)},   {MPEG4_LEVEL_3, LevelParams(11880, 396, 30, 22, 18)},
    {MPEG4_LEVEL_4, LevelParams(23760, 1200, 30, 44, 36)}, {MPEG4_LEVEL_5, LevelParams(48600, 1620, 30, 45, 36)},
};

const std::map<int32_t, LevelParams> MPEG4_SIMPLE_PARAMS_MAP = {
    {MPEG4_LEVEL_0, LevelParams(1485, 99, 15, 11, 9)},     {MPEG4_LEVEL_0B, LevelParams(1485, 99, 15, 11, 9)},
    {MPEG4_LEVEL_1, LevelParams(1485, 99, 30, 11, 9)},     {MPEG4_LEVEL_2, LevelParams(5940, 396, 30, 22, 18)},
    {MPEG4_LEVEL_3, LevelParams(11880, 396, 30, 22, 18)},  {MPEG4_LEVEL_4A, LevelParams(36000, 1200, 30, 40, 30)},
    {MPEG4_LEVEL_5, LevelParams(40500, 1620, 30, 40, 36)},
};

VideoCaps::VideoCaps(CapabilityData *capabilityData) : data_(capabilityData)
{
    InitParams();
    LoadLevelParams();
    AVCODEC_LOGD("VideoCaps:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

VideoCaps::~VideoCaps()
{
    AVCODEC_LOGD("VideoCaps:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::shared_ptr<AVCodecInfo> VideoCaps::GetCodecInfo()
{
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(data_);
    CHECK_AND_RETURN_RET_LOG(codecInfo != nullptr, nullptr, "create codecInfo failed");

    return codecInfo;
}

Range VideoCaps::GetSupportedBitrate()
{
    return data_->bitrate;
}

std::vector<int32_t> VideoCaps::GetSupportedFormats()
{
    std::vector<int32_t> pixFormat = data_->pixFormat;
    CHECK_AND_RETURN_RET_LOG(pixFormat.size() != 0, pixFormat, "GetSupportedFormats failed: format is null");
    return pixFormat;
}

int32_t VideoCaps::GetSupportedHeightAlignment()
{
    return data_->alignment.height;
}

int32_t VideoCaps::GetSupportedWidthAlignment()
{
    return data_->alignment.width;
}

Range VideoCaps::GetSupportedWidth()
{
    return data_->width;
}

Range VideoCaps::GetSupportedHeight()
{
    return data_->height;
}

std::vector<int32_t> VideoCaps::GetSupportedProfiles()
{
    std::vector<int32_t> profiles = data_->profiles;
    CHECK_AND_RETURN_RET_LOG(profiles.size() != 0, profiles, "GetSupportedProfiles failed: profiles is null");
    return profiles;
}

std::vector<int32_t> VideoCaps::GetSupportedLevels()
{
    std::vector<int32_t> levels;
    return levels;
}

Range VideoCaps::GetSupportedEncodeQuality()
{
    return data_->encodeQuality;
}

bool VideoCaps::IsSizeSupported(int32_t width, int32_t height)
{
    UpdateParams();
    if (width <= 0 || height <= 0) {
        return false;
    }
    if (data_->alignment.width == 0 || data_->alignment.height == 0 || width % data_->alignment.width != 0 ||
        height % data_->alignment.height != 0) {
        return false;
    }
    if (data_->width.minVal > width || data_->width.maxVal < width || data_->height.minVal > height ||
        data_->height.maxVal < height) {
        return false;
    }
    if (blockWidth_ != 0 && blockHeight_ != 0) {
        int64_t blockPerFrame =
            static_cast<int64_t>(DivCeil(width, blockWidth_)) * static_cast<int64_t>(DivCeil(height, blockHeight_));
        if (blockPerFrame < blockPerFrameRange_.minVal || blockPerFrame > blockPerFrameRange_.maxVal) {
            return false;
        }
    }
    return true;
}

Range VideoCaps::GetVideoWidthRangeForHeight(int32_t height)
{
    if (height < data_->height.minVal || height > data_->height.maxVal) {
        return Range(0, 0);
    }
    UpdateParams();
    Range ret = data_->width;
    if (blockWidth_ != 0 && blockHeight_ != 0) {
        int64_t verticalBlockNum = static_cast<int64_t>(DivCeil(height, blockHeight_));
        if (verticalBlockNum < verticalBlockRange_.minVal || verticalBlockNum > verticalBlockRange_.maxVal) {
            return Range(0, 0);
        }
        Range horizontalBlockNum = horizontalBlockRange_.Intersect(
            Range(blockPerFrameRange_.minVal / verticalBlockNum, blockPerFrameRange_.maxVal / verticalBlockNum));
        ret = ret.Intersect(Range(horizontalBlockNum.minVal * blockWidth_, horizontalBlockNum.maxVal * blockWidth_));
    }
    return ret;
}

Range VideoCaps::GetVideoHeightRangeForWidth(int32_t width)
{
    if (width < data_->width.minVal || width > data_->width.maxVal) {
        return Range(0, 0);
    }
    UpdateParams();
    Range ret = data_->height;
    if (blockWidth_ != 0 && blockHeight_ != 0) {
        int64_t horizontalBlockNum = static_cast<int64_t>(DivCeil(width, blockWidth_));
        if (horizontalBlockNum < horizontalBlockRange_.minVal || horizontalBlockNum > horizontalBlockRange_.maxVal) {
            return Range(0, 0);
        }
        Range verticalBlockNum = verticalBlockRange_.Intersect(
            Range(blockPerFrameRange_.minVal / horizontalBlockNum, blockPerFrameRange_.maxVal / horizontalBlockNum));
        ret = ret.Intersect(Range(verticalBlockNum.minVal * blockHeight_, verticalBlockNum.maxVal * blockHeight_));
    }
    return ret;
}

Range VideoCaps::GetSupportedFrameRate()
{
    return data_->frameRate;
}

Range VideoCaps::GetSupportedFrameRatesFor(int32_t width, int32_t height)
{
    if (!IsSizeSupported(width, height)) {
        AVCODEC_LOGD("The %{public}s can not support of:%{public}d * %{public}d", data_->codecName.c_str(), width,
                     height);
        return Range(0, 0);
    }
    Range frameRatesRange;
    int64_t blockPerFrame = DivCeil(width, blockWidth_) * static_cast<int64_t>(DivCeil(height, blockHeight_));
    if (blockPerFrame != 0) {
        frameRatesRange =
            Range(std::max(static_cast<int32_t>(blockPerSecondRange_.minVal / blockPerFrame), frameRateRange_.minVal),
                  std::min(static_cast<int32_t>(blockPerSecondRange_.maxVal / blockPerFrame), frameRateRange_.maxVal));
    }
    if (frameRatesRange.minVal > frameRatesRange.maxVal) {
        return Range(0, 0);
    }
    return frameRatesRange;
}

void VideoCaps::LoadLevelParams()
{
    if (this->GetCodecInfo()->IsSoftwareOnly()) {
        return;
    }
    if (data_->mimeType == CodecMimeType::VIDEO_AVC) {
        LoadAVCLevelParams();
    } else {
        LoadMPEGLevelParams(data_->mimeType);
    }
}

void VideoCaps::LoadAVCLevelParams()
{
    int32_t maxBlockPerFrame = BASE_BLOCK_PER_FRAME;
    int32_t maxBlockPerSecond = BASE_BLOCK_PER_SECOND;
    for (auto iter = data_->profileLevelsMap.begin(); iter != data_->profileLevelsMap.end(); iter++) {
        for (auto levelIter = iter->second.begin(); levelIter != iter->second.end(); levelIter++) {
            if (AVC_PARAMS_MAP.find(*levelIter) != AVC_PARAMS_MAP.end()) {
                maxBlockPerFrame = std::max(maxBlockPerFrame, AVC_PARAMS_MAP.at(*levelIter).maxBlockPerFrame);
                maxBlockPerSecond = std::max(maxBlockPerSecond, AVC_PARAMS_MAP.at(*levelIter).maxBlockPerSecond);
            }
        }
    }
    Range blockPerFrameRange = Range(1, maxBlockPerFrame);
    Range blockPerSecondRange = Range(1, maxBlockPerSecond);
    UpdateBlockParams(16, 16, blockPerFrameRange, blockPerSecondRange); // set AVC block size as 16x16
}

void VideoCaps::LoadMPEGLevelParams(const std::string &mime)
{
    std::map<int32_t, LevelParams> PARAMS_MAP;
    bool isMpeg2 = false;
    if (mime == CodecMimeType::VIDEO_MPEG2) {
        isMpeg2 = true;
    } else if (mime != CodecMimeType::VIDEO_MPEG4) {
        return;
    }
    int32_t firstSample = isMpeg2 ? MPEG2_PROFILE_SIMPLE : MPEG4_PROFILE_SIMPLE;
    int32_t secondSample = isMpeg2 ? MPEG2_PROFILE_MAIN : MPEG4_PROFILE_ADVANCED_SIMPLE;
    int32_t maxBlockPerFrame = BASE_BLOCK_PER_FRAME;
    int32_t maxBlockPerSecond = BASE_BLOCK_PER_SECOND;
    int32_t maxFrameRate = 0;
    int32_t maxWidth = 0;
    int32_t maxHeight = 0;
    for (auto iter = data_->profileLevelsMap.begin(); iter != data_->profileLevelsMap.end(); iter++) {
        if (iter->first == firstSample) {
            PARAMS_MAP = isMpeg2 ? MPEG2_SIMPLE_PARAMS_MAP : MPEG4_SIMPLE_PARAMS_MAP;
        } else if (iter->first == secondSample) {
            PARAMS_MAP = isMpeg2 ? MPEG2_MAIN_PARAMS_MAP : MPEG4_ADVANCED_SIMPLE_PARAMS_MAP;
        } else {
            continue;
        }
        for (auto levelIter = iter->second.begin(); levelIter != iter->second.end(); levelIter++) {
            if (PARAMS_MAP.find(*levelIter) != PARAMS_MAP.end()) {
                maxBlockPerFrame = std::max(maxBlockPerFrame, PARAMS_MAP.at(*levelIter).maxBlockPerFrame);
                maxBlockPerSecond = std::max(maxBlockPerSecond, PARAMS_MAP.at(*levelIter).maxBlockPerSecond);
                maxFrameRate = std::max(maxFrameRate, PARAMS_MAP.at(*levelIter).maxFrameRate);
                maxWidth = std::max(maxWidth, PARAMS_MAP.at(*levelIter).maxWidth);
                maxHeight = std::max(maxHeight, PARAMS_MAP.at(*levelIter).maxHeight);
            }
        }
    }

    frameRateRange_ = frameRateRange_.Intersect(Range(1, maxFrameRate));
    Range blockPerFrameRange = Range(1, maxBlockPerFrame);
    Range blockPerSecondRange = Range(1, maxBlockPerSecond);
    UpdateBlockParams(maxWidth, maxHeight, blockPerFrameRange, blockPerSecondRange);
}

void VideoCaps::UpdateBlockParams(const int32_t &blockWidth, const int32_t &blockHeight, Range &blockPerFrameRange,
                                  Range &blockPerSecondRange)
{
    int32_t factor;
    if (blockWidth > blockWidth_ && blockHeight > blockHeight_) {
        if (blockWidth_ == 0 || blockHeight_ == 0) {
            return;
        }
        factor = blockWidth * blockHeight / blockWidth_ / blockHeight_;
        blockPerFrameRange_ = DivRange(blockPerFrameRange_, factor);
        blockPerSecondRange_ = DivRange(blockPerSecondRange_, factor);
        horizontalBlockRange_ = DivRange(horizontalBlockRange_, blockWidth / blockWidth_);
        verticalBlockRange_ = DivRange(verticalBlockRange_, blockHeight / blockHeight_);
    } else if (blockWidth < blockWidth_ && blockHeight < blockHeight_) {
        if (blockWidth == 0 || blockHeight == 0) {
            return;
        }
        factor = blockWidth_ * blockHeight_ / blockWidth / blockHeight;
        blockPerFrameRange = DivRange(blockPerFrameRange, factor);
        blockPerSecondRange = DivRange(blockPerSecondRange, factor);
    }

    blockWidth_ = std::max(blockWidth_, blockWidth);
    blockHeight_ = std::max(blockHeight_, blockHeight);
    blockPerFrameRange_ = blockPerFrameRange_.Intersect(blockPerFrameRange);
    blockPerSecondRange_ = blockPerSecondRange_.Intersect(blockPerSecondRange);
}

void VideoCaps::InitParams()
{
    if (data_->blockPerSecond.minVal == 0 || data_->blockPerSecond.maxVal == 0) {
        data_->blockPerSecond = Range(1, INT32_MAX);
    }
    if (data_->blockPerFrame.minVal == 0 || data_->blockPerFrame.maxVal == 0) {
        data_->blockPerFrame = Range(1, INT32_MAX);
    }
    if (data_->width.minVal == 0 || data_->width.maxVal == 0) {
        data_->width = Range(1, INT32_MAX);
    }
    if (data_->height.minVal == 0 || data_->height.maxVal == 0) {
        data_->height = Range(1, INT32_MAX);
    }
    if (data_->frameRate.maxVal == 0) {
        data_->frameRate = Range(0, FRAME_RATE_30);
    }
    if (data_->blockSize.width == 0 || data_->blockSize.height == 0) {
        data_->blockSize.width = BLOCK_SIZE_MIN;
        data_->blockSize.height = BLOCK_SIZE_MIN;
    }

    blockWidth_ = data_->blockSize.width;
    blockHeight_ = data_->blockSize.height;
    frameRateRange_ = data_->frameRate;
    horizontalBlockRange_ = Range(1, INT32_MAX);
    verticalBlockRange_ = Range(1, INT32_MAX);
    blockPerFrameRange_ = Range(1, INT32_MAX);
    blockPerSecondRange_ = Range(1, INT32_MAX);
    widthRange_ = Range(1, INT32_MAX);
    heightRange_ = Range(1, INT32_MAX);
}

void VideoCaps::UpdateParams()
{
    if (data_->blockSize.width == 0 || data_->blockSize.height == 0 || blockWidth_ == 0 || blockHeight_ == 0 ||
        verticalBlockRange_.maxVal == 0 || verticalBlockRange_.minVal == 0 || horizontalBlockRange_.maxVal == 0 ||
        horizontalBlockRange_.minVal == 0 || blockPerFrameRange_.minVal == 0 || blockPerFrameRange_.maxVal == 0) {
        AVCODEC_LOGE("Invalid param");
        return;
    }

    int32_t factor = (blockWidth_ * blockHeight_) / (data_->blockSize.width * data_->blockSize.height);

    blockPerFrameRange_ = blockPerFrameRange_.Intersect(DivRange(data_->blockPerFrame, factor));
    blockPerSecondRange_ = blockPerSecondRange_.Intersect(DivRange(data_->blockPerSecond, factor));
    horizontalBlockRange_ = horizontalBlockRange_.Intersect(
        Range(data_->width.minVal / blockWidth_, DivCeil(data_->width.maxVal, blockWidth_)));
    if (verticalBlockRange_.maxVal != 0 && verticalBlockRange_.minVal != 0) {
        horizontalBlockRange_ =
            horizontalBlockRange_.Intersect(Range(blockPerFrameRange_.minVal / verticalBlockRange_.maxVal,
                                                  blockPerFrameRange_.maxVal / verticalBlockRange_.minVal));
    }
    verticalBlockRange_ = verticalBlockRange_.Intersect(
        Range(data_->height.minVal / blockHeight_, DivCeil(data_->height.maxVal, blockHeight_)));
    if (horizontalBlockRange_.maxVal != 0 && horizontalBlockRange_.minVal != 0) {
        verticalBlockRange_ =
            verticalBlockRange_.Intersect(Range(blockPerFrameRange_.minVal / horizontalBlockRange_.maxVal,
                                                blockPerFrameRange_.maxVal / horizontalBlockRange_.minVal));
    }
    blockPerFrameRange_ =
        blockPerFrameRange_.Intersect(Range(horizontalBlockRange_.minVal * verticalBlockRange_.minVal,
                                            horizontalBlockRange_.maxVal * verticalBlockRange_.maxVal));
    blockPerSecondRange_ = blockPerSecondRange_.Intersect(blockPerFrameRange_.minVal * frameRateRange_.minVal,
                                                          blockPerFrameRange_.maxVal * frameRateRange_.maxVal);
    if (blockPerFrameRange_.maxVal != 0 && blockPerFrameRange_.minVal != 0) {
        frameRateRange_ = frameRateRange_.Intersect(blockPerSecondRange_.minVal / blockPerFrameRange_.maxVal,
                                                    blockPerSecondRange_.maxVal / blockPerFrameRange_.minVal);
    }
}

Range VideoCaps::DivRange(const Range &range, const int32_t &divisor)
{
    if (divisor == 0) {
        AVCODEC_LOGD("The denominator cannot be 0");
        return range;
    } else if (divisor == 1) {
        return range;
    }
    return Range(DivCeil(range.minVal, divisor), range.maxVal / divisor);
}

int32_t VideoCaps::DivCeil(const int32_t &dividend, const int32_t &divisor)
{
    if (divisor == 0) {
        AVCODEC_LOGE("The denominator cannot be 0");
        return INT32_MAX;
    }
    return (dividend + divisor - 1) / divisor;
}

bool VideoCaps::IsSizeAndRateSupported(int32_t width, int32_t height, double frameRate)
{
    if (!IsSizeSupported(width, height)) {
        AVCODEC_LOGD("The %{public}s can not support of:%{public}d * %{public}d", data_->codecName.c_str(), width,
                     height);
        return false;
    }
    const auto &frameRateRange = GetSupportedFrameRatesFor(width, height);
    if (frameRateRange.minVal >= frameRate || frameRate > frameRateRange.maxVal) {
        AVCODEC_LOGD("The %{public}s can not support frameRate:%{public}lf", data_->codecName.c_str(), frameRate);
        return false;
    }
    return true;
}

Range VideoCaps::GetPreferredFrameRate(int32_t width, int32_t height)
{
    Range range;
    if (!IsSizeSupported(width, height)) {
        AVCODEC_LOGD("The %{public}s can not support of:%{public}d * %{public}d", data_->codecName.c_str(), width,
                     height);
        return range;
    }

    if (data_->measuredFrameRate.size() == 0) {
        AVCODEC_LOGD("The measuredFrameRate of %{public}s is null:", data_->codecName.c_str());
        return range;
    }
    ImgSize closestSize = MatchClosestSize(ImgSize(width, height));
    if (data_->measuredFrameRate.find(closestSize) == data_->measuredFrameRate.end()) {
        AVCODEC_LOGD("can not match measuredFrameRate of %{public}d x  %{public}d :", width, width);
        return range;
    }
    int64_t targetBlockNum = DivCeil(width, blockWidth_) * static_cast<int64_t>(DivCeil(height, blockHeight_));
    int64_t closestBlockNum =
        DivCeil(closestSize.width, blockWidth_) * static_cast<int64_t>(DivCeil(closestSize.height, blockHeight_));
    Range closestFrameRate = data_->measuredFrameRate.at(closestSize);
    int64_t minTargetBlockNum = 1;
    double factor = static_cast<double>(closestBlockNum) / std::max(targetBlockNum, minTargetBlockNum);
    return Range(closestFrameRate.minVal * factor, closestFrameRate.maxVal * factor);
}

ImgSize VideoCaps::MatchClosestSize(const ImgSize &imgSize)
{
    int64_t targetBlockNum =
        DivCeil(imgSize.width, blockWidth_) * static_cast<int64_t>(DivCeil(imgSize.height, blockHeight_));
    int64_t minDiffBlockNum = INT32_MAX;

    ImgSize closestSize;
    for (auto iter = data_->measuredFrameRate.begin(); iter != data_->measuredFrameRate.end(); iter++) {
        int64_t blockNum =
            DivCeil(iter->first.width, blockWidth_) * static_cast<int64_t>(DivCeil(iter->first.height, blockHeight_));
        int64_t diffBlockNum = abs(targetBlockNum - blockNum);
        if (minDiffBlockNum > diffBlockNum) {
            minDiffBlockNum = diffBlockNum;
            closestSize = iter->first;
        }
    }
    AVCODEC_LOGD("%{public}s: The ClosestSize of %{public}d x %{public}d is %{public}d x %{public}d:",
                 data_->codecName.c_str(), imgSize.width, imgSize.height, closestSize.width, closestSize.height);
    return closestSize;
}

std::vector<int32_t> VideoCaps::GetSupportedBitrateMode()
{
    std::vector<int32_t> bitrateMode = data_->bitrateMode;
    CHECK_AND_RETURN_RET_LOG(bitrateMode.size() != 0, bitrateMode, "GetSupportedBitrateMode failed: get null");
    return bitrateMode;
}

Range VideoCaps::GetSupportedQuality()
{
    Range quality;
    return quality;
}

Range VideoCaps::GetSupportedComplexity()
{
    return data_->complexity;
}

bool VideoCaps::IsSupportDynamicIframe()
{
    return false;
}

AudioCaps::AudioCaps(CapabilityData *capabilityData) : data_(capabilityData)
{
    AVCODEC_LOGD("AudioCaps:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AudioCaps::~AudioCaps()
{
    AVCODEC_LOGD("AudioCaps:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::shared_ptr<AVCodecInfo> AudioCaps::GetCodecInfo()
{
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(data_);
    CHECK_AND_RETURN_RET_LOG(codecInfo != nullptr, nullptr, "create codecInfo failed");
    return codecInfo;
}

Range AudioCaps::GetSupportedBitrate()
{
    return data_->bitrate;
}

Range AudioCaps::GetSupportedChannel()
{
    return data_->channels;
}

std::vector<int32_t> AudioCaps::GetSupportedFormats()
{
    std::vector<int32_t> bitDepth = data_->bitDepth;
    CHECK_AND_RETURN_RET_LOG(bitDepth.size() != 0, bitDepth, "GetSupportedFormats failed: format is null");
    return bitDepth;
}

std::vector<int32_t> AudioCaps::GetSupportedSampleRates()
{
    std::vector<int32_t> sampleRate = data_->sampleRate;
    CHECK_AND_RETURN_RET_LOG(sampleRate.size() != 0, sampleRate, "GetSupportedSampleRates failed: sampleRate is null");
    return sampleRate;
}

std::vector<int32_t> AudioCaps::GetSupportedProfiles()
{
    std::vector<int32_t> profiles = data_->profiles;
    CHECK_AND_RETURN_RET_LOG(profiles.size() != 0, profiles, "GetSupportedProfiles failed: profiles is null");
    return profiles;
}

std::vector<int32_t> AudioCaps::GetSupportedLevels()
{
    std::vector<int32_t> empty;
    return empty;
}

Range AudioCaps::GetSupportedComplexity()
{
    return data_->complexity;
}

AVCodecInfo::AVCodecInfo(CapabilityData *capabilityData) : data_(capabilityData)
{
    AVCODEC_LOGD("AVCodecInfo:0x%{public}06" PRIXPTR " Instances create", FAKE_POINTER(this));
}

AVCodecInfo::~AVCodecInfo()
{
    AVCODEC_LOGD("AVCodecInfo:0x%{public}06" PRIXPTR " Instances destroy", FAKE_POINTER(this));
}

std::string AVCodecInfo::GetName()
{
    std::string name = data_->codecName;
    CHECK_AND_RETURN_RET_LOG(name != "", "", "get codec name is null");
    return name;
}

AVCodecType AVCodecInfo::GetType()
{
    AVCodecType codecType = AVCodecType(data_->codecType);
    CHECK_AND_RETURN_RET_LOG(codecType != AVCODEC_TYPE_NONE, AVCODEC_TYPE_NONE, "can not find codec type");
    return codecType;
}

std::string AVCodecInfo::GetMimeType()
{
    std::string mimeType = data_->mimeType;
    CHECK_AND_RETURN_RET_LOG(mimeType != "", "", "get mimeType is null");
    return mimeType;
}

bool AVCodecInfo::IsHardwareAccelerated()
{
    return data_->isVendor;
}

int32_t AVCodecInfo::GetMaxSupportedInstances()
{
    return data_->maxInstance;
}

bool AVCodecInfo::IsSoftwareOnly()
{
    return !data_->isVendor;
}

bool AVCodecInfo::IsVendor()
{
    return data_->isVendor;
}

std::map<int32_t, std::vector<int32_t>> AVCodecInfo::GetSupportedLevelsForProfile()
{
    return data_->profileLevelsMap;
}

bool AVCodecInfo::IsFeatureValid(AVCapabilityFeature feature)
{
    return feature >= AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY &&
        feature < AVCapabilityFeature::MAX_VALUE;
}

bool AVCodecInfo::IsFeatureSupported(AVCapabilityFeature feature)
{
    CHECK_AND_RETURN_RET_LOG(IsFeatureValid(feature), false,
        "Varified feature failed: feature %{public}d is invalid", feature);
    return data_->featuresMap.count(static_cast<int32_t>(feature)) != 0;
}

int32_t AVCodecInfo::GetFeatureProperties(AVCapabilityFeature feature, Format &format)
{
    CHECK_AND_RETURN_RET_LOG(IsFeatureValid(feature), AVCS_ERR_INVALID_VAL,
        "Get feature properties failed: invalid feature %{public}d", feature);
    auto itr = data_->featuresMap.find(static_cast<int32_t>(feature));
    CHECK_AND_RETURN_RET_LOG(itr != data_->featuresMap.end(), AVCS_ERR_INVALID_OPERATION,
        "Get feature properties failed: feature %{public}d is not supported", feature);
    format = itr->second;
    return AVCS_ERR_OK;
}
} // namespace MediaAVCodec
} // namespace OHOS