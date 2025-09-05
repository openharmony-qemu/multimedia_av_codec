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
static OH_AVFormat *metaFormat = nullptr;
static bool g_isDataTrack = false;
static bool g_aIsEnd = false;
static bool g_vIsEnd = false;
static bool g_initResult = false;
static int g_trackType = 0;
static int g_audioFrame = 0;
static int g_videoFrame = 0;
static int g_fd = 0;
static int g_aKeyCount = 0;
static int g_vKeyCount = 0;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
constexpr int DATA_TRACK_NUM_ZERO = 0;
constexpr int DATA_TRACK_NUM_TWO = 2;
constexpr int DATA_TRACK_NUM_THREE = 3;
constexpr int FLV_AUDIONUM_AAC = 5148;
constexpr int FLV_AUDIONUM_HEVC_AAC = 210;
constexpr int FLV_VIDEONUM_HEVC_AAC_ALL = 602;
constexpr int FLV_AUDIONUM_AVC_AAC = 318;
constexpr int32_t GOPNUM = 60;
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
    if (metaFormat != nullptr) {
        OH_AVFormat_Destroy(metaFormat);
        metaFormat = nullptr;
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
        if (g_isDataTrack && index == DATA_TRACK_NUM_TWO) {
            ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        } else {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
    }
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    initResult = true;
}

static void DataTrackParameterReset()
{
    g_aIsEnd = false;
    g_vIsEnd = false;
    g_initResult = false;
    g_trackType = DATA_TRACK_NUM_ZERO;
    g_audioFrame = DATA_TRACK_NUM_ZERO;
    g_videoFrame = DATA_TRACK_NUM_ZERO;
    g_fd = DATA_TRACK_NUM_ZERO;
    g_aKeyCount = DATA_TRACK_NUM_ZERO;
    g_vKeyCount = DATA_TRACK_NUM_ZERO;
}

static bool CheckVideoSyncFrame(int &vKeyCount, int32_t &gopCount, int32_t &count)
{
    if (gopCount % GOPNUM == 0 && vKeyCount == count + 1) {
        return true;
    } else if (gopCount % GOPNUM != 0 && vKeyCount == count) {
        return true;
    } else {
        return false;
    }
}

static bool CheckVideoFirstSyncFrame(int32_t &gopCount, int &vKeyCount)
{
    if (gopCount == 0 && vKeyCount == 1) {
        return true;
    } else if (gopCount != 0 && vKeyCount == 1) {
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
    fd = -1;
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
    fd = -1;
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
    int32_t gopCount = 0;
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
                ASSERT_TRUE(CheckVideoFirstSyncFrame(gopCount, vKeyCount));
                gopCount++;
            }
        }
    }
    ASSERT_EQ(aKeyCount, FLV_AUDIONUM_HEVC_AAC);
    ASSERT_EQ(vKeyCount, 1);
    close(fd);
    fd = -1;
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
    fd = -1;
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
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_FLV_0060
 * @tc.name      : create source with fd, aac_h264.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_FLV_0060, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_h264.flv";
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
    ASSERT_EQ(aKeyCount, 528);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_FLV_0070
 * @tc.name      : create source with fd, aac_h265.flv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_FLV_0070, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_h265.flv";
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
    ASSERT_EQ(aKeyCount, 526);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0010
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0010, TestSize.Level1)
{
    int32_t metaIntValue = 0;
    const char *file = "/data/test/media/metaIntval.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    int32_t metaNum = 101010101;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.intval.intvalintval", &metaIntValue));
    ASSERT_EQ(metaIntValue, metaNum);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0020
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0020, TestSize.Level1)
{
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/metaFloatval.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    float metaFloatVal = 2.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval.aaa", &metaFloatValue));
    ASSERT_EQ(metaFloatValue, metaFloatVal);
    metaFloatVal = 25.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval.bbb", &metaFloatValue));
    ASSERT_EQ(metaFloatValue, metaFloatVal);
    metaFloatVal = 252.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval.ccc", &metaFloatValue));
    ASSERT_EQ(metaFloatValue, metaFloatVal);
    metaFloatVal = 2525.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval.ddd", &metaFloatValue));
    ASSERT_EQ(metaFloatValue, metaFloatVal);
    metaFloatVal = 25252.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval.eee", &metaFloatValue));
    ASSERT_EQ(metaFloatValue, metaFloatVal);
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : DEMUXER_META_0030
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0030, TestSize.Level1)
{
    const char *file = "/data/test/media/metaStringval.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* metaStringValue = nullptr;
    string metaKeyAdd = "aba";
    string metaKey = "com.openharmony.stringval.";
    string metaVal = "aaaaa";
    string metaValAdd = "b";
    for (int i = 0; i < 100; i++) {
        metaKey.append(metaKeyAdd);
        metaVal.append(metaValAdd);
        ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, metaKey.c_str(), &metaStringValue));
        ASSERT_EQ(metaStringValue, metaVal);
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : DEMUXER_META_0040
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0040, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/metaAlltype.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    string metaKeyAdd = "a";
    string metaKey = "com.openharmony.stringval.";
    string metaVal = "aaaaa";
    string metaValAdd = "b";
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaVal.append(metaValAdd);
        ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, metaKey.c_str(), &metaStringValue));
        ASSERT_EQ(metaStringValue, metaVal);
    }

    metaKeyAdd = "a";
    metaKey = "com.openharmony.floatval.";
    float metaFloatVal = 123.5;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaFloatVal = metaFloatVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, metaKey.c_str(), &metaFloatValue));
        ASSERT_EQ(metaFloatValue, metaFloatVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.Intval.";
    int32_t metaIntVal = 123;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaIntVal = metaIntVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, metaKey.c_str(), &metaIntValue));
        ASSERT_EQ(metaIntValue, metaIntVal);
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : DEMUXER_META_0050
 * @tc.name      : demuxer meta info, get value with error key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0050, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/metaAlltype.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.stringval.abb", &metaStringValue));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.intnum.abb", &metaIntValue));
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval.abb", &metaFloatValue));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0060
 * @tc.name      : demuxer meta info, get value with error key type
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0060, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/metaAlltype.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.floatval.a", &metaStringValue));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.stringval.a", &metaIntValue));
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.intnum.a", &metaFloatValue));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0070
 * @tc.name      : demuxer meta info, file with no meta
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0070, TestSize.Level2)
{
    const char *file = "/data/test/media/m4v_fmp4.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* language = OH_AVFormat_DumpInfo(metaFormat);
    ASSERT_EQ(language, nullptr);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0080
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0080, TestSize.Level3)
{
    const char *file = "/data/test/media/double_hevc.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    string metamanufacturer = "ABCDEF";
    string metamarketingName = "AABBAABBAABBAABBAA";
    string metamodel = "ABABABAB";
    string metaversion = "12";
    const char* manufacturer;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.abababa.manufacturer", &manufacturer));
    ASSERT_EQ(manufacturer, metamanufacturer);
    const char* marketingName;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.abababa.marketing_name", &marketingName));
    ASSERT_EQ(marketingName, metamarketingName);
    const char* model;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.abababa.model", &model));
    ASSERT_EQ(model, metamodel);
    const char* version;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.abababa.version", &version));
    ASSERT_EQ(version, metaversion);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0090
 * @tc.name      : demuxer meta info, souce is null
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0090, TestSize.Level0)
{
    metaFormat= OH_AVSource_GetCustomMetadataFormat(nullptr);
    ASSERT_EQ(metaFormat, nullptr);
}

/**
 * @tc.number    : DEMUXER_META_0100
 * @tc.name      : demuxer meta info, get max value
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_META_0100, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/metaMaxval.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.stringval", &metaStringValue));
    float floatValue = 3.4028235E38;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval.eee", &metaFloatValue));
    ASSERT_EQ(floatValue, metaFloatValue);
    int32_t intValue = 2147483647;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.intval.aaaa", &metaIntValue));
    ASSERT_EQ(metaIntValue, intValue);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MKV_0010
 * @tc.name      : create source with fd, aac_h265.mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MKV_0010, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_h265.mkv";
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
    ASSERT_EQ(aKeyCount, 526);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MKV_0020
 * @tc.name      : create source with fd, mp3_h264.mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MKV_0020, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_h264.mkv";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MKV_0030
 * @tc.name      : create source with fd, mp3_h265.mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MKV_0030, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_h265.mkv";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MKV_0040
 * @tc.name      : create source with fd, opus_h264.mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MKV_0040, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/opus_h264.mkv";
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
    ASSERT_EQ(aKeyCount, 610);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MKV_0050
 * @tc.name      : create source with fd, opus_h265.mkv
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MKV_0050, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/opus_h265.mkv";
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
    ASSERT_EQ(aKeyCount, 610);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MP4_0010
 * @tc.name      : create source with fd, aac_mpeg4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MP4_0010, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_mpeg4.mp4";
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
    ASSERT_EQ(aKeyCount, 528);
    ASSERT_EQ(vKeyCount, 31);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MP4_0020
 * @tc.name      : create source with fd, mp3_h264.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MP4_0020, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_h264.mp4";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MP4_0030
 * @tc.name      : create source with fd, mp3_h265.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MP4_0030, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_h265.mp4";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MP4_0040
 * @tc.name      : create source with fd, aac_mpeg4_subtitle.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MP4_0040, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_mpeg4_subtitle.mp4";
    int fd = 0;
    bool initResult = false;
    InitFile(file, 3, fd, initResult);
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
    ASSERT_EQ(aKeyCount, 528);
    ASSERT_EQ(vKeyCount, 31);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_MP4_0050
 * @tc.name      : create source with fd, mp3_h265_fmp4.mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_MP4_0050, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_h265_fmp4.mp4";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_TS_0010
 * @tc.name      : create source with fd, aac_h265.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_TS_0010, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_h265.ts";
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
    ASSERT_EQ(aKeyCount, 526);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(vKeyCount, 2);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_TS_0020
 * @tc.name      : create source with fd, aac_mpeg2.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_TS_0020, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_mpeg2.ts";
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
    ASSERT_EQ(aKeyCount, 528);
    ASSERT_EQ(vKeyCount, 31);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_TS_0030
 * @tc.name      : create source with fd, aac_mpeg4.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_TS_0030, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/aac_mpeg4.ts";
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
    ASSERT_EQ(aKeyCount, 528);
    ASSERT_EQ(vKeyCount, 31);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_TS_0040
 * @tc.name      : create source with fd, mp3_h264.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_TS_0040, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_h264.ts";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 2);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_TS_0050
 * @tc.name      : create source with fd, mp3_mpeg2.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_TS_0050, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_mpeg2.ts";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 31);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_TS_0060
 * @tc.name      : create source with fd, mp3_mpeg4.ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_TS_0060, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/mp3_mpeg4.ts";
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
    ASSERT_EQ(aKeyCount, 468);
    ASSERT_EQ(vKeyCount, 31);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_COMMENT_0010
 * @tc.name      : 中文字符串，使用正确/错误的接口类型获取comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_COMMENT_0010, TestSize.Level0)
{
    const char *file = "/data/test/media/comment_0010.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t intVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_COMMENT, &intVal));
    int64_t longVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_COMMENT, &longVal));
    double doubleVal;
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_COMMENT, &doubleVal));
    float floatVal = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_COMMENT, &floatVal));
    const char *stringVal = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    ASSERT_NE(stringVal, nullptr);
    ASSERT_EQ(0, strcmp(stringVal, "中文测试字符串中文测试字符串"));
}

/**
 * @tc.number    : DEMUXER_FUNCTION_COMMENT_0020
 * @tc.name      : 英文字符串，使用正确/错误的接口类型获取comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_COMMENT_0020, TestSize.Level0)
{
    const char *file = "/data/test/media/comment_0020.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t intVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_COMMENT, &intVal));
    int64_t longVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_COMMENT, &longVal));
    double doubleVal;
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_COMMENT, &doubleVal));
    float floatVal = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_COMMENT, &floatVal));
    const char *stringVal = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    ASSERT_NE(stringVal, nullptr);
    ASSERT_EQ(0, strcmp(stringVal, "comment_test"));
}

/**
 * @tc.number    : DEMUXER_FUNCTION_COMMENT_0030
 * @tc.name      : 中文数字符号字符串，使用正确/错误的接口类型获取comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_COMMENT_0030, TestSize.Level0)
{
    const char *file = "/data/test/media/comment_0030.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t intVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_COMMENT, &intVal));
    int64_t longVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_COMMENT, &longVal));
    double doubleVal;
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_COMMENT, &doubleVal));
    float floatVal = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_COMMENT, &floatVal));
    const char *stringVal = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    ASSERT_NE(stringVal, nullptr);
    ASSERT_EQ(0, strcmp(stringVal, "；；；；；‘’‘’1234"));
}

/**
 * @tc.number    : DEMUXER_FUNCTION_COMMENT_0040
 * @tc.name      : 英文数字符号字符串，使用正确/错误的接口类型获取comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_COMMENT_0040, TestSize.Level0)
{
    const char *file = "/data/test/media/comment_0040.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t intVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_COMMENT, &intVal));
    int64_t longVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_COMMENT, &longVal));
    double doubleVal;
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_COMMENT, &doubleVal));
    float floatVal = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_COMMENT, &floatVal));
    const char *stringVal = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    ASSERT_NE(stringVal, nullptr);
    ASSERT_EQ(0, strcmp(stringVal, "[[[]]]////1234"));
}
/**
 * @tc.number    : DEMUXER_FUNCTION_COMMENT_0050
 * @tc.name      : 边界值长字符串，使用正确/错误的接口类型获取comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_COMMENT_0050, TestSize.Level0)
{
    char uri[] = "/data/test/media/comment_0050.mp4";
    source = OH_AVSource_CreateWithURI(uri);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t intVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_COMMENT, &intVal));
    int64_t longVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_COMMENT, &longVal));
    double doubleVal;
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_COMMENT, &doubleVal));
    float floatVal = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_COMMENT, &floatVal));
    const char *stringVal = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    ASSERT_NE(stringVal, nullptr);
    ASSERT_EQ(0, strcmp(stringVal, "comment_comment_comment_comment_comment_comment"
        "comment_comment_comment_comment_comment_comment_comment_comment_comment_comment_comment_comment"
        "comment_comment_comment_comment_comment_comment_comment_comment_comment_comment_comment_comment"
        "comment_comment_com"));
}

/**
 * @tc.number    : DEMUXER_FUNCTION_COMMENT_0060
 * @tc.name      : 边界值长字符串超过256，使用正确/错误的接口类型获取comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_COMMENT_0060, TestSize.Level0)
{
    char uri[] = "/data/test/media/comment_0060.mp4";
    source = OH_AVSource_CreateWithURI(uri);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    int32_t intVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_COMMENT, &intVal));
    int64_t longVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_COMMENT, &longVal));
    double doubleVal;
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_COMMENT, &doubleVal));
    float floatVal = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_COMMENT, &floatVal));
    const char *stringVal = nullptr;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
}

/**
 * @tc.number    : DEMUXER_FUNCTION_COMMENT_0070
 * @tc.name      : 不含comment，使用正确/错误的接口类型获取comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_COMMENT_0070, TestSize.Level0)
{
    char uri[] = "/data/test/media/mp3_h265_fmp4.mp4";
    source = OH_AVSource_CreateWithURI(uri);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal = nullptr;
    int32_t intVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_COMMENT, &intVal));
    int64_t longVal = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_COMMENT, &longVal));
    double doubleVal;
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(sourceFormat, OH_MD_KEY_COMMENT, &doubleVal));
    float floatVal = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(sourceFormat, OH_MD_KEY_COMMENT, &floatVal));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
}

/**
 * @tc.number    : DEMUXER_FUNCTION_DATATRACK_0010
 * @tc.name      : data_track类型 获取轨道和获取帧数
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc3NdkTest, DEMUXER_FUNCTION_DATATRACK_0010, TestSize.Level0)
{
    g_isDataTrack = true;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/avc_aac_data_track.ts";
    InitFile(file, DATA_TRACK_NUM_THREE, g_fd, g_initResult);
    ASSERT_TRUE(g_initResult);
    while (!g_aIsEnd || !g_vIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            if (index == DATA_TRACK_NUM_TWO) {
                ASSERT_NE(trackFormat, nullptr);
                ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_trackType));
            } else {
                ASSERT_NE(trackFormat, nullptr);
                ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &g_trackType));
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((g_aIsEnd && (g_trackType == MEDIA_TYPE_AUD)) || (g_vIsEnd && (g_trackType == MEDIA_TYPE_VID))) {
                continue;
            }
            if (index == DATA_TRACK_NUM_TWO) {
                ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            } else {
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
                ASSERT_NE(avBuffer, nullptr);
                ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            }
            if (g_trackType == MEDIA_TYPE_AUD) {
                SetAudioValue(bufferAttr, g_aIsEnd, g_audioFrame, g_aKeyCount);
            }else if (g_trackType == MEDIA_TYPE_VID) {
                SetVideoValue(bufferAttr, g_vIsEnd, g_videoFrame, g_vKeyCount);
            }
        }
    }
    ASSERT_EQ(g_audioFrame, 940);
    ASSERT_EQ(g_videoFrame, 300);
    ASSERT_EQ(g_aKeyCount, 940);
    ASSERT_EQ(g_vKeyCount, 2);
    close(g_fd);
    g_fd = -1;
    g_isDataTrack = false;
    DataTrackParameterReset();
}