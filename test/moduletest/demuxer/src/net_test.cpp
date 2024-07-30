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
static std::shared_ptr<AVSource> source = nullptr;
static std::shared_ptr<AVDemuxer> demuxer = nullptr;
} // namespace

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
static int32_t g_maxThread = 16;
OH_AVSource *source_list[16] = {};
OH_AVMemory *memory_list[16] = {};
OH_AVDemuxer *demuxer_list[16] = {};
int g_fdList[16] = {};
OH_AVBuffer *avBuffer_list[16] = {};

void DemuxerNetNdkTest::SetUpTestCase() {}
void DemuxerNetNdkTest::TearDownTestCase() {}
void DemuxerNetNdkTest::SetUp() {}
void DemuxerNetNdkTest::TearDown()
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }

    if (source != nullptr) {
        source = nullptr;
    }

    if (demuxer != nullptr) {
        demuxer = nullptr;
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
} // namespace