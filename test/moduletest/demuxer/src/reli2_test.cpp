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

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <thread>
#include <securec.h>

namespace OHOS {
namespace Media {
class DemuxerReli2NdkTest : public testing::Test {
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

static int g_fd = -1;
static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
static int32_t g_maxThread = 16;
OH_AVSource *source_list[16] = {};
OH_AVMemory *memory_list[16] = {};
OH_AVDemuxer *demuxer_list[16] = {};
int g_fdList[16] = {};
int32_t g_track = 2;

void DemuxerReli2NdkTest::SetUpTestCase() {}
void DemuxerReli2NdkTest::TearDownTestCase() {}
void DemuxerReli2NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerReli2NdkTest::TearDown()
{
    if (g_fd > 0) {
        close(g_fd);
        g_fd = -1;
    }

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
namespace {
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
            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer_list[i], index, memory_list[i], &attr));

            if ((index == MEDIA_TYPE_AUD) && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                audioIsEnd = true;
            }
            if ((index == MEDIA_TYPE_VID) && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                videoIsEnd = true;
            }
        }
    }
}

static void SetAudioValue(OH_AVCodecBufferAttr attr, bool &audioIsEnd, int &audioFrame)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        audioIsEnd = true;
        cout << audioFrame << "    audio is end !!!!!!!!!!!!!!!" << endl;
    } else {
        audioFrame++;
    }
}

static void SetVideoValue(OH_AVCodecBufferAttr attr, bool &videoIsEnd, int &videoFrame)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        videoIsEnd = true;
        cout << videoFrame << "   video is end !!!!!!!!!!!!!!!" << endl;
    } else {
        videoFrame++;
        cout << "video track !!!!!" << endl;
    }
}

/**
 * @tc.number    : DEMUXER_RELI_9200
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_RELI_9200, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            char file[256] = {};
            sprintf_s(file, len, "/data/test/media/16/%d_creat_destroy.mov", i);
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
 * @tc.number    : DEMUXER_RELI_9300
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_RELI_9300, TestSize.Level3)
{
    int num = 0;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        const char *uri = "http://192.168.3.17:8080/share/MOV_H264_baseline@level5_1920_1080_30_AAC_48K_1.mov";
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            cout << i << "  uri:  " << uri << endl;
            source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
        }
        cout << "num: " << num << endl;
    }
}


/**
 * @tc.number    : DEMUXER_RELI_9400
 * @tc.name      : one instance demux long file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_RELI_9400, TestSize.Level3)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;

    const char *file = "/data/test/media/long.mov";

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
            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (index == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame);
            } else if (index == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame);
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_RELI_9500
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_RELI_9500, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            char file[256] = {};
            sprintf_s(file, len, "/data/test/media/16/%d_creat_destroy.mpg", i);
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
 * @tc.number    : DEMUXER_RELI_9600
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_RELI_9600, TestSize.Level3)
{
    int num = 0;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        const char *uri = "http://192.168.3.17:8080/share/MPG_H264_baseline@level5_1920_1080_30_MP2_44.1K_1.mpg";
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            cout << i << "  uri:  " << uri << endl;
            source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
        }
        cout << "num: " << num << endl;
    }
}


/**
 * @tc.number    : DEMUXER_RELI_9700
 * @tc.name      : one instance demux long file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_RELI_9700, TestSize.Level3)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;

    const char *file = "/data/test/media/long.mpg";

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
            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (index == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame);
            } else if (index == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame);
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_AVI_RELI_0100
 * @tc.name      : one fd instance repeat create destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_AVI_RELI_0100, TestSize.Level2)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;

    const char *file = "/data/test/media/AVI_MPEG4_advanced_simple@level3_352_288_MP3_2.avi";
    bool audioIsEnd = false;
    bool videoIsEnd = false;

    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    num++;
    cout << num << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    for (int32_t index = 0; index < 2; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < 2; index++) {

            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if ((index == MEDIA_TYPE_AUD) && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                audioIsEnd = true;
                cout << "    audio is end !!!!!!!!!!!!!!!" << endl;
            }
            if ((index == MEDIA_TYPE_VID) && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
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
    close(g_fd);
}

/**
 * @tc.number    : DEMUXER_AVI_RELI_0200
 * @tc.name      : one uri instance repeat create destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_AVI_RELI_0200, TestSize.Level2)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;

    const char *file = "http://192.168.3.17:8080/share/AVI_MPEG2_422P@high_1920_1080_PCM_s24_1.avi";
    bool audioIsEnd = false;
    bool videoIsEnd = false;

    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    num++;
    cout << num << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    for (int32_t index = 0; index < 2; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < 2; index++) {

            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if ((index == MEDIA_TYPE_AUD) && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                audioIsEnd = true;
                cout << "    audio is end !!!!!!!!!!!!!!!" << endl;
            }
            if ((index == MEDIA_TYPE_VID) && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
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
    close(g_fd);
}

/**
 * @tc.number    : DEMUXER_AVI_RELI_0300
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_AVI_RELI_0300, TestSize.Level3)
{
    int len = 256;
    int num = 0;
    vector<std::thread> vecThread;
    for (int i = 0; i < g_maxThread; i++) {
        memory_list[i] = OH_AVMemory_Create(g_width * g_height);
        char file[256] = {};
        sprintf_s(file, len, "/data/test/media/16/%d_video_audio.avi", i);
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
}

/**
 * @tc.number    : DEMUXER_AVI_RELI_0400
 * @tc.name      : create 16 instances repeat create-destory with avi fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_AVI_RELI_0400, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            char file[256] = {};
            sprintf_s(file, len, "/data/test/media/16/%d_video_audio.avi", i);
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
 * @tc.number    : DEMUXER_AVI_RELI_0500
 * @tc.name      : create 16 instances create-destory with avi uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_AVI_RELI_0500, TestSize.Level3)
{
    int num = 0;
    vector<std::thread> vecThread;
    for (int i = 0; i < g_maxThread; i++) {
            char uri[256];
            sprintf_s(uri, "http://192.168.3.17:8080/share/16/%d_video_audio.avi", i);
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            cout << i << "  uri:  " << uri << endl;
            source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
}

/**
 * @tc.number    : DEMUXER_AVI_RELI_0600
 * @tc.name      : create 16 instances repeat create-destory with avi uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_AVI_RELI_0600, TestSize.Level3)
{
    int num = 0;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            char uri[256];
            sprintf_s(uri, "http://192.168.3.17:8080/share/16/%d_video_audio.avi", i);
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            cout << i << "  uri:  " << uri << endl;
            source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
        }
        cout << "num: " << num << endl;
    }
}

/**
 * @tc.number    : DEMUXER_AVI_RELI_0700
 * @tc.name      : one instance demux long file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_AVI_RELI_0700, TestSize.Level3)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/long.avi";
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    num++;
    cout << num << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);

    for (int32_t index = 0; index < 2; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < 2; index++) {
            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (index == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame);
            } else if (index == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame);
            }
        }
    }
    close(g_fd);
}

/**
 * @tc.number    : DEMUXER_MTS_RELI_0100
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_MTS_RELI_0100, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            char file[256] = {};
            sprintf_s(file, len, "/data/test/media/16/%d_creat_destroy.mts", i);
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
 * @tc.number    : DEMUXER_M2TS_RELI_0100
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_M2TS_RELI_0100, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            char file[256] = {};
            sprintf_s(file, len, "/data/test/media/16/%d_creat_destroy.m2ts", i);
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
 * @tc.number    : DEMUXER_TRP_RELI_0100
 * @tc.name      : create 16 instances create-destory
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_TRP_RELI_0100, TestSize.Level3)
{
    int num = 0;
    int len = 256;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            char file[256] = {};
            sprintf_s(file, len, "/data/test/media/16/%d_creat_destroy.trp", i);
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
 * @tc.number    : DEMUXER_MTS_RELI_0200
 * @tc.name      : create 16 instances repeat create-destory with mts uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_MTS_RELI_0200, TestSize.Level3)
{
    int num = 0;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            char uri[256];
            sprintf_s(uri, "http://192.168.1.100:8080/share/three/media/16/%d_creat_destroy.mts", i);
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            cout << i << "  uri:  " << uri << endl;
            source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
        }
        cout << "num: " << num << endl;
    }
}

/**
 * @tc.number    : DEMUXER_M2TS_RELI_0200
 * @tc.name      : create 16 instances repeat create-destory with m2ts uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_M2TS_RELI_0200, TestSize.Level3)
{
    int num = 0;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            char uri[256];
            sprintf_s(uri, "http://192.168.1.100:8080/share/three/media/16/%d_creat_destroy.m2ts", i);
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            cout << i << "  uri:  " << uri << endl;
            source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
        }
        cout << "num: " << num << endl;
    }
}

/**
 * @tc.number    : DEMUXER_TRP_RELI_0200
 * @tc.name      : create 16 instances repeat create-destory with trp uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_TRP_RELI_0200, TestSize.Level3)
{
    int num = 0;
    while (num < 10) {
        num++;
        vector<std::thread> vecThread;
        for (int i = 0; i < g_maxThread; i++) {
            char uri[256];
            sprintf_s(uri, "http://192.168.1.100:8080/share/three/media/16/%d_creat_destroy.trp", i);
            memory_list[i] = OH_AVMemory_Create(g_width * g_height);
            cout << i << "  uri:  " << uri << endl;
            source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
        }
        cout << "num: " << num << endl;
    }
}

/**
 * @tc.number    : DEMUXER_MTS_RELI_0300
 * @tc.name      : one instance demux long file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_MTS_RELI_0300, TestSize.Level3)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/long.mts";

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
            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (index == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame);
            } else if (index == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame);
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_M2TS_RELI_0300
 * @tc.name      : one instance demux long file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_M2TS_RELI_0300, TestSize.Level3)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/long.m2ts";

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
            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (index == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame);
            } else if (index == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame);
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : DEMUXER_TRP_RELI_0300
 * @tc.name      : one instance demux long file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerReli2NdkTest, DEMUXER_TRP_RELI_0300, TestSize.Level3)
{
    int num = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/long.trp";

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
            if ((audioIsEnd && (index == MEDIA_TYPE_AUD)) || (videoIsEnd && (index == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));

            if (index == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame);
            } else if (index == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame);
            }
        }
    }
    close(fd);
}
}