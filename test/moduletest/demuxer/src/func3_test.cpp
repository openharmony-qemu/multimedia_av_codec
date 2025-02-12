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
class DemuxerFunc3NdkTest : public testing::Test {
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
constexpr int FLV_AUDIONUM_AAC = 5148;
constexpr int FLV_AUDIONUM_HEVC_AAC = 210;
constexpr int FLV_VIDEONUM_HEVC_AAC_ALL = 602;
constexpr int FLV_AUDIONUM_AVC_AAC = 318;
void DemuxerFunc3NdkTest::SetUpTestCase() {}
void DemuxerFunc3NdkTest::TearDownTestCase() {}
void DemuxerFunc3NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerFunc3NdkTest::TearDown()
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

static void InitFile(const char *file, int32_t trackNum, int &fd, bool &initResult)
{
    fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(trackNum, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    initResult = true;
}

static bool CheckVideoSyncFrame(int &vKeyCount, int32_t &gopCount, int32_t &count)
{
    if (gopCount == 0 && vKeyCount == count + 1) {
        return true;
    } else if (gopCount % 60 == 0 && vKeyCount == count + 1) {
        return true;
    } else if (gopCount % 60 != 0 && vKeyCount == count){
        return true;
    } else {
        return false;
    }
}
/**
 * @tc.number    : DEMUXER_FUNCTION_FLV_0010
 * @tc.name      : create source with fd, avc+aac gop60 flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_FLV_0010, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/avc_aac_60.flv";
    int fd = 0;
    bool initResult = false;
    InitFile(file, 2, fd, initResult);
    ASSERT_TRUE(initResult);
    int aKeyCount = 0;
    int vKeyCount = 0;
    int32_t gopCount = 0;
    int32_t count = 0;
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
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(bufferAttr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                count = vKeyCount;
                SetVideoValue(bufferAttr, videoIsEnd, videoFrame, vKeyCount);
                ASSERT_TRUE(CheckVideoSyncFrame(vKeyCount, gopCount, count));
                gopCount++;
            }
        }
    }
    ASSERT_EQ(aKeyCount, FLV_AUDIONUM_AVC_AAC);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_FLV_0020
 * @tc.name      : create source with fd, hevc+aac gop1 flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_FLV_0020, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/hevc_aac_1.flv";
    int fd = 0;
    bool initResult = false;
    InitFile(file, 2, fd, initResult);
    ASSERT_TRUE(initResult);
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
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(bufferAttr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(bufferAttr, videoIsEnd, videoFrame, vKeyCount);
            }
        }
    }
    ASSERT_EQ(aKeyCount, FLV_AUDIONUM_HEVC_AAC);
    ASSERT_EQ(vKeyCount, FLV_VIDEONUM_HEVC_AAC_ALL);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_FLV_0030
 * @tc.name      : create source with fd, hevc+aac gop-1 flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_FLV_0030, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/hevc_aac_first.flv";
    int fd = 0;
    bool initResult = false;
    InitFile(file, 2, fd, initResult);
    ASSERT_TRUE(initResult);
    int aKeyCount = 0;
    int vKeyCount = 0;
    int32_t count = 0;
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
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(bufferAttr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                count = vKeyCount;
                SetVideoValue(bufferAttr, videoIsEnd, videoFrame, vKeyCount);
                if (count == 0) {
                    ASSERT_EQ(vKeyCount, 1);
                }
            }
        }
    }
    ASSERT_EQ(aKeyCount, FLV_AUDIONUM_HEVC_AAC);
    ASSERT_EQ(vKeyCount, 1);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_FLV_0040
 * @tc.name      : create source with fd, avc gop60 flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_FLV_0040, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr bufferAttr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/avc_60.flv";
    int fd = 0;
    bool initResult = false;
    InitFile(file, 1, fd, initResult);
    ASSERT_TRUE(initResult);
    int vKeyCount = 0;
    int32_t gopCount = 0;
    int32_t count = 0;
    while (!videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            if (tarckType == MEDIA_TYPE_VID) {
                count = vKeyCount;
                SetVideoValue(bufferAttr, videoIsEnd, videoFrame, vKeyCount);
                ASSERT_TRUE(CheckVideoSyncFrame(vKeyCount, gopCount, count));
                gopCount++;
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_FLV_0050
 * @tc.name      : create source with fd, aac flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_FLV_0050, TestSize.Level2)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    int audioFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/only_aac.flv";
    int fd = 0;
    bool initResult = false;
    InitFile(file, 1, fd, initResult);
    ASSERT_TRUE(initResult);
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(bufferAttr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(aKeyCount, FLV_AUDIONUM_AAC);
    close(fd);
}