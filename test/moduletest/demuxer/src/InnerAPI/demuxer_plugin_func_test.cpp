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

class DemuxerPluginInnerFuncTest : public testing::Test {
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
    bool PluginSelectTracks();
    bool PluginReadSample(uint32_t idx, uint32_t& flag);
    void CountFrames(uint32_t index, uint32_t flag);
    void SetEosValue();
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    void RemoveValue();
    bool ResultAssert(uint32_t frames0, uint32_t frames1, uint32_t keyFrames0, uint32_t keyFrames1);
    bool PluginReadAllSample();
    bool PluginSeekTo(int64_t seekTime, SeekMode mode);
    bool CreateDemuxerPluginByNotExistName(const std::string& typeName, const std::string& filePath, int probSize);

    int streamId_ = 0;
    std::map<uint32_t, uint32_t> frames_;
    std::map<uint32_t, uint32_t> keyFrames_;
    std::map<uint32_t, bool> eosFlag_;
    std::vector<uint32_t> selectedTrackIds_;
    std::vector<uint8_t> buffer_;

    std::shared_ptr<Media::StreamDemuxer> realStreamDemuxer_{ nullptr };
    std::shared_ptr<Media::MediaSource> mediaSource_{ nullptr };
    std::shared_ptr<Media::Source> realSource_{ nullptr };
    std::shared_ptr<Media::PluginBase> pluginBase_{ nullptr };
};

static const int BUFFER_PADDING_SIZE = 1024;
static const int DEF_PROB_SIZE = 16 * 1024;
constexpr int32_t THOUSAND = 1000.0;

static const std::string DEMUXER_PLUGIN_NAME_AAC = "avdemux_aac";
static const std::string DEMUXER_PLUGIN_NAME_AMR = "avdemux_amr";
static const std::string DEMUXER_PLUGIN_NAME_APE = "avdemux_ape";
static const std::string DEMUXER_PLUGIN_NAME_FLAC = "avdemux_flac";
static const std::string DEMUXER_PLUGIN_NAME_FLV = "avdemux_flv";
static const std::string DEMUXER_PLUGIN_NAME_MATROSKA = "avdemux_matroska,webm";
static const std::string DEMUXER_PLUGIN_NAME_MOV_S = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
static const std::string DEMUXER_PLUGIN_NAME_MP3 = "avdemux_mp3";
static const std::string DEMUXER_PLUGIN_NAME_MPEG = "avdemux_mpeg";
static const std::string DEMUXER_PLUGIN_NAME_MPEGTS = "avdemux_mpegts";
static const std::string DEMUXER_PLUGIN_NAME_AVI = "avdemux_avi";
static const std::string DEMUXER_PLUGIN_NAME_SRT = "avdemux_srt";
static const std::string DEMUXER_PLUGIN_NAME_WEBVTT = "avdemux_webvtt";
static const std::string DEMUXER_PLUGIN_NAME_OGG = "avdemux_ogg";
static const std::string DEMUXER_PLUGIN_NAME_WAV = "avdemux_wav";
static const std::string DEMUXER_PLUGIN_NAME_ERR = "avdemux_test";

static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_FILE_URI_AAC = TEST_FILE_PATH + "audio/AAC_48000_1.aac";
static const string TEST_FILE_URI_AMRNB = TEST_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_FILE_URI_AMRWB = TEST_FILE_PATH + "audio/amr_wb_16000_1.amr";
static const string TEST_FILE_URI_APE = TEST_FILE_PATH + "audio/ape.ape";
static const string TEST_FILE_URI_FLAC = TEST_FILE_PATH + "audio/FLAC_48000_1.flac";
static const string TEST_FILE_URI_FLV = TEST_FILE_PATH + "avc_mp3.flv";
static const string TEST_FILE_URI_MATROSKA = TEST_FILE_PATH + "mkv.mkv";
static const string TEST_FILE_URI_MOV = TEST_FILE_PATH + "MPEG4_ASP@3_352_288_30_MP3_32K_1.mov";
static const string TEST_FILE_URI_MP4 = TEST_FILE_PATH + "01_video_audio.mp4";
static const string TEST_FILE_URI_FMP4 = TEST_FILE_PATH + "mp3_h265_fmp4.mp4";
static const string TEST_FILE_URI_M4A = TEST_FILE_PATH + "audio/M4A_48000_1.m4a";
static const string TEST_FILE_URI_MP3 = TEST_FILE_PATH + "audio/MP3_48000_1.mp3";
static const string TEST_FILE_URI_MPEG = TEST_FILE_PATH + "MPEG2_422p_1280_720_60_MP3_32K_1.mpg";
static const string TEST_FILE_URI_MPEGTS = TEST_FILE_PATH + "h264_aac_640.ts";
static const string TEST_FILE_URI_AVI = TEST_FILE_PATH + "long.avi";
static const string TEST_FILE_URI_SRT = TEST_FILE_PATH + "srt_2800.srt";
static const string TEST_FILE_URI_WEBVTT = TEST_FILE_PATH + "webvtt_test.vtt";
static const string TEST_FILE_URI_OGG = TEST_FILE_PATH + "audio/OGG_48000_1.ogg";
static const string TEST_FILE_URI_WAV = TEST_FILE_PATH + "audio/wav_48000_1.wav";
static const string TEST_GLTF_H264_MP4 = TEST_FILE_PATH + "test_264_B_Gop25_4sec_gltf.mp4";
static const string TEST_GLTF_H265_MP4 = TEST_FILE_PATH + "h265_mp3_gltf.mp4";
static const string TEST_GLTF_H266_MP4 = TEST_FILE_PATH + "vvc_1280_720_8_gltf.mp4";
static const string TEST_GLTF_HDRVIVID_MP4 = TEST_FILE_PATH + "demuxer_parser_hdr_vivid_gltf.mp4";
static const string TEST_GLTF_MPEG4_MP4 = TEST_FILE_PATH + "mpeg4_aac_gltf.mp4";

void DemuxerPluginInnerFuncTest::SetUpTestCase(void) {}

void DemuxerPluginInnerFuncTest::TearDownTestCase(void) {}

void DemuxerPluginInnerFuncTest::SetUp(void)
{
}

void DemuxerPluginInnerFuncTest::TearDown(void)
{
    dataSourceImpl_ = nullptr;
}

bool DemuxerPluginInnerFuncTest::CreateDataSource(const std::string& filePath)
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
    dataSourceImpl_->stream_ = realStreamDemuxer_;
    realSource_->NotifyInitSuccess();

    return true;
}

bool DemuxerPluginInnerFuncTest::CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath,
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

bool DemuxerPluginInnerFuncTest::CreateDemuxerPluginByNotExistName(const std::string& typeName,
    const std::string& filePath, int probSize)
{
    if (!CreateDataSource(filePath)) {
        printf("false return: CreateDataSource is fail\n");
        return false;
    }
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(typeName);
    if (!(pluginBase_ != nullptr)) {
        printf("false return:" "pluginBase_ == nullptr\n");
        return false;
    }
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (!(demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, probSize) == Status::ERROR_INVALID_PARAMETER)) {
        printf("false return: demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, probSize) !="
            "Status::ERROR_INVALID_PARAMETER\n");
        return false;
    }
    return true;
}

bool DemuxerPluginInnerFuncTest::PluginSelectTracks()
{
    MediaInfo mediaInfo;
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (!(demuxerPlugin->GetMediaInfo(mediaInfo) == Status::OK)) {
        printf("false return: demuxerPlugin->GetMediaInfo(mediaInfo) != Status::OK\n");
        return false;
    }
    for (auto i = 0; i < mediaInfo.tracks.size(); i++) {
        std::string mime;
        mediaInfo.tracks[i].GetData(Tag::MIME_TYPE, mime);
        if (mime.find("video/") == 0 || mime.find("audio/") == 0 ||
            mime.find("application/") == 0 || mime.find("text/vtt") == 0) {
            if (mime.find("video/") == 0) {
                printf("the video track id is %d\n", i);
            }
            if (mime.find("audio/") == 0) {
                printf("the audio track id is %d\n", i);
            }
            demuxerPlugin->SelectTrack(static_cast<uint32_t>(i));
            selectedTrackIds_.push_back(static_cast<uint32_t>(i));
            frames_[i] = 0;
            keyFrames_[i] = 0;
            eosFlag_[i] = false;
        }
    }

    return true;
}

bool DemuxerPluginInnerFuncTest::PluginReadSample(uint32_t idx, uint32_t& flag)
{
    int bufSize = 0;
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    demuxerPlugin->GetNextSampleSize(idx, bufSize);
    if (bufSize > buffer_.size()) {
        buffer_.resize(bufSize + BUFFER_PADDING_SIZE);
    }

    auto avBuf = AVBuffer::CreateAVBuffer(buffer_.data(), bufSize, bufSize);
    if (!(avBuf != nullptr)) {
        printf("false return: avBuf == nullptr\n");
        return false;
    }
    demuxerPlugin->ReadSample(idx, avBuf);
    flag = avBuf->flag_;

    return true;
}

bool DemuxerPluginInnerFuncTest::PluginSeekTo(int64_t seekTime, SeekMode mode)
{
    int64_t realSeekTime = 0;
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (!(demuxerPlugin->SeekTo(0, seekTime / THOUSAND, mode, realSeekTime) == Status::OK)) {
        printf("false return: demuxerPlugin->SeekTo(0, seekTime / THOUSAND, mode, realSeekTime) != Status::OK\n");
        return false;
    }
    PluginReadAllSample();
    return true;
}

void DemuxerPluginInnerFuncTest::CountFrames(uint32_t index, uint32_t flag)
{
    if (flag & static_cast<uint32_t>(AVBufferFlag::EOS)) {
        eosFlag_[index] = true;
        return;
    }

    if (flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) {
        keyFrames_[index]++;
        frames_[index]++;
    } else if ((flag & static_cast<uint32_t>(AVBufferFlag::NONE)) == static_cast<uint32_t>(AVBufferFlag::NONE)) {
        frames_[index]++;
    } else {
        SetEosValue();
        printf("flag is unknown, read sample break");
    }
}

bool DemuxerPluginInnerFuncTest::isEOS(map<uint32_t, bool>& countFlag)
{
    auto ret = std::find_if(countFlag.begin(), countFlag.end(), [](const pair<uint32_t, bool>& p) {
        return p.second == false;
    });

    return ret == countFlag.end();
}

void DemuxerPluginInnerFuncTest::SetEosValue()
{
    std::for_each(selectedTrackIds_.begin(), selectedTrackIds_.end(), [this](uint32_t idx) {
        eosFlag_[idx] = true;
    });
}

void DemuxerPluginInnerFuncTest::RemoveValue()
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

bool DemuxerPluginInnerFuncTest::ResultAssert(uint32_t frames0, uint32_t frames1, uint32_t keyFrames0,
    uint32_t keyFrames1)
{
    printf("frames_[0]=%d | kFrames[0]=%d\n", frames_[0], keyFrames_[0]);
    printf("frames_[1]=%d | kFrames[1]=%d\n", frames_[1], keyFrames_[1]);
    if (!(frames_[0] == frames0)) {
        printf("false return: frames_[0] != frames0\n");
        return false;
    }
    if (!(frames_[1] == frames1)) {
        printf("false return: frames_[1] =!= frames1\n");
        return false;
    }
    if (!(keyFrames_[0] == keyFrames0)) {
        printf("false return: keyFrames_[0] != keyFrames0\n");
        return false;
    }
    if (!(keyFrames_[1] == keyFrames1)) {
        printf("false return: keyFrames_[1] != keyFrames1\n");
        return false;
    }
    return true;
}

bool DemuxerPluginInnerFuncTest::PluginReadAllSample()
{
    while (!isEOS(eosFlag_)) {
        for (auto idx : selectedTrackIds_) {
            uint32_t flag = 0;
            if (!PluginReadSample(idx, flag)) {
                printf("false return: PluginReadSample(idx, flag) is fail\n");
                return false;
            }
            CountFrames(idx, flag);
        }
    }

    return true;
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0001
 * @tc.name      : SetDataSourceWithProbSize, const std::shared_ptr<DataSource>& source is nullptr
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0001, TestSize.Level1)
{
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(DEMUXER_PLUGIN_NAME_AAC);
    ASSERT_NE(pluginBase_, nullptr);

    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SetDataSourceWithProbSize(nullptr, DEF_PROB_SIZE), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0002
 * @tc.name      : SetDataSourceWithProbSize, const int probSize < 0
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0002, TestSize.Level1)
{
    ASSERT_EQ(CreateDataSource(TEST_FILE_URI_AAC), true);
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(DEMUXER_PLUGIN_NAME_AAC);
    ASSERT_NE(pluginBase_, nullptr);

    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, -1), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0003
 * @tc.name      : SetDataSourceWithProbSize, formatContext_ is nullptr
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0003, TestSize.Level2)
{
    ASSERT_EQ(CreateDataSource(TEST_FILE_URI_AAC), true);
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(DEMUXER_PLUGIN_NAME_AAC);
    ASSERT_NE(pluginBase_, nullptr);

    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, DEF_PROB_SIZE),
        Status::OK);
    ASSERT_EQ(demuxerPlugin->SetDataSourceWithProbSize(dataSourceImpl_, DEF_PROB_SIZE),
        Status::ERROR_WRONG_STATE);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0004
 * @tc.name      : CreatePluginByName, name is empty string
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_API_0004, TestSize.Level2)
{
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName("");
    ASSERT_EQ(pluginBase_, nullptr);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0001
 * @tc.name      : create plugin by avdemux_aac, container format is aac, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0001, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(9457, 0, 9457, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(0, Plugins::SeekMode::SEEK_NEXT_SYNC), true);
    ASSERT_EQ(ResultAssert(9457, 0, 9457, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0002
 * @tc.name      : create plugin by avdemux_flv, container format is flv, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0002, TestSize.Level0)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(602, 385, 3, 385), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(5042000, Plugins::SeekMode::SEEK_PREVIOUS_SYNC), true);
    ASSERT_EQ(ResultAssert(352, 225, 2, 225), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0003
 * @tc.name      : create plugin by avdemux_matroska,webm, container format is mkv, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0003, TestSize.Level0)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(600, 431, 10, 431), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(9983000, Plugins::SeekMode::SEEK_CLOSEST_SYNC), true);
    ASSERT_EQ(ResultAssert(60, 43, 1, 43), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0004
 * @tc.name      : create plugin by avdemux_mov,mp4,m4a,3gp,3g2,mj2, container format is mov, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0004, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(60, 57, 6, 57), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(0, Plugins::SeekMode::SEEK_NEXT_SYNC), true);
    ASSERT_EQ(ResultAssert(60, 57, 6, 57), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0005
 * @tc.name      : create plugin by avdemux_mov,mp4,m4a,3gp,3g2,mj2, container format is fmp4, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0005, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(372, 468, 2, 468), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(6166341, Plugins::SeekMode::SEEK_PREVIOUS_SYNC), true);
    ASSERT_EQ(ResultAssert(372, 468, 2, 468), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0006
 * @tc.name      : create plugin by avdemux_mov,mp4,m4a,3gp,3g2,mj2, container format is mp4, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0006, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(384, 250, 384, 250), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(9960000, Plugins::SeekMode::SEEK_CLOSEST_SYNC), true);
    ASSERT_EQ(ResultAssert(2, 1, 2, 1), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0007
 * @tc.name      : create plugin by avdemux_mov,mp4,m4a,3gp,3g2,mj2, container format is m4a, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0007, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(10293, 0, 10293, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(0, Plugins::SeekMode::SEEK_NEXT_SYNC), true);
    ASSERT_EQ(ResultAssert(10292, 0, 10292, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0008
 * @tc.name      : create plugin by avdemux_amr, container format is amr, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0008, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1501, 0, 1501, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(15000000, Plugins::SeekMode::SEEK_PREVIOUS_SYNC), true);
    ASSERT_EQ(ResultAssert(751, 0, 751, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(1500, 0, 1500, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(29980000, Plugins::SeekMode::SEEK_CLOSEST_SYNC), true);
    ASSERT_EQ(ResultAssert(1, 0, 1, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0009
 * @tc.name      : create plugin by avdemux_ape, container format is ape, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0009, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(8, 0, 8, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(0, Plugins::SeekMode::SEEK_NEXT_SYNC), true);
    ASSERT_EQ(ResultAssert(8, 0, 8, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0010
 * @tc.name      : create plugin by avdemux_flac, container format is flac, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0010, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(2288, 0, 2288, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(109824000, Plugins::SeekMode::SEEK_PREVIOUS_SYNC), true);
    ASSERT_GE(frames_[0], 1143);
    ASSERT_LE(frames_[0], 1145);
    ASSERT_GE(keyFrames_[0], 1143);
    ASSERT_LE(keyFrames_[0], 1145);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0011
 * @tc.name      : create plugin by avdemux_mp3, container format is mp3, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0011, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(9150, 0, 9150, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(219576000, Plugins::SeekMode::SEEK_CLOSEST_SYNC), true);
    ASSERT_EQ(ResultAssert(15, 0, 15, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0012
 * @tc.name      : create plugin by avdemux_mpeg, container format is mpg, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0012, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(119, 57, 12, 57), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(0, Plugins::SeekMode::SEEK_NEXT_SYNC), true);
    ASSERT_EQ(ResultAssert(118, 47, 11, 47), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0013
 * @tc.name      : create plugin by avdemux_mpegts, container format is ts, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0013, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(123, 176, 1, 176), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(3500000, Plugins::SeekMode::SEEK_PREVIOUS_SYNC), true);
    ASSERT_EQ(ResultAssert(16, 29, 0, 29), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0014
 * @tc.name      : create plugin by avdemux_avi, container format is avi, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0014, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(3, 9, 1, 9), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(200000, Plugins::SeekMode::SEEK_CLOSEST_SYNC), true);
    ASSERT_EQ(ResultAssert(3, 9, 1, 9), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0015
 * @tc.name      : create plugin by avdemux_ogg, container format is ogg, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0015, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(11439, 0, 11439, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(0, Plugins::SeekMode::SEEK_NEXT_SYNC), true);
    ASSERT_EQ(ResultAssert(11439, 0, 11439, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0016
 * @tc.name      : create plugin by avdemux_wav, container format is wav, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0016, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(5146, 0, 5146, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(109781333, Plugins::SeekMode::SEEK_PREVIOUS_SYNC), true);
    ASSERT_EQ(ResultAssert(2573, 0, 2573, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0017
 * @tc.name      : create plugin by avdemux_srt, container format is srt, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0017, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(10, 0, 10, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(31900000, Plugins::SeekMode::SEEK_CLOSEST_SYNC), true);
    ASSERT_EQ(ResultAssert(1, 0, 1, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0018
 * @tc.name      : create plugin by avdemux_webvtt, container format is vtt, read and seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0018, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginReadAllSample(), true);
    ASSERT_EQ(ResultAssert(8, 0, 8, 0), true);
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    ASSERT_EQ(PluginSeekTo(0, Plugins::SeekMode::SEEK_NEXT_SYNC), true);
    ASSERT_EQ(ResultAssert(8, 0, 8, 0), true);
    RemoveValue();
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0019
 * @tc.name      : create plugin by not exist name
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0019, TestSize.Level2)
{
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(DEMUXER_PLUGIN_NAME_ERR);
    ASSERT_EQ(pluginBase_, nullptr);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0020
 * @tc.name      : create plugin by avdemux_aac, container format is not aac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0020, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0021
 * @tc.name      : create plugin by avdemux_amr, container format is not amr
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0021, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0022
 * @tc.name      : create plugin by avdemux_ape, container format is not ape
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0022, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0023
 * @tc.name      : create plugin by avdemux_flac, container format is not flac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0023, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0024
 * @tc.name      : create plugin by avdemux_matroska,webm, container format is not mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0024, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE),
        true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE),
        true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE),
        true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE),
        true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0025
 * @tc.name      : create plugin by avdemux_mov,mp4,m4a,3gp,3g2,mj2, container format is not mov/fmp4/mp4/m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0025, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE),
        true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0026
 * @tc.name      : create plugin by avdemux_mp3, container format is not mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0026, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0027
 * @tc.name      : create plugin by avdemux_mpeg, container format is not mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0027, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0028
 * @tc.name      : create plugin by avdemux_mpegts, container format is not ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0028, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE),
        true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0029
 * @tc.name      : create plugin by avdemux_avi, container format is not avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0029, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0030
 * @tc.name      : create plugin by avdemux_srt, container format is not srt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0030, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0031
 * @tc.name      : create plugin by avdemux_webvtt, container format is not vtt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0031, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE),
        true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0032
 * @tc.name      : create plugin by avdemux_ogg, container format is not ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0032, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0033
 * @tc.name      : create plugin by avdemux_wav, container format is not wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0033, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_FLV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0034
 * @tc.name      : create plugin by avdemux_flv, container format is not flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0034, TestSize.Level0)
{
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_AMRNB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_AMRWB, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_APE, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_WAV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_MOV, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_M4A, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_MATROSKA, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_MPEG, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_MPEGTS, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_AVI, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_SRT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_WEBVTT, DEF_PROB_SIZE), true);
    ASSERT_EQ(CreateDemuxerPluginByNotExistName(DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
}

/**
 * @tc.number    : DEMUXER_GLTF_INNER_FUNC_0010
 * @tc.name      : h264 GLTF
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_GLTF_INNER_FUNC_0010, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_GLTF_H264_MP4, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    MediaInfo mediaInfo;
    ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo), Status::OK);
    Meta generalMeta = mediaInfo.general;
    bool isGltfValue = 0;
    int64_t glftOffsetValue = 0;
    string stringVal = "";
    float longval = 0.0;
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, longval));
    ASSERT_TRUE(generalMeta.GetData(Tag::IS_GLTF, isGltfValue));
    ASSERT_TRUE(generalMeta.GetData(Tag::GLTF_OFFSET, glftOffsetValue));
    ASSERT_TRUE(isGltfValue);
    int64_t expectedVal = 66252;
    ASSERT_EQ(expectedVal, glftOffsetValue);
}

/**
 * @tc.number    : DEMUXER_GLTF_INNER_FUNC_0020
 * @tc.name      : h265 GLTF
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_GLTF_INNER_FUNC_0020, TestSize.Level1)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_GLTF_H265_MP4, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    MediaInfo mediaInfo;
    ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo), Status::OK);
    Meta generalMeta = mediaInfo.general;
    bool isGltfValue = 0;
    int64_t glftOffsetValue = 0;
    string stringVal = "";
    float longval = 0.0;
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, longval));
    ASSERT_TRUE(generalMeta.GetData(Tag::IS_GLTF, isGltfValue));
    ASSERT_TRUE(generalMeta.GetData(Tag::GLTF_OFFSET, glftOffsetValue));
    ASSERT_TRUE(isGltfValue);
    int64_t expectedVal = 38882;
    ASSERT_EQ(expectedVal, glftOffsetValue);
}

/**
 * @tc.number    : DEMUXER_GLTF_INNER_FUNC_0030
 * @tc.name      : h266 GLTF
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_GLTF_INNER_FUNC_0030, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_GLTF_H266_MP4, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    MediaInfo mediaInfo;
    ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo), Status::OK);
    Meta generalMeta = mediaInfo.general;
    bool isGltfValue = 0;
    int64_t glftOffsetValue = 0;
    string stringVal = "";
    float longval = 0.0;
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, longval));
    ASSERT_TRUE(generalMeta.GetData(Tag::IS_GLTF, isGltfValue));
    ASSERT_TRUE(generalMeta.GetData(Tag::GLTF_OFFSET, glftOffsetValue));
    ASSERT_TRUE(isGltfValue);
    int64_t expectedVal = 20268;
    ASSERT_EQ(expectedVal, glftOffsetValue);
}

/**
 * @tc.number    : DEMUXER_GLTF_INNER_FUNC_0040
 * @tc.name      : hdrvivid GLTF
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_GLTF_INNER_FUNC_0040, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_GLTF_HDRVIVID_MP4, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    MediaInfo mediaInfo;
    ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo), Status::OK);
    Meta generalMeta = mediaInfo.general;
    bool isGltfValue = 0;
    int64_t glftOffsetValue = 0;
    string stringVal = "";
    float longval = 0.0;
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, longval));
    ASSERT_TRUE(generalMeta.GetData(Tag::IS_GLTF, isGltfValue));
    ASSERT_TRUE(generalMeta.GetData(Tag::GLTF_OFFSET, glftOffsetValue));
    ASSERT_TRUE(isGltfValue);
    int64_t expectedVal = 209563;
    ASSERT_EQ(expectedVal, glftOffsetValue);
}

/**
 * @tc.number    : DEMUXER_GLTF_INNER_FUNC_0050
 * @tc.name      : mpeg4 GLTF
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_GLTF_INNER_FUNC_0050, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_GLTF_MPEG4_MP4, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    MediaInfo mediaInfo;
    ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo), Status::OK);
    Meta generalMeta = mediaInfo.general;
    bool isGltfValue = 0;
    int64_t glftOffsetValue = 0;
    string stringVal = "";
    float longval = 0.0;
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, longval));
    ASSERT_TRUE(generalMeta.GetData(Tag::IS_GLTF, isGltfValue));
    ASSERT_TRUE(generalMeta.GetData(Tag::GLTF_OFFSET, glftOffsetValue));
    ASSERT_TRUE(isGltfValue);
    int64_t expectedVal = 60799;
    ASSERT_EQ(expectedVal, glftOffsetValue);
}

/**
 * @tc.number    : DEMUXER_GLTF_INNER_FUNC_0060
 * @tc.name      : 不含 GLTF
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_GLTF_INNER_FUNC_0060, TestSize.Level3)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    MediaInfo mediaInfo;
    ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo), Status::OK);
    Meta generalMeta = mediaInfo.general;
    bool isGltfValue = 0;
    int64_t glftOffsetValue = 0;
    string stringVal = "";
    float longval = 0.0;
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, isGltfValue));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, glftOffsetValue));
}


/**
 * @tc.number    : DEMUXER_GLTF_INNER_FUNC_0070
 * @tc.name      : 不含 GLTF
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFuncTest, DEMUXER_GLTF_INNER_FUNC_0070, TestSize.Level3)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_FMP4, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto plugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    MediaInfo mediaInfo;
    ASSERT_EQ(plugin_->GetMediaInfo(mediaInfo), Status::OK);
    Meta generalMeta = mediaInfo.general;
    bool isGltfValue = 0;
    int64_t glftOffsetValue = 0;
    string stringVal = "";
    float longval = 0.0;
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, stringVal));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, longval));
    ASSERT_FALSE(generalMeta.GetData(Tag::IS_GLTF, isGltfValue));
    ASSERT_FALSE(generalMeta.GetData(Tag::GLTF_OFFSET, glftOffsetValue));
}
}  // namespace Media
}  // namespace OHOS