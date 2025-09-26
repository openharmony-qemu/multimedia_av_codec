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
#include "avcodec_common.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace {
const string CODEC_EAC3_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_EAC3_NAME);
constexpr int32_t EAC3_SAMPLE_RATE = 44100;
constexpr int32_t EAC3_CHANNEL_COUNT = 16;
constexpr int64_t EAC3_BIT_RATE = 192000;
constexpr int32_t EAC3_MAX_INPUT_SIZE = 8192;
constexpr int32_t EAC3_ATTEMPT_COUNT_1 = 8;
constexpr int32_t EAC3_ATTEMPT_COUNT_2 = 64;
} // namespace

class EAC3UnitTest : public testing::Test, public DataCallback {
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
    static constexpr size_t eaC3HeaderSize = 4; // syncword + header size
    static constexpr std::streamoff eaC3SyncwordBackOffset = 2; // offset to rewind to syncword
    static constexpr size_t maxFrameSize = 4096;
    shared_ptr<CodecPlugin> CreatePlugin();
    void DestroyPlugin();
    shared_ptr<Media::Meta> CreateMeta();
    shared_ptr<AVBuffer> CreateAVBuffer(size_t capacity, size_t size);
    bool ReadOneFrame(ifstream &file, shared_ptr<AVBuffer> &buffer);
    void ProcessInputFrames(ifstream &file, int &inputFrameCount, int &outputFrameCount, bool &sentEos);
    void ProcessOutputFrames(int &outputFrameCount);
    shared_ptr<CodecPlugin> plugin_ = nullptr;
    shared_ptr<Media::Meta> meta_ = nullptr;
};

void EAC3UnitTest::SetUpTestCase(void)
{
}

void EAC3UnitTest::TearDownTestCase(void)
{
}

void EAC3UnitTest::SetUp(void)
{
    plugin_ = CreatePlugin();
    ASSERT_NE(plugin_, nullptr);
    meta_ = CreateMeta();
    ASSERT_NE(meta_, nullptr);
}

void EAC3UnitTest::TearDown(void)
{
    if (plugin_) {
        plugin_->Release();
    }
}

shared_ptr<CodecPlugin> EAC3UnitTest::CreatePlugin()
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName(CODEC_EAC3_DEC_NAME);
    if (!plugin) {
        return nullptr;
    }
    return reinterpret_pointer_cast<CodecPlugin>(plugin);
}

shared_ptr<Media::Meta> EAC3UnitTest::CreateMeta()
{
    auto meta = make_shared<Media::Meta>();
    meta->Set<Tag::AUDIO_CHANNEL_COUNT>(EAC3_CHANNEL_COUNT);
    meta->Set<Tag::AUDIO_SAMPLE_RATE>(EAC3_SAMPLE_RATE);
    meta->Set<Tag::AUDIO_CHANNEL_LAYOUT>(AudioChannelLayout::STEREO);
    meta->Set<Tag::MEDIA_BITRATE>(EAC3_BIT_RATE);
    meta->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    return meta;
}

shared_ptr<AVBuffer> EAC3UnitTest::CreateAVBuffer(size_t capacity, size_t size)
{
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto avBuffer = AVBuffer::CreateAVBuffer(avAllocator, capacity);
    avBuffer->memory_->SetSize(size);
    return avBuffer;
}

bool EAC3UnitTest::ReadOneFrame(ifstream &file, shared_ptr<AVBuffer> &buffer)
{
    // syncword(0x0B77) + 16bit(strmtyp(2)+substreamid(3)+frmsiz(11))
    int c = 0;
    int prev = -1;
    streampos frameStartPos;
    while (file.good()) {
        c = file.get();
        if (c == EOF) {
            return false;
        }
        if (prev == 0x0B && c == 0x77) {
            frameStartPos = file.tellg();
            file.seekg(-eaC3SyncwordBackOffset, ios::cur);
            break;
        }
        prev = c;
    }
    if (!file.good()) {
        return false;
    }
    unsigned char header[eaC3HeaderSize];
    file.read(reinterpret_cast<char *>(header), eaC3HeaderSize);
    if (file.gcount() != eaC3HeaderSize) {
        return false;
    }
    if (!(header[0] == 0x0B && header[1] == 0x77)) {
        return false;
    }
    uint16_t word = (static_cast<uint16_t>(header[2]) << 8) | header[3];
    uint16_t frmsiz = word & 0x07FF; // lower 11 bits
    size_t frameSizeBytes = static_cast<size_t>(frmsiz + 1) * 2;
    if (frameSizeBytes < eaC3HeaderSize || frameSizeBytes > maxFrameSize) {
        return false;
    }
    size_t remaining = frameSizeBytes - eaC3HeaderSize;
    vector<unsigned char> frameBuf(frameSizeBytes);
    for (auto i = 0; i < eaC3HeaderSize; i++) {
        frameBuf[i] = header[i];
    }
    file.read(reinterpret_cast<char *>(frameBuf.data() + eaC3HeaderSize), remaining);
    if (static_cast<size_t>(file.gcount()) != remaining) {
        return false;
    }
    buffer = CreateAVBuffer(frameSizeBytes, frameSizeBytes);
    errno_t ret = memcpy_s(buffer->memory_->GetAddr(), buffer->memory_->GetCapacity(), frameBuf.data(), frameSizeBytes);
    if (ret != EOK) {
        return false;
    }
    return true;
}

void EAC3UnitTest::ProcessInputFrames(ifstream &file, int &inputFrameCount, int &outputFrameCount, bool &sentEos)
{
    while (!sentEos) {
        shared_ptr<AVBuffer> inBuf;
        if (ReadOneFrame(file, inBuf)) {
            ASSERT_EQ(plugin_->QueueInputBuffer(inBuf), Status::OK);
            inputFrameCount++;
        } else {
            auto eos = CreateAVBuffer(0, 0);
            ASSERT_NE(eos, nullptr);
            eos->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
            ASSERT_EQ(plugin_->QueueInputBuffer(eos), Status::OK);
            sentEos = true;
        }
        ProcessOutputFrames(outputFrameCount);
    }
}

void EAC3UnitTest::ProcessOutputFrames(int &outputFrameCount)
{
    for (int i = 0; i < EAC3_ATTEMPT_COUNT_1; i++) {
        auto outBuf = CreateAVBuffer(EAC3_MAX_INPUT_SIZE * 8, 0);
        ASSERT_NE(outBuf, nullptr);
        Status s = plugin_->QueueOutputBuffer(outBuf);
        if (s == Status::OK || s == Status::ERROR_AGAIN) {
            if (outBuf->memory_->GetSize() > 0) {
                outputFrameCount++;
            }
        } else if (s == Status::ERROR_NOT_ENOUGH_DATA || s == Status::END_OF_STREAM) {
            break;
        } else {
            ASSERT_TRUE(false) << "Unexpected status=" << static_cast<int>(s);
        }
    }
    for (int tries = 0; tries < EAC3_ATTEMPT_COUNT_2; ++tries) {
        auto outBuf = CreateAVBuffer(EAC3_MAX_INPUT_SIZE * 8, 0);
        ASSERT_NE(outBuf, nullptr);
        Status s = plugin_->QueueOutputBuffer(outBuf);
        if (s == Status::OK || s == Status::ERROR_AGAIN) {
            if (outBuf->memory_->GetSize() > 0) {
                outputFrameCount++;
            }
        } else if (s == Status::END_OF_STREAM) {
            break;
        } else if (s == Status::ERROR_NOT_ENOUGH_DATA) {
            continue;
        } else {
            ASSERT_TRUE(false) << "Unexpected status(post-EOS)=" << static_cast<int>(s);
        }
    }
}

HWTEST_F(EAC3UnitTest, SetParameter_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
}

HWTEST_F(EAC3UnitTest, SetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(EAC3_CHANNEL_COUNT);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(0);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(-1);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(EAC3_SAMPLE_RATE);
}

HWTEST_F(EAC3UnitTest, SetParameter_003, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    meta_->Remove(Tag::AUDIO_CHANNEL_COUNT);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_CHANNEL_COUNT>(EAC3_CHANNEL_COUNT);
    meta_->Remove(Tag::AUDIO_SAMPLE_RATE);
    ASSERT_NE(plugin_->SetParameter(meta_), Status::OK);
    meta_->Set<Tag::AUDIO_SAMPLE_RATE>(EAC3_SAMPLE_RATE);
}

HWTEST_F(EAC3UnitTest, Lifecycle_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    ASSERT_EQ(plugin_->Flush(), Status::OK);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
    ASSERT_EQ(plugin_->Reset(), Status::OK);
    ASSERT_EQ(plugin_->Release(), Status::OK);
}

HWTEST_F(EAC3UnitTest, Eos_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(1024, 0);
    inputBuffer->flag_ = MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::OK);
    auto outputBuffer = CreateAVBuffer(EAC3_MAX_INPUT_SIZE * 4, 0);
    ASSERT_EQ(plugin_->QueueOutputBuffer(outputBuffer), Status::END_OF_STREAM);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(EAC3UnitTest, GetParameter_001, TestSize.Level1)
{
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(EAC3_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    int32_t channelCount = 0;
    int32_t sampleRate = 0;
    int32_t maxInputSize = 0;
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_CHANNEL_COUNT>(channelCount));
    ASSERT_EQ(channelCount, EAC3_CHANNEL_COUNT);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate));
    ASSERT_EQ(sampleRate, EAC3_SAMPLE_RATE);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_GT(maxInputSize, 0);
}

HWTEST_F(EAC3UnitTest, GetParameter_002, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    // 1536 * 16 * 4 = 98304
    int32_t defaultInputSize = 1536 * EAC3_CHANNEL_COUNT * 4;
    shared_ptr<Media::Meta> outMeta = make_shared<Media::Meta>();
    int32_t maxInputSize = 0;
    int32_t smallInputSize = 4096;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(smallInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, smallInputSize);
    int32_t largeInputSize = 100000;
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(largeInputSize);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, defaultInputSize);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(-1);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->GetParameter(outMeta), Status::OK);
    ASSERT_TRUE(outMeta->Get<Tag::AUDIO_MAX_INPUT_SIZE>(maxInputSize));
    ASSERT_EQ(maxInputSize, defaultInputSize);
}

HWTEST_F(EAC3UnitTest, Decode_With_Invalid_File_001, TestSize.Level1)
{
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(EAC3_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    auto inputBuffer = CreateAVBuffer(EAC3_MAX_INPUT_SIZE, 1024);
    auto outputBuffer = CreateAVBuffer(EAC3_MAX_INPUT_SIZE * 4, 0);
    // No valid data, so it errors
    ASSERT_EQ(plugin_->QueueInputBuffer(inputBuffer), Status::ERROR_INVALID_DATA);
    Status status = plugin_->QueueOutputBuffer(outputBuffer);
    ASSERT_TRUE(status == Status::OK || status == Status::ERROR_NOT_ENOUGH_DATA || status == Status::ERROR_UNKNOWN);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

HWTEST_F(EAC3UnitTest, Decode_With_Valid_File_001, TestSize.Level1)
{
    const char *filePath = "/data/test/media/eac3_test.eac3";
    ifstream file(filePath, ios::binary);
    ASSERT_EQ(file.is_open(), true);
    ASSERT_EQ(plugin_->Init(), Status::OK);
    ASSERT_EQ(plugin_->SetDataCallback(this), Status::OK);
    meta_->Set<Tag::AUDIO_MAX_INPUT_SIZE>(EAC3_MAX_INPUT_SIZE);
    ASSERT_EQ(plugin_->SetParameter(meta_), Status::OK);
    ASSERT_EQ(plugin_->Start(), Status::OK);
    bool sentEos = false;
    int inputFrameCount = 0;
    int outputFrameCount = 0;
    ProcessInputFrames(file, inputFrameCount, outputFrameCount, sentEos);
    ProcessOutputFrames(outputFrameCount);
    if (outputFrameCount == 0) {
        GTEST_LOG_(WARNING) << "No decoded frames produced. inputFrameCount=" << inputFrameCount;
    }
    ASSERT_GT(inputFrameCount, 0);
    ASSERT_GT(outputFrameCount, 0);
    ASSERT_EQ(plugin_->Stop(), Status::OK);
}

} // namespace Plugins
} // namespace Media
} // namespace OHOS
