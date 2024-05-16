/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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
#define HST_LOG_TAG "DashMpdDownloader"
#include <mutex>
#include <unistd.h>
#include <cstdlib>
#include <sstream>
#include "plugin/plugin_time.h"
#include "dash_mpd_downloader.h"
#include "dash_mpd_util.h"
#include "sidx_box_parser.h"
#include "utils/time_utils.h"
#include "base64_utils.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr uint32_t DRM_UUID_OFFSET = 12;

DashMpdDownloader::DashMpdDownloader()
{
    downloader_ = std::make_shared<Downloader>("dashMpd");

    dataSave_ =  [this] (uint8_t*&& data, uint32_t&& len) {
        return SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len));
    };

    mpdParser_ = std::make_shared<DashMpdParser>();
    mpdManager_ = std::make_shared<DashMpdManager>();
    periodManager_ = std::make_shared<DashPeriodManager>();
    adptSetManager_ = std::make_shared<DashAdptSetManager>();
    representationManager_ = std::make_shared<DashRepresentationManager>();
}

DashMpdDownloader::~DashMpdDownloader() noexcept
{
    downloadRequest_ = nullptr;
    downloader_ = nullptr;
    representationManager_ = nullptr;
    adptSetManager_ = nullptr;
    periodManager_ = nullptr;
    mpdManager_ = nullptr;
    mpdParser_ = nullptr;
    streamDescriptions_.clear();
}

static int64_t ParseStartNumber(const std::string numberStr)
{
    int64_t startNum = 1;
    if (numberStr.length() > 0) {
        startNum = atoi(numberStr.c_str());
    }

    return startNum;
}

static int64_t GetStartNumber(const DashRepresentationInfo* repInfo)
{
    int64_t startNumberSeq = 1;
    if (repInfo->representationSegTmplt_ != nullptr) {
        startNumberSeq = ParseStartNumber(repInfo->representationSegTmplt_->multSegBaseInfo_.startNumber_);
    } else if (repInfo->representationSegList_ != nullptr) {
        startNumberSeq = ParseStartNumber(repInfo->representationSegList_->multSegBaseInfo_.startNumber_);
    }
    return startNumberSeq;
}

static int64_t GetStartNumber(const DashAdptSetInfo* adptSetInfo)
{
    int64_t startNumberSeq = 1;
    if (adptSetInfo->adptSetSegTmplt_ != nullptr) {
        startNumberSeq = ParseStartNumber(adptSetInfo->adptSetSegTmplt_->multSegBaseInfo_.startNumber_);
    } else if (adptSetInfo->adptSetSegList_ != nullptr) {
        startNumberSeq = ParseStartNumber(adptSetInfo->adptSetSegList_->multSegBaseInfo_.startNumber_);
    }
    return startNumberSeq;
}

static int64_t GetStartNumber(const DashPeriodInfo* periodInfo)
{
    int64_t startNumberSeq = 1;
    if (periodInfo->periodSegTmplt_ != nullptr) {
        startNumberSeq = ParseStartNumber(periodInfo->periodSegTmplt_->multSegBaseInfo_.startNumber_);
    } else if (periodInfo->periodSegList_ != nullptr) {
        startNumberSeq = ParseStartNumber(periodInfo->periodSegList_->multSegBaseInfo_.startNumber_);
    }
    return startNumberSeq;
}

static void MakeAbsoluteWithBaseUrl(std::vector<std::shared_ptr<DashSegment>> segmentsVector, std::string baseUrl)
{
    std::string segUrl = baseUrl;
    unsigned int size = segmentsVector.size();

    for (unsigned int index = 0; index < size; index++) {
        if (segmentsVector[index] != nullptr) {
            segUrl = baseUrl;
            DashAppendBaseUrl(segUrl, segmentsVector[index]->url_);
            segmentsVector[index]->url_ = segUrl;
        }
    }
}

static void MakeAbsoluteWithBaseUrl(std::shared_ptr<DashInitSegment> initSegment, std::string baseUrl)
{
    if (initSegment == nullptr) {
        return;
    }

    if (DashUrlIsAbsolute(initSegment->url_)) {
        return;
    }

    std::string segUrl = baseUrl;
    DashAppendBaseUrl(segUrl, initSegment->url_);
    initSegment->url_ = segUrl;
}

static DashSegmentInitValue AddOneSegment(unsigned int segRealDur, int64_t segmentSeq, std::string tempUrl,
                                          std::shared_ptr<DashStreamDescription> streamDesc)
{
    std::shared_ptr<DashSegment> segment = std::make_shared<DashSegment>();
    if (segment == nullptr) {
        MEDIA_LOG_E("make shared DashSegment failed");
        return DASH_SEGMENT_INIT_FAILED;
    }

    segment->streamId_ = streamDesc->streamId_;
    segment->bandwidth_ = streamDesc->bandwidth_;
    segment->duration_ = segRealDur;
    segment->startNumberSeq_ = streamDesc->startNumberSeq_;
    segment->numberSeq_ = segmentSeq;
    segment->url_ = tempUrl;
    segment->byteRange_ = "";
    streamDesc->mediaSegments_.push_back(segment);
    return DASH_SEGMENT_INIT_SUCCESS;
}

static DashSegmentInitValue AddOneSegment(const DashSegment &srcSegment,
                                          std::shared_ptr<DashStreamDescription> streamDesc)
{
    std::shared_ptr<DashSegment> segment = std::make_shared<DashSegment>(srcSegment);
    if (segment == nullptr) {
        MEDIA_LOG_E("make shared DashSegment failed");
        return DASH_SEGMENT_INIT_FAILED;
    }
    
    streamDesc->mediaSegments_.push_back(segment);
    return DASH_SEGMENT_INIT_SUCCESS;
}

/**
 * @brief    Get Representation From AdaptationSet
 *
 * @param    adptSet                chosen AdaptationSet
 * @param    repreIndex             Representation index in AdaptationSet
 *
 * @return   chosen representation
 */
static DashRepresentationInfo* GetRepresentationFromAdptSet(DashAdptSetInfo* adptSet, unsigned int repreIndex)
{
    if (repreIndex >= adptSet->representationList_.size()) {
        return nullptr;
    }

    unsigned int index = 0;
    for (DashList<DashRepresentationInfo *>::iterator it = adptSet->representationList_.begin();
         it != adptSet->representationList_.end(); it++, index++) {
        if (index == repreIndex) {
            return *it;
        }
    }
    return nullptr;
}

void DashMpdDownloader::Open(const std::string& url)
{
    url_ = url;
    DoOpen(url);
}

void DashMpdDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    statusCallback_ = cb;
}

void DashMpdDownloader::UpdateDownloadFinished(const std::string& url)
{
    MEDIA_LOG_I("UpdateDownloadFinished:ondemandSegBase_=%{public}u url=%{public}s", ondemandSegBase_, url.c_str());
    if (ondemandSegBase_) {
        ParseSidx();
    } else {
        ParseManifest();
    }
}

int DashMpdDownloader::GetInUseVideoStreamId()
{
    for (uint32_t index = 0; index < streamDescriptions_.size(); index++) {
        if (streamDescriptions_[index]->inUse_ && streamDescriptions_[index]->type_ == MediaAVCodec::MEDIA_TYPE_VID) {
            return streamDescriptions_[index]->streamId_;
        }
    }
    return -1;
}

DashMpdGetRet DashMpdDownloader::GetNextSegmentByStreamId(int streamId, std::shared_ptr<DashSegment>& seg)
{
    MEDIA_LOG_I("GetNextSegmentByStreamId streamId:" PUBLIC_LOG_D32, streamId);
    seg = nullptr;
    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
    for (auto &streamDescription : streamDescriptions_) {
        if (streamDescription->streamId_ == streamId) {
            if (streamDescription->segsState_ == DASH_SEGS_STATE_FINISH) {
                int64_t segmentIndex = (streamDescription->currentNumberSeq_ == -1) ? 0 :
                                       streamDescription->currentNumberSeq_ - streamDescription->startNumberSeq_ + 1;
                MEDIA_LOG_I("get segment index :"
                PUBLIC_LOG_D64
                ", id:"
                PUBLIC_LOG_D32
                ", seq:"
                PUBLIC_LOG_D64, segmentIndex, streamDescription->streamId_, streamDescription->currentNumberSeq_);
                if (segmentIndex >= 0 && (unsigned int) segmentIndex < streamDescription->mediaSegments_.size()) {
                    seg = streamDescription->mediaSegments_[segmentIndex];
                    streamDescription->currentNumberSeq_ = seg->numberSeq_;
                    MEDIA_LOG_I("after get segment index :"
                    PUBLIC_LOG_D64, streamDescription->currentNumberSeq_);
                    ret = DASH_MPD_GET_DONE;
                } else {
                    ret = DASH_MPD_GET_FINISH;
                }
            } else {
                ret = DASH_MPD_GET_UNDONE;
            }
            break;
        }
    }

    return ret;
}

DashMpdGetRet DashMpdDownloader::GetBreakPointSegment(int streamId, int64_t breakpoint,
                                                      std::shared_ptr<DashSegment> &seg)
{
    MEDIA_LOG_I("GetBreakPointSegment streamId:" PUBLIC_LOG_D32 ", breakpoint:" PUBLIC_LOG_D64, streamId, breakpoint);
    seg = nullptr;
    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
    for (auto &streamDescription : streamDescriptions_) {
        if (streamDescription->streamId_ == streamId) {
            if (streamDescription->segsState_ == DASH_SEGS_STATE_FINISH) {
                int64_t segmentDuration = 0;
                for (unsigned int index = 0; index <  streamDescription->mediaSegments_.size(); index++) {
                    if (segmentDuration + streamDescription->mediaSegments_[index]->duration_ > breakpoint) {
                        seg = streamDescription->mediaSegments_[index];
                        break;
                    }
                }
                if (seg != nullptr) {
                    streamDescription->currentNumberSeq_ = seg->numberSeq_;
                    MEDIA_LOG_I("GetBreakPointSegment find segment index :"
                    PUBLIC_LOG_D64, streamDescription->currentNumberSeq_);
                    ret = DASH_MPD_GET_DONE;
                } else {
                    MEDIA_LOG_W("GetBreakPointSegment all segment finish");
                    ret = DASH_MPD_GET_FINISH;
                }
            } else {
                MEDIA_LOG_E("GetBreakPointSegment no segment list");
                ret = DASH_MPD_GET_UNDONE;
            }
            break;
        }
    }

    return ret;
}

bool DashMpdDownloader::IsAllSegmentFinishedByStreamId(int streamId)
{
    for (auto &streamDescription : streamDescriptions_) {
        if (streamDescription->streamId_ == streamId) {
            if (streamDescription->segsState_ == DASH_SEGS_STATE_FINISH &&
                streamDescription->mediaSegments_.size() > 0) {
                int64_t segmentIndex = (streamDescription->currentNumberSeq_ == -1) ? 0 :
                                         streamDescription->currentNumberSeq_ - streamDescription->startNumberSeq_ + 1;
                if ((unsigned int) segmentIndex >= streamDescription->mediaSegments_.size()) {
                    return true;
                }
            }
            break;
        }
    }
    return false;
}

DashMpdGetRet DashMpdDownloader::GetNextVideoStream(const DashMpdBitrateParam& param, int& streamId)
{
    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
    std::shared_ptr<DashStreamDescription> currentStream = nullptr;
    std::shared_ptr<DashStreamDescription> destStream = nullptr;
    for (auto stream : streamDescriptions_) {
        if (stream->type_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
            continue;
        }

        if (stream->bandwidth_ == param.bitrate_) {
            destStream = stream;
            if (stream->inUse_) {
                MEDIA_LOG_I("switch to bandwidth:" PUBLIC_LOG_U32 ", stream is inuse", param.bitrate_);
            }
            MEDIA_LOG_I("switch to bandwidth:"
            PUBLIC_LOG_U32
            ", id:"
            PUBLIC_LOG_D32
            ", width:"
            PUBLIC_LOG_U32, stream->bandwidth_, stream->streamId_, stream->width_);
        }

        if (stream->inUse_) {
            currentStream = stream;
        }
    }

    if (destStream == nullptr) {
        MEDIA_LOG_E("switch to bandwidth:" PUBLIC_LOG_U32 ", can not find stream", param.bitrate_);
        return ret;
    }

    if (currentStream != nullptr) {
        currentStream->inUse_ = false;
    }

    destStream->inUse_ = true;
    streamId = destStream->streamId_;

    if (destStream->startNumberSeq_ != currentStream->startNumberSeq_) {
        MEDIA_LOG_E("select bitrate:"
        PUBLIC_LOG_U32
        " but seq:"
        PUBLIC_LOG_D64
        " is not equal bitrate:"
        PUBLIC_LOG_U32
        ", seq:"
        PUBLIC_LOG_D64, destStream->bandwidth_, destStream->startNumberSeq_,
        currentStream->bandwidth_, currentStream->startNumberSeq_);
    }
    if (param.position_ == -1) {
        destStream->currentNumberSeq_ = currentStream->currentNumberSeq_;
    } else {
        destStream->currentNumberSeq_ = param.position_;
    }
    MEDIA_LOG_I("GetNextVideoStream update id:"
    PUBLIC_LOG_D32
    ", seq:"
    PUBLIC_LOG_D64, destStream->streamId_, destStream->currentNumberSeq_);

    if (destStream->segsState_ == DASH_SEGS_STATE_FINISH) {
        // segment list is ok
        MEDIA_LOG_I("GetNextVideoStream id:" PUBLIC_LOG_D32 ", segment list is ok", destStream->streamId_);
        ret = DASH_MPD_GET_DONE;
    } else {
        // get segment list
        if (ondemandSegBase_) {
            // request sidx segment
            if (destStream->indexSegment_ != nullptr) {
                ret = DASH_MPD_GET_UNDONE;
                destStream->segsState_ = DASH_SEGS_STATE_PARSING;
                currentDownloadStream_ = destStream;
                int64_t startRange = destStream->indexSegment_->indexRangeBegin_;
                int64_t endRange = destStream->indexSegment_->indexRangeEnd_;
                // exist init segment, download together
                if (CheckToDownloadSidxWithInitSeg(destStream)) {
                    MEDIA_LOG_I("update range begin from "
                    PUBLIC_LOG_D64
                    " to "
                    PUBLIC_LOG_D64, startRange, destStream->initSegment_->rangeBegin_);
                    startRange = destStream->initSegment_->rangeBegin_;
                }
                DoOpen(destStream->indexSegment_->url_, startRange, endRange);
            } else {
                MEDIA_LOG_E("GetNextSegmentByBitrate id:"
                PUBLIC_LOG_D32
                " ondemandSegBase_ but indexSegment is null", destStream->streamId_);
            }
        } else {
            // get segment list
            if (GetSegmentsInMpd(destStream) == DASH_SEGMENT_INIT_FAILED) {
                MEDIA_LOG_E("GetNextSegmentByBitrate id:"
                PUBLIC_LOG_D32
                " GetSegmentsInMpd failed", destStream->streamId_);
            }
            // 根据position查找分片
            ret = DASH_MPD_GET_DONE;
        }
    }
    return ret;
}

std::shared_ptr<DashStreamDescription> DashMpdDownloader::GetStreamByStreamId(int streamId)
{
    for (auto streamDescription : streamDescriptions_) {
        if (streamDescription->streamId_ == streamId) {
            return streamDescription;
        }
    }

    return nullptr;
}

std::shared_ptr<DashInitSegment> DashMpdDownloader::GetInitSegmentByStreamId(int streamId)
{
    for (auto &streamDescription : streamDescriptions_) {
        if (streamDescription->streamId_ == streamId) {
            return streamDescription->initSegment_;
        }
    }

    return nullptr;
}

void DashMpdDownloader::SetCurrentNumberSeqByStreamId(int streamId, int64_t numberSeq)
{
    for (unsigned int index = 0; index < streamDescriptions_.size(); index++) {
        if (streamDescriptions_[index]->streamId_ == streamId) {
            streamDescriptions_[index]->currentNumberSeq_ = numberSeq;
            MEDIA_LOG_I("SetCurrentNumberSeqByStreamId update id:"
            PUBLIC_LOG_D32
            ", seq:"
            PUBLIC_LOG_D64, streamId, numberSeq);
            break;
        }
    }
}

void DashMpdDownloader::ParseManifest()
{
    if (downloadContent_.length() == 0) {
        MEDIA_LOG_I("ParseManifest content length is 0");
        return;
    }

    mpdParser_->ParseMPD(downloadContent_.c_str(), downloadContent_.length());
    mpdParser_->GetMPD(mpdInfo_);
    if (mpdInfo_ != nullptr) {
        mpdManager_->SetMpdInfo(mpdInfo_, url_);
        mpdManager_->GetDuration(&duration_);
        SetOndemandSegBase();
        GetStreamsInfoInMpd();
        ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
        ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
        if (ondemandSegBase_) {
            for (auto stream : streamDescriptions_) {
                if (stream->inUse_ && stream->indexSegment_ != nullptr &&
                    stream->segsState_ != DASH_SEGS_STATE_FINISH) {
                    stream->segsState_ = DASH_SEGS_STATE_PARSING;
                    currentDownloadStream_ = stream;
                    int64_t startRange = stream->indexSegment_->indexRangeBegin_;
                    int64_t endRange = stream->indexSegment_->indexRangeEnd_;
                    // exist init segment, download together
                    if (CheckToDownloadSidxWithInitSeg(stream)) {
                        MEDIA_LOG_I("update range begin from "
                        PUBLIC_LOG_D64
                        " to "
                        PUBLIC_LOG_D64, startRange, stream->initSegment_->rangeBegin_);
                        startRange = stream->initSegment_->rangeBegin_;
                    }
                    DoOpen(stream->indexSegment_->url_, startRange, endRange);
                    break;
                }
            }
            ProcessDrmInfos();
            return;
        }

        if (callback_ != nullptr) {
            callback_->OnMpdInfoUpdate(DASH_MPD_EVENT_STREAM_INIT);
        }

        ProcessDrmInfos();
        notifyOpenOk_ = true;
    }
}

void DashMpdDownloader::ProcessDrmInfos()
{
    std::vector<DashDrmInfo> drmInfos;
    std::list<DashPeriodInfo*> periods = mpdInfo_->periodInfoList_;
    for (auto &periodInfo : periods) {
        if (periodInfo == nullptr) {
            continue;
        }
        std::string periodDrmId{"PeriodId:"};
        periodDrmId.append(periodInfo->id_);
        DashList<DashAdptSetInfo*> adptSetList = periodInfo->adptSetList_;
        for (auto &adptSetInfo : adptSetList) {
            if (adptSetInfo == nullptr) {
                continue;
            }

            std::string adptSetDrmId = periodDrmId;
            adptSetDrmId.append(":AdptSetId:");
            adptSetDrmId.append(std::to_string(adptSetInfo->id_));
            GetDrmInfos(adptSetDrmId, adptSetInfo->commonAttrsAndElements_.contentProtectionList_, drmInfos);
            DashList<DashRepresentationInfo*> representationList = adptSetInfo->representationList_;
            for (auto &representationInfo : representationList) {
                if (representationInfo == nullptr) {
                    continue;
                }
                std::string representationDrmId = adptSetDrmId;
                representationDrmId.append(":RepresentationId:");
                representationDrmId.append(representationInfo->id_);
                GetDrmInfos(representationDrmId, representationInfo->commonAttrsAndElements_.contentProtectionList_,
                            drmInfos);
            }
        }
    }

    std::multimap<std::string, std::vector<uint8_t>> drmInfoMap;
    for (auto &drmInfo: drmInfos) {
        bool isReported = false;
        for (auto &localDrmInfo: localDrmInfos_) {
            if (drmInfo.uuid_ == localDrmInfo.uuid_ && drmInfo.pssh_ == localDrmInfo.pssh_) {
                isReported = true;
                break;
            }
        }

        if (isReported) {
            continue;
        }

        std::string psshString = drmInfo.pssh_;
        uint8_t pssh[2048]; // 2048: pssh len
        uint32_t psshSize = 2048; // 2048: pssh len
        if (Base64Utils::Base64Decode(reinterpret_cast<const uint8_t *>(psshString.c_str()),
                                      static_cast<uint32_t>(psshString.length()), pssh, &psshSize)) {
            uint32_t uuidSize = 16; // 16: uuid len
            if (psshSize >= DRM_UUID_OFFSET + uuidSize) {
                uint8_t uuid[16]; // 16: uuid len
                errno_t ret = memcpy_s(uuid, sizeof(uuid), pssh + DRM_UUID_OFFSET, uuidSize);
                if (ret != EOK) {
                    MEDIA_LOG_W("fetch uuid from pssh error, drmId " PUBLIC_LOG_S, drmInfo.drmId_.c_str());
                    continue;
                }
                std::stringstream ssConverter;
                std::string uuidString;
                for (uint32_t i = 0; i < uuidSize; i++) {
                    ssConverter << std::hex << static_cast<int32_t>(uuid[i]);
                    uuidString = ssConverter.str();
                }
                drmInfoMap.insert({uuidString, std::vector<uint8_t>(pssh, pssh + psshSize)});
                localDrmInfos_.emplace_back(drmInfo);
            }
        } else {
            MEDIA_LOG_W("Base64Decode pssh error, drmId " PUBLIC_LOG_S, drmInfo.drmId_.c_str());
        }
    }

    if (callback_ != nullptr) {
        callback_->OnDrmInfoChanged(drmInfoMap);
    }
}

void DashMpdDownloader::GetDrmInfos(const std::string &drmId, DashList<DashDescriptor *> &contentProtections,
                                    std::vector<DashDrmInfo> &drmInfoList)
{
    std::vector<DashDrmInfo> drmInfos;
    for (auto &contentProtection : contentProtections) {
        if (contentProtection == nullptr) {
            continue;
        }

        std::string schemeIdUrl = contentProtection->schemeIdUrl_;
        size_t uuidPos = schemeIdUrl.find(DRM_URN_UUID_PREFIX);
        if (uuidPos != std::string::npos) {
            std::string urnUuid = DRM_URN_UUID_PREFIX;
            std::string systemId = schemeIdUrl.substr(uuidPos + urnUuid.length());
            auto elementIt = contentProtection->elementMap_.find(MPD_LABEL_PSSH);
            if (elementIt != contentProtection->elementMap_.end()) {
                DashDrmInfo drmInfo = {drmId, systemId, elementIt->second};
                drmInfoList.emplace_back(drmInfo);
            }
        }
    }
}

void DashMpdDownloader::ParseSidx()
{
    if (downloadContent_.length() == 0 || currentDownloadStream_ == nullptr ||
        currentDownloadStream_->indexSegment_ == nullptr) {
        MEDIA_LOG_I("ParseSidx content length is 0 or stream is nullptr");
        return;
    }

    std::string sidxContent = downloadContent_;
    if (CheckToDownloadSidxWithInitSeg(currentDownloadStream_)) {
        currentDownloadStream_->initSegment_->content_ = downloadContent_;
        currentDownloadStream_->initSegment_->isDownloadFinish_ = true;
        MEDIA_LOG_I("ParseSidx update init segment content size is "
        PUBLIC_LOG_ZU, currentDownloadStream_->initSegment_->content_.size());
        int64_t initLen = currentDownloadStream_->indexSegment_->indexRangeBegin_ -
                          currentDownloadStream_->initSegment_->rangeBegin_;
        if (initLen >= 0 && (unsigned int)initLen < downloadContent_.size()) {
            sidxContent = downloadContent_.substr((unsigned int)initLen);
            MEDIA_LOG_I("ParseSidx update sidx segment content size is "
            PUBLIC_LOG_ZU
            ", "
            PUBLIC_LOG_C
            "-"
            PUBLIC_LOG_C, sidxContent.size(), sidxContent[0], sidxContent[1]);
        }
    }

    std::list<std::shared_ptr<SubSegmentIndex>> subSegIndexList;
    int32_t parseRet = SidxBoxParser::ParseSidxBox(const_cast<char *>(sidxContent.c_str()), sidxContent.length(),
                                                   currentDownloadStream_->indexSegment_->indexRangeEnd_,
                                                   subSegIndexList);
    if (parseRet != 0) {
        MEDIA_LOG_E("sidx box parse error");
        return;
    }

    BuildDashSegment(subSegIndexList);
    currentDownloadStream_->segsState_ = DASH_SEGS_STATE_FINISH;
    if (!notifyOpenOk_) {
        if (!PutStreamToDownload()) {
            if (callback_ != nullptr) {
                callback_->OnMpdInfoUpdate(DASH_MPD_EVENT_STREAM_INIT);
            }
            
            notifyOpenOk_ = true;
        }
    } else {
        if (callback_ != nullptr) {
            callback_->OnMpdInfoUpdate(DASH_MPD_EVENT_PARSE_OK);
        }
    }
}

void DashMpdDownloader::BuildDashSegment(std::list<std::shared_ptr<SubSegmentIndex>> &subSegIndexList) const
{
    uint64_t segDurSum = 0; // the sum of segment duration, not devide timescale
    unsigned int segDurMsSum; // the sum of segment duration(ms), devide timescale
    unsigned int segAddDuration = 0; // add all segments duration(ms) before current segment
    unsigned int durationMS; // segment real duration in ms
    int64_t segSeq = currentDownloadStream_->startNumberSeq_;
    for (auto subSegIndex : subSegIndexList) {
        durationMS = (static_cast<uint64_t>(subSegIndex->duration_) * S_2_MS) / subSegIndex->timeScale_;
        segDurSum += subSegIndex->duration_;
        segDurMsSum = static_cast<unsigned int>((segDurSum * S_2_MS) / subSegIndex->timeScale_);
        if (segDurMsSum > segAddDuration) {
            durationMS = segDurMsSum - segAddDuration;
        }

        segAddDuration += durationMS;

        DashSegment srcSegment;
        srcSegment.streamId_ = currentDownloadStream_->streamId_;
        srcSegment.bandwidth_ = currentDownloadStream_->bandwidth_;
        srcSegment.duration_ = durationMS;
        srcSegment.startNumberSeq_ = currentDownloadStream_->startNumberSeq_;
        srcSegment.numberSeq_ = segSeq++;
        srcSegment.startRangeValue_ = subSegIndex->startPos_;
        srcSegment.endRangeValue_ = subSegIndex->endPos_;
        srcSegment.url_ = currentDownloadStream_->indexSegment_->url_; // only store url in stream in Dash On-Demand
        srcSegment.byteRange_ = "";
        if (DASH_SEGMENT_INIT_FAILED == AddOneSegment(srcSegment, currentDownloadStream_)) {
            MEDIA_LOG_E("ParseSidx AddOneSegment is failed");
        }
    }
}

void DashMpdDownloader::DoOpen(const std::string& url, int64_t startRange, int64_t endRange)
{
    downloadContent_.clear();
    auto realStatusCallback = [this](DownloadStatus &&status, std::shared_ptr<Downloader> &downloader,
                                     std::shared_ptr<DownloadRequest> &request) {
        if (statusCallback_ != nullptr) {
            statusCallback_(status, downloader_, std::forward<decltype(request)>(request));
        }
    };

    bool requestWholeFile = true;
    if (startRange > 0 || endRange > 0) {
        requestWholeFile = false;
    }

    MEDIA_LOG_I("DoOpen:start=%{public}lld end=%{public}lld url=%{public}s", (long long) startRange,
                (long long) endRange, url.c_str());
    downloadRequest_ = std::make_shared<DownloadRequest>(url, dataSave_, realStatusCallback, requestWholeFile);
    if (downloadRequest_ == nullptr) {
        MEDIA_LOG_I("DoOpen downloadRequest_ is nullptr");
        return;
    }
    auto downloadDoneCallback = [this](const std::string &url, const std::string &location) {
        UpdateDownloadFinished(url);
    };
    downloadRequest_->SetDownloadDoneCb(downloadDoneCallback);

    if (!requestWholeFile) {
        downloadRequest_->SetRangePos(startRange, endRange);
    }
    downloader_->Download(downloadRequest_, -1); // -1
    downloader_->Start();
}

bool DashMpdDownloader::SaveData(uint8_t* data, uint32_t len)
{
    MEDIA_LOG_I("SaveData:size=%{public}u len=%{public}u", (unsigned int)downloadContent_.size(), len);
    downloadContent_.append(reinterpret_cast<const char*>(data), len);
    return true;
}

void DashMpdDownloader::SetMpdCallback(DashMpdCallback* callback)
{
    callback_ = callback;
}

int64_t DashMpdDownloader::GetDuration() const
{
    MEDIA_LOG_I("GetDuration " PUBLIC_LOG_U32, duration_);
    return (duration_ > 0) ? (duration_ * MS_2_NS) : 0;
}

Seekable DashMpdDownloader::GetSeekable() const
{
    // need wait mpdInfo_ not null
    while (true) {
        if (mpdInfo_ != nullptr && notifyOpenOk_) {
            break;
        }
        OSAL::SleepFor(1);
    }
    MEDIA_LOG_I("GetSeekable end");
    return mpdInfo_->type_ == DashType::DASH_TYPE_STATIC ? Seekable::SEEKABLE : Seekable::UNSEEKABLE;
}

void DashMpdDownloader::SelectBitRate(uint32_t bitRate)
{
    MEDIA_LOG_I("SelectBitRate bitRate " PUBLIC_LOG_U32, bitRate);
    if (mpdInfo_ == nullptr || streamDescriptions_.empty()) {
        return;
    }

    if (IsBitrateSame(bitRate) || selectVideoStreamId_ == -1) {
        MEDIA_LOG_W("SelectBitRate is same bitRate or not exit select stream.");
        return;
    }

    std::shared_ptr<DashStreamDescription> inUseVideoStream;
    for (auto &stream : streamDescriptions_) {
        if (stream->inUse_ && stream->type_ == MediaAVCodec::MEDIA_TYPE_VID) {
            inUseVideoStream = stream;
            break;
        }
    }

    if (inUseVideoStream == nullptr) {
        MEDIA_LOG_W("SelectBitRate is failed, not exist use stream.");
        return;
    }

    for (auto &stream : streamDescriptions_) {
        if (stream->type_ != MediaAVCodec::MEDIA_TYPE_VID || stream->inUse_) {
            continue;
        }

        if (stream->streamId_ == selectVideoStreamId_) {
            stream->currentNumberSeq_ = inUseVideoStream->currentNumberSeq_;
            MEDIA_LOG_I("SelectBitRate update id:"
            PUBLIC_LOG_D32
            ", seq:"
            PUBLIC_LOG_D64, stream->streamId_, stream->currentNumberSeq_);
            inUseVideoStream->inUse_ = false;
            stream->inUse_ = true;
            if (!ondemandSegBase_) {
                GetSegmentsInMpd(stream);
            }
            return;
        }
    }
}

std::vector<uint32_t> DashMpdDownloader::GetBitRates()
{
    std::vector<uint32_t> bitRates;
    for (const auto &item : streamDescriptions_) {
        if (item->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID && item->bandwidth_) {
            bitRates.push_back(item->bandwidth_);
        }
    }
    return bitRates;
}

bool DashMpdDownloader::IsBitrateSame(uint32_t bitRate)
{
    selectVideoStreamId_ = -1;
    uint32_t maxGap = 0;
    bool isFirstSelect = true;
    std::shared_ptr<DashStreamDescription> currentStream;
    std::shared_ptr<DashStreamDescription> newStream;
    for (const auto &item: streamDescriptions_) {
        if (item->type_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
            continue;
        }

        if (item->inUse_) {
            currentStream = item;
            continue;
        }

        uint32_t tempGap = (item->bandwidth_ > bitRate) ? (item->bandwidth_ - bitRate) : (bitRate - item->bandwidth_);
        if (isFirstSelect || (tempGap < maxGap)) {
            isFirstSelect = false;
            maxGap = tempGap;
            newStream = item;
            selectVideoStreamId_ = newStream->streamId_;
        }
    }
    if (newStream == nullptr || (currentStream != nullptr && (newStream->bandwidth_ == currentStream->bandwidth_))) {
        return true;
    }
    return false;
}

void DashMpdDownloader::SeekToTs(int streamId, int64_t seekTime, std::shared_ptr<DashSegment>& seg)
{
    seg = nullptr;
    std::vector<std::shared_ptr<DashSegment>> mediaSegments;
    std::shared_ptr<DashStreamDescription> streamDescription;
    for (const auto &index : streamDescriptions_) {
        streamDescription = index;
        if (streamDescription != nullptr && streamDescription->streamId_ == streamId &&
            streamDescription->segsState_ == DASH_SEGS_STATE_FINISH) {
            mediaSegments = streamDescription->mediaSegments_;
            break;
        }
    }

    if (!mediaSegments.empty()) {
        int64_t totalDuration = 0;
        for (auto &mediaSegment : mediaSegments) {
            if (mediaSegment == nullptr) {
                continue;
            }

            totalDuration += mediaSegment->duration_;
            if (totalDuration > seekTime) {
                seg = mediaSegment;
                MEDIA_LOG_I("Dash SeekToTs segment totalDuration:"
                PUBLIC_LOG_D64
                ", segNum:"
                PUBLIC_LOG_D64
                ", duration:"
                PUBLIC_LOG_U32,
                        totalDuration, mediaSegment->numberSeq_, mediaSegment->duration_);
                return;
            }
        }
    }
}

/**
 * @brief    get segments in Period with SegmentTemplate or SegmentList or BaseUrl
 *
 * @param    periodInfo             current Period
 * @param    adptSetInfo            current AdptSet
 * @param    periodBaseUrl          Mpd.BaseUrl
 * @param    streamDesc             stream description
 *
 * @return   failed success undo
 */
DashSegmentInitValue DashMpdDownloader::GetSegmentsByPeriodInfo(DashPeriodInfo *periodInfo,
                                                                DashAdptSetInfo *adptSetInfo,
                                                                std::string &periodBaseUrl,
                                                                std::shared_ptr<DashStreamDescription> streamDesc)
{
    DashSegmentInitValue initVal;
    // get segments from Period.SegmentTemplate
    if (periodInfo->periodSegTmplt_ != nullptr && periodInfo->periodSegTmplt_->segTmpltMedia_.length() > 0) {
        DashAppendBaseUrl(periodBaseUrl, periodInfo->baseUrl_);

        std::string representationId;
        // get Representation@id in the chosen AdaptationSet
        if (adptSetInfo != nullptr && streamDesc->bandwidth_ > 0) {
            DashRepresentationInfo *repInfo = GetRepresentationFromAdptSet(adptSetInfo,
                                                                           streamDesc->representationIndex_);
            if (repInfo != nullptr) {
                representationId = repInfo->id_;
            }
        }

        initVal = GetSegmentsWithSegTemplate(periodInfo->periodSegTmplt_, representationId, streamDesc);
    } else if (periodInfo->periodSegList_ != nullptr && periodInfo->periodSegList_->segmentUrl_.size() > 0) {
        // get segments from Period.SegmentList
        DashAppendBaseUrl(periodBaseUrl, periodInfo->baseUrl_);
        initVal = GetSegmentsWithSegList(periodInfo->periodSegList_, periodBaseUrl, streamDesc);
    } else if (periodInfo->baseUrl_.size() > 0) {
        // get segments fromn Period.BaseUrl
        initVal = GetSegmentsWithBaseUrl(periodInfo->baseUrl_, streamDesc);
    } else {
        initVal = DASH_SEGMENT_INIT_UNDO;
    }

    return initVal;
}

void DashMpdDownloader::SetOndemandSegBase()
{
    for (auto periodInfo : mpdInfo_->periodInfoList_) {
        if (SetOndemandSegBase(periodInfo->adptSetList_)) {
            break;
        }
    }

    MEDIA_LOG_I("dash onDemandSegBase is " PUBLIC_LOG_D32, ondemandSegBase_);
}

bool DashMpdDownloader::SetOndemandSegBase(std::list<DashAdptSetInfo*> adptSetList)
{
    for (auto adptSetInfo : adptSetList) {
        if (adptSetInfo->representationList_.size() == 0) {
            if (adptSetInfo->adptSetSegBase_ != nullptr && adptSetInfo->adptSetSegBase_->indexRange_.size() > 0) {
                ondemandSegBase_ = true;
                return true;
            }
        } else {
            if (SetOndemandSegBase(adptSetInfo->representationList_)) {
                return true;
            }
        }
    }

    return false;
}

bool DashMpdDownloader::SetOndemandSegBase(std::list<DashRepresentationInfo*> repList)
{
    for (auto rep : repList) {
        if (rep->representationSegList_ != nullptr || rep->representationSegTmplt_ != nullptr) {
            MEDIA_LOG_I("dash representation contain segmentlist or template");
            ondemandSegBase_ = false;
            return true;
        }

        if (rep->representationSegBase_ != nullptr && rep->representationSegBase_->indexRange_.size() > 0) {
            MEDIA_LOG_I("dash representation contain indexRange");
            ondemandSegBase_ = true;
            return true;
        }
    }

    return false;
}

bool DashMpdDownloader::CheckToDownloadSidxWithInitSeg(std::shared_ptr<DashStreamDescription> streamDesc)
{
    if (streamDesc->initSegment_ != nullptr &&
        streamDesc->initSegment_->url_.compare(streamDesc->indexSegment_->url_) == 0 &&
        streamDesc->indexSegment_->indexRangeBegin_ > streamDesc->initSegment_->rangeEnd_) {
        return true;
    }

    return false;
}

bool DashMpdDownloader::GetStreamsInfoInMpd()
{
    if (mpdInfo_ == nullptr) {
        return false;
    }

    mpdManager_->SetMpdInfo(mpdInfo_, url_);
    std::string mpdBaseUrl = mpdManager_->GetBaseUrl();
    std::list<DashPeriodInfo*> periods = mpdInfo_->periodInfoList_;
    unsigned int periodIndex = 0;
    for (std::list<DashPeriodInfo*>::iterator it = periods.begin(); it != periods.end(); ++it, ++periodIndex) {
        DashPeriodInfo *period = *it;
        if (period == nullptr) {
            continue;
        }

        GetStreamsInfoInPeriod(period, periodIndex, mpdBaseUrl);
    }

    int size = static_cast<int>(streamDescriptions_.size());
    for (int index = 0; index < size; index++) {
        streamDescriptions_[index]->streamId_ = index;
        std::shared_ptr<DashInitSegment> initSegment = streamDescriptions_[index]->initSegment_;
        if (initSegment != nullptr) {
            initSegment->streamId_ = index;
        }
    }

    return true;
}

void DashMpdDownloader::GetStreamsInfoInPeriod(DashPeriodInfo *periodInfo, unsigned int periodIndex,
                                               std::string mpdBaseUrl)
{
    periodManager_->SetPeriodInfo(periodInfo);
    DashStreamDescription streamDesc;
    streamDesc.duration_ = (periodInfo->duration_ > 0) ? periodInfo->duration_ : duration_;
    streamDesc.periodIndex_ = periodIndex;
    std::string periodBaseUrl = mpdBaseUrl;
    DashAppendBaseUrl(periodBaseUrl, periodInfo->baseUrl_);

    if (periodInfo->adptSetList_.size() == 0) {
        streamDesc.startNumberSeq_ = GetStartNumber(periodInfo);
        // no adaptationset in period, store as video stream
        std::shared_ptr<DashStreamDescription> desc = std::make_shared<DashStreamDescription>(streamDesc);

        GetInitSegFromPeriod(periodBaseUrl, "", desc);
        if (ondemandSegBase_ && periodInfo->periodSegBase_ != nullptr &&
            periodInfo->periodSegBase_->indexRange_.size() > 0) {
            desc->indexSegment_ = std::make_shared<DashIndexSegment>();
            desc->indexSegment_->url_ = periodBaseUrl;
            DashParseRange(periodInfo->periodSegBase_->indexRange_, desc->indexSegment_->indexRangeBegin_,
                           desc->indexSegment_->indexRangeEnd_);
        }
        streamDescriptions_.push_back(desc);
        return;
    }

    std::vector<DashAdptSetInfo*> adptSetVector;
    DashAdptSetInfo *adptSetInfo = nullptr;
    for (int32_t type = MediaAVCodec::MediaType::MEDIA_TYPE_AUD;
         type <= MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE; type++) {
        streamDesc.type_ = (MediaAVCodec::MediaType)type;
        periodManager_->GetAdptSetsByStreamType(adptSetVector, streamDesc.type_);
        if (adptSetVector.size() == 0) {
            MEDIA_LOG_I("Get streamType " PUBLIC_LOG_D32 " in period " PUBLIC_LOG_U32 " size is 0", type, periodIndex);
            continue;
        }

        for (unsigned int adptSetIndex = 0; adptSetIndex < adptSetVector.size(); adptSetIndex++) {
            streamDesc.adptSetIndex_ = adptSetIndex;
            adptSetInfo = adptSetVector[adptSetIndex];
            GetStreamsInfoInAdptSet(adptSetInfo, periodBaseUrl, streamDesc);
        }
    }
}

void DashMpdDownloader::GetStreamsInfoInAdptSet(DashAdptSetInfo *adptSetInfo, std::string periodBaseUrl,
                                                DashStreamDescription &streamDesc)
{
    streamDesc.width_ = adptSetInfo->commonAttrsAndElements_.width_;
    streamDesc.height_ = adptSetInfo->commonAttrsAndElements_.height_;
    std::string adptSetBaseUrl = periodBaseUrl;
    DashAppendBaseUrl(adptSetBaseUrl, adptSetInfo->baseUrl_);
    adptSetManager_->SetAdptSetInfo(adptSetInfo);
    streamDesc.isHdr_ = adptSetManager_->IsHdr();
    MEDIA_LOG_I("GetStreamsInfoInAdptSet isHdr " PUBLIC_LOG_U32, streamDesc.isHdr_);

    std::list<DashRepresentationInfo*> repList = adptSetInfo->representationList_;
    if (repList.size() == 0) {
        MEDIA_LOG_I("representation count is 0 in adptset index " PUBLIC_LOG_U32, streamDesc.adptSetIndex_);
        streamDesc.startNumberSeq_ = GetStartNumber(adptSetInfo);
        std::shared_ptr<DashStreamDescription> desc = std::make_shared<DashStreamDescription>(streamDesc);
        if (!GetInitSegFromAdptSet(adptSetBaseUrl, "", desc)) {
            GetInitSegFromPeriod(periodBaseUrl, "", desc);
        }

        if (ondemandSegBase_ && adptSetInfo->adptSetSegBase_ != nullptr &&
            adptSetInfo->adptSetSegBase_->indexRange_.size() > 0) {
            desc->indexSegment_ = std::make_shared<DashIndexSegment>();
            desc->indexSegment_->url_ = adptSetBaseUrl;
            DashParseRange(adptSetInfo->adptSetSegBase_->indexRange_, desc->indexSegment_->indexRangeBegin_,
                           desc->indexSegment_->indexRangeEnd_);
        }
        streamDescriptions_.push_back(desc);
        return;
    }

    GetStreamDescriptions(periodBaseUrl, streamDesc, adptSetBaseUrl, repList);
}

void DashMpdDownloader::GetStreamDescriptions(std::string &periodBaseUrl, DashStreamDescription &streamDesc,
                                              std::string &adptSetBaseUrl,
                                              std::list<DashRepresentationInfo *> &repList)
{
    std::string repBaseUrl;
    unsigned int repIndex = 0;
    for (std::list<DashRepresentationInfo*>::iterator it = repList.begin(); it != repList.end(); it++, repIndex++) {
        if (*it != nullptr) {
            repBaseUrl = adptSetBaseUrl;
            streamDesc.representationIndex_ = repIndex;
            streamDesc.startNumberSeq_ = GetStartNumber(*it);
            streamDesc.bandwidth_ = (*it)->bandwidth_;
            if ((*it)->commonAttrsAndElements_.width_ > 0) {
                streamDesc.width_ = (*it)->commonAttrsAndElements_.width_;
            }

            if ((*it)->commonAttrsAndElements_.height_ > 0) {
                streamDesc.height_ = (*it)->commonAttrsAndElements_.height_;
            }

            std::shared_ptr<DashStreamDescription> desc = std::make_shared<DashStreamDescription>(streamDesc);
            representationManager_->SetRepresentationInfo(*it);
            DashAppendBaseUrl(repBaseUrl, (*it)->baseUrl_);
            if (!GetInitSegFromRepresentation(repBaseUrl, (*it)->id_, desc)) {
                if (!GetInitSegFromAdptSet(adptSetBaseUrl, (*it)->id_, desc)) {
                    GetInitSegFromPeriod(periodBaseUrl, (*it)->id_, desc);
                }
            }
            if (ondemandSegBase_ && (*it)->representationSegBase_ != nullptr &&
                (*it)->representationSegBase_->indexRange_.size() > 0) {
                desc->indexSegment_ = std::make_shared<DashIndexSegment>();
                desc->indexSegment_->url_ = repBaseUrl;
                DashParseRange((*it)->representationSegBase_->indexRange_, desc->indexSegment_->indexRangeBegin_,
                               desc->indexSegment_->indexRangeEnd_);
            }

            streamDescriptions_.push_back(desc);
        }
    }
}

bool DashMpdDownloader::ChooseStreamToPlay(MediaAVCodec::MediaType type)
{
    if (mpdInfo_ == nullptr || streamDescriptions_.size() == 0) {
        return false;
    }

    std::shared_ptr<DashStreamDescription> firstStream = nullptr;
    std::shared_ptr<DashStreamDescription> choosedStream = nullptr;
    for (auto stream : streamDescriptions_) {
        if (stream->type_ == type && !stream->inUse_) {
            if (firstStream == nullptr) {
                firstStream = stream;
            }

            if (type == MediaAVCodec::MediaType::MEDIA_TYPE_VID && stream->isHdr_ != isHdrStart_) {
                // skip video stream that hdr value not same
                continue;
            }
            choosedStream = stream;
        }
    }

    if (choosedStream == nullptr && type == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        MEDIA_LOG_I("ChooseStreamToPlay video can not find hdrstart:" PUBLIC_LOG_D32, isHdrStart_);
        choosedStream = firstStream;
    }

    if (choosedStream != nullptr) {
        choosedStream->inUse_ = true;
        MEDIA_LOG_I("ChooseStreamToPlay type:"
        PUBLIC_LOG_D32
        ", streamId:"
        PUBLIC_LOG_D32, (int) type, choosedStream->streamId_);
        if (!ondemandSegBase_) {
            GetSegmentsInMpd(choosedStream);
        }
        return true;
    }

    MEDIA_LOG_I("ChooseStreamToPlay false type:" PUBLIC_LOG_D32, (int)type);
    return false;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInMpd(std::shared_ptr<DashStreamDescription> streamDesc)
{
    if (mpdInfo_ == nullptr || streamDesc == nullptr) {
        MEDIA_LOG_E("GetSegments but mpdInfo_ or streamDesc is nullptr");
        return DASH_SEGMENT_INIT_FAILED;
    }

    streamDesc->segsState_ = DASH_SEGS_STATE_FINISH;
    std::string mpdBaseUrl;
    std::list<std::string> baseUrlList;
    mpdManager_->SetMpdInfo(mpdInfo_, url_);
    mpdBaseUrl = mpdManager_->GetBaseUrl();

    unsigned int currentPeriodIndex = 0;
    for (std::list<DashPeriodInfo *>::iterator it = mpdInfo_->periodInfoList_.begin();
         it != mpdInfo_->periodInfoList_.end(); it++, currentPeriodIndex++) {
        if (*it != nullptr && currentPeriodIndex == streamDesc->periodIndex_) {
            if (GetSegmentsInPeriod(*it, mpdBaseUrl, streamDesc) == DASH_SEGMENT_INIT_FAILED) {
                MEDIA_LOG_I("GetSegmentsInPeriod"
                PUBLIC_LOG_U32
                " failed, type "
                PUBLIC_LOG_D32, currentPeriodIndex, streamDesc->type_);
                return DASH_SEGMENT_INIT_FAILED;
            }

            break;
        }
    }

    if (streamDesc->mediaSegments_.size() == 0) {
        MEDIA_LOG_E("GetSegmentsInMpd failed, type " PUBLIC_LOG_D32, streamDesc->type_);
        return DASH_SEGMENT_INIT_FAILED;
    }

    return DASH_SEGMENT_INIT_SUCCESS;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInPeriod(DashPeriodInfo *periodInfo, std::string mpdBaseUrl,
                                                            std::shared_ptr<DashStreamDescription> streamDesc)
{
    std::vector<DashAdptSetInfo*> adptSetVector;
    DashSegmentInitValue initValue = DASH_SEGMENT_INIT_UNDO;
    std::string periodBaseUrl = mpdBaseUrl;
    periodManager_->SetPeriodInfo(periodInfo);
    periodManager_->GetAdptSetsByStreamType(adptSetVector, streamDesc->type_);
    DashAdptSetInfo *adptSetInfo = nullptr;
    
    if (streamDesc->adptSetIndex_ < adptSetVector.size()) {
        adptSetInfo = adptSetVector[streamDesc->adptSetIndex_];
        DashAppendBaseUrl(periodBaseUrl, periodInfo->baseUrl_);
        initValue = GetSegmentsInAdptSet(adptSetInfo, periodBaseUrl, streamDesc);
        if (initValue != DASH_SEGMENT_INIT_UNDO) {
            return initValue;
        }
    }

    // segments in period.segmentList/segmentTemplate/baseurl, should store in video stream manager
    if (streamDesc->type_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        return initValue;
    }

    // UST182 base to relative segment url
    periodBaseUrl = mpdBaseUrl;
    initValue = GetSegmentsByPeriodInfo(periodInfo, adptSetInfo, periodBaseUrl, streamDesc);
    if (initValue == DASH_SEGMENT_INIT_SUCCESS) {
        MakeAbsoluteWithBaseUrl(streamDesc->mediaSegments_, periodBaseUrl);
    }

    return initValue;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInAdptSet(DashAdptSetInfo *adptSetInfo, std::string periodBaseUrl,
                                                             std::shared_ptr<DashStreamDescription> streamDesc)
{
    if (adptSetInfo == nullptr) {
        return DASH_SEGMENT_INIT_UNDO;
    }

    DashSegmentInitValue initValue = DASH_SEGMENT_INIT_UNDO;
    DashRepresentationInfo* repInfo = nullptr;
    std::string adptSetBaseUrl = periodBaseUrl;

    if (streamDesc->representationIndex_ < adptSetInfo->representationList_.size()) {
        repInfo = GetRepresemtationFromAdptSet(adptSetInfo, streamDesc->representationIndex_);
        if (repInfo != nullptr) {
            DashAppendBaseUrl(adptSetBaseUrl, adptSetInfo->baseUrl_);
            initValue = GetSegmentsInRepresentation(repInfo, adptSetBaseUrl, streamDesc);
        }

        if (initValue != DASH_SEGMENT_INIT_UNDO) {
            return initValue;
        }
    }

    adptSetBaseUrl = periodBaseUrl;
    initValue = GetSegmentsByAdptSetInfo(adptSetInfo, repInfo, adptSetBaseUrl, streamDesc);
    if (initValue == DASH_SEGMENT_INIT_SUCCESS) {
        MakeAbsoluteWithBaseUrl(streamDesc->mediaSegments_, adptSetBaseUrl);
    }

    return initValue;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInRepresentation(DashRepresentationInfo *repInfo,
                                                                    std::string adptSetBaseUrl,
                                                                    std::shared_ptr<DashStreamDescription> streamDesc)
{
    std::string repBaseUrl = adptSetBaseUrl;
    DashSegmentInitValue initValue = DASH_SEGMENT_INIT_UNDO;
    if (repInfo->representationSegTmplt_ != nullptr && repInfo->representationSegTmplt_->segTmpltMedia_.length() > 0) {
        DashAppendBaseUrl(repBaseUrl, repInfo->baseUrl_);
        initValue = GetSegmentsWithSegTemplate(repInfo->representationSegTmplt_, repInfo->id_, streamDesc);
    } else if (repInfo->representationSegList_ != nullptr && repInfo->representationSegList_->segmentUrl_.size() > 0) {
        // get segments from Representation.SegmentList
        DashAppendBaseUrl(repBaseUrl, repInfo->baseUrl_);
        initValue = GetSegmentsWithSegList(repInfo->representationSegList_, repBaseUrl, streamDesc);
    } else if (repInfo->baseUrl_.size() > 0) {
        // get one segment from Representation.BaseUrl
        initValue = GetSegmentsWithBaseUrl(repInfo->baseUrl_, streamDesc);
    }

    if (initValue == DASH_SEGMENT_INIT_SUCCESS) {
        MakeAbsoluteWithBaseUrl(streamDesc->mediaSegments_, repBaseUrl);
    }

    return initValue;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsWithSegTemplate(const DashSegTmpltInfo *segTmpltInfo, std::string id,
                                                                   std::shared_ptr<DashStreamDescription> streamDesc)
{
    std::string media = segTmpltInfo->segTmpltMedia_;
    std::string::size_type posIden = media.find("$$");
    if (posIden != std::string::npos) {
        size_t strLength = strlen("$$");
        media.replace(posIden, strLength, "$");
    }

    if (DashSubstituteTmpltStr(media, "$RepresentationID", id) == -1) {
        MEDIA_LOG_E("media "
        PUBLIC_LOG_S
        " substitute $RepresentationID error "
        PUBLIC_LOG_S, media.c_str(), id.c_str());
        return DASH_SEGMENT_INIT_FAILED;
    }

    if (DashSubstituteTmpltStr(media, "$Bandwidth", std::to_string(streamDesc->bandwidth_)) == -1) {
        MEDIA_LOG_E("media "
        PUBLIC_LOG_S
        " substitute $Bandwidth error "
        PUBLIC_LOG_D32, media.c_str(), streamDesc->bandwidth_);
        return DASH_SEGMENT_INIT_FAILED;
    }

    streamDesc->startNumberSeq_ = ParseStartNumber(segTmpltInfo->multSegBaseInfo_.startNumber_);
    if (mpdInfo_->type_ == DashType::DASH_TYPE_STATIC) {
        return GetSegmentsWithTmpltStatic(segTmpltInfo, media, streamDesc);
    }

    return DASH_SEGMENT_INIT_FAILED;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltStatic(const DashSegTmpltInfo *segTmpltInfo,
                                                                   std::string mediaUrl,
                                                                   std::shared_ptr<DashStreamDescription> streamDesc)
{
    unsigned int timeScale = 1;
    if (segTmpltInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ > 0) {
        timeScale = segTmpltInfo->multSegBaseInfo_.segBaseInfo_.timeScale_;
    }

    if (segTmpltInfo->multSegBaseInfo_.duration_ > 0) {
        return GetSegmentsWithTmpltDurationStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    } else if (segTmpltInfo->multSegBaseInfo_.segTimeline_.size() > 0) {
        return GetSegmentsWithTmpltTimelineStatic(segTmpltInfo, mediaUrl, timeScale, streamDesc);
    } else {
        MEDIA_LOG_E("static SegmentTemplate do not have segment duration");
        return DASH_SEGMENT_INIT_FAILED;
    }
}

/**
 * @brief    Get segments with SegmentTemplate in static as contain SegmentBase.duration
 *
 * @param    segTmpltInfo           SegmentTemplate infomation
 * @param    mediaUrl               SegmentTemplate mediaSegment Url
 * @param    timeScale              the timeScale of this period, duration / timescale = seconds
 * @param    desc             stream description
 *
 * @return   DashSegmentInitValue
 */
DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltDurationStatic(const DashSegTmpltInfo *segTmpltInfo,
                                                                           std::string mediaUrl,
                                                                           unsigned int timeScale,
                                                                           std::shared_ptr<DashStreamDescription> desc)
{
    if (timeScale == 0) {
        return DASH_SEGMENT_INIT_FAILED;
    }

    // the segment duration in millisecond
    unsigned int segmentDuration =
            (static_cast<uint64_t>(segTmpltInfo->multSegBaseInfo_.duration_) * S_2_MS) / timeScale;
    if (segmentDuration == 0) {
        return DASH_SEGMENT_INIT_FAILED;
    }

    uint64_t segmentBaseDurationSum = 0; // the sum of segment duration(ms), not devide timescale
    unsigned int durationSumWithScale = 0; // the sum of segment duration(ms), devide timescale
    unsigned int accumulateDuration = 0; // add all segments dutation(ms) before current segment
    unsigned int segRealDur = 0;
    int64_t segmentSeq = -1;
    int64_t startTime = 0;
    std::string tempUrl;

    while (accumulateDuration < desc->duration_) {
        segmentBaseDurationSum += segTmpltInfo->multSegBaseInfo_.duration_;
        durationSumWithScale = static_cast<unsigned int>(segmentBaseDurationSum * S_2_MS / timeScale);
        if (durationSumWithScale > accumulateDuration) {
            segRealDur = durationSumWithScale - accumulateDuration;
        } else {
            segRealDur = segmentDuration;
        }

        if (desc->duration_ - accumulateDuration < segRealDur) {
            segRealDur = desc->duration_ - accumulateDuration;
        }

        accumulateDuration += segRealDur;

        if (segmentSeq == -1) {
            segmentSeq = desc->startNumberSeq_;
        } else {
            segmentSeq++;
        }

        // if the @duration attribute is present
        // then the time address is determined by replacing the $Time$ identifier with ((k-1) + (kStart-1))* @duration
        // with kStart the value of the @startNumber attribute, if present, or 1 otherwise.
        startTime = (segmentSeq - 1) * segTmpltInfo->multSegBaseInfo_.duration_;
        tempUrl = mediaUrl;

        if (DashSubstituteTmpltStr(tempUrl, "$Time", std::to_string(startTime)) == -1) {
            MEDIA_LOG_I(""
            PUBLIC_LOG_S
            " substitute $Time "
            PUBLIC_LOG_S
            " error in static duration", tempUrl.c_str(), std::to_string(startTime).c_str());
            return DASH_SEGMENT_INIT_FAILED;
        }

        if (DashSubstituteTmpltStr(tempUrl, "$Number", std::to_string(segmentSeq)) == -1) {
            MEDIA_LOG_I(""
            PUBLIC_LOG_S
            " substitute $Number "
            PUBLIC_LOG_S
            " error in static duration", tempUrl.c_str(), std::to_string(segmentSeq).c_str());
            return DASH_SEGMENT_INIT_FAILED;
        }

        if (DASH_SEGMENT_INIT_FAILED == AddOneSegment(segRealDur, segmentSeq, tempUrl, desc)) {
            MEDIA_LOG_I("AddOneSegment failed with [static] [duration]");
            return DASH_SEGMENT_INIT_FAILED;
        }
    }

    MEDIA_LOG_I("GetSegmentsWithTmpltDurationStatic:segment size:" PUBLIC_LOG_ZU, desc->mediaSegments_.size());
    return DASH_SEGMENT_INIT_SUCCESS;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltTimelineStatic(const DashSegTmpltInfo *segTmpltInfo,
                                                                           std::string mediaUrl,
                                                                           unsigned int timeScale,
                                                                           std::shared_ptr<DashStreamDescription> desc)
{
    if (timeScale == 0) {
        return DASH_SEGMENT_INIT_FAILED;
    }

    int64_t segmentSeq = -1;
    uint64_t startTime = 0;
    std::list<DashSegTimeline*> timelineList = segTmpltInfo->multSegBaseInfo_.segTimeline_;
    for (std::list<DashSegTimeline*>::iterator it = timelineList.begin(); it != timelineList.end(); it++) {
        if (*it != nullptr) {
            int segCount = GetSegCountFromTimeline(it, timelineList.end(), desc->duration_, timeScale, startTime);
            if (segCount < 0) {
                MEDIA_LOG_W("get segment count error in SegmentTemplate.SegmentTimeline");
                continue;
            }

            unsigned int segDuration = ((*it)->d_ * S_2_MS) / timeScale;
            MediaSegSampleInfo sampleInfo;
            sampleInfo.mediaUrl_ = mediaUrl;
            sampleInfo.segCount_ = segCount;
            sampleInfo.segDuration_ = segDuration;
            if (GetSegmentsInOneTimeline(*it, sampleInfo, segmentSeq, startTime, desc) ==
                DASH_SEGMENT_INIT_FAILED) {
                return DASH_SEGMENT_INIT_FAILED;
            }
        }
    }

    return DASH_SEGMENT_INIT_SUCCESS;
}

/**
 * @brief    Get segments in ome timeline
 *
 * @param    timeline               Segment Timeline infomation
 * @param    sampleInfo               segment count duration Url in timeline
 * @param    segmentSeq[out]        segment segquence in timeline
 * @param    startTime[out]         segment start time in timeline
 * @param    streamDesc[out]        stream description, segments store in it
 *
 * @return   0 indicates success, -1 indicates failed
 */
DashSegmentInitValue DashMpdDownloader::GetSegmentsInOneTimeline(const DashSegTimeline *timeline,
                                                                 const MediaSegSampleInfo &sampleInfo,
                                                                 int64_t &segmentSeq, uint64_t &startTime,
                                                                 std::shared_ptr<DashStreamDescription> streamDesc)
{
    int repeat = 0;
    std::string tempUrl;

    while (repeat <= sampleInfo.segCount_) {
        repeat++;
        if (segmentSeq == -1) {
            segmentSeq = streamDesc->startNumberSeq_;
        } else {
            segmentSeq++;
        }

        tempUrl = sampleInfo.mediaUrl_;
        if (DashSubstituteTmpltStr(tempUrl, "$Time", std::to_string(startTime)) == -1) {
            MEDIA_LOG_E(""
            PUBLIC_LOG_S
            " substitute $Time "
            PUBLIC_LOG_S
            " error in static timeline", tempUrl.c_str(), std::to_string(startTime).c_str());
            return DASH_SEGMENT_INIT_FAILED;
        }

        if (DashSubstituteTmpltStr(tempUrl, "$Number", std::to_string(segmentSeq)) == -1) {
            MEDIA_LOG_E(""
            PUBLIC_LOG_S
            " substitute $Number "
            PUBLIC_LOG_S
            " error in static timeline", tempUrl.c_str(), std::to_string(segmentSeq).c_str());
            return DASH_SEGMENT_INIT_FAILED;
        }

        if (DASH_SEGMENT_INIT_FAILED == AddOneSegment(sampleInfo.segDuration_, segmentSeq, tempUrl, streamDesc)) {
            MEDIA_LOG_E("AddOneSegment with SegmentTimeline in static is failed");
            return DASH_SEGMENT_INIT_FAILED;
        }

        startTime += timeline->d_;
    }

    return DASH_SEGMENT_INIT_SUCCESS;
}

/**
 * @brief    Get Segment Repeat Count In SegmentTimeline.S
 *
 * @param    it                     the current S element infomation iterator of list
 * @param    end                    the end iterator of list
 * @param    periodDuration         the duration of this period
 * @param    timeScale              the timeScale of this period, duration / timescale = seconds
 * @param    startTime              segment start time in timeline
 *
 * @return   segment repeat count, 0 means only one segment, negative means no segment
 */
int DashMpdDownloader::GetSegCountFromTimeline(DashList<DashSegTimeline *>::iterator &it,
                                               const DashList<DashSegTimeline *>::iterator &end,
                                               unsigned int periodDuration, unsigned int timeScale, uint64_t startTime)
{
    int segCount = (*it)->r_;

    /* A negative value of the @r attribute of the S element indicates that
     * the duration indicated in @d attribute repeats until the start of
     * the next S element, the end of the Period or until the next MPD update.
     */
    if (segCount < 0 && (*it)->d_) {
        ++it;
        // the start of next S
        if (it != end && *it != nullptr) {
            uint64_t nextStartTime = (*it)->t_;
            it--;

            if (nextStartTime <= startTime) {
                MEDIA_LOG_W("r is negative and next S@t "
                PUBLIC_LOG_U64
                " is not larger than startTime "
                PUBLIC_LOG_U64, nextStartTime, startTime);
                return segCount;
            }

            segCount = (nextStartTime - startTime) / (*it)->d_;
        } else {
            // the end of period
            it--;
            uint64_t scaleDuration = (uint64_t)periodDuration * timeScale / S_2_MS;
            if (scaleDuration <= startTime) {
                MEDIA_LOG_W("r is negative, duration "
                PUBLIC_LOG_U32
                " is not larger than startTime "
                PUBLIC_LOG_U64
                ", timeScale "
                PUBLIC_LOG_U32, periodDuration, startTime, timeScale);
                return segCount;
            }

            segCount = (scaleDuration - startTime) / (*it)->d_;
        }

        // 0开始表示1个分片
        segCount -= 1;
    }

    return segCount;
}

/**
 * @brief    get segments with SegmentList
 *
 * @param    segListInfo            SegmentList infomation
 * @param    baseUrl                BaseUrl that map to media attribute if media missed and range present
 * @param    streamDesc             stream description, store segments
 *
 * @return   failed success undo
 */
DashSegmentInitValue DashMpdDownloader::GetSegmentsWithSegList(const DashSegListInfo *segListInfo, std::string baseUrl,
                                                               std::shared_ptr<DashStreamDescription> streamDesc)
{
    int64_t segmentSeq = -1;
    int segmentCount = 0;
    std::list<unsigned int> durationList;

    unsigned int timescale = 1;
    if (segListInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ != 0) {
        timescale = segListInfo->multSegBaseInfo_.segBaseInfo_.timeScale_;
    }

    unsigned int segDuration = (static_cast<uint64_t>(segListInfo->multSegBaseInfo_.duration_) * S_2_MS) / timescale;
    GetSegDurationFromTimeline(streamDesc->duration_, timescale, &segListInfo->multSegBaseInfo_, durationList);

    std::list<DashSegUrl*> segUrlList = segListInfo->segmentUrl_;
    for (std::list<DashSegUrl*>::iterator it = segUrlList.begin(); it != segUrlList.end(); it++) {
        if (*it != nullptr) {
            std::string mediaUrl;
            if ((*it)->media_.length() > 0) {
                mediaUrl = (*it)->media_;
            } else if (baseUrl.length() > 0) {
                mediaUrl = ""; // append baseurl as the media url in MakeAbsoluteWithBaseUrl
            } else {
                continue;
            }

            segmentSeq = streamDesc->startNumberSeq_ + segmentCount;
            DashSegment srcSegment;
            srcSegment.streamId_ = streamDesc->streamId_;
            srcSegment.bandwidth_ = streamDesc->bandwidth_;
            if (durationList.size() > 0) {
                // by SegmentTimeline
                srcSegment.duration_ = durationList.front();

                // if size of SegmentTimeline smaller than size of SegmentURL，
                // keep the last segment duration in SegmentTimeline
                if (durationList.size() > 1) {
                    durationList.pop_front();
                }
            } else {
                // by duration
                srcSegment.duration_ = segDuration;
            }

            srcSegment.startNumberSeq_ = streamDesc->startNumberSeq_;
            srcSegment.numberSeq_ = segmentSeq;
            srcSegment.url_ = mediaUrl;
            srcSegment.byteRange_ = (*it)->mediaRange_;

            if (DASH_SEGMENT_INIT_FAILED == AddOneSegment(srcSegment, streamDesc)) {
                MEDIA_LOG_E("InitSegmentsWithSegList AddOneSegment is failed");
                return DASH_SEGMENT_INIT_FAILED;
            }

            segmentCount++;
        }
    }

    return DASH_SEGMENT_INIT_SUCCESS;
}

/**
 * @brief    Get Segment Duration From SegmentTimeline
 *
 * @param    periodDuration         the duration of this period
 * @param    timeScale              the timeScale of this period, duration / timescale = seconds
 * @param    multSegBaseInfo        multipleSegmentBaseInfomation in segmentlist
 * @param    durationList[out]      the list to store segment duration
 *
 * @return   none
 */
void DashMpdDownloader::GetSegDurationFromTimeline(unsigned int periodDuration, unsigned int timeScale,
                                                   const DashMultSegBaseInfo *multSegBaseInfo,
                                                   DashList<unsigned int> &durationList)
{
    if (timeScale == 0 || mpdInfo_ == nullptr) {
        return;
    }

    // support SegmentList.SegmentTimeline in static type
    if (multSegBaseInfo->duration_ == 0 && multSegBaseInfo->segTimeline_.size() > 0 &&
        mpdInfo_->type_ == DashType::DASH_TYPE_STATIC) {
        uint64_t startTime = 0;
        std::list<DashSegTimeline*> timelineList = multSegBaseInfo->segTimeline_;
        for (std::list<DashSegTimeline*>::iterator it = timelineList.begin(); it != timelineList.end(); it++) {
            if (*it != nullptr) {
                int segCount = GetSegCountFromTimeline(it, timelineList.end(), periodDuration, timeScale, startTime);
                if (segCount < 0) {
                    MEDIA_LOG_W("get segment count error in SegmentList.SegmentTimeline");
                    continue;
                }

                // calculate segment duration by SegmentTimeline
                unsigned int segDuration = ((*it)->d_ * 1000) / timeScale;
                for (int repeat = 0; repeat <= segCount; repeat++) {
                    durationList.push_back(segDuration);
                    startTime += (*it)->d_;
                }
            }
        }
    }
}

/**
 * @brief    init segments with BaseUrl
 *
 * @param    baseUrlList            BaseUrl List
 * @param    streamDesc             stream description, store segments
 * @return   failed success undo
 */
DashSegmentInitValue DashMpdDownloader::GetSegmentsWithBaseUrl(std::list<std::string> baseUrlList,
                                                               std::shared_ptr<DashStreamDescription> streamDesc)
{
    DashSegment srcSegment;
    srcSegment.streamId_ = streamDesc->streamId_;
    srcSegment.bandwidth_ = streamDesc->bandwidth_;
    srcSegment.duration_ = streamDesc->duration_;
    srcSegment.startNumberSeq_ = streamDesc->startNumberSeq_;
    srcSegment.numberSeq_ = streamDesc->startNumberSeq_;
    if (baseUrlList.size() > 0) {
        srcSegment.url_ = baseUrlList.front();
    }
    
    if (DASH_SEGMENT_INIT_FAILED == AddOneSegment(srcSegment, streamDesc)) {
        MEDIA_LOG_E("GetSegmentsWithBaseUrl AddOneSegment is failed");
        return DASH_SEGMENT_INIT_FAILED;
    }

    return DASH_SEGMENT_INIT_SUCCESS;
}

DashRepresentationInfo *DashMpdDownloader::GetRepresemtationFromAdptSet(DashAdptSetInfo *adptSetInfo,
                                                                        unsigned int repIndex)
{
    unsigned int index = 0;
    for (std::list<DashRepresentationInfo *>::iterator it = adptSetInfo->representationList_.begin();
         it != adptSetInfo->representationList_.end(); it++, index++) {
        if (repIndex == index) {
            return *it;
        }
    }

    return nullptr;
}

/**
 * @brief    get segments in AdaptationSet with SegmentTemplate or SegmentList or BaseUrl
 *
 * @param    adptSetInfo            chosen AdaptationSet
 * @param    repInfo                use its id as get segments with SegmentTemplate
 * @param    baseUrl                Mpd.BaseUrl + Period.BaseUrl
 * @param    streamDesc             stream description, store segments
 *
 * @return   failed success undo
 */
DashSegmentInitValue DashMpdDownloader::GetSegmentsByAdptSetInfo(const DashAdptSetInfo* adptSetInfo,
    const DashRepresentationInfo* repInfo, std::string& baseUrl, std::shared_ptr<DashStreamDescription> streamDesc)
{
    DashSegmentInitValue initValue = DASH_SEGMENT_INIT_UNDO;

    // get segments from AdaptationSet.SegmentTemplate
    if (adptSetInfo->adptSetSegTmplt_ != nullptr && adptSetInfo->adptSetSegTmplt_->segTmpltMedia_.length() > 0) {
        DashAppendBaseUrl(baseUrl, adptSetInfo->baseUrl_);

        std::string representationId;
        if (repInfo != nullptr) {
            representationId = repInfo->id_;
        }

        initValue = GetSegmentsWithSegTemplate(adptSetInfo->adptSetSegTmplt_, representationId, streamDesc);
    } else if (adptSetInfo->adptSetSegList_ != nullptr && adptSetInfo->adptSetSegList_->segmentUrl_.size() > 0) {
        // get segments from AdaptationSet.SegmentList
        DashAppendBaseUrl(baseUrl, adptSetInfo->baseUrl_);
        initValue = GetSegmentsWithSegList(adptSetInfo->adptSetSegList_, baseUrl, streamDesc);
    } else if (adptSetInfo->baseUrl_.size() > 0) {
        initValue = GetSegmentsWithBaseUrl(adptSetInfo->baseUrl_, streamDesc);
    }

    return initValue;
}

bool DashMpdDownloader::GetInitSegFromPeriod(std::string periodBaseUrl, std::string repId,
                                             std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    DashUrlType *initSegment = periodManager_->GetInitSegment(
        segTmpltFlag); // should SetPeriodInfo before GetInitSegment
    if (initSegment != nullptr) {
        streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
        UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
        MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, periodBaseUrl);
        MEDIA_LOG_I("GetInitSegFromPeriod:streamId:"
        PUBLIC_LOG_D32
        ", init seg url "
        PUBLIC_LOG_S, streamDesc->streamId_, streamDesc->initSegment_->url_.c_str());
        return true;
    }

    return false;
}

bool DashMpdDownloader::GetInitSegFromAdptSet(std::string adptSetBaseUrl, std::string repId,
                                              std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    DashUrlType *initSegment = adptSetManager_->GetInitSegment(
        segTmpltFlag); // should SetAdptSetInfo before GetInitSegment
    if (initSegment != nullptr) {
        streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
        UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
        MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, adptSetBaseUrl);
        MEDIA_LOG_I("GetInitSegFromAdptSet:streamId:"
        PUBLIC_LOG_D32
        ", init seg url "
        PUBLIC_LOG_S, streamDesc->streamId_, streamDesc->initSegment_->url_.c_str());
        return true;
    }

    return false;
}

bool DashMpdDownloader::GetInitSegFromRepresentation(std::string repBaseUrl, std::string repId,
                                                     std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    DashUrlType *initSegment = representationManager_->GetInitSegment(segTmpltFlag);
    // should SetRepresentationInfo before GetInitSegment
    if (initSegment != nullptr) {
        streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
        UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
        MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, repBaseUrl);
        MEDIA_LOG_I("GetInitSegFromRepresentation:streamId:"
        PUBLIC_LOG_D32
        ", init seg url "
        PUBLIC_LOG_S, streamDesc->streamId_, streamDesc->initSegment_->url_.c_str());
        return true;
    }

    return false;
}

void DashMpdDownloader::UpdateInitSegUrl(std::shared_ptr<DashStreamDescription> streamDesc, const DashUrlType *urlType,
                                         int segTmpltFlag, std::string representationID)
{
    if (streamDesc != nullptr && streamDesc->initSegment_ != nullptr) {
        streamDesc->initSegment_->url_ = urlType->sourceUrl_;
        if (urlType->range_.length() > 0) {
            DashParseRange(urlType->range_, streamDesc->initSegment_->rangeBegin_, streamDesc->initSegment_->rangeEnd_);
        }

        if (segTmpltFlag) {
            std::string::size_type posIden = streamDesc->initSegment_->url_.find("$$");
            if (posIden != std::string::npos) {
                size_t strLength = strlen("$$");
                streamDesc->initSegment_->url_.replace(posIden, strLength, "$");
            }

            if (DashSubstituteTmpltStr(streamDesc->initSegment_->url_, "$RepresentationID", representationID) == -1) {
                MEDIA_LOG_E("init seg url "
                PUBLIC_LOG_S
                " subtitute $RepresentationID error "
                PUBLIC_LOG_S, streamDesc->initSegment_->url_.c_str(), representationID.c_str());
            }

            if (DashSubstituteTmpltStr(streamDesc->initSegment_->url_, "$Bandwidth",
                                       std::to_string(streamDesc->bandwidth_)) == -1) {
                MEDIA_LOG_E("init seg url "
                PUBLIC_LOG_S
                " subtitute $Bandwidth error "
                PUBLIC_LOG_U32, streamDesc->initSegment_->url_.c_str(), streamDesc->bandwidth_);
            }
        }
    }
}

Status DashMpdDownloader::GetStreamInfo(std::vector<StreamInfo>& streams)
{
    MEDIA_LOG_I("GetStreamInfo");
    for (unsigned int index = 0; index < streamDescriptions_.size(); index++) {
        StreamInfo info;
        info.streamId = streamDescriptions_[index]->streamId_;
        if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            info.type = SUBTITLE;
        } else if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            info.type = AUDIO;
        } else {
            info.type = VIDEO;
        }

        streams.push_back(info);
    }
    return Status::OK;
}

bool DashMpdDownloader::PutStreamToDownload()
{
    for (auto stream : streamDescriptions_) {
        if (stream->inUse_ && stream->indexSegment_ != nullptr && stream->segsState_ != DASH_SEGS_STATE_FINISH) {
            stream->segsState_ = DASH_SEGS_STATE_PARSING;
            currentDownloadStream_ = stream;
            int64_t startRange = stream->indexSegment_->indexRangeBegin_;
            int64_t endRange = stream->indexSegment_->indexRangeEnd_;
            // exist init segment, download together
            if (CheckToDownloadSidxWithInitSeg(stream)) {
                MEDIA_LOG_I("update range begin from "
                PUBLIC_LOG_D64
                " to "
                PUBLIC_LOG_D64, startRange, stream->initSegment_->rangeBegin_);
                startRange = stream->initSegment_->rangeBegin_;
            }
            DoOpen(stream->indexSegment_->url_, startRange, endRange);
            return true;
        }
    }
    return false;
}

}
}
}
}