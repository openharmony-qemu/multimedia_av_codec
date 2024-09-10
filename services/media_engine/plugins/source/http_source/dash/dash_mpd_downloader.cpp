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
#include <algorithm>
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
constexpr size_t RETRY_TIMES = 15000;
constexpr unsigned int SLEEP_TIME = 1;
constexpr int32_t MPD_HTTP_TIME_OUT_MS = 5 * 1000;
constexpr unsigned int SEGMENT_DURATION_DELTA = 100; // ms

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

static int64_t ParseStartNumber(const std::string &numberStr)
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

static void MakeAbsoluteWithBaseUrl(const std::vector<std::shared_ptr<DashSegment>> &segmentsVector,
                                    const std::string &baseUrl)
{
    std::string segUrl;
    unsigned int size = segmentsVector.size();

    for (unsigned int index = 0; index < size; index++) {
        if (segmentsVector[index] != nullptr) {
            segUrl = baseUrl;
            DashAppendBaseUrl(segUrl, segmentsVector[index]->url_);
            segmentsVector[index]->url_ = segUrl;
        }
    }
}

static void MakeAbsoluteWithBaseUrl(std::shared_ptr<DashInitSegment> initSegment, const std::string &baseUrl)
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

static DashSegmentInitValue AddOneSegment(unsigned int segRealDur, int64_t segmentSeq, const std::string &tempUrl,
                                          std::shared_ptr<DashStreamDescription> streamDesc)
{
    std::shared_ptr<DashSegment> segment = std::make_shared<DashSegment>();
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
         it != adptSet->representationList_.end(); ++it, ++index) {
        if (index == repreIndex) {
            return *it;
        }
    }
    return nullptr;
}

void DashMpdDownloader::Open(const std::string &url)
{
    url_ = url;
    DoOpen(url);
}

void DashMpdDownloader::Close(bool isAsync)
{
    downloader_->Stop(isAsync);

    if (downloadRequest_ != nullptr && !downloadRequest_->IsClosed()) {
        downloadRequest_->Close();
    }
}

void DashMpdDownloader::SetStatusCallback(StatusCallbackFunc cb)
{
    statusCallback_ = cb;
}

void DashMpdDownloader::UpdateDownloadFinished(const std::string &url)
{
    MEDIA_LOG_I("UpdateDownloadFinished:ondemandSegBase_=%{public}u", ondemandSegBase_);
    if (ondemandSegBase_) {
        ParseSidx();
    } else {
        ParseManifest();
    }
}

int DashMpdDownloader::GetInUseVideoStreamId() const
{
    for (uint32_t index = 0; index < streamDescriptions_.size(); index++) {
        if (streamDescriptions_[index]->inUse_ && streamDescriptions_[index]->type_ == MediaAVCodec::MEDIA_TYPE_VID) {
            return streamDescriptions_[index]->streamId_;
        }
    }
    return -1;
}

DashMpdGetRet DashMpdDownloader::GetNextSegmentByStreamId(int streamId, std::shared_ptr<DashSegment> &seg)
{
    MEDIA_LOG_I("GetNextSegmentByStreamId streamId:" PUBLIC_LOG_D32, streamId);
    seg = nullptr;
    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
    for (auto &streamDescription : streamDescriptions_) {
        if (streamDescription->streamId_ != streamId) {
            continue;
        }

        if (streamDescription->segsState_ == DASH_SEGS_STATE_FINISH) {
            int64_t segmentIndex = (streamDescription->currentNumberSeq_ == -1) ? 0 :
                streamDescription->currentNumberSeq_ - streamDescription->startNumberSeq_ + 1;
            MEDIA_LOG_D("get segment index :" PUBLIC_LOG_D64 ", id:" PUBLIC_LOG_D32 ", seq:"
                PUBLIC_LOG_D64, segmentIndex, streamDescription->streamId_, streamDescription->currentNumberSeq_);
            if (segmentIndex >= 0 && (unsigned int) segmentIndex < streamDescription->mediaSegments_.size()) {
                seg = streamDescription->mediaSegments_[segmentIndex];
                streamDescription->currentNumberSeq_ = seg->numberSeq_;
                MEDIA_LOG_D("after get segment index :"
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

    return ret;
}

DashMpdGetRet DashMpdDownloader::GetBreakPointSegment(int streamId, int64_t breakpoint,
                                                      std::shared_ptr<DashSegment> &seg)
{
    MEDIA_LOG_I("GetBreakPointSegment streamId:" PUBLIC_LOG_D32 ", breakpoint:" PUBLIC_LOG_D64, streamId, breakpoint);
    seg = nullptr;
    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
    for (auto &streamDescription : streamDescriptions_) {
        if (streamDescription->streamId_ != streamId) {
            continue;
        }

        if (streamDescription->segsState_ != DASH_SEGS_STATE_FINISH) {
            MEDIA_LOG_E("GetBreakPointSegment no segment list");
            ret = DASH_MPD_GET_UNDONE;
            break;
        }

        int64_t segmentDuration = 0;
        for (unsigned int index = 0; index <  streamDescription->mediaSegments_.size(); index++) {
            if (segmentDuration + (int64_t)(streamDescription->mediaSegments_[index]->duration_) > breakpoint) {
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
        break;
    }

    return ret;
}

DashMpdGetRet DashMpdDownloader::GetNextVideoStream(DashMpdBitrateParam &param, int &streamId)
{
    std::shared_ptr<DashStreamDescription> currentStream = nullptr;
    std::shared_ptr<DashStreamDescription> destStream = nullptr;
    for (const auto &stream : streamDescriptions_) {
        if (stream->type_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
            continue;
        }

        if (stream->bandwidth_ == param.bitrate_) {
            destStream = stream;
            MEDIA_LOG_I("switch to bandwidth:" PUBLIC_LOG_U32 ", id:" PUBLIC_LOG_D32 ", width:"
                PUBLIC_LOG_U32, stream->bandwidth_, stream->streamId_, stream->width_);
        }

        if (stream->inUse_) {
            currentStream = stream;
        }
    }

    if (destStream == nullptr || currentStream == nullptr) {
        MEDIA_LOG_E("switch to bandwidth:" PUBLIC_LOG_U32 ", can not find stream", param.bitrate_);
        return DASH_MPD_GET_ERROR;
    }

    currentStream->inUse_ = false;
    destStream->inUse_ = true;
    streamId = destStream->streamId_;

    if (destStream->startNumberSeq_ != currentStream->startNumberSeq_) {
        MEDIA_LOG_E("select bitrate:" PUBLIC_LOG_U32 " but seq:" PUBLIC_LOG_D64 " is not equal bitrate:"
            PUBLIC_LOG_U32 ", seq:" PUBLIC_LOG_D64, destStream->bandwidth_, destStream->startNumberSeq_,
        currentStream->bandwidth_, currentStream->startNumberSeq_);
    }
    if (param.position_ == -1) {
        destStream->currentNumberSeq_ = currentStream->currentNumberSeq_;
    } else {
        destStream->currentNumberSeq_ = param.position_;
    }

    param.nextSegTime_ = GetSegTimeBySeq(currentStream->mediaSegments_, destStream->currentNumberSeq_);
    MEDIA_LOG_I("select bitrate current type:" PUBLIC_LOG_D32 ", change to:" PUBLIC_LOG_D32 ",nextSegTime:"
        PUBLIC_LOG_U32, currentStream->videoType_, destStream->videoType_, param.nextSegTime_);

    DashMpdGetRet ret = GetSegmentsInNewStream(destStream);
    if (ret == DASH_MPD_GET_DONE && param.nextSegTime_ > 0) {
        UpdateCurrentNumberSeqByTime(destStream, param.nextSegTime_);
        param.nextSegTime_ = 0;
    }

    return ret;
}

DashMpdGetRet DashMpdDownloader::GetNextTrackStream(DashMpdTrackParam &param)
{
    std::shared_ptr<DashStreamDescription> currentStream = nullptr;
    std::shared_ptr<DashStreamDescription> destStream = nullptr;
    for (const auto &stream : streamDescriptions_) {
        if (stream->type_ != param.type_) {
            continue;
        }

        if (stream->streamId_ == param.streamId_) {
            destStream = stream;
            MEDIA_LOG_I("switch to id:" PUBLIC_LOG_D32 ", lang:" PUBLIC_LOG_S,
                stream->streamId_, stream->lang_.c_str());
        }

        if (stream->inUse_) {
            currentStream = stream;
        }
    }

    if (destStream == nullptr || currentStream == nullptr) {
        MEDIA_LOG_E("switch to streamId:" PUBLIC_LOG_D32 ", can not find stream", param.streamId_);
        return DASH_MPD_GET_ERROR;
    }

    currentStream->inUse_ = false;
    destStream->inUse_ = true;

    if (destStream->startNumberSeq_ != currentStream->startNumberSeq_) {
        MEDIA_LOG_E("select track streamId:" PUBLIC_LOG_D32 " but seq:" PUBLIC_LOG_D64 " is not equal bitrate:"
            PUBLIC_LOG_D32 ", seq:" PUBLIC_LOG_D64, destStream->streamId_, destStream->startNumberSeq_,
        currentStream->streamId_, currentStream->startNumberSeq_);
    }
    if (param.position_ == -1) {
        destStream->currentNumberSeq_ = currentStream->currentNumberSeq_;
    } else {
        destStream->currentNumberSeq_ = param.position_;
    }

    if (!param.isEnd_) {
        destStream->currentNumberSeq_ -= 1;
        if (destStream->currentNumberSeq_ < destStream->startNumberSeq_) {
            destStream->currentNumberSeq_ = -1;
        }
    }

    param.nextSegTime_ = GetSegTimeBySeq(currentStream->mediaSegments_, destStream->currentNumberSeq_);
    MEDIA_LOG_I("select track current lang:" PUBLIC_LOG_S ", change to:" PUBLIC_LOG_S ",nextSegTime:"
        PUBLIC_LOG_U32, currentStream->lang_.c_str(), destStream->lang_.c_str(), param.nextSegTime_);

    DashMpdGetRet ret = GetSegmentsInNewStream(destStream);
    if (ret == DASH_MPD_GET_DONE && param.nextSegTime_ > 0) {
        UpdateCurrentNumberSeqByTime(destStream, param.nextSegTime_);
        param.nextSegTime_ = 0;
    }

    return ret;
}

std::shared_ptr<DashStreamDescription> DashMpdDownloader::GetStreamByStreamId(int streamId)
{
    auto iter = std::find_if(streamDescriptions_.begin(), streamDescriptions_.end(),
        [&](const std::shared_ptr<DashStreamDescription> &stream) {
            return stream->streamId_ == streamId;
        });
    if (iter == streamDescriptions_.end()) {
        return nullptr;
    }

    return *iter;
}

std::shared_ptr<DashStreamDescription> DashMpdDownloader::GetUsingStreamByType(MediaAVCodec::MediaType type)
{
    auto iter = std::find_if(streamDescriptions_.begin(), streamDescriptions_.end(),
        [&](const std::shared_ptr<DashStreamDescription> &stream) {
            return stream->type_ == type && stream->inUse_;
        });
    if (iter == streamDescriptions_.end()) {
        return nullptr;
    }

    return *iter;
}

std::shared_ptr<DashInitSegment> DashMpdDownloader::GetInitSegmentByStreamId(int streamId)
{
    auto iter = std::find_if(streamDescriptions_.begin(), streamDescriptions_.end(),
        [&](const std::shared_ptr<DashStreamDescription> &streamDescription) {
            return streamDescription->streamId_ == streamId;
        });
    if (iter == streamDescriptions_.end()) {
        return nullptr;
    }
    return (*iter)->initSegment_;
}

void DashMpdDownloader::SetCurrentNumberSeqByStreamId(int streamId, int64_t numberSeq)
{
    for (unsigned int index = 0; index < streamDescriptions_.size(); index++) {
        if (streamDescriptions_[index]->streamId_ == streamId) {
            streamDescriptions_[index]->currentNumberSeq_ = numberSeq;
            MEDIA_LOG_I("SetCurrentNumberSeqByStreamId update id:" PUBLIC_LOG_D32 ", seq:"
                PUBLIC_LOG_D64, streamId, numberSeq);
            break;
        }
    }
}

void DashMpdDownloader::UpdateCurrentNumberSeqByTime(const std::shared_ptr<DashStreamDescription> &streamDesc,
    uint32_t nextSegTime)
{
    if (streamDesc == nullptr) {
        return;
    }

    unsigned int previousSegsDuration = 0;
    int64_t numberSeq = -1;
    for (unsigned int index = 0; index <  streamDesc->mediaSegments_.size(); index++) {
        previousSegsDuration += streamDesc->mediaSegments_[index]->duration_;
        // previousSegsDuration greater than nextSegTime 100ms, get this segment, otherwise find next segment to check
        if (previousSegsDuration > nextSegTime && (previousSegsDuration - nextSegTime) > SEGMENT_DURATION_DELTA) {
            MEDIA_LOG_I("UpdateSeqByTime find next seq:" PUBLIC_LOG_D64, streamDesc->mediaSegments_[index]->numberSeq_);
            break;
        }

        numberSeq = streamDesc->mediaSegments_[index]->numberSeq_;
    }

    MEDIA_LOG_I("UpdateSeqByTime change current seq from:" PUBLIC_LOG_D64 " to:" PUBLIC_LOG_D64 ", nextSegTime:"
        PUBLIC_LOG_U32, streamDesc->currentNumberSeq_, numberSeq, nextSegTime);
    streamDesc->currentNumberSeq_ = numberSeq;
}

void DashMpdDownloader::SetHdrStart(bool isHdrStart)
{
    MEDIA_LOG_I("SetHdrStart:" PUBLIC_LOG_D32, isHdrStart);
    isHdrStart_ = isHdrStart;
}

void DashMpdDownloader::SetInitResolution(unsigned int width, unsigned int height)
{
    MEDIA_LOG_I("SetInitResolution, width:" PUBLIC_LOG_U32 ", height:" PUBLIC_LOG_U32, width, height);
    if (width > 0 && height > 0) {
        initResolution_ = width * height;
    }
}

void DashMpdDownloader::SetDefaultLang(const std::string &lang, MediaAVCodec::MediaType type)
{
    MEDIA_LOG_I("SetDefaultLang, lang:" PUBLIC_LOG_S ", type:" PUBLIC_LOG_D32, lang.c_str(), (int)type);
    if (type == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
        defaultAudioLang_ = lang;
    } else if (type == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
        defaultSubtitleLang_ = lang;
    }
}

void DashMpdDownloader::SetInterruptState(bool isInterruptNeeded)
{
    isInterruptNeeded_ = isInterruptNeeded;
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

std::string DashMpdDownloader::GetUrl() const
{
    return url_;
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
        ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE);
        if (ondemandSegBase_) {
            PutStreamToDownload();
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

void DashMpdDownloader::GetDrmInfos(std::vector<DashDrmInfo>& drmInfos)
{
    std::list<DashPeriodInfo*> periods = mpdInfo_->periodInfoList_;
    for (auto &periodInfo : periods) {
        if (periodInfo == nullptr) {
            continue;
        }
        std::string periodDrmId{"PeriodId:"};
        periodDrmId.append(periodInfo->id_);
        GetAdpDrmInfos(drmInfos, periodInfo, periodDrmId);
    }
}

void DashMpdDownloader::GetAdpDrmInfos(std::vector<DashDrmInfo> &drmInfos, DashPeriodInfo *const &periodInfo,
                                       const std::string &periodDrmId)
{
    DashList<DashAdptSetInfo*> adptSetList = periodInfo->adptSetList_;
    for (const auto &adptSetInfo : adptSetList) {
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

void DashMpdDownloader::ProcessDrmInfos()
{
    std::vector<DashDrmInfo> drmInfos;
    GetDrmInfos(drmInfos);

    std::multimap<std::string, std::vector<uint8_t>> drmInfoMap;
    for (const auto &drmInfo: drmInfos) {
        bool isReported = std::any_of(localDrmInfos_.begin(), localDrmInfos_.end(),
            [&](const DashDrmInfo &localDrmInfo) {
                return drmInfo.uuid_ == localDrmInfo.uuid_ && drmInfo.pssh_ == localDrmInfo.pssh_;
            });
        if (isReported) {
            continue;
        }

        std::string psshString = drmInfo.pssh_;
        uint8_t pssh[2048]; // 2048: pssh len
        uint32_t psshSize = 2048; // 2048: pssh len
        if (Base64Utils::Base64Decode(reinterpret_cast<const uint8_t *>(psshString.c_str()),
                                      static_cast<uint32_t>(psshString.length()), pssh, &psshSize)) {
            uint32_t uuidSize = 16; // 16: uuid len
            if (psshSize < DRM_UUID_OFFSET + uuidSize) {
                continue;
            }

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
        } else {
            MEDIA_LOG_W("Base64Decode pssh error, drmId " PUBLIC_LOG_S, drmInfo.drmId_.c_str());
        }
    }

    if (callback_ != nullptr) {
        callback_->OnDrmInfoChanged(drmInfoMap);
    }
}

void DashMpdDownloader::GetDrmInfos(const std::string &drmId, const DashList<DashDescriptor *> &contentProtections,
                                    std::vector<DashDrmInfo> &drmInfoList)
{
    for (const auto &contentProtection : contentProtections) {
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
            MEDIA_LOG_I("ParseSidx update sidx segment content size is " PUBLIC_LOG_ZU ", "
                PUBLIC_LOG_C "-" PUBLIC_LOG_C, sidxContent.size(), sidxContent[0], sidxContent[1]);
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
    unsigned int segAddDuration = 0; // add all segments duration(ms) before current segment
    int64_t segSeq = currentDownloadStream_->startNumberSeq_;
    for (const auto &subSegIndex : subSegIndexList) {
        unsigned int durationMS = (static_cast<uint64_t>(subSegIndex->duration_) * S_2_MS) / subSegIndex->timeScale_;
        segDurSum += subSegIndex->duration_;
        unsigned int segDurMsSum = static_cast<unsigned int>((segDurSum * S_2_MS) / subSegIndex->timeScale_);
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

    if (currentDownloadStream_->mediaSegments_.size() > 0) {
        std::shared_ptr<DashSegment> lastSegment = currentDownloadStream_->mediaSegments_[
            currentDownloadStream_->mediaSegments_.size() - 1];
        if (lastSegment != nullptr && mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_STATIC) {
            lastSegment->isLast_ = true;
        }
    }
}

void DashMpdDownloader::OpenStream(std::shared_ptr<DashStreamDescription> stream)
{
    stream->segsState_ = DASH_SEGS_STATE_PARSING;
    currentDownloadStream_ = stream;
    int64_t startRange = stream->indexSegment_->indexRangeBegin_;
    int64_t endRange = stream->indexSegment_->indexRangeEnd_;
    // exist init segment, download together
    if (CheckToDownloadSidxWithInitSeg(stream)) {
        MEDIA_LOG_I("update range begin from " PUBLIC_LOG_D64 " to "
            PUBLIC_LOG_D64, startRange, stream->initSegment_->rangeBegin_);
        startRange = stream->initSegment_->rangeBegin_;
    }
    DoOpen(stream->indexSegment_->url_, startRange, endRange);
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

    MEDIA_LOG_I("DoOpen:start=%{public}lld end=%{public}lld", (long long) startRange, (long long) endRange);
    MediaSouce mediaSource;
    mediaSource.url = url;
    mediaSource.timeoutMs = MPD_HTTP_TIME_OUT_MS;
    downloadRequest_ = std::make_shared<DownloadRequest>(dataSave_, realStatusCallback, mediaSource, requestWholeFile);
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
    MEDIA_LOG_D("SaveData:size=%{public}u len=%{public}u", (unsigned int)downloadContent_.size(), len);
    downloadContent_.append(reinterpret_cast<const char*>(data), len);
    return true;
}

void DashMpdDownloader::SetMpdCallback(DashMpdCallback *callback)
{
    callback_ = callback;
}

int64_t DashMpdDownloader::GetDuration() const
{
    MEDIA_LOG_I("GetDuration " PUBLIC_LOG_U32, duration_);
    return (duration_ > 0) ? ((int64_t)duration_ * MS_2_NS) : 0;
}

Seekable DashMpdDownloader::GetSeekable() const
{
    // need wait mpdInfo_ not null
    size_t times = 0;
    while (times < RETRY_TIMES && !isInterruptNeeded_) {
        if (mpdInfo_ != nullptr && notifyOpenOk_) {
            break;
        }
        OSAL::SleepFor(SLEEP_TIME);
        times++;
    }

    if (times >= RETRY_TIMES || isInterruptNeeded_) {
        MEDIA_LOG_I("GetSeekable INVALID");
        return Seekable::INVALID;
    }

    MEDIA_LOG_I("GetSeekable end");
    return mpdInfo_->type_ == DashType::DASH_TYPE_STATIC ? Seekable::SEEKABLE : Seekable::UNSEEKABLE;
}

std::vector<uint32_t> DashMpdDownloader::GetBitRates() const
{
    std::vector<uint32_t> bitRates;
    for (const auto &item : streamDescriptions_) {
        if (item->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID && item->bandwidth_ > 0) {
            bitRates.push_back(item->bandwidth_);
        }
    }
    return bitRates;
}

std::vector<uint32_t> DashMpdDownloader::GetBitRatesByHdr(bool isHdr) const
{
    std::vector<uint32_t> bitRates;
    for (const auto &item : streamDescriptions_) {
        if (item->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_VID &&
            item->bandwidth_ > 0 &&
            (isHdr == (item->videoType_ != DASH_VIDEO_TYPE_SDR))) {
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

void DashMpdDownloader::SeekToTs(int streamId, int64_t seekTime, std::shared_ptr<DashSegment> &seg) const
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
        for (const auto &mediaSegment : mediaSegments) {
            if (mediaSegment == nullptr) {
                continue;
            }

            totalDuration += static_cast<int64_t>(mediaSegment->duration_);
            if (totalDuration > seekTime) {
                seg = mediaSegment;
                MEDIA_LOG_I("Dash SeekToTs segment totalDuration:" PUBLIC_LOG_D64 ", segNum:"
                    PUBLIC_LOG_D64 ", duration:" PUBLIC_LOG_U32,
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
    for (const auto &periodInfo : mpdInfo_->periodInfoList_) {
        if (SetOndemandSegBase(periodInfo->adptSetList_)) {
            break;
        }
    }

    MEDIA_LOG_I("dash onDemandSegBase is " PUBLIC_LOG_D32, ondemandSegBase_);
}

bool DashMpdDownloader::SetOndemandSegBase(std::list<DashAdptSetInfo*> adptSetList)
{
    for (const auto &adptSetInfo : adptSetList) {
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
    for (const auto &rep : repList) {
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
                                               const std::string &mpdBaseUrl)
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

void DashMpdDownloader::GetStreamsInfoInAdptSet(DashAdptSetInfo *adptSetInfo, const std::string &periodBaseUrl,
                                                DashStreamDescription &streamDesc)
{
    streamDesc.width_ = adptSetInfo->commonAttrsAndElements_.width_;
    streamDesc.height_ = adptSetInfo->commonAttrsAndElements_.height_;
    streamDesc.lang_ = adptSetInfo->lang_;
    std::string adptSetBaseUrl = periodBaseUrl;
    DashAppendBaseUrl(adptSetBaseUrl, adptSetInfo->baseUrl_);
    adptSetManager_->SetAdptSetInfo(adptSetInfo);
    if (adptSetManager_->IsHdr()) {
        streamDesc.videoType_ = DASH_VIDEO_TYPE_HDR_10;
    } else {
        streamDesc.videoType_ = DASH_VIDEO_TYPE_SDR;
    }
    MEDIA_LOG_D("GetStreamsInfoInAdptSet hdrType " PUBLIC_LOG_U32, streamDesc.videoType_);

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

void DashMpdDownloader::GetStreamDescriptions(const std::string &periodBaseUrl, DashStreamDescription &streamDesc,
                                              const std::string &adptSetBaseUrl,
                                              std::list<DashRepresentationInfo *> &repList)
{
    std::string repBaseUrl;
    unsigned int repIndex = 0;
    DashVideoType defaultVideoType = streamDesc.videoType_;
    for (std::list<DashRepresentationInfo*>::iterator it = repList.begin(); it != repList.end(); ++it, ++repIndex) {
        if (*it == nullptr) {
            continue;
        }

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

        if (defaultVideoType != DASH_VIDEO_TYPE_SDR &&
            (*it)->commonAttrsAndElements_.cuvvVersion_.find("cuvv.") != std::string::npos) {
            streamDesc.videoType_ = DASH_VIDEO_TYPE_HDR_VIVID;
            MEDIA_LOG_I("current stream is hdr vivid, band:" PUBLIC_LOG_U32, streamDesc.bandwidth_);
        } else {
            streamDesc.videoType_ = defaultVideoType;
        }

        std::shared_ptr<DashStreamDescription> desc = std::make_shared<DashStreamDescription>(streamDesc);
        representationManager_->SetRepresentationInfo(*it);
        DashAppendBaseUrl(repBaseUrl, (*it)->baseUrl_);

        if (!GetInitSegFromRepresentation(repBaseUrl, (*it)->id_, desc)) {
            if (!GetInitSegFromAdptSet(adptSetBaseUrl, (*it)->id_, desc)) {
                GetInitSegFromPeriod(periodBaseUrl, (*it)->id_, desc);
            }
        }

        if (ondemandSegBase_ &&
            (*it)->representationSegBase_ != nullptr &&
            (*it)->representationSegBase_->indexRange_.size() > 0) {
            desc->indexSegment_ = std::make_shared<DashIndexSegment>();
            desc->indexSegment_->url_ = repBaseUrl;
            DashParseRange((*it)->representationSegBase_->indexRange_, desc->indexSegment_->indexRangeBegin_,
                           desc->indexSegment_->indexRangeEnd_);
        }
        MEDIA_LOG_I("add stream band:" PUBLIC_LOG_U32 ", hdr: " PUBLIC_LOG_D32,
            streamDesc.bandwidth_, streamDesc.videoType_);
        streamDescriptions_.push_back(desc);
    }
}

unsigned int DashMpdDownloader::GetResolutionDelta(unsigned int width, unsigned int height)
{
    unsigned int resolution = width * height;
    if (resolution > initResolution_) {
        return resolution - initResolution_;
    } else {
        return initResolution_ - resolution;
    }
}

bool DashMpdDownloader::IsChoosedVideoStream(const std::shared_ptr<DashStreamDescription> &choosedStream,
    const std::shared_ptr<DashStreamDescription> &currentStream)
{
    if (choosedStream == nullptr ||
        (initResolution_ == 0 && choosedStream->bandwidth_ > currentStream->bandwidth_) ||
        IsNearToInitResolution(choosedStream, currentStream)) {
        return true;
    }
    return false;
}

bool DashMpdDownloader::IsNearToInitResolution(const std::shared_ptr<DashStreamDescription> &choosedStream,
    const std::shared_ptr<DashStreamDescription> &currentStream)
{
    if (choosedStream == nullptr || currentStream == nullptr || initResolution_ == 0) {
        return false;
    }

    unsigned int choosedDelta = GetResolutionDelta(choosedStream->width_, choosedStream->height_);
    unsigned int currentDelta = GetResolutionDelta(currentStream->width_, currentStream->height_);
    return (currentDelta < choosedDelta)
           || (currentDelta == choosedDelta && currentStream->bandwidth_ < choosedStream->bandwidth_);
}

bool DashMpdDownloader::IsLangMatch(const std::string &lang, MediaAVCodec::MediaType type)
{
    if (type == MediaAVCodec::MediaType::MEDIA_TYPE_AUD &&
        defaultAudioLang_.length() > 0) {
        if (defaultAudioLang_.compare(lang) == 0) {
            return true;
        } else {
            return false;
        }
    } else if (type == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE &&
        defaultSubtitleLang_.length() > 0) {
        if (defaultSubtitleLang_.compare(lang) == 0) {
            return true;
        } else {
            return false;
        }
    }
    return true;
}

bool DashMpdDownloader::ChooseStreamToPlay(MediaAVCodec::MediaType type)
{
    if (mpdInfo_ == nullptr || streamDescriptions_.size() == 0) {
        return false;
    }

    std::shared_ptr<DashStreamDescription> choosedStream = nullptr;
    std::shared_ptr<DashStreamDescription> backupStream = nullptr;
    for (const auto &stream : streamDescriptions_) {
        if (stream->type_ != type || stream->inUse_) {
            continue;
        }
        
        if (type != MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
            // audio and subtitle choose first stream to play or get default lang
            if (choosedStream == nullptr) {
                choosedStream = stream;
            }
            if (!IsLangMatch(stream->lang_, type)) {
                continue;
            }

            choosedStream = stream;
            break;
        }
        
        if (backupStream == nullptr || IsNearToInitResolution(backupStream, stream) ||
            (initResolution_ == 0 && backupStream->bandwidth_ > stream->bandwidth_)) {
            backupStream = stream;
        }
        
        if ((stream->videoType_ != DASH_VIDEO_TYPE_SDR) != isHdrStart_) {
            continue;
        }
        
        if (IsChoosedVideoStream(choosedStream, stream)) {
            choosedStream = stream;
        }
    }

    if (choosedStream == nullptr && type == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        MEDIA_LOG_I("ChooseStreamToPlay video can not find hdrstart:" PUBLIC_LOG_D32, isHdrStart_);
        choosedStream = backupStream;
    }

    if (choosedStream != nullptr) {
        choosedStream->inUse_ = true;
        MEDIA_LOG_I("ChooseStreamToPlay type:" PUBLIC_LOG_D32 ", streamId:"
            PUBLIC_LOG_D32, (int) type, choosedStream->streamId_);
        if (!ondemandSegBase_) {
            GetSegmentsInMpd(choosedStream);
        }
        return true;
    }

    return false;
}

unsigned int DashMpdDownloader::GetSegTimeBySeq(const std::vector<std::shared_ptr<DashSegment>> &segments,
                                                int64_t segSeq)
{
    if (segments.size() == 0) {
        return 0;
    }

    unsigned int nextSegTime = 0;
    for (unsigned int index = 0; index <  segments.size(); index++) {
        if (segments[index]->numberSeq_ > segSeq) {
            break;
        }

        nextSegTime += segments[index]->duration_;
    }

    return nextSegTime;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInMpd(std::shared_ptr<DashStreamDescription> streamDesc)
{
    if (mpdInfo_ == nullptr || streamDesc == nullptr) {
        MEDIA_LOG_E("GetSegments but mpdInfo_ or streamDesc is nullptr");
        return DASH_SEGMENT_INIT_FAILED;
    }

    streamDesc->segsState_ = DASH_SEGS_STATE_FINISH;
    std::string mpdBaseUrl;
    mpdManager_->SetMpdInfo(mpdInfo_, url_);
    mpdBaseUrl = mpdManager_->GetBaseUrl();

    unsigned int currentPeriodIndex = 0;
    for (std::list<DashPeriodInfo *>::iterator it = mpdInfo_->periodInfoList_.begin();
         it != mpdInfo_->periodInfoList_.end(); ++it, ++currentPeriodIndex) {
        if (*it != nullptr && currentPeriodIndex == streamDesc->periodIndex_) {
            if (GetSegmentsInPeriod(*it, mpdBaseUrl, streamDesc) == DASH_SEGMENT_INIT_FAILED) {
                MEDIA_LOG_I("GetSegmentsInPeriod" PUBLIC_LOG_U32 " failed, type "
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

    std::shared_ptr<DashSegment> lastSegment = streamDesc->mediaSegments_[streamDesc->mediaSegments_.size() - 1];
    if (lastSegment != nullptr && mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_STATIC) {
        lastSegment->isLast_ = true;
    }
    return DASH_SEGMENT_INIT_SUCCESS;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInPeriod(DashPeriodInfo *periodInfo, const std::string &mpdBaseUrl,
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

DashSegmentInitValue DashMpdDownloader::GetSegmentsInAdptSet(DashAdptSetInfo *adptSetInfo,
                                                             const std::string &periodBaseUrl,
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
                                                                    const std::string &adptSetBaseUrl,
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
        MEDIA_LOG_E("media " PUBLIC_LOG_S " substitute $RepresentationID error "
            PUBLIC_LOG_S, media.c_str(), id.c_str());
        return DASH_SEGMENT_INIT_FAILED;
    }

    if (DashSubstituteTmpltStr(media, "$Bandwidth", std::to_string(streamDesc->bandwidth_)) == -1) {
        MEDIA_LOG_E("media " PUBLIC_LOG_S " substitute $Bandwidth error " PUBLIC_LOG_D32, media.c_str(),
            streamDesc->bandwidth_);
        return DASH_SEGMENT_INIT_FAILED;
    }

    streamDesc->startNumberSeq_ = ParseStartNumber(segTmpltInfo->multSegBaseInfo_.startNumber_);
    if (mpdInfo_->type_ == DashType::DASH_TYPE_STATIC) {
        return GetSegmentsWithTmpltStatic(segTmpltInfo, media, streamDesc);
    }

    return DASH_SEGMENT_INIT_FAILED;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltStatic(const DashSegTmpltInfo *segTmpltInfo,
                                                                   const std::string &mediaUrl,
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
                                                                           const std::string &mediaUrl,
                                                                           unsigned int timeScale,
                                                                           std::shared_ptr<DashStreamDescription> desc)
{
    unsigned int timeScaleTmp = timeScale > 0 ? timeScale : 1;
    // the segment duration in millisecond
    unsigned int segmentDuration =
        (static_cast<uint64_t>(segTmpltInfo->multSegBaseInfo_.duration_) * S_2_MS) / timeScaleTmp;
    if (segmentDuration == 0) {
        return DASH_SEGMENT_INIT_FAILED;
    }

    uint64_t segmentBaseDurationSum = 0; // the sum of segment duration(ms), not devide timescale
    unsigned int accumulateDuration = 0; // add all segments dutation(ms) before current segment
    int64_t segmentSeq = -1;

    while (accumulateDuration < desc->duration_) {
        segmentBaseDurationSum += segTmpltInfo->multSegBaseInfo_.duration_;
        // the sum of segment duration(ms), devide timescale
        unsigned int durationSumWithScale = static_cast<unsigned int>(segmentBaseDurationSum * S_2_MS / timeScaleTmp);
        unsigned int segRealDur = (durationSumWithScale > accumulateDuration) ?
            (durationSumWithScale - accumulateDuration) : segmentDuration;

        if (desc->duration_ - accumulateDuration < segRealDur) {
            segRealDur = desc->duration_ - accumulateDuration;
        }

        accumulateDuration += segRealDur;
        segmentSeq = (segmentSeq == -1) ? desc->startNumberSeq_ : (segmentSeq + 1);

        // if the @duration attribute is present
        // then the time address is determined by replacing the $Time$ identifier with ((k-1) + (kStart-1))* @duration
        // with kStart the value of the @startNumber attribute, if present, or 1 otherwise.
        int64_t startTime = (segmentSeq - 1) * segTmpltInfo->multSegBaseInfo_.duration_;
        std::string tempUrl = mediaUrl;

        if (DashSubstituteTmpltStr(tempUrl, "$Time", std::to_string(startTime)) == -1) {
            MEDIA_LOG_I("GetSegmentsWithTmpltDuration substitute $Time " PUBLIC_LOG_S " error in static duration",
                std::to_string(startTime).c_str());
            return DASH_SEGMENT_INIT_FAILED;
        }

        if (DashSubstituteTmpltStr(tempUrl, "$Number", std::to_string(segmentSeq)) == -1) {
            MEDIA_LOG_I("GetSegmentsWithTmpltDuration substitute $Number " PUBLIC_LOG_S " error in static duration",
                std::to_string(segmentSeq).c_str());
            return DASH_SEGMENT_INIT_FAILED;
        }

        if (DASH_SEGMENT_INIT_FAILED == AddOneSegment(segRealDur, segmentSeq, tempUrl, desc)) {
            return DASH_SEGMENT_INIT_FAILED;
        }
    }
    return DASH_SEGMENT_INIT_SUCCESS;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltTimelineStatic(const DashSegTmpltInfo *segTmpltInfo,
                                                                           const std::string &mediaUrl,
                                                                           unsigned int timeScale,
                                                                           std::shared_ptr<DashStreamDescription> desc)
{
    if (timeScale == 0) {
        return DASH_SEGMENT_INIT_FAILED;
    }

    int64_t segmentSeq = -1;
    uint64_t startTime = 0;
    std::list<DashSegTimeline*> timelineList = segTmpltInfo->multSegBaseInfo_.segTimeline_;
    for (std::list<DashSegTimeline*>::iterator it = timelineList.begin(); it != timelineList.end(); ++it) {
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

    while (repeat <= sampleInfo.segCount_) {
        repeat++;
        if (segmentSeq == -1) {
            segmentSeq = streamDesc->startNumberSeq_;
        } else {
            segmentSeq++;
        }

        std::string tempUrl = sampleInfo.mediaUrl_;
        if (DashSubstituteTmpltStr(tempUrl, "$Time", std::to_string(startTime)) == -1) {
            MEDIA_LOG_E("GetSegmentsInOneTimeline substitute $Time " PUBLIC_LOG_S " error in static timeline",
                std::to_string(startTime).c_str());
            return DASH_SEGMENT_INIT_FAILED;
        }

        if (DashSubstituteTmpltStr(tempUrl, "$Number", std::to_string(segmentSeq)) == -1) {
            MEDIA_LOG_E("GetSegmentsInOneTimeline substitute $Number " PUBLIC_LOG_S " error in static timeline",
                std::to_string(segmentSeq).c_str());
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
                MEDIA_LOG_W("r is negative and next S@t " PUBLIC_LOG_U64 " is not larger than startTime "
                    PUBLIC_LOG_U64, nextStartTime, startTime);
                return segCount;
            }

            segCount = (int)((nextStartTime - startTime) / (*it)->d_);
        } else {
            // the end of period
            it--;
            uint64_t scaleDuration = (uint64_t)periodDuration * timeScale / S_2_MS;
            if (scaleDuration <= startTime) {
                MEDIA_LOG_W("r is negative, duration " PUBLIC_LOG_U32 " is not larger than startTime "
                    PUBLIC_LOG_U64 ", timeScale " PUBLIC_LOG_U32, periodDuration, startTime, timeScale);
                return segCount;
            }

            segCount = (int)((scaleDuration - startTime) / (*it)->d_);
        }

        // 0 means only one segment
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
DashSegmentInitValue DashMpdDownloader::GetSegmentsWithSegList(const DashSegListInfo *segListInfo,
                                                               const std::string &baseUrl,
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
    for (std::list<DashSegUrl*>::iterator it = segUrlList.begin(); it != segUrlList.end(); ++it) {
        if (*it == nullptr) {
            continue;
        }

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
            // if size of SegmentTimeline smaller than size of SegmentURL,
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
            return DASH_SEGMENT_INIT_FAILED;
        }
        segmentCount++;
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
        for (std::list<DashSegTimeline*>::iterator it = timelineList.begin(); it != timelineList.end(); ++it) {
            if (*it == nullptr) {
                continue;
            }
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
         it != adptSetInfo->representationList_.end(); ++it, ++index) {
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

bool DashMpdDownloader::GetInitSegFromPeriod(const std::string &periodBaseUrl, const std::string &repId,
                                             std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    // should SetPeriodInfo before GetInitSegment
    DashUrlType *initSegment = periodManager_->GetInitSegment(segTmpltFlag);
    if (initSegment != nullptr) {
        streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
        UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
        MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, periodBaseUrl);
        MEDIA_LOG_D("GetInitSegFromPeriod:streamId:" PUBLIC_LOG_D32, streamDesc->streamId_);
        return true;
    }

    return false;
}

bool DashMpdDownloader::GetInitSegFromAdptSet(const std::string &adptSetBaseUrl, const std::string &repId,
                                              std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    // should SetAdptSetInfo before GetInitSegment
    DashUrlType *initSegment = adptSetManager_->GetInitSegment(segTmpltFlag);
    if (initSegment != nullptr) {
        streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
        UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
        MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, adptSetBaseUrl);
        MEDIA_LOG_D("GetInitSegFromAdptSet:streamId:" PUBLIC_LOG_D32, streamDesc->streamId_);
        return true;
    }

    return false;
}

bool DashMpdDownloader::GetInitSegFromRepresentation(const std::string &repBaseUrl, const std::string &repId,
                                                     std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    DashUrlType *initSegment = representationManager_->GetInitSegment(segTmpltFlag);
    // should SetRepresentationInfo before GetInitSegment
    if (initSegment != nullptr) {
        streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
        UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
        MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, repBaseUrl);
        MEDIA_LOG_D("GetInitSegFromRepresentation:streamId:" PUBLIC_LOG_D32, streamDesc->streamId_);
        return true;
    }

    return false;
}

DashMpdGetRet DashMpdDownloader::GetSegmentsInNewStream(std::shared_ptr<DashStreamDescription> destStream)
{
    MEDIA_LOG_I("GetSegmentsInNewStream update id:" PUBLIC_LOG_D32 ", seq:"
        PUBLIC_LOG_D64, destStream->streamId_, destStream->currentNumberSeq_);
    DashMpdGetRet ret = DASH_MPD_GET_ERROR;
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
                OpenStream(destStream);
            } else {
                MEDIA_LOG_E("GetNextSegmentByBitrate id:"
                    PUBLIC_LOG_D32 " ondemandSegBase_ but indexSegment is null", destStream->streamId_);
            }
        } else {
            // get segment list
            if (GetSegmentsInMpd(destStream) == DASH_SEGMENT_INIT_FAILED) {
                MEDIA_LOG_E("GetNextSegmentByBitrate id:"
                    PUBLIC_LOG_D32 " GetSegmentsInMpd failed", destStream->streamId_);
            }
            // get segment by position
            ret = DASH_MPD_GET_DONE;
        }
    }
    return ret;
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
                MEDIA_LOG_E("UpdateInitSegUrl subtitute $RepresentationID error "
                    PUBLIC_LOG_S, representationID.c_str());
            }

            if (DashSubstituteTmpltStr(streamDesc->initSegment_->url_, "$Bandwidth",
                                       std::to_string(streamDesc->bandwidth_)) == -1) {
                MEDIA_LOG_E("UpdateInitSegUrl subtitute $Bandwidth error "
                    PUBLIC_LOG_U32, streamDesc->bandwidth_);
            }
        }
    }
}

static VideoType GetVideoType(DashVideoType videoType)
{
    if (videoType == DASH_VIDEO_TYPE_HDR_VIVID) {
        return VIDEO_TYPE_HDR_VIVID;
    } else if (videoType == DASH_VIDEO_TYPE_HDR_10) {
        return VIDEO_TYPE_HDR_10;
    } else {
        return VIDEO_TYPE_SDR;
    }
}

Status DashMpdDownloader::GetStreamInfo(std::vector<StreamInfo> &streams)
{
    MEDIA_LOG_I("GetStreamInfo");
    // only support one audio/subtitle Representation in one AdaptationSet
    unsigned int audioAdptSetIndex = streamDescriptions_.size();
    unsigned int subtitleAdptSetIndex = audioAdptSetIndex;
    for (unsigned int index = 0; index < streamDescriptions_.size(); index++) {
        if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            if (streamDescriptions_[index]->adptSetIndex_ == audioAdptSetIndex) {
                MEDIA_LOG_D("GetStreamInfo skip audio stream:" PUBLIC_LOG_D32 ",lang:" PUBLIC_LOG_S,
                    streamDescriptions_[index]->streamId_, streamDescriptions_[index]->lang_.c_str());
                continue;
            }

            audioAdptSetIndex = streamDescriptions_[index]->adptSetIndex_;
        } else if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            if (streamDescriptions_[index]->adptSetIndex_ == subtitleAdptSetIndex) {
                MEDIA_LOG_D("GetStreamInfo skip subtitle stream:" PUBLIC_LOG_D32 ",lang:" PUBLIC_LOG_S,
                    streamDescriptions_[index]->streamId_, streamDescriptions_[index]->lang_.c_str());
                continue;
            }
            subtitleAdptSetIndex = streamDescriptions_[index]->adptSetIndex_;
        }

        StreamInfo info;
        info.streamId = streamDescriptions_[index]->streamId_;
        info.bitRate = streamDescriptions_[index]->bandwidth_;
        info.videoWidth = static_cast<int32_t>(streamDescriptions_[index]->width_);
        info.videoHeight = static_cast<int32_t>(streamDescriptions_[index]->height_);
        info.lang = streamDescriptions_[index]->lang_;
        info.videoType = GetVideoType(streamDescriptions_[index]->videoType_);
        if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            info.type = SUBTITLE;
        } else if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            info.type = AUDIO;
        } else {
            info.type = VIDEO;
        }

        MEDIA_LOG_D("GetStreamInfo streamId:" PUBLIC_LOG_D32 ", type:" PUBLIC_LOG_D32 ", bitRate:"
            PUBLIC_LOG_U32, info.streamId, info.type, info.bitRate);
        if (streamDescriptions_[index]->inUse_ && streams.size() > 0) {
            // play stream insert begin
            streams.insert(streams.begin(), info);
        } else {
            streams.push_back(info);
        }
    }
    return Status::OK;
}

bool DashMpdDownloader::PutStreamToDownload()
{
    auto iter = std::find_if(streamDescriptions_.begin(), streamDescriptions_.end(),
        [&](const std::shared_ptr<DashStreamDescription> &stream) {
            return stream->inUse_ &&
                stream->indexSegment_ != nullptr &&
                stream->segsState_ != DASH_SEGS_STATE_FINISH;
        });
    if (iter == streamDescriptions_.end()) {
        return false;
    }

    OpenStream(*iter);
    return true;
}

}
}
}
}