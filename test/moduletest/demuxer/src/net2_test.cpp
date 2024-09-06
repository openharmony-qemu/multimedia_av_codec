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

#include "avdemuxer.h"
#include "avsource.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "buffer/avsharedmemory.h"
#include "buffer/avsharedmemorybase.h"
#include "securec.h"
#include "inner_demuxer_sample.h"

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "av_common.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <thread>

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
class DemuxerNet2NdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

public:
    int32_t fd_ = -1;
    int64_t size;
};
static OH_AVMemory *memory = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static int32_t g_trackCount = 0;
static OH_AVBuffer *avBuffer = nullptr;

static int32_t g_width = 3840;
static int32_t g_height = 2160;
void DemuxerNet2NdkTest::SetUpTestCase() {}
void DemuxerNet2NdkTest::TearDownTestCase() {}
void DemuxerNet2NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerNet2NdkTest::TearDown()
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (avBuffer != nullptr) {
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }
}
} // namespace

namespace {


    /**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1000
 * @tc.name     : determine the orientation type of the video ROTATE_NONE.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1000, TestSize.Level0)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_NONE.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1001
 * @tc.name     : determine the orientation type of the video ROTATE_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1001, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1002
 * @tc.name     : determine the orientation type of the video ROTATE_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1002, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1003
 * @tc.name     : determine the orientation type of the video ROTATE_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1003, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1004
 * @tc.name     : determine the orientation type of the video FLIP_H.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1004, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1005
 * @tc.name     : determine the orientation type of the video FLIP_V.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1005, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1006
 * @tc.name     : determine the orientation type of the video FLIP_H_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1006, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H_90.mp4";;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1007
 * @tc.name     : determine the orientation type of the video FLIP_V_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1007, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1008
 * @tc.name     : determine the orientation type of the video FLIP_H_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1008, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V || rotation == OHOS::MediaAVCodec::FLIP_H_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1009
 * @tc.name     : determine the orientation type of the video FLIP_V_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1009, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H || rotation == OHOS::MediaAVCodec::FLIP_V_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1010
 * @tc.name     : determine the orientation type of the video FLIP_H_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1010, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_H_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1011
 * @tc.name     : determine the orientation type of the video FLIP_V_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1011, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_V_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1012
 * @tc.name     : determine the orientation type of the video INVALID.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1012, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/INVALID.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1013
 * @tc.name     : determine the orientation type of the video AV_ROTATE_NONE.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1013, TestSize.Level0)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_NONE.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1014
 * @tc.name     : determine the orientation type of the video AV_ROTATE_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1014, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1015
 * @tc.name     : determine the orientation type of the video AV_ROTATE_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1015, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1016
 * @tc.name     : determine the orientation type of the video AV_ROTATE_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1016, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1017
 * @tc.name     : determine the orientation type of the video AV_FLIP_H.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1017, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1018
 * @tc.name     : determine the orientation type of the video AV_FLIP_V.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1018, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1019
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1019, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1020
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1020, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1; 
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1021
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1021, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V || rotation == OHOS::MediaAVCodec::FLIP_H_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1022
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1022, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H || rotation == OHOS::MediaAVCodec::FLIP_V_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1023
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1023, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_H_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1024
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1024, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_V_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1025
 * @tc.name     : determine the orientation type of the video AV_INVALID.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1025, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_INVALID.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1026
 * @tc.name     : determine the orientation type of the video UNDEFINED_FLV.flv
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1026, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_FLV.flv";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1027
 * @tc.name     : determine the orientation type of the video UNDEFINED_fmp4.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1027, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_FMP4.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1028
 * @tc.name     : determine the orientation type of the video UNDEFINED_MKV.mkv
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1028, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_MKV.mkv";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1029
 * @tc.name     : determine the orientation type of the video UNDEFINED_TS.ts
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1029, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_TS.ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}
} // namespace