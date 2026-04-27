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
#include <string>

#include "buffer_common.h"
#include "native_avcodec_base.h"
#include "native_avcapability.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreprocDualOutput {
enum class FlowType {
    NO_PREPROC = 0,
    PRIMARY_PREPROC,
    SECONDARY_PREPROC,
    MIXED_PREPROC,
    INVALID_PARAMETER,
    INVALID_LIFECYCLE,
    DUPLICATE_SECONDARY,
    DROP_RATIO,
    DROP_FLUSH_STOP_RESET,
};

enum class EncoderEnableMode {
    PRIMARY_ONLY = 0,
    PRIMARY_SECONDARY,
    PRIMARY_SECONDARY_SEND_PRIMARY_ONLY,
    PRIMARY_SECONDARY_SEND_SECONDARY_ONLY,
};

enum class PreprocEnableMode {
    DISABLED = 0,
    PRIMARY_ONLY,
    SECONDARY_ONLY,
    BOTH_SAME,
    BOTH_DIFFERENT,
};

enum class RuntimeOperation {
    NONE = 0,
    SET_PREPROC_PARAMETER,
    CHANGE_BUFFER_GEOMETRY,
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
    std::string name;
    std::string codecLabel;
    const char *mime = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
    OH_AVCodecCategory category = HARDWARE;
    int32_t width = 1280;
    int32_t height = 720;
    int32_t runtimeWidth = 1280;
    int32_t runtimeHeight = 720;
    OH_NativeBuffer_Format nativeFormat = NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP;
    int32_t codecPixelFormat = AV_PIXEL_FORMAT_NV12;
    EncoderEnableMode encoderMode = EncoderEnableMode::PRIMARY_SECONDARY;
    PreprocEnableMode preprocMode = PreprocEnableMode::DISABLED;
    RuntimeOperation runtimeOperation = RuntimeOperation::NONE;
    FlowType flow = FlowType::NO_PREPROC;
    InvalidParameterType invalidParameterType = InvalidParameterType::DOWNSAMPLING_PAIR;
    PreprocConfig primary;
    PreprocConfig secondary;
    PreprocConfig runtimePrimary;
    PreprocConfig runtimeSecondary;
    int32_t expectedResult = AV_ERR_OK;
    bool expectExactResult = true;
    bool needsFrameRotation = false;
    uint32_t frameCount = 8;
    int32_t profile = -1;
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

inline const ScenarioCase kNoPreprocScenario {
    "no_preproc",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::DISABLED,
    RuntimeOperation::NONE,
    FlowType::NO_PREPROC,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OK,
    true,
    true,
    8,
};

inline const ScenarioCase kPrimaryPreprocScenario {
    "primary_preproc",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::PRIMARY_ONLY,
    RuntimeOperation::NONE,
    FlowType::PRIMARY_PREPROC,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeDropRate(15.0),
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OK,
    true,
    true,
    30,
};

inline const ScenarioCase kMixedPreprocScenario {
    "mixed_preproc",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::BOTH_DIFFERENT,
    RuntimeOperation::NONE,
    FlowType::MIXED_PREPROC,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeDownsampling(640, 360, 15.0),
    MakeCrop(0, 0, 1023, 575, 10.0),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OK,
    true,
    true,
    30,
};

inline const ScenarioCase kInvalidDownsamplingScenario {
    "invalid_downsampling_pair",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::PRIMARY_ONLY,
    RuntimeOperation::NONE,
    FlowType::INVALID_PARAMETER,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    PreprocConfig{true, 640, 0, false, 0, 0, 0, 0, false, 0.0, 30.0},
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
    8,
};

inline const ScenarioCase kInvalidCropAndDownsamplingScenario {
    "invalid_crop_and_downsampling",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::PRIMARY_ONLY,
    RuntimeOperation::NONE,
    FlowType::INVALID_PARAMETER,
    InvalidParameterType::CROP_AND_DOWNSAMPLING,
    PreprocConfig{true, 640, 360, true, 80, 40, 1120, 680, false, 0.0, 30.0},
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
    8,
};

inline const ScenarioCase kInvalidDropRateScenario {
    "invalid_drop_rate",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::PRIMARY_ONLY,
    RuntimeOperation::NONE,
    FlowType::INVALID_PARAMETER,
    InvalidParameterType::INVALID_DROP_RATE,
    PreprocConfig{false, 0, 0, false, 0, 0, 0, 0, true, 30.0, 30.0},
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
    8,
};

inline const ScenarioCase kInvalidLifecycleScenario {
    "invalid_lifecycle",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::DISABLED,
    RuntimeOperation::NONE,
    FlowType::INVALID_LIFECYCLE,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_INVALID_VAL,
    true,
    false,
    8,
};

inline const ScenarioCase kDuplicateSecondaryScenario {
    "duplicate_secondary",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::DISABLED,
    RuntimeOperation::NONE,
    FlowType::DUPLICATE_SECONDARY,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OPERATE_NOT_PERMIT,
    true,
    false,
    8,
};

inline const ScenarioCase kDropRatioScenario {
    "drop_ratio",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::PRIMARY_ONLY,
    RuntimeOperation::NONE,
    FlowType::DROP_RATIO,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeDropRate(15.0),
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OK,
    true,
    false,
    30,
};

inline const ScenarioCase kDropFlushStopResetScenario {
    "drop_flush_stop_reset",
    "hardware_avc",
    OH_AVCODEC_MIMETYPE_VIDEO_AVC,
    HARDWARE,
    1280,
    720,
    1280,
    720,
    NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP,
    AV_PIXEL_FORMAT_NV12,
    EncoderEnableMode::PRIMARY_SECONDARY,
    PreprocEnableMode::PRIMARY_ONLY,
    RuntimeOperation::NONE,
    FlowType::DROP_FLUSH_STOP_RESET,
    InvalidParameterType::DOWNSAMPLING_PAIR,
    MakeDropRate(15.0),
    MakeNoPreproc(),
    MakeNoPreproc(),
    MakeNoPreproc(),
    AV_ERR_OK,
    true,
    false,
};

inline const std::array<ScenarioCase, 10> kDualOutputScenarios = {
    kNoPreprocScenario,
    kPrimaryPreprocScenario,
    kMixedPreprocScenario,
    kInvalidDownsamplingScenario,
    kInvalidCropAndDownsamplingScenario,
    kInvalidDropRateScenario,
    kInvalidLifecycleScenario,
    kDuplicateSecondaryScenario,
    kDropRatioScenario,
    kDropFlushStopResetScenario,
};

} // namespace PreprocDualOutput
} // namespace MediaAVCodec
} // namespace OHOS

#endif // VIDEOENC_PREPROC_DUAL_OUTPUT_SCENARIOS_H
