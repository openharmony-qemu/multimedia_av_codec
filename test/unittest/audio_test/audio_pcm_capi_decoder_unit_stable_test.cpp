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
// {sampleFormat, {filePath, outputName}}
const std::unordered_map<AudioSampleFormat, std::pair<std::string, std::string>> inputFormatPathMap = {
    {AudioSampleFormat::SAMPLE_S8, std::make_pair("/data/test/media/pcm_s8.pcm", "s8_")},
    {AudioSampleFormat::SAMPLE_F64LE, std::make_pair("/data/test/media/pcm_f64le.pcm", "f64le_")},
    {AudioSampleFormat::SAMPLE_S64LE, std::make_pair("/data/test/media/pcm_s64le.pcm", "s64le_")},
    {AudioSampleFormat::SAMPLE_S8P, std::make_pair("/data/test/media/pcm_s8_planar.pcm", "s8p_")},
    {AudioSampleFormat::SAMPLE_S16LEP, std::make_pair("/data/test/media/pcm_s16le_planar.pcm", "s16lep_")},
    {AudioSampleFormat::SAMPLE_S16BEP, std::make_pair("/data/test/media/pcm_s16be_planar.pcm", "s16bep_")},
    {AudioSampleFormat::SAMPLE_S24LEP, std::make_pair("/data/test/media/pcm_s24le_planar.pcm", "s24lep_")},
    {AudioSampleFormat::SAMPLE_S32LEP, std::make_pair("/data/test/media/pcm_s32le_planar.pcm", "s32lep_")},
    {AudioSampleFormat::SAMPLE_DVD, std::make_pair("/data/test/media/pcm_dvd_16bit.pcm", "dvd_16bit_")},
    {AudioSampleFormat::SAMPLE_BLURAY, std::make_pair("/data/test/media/pcm_bluray_16bit.pcm", "bluray_16bit_")},
};
// {sampleFormat, outputName}
const std::vector<pair<AudioSampleFormat, std::string>> outputFormats = {
    std::make_pair(AudioSampleFormat::SAMPLE_U8, "to_u8"),
    std::make_pair(AudioSampleFormat::SAMPLE_S16LE, "to_s16le"),
    std::make_pair(AudioSampleFormat::SAMPLE_S24LE, "to_s24le"),
    std::make_pair(AudioSampleFormat::SAMPLE_S32LE, "to_s32le"),
    std::make_pair(AudioSampleFormat::SAMPLE_F32LE, "to_f32le")
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
        meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNELS);
        meta_->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48000);
        meta_->Set<Tag::AUDIO_RAW_SAMPLE_FORMAT>(inputFormat);
        meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(AUDIO_BITS_16);
        meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(outputFormat);
    }

protected:
    shared_ptr<Media::Meta> meta_ = nullptr;
    shared_ptr<CodecPlugin> decoder_ = nullptr;
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
}

void RawDecoderUnitTest::TearDown(void)
{
    if (decoder_ != nullptr) {
        decoder_ = nullptr;
    }
}

HWTEST_F(RawDecoderUnitTest, StableTest, TestSize.Level1)
{
    for (int count = 0; count < 5; count++) {
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
}
} // namespace Plugins
}
}