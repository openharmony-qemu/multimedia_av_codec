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
#include <unistd.h>
#include <algorithm>
#include <malloc.h>
#include <string>
#include <sstream>
#include <map>
#include <fstream>
#include <chrono>
#include <limits>
#include <numeric>

#include "avcodec_trace.h"
#include "demuxer_utils.h"
#include "securec.h"
#include "mpeg4_demuxer_plugin.h"
#include "mpeg4_sample_helper.h"
#include "mpeg4_utils.h"
#include "buffer/avbuffer.h"
#include "plugin/plugin_buffer.h"
#include "plugin/plugin_definition.h"
#include "meta/video_types.h"
#include "avcodec_sysevent.h"
#include "syspara/parameters.h"

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_CORE, LOG_DOMAIN_DEMUXER, "MPEG4ReferenceParser" };
constexpr int64_t REFERENCE_PARSER_TIMEOUT_MS = 10000;
constexpr int64_t TRACE_RATE = 100;

int64_t GetDuration(int64_t startTime)
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count() - startTime;
}

OHOS::Media::Status ConvertTrackTimeToUs(int64_t trackTime, int32_t timeScale, int64_t& timeUs)
{
    FALSE_RETURN_V_NOLOG(timeScale > 0, OHOS::Media::Status::ERROR_INVALID_DATA);
    return OHOS::Media::Plugins::MPEG4::ConvertTimeScale(trackTime,
        static_cast<int64_t>(OHOS::Media::Plugins::MPEG4::S_TO_US), static_cast<int64_t>(timeScale), &timeUs);
}
}

namespace OHOS {
namespace Media {
namespace Plugins {
namespace MPEG4 {

using RefTrace = MediaAVCodec::AVCodecTrace;

constexpr uint32_t REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT = 200000u;

bool MPEG4DemuxerPlugin::GetReferenceParserCapability(int32_t& trackIndex, std::string& mimeType) const
{
    trackIndex = -1;
    mimeType = MimeType::INVALID_TYPE;
    uint32_t videoTrackCount = 0;
    for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
        MediaType trackType = MediaType::UNKNOWN;
        std::string currentMime = MimeType::INVALID_TYPE;
        (void)mediaInfo_.tracks[i].Get<Tag::MEDIA_TYPE>(trackType);
        (void)mediaInfo_.tracks[i].Get<Tag::MIME_TYPE>(currentMime);
        if (trackType != MediaType::VIDEO || !StartWith(currentMime.c_str(), "video/")) {
            continue;
        }
        ++videoTrackCount;
        if (videoTrackCount == 1 && i <= static_cast<uint32_t>(INT32_MAX)) {
            trackIndex = static_cast<int32_t>(i);
            mimeType = currentMime;
        }
    }
    return videoTrackCount == 1 &&
        (mimeType == MimeType::VIDEO_AVC || mimeType == MimeType::VIDEO_HEVC);
}

bool MPEG4DemuxerPlugin::IsRefParserSupported()
{
    FileType fileType = FileType::UNKNOW;
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType), false,
        "fileType does not exist in mediaInfo_");
    FALSE_RETURN_V_MSG_E(fileType == FileType::MP4 || fileType == FileType::MOV, false,
        "RefParser unsupported file type " PUBLIC_LOG_D32, fileType);
    // Capability checks stay side-effect free; heavy GOP state is prepared on first ParserRefInfo().
    int32_t trackIndex = -1;
    std::string mimeType;
    return GetReferenceParserCapability(trackIndex, mimeType);
}

Status MPEG4DemuxerPlugin::GetReferenceSampleTime(
    uint32_t sampleIndex, int64_t& ptsUs, int64_t& dtsUs) const
{
    FALSE_RETURN_V_NOLOG(trackOfRefParser_ != nullptr && trackOfRefParser_->sampleHelper != nullptr,
        Status::ERROR_INVALID_DATA);
    MPEG4SampleHelper::SampleIndexView view;
    FALSE_RETURN_V_NOLOG(trackOfRefParser_->sampleHelper->GetSampleIndexView(sampleIndex, view) == Status::OK,
        Status::ERROR_INVALID_DATA);
    int32_t timeScale = trackOfRefParser_->timeScale;
    FALSE_RETURN_V_NOLOG(timeScale > 0, Status::ERROR_INVALID_DATA);
    FALSE_RETURN_V_NOLOG(ConvertTrackTimeToUs(view.pts, timeScale, ptsUs) == Status::OK &&
        ConvertTrackTimeToUs(view.dts, timeScale, dtsUs) == Status::OK, Status::ERROR_INVALID_DATA);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::EnsureReferenceParserPrepared()
{
    std::lock_guard<std::mutex> lock(refPrepareMutex_);
    if (refPrepareAttempted_) {
        return refPrepareStatus_;
    }
    refPrepareAttempted_ = true;
    int32_t trackIndex = -1;
    std::string mimeType;
    if (!GetReferenceParserCapability(trackIndex, mimeType)) {
        refPrepareStatus_ = Status::ERROR_UNSUPPORTED_FORMAT;
        return refPrepareStatus_;
    }
    auto track = FindTrackById(trackIndex);
    if (track == nullptr || track->sampleHelper == nullptr || track->sampleHelper->GetSampleCount() == 0) {
        refPrepareStatus_ = Status::ERROR_INVALID_DATA;
        return refPrepareStatus_;
    }
    // Reference parsing needs stable random access and sync remapping, so it is the lazy dense trigger.
    refPrepareStatus_ = track->sampleHelper->EnsureDenseIndex();
    if (refPrepareStatus_ != Status::OK) {
        return refPrepareStatus_;
    }
    refParserTrackIdx_ = trackIndex;
    mimeTypeOfVideoStream_ = mimeType;
    trackOfRefParser_ = track;
    syncSamplesIndex_ = track->sampleHelper->GetSyncSamplesIndexFromStssEntry();
    int64_t firstDtsUs = 0;
    refPrepareStatus_ = GetReferenceSampleTime(0, startPts_, firstDtsUs);
    MEDIA_LOG_I("Reference parser prepared, track=" PUBLIC_LOG_D32 ", sync samples=" PUBLIC_LOG_ZU,
        refParserTrackIdx_, syncSamplesIndex_.size());
    return refPrepareStatus_;
}

Status MPEG4DemuxerPlugin::ParserRefUpdatePos(int64_t timeStampMs, bool isForward)
{
    FALSE_RETURN_V_MSG_E(IsRefParserSupported(), Status::ERROR_UNSUPPORTED_FORMAT, "Unsupport ref parser");
    int64_t clipTimeStampMs = std::max(timeStampMs, static_cast<int64_t>(0));
    if (syncSamplesIndex_.size() == 0 || referenceParser_ == nullptr) {
        MEDIA_LOG_W("ParserRefUpdatePos failed, syncSamplesIndex_ size is " PUBLIC_LOG_ZU, syncSamplesIndex_.size());
        refParserPendingSeekMsTime_ = clipTimeStampMs;
        refParserIsForward_ = isForward;
        return Status::OK;
    }

    int32_t gopId = 0;
    FALSE_RETURN_V_MSG_E(GetGopIdFromSeekPos(clipTimeStampMs, gopId) == Status::OK, Status::ERROR_UNKNOWN,
        "GetGopIdFromSeekPos failed.");

    GopLayerInfo gopLayerInfo;
    Status ret = GetGopLayerInfo(gopId, gopLayerInfo);
    if (ret == Status::ERROR_AGAIN && gopId != refParserCurGopId_) {
        refParserPendingSeekMsTime_ = clipTimeStampMs;
        refParserState_ = false;
        MEDIA_LOG_I("Pending time: " PUBLIC_LOG_D64, refParserPendingSeekMsTime_);
    }
    refParserIsForward_ = isForward;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::EnsureParserRefInfoInitialized()
{
    if (isInit_) {
        return Status::OK;
    }
    FileType fileType = FileType::UNKNOW;
    // Keep the historical no-source behavior: RefParserInit() used to fail on an empty sync table first.
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType), Status::ERROR_UNKNOWN,
        "fileType does not exist in mediaInfo_");
    Status prepareRet = EnsureReferenceParserPrepared();
    FALSE_RETURN_V_MSG_E(prepareRet == Status::OK, prepareRet, "Prepare reference parser failed");
    isInit_ = true;
    Status ret = RefParserInit();
    FALSE_RETURN_V_NOLOG(ret != Status::END_OF_STREAM, Status::OK);
    FALSE_RETURN_V(ret == Status::OK, Status::ERROR_UNKNOWN);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ParserRefInfo()
{
    MEDIA_LOG_I("ParserRefInfo start, refParserCurGopId_ is " PUBLIC_LOG_D32, refParserCurGopId_);
    Status initRet = EnsureParserRefInfoInitialized();
    FALSE_RETURN_V(initRet == Status::OK, initRet);
    int64_t duration = GetDuration(refParserStartTime_);
    RefTrace trace("ParserRefCost_2_" + std::to_string(duration) + "_" + std::to_string(refParserCurGopId_));
    FALSE_RETURN_V_MSG_W(duration < REFERENCE_PARSER_TIMEOUT_MS, Status::ERROR_UNKNOWN, "Reference parser timeout");
    FALSE_RETURN_V_MSG(refParserCurGopId_ != -1, Status::OK, "Reference parser end");  // referenceship parse done
    FALSE_RETURN_V_MSG_E(SelectProGopId() == Status::OK, Status::ERROR_UNKNOWN, "Call selectProGopId failed");
    // sync sample index of current gop(start from 0)
    uint32_t curSampleIndex = syncSamplesIndex_[refParserCurGopId_];
    MEDIA_LOG_I("curSampleIndex is " PUBLIC_LOG_U32 ", refParserCurGopId_ is " PUBLIC_LOG_D32
        ", processingSyncSamplesIndex_ size is " PUBLIC_LOG_ZU, curSampleIndex, refParserCurGopId_,
        processingSyncSamplesIndex_.size());
    
    std::shared_ptr<Sample> sample = std::make_shared<Sample>();
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_UNKNOWN, "sample is nullptr");
    int loopCount = 0;
    SelectTrack(refParserTrackIdx_);
    FALSE_RETURN_V_MSG_E(trackOfRefParser_ && trackOfRefParser_->sampleHelper, Status::ERROR_INVALID_DATA,
        "trackOfRefParser_ or sampleHelper is nullptr");
    MEDIA_LOG_I("size of sampleIndexEntries_ is " PUBLIC_LOG_ZU,
        trackOfRefParser_->sampleHelper->GetSampleCount());

    while (refParserState_ && refParserCurGopId_ != -1) {
        Status rlt = ParserRefInfoLoop(sample, curSampleIndex);
        if (rlt == Status::ERROR_EARLY_STOP) {  // 编码格式等不符合快速解析时，直接跳过
            MEDIA_LOG_I("Exiting reference parsing early due to simple first GOP structure.");
            refParserState_ = true;
            return Status::OK;
        }
        if ((loopCount++ % TRACE_RATE) == 0) {  // trace降采样
            duration = GetDuration(refParserStartTime_);
            std::string suffix = "_" + std::to_string(refParserCurGopId_) + "_" + std::to_string(curSampleIndex);
            RefTrace traceInLoop("ParserRefCost_3_" + std::to_string(duration) + suffix);
            FALSE_RETURN_V_MSG_W(duration < REFERENCE_PARSER_TIMEOUT_MS, Status::ERROR_UNKNOWN, "Infoloop timeout");
        }
        if (rlt != Status::OK) {
            refParserState_ = true;
            return rlt;
        }
        curSampleIndex++;
    }
    refParserState_ = true;
    return Status::ERROR_AGAIN;
}

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(std::shared_ptr<AVBuffer> videoSample, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "referenceParser_ is nullptr");
    MEDIA_LOG_I("GetFrameLayerInfo start, dts is " PUBLIC_LOG_D64, videoSample->dts_);
    if (isSdtpExist_) {
        uint32_t frameId = 0;
        Status ret = Dts2FrameId(videoSample->dts_, frameId);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Dts2FrameId failed");
        return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
    }
    return referenceParser_->GetFrameLayerInfo(videoSample->dts_, frameLayerInfo);
}

Status MPEG4DemuxerPlugin::GetFrameLayerInfo(uint32_t frameId, FrameLayerInfo &frameLayerInfo)
{
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "referenceParser_ is nullptr");
    MEDIA_LOG_I("GetFrameLayerInfo start, frameId is " PUBLIC_LOG_U32, frameId);
    return referenceParser_->GetFrameLayerInfo(frameId, frameLayerInfo);
}

Status MPEG4DemuxerPlugin::GetGopLayerInfo(uint32_t gopId, GopLayerInfo &gopLayerInfo)
{
    FALSE_RETURN_V_MSG_W(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "referenceParser_ is nullptr");
    MEDIA_LOG_I("GetGopLayerInfo start, gopId is " PUBLIC_LOG_U32, gopId);
    return referenceParser_->GetGopLayerInfo(gopId, gopLayerInfo);
}

Status MPEG4DemuxerPlugin::GetIFramePos(std::vector<uint32_t> &IFramePos)
{
    if (!syncSamplesIndex_.empty()) {
        IFramePos = syncSamplesIndex_;
        return Status::OK;
    }
    auto track = firstTrack_;
    for (uint32_t trackIndex = 0; track != nullptr && trackIndex < mediaInfo_.tracks.size(); ++trackIndex) {
        MediaType trackType = MediaType::UNKNOWN;
        std::string mimeType = MimeType::INVALID_TYPE;
        (void)mediaInfo_.tracks[trackIndex].Get<Tag::MEDIA_TYPE>(trackType);
        (void)mediaInfo_.tracks[trackIndex].Get<Tag::MIME_TYPE>(mimeType);
        if (trackType != MediaType::VIDEO || !StartWith(mimeType.c_str(), "video/")) {
            track = track->next;
            continue;
        }
        FALSE_RETURN_V_MSG_E(track->sampleHelper != nullptr, Status::ERROR_INVALID_DATA,
            "Reference parser track is invalid");
        // GetIFramePos historically used the first video sync table and does not require RefParser preparation.
        IFramePos = track->sampleHelper->GetSyncSamplesIndexFromStssEntry();
        break;
    }
    FALSE_RETURN_V_MSG_E(!IFramePos.empty(), Status::ERROR_UNKNOWN, "sync sample size is 0");
    return Status::OK;
}

Status MPEG4DemuxerPlugin::Dts2FrameId(int64_t dts, uint32_t &frameId)
{
    FileType fileType = FileType::UNKNOW;
    // Preserve the empty-map result returned before reference-parser preparation became lazy.
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType), Status::ERROR_OUT_OF_RANGE,
        "fileType does not exist in mediaInfo_");
    Status ret = EnsureReferenceParserPrepared();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare reference parser failed");
    uint32_t left = 0;
    uint32_t right = static_cast<uint32_t>(trackOfRefParser_->sampleHelper->GetSampleCount());
    while (left < right) {
        uint32_t middle = left + (right - left) / 2;
        int64_t ptsUs = 0;
        int64_t dtsUs = 0;
        FALSE_RETURN_V_MSG_E(GetReferenceSampleTime(middle, ptsUs, dtsUs) == Status::OK,
            Status::ERROR_INVALID_DATA, "Get sample time failed");
        if (dtsUs < dts) {
            left = middle + 1;
        } else {
            right = middle;
        }
    }
    FALSE_RETURN_V_MSG_E(left < trackOfRefParser_->sampleHelper->GetSampleCount(),
        Status::ERROR_OUT_OF_RANGE, "Failed to find nearest frame");
    frameId = left;
    MEDIA_LOG_I("Dts2FrameId, dts is " PUBLIC_LOG_D64 ", frameId is " PUBLIC_LOG_U32, dts, frameId);
    return Status::OK;
}

Status MPEG4DemuxerPlugin::FrameId2SeekMs(uint32_t frameId, int64_t &seekMs)
{
    FileType fileType = FileType::UNKNOW;
    // Keep the previous no-source status from the empty sampleIndex2Pts_ lookup path.
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType), Status::ERROR_UNKNOWN,
        "fileType does not exist in mediaInfo_");
    Status ret = EnsureReferenceParserPrepared();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare reference parser failed");
    int64_t ptsUs = 0;
    int64_t dtsUs = 0;
    FALSE_RETURN_V_MSG_E(GetReferenceSampleTime(frameId, ptsUs, dtsUs) == Status::OK,
        Status::ERROR_UNKNOWN, "frameId is invalid");
    FALSE_RETURN_V_MSG_E(!__builtin_sub_overflow(ptsUs, startPts_, &ptsUs),
        Status::ERROR_INVALID_DATA, "Relative pts overflow");
    seekMs = ptsUs / US_TO_MS;
    return Status::OK;
}

bool MPEG4DemuxerPlugin::IsMultiVideoTracks() const
{
    uint32_t videoTrackCount = 0;
    for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
        MediaType trackType = MediaType::UNKNOWN;
        mediaInfo_.tracks[i].Get<Tag::MEDIA_TYPE>(trackType);
        std::string mimeType = MimeType::INVALID_TYPE;
        mediaInfo_.tracks[i].Get<Tag::MIME_TYPE>(mimeType);
        MEDIA_LOG_I("track index is " PUBLIC_LOG_U32 ", trackType is " PUBLIC_LOG_D32 ", mimeType is " PUBLIC_LOG_S, i,
            trackType, mimeType.c_str());
        if (trackType == MediaType::VIDEO && StartWith(mimeType.c_str(), "video/")) {
            videoTrackCount++;
            if (videoTrackCount > 1) {  // multi video tracks
                return true;
            }
        }
    }
    return false;
}

bool MPEG4DemuxerPlugin::IsSupportedCodec() const
{
    for (uint32_t i = 0; i < mediaInfo_.tracks.size(); ++i) {
        MediaType trackType = MediaType::UNKNOWN;
        mediaInfo_.tracks[i].Get<Tag::MEDIA_TYPE>(trackType);
        if (trackType == MediaType::VIDEO) {
            std::string mimeType = MimeType::INVALID_TYPE;
            mediaInfo_.tracks[i].Get<Tag::MIME_TYPE>(mimeType);
            if (mimeType == MimeType::VIDEO_AVC || mimeType == MimeType::VIDEO_HEVC) {
                return true;
            }
            return false;
        }
    }
    return false;
}

Status MPEG4DemuxerPlugin::GetGopIdFromSeekPos(int64_t seekMs, int32_t &gopId)
{
    FALSE_RETURN_V_MSG_E(seekMs >= 0, Status::ERROR_INVALID_PARAMETER, "SeekMs is less than 0");
    uint32_t sampleIdx = 0;
    Status ret = SeekMs2FrameId(seekMs, sampleIdx);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "GetSampleIdByDts failed, seekMs is" PUBLIC_LOG_D64, seekMs);

    gopId = std::upper_bound(syncSamplesIndex_.begin(), syncSamplesIndex_.end(), sampleIdx) - syncSamplesIndex_.begin();
    MEDIA_LOG_I("gopId is " PUBLIC_LOG_D32, gopId - 1);
    if (gopId > 0) {
        --gopId;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::EnsureReferencePtsOrder()
{
    Status ret = EnsureReferenceParserPrepared();
    FALSE_RETURN_V_NOLOG(ret == Status::OK, ret);
    std::lock_guard<std::mutex> lock(refPrepareMutex_);
    if (refPtsOrderReady_) {
        return Status::OK;
    }
    size_t sampleCount = trackOfRefParser_->sampleHelper->GetSampleCount();
    FALSE_RETURN_V_NOLOG(sampleCount <= static_cast<size_t>(UINT32_MAX), Status::ERROR_INVALID_DATA);
    std::vector<uint32_t> order(sampleCount);
    std::iota(order.begin(), order.end(), 0U);
    auto comparePts = [this](uint32_t left, uint32_t right) {
        int64_t leftPts = 0;
        int64_t leftDts = 0;
        int64_t rightPts = 0;
        int64_t rightDts = 0;
        (void)GetReferenceSampleTime(left, leftPts, leftDts);
        (void)GetReferenceSampleTime(right, rightPts, rightDts);
        return leftPts != rightPts ? leftPts < rightPts : left < right;
    };
    std::sort(order.begin(), order.end(), comparePts);
    auto duplicatePts = [this](uint32_t left, uint32_t right) {
        int64_t leftPts = 0;
        int64_t leftDts = 0;
        int64_t rightPts = 0;
        int64_t rightDts = 0;
        return GetReferenceSampleTime(left, leftPts, leftDts) == Status::OK &&
            GetReferenceSampleTime(right, rightPts, rightDts) == Status::OK && leftPts == rightPts;
    };
    order.erase(std::unique(order.begin(), order.end(), duplicatePts), order.end());
    refPtsOrder_ = std::move(order);
    refPtsOrderReady_ = true;
    return Status::OK;
}

Status MPEG4DemuxerPlugin::SeekMs2FrameId(int64_t seekMs, uint32_t &frameId)
{
    MEDIA_LOG_I("SeekMs2FrameId start, seekMs is " PUBLIC_LOG_D64, seekMs);
    FALSE_RETURN_V_MSG_E(seekMs >= 0, Status::ERROR_INVALID_PARAMETER, "SeekMs is less than 0");
    FileType fileType = FileType::UNKNOW;
    // Match the old empty dts lookup result without dereferencing an empty map.
    FALSE_RETURN_V_MSG_E(mediaInfo_.general.Get<Tag::MEDIA_FILE_TYPE>(fileType), Status::ERROR_OUT_OF_RANGE,
        "fileType does not exist in mediaInfo_");
    Status ret = EnsureReferencePtsOrder();
    FALSE_RETURN_V_MSG_E(ret == Status::OK, ret, "Prepare pts order failed");
    int64_t relativePtsUs = 0;
    FALSE_RETURN_V_MSG_E(!__builtin_mul_overflow(seekMs, MS_TO_US, &relativePtsUs),
        Status::ERROR_INVALID_DATA, "Overflow");
    auto iter = std::upper_bound(refPtsOrder_.begin(), refPtsOrder_.end(), relativePtsUs,
        [this](int64_t targetPts, uint32_t sampleIndex) {
            int64_t ptsUs = 0;
            int64_t dtsUs = 0;
            return GetReferenceSampleTime(sampleIndex, ptsUs, dtsUs) == Status::OK &&
                targetPts < ptsUs - startPts_;
        });
    if (iter != refPtsOrder_.begin()) {
        --iter;
    }
    FALSE_RETURN_V_MSG_E(iter != refPtsOrder_.end(), Status::ERROR_OUT_OF_RANGE, "No sample for pts");
    int64_t ptsUs = 0;
    int64_t dtsUs = 0;
    FALSE_RETURN_V_MSG_E(GetReferenceSampleTime(*iter, ptsUs, dtsUs) == Status::OK,
        Status::ERROR_INVALID_DATA, "Get sample time failed");
    return Dts2FrameId(dtsUs, frameId);
}

Status MPEG4DemuxerPlugin::RefParserInit()
{
    refParserStartTime_ = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    std::string suffix = std::to_string(refParserStartTime_) + "_" + std::to_string(syncSamplesIndex_.size());
    RefTrace trace("ParserRefCost_1_" + suffix);
    FALSE_RETURN_V_MSG_E(syncSamplesIndex_.size() > 0, Status::ERROR_UNKNOWN,
        "Init failed, syncSamplesIndex_ size is" PUBLIC_LOG_ZU, syncSamplesIndex_.size());

    processingSyncSamplesIndex_.assign(syncSamplesIndex_.begin(), syncSamplesIndex_.end());
    CodecType codecType = mimeTypeOfVideoStream_ == MimeType::VIDEO_HEVC ? CodecType::H265 : CodecType::H264;
    referenceParser_ = ReferenceParserManager::Create(codecType, syncSamplesIndex_);
    FALSE_RETURN_V_MSG_E(referenceParser_ != nullptr, Status::ERROR_NULL_POINTER, "referenceParser_ is nullptr");
    
    trackOfRefParser_ = FindTrackById(refParserTrackIdx_);
    FALSE_RETURN_V_MSG_E(trackOfRefParser_ && trackOfRefParser_->sampleHelper, Status::ERROR_INVALID_DATA,
        "trackOfRefParser_ or sampleHelper is nullptr");
    MPEG4SampleHelper::SdtpEntry sdtpEntry = trackOfRefParser_->sampleHelper->GetSdtpEntry();
    if (sdtpEntry.sdtpEntryCount > 0 && !sdtpEntry.sdtpData.empty()) {
        if (referenceParser_->ParserSdtpData(sdtpEntry.sdtpData.data(), sdtpEntry.sdtpEntryCount) == Status::OK) {
            isSdtpExist_ = true;
            return Status::END_OF_STREAM;
        }
    }
    return referenceParser_->ParserExtraData(trackOfRefParser_->codecParms.data.get(),
        trackOfRefParser_->codecParms.extradataSize);
}

Status MPEG4DemuxerPlugin::SelectProGopId()
{
    if (refParserPendingSeekMsTime_ >= 0) {
        FALSE_RETURN_V_MSG_E(GetGopIdFromSeekPos(refParserPendingSeekMsTime_, refParserCurGopId_) == Status::OK,
            Status::ERROR_UNKNOWN, "GetGopIdFromSeekPos failed");
        refParserPendingSeekMsTime_ = -1;
    }
    int64_t seekDtsUs = 0;
    if (gopId2DecodedSamplePts_.find(refParserCurGopId_) != gopId2DecodedSamplePts_.end()) {
        // if I frame pts had got before decoding
        seekDtsUs = gopId2DecodedSamplePts_[refParserCurGopId_];
        MEDIA_LOG_I("get I frame pts from which had been decoded, seekDtsUs is " PUBLIC_LOG_D64, seekDtsUs);
    } else {
        uint32_t syncSampleIndex = syncSamplesIndex_[refParserCurGopId_];
        int64_t ptsUs = 0;
        FALSE_RETURN_V_MSG_E(GetReferenceSampleTime(syncSampleIndex, ptsUs, seekDtsUs) == Status::OK,
            Status::ERROR_UNKNOWN, "syncSampleIndex(" PUBLIC_LOG_U32 ") is invalid", syncSampleIndex);
        MEDIA_LOG_I("SelectProGopId, ptsUs is " PUBLIC_LOG_D64 ", seekDtsUs is " PUBLIC_LOG_D64,
            ptsUs, seekDtsUs);
    }
    int64_t realSeekTime = 0;
    // Using (seekDtsUs / US_TO_MS - 1) is to adapt to SeekToFrameByDts
    Status ret = SeekToFrameByDts(refParserTrackIdx_, seekDtsUs / US_TO_MS - 1, SeekMode::SEEK_CLOSEST, realSeekTime,
        0);
    MEDIA_LOG_I("realSeekTime is " PUBLIC_LOG_D64, realSeekTime);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "SeekToFrameByDts failed");
    cacheSample_ = FrontTrackCache(refParserTrackIdx_);
    isReadSampleDataFromCache_ = true;
    return Status::OK;
}

static void InsertIframePtsMap(std::shared_ptr<Sample> sample, int32_t gopId,
    std::unordered_map<int32_t, int64_t> &gopId2DecodedSamplePts)
{
    MEDIA_LOG_I("sample info: size is " PUBLIC_LOG_D32 ", dts is " PUBLIC_LOG_D64 ", pts is " PUBLIC_LOG_D64,
        sample->size, sample->dts, sample->pts);
    bool validCheck = (sample != nullptr) && (sample->flag & Sample::SampleFlag::SYNC_FRAME) && (gopId != -1);
    if (validCheck && (gopId2DecodedSamplePts.find(gopId) == gopId2DecodedSamplePts.end())) {
        gopId2DecodedSamplePts.insert(std::pair<int32_t, int64_t>(gopId, sample->pts));
    }
}

Status MPEG4DemuxerPlugin::PrepareRefParserSample(std::shared_ptr<Sample>& sample, uint32_t sampleIndex)
{
    std::unique_lock<std::mutex> sLock(syncMutex_);
    FALSE_RETURN_V_MSG_E(trackOfRefParser_ != nullptr && trackOfRefParser_->sampleHelper != nullptr,
        Status::ERROR_UNKNOWN, "trackOfRefParser_ or sampleHelper is nullptr");
    size_t sampleCount = trackOfRefParser_->sampleHelper->GetSampleCount();
    if (sampleIndex > sampleCount) {
        MEDIA_LOG_E("sample index(" PUBLIC_LOG_U32 ") out of range", sampleIndex);
        return Status::ERROR_UNKNOWN;
    }
    if (sampleIndex == sampleCount) {
        sample->size = 0;
        sample->dts = INT64_MIN;
        sample->flag = Sample::SampleFlag::EOS;
    } else if (isReadSampleDataFromCache_) {
        FALSE_RETURN_V_MSG_E(cacheSample_ != nullptr, Status::ERROR_UNKNOWN, "cacheSample_ is nullptr");
        sample = cacheSample_->sample;
        isReadSampleDataFromCache_ = false;
    } else {
        MPEG4SampleHelper::SampleIndexView currentSample;
        FALSE_RETURN_V_MSG_E(trackOfRefParser_->sampleHelper->GetSampleIndexView(sampleIndex, currentSample) ==
            Status::OK, Status::ERROR_UNKNOWN, "Get sample view failed");
        SampleReadInfo sampleInfo {refParserTrackIdx_, sampleIndex, currentSample, true};
        Status ret = ReadSampleData(sample, sampleInfo);
        FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Failed to read sample.");
        FALSE_RETURN_V_MSG_E(trackOfRefParser_ != nullptr, Status::ERROR_UNKNOWN, "trackOfRefParser_ is nullptr");
        int32_t timeScale = trackOfRefParser_->timeScale;
        FALSE_RETURN_V_MSG_E(timeScale > 0, Status::ERROR_UNKNOWN, "timeScale is invalid");
        FALSE_RETURN_V_MSG_E(ConvertTrackTimeToUs(currentSample.pts, timeScale, sample->pts) == Status::OK &&
            ConvertTrackTimeToUs(currentSample.dts, timeScale, sample->dts) == Status::OK,
            Status::ERROR_UNKNOWN, "Convert sample time failed");
        sample->flag = currentSample.flag;
        sample->size = currentSample.size;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ParserRefInfoLoop(std::shared_ptr<Sample> sample, uint32_t sampleIndex)
{
    MEDIA_LOG_I("ParserRefInfoLoop start, sampleIndex is " PUBLIC_LOG_U32, sampleIndex);
    Status result = PrepareRefParserSample(sample, sampleIndex);
    FALSE_RETURN_V_MSG_E(result == Status::OK, result, "Prepare reference parser sample failed");
    FALSE_RETURN_V_MSG_E(sample != nullptr, Status::ERROR_UNKNOWN, "sample is nullptr!");
    InsertIframePtsMap(sample, refParserCurGopId_, gopId2DecodedSamplePts_);
    result = referenceParser_->ParserNalUnits(sample->data.get(), sample->size, sampleIndex, sample->dts);
    FALSE_RETURN_V_MSG_E(result == Status::OK, result, "parse nal units end! ret is " PUBLIC_LOG_D32,
        static_cast<int32_t>(result));
    int32_t syncSampleNum = static_cast<int32_t>(syncSamplesIndex_.size());
    if (sample->flag & Sample::SampleFlag::EOS || (refParserCurGopId_ + 1 < syncSampleNum &&
        sampleIndex == syncSamplesIndex_[refParserCurGopId_ + 1] - 1)) {  // 到达视频末尾或者本GOP最后一帧解析完
        MEDIA_LOG_D("syncSamplesIndex_ size is " PUBLIC_LOG_ZU ", processingSyncSamplesIndex_ size is " PUBLIC_LOG_ZU
            ", sampleIndex is " PUBLIC_LOG_U32 ", refParserCurGopId_ is " PUBLIC_LOG_U32, syncSamplesIndex_.size(),
            processingSyncSamplesIndex_.size(), sampleIndex, refParserCurGopId_);
        processingSyncSamplesIndex_.remove(syncSamplesIndex_[refParserCurGopId_]);
        if (processingSyncSamplesIndex_.size() == 0) {
            refParserCurGopId_ = -1;
            return Status::OK;
        }
        return UpdateParserGopId(syncSampleNum);
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::UpdateParserGopId(int32_t syncSamplesPtsSize)
{
    MEDIA_LOG_I("UpdateParserGopId start, syncSamplesPtsSize is " PUBLIC_LOG_D32 ", refParserCurGopId_ is "
        PUBLIC_LOG_D32, syncSamplesPtsSize, refParserCurGopId_);
    FALSE_RETURN_V_NOLOG(syncSamplesPtsSize != 0, Status::ERROR_UNKNOWN);
    int32_t tmpGopId = refParserCurGopId_;
    int32_t searchCnt = 0;
    std::unique_lock<std::mutex> sLock(syncMutex_);
    while (std::find(processingSyncSamplesIndex_.begin(), processingSyncSamplesIndex_.end(),
        syncSamplesIndex_[refParserCurGopId_]) == processingSyncSamplesIndex_.end()) {
        if (updatePosIsForward_) {
            refParserCurGopId_ = (refParserCurGopId_ + 1) % syncSamplesPtsSize;
        } else {
            refParserCurGopId_ = refParserCurGopId_ == 0 ? syncSamplesPtsSize - 1 : refParserCurGopId_ - 1;
        }
        searchCnt++;
        FALSE_RETURN_V_MSG_E(searchCnt < syncSamplesPtsSize, Status::ERROR_UNKNOWN, "Cannot find gop");
    }
    if (tmpGopId + 1 != refParserCurGopId_ || !updatePosIsForward_) {
        MEDIA_LOG_D("UpdateParserGopId, refParserCurGopId_ is " PUBLIC_LOG_D32, refParserCurGopId_);
        return Status::END_OF_STREAM;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetIndexByRelativePresentationTimeUs(const uint32_t trackIndex,
    const uint64_t relativePresentationTimeUs, uint32_t &index)
{
    MEDIA_LOG_I("GetIndexByRelativePresentationTimeUs, trackIndex is " PUBLIC_LOG_U32 ", time is " PUBLIC_LOG_U64
        ", index is " PUBLIC_LOG_U32, trackIndex, relativePresentationTimeUs, index);
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    uint32_t trackCount = static_cast<uint32_t>(CountTracks());
    FALSE_RETURN_V_MSG_E(trackIndex < trackCount, Status::ERROR_INVALID_DATA, "Track is out of range");

    uint32_t sampleCount = GetSampleCount(trackIndex);
    FALSE_RETURN_V_MSG_E(sampleCount <= REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT, Status::ERROR_INVALID_DATA,
        "Sample count exceeds limit");
    
    ResetParametersOfPTSAndIndexConversion();
    // Obtain the absolute pts of the first sample
    Status ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::GET_FIRST_SAMPLE_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN,
        "Failed to get the absolute pts of the first sample");
    int64_t absolutePTS = static_cast<int64_t>(relativePresentationTimeUs) + absolutePTSOfIndexZero_;

    ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::RELATIVEPTS_TO_INDEX, trackIndex,
        absolutePTS, index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Failed to get pts");

    FALSE_RETURN_V_MSG_E(absolutePTS >= minRelativePTS_ && absolutePTS <= maxRelativePTS_, Status::ERROR_INVALID_DATA,
        "Pts is out of range");
    if (minRelativePTSDiffOfLeft_ == 0 || minRelativePTSDiffOfRight_ == 0 ||
        minRelativePTSDiffOfLeft_ >= minRelativePTSDiffOfRight_) {
        index = index2RelativePTS_;
    } else if (minRelativePTSDiffOfLeft_ < minRelativePTSDiffOfRight_) {
        index = index2RelativePTS_ - 1;
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::GetRelativePresentationTimeUsByIndex(const uint32_t trackIndex,
    const uint32_t index, uint64_t &relativePresentationTimeUs)
{
    MEDIA_LOG_I("GetRelativePresentationTimeUsByIndex, trackIndex is " PUBLIC_LOG_U32 ", time is " PUBLIC_LOG_U64
        ", index is " PUBLIC_LOG_U32, trackIndex, relativePresentationTimeUs, index);
    std::shared_lock<std::shared_mutex> lock(sharedMutex_);
    uint32_t trackCount = static_cast<uint32_t>(CountTracks());
    FALSE_RETURN_V_MSG_E(trackIndex < trackCount, Status::ERROR_INVALID_DATA, "Track is out of range");
    
    FALSE_RETURN_V_MSG_E(index < UINT32_MAX, Status::ERROR_INVALID_DATA, "Index is out of range");
    uint32_t sampleCount = GetSampleCount(trackIndex);
    FALSE_RETURN_V_MSG_E(sampleCount <= REFERENCE_PARSER_PTS_LIST_UPPER_LIMIT, Status::ERROR_INVALID_DATA,
        "Sample count exceeds limit");
    FALSE_RETURN_V_MSG_E(index + 1 <= sampleCount, Status::ERROR_INVALID_DATA, "Index is out of range");
    
    ResetParametersOfPTSAndIndexConversion();
    // Obtain the absolute pts of the first sample
    Status ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::GET_FIRST_SAMPLE_PTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN,
        "Failed to get the absolute pts of the first sample");

    ret = GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode::INDEX_TO_RELATIVEPTS, trackIndex,
        static_cast<int64_t>(relativePresentationTimeUs), index);
    FALSE_RETURN_V_MSG_E(ret == Status::OK, Status::ERROR_UNKNOWN, "Failed to get pts");

    FALSE_RETURN_V_MSG_E(index2RelativePTSHeap_.top() >= absolutePTSOfIndexZero_,
                         Status::ERROR_INVALID_DATA, "Calculation results less than 0");
    int64_t relativePts = index2RelativePTSHeap_.top() - absolutePTSOfIndexZero_;
    relativePresentationTimeUs = static_cast<uint64_t>(relativePts);

    return Status::OK;
}

uint32_t MPEG4DemuxerPlugin::GetSampleCount(uint32_t trackIndex)
{
    std::shared_ptr<MPEG4AtomParser::Track> track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, 0, "firstTrack_ is nullptr");
    return track->sampleHelper->CalculateSampleCount();
}

void MPEG4DemuxerPlugin::ResetParametersOfPTSAndIndexConversion()
{
    absolutePTSOfIndexZero_ = INT64_MAX;
    index2RelativePTSHeap_ = std::priority_queue<int64_t>();
    minRelativePTS_ = INT64_MAX;
    maxRelativePTS_ = INT64_MIN;
    minRelativePTSDiffOfLeft_ = INT64_MAX;
    minRelativePTSDiffOfRight_ = INT64_MAX;
    index2RelativePTS_ = 0;
}

Status MPEG4DemuxerPlugin::GetPresentationTimeUsByMpeg4BoxParser(IndexAndPTSConversionMode mode, uint32_t trackIndex,
    int64_t absolutePTS, uint32_t sampleIndex)
{
    std::shared_ptr<MPEG4AtomParser::Track> track = FindTrackById(trackIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "track or sampleHelper is nullptr");

    uint32_t sttsEnrtyCount = track->sampleHelper->GetSttsEntryCount();
    FALSE_RETURN_V_MSG_E(sttsEnrtyCount != 0, Status::ERROR_INVALID_DATA, "sttsEnrtyCount is empty");
    uint32_t cttsEnrtyCount = track->sampleHelper->GetCttsEntryCount();

    return (cttsEnrtyCount == 0) ? ConvertPTSAndIndexOnlyByStts(track, mode, absolutePTS, sampleIndex) :
        ConvertPTSAndIndexBySttsAndCtts(track, mode, absolutePTS, sampleIndex);
}

Status MPEG4DemuxerPlugin::ConvertPTSAndIndexOnlyByStts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex)
{
    MEDIA_LOG_I("absolutePTS is " PUBLIC_LOG_D64 ", sampleIndex is " PUBLIC_LOG_U32, absolutePTS, sampleIndex);
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "track or sampleHelper is nullptr");

    int32_t timeScale = track->timeScale;
    FALSE_RETURN_V_MSG_E(timeScale > 0, Status::ERROR_INVALID_DATA, "timescale is invalid");

    uint32_t sttsEntryIndex = 0;
    uint32_t sttsEntryCount = track->sampleHelper->GetSttsEntryCount();
    uint32_t sampleCountOfCurrentSttsEntry = track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex);
    int64_t pts = 0;
    int64_t dts = 0;

    while (sttsEntryIndex < sttsEntryCount) {
        FALSE_RETURN_V_MSG_E(ConvertTrackTimeToUs(dts, timeScale, pts) == Status::OK,
            Status::ERROR_INVALID_DATA, "Convert dts to pts failed");
        ProcessConversionByMode(mode, pts, absolutePTS, sampleIndex);

        if (sampleCountOfCurrentSttsEntry == 0) {
            break;
        }
        sampleCountOfCurrentSttsEntry--;
        int64_t sampleDeltaOfCurrentSttsEntry =
            static_cast<int64_t>(track->sampleHelper->GetSampleDeltaOfSttsEntry(sttsEntryIndex));
        FALSE_RETURN_V_MSG_E(INT64_MAX - dts >= sampleDeltaOfCurrentSttsEntry, Status::ERROR_INVALID_DATA,
            "dts overflow");

        dts += sampleDeltaOfCurrentSttsEntry;
        if (sampleCountOfCurrentSttsEntry == 0) {
            sttsEntryIndex++;
            sampleCountOfCurrentSttsEntry = sttsEntryIndex < sttsEntryCount ?
                track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex) : 0;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::ConvertPTSAndIndexBySttsAndCtts(const std::shared_ptr<MPEG4AtomParser::Track>& track,
    IndexAndPTSConversionMode mode, int64_t absolutePTS, uint32_t sampleIndex)
{
    FALSE_RETURN_V_MSG_E(track && track->sampleHelper, Status::ERROR_INVALID_DATA, "track or sampleHelper is nullptr");

    int32_t timeScale = track->timeScale;
    FALSE_RETURN_V_MSG_E(timeScale > 0, Status::ERROR_INVALID_DATA, "timescale is invalid");

    uint32_t sttsEntryIndex = 0;
    uint32_t sttsEntryCount = track->sampleHelper->GetSttsEntryCount();
    uint32_t sampleCountOfCurrentSttsEntry = track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex);
    uint32_t cttsEntryIndex = 0;
    uint32_t cttsEntryCount = track->sampleHelper->GetCttsEntryCount();
    uint32_t sampleCountOfCurrentCttsEntry = track->sampleHelper->GetSampleCountOfCttsEntry(cttsEntryIndex);
    // Avoid offset less than 0 in Ctts Box
    int32_t minSampleOffsetOfCttsEntry = track->sampleHelper->GetMinOffsetOfCttsEntry();

    int64_t pts = 0;
    int64_t dts = 0;

    while (sttsEntryIndex < sttsEntryCount && cttsEntryIndex < cttsEntryCount) {
        if (sampleCountOfCurrentCttsEntry == 0) {
            cttsEntryIndex++;
            if (cttsEntryIndex >= cttsEntryCount) {
                break;
            }
            sampleCountOfCurrentCttsEntry = track->sampleHelper->GetSampleCountOfCttsEntry(cttsEntryIndex);
            if (sampleCountOfCurrentCttsEntry == 0) {
                break;
            }
        }
        sampleCountOfCurrentCttsEntry--;
        
        int64_t sampleOffsetOfCurrentCttsEntry = static_cast<int64_t>(
            track->sampleHelper->GetSampleOffsetOfCttsEntry(cttsEntryIndex) - minSampleOffsetOfCttsEntry);
        FALSE_RETURN_V_MSG_E(UpdatePTSBySttsAndCtts(pts, dts, timeScale, sampleOffsetOfCurrentCttsEntry) ==
            Status::OK, Status::ERROR_INVALID_DATA,  "dts overflow");
        ProcessConversionByMode(mode, pts, absolutePTS, sampleIndex);
        if (sampleCountOfCurrentSttsEntry == 0) {
            break;
        }
        sampleCountOfCurrentSttsEntry--;
        int64_t sampleDeltaOfCurrentSttsEntry =
            static_cast<int64_t>(track->sampleHelper->GetSampleDeltaOfSttsEntry(sttsEntryIndex));
        FALSE_RETURN_V_MSG_E(INT64_MAX - dts >= sampleDeltaOfCurrentSttsEntry, Status::ERROR_INVALID_DATA,
            "dts overflow");
        dts += sampleDeltaOfCurrentSttsEntry;
        if (sampleCountOfCurrentSttsEntry == 0) {
            sttsEntryIndex++;
            sampleCountOfCurrentSttsEntry = sttsEntryIndex < sttsEntryCount ?
                track->sampleHelper->GetSampleCountOfSttsEntry(sttsEntryIndex) : 0;
        }
    }
    return Status::OK;
}

Status MPEG4DemuxerPlugin::UpdatePTSBySttsAndCtts(int64_t& pts, int64_t dts, int32_t timeScale,
    int64_t sampleOffsetOfCurrentCttsEntry)
{
    MEDIA_LOG_D("pts is " PUBLIC_LOG_D64 ", dts is " PUBLIC_LOG_D64 ", timeScale is " PUBLIC_LOG_D32
        ", sampleOffsetOfCurrentCttsEntry is " PUBLIC_LOG_D64, pts, dts, timeScale, sampleOffsetOfCurrentCttsEntry);
    int64_t correctedPts = 0;
    FALSE_RETURN_V_MSG_E(!__builtin_add_overflow(dts, sampleOffsetOfCurrentCttsEntry, &correctedPts),
        Status::ERROR_INVALID_DATA, "pts overflow");
    FALSE_RETURN_V_MSG_E(ConvertTrackTimeToUs(correctedPts, timeScale, pts) == Status::OK,
        Status::ERROR_INVALID_DATA, "Convert pts failed");
    return Status::OK;
}

void MPEG4DemuxerPlugin::ProcessConversionByMode(IndexAndPTSConversionMode mode, int64_t pts, int64_t absolutePTS,
    uint32_t sampleIndex)
{
    switch (mode) {
        case IndexAndPTSConversionMode::GET_FIRST_SAMPLE_PTS:
            absolutePTSOfIndexZero_ = pts < absolutePTSOfIndexZero_ ? pts : absolutePTSOfIndexZero_;
            break;
        case IndexAndPTSConversionMode::INDEX_TO_RELATIVEPTS:
            ProcessIndex2RelativePTS(pts, sampleIndex);
            break;
        case IndexAndPTSConversionMode::RELATIVEPTS_TO_INDEX:
            ProcessRelativePTS2Index(pts, absolutePTS);
            break;
        default:
            MEDIA_LOG_E("mode(" PUBLIC_LOG_U32 ") is invalid", mode);
            break;
    }
}

void MPEG4DemuxerPlugin::ProcessIndex2RelativePTS(int64_t pts, uint32_t index)
{
    if (index2RelativePTSHeap_.size() < index + 1) {
        index2RelativePTSHeap_.push(pts);
    } else if (pts < index2RelativePTSHeap_.top()) {
        index2RelativePTSHeap_.pop();
        index2RelativePTSHeap_.push(pts);
    }
}

void MPEG4DemuxerPlugin::ProcessRelativePTS2Index(int64_t pts, int64_t absolutePTS)
{
    minRelativePTS_ = minRelativePTS_ > pts ? pts : minRelativePTS_;
    maxRelativePTS_ = maxRelativePTS_ < pts ? pts : maxRelativePTS_;
    
    int64_t diff = 0;
    FALSE_RETURN_MSG(!__builtin_sub_overflow(pts, absolutePTS, &diff), "diff overflow");
    diff = (diff == INT64_MIN) ? INT64_MAX : diff;
    int64_t relativePTSDiff = (diff > 0) ? diff : -diff;
    if (pts < absolutePTS && relativePTSDiff < minRelativePTSDiffOfLeft_) {
        minRelativePTSDiffOfLeft_ = relativePTSDiff;
    }
    if (pts >= absolutePTS && relativePTSDiff < minRelativePTSDiffOfRight_) {
        minRelativePTSDiffOfRight_ = relativePTSDiff;
    }
    if (pts < absolutePTS) {  // Record the first index greater than or equal to pts
        index2RelativePTS_++;
    }
}

} // namespace MPEG4
} // namespace Plugins
} // namespace Media
} // namespace OHOS
