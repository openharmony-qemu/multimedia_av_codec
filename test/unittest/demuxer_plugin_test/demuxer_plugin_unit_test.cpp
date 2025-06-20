/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "demuxer_plugin_unit_test.h"
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "common/media_source.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_manager_v2.h"
#include <fcntl.h>
#include <fstream>
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include "media_description.h"
#include "mock/mock_buffer.h"
#include "mock/mock_memory.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;
using MediaAVBuffer = OHOS::Media::AVBuffer;
using FFmpegAVBuffer = ::AVBuffer;
using InvokerTypeAlias = OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin::InvokerType;
using MockDataSourceAdapterAlias = MockDataSourceAdapter<MockDataSourceInterface, DataSourceImpl>;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;
using ::testing::Return;
using ::testing::DoAll;
using ::testing::SetArgPointee;
using ::testing::_;
using ::testing::Invoke;

list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};
string g_flvPath = string("/data/test/media/h264.flv");

void DemuxerPluginUnitTest::SetUpTestCase(void) {}

void DemuxerPluginUnitTest::TearDownTestCase(void) {}

void DemuxerPluginUnitTest::SetUp(void) {}

void DemuxerPluginUnitTest::TearDown(void)
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    if (initStatus_) {
        initStatus_ = false;
    }
    if (demuxerPlugin_ != nullptr) {
        demuxerPlugin_->Reset();
        demuxerPlugin_ = nullptr;
    }
}

void DemuxerPluginUnitTest::InitResource(const std::string &filePath, std::string pluginName)
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
    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    std::shared_ptr<MediaSource> mediaSource = std::make_shared<MediaSource>(uri);
    std::shared_ptr<StreamDemuxer> streamDemuxer = std::make_shared<StreamDemuxer>();
    std::shared_ptr<Source> source = std::make_shared<Source>();
    source->SetSource(mediaSource);
    streamDemuxer->SetSource(source);
    streamDemuxer->SetSourceType(mediaSource->GetSourceType());
    streamDemuxer->Init(uri);
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(pluginName);
    demuxerPlugin_ = std::reinterpret_pointer_cast<OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin>(basePlugin);
    ASSERT_EQ(demuxerPlugin_->SetDataSource(dataSource), Status::OK);
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    initStatus_ = true;
}

void DemuxerPluginUnitTest::InitWeakNetworkDemuxerPlugin(
    const std::string& filePath, std::string pluginName, int64_t failOffset, size_t maxFailCount)
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
    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);
    std::shared_ptr<DemuxerPluginManager> demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    SourceCallback cb = SourceCallback(demuxerPluginManager);
    std::shared_ptr<Source> source = std::make_shared<Source>();
    source->SetCallback(&cb);
    EXPECT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);
    std::shared_ptr<BaseStreamDemuxer> streamDemuxer =
        std::make_shared<StreamDemuxerPullDataFailMock>(failOffset, maxFailCount);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    streamDemuxer->Init("");
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);
    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(pluginName);
    demuxerPlugin_ = std::reinterpret_pointer_cast<OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin>(basePlugin);
    ASSERT_EQ(demuxerPlugin_->SetDataSource(dataSource), Status::OK);
}

void DemuxerPluginUnitTest::SetInitValue()
{
    for (int i = 0; i < mediaInfo_.tracks.size(); i++) {
        string codecMime = "";
        OHOS::Media::Plugins::MediaType trackType = OHOS::Media::Plugins::MediaType::UNKNOWN;
        Meta &format = mediaInfo_.tracks[i];
        std::vector<TagType> keys;
        ASSERT_TRUE(format.GetData(Tag::MIME_TYPE, codecMime));
        ASSERT_TRUE(format.GetData(Tag::MEDIA_TYPE, trackType));
        if (trackType == OHOS::Media::Plugins::MediaType::VIDEO) {
            ASSERT_TRUE(format.GetData(Tag::VIDEO_WIDTH, videoWidth_));
            ASSERT_TRUE(format.GetData(Tag::VIDEO_HEIGHT, videoHeight_));
        }
        if (codecMime.find("image/") != std::string::npos) {
            continue;
        }
        if (trackType == OHOS::Media::Plugins::MediaType::VIDEO ||
            trackType == OHOS::Media::Plugins::MediaType::AUDIO ||
            trackType == OHOS::Media::Plugins::MediaType::SUBTITLE ||
            trackType == OHOS::Media::Plugins::MediaType::TIMEDMETA) {
            frames_[i] = 0;
            keyFrames_[i] = 0;
            eosFlag_[i] = false;
        }
    }
}

bool DemuxerPluginUnitTest::isEOS(map<uint32_t, bool>& countFlag)
{
    for (auto iter = countFlag.begin(); iter != countFlag.end(); ++iter) {
        if (!iter->second) {
            return false;
        }
    }
    return true;
}

bool DemuxerPluginUnitTest::CheckKeyFrameIndex(
    std::vector<uint32_t> keyFrameIndexList, const uint32_t frameIndex, const bool isKeyFrame)
{
    bool contaionIndex = (std::count(keyFrameIndexList.begin(), keyFrameIndexList.end(), frameIndex) > 0);
    return isKeyFrame ? contaionIndex : !contaionIndex;
}

void DemuxerPluginUnitTest::SetEosValue()
{
    for (int i = 0; i < nbStreams_; i++) {
        eosFlag_[i] = true;
    }
}

void DemuxerPluginUnitTest::CountFrames(uint32_t index)
{
    if (flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_EOS) {
        eosFlag_[index] = true;
        return;
    }
    if (flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME) {
        keyFrames_[index]++;
        frames_[index]++;
    } else if ((flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE) == AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_NONE) {
        frames_[index]++;
    } else {
        SetEosValue();
        printf("flag is unknown, read sample break");
    }
}

void DemuxerPluginUnitTest::ReadData()
{
    SetInitValue();
    AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3); // 3 is use for buffer allocate
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            flag_ = buffer.mediaAVBuffer->flag_;
            CountFrames(i);
        }
    }
}

void DemuxerPluginUnitTest::RemoveValue()
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
}

/**
 * @tc.name: Demuxer_SelectTrack_0001
 * @tc.desc: Test SelectTrack
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SelectTrack_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    ASSERT_NE(demuxerPlugin_->SelectTrack(2), Status::OK);
}

/**
 * @tc.name: Demuxer_ErrorSelectTrack_0001
 * @tc.desc: Test SelectTrack without initialization
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ErrorSelectTrack_0001, TestSize.Level1)
{
    string pluginName = "avdemux_flv";
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(pluginName);
    demuxerPlugin_ = std::reinterpret_pointer_cast<OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin>(basePlugin);
    ASSERT_NE(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_NE(demuxerPlugin_->SelectTrack(1), Status::OK);
    ASSERT_NE(demuxerPlugin_->SelectTrack(2), Status::OK);
}

/**
 * @tc.name: Demuxer_ErrorSelectTrack_0002
 * @tc.desc: Test SelectTrack after repeated initialization
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ErrorSelectTrack_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    InitResource(g_flvPath, pluginName);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    InitResource(g_flvPath, pluginName);
    ASSERT_NE(demuxerPlugin_->SelectTrack(2), Status::OK);
}

/**
 * @tc.name: Demuxer_ErrorSelectTrack_0003
 * @tc.desc: Test SelectTrack with invalid parameter
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ErrorSelectTrack_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_->SelectTrack(-1), Status::OK);
}

/**
 * @tc.name: Demuxer_UnSelectTrack_0001
 * @tc.desc: Test UnselectTrack
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_UnSelectTrack_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_NE(demuxerPlugin_->SelectTrack(3), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_NE(demuxerPlugin_->SelectTrack(-1), Status::OK);
    ASSERT_EQ(demuxerPlugin_->UnselectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->UnselectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin_->UnselectTrack(3), Status::OK);
    ASSERT_EQ(demuxerPlugin_->UnselectTrack(-1), Status::OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0001
 * @tc.desc: Test ReadSample with valid buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer(DEFAULT_BUFFSIZE);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
}

/**
 * @tc.name: Demuxer_ErrorReadSample_0001
 * @tc.desc: Test ReadSample with null buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ErrorReadSample_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    std::shared_ptr<MediaAVBuffer> mediaAVBuffer = nullptr;
    ASSERT_NE(demuxerPlugin_->ReadSample(0, mediaAVBuffer, 100), Status::OK);
}

/**
 * @tc.name: Demuxer_ErrorReadSample_0002
 * @tc.desc: Test ReadSample after reset and re-select
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ErrorReadSample_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer(DEFAULT_BUFFSIZE);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_EQ(demuxerPlugin_->Reset(), Status::OK);
    ASSERT_NE(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_NE(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer_1(DEFAULT_BUFFSIZE);
    ASSERT_NE(demuxerPlugin_->ReadSample(0, buffer_1.mediaAVBuffer, 100), Status::OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0002
 * @tc.desc: Test ReadSample with GetNextSampleSize
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    int32_t size = 0;
    demuxerPlugin_->GetNextSampleSize(0, size, 100);
    // Used to cover thread existence checks
    demuxerPlugin_->GetNextSampleSize(0, size, 100);
    AVBufferWrapper buffer(size);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0003
 * @tc.desc: Copy current sample to buffer (flv)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    std::vector<uint32_t> keyFrameIndex = {0};
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            flag_ = buffer.mediaAVBuffer->flag_;
            if (i == 0) {
                ASSERT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(i);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
}

/**
 * @tc.name: Demuxer_ReadSample_0004
 * @tc.desc: Copy current sample to buffer (flv) without timeout
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    std::vector<uint32_t> keyFrameIndex = {0};
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer);
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            flag_ = buffer.mediaAVBuffer->flag_;
            if (i == 0) {
                ASSERT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(i);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
}

/**
 * @tc.name: Demuxer_ReadSample_0005
 * @tc.desc: Test ReadSample with and without timeout
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer(DEFAULT_BUFFSIZE);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_NE(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer), Status::OK);
}

/**
 * @tc.name: Demuxer_ReadSample_0006
 * @tc.desc: Test ReadSample with and without timeout (reverse order)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer(DEFAULT_BUFFSIZE);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer), Status::OK);
    ASSERT_NE(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
}

/**
 * @tc.name: Demuxer_SeekToTime_0001
 * @tc.desc: Seek to the specified time (h264 flv local)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SeekToTime_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    int64_t seekTime_ = 0;
    list<int64_t> toPtsList = {0, 1500, 1000, 1740, 1970, 2100}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 0, 76, 0, 0, 76, 76, 0, 76, 0, 0, 76, 0, 0, 76, 0};
    vector<int32_t> audioVals = {107, 107, 107, 0, 107, 0, 0, 107, 107, 0, 107, 0, 0, 107, 0, 0, 107, 0};
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            Status ret_ = demuxerPlugin_->SeekTo(-1, *toPts, *mode, seekTime_);
            if (ret_ != Status::OK) {
                printf("seek failed, time = %" PRId64 " | ret = %d\n", *toPts, ret_);
                continue;
            }
            ReadData();
            printf("time = %" PRId64 " | frames_[0]=%d\n", *toPts, frames_[0]);
            printf("time = %" PRId64 " | frames_[1]=%d\n", *toPts, frames_[1]);
            ASSERT_EQ(frames_[0], videoVals[numbers_]);
            ASSERT_EQ(frames_[1], audioVals[numbers_]);
            numbers_ += 1;
            RemoveValue();
        }
    }
    ASSERT_NE(demuxerPlugin_->SeekTo(0, 11000, SeekMode::SEEK_NEXT_SYNC, seekTime_), Status::OK);
    ASSERT_NE(demuxerPlugin_->SeekTo(0, -1000, SeekMode::SEEK_NEXT_SYNC, seekTime_), Status::OK);
}

/**
 * @tc.name: Demuxer_ErrorSeekToTime_0001
 * @tc.desc: Seek to time with large positive or negative values
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ErrorSeekToTime_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    SetInitValue();
    int64_t seekTime_ = 0;
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    
    ASSERT_NE(demuxerPlugin_->SeekTo(0, INT64_MAX, SeekMode::SEEK_NEXT_SYNC, seekTime_), Status::OK);
    ASSERT_NE(demuxerPlugin_->SeekTo(0, -1000, SeekMode::SEEK_NEXT_SYNC, seekTime_), Status::OK);
}

/**
 * @tc.name: Demuxer_WeakNetwork_001
 * @tc.desc: Test demuxer under normal network conditions
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_WeakNetwork_001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    std::string srtPath = g_flvPath;
    InitWeakNetworkDemuxerPlugin(srtPath, pluginName, 0, 0); // 无弱网
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    std::vector<uint32_t> keyFrameIndex = {0};
    int32_t readCount = 0;
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            if (ret == Status::OK) {
                readCount++;
            }
            flag_ = buffer.mediaAVBuffer->flag_;
            if (i == 0) {
                ASSERT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(i);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
}

/**
 * @tc.name: Demuxer_WeakNetwork_002
 * @tc.desc: Test demuxer with weak network (init fail once)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_WeakNetwork_002, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    std::string srtPath = g_flvPath;
    InitWeakNetworkDemuxerPlugin(srtPath, pluginName, 500, 1); // 初始化
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    std::vector<uint32_t> keyFrameIndex = {0};
    int32_t readCount = 0;
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            if (ret == Status::OK) {
                readCount++;
            }
            flag_ = buffer.mediaAVBuffer->flag_;
            if (i == 0) {
                ASSERT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(i);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
}

/**
 * @tc.name: Demuxer_WeakNetwork_003
 * @tc.desc: Test demuxer with weak network (read frame fail 3 times)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_WeakNetwork_003, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    std::string srtPath = g_flvPath;
    InitWeakNetworkDemuxerPlugin(srtPath, pluginName, 2560656, 3); // 读帧
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    std::vector<uint32_t> keyFrameIndex = {0};
    int32_t readCount = 0;
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            while (ret == Status::ERROR_WAIT_TIMEOUT) {
                ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            }
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            if (ret == Status::OK) {
                readCount++;
            }
            flag_ = buffer.mediaAVBuffer->flag_;
            if (i == 0) {
                ASSERT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(i);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
}

/**
 * @tc.name: Demuxer_WeakNetwork_004
 * @tc.desc: Test demuxer with weak network (read frame fail 3 times, check last pts)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_WeakNetwork_004, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    std::string srtPath = g_flvPath;
    InitWeakNetworkDemuxerPlugin(srtPath, pluginName, 2560656, 3); // 读帧
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    std::vector<uint32_t> keyFrameIndex = {0};
    int32_t readCount = 0;
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            int64_t lastPts = -1;
            demuxerPlugin_->GetLastPTSByTrackId(i, lastPts);
            while (ret == Status::ERROR_WAIT_TIMEOUT) {
                ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            }
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            if (ret == Status::OK) {
                readCount++;
            }
            flag_ = buffer.mediaAVBuffer->flag_;
            if (i == 0) {
                ASSERT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(i);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
}

/**
 * @tc.name: Demuxer_WeakNetwork_005
 * @tc.desc: Test demuxer with weak network (read frame fail 20 times)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_WeakNetwork_005, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    std::string srtPath = g_flvPath;
    InitWeakNetworkDemuxerPlugin(srtPath, pluginName, 2560656, 20); // 读帧
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    std::vector<uint32_t> keyFrameIndex = {0};
    int32_t readCount = 0;
    while (!isEOS(eosFlag_)) {
        for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
            auto ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            while (ret == Status::ERROR_WAIT_TIMEOUT) {
                ret = demuxerPlugin_->ReadSample(i, buffer.mediaAVBuffer, 100);
            }
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            if (ret == Status::OK) {
                readCount++;
            }
            flag_ = buffer.mediaAVBuffer->flag_;
            if (i == 0) {
                ASSERT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(i);
        }
    }
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    ASSERT_EQ(frames_[0], 76);
    ASSERT_EQ(frames_[1], 113);
    ASSERT_EQ(keyFrames_[0], 1);
    ASSERT_EQ(keyFrames_[1], 113);
}


/**
 * @tc.name: Demuxer_mock_ReadSample_0001
 * @tc.desc: Mock Test ReadSample with valid buffer
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_mock_ReadSample_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    SetInitValue();
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer(DEFAULT_BUFFSIZE);
    auto mockDataSourceImpl = std::make_shared<MockDataSourceAdapterAlias>(streamDemuxer_, 0);
    demuxerPlugin_->ioContext_.dataSource= std::static_pointer_cast<DataSource>(mockDataSourceImpl);
    EXPECT_CALL(*mockDataSourceImpl, ReadAt).WillRepeatedly(Return(Status::OK));
    for (int i = 0; i < 3; i++) {
        auto ret = demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100);
        if (ret == Status::END_OF_STREAM) {
            break;
        }
    }
    auto ret = demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    MockBufferAdapter<MockBufferInterface, Buffer> mockBufferAdapter;
    size_t capacity = 10;
    std::shared_ptr<uint8_t> bufData = std::make_shared<uint8_t>(10);
    auto mockMemory = std::make_shared<MockMemoryAdapter<MockMemoryInterface, Memory>>(capacity, bufData);
    std::shared_ptr<Memory> mockMemoryPtr = static_cast<std::shared_ptr<Memory>>(mockMemory);
    auto mockBuffer = static_cast<Buffer>(mockBufferAdapter);
    EXPECT_CALL(mockBufferAdapter, GetMemory).WillRepeatedly(Return(mockMemoryPtr));
    std::shared_ptr<Memory> memoryPtr = mockBufferAdapter.GetMemory(0);
    EXPECT_CALL(*mockMemory, GetSize).WillRepeatedly(Return(-1));
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_CALL(*mockDataSourceImpl, ReadAt).WillRepeatedly(Return(Status::ERROR_AGAIN));

    for (int i = 0; i < 6; i++) {
        ret = demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100);
    }
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: Demuxer_EnsurePacketAllocated_001
 * @tc.desc: EnsurePacketAllocated
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_EnsurePacketAllocated_001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    AVPacket *pkt = av_packet_alloc();
    demuxerPlugin_->EnsurePacketAllocated(pkt);
    av_packet_free(&pkt);
}


/**
 * @tc.name: Demuxer_UpdateInitDownloadData_001
 * @tc.desc: Test UpdateInitDownloadData
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_UpdateInitDownloadData_001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    demuxerPlugin_->ioContext_.initCompleted = false;
    demuxerPlugin_->UpdateInitDownloadData(&demuxerPlugin_->ioContext_, UINT32_MAX -1);
}

/**
 * @tc.name: Demuxer_GetNextSampleSize_001
 * @tc.desc: Test GetNextSampleSize
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetNextSampleSize_001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    int32_t size = 0;
    demuxerPlugin_->ioContext_.invokerType = InvokerTypeAlias::READ;
    demuxerPlugin_->GetNextSampleSize(0, size, 100);
}