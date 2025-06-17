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

#include "gtest/gtest.h"
#include "demuxer_plugin_manager.h"
#include "stream_demuxer.h"
#include "plugin/plugin_manager_v2.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;

namespace OHOS {
namespace Media {

class DemuxerAsynInnerFuncTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

protected:
    std::shared_ptr<DataSourceImpl> dataSourceImpl_{ nullptr };

private:
    bool CreateDataSource(const std::string& filePath);
    bool CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath, int probSize);
    void RemoveValue();
    bool CreateBufferSize();
    void GetFrameNum(int32_t i);
    int streamId_ = 0;
    std::map<uint32_t, uint32_t> frames_;
    std::map<uint32_t, uint32_t> keyFrames_;
    std::map<uint32_t, bool> eosFlag_;

    std::shared_ptr<Media::StreamDemuxer> realStreamDemuxer_{ nullptr };
    std::shared_ptr<Media::MediaSource> mediaSource_{ nullptr };
    std::shared_ptr<Media::Source> realSource_{ nullptr };
    std::shared_ptr<Media::PluginBase> pluginBase_{ nullptr };
    std::shared_ptr<AVBuffer> avBuf_{ nullptr };
    uint32_t indexVid = 0;
    uint32_t indexAud = 0;
    int32_t readPos = 0;
    int32_t unSelectTrack = 0;
    bool isVideoEosFlagForSave = false;
    bool isAudioEosFlagForSave = false;
    int32_t videoTrackIdx = 0;
    int32_t audioTrackIdx = 1;
    uint32_t videoIndexForRead = 0;
    uint32_t audioIndexForRead = 0;
};

static const int DEF_PROB_SIZE = 16 * 1024;
constexpr int32_t THOUSAND = 1000.0;

static const std::string DEMUXER_PLUGIN_NAME_FLV = "avdemux_flv";


static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_FILE_URI_FLV = TEST_FILE_PATH + "avc_aac_60.flv";
static const string TEST_FILE_URI_FLV_1 = TEST_FILE_PATH + "avc_60.flv";
static const string TEST_FILE_URI_FLV_2 = TEST_FILE_PATH + "avc_mp3.flv";
void DemuxerAsynInnerFuncTest::SetUpTestCase(void) {}

void DemuxerAsynInnerFuncTest::TearDownTestCase(void) {}

void DemuxerAsynInnerFuncTest::SetUp(void)
{
}

void DemuxerAsynInnerFuncTest::TearDown(void)
{
    dataSourceImpl_ = nullptr;
}

bool DemuxerAsynInnerFuncTest::CreateBufferSize()
{
    uint32_t buffersize = 1024*1024;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuf_ = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    if (!avBuf_) {
        return false;
    }
    return true;
}
bool DemuxerAsynInnerFuncTest::CreateDataSource(const std::string &filePath)
{
    mediaSource_ = std::make_shared<MediaSource>(filePath);
    realSource_ = std::make_shared<Source>();
    realSource_->SetSource(mediaSource_);

    realStreamDemuxer_ = std::make_shared<StreamDemuxer>();
    realStreamDemuxer_->SetSourceType(Plugins::SourceType::SOURCE_TYPE_FD);
    realStreamDemuxer_->SetSource(realSource_);
    realStreamDemuxer_->Init(filePath);

    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_HEADER);
    dataSourceImpl_ = std::make_shared<DataSourceImpl>(realStreamDemuxer_, streamId_);
    dataSourceImpl_->stream_ = realStreamDemuxer_;
    realSource_->NotifyInitSuccess();

    return true;
}

bool DemuxerAsynInnerFuncTest::CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath,
    int probSize)
{
    if (!CreateDataSource(filePath)) {
        printf("false return: CreateDataSource is fail\n");
        return false;
    }
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(typeName);
    if (!(pluginBase_ != nullptr)) {
        printf("false return: pluginBase_ == nullptr\n");
        return false;
    }
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (!(demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, probSize) == Status::OK)) {
        printf("false return: demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, probSize) != Status::OK\n");
        return false;
    }
    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);

    return true;
}

void DemuxerAsynInnerFuncTest::RemoveValue()
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

void DemuxerAsynInnerFuncTest::GetFrameNum(int32_t i)
{
    if (avBuf_->flag_ == MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) {
        if (i == videoTrackIdx) {
            isVideoEosFlagForSave = true;
        } else {
            isAudioEosFlagForSave = true;
        }
    } else {
        if (i == videoTrackIdx) {
            videoIndexForRead++;
        } else {
            audioIndexForRead++;
        }
    }
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0010
 * @tc.name      : trackId为 UNINT32_T_MAX
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0010, TestSize.Level0)
{
    indexVid = 4294967295;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0020
 * @tc.name      : buffer为nullptr
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0020, TestSize.Level0)
{
    indexVid = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, nullptr, timeout), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0030
 * @tc.name      : timeout为 UNINT32_T_MAX
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0030, TestSize.Level0)
{
    indexVid = 0;
    uint32_t timeout = 4294967295;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0040
 * @tc.name      : trackId为 UNINT32_T_MAX
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0040, TestSize.Level0)
{
    indexVid = 4294967295;
    uint32_t timeout = 10;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size, timeout), Status::ERROR_UNKNOWN);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0050
 * @tc.name      : timeout为 UNINT32_T_MAX
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0050, TestSize.Level0)
{
    indexVid = 0;
    uint32_t timeout = 4294967295;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size, timeout), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0060
 * @tc.name      : trackID为 UNINT32_T_MAX
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0060, TestSize.Level0)
{
    indexVid = 4294967295;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexVid, pts), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0070
 * @tc.name      : ReadSample全流程
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0070, TestSize.Level1)
{
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(videoIndexForRead, 307);
    ASSERT_EQ(audioIndexForRead, 318);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0080
 * @tc.name      : ReadSample,一条轨，timeout = 0
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0080, TestSize.Level1)
{
    indexVid = 0;
    uint32_t timeout = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV_1, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::ERROR_WAIT_TIMEOUT);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0090
 * @tc.name      : read , timeout > 1帧读取时间
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0090, TestSize.Level1)
{
    indexVid = 0;
    uint32_t timeout = 1;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0100
 * @tc.name      : GetNextSampleSize,读取视频轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0100, TestSize.Level1)
{
    int32_t size = 0;
    indexVid = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size, timeout), Status::OK);
    ASSERT_EQ(size, 136906);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0110
 * @tc.name      : GetNextSampleSize,读取音频轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0110, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 1;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(size, 304);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0120
 * @tc.name      : GetNextSampleSize,读取不存在轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0120, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 7;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_UNKNOWN);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0130
 * @tc.name      : GetLastPTSByTrackId,读取视频轨，得到音频轨lastpts
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0130, TestSize.Level2)
{
    indexVid = 0;
    indexAud = 1;
    readPos = 30;
    int32_t readCount = 0;
    int64_t pts = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    while (true) {
        if (readCount >= readPos) {
            ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::OK);
            break;
        } else {
            readCount++;
            ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
        }
    }
    ASSERT_EQ(pts, 1087);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0140
 * @tc.name      : GetLastPTSByTrackId,读取音频轨，得到视频轨lastpts
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0140, TestSize.Level2)
{
    indexVid = 0;
    indexAud = 1;
    readPos = 30;
    int32_t readCount = 0;
    int64_t pts = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    while (true) {
        if (readCount >= readPos) {
            ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexVid, pts), Status::OK);
            break;
        } else {
            readCount++;
            ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        }
    }
    ASSERT_EQ(pts, 1293);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0150
 * @tc.name      : GetLastPTSByTrackId,不选轨道
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0150, TestSize.Level2)
{
    indexVid = 0;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexVid, pts), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0160
 * @tc.name      : GetLastPTSByTrackId,入参的id没有被选择
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0160, TestSize.Level2)
{
    indexVid = 0;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexVid, pts), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0170
 * @tc.name      : read-pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0170, TestSize.Level3)
{
    indexVid = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(audioIndexForRead, 318);
    ASSERT_EQ(videoIndexForRead, 306);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0180
 * @tc.name      : read-pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0180, TestSize.Level3)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int64_t realtime = 0;
    int64_t seekTime = 5042000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV_2, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexVid, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(audioIndexForRead, 65);
    ASSERT_EQ(videoIndexForRead, 102);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0190
 * @tc.name      : pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0190, TestSize.Level3)
{
    indexVid = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(audioIndexForRead, 318);
    ASSERT_EQ(videoIndexForRead, 307);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0200
 * @tc.name      : pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0200, TestSize.Level3)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int64_t realtime = 0;
    int64_t seekTime = 5042000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV_2, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexVid, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(audioIndexForRead, 65);
    ASSERT_EQ(videoIndexForRead, 102);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0210
 * @tc.name      : 未选择轨，pause
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0210, TestSize.Level3)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0220
 * @tc.name      : seek + read 
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0220, TestSize.Level3)
{
    indexVid = 0;
    indexAud = 1;
    uint32_t timeout = 10;
    int64_t seekTime = 5042000;
    int64_t realtime = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV_2, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexVid, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(audioIndexForRead, 65);
    ASSERT_EQ(videoIndexForRead, 102);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0230
 * @tc.name      : seek后清理缓存
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0230, TestSize.Level3)
{
    indexVid = 0;
    indexAud = 1;
    uint32_t timeout = 10;
    int64_t realtime = 0;
    int64_t seekTime = 5042000;
    int64_t pts = 0;
    int32_t readCount = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV_2, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    while (true) {
        if (readCount >= readPos) {
            ASSERT_EQ(demuxerPlugin->SeekTo(
                indexVid, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
            ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
            break;
        } else {
            readCount++;
            ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
        }
    }
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0240
 * @tc.name      : >pts, SEEK_NEXT_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0240, TestSize.Level2)
{
    indexVid = 0;
    int64_t realtime = 0;
    int64_t seekTime = 10458000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexVid, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::ERROR_UNKNOWN);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0250
 * @tc.name      : >pts, SEEK_PREVIOUS_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0250, TestSize.Level2)
{
    indexVid = 0;
    int64_t realtime = 0;
    int64_t seekTime = 10360000;
    uint32_t timeout = 10;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexVid, seekTime / THOUSAND, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexVid, pts), Status::ERROR_NOT_EXISTED);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(audioIndexForRead, 10);
    ASSERT_EQ(videoIndexForRead, 7);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0260
 * @tc.name      : >pts, SEEK_CLOSEST_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0260, TestSize.Level2)
{
    indexVid = 0;
    int64_t realtime = 0;
    int64_t seekTime = 10360000;
    uint32_t timeout = 10;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexVid, seekTime / THOUSAND, Plugins::SeekMode::SEEK_CLOSEST_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexVid, pts), Status::ERROR_NOT_EXISTED);
    while (!isAudioEosFlagForSave || !isVideoEosFlagForSave) {
        for (int32_t i = 0;i < 2; i++) {
            if (((i == videoTrackIdx) && isVideoEosFlagForSave) || ((i == audioTrackIdx) && isAudioEosFlagForSave)) {
                continue;
            }
            ASSERT_EQ(demuxerPlugin->ReadSample(i, avBuf_, timeout), Status::OK);
            GetFrameNum(i);
        }
    }
    ASSERT_EQ(audioIndexForRead, 10);
    ASSERT_EQ(videoIndexForRead, 7);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0270
 * @tc.name      : 老ReadSample + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0270, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0280
 * @tc.name      : 老ReadSample + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0280, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0290
 * @tc.name      : 老 GetNextSampleSize + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0290, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0300
 * @tc.name      : 老 GetNextSampleSize + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0300, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0310
 * @tc.name      : 新 ReadSample + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0310, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0320
 * @tc.name      : 新 ReadSample + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0320, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0330
 * @tc.name      : 新 GetNextSampleSize + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0330, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexVid, avBuf_), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FUNC_0340
 * @tc.name      : 新 GetNextSampleSize + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynInnerFuncTest, DEMUXER_ASYN_INNER_FUNC_0340, TestSize.Level2)
{
    indexVid = 0;
    uint32_t timeout = 10;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexVid, size), Status::ERROR_INVALID_OPERATION);
}
}  // namespace Media
}  // namespace OHOS