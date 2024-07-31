/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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
class DemuxerProcNdkTest : public testing::Test {
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
constexpr uint32_t AVC_ROTATION = 270;
constexpr uint32_t HEVC_ROTATION = 90;
constexpr int32_t LAYOUTMONO = 4;
constexpr int32_t LAYOUTDUAL = 3;
constexpr int32_t SAMPLERATEMONO = 8000;
constexpr int32_t SAMPLERATEDUAL = 44100;
constexpr int32_t COUNTMONO = 1;
constexpr int32_t COUNTDUAL = 2;
constexpr int32_t BITRATEMONO = 64000;
constexpr int32_t BITRATEDUAL = 705600;
constexpr int32_t FRAME_REMAINING = 100;
void DemuxerProcNdkTest::SetUpTestCase() {}
void DemuxerProcNdkTest::TearDownTestCase() {}
void DemuxerProcNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerProcNdkTest::TearDown()
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
static void CheckAudioParam(OH_AVSource *audioSource, int &audioFrameAll)
{
    int akeyCount = 0;
    int tarckType = 0;
    OH_AVCodecBufferAttr bufferAttr;
    bool audioIsEnd = false;
    int32_t count = 0;
    int32_t rate = 0;
    int64_t bitrate = 0;
    int64_t layout = 0;
    int32_t index = 0;
    const char* mimeType = nullptr;
    while (!audioIsEnd) {
        trackFormat = OH_AVSource_GetTrackFormat(audioSource, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
        ASSERT_NE(avBuffer, nullptr);
        ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
        if (tarckType == MEDIA_TYPE_AUD) {
            ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &rate));
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &count));
            ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_CHANNEL_LAYOUT, &layout));
            ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitrate));
            if (bufferAttr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                audioIsEnd = true;
                cout << audioFrameAll << "    audio is end !!!!!!!!!!!!!!!" << endl;
                continue;
            }
            audioFrameAll++;
            if (bufferAttr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                akeyCount++;
            }
        }
    }
    if (count == 1) {
        ASSERT_EQ(0, strcmp(mimeType, "audio/g711mu"));
        ASSERT_EQ(layout, LAYOUTMONO);
        ASSERT_EQ(rate, SAMPLERATEMONO);
        ASSERT_EQ(count, COUNTMONO);
        ASSERT_EQ(bitrate, BITRATEMONO);
    } else {
        ASSERT_EQ(0, strcmp(mimeType, "audio/g711mu"));
        ASSERT_EQ(layout, LAYOUTDUAL);
        ASSERT_EQ(rate, SAMPLERATEDUAL);
        ASSERT_EQ(count, COUNTDUAL);
        ASSERT_EQ(bitrate, BITRATEDUAL);
    }
    cout << akeyCount << "---akeyCount---" << endl;
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
    ASSERT_EQ(0, strcmp(mimeType, "video/avc"));
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
    ASSERT_EQ(0, strcmp(mimeType, "video/hevc"));
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

static void IsHdrVivid(OH_AVFormat *paramFormat)
{
    int32_t videoIsHdrvivid;
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_TRUE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, &videoIsHdrvivid));
        ASSERT_EQ(1, videoIsHdrvivid);
    } else {
        ASSERT_FALSE(OH_AVFormat_GetIntValue(paramFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, &videoIsHdrvivid));
    }
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1400
 * @tc.name      : demuxer video and 2 audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1400, TestSize.Level0)
{
    int tarckType = 0;
    int auidoTrackCount = 2;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/video_2audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(auidoTrackCount + 1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount[2] = {};
    int audioFrame[2] = {};
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame[index-1], aKeyCount[index-1]);
            }
        }
    }
    for (int index = 0; index < auidoTrackCount; index++) {
        ASSERT_EQ(audioFrame[index], 433);
        ASSERT_EQ(aKeyCount[index], 433);
    }
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1500
 * @tc.name      : demuxer video and 9 audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1500, TestSize.Level0)
{
    int tarckType = 0;
    int auidoTrackCount = 9;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/video_9audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(auidoTrackCount + 1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount[9] = {};
    int audioFrame[9] = {};
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame[index-1], aKeyCount[index-1]);
            }
        }
    }
    for (int index = 0; index < auidoTrackCount; index++) {
        ASSERT_EQ(audioFrame[index], 433);
        ASSERT_EQ(aKeyCount[index], 433);
    }
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1600
 * @tc.name      : demuxer avc+MP3 flv video file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1600, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/avc_mp3.flv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 385);
    ASSERT_EQ(aKeyCount, 385);
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1700
 * @tc.name      : demuxer hevc+pcm flv video file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1700, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/hevc_pcm_a.flv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 385);
    ASSERT_EQ(aKeyCount, 385);
    ASSERT_EQ(videoFrame, 602);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1800
 * @tc.name      : demuxer damaged flv video file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1800, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/avc_mp3_error.flv";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_1900
 * @tc.name      : demuxer damaged ape audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_1900, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/ape.ape";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    string mimeTypeString = mimeType;
    string apeString = OH_AVCODEC_MIMETYPE_AUDIO_APE;
    cout << "------mimeType-------" << mimeTypeString << endl;
    ASSERT_EQ(mimeTypeString, apeString);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int aKeyCount = 0;
    while (!audioIsEnd) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
    }
    ASSERT_EQ(audioFrame, 8);
    ASSERT_EQ(aKeyCount, 8);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2000
 * @tc.name      : demuxer h264+mp3 fmp4 file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2000, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/h264_mp3_3mevx_fmp4.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 465);
    ASSERT_EQ(aKeyCount, 465);
    ASSERT_EQ(videoFrame, 369);
    ASSERT_EQ(vKeyCount, 3);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2100
 * @tc.name      : demuxer h265+aac fmp4 file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2100, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/h265_aac_1mvex_fmp4.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 173);
    ASSERT_EQ(aKeyCount, 173);
    ASSERT_EQ(videoFrame, 242);
    ASSERT_EQ(vKeyCount, 1);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2200
 * @tc.name      : demuxer HDRVivid+AudioVivid fmp4 file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2200, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/audiovivid_hdrvivid_1s_fmp4.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(videoFrame, 26);
    ASSERT_EQ(vKeyCount, 1);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2300
 * @tc.name      : demuxer M4A fmp4 file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2300, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/m4a_fmp4.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int aKeyCount = 0;
    while (!audioIsEnd) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
    }
    ASSERT_EQ(audioFrame, 352);
    ASSERT_EQ(aKeyCount, 352);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2400
 * @tc.name      : demuxer M4V fmp4 file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2400, TestSize.Level0)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *file = "/data/test/media/m4v_fmp4.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    int aKeyCount = 0;
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 176);
    ASSERT_EQ(aKeyCount, 176);
    ASSERT_EQ(videoFrame, 123);
    ASSERT_EQ(vKeyCount, 1);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2500
 * @tc.name      : create hls demuxer with error uri
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2500, TestSize.Level0)
{
    const char *uri = "http://192.168.3.11:8080/share/index.m3u8";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(nullptr, source);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2600
 * @tc.name      : create str demuxer with file and read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2600, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    int srtIndex = 1;
    int srtSubtitle = 0;
    const char *file = "/data/test/media/srt_test.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    string mimeTypeString = mimeType;
    string srtString = OH_AVCODEC_MIMETYPE_SUBTITLE_SRT;
    cout << "------mimeType-------" << mimeTypeString << endl;
    ASSERT_EQ(mimeTypeString, srtString);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   srt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        srtSubtitle = atoi(reinterpret_cast<const char*>(data));
        cout << "subtitle" << "----------------" << srtSubtitle << "-----------------" << endl;
        ASSERT_EQ(srtSubtitle, srtIndex);
        srtIndex++;
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2700
 * @tc.name      : create str demuxer with file and seek+read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2700, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    int srtIndex = 1;
    int srtSubtitle = 0;
    uint8_t *data = nullptr;
    const char *file = "/data/test/media/srt_test.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    string mimeTypeString = mimeType;
    string srtString = OH_AVCODEC_MIMETYPE_SUBTITLE_SRT;
    ASSERT_EQ(mimeTypeString, srtString);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    for (int index = 0; index < 5; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        data = OH_AVMemory_GetAddr(memory);
        srtSubtitle = atoi(reinterpret_cast<const char*>(data));
        ASSERT_EQ(srtSubtitle, srtIndex);
        srtIndex++;
    }
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, 5400, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
    data = OH_AVMemory_GetAddr(memory);
    srtSubtitle = atoi(reinterpret_cast<const char*>(data));
    srtIndex = 2;
    ASSERT_EQ(srtSubtitle, srtIndex);
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            break;
        }
        data = OH_AVMemory_GetAddr(memory);
        srtSubtitle = atoi(reinterpret_cast<const char*>(data));
        srtIndex++;
        ASSERT_EQ(srtSubtitle, srtIndex);
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2800
 * @tc.name      : create str demuxer with error file -- no empty paragraphs
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2800, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/srt_2800.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    string mimeTypeString = mimeType;
    string srtString = OH_AVCODEC_MIMETYPE_SUBTITLE_SRT;
    cout << "------mimeType-------" << mimeTypeString << endl;
    ASSERT_EQ(mimeTypeString, srtString);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   srt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    }

    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_2900
 * @tc.name      : create str demuxer with error file -- subtitle sequence error
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_2900, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/srt_2900.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    string mimeTypeString = mimeType;
    string srtString = OH_AVCODEC_MIMETYPE_SUBTITLE_SRT;
    cout << "------mimeType-------" << mimeTypeString << endl;
    ASSERT_EQ(mimeTypeString, srtString);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   srt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle" << "----------------" << data << "-----------------" << endl;
    }

    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3000
 * @tc.name      : create str demuxer with error file -- timeline format error
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3000, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/srt_3000.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    cout << "g_trackCount"<< "----------------" << g_trackCount << "-----------------" << endl;
    OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr);
    uint8_t *data = OH_AVMemory_GetAddr(memory);
    cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3100
 * @tc.name      : create str demuxer with error file -- subtitle is empty
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3100, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/srt_3100.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    string mimeTypeString = mimeType;
    string srtString = OH_AVCODEC_MIMETYPE_SUBTITLE_SRT;
    cout << "------mimeType-------" << mimeTypeString << endl;
    ASSERT_EQ(mimeTypeString, srtString);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   srt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    }

    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3200
 * @tc.name      : create str demuxer with error file -- SRT file is empty
 * @tc.desc      : function test
 * fail
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3200, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char *file = "/data/test/media/srt_3200.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount);
    cout << "g_trackCount"<< "----------------" << g_trackCount << "-----------------" << endl;
    OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr);
    uint8_t *data = OH_AVMemory_GetAddr(memory);
    cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3300
 * @tc.name      : create str demuxer with error file -- alternating Up and Down Times
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3300, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char* mimeType = nullptr;
    const char *file = "/data/test/media/srt_3300.srt";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    string mimeTypeString = mimeType;
    string srtString = OH_AVCODEC_MIMETYPE_SUBTITLE_SRT;
    cout << "------mimeType-------" << mimeTypeString << endl;
    ASSERT_EQ(mimeTypeString, srtString);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    while (true) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
            cout << "   srt is end !!!!!!!!!!!!!!!" << endl;
            break;
        }
        uint8_t *data = OH_AVMemory_GetAddr(memory);
        cout << "subtitle"<< "----------------" << data << "-----------------" << endl;
    }

    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3400
 * @tc.name      : demuxer MP4 ,OH_MD_KEY_DURATION,OH_MD_KEY_CODEC_CONFIG
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3400, TestSize.Level0)
{
    int64_t duration;
    static OH_AVFormat *trackFormatFirst = nullptr;
    static OH_AVFormat *trackFormatSecond = nullptr;
    uint8_t *codecConfig = nullptr;
    double frameRate;
    int32_t rotation;
    int64_t channelLayout;
    int32_t audioSampleFormat;
    int32_t bitsPreCodedSample;
    int32_t profile;
    int32_t colorPrimaries;
    int32_t videoIsHdrvivid;
    size_t bufferSize;
    const char *file = "/data/test/media/01_video_audio.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    trackFormatFirst = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormatFirst, nullptr);
    trackFormatSecond = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormatSecond, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(duration, 10032000);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(trackFormatSecond, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize));
    ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormatSecond, OH_MD_KEY_FRAME_RATE, &frameRate));
    ASSERT_EQ(frameRate, 25.1);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormatSecond, OH_MD_KEY_ROTATION, &rotation));
    ASSERT_EQ(rotation, 0);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormatFirst, OH_MD_KEY_CHANNEL_LAYOUT, &channelLayout));
    ASSERT_EQ(channelLayout, 3);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormatFirst, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &audioSampleFormat));
    ASSERT_EQ(audioSampleFormat, 9);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormatFirst, OH_MD_KEY_BITS_PER_CODED_SAMPLE, &bitsPreCodedSample));
    ASSERT_EQ(bitsPreCodedSample, 16);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormatFirst, OH_MD_KEY_PROFILE, &profile));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormatFirst, OH_MD_KEY_COLOR_PRIMARIES, &colorPrimaries));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormatFirst, OH_MD_KEY_VIDEO_IS_HDR_VIVID, &videoIsHdrvivid));
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3500
 * @tc.name      : demuxer MP4 ,startTime
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3500, TestSize.Level0)
{
    int64_t startTime;
    const char *file = "/data/test/media/test_265_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_START_TIME, &startTime));
    ASSERT_EQ(0, startTime);
    close(fd);
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3600
 * @tc.name      : demuxer MP4 ,SAR,bitsPreCodedSample,sampleFormat
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3600, TestSize.Level0)
{
    int tarckType = 0;
    double sar;
    int32_t bitsPreCodedSample;
    int32_t sampleFormat;
    const char *file = "/data/test/media/test_265_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        if (tarckType == MEDIA_TYPE_VID) {
            ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_VIDEO_SAR, &sar));
        }else if (tarckType == MEDIA_TYPE_AUD) {
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_BITS_PER_CODED_SAMPLE, &bitsPreCodedSample));
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &sampleFormat));
        }
    }
    ASSERT_EQ(1, sar);
    ASSERT_EQ(16, bitsPreCodedSample);
    ASSERT_EQ(9, sampleFormat);
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3700
 * @tc.name      : demuxer MP4,duration,dts
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3700, TestSize.Level0)
{
    int tarckType = 0;
    int64_t duration;
    int64_t dts;
    const char *file = "/data/test/media/test_265_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        if (tarckType == MEDIA_TYPE_VID) {
            OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer);
            ASSERT_NE(avBuffer, nullptr);
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetLongValue(format, OH_MD_KEY_BUFFER_DURATION, &duration));
            ASSERT_TRUE(OH_AVFormat_GetLongValue(format, OH_MD_KEY_DECODING_TIMESTAMP, &dts));
            ASSERT_EQ(40000, duration);
            ASSERT_EQ(-80000, dts);
        }
    }
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_3800
 * @tc.name      : demuxer MP4 ,AVCODEC_BUFFER_FLAGS_DISCARD
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_3800, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    int tarckType = 0;
    const char *file = "/data/test/media/test_265_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int audioFrame = 0;
    bool audioIsEnd = false;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_AUD && (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_DISCARD)) {
                audioIsEnd = true;
                cout << audioFrame << "    audio is end !!!!!!!!!!!!!!!" << endl;
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MP3_TITLE_RESOLUTION_4100
 * @tc.name      : audio resolution with fffe mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MP3_TITLE_RESOLUTION_4100, TestSize.Level0)
{
    const char *stringVal;
    const char *file = "/data/test/media/audio/fffe_bom.mp3";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    cout << "title" << "----------------------" << stringVal << "---------" << endl;
    ASSERT_EQ(0, strcmp(stringVal, "bom"));
    close(fd);
}

/**
 * @tc.number    : SUB_MP3_TITLE_RESOLUTION_4200
 * @tc.name      : audio resolution with feff mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MP3_TITLE_RESOLUTION_4200, TestSize.Level0)
{
    const char *stringVal;
    const char *file = "/data/test/media/audio/feff_bom.mp3";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    cout << "title" << "----------------------" << stringVal << "---------" << endl;
    ASSERT_EQ(0, strcmp(stringVal, "bom"));
    close(fd);
}

/**
 * @tc.number    : SUB_MP3_TITLE_RESOLUTION_4300
 * @tc.name      : audio resolution non_standard mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MP3_TITLE_RESOLUTION_4300, TestSize.Level0)
{
    const char *stringVal;
    const char *file = "/data/test/media/audio/nonstandard_bom.mp3";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    cout << "title" << "----------------------" << stringVal << "---------" << endl;
    ASSERT_EQ(0, strcmp(stringVal, "bom"));
    close(fd);
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4400
 * @tc.name      : demux hevc ts video
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_4400, TestSize.Level0)
{
    int tarckType = 0;
    const char *file = "/data/test/media/test_265_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
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
                ASSERT_EQ(0, strcmp(mimeType, "audio/mp4a-latm"));
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                HevcVideoParam(trackFormat);
            }
        }
    }
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4500
 * @tc.name      : demux avc ts video
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_4500, TestSize.Level0)
{
    int tarckType = 0;
    const char *file = "/data/test/media/test_264_B_Gop25_4sec.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
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
                ASSERT_EQ(0, strcmp(mimeType, "audio/mp4a-latm"));
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                AvcVideoParam(trackFormat);
            }
        }
    }
    close(fd);
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4600
 * @tc.name      : demuxer AVC MP4 ,OH_MD_KEY_DURATION,OH_MD_KEY_CODEC_CONFIG
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_4600, TestSize.Level0)
{
    int tarckType = 0;
    uint8_t *codecConfig = nullptr;
    int32_t rotation;
    int32_t videoIsHdrvivid;
    size_t bufferSize;
    const char *file = "/data/test/media/single_rk.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
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
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_TRUE(OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_ROTATION, &rotation));
                ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_VIDEO_IS_HDR_VIVID, &videoIsHdrvivid));
            }
        }
    }
    ASSERT_EQ(AVC_ROTATION, rotation);
    close(fd);
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4700
 * @tc.name      : demuxer HEVC MP4 ,OH_MD_KEY_DURATION,OH_MD_KEY_CODEC_CONFIG
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_4700, TestSize.Level0)
{
    int tarckType = 0;
    uint8_t *codecConfig = nullptr;
    int32_t rotation;
    size_t bufferSize;
    const char *file = "/data/test/media/single_60.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
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
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_TRUE(OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
                ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_ROTATION, &rotation));
                IsHdrVivid(trackFormat);
            }
        }
    }
    ASSERT_EQ(HEVC_ROTATION, rotation);
    close(fd);
}
/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_6200
 * @tc.name      : create pcm-mulaw wav demuxer with file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_6200, TestSize.Level2)
{
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/wav_audio_test_202406290859.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    CheckAudioParam(source, audioFrame);
    ASSERT_EQ(103, audioFrame);
    cout << "-----------audioFrame-----------" << audioFrame << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_6400
 * @tc.name      : create pcm-mulaw wav demuxer with Mono channel file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_6400, TestSize.Level2)
{
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/7FBD5E21-503C-41A8-83B4-34548FC01562.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    CheckAudioParam(source, audioFrame);
    ASSERT_EQ(7, audioFrame);
    cout << "-----------audioFrame-----------" << audioFrame << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_6600
 * @tc.name      : create pcm+mulaw wav demuxer with file and forward back seek+read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_6600, TestSize.Level0)
{
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/wav_audio_test_202406290859.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    int time = 4600000;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, time/1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, avBuffer));
    time = 92000;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, time/1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, avBuffer));
    CheckAudioParam(source, audioFrame);
    ASSERT_EQ(FRAME_REMAINING, audioFrame);
    cout << "-----------audioFrame-----------" << audioFrame << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_6700
 * @tc.name      : create pcm+mulaw wav demuxer with file and back seek+read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_6700, TestSize.Level0)
{
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/wav_audio_test_202406290859.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    int time = 4736000;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, time/1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, avBuffer));
    time = 600000;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, time/1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, avBuffer));
    time = 92000;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, time/1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, avBuffer));
    CheckAudioParam(source, audioFrame);
    ASSERT_EQ(FRAME_REMAINING, audioFrame);
    cout << "-----------audioFrame-----------" << audioFrame << endl;
    close(fd);
}

/**
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_6800
 * @tc.name      : create pcm+mulaw wav demuxer with file and forward seek+read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProcNdkTest, SUB_MEDIA_DEMUXER_PROCESS_6800, TestSize.Level0)
{
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/wav_audio_test_202406290859.wav";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    int tarckType = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
    int time = 92000;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, time/1000, SEEK_MODE_CLOSEST_SYNC));
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, 0, avBuffer));
    CheckAudioParam(source, audioFrame);
    ASSERT_EQ(FRAME_REMAINING, audioFrame);
    cout << "-----------audioFrame-----------" << audioFrame << endl;
    close(fd);
}
