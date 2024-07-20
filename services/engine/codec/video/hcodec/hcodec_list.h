/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HCODEC_HCODEC_LIST_H
#define HCODEC_HCODEC_LIST_H

#include "codeclistbase.h"
#include "avcodec_errors.h"
#include "codec_hdi.h"

namespace OHOS::MediaAVCodec {
class HCodecList : public CodecListBase {
public:
    HCodecList() = default;
    int32_t GetCapabilityList(std::vector<CapabilityData>& caps) override;
private:
    CapabilityData HdiCapToUserCap(const CodecHDI::CodecCompCapability& hdiCap);
    std::vector<int32_t> GetSupportedBitrateMode(const CodecHDI::CodecVideoPortCap& hdiVideoCap);
    std::vector<int32_t> GetSupportedFormat(const CodecHDI::CodecVideoPortCap& hdiVideoCap);
    std::map<ImgSize, Range> GetMeasuredFrameRate(const CodecHDI::CodecVideoPortCap& hdiVideoCap);
    void GetCodecProfileLevels(const CodecHDI::CodecCompCapability& hdiCap, CapabilityData& userCap);
    bool IsSupportedVideoCodec(const CodecHDI::CodecCompCapability& hdiCap);
    void GetSupportedFeatureParam(const CodecHDI::CodecVideoPortCap& hdiVideoCap,
                                  CapabilityData& userCap);
};

sptr<CodecHDI::ICodecComponentManager> GetManager(bool getCap, bool supportPassthrough = false);
std::vector<CodecHDI::CodecCompCapability> GetCapList();
}

#endif