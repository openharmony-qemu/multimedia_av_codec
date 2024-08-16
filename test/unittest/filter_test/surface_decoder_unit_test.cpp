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
#include <string>
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "surface_decoder_unit_test.h"
#include "video_decoder_adapter.h"
#include "common/log.h"
#include "parameters.h"
#include "avbuffer_queue.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Pipeline {
void SurfaceDecoderUnitTest::SetUpTestCase(void) {}

void SurfaceDecoderUnitTest::TearDownTestCase(void) {}

void SurfaceDecoderUnitTest::SetUp(void)
{
    surfaceDecoderAdapter_ = std::make_shared<SurfaceDecoderAdapter>();
}

void SurfaceDecoderUnitTest::TearDown(void)
{
    surfaceDecoderAdapter_ = nullptr;
}

/**
 * @tc.name: SurfaceDecoderAdapter_Init_0100
 * @tc.desc: Init
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_Init_0100, TestSize.Level1)
{
    Status status = surfaceDecoderAdapter_->Init("");
    ASSERT_EQ(status, Status::ERROR_UNKNOWN);
    status = surfaceDecoderAdapter_->Init("video/mp4");
    ASSERT_EQ(status, Status::ERROR_UNKNOWN);
    surfaceDecoderAdapter_->releaseBufferTask_ = nullptr;
    status = surfaceDecoderAdapter_->Init("video/mp4");
    surfaceDecoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("SurfaceDecoder");
    status = surfaceDecoderAdapter_->Init("video/mp4");
    ASSERT_NE(surfaceDecoderAdapter_->releaseBufferTask_, std::make_shared<Task>("SurfaceDecoder"));
}
/**
 * @tc.name: SurfaceDecoderAdapter_Configure_0100
 * @tc.desc: Configure
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_Configure_0100, TestSize.Level1)
{
    surfaceDecoderAdapter_->codecServer_ = nullptr;
    Format format;
    EXPECT_EQ(surfaceDecoderAdapter_->Configure(format), Status::ERROR_UNKNOWN);
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(surfaceDecoderAdapter_->Configure(format), Status::ERROR_UNKNOWN);
}
/**
 * @tc.name: SurfaceDecoderAdapter_GetInputBufferQueue_0100
 * @tc.desc: GetInputBufferQueue
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_GetInputBufferQueue_0100, TestSize.Level1)
{
    surfaceDecoderAdapter_->inputBufferQueue_ = AVBufferQueue::Create(0, MemoryType::UNKNOWN_MEMORY,
                                                    "inputBufferQueue", true);
    sptr<OHOS::Media::AVBufferQueueProducer> producer = surfaceDecoderAdapter_->GetInputBufferQueue();
    ASSERT_NE(producer, nullptr);
    surfaceDecoderAdapter_->inputBufferQueue_ = nullptr;
    producer = surfaceDecoderAdapter_->GetInputBufferQueue();
    ASSERT_NE(producer, nullptr);
}
/**
 * @tc.name: SurfaceDecoderAdapter_SetDecoderAdapterCallback_0100
 * @tc.desc: SetDecoderAdapterCallback
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_SetDecoderAdapterCallback_0100, TestSize.Level1)
{
    surfaceDecoderAdapter_->decoderAdapterCallback_ = std::make_shared<MyDecoderAdapterCallback>();
    EXPECT_EQ(Status::ERROR_UNKNOWN,
             surfaceDecoderAdapter_->SetDecoderAdapterCallback(surfaceDecoderAdapter_->decoderAdapterCallback_));
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(Status::OK,
             surfaceDecoderAdapter_->SetDecoderAdapterCallback(surfaceDecoderAdapter_->decoderAdapterCallback_));
}
/**
 * @tc.name: SurfaceDecoderAdapter_SetOutputSurface_0100
 * @tc.desc: SetOutputSurface
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_SetOutputSurface_0100, TestSize.Level1)
{
    sptr<Surface> surface = nullptr;
    surfaceDecoderAdapter_->codecServer_ = nullptr;
    EXPECT_EQ(surfaceDecoderAdapter_->SetOutputSurface(surface), Status::ERROR_UNKNOWN);
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(surfaceDecoderAdapter_->SetOutputSurface(surface), Status::OK);
}
/**
 * @tc.name: SurfaceDecoderAdapter_Start_0100
 * @tc.desc: Start
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_Start_0100, TestSize.Level1)
{
    EXPECT_EQ(surfaceDecoderAdapter_->Start(), Status::ERROR_UNKNOWN);
    surfaceDecoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("SurfaceDecoder");
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(surfaceDecoderAdapter_->Start(), Status::OK);
}
/**
 * @tc.name: SurfaceDecoderAdapter_Stop_0100
 * @tc.desc: Stop
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_Stop_0100, TestSize.Level1)
{
    surfaceDecoderAdapter_->releaseBufferTask_ = std::make_shared<Task>("SurfaceDecoder");
    EXPECT_EQ(surfaceDecoderAdapter_->Start(), Status::ERROR_UNKNOWN);
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(surfaceDecoderAdapter_->Start(), Status::OK);
}
/**
 * @tc.name: SurfaceDecoderAdapter_Flush_0100
 * @tc.desc: Flush
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_Flush_0100, TestSize.Level1)
{
    EXPECT_EQ(surfaceDecoderAdapter_->Flush(), Status::ERROR_UNKNOWN);
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(surfaceDecoderAdapter_->Flush(), Status::OK);
}
/**
 * @tc.name: SurfaceDecoderAdapter_Release_0100
 * @tc.desc: Release
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_Release_0100, TestSize.Level1)
{
    EXPECT_EQ(surfaceDecoderAdapter_->Release(), Status::OK);
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(surfaceDecoderAdapter_->Release(), Status::OK);
}
/**
 * @tc.name: SurfaceDecoderAdapter_SetParameter_0100
 * @tc.desc: SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_SetParameter_0100, TestSize.Level1)
{
    Format format;
    EXPECT_EQ(surfaceDecoderAdapter_->SetParameter(format), Status::ERROR_UNKNOWN);
    surfaceDecoderAdapter_->codecServer_ = std::make_shared<MyAVCodecVideoDecoder>();
    EXPECT_EQ(surfaceDecoderAdapter_->SetParameter(format), Status::OK);
}
/**
 * @tc.name: SurfaceDecoderAdapter_OnInputBufferAvailable_0100
 * @tc.desc: OnInputBufferAvailable
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_OnInputBufferAvailable_0100, TestSize.Level1)
{
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer();
    buffer->meta_ = std::make_shared<Meta>();
    surfaceDecoderAdapter_->inputBufferQueueConsumer_ = nullptr;
    surfaceDecoderAdapter_->OnInputBufferAvailable(index, buffer);
    surfaceDecoderAdapter_->inputBufferQueueConsumer_ = sptr<Media::AVBufferQueueConsumer>();
    surfaceDecoderAdapter_->OnInputBufferAvailable(index, buffer);
}
/**
 * @tc.name: SurfaceDecoderAdapter_OnOutputBufferAvailable_0100
 * @tc.desc: OnOutputBufferAvailable
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_OnOutputBufferAvailable_0100, TestSize.Level1)
{
    uint32_t index = 1;
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    buffer->flag_ = 1;
    surfaceDecoderAdapter_->OnOutputBufferAvailable(index, buffer);
    buffer->flag_ = 0;
    buffer->pts_ = 0;
    surfaceDecoderAdapter_->OnOutputBufferAvailable(index, buffer);
    buffer->flag_ = 0;
    buffer->pts_ = 1;
    surfaceDecoderAdapter_->lastBufferPts_ = 0;
    surfaceDecoderAdapter_->OnOutputBufferAvailable(index, buffer);
    EXPECT_EQ(buffer->pts_, surfaceDecoderAdapter_->lastBufferPts_);
}
/**
 * @tc.name: SurfaceDecoderAdapter_ReleaseBuffer_0100
 * @tc.desc: ReleaseBuffer
 * @tc.type: FUNC
 */
HWTEST_F(SurfaceDecoderUnitTest, SurfaceDecoderAdapter_ReleaseBuffer_0100, TestSize.Level1)
{
    surfaceDecoderAdapter_->isThreadExit_ = true;
    surfaceDecoderAdapter_->ReleaseBuffer();
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS