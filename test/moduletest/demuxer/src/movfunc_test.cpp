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
class DemuxerMovFuncNdkTest : public testing::Test {
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
    const char *INP_DIR_ADPCM_YAMAHA = "/data/test/media/adpcm_yamaha.mov";
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
static uint8_t g_track0 = 0;
static uint8_t g_track1 = 1;
static uint8_t g_track2 = 2;
static uint8_t g_track3 = 3;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
void DemuxerMovFuncNdkTest::SetUpTestCase() {}
void DemuxerMovFuncNdkTest::TearDownTestCase() {}
void DemuxerMovFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerMovFuncNdkTest::TearDown()
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

struct seekInfo {
    const char *fileName;
    OH_AVSeekMode seekmode;
    int64_t millisecond;
    int32_t videoCount;
    int32_t audioCount;
};

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

static void OpenFile(const char *fileName, int fd, OH_AVSource **src, OH_AVDemuxer **Demuxer)
{
    int64_t size = GetFileSize(fileName);
    cout << fileName << "----------------------" << fd << "---------" << size << endl;
    *src = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(*src, nullptr);

    *Demuxer = OH_AVDemuxer_CreateWithSource(*src);
    ASSERT_NE(*Demuxer, nullptr);
}

static void OpenSourceFormat(const char *fileName, int fd, OH_AVSource **src, OH_AVFormat **srcFormat)
{
    int64_t size = GetFileSize(fileName);
    cout << fileName << "----------------------" << fd << "---------" << size << endl;
    *src = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(*src, nullptr);

    *srcFormat = OH_AVSource_GetSourceFormat(*src);
    ASSERT_NE(*srcFormat, nullptr);
}

static void CheckTrackCount(OH_AVFormat **srcFormat, OH_AVSource *src, int32_t *trackCount, int trackNum)
{
    *srcFormat = OH_AVSource_GetSourceFormat(src);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(*srcFormat, OH_MD_KEY_TRACK_COUNT, trackCount));
    ASSERT_EQ(trackNum, *trackCount);
}

static void CheckTrackSelect()
{
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
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

static void CheckFrames(int videoFrameNum, int videoKey, int audioFrameNum, int audioKey)
{
    int trackType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    int vKeyCount = 0;
    int aKeyCount = 0;

    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (trackType == MEDIA_TYPE_AUD)) || (videoIsEnd && (trackType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (trackType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            } else if (trackType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, audioFrameNum);
    ASSERT_EQ(aKeyCount, audioKey);
    ASSERT_EQ(videoFrame, videoFrameNum);
    ASSERT_EQ(vKeyCount, videoKey);
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
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / 1000, seekInfo.seekmode));
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
    ASSERT_EQ(g_trackCount, 2);
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

static void InitTrkPara(int index, int *trackType)
{
    trackFormat = OH_AVSource_GetTrackFormat(source, index);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, trackType));
    OH_AVFormat_Destroy(trackFormat);
    trackFormat = nullptr;
}

static void FramesMultiTrks(int vFrameNum, int videoKey, int aFrameNum, int audioKey)
{
    OH_AVCodecBufferAttr attr;
    int aKeyCount = 0;
    int vKeyCount = 0;
    int audioFrame = 0;
    int videoFrame = 0;
    int trackType = 0;
    bool trackEndFlag[4] = {0, 0, 0, 0};
    bool allEnd = false;
    while (!allEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            InitTrkPara(index, &trackType);
            if ((trackEndFlag[index] && (trackType == MEDIA_TYPE_AUD)) ||
                (trackEndFlag[index] && (trackType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (trackType == MEDIA_TYPE_AUD &&
                (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                trackEndFlag[index] = true;
            } else if (trackType == MEDIA_TYPE_AUD &&
                (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME)) {
                aKeyCount++;
                audioFrame++;
            } else if (trackType == MEDIA_TYPE_AUD) {
                audioFrame++;
            }

            if (trackType == MEDIA_TYPE_VID &&
                (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                trackEndFlag[index] = true;
            } else if (trackType == MEDIA_TYPE_VID &&
                (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME)) {
                vKeyCount++;
                videoFrame++;
            } else if (trackType == MEDIA_TYPE_VID) {
                videoFrame++;
            }
        }
        allEnd = trackEndFlag[g_track0] && trackEndFlag[g_track1] &&
                     trackEndFlag[g_track2] && trackEndFlag[g_track3];
    }

    ASSERT_EQ(audioFrame, aFrameNum);
    ASSERT_EQ(videoFrame, vFrameNum);
    ASSERT_EQ(aKeyCount, audioKey);
    ASSERT_EQ(vKeyCount, videoKey);
}

static void CheckMOVVideoKey()
{
    int64_t bitrate = 0;
    const char* mimeType = nullptr;
    double frameRate;
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
    int bitrateResult = 750752;
    ASSERT_EQ(bitrateResult, bitrate);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectNum = 0;
    ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_AVC));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    int frameRateResult = 30.000000;
    ASSERT_EQ(frameRateResult, frameRate);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    int currentHeightResult = 1080;
    ASSERT_EQ(currentHeightResult, currentHeight);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &currentWidth));
    int currentWidthResult = 1920;
    ASSERT_EQ(currentWidthResult, currentWidth);
}

static void CheckMOVAudioKey()
{
    int32_t audioCount = 0;
    int64_t bitrate = 0;
    const char* mimeType = nullptr;
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &audioCount));
    int audioCountResult = 1;
    ASSERT_EQ(audioCountResult, audioCount);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
    int bitrateResult = 70005;
    ASSERT_EQ(bitrateResult, bitrate);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectNum = 0;
    ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_AAC));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    int sampleRateResult = 48000;
    ASSERT_EQ(sampleRateResult, sampleRate);
}

static void CheckMPGVideoKey()
{
    int64_t bitrate = 0;
    const char* mimeType = nullptr;
    double frameRate;
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectNum = 0;
    ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_AVC));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    int frameRateResult = 30.000000;
    ASSERT_EQ(frameRateResult, frameRate);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    int currentHeightResult = 1080;
    ASSERT_EQ(currentHeightResult, currentHeight);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &currentWidth));
    int currentWidthResult = 1920;
    ASSERT_EQ(currentWidthResult, currentWidth);
}

static void CheckMPGAudioKey()
{
    int32_t audioCount = 0;
    int64_t bitrate = 0;
    const char* mimeType = nullptr;
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &audioCount));
    int audioCountResult = 1;
    ASSERT_EQ(audioCountResult, audioCount);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
    int bitrateResult = 384000;
    ASSERT_EQ(bitrateResult, bitrate);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectNum = 0;
    ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_MPEG));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    int sampleRateResult = 44100;
    ASSERT_EQ(sampleRateResult, sampleRate);
}

/**
 * @tc.number    : DEMUXER_MOV_ADPCM_YAMAHA_FUNC_0100
 * @tc.name      : demuxer MOV, audio track adpcm yamaha
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, DEMUXER_MOV_ADPCM_YAMAHA_FUNC_0100, TestSize.Level2)
{
    DemuxerResult(INP_DIR_ADPCM_YAMAHA);
}

/**
 * @tc.number    : DEMUXER_MOV_ADPCM_YAMAHA_FUNC_0200
 * @tc.name      : demuxer MOV, audio track adpcm yamaha check seek mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, DEMUXER_MOV_ADPCM_YAMAHA_FUNC_0200, TestSize.Level2)
{
    seekInfo fileTest1{INP_DIR_ADPCM_YAMAHA, SEEK_MODE_NEXT_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_ADPCM_YAMAHA, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_ADPCM_YAMAHA, SEEK_MODE_CLOSEST_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest3);
}

/**
 * @tc.number    : DEMUXER_MOV_ADPCM_YAMAHA_FUNC_0300
 * @tc.name      : demuxer MOV, audio track adpcm yamaha get track format
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, DEMUXER_MOV_ADPCM_YAMAHA_FUNC_0300, TestSize.Level2)
{
    const char *mimeType = nullptr;
    const char *file = INP_DIR_ADPCM_YAMAHA;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(memcmp(mimeType, "audio/adpcm_yamaha", strlen("audio/adpcm_yamaha")), 0);
    int32_t channel = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channel));
    ASSERT_EQ(channel, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000); // 48000: source sample rate
    close(g_fd);
    g_fd = -1;
}


/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0100
 * @tc.name      : demux H264_base@5_1920_1080_30_AAC_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0100, TestSize.Level0)
{
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 95, 95);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0200
 * @tc.name      : demux H264_main@4_1280_720_60_MP2_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0200, TestSize.Level0)
{
    const char *file = "/data/test/media/H264_main@4_1280_720_60_MP2_44.1K_2.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 77, 77);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0300
 * @tc.name      : demux H264_high@5.1_3840_2160_30_MP3_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0300, TestSize.Level0)
{
    const char *file = "/data/test/media/H264_high@5.1_3840_2160_30_MP3_48K_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 85, 85);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0400
 * @tc.name      : demux H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0400, TestSize.Level2)
{
    const char *file = "/data/test/media/H264_h444@5.1_1920_1080_60_vorbis_32K_2.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 72, 72);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0500
 * @tc.name      : demux H264_main@3_720_480_30_PCM_48K_24_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0500, TestSize.Level0)
{
    const char *file = "/data/test/media/H264_main@3_720_480_30_PCM_48K_24_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 144, 144);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0600
 * @tc.name      : demux H265_main@4_1920_1080_30_AAC_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0600, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    const char *file = "/data/test/media/H265_main@4_1920_1080_30_AAC_44.1K_2.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 88, 88);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0700
 * @tc.name      : demux H265_main10@4.1_1280_720_60_MP2_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0700, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    const char *file = "/data/test/media/H265_main10@4.1_1280_720_60_MP2_48K_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 84, 84);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0800
 * @tc.name      : demux H265_main10@5_1920_1080_60_MP3_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0800, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    const char *file = "/data/test/media/H265_main10@5_1920_1080_60_MP3_44.1K_2.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 78, 78);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_0900
 * @tc.name      : demux H265_main@5_3840_2160_30_vorbis_48K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_0900, TestSize.Level0)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    const char *file = "/data/test/media/H265_main@5_3840_2160_30_vorbis_48K_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 96, 96);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1000
 * @tc.name      : demux H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1000, TestSize.Level2)
{
    if (access(HEVC_LIB_PATH.c_str(), F_OK) != 0) {
        return;
    }
    const char *file = "/data/test/media/H265_main10@5.1_3840_2160_60_PCM(mulaw)_48K_16_2.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 171, 171);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1100
 * @tc.name      : demux MPEG4_SP@5_720_480_30_AAC_32K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1100, TestSize.Level0)
{
    const char *file = "/data/test/media/MPEG4_SP@5_720_480_30_AAC_32K_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 64, 64);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1200
 * @tc.name      : demux MPEG4_SP@6_1280_720_30_MP2_32K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1200, TestSize.Level0)
{
    const char *file = "/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 56, 56);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1300
 * @tc.name      : demux MPEG4_ASP@3_352_288_30_MP3_32K_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1300, TestSize.Level2)
{
    const char *file = "/data/test/media/MPEG4_ASP@3_352_288_30_MP3_32K_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 57, 57);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1400
 * @tc.name      : demux MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1400, TestSize.Level2)
{
    const char *file = "/data/test/media/MPEG4_ASP@4_720_576_30_Vorbis_44.1K_2.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 91, 91);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1500
 * @tc.name      : demux MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1500, TestSize.Level0)
{
    const char *file = "/data/test/media/MPEG4_Core@2_1920_1080_30_PCM(mulaw)_44.1K_16_1.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(45, 5, 65, 65);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1600
 * @tc.name      : create source with invalid fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1600, TestSize.Level2)
{
    const char *file = "/data/test/media/invalid.mov";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1700
 * @tc.name      : create source with invalid uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1700, TestSize.Level2)
{
    const char *uri = "http://invalidPath/invalid.mov";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_1900
 * @tc.name      : demuxer mov  ,check source format,OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_1900, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "title"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2000
 * @tc.name      : demuxer mov  ,check source format,OH_MD_KEY_ALBUM
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2000, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "album"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2100
 * @tc.name      : demuxer mov  ,check source format,OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2100, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2200
 * @tc.name      : demuxer mov  ,check track format,OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2200, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "date"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2300
 * @tc.name      : demuxer mov  ,check track format,OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2300, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "comment"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2400
 * @tc.name      : demuxer mov  ,check track format,OH_MD_KEY_GENRE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2400, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_GENRE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "genre"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2500
 * @tc.name      : demuxer mov  ,check track format,OH_MD_KEY_COPYRIGHT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2500, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COPYRIGHT, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "copyright"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2600
 * @tc.name      : demuxer mov  ,check track format,OH_MD_KEY_LANGUAGE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2600, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_LANGUAGE, &stringVal));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2700
 * @tc.name      : demuxer mov  ,check track format,OH_MD_KEY_DESCRIPTION
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2700, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DESCRIPTION, &stringVal));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2800
 * @tc.name      : demuxer mov  ,check track format,OH_MD_KEY_LYRICS
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2800, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_LYRICS, &stringVal));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_2900
 * @tc.name      : demuxer mov  ,check source format,OH_MD_KEY_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_2900, TestSize.Level2)
{
    const char *file = "/data/test/media/meta_test.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    const char *stringVal;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "artist"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3000
 * @tc.name      : demuxer mov file, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3000, TestSize.Level2)
{
    int trackType = 0;
    int64_t duration = 0;
    int64_t startTime;
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    ASSERT_NE(OH_AVSource_GetTrackFormat(source, 0), nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(2000000, duration);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_START_TIME, &startTime));
    ASSERT_EQ(0, startTime);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
	
    const char* mimeType = nullptr;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
	
    OH_AVCodecBufferAttr attr;
    int vKeyCount = 0;
    int aKeyCount = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;

    for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
            if ((audioIsEnd && (trackType == MEDIA_TYPE_AUD) && index == MEDIA_TYPE_AUD) ||
             (videoIsEnd && (trackType == MEDIA_TYPE_VID) && index == MEDIA_TYPE_VID)) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
            if (trackType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
                CheckMOVAudioKey();
            } else if (trackType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                CheckMOVVideoKey();
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
        }

    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3100
 * @tc.name      : demux multi track mov file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3100, TestSize.Level2)
{
    const char *file = "/data/test/media/multi_trk.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 4);
    CheckTrackSelect();
    FramesMultiTrks(120, 12, 145, 145);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3200
 * @tc.name      : create source with error mov file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3200, TestSize.Level3)
{
    const char *file = "/data/test/media/error.mov";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
	
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    OH_AVCodecBufferAttr attr;
    ASSERT_NE(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
	
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3300
 * @tc.name      : demux mov , zero track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3300, TestSize.Level3)
{
    const char *file = "/data/test/media/zero_track.mov";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 0);

    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3400
 * @tc.name      : seek to a invalid time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3400, TestSize.Level3)
{
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov";
    srand(time(nullptr));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));

    ASSERT_NE(demuxer, nullptr);
    int64_t invalidPts = 12000 * 16666;
    ret = OH_AVDemuxer_SeekToTime(demuxer, invalidPts, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_NE(ret, AV_ERR_OK);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3500
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3500, TestSize.Level3)
{
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov";
    srand(time(nullptr));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_UnselectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3600
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3600, TestSize.Level3)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov";
    bool isEnd = false;
    int count = 0;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    srand(time(nullptr));
    int pos = rand() % 60;
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
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MOV_DEMUXER_FUNCTION_TEST_3700
 * @tc.name      : start demux bufore add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MOV_DEMUXER_FUNCTION_TEST_3700, TestSize.Level3)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_AAC_48K_1.mov";
    srand(time(nullptr));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0100
 * @tc.name      : demux H264_base@5_1920_1080_30_MP2_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0100, TestSize.Level0)
{
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 77, 77);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0200
 * @tc.name      : demux H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0200, TestSize.Level2)
{
    const char *file = "/data/test/media/H264_h444p@5.1_1920_1080_60_MP2_48K_2.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 84, 84);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0300
 * @tc.name      : demux H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0300, TestSize.Level2)
{
    const char *file = "/data/test/media/H264_high@5.1_3840_2160_30_MP3_44.1K_1.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 78, 78);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0400
 * @tc.name      : demux H264_main@4.2_1280_720_60_MP3_32K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0400, TestSize.Level0)
{
    const char *file = "/data/test/media/H264_main@4.2_1280_720_60_MP3_32K_2.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 57, 57);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0500
 * @tc.name      : demux MPEG2_422p_1280_720_60_MP3_32K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0500, TestSize.Level2)
{
    const char *file = "/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(119, 12, 57, 57);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0600
 * @tc.name      : demux MPEG2_high_720_480_30_MP2_32K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0600, TestSize.Level0)
{
    const char *file = "/data/test/media/MPEG2_high_720_480_30_MP2_32K_2.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 56, 56);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0700
 * @tc.name      : demux MPEG2_main_352_288_30_MP2_44.1K_1.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0700, TestSize.Level0)
{
    const char *file = "/data/test/media/MPEG2_main_352_288_30_MP2_44.1K_1.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 77, 77);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0800
 * @tc.name      : demux MPEG2_main_1920_1080_30_MP3_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0800, TestSize.Level0)
{
    const char *file = "/data/test/media/MPEG2_main_1920_1080_30_MP3_48K_2.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(60, 6, 85, 85);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_0900
 * @tc.name      : demux MPEG2_simple_320_240_24_MP3_48K_2.mpg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_0900, TestSize.Level0)
{
    const char *file = "/data/test/media/MPEG2_simple_320_240_24_MP3_48K_2.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(48, 5, 85, 85);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1100
 * @tc.name      : demuxer mpg file, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1100, TestSize.Level2)
{
    int trackType = 0;
    int64_t duration = 0;
    int64_t startTime;
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
    int fd = open(file, O_RDONLY);
    OpenSourceFormat(file, fd, &source, &sourceFormat);
    ASSERT_NE(OH_AVSource_GetTrackFormat(source, 0), nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(2011433, duration);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_START_TIME, &startTime));
    ASSERT_EQ(500000, startTime);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    const char* mimeType = nullptr;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    OH_AVCodecBufferAttr attr;
    int vKeyCount = 0;
    int aKeyCount = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;

    for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
            if ((audioIsEnd && (trackType == MEDIA_TYPE_AUD) && index == MEDIA_TYPE_AUD) ||
             (videoIsEnd && (trackType == MEDIA_TYPE_VID) && index == MEDIA_TYPE_VID)) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
            if (trackType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
                CheckMPGAudioKey();
            } else if (trackType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                CheckMPGVideoKey();
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
        }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1200
 * @tc.name      : demux multi track mpg file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1200, TestSize.Level2)
{
    const char *file = "/data/test/media/multi_trk.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 4);
    CheckTrackSelect();
    FramesMultiTrks(167, 17, 142, 142);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1300
 * @tc.name      : create source with error mpg file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1300, TestSize.Level3)
{
    const char *file = "/data/test/media/error.mpg";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    OH_AVCodecBufferAttr attr;
    ASSERT_NE(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1400
 * @tc.name      : demux mpg , zero track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1400, TestSize.Level3)
{
    const char *file = "/data/test/media/zero_track.mpg";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 0);

    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1500
 * @tc.name      : seek to a invalid time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1500, TestSize.Level3)
{
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
    srand(time(nullptr));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));

    ASSERT_NE(demuxer, nullptr);
    int64_t invalidPts = 12000 * 16666;
    ret = OH_AVDemuxer_SeekToTime(demuxer, invalidPts, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_NE(ret, AV_ERR_OK);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1600
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1600, TestSize.Level3)
{
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
    srand(time(nullptr));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_UnselectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1700
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1700, TestSize.Level3)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
    bool isEnd = false;
    int count = 0;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    srand(time(nullptr));
    int pos = rand() % 60;
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
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MPG_DEMUXER_FUNCTION_TEST_1800
 * @tc.name      : start demux bufore add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMovFuncNdkTest, MPG_DEMUXER_FUNCTION_TEST_1800, TestSize.Level3)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/H264_base@5_1920_1080_30_MP2_44.1K_1.mpg";
    srand(time(nullptr));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(fd);
    fd = -1;
}