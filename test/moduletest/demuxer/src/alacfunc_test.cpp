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
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avsource.h"
#include "native_avmemory.h"

#include <iostream>
#include <cstdio>
#include <string>
#include <fcntl.h>
#include <sys/stat.h>

namespace OHOS {
namespace Media {
class DemuxerAlacFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    const char* ALAC_M4A_1 = "/data/test/media/ALAC_16bit_44100Hz.m4a";
    const char* ALAC_M4A_2 = "/data/test/media/ALAC_24bit_48000Hz.m4a";
    const char* ALAC_M4A_3 = "/data/test/media/ALAC_32bit_96000Hz.m4a";
    
    const char* ALAC_MP4_1 = "/data/test/media/ALAC_16bit_44100Hz_mp4.m4a";
    const char* ALAC_MP4_2 = "/data/test/media/ALAC_24bit_48000Hz_mp4.m4a";
    const char* ALAC_MP4_3 = "/data/test/media/ALAC_32bit_96000Hz_mp4.m4a";
    
    const char* ALAC_MKV_1 = "/data/test/media/ALAC_16bit_44100Hz_mkv.mkv";
    const char* ALAC_MKV_2 = "/data/test/media/ALAC_24bit_48000Hz_mkv.mkv";
    const char* ALAC_MKV_3 = "/data/test/media/ALAC_32bit_96000Hz_mkv.mkv";
    
    const char* ALAC_MOV_1 = "/data/test/media/ALAC_16bit_44100Hz_mov.mov";
    const char* ALAC_MOV_2 = "/data/test/media/ALAC_24bit_48000Hz_mov.mov";
    const char* ALAC_MOV_3 = "/data/test/media/ALAC_32bit_96000Hz_mov.mov";
};

static OH_AVMemory* memory = nullptr;
static OH_AVSource* source = nullptr;
static OH_AVErrCode ret = AV_ERR_OK;
static OH_AVDemuxer* demuxer = nullptr;
static OH_AVFormat* sourceFormat = nullptr;
static OH_AVFormat* trackFormat = nullptr;
static OH_AVBuffer* avBuffer = nullptr;
static OH_AVFormat* format = nullptr;
static int32_t g_trackCount;
static int32_t g_memorySize = 1024 * 1024;
constexpr int32_t THOUSAND = 1000;
constexpr int32_t DEFAULT_TRACK_COUNT = 1;

void DemuxerAlacFuncNdkTest::SetUpTestCase() {}
void DemuxerAlacFuncNdkTest::TearDownTestCase() {}

void DemuxerAlacFuncNdkTest::SetUp() {
    memory = OH_AVMemory_Create(g_memorySize);
    g_trackCount = 0;
}

void DemuxerAlacFuncNdkTest::TearDown() {
    if (trackFormat != nullptr) { OH_AVFormat_Destroy(trackFormat); trackFormat = nullptr; }
    if (sourceFormat != nullptr) { OH_AVFormat_Destroy(sourceFormat); sourceFormat = nullptr; }
    if (format != nullptr) { OH_AVFormat_Destroy(format); format = nullptr; }
    if (memory != nullptr) { OH_AVMemory_Destroy(memory); memory = nullptr; }
    if (source != nullptr) { OH_AVSource_Destroy(source); source = nullptr; }
    if (demuxer != nullptr) { OH_AVDemuxer_Destroy(demuxer); demuxer = nullptr; }
    if (avBuffer != nullptr) { OH_AVBuffer_Destroy(avBuffer); avBuffer = nullptr; }
}
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

static int64_t GetFileSize(const char* fileName) {
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
    const char* fileName;
    OH_AVSeekMode seekmode;
    int64_t millisecond;
    int32_t audioCount;
};

static void CheckSeekModeFromEnd(seekInfo seekInfo) {
    int trackType = 0;
    int fd = open(seekInfo.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo.fileName);
    cout << seekInfo.fileName << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "track count: " << g_trackCount << endl;
    
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        ASSERT_EQ(AV_ERR_UNKNOWN, OH_AVDemuxer_SeekToTime(
            demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
    }
    close(fd);
}

static void CheckSeekMode(seekInfo seekInfo) {
    int trackType = 0;
    OH_AVCodecBufferAttr attr;
    int fd = open(seekInfo.fileName, O_RDONLY);
    int64_t size = GetFileSize(seekInfo.fileName);
    cout << seekInfo.fileName << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "track count: " << g_trackCount << endl;
    
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(
            demuxer, seekInfo.millisecond / THOUSAND, seekInfo.seekmode));
        
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
        
        if (trackType == MEDIA_TYPE_AUD) {
            cout << "audio frame count after seek: " << frameNum << endl;
            ASSERT_EQ(seekInfo.audioCount, frameNum);
        }
    }
    close(fd);
}

static void DemuxerAlacResult(const char* fileName, int32_t expectedFrames, int32_t expectedKeyFrames) {
    int trackType = 0;
    OH_AVCodecBufferAttr attr;
    bool isEnd = false;
    int32_t frameCount = 0;
    int32_t keyFrameCount = 0;
    
    int fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    cout << fileName << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, DEFAULT_TRACK_COUNT);
    
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
        ASSERT_EQ(trackType, MEDIA_TYPE_AUD);
        
        while (!isEnd) {
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
                isEnd = true;
                break;
            }
            frameCount++;
            if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
                keyFrameCount++;
            }
        }
    }
    
    ASSERT_EQ(frameCount, expectedFrames);
    ASSERT_EQ(keyFrameCount, expectedKeyFrames);
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0001
 * @tc.name      : demuxer ALAC ,GetTrackFormat,OH_MD_KEY_AUD_BIT_DEPTH
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0001, TestSize.Level2) {
    int32_t bitDepth = 0;
    const char* file = ALAC_M4A_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_BIT_DEPTH, &bitDepth));
    ASSERT_EQ(bitDepth, 16);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0002
 * @tc.name      : demuxer ALAC ,GetTrackFormat,OH_MD_KEY_AUD_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0002, TestSize.Level2) {
    int32_t sampleRate = 0;
    const char* file = ALAC_M4A_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0003
 * @tc.name      : demuxer ALAC ,GetTrackFormat,OH_MD_KEY_AUD_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0003, TestSize.Level2) {
    int32_t channelCount = 0;
    const char* file = ALAC_M4A_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 6);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0004
 * @tc.name      : demuxer ALAC ,GetTrackFormat,OH_MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0004, TestSize.Level2) {
    const char* mimeType = nullptr;
    const char* file = ALAC_M4A_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_STREQ(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_ALAC);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0005
 * @tc.name      : demuxer ALAC ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0005, TestSize.Level2) {
    const char* file = ALAC_M4A_3;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, DEFAULT_TRACK_COUNT);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0006
 * @tc.name      : demuxer ALAC ,GetTrackFormat,OH_MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0006, TestSize.Level2) {
    int32_t trackType = 0;
    const char* file = ALAC_M4A_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_EQ(trackType, MEDIA_TYPE_AUD);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0007
 * @tc.name      : demuxer ALAC ,ReadSample,check sample size
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0007, TestSize.Level3) {
    OH_AVCodecBufferAttr attr{};
    const char* file = ALAC_M4A_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ASSERT_EQ(OH_AVDemuxer_SelectTrackByID(demuxer, 0), AV_ERR_OK);
    
    ASSERT_EQ(OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr), AV_ERR_OK);
    ASSERT_GT(attr.size, 0);
    ASSERT_GT(attr.pts, 0);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0008
 * @tc.name      : demuxer ALAC ,full demux,16bit stereo
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0008, TestSize.Level3) {
    DemuxerAlacResult(ALAC_M4A_1, 128, 8);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0009
 * @tc.name      : demuxer ALAC ,full demux,24bit 5.1
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0009, TestSize.Level3) {
    DemuxerAlacResult(ALAC_M4A_2, 96, 6);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0010
 * @tc.name      : demuxer ALAC ,GetTrackFormat,OH_MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0010, TestSize.Level2) {
    int64_t bitRate = 0;
    const char* file = ALAC_M4A_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitRate));
    ASSERT_GT(bitRate, 0);
    ASSERT_LT(bitRate, 2000000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0011
 * @tc.name      : demuxer ALAC ,GetTrackFormat,OH_MD_KEY_AUD_CHANNEL_LAYOUT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0011, TestSize.Level2) {
    int64_t channelLayout = 0;
    const char* file = ALAC_M4A_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_LAYOUT, &channelLayout));
    ASSERT_EQ(channelLayout, 0x3F);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0012
 * @tc.name      : demuxer ALAC(MP4) ,GetTrackFormat,OH_MD_KEY_AUD_BIT_DEPTH
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0012, TestSize.Level2) {
    int32_t bitDepth = 0;
    const char* file = ALAC_MP4_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_BIT_DEPTH, &bitDepth));
    ASSERT_EQ(bitDepth, 16);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0013
 * @tc.name      : demuxer ALAC(MP4) ,GetTrackFormat,OH_MD_KEY_AUD_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0013, TestSize.Level2) {
    int32_t sampleRate = 0;
    const char* file = ALAC_MP4_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0014
 * @tc.name      : demuxer ALAC(MP4) ,GetTrackFormat,OH_MD_KEY_AUD_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0014, TestSize.Level2) {
    int32_t channelCount = 0;
    const char* file = ALAC_MP4_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 6);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0015
 * @tc.name      : demuxer ALAC(MP4) ,GetTrackFormat,OH_MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0015, TestSize.Level2) {
    const char* mimeType = nullptr;
    const char* file = ALAC_MP4_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_STREQ(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_ALAC);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0016
 * @tc.name      : demuxer ALAC(MP4) ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0016, TestSize.Level2) {
    const char* file = ALAC_MP4_3;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, DEFAULT_TRACK_COUNT);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0017
 * @tc.name      : demuxer ALAC(MP4) ,GetTrackFormat,OH_MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0017, TestSize.Level2) {
    int32_t trackType = 0;
    const char* file = ALAC_MP4_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_EQ(trackType, MEDIA_TYPE_AUD);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0018
 * @tc.name      : demuxer ALAC(MP4) ,ReadSample,check sample size
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0018, TestSize.Level3) {
    OH_AVCodecBufferAttr attr{};
    const char* file = ALAC_MP4_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ASSERT_EQ(OH_AVDemuxer_SelectTrackByID(demuxer, 0), AV_ERR_OK);
    
    ASSERT_EQ(OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr), AV_ERR_OK);
    ASSERT_GT(attr.size, 0);
    ASSERT_GT(attr.pts, 0);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0019
 * @tc.name      : demuxer ALAC(MP4) ,full demux,16bit stereo
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0019, TestSize.Level3) {
    DemuxerAlacResult(ALAC_MP4_1, 128, 8);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0020
 * @tc.name      : demuxer ALAC(MP4) ,full demux,24bit 5.1
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0020, TestSize.Level3) {
    DemuxerAlacResult(ALAC_MP4_2, 96, 6);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0021
 * @tc.name      : demuxer ALAC(MP4) ,GetTrackFormat,OH_MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0021, TestSize.Level2) {
    int64_t bitRate = 0;
    const char* file = ALAC_MP4_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitRate));
    ASSERT_GT(bitRate, 0);
    ASSERT_LT(bitRate, 2000000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0022
 * @tc.name      : demuxer ALAC(MP4) ,seek to middle,SEEK_MODE_NEXT_SYNC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0022, TestSize.Level3) {
    seekInfo testInfo{ALAC_MP4_1, SEEK_MODE_NEXT_SYNC, 5000, 180};
    CheckSeekMode(testInfo);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0023
 * @tc.name      : demuxer ALAC(MKV) ,GetTrackFormat,OH_MD_KEY_AUD_BIT_DEPTH
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0023, TestSize.Level2) {
    int32_t bitDepth = 0;
    const char* file = ALAC_MKV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_BIT_DEPTH, &bitDepth));
    ASSERT_EQ(bitDepth, 16);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0024
 * @tc.name      : demuxer ALAC(MKV) ,GetTrackFormat,OH_MD_KEY_AUD_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0024, TestSize.Level2) {
    int32_t sampleRate = 0;
    const char* file = ALAC_MKV_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0025
 * @tc.name      : demuxer ALAC(MKV) ,GetTrackFormat,OH_MD_KEY_AUD_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0025, TestSize.Level2) {
    int32_t channelCount = 0;
    const char* file = ALAC_MKV_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 6);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0026
 * @tc.name      : demuxer ALAC(MKV) ,GetTrackFormat,OH_MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0026, TestSize.Level2) {
    const char* mimeType = nullptr;
    const char* file = ALAC_MKV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_STREQ(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_ALAC);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0027
 * @tc.name      : demuxer ALAC(MKV) ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0027, TestSize.Level2) {
    const char* file = ALAC_MKV_3;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, DEFAULT_TRACK_COUNT);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0028
 * @tc.name      : demuxer ALAC(MKV) ,GetTrackFormat,OH_MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0028, TestSize.Level2) {
    int32_t trackType = 0;
    const char* file = ALAC_MKV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_EQ(trackType, MEDIA_TYPE_AUD);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0029
 * @tc.name      : demuxer ALAC(MKV) ,ReadSample,check sample size
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0029, TestSize.Level3) {
    OH_AVCodecBufferAttr attr{};
    const char* file = ALAC_MKV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ASSERT_EQ(OH_AVDemuxer_SelectTrackByID(demuxer, 0), AV_ERR_OK);
    
    ASSERT_EQ(OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr), AV_ERR_OK);
    ASSERT_GT(attr.size, 0);
    ASSERT_GT(attr.pts, 0);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0030
 * @tc.name      : demuxer ALAC(MKV) ,full demux,16bit stereo
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0030, TestSize.Level3) {
    DemuxerAlacResult(ALAC_MKV_1, 128, 8);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0031
 * @tc.name      : demuxer ALAC(MKV) ,full demux,24bit 5.1
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0031, TestSize.Level3) {
    DemuxerAlacResult(ALAC_MKV_2, 96, 6);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0032
 * @tc.name      : demuxer ALAC(MKV) ,GetTrackFormat,OH_MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0032, TestSize.Level2) {
    int64_t bitRate = 0;
    const char* file = ALAC_MKV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitRate));
    ASSERT_GT(bitRate, 0);
    ASSERT_LT(bitRate, 2000000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0033
 * @tc.name      : demuxer ALAC(MKV) ,seek to near end,SEEK_MODE_CLOSEST_SYNC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0033, TestSize.Level3) {
    seekInfo testInfo{ALAC_MKV_2, SEEK_MODE_CLOSEST_SYNC, 9000, 35};
    CheckSeekMode(testInfo);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0034
 * @tc.name      : demuxer ALAC(MOV) ,GetTrackFormat,OH_MD_KEY_AUD_BIT_DEPTH
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0034, TestSize.Level2) {
    int32_t bitDepth = 0;
    const char* file = ALAC_MOV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_BIT_DEPTH, &bitDepth));
    ASSERT_EQ(bitDepth, 16);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0035
 * @tc.name      : demuxer ALAC(MOV) ,GetTrackFormat,OH_MD_KEY_AUD_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0035, TestSize.Level2) {
    int32_t sampleRate = 0;
    const char* file = ALAC_MOV_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0036
 * @tc.name      : demuxer ALAC(MOV) ,GetTrackFormat,OH_MD_KEY_AUD_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0036, TestSize.Level2) {
    int32_t channelCount = 0;
    const char* file = ALAC_MOV_2;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 6);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0037
 * @tc.name      : demuxer ALAC(MOV) ,GetTrackFormat,OH_MD_KEY_CODEC_MIME
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0037, TestSize.Level2) {
    const char* mimeType = nullptr;
    const char* file = ALAC_MOV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, &mimeType));
    ASSERT_STREQ(mimeType, OH_AVCODEC_MIMETYPE_AUDIO_ALAC);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0038
 * @tc.name      : demuxer ALAC(MOV) ,GetSourceFormat,OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0038, TestSize.Level2) {
    const char* file = ALAC_MOV_3;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, DEFAULT_TRACK_COUNT);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0039
 * @tc.name      : demuxer ALAC(MOV) ,GetTrackFormat,OH_MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0039, TestSize.Level2) {
    int32_t trackType = 0;
    const char* file = ALAC_MOV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
    ASSERT_EQ(trackType, MEDIA_TYPE_AUD);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0040
 * @tc.name      : demuxer ALAC(MOV) ,ReadSample,check sample size
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0040, TestSize.Level3) {
    OH_AVCodecBufferAttr attr{};
    const char* file = ALAC_MOV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ASSERT_EQ(OH_AVDemuxer_SelectTrackByID(demuxer, 0), AV_ERR_OK);
    
    ASSERT_EQ(OH_AVDemuxer_ReadSample(demuxer, 0, memory, &attr), AV_ERR_OK);
    ASSERT_GT(attr.size, 0);
    ASSERT_GT(attr.pts, 0);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0041
 * @tc.name      : demuxer ALAC(MOV) ,full demux,16bit stereo
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0041, TestSize.Level3) {
    DemuxerAlacResult(ALAC_MOV_1, 128, 8);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0042
 * @tc.name      : demuxer ALAC(MOV) ,full demux,24bit 5.1
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0042, TestSize.Level3) {
    DemuxerAlacResult(ALAC_MOV_2, 96, 6);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0043
 * @tc.name      : demuxer ALAC(MOV) ,GetTrackFormat,OH_MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0043, TestSize.Level2) {
    int64_t bitRate = 0;
    const char* file = ALAC_MOV_1;
    int fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "-------fd:" << fd << "-------size:" << size << endl;
    
    source = OH_AVSource_CreateWithFD(fd, 0, size);
    ASSERT_NE(source, nullptr);
    
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &bitRate));
    ASSERT_GT(bitRate, 0);
    ASSERT_LT(bitRate, 2000000);
    
    close(fd);
}

/**
 * @tc.number    : DEMUXER_ALAC_FUNC_0044
 * @tc.name      : demuxer ALAC(MOV) ,seek to start,SEEK_MODE_PREVIOUS_SYNC
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAlacFuncNdkTest, DEMUXER_ALAC_FUNC_0044, TestSize.Level3) {
    seekInfo testInfo{ALAC_MOV_1, SEEK_MODE_PREVIOUS_SYNC, 0, 320};
    CheckSeekMode(testInfo);
}