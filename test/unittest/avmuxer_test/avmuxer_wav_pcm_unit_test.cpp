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

#include "avmuxer_wav_pcm_unit_test.h"
#include <gtest/gtest.h>
#include <string>
#include <vector>
#include <fcntl.h>
#include "avmuxer.h"
#include "native_avbuffer.h"
#include "avcodec_audio_channel_layout.h"
#ifdef AVMUXER_UNITTEST_CAPI
#include "native_avmuxer.h"
#include "native_avformat.h"
#endif

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
namespace {
const std::string TEST_FILE_PATH = "/data/test/media/";
constexpr int32_t TEST_CHANNEL_COUNT = 2;
constexpr int32_t TEST_SAMPLE_RATE = 44100;
constexpr int32_t TEST_BIT_RATE = 1411200;
constexpr int32_t TEST_SAMPLE_PER_FRAME = 1024;
}  // namespace

void AVMuxerWavPcmUnitTest::SetUpTestCase()
{}

void AVMuxerWavPcmUnitTest::TearDownTestCase()
{}

void AVMuxerWavPcmUnitTest::SetUp()
{
    avmuxer_ = std::make_shared<AVMuxerSample>();
}

void AVMuxerWavPcmUnitTest::TearDown()
{
    if (fd_ >= 0) {
        close(fd_);
        fd_ = -1;
    }

    if (inputFile_ != nullptr) {
        if (inputFile_->is_open()) {
            inputFile_->close();
        }
    }

    if (avmuxer_ != nullptr) {
        avmuxer_->Destroy();
        avmuxer_ = nullptr;
    }
}

static int32_t WritePCMSampleWithMetadata(std::shared_ptr<std::ofstream> file, const uint8_t *pcmData,
    uint32_t pcmSize, int64_t pts, uint32_t flags)
{
    std::cout << "WritePCMSampleWithMetadata: pts=" << pts << ", flags=0x" << std::hex << flags << std::dec
              << ", pcmSize=" << pcmSize << std::endl;

    std::cout << std::dec << std::endl;

    file->write(reinterpret_cast<const char *>(&pts), sizeof(pts));
    file->write(reinterpret_cast<const char *>(&flags), sizeof(flags));
    file->write(reinterpret_cast<const char *>(&pcmSize), sizeof(pcmSize));
    file->write(reinterpret_cast<const char *>(pcmData), pcmSize);

    if (file->fail()) {
        return 0;
    }

    return 0;
}

static int32_t ProcessPCMFile(
    std::shared_ptr<std::ifstream> pcmFile, const std::string &outputFileName, int32_t trackId, uint32_t extraFlag = 0)
{
    if (!pcmFile->is_open()) {
        std::cerr << "open pcm file fail: " << pcmFile << std::endl;
        return 0;
    }
    pcmFile->seekg(0, std::ios::end);
    size_t pcmSize = pcmFile->tellg();
    pcmFile->seekg(0, std::ios::beg);

    if (pcmSize == 0) {
        std::cerr << "pcm file empty" << std::endl;
        return 0;
    }
    std::vector<uint8_t> pcmData(pcmSize);
    pcmFile->read(reinterpret_cast<char *>(pcmData.data()), pcmSize);
    pcmFile->close();

    if (pcmFile->gcount() != static_cast<std::streamsize>(pcmSize)) {
        std::cerr << "pcm file incomplete!" << std::endl;
        return 0;
    }

    std::shared_ptr<std::ofstream> outFile = std::make_shared<std::ofstream>(outputFileName, std::ios::binary);
    if (!outFile->is_open()) {
        std::cerr << "create output  file fail: " << outputFileName << std::endl;
        return 0;
    }

    int64_t pts = 0;
    uint32_t flags = AVCODEC_BUFFER_FLAGS_NONE;
    int32_t ret = WritePCMSampleWithMetadata(outFile, pcmData.data(), pcmSize, pts, flags);
    if (ret != 0) {
        std::cerr << "write metadata fail, errcode: " << ret << std::endl;
        outFile->close();
        return ret;
    }
    outFile->close();
    return 0;
}

int32_t AVMuxerWavPcmUnitTest::WriteSample(
    int32_t trackId, std::shared_ptr<std::ifstream> file, bool &eosFlag, uint32_t flag)
{
    OH_AVCodecBufferAttr info;

    if (file->eof()) {
        eosFlag = true;
        std::cout << "file reach eof, cannot read pts" << std::endl;
        return 0;
    }
    file->read(reinterpret_cast<char *>(&info.pts), sizeof(info.pts));

    if (file->eof()) {
        eosFlag = true;
        std::cout << "file reach eof, cannot read flags" << std::endl;
        return 0;
    }
    file->read(reinterpret_cast<char *>(&info.flags), sizeof(info.flags));

    if (file->eof()) {
        eosFlag = true;
        std::cout << "file reach eof, cannot read size" << std::endl;
        return 0;
    }
    file->read(reinterpret_cast<char *>(&info.size), sizeof(info.size));

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    if (info.flags & AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
        info.flags |= flag;
    }

    OH_AVBuffer *buffer = OH_AVBuffer_Create(info.size);
    file->read(reinterpret_cast<char *>(OH_AVBuffer_GetAddr(buffer)), info.size);
    std::cout << "WriteSample: readMetadata - pts=" << info.pts << ", flags=0x" << std::hex << info.flags << std::dec
              << ", size=" << info.size << std::endl;
    OH_AVBuffer_SetBufferAttr(buffer, &info);
    int32_t ret = avmuxer_->WriteSampleBuffer(trackId, buffer);
    OH_AVBuffer_Destroy(buffer);
    return ret;
}

namespace {
/**
 * @tc.name: Muxer_WAV_PCM_U8
 * @tc.desc: supported bitwidth SAMPLE_U8
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_U8, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_U8.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_U8);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2_u8.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2_u8.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_S16LE
 * @tc.desc: supported bitwidth SAMPLE_S16LE
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_S16LE, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_S24LE
 * @tc.desc: supported bitwidth SAMPLE_S24LE
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_S24LE, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_S24LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S24LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2_s24le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2_s24le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_S32LE
 * @tc.desc: supported bitwidth SAMPLE_S32LE
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_S32LE, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_S32LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S32LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2_s32le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2_s32le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_F32LE
 * @tc.desc: supported bitwidth SAMPLE_F32LE
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_F32LE, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_F32LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_F32LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2_f32le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2_f32le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_SAMPLE_S16P
 * @tc.desc: unsupported bitwidth SAMPLE_S16P
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_SAMPLE_S16P, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_S16P.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16P);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, STEREO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_NE(ret, 0); // unsupported sample format
}


/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_MONO
 * @tc.desc: channel layout mono
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_MONO, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_MONO_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, MONO);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_mono_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_mono_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_2POINT1
 * @tc.desc: channel layout 2point1
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_2POINT1, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_2POINT1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 3); // 3 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_2POINT1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2point1_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2point1_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_2_1
 * @tc.desc: channel layout 2_1
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_2_1, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_2_1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 3); // 3 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_2_1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2_1_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2_1_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_SURROUND
 * @tc.desc: channel layout surround
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_SURROUND, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_SURROUND_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 3); // 3 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, SURROUND);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_surround_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_surround_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_3POINT1
 * @tc.desc: channel layout 3point1
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_3POINT1, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_3POINT1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 4); // 4 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_3POINT1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_3point1_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_3point1_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_4POINT0
 * @tc.desc: channel layout 4point0
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_4POINT0, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_4POINT0_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 4); // 4 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_4POINT0);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_4point0_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_4point0_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_4POINT1
 * @tc.desc: channel layout 4point1
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_4POINT1, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_4POINT1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 5); // 5 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_4POINT1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_4point1_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_4point1_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_2_2
 * @tc.desc: channel layout 2_2
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_2_2, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_4POINT1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 4); // 4 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_2_2);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_2_2_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_2_2_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_QUAD
 * @tc.desc: channel layout quad
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_QUAD, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_QUAD_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 4); // 4 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, QUAD);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_quad_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_quad_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT0
 * @tc.desc: channel layout 5point0
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT0, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_5POINT0_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 5); // 5 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_5POINT0);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_5point0_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_5point0_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT1
 * @tc.desc: channel layout 5point1
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT1, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_5POINT1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 6); // 6 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_5POINT1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_5point1_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_5point1_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT0_BACK
 * @tc.desc: channel layout 5point0back
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT0_BACK, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_5POINT0_BACK_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 5); // 5 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_5POINT0_BACK);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_5point0back_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_5point0back_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT1_BACK
 * @tc.desc: channel layout 5point1back
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_5POINT1_BACK, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_5POINT1_BACK_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 6); // 6 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_5POINT1_BACK);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_5point1back_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_5point1back_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT0
 * @tc.desc: channel layout 6point0
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT0, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_5POINT1_BACK_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 6); // 6 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_6POINT0);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_6point0_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_6point0_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT0_FRONT
 * @tc.desc: channel layout 6point0front
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT0_FRONT, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_6POINT0_FRONT_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 6); // 6 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_6POINT0_FRONT);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_6point0front_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_6point0front_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_HEXAGONAL
 * @tc.desc: channel layout hexagonal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_HEXAGONAL, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_HEXAGONAL_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 6); // 6 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, HEXAGONAL);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_hexagonal_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_hexagonal_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT1
 * @tc.desc: channel layout 6point1
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT1, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_6POINT1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 7); // 7 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_6POINT1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_6point1_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_6point1_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT1_BACK
 * @tc.desc: channel layout 6point1back
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT1_BACK, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_6POINT1_BACK_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 7); // 7 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_6POINT1_BACK);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_6point1back_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_6point1back_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT1_FRONT
 * @tc.desc: channel layout 6point1front
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_6POINT1_FRONT, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_6POINT1_FRONT_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 7); // 7 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_6POINT1_FRONT);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_6point1front_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_6point1front_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT0
 * @tc.desc: channel layout 7point0
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT0, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_7POINT0_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 7); // 7 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_7POINT0);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_7point0_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_7point0_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT0_FRONT
 * @tc.desc: channel layout 7point0front
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT0_FRONT, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_7POINT0_FRONT_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 7); // 7 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_7POINT0_FRONT);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_7point0front_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_7point0front_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT1
 * @tc.desc: channel layout 7point1
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT1, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_7POINT1_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 8); // 8 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_7POINT1);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_7point1_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_7point1_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT1_WIDE
 * @tc.desc: channel layout 7point1wide
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT1_WIDE, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_7POINT1_WIDE_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 8); // 8 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_7POINT1_WIDE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_7point1wide_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_7point1wide_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT1_WIDE_BACK
 * @tc.desc: channel layout 7point1wideback
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_CH_7POINT1_WIDE_BACK, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_CH_7POINT1_WIDE_BACK_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 8); // 8 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_7POINT1_WIDE_BACK);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_7point1wideback_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_7point1wideback_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_OCTAGONAL
 * @tc.desc: channel layout octagonal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_OCTAGONAL, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_OCTAGONAL_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 8); // 8 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, OCTAGONAL);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_octagonal_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_octagonal_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_HEXADECAGONAL
 * @tc.desc: channel layout hexadecagonal
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_HEXADECAGONAL, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_HEXADECAGONAL_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 16); // 16 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, HEXADECAGONAL);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_hexadecagonal_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_hexadecagonal_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_STEREO_DOWNMIX
 * @tc.desc: channel layout stereo downmix
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_STEREO_DOWNMIX, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_stereodownmix_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, TEST_CHANNEL_COUNT);
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, STEREO_DOWNMIX);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    std::shared_ptr<std::ifstream> inputFile =
        std::make_shared<std::ifstream>("/data/test/media/wav_pcm_44100_stereodownmix_s16le.pcm", std::ios::binary);
    std::string outputDat_ = TEST_FILE_PATH + std::string("pcm_44100_stereodownmix_s16le.dat");

    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> datFile = std::make_shared<std::ifstream>(outputDat_, std::ios::binary);

    bool eosFlag = false;

    ret = WriteSample(trackId, datFile, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, datFile, eosFlag, flag);
        ret = ProcessPCMFile(inputFile, outputDat_, trackId, flag);
    }
    ASSERT_EQ(avmuxer_->Stop(), 0);

    datFile->close();
    close(fd_);
}

/**
 * @tc.name: Muxer_WAV_PCM_CHANNEL_LAYOUT_3POINT1POINT2
 * @tc.desc: unsupported channel layout 3.1.2
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerWavPcmUnitTest, Muxer_WAV_PCM_CHANNEL_LAYOUT_3POINT1POINT2, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_PCM_44100_3point1point2_S16LE.wav");
    OH_AVOutputFormat outputFormat = AV_OUTPUT_FORMAT_WAV;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    bool isCreated = avmuxer_->CreateMuxer(fd_, outputFormat);
    ASSERT_TRUE(isCreated);

    std::shared_ptr<FormatMock> audioParams = FormatMockFactory::CreateFormat();
    audioParams->PutStringValue(OH_MD_KEY_CODEC_MIME, "audio/raw");
    audioParams->PutIntValue(OH_MD_KEY_AUD_SAMPLE_RATE, TEST_SAMPLE_RATE);
    audioParams->PutIntValue(OH_MD_KEY_AUD_CHANNEL_COUNT, 6); // 6 channels
    audioParams->PutIntValue(OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
    audioParams->PutLongValue(OH_MD_KEY_BITRATE, TEST_BIT_RATE);
    audioParams->PutIntValue("audio_samples_per_frame", TEST_SAMPLE_PER_FRAME);
    audioParams->PutLongValue(OH_MD_KEY_CHANNEL_LAYOUT, CH_3POINT1POINT2);  // not supported
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_NE(ret, 0); // channel layout not supported
}
}  // namespace