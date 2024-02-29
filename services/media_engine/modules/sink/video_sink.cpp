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
#include "syspara/parameters.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
int64_t GetvideoLatencyFixDelay()
{
    constexpr uint64_t defaultValue = 120 * HST_USECOND;
    static uint64_t fixDelay = OHOS::system::GetUintParameter("debug.media_service.video_sync_fix_delay", defaultValue);
    MEDIA_LOG_I("video_sync_fix_delay, pid:%{public}d, fixdelay: " PUBLIC_LOG_U64, getpid(), fixDelay);
    return (int64_t)fixDelay;
}

/// Video Key Frame Flag
constexpr int BUFFER_FLAG_KEY_FRAME = 0x00000002;

constexpr int64_t WAIT_TIME_US_THRESHOLD = 1500000; // max sleep time 1.5s

VideoSink::VideoSink()
{
    refreshTime_ = 0;
    syncerPriority_ = IMediaSynchronizer::VIDEO_SINK;
    fixDelay_ = GetvideoLatencyFixDelay();
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
                render = syncCenter->UpdateTimeAnchor(nowCt, latency, buffer->pts_ - firstPts_,
                    buffer->pts_, buffer->duration_, this);
            }
            isFirstFrame_ = false;
        } else {
            shouldDrop = CheckBufferLatenessMayWait(buffer);
        }
        if (forceRenderNextFrame_) {
            shouldDrop = false;
            forceRenderNextFrame_ = false;
        }
        lastTimeStamp_ = buffer->pts_ - firstPts_;
    } else {
        MEDIA_LOG_I("Video sink EOS");
        return false;
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
    lastTimeStamp_ = HST_TIME_NONE;
    lastBufferTime_ = HST_TIME_NONE;
    seekFlag_ = false;
}

Status VideoSink::GetLatency(uint64_t& nanoSec)
{
    nanoSec = 10; // 10 ns
    return Status::OK;
}

void VideoSink::SetSeekFlag()
{
    seekFlag_ = true;
}

bool VideoSink::CheckBufferLatenessMayWait(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, true);
    bool tooLate = false;
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(syncCenter != nullptr, true);
    auto pts = buffer->pts_ - firstPts_;
    auto ct4Buffer = syncCenter->GetClockTime(pts);

    MEDIA_LOG_D("VideoSink cur pts: " PUBLIC_LOG_D64 " us, ct4Buffer: " PUBLIC_LOG_D64
        " us, fixDelay: " PUBLIC_LOG_D64 " us", pts, ct4Buffer, fixDelay_);
    if (ct4Buffer != Plugins::HST_TIME_NONE) {
        if (lastBufferTime_ != HST_TIME_NONE && seekFlag_ == false) {
            int64_t thisBufferTime = lastBufferTime_ + pts - lastTimeStamp_;
            int64_t deltaTime = ct4Buffer - thisBufferTime;
            deltaTimeAccu_ = (deltaTimeAccu_ * 9 + deltaTime) / 10; // 9 10 for smoothing
            if (std::abs(deltaTimeAccu_) < 5 * HST_USECOND) { // 5ms
                ct4Buffer = thisBufferTime;
            }
            MEDIA_LOG_D("VideoSink lastBufferTime_: " PUBLIC_LOG_D64
                " us, lastTimeStamp_: " PUBLIC_LOG_D64,
                lastBufferTime_, lastTimeStamp_);
        } else {
            seekFlag_ = (seekFlag_ == true) ? false : seekFlag_;
        }
        auto nowCt = syncCenter->GetClockTimeNow();
        uint64_t latency = 0;
        GetLatency(latency);
        auto diff = nowCt + (int64_t) latency - ct4Buffer + fixDelay_;
        MEDIA_LOG_D("VideoSink ct4Buffer: " PUBLIC_LOG_D64 " us, diff: " PUBLIC_LOG_D64
                " us, nowCt: " PUBLIC_LOG_D64, ct4Buffer, diff, nowCt);
        // diff < 0 or 0 < diff < 40ms(25Hz) render it
        if (diff < 0) {
            // buffer is early
            int64_t waitTimeUs = 0 - diff;
            if (waitTimeUs > WAIT_TIME_US_THRESHOLD) {
                MEDIA_LOG_W("buffer is too early waitTimeUs: " PUBLIC_LOG_D64, waitTimeUs);
                waitTimeUs = WAIT_TIME_US_THRESHOLD;
            }
            usleep(waitTimeUs);
        } else if (diff > 0 && Plugins::HstTime2Ms(diff * HST_USECOND) > 40) { // > 40ms
            // buffer is late
            tooLate = true;
            MEDIA_LOG_DD("buffer is too late");
        }
        lastBufferTime_ = ct4Buffer;
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

void VideoSink::SetFirstPts(int64_t pts)
{
    if (firstPts_ == HST_TIME_NONE) {
        firstPts_ = pts;
        MEDIA_LOG_I("video DoSyncWrite set firstPts = " PUBLIC_LOG_D64, firstPts_);
    }
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS