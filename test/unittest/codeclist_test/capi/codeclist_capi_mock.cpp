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

#include "codeclist_capi_mock.h"

using namespace OHOS::MediaAVCodec::CodecListTestParam;

namespace OHOS {
namespace MediaAVCodec {
bool CodecListCapiMock::IsHardware()
{
    if (codeclist_ != nullptr) {
        return OH_AVCapability_IsHardware(codeclist_);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}

void StackMemOverWrite()
{
    for (int i = 0; i < STACK_BUF_OVERWRITE_TIMES; i++) {
        std::vector<int32_t> vectorTest((i % STACK_BUF_OVERWRITE_MAX_BUF_SIZE) + 1, 0);
        if (i % STACK_BUF_OVERWRITE_MAX_BUF_SIZE == 0) {
            std::cout << "StackMemOverWrite addr " << vectorTest.data() << "\r";
        }
    }
    std::cout << "\n";
}

std::string CodecListCapiMock::GetName()
{
    if (codeclist_ != nullptr) {
        const char *tmp = OH_AVCapability_GetName(codeclist_);

        StackMemOverWrite();
        std::string tmpStr = tmp;
        return tmpStr;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return "";
}

int32_t CodecListCapiMock::GetMaxSupportedInstances()
{
    if (codeclist_ != nullptr) {
        return OH_AVCapability_GetMaxSupportedInstances(codeclist_);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return 0;
}

Range CodecListCapiMock::GetEncoderBitrateRange()
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetEncoderBitrateRange(codeclist_, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetEncoderBitrateRange returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

bool CodecListCapiMock::IsEncoderBitrateModeSupported(OH_BitrateMode bitrateMode)
{
    if (codeclist_ != nullptr) {
        return OH_AVCapability_IsEncoderBitrateModeSupported(codeclist_, bitrateMode);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}

Range CodecListCapiMock::GetEncoderQualityRange()
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetEncoderQualityRange(codeclist_, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetEncoderQualityRange returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

Range CodecListCapiMock::GetEncoderComplexityRange()
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetEncoderComplexityRange(codeclist_, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetEncoderComplexityRange returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

std::vector<int32_t> CodecListCapiMock::GetAudioSupportedSampleRates()
{
    if (codeclist_ != nullptr) {
        const int32_t *sampleRates = nullptr;
        uint32_t sampleRateNum = 0;
        int32_t ret = OH_AVCapability_GetAudioSupportedSampleRates(codeclist_, &sampleRates, &sampleRateNum);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetAudioSupportedSampleRates returns error: " << ret << std::endl;
            return std::vector<int32_t>();
        }
        StackMemOverWrite();
        std::vector<int32_t> retVector = std::vector<int32_t>(sampleRates, sampleRates + sampleRateNum);
        std::sort(retVector.begin(), retVector.end());
        return retVector;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

Range CodecListCapiMock::GetAudioChannelsRange()
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetAudioChannelCountRange(codeclist_, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetAudioChannelCountRange returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

int32_t CodecListCapiMock::GetVideoWidthAlignment()
{
    if (codeclist_ != nullptr) {
        int32_t widthAlignment = 0;
        int32_t ret = OH_AVCapability_GetVideoWidthAlignment(codeclist_, &widthAlignment);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoWidthAlignment returns error: " << ret << std::endl;
            return 0;
        }
        return widthAlignment;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return 0;
}

int32_t CodecListCapiMock::GetVideoHeightAlignment()
{
    if (codeclist_ != nullptr) {
        int32_t heightAlignment = 0;
        int32_t ret = OH_AVCapability_GetVideoHeightAlignment(codeclist_, &heightAlignment);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoHeightAlignment returns error: " << ret << std::endl;
            return 0;
        }
        return heightAlignment;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return 0;
}

Range CodecListCapiMock::GetVideoWidthRangeForHeight(int32_t height)
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetVideoWidthRangeForHeight(codeclist_, height, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoWidthRangeForHeight returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

Range CodecListCapiMock::GetVideoHeightRangeForWidth(int32_t width)
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetVideoHeightRangeForWidth(codeclist_, width, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoHeightRangeForWidth returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

Range CodecListCapiMock::GetVideoWidthRange()
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetVideoWidthRange(codeclist_, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoWidthRange returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

Range CodecListCapiMock::GetVideoHeightRange()
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetVideoHeightRange(codeclist_, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoHeightRange returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

bool CodecListCapiMock::IsVideoSizeSupported(int32_t width, int32_t height)
{
    if (codeclist_ != nullptr) {
        return OH_AVCapability_IsVideoSizeSupported(codeclist_, width, height);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}

Range CodecListCapiMock::GetVideoFrameRateRange()
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetVideoFrameRateRange(codeclist_, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoFrameRateRange returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

Range CodecListCapiMock::GetVideoFrameRateRangeForSize(int32_t width, int32_t height)
{
    Range retRange(0, 0);
    if (codeclist_ != nullptr) {
        OH_AVRange range;
        int32_t ret = OH_AVCapability_GetVideoFrameRateRangeForSize(codeclist_, width, height, &range);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoFrameRateRangeForSize returns error: " << ret << std::endl;
            return retRange;
        }
        retRange.minVal = range.minVal;
        retRange.maxVal = range.maxVal;
        return retRange;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return retRange;
}

bool CodecListCapiMock::AreVideoSizeAndFrameRateSupported(int32_t width, int32_t height, int32_t frameRate)
{
    if (codeclist_ != nullptr) {
        return OH_AVCapability_AreVideoSizeAndFrameRateSupported(codeclist_, width, height, frameRate);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}

std::vector<int32_t> CodecListCapiMock::GetVideoSupportedPixelFormats()
{
    if (codeclist_ != nullptr) {
        const int32_t *pixFormats = nullptr;
        uint32_t pixFormatNum = 0;
        int32_t ret = OH_AVCapability_GetVideoSupportedPixelFormats(codeclist_, &pixFormats, &pixFormatNum);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetVideoSupportedPixelFormats returns error: " << ret << std::endl;
            return std::vector<int32_t>();
        }

        StackMemOverWrite();
        std::vector<int32_t> retVector = std::vector<int32_t>(pixFormats, pixFormats + pixFormatNum);
        std::sort(retVector.begin(), retVector.end());
        return retVector;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

std::vector<int32_t> CodecListCapiMock::GetSupportedProfiles()
{
    if (codeclist_ != nullptr) {
        const int32_t *profiles = nullptr;
        uint32_t profileNum = 0;
        int32_t ret = OH_AVCapability_GetSupportedProfiles(codeclist_, &profiles, &profileNum);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetSupportedProfiles returns error: " << ret << std::endl;
            return std::vector<int32_t>();
        }

        StackMemOverWrite();
        std::vector<int32_t> retVector = std::vector<int32_t>(profiles, profiles + profileNum);
        std::sort(retVector.begin(), retVector.end());
        return retVector;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

std::vector<int32_t> CodecListCapiMock::GetSupportedLevelsForProfile(int32_t profile)
{
    if (codeclist_ != nullptr) {
        const int32_t *levels = nullptr;
        uint32_t levelNum = 0;
        int32_t ret = OH_AVCapability_GetSupportedLevelsForProfile(codeclist_, profile, &levels, &levelNum);
        if (ret != AV_ERR_OK) {
            std::cout << "OH_AVCapability_GetSupportedLevelsForProfile returns error: " << ret << std::endl;
            return std::vector<int32_t>();
        }

        StackMemOverWrite();
        std::vector<int32_t> retVector = std::vector<int32_t>(levels, levels + levelNum);
        std::sort(retVector.begin(), retVector.end());
        return retVector;
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return std::vector<int32_t>();
}

bool CodecListCapiMock::AreProfileAndLevelSupported(int32_t profile, int32_t level)
{
    if (codeclist_ != nullptr) {
        return OH_AVCapability_AreProfileAndLevelSupported(codeclist_, profile, level);
    }
    std::cout << "codeclist_ is nullptr" << std::endl;
    return false;
}
} // namespace MediaAVCodec
} // namespace OHOS