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

class DemuxerAsynTypeInnerFunc6Test : public testing::Test {
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
    void GetFrameNumAudio(int32_t i);
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
    bool isSubtitleEosFlagForSave = false;
    bool isAudioEosFlagForSave = false;
    bool isMetaEosFlagForSave = false;
    bool isAuxDepthEosFlagForSave = false;
    bool isAuxPreyEosFlagForSave = false;
    int32_t videoTrackIdx = 0;
    int32_t audioTrackIdx = 1;
    int32_t subtitleTrackIdx = 2;
    int32_t metaTrackIdx = 3;
    int32_t auxiliaryTrackIdx = 4;
    uint32_t videoIndexForRead = 0;
    uint32_t audioIndexForRead = 0;
    uint32_t subtitleIndexForRead = 0;
    uint32_t metaIndexForRead = 0;
    uint32_t auxPreyIndexForRead = 0;
    uint32_t auxDepthIndexForRead = 0;
};

static const int DEF_PROB_SIZE = 16 * 1024;
constexpr int32_t THOUSAND = 1000.0;
constexpr uint32_t READFRAME_9456 = 9456;
constexpr uint32_t READFRAME_9449 = 9449;
constexpr uint32_t READFRAME_9150 = 9150;
constexpr uint32_t READFRAME_9457 = 9457;
constexpr uint32_t READFRAME_192 = 192;
constexpr uint32_t READFRAME_9149 = 9149;
constexpr uint32_t READFRAME_9133 = 9133;
constexpr uint32_t READFRAME_6862 = 6862;
constexpr uint32_t READFRAME_6831 = 6831;
constexpr uint32_t READFRAME_6863 = 6863;
constexpr uint32_t READFRAME_3049 = 3049;
constexpr uint32_t READFRAME_3035 = 3035;
constexpr uint32_t READFRAME_3050 = 3050;
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


static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_FILE_URI_AAC = TEST_FILE_PATH + "audio/AAC_48000_1.aac";
static const string TEST_FILE_URI_AMR = TEST_FILE_PATH + "audio/amr_nb_8000_1.aac";
static const string TEST_FILE_URI_AMRNB = TEST_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_FILE_URI_AMRWB = TEST_FILE_PATH + "audio/amr_wb_16000_1.amr";
static const string TEST_FILE_URI_APE = TEST_FILE_PATH + "audio/ape.ape";
static const string TEST_FILE_URI_FLAC = TEST_FILE_PATH + "audio/gb18030.flac";
static const string TEST_FILE_URI_FLV_1 = TEST_FILE_PATH + "aac_h265.flv";
static const string TEST_FILE_URI_MOV = TEST_FILE_PATH + "H264_main@3_720_480_30_PCM_48K_24_1.mov";
static const string TEST_FILE_URI_MOV_1 = TEST_FILE_PATH + "H264_base@5_1920_1080_30_AAC_48K_1.mov";
static const string TEST_FILE_URI_MOV_2 = TEST_FILE_PATH + "H264_high@5.1_3840_2160_30_MP3_48K_1.mov";
static const string TEST_FILE_URI_MOV_3 = TEST_FILE_PATH + "H265_main@5_3840_2160_30_vorbis_48K_1.mov";
static const string TEST_FILE_URI_MOV_4 = TEST_FILE_PATH + "MPEG4_SP@6_1280_720_30_MP2_32K_2.mov";

static const string TEST_FILE_URI_M4A = TEST_FILE_PATH + "audio/gb2312.m4a";
static const string TEST_FILE_URI_MP3 = TEST_FILE_PATH + "audio/MP3_48000_1.mp3";
static const string TEST_FILE_URI_MPEG = TEST_FILE_PATH + "MPEG2_high_720_480_30_MP2_32K_2.mpg";
static const string TEST_FILE_URI_MPEG_1 = TEST_FILE_PATH + "H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg";
static const string TEST_FILE_URI_MPEG_2 = TEST_FILE_PATH + "MPEG2_main_352_288_30_MP2_44.1K_1.mpg";
static const string TEST_FILE_URI_MPEGTS = TEST_FILE_PATH + "mp3_h264.ts";
static const string TEST_FILE_URI_MPEGTS_1 = TEST_FILE_PATH + "aac_h265.ts";
static const string TEST_FILE_URI_MPEGTS_2 = TEST_FILE_PATH + "aac_mpeg2.ts";
static const string TEST_FILE_URI_MPEGTS_3 = TEST_FILE_PATH + "mp3_mpeg4.ts";

static const string TEST_FILE_URI_AVI = TEST_FILE_PATH + "AVI_MPEG2_main@mian_640_480_MP2_1.avi";
static const string TEST_FILE_URI_AVI_1 = TEST_FILE_PATH + "AVI_H263_baseline@level10_352_288_AAC_2.avi";
static const string TEST_FILE_URI_AVI_2 = TEST_FILE_PATH + "AVI_H264_high@level5_2560_1920_PCM_mulaw_1.avi";
static const string TEST_FILE_URI_AVI_3 = TEST_FILE_PATH + "AVI_MPEG4_advanced_simple@level3_352_288_MP3_2.avi";
static const string TEST_FILE_URI_SRT = TEST_FILE_PATH + "gb2312.srt";
static const string TEST_FILE_URI_WEBVTT = TEST_FILE_PATH + "webvtt_test.vtt";
static const string TEST_FILE_URI_OGG = TEST_FILE_PATH + "audio/OGG_8K_1.ogg";
static const string TEST_FILE_URI_WAV = TEST_FILE_PATH + "audio/WAV_24K_32b_1.wav";
static const string TEST_FILE_URI_WAV_1 = TEST_FILE_PATH + "audio/wav_alaw.wav";
static const string TEST_FILE_URI_WAV_2 = TEST_FILE_PATH + "audio/wav_audio_test_202406290859.wav";

static const string TEST_FILE_URI_MP4 = TEST_FILE_PATH + "aac_mpeg4.mp4";
static const string TEST_FILE_URI_MP4_1 = TEST_FILE_PATH + "Muxer_Auxiliary.mp4";
static const string TEST_FILE_URI_MP4_2 = TEST_FILE_PATH + "sub_video_audio.mp4";
static const string TEST_FILE_URI_MP4_3 = TEST_FILE_PATH + "vvc_1280_720_8.mp4";
static const string TEST_FILE_URI_MATROSKA = TEST_FILE_PATH + "mp3_h264.mkv";
static const string TEST_FILE_URI_MATROSKA_1 = TEST_FILE_PATH + "aac_h265.mkv";
static const string TEST_FILE_URI_MATROSKA_2 = TEST_FILE_PATH + "opus_h264.mkv";
static const string TEST_FILE_URI_FMP4 = TEST_FILE_PATH + "h264_mp3_3mevx_fmp4.mp4";
static const string TEST_FILE_URI_FMP4_1 = TEST_FILE_PATH + "h265_aac_1mvex_fmp4.mp4";
static const string TEST_FILE_URI_FMP4_2 = TEST_FILE_PATH + "audiovivid_hdrvivid_1s_fmp4.mp4";


void DemuxerAsynTypeInnerFunc6Test::SetUpTestCase(void) {}

void DemuxerAsynTypeInnerFunc6Test::TearDownTestCase(void) {}

void DemuxerAsynTypeInnerFunc6Test::SetUp(void)
{
}

void DemuxerAsynTypeInnerFunc6Test::TearDown(void)
{
    dataSourceImpl_ = nullptr;
}

bool DemuxerAsynTypeInnerFunc6Test::CreateBufferSize()
{
    uint32_t buffersize = 4096*4096;
    std::shared_ptr<AVAllocator> allocator = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    avBuf_ = OHOS::Media::AVBuffer::CreateAVBuffer(allocator, buffersize);
    if (!avBuf_) {
        return false;
    }
    return true;

}
bool DemuxerAsynTypeInnerFunc6Test::CreateDataSource(const std::string &filePath)
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

bool DemuxerAsynTypeInnerFunc6Test::CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath,
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

void DemuxerAsynTypeInnerFunc6Test::RemoveValue()
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

void DemuxerAsynTypeInnerFunc6Test::GetFrameNum(int32_t i)
{
    if (avBuf_->flag_ == MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) {
        if (i == videoTrackIdx) {
            isVideoEosFlagForSave = true;
        } else if (i == audioTrackIdx) {
            isAudioEosFlagForSave = true;
        }
    } else {
        if (i == videoTrackIdx) {
            videoIndexForRead++;
        } else if (i == audioTrackIdx) {
            audioIndexForRead++;
        }
    }
}

void DemuxerAsynTypeInnerFunc6Test::GetFrameNumAudio(int32_t i)
{
    if (avBuf_->flag_ == MediaAVCodec::AVCODEC_BUFFER_FLAG_EOS) {
        if (i == 0) {
            isAudioEosFlagForSave = true;
        }
    } else {
        if (i == 0) {
            audioIndexForRead++;
        }
    }
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0100
 * @tc.name      : GetNextSampleSize,读取音频轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0100, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t sizenum = 283;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(size, sizenum);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0110
 * @tc.name      : GetNextSampleSize,读取不存在轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0110, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 7;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_UNKNOWN);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0120
 * @tc.name      : GetLastPTSByTrackId,不选轨道
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0120, TestSize.Level2)
{
    indexAud = 0;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0130
 * @tc.name      : GetLastPTSByTrackId,入参的id没有被选择
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0130, TestSize.Level2)
{
    indexAud = 7;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0140
 * @tc.name      : read-pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0140, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9456);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0150
 * @tc.name      : read-pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0150, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 185759;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9449);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0160
 * @tc.name      : pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0160, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9457);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0170
 * @tc.name      : pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0170, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 185759;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9449);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0180
 * @tc.name      : 未选择轨，pause
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0180, TestSize.Level3)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0190
 * @tc.name      : seek + read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0190, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t seekTime = 185759;
    int64_t realtime = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9449);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0200
 * @tc.name      : seek后清理缓存
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0200, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 185759;
    int64_t pts = 0;
    int32_t readCount = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    while (true) {
        if (readCount >= readPos) {
            ASSERT_EQ(demuxerPlugin->SeekTo(
                indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
            ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
            break;
        } else {
            readCount++;
            ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        }
    }
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0210
 * @tc.name      : >pts, SEEK_NEXT_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0210, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219567891;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0220
 * @tc.name      : >pts, SEEK_PREVIOUS_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0220, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219567891;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0230
 * @tc.name      : >pts, SEEK_CLOSEST_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0230, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219567891;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_CLOSEST_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0240
 * @tc.name      : 老ReadSample + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0240, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0250
 * @tc.name      : 老ReadSample + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0250, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0260
 * @tc.name      : 老 GetNextSampleSize + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0260, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0270
 * @tc.name      : 老 GetNextSampleSize + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0270, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0280
 * @tc.name      : 新 ReadSample + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0280, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0290
 * @tc.name      : 新 ReadSample + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0290, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0300
 * @tc.name      : 新 GetNextSampleSize + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0300, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::ERROR_INVALID_OPERATION);
}
  
/**
 * @tc.number    : DEMUXER_ASYN_INNER_AAC_FUNC_0310
 * @tc.name      : 新 GetNextSampleSize + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_AAC_FUNC_0310, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::ERROR_INVALID_OPERATION);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0070
 * @tc.name      : ReadSample全流程
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0070, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9150);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0080
 * @tc.name      : ReadSample,一条轨，timeout = 0
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0080, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    int64_t pts = 0;
    if (demuxerPlugin->GetLastPTSByTrackId(indexAud, pts) == Status::OK) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    } else {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_WAIT_TIMEOUT);
    }
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0090
 * @tc.name      : read , timeout > 1帧读取时间
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0090, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0100
 * @tc.name      : GetNextSampleSize,读取音频轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0100, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(size, READFRAME_192);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0110
 * @tc.name      : GetNextSampleSize,读取不存在轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0110, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 7;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_UNKNOWN);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0120
 * @tc.name      : GetLastPTSByTrackId,不选轨道
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0120, TestSize.Level2)
{
    indexAud = 0;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0130
 * @tc.name      : GetLastPTSByTrackId,入参的id没有被选择
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0130, TestSize.Level2)
{
    indexAud = 7;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0140
 * @tc.name      : read-pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0140, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9149);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0150
 * @tc.name      : read-pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0150, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 384000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9133);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0160
 * @tc.name      : pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0160, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9150);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0170
 * @tc.name      : pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0170, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 384000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9133);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0180
 * @tc.name      : 未选择轨，pause
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0180, TestSize.Level3)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0190
 * @tc.name      : seek + read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0190, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t seekTime = 384000;
    int64_t realtime = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_9133);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0200
 * @tc.name      : seek后清理缓存
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0200, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 384000;
    int64_t pts = 0;
    int32_t readCount = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    while (true) {
        if (readCount >= readPos) {
            ASSERT_EQ(demuxerPlugin->SeekTo(
                indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
            ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
            break;
        } else {
            readCount++;
            ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        }
    }
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0210
 * @tc.name      : >pts, SEEK_NEXT_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0210, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219576000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0220
 * @tc.name      : >pts, SEEK_PREVIOUS_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0220, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219576000;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0230
 * @tc.name      : >pts, SEEK_CLOSEST_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0230, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219576000;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_CLOSEST_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0240
 * @tc.name      : 老ReadSample + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0240, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0250
 * @tc.name      : 老ReadSample + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0250, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0260
 * @tc.name      : 老 GetNextSampleSize + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0260, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0270
 * @tc.name      : 老 GetNextSampleSize + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0270, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0280
 * @tc.name      : 新 ReadSample + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0280, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0290
 * @tc.name      : 新 ReadSample + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0290, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0300
 * @tc.name      : 新 GetNextSampleSize + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0300, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::ERROR_INVALID_OPERATION);
}
  
/**
 * @tc.number    : DEMUXER_ASYN_INNER_MP3_FUNC_0310
 * @tc.name      : 新 GetNextSampleSize + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_MP3_FUNC_0310, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0070
 * @tc.name      : ReadSample全流程
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0070, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_6863);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0080
 * @tc.name      : ReadSample,一条轨，timeout = 0
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0080, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    int64_t pts = 0;
    if (demuxerPlugin->GetLastPTSByTrackId(indexAud, pts) == Status::OK) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    } else {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_WAIT_TIMEOUT);
    }
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0090
 * @tc.name      : read , timeout > 1帧读取时间
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0090, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0100
 * @tc.name      : GetNextSampleSize,读取音频轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0100, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t sizenum = 57;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(size, sizenum);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0110
 * @tc.name      : GetNextSampleSize,读取不存在轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0110, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 7;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_UNKNOWN);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0120
 * @tc.name      : GetLastPTSByTrackId,不选轨道
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0120, TestSize.Level2)
{
    indexAud = 0;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0130
 * @tc.name      : GetLastPTSByTrackId,入参的id没有被选择
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0130, TestSize.Level2)
{
    indexAud = 7;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0140
 * @tc.name      : read-pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0140, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_6862);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0150
 * @tc.name      : read-pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0150, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 448000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_6831);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0160
 * @tc.name      : pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0160, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_6863);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0170
 * @tc.name      : pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0170, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 448000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_6831);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0180
 * @tc.name      : 未选择轨，pause
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0180, TestSize.Level3)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0190
 * @tc.name      : seek + read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0190, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t seekTime = 448000;
    int64_t realtime = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_6831);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0200
 * @tc.name      : seek后清理缓存
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0200, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 448000;
    int64_t pts = 0;
    int32_t readCount = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    while (true) {
        if (readCount >= readPos) {
            ASSERT_EQ(demuxerPlugin->SeekTo(
                indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
            ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
            break;
        } else {
            readCount++;
            ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        }
    }
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0210
 * @tc.name      : >pts, SEEK_NEXT_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0210, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219552000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0220
 * @tc.name      : >pts, SEEK_PREVIOUS_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0220, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219552000;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_PREVIOUS_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0230
 * @tc.name      : >pts, SEEK_CLOSEST_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0230, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219552000;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_CLOSEST_SYNC, realtime), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0240
 * @tc.name      : 老ReadSample + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0240, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0250
 * @tc.name      : 老ReadSample + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0250, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0260
 * @tc.name      : 老 GetNextSampleSize + 新 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0260, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0270
 * @tc.name      : 老 GetNextSampleSize + 新 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0270, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0280
 * @tc.name      : 新 ReadSample + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0280, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0290
 * @tc.name      : 新 ReadSample + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0290, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0300
 * @tc.name      : 新 GetNextSampleSize + 老 ReadSample
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0300, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_), Status::ERROR_INVALID_OPERATION);
}
  
/**
 * @tc.number    : DEMUXER_ASYN_INNER_OGG_FUNC_0310
 * @tc.name      : 新 GetNextSampleSize + 老 GetNextSampleSize
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_OGG_FUNC_0310, TestSize.Level2)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t size = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_OGG, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0070
 * @tc.name      : ReadSample全流程
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0070, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_3050);
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0080
 * @tc.name      : ReadSample,一条轨，timeout = 0
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0080, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    int64_t pts = 0;
    if (demuxerPlugin->GetLastPTSByTrackId(indexAud, pts) == Status::OK) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    } else {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::ERROR_WAIT_TIMEOUT);
    }
}
/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0090
 * @tc.name      : read , timeout > 1帧读取时间
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0090, TestSize.Level1)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0100
 * @tc.name      : GetNextSampleSize,读取音频轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0100, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 0;
    uint32_t timeout = 10000;
    int32_t sizenum = 1570;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::OK);
    ASSERT_EQ(size, sizenum);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0110
 * @tc.name      : GetNextSampleSize,读取不存在轨
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0110, TestSize.Level1)
{
    int32_t size = 0;
    indexAud = 7;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(CreateBufferSize(), true);
    ASSERT_EQ(demuxerPlugin->GetNextSampleSize(indexAud, size, timeout), Status::ERROR_UNKNOWN);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0120
 * @tc.name      : GetLastPTSByTrackId,不选轨道
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0120, TestSize.Level2)
{
    indexAud = 0;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0130
 * @tc.name      : GetLastPTSByTrackId,入参的id没有被选择
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0130, TestSize.Level2)
{
    indexAud = 7;
    int64_t pts = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0140
 * @tc.name      : read-pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0140, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_3049);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0150
 * @tc.name      : read-pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0150, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 1008000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_3035);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0160
 * @tc.name      : pause-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0160, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_3050);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0170
 * @tc.name      : pause-seek-read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0170, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 1008000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_3035);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0180
 * @tc.name      : 未选择轨，pause
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0180, TestSize.Level3)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->Pause(), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0190
 * @tc.name      : seek + read
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0190, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t seekTime = 1008000;
    int64_t realtime = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
    while (!isAudioEosFlagForSave) {
        ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        GetFrameNumAudio(0);
    }
    ASSERT_EQ(audioIndexForRead, READFRAME_3035);
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0200
 * @tc.name      : seek后清理缓存
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0200, TestSize.Level3)
{
    indexAud = 0;
    uint32_t timeout = 10000;
    int64_t realtime = 0;
    int64_t seekTime = 1008000;
    int64_t pts = 0;
    int32_t readCount = 0;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    ASSERT_EQ(CreateBufferSize(), true);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    while (true) {
        if (readCount >= readPos) {
            ASSERT_EQ(demuxerPlugin->SeekTo(
                indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
            ASSERT_EQ(demuxerPlugin->GetLastPTSByTrackId(indexAud, pts), Status::ERROR_NOT_EXISTED);
            break;
        } else {
            readCount++;
            ASSERT_EQ(demuxerPlugin->ReadSample(indexAud, avBuf_, timeout), Status::OK);
        }
    }
}

/**
 * @tc.number    : DEMUXER_ASYN_INNER_FLAC_FUNC_0210
 * @tc.name      : >pts, SEEK_NEXT_SYNC+GetLastPTSByTrackId
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerAsynTypeInnerFunc6Test, DEMUXER_ASYN_INNER_FLAC_FUNC_0210, TestSize.Level2)
{
    indexAud = 0;
    int64_t realtime = 0;
    int64_t seekTime = 219528000;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SeekTo(
        indexAud, seekTime / THOUSAND, Plugins::SeekMode::SEEK_NEXT_SYNC, realtime), Status::OK);
}
}  // namespace Media
}  // namespace OHOS