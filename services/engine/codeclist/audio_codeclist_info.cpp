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
#include "hdi_codec.h"
#include <fstream>

namespace OHOS {
namespace MediaAVCodec {
const std::vector<int32_t> AUDIO_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000,
                                                32000, 44100, 48000, 64000, 88200, 96000};
constexpr int MAX_AUDIO_CHANNEL_COUNT = 8;
constexpr int MAX_SUPPORT_AUDIO_INSTANCE = 16;

constexpr int MIN_BIT_RATE_MP3 = 32000;
constexpr int MAX_BIT_RATE_MP3 = 320000;
constexpr int MIN_BIT_RATE_MP3_ENCODE = 8000;
constexpr int MAX_CHANNEL_COUNT_MP3 = 2;
constexpr int MAX_CHANNEL_COUNT_APE = 2;
constexpr int MAX_CHANNEL_COUNT_RAW = 16;
constexpr int MAX_CHANNEL_COUNT_G711A = 6;

constexpr int MIN_BIT_RATE_AAC = 8000;
constexpr int MAX_BIT_RATE_AAC = 960000;
constexpr int MIN_VORBIS_SAMPLE_RATE = 8000;
constexpr int MAX_VORBIS_SAMPLE_RATE = 96000;
constexpr int MIN_FLAC_SAMPLE_RATE = 8000;
constexpr int MAX_FLAC_SAMPLE_RATE = 384000;
constexpr int MAX_INT32 = 0x7FFFFFFF; // 2147483647
constexpr int MIN_ALAC_SAMPLE_RATE = 8000;
constexpr int MAX_ALAC_SAMPLE_RATE = 384000;

const std::vector<int32_t> AUDIO_VORBIS_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000,
                                                       32000, 44100, 48000, 64000, 88200, 96000};
const std::vector<int32_t> AUDIO_AMRNB_SAMPLE_RATE = {8000};

const std::vector<int32_t> AUDIO_AMRWB_SAMPLE_RATE = {16000};

const std::vector<int32_t> AUDIO_G711MU_SAMPLE_RATE = {8000};

const std::vector<int32_t> AUDIO_G711A_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000, 32000,
                                                      44100, 48000};

const std::vector<int32_t> AUDIO_FLAC_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000, 32000,
                                                     44100, 48000, 64000, 88200, 96000, 192000};

const std::vector<int32_t> AUDIO_FLAC_ENC_SAMPLE_RATE = {8000, 16000, 22050, 24000, 32000,
                                                         44100, 48000, 88200, 96000};

const std::vector<int32_t> AUDIO_AAC_SAMPLE_RATE = {7350, 8000, 11025, 12000, 16000, 22050, 24000, 32000,
                                                    44100, 48000, 64000, 88200, 96000};

const std::vector<int32_t> AUDIO_MP3_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000, 32000, 44100, 48000};
const std::vector<int32_t> AUDIO_RAW_SAMPLE_RATE = {8000, 11025, 12000, 16000, 22050, 24000, 32000,
    44100, 48000, 64000, 88200, 96000, 192000};

constexpr int MIN_BIT_RATE_FLAC = 32000;
constexpr int MAX_BIT_RATE_FLAC = 1536000;
constexpr int MIN_FLAC_COMPLIANCE_LEVEL = -2;
constexpr int MAX_FLAC_COMPLIANCE_LEVEL = 2;
constexpr int MAX_BIT_RATE_APE = 2100000;
constexpr int MIN_BIT_RATE_VORBIS = 32000;
constexpr int MAX_BIT_RATE_VORBIS = 500000;

constexpr int MIN_BIT_RATE_AMRWB = 6600;
constexpr int MAX_BIT_RATE_AMRWB = 23850;
constexpr int MIN_BIT_RATE_AMRNB = 4750;
constexpr int MAX_BIT_RATE_AMRNB = 12200;

constexpr int MIN_BIT_RATE_AAC_ENCODER = 1;
constexpr int MAX_BIT_RATE_AAC_ENCODER = 500000;

constexpr int MAX_BIT_RATE_RAW = 1536000;

#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
const std::vector<int32_t> AUDIO_VIVID_SAMPLE_RATE = {32000, 44100, 48000, 96000, 192000};
constexpr int MIN_BIT_RATE_VIVID_DECODER = 16000;
constexpr int MAX_BIT_RATE_VIVID_DECODER = 3075000;
constexpr int MAX_CHANNEL_COUNT_VIVID = 16;
const std::vector<int32_t> AUDIO_L2HC_SAMPLE_RATE = {44100, 48000, 88200, 96000, 176400, 192000};
constexpr int MIN_BITRATE_L2HC = 160000;
constexpr int MAX_BITRATE_L2HC = 10000000;
constexpr int MAX_CHANNEL_COUNT_L2HC = 12;
constexpr int MAX_SUPPORT_L2HC_VERSION = 4;
constexpr int MIN_BIT_RATE_OPUS = 6000;
constexpr int MAX_BIT_RATE_OPUS = 510000;
constexpr int MIN_OPUS_COMPLIANCE_LEVEL = 1;
constexpr int MAX_OPUS_COMPLIANCE_LEVEL = 10;
constexpr int MAX_CHANNEL_COUNT_OPUS = 2;
const std::vector<int32_t> AUDIO_OPUS_SAMPLE_RATE = {8000, 12000, 16000, 24000, 48000};
#endif
#ifdef SUPPORT_CODEC_COOK
constexpr int MAX_BIT_RATE_COOK = 510000;
const std::vector<int32_t> AUDIO_COOK_SAMPLE_RATE = {8000, 11025, 22050, 44100};
#endif
#ifdef SUPPORT_CODEC_EAC3
constexpr int MIN_BIT_RATE_EAC3 = 32000;
constexpr int MAX_BIT_RATE_EAC3 = 640000;
constexpr int EAC3_MAX_AUDIO_CHANNEL_COUNT = 16;
const std::vector<int32_t> AUDIO_EAC3_SAMPLE_RATE = {16000, 22050, 24000, 32000, 44100, 48000};
#endif
constexpr int MIN_BIT_RATE_AC3 = 32000;
constexpr int MAX_BIT_RATE_AC3 = 640000;
constexpr int MIN_BIT_RATE_ALAC = 0;
constexpr int MAX_BIT_RATE_ALAC = 3000000;
constexpr int ALAC_MAX_AUDIO_CHANNEL_COUNT = 8;
const std::vector<int32_t> AUDIO_ALAC_SAMPLE_RATE = {
    8000, 11025, 12000, 16000, 22050, 24000,
    32000, 44100, 48000, 88200, 96000, 176400, 192000
};
const std::vector<int32_t> AUDIO_AC3_SAMPLE_RATE = {11025, 32000, 44100, 48000};
constexpr int MAX_BIT_RATE_G711MU_DECODER = 64000;
constexpr int MAX_BIT_RATE_G711MU_ENCODER = 64000;
constexpr int MAX_BIT_RATE_G711A_DECODER = 64000;

const std::string VENDOR_AAC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libaudiocodec_aac_proxy_1.0.z.so";

constexpr int MIN_BIT_RATE_GSM_MS = 13000;
constexpr int MAX_BIT_RATE_GSM_MS = 13000;
constexpr int MAX_CHANNEL_COUNT_GSM_MS = 1;
const std::vector<int32_t> AUDIO_GSM_MS_SAMPLE_RATE = {8000};

static std::vector<Range> convertVectorToRange(const std::vector<int32_t> sampleRate)
{
    std::vector<Range> sampleRateRange;
    uint32_t sampleRateSize = sampleRate.size();
    for (uint32_t i = 0; i < sampleRateSize; i++) {
        sampleRateRange.push_back(Range(sampleRate[i], sampleRate[i]));
    }
    return sampleRateRange;
}

CapabilityData AudioCodeclistInfo::GetGsmMsDecoderCapability()
{
    CapabilityData audioGsmMsCapability;
    audioGsmMsCapability.codecName = AVCodecCodecName::AUDIO_DECODER_GSM_MS_NAME;
    audioGsmMsCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioGsmMsCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_GSM_MS;
    audioGsmMsCapability.isVendor = false;
    audioGsmMsCapability.bitrate = Range(MIN_BIT_RATE_GSM_MS, MAX_BIT_RATE_GSM_MS);
    audioGsmMsCapability.channels = Range(1, MAX_CHANNEL_COUNT_GSM_MS);
    audioGsmMsCapability.sampleRate = AUDIO_GSM_MS_SAMPLE_RATE;
    audioGsmMsCapability.sampleRateRanges = convertVectorToRange(AUDIO_GSM_MS_SAMPLE_RATE);
    audioGsmMsCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioGsmMsCapability;
}

CapabilityData AudioCodeclistInfo::GetMP3DecoderCapability()
{
    CapabilityData audioMp3Capability;
    audioMp3Capability.codecName = AVCodecCodecName::AUDIO_DECODER_MP3_NAME;
    audioMp3Capability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioMp3Capability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_MPEG;
    audioMp3Capability.isVendor = false;
    audioMp3Capability.bitrate = Range(MIN_BIT_RATE_MP3, MAX_BIT_RATE_MP3);
    audioMp3Capability.channels = Range(1, MAX_CHANNEL_COUNT_MP3);
    audioMp3Capability.sampleRate = AUDIO_MP3_SAMPLE_RATE;
    audioMp3Capability.sampleRateRanges = convertVectorToRange(AUDIO_MP3_SAMPLE_RATE);
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
    audioMp3Capability.sampleRate = AUDIO_MP3_SAMPLE_RATE;
    audioMp3Capability.sampleRateRanges = convertVectorToRange(AUDIO_MP3_SAMPLE_RATE);
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
    audioAacCapability.sampleRate = AUDIO_AAC_SAMPLE_RATE;
    audioAacCapability.sampleRateRanges = convertVectorToRange(AUDIO_AAC_SAMPLE_RATE);
    audioAacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAacCapability;
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
    audioFlacCapability.sampleRateRanges = {Range(MIN_FLAC_SAMPLE_RATE, MAX_FLAC_SAMPLE_RATE)};
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
    audioVorbisCapability.sampleRateRanges = {Range(MIN_VORBIS_SAMPLE_RATE, MAX_VORBIS_SAMPLE_RATE)};
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
    audioAmrnbCapability.bitrate = Range(MIN_BIT_RATE_AMRNB, MAX_BIT_RATE_AMRNB);
    audioAmrnbCapability.channels = Range(1, 1);
    audioAmrnbCapability.sampleRate = AUDIO_AMRNB_SAMPLE_RATE;
    audioAmrnbCapability.sampleRateRanges = convertVectorToRange(AUDIO_AMRNB_SAMPLE_RATE);
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
    audioAmrwbCapability.bitrate = Range(MIN_BIT_RATE_AMRWB, MAX_BIT_RATE_AMRWB);
    audioAmrwbCapability.channels = Range(1, 1);
    audioAmrwbCapability.sampleRate = AUDIO_AMRWB_SAMPLE_RATE;
    audioAmrwbCapability.sampleRateRanges = convertVectorToRange(AUDIO_AMRWB_SAMPLE_RATE);
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
    audioApeCapability.sampleRateRanges = {Range(1, MAX_INT32)};
    audioApeCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioApeCapability;
}

CapabilityData AudioCodeclistInfo::GetRawDecoderCapability()
{
    CapabilityData audioRawCapability;
    audioRawCapability.codecName = AVCodecCodecName::AUDIO_DECODER_RAW_NAME;
    audioRawCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioRawCapability.mimeType = CodecMimeType::AUDIO_RAW;
    audioRawCapability.isVendor = false;
    audioRawCapability.bitrate = Range(0, MAX_BIT_RATE_RAW);
    audioRawCapability.channels = Range(1, MAX_CHANNEL_COUNT_RAW);
    audioRawCapability.sampleRate = AUDIO_RAW_SAMPLE_RATE;
    audioRawCapability.sampleRateRanges = {Range(1, MAX_INT32)};
    audioRawCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioRawCapability;
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
    audioVividCapability.sampleRateRanges = convertVectorToRange(AUDIO_VIVID_SAMPLE_RATE);
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
    audioAmrnbCapability.sampleRateRanges = convertVectorToRange(AUDIO_AMRNB_SAMPLE_RATE);
    audioAmrnbCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAmrnbCapability;
}

CapabilityData AudioCodeclistInfo::GetAmrwbEncoderCapability()
{
    CapabilityData audioAmrwbCapability;
    audioAmrwbCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_AMRWB_NAME;
    audioAmrwbCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioAmrwbCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AMRWB;
    audioAmrwbCapability.isVendor = false;
    audioAmrwbCapability.bitrate = Range(1, MAX_BIT_RATE_AMRWB);
    audioAmrwbCapability.channels = Range(1, 1);
    audioAmrwbCapability.sampleRate = AUDIO_AMRWB_SAMPLE_RATE;
    audioAmrwbCapability.sampleRateRanges = convertVectorToRange(AUDIO_AMRWB_SAMPLE_RATE);
    audioAmrwbCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAmrwbCapability;
}

CapabilityData AudioCodeclistInfo::GetLbvcDecoderCapability()
{
    CapabilityData audioLbvcCapability;

    std::shared_ptr<Media::Plugins::Hdi::HdiCodec> hdiCodec_;
    hdiCodec_ = std::make_shared<Media::Plugins::Hdi::HdiCodec>();
    if (!hdiCodec_->IsSupportCodecType("OMX.audio.decoder.lbvc", &audioLbvcCapability)) {
        audioLbvcCapability.codecName = "";
        audioLbvcCapability.mimeType = "";
        audioLbvcCapability.maxInstance = 0;
        audioLbvcCapability.codecType = AVCODEC_TYPE_NONE;
        audioLbvcCapability.isVendor = false;
        audioLbvcCapability.bitrate = Range(0, 0);
        audioLbvcCapability.channels = Range(0, 0);
        audioLbvcCapability.sampleRate = {0};
        return audioLbvcCapability;
    }
    audioLbvcCapability.codecName = AVCodecCodecName::AUDIO_DECODER_LBVC_NAME;
    audioLbvcCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioLbvcCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC;
    audioLbvcCapability.isVendor = true;
    return audioLbvcCapability;
}

CapabilityData AudioCodeclistInfo::GetLbvcEncoderCapability()
{
    CapabilityData audioLbvcCapability;

    std::shared_ptr<Media::Plugins::Hdi::HdiCodec> hdiCodec_;
    hdiCodec_ = std::make_shared<Media::Plugins::Hdi::HdiCodec>();
    if (!hdiCodec_->IsSupportCodecType("OMX.audio.encoder.lbvc", &audioLbvcCapability)) {
        audioLbvcCapability.codecName = "";
        audioLbvcCapability.mimeType = "";
        audioLbvcCapability.maxInstance = 0;
        audioLbvcCapability.codecType = AVCODEC_TYPE_NONE;
        audioLbvcCapability.isVendor = false;
        audioLbvcCapability.bitrate = Range(0, 0);
        audioLbvcCapability.channels = Range(0, 0);
        audioLbvcCapability.sampleRate = {0};
        return audioLbvcCapability;
    }
    audioLbvcCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_LBVC_NAME;
    audioLbvcCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioLbvcCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_LBVC;
    audioLbvcCapability.isVendor = true;
    return audioLbvcCapability;
}

CapabilityData AudioCodeclistInfo::GetL2hcEncoderCapability()
{
    CapabilityData l2hcEncodeCapability;
    l2hcEncodeCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_L2HC_NAME;
    l2hcEncodeCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    l2hcEncodeCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_L2HC;
    l2hcEncodeCapability.isVendor = false;
    l2hcEncodeCapability.bitrate = Range(MIN_BITRATE_L2HC, MAX_BITRATE_L2HC);
    l2hcEncodeCapability.channels = Range(1, MAX_CHANNEL_COUNT_L2HC);
    l2hcEncodeCapability.sampleRate = AUDIO_L2HC_SAMPLE_RATE;
    l2hcEncodeCapability.sampleRateRanges = convertVectorToRange(AUDIO_L2HC_SAMPLE_RATE);
    l2hcEncodeCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    l2hcEncodeCapability.maxVersion = MAX_SUPPORT_L2HC_VERSION;
    return l2hcEncodeCapability;
}

CapabilityData AudioCodeclistInfo::GetL2hcDecoderCapability()
{
    CapabilityData l2hcDecodeCapability;
    l2hcDecodeCapability.codecName = AVCodecCodecName::AUDIO_DECODER_L2HC_NAME;
    l2hcDecodeCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    l2hcDecodeCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_L2HC;
    l2hcDecodeCapability.isVendor = false;
    l2hcDecodeCapability.bitrate = Range(0, 0);
    l2hcDecodeCapability.channels = Range(1, MAX_CHANNEL_COUNT_L2HC);
    l2hcDecodeCapability.sampleRate = AUDIO_L2HC_SAMPLE_RATE;
    l2hcDecodeCapability.sampleRateRanges = convertVectorToRange(AUDIO_L2HC_SAMPLE_RATE);
    l2hcDecodeCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    l2hcDecodeCapability.maxVersion = MAX_SUPPORT_L2HC_VERSION;
    return l2hcDecodeCapability;
}

CapabilityData AudioCodeclistInfo::GetVendorAacEncoderCapability()
{
    std::unique_ptr<std::ifstream> libFile = std::make_unique<std::ifstream>(VENDOR_AAC_LIB_PATH, std::ios::binary);
    CapabilityData audioAacCapability;
    if (!libFile->is_open()) {
        audioAacCapability.codecName = "";
        audioAacCapability.mimeType = "";
        audioAacCapability.maxInstance = 0;
        audioAacCapability.codecType = AVCODEC_TYPE_NONE;
        audioAacCapability.isVendor = false;
        audioAacCapability.bitrate = Range(0, 0);
        audioAacCapability.channels = Range(0, 0);
        audioAacCapability.sampleRate = {0};
        return audioAacCapability;
    }
    libFile->close();
    audioAacCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_VENDOR_AAC_NAME;
    audioAacCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioAacCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AAC;
    audioAacCapability.isVendor = false;
    audioAacCapability.bitrate = Range(MIN_BIT_RATE_AAC_ENCODER, MAX_BIT_RATE_AAC_ENCODER);
    audioAacCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioAacCapability.sampleRate = AUDIO_AAC_SAMPLE_RATE;
    audioAacCapability.sampleRateRanges = convertVectorToRange(AUDIO_AAC_SAMPLE_RATE);
    audioAacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    audioAacCapability.profiles = { AAC_PROFILE_LC, AAC_PROFILE_HE, AAC_PROFILE_HE_V2 };
    audioAacCapability.rank = 1; // larger than default rank 0
    return audioAacCapability;
}

CapabilityData AudioCodeclistInfo::GetOpusEncoderCapability()
{
    CapabilityData audioOpusCapability;
    audioOpusCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_OPUS_NAME;
    audioOpusCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioOpusCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_OPUS;
    audioOpusCapability.isVendor = false;
    audioOpusCapability.bitrate = Range(MIN_BIT_RATE_OPUS, MAX_BIT_RATE_OPUS);
    audioOpusCapability.channels = Range(1, MAX_CHANNEL_COUNT_OPUS);
    audioOpusCapability.sampleRate = AUDIO_OPUS_SAMPLE_RATE;
    audioOpusCapability.sampleRateRanges = convertVectorToRange(AUDIO_OPUS_SAMPLE_RATE);
    audioOpusCapability.complexity = Range(MIN_OPUS_COMPLIANCE_LEVEL, MAX_OPUS_COMPLIANCE_LEVEL);
    audioOpusCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioOpusCapability;
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
    audioOpusCapability.sampleRate = AUDIO_OPUS_SAMPLE_RATE;
    audioOpusCapability.sampleRateRanges = convertVectorToRange(AUDIO_OPUS_SAMPLE_RATE);
    audioOpusCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioOpusCapability;
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
    audioAacCapability.sampleRate = AUDIO_AAC_SAMPLE_RATE;
    audioAacCapability.sampleRateRanges = convertVectorToRange(AUDIO_AAC_SAMPLE_RATE);
    audioAacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    audioAacCapability.profiles = { AAC_PROFILE_LC };
    return audioAacCapability;
}

CapabilityData AudioCodeclistInfo::GetFlacEncoderCapability()
{
    CapabilityData audioFlacCapability;
    audioFlacCapability.codecName = AVCodecCodecName::AUDIO_ENCODER_FLAC_NAME;
    audioFlacCapability.codecType = AVCODEC_TYPE_AUDIO_ENCODER;
    audioFlacCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_FLAC;
    audioFlacCapability.isVendor = false;
    audioFlacCapability.bitrate = Range(MIN_BIT_RATE_FLAC, MAX_BIT_RATE_FLAC);
    audioFlacCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioFlacCapability.sampleRate = AUDIO_FLAC_ENC_SAMPLE_RATE;
    audioFlacCapability.sampleRateRanges = convertVectorToRange(AUDIO_FLAC_ENC_SAMPLE_RATE);
    audioFlacCapability.complexity = Range(MIN_FLAC_COMPLIANCE_LEVEL, MAX_FLAC_COMPLIANCE_LEVEL);
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
    audioG711muDecoderCapability.sampleRateRanges = convertVectorToRange(AUDIO_G711MU_SAMPLE_RATE);
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
    // G.711 has only one bitrate 64K, bitrate is no need to set during encoding.
    audioG711muEncoderCapability.bitrate = Range(MAX_BIT_RATE_G711MU_ENCODER, MAX_BIT_RATE_G711MU_ENCODER);
    audioG711muEncoderCapability.channels = Range(1, 1);
    audioG711muEncoderCapability.sampleRate = AUDIO_G711MU_SAMPLE_RATE;
    audioG711muEncoderCapability.sampleRateRanges = convertVectorToRange(AUDIO_G711MU_SAMPLE_RATE);
    audioG711muEncoderCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioG711muEncoderCapability;
}

CapabilityData AudioCodeclistInfo::GetG711aDecoderCapability()
{
    CapabilityData audioG711aDecoderCapability;
    audioG711aDecoderCapability.codecName = AVCodecCodecName::AUDIO_DECODER_G711A_NAME;
    audioG711aDecoderCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioG711aDecoderCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_G711A;
    audioG711aDecoderCapability.isVendor = false;
    audioG711aDecoderCapability.bitrate = Range(1, MAX_BIT_RATE_G711A_DECODER);
    audioG711aDecoderCapability.channels = Range(1, MAX_CHANNEL_COUNT_G711A);
    audioG711aDecoderCapability.sampleRate = AUDIO_G711A_SAMPLE_RATE;
    audioG711aDecoderCapability.sampleRateRanges = convertVectorToRange(AUDIO_G711A_SAMPLE_RATE);
    audioG711aDecoderCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioG711aDecoderCapability;
}

#ifdef SUPPORT_CODEC_COOK
CapabilityData  AudioCodeclistInfo::GetCookDecoderCapability()
{
    CapabilityData audioCookCapability;
    audioCookCapability.codecName = AVCodecCodecName::AUDIO_DECODER_COOK_NAME;
    audioCookCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioCookCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_COOK;
    audioCookCapability.isVendor = false;
    audioCookCapability.bitrate = Range(1, MAX_BIT_RATE_COOK);
    audioCookCapability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioCookCapability.sampleRate = AUDIO_COOK_SAMPLE_RATE;
    audioCookCapability.sampleRateRanges = convertVectorToRange(AUDIO_COOK_SAMPLE_RATE);
    audioCookCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioCookCapability;
}
#endif

CapabilityData AudioCodeclistInfo::GetAc3DecoderCapability()
{
    CapabilityData audioAc3Capability;
    audioAc3Capability.codecName = AVCodecCodecName::AUDIO_DECODER_AC3_NAME;
    audioAc3Capability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioAc3Capability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_AC3;
    audioAc3Capability.isVendor = false;
    audioAc3Capability.bitrate = Range(MIN_BIT_RATE_AC3, MAX_BIT_RATE_AC3);
    audioAc3Capability.channels = Range(1, MAX_AUDIO_CHANNEL_COUNT);
    audioAc3Capability.sampleRate = AUDIO_AC3_SAMPLE_RATE;
    audioAc3Capability.sampleRateRanges = convertVectorToRange(AUDIO_AC3_SAMPLE_RATE);
    audioAc3Capability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAc3Capability;
}

#ifdef SUPPORT_CODEC_EAC3
CapabilityData AudioCodeclistInfo::GetEac3DecoderCapability()
{
    CapabilityData audioEac3Capability;
    audioEac3Capability.codecName = AVCodecCodecName::AUDIO_DECODER_EAC3_NAME;
    audioEac3Capability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioEac3Capability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_EAC3;
    audioEac3Capability.isVendor = false;
    audioEac3Capability.bitrate = Range(MIN_BIT_RATE_EAC3, MAX_BIT_RATE_EAC3);
    audioEac3Capability.channels = Range(1, EAC3_MAX_AUDIO_CHANNEL_COUNT);
    audioEac3Capability.sampleRate = AUDIO_EAC3_SAMPLE_RATE;
    audioEac3Capability.sampleRateRanges = convertVectorToRange(AUDIO_EAC3_SAMPLE_RATE);
    audioEac3Capability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioEac3Capability;
}
#endif

CapabilityData AudioCodeclistInfo::GetAlacDecoderCapability()
{
    CapabilityData audioAlacCapability;
    audioAlacCapability.codecName = AVCodecCodecName::AUDIO_DECODER_ALAC_NAME;
    audioAlacCapability.codecType = AVCODEC_TYPE_AUDIO_DECODER;
    audioAlacCapability.mimeType = AVCodecMimeType::MEDIA_MIMETYPE_AUDIO_ALAC;
    audioAlacCapability.isVendor = false;
    audioAlacCapability.bitrate = Range(MIN_BIT_RATE_ALAC, MAX_BIT_RATE_ALAC);
    audioAlacCapability.channels = Range(1, ALAC_MAX_AUDIO_CHANNEL_COUNT);
    audioAlacCapability.sampleRate = AUDIO_ALAC_SAMPLE_RATE;
    audioAlacCapability.sampleRateRanges = {Range(MIN_ALAC_SAMPLE_RATE, MAX_ALAC_SAMPLE_RATE)};
    audioAlacCapability.maxInstance = MAX_SUPPORT_AUDIO_INSTANCE;
    return audioAlacCapability;
}

AudioCodeclistInfo::AudioCodeclistInfo()
{
    audioCapabilities_ = {
#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
                          GetVendorAacEncoderCapability(),
#endif
                          GetMP3DecoderCapability(),   GetAacDecoderCapability(),    GetFlacDecoderCapability(),
                          GetVorbisDecoderCapability(), GetAmrnbDecoderCapability(), GetAmrwbDecoderCapability(),
                          GetG711muDecoderCapability(), GetRawDecoderCapability(), GetAacEncoderCapability(),
                          GetFlacEncoderCapability(), GetG711muEncoderCapability(), GetAPEDecoderCapability(),
                          GetMP3EncoderCapability(), GetG711aDecoderCapability(), GetAc3DecoderCapability(),
                          GetGsmMsDecoderCapability(), GetAlacDecoderCapability(),
#ifdef AV_CODEC_AUDIO_VIVID_CAPACITY
                          GetVividDecoderCapability(), GetAmrnbEncoderCapability(), GetAmrwbEncoderCapability(),
                          GetLbvcDecoderCapability(), GetLbvcEncoderCapability(), GetL2hcEncoderCapability(),
                          GetL2hcDecoderCapability(), GetOpusDecoderCapability(), GetOpusEncoderCapability(),
#endif
#ifdef SUPPORT_CODEC_COOK
    GetCookDecoderCapability(),
#endif
#ifdef SUPPORT_CODEC_EAC3
    GetEac3DecoderCapability(),
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