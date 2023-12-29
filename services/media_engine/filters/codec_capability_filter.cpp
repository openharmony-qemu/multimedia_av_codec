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

#include "common/log.h"
#include "codec_capability_filter.h"
#include "filter/filter_factory.h"


namespace OHOS {
namespace Media {
namespace Pipeline {
static AutoRegisterFilter<CodecCapabilityFilter> g_registerCodecCapabilityFilter("builtin.recorder.codeccapability", FilterType::FILTERTYPE_CODEC, 
    [](const std::string& name, const FilterType type) {return std::make_shared<CodecCapabilityFilter>(name, FilterType::FILTERTYPE_CODEC); });

#define HST_TIME_NONE ((int64_t)-1)
/// End of Stream Buffer Flag
// constexpr uint32_t BUFFER_FLAG_EOS = 0x00000001;

CodecCapabilityFilter::CodecCapabilityFilter(std::string name, FilterType type): Filter(name, type) {}

CodecCapabilityFilter::~CodecCapabilityFilter() {
    if (codeclist_) {
        codeclist_ = nullptr;
    }
}

void CodecCapabilityFilter::Init(const std::shared_ptr<EventReceiver>& receiver, const std::shared_ptr<FilterCallback>& callback) {
    receiver_ = receiver;
    callback_ = callback;
    state_ = FilterState::INITIALIZED;
    codeclist_ = MediaAVCodec::AVCodecListFactory::CreateAVCodecList();
    MEDIA_LOG_I("CodecCapabilityFilter Init end");
}

Status CodecCapabilityFilter::GetAvailableEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo) {
    GetAudioEncoder(encoderInfo);
    GetVideoEncoder(encoderInfo);
    return Status::OK;
}

Status CodecCapabilityFilter::GetAudioEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo) {
    MediaAVCodec::CapabilityData *capabilityData = codeclist_->GetCapability(std::string(MediaAVCodec::CodecMimeType::AUDIO_AAC),
        true, MediaAVCodec::AVCodecCategory::AVCODEC_SOFTWARE);
    if (capabilityData != nullptr) {
        encoderInfo.push_back(capabilityData);
    }
    return Status::OK;
}

Status CodecCapabilityFilter::GetVideoEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo) {
    (void)encoderInfo;
    MediaAVCodec::CapabilityData *capabilityDataAVC = codeclist_->GetCapability(std::string(MediaAVCodec::CodecMimeType::VIDEO_AVC),
        true, MediaAVCodec::AVCodecCategory::AVCODEC_HARDWARE);
    if (capabilityDataAVC != nullptr) {
        encoderInfo.push_back(capabilityDataAVC);
    }

    MediaAVCodec::CapabilityData *capabilityDataHEVC = codeclist_->GetCapability(std::string(MediaAVCodec::CodecMimeType::VIDEO_HEVC),
        true, MediaAVCodec::AVCodecCategory::AVCODEC_HARDWARE);
    if (capabilityDataHEVC != nullptr) {
        encoderInfo.push_back(capabilityDataHEVC);
    }
    return Status::OK;
}

Status CodecCapabilityFilter::Prepare() {
    // callback_->OnCallback(shared_from_this(), FilterCallBackCommand::NEXT_FILTER_NEEDED, StreamType::STREAMTYPE_RAW_AUDIO);
    return Status::OK;
}

Status CodecCapabilityFilter::Start() {
    MEDIA_LOG_I("CodecCapabilityFilter Start.");

    return Status::OK;
}

Status CodecCapabilityFilter::Pause() {
    MEDIA_LOG_I("CodecCapabilityFilter Pause.");
    return Status::OK;
}

Status CodecCapabilityFilter::Resume() {
    MEDIA_LOG_I("CodecCapabilityFilter Resume.");
    return Status::OK;
}

Status CodecCapabilityFilter::Stop() {
    MEDIA_LOG_I("CodecCapabilityFilter Stop.");
    return Status::OK;
}

Status CodecCapabilityFilter::Flush() {
    MEDIA_LOG_I("CodecCapabilityFilter Flush.");
    return Status::OK;
}

Status CodecCapabilityFilter::Release() {
    MEDIA_LOG_I("CodecCapabilityFilter Release.");
    return Status::OK;
}

void CodecCapabilityFilter::SetParameter(const std::shared_ptr<Meta>& meta) {
    (void)meta
}

void CodecCapabilityFilter::GetParameter(std::shared_ptr<Meta>& meta) {
    (void)meta
}

Status CodecCapabilityFilter::LinkNext(const std::shared_ptr<Filter>& nextFilter, StreamType outType) {
    MEDIA_LOG_I("CodecCapabilityFilter LinkNext.");
    return Status::OK;
}

Status CodecCapabilityFilter::UpdateNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    MEDIA_LOG_I("CodecCapabilityFilter UpdateNext.");
    return Status::OK;
}

Status CodecCapabilityFilter::UnLinkNext(const std::shared_ptr<Filter> &nextFilter, StreamType outType) {
    MEDIA_LOG_I("CodecCapabilityFilter UnLinkNext.");
    return Status::OK;
}

Status CodecCapabilityFilter::OnLinked(StreamType inType, const std::shared_ptr<Meta> &meta,
                                            const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("CodecCapabilityFilter OnLinked.");
    return Status::OK;
}

Status CodecCapabilityFilter::OnUpdated(StreamType inType, const std::shared_ptr<Meta> &meta,
                                             const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("CodecCapabilityFilter OnUpdate.");
    return Status::OK;
}

Status CodecCapabilityFilter::OnUnLinked(StreamType inType, const std::shared_ptr<FilterLinkCallback> &callback) {
    MEDIA_LOG_I("CodecCapabilityFilter OnUnLinked.");
    return Status::OK;
}

} // namespace Pipeline
} // namespace Media
} // namespace OHOS