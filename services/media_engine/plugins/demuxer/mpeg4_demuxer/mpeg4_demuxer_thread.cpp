/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#define MEDIA_PLUGIN
#define HST_LOG_TAG "MPEG4DemuxerThread"
#include <chrono>
#include <memory>
#include <string>
#include <vector>
#include "avcodec_trace.h"
#include "common/log.h"
#include "meta/format.h"
#include "meta/video_types.h"
#include "mpeg4_demuxer_plugin.h"
#include "mpeg4_sample_memory.h"
#include "qos.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4DemuxerThread" };
} // namespace

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {
namespace {
bool HaveValidParserForThread(const std::string& mime)
{
    return mime == "video/avc" || mime == "video/hevc" || mime == "video/vvc";
}

int32_t GetSampleSizeWithCodecConfigForThread(const MediaInfo& mediaInfo,
    const std::shared_ptr<Sample>& sample, uint32_t trackId)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr, 0, "Sample is nullptr");
    int32_t sampleSize = sample->size;
    std::string mime;
    std::vector<uint8_t> extradata {};
    if (mediaInfo.tracks[trackId].Get<Tag::MIME_TYPE>(mime) && HaveValidParserForThread(mime) &&
        mediaInfo.tracks[trackId].Get<Tag::MEDIA_CODEC_CONFIG>(extradata) &&
        (sample->flag & Sample::SampleFlag::SYNC_FRAME)) {
        sampleSize += static_cast<int32_t>(extradata.size());
    }
    return sampleSize;
}
} // namespace

Status MPEG4DemuxerPlugin::SetEosSample(std::shared_ptr<AVBuffer> outBuffer, bool replaceMemory)
{
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_INVALID_PARAMETER, "Eos buffer is nullptr");
    if (!replaceMemory) {
        FALSE_RETURN_V_MSG_E(outBuffer->memory_ != nullptr, Status::ERROR_INVALID_PARAMETER,
            "Eos memory is nullptr");
    }

    outBuffer->pts_ = 0;
    outBuffer->dts_ = 0;
    outBuffer->duration_ = 0;
    outBuffer->flag_ = static_cast<uint32_t>(AVBufferFlag::EOS);
    if (replaceMemory) {
        auto eosSample = std::make_shared<Sample>();
        FALSE_RETURN_V_MSG_E(eosSample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate eos sample");
        eosSample->size = 1;
        eosSample->data = std::make_unique<uint8_t[]>(1);
        FALSE_RETURN_V_MSG_E(eosSample->data != nullptr, Status::ERROR_NO_MEMORY,
            "Failed to allocate eos sample data");
        auto sampleMemory = std::make_shared<MPEG4SampleMemory>(eosSample);
        FALSE_RETURN_V_MSG_E(sampleMemory != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate eos memory");
        outBuffer->memory_ = std::static_pointer_cast<AVMemory>(sampleMemory);
    }
    MEDIA_LOG_D("Read to end");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::WriteSampleMemory(uint32_t trackIndex, std::shared_ptr<AVBuffer> outBuffer,
    std::shared_ptr<MPEG4Sample> mpeg4Sample)
{
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_NULL_POINTER, "Buffer is nullptr");
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr, Status::ERROR_NULL_POINTER,
        "Sample is nullptr");

    outBuffer->pts_ = mpeg4Sample->sample->pts;
    outBuffer->dts_ = mpeg4Sample->sample->dts;
    outBuffer->meta_->Set<Tag::BUFFER_DECODING_TIMESTAMP>(mpeg4Sample->sample->dts);
    outBuffer->duration_ = mpeg4Sample->sample->duration;
    outBuffer->meta_->Set<Tag::BUFFER_DURATION>(mpeg4Sample->sample->duration);
    MEDIA_LOG_D("CurrentBuffer: [" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "/" PUBLIC_LOG_D64 "], track: "
        PUBLIC_LOG_U32, outBuffer->pts_, outBuffer->dts_, outBuffer->duration_, trackIndex);

    CheckResetXPSSendStatus(trackIndex, mpeg4Sample->sample);
    if (cacheQueue_.ResetInfo(mpeg4Sample) == false) {
        MEDIA_LOG_D("Reset info failed");
    }
    Status ret = ConvertSampleToAnnexb(mpeg4Sample, trackIndex);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Convert annexb failed");
    if (cacheQueue_.SetInfo(mpeg4Sample) == false) {
        MEDIA_LOG_D("Set info failed");
    }

    outBuffer->flag_ = GetFlags(mpeg4Sample->sample, false);
    auto sampleMemory = std::make_shared<MPEG4SampleMemory>(mpeg4Sample->sample, mpeg4Sample->offset);
    FALSE_RETURN_V_MSG_E(sampleMemory != nullptr, Status::ERROR_NO_MEMORY, "Create sample memory failed");
    outBuffer->memory_ = std::static_pointer_cast<AVMemory>(sampleMemory);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ReadSample(uint32_t trackId, std::shared_ptr<AVBuffer> outBuffer, uint32_t timeout)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace(std::string("ReadSample_timeout_") + std::to_string(trackId));
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr && outBuffer->memory_ != nullptr, Status::ERROR_INVALID_PARAMETER,
        "AVBuffer or memory is nullptr");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::SYNC)) == 0,
        Status::ERROR_INVALID_OPERATION, "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::ASYNC), std::memory_order_acq_rel);

    isPauseReadPacket_.store(false, std::memory_order_release);
    trackId_.store(trackId, std::memory_order_release);
    if (!cacheQueue_.HasCache(trackId) && timeout == 0) {
        return Status::ERROR_WAIT_TIMEOUT;
    }
    if (!readThread_) {
        stopReadThread_.store(false, std::memory_order_release);
        readLoopStatus_.store(Status::OK, std::memory_order_release);
        readThread_ = std::make_unique<std::thread>(&MPEG4DemuxerPlugin::MPEG4ReadLoop, this);
    }

    Status ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Frame reading thread error, ret = " PUBLIC_LOG_D32, ret);

    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto mpeg4Sample = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr, Status::ERROR_NULL_POINTER,
        "Cache sample is nullptr");

    if ((mpeg4Sample->sample->flag & Sample::SampleFlag::EOS) != 0) {
        ret = SetEosSample(outBuffer, false);
        if (ret == Status::OK) {
            cacheQueue_.Pop(trackId);
            cacheSamples_.erase(trackId);
        }
        return ret;
    }

    ret = WriteBuffer(trackId, outBuffer, mpeg4Sample);
    if (ret == Status::ERROR_NOT_ENOUGH_DATA) {
        return Status::OK;
    } else if (ret == Status::OK) {
        cacheQueue_.Pop(trackId);
        cacheSamples_.erase(trackId);
    }
    return ret;
}

Status MPEG4DemuxerPlugin::GetNextSampleSize(uint32_t trackId, int32_t &sampleSize, uint32_t timeout)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace(std::string("GetNextSampleSize_timeout_") + std::to_string(trackId));
    MEDIA_LOG_D("In, track " PUBLIC_LOG_D32, trackId);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_UNKNOWN, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::SYNC)) == 0,
        Status::ERROR_INVALID_OPERATION, "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::ASYNC), std::memory_order_acq_rel);

    isPauseReadPacket_.store(false, std::memory_order_release);
    trackId_.store(trackId, std::memory_order_release);
    if (!cacheQueue_.HasCache(trackId) && timeout == 0) {
        return Status::ERROR_WAIT_TIMEOUT;
    }
    if (!readThread_) {
        stopReadThread_.store(false, std::memory_order_release);
        readLoopStatus_.store(Status::OK, std::memory_order_release);
        readThread_ = std::make_unique<std::thread>(&MPEG4DemuxerPlugin::MPEG4ReadLoop, this);
    }

    Status ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Frame reading thread error, ret = " PUBLIC_LOG_D32, ret);

    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto mpeg4Sample = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr, Status::ERROR_NULL_POINTER,
        "Cache sample is nullptr");
    if ((mpeg4Sample->sample->flag & Sample::SampleFlag::EOS) != 0) {
        return Status::END_OF_STREAM;
    }
    sampleSize = GetSampleSizeWithCodecConfigForThread(mediaInfo_, mpeg4Sample->sample, trackId);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::Pause()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    isPauseReadPacket_.store(true, std::memory_order_release);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetLastPTSByTrackId(uint32_t trackId, int64_t &lastPTS)
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    lastPTS = INT64_MIN;
    return cacheQueue_.GetLastPTSByTrackId(trackId, lastPTS);
}

bool MPEG4DemuxerPlugin::ShouldWaitForRead(uint32_t trackId)
{
    return !cacheQueue_.HasCache(trackId);
}

Status MPEG4DemuxerPlugin::WaitForLoop(uint32_t trackId, uint32_t timeout)
{
    FALSE_RETURN_V_MSG_E(readThread_ != nullptr, Status::ERROR_UNKNOWN, "Read thread is nullptr");
    if (ShouldWaitForRead(trackId)) {
        isWaitingForReadThread_.store(true, std::memory_order_release);
        if (threadState_.load(std::memory_order_acquire) == ThreadState::WAITING) {
            std::lock_guard<std::mutex> readLock(readThreadMutex_);
            threadReady_.store(true, std::memory_order_release);
            readLoopCv_.notify_one();
        }
        {
            std::unique_lock<std::mutex> readLock(readSampleMutex_);
            if (!readCacheCv_.wait_for(readLock, std::chrono::milliseconds(timeout), [this, trackId] {
                    return !ShouldWaitForRead(trackId) || readLoopStatus_.load(std::memory_order_acquire) != Status::OK;
                })) {
                isWaitingForReadThread_.store(false, std::memory_order_release);
                FALSE_RETURN_V_MSG_E(readLoopStatus_.load(std::memory_order_acquire) == Status::OK,
                    readLoopStatus_.load(std::memory_order_acquire), "read thread abnormal end");
                return Status::ERROR_WAIT_TIMEOUT;
            }
        }
    }
    isWaitingForReadThread_.store(false, std::memory_order_release);
    if (ShouldWaitForRead(trackId) && readLoopStatus_.load(std::memory_order_acquire) != Status::OK) {
        return readLoopStatus_.load(std::memory_order_acquire);
    }
    return Status::OK;
}

bool MPEG4DemuxerPlugin::NeedWaitForRead(uint32_t trackId)
{
    return (cacheQueue_.HasCache(trackId) || isPauseReadPacket_.load(std::memory_order_acquire)) &&
        !stopReadThread_.load(std::memory_order_acquire);
}

bool MPEG4DemuxerPlugin::IsWebVTTTrack(const std::shared_ptr<MPEG4AtomParser::Track>& track) const
{
    if (track == nullptr || track->sampleHelper == nullptr) {
        return false;
    }
    std::string metaMime;
    mediaInfo_.tracks[track->trackIndex].Get<Tag::MIME_TYPE>(metaMime);
    return track->sampleHelper->mimeType_ == MimeType::TEXT_WEBVTT || metaMime == MimeType::TEXT_WEBVTT;
}

void MPEG4DemuxerPlugin::AdvanceSampleIndex(const std::shared_ptr<MPEG4AtomParser::Track>& track)
{
    if (seekable_ == Seekable::SEEKABLE && track != nullptr) {
        ++track->currentSampleIndex;
    }
}

static void UpdateWebVTTDuration(const std::shared_ptr<Sample>& sample, int64_t endPts)
{
    FALSE_RETURN(sample != nullptr);
    if (endPts > sample->pts) {
        sample->duration = endPts - sample->pts;
    }
}

static bool GetRoundedDurationUs(int64_t durationUs, int32_t trackTimeScale, int64_t& roundedDurationUs)
{
    FALSE_RETURN_V(durationUs > 0 && trackTimeScale > 0, false);

    constexpr int64_t halfDivisor = 2;
    int64_t durationTicks = 0;
    int64_t temp = 0;
    int64_t usPerSecond = static_cast<int64_t>(S_TO_US);
    if (__builtin_mul_overflow(durationUs, static_cast<int64_t>(trackTimeScale), &temp)) {
        return false;
    }
    durationTicks = temp / usPerSecond;
    if (temp % usPerSecond >= usPerSecond / halfDivisor) {
        ++durationTicks;
    }
    return durationTicks > 0 &&
        ConvertTimeScale(durationTicks, usPerSecond, trackTimeScale, &roundedDurationUs) == Status::OK;
}

int64_t MPEG4DemuxerPlugin::GetWebVTTFlushEndPtsUs(
    const std::shared_ptr<MPEG4AtomParser::Track>& track, const std::shared_ptr<Sample>& sample) const
{
    int32_t trackTimeScale = 0;
    if (sample != nullptr && sample->duration > 0) {
        int64_t sampleEndPts = 0;
        if (!__builtin_add_overflow(sample->pts, sample->duration, &sampleEndPts)) {
            return sampleEndPts;
        }
    }

    int64_t trackDuration = 0;
    if (track != nullptr && mediaInfo_.tracks[track->trackIndex].Get<Tag::MEDIA_TIME_SCALE>(trackTimeScale) &&
        trackTimeScale > 0 &&
        ConvertTimeScale(track->duration, static_cast<int64_t>(S_TO_US), trackTimeScale, &trackDuration) ==
        Status::OK) {
        return trackDuration;
    }

    int64_t fileDuration = 0;
    int64_t roundedFileDuration = 0;
    if (track != nullptr && trackTimeScale > 0 && mediaInfo_.general.Get<Tag::MEDIA_DURATION>(fileDuration) &&
        GetRoundedDurationUs(fileDuration, trackTimeScale, roundedFileDuration)) {
        return roundedFileDuration;
    }
    return 0;
}

Status MPEG4DemuxerPlugin::ProcessWebVTTSampleForCache(
    const std::shared_ptr<MPEG4AtomParser::Track>& track, std::shared_ptr<MPEG4Sample> mpeg4Sample)
{
    FALSE_RETURN_V_MSG_E(track != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr,
        Status::ERROR_NULL_POINTER, "Sample is nullptr");

    uint32_t trackId = track->trackIndex;
    Status ret = ProcessWebVTTSample(mpeg4Sample);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ProcessWebVTTSample failed");
    AdvanceSampleIndex(track);

    auto pending = pendingWebVttSamples_.find(trackId);
    if (mpeg4Sample->isVttc) {
        if (pending != pendingWebVttSamples_.end()) {
            UpdateWebVTTDuration(pending->second, mpeg4Sample->sample->pts);
            ret = AddSampleToCacheQueue(pending->second, trackId);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add pending WebVTT cache failed");
        }
        pendingWebVttSamples_[trackId] = mpeg4Sample->sample;
        return Status::OK;
    }

    if (pending == pendingWebVttSamples_.end()) {
        return Status::OK;
    }
    UpdateWebVTTDuration(pending->second, mpeg4Sample->sample->pts);
    ret = AddSampleToCacheQueue(pending->second, trackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add WebVTT cache failed");
    pendingWebVttSamples_.erase(pending);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FlushPendingWebVTTSample(uint32_t trackId)
{
    auto iter = pendingWebVttSamples_.find(trackId);
    if (iter == pendingWebVttSamples_.end()) {
        return Status::OK;
    }
    auto track = FindTrackById(static_cast<int32_t>(trackId));
    int64_t endPts = GetWebVTTFlushEndPtsUs(track, iter->second);
    if (endPts > 0) {
        UpdateWebVTTDuration(iter->second, endPts);
    }
    Status ret = AddSampleToCacheQueue(iter->second, trackId);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add pending WebVTT cache failed");
    pendingWebVttSamples_.erase(iter);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FlushPendingWebVTTSamples()
{
    while (!pendingWebVttSamples_.empty()) {
        Status ret = FlushPendingWebVTTSample(pendingWebVttSamples_.begin()->first);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Flush pending WebVTT sample failed");
    }
    return Status::OK;
}

void MPEG4DemuxerPlugin::HandleReadWait()
{
    std::unique_lock<std::mutex> readLock(readThreadMutex_);
    {
        std::unique_lock<std::mutex> seekWaitLock(seekWaitMutex_);
        threadState_.store(ThreadState::WAITING, std::memory_order_release);
        seekWaitCv_.notify_one();
    }
    readLoopCv_.wait(readLock, [this]() {
        auto trackId = trackId_.load(std::memory_order_acquire);
        return threadReady_.load(std::memory_order_acquire) ||
            stopReadThread_.load(std::memory_order_acquire) ||
            (!cacheQueue_.HasCache(trackId) && !isPauseReadPacket_.load(std::memory_order_acquire)) ||
            isWaitingForReadThread_.load(std::memory_order_acquire);
    });
    threadState_.store(ThreadState::READING, std::memory_order_release);
    threadReady_.store(false, std::memory_order_release);
}

bool MPEG4DemuxerPlugin::ReadAndCacheSample()
{
    auto trackId = trackId_.load(std::memory_order_acquire);
    readLoopStatus_.store(Status::OK, std::memory_order_release);
    Status ret = ReadSampleToCache(trackId);
    if (ret == Status::ERROR_AGAIN) {
        MEDIA_LOG_D("Read sample again, keep async read thread alive");
        readLoopStatus_.store(Status::OK, std::memory_order_release);
        return true;
    }
    if (ret == Status::END_OF_STREAM) {
        readToEnd_ = true;
        ret = PushEosToCache(trackId);
        if (ret != Status::OK) {
            readLoopStatus_.store(ret, std::memory_order_release);
            return false;
        }
        readLoopStatus_.store(Status::END_OF_STREAM, std::memory_order_release);
        std::lock_guard<std::mutex> readLock(readSampleMutex_);
        readCacheCv_.notify_all();
        return true;
    }
    if (ret != Status::OK) {
        readLoopStatus_.store(ret, std::memory_order_release);
        return false;
    }
    std::lock_guard<std::mutex> readLock(readSampleMutex_);
    readCacheCv_.notify_all();
    return true;
}

Status MPEG4DemuxerPlugin::ReadSampleToCache(uint32_t trackId)
{
    if (readToEnd_) {
        return Status::END_OF_STREAM;
    }
    while (!cacheQueue_.HasCache(trackId)) {
        uint32_t currentTrackId = trackId;
        int64_t pos = 0;
        int32_t size = 0;
        Status ret = GetUnseekableSampleInfo(currentTrackId, pos, size);
        FALSE_RETURN_V_MSG_E(ret == Status::OK || ret == Status::END_OF_STREAM, ret,
            "Get sample info failed");
        if (ret == Status::END_OF_STREAM) {
            ret = FlushPendingWebVTTSample(trackId);
            FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Flush pending WebVTT samples failed");
            return cacheQueue_.HasCache(trackId) ? Status::OK : Status::END_OF_STREAM;
        }

        auto currentTrack = FindTrackById(static_cast<int32_t>(currentTrackId));
        FALSE_RETURN_V_MSG_E(currentTrack != nullptr, Status::ERROR_NULL_POINTER, "Track is nullptr");
        auto sample = std::make_shared<Sample>();
        FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate sample");
        auto mpeg4Sample = std::make_shared<MPEG4Sample>();
        FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate MPEG4Sample");
        mpeg4Sample->sample = sample;
        {
            std::unique_lock<std::mutex> sLock(syncMutex_);
            ret = ReadSampleData(sample, pos, size, currentTrackId);
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "ReadSampleData failed");
        if (IsWebVTTTrack(currentTrack)) {
            ret = ProcessWebVTTSampleForCache(currentTrack, mpeg4Sample);
        } else {
            ret = AddSampleToCacheQueue(sample, currentTrackId);
            AdvanceSampleIndex(currentTrack);
        }
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add cache failed");
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::PushEosToCache(uint32_t trackId)
{
    (void)trackId;
    for (const auto& selectedTrackId : selectedTrackIds_) {
        auto backSample = cacheQueue_.Back(selectedTrackId);
        if (backSample != nullptr && backSample->sample != nullptr &&
            (backSample->sample->flag & Sample::SampleFlag::EOS) != 0) {
            continue;
        }
        auto eosSample = std::make_shared<Sample>();
        FALSE_RETURN_V_MSG_E(eosSample != nullptr, Status::ERROR_NO_MEMORY, "Failed to allocate eos sample");
        eosSample->pts = 0;
        eosSample->dts = 0;
        eosSample->duration = 0;
        eosSample->flag = Sample::SampleFlag::EOS;
        eosSample->size = 0;
        Status ret = AddSampleToCacheQueue(eosSample, selectedTrackId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Add eos cache failed");
    }
    return Status::OK;
}

void MPEG4DemuxerPlugin::MPEG4ReadLoop()
{
    MEDIA_LOG_I("Read loop start");
    if (isAsyncReadThreadPrioritySet_.load(std::memory_order_acquire)) {
        UpdateAsyncReadThreadPriority();
    }
    threadState_.store(ThreadState::READING, std::memory_order_release);
    while (!stopReadThread_.load(std::memory_order_acquire)) {
        uint32_t trackId = trackId_.load(std::memory_order_acquire);
        if (NeedWaitForRead(trackId)) {
            HandleReadWait();
        }
        if (stopReadThread_.load(std::memory_order_acquire)) {
            break;
        }
        {
            std::lock_guard<std::mutex> readLock(readSampleMutex_);
            readCacheCv_.notify_all();
        }
        if (!ReadAndCacheSample()) {
            break;
        }
    }
    {
        std::lock_guard<std::mutex> readLock(readSampleMutex_);
        readCacheCv_.notify_all();
    }
    {
        std::lock_guard<std::mutex> seekWaitLock(seekWaitMutex_);
        threadState_.store(ThreadState::NOT_STARTED, std::memory_order_release);
        seekWaitCv_.notify_all();
    }
    MEDIA_LOG_I("Read loop end");
}

Status MPEG4DemuxerPlugin::BoostReadThreadPriority()
{
    std::lock_guard<std::shared_mutex> lock(sharedMutex_);
    if (threadState_.load(std::memory_order_acquire) != ThreadState::NOT_STARTED && readThread_ != nullptr) {
        MEDIA_LOG_W("Async read thread is running, cannot set priority now");
        return Status::ERROR_WRONG_STATE;
    }
    if (isAsyncReadThreadPrioritySet_.load(std::memory_order_acquire)) {
        MEDIA_LOG_W("Async read thread priority has been set, cannot set again");
        return Status::ERROR_WRONG_STATE;
    }
    isAsyncReadThreadPrioritySet_.store(true, std::memory_order_release);
    MEDIA_LOG_I("Boost read thread priority to user interactive");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SetAVReadPacketStopState(bool state)
{
    avReadPacketStopState_.store(
        state ? AVReadPacketStopState::TRUE : AVReadPacketStopState::FALSE, std::memory_order_release);
    return Status::OK;
}

void MPEG4DemuxerPlugin::UpdateAsyncReadThreadPriority()
{
    auto ret = SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    if (ret != 0) {
        MEDIA_LOG_E("Set thread qos failed, ret = " PUBLIC_LOG_D32, ret);
    } else {
        MEDIA_LOG_I("Set thread qos success, level = " PUBLIC_LOG_U32, OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE);
    }
}

void MPEG4DemuxerPlugin::ReleaseReadLoop()
{
    {
        std::lock_guard<std::mutex> readLock(readThreadMutex_);
        stopReadThread_.store(true, std::memory_order_release);
        isPauseReadPacket_.store(false, std::memory_order_release);
        threadReady_.store(true, std::memory_order_release);
    }
    readLoopCv_.notify_all();
    {
        std::lock_guard<std::mutex> readLock(readSampleMutex_);
        readCacheCv_.notify_all();
    }
    if (readThread_ != nullptr && readThread_->joinable()) {
        readThread_->join();
    }
    readThread_ = nullptr;
    stopReadThread_.store(false, std::memory_order_release);
    threadReady_.store(false, std::memory_order_release);
    isWaitingForReadThread_.store(false, std::memory_order_release);
    threadState_.store(ThreadState::NOT_STARTED, std::memory_order_release);
    readLoopStatus_.store(Status::OK, std::memory_order_release);
}

Status MPEG4DemuxerPlugin::ReadSampleZeroCopy(uint32_t trackId, std::shared_ptr<AVBuffer> outBuffer, uint32_t timeout)
{
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    MediaAVCodec::AVCodecTrace trace(std::string("ReadSampleZeroCopy_timeout_") + std::to_string(trackId));
    FALSE_RETURN_V_MSG_E(inited_, Status::ERROR_NULL_POINTER, "Plugin has not been initialized");
    FALSE_RETURN_V_MSG_E(!selectedTrackIds_.empty(), Status::ERROR_INVALID_OPERATION, "No track has been selected");
    FALSE_RETURN_V_MSG_E(TrackIsSelected(trackId), Status::ERROR_INVALID_PARAMETER, "Track has not been selected");
    FALSE_RETURN_V_MSG_E(outBuffer != nullptr, Status::ERROR_INVALID_PARAMETER, "AVBuffer is nullptr");
    FALSE_RETURN_V_MSG_E((readModeFlags_.load(std::memory_order_acquire) & ReadModeToFlags(ReadMode::SYNC)) == 0,
        Status::ERROR_INVALID_OPERATION, "Cannot use sync and async Read together");
    readModeFlags_.fetch_or(ReadModeToFlags(ReadMode::ASYNC), std::memory_order_acq_rel);

    isPauseReadPacket_.store(false, std::memory_order_release);
    trackId_.store(trackId, std::memory_order_release);
    if (!cacheQueue_.HasCache(trackId) && timeout == 0) {
        return Status::ERROR_WAIT_TIMEOUT;
    }
    if (!readThread_) {
        stopReadThread_.store(false, std::memory_order_release);
        readLoopStatus_.store(Status::OK, std::memory_order_release);
        readThread_ = std::make_unique<std::thread>(&MPEG4DemuxerPlugin::MPEG4ReadLoop, this);
    }

    Status ret = WaitForLoop(trackId, timeout);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Frame reading thread error, ret = " PUBLIC_LOG_D32, ret);

    std::lock_guard<std::mutex> lockTrack(*trackMtx_[trackId].get());
    auto mpeg4Sample = cacheQueue_.Front(trackId);
    FALSE_RETURN_V_MSG_E(mpeg4Sample != nullptr && mpeg4Sample->sample != nullptr, Status::ERROR_NULL_POINTER,
        "Cache sample is nullptr");

    if ((mpeg4Sample->sample->flag & Sample::SampleFlag::EOS) != 0) {
        ret = SetEosSample(outBuffer, true);
        if (ret == Status::OK) {
            cacheQueue_.Pop(trackId);
            cacheSamples_.erase(trackId);
        }
        return ret;
    }

    ret = WriteSampleMemory(trackId, outBuffer, mpeg4Sample);
    if (ret == Status::OK) {
        cacheQueue_.Pop(trackId);
        cacheSamples_.erase(trackId);
    }
    return ret;
}
} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
