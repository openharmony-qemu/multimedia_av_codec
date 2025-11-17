/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <gtest/gtest.h>
#include <fstream>
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_codec_name.h"

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <cmath>
#include <thread>

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
constexpr uint32_t CHANNELS = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t UNSUPPORTED_CHANNELS_MIN = 0;
constexpr uint32_t UNSUPPORTED_CHANNELS_MAX = 256;
constexpr uint32_t UNSUPPORTED_MAX_INPUT_SIZE = 80 * 1024 * 1024 + 1;
constexpr uint32_t UNSUPPORTED_SAMPLE_RATE = 0;
constexpr int32_t MAX_INPUT_SIZE = 4096;
constexpr int32_t MIN_INPUT_SIZE = 16;
constexpr AudioSampleFormat SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16LE;
constexpr AudioSampleFormat RAW_SAMPLE_FORMAT = AudioSampleFormat::SAMPLE_S16BE;
constexpr uint32_t SAMPLE_RATE_48000 = 48000;
constexpr uint32_t SAMPLE_RATE_22254 = 22254;
const std::string OUTPUT_PREFIX = "/data/test/media/pcm_";
const std::string OUTPUT_SUFFIX = ".pcm";
constexpr int32_t BYTE_LENGTH_U8 = 1;
constexpr int32_t BYTE_LENGTH_S16 = 2;
constexpr int32_t BYTE_LENGTH_S24 = 3;
constexpr int32_t BYTE_LENGTH_S32_F32 = 4;
constexpr int32_t BYTE_LENGTH_DOUBLE = 8;
constexpr int32_t AUDIO_BITS_16 = 16;
constexpr int32_t AUDIO_BITS_20 = 20;
constexpr int32_t BLURAY_PACKET_SIZE = 964;
constexpr int32_t BLURAY_PACKET_OFFSET = 4;
constexpr int32_t DVD_PACKET_SIZE = 2013;
constexpr int32_t DVD_PACKET_OFFSET = 3;
constexpr int32_t BYTES_8 = 8;
constexpr int32_t BYTES_4 = 4;
constexpr int32_t WIDTH = 3840;
constexpr int32_t HEIGHT = 2160;
constexpr int32_t THOUSAND = 1000;
// {sampleFormat, {filePath, outputName}}
const std::unordered_map<AudioSampleFormat, std::pair<std::string, std::string>> inputFormatPathMap = {
    {AudioSampleFormat::SAMPLE_S8, std::make_pair("/data/test/media/pcm_s8.pcm", "s8_")},
    {AudioSampleFormat::SAMPLE_F64LE, std::make_pair("/data/test/media/pcm_f64le.pcm", "f64le_")},
    {AudioSampleFormat::SAMPLE_S64LE, std::make_pair("/data/test/media/pcm_s64le.pcm", "s64le_")},
    {AudioSampleFormat::SAMPLE_DVD, std::make_pair("/data/test/media/pcm_dvd_16bit.pcm", "dvd_16bit_")},
    {AudioSampleFormat::SAMPLE_BLURAY, std::make_pair("/data/test/media/pcm_bluray_16bit.pcm", "bluray_16bit_")},
};

const std::unordered_map<AudioSampleFormat, std::pair<std::string, std::string>> planarInputFormatPathMap = {
    {AudioSampleFormat::SAMPLE_S8P, std::make_pair("/data/test/media/pcm_s8_planar.dat", "s8p_")},
    {AudioSampleFormat::SAMPLE_S16BEP, std::make_pair("/data/test/media/pcm_s16be_planar.dat", "s16bep_")},
};

// {sampleFormat, outputName}
const std::vector<pair<AudioSampleFormat, std::string>> outputFormats = {
    std::make_pair(AudioSampleFormat::SAMPLE_U8, "to_u8"),
    std::make_pair(AudioSampleFormat::SAMPLE_S16LE, "to_s16le"),
    std::make_pair(AudioSampleFormat::SAMPLE_S24LE, "to_s24le"),
    std::make_pair(AudioSampleFormat::SAMPLE_S32LE, "to_s32le"),
    std::make_pair(AudioSampleFormat::SAMPLE_F32LE, "to_f32le")
};

const char *INP_DIR_1 = "/data/test/media/pcm_s64le_48000_1.avi";
const char *INP_DIR_2 = "/data/test/media/pcm_f64le_48000_2.avi";
const char *INP_DIR_3 = "/data/test/media/pcm_bluray.m2ts";
const char *INP_DIR_4 = "/data/test/media/pcm_f64le_48000_2.mkv";
const char *INP_DIR_5 = "/data/test/media/pcm_s64le_48000_1.mkv";
const char *INP_DIR_6 = "/data/test/media/pcm_f64le_48000_2.mov";
const char *INP_DIR_7 = "/data/test/media/pcm_dvd.mpg";
const char *INP_DIR_8 = "/data/test/media/pcm_f64le_48000_2.wav";
const char *INP_DIR_9 = "/data/test/media/pcm_s64le_48000_1.wav";

struct seekInfo {
    const char *fileName;
    OH_AVSeekMode seekmode;
    int64_t millisecond;
    int32_t videoCount;
    int32_t audioCount;
};

class RawDecoderUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

    void OnInputBufferDone(const shared_ptr<Media::AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
        std::shared_ptr<Meta> para = std::make_shared<Meta>();
        // for mutex test
        decoder_->GetParameter(para);
    }

    void OnOutputBufferDone(const shared_ptr<Media::AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
        std::shared_ptr<Meta> para = std::make_shared<Meta>();
        // for mutex test
        decoder_->GetParameter(para);
    }

    void OnEvent(const shared_ptr<Plugins::PluginEvent> event) override
    {
        (void)event;
    }

    int32_t GetFormatBytes(AudioSampleFormat format)
    {
        int32_t bytesSize = BYTE_LENGTH_S16;
        switch (format) {
            case AudioSampleFormat::SAMPLE_S16BE:
            /* fall-through */
            case AudioSampleFormat::SAMPLE_S16LE:
                bytesSize = BYTE_LENGTH_S16;
                break;
            case AudioSampleFormat::SAMPLE_S24BE:
            /* fall-through */
            case AudioSampleFormat::SAMPLE_S24LE:
            case AudioSampleFormat::SAMPLE_S24LEP:
                bytesSize = BYTE_LENGTH_S24;
                break;
            case AudioSampleFormat::SAMPLE_S32BE:
            /* fall-through */
            case AudioSampleFormat::SAMPLE_S32LE:
            case AudioSampleFormat::SAMPLE_S32LEP:
            case AudioSampleFormat::SAMPLE_F32BE:
            /* fall-through */
            case AudioSampleFormat::SAMPLE_F32LE:
                bytesSize = BYTE_LENGTH_S32_F32;
                break;
            case AudioSampleFormat::SAMPLE_F64LE:
            case AudioSampleFormat::SAMPLE_S64LE:
            case AudioSampleFormat::SAMPLE_F64BE:
                bytesSize = BYTE_LENGTH_DOUBLE;
                break;
            case AudioSampleFormat::SAMPLE_S8:
            case AudioSampleFormat::SAMPLE_S8P:
            case AudioSampleFormat::SAMPLE_U8:
                bytesSize = BYTE_LENGTH_U8;
                break;
            case AudioSampleFormat::SAMPLE_DVD: {
                int32_t audioBits = AUDIO_BITS_16;
                if (meta_->Get<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(audioBits)) {
                    bytesSize = audioBits == AUDIO_BITS_20 ? BYTE_LENGTH_S24 : BYTE_LENGTH_S16;
                }
                break;
            }
            case AudioSampleFormat::SAMPLE_BLURAY: {
                int32_t audioBits = AUDIO_BITS_16;
                bytesSize = BYTE_LENGTH_S16;
                if (meta_->Get<Tag::AUDIO_BITS_PER_RAW_SAMPLE>(audioBits)) {
                    bytesSize = audioBits == AUDIO_BITS_16 ? BYTE_LENGTH_S16 : BYTE_LENGTH_S24;
                }
                break;
            }
            default:
                break;
        }
        return bytesSize;
    }

    void SetupParameter(AudioSampleFormat inputFormat, AudioSampleFormat outputFormat)
    {
        if (planarInputFormatPathMap.count(inputFormat) != 0) {
            if (inputFormat == AudioSampleFormat::SAMPLE_S8P) {
                meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
            } else if (inputFormat == AudioSampleFormat::SAMPLE_S16BEP) {
                meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_22254);
            }
        } else {
            meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48000);
        }
        if (inputFormat == AudioSampleFormat::SAMPLE_BLURAY) {
            meta_->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
        }
        meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
        meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(inputFormat);
        meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AUDIO_BITS_16);
        meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(outputFormat);
    }

    void ProcessPackages(std::ifstream& inputFile, std::ofstream& outputFile,
                    int32_t srcBytes, int32_t destBytes)
    {
        auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        std::vector<uint8_t> buffer(BYTES_8);
        while (!inputFile.eof()) {
            inputFile.read(reinterpret_cast<char*>(buffer.data()), BYTES_8);
            int64_t packageSize;
            memcpy_s(&packageSize, sizeof(int64_t), buffer.data(), sizeof(int64_t));
            inputFile.read(reinterpret_cast<char*>(buffer.data()), BYTES_8);
            shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, packageSize);
            inputBuffer->memory_->SetSize(packageSize);
            inputFile.read(reinterpret_cast<char*>(inputBuffer->memory_->GetAddr()), packageSize);
            EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));
            
            int32_t totalSamples = packageSize / (CHANNELS * srcBytes);
            int32_t outputSize = totalSamples * CHANNELS * destBytes;
            shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputSize);
            shared_ptr<AVBuffer> emptyOutputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputSize);
            
            Status ret = Status::ERROR_AGAIN;
            int32_t pos = 0;
            while (ret == Status::ERROR_AGAIN) {
                ret = decoder_->QueueOutputBuffer(emptyOutputBuffer);
                outputBuffer->memory_->Write(emptyOutputBuffer->memory_->GetAddr(),
                                             emptyOutputBuffer->memory_->GetSize(), pos);
                pos += emptyOutputBuffer->memory_->GetSize();
            }
            ASSERT_EQ(Status::OK, ret);
            EXPECT_EQ(outputSize, outputBuffer->memory_->GetSize());
            outputFile.write(reinterpret_cast<char*>(outputBuffer->memory_->GetAddr()),
                             outputBuffer->memory_->GetSize());
        }
    }

    void DemuxerResultRaw(const char *fileName)
    {
        int tarckType = 0;
        OH_AVCodecBufferAttr attr;
        bool audioIsEnd = false;
        int audioFrame = 0;
        const char* mimeType = nullptr;
        fd_ = open(fileName, O_RDONLY);
        int64_t size = GetFileSize(fileName);
        cout << fileName << "----------------------" << fd_ << "---------" << size << endl;
        source = OH_AVSource_CreateWithFD(fd_, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount_));
        if (trackCount_ != 1) {
            ASSERT_EQ(trackCount_, CHANNELS);
        }
        for (int32_t index = 0; index < trackCount_; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        int aKeyCount = 0;
        while (!audioIsEnd) {
            for (int32_t index = 0; index < trackCount_; index++) {
                trackFormat = OH_AVSource_GetTrackFormat(source, index);
                ASSERT_NE(trackFormat, nullptr);
                ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
                if (audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) {
                    continue;
                }
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
                if (tarckType == MEDIA_TYPE_AUD) {
                    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
                    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_RAW));
                    SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
                }
                OH_AVFormat_Destroy(trackFormat);
                trackFormat = nullptr;
            }
        }
        close(fd_);
        fd_ = -1;
    }

    void CheckSeekMode(seekInfo seekInfo)
    {
        int tarckType = 0;
        OH_AVCodecBufferAttr attr;
        fd_ = open(seekInfo.fileName, O_RDONLY);
        int64_t size = GetFileSize(seekInfo.fileName);
        cout << seekInfo.fileName << "-------" << fd_ << "-------" << size << endl;
        source = OH_AVSource_CreateWithFD(fd_, 0, size);
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_NE(sourceFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount_));
        cout << "trackCount_----" << trackCount_ << endl;
        for (int32_t index = 0; index < trackCount_; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        for (int32_t index = 0; index < trackCount_; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
            bool readEnd = false;
            int32_t frameNum = 0;
            while (!readEnd) {
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
                if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    readEnd = true;
                    break;
                }
                frameNum++;
            }
            if (tarckType == MEDIA_TYPE_AUD) {
                cout << "frameNum---" << frameNum << endl;
                ASSERT_EQ(seekInfo.audioCount, frameNum);
            }
        }
        close(fd_);
        fd_ = -1;
    }

    void CheckSourceFormat(const char *fileName, int32_t sampleRate, int32_t channels, int32_t bitRate)
    {
        const char *file = fileName;
        fd_ = open(file, O_RDONLY);
        int64_t size = GetFileSize(file);
        source = OH_AVSource_CreateWithFD(fd_, 0, size);
        ASSERT_NE(source, nullptr);
        trackFormat = OH_AVSource_GetTrackFormat(source, 0);
        ASSERT_NE(trackFormat, nullptr);

        int32_t type = 0;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
        ASSERT_EQ(type, MEDIA_TYPE_AUD);

        int32_t sr = 0;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
        ASSERT_EQ(sr, sampleRate);

        int32_t cc = 0;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
        ASSERT_EQ(cc, channels);
        int64_t br = 0;
        if (fileName == INP_DIR_4 || fileName == INP_DIR_6) {
            ASSERT_FALSE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
        } else {
            ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
            ASSERT_EQ(br, bitRate);
        }
    }

    void SetAudioValue(OH_AVCodecBufferAttr attr, bool &audioIsEnd, int &audioFrame, int &aKeyCount)
    {
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            audioIsEnd = true;
            cout << audioFrame << "    audio is end !!!!!!!!!!!!!!!" << endl;
        } else {
            audioFrame++;
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                aKeyCount++;
            }
        }
    }

    int64_t GetFileSize(const char *fileName)
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

protected:
    shared_ptr<Media::Meta> meta_ = nullptr;
    shared_ptr<CodecPlugin> decoder_ = nullptr;
    int fd_ = -1;
    int32_t trackCount_;
    OH_AVFormat *trackFormat = nullptr;
    OH_AVFormat *sourceFormat = nullptr;
    OH_AVDemuxer *demuxer = nullptr;
    OH_AVMemory *memory = nullptr;
    OH_AVSource *source = nullptr;
};

void RawDecoderUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void RawDecoderUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void RawDecoderUnitTest::SetUp(void)
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName("OH.Media.Codec.Decoder.Audio.Raw");
    if (plugin == nullptr) {
        cout << "not support raw" << endl;
        return;
    }
    decoder_ = reinterpret_pointer_cast<CodecPlugin>(plugin);
    decoder_->SetDataCallback(this);
    meta_ = make_shared<Media::Meta>();
    memory = OH_AVMemory_Create(WIDTH * HEIGHT);
}

void RawDecoderUnitTest::TearDown(void)
{
    if (decoder_ != nullptr) {
        decoder_ = nullptr;
    }
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }

    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }

    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
}

HWTEST_F(RawDecoderUnitTest, InputBuffer_Nullptr_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    decoder_->SetParameter(meta_);
    EXPECT_EQ(Status::OK, decoder_->Init());
    EXPECT_EQ(Status::OK, decoder_->Prepare());
    EXPECT_EQ(Status::OK, decoder_->Start());
    EXPECT_EQ(Status::OK, decoder_->Stop()); // set inputBuffer_ nullptr
    EXPECT_EQ(Status::OK, decoder_->Stop());
    EXPECT_EQ(Status::OK, decoder_->Flush());
    EXPECT_EQ(Status::OK, decoder_->Reset());
    EXPECT_EQ(Status::OK, decoder_->Release());
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE); // no AUDIO_CHANNEL_COUNT
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS); // no AUDIO_SAMPLE_RATE
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE); // no AUDIO_SAMPLE_FORMAT
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetMetaData_004, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT); // no AUDIO_RAW_SAMPLE_FORMAT
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(UNSUPPORTED_CHANNELS_MIN); // check channels fail
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(UNSUPPORTED_SAMPLE_RATE); // check sampleRate fail
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(INVALID_WIDTH); // check sampleFormat fail
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_004, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(INVALID_WIDTH); // check sampleFormat fail
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, CheckFormat_005, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(UNSUPPORTED_CHANNELS_MAX); // check channels fail
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, SetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(MAX_INPUT_SIZE);
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, SetParameter_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(MIN_INPUT_SIZE);
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, SetParameter_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(UNSUPPORTED_MAX_INPUT_SIZE);
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
}

HWTEST_F(RawDecoderUnitTest, GetFormatBytes_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(SAMPLE_FORMAT);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(RAW_SAMPLE_FORMAT);
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    EXPECT_EQ(Status::OK, decoder_->Start());
    std::shared_ptr<Meta> config = std::make_shared<Meta>();
    EXPECT_EQ(Status::OK, decoder_->GetParameter(config));
}

HWTEST_F(RawDecoderUnitTest, GetFormatBytes_002, TestSize.Level1)
{
    EXPECT_NE(Status::OK, decoder_->SetParameter(meta_));
    EXPECT_EQ(Status::OK, decoder_->Start());
    std::shared_ptr<Meta> config = std::make_shared<Meta>();
    EXPECT_EQ(Status::OK, decoder_->GetParameter(config));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F64BE); // SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 64000; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(64000); // size % bytesSize == 0 && size > maxInputSize_
    EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    EXPECT_NE(Status::OK, decoder_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_002, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F64BE); // SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 9; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(9); // size % bytesSize != 0
    EXPECT_EQ(Status::ERROR_UNKNOWN, decoder_->QueueInputBuffer(inputBuffer));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_003, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32BE); // not SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 16000; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(16000); // size % bytesSize == 0
    EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    EXPECT_NE(Status::OK, decoder_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_004, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32LE);
    meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_F32BE); // not SAMPLE_F64BE
    EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));
    int32_t oneFrameSize = 16; // capacity
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    inputBuffer->memory_->SetSize(16); // size % bytesSize == 0 && size < maxInputSize_
    EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));
    shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, oneFrameSize);
    EXPECT_EQ(Status::OK, decoder_->QueueOutputBuffer(outputBuffer));
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_005, TestSize.Level1)
{
    for (auto &elem : inputFormatPathMap) {
        AudioSampleFormat inputFormat = elem.first;
        if (inputFormat == AudioSampleFormat::SAMPLE_BLURAY || inputFormat == AudioSampleFormat::SAMPLE_DVD) {
            continue;
        }
        auto pair = elem.second;
        std::string inputFilePath = pair.first;
        std::string srcOutName = pair.second;

        for (auto outElem : outputFormats) {
            AudioSampleFormat outputFormat = outElem.first;
            std::string destName = outElem.second;
            SetupParameter(inputFormat, outputFormat);
            EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));

            std::ifstream inputFile;
            inputFile.open(inputFilePath, std::ios::binary | std::ios::ate);
            int32_t fileSize = static_cast<int32_t>(inputFile.tellg());
            inputFile.seekg(0, std::ios::beg);

            auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, fileSize);
            inputBuffer->memory_->SetSize(fileSize);
            inputFile.read(reinterpret_cast<char*>(inputBuffer->memory_->GetAddr()), fileSize);
            EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));

            std::ofstream outputFile;
            std::string outputPath = OUTPUT_PREFIX + srcOutName + destName + OUTPUT_SUFFIX;
            outputFile.open(outputPath, std::ios::out | std::ios::binary);

            int32_t srcBytesSize = GetFormatBytes(inputFormat);
            int32_t destBytesSize = GetFormatBytes(outputFormat);
            int32_t totalSamples = fileSize / (CHANNELS * srcBytesSize);
            int32_t outputSize = totalSamples * CHANNELS * destBytesSize;
            shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputSize);
            shared_ptr<AVBuffer> emptyOutputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputSize);

            Status ret = Status::ERROR_AGAIN;
            int32_t pos = 0;
            while (ret == Status::ERROR_AGAIN) {
                ret = decoder_->QueueOutputBuffer(emptyOutputBuffer);
                outputBuffer->memory_->Write(emptyOutputBuffer->memory_->GetAddr(),
                                            emptyOutputBuffer->memory_->GetSize(), pos);
                pos += emptyOutputBuffer->memory_->GetSize();
            }
            EXPECT_EQ(Status::OK, ret);
            EXPECT_EQ(outputSize, outputBuffer->memory_->GetSize());
            outputFile.write(reinterpret_cast<char*>(outputBuffer->memory_->GetAddr()),
                    outputBuffer->memory_->GetSize());

            inputFile.close();
            outputFile.close();
        }
    }
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_006, TestSize.Level1)
{
    auto func = [this](AudioSampleFormat inputFormat, int32_t frameSize, int32_t offset) {
        auto pair = inputFormatPathMap.at(inputFormat);
        std::string inputFilePath = pair.first;
        std::string srcOutName = pair.second;

        for (auto outElem : outputFormats) {
            AudioSampleFormat outputFormat = outElem.first;
            std::string destName = outElem.second;
            SetupParameter(inputFormat, outputFormat);
            EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));

            std::ifstream inputFile;
            inputFile.open(inputFilePath, std::ios::binary | std::ios::ate);
            int32_t fileSize = static_cast<int32_t>(inputFile.tellg());
            inputFile.seekg(0, std::ios::beg);

            int32_t frames = fileSize / frameSize;

            std::ofstream outputFile;
            std::string outputPath = OUTPUT_PREFIX + srcOutName + destName + OUTPUT_SUFFIX;
            outputFile.open(outputPath, std::ios::out | std::ios::binary);

            int32_t srcBytesSize = GetFormatBytes(inputFormat);
            int32_t destBytesSize = GetFormatBytes(outputFormat);
            int32_t outputSize = (fileSize - frames * offset) * destBytesSize / srcBytesSize;
            auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
            shared_ptr<AVBuffer> outputBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputSize);
            int32_t pos = 0;
            for (int32_t i = 0; i < frames; i++) {
                shared_ptr<AVBuffer> inputBuffer = AVBuffer::CreateAVBuffer(avAllocator, fileSize);
                inputBuffer->memory_->SetSize(frameSize);
                inputFile.read(reinterpret_cast<char*>(inputBuffer->memory_->GetAddr()), frameSize);
                EXPECT_EQ(Status::OK, decoder_->QueueInputBuffer(inputBuffer));

                shared_ptr<AVBuffer> emptyOutputBuffer = AVBuffer::CreateAVBuffer(avAllocator,
                    (frameSize - offset) * destBytesSize / srcBytesSize);
                Status ret = decoder_->QueueOutputBuffer(emptyOutputBuffer);
                outputBuffer->memory_->Write(emptyOutputBuffer->memory_->GetAddr(),
                                            emptyOutputBuffer->memory_->GetSize(), pos);
                pos += emptyOutputBuffer->memory_->GetSize();
                EXPECT_EQ(Status::OK, ret);
            }
            outputFile.write(reinterpret_cast<char*>(outputBuffer->memory_->GetAddr()),
                    outputBuffer->memory_->GetSize());

            inputFile.close();
            outputFile.close();
        }
    };

    func(AudioSampleFormat::SAMPLE_BLURAY, BLURAY_PACKET_SIZE, BLURAY_PACKET_OFFSET);

    func(AudioSampleFormat::SAMPLE_DVD, DVD_PACKET_SIZE, DVD_PACKET_OFFSET);
}

HWTEST_F(RawDecoderUnitTest, QueueInputBuffer_007, TestSize.Level1)
{
    for (auto &elem : planarInputFormatPathMap) {
        AudioSampleFormat inputFormat = elem.first;
        auto pair = elem.second;
        std::string inputFilePath = pair.first;
        std::string srcOutName = pair.second;

        for (auto outElem : outputFormats) {
            AudioSampleFormat outputFormat = outElem.first;
            std::string destName = outElem.second;
            SetupParameter(inputFormat, outputFormat);
            EXPECT_EQ(Status::OK, decoder_->SetParameter(meta_));

            std::ifstream inputFile;
            inputFile.open(inputFilePath, std::ios::binary);
            ASSERT_TRUE(inputFile.is_open());

            std::ofstream outputFile;
            std::string outputPath = OUTPUT_PREFIX + srcOutName + destName + OUTPUT_SUFFIX;
            outputFile.open(outputPath, std::ios::out | std::ios::binary);
            int32_t srcBytesSize = GetFormatBytes(inputFormat);
            int32_t destBytesSize = GetFormatBytes(outputFormat);
            
            std::vector<uint8_t> buffer(BYTES_8);
            inputFile.read(reinterpret_cast<char*>(buffer.data()), BYTES_8);
            buffer.resize(BYTES_4);
            inputFile.read(reinterpret_cast<char*>(buffer.data()), BYTES_4);
            int32_t extradataLen;
            memcpy_s(&extradataLen, sizeof(int32_t), buffer.data(), sizeof(int32_t));
            buffer.resize(extradataLen);
            inputFile.read(reinterpret_cast<char*>(buffer.data()), extradataLen);

            ProcessPackages(inputFile, outputFile, srcBytesSize, destBytesSize);

            inputFile.close();
            outputFile.close();
        }
    }
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_001, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_1);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_002, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_2);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_003, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_3);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_004, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_4);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_005, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_5);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_006, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_6);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_007, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_7);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_008, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_8);
}

HWTEST_F(RawDecoderUnitTest, DemuxerResult_009, TestSize.Level1)
{
    DemuxerResultRaw(INP_DIR_9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_001, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 418};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 400};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 24};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 210};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 191};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 375};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 469};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_002, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 0, 0, 418};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 0, 0, 400};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 0, 0, 24};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 0, 0, 210};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 0, 0, 191};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 0, 0, 375};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 0, 0, 469};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_003, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 0, 0, 418};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 0, 0, 400};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 0, 0, 24};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 0, 0, 210};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 0, 0, 191};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 0, 0, 375};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 0, 0, 469};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_004, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 418};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 400};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 24};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 210};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 94};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 191};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 375};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 469};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_005, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 300000, 0, 391};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 300000, 0, 78};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 300000, 0, 340};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 300000, 0, 20};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 300000, 0, 196};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 300000, 0, 79};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 300000, 0, 163};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 300000, 0, 319};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 300000, 0, 441};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_006, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 393};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 82};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 340};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 21};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 197};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 80};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 164};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 319};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 300000, 0, 441};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_007, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 377};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 74};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 300};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 19};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 189};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 71};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 145};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 282};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 500000, 10, 422};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_008, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 500000, 10, 375};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 500000, 10, 70};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 500000, 10, 300};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 500000, 10, 18};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 500000, 10, 188};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 500000, 10, 70};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 500000, 10, 144};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 500000, 10, 282};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 500000, 10, 422};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSeekMode_009, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 377};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 74};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 300};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 19};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 189};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 71};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 145};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 282};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 500000, 10, 422};
    CheckSeekMode(fileTest9);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_001, TestSize.Level1)
{
    const char *file = INP_DIR_1;
    int32_t sampleRate = 48000;
    int32_t bitRate = 3072000;
    int32_t channels = 1;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_002, TestSize.Level1)
{
    const char *file = INP_DIR_2;
    int32_t sampleRate = 48000;
    int32_t bitRate = 6144000;
    int32_t channels = 2;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_003, TestSize.Level1)
{
    const char *file = INP_DIR_3;
    int32_t sampleRate = 48000;
    int32_t bitRate = 1536000;
    int32_t channels = 2;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_004, TestSize.Level1)
{
    const char *file = INP_DIR_4;
    int32_t sampleRate = 48000;
    int32_t bitRate = 6144000;
    int32_t channels = 2;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_005, TestSize.Level1)
{
    const char *file = INP_DIR_5;
    int32_t sampleRate = 48000;
    int32_t bitRate = 3072000;
    int32_t channels = 1;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_006, TestSize.Level1)
{
    const char *file = INP_DIR_6;
    int32_t sampleRate = 48000;
    int32_t bitRate = 6144000;
    int32_t channels = 2;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_007, TestSize.Level1)
{
    const char *file = INP_DIR_7;
    int32_t sampleRate = 48000;
    int32_t bitRate = 1536000;
    int32_t channels = 2;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_008, TestSize.Level1)
{
    const char *file = INP_DIR_8;
    int32_t sampleRate = 48000;
    int32_t bitRate = 6144000;
    int32_t channels = 2;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}

HWTEST_F(RawDecoderUnitTest, CheekSourceFormat_009, TestSize.Level1)
{
    const char *file = INP_DIR_9;
    int32_t sampleRate = 48000;
    int32_t bitRate = 3072000;
    int32_t channels = 1;
    CheckSourceFormat(file, sampleRate, channels, bitRate);
}
} // namespace Plugins
}
}