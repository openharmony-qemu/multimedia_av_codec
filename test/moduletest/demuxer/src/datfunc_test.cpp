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

#include <fcntl.h>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

#include "file_server_demo.h"
#include "gtest/gtest.h"
#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avmemory.h"
#include "native_avsource.h"

using namespace OHOS::MediaAVCodec;
using namespace std;

namespace OHOS {
namespace Media {
class DemuxerDATFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    const char *INP_DIR_1 = "/data/test/media/dat_mpeg4_aac.dat";
    const char *INP_URI_1 = "http://127.0.0.1:46666/dat_mpeg4_aac.dat";
    const char *INP_DIR_2 = "/data/test/media/dat_h264_mp3.dat";
    const char *INP_URI_2 = "http://127.0.0.1:46666/dat_h264_mp3.dat";
    const char *INP_DIR_3 = "/data/test/media/dat_h265_aac.dat";
    const char *INP_URI_3 = "http://127.0.0.1:46666/dat_h265_aac.dat";
};

static unique_ptr<FileServerDemo> server = nullptr;
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
constexpr int32_t TWO = 2;
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";

void DemuxerDATFuncNdkTest::SetUpTestCase()
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}

void DemuxerDATFuncNdkTest::TearDownTestCase()
{
    server->StopServer();
}

void DemuxerDATFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}

void DemuxerDATFuncNdkTest::TearDown()
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
}  // namespace Media
}  // namespace OHOS

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

static void CreateFdSource(const char *fileName)
{
    g_fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    cout << fileName << "---fd:" << g_fd << "---size:" << size << "---source:" << source << endl;
}

static void CreateUriSource(const char *uri)
{
    cout << "URI:" << uri << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
}

static void CheckSeekMode(seekInfo seekInfo)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "Track count:" << g_trackCount << endl;

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond, seekInfo.seekmode));
        
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
            cout << "Video frame count:" << frameNum << endl;
            ASSERT_EQ(seekInfo.videoCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_AUD) {
            cout << "Audio frame count:" << frameNum << endl;
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
        cout << "Audio total frames:" << audioFrame << " | Audio end!" << endl;
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
        cout << "Video total frames:" << videoFrame << " | Video end!" << endl;
    } else {
        videoFrame++;
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            vKeyCount++;
        }
    }
}

static void DemuxerResult()
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
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

    cout << "Audio key frames:" << aKeyCount << " | Video key frames:" << vKeyCount << endl;
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0100
 * @tc.name      : demuxer DAT, GetVideoTrackFormat,MD_KEY_HEIGHT, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_0100, TestSize.Level0)
{
    int32_t height = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_0100
 * @tc.name      : demuxer DAT, GetVideoTrackFormat,MD_KEY_HEIGHT, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_0100, TestSize.Level0)
{
    int32_t height = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0200
 * @tc.name      : demuxer DAT, GetVideoTrackFormat,MD_KEY_WIDTH, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_0200, TestSize.Level0)
{
    int32_t width = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_0200
 * @tc.name      : demuxer DAT, GetVideoTrackFormat,MD_KEY_WIDTH, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_0200, TestSize.Level0)
{
    int32_t width = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0900
 * @tc.name      : demuxer DAT, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_0900, TestSize.Level0)
{
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_0900
 * @tc.name      : demuxer DAT, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_0900, TestSize.Level0)
{
    CreateUriSource(INP_URI_1);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1100
 * @tc.name      : demuxer DAT, GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_1100, TestSize.Level0)
{
    int32_t cc = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 2);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_1100
 * @tc.name      : demuxer DAT, GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_1100, TestSize.Level0)
{
    int32_t cc = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 2);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1200
 * @tc.name      : demuxer DAT, GetAudioTrackFormat,MD_KEY_SAMPLE_RATE, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_1200, TestSize.Level0)
{
    int32_t sr = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_1200
 * @tc.name      : demuxer DAT, GetAudioTrackFormat,MD_KEY_SAMPLE_RATE, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_1200, TestSize.Level0)
{
    int32_t sr = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1600
 * @tc.name      : demuxer DAT, Create source with g_fd, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_1600, TestSize.Level3)
{
    CreateFdSource(INP_DIR_1);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_1600
 * @tc.name      : demuxer DAT, Create source with URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_1600, TestSize.Level3)
{
    CreateUriSource(INP_URI_1);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_3200
 * @tc.name      : demuxer DAT, Seek to start time, previous mode, Local
 * @tc.desc      : 本地DAT文件跳转到起始时间（0ms），PREVIOUS_SYNC模式
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 75, 131};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_3200
 * @tc.name      : demuxer DAT, Seek to start time, previous mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 0, 75, 131};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_3500
 * @tc.name      : demuxer DAT, Seek to middle time, previous mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 3000, 1, 10};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_URI_3500
 * @tc.name      : demuxer DAT, Seek to middle time, previous mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_URI_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 3000, 1, 10};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_3800
 * @tc.name      : demuxer DAT, Seek to invalid time, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_3800, TestSize.Level0)
{
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    ASSERT_NE(demuxer, nullptr);
    int64_t invalidPts = 20000;
    ret = OH_AVDemuxer_SeekToTime(demuxer, invalidPts, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_NE(ret, AV_ERR_OK);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_4400
 * @tc.name      : demuxer DAT, Unselect track before select, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_4400, TestSize.Level0)
{
    CreateFdSource(INP_DIR_1);
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
 * @tc.number    : DEMUXER_DAT_FUNC_4600
 * @tc.name      : demuxer DAT, Read sample before select track, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDATFuncNdkTest, DEMUXER_DAT_FUNC_4600, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(g_fd);
    g_fd = -1;
}