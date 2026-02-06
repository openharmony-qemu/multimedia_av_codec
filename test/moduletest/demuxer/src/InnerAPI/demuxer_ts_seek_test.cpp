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

#include "gtest/gtest.h"
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "plugin/plugin_manager_v2.h"
#include <random>
#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;
static std::random_device g_mtsRdm;
namespace OHOS {
namespace Media {

class DemuxerTsSeekInnerTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    bool CreateDataSource(const std::string &filePath);
    bool CreateDemuxerPluginByName(const std::string &typeName, const std::string &filePath, int probSize);
    size_t PluginReadSample(uint32_t idx, uint32_t &flag);
    bool PluginSelectTracks();
    void RemoveValue();
    size_t GetBufferSize();
private:
    std::shared_ptr<Media::StreamDemuxer> realStreamDemuxer_{ nullptr };
    std::shared_ptr<Media::MediaSource> mediaSource_{ nullptr };
    std::shared_ptr<Media::Source> realSource_{ nullptr };
    std::shared_ptr<Media::PluginBase> pluginBase_{ nullptr };
    std::shared_ptr<Media::DemuxerPlugin> demuxerPlugin_{ nullptr };
    int streamId_ = 0;
    std::vector<uint8_t> buffer_;
    std::map<uint32_t, uint32_t> frames_;
    std::map<uint32_t, uint32_t> keyFrames_;
    std::map<uint32_t, bool> eosFlag_;
    std::vector<uint32_t> selectedTrackIds_;
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager_{ nullptr };
    std::shared_ptr<DataSourceImpl> dataSourceImpl_{ nullptr };
};
void DemuxerTsSeekInnerTest::SetUpTestCase(void) {}
void DemuxerTsSeekInnerTest::TearDownTestCase(void) {}
void DemuxerTsSeekInnerTest::SetUp(void) {}
void DemuxerTsSeekInnerTest::TearDown(void) {}

static const std::string DEMUXER_PLUGIN_NAME_MPEGTS = "avdemux_mpegts";
static const std::string TEST_RESOURCE_PATH = "/data/test/media/aac_mpeg4.ts";
static const int DEF_PROB_SIZE = 16 * 1024;
static const int BUFFER_PADDING_SIZE = 1024;
static const uint32_t TIMEOUT = 100;
static const int64_t EXPECT_SEEK_TIME = 1600000000;
static const size_t EXPECT_NEXT_SYNC_FRAME_SIZE = 67410;
static const uint32_t SEEK_POS = 10;

static const size_t EXPECT_NEXT_SYNC_FRAME_SIZE_2 = 68746;
static const uint32_t SEEK_POS_2 = 2000;

static const uint32_t READ_SAMPLE_TIME = 13;
static const uint32_t SEEK_POS_3 = 33;
static const size_t EXPECT_NEXT_SYNC_FRAME_SIZE_3 = 67410;

bool DemuxerTsSeekInnerTest::CreateDataSource(const std::string &filePath)
{
    mediaSource_ = std::make_shared<MediaSource>(filePath);
    realSource_ = std::make_shared<Source>();
    realSource_->SetSource(mediaSource_);

    realStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    realStreamDemuxer_->SetSourceType(Plugins::SourceType::SOURCE_TYPE_URI);
    realStreamDemuxer_->SetSource(realSource_);
    realStreamDemuxer_->Init(filePath);
    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    dataSourceImpl_ = std::make_shared<DataSourceImpl>(realStreamDemuxer_, streamId_);
    realSource_->NotifyInitSuccess();
    return true;
}

size_t DemuxerTsSeekInnerTest::GetBufferSize()
{
    size_t bufferSize = buffer_.size();
    return bufferSize;
}

bool DemuxerTsSeekInnerTest::CreateDemuxerPluginByName(
    const std::string &typeName, const std::string &filePath, int probSize)
{
    if (!CreateDataSource(filePath)) {
        printf("CreateDataSource failed for file: %s\n", filePath.c_str());
        return false;
    }
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(typeName);
    if (pluginBase_ == nullptr) {
        printf("CreatePluginByName failed for type: %s\n", typeName.c_str());
        return false;
    }
    demuxerPlugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (demuxerPlugin_->SetDataSourceWithProbSize(dataSourceImpl_, probSize) != Status::OK) {
        printf("SetDataSourceWithProbSize failed for type: %s\n", typeName.c_str());
        return false;
    }
    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);
    return true;
}

size_t DemuxerTsSeekInnerTest::PluginReadSample(uint32_t idx, uint32_t &flag)
{
    int bufSize = 0;
    demuxerPlugin_->GetNextSampleSize(idx, bufSize);
    if (static_cast<uint32_t>(bufSize) > buffer_.size()) {
        buffer_.resize(bufSize + BUFFER_PADDING_SIZE);
    }
    auto avBuf = AVBuffer::CreateAVBuffer(buffer_.data(), bufSize, bufSize);
    if (avBuf == nullptr) {
        printf("CreateAVBuffer failed for index: %u size %d\n", idx, bufSize);
        return 0;
    }
    demuxerPlugin_->ReadSample(idx, avBuf);
    flag = avBuf->flag_;
    return bufSize;
}

bool DemuxerTsSeekInnerTest::PluginSelectTracks()
{
    MediaInfo mediaInfo;
    if (demuxerPlugin_->GetMediaInfo(mediaInfo) != Status::OK) {
        printf("GetMediaInfo failed for plugin\n");
        return false;
    }
    for (size_t i = 0; i < mediaInfo.tracks.size(); i++) {
        std::string mime;
        mediaInfo.tracks[i].GetData(Tag::MIME_TYPE, mime);
        if (mime.find("video/") == 0 || mime.find("audio/") == 0 ||
            mime.find("application/") == 0 || mime.find("text/vtt") == 0) {
            demuxerPlugin_->SelectTrack(static_cast<uint32_t>(i));
            selectedTrackIds_.push_back(static_cast<uint32_t>(i));
            frames_[i] = 0;
            keyFrames_[i] = 0;
            eosFlag_[i] = false;
        }
    }
    return true;
}

void DemuxerTsSeekInnerTest::RemoveValue()
{
    if (!frames_.empty()) {
        frames_.clear();
    }
    if (!keyFrames_.empty()) {
        keyFrames_.clear();
    }
    if (!eosFlag_.empty()) {
        eosFlag_.clear();
    }
    if (!selectedTrackIds_.empty()) {
        selectedTrackIds_.clear();
    }
}

/**
 * @tc.number    : DEMUXER_TS_SEEK_INNER_API_0010
 * @tc.name      : trackID 为正常值
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerTsSeekInnerTest, DEMUXER_TS_SEEK_INNER_API_0010, TestSize.Level0)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_RESOURCE_PATH, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    Status stat = demuxerPlugin->SeekToKeyFrame(0, 0, SeekMode::SEEK_NEXT_SYNC, realSeekTime, TIMEOUT);
    ASSERT_EQ(stat, Status::OK);
}

/**
 * @tc.number    : DEMUXER_TS_SEEK_INNER_API_0020
 * @tc.name      : trackID 为正常值
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerTsSeekInnerTest, DEMUXER_TS_SEEK_INNER_API_0020, TestSize.Level0)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_RESOURCE_PATH, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    Status stat = demuxerPlugin->SeekToKeyFrame(0, INT64_MAX, SeekMode::SEEK_NEXT_SYNC, realSeekTime, TIMEOUT);
    ASSERT_EQ(stat, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_TS_SEEK_INNER_API_0030
 * @tc.name      : trackID 为正常值
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerTsSeekInnerTest, DEMUXER_TS_SEEK_INNER_API_0030, TestSize.Level0)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_RESOURCE_PATH, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    Status stat = demuxerPlugin->SeekToKeyFrame(0, 0, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime, TIMEOUT);
    ASSERT_EQ(stat, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_TS_SEEK_INNER_FUNC_0010
 * @tc.name      : seek到第二个I帧
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerTsSeekInnerTest, DEMUXER_TS_SEEK_INNER_FUNC_0010, TestSize.Level0)
{
    const std::string iframeVideo = "/data/test/media/h265_mp3_1920.ts";
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, iframeVideo, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    Status stat = demuxerPlugin->SeekToKeyFrame(0, TIMEOUT, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0);
    ASSERT_EQ(stat, Status::END_OF_STREAM);
}


/**
 * @tc.number    : DEMUXER_TS_SEEK_INNER_FUNC_0020
 * @tc.name      : seek到第二个I帧
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerTsSeekInnerTest, DEMUXER_TS_SEEK_INNER_FUNC_0020, TestSize.Level0)
{
    const std::string iframeVideo = "/data/test/media/ts_video.ts";
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, iframeVideo, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t flag = 0;
    Status stat = demuxerPlugin->SeekToKeyFrame(0, SEEK_POS, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0);
    ASSERT_EQ(stat, Status::OK);
    ASSERT_EQ(realSeekTime, EXPECT_SEEK_TIME);
    size_t bufferSize = PluginReadSample(0, flag);
    ASSERT_EQ(bufferSize, EXPECT_NEXT_SYNC_FRAME_SIZE);
}

/**
 * @tc.number    : DEMUXER_TS_SEEK_INNER_FUNC_0030
 * @tc.name      : 最后一帧为I帧，seek到最后一帧
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerTsSeekInnerTest, DEMUXER_TS_SEEK_INNER_FUNC_0030, TestSize.Level0)
{
    const std::string iframeVideo = "/data/test/media/ts_video.ts";
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, iframeVideo, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    int64_t realSeekTime = 0;
    uint32_t flag = 0;
    Status stat = demuxerPlugin->SeekToKeyFrame(0, SEEK_POS_2, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0);
    ASSERT_EQ(stat, Status::OK);
    size_t bufferSize = PluginReadSample(0, flag);
    ASSERT_EQ(bufferSize, EXPECT_NEXT_SYNC_FRAME_SIZE_2);
}

/**
 * @tc.number    : DEMUXER_TS_SEEK_INNER_FUNC_0040
 * @tc.name      : 解封装流程中调用seek接口
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerTsSeekInnerTest, DEMUXER_TS_SEEK_INNER_FUNC_0040, TestSize.Level0)
{
    const std::string iframeVideo = "/data/test/media/ts_video.ts";
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, iframeVideo, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);

    uint32_t flag = 0;
    for (int i = 0; i < READ_SAMPLE_TIME; i++) {
        PluginReadSample(0, flag);
    }
    int64_t realSeekTime = 0;
    Status stat = demuxerPlugin->SeekToKeyFrame(0, SEEK_POS_3, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 0);
    ASSERT_EQ(stat, Status::OK);
    size_t bufferSize = PluginReadSample(0, flag);
    ASSERT_EQ(bufferSize, EXPECT_NEXT_SYNC_FRAME_SIZE_3);
}
}
}