/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <atomic>
#include <cmath>
#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <thread>
#include <vector>

#include <gtest/gtest.h>
#include "fast_kits_interface.h"
#include "preprocessor.h"
#include "surface.h"
#include "surface_type.h"
#include "v1_0/buffer_handle_meta_key_type.h"
#include "v2_2/buffer_handle_meta_key_type.h"

namespace OHOS {
namespace MediaAVCodec {
namespace PreProcessing {
using namespace testing::ext;

namespace {
constexpr const char *MEDIA_DIR = "/data/test/media/preprocessor/";
constexpr int32_t INPUT_WIDTH = 1280;
constexpr int32_t INPUT_HEIGHT = 720;

// Metadata key namespaces
namespace V1_0 = OHOS::HDI::Display::Graphic::Common::V1_0;
namespace V2_2 = OHOS::HDI::Display::Graphic::Common::V2_2;

// Test HDR metadata
const std::vector<uint8_t> TEST_HDR_META_TYPE = {0, 0, 0, 0};
const std::vector<uint8_t> TEST_HDR_STATIC_DATA(48, 0xAB);
const std::vector<uint8_t> TEST_HDR_DYNAMIC_DATA = {0xDE, 0xAD, 0xBE, 0xEF};

void SetTestHDRMetadata(const sptr<SurfaceBuffer> &buf)
{
    (void)buf->SetMetadata(V1_0::ATTRKEY_HDR_METADATA_TYPE, TEST_HDR_META_TYPE);
    (void)buf->SetMetadata(V1_0::ATTRKEY_HDR_STATIC_METADATA, TEST_HDR_STATIC_DATA);
    (void)buf->SetMetadata(V1_0::ATTRKEY_HDR_DYNAMIC_METADATA, TEST_HDR_DYNAMIC_DATA);
}

void ExpectHDRMetadataEqual(const sptr<SurfaceBuffer> &input, const sptr<SurfaceBuffer> &output)
{
    for (auto key :
         {V1_0::ATTRKEY_HDR_METADATA_TYPE, V1_0::ATTRKEY_HDR_STATIC_METADATA, V1_0::ATTRKEY_HDR_DYNAMIC_METADATA}) {
        std::vector<uint8_t> inVal;
        std::vector<uint8_t> outVal;
        ASSERT_EQ(input->GetMetadata(key, inVal), GSERROR_OK) << "Get metadata " << key << " from input failed";
        ASSERT_EQ(output->GetMetadata(key, outVal), GSERROR_OK) << "Get metadata " << key << " from output failed";
        EXPECT_EQ(inVal, outVal) << "HDR metadata key " << key << " mismatch";
    }
}

void SetTestROIMetadata(const sptr<SurfaceBuffer> &buf, const std::string &roiStr)
{
    std::vector<uint8_t> data(roiStr.begin(), roiStr.end());
    (void)buf->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, data);
}

void ExpectROIMetadata(const sptr<SurfaceBuffer> &output, const std::string &expected)
{
    std::vector<uint8_t> outVal;
    ASSERT_EQ(output->GetMetadata(V2_2::ATTRKEY_ROI_METADATA, outVal), GSERROR_OK) << "Get ROI metadata failed";
    ASSERT_GE(outVal.size(), expected.size()) << "ROI metadata too short, expected " << expected.size()
                                              << " bytes, got " << outVal.size();
    std::string actual(outVal.begin(), outVal.begin() + expected.size());
    EXPECT_EQ(actual, expected) << "ROI mismatch, expected: " << expected << ", actual: " << actual;
}

// ROI transform: scale then offset
std::string TransformROI(const std::string &roiStr, double scaleX, double scaleY, int32_t offTop, int32_t offLeft)
{
    std::regex pattern(R"((-?\d+),(-?\d+)-(-?\d+),(-?\d+)(?:=(-?\d+))?(?:;|$))");
    std::smatch match;
    std::string temp = roiStr;
    std::string result;
    while (std::regex_search(temp, match, pattern)) {
        int32_t top = std::stoi(match[1].str());
        int32_t left = std::stoi(match[2].str());
        int32_t bottom = std::stoi(match[3].str());
        int32_t right = std::stoi(match[4].str());
        int32_t qp = match[5].matched ? std::stoi(match[5].str()) : -3;
        int32_t newTop = static_cast<int32_t>(std::round(top * scaleY)) - offTop;
        int32_t newLeft = static_cast<int32_t>(std::round(left * scaleX)) - offLeft;
        int32_t newBottom = static_cast<int32_t>(std::round(bottom * scaleY)) - offTop;
        int32_t newRight = static_cast<int32_t>(std::round(right * scaleX)) - offLeft;
        if (!result.empty()) {
            result += ";";
        }
        result += std::to_string(newTop) + "," + std::to_string(newLeft) + "-" + std::to_string(newBottom) + "," +
                  std::to_string(newRight) + "=" + std::to_string(qp);
        temp = match.suffix().str();
    }
    if (!result.empty()) {
        result += ";";
    }
    return result;
}

struct DsFormatParam {
    const char *name;
    const char *inputFile;
    int32_t pixelFormat;
};

const DsFormatParam DS_FORMAT_PARAMS[] = {
    {"RGBA8888", "/data/test/media/1280_720_0.rgba", GRAPHIC_PIXEL_FMT_RGBA_8888},
    {"RGBA1010102", "/data/test/media/1280_720_rgba1010102.rgb", GRAPHIC_PIXEL_FMT_RGBA_1010102},
    {"NV21", "/data/test/media/1280_720_nv.yuv", GRAPHIC_PIXEL_FMT_YCRCB_420_SP},
    {"NV12", "/data/test/media/1280_720_nv.yuv", GRAPHIC_PIXEL_FMT_YCBCR_420_SP},
    {"NV21_P010", "/data/test/media/1280_720_nv12_10bit.yuv", GRAPHIC_PIXEL_FMT_YCRCB_P010},
    {"NV12_P010", "/data/test/media/1280_720_nv12_10bit.yuv", GRAPHIC_PIXEL_FMT_YCBCR_P010},
};

void EnsureDir(const std::string &dir)
{
    mkdir(dir.c_str(), 0755);
}

sptr<SurfaceBuffer> AllocBuffer(int32_t w, int32_t h, int32_t format)
{
    sptr<SurfaceBuffer> buffer = SurfaceBuffer::Create();
    if (buffer == nullptr) {
        return nullptr;
    }

    BufferRequestConfig config = {
        .width = w,
        .height = h,
        .strideAlignment = 0x8,
        .format = format,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };

    if (buffer->Alloc(config) != GSERROR_OK) {
        return nullptr;
    }
    return buffer;
}

bool ReadFileToBuffer(const std::string &path, const sptr<SurfaceBuffer> &buffer)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        return false;
    }

    void *addr = buffer->GetVirAddr();
    int32_t bufSize = static_cast<int32_t>(buffer->GetSize());
    if (addr == nullptr || bufSize <= 0) {
        return false;
    }
    ifs.read(static_cast<char *>(addr), bufSize);
    return ifs.good() || ifs.eof();
}

bool WriteRawFile(const std::string &path, const sptr<SurfaceBuffer> &buffer, int32_t size)
{
    std::ofstream ofs(path, std::ios::binary);
    if (!ofs.is_open()) {
        return false;
    }
    void *addr = buffer->GetVirAddr();
    if (addr == nullptr) {
        return false;
    }
    ofs.write(static_cast<char *>(addr), size);
    return ofs.good();
}

void VerifyFileSize(const std::string &path, int32_t expectedSize)
{
    std::ifstream ifs(path, std::ios::binary | std::ios::ate);
    ASSERT_TRUE(ifs.is_open());
    EXPECT_EQ(static_cast<int32_t>(ifs.tellg()), expectedSize);
}

int32_t FrameSize(int32_t w, int32_t h, int32_t pixelFormat)
{
    if (pixelFormat == GRAPHIC_PIXEL_FMT_YCRCB_420_SP || pixelFormat == GRAPHIC_PIXEL_FMT_YCBCR_420_SP) {
        return w * h * 3 / 2;
    }
    if (pixelFormat == GRAPHIC_PIXEL_FMT_YCBCR_P010 || pixelFormat == GRAPHIC_PIXEL_FMT_YCRCB_P010) {
        return w * h * 3;
    }
    return w * h * 4;
}

bool SaveOutput(const std::string &prefix, int32_t outW, int32_t outH, int32_t pixelFormat, const std::string &suffix,
                const sptr<SurfaceBuffer> &buffer)
{
    int32_t frameSize = FrameSize(outW, outH, pixelFormat);
    std::string outPath =
        std::string(MEDIA_DIR) + prefix + std::to_string(outW) + "x" + std::to_string(outH) + "_" + suffix + ".raw";
    if (!WriteRawFile(outPath, buffer, frameSize)) {
        return false;
    }
    VerifyFileSize(outPath, frameSize);
    return true;
}

struct ConcurrentTask {
    const DsFormatParam &fmt;
    int32_t outW;
    int32_t outH;
    const char *outPrefix;
    const char *outSuffix;
    std::function<int32_t(FastKitsInterface &, const sptr<SurfaceBuffer> &, const sptr<SurfaceBuffer> &)> operation;
};

void RunConcurrentTask(const ConcurrentTask &task, std::atomic<int32_t> &successCount, std::atomic<bool> &crashed)
{
    try {
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        sptr<SurfaceBuffer> input = AllocBuffer(INPUT_WIDTH, INPUT_HEIGHT, task.fmt.pixelFormat);
        if (input == nullptr || !ReadFileToBuffer(task.fmt.inputFile, input)) {
            inst.Release();
            return;
        }
        sptr<SurfaceBuffer> output = AllocBuffer(task.outW, task.outH, task.fmt.pixelFormat);
        if (output == nullptr || input->GetFormat() != output->GetFormat()) {
            inst.Release();
            return;
        }
        SetTestHDRMetadata(input);
        int32_t ret = task.operation(inst, input, output);
        if (ret != AVCS_ERR_OK) {
            inst.Release();
            return;
        }
        if (SaveOutput(task.outPrefix, task.outW, task.outH, task.fmt.pixelFormat, task.outSuffix, output)) {
            successCount.fetch_add(1);
        }
        inst.Release();
    } catch (...) {
        crashed.store(true);
    }
}
} // namespace

// ========== 单例基础测试 ==========

class FastKitsBasicTest : public testing::Test {};

/**
 * @tc.name: FastKitsInterface_GetInstance_SameObject_001
 * @tc.desc: GetInstance返回同一对象
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsBasicTest, GetInstance_SameObject, TestSize.Level1)
{
    EXPECT_NO_FATAL_FAILURE({
        auto &inst1 = FastKitsInterface::GetInstance();
        auto &inst2 = FastKitsInterface::GetInstance();
        EXPECT_EQ(&inst1, &inst2);
    });
}

/**
 * @tc.name: FastKitsInterface_IsLoaded_BeforeInit_001
 * @tc.desc: 未初始化时IsLoaded返回false
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsBasicTest, IsLoaded_BeforeInit, TestSize.Level1)
{
    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        EXPECT_FALSE(inst.IsLoaded());
    });
}

// ========== Init 测试 ==========

class FastKitsInitTest : public testing::TestWithParam<DsFormatParam> {};

/**
 * @tc.name: FastKitsInterface_Init_Format_001
 * @tc.desc: 各像素格式初始化不crash
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsInitTest, Init_Format, TestSize.Level1)
{
    auto param = GetParam();
    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        int32_t ret = inst.Init(param.pixelFormat, FastResizeAlgoType::BICUBIC);
        if (ret != AVCS_ERR_OK) {
            inst.Release();
            GTEST_SKIP() << "libfast_image.so not available, skipping";
        }
        EXPECT_TRUE(inst.IsLoaded());
        inst.Release();
    });
}

INSTANTIATE_TEST_SUITE_P(, FastKitsInitTest, testing::ValuesIn(DS_FORMAT_PARAMS),
                         [](const testing::TestParamInfo<DsFormatParam> &info) { return info.param.name; });

// ========== DownSample 集成测试 ==========

class FastKitsFormatTest : public testing::TestWithParam<DsFormatParam> {
public:
    void SetUp() override
    {
        auto param = GetParam();
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        if (inst.Init(param.pixelFormat, FastResizeAlgoType::BICUBIC) != AVCS_ERR_OK) {
            inst.Release();
            GTEST_SKIP() << "libfast_image.so not available, skipping";
        }

        inputBuffer_ = AllocBuffer(INPUT_WIDTH, INPUT_HEIGHT, param.pixelFormat);
        if (inputBuffer_ == nullptr) {
            inst.Release();
            GTEST_SKIP() << "Format " << param.name << " not supported on device";
        }
        ASSERT_TRUE(ReadFileToBuffer(param.inputFile, inputBuffer_));
        EnsureDir(MEDIA_DIR);
    }

    void TearDown() override
    {
        FastKitsInterface::GetInstance().Release();
    }

protected:
    sptr<SurfaceBuffer> inputBuffer_;
};

using FastKitsDsTest = FastKitsFormatTest;
using FastKitsCropTest = FastKitsFormatTest;

INSTANTIATE_TEST_SUITE_P(FastKits, FastKitsDsTest, testing::ValuesIn(DS_FORMAT_PARAMS));

/**
 * @tc.name: FastKitsInterface_Downsample_640x360_001
 * @tc.desc: 降采样到640x360并验证输出文件
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsDsTest, Downsample_640x360, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = 640;
    constexpr int32_t outH = 360;

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());

    SetTestHDRMetadata(inputBuffer_);
    SetTestROIMetadata(inputBuffer_, "100,200-300,400=5;");

    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret = inst.DownSample(inputBuffer_, output, FastResizeAlgoType::BICUBIC);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    });

    ExpectHDRMetadataEqual(inputBuffer_, output);
    ExpectROIMetadata(output, TransformROI("100,200-300,400=5;", 0.5, 0.5, 0, 0));

    EXPECT_TRUE(SaveOutput("ds_", outW, outH, param.pixelFormat, param.name, output));
}

/**
 * @tc.name: FastKitsInterface_Downsample_OutputNotAllZero_001
 * @tc.desc: 降采样输出不能全零
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsDsTest, Downsample_OutputNotAllZero, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = 320;
    constexpr int32_t outH = 180;
    int32_t outSize = FrameSize(outW, outH, param.pixelFormat);

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());

    SetTestHDRMetadata(inputBuffer_);

    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret = inst.DownSample(inputBuffer_, output, FastResizeAlgoType::BICUBIC);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    });

    ExpectHDRMetadataEqual(inputBuffer_, output);

    uint8_t *data = static_cast<uint8_t *>(output->GetVirAddr());
    ASSERT_NE(data, nullptr);
    bool allZero = true;
    for (int32_t i = 0; i < outSize; i++) {
        if (data[i] != 0) {
            allZero = false;
            break;
        }
    }
    EXPECT_FALSE(allZero);
}

/**
 * @tc.name: FastKitsInterface_Downsample_SameDimensions_001
 * @tc.desc: 降采样到与输入相同宽高，输出应与输入一致
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsDsTest, Downsample_SameDimensions, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = INPUT_WIDTH;
    constexpr int32_t outH = INPUT_HEIGHT;

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());

    SetTestHDRMetadata(inputBuffer_);
    SetTestROIMetadata(inputBuffer_, "100,200-300,400=5;");

    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret = inst.DownSample(inputBuffer_, output, FastResizeAlgoType::BICUBIC);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    });

    ExpectHDRMetadataEqual(inputBuffer_, output);
    ExpectROIMetadata(output, TransformROI("100,200-300,400=5;", 1.0, 1.0, 0, 0));

    EXPECT_TRUE(SaveOutput("ds_same_", outW, outH, param.pixelFormat, param.name, output));
}

// ========== Crop 集成测试 ==========

INSTANTIATE_TEST_SUITE_P(FastKits, FastKitsCropTest, testing::ValuesIn(DS_FORMAT_PARAMS));

/**
 * @tc.name: FastKitsInterface_Crop_100_50_1179_669_001
 * @tc.desc: 裁剪100,50~1179,669并验证输出
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsCropTest, Crop_100_50_1179_669, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t left = 100;
    constexpr int32_t top = 50;
    constexpr int32_t right = 1179;
    constexpr int32_t bottom = 669;
    constexpr int32_t outW = right - left + 1;
    constexpr int32_t outH = bottom - top + 1;

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());

    SetTestHDRMetadata(inputBuffer_);
    SetTestROIMetadata(inputBuffer_, "100,200-300,400=5;");

    CropRect cropRect{top, left, bottom, right};
    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret = inst.Crop(inputBuffer_, output, cropRect);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    });

    ExpectHDRMetadataEqual(inputBuffer_, output);
    ExpectROIMetadata(output, TransformROI("100,200-300,400=5;", 1.0, 1.0, top, left));

    EXPECT_TRUE(SaveOutput("crop_", outW, outH, param.pixelFormat, param.name, output));
}

/**
 * @tc.name: FastKitsInterface_Crop_SameDimensions_001
 * @tc.desc: 裁剪区域与输入相同（left=0,top=0,right=W-1,bottom=H-1），输出宽高与输入一致
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsCropTest, Crop_SameDimensions, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = INPUT_WIDTH;
    constexpr int32_t outH = INPUT_HEIGHT;
    constexpr CropRect fullRect{0, 0, INPUT_HEIGHT - 1, INPUT_WIDTH - 1};

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());

    SetTestHDRMetadata(inputBuffer_);
    SetTestROIMetadata(inputBuffer_, "100,200-300,400=5;");

    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret = inst.Crop(inputBuffer_, output, fullRect);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    });

    ExpectHDRMetadataEqual(inputBuffer_, output);
    ExpectROIMetadata(output, TransformROI("100,200-300,400=5;", 1.0, 1.0, 0, 0));

    EXPECT_TRUE(SaveOutput("crop_full_", outW, outH, param.pixelFormat, param.name, output));
}

// ========== 并发测试 ==========

class FastKitsConcurrentTest : public testing::Test {
public:
    void SetUp() override
    {
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        if (inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888) != AVCS_ERR_OK) {
            inst.Release();
            GTEST_SKIP() << "libfast_image.so not available on device, skipping";
        }
        EnsureDir(MEDIA_DIR);
    }

    void TearDown() override
    {
        FastKitsInterface::GetInstance().Release();
    }
};

/**
 * @tc.name: FastKitsInterface_ConcurrentInit_001
 * @tc.desc: 多线程并发Init不同格式
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsConcurrentTest, ConcurrentInit, TestSize.Level1)
{
    constexpr int32_t threadCount = sizeof(DS_FORMAT_PARAMS) / sizeof(DS_FORMAT_PARAMS[0]);
    std::atomic<int32_t> successCount{0};
    std::atomic<bool> crashed{false};
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < threadCount; i++) {
        threads.emplace_back([&successCount, &crashed, format = DS_FORMAT_PARAMS[i].pixelFormat]() {
            try {
                auto &inst = FastKitsInterface::GetInstance();
                inst.Retain();
                int32_t ret = inst.Init(format, FastResizeAlgoType::BICUBIC);
                if (ret == AVCS_ERR_OK) {
                    successCount.fetch_add(1);
                }
                inst.Release();
            } catch (...) {
                crashed.store(true);
            }
        });
    }

    for (auto &t : threads) {
        t.join();
    }

    EXPECT_FALSE(crashed.load());
    EXPECT_EQ(successCount.load(), threadCount);
}

/**
 * @tc.name: FastKitsInterface_ConcurrentDownsample_001
 * @tc.desc: 多线程并发DownSample不同格式+相同格式同时并发
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsConcurrentTest, ConcurrentDownsample, TestSize.Level1)
{
    auto dsOp = [](FastKitsInterface &inst, const sptr<SurfaceBuffer> &in, const sptr<SurfaceBuffer> &out) -> int32_t {
        return inst.DownSample(in, out, FastResizeAlgoType::BICUBIC);
    };

    const ConcurrentTask tasks[] = {
        {DS_FORMAT_PARAMS[0], 640, 360, "ds_", "rgba", dsOp},
        {DS_FORMAT_PARAMS[1], 320, 180, "ds_", "1010102", dsOp},
        {DS_FORMAT_PARAMS[2], 640, 360, "ds_", "nv21", dsOp},
        {DS_FORMAT_PARAMS[3], 640, 360, "ds_", "nv12", dsOp},
        {DS_FORMAT_PARAMS[4], 640, 360, "ds_", "nv21_p010", dsOp},
        {DS_FORMAT_PARAMS[5], 640, 360, "ds_", "nv12_p010", dsOp},
        {DS_FORMAT_PARAMS[2], 320, 180, "ds_", "nv21_dup", dsOp},
        {DS_FORMAT_PARAMS[5], 320, 180, "ds_", "nv12_p010_dup", dsOp},
    };
    constexpr int32_t threadCount = sizeof(tasks) / sizeof(tasks[0]);
    std::atomic<int32_t> successCount{0};
    std::atomic<bool> crashed{false};
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < threadCount; i++) {
        threads.emplace_back(
            [&successCount, &crashed, &tasks, i]() { RunConcurrentTask(tasks[i], successCount, crashed); });
    }

    for (auto &t : threads) {
        t.join();
    }

    EXPECT_FALSE(crashed.load());
    EXPECT_EQ(successCount.load(), threadCount);
}

/**
 * @tc.name: FastKitsInterface_ConcurrentCrop_001
 * @tc.desc: 多线程并发Crop不同区域
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsConcurrentTest, ConcurrentCrop, TestSize.Level1)
{
    auto cropOp = [](FastKitsInterface &inst, const sptr<SurfaceBuffer> &in,
                     const sptr<SurfaceBuffer> &out) -> int32_t {
        return inst.Crop(in, out, CropRect{50, 100, 669, 1179});
    };

    const ConcurrentTask tasks[] = {
        {DS_FORMAT_PARAMS[0], 1080, 620, "crop_", "rgba", cropOp},
        {DS_FORMAT_PARAMS[1], 1080, 620, "crop_", "1010102", cropOp},
        {DS_FORMAT_PARAMS[2], 1080, 620, "crop_", "nv21", cropOp},
        {DS_FORMAT_PARAMS[3], 1080, 620, "crop_", "nv12", cropOp},
        {DS_FORMAT_PARAMS[4], 1080, 620, "crop_", "nv21_p010", cropOp},
        {DS_FORMAT_PARAMS[5], 1080, 620, "crop_", "nv12_p010", cropOp},
    };
    constexpr int32_t threadCount = sizeof(tasks) / sizeof(tasks[0]);
    std::atomic<int32_t> successCount{0};
    std::atomic<bool> crashed{false};
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < threadCount; i++) {
        threads.emplace_back(
            [&successCount, &crashed, &tasks, i]() { RunConcurrentTask(tasks[i], successCount, crashed); });
    }

    for (auto &t : threads) {
        t.join();
    }

    EXPECT_FALSE(crashed.load());
    EXPECT_EQ(successCount.load(), threadCount);
}

// ========== Preprocessor 生命周期测试 ==========

class PreprocessorLifecycleTest : public testing::Test {
public:
    static bool IsSoAvailable()
    {
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        bool available = inst.IsLoaded();
        inst.Release();
        return available;
    }
};

/**
 * @tc.name: PreprocessorLifecycle_ConstructDestroy_UnloadsLibrary_001
 * @tc.desc: Preprocessor析构后SO被卸载
 * @tc.type: FUNC
 */
HWTEST_F(PreprocessorLifecycleTest, ConstructDestroy_UnloadsLibrary, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    {
        auto proc = std::make_unique<MediaAVCodec::Preprocessor>("video/avc");
    }
    EXPECT_FALSE(inst.IsLoaded()) << "Library should be unloaded after Preprocessor destroyed";
}

/**
 * @tc.name: PreprocessorLifecycle_ReopenAfterDestroy_Works_001
 * @tc.desc: Preprocessor销毁后重新创建，SO能正常加载
 * @tc.type: FUNC
 */
HWTEST_F(PreprocessorLifecycleTest, ReopenAfterDestroy_Works, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    {
        auto proc = std::make_unique<MediaAVCodec::Preprocessor>("video/avc");
    }
    EXPECT_FALSE(inst.IsLoaded());

    {
        auto proc = std::make_unique<MediaAVCodec::Preprocessor>("video/avc");
        EXPECT_TRUE(inst.IsLoaded()) << "Library should load again after re-creating Preprocessor";
    }
    EXPECT_FALSE(inst.IsLoaded());
}

/**
 * @tc.name: PreprocessorLifecycle_MultipleCycles_NoCrash_001
 * @tc.desc: 多次创建/销毁Preprocessor循环，无崩溃无泄漏
 * @tc.type: FUNC
 */
HWTEST_F(PreprocessorLifecycleTest, MultipleCycles_NoCrash, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    for (int i = 0; i < 5; i++) {
        auto proc = std::make_unique<MediaAVCodec::Preprocessor>("video/avc");
        EXPECT_TRUE(inst.IsLoaded()) << "Cycle " << i << ": not loaded";
        proc.reset();
        EXPECT_FALSE(inst.IsLoaded()) << "Cycle " << i << ": still loaded after destroy";
    }
}

/**
 * @tc.name: PreprocessorLifecycle_MultiInstance_OnlyLastDestroyUnloads_001
 * @tc.desc: 多个Preprocessor实例共存时SO保持加载，全部析构后卸载
 * @tc.type: FUNC
 */
HWTEST_F(PreprocessorLifecycleTest, MultiInstance_OnlyLastDestroyUnloads, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    auto proc1 = std::make_unique<MediaAVCodec::Preprocessor>("video/avc");
    EXPECT_TRUE(inst.IsLoaded());

    {
        auto proc2 = std::make_unique<MediaAVCodec::Preprocessor>("video/avc");
        EXPECT_TRUE(inst.IsLoaded());
        proc2.reset();
        EXPECT_TRUE(inst.IsLoaded()) << "Library should still be loaded with one Preprocessor alive";
    }

    proc1.reset();
    EXPECT_FALSE(inst.IsLoaded()) << "Library should be unloaded after all Preprocessors destroyed";
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
