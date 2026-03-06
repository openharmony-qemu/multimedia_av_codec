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
#ifndef AUDIO_CODECLIST_INFO_H
#define AUDIO_CODECLIST_INFO_H

#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
class AudioCodeclistInfo {
public:
    ~AudioCodeclistInfo();
    static AudioCodeclistInfo &GetInstance();
    std::vector<CapabilityData> GetAudioCapabilities() const noexcept;
    CapabilityData GetMP3DecoderCapability();
    CapabilityData GetAacDecoderCapability();
    CapabilityData GetFlacDecoderCapability();
    CapabilityData GetVorbisDecoderCapability();
    CapabilityData GetAmrnbDecoderCapability();
    CapabilityData GetAmrwbDecoderCapability();
    CapabilityData GetAacEncoderCapability();
    CapabilityData GetFlacEncoderCapability();
    CapabilityData GetG711muEncoderCapability();
    CapabilityData GetG711muDecoderCapability();
    CapabilityData GetG711aDecoderCapability();
    CapabilityData GetAPEDecoderCapability();
    CapabilityData GetMP3EncoderCapability();
    CapabilityData GetLbvcDecoderCapability();
    CapabilityData GetRawDecoderCapability();
    CapabilityData GetLbvcEncoderCapability();
    CapabilityData GetVendorAacEncoderCapability();
    CapabilityData GetAc3DecoderCapability();
    CapabilityData GetGsmMsDecoderCapability();
    CapabilityData GetGsmDecoderCapability();
    CapabilityData GetAlacDecoderCapability();
    CapabilityData GetWMAV1DecoderCapability();
    CapabilityData GetWMAV2DecoderCapability();
    CapabilityData GetWMAProDecoderCapability();
    CapabilityData GetIlbcDecoderCapability();
#ifdef SUPPORT_CODEC_TRUEHD
    CapabilityData GetTruehdDecoderCapability();
#endif
    CapabilityData GetTwinVQDecoderCapability();
#ifdef SUPPORT_CODEC_EAC3
    CapabilityData GetEac3DecoderCapability();
#endif
#ifdef AV_CODEC_AUDIO_SPECIAL_CAPACITY
    CapabilityData GetVividDecoderCapability();
    CapabilityData GetAmrnbEncoderCapability();
    CapabilityData GetAmrwbEncoderCapability();
    CapabilityData GetL2hcEncoderCapability();
    CapabilityData GetL2hcDecoderCapability();
#endif
#ifdef SUPPORT_CODEC_OPUS
    CapabilityData GetOpusDecoderCapability();
    CapabilityData GetOpusEncoderCapability();
#endif
    CapabilityData GetDVAudioDecoderCapability();
#ifdef SUPPORT_CODEC_DTS
    CapabilityData GetDtsDecoderCapability();
#endif
    CapabilityData GetCookDecoderCapability();
private:
    std::vector<CapabilityData> audioCapabilities_;
    AudioCodeclistInfo();

    void AppendAdpcmCapabilities();
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif