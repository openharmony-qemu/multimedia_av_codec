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
#include "avcodec_common.h"
#include "avcodec_mime_type.h"
#include "mpeg4_utils.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "AudioTrack" };
constexpr int32_t MIN_HE_AAC_SAMPLE_RATE = 16000;
}
#endif // !_WIN32

using namespace OHOS::MediaAVCodec;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
AudioTrack::AudioTrack(std::string mimeType, std::shared_ptr<BasicBox> moov,
    std::vector<std::shared_ptr<BasicTrack>> &tracks) : BasicTrack(std::move(mimeType), moov), tracks_(tracks)
{
    hdlrType_ = 'soun';
    hdlrName_ = "SoundHandler";
}

AudioTrack::~AudioTrack()
{
}

Status AudioTrack::Init(const std::shared_ptr<Meta> &trackDesc)
{
    mediaType_ = MediaType::AUDIO;
    bool ret = trackDesc->Get<Tag::AUDIO_SAMPLE_RATE>(sampleRate_); // sample rate
    FALSE_RETURN_V_MSG_E((ret && sampleRate_ > 0), Status::ERROR_MISMATCHED_TYPE,
        "get audio sample_rate failed! sampleRate:%{public}d", sampleRate_);
    ret = trackDesc->Get<Tag::AUDIO_CHANNEL_COUNT>(channels_); // channels
    FALSE_RETURN_V_MSG_E((ret && channels_ > 0), Status::ERROR_MISMATCHED_TYPE,
        "get audio channels failed! channels:%{public}d", channels_);
    if (trackDesc->Find(Tag::AUDIO_SAMPLE_PER_FRAME) != trackDesc->end()) {
        trackDesc->Get<Tag::AUDIO_SAMPLE_PER_FRAME>(frameSize_);  // frame size
        MEDIA_LOG_I("audio track:%{public}s, frame size:%{public}d", mimeType_.c_str(), frameSize_);
    }
    if (trackDesc->Find(Tag::MEDIA_BITRATE) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_BITRATE>(bitRate_); // bit rate
    }
    codecConfig_.clear();
    if (trackDesc->Find(Tag::MEDIA_CODEC_CONFIG) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig_); // codec config
        MEDIA_LOG_I("audio track:%{public}s, codec config len:%{public}zu", mimeType_.c_str(), codecConfig_.size());
    }
    if (mimeType_ == MimeType::AUDIO_AAC && codecConfig_.empty()) {
        int32_t profile = 0;
        if (trackDesc->GetData(Tag::MEDIA_PROFILE, profile)) {
            if ((profile == AAC_PROFILE_HE || profile == AAC_PROFILE_HE_V2) &&
                sampleRate_ < MIN_HE_AAC_SAMPLE_RATE) {
                MEDIA_LOG_E("HE-AAC only support sample rate >= 16k, input rate:%{public}d", sampleRate_);
                return Status::ERROR_INVALID_PARAMETER;
            }
            codecConfig_ = GenerateAACCodecConfig(profile, sampleRate_, channels_);
            MEDIA_LOG_I("audio generate aac  codec config len:%{public}zu", codecConfig_.size());
        } else {
            MEDIA_LOG_W("missing codec config of aac!");
        }
    }
    timeScale_ = sampleRate_;

    Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
    trackDesc->GetData(Tag::MEDIA_TYPE, mediaType);
    if (mediaType == Plugins::MediaType::AUXILIARY) {
        // 初始化辅助轨独有参数
        FALSE_RETURN_V_MSG_E(GetSrcTrackIds(trackDesc), Status::ERROR_INVALID_PARAMETER, "get src track ids failed!");
        return SetAuxiliaryTrackParam(trackDesc);
    }
    return Status::NO_ERROR;
}

bool AudioTrack::GetSrcTrackIds(const std::shared_ptr<Meta> &trackDesc)
{
    std::vector<int32_t> trackIndexs;
    // get reference track ids
    FALSE_RETURN_V_MSG_E(trackDesc->GetData(Tag::REFERENCE_TRACK_IDS, trackIndexs), false, "track ids not set!");
    srcTrackIds_ = std::make_shared<std::vector<uint32_t>>();
    for (auto i : trackIndexs) {
        if (i >= 0 && i < static_cast<int32_t>(tracks_.size()) && tracks_[i]->GetTrackId() >= 0) {
            srcTrackIds_->push_back(static_cast<uint32_t>(tracks_[i]->GetTrackId()));
        } else {
            MEDIA_LOG_W("audio aux unsupport track index:%{public}d, track size:%{public}zu",
                i, tracks_.size());
        }
    }
    FALSE_RETURN_V_MSG_E(srcTrackIds_->size() > 0, false, "audio track ids get failed!");
    return true;
}

int32_t AudioTrack::GetAacAdtsSize(const uint8_t *data, int32_t len)
{
    constexpr int32_t adtsMinLen = 7;
    constexpr int32_t adtsMaxLen = 9;
    if (len < adtsMinLen || data[0] != 0xff || (data[1] & 0xf0) != 0xf0) {
        return 0;
    }
    int32_t skipBytes = (data[1] & 0x01) == 0x01 ? adtsMinLen : adtsMaxLen;  // adts head len is 7 or 9
    return len >= skipBytes ? skipBytes : 0;
}

Status AudioTrack::WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr,
        Status::ERROR_NULL_POINTER, "sample is null");
    FALSE_RETURN_V_MSG_E(sample->pts_ >= lastTimestampUs_, Status::ERROR_INVALID_PARAMETER,
        "pts: " PUBLIC_LOG_D64 " error, < " PUBLIC_LOG_D64, sample->pts_, lastTimestampUs_);
    FALSE_RETURN_V_MSG_E(io != nullptr, Status::ERROR_INVALID_OPERATION, "io is null");
    FALSE_RETURN_V_MSG_E(stsz_ != nullptr, Status::ERROR_INVALID_OPERATION, "stsz box is empty");
    int32_t skipBytes = 0;
    if (mimeType_ == MimeType::AUDIO_AAC) {
        skipBytes = GetAacAdtsSize(sample->memory_->GetAddr(), sample->memory_->GetSize());
    }
    int32_t writeSize = sample->memory_->GetSize() - skipBytes;
    int64_t pos = io->GetPos();
    io->Write(sample->memory_->GetAddr() + skipBytes, writeSize);
    if (stsz_->sampleCount_ == 0) {
        startTimestampUs_ = sample->pts_;
        MEDIA_LOG_I("audio track:" PUBLIC_LOG_D32 ", start timestamp:" PUBLIC_LOG_D64, trackId_, startTimestampUs_);
    } else { // 先写上1帧的时间，最后少一帧stts数据，写tailer前添加
        lastDuration_ = std::max(static_cast<int64_t>(frameSize_),
            ConvertTimeToMpeg4(sample->pts_ - lastTimestampUs_, timeScale_));
        DisposeStts(lastDuration_, sample->pts_);
    }
    DisposeStco(pos);
    DisposeStsz(writeSize);
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

int32_t AudioTrack::GetSampleRate() const
{
    return sampleRate_;
}

int32_t AudioTrack::GetChannels() const
{
    return channels_;
}

void AudioTrack::DisposeDuration()
{
    durationUs_ = lastTimestampUs_ - startTimestampUs_ + ConvertTimeFromMpeg4(lastDuration_, timeScale_);
    auto mvhdBox = BasicBox::GetBoxPtr<MvhdBox>(moov_, "mvhd");
    FALSE_RETURN_MSG(mvhdBox != nullptr, "mvhd box is empty");

    auto mdhdBox = BasicBox::GetBoxPtr<MdhdBox>(moov_, trackPath_ + ".mdia.mdhd");
    FALSE_RETURN_MSG(mdhdBox != nullptr, "mdhd box is empty");
    mdhdBox->duration_ = static_cast<uint64_t>(ConvertTimeToMpeg4(durationUs_, timeScale_));

    auto tkhdBox = BasicBox::GetBoxPtr<TkhdBox>(moov_, trackPath_ + ".tkhd");
    FALSE_RETURN_MSG(tkhdBox != nullptr, "tkhd box is empty");
    tkhdBox->duration_ = static_cast<uint64_t>(
        ConvertTimeToMpeg4(durationUs_ + startTimestampUs_, mvhdBox->timeScale_, RoundingType::UP));
    mvhdBox->duration_ = std::max(mvhdBox->duration_, tkhdBox->duration_);

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
        data0.mediaTime_ = -1;
        elstBox->data_.emplace_back(data0);
    } else {
        data.segmentDuration_ = static_cast<uint64_t>(delay + duration);
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
        int64_t bitRate = static_cast<int64_t>(allSampleSize_) *
            8 * timeScale_ / ConvertTimeToMpeg4(durationUs_, timeScale_); // 1B->8b
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