/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file expect in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <chrono>
#include <memory>
#include <thread>
#include "avcodec_errors.h"
#include "codec_server_coverage_unit_test.h"
#include "codeclist_core.h"
#include "media_description.h"
#include "meta/meta_key.h"
#include "unittest_utils.h"

#define EXPECT_CALL_GET_HCODEC_CAPS_MOCK                                                                               \
    EXPECT_CALL(*codecBaseMock_, GetHCapabilityList).Times(AtLeast(1)).WillRepeatedly
#define EXPECT_CALL_GET_FCODEC_CAPS_MOCK                                                                               \
    EXPECT_CALL(*codecBaseMock_, GetFCapabilityList).Times(AtLeast(1)).WillRepeatedly
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace MediaAVCodec {
void CodecServerUnitTest::CreateCodecByMime()
{
    std::string codecName = "video.H.Encoder.Name.00";
    std::string codecMime = CODEC_MIME_MOCK_00;

    EXPECT_CALL(*codecBaseMock_, Init).Times(1).WillOnce(Return(AVCS_ERR_OK));
    EXPECT_CALL_GET_HCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, HCODEC_CAPS)));
    EXPECT_CALL_GET_FCODEC_CAPS_MOCK(Return(RetAndCaps(AVCS_ERR_OK, {})));
    EXPECT_CALL(*codecBaseMock_, CodecBaseCtor()).Times(1);
    EXPECT_CALL(*codecBaseMock_, CreateHCodecByName(codecName))
        .Times(1)
        .WillOnce(Return(std::make_shared<CodecBase>()));
    EXPECT_CALL(*codecBaseMock_, SetCallback(std::shared_ptr<MediaCodecCallback>(nullptr)))
        .Times(1)
        .WillOnce(Return(AVCS_ERR_OK));

    int32_t ret = server_->Init(AVCODEC_TYPE_VIDEO_ENCODER, true, codecMime, *validFormat_.GetMeta());
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: PreparePostProcessing_Invalid_Test_001
 * @tc.desc: postProcessing controller is nullptr
 */
HWTEST_F(CodecServerUnitTest, PreparePostProcessing_Invalid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    server_->postProcessing_ = std::make_unique<CodecServer::PostProcessingType>(server_->codecBase_);
    int32_t ret = server_->PreparePostProcessing();
    EXPECT_NE(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: PreparePostProcessing_Valid_Test_002
 * @tc.desc: codec PreparePostProcessing
 */
HWTEST_F(CodecServerUnitTest, PreparePostProcessing_Valid_Test_002, TestSize.Level1)
{
    CreateCodecByMime();
    server_->postProcessing_ = nullptr;
    int32_t ret = server_->PreparePostProcessing();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: PostProcessingOnError_Valid_Test_001
 * @tc.desc: videoCb_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, PostProcessingOnError_Valid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    int32_t errorCode = AVCS_ERR_OK;
    server_->PostProcessingOnError(errorCode);
}

/**
 * @tc.name: PostProcessingOnOutputBufferAvailable_Valid_Test_001
 * @tc.desc: videoCb_ is not nullptr
 */
HWTEST_F(CodecServerUnitTest, PostProcessingOnOutputBufferAvailable_Valid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    auto mock = std::make_shared<MediaCodecCallbackMock>();
    server_->videoCb_ = mock;
    int32_t index = 1;
    int32_t flag = 1;
    server_->PostProcessingOnOutputBufferAvailable(index, flag);
}

/**
 * @tc.name: PostProcessingOnOutputBufferAvailable_Valid_Test_002
 * @tc.desc: videoCb_ is nullptr
 */
HWTEST_F(CodecServerUnitTest, PostProcessingOnOutputBufferAvailable_Valid_Test_002, TestSize.Level1)
{
    CreateCodecByMime();
    server_->videoCb_ = nullptr;
    int32_t index = 1;
    int32_t flag = 1;
    server_->PostProcessingOnOutputBufferAvailable(index, flag);
}

/**
 * @tc.name: StartPostProcessing_Invalid_Test_001
 * @tc.desc: postProcessing controller is nullptr
 */
HWTEST_F(CodecServerUnitTest, StartPostProcessing_Invalid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    server_->postProcessing_ = std::make_unique<CodecServer::PostProcessingType>(server_->codecBase_);
    int32_t ret = server_->StartPostProcessing();
    EXPECT_NE(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: StopPostProcessing_inValid_Test_001
 * @tc.desc: postProcessing controller is nullptr
 */
HWTEST_F(CodecServerUnitTest, StopPostProcessing_inValid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    server_->postProcessingTask_ = nullptr;
    server_->postProcessing_ = std::make_unique<CodecServer::PostProcessingType>(server_->codecBase_);
    int32_t ret = server_->StopPostProcessing();
    EXPECT_NE(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: StopPostProcessing_Valid_Test_002
 * @tc.desc: codec StopPostProcessing
 */
HWTEST_F(CodecServerUnitTest, StopPostProcessing_Valid_Test_002, TestSize.Level1)
{
    CreateCodecByMime();
    auto bufferInfoQueue = std::make_shared<CodecServer::DecodedBufferInfoQueue>(DEFAULT_LOCK_FREE_QUEUE_NAME);
    auto postprocessingBufferInfoQueue =
        std::make_shared<CodecServer::PostProcessingBufferInfoQueue>(POST_PROCESSING_LOCK_FREE_QUEUE_NAME);
    server_->postProcessingTask_ = std::make_unique<TaskThread>(DEFAULT_TASK_NAME);
    server_->postProcessing_ = nullptr;
    server_->decodedBufferInfoQueue_ = bufferInfoQueue;
    server_->postProcessingInputBufferInfoQueue_ = postprocessingBufferInfoQueue;
    int32_t ret = server_->StopPostProcessing();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: FlushPostProcessing_Invalid_Test_001
 * @tc.desc: postProcessing flush failed
 */
HWTEST_F(CodecServerUnitTest, FlushPostProcessing_Invalid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    auto bufferInfoQueue = std::make_shared<CodecServer::DecodedBufferInfoQueue>(DEFAULT_LOCK_FREE_QUEUE_NAME);
    auto postprocessingBufferInfoQueue =
        std::make_shared<CodecServer::PostProcessingBufferInfoQueue>(POST_PROCESSING_LOCK_FREE_QUEUE_NAME);
    server_->postProcessing_ = std::make_unique<CodecServer::PostProcessingType>(server_->codecBase_);
    server_->postProcessingTask_ = std::make_unique<TaskThread>(DEFAULT_TASK_NAME);
    server_->decodedBufferInfoQueue_ = bufferInfoQueue;
    server_->postProcessingInputBufferInfoQueue_ = postprocessingBufferInfoQueue;
    int32_t ret = server_->FlushPostProcessing();
    EXPECT_NE(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: FlushPostProcessing_Invalid_Test_002
 * @tc.desc: postProcessing flush failed
 */
HWTEST_F(CodecServerUnitTest, FlushPostProcessing_Invalid_Test_002, TestSize.Level1)
{
    CreateCodecByMime();
    auto bufferInfoQueue = std::make_shared<CodecServer::DecodedBufferInfoQueue>(DEFAULT_LOCK_FREE_QUEUE_NAME);
    server_->postProcessing_ = std::make_unique<CodecServer::PostProcessingType>(server_->codecBase_);
    server_->postProcessingTask_ = nullptr;
    server_->decodedBufferInfoQueue_ = nullptr;
    server_->postProcessingInputBufferInfoQueue_ = nullptr;
    int32_t ret = server_->FlushPostProcessing();
    EXPECT_NE(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: FlushPostProcessing_Valid_Test_002
 * @tc.desc: codec FlushPostProcessing
 */
HWTEST_F(CodecServerUnitTest, FlushPostProcessing_Valid_Test_002, TestSize.Level1)
{
    CreateCodecByMime();
    server_->postProcessing_ = nullptr;
    int32_t ret = server_->FlushPostProcessing();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: ResetPostProcessing_Valid_Test_001
 * @tc.desc: codec ResetPostProcessing
 */
HWTEST_F(CodecServerUnitTest, ResetPostProcessing_Valid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    server_->postProcessing_ = std::make_unique<CodecServer::PostProcessingType>(server_->codecBase_);
    int32_t ret = server_->ResetPostProcessing();
    EXPECT_EQ(ret, AVCS_ERR_OK);
}

/**
 * @tc.name: StartPostProcessingTask_Valid_Test_001
 * @tc.desc: codec StartPostProcessingTask
 */
HWTEST_F(CodecServerUnitTest, StartPostProcessingTask_Valid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    auto bufferInfoQueue = std::make_shared<CodecServer::DecodedBufferInfoQueue>(DEFAULT_LOCK_FREE_QUEUE_NAME);
    auto postprocessingBufferInfoQueue =
        std::make_shared<CodecServer::PostProcessingBufferInfoQueue>(POST_PROCESSING_LOCK_FREE_QUEUE_NAME);
    server_->postProcessingTask_ = nullptr;
    server_->decodedBufferInfoQueue_ = bufferInfoQueue;
    server_->postProcessingInputBufferInfoQueue_ = postprocessingBufferInfoQueue;
    server_->StartPostProcessingTask();
    EXPECT_NE(server_->postProcessingTask_, nullptr);
}

/**
 * @tc.name: StartPostProcessingTask_Valid_Test_002
 * @tc.desc: codec StartPostProcessingTask
 */
HWTEST_F(CodecServerUnitTest, StartPostProcessingTask_Valid_Test_002, TestSize.Level1)
{
    CreateCodecByMime();
    auto bufferInfoQueue = std::make_shared<CodecServer::DecodedBufferInfoQueue>(DEFAULT_LOCK_FREE_QUEUE_NAME);
    server_->postProcessingTask_ = std::make_unique<TaskThread>(DEFAULT_TASK_NAME);
    server_->decodedBufferInfoQueue_ = nullptr;
    server_->postProcessingInputBufferInfoQueue_ = nullptr;
    server_->StartPostProcessingTask();
}

/**
 * @tc.name: DeactivatePostProcessingQueue_Valid_Test_001
 * @tc.desc: codec DeactivatePostProcessingQueue
 */
HWTEST_F(CodecServerUnitTest, DeactivatePostProcessingQueue_Valid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    auto bufferInfoQueue = std::make_shared<CodecServer::DecodedBufferInfoQueue>(DEFAULT_LOCK_FREE_QUEUE_NAME);
    auto postprocessingBufferInfoQueue =
        std::make_shared<CodecServer::PostProcessingBufferInfoQueue>(POST_PROCESSING_LOCK_FREE_QUEUE_NAME);
    server_->decodedBufferInfoQueue_ = bufferInfoQueue;
    server_->postProcessingInputBufferInfoQueue_ = postprocessingBufferInfoQueue;
    server_->DeactivatePostProcessingQueue();
    EXPECT_EQ(server_->decodedBufferInfoQueue_->active_, false);
    EXPECT_EQ(server_->postProcessingInputBufferInfoQueue_->active_, false);
}

/**
 * @tc.name: CleanPostProcessingResource_Valid_Test_001
 * @tc.desc: codec CleanPostProcessingResource
 */
HWTEST_F(CodecServerUnitTest, CleanPostProcessingResource_Valid_Test_001, TestSize.Level1)
{
    CreateCodecByMime();
    auto bufferInfoQueue = std::make_shared<CodecServer::DecodedBufferInfoQueue>(DEFAULT_LOCK_FREE_QUEUE_NAME);
    auto postprocessingBufferInfoQueue =
        std::make_shared<CodecServer::PostProcessingBufferInfoQueue>(POST_PROCESSING_LOCK_FREE_QUEUE_NAME);
    server_->postProcessingTask_ = std::make_unique<TaskThread>(DEFAULT_TASK_NAME);
    server_->decodedBufferInfoQueue_ = bufferInfoQueue;
    server_->postProcessingInputBufferInfoQueue_ = postprocessingBufferInfoQueue;
    server_->CleanPostProcessingResource();
    EXPECT_EQ(server_->postProcessingTask_, nullptr);
    EXPECT_EQ(server_->decodedBufferInfoQueue_, nullptr);
    EXPECT_EQ(server_->postProcessingInputBufferInfoQueue_, nullptr);
}
} // namespace MediaAVCodec
} // namespace OHOS

int main(int argc, char **argv)
{
    testing::GTEST_FLAG(output) = "xml:./";
    testing::InitGoogleTest(&argc, argv);
    auto res = RUN_ALL_TESTS();
    std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // 1000ms
    return res;
}