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
const string TEST_FILE_PATH = "/data/test/media/";
constexpr int32_t ADPCM_CHANNEL_COUNT = 1;
constexpr int32_t ADPCM_SAMPLE_RATE = 16000;
constexpr int32_t ADPCM_BITS_PER_SAMPLE = 4;

class AudioAdpcmDecoderUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void)
    {
    }
    static void TearDownTestCase(void)
    {
    }
    void SetUp() override;
    void TearDown() override;
    bool ReadBuffer(std::shared_ptr<AVBuffer> buffer);
    Status ProcessDecoder(const string &inputFile);
    void InitDecoder(std::string name);
    void ReleaseDecoder();

    void OnInputBufferDone(const std::shared_ptr<AVBuffer> &inputBuffer) override
    {
        (void)inputBuffer;
    }

    void OnOutputBufferDone(const std::shared_ptr<AVBuffer> &outputBuffer) override
    {
        (void)outputBuffer;
    }

    void OnEvent(const std::shared_ptr<PluginEvent> event) override
    {
        (void)event;
    }

protected:
    int32_t outputBufferCapacity_;
    int32_t inputBufferCapacity_;
    std::ifstream inputFile_;
    std::ofstream outputFile_;
    std::shared_ptr<Meta> meta_ = nullptr;
    std::shared_ptr<CodecPlugin> decoder_ = nullptr;
    int32_t outBytes_ = 0;
};

void AudioAdpcmDecoderUnitTest::SetUp(void)
{
}

void AudioAdpcmDecoderUnitTest::TearDown(void)
{
}

bool AudioAdpcmDecoderUnitTest::ReadBuffer(std::shared_ptr<AVBuffer> buffer)
{
    int64_t size;
    int64_t pts;
    inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_.eof() || inputFile_.gcount() == 0 || size == 0) {
        buffer->memory_->SetSize(1);
        buffer->flag_ = 1;
        cout << "Set EOS" << endl;
        decoder_->QueueInputBuffer(buffer);
        return false;
    }

    if (inputFile_.gcount() != sizeof(size)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }

    inputFile_.read(reinterpret_cast<char *>(&buffer->pts_), sizeof(buffer->pts_));
    if (inputFile_.gcount() != sizeof(pts)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }

    inputFile_.read(reinterpret_cast<char *>(buffer->memory_->GetAddr()), size);
    buffer->memory_->SetSize(size);
    if (inputFile_.gcount() != size) {
        cout << "Fatal: read buffer fail" << endl;
        return false;
    }
    cout << "read adpcm buffer size:" << size << endl;
    decoder_->QueueInputBuffer(buffer);

    return true;
}

Status AudioAdpcmDecoderUnitTest::ProcessDecoder(const string &inputFile)
{
    inputFile_.open(TEST_FILE_PATH + inputFile, ios::binary);
    if (!inputFile_.is_open()) {
        cout << "input file open fail" << endl;
        return Status::ERROR_UNKNOWN;
    }
    outputFile_.open(TEST_FILE_PATH + "adpcm_out.pcm", ios::binary);
    if (!outputFile_.is_open()) {
        cout << "output file open fail" << endl;
        return Status::ERROR_UNKNOWN;
    }
    Status ret = Status::OK;
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto inBuffer = AVBuffer::CreateAVBuffer(avAllocator, inputBufferCapacity_);
    auto outBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputBufferCapacity_);
    bool isContinue = true;
    while (isContinue) {
        isContinue = ReadBuffer(inBuffer);
        Status outRet;
        do {
            outRet = decoder_->QueueOutputBuffer(outBuffer);
            if (outRet == Status::OK || outRet == Status::ERROR_AGAIN) {
                outputFile_.write(reinterpret_cast<char *>(outBuffer->memory_->GetAddr()),
                                  outBuffer->memory_->GetSize());
                outBytes_ += outBuffer->memory_->GetSize();
            }
        } while (outRet == Status::ERROR_AGAIN);
    }
    return ret;
}

void AudioAdpcmDecoderUnitTest::InitDecoder(string name)
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(name);
    ASSERT_NE(plugin, nullptr);
    decoder_ = reinterpret_pointer_cast<CodecPlugin>(plugin);
    decoder_->SetDataCallback(this);
    meta_ = make_shared<Meta>();
}

void AudioAdpcmDecoderUnitTest::ReleaseDecoder()
{
    if (decoder_ != nullptr) {
        ASSERT_EQ(decoder_->Release(), Status::OK);
    }
    if (inputFile_.is_open()) {
        inputFile_.close();
    }
    if (outputFile_.is_open()) {
        outputFile_.close();
    }
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_missing_format, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.MS");
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_NE(decoder_->SetParameter(meta_), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ms, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.MS");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_qt, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.QT");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_wav, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.WAV");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    meta_->Set<Tag::AUDIO_BLOCK_ALIGN>(1);
    meta_->Set<Tag::AUDIO_BITS_PER_CODED_SAMPLE>(ADPCM_BITS_PER_SAMPLE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_dk3, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.DK3");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_dk4, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.DK4");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ws, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.WS");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_smjpeg, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.SMJPEG");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_dat4, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.DAT4");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_adx, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.ADX");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_afc, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.AFC");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_aica, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.AICA");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ct, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.CT");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_amv, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.AMV");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_apc, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.APC");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_iss, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.ISS");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_oki, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.OKI");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_ima_rad, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.IMA.RAD");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_psx, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.PSX");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_sbpro2, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.SBPRO2");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_sbpro3, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.SBPRO3");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_sbpro4, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.SBPRO4");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_thp, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.THP");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_thp_le, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.THP.LE");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_xa, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.XA");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

HWTEST_F(AudioAdpcmDecoderUnitTest, decode_yamaha, TestSize.Level0)
{
    InitDecoder("OH.Media.Codec.Decoder.Audio.ADPCM.YAMAHA");
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(ADPCM_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(ADPCM_SAMPLE_RATE);
    meta_->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    ASSERT_EQ(decoder_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(decoder_->Start(), Status::OK);
}

}
}
}