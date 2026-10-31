/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
#include <cstdint>
#include <climits>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <algorithm>
#include "network/network_typs.h"
#include "dash_mpd_downloader.h"
#include "dash_mpd_util.h"
#include "sidx_box_parser.h"
#include "utils/string_utils.h"
#include "utils/time_utils.h"
#include "base64_utils.h"
#include "safe_arithmetic.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
constexpr uint32_t DRM_UUID_OFFSET = 12;
constexpr size_t RETRY_TIMES = 15000;
constexpr unsigned int SLEEP_TIME = 1;
constexpr int32_t MPD_HTTP_TIME_OUT_MS = 5 * 1000;
constexpr uint32_t MPD_MAX_SIZE = 100 * 1024 * 1024;

constexpr double TOLERANCE = 1e-15;
constexpr uint64_t SEGMENT_DURATION_DELTA_NS = 100ULL * MS_2_NS;  // 100ms tolerance in nanoseconds
// MIN_SEGMENT_DURATION_NS is defined in utils/time_utils.h (included above)
constexpr uint32_t MAX_PSSH_BASE64_LEN = 2730;
constexpr size_t MAX_TIMELINE_ENTRIES = 1000000;  // max timeline entries per representation
constexpr size_t MAX_SIDX_ENTRIES = 65536;       // sidx referenceCount is unsigned short (max 65535)
constexpr int64_t MAX_SEG_COUNT_PER_S = 1000000;

namespace {
void CollectPeriodRanges(const DashMpdInfo *mpdInfo, uint64_t fallbackDurationNs,
    std::vector<SeekRange> &periodRanges, int64_t &totalEndNs)
{
    totalEndNs = 0;
    FALSE_RETURN_NOLOG(mpdInfo != nullptr);

    std::vector<DashPeriodInfo*> periodList;
    for (auto &periodInfo : mpdInfo->periodInfoList_) {
        if (periodInfo != nullptr) {
            periodList.push_back(periodInfo);
        }
    }

    for (size_t index = 0; index < periodList.size(); ++index) {
        auto periodInfo = periodList[index];
        uint64_t startNs = periodInfo->start_;
        uint64_t durationNs = periodInfo->duration_;
        if (durationNs == 0) {
            uint64_t nextStartNs = 0;
            if (index + 1 < periodList.size()) {
                nextStartNs = periodList[index + 1]->start_;
            }
            if (nextStartNs > startNs) {
                durationNs = nextStartNs - startNs;
            } else if (mpdInfo->mediaPresentationDuration_ > startNs) {
                durationNs = mpdInfo->mediaPresentationDuration_ - startNs;
            } else if (fallbackDurationNs > startNs) {
                durationNs = fallbackDurationNs - startNs;
            }
        }
        FALSE_CONTINUE_NOLOG(durationNs != 0);

        int64_t startVal = 0;
        int64_t durationI = 0;
        int64_t endNs = 0;
        if (!SafeIntCast<uint64_t, int64_t>(startNs, startVal) ||
            !SafeIntCast<uint64_t, int64_t>(durationNs, durationI) ||
            !SafeAdd(startVal, durationI, endNs) || endNs <= startVal) {
            continue;
        }
        periodRanges.push_back({startVal, endNs});
        totalEndNs = std::max(totalEndNs, endNs);
    }
}

std::vector<SeekRange> ClipRangesToWindow(const std::vector<SeekRange> &periodRanges,
    int64_t windowStart, int64_t windowEnd)
{
    std::vector<SeekRange> ranges;
    for (const auto &range : periodRanges) {
        int64_t start = std::max(range.start, windowStart);
        int64_t end = std::min(range.end, windowEnd);
        if (end > start) {
            ranges.push_back({start, end});
        }
    }
    if (ranges.empty()) {
        ranges.push_back({windowStart, windowEnd});
    }
    return ranges;
}
}

DashMpdDownloader::DashMpdDownloader(std::shared_ptr<MediaSourceLoaderCombinations> sourceLoader)
{
    if (sourceLoader != nullptr) {
        MEDIA_LOG_I("DashMpdDownloader app download.");
        downloader_ = std::make_shared<Downloader>("dashMpd", sourceLoader);
    } else {
        downloader_ = std::make_shared<Downloader>("dashMpd");
    }
    sourceLoader_ = sourceLoader;
    downloader_->Init();

    mpdParser_ = std::make_shared<DashMpdParser>();
    mpdManager_ = std::make_shared<DashMpdManager>();
    periodManager_ = std::make_shared<DashPeriodManager>();
    adptSetManager_ = std::make_shared<DashAdptSetManager>();
    representationManager_ = std::make_shared<DashRepresentationManager>();
}

DashMpdDownloader::~DashMpdDownloader()
{
    if (downloader_ != nullptr) {
        downloader_->Stop(false);
    }
    streamDescriptions_.clear();
    FALSE_RETURN_MSG(reportInfo_ != nullptr, "reportInfo_ is nullptr");
    FALSE_RETURN_MSG(mpdInfo_ != nullptr, "mpdInfo_ is nullptr");
    if (mpdInfo_->type_ == DashType::DASH_TYPE_STATIC) {
        reportInfo_->sourceType_ = static_cast<int8_t>(MediaAVCodec::DfxSourceType::DASHVOD);
    } else if (mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC) {
        reportInfo_->sourceType_ = static_cast<int8_t>(MediaAVCodec::DfxSourceType::DASHLIVE);
    }
    FALSE_RETURN_MSG(downloader_ != nullptr, "downloader_ is nullptr");
    reportInfo_->ca_ = downloader_->GetCertIssuerNames();
}

std::shared_ptr<DashSegment> DashMpdDownloader::BuildLazyFromTimeline(
    const std::shared_ptr<DashStreamDescription>& streamDesc, uint64_t index, uint64_t totalCount) const
{
    uint64_t remaining = index;
    for (const auto& entry : streamDesc->timelineEntries_) {
        if (entry.repeatCount <= 0) {
            continue;
        }
        uint64_t cnt = static_cast<uint64_t>(entry.repeatCount);
        if (remaining < cnt) {
            auto seg = std::make_shared<DashSegment>();
            seg->streamId_ = streamDesc->streamId_;
            seg->bandwidth_ = streamDesc->bandwidth_;
            seg->duration_ = entry.segDurationNs;
            seg->startNumberSeq_ = streamDesc->startNumberSeq_;
            int64_t numberSeq = 0;
            if (!SafeAdd(entry.startSeq, static_cast<int64_t>(remaining), numberSeq)) {
                MEDIA_LOG_W("BuildLazyFromTimeline SafeAdd overflow, startSeq=" PUBLIC_LOG_D64
                    ", remaining=" PUBLIC_LOG_U64, entry.startSeq, remaining);
                return nullptr;
            }
            seg->numberSeq_ = numberSeq;
            seg->isLast_ = (index == totalCount - 1);
            uint64_t mulRes = 0;
            if (!SafeMul(remaining, entry.duration, mulRes)) {
                MEDIA_LOG_W("BuildLazyFromTimeline SafeMul overflow, remaining=" PUBLIC_LOG_U64
                    ", duration=" PUBLIC_LOG_U64, remaining, entry.duration);
                return nullptr;
            }
            uint64_t segTime = 0;
            if (!SafeAdd(entry.startTime, mulRes, segTime)) {
                MEDIA_LOG_W("BuildLazyFromTimeline SafeAdd overflow, startTime=" PUBLIC_LOG_U64
                    ", mulRes=" PUBLIC_LOG_U64, entry.startTime, mulRes);
                return nullptr;
            }
            std::string url = streamDesc->durationSegInfo_.mediaUrlTemplate;
            if (DashSubstituteTmpltStr(url, "$Time", std::to_string(segTime)) == -1 ||
                DashSubstituteTmpltStr(url, "$Number", std::to_string(seg->numberSeq_)) == -1) {
                return nullptr;
            }
            seg->url_ = url;
            return seg;
        }
        remaining -= cnt;
    }
    return nullptr;
}

std::shared_ptr<DashSegment> DashMpdDownloader::BuildLazyFromSidx(
    const std::shared_ptr<DashStreamDescription>& streamDesc, uint64_t index, uint64_t totalCount) const
{
    if (index >= streamDesc->sidxEntries_.size()) {
        return nullptr;
    }
    const auto& entry = streamDesc->sidxEntries_[index];
    auto seg = std::make_shared<DashSegment>();
    seg->streamId_ = streamDesc->streamId_;
    seg->bandwidth_ = streamDesc->bandwidth_;
    seg->duration_ = entry.durationNs;
    seg->startNumberSeq_ = streamDesc->startNumberSeq_;
    seg->numberSeq_ = entry.numberSeq;
    seg->startRangeValue_ = entry.startRange;
    seg->endRangeValue_ = entry.endRange;
    seg->url_ = entry.url;
    seg->isLast_ = (index == totalCount - 1);
    return seg;
}

std::shared_ptr<DashSegment> DashMpdDownloader::BuildLazyFromSegList(
    const std::shared_ptr<DashStreamDescription>& streamDesc, uint64_t index, uint64_t totalCount) const
{
    if (index >= streamDesc->segListEntries_.size()) {
        return nullptr;
    }
    const auto& entry = streamDesc->segListEntries_[index];
    auto seg = std::make_shared<DashSegment>();
    seg->streamId_ = streamDesc->streamId_;
    seg->bandwidth_ = streamDesc->bandwidth_;
    seg->duration_ = entry.durationNs;
    seg->startNumberSeq_ = streamDesc->startNumberSeq_;
    seg->numberSeq_ = entry.numberSeq;
    seg->url_ = entry.mediaUrl;
    seg->byteRange_ = entry.mediaRange;
    seg->isLast_ = (index == totalCount - 1);
    return seg;
}

std::shared_ptr<DashSegment> DashMpdDownloader::BuildLazyFromDuration(
    const std::shared_ptr<DashStreamDescription>& streamDesc, uint64_t index, uint64_t totalCount) const
{
    auto seg = std::make_shared<DashSegment>();
    seg->streamId_ = streamDesc->streamId_;
    seg->bandwidth_ = streamDesc->bandwidth_;
    seg->duration_ = streamDesc->GetMediaSegmentDuration(index);
    seg->startNumberSeq_ = streamDesc->startNumberSeq_;
    seg->numberSeq_ = streamDesc->GetMediaSegmentNumberSeq(index);
    seg->isLast_ = (index == totalCount - 1);
    if (seg->numberSeq_ < 0) {
        MEDIA_LOG_W("BuildLazyFromDuration invalid numberSeq=" PUBLIC_LOG_D64 ", index=" PUBLIC_LOG_U64,
            seg->numberSeq_, index);
        return nullptr;
    }
    int64_t subRes = 0;
    if (!SafeSub<int64_t>(seg->numberSeq_, 1, subRes)) {
        MEDIA_LOG_W("BuildLazyFromDuration SafeSub overflow, numberSeq=" PUBLIC_LOG_D64, seg->numberSeq_);
        return nullptr;
    }
    int64_t sTime = 0;
    if (!SafeMul(subRes, streamDesc->durationSegInfo_.baseDuration, sTime)) {
        MEDIA_LOG_W("BuildLazyFromDuration SafeMul overflow, subRes=" PUBLIC_LOG_D64
            ", baseDuration=" PUBLIC_LOG_D64, subRes, streamDesc->durationSegInfo_.baseDuration);
        return nullptr;
    }
    std::string url = streamDesc->durationSegInfo_.mediaUrlTemplate;
    if (DashSubstituteTmpltStr(url, "$Time", std::to_string(sTime)) == -1 ||
        DashSubstituteTmpltStr(url, "$Number", std::to_string(seg->numberSeq_)) == -1) {
        return nullptr;
    }
    seg->url_ = url;
    return seg;
}

std::shared_ptr<DashSegment> DashMpdDownloader::BuildLazySegment(
    const std::shared_ptr<DashStreamDescription>& streamDesc, uint64_t index) const
{
    if (streamDesc == nullptr || streamDesc->lazyMode_ == DASH_LAZY_MODE_NONE) {
        if (streamDesc != nullptr && index < streamDesc->mediaSegments_.size()) {
            return streamDesc->mediaSegments_[index];
        }
        return nullptr;
    }
    uint64_t totalCount = streamDesc->GetMediaSegmentCount();
    if (index >= totalCount) {
        return nullptr;
    }
    switch (streamDesc->lazyMode_) {
        case DASH_LAZY_MODE_TIMELINE:
            return BuildLazyFromTimeline(streamDesc, index, totalCount);
        case DASH_LAZY_MODE_SIDX:
            return BuildLazyFromSidx(streamDesc, index, totalCount);
        case DASH_LAZY_MODE_SEGLIST:
            return BuildLazyFromSegList(streamDesc, index, totalCount);
        case DASH_LAZY_MODE_DURATION:
            return BuildLazyFromDuration(streamDesc, index, totalCount);
        default:
            return nullptr;
    }
}

std::vector<MpdStreamResource> DashMpdDownloader::ParseMpdResources(const uint8_t* data, size_t size,
    const std::string& rootUrl)
{
    std::vector<MpdStreamResource> resources;
    if (data == nullptr || size == 0) {
        MEDIA_LOG_E("ParseMpdResources: data is null or size is 0");
        return resources;
    }

    MEDIA_LOG_I("ParseMpdResources: rootUrl=%{public}s, data size=%{public}zu", rootUrl.c_str(), size);

    auto downloader = std::make_shared<DashMpdDownloader>(nullptr);
    downloader->url_ = rootUrl;

    std::string mpdContent(reinterpret_cast<const char*>(data), size);
    {
        std::lock_guard<std::mutex> lock(downloader->downloadContentMutex_);
        downloader->downloadContent_ = mpdContent;
    }
    downloader->ParseManifest();
    MEDIA_LOG_I("ParseMpdResources: streamDescriptions_.size()=%{public}zu",
        downloader->streamDescriptions_.size());

    BuildMpdStreamResources(downloader, resources);

    MEDIA_LOG_I("ParseMpdResources: done, total resources=%{public}zu", resources.size());
    return resources;
}

void DashMpdDownloader::BuildMpdStreamResources(std::shared_ptr<DashMpdDownloader>& downloader,
    std::vector<MpdStreamResource>& resources)
{
    for (size_t i = 0; i < downloader->streamDescriptions_.size(); i++) {
        auto& streamDesc = downloader->streamDescriptions_[i];
        if (streamDesc == nullptr) {
            continue;
        }
        downloader->GetSegmentsInMpd(streamDesc);

        StreamInfo si = downloader->AssignStreamInfo(i);
        if (streamDesc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            si.type = StreamType::SUBTITLE;
        } else if (streamDesc->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            si.type = StreamType::AUDIO;
        } else {
            si.type = StreamType::VIDEO;
        }
        si.codecs = downloader->GetMimeType(streamDesc->codecs_, si.type);
        si.originCodecs = streamDesc->codecs_;

        MpdStreamResource res;
        res.info = si;
        if (streamDesc->initSegment_ != nullptr && !streamDesc->initSegment_->url_.empty()) {
            res.urls.push_back(streamDesc->initSegment_->url_);
        }
        downloader->CollectSegmentUrls(streamDesc, res.urls);
        MEDIA_LOG_D("ParseMpdResources: stream[%{public}zu] type=%{public}d, bw=%{public}u, "
            "res=%{public}dx%{public}d, urls=%{public}zu",
            i, static_cast<int>(si.type), si.bitRate, si.videoWidth, si.videoHeight, res.urls.size());
        resources.push_back(std::move(res));
    }
}

void DashMpdDownloader::CollectSegmentUrls(const std::shared_ptr<DashStreamDescription>& streamDesc,
    std::vector<std::string>& urls)
{
    if (streamDesc->lazyMode_ == DASH_LAZY_MODE_NONE) {
        for (const auto& segment : streamDesc->mediaSegments_) {
            if (segment && !segment->url_.empty()) {
                urls.push_back(segment->url_);
            }
        }
    } else {
        uint64_t segCount = streamDesc->GetMediaSegmentCount();
        for (uint64_t i = 0; i < segCount; i++) {
            auto seg = BuildLazySegment(streamDesc, i);
            if (seg && !seg->url_.empty()) {
                urls.push_back(seg->url_);
            }
        }
    }
}

void DashMpdDownloader::Init()
{
    auto weakDownloader = weak_from_this();
    dataSave_ = [weakDownloader] (uint8_t*&& data, uint32_t&& len, bool&& notBlock) -> uint32_t {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_V_MSG(shareDownloader != nullptr, 0u, "doneCb, dash mpd downloader already destructed.");
        return shareDownloader->SaveData(std::forward<decltype(data)>(data), std::forward<decltype(len)>(len),
            std::forward<decltype(notBlock)>(notBlock));
    };
    if (downloader_!= nullptr && downloadCallback_ != nullptr) {
        downloader_->SetDownloadCallback(downloadCallback_);
    }
}

void DashMpdDownloader::SetDownloadCallback(const std::shared_ptr<DownloadMetricsInfo> &callback)
{
    downloadCallback_ = callback;
}

void DashMpdDownloader::SetCallback(const std::shared_ptr<Callback>& cb)
{
    eventCallback_ = cb;
}

std::string DashMpdDownloader::GetContentType()
{
    FALSE_RETURN_V(downloader_ != nullptr, "");
    return downloader_->GetContentType();
}

static int64_t ParseStartNumber(const std::string &numberStr)
{
    int64_t startNum = 1;
    if (numberStr.length() > 0) {
        int64_t tmp = 0;
        if (StringUtil::SafeStoInt64(numberStr, tmp) && tmp >= 0) {
            startNum = tmp;
        }
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

// Apply base URL to lazy segment template (instead of individual segment URLs)
static void AppendBaseUrlIfRelative(const std::string &baseUrl, std::string &url)
{
    if (url.empty() || DashUrlIsAbsolute(url)) {
        return;
    }
    std::string segUrl = baseUrl;
    DashAppendBaseUrl(segUrl, url);
    url = segUrl;
}

static void MakeAbsoluteWithBaseUrlLazy(std::shared_ptr<DashStreamDescription> streamDesc,
    const std::string &baseUrl)
{
    if (streamDesc == nullptr || streamDesc->lazyMode_ == DASH_LAZY_MODE_NONE) {
        return;
    }
    switch (streamDesc->lazyMode_) {
        case DASH_LAZY_MODE_TIMELINE:
        case DASH_LAZY_MODE_DURATION:
            AppendBaseUrlIfRelative(baseUrl, streamDesc->durationSegInfo_.mediaUrlTemplate);
            break;
        case DASH_LAZY_MODE_SIDX:
            for (auto& entry : streamDesc->sidxEntries_) {
                AppendBaseUrlIfRelative(baseUrl, entry.url);
            }
            break;
        case DASH_LAZY_MODE_SEGLIST:
            for (auto& entry : streamDesc->segListEntries_) {
                AppendBaseUrlIfRelative(baseUrl, entry.mediaUrl);
            }
            break;
        default:
            break;
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

    auto downloadRequest = GetDownloadRequest();
    if (downloadRequest != nullptr && !downloadRequest->IsClosed()) {
        downloadRequest->Close();
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
        if (streamDescription->segsState_ != DASH_SEGS_STATE_FINISH) {
            ret = DASH_MPD_GET_UNDONE;
            break;
        }
        int64_t segmentIndex = 0;
        if (!ComputeNextSegmentIndex(streamDescription, segmentIndex)) {
            ret = DASH_MPD_GET_ERROR;
            break;
        }
        MEDIA_LOG_D("get segment index :" PUBLIC_LOG_D64 ", id:" PUBLIC_LOG_D32 ", seq:"
            PUBLIC_LOG_D64, segmentIndex, streamDescription->streamId_, streamDescription->currentNumberSeq_);
        uint64_t segCount = streamDescription->GetMediaSegmentCount();
        if (segmentIndex < 0 || static_cast<uint64_t>(segmentIndex) >= segCount) {
            ret = DASH_MPD_GET_FINISH;
            break;
        }
        seg = BuildLazySegment(streamDescription, static_cast<uint64_t>(segmentIndex));
        if (seg != nullptr) {
            streamDescription->currentNumberSeq_ = seg->numberSeq_;
            MEDIA_LOG_D("after get segment index :" PUBLIC_LOG_D64, streamDescription->currentNumberSeq_);
            ret = DASH_MPD_GET_DONE;
        } else {
            ret = DASH_MPD_GET_FINISH;
        }
        break;
    }

    return ret;
}

bool DashMpdDownloader::ComputeNextSegmentIndex(const std::shared_ptr<DashStreamDescription> &streamDesc,
    int64_t &segmentIndex) const
{
    if (streamDesc->currentNumberSeq_ == -1) {
        segmentIndex = 0;
        return true;
    }
    int64_t subRes = 0;
    if (!SafeSub(streamDesc->currentNumberSeq_, streamDesc->startNumberSeq_, subRes)) {
        MEDIA_LOG_E("ComputeNextSegmentIndex SafeSub overflow, cur=" PUBLIC_LOG_D64
            ", start=" PUBLIC_LOG_D64, streamDesc->currentNumberSeq_, streamDesc->startNumberSeq_);
        return false;
    }
    if (!SafeAdd<int64_t>(subRes, 1, segmentIndex)) {
        MEDIA_LOG_E("ComputeNextSegmentIndex SafeAdd overflow, subRes=" PUBLIC_LOG_D64, subRes);
        return false;
    }
    return true;
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
        uint64_t segCount = streamDescription->GetMediaSegmentCount();
        for (uint64_t index = 0; index < segCount; index++) {
            int64_t curDuration = static_cast<int64_t>(streamDescription->GetMediaSegmentDuration(index));
            if (segmentDuration + curDuration >= breakpoint) {
                seg = BuildLazySegment(streamDescription, index);
                break;
            }
            int64_t newSegDur = 0;
            if (!SafeAdd(segmentDuration, curDuration, newSegDur)) {
                MEDIA_LOG_W("GetBreakPointSegment SafeAdd overflow, stop seeking");
                break;
            }
            segmentDuration = newSegDur;
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
    bool isFirstSelect = true;
    uint32_t maxGap = 0;
    for (const auto &stream : streamDescriptions_) {
        if (stream->type_ != MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
            continue;
        }

        uint32_t tempGap = (stream->bandwidth_ > param.bitrate_) ?
            (stream->bandwidth_ - param.bitrate_) : (param.bitrate_ - stream->bandwidth_);
        if (isFirstSelect || (tempGap < maxGap)) {
            isFirstSelect = false;
            maxGap = tempGap;
            destStream = stream;
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
    if (param.position_ == -1) {
        destStream->currentNumberSeq_ = currentStream->currentNumberSeq_;
    } else {
        destStream->currentNumberSeq_ = param.position_;
    }

    param.nextSegTime_ = GetSegTimeByStream(currentStream, destStream->currentNumberSeq_);
    MEDIA_LOG_I("select bitrate from type:" PUBLIC_LOG_D32 ", to type:" PUBLIC_LOG_D32 ", nextSegTime:"
        PUBLIC_LOG_U64 ", bandwidth:" PUBLIC_LOG_U32 ", id:" PUBLIC_LOG_D32 ", width:"
        PUBLIC_LOG_U32, currentStream->videoType_, destStream->videoType_, param.nextSegTime_,
        destStream->bandwidth_, destStream->streamId_, destStream->width_);

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

    param.nextSegTime_ = GetSegTimeByStream(currentStream, destStream->currentNumberSeq_);
    MEDIA_LOG_I("select track current lang:" PUBLIC_LOG_S ", change to:" PUBLIC_LOG_S ",nextSegTime:"
        PUBLIC_LOG_U64, currentStream->lang_.c_str(), destStream->lang_.c_str(), param.nextSegTime_);

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
    uint64_t nextSegTime)
{
    if (streamDesc == nullptr) {
        return;
    }

    uint64_t previousSegsDuration = 0;
    int64_t numberSeq = -1;
    uint64_t segCount = streamDesc->GetMediaSegmentCount();
    for (uint64_t index = 0; index < segCount; index++) {
        uint64_t newDur = 0;
        uint64_t curSegDuration = streamDesc->GetMediaSegmentDuration(index);
        if (!SafeAdd(previousSegsDuration, curSegDuration, newDur)) {
            MEDIA_LOG_W("UpdateCurrentNumberSeqByTime SafeAdd overflow, stop");
            break;
        }
        previousSegsDuration = newDur;
        int64_t numberSeqTemp = streamDesc->GetMediaSegmentNumberSeq(index);
        // previousSegsDuration greater than nextSegTime 100ms, get this segment, otherwise find next segment to check
        if (previousSegsDuration > nextSegTime && (previousSegsDuration - nextSegTime) > SEGMENT_DURATION_DELTA_NS) {
            MEDIA_LOG_I("UpdateSeqByTime find next seq:" PUBLIC_LOG_D64, numberSeqTemp);
            break;
        }
        numberSeq = numberSeqTemp;
    }

    MEDIA_LOG_I("UpdateSeqByTime change current seq from:" PUBLIC_LOG_D64 " to:" PUBLIC_LOG_D64 ", nextSegTime:"
        PUBLIC_LOG_U64, streamDesc->currentNumberSeq_, numberSeq, nextSegTime);
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
        uint64_t product = static_cast<uint64_t>(width) * static_cast<uint64_t>(height);
        if (product > std::numeric_limits<unsigned int>::max()) {
            initResolution_ = std::numeric_limits<unsigned int>::max();
        } else {
            initResolution_ = static_cast<unsigned int>(product);
        }
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
    isInterruptNeeded_.store(isInterruptNeeded);
    if (downloader_ != nullptr) {
        downloader_->SetInterruptState(isInterruptNeeded);
    }
}

std::string DashMpdDownloader::GetUrl() const
{
    return url_;
}

DashList<DashEventStreamInfo *> DashMpdDownloader::GetCurrentPeriodEventStreams() const
{
    if (mpdInfo_ == nullptr || mpdManager_ == nullptr) {
        return DashList<DashEventStreamInfo *>();
    }

    DashPeriodInfo *currentPeriod = mpdManager_->GetCurrentPeriod();
    if (currentPeriod == nullptr) {
        return DashList<DashEventStreamInfo *>();
    }

    return currentPeriod->eventStreamList_;
}

void DashMpdDownloader::ParseManifest()
{
    std::string content;
    {
        std::lock_guard<std::mutex> lock(downloadContentMutex_);
        content = downloadContent_;
    }
    if (content.length() == 0) {
        MEDIA_LOG_I("ParseManifest content length is 0");
        return;
    }

    mpdParser_->ParseMPD(content.c_str(), content.length());
    if (mpdParser_->IsParseFailed()) {
        MEDIA_LOG_E("MPD parse failed, report error to upper layer");
        auto callback = callback_.lock();
        if (callback) {
            callback->OnMpdInfoUpdate(DASH_MPD_EVENT_PARSE_FAILED);
        }
        return;
    }
    mpdParser_->GetMPD(mpdInfo_);
    if (mpdInfo_ != nullptr) {
        mpdManager_->SetMpdInfo(mpdInfo_, url_);
        mpdManager_->GetDuration(&duration_);
        UpdateSeekRanges();
        SetOndemandSegBase();
        GetStreamsInfoInMpd();
        ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_VID);
        ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_AUD);
        ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE);
        auto eventCallback = eventCallback_.lock();
        if (eventCallback) {
            std::string type = "dash";
            eventCallback->OnEvent({PluginEventType::STREAM_UPDATE, {type}, "Stream update."});
        }
    }
}

bool DashMpdDownloader::ChooseStreamToPlay(MediaAVCodec::MediaType type)
{
    if (mpdInfo_ == nullptr || streamDescriptions_.empty()) {
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
            FALSE_CONTINUE_NOLOG(IsLangMatch(stream->lang_, type));
            choosedStream = stream;
            break;
        }
        if (backupStream == nullptr || IsNearToInitResolution(backupStream, stream) ||
            (initResolution_ == 0 && backupStream->bandwidth_ > stream->bandwidth_)) {
            backupStream = stream;
        }
        FALSE_CONTINUE_NOLOG((stream->videoType_ != DASH_VIDEO_TYPE_SDR) == isHdrStart_);
        if (IsChoosedVideoStream(choosedStream, stream)) {
            choosedStream = stream;
        }
    }
    if (choosedStream == nullptr && type == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        choosedStream = backupStream;
    }
    FALSE_RETURN_V(choosedStream != nullptr, false);
    if (type == MediaAVCodec::MediaType::MEDIA_TYPE_VID) {
        videoStreamId_ = choosedStream->streamId_;
    } else if (type == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
        audioStreamId_ = choosedStream->streamId_;
    } else if (type == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
        subTitleStreamId_ = choosedStream->streamId_;
    } else {
        MEDIA_LOG_I("ChooseStreamToPlay type error!");
    }
    return true;
}

void DashMpdDownloader::SetDefaultStreamId(int32_t &videoStreamId, int32_t &audioStreamId, int32_t &subTitleStreamId)
{
    if (mpdInfo_ == nullptr) {
        return;
    }
    if (videoStreamId != -1) {
        videoStreamId_ = videoStreamId;
    } else {
        videoStreamId = videoStreamId_;
    }

    if (audioStreamId != -1) {
        audioStreamId_ = audioStreamId;
    } else {
        audioStreamId = audioStreamId_;
    }

    if (subTitleStreamId != -1) {
        subTitleStreamId_ = subTitleStreamId;
    } else {
        subTitleStreamId = subTitleStreamId_;
    }

    ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_VID, videoStreamId_);
    ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_AUD, audioStreamId_);
    ChooseStreamToPlay(MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE, subTitleStreamId_);
    if (ondemandSegBase_) {
        PutStreamToDownload();
        ProcessDrmInfos();
        return;
    }

    auto callback = callback_.lock();
    if (callback) {
        callback->OnMpdInfoUpdate(DASH_MPD_EVENT_STREAM_INIT);
    }

    ProcessDrmInfos();
    notifyOpenOk_.store(true);
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
        if (psshString.length() > MAX_PSSH_BASE64_LEN) {
            MEDIA_LOG_W("pssh base64 length too large, drmId " PUBLIC_LOG_S, drmInfo.drmId_.c_str());
            continue;
        }
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
                ssConverter << std::hex << std::setfill('0') << std::setw(2) << static_cast<int32_t>(uuid[i]); // 2:w
                uuidString = ssConverter.str();
            }
            drmInfoMap.insert({uuidString, std::vector<uint8_t>(pssh, pssh + psshSize)});
            localDrmInfos_.emplace_back(drmInfo);
        } else {
            MEDIA_LOG_W("Base64Decode pssh error, drmId " PUBLIC_LOG_S, drmInfo.drmId_.c_str());
        }
    }

    auto callback = callback_.lock();
    if (callback) {
        callback->OnDrmInfoChanged(drmInfoMap);
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
    std::string content;
    {
        std::lock_guard<std::mutex> lock(downloadContentMutex_);
        content = downloadContent_;
    }
    if (content.length() == 0 || currentDownloadStream_ == nullptr ||
        currentDownloadStream_->indexSegment_ == nullptr) {
        MEDIA_LOG_I("ParseSidx content length is 0 or stream is nullptr");
        return;
    }

    std::string sidxContent = content;
    if (CheckToDownloadSidxWithInitSeg(currentDownloadStream_)) {
        currentDownloadStream_->initSegment_->content_ = content;
        currentDownloadStream_->initSegment_->isDownloadFinish_ = true;
        MEDIA_LOG_I("ParseSidx update init segment content size is "
            PUBLIC_LOG_ZU, currentDownloadStream_->initSegment_->content_.size());
        int64_t initLen = currentDownloadStream_->indexSegment_->indexRangeBegin_ -
            currentDownloadStream_->initSegment_->rangeBegin_;
        if (initLen >= 0 && static_cast<size_t>(initLen) < content.size()) {
            sidxContent = content.substr(static_cast<size_t>(initLen));
            MEDIA_LOG_I("ParseSidx update sidx segment content size is " PUBLIC_LOG_ZU, sidxContent.size());
        }
    }

    std::list<std::shared_ptr<SubSegmentIndex>> subSegIndexList;
    int32_t parseRet = SidxBoxParser::ParseSidxBox(sidxContent.data(), sidxContent.length(),
        currentDownloadStream_->indexSegment_->indexRangeEnd_, subSegIndexList);
    if (parseRet != 0) {
        MEDIA_LOG_E("sidx box parse error");
        return;
    }

    BuildSidxEntries(subSegIndexList);
    currentDownloadStream_->segsState_ = DASH_SEGS_STATE_FINISH;
    NotifyParseSidxResult();
}

void DashMpdDownloader::NotifyParseSidxResult()
{
    auto callback = callback_.lock();
    if (!notifyOpenOk_.load()) {
        if (!PutStreamToDownload()) {
            if (callback) {
                callback->OnMpdInfoUpdate(DASH_MPD_EVENT_STREAM_INIT);
            }
            notifyOpenOk_.store(true);
        }
    } else {
        if (callback) {
            callback->OnMpdInfoUpdate(DASH_MPD_EVENT_PARSE_OK);
        }
    }
}

void DashMpdDownloader::BuildSidxEntries(std::list<std::shared_ptr<SubSegmentIndex>> &subSegIndexList) const
{
    uint64_t segDurSum = 0; // the sum of segment duration, not devide timescale
    uint64_t segAddDuration = 0; // add all segments duration(ns) before current segment
    int64_t segSeq = currentDownloadStream_->startNumberSeq_;
    std::string segUrl;
    if (currentDownloadStream_->indexSegment_ != nullptr) {
        segUrl = currentDownloadStream_->indexSegment_->url_;
    }
    // Lazy loading: store compact sidx entries instead of expanding into DashSegment objects.
    currentDownloadStream_->lazyMode_ = DASH_LAZY_MODE_SIDX;
    currentDownloadStream_->sidxEntries_.reserve(subSegIndexList.size());
    for (const auto &subSegIndex : subSegIndexList) {
        if (currentDownloadStream_->sidxEntries_.size() >= MAX_SIDX_ENTRIES) {
            MEDIA_LOG_W("sidxEntries count " PUBLIC_LOG_ZU " exceeds max " PUBLIC_LOG_ZU
                ", stop parsing", currentDownloadStream_->sidxEntries_.size(), MAX_SIDX_ENTRIES);
            break;
        }
        if (!AppendSidxEntry(subSegIndex, segUrl, segDurSum, segAddDuration, segSeq)) {
            break;
        }
    }

    // For static MPD, isLast_ is set dynamically in BuildLazySegment (index == count - 1).
    if (mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC) {
        if (sourceLoader_ && sourceLoader_->GetenableOfflineCache()) {
            sourceLoader_->Close(-1);
        }
    }
}

bool DashMpdDownloader::AppendSidxEntry(const std::shared_ptr<SubSegmentIndex> &subSegIndex,
    const std::string &segUrl, uint64_t &segDurSum, uint64_t &segAddDuration, int64_t &segSeq) const
{
    uint64_t timeScale = (subSegIndex->timeScale_ > 0) ? subSegIndex->timeScale_ : 1;
    uint64_t mulRes = 0;
    if (!SafeMul(subSegIndex->duration_, S_2_NS, mulRes)) {
        MEDIA_LOG_W("AppendSidxEntry SafeMul overflow, duration=" PUBLIC_LOG_U64, subSegIndex->duration_);
        return false;
    }
    uint64_t durationNS = mulRes / timeScale;
    uint64_t newSegDurSum = 0;
    if (!SafeAdd(segDurSum, subSegIndex->duration_, newSegDurSum)) {
        MEDIA_LOG_W("AppendSidxEntry SafeAdd(segDurSum) overflow");
        return false;
    }
    segDurSum = newSegDurSum;
    if (!SafeMul(segDurSum, S_2_NS, mulRes)) {
        MEDIA_LOG_W("AppendSidxEntry SafeMul(segDurSum) overflow");
        return false;
    }
    uint64_t segDurNsSum = mulRes / timeScale;
    if (segDurNsSum > segAddDuration) {
        durationNS = segDurNsSum - segAddDuration;
    }
    uint64_t newSegAddDur = 0;
    if (!SafeAdd(segAddDuration, durationNS, newSegAddDur)) {
        MEDIA_LOG_W("AppendSidxEntry SafeAdd(segAddDuration) overflow");
        return false;
    }
    segAddDuration = newSegAddDur;

    DashSidxEntry entry;
    entry.durationNs = durationNS;
    entry.numberSeq = segSeq++;
    entry.startRange = subSegIndex->startPos_;
    entry.endRange = subSegIndex->endPos_;
    entry.url = segUrl;
    currentDownloadStream_->sidxEntries_.push_back(entry);
    return true;
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
    {
        std::lock_guard<std::mutex> lock(downloadContentMutex_);
        downloadContent_.clear();
    }
    auto weakDownloader = weak_from_this();
    auto realStatusCallback = [weakDownloader](DownloadStatus &&status, std::shared_ptr<Downloader> &downloader,
        std::shared_ptr<DownloadRequest> &request) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "realStatusCb, dash mpd downloader already destructed.");
        if (shareDownloader->statusCallback_ != nullptr) {
            shareDownloader->statusCallback_(status, shareDownloader->downloader_,
                std::forward<decltype(request)>(request));
        }
    };

    bool requestWholeFile = true;
    if (startRange > 0 || endRange > 0) {
        requestWholeFile = false;
    }

    MEDIA_LOG_I("DoOpen:start=%{public}lld end=%{public}lld", (long long) startRange, (long long) endRange);
    RequestInfo mediaSource;
    mediaSource.url = url;
    mediaSource.timeoutMs = MPD_HTTP_TIME_OUT_MS;
    SetDownloadRequest(std::make_shared<DownloadRequest>(dataSave_, realStatusCallback, mediaSource,
        requestWholeFile));
    auto downloadDoneCallback = [weakDownloader](const std::string &url, const std::string &location) {
        auto shareDownloader = weakDownloader.lock();
        FALSE_RETURN_MSG(shareDownloader != nullptr, "downloadDoneCb, dash mpd downloader already destructed.");
        shareDownloader->UpdateDownloadFinished(url);
    };
    auto downloadRequest = GetDownloadRequest();
    FALSE_RETURN_MSG(downloadRequest != nullptr, "downloadRequest is nullptr");
    downloadRequest->SetDownloadDoneCb(downloadDoneCallback);
    downloadRequest->SetRequestProtocolType(RequestProtocolType::DASH);
    downloadRequest->SetRequestType(RequestType::DASH_MPD);
    if (!requestWholeFile) {
        downloadRequest->SetRangePos(startRange, endRange);
    }
    downloader_->Download(downloadRequest, -1); // -1
    downloader_->Start();
}

uint32_t DashMpdDownloader::SaveData(uint8_t* data, uint32_t len, bool notBlock)
{
    std::lock_guard<std::mutex> lock(downloadContentMutex_);
    MEDIA_LOG_D("SaveData:size=%{public}zu len=%{public}u", downloadContent_.size(), len);
    FALSE_RETURN_V_MSG(data != nullptr && len <= MPD_MAX_SIZE, 0, "SaveData failed, dash mpd data too large.");
    if (downloadContent_.size() + len > MPD_MAX_SIZE) {
        MEDIA_LOG_W("downloadContent_ size exceeds max, reject to prevent OOM");
        return 0;
    }
    downloadContent_.append(reinterpret_cast<const char*>(data), len);
    return len;
}

void DashMpdDownloader::SetMpdCallback(const std::shared_ptr<DashMpdCallback>& callback)
{
    callback_ = callback;
}

int64_t DashMpdDownloader::GetDuration() const
{
    if (mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC) {
        uint64_t timeShiftDepth = mpdInfo_->timeShiftBufferDepth_;
        if (timeShiftDepth > 0) {
            MEDIA_LOG_I("GetDuration dynamic timeShiftBufferDepth " PUBLIC_LOG_U64, timeShiftDepth);
            return static_cast<int64_t>(timeShiftDepth);
        }
    }
    MEDIA_LOG_I("GetDuration " PUBLIC_LOG_U64, duration_);
    if (duration_ == 0) {
        return 0;
    }
    return static_cast<int64_t>(duration_);
}

Seekable DashMpdDownloader::GetSeekable() const
{
    // need wait mpdInfo_ not null
    size_t times = 0;
    while (times < RETRY_TIMES && !isInterruptNeeded_.load()) {
        if (mpdInfo_ != nullptr && notifyOpenOk_.load()) {
            break;
        }
        OSAL::SleepFor(SLEEP_TIME);
        times++;
    }

    if (times >= RETRY_TIMES || isInterruptNeeded_.load()) {
        MEDIA_LOG_I("GetSeekable INVALID");
        return Seekable::INVALID;
    }

    MEDIA_LOG_I("GetSeekable end");
    FALSE_RETURN_V(mpdInfo_ != nullptr, Seekable::INVALID);
    if (mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC) {
        return mpdInfo_->timeShiftBufferDepth_ > 0 ? Seekable::SEEKABLE : Seekable::UNSEEKABLE;
    }
    return Seekable::SEEKABLE;
}

std::vector<SeekRange> DashMpdDownloader::GetSeekableRanges() const
{
    std::lock_guard<std::mutex> lock(seekRangeMutex_);
    return seekRanges_;
}

void DashMpdDownloader::UpdateSeekRanges()
{
    std::vector<SeekRange> periodRanges;
    int64_t totalEndNs = 0;

    CollectPeriodRanges(mpdInfo_, duration_, periodRanges, totalEndNs);

    if (periodRanges.empty() && duration_ > 0) {
        int64_t endNs = static_cast<int64_t>(duration_);
        periodRanges.push_back({0, endNs});
        totalEndNs = endNs;
    }

    std::vector<SeekRange> ranges;
    if (mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC &&
        mpdInfo_->timeShiftBufferDepth_ > 0) {
        int64_t windowNs = static_cast<int64_t>(mpdInfo_->timeShiftBufferDepth_);
        int64_t windowEnd = totalEndNs > 0 ? totalEndNs : windowNs;
        int64_t windowStart = windowEnd - windowNs;
        if (windowStart < 0) {
            windowStart = 0;
        }
        ranges = ClipRangesToWindow(periodRanges, windowStart, windowEnd);
    } else {
        ranges = std::move(periodRanges);
    }

    std::lock_guard<std::mutex> lock(seekRangeMutex_);
    seekRanges_ = std::move(ranges);
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

std::shared_ptr<DashStreamDescription> DashMpdDownloader::FindStreamByStreamId(int streamId) const
{
    for (const auto &index : streamDescriptions_) {
        if (index != nullptr && index->streamId_ == streamId &&
            index->segsState_ == DASH_SEGS_STATE_FINISH) {
            return index;
        }
    }
    return nullptr;
}

bool DashMpdDownloader::FindTargetPeriodIndex(const std::shared_ptr<DashStreamDescription> &currentStream,
    int64_t seekTime, uint32_t &targetPeriodIndex, uint64_t &periodStartTime) const
{
    if (seekTime < 0) {
        return targetPeriodIndex != currentStream->periodIndex_;
    }
    uint64_t seekTimeU = static_cast<uint64_t>(seekTime);
    for (const auto& streamDesc : streamDescriptions_) {
        if (streamDesc != nullptr && streamDesc->type_ == currentStream->type_ &&
            streamDesc->periodStartTime_ <= seekTimeU &&
            streamDesc->periodStartTime_ >= periodStartTime) {
            targetPeriodIndex = streamDesc->periodIndex_;
            periodStartTime = streamDesc->periodStartTime_;
        }
    }
    return targetPeriodIndex != currentStream->periodIndex_;
}

void DashMpdDownloader::SwitchStreamIfNeeded(std::shared_ptr<DashStreamDescription> &currentStream,
    uint32_t targetPeriodIndex)
{
    for (const auto& streamDesc : streamDescriptions_) {
        if (streamDesc != nullptr &&
            streamDesc->periodIndex_ == targetPeriodIndex &&
            streamDesc->type_ == currentStream->type_) {
            currentStream = streamDesc;
            if (currentStream->GetMediaSegmentCount() == 0) {
                GetSegmentsInMpd(streamDesc);
            }
            break;
        }
    }
}

void DashMpdDownloader::FindSegmentByRelativeTime(std::shared_ptr<DashStreamDescription> &currentStream,
    int64_t relativeTime, std::shared_ptr<DashSegment> &seg, uint32_t targetPeriodIndex, uint64_t &totalDuration)
{
    uint64_t segCount = currentStream->GetMediaSegmentCount();
    if (segCount == 0 || relativeTime < 0) {
        return;
    }
    uint64_t relativeTimeU = static_cast<uint64_t>(relativeTime);
    for (uint64_t index = 0; index < segCount; index++) {
        uint64_t segDur = currentStream->GetMediaSegmentDuration(index);
        uint64_t newTotal = 0;
        if (!SafeAdd(totalDuration, segDur, newTotal)) {
            MEDIA_LOG_W("FindSegmentByRelativeTime SafeAdd overflow, stop seeking, total=" PUBLIC_LOG_U64
                ", segDur=" PUBLIC_LOG_U64, totalDuration, segDur);
            return;
        }
        totalDuration = newTotal;
        if (totalDuration > relativeTimeU) {
            seg = BuildLazySegment(currentStream, index);
            int64_t segNum = currentStream->GetMediaSegmentNumberSeq(index);
            MEDIA_LOG_I("Dash SeekToTs period" PUBLIC_LOG_U32 " segment totalDuration:" PUBLIC_LOG_U64 ", segNum:"
                PUBLIC_LOG_D64 ", duration:" PUBLIC_LOG_U64, targetPeriodIndex, totalDuration, segNum, segDur);
            totalDuration -= segDur;
            return;
        }
    }
}

int64_t DashMpdDownloader::SeekToTs(int streamId, int64_t seekTime, std::shared_ptr<DashSegment> &seg)
{
    seg = nullptr;

    std::shared_ptr<DashStreamDescription> currentStream = FindStreamByStreamId(streamId);

    if (currentStream == nullptr) {
        return 0;
    }

    uint32_t targetPeriodIndex = 0;
    uint64_t periodStartTime = 0;

    bool needSwitch = FindTargetPeriodIndex(currentStream, seekTime, targetPeriodIndex, periodStartTime);
    if (needSwitch) {
        SwitchStreamIfNeeded(currentStream, targetPeriodIndex);
    }

    int64_t relativeTime = 0;
    if (periodStartTime <= static_cast<uint64_t>(std::numeric_limits<int64_t>::max()) && seekTime >= 0) {
        relativeTime = seekTime - static_cast<int64_t>(periodStartTime);
        if (relativeTime < 0) {
            relativeTime = 0;
        }
    }

    uint64_t totalDuration = 0;
    FindSegmentByRelativeTime(currentStream, relativeTime, seg, targetPeriodIndex, totalDuration);
    uint64_t resultU = 0;
    if (!SafeAdd(totalDuration, periodStartTime, resultU)) {
        MEDIA_LOG_W("SeekToTs SafeAdd overflow, totalDuration=" PUBLIC_LOG_U64
            ", periodStartTime=" PUBLIC_LOG_U64, totalDuration, periodStartTime);
        return 0;
    }
    if (resultU > static_cast<uint64_t>(std::numeric_limits<int64_t>::max())) {
        MEDIA_LOG_W("SeekToTs result overflow int64_t, result=" PUBLIC_LOG_U64, resultU);
        return 0;
    }
    return static_cast<int64_t>(resultU);
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
    DashAdptSetInfo *adptSetInfo, std::string &periodBaseUrl, std::shared_ptr<DashStreamDescription> streamDesc)
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
    streamDesc.duration_ = (periodInfo->duration_ > 0) ? periodInfo->duration_ : duration_;  // nanoseconds
    streamDesc.periodIndex_ = periodIndex;
    streamDesc.periodStartTime_ = periodInfo->start_;
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
    FALSE_RETURN(adptSetInfo != nullptr);
    streamDesc.width_ = adptSetInfo->commonAttrsAndElements_.width_;
    streamDesc.height_ = adptSetInfo->commonAttrsAndElements_.height_;
    streamDesc.lang_ = adptSetInfo->lang_;
    streamDesc.minBandwidth_ = adptSetInfo->minBandwidth_;
    streamDesc.maxBandwidth_ = adptSetInfo->maxBandwidth_;
    streamDesc.minWidth_ = adptSetInfo->minWidth_;
    streamDesc.maxWidth_ = adptSetInfo->maxWidth_;
    streamDesc.minHeight_ = adptSetInfo->minHeight_;
    streamDesc.maxHeight_ = adptSetInfo->maxHeight_;
    streamDesc.channels_ = adptSetInfo->channels_;
    streamDesc.frameRate_ = adptSetInfo->commonAttrsAndElements_.frameRate_;
    streamDesc.minFrameRate_ = adptSetInfo->minFrameRate_;
    streamDesc.maxFrameRate_ = adptSetInfo->maxFrameRate_;
    streamDesc.mimeType_ = adptSetInfo->mimeType_;
    streamDesc.codecs_ = adptSetInfo->codecs_;
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

void DashMpdDownloader::FillStreamDescription(DashStreamDescription &streamDesc, const DashRepresentationInfo *rep,
    unsigned int index, DashVideoType defaultType)
{
    streamDesc.representationIndex_ = index;
    streamDesc.startNumberSeq_ = GetStartNumber(rep);
    streamDesc.bandwidth_ = rep->bandwidth_;
    if (rep->commonAttrsAndElements_.width_ > 0) {
        streamDesc.width_ = rep->commonAttrsAndElements_.width_;
    }

    if ((rep)->commonAttrsAndElements_.height_ > 0) {
        streamDesc.height_ = rep->commonAttrsAndElements_.height_;
    }

    if (!rep->commonAttrsAndElements_.codecs_.empty()) {
        streamDesc.codecs_ = rep->commonAttrsAndElements_.codecs_;
    }

    if (!rep->commonAttrsAndElements_.frameRate_.empty()) {
        streamDesc.frameRate_ = rep->commonAttrsAndElements_.frameRate_;
    }

    if (rep->commonAttrsAndElements_.channels_ > 0) {
        streamDesc.channels_ = rep->commonAttrsAndElements_.channels_;
    }

    if (defaultType != DASH_VIDEO_TYPE_SDR &&
        rep->commonAttrsAndElements_.cuvvVersion_.find("cuvv.") != std::string::npos) {
        streamDesc.videoType_ = DASH_VIDEO_TYPE_HDR_VIVID;
        MEDIA_LOG_I("current stream is hdr vivid, band:" PUBLIC_LOG_U32, streamDesc.bandwidth_);
    } else {
        streamDesc.videoType_ = defaultType;
    }
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
        FillStreamDescription(streamDesc, *it, repIndex, defaultVideoType);

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
    unsigned int resolution = 0;
    if (!SafeMul(width, height, resolution)) {
        MEDIA_LOG_W("GetResolutionDelta SafeMul overflow, w=" PUBLIC_LOG_U32 ", h=" PUBLIC_LOG_U32
            ", saturating to UINT_MAX", width, height);
        resolution = std::numeric_limits<unsigned int>::max();
    }
    return std::max(resolution, initResolution_) - std::min(resolution, initResolution_);
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

bool DashMpdDownloader::ChooseStreamToPlay(MediaAVCodec::MediaType type, int32_t streamId)
{
    if (mpdInfo_ == nullptr || streamDescriptions_.size() == 0) {
        return false;
    }

    std::shared_ptr<DashStreamDescription> choosedStream = nullptr;
    for (const auto &stream : streamDescriptions_) {
        if (stream->type_ != type || stream->inUse_) {
            continue;
        }

        if (streamId == stream->streamId_) {
            choosedStream = stream;
            break;
        }
    }

    if (choosedStream != nullptr) {
        choosedStream->inUse_ = true;
        MEDIA_LOG_I("ChooseStreamToPlay type:" PUBLIC_LOG_D32 " type: " PUBLIC_LOG_D32 " mimeType" PUBLIC_LOG_S
            " frameRate: " PUBLIC_LOG_S " bitRate: " PUBLIC_LOG_D32 "channels: " PUBLIC_LOG_D32
            " hight: " PUBLIC_LOG_D32 " width:" PUBLIC_LOG_D32 "lang: " PUBLIC_LOG_S,
            choosedStream->streamId_, choosedStream->type_, choosedStream->mimeType_.c_str(),
            choosedStream->frameRate_.c_str(), choosedStream->bandwidth_, choosedStream->channels_,
            choosedStream->height_, choosedStream->width_, choosedStream->lang_.c_str());
        if (!ondemandSegBase_) {
            GetSegmentsInMpd(choosedStream);
        }
        return true;
    }

    return false;
}

uint64_t DashMpdDownloader::GetSegTimeBySeq(const std::vector<std::shared_ptr<DashSegment>> &segments, int64_t segSeq)
{
    if (segments.size() == 0) {
        return 0;
    }

    uint64_t nextSegTime = 0;
    for (size_t index = 0; index < segments.size(); index++) {
        if (segments[index] && segments[index]->numberSeq_ > segSeq) {
            break;
        }

        uint64_t newTime = 0;
        if (!SafeAdd(nextSegTime, segments[index]->duration_, newTime)) {
            MEDIA_LOG_W("GetSegTimeBySeq SafeAdd overflow, saturating to UINT64_MAX");
            return std::numeric_limits<uint64_t>::max();
        }
        nextSegTime = newTime;
    }

    return nextSegTime;
}

uint64_t DashMpdDownloader::GetSegTimeByStream(const std::shared_ptr<DashStreamDescription> &streamDesc, int64_t segSeq)
{
    if (streamDesc == nullptr) {
        return 0;
    }
    if (streamDesc->lazyMode_ == DASH_LAZY_MODE_NONE) {
        return GetSegTimeBySeq(streamDesc->mediaSegments_, segSeq);
    }
    uint64_t segCount = streamDesc->GetMediaSegmentCount();
    uint64_t nextSegTime = 0;
    for (uint64_t index = 0; index < segCount; index++) {
        if (streamDesc->GetMediaSegmentNumberSeq(index) > segSeq) {
            break;
        }
        uint64_t newTime = 0;
        uint64_t curSegDuration = streamDesc->GetMediaSegmentDuration(index);
        if (!SafeAdd(nextSegTime, curSegDuration, newTime)) {
            MEDIA_LOG_W("GetSegTimeByStream SafeAdd overflow, saturating to UINT64_MAX");
            return std::numeric_limits<uint64_t>::max();
        }
        nextSegTime = newTime;
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

    if (streamDesc->GetMediaSegmentCount() == 0) {
        MEDIA_LOG_E("GetSegmentsInMpd failed, type " PUBLIC_LOG_D32, streamDesc->type_);
        return DASH_SEGMENT_INIT_FAILED;
    }

    // Mark last segment for static MPD (lazy modes compute isLast_ dynamically in BuildLazySegment)
    if (mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_STATIC &&
        streamDesc->lazyMode_ == DASH_LAZY_MODE_NONE && !streamDesc->mediaSegments_.empty()) {
        std::shared_ptr<DashSegment> lastSegment = streamDesc->mediaSegments_.back();
        if (lastSegment != nullptr) {
            lastSegment->isLast_ = true;
        }
    }
    if (mpdInfo_ != nullptr && mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC) {
        if (sourceLoader_ && sourceLoader_->GetenableOfflineCache()) {
            sourceLoader_->Close(-1);
        }
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
        if (streamDesc->lazyMode_ != DASH_LAZY_MODE_NONE) {
            MakeAbsoluteWithBaseUrlLazy(streamDesc, periodBaseUrl);
        } else {
            MakeAbsoluteWithBaseUrl(streamDesc->mediaSegments_, periodBaseUrl);
        }
    }

    return initValue;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInAdptSet(DashAdptSetInfo *adptSetInfo,
    const std::string &periodBaseUrl, std::shared_ptr<DashStreamDescription> streamDesc)
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
        if (streamDesc->lazyMode_ != DASH_LAZY_MODE_NONE) {
            MakeAbsoluteWithBaseUrlLazy(streamDesc, adptSetBaseUrl);
        } else {
            MakeAbsoluteWithBaseUrl(streamDesc->mediaSegments_, adptSetBaseUrl);
        }
    }

    return initValue;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsInRepresentation(DashRepresentationInfo *repInfo,
    const std::string &adptSetBaseUrl, std::shared_ptr<DashStreamDescription> streamDesc)
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
        if (streamDesc->lazyMode_ != DASH_LAZY_MODE_NONE) {
            MakeAbsoluteWithBaseUrlLazy(streamDesc, repBaseUrl);
        } else {
            MakeAbsoluteWithBaseUrl(streamDesc->mediaSegments_, repBaseUrl);
        }
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

    if (mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC) {
        if (sourceLoader_ && sourceLoader_->GetenableOfflineCache()) {
            sourceLoader_->Close(-1);
        }
    }
    return DASH_SEGMENT_INIT_FAILED;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltStatic(const DashSegTmpltInfo *segTmpltInfo,
    const std::string &mediaUrl, std::shared_ptr<DashStreamDescription> streamDesc)
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
 * Lazy loading: store template parameters instead of expanding while loop.
 * Segments are built on demand via BuildLazySegment().
 *
 * @param    segTmpltInfo           SegmentTemplate infomation
 * @param    mediaUrl               SegmentTemplate mediaSegment Url
 * @param    timeScale              the timeScale of this period, duration / timescale = seconds
 * @param    desc                   stream description
 *
 * @return   DashSegmentInitValue
 */
DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltDurationStatic(const DashSegTmpltInfo *segTmpltInfo,
    const std::string &mediaUrl, unsigned int timeScale, std::shared_ptr<DashStreamDescription> desc)
{
    unsigned int timeScaleTmp = timeScale > 0 ? timeScale : 1;
    uint64_t mulRes = 0;
    if (!SafeMul(segTmpltInfo->multSegBaseInfo_.duration_, S_2_NS, mulRes)) {
        MEDIA_LOG_W("GetSegmentsWithTmpltDurationStatic SafeMul overflow, duration=" PUBLIC_LOG_U64
            ", reject", segTmpltInfo->multSegBaseInfo_.duration_);
        return DASH_SEGMENT_INIT_FAILED;
    }
    uint64_t segmentDuration = mulRes / timeScaleTmp;
    if (segmentDuration < MIN_SEGMENT_DURATION_NS) {
        MEDIA_LOG_W("segmentDuration " PUBLIC_LOG_U64 "ns below min, reject", segmentDuration);
        return DASH_SEGMENT_INIT_FAILED;
    }

    // Lazy loading: store template params, don't expand while loop
    desc->lazyMode_ = DASH_LAZY_MODE_DURATION;
    desc->durationSegInfo_.mediaUrlTemplate = mediaUrl;
    desc->durationSegInfo_.segDurationNs = segmentDuration;
    desc->durationSegInfo_.baseDuration = static_cast<int64_t>(segTmpltInfo->multSegBaseInfo_.duration_);
    desc->durationSegInfo_.startNumberSeq = desc->startNumberSeq_;
    desc->durationSegInfo_.totalDuration = desc->duration_;

    MEDIA_LOG_I("Duration lazy mode: segDur " PUBLIC_LOG_U64 "ns, totalDur " PUBLIC_LOG_U64
        "ns, segCount " PUBLIC_LOG_U64, segmentDuration, desc->duration_, desc->GetMediaSegmentCount());
    return DASH_SEGMENT_INIT_SUCCESS;
}

DashSegmentInitValue DashMpdDownloader::GetSegmentsWithTmpltTimelineStatic(const DashSegTmpltInfo *segTmpltInfo,
    const std::string &mediaUrl, unsigned int timeScale, std::shared_ptr<DashStreamDescription> desc)
{
    if (timeScale == 0) {
        return DASH_SEGMENT_INIT_FAILED;
    }

    desc->lazyMode_ = DASH_LAZY_MODE_TIMELINE;
    desc->durationSegInfo_.mediaUrlTemplate = mediaUrl;

    TimelineIterState state;
    std::list<DashSegTimeline*> timelineList = segTmpltInfo->multSegBaseInfo_.segTimeline_;
    for (auto it = timelineList.begin(); it != timelineList.end(); ++it) {
        if (*it == nullptr) {
            continue;
        }
        if (desc->timelineEntries_.size() >= MAX_TIMELINE_ENTRIES) {
            MEDIA_LOG_W("timelineEntries count exceeds max, stop parse" PUBLIC_LOG_ZU, desc->timelineEntries_.size());
            break;
        }
        if (!ProcessTimelineEntry(it, timelineList, timeScale, desc, state)) {
            break;
        }
    }

    return DASH_SEGMENT_INIT_SUCCESS;
}

bool DashMpdDownloader::ProcessTimelineEntry(std::list<DashSegTimeline*>::iterator &it,
    const std::list<DashSegTimeline*>& timelineList, uint64_t timeScale,
    std::shared_ptr<DashStreamDescription> desc, TimelineIterState &state)
{
    int64_t segCount = GetSegCountFromTimeline(it, timelineList.end(), desc->duration_, timeScale, state.startTime);
    if (segCount < 0) {
        MEDIA_LOG_W("get segment count error in SegmentTemplate.SegmentTimeline");
        return true; // continue
    }
    uint64_t mulRes = 0;
    if (!SafeMul((*it)->d_, S_2_NS, mulRes)) {
        MEDIA_LOG_W("SafeMul overflow in SegmentTimeline d_=" PUBLIC_LOG_U64 ", skip", (*it)->d_);
        return true; // continue
    }
    uint64_t segDuration = mulRes / timeScale;
    if (state.segmentSeq == -1) {
        state.segmentSeq = desc->startNumberSeq_;
    }
    int64_t segCountPlus1 = 0;
    if (!SafeAdd<int64_t>(segCount, 1, segCountPlus1)) {
        MEDIA_LOG_W("SafeAdd(segCount,1) overflow, segCount=" PUBLIC_LOG_D64 ", skip", segCount);
        return true; // continue
    }
    desc->timelineEntries_.push_back({state.startTime, (*it)->d_, segDuration, segCountPlus1, state.segmentSeq});
    int64_t newSegSeq = 0;
    if (!SafeAdd(state.segmentSeq, segCountPlus1, newSegSeq)) {
        MEDIA_LOG_W("SafeAdd(segmentSeq,segCountPlus1) overflow, stop");
        return false; // break
    }
    state.segmentSeq = newSegSeq;
    uint64_t mul2 = 0;
    if (!SafeMul(static_cast<uint64_t>(segCountPlus1), (*it)->d_, mul2)) {
        MEDIA_LOG_W("SafeMul(segCountPlus1,d_) overflow, stop");
        return false; // break
    }
    uint64_t newStartTime = 0;
    if (!SafeAdd(state.startTime, mul2, newStartTime)) {
        MEDIA_LOG_W("SafeAdd(startTime,mul2) overflow, stop");
        return false; // break
    }
    state.startTime = newStartTime;
    return true; // continue
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
int64_t DashMpdDownloader::GetSegCountFromTimeline(DashList<DashSegTimeline *>::iterator &it,
    const DashList<DashSegTimeline *>::const_iterator &end,
    uint64_t periodDuration, unsigned int timeScale, uint64_t startTime)
{
    int64_t segCount = static_cast<int64_t>((*it)->r_);
    if (segCount >= 0 || (*it)->d_ == 0) {
        return std::min(segCount, static_cast<int64_t>(MAX_SEG_COUNT_PER_S));
    }

    // @r is negative: repeats until the start of the next S element or end of Period
    uint64_t boundary;
    auto next = std::next(it);
    if (next != end && *next != nullptr) {
        uint64_t nextStartTime = (*next)->t_;
        if (nextStartTime <= startTime) {
            MEDIA_LOG_W("r is negative and next S@t " PUBLIC_LOG_U64 " is not larger than startTime "
                PUBLIC_LOG_U64, nextStartTime, startTime);
            return segCount;
        }
        boundary = nextStartTime;
    } else {
        uint64_t mulRes = 0;
        if (!SafeMul(periodDuration, static_cast<uint64_t>(timeScale), mulRes)) {
            MEDIA_LOG_W("GetSegCountFromTimeline SafeMul overflow, periodDur=" PUBLIC_LOG_U64
                ", timeScale=" PUBLIC_LOG_U32, periodDuration, timeScale);
            return segCount;
        }
        uint64_t scaleDuration = mulRes / S_2_NS;
        if (scaleDuration <= startTime) {
            MEDIA_LOG_W("r is negative, duration " PUBLIC_LOG_U64 " is not larger than startTime "
                PUBLIC_LOG_U64 ", timeScale " PUBLIC_LOG_U32, periodDuration, startTime, timeScale);
            return segCount;
        }
        boundary = scaleDuration;
    }

    uint64_t span = boundary - startTime;
    uint64_t segCountTmp = std::min(span / (*it)->d_, static_cast<uint64_t>(std::numeric_limits<int64_t>::max()));
    int64_t subRes = 0;
    if (!SafeSub(static_cast<int64_t>(segCountTmp), static_cast<int64_t>(1), subRes)) {
        segCount = 0;
    } else {
        segCount = subRes;
    }
    if (segCount > MAX_SEG_COUNT_PER_S) {
        MEDIA_LOG_W("S@r " PUBLIC_LOG_D64 " exceeds max " PUBLIC_LOG_D64 ", clamped", segCount, MAX_SEG_COUNT_PER_S);
        segCount = MAX_SEG_COUNT_PER_S;
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
DashSegmentInitValue DashMpdDownloader::GetSegmentsWithSegList(DashSegListInfo *segListInfo,
    const std::string &baseUrl, std::shared_ptr<DashStreamDescription> streamDesc)
{
    unsigned int timescale = (segListInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ != 0)
        ? segListInfo->multSegBaseInfo_.segBaseInfo_.timeScale_ : 1;

    uint64_t mulRes = 0;
    uint64_t segDuration = 0;
    if (!SafeMul(segListInfo->multSegBaseInfo_.duration_, S_2_NS, mulRes)) {
        MEDIA_LOG_W("GetSegmentsWithSegList SafeMul overflow, duration=" PUBLIC_LOG_U64
            ", using 0", segListInfo->multSegBaseInfo_.duration_);
    } else {
        segDuration = mulRes / timescale;
    }
    DashList<uint64_t> durationList;
    GetSegDurationFromTimeline(streamDesc->duration_, timescale, &segListInfo->multSegBaseInfo_, durationList);

    // Lazy loading: store compact entries instead of expanding into DashSegment objects.
    streamDesc->lazyMode_ = DASH_LAZY_MODE_SEGLIST;
    streamDesc->segListEntries_.reserve(segListInfo->segmentUrl_.size());
    int segmentCount = 0;
    for (auto it = segListInfo->segmentUrl_.begin(); it != segListInfo->segmentUrl_.end(); ++it) {
        uint64_t durationNs = segDuration;
        if (!durationList.empty()) {
            durationNs = durationList.front();
            if (durationList.size() > 1) {
                durationList.pop_front();
            }
        }
        if (baseUrl.empty() && it->media_.empty()) {
            continue; // skip empty segment
        }
        DashSegListEntry entry;
        entry.durationNs = durationNs;
        int64_t numberSeq = 0;
        if (!SafeAdd(streamDesc->startNumberSeq_, static_cast<int64_t>(segmentCount), numberSeq)) {
            MEDIA_LOG_W("GetSegmentsWithSegList SafeAdd overflow, startSeq=" PUBLIC_LOG_D64
                ", count=" PUBLIC_LOG_D32 ", stop", streamDesc->startNumberSeq_, segmentCount);
            break;
        }
        entry.numberSeq = numberSeq;
        entry.mediaUrl = it->media_;  // empty media_ means append baseUrl later
        entry.mediaRange = it->mediaRange_;
        streamDesc->segListEntries_.push_back(std::move(entry));
        segmentCount++;
    }

    // Release parser-side segmentUrl_ immediately to avoid dual-copy residency.
    // segListEntries_ now holds the only copy; segmentUrl_ won't be read again
    // (segsState_ == DASH_SEGS_STATE_FINISH guards re-entry into GetSegmentsInMpd).
    segListInfo->segmentUrl_.clear();

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
void DashMpdDownloader::GetSegDurationFromTimeline(uint64_t periodDuration, unsigned int timeScale,
    const DashMultSegBaseInfo *multSegBaseInfo, DashList<uint64_t> &durationList)
{
    if (timeScale == 0 || mpdInfo_ == nullptr) {
        return;
    }

    if (mpdInfo_->type_ == DashType::DASH_TYPE_DYNAMIC) {
        if (sourceLoader_ && sourceLoader_->GetenableOfflineCache()) {
            sourceLoader_->Close(-1);
        }
        return;
    }

    // support SegmentList.SegmentTimeline in static type
    if (multSegBaseInfo->duration_ != 0 || multSegBaseInfo->segTimeline_.empty() ||
        mpdInfo_->type_ != DashType::DASH_TYPE_STATIC) {
        return;
    }

    uint64_t startTime = 0;
    std::list<DashSegTimeline*> timelineList = multSegBaseInfo->segTimeline_;
    for (std::list<DashSegTimeline*>::iterator it = timelineList.begin(); it != timelineList.end(); ++it) {
        if (*it == nullptr) {
            continue;
        }
        int64_t segCount = GetSegCountFromTimeline(it, timelineList.end(), periodDuration, timeScale, startTime);
        if (segCount < 0) {
            MEDIA_LOG_W("get segment count error in SegmentList.SegmentTimeline");
            continue;
        }
        ExpandSegDuration(*it, segCount, timeScale, startTime, durationList);
    }
}

/**
 * @brief    Expand Segment Duration From SegmentTimeline
 *
 * @param    timeline          the segment timeline entry
 * @param    segCount          the repeat count of this timeline entry
 * @param    timeScale        the timeScale of this period, duration / timescale = seconds
 * @param    startTime[in,out] the start time of this timeline entry
 * @param    durationList[out] the list to store segment duration
 */
void DashMpdDownloader::ExpandSegDuration(const DashSegTimeline *timeline, int64_t segCount, unsigned int timeScale,
                                          uint64_t &startTime, DashList<uint64_t> &durationList)
{
    FALSE_RETURN_MSG(timeline != nullptr && timeScale != 0, "invalid param");
    uint64_t mulRes = 0;
    if (!SafeMul(timeline->d_, S_2_NS, mulRes)) {
        MEDIA_LOG_W("ExpandSegDuration SafeMul overflow, d_=" PUBLIC_LOG_U64 ", skip", timeline->d_);
        return;
    }
    uint64_t segDuration = mulRes / timeScale;
    for (int64_t repeat = 0; repeat <= segCount; repeat++) {
        durationList.push_back(segDuration);
        uint64_t newStartTime = 0;
        if (!SafeAdd(startTime, timeline->d_, newStartTime)) {
            MEDIA_LOG_W("ExpandSegDuration SafeAdd overflow, startTime=" PUBLIC_LOG_U64
                ", d_=" PUBLIC_LOG_U64 ", stop", startTime, timeline->d_);
            return;
        }
        startTime = newStartTime;
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

    if (AddOneSegment(srcSegment, streamDesc) == DASH_SEGMENT_INIT_FAILED) {
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
    if (adptSetInfo->adptSetSegTmplt_ != nullptr && !adptSetInfo->adptSetSegTmplt_->segTmpltMedia_.empty()) {
        DashAppendBaseUrl(baseUrl, adptSetInfo->baseUrl_);
        std::string repId = (repInfo != nullptr) ? repInfo->id_ : "";
        return GetSegmentsWithSegTemplate(adptSetInfo->adptSetSegTmplt_, repId, streamDesc);
    }
    if (adptSetInfo->adptSetSegList_ != nullptr && !adptSetInfo->adptSetSegList_->segmentUrl_.empty()) {
        DashAppendBaseUrl(baseUrl, adptSetInfo->baseUrl_);
        return GetSegmentsWithSegList(adptSetInfo->adptSetSegList_, baseUrl, streamDesc);
    }
    if (!adptSetInfo->baseUrl_.empty()) {
        return GetSegmentsWithBaseUrl(adptSetInfo->baseUrl_, streamDesc);
    }
    return DASH_SEGMENT_INIT_UNDO;
}

bool DashMpdDownloader::GetInitSegFromPeriod(const std::string &periodBaseUrl, const std::string &repId,
    std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    DashUrlType *initSegment = periodManager_->GetInitSegment(segTmpltFlag);
    FALSE_RETURN_V_MSG(initSegment != nullptr, false, "no init segment in Period");
    streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
    UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
    MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, periodBaseUrl);
    MEDIA_LOG_D("GetInitSegFromPeriod:streamId:" PUBLIC_LOG_D32, streamDesc->streamId_);
    return true;
}

bool DashMpdDownloader::GetInitSegFromAdptSet(const std::string &adptSetBaseUrl, const std::string &repId,
    std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    DashUrlType *initSegment = adptSetManager_->GetInitSegment(segTmpltFlag);
    FALSE_RETURN_V_MSG(initSegment != nullptr, false, "no init segment in AdptSet");
    streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
    UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
    MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, adptSetBaseUrl);
    MEDIA_LOG_D("GetInitSegFromAdptSet:streamId:" PUBLIC_LOG_D32, streamDesc->streamId_);
    return true;
}

bool DashMpdDownloader::GetInitSegFromRepresentation(const std::string &repBaseUrl, const std::string &repId,
    std::shared_ptr<DashStreamDescription> streamDesc)
{
    int segTmpltFlag = 0;
    DashUrlType *initSegment = representationManager_->GetInitSegment(segTmpltFlag);
    FALSE_RETURN_V_MSG(initSegment != nullptr, false, "no init segment in Representation");
    streamDesc->initSegment_ = std::make_shared<DashInitSegment>();
    UpdateInitSegUrl(streamDesc, initSegment, segTmpltFlag, repId);
    MakeAbsoluteWithBaseUrl(streamDesc->initSegment_, repBaseUrl);
    MEDIA_LOG_D("GetInitSegFromRepresentation:streamId:" PUBLIC_LOG_D32, streamDesc->streamId_);
    return true;
}

DashMpdGetRet DashMpdDownloader::GetSegmentsInNewStream(std::shared_ptr<DashStreamDescription> destStream)
{
    MEDIA_LOG_I("GetSegmentsInNewStream update id:" PUBLIC_LOG_D32 ", seq:"
        PUBLIC_LOG_D64, destStream->streamId_, destStream->currentNumberSeq_);

    if (destStream->segsState_ == DASH_SEGS_STATE_FINISH) {
        MEDIA_LOG_I("GetNextVideoStream id:" PUBLIC_LOG_D32 ", segment list is ok", destStream->streamId_);
        return DASH_MPD_GET_DONE;
    }

    if (ondemandSegBase_) {
        if (destStream->indexSegment_ == nullptr) {
            MEDIA_LOG_E("GetNextSegmentByBitrate id:" PUBLIC_LOG_D32 " is null", destStream->streamId_);
            return DASH_MPD_GET_ERROR;
        }
        OpenStream(destStream);
        return DASH_MPD_GET_UNDONE;
    }

    if (GetSegmentsInMpd(destStream) == DASH_SEGMENT_INIT_FAILED) {
        MEDIA_LOG_E("GetNextSegmentByBitrate id:"
            PUBLIC_LOG_D32 " GetSegmentsInMpd failed", destStream->streamId_);
    }
    return DASH_MPD_GET_DONE;
}

void DashMpdDownloader::UpdateInitSegUrl(std::shared_ptr<DashStreamDescription> streamDesc, const DashUrlType *urlType,
    int segTmpltFlag, std::string representationID)
{
    FALSE_RETURN_MSG(streamDesc != nullptr && streamDesc->initSegment_ != nullptr, "initSegment_ is nullptr");
    streamDesc->initSegment_->url_ = urlType->sourceUrl_;
    if (urlType->range_.length() > 0) {
        DashParseRange(urlType->range_, streamDesc->initSegment_->rangeBegin_, streamDesc->initSegment_->rangeEnd_);
    }

    if (!segTmpltFlag) {
        return;
    }

    std::string::size_type posIden = streamDesc->initSegment_->url_.find("$$");
    if (posIden != std::string::npos) {
        size_t strLength = strlen("$$");
        streamDesc->initSegment_->url_.replace(posIden, strLength, "$");
    }

    if (DashSubstituteTmpltStr(streamDesc->initSegment_->url_, "$RepresentationID", representationID) == -1) {
        MEDIA_LOG_E("UpdateInitSegUrl subtitute $RepresentationID error " PUBLIC_LOG_S, representationID.c_str());
    }

    if (DashSubstituteTmpltStr(streamDesc->initSegment_->url_, "$Bandwidth",
                               std::to_string(streamDesc->bandwidth_)) == -1) {
        MEDIA_LOG_E("UpdateInitSegUrl subtitute $Bandwidth error " PUBLIC_LOG_U32, streamDesc->bandwidth_);
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

void DashMpdDownloader::SetSourceStatisticsDfx(
    std::shared_ptr<OHOS::MediaAVCodec::SourceStatisticsReportInfo> rpInfoPtr)
{
    reportInfo_ = rpInfoPtr;
}

void DashMpdDownloader::DfxAudioCntIncrease()
{
    FALSE_RETURN_MSG(reportInfo_ != nullptr, "reportInfo_ is nullptr");
    reportInfo_->audioStreamCnt_++;
}

void DashMpdDownloader::DfxSubtitleCntIncrease()
{
    FALSE_RETURN_MSG(reportInfo_ != nullptr, "reportInfo_ is nullptr");
    reportInfo_->subtitleCnt_++;
}

bool DashMpdDownloader::ContainsNonDigit(const std::string& str)
{
    auto it = std::find_if(str.begin(), str.end(), [](unsigned char c) {
        return !std::isdigit(c);
    });
    
    return it != str.end();
}

bool DashMpdDownloader::ParseFrameRateToUint32(const std::string& str, uint32_t& value)
{
    if (str.empty()) {
        return false;
    }

    auto slashPos = str.find('/');
    if (slashPos != std::string::npos) {
        std::string numStr = str.substr(0, slashPos);
        std::string denStr = str.substr(slashPos + 1);
        double num = 0.0;
        double den = 0.0;
        if (!DashStrToNonNegativeDouble(numStr, num) || !DashStrToNonNegativeDouble(denStr, den)) {
            return false;
        }
        if (std::abs(den) < TOLERANCE) {
            return false;
        }
        double fps = num / den;
        if (fps < 0.0) {
            return false;
        }
        value = static_cast<uint32_t>(std::ceil(fps));
        return true;
    }
    double fps = 0.0;
    if (!DashStrToNonNegativeDouble(str, fps)) {
        return false;
    }
    value = static_cast<uint32_t>(std::ceil(fps));
    return true;
}

std::string DashMpdDownloader::GetMimeType(const std::string &mimeType, StreamType type)
{
    std::string mimeTypeHead = type == VIDEO ? "video/" : "audio/";
    std::string suffix = mimeType;
    auto slashPos = suffix.rfind('/');
    if (slashPos != std::string::npos && slashPos + 1 < suffix.size()) {
        suffix = suffix.substr(slashPos + 1);
    }

    auto dotPos = suffix.find('.');
    if (dotPos != std::string::npos && dotPos > 0) {
        suffix = suffix.substr(0, dotPos);
    }

    return mimeTypeHead + suffix;
}

StreamInfo DashMpdDownloader::AssignStreamInfo(unsigned int index)
{
    StreamInfo info;
    FALSE_RETURN_V_MSG(streamDescriptions_[index] != nullptr, info, "AssignStreamInfo fail");
    info.streamId = streamDescriptions_[index]->streamId_;
    info.bitRate = streamDescriptions_[index]->bandwidth_;
    info.videoWidth = static_cast<int32_t>(streamDescriptions_[index]->width_);
    info.videoHeight = static_cast<int32_t>(streamDescriptions_[index]->height_);
    info.lang = streamDescriptions_[index]->lang_;
    info.videoType = GetVideoType(streamDescriptions_[index]->videoType_);
    info.minBitRate = streamDescriptions_[index]->minBandwidth_;
    info.maxBitRate = streamDescriptions_[index]->maxBandwidth_;
    info.minWidth = streamDescriptions_[index]->minWidth_;
    info.maxWidth = streamDescriptions_[index]->maxWidth_;
    info.minHeight = streamDescriptions_[index]->minHeight_;
    info.maxHeight = streamDescriptions_[index]->maxHeight_;
    info.channels = streamDescriptions_[index]->channels_;

    uint32_t parsedRate = 0;
    if (ParseFrameRateToUint32(streamDescriptions_[index]->frameRate_, parsedRate)) {
        info.frameRate = parsedRate;
    }
    if (ParseFrameRateToUint32(streamDescriptions_[index]->minFrameRate_, parsedRate)) {
        info.minFrameRate = parsedRate;
    }
    if (ParseFrameRateToUint32(streamDescriptions_[index]->maxFrameRate_, parsedRate)) {
        info.maxFrameRate = parsedRate;
    }
    info.mimeType = streamDescriptions_[index]->mimeType_;
    return info;
}

Status DashMpdDownloader::GetStreamInfo(std::vector<StreamInfo> &streams)
{
    MEDIA_LOG_I("GetStreamInfo");
    // only support one audio/subtitle Representation in one AdaptationSet
    std::map<std::pair<unsigned int, uint32_t>, bool> audioAdptSetSeen;
    std::map<std::pair<unsigned int, uint32_t>, bool> subtitleAdptSetSeen;
    for (unsigned int index = 0; index < streamDescriptions_.size(); index++) {
        if (streamDescriptions_[index] == nullptr) {
            continue;
        }
        if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            auto key = std::make_pair(streamDescriptions_[index]->adptSetIndex_,
                                      streamDescriptions_[index]->periodIndex_);
            if (audioAdptSetSeen[key]) {
                DfxAudioCntIncrease();
                MEDIA_LOG_D("GetStreamInfo skip audio stream:" PUBLIC_LOG_D32 ",lang:" PUBLIC_LOG_S,
                    streamDescriptions_[index]->streamId_, streamDescriptions_[index]->lang_.c_str());
                continue;
            }
            audioAdptSetSeen[key] = true;
        } else if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            auto key = std::make_pair(streamDescriptions_[index]->adptSetIndex_,
                                      streamDescriptions_[index]->periodIndex_);
            if (subtitleAdptSetSeen[key]) {
                DfxSubtitleCntIncrease();
                MEDIA_LOG_D("GetStreamInfo skip subtitle stream:" PUBLIC_LOG_D32 ",lang:" PUBLIC_LOG_S,
                    streamDescriptions_[index]->streamId_, streamDescriptions_[index]->lang_.c_str());
                continue;
            }
            subtitleAdptSetSeen[key] = true;
        }

        StreamInfo info = AssignStreamInfo(index);
        if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE) {
            info.type = SUBTITLE;
        } else if (streamDescriptions_[index]->type_ == MediaAVCodec::MediaType::MEDIA_TYPE_AUD) {
            info.type = AUDIO;
        } else {
            info.type = VIDEO;
        }
        info.codecs = GetMimeType(streamDescriptions_[index]->codecs_, info.type);
        info.originCodecs = streamDescriptions_[index]->codecs_;
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

void DashMpdDownloader::SetDownloadRequest(std::shared_ptr<DownloadRequest> downloadRequest)
{
    std::unique_lock<std::shared_mutex> lock(downloadRequestMutex_);
    downloadRequest_ = std::move(downloadRequest);
}

std::shared_ptr<DownloadRequest> DashMpdDownloader::GetDownloadRequest()
{
    std::shared_lock<std::shared_mutex> lock(downloadRequestMutex_);
    return downloadRequest_;
}

std::shared_ptr<DashStreamDescription> DashMpdDownloader::GetStreamByPeriodIndex(uint32_t periodIndex,
    MediaAVCodec::MediaType type)
{
    auto iter = std::find_if(streamDescriptions_.begin(), streamDescriptions_.end(),
        [&](const std::shared_ptr<DashStreamDescription> &stream) {
            return stream->periodIndex_ == periodIndex && stream->type_ == type;
        });
    if (iter == streamDescriptions_.end()) {
        return nullptr;
    }
    return *iter;
}

uint32_t DashMpdDownloader::GetPeriodIndexBySeekTime(int64_t seekTime) const
{
    std::shared_ptr<DashStreamDescription> currentStream;
    for (const auto &index : streamDescriptions_) {
        if (index != nullptr && index->periodIndex_ == currentPeriodIndex_.load()) {
            currentStream = index;
            break;
        }
    }
    if (currentStream == nullptr) {
        for (const auto &index : streamDescriptions_) {
            if (index != nullptr && index->segsState_ == DASH_SEGS_STATE_FINISH) {
                currentStream = index;
                break;
            }
        }
    }
    if (currentStream == nullptr) {
        return 0;
    }

    uint32_t targetPeriodIndex = 0;
    uint64_t periodStartTime = 0;
    for (const auto& streamDesc : streamDescriptions_) {
        if (streamDesc != nullptr && streamDesc->type_ == currentStream->type_ &&
            streamDesc->periodStartTime_ <= static_cast<uint64_t>(seekTime) &&
            streamDesc->periodStartTime_ >= periodStartTime) {
            targetPeriodIndex = streamDesc->periodIndex_;
            periodStartTime = streamDesc->periodStartTime_;
        }
    }
    return targetPeriodIndex;
}
}
}
}
}