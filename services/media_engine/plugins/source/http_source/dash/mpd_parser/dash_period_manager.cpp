/*
 * Copyright (c) 2024-2024 Huawei Device Co., Ltd.
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

#include "dash_period_manager.h"
#include "dash_manager_util.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
/**
 * @brief   ContentComponent contain mime type such as audio/application
 *
 * @param   contentComp         contentcomponent list
 * @param   pattern             audio/application
 *
 * @return  bool
 */
static bool ContainType(DashList<DashContentCompInfo *> contentComp, const std::string &pattern)
{
    bool flag = false;
    for (DashList<DashContentCompInfo *>::iterator it = contentComp.begin(); it != contentComp.end(); ++it) {
        std::string type((*it)->contentType_);
        if (!type.empty() && type == pattern) {
            flag = true;
            break;
        }
    }

    return flag;
}

static void GetMimeTypeFromRepresentation(const DashAdptSetInfo *adptSetInfo, std::string &mimeType)
{
    DashList<DashRepresentationInfo *> representationList = adptSetInfo->representationList_;
    for (DashRepresentationInfo *representationInfo : representationList) {
        mimeType = representationInfo->commonAttrsAndElements_.mimeType_;
        if (!mimeType.empty()) {
            break;
        }
    }
}

DashPeriodManager::DashPeriodManager(DashPeriodInfo *period)
{
    SetPeriodInfo(period);
}

DashPeriodManager::~DashPeriodManager()
{
    Clear();
}

bool DashPeriodManager::Empty() const
{
    return (this->periodInfo_ == nullptr);
}

void DashPeriodManager::Reset()
{
    Clear();
    this->periodInfo_ = nullptr;
    this->previousPeriodInfo_ = nullptr;
}

void DashPeriodManager::Init()
{
    if (this->periodInfo_ == nullptr) {
        return;
    }

    DashList<DashAdptSetInfo *> adptSets = this->periodInfo_->adptSetList_;
    if (adptSets.size() == 0) {
        return;
    }

    // before init, clear the init segment and adaptationset vectors
    Clear();

    ParseAdptSetList(adptSets);

    ParseInitSegment();
}

void DashPeriodManager::Clear()
{
    if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }

    segTmpltFlag_ = 0;
    videoAdptSetsVector_.clear();
    audioAdptSetsVector_.clear();
    subtitleAdptSetsVector_.clear();
    subtitleMimeType_.clear();
}

void DashPeriodManager::ParseAdptSetList(DashList<DashAdptSetInfo *> adptSetList)
{
    for (DashList<DashAdptSetInfo *>::iterator it = adptSetList.begin(); it != adptSetList.end(); ++it) {
        // get the AdaptationSet type by parse attribute mimeType/contentType, if the both attributes disappear, parse
        // element ContentComponent
        std::string mimeType((*it)->commonAttrsAndElements_.mimeType_);
        std::string contentType((*it)->contentType_);

        DashList<DashContentCompInfo *> contentCompList((*it)->contentCompList_);

        // if mimeType of AdaptationSet disappear, get mimeType from AdaptationSet.Representation
        if (mimeType.empty()) {
            GetMimeTypeFromRepresentation(*it, mimeType);
        }

        // if mimeType or contentType appear, parse it
        if (!mimeType.empty()) {
            ParseAdptSetTypeByMime(mimeType, *it);
        } else if (!contentType.empty()) {
            ParseAdptSetTypeByMime(contentType, *it);
        } else if (!contentCompList.empty()) {
            // if AdaptationSet and AdaptationSet.Representation can not find mime/contentType. we parse
            // ContentComponent
            ParseAdptSetTypeByComp(contentCompList, *it);
        } else {
            // if attributes mimeType/contentType both disappear and element ContentComponent disappear, think the
            // AdaptationSet is video
            videoAdptSetsVector_.push_back(*it);
        }
    }
}

void DashPeriodManager::ParseAdptSetTypeByMime(std::string mimeType, DashAdptSetInfo *adptSetInfo)
{
    if (adptSetInfo->mimeType_.empty()) {
        adptSetInfo->mimeType_.append(mimeType);
    }

    if (mimeType.find(VIDEO_MIME_TYPE) != std::string::npos) {
        videoAdptSetsVector_.push_back(adptSetInfo);
    } else if (mimeType.find(AUDIO_MIME_TYPE) != std::string::npos) {
        audioAdptSetsVector_.push_back(adptSetInfo);
    } else if (mimeType.find(SUBTITLE_MIME_APPLICATION) != std::string::npos ||
               mimeType.find(SUBTITLE_MIME_TEXT) != std::string::npos) {
        // if exist application(such as application/ttml+xml) in mimeType/contentType, we think as subtitle
        if (subtitleMimeType_.empty()) {
            // parse subtitle mime type first time, store as subtitleMimeType
            subtitleMimeType_ =
                ((mimeType.find(SUBTITLE_MIME_APPLICATION) != std::string::npos) ? SUBTITLE_MIME_APPLICATION
                                                                                 : SUBTITLE_MIME_TEXT);
        } else if (mimeType.find(subtitleMimeType_) == std::string::npos) {
            // only use one mimeType("application" or "text/"), skip another one
            return;
        }

        subtitleAdptSetsVector_.push_back(adptSetInfo);
    } else {
        videoAdptSetsVector_.push_back(adptSetInfo);
    }
}

void DashPeriodManager::ParseAdptSetTypeByComp(DashList<DashContentCompInfo *> contentCompList,
                                               DashAdptSetInfo *adptSetInfo)
{
    if (ContainType(contentCompList, VIDEO_MIME_TYPE)) {
        videoAdptSetsVector_.push_back(adptSetInfo);
    } else if (ContainType(contentCompList, AUDIO_MIME_TYPE)) {
        audioAdptSetsVector_.push_back(adptSetInfo);
    } else if (ContainType(contentCompList, SUBTITLE_MIME_APPLICATION) ||
               ContainType(contentCompList, SUBTITLE_MIME_TEXT)) {
        if (subtitleMimeType_.empty()) {
            // parse subtitle mime type first time, store as subtitleMimeType
            subtitleMimeType_ = ((ContainType(contentCompList, SUBTITLE_MIME_APPLICATION)) ? SUBTITLE_MIME_APPLICATION
                                                                                           : SUBTITLE_MIME_TEXT);
        } else if (!ContainType(contentCompList, subtitleMimeType_)) {
            // only use one mimeType("application" or "text/"), skip another one
            return;
        }
        subtitleAdptSetsVector_.push_back(adptSetInfo);
    } else {
        videoAdptSetsVector_.push_back(adptSetInfo);
    }
}

void DashPeriodManager::ParseInitSegment()
{
    // ----------  init initial segment -------------------
    if (periodInfo_->periodSegBase_ && periodInfo_->periodSegBase_->initialization_) {
        // first get initSegment from <Period> <SegmentBase> <Initialization /> </SegmentBase>  </Period>
        initSegment_ = CloneUrlType(periodInfo_->periodSegBase_->initialization_);
    } else if (periodInfo_->periodSegList_ &&
               periodInfo_->periodSegList_->multSegBaseInfo_.segBaseInfo_.initialization_) {
        // second get initSegment from <SegmentList> <Initialization sourceURL="seg-m-init.mp4"/> </SegmentList>
        initSegment_ = CloneUrlType(periodInfo_->periodSegList_->multSegBaseInfo_.segBaseInfo_.initialization_);
    } else if (periodInfo_->periodSegTmplt_) {
        ParseInitSegmentBySegTmplt();
    } else if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }

    // if sourceUrl is not present, use BaseURL in mpd
}

void DashPeriodManager::ParseInitSegmentBySegTmplt()
{
    if (periodInfo_->periodSegTmplt_->multSegBaseInfo_.segBaseInfo_.initialization_) {
        initSegment_ = CloneUrlType(periodInfo_->periodSegTmplt_->multSegBaseInfo_.segBaseInfo_.initialization_);
    } else if (periodInfo_->periodSegTmplt_->segTmpltInitialization_.length() > 0) {
        initSegment_ = new DashUrlType;
        if (initSegment_ != nullptr) {
            initSegment_->sourceUrl_ = periodInfo_->periodSegTmplt_->segTmpltInitialization_;
            segTmpltFlag_ = 1;
        }
    } else if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }
}

void DashPeriodManager::SetPeriodInfo(DashPeriodInfo *period)
{
    periodInfo_ = period;

    // if periodInfo changed, update it
    if (previousPeriodInfo_ == nullptr || previousPeriodInfo_ != period) {
        Init();
        previousPeriodInfo_ = period;
    }
}

void DashPeriodManager::GetBaseUrlList(std::list<std::string> &baseUrlList)
{
    if (this->periodInfo_ != nullptr) {
        baseUrlList = this->periodInfo_->baseUrl_;
    }
}

void DashPeriodManager::GetAdptSetsByStreamType(DashVector<DashAdptSetInfo *> &adptSetInfo,
                                                MediaAVCodec::MediaType streamType)
{
    switch (streamType) {
        case MediaAVCodec::MediaType::MEDIA_TYPE_VID:
            adptSetInfo = videoAdptSetsVector_;
            break;
        case MediaAVCodec::MediaType::MEDIA_TYPE_AUD:
            adptSetInfo = audioAdptSetsVector_;
            break;
        case MediaAVCodec::MediaType::MEDIA_TYPE_SUBTITLE:
            adptSetInfo = subtitleAdptSetsVector_;
            break;
        default:
            adptSetInfo = videoAdptSetsVector_;
            break;
    }
}

DashUrlType *DashPeriodManager::GetInitSegment(int32_t &flag)
{
    flag = this->segTmpltFlag_;
    return this->initSegment_;
}

DashPeriodInfo *DashPeriodManager::GetPeriod()
{
    return this->periodInfo_;
}

DashPeriodInfo *DashPeriodManager::GetPreviousPeriod()
{
    return this->previousPeriodInfo_;
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS