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

#include "native_avcodec_base.h"
#include "avcodec_audio_channel_layout.h"
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
#ifdef SUPPORT_CODEC_RM
namespace OHOS {
namespace Media {
class DemuxerRmFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/ac3.rm";
    const char *INP_DIR_2 = "/data/test/media/test_error.rm";
    const char *INP_DIR_3 = "/data/test/media/rv40_aac.rm";
    const char *INP_DIR_4 = "/data/test/media/rv30_cook.rm";
};

static int g_fd = -1;
static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
static OH_AVErrCode ret = AV_ERR_OK;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
constexpr int32_t THOUSAND = 1000.0;
constexpr int32_t ONE = 1;
constexpr int32_t TWO = 2;
void DemuxerRmFuncNdkTest::SetUpTestCase() {}
void DemuxerRmFuncNdkTest::TearDownTestCase() {}
void DemuxerRmFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerRmFuncNdkTest::TearDown()
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
    if (g_fd > 0) {
        close(g_fd);
        g_fd = -1;
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
    int32_t audioCount;
    int32_t videoCount;
};

static void CheckSeekMode(seekInfo seekInfo)
{
    int trackType = 0;
    OH_AVCodecBufferAttr attr;
    g_fd = open(seekInfo.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo.fileName);
    cout << seekInfo.fileName << "-------" << g_fd << "-------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
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
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
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
        if (trackType == MEDIA_TYPE_VID) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(seekInfo.videoCount, frameNum);
        } else if (trackType == MEDIA_TYPE_AUD) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(seekInfo.audioCount, frameNum);
        }
    }
}

static void CheckSeekModeCombination(seekInfo seekInfo1, seekInfo secondSeekInfo)
{
    int trackType = 0;
    OH_AVCodecBufferAttr attr;
    g_fd = open(seekInfo1.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo1.fileName);
    cout << seekInfo1.fileName << "-------" << g_fd << "-------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
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
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo1.millisecond / THOUSAND, seekInfo1.seekmode));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, secondSeekInfo.millisecond / THOUSAND,
                                                 secondSeekInfo.seekmode));
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
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
        if (trackType == MEDIA_TYPE_VID) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(secondSeekInfo.videoCount, frameNum);
        } else if (trackType == MEDIA_TYPE_AUD) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(secondSeekInfo.audioCount, frameNum);
        }
    }
    close(g_fd);
    g_fd = -1;
}

static void DemuxerResult(const char *file, int aFrames, int vFrames)
{
    int trackType = 0;
    OH_AVCodecBufferAttr attr;
    g_fd = open(file, O_RDONLY);
    if (g_fd < 0) {
        return;
    }
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
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
        if (trackType == MEDIA_TYPE_VID) {
            cout << "vFrames---" << frameNum << endl;
            ASSERT_EQ(vFrames, frameNum);
        } else if (trackType == MEDIA_TYPE_AUD) {
            cout << "aFrames---" << frameNum << endl;
            ASSERT_EQ(aFrames, frameNum);
        }
    }
    close(g_fd);
    g_fd = -1;
}

static void DemuxerFile(const char *file)
{
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0100
 * @tc.name      : create source with g_fd, rm
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0100, TestSize.Level2)
{
    DemuxerResult(INP_DIR_1, 288, 0);
}
/**
 * @tc.number    : DEMUXER_RM_FUNC_0200
 * @tc.name      : demuxer rm ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0200, TestSize.Level2)
{
    DemuxerFile(INP_DIR_1);
    ASSERT_EQ(g_trackCount, ONE);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0300
 * @tc.name      : demuxer rm GetPublicTrackFormat,MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0300, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    int trackType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &stringVal));
        if (trackType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(0, strcmp(stringVal, OH_AVCODEC_MIMETYPE_AUDIO_AC3));
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0400
 * @tc.name      : demuxer rm ,GetAudioTrackFormat,MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0400, TestSize.Level2)
{
    int32_t sr = 0;
    DemuxerFile(INP_DIR_1);
    int trackType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
        if (trackType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(sr, 44100);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0500
 * @tc.name      : demuxer rm ,GetAudioTrackFormat ,MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0500, TestSize.Level2)
{
    int64_t br = 0;
    DemuxerFile(INP_DIR_1);
    int trackType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
        if (trackType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(br, 96000);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0600
 * @tc.name      : demuxer rm ,GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0600, TestSize.Level2)
{
    int32_t cc = 0;
    DemuxerFile(INP_DIR_1);
    int trackType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
        if (trackType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(cc, ONE);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0700
 * @tc.name      : demuxer rm ,check source format,OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0700, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "test"));
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0800
 * @tc.name      : demuxer rm ,check source format,OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0800, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_0900
 * @tc.name      : demuxer rm ,check track format,OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_0900, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1000
 * @tc.name      : demuxer rm ,check track format,OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1000, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1100
 * @tc.name      : demuxer rm ,check track format,OH_MD_KEY_GENRE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1100, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_GENRE, &stringVal));
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1200
 * @tc.name      : demuxer rm ,check track format,OH_MD_KEY_COPYRIGHT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1200, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COPYRIGHT, &stringVal));
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1300
 * @tc.name      : demuxer rm ,GetPublicTrackFormat,OH_MD_KEY_CHANNEL_LAYOUT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1300, TestSize.Level2)
{
    int64_t channelLayout;
    DemuxerFile(INP_DIR_1);
    int trackType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        if (trackType == MEDIA_TYPE_VID) {
            continue;
        }
        ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_CHANNEL_LAYOUT, &channelLayout));
        ASSERT_EQ(MONO, channelLayout);
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1400
 * @tc.name      : demuxer rm ,GetPublicTrackFormat,OH_MD_KEY_AUDIO_SAMPLE_FORMAT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1400, TestSize.Level2)
{
    int32_t sampleFormat;
    DemuxerFile(INP_DIR_1);
    int trackType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        if (trackType == MEDIA_TYPE_VID) {
            continue;
        }
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &sampleFormat));
        int sampleFormatResult = 9;
        ASSERT_EQ(sampleFormatResult, sampleFormat);
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1500
 * @tc.name      : demuxer rm ,GetPublicTrackFormat,OH_MD_KEY_BITS_PER_CODED_SAMPLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1500, TestSize.Level2)
{
    int32_t bitsPreCodedSample;
    DemuxerFile(INP_DIR_1);
    int trackType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        if (trackType == MEDIA_TYPE_VID) {
            continue;
        }
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_BITS_PER_CODED_SAMPLE, &bitsPreCodedSample));
        int bitsPreCodedSampleResult = 0;
        ASSERT_EQ(bitsPreCodedSampleResult, bitsPreCodedSample);
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1600
 * @tc.name      : seek to the start time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 288, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1700
 * @tc.name      : seek to the start time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 0, 288, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1800
 * @tc.name      : seek to the start time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 0, 288, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_1900
 * @tc.name      : seek to the end time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_1900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 10031000, 7, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2000
 * @tc.name      : seek to the end time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 10031000, 7, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2100
 * @tc.name      : seek to the end time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 10031000, 7, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2200
 * @tc.name      : seek to the middle time, 5000000 millisecond, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 5000000, 182, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2300
 * @tc.name      : seek to the middle time, 5000000 millisecond, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 5000000, 102, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2400
 * @tc.name      : seek to the middle time, 5000000 millisecond, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 5000000, 182, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2500
 * @tc.name      : seek to the middle time, closest mode, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 288, 0};
    seekInfo fileTest2{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 5000000, 182, 0};
    CheckSeekModeCombination(fileTest1, fileTest2);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2600
 * @tc.name      : seek to the middle time, previous mode, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 288, 0};
    seekInfo fileTest2{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 10031000, 7, 0};
    CheckSeekModeCombination(fileTest1, fileTest2);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2700
 * @tc.name      : seek to the middle time, closest mode, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 5000000, 182, 0};
    seekInfo fileTest2{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 10031000, 7, 0};
    CheckSeekModeCombination(fileTest1, fileTest2);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2800
 * @tc.name      : demuxer damaged rm audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2800, TestSize.Level2)
{
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_2900
 * @tc.name      : seek to a invalid time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_2900, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    srand(time(nullptr));
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));

    ASSERT_NE(demuxer, nullptr);
    int64_t invalidPts = 12000 * 16666;
    ret = OH_AVDemuxer_SeekToTime(demuxer, invalidPts, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_NE(ret, AV_ERR_OK);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3000
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3000, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    srand(time(nullptr));
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_UnselectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3100
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3100, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char *file = INP_DIR_1;
    bool isEnd = false;
    int count = 0;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, ONE);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    srand(time(nullptr));
    int pos = rand() % 250;
    cout << " pos= " << pos << endl;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (count == pos) {
                cout << count << " count == pos!!!!!!!!!" << endl;
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 0));
                ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
                isEnd = true;
                break;
            }
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end !!!!!!!!!!!!!!!" << endl;
            }
            if (index == MEDIA_TYPE_AUD) {
                count++;
            }
        }
    }
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3200
 * @tc.name      : start demux before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3200, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = INP_DIR_1;
    srand(time(nullptr));
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3300
 * @tc.name      : create source with g_fd, rv40_aac.rm
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3300, TestSize.Level2)
{
    DemuxerResult(INP_DIR_3, 12730, 14204);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3400
 * @tc.name      : create source with g_fd, rv30_cook.rm
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3400, TestSize.Level2)
{
    DemuxerResult(INP_DIR_4, 2720, 1937);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3500
 * @tc.name      : seek to the middle time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 557000, 2720, 1937};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3600
 * @tc.name      : seek to the middle time, close mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 55000000, 11524, 12861};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3700
 * @tc.name      : demuxer rv40_aac.rm GetPublicTrackFormat,MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3700, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_3);
    int trackType = 1;
    trackFormat = OH_AVSource_GetTrackFormat(source, trackType);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, OH_AVCODEC_MIMETYPE_AUDIO_AAC));
    OH_AVFormat_Destroy(trackFormat);
    trackFormat = nullptr;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3800
 * @tc.name      : demuxer rv40_aac.rm ,GetAudioTrackFormat,MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3800, TestSize.Level2)
{
    int32_t sr = 0;
    DemuxerFile(INP_DIR_3);
    int trackType = 1;
    trackFormat = OH_AVSource_GetTrackFormat(source, trackType);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    OH_AVFormat_Destroy(trackFormat);
    trackFormat = nullptr;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_3900
 * @tc.name      : demuxer rv40_aac.rm ,GetAudioTrackFormat ,MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_3900, TestSize.Level2)
{
    int64_t br = 0;
    DemuxerFile(INP_DIR_3);
    int trackType = 1;
    trackFormat = OH_AVSource_GetTrackFormat(source, trackType);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 48000);
    OH_AVFormat_Destroy(trackFormat);
    trackFormat = nullptr;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_4000
 * @tc.name      : demuxer rv40_aac.rm ,GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_4000, TestSize.Level2)
{
    int32_t cc = 0;
    DemuxerFile(INP_DIR_3);
    int trackType = 1;
    trackFormat = OH_AVSource_GetTrackFormat(source, trackType);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, TWO);
    OH_AVFormat_Destroy(trackFormat);
    trackFormat = nullptr;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_4100
 * @tc.name      : demuxer rv40_aac.rm ,GetPublicTrackFormat,OH_MD_KEY_CHANNEL_LAYOUT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_4100, TestSize.Level2)
{
    int64_t channelLayout;
    DemuxerFile(INP_DIR_3);
    int trackType = 1;
    trackFormat = OH_AVSource_GetTrackFormat(source, trackType);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_CHANNEL_LAYOUT, &channelLayout));
    ASSERT_EQ(STEREO, channelLayout);
    OH_AVFormat_Destroy(trackFormat);
    trackFormat = nullptr;
}

/**
 * @tc.number    : DEMUXER_RM_FUNC_4200
 * @tc.name      : demuxer rv40_aac.rm ,GetPublicTrackFormat,OH_MD_KEY_AUDIO_SAMPLE_FORMAT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_FUNC_4200, TestSize.Level2)
{
    int32_t sampleFormat;
    DemuxerFile(INP_DIR_3);
    int trackType = 1;
    trackFormat = OH_AVSource_GetTrackFormat(source, trackType);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &sampleFormat));
    int sampleFormatResult = 9;
    ASSERT_EQ(sampleFormatResult, sampleFormat);
    OH_AVFormat_Destroy(trackFormat);
    trackFormat = nullptr;
}

/**
 * @tc.number    : DEMUXER_RM_ILLEGAL_PARA_0100
 * @tc.name      : input invalid rm uri file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerRmFuncNdkTest, DEMUXER_RM_ILLEGAL_PARA_0100, TestSize.Level2)
{
    const char *uri = "http://192.168.3.11:8080/share/invalid.rm";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(source, nullptr);
}
#endif
