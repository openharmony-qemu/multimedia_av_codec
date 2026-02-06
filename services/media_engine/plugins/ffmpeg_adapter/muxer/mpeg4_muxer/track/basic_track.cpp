/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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

#include "basic_track.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_MUXER, "Mpeg4MuxerBasicTrack"};
const std::map<std::string, uint32_t> TREF_TAG_MAP = {
    {"hint", 'hint'},
    {"cdsc", 'cdsc'},
    {"font", 'font'},
    {"hind", 'hind'},
    {"vdep", 'vdep'},
    {"vplx", 'vplx'},
    {"subt", 'subt'},
    {"thmb", 'thmb'},
    {"auxl", 'auxl'},
    {"cdtg", 'cdtg'},
    {"shsc", 'shsc'},
    {"aest", 'aest'},
};
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
BasicTrack::BasicTrack(std::string mimeType, std::shared_ptr<BasicBox> moov)
    : mimeType_(std::move(mimeType)), moov_(moov)
{
}

Status BasicTrack::Init(const std::shared_ptr<Meta> &trackDesc)
{
    return Status::NO_ERROR;
}

void BasicTrack::SetRotation(uint32_t rotation)
{
}

Status BasicTrack::WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample)
{
    return Status::NO_ERROR;
}

Status BasicTrack::WriteTailer()
{
    return Status::NO_ERROR;
}

void BasicTrack::SetTrackId(int32_t trackId)
{
    trackId_ = trackId;
}

void BasicTrack::SetTrackPath(std::string trackPath)
{
    trackPath_ = trackPath;
}

void BasicTrack::SetCodingType(std::string codingType)
{
    codingType_ = codingType;
}

int32_t BasicTrack::GetTrackId() const
{
    return trackId_;
}

std::string BasicTrack::GetMimeType() const
{
    return mimeType_;
}

MediaType BasicTrack::GetMediaType() const
{
    return mediaType_;
}

std::vector<uint8_t> &BasicTrack::GetCodecConfig()
{
    return codecConfig_;
}

int32_t BasicTrack::GetTimeScale() const
{
    return timeScale_;
}

int64_t BasicTrack::GetDurationUs() const
{
    return durationUs_;
}

int64_t BasicTrack::GetStartTimeUs() const
{
    return startTimestampUs_;
}

bool BasicTrack::IsAuxiliary() const
{
    return isAuxiliary_;
}

uint32_t BasicTrack::GetHdlrType() const
{
    return hdlrType_;
}

const std::string &BasicTrack::GetHdlrName() const
{
    return hdlrName_;
}

uint32_t BasicTrack::GetTrefTag() const
{
    return trefTag_;
}

std::shared_ptr<std::vector<uint32_t>> &BasicTrack::GetSrcTrackIds()
{
    return srcTrackIds_;
}

void BasicTrack::SetSttsBox(std::shared_ptr<SttsBox> stts)
{
    stts_ = stts;
}

void BasicTrack::SetStszBox(std::shared_ptr<StszBox> stsz)
{
    stsz_ = stsz;
}

void BasicTrack::SetStscBox(std::shared_ptr<StscBox> stsc)
{
    stsc_ = stsc;
}

void BasicTrack::SetStcoBox(std::shared_ptr<StcoBox> stco)
{
    stco_ = stco;
}

bool BasicTrack::Compare(std::shared_ptr<BasicTrack> trackOne, std::shared_ptr<BasicTrack> trackTwo)
{
    if (trackOne == nullptr || trackOne->stco_ == nullptr ||
        trackOne->stco_->isCo64_ || trackOne->stco_->chunksOffset32_.size() == 0) {
        return false;
    }
    if (trackTwo == nullptr || trackTwo->stco_ == nullptr ||
        trackTwo->stco_->isCo64_ || trackTwo->stco_->chunksOffset32_.size() == 0) {
        return true;
    }
    size_t i = trackOne->stco_->chunksOffset32_.size() - 1;
    size_t j = trackTwo->stco_->chunksOffset32_.size() - 1;
    return trackOne->stco_->chunksOffset32_[i] > trackTwo->stco_->chunksOffset32_[j];
}

uint32_t BasicTrack::MoovIncrements(uint32_t moovSize)
{
    if (stco_ == nullptr || stco_->isCo64_ || stco_->chunksOffset32_.size() == 0) {
        return 0;
    }
    size_t i = stco_->chunksOffset32_.size();
    if (stco_->chunksOffset32_[i - 1] >= UINT32_MAX - moovSize) {
        stco_->isCo64_ = true;
        return i * sizeof(uint32_t);  // 4
    }
    return 0;
}

void BasicTrack::SetOffset(uint32_t moovSize)
{
    if (stco_ != nullptr) {
        stco_->offset_ = moovSize;
    }
}

void BasicTrack::SetCreationTime(uint64_t creationTime, uint64_t modifyTime)
{
    auto tkhdBox = BasicBox::GetBoxPtr<TkhdBox>(moov_, trackPath_ + ".tkhd");
    FALSE_RETURN_MSG(tkhdBox != nullptr, "tkhd box is empty");
    tkhdBox->creationTime_ = creationTime;
    tkhdBox->modificationTime_ = modifyTime;

    auto mdhdBox = BasicBox::GetBoxPtr<MdhdBox>(moov_, trackPath_ + ".mdia.mdhd");
    FALSE_RETURN_MSG(mdhdBox != nullptr, "mdhd box is empty");
    mdhdBox->creationTime_ = creationTime;
    mdhdBox->modificationTime_ = modifyTime;
}

void BasicTrack::DisposeStts(int64_t duration, int64_t pts)
{
    FALSE_RETURN_MSG(stts_ != nullptr, "track[%{public}d] stts box is empty", trackId_);
    if (duration > INT32_MAX || duration < 0) {
        MEDIA_LOG_E("track[%{public}d] stts sample_delta must <= INT32_MAX and >= 0, but the value is "
            PUBLIC_LOG_D64 ". the pts is " PUBLIC_LOG_D64, trackId_, duration, pts);
    }
    stts_->AddData(static_cast<uint32_t>(duration));
}

void BasicTrack::DisposeStco(int64_t pos)
{
    constexpr size_t num2 = 2;
    FALSE_RETURN_MSG(stsc_ != nullptr, "track[%{public}d] stsc box is empty", trackId_);
    FALSE_RETURN_MSG(stco_ != nullptr, "track[%{public}d] stco box is empty", trackId_);
    size_t iChunk = stsc_->chunks_.size();
    if (pos_ == pos && iChunk > 0) {
        stsc_->chunks_[iChunk - 1].samplesPerChunk_++;
    } else {
        stco_->chunkCount_++;
        if (pos <= UINT32_MAX) {
            stco_->chunksOffset32_.emplace_back(static_cast<uint32_t>(pos));
        } else {
            stco_->isCo64_ = true;
            stco_->chunksOffset64_.emplace_back(pos);
        }
        if (iChunk >= num2 &&
            stsc_->chunks_[iChunk - 1].samplesPerChunk_ == stsc_->chunks_[iChunk - num2].samplesPerChunk_) {
            stsc_->chunks_.pop_back();
        }
        StscBox::Data stscData;
        stscData.chunkNum_ = stco_->chunkCount_;
        stscData.samplesPerChunk_ = 1;
        stsc_->chunks_.emplace_back(stscData);
        stsc_->entryCount_ = static_cast<uint32_t>(stsc_->chunks_.size());
    }
}

void BasicTrack::DisposeStsz(int32_t size)
{
    FALSE_RETURN_MSG(stsz_ != nullptr, "track[%{public}d] stsz box is empty", trackId_);
    allSampleSize_ += static_cast<uint64_t>(size);
    if (isSameSize_ && stsz_->sampleCount_ > 0 &&
        static_cast<int32_t>(stsz_->samples_[stsz_->sampleCount_ - 1]) != size) {
        isSameSize_ = false;
    }
    stsz_->sampleCount_++;
    stsz_->samples_.emplace_back(size);
}

Status BasicTrack::SetAuxiliaryTrackParam(const std::shared_ptr<Meta> &trackDesc)
{
    constexpr uint32_t compareLen = 16;
    hdlrType_ = 'auxv';
    isAuxiliary_ = true;
    // get reference track description (hdlr name)
    std::string trackDescription;
    FALSE_RETURN_V_MSG_E(trackDesc->GetData(Tag::TRACK_DESCRIPTION, trackDescription),
        Status::ERROR_INVALID_PARAMETER, "missing track description.");
    FALSE_RETURN_V_MSG_E(trackDescription.compare(0, compareLen, "com.openharmony.") == 0,
        Status::ERROR_INVALID_PARAMETER, "track description %{public}s must com.openharmony.xxx!",
        trackDescription.c_str());
    hdlrName_ = trackDescription;

    // get track reference type
    std::string trackRefType;
    FALSE_RETURN_V_MSG_E(trackDesc->GetData(Tag::TRACK_REFERENCE_TYPE, trackRefType), Status::ERROR_INVALID_PARAMETER,
        "miss track reference type.");
    FALSE_RETURN_V_MSG_E(TREF_TAG_MAP.count(trackRefType) > 0, Status::ERROR_INVALID_PARAMETER,
        "unsupport tref type: %{public}s", trackRefType.c_str());
    trefTag_ = TREF_TAG_MAP.at(trackRefType);
    return Status::NO_ERROR;
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS