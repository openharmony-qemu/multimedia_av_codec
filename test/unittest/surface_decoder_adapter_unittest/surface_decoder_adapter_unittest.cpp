/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
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

#include "surface_decoder_adapter_unittest.h"

namespace OHOS {
namespace Media {
using namespace std;
using namespace testing;
using namespace testing::ext;

static const int32_t NUM_0 = 0;
static const int32_t NUM_1 = 1;

void SurfaceDecoderAdapterUnitTest::SetUpTestCase(void) {}

void SurfaceDecoderAdapterUnitTest::TearDownTestCase(void) {}

void SurfaceDecoderAdapterUnitTest::SetUp(void)
{
    adapter_ = std::make_shared<SurfaceDecoderAdapter>();
}

void SurfaceDecoderAdapterUnitTest::TearDown(void)
{
    adapter_ = nullptr;
}

/**
 * @tc.name  : Test SurfaceDecoderAdapterCallback OnError
 * @tc.number: OnError_001
 * @tc.desc  : Test surfaceDecoderAdapter_.lock() == nullptr
 *             Test surfaceDecoderAdapter_.lock() != nullptr
 *             Test transCoderErrorCbOnce_ == false
 */
HWTEST_F(SurfaceDecoderAdapterUnitTest, OnError_001, TestSize.Level0)
{
    // Test surfaceDecoderAdapter_.lock() == nullptr
    adapter_ = nullptr;
    auto callback = std::make_shared<SurfaceDecoderAdapterCallback>(adapter_);
    callback->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, NUM_1);

    // Test surfaceDecoderAdapter_.lock() != nullptr
    // Test transCoderErrorCbOnce_ == false
    adapter_ = std::make_shared<SurfaceDecoderAdapter>();
    ASSERT_NE(adapter_, nullptr);
    auto decoderAdapterCallback = std::make_shared<MockDecoderAdapterCallback>();
    adapter_->decoderAdapterCallback_ = decoderAdapterCallback;
    callback = std::make_shared<SurfaceDecoderAdapterCallback>(adapter_);
    EXPECT_CALL(*decoderAdapterCallback, OnError(_, _)).WillOnce(Return());
    callback->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_INTERNAL, NUM_1);
    EXPECT_TRUE(callback->transCoderErrorCbOnce_);
}

/**
 * @tc.name  : Test SurfaceDecoderAdapterCallback OnOutputBufferAvailable
 * @tc.number: SurfaceDecoderAdapterCallback_OnOutputBufferAvailable_001
 * @tc.desc  : Test surfaceDecoderAdapter_.lock() == nullptr
 *             Test surfaceDecoderAdapter_.lock() != nullptr
 *             Test (buffer->flag_ & BUFFER_IS_EOS) == 1
 */
HWTEST_F(SurfaceDecoderAdapterUnitTest, SurfaceDecoderAdapterCallback_OnOutputBufferAvailable_001, TestSize.Level0)
{
    // Test surfaceDecoderAdapter_.lock() == nullptr
    adapter_ = nullptr;
    auto buffer = std::make_shared<AVBuffer>();
    auto callback = std::make_shared<SurfaceDecoderAdapterCallback>(adapter_);
    callback->OnOutputBufferAvailable(NUM_1, buffer);

    // Test surfaceDecoderAdapter_.lock() != nullptr
    // Test (buffer->flag_ & BUFFER_IS_EOS) == 1
    adapter_ = std::make_shared<SurfaceDecoderAdapter>();
    ASSERT_NE(adapter_, nullptr);
    auto decoderAdapterCallback = std::make_shared<MockDecoderAdapterCallback>();
    adapter_->decoderAdapterCallback_ = decoderAdapterCallback;
    callback = std::make_shared<SurfaceDecoderAdapterCallback>(adapter_);
    buffer->flag_ = 1;
    callback->OnOutputBufferAvailable(NUM_1, buffer);
    surfaceDecoderAdapter = callback->surfaceDecoderAdapter_.lock();
    ASSERT_NE(surfaceDecoderAdapter, nullptr);
    uint32_t renderIndex = surfaceDecoderAdapter->indexs_[0];
    EXPECT_EQ(renderIndex, NUM_1);
}

/**
 * @tc.name  : Test AVBufferAvailableListener OnBufferAvailable
 * @tc.number: OnBufferAvailable_001
 * @tc.desc  : Test surfaceDecoderAdapter_.lock() == nullptr
 */
HWTEST_F(SurfaceDecoderAdapterUnitTest, OnBufferAvailable_001, TestSize.Level0)
{
    adapter_ = nullptr;
    auto listener = std::make_shared<AVBufferAvailableListener>(adapter_);
    listener->OnBufferAvailable();
    EXPECT_EQ(listener->surfaceDecoderAdapter_.lock(), nullptr);
}

/**
 * @tc.name  : Test GetInputBufferQueue
 * @tc.number: GetInputBufferQueue_001
 * @tc.desc  : Test inputBufferQueue_ != nullptr && inputBufferQueue_-> GetQueueSize() > 0
 */
HWTEST_F(SurfaceDecoderAdapterUnitTest, GetInputBufferQueue_001, TestSize.Level0)
{
    ASSERT_NE(adapter_, nullptr);
    auto inputBufferQueue = std::make_shared<MockAVBufferQueue>();
    adapter_->inputBufferQueue_ = inputBufferQueue;
    EXPECT_CALL(*inputBufferQueue, GetQueueSize()).WillOnce(Return(NUM_1));
    sptr<AVBufferQueueProducer> avBufferQueueProducer = new MockAVBufferQueueProducer();
    adapter_->inputBufferQueueProducer_ = avBufferQueueProducer;
    auto ret = adapter_->GetInputBufferQueue();
    EXPECT_EQ(ret, avBufferQueueProducer);
}

/**
 * @tc.name  : Test SetDecoderAdapterCallback
 * @tc.number: SetDecoderAdapterCallback_001
 * @tc.desc  : Test ret != 0
 */
HWTEST_F(SurfaceDecoderAdapterUnitTest, SetDecoderAdapterCallback_001, TestSize.Level0)
{
    ASSERT_NE(adapter_, nullptr);
    auto decoderAdapterCallback = std::make_shared<MockDecoderAdapterCallback>();
    auto codecServer = std::make_shared<MockAVCodecVideoDecoder>();
    adapter_->codecServer_ = codecServer;
    EXPECT_CALL(*codecServer, SetCallback(::testing::A<const std::shared_ptr<MediaCodecCallback> &>()))
        .WillOnce(Return(NUM_1));
    EXPECT_CALL(*codecServer, Release()).WillOnce(Return(NUM_1));
    auto ret = adapter_->SetDecoderAdapterCallback(decoderAdapterCallback);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name  : Test OnOutputBufferAvailable
 * @tc.number: OnOutputBufferAvailable_001
 * @tc.desc  : Test (buffer->flag_ & BUFFER_IS_EOS) == 1 && buffer->pts_ <= lastBufferPts_.load()
 */
HWTEST_F(SurfaceDecoderAdapterUnitTest, OnOutputBufferAvailable_001, TestSize.Level0)
{
    ASSERT_NE(adapter_, nullptr);
    auto buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = NUM_0;
    buffer->pts_ = NUM_1;
    adapter_->lastBufferPts_ = NUM_1;
    adapter_->OnOutputBufferAvailable(NUM_1, buffer);
    EXPECT_NE(adapter_->dropIndexs_.size(), 0);
}
} // namespace Media
} // namespace OHOS