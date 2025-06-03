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

#include "avdemuxer.h"
#include "avsource.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "buffer/avsharedmemory.h"
#include "buffer/avsharedmemorybase.h"
#include "securec.h"
#include "inner_demuxer_sample.h"

#include "common/status.h"
#include "avdemuxer.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
class DemuxerCacheFuncNdkTest : public testing::Test {
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

void DemuxerCacheFuncNdkTest::SetUpTestCase() {}
void DemuxerCacheFuncNdkTest::TearDownTestCase() {}
void DemuxerCacheFuncNdkTest::SetUp() {}
void DemuxerCacheFuncNdkTest::TearDown() {}
} // namespace

namespace {
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0010
 * @tc.name      : No track ID is selected
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0010, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->indexVid = 0;
    demuxerSample->unSelectTrack = 0;
    uint32_t memoryUsage = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/audio/gb2312.mp3", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, demuxerSample->demuxer_->GetCurrentCacheSize(
                                                demuxerSample->unSelectTrack, memoryUsage));
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0020
 * @tc.name      : The entered track ID is not selected
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0020, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->indexVid = 0;
    demuxerSample->unSelectTrack = 2;
    uint32_t memoryUsage = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, demuxerSample->demuxer_->GetCurrentCacheSize(
                                        demuxerSample->unSelectTrack, memoryUsage));
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0030
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0030, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0, 0},
        {1, 218, 0, 0},
        {2, 2, 2, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0040
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the video track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0040, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 326;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0, 0},
        {1, 43671, 43453, 43453},
        {2, 8, 8, 6},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0050
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0050, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 600;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0, 0},
        {1, 80225, 80007, 80007},
        {2, 14, 14, 12},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0060
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0060, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 29661, 9197, 9197},
        {1, 0, 0, 0},
        {2, 2, 2, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0070
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the audio track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0070, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 326;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 4261461, 4240997, 4240997},
        {1, 0, 0, 0},
        {2, 11, 11, 9},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0080
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0080, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 430;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 5462115, 5441651, 5441651},
        {1, 0, 0, 0},
        {2, 14, 14, 12},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0090
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the subtitle track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0090, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 655777, 635313, 635313},
        {1, 8214, 8214, 7996},
        {2, 0, 0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0100
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the subtitle track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0100, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 4197109, 4176645, 4176645},
        {1, 59519, 59519, 59301},
        {2, 0, 0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0110
 * @tc.name      : mp4(1video+1audio+1subtitle), Read the subtitle track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0110, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 12;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    demuxerSample->indexSub = 2;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 5476556, 5456092, 5456092},
        {1, 80412, 80412, 80194},
        {2, 0, 0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/sub_video_audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexSub, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0120
 * @tc.name      : mp4(1video+1audio+1meta), Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0120, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 61765, 61765, 0},
        {1, 0, 0, 0},
        {2, 15, 0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0130
 * @tc.name      : mp4(1video+1audio+1meta), Read the video track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0130, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 100;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 9711576, 9711576, 9649811},
        {1, 0, 0, 0},
        {2, 37428, 37413, 37413},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0140
 * @tc.name      : mp4(1video+1audio+1meta), Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0140, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 210;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 9711576, 9711576, 9649811},
        {1, 0, 0, 0},
        {2, 79252, 79237, 79237},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0150
 * @tc.name      : mp4(1video+1audio+1meta), Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0150, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 97145, 97145, 35380},
        {1, 97147, 35381, 35381},
        {2, 0, 0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0160
 * @tc.name      : mp4(1video+1audio+1meta), Read the audio track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0160, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 100;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 9711576, 9711576, 9649811},
        {1, 4618815, 4557049, 4557049},
        {2, 0, 0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0170
 * @tc.name      : mp4(1video+1audio+1meta), Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0170, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 211;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 9711576, 9711576, 9649811},
        {1, 8320000, 8258234, 8258234},
        {2, 0, 0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0180
 * @tc.name      : mp4(1video+1audio+1meta), Read the meta track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0180, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0, 0},
        {1, 97147, 35381, 35381},
        {2, 15, 15, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0190
 * @tc.name      : mp4(1video+1audio+1meta), Read the meta track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0190, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 100;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0, 0},
        {1, 1463038, 1401272, 1401272},
        {2, 16526, 16526, 16511},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0200
 * @tc.name      : mp4(1video+1audio+1meta), Read the meta track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0200, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 210;
    demuxerSample->indexVid = 1;
    demuxerSample->indexAud = 2;
    demuxerSample->indexData = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0, 0},
        {1, 1502812, 1441046, 1441046},
        {2, 17247, 17247, 17232},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/Timedmetadata1Track0.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexVid, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 2));
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 3));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexData, demuxerSample->avBuffer);
        }
    }
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0210
 * @tc.name      : mp4(1video+9audio), Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0210, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 630, 630},
        {2, 630, 630},
        {3, 630, 630},
        {4, 630, 315},
        {5, 630, 630},
        {6, 630, 630},
        {7, 630, 630},
        {8, 630, 630},
        {9, 630, 630},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0220
 * @tc.name      : mp4(1video+9audio), Read the video track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0220, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 300;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 78437, 78437},
        {2, 78437, 78437},
        {3, 78437, 78437},
        {4, 78437, 78122},
        {5, 78437, 78437},
        {6, 78437, 78437},
        {7, 78437, 78437},
        {8, 78437, 78437},
        {9, 78437, 78437},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0230
 * @tc.name      : mp4(1video+9audio), Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0230, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 600;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 161568, 161568},
        {2, 161568, 161568},
        {3, 161568, 161568},
        {4, 161568, 161253},
        {5, 161568, 161568},
        {6, 161568, 161568},
        {7, 161568, 161568},
        {8, 161568, 161568},
        {9, 161568, 161568},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0240
 * @tc.name      : mp4(1video+9audio), Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0240, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 120297, 4877},
        {1, 630, 630},
        {2, 630, 630},
        {3, 630, 630},
        {4, 0, 0},
        {5, 315, 315},
        {6, 315, 315},
        {7, 315, 315},
        {8, 315, 315},
        {9, 315, 315},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0250
 * @tc.name      : mp4(1video+9audio), Read the audio track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0250, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 200;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 1756633, 1641213},
        {1, 72844, 72844},
        {2, 72844, 72844},
        {3, 72844, 72844},
        {4, 0, 0},
        {5, 72283, 72283},
        {6, 72283, 72283},
        {7, 72283, 72283},
        {8, 72283, 72283},
        {9, 72283, 72283},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0260
 * @tc.name      : mp4(1video+9audio), Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0260, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 433;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 5024021, 4908601},
        {1, 162955, 162955},
        {2, 162955, 162955},
        {3, 162955, 162955},
        {4, 0, 0},
        {5, 162497, 162497},
        {6, 162497, 162497},
        {7, 162497, 162497},
        {8, 162497, 162497},
        {9, 162497, 162497},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0270
 * @tc.name      : mp4(1video+9audio), unselect track8(audio8),Read the audio4 track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0270, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    demuxerSample->unSelectTrack = 8;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 120297, 4877},
        {1, 630, 630},
        {2, 630, 630},
        {3, 630, 630},
        {4, 0, 0},
        {5, 315, 315},
        {6, 315, 315},
        {7, 315, 315},
        {9, 315, 315},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0280
 * @tc.name      : mp4(1video+9audio), unselect track8(audio8),Read the audio4 track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0280, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 200;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    demuxerSample->unSelectTrack = 8;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 1756633, 1641213},
        {1, 72844, 72844},
        {2, 72844, 72844},
        {3, 72844, 72844},
        {4, 0, 0},
        {5, 72283, 72283},
        {6, 72283, 72283},
        {7, 72283, 72283},
        {9, 72283, 72283},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0290
 * @tc.name      : mp4(1video+9audio), unselect track8(audio8),Read the audio4 track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0290, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 433;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 4;
    demuxerSample->unSelectTrack = 8;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 5024021, 4908601},
        {1, 162955, 162955},
        {2, 162955, 162955},
        {3, 162955, 162955},
        {4, 0, 0},
        {5, 162497, 162497},
        {6, 162497, 162497},
        {7, 162497, 162497},
        {9, 162497, 162497},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/video_9audio.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0300
 * @tc.name      : mkv(1video+1audio), Read the video track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0300, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 3, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/opus_h264.mkv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0310
 * @tc.name      : mkv(1video+1audio), Read the video track in the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0310, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 180;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 54019, 54016},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/opus_h264.mkv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0320
 * @tc.name      : mkv(1video+1audio), Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0320, TestSize.Level1)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 372;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 116656, 116653},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/opus_h264.mkv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}

/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0330
 * @tc.name      : mkv(1video+1audio),Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0330, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 2;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 69641, 19550},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/opus_h264.mkv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0340
 * @tc.name      : mkv(1video+1audio),Read the audio track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0340, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 300;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 3227617, 3177526},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/opus_h264.mkv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0350
 * @tc.name      : mkv(1video+1audio),Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0350, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 610;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 6585274, 6535183},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/opus_h264.mkv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0360
 * @tc.name      : ts(1video+1audio),Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0360, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 10;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 2742, 2712},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/aac_mpeg4.ts", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0370
 * @tc.name      : ts(1video+1audio),Read the video track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0370, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 180;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 78573, 78543},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/aac_mpeg4.ts", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0380
 * @tc.name      : ts(1video+1audio),Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0380, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 372;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 163470, 163440},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/aac_mpeg4.ts", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0390
 * @tc.name      : ts(1video+1audio),Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0390, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 10;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 534706, 453412},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/aac_mpeg4.ts", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0400
 * @tc.name      : ts(1video+1audio),Read the audio track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0400, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 300;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 4933778, 4852484},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/aac_mpeg4.ts", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0410
 * @tc.name      : ts(1video+1audio),Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0410, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 528;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 8598818, 8517524},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/aac_mpeg4.ts", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0420
 * @tc.name      : fmp4(1video+1audio),Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0420, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 1671, 1254},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265_fmp4.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0430
 * @tc.name      : fmp4(1video+1audio),Read the video track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0430, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 180;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 93622, 93205},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265_fmp4.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0440
 * @tc.name      : fmp4(1video+1audio),Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0440, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 372;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 194769, 194352},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265_fmp4.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0450
 * @tc.name      : fmp4(1video+1audio),Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0450, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 33263, 248},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265_fmp4.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0460
 * @tc.name      : fmp4(1video+1audio),Read the audio track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0460, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 300;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 1553199, 1520184},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265_fmp4.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0470
 * @tc.name      : fmp4(1video+1audio),Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0470, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 468;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 2419189, 2386174},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/mp3_h265_fmp4.mp4", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0480
 * @tc.name      : flv(1video+1audio),Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0480, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 1260, 478},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hevc_aac_first.flv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0490
 * @tc.name      : flv(1video+1audio),Read the video track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0490, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 300;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 78538, 77756},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hevc_aac_first.flv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0500
 * @tc.name      : flv(1video+1audio),Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0500, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 602;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 165632, 164850},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hevc_aac_first.flv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0510
 * @tc.name      : flv(1video+1audio),Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0510, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 110977, 17555},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hevc_aac_first.flv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0520
 * @tc.name      : flv(1video+1audio),Read the audio track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0520, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 100;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 2157637, 2064215},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hevc_aac_first.flv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0530
 * @tc.name      : flv(1video+1audio),Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0530, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 210;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 5435273, 5341851},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/hevc_aac_first.flv", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0540
 * @tc.name      : mov(1video+1audio),Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0540, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 6912, 5184},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0550
 * @tc.name      : mov(1video+1audio),Read the video track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0550, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 30;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 48384, 46656},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0560
 * @tc.name      : mov(1video+1audio),Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0560, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 60;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 96768, 95040},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0570
 * @tc.name      : mov(1video+1audio),Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0570, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 68577, 5564},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0580
 * @tc.name      : mov(1video+1audio),Read the audio track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0580, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 30;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 206841, 143828},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0590
 * @tc.name      : mov(1video+1audio),Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0590, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 56;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 354199, 291186},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG4_SP@6_1280_720_30_MP2_32K_2.mov", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0600
 * @tc.name      : avi(1video+1audio),Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0600, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 24576, 20480},
    };
    ASSERT_EQ(demuxerSample->InitWithFile(
        "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0610
 * @tc.name      : avi(1video+1audio),Read the video track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0610, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 15;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 86016, 81920},
    };
    ASSERT_EQ(demuxerSample->InitWithFile(
        "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0620
 * @tc.name      : avi(1video+1audio),Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0620, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 30;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 176128, 172032},
    };
    ASSERT_EQ(demuxerSample->InitWithFile(
        "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0630
 * @tc.name      : avi(1video+1audio),Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0630, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 70742, 30302},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile(
        "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0640
 * @tc.name      : avi(1video+1audio),Read the audio track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0640, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 20;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 207760, 167320},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile(
        "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0650
 * @tc.name      : avi(1video+1audio),Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0650, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 44;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 305390, 264950},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile(
        "/data/test/media/AVI_MPEG4_main@level5_720_576_PCM_s32_1.avi", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0660
 * @tc.name      : mpg(1video+1audio),Read the video track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0660, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 4;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 1944, 1728},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0670
 * @tc.name      : mpg(1video+1audio),Read the video track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0670, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 60;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 3888, 3672},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0680
 * @tc.name      : mpg(1video+1audio),Read the video track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0680, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 119;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0, 0},
        {1, 12312, 12096},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadVideo(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0690
 * @tc.name      : mpg(1video+1audio),Read the audio track at the beginning stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0690, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 10;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 94517, 63753},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0700
 * @tc.name      : mpg(1video+1audio),Read the audio track at the middle stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0700, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 30;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 421101, 390337},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0710
 * @tc.name      : mpg(1video+1audio),Read the audio track at the final stage
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0710, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 57;
    demuxerSample->indexVid = 0;
    demuxerSample->indexAud = 1;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 652380, 621616},
        {1, 0, 0},
    };
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/MPEG2_422p_1280_720_60_MP3_32K_1.mpg", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    ASSERT_EQ(demuxerSample->ReadAudio(cacheCheckSteps), true);
}
/**
 * @tc.number    : DEMUXER_CACHESIZE_INNER_FUNC_0720
 * @tc.name      : mp3(1audio),Read the audio,get audio ,size = 0
 * @tc.desc      : func test
 */
HWTEST_F(DemuxerCacheFuncNdkTest, DEMUXER_CACHESIZE_INNER_FUNC_0720, TestSize.Level3)
{
    auto demuxerSample = make_unique<InnerDemuxerSample>();
    demuxerSample->readPos = 10;
    demuxerSample->indexAud = 0;
    std::vector<std::vector<int32_t>> cacheCheckSteps = {
        {0, 0},
    };
    int32_t readCount = 0;
    ASSERT_EQ(demuxerSample->InitWithFile("/data/test/media/audio/gb2312.mp3", true), AVCS_ERR_OK);
    ASSERT_EQ(demuxerSample->CreateBuffer(), true);
    while (true)
    {
        if (readCount >= demuxerSample->readPos)
        {
            ASSERT_EQ(true, demuxerSample->CheckCache(cacheCheckSteps, 1));
            break;
        } else {
            readCount++;
            demuxerSample->demuxer_->ReadSampleBuffer(demuxerSample->indexAud, demuxerSample->avBuffer);
        }
    }
}
}
