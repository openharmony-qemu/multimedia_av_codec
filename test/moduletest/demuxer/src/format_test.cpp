/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "gtest/gtest.h"

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <cmath>
#include <thread>

namespace OHOS {
namespace Media {
class DemuxerFormatNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};

static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;

static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
constexpr uint64_t AVC_BITRATE = 2144994;
constexpr uint64_t ACTUAL_DURATION = 4120000;
constexpr uint32_t ACTUAL_AUDIOFORMAT = 9;
constexpr uint32_t ACTUAL_AUDIOCOUNT = 2;
constexpr uint64_t ACTUAL_LAYOUT = 3;
constexpr uint32_t ACTUAL_SAMPLERATE = 44100;
constexpr uint32_t ACTUAL_CODEDSAMPLE = 16;
constexpr uint32_t ACTUAL_CURRENTWIDTH = 1920;
constexpr uint32_t ACTUAL_CURRENTHEIGHT = 1080;
constexpr double ACTUAL_FRAMERATE = 25;
constexpr uint64_t HEVC_BITRATE = 4162669;
constexpr uint32_t ACTUAL_CHARACTERISTICS = 2;
constexpr uint32_t ACTUAL_COEFFICIENTS = 2;
constexpr uint32_t ACTUAL_PRIMARIES = 2;

void DemuxerFormatNdkTest::SetUpTestCase() {}
void DemuxerFormatNdkTest::TearDownTestCase() {}
void DemuxerFormatNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerFormatNdkTest::TearDown()
{
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }

    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }

    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (avBuffer != nullptr) {
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
}
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

string g_vvc8bitPath = string("/data/test/media/vvc_8bit_3840_2160.mp4");
string g_vvc10bitPath = string("/data/test/media/vvc_aac_10bit_1920_1080.mp4");

static int64_t GetFileSize(const char *fileName)
{
    int64_t fileSize = 0;
    if (fileName != nullptr) {
        struct stat fileStatus {};
        if (stat(fileName, &fileStatus) == 0) {
            fileSize = static_cast<int64_t>(fileStatus.st_size);
        }
    }
    return fileSize;
}

static void SetAudioValue(OH_AVCodecBufferAttr attr, bool &audioIsEnd, int &audioFrame, int &aKeyCount)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        audioIsEnd = true;
        cout << audioFrame << "    audio is end !!!!!!!!!!!!!!!" << endl;
    } else {
        audioFrame++;
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            aKeyCount++;
        }
    }
}

static void SetVideoValue(OH_AVCodecBufferAttr attr, bool &videoIsEnd, int &videoFrame, int &vKeyCount)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        videoIsEnd = true;
        cout << videoFrame << "   video is end !!!!!!!!!!!!!!!" << endl;
    } else {
        videoFrame++;
        cout << "video track !!!!!" << endl;
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            vKeyCount++;
        }
    }
}

static void CheckVideoKey()
{
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    int64_t bitrate = 0;
    const char* mimeType = nullptr;
    double frameRate;
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    const char* language = nullptr;
    int32_t rotation;
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
    int bitrateResult = 1660852;
    ASSERT_EQ(bitrateResult, bitrate);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize));
    size_t bufferSizeResult = 255;
    ASSERT_EQ(bufferSizeResult, bufferSize);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectNum = 0;
    ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_VVC));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    int frameRateResult = 50.000000;
    ASSERT_EQ(frameRateResult, frameRate);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    int currentHeightResult = 1080;
    ASSERT_EQ(currentHeightResult, currentHeight);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &currentWidth));
    int currentWidthResult = 1920;
    ASSERT_EQ(currentWidthResult, currentWidth);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_LANGUAGE, &language));
    ASSERT_EQ(expectNum, strcmp(language, "und"));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_ROTATION, &rotation));
    int rotationResult = 0;
    ASSERT_EQ(rotationResult, rotation);
}

static void CheckAudioKey()
{
    int32_t aacisAdts = 0;
    int64_t channelLayout;
    int32_t audioCount = 0;
    int32_t sampleFormat;
    int64_t bitrate = 0;
    int32_t bitsPreCodedSample;
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    const char* mimeType = nullptr;
    int32_t sampleRate = 0;
    const char* language = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AAC_IS_ADTS, &aacisAdts));
    int aacisAdtsResult = 1;
    ASSERT_EQ(aacisAdtsResult, aacisAdts);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_CHANNEL_LAYOUT, &channelLayout));
    int channelLayoutResult = 3;
    ASSERT_EQ(channelLayoutResult, channelLayout);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &audioCount));
    int audioCountResult = 2;
    ASSERT_EQ(audioCountResult, audioCount);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &sampleFormat));
    int sampleFormatResult = 0;
    ASSERT_EQ(sampleFormatResult, sampleFormat);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
    int bitrateResult = 127881;
    ASSERT_EQ(bitrateResult, bitrate);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_BITS_PER_CODED_SAMPLE, &bitsPreCodedSample));
    int bitsPreCodedSampleResult = 16;
    ASSERT_EQ(bitsPreCodedSampleResult, bitsPreCodedSample);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize));
    int bufferSizeResult = 5;
    ASSERT_EQ(bufferSizeResult, bufferSize);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectNum = 0;
    ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_AAC));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_LANGUAGE, &language));
    ASSERT_EQ(expectNum, strcmp(language, "eng"));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    int sampleRateResult = 48000;
    ASSERT_EQ(sampleRateResult, sampleRate);
}

static void CheckAudioKeyVvc()
{
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    const char* language = nullptr;
    int64_t bitrate = 0;
    double frameRate;
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int tarckType = 0;
    const char* mimeType = nullptr;
    int32_t rotation;
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
    int bitrateResult = 10014008;
    ASSERT_EQ(bitrateResult, bitrate);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize));
    int bufferSizeResult = 247;
    ASSERT_EQ(bufferSizeResult, bufferSize);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    int expectNum = 0;
    ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_VVC));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    int frameRateResult = 60.000000;
    ASSERT_EQ(frameRateResult, frameRate);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    int currentHeightResult = 2160;
    ASSERT_EQ(currentHeightResult, currentHeight);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &currentWidth));
    int currentWidthResult = 3840;
    ASSERT_EQ(currentWidthResult, currentWidth);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_LANGUAGE, &language));
    ASSERT_EQ(expectNum, strcmp(language, "und"));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_ROTATION, &rotation));
    int rotationResult = 0;
    ASSERT_EQ(rotationResult, rotation);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    int tarckTypeResult = 1;
    ASSERT_EQ(tarckTypeResult, tarckType);
}

static void SetAllParam(OH_AVFormat *paramFormat)
{
    int64_t duration = 0;
    int64_t startTime = 0;
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* albumArtist = nullptr;
    const char* date = nullptr;
    const char* comment = nullptr;
    const char* genre = nullptr;
    const char* copyright = nullptr;
    const char* language = nullptr;
    const char* description = nullptr;
    const char* lyrics = nullptr;
    const char* title = nullptr;
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(ACTUAL_DURATION, duration);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "title"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "artist"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "album"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_ALBUM_ARTIST, &albumArtist));
    ASSERT_EQ(0, strcmp(albumArtist, "album artist"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_DATE, &date));
    ASSERT_EQ(0, strcmp(date, "2023"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_COMMENT, &comment));
    ASSERT_EQ(0, strcmp(comment, "comment"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_GENRE, &genre));
    ASSERT_EQ(0, strcmp(genre, "genre"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_COPYRIGHT, &copyright));
    ASSERT_EQ(0, strcmp(copyright, "Copyright"));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_LANGUAGE, &language));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_DESCRIPTION, &description));
    ASSERT_EQ(0, strcmp(description, "description"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_LYRICS, &lyrics));
    ASSERT_EQ(0, strcmp(lyrics, "lyrics"));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_START_TIME, &startTime));
    ASSERT_EQ(0, startTime);
}

static void SetAudioParam(OH_AVFormat *paramFormat)
{
    int32_t codedSample = 0;
    int32_t audioFormat = 0;
    int32_t audioCount = 0;
    int32_t sampleRate = 0;
    int32_t aacisAdts = 0;
    int64_t layout = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &audioFormat));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &audioCount));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_BITS_PER_CODED_SAMPLE, &codedSample));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AAC_IS_ADTS, &aacisAdts));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_CHANNEL_LAYOUT, &layout));
    ASSERT_EQ(ACTUAL_AUDIOFORMAT, audioFormat);
    ASSERT_EQ(ACTUAL_AUDIOCOUNT, audioCount);
    ASSERT_EQ(ACTUAL_SAMPLERATE, sampleRate);
    ASSERT_EQ(ACTUAL_CODEDSAMPLE, codedSample);
    ASSERT_EQ(1, aacisAdts);
    ASSERT_EQ(ACTUAL_LAYOUT, layout);
}

static void AvcVideoParam(OH_AVFormat *paramFormat)
{
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int32_t rotation = 0;
    double frameRate = 0.0;
    int32_t profile = 0;
    int32_t flag = 0;
    int32_t characteristics = 0;
    int32_t coefficients = 0;
    int32_t mode = 0;
    int64_t bitrate = 0;
    int32_t primaries = 0;
    int32_t videoIsHdrvivid = 0;
    const char* mimeType = nullptr;
    double sar = 0.0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_WIDTH, &currentWidth));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_ROTATION, &rotation));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(paramFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_PROFILE, &profile));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_BITRATE, &bitrate));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, &mode));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_RANGE_FLAG, &flag));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS, &characteristics));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_MATRIX_COEFFICIENTS, &coefficients));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, &videoIsHdrvivid));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_COLOR_PRIMARIES, &primaries));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(paramFormat, OH_MD_KEY_VIDEO_SAR, &sar));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_AVC));
    ASSERT_EQ(ACTUAL_CURRENTWIDTH, currentWidth);
    ASSERT_EQ(ACTUAL_CURRENTHEIGHT, currentHeight);
    ASSERT_EQ(ACTUAL_FRAMERATE, frameRate);
    ASSERT_EQ(AVC_BITRATE, bitrate);
    ASSERT_EQ(1, sar);
}

static void HevcVideoParam(OH_AVFormat *paramFormat)
{
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int32_t rotation = 0;
    double frameRate = 0.0;
    int32_t profile = 0;
    int32_t flag = 0;
    int32_t characteristics = 0;
    int32_t coefficients = 0;
    int32_t mode = 0;
    int64_t bitrate = 0;
    int32_t primaries = 0;
    int32_t videoIsHdrvivid = 0;
    const char* mimeType = nullptr;
    double sar = 0.0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_WIDTH, &currentWidth));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_ROTATION, &rotation));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(paramFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_BITRATE, &bitrate));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, &mode));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_RANGE_FLAG, &flag));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS, &characteristics));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_MATRIX_COEFFICIENTS, &coefficients));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, &videoIsHdrvivid));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_COLOR_PRIMARIES, &primaries));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(paramFormat, OH_MD_KEY_VIDEO_SAR, &sar));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_PROFILE, &profile));
        ASSERT_EQ(0, profile);
    } else {
        ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_PROFILE, &profile));
    }
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_HEVC));
    ASSERT_EQ(ACTUAL_CURRENTWIDTH, currentWidth);
    ASSERT_EQ(ACTUAL_CURRENTHEIGHT, currentHeight);
    ASSERT_EQ(ACTUAL_FRAMERATE, frameRate);
    ASSERT_EQ(HEVC_BITRATE, bitrate);
    ASSERT_EQ(0, flag);
    ASSERT_EQ(ACTUAL_CHARACTERISTICS, characteristics);
    ASSERT_EQ(ACTUAL_COEFFICIENTS, coefficients);
    ASSERT_EQ(ACTUAL_PRIMARIES, primaries);
    ASSERT_EQ(1, sar);
}

static void SetOtherAllParam(OH_AVFormat *paramFormat)
{
    int64_t duration = 0;
    int64_t startTime = 0;
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* albumArtist = nullptr;
    const char* date = nullptr;
    const char* comment = nullptr;
    const char* genre = nullptr;
    const char* copyright = nullptr;
    const char* language = nullptr;
    const char* description = nullptr;
    const char* lyrics = nullptr;
    const char* title = nullptr;
    int32_t dur = 10800000;
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(dur, duration);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_ALBUM_ARTIST, &albumArtist));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_DATE, &date));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_COMMENT, &comment));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_GENRE, &genre));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_COPYRIGHT, &copyright));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_LANGUAGE, &language));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_DESCRIPTION, &description));
    ASSERT_FALSE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_LYRICS, &lyrics));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_START_TIME, &startTime));
    ASSERT_EQ(0, startTime);
}

static void SetOtherAudioParam(OH_AVFormat *paramFormat)
{
    int32_t codedSample = 0;
    int32_t audioFormat = 0;
    int32_t audioCount = 0;
    int32_t sampleRate = 0;
    int32_t aacisAdts = 0;
    int64_t layout = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &audioFormat));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &audioCount));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_BITS_PER_CODED_SAMPLE, &codedSample));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_AAC_IS_ADTS, &aacisAdts));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_CHANNEL_LAYOUT, &layout));
    ASSERT_EQ(ACTUAL_AUDIOFORMAT, audioFormat);
    ASSERT_EQ(ACTUAL_AUDIOCOUNT, audioCount);
    ASSERT_EQ(ACTUAL_SAMPLERATE, sampleRate);
    ASSERT_EQ(ACTUAL_CODEDSAMPLE, codedSample);
    ASSERT_EQ(1, aacisAdts);
    ASSERT_EQ(ACTUAL_LAYOUT, layout);
}

static void OtherVideoParam(OH_AVFormat *paramFormat)
{
    int32_t currentWidth = 0;
    int32_t currentHeight = 0;
    int32_t rotation = 0;
    double frameRate = 0.0;
    int32_t profile = 0;
    int32_t flag = 0;
    int32_t characteristics = 0;
    int32_t coefficients = 0;
    int32_t mode = 0;
    int64_t bitrate = 0;
    int32_t primaries = 0;
    int32_t videoIsHdrvivid = 0;
    const char* mimeType = nullptr;
    double sar = 0.0;
    int32_t width = 3840;
    int32_t height = 2160;
    int32_t framerateActual = 30;
    int32_t bitrateActual = 24863756;
    int32_t rotationActual = 180;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_WIDTH, &currentWidth));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_HEIGHT, &currentHeight));
    ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_ROTATION, &rotation));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(paramFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_PROFILE, &profile));
    ASSERT_TRUE(OH_AVFormat_GetLongValue(paramFormat, OH_MD_KEY_BITRATE, &bitrate));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_ENCODE_BITRATE_MODE, &mode));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_RANGE_FLAG, &flag));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_TRANSFER_CHARACTERISTICS, &characteristics));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_MATRIX_COEFFICIENTS, &coefficients));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, &videoIsHdrvivid));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_COLOR_PRIMARIES, &primaries));
    ASSERT_FALSE(OH_AVFormat_GetDoubleValue(paramFormat, OH_MD_KEY_VIDEO_SAR, &sar));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(paramFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_AVC));
    ASSERT_EQ(width, currentWidth);
    ASSERT_EQ(height, currentHeight);
    ASSERT_EQ(framerateActual, frameRate);
    ASSERT_EQ(bitrateActual, bitrate);
    ASSERT_EQ(rotationActual, rotation);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4400
 * @tc.name      : demux hevc ts video
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, SUB_MEDIA_DEMUXER_PROCESS_4400, TestSize.Level0)
{
    int tarckType = 0;
    const char *file = "/data/test/media/test_265_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    SetAllParam(sourceFormat);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    const char* mimeType = nullptr;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    OH_AVCodecBufferAttr attr;
    int vKeyCount = 0;
    int aKeyCount = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD) && index == MEDIA_TYPE_AUD) ||
             (videoIsEnd && (tarckType == MEDIA_TYPE_VID) && index == MEDIA_TYPE_VID)) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
                SetAudioParam(trackFormat);
                ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_AAC));
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                HevcVideoParam(trackFormat);
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
        }
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4500
 * @tc.name      : demux avc ts video
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, SUB_MEDIA_DEMUXER_PROCESS_4500, TestSize.Level0)
{
    int tarckType = 0;
    const char *file = "/data/test/media/test_264_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    SetAllParam(sourceFormat);
    const char* mimeType = nullptr;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    OH_AVCodecBufferAttr attr;
    int vKeyCount = 0;
    int aKeyCount = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD) && index == MEDIA_TYPE_AUD)
             || (videoIsEnd && (tarckType == MEDIA_TYPE_VID) && index == MEDIA_TYPE_VID)) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
                SetAudioParam(trackFormat);
                ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_AAC));
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                AvcVideoParam(trackFormat);
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
        }
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4510
 * @tc.name      : demux avc ios video, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, SUB_MEDIA_DEMUXER_PROCESS_4510, TestSize.Level0)
{
    int tarckType = 0;
    const char *file = "/data/test/media/record_from_ios.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(5, g_trackCount);
    SetOtherAllParam(sourceFormat);
    const char* mimeType = nullptr;
    for (int32_t index = 0; index < 2; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    OH_AVCodecBufferAttr attr;
    int vKeyCount = 0;
    int aKeyCount = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < 2; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD) && index == MEDIA_TYPE_AUD)
             || (videoIsEnd && (tarckType == MEDIA_TYPE_VID) && index == MEDIA_TYPE_VID)) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
                SetOtherAudioParam(trackFormat);
                ASSERT_EQ(0, strcmp(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_AAC));
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                OtherVideoParam(trackFormat);
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
        }
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : VIDEO_DEMUXER_VVC_0500
 * @tc.name      : demuxer 8bit H266 MP4 file, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, VIDEO_DEMUXER_VVC_0500, TestSize.Level0)
{
    if (access(g_vvc8bitPath.c_str(), F_OK) != 0) {
        return;
    }
    int64_t duration = 0;
    int64_t startTime;
    int fd = open(g_vvc8bitPath.c_str(), O_RDONLY);
    int64_t size = GetFileSize(g_vvc8bitPath.c_str());
    cout << g_vvc8bitPath.c_str() << "---------" << fd << "----------" << size <<endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(10000000, duration);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_START_TIME, &startTime));
    ASSERT_EQ(0, startTime);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    CheckAudioKeyVvc();
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : VIDEO_DEMUXER_VVC_0600
 * @tc.name      : demuxer 10bit H266 MP4 file, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, VIDEO_DEMUXER_VVC_0600, TestSize.Level0)
{
    if (access(g_vvc10bitPath.c_str(), F_OK) != 0) {
        return;
    }
    int64_t duration = 0;
    int64_t startTime;
    int tarckType = 0;
    int fd = open(g_vvc10bitPath.c_str(), O_RDONLY);
    int64_t size = GetFileSize(g_vvc10bitPath.c_str());
    cout << g_vvc10bitPath.c_str() << "---------" << fd << "----------" << size <<endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(60000000, duration);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_START_TIME, &startTime));
    ASSERT_EQ(0, startTime);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, 0);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        if (tarckType == MEDIA_TYPE_VID) {
            CheckVideoKey();
        } else if (tarckType == MEDIA_TYPE_AUD) {
            CheckAudioKey();
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0010
 * @tc.name      : demux mp3 file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0010, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gbk.mp3";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "bom"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "a"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0020
 * @tc.name      : demux mp3 file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0020, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb2312.mp3";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "bom"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "a"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0030
 * @tc.name      : demux mp3 file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0030, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb18030.mp3";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "bom"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "a"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0040
 * @tc.name      : demux flac file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0040, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gbk.flac";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0050
 * @tc.name      : demux flac file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0050, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb2312.flac";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0060
 * @tc.name      : demux flac file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0060, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb18030.flac";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0070
 * @tc.name      : demux flv file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0070, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gbk.flv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0080
 * @tc.name      : demux flv file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0080, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb2312.flv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0090
 * @tc.name      : demux flv file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0090, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb18030.flv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0100
 * @tc.name      : demux m4a file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0100, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gbk.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0110
 * @tc.name      : demux m4a file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0110, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb2312.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0120
 * @tc.name      : demux m4a file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0120, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb18030.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0130
 * @tc.name      : demux mkv file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0130, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gbk.mkv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0140
 * @tc.name      : demux mkv file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0140, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb2312.mkv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0150
 * @tc.name      : demux mkv file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0150, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb18030.mkv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0160
 * @tc.name      : demux mov file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0160, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gbk.mov";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0170
 * @tc.name      : demux mov file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0170, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb2312.mov";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0180
 * @tc.name      : demux mov file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0180, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb18030.mov";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0190
 * @tc.name      : demux mp4 file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0190, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gbk.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0200
 * @tc.name      : demux mp4 file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0200, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb2312.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0210
 * @tc.name      : demux mp4 file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0210, TestSize.Level2)
{
    const char* title = nullptr;
    const char *file = "/data/test/media/gb18030.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "张三"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0220
 * @tc.name      : demux wav file with gbk, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0220, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gbk.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0230
 * @tc.name      : demux wav file with gb2312, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0230, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb2312.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_GBK_0240
 * @tc.name      : demux wav file with gb18030, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFormatNdkTest, DEMUXER_GBK_0240, TestSize.Level2)
{
    const char* artist = nullptr;
    const char* album = nullptr;
    const char* title = nullptr;
    const char *file = "/data/test/media/audio/gb18030.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &title));
    ASSERT_EQ(0, strcmp(title, "音乐"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ARTIST, &artist));
    ASSERT_EQ(0, strcmp(artist, "张三"));
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM, &album));
    ASSERT_EQ(0, strcmp(album, "风景"));
    close(fd);
    fd = -1;
}