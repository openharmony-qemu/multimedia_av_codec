/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <cmath>
#include <thread>
#include <random>
namespace OHOS {
namespace Media {
class DemuxerRandomSeekNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};

static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
constexpr int32_t THOUSAND = 1000.0;
constexpr int32_t SEEKTIMES = 10;
void DemuxerRandomSeekNdkTest::SetUpTestCase() {}
void DemuxerRandomSeekNdkTest::TearDownTestCase() {}
void DemuxerRandomSeekNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerRandomSeekNdkTest::TearDown()
{
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }

    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }

    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (avBuffer != nullptr) {
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
}
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
std::random_device rd;

static int64_t GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

static void CheckSeekResult(const char *fileName, uint32_t seekCount)
{
    int64_t duration = 0;
    OH_AVCodecBufferAttr attr;
    int fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    cout << fileName << "-------" << fd << "-------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    cout << "duration----" << duration << endl;
    srand(time(NULL));
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        for (int32_t i = 0; i < seekCount; i++) {
            if (duration != 0) {
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, (rd() % duration) / THOUSAND,
                (OH_AVSeekMode)((rd() % 1) +1)));
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
        }
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0001
 * @tc.name      : demuxer random seek, 01_video_audio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0001, TestSize.Level0)
{
    CheckSeekResult("/data/test/media/01_video_audio.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0002
 * @tc.name      : demuxer random seek, avc_mp3.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0002, TestSize.Level0)
{
    CheckSeekResult("/data/test/media/avc_mp3.flv", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0003
 * @tc.name      : demuxer random seek, avc_mp3_error.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0003, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/avc_mp3_error.flv", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0004
 * @tc.name      : demuxer random seek, avcc_10sec.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0004, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/avcc_10sec.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0005
 * @tc.name      : demuxer random seek, demuxer_parser_2_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0005, TestSize.Level0)
{
    CheckSeekResult("/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0006
 * @tc.name      : demuxer random seek, demuxer_parser_2_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0006, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0007
 * @tc.name      : demuxer random seek, demuxer_parser_3_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0007, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0008
 * @tc.name      : demuxer random seek, demuxer_parser_3_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0008, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0009
 * @tc.name      : demuxer random seek, demuxer_parser_4_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0009, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0010
 * @tc.name      : demuxer random seek, demuxer_parser_4_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0010, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0011
 * @tc.name      : demuxer random seek, demuxer_parser_all_i_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0011, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0012
 * @tc.name      : demuxer random seek, demuxer_parser_all_i_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0012, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0013
 * @tc.name      : demuxer random seek, demuxer_parser_ipb_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0013, TestSize.Level0)
{
    CheckSeekResult("/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0014
 * @tc.name      : demuxer random seek, demuxer_parser_ipb_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0014, TestSize.Level0)
{
    CheckSeekResult("/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0015
 * @tc.name      : demuxer random seek, demuxer_parser_ltr_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0015, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0016
 * @tc.name      : demuxer random seek, demuxer_parser_ltr_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0016, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0017
 * @tc.name      : demuxer random seek, demuxer_parser_one_i_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0017, TestSize.Level0)
{
    CheckSeekResult("/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0018
 * @tc.name      : demuxer random seek, demuxer_parser_one_i_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0018, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0019
 * @tc.name      : demuxer random seek, demuxer_parser_one_i_frame_no_audio_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0019, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0020
 * @tc.name      : demuxer random seek, demuxer_parser_one_i_frame_no_audio_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0020, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0021
 * @tc.name      : demuxer random seek, demuxer_parser_sdtp_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0021, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0022
 * @tc.name      : demuxer random seek, demuxer_parser_sdtp_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0022, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0023
 * @tc.name      : demuxer random seek, double_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0023, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/double_hevc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0024
 * @tc.name      : demuxer random seek, double_vivid.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0024, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/double_vivid.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0026
 * @tc.name      : demuxer random seek, h264_aac_640.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0026, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h264_aac_640.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0027
 * @tc.name      : demuxer random seek, h264_aac_1280.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0027, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h264_aac_1280.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0028
 * @tc.name      : demuxer random seek, h264_aac_1920.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0028, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h264_aac_1920.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0029
 * @tc.name      : demuxer random seek, h264_mp3_3mevx_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0029, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0030
 * @tc.name      : demuxer random seek, h265_aac_1mvex_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0030, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0032
 * @tc.name      : demuxer random seek, h265_mp3_640.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0032, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h265_mp3_640.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0033
 * @tc.name      : demuxer random seek, h265_mp3_1280.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0033, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h265_mp3_1280.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0034
 * @tc.name      : demuxer random seek, h265_mp3_1920.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0034, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h265_mp3_1920.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0035
 * @tc.name      : demuxer random seek, hevc_pcm_a.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0035, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/hevc_pcm_a.flv", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0036
 * @tc.name      : demuxer random seek, hevc_v.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0036, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/hevc_v.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0037
 * @tc.name      : demuxer random seek, hevc_v_a.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0037, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/hevc_v_a.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0038
 * @tc.name      : demuxer random seek, hvcc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0038, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/hvcc.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0039
 * @tc.name      : demuxer random seek, m4a_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0039, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/m4a_fmp4.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0040
 * @tc.name      : demuxer random seek, m4v_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0040, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/m4v_fmp4.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0041
 * @tc.name      : demuxer random seek, mkv.mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0041, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/mkv.mkv", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0042
 * @tc.name      : demuxer random seek, MP3_avcc_10sec.bin
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0042, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MP3_avcc_10sec.bin", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0043
 * @tc.name      : demuxer random seek, MP3_OGG_48000_1.bin
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0043, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MP3_OGG_48000_1.bin", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0044
 * @tc.name      : demuxer random seek, mpeg2.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0044, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/mpeg2.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0045
 * @tc.name      : demuxer random seek, noPermission.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0045, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/noPermission.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0046
 * @tc.name      : demuxer random seek, NoTimedmetadataAudio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0046, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/NoTimedmetadataAudio.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0047
 * @tc.name      : demuxer random seek, single_60.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0047, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/single_60.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0048
 * @tc.name      : demuxer random seek, single_rk.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0048, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/single_rk.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0054
 * @tc.name      : demuxer random seek, test_264_B_Gop25_4sec.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0054, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/test_264_B_Gop25_4sec.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0055
 * @tc.name      : demuxer random seek, test_265_B_Gop25_4sec.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0055, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/test_265_B_Gop25_4sec.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0056
 * @tc.name      : demuxer random seek, test_video_avcc_10sec.bin
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0056, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/test_video_avcc_10sec.bin", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0057
 * @tc.name      : demuxer random seek, Timedmetadata1Track0.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0057, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/Timedmetadata1Track0.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0058
 * @tc.name      : demuxer random seek, Timedmetadata1Track1.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0058, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/Timedmetadata1Track1.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0059
 * @tc.name      : demuxer random seek, Timedmetadata1Track2.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0059, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/Timedmetadata1Track2.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0060
 * @tc.name      : demuxer random seek, Timedmetadata2Track2.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0060, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/Timedmetadata2Track2.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0061
 * @tc.name      : demuxer random seek, TimedmetadataAudio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0061, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/TimedmetadataAudio.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0062
 * @tc.name      : demuxer random seek, TimedmetadataVideo.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0062, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/TimedmetadataVideo.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0063
 * @tc.name      : demuxer random seek, ts_video.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0063, TestSize.Level1)
{
    CheckSeekResult("/data/test/media/ts_video.ts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0064
 * @tc.name      : demuxer random seek, video_2audio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0064, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/video_2audio.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0065
 * @tc.name      : demuxer random seek, video_9audio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0065, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/video_9audio.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0071
 * @tc.name      : demuxer random seek, xm.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0071, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/xm.mp4", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0072
 * @tc.name      : demuxer random seek, AAC_48000_1.aac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0072, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/AAC_48000_1.aac", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0073
 * @tc.name      : demuxer random seek, amr_nb_8000_1.amr
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0073, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/amr_nb_8000_1.amr", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0074
 * @tc.name      : demuxer random seek, amr_wb_16000_1.amr
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0074, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/amr_wb_16000_1.amr", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0075
 * @tc.name      : demuxer random seek, ape.ape
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0075, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/ape.ape", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0076
 * @tc.name      : demuxer random seek, feff_bom.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0076, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/feff_bom.mp3", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0077
 * @tc.name      : demuxer random seek, fffe_bom.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0077, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/fffe_bom.mp3", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0078
 * @tc.name      : demuxer random seek, FLAC_48000_1.flac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0078, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/FLAC_48000_1.flac", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0079
 * @tc.name      : demuxer random seek, M4A_48000_1.m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0079, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/M4A_48000_1.m4a", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0080
 * @tc.name      : demuxer random seek, MP3_48000_1.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0080, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/MP3_48000_1.mp3", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0081
 * @tc.name      : demuxer random seek, nonstandard_bom.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0081, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/nonstandard_bom.mp3", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0082
 * @tc.name      : demuxer random seek, OGG_48000_1.ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0082, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/OGG_48000_1.ogg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0083
 * @tc.name      : demuxer random seek, wav_48000_1.wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0083, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/wav_48000_1.wav", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0084
 * @tc.name      : demuxer random seek, wav_audio_test_1562.wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0084, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/wav_audio_test_1562.wav", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0085
 * @tc.name      : demuxer random seek, wav_audio_test_202406290859.wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0085, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/audio/wav_audio_test_202406290859.wav", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0086
 * @tc.name      : demuxer random seek, H264_base@5_1920_1080_30_AAC_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0086, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0087
 * @tc.name      : demuxer random seek, H264_main@4_1280_720_60_MP2_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0087, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0088
 * @tc.name      : demuxer random seek, H264_high@5.1_3840_2160_30_MP3_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0088, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0089
 * @tc.name      : demuxer random seek, H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0089, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0090
 * @tc.name      : demuxer random seek, H264_main@3_720_480_30_PCM_48K_24_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0090, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0091
 * @tc.name      : demuxer random seek, H265_main@4_1920_1080_30_AAC_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0091, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0092
 * @tc.name      : demuxer random seek, H265_main10@4.1_1280_720_60_MP2_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0092, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0093
 * @tc.name      : demuxer random seek, H265_main10@5_1920_1080_60_MP3_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0093, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0094
 * @tc.name      : demuxer random seek, H265_main@5_3840_2160_30_vorbis_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0094, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0095
 * @tc.name      : demuxer random seek, H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0095, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0096
 * @tc.name      : demuxer random seek, MPEG4_SP@5_720_480_30_AAC_32K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0096, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0097
 * @tc.name      : demuxer random seek, MPEG4_SP@6_1280_720_30_MP2_32K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0097, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0098
 * @tc.name      : demuxer random seek, MPEG4_ASP@3_352_288_30_MP3_32K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0098, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0099
 * @tc.name      : demuxer random seek, MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0099, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0100
 * @tc.name      : demuxer random seek, MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0100, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0101
 * @tc.name      : demuxer random seek, H264_base@5_1920_1080_30_MP2_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0101, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0102
 * @tc.name      : demuxer random seek, H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0102, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0103
 * @tc.name      : demuxer random seek, H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0103, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0104
 * @tc.name      : demuxer random seek, H264_main@4.2_1280_720_60_MP3_32K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0104, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0105
 * @tc.name      : demuxer random seek, MPEG2_422p_1280_720_60_MP3_32K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0105, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0106
 * @tc.name      : demuxer random seek, MPEG2_high_720_480_30_MP2_32K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0106, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0107
 * @tc.name      : demuxer random seek, MPEG2_main_352_288_30_MP2_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0107, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0108
 * @tc.name      : demuxer random seek, MPEG2_main_1920_1080_30_MP3_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0108, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_RANDOM_SEEK_0109
 * @tc.name      : demuxer random seek, MPEG2_simple_320_240_24_MP3_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_RANDOM_SEEK_0109, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0100
 * @tc.name      : demuxer random seek,AVI_H263_baseline@level10_352_288_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0100, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H263_baseline@level10_352_288_AAC_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0200
 * @tc.name      : demuxer random seek, AVI_H263_baseline@level20_352_288_MP2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0200, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H263_baseline@level20_352_288_MP2_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0300
 * @tc.name      : demuxer random seek, AVI_H263_baseline@level60_704_576_MP3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0300, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H263_baseline@level60_704_576_MP3_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0400
 * @tc.name      : demuxer random seek, AVI_H263_baseline@level70_1408_1152_PCM_mulaw_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0400, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H263_baseline@level70_1408_1152_PCM_mulaw_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0500
 * @tc.name      : demuxer random seek, AVI_H264_baseline@level4.1_1280_720_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0500, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H264_constrained_baseline@level4.1_1280_720_AAC_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0600
 * @tc.name      : demuxer random seek, AVI_H264_main@level5_1920_1080_PM2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0600, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H264_main@level5_1920_1080_MP2_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0700
 * @tc.name      : demuxer random seek, AVI_H264_high@level5.1_1920_1080_PM3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0700, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H264_high@level5.1_1920_1080_MP3_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0800
 * @tc.name      : demuxer random seek, AVI_H264_h422@level5.1_2560_1920_PCM_mulaw_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0800, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_H264_high@level5_2560_1920_PCM_mulaw_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_0900
 * @tc.name      : demuxer random seek, AVI_MPEG2_simple@low_320_240_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_0900, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG2_simple@low_320_240_AAC_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_1000
 * @tc.name      : demuxer random seek, AVI_MPEG2_main@mian_720_480_MP2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_1000, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG2_main@mian_640_480_MP2_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_1100
 * @tc.name      : demuxer random seek, AVI_MPEG2_high@high_1440_1080_MP3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_1100, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG2_simple@main_640_480_MP3_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_1200
 * @tc.name      : demuxer random seek, AVI_MPEG2_422P@high_1920_1080_PCM_s24_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_1200, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG2_422P@high_1920_1080_PCM_s24_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_1300
 * @tc.name      : demuxer random seek, AVI_MPEG4_simple@level5_720_480_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_1300, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG4_simple@level1_640_480_AAC_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_1400
 * @tc.name      : demuxer random seek, AVI_MPEG4_advanced_simple@level6_1280_720_MP2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_1400, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG4_advanced_simple@level6_1280_720_MP2_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_1500
 * @tc.name      : demuxer random seek, AVI_MPEG4_advanced_simple@level3_352_288_MP3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_1500, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG4_advanced_simple@level3_352_288_MP3_2.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_AVI_RANDOM_SEEK_1600
 * @tc.name      : demuxer random seek, AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_AVI_RANDOM_SEEK_1600, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_ALAC_RANDOM_SEEK_1700
 * @tc.name      : demuxer random seek, ALAC_16bit_44100Hz.m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_ALAC_RANDOM_SEEK_1700, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/ALAC_16bit_44100Hz.m4a", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_ALAC_RANDOM_SEEK_1800
 * @tc.name      : demuxer random seek, ALAC_24bit_48000Hz.m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_ALAC_RANDOM_SEEK_1800, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/ALAC_24bit_48000Hz.m4a", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_ALAC_RANDOM_SEEK_1900
 * @tc.name      : demuxer random seek, ALAC_32bit_96000Hz.m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRandomSeekNdkTest, DEMUXER_ALAC_RANDOM_SEEK_1900, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/ALAC_32bit_96000Hz.m4a", SEEKTIMES);
}