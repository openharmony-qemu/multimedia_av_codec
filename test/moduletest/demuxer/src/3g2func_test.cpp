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
class Demuxer3G2FuncNdkTest : public testing::Test {
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
    const char *INP_DIR_1 = "/data/test/media/3g2_mp4v_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_URI_1 = "http://127.0.0.1:46666/3g2_mp4v_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_DIR_2 = "/data/test/media/3g2_mp4v_simple@level1_720_480_mp4a_31.3g2";
    const char *INP_URI_2 = "http://127.0.0.1:46666/3g2_mp4v_simple@level1_720_480_mp4a_31.3g2";
    const char *INP_DIR_3 = "/data/test/media/3g2_mp4v_simple@level1_720_480_mp4a_13.3g2";
    const char *INP_URI_3 = "http://127.0.0.1:46666/3g2_mp4v_simple@level1_720_480_mp4a_13.3g2";
    const char *INP_DIR_4 = "/data/test/media/3g2_h264_amrwb_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_URI_4 = "http://127.0.0.1:46666/3g2_h264_amrwb_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_DIR_5 = "/data/test/media/3g2_h264_amrnb_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_URI_5 = "http://127.0.0.1:46666/3g2_h264_amrnb_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_DIR_6 = "/data/test/media/3g2_h263_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_URI_6 = "http://127.0.0.1:46666/3g2_h263_simple@level1_720_480_mp4a_2.3g2";
    const char *INP_DIR_7 = "/data/test/media/3g2_h264_720_480_mp4a_31.3g2";
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
constexpr int32_t FOUR = 4;
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
void Demuxer3G2FuncNdkTest::SetUpTestCase()
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}
void Demuxer3G2FuncNdkTest::TearDownTestCase()
{
    server->StopServer();
}
void Demuxer3G2FuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void Demuxer3G2FuncNdkTest::TearDown()
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
    cout << fileName << "---" << g_fd << "---" << size << "---source" << source << endl;
}

static void CreateUriSource(const char *uri)
{
    cout << uri << endl;
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
    cout << "g_trackCount----" << g_trackCount << endl;
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
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_0100
 * @tc.name      : demuxer 3g2, GetVideoTrackFormat,MD_KEY_HEIGHT, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_0100, TestSize.Level0)
{
    int32_t height = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_0100
 * @tc.name      : demuxer 3g2, GetVideoTrackFormat,MD_KEY_HEIGHT, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_0100, TestSize.Level0)
{
    int32_t height = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_0200
 * @tc.name      : demuxer 3g2, GetVideoTrackFormat,MD_KEY_WIDTH, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_0200, TestSize.Level0)
{
    int32_t width = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_0200
 * @tc.name      : demuxer 3g2, GetVideoTrackFormat,MD_KEY_WIDTH, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_0200, TestSize.Level0)
{
    int32_t width = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_0900
 * @tc.name      : demuxer 3g2, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_0900, TestSize.Level0)
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
 * @tc.number    : DEMUXER_3G2_FUNC_URI_0900
 * @tc.name      : demuxer 3g2, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_0900, TestSize.Level0)
{
    CreateUriSource(INP_URI_1);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_TRACK_VIDEO3_AUDIO1_0900
 * @tc.name      : demuxer 3g2, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_TRACK_VIDEO3_AUDIO1_0900, TestSize.Level0)
{
    CreateFdSource(INP_DIR_2);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, FOUR);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_TRACK_VIDEO3_AUDIO1_0900
 * @tc.name      : demuxer 3g2, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_TRACK_VIDEO3_AUDIO1_0900, TestSize.Level0)
{
    CreateUriSource(INP_URI_2);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, FOUR);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_TRACK_VIDEO1_AUDIO3_0900
 * @tc.name      : demuxer 3g2, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_TRACK_VIDEO1_AUDIO3_0900, TestSize.Level0)
{
    CreateFdSource(INP_DIR_3);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, FOUR);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_TRACK_VIDEO1_AUDIO3_0900
 * @tc.name      : demuxer 3g2, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_TRACK_URI_VIDEO1_AUDIO3_0900, TestSize.Level0)
{
    CreateUriSource(INP_URI_3);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, FOUR);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_1100
 * @tc.name      : demuxer 3g2, GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_1100, TestSize.Level0)
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
 * @tc.number    : DEMUXER_3G2_FUNC_URI_1100
 * @tc.name      : demuxer 3g2, GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_1100, TestSize.Level0)
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
 * @tc.number    : DEMUXER_3G2_FUNC_1200
 * @tc.name      : demuxer 3g2, GetAudioTrackFormat,MD_KEY_SAMPLE_RATE, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_1200, TestSize.Level0)
{
    int32_t sr = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 22050);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_1200
 * @tc.name      : demuxer 3g2, GetAudioTrackFormat,MD_KEY_SAMPLE_RATE, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_1200, TestSize.Level0)
{
    int32_t sr = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 22050);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_1400
 * @tc.name      : demuxer 3g2, GetPublicTrackFormat,MD_KEY_TRACK_TYPE, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_1400, TestSize.Level0)
{
    int32_t type = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_1400
 * @tc.name      : demuxer 3g2, GetPublicTrackFormat,MD_KEY_TRACK_TYPE, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_1400, TestSize.Level0)
{
    int32_t type = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_1600
 * @tc.name      : demuxer 3g2, Create source with g_fd, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_1600, TestSize.Level3)
{
    CreateFdSource(INP_DIR_1);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_1600
 * @tc.name      : demuxer 3g2, Create source with g_fd, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_1600, TestSize.Level3)
{
    CreateUriSource(INP_URI_1);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3200
 * @tc.name      : demuxer 3g2, Seek to the start time, previous mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 180, 216};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_3200
 * @tc.name      : demuxer 3g2, Seek to the start time, previous mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 0, 180, 216};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3300
 * @tc.name      : demuxer 3g2, Seek to the start time, next mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_3300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 0, 180, 216};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_3300
 * @tc.name      : demuxer 3g2, Seek to the start time, next mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_3300, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_NEXT_SYNC, 0, 180, 216};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3400
 * @tc.name      : demuxer 3g2, Seek to the start time, closest mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_3400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 0, 180, 216};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_3400
 * @tc.name      : demuxer 3g2, Seek to the start time, closest mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_3400, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_CLOSEST_SYNC, 0, 180, 216};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3500
 * @tc.name      : demuxer 3g2, Seek to the end time, previous mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 5000, 97, 117};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_3500
 * @tc.name      : demuxer 3g2, Seek to the end time, previous mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 5000, 97, 117};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3600
 * @tc.name      : demuxer 3g2, Seek to the end time, next mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_3600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 5000, 85, 102};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_3600
 * @tc.name      : demuxer 3g2, Seek to the end time, next mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_3600, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_NEXT_SYNC, 5000, 85, 102};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3700
 * @tc.name      : demuxer 3g2, Seek to the end time, closest mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_3700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 5000, 85, 102};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_URI_3700
 * @tc.name      : demuxer 3g2, Seek to the end time, closest mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_3700, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_CLOSEST_SYNC, 5000, 85, 102};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3800
 * @tc.name      : demuxer 3g2, Seek to the middle time, previous mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_3800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 10000, 1, 2};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_3800
 * @tc.name      : demuxer 3g2, Seek to the middle time, previous mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_3800, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 10000, 1, 2};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_4000
 * @tc.name      : demuxer 3g2, Seek to the middle time, closest mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 10000, 1, 2};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_4000
 * @tc.name      : demuxer 3g2, Seek to the middle time, closest mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_URI_4000, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_CLOSEST_SYNC, 10000, 1, 2};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_4300
 * @tc.name      : demuxer 3g2, Seek to a invalid time, closest mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4300, TestSize.Level0)
{
    CreateFdSource(INP_DIR_1);
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
 * @tc.number    : DEMUXER_3G2_FUNC_4400
 * @tc.name      : demuxer 3g2, Remove track before add track, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4400, TestSize.Level0)
{
    srand(time(nullptr));
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
 * @tc.number    : DEMUXER_3G2_FUNC_4500
 * @tc.name      : demuxer 3g2, Remove all tracks before demux finish, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4500, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;
    int count = 0;
    CreateFdSource(INP_DIR_1);
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
 * @tc.number    : DEMUXER_3G2_FUNC_4600
 * @tc.name      : demuxer 3g2, Start demux before add track, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4600, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    srand(time(nullptr));
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_4700
 * @tc.name      : demuxer 3g2, Seek to the time when there is no I frame, next mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4700, TestSize.Level0)
{
    CreateFdSource(INP_DIR_7);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    ASSERT_NE(demuxer, nullptr);
    int64_t pts = 9500;
    ret = OH_AVDemuxer_SeekToTime(demuxer, pts, SEEK_MODE_NEXT_SYNC);
    ASSERT_EQ(ret, AV_ERR_UNKNOWN);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_4800
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrwb_simple@level1_720_480_mp4a_2 basic process, local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4800, TestSize.Level2)
{
    CreateFdSource(INP_DIR_4);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_URI_FUNC_4800
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrwb_simple@level1_720_480_mp4a_2 basic process, uri
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_URI_FUNC_4800, TestSize.Level2)
{
    CreateUriSource(INP_URI_4);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_4900
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrwb_simple@level1_720_480_mp4a_2 seek process, local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_4900, TestSize.Level2)
{
    seekInfo fileTest1{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 0, 180, 502};
    CreateFdSource(INP_DIR_4);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_URI_FUNC_4900
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrwb_simple@level1_720_480_mp4a_2 seek process, uri
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_URI_FUNC_4900, TestSize.Level2)
{
    seekInfo fileTest1{INP_URI_4, SEEK_MODE_PREVIOUS_SYNC, 0, 180, 502};
    CreateUriSource(INP_URI_4);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_5000
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrnb_simple@level1_720_480_mp4a_2 basic process, local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_5000, TestSize.Level2)
{
    CreateFdSource(INP_DIR_5);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_URI_FUNC_5000
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrnb_simple@level1_720_480_mp4a_2 basic process, uri
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_URI_FUNC_5000, TestSize.Level2)
{
    CreateUriSource(INP_URI_5);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_5100
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrnb_simple@level1_720_480_mp4a_2 seek process, local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_5100, TestSize.Level2)
{
    seekInfo fileTest1{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 0, 180, 502};
    CreateFdSource(INP_DIR_5);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_URI_FUNC_5100
 * @tc.name      : demuxer 3g2, demuxer 3g2_h264_amrnb_simple@level1_720_480_mp4a_2 seek process, uri
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_URI_FUNC_5100, TestSize.Level2)
{
    seekInfo fileTest1{INP_URI_5, SEEK_MODE_PREVIOUS_SYNC, 0, 180, 502};
    CreateUriSource(INP_URI_5);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_5200
 * @tc.name      : demuxer 3g2, demuxer 3g2_h263_simple@level1_720_480_mp4a_2 basic process, local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_5200, TestSize.Level2)
{
    CreateFdSource(INP_DIR_6);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_URI_FUNC_5200
 * @tc.name      : demuxer 3g2, demuxer 3g2_h263_simple@level1_720_480_mp4a_2 basic process, uri
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_URI_FUNC_5200, TestSize.Level2)
{
    CreateUriSource(INP_URI_6);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_3G2_FUNC_5300
 * @tc.name      : demuxer 3g2, demuxer 3g2_h263_simple@level1_720_480_mp4a_2 seek process, local
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_FUNC_5300, TestSize.Level2)
{
    seekInfo fileTest1{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 0, 300, 216};
    CreateFdSource(INP_DIR_6);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_3G2_URI_FUNC_5300
 * @tc.name      : demuxer 3g2, demuxer 3g2_h263_simple@level1_720_480_mp4a_2 seek process, uri
 * @tc.desc      : function test
 */
HWTEST_F(Demuxer3G2FuncNdkTest, DEMUXER_3G2_URI_FUNC_5300, TestSize.Level2)
{
    seekInfo fileTest1{INP_URI_6, SEEK_MODE_PREVIOUS_SYNC, 0, 300, 216};
    CreateUriSource(INP_URI_6);
    CheckSeekMode(fileTest1);
}