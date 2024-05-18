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

#include "dash_mpd_manager.h"
#include "dash_mpd_util.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
DashMpdManager::DashMpdManager(DashMpdInfo *mpdInfomation, const std::string &mpdUrlStr)
    : mpdInfo_(nullptr), mpdUrl_(mpdUrlStr)
{
    SetMpdInfo(mpdInfomation, mpdUrlStr);
}

void DashMpdManager::Reset()
{
    this->mpdInfo_ = nullptr;
    this->mpdUrl_ = "";
}

void DashMpdManager::SetMpdInfo(DashMpdInfo *mpdInfomation, const std::string &mpdUrlStr)
{
    this->mpdInfo_ = mpdInfomation;
    this->mpdUrl_ = mpdUrlStr;
}

const DashList<DashPeriodInfo *> &DashMpdManager::GetPeriods() const
{
    return this->mpdInfo_->periodInfoList_;
}

DashPeriodInfo *DashMpdManager::GetFirstPeriod()
{
    if (this->mpdInfo_ == nullptr || this->mpdInfo_->periodInfoList_.size() == 0) {
        return nullptr;
    }

    return *(this->mpdInfo_->periodInfoList_.begin());
}

DashPeriodInfo *DashMpdManager::GetNextPeriod(const DashPeriodInfo *period)
{
    if (this->mpdInfo_ == nullptr || this->mpdInfo_->periodInfoList_.size() == 0) {
        return nullptr;
    }

    DashList<DashPeriodInfo *> periods = this->mpdInfo_->periodInfoList_;
    for (DashList<DashPeriodInfo *>::iterator it = periods.begin(); it != periods.end(); ++it) {
        if ((*it) == period) {
            ++it;
            return (it != periods.end() ? *it : nullptr);
        }
    }

    return nullptr;
}

void DashMpdManager::GetBaseUrlList(std::list<std::string> &baseUrlList)
{
    // if no BaseUrl in MPD Element, get base url from mpd url
    if (this->mpdInfo_ == nullptr) {
        return;
    }

    if (this->mpdInfo_->baseUrl_.empty() && !this->mpdUrl_.empty()) {
        std::string mpdUrlBase = this->mpdUrl_;
        size_t sepCharIndex = mpdUrlBase.find('?');
        if (sepCharIndex != std::string::npos) {
            mpdUrlBase = mpdUrlBase.substr(0, sepCharIndex);
        }

        if (mpdUrlBase.find('/') != std::string::npos) {
            size_t end = mpdUrlBase.find_last_of('/');
            std::string newBaseUrl(mpdUrlBase.substr(0, end + 1));
            this->mpdInfo_->baseUrl_.push_back(newBaseUrl);
        }
    }

    baseUrlList = this->mpdInfo_->baseUrl_;
}

std::string DashMpdManager::GetBaseUrl()
{
    std::string mpdUrlBase = this->mpdUrl_;
    size_t sepCharIndex = mpdUrlBase.find('?');
    if (sepCharIndex != std::string::npos) {
        mpdUrlBase = mpdUrlBase.substr(0, sepCharIndex);
    }

    std::string urlSchem;
    size_t schemIndex = mpdUrlBase.find("://");
    if (schemIndex != std::string::npos) {
        size_t schemLength = strlen("://");
        urlSchem = mpdUrlBase.substr(0, schemIndex + schemLength);
        mpdUrlBase = mpdUrlBase.substr(schemIndex + schemLength);
    }

    if (mpdUrlBase.find('/') != std::string::npos) {
        size_t end = mpdUrlBase.find_last_of('/');
        mpdUrlBase = mpdUrlBase.substr(0, end + 1);
    }

    MakeBaseUrl(mpdUrlBase, urlSchem);
    return urlSchem.append(mpdUrlBase);
}

void DashMpdManager::MakeBaseUrl(std::string &mpdUrlBase, std::string &urlSchem)
{
    if (mpdInfo_->baseUrl_.empty()) {
        return;
    }

    std::string baseUrl = mpdInfo_->baseUrl_.front();
    if (DashUrlIsAbsolute(baseUrl)) {
        urlSchem.clear();
        mpdUrlBase = baseUrl;
    } else {
        if (baseUrl.find('/') == 0) {
            // absolute path, strip path in mpdUrl
            size_t beginPathIndex = mpdUrlBase.find('/');
            if (beginPathIndex != std::string::npos) {
                mpdUrlBase = mpdUrlBase.substr(0, beginPathIndex);
            }
        }

        mpdUrlBase.append(baseUrl);
    }
}

DashMpdInfo *DashMpdManager::GetMpdInfo()
{
    return this->mpdInfo_;
}

void DashMpdManager::Update(DashMpdInfo *newMpdInfo)
{
    SetMpdInfo(newMpdInfo, this->mpdUrl_);
}

void DashMpdManager::GetDuration(uint32_t *duration)
{
    if (duration == nullptr || this->mpdInfo_ == nullptr) {
        return;
    }

    *duration = 0;

    uint32_t dur = this->mpdInfo_->mediaPresentationDuration_;
    if (dur > 0) {
        *duration = dur;
        return;
    }

    // get duration from Period
    for (DashList<DashPeriodInfo *>::iterator it = this->mpdInfo_->periodInfoList_.begin();
         it != this->mpdInfo_->periodInfoList_.end(); ++it) {
        dur += (*it)->duration_;
    }

    // count duration by segments not implement
    *duration = dur;
    return;
}
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS