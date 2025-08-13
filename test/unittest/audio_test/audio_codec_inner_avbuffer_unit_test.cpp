/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "audio_codec_inner_avbuffer_unit_test.h"
#include <unistd.h>
#include <string>
#include <string_view>
#include <iostream>
#include <unistd.h>
#include <chrono>
#include "securec.h"
#include "demo_log.h"
#include "meta/audio_types.h"
#include "avcodec_audio_codec.h"
#include "avcodec_codec_name.h"
#include "avcodec_mime_type.h"
#include "avcodec_errors.h"
#include "avcodec_list.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr uint32_t CHANNEL_COUNT_STEREO = 2;
constexpr uint32_t SAMPLE_RATE = 48000;
constexpr std::string_view INPUT_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.dat";
constexpr std::string_view OUTPUT_PCM_FILE_PATH = "/data/test/media/mp3_2c_44100hz_60k.pcm";
constexpr std::string_view INPUT_AC3_FILE_PATH = "/data/test/media/voice_ac3.dat";
constexpr std::string_view OUTPUT_AC3_PCM_FILE_PATH = "/data/test/media/voice_ac3.pcm";
constexpr int32_t INPUT_FRAME_BYTES = 2 * 1024 * 4;
constexpr int32_t TIME_OUT_MS = 8;
constexpr int64_t MILLISECOUND_TO_SECOND = 1000;
constexpr std::string_view MP3_CODEC_NAME = AVCodecCodecName::AUDIO_DECODER_MP3_NAME;
constexpr std::string_view AC3_CODEC_NAME = AVCodecCodecName::AUDIO_DECODER_AC3_NAME;
constexpr int64_t CHANGE_PLUGIN_INTERVAL_MS = 1;
constexpr uint32_t MAX_MIME_LENGTH = 255;

typedef enum OH_AVCodecBufferFlags {
    AVCODEC_BUFFER_FLAGS_NONE = 0,
    /* Indicates that the Buffer is an End-of-Stream frame */
    AVCODEC_BUFFER_FLAGS_EOS = 1 << 0,
    /* Indicates that the Buffer contains keyframes */
    AVCODEC_BUFFER_FLAGS_SYNC_FRAME = 1 << 1,
    /* Indicates that the data contained in the Buffer is only part of a frame */
    AVCODEC_BUFFER_FLAGS_INCOMPLETE_FRAME = 1 << 2,
    /* Indicates that the Buffer contains Codec-Specific-Data */
    AVCODEC_BUFFER_FLAGS_CODEC_DATA = 1 << 3,
} OH_AVCodecBufferFlags;
}  // namespace

namespace OHOS {
namespace MediaAVCodec {
static std::string GetCapabilityName(const char *mime, bool isEncoder)
{
    if (mime == nullptr) {
        std::cout << "Get capability failed: mime is nullptr" << std::endl;
        return "";
    }
    if (strlen(mime) == 0 || strlen(mime) > MAX_MIME_LENGTH) {
        std::cout << "Get capability failed: invalid mime strlen, " << strlen(mime) << std::endl;
        return "";
    }
    std::shared_ptr<AVCodecList> codeclist = AVCodecListFactory::CreateAVCodecList();
    if (codeclist == nullptr) {
        std::cout << "Get capability failed: CreateAVCodecList failed" << std::endl;
        return "";
    }
    CapabilityData *capabilityData = codeclist->GetCapability(mime, isEncoder, AVCodecCategory::AVCODEC_NONE);
    if (capabilityData == nullptr) {
        std::cout << "Get capability failed: cannot find matched capability" << std::endl;
        return "";
    }
    return capabilityData->codecName;
}

static int32_t GetFileSize(const std::string &filePath)
{
    std::ifstream file(filePath, std::ios::binary | std::ios::ate);
    if (!file) {
        std::cerr << "Failed to open file:" << filePath << std::endl;
        return -1;
    }

    std::streampos fileSize = file.tellg();
    file.close();

    return (int32_t)fileSize;
}

void AVCodecAudioCodecUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AVCodecAudioCodecUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void AVCodecAudioCodecUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void AVCodecAudioCodecUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

int32_t AudioDecInnerAvBuffer::RunCase(
    const std::string_view &codecName, const std::string_view &inputPath, const std::string_view &outputPath)
{
    int32_t ret = AVCodecServiceErrCode::AVCS_ERR_OK;
    innerBufferQueue_ = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "InnerDemo");  // 4
    audioCodec_ = AudioCodecFactory::CreateByName(codecName.data());
    DEMO_CHECK_AND_RETURN_RET_LOG(
        audioCodec_ != nullptr, AVCodecServiceErrCode::AVCS_ERR_UNKNOWN, "audioCodec_ is null");
    bool enableSync = false;
    meta_->GetData(Tag::AV_CODEC_ENABLE_SYNC_MODE, enableSync);
    EXPECT_EQ(static_cast<int32_t>(AVCS_ERR_OK), audioCodec_->Configure(meta_));

    if (!enableSync) {
        audioCodec_->SetOutputBufferQueue(innerBufferQueue_->GetProducer());
    }
    EXPECT_EQ(static_cast<int32_t>(AVCS_ERR_OK), audioCodec_->Prepare());

    if (!enableSync) {
        implConsumer_ = innerBufferQueue_->GetConsumer();
        sptr<Media::IConsumerListener> comsumerListener = new AudioCodecConsumerListener(this);
        implConsumer_->SetBufferAvailableListener(comsumerListener);
        mediaCodecProducer_ = audioCodec_->GetInputBufferQueue();
    }

    EXPECT_EQ(static_cast<int32_t>(AVCS_ERR_OK), audioCodec_->Start());
    isRunning_.store(true);

    fileSize_ = GetFileSize(inputPath.data());
    inputFile_ = std::make_unique<std::ifstream>(inputPath, std::ios::binary);
    outputFile_ = std::make_unique<std::ofstream>(outputPath, std::ios::binary);

    if (enableAsyncChangePluginTest_) {
        isChangePluginThreadRunning_ = true;
        changePluginThread_ = std::make_unique<std::thread>([this] {
            constexpr bool isEncoder = false;
            while (isChangePluginThreadRunning_) {
                std::this_thread::sleep_for(std::chrono::milliseconds(CHANGE_PLUGIN_INTERVAL_MS));
                audioCodec_->ChangePlugin("audio/mpeg", isEncoder, meta_);
            }
        });
    }

    if (!enableSync) {
        InputFunc();
    } else {
        SyncFunc();
    }
    EXPECT_GT(outputSize_, 0);

    if (changePluginThread_ && changePluginThread_->joinable()) {
        isChangePluginThreadRunning_ = false;
        enableAsyncChangePluginTest_ = false;
        changePluginThread_->join();
        changePluginThread_.reset();
    }

    inputFile_->close();
    outputFile_->close();
    return ret;
}

int32_t AudioDecInnerAvBuffer::GetInputBufferSize()
{
    int32_t capacity = 0;
    DEMO_CHECK_AND_RETURN_RET_LOG(audioCodec_ != nullptr, capacity, "audioCodec_ is nullptr");
    std::shared_ptr<Media::Meta> bufferConfig = std::make_shared<Media::Meta>();
    DEMO_CHECK_AND_RETURN_RET_LOG(bufferConfig != nullptr, capacity, "bufferConfig is nullptr");
    int32_t ret = audioCodec_->GetOutputFormat(bufferConfig);
    DEMO_CHECK_AND_RETURN_RET_LOG(ret == AVCodecServiceErrCode::AVCS_ERR_OK, capacity, "GetOutputFormat fail");
    DEMO_CHECK_AND_RETURN_RET_LOG(
        bufferConfig->Get<Media::Tag::AUDIO_MAX_INPUT_SIZE>(capacity), capacity, "get max input buffer size fail");
    return capacity;
}

void AudioDecInnerAvBuffer::InputFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(inputFile_ != nullptr && inputFile_->is_open(), "Fatal: open file fail");
    int32_t sumReadSize = 0;
    Media::Status ret;
    int64_t size;
    int64_t pts;
    Media::AVBufferConfig avBufferConfig;
    avBufferConfig.size = GetInputBufferSize();
    while (isRunning_) {
        std::shared_ptr<AVBuffer> inputBuffer = nullptr;
        DEMO_CHECK_AND_BREAK_LOG(mediaCodecProducer_ != nullptr, "mediaCodecProducer_ is nullptr");
        ret = mediaCodecProducer_->RequestBuffer(inputBuffer, avBufferConfig, TIME_OUT_MS);
        if (ret != Media::Status::OK) {
            std::cout << "produceInputBuffer RequestBuffer fail,ret=" << (int32_t)ret << std::endl;
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputBuffer != nullptr, "buffer is nullptr");
        inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
        if (inputFile_->eof() || inputFile_->gcount() == 0 || size == 0) {
            inputBuffer->memory_->SetSize(1);
            inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            sumReadSize += 0;
            mediaCodecProducer_->PushBuffer(inputBuffer, true);
            sumReadSize += inputFile_->gcount();
            std::cout << "InputFunc, INPUT_FRAME_BYTES:" << INPUT_FRAME_BYTES << " flag:" << inputBuffer->flag_
                      << " sumReadSize:" << sumReadSize << " fileSize_:" << fileSize_
                      << " process:" << 100 * sumReadSize / fileSize_ << "%" << std::endl;  // 100
            std::cout << "end buffer\n";
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == sizeof(size), "Fatal: read size fail");
        sumReadSize += inputFile_->gcount();
        inputFile_->read(reinterpret_cast<char *>(&pts), sizeof(pts));
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == sizeof(pts), "Fatal: read pts fail");
        sumReadSize += inputFile_->gcount();
        inputFile_->read(reinterpret_cast<char *>(inputBuffer->memory_->GetAddr()), size);
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == size, "Fatal: read buffer fail");
        inputBuffer->memory_->SetSize(size);
        inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
        sumReadSize += inputFile_->gcount();
        mediaCodecProducer_->PushBuffer(inputBuffer, true);
        std::cout << "InputFunc, INPUT_FRAME_BYTES:" << INPUT_FRAME_BYTES << " flag:" << inputBuffer->flag_
                  << " sumReadSize:" << sumReadSize << " fileSize_:" << fileSize_
                  << " process:" << 100 * sumReadSize / fileSize_ << "%" << std::endl;  // 100
    }
}

void AudioDecInnerAvBuffer::OutputFunc()
{
    bufferConsumerAvailableCount_++;
    Media::Status ret = Media::Status::OK;
    while (isRunning_ && (bufferConsumerAvailableCount_ > 0)) {
        std::cout << "/**********ImplConsumerOutputBuffer while**********/" << std::endl;
        std::shared_ptr<AVBuffer> outputBuffer;
        ret = implConsumer_->AcquireBuffer(outputBuffer);
        if (ret != Media::Status::OK) {
            std::cout << "Consumer AcquireBuffer fail,ret=" << (int32_t)ret << std::endl;
            break;
        }
        if (outputBuffer == nullptr) {
            std::cout << "OutputFunc OH_AVBuffer is nullptr" << std::endl;
            continue;
        }
        outputFile_->write(
            reinterpret_cast<char *>(outputBuffer->memory_->GetAddr()), outputBuffer->memory_->GetSize());
        outputSize_ += outputBuffer->memory_->GetSize();
        if (outputBuffer->flag_ == AVCODEC_BUFFER_FLAGS_EOS || outputBuffer->memory_->GetSize() == 0) {
            std::cout << "out eos" << std::endl;
            isRunning_.store(false);
        }
        implConsumer_->ReleaseBuffer(outputBuffer);
        bufferConsumerAvailableCount_--;
    }
}

void AudioDecInnerAvBuffer::SyncFunc()
{
    DEMO_CHECK_AND_RETURN_LOG(inputFile_ != nullptr && inputFile_->is_open(), "Fatal: open file fail");
    int32_t sumReadSize = 0;
    int32_t ret;
    int64_t size;
    int64_t pts;
    uint32_t index = 0;
    size_t requestBufferSize = GetInputBufferSize();
    while (isRunning_) {
        // input
        ret = audioCodec_->QueryInputBuffer(
            &index, requestBufferSize, static_cast<int64_t>(TIME_OUT_MS) * MILLISECOUND_TO_SECOND);
        if (ret != static_cast<int32_t>(Media::Status::OK)) {
            std::cout << "audioCodec_->QueryInputBuffer fail,ret=" << ret << std::endl;
            break;
        }
        std::shared_ptr<AVBuffer> inputBuffer = audioCodec_->GetInputBuffer(index);
        DEMO_CHECK_AND_BREAK_LOG(inputBuffer != nullptr, "buffer is nullptr");
        inputFile_->read(reinterpret_cast<char *>(&size), sizeof(size));
        if (inputFile_->eof() || inputFile_->gcount() == 0 || size == 0) {
            inputBuffer->memory_->SetSize(1);
            inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_EOS;
            sumReadSize += 0;
            audioCodec_->PushInputBuffer(index, true);
            sumReadSize += inputFile_->gcount();
            std::cout << "SyncFunc, INPUT_FRAME_BYTES:" << size << " flag:" << inputBuffer->flag_
                      << " sumReadSize:" << sumReadSize << " fileSize_:" << fileSize_
                      << " process:" << 100 * sumReadSize / fileSize_ << "%" << std::endl;  // 100
            std::cout << "end buffer\n";
            break;
        }
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == sizeof(size), "Fatal: read size fail");
        sumReadSize += inputFile_->gcount();
        inputFile_->read(reinterpret_cast<char *>(&pts), sizeof(pts));
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == sizeof(pts), "Fatal: read pts fail");
        sumReadSize += inputFile_->gcount();
        inputFile_->read(reinterpret_cast<char *>(inputBuffer->memory_->GetAddr()), size);
        DEMO_CHECK_AND_BREAK_LOG(inputFile_->gcount() == size, "Fatal: read buffer fail");
        inputBuffer->memory_->SetSize(size);
        inputBuffer->flag_ = AVCODEC_BUFFER_FLAGS_NONE;
        sumReadSize += inputFile_->gcount();
        audioCodec_->PushInputBuffer(index, true);
        std::cout << "SyncFunc, INPUT_FRAME_BYTES:" << size << " flag:" << inputBuffer->flag_
                  << " sumReadSize:" << sumReadSize << " fileSize_:" << fileSize_
                  << " process:" << 100 * sumReadSize / fileSize_ << "%" << std::endl;  // 100

        SyncOutputFunc();
    }
}

void AudioDecInnerAvBuffer::SyncOutputFunc()
{
    int32_t sumWriteSize = 0;
    while (isRunning_) {
        std::shared_ptr<AVBuffer> outputBuffer =
            audioCodec_->GetOutputBuffer(static_cast<int64_t>(TIME_OUT_MS) * MILLISECOUND_TO_SECOND);
        if (outputBuffer == nullptr) {
            std::cout << "SyncFunc OH_AVBuffer is nullptr" << std::endl;
            break;
        }
        std::cout << "SyncFunc, OUT_FRAME_BYTES:" << outputBuffer->memory_->GetSize()
                  << ", sumWriteSize:" << sumWriteSize << std::endl;
        sumWriteSize += outputBuffer->memory_->GetSize();
        outputFile_->write(
            reinterpret_cast<char *>(outputBuffer->memory_->GetAddr()), outputBuffer->memory_->GetSize());
        outputSize_ += outputBuffer->memory_->GetSize();
        if (outputBuffer->flag_ == AVCODEC_BUFFER_FLAGS_EOS || outputBuffer->memory_->GetSize() == 0) {
            std::cout << "out eos" << std::endl;
            isRunning_.store(false);
        }
        audioCodec_->ReleaseOutputBuffer(outputBuffer);
    }
}

void AudioDecInnerAvBuffer::EnableAsyncChangePluginTest()
{
    enableAsyncChangePluginTest_ = true;
}

AudioCodecConsumerListener::AudioCodecConsumerListener(AudioDecInnerAvBuffer *demo)
{
    demo_ = demo;
}

void AudioCodecConsumerListener::OnBufferAvailable()
{
    demo_->OutputFunc();
}

void AVCodecInnerCallback::OnError(AVCodecErrorType errorType, int32_t errorCode)
{
    (void)errorType;
    cout << "Error errorCode=" << errorCode << endl;
}

void AVCodecInnerCallback::OnOutputFormatChanged(const Format &format)
{
    (void)format;
    cout << "Format Changed" << endl;
}

void AVCodecInnerCallback::OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (void)index;
    (void)buffer;
}

void AVCodecInnerCallback::OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
{
    (void)index;
    (void)buffer;
}

HWTEST_F(AVCodecAudioCodecUnitTest, Stop_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Stop());

    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    auto &innerBufferQueue = audioDec->GetInnerBufferQueue();
    innerBufferQueue = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "AVBufferInnerUT");  // 4
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetOutputBufferQueue(innerBufferQueue->GetProducer()));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Prepare());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Start());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Stop());

    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Release());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Stop());
}

HWTEST_F(AVCodecAudioCodecUnitTest, Flush_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    auto &innerBufferQueue = audioDec->GetInnerBufferQueue();
    innerBufferQueue = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "AVBufferInnerUT");  // 4
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetOutputBufferQueue(innerBufferQueue->GetProducer()));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Prepare());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Start());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Flush());
}

HWTEST_F(AVCodecAudioCodecUnitTest, Reset_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Reset());
}

HWTEST_F(AVCodecAudioCodecUnitTest, Release_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Release());
}

HWTEST_F(AVCodecAudioCodecUnitTest, NotifyEos_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    auto &innerBufferQueue = audioDec->GetInnerBufferQueue();
    innerBufferQueue = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "AVBufferInnerUT");  // 4
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetOutputBufferQueue(innerBufferQueue->GetProducer()));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Prepare());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Start());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->NotifyEos());
}

HWTEST_F(AVCodecAudioCodecUnitTest, SetParameter_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    auto &innerBufferQueue = audioDec->GetInnerBufferQueue();
    innerBufferQueue = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "AVBufferInnerUT");  // 4
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetOutputBufferQueue(innerBufferQueue->GetProducer()));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Prepare());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Start());
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetParameter(meta));
}

HWTEST_F(AVCodecAudioCodecUnitTest, ChangePlugin_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    std::string mime;
    meta->GetData(Tag::MIME_TYPE, mime);
    meta->SetData(Tag::AUDIO_SAMPLE_FORMAT, Plugins::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    EXPECT_NE(AVCodecServiceErrCode::AVCS_ERR_OK, dec->ChangePlugin(mime, false, meta));
}

HWTEST_F(AVCodecAudioCodecUnitTest, SetDataCallback_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    std::shared_ptr<MediaCodecCallback> codecCallback = std::make_shared<AVCodecInnerCallback>();
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetCodecCallback(codecCallback));
}

HWTEST_F(AVCodecAudioCodecUnitTest, ProcessInputBuffer_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    auto &innerBufferQueue = audioDec->GetInnerBufferQueue();
    innerBufferQueue = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "AVBufferInnerUT");  // 4
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetOutputBufferQueue(innerBufferQueue->GetProducer()));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Prepare());
    dec->ProcessInputBuffer();
}

HWTEST_F(AVCodecAudioCodecUnitTest, SetDumpInfo_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    auto &innerBufferQueue = audioDec->GetInnerBufferQueue();
    innerBufferQueue = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "AVBufferInnerUT");  // 4
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetOutputBufferQueue(innerBufferQueue->GetProducer()));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Prepare());
    dec->SetDumpInfo(false, 0);
    dec->SetDumpInfo(true, 1);
    dec->SetDumpInfo(false, 0);
    dec->SetDumpInfo(true, 1);
}

HWTEST_F(AVCodecAudioCodecUnitTest, GetInputBufferQueueConsumer_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    sptr<Media::AVBufferQueueConsumer> inputConsumer = dec->GetInputBufferQueueConsumer();
    EXPECT_EQ(inputConsumer, nullptr);
}

HWTEST_F(AVCodecAudioCodecUnitTest, GetOutputBufferQueueProducer_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    sptr<Media::AVBufferQueueProducer> outputProducer = dec->GetOutputBufferQueueProducer();
    EXPECT_EQ(outputProducer, nullptr);
}

HWTEST_F(AVCodecAudioCodecUnitTest, ProcessInputBufferInner_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &innerBufferQueue = audioDec->GetInnerBufferQueue();
    innerBufferQueue = Media::AVBufferQueue::Create(4, Media::MemoryType::SHARED_MEMORY, "AVBufferInnerUT");  // 4
    auto &dec = audioDec->GetAudioCodec();
    auto &meta = audioDec->GetAudioMeta();
    dec = AudioCodecFactory::CreateByName(MP3_CODEC_NAME.data());
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Configure(meta));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->SetOutputBufferQueue(innerBufferQueue->GetProducer()));
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, dec->Prepare());

    auto &implConsumer = audioDec->GetImplConsumer();
    implConsumer = innerBufferQueue->GetConsumer();
    bool isTriggeredByOutport = true;
    bool isFlushed = true;
    uint32_t bufferStatus = 0;
    dec->ProcessInputBufferInner(isTriggeredByOutport, isFlushed, bufferStatus);
}

HWTEST_F(AVCodecAudioCodecUnitTest, AudioDecode_MP3, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &meta = audioDec->GetAudioMeta();
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(44100);  // expected sampleRate
    meta->Set<Tag::MEDIA_BITRATE>(60000);      // BITRATE is 60000
    EXPECT_EQ(
        AVCodecServiceErrCode::AVCS_ERR_OK, audioDec->RunCase(MP3_CODEC_NAME, INPUT_FILE_PATH, OUTPUT_PCM_FILE_PATH));
}

HWTEST_F(AVCodecAudioCodecUnitTest, AudioDecode_AC3, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &meta = audioDec->GetAudioMeta();
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(44100);  // expected sampleRate
    meta->Set<Tag::MEDIA_BITRATE>(60000);      // BITRATE is 60000
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK,
        audioDec->RunCase(AC3_CODEC_NAME, INPUT_AC3_FILE_PATH, OUTPUT_AC3_PCM_FILE_PATH));
}

HWTEST_F(AVCodecAudioCodecUnitTest, AudioSyncDecode_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &meta = audioDec->GetAudioMeta();
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(44100);  // expected sampleRate
    meta->Set<Tag::MEDIA_BITRATE>(60000);      // BITRATE is 60000
    meta->Set<Tag::AV_CODEC_ENABLE_SYNC_MODE>(true);
    EXPECT_EQ(
        AVCodecServiceErrCode::AVCS_ERR_OK, audioDec->RunCase(MP3_CODEC_NAME, INPUT_FILE_PATH, OUTPUT_PCM_FILE_PATH));
}

HWTEST_F(AVCodecAudioCodecUnitTest, OnOutputFormatChanged_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &meta = audioDec->GetAudioMeta();
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(48000);  // supported but unexpected sampleRate
    meta->Set<Tag::MEDIA_BITRATE>(60000);      // BITRATE is 60000
    EXPECT_EQ(
        AVCodecServiceErrCode::AVCS_ERR_OK, audioDec->RunCase(MP3_CODEC_NAME, INPUT_FILE_PATH, OUTPUT_PCM_FILE_PATH));
}

HWTEST_F(AVCodecAudioCodecUnitTest, AsyncChangePluginWhileDecode_001, TestSize.Level1)
{
    auto audioDec = std::make_shared<AudioDecInnerAvBuffer>();
    auto &meta = audioDec->GetAudioMeta();
    meta = std::make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT_STEREO);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(Media::Plugins::AudioChannelLayout::STEREO);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(44100);  // expected sampleRate
    meta->Set<Tag::MEDIA_BITRATE>(60000);      // BITRATE is 60000
    audioDec->EnableAsyncChangePluginTest();
    EXPECT_EQ(
        AVCodecServiceErrCode::AVCS_ERR_OK, audioDec->RunCase(MP3_CODEC_NAME, INPUT_FILE_PATH, OUTPUT_PCM_FILE_PATH));
}

HWTEST_F(AVCodecAudioCodecUnitTest, CheckCreateDecoderByMimeType, TestSize.Level1)
{
    const vector<std::string_view> allMimeTable = {AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VORBIS,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VIVID,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711A,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_L2HC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_APE,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AC3,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_RAW};

    const bool isEncoder = false;
    for (const auto &mime : allMimeTable) {
        auto audioDec = AudioCodecFactory::CreateByMime(mime.data(), isEncoder);
        EXPECT_NE(nullptr, audioDec);
        if (audioDec == nullptr) {
            std::cout << "create " << mime << " failed" << std::endl;
        } else {
            audioDec->Release();
        }
    }
}

HWTEST_F(AVCodecAudioCodecUnitTest, CheckCreateDecoderByCodecName, TestSize.Level1)
{
    const vector<std::string_view> allMimeTable = {AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VORBIS,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VIVID,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711A,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_L2HC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_APE,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AC3,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_RAW};

    const bool isEncoder = false;
    for (const auto &mime : allMimeTable) {
        const std::string name = GetCapabilityName(mime.data(), isEncoder);
        if (name.empty()) {
            std::cout << name << "without Capability" << std::endl;
            continue;
        }
        auto audioDec = AudioCodecFactory::CreateByName(name.c_str());
        EXPECT_NE(nullptr, audioDec);
        if (audioDec == nullptr) {
            std::cout << "create " << name << " failed" << std::endl;
        } else {
            audioDec->Release();
        }
    }
}

HWTEST_F(AVCodecAudioCodecUnitTest, CheckCreateEncoderByMimeType, TestSize.Level1)
{
    const vector<std::string_view> allMimeTable = {AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_L2HC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC};

    const bool isEncoder = true;
    for (const auto &mime : allMimeTable) {
        auto audioEnc = AudioCodecFactory::CreateByMime(mime.data(), isEncoder);
        EXPECT_NE(nullptr, audioEnc);
        if (audioEnc == nullptr) {
            std::cout << "create " << mime << " failed" << std::endl;
        } else {
            audioEnc->Release();
        }
    }
}

HWTEST_F(AVCodecAudioCodecUnitTest, CheckCreateEncoderByCodecName, TestSize.Level1)
{
    const vector<std::string_view> allMimeTable = {AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_L2HC,
        AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC};

    const bool isEncoder = true;
    for (const auto &mime : allMimeTable) {
        const std::string name = GetCapabilityName(mime.data(), isEncoder);
        if (name.empty()) {
            std::cout << name << "without Capability" << std::endl;
            continue;
        }
        auto audioEnc = AudioCodecFactory::CreateByName(name.c_str());
        EXPECT_NE(nullptr, audioEnc);
        if (audioEnc == nullptr) {
            std::cout << "create " << name << " failed" << std::endl;
        } else {
            audioEnc->Release();
        }
    }
}

}  // namespace MediaAVCodec
}  // namespace OHOS