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

#include <atomic>
#include <fstream>
#include <iostream>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include <vector>
#include <gtest/gtest.h>
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "iconsumer_surface.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_avformat.h"
#include "securec.h"
#include "surface/window.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
const uint32_t ES_H264[] = { // H264_FRAME_SIZE_240
    2106, 11465, 321,  72,   472,   68,   76,   79,    509,  90,   677,  88,   956,  99,   347,  77,   452,  681,  81,
    1263, 94,    106,  97,   998,   97,   797,  93,    1343, 150,  116,  117,  926,  1198, 128,  110,  78,   1582, 158,
    135,  112,   1588, 165,  132,   128,  1697, 168,   149,  117,  1938, 170,  141,  142,  1830, 106,  161,  122,  1623,
    160,  154,   156,  1998, 230,   177,  139,  1650,  186,  128,  134,  1214, 122,  1411, 120,  1184, 128,  1591, 195,
    145,  105,   1587, 169,  140,   118,  1952, 177,   150,  161,  1437, 159,  123,  1758, 180,  165,  144,  1936, 214,
    191,  175,   2122, 180,  179,   160,  1927, 161,   184,  119,  1973, 218,  210,  129,  1962, 196,  127,  154,  2308,
    173,  127,   1572, 142,  122,   2065, 262,  159,   206,  2251, 269,  179,  170,  2056, 308,  168,  191,  2090, 303,
    191,  110,   1932, 272,  162,   122,  1877, 245,   167,  141,  1908, 294,  162,  118,  1493, 132,  1782, 273,  184,
    133,  1958,  274,  180,  149,   2070, 216,  169,   143,  1882, 224,  149,  139,  1749, 277,  184,  139,  2141, 197,
    170,  140,   2002, 269,  162,   140,  1862, 202,   179,  131,  1868, 214,  164,  140,  1546, 226,  150,  130,  1707,
    162,  146,   1824, 181,  147,   130,  1898, 209,   143,  131,  1805, 180,  148,  106,  1776, 147,  141,  1572, 177,
    130,  105,   1776, 178,  144,   122,  1557, 142,   124,  114,  1436, 143,  126,  1326, 127,  1755, 169,  127,  105,
    1807, 177,   131,  134,  1613,  187,  137,  136,   1314, 134,  118,  2005, 194,  129,  147,  1566, 185,  132,  131,
    1236, 174,   137,  106,  11049, 574,  126,  1242,  188,  130,  119,  1450, 187,  137,  141,  1116, 124,  1848, 138,
    122,  1605,  186,  127,  140,   1798, 170,  124,   121,  1666, 157,  128,  130,  1678, 135,  118,  1804, 169,  135,
    125,  1837,  168,  124,  124,   2049, 180,  122,   128,  1334, 143,  128,  1379, 116,  1884, 149,  122,  150,  1962,
    176,  122,   122,  1197, 139,   1853, 184,  151,   148,  1692, 209,  129,  126,  1736, 149,  135,  104,  1775, 165,
    160,  121,   1653, 163,  123,   112,  1907, 181,   129,  107,  1808, 177,  125,  111,  2405, 166,  144,  114,  1833,
    198,  136,   113,  1960, 206,   139,  116,  1791,  175,  130,  129,  1909, 194,  138,  119,  1807, 160,  156,  124,
    1998, 184,   173,  114,  2069,  181,  127,  139,   2212, 182,  138,  146,  1993, 214,  135,  139,  2286, 194,  137,
    120,  2090,  196,  159,  132,   2294, 194,  148,   137,  2312, 183,  163,  106,  2118, 201,  158,  127,  2291, 187,
    144,  116,   2413, 139,  115,   2148, 178,  122,   103,  2370, 207,  161,  117,  2291, 213,  159,  129,  2244, 243,
    157,  133,   2418, 255,  171,   127,  2316, 185,   160,  132,  2405, 220,  165,  155,  2539, 219,  172,  128,  2433,
    199,  154,   119,  1681, 140,   1960, 143,  2682,  202,  153,  127,  2794, 239,  158,  155,  2643, 229,  172,  125,
    2526, 201,   181,  159,  2554,  233,  167,  125,   2809, 205,  164,  117,  2707, 221,  156,  138,  2922, 240,  160,
    146,  2952,  267,  177,  149,   3339, 271,  175,   136,  3006, 242,  168,  141,  3068, 232,  194,  149,  2760, 214,
    208,  143,   2811, 218,  184,   149,  137,  15486, 2116, 235,  167,  157,  2894, 305,  184,  139,  3090, 345,  179,
    155,  3226,  347,  160,  164,   3275, 321,  184,   174,  3240, 269,  166,  170,  3773, 265,  169,  155,  3023, 301,
    188,  161,   3343, 275,  174,   155,  3526, 309,   177,  173,  3546, 307,  183,  149,  3648, 295,  213,  170,  3568,
    305,  198,   166,  3641, 297,   172,  148,  3608,  301,  200,  159,  3693, 322,  209,  166,  3453, 318,  206,  162,
    3696, 341,   200,  176,  3386,  320,  192,  176,   3903, 373,  207,  187,  3305, 361,  200,  202,  3110, 367,  220,
    197,  2357,  332,  196,  201,   1827, 377,  187,   199,  860,  472,  173,  223,  238};
const uint32_t FC_H264[] = {139107, 1114, 474, 253, 282,    146,  197, 90,  108, 3214, 301, 77, 51, 43,
                            234,    210,  143, 108, 139107, 1114, 474, 253, 282, 146,  197, 90, 108};
constexpr uint32_t ES_LENGTH_H264 = sizeof(ES_H264) / sizeof(uint32_t);
constexpr uint32_t FC_LENGTH_H264 = sizeof(FC_H264) / sizeof(uint32_t);
constexpr uint32_t DEFAULT_WIDTH = 320;
constexpr uint32_t DEFAULT_HEIGHT = 240;
constexpr string_view inputFilePath = "/data/test/media/out_320_240_10s.h264";
constexpr string_view formatChangeInputFilePath = "/data/test/media/format_change_testseq.h264";
constexpr string_view outputFilePath = "/data/test/media/out_320_240_10s.yuv";
constexpr string_view outputSurfacePath = "/data/test/media/out_320_240_10s.rgba";
uint32_t writeFrameCount = 0;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
namespace VCodecUT {
class VDecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVMemory *> inBufferQueue_;
    std::queue<OH_AVMemory *> outBufferQueue_;
    std::queue<OH_AVCodecBufferAttr> attrQueue_;
};

static void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
    cout << "Error received, errorCode:" << errorCode << endl;
}

static void OnOutputFormatChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)format;
    (void)userData;
    cout << "OnOutputFormatChanged received" << endl;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                    void *userData)
{
    (void)codec;
    VDecSignal *signal = static_cast<VDecSignal *>(userData);
    if (attr) {
        unique_lock<mutex> lock(signal->outMutex_);
        signal->outQueue_.push(index);
        signal->outBufferQueue_.push(data);
        signal->attrQueue_.push(*attr);
        writeFrameCount += attr->size > 0 ? 1 : 0;
        signal->outCond_.notify_all();
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
}

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t timestamp_ = 0;
    OHOS::Rect damage_ = {};
    sptr<Surface> cs_ = nullptr;
    std::unique_ptr<std::ofstream> outFile_;
};

TestConsumerListener::TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs_(cs)
{
    outFile_ = std::make_unique<std::ofstream>();
    outFile_->open(name.data(), std::ios::out | std::ios::binary);
}

TestConsumerListener::~TestConsumerListener()
{
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

static sptr<Surface> GetSurface()
{
    sptr<Surface> cs = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs, outputSurfacePath);
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    sptr<Surface> ps = Surface::CreateSurfaceAsProducer(p);
    return ps;
}

class VideoCodeCapiDecoderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t ProceFunc();
    void InputFunc();
    void FormatChangeInputFunc();
    void OutputFunc();

protected:
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> testFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;

    struct OH_AVCodecAsyncCallback cb_;
    std::shared_ptr<VDecSignal> signal_ = nullptr;
    OH_AVCodec *videoDec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
    sptr<Surface> surface_ = nullptr;
};

void VideoCodeCapiDecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void VideoCodeCapiDecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void VideoCodeCapiDecoderUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
    writeFrameCount = 0;
}

void VideoCodeCapiDecoderUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
    OH_VideoDecoder_Destroy(videoDec_);
}

void VideoCodeCapiDecoderUnitTest::InputFunc()
{
    testFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr, "Fatal: No memory");
    testFile_->open(inputFilePath, std::ios::in | std::ios::binary);
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
        if (frameCount_ < ES_LENGTH_H264) {
            info.size = ES_H264[frameCount_];
            char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * info.size + 1));
            UNITTEST_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail.");
            (void)testFile_->read(fileBuffer, info.size);
            if (memcpy_s(OH_AVMemory_GetAddr(buffer), OH_AVMemory_GetSize(buffer), fileBuffer, info.size) != EOK) {
                cout << "Fatal: memcpy fail" << endl;
                free(fileBuffer);
                break;
            }
            free(fileBuffer);
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            if (isFirstFrame_) {
                info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                isFirstFrame_ = false;
            }
            int32_t ret = OH_VideoDecoder_PushInputData(videoDec_, index, info);
            UNITTEST_CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Fatal error, exit.");
            frameCount_++;
        } else {
            OH_VideoDecoder_PushInputData(videoDec_, index, info);
            std::cout << "input end buffer" << std::endl;
            break;
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
    if (testFile_ != nullptr) {
        testFile_->close();
    }
}

void VideoCodeCapiDecoderUnitTest::FormatChangeInputFunc()
{
    testFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr, "Fatal: No memory");
    testFile_->open(formatChangeInputFilePath, std::ios::in | std::ios::binary);
    while (true) {
        if (!isRunning_.load()) {
            break;
        }
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
        if (frameCount_ < FC_LENGTH_H264) {
            info.size = FC_H264[frameCount_];
            char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * info.size + 1));
            UNITTEST_CHECK_AND_RETURN_LOG(fileBuffer != nullptr, "Fatal: malloc fail.");
            (void)testFile_->read(fileBuffer, info.size);
            if (memcpy_s(OH_AVMemory_GetAddr(buffer), OH_AVMemory_GetSize(buffer), fileBuffer, info.size) != EOK) {
                cout << "Fatal: memcpy fail" << endl;
                free(fileBuffer);
                break;
            }
            free(fileBuffer);
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            if (isFirstFrame_) {
                info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
                isFirstFrame_ = false;
            }
            int32_t ret = OH_VideoDecoder_PushInputData(videoDec_, index, info);
            UNITTEST_CHECK_AND_RETURN_LOG(ret == AVCS_ERR_OK, "Fatal error, exit.");
            frameCount_++;
        } else {
            OH_VideoDecoder_PushInputData(videoDec_, index, info);
            std::cout << "input end buffer" << std::endl;
            break;
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
    if (testFile_ != nullptr) {
        testFile_->close();
    }
}

void VideoCodeCapiDecoderUnitTest::OutputFunc()
{
    if (!surface_) {
        outFile_ = std::make_unique<std::ofstream>();
        UNITTEST_CHECK_AND_RETURN_LOG(outFile_ != nullptr, "Fatal: No memory");
        outFile_->open(outputFilePath.data(), std::ios::out | std::ios::binary);
    }

    while (true) {
        if (!isRunning_.load()) {
            cout << "stop, exit" << endl;
            break;
        }

        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVMemory *data = signal_->outBufferQueue_.front();
        if (outFile_ != nullptr && attr.size != 0 && data != nullptr && OH_AVMemory_GetAddr(data) != nullptr) {
            outFile_->write(reinterpret_cast<char *>(OH_AVMemory_GetAddr(data)), attr.size);
        }

        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos, write frame:" << writeFrameCount << endl;
            isRunning_.store(false);
        }
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        if (surface_) {
            EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RenderOutputData(videoDec_, index));
        } else {
            EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_FreeOutputData(videoDec_, index));
        }
    }
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

int32_t VideoCodeCapiDecoderUnitTest::ProceFunc(void)
{
    videoDec_ = OH_VideoDecoder_CreateByName((AVCodecCodecName::VIDEO_DECODER_AVC_NAME).data());
    EXPECT_NE(nullptr, videoDec_);

    signal_ = make_shared<VDecSignal>();
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetCallback(videoDec_, cb_, signal_.get()));

    format_ = OH_AVFormat_Create();
    return AVCS_ERR_OK;
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Create_01, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName("");
    EXPECT_EQ(nullptr, videoDec_);
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Create_02, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName("h266");
    EXPECT_EQ(nullptr, videoDec_);
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_02, TestSize.Level1)
{
    ProceFunc();
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_getOutputFormat_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_getOutputFormat_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_SetParameter_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_SetParameter_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_SetParameter_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_MAX_INPUT_SIZE, 114514);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));

    surface_ = GetSurface();
    EXPECT_NE(nullptr, surface_);
    OHNativeWindow *nativeWindow = CreateNativeWindowFromSurface(&surface_);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetSurface(videoDec_, nativeWindow));

    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);
    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_06, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);
    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_formatchange, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, 1920);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, 1080);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));

    surface_ = GetSurface();
    EXPECT_NE(nullptr, surface_);
    OHNativeWindow *nativeWindow = CreateNativeWindowFromSurface(&surface_);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetSurface(videoDec_, nativeWindow));

    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::FormatChangeInputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiDecoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.notify_all();
        lock.unlock();
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.notify_all();
        lock.unlock();
        outputLoop_->join();
    }
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_01, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_02, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_03, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_06, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_07, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_08, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_statuscase_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_statuscase_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_setcallback_01, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName((AVCodecCodecName::VIDEO_DECODER_AVC_NAME).data());
    EXPECT_NE(nullptr, videoDec_);
    cb_ = {nullptr, nullptr, nullptr, nullptr};
    signal_ = make_shared<VDecSignal>();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetCallback(videoDec_, cb_, signal_.get()));
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_pushInputData_01, TestSize.Level1)
{
    OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
    int32_t bufferSize = 13571;
    testFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr, "Fatal: No memory");

    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));

    isRunning_.store(true);
    testFile_->open(inputFilePath, std::ios::in | std::ios::binary);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
    uint32_t index = signal_->inQueue_.front();
    auto buffer = signal_->inBufferQueue_.front();
    info.size = bufferSize;
    char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * info.size + 1));
    (void)testFile_->read(fileBuffer, info.size);
    memcpy_s(OH_AVMemory_GetAddr(buffer), OH_AVMemory_GetSize(buffer), fileBuffer, info.size);
    free(fileBuffer);
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_PushInputData(videoDec_, index, info));
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();

    index = 0;
    buffer = signal_->inBufferQueue_.front();
    info.size = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_PushInputData(videoDec_, index, info));
    testFile_->close();
    isRunning_.store(false);
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_pushInputData_02, TestSize.Level1)
{
    OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
    int32_t bufferSize = 13571;
    testFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr, "Fatal: No memory");

    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));

    isRunning_.store(true);
    testFile_->open(inputFilePath, std::ios::in | std::ios::binary);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
    uint32_t index = 1024;
    auto buffer = signal_->inBufferQueue_.front();
    info.size = bufferSize;
    char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * info.size + 1));
    (void)testFile_->read(fileBuffer, info.size);
    memcpy_s(OH_AVMemory_GetAddr(buffer), OH_AVMemory_GetSize(buffer), fileBuffer, info.size);
    free(fileBuffer);
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_PushInputData(videoDec_, index, info));
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();

    testFile_->close();
    isRunning_.store(false);
}

HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_getOutputBuffer_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputData(videoDec_, 0));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputData(videoDec_, -1));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputData(videoDec_, 1024));
    surface_ = GetSurface();
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputData(videoDec_, 0));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputData(videoDec_, -1));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputData(videoDec_, 1024));
}
} // namespace VCodecUT
} // namespace MediaAVCodec
} // namespace OHOS