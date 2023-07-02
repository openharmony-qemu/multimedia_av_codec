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

#include <vector>
#include <mutex>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <queue>
#include <string>
#include <thread>
#include "avcodec_codec_name.h"
#include "avcodec_mime_type.h"
#include "avcodec_common.h"
#include "media_description.h"
#include "native_avformat.h"
#include "avcodec_errors.h"
#include "native_avcodec_audioencoder.h"
#include "securec.h"
#include "ffmpeg_converter.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
const string CODEC_FLAC_NAME = std::string(AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME);
const string CODEC_AAC_NAME = std::string(AVCodecCodecName::AUDIO_ENCODER_AAC_NAME);
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t BITS_RATE = 261000;
constexpr int32_t BITS_PER_CODED_SAMPLE = AudioSampleFormat::SAMPLE_S16LE;
constexpr int32_t ABNORMAL_BITS_PER_CODED_SAMPLE = AudioSampleFormat::INVALID_WIDTH;
constexpr uint32_t FRAME_DURATION_US = 33000;
constexpr uint64_t CHANNEL_LAYOUT = AudioChannelLayout::STEREO;
constexpr uint64_t ABNORMAL_CHANNEL_LAYOUT = AudioChannelLayout::CH_10POINT2;
constexpr int32_t SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16LE;
constexpr uint32_t FRAME_BYTES = 18432;
constexpr int32_t COMPLIANCE_LEVEL = 0;

constexpr string_view INPUT_FILE_PATH = "/data/test/media/flac_2c_44100hz_261k.pcm";
constexpr string_view OUTPUT_FILE_PATH = "/data/test/media/encoderTest.flac";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
class AEncSignal {
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

class BufferCallback : public AVCodecCallback {
public:
    explicit BufferCallback(AEncSignal *userData) : userData_(userData) {}
    virtual ~BufferCallback() = default;
    AEncSignal *userData_;
    void OnError(AVCodecErrorType errorType, int32_t errorCode) override;
    void OnOutputFormatChanged(const Format &format) override;
    void OnInputBufferAvailable(uint32_t index) override;
    void OnOutputBufferAvailable(uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag) override;
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
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                                    void *userData)
{
    (void)codec;
    AEncSignal *signal = static_cast<AEncSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    if (attr) {
        signal->attrQueue_.push(*attr);
    } else {
        cout << "OnOutputBufferAvailable error, attr is nullptr!" << endl;
    }
    signal->outCond_.notify_all();
}

class AudioCodeCapiEncoderUnitTest : public testing::Test {
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
    std::unique_ptr<std::ifstream> inputFile_ = std::make_unique<std::ifstream>(INPUT_FILE_PATH, std::ios::binary);
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    int32_t index_;

    struct OH_AVCodecAsyncCallback cb_;
    AEncSignal *signal_;
    OH_AVCodec *audioEnc_;
    OH_AVFormat *format;
    bool isFirstFrame_ = true;
    int64_t timeStamp_ = 0;
    uint32_t frameCount_ = 0;
};

void AudioCodeCapiEncoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioCodeCapiEncoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioCodeCapiEncoderUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioCodeCapiEncoderUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

void AudioCodeCapiEncoderUnitTest::InputFunc()
{
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
        if (!inputFile_->eof()) {
            inputFile_->read((char *)OH_AVMemory_GetAddr(buffer), FRAME_BYTES);
        } else {
            OH_AVCodecBufferAttr info;
            info.size = 0;
            info.offset = 0;
            info.pts = 0;
            info.flags = AVCODEC_BUFFER_FLAGS_EOS;
            OH_AudioEncoder_PushInputData(audioEnc_, index, info);
            signal_->inQueue_.pop();
            signal_->inBufferQueue_.pop();
            break;
        }
        OH_AVCodecBufferAttr info;
        info.size = FRAME_BYTES;
        info.offset = 0;

        int32_t ret = AVCS_ERR_OK;
        if (isFirstFrame_) {
            info.flags = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
            isFirstFrame_ = false;
        } else {
            info.flags = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioEncoder_PushInputData(audioEnc_, index, info);
        }

        timeStamp_ += FRAME_DURATION_US;
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            isRunning_ = false;
            break;
        }
    }
    inputFile_->close();
}

void AudioCodeCapiEncoderUnitTest::OutputFunc()
{
    std::ofstream outputFile;
    outputFile.open(OUTPUT_FILE_PATH.data(), std::ios::out | std::ios::binary);

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
        if (data != nullptr) {
            outputFile.write(reinterpret_cast<char *>(OH_AVMemory_GetAddr(data)), attr.size);
        }
        if (attr.flags == AVCODEC_BUFFER_FLAGS_EOS || attr.size == 0) {
            cout << "encode eos" << endl;
            isRunning_.store(false);
        }

        signal_->outBufferQueue_.pop();
        signal_->attrQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioEncoder_FreeOutputData(audioEnc_, index) != AV_ERR_OK) {
            cout << "Fatal: FreeOutputData fail" << endl;
            break;
        }
    }
    outputFile.close();
}

int32_t AudioCodeCapiEncoderUnitTest::ProceFunc(void)
{
    audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME).data());
    EXPECT_NE((OH_AVCodec *)nullptr, audioEnc_);

    signal_ = new AEncSignal();
    EXPECT_NE(nullptr, signal);

    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_SetCallback(audioEnc_, cb_, signal_));

    format = OH_AVFormat_Create();
    return AVCS_ERR_OK;
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_CreateByName_01, TestSize.Level1)
{
    audioEnc_ = OH_AudioEncoder_CreateByName((AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME).data());
    EXPECT_NE(nullptr, audioEnc_);
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_CreateByMime_01, TestSize.Level1)
{
    audioEnc_ = OH_AudioEncoder_CreateByMime((AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC).data());
    EXPECT_NE(nullptr, audioEnc_);
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_Prepare_01, TestSize.Level1)
{
    ProceFunc();
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Prepare(audioEnc_));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_GetOutputDescription_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    EXPECT_NE(nullptr, OH_AudioEncoder_GetOutputDescription(audioEnc_));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_IsValid_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    bool value = true;
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_IsValid(audioEnc_, &value));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_SetParameter_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Start(audioEnc_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_SetParameter(audioEnc_, format));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_SetParameter_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    EXPECT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_SetParameter(audioEnc_, format));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_Configure_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_Configure_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), ABNORMAL_CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);
    
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_Configure_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), ABNORMAL_BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);
    
    ASSERT_NE(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_normalcase_01, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Start(audioEnc_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
    }
    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Destroy(audioEnc_));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_normalcase_02, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);
    outputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Start(audioEnc_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Stop(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Destroy(audioEnc_));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_normalcase_03, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Start(audioEnc_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Destroy(audioEnc_));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_normalcase_04, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);

    outputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Start(audioEnc_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }
    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Reset(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Destroy(audioEnc_));
}

HWTEST_F(AudioCodeCapiEncoderUnitTest, audioEncoder_normalcase_05, TestSize.Level1)
{
    ProceFunc();
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), SAMPLE_RATE);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_BITRATE.data(), BITS_RATE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_BITS_PER_CODED_SAMPLE.data(), BITS_PER_CODED_SAMPLE);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AUDIO_SAMPLE_FORMAT.data(), SAMPLE_FORMAT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(), CHANNEL_LAYOUT);
    OH_AVFormat_SetLongValue(format, MediaDescriptionKey::MD_KEY_COMPLIANCE_LEVEL.data(), COMPLIANCE_LEVEL);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Configure(audioEnc_, format));
    isRunning_.store(true);

    inputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::InputFunc, this);
    EXPECT_NE(nullptr, inputLoop_);
    outputLoop_ = make_unique<thread>(&AudioCodeCapiEncoderUnitTest::OutputFunc, this);
    EXPECT_NE(nullptr, outputLoop_);

    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Start(audioEnc_));
    while (isRunning_.load()) {
        sleep(1); // sleep 1s
    }

    isRunning_.store(false);
    if (inputLoop_ != nullptr && inputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        inputLoop_->join();
    }

    if (outputLoop_ != nullptr && outputLoop_->joinable()) {
        {
            unique_lock<mutex> lock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        outputLoop_->join();
    }
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Flush(audioEnc_));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, OH_AudioEncoder_Destroy(audioEnc_));
}
} // namespace MediaAVCodec
} // namespace OHOS
