/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "window.h"
#include "unittest_log.h"

#include "native_avcapability.h"
#include "avcodec_info.h"

#ifdef __cplusplus
extern "C" {
#endif
#include <libavformat/avformat.h>
#ifdef __cplusplus
}
#endif

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
const uint32_t ES_VC1[] = { // VC1_FRAME_SIZE_240
    205160, 102763, 98905,  101764, 100485, 100254, 103866, 110127, 107659, 103838,
    103995, 109044, 105965, 101452, 209208, 106389, 105631, 102050, 105288, 104468,
    101409, 100080, 104598, 104097, 102008, 104368, 106377, 102195, 211446, 105566,
    105267, 103148, 101445, 109630, 107045, 103166, 108955, 109065, 103615, 105870,
    106833, 107903, 209977, 101912, 109641, 108617, 102896, 104681, 108202, 103409,
    106049, 106085, 106617, 101801, 102721, 109604};
constexpr uint32_t ES_LENGTH_VC1 = sizeof(ES_VC1) / sizeof(uint32_t);
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr string_view inputFilePath = "/data/test/media/test.vc1";
constexpr string_view outputFilePath = "/data/test/media/test.yuv";
uint32_t g_writeFrameCount = 0;
uint8_t* g_extraData = nullptr;
int64_t g_extraDataSize = 0;
constexpr string_view filename = "/data/test/media/glitch-ffvc1.avi";
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
        g_writeFrameCount += attr->size > 0 ? 1 : 0;
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

class VideoCodeCapiVC1DecoderUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t ProceFunc();
    void InputFunc();
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

void VideoCodeCapiVC1DecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void VideoCodeCapiVC1DecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void VideoCodeCapiVC1DecoderUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
    g_writeFrameCount = 0;
}

void VideoCodeCapiVC1DecoderUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
    OH_VideoDecoder_Destroy(videoDec_);
}

void VideoCodeCapiVC1DecoderUnitTest::InputFunc()
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
        if (frameCount_ < ES_LENGTH_VC1) {
            info.size = ES_VC1[frameCount_];
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

void VideoCodeCapiVC1DecoderUnitTest::OutputFunc()
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
            cout << "decode eos, write frame:" << g_writeFrameCount << endl;
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

int32_t VideoCodeCapiVC1DecoderUnitTest::ProceFunc(void)
{
    videoDec_ = OH_VideoDecoder_CreateByName((AVCodecCodecName::VIDEO_DECODER_VC1_NAME).data());
    EXPECT_NE(nullptr, videoDec_);

    signal_ = make_shared<VDecSignal>();
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetCallback(videoDec_, cb_, signal_.get()));

    format_ = OH_AVFormat_Create();
    AVFormatContext* formatContext = avformat_alloc_context();
    int ret = avformat_open_input(&formatContext, filename.data(), nullptr, nullptr);
    if (ret != 0) {
        cout << "avformat_open_input error" << endl;
    }
    ret = avformat_find_stream_info(formatContext, nullptr);
    if (ret != 0) {
        cout << "avformat_find_stream_info error" << endl;
    }
    for (int i = 0; i < formatContext->nb_streams; i++) {
        AVStream* stream = formatContext->streams[i];
        if (stream->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            g_extraData = stream->codecpar->extradata;
            g_extraDataSize = stream->codecpar->extradata_size;
            return AVCS_ERR_OK;
        }
    }
    return AVCS_ERR_OK;
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Create_VC1_01, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName("");
    EXPECT_EQ(nullptr, videoDec_);
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Create_VC1_02, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName("h266");
    EXPECT_EQ(nullptr, videoDec_);
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Configure_VC1_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Configure_VC1_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Configure_VC1_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Configure_VC1_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Configure_VC1_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, 1000000);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Configure_VC1_06, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_SCALING_MODE, DEFAULT_HEIGHT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Configure_VC1_07, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, OH_MD_MAX_INPUT_BUFFER_COUNT, 4);
    OH_AVFormat_SetIntValue(format_, OH_MD_MAX_OUTPUT_BUFFER_COUNT, 4);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_BITRATE, 30000);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 4);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Start_VC1_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Start_VC1_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Start_VC1_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_Start_VC1_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_getOutputFormat_VC1_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    auto tFormat = OH_VideoDecoder_GetOutputDescription(videoDec_);
    EXPECT_NE(nullptr, tFormat);
    int32_t ret = 0;
    OH_AVFormat_GetIntValue(tFormat, OH_MD_KEY_WIDTH, &ret);
    EXPECT_EQ(DEFAULT_WIDTH, ret);
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_getOutputFormat_VC1_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    auto tFormat = OH_VideoDecoder_GetOutputDescription(videoDec_);
    EXPECT_NE(nullptr, tFormat);
    int32_t ret = 0;
    OH_AVFormat_GetIntValue(tFormat, OH_MD_KEY_WIDTH, &ret);
    EXPECT_EQ(DEFAULT_WIDTH, ret);
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_SetParameter_VC1_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::OutputFunc, this);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_SetParameter_VC1_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_SetParameter_VC1_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_MAX_INPUT_SIZE, 114514);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_normalcase_VC1_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV21);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::OutputFunc, this);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_normalcase_VC1_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);
    outputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::OutputFunc, this);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_normalcase_VC1_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::OutputFunc, this);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_normalcase_VC1_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::OutputFunc, this);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_normalcase_VC1_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);
    outputLoop_ = make_unique<thread>(&VideoCodeCapiVC1DecoderUnitTest::OutputFunc, this);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_01, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_02, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_03, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_06, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_07, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_abnormalcase_VC1_08, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_statuscase_VC1_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_setcallback_VC1_01, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName((AVCodecCodecName::VIDEO_DECODER_VC1_NAME).data());
    EXPECT_NE(nullptr, videoDec_);
    cb_ = {nullptr, nullptr, nullptr, nullptr};
    signal_ = make_shared<VDecSignal>();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetCallback(videoDec_, cb_, signal_.get()));
}

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_pushInputData_VC1_01, TestSize.Level1)
{
    OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
    int32_t bufferSize = 13571;
    testFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr, "Fatal: No memory");

    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_pushInputData_VC1_02, TestSize.Level1)
{
    OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
    int32_t bufferSize = 13571;
    testFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(testFile_ != nullptr, "Fatal: No memory");

    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
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

HWTEST_F(VideoCodeCapiVC1DecoderUnitTest, videoDecoder_getOutputBuffer_VC1_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetBuffer(format_, MediaDescriptionKey::MD_KEY_CODEC_CONFIG.data(), g_extraData,
                          g_extraDataSize);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputData(videoDec_, 0));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputData(videoDec_, -1));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputData(videoDec_, 1024));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputData(videoDec_, 0));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputData(videoDec_, -1));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputData(videoDec_, 1024));
}
} // namespace VCodecUT
} // namespace MediaAVCodec
} // namespace OHOS