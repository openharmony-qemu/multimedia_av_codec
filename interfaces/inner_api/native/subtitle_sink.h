/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_SUBTITLE_SINK_H
#define HISTREAMER_SUBTITLE_SINK_H
#include <mutex>
#include "common/status.h"
#include "meta/meta.h"
#include "sink/media_synchronous_sink.h"
#include "media_sync_manager.h"
#include "buffer/avbuffer_queue.h"
#include "buffer/avbuffer_queue_define.h"
#include "filter/filter.h"

namespace OHOS {
namespace Media {
using namespace OHOS::Media::Plugins;
class SubtitleSink : public std::enable_shared_from_this<SubtitleSink>, public Pipeline::MediaSynchronousSink {
public:
    SubtitleSink();
    ~SubtitleSink();
    Status Init(std::shared_ptr<Meta>& meta, const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    sptr<AVBufferQueueProducer> GetBufferQueueProducer();
    sptr<AVBufferQueueConsumer> GetBufferQueueConsumer();
    Status SetParameter(const std::shared_ptr<Meta>& meta);
    Status GetParameter(std::shared_ptr<Meta>& meta);
    Status Prepare();
    Status Start();
    Status Stop();
    Status Pause();
    Status Resume();
    Status Flush();
    Status Release();
    void DrainOutputBuffer(bool flushed);
    void SetEventReceiver(const std::shared_ptr<Pipeline::EventReceiver>& receiver);
    Status GetLatency(uint64_t& nanoSec);
    void SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter);
    int64_t DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer) override;
    void ResetSyncInfo() override;
    Status SetIsTransitent(bool isTransitent);
    void NotifySeek();
    Status SetSpeed(float speed);
protected:
    std::atomic<OHOS::Media::Pipeline::FilterState> state_;
private:
    struct SubtitleInfo {
        std::string text_;
        int64_t pts_;
        int64_t duration_;
    };
    void NotifyRender(SubtitleInfo &subtitleInfo);
    void RenderLoop();
    uint64_t CalcWaitTime(SubtitleInfo &subtitleInfo);
    uint32_t ActionToDo(SubtitleInfo &subtitleInfo);
    void GetTargetSubtitleIndex(int64_t currentTime);
    Status PrepareInputBufferQueue();
    int64_t getDurationUsPlayedAtSampleRate(uint32_t numFrames);
    int64_t GetMediaTime();
    std::shared_ptr<Pipeline::EventReceiver> playerEventReceiver_;
    int32_t appUid_{0};
    int32_t appPid_{0};
    int64_t numFramesWritten_ {0};
    int64_t lastReportedClockTime_ {HST_TIME_NONE};
    int64_t latestBufferPts_ {HST_TIME_NONE};
    int64_t latestBufferDuration_ {0};
    const std::string INPUT_BUFFER_QUEUE_NAME = "SubtitleSinkInputBufferQueue";
    std::shared_ptr<AVBufferQueue> inputBufferQueue_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;
    sptr<AVBufferQueueConsumer> inputBufferQueueConsumer_;
    bool isTransitent_ {false};
    std::atomic<bool> isEos_{false};
    std::unique_ptr<std::thread> readThread_ = nullptr;
    std::mutex mutex_;
    std::condition_variable updateCond_;
    std::shared_ptr<AVBuffer> filledOutputBuffer_;
    std::atomic<bool> isPaused_{false};
    std::atomic<bool> isThreadExit_{false};
    std::atomic<bool> shouldUpdate_{false};
    float speed_ = 1.0;
    enum SubtitleBufferState : uint32_t {
        WAIT,
        SHOW,
        DROP,
    };
    std::vector<SubtitleInfo> subtitleInfoVec_;
    uint32_t currentInfoIndex_ = 0;
    std::atomic<bool> isFlush_ = false;
    std::vector<std::shared_ptr<AVBuffer>> inputBufferVector_;
};
}
}

#endif // HISTREAMER_AUDIO_SINK_H
