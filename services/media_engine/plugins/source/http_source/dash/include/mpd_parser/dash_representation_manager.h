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

#ifndef DASH_REPRESENTATION_MANAGER_H
#define DASH_REPRESENTATION_MANAGER_H

#include <cstdint>
#include "dash_mpd_def.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashRepresentationManager {
public:
    explicit DashRepresentationManager(DashRepresentationInfo *representation);
    DashRepresentationManager() = default;
    ~DashRepresentationManager();

    void Reset();
    void SetRepresentationInfo(DashRepresentationInfo *representation);
    DashUrlType *GetInitSegment(int32_t &segTmpltFlag);
    DashRepresentationInfo *GetRepresentationInfo();
    DashRepresentationInfo *GetPreviousRepresentationInfo();

private:
    void Init();
    void Clear();
    void ParseInitSegment();
    void ParseInitSegmentBySegTmplt();

    DashRepresentationManager(const DashRepresentationManager &representationManager);
    DashRepresentationManager &operator=(const DashRepresentationManager &representationManager);

    DashUrlType *initSegment_{nullptr};
    DashRepresentationInfo *representationInfo_{nullptr};
    DashRepresentationInfo *previousRepresentationInfo_{nullptr};
    int32_t segTmpltFlag_{0}; // indicate the initialization segment is get from SegmentTemplate@Initialization
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_REPRESENTATION_MANAGER_H