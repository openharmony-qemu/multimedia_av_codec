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

#include "timed_meta_track.h"
#include <algorithm>
#include "avcodec_common.h"
#include "avcodec_mime_type.h"
#include "mpeg4_utils.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "TimedMetaTrack" };
}
#endif // !_WIN32

using namespace OHOS::MediaAVCodec;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
TimedMetaTrack::TimedMetaTrack(std::string mimeType, std::shared_ptr<BasicBox> moov,
    std::vector<std::shared_ptr<BasicTrack>> &tracks, bool &useTimedMeta)
    : BasicTrack(std::move(mimeType), moov), tracks_(tracks), useTimedMeta_(useTimedMeta)
{
    hdlrType_ = 'meta';
    hdlrName_ = "timed_metadata";
    trackDesc_ = std::make_shared<Meta>();
    srcTrackIds_ = std::make_shared<std::vector<uint32_t>>();
    isAuxiliary_ = true;
    trefTag_ = 'cdsc';
}

TimedMetaTrack::~TimedMetaTrack()
{
}

Status TimedMetaTrack::Init(const std::shared_ptr<Meta> &trackDesc)
{
    FALSE_RETURN_V_MSG_E(moov_ != nullptr,  Status::ERROR_INVALID_PARAMETER, "moov box is empty");
    (*trackDesc_) = (*trackDesc);

    int32_t trackIndex = -1;
    bool ret = trackDesc->GetData(Tag::TIMED_METADATA_SRC_TRACK, trackIndex);
    FALSE_RETURN_V_MSG_E(ret && trackIndex >=0 && static_cast<uint32_t>(trackIndex) < tracks_.size(),
        Status::ERROR_MISMATCHED_TYPE, "source track index not set or invalid! "
        "trackIndex:%{public}d, count:%{public}zu", trackIndex, tracks_.size());
    int32_t sourceTrackId = tracks_[trackIndex]->GetTrackId();
    FALSE_RETURN_V_MSG_E(sourceTrackId > 0, Status::ERROR_MISMATCHED_TYPE, "source track id invalid! "
        "src track id:%{public}d, mime:%{public}s", sourceTrackId, tracks_[trackIndex]->GetMimeType().c_str());
    srcTrackIds_->push_back(static_cast<uint32_t>(sourceTrackId));
    MEDIA_LOG_I("src track is track[%{public}d], src track id is:%{public}d", trackIndex, sourceTrackId);
    timeScale_ = tracks_[trackIndex]->GetTimeScale();  // use source track's time scale

    double tmpframeRate = 0;
    if (trackDesc->GetData(Tag::VIDEO_FRAME_RATE, tmpframeRate)) {  // video frame rate
        int32_t frameRate = static_cast<int32_t>(tmpframeRate);
        FALSE_RETURN_V_MSG_E(frameRate > 0, Status::ERROR_MISMATCHED_TYPE,
            "get video frame rate failed! video frame rate:%{public}lf", tmpframeRate);
        frameSize_ = ConvertTime(1, static_cast<int32_t>(frameRate), timeScale_);
        MEDIA_LOG_I("timed meta track, frame size:%{public}d, frame rate:%{public}lf", frameSize_, tmpframeRate);
    }
    mediaType_ = MediaType::TIMEDMETA;
    return Status::NO_ERROR;
}

void TimedMetaTrack::SetIsAuxiliary()
{
    if (useTimedMeta_) {
        return;
    }
    isAuxiliary_ = false;
    MEDIA_LOG_W("track[%{public}d] use timed meta track not set! src track id use self", trackId_);
    auto tref = BasicBox::GetBoxPtr<TrefBox>(moov_, trackPath_ + ".tref");
    FALSE_RETURN_MSG(tref != nullptr, "tref box is empty");
    tref->srcTrackIds_.clear();
    tref->srcTrackIds_.push_back(static_cast<uint32_t>(trackId_));
}

Status TimedMetaTrack::WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr,
        Status::ERROR_NULL_POINTER, "sample is null");
    FALSE_RETURN_V_MSG_E(sample->pts_ >= lastTimestampUs_, Status::ERROR_INVALID_PARAMETER,
        "pts: " PUBLIC_LOG_D64 " error, < " PUBLIC_LOG_D64, sample->pts_, lastTimestampUs_);
    FALSE_RETURN_V_MSG_E(io != nullptr, Status::ERROR_INVALID_OPERATION, "io is null");
    FALSE_RETURN_V_MSG_E(stsz_ != nullptr, Status::ERROR_INVALID_OPERATION, "stsz box is empty");
    int64_t pos = io->GetPos();
    io->Write(sample->memory_->GetAddr(), sample->memory_->GetSize());
    if (stsz_->sampleCount_ == 0) {
        SetIsAuxiliary();
        startTimestampUs_ = sample->pts_;
        MEDIA_LOG_I("timedMeta track:" PUBLIC_LOG_D32 ", start timestamp:" PUBLIC_LOG_D64, trackId_, startTimestampUs_);
    } else { // 先写上1帧的时间，最后少一帧stts数据，写tailer前添加
        lastDuration_ = std::max(static_cast<int64_t>(frameSize_),
            ConvertTimeToMpeg4(sample->pts_, timeScale_) - ConvertTimeToMpeg4(lastTimestampUs_, timeScale_));
        DisposeStts(lastDuration_, sample->pts_);
    }
    DisposeStco(pos);
    DisposeStsz(sample->memory_->GetSize());
    lastTimestampUs_ = sample->pts_;
    pos_ = io->GetPos();
    return Status::NO_ERROR;
}

Status TimedMetaTrack::WriteTailer()
{
    FALSE_RETURN_V_MSG_E(stsz_ != nullptr, Status::ERROR_INVALID_OPERATION, "stsz box is empty");
    if (stsz_->sampleCount_ > 0) {
        lastDuration_ = std::max(static_cast<int64_t>(frameSize_), lastDuration_);
        DisposeStts(lastDuration_, lastTimestampUs_);
        DisposeDuration();
        if (isSameSize_) {
            stsz_->sampleSize_ = stsz_->samples_.size() > 0 ? stsz_->samples_[0] : 0;
            stsz_->samples_.clear();
        }
    }
    return Status::NO_ERROR;
}

bool TimedMetaTrack::GetSrcTrackDurationUs(int64_t &durationUs, int64_t &startTimeUs)
{
    int32_t trackIndex = -1;
    bool ret = trackDesc_->GetData(Tag::TIMED_METADATA_SRC_TRACK, trackIndex);
    FALSE_RETURN_V_MSG_W(ret && trackIndex >=0 && static_cast<uint32_t>(trackIndex) < tracks_.size(), false,
        "source track index not set or invalid! trackIndex:%{public}d, count:%{public}zu", trackIndex, tracks_.size());
    durationUs = tracks_[trackIndex]->GetDurationUs();
    startTimeUs = tracks_[trackIndex]->GetStartTimeUs();
    MEDIA_LOG_I("use track[%{public}d]'s duration:" PUBLIC_LOG_D64 " us, start time:" PUBLIC_LOG_D64 " us",
        trackIndex, durationUs, startTimeUs);
    return true;
}

void TimedMetaTrack::DisposeDuration()
{
    int64_t srcTrackDurationUs = 0;
    int64_t srcStartTimeUs = 0;
    durationUs_ = lastTimestampUs_ - startTimestampUs_ + ConvertTimeFromMpeg4(lastDuration_, timeScale_);
    auto durationUs = durationUs_;
    auto startTimestampUs = startTimestampUs_;
    if (isAuxiliary_ && GetSrcTrackDurationUs(srcTrackDurationUs, srcStartTimeUs) && srcTrackDurationUs > 0) {
        durationUs = srcTrackDurationUs;
        startTimestampUs = srcStartTimeUs;
    }
    auto mvhdBox = BasicBox::GetBoxPtr<MvhdBox>(moov_, "mvhd");
    FALSE_RETURN_MSG(mvhdBox != nullptr, "mvhd box is empty");

    auto mdhdBox = BasicBox::GetBoxPtr<MdhdBox>(moov_, trackPath_ + ".mdia.mdhd");
    FALSE_RETURN_MSG(mdhdBox != nullptr, "mdhd box is empty");
    mdhdBox->duration_ = static_cast<uint64_t>(ConvertTimeToMpeg4(durationUs, timeScale_));

    auto tkhdBox = BasicBox::GetBoxPtr<TkhdBox>(moov_, trackPath_ + ".tkhd");
    FALSE_RETURN_MSG(tkhdBox != nullptr, "tkhd box is empty");
    tkhdBox->duration_ = static_cast<uint64_t>(
        ConvertTimeToMpeg4(durationUs + startTimestampUs, mvhdBox->timeScale_, RoundingType::UP));

    // set elst box
    int64_t delay = ConvertTimeToMpeg4(startTimestampUs_, mvhdBox->timeScale_, RoundingType::DOWN);
    int64_t duration = ConvertTimeToMpeg4(durationUs_, mvhdBox->timeScale_, RoundingType::UP);
    auto elstBox = BasicBox::GetBoxPtr<ElstBox>(moov_, trackPath_ + ".edts.elst");
    FALSE_RETURN_MSG(elstBox != nullptr, "elst box is empty");
    elstBox->SetVersion(duration < INT32_MAX && delay < INT32_MAX ? 0 : 1);
    elstBox->entryCount_ = 1;
    ElstBox::Data data;
    data.segmentDuration_ = static_cast<uint64_t>(duration);
    data.mediaTime_ = 0;
    if (delay > 0) {
        elstBox->entryCount_ += 1;
        ElstBox::Data data0;
        data0.segmentDuration_ = static_cast<uint64_t>(delay);
        data0.mediaTime_ = static_cast<uint64_t>(-1);
        elstBox->data_.emplace_back(data0);
    } else {
        data.segmentDuration_ = static_cast<uint64_t>(duration + delay);
        int64_t mediaTime = ConvertTimeToMpeg4(startTimestampUs_, timeScale_, RoundingType::DOWN);
        data.mediaTime_ = -std::min(static_cast<int64_t>(0), mediaTime);
    }
    elstBox->data_.emplace_back(data);
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS