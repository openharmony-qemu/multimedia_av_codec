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

#include "video_decoder_adapter_unit_test.h"
#include <malloc.h>
#include <map>
#include <unistd.h>
#include <vector>
#include "avcodec_video_decoder.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"
#include "common/log.h"
#include "media_description.h"
#include "surface_type.h"
#include "buffer/avbuffer_queue_consumer.h"
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "video_decoder_adapter.h"
#include "media_core.h"
#include "avcodec_sysevent.h"

namespace OHOS::Media {

using namespace std;
using namespace testing::ext;
void VideoDecoderAdapterUnitTest::SetUpTestCase(void)
{
}

void VideoDecoderAdapterUnitTest::TearDownTestCase(void)
{
}

void VideoDecoderAdapterUnitTest::SetUp()
{
}

void VideoDecoderAdapterUnitTest::TearDown()
{
}

HWTEST_F(VideoDecoderAdapterUnitTest, VideoDecoderAdapter_001, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoResize = std::make_shared<VideoDecoderAdapter>();
    Status ret = videoResize->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, true, "name");
    EXPECT_EQ(ret, Status::ERROR_INVALID_STATE);
}

HWTEST_F(VideoDecoderAdapterUnitTest, VideoDecoderAdapter_002, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoDecoder = std::make_shared<VideoDecoderAdapter>();
    std::shared_ptr<TestAVCodecVideoDecoder> mediaCodecTest = std::make_shared<TestAVCodecVideoDecoder>();
    videoDecoder->mediaCodec_ = mediaCodecTest;
    EXPECT_EQ(videoDecoder->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, true, "name"), Status::OK);
    videoDecoder->SetEventReceiver(nullptr);
    videoDecoder->SetCallingInfo(0, 0, "test", 0);

    Format format;
    EXPECT_EQ(videoDecoder->Configure(format), Status::OK);
    EXPECT_EQ(videoDecoder->SetParameter(format), 0);
    EXPECT_EQ(videoDecoder->Start(), Status::OK);

    EXPECT_EQ(videoDecoder->Flush(), Status::OK);
    EXPECT_EQ(videoDecoder->Stop(), Status::OK);
    EXPECT_EQ(videoDecoder->Reset(), Status::OK);
    EXPECT_EQ(videoDecoder->Release(), Status::OK);

    videoDecoder->ResetRenderTime();

    EXPECT_EQ(videoDecoder->GetBufferQueueProducer(), nullptr);
    EXPECT_EQ(videoDecoder->GetBufferQueueConsumer(), nullptr);
    EXPECT_EQ(videoDecoder->RenderOutputBufferAtTime(0, 0), 0);

    videoDecoder->GetCurrentMillisecond();
    int32_t lagTimes = 0;
    int32_t maxLagDuration = 0;
    int32_t avgLagDuration = 0;
    EXPECT_EQ(videoDecoder->GetLagInfo(lagTimes, maxLagDuration, avgLagDuration), Status::OK);

    mediaCodecTest->Init(1);
    videoDecoder->mediaCodec_ = mediaCodecTest;
    EXPECT_EQ(videoDecoder->Start(), Status::ERROR_INVALID_STATE);

    videoDecoder->PrepareInputBufferQueue();
    videoDecoder->OnDumpInfo(-1);
    std::shared_ptr<TestMediaCodecCallback> callback = std::make_shared<TestMediaCodecCallback>();
    EXPECT_EQ(videoDecoder->SetCallback(callback), 1);
}

HWTEST_F(VideoDecoderAdapterUnitTest, VideoDecoderAdapter_003, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoDecoder = std::make_shared<VideoDecoderAdapter>();
    std::shared_ptr<TestAVCodecVideoDecoder> mediaCodecTest = std::make_shared<TestAVCodecVideoDecoder>();
    videoDecoder->mediaCodec_ = mediaCodecTest;
    EXPECT_EQ(videoDecoder->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, true, "name"), Status::OK);

    EXPECT_EQ(videoDecoder->SetOutputSurface(nullptr), 0);
    int32_t lagTimes = 1;
    int32_t maxLagDuration = 0;
    int32_t avgLagDuration = 0;
    EXPECT_EQ(videoDecoder->GetLagInfo(lagTimes, maxLagDuration, avgLagDuration), Status::OK);

    std::shared_ptr<TestMediaCodecCallback> callback = std::make_shared<TestMediaCodecCallback>();
    EXPECT_EQ(videoDecoder->SetCallback(callback), 0);
    videoDecoder->OnOutputBufferAvailable(0, nullptr);

    Format format;
    videoDecoder->OnOutputFormatChanged(format);
    videoDecoder->OnError(MediaAVCodec::AVCodecErrorType::AVCODEC_ERROR_DECRYTION_FAILED, 11);
    EXPECT_EQ(videoDecoder->GetOutputFormat(format), 0);
    videoDecoder->currentTime_ = 0;
    EXPECT_EQ(videoDecoder->ReleaseOutputBuffer(1, true), 0);
}

HWTEST_F(VideoDecoderAdapterUnitTest, VideoDecoderAdapter_004, TestSize.Level1)
{
    std::shared_ptr<VideoDecoderAdapter> videoDecoder = std::make_shared<VideoDecoderAdapter>();
    std::shared_ptr<TestAVCodecVideoDecoder> mediaCodecTest = std::make_shared<TestAVCodecVideoDecoder>();
    videoDecoder->mediaCodec_ = mediaCodecTest;
    EXPECT_EQ(videoDecoder->Init(MediaAVCodec::AVCodecType::AVCODEC_TYPE_VIDEO_DECODER, true, "name"), Status::OK);
    videoDecoder->PrepareInputBufferQueue();
   
    videoDecoder->AquireAvailableInputBuffer();
    std::shared_ptr<AVBuffer> buffer;
    videoDecoder->OnInputBufferAvailable(100, buffer);
    videoDecoder->isConfigured_ = true;
    EXPECT_EQ(videoDecoder->Flush(), Status::OK);
    EXPECT_EQ(videoDecoder->Reset(), Status::OK);
}

}