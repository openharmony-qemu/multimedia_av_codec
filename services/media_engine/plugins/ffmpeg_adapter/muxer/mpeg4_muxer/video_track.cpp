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

#include "video_track.h"
#include "av_common.h"
#include "avcodec_common.h"
#include "avcodec_mime_type.h"
#include "mpeg4_utils.h"
#include "avc_parser.h"
#include "hevc_parser.h"

#ifndef _WIN32
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "VideoTrack" };
}
#endif // !_WIN32

using namespace OHOS::MediaAVCodec;
namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
VideoTrack::VideoTrack(std::string mimeType, std::shared_ptr<BasicBox> moov)
    : BasicTrack(std::move(mimeType), moov)
{
}

VideoTrack::~VideoTrack()
{
}

int32_t VideoTrack::CreateParser()
{
    // before add video box (BoxParser::AddTrakBox)
    if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC) == 0) {
        auto tmpParser = std::make_shared<AvcParser>();
        videoParser_ = tmpParser;
        MEDIA_LOG_I("VideoTrack create AvcParser");
    } else if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) == 0) {
        auto tmpParser = std::make_shared<HevcParser>();
        FALSE_RETURN_V_MSG_E(tmpParser->Init() == 0, -1, "VideoTrack create h265 parser failed!");
        videoParser_ = tmpParser;
        MEDIA_LOG_I("VideoTrack create HevcParser");
    } else {
        videoParser_ = std::make_shared<VideoParser>(VideoParser::VideoParserStreamType::MPEG4);
        MEDIA_LOG_I("VideoTrack create VideoParser");
    }
    return 0;
}

Status VideoTrack::Init(const std::shared_ptr<Meta> &trackDesc)
{
    FALSE_RETURN_V_MSG_E(CreateParser() == 0, Status::ERROR_INVALID_DATA,
        "create video parser failed! mimetype:%{public}s", mimeType_.c_str());
    constexpr int32_t maxLength = 65535;
    constexpr int32_t videoTimeScale = 90000;
    mediaType_ = OHOS::MediaAVCodec::MEDIA_TYPE_VID;
    bool ret = trackDesc->Get<Tag::VIDEO_WIDTH>(width_); // width
    FALSE_RETURN_V_MSG_E((ret && width_ > 0 && width_ <= maxLength), Status::ERROR_INVALID_PARAMETER,
        "get video width failed! width:%{public}d", width_);
    ret = trackDesc->Get<Tag::VIDEO_HEIGHT>(height_); // height
    FALSE_RETURN_V_MSG_E((ret && height_ > 0 && height_ <= maxLength), Status::ERROR_INVALID_PARAMETER,
        "get video height failed! height:%{public}d", height_);
    if (trackDesc->Find(Tag::VIDEO_FRAME_RATE) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_FRAME_RATE>(frameRate_);
        FALSE_RETURN_V_MSG_E(frameRate_ > 0, Status::ERROR_MISMATCHED_TYPE, "err frame rate %{public}lf", frameRate_);
        MEDIA_LOG_I("video track:%{public}d, frame rate:%{public}lf", trackId_, frameRate_);
    }

    if (trackDesc->Find(Tag::MEDIA_BITRATE) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_BITRATE>(bitRate_); // bit rate
        MEDIA_LOG_I("video track:%{public}d, bit rate:" PUBLIC_LOG_D64, trackId_, bitRate_);
    }
    
    InitColor(trackDesc);
    InitCuva(trackDesc);
    codecConfig_.clear();
    if (trackDesc->Find(Tag::MEDIA_CODEC_CONFIG) != trackDesc->end()) { // codec config
        trackDesc->Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig_); // codec config
        MEDIA_LOG_I("video track:%{public}d, codec config len:%{public}zu", trackId_, codecConfig_.size());
    }
    timeScale_ = videoTimeScale;
    if (frameRate_ >= 1) {
        lastDuration_ = ConvertTime(1, static_cast<int32_t>(frameRate_), timeScale_);
    }
    return Status::NO_ERROR;
}

void VideoTrack::ParserSetConfig()
{
    // after add video box (BoxParser::AddTrakBox)
    if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_AVC) == 0) {
        auto box = moov_->GetChild(trackPath_ + ".mdia.minf.stbl.stsd.avc1.avcC");
        videoParser_->SetConfig(box, codecConfig_);
    } else if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) == 0) {
        auto box = moov_->GetChild(trackPath_ + ".mdia.minf.stbl.stsd.hvc1.hvcC");
        videoParser_->SetConfig(box, codecConfig_);
    }
    hasSetParserConfig_ = true;
    return;
}

void VideoTrack::SetRotation(uint32_t rotation)
{
    FALSE_RETURN_MSG(moov_ != nullptr, "moov box is empty");
    auto tkhdBox = BasicBox::GetBoxPtr<TkhdBox>(moov_, trackPath_ + ".tkhd");
    FALSE_RETURN_MSG(tkhdBox != nullptr, "tkhd box is empty");
    if (rotation == VIDEO_ROTATION_90) {
        tkhdBox->matrix_[0][0] = 0;
        tkhdBox->matrix_[0][1] = 0x00010000;
        tkhdBox->matrix_[1][0] = 0xFFFF0000;
        tkhdBox->matrix_[1][1] = 0;
    } else if (rotation == VIDEO_ROTATION_180) {
        tkhdBox->matrix_[0][0] = 0xFFFF0000;
        tkhdBox->matrix_[1][1] = 0xFFFF0000;
    } else if (rotation == VIDEO_ROTATION_270) {
        tkhdBox->matrix_[0][0] = 0;
        tkhdBox->matrix_[0][1] = 0xFFFF0000;
        tkhdBox->matrix_[1][0] = 0x00010000;
        tkhdBox->matrix_[1][1] = 0;
    }
}

Status VideoTrack::WriteSample(std::shared_ptr<AVIOStream> io, const std::shared_ptr<AVBuffer> &sample)
{
    FALSE_RETURN_V_MSG_E(sample != nullptr && sample->memory_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "sample is null");
    FALSE_RETURN_V_MSG_E(io != nullptr, Status::ERROR_INVALID_OPERATION, "io is null");
    FALSE_RETURN_V_MSG_E(stss_ != nullptr && stsz_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "stss or stsz box is empty");
    FALSE_RETURN_V_MSG_E(videoParser_ != nullptr, Status::ERROR_INVALID_OPERATION, "videoParser is null");
    if (!hasSetParserConfig_) {
        ParserSetConfig();
    }
    int64_t pos = io->GetPos();
    int32_t size = videoParser_->WriteFrame(io, sample);
    FALSE_RETURN_V_MSG_E(size > 0, (size == 0 ? Status::NO_ERROR : Status::ERROR_INVALID_DATA),
        "video write frame err:%{public}d", size);
    DisposeCtts(sample->pts_);
    DisposeStco(pos);
    DisposeStsz(static_cast<uint32_t>(size));
    if (sample->flag_ & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) {
        stss_->syncCount_++;
        stss_->syncIndex_.emplace_back(stsz_->sampleCount_);
    }
    pos_ = io->GetPos();
    return Status::NO_ERROR;
}

Status VideoTrack::WriteTailer()
{
    FALSE_RETURN_V_MSG_E(stsz_ != nullptr, Status::ERROR_INVALID_OPERATION, "stsz box is empty");
    if (stsz_->sampleCount_ > 0) {
        DisposeCtts();
        DisposeStts();
        DisposeDuration();
        DisposeBitrate();
        DisposeColor();
        DisposeCuva();
    }
    return Status::NO_ERROR;
}

int32_t VideoTrack::GetWidth()
{
    return width_;
}

int32_t VideoTrack::GetHeight()
{
    return height_;
}

bool VideoTrack::IsColor()
{
    return isColor_;
}

ColorPrimary VideoTrack::GetColorPrimaries()
{
    return colorPrimaries_;
}

TransferCharacteristic VideoTrack::GetColorTransfer()
{
    return colorTransfer_;
}

MatrixCoefficient VideoTrack::GetColorMatrixCoeff()
{
    return colorMatrixCoeff_;
}

bool VideoTrack::GetColorRange()
{
    return colorRange_;
}

bool VideoTrack::IsCuvaHDR()
{
    return isCuvaHDR_;
}

void VideoTrack::SetCttsBox(std::shared_ptr<SttsBox> ctts)
{
    ctts_ = ctts;
}

void VideoTrack::SetStssBox(std::shared_ptr<StssBox> stss)
{
    stss_ = stss;
}

void VideoTrack::InitColor(const std::shared_ptr<Meta> &trackDesc)
{
    isColor_ = false;
    if (trackDesc->Find(Tag::VIDEO_COLOR_PRIMARIES) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_TRC) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_MATRIX_COEFF) != trackDesc->end() ||
        trackDesc->Find(Tag::VIDEO_COLOR_RANGE) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_COLOR_PRIMARIES>(colorPrimaries_);
        trackDesc->Get<Tag::VIDEO_COLOR_TRC>(colorTransfer_);
        trackDesc->Get<Tag::VIDEO_COLOR_MATRIX_COEFF>(colorMatrixCoeff_);
        trackDesc->Get<Tag::VIDEO_COLOR_RANGE>(colorRange_);
        MEDIA_LOG_D("color info.: primary %{public}d, transfer %{public}d, matrix coeff %{public}d,"
            " range %{public}d,", static_cast<int32_t>(colorPrimaries_), static_cast<int32_t>(colorTransfer_),
            static_cast<int32_t>(colorMatrixCoeff_), static_cast<int32_t>(colorRange_));
        if (colorPrimaries_ != ColorPrimary::UNSPECIFIED &&
            colorTransfer_ != TransferCharacteristic::UNSPECIFIED &&
            colorMatrixCoeff_ != MatrixCoefficient::UNSPECIFIED) {
            isColor_ = true;
        }
    }
}

void VideoTrack::InitCuva(const std::shared_ptr<Meta> &trackDesc)
{
    isCuvaHDR_ = false;
    if (trackDesc->Find(Tag::VIDEO_IS_HDR_VIVID) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_IS_HDR_VIVID>(isCuvaHDR_);
        MEDIA_LOG_D("hdr type: %{public}d", isCuvaHDR_);
    }
}

void VideoTrack::DisposeCtts(int64_t pts)
{
    FALSE_RETURN_MSG(stsz_ != nullptr, "stsz box is empty");
    if (stsz_->sampleCount_ == 0) {
        startTimestampUs_ = pts;
        lastTimestampUs_ = pts;
        startDts_ = ConvertTimeToMpeg4(pts, timeScale_);
        dts_ = startDts_;
        delay_ = 0;
        MEDIA_LOG_I("video track:%{public}d, start timestamp:" PUBLIC_LOG_D64, trackId_, pts);
    } else {
        startTimestampUs_ = std::min(startTimestampUs_, pts);
        lastTimestampUs_ = std::max(lastTimestampUs_, pts);
        if (lastTimestampUs_ != pts) {
            isHaveBFrame_ = true;
        }
    }
    if (lastDuration_ == 0) {
        allPts_.emplace(pts);
    } else {
        DisposeCttsByFrameRate(pts);
    }
}

void VideoTrack::DisposeCtts()
{
    FALSE_RETURN_MSG(stsz_ != nullptr, "stsz box is empty");
    FALSE_RETURN_MSG(ctts_ != nullptr, "ctts box is empty");
    if (lastDuration_ == 0 && stsz_->sampleCount_ > 1) {
        lastDuration_ = ConvertTime(ConvertTimeToMpeg4(lastTimestampUs_ - startTimestampUs_, timeScale_),
            stsz_->sampleCount_ - 1, 1);
    }
    if (isHaveBFrame_) {
        while (allPts_.size() > 0) {
            auto& pts = allPts_.front();
            DisposeCttsByFrameRate(pts);
            allPts_.pop();
        }
        for (auto& ctts : ctts_->timeToSamples_) {
            int64_t tmpNum = static_cast<int64_t>(static_cast<int32_t>(ctts.second)) - delay_;
            if (tmpNum < INT32_MIN) {
                tmpNum = INT32_MIN;
            } else if (tmpNum > INT32_MAX) {
                tmpNum = INT32_MAX;
            }
            ctts.second = static_cast<uint32_t>(tmpNum);
        }
        delay_ += startDts_;
        if (ctts_->entryCount_ == 1) {
            ctts_->entryCount_ = 0;
            ctts_->timeToSamples_.clear();
        }
    }
}

void VideoTrack::DisposeCttsByFrameRate(int64_t pts)
{
    FALSE_RETURN_MSG(ctts_ != nullptr, "ctts box is empty");
    int64_t cts = ConvertTimeToMpeg4(pts, timeScale_) - dts_;
    if (cts > INT32_MAX || cts < INT32_MIN) {
        MEDIA_LOG_E("ctts sample_offset must <= INT32_MAX and >= INT32_MIN, but the value is " PUBLIC_LOG_D64 ", "
            "the pts is " PUBLIC_LOG_D64, cts, pts);
    }
    delay_ = std::min(cts, delay_);
    size_t size = ctts_->timeToSamples_.size();
    if (size > 0 && ctts_->timeToSamples_[size - 1].second == static_cast<uint32_t>(cts)) {
        ctts_->timeToSamples_[size - 1].first++;
    } else {
        ctts_->entryCount_++;
        ctts_->timeToSamples_.emplace_back(std::pair<uint32_t, uint32_t>(1, static_cast<uint32_t>(cts)));
    }
    dts_ += lastDuration_;
}

void VideoTrack::DisposeSttsNoPts()
{
    int64_t dts = 0;
    int64_t lastTimestamp = 0;
    bool isFirst = true;
    while (ctts_->timeToSamples_.size() > 0) {
        auto timeToSample = ctts_->timeToSamples_.front();
        ctts_->timeToSamples_.pop_front();
        ctts_->entryCount_--;
        for (uint32_t j = 0; j < timeToSample.first; ++j) {
            if (isFirst) {
                isFirst = false;
                dts = startDts_;
                lastTimestamp = dts + static_cast<int32_t>(timeToSample.second);
                delay_ = lastTimestamp;
            } else {
                dts += lastDuration_;
                int64_t pts = dts + static_cast<int32_t>(timeToSample.second);
                int64_t duration = pts - lastTimestamp;
                lastTimestamp = pts;
                DisposeStts(duration, lastTimestamp);
            }
        }
    }
    DisposeStts(lastDuration_, lastTimestamp);
}

void VideoTrack::DisposeStts()
{
    FALSE_RETURN_MSG(stts_ != nullptr && stsz_ != nullptr, "stts or stsz box is empty");
    if (isHaveBFrame_) {
        stts_->entryCount_ = 1;
        stts_->timeToSamples_.emplace_back(std::pair<uint32_t, uint32_t>(stsz_->sampleCount_,
            static_cast<uint32_t>(lastDuration_)));
    } else if (allPts_.size() > 0) {
        int64_t lastTimestampUs = allPts_.front();
        allPts_.pop();
        while (allPts_.size() > 0) {
            int64_t pts = allPts_.front();
            allPts_.pop();
            int64_t duration = ConvertTimeToMpeg4(pts - lastTimestampUs, timeScale_);
            lastTimestampUs = pts;
            DisposeStts(duration, lastTimestampUs);
        }
        DisposeStts(lastDuration_, lastTimestampUs);
    } else {
        DisposeSttsNoPts();
    }
}

void VideoTrack::DisposeStts(int64_t duration, int64_t pts)
{
    if (duration > INT32_MAX || duration < 0) {
        MEDIA_LOG_E("stts sample_delta must <= INT32_MAX and >= 0, but the value is " PUBLIC_LOG_D64
            ", the pts is " PUBLIC_LOG_D64, duration, pts);
    }
    size_t size = stts_->timeToSamples_.size();
    if (size > 0 && stts_->timeToSamples_[size - 1].second == static_cast<uint32_t>(duration)) {
        stts_->timeToSamples_[size - 1].first++;
    } else {
        stts_->entryCount_++;
        stts_->timeToSamples_.emplace_back(std::pair<uint32_t, uint32_t>(1, static_cast<uint32_t>(duration)));
    }
}

void VideoTrack::DisposeStco(int64_t pos)
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

void VideoTrack::DisposeStsz(uint32_t size)
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

void VideoTrack::DisposeDuration()
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
        delay_ -= ConvertTimeToMpeg4(startTimestampUs_, timeScale_);
    } else {
        data.segmentDuration_ += delay;
    }
    data.mediaTime_ = -std::min(static_cast<int64_t>(0), delay_);
    elstBox->data_.emplace_back(data);
}

void VideoTrack::DisposeBitrate()
{
    FALSE_RETURN_MSG(stsz_ != nullptr, "stsz box is empty");
    if (isSameSize_) {
        stsz_->samples_.clear();
        stsz_->sampleSize_ = stsz_->samples_.size() > 0 ? stsz_->samples_[0] : 0;
    }
    if (durationUs_ > 0 && !codingType_.empty()) {
        int64_t bitRate = allSampleSize_ * 8 * timeScale_ / ConvertTimeToMpeg4(durationUs_, timeScale_);  // 8
        auto esdsBox = BasicBox::GetBoxPtr<EsdsBox>(
            moov_, trackPath_ + ".mdia.minf.stbl.stsd." + codingType_ + ".esds"); // mp4v
        if (esdsBox != nullptr) {
            esdsBox->avgBitrate_ = static_cast<uint32_t>(bitRate);
            esdsBox->maxBitrate_ = static_cast<uint32_t>(std::max(bitRate, bitRate_));
        }

        auto btrtBox = BasicBox::GetBoxPtr<BtrtBox>(
            moov_, trackPath_ + ".mdia.minf.stbl.stsd." + codingType_ + ".btrt"); // mp4v, avc1, hvc1
        if (btrtBox != nullptr) {
            btrtBox->avgBitrate_ = static_cast<uint32_t>(bitRate);
            btrtBox->maxBitrate_ = static_cast<uint32_t>(std::max(bitRate, bitRate_));
        }
    }
}

void VideoTrack::DisposeColor()
{
    if (videoParser_ != nullptr && mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) == 0) {
        HevcParser* parser = VideoParser::GetVideoParserPtr<HevcParser>(videoParser_);
        if (parser != nullptr) {
            if (parser->IsParserColor()) {
                colorPrimaries_ = static_cast<ColorPrimary>(parser->GetColorPrimary());
                colorTransfer_ = static_cast<TransferCharacteristic>(parser->GetColorTransfer());
                colorMatrixCoeff_ = static_cast<MatrixCoefficient>(parser->GetColorMatrixCoeff());
                colorRange_ = parser->GetColorRange();
            }
            MEDIA_LOG_I("color info. isParser:%{public}d: primary %{public}d, transfer %{public}d, "
                "matrix coeff %{public}d, range %{public}d,", static_cast<int32_t>(parser->IsParserColor()),
                static_cast<int32_t>(colorPrimaries_), static_cast<int32_t>(colorTransfer_),
                static_cast<int32_t>(colorMatrixCoeff_), static_cast<int32_t>(colorRange_));
            if (colorPrimaries_ != ColorPrimary::UNSPECIFIED &&
                colorTransfer_ != TransferCharacteristic::UNSPECIFIED &&
                colorMatrixCoeff_ != MatrixCoefficient::UNSPECIFIED) {
                isColor_ = true;
            }
        }
    }
    if (isColor_ && !codingType_.empty()) {
        auto videoBox = moov_->GetChild(trackPath_ + ".mdia.minf.stbl.stsd." + codingType_);
        FALSE_RETURN_MSG(videoBox != nullptr, "%{public}s box is empty", codingType_.c_str());
        std::shared_ptr<ColrBox> colrBoxSharedPtr = nullptr;
        auto colrBox = BasicBox::GetBoxPtr<ColrBox>(videoBox, "colr");
        if (colrBox == nullptr) {
            colrBoxSharedPtr = std::make_shared<ColrBox>(0, "colr");
            videoBox->AddChild(colrBoxSharedPtr);
            colrBox = colrBoxSharedPtr.get();
        }
        colrBox->colorPrimaries_ = static_cast<uint16_t>(colorPrimaries_);
        colrBox->colorTransfer_ = static_cast<uint16_t>(colorTransfer_);
        colrBox->colorMatrixCoeff_ = static_cast<uint16_t>(colorMatrixCoeff_);
        colrBox->colorRange_ = colorRange_ ? 0x80 : 0x00;
    }
}

void VideoTrack::DisposeCuva()
{
    // 需要通过buffer或设置获取hdr vivid
    if (isCuvaHDR_ && mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) == 0) {
        // hvc1
        auto videoBox = BasicBox::GetBoxPtr<VideoBox>(moov_, trackPath_ + ".mdia.minf.stbl.stsd." + codingType_);
        FALSE_RETURN_MSG(videoBox != nullptr, "%{public}s box is empty", codingType_.c_str());
        auto box = videoBox->GetChild("cuvv");
        if (box == nullptr) {
            std::string name = "CUVA HDR Video";
            videoBox->compressorLen_ = static_cast<uint8_t>(name.length());
            (void)memcpy_s(videoBox->compressor_, sizeof(videoBox->compressor_), name.data(), name.length());
            videoBox->AddChild(std::make_shared<CuvvBox>(0, "cuvv"));
        }
    }
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS