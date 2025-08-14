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
#include <vector>
#include "avcodec_log.h"
#include "ffmpeg_converter.h"
namespace {
    constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_AUDIO, "FFmpegConverter"};
    constexpr int US_PER_SECOND = 1000000;
}
namespace OHOS {
namespace MediaAVCodec {
// ffmpeg channel layout to histreamer channel layout
const std::vector<std::pair<AudioChannelLayout, uint64_t>> g_toFFMPEGChannelLayout = {
    {AudioChannelLayout::MONO, AV_CH_LAYOUT_MONO},
    {AudioChannelLayout::STEREO, AV_CH_LAYOUT_STEREO},
    {AudioChannelLayout::CH_2POINT1, AV_CH_LAYOUT_2POINT1},
    {AudioChannelLayout::CH_2_1, AV_CH_LAYOUT_2_1},
    {AudioChannelLayout::SURROUND, AV_CH_LAYOUT_SURROUND},
    {AudioChannelLayout::CH_3POINT1, AV_CH_LAYOUT_3POINT1},
    {AudioChannelLayout::CH_4POINT0, AV_CH_LAYOUT_4POINT0},
    {AudioChannelLayout::CH_4POINT1, AV_CH_LAYOUT_4POINT1},
    {AudioChannelLayout::CH_2_2, AV_CH_LAYOUT_2_2},
    {AudioChannelLayout::QUAD, AV_CH_LAYOUT_QUAD},
    {AudioChannelLayout::CH_5POINT0, AV_CH_LAYOUT_5POINT0},
    {AudioChannelLayout::CH_5POINT1, AV_CH_LAYOUT_5POINT1},
    {AudioChannelLayout::CH_5POINT0_BACK, AV_CH_LAYOUT_5POINT0_BACK},
    {AudioChannelLayout::CH_5POINT1_BACK, AV_CH_LAYOUT_5POINT1_BACK},
    {AudioChannelLayout::CH_6POINT0, AV_CH_LAYOUT_6POINT0},
    {AudioChannelLayout::CH_6POINT0_FRONT, AV_CH_LAYOUT_6POINT0_FRONT},
    {AudioChannelLayout::HEXAGONAL, AV_CH_LAYOUT_HEXAGONAL},
    {AudioChannelLayout::CH_6POINT1, AV_CH_LAYOUT_6POINT1},
    {AudioChannelLayout::CH_6POINT1_BACK, AV_CH_LAYOUT_6POINT1_BACK},
    {AudioChannelLayout::CH_6POINT1_FRONT, AV_CH_LAYOUT_6POINT1_FRONT},
    {AudioChannelLayout::CH_7POINT0, AV_CH_LAYOUT_7POINT0},
    {AudioChannelLayout::CH_7POINT0_FRONT, AV_CH_LAYOUT_7POINT0_FRONT},
    {AudioChannelLayout::CH_7POINT1, AV_CH_LAYOUT_7POINT1},
    {AudioChannelLayout::CH_7POINT1_WIDE, AV_CH_LAYOUT_7POINT1_WIDE},
    {AudioChannelLayout::CH_7POINT1_WIDE_BACK, AV_CH_LAYOUT_7POINT1_WIDE_BACK},
    {AudioChannelLayout::OCTAGONAL, AV_CH_LAYOUT_OCTAGONAL},
    {AudioChannelLayout::HEXADECAGONAL, AV_CH_LAYOUT_HEXADECAGONAL},
    {AudioChannelLayout::STEREO_DOWNMIX, AV_CH_LAYOUT_STEREO_DOWNMIX},
};

const std::vector<std::pair<AVSampleFormat, AudioSampleFormat>> g_pFfSampleFmtMap = {
    {AVSampleFormat::AV_SAMPLE_FMT_U8, AudioSampleFormat::SAMPLE_U8},
    {AVSampleFormat::AV_SAMPLE_FMT_S16, AudioSampleFormat::SAMPLE_S16LE},
    {AVSampleFormat::AV_SAMPLE_FMT_S32, AudioSampleFormat::SAMPLE_S32LE},
    {AVSampleFormat::AV_SAMPLE_FMT_FLT, AudioSampleFormat::SAMPLE_F32LE},
    {AVSampleFormat::AV_SAMPLE_FMT_U8P, AudioSampleFormat::SAMPLE_U8P},
    {AVSampleFormat::AV_SAMPLE_FMT_S16P, AudioSampleFormat::SAMPLE_S16P},
    {AVSampleFormat::AV_SAMPLE_FMT_S32P, AudioSampleFormat::SAMPLE_S32P},
    {AVSampleFormat::AV_SAMPLE_FMT_FLTP, AudioSampleFormat::SAMPLE_F32P},
};

const std::vector<std::pair<AudioChannelLayout, std::string_view>> g_ChannelLayoutToString = {
    {AudioChannelLayout::UNKNOWN_CHANNEL_LAYOUT, "UNKNOW"},
    {AudioChannelLayout::MONO, "MONO"},
    {AudioChannelLayout::STEREO, "STEREO"},
    {AudioChannelLayout::CH_2POINT1, "2POINT1"},
    {AudioChannelLayout::CH_2_1, "CH_2_1"},
    {AudioChannelLayout::SURROUND, "SURROUND"},
    {AudioChannelLayout::CH_3POINT1, "3POINT1"},
    {AudioChannelLayout::CH_4POINT0, "4POINT0"},
    {AudioChannelLayout::CH_4POINT1, "4POINT1"},
    {AudioChannelLayout::CH_2_2, "CH_2_2"},
    {AudioChannelLayout::QUAD, "QUAD"},
    {AudioChannelLayout::CH_5POINT0, "5POINT0"},
    {AudioChannelLayout::CH_5POINT1, "5POINT1"},
    {AudioChannelLayout::CH_5POINT0_BACK, "5POINT0_BACK"},
    {AudioChannelLayout::CH_5POINT1_BACK, "5POINT1_BACK"},
    {AudioChannelLayout::CH_6POINT0, "6POINT0"},
    {AudioChannelLayout::CH_6POINT0_FRONT, "6POINT0_FRONT"},
    {AudioChannelLayout::HEXAGONAL, "HEXAGONAL"},
    {AudioChannelLayout::CH_6POINT1, "6POINT1"},
    {AudioChannelLayout::CH_6POINT1_BACK, "6POINT1_BACK"},
    {AudioChannelLayout::CH_6POINT1_FRONT, "6POINT1_FRONT"},
    {AudioChannelLayout::CH_7POINT0, "7POINT0"},
    {AudioChannelLayout::CH_7POINT0_FRONT, "7POINT0_FRONT"},
    {AudioChannelLayout::CH_7POINT1, "7POINT1"},
    {AudioChannelLayout::CH_7POINT1_WIDE, "7POINT1_WIDE"},
    {AudioChannelLayout::CH_7POINT1_WIDE_BACK, "7POINT1_WIDE_BACK"},
    {AudioChannelLayout::CH_3POINT1POINT2, "CH_3POINT1POINT2"},
    {AudioChannelLayout::CH_5POINT1POINT2, "CH_5POINT1POINT2"},
    {AudioChannelLayout::CH_5POINT1POINT4, "CH_5POINT1POINT4"},
    {AudioChannelLayout::CH_7POINT1POINT2, "CH_7POINT1POINT2"},
    {AudioChannelLayout::CH_7POINT1POINT4, "CH_7POINT1POINT4"},
    {AudioChannelLayout::CH_9POINT1POINT4, "CH_9POINT1POINT4"},
    {AudioChannelLayout::CH_9POINT1POINT6, "CH_9POINT1POINT6"},
    {AudioChannelLayout::CH_10POINT2, "CH_10POINT2"},
    {AudioChannelLayout::CH_22POINT2, "CH_22POINT2"},
    {AudioChannelLayout::OCTAGONAL, "OCTAGONAL"},
    {AudioChannelLayout::HEXADECAGONAL, "HEXADECAGONAL"},
    {AudioChannelLayout::STEREO_DOWNMIX, "STEREO_DOWNMIX"},
    {AudioChannelLayout::HOA_FIRST, "HOA_FIRST"},
    {AudioChannelLayout::HOA_SECOND, "HOA_SECOND"},
    {AudioChannelLayout::HOA_THIRD, "HOA_THIRD"},
};

AudioSampleFormat FFMpegConverter::ConvertFFMpegToOHAudioFormat(AVSampleFormat ffSampleFormat)
{
    auto ite = std::find_if(g_pFfSampleFmtMap.begin(), g_pFfSampleFmtMap.end(),
                            [&ffSampleFormat](const auto &item) -> bool { return item.first == ffSampleFormat; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfSampleFmtMap.end(), AudioSampleFormat::INVALID_WIDTH,
                              "Convert sample format failed: %{public}d", static_cast<int32_t>(ffSampleFormat));
    return ite->second;
}

AVSampleFormat FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat sampleFormat)
{
    auto ite = std::find_if(g_pFfSampleFmtMap.begin(), g_pFfSampleFmtMap.end(),
                            [&sampleFormat](const auto &item) -> bool { return item.second == sampleFormat; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_pFfSampleFmtMap.end(), AVSampleFormat::AV_SAMPLE_FMT_NONE,
                              "Convert sample format failed: %{public}d", static_cast<int32_t>(sampleFormat));
    return ite->first;
}

AudioChannelLayout FFMpegConverter::ConvertFFToOHAudioChannelLayout(uint64_t ffChannelLayout)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&ffChannelLayout](const auto &item) -> bool { return item.second == ffChannelLayout; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_toFFMPEGChannelLayout.end(), AudioChannelLayout::MONO,
                              "Convert channel layout failed: %{public}" PRIu64, ffChannelLayout);
    return ite->first;
}

uint64_t FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout channelLayout)
{
    auto ite = std::find_if(g_toFFMPEGChannelLayout.begin(), g_toFFMPEGChannelLayout.end(),
                            [&channelLayout](const auto &item) -> bool { return item.first == channelLayout; });
    if (ite == g_toFFMPEGChannelLayout.end()) {
        AVCODEC_LOGW("Convert channel layout failed: %{public}d", static_cast<int32_t>(channelLayout));
        return AV_CH_LAYOUT_NATIVE;
    }
    return ite->second;
}

std::string_view FFMpegConverter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout layout)
{
    auto ite = std::find_if(g_ChannelLayoutToString.begin(), g_ChannelLayoutToString.end(),
                            [&layout](const auto &item) -> bool { return item.first == layout; });
    CHECK_AND_RETURN_RET_LOGW(ite != g_ChannelLayoutToString.end(), g_ChannelLayoutToString[0].second,
                              "Convert channel layout failed: %{public}d", static_cast<int32_t>(layout));
    return ite->second;
}

int64_t FFMpegConverter::ConvertAudioPtsToUs(int64_t pts, AVRational base)
{
    CHECK_AND_RETURN_RET_LOG(pts != AV_NOPTS_VALUE, -1, "pts is invalid: %{public}" PRIu64, pts);
    AVRational us = {1, US_PER_SECOND};
    return av_rescale_q(pts, base, us);
}

std::string FFMpegConverter::AVStrError(int errnum)
{
    char errbuf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(errnum, errbuf, AV_ERROR_MAX_STRING_SIZE);
    return std::string(errbuf);
}
} // namespace MediaAVCodec
} // namespace OHOS