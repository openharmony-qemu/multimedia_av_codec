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
#include <tuple>
#include <vector>

#include <gtest/gtest.h>
#include "fast_kits_interface.h"
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
constexpr int32_t INPUT_WIDTH = 1080;
constexpr int32_t INPUT_HEIGHT = 720;
constexpr int32_t SRC_FILE_WIDTH = 1280;

// Metadata key namespaces
namespace V1_0 = OHOS::HDI::Display::Graphic::Common::V1_0;
namespace V2_2 = OHOS::HDI::Display::Graphic::Common::V2_2;

// Test HDR metadata
const std::vector<uint8_t> TEST_HDR_META_TYPE = {1, 0, 0, 0};
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
    (void)output;
    (void)expected;
    // std::vector<uint8_t> outVal;
    // ASSERT_EQ(output->GetMetadata(V2_2::ATTRKEY_ROI_METADATA, outVal), GSERROR_OK) << "Get ROI metadata failed";
    // ASSERT_GE(outVal.size(), expected.size())
    //     << "ROI metadata too short, expected " << expected.size() << " bytes, got " << outVal.size();
    // std::string actual(outVal.begin(), outVal.begin() + expected.size());
    // EXPECT_EQ(actual, expected) << "ROI mismatch, expected: " << expected << ", actual: " << actual;
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
        int32_t qp = match[5].matched ? std::stoi(match[5].str()) : 0;
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

std::vector<std::tuple<int32_t, int32_t>> FormatMismatchPairs()
{
    std::vector<std::tuple<int32_t, int32_t>> pairs;
    constexpr int32_t n = sizeof(DS_FORMAT_PARAMS) / sizeof(DS_FORMAT_PARAMS[0]);
    for (int32_t i = 0; i < n; ++i) {
        for (int32_t j = i + 1; j < n; ++j) {
            pairs.emplace_back(i, j);
        }
    }
    return pairs;
}

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
        .strideAlignment = 0x40,
        .format = format,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };

    if (buffer->Alloc(config) != GSERROR_OK) {
        return nullptr;
    }
    return buffer;
}

struct PlaneRowInfo {
    int32_t fileRowBytes{0};
    int32_t rowCount{0};
};

bool GetPlaneRowInfo(int32_t format, int32_t width, int32_t height, std::vector<PlaneRowInfo> &planes)
{
    switch (format) {
        case GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
            planes = {{width * 4, height}};
            return true;
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
            planes = {{width, height}, {width, (height + 1) / 2}};
            return true;
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
            planes = {{width * 2, height}, {width * 2, (height + 1) / 2}};
            return true;
        default:
            return false;
    }
}

bool ReadFileToBuffer(const std::string &path, const sptr<SurfaceBuffer> &buffer, int32_t srcWidth = 0)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        printf("[ReadFile] FAIL open %s\n", path.c_str());
        return false;
    }
    ifs.seekg(0, std::ios::end);
    int32_t fileSize = static_cast<int32_t>(ifs.tellg());
    ifs.seekg(0, std::ios::beg);
    auto *addr = static_cast<uint8_t *>(buffer->GetVirAddr());
    if (addr == nullptr) {
        printf("[ReadFile] FAIL virAddr is null\n");
        return false;
    }
    int32_t dstWidth = buffer->GetWidth();
    int32_t height = buffer->GetHeight();
    int32_t stride = buffer->GetStride();
    int32_t format = buffer->GetFormat();
    int32_t bufSize = static_cast<int32_t>(buffer->GetSize());
    if (srcWidth <= 0) {
        srcWidth = dstWidth;
    }
    printf("[ReadFile] %s -> %dx%d fmt=%d stride=%d bufSize=%d fileSize=%d srcWidth=%d\n", path.c_str(), dstWidth,
           height, format, stride, bufSize, fileSize, srcWidth);
    std::vector<PlaneRowInfo> dstPlanes;
    std::vector<PlaneRowInfo> srcPlanes;
    if (!GetPlaneRowInfo(format, dstWidth, height, dstPlanes) ||
        !GetPlaneRowInfo(format, srcWidth, height, srcPlanes)) {
        ifs.read(reinterpret_cast<char *>(addr), bufSize);
        printf("[ReadFile] unknown format, bulk read %d bytes\n", bufSize);
        return ifs.good() || ifs.eof();
    }
    int32_t planeOffset = 0;
    for (size_t i = 0; i < dstPlanes.size(); ++i) {
        int32_t dstRowBytes = dstPlanes[i].fileRowBytes;
        int32_t srcRowBytes = srcPlanes[i].fileRowBytes;
        int32_t rowCount = dstPlanes[i].rowCount;
        int32_t skipBytes = srcRowBytes - dstRowBytes;
        printf("[ReadFile] plane%zu: dstRowBytes=%d srcRowBytes=%d rows=%d skip=%d planeOffset=%d\n", i, dstRowBytes,
               srcRowBytes, rowCount, skipBytes, planeOffset);
        for (int32_t row = 0; row < rowCount; ++row) {
            ifs.read(reinterpret_cast<char *>(addr + planeOffset + row * stride), dstRowBytes);
            if (skipBytes > 0) {
                ifs.seekg(skipBytes, std::ios::cur);
            }
            if (!ifs.good() && !ifs.eof()) {
                printf("[ReadFile] FAIL at plane%zu row %d\n", i, row);
                return false;
            }
        }
        planeOffset += rowCount * stride;
    }
    printf("[ReadFile] OK, total read %lld bytes\n", static_cast<long long>(ifs.tellg()));
    return true;
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

int32_t CalcDataSize(const sptr<SurfaceBuffer> &buffer)
{
    int32_t width = buffer->GetWidth();
    int32_t height = buffer->GetHeight();
    int32_t stride = buffer->GetStride();
    int32_t format = buffer->GetFormat();
    std::vector<PlaneRowInfo> planes;
    if (!GetPlaneRowInfo(format, width, height, planes)) {
        return static_cast<int32_t>(buffer->GetSize());
    }
    int32_t size = 0;
    for (const auto &plane : planes) {
        size += plane.rowCount * stride;
    }
    return size;
}

bool SaveOutput(const std::string &prefix, const sptr<SurfaceBuffer> &buffer, const std::string &suffix)
{
    int32_t dataSize = CalcDataSize(buffer);
    int32_t bufSize = static_cast<int32_t>(buffer->GetSize());
    std::string outPath = std::string(MEDIA_DIR) + prefix + std::to_string(buffer->GetWidth()) + "x" +
                          std::to_string(buffer->GetHeight()) + "_stride" + std::to_string(buffer->GetStride()) + "_" +
                          suffix + ".raw";
    printf("[SaveFile] %s %dx%d fmt=%d stride=%d dataSize=%d bufSize=%d\n", outPath.c_str(), buffer->GetWidth(),
           buffer->GetHeight(), buffer->GetFormat(), buffer->GetStride(), dataSize, bufSize);
    if (!WriteRawFile(outPath, buffer, dataSize)) {
        printf("[SaveFile] FAIL write\n");
        return false;
    }
    VerifyFileSize(outPath, dataSize);
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

void RunConcurrentTask(const ConcurrentTask &task, std::atomic<int32_t> &successCount)
{
    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        sptr<SurfaceBuffer> input = AllocBuffer(INPUT_WIDTH, INPUT_HEIGHT, task.fmt.pixelFormat);
        if (input == nullptr || !ReadFileToBuffer(task.fmt.inputFile, input, SRC_FILE_WIDTH)) {
            inst.Release();
            return;
        }
        SaveOutput("in_", input, task.fmt.name);
        sptr<SurfaceBuffer> output = AllocBuffer(task.outW, task.outH, task.fmt.pixelFormat);
        if (output == nullptr || input->GetFormat() != output->GetFormat()) {
            inst.Release();
            return;
        }
        SetTestHDRMetadata(input);
        int32_t ret = task.operation(inst, input, output);
        EXPECT_EQ(ret, AVCS_ERR_OK);
        if (ret != AVCS_ERR_OK) {
            inst.Release();
            return;
        }
        if (SaveOutput(task.outPrefix, output, task.outSuffix)) {
            successCount.fetch_add(1);
        }
        inst.Release();
    });
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
        EnsureDir(MEDIA_DIR);
        ASSERT_TRUE(ReadFileToBuffer(param.inputFile, inputBuffer_, SRC_FILE_WIDTH));
        SaveOutput("in_", inputBuffer_, param.name);
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
 * @tc.name: FastKitsInterface_Downsample_636x360_001
 * @tc.desc: 降采样到636x360并验证输出文件
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsDsTest, Downsample_636x360, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = 636;
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
    ExpectROIMetadata(output, TransformROI("100,200-300,400=5;", 636.0 / INPUT_WIDTH, 360.0 / INPUT_HEIGHT, 0, 0));

    EXPECT_TRUE(SaveOutput("ds_", output, param.name));
}

/**
 * @tc.name: FastKitsInterface_Downsample_OutputNotAllZero_001
 * @tc.desc: 降采样输出不能全零
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsDsTest, Downsample_OutputNotAllZero, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = 316;
    constexpr int32_t outH = 180;

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());
    int32_t outSize = CalcDataSize(output);

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

    EXPECT_TRUE(SaveOutput("ds_same_", output, param.name));
}

// ========== Crop 集成测试 ==========

INSTANTIATE_TEST_SUITE_P(FastKits, FastKitsCropTest, testing::ValuesIn(DS_FORMAT_PARAMS));

/**
 * @tc.name: FastKitsInterface_Crop_80_40_979_659_001
 * @tc.desc: 裁剪80,40~979,659并验证输出
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsCropTest, Crop_80_40_979_659, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t left = 80;
    constexpr int32_t top = 40;
    constexpr int32_t right = 979;
    constexpr int32_t bottom = 659;
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

    EXPECT_TRUE(SaveOutput("crop_", output, param.name));
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

    EXPECT_TRUE(SaveOutput("crop_full_", output, param.name));
}

// ========== 格式不匹配测试 ==========

class FastKitsFormatMismatchTest : public testing::TestWithParam<std::tuple<int32_t, int32_t>> {
public:
    void SetUp() override
    {
        auto [idx, _] = GetParam();
        auto &inputFmt = DS_FORMAT_PARAMS[idx];
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        for (const auto &fmt : DS_FORMAT_PARAMS) {
            (void)inst.Init(fmt.pixelFormat, FastResizeAlgoType::BICUBIC);
        }
        if (!inst.IsLoaded()) {
            inst.Release();
            GTEST_SKIP() << "libfast_image.so not available, skipping";
        }
        inputBuffer_ = AllocBuffer(INPUT_WIDTH, INPUT_HEIGHT, inputFmt.pixelFormat);
        if (inputBuffer_ == nullptr) {
            inst.Release();
            GTEST_SKIP() << "Format not supported on device";
        }
        EnsureDir(MEDIA_DIR);
        (void)ReadFileToBuffer(inputFmt.inputFile, inputBuffer_, SRC_FILE_WIDTH);
    }

    void TearDown() override
    {
        FastKitsInterface::GetInstance().Release();
    }

protected:
    sptr<SurfaceBuffer> inputBuffer_;
};

INSTANTIATE_TEST_SUITE_P(
    FastKits, FastKitsFormatMismatchTest, testing::ValuesIn(FormatMismatchPairs()),
    [](const testing::TestParamInfo<std::tuple<int32_t, int32_t>> &info) {
        return std::string(DS_FORMAT_PARAMS[std::get<0>(info.param)].name) + "_to_" +
               DS_FORMAT_PARAMS[std::get<1>(info.param)].name;
    });

/**
 * @tc.name: FastKitsInterface_FormatMismatch_ReturnError_001
 * @tc.desc: 输入输出像素格式不一致时Crop和DownSample均应报错
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsFormatMismatchTest, FormatMismatch_ReturnError, TestSize.Level1)
{
    auto [inputIdx, outputIdx] = GetParam();
    auto &outputFmt = DS_FORMAT_PARAMS[outputIdx];
    auto &inst = FastKitsInterface::GetInstance();
    constexpr int32_t outW = 636;
    constexpr int32_t outH = 360;

    // DownSample with mismatched format
    {
        sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, outputFmt.pixelFormat);
        ASSERT_NE(output, nullptr);
        EXPECT_NE(inputBuffer_->GetFormat(), output->GetFormat());
        EXPECT_NO_FATAL_FAILURE({
            int32_t ret = inst.DownSample(inputBuffer_, output, FastResizeAlgoType::BICUBIC);
            EXPECT_NE(ret, AVCS_ERR_OK) << "DownSample should fail with format mismatch";
        });
    }

    // Crop with mismatched format
    {
        sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, outputFmt.pixelFormat);
        ASSERT_NE(output, nullptr);
        EXPECT_NE(inputBuffer_->GetFormat(), output->GetFormat());
        CropRect rect{0, 0, outH - 1, outW - 1};
        EXPECT_NO_FATAL_FAILURE({
            int32_t ret = inst.Crop(inputBuffer_, output, rect);
            EXPECT_NE(ret, AVCS_ERR_OK) << "Crop should fail with format mismatch";
        });
    }
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
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < threadCount; i++) {
        threads.emplace_back([&successCount, format = DS_FORMAT_PARAMS[i].pixelFormat]() {
            EXPECT_NO_FATAL_FAILURE({
                auto &inst = FastKitsInterface::GetInstance();
                inst.Retain();
                int32_t ret = inst.Init(format, FastResizeAlgoType::BICUBIC);
                if (ret == AVCS_ERR_OK) {
                    successCount.fetch_add(1);
                }
                inst.Release();
            });
        });
    }

    for (auto &t : threads) {
        t.join();
    }

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
        int32_t ret = inst.Init(in->GetFormat(), FastResizeAlgoType::BICUBIC);
        if (ret != 0) {
            return ret;
        }
        return inst.DownSample(in, out, FastResizeAlgoType::BICUBIC);
    };

    const ConcurrentTask tasks[] = {
        {DS_FORMAT_PARAMS[0], 636, 360, "ds_", "rgba", dsOp},
        {DS_FORMAT_PARAMS[1], 316, 180, "ds_", "1010102", dsOp},
        {DS_FORMAT_PARAMS[2], 636, 360, "ds_", "nv21", dsOp},
        {DS_FORMAT_PARAMS[3], 636, 360, "ds_", "nv12", dsOp},
        {DS_FORMAT_PARAMS[4], 636, 360, "ds_", "nv21_p010", dsOp},
        {DS_FORMAT_PARAMS[5], 636, 360, "ds_", "nv12_p010", dsOp},
        {DS_FORMAT_PARAMS[2], 316, 180, "ds_", "nv21_dup", dsOp},
        {DS_FORMAT_PARAMS[5], 316, 180, "ds_", "nv12_p010_dup", dsOp},
    };
    constexpr int32_t threadCount = sizeof(tasks) / sizeof(tasks[0]);
    std::atomic<int32_t> successCount{0};
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < threadCount; i++) {
        threads.emplace_back([&successCount, &tasks, i]() { RunConcurrentTask(tasks[i], successCount); });
    }

    for (auto &t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount.load(), threadCount);
}

/**
 * @tc.name: FastKitsInterface_ConcurrentCrop_001
 * @tc.desc: 多线程并发Crop不同格式+相同格式同时并发
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsConcurrentTest, ConcurrentCrop, TestSize.Level1)
{
    auto cropOp = [](FastKitsInterface &inst, const sptr<SurfaceBuffer> &in,
                     const sptr<SurfaceBuffer> &out) -> int32_t {
        return inst.Crop(in, out, CropRect{40, 80, 659, 979});
    };

    const ConcurrentTask tasks[] = {
        {DS_FORMAT_PARAMS[0], 900, 620, "crop_", "rgba", cropOp},
        {DS_FORMAT_PARAMS[1], 900, 620, "crop_", "1010102", cropOp},
        {DS_FORMAT_PARAMS[2], 900, 620, "crop_", "nv21", cropOp},
        {DS_FORMAT_PARAMS[3], 900, 620, "crop_", "nv12", cropOp},
        {DS_FORMAT_PARAMS[4], 900, 620, "crop_", "nv21_p010", cropOp},
        {DS_FORMAT_PARAMS[5], 900, 620, "crop_", "nv12_p010", cropOp},
        {DS_FORMAT_PARAMS[2], 900, 620, "crop_", "nv21_dup", cropOp},
        {DS_FORMAT_PARAMS[5], 900, 620, "crop_", "nv12_p010_dup", cropOp},
    };
    constexpr int32_t threadCount = sizeof(tasks) / sizeof(tasks[0]);
    std::atomic<int32_t> successCount{0};
    std::vector<std::thread> threads;

    for (int32_t i = 0; i < threadCount; i++) {
        threads.emplace_back([&successCount, &tasks, i]() { RunConcurrentTask(tasks[i], successCount); });
    }

    for (auto &t : threads) {
        t.join();
    }

    EXPECT_EQ(successCount.load(), threadCount);
}

// ========== FastKitsInterface 生命周期测试 ==========

class FastKitsLifecycleTest : public testing::Test {
public:
    static bool IsSoAvailable()
    {
        void *handle = dlopen("libfast_image.so", RTLD_NOW);
        if (handle == nullptr) {
            return false;
        }
        dlclose(handle);
        return true;
    }
};

/**
 * @tc.name: FastKitsLifecycle_RetainInitRelease_UnloadsLibrary_001
 * @tc.desc: Retain+Init加载SO，Release后SO被卸载
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsLifecycleTest, RetainInitRelease_UnloadsLibrary, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    inst.Retain();
    EXPECT_EQ(inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888), AVCS_ERR_OK);
    EXPECT_TRUE(inst.IsLoaded());
    inst.Release();
    EXPECT_FALSE(inst.IsLoaded()) << "Library should be unloaded after Release";
}

/**
 * @tc.name: FastKitsLifecycle_ReopenAfterRelease_Works_001
 * @tc.desc: Release卸载后再次Retain+Init，SO能正常加载
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsLifecycleTest, ReopenAfterRelease_Works, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    inst.Retain();
    EXPECT_EQ(inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888), AVCS_ERR_OK);
    inst.Release();
    EXPECT_FALSE(inst.IsLoaded());

    inst.Retain();
    EXPECT_EQ(inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888), AVCS_ERR_OK);
    EXPECT_TRUE(inst.IsLoaded()) << "Library should load again after Retain+Init";
    inst.Release();
    EXPECT_FALSE(inst.IsLoaded());
}

/**
 * @tc.name: FastKitsLifecycle_MultipleCycles_NoCrash_001
 * @tc.desc: 多次Retain+Init/Release循环，无崩溃无泄漏
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsLifecycleTest, MultipleCycles_NoCrash, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    for (int i = 0; i < 5; i++) {
        inst.Retain();
        EXPECT_EQ(inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888), AVCS_ERR_OK) << "Cycle " << i << ": Init failed";
        EXPECT_TRUE(inst.IsLoaded()) << "Cycle " << i << ": not loaded";
        inst.Release();
        EXPECT_FALSE(inst.IsLoaded()) << "Cycle " << i << ": still loaded after Release";
    }
}

/**
 * @tc.name: FastKitsLifecycle_MultiRetain_OnlyLastReleaseUnloads_001
 * @tc.desc: 多次Retain共享SO，只有最后一次Release才卸载
 * @tc.type: FUNC
 */
HWTEST_F(FastKitsLifecycleTest, MultiRetain_OnlyLastReleaseUnloads, TestSize.Level1)
{
    if (!IsSoAvailable()) {
        GTEST_SKIP() << "libfast_image.so not available, skipping";
    }
    auto &inst = FastKitsInterface::GetInstance();

    inst.Retain();
    EXPECT_EQ(inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888), AVCS_ERR_OK);
    EXPECT_TRUE(inst.IsLoaded());

    inst.Retain();
    EXPECT_TRUE(inst.IsLoaded());
    inst.Release();
    EXPECT_TRUE(inst.IsLoaded()) << "Library should still be loaded with one Retain alive";

    inst.Release();
    EXPECT_FALSE(inst.IsLoaded()) << "Library should be unloaded after all Release";
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
