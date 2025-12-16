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

#include "audio_track.h"
#include <algorithm>
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_mime_type.h"
#include "mpeg4_utils.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "AudioTrack" };
}
#endif // !_WIN32

using namespace OHOS::MediaAVCodec;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
AudioTrack::AudioTrack(std::string mimeType, std::shared_ptr<BasicBox> moov)
    : BasicTrack(std::move(mimeType), moov)
{
}

AudioTrack::~AudioTrack()
{
}

Status AudioTrack::Init(const std::shared_ptr<Meta> &trackDesc)
{
    mediaType_ = OHOS::MediaAVCodec::MEDIA_TYPE_AUD;
    bool ret = trackDesc->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_); // sample rate
    FALSE_RETURN_V_MSG_E((ret && sampleRate_ > 0), Status::ERROR_MISMATCHED_TYPE,
        "get audio sample_rate failed! sampleRate:%{public}d", sampleRate_);
    ret = trackDesc->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_); // channels
    FALSE_RETURN_V_MSG_E((ret && channels_ > 0), Status::ERROR_MISMATCHED_TYPE,
        "get audio channels failed! channels:%{public}d", channels_);
    if (trackDesc->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != trackDesc->end()) {
        trackDesc->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize_);  // frame size
        MEDIA_LOG_I("audio track:%{public}d, frame size:%{public}d", trackId_, frameSize_);
    }
    if (trackDesc->Find(Tag::MEDIA_BITRATE) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_BITRATE>(bitRate_); // bit rate
    }
    codecConfig_.clear();
    if (trackDesc->Find(Tag::MEDIA_CODEC_CONFIG) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig_); // codec config
        MEDIA_LOG_I("audio track:%{public}d, codec config len:%{public}zu", trackId_, codecConfig_.size());
    }
    timeScale_ = sampleRate_;
    return Status::NO_ERROR;
}

//    const uint8_t* sample, AVCodecBufferInfo info, AVCodecBufferFlag flag
Status AudioTrack::WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "sample is null");
    FALSE_RETURN_V_MSG_E(sample->pts_ >= lastTimestampUs_, Status::ERROR_INVALID_PARAMETER,
        "pts: " PUBLIC_LOG_D64 " error, < " PUBLIC_LOG_D64, sample->pts_, lastTimestampUs_);
    FALSE_RETURN_V_MSG_E(io != nullptr, Status::ERROR_INVALID_OPERATION, "io is null");
    FALSE_RETURN_V_MSG_E(stsz_ != nullptr, Status::ERROR_INVALID_OPERATION, "stsz box is empty");
    int64_t pos = io->GetPos();
    io->Write(sample->memory_->GetAddr(), sample->memory_->GetSize());
    if (stsz_->sampleCount_ == 0) {
        startTimestampUs_ = sample->pts_;
        MEDIA_LOG_I("audio track:" PUBLIC_LOG_D32 ", start timestamp:" PUBLIC_LOG_D64, trackId_, startTimestampUs_);
    } else { // 先写上1帧的时间，最后少一帧stts数据，写tailer前添加
        lastDuration_ = std::max(static_cast<int64_t>(frameSize_),
            ConvertTimeToMpeg4(sample->pts_ - lastTimestampUs_, timeScale_));
        DisposeStts(lastDuration_, sample->pts_);
    }
    DisposeStco(pos);
    DisposeStsz(sample->memory_->GetSize());
    lastTimestampUs_ = sample->pts_;
    pos_ = io->GetPos();
    return Status::NO_ERROR;
}

Status AudioTrack::WriteTailer()
{
    FALSE_RETURN_V_MSG_E(stsz_ != nullptr, Status::ERROR_INVALID_OPERATION, "stsz box is empty");
    if (stsz_->sampleCount_ > 0) {
        lastDuration_ = std::max(static_cast<int64_t>(frameSize_), lastDuration_);
        DisposeStts(lastDuration_, lastTimestampUs_);
        DisposeDuration();
        DisposeBitrate();
    }
    return Status::NO_ERROR;
}

int32_t AudioTrack::GetSampleRate()
{
    return sampleRate_;
}

int32_t AudioTrack::GetChannels()
{
    return channels_;
}

void AudioTrack::DisposeStts(int64_t duration, int64_t pts)
{
    FALSE_RETURN_MSG(stts_ != nullptr, "stts box is empty");
    if (duration > INT32_MAX || duration < 0) {
        MEDIA_LOG_E("stts sample_delta must <= INT32_MAX and >= 0, but the value is " PUBLIC_LOG_D64 ". "
            "the pts is " PUBLIC_LOG_D64, duration, pts);
    }
    size_t size = stts_->timeToSamples_.size();
    if (size > 0 && stts_->timeToSamples_[size - 1].second == static_cast<uint32_t>(duration)) {
        stts_->timeToSamples_[size - 1].first++;
    } else {
        stts_->entryCount_++;
        stts_->timeToSamples_.emplace_back(std::pair<uint32_t, uint32_t>(1, static_cast<uint32_t>(duration)));
    }
}

void AudioTrack::DisposeStco(int64_t pos)
{
    constexpr size_t num2 = 2;
    FALSE_RETURN_MSG(stsc_ != nullptr, "stsc box is empty");
    FALSE_RETURN_MSG(stco_ != nullptr, "stco box is empty");
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

void AudioTrack::DisposeStsz(int32_t size)
{
    FALSE_RETURN_MSG(stsz_ != nullptr, "stsz box is empty");
    allSampleSize_ += size;
    if (isSameSize_ && stsz_->sampleCount_ > 0 &&
        stsz_->samples_[stsz_->sampleCount_ - 1] != size) {
        isSameSize_ = false;
    }
    stsz_->sampleCount_++;
    stsz_->samples_.emplace_back(size);
}

void AudioTrack::DisposeDuration()
{
    durationUs_ = lastTimestampUs_ - startTimestampUs_ + ConvertTimeFromMpeg4(lastDuration_, timeScale_);
    auto mvhdBox = BasicBox::GetBoxPtr<MvhdBox>(moov_, "mvhd");
    FALSE_RETURN_MSG(mvhdBox != nullptr, "mvhd box is empty");

    auto mdhdBox = BasicBox::GetBoxPtr<MdhdBox>(moov_, trackPath_ + ".mdia.mdhd");
    FALSE_RETURN_MSG(mdhdBox != nullptr, "mdhd box is empty");
    mdhdBox->duration_ = ConvertTimeToMpeg4(durationUs_, timeScale_);

    auto tkhdBox = BasicBox::GetBoxPtr<TkhdBox>(moov_, trackPath_ + ".tkhd");
    FALSE_RETURN_MSG(tkhdBox != nullptr, "tkhd box is empty");
    tkhdBox->duration_ = ConvertTimeToMpeg4(durationUs_ + startTimestampUs_, mvhdBox->timeScale_, RoundingType::UP);
    mvhdBox->duration_ = std::max(mvhdBox->duration_, tkhdBox->duration_);

    int64_t delay = ConvertTimeToMpeg4(startTimestampUs_, mvhdBox->timeScale_, RoundingType::DOWN);
    int64_t duration = ConvertTimeToMpeg4(durationUs_, mvhdBox->timeScale_, RoundingType::UP);
    auto elstBox = BasicBox::GetBoxPtr<ElstBox>(moov_, trackPath_ + ".edts.elst");
    FALSE_RETURN_MSG(elstBox != nullptr, "elst box is empty");
    elstBox->SetVersion(duration < INT32_MAX && delay < INT32_MAX ? 0 : 1);
    elstBox->entryCount_ = 1;
    ElstBox::Data data;
    data.segmentDuration_ = duration;
    data.mediaTime_ = 0;
    if (delay > 0) {
        elstBox->entryCount_ += 1;
        ElstBox::Data data0;
        data0.segmentDuration_ = delay;
        data0.mediaTime_ = -1;
        elstBox->data_.emplace_back(data0);
    } else {
        data.segmentDuration_ += delay;
        int64_t mediaTime = ConvertTimeToMpeg4(startTimestampUs_, timeScale_, RoundingType::DOWN);
        data.mediaTime_ = -std::min(static_cast<int64_t>(0), mediaTime);
    }
    elstBox->data_.emplace_back(data);
}

void AudioTrack::DisposeBitrate()
{
    FALSE_RETURN_MSG(stsz_ != nullptr, "stsz box is empty");
    if (isSameSize_) {
        stsz_->samples_.clear();
        stsz_->sampleSize_ = 0;
    }
    if (durationUs_ > 0 && !codingType_.empty()) {
        int64_t bitRate = allSampleSize_ * 8 * timeScale_ / ConvertTimeToMpeg4(durationUs_, timeScale_); // 1B->8b
        auto esdsBox = BasicBox::GetBoxPtr<EsdsBox>(
            moov_, trackPath_ + ".mdia.minf.stbl.stsd." + codingType_ + ".esds");
        FALSE_RETURN_MSG(esdsBox != nullptr, "esds box is empty");
        esdsBox->avgBitrate_ = static_cast<uint32_t>(bitRate);
        esdsBox->maxBitrate_ = static_cast<uint32_t>(std::max(bitRate, bitRate_));

        auto btrtBox = BasicBox::GetBoxPtr<BtrtBox>(
            moov_, trackPath_ + ".mdia.minf.stbl.stsd." + codingType_ + ".btrt");
        if (btrtBox != nullptr) {
            btrtBox->avgBitrate_ = esdsBox->avgBitrate_;
            btrtBox->maxBitrate_ = esdsBox->maxBitrate_;
        }
    }
    if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC) == 0) {
        auto sbgpBox = BasicBox::GetBoxPtr<SbgpBox>(moov_, trackPath_ + ".mdia.minf.stbl.sbgp");
        FALSE_RETURN_MSG(sbgpBox != nullptr, "sbgp box is empty");
        sbgpBox->descriptions_[0].first = stsz_->sampleCount_;
    }
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS