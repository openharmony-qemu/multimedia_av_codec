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

#include "audio_codeclist_info.h"
#include "avcodec_mime_type.h"
#include "avcodec_codec_name.h"

namespace OHOS {
namespace MediaAVCodec {
const std::vector<int32_t> AUDIO_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000,
                                                32000, 44100, 48000, 64000, 88200, 96000};
constexpr int MAX_AUDIO_CHANNEL_COUNT = 8;
constexpr int MAX_SUPPORT_AUDIO_INSTANCE = 16;

constexpr int MIN_BIT_RATE_MP3 = 32000;
constexpr int MAX_BIT_RATE_MP3 = 320000;
constexpr int MAX_BIT_RATE_OPUS = 510000;
constexpr int MIN_BIT_RATE_MP3_ENCODE = 8000;
constexpr int MAX_CHANNEL_COUNT_MP3 = 2;
constexpr int MAX_CHANNEL_COUNT_APE = 2;

constexpr int MIN_BIT_RATE_AAC = 8000;
constexpr int MAX_BIT_RATE_AAC = 960000;
const std::vector<int32_t> AUDIO_VORBIS_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000,
                                                       32000, 44100, 48000, 64000, 88200, 96000};
const std::vector<int32_t> AUDIO_AMRNB_SAMPLE_RATE = {8000};

const std::vector<int32_t> AUDIO_AMRWB_SAMPLE_RATE = {16000};

const std::vector<int32_t> AUDIO_G711MU_SAMPLE_RATE = {8000};

const std::vector<int32_t> AUDIO_FLAC_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000, 32000,
                                                     44100, 48000, 64000, 88200, 96000, 192000};

const std::vector<int32_t> AUDIO_MP3_EN_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};

const std::vector<int32_t> AUDIO_LBVC_SAMPLE_RATE = {16000};

constexpr int MAX_BIT_RATE_FLAC = 2100000;
constexpr int MAX_BIT_RATE_APE = 2100000;
constexpr int MIN_BIT_RATE_VORBIS = 32000;
constexpr int MAX_BIT_RATE_VORBIS = 500000;

constexpr int MAX_BIT_RATE_AMRWB = 23850;
constexpr int MAX_BIT_RATE_AMRNB = 12200;

constexpr int MIN_BIT_RATE_AAC_ENCODER = 8000;
constexpr int MAX_BIT_RATE_AAC_ENCODER = 448000;

constexpr int MAX_BIT_RATE_LBVC = 6000;

#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
const std::vector<int32_t> AUDIO_VIVID_SAMPLE_RATE = {32000, 44100, 48000, 96000, 192000};
constexpr int MIN_BIT_RATE_VIVID_DECODER = 16000;
constexpr int MAX_BIT_RATE_VIVID_DECODER = 3075000;
constexpr int MAX_CHANNEL_COUNT_VIVID = 16;
#endif
constexpr int MAX_BIT_RATE_G711MU_DECODER = 64000;
constexpr int MAX_BIT_RATE_G711MU_ENCODER = 64000;

CapabilityData AudioCodeclistInfo::GetMP3DecoderCapability()
{
    CapabilityData audioMp3Capability;
    audioMp3Capability.codecName = AVCodecCodecName::AUDIO_DECODER_MP3_NAME;
    audioMp3Capability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioMp3Capability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG;
    audioMp3Capability.isVendor = false;
    audioMp3Capability.bitrate = Range(MIN_BIT_RATE_MP3, MAX_BIT_RATE_MP3);
    audioMp3Capability.channels = Range(1, MAX_CHANNEL_COUNT_MP3);
    audioMp3Capability.sampleRate = AUDIO_SAMPLE_RATE;
    audioMp3Capability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioMp3Capability;
}

CapabilityData AudioCodeclistInfo::GetMP3EncoderCapability()
{
    CapabilityData audioMp3Capability;
    audioMp3Capability.codecName = AVCodecCodecName::AUDIO_ENCODER_MP3_NAME;
    audioMp3Capability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioMp3Capability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG;
    audioMp3Capability.isVendor = false;
    audioMp3Capability.bitrate = Range(MIN_BIT_RATE_MP3_ENCODE, MAX_BIT_RATE_MP3);
    audioMp3Capability.channels = Range(1, MAX_CHANNEL_COUNT_MP3);
    audioMp3Capability.sampleRate = AUDIO_MP3_EN_SAMPLE_RATE;
    audioMp3Capability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioMp3Capability;
}

CapabilityData AudioCodeclistInfo::GetAacDecoderCapability()
{
    CapabilityData audioAacCapability;
    audioAacCapability.codecName = AVCodecCodecName::AUDIO_DECODER_AAC_NAME;
    audioAacCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioAacCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC;
    audioAacCapability.isVendor = false;
    audioAacCapability.bitrate = Range(MIN_BIT_RATE_AAC, MAX_BIT_RATE_AAC);
    audioAacCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioAacCapability.sampleRate = AUDIO_SAMPLE_RATE;
    audioAacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAacCapability;
}

CapabilityData AudioCodeclistInfo::GetOpusDecoderCapability()
{
    CapabilityData audioOpusCapability;
    audioOpusCapability.codecName = AVCodecCodecName::AUDIO_DECODER_OPUS_NAME;
    audioOpusCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioOpusCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS;
    audioOpusCapability.isVendor = false;
    audioOpusCapability.bitrate = Range(1, MAX_BIT_RATE_OPUS);
    audioOpusCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioOpusCapability.sampleRate = AUDIO_SAMPLE_RATE;
    audioOpusCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioOpusCapability;
}

CapabilityData AudioCodeclistInfo::GetFlacDecoderCapability()
{
    CapabilityData audioFlacCapability;
    audioFlacCapability.codecName = AVCodecCodecName::AUDIO_DECODER_FLAC_NAME;
    audioFlacCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioFlacCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC;
    audioFlacCapability.isVendor = false;
    audioFlacCapability.bitrate = Range(1, MAX_BIT_RATE_FLAC);
    audioFlacCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioFlacCapability.sampleRate = AUDIO_FLAC_SAMPLE_RATE;
    audioFlacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioFlacCapability;
}

CapabilityData AudioCodeclistInfo::GetVorbisDecoderCapability()
{
    CapabilityData audioVorbisCapability;
    audioVorbisCapability.codecName = AVCodecCodecName::AUDIO_DECODER_VORBIS_NAME;
    audioVorbisCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioVorbisCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VORBIS;
    audioVorbisCapability.isVendor = false;
    audioVorbisCapability.bitrate = Range(MIN_BIT_RATE_VORBIS, MAX_BIT_RATE_VORBIS);
    audioVorbisCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioVorbisCapability.sampleRate = AUDIO_VORBIS_SAMPLE_RATE;
    audioVorbisCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioVorbisCapability;
}

CapabilityData AudioCodeclistInfo::GetAmrnbDecoderCapability()
{
    CapabilityData audioAmrnbCapability;
    audioAmrnbCapability.codecName = AVCodecCodecName::AUDIO_DECODER_AMRNB_NAME;
    audioAmrnbCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioAmrnbCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB;
    audioAmrnbCapability.isVendor = false;
    audioAmrnbCapability.bitrate = Range(1, MAX_BIT_RATE_AMRNB);
    audioAmrnbCapability.channels = Range(1, 1);
    audioAmrnbCapability.sampleRate = AUDIO_AMRNB_SAMPLE_RATE;
    audioAmrnbCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAmrnbCapability;
}

CapabilityData AudioCodeclistInfo::GetAmrwbDecoderCapability()
{
    CapabilityData audioAmrwbCapability;
    audioAmrwbCapability.codecName = AVCodecCodecName::AUDIO_DECODER_AMRWB_NAME;
    audioAmrwbCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioAmrwbCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB;
    audioAmrwbCapability.isVendor = false;
    audioAmrwbCapability.bitrate = Range(1, MAX_BIT_RATE_AMRWB);
    audioAmrwbCapability.channels = Range(1, 1);
    audioAmrwbCapability.sampleRate = AUDIO_AMRWB_SAMPLE_RATE;
    audioAmrwbCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAmrwbCapability;
}

CapabilityData AudioCodeclistInfo::GetAPEDecoderCapability()
{
    CapabilityData audioApeCapability;
    audioApeCapability.codecName = AVCodecCodecName::AUDIO_DECODER_APE_NAME;
    audioApeCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioApeCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_APE;
    audioApeCapability.isVendor = false;
    audioApeCapability.bitrate = Range(0, MAX_BIT_RATE_APE);
    audioApeCapability.channels = Range(1, MAX_CHANNEL_COUNT_APE);
    audioApeCapability.sampleRate = AUDIO_SAMPLE_RATE;
    audioApeCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioApeCapability;
}

#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
CapabilityData AudioCodeclistInfo::GetVividDecoderCapability()
{
    CapabilityData audioVividCapability;
    audioVividCapability.codecName = AVCodecCodecName::AUDIO_DECODER_VIVID_NAME;
    audioVividCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioVividCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_VIVID;
    audioVividCapability.isVendor = false;
    audioVividCapability.bitrate = Range(MIN_BIT_RATE_VIVID_DECODER, MAX_BIT_RATE_VIVID_DECODER);
    audioVividCapability.channels = Range(1, MAX_CHANNEL_COUNT_VIVID);
    audioVividCapability.sampleRate = AUDIO_VIVID_SAMPLE_RATE;
    audioVividCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioVividCapability;
}

CapabilityData AudioCodeclistInfo::GetAmrnbEncoderCapability()
{
    CapabilityData audioAmrnbCapability;
    audioAmrnbCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_AMRNB_NAME;
    audioAmrnbCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioAmrnbCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRNB;
    audioAmrnbCapability.isVendor = false;
    audioAmrnbCapability.bitrate = Range(1, MAX_BIT_RATE_AMRNB);
    audioAmrnbCapability.channels = Range(1, 1);
    audioAmrnbCapability.sampleRate = AUDIO_AMRNB_SAMPLE_RATE;
    audioAmrnbCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAmrnbCapability;
}

CapabilityData AudioCodeclistInfo::GetAmrwbEncoderCapability()
{
    CapabilityData audioAmrwbCapability;
    audioAmrnbCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_AMRWB_NAME;
    audioAmrnbCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioAmrnbCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB;
    audioAmrnbCapability.isVendor = false;
    audioAmrnbCapability.bitrate = Range(1, MAX_BIT_RATE_AMRWB);
    audioAmrnbCapability.channels = Range(1, 1);
    audioAmrnbCapability.sampleRate = AUDIO_AMRWB_SAMPLE_RATE;
    audioAmrnbCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAmrwbCapability;
}
#endif

CapabilityData AudioCodeclistInfo::GetAacEncoderCapability()
{
    CapabilityData audioAacCapability;
    audioAacCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_AAC_NAME;
    audioAacCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioAacCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC;
    audioAacCapability.isVendor = false;
    audioAacCapability.bitrate = Range(MIN_BIT_RATE_AAC_ENCODER, MAX_BIT_RATE_AAC_ENCODER);
    audioAacCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioAacCapability.sampleRate = AUDIO_SAMPLE_RATE;
    audioAacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    audioAacCapability.profiles = { AAC_PROFILE_LC };
    return audioAacCapability;
}

CapabilityData AudioCodeclistInfo::GetOpusEncoderCapability()
{
    CapabilityData audioOpusCapability;
    audioOpusCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME;
    audioOpusCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioOpusCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS;
    audioOpusCapability.isVendor = false;
    audioOpusCapability.bitrate = Range(1, MAX_BIT_RATE_OPUS);
    audioOpusCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioOpusCapability.sampleRate = AUDIO_SAMPLE_RATE;
    audioOpusCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioOpusCapability;
}

CapabilityData AudioCodeclistInfo::GetFlacEncoderCapability()
{
    CapabilityData audioFlacCapability;
    audioFlacCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME;
    audioFlacCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioFlacCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC;
    audioFlacCapability.isVendor = false;
    audioFlacCapability.bitrate = Range(1, MAX_BIT_RATE_FLAC);
    audioFlacCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioFlacCapability.sampleRate = AUDIO_SAMPLE_RATE;
    audioFlacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioFlacCapability;
}

CapabilityData AudioCodeclistInfo::GetG711muDecoderCapability()
{
    CapabilityData audioG711muDecoderCapability;
    audioG711muDecoderCapability.codecName = AVCodecCodecName::AUDIO_DECODER_G711MU_NAME;
    audioG711muDecoderCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioG711muDecoderCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU;
    audioG711muDecoderCapability.isVendor = false;
    audioG711muDecoderCapability.bitrate = Range(1, MAX_BIT_RATE_G711MU_DECODER);
    audioG711muDecoderCapability.channels = Range(1, 1);
    audioG711muDecoderCapability.sampleRate = AUDIO_G711MU_SAMPLE_RATE;
    audioG711muDecoderCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioG711muDecoderCapability;
}

CapabilityData AudioCodeclistInfo::GetG711muEncoderCapability()
{
    CapabilityData audioG711muEncoderCapability;
    audioG711muEncoderCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_G711MU_NAME;
    audioG711muEncoderCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioG711muEncoderCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711MU;
    audioG711muEncoderCapability.isVendor = false;
    audioG711muEncoderCapability.bitrate = Range(1, MAX_BIT_RATE_G711MU_ENCODER);
    audioG711muEncoderCapability.channels = Range(1, 1);
    audioG711muEncoderCapability.sampleRate = AUDIO_G711MU_SAMPLE_RATE;
    audioG711muEncoderCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioG711muEncoderCapability;
}

CapabilityData AudioCodeclistInfo::GetLbvcDecoderCapability()
{
    CapabilityData audioLbvcCapability;
    audioLbvcCapability.codecName = AVCodecCodecName::AUDIO_DECODER_LBVC_NAME;
    audioLbvcCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioLbvcCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC;
    audioLbvcCapability.isVendor = true;
    audioLbvcCapability.bitrate = Range(MAX_BIT_RATE_LBVC, MAX_BIT_RATE_LBVC);
    audioLbvcCapability.channels = Range(1, 1);
    audioLbvcCapability.sampleRate = AUDIO_LBVC_SAMPLE_RATE;
    audioLbvcCapability.maxInstance = 1;
    return audioLbvcCapability;
}

CapabilityData AudioCodeclistInfo::GetLbvcEncoderCapability()
{
    CapabilityData audioLbvcCapability;
    audioLbvcCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_LBVC_NAME;
    audioLbvcCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioLbvcCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC;
    audioLbvcCapability.isVendor = true;
    audioLbvcCapability.bitrate = Range(MAX_BIT_RATE_LBVC, MAX_BIT_RATE_LBVC);
    audioLbvcCapability.channels = Range(1, 1);
    audioLbvcCapability.sampleRate = AUDIO_LBVC_SAMPLE_RATE;
    audioLbvcCapability.maxInstance = 1;
    return audioLbvcCapability;
}

AudioCodeclistInfo::AudioCodeclistInfo()
{
    audioCapabilities_ = {GetMP3DecoderCapability(),   GetAacDecoderCapability(),    GetFlacDecoderCapability(),
                          GetOpusDecoderCapability(),  GetVorbisDecoderCapability(), GetAmrnbDecoderCapability(),
                          GetAmrwbDecoderCapability(), GetG711muDecoderCapability(), GetAacEncoderCapability(),
                          GetFlacEncoderCapability(),  GetOpusEncoderCapability(),   GetG711muEncoderCapability(),
                          GetLbvcDecoderCapability(), GetLbvcEncoderCapability(), GetAPEDecoderCapability(),
                          GetMP3EncoderCapability(),
#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
                          GetVividDecoderCapability(), GetAmrnbEncoderCapability(), GetAmrwbEncoderCapability()
#endif
    };
}

AudioCodeclistInfo::~AudioCodeclistInfo()
{
    audioCapabilities_.clear();
}

AudioCodeclistInfo &AudioCodeclistInfo::GetInstance()
{
    static AudioCodeclistInfo audioCodecList;
    return audioCodecList;
}

std::vector<CapabilityData> AudioCodeclistInfo::GetAudioCapabilities() const noexcept
{
    return audioCapabilities_;
}
} // namespace MediaAVCodec
} // namespace OHOS