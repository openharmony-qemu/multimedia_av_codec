/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "metadata_filter_unittest.h"

namespace OHOS::Media {
const static int32_t NUM_TEST = 0;
const static int32_t TIME_TEST = 1;
using namespace std;
using namespace testing;
using namespace testing::ext;
using namespace OHOS::Media::Pipeline;
void MetaDataFilterUnittest::SetUpTestCase(void)
{
}

void MetaDataFilterUnittest::TearDownTestCase(void)
{
}

void MetaDataFilterUnittest::SetUp()
{
    metaData_ = std::make_shared<Pipeline::MetaDataFilter>("testMetaDataFilter", Pipeline::FilterType::FILTERTYPE_AENC);
}

void MetaDataFilterUnittest::TearDown()
{
    metaData_ = nullptr;
}

/**
 * @tc.name: Test OnLinkedResult  API
 * @tc.number: CallBackOnLinkedResult_001
 * @tc.desc: Test metaDataFilter_ == nullptr
 */
HWTEST_F(MetaDataFilterUnittest, CallBackOnLinkedResult_001, TestSize.Level0)
{
    ASSERT_NE(metaData_, nullptr);
    auto callback = std::make_shared<MetaDataFilterLinkCallback>(nullptr);
    sptr<AVBufferQueueProducer> queue;
    std::shared_ptr<Meta> meta;
    callback->OnLinkedResult(queue, meta);
    auto test = callback->metaDataFilter_.lock();
    EXPECT_EQ(test, nullptr);
}

/**
 * @tc.name: Test OnUnLinkedResult  API
 * @tc.number: CallBackOnUnLinkedResult_001
 * @tc.desc: Test metaDataFilter_ == nullptr
 *           Test metaDataFilter_ != nullptr
 */
HWTEST_F(MetaDataFilterUnittest, CallBackOnUnLinkedResult_001, TestSize.Level0)
{
    ASSERT_NE(metaData_, nullptr);
    auto callback = std::make_shared<MetaDataFilterLinkCallback>(nullptr);
    std::shared_ptr<Meta> meta;
    callback->OnUnlinkedResult(meta);
    auto test = callback->metaDataFilter_.lock();
    EXPECT_EQ(test, nullptr);

    callback->metaDataFilter_ = metaData_;
    callback->OnUnlinkedResult(meta);
    test = callback->metaDataFilter_.lock();
    EXPECT_NE(test, nullptr);
}

/**
 * @tc.name: Test OnUpdatedResult  API
 * @tc.number: CallBackOnUpdatedResult_001
 * @tc.desc: Test metaDataFilter_ == nullptr
 *           Test metaDataFilter_ != nullptr
 */
HWTEST_F(MetaDataFilterUnittest, CallBackOnUpdatedResult_001, TestSize.Level0)
{
    ASSERT_NE(metaData_, nullptr);
    auto callback = std::make_shared<MetaDataFilterLinkCallback>(nullptr);
    std::shared_ptr<Meta> meta;
    callback->OnUpdatedResult(meta);
    auto test = callback->metaDataFilter_.lock();
    EXPECT_EQ(test, nullptr);

    callback->metaDataFilter_ = metaData_;
    callback->OnUpdatedResult(meta);
    test = callback->metaDataFilter_.lock();
    EXPECT_NE(test, nullptr);
}

/**
 * @tc.name: Test OnBufferAvailable  API
 * @tc.number: BufferListenerOnBufferAvailable_001
 * @tc.desc: Test metaDataFilter_ == nullptr
 *           Test metaDataFilter_ != nullptr
 */
HWTEST_F(MetaDataFilterUnittest, BufferListenerOnBufferAvailable_001, TestSize.Level0)
{
    ASSERT_NE(metaData_, nullptr);
    auto mockSurface = new MockSurface();
    EXPECT_CALL(*(mockSurface), AcquireBuffer(_, _, _, _)).WillRepeatedly(Return(GSERROR_OK));
    metaData_->inputSurface_ = mockSurface;
    auto callback = std::make_shared<MetaDataSurfaceBufferListener>(nullptr);
    callback->OnBufferAvailable();
    auto test = callback->metaDataFilter_.lock();
    EXPECT_EQ(test, nullptr);

    callback->metaDataFilter_ = metaData_;
    callback->OnBufferAvailable();
    test = callback->metaDataFilter_.lock();
    EXPECT_NE(test, nullptr);
    OHOS::Rect damage;
    sptr<SurfaceBuffer> buffer;
    sptr<SyncFence> fence;
    int64_t numTest = NUM_TEST;
    EXPECT_EQ(metaData_->inputSurface_->AcquireBuffer(buffer, fence, numTest, damage), GSERROR_OK);
}

/**
 * @tc.name: Test UpdateBufferConfig  API
 * @tc.number: UpdateBufferConfig_001
 * @tc.desc: Test refreshTotalPauseTime_ == true
 *           Test latestPausedTime_ != TIME_NONE && latestBufferTime_ > latestPausedTime_
 */
HWTEST_F(MetaDataFilterUnittest, UpdateBufferConfig_001, TestSize.Level0)
{
    ASSERT_NE(metaData_, nullptr);
    metaData_->refreshTotalPauseTime_ = true;
    metaData_->latestPausedTime_ = NUM_TEST;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    int64_t timestamp = TIME_TEST;
    metaData_->UpdateBufferConfig(buffer, timestamp);
    EXPECT_EQ(metaData_->totalPausedTime_, TIME_TEST);
    EXPECT_EQ(metaData_->refreshTotalPauseTime_, false);
}
} // namespace OHOS::Media
