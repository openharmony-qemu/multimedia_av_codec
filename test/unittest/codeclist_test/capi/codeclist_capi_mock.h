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

#ifndef CODECLIST_CAPI_MOCK_H
#define CODECLIST_CAPI_MOCK_H

#include <algorithm>
#include "avcodec_common.h"
#include "avformat_capi_mock.h"
#include "codeclist_mock.h"

namespace OHOS {
namespace MediaAVCodec {
class CodecListCapiMock : public CodecListMock {
public:
    explicit CodecListCapiMock(OH_AVCapability *codeclist) : codeclist_(codeclist) {};
    ~CodecListCapiMock() = default;
    bool IsHardware() override;
    std::string GetName() override;
    int32_t GetMaxSupportedInstances() override; // return is not errcode
    Range GetEncoderBitrateRange() override;
    bool IsEncoderBitrateModeSupported(OH_BitrateMode bitrateMode) override;
    Range GetEncoderQualityRange() override;
    Range GetEncoderComplexityRange() override;
    std::vector<int32_t> GetAudioSupportedSampleRates() override;
    Range GetAudioChannelsRange() override;
    int32_t GetVideoWidthAlignment() override;
    int32_t GetVideoHeightAlignment() override;
    Range GetVideoWidthRangeForHeight(int32_t height) override;
    Range GetVideoHeightRangeForWidth(int32_t width) override;
    Range GetVideoWidthRange() override;
    Range GetVideoHeightRange() override;
    bool IsVideoSizeSupported(int32_t width, int32_t height) override;
    Range GetVideoFrameRateRange() override;
    Range GetVideoFrameRateRangeForSize(int32_t width, int32_t height) override;
    bool AreVideoSizeAndFrameRateSupported(int32_t width, int32_t height, int32_t frameRate) override;
    std::vector<int32_t> GetVideoSupportedPixelFormats() override;
    std::vector<int32_t> GetVideoSupportedGraphicPixelFormats() override;
    std::vector<int32_t> GetSupportedProfiles() override;
    std::vector<int32_t> GetSupportedLevelsForProfile(int32_t profile) override;
    bool AreProfileAndLevelSupported(int32_t profile, int32_t level) override;
    std::vector<Range> GetAudioSupportedSampleRateRanges() override;

private:
    OH_AVCapability *codeclist_ = nullptr;
    CapabilityData capabilityData_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_CAPI_MOCK_H