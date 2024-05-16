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
    constexpr uint64_t defaultValue = 0;
    static uint64_t fixDelay = OHOS::system::GetUintParameter("debug.media_service.video_sync_fix_delay", defaultValue);
    MEDIA_LOG_I("video_sync_fix_delay, pid:%{public}d, fixdelay: " PUBLIC_LOG_U64, getprocpid(), fixDelay);
    return (int64_t)fixDelay;
}

/// Video Key Frame Flag
constexpr int BUFFER_FLAG_KEY_FRAME = 0x00000002;

// Video Sync Start Frame
constexpr int VIDEO_SINK_START_FRAME = 4;

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

int64_t VideoSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, false);
    int64_t waitTime = 0;
    bool render = true;
    if ((buffer->flag_ & BUFFER_FLAG_EOS) == 0) {
        auto syncCenter = syncCenter_.lock();
        int64_t nowCt = syncCenter ? syncCenter->GetClockTimeNow() : 0;
        if (isFirstFrame_) {
            eventReceiver_->OnEvent({"video_sink", EventType::EVENT_VIDEO_RENDERING_START, Status::OK});
            FALSE_RETURN_V(syncCenter != nullptr, false);
            isFirstFrame_ = false;
            firstFrameNowct_ = nowCt;
            firstFramePts_ = buffer->pts_;
        } else {
            waitTime = CheckBufferLatenessMayWait(buffer);
        }
        if (syncCenter) {
            uint64_t latency = 0;
            if (GetLatency(latency) != Status::OK) {
                MEDIA_LOG_I("failed to get latency, treat as 0");
            }
            render = syncCenter->UpdateTimeAnchor(nowCt + waitTime, latency, buffer->pts_ - firstPts_,
                buffer->pts_, buffer->duration_, this);
        }
        lastTimeStamp_ = buffer->pts_ - firstPts_;
    } else {
        MEDIA_LOG_I("Video sink EOS");
        return -1;
    }
    if ((render && waitTime >= 0) || lastFrameDropped_) {
        lastFrameDropped_ = false;
        renderFrameCnt_++;
        return waitTime > 0 ? waitTime : 0;
    }
    lastFrameDropped_ = true;
    discardFrameCnt_++;
    return -1;
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

int64_t VideoSink::CheckBufferLatenessMayWait(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, true);
    bool tooLate = false;
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(syncCenter != nullptr, true);
    auto pts = buffer->pts_ - firstPts_;
    auto ct4Buffer = syncCenter->GetClockTime(pts);

    MEDIA_LOG_D("VideoSink cur pts: " PUBLIC_LOG_D64 " us, ct4Buffer: " PUBLIC_LOG_D64 " us, buf_pts: " PUBLIC_LOG_D64
        " us, fixDelay: " PUBLIC_LOG_D64 " us", pts, ct4Buffer, buffer->pts_, fixDelay_);
    int64_t waitTimeUs = 0;
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
        // use video first render time as anchor when first few times
        if (discardFrameCnt_ + renderFrameCnt_ < VIDEO_SINK_START_FRAME) {
            diff = (nowCt - firstFrameNowct_) - (buffer->pts_ - firstFramePts_);
            MEDIA_LOG_I("VideoSink first few times diff is " PUBLIC_LOG_D64 " us", diff);
        }
        MEDIA_LOG_D("VideoSink ct4Buffer: " PUBLIC_LOG_D64 " us, diff: " PUBLIC_LOG_D64
                " us, nowCt: " PUBLIC_LOG_D64, ct4Buffer, diff, nowCt);
        // diff < 0 or 0 < diff < 40ms(25Hz) render it
        if (diff < 0) {
            // buffer is early
            waitTimeUs = 0 - diff;
            if (waitTimeUs > WAIT_TIME_US_THRESHOLD) {
                MEDIA_LOG_W("buffer is too early waitTimeUs: " PUBLIC_LOG_D64, waitTimeUs);
                waitTimeUs = WAIT_TIME_US_THRESHOLD;
            }
        } else if (diff > 0 && Plugins::HstTime2Ms(diff * HST_USECOND) > 40) { // > 40ms
            // buffer is late
            tooLate = true;
            MEDIA_LOG_D("buffer is too late");
        }
        lastBufferTime_ = ct4Buffer;
        // buffer is too late and is not key frame drop it
        if (tooLate && (buffer->flag_ & BUFFER_FLAG_KEY_FRAME) == 0) {
            return -1;
        }
    }
    return waitTimeUs;
}

void VideoSink::SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter)
{
    MEDIA_LOG_I("VideoSink::SetSyncCenter");
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
