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
#include <cmath>
#include <thread>
namespace OHOS {
namespace Media {
class DemuxerAdpcmFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase(void);

    static void TearDownTestCase(void);

    void SetUp(void);

    void TearDown(void);
protected:
    const char *INP_DIR_1 = "/data/test/media/ADPCM_G722.wav";
    const char *INP_DIR_2 = "/data/test/media/ADPCM_G722.mov";
    const char *INP_DIR_3 = "/data/test/media/ADPCM_G722.avi";
    const char *INP_DIR_4 = "/data/test/media/ADPCM_G722.mkv";
    const char *INP_DIR_5 = "/data/test/media/ADPCM_G726.wav";
    const char *INP_DIR_QT_MOV = "/data/test/media/adpcm_ima_qt.mov";
    const char *INP_DIR_WAV_WAV = "/data/test/media/adpcm_ima_wav.wav";
    const char *INP_DIR_WAV_MOV = "/data/test/media/adpcm_ima_wav.mov";
    const char *INP_DIR_WAV_AVI = "/data/test/media/adpcm_ima_wav.avi";
    const char *INP_DIR_WAV_MKV = "/data/test/media/adpcm_ima_wav.mkv";

};

static int g_fd = -1;
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
constexpr int32_t THOUSAND = 1000.0;
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";

void DemuxerAdpcmFuncNdkTest::SetUpTestCase() {}

void DemuxerAdpcmFuncNdkTest::TearDownTestCase() {}

void DemuxerAdpcmFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}

void DemuxerAdpcmFuncNdkTest::TearDown()
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
    int32_t videoCount;
    int32_t audioCount;
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
        cout << "frameNum---" << frameNum << endl;
        ASSERT_EQ(seekInfo.audioCount, frameNum);
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

static void DemuxerResult(const char *fileName)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    g_fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    cout << fileName << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0300
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0300, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0301
 * @tc.name      : demuxer mov, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0301, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0302
 * @tc.name      : demuxer avi, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0302, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0303
 * @tc.name      : demuxer mkv, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0303, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0304
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0304, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0305
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0305, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0306
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0306, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0307
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0307, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0308
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0308, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0309
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_TITLE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0309, TestSize.Level2)
{
    const char *stringVal;
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_TITLE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0400
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0400, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0401
 * @tc.name      : demuxer mov, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0401, TestSize.Level2)
{
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0402
 * @tc.name      : demuxer avi, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0402, TestSize.Level2)
{
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0403
 * @tc.name      : demuxer mkv, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0403, TestSize.Level2)
{
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0404
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0404, TestSize.Level2)
{
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0405
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0405, TestSize.Level2)
{
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0406
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0406, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0407
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0407, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0408
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0408, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0409
 * @tc.name      : demuxer wav, check source format, OH_MD_KEY_ALBUM_ARTIST
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0409, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_ALBUM_ARTIST, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0500
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0500, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0501
 * @tc.name      : demuxer mov, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0501, TestSize.Level2)
{
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0502
 * @tc.name      : demuxer avi, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0502, TestSize.Level2)
{
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0503
 * @tc.name      : demuxer mkv, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0503, TestSize.Level2)
{
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0504
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0504, TestSize.Level2)
{
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0505
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0505, TestSize.Level2)
{
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0506
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0506, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0507
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0507, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0508
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0508, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0509
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_DATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest,  DEMUXER_ADPCM_FUNC_0509, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_DATE, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0600
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0600, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0601
 * @tc.name      : demuxer mov, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0601, TestSize.Level2)
{
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0602
 * @tc.name      : demuxer avi, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0602, TestSize.Level2)
{
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0603
 * @tc.name      : demuxer mkv, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0603, TestSize.Level2)
{
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0604
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0604, TestSize.Level2)
{
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0605
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0605, TestSize.Level2)
{
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0606
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0606, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0607
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0607, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0608
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0608, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0609
 * @tc.name      : demuxer wav, check track format, OH_MD_KEY_COMMENT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0609, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    const char *stringVal;
    ASSERT_FALSE(OH_AVFormat_GetStringValue(sourceFormat, OH_MD_KEY_COMMENT, &stringVal));
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0900
 * @tc.name      : demuxer wav, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0900, TestSize.Level2)
{
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0901
 * @tc.name      : demuxer mov, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0901, TestSize.Level2)
{
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0902
 * @tc.name      : demuxer avi, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0902, TestSize.Level2)
{
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0903
 * @tc.name      : demuxer mkv, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0903, TestSize.Level2)
{
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0904
 * @tc.name      : demuxer wav, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0904, TestSize.Level2)
{
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0905
 * @tc.name      : demuxer wav, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0905, TestSize.Level2)
{
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0906
 * @tc.name      : demuxer wav, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0906, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0907
 * @tc.name      : demuxer wav, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0907, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0908
 * @tc.name      : demuxer wav, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0908, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_0909
 * @tc.name      : demuxer wav, GetSourceFormat, OH_MD_KEY_TRACK_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_0909, TestSize.Level2)
{
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);

    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1000
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1000, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 128000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1001
 * @tc.name      : demuxer mov, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1001, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 176400);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1002
 * @tc.name      : demuxer avi, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1002, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 128000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1003
 * @tc.name      : demuxer mkv, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1003, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 128000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1004
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1004, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 32000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1005
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1005, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 187425);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1006
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1006, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 128000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1007
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1007, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 176400);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1008
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1008, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 128000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1009
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_BITRATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1009, TestSize.Level2)
{
    int64_t br = 0;
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetLongValue(trackFormat, OH_MD_KEY_BITRATE, &br));
    ASSERT_EQ(br, 128000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1100
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1100, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1101
 * @tc.name      : demuxer mov, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1101, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1102
 * @tc.name      : demuxer avi, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1102, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1103
 * @tc.name      : demuxer mkv, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1103, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1104
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1104, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1105
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1105, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1106
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1106, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1107
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1107, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1108
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1108, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1109
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_CHANNEL_COUNT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1109, TestSize.Level2)
{
    int32_t cc = 0;
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1200
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1200, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1201
 * @tc.name      : demuxer mov, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1201, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1202
 * @tc.name      : demuxer avi, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1202, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1203
 * @tc.name      : demuxer mkv, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1203, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1204
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1204, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 8000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1205
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1205, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1206
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1206, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1207
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1207, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1208
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1208, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1209
 * @tc.name      : demuxer wav, GetAudioTrackFormat, MD_KEY_SAMPLE_RATE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1209, TestSize.Level2)
{
    int32_t sr = 0;
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1400
 * @tc.name      : demuxer wav, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1400, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1401
 * @tc.name      : demuxer mov, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1401, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1402
 * @tc.name      : demuxer avi, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1402, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1403
 * @tc.name      : demuxer mkv, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1403, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1404
 * @tc.name      : demuxer wav, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1404, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1405
 * @tc.name      : demuxer wav, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1405, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1406
 * @tc.name      : demuxer wav, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1406, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1407
 * @tc.name      : demuxer wav, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1407, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1408
 * @tc.name      : demuxer wav, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1408, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1409
 * @tc.name      : demuxer wav, GetPublicTrackFormat, MD_KEY_TRACK_TYPE
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1409, TestSize.Level2)
{
    int32_t type = 0;
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &type));
    ASSERT_EQ(type, MEDIA_TYPE_AUD);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1500
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1500, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_1;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 54);
    ASSERT_EQ(aKeyCount, 54);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1501
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1501, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_2;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 1383);
    ASSERT_EQ(aKeyCount, 1383);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1502
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1502, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_3;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 1383);
    ASSERT_EQ(aKeyCount, 1383);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1503
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1503, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_4;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 1383);
    ASSERT_EQ(aKeyCount, 1383);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1504
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1504, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_5;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 10);
    ASSERT_EQ(aKeyCount, 10);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1505
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1505, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_QT_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 6912);
    ASSERT_EQ(aKeyCount, 6912);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1506
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1506, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_WAV_WAV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 55);
    ASSERT_EQ(aKeyCount, 55);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1507
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1507, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_WAV_MOV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 217);
    ASSERT_EQ(aKeyCount, 217);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1508
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1508, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_WAV_AVI;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 217);
    ASSERT_EQ(aKeyCount, 217);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1509
 * @tc.name      : create source with g_fd
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1509, TestSize.Level2)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    int audioFrame = 0;
    const char *file = INP_DIR_WAV_MKV;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    int aKeyCount = 0;
    while (!audioIsEnd) {
        for (int32_t index = 0; index < g_trackCount; index++) {
            trackFormat = OH_AVSource_GetTrackFormat(source, index);
            ASSERT_NE(trackFormat, nullptr);
            ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
            OH_AVFormat_Destroy(trackFormat);
            trackFormat = nullptr;
            if ((audioIsEnd && (tarckType == MEDIA_TYPE_AUD))) {
                continue;
            }
            ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_ReadSample(demuxer, index, memory, &attr));
            SetAudioValue(attr, audioIsEnd, audioFrame, aKeyCount);
        }
    }
    ASSERT_EQ(audioFrame, 217);
    ASSERT_EQ(aKeyCount, 217);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1600
 * @tc.name      : create source with g_fd, ADPCM_G722.wav
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1600, TestSize.Level3)
{
    DemuxerResult(INP_DIR_1);
    DemuxerResult(INP_DIR_2);
    DemuxerResult(INP_DIR_3);
    DemuxerResult(INP_DIR_4);
    DemuxerResult(INP_DIR_5);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1601
 * @tc.name      : create source with g_fd, ADPCM_IMA_QT
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1601, TestSize.Level3)
{
    DemuxerResult(INP_DIR_QT_MOV);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_1602
 * @tc.name      : create source with g_fd, ADPCM_IMA_WAV
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_1602, TestSize.Level3)
{
    DemuxerResult(INP_DIR_WAV_WAV);
    DemuxerResult(INP_DIR_WAV_MOV);
    DemuxerResult(INP_DIR_WAV_AVI);
    DemuxerResult(INP_DIR_WAV_MKV);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3200
 * @tc.name      : seek to the start time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 54};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 10};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 6912};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 55};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3300
 * @tc.name      : seek to the start time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 0, 29, 54};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 0, 29, 10};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_NEXT_SYNC, 0, 29, 6912};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_NEXT_SYNC, 0, 29, 55};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_NEXT_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_NEXT_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_NEXT_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3400
 * @tc.name      : seek to the start time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 0, 29, 54};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 0, 29, 1383};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 0, 29, 10};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_CLOSEST_SYNC, 0, 29, 6912};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_CLOSEST_SYNC, 0, 29, 55};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_CLOSEST_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_CLOSEST_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_CLOSEST_SYNC, 0, 29, 217};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3500
 * @tc.name      : seek to the end time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 51};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 1251};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 1383};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 1383};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 9};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 6247};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 51};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 197};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 196};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_PREVIOUS_SYNC, 966667, 1, 217};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3600
 * @tc.name      : seek to the end time, next mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 966667, 1, 51};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 966667, 1, 1249};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 966667, 1, 1382};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 966667, 1, 694};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 966667, 1, 9};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_NEXT_SYNC, 966667, 1, 6246};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_NEXT_SYNC, 966667, 1, 51};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_NEXT_SYNC, 966667, 1, 196};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_NEXT_SYNC, 966667, 1, 196};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_NEXT_SYNC, 966667, 1, 109};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3700
 * @tc.name      : seek to the end time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 51};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 1251};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 1383};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 1383};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 9};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 6247};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 51};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 197};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 196};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_CLOSEST_SYNC, 966667, 1, 217};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3800
 * @tc.name      : seek to the middle time, previous mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 53};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 1315};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 1383};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 1383};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 10};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 6568};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 53};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 207};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 206};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_PREVIOUS_SYNC, 500000, 15, 217};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_3900
 * @tc.name      : seek to the middle time, next mode
 * @tc.desc      : function testZHE
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_3900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 500000, 15, 53};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 500000, 15, 1314};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 500000, 15, 1382};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 500000, 15, 694};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 500000, 15, 10};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_NEXT_SYNC, 500000, 15, 6567};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_NEXT_SYNC, 500000, 15, 53};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_NEXT_SYNC, 500000, 15, 206};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_NEXT_SYNC, 500000, 15, 206};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_NEXT_SYNC, 500000, 15, 109};
    CheckSeekMode(fileTest10);
}

/**
 * @tc.number    : DEMUXER_ADPCM_FUNC_4000
 * @tc.name      : seek to the middle time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAdpcmFuncNdkTest, DEMUXER_ADPCM_FUNC_4000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 53};
    CheckSeekMode(fileTest1);
    seekInfo fileTest2{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 1315};
    CheckSeekMode(fileTest2);
    seekInfo fileTest3{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 1383};
    CheckSeekMode(fileTest3);
    seekInfo fileTest4{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 1383};
    CheckSeekMode(fileTest4);
    seekInfo fileTest5{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 10};
    CheckSeekMode(fileTest5);
    seekInfo fileTest6{INP_DIR_QT_MOV, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 6568};
    CheckSeekMode(fileTest6);
    seekInfo fileTest7{INP_DIR_WAV_WAV, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 53};
    CheckSeekMode(fileTest7);
    seekInfo fileTest8{INP_DIR_WAV_MOV, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 207};
    CheckSeekMode(fileTest8);
    seekInfo fileTest9{INP_DIR_WAV_AVI, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 206};
    CheckSeekMode(fileTest9);
    seekInfo fileTest10{INP_DIR_WAV_MKV, SEEK_MODE_CLOSEST_SYNC, 500000, 15, 217};
    CheckSeekMode(fileTest10);
}
