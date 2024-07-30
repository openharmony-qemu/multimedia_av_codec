/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include "gtest/gtest.h"
#include "filter/filter.h"
#include "video_sink.h"
#include "sink/media_synchronous_sink.h"

using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Test {
using namespace Pipeline;

class TestEventReceiver : public EventReceiver {
public:
    explicit TestEventReceiver()
    {
    }

    void OnEvent(const Event &event)
    {
        (void)event;
    }

private:
};

std::shared_ptr<VideoSink> VideoSinkCreate()
{
    auto videoSink = std::make_shared<VideoSink>();
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    videoSink->SetEventReceiver(testEventReceiver);
    auto meta = std::make_shared<Meta>();
    videoSink->SetParameter(meta);
    videoSink->ResetSyncInfo();
    videoSink->SetLastPts(0);
    videoSink->SetFirstPts(HST_TIME_NONE);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    videoSink->SetSyncCenter(syncCenter);
    videoSink->SetSeekFlag();
    return videoSink;
}

HWTEST(TestVideoSink, do_sync_write_not_eos, TestSize.Level1)
{
    auto videoSink = std::make_shared<VideoSink>();
    ASSERT_TRUE(videoSink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    ASSERT_TRUE(syncCenter != nullptr);
    videoSink->SetSyncCenter(syncCenter);
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    ASSERT_TRUE(testEventReceiver != nullptr);
    videoSink->SetEventReceiver(testEventReceiver);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    auto setParam = videoSink->SetParameter(meta);
    ASSERT_TRUE(setParam == Status::OK);
    videoSink->ResetSyncInfo();
    videoSink->SetLastPts(0);
    videoSink->SetFirstPts(HST_TIME_NONE);
    videoSink->SetSeekFlag();
    uint64_t latency = 0;
    auto getLatency = videoSink->GetLatency(latency);
    ASSERT_TRUE(getLatency == Status::OK);
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    buffer->flag_ = 0; // not eos
    videoSink->DoSyncWrite(buffer);
    buffer->flag_ = BUFFER_FLAG_EOS;
    videoSink->DoSyncWrite(buffer);
    buffer->pts_ = 1;
    videoSink->lastBufferTime_ = 1;
    videoSink->seekFlag_ = false;
    (void)videoSink->CheckBufferLatenessMayWait(buffer);
    float speed = 0;
    videoSink->GetSpeed(speed);
}

HWTEST(TestVideoSink, do_sync_write_two_frames, TestSize.Level1)
{
    auto videoSink = std::make_shared<VideoSink>();
    ASSERT_TRUE(videoSink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    ASSERT_TRUE(syncCenter != nullptr);
    videoSink->SetSyncCenter(syncCenter);
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    ASSERT_TRUE(testEventReceiver != nullptr);
    videoSink->SetEventReceiver(testEventReceiver);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    auto setParam = videoSink->SetParameter(meta);
    ASSERT_TRUE(setParam == Status::OK);
    videoSink->ResetSyncInfo();
    videoSink->SetLastPts(0);
    videoSink->SetFirstPts(HST_TIME_NONE);
    videoSink->SetSeekFlag();
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    buffer->flag_ = 0; // not eos
    videoSink->DoSyncWrite(buffer);
    const std::shared_ptr<AVBuffer> buffer2 = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer2 != nullptr);
    buffer->flag_ = 0; // not eos
    videoSink->DoSyncWrite(buffer2);
    buffer->pts_ = 1;
    videoSink->lastBufferTime_ = 1;
    videoSink->seekFlag_ = false;
    (void)videoSink->CheckBufferLatenessMayWait(buffer);
    float speed = 0;
    videoSink->GetSpeed(speed);
}

HWTEST(TestVideoSink, do_sync_write_eos, TestSize.Level1)
{
    auto videoSink = std::make_shared<VideoSink>();
    ASSERT_TRUE(videoSink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    ASSERT_TRUE(syncCenter != nullptr);
    videoSink->SetSyncCenter(syncCenter);
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    ASSERT_TRUE(testEventReceiver != nullptr);
    videoSink->SetEventReceiver(testEventReceiver);
    auto meta = std::make_shared<Meta>();
    ASSERT_TRUE(meta != nullptr);
    auto setParam = videoSink->SetParameter(meta);
    ASSERT_TRUE(setParam == Status::OK);
    videoSink->ResetSyncInfo();
    videoSink->SetLastPts(0);
    videoSink->SetFirstPts(HST_TIME_NONE);
    videoSink->SetSeekFlag();
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    ASSERT_TRUE(buffer != nullptr);
    buffer->flag_ = 1; // eos
    videoSink->DoSyncWrite(buffer);
    videoSink->DoSyncWrite(buffer);
    buffer->flag_ = BUFFER_FLAG_EOS;
    videoSink->DoSyncWrite(buffer);
    buffer->pts_ = 1;
    videoSink->lastBufferTime_ = 1;
    videoSink->seekFlag_ = false;
    (void)videoSink->CheckBufferLatenessMayWait(buffer);
    float speed = 0;
    videoSink->GetSpeed(speed);
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS