/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    virtual  http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef MEDIA_PIPELINE_VIDEO_SINK_H
#define MEDIA_PIPELINE_VIDEO_SINK_H

#include "gmock/gmock.h"

#include "buffer/avbuffer.h"
#include "common/status.h"
#include "filter/filter.h"
#include "media_sync_manager.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class VideoSink {
public:
    MOCK_METHOD(int64_t, DoSyncWrite, (const std::shared_ptr<OHOS::Media::AVBuffer> &buffer, int64_t& actionClock), ());
    MOCK_METHOD(void, ResetSyncInfo, (), ());
    MOCK_METHOD(Status, GetLatency, (uint64_t & nanoSec), ());
    MOCK_METHOD(int64_t, CheckBufferLatenessMayWait, (const std::shared_ptr<OHOS::Media::AVBuffer> &buffer), ());
    MOCK_METHOD(void, SetSyncCenter, (std::shared_ptr<MediaSyncManager> syncCenter), ());
    MOCK_METHOD(void, SetEventReceiver, (const std::shared_ptr<EventReceiver> &receiver), ());
    MOCK_METHOD(void, SetFirstPts, (int64_t pts), ());
    MOCK_METHOD(void, SetSeekFlag, (), ());
    MOCK_METHOD(void, SetLastPts, (int64_t lastPts, int64_t renderDelay), ());
    MOCK_METHOD(Status, SetParameter, (const std::shared_ptr<Meta> &meta), ());
    MOCK_METHOD(void, UpdateTimeAnchorActually,
        (const std::shared_ptr<OHOS::Media::AVBuffer> &buffer, int64_t renderDelay), ());
    MOCK_METHOD(Status, GetLagInfo, (int32_t & lagTimes, int32_t &maxLagDuration, int32_t &avgLagDuration), ());
    MOCK_METHOD(Status, SetPerfRecEnabled, (bool isPerfRecEnabled), ());
    MOCK_METHOD(void, SetMediaMuted, (bool isMuted), ());
};
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
#endif  // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H
