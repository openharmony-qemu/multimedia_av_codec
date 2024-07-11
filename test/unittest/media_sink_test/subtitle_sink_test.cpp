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
#include "subtitle_sink.h"
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
    }

private:
};

std::shared_ptr<SubtitleSink> SubtitleSinkCreate()
{
    auto sink = std::make_shared<SubtitleSink>();
    auto meta = std::make_shared<Meta>();
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    sink->Init(meta, testEventReceiver);
    sink->SetParameter(meta);
    sink->GetParameter(meta);
    sink->SetIsTransitent(false);
    sink->SetEventReceiver(testEventReceiver);
    sink->DrainOutputBuffer(false);
    sink->ResetSyncInfo();
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    return sink;
}

HWTEST(TestSubtitleSink, do_sync_write_not_eos, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    sink->Prepare();
    auto bufferQP = sink->GetBufferQueueProducer();
    ASSERT_TRUE(bufferQP != nullptr);
    auto bufferQC = sink->GetBufferQueueConsumer();
    ASSERT_TRUE(bufferQC != nullptr);
    sink->Start();
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    std::shared_ptr<AVBuffer> buffer = nullptr;
    auto ret = bufferQP->RequestBuffer(buffer, config, 1000);
    ASSERT_TRUE(ret == Status::OK);
    ASSERT_TRUE(buffer != nullptr && buffer->memory_ != nullptr);
    auto addr = buffer->memory_->GetAddr();
    char subtitle[] = "test";
    memcpy_s(addr, 4, subtitle, 4);
    ret = bufferQP->ReturnBuffer(buffer, true);
}

HWTEST(TestSubtitleSink, do_sync_write_two_frames, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);

    sink->Prepare();
    sink->Start();

    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    buffer->flag_ = 0; // not eos
    sink->DoSyncWrite(buffer);
    const std::shared_ptr<AVBuffer> buffer2 = AVBuffer::CreateAVBuffer(config);
    buffer->flag_ = 0; // not eos
    sink->DoSyncWrite(buffer2);
    sink->SetSpeed(2.0F);
    sink->Flush();
    sink->Pause();
    sink->NotifySeek();
    sink->Resume();
    sink->Stop();
    sink->Release();
}

HWTEST(TestSubtitleSink, do_sync_write_eos, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    AVBufferConfig config;
    config.size = 4;
    config.memoryType = MemoryType::SHARED_MEMORY;
    const std::shared_ptr<AVBuffer> buffer = AVBuffer::CreateAVBuffer(config);
    buffer->flag_ = 1; // eos
    sink->DoSyncWrite(buffer);
    sink->DoSyncWrite(buffer);
    buffer->flag_ = BUFFER_FLAG_EOS;
    sink->DoSyncWrite(buffer);
}
}  // namespace Test
}  // namespace Media
}  // namespace OHOS