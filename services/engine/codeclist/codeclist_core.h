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

#ifndef CODECLIST_CORE_H
#define CODECLIST_CORE_H

#include <mutex>
#include "nocopyable.h"
#include "meta/format.h"
#include "codeclist_utils.h"
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
class __attribute__((visibility("default"))) CodecListCore : public NoCopyable {
public:
    CodecListCore();
    ~CodecListCore();
    std::string FindEncoder(const Media::Format &format);
    std::string FindDecoder(const Media::Format &format);
    CodecType FindCodecType(std::string codecName);
    std::vector<std::string> FindCodecNameArray(const std::string &mime, bool isEncoder);
    int32_t GetCapability(CapabilityData &capData, const std::string &mime, const bool isEncoder,
                          const AVCodecCategory &category);

private:
    bool CheckBitrate(const Media::Format &format, const CapabilityData &data);
    bool CheckVideoResolution(const Media::Format &format, const CapabilityData &data);
    bool CheckVideoPixelFormat(const Media::Format &format, const CapabilityData &data);
    bool CheckVideoFrameRate(const Media::Format &format, const CapabilityData &data);
    bool CheckAudioChannel(const Media::Format &format, const CapabilityData &data);
    bool CheckAudioSampleRate(const Media::Format &format, const CapabilityData &data);
    bool IsVideoCapSupport(const Media::Format &format, const CapabilityData &data);
    bool IsAudioCapSupport(const Media::Format &format, const CapabilityData &data);
    std::string FindCodec(const Media::Format &format, bool isEncoder);
    std::mutex mutex_;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // CODECLIST_CORE_H
