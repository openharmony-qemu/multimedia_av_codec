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

#include <fcntl.h>

#include <cmath>
#include <cstdio>
#include <iostream>
#include <string>
#include <thread>

#include "file_server_demo.h"
#include "gtest/gtest.h"
#include "native_avcodec_base.h"
#include "native_avdemuxer.h"
#include "native_avformat.h"
#include "native_avmemory.h"
#include "native_avsource.h"

using namespace OHOS::MediaAVCodec;
using namespace std;

namespace OHOS {
namespace Media {
class DemuxerDAT3FuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    const char *INP_DIR_1 = "/data/test/media/audio/ac3.dat";
    const char *INP_URI_1 = "http://127.0.0.1:46666/audio/ac3.dat";
    const char *INP_DIR_2 = "/data/test/media/audio/eac3.dat";
    const char *INP_URI_2 = "http://127.0.0.1:46666/audio/eac3.dat";
    const char *INP_DIR_3 = "/data/test/media/audio/amr_nb_8000_1_amr.dat";
    const char *INP_URI_3 = "http://127.0.0.1:46666/audio/amr_nb_8000_1_amr.dat";
    const char *INP_DIR_4 = "/data/test/media/audio/AAC_48000_1_aac.dat";
    const char *INP_URI_4 = "http://127.0.0.1:46666/audio/AAC_48000_1_aac.dat";
    const char *INP_DIR_5 = "/data/test/media/audio/MP3_48000_1_mp3.dat";
    const char *INP_URI_5 = "http://127.0.0.1:46666/audio/MP3_48000_1_mp3.dat";
    const char *INP_DIR_6 = "/data/test/media/audio/FLAC_48000_1_flac.dat";
    const char *INP_URI_6 = "http://127.0.0.1:46666/audio/FLAC_48000_1_flac.dat";
    const char *INP_DIR_7 = "/data/test/media/audio/OGG_48000_1_ogg.dat";
    const char *INP_URI_7 = "http://127.0.0.1:46666/audio/OGG_48000_1_ogg.dat";
    const char *INP_DIR_8 = "/data/test/media/audio/M4A_48000_1_m4a.dat";
    const char *INP_URI_8 = "http://127.0.0.1:46666/audio/M4A_48000_1_m4a.dat";
    const char *INP_DIR_9 = "/data/test/media/audio/wav_48000_1_wav.dat";
    const char *INP_URI_9 = "http://127.0.0.1:46666/audio/wav_48000_1_wav.dat";
    const char *INP_DIR_10 = "/data/test/media/audio/ape.dat";
    const char *INP_URI_10 = "http://127.0.0.1:46666/audio/ape.dat";
    const char *INP_DIR_11 = "/data/test/media/audio/aac_wma.dat";
    const char *INP_URI_11 = "http://127.0.0.1:46666/audio/aac_wma.dat";
    const char *INP_DIR_12 = "/data/test/media/audio/dts.dat";
    const char *INP_URI_12 = "http://127.0.0.1:46666/audio/dts.dat";
    const char *INP_DIR_13 = "/data/test/media/audio/alac_caf.dat";
    const char *INP_URI_13 = "http://127.0.0.1:46666/audio/alac_caf.dat";
    const char *INP_DIR_14 = "/data/test/media/audio/pcmS16be_44100_aiff.dat";
    const char *INP_URI_14 = "http://127.0.0.1:46666/audio/pcmS16be_44100_aiff.dat";
};

static unique_ptr<FileServerDemo> server = nullptr;
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

void DemuxerDAT3FuncNdkTest::SetUpTestCase()
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}

void DemuxerDAT3FuncNdkTest::TearDownTestCase()
{
    server->StopServer();
}

void DemuxerDAT3FuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}

void DemuxerDAT3FuncNdkTest::TearDown()
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
}  // namespace Media
}  // namespace OHOS

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

static void CreateFdSource(const char *fileName)
{
    g_fd = open(fileName, O_RDONLY);
    int64_t size = GetFileSize(fileName);
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    cout << fileName << "---fd:" << g_fd << "---size:" << size << "---source:" << source << endl;
}

static void CreateUriSource(const char *uri)
{
    cout << "URI:" << uri << endl;
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
}

static void Create(seekInfo seekInfo)
{
    if (memory == nullptr) {
        memory = OH_AVMemory_Create(g_width * g_height);
        g_trackCount = 0;
    }
    if (source == nullptr && ((strstr(seekInfo.fileName, "http") != nullptr))) {
        CreateUriSource(seekInfo.fileName);
    } else if (source == nullptr && ((strstr(seekInfo.fileName, "http") == nullptr))) {
        CreateFdSource(seekInfo.fileName);
    }
}

static void Destroy()
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

static void CheckSeekMode(seekInfo seekInfo)
{
    Create(seekInfo);
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_NE(sourceFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    cout << "Track count:" << g_trackCount << endl;

    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }

    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &tarckType));
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond, seekInfo.seekmode));
        OH_AVFormat_Destroy(trackFormat);
        trackFormat = nullptr;
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
            cout << "Video frame count:" << frameNum << endl;
            ASSERT_EQ(seekInfo.videoCount, frameNum);
        } else if (tarckType == MEDIA_TYPE_AUD) {
            cout << "Audio frame count:" << frameNum << endl;
            ASSERT_EQ(seekInfo.audioCount, frameNum);
        }
    }
    Destroy();
}

static void SetAudioValue(OH_AVCodecBufferAttr attr, bool &audioIsEnd, int &audioFrame, int &aKeyCount)
{
    if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_EOS) {
        audioIsEnd = true;
        cout << "Audio total frames:" << audioFrame << " | Audio end!" << endl;
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
        cout << "Video total frames:" << videoFrame << " | Video end!" << endl;
    } else {
        videoFrame++;
        if (attr.flags & OH_AVCodecBufferFlags::AVCODEC_BUFFER_FLAGS_SYNC_FRAME) {
            vKeyCount++;
        }
    }
}

static void DemuxerResult(int audioFramesCount)
{
    int tarckType = 0;
    OH_AVCodecBufferAttr attr;
    bool audioIsEnd = false;
    bool videoIsEnd = false;
    int audioFrame = 0;
    int videoFrame = 0;
    ASSERT_NE(source, nullptr);
    
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));

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

    cout << "Audio key frames:" << aKeyCount << " | Video key frames:" << vKeyCount << endl;
    ASSERT_EQ(audioFramesCount, audioFrame);
    Destroy();
}
/**
 * @tc.number    : DEMUXER_DAT_FUNC_0610
 * @tc.name      : demuxer DAT, GetTrackFormat, Local ac3.dat(ac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0610, TestSize.Level2)
{
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0620
 * @tc.name      : demuxer DAT, GetTrackFormat, URI ac3.dat(ac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0620, TestSize.Level2)
{
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0630
 * @tc.name      : demuxer DAT, Read Seek with Local ac3.dat(ac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0630, TestSize.Level2)
{
    CreateFdSource(INP_DIR_1);
    DemuxerResult(317);
    seekInfo fileTestDirPrevious{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 317};
    seekInfo fileTestDirClosest{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 4500, 0, 188};
    seekInfo fileTestDirNext{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 10000, 0, 29};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0640
 * @tc.name      : demuxer DAT, Read Seek with URI ac3.dat(ac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0640, TestSize.Level2)
{
    CreateUriSource(INP_URI_1);
    DemuxerResult(317);
    seekInfo fileTestUriPrevious{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 317};
    seekInfo fileTestUriClosest{INP_URI_1, SEEK_MODE_CLOSEST_SYNC, 4500, 0, 188};
    seekInfo fileTestUriNext{INP_URI_1, SEEK_MODE_NEXT_SYNC, 10000, 0, 29};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

#ifdef SUPPORT_DEMUXER_EAC3
/**
 * @tc.number    : DEMUXER_DAT_FUNC_0650
 * @tc.name      : demuxer DAT, GetTrackFormat, Local eac3.dat(eac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0650, TestSize.Level2)
{
    CreateFdSource(INP_DIR_2);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0660
 * @tc.name      : demuxer DAT, GetTrackFormat, eac3.dat(eac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0660, TestSize.Level2)
{
    CreateUriSource(INP_URI_2);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0670
 * @tc.name      : demuxer DAT, Read Seek with Local eac3.dat(eac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0670, TestSize.Level2)
{
    CreateFdSource(INP_DIR_2);
    DemuxerResult(317);
    seekInfo fileTestDirPrevious{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 317};
    seekInfo fileTestDirClosest{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 4500, 0, 188};
    seekInfo fileTestDirNext{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 10000, 0, 29};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0680
 * @tc.name      : demuxer DAT, Read Seek with URI eac3.dat(eac3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0680, TestSize.Level2)
{
    CreateUriSource(INP_URI_2);
    DemuxerResult(317);
    seekInfo fileTestUriPrevious{INP_URI_2, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 317};
    seekInfo fileTestUriClosest{INP_URI_2, SEEK_MODE_CLOSEST_SYNC, 4500, 0, 188};
    seekInfo fileTestUriNext{INP_URI_2, SEEK_MODE_NEXT_SYNC, 10000, 0, 29};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}
#endif

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0690
 * @tc.name      : demuxer DAT, GetTrackFormat, Local amr_nb_8000_1_amr.dat(amr)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0690, TestSize.Level2)
{
    CreateFdSource(INP_DIR_3);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 8000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0700
 * @tc.name      : demuxer DAT, GetTrackFormat, URI amr_nb_8000_1_amr.dat(amr)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0700, TestSize.Level2)
{
    CreateUriSource(INP_URI_3);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 8000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0710
 * @tc.name      : demuxer DAT, Read Seek with Local amr_nb_8000_1_amr.dat(amr)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0710, TestSize.Level2)
{
    CreateFdSource(INP_DIR_3);
    DemuxerResult(1501);
    seekInfo fileTestDirPrevious{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 1501};
    seekInfo fileTestDirClosest{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 15000, 0, 751};
    seekInfo fileTestDirNext{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 30000, 0, 1};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0720
 * @tc.name      : demuxer DAT, Read Seek with URI amr_nb_8000_1_amr.dat(amr)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0720, TestSize.Level2)
{
    CreateUriSource(INP_URI_3);
    DemuxerResult(1501);
    seekInfo fileTestUriPrevious{INP_URI_3, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 1501};
    seekInfo fileTestUriClosest{INP_URI_3, SEEK_MODE_CLOSEST_SYNC, 15000, 0, 751};
    seekInfo fileTestUriNext{INP_URI_3, SEEK_MODE_NEXT_SYNC, 30000, 0, 1};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0730
 * @tc.name      : demuxer DAT, GetTrackFormat, Local AAC_48000_1_aac.dat(aac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0730, TestSize.Level2)
{
    CreateFdSource(INP_DIR_4);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0740
 * @tc.name      : demuxer DAT, GetTrackFormat, URI AAC_48000_1_aac.dat(aac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0740, TestSize.Level2)
{
    CreateUriSource(INP_URI_4);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0750
 * @tc.name      : demuxer DAT, Read Seek with Local AAC_48000_1_aac.dat(aac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0750, TestSize.Level2)
{
    CreateFdSource(INP_DIR_4);
    DemuxerResult(9457);
    seekInfo fileTestDirPrevious{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 9457};
    seekInfo fileTestDirClosest{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 109784, 0, 4729};
    seekInfo fileTestDirNext{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 219567, 0, 2};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0760
 * @tc.name      : demuxer DAT, Read Seek with URI AAC_48000_1_aac.dat(aac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0760, TestSize.Level2)
{
    CreateUriSource(INP_URI_4);
    DemuxerResult(9457);
    seekInfo fileTestUriPrevious{INP_URI_4, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 9457};
    seekInfo fileTestUriClosest{INP_URI_4, SEEK_MODE_CLOSEST_SYNC, 109784, 0, 4729};
    seekInfo fileTestUriNext{INP_URI_4, SEEK_MODE_NEXT_SYNC, 219567, 0, 2};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0770
 * @tc.name      : demuxer DAT, GetTrackFormat, Local MP3_48000_1_mp3.dat(mp3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0770, TestSize.Level2)
{
    CreateFdSource(INP_DIR_5);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0780
 * @tc.name      : demuxer DAT, GetTrackFormat, URI MP3_48000_1_mp3.dat(mp3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0780, TestSize.Level2)
{
    CreateUriSource(INP_URI_5);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0790
 * @tc.name      : demuxer DAT, Read Seek with Local MP3_48000_1_mp3.dat(mp3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0790, TestSize.Level2)
{
    CreateFdSource(INP_DIR_5);
    DemuxerResult(9150);
    seekInfo fileTestDirPrevious{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 9150};
    seekInfo fileTestDirClosest{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 109800, 0, 4575};
    seekInfo fileTestDirNext{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 219576, 0, 15};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0800
 * @tc.name      : demuxer DAT, Read Seek with URI MP3_48000_1_mp3.dat(mp3)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0800, TestSize.Level2)
{
    CreateUriSource(INP_URI_5);
    DemuxerResult(9150);
    seekInfo fileTestUriPrevious{INP_URI_5, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 9150};
    seekInfo fileTestUriClosest{INP_URI_5, SEEK_MODE_CLOSEST_SYNC, 109800, 0, 4575};
    seekInfo fileTestUriNext{INP_URI_5, SEEK_MODE_NEXT_SYNC, 219576, 0, 15};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0810
 * @tc.name      : demuxer DAT, GetTrackFormat, Local FLAC_48000_1_flac.dat(flac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0810, TestSize.Level2)
{
    CreateFdSource(INP_DIR_6);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0820
 * @tc.name      : demuxer DAT, GetTrackFormat, URI FLAC_48000_1_flac.dat(flac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0820, TestSize.Level2)
{
    CreateUriSource(INP_URI_6);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0830
 * @tc.name      : demuxer DAT, Read Seek with Local FLAC_48000_1_flac.dat(flac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0830, TestSize.Level2)
{
    CreateFdSource(INP_DIR_6);
    DemuxerResult(2288);
    seekInfo fileTestDirPrevious{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 2288};
    seekInfo fileTestDirClosest{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 109824, 0, 1145};
    seekInfo fileTestDirNext{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 219552, 0, 2};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0840
 * @tc.name      : demuxer DAT, Read Seek with URI FLAC_48000_1_flac.dat(flac)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0840, TestSize.Level2)
{
    CreateUriSource(INP_URI_6);
    DemuxerResult(2288);
    seekInfo fileTestUriPrevious{INP_URI_6, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 2288};
    seekInfo fileTestUriClosest{INP_URI_6, SEEK_MODE_CLOSEST_SYNC, 109824, 0, 1145};
    seekInfo fileTestUriNext{INP_URI_6, SEEK_MODE_NEXT_SYNC, 219552, 0, 2};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0850
 * @tc.name      : demuxer DAT, GetTrackFormat, Local OGG_48000_1_ogg.dat(ogg)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0850, TestSize.Level2)
{
    CreateFdSource(INP_DIR_7);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0860
 * @tc.name      : demuxer DAT, GetTrackFormat, URI OGG_48000_1_ogg.dat(ogg)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0860, TestSize.Level2)
{
    CreateUriSource(INP_URI_7);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0870
 * @tc.name      : demuxer DAT, Read Seek with Local OGG_48000_1_ogg.dat(ogg)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0870, TestSize.Level2)
{
    CreateFdSource(INP_DIR_7);
    DemuxerResult(11439);
    seekInfo fileTestDirPrevious{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 11439};
    seekInfo fileTestDirClosest{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 104726, 0, 5724};
    seekInfo fileTestDirNext{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 219545, 0, 66};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0880
 * @tc.name      : demuxer DAT, Read Seek with URI OGG_48000_1_ogg.dat(ogg)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0880, TestSize.Level2)
{
    CreateUriSource(INP_URI_7);
    DemuxerResult(11439);
    seekInfo fileTestUriPrevious{INP_URI_7, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 11439};
    seekInfo fileTestUriClosest{INP_URI_7, SEEK_MODE_CLOSEST_SYNC, 104726, 0, 5724};
    seekInfo fileTestUriNext{INP_URI_7, SEEK_MODE_NEXT_SYNC, 219545, 0, 66};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0890
 * @tc.name      : demuxer DAT, GetTrackFormat, Local M4A_48000_1_m4a.dat(m4a)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0890, TestSize.Level2)
{
    CreateFdSource(INP_DIR_8);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0900
 * @tc.name      : demuxer DAT, GetTrackFormat, M4A_48000_1_m4a.dat(m4a)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0900, TestSize.Level2)
{
    CreateUriSource(INP_URI_8);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0910
 * @tc.name      : demuxer DAT, Read Seek with Local M4A_48000_1_m4a.dat(m4a)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0910, TestSize.Level2)
{
    CreateFdSource(INP_DIR_8);
    DemuxerResult(10293);
    seekInfo fileTestDirPrevious{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 10292};
    seekInfo fileTestDirClosest{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 109781, 0, 5147};
    seekInfo fileTestDirNext{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 219541, 0, 2};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0920
 * @tc.name      : demuxer DAT, Read Seek with URI M4A_48000_1_m4a.dat(m4a)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0920, TestSize.Level2)
{
    CreateUriSource(INP_URI_8);
    DemuxerResult(10293);
    seekInfo fileTestUriPrevious{INP_URI_8, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 10292};
    seekInfo fileTestUriClosest{INP_URI_8, SEEK_MODE_CLOSEST_SYNC, 109781, 0, 5147};
    seekInfo fileTestUriNext{INP_URI_8, SEEK_MODE_NEXT_SYNC, 219541, 0, 2};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0930
 * @tc.name      : demuxer DAT, GetTrackFormat, Local wav_48000_1_wav.dat(wav)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0930, TestSize.Level2)
{
    CreateFdSource(INP_DIR_9);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0940
 * @tc.name      : demuxer DAT, GetTrackFormat, wav_48000_1_wav.dat(wav)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0940, TestSize.Level2)
{
    CreateUriSource(INP_URI_9);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0950
 * @tc.name      : demuxer DAT, Read Seek with Local wav_48000_1_wav.dat(wav)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0950, TestSize.Level2)
{
    CreateFdSource(INP_DIR_9);
    DemuxerResult(5146);
    seekInfo fileTestDirPrevious{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 5146};
    seekInfo fileTestDirClosest{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 109781, 0, 2573};
    seekInfo fileTestDirNext{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 219520, 0, 1};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0960
 * @tc.name      : demuxer DAT, Read Seek with URI wav_48000_1_wav.dat(wav)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0960, TestSize.Level2)
{
    CreateUriSource(INP_URI_9);
    DemuxerResult(5146);
    seekInfo fileTestUriPrevious{INP_URI_9, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 5146};
    seekInfo fileTestUriClosest{INP_URI_9, SEEK_MODE_CLOSEST_SYNC, 109781, 0, 2573};
    seekInfo fileTestUriNext{INP_URI_9, SEEK_MODE_NEXT_SYNC, 219520, 0, 1};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0970
 * @tc.name      : demuxer DAT, GetTrackFormat, Local ape.dat(ape)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0970, TestSize.Level2)
{
    CreateFdSource(INP_DIR_10);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0980
 * @tc.name      : demuxer DAT, GetTrackFormat, ape.dat(ape)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0980, TestSize.Level2)
{
    CreateUriSource(INP_URI_10);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0990
 * @tc.name      : demuxer DAT, Read Seek with Local ape.dat(ape)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_0990, TestSize.Level2)
{
    CreateFdSource(INP_DIR_10);
    DemuxerResult(8);
    seekInfo fileTestDirPrevious{INP_DIR_10, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 8};
    seekInfo fileTestDirClosest{INP_DIR_10, SEEK_MODE_CLOSEST_SYNC, 6144, 0, 4};
    seekInfo fileTestDirNext{INP_DIR_10, SEEK_MODE_NEXT_SYNC, 10752, 0, 1};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1000
 * @tc.name      : demuxer DAT, Read Seek with URI ape.dat(ape)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1000, TestSize.Level2)
{
    CreateUriSource(INP_URI_10);
    DemuxerResult(8);
    seekInfo fileTestUriPrevious{INP_URI_10, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 8};
    seekInfo fileTestUriClosest{INP_URI_10, SEEK_MODE_CLOSEST_SYNC, 6144, 0, 4};
    seekInfo fileTestUriNext{INP_URI_10, SEEK_MODE_NEXT_SYNC, 10752, 0, 1};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1010
 * @tc.name      : demuxer DAT, GetTrackFormat, Local aac_wma.dat(wma)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1010, TestSize.Level2)
{
    CreateFdSource(INP_DIR_11);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1020
 * @tc.name      : demuxer DAT, GetTrackFormat, aac_wma.dat(wma)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1020, TestSize.Level2)
{
    CreateUriSource(INP_URI_11);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1030
 * @tc.name      : demuxer DAT, Read Seek with Local aac_wma.dat(wma)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1030, TestSize.Level2)
{
    CreateFdSource(INP_DIR_11);
    DemuxerResult(433);
    seekInfo fileTestDirPrevious{INP_DIR_11, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 433};
    seekInfo fileTestDirClosest{INP_DIR_11, SEEK_MODE_CLOSEST_SYNC, 5000, 0, 220};
    seekInfo fileTestDirNext{INP_DIR_11, SEEK_MODE_NEXT_SYNC, 10031, 0, 10};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1040
 * @tc.name      : demuxer DAT, Read Seek with URI aac_wma.dat(wma)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1040, TestSize.Level2)
{
    CreateUriSource(INP_URI_11);
    DemuxerResult(433);
    seekInfo fileTestUriPrevious{INP_URI_11, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 433};
    seekInfo fileTestUriClosest{INP_URI_11, SEEK_MODE_CLOSEST_SYNC, 5000, 0, 220};
    seekInfo fileTestUriNext{INP_URI_11, SEEK_MODE_NEXT_SYNC, 10031, 0, 10};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1050
 * @tc.name      : demuxer DAT, GetTrackFormat, Local dts.dat(dts)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1050, TestSize.Level2)
{
    CreateFdSource(INP_DIR_12);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1060
 * @tc.name      : demuxer DAT, GetTrackFormat, dts.dat(dts)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1060, TestSize.Level2)
{
    CreateUriSource(INP_URI_12);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1070
 * @tc.name      : demuxer DAT, Read Seek with Local dts.dat(dts)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1070, TestSize.Level2)
{
    CreateFdSource(INP_DIR_12);
    DemuxerResult(469);
    seekInfo fileTestDirPrevious{INP_DIR_12, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 469};
    seekInfo fileTestDirClosest{INP_DIR_12, SEEK_MODE_CLOSEST_SYNC, 1000, 0, 376};
    seekInfo fileTestDirNext{INP_DIR_12, SEEK_MODE_NEXT_SYNC, 4500, 0, 47};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1080
 * @tc.name      : demuxer DAT, Read Seek with URI dts.dat(dts)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1080, TestSize.Level2)
{
    CreateUriSource(INP_URI_12);
    DemuxerResult(469);
    seekInfo fileTestUriPrevious{INP_URI_12, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 469};
    seekInfo fileTestUriClosest{INP_URI_12, SEEK_MODE_CLOSEST_SYNC, 1000, 0, 376};
    seekInfo fileTestUriNext{INP_URI_12, SEEK_MODE_NEXT_SYNC, 4500, 0, 47};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1090
 * @tc.name      : demuxer DAT, GetTrackFormat, Local alac_caf.dat(caf)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1090, TestSize.Level2)
{
    CreateFdSource(INP_DIR_13);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1100
 * @tc.name      : demuxer DAT, GetTrackFormat, alac_caf.dat(caf)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1100, TestSize.Level2)
{
    CreateUriSource(INP_URI_13);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1110
 * @tc.name      : demuxer DAT, Read Seek with Local alac_caf.dat(caf)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1110, TestSize.Level2)
{
    CreateFdSource(INP_DIR_13);
    DemuxerResult(108);
    seekInfo fileTestDirPrevious{INP_DIR_13, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 108};
    seekInfo fileTestDirClosest{INP_DIR_13, SEEK_MODE_CLOSEST_SYNC, 2000, 0, 87};
    seekInfo fileTestDirNext{INP_DIR_13, SEEK_MODE_NEXT_SYNC, 6000, 0, 43};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1120
 * @tc.name      : demuxer DAT, Read Seek with URI alac_caf.dat(caf)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1120, TestSize.Level2)
{
    CreateUriSource(INP_URI_13);
    DemuxerResult(108);
    seekInfo fileTestUriPrevious{INP_URI_13, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 108};
    seekInfo fileTestUriClosest{INP_URI_13, SEEK_MODE_CLOSEST_SYNC, 2000, 0, 87};
    seekInfo fileTestUriNext{INP_URI_13, SEEK_MODE_NEXT_SYNC, 6000, 0, 43};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1130
 * @tc.name      : demuxer DAT, GetTrackFormat, Local pcmS16be_44100_aiff.dat(aiff)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1130, TestSize.Level2)
{
    CreateFdSource(INP_DIR_14);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1140
 * @tc.name      : demuxer DAT, GetTrackFormat, pcmS16be_44100_aiff.dat(aiff)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1140, TestSize.Level2)
{
    CreateUriSource(INP_URI_14);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, 1);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t channelCount = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1150
 * @tc.name      : demuxer DAT, Read Seek with Local pcmS16be_44100_aiff.dat(aiff)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1150, TestSize.Level2)
{
    CreateFdSource(INP_DIR_14);
    DemuxerResult(1292);
    seekInfo fileTestDirPrevious{INP_DIR_14, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 1292};
    seekInfo fileTestDirClosest{INP_DIR_14, SEEK_MODE_CLOSEST_SYNC, 7000, 0, 991};
    seekInfo fileTestDirNext{INP_DIR_14, SEEK_MODE_NEXT_SYNC, 15000, 0, 646};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_1160
 * @tc.name      : demuxer DAT, Read Seek with URI pcmS16be_44100_aiff.dat(aiff)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT3FuncNdkTest, DEMUXER_DAT_FUNC_1160, TestSize.Level2)
{
    CreateUriSource(INP_URI_14);
    DemuxerResult(1292);
    seekInfo fileTestUriPrevious{INP_URI_14, SEEK_MODE_PREVIOUS_SYNC, 0, 0, 1292};
    seekInfo fileTestUriClosest{INP_URI_14, SEEK_MODE_CLOSEST_SYNC, 7000, 0, 991};
    seekInfo fileTestUriNext{INP_URI_14, SEEK_MODE_NEXT_SYNC, 15000, 0, 646};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
    close(g_fd);
    g_fd = -1;
}