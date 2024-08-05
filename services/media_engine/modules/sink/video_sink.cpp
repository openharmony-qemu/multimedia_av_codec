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

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "VideoSink" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
int64_t GetvideoLatencyFixDelay()
{
    constexpr uint64_t defaultValue = 0;
    static uint64_t fixDelay = OHOS::system::GetUintParameter("debug.media_service.video_sync_fix_delay", defaultValue);
    MEDIA_LOG_I_SHORT("video_sync_fix_delay, pid:%{public}d, fixdelay: " PUBLIC_LOG_U64, getprocpid(), fixDelay);
    return (int64_t)fixDelay;
}

/// Video Key Frame Flag
constexpr int BUFFER_FLAG_KEY_FRAME = 0x00000002;

// Video Sync Start Frame
constexpr int VIDEO_SINK_START_FRAME = 4;

constexpr int64_t WAIT_TIME_US_THRESHOLD = 1500000; // max sleep time 1.5s

constexpr int64_t SINK_TIME_US_THRESHOLD = 100000; // max sink time 100ms

constexpr int64_t PER_SINK_TIME_THRESHOLD = 33000; // max per sink time 33ms

VideoSink::VideoSink()
{
    refreshTime_ = 0;
    syncerPriority_ = IMediaSynchronizer::VIDEO_SINK;
    fixDelay_ = GetvideoLatencyFixDelay();
    MEDIA_LOG_I_SHORT("ctor");
}

VideoSink::~VideoSink()
{
    MEDIA_LOG_I_SHORT("dtor");
    this->eventReceiver_ = nullptr;
}

int64_t VideoSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, false);
    int64_t waitTime = 0;
    bool render = true;
    auto syncCenter = syncCenter_.lock();
    if ((buffer->flag_ & BUFFER_FLAG_EOS) == 0) {
        int64_t nowCt = syncCenter ? syncCenter->GetClockTimeNow() : 0;
        if (isFirstFrame_) {
            FALSE_RETURN_V(syncCenter != nullptr, false);
            isFirstFrame_ = false;
            firstFrameNowct_ = nowCt;
            firstFramePts_ = buffer->pts_;
        } else {
            waitTime = CheckBufferLatenessMayWait(buffer);
        }
        if (syncCenter) {
            uint64_t latency = 0;
            (void)GetLatency(latency);
            render = syncCenter->UpdateTimeAnchor(nowCt + waitTime, latency, buffer->pts_ - firstPts_,
                buffer->pts_, buffer->duration_, this);
        }
        lastTimeStamp_ = buffer->pts_ - firstPts_;
    } else {
        MEDIA_LOG_I_SHORT("Video sink EOS");
        if (syncCenter) {
            syncCenter->ReportEos(this);
        }
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
    lastPts_ = HST_TIME_NONE;
    lastClockTime_ = HST_TIME_NONE;
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

void VideoSink::SetLastPts(int64_t lastPts)
{
    lastPts_ = lastPts;
    auto syncCenter = syncCenter_.lock();
    if (syncCenter != nullptr) {
        lastClockTime_ = syncCenter->GetClockTimeNow();
    }
}

int64_t VideoSink::CheckBufferLatenessMayWait(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, true);
    bool tooLate = false;
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(syncCenter != nullptr, true);
    auto pts = buffer->pts_ - firstPts_;
    auto ct4Buffer = syncCenter->GetClockTime(pts);
    MEDIA_LOG_D_SHORT("VideoSink cur pts: " PUBLIC_LOG_D64 " us, ct4Buffer: " PUBLIC_LOG_D64 " us, buf_pts: "
        PUBLIC_LOG_D64 " us, fixDelay: " PUBLIC_LOG_D64 " us", pts, ct4Buffer, buffer->pts_, fixDelay_);
    FALSE_RETURN_V(ct4Buffer != Plugins::HST_TIME_NONE, 0);
    int64_t waitTimeUs = 0;
    if (lastBufferTime_ != HST_TIME_NONE && seekFlag_ == false) {
        int64_t thisBufferTime = lastBufferTime_ + pts - lastTimeStamp_;
        int64_t deltaTime = ct4Buffer - thisBufferTime;
        deltaTimeAccu_ = (deltaTimeAccu_ * 9 + deltaTime) / 10; // 9 10 for smoothing
        if (std::abs(deltaTimeAccu_) < 5 * HST_USECOND) { // 5ms
            ct4Buffer = thisBufferTime;
        }
        MEDIA_LOG_D_SHORT("lastBfTime:" PUBLIC_LOG_D64" us, lastTS:" PUBLIC_LOG_D64, lastBufferTime_, lastTimeStamp_);
    } else {
        seekFlag_ = (seekFlag_ == true) ? false : seekFlag_;
    }
    auto nowCt = syncCenter->GetClockTimeNow();
    uint64_t latency = 0;
    GetLatency(latency);
    float speed = GetSpeed(syncCenter->GetPlaybackRate());
    auto diff = nowCt + (int64_t) latency - ct4Buffer + fixDelay_; // anhor diff
    auto diff2 = (nowCt - lastClockTime_) - static_cast<int64_t>((buffer->pts_ - lastPts_) / speed); // video diff
    auto diff3 = diff2 - PER_SINK_TIME_THRESHOLD; // video diff with PER_SINK_TIME_THRESHOLD
    if (discardFrameCnt_ + renderFrameCnt_ < VIDEO_SINK_START_FRAME) {
        diff = (nowCt - firstFrameNowct_) - (buffer->pts_ - firstFramePts_);
        MEDIA_LOG_I_SHORT("VideoSink first few times diff is " PUBLIC_LOG_D64 " us", diff);
    } else if (diff < 0 && diff2 < SINK_TIME_US_THRESHOLD && diff < diff3) { // per frame render time reduced by 33ms
        diff = diff3;
    }
    MEDIA_LOG_D_SHORT("VS ct4Bf:" PUBLIC_LOG_D64 "diff:" PUBLIC_LOG_D64 "nowCt:" PUBLIC_LOG_D64, ct4Buffer, diff,
        nowCt);
    if (diff < 0) { // buffer is early, diff < 0 or 0 < diff < 40ms(25Hz) render it
        waitTimeUs = 0 - diff;
        MEDIA_LOG_D_SHORT("buffer is too early waitTimeUs: " PUBLIC_LOG_D64, waitTimeUs);
        if (waitTimeUs > WAIT_TIME_US_THRESHOLD) {
            waitTimeUs = WAIT_TIME_US_THRESHOLD;
        }
    } else if (diff > 0 && Plugins::HstTime2Ms(diff * HST_USECOND) > 40) { // > 40ms, buffer is late
        tooLate = true;
        MEDIA_LOG_D_SHORT("buffer is too late");
    }
    lastBufferTime_ = ct4Buffer;
    bool dropFlag = tooLate && ((buffer->flag_ & BUFFER_FLAG_KEY_FRAME) == 0); // buffer is too late, drop it
    return dropFlag ? -1 : waitTimeUs;
}

void VideoSink::SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter)
{
    MEDIA_LOG_D_SHORT("VideoSink::SetSyncCenter");
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
        MEDIA_LOG_I_SHORT("video DoSyncWrite set firstPts = " PUBLIC_LOG_D64, firstPts_);
    }
}

Status VideoSink::SetParameter(const std::shared_ptr<Meta>& meta)
{
    UpdateMediaTimeRange(meta);
    return Status::OK;
}

float VideoSink::GetSpeed(float speed)
{
    if (std::fabs(speed - 0) < 1e-9) {
        return 1.0f;
    }
    return speed;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
