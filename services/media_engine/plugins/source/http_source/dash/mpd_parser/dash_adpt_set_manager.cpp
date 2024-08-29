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

#include <cstdint>
#include "dash_adpt_set_manager.h"
#include "dash_manager_util.h"
#include "dash_mpd_util.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
static bool SortByBitrate(const DashRepresentationInfo *x, const DashRepresentationInfo *y)
{
    return x->bandwidth_ < y->bandwidth_;
}

DashAdptSetManager::DashAdptSetManager(DashAdptSetInfo *adptSet)
{
    SetAdptSetInfo(adptSet);
}

DashAdptSetManager::~DashAdptSetManager()
{
    Clear();
}

/**
 * @brief   Get initialization segment and representation list
 *
 * @param   void
 *
 * @return  none
 */
void DashAdptSetManager::Init()
{
    if (this->adptSetInfo_ == nullptr) {
        return;
    }

    // clear init segment
    Clear();

    // ----------  init initial segment -------------------
    ParseInitSegment();

    this->representationList_ = this->adptSetInfo_->representationList_;
    if (this->representationList_.size() == 0) {
        return;
    }

    this->representationList_.sort(SortByBitrate);
}

void DashAdptSetManager::Clear()
{
    if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }

    segTmpltFlag_ = 0;
}

void DashAdptSetManager::Reset()
{
    Clear();
    adptSetInfo_ = nullptr;
    previousAdptSetInfo_ = nullptr;
    this->representationList_.clear();
}

void DashAdptSetManager::ParseInitSegment()
{
    if (adptSetInfo_->adptSetSegBase_ && adptSetInfo_->adptSetSegBase_->initialization_) {
        // if exist <AdaptationSet> <SegmentBase> <Initialization /> </SegmentBase>  </AdaptationSet>
        initSegment_ = CloneUrlType(adptSetInfo_->adptSetSegBase_->initialization_);
    } else if (adptSetInfo_->adptSetSegList_ &&
               adptSetInfo_->adptSetSegList_->multSegBaseInfo_.segBaseInfo_.initialization_) {
        // or exist <SegmentList> <Initialization sourceURL="seg-m-init.mp4"/> </SegmentList>
        initSegment_ = CloneUrlType(adptSetInfo_->adptSetSegList_->multSegBaseInfo_.segBaseInfo_.initialization_);
    } else if (adptSetInfo_->adptSetSegTmplt_) {
        ParseInitSegmentBySegTmplt();
    } else if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }

    // if sourceUrl is not present, use BaseURL, use BaseURL in mpd
}

void DashAdptSetManager::ParseInitSegmentBySegTmplt()
{
    if (adptSetInfo_->adptSetSegTmplt_->multSegBaseInfo_.segBaseInfo_.initialization_) {
        initSegment_ =
            CloneUrlType(adptSetInfo_->adptSetSegTmplt_->multSegBaseInfo_.segBaseInfo_.initialization_);
    } else if (adptSetInfo_->adptSetSegTmplt_->segTmpltInitialization_.length() > 0) {
        initSegment_ = new DashUrlType;
        if (initSegment_ != nullptr) {
            initSegment_->sourceUrl_ = adptSetInfo_->adptSetSegTmplt_->segTmpltInitialization_;
            segTmpltFlag_ = 1;
        }
    } else if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }
}

void DashAdptSetManager::SetAdptSetInfo(DashAdptSetInfo *adptSet)
{
    adptSetInfo_ = adptSet;

    // adaptationset changed
    if (previousAdptSetInfo_ == nullptr || previousAdptSetInfo_ != adptSet) {
        Init();
        previousAdptSetInfo_ = adptSet;
    }
}

void DashAdptSetManager::GetBaseUrlList(std::list<std::string> &baseUrlList)
{
    if (this->adptSetInfo_ != nullptr) {
        baseUrlList = this->adptSetInfo_->baseUrl_;
    }
}

void DashAdptSetManager::GetBandwidths(DashVector<uint32_t> &bandwidths)
{
    if (representationList_.size() == 0) {
        return;
    }

    for (DashList<DashRepresentationInfo *>::iterator curRep = representationList_.begin();
         curRep != representationList_.end(); ++curRep) {
        if (*curRep != nullptr) {
            bandwidths.push_back((*curRep)->bandwidth_);
        }
    }
}

DashUrlType *DashAdptSetManager::GetInitSegment(int32_t &flag)
{
    flag = this->segTmpltFlag_;
    return this->initSegment_;
}

DashAdptSetInfo *DashAdptSetManager::GetAdptSetInfo()
{
    return this->adptSetInfo_;
}

DashAdptSetInfo *DashAdptSetManager::GetPreviousAdptSetInfo()
{
    return this->previousAdptSetInfo_;
}

DashRepresentationInfo *DashAdptSetManager::GetRepresentationByBandwidth(uint32_t bandwidth)
{
    if (this->representationList_.size() == 0) {
        return nullptr;
    }

    DashList<DashRepresentationInfo *>::iterator it = this->representationList_.begin();
    for (; it != this->representationList_.end(); ++it) {
        if ((*it)->bandwidth_ == bandwidth) {
            return (*it);
        }
    }

    return nullptr;
}

DashRepresentationInfo *DashAdptSetManager::GetHighRepresentation()
{
    if (this->representationList_.size() == 0) {
        return nullptr;
    }

    DashList<DashRepresentationInfo *>::iterator it = this->representationList_.end();
    return *(--it); // list sort up
}

DashRepresentationInfo *DashAdptSetManager::GetLowRepresentation()
{
    if (this->representationList_.size() == 0) {
        return nullptr;
    }

    DashList<DashRepresentationInfo *>::iterator it = this->representationList_.begin();
    return (*it); // list sort up
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

std::string DashAdptSetManager::GetMime() const
{
    std::string mimeInfo;
    if (this->adptSetInfo_ == nullptr) {
        return mimeInfo;
    }

    if (!this->adptSetInfo_->commonAttrsAndElements_.mimeType_.empty()) {
        mimeInfo = this->adptSetInfo_->commonAttrsAndElements_.mimeType_;
    } else {
        // get mime from adaptationset
        GetMimeTypeFromRepresentation(this->adptSetInfo_, mimeInfo);

        if (mimeInfo.empty() && !this->adptSetInfo_->contentType_.empty()) {
            mimeInfo = this->adptSetInfo_->contentType_;
        }
    }

    return mimeInfo;
}

bool DashAdptSetManager::IsOnDemand()
{
    if (adptSetInfo_ == nullptr) {
        return false;
    }

    if (adptSetInfo_->representationList_.size() == 0) {
        if (adptSetInfo_->adptSetSegBase_ != nullptr && adptSetInfo_->adptSetSegBase_->indexRange_.size() > 0) {
            return true;
        }
    } else {
        for (auto representation : adptSetInfo_->representationList_) {
            if (representation->representationSegList_ != nullptr) {
                return false;
            }

            if (representation->representationSegTmplt_ != nullptr) {
                return false;
            }

            if (representation->representationSegBase_ != nullptr &&
                representation->representationSegBase_->indexRange_.size() > 0) {
                return true;
            }
        }
    }

    return false;
}

bool DashAdptSetManager::IsHdr()
{
    if (adptSetInfo_ == nullptr ||
        adptSetInfo_->commonAttrsAndElements_.essentialPropertyList_.size() == 0) {
        return false;
    }
    
    return DashStreamIsHdr(adptSetInfo_->commonAttrsAndElements_.essentialPropertyList_);
}

} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS