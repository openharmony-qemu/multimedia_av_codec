/*
 * Copyright (c) 2023-2023 Huawei Device Co., Ltd.
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
#include "media_sync_manager.h"
#include "common/status.h"
#include "buffer/avbuffer.h"
#include "meta/meta.h"
#include "osal/task/condition_variable.h"
#include "osal/task/mutex.h"

#define BUFFER_FLAG_EOS 0x00000001

namespace OHOS {
namespace Media {
namespace Pipeline {
class MediaSynchronousSink : public IMediaSynchronizer {
public:
    MediaSynchronousSink() {};
    ~MediaSynchronousSink();
    void WaitAllPrerolled(bool shouldWait) final;
    int8_t GetPriority() final;

    void NotifyAllPrerolled() final;

protected:
    virtual int64_t DoSyncWrite(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer) = 0;
    virtual void ResetSyncInfo() = 0;

    void Init();
    void WriteToPluginRefTimeSync(const std::shared_ptr<OHOS::Media::AVBuffer>& buffer);
    void ResetPrerollReported();
    void UpdateMediaTimeRange(const std::shared_ptr<Meta>& meta);

    int8_t syncerPriority_ {IMediaSynchronizer::NONE};
    bool hasReportedPreroll_ {false};
    std::atomic<bool> waitForPrerolled_ {false};
    OHOS::Media::Mutex prerollMutex_ {};
    OHOS::Media::ConditionVariable prerollCond_ {};
    std::weak_ptr<IMediaSyncCenter> syncCenter_;

    int64_t waitPrerolledTimeout_ {0};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS

#endif // HISTREAMER_PIPELINE_MEDIA_SYNC_SINK_H
