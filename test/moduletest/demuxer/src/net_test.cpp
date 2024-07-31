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

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include <thread>
#include "native_avmemory.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
class DemuxerNetNdkTest : public testing::Test {
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

static int32_t g_maxThread = 16;
OH_AVSource *source_list[16] = {};
OH_AVMemory *memory_list[16] = {};
OH_AVDemuxer *demuxer_list[16] = {};
int g_fdList[16] = {};
OH_AVBuffer *avBuffer_list[16] = {};

constexpr int32_t LAYOUTMONO = 4;
constexpr int32_t LAYOUTDUAL = 3;
constexpr int32_t SAMPLERATEMONO = 8000;
constexpr int32_t SAMPLERATEDUAL = 44100;
constexpr int32_t COUNTMONO = 1;
constexpr int32_t COUNTDUAL = 2;
constexpr int32_t BITRATEMONO = 64000;
constexpr int32_t BITRATEDUAL = 705600;

void DemuxerNetNdkTest::SetUpTestCase() {}
void DemuxerNetNdkTest::TearDownTestCase() {}
void DemuxerNetNdkTest::SetUp() {}
void DemuxerNetNdkTest::TearDown()
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
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
    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
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
        if (avBuffer_list[i] != nullptr) {
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer_list[i]));
            avBuffer_list[i] = nullptr;
        }
        std::cout << i << "            finish Destroy!!!!" << std::endl;
        close(g_fdList[i]);
    }
}
} // namespace

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
    void DemuxFuncWav(int i, int loop)
    {
        bool audioIsEnd = false;
        OH_AVCodecBufferAttr bufferAttr;
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer_list[i], 0));
        int index = 0;
        while (!audioIsEnd) {
            if (audioIsEnd && (index == OH_MediaType::MEDIA_TYPE_AUD)) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer_list[i], index, avBuffer_list[i]));
            ASSERT_NE(avBuffer_list[i], nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer_list[i], &bufferAttr));
            if ((index == OH_MediaType::MEDIA_TYPE_AUD) &&
             (bufferAttr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS)) {
                audioIsEnd = true;
            }
        }
    }

    static void CheckAudioParam(OH_AVSource *audioSource, int &audioFrameAll)
    {
        int akeyCount = 0;
        int tarckType = 0;
        OH_AVCodecBufferAttr bufferAttr;
        bool audioIsEnd = false;
        int32_t count = 0;
        int32_t rate = 0;
        int64_t bitrate = 0;
        int64_t layout = 0;
        int32_t index = 0;
        const char* mimeType = nullptr;
        while (!audioIsEnd) {
            trackFormat = OH_AVSource_GetTrackFormat(audioSource, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            if (tarckType == OH_MediaType::MEDIA_TYPE_AUD) {
                ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
                ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &rate));
                ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &count));
                ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_CHANNEL_LAYOUT, &layout));
                ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
                if (bufferAttr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                    audioIsEnd = true;
                    cout << audioFrameAll << "    audio is end !!!!!!!!!!!!!!!" << endl;
                    continue;
                }
                audioFrameAll++;
                if (bufferAttr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                    akeyCount++;
                }
            }
        }
        if (count == 1) {
            ASSERT_EQ(0, strcmp(mimeType, "audio/g711mu"));
            ASSERT_EQ(layout, LAYOUTMONO);
            ASSERT_EQ(rate, SAMPLERATEMONO);
            ASSERT_EQ(count, COUNTMONO);
            ASSERT_EQ(bitrate, BITRATEMONO);
        } else {
            ASSERT_EQ(0, strcmp(mimeType, "audio/g711mu"));
            ASSERT_EQ(layout, LAYOUTDUAL);
            ASSERT_EQ(rate, SAMPLERATEDUAL);
            ASSERT_EQ(count, COUNTDUAL);
            ASSERT_EQ(bitrate, BITRATEDUAL);
        }
        cout << akeyCount << "---akeyCount---" << endl;
    }
    /**
     * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0110
     * @tc.name      : demuxer timed metadata with 1 meta track and video track uri-meta track at 0
     * @tc.desc      : func test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0110, TestSize.Level1)
    {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        const char *uri = "http://192.168.3.11:8080/share/Timedmetadata1Track0.mp4";
        ASSERT_EQ(demuxerSample->InitWithFile(uri, false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
        ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(0, 1), 0);
        ASSERT_EQ(demuxerSample->CheckTimedMeta(0), 0);
    }

    /**
     * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0120
     * @tc.name      : demuxer timed metadata with 1 meta track and video track uri-meta track at 1
     * @tc.desc      : func test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0120, TestSize.Level1)
    {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        const char *uri = "http://192.168.3.11:8080/share/Timedmetadata1Track1.mp4";
        ASSERT_EQ(demuxerSample->InitWithFile(uri, false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
        ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(1, 0), 0);
        ASSERT_EQ(demuxerSample->CheckTimedMeta(1), 0);
    }

    /**
     * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0130
     * @tc.name      : demuxer timed metadata with 1 meta track and video track uri-meta track at 2
     * @tc.desc      : func test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0130, TestSize.Level1)
    {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        const char *uri = "http://192.168.3.11:8080/share/Timedmetadata1Track2.mp4";
        ASSERT_EQ(demuxerSample->InitWithFile(uri, false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
        ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(2, 0), 0);
        ASSERT_EQ(demuxerSample->CheckTimedMeta(2), 0);
    }

    /**
     * @tc.number    : DEMUXER_TIMED_META_INNER_FUNC_0140
     * @tc.name      : demuxer timed metadata with 2 meta track and video track uri
     * @tc.desc      : func test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_TIMED_META_INNER_FUNC_0140, TestSize.Level1)
    {
        auto demuxerSample = make_unique<InnerDemuxerSample>();
        const char *uri = "http://192.168.3.11:8080/share/Timedmetadata2Track2.mp4";
        ASSERT_EQ(demuxerSample->InitWithFile(uri, false), AVCS_ERR_OK);
        ASSERT_EQ(demuxerSample->CheckHasTimedMeta(), 1);
        ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(2, 0), 0);
        ASSERT_EQ(demuxerSample->CheckTimedMetaFormat(3, 0), 0);
        ASSERT_EQ(demuxerSample->CheckTimedMeta(3), 0);
    }
    /**
     * @tc.number    : DEMUXER_FUNC_NET_001
     * @tc.name      : create 16 instances repeat create-destory with wav file
     * @tc.desc      : function test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_FUNC_NET_001, TestSize.Level2)
    {
        int num = 0;
        int len = 256;
        while (num < 10) {
            num++;
            vector<std::thread> vecThread;
            for (int i = 0; i < g_maxThread; i++) {
                char file[256] = {};
                sprintf_s(file, len, "/data/test/media/16/%d_wav_audio_test_202406290859.wav", i);
                g_fdList[i] = open(file, O_RDONLY);
                int64_t size = GetFileSize(file);
                cout << file << "----------------------" << g_fdList[i] << "---------" << size << endl;
                avBuffer_list[i] = OH_AVBuffer_Create(size);
                ASSERT_NE(avBuffer_list[i], nullptr);
                source_list[i] = OH_AVSource_CreateWithFD(g_fdList[i], 0, size);
                ASSERT_NE(source_list[i], nullptr);
                demuxer_list[i] = OH_AVDemuxer_CreateWithSource(source_list[i]);
                ASSERT_NE(demuxer_list[i], nullptr);
                vecThread.emplace_back(DemuxFuncWav, i, num);
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
                if (avBuffer_list[i] != nullptr) {
                    ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer_list[i]));
                    avBuffer_list[i] = nullptr;
                }
                std::cout << i << "            finish Destroy!!!!" << std::endl;
                close(g_fdList[i]);
            }
            cout << "num: " << num << endl;
        }
    }

    /**
     * @tc.number    : DEMUXER_FUNC_NET_002
     * @tc.name      : create 16 instances repeat create-destory with wav network file
     * @tc.desc      : function test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_FUNC_NET_002, TestSize.Level3)
    {
        int num = 0;
        int sizeinfo = 421888;
        while (num < 10) {
            num++;
            vector<std::thread> vecThread;
            const char *uri = "http://192.168.3.11:8080/share/audio/audio/wav_audio_test_202406290859.wav";
            for (int i = 0; i < g_maxThread; i++) {
                avBuffer_list[i] = OH_AVBuffer_Create(sizeinfo);
                ASSERT_NE(avBuffer_list[i], nullptr);
                cout << i << "  uri:  " << uri << endl;
                source_list[i] = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
                ASSERT_NE(source_list[i], nullptr);
                demuxer_list[i] = OH_AVDemuxer_CreateWithSource(source_list[i]);
                ASSERT_NE(demuxer_list[i], nullptr);
                vecThread.emplace_back(DemuxFuncWav, i, num);
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
                if (avBuffer_list[i] != nullptr) {
                    ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer_list[i]));
                    avBuffer_list[i] = nullptr;
                }
                std::cout << i << "            finish Destroy!!!!" << std::endl;
            }
            cout << "num: " << num << endl;
        }
    }
    /**
     * @tc.number    : DEMUXER_FUNC_NET_003
     * @tc.name      : create pcm-mulaw wav demuxer with network file
     * @tc.desc      : function test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_FUNC_NET_003, TestSize.Level2)
    {
        int audioFrame = 0;
        int sizeinfo = 421888;
        const char *uri = "http://192.168.3.11:8080/share/audio/audio/wav_audio_test_202406290859.wav";
        source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        avBuffer = OH_AVBuffer_Create(sizeinfo);
        ASSERT_NE(avBuffer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        ASSERT_EQ(1, g_trackCount);
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        CheckAudioParam(source, audioFrame);
        ASSERT_EQ(103, audioFrame);
        cout << "-----------audioFrame-----------" << audioFrame << endl;
    }
    /**
     * @tc.number    : DEMUXER_FUNC_NET_004
     * @tc.name      : create pcm-mulaw wav demuxer with Mono channel uri file
     * @tc.desc      : function test
     */
    HWTEST_F(DemuxerNetNdkTest, DEMUXER_FUNC_NET_004, TestSize.Level2)
    {
        int sizeinfo = 28672;
        int audioFrame = 0;
        const char *uri = "http://192.168.3.11:8080/share/audio/audio/7FBD5E21-503C-41A8-83B4-34548FC01562.wav";
        source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
        ASSERT_NE(source, nullptr);
        demuxer = OH_AVDemuxer_CreateWithSource(source);
        ASSERT_NE(demuxer, nullptr);
        avBuffer = OH_AVBuffer_Create(sizeinfo);
        ASSERT_NE(avBuffer, nullptr);
        sourceFormat = OH_AVSource_GetSourceFormat(source);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
        ASSERT_EQ(1, g_trackCount);
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        }
        CheckAudioParam(source, audioFrame);
        ASSERT_EQ(7, audioFrame);
        cout << "-----------audioFrame-----------" << audioFrame << endl;
    }

} // namespace