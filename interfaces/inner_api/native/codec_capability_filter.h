/*
 * Copyright (c) 2021-2021 Huawei Device Co., Ltd.
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
#ifndef MEDIA_PIPELINE_CODEC_CAPABILITY_FILTER_H
#define MEDIA_PIPELINE_CODEC_CAPABILITY_FILTER_H

#include "filter/filter.h"
#include "common/status.h"
#include "osal/task/task.h"
#include "avcodec_list.h"

#define TIME_NONE ((int64_t) -1)

namespace OHOS {
namespace Media {
namespace Pipeline {
class CodecCapabilityFilter : public Filter, public std::enable_shared_from_this<CodecCapabilityFilter> {
public:
    explicit CodecCapabilityFilter(std::string name, FilterType type);
    ~CodecCapabilityFilter() override;

    void Init(const std::shared_ptr<EventReceiver>& receiver,
        const std::shared_ptr<FilterCallback>& callback) override;

    Status Prepare() override;
    
    Status Start() override;

    Status Pause() override;
    
    Status Resume() override;

    Status Stop() override;
    
    Status Flush() override;
    
    Status Release() override;

    void SetParameter(const std::shared_ptr<Meta>& meta) override;
    
    void GetParameter(std::shared_ptr<Meta>& meta) override;
    
    Status LinkNext(const std::shared_ptr<Filter>& nextFilter, StreamType outType) override;

    Status UpdateNext(const std::shared_ptr<Filter>& nextFilter, StreamType outType) override;

    Status UnLinkNext(const std::shared_ptr<Filter>& nextFilter, StreamType outType) override;

    Status OnLinked(StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback) override;

    Status OnUpdated(StreamType inType, const std::shared_ptr<Meta>& meta,
        const std::shared_ptr<FilterLinkCallback>& callback) override;

    Status OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback>& callback) override;

    Status GetAvailableEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo);
private:
    Status GetVideoEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo);

    Status GetAudioEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo);
    std::atomic<FilterState> state_ {FilterState::CREATED};
    std::shared_ptr<MediaAVCodec::AVCodecList> codeclist_ {nullptr};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_CODEC_CAPABILITY_FILTER_H

