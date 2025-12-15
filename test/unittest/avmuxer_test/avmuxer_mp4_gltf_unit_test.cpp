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
#include "buffer/avbuffer.h"
#include "buffer/avbuffer_queue_producer.h"
#include "nocopyable.h"
#include "avmuxer.h"
#include "native_avbuffer.h"
#include "native_avmuxer.h"
#include "native_avformat.h"
#include "stream_demuxer.h"
#include "common/media_source.h"
#include "avsource.h"


using namespace testing::ext;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;

namespace OHOS {
namespace MediaAVCodec {
const std::string INPUT_AUDIO_FILE_PATH = "/data/test/media/aac_2c_44100hz_199k_muxer.dat";
const std::string OUTPUT_MP4_FILE_PATH = "/data/test/media/Muxer_AAC_44100_1.mp4";
const std::string TEST_FILE_PATH = "/data/test/media/";

class AVMuxerMp4GltfUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    virtual int32_t WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file, bool &eosFlag, uint32_t flag);
    virtual void InitResource(const std::string &filePath, std::string pluginName);

protected:
    std::shared_ptr<AVMuxer> avmuxer_ {nullptr};
    std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    int32_t fd_ {-1};
    std::shared_ptr<AVSource> avsource_ = nullptr;
    bool initStatus_ = false;
    MediaInfo mediaInfo_;
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

    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    file->read(reinterpret_cast<char*>(&info.pts), sizeof(info.pts));
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    file->read(reinterpret_cast<char*>(&info.flags), sizeof(info.flags));
    if (file->eof()) {
        eosFlag = true;
        return 0;
    }

    file->read(reinterpret_cast<char*>(&info.size), sizeof(info.size));
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
    int32_t ret = avmuxer_->WriteSample(trackId, buffer);
    return ret;
}

void AVMuxerMp4GltfUnitTest::InitResource(const std::string &filePath, std::string pluginName)
{
    struct stat fileStatus {};
    if (stat(filePath.c_str(), &fileStatus) != 0) {
        printf("Failed to get file status for path: %s\n", filePath.c_str());
        return;
    }
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    int fd = open(filePath.c_str(), O_RDONLY);
    if (fd < 0) {
        printf("Failed to open file: %s\n", filePath.c_str());
        return;
    }
    avsource_ = AVSourceFactory::CreateWithFD(fd, 0, fileSize);
    initStatus_ = true;
}

/**
 * @tc.name: Muxer_MP4_GLTF_001
 * @tc.desc: normal case: muxer mp4 gltf box
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerMp4GltfUnitTest, Muxer_MP4_GLTF_001, TestSize.Level0)
{
    int32_t trackId = -1;
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    fd_ = open(OUTPUT_MP4_FILE_PATH.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    avmuxer_ = AVMuxerFactory::CreateAVMuxer(fd_, outputFormat);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(0);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("mime");
    ASSERT_EQ(avmuxer_->SetParameter(param), 0);

    int32_t ret = avmuxer_->AddTrack(trackId, param);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    InitResource(OUTPUT_MP4_FILE_PATH, pluginName);
    Format format;
    ret = avsource_->GetSourceFormat(format);
    EXPECT_EQ(ret, 0);
    int32_t isGltf = 0;
    int64_t gltfOffset = 0;
    format.GetIntValue(Tag::IS_GLTF, isGltf);
    format.GetLongValue(Tag::GLTF_OFFSET, gltfOffset);
    EXPECT_NE(isGltf, 0);
    EXPECT_EQ(gltfOffset, 161623);
}

/**
 * @tc.name: Muxer_MP4_GLTF_002
 * @tc.desc: mux mp4 gltf box with incomplete param
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerMp4GltfUnitTest, Muxer_MP4_GLTF_002, TestSize.Level0)
{
    int32_t trackId = -1;
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    fd_ = open(OUTPUT_MP4_FILE_PATH.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    avmuxer_ = AVMuxerFactory::CreateAVMuxer(fd_, outputFormat);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(0); // missing other gltf tag
    ASSERT_EQ(avmuxer_->SetParameter(param), 0);

    int32_t ret = avmuxer_->AddTrack(trackId, param);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    InitResource(OUTPUT_MP4_FILE_PATH, pluginName);
    Format format;
    ret = avsource_->GetSourceFormat(format);
    EXPECT_EQ(ret, 0);
    int32_t isGltf = 0;
    format.GetIntValue(Tag::IS_GLTF, isGltf);
    EXPECT_EQ(isGltf, 0);
}

/**
 * @tc.name: Muxer_MP4_GLTF_003
 * @tc.desc: normal case: muxer mp4 gltf box, moov front
 * @tc.type: FUNC
 */
HWTEST_F(AVMuxerMp4GltfUnitTest, Muxer_MP4_GLTF_003, TestSize.Level0)
{
    int32_t trackId = -1;
    Plugins::OutputFormat outputFormat = Plugins::OutputFormat::MPEG_4;

    fd_ = open(OUTPUT_MP4_FILE_PATH.c_str(), O_CREAT | O_RDWR | O_TRUNC, S_IRUSR | S_IWUSR);
    avmuxer_ = AVMuxerFactory::CreateAVMuxer(fd_, outputFormat);

    std::shared_ptr<Meta> param = std::make_shared<Meta>();
    param->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    param->Set<Tag::AUDIO_SAMPLE_RATE>(44100); // 44100 sample rate
    param->Set<Tag::AUDIO_CHANNEL_COUNT>(2); // 2 channel count
    param->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);
    param->Set<Tag::AUDIO_SAMPLE_FORMAT>(AudioSampleFormat::SAMPLE_S16LE);
    param->Set<Tag::MEDIA_ENABLE_MOOV_FRONT>(1); // moov front

    // 添加文件级元数据
    std::vector<uint8_t> binVec = {
        0x89, 0x47, 0x4C, 0x42, 0x02, 0x00, 0x00, 0x00,
        0x10, 0x00, 0x00, 0x00, 0x4E, 0x4F, 0x53, 0x4A,
        0x08, 0x00, 0x00, 0x00
    };
    param->Set<Tag::MEDIA_GLTF_VERSION>(0);
    param->Set<Tag::MEDIA_GLTF_ITEM_NAME>("result_panda_0918.glb");
    param->Set<Tag::MEDIA_GLTF_CONTENT_TYPE>("model/gltf-binary");
    param->Set<Tag::MEDIA_GLTF_CONTENT_ENCODING>("binary");
    param->Set<Tag::MEDIA_GLTF_DATA>(binVec);
    param->Set<Tag::MEDIA_GLTF_ITEM_TYPE>("mime");
    ASSERT_EQ(avmuxer_->SetParameter(param), 0);

    int32_t ret = avmuxer_->AddTrack(trackId, param);
    ASSERT_EQ(ret, 0);
    ASSERT_GE(trackId, 0);
    ASSERT_EQ(avmuxer_->Start(), 0);

    inputFile_ = std::make_shared<std::ifstream>(INPUT_AUDIO_FILE_PATH, std::ios::binary);

    int32_t extSize = 0;
    inputFile_->read(reinterpret_cast<char*>(&extSize), sizeof(extSize));
    if (extSize > 0) {
        std::vector<uint8_t> buffer(extSize);
        inputFile_->read(reinterpret_cast<char*>(buffer.data()), extSize);
    }

    bool eosFlag = false;
    uint32_t flag = AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
    ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    while (!eosFlag && (ret == 0)) {
        ret = WriteSample(trackId, inputFile_, eosFlag, flag);
    }
    ASSERT_EQ(ret, 0);
    ASSERT_EQ(avmuxer_->Stop(), 0);

    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    InitResource(OUTPUT_MP4_FILE_PATH, pluginName);
    Format format;
    ret = avsource_->GetSourceFormat(format);
    EXPECT_EQ(ret, 0);
    int32_t isGltf = 0;
    int64_t gltfOffset = 0;
    format.GetIntValue(Tag::IS_GLTF, isGltf);
    format.GetLongValue(Tag::GLTF_OFFSET, gltfOffset);
    EXPECT_NE(isGltf, 0);
    EXPECT_EQ(gltfOffset, 164126);
}
} // namespace MediaAVCodec
} // namespace OHOS