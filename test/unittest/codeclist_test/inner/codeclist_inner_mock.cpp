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

#include "codeclist_inner_mock.h"

namespace OHOS {
namespace MediaAVCodec {
bool CodecListInnerMock::IsHardware()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData_);
        return codecInfo->IsHardwareAccelerated();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}

std::string CodecListInnerMock::GetName()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData_);
        return codecInfo->GetName();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return "";
}

int32_t CodecListInnerMock::GetMaxSupportedInstances()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData_);
        return codecInfo->GetMaxSupportedInstances();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return 0;
}

Range CodecListInnerMock::GetEncoderBitrateRange()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capabilityData_);
        return codecInfo->GetSupportedBitrate();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

bool CodecListInnerMock::IsEncoderBitrateModeSupported(OH_BitrateMode bitrateMode)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        auto bitrateModeVec = codecInfo->GetSupportedBitrateMode();
        return find(bitrateModeVec.begin(), bitrateModeVec.end(), bitrateMode) != bitrateModeVec.end();
    }
    return false;
}

Range CodecListInnerMock::GetEncoderQualityRange()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetSupportedEncodeQuality();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

Range CodecListInnerMock::GetEncoderComplexityRange()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetSupportedComplexity();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

std::vector<int32_t> CodecListInnerMock::GetAudioSupportedSampleRates()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capabilityData_);
        auto ret = codecInfo->GetSupportedSampleRates();
        std::sort(ret.begin(), ret.end());
        return ret;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

Range CodecListInnerMock::GetAudioChannelsRange()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capabilityData_);
        return codecInfo->GetSupportedChannel();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

int32_t CodecListInnerMock::GetVideoWidthAlignment()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetSupportedWidthAlignment();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return 0;
}

int32_t CodecListInnerMock::GetVideoHeightAlignment()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetSupportedHeightAlignment();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return 0;
}

Range CodecListInnerMock::GetVideoWidthRangeForHeight(int32_t height)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetVideoWidthRangeForHeight(height);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

Range CodecListInnerMock::GetVideoHeightRangeForWidth(int32_t width)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetVideoHeightRangeForWidth(width);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

Range CodecListInnerMock::GetVideoWidthRange()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetSupportedWidth();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

Range CodecListInnerMock::GetVideoHeightRange()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        return codecInfo->GetSupportedHeight();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

bool CodecListInnerMock::IsVideoSizeSupported(int32_t width, int32_t height)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capabilityData_);
        return videoCap->IsSizeSupported(width, height);
    }
    return false;
}

Range CodecListInnerMock::GetVideoFrameRateRange()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capabilityData_);
        auto ret = videoCap->GetSupportedFrameRate();
        return ret;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

Range CodecListInnerMock::GetVideoFrameRateRangeForSize(int32_t width, int32_t height)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capabilityData_);
        return videoCap->GetSupportedFrameRatesFor(width, height);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return Range(0, 0);
}

bool CodecListInnerMock::AreVideoSizeAndFrameRateSupported(int32_t width, int32_t height, int32_t frameRate)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> videoCap = std::make_shared<VideoCaps>(capabilityData_);
        return videoCap->IsSizeAndRateSupported(width, height, frameRate);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}

std::vector<int32_t> CodecListInnerMock::GetVideoSupportedPixelFormats()
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
        auto ret = codecInfo->GetSupportedFormats();
        std::sort(ret.begin(), ret.end());
        return ret;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

std::vector<int32_t> CodecListInnerMock::GetSupportedProfiles()
{
    if (codeclist_ != nullptr && capabilityData_ != nullptr) {
        std::vector<int32_t> ret;
        if (capabilityData_->codecType == AVCODEC_TYPE_VIDEO_ENCODER ||
            capabilityData_->codecType == AVCODEC_TYPE_VIDEO_DECODER) {
            std::shared_ptr<VideoCaps> codecInfo = std::make_shared<VideoCaps>(capabilityData_);
            ret = codecInfo->GetSupportedProfiles();
        } else if (capabilityData_->codecType == AVCODEC_TYPE_AUDIO_DECODER ||
                   capabilityData_->codecType == AVCODEC_TYPE_AUDIO_ENCODER) {
            std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capabilityData_);
            ret = codecInfo->GetSupportedProfiles();
        }
        std::sort(ret.begin(), ret.end());
        return ret;
    }
    std::cout << "codeclist_ or capabilityData_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

std::vector<int32_t> CodecListInnerMock::GetSupportedLevelsForProfile(int32_t profile)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData_);
        auto profileLevelsMap = codecInfo->GetSupportedLevelsForProfile();
        auto levelsmatch = profileLevelsMap.find(profile);
        if (levelsmatch == profileLevelsMap.end()) {
            return std::vector<int32_t>();
        }
        std::sort(levelsmatch->second.begin(), levelsmatch->second.end());
        return levelsmatch->second;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

bool CodecListInnerMock::AreProfileAndLevelSupported(int32_t profile, int32_t level)
{
    if (codeclist_ != nullptr) {
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData_);
        auto profileLevelsMap = codecInfo->GetSupportedLevelsForProfile();
        auto levels = profileLevelsMap.find(profile);
        if (levels == profileLevelsMap.end()) {
            return false;
        }

        return find(levels->second.begin(), levels->second.end(), level) != levels->second.end();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}

std::vector<Range> CodecListInnerMock::GetAudioSupportedSampleRateRanges()
{
    std::vector<Range> retRange{Range(0, 0)};
    if (codeclist_ != nullptr) {
        std::shared_ptr<AudioCaps> codecInfo = std::make_shared<AudioCaps>(capabilityData_);
        return codecInfo->GetSupportedSampleRateRanges();
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}
} // namespace MediaAVCodec
} // namespace OHOS