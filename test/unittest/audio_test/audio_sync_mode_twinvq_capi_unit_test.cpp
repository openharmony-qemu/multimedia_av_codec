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
#include <cstdint>
#include <fcntl.h>
#include <fstream>
#include <gtest/gtest.h>
#include <iostream>
#include <libavutil/samplefmt.h>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <unistd.h>
#include "avcodec_audio_common.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_mime_type.h"
#include "common/native_mfmagic.h"
#include "common/status.h"
#include "media_description.h"
#include "native_audio_channel_layout.h"
#include "native_avbuffer.h"
#include "native_avcapability.h"
#include "native_avcodec_audiocodec.h"
#include "native_avcodec_audiodecoder.h"
#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr size_t FIELDSIZE = 8;
constexpr int32_t DEFAULT_CHANNELS = 1;
constexpr uint32_t DEFAULT_SAMPLE_RATE = 8000;
constexpr string_view TWINVQ_FILE_TODEMUX = "/data/test/media/twinvq_8000_mono.dat";
constexpr string_view OUTPUT_TWINVQ_PCM_FILE_PATH = "/data/test/media/twinvq_8000_stereo.pcm";
} // namespace

namespace OHOS {
namespace MediaAVCodec {
class AudioCodecBufferSignal {
public:
    std::mutex inMutex_;
    std::mutex outMutex_;
    std::mutex startMutex_;
    std::condition_variable inCond_;
    std::condition_variable outCond_;
    std::condition_variable startCond_;
    std::queue<uint32_t> inQueue_;
    std::queue<uint32_t> outQueue_;
    std::queue<OH_AVBuffer *> inBufferQueue_;
    std::queue<OH_AVBuffer *> outBufferQueue_;
    std::atomic<uint64_t> generation_ {0};
    std::queue<uint64_t, std::deque<uint64_t>> inGenQueue_;
    std::queue<uint64_t, std::deque<uint64_t>> outGenQueue_;
};

static int32_t g_outputSampleRate = 0;
static int32_t g_outputChannels = 0;
static int64_t g_outputChannelLayout = 0;
static int32_t g_outputSampleFormat = 0;

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
    (void)userData;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUD_CHANNEL_COUNT, &g_outputChannels);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUD_SAMPLE_RATE, &g_outputSampleRate);
    OH_AVFormat_GetLongValue(format, OH_MD_KEY_CHANNEL_LAYOUT, &g_outputChannelLayout);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &g_outputSampleFormat);
    cout << "OnOutputFormatChanged received, rate:" << g_outputSampleRate << ",channel:" << g_outputChannels << endl;
    cout << "OnOutputFormatChanged received, layout:" << g_outputChannelLayout << ",format:" << g_outputSampleFormat
         << endl;
}

static void OnInputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    AudioCodecBufferSignal *signal = static_cast<AudioCodecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inGenQueue_.push(signal->generation_.load(std::memory_order_relaxed));
    signal->inCond_.notify_all();
}

static void OnOutputBufferAvailable(OH_AVCodec *codec, uint32_t index, OH_AVBuffer *data, void *userData)
{
    (void)codec;
    AudioCodecBufferSignal *signal = static_cast<AudioCodecBufferSignal *>(userData);
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outQueue_.push(index);
    signal->outBufferQueue_.push(data);
    signal->outGenQueue_.push(signal->generation_.load(std::memory_order_relaxed));
    signal->outCond_.notify_all();
}

class AudioSyncModeCapiUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
    int32_t InitFile(string_view input, string_view output);
    void InputFunc();
    bool DecoderFillInputBuffer(OH_AVBuffer *buffer);
    void DecoderRun();
    void OutputFunc();
    bool ReadBuffer(OH_AVBuffer *buffer, uint32_t index);
    int32_t CreateCodecFunc();
    int32_t Configure();
    int32_t Start();
    int32_t Stop();
    int32_t Reset();
    void Release();
    void HandleInputEOS(const uint32_t index);
    int64_t GetFileSize(const char *fileName);
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    void CleanUp();
    void StopLoops();

protected:
    int fd_ = -1;
    bool isTestingFormat_ = false;
    bool isSync_ = false;
    std::atomic<bool> isRunning_ = false;
    uint64_t loopIndex_ = 0;
    std::unique_ptr<std::thread> inputLoop_;
    std::unique_ptr<std::thread> outputLoop_;
    struct OH_AVCodecCallback cb_;
    AudioCodecBufferSignal *signal_ = nullptr;
    OH_AVCodec *audioDec_ = nullptr;
    OH_AVFormat *format_ = nullptr;
    bool isFirstFrame_ = true;
    uint32_t frameCount_ = 0;
    uint32_t decodeInputFrameCnt_ = 0;
    uint32_t outputFormatChangedTimes = 0;
    uint32_t outputFrameCnt_ = 0;
    std::ifstream infile;
    int64_t inTimeout_ = 20000; // 20000us: 20ms
    int64_t outTimeout_ = 20000; // 20000us: 20ms
    std::unique_ptr<std::ifstream> soFile_;
    std::ofstream pcmOutputFile_;
    OH_AVDemuxer *demuxer = nullptr;
    OH_AVSource *source = nullptr;
    int32_t outputChannels = 0;
    int32_t outputSampleRate = 0;
    std::vector<uint8_t> rawData_;
    size_t currentDataPos_ = 0;
    bool headerParsed_ = false;
    std::atomic<bool> gotEos_ {false};
    size_t staleInputDiscarded_ = 0;
    size_t staleOutputDiscarded_ = 0;
};

void AudioSyncModeCapiUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioSyncModeCapiUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioSyncModeCapiUnitTest::SetUp(void)
{
    g_outputSampleRate = 0;
    g_outputChannels = 0;
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioSyncModeCapiUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
    CleanUp();
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
    if (pcmOutputFile_.is_open()) {
        pcmOutputFile_.close();
    }
    if (format_ != nullptr) {
        OH_AVFormat_Destroy(format_);
        format_ = nullptr;
    }
}

void AudioSyncModeCapiUnitTest::Release()
{
    Stop();
    OH_AudioCodec_Destroy(audioDec_);
    audioDec_ = nullptr;
    CleanUp();
}

void AudioSyncModeCapiUnitTest::SetEOS(uint32_t index, OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    OH_AVBuffer_SetBufferAttr(buffer, &attr);
    int32_t res = OH_AudioCodec_PushInputBuffer(audioDec_, index);
    if (res != AV_ERR_OK) {
        cout << "Fatal: PushInputBuffer EOS fail, ret:" << res << endl;
    }
}

void AudioSyncModeCapiUnitTest::HandleInputEOS(const uint32_t index)
{
    std::cout << "end buffer\n";
    OH_AudioCodec_PushInputBuffer(audioDec_, index);
    signal_->inBufferQueue_.pop();
    signal_->inQueue_.pop();
}

bool AudioSyncModeCapiUnitTest::ReadBuffer(OH_AVBuffer *buffer, uint32_t index)
{
    constexpr size_t fileHeaderSize = 12;
    if (!headerParsed_) {
        if (rawData_.size() < fileHeaderSize) {
            cout << "Error: file too small for header" << endl;
            HandleInputEOS(index);
            return false;
        }
        uint32_t extraDataLen = 0;
        memcpy_s(&extraDataLen, sizeof(extraDataLen), rawData_.data() + FIELDSIZE, sizeof(extraDataLen));
        const size_t totalHeader = fileHeaderSize + extraDataLen;
        if (rawData_.size() < totalHeader) {
            HandleInputEOS(index);
            return false;
        }
        currentDataPos_ = totalHeader;
        headerParsed_ = true;
    }
    constexpr size_t frameHeaderSize = 16;
    if (currentDataPos_ + frameHeaderSize > rawData_.size()) {
        buffer->buffer_->memory_->SetSize(1);
        buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
        HandleInputEOS(index);
        return false;
    }
    uint64_t frameSize = 0;
    int64_t pts = 0;
    memcpy_s(&frameSize, sizeof(frameSize), rawData_.data() + currentDataPos_, sizeof(frameSize));
    memcpy_s(&pts, sizeof(pts), rawData_.data() + currentDataPos_ + FIELDSIZE, sizeof(pts));
    currentDataPos_ += frameHeaderSize;
    if (currentDataPos_ + frameSize > rawData_.size()) {
        buffer->buffer_->memory_->SetSize(1);
        buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
        HandleInputEOS(index);
        return false;
    }
    void *addr = OH_AVBuffer_GetAddr(buffer);
    errno_t ret = memcpy_s(addr, buffer->buffer_->memory_->GetCapacity(), rawData_.data() + currentDataPos_, frameSize);
    if (ret != EOK) {
        HandleInputEOS(index);
        return false;
    }
    buffer->buffer_->memory_->SetSize(frameSize);
    buffer->buffer_->pts_ = pts;
    buffer->buffer_->flag_ = isFirstFrame_ ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAGS_NONE;
    currentDataPos_ += frameSize;
    isFirstFrame_ = false;
    return true;
}

void AudioSyncModeCapiUnitTest::InputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return (signal_->inQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "[InputFunc]   isRunning false → break" << endl;
            break;
        }

        uint32_t index = signal_->inQueue_.front();
        auto buffer = signal_->inBufferQueue_.front();
        if (buffer == nullptr) {
            cout << "[InputFunc]   buffer null → break" << endl;
            break;
        }
        if (ReadBuffer(buffer, index) == false) {
            cout << "[InputFunc]   ReadBuffer return false → break" << endl;
            break;
        }

        int32_t ret;
        if (isFirstFrame_) {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_CODEC_DATA;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
            isFirstFrame_ = false;
        } else {
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
            ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
        }
        signal_->inQueue_.pop();
        signal_->inBufferQueue_.pop();
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            cout << "[InputFunc]   PushInputBuffer fail " << ret << " → break" << endl;
            break;
        }
    }
    cout << "[InputFunc]   thread exit" << endl;
    infile.close();
}

void AudioSyncModeCapiUnitTest::OutputFunc()
{
    if (!pcmOutputFile_.is_open()) {
        cout << "open " << OUTPUT_TWINVQ_PCM_FILE_PATH << " failed!" << std::endl;
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return (signal_->outQueue_.size() > 0 || !isRunning_.load()); });

        if (!isRunning_.load()) {
            cout << "[OutputFunc]  isRunning false → break" << endl;
            break;
        }

        uint32_t index = signal_->outQueue_.front();
        OH_AVBuffer *data = signal_->outBufferQueue_.front();

        if (data == nullptr) {
            cout << "[OutputFunc]  data null → skip" << endl;
            continue;
        }
        size_t outSize = data->buffer_->memory_->GetSize();
        pcmOutputFile_.write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(data)), outSize);

        bool eos = (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS || outSize == 0);
        cout << "[OutputFunc]  index=" << index
             << "  size=" << outSize
             << "  eos=" << eos << endl;

        signal_->outBufferQueue_.pop();
        signal_->outQueue_.pop();
        if (OH_AudioCodec_FreeOutputBuffer(audioDec_, index) != AV_ERR_OK) {
            cout << "[OutputFunc]  FreeOutputBuffer fail → break" << endl;
            break;
        }
        if (eos) {
            isRunning_.store(false);
            signal_->startCond_.notify_all();
            cout << "[OutputFunc]  EOS → stop" << endl;
            break;
        }
    }
    cout << "[OutputFunc]  thread exit" << endl;
    pcmOutputFile_.close();
}

int32_t AudioSyncModeCapiUnitTest::Start()
{
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioSyncModeCapiUnitTest::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&AudioSyncModeCapiUnitTest::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AudioCodec_Start(audioDec_);
}

int32_t AudioSyncModeCapiUnitTest::Stop()
{
    isRunning_.store(false);
    StopLoops();
    if (signal_) {
        signal_->generation_.fetch_add(1, std::memory_order_relaxed);
    }
    int32_t ret = OH_AudioCodec_Stop(audioDec_);
    CleanUp();
    return ret;
}

void AudioSyncModeCapiUnitTest::StopLoops()
{
    if (signal_ != nullptr) {
        {
            unique_lock<mutex> inLock(signal_->inMutex_);
            signal_->inCond_.notify_all();
        }
        {
            unique_lock<mutex> outLock(signal_->outMutex_);
            signal_->outCond_.notify_all();
        }
        {
            unique_lock<mutex> startLock(signal_->startMutex_);
            signal_->startCond_.notify_all();
        }
    }
    if (inputLoop_ != nullptr) {
        if (inputLoop_->joinable()) {
            inputLoop_->join();
        }
        inputLoop_.reset();
    }
    if (outputLoop_ != nullptr) {
        if (outputLoop_->joinable()) {
            outputLoop_->join();
        }
        outputLoop_.reset();
    }
    if (signal_ != nullptr) {
        while (!signal_->inQueue_.empty()) {
            signal_->inQueue_.pop();
        }
        while (!signal_->outQueue_.empty()) {
            signal_->outQueue_.pop();
        }
        while (!signal_->inBufferQueue_.empty()) {
            signal_->inBufferQueue_.pop();
        }
        while (!signal_->outBufferQueue_.empty()) {
            signal_->outBufferQueue_.pop();
        }
        while (!signal_->inGenQueue_.empty()) {
            signal_->inGenQueue_.pop();
        }
        while (!signal_->outGenQueue_.empty()) {
            signal_->outGenQueue_.pop();
        }
    }
}

int32_t AudioSyncModeCapiUnitTest::InitFile(string_view inputFilename, string_view outputFilename)
{
    pcmOutputFile_.open(outputFilename.data(), std::ios::out | std::ios::binary);
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed: " << outputFilename << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    infile.open(inputFilename.data(), std::ios::binary);
    if (!infile.is_open()) {
        cout << "Fatal: open input file failed: " << inputFilename << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }

    infile.seekg(0, std::ios::end);
    size_t fileSize = infile.tellg();
    infile.seekg(0, std::ios::beg);
    rawData_.resize(fileSize);
    infile.read(reinterpret_cast<char*>(rawData_.data()), fileSize);
    infile.close();
    if (infile.gcount() != static_cast<std::streamsize>(fileSize)) {
        cout << "Error: Failed to read full input file" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    currentDataPos_ = 0;
    headerParsed_ = false;
    isFirstFrame_ = true;
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioSyncModeCapiUnitTest::CreateCodecFunc()
{
    audioDec_ = OH_AudioCodec_CreateByName((std::string(AVCodecCodecName::AUDIO_DECODER_TWINVQ_NAME)).data());
    if (audioDec_ == nullptr) {
        cout << "Fatal: CreateByName fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    signal_ = new AudioCodecBufferSignal();
    if (signal_ == nullptr) {
        cout << "Fatal: create signal fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    cb_ = {&OnError, &OnOutputFormatChanged, &OnInputBufferAvailable, &OnOutputBufferAvailable};
    int32_t ret = OH_AudioCodec_RegisterCallback(audioDec_, cb_, signal_);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: SetCallback fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioSyncModeCapiUnitTest::Configure()
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    uint32_t bitRate = 8000;
    OH_AVFormat_SetIntValue(format_, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_BITRATE.data(), bitRate);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), DEFAULT_CHANNELS);
    OH_AVFormat_SetLongValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_LAYOUT.data(),
        OH_AudioChannelLayout::CH_LAYOUT_MONO);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), DEFAULT_SAMPLE_RATE);
    static constexpr size_t minHeaderSize{12};
    if (rawData_.size() < minHeaderSize) {
        std::cout << "Fatal: file too small for header" << std::endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    uint32_t extraDataSize{};
    if (memcpy_s(&extraDataSize, sizeof(extraDataSize),
                 rawData_.data() + FIELDSIZE, sizeof(extraDataSize)) != EOK) {
        std::cout << "Fatal: memcpy_s extraDataSize failed" << std::endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    if (extraDataSize > 0) {
        if (rawData_.size() < minHeaderSize + extraDataSize) {
            std::cout << "Fatal: file too small for extra data" << std::endl;
            return OH_AVErrCode::AV_ERR_UNKNOWN;
        }
        std::vector<uint8_t> extraData(extraDataSize);
        if (memcpy_s(extraData.data(), extraData.size(),
                     rawData_.data() + minHeaderSize, extraDataSize) != EOK) {
            std::cout << "Fatal: memcpy_s extraData failed" << std::endl;
            return OH_AVErrCode::AV_ERR_UNKNOWN;
        }
        OH_AVFormat_SetBuffer(format_, OH_MD_KEY_CODEC_CONFIG,
                              extraData.data(), static_cast<int32_t>(extraData.size()));
    }
    return OH_AudioCodec_Configure(audioDec_, format_);
}

int32_t AudioSyncModeCapiUnitTest::Reset()
{
    isRunning_.store(false);
    StopLoops();
    if (signal_) {
        signal_->generation_.fetch_add(1, std::memory_order_relaxed);
    }
    CleanUp();
    return OH_AudioCodec_Reset(audioDec_);
}

void AudioSyncModeCapiUnitTest::CleanUp()
{
    if (pcmOutputFile_.is_open()) {
        pcmOutputFile_.close();
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (fd_ != -1) {
        close(fd_);
        fd_ = -1;
    }
}

bool AudioSyncModeCapiUnitTest::DecoderFillInputBuffer(OH_AVBuffer *buffer)
{
    OH_AVCodecBufferAttr attr;
    memset_s(&attr, sizeof(attr), 0, sizeof(attr));
    bool finish = true;
    uint32_t index = 0;

    if (ReadBuffer(buffer, index)) {
        OH_AVBuffer_GetBufferAttr(buffer, &attr);
        if (attr.size > 0) {
            attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
            decodeInputFrameCnt_++;
            finish = false;
        }
    } else {
        attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    }

    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(buffer, &attr), AV_ERR_OK);
    return finish;
}

void AudioSyncModeCapiUnitTest::DecoderRun()
{
    ASSERT_EQ(OH_AudioCodec_Prepare(audioDec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(audioDec_), AV_ERR_OK);
    bool inputEnd = false;
    OH_AVErrCode ret = AV_ERR_OK;
    while (isRunning_) {
        uint32_t index = 0;
        if (!inputEnd) {
            ret = OH_AudioCodec_QueryInputBuffer(audioDec_, &index, inTimeout_);
            if (ret == AV_ERR_TRY_AGAIN_LATER) {
                continue;
            }
            ASSERT_EQ(ret, AV_ERR_OK);
            OH_AVBuffer *inputBuf = OH_AudioCodec_GetInputBuffer(audioDec_, index);
            ASSERT_NE(inputBuf, nullptr);

            inputEnd = DecoderFillInputBuffer(inputBuf);
            EXPECT_EQ(OH_AudioCodec_PushInputBuffer(audioDec_, index), AV_ERR_OK);
        }

        ret = OH_AudioCodec_QueryOutputBuffer(audioDec_, &index, outTimeout_);
        if (ret == AV_ERR_TRY_AGAIN_LATER) {
            outputFrameCnt_++;
            continue;
        } else if (ret == AV_ERR_STREAM_CHANGED) {
            OH_AVFormat *outFormat = OH_AudioCodec_GetOutputDescription(audioDec_);
            outputFormatChangedTimes++;
            OH_AVFormat_GetIntValue(outFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &outputChannels);
            OH_AVFormat_GetIntValue(outFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &outputSampleRate);
            continue;
        }
        EXPECT_EQ(ret, AV_ERR_OK);
        OH_AVBuffer *outputBuf = OH_AudioCodec_GetOutputBuffer(audioDec_, index);
        ASSERT_NE(outputBuf, nullptr);
        OH_AVCodecBufferAttr attr;
        memset_s(&attr, sizeof(attr), 0, sizeof(attr));
        ASSERT_EQ(OH_AVBuffer_GetBufferAttr(outputBuf, &attr), AV_ERR_OK);

        if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "output eos" << endl;
            ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(audioDec_, index), AV_ERR_OK);
            break;
        }

        outputFrameCnt_++;
        pcmOutputFile_.write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(outputBuf)), attr.size);
        ASSERT_EQ(OH_AudioCodec_FreeOutputBuffer(audioDec_, index), AV_ERR_OK);
    }
}

HWTEST_F(AudioSyncModeCapiUnitTest, twinvq_decode_01, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));

    audioDec_ = OH_AudioCodec_CreateByMime(OH_AVCODEC_MIMETYPE_AUDIO_TWINVQ, false);
    ASSERT_NE(audioDec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());

    DecoderRun();
    EXPECT_EQ(decodeInputFrameCnt_, outputFrameCnt_);
    OH_AudioCodec_Destroy(audioDec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, twinvq_InvalidSizeTest, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));

    audioDec_ = OH_AudioCodec_CreateByMime(std::string(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_TWINVQ).data(), false);
    ASSERT_NE(audioDec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_EQ(OH_AudioCodec_Prepare(audioDec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(audioDec_), AV_ERR_OK);

    OH_AVBuffer *inputBuf = nullptr;
    uint32_t index = 0;
    OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(audioDec_, &index, inTimeout_);
    ASSERT_EQ(ret, AV_ERR_OK);
    inputBuf = OH_AudioCodec_GetInputBuffer(audioDec_, index);
    ASSERT_NE(inputBuf, nullptr);

    OH_AVCodecBufferAttr attr;
    memset_s(&attr, sizeof(attr), 0, sizeof(attr));

    attr.size = -1;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, &attr), AV_ERR_INVALID_VAL);

    OH_AudioCodec_Destroy(audioDec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, twinvq_InvalidOffsetTest, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));

    audioDec_ = OH_AudioCodec_CreateByMime(std::string(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_TWINVQ).data(), false);
    ASSERT_NE(audioDec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_EQ(OH_AudioCodec_Prepare(audioDec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(audioDec_), AV_ERR_OK);

    OH_AVBuffer *inputBuf = nullptr;
    uint32_t index = 0;
    OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(audioDec_, &index, inTimeout_);
    ASSERT_EQ(ret, AV_ERR_OK);
    inputBuf = OH_AudioCodec_GetInputBuffer(audioDec_, index);
    ASSERT_NE(inputBuf, nullptr);

    OH_AVCodecBufferAttr attr;
    memset_s(&attr, sizeof(attr), 0, sizeof(attr));

    attr.offset = -1;
    attr.flags = AVCODEC_BUFFER_FLAGS_NONE;
    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, &attr), AV_ERR_INVALID_VAL);

    OH_AudioCodec_Destroy(audioDec_);
}

HWTEST_F(AudioSyncModeCapiUnitTest, twinvq_InvalidAttrTest, TestSize.Level1)
{
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(TWINVQ_FILE_TODEMUX, OUTPUT_TWINVQ_PCM_FILE_PATH));

    audioDec_ = OH_AudioCodec_CreateByMime(std::string(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_TWINVQ).data(), false);
    ASSERT_NE(audioDec_, nullptr);
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Configure());
    ASSERT_EQ(OH_AudioCodec_Prepare(audioDec_), AV_ERR_OK);
    ASSERT_EQ(OH_AudioCodec_Start(audioDec_), AV_ERR_OK);

    OH_AVBuffer *inputBuf = nullptr;
    uint32_t index = 0;
    OH_AVErrCode ret = OH_AudioCodec_QueryInputBuffer(audioDec_, &index, inTimeout_);
    ASSERT_EQ(ret, AV_ERR_OK);
    inputBuf = OH_AudioCodec_GetInputBuffer(audioDec_, index);
    ASSERT_NE(inputBuf, nullptr);

    OH_AVCodecBufferAttr *attr = nullptr;
    EXPECT_EQ(OH_AVBuffer_SetBufferAttr(inputBuf, attr), AV_ERR_INVALID_VAL);

    OH_AudioCodec_Destroy(audioDec_);
}
} // namespace MediaAVCodec
} // namespace OHOS