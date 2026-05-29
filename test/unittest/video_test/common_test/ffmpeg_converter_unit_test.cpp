/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include <gtest/gtest.h>
#include <memory>
#include "ffmpeg_converter.h"
#include "avcodec_audio_common.h"
#include "avcodec_audio_channel_layout.h"
#include "unittest_log.h"
#include "libavutil/channel_layout.h"
#include "libavcodec/avcodec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace OHOS {
namespace MediaAVCodec {
class FFmpegConverterUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp();
    void TearDown();
};

void FFmpegConverterUnitTest::SetUpTestCase(void)
{
    cout << "[SetUpTestCase]: " << endl;
}

void FFmpegConverterUnitTest::TearDownTestCase(void)
{
    cout << "[TearDownTestCase]: " << endl;
}

void FFmpegConverterUnitTest::SetUp(void)
{
    cout << "[SetUp]: SetUp!!!" << endl;
}

void FFmpegConverterUnitTest::TearDown(void)
{
    cout << "[TearDown]: over!!!" << endl;
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_01, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_U8);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_U8, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_02, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_S16);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_S16LE, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_03, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_S32);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_S32LE, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_04, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_FLT);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_F32LE, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_05, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_U8P);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_U8P, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_06, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_S16P);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_S16P, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_07, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_S32P);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_S32P, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_08, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_FLTP);
    EXPECT_EQ(AudioSampleFormat::SAMPLE_F32P, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_09, TestSize.Level1)
{
    AudioSampleFormat result = FFMpegConverter::ConvertFFMpegToOHAudioFormat(AV_SAMPLE_FMT_NONE);
    EXPECT_EQ(AudioSampleFormat::INVALID_WIDTH, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_10, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_U8);
    EXPECT_EQ(AV_SAMPLE_FMT_U8, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_11, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_S16LE);
    EXPECT_EQ(AV_SAMPLE_FMT_S16, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_12, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_S32LE);
    EXPECT_EQ(AV_SAMPLE_FMT_S32, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_13, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_F32LE);
    EXPECT_EQ(AV_SAMPLE_FMT_FLT, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_14, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_U8P);
    EXPECT_EQ(AV_SAMPLE_FMT_U8P, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_15, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_S16P);
    EXPECT_EQ(AV_SAMPLE_FMT_S16P, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_16, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_S32P);
    EXPECT_EQ(AV_SAMPLE_FMT_S32P, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_17, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::SAMPLE_F32P);
    EXPECT_EQ(AV_SAMPLE_FMT_FLTP, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_sample_format_18, TestSize.Level1)
{
    AVSampleFormat result = FFMpegConverter::ConvertOHAudioFormatToFFMpeg(AudioSampleFormat::INVALID_WIDTH);
    EXPECT_EQ(AV_SAMPLE_FMT_NONE, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_01, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_MONO);
    EXPECT_EQ(AudioChannelLayout::MONO, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_02, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_STEREO);
    EXPECT_EQ(AudioChannelLayout::STEREO, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_03, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_2POINT1);
    EXPECT_EQ(AudioChannelLayout::CH_2POINT1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_04, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_2_1);
    EXPECT_EQ(AudioChannelLayout::CH_2_1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_05, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_SURROUND);
    EXPECT_EQ(AudioChannelLayout::SURROUND, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_06, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_3POINT1);
    EXPECT_EQ(AudioChannelLayout::CH_3POINT1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_07, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_4POINT0);
    EXPECT_EQ(AudioChannelLayout::CH_4POINT0, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_08, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_4POINT1);
    EXPECT_EQ(AudioChannelLayout::CH_4POINT1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_09, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_5POINT0);
    EXPECT_EQ(AudioChannelLayout::CH_5POINT0, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_10, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_5POINT1);
    EXPECT_EQ(AudioChannelLayout::CH_5POINT1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_11, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_5POINT0_BACK);
    EXPECT_EQ(AudioChannelLayout::CH_5POINT0_BACK, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_12, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_5POINT1_BACK);
    EXPECT_EQ(AudioChannelLayout::CH_5POINT1_BACK, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_13, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(AV_CH_LAYOUT_7POINT1);
    EXPECT_EQ(AudioChannelLayout::CH_7POINT1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_14, TestSize.Level1)
{
    AudioChannelLayout result = FFMpegConverter::ConvertFFToOHAudioChannelLayout(0xFFFFFFFF);
    EXPECT_EQ(AudioChannelLayout::MONO, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_15, TestSize.Level1)
{
    uint64_t result = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout::MONO);
    EXPECT_EQ(AV_CH_LAYOUT_MONO, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_16, TestSize.Level1)
{
    uint64_t result = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout::STEREO);
    EXPECT_EQ(AV_CH_LAYOUT_STEREO, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_17, TestSize.Level1)
{
    uint64_t result = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout::CH_5POINT1);
    EXPECT_EQ(AV_CH_LAYOUT_5POINT1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_18, TestSize.Level1)
{
    uint64_t result = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout::CH_7POINT1);
    EXPECT_EQ(AV_CH_LAYOUT_7POINT1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_19, TestSize.Level1)
{
    uint64_t result = FFMpegConverter::ConvertOHAudioChannelLayoutToFFMpeg(AudioChannelLayout::UNKNOWN_CHANNEL_LAYOUT);
    EXPECT_EQ(0u, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_to_string_01, TestSize.Level1)
{
    std::string_view result = FFMpegConverter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout::MONO);
    EXPECT_STREQ("MONO", result.data());
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_to_string_02, TestSize.Level1)
{
    std::string_view result = FFMpegConverter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout::STEREO);
    EXPECT_STREQ("STEREO", result.data());
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_to_string_03, TestSize.Level1)
{
    std::string_view result = FFMpegConverter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout::CH_5POINT1);
    EXPECT_STREQ("5POINT1", result.data());
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_to_string_04, TestSize.Level1)
{
    std::string_view result = FFMpegConverter::ConvertOHAudioChannelLayoutToString(AudioChannelLayout::CH_7POINT1);
    EXPECT_STREQ("7POINT1", result.data());
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_channel_layout_to_string_05, TestSize.Level1)
{
    std::string_view result = FFMpegConverter::ConvertOHAudioChannelLayoutToString(
        AudioChannelLayout::UNKNOWN_CHANNEL_LAYOUT);
    EXPECT_STREQ("UNKNOW", result.data());
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_pts_01, TestSize.Level1)
{
    int64_t pts = 1000; // 1000
    AVRational base = {1, 1000}; // 1000
    int64_t result = FFMpegConverter::ConvertAudioPtsToUs(pts, base);
    EXPECT_EQ(1000000, result); // 1000000
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_pts_02, TestSize.Level1)
{
    int64_t pts = AV_NOPTS_VALUE;
    AVRational base = {1, 1000}; // 1000
    int64_t result = FFMpegConverter::ConvertAudioPtsToUs(pts, base);
    EXPECT_EQ(-1, result);
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_pts_03, TestSize.Level1)
{
    int64_t pts = 5000000; // 5000000
    AVRational base = {1, 1000000}; // 1000000
    int64_t result = FFMpegConverter::ConvertAudioPtsToUs(pts, base);
    EXPECT_EQ(5000000, result); // 5000000
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_avstrerror_01, TestSize.Level1)
{
    std::string result = FFMpegConverter::AVStrError(0);
    EXPECT_FALSE(result.empty());
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_avstrerror_02, TestSize.Level1)
{
    std::string result = FFMpegConverter::AVStrError(AVERROR_EOF);
    EXPECT_FALSE(result.empty());
}

HWTEST_F(FFmpegConverterUnitTest, ffmpeg_converter_avstrerror_03, TestSize.Level1)
{
    std::string result = FFMpegConverter::AVStrError(-1);
    EXPECT_FALSE(result.empty());
}
} // namespace MediaAVCodec
} // namespace OHOS