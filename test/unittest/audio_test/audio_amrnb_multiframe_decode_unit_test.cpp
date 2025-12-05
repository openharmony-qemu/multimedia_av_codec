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

#include <gtest/gtest.h>
#include <iostream>
#include <fstream>
#include "avcodec_codec_name.h"
#include "avcodec_log.h"
#include "avcodec_common.h"
#include "plugin/plugin_manager_v2.h"
#include "plugin/codec_plugin.h"
#include "avcodec_audio_common.h"
#include "avcodec_mime_type.h"
#include "common/log.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Ffmpeg {
constexpr int32_t SAMPLE_RATE_8K = 8000;
constexpr int32_t CHANNEL_COUNT = 1;
const std::string TEST_FILE_PATH = "/data/test/media/";
const std::string AMRNB_INPUT_FILE = "amrnb_multiframe_decode.dat";
const string CODEC_AMRNB_DEC_NAME = std::string(MediaAVCodec::AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME);

class AmrnbMulFrameDecTest : public testing::Test, public DataCallback {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp() override
    {
        auto tmp = PluginManagerV2::Instance().CreatePluginByName(CODEC_AMRNB_DEC_NAME);
        plugin_ = reinterpret_pointer_cast<CodecPlugin>(tmp);
        plugin_->SetDataCallback(this);
        
        meta_ = std::make_shared<Meta>();
        meta_->SetData(Tag::AUDIO_CHANNEL_COUNT, CHANNEL_COUNT);
        meta_->SetData(Tag::AUDIO_SAMPLE_RATE, SAMPLE_RATE_8K);
        meta_->SetData(Tag::AUDIO_SAMPLE_FORMAT, AudioSampleFormat::SAMPLE_S16LE);
        meta_->SetData(Tag::MIME_TYPE, MediaAVCodec::AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB);
        
        EXPECT_EQ(plugin_->SetParameter(meta_), Status::OK);
    }

    void TearDown() override
    {
        if (plugin_ != nullptr) {
            plugin_->Release();
            plugin_ = nullptr;
        }
        inputFile_.close();
        outputFile_.close();
    }

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

    bool ReadBuffer(std::shared_ptr<AVBuffer> buffer);
    Status ProcessDecoder(void);

protected:
    std::shared_ptr<CodecPlugin> plugin_ = nullptr;
    std::shared_ptr<Meta> meta_ = nullptr;
    std::ifstream inputFile_;
    std::ofstream outputFile_;
    int32_t outputBufferCapacity_;
    int32_t inputBufferCapacity_;
};

void AmrnbMulFrameDecTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void AmrnbMulFrameDecTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

bool AmrnbMulFrameDecTest::ReadBuffer(std::shared_ptr<AVBuffer> buffer)
{
    int64_t pts;
    uint32_t flag;
    int32_t size;

    inputFile_.read(reinterpret_cast<char *>(&buffer->pts_), sizeof(buffer->pts_));
    if (inputFile_.gcount() != sizeof(pts)) {
        cout << "Fatal: read pts fail" << endl;
        return false;
    }
    cout << "pts_: " << buffer->pts_ << endl;

    inputFile_.read(reinterpret_cast<char *>(&buffer->flag_), sizeof(buffer->flag_));
    if (inputFile_.gcount() != sizeof(flag)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }
    cout << "flag_: " << buffer->flag_ << endl;

    inputFile_.read(reinterpret_cast<char *>(&size), sizeof(size));
    if (inputFile_.eof() || inputFile_.gcount() == 0 || size == 0) {
        buffer->memory_->SetSize(1);
        buffer->flag_ = 1;
        cout << "Set EOS" << endl;
        plugin_->QueueInputBuffer(buffer);
        return false;
    }

    if (inputFile_.gcount() != sizeof(size)) {
        cout << "Fatal: read size fail" << endl;
        return false;
    }
    cout << "size: " << size << endl;

    inputFile_.read(reinterpret_cast<char *>(buffer->memory_->GetAddr()), size);
    buffer->memory_->SetSize(size);
    if (inputFile_.gcount() != size) {
        cout << "Fatal: read buffer fail" << endl;
        return false;
    }
    cout << "read amrnb buffer size:" << size << endl;
    plugin_->QueueInputBuffer(buffer);

    return true;
}

Status AmrnbMulFrameDecTest::ProcessDecoder()
{
    outputFile_.open(TEST_FILE_PATH + "amrnb_multiframe_decode_out.pcm", ios::binary);
    if (!outputFile_.is_open()) {
        cout << "output file open fail" << endl;
        return Status::ERROR_UNKNOWN;
    }
    shared_ptr<Meta> config = make_shared<Meta>();
    plugin_->GetParameter(config);
    config->Get<Tag::AUDIO_MAX_OUTPUT_SIZE>(outputBufferCapacity_);
    config->Get<Tag::AUDIO_MAX_INPUT_SIZE>(inputBufferCapacity_);
    Status ret = Status::OK;
    auto avAllocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    auto inBuffer = AVBuffer::CreateAVBuffer(avAllocator, inputBufferCapacity_);
    auto outBuffer = AVBuffer::CreateAVBuffer(avAllocator, outputBufferCapacity_);
    bool isContinue = true;
    int32_t cnts = 0;
    while (isContinue) {
        isContinue = ReadBuffer(inBuffer);
        cnts++;
        cout << "ReadBuffer cnts: " << cnts << endl;
        Status outRet;
        do {
            outRet = plugin_->QueueOutputBuffer(outBuffer);
            if (outRet == Status::OK || outRet == Status::ERROR_AGAIN) {
                outputFile_.write(reinterpret_cast<char *>(outBuffer->memory_->GetAddr()),
                                  outBuffer->memory_->GetSize());
            }
        } while (outRet == Status::ERROR_AGAIN);
    }
    return ret;
}

HWTEST_F(AmrnbMulFrameDecTest, NormalDecode, TestSize.Level1)
{
    inputFile_.open(TEST_FILE_PATH + AMRNB_INPUT_FILE, ios::binary);
    EXPECT_NE(inputFile_.is_open(), 0);
    Status ret = ProcessDecoder();
    EXPECT_EQ(ret, Status::OK);
}
} // namespace Ffmpeg
} // namespace Plugins
} // namespace Media
} // namespace OHOS