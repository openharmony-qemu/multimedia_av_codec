/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
#include "avcodec_audio_channel_layout.h"
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
class DemuxerCafFuncNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
protected:
    const char *INP_DIR_1 = "/data/test/media/audio/ac3.caf";
    const char *INP_DIR_2 = "/data/test/media/audio/error.caf";
    const char *INP_DIR_3 = "/data/test/media/audio/adpcm_ima_wav.caf";
    const char *INP_DIR_4 = "/data/test/media/audio/adpcm_ms.caf";
    const char *INP_DIR_5 = "/data/test/media/audio/alac.caf";
    const char *INP_DIR_6 = "/data/test/media/audio/amr_nb.caf";
    const char *INP_DIR_7 = "/data/test/media/audio/flac.caf";
    const char *INP_DIR_8 = "/data/test/media/audio/gsm.caf";
    const char *INP_DIR_9 = "/data/test/media/audio/gsm_ms.caf";
    const char *INP_DIR_10 = "/data/test/media/audio/ilbc.caf";
    const char *INP_DIR_11 = "/data/test/media/audio/mp2.caf";
    const char *INP_DIR_12 = "/data/test/media/audio/mp3.caf";
    const char *INP_DIR_13 = "/data/test/media/audio/opus.caf";
    const char *INP_DIR_14 = "/data/test/media/audio/pcm_alaw.caf";
    const char *INP_DIR_15 = "/data/test/media/audio/pcm_mulaw.caf";
    const char *INP_DIR_16 = "/data/test/media/audio/pcm_s16be.caf";
    const char *INP_DIR_17 = "/data/test/media/audio/caf_adpcm_ima_qt.caf";
    const char *INP_DIR_18 = "/data/test/media/audio/caf_pcm_f32be.caf";
    const char *INP_DIR_19 = "/data/test/media/audio/caf_pcm_f32le.caf";
    const char *INP_DIR_20 = "/data/test/media/audio/caf_pcm_f64be.caf";
    const char *INP_DIR_21 = "/data/test/media/audio/caf_pcm_f64le.caf";
    const char *INP_DIR_22 = "/data/test/media/audio/caf_pcm_s8.caf";
    const char *INP_DIR_23 = "/data/test/media/audio/caf_pcm_s16le.caf";
    const char *INP_DIR_24 = "/data/test/media/audio/caf_pcm_s24be.caf";
    const char *INP_DIR_25 = "/data/test/media/audio/caf_pcm_s24le.caf";
    const char *INP_DIR_26 = "/data/test/media/audio/caf_pcm_s32be.caf";
    const char *INP_DIR_27 = "/data/test/media/audio/caf_pcm_s32le.caf";
};

static int g_fd = -1;
static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
static OH_AVErrCode ret = AV_ERR_OK;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
constexpr int32_t THOUSAND = 1000.0;
constexpr int32_t ONE = 1;
void DemuxerCafFuncNdkTest::SetUpTestCase() {}
void DemuxerCafFuncNdkTest::TearDownTestCase() {}
void DemuxerCafFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerCafFuncNdkTest::TearDown()
{
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }

    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
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
    if (g_fd > 0) {
        close(g_fd);
        g_fd = -1;
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

struct seekInfo {
    const char *fileName;
    OH_AVSeekMode seekmode;
    int64_t millisecond;
    int32_t audioCount;
    int32_t videoCount;
};

static void CheckSeekMode(seekInfo seekInfo)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    g_fd = open(seekInfo.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo.fileName);
    cout << seekInfo.fileName << "-------" << g_fd << "-------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
        bool readEnd = false;
        int32_t frameNum = 0;
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                readEnd = true;
                break;
            }
            frameNum++;
        }
        if (tarckType == MEDIA_TYPE_VID) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(seekInfo.videoCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_AUD) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(seekInfo.audioCount, frameNum);
        }
    }
}

static void CheckSeekModeCombination(seekInfo seekInfo1, seekInfo secondSeekInfo)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    g_fd = open(seekInfo1.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo1.fileName);
    cout << seekInfo1.fileName << "-------" << g_fd << "-------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "g_trackCount----" << g_trackCount << endl;
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo1.millisecond / THOUSAND, seekInfo1.seekmode));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, secondSeekInfo.millisecond / THOUSAND,
                                                     secondSeekInfo.seekmode));
        bool readEnd = false;
        int32_t frameNum = 0;
        while (!readEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                readEnd = true;
                break;
            }
            frameNum++;
        }
        if (tarckType == MEDIA_TYPE_VID) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(secondSeekInfo.videoCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_AUD) {
            cout << "frameNum---" << frameNum << endl;
            ASSERT_EQ(secondSeekInfo.audioCount, frameNum);
        }
    }
    close(g_fd);
    g_fd = -1;
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
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            vKeyCount++;
        }
    }
}

static void DemuxerResult(const char *file, int aFrames, int vFrames)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    g_fd = open(file, O_RDONLY);
    if (g_fd < 0) {
        return;
    }
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, ONE);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    int vKeyCount = 0;
    while (!audioIsEnd && !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            }
        }
    }
    close(g_fd);
    g_fd = -1;
    ASSERT_EQ(audioFrame, aFrames);
    ASSERT_EQ(videoFrame, vFrames);
}

static void DemuxerFile(const char *file)
{
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0100
 * @tc.name      : create_source_with_fd_ac3_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0100, TestSize.Level2)
{
    DemuxerResult(INP_DIR_1, 158, 0);
}
/**
 * @tc.number    : DEMUXER_CAF_FUNC_0200
 * @tc.name      : demuxer_caf_get_source_format_track_count
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0200, TestSize.Level2)
{
    DemuxerFile(INP_DIR_1);
    ASSERT_EQ(g_trackCount, ONE);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0300
 * @tc.name      : demuxer_caf_get_public_track_format_mime
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0300, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    int tarckType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &stringVal));
        if (tarckType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(0, strcmp(stringVal, OH_AVCODEC_MIMETYPE_AUDIO_AC3));
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0400
 * @tc.name      : demuxer_caf_get_audio_track_format_sample_rate
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0400, TestSize.Level2)
{
    int32_t sr = 0;
    DemuxerFile(INP_DIR_1);
    int tarckType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
        if (tarckType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(sr, 48000);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0500
 * @tc.name      : demuxer_caf_get_audio_track_format_bitrate
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0500, TestSize.Level2)
{
    int64_t br = 0;
    DemuxerFile(INP_DIR_1);
    int tarckType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
        if (tarckType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(br, 96000);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0600
 * @tc.name      : demuxer_caf_get_audio_track_format_channel_count
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0600, TestSize.Level2)
{
    int32_t cc = 0;
    DemuxerFile(INP_DIR_1);
    int tarckType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
        if (tarckType == MEDIA_TYPE_AUD) {
            ASSERT_EQ(cc, ONE);
        }
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0700
 * @tc.name      : demuxer_caf_check_source_format_title
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0700, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0800
 * @tc.name      : demuxer_caf_check_source_format_album_artist
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0800, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_0900
 * @tc.name      : demuxer_caf_check_track_format_date
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_0900, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    ASSERT_EQ(0, strcmp(stringVal, "2015-11-23"));
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1000
 * @tc.name      : demuxer_caf_check_track_format_comment
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1000, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1100
 * @tc.name      : demuxer_caf_check_track_format_genre
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1100, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_GENRE, &stringVal));
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1200
 * @tc.name      : demuxer_caf_check_track_format_copyright
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1200, TestSize.Level2)
{
    const char *stringVal;
    DemuxerFile(INP_DIR_1);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COPYRIGHT, &stringVal));
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1300
 * @tc.name      : demuxer_caf_get_public_track_format_channel_layout
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1300, TestSize.Level2)
{
    int64_t channelLayout;
    DemuxerFile(INP_DIR_1);
    int tarckType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        if (tarckType == MEDIA_TYPE_VID) {
            continue;
        }
        ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_CHANNEL_LAYOUT, &channelLayout));
        ASSERT_EQ(MONO, channelLayout);
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1400
 * @tc.name      : demuxer_caf_get_public_track_format_audio_sample_format
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1400, TestSize.Level2)
{
    int32_t sampleFormat;
    DemuxerFile(INP_DIR_1);
    int tarckType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        if (tarckType == MEDIA_TYPE_VID) {
            continue;
        }
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, &sampleFormat));
        int sampleFormatResult = 9;
        ASSERT_EQ(sampleFormatResult, sampleFormat);
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1500
 * @tc.name      : demuxer_caf_get_public_track_format_bits_per_coded_sample
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1500, TestSize.Level2)
{
    int32_t bitsPreCodedSample;
    DemuxerFile(INP_DIR_1);
    int tarckType = 0;
    for (int index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        if (tarckType == MEDIA_TYPE_VID) {
            continue;
        }
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_BITS_PER_CODED_SAMPLE, &bitsPreCodedSample));
        int bitsPreCodedSampleResult = 0;
        ASSERT_EQ(bitsPreCodedSampleResult, bitsPreCodedSample);
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1600
 * @tc.name      : seek to the start time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 158, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1700
 * @tc.name      : seek to the start time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 0, 158, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1800
 * @tc.name      : seek to the start time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 0, 158, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_1900
 * @tc.name      : seek to the end time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_1900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 4990000, 3, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2000
 * @tc.name      : seek to the end time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 4990000, 3, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2100
 * @tc.name      : seek to the end time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 4990000, 3, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2200
 * @tc.name      : seek to the middle time, 5000000 millisecond, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 5000000, 2, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2300
 * @tc.name      : seek to the middle time, 5000000 millisecond, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 5000000, 2, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2400
 * @tc.name      : seek to the middle time, 5000000 millisecond, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 5000000, 2, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2500
 * @tc.name      : seek to the middle time, closest mode, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 158, 0};
    seekInfo fileTest2{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 5000000, 2, 0};
    CheckSeekModeCombination(fileTest1, fileTest2);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2600
 * @tc.name      : seek to the middle time, previous mode, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 158, 0};
    seekInfo fileTest2{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 4990000, 3, 0};
    CheckSeekModeCombination(fileTest1, fileTest2);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2700
 * @tc.name      : seek to the middle time, closest mode, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 4990000, 3, 0};
    seekInfo fileTest2{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 4990000, 3, 0};
    CheckSeekModeCombination(fileTest1, fileTest2);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2800
 * @tc.name      : demuxer_damaged_caf_file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2800, TestSize.Level2)
{
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_2900
 * @tc.name      : seek to a invalid time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_2900, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    srand(time(nullptr));
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));

    ASSERT_NE(demuxer, nullptr);
    int64_t invalidPts = 12000 * 16666;
    ret = OH_AVDemuxer_SeekToTime(demuxer, invalidPts, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_NE(ret, AV_ERR_OK);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3000
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3000, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    srand(time(nullptr));
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_UnselectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    ret = OH_AVDemuxer_SelectTrackByID(demuxer, 0);
    ASSERT_EQ(ret, AV_ERR_OK);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3100
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3100, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    const char *file = INP_DIR_1;
    bool isEnd = false;
    int count = 0;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, ONE);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    srand(time(nullptr));
    int pos = rand() % 250;
    cout << " pos= " << pos << endl;
    while (!isEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (count == pos) {
                cout << count << " count == pos!!!!!!!!!" << endl;
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 0));
                ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
                isEnd = true;
                break;
            }
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                cout << "is end !!!!!!!!!!!!!!!" << endl;
            }
            if (index == MEDIA_TYPE_AUD) {
                count++;
            }
        }
    }
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3200
 * @tc.name      : start demux before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3200, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    const char *file = INP_DIR_1;
    srand(time(nullptr));
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3300
 * @tc.name      : create_source_with_fd_adpcm_ima_wav_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3300, TestSize.Level2)
{
    DemuxerResult(INP_DIR_3, 119, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3400
 * @tc.name      : create_source_with_fd_adpcm_ms_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3400, TestSize.Level2)
{
    DemuxerResult(INP_DIR_4, 119, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3500
 * @tc.name      : create_source_with_fd_alac_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3500, TestSize.Level2)
{
    DemuxerResult(INP_DIR_5, 60, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3600
 * @tc.name      : create_source_with_fd_amr_nb_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3600, TestSize.Level2)
{
    DemuxerResult(INP_DIR_6, 253, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3700
 * @tc.name      : create_source_with_fd_flac_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3700, TestSize.Level2)
{
    DemuxerResult(INP_DIR_7, 53, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3800
 * @tc.name      : create_source_with_fd_gsm_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3800, TestSize.Level2)
{
    DemuxerResult(INP_DIR_8, 252, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_3900
 * @tc.name      : create_source_with_fd_gsm_ms_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_3900, TestSize.Level2)
{
    DemuxerResult(INP_DIR_9, 126, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4000
 * @tc.name      : create_source_with_fd_ilbc_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4000, TestSize.Level2)
{
    DemuxerResult(INP_DIR_10, 252, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4100
 * @tc.name      : create_source_with_fd_mp2_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4100, TestSize.Level2)
{
    DemuxerResult(INP_DIR_11, 210, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4200
 * @tc.name      : create_source_with_fd_mp3_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4200, TestSize.Level2)
{
    DemuxerResult(INP_DIR_12, 210, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4300
 * @tc.name      : create_source_with_fd_opus_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4300, TestSize.Level2)
{
    DemuxerResult(INP_DIR_13, 253, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4400
 * @tc.name      : create_source_with_fd_pcm_alaw_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4400, TestSize.Level2)
{
    DemuxerResult(INP_DIR_14, 60, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4500
 * @tc.name      : create_source_with_fd_pcm_mulaw_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4500, TestSize.Level2)
{
    DemuxerResult(INP_DIR_15, 10, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4600
 * @tc.name      : create_source_with_fd_pcm_s16be_caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4600, TestSize.Level2)
{
    DemuxerResult(INP_DIR_16, 119, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4700
 * @tc.name      : create source with caf_adpcm_ima_qt, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4700, TestSize.Level2)
{
    DemuxerResult(INP_DIR_17, 3750, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4800
 * @tc.name      : create source with caf_pcm_f32be, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4800, TestSize.Level2)
{
    DemuxerResult(INP_DIR_18, 235, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_4900
 * @tc.name      : create source with caf_pcm_f32le, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_4900, TestSize.Level2)
{
    DemuxerResult(INP_DIR_19, 235, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5000
 * @tc.name      : create source with caf_pcm_f64be, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5000, TestSize.Level2)
{
    DemuxerResult(INP_DIR_20, 469, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5100
 * @tc.name      : create source with caf_pcm_f64le, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5100, TestSize.Level2)
{
    DemuxerResult(INP_DIR_21, 469, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5200
 * @tc.name      : create source with caf_pcm_s8, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5200, TestSize.Level2)
{
    DemuxerResult(INP_DIR_22, 59, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5300
 * @tc.name      : create source with caf_pcm_s16le, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5300, TestSize.Level2)
{
    DemuxerResult(INP_DIR_23, 118, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5400
 * @tc.name      : seek to the middle time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 4990000, 3, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5500
 * @tc.name      : seek to the middle time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 4990000, 3, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5600
 * @tc.name      : seek to the middle time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 4990000, 3, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5700
 * @tc.name      : create source with caf_pcm_s24be, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5700, TestSize.Level2)
{
    DemuxerResult(INP_DIR_24, 176, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5800
 * @tc.name      : create source with caf_pcm_s24le, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5800, TestSize.Level2)
{
    DemuxerResult(INP_DIR_25, 176, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_5900
 * @tc.name      : create source with caf_pcm_s32be, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_5900, TestSize.Level2)
{
    DemuxerResult(INP_DIR_26, 235, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6000
 * @tc.name      : create source with caf_pcm_s32le, caf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6000, TestSize.Level2)
{
    DemuxerResult(INP_DIR_27, 235, 0);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6100
 * @tc.name      : seek_to_middle_time_previous_mode_adpcm_ima_wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 2500000, 61, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6200
 * @tc.name      : seek_to_middle_time_next_mode_adpcm_ms
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 2500000, 60, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6300
 * @tc.name      : seek_to_middle_time_closest_mode_alac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 2500000, 31, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6400
 * @tc.name      : seek_to_middle_time_previous_mode_amr_nb
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 2500000, 128, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6500
 * @tc.name      : seek_to_middle_time_next_mode_flac
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 2500000, 26, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6600
 * @tc.name      : seek_to_middle_time_closest_mode_gsm
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 2500000, 127, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6700
 * @tc.name      : seek_to_middle_time_previous_mode_gsm_ms
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 2500000, 64, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6800
 * @tc.name      : seek_to_middle_time_next_mode_ilbc
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_10, SEEK_MODE_NEXT_SYNC, 2500000, 127, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_6900
 * @tc.name      : seek_to_middle_time_closest_mode_mp2
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_6900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_11, SEEK_MODE_CLOSEST_SYNC, 2500000, 106, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7000
 * @tc.name      : seek_to_middle_time_previous_mode_mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_12, SEEK_MODE_PREVIOUS_SYNC, 2500000, 106, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7100
 * @tc.name      : seek_to_middle_time_next_mode_opus
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_13, SEEK_MODE_NEXT_SYNC, 300000, 238, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7200
 * @tc.name      : seek_to_middle_time_closest_mode_pcm_alaw
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_14, SEEK_MODE_CLOSEST_SYNC, 2500000, 30, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7300
 * @tc.name      : seek_to_middle_time_previous_mode_pcm_mulaw
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_15, SEEK_MODE_PREVIOUS_SYNC, 2500000, 5, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7400
 * @tc.name      : seek_to_middle_time_next_mode_pcm_s16be
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_16, SEEK_MODE_NEXT_SYNC, 2500000, 60, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7500
 * @tc.name      : seek_to_middle_time_closest_mode_caf_adpcm_ima_qt
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_17, SEEK_MODE_CLOSEST_SYNC, 2500000, 1875, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7600
 * @tc.name      : seek_to_middle_time_previous_mode_caf_pcm_f32be
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_18, SEEK_MODE_PREVIOUS_SYNC, 2500000, 118, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7700
 * @tc.name      : seek_to_middle_time_next_mode_caf_pcm_f32le
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_19, SEEK_MODE_NEXT_SYNC, 2500000, 118, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7800
 * @tc.name      : seek_to_middle_time_closest_mode_caf_pcm_f64be
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_20, SEEK_MODE_CLOSEST_SYNC, 2500000, 235, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_7900
 * @tc.name      : seek_to_middle_time_previous_mode_caf_pcm_f64le
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_7900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_21, SEEK_MODE_PREVIOUS_SYNC, 2500000, 235, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_8000
 * @tc.name      : seek_to_middle_time_next_mode_caf_pcm_s8
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_8000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_22, SEEK_MODE_NEXT_SYNC, 2500000, 30, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_8100
 * @tc.name      : seek_to_middle_time_closest_mode_caf_pcm_s16le
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_8100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_23, SEEK_MODE_CLOSEST_SYNC, 2500000, 59, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_8200
 * @tc.name      : seek_to_middle_time_previous_mode_caf_pcm_s24be
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_8200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_24, SEEK_MODE_PREVIOUS_SYNC, 2500000, 88, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_8300
 * @tc.name      : seek_to_middle_time_next_mode_caf_pcm_s24le
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_8300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_25, SEEK_MODE_NEXT_SYNC, 2500000, 88, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_8400
 * @tc.name      : seek_to_middle_time_closest_mode_caf_pcm_s32be
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_8400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_26, SEEK_MODE_CLOSEST_SYNC, 2500000, 118, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_FUNC_8500
 * @tc.name      : seek_to_middle_time_previous_mode_caf_pcm_s32le
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_FUNC_8500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_27, SEEK_MODE_PREVIOUS_SYNC, 2500000, 118, 0};
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_CAF_ILLEGAL_PARA_0100
 * @tc.name      : input invalid wma uri file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerCafFuncNdkTest, DEMUXER_CAF_ILLEGAL_PARA_0100, TestSize.Level2)
{
    const char *uri = "http://192.168.3.11:8080/share/invalid.wma";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(source, nullptr);
}
