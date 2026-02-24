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
#include "file_server_demo.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace testing;
using namespace std;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace Media {

class DemuxerZerocopyInnerFuncTest : public testing::Test {
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
    typedef struct TestFileInfo {
        string pluginName;
        string fileName;
        vector<uint32_t> frameCnt;
        vector<uint32_t> keyFrameCnt;
        TestFileInfo(string name, string file, vector<uint32_t> &&cnt, vector<uint32_t> &&keyCnt)
            : pluginName(name), fileName(file), frameCnt(std::move(cnt)), keyFrameCnt(std::move(keyCnt)) {}
    } TestFileInfo;

private:
    bool CreateDataSource(const std::string& filePath);
    bool CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath, int probSize);
    bool PluginSelectTracks();
    bool PluginReadSample(uint32_t idx, uint32_t& flag);
    void CountFrames(uint32_t index, uint32_t flag);
    void SetEosValue();
    bool isEOS(std::map<uint32_t, bool>& countFlag);
    void RemoveValue();
    bool ResultAssert(std::vector<uint32_t> &framesExpect, std::vector<uint32_t> &keyFramesExpect);
    bool PluginReadAllSample();
    bool PluginCheckDemuxerResult(TestFileInfo info);

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

static const int DEF_PROB_SIZE = 16 * 1024;
uint32_t TIMEOUT = 10000;
static unique_ptr<FileServerDemo> SERVER{nullptr };

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
static const std::string DEMUXER_PLUGIN_NAME_ASF = "avdemux_asf";
static const std::string DEMUXER_PLUGIN_NAME_EAC3 = "avdemux_eac3";
static const std::string DEMUXER_PLUGIN_NAME_RM = "avdemux_rm";
static const std::string DEMUXER_PLUGIN_NAME_DTS = "avdemux_dts";

static const string TEST_FILE_PATH = "/data/test/media/";
static const string TEST_URI_FILE_PATH = "http://127.0.0.1:46666/";
static const string TEST_FILE_LOCAL_AVC_AAC = TEST_FILE_PATH + "3g2_h264_720_480_mp4a_31.3g2";
static const string TEST_FILE_URI_AVC_AAC = TEST_URI_FILE_PATH + "3g2_h264_720_480_mp4a_31.3g2";
static const string TEST_FILE_LOCAL_HEVC_AAC = TEST_FILE_PATH + "aac_h265.mkv";
static const string TEST_FILE_URI_HEVC_AAC = TEST_URI_FILE_PATH + "aac_h265.mkv";
static const string TEST_FILE_LOCAL_HDRVIVID_AV3A = TEST_FILE_PATH + "audiovivid_hdrvivid_1s_fmp4.mp4";
static const string TEST_FILE_URI_HDRVIVID_AV3A = TEST_URI_FILE_PATH + "audiovivid_hdrvivid_1s_fmp4.mp4";
static const string TEST_FILE_LOCAL_263_MP3 = TEST_FILE_PATH + "AVI_H263_baseline@level60_704_576_MP3_2.avi";
static const string TEST_FILE_URI_263_MP3 = TEST_URI_FILE_PATH + "AVI_H263_baseline@level60_704_576_MP3_2.avi";
static const string TEST_FILE_LOCAL_MPEG2_MP2 = TEST_FILE_PATH + "MPEG2_main_352_288_30_MP2_44.1K_1.mpg";
static const string TEST_FILE_URI_MPEG2_MP2 = TEST_URI_FILE_PATH + "MPEG2_main_352_288_30_MP2_44.1K_1.mpg";
static const string TEST_FILE_LOCAL_MPEG4_AAC = TEST_FILE_PATH + "mpeg4_aac_gltf.mp4";
static const string TEST_FILE_URI_MPEG4_AAC = TEST_URI_FILE_PATH + "mpeg4_aac_gltf.mp4";
static const string TEST_FILE_LOCAL_AVC_OPUS = TEST_FILE_PATH + "opus_h264.mkv";
static const string TEST_FILE_URI_AVC_OPUS = TEST_URI_FILE_PATH + "opus_h264.mkv";
static const string TEST_FILE_LOCAL_VORBIS = TEST_FILE_PATH + "audio/OGG_8K_1.ogg";
static const string TEST_FILE_URI_VORBIS = TEST_URI_FILE_PATH + "audio/OGG_8K_1.ogg";
static const string TEST_FILE_LOCAL_FLAC = TEST_FILE_PATH + "audio/FLAC_16K_24b_1.flac";
static const string TEST_FILE_URI_FLAC = TEST_URI_FILE_PATH + "audio/FLAC_16K_24b_1.flac";
static const string TEST_FILE_LOCAL_AMRNB = TEST_FILE_PATH + "audio/amr_nb.wma";
static const string TEST_FILE_URI_AMRNB = TEST_URI_FILE_PATH + "audio/amr_nb.wma";
static const string TEST_FILE_LOCAL_AMRWB = TEST_FILE_PATH + "audio/amr_wb.wma";
static const string TEST_FILE_URI_AMRWB = TEST_URI_FILE_PATH + "audio/amr_wb.wma";
static const string TEST_FILE_LOCAL_APE = TEST_FILE_PATH + "audio/ape.ape";
static const string TEST_FILE_URI_APE = TEST_URI_FILE_PATH + "audio/ape.ape";
static const string TEST_FILE_LOCAL_PCM = TEST_FILE_PATH + "audio/gb2312.wav";
static const string TEST_FILE_URI_PCM = TEST_URI_FILE_PATH + "audio/gb2312.wav";
static const string TEST_FILE_LOCAL_PCMMULAW = TEST_FILE_PATH + "audio/pcm_mulaw.wma";
static const string TEST_FILE_URI_PCMMULAW = TEST_URI_FILE_PATH + "audio/pcm_mulaw.wma";
static const string TEST_FILE_LOCAL_PCMALAW = TEST_FILE_PATH + "audio/pcm_alaw.wma";
static const string TEST_FILE_URI_PCMALAW = TEST_URI_FILE_PATH + "audio/pcm_alaw.wma";
static const string TEST_FILE_LOCAL_VVC = TEST_FILE_PATH + "vvc_1280_720_8.mp4";
static const string TEST_FILE_URI_VVC = TEST_URI_FILE_PATH + "vvc_1280_720_8.mp4";
static const string TEST_FILE_LOCAL_PCMS16LE = TEST_FILE_PATH + "hevc_pcm_a.flv";
static const string TEST_FILE_URI_PCMS16LE = TEST_URI_FILE_PATH + "hevc_pcm_a.flv";
static const string TEST_FILE_LOCAL_PCMS24LE = TEST_FILE_PATH + "AVI_MPEG2_422P@high_1920_1080_PCM_s24_1.avi";
static const string TEST_FILE_URI_PCMS24LE = TEST_URI_FILE_PATH + "AVI_MPEG2_422P@high_1920_1080_PCM_s24_1.avi";
static const string TEST_FILE_LOCAL_PCMS32LE = TEST_FILE_PATH + "AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi";
static const string TEST_FILE_URI_PCMS32LE = TEST_URI_FILE_PATH + "AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi";
static const string TEST_FILE_LOCAL_PCMF32LE = TEST_FILE_PATH + "audio/pcm_f32le.wma";
static const string TEST_FILE_URI_PCMF32LE = TEST_URI_FILE_PATH + "audio/pcm_f32le.wma";
static const string TEST_FILE_LOCAL_WMV3 = TEST_FILE_PATH + "wmv_wmv3_main@level99_320_240_1.wmv";
static const string TEST_FILE_URI_WMV3 = TEST_URI_FILE_PATH + "wmv_wmv3_main@level99_320_240_1.wmv";
static const string TEST_FILE_LOCAL_EAC3 = TEST_FILE_PATH + "eac3.eac3";
static const string TEST_FILE_URI_EAC3 = TEST_URI_FILE_PATH + "eac3.eac3";
static const string TEST_FILE_LOCAL_WMAV1 = TEST_FILE_PATH + "wmv_h264_wmav1.wmv";
static const string TEST_FILE_URI_WMAV1 = TEST_URI_FILE_PATH + "wmv_h264_wmav1.wmv";
static const string TEST_FILE_LOCAL_WMAV2 = TEST_FILE_PATH + "wmv_h264_wmav2.wmv";
static const string TEST_FILE_URI_WMAV2 = TEST_URI_FILE_PATH + "wmv_h264_wmav2.wmv";
static const string TEST_FILE_LOCAL_WMAPRO = TEST_FILE_PATH + "wmv_wmv3_wmapro.wmv";
static const string TEST_FILE_URI_WMAPRO = TEST_URI_FILE_PATH + "wmv_wmv3_wmapro.wmv";
static const string TEST_FILE_LOCAL_ALAC = TEST_FILE_PATH + "ALAC_16bit_44100Hz.m4a";
static const string TEST_FILE_URI_ALAC = TEST_URI_FILE_PATH + "ALAC_16bit_44100Hz.m4a";
static const string TEST_FILE_LOCAL_GSMMS = TEST_FILE_PATH + "audio/gsmms.wma";
static const string TEST_FILE_URI_GSMMS = TEST_URI_FILE_PATH + "audio/gsmms.wma";
static const string TEST_FILE_LOCAL_ADPCMG722 = TEST_FILE_PATH + "ADPCM_G722.avi";
static const string TEST_FILE_URI_ADPCMG722 = TEST_URI_FILE_PATH + "ADPCM_G722.avi";
static const string TEST_FILE_LOCAL_ADPCMYAMAHA = TEST_FILE_PATH + "audio/adpcm_yamaha.wma";
static const string TEST_FILE_URI_ADPCMYAMAHA = TEST_URI_FILE_PATH + "audio/adpcm_yamaha.wma";
static const string TEST_FILE_LOCAL_ADPCMMS = TEST_FILE_PATH + "audio/adpcm_ms.wma";
static const string TEST_FILE_URI_ADPCMMS = TEST_URI_FILE_PATH + "audio/adpcm_ms.wma";
static const string TEST_FILE_LOCAL_ADPCMIMAWAV = TEST_FILE_PATH + "audio/adpcm_ima_wav.wma";
static const string TEST_FILE_URI_ADPCMIMAWAV = TEST_URI_FILE_PATH + "audio/adpcm_ima_wav.wma";
static const string TEST_FILE_LOCAL_ADPCMG726 = TEST_FILE_PATH + "audio/adpcm_g726.wma";
static const string TEST_FILE_URI_ADPCMG726 = TEST_URI_FILE_PATH + "audio/adpcm_g726.wma";
static const string TEST_FILE_LOCAL_ADPCMIMAQT = TEST_FILE_PATH + "adpcm_ima_qt.mov";
static const string TEST_FILE_URI_ADPCMIMAQT = TEST_URI_FILE_PATH + "adpcm_ima_qt.mov";
static const string TEST_FILE_LOCAL_PCMDVD = TEST_FILE_PATH + "vob_mpeg2_main@level6_960_400_pcm_2.vob";
static const string TEST_FILE_URI_PCMDVD = TEST_URI_FILE_PATH + "vob_mpeg2_main@level6_960_400_pcm_2.vob";
static const string TEST_FILE_LOCAL_AV1 = TEST_FILE_PATH + "av1_vorbis.webm";
static const string TEST_FILE_URI_AV1 = TEST_URI_FILE_PATH + "av1_vorbis.webm";
static const string TEST_FILE_LOCAL_VP8 = TEST_FILE_PATH + "opus_vp8.webm";
static const string TEST_FILE_URI_VP8 = TEST_URI_FILE_PATH + "opus_vp8.webm";
static const string TEST_FILE_LOCAL_VP9 = TEST_FILE_PATH + "vorbis_vp9.webm";
static const string TEST_FILE_URI_VP9 = TEST_URI_FILE_PATH + "vorbis_vp9.webm";
static const string TEST_FILE_LOCAL_RV40 = TEST_FILE_PATH + "rv40_aac.rm";
static const string TEST_FILE_URI_RV40 = TEST_URI_FILE_PATH + "rv40_aac.rm";
static const string TEST_FILE_LOCAL_MPEG1 = TEST_FILE_PATH + "vob_mpeg1_main@level6_960_400_mp2_2.vob";
static const string TEST_FILE_URI_MPEG1 = TEST_URI_FILE_PATH + "vob_mpeg1_main@level6_960_400_mp2_2.vob";
static const string TEST_FILE_LOCAL_RV30_COOK = TEST_FILE_PATH + "rv30_cook.rm";
static const string TEST_FILE_URI_RV30_COOK = TEST_URI_FILE_PATH + "rv30_cook.rm";
static const string TEST_FILE_LOCAL_DTS = TEST_FILE_PATH + "dts.dts";
static const string TEST_FILE_URI_DTS = TEST_URI_FILE_PATH + "dts.dts";
static const string TEST_FILE_LOCAL_TRUEHD = TEST_FILE_PATH + "truehd.mkv";
static const string TEST_FILE_URI_TRUEHD = TEST_URI_FILE_PATH + "truehd.mkv";
static const string TEST_FILE_LOCAL_DVAUDIO = TEST_FILE_PATH + "dvaudio_48000_stereo.wav";
static const string TEST_FILE_URI_DVAUDIO = TEST_URI_FILE_PATH + "dvaudio_48000_stereo.wav";
static const string TEST_FILE_LOCAL_ILBC = TEST_FILE_PATH + "ilbc_mov.mov";
static const string TEST_FILE_URI_ILBC = TEST_URI_FILE_PATH + "ilbc_mov.mov";
static const string TEST_FILE_LOCAL_PCMBLURAY = TEST_FILE_PATH + "pcm_bluray.m2ts";
static const string TEST_FILE_URI_PCMBLURAY = TEST_URI_FILE_PATH + "pcm_bluray.m2ts";
static const string TEST_FILE_LOCAL_VC1 = TEST_FILE_PATH + "vc1_ts.ts";
static const string TEST_FILE_URI_VC1 = TEST_URI_FILE_PATH + "vc1_ts.ts";
static const string TEST_FILE_LOCAL_MSVIDEO1 = TEST_FILE_PATH + "msvideo1_mkv.mkv";
static const string TEST_FILE_URI_MSVIDEO1 = TEST_URI_FILE_PATH + "msvideo1_mkv.mkv";
static const string TEST_FILE_LOCAL_MJPEG = TEST_FILE_PATH + "mjpeg.avi";
static const string TEST_FILE_URI_MJPEG = TEST_URI_FILE_PATH + "mjpeg.avi";
static const string TEST_FILE_LOCAL_DVCNTSC = TEST_FILE_PATH + "DVCNTSC_720x480_25_422_dv5n.mov";
static const string TEST_FILE_URI_DVCNTSC = TEST_URI_FILE_PATH + "DVCNTSC_720x480_25_422_dv5n.mov";
static const string TEST_FILE_LOCAL_DVCPAL = TEST_FILE_PATH + "DVCPAL_720x576_25_411_dvpp.mov";
static const string TEST_FILE_URI_DVCPAL = TEST_URI_FILE_PATH + "DVCPAL_720x576_25_411_dvpp.mov";
static const string TEST_FILE_LOCAL_DVCPROHD = TEST_FILE_PATH + "DVCPROHD_1280x1080_4_422_dvh6.mov";
static const string TEST_FILE_URI_DVCPROHD = TEST_URI_FILE_PATH + "DVCPROHD_1280x1080_4_422_dvh6.mov";
static const string TEST_FILE_LOCAL_RAWVIDEO = TEST_FILE_PATH + "rawvideo_4_4_30.avi";
static const string TEST_FILE_URI_RAWVIDEO = TEST_URI_FILE_PATH + "rawvideo_4_4_30.avi";
static const string TEST_FILE_LOCAL_MUXER_PREY_DEPTH = TEST_FILE_PATH + "Muxer_Auxiliary_04.mp4";
static const string TEST_FILE_URI_MUXER_PREY_DEPTH = TEST_URI_FILE_PATH + "Muxer_Auxiliary_04.mp4";
static const string TEST_FILE_LOCAL_MUXER_META = TEST_FILE_PATH + "Timedmetadata1Track2.mp4";
static const string TEST_FILE_URI_MUXER_META = TEST_URI_FILE_PATH + "Timedmetadata1Track2.mp4";
static const string TEST_FILE_LOCAL_MUXER_AAC = TEST_FILE_PATH + "Muxer_Auxiliary_02.mp4";
static const string TEST_FILE_URI_MUXER_AAC = TEST_URI_FILE_PATH + "Muxer_Auxiliary_02.mp4";
static const string TEST_FILE_LOCAL_MUXER_MP3 = TEST_FILE_PATH + "Muxer_Auxiliary_03.mp4";
static const string TEST_FILE_URI_MUXER_MP3 = TEST_URI_FILE_PATH + "Muxer_Auxiliary_03.mp4";
static const string TEST_FILE_LOCAL_MPG = TEST_FILE_PATH + "H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
static const string TEST_FILE_URI_MPG = TEST_URI_FILE_PATH + "H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
static const string TEST_FILE_LOCAL_AAC = TEST_FILE_PATH + "audio/AAC_8K_1.aac";
static const string TEST_FILE_URI_AAC = TEST_URI_FILE_PATH + "audio/AAC_8K_1.aac";
static const string TEST_FILE_LOCAL_MP3 = TEST_FILE_PATH + "audio/MP3_8K_1.mp3";
static const string TEST_FILE_URI_MP3 = TEST_URI_FILE_PATH + "audio/MP3_8K_1.mp3";
static const string TEST_FILE_LOCAL_AMR = TEST_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_FILE_URI_AMR = TEST_URI_FILE_PATH + "audio/amr_nb_8000_1.amr";
static const string TEST_FILE_LOCAL_SRT = TEST_FILE_PATH + "srt_2800.srt";
static const string TEST_FILE_URI_SRT = TEST_URI_FILE_PATH + "srt_2800.srt";
static const string TEST_FILE_LOCAL_VTT = TEST_FILE_PATH + "webvtt_test.vtt";
static const string TEST_FILE_URI_VTT = TEST_URI_FILE_PATH + "webvtt_test.vtt";
static const string TEST_FILE_LOCAL_M4V = TEST_FILE_PATH + "m4v_h264_alac.m4v";
static const string TEST_FILE_URI_M4V = TEST_URI_FILE_PATH + "m4v_h264_alac.m4v";
static const string TEST_FILE_LOCAL_3GP = TEST_FILE_PATH + "3gp_amr_wb.3gp";
static const string TEST_FILE_URI_3GP = TEST_URI_FILE_PATH + "3gp_amr_wb.3gp";
static const string TEST_FILE_LOCAL_MTS = TEST_FILE_PATH + "mpeg2_aac.mts";
static const string TEST_FILE_URI_MTS = TEST_URI_FILE_PATH + "mpeg2_aac.mts";
static const string TEST_FILE_LOCAL_TRP = TEST_FILE_PATH + "mpeg2_mp3.trp";
static const string TEST_FILE_URI_TRP = TEST_URI_FILE_PATH + "mpeg2_mp3.trp";
static const string TEST_FILE_LOCAL_RMVB = TEST_FILE_PATH + "rv40_cook.rmvb";
static const string TEST_FILE_URI_RMVB = TEST_URI_FILE_PATH + "rv40_cook.rmvb";

void DemuxerZerocopyInnerFuncTest::SetUpTestCase(void)
{
    SERVER = make_unique<FileServerDemo>();
    SERVER->StartServer();
}

void DemuxerZerocopyInnerFuncTest::TearDownTestCase(void)
{
    SERVER->StopServer();
}

void DemuxerZerocopyInnerFuncTest::SetUp(void)
{
}

void DemuxerZerocopyInnerFuncTest::TearDown(void)
{
    dataSourceImpl_ = nullptr;
}

bool DemuxerZerocopyInnerFuncTest::CreateDataSource(const std::string& filePath)
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

bool DemuxerZerocopyInnerFuncTest::CreateDemuxerPluginByName(const std::string& typeName, const std::string& filePath,
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

bool DemuxerZerocopyInnerFuncTest::PluginSelectTracks()
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
        cout << "i: " << i << " mime: " << mime << endl;
        if (mime.find("video/") == 0) {
            printf("the video track id is %d\n", i);
        }
        if (mime.find("audio/") == 0) {
            printf("the audio track id is %d\n", i);
        }
        if (mime.find("meta/") == 0) {
            printf("the meta track id is %d\n", i);
        }
        demuxerPlugin->SelectTrack(static_cast<uint32_t>(i));
        selectedTrackIds_.push_back(static_cast<uint32_t>(i));
        frames_[i] = 0;
        keyFrames_[i] = 0;
        eosFlag_[i] = false;
    }

    return true;
}

bool DemuxerZerocopyInnerFuncTest::PluginReadSample(uint32_t idx, uint32_t& flag)
{
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    auto avBuf = AVBuffer::CreateAVBuffer();
    if (!(avBuf != nullptr)) {
        printf("false return: avBuf == nullptr\n");
        return false;
    }
    if (demuxerPlugin->ReadSampleZeroCopy(idx, avBuf, TIMEOUT) != Status::OK) {
        printf("false return: ReadSampleZeroCopy failed\n");
        return false;
    }
    flag = avBuf->flag_;

    return true;
}

void DemuxerZerocopyInnerFuncTest::CountFrames(uint32_t index, uint32_t flag)
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

bool DemuxerZerocopyInnerFuncTest::isEOS(map<uint32_t, bool>& countFlag)
{
    auto ret = std::find_if(countFlag.begin(), countFlag.end(), [](const pair<uint32_t, bool>& p) {
        return p.second == false;
    });

    return ret == countFlag.end();
}

void DemuxerZerocopyInnerFuncTest::SetEosValue()
{
    std::for_each(selectedTrackIds_.begin(), selectedTrackIds_.end(), [this](uint32_t idx) {
        eosFlag_[idx] = true;
    });
}

void DemuxerZerocopyInnerFuncTest::RemoveValue()
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

bool DemuxerZerocopyInnerFuncTest::ResultAssert(std::vector<uint32_t> &framesExpect,
    std::vector<uint32_t> &keyFramesExpect)
{
    for (size_t i = 0; i < framesExpect.size(); i++) {
        printf("frames_[%zu]=%d | kFrames_[%zu]=%d\n", i, frames_[i], i, keyFrames_[i]);
        if (frames_[i] != framesExpect[i]) {
            printf("Expected frames_[%zu] = %u, but got %d\n", i, framesExpect[i], frames_[i]);
            return false;
        }
        if (keyFrames_[i] != keyFramesExpect[i]) {
            printf("Expected keyFrames_[%zu] = %u, but got %d\n", i, keyFramesExpect[i], keyFrames_[i]);
            return false;
        }
    }
    return true;
}

bool DemuxerZerocopyInnerFuncTest::PluginReadAllSample()
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

bool DemuxerZerocopyInnerFuncTest::PluginCheckDemuxerResult(TestFileInfo info)
{
    if (!CreateDemuxerPluginByName(info.pluginName, info.fileName, DEF_PROB_SIZE)) {
        return false;
    }
    if (!PluginSelectTracks()) {
        return false;
    }
    if (!PluginReadAllSample()) {
        return false;
    }
    if (!ResultAssert(info.frameCnt, info.keyFrameCnt)) {
        return false;
    }
    RemoveValue();
    return true;
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_API_0001
 * @tc.name      : trackId为 UNINT32_T_MAX
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_API_0001, TestSize.Level0)
{
    uint32_t indexVid = 4294967295;
    uint32_t timeout = 100;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_AVC_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    auto avBuf = AVBuffer::CreateAVBuffer();
    ASSERT_NE(avBuf, nullptr);
    ASSERT_EQ(demuxerPlugin->ReadSampleZeroCopy(indexVid, avBuf, timeout), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_API_0002
 * @tc.name      : buffer为nullptr
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_API_0002, TestSize.Level0)
{
    uint32_t indexVid = 0;
    uint32_t timeout = 100;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_AVC_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    ASSERT_EQ(demuxerPlugin->ReadSampleZeroCopy(indexVid, nullptr, timeout), Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_API_0003
 * @tc.name      : timeout为 UNINT32_T_MAX
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_API_0003, TestSize.Level0)
{
    uint32_t indexVid = 0;
    uint32_t timeout = 4294967295;
    ASSERT_EQ(CreateDemuxerPluginByName(DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_AVC_AAC, DEF_PROB_SIZE), true);
    ASSERT_NE(pluginBase_, nullptr);
    auto demuxerPlugin = std::static_pointer_cast<Plugins::DemuxerPlugin>(pluginBase_);
    ASSERT_EQ(demuxerPlugin->SelectTrack(0), Status::OK);
    ASSERT_EQ(demuxerPlugin->SelectTrack(1), Status::OK);
    auto avBuf = AVBuffer::CreateAVBuffer();
    ASSERT_NE(avBuf, nullptr);
    ASSERT_EQ(demuxerPlugin->ReadSampleZeroCopy(indexVid, avBuf, timeout), Status::OK);
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0001
 * @tc.name      : create empty buffer for read, local and uri, 3g2
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0001, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_AVC_AAC, {180, 216}, {5, 216}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_AVC_AAC
    , {180, 216}, {5, 216}}));
}

/**
 * @tc.number    : DEMUXER_CREATE_PLUGIN_BY_NAME_INNER_FUNC_0002
 * @tc.name      : create empty buffer for read, local and uri, mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0002, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_LOCAL_HEVC_AAC,
        {372, 526}, {2, 526}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_HEVC_AAC, {372, 526}, {2, 526}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_LOCAL_AVC_OPUS,
        {372, 610}, {2, 610}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_AVC_OPUS, {372, 610}, {2, 610}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_LOCAL_TRUEHD, {5513}, {345}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_TRUEHD, {5513}, {345}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_LOCAL_MSVIDEO1, {98}, {4}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_MSVIDEO1, {98}, {4}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0003
 * @tc.name      : create empty buffer for read, local and uri, mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0003, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_HDRVIVID_AV3A,
        {26, 44}, {1, 44}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_HDRVIVID_AV3A, {26, 44}, {1, 44}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_MPEG4_AAC, {2, 2}, {1, 2}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MPEG4_AAC, {2, 2}, {1, 2}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_VVC, {254}, {4,}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_VVC, {254}, {4,}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_MUXER_PREY_DEPTH,
        {16,77, 77}, {1, 3, 3}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MUXER_PREY_DEPTH,
        {16,77, 77}, {1, 3, 3}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_MUXER_META,
        {210, 211, 210}, {210, 211, 210}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MUXER_META,
        {210, 211, 210}, {210, 211, 210}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_MUXER_AAC,
        {431, 431, 431, 231}, {431, 431, 431, 231}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MUXER_AAC,
        {431, 431, 431, 231}, {431, 431, 431, 231}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_MUXER_MP3,
        {417, 417, 417, 223}, {417, 417, 417, 223}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_MUXER_MP3,
        {417, 417, 417, 223}, {417, 417, 417, 223}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0004
 * @tc.name      : create empty buffer for read, local and uri, avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0004, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_LOCAL_263_MP3, {28, 39}, {28, 39}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_263_MP3, {28, 39}, {28, 39}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_LOCAL_PCMS24LE, {30, 17}, {3, 17}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_PCMS24LE, {30, 17}, {3, 17}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_LOCAL_PCMS32LE, {30, 44}, {3, 44}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_PCMS32LE, {30, 44}, {3, 44}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_LOCAL_ADPCMG722, {1383}, {1383}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_ADPCMG722, {1383}, {1383}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_LOCAL_MJPEG, {30}, {30}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_MJPEG, {30}, {30}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_LOCAL_RAWVIDEO, {7}, {7}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AVI, TEST_FILE_URI_RAWVIDEO, {7}, {7}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0005
 * @tc.name      : create empty buffer for read, local and uri, mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0005, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_LOCAL_MPEG2_MP2, {60, 77}, {6, 77}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPEG2_MP2, {60, 77}, {6, 77}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_LOCAL_MPG, {60, 77}, {6, 77}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPG, {60, 77}, {6, 77}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0006
 * @tc.name      : create empty buffer for read, local and uri, ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0006, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_LOCAL_VORBIS, {6863}, {6863}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_OGG, TEST_FILE_URI_VORBIS, {6863}, {6863}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0007
 * @tc.name      : create empty buffer for read, local and uri, wma
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0007, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_AMRNB, {827}, {827}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_AMRNB, {827}, {827}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_AMRWB, {1383}, {1383}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_AMRWB, {1383}, {1383}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_PCMMULAW, {130}, {130}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_PCMMULAW, {130}, {130}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_PCMMULAW, {130}, {130}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_PCMMULAW, {130}, {130}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_PCMF32LE, {433}, {433}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_PCMF32LE, {433}, {433}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_GSMMS, {251}, {251}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_GSMMS, {251}, {251}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_ADPCMYAMAHA, {65}, {65}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_ADPCMYAMAHA, {65}, {65}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_ADPCMMS, {65}, {65}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_ADPCMMS, {65}, {65}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_ADPCMIMAWAV, {65}, {65}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_ADPCMIMAWAV, {65}, {65}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_ADPCMG726, {12}, {12}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_ADPCMG726, {12}, {12}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0008
 * @tc.name      : create empty buffer for read, local and uri, ape
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0008, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_APE, TEST_FILE_LOCAL_APE, {8}, {8}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_APE, TEST_FILE_URI_APE, {8}, {8}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0009
 * @tc.name      : create empty buffer for read, local and uri, wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0009, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_LOCAL_PCM, {5146}, {5146}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_PCM, {5146}, {5146}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_LOCAL_DVAUDIO, {100}, {100}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_WAV, TEST_FILE_URI_DVAUDIO, {100}, {100}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0010
 * @tc.name      : create empty buffer for read, local and uri, flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0010, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_LOCAL_PCMS16LE, {602, 385}, {3, 385}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_FLV, TEST_FILE_URI_PCMS16LE, {602, 385}, {3, 385}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0011
 * @tc.name      : create empty buffer for read, local and uri, wmv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0011, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_WMV3, {2641}, {68}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_WMV3, {2641}, {68}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_WMAV1, {602, 218}, {3, 218}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_WMAV1, {602, 218}, {3, 218}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_WMAV2, {602, 218}, {3, 218}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_WMAV2, {602, 218}, {3, 218}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_LOCAL_WMAPRO, {122, 480}, {122, 8}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_ASF, TEST_FILE_URI_WMAPRO, {122, 480}, {122, 8}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0012
 * @tc.name      : create empty buffer for read, local and uri, eac3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0012, TestSize.Level2)
{
#ifdef SUPPORT_DEMUXER_EAC3
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_EAC3, TEST_FILE_LOCAL_EAC3, {6862}, {6862}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_EAC3, TEST_FILE_URI_EAC3, {6862}, {6862}}));
#endif
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0013
 * @tc.name      : create empty buffer for read, local and uri, m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0013, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_ALAC, {54}, {54}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_ALAC, {54}, {54}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0014
 * @tc.name      : create empty buffer for read, local and uri, mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0014, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_ADPCMIMAQT, {6912}, {6912}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_ADPCMIMAQT, {6912}, {6912}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_ILBC, {250}, {250}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_ILBC, {250}, {250}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_DVCNTSC, {5}, {5}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_DVCNTSC, {5}, {5}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_DVCPAL, {5}, {5}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_DVCPAL, {5}, {5}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_DVCPROHD, {5}, {5}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_DVCPROHD, {5}, {5}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0015
 * @tc.name      : create empty buffer for read, local and uri, vob
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0015, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_LOCAL_PCMDVD, {699, 2714}, {73, 2714}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_PCMDVD, {699, 2714}, {73, 2714}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_LOCAL_MPEG1, {1116, 1943}, {114, 1943}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEG, TEST_FILE_URI_MPEG1, {1116, 1943}, {114, 1943}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0016
 * @tc.name      : create empty buffer for read, local and uri, webm
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0016, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_LOCAL_AV1, {602, 433}, {1, 433}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_AV1, {602, 433}, {1, 433}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_LOCAL_VP8, {602, 502}, {5, 502}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_VP8, {602, 502}, {5, 502}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_LOCAL_VP9, {602, 433}, {5, 433}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MATROSKA, TEST_FILE_URI_VP9, {602, 433}, {5, 433}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0017
 * @tc.name      : create empty buffer for read, local and uri, rm
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0017, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_RM, TEST_FILE_LOCAL_RV40,
        {14204, 12730}, {508, 12730}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_RM, TEST_FILE_URI_RV40, {14204, 12730}, {508, 12730}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_RM, TEST_FILE_LOCAL_RV30_COOK,
        {2720, 1937}, {2720, 155}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_RM, TEST_FILE_URI_RV30_COOK, {2720, 1937}, {2720, 155}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0018
 * @tc.name      : create empty buffer for read, local and uri, dts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0018, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_DTS, TEST_FILE_LOCAL_DTS, {469}, {469}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_DTS, TEST_FILE_URI_DTS, {469}, {469}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0019
 * @tc.name      : create empty buffer for read, local and uri, m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0019, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_LOCAL_PCMBLURAY, {400}, {400}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_PCMBLURAY, {400}, {400}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0020
 * @tc.name      : create empty buffer for read, local and uri, ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0020, TestSize.Level2)
{
#ifdef SUPPORT_DEMUXER_VC1
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_LOCAL_VC1, {60}, {1}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_VC1, {60}, {1}}));
#endif
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0021
 * @tc.name      : create empty buffer for read, local and uri, flac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0021, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_LOCAL_FLAC, {3050}, {3050}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_FLAC, TEST_FILE_URI_FLAC, {3050}, {3050}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0022
 * @tc.name      : create empty buffer for read, local and uri, aac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0022, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_LOCAL_AAC, {1717}, {1717}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AAC, TEST_FILE_URI_AAC, {1717}, {1717}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0023
 * @tc.name      : create empty buffer for read, local and uri, mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0023, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_LOCAL_MP3, {3052}, {3052}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MP3, TEST_FILE_URI_MP3, {3052}, {3052}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0024
 * @tc.name      : create empty buffer for read, local and uri, amr
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0024, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_LOCAL_AMR, {1501}, {1501}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_AMR, TEST_FILE_URI_AMR, {1501}, {1501}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0025
 * @tc.name      : create empty buffer for read, local and uri, srt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0025, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_LOCAL_SRT, {10}, {10}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_SRT, TEST_FILE_URI_SRT, {10}, {10}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0026
 * @tc.name      : create empty buffer for read, local and uri, vtt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0026, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_LOCAL_VTT, {8}, {8}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_WEBVTT, TEST_FILE_URI_VTT, {8}, {8}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0027
 * @tc.name      : create empty buffer for read, local and uri, m4v
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0027, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_M4V, {602, 109}, {3, 109}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_M4V, {602, 109}, {3, 109}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0028
 * @tc.name      : create empty buffer for read, local and uri, 3gp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0028, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_LOCAL_3GP, {2331}, {2331}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MOV_S, TEST_FILE_URI_3GP, {2331}, {2331}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0029
 * @tc.name      : create empty buffer for read, local and uri, mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0029, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_LOCAL_MTS, {303, 434}, {26, 434}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_MTS, {303, 434}, {26, 434}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0030
 * @tc.name      : create empty buffer for read, local and uri, trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0030, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_LOCAL_TRP, {303, 387}, {26, 387}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_MPEGTS, TEST_FILE_URI_TRP, {303, 387}, {26, 387}}));
}

/**
 * @tc.number    : DEMUXER_ZEROCOPY_INNER_FUNC_0031
 * @tc.name      : create empty buffer for read, local and uri, rmvb
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerZerocopyInnerFuncTest, DEMUXER_ZEROCOPY_INNER_FUNC_0031, TestSize.Level2)
{
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_RM, TEST_FILE_LOCAL_RMVB, {251, 480}, {2, 480}}));
    ASSERT_TRUE(PluginCheckDemuxerResult({DEMUXER_PLUGIN_NAME_RM, TEST_FILE_URI_RMVB, {251, 480}, {2, 480}}));
}
}  // namespace Media
}  // namespace OHOS