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
#include <dlfcn.h>
#include <fstream>
#include <regex>
#include <sys/stat.h>
#include <thread>
#include <tuple>
#include <vector>

#include <gtest/gtest.h>
#include "avcodec_errors.h"
#include "fast_kits_interface.h"
#include "surface.h"
#include "surface_buffer_impl.h"
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
    data.push_back('\0');
    (void)buf->SetMetadata(V2_2::ATTRKEY_ROI_METADATA, data, false);
}

void ExpectROIMetadata(const sptr<SurfaceBuffer> &output, const std::string &expected, const std::string &context)
{
    std::vector<uint8_t> outVal;
    ASSERT_EQ(output->GetMetadata(V2_2::ATTRKEY_ROI_METADATA, outVal), GSERROR_OK) << "Get ROI metadata failed";
    auto nullPos = std::find(outVal.begin(), outVal.end(), '\0');
    std::string actual(outVal.begin(), nullPos);
    EXPECT_EQ(actual, expected) << context << "\n  expected: " << expected << "\n  actual:   " << actual;
}

std::string TransformROI(const std::string &roiStr, double scaleX, double scaleY, int32_t offTop, int32_t offLeft)
{
    std::regex pattern(R"((?:^|;)(-?\d+),(-?\d+)-(-?\d+),(-?\d+)(?:=([^;]*))?(?=;|$))");
    std::smatch match;
    std::string temp = roiStr;
    std::string result;
    while (std::regex_search(temp, match, pattern)) {
        int32_t top = std::stoi(match[1].str());
        int32_t left = std::stoi(match[2].str());
        int32_t bottom = std::stoi(match[3].str());
        int32_t right = std::stoi(match[4].str());
        int32_t newTop = static_cast<int32_t>(std::round(top * scaleY)) - offTop;
        int32_t newLeft = static_cast<int32_t>(std::round(left * scaleX)) - offLeft;
        int32_t newBottom = static_cast<int32_t>(std::round(bottom * scaleY)) - offTop;
        int32_t newRight = static_cast<int32_t>(std::round(right * scaleX)) - offLeft;
        if (!result.empty()) {
            result += ";";
        }
        result += std::to_string(newTop) + "," + std::to_string(newLeft) + "-" + std::to_string(newBottom) + "," +
                  std::to_string(newRight);
        if (match[5].matched) {
            result += "=" + match[5].str();
        }
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

// RGBA formats — YUV DS/Crop is covered by FastKitsSliceComboTest
const DsFormatParam RGBA_FORMAT_PARAMS[] = {
    {"RGBA8888", "/data/test/media/1280_720_0.rgba", GRAPHIC_PIXEL_FMT_RGBA_8888},
    {"RGBA1010102", "/data/test/media/1280_720_rgba1010102.rgb", GRAPHIC_PIXEL_FMT_RGBA_1010102},
};

void EnsureDir(const std::string &dir)
{
    mkdir(dir.c_str(), 0755); // 0755: rwxr-xr-x directory permission
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
        .strideAlignment = 0x40, // 0x40: 64-byte stride alignment
        .format = format,
        .usage = BUFFER_USAGE_CPU_READ | BUFFER_USAGE_CPU_WRITE | BUFFER_USAGE_MEM_DMA,
        .timeout = 0,
    };

    if (buffer->Alloc(config) != GSERROR_OK) {
        return nullptr;
    }
    return buffer;
}

// 分配带 slice height 的 buffer：
// 1. 按 64 对齐分配（如 720 → 768）
// 2. 将 BufferHandle.height 改回 contentHeight（模拟解码器输出的 slice > height）
// 3. 手动设置 planesInfo_，使 GetPlanesInfo 返回正确的 slice 布局
sptr<SurfaceBuffer> AllocSliceBuffer(int32_t w, int32_t contentHeight, int32_t format)
{
    int32_t alignedH = ((contentHeight + 63) / 64) * 64; // 64: height alignment, 63 = alignment - 1
    sptr<SurfaceBuffer> buffer = AllocBuffer(w, alignedH, format);
    if (buffer == nullptr) {
        return nullptr;
    }
    buffer->GetBufferHandle()->height = contentHeight;

    auto *impl = static_cast<SurfaceBufferImpl *>(buffer.GetRefPtr());
    int32_t stride = buffer->GetStride();
    bool isYUV = (format == GRAPHIC_PIXEL_FMT_YCBCR_420_SP || format == GRAPHIC_PIXEL_FMT_YCRCB_420_SP ||
                  format == GRAPHIC_PIXEL_FMT_YCBCR_P010 || format == GRAPHIC_PIXEL_FMT_YCRCB_P010);
    if (!isYUV) {
        return buffer;
    }
    constexpr int32_t yuvPlaneCount = 2; // Y plane + UV interleaved plane
    impl->planesInfo_.planeCount = yuvPlaneCount;
    impl->planesInfo_.planes[0].offset = 0;
    impl->planesInfo_.planes[0].rowStride = stride;
    impl->planesInfo_.planes[0].columnStride = 1;
    impl->planesInfo_.planes[1].offset = static_cast<uint64_t>(stride) * alignedH; // UV offset = stride * sliceHeight
    impl->planesInfo_.planes[1].rowStride = stride;
    impl->planesInfo_.planes[1].columnStride = 1;
    return buffer;
}

struct PlaneRowInfo {
    int32_t fileRowBytes{0};
    int32_t rowCount{0};
};

struct PlaneReadParams {
    int32_t planeOffset{0};
    int32_t dstRowBytes{0};
    int32_t srcRowBytes{0};
    int32_t rowCount{0};
};

bool GetPlaneRowInfo(int32_t format, int32_t width, int32_t height, std::vector<PlaneRowInfo> &planes)
{
    switch (format) {
        case GRAPHIC_PIXEL_FMT_RGBA_8888:
        case GRAPHIC_PIXEL_FMT_RGBA_1010102:
            planes = {{width * 4, height}}; // 4: bytes per pixel for RGBA formats
            return true;
        case GRAPHIC_PIXEL_FMT_YCBCR_420_SP:
        case GRAPHIC_PIXEL_FMT_YCRCB_420_SP:
            planes = {{width, height}, {width, (height + 1) / 2}}; // 2: UV plane height is half of Y plane height
            return true;
        case GRAPHIC_PIXEL_FMT_YCBCR_P010:
        case GRAPHIC_PIXEL_FMT_YCRCB_P010:
            planes = {{width * 2, height},
                      {width * 2, (height + 1) / 2}}; // 2: 10-bit samples stored in 16-bit containers
            return true;
        default:
            return false;
    }
}

bool ReadPlanesFromFile(std::ifstream &ifs, uint8_t *addr, int32_t stride, const std::vector<PlaneRowInfo> &dstPlanes,
                        const std::vector<PlaneRowInfo> &srcPlanes)
{
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
            if (ifs.gcount() != dstRowBytes) {
                printf("[ReadFile] FAIL at plane%zu row %d/%d: read %lld/%d bytes\n", i, row, rowCount,
                       static_cast<long long>(ifs.gcount()), dstRowBytes);
                return false;
            }
            if (skipBytes > 0) {
                ifs.seekg(skipBytes, std::ios::cur);
            }
        }
        planeOffset += rowCount * stride;
    }
    return true;
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
    if (!ReadPlanesFromFile(ifs, addr, stride, dstPlanes, srcPlanes)) {
        return false;
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

int32_t GetSliceHeight(const sptr<SurfaceBuffer> &buffer)
{
    int32_t height = buffer->GetHeight();
    int32_t stride = buffer->GetStride();
    OH_NativeBuffer_Planes *planes = nullptr;
    GSError err = buffer->GetPlanesInfo(reinterpret_cast<void **>(&planes));
    if (stride > 0 && err == GSERROR_OK && planes != nullptr && planes->planeCount > 1) {
        return static_cast<int32_t>(static_cast<int64_t>(planes->planes[1].offset) / stride);
    }
    return height;
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
    auto *addr = static_cast<uint8_t *>(buffer->GetVirAddr());
    if (addr == nullptr) {
        return false;
    }
    int32_t w = buffer->GetWidth();
    int32_t h = buffer->GetHeight();
    int32_t stride = buffer->GetStride();
    int32_t sliceH = GetSliceHeight(buffer);
    int32_t dataSize = static_cast<int32_t>(buffer->GetSize());
    std::string outPath = std::string(MEDIA_DIR) + prefix + std::to_string(w) + "x" + std::to_string(h) + "(" +
                          std::to_string(stride) + "x" + std::to_string(sliceH) + ")_" + suffix + ".raw";
    printf("[Save] %s fmt=%d dataSize=%d\n", outPath.c_str(), buffer->GetFormat(), dataSize);
    if (!WriteRawFile(outPath, buffer, dataSize)) {
        printf("[Save] FAIL write\n");
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

// ========== RGBA 格式 DownSample / Crop 测试 ==========

class FastKitsRgbaTest : public testing::TestWithParam<DsFormatParam> {
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

using FastKitsRgbaDsTest = FastKitsRgbaTest;
using FastKitsRgbaCropTest = FastKitsRgbaTest;

INSTANTIATE_TEST_SUITE_P(FastKits, FastKitsRgbaDsTest, testing::ValuesIn(RGBA_FORMAT_PARAMS));

/**
 * @tc.name: FastKitsInterface_Downsample_636x360_001
 * @tc.desc: 降采样到636x360并验证输出文件
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsRgbaDsTest, Downsample_636x360, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = 636; // 636: downsample target width
    constexpr int32_t outH = 360; // 360: downsample target height

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

    EXPECT_TRUE(SaveOutput("ds_", output, param.name));
}

/**
 * @tc.name: FastKitsInterface_Downsample_OutputNotAllZero_001
 * @tc.desc: 降采样输出不能全零
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsRgbaDsTest, Downsample_OutputNotAllZero, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = 316; // 316: downsample target width
    constexpr int32_t outH = 180; // 180: downsample target height

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
HWTEST_P(FastKitsRgbaDsTest, Downsample_SameDimensions, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = INPUT_WIDTH;
    constexpr int32_t outH = INPUT_HEIGHT;

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

    EXPECT_TRUE(SaveOutput("ds_same_", output, param.name));
}

// ========== RGBA Crop 测试 ==========

INSTANTIATE_TEST_SUITE_P(FastKits, FastKitsRgbaCropTest, testing::ValuesIn(RGBA_FORMAT_PARAMS));

/**
 * @tc.name: FastKitsInterface_Crop_80_40_979_659_001
 * @tc.desc: 裁剪80,40~979,659并验证输出
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsRgbaCropTest, Crop_80_40_979_659, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t left = 80;    // 80: crop left boundary
    constexpr int32_t top = 40;     // 40: crop top boundary
    constexpr int32_t right = 979;  // 979: crop right boundary
    constexpr int32_t bottom = 659; // 659: crop bottom boundary
    constexpr int32_t outW = right - left + 1;
    constexpr int32_t outH = bottom - top + 1;

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());

    SetTestHDRMetadata(inputBuffer_);

    CropRect cropRect{top, left, bottom, right};
    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret = inst.Crop(inputBuffer_, output, cropRect);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    });

    ExpectHDRMetadataEqual(inputBuffer_, output);

    EXPECT_TRUE(SaveOutput("crop_", output, param.name));
}

/**
 * @tc.name: FastKitsInterface_Crop_SameDimensions_001
 * @tc.desc: 裁剪区域与输入相同（left=0,top=0,right=W-1,bottom=H-1），输出宽高与输入一致
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsRgbaCropTest, Crop_SameDimensions, TestSize.Level1)
{
    auto param = GetParam();
    constexpr int32_t outW = INPUT_WIDTH;
    constexpr int32_t outH = INPUT_HEIGHT;
    constexpr CropRect fullRect{0, 0, INPUT_HEIGHT - 1, INPUT_WIDTH - 1};

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, param.pixelFormat);
    ASSERT_NE(output, nullptr);
    EXPECT_EQ(inputBuffer_->GetFormat(), output->GetFormat());

    SetTestHDRMetadata(inputBuffer_);

    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret = inst.Crop(inputBuffer_, output, fullRect);
        EXPECT_EQ(ret, AVCS_ERR_OK);
    });

    ExpectHDRMetadataEqual(inputBuffer_, output);

    EXPECT_TRUE(SaveOutput("crop_full_", output, param.name));
}

// ========== ROI 格式专项测试 ==========

struct RoiTestCase {
    const char *name;
    const char *input;
};

const RoiTestCase ROI_TEST_CASES[] = {
    {"LegacyDqp", "100,200-300,400=5;"},
    {"KeyValueDqp", "100,200-300,400=dqp:-6;"},
    {"KeyValueDqpSlb", "100,200-300,400=dqp:-3,slb:1;"},
    {"KeyValueSlbOnly", "100,200-300,400=slb:2;"},
    {"NoParams", "100,200-300,400;"},
    {"MultiRoi", "50,100-200,300=dqp:-6,slb:1;400,500-600,700=slb:3;"},
    {"MultiRoiMixed", "50,100-200,300=5;400,500-600,700=dqp:-3,slb:1;80,90-150,160;"},
    // No trailing semicolon cases
    {"NoTrailingSemicolon", "100,200-300,400=5"},
    {"NoTrailingSemicolonNoParam", "100,200-300,400"},
    {"NoTrailingSemicolonKeyValue", "100,200-300,400=dqp:-6,slb:1"},
    {"MultiRoiNoTrailingSemicolon", "50,100-200,300=5;400,500-600,700=dqp:-3,slb:1"},
    // Negative value after =
    {"NegativeLegacyValue", "100,200-300,400=-5;"},
    {"NegativeLegacyValueNoTrailing", "100,200-300,400=-5"},
};

class FastKitsRoiTest : public testing::TestWithParam<RoiTestCase> {
public:
    void SetUp() override
    {
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        if (inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888, FastResizeAlgoType::BICUBIC) != AVCS_ERR_OK) {
            inst.Release();
            GTEST_SKIP() << "libfast_image.so not available, skipping";
        }
        inputBuffer_ = AllocBuffer(INPUT_WIDTH, INPUT_HEIGHT, GRAPHIC_PIXEL_FMT_RGBA_8888);
        ASSERT_NE(inputBuffer_, nullptr);
        EnsureDir(MEDIA_DIR);
    }

    void TearDown() override
    {
        FastKitsInterface::GetInstance().Release();
    }

protected:
    sptr<SurfaceBuffer> inputBuffer_;
};

INSTANTIATE_TEST_SUITE_P(FastKits, FastKitsRoiTest, testing::ValuesIn(ROI_TEST_CASES),
                         [](const testing::TestParamInfo<RoiTestCase> &info) { return info.param.name; });

HWTEST_P(FastKitsRoiTest, Downsample_PreservesRoi, TestSize.Level1)
{
    auto roiInput = GetParam().input;
    constexpr int32_t outW = 636;
    constexpr int32_t outH = 360;
    double scaleX = 636.0 / INPUT_WIDTH;
    double scaleY = 360.0 / INPUT_HEIGHT;

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_NE(output, nullptr);

    SetTestROIMetadata(inputBuffer_, roiInput);

    auto &inst = FastKitsInterface::GetInstance();
    int32_t ret = inst.DownSample(inputBuffer_, output, FastResizeAlgoType::BICUBIC);
    ASSERT_EQ(ret, AVCS_ERR_OK);

    auto expected = TransformROI(roiInput, scaleX, scaleY, 0, 0);
    std::string ctx = "DownSample " + std::to_string(INPUT_WIDTH) + "x" + std::to_string(INPUT_HEIGHT) + "->" +
                      std::to_string(outW) + "x" + std::to_string(outH) + ", inputROI: " + roiInput;
    ExpectROIMetadata(output, expected, ctx);
}

HWTEST_P(FastKitsRoiTest, Crop_PreservesRoi, TestSize.Level1)
{
    auto roiInput = GetParam().input;
    constexpr int32_t left = 80;
    constexpr int32_t top = 40;
    constexpr int32_t right = 979;
    constexpr int32_t bottom = 659;
    constexpr int32_t outW = 900;
    constexpr int32_t outH = 620;

    sptr<SurfaceBuffer> output = AllocBuffer(outW, outH, GRAPHIC_PIXEL_FMT_RGBA_8888);
    ASSERT_NE(output, nullptr);

    SetTestROIMetadata(inputBuffer_, roiInput);

    auto &inst = FastKitsInterface::GetInstance();
    int32_t ret = inst.Crop(inputBuffer_, output, CropRect{top, left, bottom, right});
    ASSERT_EQ(ret, AVCS_ERR_OK);

    auto expected = TransformROI(roiInput, 1.0, 1.0, top, left);
    std::string ctx = "Crop rect[" + std::to_string(top) + "," + std::to_string(left) + "-" + std::to_string(bottom) +
                      "," + std::to_string(right) + "], inputROI: " + roiInput;
    ExpectROIMetadata(output, expected, ctx);
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
        {DS_FORMAT_PARAMS[0], 636, 360, "ds_", "rgba", dsOp},    // 636: ds width, 360: ds height
        {DS_FORMAT_PARAMS[1], 316, 180, "ds_", "1010102", dsOp}, // 316: ds width, 180: ds height
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
        return inst.Crop(in, out,
                         CropRect{40, 80, 659, 979}); // 40: top, 80: left, 659: bottom, 979: right crop boundary
    };

    const ConcurrentTask tasks[] = {
        {DS_FORMAT_PARAMS[0], 900, 620, "crop_", "rgba", cropOp}, // 900: crop output width, 620: crop output height
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

// ========== 高跨距（Slice Height）测试 ==========

void PrintBufferLayout(const sptr<SurfaceBuffer> &buffer, const char *tag)
{
    OH_NativeBuffer_Planes *planes = nullptr;
    GSError err = buffer->GetPlanesInfo(reinterpret_cast<void **>(&planes));
    printf("[%s] %dx%d fmt=%d stride=%d size=%u sliceHeight=%d", tag, buffer->GetWidth(), buffer->GetHeight(),
           buffer->GetFormat(), buffer->GetStride(), buffer->GetSize(), GetSliceHeight(buffer));
    if (err == GSERROR_OK && planes != nullptr) {
        printf(" planes=%u", planes->planeCount);
        for (uint32_t i = 0; i < planes->planeCount && i < 4; i++) { // 4: max planes
            printf(" [%u:off=%llu rs=%u cs=%u]", i, (unsigned long long)planes->planes[i].offset,
                   planes->planes[i].rowStride, planes->planes[i].columnStride);
        }
    }
    printf("\n");
}

// 读取一个 plane 的数据到 buffer，跳过 src/dst 行宽差异
bool ReadPlaneRows(std::ifstream &ifs, uint8_t *addr, int32_t stride, const PlaneReadParams &params)
{
    for (int32_t row = 0; row < params.rowCount; ++row) {
        ifs.read(reinterpret_cast<char *>(addr + params.planeOffset + row * stride), params.dstRowBytes);
        if (ifs.gcount() != params.dstRowBytes) {
            printf("[ReadPlaneRows] FAIL at row %d/%d: read %lld/%d bytes\n", row, params.rowCount,
                   static_cast<long long>(ifs.gcount()), params.dstRowBytes);
            return false;
        }
        if (params.srcRowBytes > params.dstRowBytes) {
            ifs.seekg(params.srcRowBytes - params.dstRowBytes, std::ios::cur);
        }
    }
    return true;
}

bool ReadFileToBufferByPlanes(const std::string &path, const sptr<SurfaceBuffer> &buffer, int32_t srcWidth)
{
    std::ifstream ifs(path, std::ios::binary);
    if (!ifs.is_open()) {
        printf("[ReadByPlanes] FAIL open %s\n", path.c_str());
        return false;
    }
    auto *addr = static_cast<uint8_t *>(buffer->GetVirAddr());
    if (addr == nullptr) {
        return false;
    }
    int32_t dstWidth = buffer->GetWidth();
    int32_t height = buffer->GetHeight();
    int32_t format = buffer->GetFormat();
    int32_t stride = buffer->GetStride();
    int32_t sliceHeight = GetSliceHeight(buffer);
    if (srcWidth <= 0) {
        srcWidth = dstWidth;
    }
    printf("[ReadByPlanes] %s -> %dx%d fmt=%d stride=%d sliceHeight=%d srcWidth=%d\n", path.c_str(), dstWidth, height,
           format, stride, sliceHeight, srcWidth);

    std::vector<PlaneRowInfo> dstPlanes;
    std::vector<PlaneRowInfo> srcPlanes;
    if (!GetPlaneRowInfo(format, dstWidth, height, dstPlanes) ||
        !GetPlaneRowInfo(format, srcWidth, height, srcPlanes)) {
        ifs.read(reinterpret_cast<char *>(addr), buffer->GetSize());
        return ifs.good() || ifs.eof();
    }
    // Y plane
    PlaneReadParams yParams = {0, dstPlanes[0].fileRowBytes, srcPlanes[0].fileRowBytes, dstPlanes[0].rowCount};
    if (!ReadPlaneRows(ifs, addr, stride, yParams)) {
        return false;
    }
    // UV plane: UV offset = stride * sliceHeight
    int32_t uvOff = stride * sliceHeight;
    PlaneReadParams uvParams = {uvOff, dstPlanes[1].fileRowBytes, srcPlanes[1].fileRowBytes, dstPlanes[1].rowCount};
    if (!ReadPlaneRows(ifs, addr, stride, uvParams)) {
        return false;
    }
    return true;
}

// Only test multi-planar formats (NV12, NV21, P010) — RGBA has no slice height concept
const DsFormatParam SLICE_HEIGHT_FORMAT_PARAMS[] = {
    {"NV21", "/data/test/media/1280_720_nv.yuv", GRAPHIC_PIXEL_FMT_YCRCB_420_SP},
    {"NV12", "/data/test/media/1280_720_nv.yuv", GRAPHIC_PIXEL_FMT_YCBCR_420_SP},
    {"NV21_P010", "/data/test/media/1280_720_nv12_10bit.yuv", GRAPHIC_PIXEL_FMT_YCRCB_P010},
    {"NV12_P010", "/data/test/media/1280_720_nv12_10bit.yuv", GRAPHIC_PIXEL_FMT_YCBCR_P010},
};

// ========== Slice Height 参数化组合测试 ==========
// Buffer dimension type: first char = width (w=aligned, s=has stride padding)
//                         second char = height (h=normal, s=has slice alignment)
struct BufDimType {
    const char *name;
    bool useSlice;
    int32_t width;
    int32_t height;
};

// Input buffer types (720 height)
static const BufDimType IN_BUF_TYPES[] = {
    {"wh", false, 1280, 720}, // 1280: 128-aligned, likely stride==width
    {"sh", false, 1080, 720}, // 1080: stride > width expected
    {"ws", true, 1280, 720},  // aligned width + slice height
    {"ss", true, 1080, 720},  // stride + slice height
};

// DownSample output buffer types (360 height)
static const BufDimType DS_OUT_BUF_TYPES[] = {
    {"wh", false, 640, 360}, // 640 = 5*128, aligned
    {"sh", false, 540, 360}, // unaligned
    {"ws", true, 640, 360},  // aligned + slice
    {"ss", true, 540, 360},  // unaligned + slice
};

// Crop output buffer types (640 height)
static const BufDimType CROP_OUT_BUF_TYPES[] = {
    {"wh", false, 896, 640}, // 896 = 7*128, aligned
    {"sh", false, 900, 640}, // unaligned
    {"ws", true, 896, 640},  // aligned + slice
    {"ss", true, 900, 640},  // unaligned + slice
};

struct SliceConvertParam {
    const BufDimType *inType;
    const BufDimType *outType;
    bool isCrop;
    DsFormatParam format;

    std::string Name() const
    {
        return std::string(inType->name) + "2" + outType->name + "_" + (isCrop ? "crop" : "ds") + "_" + format.name;
    }
};

std::vector<SliceConvertParam> GenerateSliceConvertParams()
{
    std::vector<SliceConvertParam> params;
    for (const auto &fmt : SLICE_HEIGHT_FORMAT_PARAMS) {
        for (const auto &in : IN_BUF_TYPES) {
            for (const auto &out : DS_OUT_BUF_TYPES) {
                params.push_back({&in, &out, false, fmt});
            }
            for (const auto &out : CROP_OUT_BUF_TYPES) {
                params.push_back({&in, &out, true, fmt});
            }
        }
    }
    return params;
}

sptr<SurfaceBuffer> AllocByType(const BufDimType &type, int32_t format)
{
    if (type.useSlice) {
        return AllocSliceBuffer(type.width, type.height, format);
    }
    return AllocBuffer(type.width, type.height, format);
}

CropRect ComputeCropRect(int32_t inW, int32_t inH, int32_t outW, int32_t outH)
{
    int32_t left = (inW - outW) / 2; // center crop horizontally
    int32_t top = (inH - outH) / 2;  // center crop vertically
    return {top, left, top + outH - 1, left + outW - 1};
}

class FastKitsSliceComboTest : public testing::TestWithParam<SliceConvertParam> {
public:
    void SetUp() override
    {
        auto &p = GetParam();
        auto &inst = FastKitsInterface::GetInstance();
        inst.Retain();
        if (inst.Init(p.format.pixelFormat, FastResizeAlgoType::BICUBIC) != AVCS_ERR_OK) {
            GTEST_SKIP() << "libfast_image.so not available";
        }

        inputBuffer_ = AllocByType(*p.inType, p.format.pixelFormat);
        if (inputBuffer_ == nullptr) {
            GTEST_SKIP() << "Input alloc failed for " << p.format.name;
        }
        EnsureDir(MEDIA_DIR);
        ASSERT_TRUE(ReadFileToBufferByPlanes(p.format.inputFile, inputBuffer_, SRC_FILE_WIDTH));
        PrintBufferLayout(inputBuffer_, p.inType->name);
        SaveOutput("in_", inputBuffer_, p.format.name);

        outputBuffer_ = AllocByType(*p.outType, p.format.pixelFormat);
        if (outputBuffer_ == nullptr) {
            GTEST_SKIP() << "Output alloc failed for " << p.format.name;
        }
        PrintBufferLayout(outputBuffer_, p.outType->name);
    }

    void TearDown() override
    {
        FastKitsInterface::GetInstance().Release();
    }

protected:
    sptr<SurfaceBuffer> inputBuffer_;
    sptr<SurfaceBuffer> outputBuffer_;
};

INSTANTIATE_TEST_SUITE_P(FastKits, FastKitsSliceComboTest, testing::ValuesIn(GenerateSliceConvertParams()),
                         [](const testing::TestParamInfo<SliceConvertParam> &info) { return info.param.Name(); });

/**
 * @tc.name: FastKitsInterface_SliceCombo_Convert
 * @tc.desc: 参数化测试 wh/sh/ws/ss × DownSample/Crop，验证各种 stride+slice 组合
 * @tc.type: FUNC
 */
HWTEST_P(FastKitsSliceComboTest, Convert, TestSize.Level1)
{
    auto &p = GetParam();
    std::string prefix = std::string(p.inType->name) + "2" + p.outType->name + "_" + (p.isCrop ? "crop_" : "ds_");

    SetTestHDRMetadata(inputBuffer_);

    EXPECT_NO_FATAL_FAILURE({
        auto &inst = FastKitsInterface::GetInstance();
        int32_t ret;
        if (p.isCrop) {
            CropRect cropRect = ComputeCropRect(p.inType->width, p.inType->height, p.outType->width, p.outType->height);
            ret = inst.Crop(inputBuffer_, outputBuffer_, cropRect);
        } else {
            ret = inst.DownSample(inputBuffer_, outputBuffer_, FastResizeAlgoType::BICUBIC);
        }
        EXPECT_EQ(ret, AVCS_ERR_OK) << prefix << " failed";
    });

    ExpectHDRMetadataEqual(inputBuffer_, outputBuffer_);

    int32_t outSize = CalcDataSize(outputBuffer_);
    uint8_t *data = static_cast<uint8_t *>(outputBuffer_->GetVirAddr());
    ASSERT_NE(data, nullptr);
    bool allZero = true;
    for (int32_t i = 0; i < outSize; i++) {
        if (data[i] != 0) {
            allZero = false;
            break;
        }
    }
    EXPECT_FALSE(allZero) << prefix << " output all zero";
    EXPECT_TRUE(SaveOutput(prefix, outputBuffer_, p.format.name));
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

    for (int i = 0; i < 5; i++) { // 5: number of retain/init/release cycles
        inst.Retain();
        EXPECT_EQ(inst.Init(GRAPHIC_PIXEL_FMT_RGBA_8888), AVCS_ERR_OK) << "Cycle " << i << ": Init failed";
        EXPECT_TRUE(inst.IsLoaded()) << "Cycle " << i << ": not loaded";
        inst.Release();
        EXPECT_FALSE(inst.IsLoaded()) << "Cycle " << i << ": still loaded after Release";
    }
}

} // namespace PreProcessing
} // namespace MediaAVCodec
} // namespace OHOS
