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
class DemuxerDAT2FuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    const char *INP_DIR_1 = "/data/test/media/aac_mpeg4_mp4.dat";
    const char *INP_URI_1 = "http://127.0.0.1:46666/aac_mpeg4_mp4.dat";
    const char *INP_DIR_2 = "/data/test/media/mp3_h264_ts.dat";
    const char *INP_URI_2 = "http://127.0.0.1:46666/mp3_h264_ts.dat";
    const char *INP_DIR_3 = "/data/test/media/mkv.dat";
    const char *INP_URI_3 = "http://127.0.0.1:46666/mkv.dat";
    const char *INP_DIR_4 = "/data/test/media/av1_vorbis_webm.dat";
    const char *INP_URI_4 = "http://127.0.0.1:46666/av1_vorbis_webm.dat";
    const char *INP_DIR_5 = "/data/test/media/avc_mp3_flv.dat";
    const char *INP_URI_5 = "http://127.0.0.1:46666/avc_mp3_flv.dat";
    const char *INP_DIR_6 = "/data/test/media/adpcm_yamaha_avi.dat";
    const char *INP_URI_6 = "http://127.0.0.1:46666/adpcm_yamaha_avi.dat";
    const char *INP_DIR_7 = "/data/test/media/adpcm_yamaha_mov.dat";
    const char *INP_URI_7 = "http://127.0.0.1:46666/adpcm_yamaha_mov.dat";
    const char *INP_DIR_8 = "/data/test/media/mpeg_h264_mp3_mpeg.dat";
    const char *INP_URI_8 = "http://127.0.0.1:46666/mpeg_h264_mp3_mpeg.dat";
    const char *INP_DIR_9 = "/data/test/media/rv30_cook_rm.dat";
    const char *INP_URI_9 = "http://127.0.0.1:46666/rv30_cook_rm.dat";
    const char *INP_DIR_10 = "/data/test/media/rv40_cook_rmvb.dat";
    const char *INP_URI_10 = "http://127.0.0.1:46666/rv40_cook_rmvb.dat";
    const char *INP_DIR_11 = "/data/test/media/m4v_h264_alac_m4v.dat";
    const char *INP_URI_11 = "http://127.0.0.1:46666/m4v_h264_alac_m4v.dat";
    const char *INP_DIR_12 = "/data/test/media/wmv_h264_wmav1_wmv.dat";
    const char *INP_URI_12 = "http://127.0.0.1:46666/wmv_h264_wmav1_wmv.dat";
    const char *INP_DIR_13 = "/data/test/media/mpeg2_ac3_vob.dat";
    const char *INP_URI_13 = "http://127.0.0.1:46666/mpeg2_ac3_vob.dat";
    const char *INP_DIR_14 = "/data/test/media/3g2_h263_aac_3g2.dat";
    const char *INP_URI_14 = "http://127.0.0.1:46666/3g2_h263_aac_3g2.dat";
    const char *INP_DIR_15 = "/data/test/media/3gp_h264_aac_3gp.dat";
    const char *INP_URI_15 = "http://127.0.0.1:46666/3gp_h264_aac_3gp.dat";
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
constexpr int32_t TWO = 2;
const std::string HEVC_LIB_PATH = std::string(AV_CODEC_PATH) + "/libav_codec_hevc_parser.z.so";

void DemuxerDAT2FuncNdkTest::SetUpTestCase()
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}

void DemuxerDAT2FuncNdkTest::TearDownTestCase()
{
    server->StopServer();
}

void DemuxerDAT2FuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}

void DemuxerDAT2FuncNdkTest::TearDown()
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

static void CheckSeekMode(seekInfo seekInfo)
{
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

static void DemuxerResult(int videoFramesCount, int audioFramesCount)
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
    while (!audioIsEnd || !videoIsEnd) {
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
    ASSERT_EQ(videoFramesCount, videoFrame);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0010
 * @tc.name      : demuxer DAT, GetTrackFormat, Local aac_mpeg4_mp4.dat(mp4)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0010, TestSize.Level1)
{
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0020
 * @tc.name      : demuxer DAT, GetTrackFormat, URI aac_mpeg4_mp4.dat(mp4)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0020, TestSize.Level1)
{
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0030
 * @tc.name      : demuxer DAT, Read Seek with Local aac_mpeg4_mp4.dat(mp4)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0030, TestSize.Level1)
{
    CreateFdSource(INP_DIR_1);
    DemuxerResult(372, 528);
    seekInfo fileTestDirPrevious{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525};
    seekInfo fileTestDirClosest{INP_DIR_1, SEEK_MODE_CLOSEST_SYNC, 5000, 228, 323};
    seekInfo fileTestDirNext{INP_DIR_1, SEEK_MODE_NEXT_SYNC, 12000, 12, 15};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0040
 * @tc.name      : demuxer DAT, Read Seek with URI aac_mpeg4_mp4.dat(mp4)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0040, TestSize.Level1)
{
    CreateUriSource(INP_URI_1);
    DemuxerResult(372, 528);
    seekInfo fileTestUriPrevious{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 0, 372, 525};
    seekInfo fileTestUriClosest{INP_URI_1, SEEK_MODE_CLOSEST_SYNC, 5000, 228, 323};
    seekInfo fileTestUriNext{INP_URI_1, SEEK_MODE_NEXT_SYNC, 12000, 12, 15};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0050
 * @tc.name      : demuxer DAT, GetTrackFormat, Local mp3_h264_ts.dat(MPEGTS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0050, TestSize.Level1)
{
    CreateFdSource(INP_DIR_2);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0060
 * @tc.name      : demuxer DAT, GetTrackFormat, URI mp3_h264_ts.dat(MPEGTS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0060, TestSize.Level1)
{
    CreateUriSource(INP_URI_2);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0070
 * @tc.name      : demuxer DAT, Read Seek with Local mp3_h264_ts.dat(MPEGTS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0070, TestSize.Level1)
{
    CreateFdSource(INP_DIR_2);
    DemuxerResult(372, 468);
    seekInfo fileTestDirPrevious{INP_DIR_2, SEEK_MODE_PREVIOUS_SYNC, 0, 372, 468};
    seekInfo fileTestDirClosest{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 5000, 220, 279};
    seekInfo fileTestDirNext{INP_DIR_2, SEEK_MODE_NEXT_SYNC, 12000, 10, 20};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0080
 * @tc.name      : demuxer DAT, Read Seek with URI mp3_h264_ts.dat(MPEGTS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0080, TestSize.Level1)
{
    CreateUriSource(INP_URI_2);
    DemuxerResult(372, 468);
    seekInfo fileTestUriPrevious{INP_URI_2, SEEK_MODE_PREVIOUS_SYNC, 0, 372, 468};
    seekInfo fileTestUriClosest{INP_URI_2, SEEK_MODE_CLOSEST_SYNC, 5000, 220, 279};
    seekInfo fileTestUriNext{INP_URI_2, SEEK_MODE_NEXT_SYNC, 12000, 10, 20};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0090
 * @tc.name      : demuxer DAT, GetTrackFormat, Local mkv.dat(mkv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0090, TestSize.Level1)
{
    CreateFdSource(INP_DIR_3);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0100
 * @tc.name      : demuxer DAT, GetTrackFormat, URI mkv.dat(mkv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0100, TestSize.Level1)
{
    CreateUriSource(INP_URI_3);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0110
 * @tc.name      : demuxer DAT, Read Seek with Local mkv.dat(mkv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0110, TestSize.Level1)
{
    CreateFdSource(INP_DIR_3);
    DemuxerResult(600, 431);
    seekInfo fileTestDirPrevious{INP_DIR_3, SEEK_MODE_PREVIOUS_SYNC, 0, 600, 431};
    seekInfo fileTestDirClosest{INP_DIR_3, SEEK_MODE_CLOSEST_SYNC, 5000, 300, 215};
    seekInfo fileTestDirNext{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 9000, 60, 43};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0120
 * @tc.name      : demuxer DAT, Read Seek with URI mkv.dat(mkv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0120, TestSize.Level1)
{
    CreateUriSource(INP_URI_3);
    DemuxerResult(600, 431);
    seekInfo fileTestUriPrevious{INP_URI_3, SEEK_MODE_PREVIOUS_SYNC, 0, 600, 431};
    seekInfo fileTestUriClosest{INP_URI_3, SEEK_MODE_CLOSEST_SYNC, 5000, 300, 215};
    seekInfo fileTestUriNext{INP_URI_3, SEEK_MODE_NEXT_SYNC, 9000, 60, 43};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0130
 * @tc.name      : demuxer DAT, GetTrackFormat, Local av1_vorbis_webm.dat(webm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0130, TestSize.Level1)
{
    CreateFdSource(INP_DIR_4);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0140
 * @tc.name      : demuxer DAT, GetTrackFormat, URI av1_vorbis_webm.dat(webm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0140, TestSize.Level1)
{
    CreateUriSource(INP_URI_4);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0150
 * @tc.name      : demuxer DAT, Read Seek with Local av1_vorbis_webm.dat(webm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0150, TestSize.Level1)
{
    CreateFdSource(INP_DIR_4);
    DemuxerResult(179, 130);
    seekInfo fileTestDirPrevious{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 0, 179, 129};
    seekInfo fileTestDirClosest{INP_DIR_4, SEEK_MODE_CLOSEST_SYNC, 2995, 29, 21};
    seekInfo fileTestDirNext{INP_DIR_4, SEEK_MODE_NEXT_SYNC, 2500, 29, 21};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0160
 * @tc.name      : demuxer DAT, Read Seek with URI av1_vorbis_webm.dat(webm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0160, TestSize.Level1)
{
    CreateUriSource(INP_URI_4);
    DemuxerResult(179, 130);
    seekInfo fileTestUriPrevious{INP_URI_4, SEEK_MODE_PREVIOUS_SYNC, 0, 179, 129};
    seekInfo fileTestUriClosest{INP_URI_4, SEEK_MODE_CLOSEST_SYNC, 2995, 29, 21};
    seekInfo fileTestUriNext{INP_URI_4, SEEK_MODE_NEXT_SYNC, 2500, 29, 21};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0170
 * @tc.name      : demuxer DAT, GetTrackFormat, Local avc_mp3_flv.dat(flv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0170, TestSize.Level1)
{
    CreateFdSource(INP_DIR_5);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0180
 * @tc.name      : demuxer DAT, GetTrackFormat, URI avc_mp3_flv.dat(flv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0180, TestSize.Level1)
{
    CreateUriSource(INP_URI_5);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0190
 * @tc.name      : demuxer DAT, Read Seek with Local avc_mp3_flv.dat(flv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0190, TestSize.Level1)
{
    CreateFdSource(INP_DIR_5);
    DemuxerResult(602, 385);
    seekInfo fileTestDirPrevious{INP_DIR_5, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 384};
    seekInfo fileTestDirClosest{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 5042, 352, 225};
    seekInfo fileTestDirNext{INP_DIR_5, SEEK_MODE_NEXT_SYNC, 5042, 102, 65};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0200
 * @tc.name      : demuxer DAT, Read Seek with URI avc_mp3_flv.dat(flv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0200, TestSize.Level1)
{
    CreateUriSource(INP_URI_5);
    DemuxerResult(602, 385);
    seekInfo fileTestUriPrevious{INP_URI_5, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 384};
    seekInfo fileTestUriClosest{INP_URI_5, SEEK_MODE_CLOSEST_SYNC, 5042, 352, 225};
    seekInfo fileTestUriNext{INP_URI_5, SEEK_MODE_NEXT_SYNC, 5042, 102, 65};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0210
 * @tc.name      : demuxer DAT, GetTrackFormat, Local adpcm_yamaha_avi.dat(avi)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0210, TestSize.Level2)
{
    CreateFdSource(INP_DIR_6);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 288);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 352);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0220
 * @tc.name      : demuxer DAT, GetTrackFormat, URI adpcm_yamaha_avi.dat(avi)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0220, TestSize.Level2)
{
    CreateUriSource(INP_URI_6);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 288);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 352);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0230
 * @tc.name      : demuxer DAT, Read Seek with Local adpcm_yamaha_avi.dat(avi)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0230, TestSize.Level2)
{
    CreateFdSource(INP_DIR_6);
    DemuxerResult(29, 47);
    seekInfo fileTestDirPrevious{INP_DIR_6, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 47};
    seekInfo fileTestDirClosest{INP_DIR_6, SEEK_MODE_CLOSEST_SYNC, 500, 17, 27};
    seekInfo fileTestDirNext{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 700, 5, 8};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0240
 * @tc.name      : demuxer DAT, Read Seek with URI adpcm_yamaha_avi.dat(avi)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0240, TestSize.Level2)
{
    CreateUriSource(INP_URI_6);
    DemuxerResult(29, 47);
    seekInfo fileTestUriPrevious{INP_URI_6, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 47};
    seekInfo fileTestUriClosest{INP_URI_6, SEEK_MODE_CLOSEST_SYNC, 500, 17, 27};
    seekInfo fileTestUriNext{INP_URI_6, SEEK_MODE_NEXT_SYNC, 700, 5, 8};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0250
 * @tc.name      : demuxer DAT, GetTrackFormat, Local adpcm_yamaha_mov.dat(mov)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0250, TestSize.Level2)
{
    CreateFdSource(INP_DIR_7);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 288);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 352);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0260
 * @tc.name      : demuxer DAT, GetTrackFormat, URI adpcm_yamaha_mov.dat(mov)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0260, TestSize.Level2)
{
    CreateUriSource(INP_URI_7);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 288);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 352);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0270
 * @tc.name      : demuxer DAT, Read Seek with Local adpcm_yamaha_mov.dat(mov)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0270, TestSize.Level2)
{
    CreateFdSource(INP_DIR_7);
    DemuxerResult(29, 47);
    seekInfo fileTestDirPrevious{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 47};
    seekInfo fileTestDirClosest{INP_DIR_7, SEEK_MODE_CLOSEST_SYNC, 500, 29, 47};
    seekInfo fileTestDirNext{INP_DIR_7, SEEK_MODE_NEXT_SYNC, 0, 29, 47};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0280
 * @tc.name      : demuxer DAT, Read Seek with URI adpcm_yamaha_mov.dat(mov)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0280, TestSize.Level2)
{
    CreateUriSource(INP_URI_7);
    DemuxerResult(29, 47);
    seekInfo fileTestUriPrevious{INP_URI_7, SEEK_MODE_PREVIOUS_SYNC, 0, 29, 47};
    seekInfo fileTestUriClosest{INP_URI_7, SEEK_MODE_CLOSEST_SYNC, 500, 29, 47};
    seekInfo fileTestUriNext{INP_URI_7, SEEK_MODE_NEXT_SYNC, 0, 29, 47};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0290
 * @tc.name      : demuxer DAT, GetTrackFormat, Local mpeg_h264_mp3_mpeg.dat(MPEGPS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0290, TestSize.Level2)
{
    CreateFdSource(INP_DIR_8);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0300
 * @tc.name      : demuxer DAT, GetTrackFormat, URI mpeg_h264_mp3_mpeg.dat(MPEGPS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0300, TestSize.Level2)
{
    CreateUriSource(INP_URI_8);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 1080);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 1920);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0310
 * @tc.name      : demuxer DAT, Read Seek with Local mpeg_h264_mp3_mpeg.dat(MPEGPS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0310, TestSize.Level2)
{
    CreateFdSource(INP_DIR_8);
    DemuxerResult(1253, 2165);
    seekInfo fileTestDirPrevious{INP_DIR_8, SEEK_MODE_PREVIOUS_SYNC, 0, 1253, 2165};
    seekInfo fileTestDirClosest{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 10000, 1011, 1769};
    seekInfo fileTestDirNext{INP_DIR_8, SEEK_MODE_NEXT_SYNC, 50000, 31, 70};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0320
 * @tc.name      : demuxer DAT, Read Seek with URI mpeg_h264_mp3_mpeg.dat(MPEGPS)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0320, TestSize.Level2)
{
    CreateUriSource(INP_URI_8);
    DemuxerResult(1253, 2165);
    seekInfo fileTestUriPrevious{INP_URI_8, SEEK_MODE_PREVIOUS_SYNC, 0, 1253, 2165};
    seekInfo fileTestUriClosest{INP_URI_8, SEEK_MODE_CLOSEST_SYNC, 10000, 1011, 1769};
    seekInfo fileTestUriNext{INP_URI_8, SEEK_MODE_NEXT_SYNC, 50000, 31, 70};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

#ifdef SUPPORT_CODEC_RM
/**
 * @tc.number    : DEMUXER_DAT_FUNC_0330
 * @tc.name      : demuxer DAT, GetTrackFormat, Local rv30_cook_rm.dat(rm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0330, TestSize.Level2)
{
    CreateFdSource(INP_DIR_9);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 240);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 320);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0340
 * @tc.name      : demuxer DAT, GetTrackFormat, URI rv30_cook_rm.dat(rm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0340, TestSize.Level2)
{
    CreateUriSource(INP_URI_9);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 240);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 320);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0350
 * @tc.name      : demuxer DAT, Read Seek with Local rv30_cook_rm.dat(rm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0350, TestSize.Level2)
{
    CreateFdSource(INP_DIR_9);
    DemuxerResult(1937, 2720);
    seekInfo fileTestDirPrevious{INP_DIR_9, SEEK_MODE_PREVIOUS_SYNC, 0, 1937, 2720};
    seekInfo fileTestDirClosest{INP_DIR_9, SEEK_MODE_CLOSEST_SYNC, 1000, 1908, 2720};
    seekInfo fileTestDirNext{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 5570, 1760, 2480};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0360
 * @tc.name      : demuxer DAT, Read Seek with URI rv30_cook_rm.dat(rm)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0360, TestSize.Level2)
{
    CreateUriSource(INP_URI_9);
    DemuxerResult(1937, 2720);
    seekInfo fileTestUriPrevious{INP_URI_9, SEEK_MODE_PREVIOUS_SYNC, 0, 1937, 2720};
    seekInfo fileTestUriClosest{INP_URI_9, SEEK_MODE_CLOSEST_SYNC, 1000, 1908, 2720};
    seekInfo fileTestUriNext{INP_URI_9, SEEK_MODE_NEXT_SYNC, 5570, 1760, 2480};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0370
 * @tc.name      : demuxer DAT, GetTrackFormat, Local rv40_cook_rmvb.dat(rmvb)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0370, TestSize.Level2)
{
    CreateFdSource(INP_DIR_10);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 288);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 352);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0380
 * @tc.name      : demuxer DAT, GetTrackFormat, URI rv40_cook_rmvb.dat(rmvb)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0380, TestSize.Level2)
{
    CreateUriSource(INP_URI_10);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 288);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 352);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0390
 * @tc.name      : demuxer DAT, Read Seek with Local rv40_cook_rmvb.dat(rmvb)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0390, TestSize.Level2)
{
    CreateFdSource(INP_DIR_10);
    DemuxerResult(251, 480);
    seekInfo fileTestDirPrevious{INP_DIR_10, SEEK_MODE_PREVIOUS_SYNC, 0, 251, 480};
    seekInfo fileTestDirClosest{INP_DIR_10, SEEK_MODE_CLOSEST_SYNC, 100, 251, 480};
    seekInfo fileTestDirNext{INP_DIR_10, SEEK_MODE_NEXT_SYNC, 557, 2, 80};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0400
 * @tc.name      : demuxer DAT, Read Seek with URI rv40_cook_rmvb.dat(rmvb)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0400, TestSize.Level2)
{
    CreateUriSource(INP_URI_10);
    DemuxerResult(251, 480);
    seekInfo fileTestUriPrevious{INP_URI_10, SEEK_MODE_PREVIOUS_SYNC, 0, 251, 480};
    seekInfo fileTestUriClosest{INP_URI_10, SEEK_MODE_CLOSEST_SYNC, 100, 251, 480};
    seekInfo fileTestUriNext{INP_URI_10, SEEK_MODE_NEXT_SYNC, 557, 2, 80};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}
#endif

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0410
 * @tc.name      : demuxer DAT, GetTrackFormat, Local m4v_h264_alac_m4v.dat(m4v)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0410, TestSize.Level2)
{
    CreateFdSource(INP_DIR_11);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0420
 * @tc.name      : demuxer DAT, GetTrackFormat, URI m4v_h264_alac_m4v.dat(m4v)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0420, TestSize.Level2)
{
    CreateUriSource(INP_URI_11);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0430
 * @tc.name      : demuxer DAT, Read Seek with Local m4v_h264_alac_m4v.dat(m4v)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0430, TestSize.Level2)
{
    CreateFdSource(INP_DIR_11);
    DemuxerResult(602, 109);
    seekInfo fileTestDirPrevious{INP_DIR_11, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 109};
    seekInfo fileTestDirClosest{INP_DIR_11, SEEK_MODE_CLOSEST_SYNC, 100, 602, 109};
    seekInfo fileTestDirNext{INP_DIR_11, SEEK_MODE_NEXT_SYNC, 557, 352, 64};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0440
 * @tc.name      : demuxer DAT, Read Seek with URI m4v_h264_alac_m4v.dat(m4v)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0440, TestSize.Level2)
{
    CreateUriSource(INP_URI_11);
    DemuxerResult(602, 109);
    seekInfo fileTestUriPrevious{INP_URI_11, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 109};
    seekInfo fileTestUriClosest{INP_URI_11, SEEK_MODE_CLOSEST_SYNC, 100, 602, 109};
    seekInfo fileTestUriNext{INP_URI_11, SEEK_MODE_NEXT_SYNC, 557, 352, 64};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0450
 * @tc.name      : demuxer DAT, GetTrackFormat, Local wmv_h264_wmav1_wmv.dat(wmv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0450, TestSize.Level2)
{
    CreateFdSource(INP_DIR_12);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0460
 * @tc.name      : demuxer DAT, GetTrackFormat, URI wmv_h264_wmav1_wmv.dat(wmv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0460, TestSize.Level2)
{
    CreateUriSource(INP_URI_12);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0470
 * @tc.name      : demuxer DAT, Read Seek with Local wmv_h264_wmav1_wmv.dat(wmv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0470, TestSize.Level2)
{
    CreateFdSource(INP_DIR_12);
    DemuxerResult(602, 218);
    seekInfo fileTestDirPrevious{INP_DIR_12, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 218};
    seekInfo fileTestDirClosest{INP_DIR_12, SEEK_MODE_CLOSEST_SYNC, 5000, 352, 127};
    seekInfo fileTestDirNext{INP_DIR_12, SEEK_MODE_NEXT_SYNC, 10000, 0, 36};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0480
 * @tc.name      : demuxer DAT, Read Seek with URI wmv_h264_wmav1_wmv.dat(wmv)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0480, TestSize.Level2)
{
    CreateUriSource(INP_URI_12);
    DemuxerResult(602, 218);
    seekInfo fileTestUriPrevious{INP_URI_12, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 218};
    seekInfo fileTestUriClosest{INP_URI_12, SEEK_MODE_CLOSEST_SYNC, 5000, 352, 127};
    seekInfo fileTestUriNext{INP_URI_12, SEEK_MODE_NEXT_SYNC, 10000, 0, 36};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0490
 * @tc.name      : demuxer DAT, GetTrackFormat, Local mpeg2_ac3_vob.dat(vob)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0490, TestSize.Level2)
{
    CreateFdSource(INP_DIR_13);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0500
 * @tc.name      : demuxer DAT, GetTrackFormat, URI mpeg2_ac3_vob.dat(vob)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0500, TestSize.Level2)
{
    CreateUriSource(INP_URI_13);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 480);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 720);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0510
 * @tc.name      : demuxer DAT, Read Seek with Local mpeg2_ac3_vob.dat(vob)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0510, TestSize.Level2)
{
    CreateFdSource(INP_DIR_13);
    DemuxerResult(602, 314);
    seekInfo fileTestDirPrevious{INP_DIR_13, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 313};
    seekInfo fileTestDirClosest{INP_DIR_13, SEEK_MODE_CLOSEST_SYNC, 5000, 302, 178};
    seekInfo fileTestDirNext{INP_DIR_13, SEEK_MODE_NEXT_SYNC, 10000, 2, 22};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0520
 * @tc.name      : demuxer DAT, Read Seek with URI mpeg2_ac3_vob.dat(vob)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0520, TestSize.Level2)
{
    CreateUriSource(INP_URI_13);
    DemuxerResult(602, 314);
    seekInfo fileTestUriPrevious{INP_URI_13, SEEK_MODE_PREVIOUS_SYNC, 0, 602, 313};
    seekInfo fileTestUriClosest{INP_URI_13, SEEK_MODE_CLOSEST_SYNC, 5000, 302, 178};
    seekInfo fileTestUriNext{INP_URI_13, SEEK_MODE_NEXT_SYNC, 10000, 2, 22};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0530
 * @tc.name      : demuxer DAT, GetTrackFormat, Local 3g2_h263_aac_3g2.dat(3g2)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0530, TestSize.Level2)
{
    CreateFdSource(INP_DIR_14);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 144);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 176);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 22050);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0540
 * @tc.name      : demuxer DAT, GetTrackFormat, URI 3g2_h263_aac_3g2.dat(3g2)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0540, TestSize.Level2)
{
    CreateUriSource(INP_URI_14);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 144);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 176);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 1);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 22050);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0550
 * @tc.name      : demuxer DAT, Read Seek with Local 3g2_h263_aac_3g2.dat(3g2)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0550, TestSize.Level2)
{
    CreateFdSource(INP_DIR_14);
    DemuxerResult(300, 217);
    seekInfo fileTestDirPrevious{INP_DIR_14, SEEK_MODE_PREVIOUS_SYNC, 0, 300, 216};
    seekInfo fileTestDirClosest{INP_DIR_14, SEEK_MODE_CLOSEST_SYNC, 5000, 156, 113};
    seekInfo fileTestDirNext{INP_DIR_14, SEEK_MODE_NEXT_SYNC, 9000, 24, 17};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0560
 * @tc.name      : demuxer DAT, Read Seek with URI 3g2_h263_aac_3g2.dat(3g2)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0560, TestSize.Level2)
{
    CreateUriSource(INP_URI_14);
    DemuxerResult(300, 217);
    seekInfo fileTestUriPrevious{INP_URI_14, SEEK_MODE_PREVIOUS_SYNC, 0, 300, 216};
    seekInfo fileTestUriClosest{INP_URI_14, SEEK_MODE_CLOSEST_SYNC, 5000, 156, 113};
    seekInfo fileTestUriNext{INP_URI_14, SEEK_MODE_NEXT_SYNC, 9000, 24, 17};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0570
 * @tc.name      : demuxer DAT, GetTrackFormat, Local 3gp_h264_aac_3gp.dat(3gp)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0570, TestSize.Level2)
{
    CreateFdSource(INP_DIR_15);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 400);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 960);
 
    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0580
 * @tc.name      : demuxer DAT, GetTrackFormat, URI 3gp_h264_aac_3gp.dat(3gp)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0580, TestSize.Level2)
{
    CreateUriSource(INP_URI_15);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    int32_t height = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 400);
    int32_t width = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 960);

    int32_t channelCount = 0;
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &channelCount));
    ASSERT_EQ(channelCount, 2);
    int32_t sampleRate = 0;
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sampleRate));
    ASSERT_EQ(sampleRate, 48000);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0590
 * @tc.name      : demuxer DAT, Read Seek with Local 3gp_h264_aac_3gp.dat(3gp)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0590, TestSize.Level2)
{
    CreateFdSource(INP_DIR_15);
    DemuxerResult(1116, 2186);
    seekInfo fileTestDirPrevious{INP_DIR_15, SEEK_MODE_PREVIOUS_SYNC, 0, 1116, 2186};
    seekInfo fileTestDirClosest{INP_DIR_15, SEEK_MODE_CLOSEST_SYNC, 10000, 889, 1746};
    seekInfo fileTestDirNext{INP_DIR_15, SEEK_MODE_NEXT_SYNC, 40000, 156, 312};
    CheckSeekMode(fileTestDirPrevious);
    CheckSeekMode(fileTestDirClosest);
    CheckSeekMode(fileTestDirNext);
}

/**
 * @tc.number    : DEMUXER_DAT_FUNC_0600
 * @tc.name      : demuxer DAT, Read Seek with URI 3gp_h264_aac_3gp.dat(3gp)
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerDAT2FuncNdkTest, DEMUXER_DAT_FUNC_0600, TestSize.Level2)
{
    CreateUriSource(INP_URI_15);
    DemuxerResult(1116, 2186);
    seekInfo fileTestUriPrevious{INP_URI_15, SEEK_MODE_PREVIOUS_SYNC, 0, 1116, 2186};
    seekInfo fileTestUriClosest{INP_URI_15, SEEK_MODE_CLOSEST_SYNC, 10000, 889, 1746};
    seekInfo fileTestUriNext{INP_URI_15, SEEK_MODE_NEXT_SYNC, 40000, 156, 312};
    CheckSeekMode(fileTestUriPrevious);
    CheckSeekMode(fileTestUriClosest);
    CheckSeekMode(fileTestUriNext);
}
