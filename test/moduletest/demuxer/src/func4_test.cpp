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
#include <cstdio>
#include <cmath>
#include <fcntl.h>
#include <iostream>
#include <string>
#include <thread>
#include "gtest/gtest.h"
#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"
#include "securec.h"

namespace OHOS {
namespace Media {
class DemuxerFunc4NdkTest : public testing::Test {
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
    const char *INP_DIR_ADPCM_YAMAHA_MKV = "/data/test/media/adpcm_yamaha.mkv";
    const char *INP_DIR_ADPCM_YAMAHA_WAV = "/data/test/media/adpcm_yamaha.wav";
};

static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVDemuxer *demuxer = nullptr;
static OH_AVFormat *sourceFormat = nullptr;
static OH_AVFormat *trackFormat = nullptr;
static OH_AVBuffer *avBuffer = nullptr;
static OH_AVFormat *format = nullptr;
static OH_AVFormat *metaFormat = nullptr;
static int g_fd = -1;
static int32_t g_trackCount;
static int32_t g_width = 3840;
static int32_t g_height = 2160;
constexpr int  FIFTY = 50;
constexpr int  THREE = 3;
constexpr int  TWO = 2;
constexpr uint8_t NUM_FOUR = 4;
constexpr uint8_t NUM_EIGHT = 8;
constexpr uint8_t NUM_NINE = 9;
void DemuxerFunc4NdkTest::SetUpTestCase() {}
void DemuxerFunc4NdkTest::TearDownTestCase() {}
void DemuxerFunc4NdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}
void DemuxerFunc4NdkTest::TearDown()
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
    if (metaFormat != nullptr) {
        OH_AVFormat_Destroy(metaFormat);
        metaFormat = nullptr;
    }
}
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

struct seekInfo {
    const char *fileName;
    OH_AVSeekMode seekmode;
    int64_t millisecond;
    int32_t videoCount;
    int32_t audioCount;
};

struct SkipSampleInfo {
    int32_t frame;
    uint32_t skipSamples;
    uint32_t discardPadding;
    int skipReason;
    int discardReason;
    size_t buffSize;
};

struct ExpectSkipSampleInfo {
    int32_t expectFrame;
    uint32_t expectSkipSamples;
    uint32_t expectDiscardPadding;
    int expectSkipReason;
    int expectDiscardReason;
    size_t expectBuffSize;
};

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

void GetMetaInfo(string metaKeyAdd, string metaKey)
{
    int32_t extraSize = 30;
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    for (int i = 0; i < FIFTY; i++) {
        metaKey.append(metaKeyAdd);
        ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, metaKey.c_str(), &codecConfig, &bufferSize));
        ASSERT_EQ(bufferSize, extraSize);
        for (int j = 0; j < bufferSize; j++) {
            ASSERT_EQ(THREE, codecConfig[j]);
        }
        extraSize = extraSize + TWO;
    }
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

static bool InitFile(const char *file, int32_t trackNum, int &fd, int64_t &size)
{
    fd = open(file, O_RDONLY);
    size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    if (source == nullptr) {
        return false;
    }
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    if (demuxer == nullptr) {
        return false;
    }
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    if (!OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount)) {
        return false;
    }
    if (trackNum != g_trackCount) {
        return false;
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        if (OH_AVDemuxer_SelectTrackByID(demuxer, index) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}

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
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond / 1000, seekInfo.seekmode));
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
    close(g_fd);
    g_fd = -1;
}

static void DemuxerResult(const char *fileName, int32_t expectTrack)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    g_fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    cout << fileName << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, expectTrack);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    int vKeyCount = 0;
    while (((!audioIsEnd || !videoIsEnd) && expectTrack >= 2)
        || (!audioIsEnd && !videoIsEnd && expectTrack == 1)) {
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
}

static bool CompareSkipSampleValue(SkipSampleInfo& info, ExpectSkipSampleInfo expectInfo)
{
    if (info.frame != expectInfo.expectFrame) {
        return false;
    }
    if (info.skipSamples != expectInfo.expectSkipSamples) {
        return false;
    }
    if (info.discardPadding != expectInfo.expectDiscardPadding) {
        return false;
    }
    if (info.skipReason != expectInfo.expectSkipReason) {
        return false;
    }
    if (info.discardReason != expectInfo.expectDiscardReason) {
        return false;
    }
    if (info.buffSize != expectInfo.expectBuffSize) {
        return false;
    }
    return true;
}

static void GetSkipSampleInfo(uint8_t *addr, size_t buffSize, int &audioFrame, std::vector<SkipSampleInfo> &skipSample)
{
    if (OH_AVFormat_GetBuffer(format, OH_MD_KEY_BUFFER_SKIP_SAMPLES_INFO, &addr, &buffSize)) {
        uint32_t skipSamples;
        memcpy_s(&skipSamples, sizeof(uint32_t), addr, sizeof(uint32_t));
        uint32_t discardPadding;
        memcpy_s(&discardPadding, sizeof(uint32_t), addr + NUM_FOUR, sizeof(uint32_t));
        uint8_t skipReason;
        memcpy_s(&skipReason, sizeof(uint8_t), addr + NUM_EIGHT, sizeof(uint8_t));
        uint8_t discardReason;
        memcpy_s(&discardReason, sizeof(uint8_t), addr + NUM_NINE, sizeof(uint8_t));
        SkipSampleInfo skipSampleInfo{audioFrame, skipSamples, discardPadding, static_cast<int>(skipReason),
            static_cast<int>(discardReason), buffSize};
        skipSample.push_back(skipSampleInfo);
    }
}

/**
 * @tc.number    : DEMUXER_MKV_ADPCM_YAMAHA_FUNC_0100
 * @tc.name      : demuxer MKV, audio track adpcm yamaha
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_MKV_ADPCM_YAMAHA_FUNC_0100, TestSize.Level2)
{
    DemuxerResult(INP_DIR_ADPCM_YAMAHA_MKV, 2);
}

/**
 * @tc.number    : DEMUXER_MKV_ADPCM_YAMAHA_FUNC_0200
 * @tc.name      : demuxer MKV, audio track adpcm yamaha check seek mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_MKV_ADPCM_YAMAHA_FUNC_0200, TestSize.Level2)
{
    seekInfo fileTest1{INP_DIR_ADPCM_YAMAHA_MKV, SEEK_MODE_NEXT_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_ADPCM_YAMAHA_MKV, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_ADPCM_YAMAHA_MKV, SEEK_MODE_CLOSEST_SYNC, 0, 29, 47};
    CheckSeekMode(fileTest3);
}

/**
 * @tc.number    : DEMUXER_MKV_ADPCM_YAMAHA_FUNC_0300
 * @tc.name      : demuxer MOV, audio track adpcm yamaha get track format
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_MKV_ADPCM_YAMAHA_FUNC_0300, TestSize.Level2)
{
    const char *mimeType = nullptr;
    const char *file = INP_DIR_ADPCM_YAMAHA_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(memcmp(mimeType, "audio/adpcm_yamaha", strlen("audio/adpcm_yamaha")), 0);
    int32_t channel = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channel));
    ASSERT_EQ(channel, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000); // 48000: source sample rate
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_WAV_ADPCM_YAMAHA_FUNC_0100
 * @tc.name      : demuxer WAV, audio track adpcm yamaha
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_WAV_ADPCM_YAMAHA_FUNC_0100, TestSize.Level2)
{
    DemuxerResult(INP_DIR_ADPCM_YAMAHA_WAV, 1);
}

/**
 * @tc.number    : DEMUXER_WAV_ADPCM_YAMAHA_FUNC_0200
 * @tc.name      : demuxer WAV, audio track adpcm yamaha check seek mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_WAV_ADPCM_YAMAHA_FUNC_0200, TestSize.Level2)
{
    seekInfo fileTest1{INP_DIR_ADPCM_YAMAHA_WAV, SEEK_MODE_NEXT_SYNC, 0, 29, 12};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_ADPCM_YAMAHA_WAV, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 12};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_ADPCM_YAMAHA_WAV, SEEK_MODE_CLOSEST_SYNC, 0, 29, 12};
    CheckSeekMode(fileTest3);
}

/**
 * @tc.number    : DEMUXER_WAV_ADPCM_YAMAHA_FUNC_0300
 * @tc.name      : demuxer WAV, audio track adpcm yamaha get track format
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_WAV_ADPCM_YAMAHA_FUNC_0300, TestSize.Level2)
{
    const char *mimeType = nullptr;
    const char *file = INP_DIR_ADPCM_YAMAHA_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_EQ(memcmp(mimeType, "audio/adpcm_yamaha", strlen("audio/adpcm_yamaha")), 0);
    int32_t channel = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channel));
    ASSERT_EQ(channel, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000); // 48000: source sample rate
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0110
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0110, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_03.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0120
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0120, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_03.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_FALSE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.buffervalval", &codecConfig, &bufferSize));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0130
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0130, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_03.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.bufferval", &metaStringValue));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.bufferval", &metaIntValue));
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.bufferval", &metaFloatValue));
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : DEMUXER_META_0140
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0140, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_01.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval.max", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 1048576);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0150
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0150, TestSize.Level1)
{
    const char *file = "/data/test/media/metaval_04.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    string metaVal = "aaaaa";
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.stringval", &metaStringValue));
    ASSERT_EQ(metaStringValue, metaVal);
    float floatValue = 252.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval", &metaFloatValue));
    ASSERT_EQ(floatValue, metaFloatValue);
    int32_t intValue = 10000;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.intval.intvalintval", &metaIntValue));
    ASSERT_EQ(metaIntValue, intValue);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0160
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0160, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/metaval_05.mp4";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    string metaKeyAdd = "a";
    string metaKey = "com.openharmony.stringval.";
    string metaVal = "aaaaa";
    string metaValAdd = "b";
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaVal.append(metaValAdd);
        ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, metaKey.c_str(), &metaStringValue));
        ASSERT_EQ(metaStringValue, metaVal);
    }

    metaKeyAdd = "a";
    metaKey = "com.openharmony.floatval.";
    float metaFloatVal = 123.5;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaFloatVal = metaFloatVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, metaKey.c_str(), &metaFloatValue));
        ASSERT_EQ(metaFloatValue, metaFloatVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.Intval.";
    int32_t metaIntVal = 123;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaIntVal = metaIntVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, metaKey.c_str(), &metaIntValue));
        ASSERT_EQ(metaIntValue, metaIntVal);
    }
    
    metaKeyAdd = "a";
    metaKey = "com.openharmony.bufferval.";
    GetMetaInfo(metaKeyAdd, metaKey);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0170
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0170, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_03.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0180
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0180, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_03.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_FALSE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.buffervalval", &codecConfig, &bufferSize));
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0190
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0190, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_03.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.bufferval", &metaStringValue));
    ASSERT_FALSE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.bufferval", &metaIntValue));
    ASSERT_FALSE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.bufferval", &metaFloatValue));
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}
/**
 * @tc.number    : DEMUXER_META_0200
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0200, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_01.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval.max", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 1048576);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0210
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0210, TestSize.Level1)
{
    const char *file = "/data/test/media/audio/metaval_04.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    uint8_t *codecConfig = nullptr;
    size_t bufferSize;
    string metaVal = "aaaaa";
    ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, "com.openharmony.stringval", &metaStringValue));
    ASSERT_EQ(metaStringValue, metaVal);
    float floatValue = 252.3;
    ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, "com.openharmony.floatval", &metaFloatValue));
    ASSERT_EQ(floatValue, metaFloatValue);
    int32_t intValue = 10000;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, "com.openharmony.intval.intvalintval", &metaIntValue));
    ASSERT_EQ(metaIntValue, intValue);
    ASSERT_TRUE(OH_AVFormat_GetBuffer(metaFormat, "com.openharmony.bufferval", &codecConfig, &bufferSize));
    ASSERT_EQ(bufferSize, 30);
    for (int j = 0; j < bufferSize; j++) {
        ASSERT_EQ(3, codecConfig[j]);
    }
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0220
 * @tc.name      : demuxer meta info, get value with right key
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0220, TestSize.Level2)
{
    const char* metaStringValue = nullptr;
    int32_t metaIntValue = 0;
    float metaFloatValue = 0.0;
    const char *file = "/data/test/media/audio/metaval_05.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    string metaKeyAdd = "a";
    string metaKey = "com.openharmony.stringval.";
    string metaVal = "aaaaa";
    string metaValAdd = "b";
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaVal.append(metaValAdd);
        ASSERT_TRUE(OH_AVFormat_GetStringValue(metaFormat, metaKey.c_str(), &metaStringValue));
        ASSERT_EQ(metaStringValue, metaVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.floatval.";
    float metaFloatVal = 123.5;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaFloatVal = metaFloatVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetFloatValue(metaFormat, metaKey.c_str(), &metaFloatValue));
        ASSERT_EQ(metaFloatValue, metaFloatVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.Intval.";
    int32_t metaIntVal = 123;
    for (int i = 0; i < 50; i++) {
        metaKey.append(metaKeyAdd);
        metaIntVal = metaIntVal + 2;
        ASSERT_TRUE(OH_AVFormat_GetIntValue(metaFormat, metaKey.c_str(), &metaIntValue));
        ASSERT_EQ(metaIntValue, metaIntVal);
    }
    metaKeyAdd = "a";
    metaKey = "com.openharmony.bufferval.";
    GetMetaInfo(metaKeyAdd, metaKey);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_META_0230
 * @tc.name      : demuxer meta info, file with no meta
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_META_0230, TestSize.Level2)
{
    const char *file = "/data/test/media/audio/M4A_48000_1.m4a";
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    metaFormat= OH_AVSource_GetCustomMetadataFormat(source);
    ASSERT_NE(metaFormat, nullptr);
    const char* language = OH_AVFormat_DumpInfo(metaFormat);
    ASSERT_EQ(language, nullptr);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SKIP_SAMPLE_FUNC_0010
 * @tc.name      : demuxer skip sample, mp3, read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_SKIP_SAMPLE_FUNC_0010, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    std::vector<SkipSampleInfo> skipSample;
    const char *file = "/data/test/media/audio/MP3_8K_1.mp3";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 1, fd, size));
    int aKeyCount = 0;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer));
            avBuffer = nullptr;
            GetSkipSampleInfo(addr, buffSize, audioFrame, skipSample);
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            OH_AVFormat_Destroy(format);
            format = nullptr;
        }
    }
    ASSERT_EQ(skipSample.size(), 2);
    ASSERT_EQ(true, CompareSkipSampleValue(skipSample[0], {0, 1105, 0, 0, 0, 10}));
    ASSERT_EQ(true, CompareSkipSampleValue(skipSample[1], {3051, 0, 419, 0, 0, 10}));
    ASSERT_EQ(audioFrame, 3052);
    ASSERT_EQ(aKeyCount, 3052);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SKIP_SAMPLE_FUNC_0020
 * @tc.name      : demuxer skip sample, mp3, seek + read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_SKIP_SAMPLE_FUNC_0020, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    std::vector<SkipSampleInfo> skipSample;
    const char *file = "/data/test/media/audio/MP3_8K_1.mp3";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 1, fd, size));
    int aKeyCount = 0;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, 218088000 / 1000, SEEK_MODE_CLOSEST_SYNC));
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer));
            avBuffer = nullptr;
            GetSkipSampleInfo(addr, buffSize, audioFrame, skipSample);
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            OH_AVFormat_Destroy(format);
            format = nullptr;
        }
    }
    ASSERT_EQ(skipSample.size(), 1);
    ASSERT_EQ(true, CompareSkipSampleValue(skipSample[0], {21, 0, 419, 0, 0, 10}));
    ASSERT_EQ(audioFrame, 22);
    ASSERT_EQ(aKeyCount, 22);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SKIP_SAMPLE_FUNC_0030
 * @tc.name      : demuxer skip sample, ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_SKIP_SAMPLE_FUNC_0030, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    std::vector<SkipSampleInfo> skipSample;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/OGG_8K_1.ogg";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 1, fd, size));
    int aKeyCount = 0;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer));
            avBuffer = nullptr;
            GetSkipSampleInfo(addr, buffSize, audioFrame, skipSample);
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            OH_AVFormat_Destroy(format);
            format = nullptr;
        }
    }
    ASSERT_EQ(skipSample.size(), 1);
    ASSERT_EQ(true, CompareSkipSampleValue(skipSample[0], {6862, 0, 244, 0, 0, 10}));
    ASSERT_EQ(audioFrame, 6863);
    ASSERT_EQ(aKeyCount, 6863);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SKIP_SAMPLE_FUNC_0040
 * @tc.name      : demuxer skip sample, ogg, seek + read
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_SKIP_SAMPLE_FUNC_0040, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    std::vector<SkipSampleInfo> skipSample;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/OGG_8K_1.ogg";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 1, fd, size));
    int aKeyCount = 0;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, 219296000 / 1000, SEEK_MODE_CLOSEST_SYNC));
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer));
            avBuffer = nullptr;
            GetSkipSampleInfo(addr, buffSize, audioFrame, skipSample);
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
            OH_AVFormat_Destroy(format);
            format = nullptr;
        }
    }
    ASSERT_EQ(skipSample.size(), 1);
    ASSERT_EQ(true, CompareSkipSampleValue(skipSample[0], {46, 0, 244, 0, 0, 10}));
    ASSERT_EQ(audioFrame, 47);
    ASSERT_EQ(aKeyCount, 47);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SKIP_SAMPLE_FUNC_0050
 * @tc.name      : demuxer no skip sample, mp3
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_SKIP_SAMPLE_FUNC_0050, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/feff_bom.mp3";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 1, fd, size));
    int aKeyCount = 0;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_FALSE(OH_AVFormat_GetBuffer(format, OH_MD_KEY_BUFFER_SKIP_SAMPLES_INFO, &addr, &buffSize));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer));
            avBuffer = nullptr;
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 203);
    ASSERT_EQ(aKeyCount, 203);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SKIP_SAMPLE_FUNC_0060
 * @tc.name      : demuxer no skip sample, ogg
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_SKIP_SAMPLE_FUNC_0060, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/no_skip_sample.ogg";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 1, fd, size));
    int aKeyCount = 0;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_FALSE(OH_AVFormat_GetBuffer(format, OH_MD_KEY_BUFFER_SKIP_SAMPLES_INFO, &addr, &buffSize));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer));
            avBuffer = nullptr;
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 6813);
    ASSERT_EQ(aKeyCount, 6813);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_SKIP_SAMPLE_FUNC_0070
 * @tc.name      : demuxer no skip sample, wma
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_SKIP_SAMPLE_FUNC_0070, TestSize.Level0)
{
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = "/data/test/media/audio/aac.wma";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 1, fd, size));
    int aKeyCount = 0;
    uint8_t *addr = nullptr;
    size_t buffSize = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            avBuffer = OH_AVBuffer_Create(size);
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &attr));
            format = OH_AVBuffer_GetParameter(avBuffer);
            ASSERT_NE(format, nullptr);
            ASSERT_FALSE(OH_AVFormat_GetBuffer(format, OH_MD_KEY_BUFFER_SKIP_SAMPLES_INFO, &addr, &buffSize));
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_Destroy(avBuffer));
            avBuffer = nullptr;
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 433);
    ASSERT_EQ(aKeyCount, 433);
    close(fd);
    fd = -1;
}

/**
 * @tc.number    : DEMUXER_FUNCTION_SAR_0010
 * @tc.name      : demuxer mp4 file with sar is not 1
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerFunc4NdkTest, DEMUXER_FUNCTION_SAR_0010, TestSize.Level0)
{
    int tarckType = 0;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    OH_AVCodecBufferAttr bufferAttr;
    const char *file = "/data/test/media/sarnot1.mp4";
    int fd = 0;
    int64_t size = 0;
    ASSERT_TRUE(InitFile(file, 2, fd, size));
    int aKeyCount = 0;
    int vKeyCount = 0;
    double sar = 0.0;
    avBuffer = OH_AVBuffer_Create(size);
    ASSERT_NE(avBuffer, nullptr);
    while (!audioIsEnd || !videoIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD)) || (videoIsEnd && (tarckType == MEDIA_TYPE_VID))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSampleBuffer(demuxer, index, avBuffer));
            ASSERT_NE(avBuffer, nullptr);
            ASSERT_EQ(AV_ERR_OK, OH_AVBuffer_GetBufferAttr(avBuffer, &bufferAttr));
            if (tarckType == MEDIA_TYPE_AUD) {
                SetAudioValue(bufferAttr, audioIsEnd, audioFrame, aKeyCount);
            } else if (tarckType == MEDIA_TYPE_VID) {
                ASSERT_TRUE(OH_AVFormat_GetDoubleValue(trackFormat, OH_MD_KEY_VIDEO_SAR, &sar));
                ASSERT_EQ(0.31645569620253167, sar);
                SetVideoValue(bufferAttr, videoIsEnd, videoFrame, vKeyCount);
            }
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
        }
    }
    ASSERT_EQ(audioFrame, 95);
    ASSERT_EQ(videoFrame, 60);    
    ASSERT_EQ(aKeyCount, 95);
    ASSERT_EQ(vKeyCount, 1);
    close(fd);
    fd = -1;
}