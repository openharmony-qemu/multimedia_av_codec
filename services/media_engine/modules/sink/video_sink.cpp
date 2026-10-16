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
#include "common/event.h"

#include <cinttypes>

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_SYSTEM_PLAYER, "VideoSink" };
constexpr int64_t LAG_LIMIT_TIME = 100;
constexpr int32_t DROP_FRAME_CONTINUOUSLY_MAX_CNT = 2;
constexpr int32_t MAX_ADVANCE_US = 80000;
static const int64_t MS_PER_US = 1000; // us change to ms
constexpr int64_t STARTUP_FRAME_INTERVAL_FACTOR = 2;
}

namespace OHOS {
namespace Media {
namespace Pipeline {
int64_t GetVideoLatencyFixDelay()
{
    constexpr uint64_t defaultValue = 0;
    static uint64_t fixDelay = OHOS::system::GetUintParameter("debug.media_service.video_sync_fix_delay", defaultValue);
    MEDIA_LOG_I("video_sync_fix_delay, pid:%{public}d , fixdelay: %{public}" PRIu64, getprocpid(), fixDelay);
    return static_cast<int64_t>(fixDelay);
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

constexpr int64_t PTS_DISCONTINUE_THRESHOLD_US = 1000000;

VideoSink::VideoSink()
{
    refreshTime_ = 0;
    syncerPriority_ = IMediaSynchronizer::VIDEO_SINK;
    fixDelay_ = GetVideoLatencyFixDelay();
    enableRenderAtTime_ = system::GetParameter("debug.media_service.enable_renderattime", "1") == "1";
    renderAdvanceThreshold_ = static_cast<int64_t>
        (system::GetIntParameter("debug.media_service.renderattime_advance", MAX_ADVANCE_US));
    MEDIA_LOG_I("VideoSink ctor called...");
}

VideoSink::~VideoSink()
{
    MEDIA_LOG_I("VideoSink dtor called...");
    this->eventReceiver_ = nullptr;
}

void VideoSink::UpdateTimeAnchorIfNeeded(int64_t nowCt, int64_t waitTime,
    const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN(syncCenter != nullptr && buffer != nullptr);
    syncCenter->SetLastVideoBufferPts(buffer->pts_ - firstPts_.load());
    syncCenter->SetLastVideoBufferAbsPts(buffer->pts_);
    if (!needUpdateTimeAnchor_) {
        return;
    }
    uint64_t latency = 0;
    (void)GetLatency(latency);
    Pipeline::IMediaSyncCenter::IMediaTime iMediaTime
        = {buffer->pts_ - firstPts_.load(), buffer->pts_, buffer->duration_};
    syncCenter->UpdateTimeAnchor(nowCt + waitTime, latency, iMediaTime, this);
    needUpdateTimeAnchor_.store(false);
}

void VideoSink::UpdateTimeAnchorActually(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer, int64_t renderDelay)
{
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN(syncCenter != nullptr && buffer != nullptr);
    syncCenter->SetLastVideoBufferPts(buffer->pts_ - firstPts_.load());
    int64_t ct4Buffer = syncCenter->GetClockTimeNow() + (renderDelay > 0 ? renderDelay : 0);
    uint64_t latency = 0;
    (void)GetLatency(latency);
    Pipeline::IMediaSyncCenter::IMediaTime iMediaTime
        = {buffer->pts_ - firstPts_.load(), buffer->pts_, buffer->duration_};
    syncCenter->UpdateTimeAnchor(ct4Buffer, latency, iMediaTime, this);
}
void VideoSink::RecordStallingTimestamp(AVBuffer& buffer, int64_t stage, int64_t timeStamp)
{
    std::vector<int64_t> timeStampList;
    buffer.meta_->GetData(Tag::STALLING_TIMESTAMP, timeStampList);
    timeStampList.push_back(stage);
    timeStampList.push_back(timeStamp);
    buffer.meta_->SetData(Tag::STALLING_TIMESTAMP, timeStampList);
    MEDIA_LOG_D("VideoSink set stalling timestamp, stage:" PUBLIC_LOG_D64 ", timeStamp:" PUBLIC_LOG_D64,
        stage, timeStamp);
}

int64_t VideoSink::DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer, int64_t& actionClock)
{
    FALSE_RETURN_V(buffer != nullptr, 0);
    int64_t waitTime = 0;
    bool render = !isMuted_.load();
    auto syncCenter = syncCenter_.lock();
    
    UpdateSeekState(buffer);
    
    if ((buffer->flag_ & BUFFER_FLAG_EOS) == 0) {
        int64_t nowCt = syncCenter ? syncCenter->GetClockTimeNow() : 0;
        actionClock = nowCt;
        if (isFirstFrame_.load() && !needDropOnMute_.load()) {
            FALSE_RETURN_V(syncCenter != nullptr, false);
            isFirstFrame_.store(false);
            firstFrameClockTime_.store(nowCt);
            firstFramePts_.store(buffer->pts_);
        } else {
            waitTime = CheckBufferLatenessMayWait(buffer, nowCt);
        }
        UpdateTimeAnchorIfNeeded(nowCt, waitTime, buffer);
        lagDetector_.CalcLag(buffer);
        ReportPts(buffer->pts_);
        lastBufferRelativePts_.store(buffer->pts_ - firstPts_.load());
    } else {
        MEDIA_LOG_I("Video receives EOS.");
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
    int64_t lastPts = lastPts_.load();
    if (nowPts < lastPts) {
        MEDIA_LOG_W("video pts is not increasing, last pts: " PUBLIC_LOG_D64 ", current pts: " PUBLIC_LOG_D64,
            lastPts, nowPts);
    }
}

Status VideoSink::SetPerfRecEnabled(bool isPerfRecEnabled)
{
    isPerfRecEnabled_.store(isPerfRecEnabled);
    return Status::OK;
}

void VideoSink::PerfRecord(int64_t waitTime)
{
    FALSE_RETURN_NOLOG(isPerfRecEnabled_.load());
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "Report perf failed, callback is nullptr");
    FALSE_RETURN_NOLOG(perfRecorder_.Record(Plugins::Us2Ms(waitTime)) == PerfRecorder::FULL);
    eventReceiver_->OnDfxEvent({ "VSINK", DfxEventType::DFX_INFO_PERF_REPORT, perfRecorder_.GetMainPerfData() });
    perfRecorder_.Reset();
}

void VideoSink::ResetSyncInfo()
{
    int64_t lastPts = lastPts_.load();
    int64_t endPtsMs = (lastPts != HST_TIME_NONE) ? (lastPts / MS_PER_US) : 0;
    avSyncDetector_.OnPlaybackEnd(endPtsMs);
    ResetPrerollReported();
    renderFrameCnt_.store(0);
    dropFrameContinuouslyCnt_.store(0);
    isFirstFrame_.store(true);
    lastBufferRelativePts_.store(HST_TIME_NONE);
    lastBufferAnchoredClockTime_.store(HST_TIME_NONE);
    seekFlag_.store(false);
    lastPts_.store(HST_TIME_NONE);
    lastClockTime_.store(HST_TIME_NONE);
    needUpdateTimeAnchor_.store(true);
    lagDetector_.Reset();
    perfRecorder_.Reset();
    avSyncDetector_.Reset();
    lastPtsForDiscontinuity_.store(HST_TIME_NONE);
    isFirstFrameAfterSeek_.store(true);
}

Status VideoSink::GetLatency(uint64_t& nanoSec)
{
    nanoSec = 10; // 10 ns
    return Status::OK;
}

void VideoSink::SetSeekFlag()
{
    seekFlag_.store(true);
    lastPtsForDiscontinuity_.store(HST_TIME_NONE);
    isFirstFrameAfterSeek_.store(true);
}

void VideoSink::SetLastPts(int64_t lastPts, int64_t renderDelay)
{
    lastPts_.store(lastPts);
    auto syncCenter = syncCenter_.lock();
    if (syncCenter != nullptr) {
        lastClockTime_.store(syncCenter->GetClockTimeNow() + (renderDelay > 0 ? renderDelay : 0));
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
    auto videoDiff = (currentClockTime - lastClockTime_.load())
        - static_cast<int64_t>((buffer->pts_ - lastPts_.load()) / AdjustPlaybackRate(playbackRate));
    // render time per frame reduced by initialVideoWaitPeriod_
    auto thresholdAdjustedVideoDiff = videoDiff
        - static_cast<int64_t>(initialVideoWaitPeriod_ / STARTUP_FRAME_INTERVAL_FACTOR);

    auto diff = anchorDiff;
    if (discardFrameCnt_.load() + renderFrameCnt_.load() < VIDEO_SINK_START_FRAME) {
        float ptsDiffWithSpeed =
            static_cast<float>(buffer->pts_ - firstFramePts_.load()) / AdjustPlaybackRate(playbackRate);
        diff = (currentClockTime - firstFrameClockTime_.load()) - static_cast<int64_t>(ptsDiffWithSpeed);
        MEDIA_LOG_I("VideoSink first few times diff is " PUBLIC_LOG_D64 " us speed %{public}.3f", diff, playbackRate);
    } else if (diff < 0 && videoDiff < SINK_TIME_US_THRESHOLD && diff < thresholdAdjustedVideoDiff) {
        diff = thresholdAdjustedVideoDiff;
    }
    MEDIA_LOG_D("VS ct4Bf:" PUBLIC_LOG_D64 " diff:" PUBLIC_LOG_D64 " nowCt:" PUBLIC_LOG_D64,
        bufferAnchoredClockTime, diff, currentClockTime);
    return diff;
}

int64_t VideoSink::CheckBufferLatenessMayWait(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer, int64_t clockNow)
{
    InitWaitPeriod();
    auto syncCenter = syncCenter_.lock();
    FALSE_RETURN_V(buffer != nullptr, true);
    FALSE_RETURN_V(syncCenter != nullptr, true);

    auto relativePts = buffer->pts_ - firstPts_.load();
    auto bufferAnchoredClockTime = syncCenter->GetAnchoredClockTime(relativePts);
    MEDIA_LOG_D("VideoSink cur pts: " PUBLIC_LOG_D64 " us, bufferAnchoredClockTime: " PUBLIC_LOG_D64
        " us, buf_pts: " PUBLIC_LOG_D64 " us, fixDelay: " PUBLIC_LOG_D64 " us", relativePts,
        bufferAnchoredClockTime, buffer->pts_, fixDelay_);
    FALSE_RETURN_V(bufferAnchoredClockTime != Plugins::HST_TIME_NONE, 0);

    int64_t lastBufferAnchoredClockTime = lastBufferAnchoredClockTime_.load();
    if (lastBufferAnchoredClockTime != HST_TIME_NONE && seekFlag_.load() == false) {
        int64_t currentBufferRelativeClockTime =
            lastBufferAnchoredClockTime + relativePts - lastBufferRelativePts_.load();
        int64_t deltaTime = bufferAnchoredClockTime - currentBufferRelativeClockTime;
        deltaTimeAccu_ = SmoothDeltaTime(deltaTimeAccu_, deltaTime);
        if (std::abs(deltaTimeAccu_) < DELTA_TIME_THRESHOLD) {
            bufferAnchoredClockTime = currentBufferRelativeClockTime;
        }
        MEDIA_LOG_D("lastBfTime:" PUBLIC_LOG_D64" us, lastPts:" PUBLIC_LOG_D64,
            lastBufferAnchoredClockTime, lastBufferRelativePts_.load());
    } else {
        seekFlag_.store(false);
    }

    auto diff = CalcBufferDiff(buffer, bufferAnchoredClockTime,
        clockNow, syncCenter->GetPlaybackRate());
    int64_t lastPts = lastPts_.load();
    frameInterval_.store((lastPts < 0) ? -1 : (buffer->pts_ - lastPts));

    bool tooLate = false;
    int64_t waitTimeUs = 0;
    if (diff < 0) { // buffer is early, diff < 0 or 0 < diff < 40ms(25Hz) render it
        waitTimeUs = 0 - diff;
        RenderAtTimeLog(waitTimeUs);
        if (waitTimeUs > WAIT_TIME_US_THRESHOLD && !needDropOnMute_.load()) {
            int64_t ptsDiff = static_cast<int64_t>(buffer->pts_ - lastPts);
            if (ptsDiff > WAIT_TIME_US_THRESHOLD) {
                waitTimeUs = ptsDiff;
            } else {
                waitTimeUs = WAIT_TIME_US_THRESHOLD;
            }
        }
    } else if (diff > 0 && Plugins::HstTime2Ms(diff * HST_USECOND) > 40) { // > 40ms, buffer is late
        tooLate = true;
        MEDIA_LOG_D("buffer is too late");
    }
    lastBufferAnchoredClockTime_.store(bufferAnchoredClockTime);
    avSyncDetector_.CheckSync(diff, buffer->pts_);
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
    MEDIA_LOG_I("VideoSink::SetSyncCenter");
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
    avSyncDetector_.SetEventReceiver(receiver);
}

void VideoSink::SetFirstPts(int64_t pts)
{
    auto syncCenter = syncCenter_.lock();
    if (firstPts_.load() == HST_TIME_NONE) {
        if (syncCenter && syncCenter->GetMediaStartPts() != HST_TIME_NONE) {
            firstPts_.store(syncCenter->GetMediaStartPts());
        } else {
            firstPts_.store(pts);
        }
        MEDIA_LOG_I("video DoSyncWrite set firstPts = " PUBLIC_LOG_D64, firstPts_.load());
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

int64_t VideoSink::GetFrameInterval()
{
    return frameInterval_.load() / MS_PER_US;
}

bool VideoSink::VideoLagDetector::CalcLag(std::shared_ptr<AVBuffer> buffer)
{
    FALSE_RETURN_V(!(buffer->flag_ & (uint32_t)(Plugins::AVBufferFlag::EOS)), false);
    AutoLock lock(VideoLagMutex_);
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
    AutoLock lock(VideoLagMutex_);
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
    AutoLock lock(VideoLagMutex_);
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
        if (discardFrameCnt_.load() < VIDEO_SINK_START_FRAME) {
            discardFrameCnt_.store(VIDEO_SINK_START_FRAME);
        }
    }
    isMuted_.store(isMuted);
}

void VideoSink::AVSyncDetector::Reset()
{
    AutoLock lock(AVSyncMutex_);
    isInAVSync_ = false;
    syncStartPts_ = 0;
}

void VideoSink::AVSyncDetector::CheckSync(int64_t diffUs, int64_t pts)
{
    AutoLock lock(AVSyncMutex_);
    if (std::abs(diffUs) > AV_SYNC_THRESHOLD_US) {
        if (!isInAVSync_) {
            isInAVSync_ = true;
            if (syncStartPts_ <= syncEndPts_) {
                syncStartPts_ = pts;
            }
            syncCount_++;
            lipAsyncType_ = (diffUs < 0) ? VIDEO_AHEAD_FLAG : VIDEO_BEHIND_FLAG;
            lipAsyncStartPtsMs_ = pts / MS_PER_US;
            MEDIA_LOG_D("AVSync start, pts: " PUBLIC_LOG_D64 ", diff: " PUBLIC_LOG_D64 " us", pts, diffUs);
        } else if ((pts - syncStartPts_) > AV_SYNC_CUMULATIVE_US) {
            totalSyncDurationUs_ += pts - syncStartPts_;
            syncStartPts_ = pts;
        }
    } else {
        if (isInAVSync_) {
            int64_t duration = pts - syncStartPts_;
            if (duration > 0) {
                totalSyncDurationUs_ += duration;
                MEDIA_LOG_D("AVSync end, startPts: " PUBLIC_LOG_D64 ", endPts: " PUBLIC_LOG_D64
                    ", duration: " PUBLIC_LOG_D64 " us, count: " PUBLIC_LOG_D64 ", totalDuration: "
                    PUBLIC_LOG_D64 " us", syncStartPts_, pts, duration, syncCount_, totalSyncDurationUs_);
            }
            syncEndPts_ = pts;
            ReportLipAsyncEvent(pts / MS_PER_US);
            isInAVSync_ = false;
            syncStartPts_ = 0;
        }
    }
}

void VideoSink::AVSyncDetector::GetAVSyncInfo(int64_t& syncCount, int64_t& totalSyncDurationMs)
{
    AutoLock lock(AVSyncMutex_);
    syncCount = syncCount_;
    totalSyncDurationMs = totalSyncDurationUs_ / MS_PER_US;
}

void VideoSink::AVSyncDetector::ReportLipAsyncEvent(int64_t endPtsMs)
{
    if (eventReceiver_ == nullptr) {
        return;
    }
    LipAsyncInfo lipAsyncInfo {
        .asyncType = lipAsyncType_,
        .startTimeMs = lipAsyncStartPtsMs_,
        .endTimeMs = endPtsMs
    };
    MEDIA_LOG_I("LipAsync event, type: " PUBLIC_LOG_D32 ", start: " PUBLIC_LOG_D64
        " ms, end: " PUBLIC_LOG_D64 " ms", lipAsyncType_, lipAsyncStartPtsMs_, endPtsMs);
    eventReceiver_->OnDfxEvent({"VideoSink", DfxEventType::DFX_EVENT_AVDESYNC, lipAsyncInfo});
}

void VideoSink::AVSyncDetector::OnPlaybackEnd(int64_t endPtsMs)
{
    AutoLock lock(AVSyncMutex_);
    if (isInAVSync_ && eventReceiver_ != nullptr) {
        MEDIA_LOG_I("LipAsync event on playback end, type: " PUBLIC_LOG_D32
            ", start: " PUBLIC_LOG_D64 " ms, end: " PUBLIC_LOG_D64 " ms",
            lipAsyncType_, lipAsyncStartPtsMs_, endPtsMs);
        ReportLipAsyncEvent(endPtsMs);
    }
}

void VideoSink::AVSyncDetector::SetEventReceiver(const std::shared_ptr<EventReceiver> eventReceiver)
{
    eventReceiver_ = eventReceiver;
}

Status VideoSink::GetAVSyncInfo(int64_t& syncCount, int64_t& totalSyncDurationMs)
{
    avSyncDetector_.GetAVSyncInfo(syncCount, totalSyncDurationMs);
    return Status::OK;
}

void VideoSink::UpdateSeekState(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    if (seekFlag_.load()) {
        seekFlag_.store(false);
        isFirstFrameAfterSeek_.store(true);
    } else if (isFirstFrameAfterSeek_) {
        isFirstFrameAfterSeek_.store(false);
    } else {
        CheckPtsDiscontinuity(buffer);
    }
}

void VideoSink::CheckPtsDiscontinuity(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer)
{
    FALSE_RETURN_NOLOG(eventReceiver_ != nullptr);
    FALSE_RETURN_NOLOG(buffer != nullptr);
    FALSE_RETURN_NOLOG(!(buffer->flag_ & static_cast<uint32_t>(AVBufferFlag::EOS)));
    
    int64_t curPts = buffer->pts_;
    int64_t lastPts = lastPtsForDiscontinuity_.load();
    lastPtsForDiscontinuity_.store(curPts);
    
    int64_t ptsDiff = curPts > lastPts ? (curPts - lastPts) : (lastPts - curPts);
    if (ptsDiff > PTS_DISCONTINUE_THRESHOLD_US && lastPts != HST_TIME_NONE) {
        MEDIA_LOG_I("Video PTS discontinuity detected, lastPts " PUBLIC_LOG_D64
            ", curPts " PUBLIC_LOG_D64, lastPts, curPts);
        MediaDiscontinueInfo discontinueInfo {
            .streamType = 2,
            .discontinueType = 2,
            .ptsBefore = lastPts,
            .ptsAfter = curPts
        };
        eventReceiver_->OnDfxEvent({"VideoSink", DfxEventType::DFX_EVENT_MEDIA_DISCONTINUE, discontinueInfo});
    }
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
