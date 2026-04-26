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
#include <iostream>
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
constexpr uint32_t kFrameCount = 30;

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

class VideoEncPreprocDualOutputTest : public testing::TestWithParam<ScenarioCase> {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
};

class VideoEncPreprocDualOutputInfoTest : public testing::TestWithParam<ScenarioCase> {};

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

const char *BoolToString(bool value)
{
    return value ? "true" : "false";
}

std::string DescribeRuntimeDelivery(const ScenarioCase &scenario)
{
    if (scenario.runtimeOperation == RuntimeOperation::NONE) {
        return "not scheduled";
    }

    uint32_t triggerFrameIndex = kFrameCount / 2;
    std::ostringstream oss;
    oss << "frameNumber=" << (triggerFrameIndex + 1) << ", frameIndex=" << triggerFrameIndex
        << ", sendFrameCount=" << kFrameCount;
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
        << scenario.codecPixelFormat << ")\n"
        << "  modes: encoder=" << ToString(scenario.encoderMode) << ", preproc="
        << ToString(scenario.preprocMode) << ", runtime=" << ToString(scenario.runtimeOperation) << "\n"
        << "  invalidParameterType: " << ToString(scenario.invalidParameterType) << "\n"
        << "  primaryPreproc: " << DescribePreprocConfig(scenario.primary) << "\n"
        << "  secondaryPreproc: " << DescribePreprocConfig(scenario.secondary) << "\n"
        << "  runtimeDelivery: " << DescribeRuntimeDelivery(scenario) << "\n"
        << "  runtimePrimaryPreproc: " << DescribePreprocConfig(scenario.runtimePrimary) << "\n"
        << "  runtimeSecondaryPreproc: " << DescribePreprocConfig(scenario.runtimeSecondary) << "\n"
        << "  expectation: result=" << scenario.expectedResult << ", exact="
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

#define ASSERT_TRUE_SAMPLE_STEP(condition, sample, step) \
    do {                                                 \
        SCOPED_TRACE(DescribeStep(step));                \
        ASSERT_TRUE(condition) << DescribeSampleState(sample); \
    } while (false)

#define ASSERT_EQ_SAMPLE_STEP(actual, expected, sample, step)  \
    do {                                                       \
        SCOPED_TRACE(DescribeStep(step));                      \
        ASSERT_EQ((actual), (expected)) << DescribeSampleState(sample); \
    } while (false)

#define EXPECT_TRUE_SAMPLE_STEP(condition, sample, step)       \
    do {                                                       \
        SCOPED_TRACE(DescribeStep(step));                      \
        EXPECT_TRUE(condition) << DescribeSampleState(sample);  \
    } while (false)

#define EXPECT_EQ_SAMPLE_STEP(actual, expected, sample, step)  \
    do {                                                       \
        SCOPED_TRACE(DescribeStep(step));                      \
        EXPECT_EQ((actual), (expected)) << DescribeSampleState(sample); \
    } while (false)

#define EXPECT_NEAR_SAMPLE_STEP(actual, expected, absError, sample, step) \
    do {                                                                 \
        SCOPED_TRACE(DescribeStep(step));                                \
        EXPECT_NEAR((actual), (expected), (absError)) << DescribeSampleState(sample); \
    } while (false)

#define EXPECT_LE_SAMPLE_STEP(actual, expected, sample, step)   \
    do {                                                        \
        SCOPED_TRACE(DescribeStep(step));                       \
        EXPECT_LE((actual), (expected)) << DescribeSampleState(sample); \
    } while (false)

#define EXPECT_LT_SAMPLE_STEP(actual, expected, sample, step)   \
    do {                                                        \
        SCOPED_TRACE(DescribeStep(step));                       \
        EXPECT_LT((actual), (expected)) << DescribeSampleState(sample); \
    } while (false)

#define EXPECT_GE_SAMPLE_STEP(actual, expected, sample, step)   \
    do {                                                        \
        SCOPED_TRACE(DescribeStep(step));                       \
        EXPECT_GE((actual), (expected)) << DescribeSampleState(sample); \
    } while (false)

#define EXPECT_GT_SAMPLE_STEP(actual, expected, sample, step)   \
    do {                                                        \
        SCOPED_TRACE(DescribeStep(step));                       \
        EXPECT_GT((actual), (expected)) << DescribeSampleState(sample); \
    } while (false)

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

PreprocConfig MakeRuntimeConfig(int32_t width, int32_t height, uint32_t seed)
{
    if (seed % 2 == 0) {
        return MakeDropRate(12.0);
    }
    return MakeDownsampling(width / 2, height / 2, 12.0);
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
            scenario.primary = MakeDropRate(15.0);
            break;
        case PreprocEnableMode::SECONDARY_ONLY:
            scenario.secondary = MakeDropRate(15.0);
            break;
        case PreprocEnableMode::BOTH_SAME:
            scenario.primary = MakeDownsampling(scenario.width / 2, scenario.height / 2, 15.0);
            scenario.secondary = scenario.primary;
            break;
        case PreprocEnableMode::BOTH_DIFFERENT:
            scenario.primary = MakeDownsampling(scenario.width / 2, scenario.height / 2, 15.0);
            scenario.secondary = MakeCrop(0, 0, scenario.width - 1, scenario.height - 1, 10.0);
            break;
        case PreprocEnableMode::DISABLED:
        default:
            break;
    }
    scenario.runtimePrimary = MakeRuntimeConfig(scenario.width, scenario.height, 0);
    scenario.runtimeSecondary = MakeRuntimeConfig(scenario.width, scenario.height, 1);
}

ScenarioCase MakeScenario(const CodecDimension &codec, const ResolutionDimension &resolution,
                          const NativeFormatDimension &format, EncoderEnableMode encoderMode,
                          PreprocEnableMode preprocMode, RuntimeOperation runtimeOperation)
{
    ScenarioCase scenario {};
    scenario.codecLabel = codec.label;
    scenario.mime = codec.mime;
    scenario.category = codec.category;
    scenario.width = resolution.width;
    scenario.height = resolution.height;
    scenario.runtimeWidth = std::max(64, resolution.width / 2);
    scenario.runtimeHeight = std::max(64, resolution.height / 2);
    scenario.nativeFormat = format.nativeFormat;
    scenario.codecPixelFormat = format.codecPixelFormat;
    scenario.encoderMode = encoderMode;
    scenario.preprocMode = preprocMode;
    scenario.runtimeOperation = runtimeOperation;
    scenario.flow = MakeFlowType(preprocMode);
    scenario.expectedResult = AV_ERR_OK;
    scenario.expectExactResult = true;
    scenario.needsFrameRotation = true;
    ApplyPreprocMode(scenario);
    scenario.name = scenario.codecLabel + "_" + resolution.label + "_" + format.label + "_" +
                    ToString(encoderMode) + "_" + ToString(preprocMode) + "_" + ToString(runtimeOperation);
    return scenario;
}

std::vector<ScenarioCase> BuildSupportedScenarios()
{
    std::vector<ScenarioCase> scenarios;
    for (const auto &codec : CodecDimensions()) {
        OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codec.mime, true, codec.category);
        if (capability == nullptr) {
            continue;
        }
        for (const auto &resolution : ResolutionDimensions()) {
            if (!OH_AVCapability_IsVideoSizeSupported(capability, resolution.width, resolution.height)) {
                continue;
            }
            for (const auto &format : NativeFormatDimensions()) {
                if (!IsNativeFormatSupported(capability, format.nativeFormat)) {
                    continue;
                }
                for (auto encoderMode : EncoderModeDimensions()) {
                    for (auto preprocMode : PreprocModeDimensions()) {
                        for (auto runtimeOperation : RuntimeOperationDimensions()) {
                            scenarios.emplace_back(MakeScenario(codec, resolution, format, encoderMode, preprocMode,
                                                                runtimeOperation));
                        }
                    }
                }
            }
        }
    }
    return scenarios;
}

const std::vector<ScenarioCase> &GetSupportedScenarios()
{
    static std::vector<ScenarioCase> scenarios = BuildSupportedScenarios();
    static bool once = false;
    if (!once) {
        once = true;
        for (const auto &s : kDualOutputScenarios) {
            scenarios.push_back(s);
        }
    }
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
    return "./1in2out_out/" + testName + ".bin";
}

void RunDropRatioScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE_SAMPLE_STEP(sample.SetOutputFilePath(GetOutputFilePath()), sample,
                            "RunDropRatioScenario.SetOutputFilePath");
    ASSERT_TRUE_SAMPLE_STEP(sample.CreatePrimary(scenario.mime), sample, "RunDropRatioScenario.CreatePrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.ConfigurePrimary(scenario), AV_ERR_OK, sample,
                          "RunDropRatioScenario.ConfigurePrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.GetPrimarySurface(scenario), AV_ERR_OK, sample,
                          "RunDropRatioScenario.GetPrimarySurface");
    ASSERT_EQ_SAMPLE_STEP(sample.CreateSecondary(), AV_ERR_OK, sample, "RunDropRatioScenario.CreateSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.ConfigureSecondary(scenario), AV_ERR_OK, sample,
                          "RunDropRatioScenario.ConfigureSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.StartPrimary(), AV_ERR_OK, sample, "RunDropRatioScenario.StartPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StartSecondary(), AV_ERR_OK, sample, "RunDropRatioScenario.StartSecondary");
    constexpr uint32_t kDropFrames = 30;
    ASSERT_EQ_SAMPLE_STEP(sample.SendFramesFixedPts(scenario, kDropFrames, 0), AV_ERR_OK, sample,
                          "RunDropRatioScenario.SendFramesFixedPts");
    ASSERT_TRUE_SAMPLE_STEP(sample.WaitForDualEos(), sample, "RunDropRatioScenario.WaitForDualEos");
    // Primary: 30fps->15fps (keepRatio=0.5), expect ~15 kept out of 30
    EXPECT_NEAR_SAMPLE_STEP(sample.PrimaryDataCount(), 15, 3, sample, "RunDropRatioScenario.PrimaryDataCount");
    // Secondary: no drop, expect all 30 frames
    EXPECT_NEAR_SAMPLE_STEP(sample.SecondaryDataCount(), static_cast<int32_t>(kDropFrames), 2, sample,
                            "RunDropRatioScenario.SecondaryDataCount");
    EXPECT_EQ_SAMPLE_STEP(sample.PrimaryError(), 0, sample, "RunDropRatioScenario.PrimaryError");
    EXPECT_EQ_SAMPLE_STEP(sample.SecondaryError(), 0, sample, "RunDropRatioScenario.SecondaryError");
}

void VerifyDropAfterResume(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    (void)scenario;
    // Primary: 30fps->15fps (keepRatio=0.5), 8 frames sent, expect ~4-5 kept
    EXPECT_NEAR_SAMPLE_STEP(sample.PrimaryDataCount(), 4, 2, sample, "VerifyDropAfterResume.PrimaryDataCount");
    // Secondary: no drop, expect all 8 frames
    EXPECT_NEAR_SAMPLE_STEP(sample.SecondaryDataCount(), 8, 1, sample,
                            "VerifyDropAfterResume.SecondaryDataCount");
    EXPECT_EQ_SAMPLE_STEP(sample.PrimaryError(), 0, sample, "VerifyDropAfterResume.PrimaryError");
    EXPECT_EQ_SAMPLE_STEP(sample.SecondaryError(), 0, sample, "VerifyDropAfterResume.SecondaryError");
}

void RunDropFlushStopResetScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE_SAMPLE_STEP(sample.SetOutputFilePath(GetOutputFilePath()), sample,
                            "RunDropFlushStopResetScenario.SetOutputFilePath");
    ASSERT_TRUE_SAMPLE_STEP(sample.CreatePrimary(scenario.mime), sample,
                            "RunDropFlushStopResetScenario.CreatePrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.ConfigurePrimary(scenario), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.ConfigurePrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.GetPrimarySurface(scenario), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.GetPrimarySurface");
    ASSERT_EQ_SAMPLE_STEP(sample.CreateSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.CreateSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.ConfigureSecondary(scenario), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.ConfigureSecondary");

    // Flush + resume
    ASSERT_EQ_SAMPLE_STEP(sample.StartPrimary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.FlushResume.StartPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StartSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.FlushResume.StartSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.FlushResume.SendFramesFixedPts");
    ASSERT_TRUE_SAMPLE_STEP(sample.WaitForDualEos(), sample,
                            "RunDropFlushStopResetScenario.FlushResume.WaitForDualEos");
    ASSERT_EQ_SAMPLE_STEP(sample.FlushPrimary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.FlushResume.FlushPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.FlushSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.FlushResume.FlushSecondary");
    sample.ResetDataCounts();
    ASSERT_EQ_SAMPLE_STEP(sample.StartPrimary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.AfterFlush.StartPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StartSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.AfterFlush.StartSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.AfterFlush.SendFramesFixedPts");
    ASSERT_TRUE_SAMPLE_STEP(sample.WaitForDualEos(), sample,
                            "RunDropFlushStopResetScenario.AfterFlush.WaitForDualEos");
    VerifyDropAfterResume(sample, scenario);

    // Stop + resume
    ASSERT_EQ_SAMPLE_STEP(sample.StopPrimary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.StopResume.StopPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StopSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.StopResume.StopSecondary");
    sample.ResetDataCounts();
    ASSERT_EQ_SAMPLE_STEP(sample.StartPrimary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.AfterStop.StartPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StartSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.AfterStop.StartSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.AfterStop.SendFramesFixedPts");
    ASSERT_TRUE_SAMPLE_STEP(sample.WaitForDualEos(), sample,
                            "RunDropFlushStopResetScenario.AfterStop.WaitForDualEos");
    VerifyDropAfterResume(sample, scenario);

    // SetParameter to change drop rate to 30fps->25fps (keepRatio~0.83)
    sample.ResetDataCounts();
    ASSERT_EQ_SAMPLE_STEP(sample.SetParameterPrimary(MakeDropRate(25.0)), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.SetParameter.SetParameterPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StopPrimary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.SetParameter.StopPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StopSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.SetParameter.StopSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.StartPrimary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.SetParameter.StartPrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.StartSecondary(), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.SetParameter.StartSecondary");
    ASSERT_EQ_SAMPLE_STEP(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK, sample,
                          "RunDropFlushStopResetScenario.SetParameter.SendFramesFixedPts");
    ASSERT_TRUE_SAMPLE_STEP(sample.WaitForDualEos(), sample,
                            "RunDropFlushStopResetScenario.SetParameter.WaitForDualEos");
    // 30fps->25fps with 8 frames: expect ~7 kept (only ~1 dropped)
    EXPECT_NEAR_SAMPLE_STEP(sample.PrimaryDataCount(), 7, 1, sample,
                            "RunDropFlushStopResetScenario.SetParameter.PrimaryDataCount");
    EXPECT_EQ_SAMPLE_STEP(sample.PrimaryError(), 0, sample,
                          "RunDropFlushStopResetScenario.SetParameter.PrimaryError");
    EXPECT_EQ_SAMPLE_STEP(sample.SecondaryError(), 0, sample,
                          "RunDropFlushStopResetScenario.SetParameter.SecondaryError");
}

void RunInvalidParameterScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE_SAMPLE_STEP(sample.SetOutputFilePath(GetOutputFilePath()), sample,
                            "RunInvalidParameterScenario.SetOutputFilePath");
    ASSERT_TRUE_SAMPLE_STEP(sample.CreatePrimary(scenario.mime), sample,
                            "RunInvalidParameterScenario.CreatePrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.ConfigurePrimary(scenario), scenario.expectedResult, sample,
                          "RunInvalidParameterScenario.ConfigurePrimary");
}

void RunScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE_SAMPLE_STEP(sample.SetOutputFilePath(GetOutputFilePath()), sample, "RunScenario.SetOutputFilePath");
    ASSERT_TRUE_SAMPLE_STEP(sample.CreatePrimary(scenario.mime), sample, "RunScenario.CreatePrimary");
    ASSERT_EQ_SAMPLE_STEP(sample.ConfigurePrimary(scenario), scenario.expectedResult, sample,
                          "RunScenario.ConfigurePrimary");
    if (scenario.expectedResult != AV_ERR_OK) {
        return;
    }
    ASSERT_EQ_SAMPLE_STEP(sample.GetPrimarySurface(scenario), AV_ERR_OK, sample, "RunScenario.GetPrimarySurface");

    if (NeedsSecondary(scenario.encoderMode)) {
        ASSERT_EQ_SAMPLE_STEP(sample.CreateSecondary(), AV_ERR_OK, sample, "RunScenario.CreateSecondary");
        ASSERT_EQ_SAMPLE_STEP(sample.ConfigureSecondary(scenario), AV_ERR_OK, sample,
                              "RunScenario.ConfigureSecondary");
    }

    ASSERT_EQ_SAMPLE_STEP(sample.StartPrimary(), AV_ERR_OK, sample, "RunScenario.StartPrimary");
    if (NeedsSecondary(scenario.encoderMode)) {
        ASSERT_EQ_SAMPLE_STEP(sample.StartSecondary(), AV_ERR_OK, sample, "RunScenario.StartSecondary");
    }

    ASSERT_EQ_SAMPLE_STEP(sample.SendFrames(scenario, kFrameCount), AV_ERR_OK, sample, "RunScenario.SendFrames");

    switch (scenario.encoderMode) {
        case EncoderEnableMode::PRIMARY_ONLY:
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_PRIMARY_ONLY:
            ASSERT_TRUE_SAMPLE_STEP(sample.WaitForPrimaryEos(), sample, "RunScenario.WaitForPrimaryEos");
            EXPECT_TRUE_SAMPLE_STEP(sample.PrimaryHasData(), sample, "RunScenario.PrimaryHasData");
            break;
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_SECONDARY_ONLY:
            ASSERT_TRUE_SAMPLE_STEP(sample.WaitForSecondaryEos(), sample, "RunScenario.WaitForSecondaryEos");
            EXPECT_TRUE_SAMPLE_STEP(sample.SecondaryHasData(), sample, "RunScenario.SecondaryHasData");
            break;
        case EncoderEnableMode::PRIMARY_SECONDARY:
            ASSERT_TRUE_SAMPLE_STEP(sample.WaitForDualEos(), sample, "RunScenario.WaitForDualEos");
            EXPECT_TRUE_SAMPLE_STEP(sample.PrimaryHasData(), sample, "RunScenario.PrimaryHasData");
            EXPECT_TRUE_SAMPLE_STEP(sample.SecondaryHasData(), sample, "RunScenario.SecondaryHasData");
            break;
        default:
            GTEST_FAIL() << "unexpected encoder mode\n" << DescribeSampleState(sample);
            break;
    }
    if (ExpectsPrimaryOutput(scenario.encoderMode) && scenario.primary.useDropToFrameRate) {
        EXPECT_LE_SAMPLE_STEP(sample.PrimaryDataCount(), static_cast<int32_t>(kFrameCount), sample,
                              "RunScenario.PrimaryDropDataCount");
        if (ExpectsSecondaryOutput(scenario.encoderMode) && !scenario.secondary.useDropToFrameRate) {
            EXPECT_GE_SAMPLE_STEP(sample.SecondaryDataCount(), sample.PrimaryDataCount(), sample,
                                  "RunScenario.SecondaryDataCountGreaterThanPrimary");
        }
    }

    EXPECT_EQ_SAMPLE_STEP(sample.PrimaryError(), 0, sample, "RunScenario.PrimaryError");
    if (NeedsSecondary(scenario.encoderMode)) {
        EXPECT_EQ_SAMPLE_STEP(sample.SecondaryError(), 0, sample, "RunScenario.SecondaryError");
    }

    if (NeedsSecondary(scenario.encoderMode) && !sample.SecondaryGotEos()) {
        (void)sample.NotifySecondaryEos();
        (void)sample.WaitForSecondaryEos(2);
    }
    if (!sample.PrimaryGotEos()) {
        (void)sample.NotifyPrimaryEos();
        (void)sample.WaitForPrimaryEos(2);
    }
}
} // namespace

INSTANTIATE_TEST_SUITE_P(Dynamic, VideoEncPreprocDualOutputTest, testing::ValuesIn(GetSupportedScenarios()),
                         [](const testing::TestParamInfo<ScenarioCase> &info) {
                             return SanitizeName(info.param.name);
                         });

INSTANTIATE_TEST_SUITE_P(Query, VideoEncPreprocDualOutputInfoTest, testing::ValuesIn(GetSupportedScenarios()),
                         [](const testing::TestParamInfo<ScenarioCase> &info) {
                             return SanitizeName(info.param.name);
                         });

/**
 * @tc.name: VideoEncoder_Preproc_Dual_Output_PrintScenarioInfo
 * @tc.desc: print preproc dual-output scenario information by scenario name on demand.
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncPreprocDualOutputInfoTest, DISABLED_PrintScenarioInfo, TestSize.Level0)
{
    std::cout << DescribeScenario(GetParam()) << std::endl;
}

/**
 * @tc.name: VideoEncoder_Preproc_Dual_Output_001
 * @tc.desc: preproc dual-output dynamic dimension scenarios on primary surface input.
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncPreprocDualOutputTest, VideoEncoder_Preproc_Dual_Output_001, TestSize.Level1)
{
    const auto &scenario = GetParam();
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
        default:
            RunScenario(scenario);
            break;
    }
}
