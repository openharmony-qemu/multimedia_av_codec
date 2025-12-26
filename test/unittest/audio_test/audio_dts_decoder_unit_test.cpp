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

#include <queue>
#include <mutex>
#include <libavutil/samplefmt.h>
#include <gtest/gtest.h>
#include <iostream>
#include <unistd.h>
#include <atomic>
#include <fstream>
#include <string>
#include <thread>
#include <fcntl.h>
#include "plugin/codec_plugin.h"
#include "plugin/plugin_manager_v2.h"
#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "avcodec_errors.h"
#include "avcodec_mime_type.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "securec.h"
#include "avcodec_audio_common.h"
#include "native_avbuffer.h"
#include "common/native_mfmagic.h"
#include "native_avcodec_audiocodec.h"
#include "native_audio_channel_layout.h"
#include "media_key_system_mock.h"
#include "native_avcapability.h"
#include "native_avdemuxer.h"
#include "native_avsource.h"
#include "common/status.h"
#include "native_avcodec_audiodecoder.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

namespace {
const string CODEC_DTS_NAME = std::string(std::string(AVCodecCodecName::AUDIO_DECODER_DTS_NAME));
constexpr int32_t FRAME_COUNT = 5;
constexpr string_view DTS_FILE_TODEMUX = "/data/test/media/dts_48000_mono.mp4";
constexpr string_view OUTPUT_DTS_PCM_FILE_PATH = "/data/test/media/test_decoder_dts.pcm";
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
constexpr int SAMPLERATE = 8000;
constexpr int SAMPLES_PER_FRAME = 160;

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

class AudioCodeDtsDecoderUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;
    int32_t InitFile(string_view input);
    int32_t InitFile(string_view input, string_view output);
    void InputFunc();
    void OutputFunc();
    bool ReadBuffer(OH_AVBuffer *buffer, uint32_t index);
    int32_t CreateCodecFunc();
    void HandleEOS(const uint32_t &index);
    int32_t Configure();
    int32_t Start();
    int32_t Stop();
    int32_t Reset();
    void Release();
    int64_t GetFileSize(const char *fileName);
    void SetEOS(uint32_t index, OH_AVBuffer *buffer);
    void CleanUp();
    void StopLoops();
    shared_ptr<CodecPlugin> CreatePlugin();
    shared_ptr<AVBuffer> CreateAVBuffer(size_t capacity, size_t size);

    void OnInputBufferDone(const shared_ptr<Media::AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
    }
    void OnOutputBufferDone(const shared_ptr<Media::AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
    }
    void OnEvent(const shared_ptr<Plugins::PluginEvent> event) override
    {
        (void)event;
    }

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
    std::unique_ptr<std::ifstream> soFile_;
    std::ofstream pcmOutputFile_;
    OH_AVDemuxer *demuxer = nullptr;
    OH_AVSource *source = nullptr;
    // Diagnostics
    std::atomic<bool> gotEos_ {false};
    size_t staleInputDiscarded_ = 0;
    size_t staleOutputDiscarded_ = 0;
    shared_ptr<CodecPlugin> plugin_ = nullptr;
};

void AudioCodeDtsDecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AudioCodeDtsDecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AudioCodeDtsDecoderUnitTest::SetUp(void)
{
    g_outputSampleRate = 0;
    g_outputChannels = 0;
    plugin_ = CreatePlugin();
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AudioCodeDtsDecoderUnitTest::TearDown(void)
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

shared_ptr<CodecPlugin> AudioCodeDtsDecoderUnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_DTS_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<AVBuffer> AudioCodeDtsDecoderUnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    avBuffer->memory_->SetSize(size);
    return avBuffer;
}

void AudioCodeDtsDecoderUnitTest::Release()
{
    Stop();
    OH_AudioCodec_Destroy(audioDec_);
    audioDec_ = nullptr;
    CleanUp();
}

void AudioCodeDtsDecoderUnitTest::HandleEOS(const uint32_t &index)
{
    OH_AudioCodec_PushInputBuffer(audioDec_, index);
    if (!signal_->inQueue_.empty()) {
        signal_->inQueue_.pop();
    }
    if (!signal_->inBufferQueue_.empty()) {
        signal_->inBufferQueue_.pop();
    }
    if (!signal_->inGenQueue_.empty()) {
        signal_->inGenQueue_.pop();
    }
    gotEos_.store(true, std::memory_order_relaxed);
}

void AudioCodeDtsDecoderUnitTest::SetEOS(uint32_t index, OH_AVBuffer *buffer)
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

bool AudioCodeDtsDecoderUnitTest::ReadBuffer(OH_AVBuffer *buffer, uint32_t index)
{
    if (demuxer == nullptr || buffer == nullptr) {
        cout << "Fatal: demuxer or buffer is null loop=" << loopIndex_ << " frame=" << frameCount_ << endl;
        return false;
    }
    if (buffer->magic_ != MFMagic::MFMAGIC_AVBUFFER) {
        cout << "Fatal: buffer magic invalid before demux read loop=" << loopIndex_ << " frame=" << frameCount_
             << " magic=" << static_cast<uint64_t>(buffer->magic_) << endl;
        return false;
    }
    OH_AVCodecBufferAttr attr;
    OH_AVErrCode readRet = OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, buffer);
    if (readRet != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: OH_AVDemuxer_ReadSampleBuffer fail loop=" << loopIndex_ << " frame=" << frameCount_
             << " ret=" << readRet << " addr=" << (void *)buffer << " magic=" << static_cast<uint64_t>(buffer->magic_)
             << endl;
        return false;
    }
    if (OH_AVBuffer_GetBufferAttr(buffer, &attr) != AV_ERR_OK) {
        cout << "Fatal: OH_AVBuffer_GetBufferAttr fail loop=" << loopIndex_ << " frame=" << frameCount_ << endl;
        return false;
    }
    if (attr.flags & AVCODEC_BUFFER_FLAGS_EOS) {
        SetEOS(index, buffer);
        return false;
    }
    buffer->buffer_->pts_ = frameCount_ * SAMPLES_PER_FRAME * 1000000LL / SAMPLERATE;
    buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
    if (frameCount_ < FRAME_COUNT) {
        cout << "[ReadBuffer] loop=" << loopIndex_ << " frame=" << frameCount_ << " size=" << attr.size << endl;
    }
    return true;
}

int64_t AudioCodeDtsDecoderUnitTest::GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

inline void PopInputQueues(AudioCodecBufferSignal* sig, bool popGen = true)
{
    if (!sig->inQueue_.empty()) {
        sig->inQueue_.pop();
    }
    if (!sig->inBufferQueue_.empty()) {
        sig->inBufferQueue_.pop();
    }
    if (popGen && !sig->inGenQueue_.empty()) {
        sig->inGenQueue_.pop();
    }
}

void AudioCodeDtsDecoderUnitTest::InputFunc()
{
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->inMutex_);
        signal_->inCond_.wait(lock, [this]() { return !signal_->inQueue_.empty() || !isRunning_.load(); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->inQueue_.front();
        OH_AVBuffer *buffer = signal_->inBufferQueue_.empty() ? nullptr : signal_->inBufferQueue_.front();
        uint64_t bufGen = signal_->inGenQueue_.empty() ? 0 : signal_->inGenQueue_.front();
        uint64_t curGen = signal_->generation_.load();
        if (!buffer) {
            PopInputQueues(signal_);
            continue;
        }
        if (bufGen != curGen) {
            staleInputDiscarded_++;
            PopInputQueues(signal_);
            continue;
        }
        if (buffer->magic_ != MFMagic::MFMAGIC_AVBUFFER) {
            PopInputQueues(signal_);
            continue;
        }
        if (!ReadBuffer(buffer, index)) {
            buffer->buffer_->memory_->SetSize(1);
            buffer->buffer_->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            HandleEOS(index);
            break;
        }
        OH_AVCodecBufferAttr attr;
        OH_AVBuffer_GetBufferAttr(buffer, &attr);
        if (attr.size == 0) {
            PopInputQueues(signal_);
            continue;
        }
        buffer->buffer_->memory_->SetSize(attr.size);
        buffer->buffer_->flag_ = isFirstFrame_ ? AVCODEC_BUFFER_FLAGS_CODEC_DATA : AVCODEC_BUFFER_FLAGS_NONE;
        int32_t ret = OH_AudioCodec_PushInputBuffer(audioDec_, index);
        isFirstFrame_ = false;
        PopInputQueues(signal_);
        frameCount_++;
        if (ret != AVCS_ERR_OK) {
            isRunning_.store(false);
            signal_->startCond_.notify_all();
            break;
        }
    }
}

inline void PopOutputQueues(AudioCodecBufferSignal* sig, bool popGen = true)
{
    if (!sig->outQueue_.empty()) {
        sig->outQueue_.pop();
    }
    if (!sig->outBufferQueue_.empty()) {
        sig->outBufferQueue_.pop();
    }
    if (popGen && !sig->outGenQueue_.empty()) {
        sig->outGenQueue_.pop();
    }
}

void AudioCodeDtsDecoderUnitTest::OutputFunc()
{
    if (!pcmOutputFile_.is_open()) {
        std::cout << "open " << DTS_FILE_TODEMUX << " failed!" << std::endl;
    }
    while (isRunning_.load()) {
        unique_lock<mutex> lock(signal_->outMutex_);
        signal_->outCond_.wait(lock, [this]() { return !signal_->outQueue_.empty() || !isRunning_.load(); });
        if (!isRunning_.load()) {
            break;
        }
        uint32_t index = signal_->outQueue_.front();
        OH_AVBuffer *data = signal_->outBufferQueue_.empty() ? nullptr : signal_->outBufferQueue_.front();
        uint64_t bufGen = signal_->outGenQueue_.empty() ? 0 : signal_->outGenQueue_.front();
        uint64_t curGen = signal_->generation_.load(std::memory_order_relaxed);
        if (!data) {
            PopOutputQueues(signal_);
            continue;
        }
        if (bufGen != curGen) {
            staleOutputDiscarded_++;
            PopOutputQueues(signal_);
            continue;
        }
        if (data->magic_ != MFMagic::MFMAGIC_AVBUFFER) {
            PopOutputQueues(signal_);
            continue;
        }
        OH_AVCodecBufferAttr outAttr;
        OH_AVBuffer_GetBufferAttr(data, &outAttr);
        if (frameCount_ < FRAME_COUNT) {
            cout << "[OutputFunc] loop=" << loopIndex_ << " gen=" << curGen << " idx=" << index
                 << " outSize=" << outAttr.size << " flags=" << outAttr.flags << " pts=" << data->buffer_->pts_ << endl;
        }
        pcmOutputFile_.write(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(data)), data->buffer_->memory_->GetSize());
        if (data->buffer_->flag_ == AVCODEC_BUFFER_FLAGS_EOS || data->buffer_->memory_->GetSize() == 0) {
            isRunning_.store(false);
            signal_->startCond_.notify_all();
        }
        PopOutputQueues(signal_);
        if (OH_AudioCodec_FreeOutputBuffer(audioDec_, index) != AV_ERR_OK) {
            isRunning_.store(false);
            signal_->startCond_.notify_all();
            break;
        }
    }
    pcmOutputFile_.close();
}

int32_t AudioCodeDtsDecoderUnitTest::Start()
{
    isRunning_.store(true);
    inputLoop_ = make_unique<thread>(&AudioCodeDtsDecoderUnitTest::InputFunc, this);
    if (inputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    outputLoop_ = make_unique<thread>(&AudioCodeDtsDecoderUnitTest::OutputFunc, this);
    if (outputLoop_ == nullptr) {
        cout << "Fatal: No memory" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AudioCodec_Start(audioDec_);
}

int32_t AudioCodeDtsDecoderUnitTest::Stop()
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

void AudioCodeDtsDecoderUnitTest::StopLoops()
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

int32_t AudioCodeDtsDecoderUnitTest::InitFile(string_view input)
{
    CleanUp();
    isFirstFrame_ = true;
    frameCount_ = 0;
    pcmOutputFile_.open(OUTPUT_DTS_PCM_FILE_PATH.data(), std::ios::out | std::ios::binary);
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    int fd = open(input.data(), O_RDONLY);
    int64_t size = GetFileSize(input.data());
    cout << input.data() << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        cout << "Fatal: source is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (demuxer == nullptr) {
        cout << "Fatal: demuxer is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    OH_AVErrCode ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: OH_AVDemuxer_SelectTrackByID is fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioCodeDtsDecoderUnitTest::InitFile(string_view input, string_view output)
{
    CleanUp();
    isFirstFrame_ = true;
    frameCount_ = 0;
    pcmOutputFile_.open(output.data(), std::ios::out | std::ios::binary);
    if (!pcmOutputFile_.is_open()) {
        cout << "Fatal: open output file failed" << output << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    int fd = open(input.data(), O_RDONLY);
    int64_t size = GetFileSize(input.data());
    cout << input.data() << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        cout << "Fatal: source is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (demuxer == nullptr) {
        cout << "Fatal: demuxer is null" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    OH_AVErrCode ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    if (ret != OH_AVErrCode::AV_ERR_OK) {
        cout << "Fatal: OH_AVDemuxer_SelectTrackByID is fail" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    return OH_AVErrCode::AV_ERR_OK;
}

int32_t AudioCodeDtsDecoderUnitTest::CreateCodecFunc()
{
    audioDec_ = OH_AudioCodec_CreateByName((std::string(AVCodecCodecName::AUDIO_DECODER_DTS_NAME)).data());
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

int32_t AudioCodeDtsDecoderUnitTest::Configure()
{
    format_ = OH_AVFormat_Create();
    if (format_ == nullptr) {
        cout << "Fatal: create format failed" << endl;
        return OH_AVErrCode::AV_ERR_UNKNOWN;
    }
    int32_t channel = 1;
    int32_t sampleRate = 48000;
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), channel);
    OH_AVFormat_SetIntValue(format_, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), sampleRate);
    return OH_AudioCodec_Configure(audioDec_, format_);
}

int32_t AudioCodeDtsDecoderUnitTest::Reset()
{
    isRunning_.store(false);
    StopLoops();
    if (signal_) {
        signal_->generation_.fetch_add(1, std::memory_order_relaxed);
    }
    CleanUp();
    return OH_AudioCodec_Reset(audioDec_);
}

void AudioCodeDtsDecoderUnitTest::CleanUp()
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

HWTEST_F(AudioCodeDtsDecoderUnitTest, AudioDemuxAndDecode_Dts_001, TestSize.Level1)
{
    isTestingFormat_ = true;
    ASSERT_EQ(OH_AVErrCode::AV_ERR_OK, InitFile(DTS_FILE_TODEMUX, OUTPUT_DTS_PCM_FILE_PATH));
    ASSERT_EQ(AV_ERR_OK, CreateCodecFunc());
    OH_AVFormat *fmt = OH_AVFormat_Create();
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_CHANNEL_COUNT.data(), 1);
    OH_AVFormat_SetIntValue(fmt, MediaDescriptionKey::MD_KEY_SAMPLE_RATE.data(), 48000);
    EXPECT_EQ(AV_ERR_OK, OH_AudioCodec_Configure(audioDec_, fmt));
    EXPECT_EQ(OH_AVErrCode::AV_ERR_OK, Start());
    {
        unique_lock<mutex> lock(signal_->startMutex_);
        signal_->startCond_.wait(lock, [this]() { return (!(isRunning_.load())); });
    }
    OH_AVFormat_Destroy(fmt);
    Release();
}

HWTEST_F(AudioCodeDtsDecoderUnitTest, SetParameter_001, TestSize.Level1)
{
    auto meta = make_shared<Media::Meta>();
    int32_t channels = 1;
    int32_t sampleRate = 48000;
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
}

HWTEST_F(AudioCodeDtsDecoderUnitTest, SetParameter_002, TestSize.Level1)
{
    auto meta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta), Status::OK);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    ASSERT_NE(plugin_->SetParameter(meta), Status::OK);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta), Status::OK);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(-1);
    ASSERT_NE(plugin_->SetParameter(meta), Status::OK);
}

HWTEST_F(AudioCodeDtsDecoderUnitTest, SetParameter_003, TestSize.Level1)
{
    auto meta = make_shared<Media::Meta>();
    int32_t channels = 1;
    int32_t sampleRate = 48000;
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);

    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta), Status::OK);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
}

HWTEST_F(AudioCodeDtsDecoderUnitTest, Lifecycle_001, TestSize.Level1)
{
    auto meta = make_shared<Media::Meta>();
    int32_t channels = 1;
    int32_t sampleRate = 48000;
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);

    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->Release(), Status::OK);
}

HWTEST_F(AudioCodeDtsDecoderUnitTest, Eos_001, TestSize.Level1)
{
    auto meta = make_shared<Media::Meta>();
    int32_t channels = 1;
    int32_t sampleRate = 48000;
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);

    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(1024, 0);
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    int32_t maxInputSize = 8192;
    auto outputBuffer = CreateAVBuffer(maxInputSize * 4, 0);
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(AudioCodeDtsDecoderUnitTest, GetParameter_001, TestSize.Level1)
{
    auto meta = make_shared<Media::Meta>();
    int32_t channels = 1;
    int32_t sampleRate = 48000;
    int32_t maxInputSize = 8192;
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t getSampleRate = 0;
    maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, channels);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(getSampleRate));
    ASSERT_EQ(getSampleRate, sampleRate);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

HWTEST_F(AudioCodeDtsDecoderUnitTest, GetParameter_002, TestSize.Level1)
{
    auto meta = make_shared<Media::Meta>();
    int32_t channels = 1;
    int32_t sampleRate = 48000;
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    int32_t defaultInputSize = 16384;
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    int32_t usrInputSize = 0;
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize / 2);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_NE(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize * 2);
    ASSERT_EQ(plugin_->SetParameter(meta), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
}
} // namespace MediaAVCodec
} // namespace OHOS