/*
* Copyright (c) 2023-2025 Huawei Device Co., Ltd.
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
constexpr int64_t LAG_LIMIT_TIME = 100;
constexpr int32_t DROP_FRAME_CONTINUOUSLY_MAX_CNT = 2;
constexpr int32_t MAX_ADVANCE_US = 80000;
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

constexpr int64_t WAIT_TIME_US_THRESHOLD = 1500000; // max sleep time 1.5s

constexpr int64_t SINK_TIME_US_THRESHOLD = 100000; // max sink time 100ms

constexpr int64_t PER_SINK_TIME_THRESHOLD_MAX = 33000; // max per sink time 33ms(30Hz)

constexpr int64_t PER_SINK_TIME_THRESHOLD_MIN = 8333; // min per sink time 8.33ms(120Hz)

// Video Sync Start Frame
constexpr int VIDEO_SINK_START_FRAME = 4;

constexpr int64_t WAIT_TIME_US_THRESHOLD_WARNING = 40000; // warning threshold 40ms

constexpr int64_t WAIT_TIME_US_THRESHOLD_RENDER = 70000; // warning threshold render 70ms

constexpr int64_t DELTA_TIME_THRESHOLD = 5000; // max delta time 5ms

constexpr int64_t US_PER_SECOND = 1000000; // 1000 * 1000 us per second

VideoSink::VideoSink()
{
    refreshTime_ = 0;
    syncerPriority_ = IMediaSynchronizer::VIDEO_SINK;
    fixDelay_ = GetvideoLatencyFixDelay();
    enableRenderAtTime_ = system::GetParameter("debug.media_service.enable_renderattime", "1") == "1";
    renderAdvanceThreshold_ = static_cast<int64_t>
        (system::GetIntParameter("debug.media_service.renderattime_advance", MAX_ADVANCE_US));
    MEDIA_LOG_I_SHORT("VideoSink ctor called...");
}

VideoSink::~VideoSink()
{
    MEDIA_LOG_I_SHORT("dtor");
    this->eventReceiver_ = nullptr;
}

void VideoSink::UpdateTimeAnchorIfNeeded(int64_t nowCt, int64_t waitTime,
    const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN(syncCenter != nullptr && buffer != nullptr);
    syncCenter->SetLastVideoBufferPts(buffer->pts_ - firstPts_);
    syncCenter->SetLastVideoBufferAbsPts(buffer->pts_);
    if (!needUpdateTimeAnchor_) {
        return;
    }
    uint64_t latency = 0;
    (void)GetLatency(latency);
    Pipeline::IMediaSyncCenter::IMediaTime iMediaTime = {buffer->pts_ - firstPts_, buffer->pts_, buffer->duration_};
    syncCenter->UpdateTimeAnchor(nowCt + waitTime, latency, iMediaTime, this);
    needUpdateTimeAnchor_ = false;
}

void VideoSink::UpdateTimeAnchorActually(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer, int64_t renderDelay)
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN(syncCenter != nullptr && buffer != nullptr);
    syncCenter->SetLastVideoBufferPts(buffer->pts_ - firstPts_);
    int64_t ct4Buffer = syncCenter->GetClockTimeNow() + (renderDelay > 0 ? renderDelay : 0);
    uint64_t latency = 0;
    (void)GetLatency(latency);
    Pipeline::IMediaSyncCenter::IMediaTime iMediaTime = {buffer->pts_ - firstPts_, buffer->pts_, buffer->duration_};
    syncCenter->UpdateTimeAnchor(ct4Buffer, latency, iMediaTime, this);
}

int64_t VideoSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_V(buffer != nullptr, 0);
    int64_t waitTime = 0;
    bool render = !isMuted_;
    auto syncCenter = syncCenter_.lock();
    if ((buffer->flag_ & BUFFER_FLAG_EOS) == 0) {
        int64_t nowCt = syncCenter ? syncCenter->GetClockTimeNow() : 0;
        if (isFirstFrame_ && !needDropOnMute_) {
            FALSE_RETURN_V(syncCenter != nullptr, false);
            isFirstFrame_ = false;
            firstFrameClockTime_  = nowCt;
            firstFramePts_ = buffer->pts_;
        } else {
            waitTime = CheckBufferLatenessMayWait(buffer);
        }
        UpdateTimeAnchorIfNeeded(nowCt, waitTime, buffer);
        lagDetector_.CalcLag(buffer);
        ReportPts(buffer->pts_);
        lastBufferRelativePts_ = buffer->pts_ - firstPts_;
    } else {
        MEDIA_LOG_I_SHORT("Video sink EOS");
        if (syncCenter) {
            syncCenter->ReportEos(this);
        }
        return -1;
    }
    if ((render && waitTime >= 0) || dropFrameContinuouslyCnt_.load() >= DROP_FRAME_CONTINUOUSLY_MAX_CNT) {
        dropFrameContinuouslyCnt_.store(0);
        needDropOnMute_.store(false);
        renderFrameCnt_++;
        MEDIA_LOG_D("VideoSink::DoSyncWrite waitTime is " PUBLIC_LOG_D64, waitTime);
        return waitTime > 0 ? waitTime : 0;
    }
    if (!needDropOnMute_.load()) {
        dropFrameContinuouslyCnt_.fetch_add(1);
    }
    discardFrameCnt_++;
    PerfRecord(waitTime);
    return -1;
}

void VideoSink::ReportPts(int64_t nowPts)
{
    if (nowPts < lastPts_) {
        MEDIA_LOG_W("video pts is not increasing, last pts: " PUBLIC_LOG_D64 ", current pts: " PUBLIC_LOG_D64,
            lastPts_, nowPts);
    }
}

Status VideoSink::SetPerfRecEnabled(bool isPerfRecEnabled)
{
    isPerfRecEnabled_ = isPerfRecEnabled;
    return Status::OK;
}

void VideoSink::PerfRecord(int64_t waitTime)
{
    FALSE_RETURN_NOLOG(isPerfRecEnabled_);
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "Report perf failed, callback is nullptr");
    FALSE_RETURN_NOLOG(perfRecorder_.Record(Plugins::Us2Ms(waitTime)) == PerfRecorder::FULL);
    eventReceiver_->OnDfxEvent({ "VSINK", DfxEventType::DFX_INFO_PERF_REPORT, perfRecorder_.GetMainPerfData() });
    perfRecorder_.Reset();
}

void VideoSink::ResetSyncInfo()
{
    ResetPrerollReported();
    renderFrameCnt_ = 0;
    dropFrameContinuouslyCnt_ = 0;
    isFirstFrame_ = true;
    lastBufferRelativePts_ = HST_TIME_NONE;
    lastBufferAnchoredClockTime_ = HST_TIME_NONE;
    seekFlag_ = false;
    lastPts_ = HST_TIME_NONE;
    lastClockTime_ = HST_TIME_NONE;
    needUpdateTimeAnchor_ = true;
    lagDetector_.Reset();
    perfRecorder_.Reset();
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

void VideoSink::SetLastPts(int64_t lastPts, int64_t renderDelay)
{
    lastPts_ = lastPts;
    auto syncCenter = syncCenter_.lock();
    if (syncCenter != nullptr) {
        lastClockTime_ = syncCenter->GetClockTimeNow() + (renderDelay > 0 ? renderDelay : 0);
    }
}

int64_t VideoSink::CalcBufferDiff(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer, int64_t bufferAnchoredClockTime,
    int64_t currentClockTime, float playbackRate)
{
    uint64_t latency = 0;
    GetLatency(latency);
    //  the diff between the current clock time and the buffer's
    //  anchored clock time, adjusted by latency and a fixed delay
    auto anchorDiff = currentClockTime + (int64_t) latency - bufferAnchoredClockTime + fixDelay_;
    //  the diff between the actual duration of the previous video frame and the theoretically calculated duration
    //  based on the PTS, considering the playback rate
    auto videoDiff = (currentClockTime - lastClockTime_)
        - static_cast<int64_t>((buffer->pts_ - lastPts_) / AdjustPlaybackRate(playbackRate));
    // render time per frame reduced by initialVideoWaitPeriod_
    auto thresholdAdjustedVideoDiff = videoDiff - initialVideoWaitPeriod_;

    auto diff = anchorDiff;
    if (discardFrameCnt_ + renderFrameCnt_ < VIDEO_SINK_START_FRAME) {
        float ptsDiffWithSpeed = static_cast<float>(buffer->pts_ - firstFramePts_) / AdjustPlaybackRate(playbackRate);
        diff = (currentClockTime - firstFrameClockTime_) - static_cast<int64_t>(ptsDiffWithSpeed);
        MEDIA_LOG_I("VideoSink first few times diff is " PUBLIC_LOG_D64 " us speed %{public}.3f", diff, playbackRate);
    } else if (diff < 0 && videoDiff < SINK_TIME_US_THRESHOLD && diff < thresholdAdjustedVideoDiff) {
        diff = thresholdAdjustedVideoDiff;
    }
    MEDIA_LOG_D("VS ct4Bf:" PUBLIC_LOG_D64 " diff:" PUBLIC_LOG_D64 " nowCt:" PUBLIC_LOG_D64,
        bufferAnchoredClockTime, diff, currentClockTime);
    return diff;
}

int64_t VideoSink::CheckBufferLatenessMayWait(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    InitWaitPeriod();
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(buffer != nullptr, true);
    FALSE_RETURN_V(syncCenter != nullptr, true);

    auto relativePts = buffer->pts_ - firstPts_;
    auto bufferAnchoredClockTime = syncCenter->GetAnchoredClockTime(relativePts);
    MEDIA_LOG_D("VideoSink cur pts: " PUBLIC_LOG_D64 " us, bufferAnchoredClockTime: " PUBLIC_LOG_D64
        " us, buf_pts: " PUBLIC_LOG_D64 " us, fixDelay: " PUBLIC_LOG_D64 " us", relativePts,
        bufferAnchoredClockTime, buffer->pts_, fixDelay_);
    FALSE_RETURN_V(bufferAnchoredClockTime != Plugins::HST_TIME_NONE, 0);

    if (lastBufferAnchoredClockTime_ != HST_TIME_NONE && seekFlag_ == false) {
        int64_t currentBufferRelativeClockTime = lastBufferAnchoredClockTime_ + relativePts - lastBufferRelativePts_;
        int64_t deltaTime = bufferAnchoredClockTime - currentBufferRelativeClockTime;
        deltaTimeAccu_ = SmoothDeltaTime(deltaTimeAccu_, deltaTime);
        if (std::abs(deltaTimeAccu_) < DELTA_TIME_THRESHOLD) {
            bufferAnchoredClockTime = currentBufferRelativeClockTime;
        }
        MEDIA_LOG_D("lastBfTime:" PUBLIC_LOG_D64" us, lastPts:" PUBLIC_LOG_D64,
            lastBufferAnchoredClockTime_, lastBufferRelativePts_);
    } else {
        seekFlag_ = (seekFlag_ == true) ? false : seekFlag_;
    }

    auto diff = CalcBufferDiff(buffer, bufferAnchoredClockTime,
        syncCenter->GetClockTimeNow(), syncCenter->GetPlaybackRate());

    bool tooLate = false;
    int64_t waitTimeUs = 0;
    if (diff < 0) { // buffer is early, diff < 0 or 0 < diff < 40ms(25Hz) render it
        waitTimeUs = 0 - diff;
        RenderAtTimeLog(waitTimeUs);
        if (waitTimeUs > WAIT_TIME_US_THRESHOLD && !needDropOnMute_.load()) {
            waitTimeUs = WAIT_TIME_US_THRESHOLD;
        }
    } else if (diff > 0 && Plugins::HstTime2Ms(diff * HST_USECOND) > 40) { // > 40ms, buffer is late
        tooLate = true;
        MEDIA_LOG_D_SHORT("buffer is too late");
    }
    lastBufferAnchoredClockTime_ = bufferAnchoredClockTime;
    bool dropFlag = tooLate && ((buffer->flag_ & BUFFER_FLAG_KEY_FRAME) == 0); // buffer is too late, drop it
    return dropFlag ? -1 : waitTimeUs;
}

void VideoSink::RenderAtTimeLog(int64_t waitTimeUs)
{
    if (enableRenderAtTime_) {
        MEDIA_LOG_I_FALSE_D((waitTimeUs >= WAIT_TIME_US_THRESHOLD_RENDER + renderAdvanceThreshold_),
            "enableRenderAtTime is true, buffer is too early waitTimeUs: " PUBLIC_LOG_D64, waitTimeUs);
    } else {
        MEDIA_LOG_I_FALSE_D((waitTimeUs >= WAIT_TIME_US_THRESHOLD_WARNING),
            "buffer is too early waitTimeUs: " PUBLIC_LOG_D64, waitTimeUs);
    }
}

void VideoSink::SetSyncCenter(std::shared_ptr<Pipeline::MediaSyncManager> syncCenter)
{
    MEDIA_LOG_D_SHORT("VideoSink::SetSyncCenter");
    syncCenter_ = syncCenter;
    MediaSynchronousSink::Init();
}

void VideoSink::InitWaitPeriod()
{
    FALSE_RETURN_NOLOG(initialVideoWaitPeriod_ <= 0);
    initialVideoWaitPeriod_ = PER_SINK_TIME_THRESHOLD_MAX;
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_NOLOG(syncCenter != nullptr);
    double initialVideoFrameRate = syncCenter->GetInitialVideoFrameRate();
    MEDIA_LOG_I("VideoSink initialFrameRate is " PUBLIC_LOG_D64, static_cast<int64_t>(initialVideoFrameRate));
    FALSE_RETURN_NOLOG(initialVideoFrameRate > 1e-9); // frameRate > 0
    initialVideoWaitPeriod_ = static_cast<int64_t>(US_PER_SECOND
                                                   / (2 * initialVideoFrameRate)); // max delay 2 times frame interval
    MEDIA_LOG_I("VideoSink intial initialVideoWaitPerild_ is >> " PUBLIC_LOG_D64, initialVideoWaitPeriod_);
    initialVideoWaitPeriod_ = std::min(initialVideoWaitPeriod_, PER_SINK_TIME_THRESHOLD_MAX);
    initialVideoWaitPeriod_ = std::max(initialVideoWaitPeriod_, PER_SINK_TIME_THRESHOLD_MIN);
    MEDIA_LOG_I("VideoSink final initialVideoWaitPerild_ is << " PUBLIC_LOG_D64, initialVideoWaitPeriod_);
}

void VideoSink::SetEventReceiver(const std::shared_ptr<EventReceiver> &receiver)
{
    this->eventReceiver_ = receiver;
    lagDetector_.SetEventReceiver(receiver);
}

void VideoSink::SetFirstPts(int64_t pts)
{
    auto syncCenter = syncCenter_.lock();
    if (firstPts_ == HST_TIME_NONE) {
        if (syncCenter && syncCenter->GetMediaStartPts() != HST_TIME_NONE) {
            firstPts_ = syncCenter->GetMediaStartPts();
        } else {
            firstPts_ = pts;
        }
        MEDIA_LOG_I("video DoSyncWrite set firstPts = " PUBLIC_LOG_D64, firstPts_);
    }
}

Status VideoSink::SetParameter(const std::shared_ptr<Meta>& meta)
{
    UpdateMediaTimeRange(meta);
    return Status::OK;
}

float VideoSink::AdjustPlaybackRate(float playbackRate)
{
    static constexpr float MIN_PLAYBACK_RATE = 1e-9;
    if (std::fabs(playbackRate) < MIN_PLAYBACK_RATE) {
        return 1.0f;
    }
    return playbackRate;
}

int64_t VideoSink::SmoothDeltaTime(int64_t accumulatedDeltaTime, int64_t currentDeltaTime)
{
    static constexpr int64_t SMOOTHING_FACTOR = 9;
    return (accumulatedDeltaTime * SMOOTHING_FACTOR + currentDeltaTime) / (SMOOTHING_FACTOR + 1);
}

Status VideoSink::GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration)
{
    lagDetector_.GetLagInfo(lagTimes, maxLagDuration, avgLagDuration);
    return Status::OK;
}

bool VideoSink::VideoLagDetector::CalcLag(std::shared_ptr<AVBuffer> buffer)
{
    FALSE_RETURN_V(!(buffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS)), false);
    auto systemTimeMsNow = Plugins::GetCurrentMillisecond();
    auto systemTimeMsDiff = systemTimeMsNow - lastSystemTimeMs_;
    auto bufferTimeUsNow = Plugins::Us2Ms(buffer->pts_);
    auto bufferTimeMsDiff = bufferTimeUsNow - lastBufferTimeMs_;
    auto lagTimeMs = systemTimeMsDiff - bufferTimeMsDiff;
    bool isVideoLag = lastSystemTimeMs_ > 0 && lagTimeMs >= LAG_LIMIT_TIME;
    MEDIA_LOG_I_FALSE_D(isVideoLag,
        "prePts " PUBLIC_LOG_D64 " curPts " PUBLIC_LOG_D64 " ptsDiff " PUBLIC_LOG_D64 " tDiff " PUBLIC_LOG_D64,
        lastBufferTimeMs_, bufferTimeUsNow, bufferTimeMsDiff, systemTimeMsDiff);
    lastSystemTimeMs_ = systemTimeMsNow;
    lastBufferTimeMs_ = bufferTimeUsNow;
    if (isVideoLag) {
        ResolveLagEvent(lagTimeMs);
    }
    return isVideoLag;
}

void VideoSink::VideoLagDetector::SetEventReceiver(const std::shared_ptr<EventReceiver> eventReceiver)
{
    eventReceiver_ = eventReceiver;
}

void VideoSink::VideoLagDetector::ResolveLagEvent(const int64_t &lagTimeMs)
{
    lagTimes_++;
    maxLagDuration_ = std::max(maxLagDuration_, lagTimeMs);
    totalLagDuration_ += lagTimeMs;
    FALSE_RETURN(eventReceiver_ != nullptr);
    eventReceiver_->OnDfxEvent({"VideoSink", DfxEventType::DFX_INFO_PLAYER_VIDEO_LAG, lagTimeMs});
}

void VideoSink::VideoLagDetector::GetLagInfo(int32_t& lagTimes, int32_t& maxLagDuration, int32_t& avgLagDuration)
{
    lagTimes = lagTimes_;
    maxLagDuration = static_cast<int32_t>(maxLagDuration_);
    if (lagTimes_ != 0) {
        avgLagDuration = static_cast<int32_t>(totalLagDuration_ / lagTimes_);
    } else {
        avgLagDuration = 0;
    }
}

void VideoSink::VideoLagDetector::Reset()
{
    lagTimes_ = 0;
    maxLagDuration_ = 0;
    lastSystemTimeMs_ = 0;
    lastBufferTimeMs_ = 0;
    totalLagDuration_ = 0;
}

void VideoSink::SetMediaMuted(bool isMuted)
{
    if (isMuted) {
        needDropOnMute_.store(true);
        dropFrameContinuouslyCnt_.store(0);
    }
    isMuted_ = isMuted;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
