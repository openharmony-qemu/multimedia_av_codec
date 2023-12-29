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
#include "codec_capability_adapter.h"


namespace OHOS {
namespace Media {
namespace Pipeline {
CodecCapabilityAdapter::CodecCapabilityAdapter()
{
}

CodecCapabilityAdapter::~CodecCapabilityAdapter()
{
    if (codeclist_) {
        codeclist_ = nullptr;
    }
}

void CodecCapabilityAdapter::Init()
{
    codeclist_ = MediaAVCodec::AVCodecListFactory::CreateAVCodecList();
    MEDIA_LOG_I("CodecCapabilityAdapter Init end");
}

Status CodecCapabilityAdapter::GetAvailableEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo)
{
    GetAudioEncoder(encoderInfo);
    GetVideoEncoder(encoderInfo);
    return Status::OK;
}

Status CodecCapabilityAdapter::GetAudioEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo)
{
    MediaAVCodec::CapabilityData *capabilityData = codeclist_->GetCapability(
        std::string(MediaAVCodec::CodecMimeType::AUDIO_AAC), true, MediaAVCodec::AVCodecCategory::AVCODEC_SOFTWARE);
    if (capabilityData != nullptr) {
        encoderInfo.push_back(capabilityData);
    }
    return Status::OK;
}

Status CodecCapabilityAdapter::GetVideoEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo)
{
    MediaAVCodec::CapabilityData *capabilityDataAVC = codeclist_->GetCapability(
        std::string(MediaAVCodec::CodecMimeType::VIDEO_AVC), true, MediaAVCodec::AVCodecCategory::AVCODEC_HARDWARE);
    if (capabilityDataAVC != nullptr) {
        encoderInfo.push_back(capabilityDataAVC);
    }

    MediaAVCodec::CapabilityData *capabilityDataHEVC = codeclist_->GetCapability(
        std::string(MediaAVCodec::CodecMimeType::VIDEO_HEVC), true, MediaAVCodec::AVCodecCategory::AVCODEC_HARDWARE);
    if (capabilityDataHEVC != nullptr) {
        encoderInfo.push_back(capabilityDataHEVC);
    }
    return Status::OK;
}
} // namespace Pipeline
} // namespace Media
} // namespace OHOS