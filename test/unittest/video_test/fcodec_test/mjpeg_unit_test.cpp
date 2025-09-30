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

extern "C" {
#include "libavcodec/avcodec.h"
}

using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace std;
using namespace testing::ext;
namespace {
constexpr uint32_t DEFAULT_WIDTH = 640;
constexpr uint32_t DEFAULT_HEIGHT = 360;
constexpr uint32_t FRAME_DURATION_US = 33000;

constexpr string_view inputFilePath = "/data/test/media/origin_mjpeg.avi";
constexpr string_view outputFilePath = "/data/test/media/origin_mjpeg.yuv";
constexpr string_view outputSurfacePath = "/data/test/media/out_320_240_10s.rgba";
constexpr uint32_t SLEEP_TIME = 1;
uint32_t g_outFrameCount = 0;
} // namespace

namespace OHOS {
namespace MediaAVCodec {
class VDecSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
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

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    VDecSignal *signal_ = static_cast<VDecSignal *>(userData);
    cout << "OnInputBufferAvailable received, index:" << index << endl;
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inQueue_.push(index);
    signal_->inBufferQueue_.push(data);
    signal_->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    VDecSignal *signal_ = static_cast<VDecSignal *>(userData);
    if (data) {
        int size = OH_AVBuffer_GetCapacity(data);
        cout << "OnOutputBufferAvailable received, index:" << index << ", data->size:" << size << endl;
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outQueue_.push(index);
        signal_->outBufferQueue_.push(data);
        OH_AVCodecBufferAttr attr;
        OH_AVBuffer_GetBufferAttr(data, &attr);
        signal_->attrQueue_.push(attr);
        g_outFrameCount += 1;
        signal_->outCond_.notify_all();
    } else {
        cout << "OnOutputBufferAvailable error, data is nullptr!" << endl;
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
    void VDecDemo();
    void DelVDecDemo();
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t ProceFunc();
    void InputFunc();
    void OutputFunc();

protected:
    int32_t CreateDec();
    int32_t Configure(OH_AVFormat *format);
    int32_t SetSurface(OHNativeWindow *window);
    int32_t Start();
    int32_t Stop();
    int32_t Flush();
    int32_t Reset();
    int32_t Release();
    int32_t ExtractPacket();
    std::atomic<bool> isRunning_ = false;
    std::unique_ptr<std::ifstream> inputFile_;
    std::unique_ptr<std::ofstream> outFile_;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    OH_AVCodec *videoDec_ = nullptr;
    struct OH_AVCodecCallback cb_;
    VDecSignal *signal_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
    sptr<Surface> surface_ = nullptr;
    int64_t timeStamp_ = 0;

    // Extract packet
    static constexpr int32_t VIDEO_INBUF_SIZE = 10240;
    static constexpr int32_t VIDEO_REFILL_THRESH = 4096;
    const AVCodec *codec_ = nullptr;
    AVCodecParserContext *parser_ = nullptr;
    AVCodecContext *codec_ctx_ = nullptr;
    AVPacket *pkt_ = nullptr;
    size_t data_size_ = 0;
    uint8_t *data_ = nullptr;
    uint8_t inbuf_[VIDEO_INBUF_SIZE + 64] = {0};
    bool file_end_ = false;
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
    g_outFrameCount = 0;
}

void VideoCodeCapiDecoderUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
    OH_VideoDecoder_Destroy(videoDec_);
}

void VideoCodeCapiDecoderUnitTest::InputFunc()
{
    inputFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(inputFile_ != nullptr, "Fatal: No memory");
    inputFile_->open(inputFilePath.data(), std::ios::in | std::ios::binary);
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        lock.unlock();
        if (!file_end_ && (ExtractPacket() != AVCS_ERR_OK || pkt_->size == 0)) {
            continue;
        }
        OH_AVCodecBufferAttr info;
        if (file_end_) {
            info.pts = 0;
            info.size = 0;
            info.offset = 0;
            info.flags = AVCODEC_BUFFER_FLAGS_EOS;
            OH_AVBuffer_SetBufferAttr(buffer, &info);
            OH_VideoDecoder_PushInputBuffer(videoDec_, index);
            cout << "push end" << endl;
            break;
        }
        info.size = pkt_->size;
        info.offset = 0;
        info.pts = pkt_->pts;
        UNITTEST_CHECK_AND_RETURN_LOG(buffer != nullptr, "Fatal: GetInputBuffer fail");
        memcpy_s(OH_AVBuffer_GetAddr(buffer), pkt_->size, pkt_->data, pkt_->size);
        int32_t ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            info.flags = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
            OH_AVBuffer_SetBufferAttr(buffer, &info);
            ret = OH_VideoDecoder_PushInputBuffer(videoDec_, index);
            isFirstFrame_ = false;
        } else {
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            OH_AVBuffer_SetBufferAttr(buffer, &info);
            ret = OH_VideoDecoder_PushInputBuffer(videoDec_, index);
        }
        if (ret != AVCS_ERR_OK) {
            cout << "Fatal error, exit" << endl;
            break;
        }

        timeStamp_ += FRAME_DURATION_US;
        lock.lock();
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
    }
}

void VideoCodeCapiDecoderUnitTest::OutputFunc()
{
    if (!surface_) {
        outFile_ = std::make_unique<std::ofstream>();
        UNITTEST_CHECK_AND_RETURN_LOG(outFile_ != nullptr, "Fatal: No memory");
        outFile_->open(outputFilePath.data(), std::ios::out | std::ios::binary);
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "wait to stop, exit" << endl;
            break;
        }
        uint32_t index = signal_->outQueue_.front();
        OH_AVCodecBufferAttr attr = signal_->attrQueue_.front();
        OH_AVBuffer *data = signal_->outBufferQueue_.front();
        lock.unlock();
        if (outFile_ != nullptr && attr.size != 0 && data != nullptr && OH_AVBuffer_GetAddr(data) != nullptr) {
            cout << "OutputFunc write file,buffer index" << index << ", data size = :" << attr.size << endl;
            outFile_->write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(data)), attr.size);
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "decode eos, write frame:" << g_outFrameCount << endl;
            isRunning_.store(false);
        }
        if (surface_) {
            EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RenderOutputBuffer(videoDec_, index));
        } else {
            EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_FreeOutputBuffer(videoDec_, index));
        }
        lock.lock();
        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
    }
}

void VideoCodeCapiDecoderUnitTest::VDecDemo()
{
    codec_ = avcodec_find_decoder(AV_CODEC_ID_MJPEG);
    if (codec_ == nullptr) {
        std::cout << "find mjpeg fail" << std::endl;
        exit(1);
    }

    parser_ = av_parser_init(codec_->id);
    if (parser_ == nullptr) {
        std::cout << "parser init fail" << std::endl;
        exit(1);
    }

    codec_ctx_ = avcodec_alloc_context3(codec_);
    if (codec_ctx_ == nullptr) {
        std::cout << "alloc context fail" << std::endl;
        exit(1);
    }

    if (avcodec_open2(codec_ctx_, codec_, NULL) < 0) {
        std::cout << "codec open fail" << std::endl;
        exit(1);
    }

    pkt_ = av_packet_alloc();
    if (pkt_ == nullptr) {
        std::cout << "alloc pkt fail" << std::endl;
        exit(1);
    }
    pkt_->data = NULL;
    pkt_->size = 0;
}

void VideoCodeCapiDecoderUnitTest::DelVDecDemo()
{
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }

    avcodec_free_context(&codec_ctx_);
    av_parser_close(parser_);
    av_packet_free(&pkt_);

    if (inputFile_ != nullptr) {
        inputFile_->close();
    }

    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

int32_t VideoCodeCapiDecoderUnitTest::ProceFunc(void)
{
    videoDec_ = OH_VideoDecoder_CreateByName(AVCodecCodecName::VIDEO_DECODER_MJPEG_NAME.data());
    EXPECT_NE(nullptr, videoDec_);

    signal_ = new VDecSignal();
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec_, cb_, signal_));

    format_ = OH_AVFormat_Create();
    return AVCS_ERR_OK;
}

int32_t VideoCodeCapiDecoderUnitTest::ExtractPacket()
{
    int32_t len = 0;
    int32_t ret = 0;

    if (data_ == nullptr) {
        data_ = inbuf_;
        (void)inputFile_->read(reinterpret_cast<char *>(inbuf_), VIDEO_INBUF_SIZE);
        data_size_ = inputFile_->gcount();
    }

    if ((data_size_ < VIDEO_REFILL_THRESH) && !file_end_) {
        memmove_s(inbuf_, data_size_, data_, data_size_);
        data_ = inbuf_;
        (void)inputFile_->read(reinterpret_cast<char *>(data_ + data_size_), VIDEO_INBUF_SIZE - data_size_);
        len = inputFile_->gcount();
        if (len > 0) {
            data_size_ += len;
        } else if (len == 0 && data_size_ == 0) {
            file_end_ = true;
            cout << "extract file end" << endl;
        }
    }

    if (data_size_ > 0) {
        ret = av_parser_parse2(parser_, codec_ctx_, &pkt_->data, &pkt_->size, data_, data_size_, AV_NOPTS_VALUE,
                               AV_NOPTS_VALUE, 0);
        if (ret < 0) {
            cout << "av_parser_parser2 Error!" << endl;
        }
        data_ += ret;
        data_size_ -= ret;
        if (pkt_->size) {
            return AVCS_ERR_OK;
        } else {
            return AVCS_ERR_UNKNOWN;
        }
    }
    return AVCS_ERR_UNKNOWN;
}


/**
 * @tc.name: videoDecoder_Create_01
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Create_01, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName("");
    EXPECT_EQ(nullptr, videoDec_);
}

/**
 * @tc.name: videoDecoder_Create_02
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Create_02, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName("h266");
    EXPECT_EQ(nullptr, videoDec_);
}

/**
 * @tc.name: videoDecoder_Configure_01
 * @tc.desc: video configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

/**
 * @tc.name: videoDecoder_Configure_02
 * @tc.desc: video configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

/**
 * @tc.name: videoDecoder_Configure_03
 * @tc.desc: video configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

/**
 * @tc.name: videoDecoder_Configure_04
 * @tc.desc: video configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

/**
 * @tc.name: videoDecoder_Configure_05
 * @tc.desc: video configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), 1000000);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

/**
 * @tc.name: videoDecoder_Configure_06
 * @tc.desc: video configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_06, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_SCALING_MODE, DEFAULT_HEIGHT);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

/**
 * @tc.name: videoDecoder_Configure_07
 * @tc.desc: video configure
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Configure_07, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    OH_AVFormat_SetIntValue(format_, OH_MD_MAX_INPUT_BUFFER_COUNT, 4);
    OH_AVFormat_SetIntValue(format_, OH_MD_MAX_OUTPUT_BUFFER_COUNT, 4);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_BITRATE, 30000);
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_VIDEO_ENCODER_ENABLE_TEMPORAL_SCALABILITY, 4);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

/**
 * @tc.name: videoDecoder_Start_01
 * @tc.desc: video start
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

/**
 * @tc.name: videoDecoder_Start_02
 * @tc.desc: video start
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_02, TestSize.Level1)
{
    ProceFunc();
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

/**
 * @tc.name: videoDecoder_Start_03
 * @tc.desc: video start
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

/**
 * @tc.name: videoDecoder_Start_04
 * @tc.desc: video start
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_Start_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

/**
 * @tc.name: videoDecoder_getOutputFormat_01
 * @tc.desc: video output format
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_getOutputFormat_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

/**
 * @tc.name: videoDecoder_getOutputFormat_01
 * @tc.desc: video output format
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_getOutputFormat_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

/**
 * @tc.name: videoDecoder_SetParameter_01
 * @tc.desc: video SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_SetParameter_01, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_SetParameter_02
 * @tc.desc: video SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_SetParameter_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

/**
 * @tc.name: videoDecoder_SetParameter_03
 * @tc.desc: video SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_SetParameter_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_MAX_INPUT_SIZE, 114514);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format_));
    EXPECT_NE(nullptr, OH_VideoDecoder_GetOutputDescription(videoDec_));
}

/**
 * @tc.name: videoDecoder_normalcase_01
 * @tc.desc: video normal case
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_01, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));

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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_normalcase_02
 * @tc.desc: video normal case  surface
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_02, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));

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
        if (g_outFrameCount == 5) {
            OH_AVFormat *format = OH_AVFormat_Create();
            EXPECT_NE(nullptr, format);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, 0);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_ROTATION, 0);
            OH_AVFormat_SetIntValue(format, OH_MD_KEY_SCALING_MODE, 0);
            EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_SetParameter(videoDec_, format));
        }
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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_normalcase_03
 * @tc.desc: video normal case
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_03, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));

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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_normalcase_04
 * @tc.desc: video normal case
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_04, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));

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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_normalcase_05
 * @tc.desc: video normal case
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_05, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));

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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_normalcase_06
 * @tc.desc: video normal case
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_06, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));

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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_normalcase_formatchange
 * @tc.desc: video format change surface
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_normalcase_formatchange, TestSize.Level1)
{
    VDecDemo();
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), 1920);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), 1080);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));

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
    DelVDecDemo();
}

/**
 * @tc.name: videoDecoder_abnormalcase_01
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_01, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
}

/**
 * @tc.name: videoDecoder_abnormalcase_02
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_02, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
}

/**
 * @tc.name: videoDecoder_abnormalcase_03
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_03, TestSize.Level1)
{
    EXPECT_EQ(nullptr, videoDec_);
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

/**
 * @tc.name: videoDecoder_abnormalcase_04
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

/**
 * @tc.name: videoDecoder_abnormalcase_05
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
}

/**
 * @tc.name: videoDecoder_abnormalcase_06
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_06, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
}

/**
 * @tc.name: videoDecoder_abnormalcase_07
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_07, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Stop(videoDec_));
}

/**
 * @tc.name: videoDecoder_abnormalcase_08
 * @tc.desc: video abnormalcase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_abnormalcase_08, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Flush(videoDec_));
}

/**
 * @tc.name: videoDecoder_statuscase_01
 * @tc.desc: video statuscase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_statuscase_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Reset(videoDec_));
}

/**
 * @tc.name: videoDecoder_statuscase_02
 * @tc.desc: video statuscase
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_statuscase_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
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

/**
 * @tc.name: videoDecoder_RegisterCallback_01
 * @tc.desc: video RegisterCallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_RegisterCallback_01, TestSize.Level1)
{
    videoDec_ = OH_VideoDecoder_CreateByName(AVCodecCodecName::VIDEO_DECODER_MJPEG_NAME.data());
    EXPECT_NE(nullptr, videoDec_);
    cb_ = {nullptr, nullptr, nullptr, nullptr};
    signal_ = new VDecSignal();
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec_, cb_, signal_));
}

/**
 * @tc.name: videoDecoder_PushInputBuffer_01
 * @tc.desc: video PushInputBuffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_PushInputBuffer_01, TestSize.Level1)
{
    OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
    int32_t bufferSize = 13571;
    inputFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(inputFile_ != nullptr, "Fatal: No memory");

    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));

    isRunning_.store(true);
    inputFile_->open(inputFilePath, std::ios::in | std::ios::binary);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
    uint32_t index = signal_->inQueue_.front();
    auto buffer = signal_->inBufferQueue_.front();
    info.size = bufferSize;
    char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * info.size + 1));
    (void)inputFile_->read(fileBuffer, info.size);
    memcpy_s(OH_AVBuffer_GetAddr(buffer), OH_AVBuffer_GetCapacity(buffer), fileBuffer, info.size);
    free(fileBuffer);
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_PushInputBuffer(videoDec_, index));
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();

    index = 0;
    buffer = signal_->inBufferQueue_.front();
    info.size = 0;
    info.flags = AVCODEC_BUFFER_FLAGS_EOS;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_PushInputBuffer(videoDec_, index));
    inputFile_->close();
    isRunning_.store(false);
}

/**
 * @tc.name: videoDecoder_PushInputBuffer_01
 * @tc.desc: video PushInputBuffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_PushInputBuffer_02, TestSize.Level1)
{
    OH_AVCodecBufferAttr info = {0, 0, 0, AVCODEC_BUFFER_FLAGS_EOS};
    int32_t bufferSize = 13571;
    inputFile_ = std::make_unique<std::ifstream>();
    UNITTEST_CHECK_AND_RETURN_LOG(inputFile_ != nullptr, "Fatal: No memory");

    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));

    isRunning_.store(true);
    inputFile_->open(inputFilePath, std::ios::in | std::ios::binary);
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });
    uint32_t index = 1024;
    auto buffer = signal_->inBufferQueue_.front();
    info.size = bufferSize;
    char *fileBuffer = static_cast<char *>(malloc(sizeof(char) * info.size + 1));
    (void)inputFile_->read(fileBuffer, info.size);
    memcpy_s(OH_AVBuffer_GetAddr(buffer), OH_AVBuffer_GetCapacity(buffer), fileBuffer, info.size);
    free(fileBuffer);
    info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_PushInputBuffer(videoDec_, index));
    signal_->inQueue_.pop();
    signal_->inBufferQueue_.pop();

    inputFile_->close();
    isRunning_.store(false);
}

/**
 * @tc.name: videoDecoder_getOutputBuffer_01
 * @tc.desc: video getOutputBuffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoCodeCapiDecoderUnitTest, videoDecoder_getOutputBuffer_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_WIDTH.data(), DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_HEIGHT.data(), DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_PIXEL_FORMAT.data(),
                            static_cast<int32_t>(VideoPixelFormat::NV12));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Configure(videoDec_, format_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_VideoDecoder_Start(videoDec_));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputBuffer(videoDec_, 0));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputBuffer(videoDec_, -1));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_FreeOutputBuffer(videoDec_, 1024));
    surface_ = GetSurface();
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputBuffer(videoDec_, 0));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputBuffer(videoDec_, -1));
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_RenderOutputBuffer(videoDec_, 1024));
}
} // namespace MediaAVCodec
} // namespace OHOS