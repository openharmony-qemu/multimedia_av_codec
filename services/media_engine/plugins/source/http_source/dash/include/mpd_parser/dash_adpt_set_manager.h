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

#ifndef DASH_ADAPTATION_SET_MANAGER_H
#define DASH_ADAPTATION_SET_MANAGER_H

#include "dash_mpd_def.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashAdptSetManager {
public:
    DashAdptSetManager() = default;
    explicit DashAdptSetManager(DashAdptSetInfo *adptSet);
    ~DashAdptSetManager();

    void Reset();
    void SetAdptSetInfo(DashAdptSetInfo *adptSet);
    void GetBaseUrlList(std::list<std::string> &baseUrlList);
    void GetBandwidths(DashVector<uint32_t> &bandwidths);
    DashUrlType *GetInitSegment(int32_t &flag);
    DashAdptSetInfo *GetAdptSetInfo();
    DashAdptSetInfo *GetPreviousAdptSetInfo();
    DashRepresentationInfo *GetRepresentationByBandwidth(uint32_t bandwidth);
    DashRepresentationInfo *GetHighRepresentation();
    DashRepresentationInfo *GetLowRepresentation();
    std::string GetMime() const;
    bool IsOnDemand();
    bool IsHdr();
private:
    void Init();
    void Clear();
    void ParseInitSegment();
    void ParseInitSegmentBySegTmplt();
    DashAdptSetManager(const DashAdptSetManager &adptSetManager);
    DashAdptSetManager &operator=(const DashAdptSetManager &adptSetManager);

private:
    int32_t segTmpltFlag_{0}; // indicate the initialization segment is get from SegmentTemplate@Initialization
    DashList<DashRepresentationInfo *> representationList_;
    DashUrlType *initSegment_{nullptr};
    DashAdptSetInfo *previousAdptSetInfo_{nullptr};
    DashAdptSetInfo *adptSetInfo_{nullptr};
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_ADAPTATION_SET_MANAGER_H