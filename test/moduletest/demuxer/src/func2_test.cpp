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
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "av_common.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <cmath>
#include <thread>

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class DemuxerFunc2NdkTest : public testing::Test {
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
constexpr int32_t VTTBACK = 4;
constexpr int32_t VTTFORWARD = 7;
constexpr int32_t VTTSEEKFORWARD = 5100;
constexpr int32_t VTTSEEKBACK = 2100;
void DemuxerFunc2NdkTest::SetUpTestCase() {}
void DemuxerFunc2NdkTest::TearDownTestCase() {}
void DemuxerFunc2NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerFunc2NdkTest::TearDown()
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
    if (avBuffer != nullptr) {
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
}
} // namespace Media
} // namespace OHOS
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

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_4800
 * @tc.name      : create vtt demuxer with file and read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_4800, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    int vttIndex = 1;
    int vttSubtitle = 0;
    const char *file = "/data/test/media/webvtt_test.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    int64_t starttime = 0;
    ASSERT_FALSE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_TRACK_START_TIME, &starttime));
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   vtt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        vttSubtitle = atoi(reinterpret_cast<const char*>(data));
        cout << "subtitle" << "----------------" << vttSubtitle << "-----------------" << endl;
        ASSERT_EQ(vttSubtitle, vttIndex);
        vttIndex++;
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_4900
 * @tc.name      : create vtt demuxer with file and forward back seek+read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_4900, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    int vttIndex = 1;
    int vttSubtitle = 0;
    uint8_t *data = nullptr;
    const char *file = "/data/test/media/webvtt_test.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    for (int index = 0; index < 5; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, VTTSEEKBACK, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
    data = OH_AVMemory_GetAddr(memory);
    vttSubtitle = atoi(reinterpret_cast<const char*>(data));
    ASSERT_EQ(vttSubtitle, VTTBACK);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, VTTSEEKFORWARD, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
    data = OH_AVMemory_GetAddr(memory);
    vttSubtitle = atoi(reinterpret_cast<const char*>(data));
    vttIndex = VTTFORWARD;
    ASSERT_EQ(vttSubtitle, VTTFORWARD);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        data = OH_AVMemory_GetAddr(memory);
        vttSubtitle = atoi(reinterpret_cast<const char*>(data));
        vttIndex++;
        ASSERT_EQ(vttSubtitle, vttIndex);
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_5000
 * @tc.name      : create vtt demuxer with file and back seek+read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_5000, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    int vttIndex = 1;
    int vttSubtitle = 0;
    uint8_t *data = nullptr;
    const char *file = "/data/test/media/webvtt_test.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    for (int index = 0; index < 5; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        data = OH_AVMemory_GetAddr(memory);
        vttSubtitle = atoi(reinterpret_cast<const char*>(data));
        ASSERT_EQ(vttSubtitle, vttIndex);
        vttIndex++;
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, VTTSEEKBACK, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
    data = OH_AVMemory_GetAddr(memory);
    vttSubtitle = atoi(reinterpret_cast<const char*>(data));
    vttIndex = 4;
    ASSERT_EQ(vttSubtitle, vttIndex);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        data = OH_AVMemory_GetAddr(memory);
        vttSubtitle = atoi(reinterpret_cast<const char*>(data));
        vttIndex++;
        ASSERT_EQ(vttSubtitle, vttIndex);
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_5100
 * @tc.name      : create vtt demuxer with file and forward seek+read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_5100, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    int vttIndex = 1;
    int vttSubtitle = 0;
    uint8_t *data = nullptr;
    const char *file = "/data/test/media/webvtt_test.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    for (int index = 0; index < 5; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        data = OH_AVMemory_GetAddr(memory);
        vttSubtitle = atoi(reinterpret_cast<const char*>(data));
        ASSERT_EQ(vttSubtitle, vttIndex);
        vttIndex++;
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, VTTSEEKFORWARD, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
    data = OH_AVMemory_GetAddr(memory);
    vttSubtitle = atoi(reinterpret_cast<const char*>(data));
    vttIndex = 7;
    ASSERT_EQ(vttSubtitle, vttIndex);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        data = OH_AVMemory_GetAddr(memory);
        vttSubtitle = atoi(reinterpret_cast<const char*>(data));
        vttIndex++;
        ASSERT_EQ(vttSubtitle, vttIndex);
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_5600
 * @tc.name      : create vtt demuxer with error file -- no empty paragraphs
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_5600, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/vtt_5600.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   vtt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_5700
 * @tc.name      : create vtt demuxer with error file -- subtitle sequence error
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_5700, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/vtt_5700.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   vtt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle" << "----------------" << data << "-----------------" << endl;
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_5800
 * @tc.name      : create vtt demuxer with error file -- timeline format error null
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_5800, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/vtt_5800.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    int tarckType = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr);
    uint8_t *data = OH_AVMemory_GetAddr(memory);
    cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_5900
 * @tc.name      : create vtt demuxer with error file -- subtitle is empty
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_5900, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/vtt_5900.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   vtt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_6000
 * @tc.name      : create vtt demuxer with error file -- vtt file is empty
 * @tc.desc      : function test
 * fail
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_6000, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/vtt_6000.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr);
    uint8_t *data = OH_AVMemory_GetAddr(memory);
    cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_VTT_6100
 * @tc.name      : create vtt demuxer with error file -- alternating Up and Down Times
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, SUB_MEDIA_DEMUXER_VTT_6100, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/vtt_6100.vtt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_SUBTITLE_WEBVTT));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    ASSERT_EQ(tarckType, MEDIA_TYPE_SUBTITLE);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   vtt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    }
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1000
 * @tc.name     : determine the orientation type of the video ROTATE_NONE.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1000, TestSize.Level0)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/ROTATE_NONE.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1001
 * @tc.name     : determine the orientation type of the video ROTATE_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1001, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/ROTATE_90.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_90);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1002
 * @tc.name     : determine the orientation type of the video ROTATE_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1002, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/ROTATE_180.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_180);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1003
 * @tc.name     : determine the orientation type of the video ROTATE_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1003, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/ROTATE_270.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_270);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1004
 * @tc.name     : determine the orientation type of the video FLIP_H.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1004, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_H.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1005
 * @tc.name     : determine the orientation type of the video FLIP_V.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1005, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_V.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1006
 * @tc.name     : determine the orientation type of the video FLIP_H_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1006, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_H_90.mp4";;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H_ROT90);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1007
 * @tc.name     : determine the orientation type of the video FLIP_V_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1007, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_V_90.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V_ROT90);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1008
 * @tc.name     : determine the orientation type of the video FLIP_H_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1008, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_H_180.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V || rotation == OHOS::MediaAVCodec::FLIP_H_ROT180);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1009
 * @tc.name     : determine the orientation type of the video FLIP_V_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1009, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_V_180.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H || rotation == OHOS::MediaAVCodec::FLIP_V_ROT180);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1010
 * @tc.name     : determine the orientation type of the video FLIP_H_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1010, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_H_270.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_H_ROT270);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1011
 * @tc.name     : determine the orientation type of the video FLIP_V_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1011, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/FLIP_V_270.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_V_ROT270);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1012
 * @tc.name     : determine the orientation type of the video INVALID.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1012, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/INVALID.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1013
 * @tc.name     : determine the orientation type of the video AV_ROTATE_NONE.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1013, TestSize.Level0)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_ROTATE_NONE.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1014
 * @tc.name     : determine the orientation type of the video AV_ROTATE_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1014, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_ROTATE_90.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_90);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1015
 * @tc.name     : determine the orientation type of the video AV_ROTATE_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1015, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_ROTATE_180.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_180);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1016
 * @tc.name     : determine the orientation type of the video AV_ROTATE_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1016, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_ROTATE_270.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_270);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1017
 * @tc.name     : determine the orientation type of the video AV_FLIP_H.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1017, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_H.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1018
 * @tc.name     : determine the orientation type of the video AV_FLIP_V.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1018, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_V.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1019
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1019, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_H_90.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H_ROT90);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1020
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1020, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_V_90.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V_ROT90);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1021
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1021, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_H_180.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V || rotation == OHOS::MediaAVCodec::FLIP_H_ROT180);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1022
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1022, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_V_180.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H || rotation == OHOS::MediaAVCodec::FLIP_V_ROT180);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1023
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1023, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_H_270.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_H_ROT270);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1024
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1024, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_FLIP_V_270.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_V_ROT270);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1025
 * @tc.name     : determine the orientation type of the video AV_INVALID.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1025, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *file = "/data/test/media/AV_INVALID.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1026
 * @tc.name     : determine the orientation type of the video UNDEFINED_FLV.flv
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1026, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *file = "/data/test/media/UNDEFINED_FLV.flv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1027
 * @tc.name     : determine the orientation type of the video UNDEFINED_fmp4.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1027, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *file = "/data/test/media/UNDEFINED_FMP4.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1028
 * @tc.name     : determine the orientation type of the video UNDEFINED_MKV.mkv
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1028, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *file = "/data/test/media/UNDEFINED_MKV.mkv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1029
 * @tc.name     : determine the orientation type of the video UNDEFINED_TS.ts
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerFunc2NdkTest, DEMUXER_ORIENTATIONTYPE_1029, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *file = "/data/test/media/UNDEFINED_TS.ts";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
    close(fd);
}