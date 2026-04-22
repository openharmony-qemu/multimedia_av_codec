/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef VIDEOENC_PREPROC_DUAL_OUTPUT_SCENARIOS_H
#define VIDEOENC_PREPROC_DUAL_OUTPUT_SCENARIOS_H

#include <array>
#include <cstdint>

#include "native_avcodec_base.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreprocDualOutput {
enum class FlowType {
    NO_PREPROC = 0,
    PRIMARY_PREPROC,
    MIXED_PREPROC,
    INVALID_PARAMETER,
    INVALID_LIFECYCLE,
    DUPLICATE_SECONDARY,
};

enum class InvalidParameterType {
    DOWNSAMPLING_PAIR = 0,
    CROP_AND_DOWNSAMPLING,
    INVALID_DROP_RATE,
};

struct PreprocConfig {
    bool useDownsampling = false;
    int32_t downsamplingWidth = 0;
    int32_t downsamplingHeight = 0;
    bool useCrop = false;
    int32_t cropLeft = 0;
    int32_t cropTop = 0;
    int32_t cropRight = 0;
    int32_t cropBottom = 0;
    bool useDropToFrameRate = false;
    double dropToFrameRate = 0.0;
    double frameRate = 30.0;
};

struct ScenarioCase {
    const char *name = "";
    FlowType flow = FlowType::NO_PREPROC;
    InvalidParameterType invalidParameterType = InvalidParameterType::DOWNSAMPLING_PAIR;
    PreprocConfig primary;
    PreprocConfig secondary;
    int32_t expectedResult = AV_ERR_OK;
    bool expectExactResult = true;
    bool needsFrameRotation = false;
};

constexpr PreprocConfig MakeNoPreproc(double frameRate = 30.0)
{
    return PreprocConfig{false, 0, 0, false, 0, 0, 0, 0, false, 0.0, frameRate};
}

constexpr PreprocConfig MakeDropRate(double dropToFrameRate, double frameRate = 30.0)
{
    return PreprocConfig{false, 0, 0, false, 0, 0, 0, 0, true, dropToFrameRate, frameRate};
}

constexpr PreprocConfig MakeDownsampling(int32_t width, int32_t height, double dropToFrameRate = 0.0,
                                         double frameRate = 30.0)
{
    return PreprocConfig{true, width, height, false, 0, 0, 0, 0, dropToFrameRate > 0.0, dropToFrameRate, frameRate};
}

constexpr PreprocConfig MakeCrop(int32_t left, int32_t top, int32_t right, int32_t bottom,
                                double dropToFrameRate = 0.0, double frameRate = 30.0)
{
    return PreprocConfig{false, 0, 0, true, left, top, right, bottom, dropToFrameRate > 0.0, dropToFrameRate,
                         frameRate};
}

constexpr ScenarioCase kNoPreprocScenario {
    "no_preproc",
    FlowType::NO_PREPROC,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OK,
    true,
    true,
};

constexpr ScenarioCase kPrimaryPreprocScenario {
    "primary_preproc",
    FlowType::PRIMARY_PREPROC,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeDropRate(15.0),
    MakeNoPreproc(),
    AV_ERR_OK,
    true,
    true,
};

constexpr ScenarioCase kMixedPreprocScenario {
    "mixed_preproc",
    FlowType::MIXED_PREPROC,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeDownsampling(640, 360, 15.0),
    MakeCrop(0, 0, 1279, 719, 10.0),
    AV_ERR_OK,
    true,
    true,
};

constexpr ScenarioCase kInvalidDownsamplingScenario {
    "invalid_downsampling_pair",
    FlowType::INVALID_PARAMETER,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    PreprocConfig{true, 640, 0, false, 0, 0, 0, 0, false, 0.0, 30.0},
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
};

constexpr ScenarioCase kInvalidCropAndDownsamplingScenario {
    "invalid_crop_and_downsampling",
    FlowType::INVALID_PARAMETER,
    InvalidParameterType::CROP_AND_DOWNSAMPLING,
    PreprocConfig{true, 640, 360, true, 80, 40, 1120, 680, false, 0.0, 30.0},
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
};

constexpr ScenarioCase kInvalidDropRateScenario {
    "invalid_drop_rate",
    FlowType::INVALID_PARAMETER,
    InvalidParameterType::INVALID_DROP_RATE,
    PreprocConfig{false, 0, 0, false, 0, 0, 0, 0, true, 30.0, 30.0},
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
};

constexpr ScenarioCase kInvalidLifecycleScenario {
    "invalid_lifecycle",
    FlowType::INVALID_LIFECYCLE,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
};

constexpr ScenarioCase kDuplicateSecondaryScenario {
    "duplicate_secondary",
    FlowType::DUPLICATE_SECONDARY,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OPERATE_NOT_PERMIT,
    true,
    false,
};

constexpr std::array<ScenarioCase, 8> kDualOutputScenarios = {
    kNoPreprocScenario,
    kPrimaryPreprocScenario,
    kMixedPreprocScenario,
    kInvalidDownsamplingScenario,
    kInvalidCropAndDownsamplingScenario,
    kInvalidDropRateScenario,
    kInvalidLifecycleScenario,
    kDuplicateSecondaryScenario,
};

} // namespace PreprocDualOutput
} // namespace MediaAVCodec
} // namespace OHOS

#endif // VIDEOENC_PREPROC_DUAL_OUTPUT_SCENARIOS_H
