/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef CODECLIST_BUILDER_H
#define CODECLIST_BUILDER_H

#include "avcodec_info.h"
#include "codeclistbase.h"

namespace OHOS {
namespace MediaAVCodec {
class VideoCodecList : public CodecListBase {
public:
    ~VideoCodecList() override = default;
    int32_t GetCapabilityList(std::vector<CapabilityData> &caps) override;
};
class VideoHevcDecoderList : public CodecListBase {
public:
    ~VideoHevcDecoderList() override = default;
    int32_t GetCapabilityList(std::vector<CapabilityData> &caps) override;
};
class AudioCodecList : public CodecListBase {
public:
    ~AudioCodecList() override = default;
    int32_t GetCapabilityList(std::vector<CapabilityData> &caps) override;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_BUILDER_H
