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
#include <gtest/gtest.h>
#include "media_description.h"
#include "file_server_demo.h"
#include <numeric>
#include <vector>

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace std;
using MediaAVBuffer = OHOS::Media::AVBuffer;
using FFmpegAVBuffer = ::AVBuffer;
using InvokerTypeAlias = OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin::InvokerType;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media::Plugins;

list<SeekMode> seekModes = {SeekMode::SEEK_NEXT_SYNC, SeekMode::SEEK_PREVIOUS_SYNC,
    SeekMode::SEEK_CLOSEST_SYNC};
const int32_t DEFAULT_TIMEOUT = 100; // 100ms
unique_ptr<FileServerDemo> server = nullptr;
static const string TEST_URI_PATH = "http://127.0.0.1:46666/";
static const string TEST_RELATIVE_PATH = "/data/test/media/";
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
// FLV
string g_flvPath = string("/data/test/media/h264.flv");
// MP4
string g_mp4Path1 = string("/data/test/media/h264_double_video_audio.mp4");
string g_mp4Path2 = string("/data/test/media/avcc_aac_mp3.mp4");
string g_mp4Path3 = string("/data/test/media/MPEG4.mp4");
string g_mp4Path4 = string("/data/test/media/muxer_auxl_265.mp4");
string g_mp4Path5 = string("/data/test/media/muxer_auxl_265_264_aac.mp4");
string g_mp43dgsPath = string("/data/test/media/3dgs.mp4");
// FMP4
string g_fmp4Path1 = string("/data/test/media/h264_fmp4.mp4");
string g_fmp4Path2 = string("/data/test/media/h265_fmp4.mp4");
// MOV
string g_movPath1 = string("/data/test/media/265_pcm_s16le.mov");
string g_movPath2 = string("/data/test/media/h264_aac.mov");
string g_movPath3 = string("/data/test/media/h264_mp3.mov");
string g_movPath4 = string("/data/test/media/h264_vorbis.mov");
string g_movPath5 = string("/data/test/media/MPEG4_mp2.mov");
// MKV
string g_mkvPath1 = string("/data/test/media/h264_mp3_4sec.mkv");
string g_mkvPath2 = string("/data/test/media/h265_aac_4sec.mkv");
string g_mkvPath3 = string("/data/test/media/h265_opus_4sec.mkv");
// MPEG-TS
string g_mpegTsPath1 = string("/data/test/media/2obj_44100Hz_16bit_32k.ts");
string g_mpegTsPath2 = string("/data/test/media/hevc_aac_1920x1080_g30_30fps.ts");
string g_mpegTsPath3 = string("/data/test/media/h264_ac3.mts");
string g_mpegTsPath4 = string("/data/test/media/test_mpeg2_Gop25_4sec.ts");
string g_mpegTsPath5 = string("/data/test/media/test_mpeg4_Gop25_4sec.ts");
string g_mpegTsPath6 = string("/data/test/media/hevc_aac_3840x2160_30frames.ts");
// AVI
string g_aviPath1 = string("/data/test/media/h264_aac.avi");
string g_aviPath2 = string("/data/test/media/h264_mp3.avi");
string g_aviPath3 = string("/data/test/media/mpeg4_pcm.avi");
string g_aviPath4 = string("/data/test/media/test_263_aac_B_Gop25_4sec_cover.avi");
string g_aviPath5 = string("/data/test/media/test_mpeg4_mp3_B_Gop25_4sec_cover.avi");
string g_aviPath6 = string("/data/test/media/test_mpeg2_mp2_B_Gop25_4sec_cover.avi");
// MPG
string g_mpgPath1 = string("/data/test/media/mpeg_mpeg2_mp2.mpeg");
string g_mpgPath2 = string("/data/test/media/mpeg_mpeg2_mp3.mpeg");
string g_mpgPath3 = string("/data/test/media/mpeg_h264_mp2.mpeg");
// SRT
string g_srtPath1 = string("/data/test/media/subtitle.srt");
// VTT
string g_vttPath1 = string("/data/test/media/webvtt_test_ut.vtt");

void DemuxerPluginUnitTest::SetUpTestCase(void)
{
    if (server != nullptr) {
        server->StopServer();
        server.reset();
    }
    server = make_unique<FileServerDemo>();
    server->StartServer();
    cout << "start" << endl;
}

void DemuxerPluginUnitTest::TearDownTestCase(void)
{
    if (server == nullptr) {
        return;
    }
    server->StopServer();
}

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

void DemuxerPluginUnitTest::InitResourceURI(const std::string &filePath, std::string pluginName)
{
    std::string uri = TEST_URI_PATH + filePath.substr(TEST_RELATIVE_PATH.size());
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

void DemuxerPluginUnitTest::InitWeakNetworkDemuxerPluginURI(
    const std::string& filePath, std::string pluginName, int64_t failOffset, size_t maxFailCount)
{
    std::string uri = TEST_URI_PATH + filePath.substr(TEST_RELATIVE_PATH.size());
    printf("DemuxerPluginUnitTest::InitWeakNetworkDemuxerPluginURI, uri: %s\n", uri.c_str());
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
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    initStatus_ = true;
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
    ASSERT_EQ(demuxerPlugin_->GetMediaInfo(mediaInfo_), Status::OK);
    initStatus_ = true;
}

void TestAVReadPacketStopState(const std::string& pluginName, const std::string& filePath,
                               int sleepMs, bool expectSuccess)
{
    struct stat fileStatus {};
    ASSERT_EQ(stat(filePath.c_str(), &fileStatus), 0);
    int64_t fileSize = static_cast<int64_t>(fileStatus.st_size);
    int fd = open(filePath.c_str(), O_RDONLY);
    ASSERT_GE(fd, 0);
    auto uri = "fd://" + std::to_string(fd) + "?offset=0&size=" + std::to_string(fileSize);

    auto demuxerPluginManager = std::make_shared<DemuxerPluginManager>();
    SourceCallback cb(demuxerPluginManager);
    auto source = std::make_shared<Source>();
    source->SetCallback(&cb);
    EXPECT_EQ(source->SetSource(std::make_shared<MediaSource>(uri)), Status::OK);
    std::vector<StreamInfo> streams;
    source->GetStreamInfo(streams);
    demuxerPluginManager->InitDefaultPlay(streams);

    auto streamDemuxer = std::make_shared<StreamDemuxerPullDataFailMock>(0, 20);
    streamDemuxer->SetInterruptState(false);
    streamDemuxer->SetSource(source);
    streamDemuxer->Init("");
    streamDemuxer->SetDemuxerState(0, DemuxerState::DEMUXER_STATE_PARSE_FRAME);

    auto dataSource = std::make_shared<DataSourceImpl>(streamDemuxer, 0);
    auto basePlugin = Plugins::PluginManagerV2::Instance().CreatePluginByName(pluginName);
    auto demuxerPlugin = std::reinterpret_pointer_cast<OHOS::Media::Plugins::Ffmpeg::FFmpegDemuxerPlugin>(basePlugin);

    Status setDataSourceStatus = Status::ERROR_UNKNOWN;
    demuxerPlugin->SetAVReadPacketStopState(false);
    std::thread t([&]() {
        setDataSourceStatus = demuxerPlugin->SetDataSource(dataSource);
    });
    std::this_thread::sleep_for(std::chrono::milliseconds(sleepMs));
    demuxerPlugin->SetAVReadPacketStopState(true);
    t.join();
    if (expectSuccess) {
        ASSERT_EQ(setDataSourceStatus, Status::OK);
    } else {
        ASSERT_NE(setDataSourceStatus, Status::OK);
    }
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

void DemuxerPluginUnitTest::CheckAllFrames(const std::vector<int>& expectedFrames,
    const std::vector<int>& expectedKeyFrames, const std::vector<uint32_t>& keyFrameIndex)
{
    for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
        std::string mime;
        mediaInfo_.tracks[i].GetData(Tag::MIME_TYPE, mime);
        if (mime.find("video/") == 0 || mime.find("audio/") == 0 ||
            mime.find("application/") == 0 || mime.find("text/vtt") == 0) {
            demuxerPlugin_->SelectTrack(i);
            selectedTrackIds_.push_back(i);
            frames_[i] = 0;
            keyFrames_[i] = 0;
            eosFlag_[i] = false;
        }
    }
    SetInitValue();
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3); // 3 is use for buffer allocate
    while (!isEOS(eosFlag_)) {
        for (auto id : selectedTrackIds_) {
            auto ret = demuxerPlugin_->ReadSample(id, buffer.mediaAVBuffer, DEFAULT_TIMEOUT);
            while (ret == Status::ERROR_WAIT_TIMEOUT) {
                ret = demuxerPlugin_->ReadSample(id, buffer.mediaAVBuffer, DEFAULT_TIMEOUT);
            }
            ASSERT_TRUE(ret == Status::OK || ret == Status::END_OF_STREAM || ret == Status::NO_ERROR);
            flag_ = buffer.mediaAVBuffer->flag_;
            if (id == 0) {
                EXPECT_TRUE(CheckKeyFrameIndex(
                    keyFrameIndex, frames_[0], flag_ & AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_SYNC_FRAME));
            }
            CountFrames(id);
        }
    }
    for (size_t i = 0; i < expectedFrames.size(); ++i) {
        EXPECT_EQ(frames_[i], expectedFrames[i]);
        EXPECT_EQ(keyFrames_[i], expectedKeyFrames[i]);
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
 * @tc.name: Demuxer_BoostReadThreadPriority_0001
 * @tc.desc: Test BoostReadThreadPriority
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_BoostReadThreadPriority_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer(DEFAULT_BUFFSIZE);
    ASSERT_EQ(demuxerPlugin_->BoostReadThreadPriority(), Status::OK);
    ASSERT_EQ(demuxerPlugin_->BoostReadThreadPriority(), Status::ERROR_WRONG_STATE);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_EQ(demuxerPlugin_->BoostReadThreadPriority(), Status::ERROR_WRONG_STATE);
}

/**
 * @tc.name: Demuxer_BoostReadThreadPriority_0002
 * @tc.desc: Test BoostReadThreadPriority
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_BoostReadThreadPriority_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_flv";
    InitResource(g_flvPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_NE(demuxerPlugin_, nullptr); // 检查插件是否初始化成功
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(1), Status::OK);
    OHOS::Media::AVBufferWrapper buffer(DEFAULT_BUFFSIZE);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_EQ(demuxerPlugin_->ReadSample(0, buffer.mediaAVBuffer, 100), Status::OK);
    ASSERT_EQ(demuxerPlugin_->BoostReadThreadPriority(), Status::ERROR_WRONG_STATE);
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
    OHOS::Media::AVBufferWrapper buffer1(DEFAULT_BUFFSIZE);
    ASSERT_NE(demuxerPlugin_->ReadSample(0, buffer1.mediaAVBuffer, 100), Status::OK);
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
    std::string filePath = g_flvPath;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({76, 113}, {1, 113}, {0});
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
    int64_t seekTime = 0;
    list<int64_t> toPtsList = {0, 1500, 1000, 1740, 1970, 2100}; // ms
    vector<int32_t> videoVals = {76, 76, 76, 0, 76, 0, 0, 76, 76, 0, 76, 0, 0, 76, 0, 0, 76, 0};
    vector<int32_t> audioVals = {107, 107, 107, 0, 107, 0, 0, 107, 107, 0, 107, 0, 0, 107, 0, 0, 107, 0};
    OHOS::Media::AVBufferWrapper buffer(videoHeight_ * videoWidth_ * 3);
    for (auto toPts = toPtsList.begin(); toPts != toPtsList.end(); toPts++) {
        for (auto mode = seekModes.begin(); mode != seekModes.end(); mode++) {
            Status ret_ = demuxerPlugin_->SeekTo(-1, *toPts, *mode, seekTime);
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
    ASSERT_NE(demuxerPlugin_->SeekTo(0, 11000, SeekMode::SEEK_NEXT_SYNC, seekTime), Status::OK);
    ASSERT_NE(demuxerPlugin_->SeekTo(0, -1000, SeekMode::SEEK_NEXT_SYNC, seekTime), Status::OK);
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
    int64_t seekTime = 0;
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    ASSERT_NE(demuxerPlugin_->SeekTo(0, INT64_MAX, SeekMode::SEEK_NEXT_SYNC, seekTime), Status::OK);
    ASSERT_NE(demuxerPlugin_->SeekTo(0, -1000, SeekMode::SEEK_NEXT_SYNC, seekTime), Status::OK);
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
    CheckAllFrames({76, 113}, {1, 113}, {0});
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
    CheckAllFrames({76, 113}, {1, 113}, {0});
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
    CheckAllFrames({76, 113}, {1, 113}, {0});
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
    CheckAllFrames({76, 113}, {1, 113}, {0});
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
    CheckAllFrames({76, 113}, {1, 113}, {0});
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

/**
 * @tc.name: Demuxer_ReadSample_MP4_0001
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MP4_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_MP4_0002
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MP4_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433, 417}, {3, 433, 417}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_MP4_0003
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MP4_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path3;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 434}, {51, 434}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144,
                                           156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276,
                                           288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408,
                                           420, 432, 444, 456, 468, 480, 492, 504, 516, 528, 540,
                                           552, 564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_MP4_0004
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MP4_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path4;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({16, 16, 16}, {1, 1, 1}, {0});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_MP4_0005
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MP4_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path5;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        std::vector<uint32_t> numbers(430);
        std::iota(numbers.begin(), numbers.end(), 0);
        CheckAllFrames({430, 601, 430, 601, 601}, {430, 3, 430, 3, 3}, numbers);
    }
}

/**
 * @tc.name: Demuxer_ReadSample_FMP4_0001
 * @tc.desc: Copy current sample to buffer (fmp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_FMP4_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_fmp4Path1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433}, {3, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_FMP4_0002
 * @tc.desc: Copy current sample to buffer (fmp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_FMP4_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_fmp4Path2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Demuxer_ReadSample_MOV_0001
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MOV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Demuxer_ReadSample_MOV_0002
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MOV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 434}, {3, 434}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_MOV_0003
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MOV_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath3;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 386}, {3, 386}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_MOV_0004
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MOV_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath4;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 609}, {3, 609}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_MOV_0005
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MOV_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath5;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 385}, {51, 385}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120,
                                           132, 144, 156, 168, 180, 192, 204, 216, 228,
                                           240, 252, 264, 276, 288, 300, 312, 324, 336,
                                           348, 360, 372, 384, 396, 408, 420, 432, 444,
                                           456, 468, 480, 492, 504, 516, 528, 540, 552,
                                           564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_MKV_0001
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MKV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({239, 153}, {4, 153}, {0, 60, 120, 180});
}

/**
 * @tc.name: Demuxer_ReadSample_MKV_0002
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MKV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({242, 173}, {1, 173}, {0});
}

/**
 * @tc.name: Demuxer_ReadSample_MKV_0003
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MKV_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath3;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({242, 200}, {1, 200}, {0});
}

/**
 * @tc.name: Demuxer_ReadSample_MPEG_TS_0001
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPEG_TS_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({92}, {92}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                                44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
                                72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                                86, 87, 88, 89, 90, 91});
}

/**
 * @tc.name: Demuxer_ReadSample_MPEG_TS_0002
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPEG_TS_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({303, 433}, {11, 433}, {0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_MPEG_TS_0003
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPEG_TS_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath3;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 314}, {3, 314}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_MPEG_TS_0004
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPEG_TS_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath4;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 174}, {5, 174}, {0, 25, 50, 75, 100});
}

/**
 * @tc.name: Demuxer_ReadSample_MPEG_TS_0005
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPEG_TS_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath5;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 155}, {9, 155}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_MPEG_TS_0006
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPEG_TS_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath6;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({30}, {1}, {0});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_AVI_0001
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AVI_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433}, {3, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_AVI_0002
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AVI_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 386}, {3, 386}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_AVI_0003
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AVI_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath3;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {51, 433}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
                                           120, 132, 144, 156, 168, 180, 192, 204, 216,
                                           228, 240, 252, 264, 276, 288, 300, 312, 324,
                                           336, 348, 360, 372, 384, 396, 408, 420, 432,
                                           444, 456, 468, 480, 492, 504, 516, 528, 540,
                                           552, 564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_AVI_0004
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AVI_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath4;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 174}, {103, 174}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                            30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                                            44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                            58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
                                            72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                                            86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
                                            100, 101, 102});
}

/**
 * @tc.name: Demuxer_ReadSample_AVI_0005
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AVI_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath5;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 156}, {9, 156}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_AVI_0006
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AVI_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath6;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 155}, {9, 155}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_MPG_0001
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPG_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2164}, {109, 2164}, {0, 12, 14, 26, 38, 50, 62, 74, 86,
                                               98, 110, 122, 134, 146, 158, 170, 182, 194,
                                               206, 218, 230, 242, 254, 266, 278, 290, 302,
                                               314, 326, 338, 350, 362, 374, 386, 396, 408,
                                               420, 432, 444, 456, 468, 480, 492, 504, 516,
                                               528, 540, 552, 564, 571, 583, 595, 607, 619,
                                               621, 633, 645, 657, 658, 670, 672, 684, 696,
                                               708, 720, 732, 744, 756, 768, 780, 792, 804,
                                               816, 828, 840, 852, 864, 876, 888, 900, 912,
                                               924, 936, 948, 960, 972, 984, 996, 1008, 1020,
                                               1032, 1044, 1056, 1068, 1080, 1092, 1104, 1116,
                                               1128, 1140, 1152, 1164, 1176, 1188, 1200, 1212,
                                               1224, 1236, 1248});
}

/**
 * @tc.name: Demuxer_ReadSample_MPG_0002
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPG_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2165}, {120, 2165}, {0, 12, 14, 17, 29, 41, 53, 65, 77, 89, 101, 113,
                                               125, 137, 149, 161, 173, 185, 197, 209,
                                               217, 218, 219, 220, 221, 222, 234, 246, 258, 270, 282,
                                               294, 306, 318, 330, 334, 346, 358,
                                               370, 382, 394, 396, 408, 420, 432, 435, 447, 459, 471,
                                               483, 495, 507, 519, 531, 543, 555,
                                               567, 571, 583, 595, 607, 619, 631, 643, 655, 667, 672,
                                               679, 680, 681, 693, 705, 717, 729,
                                               741, 753, 765, 772, 773, 774, 775, 787, 799, 811,
                                               823, 835, 847, 859, 871, 883, 895, 907,
                                               919, 931, 943, 955, 967, 979, 991, 1003, 1015,
                                               1027, 1039, 1051, 1063, 1075, 1087, 1099,
                                               1111, 1123, 1135, 1147, 1159, 1171, 1183, 1195,
                                               1207, 1219, 1231, 1243});
}

/**
 * @tc.name: Demuxer_ReadSample_MPG_0003
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MPG_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath3;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2164}, {19, 2164}, {0, 89, 123, 193, 224, 288, 334, 396, 436, 531,
                                              571, 621, 658, 684, 727, 772, 867, 1117, 1163});
}

/**
 * @tc.name: Demuxer_ReadSample_SRT_0001
 * @tc.desc: Copy current sample to buffer (SRT)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_SRT_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_srt";
    std::string filePath = g_srtPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({5}, {5}, {0, 1, 2, 3, 4});
}

/**
 * @tc.name: Demuxer_ReadSample_VTT_0001
 * @tc.desc: Copy current sample to buffer (VTT)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_VTT_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_webvtt";
    std::string filePath = g_vttPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({4}, {4}, {0, 1, 2, 3});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MP4_0001
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MP4_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MP4_0002
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MP4_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433, 417}, {3, 433, 417}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MP4_0003
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MP4_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path3;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 434}, {51, 434}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144,
                                           156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276,
                                           288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408,
                                           420, 432, 444, 456, 468, 480, 492, 504, 516, 528, 540,
                                           552, 564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MP4_0004
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MP4_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path4;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 4330600, 3);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({16, 16, 16}, {1, 1, 1}, {0});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MP4_0005
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MP4_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path5;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        std::vector<uint32_t> numbers(430);
        std::iota(numbers.begin(), numbers.end(), 0);
        CheckAllFrames({430, 601, 430, 601, 601}, {430, 3, 430, 3, 3}, numbers);
    }
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_FMP4_0001
 * @tc.desc: Copy current sample to buffer (fmp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_FMP4_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_fmp4Path1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433}, {3, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_FMP4_0002
 * @tc.desc: Copy current sample to buffer (fmp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_FMP4_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_fmp4Path2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MOV_0001
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MOV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MOV_0002
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MOV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 434}, {3, 434}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MOV_0003
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MOV_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath3;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 386}, {3, 386}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MOV_0004
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MOV_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath4;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 609}, {3, 609}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MOV_0005
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MOV_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath5;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 385}, {51, 385}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120,
                                           132, 144, 156, 168, 180, 192, 204, 216, 228,
                                           240, 252, 264, 276, 288, 300, 312, 324, 336,
                                           348, 360, 372, 384, 396, 408, 420, 432, 444,
                                           456, 468, 480, 492, 504, 516, 528, 540, 552,
                                           564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MKV_0001
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MKV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({239, 153}, {4, 153}, {0, 60, 120, 180});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MKV_0002
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MKV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({242, 173}, {1, 173}, {0});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MKV_0003
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MKV_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath3;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({242, 200}, {1, 200}, {0});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPEG_TS_0001
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPEG_TS_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({92}, {92}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                                44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
                                72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                                86, 87, 88, 89, 90, 91});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPEG_TS_0002
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPEG_TS_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({303, 433}, {11, 433}, {0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPEG_TS_0003
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPEG_TS_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath3;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 314}, {3, 314}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPEG_TS_0004
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPEG_TS_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath4;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 174}, {5, 174}, {0, 25, 50, 75, 100});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPEG_TS_0005
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPEG_TS_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath5;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 155}, {9, 155}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPEG_TS_0006
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPEG_TS_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath6;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({30}, {1}, {0});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AVI_0001
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AVI_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433}, {3, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AVI_0002
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AVI_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 386}, {3, 386}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AVI_0003
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AVI_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath3;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {51, 433}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
                                           120, 132, 144, 156, 168, 180, 192, 204, 216,
                                           228, 240, 252, 264, 276, 288, 300, 312, 324,
                                           336, 348, 360, 372, 384, 396, 408, 420, 432,
                                           444, 456, 468, 480, 492, 504, 516, 528, 540,
                                           552, 564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AVI_0004
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AVI_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath4;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 174}, {103, 174}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                            30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                                            44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                            58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
                                            72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                                            86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
                                            100, 101, 102});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AVI_0005
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AVI_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath5;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 156}, {9, 156}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AVI_0006
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AVI_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath6;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 155}, {9, 155}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPG_0001
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPG_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2164}, {109, 2164}, {0, 12, 14, 26, 38, 50, 62, 74, 86,
                                               98, 110, 122, 134, 146, 158, 170, 182, 194,
                                               206, 218, 230, 242, 254, 266, 278, 290, 302,
                                               314, 326, 338, 350, 362, 374, 386, 396, 408,
                                               420, 432, 444, 456, 468, 480, 492, 504, 516,
                                               528, 540, 552, 564, 571, 583, 595, 607, 619,
                                               621, 633, 645, 657, 658, 670, 672, 684, 696,
                                               708, 720, 732, 744, 756, 768, 780, 792, 804,
                                               816, 828, 840, 852, 864, 876, 888, 900, 912,
                                               924, 936, 948, 960, 972, 984, 996, 1008, 1020,
                                               1032, 1044, 1056, 1068, 1080, 1092, 1104, 1116,
                                               1128, 1140, 1152, 1164, 1176, 1188, 1200, 1212,
                                               1224, 1236, 1248});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPG_0002
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPG_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2165}, {120, 2165}, {0, 12, 14, 17, 29, 41, 53, 65, 77, 89, 101, 113,
                                               125, 137, 149, 161, 173, 185, 197, 209,
                                               217, 218, 219, 220, 221, 222, 234, 246, 258, 270, 282,
                                               294, 306, 318, 330, 334, 346, 358,
                                               370, 382, 394, 396, 408, 420, 432, 435, 447, 459, 471,
                                               483, 495, 507, 519, 531, 543, 555,
                                               567, 571, 583, 595, 607, 619, 631, 643, 655, 667, 672,
                                               679, 680, 681, 693, 705, 717, 729,
                                               741, 753, 765, 772, 773, 774, 775, 787, 799, 811,
                                               823, 835, 847, 859, 871, 883, 895, 907,
                                               919, 931, 943, 955, 967, 979, 991, 1003, 1015,
                                               1027, 1039, 1051, 1063, 1075, 1087, 1099,
                                               1111, 1123, 1135, 1147, 1159, 1171, 1183, 1195,
                                               1207, 1219, 1231, 1243});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MPG_0003
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MPG_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath3;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2164}, {19, 2164}, {0, 89, 123, 193, 224, 288, 334, 396, 436, 531,
                                              571, 621, 658, 684, 727, 772, 867, 1117, 1163});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_SRT_0001
 * @tc.desc: Copy current sample to buffer (SRT)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_SRT_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_srt";
    std::string filePath = g_srtPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({5}, {5}, {0, 1, 2, 3, 4});
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_VTT_0001
 * @tc.desc: Copy current sample to buffer (VTT)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_VTT_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_webvtt";
    std::string filePath = g_vttPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({4}, {4}, {0, 1, 2, 3});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MP4_0001
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MP4_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 604, 433, 433}, {3, 3, 433, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MP4_0002
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MP4_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433, 417}, {3, 433, 417}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MP4_0003
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MP4_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path3;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 434}, {51, 434}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120, 132, 144,
                                           156, 168, 180, 192, 204, 216, 228, 240, 252, 264, 276,
                                           288, 300, 312, 324, 336, 348, 360, 372, 384, 396, 408,
                                           420, 432, 444, 456, 468, 480, 492, 504, 516, 528, 540,
                                           552, 564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MP4_0004
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MP4_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path4;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({16, 16, 16}, {1, 1, 1}, {0});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MP4_0005
 * @tc.desc: Copy current sample to buffer (mp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MP4_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp4Path5;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        std::vector<uint32_t> numbers(430);
        std::iota(numbers.begin(), numbers.end(), 0);
        CheckAllFrames({430, 601, 430, 601, 601}, {430, 3, 430, 3, 3}, numbers);
    }
}

/**
 * @tc.name: Demuxer_ReadSample_URI_FMP4_0001
 * @tc.desc: Copy current sample to buffer (fmp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_FMP4_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_fmp4Path1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433}, {3, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_FMP4_0002
 * @tc.desc: Copy current sample to buffer (fmp4)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_FMP4_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_fmp4Path2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MOV_0001
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MOV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {3, 433}, {0, 246, 497});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MOV_0002
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MOV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 434}, {3, 434}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MOV_0003
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MOV_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath3;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 386}, {3, 386}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MOV_0004
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MOV_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath4;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 609}, {3, 609}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MOV_0005
 * @tc.desc: Copy current sample to buffer (mov)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MOV_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_movPath5;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 385}, {51, 385}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108, 120,
                                           132, 144, 156, 168, 180, 192, 204, 216, 228,
                                           240, 252, 264, 276, 288, 300, 312, 324, 336,
                                           348, 360, 372, 384, 396, 408, 420, 432, 444,
                                           456, 468, 480, 492, 504, 516, 528, 540, 552,
                                           564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MKV_0001
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MKV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({239, 153}, {4, 153}, {0, 60, 120, 180});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MKV_0002
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MKV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({242, 173}, {1, 173}, {0});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MKV_0003
 * @tc.desc: Copy current sample to buffer (MKV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MKV_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_matroska,webm";
    std::string filePath = g_mkvPath3;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({242, 200}, {1, 200}, {0});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPEG_TS_0001
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPEG_TS_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({92}, {92}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                                44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
                                72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                                86, 87, 88, 89, 90, 91});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPEG_TS_0002
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPEG_TS_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({303, 433}, {11, 433}, {0, 30, 60, 90, 120, 150, 180, 210, 240, 270, 300});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPEG_TS_0003
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPEG_TS_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath3;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 314}, {3, 314}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPEG_TS_0004
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPEG_TS_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath4;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 174}, {5, 174}, {0, 25, 50, 75, 100});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPEG_TS_0005
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPEG_TS_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath5;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 155}, {9, 155}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPEG_TS_0006
 * @tc.desc: Copy current sample to buffer (MPEG-TS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPEG_TS_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpegts";
    std::string filePath = g_mpegTsPath6;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    if (access(HEVC_LIB_PATH.c_str(), F_OK) == 0) {
        CheckAllFrames({30}, {1}, {0});
    }
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AVI_0001
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AVI_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 433}, {3, 433}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AVI_0002
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AVI_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({602, 386}, {3, 386}, {0, 250, 500});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AVI_0003
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AVI_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath3;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({604, 433}, {51, 433}, {0, 12, 24, 36, 48, 60, 72, 84, 96, 108,
                                           120, 132, 144, 156, 168, 180, 192, 204, 216,
                                           228, 240, 252, 264, 276, 288, 300, 312, 324,
                                           336, 348, 360, 372, 384, 396, 408, 420, 432,
                                           444, 456, 468, 480, 492, 504, 516, 528, 540,
                                           552, 564, 576, 588, 600});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AVI_0004
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AVI_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath4;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 174}, {103, 174}, {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
                                            16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
                                            30, 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43,
                                            44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56, 57,
                                            58, 59, 60, 61, 62, 63, 64, 65, 66, 67, 68, 69, 70, 71,
                                            72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85,
                                            86, 87, 88, 89, 90, 91, 92, 93, 94, 95, 96, 97, 98, 99,
                                            100, 101, 102});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AVI_0005
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AVI_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath5;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 156}, {9, 156}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AVI_0006
 * @tc.desc: Copy current sample to buffer (AVI)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AVI_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_avi";
    std::string filePath = g_aviPath6;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({103, 155}, {9, 155}, {0, 12, 24, 36, 48, 60, 72, 84, 96});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPG_0001
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPG_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2164}, {109, 2164}, {0, 12, 14, 26, 38, 50, 62, 74, 86,
                                               98, 110, 122, 134, 146, 158, 170, 182, 194,
                                               206, 218, 230, 242, 254, 266, 278, 290, 302,
                                               314, 326, 338, 350, 362, 374, 386, 396, 408,
                                               420, 432, 444, 456, 468, 480, 492, 504, 516,
                                               528, 540, 552, 564, 571, 583, 595, 607, 619,
                                               621, 633, 645, 657, 658, 670, 672, 684, 696,
                                               708, 720, 732, 744, 756, 768, 780, 792, 804,
                                               816, 828, 840, 852, 864, 876, 888, 900, 912,
                                               924, 936, 948, 960, 972, 984, 996, 1008, 1020,
                                               1032, 1044, 1056, 1068, 1080, 1092, 1104, 1116,
                                               1128, 1140, 1152, 1164, 1176, 1188, 1200, 1212,
                                               1224, 1236, 1248});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPG_0002
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPG_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2165}, {120, 2165}, {0, 12, 14, 17, 29, 41, 53, 65, 77, 89, 101, 113,
                                               125, 137, 149, 161, 173, 185, 197, 209,
                                               217, 218, 219, 220, 221, 222, 234, 246, 258, 270, 282,
                                               294, 306, 318, 330, 334, 346, 358,
                                               370, 382, 394, 396, 408, 420, 432, 435, 447, 459, 471,
                                               483, 495, 507, 519, 531, 543, 555,
                                               567, 571, 583, 595, 607, 619, 631, 643, 655, 667, 672,
                                               679, 680, 681, 693, 705, 717, 729,
                                               741, 753, 765, 772, 773, 774, 775, 787, 799, 811,
                                               823, 835, 847, 859, 871, 883, 895, 907,
                                               919, 931, 943, 955, 967, 979, 991, 1003, 1015,
                                               1027, 1039, 1051, 1063, 1075, 1087, 1099,
                                               1111, 1123, 1135, 1147, 1159, 1171, 1183, 1195,
                                               1207, 1219, 1231, 1243});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MPG_0003
 * @tc.desc: Copy current sample to buffer (MPG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MPG_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_mpeg";
    std::string filePath = g_mpgPath3;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({1253, 2164}, {19, 2164}, {0, 89, 123, 193, 224, 288, 334, 396, 436, 531,
                                              571, 621, 658, 684, 727, 772, 867, 1117, 1163});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_SRT_0001
 * @tc.desc: Copy current sample to buffer (SRT)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_SRT_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_srt";
    std::string filePath = g_srtPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({5}, {5}, {0, 1, 2, 3, 4});
}

/**
 * @tc.name: Demuxer_ReadSample_URI_VTT_0001
 * @tc.desc: Copy current sample to buffer (VTT)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_VTT_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_webvtt";
    std::string filePath = g_vttPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    CheckAllFrames({4}, {4}, {0, 1, 2, 3});
}

/**
 * @tc.name: Demuxer_GetMediaInfo_3DGS_0001
 * @tc.desc: Get media info (3DGS)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetMediaInfo_3DGS_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_mp43dgsPath;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    MediaInfo mediaInfo;
    Status ret = demuxerPlugin_->GetMediaInfo(mediaInfo);
    EXPECT_EQ(ret, Status::OK);
    auto meta = mediaInfo.general;
    bool isGltf = 0;
    int64_t gltfOffset = 0;
    meta.Get<Tag::IS_GLTF>(isGltf);
    meta.Get<Tag::GLTF_OFFSET>(gltfOffset);
    EXPECT_EQ(isGltf, 1);
    EXPECT_EQ(gltfOffset, 3526448);
}

/**
 * @tc.name: Demuxer_SetAVReadPacketStopState_TimeoutFail_0001
 * @tc.desc: 500ms timeout, expect fail
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SetAVReadPacketStopState_TimeoutFail_0001, TestSize.Level1)
{
    TestAVReadPacketStopState("avdemux_mov,mp4,m4a,3gp,3g2,mj2", g_mp4Path1, 500, false);
}

/**
 * @tc.name: Demuxer_SetAVReadPacketStopState_TimeoutSuccess_0002
 * @tc.desc: 1000ms timeout, expect success
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SetAVReadPacketStopState_TimeoutSuccess_0002, TestSize.Level1)
{
    TestAVReadPacketStopState("avdemux_mov,mp4,m4a,3gp,3g2,mj2", g_mp4Path1, 1000, true);
}