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
        (void)event;
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
    return sink;
}

HWTEST(TestSubtitleSink, do_sync_write_not_eos, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->PrepareInputBufferQueue();
    sink->Prepare();
    sink->state_ = Pipeline::FilterState::READY;
    auto bufferQP = sink->GetBufferQueueProducer();
    ASSERT_TRUE(bufferQP != nullptr);
    auto bufferQC = sink->GetBufferQueueConsumer();
    ASSERT_TRUE(bufferQC != nullptr);
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    sink->isThreadExit_ = true;
    sink->RenderLoop();
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_two_frames_case1, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->Prepare();
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    SubtitleSink::SubtitleInfo tempSubtitleInfo = {{"test", 1, 1}};
    sink->NotifyRender(tempSubtitleInfo);
    sink->isThreadExit_ = true;
    sink->CalcWaitTime(tempSubtitleInfo);
    sink->ActionToDo(tempSubtitleInfo);
    sink->RenderLoop();
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_prepare_two_frames_case2, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->PrepareInputBufferQueue();
    sink->PrepareInputBufferQueue();
    sink->Prepare();
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    sink->shouldUpdate_ = true;
    SubtitleSink::SubtitleInfo tempSubtitleInfo = {{"test", 1, 1}};
    std::shared_ptr<EventReceiver> testEventReceiver = std::make_shared<TestEventReceiver>();
    sink->SetEventReceiver(testEventReceiver);
    sink->NotifyRender(tempSubtitleInfo);
    sink->isThreadExit_ = true;
    sink->RenderLoop();
    sink->isEos_ = false;
    sink->filledOutputBuffer_ = std::make_shared<AVBuffer>();
    sink->filledOutputBuffer_->flag_ = 1;
    sink->filledOutputBuffer_->memory_= std::make_shared<AVMemory>();
    sink->DrainOutputBuffer(true);
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_prepare_two_frames_case3, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->PrepareInputBufferQueue();
    sink->PrepareInputBufferQueue();
    sink->Prepare();
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    sink->isThreadExit_ = true;
    sink->shouldUpdate_ = false;
    sink->RenderLoop();
    sink->filledOutputBuffer_ = std::make_shared<AVBuffer>();
    sink->filledOutputBuffer_->flag_ = 1;
    sink->filledOutputBuffer_->memory_= std::make_shared<AVMemory>();
    sink->DrainOutputBuffer(true);
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_prepare_two_frames_case4, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->PrepareInputBufferQueue();
    sink->PrepareInputBufferQueue();
    sink->Prepare();
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    sink->isThreadExit_ = true;
    sink->RenderLoop();
    sink->filledOutputBuffer_ = std::make_shared<AVBuffer>();
    sink->filledOutputBuffer_->flag_ = 1;
    sink->filledOutputBuffer_->memory_= std::make_shared<AVMemory>();
    sink->DrainOutputBuffer(true);
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_prepare_two_frames_case5, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->PrepareInputBufferQueue();
    sink->PrepareInputBufferQueue();
    sink->Prepare();
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    sink->subtitleInfoVec_.push_back({"test", 2, 1});
    sink->isThreadExit_ = true;
    sink->RenderLoop();
    sink->filledOutputBuffer_ = std::make_shared<AVBuffer>();
    sink->filledOutputBuffer_->flag_ = 1;
    sink->filledOutputBuffer_->memory_= std::make_shared<AVMemory>();
    sink->DrainOutputBuffer(true);
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_prepare_two_frames_case6, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->PrepareInputBufferQueue();
    sink->PrepareInputBufferQueue();
    sink->Prepare();
    sink->isThreadExit_ = true;
    sink->RenderLoop();
    sink->filledOutputBuffer_ = std::make_shared<AVBuffer>();
    sink->filledOutputBuffer_->flag_ = 1;
    sink->filledOutputBuffer_->memory_= std::make_shared<AVMemory>();
    sink->DrainOutputBuffer(true);
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_two_frames_case7, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->Prepare();
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    SubtitleSink::SubtitleInfo tempSubtitleInfo = {{"test", 1, 1}};
    sink->NotifyRender(tempSubtitleInfo);
    sink->isThreadExit_ = true;
    sink->CalcWaitTime(tempSubtitleInfo);
    sink->ActionToDo(tempSubtitleInfo);
    sink->RenderLoop();
 
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
 
HWTEST(TestSubtitleSink, do_sync_write_two_frames_case8, TestSize.Level1)
{
    auto sink = SubtitleSinkCreate();
    ASSERT_TRUE(sink != nullptr);
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
    sink->Prepare();
    sink->subtitleInfoVec_.push_back({"test", 1, 1});
    SubtitleSink::SubtitleInfo tempSubtitleInfo = {{"test", 2, 2}};
    sink->NotifyRender(tempSubtitleInfo);
 
    sink->CalcWaitTime(tempSubtitleInfo);
    sink->ActionToDo(tempSubtitleInfo);
    sink->isThreadExit_ = true;
    sink->RenderLoop();
 
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
    auto syncCenter = std::make_shared<MediaSyncManager>();
    sink->SetSyncCenter(syncCenter);
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