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
#ifndef MEDIA_PIPELINE_CODEC_CAPABILITY_ADAPTER_H
#define MEDIA_PIPELINE_CODEC_CAPABILITY_ADAPTER_H

#include "common/status.h"
#include "osal/task/task.h"
#include "avcodec_list.h"

namespace OHOS {
namespace Media {
namespace Pipeline {
class CodecCapabilityAdapter {
public:
    explicit CodecCapabilityAdapter();
    ~CodecCapabilityAdapter();

    void Init();

    Status GetAvailableEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo);
private:
    Status GetVideoEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo);

    Status GetAudioEncoder(std::vector<MediaAVCodec::CapabilityData*> &encoderInfo);
    std::shared_ptr<MediaAVCodec::AVCodecList> codeclist_ {nullptr};
};
} // namespace Pipeline
} // namespace Media
} // namespace OHOS
#endif // MEDIA_PIPELINE_CODEC_CAPABILITY_Adapter_H

