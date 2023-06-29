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

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <thread>
#include <securec.h>

namespace OHOS {
namespace Media {
class DemuxerReliNdkTest : public testing::Test {
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
static OH_AVErrCode ret = AV_ERR_OK;
const char *URI2 = "http://192.168.3.11:8080/share/audio/AAC_48000_1.aac";
const char *URI1 = "http://192.168.3.11:8080/share/audio/MP3_48000_1.mp3";
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
static int32_t g_maxThread = 16;
OH_AVSource *source_list[16] = {};
OH_AVMemory *memory_list[16] = {};
OH_AVDemuxer *demuxer_list[16] = {};
int g_fdList[16] = {};
int32_t g_track = 2;

void DemuxerReliNdkTest::SetUpTestCase() {}
void DemuxerReliNdkTest::TearDownTestCase() {}
void DemuxerReliNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerReliNdkTest::TearDown()
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

    for (int i = 0; i < g_maxThread; i++) {
        if (demuxer_list[i] != nullptr) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_Destroy(demuxer_list[i]));
            demuxer_list[i] = nullptr;
        }

        if (source_list[i] != nullptr) {
            ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source_list[i]));
            source_list[i] = nullptr;
        }
        if (memory_list[i] != nullptr) {
            ASSERT_EQ(AV_ERR_OK, OH_AVMemory_Destroy(memory_list[i]));
            memory_list[i] = nullptr;
        }
        std::cout << i << "            finish Destroy!!!!" << std::endl;

        close(g_fdList[i]);
    }
}
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace{
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

void DemuxFunc(int i, int loop)
{
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    OH_AVCodecBufferAttr attr;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer_list[i], 0));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer_list[i], 1));
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_track; index++) {
            if ((audioIsEnd && (index == 0)) || (videoIsEnd && (index == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer_list[i], index, memory_list[i], &attr));

            if ((index == 0) && (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                audioIsEnd = true;
            }
            if ((index == 1) && (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                videoIsEnd = true;
            }
        }
    }
}

/**
 * @tc.number    : DEMUXER_RELI_0100
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_1000, TestSize.Level3)
{
    int len = 256;
    int num = 0;
    vector<std::thread> vecThread;
    for (int i = 0; i < g_maxThread; i++) {
        memory_list[i] = OH_AVMemory_Create(g_width * g_height);
        char file[256] = {};
        sprintf_s(file, len, "/data/test/media/16/%d_video_audio.mp4", i);
        g_fdList[i] = open(file, O_RDONLY);
        int64_t size = GetFileSize(file);
        cout << file << "----------------------" << g_fdList[i] << "---------" << size << endl;

        source_list[i] = OH_AVSource_CreateWithFD(g_fdList[i], 0, size);
        ASSERT_NE(source_list[i], nullptr);

        demuxer_list[i] = OH_AVDemuxer_CreateWithSource(source_list[i]);
        ASSERT_NE(demuxer_list[i], nullptr);
        vecThread.emplace_back(DemuxFunc, i, num);
    }
    for (auto &val : vecThread) {
        val.join();
    }
}

/**
 * @tc.number    : DEMUXER_RELI_0200
 * @tc.name      : create 16 instances repeat create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_0200, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            char file[256] = {};
            sprintf_s(file, len, "/data/test/media/16/%d_video_audio.mp4", i);
            g_fdList[i] = open(file, O_RDONLY);
            int64_t size = GetFileSize(file);
            cout << file << "----------------------" << g_fdList[i] << "---------" << size << endl;

            source_list[i] = OH_AVSource_CreateWithFD(g_fdList[i], 0, size);
            ASSERT_NE(source_list[i], nullptr);

            demuxer_list[i] = OH_AVDemuxer_CreateWithSource(source_list[i]);
            ASSERT_NE(demuxer_list[i], nullptr);
            vecThread.emplace_back(DemuxFunc, i, num);
        }
        for (auto &val : vecThread) {
            val.join();
        }

        for (int i = 0; i < g_maxThread; i++) {
            if (demuxer_list[i] != nullptr) {
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_Destroy(demuxer_list[i]));
                demuxer_list[i] = nullptr;
            }

            if (source_list[i] != nullptr) {
                ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source_list[i]));
                source_list[i] = nullptr;
            }
            if (memory_list[i] != nullptr) {
                ASSERT_EQ(AV_ERR_OK, OH_AVMemory_Destroy(memory_list[i]));
                memory_list[i] = nullptr;
            }
            std::cout << i << "            finish Destroy!!!!" << std::endl;

            close(g_fdList[i]);
        }
        cout << "num: " << num << endl;
    }
}

/**
 * @tc.number    : DEMUXER_RELI_0300
 * @tc.name      : create 17 instances,17 failed
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_0300, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    int64_t size = 0;
    vector<std::thread> vecThread;
    for (int i = 0; i < g_maxThread; i++) {
        memory_list[i] = OH_AVMemory_Create(g_width * g_height);
        char file[256] = {};
        sprintf_s(file, len, "/data/test/media/16/%d_video_audio.mp4", i);
        g_fdList[i] = open(file, O_RDONLY);
        size = GetFileSize(file);
        cout << file << "----------------------" << g_fdList[i] << "---------" << size << endl;

        source_list[i] = OH_AVSource_CreateWithFD(g_fdList[i], 0, size);
        ASSERT_NE(source_list[i], nullptr);

        demuxer_list[i] = OH_AVDemuxer_CreateWithSource(source_list[i]);
        ASSERT_NE(demuxer_list[i], nullptr);
        vecThread.emplace_back(DemuxFunc, i, num);
    }
    for (auto &val : vecThread) {
        val.join();
    }

    source = OH_AVSource_CreateWithFD(g_fdList[15], 0, size);
    ASSERT_EQ(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(demuxer, nullptr);
}

/**
 * @tc.number    : DEMUXER_RELI_0400
 * @tc.name      : one instance repeat create destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_0400, TestSize.Level0)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;

    const char *file = "/data/test/media/01_video_audio.mp4";
    while (num < 10) {
        bool audioIsEnd = false;
        bool videoIsEnd = false;

        int fd = open(file, O_RDONLY);
        int64_t size = GetFileSize(file);
        cout << file << "----------------------" << fd << "---------" << size << endl;
        num++;
        cout << num << endl;
        source = OH_AVSource_CreateWithFD(fd, 0, size);
        ASSERT_NE(source, nullptr);

        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);

        for (int32_t index = 0; index < 2; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        while (!audioIsEnd || !videoIsEnd) {
            for (int32_t index = 0; index < 2; index++) {

                if ((audioIsEnd && (index == 0)) || (videoIsEnd && (index == 1))) {
                    continue;
                }
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

                if ((index == 0) && (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                    audioIsEnd = true;
                    cout << "    audio is end !!!!!!!!!!!!!!!" << endl;
                }
                if ((index == 1) && (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                    videoIsEnd = true;
                    cout << "   video is end !!!!!!!!!!!!!!!" << endl;
                }
            }
        }
        if (source != nullptr) {
            OH_AVSource_Destroy(source);
            source = nullptr;
        }
        if (demuxer != nullptr) {
            OH_AVDemuxer_Destroy(demuxer);
            demuxer = nullptr;
        }
        close(fd);
    }
}

/**
 * @tc.number    : DEMUXER_RELI_0500
 * @tc.name      : one instance demux long file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_0500, TestSize.Level3)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;

    const char *file = "/data/test/media/long.mp4";

    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;

    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    num++;
    cout << num << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    for (int32_t index = 0; index < 2; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < 2; index++) {
            if ((audioIsEnd && (index == 0)) || (videoIsEnd && (index == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (index == 0) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << audioFrame << "    audio is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    audioFrame++;
                }
            } else if (index == 1) {
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    videoIsEnd = true;
                    cout << videoFrame << "   video is end !!!!!!!!!!!!!!!" << endl;
                } else {
                    videoFrame++;
                }
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_RELI_0100
 * @tc.name      : OH_AVSource_CreateWithURI Repeat Call
 * @tc.desc      : api test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_0100, TestSize.Level2)
{
    OH_AVSource *source1 = OH_AVSource_CreateWithURI(const_cast<char *>(URI1));
    cout << URI1 << "-----------------------" << endl;
    ASSERT_NE(source1, nullptr);
    OH_AVSource *source2 = OH_AVSource_CreateWithURI(const_cast<char *>(URI2));
    cout << URI2 << "-----------------------" << endl;
    ASSERT_NE(source2, nullptr);
    ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source1));
    source1 = nullptr;
    ASSERT_EQ(AV_ERR_OK, OH_AVSource_Destroy(source2));
    source2 = nullptr;
}

/**
 * @tc.number    : DEMUXER_RELI_4900
 * @tc.name      : create source with uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_4900, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/audio/MP3_48000_1.mp3";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
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
 * @tc.number    : DEMUXER_RELI_0500
 * @tc.name      : create source with uri,aac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5000, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/audio/AAC_48000_1.aac";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int audioFrame = 0;
    int keyCount = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            } else {
                audioFrame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(audioFrame, 9457);
    ASSERT_EQ(keyCount, 9457);
}

/**
 * @tc.number    : DEMUXER_RELI_5100
 * @tc.name      : create source with uri,flac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5100, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/audio/FLAC_48000_1.flac";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int audioFrame = 0;
    int keyCount = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            } else {
                audioFrame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(audioFrame, 2288);
    ASSERT_EQ(keyCount, 2288);
}

/**
 * @tc.number    : DEMUXER_RELI_5200
 * @tc.name      : create source with uri,m4a
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5200, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/audio/M4A_48000_1.m4a";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int audioFrame = 0;
    int keyCount = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            } else {
                audioFrame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(audioFrame, 10293);
    ASSERT_EQ(keyCount, 10293);
}

/**
 * @tc.number    : DEMUXER_RELI_5300
 * @tc.name      : create source with uri,mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5300, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/audio/MP3_48000_1.mp3";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int audioFrame = 0;
    int keyCount = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            } else {
                audioFrame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(audioFrame, 9150);
    ASSERT_EQ(keyCount, 9150);
}

/**
 * @tc.number    : DEMUXER_RELI_5400
 * @tc.name      : create source with uri,ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5400, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/audio/OGG_48000_1.ogg";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int audioFrame = 0;
    int keyCount = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            } else {
                audioFrame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(audioFrame, 11439);
    ASSERT_EQ(keyCount, 11439);
}

/**
 * @tc.number    : DEMUXER_RELI_5500
 * @tc.name      : create source with uri,wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5500, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/audio/wav_48000_1.wav";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    int audioFrame = 0;
    int keyCount = 0;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            } else {
                audioFrame++;
                if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    keyCount++;
                }
            }
        }
    }
    ASSERT_EQ(audioFrame, 5146);
    ASSERT_EQ(keyCount, 5146);
}

/**
 * @tc.number    : DEMUXER_RELI_5600
 * @tc.name      : create source with uri,mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5600, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/01_video_audio.mp4";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            }
        }
    }
}

/**
 * @tc.number    : DEMUXER_RELI_5700
 * @tc.name      : create source with uri,hvcc mp4
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5700, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;
    const char *URI = "http://192.168.3.11:8080/share/hvcc_1920x1080_60.mp4";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            }
        }
    }
}

/**
 * @tc.number    : DEMUXER_RELI_5800
 * @tc.name      : create source with uri,wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5800, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/avcc_10sec.mp4";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            }
        }
    }
}

/**
 * @tc.number    : DEMUXER_RELI_5900
 * @tc.name      : create source with uri,wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_5900, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;

    const char *URI = "http://192.168.3.11:8080/share/test_video.ts";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            cout << attr.size << "size---------------pts:" << attr.pts << endl;
            if (attr.flags == OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "isend !!!!!!!!!!!!!!!" << endl;
            }
        }
    }
}

/**
 * @tc.number    : DEMUXER_RELI_6000
 * @tc.name      : create source with uri,wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReliNdkTest, DEMUXER_RELI_6000, TestSize.Level0)
{
    const char *URI = "http://192.168.3.11:8080/share/zero_track.mp4";
    cout << URI << "------" << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(URI));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 0);

    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
}
}