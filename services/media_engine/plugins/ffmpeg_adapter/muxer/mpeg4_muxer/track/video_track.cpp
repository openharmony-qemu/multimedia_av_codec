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
#include <set>
#include <iomanip>
#include <sstream>
#include "avcodec_common.h"
#include "avcodec_mime_type.h"
#include "mpeg4_utils.h"
#include "avc_parser.h"
#include "hevc_parser.h"

using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_MUXER, "VideoTrack" };
constexpr uint8_t FRAME_DEPENDENCY_UNKNOWN = 0x00;
constexpr uint8_t FRAME_DEPENDENCY_YES = 0x01;
constexpr uint8_t FRAME_DEPENDENCY_NO = 0x02;
constexpr uint8_t FRAME_DEPENDENCY_EXT = 0x03;

bool CheckColorPrimary(Plugins::ColorPrimary primary)
{
    static const std::set<Plugins::ColorPrimary> table = {
        Plugins::ColorPrimary::BT709,
        Plugins::ColorPrimary::UNSPECIFIED,
        Plugins::ColorPrimary::BT470_M,
        Plugins::ColorPrimary::BT601_625,
        Plugins::ColorPrimary::BT601_525,
        Plugins::ColorPrimary::SMPTE_ST240,
        Plugins::ColorPrimary::GENERIC_FILM,
        Plugins::ColorPrimary::BT2020,
        Plugins::ColorPrimary::SMPTE_ST428,
        Plugins::ColorPrimary::P3DCI,
        Plugins::ColorPrimary::P3D65,
    };
    auto it = table.find(primary);
    if (it != table.end()) {
        return true;
    }
    return false;
}

bool CheckColorTransfer(Plugins::TransferCharacteristic transfer)
{
    static const std::set<Plugins::TransferCharacteristic> table = {
        Plugins::TransferCharacteristic::BT709,
        Plugins::TransferCharacteristic::UNSPECIFIED,
        Plugins::TransferCharacteristic::GAMMA_2_2,
        Plugins::TransferCharacteristic::GAMMA_2_8,
        Plugins::TransferCharacteristic::BT601,
        Plugins::TransferCharacteristic::SMPTE_ST240,
        Plugins::TransferCharacteristic::LINEAR,
        Plugins::TransferCharacteristic::LOG,
        Plugins::TransferCharacteristic::LOG_SQRT,
        Plugins::TransferCharacteristic::IEC_61966_2_4,
        Plugins::TransferCharacteristic::BT1361,
        Plugins::TransferCharacteristic::IEC_61966_2_1,
        Plugins::TransferCharacteristic::BT2020_10BIT,
        Plugins::TransferCharacteristic::BT2020_12BIT,
        Plugins::TransferCharacteristic::PQ,
        Plugins::TransferCharacteristic::SMPTE_ST428,
        Plugins::TransferCharacteristic::HLG,
    };
    auto it = table.find(transfer);
    if (it != table.end()) {
        return true;
    }
    return false;
}

bool CheckColorMatrix(Plugins::MatrixCoefficient matrix)
{
    static const std::set table = {
        Plugins::MatrixCoefficient::IDENTITY,
        Plugins::MatrixCoefficient::BT709,
        Plugins::MatrixCoefficient::UNSPECIFIED,
        Plugins::MatrixCoefficient::FCC,
        Plugins::MatrixCoefficient::BT601_625,
        Plugins::MatrixCoefficient::BT601_525,
        Plugins::MatrixCoefficient::SMPTE_ST240,
        Plugins::MatrixCoefficient::YCGCO,
        Plugins::MatrixCoefficient::BT2020_NCL,
        Plugins::MatrixCoefficient::BT2020_CL,
        Plugins::MatrixCoefficient::SMPTE_ST2085,
        Plugins::MatrixCoefficient::CHROMATICITY_NCL,
        Plugins::MatrixCoefficient::CHROMATICITY_CL,
        Plugins::MatrixCoefficient::ICTCP,
    };
    auto it = table.find(matrix);
    if (it != table.end()) {
        return true;
    }
    return false;
}
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace Mpeg4 {
VideoTrack::VideoTrack(std::string mimeType, std::shared_ptr<BasicBox> moov,
    std::vector<std::shared_ptr<BasicTrack>> &tracks) : BasicTrack(std::move(mimeType), moov), tracks_(tracks)
{
    hdlrType_ = 'vide';
    hdlrName_ = "VideoHandler";
    tempCtts_ = std::make_shared<SttsBox>(0, "temp");
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
        auto tmpParser = std::make_shared<HevcParser>(hasVideoDelay_);
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
    mediaType_ = MediaType::VIDEO;
    bool ret = trackDesc->Get<Tag::VIDEO_WIDTH>(width_); // width
    FALSE_RETURN_V_MSG_E((ret && width_ > 0 && width_ <= maxLength), Status::ERROR_INVALID_PARAMETER,
        "get video width failed! width:%{public}d", width_);
    ret = trackDesc->Get<Tag::VIDEO_HEIGHT>(height_); // height
    FALSE_RETURN_V_MSG_E((ret && height_ > 0 && height_ <= maxLength), Status::ERROR_INVALID_PARAMETER,
        "get video height failed! height:%{public}d", height_);
    MEDIA_LOG_I("video track width:%{public}d, height:%{public}d", width_, height_);

    if (trackDesc->Find(Tag::VIDEO_FRAME_RATE) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_FRAME_RATE>(frameRate_);
        FALSE_RETURN_V_MSG_E(frameRate_ > 0, Status::ERROR_MISMATCHED_TYPE, "err frame rate %{public}lf", frameRate_);
        MEDIA_LOG_I("video track: frame rate:%{public}lf", frameRate_);
    }

    if (trackDesc->Find(Tag::MEDIA_BITRATE) != trackDesc->end()) {
        trackDesc->Get<Tag::MEDIA_BITRATE>(bitRate_); // bit rate
        MEDIA_LOG_I("video track bit rate:" PUBLIC_LOG_D64, bitRate_);
    }
    
    FALSE_RETURN_V_MSG_E(InitColor(trackDesc), Status::ERROR_INVALID_PARAMETER, "Init color failed!");
    InitCuva(trackDesc);
    codecConfig_.clear();
    if (trackDesc->Find(Tag::MEDIA_CODEC_CONFIG) != trackDesc->end()) { // codec config
        trackDesc->Get<Tag::MEDIA_CODEC_CONFIG>(codecConfig_); // codec config
        MEDIA_LOG_I("video track codec config len:%{public}zu", codecConfig_.size());
    }
    timeScale_ = videoTimeScale;
    if (frameRate_ >= 1) {
        lastDuration_ = ConvertTime(1, static_cast<int32_t>(frameRate_), timeScale_);
    }
    Plugins::MediaType mediaType = Plugins::MediaType::UNKNOWN;
    trackDesc->GetData(Tag::MEDIA_TYPE, mediaType);
    if (mediaType == Plugins::MediaType::AUXILIARY) {
        // 初始化辅助轨独有参数
        FALSE_RETURN_V_MSG_E(GetSrcTrackIds(trackDesc), Status::ERROR_INVALID_PARAMETER, "get src track ids failed!");
        return SetAuxiliaryTrackParam(trackDesc);
    }
    FALSE_RETURN_V_MSG_E(SetVideoDelay(trackDesc) == Status::NO_ERROR,
        Status::ERROR_MISMATCHED_TYPE, "SetVideoDelay failed!");
    return Status::NO_ERROR;
}

Status VideoTrack::SetVideoDelay(const std::shared_ptr<Meta> &trackDesc)
{
    constexpr int32_t maxVideoDelay = 16;
    int32_t videoDelay = 0;
    if (trackDesc->Find(Tag::VIDEO_DELAY) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_DELAY>(videoDelay); // video delay
        FALSE_RETURN_V_MSG_E(videoDelay >= 0, Status::ERROR_MISMATCHED_TYPE,
            "get video delay failed! video delay:%{public}d", videoDelay);
        hasVideoDelay_ = videoDelay > 0 && videoDelay <= maxVideoDelay;  // max 16
        MEDIA_LOG_I("video track get video delay:%{public}d, has:%{public}d",
            videoDelay, static_cast<int32_t>(hasVideoDelay_));
    }
    FALSE_RETURN_V_MSG_E((videoDelay > 0 && videoDelay <= maxVideoDelay && frameRate_ > 0) || videoDelay == 0,
        Status::ERROR_MISMATCHED_TYPE, "If the video delayed, the frame rate is required. "
        "The delay is greater than or equal to 0 and less than or equal to 16.");
    return Status::NO_ERROR;
}

bool VideoTrack::GetSrcTrackIds(const std::shared_ptr<Meta> &trackDesc)
{
    std::vector<int32_t> trackIndexs;
    // get reference track ids
    FALSE_RETURN_V_MSG_E(trackDesc->GetData(Tag::REFERENCE_TRACK_IDS, trackIndexs), false, "track ids not set!");
    srcTrackIds_ = std::make_shared<std::vector<uint32_t>>();
    for (auto i : trackIndexs) {
        if (i >= 0 && i < static_cast<int32_t>(tracks_.size()) && tracks_[i]->GetTrackId() >= 0) {
            srcTrackIds_->push_back(static_cast<uint32_t>(tracks_[i]->GetTrackId()));
        } else {
            MEDIA_LOG_W("video aux unsupport track index:%{public}d, track size:%{public}zu",
                i, tracks_.size());
        }
    }
    FALSE_RETURN_V_MSG_E(srcTrackIds_->size() > 0, false, "video track ids get failed!");
    return true;
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
        Status::ERROR_NULL_POINTER, "sample is null");
    FALSE_RETURN_V_MSG_E(io != nullptr, Status::ERROR_INVALID_OPERATION, "io is null");
    FALSE_RETURN_V_MSG_E(stss_ != nullptr && stsz_ != nullptr,
        Status::ERROR_INVALID_OPERATION, "stss or stsz box is empty");
    FALSE_RETURN_V_MSG_E(videoParser_ != nullptr, Status::ERROR_INVALID_OPERATION, "videoParser is null");
    auto pts = sample->pts_;
    if (!hasVideoDelay_ && sample->flag_ & static_cast<uint32_t>(AVBufferFlag::CODEC_DATA) && pts < lastTimestampUs_) {
        MEDIA_LOG_I("[%{public}d] change video pts:" PUBLIC_LOG_D64 " -> " PUBLIC_LOG_D64,
            trackId_, pts, lastTimestampUs_);
        pts = lastTimestampUs_;
    }
    FALSE_RETURN_V_MSG_E(hasVideoDelay_ || pts >= lastTimestampUs_, Status::ERROR_INVALID_PARAMETER,
        "[%{public}d] hasVideoDelay:%{public}d, pts: " PUBLIC_LOG_D64 " error, < " PUBLIC_LOG_D64,
        trackId_, hasVideoDelay_, sample->pts_, lastTimestampUs_);
    if (!hasSetParserConfig_) {
        ParserSetConfig();
    }
    int64_t pos = io->GetPos();
    int32_t size = videoParser_->WriteFrame(io, sample);
    FALSE_RETURN_V_MSG_E(size > 0, (size == 0 ? Status::NO_ERROR : Status::ERROR_INVALID_DATA),
        "video write frame err:%{public}d", size);
    DisposeCtts(pts);
    DisposeStco(pos);
    DisposeSdtp(sample->flag_);
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
    auto trackBox = moov_->GetChild(std::string("trak") + std::to_string(trackId_));
    if (trackBox != nullptr && stsz_->sampleCount_ == 0) {
        trackBox->NeedWrite(false);
        MEDIA_LOG_I("[%{public}d] sample is null, set not write trak box", trackId_);
    }
    if (lastDuration_ == 0 && stsz_->sampleCount_ > 1) {
        lastDuration_ = ConvertTime(ConvertTimeToMpeg4(lastTimestampUs_ - startTimestampUs_, timeScale_),
            stsz_->sampleCount_ - 1, 1);
    }
    if (stsz_->sampleCount_ > 0) {
        if (isHaveBFrame_) {
            DisposeCttsAndStts();
        } else {
            DisposeSttsOnly();
        }
        DisposeDuration();
        DisposeBitrate();
        DisposeColor();
        DisposeCuva();
    }
    return Status::NO_ERROR;
}

int32_t VideoTrack::GetWidth() const
{
    return width_;
}

int32_t VideoTrack::GetHeight() const
{
    return height_;
}

bool VideoTrack::IsColor() const
{
    return isColor_;
}

ColorPrimary VideoTrack::GetColorPrimaries() const
{
    return colorPrimaries_;
}

TransferCharacteristic VideoTrack::GetColorTransfer() const
{
    return colorTransfer_;
}

MatrixCoefficient VideoTrack::GetColorMatrixCoeff() const
{
    return colorMatrixCoeff_;
}

bool VideoTrack::GetColorRange() const
{
    return colorRange_;
}

bool VideoTrack::IsCuvaHDR() const
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

void VideoTrack::UpdateUserMeta(const std::shared_ptr<Meta> &userMeta)
{
    HevcParser* parser = VideoParser::GetVideoParserPtr<HevcParser>(videoParser_);
    FALSE_RETURN_MSG(parser != nullptr && userMeta != nullptr, "parser or user meta is nullptr");
    if (mimeType_.compare(AVCodecMimeType::MEDIA_MIMETYPE_VIDEO_HEVC) == 0 &&
        parser->GetColorTransfer() == static_cast<uint8_t>(TransferCharacteristic::UNSPECIFIED)) {
        constexpr uint32_t maxSize = 128;
        std::ostringstream oss;
        std::vector<uint8_t> logInfo = parser->GetLogInfo();
        FALSE_RETURN_MSG_W(!logInfo.empty() && logInfo.size() <= maxSize,
            "invalid logInfo, logInfo.size: %{public}zu", logInfo.size());
        for (uint8_t info : logInfo) {
            oss << std::hex << std::setw(2) << std::setfill('0') << static_cast<int>(info); // 2 characters indicate
        }
        userMeta->SetData("com.openharmony.video.sei.h_log", oss.str());
        MEDIA_LOG_I("set h265 log info finished");
    }
}

bool VideoTrack::InitColor(const std::shared_ptr<Meta> &trackDesc)
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
        MEDIA_LOG_I("color info.: primary %{public}d, transfer %{public}d, matrix coeff %{public}d,"
            " range %{public}d,", static_cast<int32_t>(colorPrimaries_), static_cast<int32_t>(colorTransfer_),
            static_cast<int32_t>(colorMatrixCoeff_), static_cast<int32_t>(colorRange_));
        if (!CheckColorPrimary(colorPrimaries_) || !CheckColorTransfer(colorTransfer_) ||
            !CheckColorMatrix(colorMatrixCoeff_)) {
            return false;
        }
        if (colorPrimaries_ != ColorPrimary::UNSPECIFIED &&
            colorTransfer_ != TransferCharacteristic::UNSPECIFIED &&
            colorMatrixCoeff_ != MatrixCoefficient::UNSPECIFIED) {
            isColor_ = true;
        }
    }
    return true;
}

void VideoTrack::InitCuva(const std::shared_ptr<Meta> &trackDesc)
{
    isCuvaHDR_ = false;
    if (trackDesc->Find(Tag::VIDEO_IS_HDR_VIVID) != trackDesc->end()) {
        trackDesc->Get<Tag::VIDEO_IS_HDR_VIVID>(isCuvaHDR_);
        MEDIA_LOG_I("hdr type: %{public}d", isCuvaHDR_);
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

void VideoTrack::DisposeCttsByFrameRate(int64_t pts)
{
    FALSE_RETURN_MSG(tempCtts_ != nullptr, "ctts box is empty");
    int64_t cts = ConvertTimeToMpeg4(pts, timeScale_) - dts_;
    if (cts > INT32_MAX || cts < INT32_MIN) {
        MEDIA_LOG_E("ctts sample_offset must <= INT32_MAX and >= INT32_MIN, but the value is " PUBLIC_LOG_D64 ", "
            "the pts is " PUBLIC_LOG_D64, cts, pts);
    }
    delay_ = std::min(cts, delay_);
    tempCtts_->AddData(static_cast<uint32_t>(cts));
    dts_ += lastDuration_;
}

void VideoTrack::DisposeCttsAndStts(int64_t mpeg4Pts)
{
    FALSE_RETURN_MSG(ctts_ != nullptr, "ctts box is empty");
    int64_t dtsMin = dts_;
    if (mpeg4Pts > ptsMax_) {
        // 更新dts为上一次ptsMax和dts的最大值
        dts_ = std::max(ptsMax_, dts_);
        ptsMax_ = mpeg4Pts;
    }
    int64_t cts = mpeg4Pts - dtsMin;
    if (cts > INT32_MAX || cts < INT32_MIN) {
        MEDIA_LOG_E("ctts sample_offset must <= INT32_MAX and >= INT32_MIN, but the value is " PUBLIC_LOG_D64 ", "
            "the pts is " PUBLIC_LOG_D64, cts, mpeg4Pts);
    }
    delay_ = std::min(cts, delay_);
    ctts_->AddData(static_cast<uint32_t>(cts));
    dts_ += lastDuration_;
    DisposeStts(dts_ - dtsMin, mpeg4Pts);
}

void VideoTrack::DisposeSttsNoPts()
{
    int64_t dts = 0;
    int64_t lastTimestamp = 0;
    bool isFirst = true;
    while (tempCtts_->timeToSamples_.size() > 0) {
        auto timeToSample = tempCtts_->timeToSamples_.front();
        tempCtts_->timeToSamples_.pop_front();
        tempCtts_->entryCount_--;
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

void VideoTrack::DisposeSttsOnly()
{
    if (allPts_.size() > 0) {
        int64_t lastTimestampUs = ConvertTimeToMpeg4(allPts_.front(), timeScale_);
        allPts_.pop();
        while (allPts_.size() > 0) {
            int64_t pts = ConvertTimeToMpeg4(allPts_.front(), timeScale_);
            allPts_.pop();
            int64_t duration = pts - lastTimestampUs;
            lastTimestampUs = pts;
            DisposeStts(duration, lastTimestampUs);
        }
        DisposeStts(lastDuration_, lastTimestampUs);
    } else {
        DisposeSttsNoPts();
    }
}

void VideoTrack::DisposeCttsAndStts()
{
    delay_ = 0;
    dts_ = startDts_;
    ptsMax_ = startDts_;
    if (allPts_.size() > 0) {  // 没有设置帧率
        while (allPts_.size() > 0) {
            int64_t pts = allPts_.front();
            allPts_.pop();
            DisposeCttsAndStts(ConvertTimeToMpeg4(pts, timeScale_));
        }
    } else {  // 有帧率，用ctts还原pts，再计算ctts和stts
        int64_t dts = startDts_;
        while (tempCtts_->timeToSamples_.size() > 0) {
            auto timeToSample = tempCtts_->timeToSamples_.front();
            tempCtts_->timeToSamples_.pop_front();
            tempCtts_->entryCount_--;
            for (uint32_t j = 0; j < timeToSample.first; ++j) {
                int64_t pts = dts + static_cast<int32_t>(timeToSample.second);
                DisposeCttsAndStts(pts);
                dts += lastDuration_;
            }
        }
    }
    // 更新ctts
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

void VideoTrack::DisposeDuration()
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
        data0.mediaTime_ = static_cast<uint64_t>(-1);
        elstBox->data_.emplace_back(data0);
        delay_ -= ConvertTimeToMpeg4(startTimestampUs_, timeScale_);
    } else {
        data.segmentDuration_ = static_cast<uint64_t>(duration + delay);
    }
    data.mediaTime_ = static_cast<uint64_t>(-std::min(static_cast<int64_t>(0), delay_));
    elstBox->data_.emplace_back(data);
}

void VideoTrack::DisposeBitrate()
{
    FALSE_RETURN_MSG(stsz_ != nullptr, "stsz box is empty");
    if (isSameSize_) {
        stsz_->sampleSize_ = stsz_->samples_.size() > 0 ? stsz_->samples_[0] : 0;
        stsz_->samples_.clear();
    }
    if (durationUs_ > 0 && !codingType_.empty()) {
        int64_t bitRate = static_cast<int64_t>(allSampleSize_) *
            8 * timeScale_ / ConvertTimeToMpeg4(durationUs_, timeScale_);  // 8
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
    HevcParser* parser = VideoParser::GetVideoParserPtr<HevcParser>(videoParser_);
    if (parser != nullptr && parser->IsHdrVivid()) {
        MEDIA_LOG_I("set cuva hdr (hdr vivid) true by video parser");
        isCuvaHDR_ = true;
    }
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

void VideoTrack::AddSdtpBox()
{
    FALSE_RETURN_MSG(sdtp_ != nullptr, "sdtp box is empty");
    auto stblBox = moov_->GetChild(trackPath_ + ".mdia.minf.stbl");
    FALSE_RETURN_MSG(stblBox != nullptr, "stbl box is empty");
    stblBox->AddChild(sdtp_);
    sdtp_->dependencyFlags_.clear();
    if (stsz_->sampleCount_ > 0) {
        sdtp_->dependencyFlags_.assign(stsz_->sampleCount_, 0x10);  // dependent = FRAME_DEPENDENCY_YES
        for (auto syncIndex : stss_->syncIndex_) {
            FALSE_RETURN_MSG(syncIndex > 0 && syncIndex <= sdtp_->dependencyFlags_.size(),
                "sync index out of range! index:%{public}u, count:%{public}zu",
                syncIndex, sdtp_->dependencyFlags_.size());
            sdtp_->dependencyFlags_[syncIndex - 1] = 0x20;  // dependent = FRAME_DEPENDENCY_NO
        }
    }
}

void VideoTrack::DisposeSdtp(uint32_t flag)
{
    uint8_t reference = FRAME_DEPENDENCY_UNKNOWN;
    if ((flag & static_cast<uint32_t>(AVBufferFlag::DISPOSABLE)) != 0) {
        reference = FRAME_DEPENDENCY_NO;
        MEDIA_LOG_D("frame[%{public}u] is disposable frame", stsz_->sampleCount_);
    } else if ((flag & static_cast<uint32_t>(AVBufferFlag::DISPOSABLE_EXT)) != 0) {
        reference = FRAME_DEPENDENCY_EXT;
        MEDIA_LOG_D("frame[%{public}u] is disposable_ext frame", stsz_->sampleCount_);
    }
    if (sdtp_ == nullptr && reference == FRAME_DEPENDENCY_UNKNOWN) {
        return;
    }
    uint8_t dependent = FRAME_DEPENDENCY_YES;
    uint8_t leading = FRAME_DEPENDENCY_UNKNOWN;
    uint8_t redundancy = FRAME_DEPENDENCY_UNKNOWN;
    if (sdtp_ == nullptr) {
        sdtp_ = std::make_shared<SdtpBox>(0, "sdtp");
        AddSdtpBox();
    }
    if (flag & static_cast<uint32_t>(AVBufferFlag::SYNC_FRAME)) {
        dependent = FRAME_DEPENDENCY_NO;
    }
    sdtp_->dependencyFlags_.push_back((leading << 6) | (dependent << 4) | (reference << 2) | redundancy);  // 6 4 2
}
} // Mpeg4
} // Plugins
} // Media
} // OHOS