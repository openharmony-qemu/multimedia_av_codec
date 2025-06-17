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
class DemuxerAviFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/AVI_H263_baseline@level10_352_288_AAC_2.avi";
    const char *INP_DIR_2 = "/data/test/media/AVI_H263_baseline@level20_352_288_MP2_1.avi";
    const char *INP_DIR_3 = "/data/test/media/AVI_H263_baseline@level60_704_576_MP3_2.avi";
    const char *INP_DIR_4 = "/data/test/media/AVI_H263_baseline@level70_1408_1152_PCM_mulaw_1.avi";
    const char *INP_DIR_5 = "/data/test/media/AVI_H264_constrained_baseline@level4.1_1280_720_AAC_2.avi";
    const char *INP_DIR_6 = "/data/test/media/AVI_H264_main@level5_1920_1080_MP2_1.avi";
    const char *INP_DIR_7 = "/data/test/media/AVI_H264_high@level5.1_1920_1080_MP3_2.avi";
    const char *INP_DIR_8 = "/data/test/media/AVI_H264_high@level5_2560_1920_PCM_mulaw_1.avi";
    const char *INP_DIR_9 = "/data/test/media/AVI_MPEG2_simple@low_320_240_AAC_2.avi";
    const char *INP_DIR_10 = "/data/test/media/AVI_MPEG2_main@mian_640_480_MP2_1.avi";
    const char *INP_DIR_11 = "/data/test/media/AVI_MPEG2_simple@main_640_480_MP3_2.avi";
    const char *INP_DIR_12 = "/data/test/media/AVI_MPEG2_422P@high_1920_1080_PCM_s24_1.avi";
    const char *INP_DIR_13 = "/data/test/media/AVI_MPEG4_simple@level1_640_480_AAC_2.avi";
    const char *INP_DIR_14 = "/data/test/media/AVI_MPEG4_advanced_simple@level6_1280_720_MP2_1.avi";
    const char *INP_DIR_15 = "/data/test/media/AVI_MPEG4_advanced_simple@level3_352_288_MP3_2.avi";
    const char *INP_DIR_16 = "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi";
};

static int g_fd = -1;
static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVErrCode ret = AV_ERR_OK;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
constexpr int32_t THOUSAND = 1000.0;
constexpr int32_t TWO = 2;
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
void DemuxerAviFuncNdkTest::SetUpTestCase() {}
void DemuxerAviFuncNdkTest::TearDownTestCase() {}
void DemuxerAviFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerAviFuncNdkTest::TearDown()
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
    int32_t videoCount;
    int32_t audioCount;
};

static void CheckSeekModefromEnd(seekInfo seekInfo)
{
    int tarckType = 0;
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
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_UNKNOWN, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
    }
    close(g_fd);
    g_fd = -1;
}

static void CheckSeekMode(seekInfo seekInfo)
{
    int tarckType = 0;
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
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
        bool readEnd = false;
        int32_t frameNum = 0;
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            //cout << "frameNum---" << frameNum << "---PTS---" << attr.pts << "---tarckType---" << tarckType << endl;
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                readEnd = true;
                break;
            }
            frameNum++;
        }
        if (tarckType == MEDIA_TYPE_VID) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(seekInfo.videoCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_AUD) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(seekInfo.audioCount, frameNum);
        }
    }
    close(g_fd);
    g_fd = -1;
}

static void SetAudioValue(OH_AVCodecBufferAttr attr, bool &audioIsEnd, int &audioFrame, int &aKeyCount)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        audioIsEnd = true;
        cout << audioFrame << "    audio is end !!!!!!!!!!!!!!!" << endl;
    } else {
        audioFrame++;
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            aKeyCount++;
        }
    }
}

static void SetVideoValue(OH_AVCodecBufferAttr attr, bool &videoIsEnd, int &videoFrame, int &vKeyCount)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        videoIsEnd = true;
        cout << videoFrame << "   video is end !!!!!!!!!!!!!!!" << endl;
    } else {
        videoFrame++;
        cout << "video track !!!!!" << endl;
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            vKeyCount++;
        }
    }
}

static void DemuxerResult(const char *fileName)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    g_fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    cout << fileName << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            }
        }
    }
    close(g_fd);
    g_fd = -1;
}

static void DemuxerResultRaw(const char *fileName)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    const char* mimeType = nullptr;
    g_fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    cout << fileName << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_AUD) {
                ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
                ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_RAW));
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
        }
    }
    close(g_fd);
    g_fd = -1;
}
/**
 * @tc.number    : DEMUXER_AVI_FUNC_0100
 * @tc.name      : demuxer AVI ,GetVideoTrackFormat,MD_KEY_HEIGHT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0100, TestSize.Level2)
{
    int32_t height = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 288);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0200
 * @tc.name      : demuxer AVI ,GetVideoTrackFormat,MD_KEY_WIDTHs
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0200, TestSize.Level2)
{
    int32_t weight = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &weight));
    ASSERT_EQ(weight, 352);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0300
 * @tc.name      : demuxer avi ,check source format,OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0300, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "title"));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0400
 * @tc.name      : demuxer avi ,check source format,OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0400, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0500
 * @tc.name      : demuxer avi ,check track format,OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest,  DEMUXER_AVI_FUNC_0500, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "2023"));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0600
 * @tc.name      : demuxer avi ,check track format,OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0600, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "COMMENT"));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0700
 * @tc.name      : demuxer avi ,check track format,OH_MD_KEY_GENRE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0700, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_GENRE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "Classical"));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0800
 * @tc.name      : demuxer avi ,check track format,OH_MD_KEY_COPYRIGHT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0800, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COPYRIGHT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_0900
 * @tc.name      : demuxer avi ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_0900, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1000
 * @tc.name      : demuxer avi ,GetAudioTrackFormat ,MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1000, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 1092058);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1100
 * @tc.name      : demuxer avi ,GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1100, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 2);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1200
 * @tc.name      : demuxer avi ,GetAudioTrackFormat,MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1200, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1300
 * @tc.name      : demuxer avi GetPublicTrackFormat,MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1300, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_13;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, OH_AVCODEC_MIMETYPE_VIDEO_MPEG4_PART2));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1400
 * @tc.name      : demuxer avi ,GetPublicTrackFormat,MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1400, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1500
 * @tc.name      : create source with g_fd,avcc avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1500, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    const char *file = INP_DIR_7;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 14);
    ASSERT_EQ(videoFrame, 26);
    ASSERT_EQ(aKeyCount, 14);
    ASSERT_EQ(vKeyCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1600
 * @tc.name      : create source with g_fd,AVI_H263_baseline@level10_352_288_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1600, TestSize.Level3)
{
    DemuxerResult(INP_DIR_1);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1700
 * @tc.name      : create source with g_fd,AVI_H263_baseline@level20_352_288_MP2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1700, TestSize.Level3)
{
    DemuxerResult(INP_DIR_2);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1800
 * @tc.name      : create source with g_fd,AVI_H263_baseline@level60_704_576_MP3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1800, TestSize.Level3)
{
    DemuxerResult(INP_DIR_3);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_1900
 * @tc.name      : create source with g_fd,AVI_H263_baseline@level70_1408_1152_PCM_mulaw_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_1900, TestSize.Level3)
{
    DemuxerResult(INP_DIR_4);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2000
 * @tc.name      : create source with g_fd,AVI_H264_baseline@level4.1_1280_720_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2000, TestSize.Level3)
{
    DemuxerResult(INP_DIR_5);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2100
 * @tc.name      : create source with g_fd,AVI_H264_main@level5_1920_1080_PM2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2100, TestSize.Level3)
{
    DemuxerResult(INP_DIR_6);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2200
 * @tc.name      : create source with g_fd,AVI_H264_high@level5.1_1920_1080_PM3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2200, TestSize.Level3)
{
    DemuxerResult(INP_DIR_7);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2300
 * @tc.name      : create source with g_fd,AVI_H264_h422@level5.1_2560_1920_PCM_mulaw_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2300, TestSize.Level3)
{
    DemuxerResult(INP_DIR_8);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2400
 * @tc.name      : create source with g_fd,AVI_MPEG2_simple@low_320_240_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2400, TestSize.Level3)
{
    DemuxerResult(INP_DIR_9);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2500
 * @tc.name      : create source with g_fd,AVI_MPEG2_main@mian_720_480_MP2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2500, TestSize.Level3)
{
    DemuxerResult(INP_DIR_10);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2600
 * @tc.name      : create source with g_fd,AVI_MPEG2_high@high_1440_1080_MP3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2600, TestSize.Level3)
{
    DemuxerResult(INP_DIR_11);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2700
 * @tc.name      : create source with g_fd,AVI_MPEG2_422P@high_1920_1080_PCM_s24_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2700, TestSize.Level3)
{
    DemuxerResultRaw(INP_DIR_12);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2800
 * @tc.name      : create source with g_fd,AVI_MPEG4_simple@level5_720_480_AAC_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2800, TestSize.Level3)
{
    DemuxerResult(INP_DIR_13);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_2900
 * @tc.name      : create source with g_fd,AVI_MPEG4_advanced_simple@level6_1280_720_MP2_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_2900, TestSize.Level3)
{
    DemuxerResult(INP_DIR_14);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3000
 * @tc.name      : create source with g_fd,AVI_MPEG4_advanced_simple@level3_352_288_MP3_2.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3000, TestSize.Level3)
{
    DemuxerResult(INP_DIR_15);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3100
 * @tc.name      : create source with g_fd,AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3100, TestSize.Level3)
{
    DemuxerResultRaw(INP_DIR_16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3200
 * @tc.name      : seek to the start time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 39};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 0, 28, 39};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 19};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 44};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 42};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 0, 26, 14};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 0, 12, 21};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 47};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 39};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 43};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 17};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 45};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 42};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_PREVIOUS_SYNC, 0, 24, 14};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_PREVIOUS_SYNC, 0, 30, 44};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3300
 * @tc.name      : seek to the start time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 0, 30, 39};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 0, 28, 39};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 0, 30, 19};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 0, 29, 44};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 0, 30, 42};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 0, 26, 14};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 0, 12, 21};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 0, 30, 47};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_NEXT_SYNC, 0, 30, 39};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_NEXT_SYNC, 0, 30, 43};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_NEXT_SYNC, 0, 30, 17};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_NEXT_SYNC, 0, 29, 45};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_NEXT_SYNC, 0, 30, 42};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_NEXT_SYNC, 0, 24, 14};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_NEXT_SYNC, 0, 30, 44};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3400
 * @tc.name      : seek to the start time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 0, 30, 39};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 0, 28, 39};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 0, 30, 19};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 0, 29, 44};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 0, 30, 42};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 0, 26, 14};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 0, 12, 21};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 0, 30, 47};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_CLOSEST_SYNC, 0, 30, 39};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_CLOSEST_SYNC, 0, 30, 43};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_CLOSEST_SYNC, 0, 30, 17};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_CLOSEST_SYNC, 0, 29, 45};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_CLOSEST_SYNC, 0, 30, 42};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_CLOSEST_SYNC, 0, 24, 14};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_CLOSEST_SYNC, 0, 30, 44};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3500
 * @tc.name      : seek to the end time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 1};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 983333, 1, 1};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 2};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 1301300, 6, 4};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 966667, 29, 44};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 966667, 30, 42};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 966667, 26, 14};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 400000, 12, 21};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 966667, 6, 1};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_PREVIOUS_SYNC, 966667, 6, 8};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_PREVIOUS_SYNC, 1000000, 6, 8};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_PREVIOUS_SYNC, 1366667, 6, 1};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_PREVIOUS_SYNC, 1000000, 5, 8};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_PREVIOUS_SYNC, 983333, 6, 8};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_PREVIOUS_SYNC, 966667, 12, 6};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_PREVIOUS_SYNC, 983333, 6, 9};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3600
 * @tc.name      : seek to the end time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 966667, 1, 1};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 983333, 1, 1};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 966667, 1, 2};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 1301300, 0, 0};
    CheckSeekModefromEnd(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 966667, 0, 0};
    CheckSeekModefromEnd(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 966667, 0, 0};
    CheckSeekModefromEnd(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 966667, 0, 0};
    CheckSeekModefromEnd(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 400000, 0, 0};
    CheckSeekModefromEnd(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 966667, 0, 0};
    CheckSeekModefromEnd(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_NEXT_SYNC, 966667, 0, 0};
    CheckSeekModefromEnd(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_NEXT_SYNC, 1000000, 0, 0};
    CheckSeekModefromEnd(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_NEXT_SYNC, 1366667, 0, 0};
    CheckSeekModefromEnd(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_NEXT_SYNC, 1000000, 0, 0};
    CheckSeekModefromEnd(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_NEXT_SYNC, 983333, 0, 0};
    CheckSeekModefromEnd(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_NEXT_SYNC, 966667, 0, 0};
    CheckSeekModefromEnd(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_NEXT_SYNC, 983333, 0, 0};
    CheckSeekModefromEnd(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3700
 * @tc.name      : seek to the end time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 1};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 983333, 1, 1};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 2};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 1301300, 6, 4};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 966667, 29, 44};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 966667, 30, 42};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 966667, 26, 14};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 400000, 12, 21};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 966667, 6, 1};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_CLOSEST_SYNC, 966667, 6, 8};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_CLOSEST_SYNC, 1008000, 6, 8};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_CLOSEST_SYNC, 1366667, 6, 1};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_CLOSEST_SYNC, 1000000, 5, 8};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_CLOSEST_SYNC, 983333, 6, 8};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_CLOSEST_SYNC, 966667, 12, 6};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_CLOSEST_SYNC, 983333, 6, 9};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3800
 * @tc.name      : seek to the middle time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 1};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 500000, 16, 20};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 20};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 600000, 30, 19};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 500000, 29, 44};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 500000, 30, 42};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 500000, 26, 14};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 200000, 12, 21};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 500000, 18, 1};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_PREVIOUS_SYNC, 500000, 18, 24};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_PREVIOUS_SYNC, 500000, 18, 25};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_PREVIOUS_SYNC, 600000, 30, 17};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_PREVIOUS_SYNC, 500000, 17, 25};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_PREVIOUS_SYNC, 500000, 18, 25};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_PREVIOUS_SYNC, 500000, 24, 14};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_PREVIOUS_SYNC, 500000, 18, 27};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_3900
 * @tc.name      : seek to the middle time, next mode
 * @tc.desc      : function testZHE
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_3900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 500000, 15, 1};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 500000, 15, 19};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 500000, 15, 20};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 600000, 18, 9};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 500000, 0, 0};
    CheckSeekModefromEnd(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 500000, 0, 0};
    CheckSeekModefromEnd(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 500000, 0, 0};
    CheckSeekModefromEnd(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 200000, 0, 0};
    CheckSeekModefromEnd(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 500000, 6, 1};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_NEXT_SYNC, 500000, 6, 8};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_NEXT_SYNC, 500000, 6, 8};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_NEXT_SYNC, 600000, 18, 6};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_NEXT_SYNC, 500000, 5, 8};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_NEXT_SYNC, 500000, 6, 8};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_NEXT_SYNC, 500000, 12, 6};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_NEXT_SYNC, 500000, 6, 9};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_4000
 * @tc.name      : seek to the middle time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_4000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 1};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 500000, 16, 20};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 20};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 600000, 18, 9};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 500000, 29, 44};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 500000, 30, 42};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 500000, 26, 14};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 200000, 12, 21};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 500000, 18, 1};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_10, SEEK_MODE_CLOSEST_SYNC, 500000, 18, 24};
    CheckSeekMode(fileTest10);
    seekInfo fileTest11{INP_DIR_11, SEEK_MODE_CLOSEST_SYNC, 500000, 18, 25};
    CheckSeekMode(fileTest11);
    seekInfo fileTest12{INP_DIR_12, SEEK_MODE_CLOSEST_SYNC, 600000, 18, 6};
    CheckSeekMode(fileTest12);
    seekInfo fileTest13{INP_DIR_13, SEEK_MODE_CLOSEST_SYNC, 500000, 17, 25};
    CheckSeekMode(fileTest13);
    seekInfo fileTest14{INP_DIR_14, SEEK_MODE_CLOSEST_SYNC, 500000, 18, 25};
    CheckSeekMode(fileTest14);
    seekInfo fileTest15{INP_DIR_15, SEEK_MODE_CLOSEST_SYNC, 500000, 12, 6};
    CheckSeekMode(fileTest15);
    seekInfo fileTest16{INP_DIR_16, SEEK_MODE_CLOSEST_SYNC, 500000, 18, 27};
    CheckSeekMode(fileTest16);
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_4100
 * @tc.name      : demuxer damaged avi video file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_4100, TestSize.Level2)
{
    int tarckType = 0;
    bool readingFailed = false;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    const char *file = "/data/test/media/error.avi";
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            int readResult = OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr);
            if (readResult != AV_ERR_OK) {
                cerr << "Failed to read sample for track " << index << ", error code: " << ret << endl;
                readingFailed = true;
                break;
            }
        }
        if (readingFailed) {
            break;
        }
    }
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_4200
 * @tc.name      : demux avi, zero track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_4200, TestSize.Level2)
{
    const char *file = "/data/test/media/zero_track.avi";
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 0);

    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_AVI_FUNC_4300
 * @tc.name      : seek to a invalid time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_4300, TestSize.Level2)
{
    const char *file = INP_DIR_12;
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
 * @tc.number    : DEMUXER_AVI_FUNC_4400
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_4400, TestSize.Level2)
{
    const char *file = INP_DIR_7;
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
 * @tc.number    : DEMUXER_AVI_FUNC_4500
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_4500, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char *file = INP_DIR_13;
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
    ASSERT_EQ(g_trackCount, TWO);
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
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 1));
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
 * @tc.number    : DEMUXER_AVI_FUNC_4600
 * @tc.name      : start demux before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_FUNC_4600, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = INP_DIR_2;
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
 * @tc.number    : DEMUXER_AVI_ILLEGAL_PARA_0100
 * @tc.name      : input invalid avi g_fd file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_ILLEGAL_PARA_0100, TestSize.Level2)
{
    const char *file = "/data/test/media/invalid.avi";
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_EQ(source, nullptr);
}

/**
 * @tc.number    : DEMUXER_AVI_ILLEGAL_PARA_0200
 * @tc.name      : input invalid avi uri file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAviFuncNdkTest, DEMUXER_AVI_ILLEGAL_PARA_0200, TestSize.Level2)
{
    const char *uri = "http://192.168.3.11:8080/share/invalid.avi";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(source, nullptr);
}
