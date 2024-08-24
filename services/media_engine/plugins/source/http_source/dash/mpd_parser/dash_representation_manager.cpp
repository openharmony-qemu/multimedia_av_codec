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

#include "dash_representation_manager.h"
#include "dash_manager_util.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
DashRepresentationManager::DashRepresentationManager(DashRepresentationInfo *rep)
{
    SetRepresentationInfo(rep);
}

DashRepresentationManager::~DashRepresentationManager()
{
    Clear();
}

void DashRepresentationManager::Reset()
{
    this->representationInfo_ = nullptr;
    this->previousRepresentationInfo_ = nullptr;
    Clear();
}

void DashRepresentationManager::SetRepresentationInfo(DashRepresentationInfo *rep)
{
    representationInfo_ = rep;
    // if representationInfo changed, update it
    if (previousRepresentationInfo_ == nullptr || previousRepresentationInfo_ != rep) {
        Init();
        previousRepresentationInfo_ = rep;
    }
}

void DashRepresentationManager::Init()
{
    if (this->representationInfo_ == nullptr) {
        return;
    }

    // before init, clear initSegment
    Clear();

    // ----------  init initial segment -------------------
    ParseInitSegment();
}

void DashRepresentationManager::Clear()
{
    if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }

    this->segTmpltFlag_ = 0;
}

void DashRepresentationManager::ParseInitSegment()
{
    if (representationInfo_->representationSegBase_ && representationInfo_->representationSegBase_->initialization_) {
        // first get initSegment from <Representation> <SegmentBase> <Initialization /> </SegmentBase> </Representation>
        initSegment_ = CloneUrlType(representationInfo_->representationSegBase_->initialization_);
    } else if (representationInfo_->representationSegList_ &&
               representationInfo_->representationSegList_->multSegBaseInfo_.segBaseInfo_.initialization_) {
        // Second get initSegment from <SegmentList> <Initialization sourceURL="seg-m-init.mp4"/> </SegmentList>
        initSegment_ =
            CloneUrlType(representationInfo_->representationSegList_->multSegBaseInfo_.segBaseInfo_.initialization_);
    } else if (representationInfo_->representationSegTmplt_) {
        ParseInitSegmentBySegTmplt();
    } else if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }

    // if sourceUrl is not present, use BaseURL, use BaseURL in mpd
}

void DashRepresentationManager::ParseInitSegmentBySegTmplt()
{
    if (representationInfo_->representationSegTmplt_->multSegBaseInfo_.segBaseInfo_.initialization_) {
        initSegment_ =
            CloneUrlType(representationInfo_->representationSegTmplt_->multSegBaseInfo_.segBaseInfo_.initialization_);
    } else if (representationInfo_->representationSegTmplt_->segTmpltInitialization_.length() > 0) {
        initSegment_ = new DashUrlType;
        if (initSegment_ != nullptr) {
            initSegment_->sourceUrl_ = representationInfo_->representationSegTmplt_->segTmpltInitialization_;
            this->segTmpltFlag_ = 1;
        }
    } else if (initSegment_ != nullptr) {
        delete initSegment_;
        initSegment_ = nullptr;
    }
}

DashUrlType *DashRepresentationManager::GetInitSegment(int32_t &flag)
{
    flag = this->segTmpltFlag_;
    return this->initSegment_;
}

DashRepresentationInfo *DashRepresentationManager::GetRepresentationInfo()
{
    return this->representationInfo_;
}

DashRepresentationInfo *DashRepresentationManager::GetPreviousRepresentationInfo()
{
    return this->previousRepresentationInfo_;
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS