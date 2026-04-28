/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <fcntl.h>
#include <climits>
#include <memory>
#include <sys/stat.h>
#include <unistd.h>
#include <gtest/gtest.h>
#include "buffer/avbuffer.h"
#include "common/media_source.h"
#include "demuxer_plugin_manager.h"
#include "mpeg4_demuxer_plugin.h"
#include "stream_demuxer.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::Media::Plugins;
using namespace OHOS::Media::Plugins::MPEG4;
using namespace testing::ext;

namespace {
constexpr uint32_t TEST_TIMEOUT_MS = 100;
constexpr uint32_t TEST_BUFFER_SIZE = 4 * 1024 * 1024;
const std::string TEST_MP4_PATH = "/data/test/media/test_264_B_Gop25_4sec_cover.mp4";
using MediaAVBuffer = OHOS::Media::AVBuffer;
using PluginMediaType = OHOS::Media::Plugins::MediaType;

std::shared_ptr<MediaAVBuffer> CreateTestBuffer(std::unique_ptr<uint8_t[]>& storage,
    uint32_t size = TEST_BUFFER_SIZE)
{
    storage = std::make_unique<uint8_t[]>(size);
    return MediaAVBuffer::CreateAVBuffer(storage.get(), size, 0);
}

bool IsEos(const std::shared_ptr<MediaAVBuffer>& buffer)
{
    return buffer != nullptr && (buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)) != 0;
}

bool IsSupportedTrack(const Meta& meta)
{
    PluginMediaType trackType = PluginMediaType::UNKNOWN;
    if (!meta.Get<Tag::MEDIA_TYPE>(trackType)) {
        return false;
    }
    return trackType == PluginMediaType::VIDEO || trackType == PluginMediaType::AUDIO;
}
} // namespace

class Mpeg4DemuxerPluginUnitTest : public testing::Test {
public:
    void TearDown() override
    {
        if (plugin_ != nullptr) {
            plugin_->Reset();
            plugin_ = nullptr;
        }
        if (fd_ >= 0) {
            close(fd_);
            fd_ = -1;
        }
    }

    void InitPlugin(const std::string& filePath)
    {
        struct stat fileStatus {};
        ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
        fd_ = open(filePath.c_str(), O_RDONLY);
        ASSERT_GE(fd_, 0);

        auto uri = "fd://" + std::to_string(fd_) + "?offset=0&size=" + std::to_string(fileStatus.st_size);
        auto mediaSource = std::make_shared<MediaSource>(uri);
        auto streamDemuxer = std::make_shared<StreamDemuxer>();
        auto source = std::make_shared<Source>();
        ASSERT_EQ(source->SetSource(mediaSource), Status::OK);
        streamDemuxer->SetSource(source);
        streamDemuxer->SetSourceType(mediaSource->GetSourceType());
        ASSERT_EQ(streamDemuxer->Init(uri), Status::OK);
        streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

        auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
        plugin_ = std::make_shared<MPEG4DemuxerPlugin>("avdemux_mpeg4");
        ASSERT_NE(plugin_, nullptr);
        ASSERT_EQ(plugin_->SetDataSource(dataSource), Status::OK);
        ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo_), Status::OK);
    }

    uint32_t SelectFirstSupportedTrack()
    {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            if (IsSupportedTrack(mediaInfo_.tracks[i])) {
                EXPECT_EQ(plugin_->SelectTrack(i), Status::OK);
                return i;
            }
        }
        ADD_FAILURE() << "No audio/video track found";
        return 0;
    }

protected:
    int fd_ = -1;
    MediaInfo mediaInfo_ {};
    std::shared_ptr<MPEG4DemuxerPlugin> plugin_ {nullptr};
};

/**
 * @tc.name: Mpeg4DemuxerPlugin_InitSelectTrack_001
 * @tc.desc: Directly create MPEG4DemuxerPlugin, set data source and select a media track
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_InitSelectTrack_001, TestSize.Level1)
{
    InitPlugin(TEST_MP4_PATH);
    ASSERT_GT(mediaInfo_.tracks.size(), 0U);
    uint32_t trackId = SelectFirstSupportedTrack();
    EXPECT_NE(plugin_->FindTrackById(trackId), nullptr);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncReadSample_001
 * @tc.desc: Directly call MPEG4DemuxerPlugin async GetNextSampleSize and ReadSample
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AsyncReadSample_001, TestSize.Level1)
{
    InitPlugin(TEST_MP4_PATH);
    uint32_t trackId = SelectFirstSupportedTrack();

    int32_t nextSize = 0;
    EXPECT_EQ(plugin_->GetNextSampleSize(trackId, nextSize, TEST_TIMEOUT_MS), Status::OK);
    EXPECT_GT(nextSize, 0);

    std::unique_ptr<uint8_t[]> bufferStorage;
    auto buffer = CreateTestBuffer(bufferStorage);
    ASSERT_NE(buffer, nullptr);
    Status ret = plugin_->ReadSample(trackId, buffer, TEST_TIMEOUT_MS);
    ASSERT_EQ(ret, Status::OK);
    EXPECT_FALSE(IsEos(buffer));
    EXPECT_GT(buffer->memory_->GetSize(), 0);

    int64_t lastPts = INT64_MIN;
    EXPECT_EQ(plugin_->GetLastPTSByTrackId(trackId, lastPts), Status::OK);
    EXPECT_NE(lastPts, INT64_MIN);
    EXPECT_EQ(plugin_->Pause(), Status::OK);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncReadSampleZeroCopy_001
 * @tc.desc: Directly call MPEG4DemuxerPlugin async ReadSampleZeroCopy
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AsyncReadSampleZeroCopy_001, TestSize.Level1)
{
    InitPlugin(TEST_MP4_PATH);
    uint32_t trackId = SelectFirstSupportedTrack();

    auto buffer = MediaAVBuffer::CreateAVBuffer();
    ASSERT_NE(buffer, nullptr);
    Status ret = plugin_->ReadSampleZeroCopy(trackId, buffer, TEST_TIMEOUT_MS);
    ASSERT_EQ(ret, Status::OK);
    EXPECT_FALSE(IsEos(buffer));
    ASSERT_NE(buffer->memory_, nullptr);
    EXPECT_GT(buffer->memory_->GetSize(), 0);
}

/**
 * @tc.name: Mpeg4DemuxerPlugin_AsyncTimeout_001
 * @tc.desc: Direct async read with zero timeout returns wait timeout when cache is empty
 * @tc.type: FUNC
 */
HWTEST_F(Mpeg4DemuxerPluginUnitTest, Mpeg4DemuxerPlugin_AsyncTimeout_001, TestSize.Level1)
{
    InitPlugin(TEST_MP4_PATH);
    uint32_t trackId = SelectFirstSupportedTrack();

    std::unique_ptr<uint8_t[]> bufferStorage;
    auto buffer = CreateTestBuffer(bufferStorage);
    ASSERT_NE(buffer, nullptr);
    EXPECT_EQ(plugin_->ReadSample(trackId, buffer, 0), Status::ERROR_WAIT_TIMEOUT);
}
