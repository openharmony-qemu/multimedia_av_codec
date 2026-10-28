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
#include "muxer_filter.h"
#include <algorithm>
#include <sys/timeb.h>
#include <thread>
#include <unordered_map>
#include <vector>
#include "common/log.h"
#include "common/media_core.h"
#include "filter/filter_factory.h"
#include "muxer/media_muxer.h"
#include "avcodec_trace.h"
#include "avcodec_sysevent.h"
#include "avcodec_log.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_RECORDER, "MuxerFilter" };
static const std::unordered_map<OHOS::Media::Plugins::OutputFormat, std::string> FORMAT_TABLE = {
    {OHOS::Media::Plugins::OutputFormat::DEFAULT, OHOS::Media::Plugins::MimeType::MEDIA_MP4},
    {OHOS::Media::Plugins::OutputFormat::MPEG_4, OHOS::Media::Plugins::MimeType::MEDIA_MP4},
    {OHOS::Media::Plugins::OutputFormat::M4A, OHOS::Media::Plugins::MimeType::MEDIA_M4A},
    {OHOS::Media::Plugins::OutputFormat::AMR, OHOS::Media::Plugins::MimeType::MEDIA_AMR},
    {OHOS::Media::Plugins::OutputFormat::MP3, OHOS::Media::Plugins::MimeType::MEDIA_MP3},
    {OHOS::Media::Plugins::OutputFormat::WAV, OHOS::Media::Plugins::MimeType::MEDIA_WAV},
    {OHOS::Media::Plugins::OutputFormat::AAC, OHOS::Media::Plugins::MimeType::MEDIA_AAC},
};
}

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace OHOS::MediaAVCodec;
constexpr int64_t WAIT_TIME_OUT_NS = 3000000000;
constexpr int64_t US_TO_MS = 1000;
constexpr int64_t S_TO_MS = 1000;
constexpr int64_t SYNC_THRESHOLD_US = 200000; // 200ms
constexpr uint32_t BUFFER_IS_EOS = 1;
constexpr uint32_t BUFFER_IS_CODEC_CONFIG = 1 << 3; // AVCodecBufferFlag::AVCODEC_BUFFER_FLAG_CODEC_DATA
constexpr int32_t VALID_MIN_DURATION = 1;
static AutoRegisterFilter<MuxerFilter> g_registerMuxerFilter("builtin.recorder.muxer", FilterType::FILTERTYPE_MUXER,
    [](const std::string& name, const FilterType type) {
        return std::make_shared<MuxerFilter>(name, FilterType::FILTERTYPE_MUXER);
    });

class MuxerBrokerListener : public IBrokerListener {
public:
    MuxerBrokerListener(std::shared_ptr<MuxerFilter> muxerFilter, int32_t trackIndex,
        StreamType streamType, sptr<AVBufferQueueProducer> inputBufferQueue)
        : muxerFilter_(std::move(muxerFilter)), trackIndex_(trackIndex), streamType_(streamType),
        inputBufferQueue_(inputBufferQueue)
    {
    }

    sptr<IRemoteObject> AsObject() override
    {
        return nullptr;
    }

    void OnBufferFilled(std::shared_ptr<AVBuffer> &avBuffer) override
    {
        if (inputBufferQueue_ != nullptr) {
            if (auto muxerFilter = muxerFilter_.lock()) {
                muxerFilter->OnBufferFilled(avBuffer, trackIndex_, streamType_, inputBufferQueue_.promote());
            } else {
                MEDIA_LOG_I("invalid muxerFilter");
            }
        }
    }

private:
    std::weak_ptr<MuxerFilter> muxerFilter_;
    int32_t trackIndex_;
    StreamType streamType_;
    wptr<AVBufferQueueProducer> inputBufferQueue_;
};

MuxerFilter::MuxerFilter(std::string name, FilterType type): Filter(name, type)
{
    MEDIA_LOG_I("MuxerFilter create");
}

MuxerFilter::~MuxerFilter()
{
    MEDIA_LOG_I("MuxerFilter destroy");
}

Status MuxerFilter::SetOutputParameter(int32_t appUid, int32_t appPid, int32_t fd, int32_t format)
{
    MEDIA_LOG_I("SetOutputParameter, appUid:" PUBLIC_LOG_D32 ", appPid:" PUBLIC_LOG_D32 ", format:" PUBLIC_LOG_D32,
        static_cast<int32_t>(appUid), static_cast<int32_t>(appPid), static_cast<int32_t>(format));
    mediaMuxer_ = std::make_shared<MediaMuxer>(appUid, appPid);
    Status ret = mediaMuxer_->Init(fd, (Plugins::OutputFormat)format);
    outputFormat_ = (Plugins::OutputFormat)format;
    if (ret != Status::OK) {
        SetFaultEvent("MuxerFilter::SetOutputParameter, muxerFilter init error", (int32_t)ret);
    }
    return ret;
}

Status MuxerFilter::SetTransCoderMode()
{
    MEDIA_LOG_I("SetTransCoderMode");
    isTransCoderMode.store(true);
    return Status::OK;
}

void MuxerFilter::SetClipEndTimeUs(int64_t endTimeUs)
{
    MEDIA_LOG_I("SetClipEndTimeUs: " PUBLIC_LOG_D64, endTimeUs);
    clipEndTimeUs_ = endTimeUs;
}

void MuxerFilter::SetClipStartTimeUs(int64_t startTimeUs)
{
    MEDIA_LOG_I("SetClipStartTimeUs: " PUBLIC_LOG_D64, startTimeUs);
    clipStartTimeUs_ = startTimeUs;
}

int64_t MuxerFilter::GetCurrentPtsMs()
{
    // 只要有视频轨，都以视频pts为准，反之以音频pts为准
    if (videoCodecMimeType_.empty()) {
        return lastAudioPts_.load() / US_TO_MS;
    }
    return lastVideoPts_.load() / US_TO_MS;
}

void MuxerFilter::Init(const std::shared_ptr<EventReceiver> &receiver,
    const std::shared_ptr<FilterCallback> &callback)
{
    MEDIA_LOG_I("Init");
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::Init");
    eventReceiver_ = receiver;
    filterCallback_ = callback;
}

Status MuxerFilter::DoPrepare()
{
    MEDIA_LOG_I("Prepare");
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::Prepare");
    return Status::OK;
}

Status MuxerFilter::DoStart()
{
    MEDIA_LOG_I("MuxerFilter Start");
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::Start");

    CHECK_AND_RETURN_RET_LOG(!isStarted, Status::OK, "MuxerFilter has started");
    FALSE_RETURN_V_MSG(mediaMuxer_ != nullptr, Status::ERROR_FRAMEWORK_ERR, "MediaMuxer is nullptr.");
    if (!pendingMetaTracks_.empty()) {
        auto ret = AddPendingMetaTracks();
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "AddPendingMetaTracks failed in DoStart");
    }
    Status ret = mediaMuxer_->Start();
    if (ret != Status::OK) {
        SetFaultEvent("MuxerFilter::DoStart error", (int32_t)ret);
        return Status::ERROR_MUXER_START_FAILED;
    } else {
        isStarted = true;
        isReachMaxDuration_.store(false);
    }
    return ret;
}

Status MuxerFilter::DoPause()
{
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::Pause");
    MEDIA_LOG_I("Pause");
    return Status::OK;
}

Status MuxerFilter::DoResume()
{
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::Resume");
    MEDIA_LOG_I("Resume");
    return Status::OK;
}

Status MuxerFilter::DoStop()
{
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::Stop");
    FALSE_RETURN_V_MSG(mediaMuxer_ != nullptr, Status::ERROR_NULL_POINTER, "MediaMuxer is nullptr.");
    stopCount_++;
    MEDIA_LOG_I("MuxerFilter Stop, stopCount_: %{public}d, preFilterCount_: %{public}d",
        stopCount_.load(), preFilterCount_);
    if (stopCount_.load() == preFilterCount_) {
        stopCount_.store(0);
        Status ret = mediaMuxer_->Stop();
        if (ret == Status::OK) {
            isStarted = false;
        } else if (ret == Status::ERROR_WRONG_STATE) {
            ret = Status::OK;
        } else {
            ret = Status::ERROR_MUXER_STOP_FAILED;
            SetFaultEvent("MuxerFilter::DoStop error", (int32_t)ret);
        }
        return ret;
    }
    return Status::OK;
}

Status MuxerFilter::DoFlush()
{
    return Status::OK;
}

Status MuxerFilter::DoRelease()
{
    MEDIA_LOG_I("Release");
    return Status::OK;
}

void MuxerFilter::SetParameter(const std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("SetParameter");
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::SetParameter");
    FALSE_RETURN_MSG(mediaMuxer_ != nullptr, "mediaMuxer_ is nullptr");
    mediaMuxer_->SetParameter(parameter);
}

Status MuxerFilter::SetUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    MEDIA_LOG_I("SetUserMeta enter");
    FALSE_RETURN_V_MSG(mediaMuxer_ != nullptr, Status::ERROR_NULL_POINTER, "MediaMuxer is nullptr.");
    Status ret = mediaMuxer_->SetUserMeta(userMeta);
    if (ret != Status::OK) {
        MEDIA_LOG_I("SetUserMeta failed");
    }
    return ret;
}

void MuxerFilter::GetParameter(std::shared_ptr<Meta> &parameter)
{
    MEDIA_LOG_I("GetParameter");
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::GetParameter");
}

Status MuxerFilter::LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    return Status::OK;
}

Status MuxerFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UpdateNext");
    return Status::OK;
}

Status MuxerFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType)
{
    MEDIA_LOG_I("UnLinkNext");
    return Status::OK;
}

FilterType MuxerFilter::GetFilterType()
{
    MEDIA_LOG_I("GetFilterType");
    return FilterType::FILTERTYPE_MUXER;
}

void MuxerFilter::ConfigureTrackMeta(const std::shared_ptr<Meta> &meta, const std::string &mimeType)
{
    if (mimeType.find("audio/") == 0) {
        audioCodecMimeType_ = mimeType;
        if (mimeType == "audio/mp4a-latm") {
            meta->Set<Tag::AUDIO_AAC_IS_ADTS>(isAdts_);
            if (meta->Find(Tag::MEDIA_PROFILE) == meta->end()) {
                meta->Set<Tag::MEDIA_PROFILE>(mediaProfile_);
            }
        }
    } else if (mimeType.find("video/") == 0) {
        videoCodecMimeType_ = mimeType;
    } else if (mimeType.find("meta/") == 0) {
        metaDataCodecMimeType_ = mimeType;
    }
}

Status MuxerFilter::AddPendingMetaTracks()
{
    std::vector<PendingMetaTrack> stillPending;
    for (auto &pending : pendingMetaTracks_) {
        std::string srcMimeType;
        pending.meta->Get<Tag::TIMED_METADATA_SRC_TRACK_MIME>(srcMimeType);
        auto it = trackIndexMap_.find(srcMimeType);
        if (it == trackIndexMap_.end()) {
            MEDIA_LOG_I("Source track not ready for meta track, re-pending. srcMimeType: %{public}s",
                srcMimeType.c_str());
            stillPending.push_back(pending);
            continue;
        }
        pending.meta->Set<Tag::TIMED_METADATA_SRC_TRACK>(it->second);
        int32_t trackIndex = -1;
        auto ret = mediaMuxer_->AddTrack(trackIndex, pending.meta);
        if (ret != Status::OK) {
            MEDIA_LOG_E("AddPendingMetaTracks AddTrack failed, ret: %{public}d", static_cast<int32_t>(ret));
            if (eventReceiver_ != nullptr) {
                eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_ERROR, static_cast<int32_t>(ret)});
            }
            SetFaultEvent("MuxerFilter::AddPendingMetaTracks error", (int32_t)ret);
            pendingMetaTracks_ = std::move(stillPending);
            return ret;
        }
        std::string mimeType;
        pending.meta->Get<Tag::MIME_TYPE>(mimeType);
        trackIndexMap_.emplace(std::make_pair(mimeType, trackIndex));
        sptr<AVBufferQueueProducer> inputBufferQueue = mediaMuxer_->GetInputBufferQueue(trackIndex);
        if (inputBufferQueue == nullptr) {
            MEDIA_LOG_E("AddPendingMetaTracks inputBufferQueue is nullptr");
            pendingMetaTracks_ = std::move(stillPending);
            return Status::ERROR_NULL_POINTER;
        }
        if (pending.callback != nullptr) {
            pending.callback->OnLinkedResult(inputBufferQueue, const_cast<std::shared_ptr<Meta> &>(pending.meta));
        }
        sptr<IBrokerListener> listener = new MuxerBrokerListener(shared_from_this(), trackIndex,
            pending.streamType, inputBufferQueue);
        inputBufferQueue->SetBufferFilledListener(listener);
        preFilterCount_++;
        {
            std::lock_guard<std::mutex> lock(bufferPtsMutex_);
            bufferPtsMap_.insert(std::pair<int32_t, int64_t>(trackIndex, 0));
        }
        MEDIA_LOG_I("AddPendingMetaTracks done, mimeType: %{public}s, trackIndex: %{public}d",
            mimeType.c_str(), trackIndex);
    }
    pendingMetaTracks_ = std::move(stillPending);
    return Status::OK;
}

Status MuxerFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnLinked");
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::OnLinked");
    int32_t trackIndex;
    std::string mimeType;
    FALSE_RETURN_V_MSG(meta != nullptr, Status::ERROR_NULL_POINTER, "meta is nullptr.");
    meta->Get<Tag::MIME_TYPE>(mimeType);
    MEDIA_LOG_I("MuxerFilter::OnLinked, mimeType: %{public}s", mimeType.c_str());
    ConfigureTrackMeta(meta, mimeType);
    FALSE_RETURN_V_MSG(mediaMuxer_ != nullptr, Status::ERROR_NULL_POINTER, "MediaMuxer is nullptr.");

    // For meta tracks, defer AddTrack until source track is registered
    if (mimeType.find("meta/") == 0) {
        std::string srcMimeType;
        meta->Get<Tag::TIMED_METADATA_SRC_TRACK_MIME>(srcMimeType);
        if (trackIndexMap_.find(srcMimeType) == trackIndexMap_.end()) {
            MEDIA_LOG_I("Meta track source not ready, pending. srcMimeType: %{public}s", srcMimeType.c_str());
            pendingMetaTracks_.push_back({meta, callback, inType});
            return Status::OK;
        }
    }

    auto ret = mediaMuxer_->AddTrack(trackIndex, meta);
    if (ret != Status::OK && eventReceiver_ != nullptr) {
        if (isTransCoderMode.load()) {
            eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_ERROR, static_cast<int32_t>(MSERR_MUXER_FAILED)});
        } else {
            eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_ERROR, static_cast<int32_t>(ret)});
        }
        SetFaultEvent("MuxerFilter::OnLinked error", (int32_t)ret);
        return ret;
    }
    trackIndexMap_.emplace(std::make_pair(mimeType, trackIndex));

    // Process any pending meta tracks now that current track is registered in trackIndexMap_
    if (mimeType.find("audio/") == 0 || mimeType.find("video/") == 0) {
        ret = AddPendingMetaTracks();
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "AddPendingMetaTracks failed");
    }

    sptr<AVBufferQueueProducer> inputBufferQueue = mediaMuxer_->GetInputBufferQueue(trackIndex);
    FALSE_RETURN_V_MSG(inputBufferQueue != nullptr, Status::ERROR_NULL_POINTER, "inputBufferQueue is nullptr");
    FALSE_RETURN_V_MSG(callback != nullptr, Status::ERROR_NULL_POINTER, "callback is nullptr");
    callback->OnLinkedResult(inputBufferQueue, const_cast<std::shared_ptr<Meta> &>(meta));
    sptr<IBrokerListener> listener = new MuxerBrokerListener(shared_from_this(), trackIndex,
        inType, inputBufferQueue);
    inputBufferQueue->SetBufferFilledListener(listener);
    preFilterCount_++;

    {
        std::lock_guard<std::mutex> lock(bufferPtsMutex_);
        bufferPtsMap_.insert(std::pair<int32_t, int64_t>(trackIndex, 0));
    }
    return Status::OK;
}

Status MuxerFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
    const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUpdated");
    return Status::OK;
}


Status MuxerFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback)
{
    MEDIA_LOG_I("OnUnLinked");
    return Status::OK;
}

void MuxerFilter::OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer, int32_t trackIndex,
    StreamType streamType, sptr<AVBufferQueueProducer> inputBufferQueue)
{
    MEDIA_LOG_D("OnBufferFilled");
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::OnBufferFilled");
    if (!isTransCoderMode.load()) {
        int64_t currentBufferPts = inputBuffer->pts_;
        if (currentBufferPts / US_TO_MS > maxDuration_ * S_TO_MS && isReachMaxDuration_.load() == false) {
            MEDIA_LOG_I("MuxerFilter::OnBufferFilled currentBufferPts > maxDuration_ start to stop");
            isReachMaxDuration_.store(true);
            std::shared_ptr<MuxerFilter> self = this->shared_from_this();
            std::thread asyncThread(std::bind(&MuxerFilter::EventCompleteStopAsync, self));
            asyncThread.detach();
        }
        int64_t anotherBufferPts = 0;
        {
            std::lock_guard<std::mutex> lock(bufferPtsMutex_);
            for (auto mapInterator = bufferPtsMap_.begin(); mapInterator != bufferPtsMap_.end(); mapInterator++) {
                if (mapInterator->first != trackIndex) {
                    anotherBufferPts = mapInterator->second;
                }
            }
            bufferPtsMap_[trackIndex] = currentBufferPts;
        }
        if (preFilterCount_ != 1 && std::abs(currentBufferPts - anotherBufferPts) >= WAIT_TIME_OUT_NS) {
            MEDIA_LOG_I("OnBufferFilled pts time interval is greater than 3 seconds");
        }
        MEDIA_LOG_D("OnBufferFilled buffer->pts" PUBLIC_LOG_D64, inputBuffer->pts_);
        inputBufferQueue->ReturnBuffer(inputBuffer, true);
        return;
    }
    OnTransCoderBufferFilled(inputBuffer, trackIndex, streamType, inputBufferQueue);
}

void MuxerFilter::EventCompleteStopAsync()
{
    MEDIA_LOG_I("MuxerFilter EventCompleteStopAsync");
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "eventReceiver_ is nullptr");
    eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_COMPLETE, Status::OK});
}

void MuxerFilter::OnTransCoderBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer, int32_t trackIndex,
    StreamType streamType, sptr<AVBufferQueueProducer> inputBufferQueue)
{
    MEDIA_LOG_D("OnTransCoderBufferFilled");
    FALSE_RETURN_MSG(inputBuffer != nullptr && inputBufferQueue != nullptr,
        "OnTransCoderBufferFilled invalid params, inputBuffer or inputBufferQueue is nullptr");

    // Step 1: Discard frames whose PTS is before the clip start time.
    FALSE_RETURN_NOLOG(!FilterPreClipBuffers(inputBuffer, streamType, inputBufferQueue));

    // Step 2: If clip end time is set, truncate frames past the end and send EOS.
    FALSE_RETURN_NOLOG(!HandleClipEnd(inputBuffer, streamType, inputBufferQueue));

    // Step 3: Adjust PTS so output starts from 0.
    AdjustPtsForClip(inputBuffer, streamType);

    // Step 4: Handle EOS bookkeeping.
    bool isCompleted = false;
    HandleTransCoderEos(inputBuffer, streamType, isCompleted);

    // Step 5: Write the buffer to the muxer with audio/video sync.
    WriteTransCoderBuffer(inputBuffer, streamType, inputBufferQueue);

    if (eventReceiver_ != nullptr && isCompleted) {
        HandleTransCoderComplete();
    }
}

bool MuxerFilter::FilterPreClipBuffers(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType,
    sptr<AVBufferQueueProducer> inputBufferQueue)
{
    // Discard buffers before clip start time (handles pre-clip audio frames after seek).
    // Always keep CODEC_CONFIG frames (SPS/PPS) and EOS frames regardless of PTS.
    bool shouldFilter = clipStartTimeUs_ > 0 && (inputBuffer->flag_ & BUFFER_IS_EOS) == 0 &&
        (inputBuffer->flag_ & BUFFER_IS_CODEC_CONFIG) == 0 && inputBuffer->pts_ < clipStartTimeUs_;
    if (!shouldFilter) {
        LogFirstAudioAfterClip(inputBuffer, streamType);
        return false;
    }
    if (streamType == StreamType::STREAMTYPE_ENCODED_AUDIO) {
        audioClipFilteredCount_.fetch_add(1, std::memory_order_relaxed);
        MEDIA_LOG_D("Audio before clip start, discarding. pts: " PUBLIC_LOG_D64
            " clipStartTime: " PUBLIC_LOG_D64 " filteredCount: " PUBLIC_LOG_D32,
            inputBuffer->pts_, clipStartTimeUs_, audioClipFilteredCount_.load());
    } else {
        MEDIA_LOG_D("Buffer before clip start, discarding. streamType: " PUBLIC_LOG_D32
            " pts: " PUBLIC_LOG_D64 " clipStartTime: " PUBLIC_LOG_D64,
            static_cast<int32_t>(streamType), inputBuffer->pts_, clipStartTimeUs_);
    }
    inputBufferQueue->ReturnBuffer(inputBuffer, false);
    return true;
}

bool MuxerFilter::HandleClipEnd(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType,
    sptr<AVBufferQueueProducer> inputBufferQueue)
{
    FALSE_RETURN_V_MSG_E(clipEndTimeUs_ > 0, false, "clipEndTimeUs_ is not set");
    // If this track already hit EOS (clip end or real EOS), discard further buffers.
    bool trackAlreadyEos = IsTrackEos(streamType);
    if (trackAlreadyEos) {
        inputBufferQueue->ReturnBuffer(inputBuffer, false);
        return true;
    }
    // Frames past clip end: discard data but send an empty EOS buffer to signal completion.
    if ((inputBuffer->flag_ & BUFFER_IS_EOS) == 0 && inputBuffer->pts_ > clipEndTimeUs_) {
        MEDIA_LOG_I("Clip end time reached, pts: " PUBLIC_LOG_D64 " clipEndTime: " PUBLIC_LOG_D64
            ", discarding frame data and sending EOS only, flag=0x%{public}08x isCodecConfig=%{public}d",
            inputBuffer->pts_, clipEndTimeUs_, inputBuffer->flag_,
            (inputBuffer->flag_ & BUFFER_IS_CODEC_CONFIG) != 0);
        if (inputBuffer->memory_ != nullptr) {
            inputBuffer->memory_->SetSize(0);
        }
        inputBuffer->flag_ |= BUFFER_IS_EOS;
    }
    return false;
}

void MuxerFilter::AdjustPtsForClip(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType)
{
    // Make output start from 0 so the player shows correct duration.
    FALSE_RETURN_NOLOG(clipStartTimeUs_ > 0);
    int64_t ptsBefore = inputBuffer->pts_;
    // Video CODEC_CONFIG frames (SPS/PPS) should be at pts=0 in the output.
    // Audio frames may carry the CODEC_CONFIG flag but are regular data frames
    // with valid PTS that must be offset, not zeroed.
    bool isVideoCodecConfig = (inputBuffer->flag_ & BUFFER_IS_CODEC_CONFIG) != 0 &&
        streamType != StreamType::STREAMTYPE_ENCODED_AUDIO;
    if (isVideoCodecConfig) {
        inputBuffer->pts_ = 0;
    } else {
        inputBuffer->pts_ = std::max(inputBuffer->pts_ - clipStartTimeUs_, static_cast<int64_t>(0));
    }
    if (streamType == StreamType::STREAMTYPE_ENCODED_AUDIO &&
        ((inputBuffer->flag_ & BUFFER_IS_EOS) != 0 || audioClipPassedCount_.load() <= 5)) {
        MEDIA_LOG_I("Audio PTS adjust: " PUBLIC_LOG_D64 " -> " PUBLIC_LOG_D64
            " clipStart=" PUBLIC_LOG_D64 " flag=0x%{public}08x passedCount=" PUBLIC_LOG_D32,
            ptsBefore, inputBuffer->pts_, clipStartTimeUs_, inputBuffer->flag_, audioClipPassedCount_.load());
    }
}

void MuxerFilter::HandleTransCoderEos(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType,
    bool &isCompleted)
{
    FALSE_RETURN_NOLOG((inputBuffer->flag_ & BUFFER_IS_EOS) != 0);
    std::unique_lock<std::mutex> lock(eosMutex_);
    eosCount_++;
    UpdateTrackEosState(inputBuffer, streamType);
    isCompleted = (eosCount_ == preFilterCount_) || (videoIsEos.load() && audioIsEos.load());
}

void MuxerFilter::LogFirstAudioAfterClip(const std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType)
{
    FALSE_RETURN_NOLOG(streamType == StreamType::STREAMTYPE_ENCODED_AUDIO);
    int32_t passed = audioClipPassedCount_.fetch_add(1, std::memory_order_relaxed) + 1;
    if (passed == 1) {
        MEDIA_LOG_I("First audio frame after clip filter, pts: " PUBLIC_LOG_D64
            " clipStartTime: " PUBLIC_LOG_D64, inputBuffer->pts_, clipStartTimeUs_);
    }
}

bool MuxerFilter::IsTrackEos(StreamType streamType)
{
    if (streamType == StreamType::STREAMTYPE_ENCODED_VIDEO) {
        return videoIsEos.load();
    }
    if (streamType == StreamType::STREAMTYPE_ENCODED_AUDIO) {
        return audioIsEos.load();
    }
    return false;
}

void MuxerFilter::UpdateTrackEosState(const std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType)
{
    if (streamType == StreamType::STREAMTYPE_ENCODED_VIDEO) {
        MEDIA_LOG_I("video is eos");
        videoIsEos.store(true);
        return;
    }
    if (streamType == StreamType::STREAMTYPE_ENCODED_AUDIO) {
        MEDIA_LOG_I("audio is eos, passedCount: " PUBLIC_LOG_D32
            " filteredCount: " PUBLIC_LOG_D32
            " lastAudioPts: " PUBLIC_LOG_D64,
            audioClipPassedCount_.load(), audioClipFilteredCount_.load(), inputBuffer->pts_);
        audioIsEos.store(true);
    }
}

void MuxerFilter::WriteTransCoderBuffer(std::shared_ptr<AVBuffer> &inputBuffer, StreamType streamType,
    sptr<AVBufferQueueProducer> inputBufferQueue)
{
    if (streamType == StreamType::STREAMTYPE_ENCODED_AUDIO) {
        lastAudioPts_.store(inputBuffer->pts_);
        if (videoCodecMimeType_.empty()) {
            inputBufferQueue->ReturnBuffer(inputBuffer, true);
        } else if (inputBuffer->pts_ <= lastVideoPts_.load() + SYNC_THRESHOLD_US || videoIsEos.load()) {
            inputBufferQueue->ReturnBuffer(inputBuffer, true);
        } else {
            MEDIA_LOG_D("Audio ahead of video, waiting. audioPts: " PUBLIC_LOG_D64
                " lastVideoPts: " PUBLIC_LOG_D64 " diff: " PUBLIC_LOG_D64 " us",
                inputBuffer->pts_, lastVideoPts_.load(),
                inputBuffer->pts_ - lastVideoPts_.load());
            int64_t audioPts = inputBuffer->pts_;
            std::unique_lock<std::mutex> lock(stopMutex_);
            stopCondition_.wait_for(lock, std::chrono::milliseconds(US_TO_MS),
                [this, audioPts]() {
                    return audioPts <= lastVideoPts_.load() + SYNC_THRESHOLD_US
                        || videoIsEos.load();
                });
            inputBufferQueue->ReturnBuffer(inputBuffer, true);
        }
    } else if (streamType == StreamType::STREAMTYPE_ENCODED_VIDEO) {
        if (!videoIsEos.load()) {
            lastVideoPts_.store(inputBuffer->pts_);
        }
        std::unique_lock<std::mutex> lock(stopMutex_);
        stopCondition_.notify_all();
        inputBufferQueue->ReturnBuffer(inputBuffer, true);
    } else {
        inputBufferQueue->ReturnBuffer(inputBuffer, true);
    }
}

void MuxerFilter::HandleTransCoderComplete()
{
    FALSE_RETURN_MSG(mediaMuxer_ != nullptr, "mediaMuxer_ is nullptr");
    MEDIA_LOG_I("mediaMuxer_ stop begin");
    stopCount_.store(preFilterCount_);
    Status res = mediaMuxer_->Stop();
    FALSE_RETURN_MSG(eventReceiver_ != nullptr, "eventReceiver_ is nullptr");
    if (res == Status::OK) {
        MEDIA_LOG_I("mediaMuxer_ stop success");
        eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_COMPLETE, MSERR_OK});
    } else {
        MEDIA_LOG_I("mediaMuxer_ stop failed, res = %{public}d", static_cast<int32_t>(res));
        eventReceiver_->OnEvent({"muxer_filter", EventType::EVENT_ERROR, static_cast<int32_t>(MSERR_MUXER_FAILED)});
    }
}

void MuxerFilter::SetFaultEvent(const std::string &errMsg, int32_t ret)
{
    SetFaultEvent(errMsg + ", ret = " + std::to_string(ret));
}

void MuxerFilter::SetFaultEvent(const std::string &errMsg)
{
    MuxerFaultInfo muxerFaultInfo;
    muxerFaultInfo.appName = bundleName_;
    muxerFaultInfo.instanceId = std::to_string(instanceId_);
    muxerFaultInfo.callerType = "player_framework";
    muxerFaultInfo.videoCodec = videoCodecMimeType_;
    muxerFaultInfo.audioCodec = audioCodecMimeType_;
    muxerFaultInfo.metaCodec = metaDataCodecMimeType_;
    muxerFaultInfo.containerFormat = GetContainerFormat(outputFormat_);
    muxerFaultInfo.errMsg = errMsg;
    FaultMuxerEventWrite(muxerFaultInfo);
}

const std::string &MuxerFilter::GetContainerFormat(Plugins::OutputFormat format)
{
    static std::string emptyFormat = "";
    FALSE_RETURN_V_MSG_E(FORMAT_TABLE.find(format) != FORMAT_TABLE.end(), emptyFormat,
        "The output format %{public}d is not supported!", format);
    return FORMAT_TABLE.at(format);
}

void MuxerFilter::SetCallingInfo(int32_t appUid, int32_t appPid,
    const std::string &bundleName, uint64_t instanceId)
{
    appUid_ = appUid;
    appPid_ = appPid;
    bundleName_ = bundleName;
    instanceId_ = instanceId;
}

void MuxerFilter::SetMaxDuration(int32_t maxDuration)
{
    MEDIA_LOG_I("MuxerFilter SetMaxDuration = %{public}d", maxDuration);
    MediaAVCodec::AVCodecTrace trace("MuxerFilter::SetMaxDuration");
    if (maxDuration < VALID_MIN_DURATION) {
        maxDuration = INT32_MAX;
        MEDIA_LOG_I("MuxerFilter MaxDuration set to INT32_MAX");
    }
    maxDuration_ = maxDuration;
}
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
