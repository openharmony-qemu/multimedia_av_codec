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
class DemuxerProc2NdkTest : public testing::Test {
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
constexpr uint32_t ACTUAL_AUDIOFORMAT = 9;
constexpr uint32_t ACTUAL_AUDIOCOUNT = 2;
constexpr uint64_t ACTUAL_LAYOUT = 3;
constexpr uint32_t ACTUAL_SAMPLERATE = 44100;
constexpr uint32_t ACTUAL_CODEDSAMPLE = 16;

void DemuxerProc2NdkTest::SetUpTestCase() {}
void DemuxerProc2NdkTest::TearDownTestCase() {}
void DemuxerProc2NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerProc2NdkTest::TearDown()
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
 * @tc.number    : SUB_MEDIA_DEMUXER_PROCESS_4510
 * @tc.name      : demux avc pg video
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerProc2NdkTest, SUB_MEDIA_DEMUXER_PROCESS_4510, TestSize.Level0)
{
    int tarckType = 0;
    const char *file = "/data/test/media/01_1.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
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
        }
    }
    close(fd);
}
