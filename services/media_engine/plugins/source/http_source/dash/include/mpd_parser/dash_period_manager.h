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

#ifndef DASH_PERIOD_MANAGER_H
#define DASH_PERIOD_MANAGER_H

#include <cstdint>
#include "dash_mpd_def.h"
#include "av_common.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashPeriodManager {
public:
    DashPeriodManager() = default;
    explicit DashPeriodManager(DashPeriodInfo *period);
    ~DashPeriodManager();

    bool Empty() const;
    void Reset();
    void SetPeriodInfo(DashPeriodInfo *period);
    void GetBaseUrlList(std::list<std::string> &baseUrlList);
    void GetAdptSetsByStreamType(DashVector<DashAdptSetInfo *> &adptSetInfo, MediaAVCodec::MediaType streamType);
    DashUrlType *GetInitSegment(int32_t &flag);
    DashPeriodInfo *GetPeriod();
    DashPeriodInfo *GetPreviousPeriod();

private:
    void Init();
    void Clear();
    void ParseAdptSetList(DashList<DashAdptSetInfo *> adptSetList);
    void ParseAdptSetTypeByMime(std::string mimeType, DashAdptSetInfo *adptSetInfo);
    void ParseAdptSetTypeByComp(DashList<DashContentCompInfo *> contentCompList, DashAdptSetInfo *adptSetInfo);
    void ParseInitSegment();
    void ParseInitSegmentBySegTmplt();
    DashPeriodManager(const DashPeriodManager &period);
    DashPeriodManager &operator=(const DashPeriodManager &period);

private:
    static constexpr const char *const SUBTITLE_MIME_APPLICATION = "application";
    static constexpr const char *const SUBTITLE_MIME_TEXT = "text";
    static constexpr const char *const AUDIO_MIME_TYPE = "audio";
    static constexpr const char *const VIDEO_MIME_TYPE = "video";

    DashPeriodInfo *periodInfo_{nullptr};
    DashPeriodInfo *previousPeriodInfo_{nullptr};
    DashUrlType *initSegment_{nullptr};
    int32_t segTmpltFlag_{0};      // indicate the initialization segment is get from SegmentTemplate@Initialization
    std::string subtitleMimeType_; // mimeType of subtitle such as application/mp4 text/vtt
    DashVector<DashAdptSetInfo *> videoAdptSetsVector_;
    DashVector<DashAdptSetInfo *> audioAdptSetsVector_;
    DashVector<DashAdptSetInfo *> subtitleAdptSetsVector_;
};
} // namespace HttpPluginLite
} // namespace Plugin
} // namespace Media
} // namespace OHOS
#endif // DASH_PERIOD_MANAGER_H