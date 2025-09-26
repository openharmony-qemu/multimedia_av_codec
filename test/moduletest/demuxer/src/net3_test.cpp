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
class DemuxerNet3NdkTest : public testing::Test {
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
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";
void DemuxerNet3NdkTest::SetUpTestCase() {}
void DemuxerNet3NdkTest::TearDownTestCase() {}
void DemuxerNet3NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerNet3NdkTest::TearDown()
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

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0000
 * @tc.name      : uri demux invalid.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0000, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/invalid.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(source, nullptr);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0100
 * @tc.name      : uri demux h264_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0100, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h264_mp3.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 420, 420);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0200
 * @tc.name      : uri demux h264_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0200, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h264_aac.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 472, 472);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0300
 * @tc.name      : uri demux h265_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0300, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h265_mp3.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 420, 420);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0400
 * @tc.name      : uri demux h265_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0400, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h265_aac.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 472, 472);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0500
 * @tc.name      : uri demux mpeg2_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0500, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg2_mp3.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 387, 387);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0600
 * @tc.name      : uri demux mpeg2_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0600, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg2_aac.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 434, 434);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0700
 * @tc.name      : uri demux mpeg4_mp3.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0700, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg4_mp3.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 387, 387);
}

/**
 * @tc.number    : MTS_DEMUXER_URI_TEST_0800
 * @tc.name      : uri demux mpeg4_aac.mts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, MTS_DEMUXER_URI_TEST_0800, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg4_aac.mts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 434, 434);
}

/**
 * @tc.number    : M2TS_DEMUXER_URI_TEST_0000
 * @tc.name      : uri demux invalid.m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, M2TS_DEMUXER_URI_TEST_0000, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/invalid.m2ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(source, nullptr);
}

/**
 * @tc.number    : M2TS_DEMUXER_URI_TEST_0100
 * @tc.name      : uri demux h264_mp3.m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, M2TS_DEMUXER_URI_TEST_0100, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h264_mp3.m2ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 420, 420);
}

/**
 * @tc.number    : M2TS_DEMUXER_URI_TEST_0200
 * @tc.name      : uri demux h264_aac.m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, M2TS_DEMUXER_URI_TEST_0200, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h264_aac.m2ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 472, 472);
}

/**
 * @tc.number    : M2TS_DEMUXER_URI_TEST_0300
 * @tc.name      : uri demux h265_mp3.m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, M2TS_DEMUXER_URI_TEST_0300, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h265_mp3.m2ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 420, 420);
}

/**
 * @tc.number    : M2TS_DEMUXER_URI_TEST_0400
 * @tc.name      : uri demux h265_aac.m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, M2TS_DEMUXER_URI_TEST_0400, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h265_aac.m2ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 472, 472);
}

/**
 * @tc.number    : M2TS_DEMUXER_URI_TEST_0500
 * @tc.name      : uri demux mpeg2_aac.m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, M2TS_DEMUXER_URI_TEST_0500, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg2_aac.m2ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 434, 434);
}

/**
 * @tc.number    : M2TS_DEMUXER_URI_TEST_0600
 * @tc.name      : uri demux mpeg2_mp3.m2ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, M2TS_DEMUXER_URI_TEST_0600, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg2_mp3.m2ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 387, 387);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0000
 * @tc.name      : uri demux invalid.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0000, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/invalid.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(source, nullptr);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0100
 * @tc.name      : uri demux h264_aac.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0100, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h264_aac.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 474, 474);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0200
 * @tc.name      : uri demux h264_mp3.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0200, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h264_mp3.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(602, 3, 420, 420);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0300
 * @tc.name      : uri demux h265_aac.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0300, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h265_aac.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 2, 434, 434);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0400
 * @tc.name      : uri demux h265_mp3.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0400, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/h265_mp3.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 2, 387, 387);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0500
 * @tc.name      : uri demux mpeg2_aac.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0500, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg2_aac.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 434, 434);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0600
 * @tc.name      : uri demux mpeg2_mp3.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0600, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg2_mp3.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 387, 387);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0700
 * @tc.name      : uri demux mpeg4_aac.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0700, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg4_aac.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 434, 434);
}

/**
 * @tc.number    : TRP_DEMUXER_URI_TEST_0800
 * @tc.name      : uri demux mpeg4_mp3.trp
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet3NdkTest, TRP_DEMUXER_URI_TEST_0800, TestSize.Level0)
{
    const char *uri = "http://192.168.1.100:8080/share/three/media/mpeg4_mp3.trp";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    CheckTrackCount(&sourceFormat, source, &g_trackCount, 2);
    CheckTrackSelect();
    CheckFrames(303, 26, 387, 387);
}