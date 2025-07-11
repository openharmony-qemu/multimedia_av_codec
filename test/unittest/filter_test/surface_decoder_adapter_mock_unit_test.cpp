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
#include "surface_decoder_adapter_mock_unit_test.h"
#include "avcodec_errors.h"
#include "avbuffer_queue.h"
#include "media_description.h"


using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;

namespace OHOS {
namespace Media {
void SurfaceDecoderMockUnitTest::SetUpTestCase(void) {}

void SurfaceDecoderMockUnitTest::TearDownTestCase(void) {}

void SurfaceDecoderMockUnitTest::SetUp(void)
{
    surfaceDecoderAdapter_ = std::make_shared<SurfaceDecoderAdapter>();
    mockCodecServer_ = std::make_shared<MockAVCodecVideoDecoder>();
    mockDecoderAdapterCallback_ = std::make_shared<MockDecoderAdapterCallback>();
    mockInputBufferQueueConsumer_ = new MockAVBufferQueueConsumer();
}

void SurfaceDecoderMockUnitTest::TearDown(void)
{
    if (mockInputBufferQueueConsumer_) {
        delete mockInputBufferQueueConsumer_;
        mockInputBufferQueueConsumer_ = nullptr;
    }
    surfaceDecoderAdapter_ = nullptr;
    mockCodecServer_ = nullptr;
    mockDecoderAdapterCallback_ = nullptr;
}

/**
 * @tc.name: FailBranchsCover_0100
 * @tc.desc: Mock codecServer Fail Branchs Cover
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderMockUnitTest, FailBranchsCover_0100, TestSize.Level1)
{
    EXPECT_CALL(*(mockCodecServer_), SetOutputSurface(_)).WillRepeatedly(testing::Return(-1));
    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(testing::Return(0));
    surfaceDecoderAdapter_->codecServer_ = mockCodecServer_;
    sptr<Surface> surface = nullptr;
    Status ret = surfaceDecoderAdapter_->SetOutputSurface(surface);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    EXPECT_CALL(*(mockCodecServer_), Prepare()).WillRepeatedly(testing::Return(-1));
    ret = surfaceDecoderAdapter_->Start();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
    EXPECT_EQ(surfaceDecoderAdapter_->isThreadExit_, false);

    EXPECT_CALL(*(mockCodecServer_), Prepare()).WillRepeatedly(testing::Return(0));
    EXPECT_CALL(*(mockCodecServer_), Start()).WillRepeatedly(testing::Return(-1));
    ret = surfaceDecoderAdapter_->Start();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    surfaceDecoderAdapter_->releaseBufferTask_ = nullptr;
    EXPECT_CALL(*(mockCodecServer_), Stop()).WillRepeatedly(testing::Return(-1));
    ret = surfaceDecoderAdapter_->Stop();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    EXPECT_CALL(*(mockCodecServer_), Flush()).WillRepeatedly(testing::Return(-1));
    ret = surfaceDecoderAdapter_->Flush();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    EXPECT_CALL(*(mockCodecServer_), Release()).WillRepeatedly(testing::Return(-1));
    ret = surfaceDecoderAdapter_->Release();
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);

    Format format;
    EXPECT_CALL(*(mockCodecServer_), SetParameter(_)).WillRepeatedly(testing::Return(-1));
    ret = surfaceDecoderAdapter_->SetParameter(format);
    EXPECT_EQ(ret, Status::ERROR_UNKNOWN);
}

/**
 * @tc.name: FailBranchsCover_0200
 * @tc.desc: Mock InputBufferQueueConsumer Fail Branchs Cover
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderMockUnitTest, FailBranchsCover_0200, TestSize.Level1)
{
    int32_t metaIndex = 1;
    uint32_t index = static_cast<uint32_t>(metaIndex);
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    buffer->meta_ = std::make_shared<Meta>();
    EXPECT_CALL(*(mockInputBufferQueueConsumer_), IsBufferInQueue(_)).WillRepeatedly(testing::Return(true));
    EXPECT_CALL(*(mockInputBufferQueueConsumer_), ReleaseBuffer(_)).WillRepeatedly(testing::Return(Status::OK));
    EXPECT_CALL(*(mockInputBufferQueueConsumer_), GetQueueSize()).WillRepeatedly(testing::Return(0));
    EXPECT_CALL(*(mockInputBufferQueueConsumer_), SetQueueSize(_)).WillRepeatedly(testing::Return(Status::OK));
    EXPECT_CALL(*(mockInputBufferQueueConsumer_), AttachBuffer(_, _)).WillRepeatedly(testing::Return(Status::OK));
    surfaceDecoderAdapter_->inputBufferQueueConsumer_ = mockInputBufferQueueConsumer_;
    surfaceDecoderAdapter_->OnInputBufferAvailable(index, buffer);
    int32_t ret = 0;
    std::string metaKey = Tag::REGULAR_TRACK_ID;
    EXPECT_TRUE(buffer->meta_->GetData(metaKey, ret));
    EXPECT_EQ(ret, metaIndex);

    EXPECT_CALL(*(mockInputBufferQueueConsumer_), ReleaseBuffer(_)).WillRepeatedly(
        testing::Return(Status::ERROR_UNKNOWN));
    surfaceDecoderAdapter_->OnInputBufferAvailable(index, buffer);
    ret = 0;
    EXPECT_TRUE(buffer->meta_->GetData(metaKey, ret));
    EXPECT_EQ(ret, metaIndex);

    EXPECT_CALL(*(mockInputBufferQueueConsumer_), IsBufferInQueue(_)).WillRepeatedly(testing::Return(false));
    surfaceDecoderAdapter_->OnInputBufferAvailable(index, buffer);
    ret = 0;
    EXPECT_TRUE(buffer->meta_->GetData(metaKey, ret));
    EXPECT_EQ(ret, metaIndex);
}

/**
 * @tc.name: FailBranchsCover_0300
 * @tc.desc: Init Fail Branchs Cover
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderMockUnitTest, FailBranchsCover_0300, TestSize.Level1)
{
    surfaceDecoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("SurfaceDecoder");
    surfaceDecoderAdapter_->codecServer_ = mockCodecServer_;
    Status status = surfaceDecoderAdapter_->Init("video/avc");
    EXPECT_EQ(status, Status::OK);
}

/**
 * @tc.name: FailBranchsCover_0400
 * @tc.desc: OnOutputBufferAvailable Fail Branchs Cover
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderMockUnitTest, FailBranchsCover_0400, TestSize.Level1)
{
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    buffer->meta_ = std::make_shared<Meta>();
    buffer->flag_ = 1;
    buffer->pts_ = 0;
    surfaceDecoderAdapter_->lastBufferPts_.store(1);
    surfaceDecoderAdapter_->OnOutputBufferAvailable(index, buffer);
    uint32_t renderIndex = surfaceDecoderAdapter_->indexs_[0];
    EXPECT_EQ(renderIndex, index);
}

}  // namespace Media
}  // namespace OHOS