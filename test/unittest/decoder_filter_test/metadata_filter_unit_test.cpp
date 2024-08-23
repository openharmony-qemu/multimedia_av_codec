/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include <ctime>
#include <sys/time.h>
#include "sync_fence.h"
#include "filter/filter_factory.h"
#include "avcodec_info.h"
#include "avcodec_common.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "metadata_filter.h"

#include "metadata_filter_unit_test.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
void MetaDataFilterUnitTest::SetUpTestCase(void)
{
}

void MetaDataFilterUnitTest::TearDownTestCase(void)
{
}

void MetaDataFilterUnitTest::SetUp()
{
}

void MetaDataFilterUnitTest::TearDown()
{
}

HWTEST_F(MetaDataFilterUnitTest, MetaDataFilter_001, TestSize.Level1)
{
    std::shared_ptr<Pipeline::MetaDataFilter> metaData =
        std::make_shared<Pipeline::MetaDataFilter>("MetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
    std::shared_ptr<TestEventReceiver> eventReceive = std::make_shared<TestEventReceiver>();
    std::shared_ptr<TestFilterCallback> filterCallback = std::make_shared<TestFilterCallback>();
    metaData->Init(eventReceive, filterCallback);

    std::shared_ptr<Meta> format;
    std::shared_ptr<AVBufferQueue> inputBufferQueue =
	    AVBufferQueue::Create(8, MemoryType::SHARED_MEMORY, "testInputBufferQueue");
    sptr<AVBufferQueueProducer> inputBufferQueueProducer = inputBufferQueue->GetProducer();
    metaData->OnLinkedResult(inputBufferQueueProducer, format);

    EXPECT_EQ(metaData->SetCodecFormat(format), Status::OK);
    EXPECT_EQ(metaData->Configure(format), Status::OK);
    EXPECT_EQ(metaData->SetInputMetaSurface(nullptr), Status::ERROR_UNKNOWN);
    EXPECT_EQ(metaData->SetInputMetaSurface(metaData->GetInputMetaSurface()), Status::OK);
    EXPECT_EQ(metaData->DoPrepare(), Status::OK);
    EXPECT_EQ(metaData->DoStart(), Status::OK);
    metaData->OnBufferAvailable();
    EXPECT_EQ(metaData->DoPause(), Status::OK);
    EXPECT_EQ(metaData->DoResume(), Status::OK);
    EXPECT_EQ(metaData->DoFlush(), Status::OK);
    EXPECT_EQ(metaData->DoStop(), Status::OK);
    metaData->OnBufferAvailable();
    EXPECT_EQ(metaData->DoRelease(), Status::OK);
    EXPECT_EQ(metaData->NotifyEos(), Status::OK);
    metaData->SetParameter(format);
    metaData->GetParameter(format);
    EXPECT_EQ(metaData->UpdateNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    EXPECT_EQ(metaData->UnLinkNext(nullptr, Pipeline::StreamType::STREAMTYPE_PACKED), Status::OK);
    metaData->GetFilterType();

    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->pts_ = 0;
    metaData->refreshTotalPauseTime_ = true;
    metaData->latestPausedTime_ = 70;

    metaData->UpdateBufferConfig(buffer, 50);
    metaData->UpdateBufferConfig(buffer, 100);

    metaData->OnUpdatedResult(format);
    metaData->OnUnlinkedResult(format);

    EXPECT_EQ(metaData->OnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, format, nullptr), Status::OK);
    EXPECT_EQ(metaData->OnUpdated(Pipeline::StreamType::STREAMTYPE_PACKED, format, nullptr), Status::OK);
    EXPECT_EQ(metaData->OnUnLinked(Pipeline::StreamType::STREAMTYPE_PACKED, nullptr), Status::OK);
}

}