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
#include "meta/meta.h"
#include "meta/meta_key.h"
#include "meta/video_types.h"
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
// M4A, AAC, MP3, OGG, FLAC, WAV, AMR, APE
string g_m4aPath1 = string("/data/test/media/audio/h264_fmp4.m4a");
string g_accPath1 = string("/data/test/media/audio/aac_44100_1.aac");
string g_mp3Path1 = string("/data/test/media/audio/mp3_48000_1_cover.mp3");
string g_oggPath1 = string("/data/test/media/audio/ogg_48000_1_ut.ogg");
string g_flacPath1 = string("/data/test/media/audio/flac_48000_1_cover.flac");
string g_wavPath1 = string("/data/test/media/audio/wav_48000_1_ut.wav");
string g_wavPath2 = string("/data/test/media/audio/wav_48000_1_pcm_alaw.wav");
string g_amrPath1 = string("/data/test/media/audio/amr_nb_8000_1.amr");
string g_amrPath2 = string("/data/test/media/audio/amr_wb_16000_1.amr");
string g_apePath1 = string("/data/test/media/ape_test.ape");
// DSF
string g_dsfPath = string("/data/test/media/DSD128.dsf");

/**
 * @tc.name: Demuxer_ReadSample_M4A_0001
 * @tc.desc: Copy current sample to buffer (M4A)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_M4A_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_m4aPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(433);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({433}, {433}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_AAC_0001
 * @tc.desc: Copy current sample to buffer (AAC)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AAC_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_aac";
    std::string filePath = g_accPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1293);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1293}, {1293}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_MP3_0001
 * @tc.desc: Copy current sample to buffer (MP3)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_MP3_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mp3";
    std::string filePath = g_mp3Path1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1251);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1251, 0}, {1251, 0}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_OGG_0001
 * @tc.desc: Copy current sample to buffer (OGG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_OGG_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_ogg";
    std::string filePath = g_oggPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1598);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1598}, {1598}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_FLAC_0001
 * @tc.desc: Copy current sample to buffer (FLAC)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_FLAC_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flac";
    std::string filePath = g_flacPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(313);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({313, 0}, {313, 0}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WAV_0001
 * @tc.desc: Copy current sample to buffer (WAV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WAV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_wav";
    std::string filePath = g_wavPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(704);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({704}, {704}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WAV_0002
 * @tc.desc: Copy current sample to buffer (WAV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WAV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_wav";
    std::string filePath = g_wavPath2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(352);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({352}, {352}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_AMR_0001
 * @tc.desc: Copy current sample to buffer (AMR)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AMR_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_amr";
    std::string filePath = g_amrPath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1501);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1501}, {1501}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_AMR_0002
 * @tc.desc: Copy current sample to buffer (AMR)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_AMR_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_amr";
    std::string filePath = g_amrPath2;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1500);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1500}, {1500}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_APE_0001
 * @tc.desc: Copy current sample to buffer (APE)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_APE_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_ape";
    std::string filePath = g_apePath1;
    InitResource(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(7);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({7}, {7}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_M4A_0001
 * @tc.desc: Copy current sample to buffer (M4A)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_M4A_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_m4aPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(433);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({433}, {433}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AAC_0001
 * @tc.desc: Copy current sample to buffer (AAC)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AAC_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_aac";
    std::string filePath = g_accPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1293);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1293}, {1293}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_MP3_0001
 * @tc.desc: Copy current sample to buffer (MP3)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_MP3_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mp3";
    std::string filePath = g_mp3Path1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1251);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1251, 0}, {1251, 0}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_OGG_0001
 * @tc.desc: Copy current sample to buffer (OGG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_OGG_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_ogg";
    std::string filePath = g_oggPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1598);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1598}, {1598}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_FLAC_0001
 * @tc.desc: Copy current sample to buffer (FLAC)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_FLAC_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flac";
    std::string filePath = g_flacPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(313);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({313, 0}, {313, 0}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_WAV_0001
 * @tc.desc: Copy current sample to buffer (WAV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_WAV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_wav";
    std::string filePath = g_wavPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(704);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({704}, {704}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_WAV_0002
 * @tc.desc: Copy current sample to buffer (WAV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_WAV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_wav";
    std::string filePath = g_wavPath2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(352);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({352}, {352}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AMR_0001
 * @tc.desc: Copy current sample to buffer (AMR)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AMR_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_amr";
    std::string filePath = g_amrPath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1501);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1501}, {1501}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_AMR_0002
 * @tc.desc: Copy current sample to buffer (AMR)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_AMR_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_amr";
    std::string filePath = g_amrPath2;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1500);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1500}, {1500}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_WeakNetwork_APE_0001
 * @tc.desc: Copy current sample to buffer (APE)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_WeakNetwork_APE_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_ape";
    std::string filePath = g_apePath1;
    InitWeakNetworkDemuxerPlugin(filePath, pluginName, 2560656, 3);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(7);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({7}, {7}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_M4A_0001
 * @tc.desc: Copy current sample to buffer (M4A)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_M4A_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mov,mp4,m4a,3gp,3g2,mj2";
    std::string filePath = g_m4aPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(433);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({433}, {433}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AAC_0001
 * @tc.desc: Copy current sample to buffer (AAC)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AAC_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_aac";
    std::string filePath = g_accPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1293);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1293}, {1293}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_MP3_0001
 * @tc.desc: Copy current sample to buffer (MP3)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_MP3_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_mp3";
    std::string filePath = g_mp3Path1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1251);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1251, 0}, {1251, 0}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_OGG_0001
 * @tc.desc: Copy current sample to buffer (OGG)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_OGG_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_ogg";
    std::string filePath = g_oggPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1598);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1598}, {1598}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_FLAC_0001
 * @tc.desc: Copy current sample to buffer (FLAC)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_FLAC_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_flac";
    std::string filePath = g_flacPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(313);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({313, 0}, {313, 0}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_WAV_0001
 * @tc.desc: Copy current sample to buffer (WAV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_WAV_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_wav";
    std::string filePath = g_wavPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(704);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({704}, {704}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_WAV_0002
 * @tc.desc: Copy current sample to buffer (WAV)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_WAV_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_wav";
    std::string filePath = g_wavPath2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(352);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({352}, {352}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AMR_0001
 * @tc.desc: Copy current sample to buffer (AMR)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AMR_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_amr";
    std::string filePath = g_amrPath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1501);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1501}, {1501}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_AMR_0002
 * @tc.desc: Copy current sample to buffer (AMR)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_AMR_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_amr";
    std::string filePath = g_amrPath2;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(1500);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({1500}, {1500}, numbers);
}

/**
 * @tc.name: Demuxer_ReadSample_URI_APE_0001
 * @tc.desc: Copy current sample to buffer (APE)
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_URI_APE_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_ape";
    std::string filePath = g_apePath1;
    InitResourceURI(filePath, pluginName);
    ASSERT_TRUE(initStatus_);
    std::vector<uint32_t> numbers(7);
    std::iota(numbers.begin(), numbers.end(), 0);
    CheckAllFrames({7}, {7}, numbers);
}

static Meta *FindAudioTrack(MediaInfo &mediaInfo)
{
    for (uint32_t i = 0; i < mediaInfo.tracks.size(); ++i) {
        Plugins::MediaType trackType = Plugins::MediaType::UNKNOWN;
        if (mediaInfo.tracks[i].GetData(Tag::MEDIA_TYPE, trackType) &&
            trackType == Plugins::MediaType::AUDIO) {
            return &mediaInfo.tracks[i];
        }
    }
    return nullptr;
}

/**
 * @tc.name: Demuxer_GetMediaInfo_DSF_0001
 * @tc.desc: Open DSF file and verify basic track metadata
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetMediaInfo_DSF_0001, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_GE(mediaInfo_.tracks.size(), 1);
    Meta *format = FindAudioTrack(mediaInfo_);
    ASSERT_NE(format, nullptr);

    std::string mimeType;
    EXPECT_TRUE(format->GetData(Tag::MIME_TYPE, mimeType));
    EXPECT_EQ(mimeType, "audio/dsd");

    int32_t sampleRate = 0;
    EXPECT_TRUE(format->GetData(Tag::AUDIO_SAMPLE_RATE, sampleRate));
    EXPECT_EQ(sampleRate, 352800);

    int32_t channelCount = 0;
    EXPECT_TRUE(format->GetData(Tag::AUDIO_CHANNEL_COUNT, channelCount));
    EXPECT_EQ(channelCount, 2);
}

/**
 * @tc.name: Demuxer_GetMediaInfo_DSF_FileType_0002
 * @tc.desc: Verify DSF file type on general meta
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetMediaInfo_DSF_FileType_0002, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);

    Plugins::FileType fileType = Plugins::FileType::UNKNOW;
    EXPECT_TRUE(mediaInfo_.general.GetData(Tag::MEDIA_FILE_TYPE, fileType));
    EXPECT_EQ(fileType, Plugins::FileType::DSF);
}

/**
 * @tc.name: Demuxer_GetMediaInfo_DSF_DsdCodecType_0003
 * @tc.desc: Verify DSD codec type and original codec name
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetMediaInfo_DSF_DsdCodecType_0003, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    Meta *format = FindAudioTrack(mediaInfo_);
    ASSERT_NE(format, nullptr);

    int32_t dsdCodecType = -1;
    EXPECT_TRUE(format->GetData(Tag::AUDIO_DSD_CODEC_TYPE, dsdCodecType));
    EXPECT_EQ(dsdCodecType, 0);

    std::string codecName;
    EXPECT_TRUE(format->GetData(Tag::ORIGINAL_CODEC_NAME, codecName));
    EXPECT_EQ(codecName, "dsd_lsbf_planar");
}

/**
 * @tc.name: Demuxer_ReadSample_DSF_0004
 * @tc.desc: Read samples from DSF file and verify EOS
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ReadSample_DSF_0004, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_GE(mediaInfo_.tracks.size(), 1);
    ASSERT_EQ(demuxerPlugin_->SelectTrack(0), Status::OK);
    ReadData();
    EXPECT_TRUE(eosFlag_[0]);
    EXPECT_GT(frames_[0], 0);
}

/**
 * @tc.name: Demuxer_GetMediaInfo_DSF_TrackType_0005
 * @tc.desc: Verify track media type is AUDIO and track count is 1
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetMediaInfo_DSF_TrackType_0005, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    ASSERT_EQ(mediaInfo_.tracks.size(), 1u);

    Plugins::MediaType trackType = Plugins::MediaType::UNKNOWN;
    EXPECT_TRUE(mediaInfo_.tracks[0].GetData(Tag::MEDIA_TYPE, trackType));
    EXPECT_EQ(trackType, Plugins::MediaType::AUDIO);
}

/**
 * @tc.name: Demuxer_GetMediaInfo_DSF_Duration_0006
 * @tc.desc: Verify media duration is set on general meta
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetMediaInfo_DSF_Duration_0006, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);

    int64_t duration = 0;
    EXPECT_TRUE(mediaInfo_.general.GetData(Tag::MEDIA_DURATION, duration));
    EXPECT_GT(duration, 0);
}

/**
 * @tc.name: Demuxer_GetMediaInfo_IFF_PluginRegistered_0007
 * @tc.desc: Verify IFF (DFF) demuxer plugin is registered in PluginList
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_GetMediaInfo_IFF_PluginRegistered_0007, TestSize.Level1)
{
    auto plugin = PluginManagerV2::Instance().CreatePluginByName("avdemux_iff");
    EXPECT_NE(plugin, nullptr);
}

/**
 * @tc.name: Demuxer_SeekToTime_DSF_0008
 * @tc.desc: Seek to various positions with different seek modes
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SeekToTime_DSF_0008, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    std::vector<int64_t> seekPositions = {0, 500, 1500, 2500, 3000};
    std::vector<SeekMode> modes = {SeekMode::SEEK_PREVIOUS_SYNC, SeekMode::SEEK_NEXT_SYNC,
        SeekMode::SEEK_CLOSEST_SYNC};
    for (auto seekMs : seekPositions) {
        for (auto mode : modes) {
            int64_t realSeekTime = -1;
            Status ret = demuxerPlugin_->SeekTo(-1, seekMs, mode, realSeekTime);
            EXPECT_EQ(ret, Status::OK);
            EXPECT_GE(realSeekTime, 0);
        }
    }
}

/**
 * @tc.name: Demuxer_SeekAndRead_DSF_0009
 * @tc.desc: Seek to middle of DSF file, then read frames until EOS
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SeekAndRead_DSF_0009, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    int64_t realSeekTime = -1;
    Status ret = demuxerPlugin_->SeekTo(-1, 1500, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    ASSERT_EQ(ret, Status::OK);
    EXPECT_GE(realSeekTime, 0);
    ReadData();
    EXPECT_TRUE(eosFlag_[0]);
    EXPECT_GT(frames_[0], 0);
}

/**
 * @tc.name: Demuxer_SeekAndRead_DSF_0010
 * @tc.desc: Seek to beginning of DSF file, then read frames until EOS
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SeekAndRead_DSF_0010, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    int64_t realSeekTime = -1;
    Status ret = demuxerPlugin_->SeekTo(-1, 0, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    ASSERT_EQ(ret, Status::OK);
    ReadData();
    EXPECT_TRUE(eosFlag_[0]);
    EXPECT_GT(frames_[0], 0);
}

/**
 * @tc.name: Demuxer_SeekAndRead_DSF_0011
 * @tc.desc: Seek to near end of DSF file, then read frames, verify fewer frames than from beginning
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_SeekAndRead_DSF_0011, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    int64_t realSeekTime = -1;
    Status ret = demuxerPlugin_->SeekTo(-1, 3000, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    ASSERT_EQ(ret, Status::OK);
    ReadData();
    int32_t framesFromEnd = frames_[0];
    EXPECT_GT(framesFromEnd, 0);

    RemoveValue();
    ret = demuxerPlugin_->SeekTo(-1, 0, SeekMode::SEEK_PREVIOUS_SYNC, realSeekTime);
    ASSERT_EQ(ret, Status::OK);
    ReadData();
    int32_t framesFromStart = frames_[0];
    EXPECT_GT(framesFromStart, framesFromEnd);
}

/**
 * @tc.name: Demuxer_ErrorSeek_DSF_0012
 * @tc.desc: Seek to invalid positions, expect failure
 * @tc.type: FUNC
 */
HWTEST_F(DemuxerPluginUnitTest, Demuxer_ErrorSeek_DSF_0012, TestSize.Level1)
{
    std::string pluginName = "avdemux_dsf";
    InitResource(g_dsfPath, pluginName);
    ASSERT_TRUE(initStatus_);
    for (uint32_t idx = 0; idx < mediaInfo_.tracks.size(); ++idx) {
        ASSERT_EQ(demuxerPlugin_->SelectTrack(idx), Status::OK);
    }
    int64_t realSeekTime = 0;
    EXPECT_NE(demuxerPlugin_->SeekTo(-1, -1000, SeekMode::SEEK_NEXT_SYNC, realSeekTime), Status::OK);
    EXPECT_NE(demuxerPlugin_->SeekTo(-1, INT64_MAX, SeekMode::SEEK_NEXT_SYNC, realSeekTime), Status::OK);
}