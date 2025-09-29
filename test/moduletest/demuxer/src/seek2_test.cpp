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
class DemuxerSeek2NdkTest : public testing::Test {
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
void DemuxerSeek2NdkTest::SetUpTestCase() {}
void DemuxerSeek2NdkTest::TearDownTestCase() {}
void DemuxerSeek2NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerSeek2NdkTest::TearDown()
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

static void CheckSeekMode(seekInfo seekInfo)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    int fd = open(seekInfo.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo.fileName);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
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
            ASSERT_EQ(seekInfo.audioCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_SUBTITLE) {
            ASSERT_EQ(seekInfo.subtitleCount, frameNum);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
    OH_AVSource_Destroy(source);
    source = nullptr;
    OH_AVDemuxer_Destroy(demuxer);
    demuxer = nullptr;
    OH_AVFormat_Destroy(sourceFormat);
    sourceFormat = nullptr;
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SEEK_0087
 * @tc.name      : demuxer seek, H264_base@5_1920_1080_30_AAC_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0087, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 94, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 900000, 30, 47, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 94, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 900000, 40, 63, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 16, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 94, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 900000, 30, 47, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 16, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0088
 * @tc.name      : demuxer seek, H264_high@5.1_3840_2160_30_MP3_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0088, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 85, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 866667, 30, 45, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 85, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 866667, 40, 60, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 18, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 85, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 866667, 30, 45, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 18, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0089
 * @tc.name      : demuxer seek, H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0089, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_NEXT_SYNC, 0, 119, 72, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_NEXT_SYNC, 1300000, 39, 27, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 119, 72, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1300000, 49, 33, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 9, 7, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 119, 72, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1300000, 39, 27, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 9, 7, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0090
 * @tc.name      : demuxer seek, H264_main@3_720_480_30_PCM_48K_24_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0090, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 144, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_NEXT_SYNC, 1266667, 20, 52, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 144, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1266667, 30, 77, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 28, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 144, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1266667, 20, 52, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 28, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0091
 * @tc.name      : demuxer seek, H264_main@4_1280_720_60_MP2_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0091, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 0, 119, 77, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 1366667, 29, 21, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 119, 77, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1366667, 39, 28, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 9, 9, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 119, 77, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1366667, 39, 28, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 9, 9, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0092
 * @tc.name      : demuxer seek, H265_main@4_1920_1080_30_AAC_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0092, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 88, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 1133333, 24, 38, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 88, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1133333, 34, 53, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 19, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 88, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1133333, 24, 38, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 19, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0093
 * @tc.name      : demuxer seek, H265_main@5_3840_2160_30_vorbis_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0093, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 96, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 1233333, 22, 38, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 96, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1233333, 33, 56, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 20, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 96, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1233333, 33, 56, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 20, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0094
 * @tc.name      : demuxer seek, H265_main10@4.1_1280_720_60_MP2_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0094, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 119, 84, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_NEXT_SYNC, 983333, 53, 39, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 119, 84, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 983333, 63, 47, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 13, 12, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 119, 84, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 983333, 63, 47, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 13, 12, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0095
 * @tc.name      : demuxer seek, H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0095, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_NEXT_SYNC, 0, 119, 171, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_NEXT_SYNC, 1433333, 30, 46, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 119, 171, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1433333, 40, 61, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 19, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 119, 171, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1433333, 30, 46, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 19, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0096
 * @tc.name      : demuxer seek, H265_main10@5_1920_1080_60_MP3_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0096, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 0, 119, 78, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 1300000, 40, 27, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 119, 78, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1300000, 53, 37, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 9, 9, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 119, 78, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1300000, 53, 37, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 9, 9, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0097
 * @tc.name      : demuxer seek, MPEG4_ASP@3_352_288_30_MP3_32K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0097, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 57, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_NEXT_SYNC, 500000, 40, 37, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 57, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 500000, 50, 47, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 10, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 57, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 500000, 50, 47, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 10, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0098
 * @tc.name      : demuxer seek, MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0098, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 90, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_NEXT_SYNC, 933333, 30, 46, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 90, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 933333, 40, 61, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 16, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 90, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 933333, 30, 46, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 16, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0099
 * @tc.name      : demuxer seek, MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0099, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 45, 65, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_NEXT_SYNC, 700000, 15, 21, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 45, 65, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 700000, 25, 37, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1466667, 5, 8, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 45, 65, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 700000, 25, 37, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1466667, 5, 8, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0100
 * @tc.name      : demuxer seek, MPEG4_SP@5_720_480_30_AAC_32K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0100, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 63, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_NEXT_SYNC, 1466667, 10, 10, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 63, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1466667, 20, 22, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 11, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 63, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1466667, 20, 22, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 11, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0101
 * @tc.name      : demuxer seek, MPEG4_SP@6_1280_720_30_MP2_32K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0101, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_NEXT_SYNC, 0, 60, 56, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_NEXT_SYNC, 1100000, 20, 19, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 56, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1100000, 30, 29, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_PREVIOUS_SYNC, 1966667, 10, 10, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 56, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1100000, 30, 29, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov",
     SEEK_MODE_CLOSEST_SYNC, 1966667, 10, 10, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0102
 * @tc.name      : demuxer seek, H264_base@5_1920_1080_30_MP2_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0102, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 60, 77, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 800000, 36, 72, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 77, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 800000, 36, 72, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 2000000, 2, 28, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 77, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 800000, 36, 72, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 2000000, 2, 28, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0103
 * @tc.name      : demuxer seek, H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0103, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 118, 82, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 940000, 59, 69, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 118, 82, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 940000, 62, 73, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 1950000, 2, 29, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 118, 82, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 940000, 62, 73, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 1950000, 2, 29, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0104
 * @tc.name      : demuxer seek, H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0104, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 59, 68, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 940000, 29, 48, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 59, 68, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 940000, 30, 58, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 2000000, 1, 17, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 59, 68, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 940000, 30, 58, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 2000000, 1, 17, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0105
 * @tc.name      : demuxer seek, H264_main@4.2_1280_720_60_MP3_32K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0105, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 118, 53, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 700000, 74, 46, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 118, 53, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 700000, 78, 46, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 1983333, 3, 15, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 118, 53, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 700000, 74, 46, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 1983333, 3, 15, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0106
 * @tc.name      : demuxer seek, MPEG2_422p_1280_720_60_MP3_32K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0106, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 118, 47, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 700000, 76, 38, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 118, 47, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 700000, 78, 38, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 1983333, 2, 1, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 118, 47, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 700000, 78, 38, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 1983333, 2, 1, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0107
 * @tc.name      : demuxer seek, MPEG2_high_720_480_30_MP2_32K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0107, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 59, 57, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 790000, 36, 52, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 59, 57, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 790000, 38, 55, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 2000000, 2, 20, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 59, 57, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 790000, 36, 52, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 2000000, 2, 20, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0108
 * @tc.name      : demuxer seek, MPEG2_main_352_288_30_MP2_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0108, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 60, 79, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_NEXT_SYNC, 1260000, 20, 52, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 79, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 1260000, 23, 54, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 1933333, 3, 31, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 79, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 1260000, 23, 54, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg",
     SEEK_MODE_CLOSEST_SYNC, 1933333, 3, 31, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0109
 * @tc.name      : demuxer seek, MPEG2_main_1920_1080_30_MP3_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0109, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 60, 79, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 1190000, 24, 48, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 60, 79, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 1190000, 25, 48, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 2000000, 2, 16, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 60, 79, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 1190000, 24, 48, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 2000000, 2, 16, 0};
    CheckSeekMode(fileTest8);
}

/**
 * @tc.number    : DEMUXER_SEEK_0110
 * @tc.name      : demuxer seek, MPEG2_simple_320_240_24_MP3_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerSeek2NdkTest, DEMUXER_SEEK_0110, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 0, 48, 79, 0};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_NEXT_SYNC, 833333, 28, 74, 0};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 0, 48, 79, 0};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 833333, 34, 74, 0};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_PREVIOUS_SYNC, 1939689, 5, 32, 0};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 0, 48, 79, 0};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 833333, 28, 74, 0};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{"/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg",
     SEEK_MODE_CLOSEST_SYNC, 1939689, 5, 32, 0};
    CheckSeekMode(fileTest8);
}