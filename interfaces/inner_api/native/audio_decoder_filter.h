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

#ifndef FILTERS_CODEC_FILTER_H
#define FILTERS_CODEC_FILTER_H

#include <cstring>
#include "media_codec/media_codec.h"
#include "filter/filter.h"
#include "plugin/plugin_time.h"
#include "foundation/multimedia/drm_framework/services/drm_service/ipc/i_keysession_service.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
using namespace OHOS::Media::Plugins;
class AudioDecoderFilter : public Filter, public std::enable_shared_from_this<AudioDecoderFilter> {
public:
    explicit AudioDecoderFilter(std::string name, FilterType type);
    ~AudioDecoderFilter() override;

    void Init(const std::shared_ptr<EventReceiver> &receiver, const std::shared_ptr<FilterCallback> &callback) override;

    Status PrepareFrame(bool renderFirstFrame) override;

    Status DoPrepare() override;

    Status DoStart() override;

    Status DoPause() override;

    Status DoResume() override;

    Status DoStop() override;

    Status DoFlush() override;

    Status DoRelease() override;

    void SetParameter(const std::shared_ptr<Meta> &parameter) override;

    void GetParameter(std::shared_ptr<Meta> &parameter) override;

    Status LinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    Status UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    Status UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) override;

    Status ChangePlugin(std::shared_ptr<Meta> meta);

    FilterType GetFilterType();

    void OnLinkedResult(const sptr<AVBufferQueueProducer> &outputBufferQueue, std::shared_ptr<Meta> &meta);

    void OnUpdatedResult(std::shared_ptr<Meta> &meta);

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta);

    void OnBufferFilled(std::shared_ptr<AVBuffer> &inputBuffer);

    Status SetDecryptionConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySessionProxy,
        bool svp);

    void OnDumpInfo(int32_t fd);

protected:
    Status OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
        const std::shared_ptr<FilterLinkCallback> &callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

private:
    sptr<AVBufferQueueProducer> GetInputBufferQueue();

    std::string name_;
    FilterType filterType_;
    std::shared_ptr<Meta> meta_;
    std::shared_ptr<Filter> nextFilter_;

    std::shared_ptr<EventReceiver> eventReceiver_;
    std::shared_ptr<FilterCallback> filterCallback_;

    std::shared_ptr<FilterLinkCallback> onLinkedResultCallback_;

    std::shared_ptr<MediaCodec> mediaCodec_;
    sptr<AVBufferQueueProducer> inputBufferQueueProducer_;

    bool isDrmProtected_ = false;
    sptr<DrmStandard::IMediaKeySessionService> keySessionServiceProxy_;
    bool svpFlag_ = false;

    bool refreshTotalPauseTime_{false};
    int64_t latestBufferTime_{HST_TIME_NONE};
    int64_t latestPausedTime_{HST_TIME_NONE};
    int64_t totalPausedTime_{0};
};
} // namespace Pipeline
} // namespace MEDIA
} // namespace OHOS
#endif // FILTERS_CODEC_FILTER_H

