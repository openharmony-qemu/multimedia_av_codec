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
}
} // namespace

namespace {
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
} // namespace