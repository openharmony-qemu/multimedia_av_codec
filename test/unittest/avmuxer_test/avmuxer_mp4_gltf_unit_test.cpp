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
#include <string>
#include <vector>
#include <fcntl.h>
#include <fstream>
#include "avmuxer_sample.h"
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue_producer.h"
#include "nocopyable.h"
#include "avmuxer.h"
#include "native_avbuffer.h"
#include "native_avmuxer.h"
#include "native_avformat.h"

using namespace testing::ext;
using namespace OHOS::Media;
namespace OHOS {
namespace MediaAVCodec {
const std::string INPUT_AUDIO_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k_lc.dat";
const std::string INPUT_GLTF_BIN_PATH = "/data/test/media/mp4_gltf.bin";
const std::string TEST_FILE_PATH = "/data/test/media/";

class AVMuxerMp4GltfUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    int32_t WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file, bool &eosFlag, uint32_t flag);

protected:
    std::shared_ptr<AVMuxer> avmuxer_ {nullptr};
    std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    int32_t fd_ {-1};
};

void AVMuxerMp4GltfUnitTest::SetUpTestCase()
{}

void AVMuxerMp4GltfUnitTest::TearDownTestCase()
{}

void AVMuxerMp4GltfUnitTest::SetUp()
{}

void AVMuxerMp4GltfUnitTest::TearDown()
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
        avmuxer_ = nullptr;
    }
}

int32_t AVMuxerMp4GltfUnitTest::WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file,
                                            bool &eosFlag, uint32_t flag)
{
    OH_AVCodecBufferAttr info;
    int64_t size = 0;

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    file->read(reinterpret_cast<char*>(&size), sizeof(size));
    info.size = static_cast<int32_t>(size);
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    file->read(reinterpret_cast<char*>(&info.pts), sizeof(info.pts));
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> buffer = nullptr;

    if (info.size == 0) {
        info.size = 1;
        buffer = AVBuffer::CreateAVBuffer(alloc, info.size);
    } else {
        buffer = AVBuffer::CreateAVBuffer(alloc, info.size);
        file->read(reinterpret_cast<char*>(buffer->memory_->GetAddr()), info.size);
        buffer->pts_ = info.pts;
        buffer->flag_ = info.flags;
        buffer->memory_->SetSize(info.size);
    }
    
    return avmuxer_->WriteSample(trackId, buffer);
}

/**
 * @tc.name: Muxer_MP4_GLTF_001
 * @tc.desc: support muxer mp4 gltf box
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerMp4GltfUnitTest, Muxer_MP4_GLTF_001, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AAC_44100_1.mp4");
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    avmuxer_ = AVMuxerFactory::CreateAVMuxer(fd_, outputFormat);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    audioParams->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    audioParams->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);

    std::shared_ptr<std::ifstream> gltfBin = std::make_shared<std::ifstream>(INPUT_GLTF_BIN_PATH, std::ios::binary);
    gltfBin->seekg(0, std::ios::end);
    size_t binSize = gltfBin->tellg();
    gltfBin->seekg(0);
    std::vector<uint8_t> binVec(binSize);
    gltfBin->read(reinterpret_cast<char*>(binVec.data()), binSize);
    std::cout << "binSize:" << binSize << std::endl;

    // 添加文件级元数据
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MEDIA_GLTF_VERSION>(0);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("mime");
    ASSERT_EQ(avmuxer_->SetUserMeta(param), 0);
 
    ASSERT_EQ(avmuxer_->Stop(), 0);
}

/**
 * @tc.name: Muxer_MP4_GLTF_002
 * @tc.desc: mux mp4 gltf box with incomplete param
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerMp4GltfUnitTest, Muxer_MP4_GLTF_002, TestSize.Level0)
{
    int32_t trackId = -1;
    std::string outputFile = TEST_FILE_PATH + std::string("Muxer_AAC_44100_2.mp4");
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    fd_ = open(outputFile.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    avmuxer_ = AVMuxerFactory::CreateAVMuxer(fd_, outputFormat);

    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    audioParams->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    audioParams->Set<Tag::AUDIO_SAMPLE_FORMAT>(Media::Plugins::AudioSampleFormat::SAMPLE_S16LE);
    int32_t ret = avmuxer_->AddTrack(trackId, audioParams);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);

    // 添加文件级元数据
    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MEDIA_GLTF_VERSION>(0);
    ASSERT_EQ(avmuxer_->SetUserMeta(param), 0);
 
    ASSERT_EQ(avmuxer_->Stop(), 0);
}
} // namespace MediaAVCodec
} // namespace OHOS