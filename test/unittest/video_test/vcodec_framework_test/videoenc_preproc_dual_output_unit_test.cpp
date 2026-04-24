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
#include <string>
#include <vector>

#include "native_avcapability.h"
#include "native_avcodec_base.h"
#include "videoenc_preproc_dual_output_sample.h"

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::PreprocDualOutput;
using namespace testing::ext;

namespace {
constexpr uint32_t kFrameCount = 8;

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
    scenario.flow = FlowType::NO_PREPROC;
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
    ASSERT_TRUE(sample.SetOutputFilePath(GetOutputFilePath())) << scenario.name;
    ASSERT_TRUE(sample.CreatePrimary(scenario.mime)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.GetPrimarySurface(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.CreateSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.ConfigureSecondary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartSecondary(), AV_ERR_OK) << scenario.name;
    constexpr uint32_t kDropFrames = 30;
    ASSERT_EQ(sample.SendFramesFixedPts(scenario, kDropFrames, 0), AV_ERR_OK) << scenario.name;
    ASSERT_TRUE(sample.WaitForDualEos()) << scenario.name;
    // Primary: 30fps->15fps (keepRatio=0.5), expect ~15 kept out of 30
    EXPECT_NEAR(sample.PrimaryDataCount(), 15, 3) << scenario.name;
    // Secondary: no drop, expect all 30 frames
    EXPECT_NEAR(sample.SecondaryDataCount(), static_cast<int32_t>(kDropFrames), 2) << scenario.name;
    EXPECT_EQ(sample.PrimaryError(), 0) << scenario.name;
    EXPECT_EQ(sample.SecondaryError(), 0) << scenario.name;
}

void VerifyDropAfterResume(VideoEncPreprocDualOutputSample &sample, const ScenarioCase &scenario)
{
    // Primary: 30fps->15fps (keepRatio=0.5), 8 frames sent, expect ~4-5 kept
    EXPECT_NEAR(sample.PrimaryDataCount(), 4, 2) << scenario.name;
    // Secondary: no drop, expect all 8 frames
    EXPECT_NEAR(sample.SecondaryDataCount(), 8, 1) << scenario.name;
    EXPECT_EQ(sample.PrimaryError(), 0) << scenario.name;
    EXPECT_EQ(sample.SecondaryError(), 0) << scenario.name;
}

void RunDropFlushStopResetScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(sample.SetOutputFilePath(GetOutputFilePath())) << scenario.name;
    ASSERT_TRUE(sample.CreatePrimary(scenario.mime)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.GetPrimarySurface(scenario), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.CreateSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.ConfigureSecondary(scenario), AV_ERR_OK) << scenario.name;

    // Flush + resume
    ASSERT_EQ(sample.StartPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK) << scenario.name;
    ASSERT_TRUE(sample.WaitForDualEos()) << scenario.name;
    ASSERT_EQ(sample.FlushPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.FlushSecondary(), AV_ERR_OK) << scenario.name;
    sample.ResetDataCounts();
    ASSERT_EQ(sample.StartPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK) << scenario.name;
    ASSERT_TRUE(sample.WaitForDualEos()) << scenario.name;
    VerifyDropAfterResume(sample, scenario);

    // Stop + resume
    ASSERT_EQ(sample.StopPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StopSecondary(), AV_ERR_OK) << scenario.name;
    sample.ResetDataCounts();
    ASSERT_EQ(sample.StartPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK) << scenario.name;
    ASSERT_TRUE(sample.WaitForDualEos()) << scenario.name;
    VerifyDropAfterResume(sample, scenario);

    // SetParameter to change drop rate to 30fps->25fps (keepRatio~0.83)
    sample.ResetDataCounts();
    ASSERT_EQ(sample.SetParameterPrimary(MakeDropRate(25.0)), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StopPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StopSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartPrimary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.StartSecondary(), AV_ERR_OK) << scenario.name;
    ASSERT_EQ(sample.SendFramesFixedPts(scenario, 8, 0), AV_ERR_OK) << scenario.name;
    ASSERT_TRUE(sample.WaitForDualEos()) << scenario.name;
    // 30fps->25fps with 8 frames: expect ~7 kept (only ~1 dropped)
    EXPECT_NEAR(sample.PrimaryDataCount(), 7, 1) << scenario.name;
    EXPECT_EQ(sample.PrimaryError(), 0) << scenario.name;
    EXPECT_EQ(sample.SecondaryError(), 0) << scenario.name;
}

void RunInvalidParameterScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(sample.SetOutputFilePath(GetOutputFilePath())) << scenario.name;
    ASSERT_TRUE(sample.CreatePrimary(scenario.mime)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), scenario.expectedResult) << scenario.name;
}

void RunScenario(const ScenarioCase &scenario)
{
    VideoEncPreprocDualOutputSample sample;
    ASSERT_TRUE(sample.SetOutputFilePath(GetOutputFilePath())) << scenario.name;
    ASSERT_TRUE(sample.CreatePrimary(scenario.mime)) << scenario.name;
    ASSERT_EQ(sample.ConfigurePrimary(scenario), scenario.expectedResult) << scenario.name;
    if (scenario.expectedResult != AV_ERR_OK) {
        return;
    }
    ASSERT_EQ(sample.GetPrimarySurface(scenario), AV_ERR_OK) << scenario.name;

    if (NeedsSecondary(scenario.encoderMode)) {
        ASSERT_EQ(sample.CreateSecondary(), AV_ERR_OK) << scenario.name;
        ASSERT_EQ(sample.ConfigureSecondary(scenario), AV_ERR_OK) << scenario.name;
    }

    ASSERT_EQ(sample.StartPrimary(), AV_ERR_OK) << scenario.name;
    if (NeedsSecondary(scenario.encoderMode)) {
        ASSERT_EQ(sample.StartSecondary(), AV_ERR_OK) << scenario.name;
    }

    ASSERT_EQ(sample.SendFrames(scenario, kFrameCount), AV_ERR_OK) << scenario.name;

    switch (scenario.encoderMode) {
        case EncoderEnableMode::PRIMARY_ONLY:
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_PRIMARY_ONLY:
            ASSERT_TRUE(sample.WaitForPrimaryEos()) << scenario.name;
            EXPECT_TRUE(sample.PrimaryHasData()) << scenario.name;
            break;
        case EncoderEnableMode::PRIMARY_SECONDARY_SEND_SECONDARY_ONLY:
            ASSERT_TRUE(sample.WaitForSecondaryEos()) << scenario.name;
            EXPECT_TRUE(sample.SecondaryHasData()) << scenario.name;
            break;
        case EncoderEnableMode::PRIMARY_SECONDARY:
            ASSERT_TRUE(sample.WaitForDualEos()) << scenario.name;
            EXPECT_TRUE(sample.PrimaryHasData()) << scenario.name;
            EXPECT_TRUE(sample.SecondaryHasData()) << scenario.name;
            break;
        default:
            GTEST_FAIL() << "unexpected encoder mode";
            break;
    }
    if (scenario.primary.useDropToFrameRate) {
        EXPECT_LT(sample.PrimaryDataCount(), static_cast<int32_t>(kFrameCount)) << scenario.name;
        if (!scenario.secondary.useDropToFrameRate) {
            EXPECT_GT(sample.SecondaryDataCount(), sample.PrimaryDataCount()) << scenario.name;
        }
    }

    EXPECT_EQ(sample.PrimaryError(), 0) << scenario.name;
    EXPECT_EQ(sample.SecondaryError(), 0) << scenario.name;

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

/**
 * @tc.name: VideoEncoder_Preproc_Dual_Output_001
 * @tc.desc: preproc dual-output dynamic dimension scenarios on primary surface input.
 * @tc.type: FUNC
 */
HWTEST_P(VideoEncPreprocDualOutputTest, VideoEncoder_Preproc_Dual_Output_001, TestSize.Level1)
{
    const auto &scenario = GetParam();
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
