/*
* Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include "video_sink.h"

#include <algorithm>

#include "common/log.h"
#include "media_sync_manager.h"
#include "osal/task/jobutils.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
/// Video Key Frame Flag
constexpr int BUFFER_FLAG_KEY_FRAME = 0x00000002;
constexpr int64_t WAIT_TIME_MS_THRESHOLD = 80;

VideoSink::VideoSink()
{
    refreshTime_ = 0;
    syncerPriority_ = IMediaSynchronizer::VIDEO_SINK;
    MEDIA_LOG_I("VideoSink ctor called...");
}

VideoSink::~VideoSink()
{
    MEDIA_LOG_I("VideoSink dtor called...");
    this->eventReceiver_ = nullptr;
}

bool VideoSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, false);
    bool shouldDrop = false;
    bool render = true;
    if ((buffer->flag_ & BUFFER_FLAG_EOS) == 0) {
        if (isFirstFrame_) {
            if (firstPts_ == HST_TIME_NONE) {
                firstPts_ = buffer->pts_;
            }
            eventReceiver_->OnEvent({"video_sink", EventType::EVENT_VIDEO_RENDERING_START, Status::OK});
            int64_t nowCt = 0;
            auto syncCenter = syncCenter_.lock();
            FALSE_RETURN_V(syncCenter != nullptr, false);
            if (syncCenter) {
                nowCt = syncCenter->GetClockTimeNow();
            }
            uint64_t latency = 0;
            if (GetLatency(latency) != Status::OK) {
                MEDIA_LOG_I("failed to get latency, treat as 0");
            }
            if (syncCenter) {
                render = syncCenter->UpdateTimeAnchor(nowCt + latency, buffer->pts_ - firstPts_, 
                    buffer->duration_, this);
            }
            isFirstFrame_ = false;
        } else {
            shouldDrop = CheckBufferLatenessMayWait(buffer);
        }
        if (forceRenderNextFrame_) {
            shouldDrop = false;
            forceRenderNextFrame_ = false;
        }
    }
    if (shouldDrop) {
        discardFrameCnt_++;
        MEDIA_LOG_DD("drop buffer with pts " PUBLIC_LOG_D64 " due to too late", buffer->pts_);
        return false;
    } else if (!render) {
        discardFrameCnt_++;
        MEDIA_LOG_DD("drop buffer with pts " PUBLIC_LOG_D64 " due to seek not need to render", buffer->pts_);
        return false;
    } else {
        renderFrameCnt_++;
        return true;
    }
}

void VideoSink::ResetSyncInfo()
{
    ResetPrerollReported();
    isFirstFrame_ = true;
    firstPts_ = HST_TIME_NONE;
}

Status VideoSink::GetLatency(uint64_t& nanoSec)
{
    nanoSec = 10; // 10 ns
    return Status::OK;
}

bool VideoSink::CheckBufferLatenessMayWait(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, true);
    bool tooLate = false;
    auto syncCenter = syncCenter_.lock();
    if (!syncCenter) {
        return false;
    }
    auto ct4Buffer = syncCenter->GetClockTime(buffer->pts_);
    if (ct4Buffer != Plugins::HST_TIME_NONE) {
        auto nowCt = syncCenter->GetClockTimeNow();
        uint64_t latency = 0;
        GetLatency(latency);
        auto diff = nowCt + (int64_t) latency - ct4Buffer;
        // diff < 0 or 0 < diff < 40ms(25Hz) render it
        if (diff < 0) {
            // buffer is early
            auto waitTimeMs = Plugins::HstTime2Ms(0 - diff * HST_USECOND);
            MEDIA_LOG_DD("buffer is eary, sleep for " PUBLIC_LOG_D64 " ms", waitTimeMs);
            // use a threshold to prevent sleeping abnormally for seek
            waitTimeMs = std::min(waitTimeMs, WAIT_TIME_MS_THRESHOLD);
            OHOS::Media::SleepInJob(waitTimeMs);
        } else if (diff > 0 && Plugins::HstTime2Ms(diff * HST_USECOND) > 40) { // > 40ms
            // buffer is late
            tooLate = true;
            MEDIA_LOG_DD("buffer is too late");
        }
        // buffer is too late and is not key frame drop it
        if (tooLate && (buffer->flag_ & BUFFER_FLAG_KEY_FRAME) == 0) {
            return true;
        }
    }
    return false;
}

void VideoSink::SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter)
{
    syncCenter_ = syncCenter;
    MediaSynchronousSink::Init();
}

void VideoSink::SetEventReceiver(const std::shared_ptr<EventReceiver> &receiver)
{
    this->eventReceiver_ = receiver;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS