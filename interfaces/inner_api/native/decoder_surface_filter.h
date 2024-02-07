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

#ifndef MEDIA_PIPELINE_VIDEO_SINK_FILTER_H
#define MEDIA_PIPELINE_VIDEO_SINK_FILTER_H

#include <atomic>
#include "plugin/plugin_time.h"
#include "surface/surface.h"
#include "osal/task/condition_variable.h"
#include "osal/task/mutex.h"
#include "osal/task/task.h"
#include "video_sink.h"
#include "sink/media_synchronous_sink.h"
#include "common/status.h"
#include "meta/meta.h"
#include "meta/format.h"
#include "filter/filter.h"
#include "media_sync_manager.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"

namespace OHOS {
namespace Media {
class VideoDecoderAdapter;
namespace Pipeline {
class DecoderSurfaceFilter : public Filter, public std::enable_shared_from_this<DecoderSurfaceFilter> {
public:
    explicit DecoderSurfaceFilter(const std::string& name, FilterType type);
    ~DecoderSurfaceFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver,
        const std::shared_ptr<FilterCallback> &callback) override;
    Status Configure(const std::shared_ptr<Meta> &parameter);
    Status Prepare() override;
    Status Start() override;
    Status Pause() override;
    Status Resume() override;
    Status Stop() override;
    Status Flush() override;
    Status Release() override;

    void SetParameter(const std::shared_ptr<Meta>& parameter) override;
    void GetParameter(std::shared_ptr<Meta>& parameter) override;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);
    void OnUpdatedResult(std::shared_ptr<Meta> &meta);

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);
    FilterType GetFilterType();
    void DrainOutputBuffer(uint32_t index, std::shared_ptr<AVBuffer> &outputBuffer);
    Status SetVideoSurface(sptr<Surface> videoSurface);

    Status SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp);

    sptr<AVBufferQueueProducer> GetInputBufferQueue();
    void SetSyncCenter(std::shared_ptr<MediaSyncManager> syncCenter);

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;
    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    std::string GetCodecName(std::string mimeType);

    std::string name_;
    FilterType filterType_;
    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;
    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;
    std::shared_ptr<VideoDecoderAdapter> videoDecoder_;
    std::shared_ptr<VideoSink> videoSink_;
    std::string codecMimeType_;
    std::shared_ptr<Meta> configureParameter_;
    std::shared_ptr<Meta> meta_;

    std::shared_ptr<Filter> nextFilter_;
    Format configFormat_;

    std::atomic<uint64_t> renderFrameCnt_{0};
    std::atomic<uint64_t> discardFrameCnt_{0};

    bool refreshTotalPauseTime_{false};
    int64_t latestBufferTime_{HST_TIME_NONE};
    int64_t latestPausedTime_{HST_TIME_NONE};
    int64_t totalPausedTime_{0};
    int64_t stopTime_{0};
    sptr<Surface> videoSurface_;
    bool isDrmProtected_ = false;
    sptr<DrmStandard::IMediaKeySessionService> keySessionServiceProxy_;
    bool svpFlag_ = false;
    std::atomic<bool> isPaused_{false};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_VIDEO_SINK_FILTER_H