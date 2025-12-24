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

#include "avcodec_codec_name.h"
#include "avcodec_common.h"
#include "plugin/codec_plugin.h"
#include "plugin/plugin_manager_v2.h"
#include <array>
#include <cstdint>
#include <fstream>
#include <gtest/gtest.h>

#include "native_avcodec_audiocodec.h"
#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"
#include "native_avcapability.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace {
constexpr size_t FIELDSIZE = 8;
const string CODEC_TWINVQ_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_TWINVQ_NAME);
constexpr int32_t TWINVQ_SAMPLE_RATE = 8000;
constexpr int32_t TWINVQ_CHANNEL_COUNT = 1;
constexpr size_t TWINVQ_AVBUFFER_SIZE = 640; // 320 samples * 2 bytes
constexpr int32_t TWINVQ_MAX_INPUT_SIZE = 320;
constexpr int32_t TWINVQ_OUTPUT_BUFFER_SIZE = 3528;
const string TWINVQ_FILE_PATH = "/data/test/media/twinvq_8000_mono.dat";
const string TWINVQ_OUTPUT_FILE_PATH = "/data/test/media/twinvq_8000_1.pcm";
constexpr int PROBE = 2;

} // namespace

class TWINVQUnitTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override;
    void TearDown() override;

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
    shared_ptr<CodecPlugin> CreatePlugin();
    void DestroyPlugin();
    shared_ptr<Media::Meta> CreateMeta();
    shared_ptr<AVBuffer> CreateAVBuffer(size_t capacity, size_t size);
    void OutHelper(size_t &produced);
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    shared_ptr<Media::Meta> meta_ = nullptr;
};

void TWINVQUnitTest::SetUpTestCase(void) {}

void TWINVQUnitTest::TearDownTestCase(void) {}

void TWINVQUnitTest::SetUp(void)
{
    plugin_ = CreatePlugin();
    ASSERT_NE(plugin_, nullptr);
    meta_ = CreateMeta();
    ASSERT_NE(meta_, nullptr);
}

void TWINVQUnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
    }
}

shared_ptr<CodecPlugin> TWINVQUnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_TWINVQ_DEC_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<Media::Meta> TWINVQUnitTest::CreateMeta()
{
    auto meta = make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(TWINVQ_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(TWINVQ_SAMPLE_RATE);
    // extradata = [channels - 1, bitrate, sample rate]
    int32_t extraDataSize = 0;
    int32_t channels = 0;
    int32_t sampleRate = 0;
    std::vector<uint8_t> extraData;
    std::ifstream inputFile;
    inputFile.open(TWINVQ_FILE_PATH, std::ios::binary);
    if (!inputFile.is_open()) {
        std::cout << "Fatal: inputFile is null" << std::endl;
        return nullptr;
    }
    inputFile.read(reinterpret_cast<char *>(&channels), sizeof(int32_t));
    inputFile.read(reinterpret_cast<char *>(&sampleRate), sizeof(int32_t));
    inputFile.read(reinterpret_cast<char*>(&extraDataSize), sizeof(extraDataSize));
    if (inputFile.gcount() != sizeof(extraDataSize)) {
        std::cout << "Fatal: read extraDataSize fail" << std::endl;
        return nullptr;
    }
    if (extraDataSize > 0) {
        extraData.resize(extraDataSize);
        inputFile.read(reinterpret_cast<char*>(extraData.data()), extraDataSize);
        if (inputFile.gcount() != extraDataSize) {
            std::cout << "Fatal: read extraData fail" << std::endl;
            return nullptr;
        }
        meta->Set<Tag::MEDIA_CODEC_CONFIG>(extraData);
    }
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(channels);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(sampleRate);
    inputFile.close();

    return meta;
}

shared_ptr<AVBuffer> TWINVQUnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    avBuffer->memory_->SetSize(size);
    return avBuffer;
}
void TWINVQUnitTest::OutHelper(size_t &produced)
{
    for (int probe = 0; probe < PROBE; ++probe) {
        auto out = CreateAVBuffer(TWINVQ_OUTPUT_BUFFER_SIZE, 0);
        auto ost = plugin_->QueueOutputBuffer(out);
        if ((ost == Status::OK || ost == Status::ERROR_NOT_ENOUGH_DATA || ost == Status::ERROR_AGAIN)) {
            if (out->memory_->GetSize() > 0) {
                produced++;
            }
        } else if (ost == Status::END_OF_STREAM) {
            break;
        } else {
            std::cout << "QueueOutputBuffer returned status " << static_cast<int>(ost) << std::endl;
        }
    }
}
HWTEST_F(TWINVQUnitTest, SetParameter_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);

    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

HWTEST_F(TWINVQUnitTest, SetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(TWINVQ_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(TWINVQ_SAMPLE_RATE);
}

HWTEST_F(TWINVQUnitTest, SetParameter_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(TWINVQ_CHANNEL_COUNT);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(TWINVQ_SAMPLE_RATE);
}

HWTEST_F(TWINVQUnitTest, Lifecycle_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->Release(), Status::OK);
}

HWTEST_F(TWINVQUnitTest, Eos_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(1024, 0);
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    auto outputBuffer = CreateAVBuffer(TWINVQ_MAX_INPUT_SIZE * 4, 0);
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(TWINVQUnitTest, GetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(TWINVQ_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, TWINVQ_CHANNEL_COUNT);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_EQ(sampleRate, TWINVQ_SAMPLE_RATE);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

HWTEST_F(TWINVQUnitTest, GetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    int32_t defaultInputSize = 320;
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    int32_t usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize / 2);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
    usrInputSize = 0;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(defaultInputSize * 2);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(usrInputSize));
    ASSERT_EQ(defaultInputSize, usrInputSize);
}

HWTEST_F(TWINVQUnitTest, Decode_With_Invalid_File_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(TWINVQ_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    // This causes ffmpeg complaining 'Packet is too small'
    const int invalidSize = 16;
    auto inputBuffer = CreateAVBuffer(invalidSize, invalidSize);
    std::fill_n(inputBuffer->memory_->GetAddr(), 0xAB, invalidSize);
    auto st = plugin_->QueueInputBuffer(inputBuffer);
    ASSERT_NE(st, Status::OK);
    auto outputBuffer = CreateAVBuffer(TWINVQ_MAX_INPUT_SIZE, 0);
    Status status = plugin_->QueueOutputBuffer(outputBuffer);
    ASSERT_TRUE(status != Status::OK) << "Unexpected status=" << static_cast<int>(status);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(TWINVQUnitTest, Decode_With_Embedded_RealBlocks_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    size_t pushed = 0;
    size_t produced = 0;
    std::ifstream inputFile;
    inputFile.open(TWINVQ_FILE_PATH, std::ios::binary);
    ASSERT_TRUE(inputFile.is_open());

    int32_t channels = 0;
    int32_t sampleRate = 0;
    int32_t extradatalen = 0;
    inputFile.read(reinterpret_cast<char *>(&channels), sizeof(int32_t));
    inputFile.read(reinterpret_cast<char *>(&sampleRate), sizeof(int32_t));
    inputFile.read(reinterpret_cast<char *>(&extradatalen), sizeof(int32_t));
    std::vector<uint8_t> buffer(extradatalen);
    inputFile.read(reinterpret_cast<char *>(buffer.data()), extradatalen);

    while (!inputFile.eof()) {
        buffer.resize(FIELDSIZE);
        inputFile.read(reinterpret_cast<char*>(buffer.data()), FIELDSIZE);
        int64_t packageSize;
        memcpy_s(&packageSize, sizeof(int64_t), buffer.data(), sizeof(int64_t));
        inputFile.read(reinterpret_cast<char*>(buffer.data()), FIELDSIZE);
        auto in = CreateAVBuffer(TWINVQ_AVBUFFER_SIZE, packageSize);
        inputFile.read(reinterpret_cast<char*>(in->memory_->GetAddr()), packageSize);
        auto st = plugin_->QueueInputBuffer(in);
        ASSERT_EQ(st, Status::OK);
        pushed++;
        OutHelper(produced);
    }
    inputFile.close();

    auto eos = CreateAVBuffer(0, 0);
    eos->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(eos), Status::OK);
    for (int tries = 0; tries < 16; ++tries) {
        auto out = CreateAVBuffer(TWINVQ_OUTPUT_BUFFER_SIZE, 0);
        auto s = plugin_->QueueOutputBuffer(out);
        if (s == Status::END_OF_STREAM) {
            break;
        }
        if ((s == Status::OK || s == Status::ERROR_NOT_ENOUGH_DATA || s == Status::ERROR_AGAIN) &&
            out->memory_->GetSize() > 0) {
            produced++;
        }
    }
    ASSERT_GT(pushed, 0u);
    ASSERT_GT(produced, 0u);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(TWINVQUnitTest, Decode_With_Embedded_RealBlocks_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(TWINVQ_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);

    std::ifstream inputFile;
    inputFile.open(TWINVQ_FILE_PATH, std::ios::binary);
    ASSERT_TRUE(inputFile.is_open());
    std::vector<uint8_t> buffer(FIELDSIZE);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), FIELDSIZE);
    buffer.resize(4);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), 4);
    int32_t extradataLen;
    memcpy_s(&extradataLen, sizeof(int32_t), buffer.data(), sizeof(int32_t));
    buffer.resize(extradataLen);
    inputFile.read(reinterpret_cast<char*>(buffer.data()), extradataLen);

    std::ofstream outputFile;
    outputFile.open(TWINVQ_OUTPUT_FILE_PATH, std::ios::out | std::ios::binary);
    auto outBuffer = CreateAVBuffer(80 * 1024, 0);
    int32_t pos = 0;
    while (!inputFile.eof()) {
        buffer.resize(FIELDSIZE);
        inputFile.read(reinterpret_cast<char*>(buffer.data()), FIELDSIZE);
        int64_t packageSize;
        memcpy_s(&packageSize, sizeof(int64_t), buffer.data(), sizeof(int64_t));
        inputFile.read(reinterpret_cast<char*>(buffer.data()), FIELDSIZE);
        auto in = CreateAVBuffer(TWINVQ_AVBUFFER_SIZE, packageSize);
        inputFile.read(reinterpret_cast<char*>(in->memory_->GetAddr()), packageSize);
        std::cout << "package size: " << packageSize << " in size: " << in->memory_->GetSize() << std::endl;
        auto st = plugin_->QueueInputBuffer(in);
        ASSERT_EQ(st, Status::OK);

        for (int probe = 0; probe < PROBE; ++probe) {
            auto tempOutBuf = CreateAVBuffer(TWINVQ_OUTPUT_BUFFER_SIZE, 0);
            Status ret = plugin_->QueueOutputBuffer(tempOutBuf);
            if (ret == Status::OK || ret == Status::ERROR_NOT_ENOUGH_DATA || ret == Status::ERROR_AGAIN) {
                std::cout << "buffer out: " << tempOutBuf->memory_->GetSize() << std::endl;
                outBuffer->memory_->Write(tempOutBuf->memory_->GetAddr(), tempOutBuf->memory_->GetSize(), pos);
                pos += tempOutBuf->memory_->GetSize();
            }
        }
    }
    std::cout << "outputBuffer size: " << outBuffer->memory_->GetSize() << std::endl;
    outputFile.write(reinterpret_cast<char*>(outBuffer->memory_->GetAddr()), outBuffer->memory_->GetSize());
    inputFile.close();
    outputFile.close();
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS
