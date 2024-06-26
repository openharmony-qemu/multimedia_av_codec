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

#ifndef DASH_MPD_MANAGER_H
#define DASH_MPD_MANAGER_H

#include <cstdint>
#include "dash_mpd_def.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashMpdManager {
public:
    DashMpdManager() = default;
    DashMpdManager(DashMpdInfo *mpdInfo, const std::string &mpdUrl);
    ~DashMpdManager() = default;

public:
    void Reset();
    void SetMpdInfo(DashMpdInfo *mpdInfo, const std::string &mpdUrl);
    DashMpdInfo *GetMpdInfo();
    DashPeriodInfo *GetFirstPeriod();
    DashPeriodInfo *GetNextPeriod(const DashPeriodInfo *periodInfo);
    TimeBase &GetTimeBase();

    // update mpd
    void Update(DashMpdInfo *mpdInfo);
    void SetTimeBase(time_t server, time_t client);
    void GetBaseUrlList(std::list<std::string> &baseUrlList);
    std::string GetBaseUrl();
    void GetDuration(uint32_t *duration);
    const DashList<DashPeriodInfo *> &GetPeriods() const;
    void MakeBaseUrl(std::string &mpdUrlBase, std::string &urlSchem);

private:
    DashMpdInfo *mpdInfo_{nullptr};
    std::string mpdUrl_;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_MPD_MANAGER_H