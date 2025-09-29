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

#include <sys/stat.h>
#include <cinttypes>
#include <fstream>
#include "media_description.h"
#include "gtest/gtest.h"
#include "meta/format.h"
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

#include "meta/meta_key.h"
#include "meta/meta.h"
#include "av_common.h"
#include "gtest/gtest.h"

namespace OHOS {
namespace Media {
class DemuxerMtsFuncNdkTest : public testing::Test {
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
static uint8_t g_track0 = 0;
static uint8_t g_track1 = 1;
static uint8_t g_track2 = 2;
static uint8_t g_track3 = 3;
static OH_AVErrCode ret = AV_ERR_OK;
static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
static int32_t g_trackCount;
static int g_tarckType = 0;
constexpr int32_t SEEKTIMES = 10;
constexpr int32_t THOUSAND = 1000.0;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
void DemuxerMtsFuncNdkTest::SetUpTestCase() {}
void DemuxerMtsFuncNdkTest::TearDownTestCase() {}
void DemuxerMtsFuncNdkTest::SetUp()
{
    avBuffer = OH_AVBuffer_Create(g_width * g_height);
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerMtsFuncNdkTest::TearDown()
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
std::random_device g_mtsRdm;

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
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
        bool readEnd = false;
        int32_t frameNum = 0;
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                readEnd = true;
                break;
            }
            frameNum++;
        }
        if (g_tarckType == MEDIA_TYPE_VID) {
            ASSERT_EQ(seekInfo.videoCount, frameNum);
            cout << "---g_tarckType---" << g_tarckType << endl;
        } else if (g_tarckType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(seekInfo.audioCount, frameNum);
            cout << "---g_tarckType---" << g_tarckType << endl;
        } else if (g_tarckType == MEDIA_TYPE_SUBTITLE) {
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
}

static void CheckSeekResult(const char *fileName, uint32_t seekCount)
{
    static int64_t duration = 0;
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
    srand(time(nullptr));
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        for (int32_t i = 0; i < seekCount; i++) {
            if (duration != 0) {
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, (g_mtsRdm() % duration) / THOUSAND,
                (OH_AVSeekMode)((g_mtsRdm() % 1) +1)));
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
        }
    }
    close(fd);
    fd = -1;
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
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
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

static void CheckTrackCount(OH_AVFormat **srcFormat, OH_AVSource *src, int32_t *trackCount, int trackNum)
{
    *srcFormat = OH_AVSource_GetSourceFormat(src);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(*srcFormat, OH_MD_KEY_TRACK_COUNT, trackCount));
    ASSERT_EQ(trackNum, *trackCount);
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
        cout << "audio track !!!!!" << endl;
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
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
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

static void CheckMTSVideoKey()
{
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    double frameRate;
    const char* mimeType = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &currentWidth));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    int expectWidth = 1920;
    ASSERT_EQ(expectWidth, currentWidth);
    int expectHeight = 1080;
    ASSERT_EQ(expectHeight, currentHeight);
    ASSERT_EQ(0, strcmp(mimeType, "video/mpeg2"));
    int expectframeRate = 30;
    ASSERT_EQ(expectframeRate, frameRate);
}

static void CheckMTSAudioKey()
{
    const char* mimeType = nullptr;
    int32_t audioCount = 0;
    int32_t sampleRate = 0;
    int sampleFormat = 0;
    int64_t br = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &audioCount));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &sampleFormat));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectaudioCount = 2;
    ASSERT_EQ(expectaudioCount, audioCount);
    int expectsampleRate = 44100;
    ASSERT_EQ(expectsampleRate, sampleRate);
    int expectsampleFormat = 9;
    ASSERT_EQ(expectsampleFormat, sampleFormat);
    ASSERT_EQ(0, strcmp(mimeType, "audio/mp4a-latm"));
    int expectbr = 100192;
    ASSERT_EQ(expectbr, br);
}

static void CheckMTSSourceKey()
{
    int64_t duration = 0;
    int currentFileType = 0;
    int32_t hasAudio = 0;
    int32_t hasVideo = 0;
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, Media::Tag::MEDIA_FILE_TYPE, &currentFileType));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, Media::Tag::MEDIA_HAS_VIDEO, &hasVideo));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, Media::Tag::MEDIA_HAS_AUDIO, &hasAudio));
    int expecthasVideo = 1;
    ASSERT_EQ(expecthasVideo, hasVideo);
    int expecthasAudio = 1;
    ASSERT_EQ(expecthasAudio, hasAudio);
    int expectcurrentFileType = 102;
    ASSERT_EQ(expectcurrentFileType, currentFileType);
    int expectduration = 10166667;
    ASSERT_EQ(expectduration, duration);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0000
 * @tc.name      : create source with invalid fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0000, TestSize.Level0)
{
    const char *file = "/data/test/media/invalid.mts";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(fd);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0100
 * @tc.name      : fd demux h264_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0100, TestSize.Level0)
{
    const char *file = "/data/test/media/h264_aac.mts";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 472, 472);
    close(fd);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0200
 * @tc.name      : fd demux h264_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0200, TestSize.Level0)
{
    const char *file = "/data/test/media/h264_mp3.mts";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 420, 420);
    close(fd);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0300
 * @tc.name      : fd demux h265_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0300, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        const char *file = "/data/test/media/h265_aac.mts";
        int fd = open(file, O_RDONLY);
        OpenFile(file, fd, &source, &demuxer);
        CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
        CheckTrackSelect();
        CheckFrames(602, 3, 472, 472);
        close(fd);
    }
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0400
 * @tc.name      : fd demux h265_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0400, TestSize.Level0)
{
    if (!access("/system/lib64/media/", 0)) {
        const char *file = "/data/test/media/h265_mp3.mts";
        int fd = open(file, O_RDONLY);
        OpenFile(file, fd, &source, &demuxer);
        CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
        CheckTrackSelect();
        CheckFrames(602, 3, 420, 420);
        close(fd);
    }
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0500
 * @tc.name      : fd demux mpeg2_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0500, TestSize.Level0)
{
    const char *file = "/data/test/media/mpeg2_aac.mts";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 434, 434);
    close(fd);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0600
 * @tc.name      : fd demux mpeg2_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0600, TestSize.Level0)
{
    const char *file = "/data/test/media/mpeg2_mp3.mts";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 387, 387);
    close(fd);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0700
 * @tc.name      : fd demux mpeg4_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0700, TestSize.Level0)
{
    const char *file = "/data/test/media/mpeg4_aac.mts";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 434, 434);
    close(fd);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0800
 * @tc.name      : fd demux mpeg4_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0800, TestSize.Level0)
{
    const char *file = "/data/test/media/mpeg4_mp3.mts";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 387, 387);
    close(fd);
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_0900
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_0900, TestSize.Level3)
{
    const char *file = "/data/test/media/h264_mp3.mts";
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
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_1000
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_1000, TestSize.Level3)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/h264_mp3.mts";
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
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            if (count == pos) {
                cout << count << " count == pos!!!!!!!!!" << endl;
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 0));
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 1));
                ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
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
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_1100
 * @tc.name      : start demux bufore add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_1100, TestSize.Level3)
{
    uint32_t trackIndex = 0;
    const char *file = "/data/test/media/h264_mp3.mts";
    srand(time(nullptr));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSampleBuffer(demuxer, trackIndex, avBuffer);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_1200
 * @tc.name      : demux multi track TS file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_1200, TestSize.Level2)
{
    const char *file = "/data/test/media/multi_trk.mts";
    int fd = open(file, O_RDONLY);
    OpenFile(file, fd, &source, &demuxer);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 4);
    CheckTrackSelect();
    FramesMultiTrks(120, 6, 145, 145);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_1300
 * @tc.name      : create source with error TS file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_1300, TestSize.Level3)
{
    const char *file = "/data/test/media/error.mts";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MTS_DEMUXER_FUNCTION_TEST_1400
 * @tc.name      : demux hevc ts video
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_FUNCTION_TEST_1400, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    int audioFrame = 0;
    int vKeyCount = 0;
    int aKeyCount = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/mpeg2_aac.mts";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    CheckMTSSourceKey();
    for (int32_t index = 0; index < 2; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < 2; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_tarckType));
        if ((audioIsEnd && (g_tarckType == MEDIA_TYPE_AUD) && index == MEDIA_TYPE_AUD)
        || (videoIsEnd && (g_tarckType == MEDIA_TYPE_VID) && index == MEDIA_TYPE_VID)) {
            continue;
        }
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
        ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
        if (g_tarckType == MEDIA_TYPE_AUD) {
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            CheckMTSAudioKey();
        } else if (g_tarckType == MEDIA_TYPE_VID) {
            SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            CheckMTSVideoKey();
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0100
 * @tc.name      : demuxer seek, mts h264 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0100, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/h264_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 552, 392, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/h264_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 552, 392, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/h264_mp3.mts", SEEK_MODE_NEXT_SYNC,
        800000, 552, 392, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/h264_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 384, 273, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/h264_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 384, 273, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/h264_mp3.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 384, 273, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/h264_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 228, 161, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/h264_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 228, 161, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/h264_mp3.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 228, 161, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0200
 * @tc.name      : demuxer seek, mts h264 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0200, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/h264_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 552, 439, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/h264_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 552, 439, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/h264_aac.mts", SEEK_MODE_NEXT_SYNC,
        800000, 552, 439, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/h264_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 384, 307, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/h264_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 384, 307, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/h264_aac.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 384, 307, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/h264_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 228, 181, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/h264_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 228, 181, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/h264_aac.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 228, 181, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0300
 * @tc.name      : demuxer seek, mts h265 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0300, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/h265_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 552, 392, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/h265_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 552, 392, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/h265_mp3.mts", SEEK_MODE_NEXT_SYNC,
        800000, 552, 392, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/h265_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 384, 273, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/h265_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 384, 273, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/h265_mp3.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 384, 273, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/h265_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 228, 161, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/h265_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 228, 161, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/h265_mp3.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 228, 161, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0400
 * @tc.name      : demuxer seek, mts h265 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0400, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/h265_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 552, 439, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/h265_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 552, 439, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/h265_aac.mts", SEEK_MODE_NEXT_SYNC,
        800000, 552, 439, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/h265_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 384, 307, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/h265_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 384, 307, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/h265_aac.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 384, 307, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/h265_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 228, 181, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/h265_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 228, 181, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/h265_aac.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 228, 181, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0500
 * @tc.name      : demuxer seek, mts mpeg2 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0500, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 278, 359, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 278, 359, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_NEXT_SYNC,
        800000, 278, 359, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 194, 247, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 194, 247, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 194, 247, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 116, 149, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 116, 149, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/mpeg2_mp3.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 116, 149, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0600
 * @tc.name      : demuxer seek, mts mpeg2 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0600, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 278, 399, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 278, 399, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_NEXT_SYNC,
        800000, 278, 399, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 194, 279, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 194, 279, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 194, 279, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 116, 169, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 116, 169, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/mpeg2_aac.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 116, 169, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0700
 * @tc.name      : demuxer seek, mts mpeg4 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0700, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 279, 359, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 279, 359, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_NEXT_SYNC,
        800000, 279, 359, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 195, 247, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 195, 247, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 195, 247, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 116, 149, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 116, 149, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/mpeg4_mp3.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 116, 149, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : MTS_DEMUXER_SEEK_TEST_0800
 * @tc.name      : demuxer seek, mts mpeg4 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, MTS_DEMUXER_SEEK_TEST_0800, TestSize.Level0)
{
    seekInfo fileTest1{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        800000, 279, 399, 0};
    CheckSeekMode(fileTest1);
    cout << "-----------------fileTest1_finish-----------------"<< endl;
    seekInfo fileTest2{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        800000, 279, 399, 0};
    CheckSeekMode(fileTest2);
    cout << "-----------------fileTest2_finish-----------------"<< endl;
    seekInfo fileTest3{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_NEXT_SYNC,
        800000, 279, 399, 0};
    CheckSeekMode(fileTest3);
    cout << "-----------------fileTest3_finish-----------------"<< endl;
    seekInfo fileTest4{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        3600000, 195, 279, 0};
    CheckSeekMode(fileTest4);
    cout << "-----------------fileTest4_finish-----------------"<< endl;
    seekInfo fileTest5{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        3600000, 195, 279, 0};
    CheckSeekMode(fileTest5);
    cout << "-----------------fileTest5_finish-----------------"<< endl;
    seekInfo fileTest6{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_NEXT_SYNC,
        3600000, 195, 279, 0};
    CheckSeekMode(fileTest6);
    cout << "-----------------fileTest6_finish-----------------"<< endl;
    seekInfo fileTest7{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_PREVIOUS_SYNC,
        6200000, 116, 169, 0};
    CheckSeekMode(fileTest7);
    cout << "-----------------fileTest7_finish-----------------"<< endl;
    seekInfo fileTest8{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_CLOSEST_SYNC,
        6200000, 116, 169, 0};
    CheckSeekMode(fileTest8);
    cout << "-----------------fileTest8_finish-----------------"<< endl;
    seekInfo fileTest9{"/data/test/media/mpeg4_aac.mts", SEEK_MODE_NEXT_SYNC,
        6200000, 116, 169, 0};
    CheckSeekMode(fileTest9);
    cout << "-----------------fileTest9_finish-----------------"<< endl;
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0100
 * @tc.name      : demuxer random seek, mts + h264 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0100, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h264_mp3.mts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0200
 * @tc.name      : demuxer random seek, mts + h264 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0200, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h264_aac.mts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0300
 * @tc.name      : demuxer random seek, mts + h265 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0300, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h265_mp3.mts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0400
 * @tc.name      : demuxer random seek, mts + h265 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0400, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/h265_aac.mts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0500
 * @tc.name      : demuxer random seek, mts + MPEG2 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0500, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/mpeg2_mp3.mts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0600
 * @tc.name      : demuxer random seek, mts + MPEG2 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0600, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/mpeg2_aac.mts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0700
 * @tc.name      : demuxer random seek, mts + MPEG4 + MP3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0700, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/mpeg4_mp3.mts", SEEKTIMES);
}

/**
 * @tc.number    : DEMUXER_MTS_RANDOM_SEEK_0800
 * @tc.name      : demuxer random seek, mts + MPEG4 + AAC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerMtsFuncNdkTest, DEMUXER_MTS_RANDOM_SEEK_0800, TestSize.Level2)
{
    CheckSeekResult("/data/test/media/mpeg4_aac.mts", SEEKTIMES);
}

