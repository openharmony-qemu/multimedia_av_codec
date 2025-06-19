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

#include "audio_data_source_filter_unittest.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int32_t NUM_0 = 0;
static const int32_t ERROR_RET = -1;

void AudioDataSourceFilterUnitTest::SetUpTestCase(void) {}

void AudioDataSourceFilterUnitTest::TearDownTestCase(void) {}

void AudioDataSourceFilterUnitTest::SetUp(void)
{
    filter_ = std::make_shared<AudioDataSourceFilter>("testAudioDataSourceFilter", FilterType::AUDIO_DATA_SOURCE);
}

void AudioDataSourceFilterUnitTest::TearDown(void)
{
    filter_ = nullptr;
}

/**
 * @tc.name  : Test OnLinkedResult
 * @tc.number: OnLinkedResult_001
 * @tc.desc  : Test OnUnlinkedResult dataSourceFilter != nullptr
 *             Test OnUnlinkedResult dataSourceFilter != nullptr
 *             Test OnLinkedResult dataSourceFilter == nullptr
 *             Test OnUpdatedResult dataSourceFilter == nullptr
 *             Test SetVideoFirstFramePts audioDataSource_ != nullptr
 */
HWTEST_F(AudioDataSourceFilterUnitTest, OnLinkedResult_001, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    sptr<AVBufferQueueProducer> outputBufferQueue = nullptr;
    std::shared_ptr<Meta> meta = nullptr;
    auto linkCallback = std::make_shared<AudioDataSourceFilterLinkCallback>(filter_);
    auto ptr = linkCallback->audioDataSourceFilter_.lock();
    ASSERT_NE(ptr, nullptr);
    // Test OnUnlinkedResult dataSourceFilter != nullptr
    linkCallback->OnUnlinkedResult(meta);
    // Test OnUpdatedResult dataSourceFilter != nullptr
    linkCallback->OnUpdatedResult(meta);
    
    // Test SetVideoFirstFramePts audioDataSource_ != nullptr
    auto mockAudioDataSource = std::make_shared<MockAudioDataSource>();
    filter_->audioDataSource_ = mockAudioDataSource;
    EXPECT_CALL(*mockAudioDataSource, SetVideoFirstFramePts(_)).Times(1);
    filter_->SetVideoFirstFramePts(NUM_0);

    filter_ = nullptr;
    linkCallback = std::make_shared<AudioDataSourceFilterLinkCallback>(filter_);
    // Test OnLinkedResult dataSourceFilter == nullptr
    linkCallback->OnLinkedResult(outputBufferQueue, meta);
    // Test OnUpdatedResult dataSourceFilter == nullptr
    linkCallback->OnUpdatedResult(meta);
}

/**
 * @tc.name  : Test DoPause
 * @tc.number: DoPause_001
 * @tc.desc  : Test taskptr_ != nullptr
 */
HWTEST_F(AudioDataSourceFilterUnitTest, DoPause_001, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    filter_->taskPtr_ = std::make_shared<Task>("testTask");
    EXPECT_CALL(*(filter_->taskPtr_), Pause()).Times(1);
    auto ret = filter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test DoResume
 * @tc.number: DoResume_001
 * @tc.desc  : Test taskptr_ != nullptr
 */
HWTEST_F(AudioDataSourceFilterUnitTest, DoResume_001, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    filter_->taskPtr_ = std::make_shared<Task>("testTask");
    EXPECT_CALL(*(filter_->taskPtr_), Start()).Times(1);
    auto ret = filter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name  : Test RelativeSleep
 * @tc.number: RelativeSleep_001
 * @tc.desc  : Test nanoTime <= 0
 */
HWTEST_F(AudioDataSourceFilterUnitTest, RelativeSleep_001, TestSize.Level0)
{
    ASSERT_NE(filter_, nullptr);
    auto ret = filter_->RelativeSleep(NUM_0);
    EXPECT_EQ(ret, ERROR_RET);
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS