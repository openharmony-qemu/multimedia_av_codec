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

#include "avdemuxer.h"
#include "avsource.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "buffer/avsharedmemory.h"
#include "buffer/avsharedmemorybase.h"
#include "securec.h"
#include "inner_demuxer_sample.h"

#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"
#include "meta/meta_key.h"
#include "meta/meta.h"
#include "av_common.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <thread>

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
using namespace testing::ext;

namespace {
class DemuxerNet2NdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

public:
    int32_t fd_ = -1;
    int64_t size;
};
static OH_AVMemory *memory = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static int32_t g_trackCount = 0;
static OH_AVBuffer *avBuffer = nullptr;

static int32_t g_width = 3840;
static int32_t g_height = 2160;
void DemuxerNet2NdkTest::SetUpTestCase() {}
void DemuxerNet2NdkTest::TearDownTestCase() {}
void DemuxerNet2NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerNet2NdkTest::TearDown()
{
    if (fd_ > 0) {
        close(fd_);
        fd_ = -1;
    }
    if (demuxer != nullptr) {
        OH_AVDemuxer_Destroy(demuxer);
        demuxer = nullptr;
    }
    if (memory != nullptr) {
        OH_AVMemory_Destroy(memory);
        memory = nullptr;
    }
    if (source != nullptr) {
        OH_AVSource_Destroy(source);
        source = nullptr;
    }
    if (avBuffer != nullptr) {
        OH_AVBuffer_Destroy(avBuffer);
        avBuffer = nullptr;
    }
    if (trackFormat != nullptr) {
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
    }
    if (sourceFormat != nullptr) {
        OH_AVFormat_Destroy(sourceFormat);
        sourceFormat = nullptr;
    }
}
} // namespace

namespace {
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
        int bufferSizeResult = 255;
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
        ASSERT_EQ(0, strcmp(language, "und"));
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_ROTATION, &rotation));
        ASSERT_EQ(0, rotation);
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
    static void CheckAudioKeyVVC()
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
        int bitrateResutlt = 10014008;
        ASSERT_EQ(bitrateResutlt, bitrate);
        ASSERT_TRUE(OH_AVFormat_GetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, &codecConfig, &bufferSize));
        size_t bufferSizeResult = 247;
        ASSERT_EQ(bufferSizeResult, bufferSize);
        ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
        int expectNum = 0;
        ASSERT_EQ(expectNum, strcmp(mimeType, OH_AVCODEC_MIMETYPE_VIDEO_VVC));
        ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_FRAME_RATE, &frameRate));
        double frameRateResult = 60.000000;
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

    /**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1000
 * @tc.name     : determine the orientation type of the video ROTATE_NONE.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1000, TestSize.Level0)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_NONE.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1001
 * @tc.name     : determine the orientation type of the video ROTATE_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1001, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1002
 * @tc.name     : determine the orientation type of the video ROTATE_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1002, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1003
 * @tc.name     : determine the orientation type of the video ROTATE_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1003, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/ROTATE_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1004
 * @tc.name     : determine the orientation type of the video FLIP_H.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1004, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1005
 * @tc.name     : determine the orientation type of the video FLIP_V.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1005, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1006
 * @tc.name     : determine the orientation type of the video FLIP_H_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1006, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H_90.mp4";;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1007
 * @tc.name     : determine the orientation type of the video FLIP_V_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1007, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1008
 * @tc.name     : determine the orientation type of the video FLIP_H_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1008, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V || rotation == OHOS::MediaAVCodec::FLIP_H_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1009
 * @tc.name     : determine the orientation type of the video FLIP_V_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1009, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H || rotation == OHOS::MediaAVCodec::FLIP_V_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1010
 * @tc.name     : determine the orientation type of the video FLIP_H_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1010, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_H_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_H_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1011
 * @tc.name     : determine the orientation type of the video FLIP_V_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1011, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/FLIP_V_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_V_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1012
 * @tc.name     : determine the orientation type of the video INVALID.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1012, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/INVALID.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1013
 * @tc.name     : determine the orientation type of the video AV_ROTATE_NONE.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1013, TestSize.Level0)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_NONE.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1014
 * @tc.name     : determine the orientation type of the video AV_ROTATE_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1014, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1015
 * @tc.name     : determine the orientation type of the video AV_ROTATE_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1015, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1016
 * @tc.name     : determine the orientation type of the video AV_ROTATE_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1016, TestSize.Level1)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_ROTATE_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1017
 * @tc.name     : determine the orientation type of the video AV_FLIP_H.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1017, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1018
 * @tc.name     : determine the orientation type of the video AV_FLIP_V.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1018, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1019
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1019, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_H_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1020
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_90.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1020, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V_90.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::FLIP_V_ROT90);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1021
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1021, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V || rotation == OHOS::MediaAVCodec::FLIP_H_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1022
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_180.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1022, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V_180.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H || rotation == OHOS::MediaAVCodec::FLIP_V_ROT180);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1023
 * @tc.name     : determine the orientation type of the video AV_FLIP_H_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1023, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_H_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_V_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_H_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1024
 * @tc.name     : determine the orientation type of the video AV_FLIP_V_270.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1024, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_FLIP_V_270.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_TRUE(rotation == OHOS::MediaAVCodec::FLIP_H_ROT90 || rotation == OHOS::MediaAVCodec::FLIP_V_ROT270);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1025
 * @tc.name     : determine the orientation type of the video AV_INVALID.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1025, TestSize.Level2)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = -1;
    const char *uri = "http://192.168.3.11:8080/share/rotation/AV_INVALID.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1026
 * @tc.name     : determine the orientation type of the video UNDEFINED_FLV.flv
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1026, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_FLV.flv";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1027
 * @tc.name     : determine the orientation type of the video UNDEFINED_fmp4.mp4
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1027, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_FMP4.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1028
 * @tc.name     : determine the orientation type of the video UNDEFINED_MKV.mkv
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1028, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_MKV.mkv";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number   : DEMUXER_ORIENTATIONTYPE_1029
 * @tc.name     : determine the orientation type of the video UNDEFINED_TS.ts
 * @tc.desc     : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_ORIENTATIONTYPE_1029, TestSize.Level3)
{
    static OH_AVFormat *trackFormat = nullptr;
    int32_t rotation = 0;
    const char *uri = "http://192.168.3.11:8080/share/rotation/UNDEFINED_TS.ts";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetIntValue(trackFormat, Media::Tag::VIDEO_ORIENTATION_TYPE, &rotation));
    ASSERT_EQ(rotation, OHOS::MediaAVCodec::ROTATE_NONE);
    OH_AVFormat_Destroy(trackFormat);
}

/**
 * @tc.number    : DEMUXER_VVC_NET_0100
 * @tc.name      : demuxer 8bit H266 MP4 file, read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_VVC_NET_0100, TestSize.Level0)
{
    if (memory == nullptr) {
        memory = OH_AVMemory_Create(g_width * g_height);
    }
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *uri = "http://192.168.3.11:8080/share/vvc_8bit_3840_2160.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int vKeyCount = 0;
    while (!videoIsEnd) {
        trackFormat = OH_AVSource_GetTrackFormat(source, 0);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        if (videoIsEnd) {
            continue;
        }
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
    }
    ASSERT_EQ(videoFrame, 600);
    ASSERT_EQ(vKeyCount, 10);
}

/**
 * @tc.number    : DEMUXER_VVC_NET_0200
 * @tc.name      : demuxer 10bit H266 MP4 file, read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_VVC_NET_0200, TestSize.Level0)
{
    if (memory == nullptr) {
        memory = OH_AVMemory_Create(g_width * g_height);
    }
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool videoIsEnd = false;
    int videoFrame = 0;
    const char *uri = "http://192.168.3.11:8080/share/vvc_aac_10bit_1920_1080.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
            if ((audioIsEnd && (tarckType == 0)) || (videoIsEnd && (tarckType == 1))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (tarckType == OHOS::Media::MEDIA_TYPE_VID) {
                SetVideoValue(attr, videoIsEnd, videoFrame, vKeyCount);
            } else if (tarckType == OHOS::Media::MEDIA_TYPE_AUD) {
                SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            }
        }
    }
    ASSERT_EQ(audioFrame, 2812);
    ASSERT_EQ(aKeyCount, 2812);
    ASSERT_EQ(videoFrame, 3000);
    ASSERT_EQ(vKeyCount, 63);
}

/**
 * @tc.number    : DEMUXER_VVC_NET_0300
 * @tc.name      : demuxer 8bit H266 MP4 file, read+seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_VVC_NET_0300, TestSize.Level0)
{
    if (memory == nullptr) {
        memory = OH_AVMemory_Create(g_width * g_height);
    }
    int64_t duration = 0;
    OH_AVCodecBufferAttr attr;
    const char *uri = "http://192.168.3.11:8080/share/vvc_8bit_3840_2160.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(duration, 10000000);
    for (int index = 0; index < (duration / 1000); index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, index, OH_AVSeekMode::SEEK_MODE_CLOSEST_SYNC));
    }
}

/**
 * @tc.number    : DEMUXER_VVC_NET_0400
 * @tc.name      : demuxer 10bit H266 MP4 file, read+seek
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_VVC_NET_0400, TestSize.Level0)
{
    if (memory == nullptr) {
        memory = OH_AVMemory_Create(g_width * g_height);
    }
    int64_t duration = 0;
    OH_AVCodecBufferAttr attr;
    const char *uri = "http://192.168.3.11:8080/share/vvc_aac_10bit_1920_1080.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(2, g_trackCount);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    ASSERT_EQ(duration, 60000000);
    for (int num = 0; num < (duration / 1000); num++) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, num, OH_AVSeekMode::SEEK_MODE_CLOSEST_SYNC));
        }
    }
}

/**
 * @tc.number    : DEMUXER_VVC_NET_0500
 * @tc.name      : demuxer 8bit H266 MP4 file, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_VVC_NET_0500, TestSize.Level0)
{
    int64_t duration = 0;
    int64_t startTime;
    const char *uri = "http://192.168.3.11:8080/share/vvc_8bit_3840_2160.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_DURATION, &duration));
    int durationResutlt = 10000000;
    ASSERT_EQ(durationResutlt, duration);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(sourceFormat, OH_MD_KEY_START_TIME, &startTime));
    int startTimeResult = 0;
    ASSERT_EQ(startTimeResult, startTime);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(1, g_trackCount);
    CheckAudioKeyVVC();
}

/**
 * @tc.number    : DEMUXER_VVC_NET_0600
 * @tc.name      : demuxer 10bit H266 MP4 file, check key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerNet2NdkTest, DEMUXER_VVC_NET_0600, TestSize.Level0)
{
    int64_t duration = 0;
    int64_t startTime;
    int tarckType = 0;
    const char *uri = "http://192.168.3.11:8080/share/vvc_aac_10bit_1920_1080.mp4";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
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
        if (tarckType == OHOS::Media::MEDIA_TYPE_VID) {
            CheckVideoKey();
        } else if (tarckType == OHOS::Media::MEDIA_TYPE_AUD) {
            CheckAudioKey();
        }
    }
}
} // namespace