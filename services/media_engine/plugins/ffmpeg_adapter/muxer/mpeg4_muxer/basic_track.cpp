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

int32_t BasicTrack::GetTrackId()
{
    return trackId_;
}

std::string BasicTrack::GetMimeType()
{
    return mimeType_;
}

int32_t BasicTrack::GetMediaType()
{
    return mediaType_;
}

std::vector<uint8_t> BasicTrack::GetCodecConfig()
{
    return codecConfig_;
}

int32_t BasicTrack::GetTimeScale()
{
    return timeScale_;
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
} // Mpeg4
} // Plugins
} // Media
} // OHOS