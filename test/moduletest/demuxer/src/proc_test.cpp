/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
class DemuxerProcNdkTest : public testing::Test {
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
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;

void DemuxerProcNdkTest::SetUpTestCase() {}
void DemuxerProcNdkTest::TearDownTestCase() {}
void DemuxerProcNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerProcNdkTest::TearDown()
{
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }

    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
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

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1400
 * @tc.name      : demuxer video and 2 audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1400, TestSize.Level0)
{
    int tarckType = 0;
    int auidoTrackCount = 2;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/video_2audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(auidoTrackCount + 1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount[2] = {};
    int audioFrame[2] = {};
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == 1) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == 0) {
                SetAudioValue(attr, audioIsEnd, audioFrame[index-1], aKeyCount[index-1]);
            }
        }
    }
    for (int index = 0; index < auidoTrackCount; index++) {
        ASSERT_EQ(audioFrame[index], 433);
        ASSERT_EQ(aKeyCount[index], 433);
    }
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1500
 * @tc.name      : demuxer video and 9 audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1500, TestSize.Level0)
{
    int tarckType = 0;
    int auidoTrackCount = 9;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/video_9audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(auidoTrackCount + 1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount[9] = {};
    int audioFrame[9] = {};
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == 1) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == 0) {
                SetAudioValue(attr, audioIsEnd, audioFrame[index-1], aKeyCount[index-1]);
            }
        }
    }
    for (int index = 0; index < auidoTrackCount; index++) {
        ASSERT_EQ(audioFrame[index], 433);
        ASSERT_EQ(aKeyCount[index], 433);
    }
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1600
 * @tc.name      : demuxer avc+MP3 flv video file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1600, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/avc_mp3.flv";
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
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == 1) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == 0) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 385);
    ASSERT_EQ(aKeyCount, 385);
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1700
 * @tc.name      : demuxer hevc+pcm flv video file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1700, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/hevc_pcm_a.flv";
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
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == 1) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == 0) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 385);
    ASSERT_EQ(aKeyCount, 385);
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1800
 * @tc.name      : demuxer damaged flv video file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1800, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/avc_mp3_error.flv";
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
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == 1) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == 0) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1900
 * @tc.name      : demuxer damaged ape audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1900, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/ape.ape";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int aKeyCount = 0;
    while (!audioIsEnd) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
    }
    ASSERT_EQ(audioFrame, 8);
    ASSERT_EQ(aKeyCount, 8);
    close(fd);
}