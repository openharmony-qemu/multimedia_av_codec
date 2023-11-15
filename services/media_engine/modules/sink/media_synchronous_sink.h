/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#ifndef HISTREAMER_PIPELINE_MEDIA_SYNC_SINK_H
#define HISTREAMER_PIPELINE_MEDIA_SYNC_SINK_H
#include <functional>
#include "i_media_sync_center.h"
#include "common/status.h"
#include "buffer/avbuffer.h"
#include "meta/meta.h"
#include "osal/task/condition_variable.h"
#include "osal/task/mutex.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class MediaSynchronousSink : public IMediaSynchronizer {
public:
    explicit MediaSynchronousSink();
    ~MediaSynchronousSink();
    void WaitAllPrerolled(bool shouldWait) final;
    int8_t GetPriority() final;

    void NotifyAllPrerolled() final;

    void Init();

protected:
    virtual Status DoSyncWrite(const AVBuffer& buffer) = 0;
    virtual Status WriteToPluginRefTimeSync(const AVBuffer& buffer);
    virtual void ResetSyncInfo() = 0;

    void ResetPrerollReported();
    void UpdateMediaTimeRange(const Meta& meta);

    int8_t syncerPriority_ {IMediaSynchronizer::NONE};
    bool hasReportedPreroll_ {false};
    std::atomic<bool> waitForPrerolled_ {false};
    OHOS::Media::Mutex prerollMutex_ {};
    OHOS::Media::ConditionVariable prerollCond_ {};

    int64_t waitPrerolledTimeout_ {0};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_PIPELINE_MEDIA_SYNC_SINK_H
