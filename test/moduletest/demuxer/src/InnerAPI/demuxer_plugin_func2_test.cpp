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
#include "file_server_demo.h"
#include <thread>
#include <iostream>

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace Media {

class DemuxerPluginInnerFunc2Test : public testing::Test {
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
    typedef struct readCounts
    {
        std::vector<uint32_t> firstReadCount;
        std::vector<uint32_t> secondReadCount;
    } readCounts;

private:
    bool CreateDataSource(const std::string& filePath);
    bool CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath, int probSize);
    bool PluginSelectTracks();
    bool PluginReadSample(uint32_t idx, uint32_t count, uint32_t sleepUs);
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    bool CallbackResult();
    void RemoveValue();
    bool AssertCallback(int assertCount, uint32_t trackId, uint32_t assertFCount, uint32_t assertFCache);
    bool ReadAndAssertCache(uint32_t trackId, bool readEnd);
    bool AssertCacheBack(readCounts readCount);
    bool AssertTrackCache(uint32_t fTrackID, uint32_t sTrackID, std::vector<uint32_t> readCount);
    bool AssertSeekDts(int32_t trackId, int64_t seekTime, int64_t assertDts);
    std::vector<uint8_t> buffer_;
    std::shared_ptr<Media::StreamDemuxer> realStreamDemuxer_{ nullptr };
    std::shared_ptr<Media::MediaSource> mediaSource_{ nullptr };
    std::shared_ptr<Media::Source> realSource_{ nullptr };
    std::shared_ptr<Media::PluginBase> pluginBase_{ nullptr };
    int callbackCount_ = 0;
    int streamId_ = 0;
};

static const int DEF_PROB_SIZE = 16 * 1024;
static unique_ptr<FileServerDemo> SERVER{nullptr };
uint32_t initialFrameCount = 0;
uint32_t initialCacheSize = 0;
std::map<uint32_t, uint32_t> callbackFrameCount_;
std::map<uint32_t, uint32_t> callbackFrameCache_;
uint32_t firstLimitBytes = 8 * 1024;
uint32_t secondLimitBytes = 7 * 1024;
uint32_t testWindowMs = 500;
uint32_t secondTrackID = 1;
uint32_t firstTrackID = 0;
// uint32_t firstReadCount = 0; // 需要超出缓存上限，触发回调
// uint32_t secondReadCount = 0; // 需要超出缓存上限，触发回调

std::shared_ptr<Media::DemuxerPlugin> demuxerPlugin_;
bool readSaveTime = false;
int64_t readDts = 0;

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
static const std::string DEMUXER_PLUGIN_NAME_ASF = "avdemux_asf";
static const std::string DEMUXER_PLUGIN_NAME_RM = "avdemux_rm";
static const std::string DEMUXER_PLUGIN_NAME_EAC3 = "avdemux_eac3";
static const std::string DEMUXER_PLUGIN_NAME_DTS = "avdemux_dts";


static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_NET_FILE_PATH = "http://127.0.0.1:46666/";
static const string TEST_FILE_URI_MP4_1 = TEST_FILE_PATH + "01_video_audio.mp4";
static const string TEST_NET_URI_MP4_1 = TEST_NET_FILE_PATH + "01_video_audio.mp4";
static const string TEST_FILE_URI_MP4_2 = TEST_FILE_PATH + "demuxer_parser_hdr_1_hevc.mp4";
static const string TEST_NET_URI_MP4_2 = TEST_NET_FILE_PATH + "demuxer_parser_hdr_1_hevc.mp4";
static const string TEST_FILE_URI_MP4_3 = TEST_FILE_PATH + "mp3_h265_fmp4.mp4";
static const string TEST_NET_URI_MP4_3 = TEST_NET_FILE_PATH + "mp3_h265_fmp4.mp4";
static const string TEST_FILE_URI_MP4_4 = TEST_FILE_PATH + "m4v_fmp4.mp4";
static const string TEST_NET_URI_MP4_4 = TEST_NET_FILE_PATH + "m4v_fmp4.mp4";
static const string TEST_FILE_URI_MP4_5 = TEST_FILE_PATH + "aac_mpeg4.mp4";
static const string TEST_NET_URI_MP4_5 = TEST_NET_FILE_PATH + "aac_mpeg4.mp4";
static const string TEST_FILE_URI_3G2 = TEST_FILE_PATH + "3g2_mp4v_simple@level1_720_480_mp4a_2.3g2";
static const string TEST_NET_URI_3G2 = TEST_NET_FILE_PATH + "3g2_mp4v_simple@level1_720_480_mp4a_2.3g2";
static const string TEST_FILE_URI_3GP = TEST_FILE_PATH + "3gp_h264_high@level6__940_400_aac_2.3gp";
static const string TEST_NET_URI_3GP = TEST_NET_FILE_PATH + "3gp_h264_high@level6__940_400_aac_2.3gp";
static const string TEST_FILE_URI_MKV = TEST_FILE_PATH + "mkv.mkv";
static const string TEST_NET_URI_MKV = TEST_NET_FILE_PATH + "mkv.mkv";
static const string TEST_FILE_URI_AVI = TEST_FILE_PATH + "AVI_H263_baseline@level60_704_576_MP3_2.avi";
static const string TEST_NET_URI_AVI = TEST_NET_FILE_PATH + "AVI_H263_baseline@level60_704_576_MP3_2.avi";
static const string TEST_FILE_URI_MOV = TEST_FILE_PATH + "MPEG4_ASP@3_352_288_30_MP3_32K_1.mov";
static const string TEST_NET_URI_MOV = TEST_NET_FILE_PATH + "MPEG4_ASP@3_352_288_30_MP3_32K_1.mov";
static const string TEST_FILE_URI_MPG = TEST_FILE_PATH + "H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
static const string TEST_NET_URI_MPG = TEST_NET_FILE_PATH + "H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
static const string TEST_FILE_URI_M2TS = TEST_FILE_PATH + "h264_aac.m2ts";
static const string TEST_NET_URI_M2TS = TEST_NET_FILE_PATH + "h264_aac.m2ts";
static const string TEST_FILE_URI_MTS = TEST_FILE_PATH + "h264_aac.mts";
static const string TEST_NET_URI_MTS = TEST_NET_FILE_PATH + "h264_aac.mts";
static const string TEST_FILE_URI_TRP = TEST_FILE_PATH + "h264_aac.trp";
static const string TEST_NET_URI_TRP = TEST_NET_FILE_PATH + "h264_aac.trp";
static const string TEST_FILE_URI_M4V = TEST_FILE_PATH + "m4v_h264_high@level31_1280_720_aac_2.m4v";
static const string TEST_NET_URI_M4V = TEST_NET_FILE_PATH + "m4v_h264_high@level31_1280_720_aac_2.m4v";
static const string TEST_FILE_URI_VOB = TEST_FILE_PATH + "vob_mpeg2_main@level6_960_400_mp2_2.vob";
static const string TEST_NET_URI_VOB = TEST_NET_FILE_PATH + "vob_mpeg2_main@level6_960_400_mp2_2.vob";
static const string TEST_FILE_URI_WMV = TEST_FILE_PATH + "wmv_h264_wmav1.wmv";
static const string TEST_NET_URI_WMV = TEST_NET_FILE_PATH + "wmv_h264_wmav1.wmv";
static const string TEST_FILE_URI_WEBM = TEST_FILE_PATH + "av1_vorbis.webm";
static const string TEST_NET_URI_WEBM = TEST_NET_FILE_PATH + "av1_vorbis.webm";
static const string TEST_FILE_URI_RM = TEST_FILE_PATH + "rv30_cook.rm";
static const string TEST_NET_URI_RM = TEST_NET_FILE_PATH + "rv30_cook.rm";
static const string TEST_FILE_URI_RMVB = TEST_FILE_PATH + "rv40_cook.rmvb";
static const string TEST_NET_URI_RMVB = TEST_NET_FILE_PATH + "rv40_cook.rmvb";
static const string TEST_FILE_URI_FLV = TEST_FILE_PATH + "avc_mp3.flv";
static const string TEST_NET_URI_FLV = TEST_NET_FILE_PATH + "avc_mp3.flv";
static const string TEST_FILE_URI_TS = TEST_FILE_PATH + "h264_aac_640.ts";
static const string TEST_NET_URI_TS = TEST_NET_FILE_PATH + "h264_aac_640.ts";
static const string TEST_FILE_URI_WMA = TEST_FILE_PATH + "audio/aac_ac3.wma";
static const string TEST_NET_URI_WMA = TEST_NET_FILE_PATH + "audio/aac_ac3.wma";
static const string TEST_FILE_URI_M4A = TEST_FILE_PATH + "audio/m4a_m4a.m4a";
static const string TEST_NET_URI_M4A = TEST_NET_FILE_PATH + "audio/m4a_m4a.m4a";
static const string TEST_FILE_URI_OGG = TEST_FILE_PATH + "audio/ogg_ogg.ogg";
static const string TEST_NET_URI_OGG = TEST_NET_FILE_PATH + "audio/ogg_ogg.ogg";
static const string TEST_FILE_URI_AAC = TEST_FILE_PATH + "audio/AAC_48000_1.aac";
static const string TEST_NET_URI_AAC = TEST_NET_FILE_PATH + "audio/AAC_48000_1.aac";
static const string TEST_FILE_URI_AMR = TEST_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_NET_URI_AMR = TEST_NET_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_FILE_URI_APE = TEST_FILE_PATH + "audio/ape.ape";
static const string TEST_NET_URI_APE = TEST_NET_FILE_PATH + "audio/ape.ape";
static const string TEST_FILE_URI_MP3 = TEST_FILE_PATH + "audio/MP3_48000_1.mp3";
static const string TEST_NET_URI_MP3 = TEST_NET_FILE_PATH + "audio/MP3_48000_1.mp3";
static const string TEST_FILE_URI_FLAC = TEST_FILE_PATH + "audio/FLAC_48000_1.flac";
static const string TEST_NET_URI_FLAC = TEST_NET_FILE_PATH + "audio/FLAC_48000_1.flac";
static const string TEST_FILE_URI_WAV = TEST_FILE_PATH + "audio/wav_48000_1.wav";
static const string TEST_NET_URI_WAV = TEST_NET_FILE_PATH + "audio/wav_48000_1.wav";
static const string TEST_FILE_URI_EAC3 = TEST_FILE_PATH + "eac3.eac3";
static const string TEST_NET_URI_EAC3 = TEST_NET_FILE_PATH + "eac3.eac3";
static const string TEST_FILE_URI_DTS = TEST_FILE_PATH + "dts.dts";
static const string TEST_NET_URI_DTS = TEST_NET_FILE_PATH + "dts.dts";
static const string TEST_FILE_URI_SRT = TEST_FILE_PATH + "srt_2800.srt";
static const string TEST_NET_URI_SRT = TEST_NET_FILE_PATH + "srt_2800.srt";
static const string TEST_FILE_URI_VTT = TEST_FILE_PATH + "webvtt_test.vtt";
static const string TEST_NET_URI_VTT = TEST_NET_FILE_PATH + "webvtt_test.vtt";

void DemuxerPluginInnerFunc2Test::SetUpTestCase(void)
{
    SERVER = make_unique<FileServerDemo>();
    SERVER->StartServer();
}

void DemuxerPluginInnerFunc2Test::TearDownTestCase(void)
{
    SERVER->StopServer();
}

void DemuxerPluginInnerFunc2Test::SetUp(void)
{
}

void DemuxerPluginInnerFunc2Test::TearDown(void)
{
    dataSourceImpl_ = nullptr;
    realStreamDemuxer_ = nullptr;
    realSource_ = nullptr;
    mediaSource_ = nullptr;
    pluginBase_ = nullptr;
    demuxerPlugin_ = nullptr;
}

bool DemuxerPluginInnerFunc2Test::CreateDataSource(const std::string& filePath)
{
    cout << "file: " << filePath << endl;
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

bool DemuxerPluginInnerFunc2Test::CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath,
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
    demuxerPlugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    if (!(demuxerPlugin_->SetDataSourceWithProbSize(dataSourceImpl_, probSize) == Status::OK)) {
        printf("false return: demuxerPlugin_->SetDataSourceWithProbSize(dataSourceImpl_, probSize) != Status::OK\n");
        return false;
    }
    realStreamDemuxer_->SetDemuxerState(streamId_, DemuxerState::DEMUXER_STATE_PARSE_FIRST_FRAME);

    return true;
}

bool DemuxerPluginInnerFunc2Test::PluginSelectTracks()
{
    MediaInfo mediaInfo;
    if (!(demuxerPlugin_->GetMediaInfo(mediaInfo) == Status::OK)) {
        printf("false return: demuxerPlugin_->GetMediaInfo(mediaInfo) != Status::OK\n");
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
            demuxerPlugin_->SelectTrack(static_cast<uint32_t>(i));
        }
    }

    return true;
}

bool DemuxerPluginInnerFunc2Test::PluginReadSample(uint32_t idx, uint32_t count, uint32_t sleepUs)
{
    auto avBuf = AVBuffer::CreateAVBuffer();
    if (avBuf == nullptr) {
        printf("false return: avBuf == nullptr\n");
        return false;
    }
    for (auto i = 0; i < count; i++) {
        Status status = demuxerPlugin_->ReadSampleZeroCopy(idx, avBuf, 500);
        if (sleepUs > 0) {
            usleep(sleepUs);
        }
        if (status != Status::OK) {
            std::cout << "demuxerPlugin_->ReadSample status:" << (int)status << "  idx:" << idx << std::endl;
        }
        if (readSaveTime) {
            readDts = avBuf->dts_;
        }
    }
    return true;
}

bool DemuxerPluginInnerFunc2Test::isEOS(map<uint32_t, bool>& countFlag)
{
    auto ret = std::find_if(countFlag.begin(), countFlag.end(), [](const pair<uint32_t, bool>& p) {
        return p.second == false;
    });

    return ret == countFlag.end();
}

void DemuxerPluginInnerFunc2Test::RemoveValue()
{
    if (!callbackFrameCount_.empty()) {
        callbackFrameCount_.clear();
    }
    if (!callbackFrameCache_.empty()) {
        callbackFrameCache_.clear();
    }
    callbackCount_ = 0;
    readSaveTime = false;
    readDts = 0;
}

void CallbackThreadFun(uint32_t trackId, uint32_t bytes) {
    Status status = demuxerPlugin_->GetCurrentCacheFrameCount(trackId, initialFrameCount);
    if (status == Status::OK) {
        callbackFrameCount_[trackId] = initialFrameCount;
    }
    status = demuxerPlugin_->GetCurrentCacheSize(trackId, initialCacheSize);
    if (status == Status::OK) {
        callbackFrameCache_[trackId] = initialCacheSize;
    }
}

bool DemuxerPluginInnerFunc2Test::CallbackResult()
{
    Status status = demuxerPlugin_->SetCachePressureCallback([this](uint32_t trackId, uint32_t bytes) {
        callbackCount_ ++;
        std::thread t(CallbackThreadFun, trackId, bytes);
        t.join();
        std::cout << "CallbackResult trackId:" << trackId << "   callbackCount_:" << callbackCount_ << 
        "   initialFrameCount:" << initialFrameCount << "   initialCacheSize:" << initialCacheSize << std::endl;
    });
    if (status != Status::OK) {
        std::cout << "SetCachePressureCallback fail:" << static_cast<int32_t>(status) << std::endl;
        return false;
    }
    status = demuxerPlugin_->SetTrackCacheLimit(firstTrackID, firstLimitBytes, testWindowMs);
    EXPECT_EQ(status, Status::OK);
    status = demuxerPlugin_->SetTrackCacheLimit(secondTrackID, secondLimitBytes, testWindowMs);
    EXPECT_EQ(status, Status::OK);
    return true;
}

bool DemuxerPluginInnerFunc2Test::AssertCallback(int assertCount, uint32_t trackId, uint32_t assertFCount, uint32_t assertFCache)
{
    if (callbackCount_ != assertCount) {
        std::cout << "callbackCount_ != assertCount callbackCount_:" << callbackCount_ << "   assertCount:" << assertCount << std::endl;
        return false;
    }
    if (callbackFrameCount_.find(trackId) == callbackFrameCount_.end()) {
        std::cout << "callbackFrameCount_.find(trackId)!!!" << std::endl;
        return false;
    }
    if (callbackFrameCount_[trackId] != assertFCount) {
        std::cout << "callbackFrameCount_[trackId] != assertFCount:" << callbackFrameCount_[trackId] << "   assertFCount:" << assertFCount << std::endl;
        return false;
    }
    if (callbackFrameCache_.find(trackId) == callbackFrameCache_.end()) {
        std::cout << "callbackFrameCache_.find(trackId)!!!" << std::endl;
        return false;
    }
    if (callbackFrameCache_[trackId] != assertFCache) {
        std::cout << "callbackFrameCache_[trackId] != assertFCache:" << callbackFrameCache_[trackId] << "   assertFCache:" << assertFCache << std::endl;
        return false;
    }
    return true;
}

bool DemuxerPluginInnerFunc2Test::ReadAndAssertCache(uint32_t trackId, bool readEnd)
{
    int readSampleCount = 1;
    CallbackThreadFun(trackId, 0);
    if (readEnd) {
        readSampleCount = callbackFrameCount_[trackId];
    }
    if (!PluginReadSample(trackId, readSampleCount, 0)) {
        std::cout << "PluginReadSample fail!" << std::endl;
        return false;
    }
    Status status = demuxerPlugin_->GetCurrentCacheSize(trackId, initialCacheSize);
    if (status != Status::OK) {
        std::cout << "GetCurrentCacheSize fail!" << std::endl;
        return false;
    }
    if (callbackFrameCache_.find(trackId) == callbackFrameCache_.end()) {
        std::cout << "callbackFrameCache_.find(trackId)!!!" << std::endl;
        return false;
    }
    if (readEnd && initialCacheSize != 0) {
        std::cout << "readEnd && initialCacheSize != 0:" << initialCacheSize << std::endl;
        return false;
    }
    if (!readEnd && initialCacheSize >= callbackFrameCache_[trackId]) {
        std::cout << "initialCacheSize >= callbackFrameCache_:" << initialCacheSize << " callbackFrameCache_[trackId]:" << callbackFrameCache_[trackId] << std::endl;
        return false;
    }
    status = demuxerPlugin_->GetCurrentCacheFrameCount(trackId, initialFrameCount);
    if (status != Status::OK) {
        std::cout << "GetCurrentCacheFrameCount fail!" << std::endl;
        return false;
    }
    if (callbackFrameCount_.find(trackId) == callbackFrameCount_.end()) {
        std::cout << "callbackFrameCount_.find(trackId)!!!" << std::endl;
        return false;
    }
    if (readEnd && initialFrameCount != 0) {
        std::cout << "readEnd && initialFrameCount != 0:" << initialFrameCount << std::endl;
        return false;
    }
    if (!readEnd && initialFrameCount >= callbackFrameCount_[trackId]) {
        std::cout << "initialFrameCount >= callbackFrameCount_:" << initialFrameCount << " callbackFrameCount_[trackId]:" << callbackFrameCount_[trackId] << std::endl;
        return false;
    }
    return true;
}

bool DemuxerPluginInnerFunc2Test::AssertTrackCache(uint32_t fTrackID, uint32_t sTrackID, std::vector<uint32_t> readCount)
{
    // testcase 1
    if (!PluginReadSample(sTrackID, readCount[0], 0)) {
        std::cout << "PluginReadSample1 fail" << std::endl;
        return false;
    }
    usleep(testWindowMs * 1000);
    int cacheBytes = 0;
    if (fTrackID == 0) {
        cacheBytes = firstLimitBytes;
    } else {
        cacheBytes = secondLimitBytes;
    }
    if (callbackCount_ <= 0 || callbackFrameCount_[fTrackID] <= 0 || callbackFrameCache_[fTrackID] <= cacheBytes) {
        std::cout << "callbackCount_: " << callbackCount_ << " callbackFrameCount_[fTrackID]: " << callbackFrameCount_[fTrackID] << "callbackFrameCache_[fTrackID]: " << callbackFrameCache_[fTrackID] << "cacheBytes: " << cacheBytes << std::endl;
        return false;
    }
    // testcase 2
    if (!PluginReadSample(sTrackID, readCount[1], 0)) {
        std::cout << "PluginReadSample2 fail" << std::endl;
        return false;
    }
    usleep(testWindowMs * 1000);
    Status status = demuxerPlugin_->GetCurrentCacheFrameCount(fTrackID, initialFrameCount);
    if (status != Status::OK) {
        return false;
    }
    status = demuxerPlugin_->GetCurrentCacheSize(fTrackID, initialCacheSize);
    if (status != Status::OK) {
        return false;
    }
    if (initialFrameCount < callbackFrameCount_[fTrackID] || initialCacheSize < callbackFrameCache_[fTrackID]) {
        return false;
    }
    // testcase 3
    if (!ReadAndAssertCache(fTrackID, false)) {
        std::cout << "ReadAndAssertCache(fTrackID, false) fail" << std::endl;
        return false;
    }
    // testcase 4
    if (!ReadAndAssertCache(fTrackID, true)) {
        std::cout << "ReadAndAssertCache(fTrackID, true) fail" << std::endl;
        return false;
    }
    return true;
}

bool DemuxerPluginInnerFunc2Test::AssertCacheBack(readCounts readCount)
{
    if (!AssertTrackCache(firstTrackID, secondTrackID, readCount.secondReadCount)) {
        std::cout << "AssertTrackCache(firstTrackID, secondTrackID) fail" << std::endl;
        return false;
    }
    std::cout << "start testcase 2-1" << std::endl;
    callbackCount_ = 0;
    if (!AssertTrackCache(secondTrackID, firstTrackID, readCount.firstReadCount)) {
        std::cout << "AssertTrackCache(secondTrackID, firstTrackID) fail" << std::endl;
        return false;
    }
    return true;
}

bool DemuxerPluginInnerFunc2Test::AssertSeekDts(int32_t trackId, int64_t seekTime, int64_t assertDts)
{
    int64_t realSeekTime = 0;
    Status status = demuxerPlugin_->SeekToFrameByDts(trackId, seekTime, SeekMode::SEEK_CLOSEST, realSeekTime, 1000);
    if (status != Status::OK) {
        cout << "SeekToFrameByDts failed" << endl;
        return false;
    }
    readSaveTime = true;
    if (!PluginReadSample(trackId, 1, 0)) {
        return false;
    }
    if (readDts != assertDts) {
        std::cout << "readDts:" << readDts << "   assertDts:" << assertDts << std::endl;
        return false;
    }
    return true;
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_API_0001
 * @tc.name      : SetCachePressureCallback 入参为空
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_API_0001, TestSize.Level1)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MP4_1, TEST_NET_URI_MP4_1};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 8 * 1024;
        secondLimitBytes = 7 * 1024;
        testWindowMs = 500;
        readCounts readCount = {{6, 6}, {6, 6}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        RemoveValue();
        ASSERT_EQ(Status::OK, demuxerPlugin_->SetCachePressureCallback(nullptr));
        ASSERT_TRUE(PluginReadSample(secondTrackID, readCount.secondReadCount[0], 0));
        usleep(testWindowMs * 1000);
        ASSERT_TRUE(PluginReadSample(secondTrackID, readCount.secondReadCount[1], 0));        
        usleep(testWindowMs * 1000);
        ASSERT_EQ(callbackCount_, 0);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0001
 * @tc.name      : mp4, 设置缓存上限触发回调，seekToFrameByDts, MPEG4_MPEG3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0001, TestSize.Level0)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MP4_1, TEST_NET_URI_MP4_1};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 8 * 1024;
        secondLimitBytes = 7 * 1024;
        testWindowMs = 500;
        readCounts readCount = {{6, 6}, {6, 6}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 80, 80000), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 24, 26122), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0002
 * @tc.name      : mp4, 设置缓存上限触发回调，seekToFrameByDts, hdr10
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0002, TestSize.Level1)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MP4_2, TEST_NET_URI_MP4_2};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 574100;
        secondLimitBytes = 1000;
        testWindowMs = 500;
        readCounts readCount = {{4, 2}, {48, 3}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1001, 1001000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 448, 448000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0003
 * @tc.name      : mp4, 设置缓存上限触发回调，seekToFrameByDts, fmp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0003, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MP4_3, TEST_NET_URI_MP4_3};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 44800;
        secondLimitBytes = 1200;
        testWindowMs = 500;
        readCounts readCount = {{4, 3}, {5, 3}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, -34, -33658), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, -26, -25056), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0004
 * @tc.name      : mp4, 设置缓存上限触发回调，seekToFrameByDts, m4v
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0004, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MP4_4, TEST_NET_URI_MP4_4};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 74900;
        secondLimitBytes = 1300;
        testWindowMs = 500;
        readCounts readCount = {{4, 3}, {2, 2}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1366, 1366687), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 41, 41700), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0005
 * @tc.name      : mp4, 设置缓存上限触发回调，seekToFrameByDts, aac_mpeg4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0005, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MP4_5, TEST_NET_URI_MP4_5};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 239500;
        secondLimitBytes = 1900;
        testWindowMs = 500;
        readCounts readCount = {{6, 3}, {7, 3}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 389, 389000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 900, 900816), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0006
 * @tc.name      : 3g2, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0006, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_3G2, TEST_NET_URI_3G2};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 4410;
        secondLimitBytes = 1370;
        testWindowMs = 500;
        readCounts readCount = {{5, 5}, {5, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 500, 500000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 1578, 1578956), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0007
 * @tc.name      : 3gp, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0007, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_3GP, TEST_NET_URI_3GP};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 82280;
        secondLimitBytes = 3160;
        testWindowMs = 300;
        readCounts readCount = {{5, 5}, {245, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 5088, 5088000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 1344, 1344000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0008
 * @tc.name      : mkv, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0008, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MKV, TEST_NET_URI_MKV};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MATROSKA, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 72270;
        secondLimitBytes = 530;
        testWindowMs = 300;
        readCounts readCount = {{5, 5}, {5, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1500, 1500000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 1486, 1486000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0009
 * @tc.name      : avi, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0009, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_AVI, TEST_NET_URI_AVI};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AVI, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 50800;
        secondLimitBytes = 2000;
        testWindowMs = 500;
        readCounts readCount = {{4, 4}, {5, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 200, 200000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 208, 208979), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0010
 * @tc.name      : mov, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0010, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MOV, TEST_NET_URI_MOV};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 45200;
        secondLimitBytes = 640;
        testWindowMs = 500;
        readCounts readCount = {{4, 4}, {5, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 200, 200000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 505, 505468), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0011
 * @tc.name      : mpg, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0011, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MPG, TEST_NET_URI_MPG};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEG, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 85000;
        secondLimitBytes = 7500;
        testWindowMs = 500;
        readCounts readCount = {{5, 5}, {5, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 2210, 2210911), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 1780, 1780000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0012
 * @tc.name      : m2ts, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0012, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_M2TS, TEST_NET_URI_M2TS};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 61635;
        secondLimitBytes = 5000;
        testWindowMs = 500;
        readCounts readCount = {{9, 13}, {13, 7}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1950, 1950000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 2308, 2308000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0013
 * @tc.name      : mts, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0013, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MTS, TEST_NET_URI_MTS};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 69000;
        secondLimitBytes = 2300;
        testWindowMs = 500;
        readCounts readCount = {{9, 13}, {13, 7}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1950, 1950000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 2324, 2324000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0014
 * @tc.name      : trp, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0014, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_TRP, TEST_NET_URI_TRP};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 61900;
        secondLimitBytes = 3100;
        testWindowMs = 400;
        readCounts readCount = {{10, 5}, {13, 7}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1594, 1594355), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 2360, 2360000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0015
 * @tc.name      : m4v, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0015, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_M4V, TEST_NET_URI_M4V};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 160500;
        secondLimitBytes = 1900;
        testWindowMs = 500;
        readCounts readCount = {{4, 2}, {5, 2}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 200, 200000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 2089, 2089795), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0016
 * @tc.name      : vob, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0016, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_VOB, TEST_NET_URI_VOB};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEG, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 8000;
        secondLimitBytes = 8000;
        testWindowMs = 500;
        readCounts readCount = {{5, 2}, {3, 2}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1501, 1501000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 1275, 1275688), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0017
 * @tc.name      : wmv, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0017, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_WMV, TEST_NET_URI_WMV};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_ASF, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 48100;
        secondLimitBytes = 2900;
        testWindowMs = 500;
        readCounts readCount = {{14, 3}, {4, 3}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 1946, 1946000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 696, 696000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0018
 * @tc.name      : webm, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0018, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_WEBM, TEST_NET_URI_WEBM};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MATROSKA, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 103480;
        secondLimitBytes = 2000;
        testWindowMs = 500;
        readCounts readCount = {{17, 2}, {3, 2}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 117, 117000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 139, 139000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0019
 * @tc.name      : rm, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0019, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_RM, TEST_NET_URI_RM};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_RM, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 10200;
        secondLimitBytes = 88800;
        testWindowMs = 500;
        readCounts readCount = {{1, 80}, {53, 55}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 4092, 4092000), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 3144, 3144000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0020
 * @tc.name      : rmvb, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0020, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_RMVB, TEST_NET_URI_RMVB};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_RM, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 156400;
        secondLimitBytes = 22100;
        testWindowMs = 500;
        readCounts readCount = {{1, 48}, {81, 80}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 5600, 5600000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 5826, 5826000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0021
 * @tc.name      : flv, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0021, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_FLV, TEST_NET_URI_FLV};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLV, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 182800;
        secondLimitBytes = 800;
        testWindowMs = 500;
        readCounts readCount = {{5, 5}, {5, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 4425, 4425000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 810, 810000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0022
 * @tc.name      : ts, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0022, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_TS, TEST_NET_URI_TS};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MPEGTS, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 37000;
        secondLimitBytes = 1800;
        testWindowMs = 500;
        readCounts readCount = {{5, 5}, {10, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 4500, 4500000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 3779, 3779000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0023
 * @tc.name      : wma, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0023, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_WMA, TEST_NET_URI_WMA};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_ASF, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 2500;
        secondLimitBytes = 3000;
        testWindowMs = 500;
        readCounts readCount = {{10, 5}, {10, 5}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 175, 175000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 209, 209000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0024
 * @tc.name      : m4a, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0024, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_M4A, TEST_NET_URI_M4A};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 6000;
        secondLimitBytes = 5700;
        testWindowMs = 500;
        readCounts readCount = {{10, 10}, {10, 10}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 256, 256000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 298, 298666), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0025
 * @tc.name      : ogg, 设置缓存上限触发回调，seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0025, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_OGG, TEST_NET_URI_OGG};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_OGG, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        firstLimitBytes = 3550;
        secondLimitBytes = 4950;
        testWindowMs = 500;
        readCounts readCount = {{1, 33}, {51, 49}};
        ASSERT_EQ(CallbackResult(), true);
        ASSERT_EQ(AssertCacheBack(readCount), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 224, 224000), true);
        ASSERT_EQ(AssertSeekDts(secondTrackID, 944, 944000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0026
 * @tc.name      : aac, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0026, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_AAC, TEST_NET_URI_AAC};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AAC, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 6013, 6013968), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0027
 * @tc.name      : amr, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0027, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_AMR, TEST_NET_URI_AMR};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_AMR, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 780, 780000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0028
 * @tc.name      : ape, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0028, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_APE, TEST_NET_URI_APE};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_APE, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 9216, 9216000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0029
 * @tc.name      : mp3, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0029, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_MP3, TEST_NET_URI_MP3};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MP3, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 219240, 219240000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0030
 * @tc.name      : flac, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0030, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_FLAC, TEST_NET_URI_FLAC};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_FLAC, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 217920, 217920000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0031
 * @tc.name      : wav, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0031, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_WAV, TEST_NET_URI_WAV};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WAV, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 193792, 193792000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0032
 * @tc.name      : eac3, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0032, TestSize.Level2)
{
#ifdef SUPPORT_DEMUXER_EAC3
    std::vector<std::string> testFile = {TEST_FILE_URI_EAC3, TEST_NET_URI_EAC3};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_EAC3, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 72512, 72512000), true);
    }
#endif
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0033
 * @tc.name      : dts, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0033, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_DTS, TEST_NET_URI_DTS};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_DTS, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 4736, 4736000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0034
 * @tc.name      : srt, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0034, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_SRT, TEST_NET_URI_SRT};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_SRT, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 24900, 24900000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0035
 * @tc.name      : vtt, seekToFrameByDts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0035, TestSize.Level2)
{
    std::vector<std::string> testFile = {TEST_FILE_URI_VTT, TEST_NET_URI_VTT};
    for (const std::string& file : testFile) {
        RemoveValue();
        ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_WEBVTT, file, DEF_PROB_SIZE), true);
        ASSERT_EQ(PluginSelectTracks(), true);
        ASSERT_EQ(AssertSeekDts(firstTrackID, 6100, 6100000), true);
    }
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0036
 * @tc.name      : 缓存上限为0
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0036, TestSize.Level2)
{
    RemoveValue();
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_1, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    firstLimitBytes = 0;
    secondLimitBytes = 0;
    testWindowMs = 100;
    uint32_t readCount = 5;
    ASSERT_EQ(CallbackResult(), true);
    ASSERT_EQ(PluginReadSample(secondTrackID, readCount, 0), true);
    ASSERT_EQ(callbackCount_, 0);
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0037
 * @tc.name      : 获取未选中的轨道缓存
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0037, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_1, DEF_PROB_SIZE), true);
    demuxerPlugin_->SelectTrack(static_cast<uint32_t>(secondTrackID));
    ASSERT_EQ(CallbackResult(), true);
    uint32_t readCount = 6;
    ASSERT_EQ(PluginReadSample(secondTrackID, readCount, 0), true);
    Status status = demuxerPlugin_->GetCurrentCacheFrameCount(firstTrackID, initialFrameCount);
    ASSERT_EQ(status, Status::ERROR_INVALID_PARAMETER);
    status = demuxerPlugin_->GetCurrentCacheSize(firstTrackID, initialCacheSize);
    ASSERT_EQ(status, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0038
 * @tc.name      : 未选择轨道获取缓存
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0038, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_1, DEF_PROB_SIZE), true);
    ASSERT_EQ(CallbackResult(), true);
    Status status = demuxerPlugin_->GetCurrentCacheFrameCount(firstTrackID, initialFrameCount);
    ASSERT_EQ(status, Status::ERROR_INVALID_OPERATION);
    status = demuxerPlugin_->GetCurrentCacheSize(firstTrackID, initialCacheSize);
    ASSERT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_CACHE_CALLBACK_INNER_FUNC_0039
 * @tc.name      : 插件未初始化化完成
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_CACHE_CALLBACK_INNER_FUNC_0039, TestSize.Level2)
{
    ASSERT_EQ(CreateDataSource(TEST_FILE_URI_MP4_1) ,true);
    pluginBase_ = Plugins::PluginManagerV2::Instance().CreatePluginByName(DEMUXER_PLUGIN_NAME_MOV_S);
    ASSERT_NE(pluginBase_, nullptr);
    demuxerPlugin_ = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_NE(demuxerPlugin_, nullptr);
    Status status = demuxerPlugin_->GetCurrentCacheFrameCount(firstTrackID, initialFrameCount);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
    status = demuxerPlugin_->GetCurrentCacheSize(firstTrackID, initialCacheSize);
    ASSERT_EQ(status, Status::ERROR_NULL_POINTER);
}

/**
 * @tc.number    : DEMUXER_SEEK_DTS_INNER_FUNC_0040
 * @tc.name      : 不存在的seektime
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_SEEK_DTS_INNER_FUNC_0040, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_1, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    int64_t realSeekTime = 0;
    int64_t nonExistentSeekTime = 999999999999999;
    Status status = demuxerPlugin_->SeekToFrameByDts(firstTrackID, nonExistentSeekTime, SeekMode::SEEK_CLOSEST, realSeekTime, 1000);
    ASSERT_EQ(status, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_SEEK_DTS_INNER_FUNC_0041
 * @tc.name      : 不存在的trackId
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_SEEK_DTS_INNER_FUNC_0041, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_1, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    int64_t realSeekTime = 0;
    int64_t seekTimeDts = 7200;
    int64_t nonExistentTrackId = 3;
    Status status = demuxerPlugin_->SeekToFrameByDts(nonExistentTrackId, seekTimeDts, SeekMode::SEEK_CLOSEST, realSeekTime, 1000);
    ASSERT_EQ(status, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_SEEK_DTS_INNER_FUNC_0042
 * @tc.name      : 未选中的trackId
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_SEEK_DTS_INNER_FUNC_0042, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_1, DEF_PROB_SIZE), true);
    demuxerPlugin_->SelectTrack(static_cast<uint32_t>(secondTrackID));
    int64_t realSeekTime = 0;
    int64_t seekTimeDts = 7200;
    Status status = demuxerPlugin_->SeekToFrameByDts(firstTrackID, seekTimeDts, SeekMode::SEEK_CLOSEST, realSeekTime, 1000);
    ASSERT_EQ(status, Status::ERROR_INVALID_OPERATION);
}

/**
 * @tc.number    : DEMUXER_SEEK_DTS_INNER_FUNC_0043
 * @tc.name      : SeekMode非SeekMode::SEEK_CLOSEST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerPluginInnerFunc2Test, DEMUXER_SEEK_DTS_INNER_FUNC_0043, TestSize.Level2)
{
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MP4_1, DEF_PROB_SIZE), true);
    ASSERT_EQ(PluginSelectTracks(), true);
    int64_t realSeekTime = 0;
    int64_t seekTimeDts = 7200;
    Status status = demuxerPlugin_->SeekToFrameByDts(firstTrackID, seekTimeDts, SeekMode::SEEK_NEXT_SYNC, realSeekTime, 1000);
    ASSERT_EQ(status, Status::ERROR_INVALID_PARAMETER);
}
}  // namespace Media
}  // namespace OHOS