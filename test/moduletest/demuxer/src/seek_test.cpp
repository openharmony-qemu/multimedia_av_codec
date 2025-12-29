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
namespace OHOS {
namespace Media {
class DemuxerSeekNdkTest : public testing::Test {
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
void DemuxerSeekNdkTest::SetUpTestCase() {}
void DemuxerSeekNdkTest::TearDownTestCase() {}
void DemuxerSeekNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerSeekNdkTest::TearDown()
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

struct seekInfo {
    const char *fileName;
    OH_AVSeekMode seekmode;
    int64_t millisecond;
    int32_t videoCount;
    int32_t audioCount;
    int32_t subtitleCount;
};

static void SeekDestroy()
{
    OH_AVSource_Destroy(source);
    source = nullptr;
    OH_AVDemuxer_Destroy(demuxer);
    demuxer = nullptr;
    OH_AVFormat_Destroy(sourceFormat);
    sourceFormat = nullptr;
}

static void CheckSeekMode(seekInfo seekInfo)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    int fd = open(seekInfo.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo.fileName);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
        bool readEnd = false;
        int32_t frameNum = 0;
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                readEnd = true;
                break;
            }
            frameNum++;
        }
        if (tarckType == MEDIA_TYPE_VID) {
            ASSERT_EQ(seekInfo.videoCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_AUD) {
            OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType);
            if (strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_FLAC) == 0) {
                ASSERT_GE(frameNum, seekInfo.audioCount - 1);
                ASSERT_LE(frameNum, seekInfo.audioCount + 1);
            } else {
                ASSERT_EQ(seekInfo.audioCount, frameNum);
            }
        } else if (tarckType == MEDIA_TYPE_SUBTITLE) {
            ASSERT_EQ(seekInfo.subtitleCount, frameNum);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
    SeekDestroy();
    close(fd);
}

/**
 * @tc.number    : DEMUXER_SEEK_0001
 * @tc.name      : demuxer seek, 01_video_audio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0001, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/01_video_audio.mp4", SEEK_MODE_NEXT_SYNC, 0, 250, 384, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/01_video_audio.mp4", SEEK_MODE_NEXT_SYNC, 5000000, 125, 192, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/01_video_audio.mp4", SEEK_MODE_NEXT_SYNC, 9960000, 1, 2, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/01_video_audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 250, 384, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/01_video_audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 5000000, 125, 193, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/01_video_audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 9960000, 1, 3, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/01_video_audio.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 250, 384, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/01_video_audio.mp4", SEEK_MODE_CLOSEST_SYNC, 5000000, 125, 192, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/01_video_audio.mp4", SEEK_MODE_CLOSEST_SYNC, 9960000, 1, 2, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0002
 * @tc.name      : demuxer seek, audiovivid_hdrvivid_1s_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0002, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 0, 26, 44, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 26, 44, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 599348, 26, 44, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 999348, 26, 44, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 26, 44, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 599348, 26, 44, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 999348, 26, 44, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0003
 * @tc.name      : demuxer seek, avc_mp3.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0003, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/avc_mp3.flv", SEEK_MODE_NEXT_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/avc_mp3.flv", SEEK_MODE_NEXT_SYNC, 5042000, 102, 65, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/avc_mp3.flv", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/avc_mp3.flv", SEEK_MODE_PREVIOUS_SYNC, 5042000, 352, 225, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/avc_mp3.flv", SEEK_MODE_PREVIOUS_SYNC, 9980000, 102, 65, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/avc_mp3.flv", SEEK_MODE_CLOSEST_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/avc_mp3.flv", SEEK_MODE_CLOSEST_SYNC, 5042000, 352, 225, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/avc_mp3.flv", SEEK_MODE_CLOSEST_SYNC, 9980000, 102, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0004
 * @tc.name      : demuxer seek, avc_mp3_error.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0004, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_NEXT_SYNC, 0, 601, 384, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_NEXT_SYNC, 5058000, 102, 65, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_PREVIOUS_SYNC, 0, 601, 384, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_PREVIOUS_SYNC, 5058000, 352, 225, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_PREVIOUS_SYNC, 9848000, 102, 65, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_CLOSEST_SYNC, 0, 601, 384, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_CLOSEST_SYNC, 5058000, 352, 225, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/avc_mp3_error.flv", SEEK_MODE_CLOSEST_SYNC, 9848000, 102, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0005
 * @tc.name      : demuxer seek, avcc_10sec.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0005, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_NEXT_SYNC, 0, 600, 431, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_NEXT_SYNC, 5000000, 300, 215, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 600, 431, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 5000000, 300, 216, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 9983333, 60, 44, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 600, 431, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_CLOSEST_SYNC, 5000000, 300, 215, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/avcc_10sec.mp4", SEEK_MODE_CLOSEST_SYNC, 9983333, 60, 44, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0006
 * @tc.name      : demuxer seek, demuxer_parser_2_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0006, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_NEXT_SYNC,
    5466666, 150, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_2_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0007
 * @tc.name      : demuxer seek, demuxer_parser_2_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0007, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC,
    5466666, 150, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_2_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0008
 * @tc.name      : demuxer seek, demuxer_parser_3_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0008, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_NEXT_SYNC,
    5466666, 150, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_3_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0009
 * @tc.name      : demuxer seek, demuxer_parser_3_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0009, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC,
    5466666, 150, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_3_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0010
 * @tc.name      : demuxer seek, demuxer_parser_4_layer_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0010, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_NEXT_SYNC,
    5466666, 150, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_4_layer_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0011
 * @tc.name      : demuxer seek, demuxer_parser_4_layer_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0011, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC,
    5466666, 150, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    5466666, 180, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_4_layer_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    10966666, 30, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0012
 * @tc.name      : demuxer seek, demuxer_parser_all_i_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0012, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 210, 297, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_NEXT_SYNC,
    3500000, 105, 148, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 6966666, 1, 2, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 210, 297, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    3500000, 105, 149, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6966666, 2, 4, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 210, 297, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    3500000, 105, 148, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/demuxer_parser_all_i_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 6966666, 1, 2, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0013
 * @tc.name      : demuxer seek, demuxer_parser_all_i_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0013, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 210, 297, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC,
    3500000, 105, 148, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 6966666, 1, 2, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 210, 297, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    3500000, 105, 149, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6966666, 2, 4, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 210, 297, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    3500000, 105, 148, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/demuxer_parser_all_i_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6966666, 1, 2, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0014
 * @tc.name      : demuxer seek, demuxer_parser_ipb_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0014, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 6200000, 162, 230, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6200000, 192, 274, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12366666, 12, 19, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6200000, 192, 274, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_ipb_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12366666, 12, 19, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0015
 * @tc.name      : demuxer seek, demuxer_parser_ipb_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0015, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 6323000, 172, 244, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6323000, 197, 281, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12356333, 22, 33, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6323000, 172, 244, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_ipb_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12356333, 22, 33, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0016
 * @tc.name      : demuxer seek, demuxer_parser_ltr_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0016, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 5600000, 80, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5600000, 330, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 80, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 5600000, 80, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_ltr_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 10966666, 80, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0017
 * @tc.name      : demuxer seek, demuxer_parser_ltr_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0017, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 5600000, 80, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    5600000, 330, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    10966666, 80, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 330, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 5600000, 80, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_ltr_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    10966666, 80, 0, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0018
 * @tc.name      : demuxer seek, demuxer_parser_one_i_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0018, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6200000, 372, 525, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12366666, 372, 525, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6200000, 372, 525, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_one_i_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12366666, 372, 525, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0019
 * @tc.name      : demuxer seek, demuxer_parser_one_i_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0019, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6200000, 372, 525, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12366666, 372, 525, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6200000, 372, 525, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_one_i_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12366666, 372, 525, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0020
 * @tc.name      : demuxer seek, demuxer_parser_one_i_frame_no_audio_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0020, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_NEXT_SYNC,
    0, 372, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    0, 372, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6200000, 372, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12366666, 372, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    0, 372, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6200000, 372, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_one_i_frame_no_audio_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12366666, 372, 0, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0021
 * @tc.name      : demuxer seek, demuxer_parser_one_i_frame_no_audio_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0021, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEK_MODE_NEXT_SYNC,
    0, 372, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    0, 372, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6200000, 372, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12366666, 372, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    0, 372, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6200000, 372, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_one_i_frame_no_audio_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12366666, 372, 0, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0022
 * @tc.name      : demuxer seek, demuxer_parser_sdtp_frame_avc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0022, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_NEXT_SYNC, 6200000, 122, 175, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6200000, 372, 525, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12366666, 122, 176, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6200000, 122, 175, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_sdtp_frame_avc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12366666, 122, 176, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0023
 * @tc.name      : demuxer seek, demuxer_parser_sdtp_frame_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0023, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_NEXT_SYNC,
    6300000, 126, 178, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    6300000, 372, 525, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC,
    12333333, 126, 179, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 372, 525, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    6300000, 126, 178, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/demuxer_parser_sdtp_frame_hevc.mp4", SEEK_MODE_CLOSEST_SYNC,
    12333333, 126, 179, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0024
 * @tc.name      : demuxer seek, double_hevc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0024, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/double_hevc.mp4", SEEK_MODE_NEXT_SYNC, 0, 77, 120, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/double_hevc.mp4", SEEK_MODE_NEXT_SYNC, 1266666, 17, 26, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/double_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 77, 120, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/double_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 1266666, 47, 74, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/double_hevc.mp4", SEEK_MODE_PREVIOUS_SYNC, 2533444, 17, 27, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/double_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 77, 120, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/double_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 1266666, 47, 74, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/double_hevc.mp4", SEEK_MODE_CLOSEST_SYNC, 2533444, 17, 27, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0025
 * @tc.name      : demuxer seek, double_vivid.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0025, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/double_vivid.mp4", SEEK_MODE_NEXT_SYNC, 0, 66, 103, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/double_vivid.mp4", SEEK_MODE_NEXT_SYNC, 1100000, 6, 10, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/double_vivid.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 66, 103, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/double_vivid.mp4", SEEK_MODE_PREVIOUS_SYNC, 1100000, 36, 58, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/double_vivid.mp4", SEEK_MODE_PREVIOUS_SYNC, 2166777, 6, 11, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/double_vivid.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 66, 103, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/double_vivid.mp4", SEEK_MODE_CLOSEST_SYNC, 1100000, 36, 58, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/double_vivid.mp4", SEEK_MODE_CLOSEST_SYNC, 2166777, 6, 11, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0027
 * @tc.name      : demuxer seek, h264_aac_640.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0027, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/h264_aac_640.ts", SEEK_MODE_NEXT_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h264_aac_640.ts", SEEK_MODE_NEXT_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h264_aac_640.ts", SEEK_MODE_NEXT_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h264_aac_640.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h264_aac_640.ts", SEEK_MODE_PREVIOUS_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h264_aac_640.ts", SEEK_MODE_PREVIOUS_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h264_aac_640.ts", SEEK_MODE_CLOSEST_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/h264_aac_640.ts", SEEK_MODE_CLOSEST_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/h264_aac_640.ts", SEEK_MODE_CLOSEST_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0028
 * @tc.name      : demuxer seek, h264_aac_1280.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0028, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_NEXT_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_NEXT_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_NEXT_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_PREVIOUS_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_PREVIOUS_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_CLOSEST_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_CLOSEST_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/h264_aac_1280.ts", SEEK_MODE_CLOSEST_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0029
 * @tc.name      : demuxer seek, h264_aac_1920.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0029, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_NEXT_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_NEXT_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_NEXT_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_PREVIOUS_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_PREVIOUS_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_CLOSEST_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_CLOSEST_SYNC, 3500000, 16, 29, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/h264_aac_1920.ts", SEEK_MODE_CLOSEST_SYNC, 4156556, 1, 8, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0030
 * @tc.name      : demuxer seek, h264_mp3_3mevx_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0030, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 0, 369, 465, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 6224375, 123, 155, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 369, 465, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 6224375, 246, 311, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 12383062, 123, 156, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 369, 465, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 6224375, 123, 155, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/h264_mp3_3mevx_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 12383062, 123, 156, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0031
 * @tc.name      : demuxer seek, h265_aac_1mvex_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0031, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 0, 242, 173, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 242, 173, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 2016312, 242, 173, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 4050312, 242, 173, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 242, 173, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 2016312, 242, 173, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h265_aac_1mvex_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 4050312, 242, 173, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0033
 * @tc.name      : demuxer seek, h265_mp3_640.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0033, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_NEXT_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_NEXT_SYNC, 3466666, 32, 22, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_NEXT_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_PREVIOUS_SYNC, 3466666, 33, 22, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_PREVIOUS_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_CLOSEST_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_CLOSEST_SYNC, 3466666, 33, 22, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/h265_mp3_640.ts", SEEK_MODE_CLOSEST_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0034
 * @tc.name      : demuxer seek, h265_mp3_1280.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0034, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_NEXT_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_NEXT_SYNC, 3466666, 32, 22, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_NEXT_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_PREVIOUS_SYNC, 3466666, 33, 22, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_PREVIOUS_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_CLOSEST_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_CLOSEST_SYNC, 3466666, 33, 22, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/h265_mp3_1280.ts", SEEK_MODE_CLOSEST_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0035
 * @tc.name      : demuxer seek, h265_mp3_1920.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0035, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_NEXT_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_NEXT_SYNC, 3466666, 32, 22, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_NEXT_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_PREVIOUS_SYNC, 3466666, 33, 22, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_PREVIOUS_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_CLOSEST_SYNC, 0, 242, 155, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_CLOSEST_SYNC, 3466666, 33, 22, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/h265_mp3_1920.ts", SEEK_MODE_CLOSEST_SYNC, 4091722, 1, 8, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0036
 * @tc.name      : demuxer seek, hevc_pcm_a.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0036, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_NEXT_SYNC, 0, 602, 385, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_NEXT_SYNC, 5034000, 102, 66, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest4{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 385, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_PREVIOUS_SYNC, 5034000, 354, 227, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_PREVIOUS_SYNC, 10034000, 102, 66, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_CLOSEST_SYNC, 0, 602, 385, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_CLOSEST_SYNC, 5034000, 354, 227, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/hevc_pcm_a.flv", SEEK_MODE_CLOSEST_SYNC, 10034000, 102, 66, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0037
 * @tc.name      : demuxer seek, hevc_v.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0037, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/hevc_v.ts", SEEK_MODE_NEXT_SYNC, 0, 602, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/hevc_v.ts", SEEK_MODE_NEXT_SYNC, 6466666, 212, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/hevc_v.ts", SEEK_MODE_NEXT_SYNC, 10033333, 1, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/hevc_v.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/hevc_v.ts", SEEK_MODE_PREVIOUS_SYNC, 6466666, 213, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/hevc_v.ts", SEEK_MODE_PREVIOUS_SYNC, 10033333, 1, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/hevc_v.ts", SEEK_MODE_CLOSEST_SYNC, 0, 602, 0, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/hevc_v.ts", SEEK_MODE_CLOSEST_SYNC, 6466666, 213, 0, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/hevc_v.ts", SEEK_MODE_CLOSEST_SYNC, 10033333, 1, 0, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0038
 * @tc.name      : demuxer seek, hevc_v_a.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0038, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/hevc_v_a.ts", SEEK_MODE_NEXT_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/hevc_v_a.ts", SEEK_MODE_NEXT_SYNC, 6466666, 212, 136, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/hevc_v_a.ts", SEEK_MODE_NEXT_SYNC, 9946799, 3, 4, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/hevc_v_a.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/hevc_v_a.ts", SEEK_MODE_PREVIOUS_SYNC, 6466666, 213, 138, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/hevc_v_a.ts", SEEK_MODE_PREVIOUS_SYNC, 9946799, 4, 4, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/hevc_v_a.ts", SEEK_MODE_CLOSEST_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/hevc_v_a.ts", SEEK_MODE_CLOSEST_SYNC, 6466666, 213, 136, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/hevc_v_a.ts", SEEK_MODE_CLOSEST_SYNC, 9946799, 4, 4, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0039
 * @tc.name      : demuxer seek, hvcc.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0039, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/hvcc.mp4", SEEK_MODE_NEXT_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/hvcc.mp4", SEEK_MODE_NEXT_SYNC, 4983333, 105, 76, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/hvcc.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/hvcc.mp4", SEEK_MODE_PREVIOUS_SYNC, 4983333, 354, 256, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/hvcc.mp4", SEEK_MODE_PREVIOUS_SYNC, 9983333, 105, 77, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/hvcc.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/hvcc.mp4", SEEK_MODE_CLOSEST_SYNC, 4983333, 354, 256, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/hvcc.mp4", SEEK_MODE_CLOSEST_SYNC, 9983333, 105, 77, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0040
 * @tc.name      : demuxer seek, m4a_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0040, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 0, 0, 352, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 15018666, 0, 176, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 29952000, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 352, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 15018666, 0, 177, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 29952000, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 0, 352, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 15018666, 0, 177, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/m4a_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 29952000, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0041
 * @tc.name      : demuxer seek, m4v_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0041, TestSize.Level1)
{
    seekInfo fileTest1{"/data/test/media/m4v_fmp4.mp4", SEEK_MODE_NEXT_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/m4v_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/m4v_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 2099687, 123, 176, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/m4v_fmp4.mp4", SEEK_MODE_PREVIOUS_SYNC, 4133687, 123, 176, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/m4v_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 123, 176, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/m4v_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 2099687, 123, 176, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/m4v_fmp4.mp4", SEEK_MODE_CLOSEST_SYNC, 4133687, 123, 176, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0042
 * @tc.name      : demuxer seek, mkv.mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0042, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/mkv.mkv", SEEK_MODE_NEXT_SYNC, 0, 600, 431, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/mkv.mkv", SEEK_MODE_NEXT_SYNC, 5000000, 300, 215, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/mkv.mkv", SEEK_MODE_PREVIOUS_SYNC, 0, 600, 431, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/mkv.mkv", SEEK_MODE_PREVIOUS_SYNC, 5000000, 300, 215, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/mkv.mkv", SEEK_MODE_PREVIOUS_SYNC, 9983000, 60, 43, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/mkv.mkv", SEEK_MODE_CLOSEST_SYNC, 0, 600, 431, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/mkv.mkv", SEEK_MODE_CLOSEST_SYNC, 5000000, 300, 215, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/mkv.mkv", SEEK_MODE_CLOSEST_SYNC, 9983000, 60, 43, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0043
 * @tc.name      : demuxer seek, MP3_avcc_10sec.bin
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0043, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_NEXT_SYNC, 0, 0, 10552, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_NEXT_SYNC, 126624000, 0, 5275, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_NEXT_SYNC, 253224000, 0, 1326, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 10552, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_PREVIOUS_SYNC, 126624000, 0, 5276, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_PREVIOUS_SYNC, 253224000, 0, 1326, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_CLOSEST_SYNC, 0, 0, 10552, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_CLOSEST_SYNC, 126624000, 0, 5276, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/MP3_avcc_10sec.bin", SEEK_MODE_CLOSEST_SYNC, 253224000, 0, 1326, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0044
 * @tc.name      : demuxer seek, MP3_OGG_48000_1.bin
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0044, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_NEXT_SYNC, 0, 0, 9420, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_NEXT_SYNC, 113040000, 0, 4709, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_NEXT_SYNC, 226056000, 0, 261, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 9420, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_PREVIOUS_SYNC, 113040000, 0, 4710, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_PREVIOUS_SYNC, 226056000, 0, 261, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_CLOSEST_SYNC, 0, 0, 9420, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_CLOSEST_SYNC, 113040000, 0, 4710, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/MP3_OGG_48000_1.bin", SEEK_MODE_CLOSEST_SYNC, 226056000, 0, 261, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0045
 * @tc.name      : demuxer seek, mpeg2.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0045, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/mpeg2.mp4", SEEK_MODE_NEXT_SYNC, 0, 303, 433, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/mpeg2.mp4", SEEK_MODE_NEXT_SYNC, 4966666, 147, 209, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest4{"/data/test/media/mpeg2.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 303, 433, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/mpeg2.mp4", SEEK_MODE_PREVIOUS_SYNC, 4966666, 159, 227, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/mpeg2.mp4", SEEK_MODE_PREVIOUS_SYNC, 10066666, 3, 3, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/mpeg2.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 303, 433, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/mpeg2.mp4", SEEK_MODE_CLOSEST_SYNC, 4966666, 159, 227, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/mpeg2.mp4", SEEK_MODE_CLOSEST_SYNC, 10066666, 3, 3, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0046
 * @tc.name      : demuxer seek, noPermission.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0046, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/noPermission.mp4", SEEK_MODE_NEXT_SYNC, 0, 250, 384, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/noPermission.mp4", SEEK_MODE_NEXT_SYNC, 5015510, 124, 191, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest4{"/data/test/media/noPermission.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 250, 384, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/noPermission.mp4", SEEK_MODE_PREVIOUS_SYNC, 5015510, 125, 193, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/noPermission.mp4", SEEK_MODE_PREVIOUS_SYNC, 10004897, 1, 3, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/noPermission.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 250, 384, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/noPermission.mp4", SEEK_MODE_CLOSEST_SYNC, 5015510, 125, 193, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/noPermission.mp4", SEEK_MODE_CLOSEST_SYNC, 10004897, 1, 3, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0047
 * @tc.name      : demuxer seek, NoTimedmetadataAudio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0047, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_NEXT_SYNC, 0, 0, 124, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_NEXT_SYNC, 2849750, 0, 2, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_NEXT_SYNC, 4266553, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 124, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_PREVIOUS_SYNC, 2849750, 0, 2, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_PREVIOUS_SYNC, 4266553, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 0, 124, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_CLOSEST_SYNC, 2849750, 0, 2, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/NoTimedmetadataAudio.mp4", SEEK_MODE_CLOSEST_SYNC, 4266553, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0048
 * @tc.name      : demuxer seek, single_60.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0048, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/single_60.mp4", SEEK_MODE_NEXT_SYNC, 0, 90, 135, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/single_60.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 90, 135, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/single_60.mp4", SEEK_MODE_PREVIOUS_SYNC, 1500122, 90, 135, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/single_60.mp4", SEEK_MODE_PREVIOUS_SYNC, 2966900, 90, 135, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/single_60.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 90, 135, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/single_60.mp4", SEEK_MODE_CLOSEST_SYNC, 1500122, 90, 135, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/single_60.mp4", SEEK_MODE_CLOSEST_SYNC, 2966900, 90, 135, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0049
 * @tc.name      : demuxer seek, single_rk.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0049, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/single_rk.mp4", SEEK_MODE_NEXT_SYNC, 0, 25, 60, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/single_rk.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 25, 60, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/single_rk.mp4", SEEK_MODE_PREVIOUS_SYNC, 1014388, 25, 60, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/single_rk.mp4", SEEK_MODE_PREVIOUS_SYNC, 2454433, 25, 60, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/single_rk.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 25, 60, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/single_rk.mp4", SEEK_MODE_CLOSEST_SYNC, 1014388, 25, 60, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/single_rk.mp4", SEEK_MODE_CLOSEST_SYNC, 2454433, 25, 60, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0050
 * @tc.name      : demuxer seek, srt_2800.srt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0050, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/srt_2800.srt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/srt_2800.srt", SEEK_MODE_NEXT_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/srt_2800.srt", SEEK_MODE_NEXT_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/srt_2800.srt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/srt_2800.srt", SEEK_MODE_PREVIOUS_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/srt_2800.srt", SEEK_MODE_PREVIOUS_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/srt_2800.srt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/srt_2800.srt", SEEK_MODE_CLOSEST_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/srt_2800.srt", SEEK_MODE_CLOSEST_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0051
 * @tc.name      : demuxer seek, srt_2900.srt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0051, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/srt_2900.srt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/srt_2900.srt", SEEK_MODE_NEXT_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/srt_2900.srt", SEEK_MODE_NEXT_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/srt_2900.srt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/srt_2900.srt", SEEK_MODE_PREVIOUS_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/srt_2900.srt", SEEK_MODE_PREVIOUS_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/srt_2900.srt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/srt_2900.srt", SEEK_MODE_CLOSEST_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/srt_2900.srt", SEEK_MODE_CLOSEST_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0052
 * @tc.name      : demuxer seek, srt_3100.srt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0052, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/srt_3100.srt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 5};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/srt_3100.srt", SEEK_MODE_NEXT_SYNC, 24900000, 0, 0, 3};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/srt_3100.srt", SEEK_MODE_NEXT_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/srt_3100.srt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 5};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/srt_3100.srt", SEEK_MODE_PREVIOUS_SYNC, 24900000, 0, 0, 3};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/srt_3100.srt", SEEK_MODE_PREVIOUS_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/srt_3100.srt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 5};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/srt_3100.srt", SEEK_MODE_CLOSEST_SYNC, 24900000, 0, 0, 3};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/srt_3100.srt", SEEK_MODE_CLOSEST_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0053
 * @tc.name      : demuxer seek, srt_3300.srt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0053, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/srt_3300.srt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/srt_3300.srt", SEEK_MODE_NEXT_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/srt_3300.srt", SEEK_MODE_NEXT_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/srt_3300.srt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/srt_3300.srt", SEEK_MODE_PREVIOUS_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/srt_3300.srt", SEEK_MODE_PREVIOUS_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/srt_3300.srt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/srt_3300.srt", SEEK_MODE_CLOSEST_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/srt_3300.srt", SEEK_MODE_CLOSEST_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0054
 * @tc.name      : demuxer seek, srt_test.srt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0054, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/srt_test.srt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/srt_test.srt", SEEK_MODE_NEXT_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/srt_test.srt", SEEK_MODE_NEXT_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/srt_test.srt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/srt_test.srt", SEEK_MODE_PREVIOUS_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/srt_test.srt", SEEK_MODE_PREVIOUS_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/srt_test.srt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 10};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/srt_test.srt", SEEK_MODE_CLOSEST_SYNC, 15900000, 0, 0, 5};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/srt_test.srt", SEEK_MODE_CLOSEST_SYNC, 31900000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0055
 * @tc.name      : demuxer seek, test_264_B_Gop25_4sec.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0055, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/test_264_B_Gop25_4sec.mp4", SEEK_MODE_NEXT_SYNC, 0, 103, 173, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/test_264_B_Gop25_4sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 103, 173, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/test_264_B_Gop25_4sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 2040000, 103, 173, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/test_264_B_Gop25_4sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 4080000, 103, 173, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/test_264_B_Gop25_4sec.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 103, 173, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/test_264_B_Gop25_4sec.mp4", SEEK_MODE_CLOSEST_SYNC, 2040000, 103, 173, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/test_264_B_Gop25_4sec.mp4", SEEK_MODE_CLOSEST_SYNC, 4080000, 103, 173, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0056
 * @tc.name      : demuxer seek, test_265_B_Gop25_4sec.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0056, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_NEXT_SYNC, 0, 103, 174, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_NEXT_SYNC, 2160000, 28, 47, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 103, 174, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 2160000, 53, 91, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_PREVIOUS_SYNC, 4040000, 3, 5, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 103, 174, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_CLOSEST_SYNC, 2160000, 53, 91, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/test_265_B_Gop25_4sec.mp4", SEEK_MODE_CLOSEST_SYNC, 4040000, 3, 5, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0058
 * @tc.name      : demuxer seek, Timedmetadata1Track0.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0058, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_NEXT_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_NEXT_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_NEXT_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_PREVIOUS_SYNC, 3500000, 105, 63, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_PREVIOUS_SYNC, 6966666, 2, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_CLOSEST_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/Timedmetadata1Track0.mp4", SEEK_MODE_CLOSEST_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0059
 * @tc.name      : demuxer seek, Timedmetadata1Track1.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0059, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_NEXT_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_NEXT_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_NEXT_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_PREVIOUS_SYNC, 3500000, 105, 63, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_PREVIOUS_SYNC, 6966666, 2, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_CLOSEST_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/Timedmetadata1Track1.mp4", SEEK_MODE_CLOSEST_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0060
 * @tc.name      : demuxer seek, Timedmetadata1Track2.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0060, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_NEXT_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_NEXT_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_NEXT_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_PREVIOUS_SYNC, 3500000, 105, 63, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_PREVIOUS_SYNC, 6966666, 2, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_CLOSEST_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/Timedmetadata1Track2.mp4", SEEK_MODE_CLOSEST_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0061
 * @tc.name      : demuxer seek, Timedmetadata2Track2.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0061, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_NEXT_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_NEXT_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_NEXT_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_PREVIOUS_SYNC, 3500000, 105, 63, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_PREVIOUS_SYNC, 6966666, 2, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 210, 211, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_CLOSEST_SYNC, 3500000, 105, 62, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/Timedmetadata2Track2.mp4", SEEK_MODE_CLOSEST_SYNC, 6966666, 1, 0, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0062
 * @tc.name      : demuxer seek, TimedmetadataAudio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0062, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_NEXT_SYNC, 0, 0, 124, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_NEXT_SYNC, 2849750, 0, 2, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_NEXT_SYNC, 4266553, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 124, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_PREVIOUS_SYNC, 2849750, 0, 2, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_PREVIOUS_SYNC, 4266553, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 0, 124, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_CLOSEST_SYNC, 2849750, 0, 2, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/TimedmetadataAudio.mp4", SEEK_MODE_CLOSEST_SYNC, 4266553, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0063
 * @tc.name      : demuxer seek, TimedmetadataVideo.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0063, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/TimedmetadataVideo.mp4", SEEK_MODE_NEXT_SYNC, 0, 123, 0, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/TimedmetadataVideo.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 123, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/TimedmetadataVideo.mp4", SEEK_MODE_PREVIOUS_SYNC, 2133333, 123, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/TimedmetadataVideo.mp4", SEEK_MODE_PREVIOUS_SYNC, 4033333, 123, 0, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/TimedmetadataVideo.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 123, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/TimedmetadataVideo.mp4", SEEK_MODE_CLOSEST_SYNC, 2133333, 123, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/TimedmetadataVideo.mp4", SEEK_MODE_CLOSEST_SYNC, 4033333, 123, 0, 0};
    CheckSeekMode(fileTest7);
}

/**
 * @tc.number    : DEMUXER_SEEK_0064
 * @tc.name      : demuxer seek, ts_video.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0064, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/ts_video.ts", SEEK_MODE_NEXT_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/ts_video.ts", SEEK_MODE_NEXT_SYNC, 6433333, 215, 138, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/ts_video.ts", SEEK_MODE_NEXT_SYNC, 10000033, 1, 2, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/ts_video.ts", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/ts_video.ts", SEEK_MODE_PREVIOUS_SYNC, 6433333, 216, 138, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/ts_video.ts", SEEK_MODE_PREVIOUS_SYNC, 10000033, 1, 2, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/ts_video.ts", SEEK_MODE_CLOSEST_SYNC, 0, 602, 384, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/ts_video.ts", SEEK_MODE_CLOSEST_SYNC, 6433333, 216, 138, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/ts_video.ts", SEEK_MODE_CLOSEST_SYNC, 10000033, 1, 2, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0065
 * @tc.name      : demuxer seek, video_2audio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0065, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/video_2audio.mp4", SEEK_MODE_NEXT_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/video_2audio.mp4", SEEK_MODE_NEXT_SYNC, 4983333, 105, 76, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest4{"/data/test/media/video_2audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/video_2audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 4983333, 354, 256, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/video_2audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 10016666, 105, 77, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/video_2audio.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/video_2audio.mp4", SEEK_MODE_CLOSEST_SYNC, 4983333, 354, 256, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/video_2audio.mp4", SEEK_MODE_CLOSEST_SYNC, 10016666, 105, 77, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0066
 * @tc.name      : demuxer seek, video_9audio.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0066, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/video_9audio.mp4", SEEK_MODE_NEXT_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/video_9audio.mp4", SEEK_MODE_NEXT_SYNC, 4983333, 105, 76, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest4{"/data/test/media/video_9audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/video_9audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 4983333, 354, 256, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/video_9audio.mp4", SEEK_MODE_PREVIOUS_SYNC, 10016666, 105, 77, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/video_9audio.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 602, 433, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/video_9audio.mp4", SEEK_MODE_CLOSEST_SYNC, 4983333, 354, 256, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/video_9audio.mp4", SEEK_MODE_CLOSEST_SYNC, 10016666, 105, 77, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0067
 * @tc.name      : demuxer seek, vtt_5600.vtt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0067, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/vtt_5600.vtt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 1};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/vtt_5600.vtt", SEEK_MODE_NEXT_SYNC, 3100000, 0, 0, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/vtt_5600.vtt", SEEK_MODE_NEXT_SYNC, 6100000, 0, 0, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/vtt_5600.vtt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 1};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/vtt_5600.vtt", SEEK_MODE_PREVIOUS_SYNC, 3100000, 0, 0, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/vtt_5600.vtt", SEEK_MODE_PREVIOUS_SYNC, 6100000, 0, 0, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/vtt_5600.vtt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 1};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/vtt_5600.vtt", SEEK_MODE_CLOSEST_SYNC, 3100000, 0, 0, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/vtt_5600.vtt", SEEK_MODE_CLOSEST_SYNC, 6100000, 0, 0, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0068
 * @tc.name      : demuxer seek, vtt_5700.vtt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0068, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/vtt_5700.vtt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/vtt_5700.vtt", SEEK_MODE_NEXT_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/vtt_5700.vtt", SEEK_MODE_NEXT_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/vtt_5700.vtt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/vtt_5700.vtt", SEEK_MODE_PREVIOUS_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/vtt_5700.vtt", SEEK_MODE_PREVIOUS_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/vtt_5700.vtt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/vtt_5700.vtt", SEEK_MODE_CLOSEST_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/vtt_5700.vtt", SEEK_MODE_CLOSEST_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0069
 * @tc.name      : demuxer seek, vtt_5900.vtt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0069, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/vtt_5900.vtt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/vtt_5900.vtt", SEEK_MODE_NEXT_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/vtt_5900.vtt", SEEK_MODE_NEXT_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/vtt_5900.vtt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/vtt_5900.vtt", SEEK_MODE_PREVIOUS_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/vtt_5900.vtt", SEEK_MODE_PREVIOUS_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/vtt_5900.vtt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/vtt_5900.vtt", SEEK_MODE_CLOSEST_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/vtt_5900.vtt", SEEK_MODE_CLOSEST_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0070
 * @tc.name      : demuxer seek, vtt_6100.vtt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0070, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/vtt_6100.vtt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/vtt_6100.vtt", SEEK_MODE_NEXT_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/vtt_6100.vtt", SEEK_MODE_NEXT_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/vtt_6100.vtt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/vtt_6100.vtt", SEEK_MODE_PREVIOUS_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/vtt_6100.vtt", SEEK_MODE_PREVIOUS_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/vtt_6100.vtt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/vtt_6100.vtt", SEEK_MODE_CLOSEST_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/vtt_6100.vtt", SEEK_MODE_CLOSEST_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0071
 * @tc.name      : demuxer seek, webvtt_test.vtt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0071, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/webvtt_test.vtt", SEEK_MODE_NEXT_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/webvtt_test.vtt", SEEK_MODE_NEXT_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/webvtt_test.vtt", SEEK_MODE_NEXT_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/webvtt_test.vtt", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/webvtt_test.vtt", SEEK_MODE_PREVIOUS_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/webvtt_test.vtt", SEEK_MODE_PREVIOUS_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/webvtt_test.vtt", SEEK_MODE_CLOSEST_SYNC, 0, 0, 0, 8};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/webvtt_test.vtt", SEEK_MODE_CLOSEST_SYNC, 3100000, 0, 0, 4};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/webvtt_test.vtt", SEEK_MODE_CLOSEST_SYNC, 6100000, 0, 0, 1};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0072
 * @tc.name      : demuxer seek, xm.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0072, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/xm.mp4", SEEK_MODE_NEXT_SYNC, 0, 74, 112, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/xm.mp4", SEEK_MODE_NEXT_SYNC, 1232922, 14, 20, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/xm.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 74, 112, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/xm.mp4", SEEK_MODE_PREVIOUS_SYNC, 1232922, 44, 68, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/xm.mp4", SEEK_MODE_PREVIOUS_SYNC, 2432522, 14, 21, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/xm.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 74, 112, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/xm.mp4", SEEK_MODE_CLOSEST_SYNC, 1232922, 44, 68, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/xm.mp4", SEEK_MODE_CLOSEST_SYNC, 2432522, 14, 21, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0073
 * @tc.name      : demuxer seek, AAC_48000_1.aac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0073, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_NEXT_SYNC, 0, 0, 9457, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_NEXT_SYNC, 109783945, 0, 4729, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_NEXT_SYNC, 219567891, 0, 2, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 9457, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_PREVIOUS_SYNC, 109783945, 0, 4730, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_PREVIOUS_SYNC, 219567891, 0, 2, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_CLOSEST_SYNC, 0, 0, 9457, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_CLOSEST_SYNC, 109783945, 0, 4730, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/AAC_48000_1.aac", SEEK_MODE_CLOSEST_SYNC, 219567891, 0, 2, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0074
 * @tc.name      : demuxer seek, amr_nb_8000_1.amr
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0074, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_NEXT_SYNC, 0, 0, 1501, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_NEXT_SYNC, 15000000, 0, 751, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_NEXT_SYNC, 30000000, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 1501, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_PREVIOUS_SYNC, 15000000, 0, 751, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_PREVIOUS_SYNC, 30000000, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_CLOSEST_SYNC, 0, 0, 1501, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_CLOSEST_SYNC, 15000000, 0, 751, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/amr_nb_8000_1.amr", SEEK_MODE_CLOSEST_SYNC, 30000000, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0075
 * @tc.name      : demuxer seek, amr_wb_16000_1.amr
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0075, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_NEXT_SYNC, 0, 0, 1500, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_NEXT_SYNC, 15000000, 0, 750, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_NEXT_SYNC, 29980000, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 1500, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_PREVIOUS_SYNC, 15000000, 0, 750, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_PREVIOUS_SYNC, 29980000, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_CLOSEST_SYNC, 0, 0, 1500, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_CLOSEST_SYNC, 15000000, 0, 750, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/amr_wb_16000_1.amr", SEEK_MODE_CLOSEST_SYNC, 29980000, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0076
 * @tc.name      : demuxer seek, ape.ape
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0076, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/ape.ape", SEEK_MODE_NEXT_SYNC, 0, 0, 8, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/ape.ape", SEEK_MODE_NEXT_SYNC, 6144000, 0, 4, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/ape.ape", SEEK_MODE_NEXT_SYNC, 10752000, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/ape.ape", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 8, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/ape.ape", SEEK_MODE_PREVIOUS_SYNC, 6144000, 0, 4, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/ape.ape", SEEK_MODE_PREVIOUS_SYNC, 10752000, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/ape.ape", SEEK_MODE_CLOSEST_SYNC, 0, 0, 8, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/ape.ape", SEEK_MODE_CLOSEST_SYNC, 6144000, 0, 4, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/ape.ape", SEEK_MODE_CLOSEST_SYNC, 10752000, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0077
 * @tc.name      : demuxer seek, feff_bom.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0077, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_NEXT_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_NEXT_SYNC, 2638367, 0, 106, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_NEXT_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 2638367, 0, 107, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 2638367, 0, 107, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/feff_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0078
 * @tc.name      : demuxer seek, fffe_bom.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0078, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_NEXT_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_NEXT_SYNC, 2638367, 0, 106, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_NEXT_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 2638367, 0, 107, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 2638367, 0, 107, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/fffe_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0079
 * @tc.name      : demuxer seek, FLAC_48000_1.flac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0079, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_NEXT_SYNC, 0, 0, 2288, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_NEXT_SYNC, 109824000, 0, 1144, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_NEXT_SYNC, 219552000, 0, 2, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 2288, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_PREVIOUS_SYNC, 109824000, 0, 1144, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_PREVIOUS_SYNC, 219552000, 0, 2, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_CLOSEST_SYNC, 0, 0, 2288, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_CLOSEST_SYNC, 109824000, 0, 1144, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/FLAC_48000_1.flac", SEEK_MODE_CLOSEST_SYNC, 219552000, 0, 2, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0080
 * @tc.name      : demuxer seek, M4A_48000_1.m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0080, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_NEXT_SYNC, 0, 0, 10292, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_NEXT_SYNC, 109781333, 0, 5146, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_NEXT_SYNC, 219541333, 0, 2, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 10292, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_PREVIOUS_SYNC, 109781333, 0, 5147, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_PREVIOUS_SYNC, 219541333, 0, 2, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_CLOSEST_SYNC, 0, 0, 10292, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_CLOSEST_SYNC, 109781333, 0, 5147, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/M4A_48000_1.m4a", SEEK_MODE_CLOSEST_SYNC, 219541333, 0, 2, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0081
 * @tc.name      : demuxer seek, MP3_48000_1.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0081, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_NEXT_SYNC, 0, 0, 9150, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_NEXT_SYNC, 109800000, 0, 4573, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_NEXT_SYNC, 219576000, 0, 15, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 9150, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_PREVIOUS_SYNC, 109800000, 0, 4574, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_PREVIOUS_SYNC, 219576000, 0, 15, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_CLOSEST_SYNC, 0, 0, 9150, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_CLOSEST_SYNC, 109800000, 0, 4574, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/MP3_48000_1.mp3", SEEK_MODE_CLOSEST_SYNC, 219576000, 0, 15, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0082
 * @tc.name      : demuxer seek, nonstandard_bom.mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0082, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_NEXT_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_NEXT_SYNC, 2638367, 0, 106, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_NEXT_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 2638367, 0, 107, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_PREVIOUS_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 0, 0, 203, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 2638367, 0, 107, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/nonstandard_bom.mp3", SEEK_MODE_CLOSEST_SYNC, 5276734, 0, 11, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0083
 * @tc.name      : demuxer seek, OGG_48000_1.ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0083, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_NEXT_SYNC, 0, 0, 11439, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_NEXT_SYNC, 104726666, 0, 5677, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_NEXT_SYNC, 219545333, 0, 66, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 11439, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_PREVIOUS_SYNC, 104726666, 0, 5724, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_PREVIOUS_SYNC, 219545333, 0, 66, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_CLOSEST_SYNC, 0, 0, 11439, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_CLOSEST_SYNC, 104726666, 0, 5724, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/OGG_48000_1.ogg", SEEK_MODE_CLOSEST_SYNC, 219545333, 0, 66, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0084
 * @tc.name      : demuxer seek, wav_48000_1.wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0084, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_NEXT_SYNC, 0, 0, 5146, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_NEXT_SYNC, 109781333, 0, 2573, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_NEXT_SYNC, 219520000, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 5146, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_PREVIOUS_SYNC, 109781333, 0, 2573, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_PREVIOUS_SYNC, 219520000, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_CLOSEST_SYNC, 0, 0, 5146, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_CLOSEST_SYNC, 109781333, 0, 2573, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/wav_48000_1.wav", SEEK_MODE_CLOSEST_SYNC, 219520000, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0085
 * @tc.name      : demuxer seek, wav_audio_test_1562.wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0085, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_NEXT_SYNC, 0, 0, 7, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_NEXT_SYNC, 1536000, 0, 4, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_NEXT_SYNC, 3072000, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 7, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_PREVIOUS_SYNC, 1536000, 0, 4, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_PREVIOUS_SYNC, 3072000, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_CLOSEST_SYNC, 0, 0, 7, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_CLOSEST_SYNC, 1536000, 0, 4, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/wav_audio_test_1562.wav", SEEK_MODE_CLOSEST_SYNC, 3072000, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0086
 * @tc.name      : demuxer seek, wav_audio_test_202406290859.wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0086, TestSize.Level2)
{
    seekInfo fileTest1{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_NEXT_SYNC, 0, 0, 103, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_NEXT_SYNC,
    2368435, 0, 52, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_NEXT_SYNC, 4736870, 0, 1, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_PREVIOUS_SYNC, 0, 0, 103, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_PREVIOUS_SYNC,
    2368435, 0, 52, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_PREVIOUS_SYNC,
    4736870, 0, 1, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_CLOSEST_SYNC, 0, 0, 103, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_CLOSEST_SYNC,
    2368435, 0, 52, 0};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{"/data/test/media/audio/wav_audio_test_202406290859.wav", SEEK_MODE_CLOSEST_SYNC,
    4736870, 0, 1, 0};
    CheckSeekMode(fileTest9);
}

/**
 * @tc.number    : DEMUXER_SEEK_0087
 * @tc.name      : demuxer seek, video_audio_one_frame_test.mp4, SEEK_MODE_NEXT_SYNC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0087, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/video_audio_one_frame_test.mp4", SEEK_MODE_NEXT_SYNC, 0, 1, 5, 0};
    CheckSeekMode(fileTest);
}

/**
 * @tc.number    : DEMUXER_SEEK_0088
 * @tc.name      : demuxer seek, video_audio_one_frame_test.mp4, SEEK_MODE_PREVIOUS_SYNC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0088, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/video_audio_one_frame_test.mp4", SEEK_MODE_PREVIOUS_SYNC, 0, 1, 5, 0};
    CheckSeekMode(fileTest);
}

/**
 * @tc.number    : DEMUXER_SEEK_0089
 * @tc.name      : demuxer seek, video_audio_one_frame_test.mp4, SEEK_MODE_CLOSEST_SYNC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeekNdkTest, DEMUXER_SEEK_0089, TestSize.Level2)
{
    seekInfo fileTest{"/data/test/media/video_audio_one_frame_test.mp4", SEEK_MODE_CLOSEST_SYNC, 0, 1, 5, 0};
    CheckSeekMode(fileTest);
}