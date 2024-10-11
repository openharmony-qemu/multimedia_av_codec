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

#include "audio_data_source_filter_unit_test.h"
#include "audio_data_source_filter.h"
#include "common/log.h"
#include "filter/filter_factory.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void AudioDataSourceFilterUnitTest::SetUpTestCase(void) {}

void AudioDataSourceFilterUnitTest::TearDownTestCase(void) {}

void AudioDataSourceFilterUnitTest::SetUp(void)
{
    audioDataSourceFilter_ =
        std::make_shared<AudioDataSourceFilter>("testAudioDataSourceFilter", FilterType::AUDIO_DATA_SOURCE);
}

void AudioDataSourceFilterUnitTest::TearDown(void)
{
    audioDataSourceFilter_ = nullptr;
}

/**
 * @tc.name: AudioDataSourceFilter_Init_001
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_Init_001, TestSize.Level1)
{
    auto testEventReceiver = std::make_shared<TestEventReceiver>();
    auto testFilterCallback = std::make_shared<TestFilterCallback>();
    audioDataSourceFilter_->Init(testEventReceiver, testFilterCallback);
    std::shared_ptr<Task> taskPtr_ = std::make_shared<Task>("test");
    audioDataSourceFilter_->Init(testEventReceiver, testFilterCallback);
    ASSERT_NE(audioDataSourceFilter_->taskPtr_, nullptr);
}

/**
 * @tc.name: AudioDataSourceFilter_DoPrepare_001
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_DoPrepare_001, TestSize.Level1)
{
    Status ret = audioDataSourceFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::ERROR_NULL_POINTER);
    audioDataSourceFilter_->callback_ = std::make_shared<TestFilterCallback>();
    ret = audioDataSourceFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: AudioDataSourceFilter_DoStart_001
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_DoStart_001, TestSize.Level1)
{
    audioDataSourceFilter_->nextFilter_ = std::make_shared<TestFilter>();
    Status ret = audioDataSourceFilter_->DoStart();
    EXPECT_EQ(ret, Status::OK);
    std::shared_ptr<Task> taskPtr_ = std::make_shared<Task>("test");
    ret = audioDataSourceFilter_->DoStart();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: AudioDataSourceFilter_DoPause_001
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_DoPause_001, TestSize.Level1)
{
    Status ret = audioDataSourceFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
    std::shared_ptr<Task> taskPtr_ = std::make_shared<Task>("test");
    ret = audioDataSourceFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: AudioDataSourceFilter_DoResume_001
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_DoResume_001, TestSize.Level1)
{
    Status ret = audioDataSourceFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
    std::shared_ptr<Task> taskPtr_ = std::make_shared<Task>("test");
    ret = audioDataSourceFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: AudioDataSourceFilter_DoStop_001
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_DoStop_001, TestSize.Level1)
{
    std::shared_ptr<Task> taskPtr_ = std::make_shared<Task>("test");
    Status ret = audioDataSourceFilter_->DoStop();
    taskPtr_ = nullptr;
    audioDataSourceFilter_->nextFilter_ = std::make_shared<TestFilter>();
    ret = audioDataSourceFilter_->DoStop();
    audioDataSourceFilter_->nextFilter_ = nullptr;
    ret = audioDataSourceFilter_->DoStop();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: AudioDataSourceFilter_DoRelease_001
 * @tc.desc: DoRelease
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_DoRelease_001, TestSize.Level1)
{
    std::shared_ptr<Task> taskPtr_ = std::make_shared<Task>("test");
    Status ret = audioDataSourceFilter_->DoRelease();
    taskPtr_ = nullptr;
    ret = audioDataSourceFilter_->DoRelease();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: AudioDataSourceFilter_SendEos_001
 * @tc.desc: SendEos
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_SendEos_001, TestSize.Level1)
{
    audioDataSourceFilter_->outputBufferQueue_ = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    Status ret = audioDataSourceFilter_->SendEos();
    audioDataSourceFilter_->outputBufferQueue_ = nullptr;
    ret = audioDataSourceFilter_->SendEos();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: AudioDataSourceFilter_ReadLoop_001
 * @tc.desc: ReadLoop
 * @tc.type: FUNC
 */
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_ReadLoop_001, TestSize.Level1)
{
    audioDataSourceFilter_->audioDataSource_ = std::make_shared<MyIAudioDataSource>();
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    audioDataSourceFilter_->outputBufferQueue_ = new OHOS::Media::Pipeline::MyAVBufferQueueProducer();
    audioDataSourceFilter_->eos_ = true;
    audioDataSourceFilter_->ReadLoop();
    audioDataSourceFilter_->eos_ = false;
    audioDataSourceFilter_->ReadLoop();
    EXPECT_EQ(audioDataSourceFilter_->nextFilter_, nullptr);
}

HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_002, TestSize.Level1)
{
    // 1. Set up the test environment
    std::shared_ptr<TestFilter> nextFilter = std::make_shared<TestFilter>();
    StreamType outType = StreamType::STREAMTYPE_RAW_AUDIO;

    // 2. Call the function to be tested
    Status status = audioDataSourceFilter_->UpdateNext(nextFilter, outType);

    // 3. Verify the result
    EXPECT_EQ(status, Status::OK);
}

/**
* @tc.name    : Test AudioDataSourceFilter API
* @tc.number  : AudioDataSourceFilter_003
* @tc.desc    : Test UnLinkNext interface, return Status::OK.
* @tc.require : issueI5NZAQ
*/
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_003, TestSize.Level1)
{
    // 1. Set up the test environment
    std::shared_ptr<TestFilter> nextFilter = std::make_shared<TestFilter>();
    StreamType outType = StreamType::STREAMTYPE_RAW_AUDIO;

    // 2. Call the function to be tested
    Status status = audioDataSourceFilter_->UnLinkNext(nextFilter, outType);

    // 3. Verify the result
    EXPECT_EQ(status, Status::OK);
}

/**
* @tc.name    : Test AudioDataSourceFilter API
* @tc.number  : AudioDataSourceFilter_004
* @tc.desc    : Test OnLinked interface, return Status::OK.
* @tc.require : issueI5NZAQ
*/
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_004, TestSize.Level1)
{
    // 1. Set up the test environment
    StreamType inType = StreamType::STREAMTYPE_RAW_AUDIO;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    std::shared_ptr<TestFilterLinkCallback> callback = std::make_shared<TestFilterLinkCallback>();

    // 2. Call the function to be tested
    Status status = audioDataSourceFilter_->OnLinked(inType, meta, callback);

    // 3. Verify the result
    EXPECT_EQ(status, Status::OK);
}

/**
* @tc.name    : Test AudioDataSourceFilter API
* @tc.number  : AudioDataSourceFilter_005
* @tc.desc    : Test OnUpdated interface, return Status::OK.
* @tc.require : issueI5NZAQ
*/
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_005, TestSize.Level1)
{
    // 1. Set up the test environment
    StreamType inType = StreamType::STREAMTYPE_RAW_AUDIO;
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    std::shared_ptr<TestFilterLinkCallback> callback = std::make_shared<TestFilterLinkCallback>();

    // 2. Call the function to be tested
    Status status = audioDataSourceFilter_->OnUpdated(inType, meta, callback);

    // 3. Verify the result
    EXPECT_EQ(status, Status::OK);
}

/**
* @tc.name    : Test AudioDataSourceFilter API
* @tc.number  : AudioDataSourceFilter_006
* @tc.desc    : Test OnUnLinked interface, return Status::OK.
* @tc.require : issueI5NZAQ
*/
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_006, TestSize.Level1)
{
    // 1. Set up the test environment
    StreamType inType = StreamType::STREAMTYPE_RAW_AUDIO;
    std::shared_ptr<TestFilterLinkCallback> callback = std::make_shared<TestFilterLinkCallback>();

    // 2. Call the function to be tested
    Status status = audioDataSourceFilter_->OnUnLinked(inType, callback);

    // 3. Verify the result
    EXPECT_EQ(status, Status::OK);
}
/**
* @tc.name    : Test AudioDataSourceFilter API
* @tc.number  : AudioDataSourceFilter_001
* @tc.desc    : Test LinkNext interface, return Status::OK.
* @tc.require : issueI5NZAQ
*/
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_007, TestSize.Level1)
{
    // 1. Set up the test environment
    std::shared_ptr<TestFilter> nextFilter = std::make_shared<TestFilter>();
    StreamType outType = StreamType::STREAMTYPE_RAW_AUDIO;

    // 2. Call the function to be tested
    Status status = audioDataSourceFilter_->LinkNext(nextFilter, outType);

    // 3. Verify the result
    EXPECT_EQ(status, Status::OK);
}

/**
* @tc.name    : Test AudioDataSourceFilter API
* @tc.number  : AudioDataSourceFilter_002
* @tc.desc    : Test GetFilterType interface, return FilterType::AUDIO_CAPTURE.
* @tc.require : issueI5NZAQ
*/
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_008, TestSize.Level1)
{
    FilterType filterType = audioDataSourceFilter_->GetFilterType();

    EXPECT_EQ(filterType, FilterType::AUDIO_CAPTURE);
}

/**
* @tc.name    : Test AudioDataSourceFilter API
* @tc.number  : AudioDataSourceFilter_003
* @tc.desc    : Test SetAudioDataSource interface, set audioDataSource_.
* @tc.require : issueI5NZAQ
*/
HWTEST_F(AudioDataSourceFilterUnitTest, AudioDataSourceFilter_009, TestSize.Level1)
{
    // 1. Set up the test environment
    std::shared_ptr<MyIAudioDataSource> audioSource = std::make_shared<MyIAudioDataSource>();

    // 2. Call the function to be tested
    audioDataSourceFilter_->SetAudioDataSource(audioSource);

    // 3. Verify the result
    EXPECT_EQ(audioDataSourceFilter_->audioDataSource_, audioSource);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS