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

#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <iostream>
#include <map>
#include <sstream>
#include <string>
#include <vector>

#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "videoenc_preproc_dual_output_sample.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::PreprocDualOutput;
using namespace testing::ext;

namespace OHOS {
namespace MediaAVCodec {
namespace PreprocDualOutput {
void PrintTo(const ScenarioCase &scenario, std::ostream *os)
{
    (void)scenario;
    (void)os;
}
} // namespace PreprocDualOutput
} // namespace MediaAVCodec
} // namespace OHOS

namespace {
constexpr uint32_t K_FRAME_COUNT = 30;
constexpr int32_t K_HALF_DIVISOR = 2;
constexpr uint32_t K_EOS_CLEANUP_TIMEOUT_SECONDS = 2;
constexpr int32_t K_MIN_RUNTIME_DIMENSION = 64;
constexpr int32_t K_BASE_WIDTH = 1280;
constexpr int32_t K_BASE_HEIGHT = 720;
constexpr double K_RUNTIME_DROP_FRAME_RATE = 12.0;
constexpr double K_PRIMARY_DROP_FRAME_RATE = 15.0;
constexpr double K_SECONDARY_CROP_DROP_FRAME_RATE = 10.0;
constexpr double K_SET_PARAMETER_DROP_FRAME_RATE = 25.0;
constexpr uint32_t K_RESUME_FRAME_COUNT = 8;
constexpr int32_t K_EXPECTED_RESUME_PRIMARY_COUNT = 4;
constexpr int32_t K_EXPECTED_SET_PARAMETER_PRIMARY_COUNT = 7;
constexpr int32_t K_NEAR_TOLERANCE_ONE = 1;
constexpr int32_t K_NEAR_TOLERANCE_TWO = 2;
constexpr int32_t K_NEAR_TOLERANCE_THREE = 3;

struct CodecDimension {
    const char *label;
    const char *mime;
    OH_AVCodecCategory category;
};

struct ResolutionDimension {
    const char *label;
    int32_t width;
    int32_t height;
};

struct NativeFormatDimension {
    const char *label;
    OH_NativeBuffer_Format nativeFormat;
    int32_t codecPixelFormat;
};

struct ScenarioMatrixCase {
    CodecDimension codec;
    ResolutionDimension resolution;
    NativeFormatDimension format;
    EncoderEnableMode encoderMode;
    PreprocEnableMode preprocMode;
    RuntimeOperation runtimeOperation;
};

class VideoEncPreprocDualOutputLevel0Test : public testing::TestWithParam<ScenarioCase> {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

const std::array<CodecDimension, 5> &CodecDimensions()
{
    static const std::array<CodecDimension, 5> codecs = {{
        {"hw_h264", OH_AVCODEC_MIMETYPE_VIDEO_AVC, HARDWARE},
        {"hw_h265", OH_AVCODEC_MIMETYPE_VIDEO_HEVC, HARDWARE},
        {"hw_h266", OH_AVCODEC_MIMETYPE_VIDEO_VVC, HARDWARE},
        {"sw_h264", OH_AVCODEC_MIMETYPE_VIDEO_AVC, SOFTWARE},
        {"sw_h265", OH_AVCODEC_MIMETYPE_VIDEO_HEVC, SOFTWARE},
    }};
    return codecs;
}

const std::array<ResolutionDimension, 4> &ResolutionDimensions()
{
    static const std::array<ResolutionDimension, 4> resolutions = {{
        {"720p", 1280, 720},
        {"1080p", 1920, 1080},
        {"1440p", 2560, 1440},
        {"2160p", 3840, 2160},
    }};
    return resolutions;
}

const std::array<NativeFormatDimension, 6> &NativeFormatDimensions()
{
    static const std::array<NativeFormatDimension, 6> formats = {{
        {"ycbcr420sp", NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP, AV_PIXEL_FORMAT_NV12},
        {"ycrcb420sp", NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP, AV_PIXEL_FORMAT_NV21},
        {"rgba8888", NATIVEBUFFER_PIXEL_FMT_RGBA_8888, AV_PIXEL_FORMAT_RGBA},
        {"rgba1010102", NATIVEBUFFER_PIXEL_FMT_RGBA_1010102, AV_PIXEL_FORMAT_RGBA1010102},
        {"ycbcrp010", NATIVEBUFFER_PIXEL_FMT_YCBCR_P010, AV_PIXEL_FORMAT_NV12},
        {"ycrcbp010", NATIVEBUFFER_PIXEL_FMT_YCRCB_P010, AV_PIXEL_FORMAT_NV21},
    }};
    return formats;
}

const std::array<EncoderEnableMode, 4> &EncoderModeDimensions()
{
    static const std::array<EncoderEnableMode, 4> modes = {{
        EncoderEnableMode::PRIMARY_ONLY,
        EncoderEnableMode::PRIMARY_SECONDARY,
        EncoderEnableMode::PRIMARY_SECONDARY_SEND_PRIMARY_ONLY,
        EncoderEnableMode::PRIMARY_SECONDARY_SEND_SECONDARY_ONLY,
    }};
    return modes;
}

const std::array<PreprocEnableMode, 5> &PreprocModeDimensions()
{
    static const std::array<PreprocEnableMode, 5> modes = {{
        PreprocEnableMode::DISABLED,
        PreprocEnableMode::PRIMARY_ONLY,
        PreprocEnableMode::SECONDARY_ONLY,
        PreprocEnableMode::BOTH_SAME,
        PreprocEnableMode::BOTH_DIFFERENT,
    }};
    return modes;
}

const std::array<RuntimeOperation, 3> &RuntimeOperationDimensions()
{
    static const std::array<RuntimeOperation, 3> operations = {{
        RuntimeOperation::NONE,
        RuntimeOperation::SET_PREPROC_PARAMETER,
        RuntimeOperation::CHANGE_BUFFER_GEOMETRY,
    }};
    return operations;
}

bool NeedsSecondary(EncoderEnableMode mode)
{
    return mode != EncoderEnableMode::PRIMARY_ONLY;
}

bool ExpectsPrimaryOutput(EncoderEnableMode mode)
{
    return mode == EncoderEnableMode::PRIMARY_ONLY || mode == EncoderEnableMode::PRIMARY_SECONDARY ||
           mode == EncoderEnableMode::PRIMARY_SECONDARY_SEND_PRIMARY_ONLY;
}

bool ExpectsSecondaryOutput(EncoderEnableMode mode)
{
    return mode == EncoderEnableMode::PRIMARY_SECONDARY ||
           mode == EncoderEnableMode::PRIMARY_SECONDARY_SEND_SECONDARY_ONLY;
}

std::string ToString(EncoderEnableMode mode)
{
    switch (mode) {
        case EncoderEnableMode::PRIMARY_ONLY:
            return "primary";
        case EncoderEnableMode::PRIMARY_SECONDARY:
            return "dual";
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_PRIMARY_ONLY:
            return "dual_send_primary";
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_SECONDARY_ONLY:
            return "dual_send_secondary";
        default:
            return "unknown_encoder_mode";
    }
}

std::string ToString(PreprocEnableMode mode)
{
    switch (mode) {
        case PreprocEnableMode::DISABLED:
            return "preproc_none";
        case PreprocEnableMode::PRIMARY_ONLY:
            return "preproc_primary";
        case PreprocEnableMode::SECONDARY_ONLY:
            return "preproc_secondary";
        case PreprocEnableMode::BOTH_SAME:
            return "preproc_same";
        case PreprocEnableMode::BOTH_DIFFERENT:
            return "preproc_diff";
        default:
            return "preproc_unknown";
    }
}

std::string ToString(RuntimeOperation operation)
{
    switch (operation) {
        case RuntimeOperation::NONE:
            return "runtime_none";
        case RuntimeOperation::SET_PREPROC_PARAMETER:
            return "runtime_set_param";
        case RuntimeOperation::CHANGE_BUFFER_GEOMETRY:
            return "runtime_geometry";
        default:
            return "runtime_unknown";
    }
}

std::string ToString(FlowType flow)
{
    switch (flow) {
        case FlowType::NO_PREPROC:
            return "no_preproc";
        case FlowType::PRIMARY_PREPROC:
            return "primary_preproc";
        case FlowType::SECONDARY_PREPROC:
            return "secondary_preproc";
        case FlowType::MIXED_PREPROC:
            return "mixed_preproc";
        case FlowType::INVALID_PARAMETER:
            return "invalid_parameter";
        case FlowType::INVALID_LIFECYCLE:
            return "invalid_lifecycle";
        case FlowType::DUPLICATE_SECONDARY:
            return "duplicate_secondary";
        case FlowType::DROP_RATIO:
            return "drop_ratio";
        case FlowType::DROP_FLUSH_STOP_RESET:
            return "drop_flush_stop_reset";
        default:
            return "unknown_flow";
    }
}

std::string ToString(InvalidParameterType type)
{
    switch (type) {
        case InvalidParameterType::DOWNSAMPLING_PAIR:
            return "downsampling_pair";
        case InvalidParameterType::CROP_AND_DOWNSAMPLING:
            return "crop_and_downsampling";
        case InvalidParameterType::INVALID_DROP_RATE:
            return "invalid_drop_rate";
        default:
            return "unknown_invalid_parameter";
    }
}

std::string ToString(OH_AVCodecCategory category)
{
    switch (category) {
        case HARDWARE:
            return "hardware";
        case SOFTWARE:
            return "software";
        default:
            return "unknown_category";
    }
}

std::string NativeFormatToString(OH_NativeBuffer_Format format)
{
    switch (format) {
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP:
            return "NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP";
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP:
            return "NATIVEBUFFER_PIXEL_FMT_YCRCB_420_SP";
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P:
            return "NATIVEBUFFER_PIXEL_FMT_YCBCR_420_P";
        case NATIVEBUFFER_PIXEL_FMT_RGBA_8888:
            return "NATIVEBUFFER_PIXEL_FMT_RGBA_8888";
        case NATIVEBUFFER_PIXEL_FMT_RGBA_1010102:
            return "NATIVEBUFFER_PIXEL_FMT_RGBA_1010102";
        case NATIVEBUFFER_PIXEL_FMT_YCBCR_P010:
            return "NATIVEBUFFER_PIXEL_FMT_YCBCR_P010";
        case NATIVEBUFFER_PIXEL_FMT_YCRCB_P010:
            return "NATIVEBUFFER_PIXEL_FMT_YCRCB_P010";
        default:
            return "unknown_native_format";
    }
}

std::string CodecPixelFormatToString(int32_t format)
{
    switch (format) {
        case AV_PIXEL_FORMAT_NV12:
            return "AV_PIXEL_FORMAT_NV12";
        case AV_PIXEL_FORMAT_NV21:
            return "AV_PIXEL_FORMAT_NV21";
        case AV_PIXEL_FORMAT_RGBA:
            return "AV_PIXEL_FORMAT_RGBA";
        case AV_PIXEL_FORMAT_RGBA1010102:
            return "AV_PIXEL_FORMAT_RGBA1010102";
        default:
            return "unknown_codec_pixel_format";
    }
}

std::string ProfileToString(int32_t profile)
{
    if (profile < 0) {
        return "unset";
    }
    switch (profile) {
        case HEVC_PROFILE_MAIN:
            return "HEVC_PROFILE_MAIN";
        case HEVC_PROFILE_MAIN_10:
            return "HEVC_PROFILE_MAIN_10";
        default:
            return std::to_string(profile);
    }
}

const char *BoolToString(bool value)
{
    return value ? "true" : "false";
}

std::string DescribeRuntimeDelivery(const ScenarioCase &scenario)
{
    if (scenario.runtimeOperation == RuntimeOperation::NONE) {
        return "not scheduled";
    }

    uint32_t triggerFrameIndex = K_FRAME_COUNT / K_HALF_DIVISOR;
    std::ostringstream oss;
    oss << "frameNumber=" << (triggerFrameIndex + 1) <<
        ", frameIndex=" << triggerFrameIndex <<
        ", sendFrameCount=" << K_FRAME_COUNT;
    return oss.str();
}

std::string DescribePreprocConfig(const PreprocConfig &config)
{
    std::ostringstream oss;
    oss << "{frameRate=" << config.frameRate;
    if (config.useDownsampling) {
        oss << ", downsampling=" << config.downsamplingWidth << "x" << config.downsamplingHeight;
    }
    if (config.useCrop) {
        oss << ", crop=[" << config.cropLeft << "," << config.cropTop << "," << config.cropRight << ","
            << config.cropBottom << "]";
    }
    if (config.useDropToFrameRate) {
        oss << ", dropToFrameRate=" << config.dropToFrameRate;
    }
    if (!config.useDownsampling && !config.useCrop && !config.useDropToFrameRate) {
        oss << ", preproc=none";
    }
    oss << "}";
    return oss.str();
}

std::string DescribeScenario(const ScenarioCase &scenario)
{
    std::ostringstream oss;
    oss << "scenario\n"
        << "  name: " << scenario.name << "\n"
        << "  flow: " << ToString(scenario.flow) << "\n"
        << "  codec: " << scenario.codecLabel << ", mime=" << scenario.mime << ", category="
        << ToString(scenario.category) << "\n"
        << "  size: " << scenario.width << "x" << scenario.height << ", runtimeSize=" << scenario.runtimeWidth
        << "x" << scenario.runtimeHeight << "\n"
        << "  formats: native=" << NativeFormatToString(scenario.nativeFormat) << "(" << scenario.nativeFormat
        << "), codecPixel=" << CodecPixelFormatToString(scenario.codecPixelFormat) << "("
        << scenario.codecPixelFormat << "), profile=" << ProfileToString(scenario.profile) << "\n"
        << "  modes: encoder=" << ToString(scenario.encoderMode) << ", preproc="
        << ToString(scenario.preprocMode) << ", runtime=" << ToString(scenario.runtimeOperation) << "\n"
        << "  invalidParameterType: " << ToString(scenario.invalidParameterType) << "\n"
        << "  primaryPreproc: " << DescribePreprocConfig(scenario.primary) << "\n"
        << "  secondaryPreproc: " << DescribePreprocConfig(scenario.secondary) << "\n"
        << "  runtimeDelivery: " << DescribeRuntimeDelivery(scenario) << "\n";
    if (scenario.runtimeOperation != RuntimeOperation::NONE) {
        oss << "  runtimePrimaryPreproc: " << DescribePreprocConfig(scenario.runtimePrimary) << "\n"
            << "  runtimeSecondaryPreproc: " << DescribePreprocConfig(scenario.runtimeSecondary) << "\n";
    }
    oss << "  expectation: result=" << scenario.expectedResult << ", exact="
        << BoolToString(scenario.expectExactResult) << ", needsFrameRotation="
        << BoolToString(scenario.needsFrameRotation) << ", frameCount=" << scenario.frameCount;
    return oss.str();
}

std::string DescribeStep(const char *step)
{
    std::ostringstream oss;
    oss << "step: " << step;
    return oss.str();
}

std::string DescribeSampleState(const VideoEncPreprocDualOutputSample &sample)
{
    std::ostringstream oss;
    oss << "\nsample_state {primaryData=" << sample.PrimaryDataCount()
        << ", secondaryData=" << sample.SecondaryDataCount()
        << ", primaryError=" << sample.PrimaryError()
        << ", secondaryError=" << sample.SecondaryError()
        << ", primaryEos=" << BoolToString(sample.PrimaryGotEos())
        << ", secondaryEos=" << BoolToString(sample.SecondaryGotEos()) << "}";
    return oss.str();
}

testing::AssertionResult SampleFailure(const VideoEncPreprocDualOutputSample &sample, const char *step)
{
    return testing::AssertionFailure() << DescribeStep(step) << DescribeSampleState(sample);
}

testing::AssertionResult AssertTrueSampleStep(bool condition, const VideoEncPreprocDualOutputSample &sample,
                                              const char *step)
{
    if (condition) {
        return testing::AssertionSuccess();
    }
    return SampleFailure(sample, step);
}

template<typename ActualType, typename ExpectedType>
testing::AssertionResult AssertEqSampleStep(const ActualType &actual, const ExpectedType &expected,
                                            const VideoEncPreprocDualOutputSample &sample, const char *step)
{
    if (actual == expected) {
        return testing::AssertionSuccess();
    }
    auto result = SampleFailure(sample, step);
    result << ", actual=" << actual << ", expected=" << expected;
    return result;
}

testing::AssertionResult ExpectNearSampleStep(double actual, double expected, double absError,
                                              const VideoEncPreprocDualOutputSample &sample, const char *step)
{
    if (std::abs(actual - expected) <= absError) {
        return testing::AssertionSuccess();
    }
    auto result = SampleFailure(sample, step);
    result << ", actual=" << actual << ", expected=" << expected << ", absError=" << absError;
    return result;
}

template<typename ActualType, typename ExpectedType>
testing::AssertionResult ExpectLeSampleStep(const ActualType &actual, const ExpectedType &expected,
                                            const VideoEncPreprocDualOutputSample &sample, const char *step)
{
    if (actual <= expected) {
        return testing::AssertionSuccess();
    }
    auto result = SampleFailure(sample, step);
    result << ", actual=" << actual << ", expected<=" << expected;
    return result;
}

template<typename ActualType, typename ExpectedType>
testing::AssertionResult ExpectGeSampleStep(const ActualType &actual, const ExpectedType &expected,
                                            const VideoEncPreprocDualOutputSample &sample, const char *step)
{
    if (actual >= expected) {
        return testing::AssertionSuccess();
    }
    auto result = SampleFailure(sample, step);
    result << ", actual=" << actual << ", expected>=" << expected;
    return result;
}

bool IsNativeFormatSupported(OH_AVCapability *capability, OH_NativeBuffer_Format nativeFormat)
{
    const OH_NativeBuffer_Format *formats = nullptr;
    uint32_t formatCount = 0;
    if (OH_AVCapability_GetVideoSupportedNativeBufferFormats(capability, &formats, &formatCount) != AV_ERR_OK ||
        formats == nullptr) {
        return false;
    }
    for (uint32_t i = 0; i < formatCount; ++i) {
        if (formats[i] == nativeFormat) {
            return true;
        }
    }
    return false;
}

bool IsTenBitNativeFormat(OH_NativeBuffer_Format nativeFormat)
{
    return nativeFormat == NATIVEBUFFER_PIXEL_FMT_RGBA_1010102 ||
           nativeFormat == NATIVEBUFFER_PIXEL_FMT_YCBCR_P010 ||
           nativeFormat == NATIVEBUFFER_PIXEL_FMT_YCRCB_P010;
}

bool IsProfileSupported(OH_AVCapability *capability, int32_t profile)
{
    const int32_t *profiles = nullptr;
    uint32_t profileCount = 0;
    if (OH_AVCapability_GetSupportedProfiles(capability, &profiles, &profileCount) != AV_ERR_OK ||
        profiles == nullptr) {
        return false;
    }
    for (uint32_t i = 0; i < profileCount; ++i) {
        if (profiles[i] == profile) {
            return true;
        }
    }
    return false;
}

bool TryGetScenarioProfile(OH_AVCapability *capability, const CodecDimension &codec,
                           const NativeFormatDimension &format, int32_t &profile)
{
    profile = -1;
    if (!IsTenBitNativeFormat(format.nativeFormat)) {
        return true;
    }
    if (std::string(codec.mime) != OH_AVCODEC_MIMETYPE_VIDEO_HEVC) {
        return false;
    }
    if (!IsProfileSupported(capability, HEVC_PROFILE_MAIN_10)) {
        return false;
    }
    profile = HEVC_PROFILE_MAIN_10;
    return true;
}

PreprocConfig MakeRuntimeConfig(int32_t width, int32_t height, uint32_t seed)
{
    if (seed % K_HALF_DIVISOR == 0) {
        return MakeDropRate(K_RUNTIME_DROP_FRAME_RATE);
    }
    return MakeDownsampling(width / K_HALF_DIVISOR, height / K_HALF_DIVISOR, K_RUNTIME_DROP_FRAME_RATE);
}

FlowType MakeFlowType(PreprocEnableMode preprocMode)
{
    switch (preprocMode) {
        case PreprocEnableMode::DISABLED:
            return FlowType::NO_PREPROC;
        case PreprocEnableMode::PRIMARY_ONLY:
            return FlowType::PRIMARY_PREPROC;
        case PreprocEnableMode::SECONDARY_ONLY:
            return FlowType::SECONDARY_PREPROC;
        case PreprocEnableMode::BOTH_SAME:
        case PreprocEnableMode::BOTH_DIFFERENT:
            return FlowType::MIXED_PREPROC;
        default:
            return FlowType::NO_PREPROC;
    }
}

void ApplyPreprocMode(ScenarioCase &scenario)
{
    scenario.primary = MakeNoPreproc();
    scenario.secondary = MakeNoPreproc();
    switch (scenario.preprocMode) {
        case PreprocEnableMode::PRIMARY_ONLY:
            scenario.primary = MakeDropRate(K_PRIMARY_DROP_FRAME_RATE);
            break;
        case PreprocEnableMode::SECONDARY_ONLY:
            scenario.secondary = MakeDropRate(K_PRIMARY_DROP_FRAME_RATE);
            break;
        case PreprocEnableMode::BOTH_SAME:
#ifdef SUPPORT_VIDEO_ENCODER_PREPROCESSING
            scenario.primary = MakeDownsampling(scenario.width / K_HALF_DIVISOR,
                                                scenario.height / K_HALF_DIVISOR,
                                                K_PRIMARY_DROP_FRAME_RATE);
#else
            scenario.primary = MakeDropRate(K_PRIMARY_DROP_FRAME_RATE);
#endif
            scenario.secondary = scenario.primary;
            break;
        case PreprocEnableMode::BOTH_DIFFERENT:
            scenario.primary = MakeDownsampling(scenario.width / K_HALF_DIVISOR,
                                                scenario.height / K_HALF_DIVISOR,
                                                K_PRIMARY_DROP_FRAME_RATE);
            scenario.secondary = MakeCrop(MakeCropRect(0, 0, scenario.width - 1, scenario.height - 1),
                                          K_SECONDARY_CROP_DROP_FRAME_RATE);
            break;
        case PreprocEnableMode::DISABLED:
        default:
            break;
    }
    scenario.runtimePrimary = MakeRuntimeConfig(scenario.width, scenario.height, 0);
    scenario.runtimeSecondary = MakeRuntimeConfig(scenario.width, scenario.height, 1);
}

ScenarioCase MakeScenario(const ScenarioMatrixCase &matrix)
{
    ScenarioCase scenario {};
    scenario.codecLabel = matrix.codec.label;
    scenario.mime = matrix.codec.mime;
    scenario.category = matrix.codec.category;
    scenario.width = matrix.resolution.width;
    scenario.height = matrix.resolution.height;
    scenario.runtimeWidth = std::max(K_MIN_RUNTIME_DIMENSION, matrix.resolution.width / K_HALF_DIVISOR);
    scenario.runtimeHeight = std::max(K_MIN_RUNTIME_DIMENSION, matrix.resolution.height / K_HALF_DIVISOR);
    scenario.nativeFormat = matrix.format.nativeFormat;
    scenario.codecPixelFormat = matrix.format.codecPixelFormat;
    scenario.encoderMode = matrix.encoderMode;
    scenario.preprocMode = matrix.preprocMode;
    scenario.runtimeOperation = matrix.runtimeOperation;
    scenario.flow = MakeFlowType(matrix.preprocMode);
    scenario.expectedResult = AV_ERR_OK;
    scenario.expectExactResult = true;
    scenario.needsFrameRotation = true;
    ApplyPreprocMode(scenario);
    scenario.name = scenario.codecLabel + "_" + matrix.resolution.label + "_" + matrix.format.label + "_" +
                    ToString(matrix.encoderMode) + "_" + ToString(matrix.preprocMode) + "_" +
                    ToString(matrix.runtimeOperation);
    return scenario;
}

void AppendRuntimeScenarioVariants(std::vector<ScenarioCase> &scenarios, const CodecDimension &codec,
                                   const ResolutionDimension &resolution, const NativeFormatDimension &format,
                                   int32_t profile)
{
    for (auto encoderMode : EncoderModeDimensions()) {
        for (auto preprocMode : PreprocModeDimensions()) {
            for (auto runtimeOperation : RuntimeOperationDimensions()) {
                auto scenario = MakeScenario({codec, resolution, format, encoderMode, preprocMode, runtimeOperation});
                scenario.profile = profile;
                scenarios.emplace_back(scenario);
            }
        }
    }
}

void AppendFormatScenarios(std::vector<ScenarioCase> &scenarios, OH_AVCapability *capability,
                           const CodecDimension &codec, const ResolutionDimension &resolution)
{
    for (const auto &format : NativeFormatDimensions()) {
        if (!IsNativeFormatSupported(capability, format.nativeFormat)) {
            continue;
        }
        int32_t profile = -1;
        if (!TryGetScenarioProfile(capability, codec, format, profile)) {
            continue;
        }
        AppendRuntimeScenarioVariants(scenarios, codec, resolution, format, profile);
    }
}

void AppendCodecScenarios(std::vector<ScenarioCase> &scenarios, const CodecDimension &codec)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codec.mime, true, codec.category);
    if (capability == nullptr) {
        return;
    }
    for (const auto &resolution : ResolutionDimensions()) {
        if (!OH_AVCapability_IsVideoSizeSupported(capability, resolution.width, resolution.height)) {
            continue;
        }
        AppendFormatScenarios(scenarios, capability, codec, resolution);
    }
}

std::vector<ScenarioCase> BuildSupportedScenarios()
{
    std::vector<ScenarioCase> scenarios;
    for (const auto &codec : CodecDimensions()) {
        AppendCodecScenarios(scenarios, codec);
    }
    return scenarios;
}

const std::vector<ScenarioCase> &GetSupportedScenarios()
{
    static std::vector<ScenarioCase> scenarios = BuildSupportedScenarios();
    static bool once = false;
    if (!once) {
        once = true;
        for (const auto &s : K_DUAL_OUTPUT_SCENARIOS) {
            scenarios.push_back(s);
        }
    }
    return scenarios;
}

bool ContainsScenarioName(const std::vector<ScenarioCase> &scenarios, const std::string &name)
{
    for (const auto &scenario : scenarios) {
        if (scenario.name == name) {
            return true;
        }
    }
    return false;
}

void AddScenarioIfMissing(std::vector<ScenarioCase> &scenarios, const ScenarioCase &scenario)
{
    if (!ContainsScenarioName(scenarios, scenario.name)) {
        scenarios.push_back(scenario);
    }
}

template<typename Predicate>
bool AddFirstScenarioMatching(std::vector<ScenarioCase> &scenarios, Predicate predicate,
                              const std::vector<ScenarioCase> *excluded = nullptr)
{
    for (const auto &scenario : GetSupportedScenarios()) {
        if (!predicate(scenario)) {
            continue;
        }
        if (excluded != nullptr && ContainsScenarioName(*excluded, scenario.name)) {
            continue;
        }
        AddScenarioIfMissing(scenarios, scenario);
        return true;
    }
    return false;
}

bool IsBaseCodecResolutionFormat(const ScenarioCase &scenario)
{
    return scenario.codecLabel == "hw_h264" && scenario.width == K_BASE_WIDTH && scenario.height == K_BASE_HEIGHT &&
           scenario.nativeFormat == NATIVEBUFFER_PIXEL_FMT_YCBCR_420_SP;
}

bool IsDualRuntimeNone(const ScenarioCase &scenario)
{
    return scenario.encoderMode == EncoderEnableMode::PRIMARY_SECONDARY &&
           scenario.runtimeOperation == RuntimeOperation::NONE;
}

std::vector<ScenarioCase> BuildLevel0Scenarios()
{
    std::vector<ScenarioCase> scenarios;
    const std::array<FlowType, 4> smokeFlows = {{
        FlowType::NO_PREPROC,
        FlowType::PRIMARY_PREPROC,
        FlowType::SECONDARY_PREPROC,
        FlowType::MIXED_PREPROC,
    }};

    for (auto flow : smokeFlows) {
        bool added = AddFirstScenarioMatching(scenarios, [flow](const ScenarioCase &scenario) {
            return scenario.flow == flow && IsBaseCodecResolutionFormat(scenario) && IsDualRuntimeNone(scenario);
        });
        if (!added) {
            (void)AddFirstScenarioMatching(scenarios, [flow](const ScenarioCase &scenario) {
                return scenario.flow == flow && IsDualRuntimeNone(scenario);
            });
        }
    }
    return scenarios;
}

const std::vector<ScenarioCase> &GetLevel0Scenarios()
{
    static const std::vector<ScenarioCase> scenarios = BuildLevel0Scenarios();
    return scenarios;
}

std::string SanitizeName(std::string name)
{
    for (char &value : name) {
        bool isValid = (value >= '0' && value <= '9') || (value >= 'A' && value <= 'Z') ||
                       (value >= 'a' && value <= 'z') || value == '_';
        if (!isValid) {
            value = '_';
        }
    }
    if (name.empty() || (name[0] >= '0' && name[0] <= '9')) {
        name = "S_" + name;
    }
    return name;
}

std::string GetOutputFilePath()
{
    const testing::TestInfo *testInfo = testing::UnitTest::GetInstance()->current_test_info();
    std::string testName = testInfo == nullptr ? "unknown" : testInfo->name();
    auto isPathSeparator = [](char value) { return value == '/' || value == '\\'; };
    testName.erase(std::remove_if(testName.begin(), testName.end(), isPathSeparator), testName.end());

    static std::map<std::string, uint32_t> outputPathCounts;
    uint32_t repeatIndex = ++outputPathCounts[testName];
    std::string repeatSuffix = repeatIndex > 1 ? "_repeat" + std::to_string(repeatIndex) : "";
    return "./1in2out_out/" + testName + repeatSuffix + ".bin";
}

void RunDropRatioScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(AssertTrueSampleStep(sample.SetOutputFilePath(GetOutputFilePath()), sample,
        "RunDropRatioScenario.SetOutputFilePath"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.CreatePrimary(scenario.mime), sample,
        "RunDropRatioScenario.CreatePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigurePrimary(scenario), AV_ERR_OK, sample,
        "RunDropRatioScenario.ConfigurePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.GetPrimarySurface(scenario), AV_ERR_OK, sample,
        "RunDropRatioScenario.GetPrimarySurface"));
    ASSERT_TRUE(AssertEqSampleStep(sample.CreateSecondary(), AV_ERR_OK, sample,
        "RunDropRatioScenario.CreateSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigureSecondary(scenario), AV_ERR_OK, sample,
        "RunDropRatioScenario.ConfigureSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StartPrimary(), AV_ERR_OK, sample, "RunDropRatioScenario.StartPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StartSecondary(), AV_ERR_OK, sample, "RunDropRatioScenario.StartSecondary"));
    constexpr uint32_t kDropRatioFrameCount = K_FRAME_COUNT;
    ASSERT_TRUE(AssertEqSampleStep(sample.SendFramesWithConfiguredPts(scenario, kDropRatioFrameCount),
        AV_ERR_OK, sample,
        "RunDropRatioScenario.SendFramesWithConfiguredPts"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForDualEos(), sample, "RunDropRatioScenario.WaitForDualEos"));
    // Primary: 30fps->15fps (keepRatio=0.5), expect ~15 kept out of 30
    EXPECT_TRUE(ExpectNearSampleStep(sample.PrimaryDataCount(), K_PRIMARY_DROP_FRAME_RATE,
        K_NEAR_TOLERANCE_THREE, sample,
        "RunDropRatioScenario.PrimaryDataCount"));
    // Secondary: no drop, expect all 30 frames
    EXPECT_TRUE(ExpectNearSampleStep(sample.SecondaryDataCount(), static_cast<int32_t>(kDropRatioFrameCount),
        K_NEAR_TOLERANCE_TWO, sample,
        "RunDropRatioScenario.SecondaryDataCount"));
    EXPECT_TRUE(AssertEqSampleStep(sample.PrimaryError(), 0, sample, "RunDropRatioScenario.PrimaryError"));
    EXPECT_TRUE(AssertEqSampleStep(sample.SecondaryError(), 0, sample, "RunDropRatioScenario.SecondaryError"));
}

void VerifyDropAfterResume(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    (void)scenario;
    // Primary: 30fps->15fps (keepRatio=0.5), 8 frames sent, expect ~4-5 kept
    EXPECT_TRUE(ExpectNearSampleStep(sample.PrimaryDataCount(), K_EXPECTED_RESUME_PRIMARY_COUNT,
        K_NEAR_TOLERANCE_TWO, sample,
        "VerifyDropAfterResume.PrimaryDataCount"));
    // Secondary: no drop, expect all 8 frames
    EXPECT_TRUE(ExpectNearSampleStep(sample.SecondaryDataCount(), static_cast<int32_t>(K_RESUME_FRAME_COUNT),
        K_NEAR_TOLERANCE_ONE, sample,
        "VerifyDropAfterResume.SecondaryDataCount"));
    EXPECT_TRUE(AssertEqSampleStep(sample.PrimaryError(), 0, sample, "VerifyDropAfterResume.PrimaryError"));
    EXPECT_TRUE(AssertEqSampleStep(sample.SecondaryError(), 0, sample, "VerifyDropAfterResume.SecondaryError"));
}

void PrepareDropFlushStopResetSample(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    ASSERT_TRUE(AssertTrueSampleStep(sample.SetOutputFilePath(GetOutputFilePath()), sample,
        "RunDropFlushStopResetScenario.SetOutputFilePath"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.CreatePrimary(scenario.mime), sample,
        "RunDropFlushStopResetScenario.CreatePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigurePrimary(scenario), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.ConfigurePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.GetPrimarySurface(scenario), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.GetPrimarySurface"));
    ASSERT_TRUE(AssertEqSampleStep(sample.CreateSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.CreateSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigureSecondary(scenario), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.ConfigureSecondary"));
}

void RunFlushResumePhase(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    ASSERT_TRUE(AssertEqSampleStep(sample.StartPrimary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.FlushResume.StartPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StartSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.FlushResume.StartSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.SendFramesWithConfiguredPts(scenario, K_RESUME_FRAME_COUNT),
        AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.FlushResume.SendFramesWithConfiguredPts"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForDualEos(), sample,
        "RunDropFlushStopResetScenario.FlushResume.WaitForDualEos"));
    ASSERT_TRUE(AssertEqSampleStep(sample.FlushPrimary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.FlushResume.FlushPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.FlushSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.FlushResume.FlushSecondary"));
    sample.ResetDataCounts();
    ASSERT_TRUE(AssertEqSampleStep(sample.StartPrimary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.AfterFlush.StartPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StartSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.AfterFlush.StartSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.SendFramesWithConfiguredPts(scenario, K_RESUME_FRAME_COUNT),
        AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.AfterFlush.SendFramesWithConfiguredPts"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForDualEos(), sample,
        "RunDropFlushStopResetScenario.AfterFlush.WaitForDualEos"));
    VerifyDropAfterResume(sample, scenario);
}

void RunStopResumePhase(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    ASSERT_TRUE(AssertEqSampleStep(sample.StopPrimary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.StopResume.StopPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StopSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.StopResume.StopSecondary"));
    sample.ResetDataCounts();
    ASSERT_TRUE(AssertEqSampleStep(sample.StartPrimary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.AfterStop.StartPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StartSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.AfterStop.StartSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.SendFramesWithConfiguredPts(scenario, K_RESUME_FRAME_COUNT),
        AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.AfterStop.SendFramesWithConfiguredPts"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForDualEos(), sample,
        "RunDropFlushStopResetScenario.AfterStop.WaitForDualEos"));
    VerifyDropAfterResume(sample, scenario);
}

void RunSetParameterPhase(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    sample.ResetDataCounts();
    ASSERT_TRUE(AssertEqSampleStep(sample.SetParameterPrimary(MakeDropRate(K_SET_PARAMETER_DROP_FRAME_RATE)),
        AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.SetParameter.SetParameterPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StopPrimary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.SetParameter.StopPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StopSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.SetParameter.StopSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StartPrimary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.SetParameter.StartPrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.StartSecondary(), AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.SetParameter.StartSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.SendFramesWithConfiguredPts(scenario, K_RESUME_FRAME_COUNT),
        AV_ERR_OK, sample,
        "RunDropFlushStopResetScenario.SetParameter.SendFramesWithConfiguredPts"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForDualEos(), sample,
        "RunDropFlushStopResetScenario.SetParameter.WaitForDualEos"));
    // 30fps->25fps with 8 frames: expect ~7 kept (only ~1 dropped)
    EXPECT_TRUE(ExpectNearSampleStep(sample.PrimaryDataCount(), K_EXPECTED_SET_PARAMETER_PRIMARY_COUNT,
        K_NEAR_TOLERANCE_ONE, sample,
        "RunDropFlushStopResetScenario.SetParameter.PrimaryDataCount"));
    EXPECT_TRUE(AssertEqSampleStep(sample.PrimaryError(), 0, sample,
        "RunDropFlushStopResetScenario.SetParameter.PrimaryError"));
    EXPECT_TRUE(AssertEqSampleStep(sample.SecondaryError(), 0, sample,
        "RunDropFlushStopResetScenario.SetParameter.SecondaryError"));
}

void RunDropFlushStopResetScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_NO_FATAL_FAILURE(PrepareDropFlushStopResetSample(sample, scenario));
    ASSERT_NO_FATAL_FAILURE(RunFlushResumePhase(sample, scenario));
    ASSERT_NO_FATAL_FAILURE(RunStopResumePhase(sample, scenario));
    ASSERT_NO_FATAL_FAILURE(RunSetParameterPhase(sample, scenario));
}

void RunInvalidParameterScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(AssertTrueSampleStep(sample.SetOutputFilePath(GetOutputFilePath()), sample,
        "RunInvalidParameterScenario.SetOutputFilePath"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.CreatePrimary(scenario.mime), sample,
        "RunInvalidParameterScenario.CreatePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigurePrimary(scenario), scenario.expectedResult, sample,
        "RunInvalidParameterScenario.ConfigurePrimary"));
}

void RunInvalidLifecycleScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(AssertTrueSampleStep(sample.SetOutputFilePath(GetOutputFilePath()), sample,
        "RunInvalidLifecycleScenario.SetOutputFilePath"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.CreatePrimary(scenario.mime), sample,
        "RunInvalidLifecycleScenario.CreatePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigureSecondary(scenario), scenario.expectedResult, sample,
        "RunInvalidLifecycleScenario.ConfigureSecondaryBeforeCreateSecondary"));
}

void RunDuplicateSecondaryScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(AssertTrueSampleStep(sample.SetOutputFilePath(GetOutputFilePath()), sample,
        "RunDuplicateSecondaryScenario.SetOutputFilePath"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.CreatePrimary(scenario.mime), sample,
        "RunDuplicateSecondaryScenario.CreatePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigurePrimary(scenario), AV_ERR_OK, sample,
        "RunDuplicateSecondaryScenario.ConfigurePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.GetPrimarySurface(scenario), AV_ERR_OK, sample,
        "RunDuplicateSecondaryScenario.GetPrimarySurface"));
    ASSERT_TRUE(AssertEqSampleStep(sample.CreateSecondary(), AV_ERR_OK, sample,
        "RunDuplicateSecondaryScenario.CreateSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigureSecondary(scenario), AV_ERR_OK, sample,
        "RunDuplicateSecondaryScenario.ConfigureSecondary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.DuplicateSecondary(), scenario.expectedResult, sample,
        "RunDuplicateSecondaryScenario.DuplicateSecondary"));
}

void PreparePrimaryScenario(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    ASSERT_TRUE(AssertTrueSampleStep(sample.SetOutputFilePath(GetOutputFilePath()), sample,
        "RunScenario.SetOutputFilePath"));
    ASSERT_TRUE(AssertTrueSampleStep(sample.CreatePrimary(scenario.mime), sample, "RunScenario.CreatePrimary"));
    ASSERT_TRUE(AssertEqSampleStep(sample.ConfigurePrimary(scenario), scenario.expectedResult, sample,
        "RunScenario.ConfigurePrimary"));
}

void PrepareSecondaryAndStart(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    ASSERT_TRUE(AssertEqSampleStep(sample.GetPrimarySurface(scenario), AV_ERR_OK, sample,
        "RunScenario.GetPrimarySurface"));
    if (NeedsSecondary(scenario.encoderMode)) {
        ASSERT_TRUE(AssertEqSampleStep(sample.CreateSecondary(), AV_ERR_OK, sample, "RunScenario.CreateSecondary"));
        ASSERT_TRUE(AssertEqSampleStep(sample.ConfigureSecondary(scenario), AV_ERR_OK, sample,
            "RunScenario.ConfigureSecondary"));
    }
    ASSERT_TRUE(AssertEqSampleStep(sample.StartPrimary(), AV_ERR_OK, sample, "RunScenario.StartPrimary"));
    if (NeedsSecondary(scenario.encoderMode)) {
        ASSERT_TRUE(AssertEqSampleStep(sample.StartSecondary(), AV_ERR_OK, sample, "RunScenario.StartSecondary"));
    }
}

void WaitForExpectedEos(VideoEncPreprocDualOutputSample &sample, EncoderEnableMode encoderMode)
{
    switch (encoderMode) {
        case EncoderEnableMode::PRIMARY_ONLY:
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_PRIMARY_ONLY:
            ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForPrimaryEos(), sample, "RunScenario.WaitForPrimaryEos"));
            EXPECT_TRUE(AssertTrueSampleStep(sample.PrimaryHasData(), sample, "RunScenario.PrimaryHasData"));
            break;
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_SECONDARY_ONLY:
            ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForSecondaryEos(), sample, "RunScenario.WaitForSecondaryEos"));
            EXPECT_TRUE(AssertTrueSampleStep(sample.SecondaryHasData(), sample, "RunScenario.SecondaryHasData"));
            break;
        case EncoderEnableMode::PRIMARY_SECONDARY:
            ASSERT_TRUE(AssertTrueSampleStep(sample.WaitForDualEos(), sample, "RunScenario.WaitForDualEos"));
            EXPECT_TRUE(AssertTrueSampleStep(sample.PrimaryHasData(), sample, "RunScenario.PrimaryHasData"));
            EXPECT_TRUE(AssertTrueSampleStep(sample.SecondaryHasData(), sample, "RunScenario.SecondaryHasData"));
            break;
        default:
            GTEST_FAIL() << "unexpected encoder mode\n" << DescribeSampleState(sample);
            break;
    }
}

void VerifyScenarioResult(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    if (ExpectsPrimaryOutput(scenario.encoderMode) && scenario.primary.useDropToFrameRate) {
        EXPECT_TRUE(ExpectLeSampleStep(sample.PrimaryDataCount(), static_cast<int32_t>(K_FRAME_COUNT), sample,
            "RunScenario.PrimaryDropDataCount"));
        if (ExpectsSecondaryOutput(scenario.encoderMode) && !scenario.secondary.useDropToFrameRate) {
            EXPECT_TRUE(ExpectGeSampleStep(sample.SecondaryDataCount(), sample.PrimaryDataCount(), sample,
                "RunScenario.SecondaryDataCountGreaterThanPrimary"));
        }
    }

    EXPECT_TRUE(AssertEqSampleStep(sample.PrimaryError(), 0, sample, "RunScenario.PrimaryError"));
    if (NeedsSecondary(scenario.encoderMode)) {
        EXPECT_TRUE(AssertEqSampleStep(sample.SecondaryError(), 0, sample, "RunScenario.SecondaryError"));
    }
}

void CleanupPendingEos(VideoEncPreprocDualOutputSample &sample, EncoderEnableMode encoderMode)
{
    if (NeedsSecondary(encoderMode) && !sample.SecondaryGotEos()) {
        (void)sample.NotifySecondaryEos();
        (void)sample.WaitForSecondaryEos(K_EOS_CLEANUP_TIMEOUT_SECONDS);
    }
    if (!sample.PrimaryGotEos()) {
        (void)sample.NotifyPrimaryEos();
        (void)sample.WaitForPrimaryEos(K_EOS_CLEANUP_TIMEOUT_SECONDS);
    }
}

void RunScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_NO_FATAL_FAILURE(PreparePrimaryScenario(sample, scenario));
    if (scenario.expectedResult != AV_ERR_OK) {
        return;
    }
    ASSERT_NO_FATAL_FAILURE(PrepareSecondaryAndStart(sample, scenario));
    ASSERT_TRUE(AssertEqSampleStep(sample.SendFrames(scenario, K_FRAME_COUNT), AV_ERR_OK, sample,
        "RunScenario.SendFrames"));
    ASSERT_NO_FATAL_FAILURE(WaitForExpectedEos(sample, scenario.encoderMode));
    VerifyScenarioResult(sample, scenario);
    CleanupPendingEos(sample, scenario.encoderMode);
}

void RunScenarioByFlow(const ScenarioCase &scenario)
{
    SCOPED_TRACE(DescribeScenario(scenario));
    switch (scenario.flow) {
        case FlowType::DROP_RATIO:
            RunDropRatioScenario(scenario);
            break;
        case FlowType::DROP_FLUSH_STOP_RESET:
            RunDropFlushStopResetScenario(scenario);
            break;
        case FlowType::INVALID_PARAMETER:
            RunInvalidParameterScenario(scenario);
            break;
        case FlowType::INVALID_LIFECYCLE:
            RunInvalidLifecycleScenario(scenario);
            break;
        case FlowType::DUPLICATE_SECONDARY:
            RunDuplicateSecondaryScenario(scenario);
            break;
        default:
            RunScenario(scenario);
            break;
    }
}
} // namespace

INSTANTIATE_TEST_SUITE_P(Level0, VideoEncPreprocDualOutputLevel0Test, testing::ValuesIn(GetLevel0Scenarios()),
                         [](const testing::TestParamInfo<ScenarioCase> &info) {
                             return SanitizeName(info.param.name);
                         });

/**
 * @tc.name: Level0
 * @tc.desc: preproc dual-output smoke scenarios on primary surface input.
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncPreprocDualOutputLevel0Test, Level0, TestSize.Level0)
{
    RunScenarioByFlow(GetParam());
}

#ifdef SUPPORT_VIDEO_ENCODER_PREPROCESSING
class VideoEncPreprocDualOutputLevel1Test : public testing::TestWithParam<ScenarioCase> {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

void AddBaseFormatScenarios(std::vector<ScenarioCase> &scenarios, const std::vector<ScenarioCase> &level0)
{
    for (const auto &scenario : GetSupportedScenarios()) {
        if (ContainsScenarioName(level0, scenario.name)) {
            continue;
        }
        if (IsBaseCodecResolutionFormat(scenario)) {
            AddScenarioIfMissing(scenarios, scenario);
        }
    }
}

void AddCodecCoverageScenarios(std::vector<ScenarioCase> &scenarios, const std::vector<ScenarioCase> &level0)
{
    for (const auto &codec : CodecDimensions()) {
        bool added = AddFirstScenarioMatching(scenarios, [&codec](const ScenarioCase &scenario) {
            return scenario.codecLabel == codec.label && scenario.flow == FlowType::MIXED_PREPROC &&
                   IsDualRuntimeNone(scenario);
        }, &level0);
        if (!added) {
            (void)AddFirstScenarioMatching(scenarios, [&codec](const ScenarioCase &scenario) {
                return scenario.codecLabel == codec.label && IsDualRuntimeNone(scenario);
            }, &level0);
        }
    }
}

void AddNativeFormatCoverageScenarios(std::vector<ScenarioCase> &scenarios, const std::vector<ScenarioCase> &level0)
{
    for (const auto &format : NativeFormatDimensions()) {
        bool added = AddFirstScenarioMatching(scenarios, [&format](const ScenarioCase &scenario) {
            return scenario.nativeFormat == format.nativeFormat && scenario.flow == FlowType::MIXED_PREPROC &&
                   IsDualRuntimeNone(scenario);
        }, &level0);
        if (!added) {
            (void)AddFirstScenarioMatching(scenarios, [&format](const ScenarioCase &scenario) {
                return scenario.nativeFormat == format.nativeFormat && IsDualRuntimeNone(scenario);
            }, &level0);
        }
    }
}

void AddResolutionCoverageScenarios(std::vector<ScenarioCase> &scenarios, const std::vector<ScenarioCase> &level0)
{
    for (const auto &resolution : ResolutionDimensions()) {
        bool added = AddFirstScenarioMatching(scenarios, [&resolution](const ScenarioCase &scenario) {
            return scenario.width == resolution.width && scenario.height == resolution.height &&
                   scenario.flow == FlowType::MIXED_PREPROC && IsDualRuntimeNone(scenario);
        }, &level0);
        if (!added) {
            (void)AddFirstScenarioMatching(scenarios, [&resolution](const ScenarioCase &scenario) {
                return scenario.width == resolution.width && scenario.height == resolution.height &&
                       IsDualRuntimeNone(scenario);
            }, &level0);
        }
    }
}

bool IsSpecialFlowScenario(const ScenarioCase &scenario)
{
    return scenario.flow == FlowType::INVALID_PARAMETER || scenario.flow == FlowType::INVALID_LIFECYCLE ||
           scenario.flow == FlowType::DUPLICATE_SECONDARY || scenario.flow == FlowType::DROP_RATIO ||
           scenario.flow == FlowType::DROP_FLUSH_STOP_RESET;
}

void AddSpecialFlowScenarios(std::vector<ScenarioCase> &scenarios, const std::vector<ScenarioCase> &level0)
{
    for (const auto &scenario : GetSupportedScenarios()) {
        if (ContainsScenarioName(level0, scenario.name)) {
            continue;
        }
        if (IsSpecialFlowScenario(scenario)) {
            AddScenarioIfMissing(scenarios, scenario);
        }
    }
}

std::vector<ScenarioCase> BuildLevel1Scenarios()
{
    std::vector<ScenarioCase> scenarios;
    const auto &level0 = GetLevel0Scenarios();
    AddBaseFormatScenarios(scenarios, level0);
    AddCodecCoverageScenarios(scenarios, level0);
    AddNativeFormatCoverageScenarios(scenarios, level0);
    AddResolutionCoverageScenarios(scenarios, level0);
    AddSpecialFlowScenarios(scenarios, level0);
    return scenarios;
}

const std::vector<ScenarioCase> &GetLevel1Scenarios()
{
    static const std::vector<ScenarioCase> scenarios = BuildLevel1Scenarios();
    return scenarios;
}


INSTANTIATE_TEST_SUITE_P(Level1, VideoEncPreprocDualOutputLevel1Test, testing::ValuesIn(GetLevel1Scenarios()),
                         [](const testing::TestParamInfo<ScenarioCase> &info) {
                             return SanitizeName(info.param.name);
                         });

/**
 * @tc.name: Level1
 * @tc.desc: preproc dual-output core matrix scenarios on primary surface input.
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncPreprocDualOutputLevel1Test, Level1, TestSize.Level1)
{
    RunScenarioByFlow(GetParam());
}
#endif
