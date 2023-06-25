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
class DemuxerFuncNdkTest : public testing::Test {
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
static OH_AVErrCode ret = AV_ERR_OK;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static int32_t trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;

void DemuxerFuncNdkTest::SetUpTestCase() {}
void DemuxerFuncNdkTest::TearDownTestCase() {}
void DemuxerFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    trackCount = 0;
}
void DemuxerFuncNdkTest::TearDown()
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

/**
 * @tc.number    : DEMUXER_FUNCTION_0100
 * @tc.name      : create source with uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0100, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI1 = "http://192.168.3.11:8080/share/audio/MP3_48000_1.mp3";
    cout << URI1 << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI1));
    ASSERT_NE(nullptr, source);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);

    while (!isEnd) {
        ret = OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr);
        ASSERT_EQ(ret, AV_ERR_OK);
        if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            isEnd = true;
        }
    }
}

/**
 * @tc.number    : DEMUXER_FUNCTION_0200
 * @tc.name      : create source with no permission URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0200, TestSize.Level0)
{
    const char *uri = "http://10.174.172.228:8080/share/audio/AAC_48000_1.aac";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_0300
 * @tc.name      : create source with invalid uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0300, TestSize.Level1)
{
    const char *uri = "http://invalidPath/invalid.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_0400
 * @tc.name      : create source with fd but no permission
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0400, TestSize.Level1)
{
    const char *file = "/data/media/noPermission.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    cout << file << "------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(demuxer, nullptr);

    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_0500
 * @tc.name      : create source with invalid fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0500, TestSize.Level1)
{
    const char *file = "/data/test/media/invalid.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_0700
 * @tc.name      : create source with fd, mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0700, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {

            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    a_frame++;
                }
            } else if (tarckType == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    cout << v_frame << "   video is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    v_frame++;
                }
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_0800
 * @tc.name      : create source with fd,avcc mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0800, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file = "/data/test/media/avcc_10sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {

            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    a_frame++;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        aKeyCount++;
                    }
                }
            } else if (tarckType == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    cout << v_frame << "   video is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    v_frame++;
                    uint8_t *buffer = OH_AVMemory_GetAddr(memory);
                    for (int i = 0; i < 16; i++) {
                        printf("%2x ", buffer[i]);
                    }
                    cout << "video track !!!!!" << endl;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        vKeyCount++;
                    }
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 431);
    ASSERT_EQ(v_frame, 600);
    ASSERT_EQ(aKeyCount, 431);
    ASSERT_EQ(vKeyCount, 10);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_0900
 * @tc.name      : create source with fd,hvcc mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_0900, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file = "/data/test/media/hvcc.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {

            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                } else {
                    a_frame++;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        aKeyCount++;
                    }
                }
            } else if (tarckType == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                } else {
                    v_frame++;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        vKeyCount++;
                    }
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 433);
    ASSERT_EQ(v_frame, 602);
    ASSERT_EQ(aKeyCount, 433);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1000
 * @tc.name      : create source with fd,mpeg mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1000, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file = "/data/test/media/mpeg2.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    a_frame++;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        aKeyCount++;
                    }
                }
            } else if (tarckType == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    cout << v_frame << "   video is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    v_frame++;
                    cout << "video track !!!!!" << endl;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        vKeyCount++;
                    }
                }
            }
        }
    }

    ASSERT_EQ(a_frame, 433);
    ASSERT_EQ(v_frame, 303);
    ASSERT_EQ(aKeyCount, 433);
    ASSERT_EQ(vKeyCount, 26);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1100
 * @tc.name      : demux m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1100, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file = "/data/test/media/audio/M4A_48000_1.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }

    ASSERT_EQ(a_frame, 10293);
    ASSERT_EQ(keyCount, 10293);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1200
 * @tc.name      : demux aac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1200, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file = "/data/test/media/audio/AAC_48000_1.aac";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 9457);
    ASSERT_EQ(keyCount, 9457);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1300
 * @tc.name      : demux mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1300, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file = "/data/test/media/audio/MP3_48000_1.mp3";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 9150);
    ASSERT_EQ(keyCount, 9150);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1400
 * @tc.name      : demux ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1400, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file = "/data/test/media/audio/OGG_48000_1.ogg";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 11439);
    ASSERT_EQ(keyCount, 11439);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1500
 * @tc.name      : demux flac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1500, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file = "/data/test/media/audio/FLAC_48000_1.flac";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 2288);
    ASSERT_EQ(keyCount, 2288);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1600
 * @tc.name      : demux wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1600, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file = "/data/test/media/audio/wav_48000_1.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 5146);
    ASSERT_EQ(keyCount, 5146);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1700
 * @tc.name      : demux mpeg-ts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1700, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file = "/data/test/media/ts_video.ts";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int vKeyCount = 0;
    int aKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {

            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    a_frame++;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        aKeyCount++;
                    }
                }
            } else if (tarckType == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    cout << v_frame << "   video is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    v_frame++;
                    cout << "video track !!!!!" << endl;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        vKeyCount++;
                    }
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 384);
    ASSERT_EQ(aKeyCount, 384);
    ASSERT_EQ(v_frame, 602);
    ASSERT_EQ(vKeyCount, 51);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1800
 * @tc.name      : demux unsupported source
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1800, TestSize.Level2)
{
    const char *file = "/data/test/media/mkv.mkv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_1900
 * @tc.name      : demux mp4, zero track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_1900, TestSize.Level1)
{
    const char *file = "/data/test/media/zero_track.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(trackCount, 0);

    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_2000
 * @tc.name      : OH_AVSource_CreateWithFD test
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_2000, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file1 = "/data/test/media/audio/MP3_48000_1.mp3";
    int64_t size1 = GetFileSize(file1);

    const char *file = "/data/test/media/audio/MP3_avcc_10sec.bin";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size1);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 9150);
    ASSERT_EQ(keyCount, 9150);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_2100
 * @tc.name      : OH_AVSource_CreateWithFD test
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_2100, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file1 = "/data/test/media/audio/MP3_48000_1.mp3";
    int64_t size1 = GetFileSize(file1);

    const char *file2 = "/data/test/media/avcc_10sec.mp4";
    int64_t size2 = GetFileSize(file2);

    const char *file = "/data/test/media/audio/MP3_avcc_10sec.bin";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, size1, size2);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int tarckType = 0;
    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {

            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    a_frame++;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        aKeyCount++;
                    }
                }
            } else if (tarckType == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    cout << v_frame << "   video is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    v_frame++;
                    cout << "video track !!!!!" << endl;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        vKeyCount++;
                    }
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 431);
    ASSERT_EQ(v_frame, 600);
    ASSERT_EQ(aKeyCount, 431);
    ASSERT_EQ(vKeyCount, 10);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_2200
 * @tc.name      : OH_AVSource_CreateWithFD test
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_2200, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file1 = "/data/test/media/audio/MP3_48000_1.mp3";
    int64_t size1 = GetFileSize(file1);

    const char *file = "/data/test/media/audio/MP3_OGG_48000_1.bin";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size1);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 9150);
    ASSERT_EQ(keyCount, 9150);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_2300
 * @tc.name      : OH_AVSource_CreateWithFD test
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_2300, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int a_frame = 0;

    const char *file1 = "/data/test/media/audio/MP3_48000_1.mp3";
    int64_t size1 = GetFileSize(file1);

    const char *file2 = "/data/test/media/audio/OGG_48000_1.ogg";
    int64_t size2 = GetFileSize(file2);

    const char *file = "/data/test/media/audio/MP3_OGG_48000_1.bin";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, size1, size2);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int keyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            } else {
                a_frame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 11439);
    ASSERT_EQ(keyCount, 11439);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_2400
 * @tc.name      : OH_AVSource_CreateWithFD test
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_2400, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file1 = "/data/test/media/ts_video.ts";
    int64_t size1 = GetFileSize(file1);

    const char *file = "/data/test/media/test_video_avcc_10sec.bin";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size1);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(1, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int tarckType = 0;
    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {

            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (tarckType == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    a_frame++;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        aKeyCount++;
                    }
                }
            } else if (tarckType == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    cout << v_frame << "   video is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    v_frame++;
                    cout << "video track !!!!!" << endl;
                    if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                        vKeyCount++;
                    }
                }
            }
        }
    }
    ASSERT_EQ(a_frame, 384);
    ASSERT_EQ(v_frame, 602);
    ASSERT_EQ(aKeyCount, 384);
    ASSERT_EQ(vKeyCount, 51);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3100
 * @tc.name      : seek to the start time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3100, TestSize.Level1)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    int pos = rand() % 250;
    int64_t start_pts = 0;
    bool isFirstFrame = true;
    bool isEnd = false;
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    cout << trackCount << "####################" << endl;

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (isFirstFrame) {
                start_pts = attr.pts;
                isFirstFrame = false;
            }
            if (count == pos) {
                isEnd = true;
                cout << pos << " =====curr_pts = attr.pts" << endl;
                break;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end!!!!!!!!" << endl;
            }
            count++;
        }
        cout << "count: " << count << endl;
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, start_pts / 1000, SEEK_MODE_PREVIOUS_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr));
    ASSERT_EQ(attr.pts, start_pts);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3200
 * @tc.name      : seek to the start time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3200, TestSize.Level1)
{
    bool isEnd = false;
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    int pos = rand() % 250;
    int64_t start_pts = 0;
    bool isFirstFrame = true;
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (isFirstFrame) {
                start_pts = attr.pts;
                isFirstFrame = false;
            }
            if (count == pos) {
                isEnd = true;
                cout << "curr_pts = attr.pts" << endl;
                break;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end!!!!!!!!" << endl;
            }
            count++;
        }
        cout << "count: " << count << endl;
    }

    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, start_pts / 1000, SEEK_MODE_NEXT_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr));
    ASSERT_EQ(attr.pts, start_pts);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3300
 * @tc.name      : seek to the start time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3300, TestSize.Level1)
{
    bool isEnd = false;
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    int pos = rand() % 250;
    int64_t start_pts = 0;
    bool isFirstFrame = true;
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (isFirstFrame) {
                start_pts = attr.pts;
                isFirstFrame = false;
            }
            if (count == pos) {
                isEnd = true;
                cout << "curr_pts = attr.pts" << endl;
                break;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end!!!!!!!!" << endl;
            }
            count++;
        }
        cout << "count: " << count << endl;
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, start_pts / 1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr));
    ASSERT_EQ(attr.pts, start_pts);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3400
 * @tc.name      : seek to the end time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3400, TestSize.Level1)
{
    bool isEnd = false;
    uint32_t trackIndex = 1;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    cout << trackCount << "####################" << endl;

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int64_t end_pts = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (static_cast<uint32_t>(index) != trackIndex) {
                continue;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end!!!!!!!!" << endl;
            } else {
                end_pts = attr.pts;
            }
            count++;
        }
        cout << "count: " << count << endl;
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, end_pts / 1000, SEEK_MODE_PREVIOUS_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr));
    ASSERT_EQ(attr.pts, end_pts);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3500
 * @tc.name      : seek to the end time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3500, TestSize.Level1)
{
    bool isEnd = false;
    uint32_t trackIndex = 1;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    cout << trackCount << "####################" << endl;

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int64_t end_pts = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (static_cast<uint32_t>(index) != trackIndex) {
                continue;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end!!!!!!!!" << endl;
            } else {
                end_pts = attr.pts;
            }
            count++;
        }
        cout << "count: " << count << endl;
    }
    // end I
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, end_pts / 1000, SEEK_MODE_NEXT_SYNC));
    end_pts += 1000;
    ASSERT_EQ(AV_ERR_UNKNOWN, OH_AVDemuxer_SeekToTime(demuxer, end_pts / 1000, SEEK_MODE_NEXT_SYNC));
    end_pts += 1000000;
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_SeekToTime(demuxer, end_pts / 1000, SEEK_MODE_NEXT_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3600
 * @tc.name      : seek to the end time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3600, TestSize.Level1)
{
    bool isEnd = false;
    uint32_t trackIndex = 1;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    cout << trackCount << "####################" << endl;

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int64_t end_pts = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (static_cast<uint32_t>(index) != trackIndex) {
                continue;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end!!!!!!!!" << endl;
            } else {
                end_pts = attr.pts;
            }
            count++;
        }
        cout << "count: " << count << endl;
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, end_pts / 1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr));
    ASSERT_EQ(attr.pts, end_pts);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3700
 * @tc.name      : seek to a random time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3700, TestSize.Level0)
{
    bool isFirstFrame = true;
    uint32_t trackIndex = 1;
    int count = 0;
    srand(time(NULL));
    int pos = rand() % 250;
    int pos_to = rand() % 250;
    int64_t to_ms = pos_to * 40000;
    cout << "pos: " << pos << "pos_to: " << pos_to << "to_ms: " << to_ms << endl;
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int a_frame = 0;
    int v_frame = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));

            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (isFirstFrame) {
                isFirstFrame = false;
            }

            if (count == pos) {
                videoIsEnd = true;
                audioIsEnd = true;
                cout << "curr_pts = attr.pts" << endl;
                break;
            }

            if (tarckType == 0 && attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << a_frame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            }
            if (tarckType == 1 && attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                videoIsEnd = true;
                cout << v_frame << "   video is end !!!!!!!!!!!!!!!" << endl;
            }
        }
        count++;
    }
    cout << "count: " << count << endl;
    int64_t prev_i_pts = to_ms;
    ret = OH_AVDemuxer_SeekToTime(demuxer, to_ms / 1000, SEEK_MODE_PREVIOUS_SYNC);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OK);
    bool ans = abs(prev_i_pts - attr.pts) < 40000 ? true : false;
    ASSERT_EQ(ans, true);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3800
 * @tc.name      : seek to a random time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3800, TestSize.Level0)
{
    bool isEnd = false;
    bool isFirstFrame = true;
    uint32_t trackIndex = 1;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int pos = rand() % 250;
    int pos_to = rand() % 250;
    int64_t to_ms = pos_to * 40000;
    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (isFirstFrame) {
                isFirstFrame = false;
            }
            if (count == pos) {
                isEnd = true;
                break;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
            }
            count++;
        }
    }
    int64_t next_i_pts = to_ms;
    ret = OH_AVDemuxer_SeekToTime(demuxer, to_ms / 1000, SEEK_MODE_NEXT_SYNC);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OK);
    bool ans = abs(next_i_pts - attr.pts) < 40000 ? true : false;
    ASSERT_EQ(ans, true);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_3900
 * @tc.name      : seek to a random time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_3900, TestSize.Level0)
{
    bool isEnd = false;
    bool isFirstFrame = true;
    uint32_t trackIndex = 1;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int count = 0;
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int pos = rand() % 250;
    int pos_to = rand() % 250;
    int64_t to_ms = pos_to * 40000;
    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (isFirstFrame) {
                isFirstFrame = false;
            }
            if (count == pos) {
                isEnd = true;
                cout << "curr_pts = attr.pts" << endl;
                break;
            }
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
            }
            count++;
        }
    }
    int64_t closest_i_pts = to_ms;
    ret = OH_AVDemuxer_SeekToTime(demuxer, to_ms / 1000, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OK);
    bool ans = abs(closest_i_pts - attr.pts) < 40000 ? true : false;
    ASSERT_EQ(ans, true);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_4000
 * @tc.name      : seek to a invalid time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_4000, TestSize.Level2)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    srand(time(NULL));
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));

    ASSERT_NE(demuxer, nullptr);
    int64_t invalid_pts = 12000 * 16666;
    ret = OH_AVDemuxer_SeekToTime(demuxer, invalid_pts, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_NE(ret, AV_ERR_OK);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_4100
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_4100, TestSize.Level2)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    srand(time(NULL));
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
}

/**
 * @tc.number    : DEMUXER_FUNCTION_4200
 * @tc.name      : remove all tracks after demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_4200, TestSize.Level1)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    bool isEnd = false;

    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end !!!!!!!!!!!!!!!" << endl;
            }
        }
    }

    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, index));
    }

    int32_t memory_size = OH_AVMemory_GetSize(memory);
    ASSERT_NE(0, memory_size);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_4300
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_4300, TestSize.Level1)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
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
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    srand(time(NULL));
    int pos = rand() % 250;
    cout << " pos= " << pos << endl;
    while (!isEnd) {
        for (int32_t index = 0; index < trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (count == pos) {
                cout << count << " count == pos!!!!!!!!!" << endl;
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 0));
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 1));
                ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
                isEnd = true;
                break;
            }

            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end !!!!!!!!!!!!!!!" << endl;
            }
            if (index == 0) {
                count++;
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_4400
 * @tc.name      : remove audio track before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_4400, TestSize.Level1)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";

    int a_count = 0;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(2, trackCount);
    for (int32_t index = 0; index < trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    srand(time(NULL));
    int pos = rand() % 250;
    cout << " pos= " << pos << endl;

    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 1, memory, &attr));
        if (a_count == pos) {
            cout << a_count << " a_count == pos remove audio track!!!!!!!!!" << endl;
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 1));
            ASSERT_NE(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 1, memory, &attr));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
            break;
        }
        a_count++;
    }

    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_4500
 * @tc.name      : start demux bufore add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_4500, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    srand(time(NULL));
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
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7000
 * @tc.name      : demuxer MP4 ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7000, TestSize.Level0)
{
    OH_AVFormat *trackFormat2 = nullptr;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    cout << OH_AVFormat_DumpInfo(sourceFormat) << "sourceFormat" << endl;

    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    cout << OH_AVFormat_DumpInfo(trackFormat) << "trackformat1" << endl;

    trackFormat2 = OH_AVSource_GetTrackFormat(source, 0);
    cout << OH_AVFormat_DumpInfo(trackFormat2) << "trackformat0" << endl;

    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7100
 * @tc.name      : demuxer MP4 ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7100, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &trackCount));
    ASSERT_EQ(trackCount, 2);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7200
 * @tc.name      : demuxer MP4 ,GetAudioTrackFormat,MD_KEY_AAC_IS_ADTS
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7200, TestSize.Level0)
{
    const char *string_val;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_AAC_IS_ADTS, &string_val));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7300
 * @tc.name      : demuxer MP4 ,GetAudioTrackFormat ,MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7300, TestSize.Level0)
{
    int32_t br = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 319999);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7400
 * @tc.name      : demuxer MP4 ,GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7400, TestSize.Level0)
{
    int32_t cc = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));

    ASSERT_EQ(cc, 2);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7500
 * @tc.name      : demuxer MP4 ,GetAudioTrackFormat,MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7500, TestSize.Level0)
{
    int32_t sr = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7600
 * @tc.name      : demuxer MP4 ,GetVideoTrackFormat,MD_KEY_HEIGHT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7600, TestSize.Level0)
{
    int32_t height = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 0);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7700
 * @tc.name      : demuxer MP4 ,GetVideoTrackFormat,MD_KEY_WIDTH
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7700, TestSize.Level0)
{
    int32_t weight = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &weight));
    ASSERT_EQ(weight, 0);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7800
 * @tc.name      : demuxer MP4 GetPublicTrackFormat,MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7800, TestSize.Level0)
{
    const char *string_val;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &string_val));
    ASSERT_EQ(0, strcmp(string_val, "video/mp4v-es"));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_7900
 * @tc.name      : demuxer MP4 ,GetPublicTrackFormat,MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_7900, TestSize.Level0)
{
    int32_t type = 0;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));

    ASSERT_EQ(type, 0);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8000
 * @tc.name      : demuxer MP4 ,check source format,OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8000, TestSize.Level0)
{
    const char *string_val;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &string_val));
    ASSERT_EQ(0, strcmp(string_val, "title"));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8100
 * @tc.name      : demuxer MP4 ,check source format,OH_MD_KEY_ALBUM
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8100, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &string_val));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8200
 * @tc.name      : demuxer MP4 ,check source format,OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8200, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &string_val));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8300
 * @tc.name      : demuxer MP4 ,check track format,OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8300, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &string_val));
    ASSERT_EQ(0, strcmp(string_val, "2023"));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8400
 * @tc.name      : demuxer MP4 ,check track format,OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8400, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &string_val));
    ASSERT_EQ(0, strcmp(string_val, "COMMENT"));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8500
 * @tc.name      : demuxer MP4 ,check track format,OH_MD_KEY_GENRE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8500, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_GENRE, &string_val));
    ASSERT_EQ(0, strcmp(string_val, "Classical"));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8600
 * @tc.name      : demuxer MP4 ,check track format,OH_MD_KEY_COPYRIGHT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8600, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COPYRIGHT, &string_val));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8700
 * @tc.name      : demuxer MP4 ,check track format,OH_MD_KEY_LANGUAGE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8700, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_LANGUAGE, &string_val));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8800
 * @tc.name      : demuxer MP4 ,check track format,OH_MD_KEY_DESCRIPTION
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8800, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DESCRIPTION, &string_val));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_8800
 * @tc.name      : demuxer MP4 ,check track format,OH_MD_KEY_LYRICS
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_8900, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_LYRICS, &string_val));
    close(fd);
}

/**
 * @tc.number    : DEMUXER_FUNCTION_9000
 * @tc.name      : demuxer MP4 ,check source format,OH_MD_KEY_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFuncNdkTest, DEMUXER_FUNCTION_9000, TestSize.Level0)
{
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *string_val;
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &string_val));
    ASSERT_EQ(0, strcmp(string_val, "sam"));
    close(fd);
}
