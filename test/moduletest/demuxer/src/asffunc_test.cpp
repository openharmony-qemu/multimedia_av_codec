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
class DemuxerAsfFuncNdkTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

protected:
    const char *INP_DIR_1 = "/data/test/media/test_msmpeg4_wma.asf";
    const char *INP_URI_1 = "http://127.0.0.1:46666/test_msmpeg4_wma.asf";
    const char *INP_DIR_2 = "/data/test/media/test_mpeg4_wma.asf";
    const char *INP_URI_2 = "http://127.0.0.1:46666/test_mpeg4_wma.asf";
    const char *INP_DIR_3 = "/data/test/media/h263_aac.asf";
    const char *INP_DIR_4 = "/data/test/media/h264_aac.asf";
    const char *INP_DIR_5 = "/data/test/media/h264_ac3.asf";
    const char *INP_DIR_6 = "/data/test/media/h264_adpcm_g722.asf";
    const char *INP_DIR_7 = "/data/test/media/h264_adpcm_g726.asf";
    const char *INP_DIR_8 = "/data/test/media/h264_adpcm_ima_wav.asf";
    const char *INP_DIR_9 = "/data/test/media/h264_adpcm_ms.asf";
    const char *INP_DIR_11 = "/data/test/media/h264_adpcm_yamaha.asf";
    const char *INP_DIR_12 = "/data/test/media/h264_amr_nb.asf";
    const char *INP_DIR_13 = "/data/test/media/h264_amr_wb.asf";
    const char *INP_DIR_14 = "/data/test/media/h264_av1_aac.asf";
    const char *INP_DIR_15 = "/data/test/media/h264_eac3.asf";
    const char *INP_DIR_16 = "/data/test/media/h264_flac.asf";
    const char *INP_DIR_17 = "/data/test/media/h264_gsm_ms.asf";
    const char *INP_DIR_18 = "/data/test/media/h264_mp2.asf";
    const char *INP_DIR_19 = "/data/test/media/h264_mp3.asf";
    const char *INP_DIR_20 = "/data/test/media/h264_pcm_alaw.asf";
    const char *INP_DIR_21 = "/data/test/media/h264_pcm_f32le.asf";
    const char *INP_DIR_22 = "/data/test/media/h264_pcm_f64le.asf";
    const char *INP_DIR_23 = "/data/test/media/h264_pcm_mulaw.asf";
    const char *INP_DIR_24 = "/data/test/media/h264_pcm_s16le.asf";
    const char *INP_DIR_25 = "/data/test/media/h264_pcm_s24le.asf";
    const char *INP_DIR_26 = "/data/test/media/h264_pcm_s32le.asf";
    const char *INP_DIR_27 = "/data/test/media/h264_pcm_s64le.asf";
    const char *INP_DIR_28 = "/data/test/media/h264_pcm_u8.asf";
    const char *INP_DIR_29 = "/data/test/media/h264_vorbis.asf";
    const char *INP_DIR_30 = "/data/test/media/h264_wmav1.asf";
    const char *INP_DIR_31 = "/data/test/media/h264_wmav2.asf";
    const char *INP_DIR_32 = "/data/test/media/mjpeg_aac.asf";
    const char *INP_DIR_33 = "/data/test/media/mpeg2_aac.asf";
    const char *INP_DIR_34 = "/data/test/media/mpeg4_aac.asf";
    const char *INP_DIR_35 = "/data/test/media/msvideo1_aac.asf";
    const char *INP_DIR_36 = "/data/test/media/vp9_aac.asf";
    const char *INP_DIR_37 = "/data/test/media/test_error.asf";
    const char *INP_DIR_38 = "/data/test/media/av1_mp3.asf";
    const char *INP_DIR_39 = "/data/test/media/DVCNTSC_720x480_25_422_dv5n.asf";
    const char *INP_DIR_40 = "/data/test/media/DVCPAL_720x576_25_411_dvpp.asf";
    const char *INP_DIR_41 = "/data/test/media/DVCPROHD_1280x1080_29_422_dvh6.asf";
    const char *INP_DIR_42 = "/data/test/media/h264_adpcm_ima_wav.asf";
    const char *INP_DIR_43 = "/data/test/media/vc1.asf";
    const char *INP_DIR_44 = "/data/test/media/vp8_aac.asf";
    const char *INP_DIR_45 = "/data/test/media/wmv3_wmapro.asf";
    const char *INP_DIR_46 = "/data/test/media/mpeg1_dts.asf";
    const char *INP_DIR_47 = "/data/test/media/rawvideo_dts.asf";
    const char *INP_DIR_48 = "/data/test/media/cinepak_dts.asf";
};

static unique_ptr<FileServerDemo> server = nullptr;
static int g_fd = -1;
static OH_AVMemory *memory = nullptr;
static OH_AVSource *source = nullptr;
static OH_AVErrCode ret = AV_ERR_OK;
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

void DemuxerAsfFuncNdkTest::SetUpTestCase()
{
    server = make_unique<FileServerDemo>();
    server->StartServer();
}

void DemuxerAsfFuncNdkTest::TearDownTestCase()
{
    server->StopServer();
}

void DemuxerAsfFuncNdkTest::SetUp()
{
    memory = OH_AVMemory_Create(g_width * g_height);
    g_trackCount = 0;
}

void DemuxerAsfFuncNdkTest::TearDown()
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
        int32_t seekRet = OH_AVDemuxer_SeekToTime(demuxer, seekInfo.millisecond, seekInfo.seekmode);
        if (seekRet != AV_ERR_OK && seekRet != AV_ERR_OPERATE_NOT_PERMIT) {
            ASSERT_EQ(AV_ERR_OK, seekRet);
        }
        
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
    close(g_fd);
    g_fd = -1;
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

static void DemuxerResult()
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
    ASSERT_EQ(g_trackCount, TWO);

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
    close(g_fd);
    g_fd = -1;
}

static void DemuxerAsfResult(const char *file, int aFrames, int vFrames)
{
    int trackType = 0;
    OH_AVCodecBufferAttr attr;
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
    for (int32_t index = 0; index < g_trackCount; index++) {
        ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, index));
    }
    for (int32_t index = 0; index < g_trackCount; index++) {
        trackFormat = OH_AVSource_GetTrackFormat(source, index);
        ASSERT_NE(trackFormat, nullptr);
        ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_TRACK_TYPE, &trackType));
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
        if (trackType == MEDIA_TYPE_VID) {
            cout << "vFrames---" << frameNum << endl;
            ASSERT_EQ(vFrames, frameNum);
        } else if (trackType == MEDIA_TYPE_AUD) {
            cout << "aFrames---" << frameNum << endl;
            ASSERT_EQ(aFrames, frameNum);
        }
    }
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_0100
 * @tc.name      : demuxer ASF, GetVideoTrackFormat,MD_KEY_HEIGHT, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_0100, TestSize.Level0)
{
    int32_t height = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 360);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_0100
 * @tc.name      : demuxer ASF, GetVideoTrackFormat,MD_KEY_HEIGHT, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_0100, TestSize.Level0)
{
    int32_t height = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_HEIGHT, &height));
    ASSERT_EQ(height, 360);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_0200
 * @tc.name      : demuxer ASF, GetVideoTrackFormat,MD_KEY_WIDTH, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_0200, TestSize.Level0)
{
    int32_t width = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 640);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_0200
 * @tc.name      : demuxer ASF, GetVideoTrackFormat,MD_KEY_WIDTH, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_0200, TestSize.Level0)
{
    int32_t width = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 0);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_WIDTH, &width));
    ASSERT_EQ(width, 640);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_0300
 * @tc.name      : demuxer ASF, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_0300, TestSize.Level0)
{
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_0300
 * @tc.name      : demuxer ASF, GetSourceFormat,OH_MD_KEY_TRACK_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_0300, TestSize.Level0)
{
    CreateUriSource(INP_URI_1);
    sourceFormat = OH_AVSource_GetSourceFormat(source);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(sourceFormat, OH_MD_KEY_TRACK_COUNT, &g_trackCount));
    ASSERT_EQ(g_trackCount, TWO);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_0400
 * @tc.name      : demuxer ASF, GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_0400, TestSize.Level0)
{
    int32_t cc = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_0400
 * @tc.name      : demuxer ASF, GetAudioTrackFormat,MD_KEY_CHANNEL_COUNT, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_0400, TestSize.Level0)
{
    int32_t cc = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, &cc));
    ASSERT_EQ(cc, 1);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_1200
 * @tc.name      : demuxer ASF, GetAudioTrackFormat,MD_KEY_SAMPLE_RATE, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_1200, TestSize.Level0)
{
    int32_t sr = 0;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_1200
 * @tc.name      : demuxer ASF, GetAudioTrackFormat,MD_KEY_SAMPLE_RATE, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_1200, TestSize.Level0)
{
    int32_t sr = 0;
    CreateUriSource(INP_URI_1);
    ASSERT_NE(source, nullptr);
    trackFormat = OH_AVSource_GetTrackFormat(source, 1);
    ASSERT_NE(trackFormat, nullptr);
    ASSERT_TRUE(OH_AVFormat_GetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, &sr));
    ASSERT_EQ(sr, 44100);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_1600
 * @tc.name      : demuxer ASF, Create source with g_fd, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_1600, TestSize.Level3)
{
    CreateFdSource(INP_DIR_1);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_1600
 * @tc.name      : demuxer ASF, Create source with URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_1600, TestSize.Level3)
{
    CreateUriSource(INP_URI_1);
    DemuxerResult();
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_1700
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h263_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_1700, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_3, 132, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_1800
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_1800, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_4, 131, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_1900
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_ac3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_1900, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_5, 87, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2000
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_adpcm_g722.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2000, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_6, 416, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2100
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_adpcm_g726.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2100, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_7, 12, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2200
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_adpcm_ima_wav.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2200, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_8, 131, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2300
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_adpcm_ms.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2300, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_9, 132, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2500
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_adpcm_yamaha.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2500, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_11, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2600
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_amr_nb.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2600, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_12, 152, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2700
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_amr_wb.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2700, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_13, 151, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_2800
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_av1_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2800, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_14, 130, 179);
}

#ifdef SUPPORT_DEMUXER_EAC3
/**
 * @tc.number    : DEMUXER_ASF_FUNC_2900
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_eac3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_2900, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_15, 87, 184);
}
#endif

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3000
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_flac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3000, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_16, 29, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3100
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_gsm_ms.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3100, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_17, 76, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3300
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_mp3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3300, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_19, 117, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3400
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_alaw.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3400, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_20, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5700
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_f32le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5700, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_20, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3600
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_f64le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3600, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_21, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3700
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_mulaw.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3700, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_22, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3900
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_s24le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3900, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_24, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4000
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_s32le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4000, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_25, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4100
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_s64le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4100, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_26, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4200
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_u8.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4200, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_27, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4300
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_vorbis.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4300, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_28, 130, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4500
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_wmav2.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4500, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_30, 65, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4700
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, mpeg2_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4700, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_32, 132, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4800
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, mpeg4_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4800, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_33, 132, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4900
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, msvideo1_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4900, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_34, 132, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5000
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, vp9_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5000, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_35, 132, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5100
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, vp9_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5100, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_36, 132, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5200
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_mp2.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5200, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_18, 116, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5300
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 3000, 216, 155};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5400
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_pcm_s16le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5400, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_23, 131, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5500
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_wmav1.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5500, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_29, 131, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5600
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, mjpeg_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5600, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_31, 65, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3200
 * @tc.name      : demuxer ASF, Seek to start time, previous mode, Local
 * @tc.desc      : 本地ASF文件跳转到起始时间（0ms），PREVIOUS_SYNC模式
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 0, 300, 216};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_3200
 * @tc.name      : demuxer ASF, Seek to start time, previous mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_3200, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 0, 300, 216};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3500
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_1, SEEK_MODE_PREVIOUS_SYNC, 3000, 216, 155};
    CreateFdSource(INP_DIR_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_URI_3500
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, URI
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_URI_3500, TestSize.Level1)
{
    seekInfo fileTest1{INP_URI_1, SEEK_MODE_PREVIOUS_SYNC, 3000, 216, 155};
    CreateUriSource(INP_URI_1);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_3800
 * @tc.name      : demuxer ASF, Seek to invalid time, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_3800, TestSize.Level0)
{
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_SelectTrackByID(demuxer, 0));
    ASSERT_NE(demuxer, nullptr);
    int64_t invalidPts = 20000;
    ret = OH_AVDemuxer_SeekToTime(demuxer, invalidPts, SEEK_MODE_CLOSEST_SYNC);
    ASSERT_NE(ret, AV_ERR_OK);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_4400
 * @tc.name      : demuxer ASF, Unselect track before select, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4400, TestSize.Level0)
{
    CreateFdSource(INP_DIR_1);
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
 * @tc.number    : DEMUXER_ASF_FUNC_4600
 * @tc.name      : demuxer ASF, Read sample before select track, Local
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_4600, TestSize.Level2)
{
    uint32_t trackIndex = 0;
    OH_AVCodecBufferAttr attr;
    CreateFdSource(INP_DIR_1);
    ASSERT_NE(source, nullptr);
    demuxer = OH_AVDemuxer_CreateWithSource(source);
    ASSERT_NE(demuxer, nullptr);
    ret = OH_AVDemuxer_ReadSample(demuxer, trackIndex, memory, &attr);
    ASSERT_EQ(ret, AV_ERR_OPERATE_NOT_PERMIT);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_5900
 * @tc.name      : demuxer damaged rmvb audio file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_5900, TestSize.Level2)
{
    const char *file = INP_DIR_37;
    g_fd = open(file, O_RDONLY);
    int64_t size = GetFileSize(file);
    cout << file << "----------------------" << g_fd << "---------" << size << endl;
    source = OH_AVSource_CreateWithFD(g_fd, 0, size);
    ASSERT_EQ(source, nullptr);
    close(g_fd);
    g_fd = -1;
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_6000
 * @tc.name      : seek to a invalid time, closest mode
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6000, TestSize.Level2)
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
 * @tc.number    : DEMUXER_ASF_FUNC_6100
 * @tc.name      : remove track before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6100, TestSize.Level2)
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
 * @tc.number    : DEMUXER_ASF_FUNC_6200
 * @tc.name      : remove all tracks before demux finish
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6200, TestSize.Level2)
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
    ASSERT_EQ(g_trackCount, TWO);
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
                ASSERT_EQ(AV_ERR_OK, OH_AVDemuxer_UnselectTrackByID(demuxer, 1));
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
 * @tc.number    : DEMUXER_ASF_FUNC_6300
 * @tc.name      : start demux before add track
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6300, TestSize.Level2)
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
 * @tc.number    : DEMUXER_ASF_FUNC_6400
 * @tc.name      : input invalid rmvb uri file
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6400, TestSize.Level2)
{
    const char *uri = "http://192.168.3.11:8080/share/invalid.asf";
    source = OH_AVSource_CreateWithURI(const_cast<char *>(uri));
    ASSERT_EQ(source, nullptr);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_6500
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h263_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_3, SEEK_MODE_NEXT_SYNC, 3000, 4, 2};
    CreateFdSource(INP_DIR_3);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_6600
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_4, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 131};
    CreateFdSource(INP_DIR_4);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_6700
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_ac3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_5, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 87};
    CreateFdSource(INP_DIR_5);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_6800
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_adpcm_g722.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_6, SEEK_MODE_NEXT_SYNC, 3000, 0, 416};
    CreateFdSource(INP_DIR_6);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_6900
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_adpcm_g726.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_6900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_7, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 12};
    CreateFdSource(INP_DIR_7);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7000
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_adpcm_ima_wav.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_8, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 131};
    CreateFdSource(INP_DIR_8);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7100
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_adpcm_ms.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_9, SEEK_MODE_NEXT_SYNC, 3000, 0, 132};
    CreateFdSource(INP_DIR_9);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7300
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_adpcm_yamaha.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_11, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 130};
    CreateFdSource(INP_DIR_11);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7400
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_amr_nb.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_12, SEEK_MODE_NEXT_SYNC, 3000, 0, 152};
    CreateFdSource(INP_DIR_12);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7500
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_amr_wb.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_13, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 151};
    CreateFdSource(INP_DIR_13);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7600
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_av1_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_14, SEEK_MODE_CLOSEST_SYNC, 3000, 179, 130};
    CreateFdSource(INP_DIR_14);
    CheckSeekMode(fileTest1);
}

#ifdef SUPPORT_DEMUXER_EAC3
/**
 * @tc.number    : DEMUXER_ASF_FUNC_7700
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_eac3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_15, SEEK_MODE_NEXT_SYNC, 3000, 0, 87};
    CreateFdSource(INP_DIR_15);
    CheckSeekMode(fileTest1);
}
#endif

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7800
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_flac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_16, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 29};
    CreateFdSource(INP_DIR_16);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_7900
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_gsm_ms.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_7900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_17, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 76};
    CreateFdSource(INP_DIR_17);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8000
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_mp2.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_18, SEEK_MODE_NEXT_SYNC, 3000, 0, 116};
    CreateFdSource(INP_DIR_18);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8100
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_mp3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_19, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 117};
    CreateFdSource(INP_DIR_19);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8200
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_pcm_alaw.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_20, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 130};
    CreateFdSource(INP_DIR_20);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8300
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_pcm_f32le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_21, SEEK_MODE_NEXT_SYNC, 3000, 0, 130};
    CreateFdSource(INP_DIR_21);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8400
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_pcm_f64le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_22, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 130};
    CreateFdSource(INP_DIR_22);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8500
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_pcm_mulaw.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_23, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 131};
    CreateFdSource(INP_DIR_23);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8600
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_pcm_s16le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_24, SEEK_MODE_NEXT_SYNC, 3000, 0, 130};
    CreateFdSource(INP_DIR_24);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8700
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_pcm_s24le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_25, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 130};
    CreateFdSource(INP_DIR_25);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8800
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_pcm_s32le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_26, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 130};
    CreateFdSource(INP_DIR_26);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_8900
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_pcm_s64le.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_8900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_27, SEEK_MODE_NEXT_SYNC, 3000, 0, 130};
    CreateFdSource(INP_DIR_27);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9000
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_pcm_u8.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_28, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 130};
    CreateFdSource(INP_DIR_28);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9100
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_vorbis.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_29, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 131};
    CreateFdSource(INP_DIR_29);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9200
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, h264_wmav1.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_30, SEEK_MODE_NEXT_SYNC, 3000, 0, 64};
    CreateFdSource(INP_DIR_30);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9300
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, h264_wmav2.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_31, SEEK_MODE_PREVIOUS_SYNC, 3000, 184, 65};
    CreateFdSource(INP_DIR_31);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9400
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, mjpeg_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_32, SEEK_MODE_CLOSEST_SYNC, 3000, 12, 8};
    CreateFdSource(INP_DIR_32);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9500
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, mpeg2_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9500, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_33, SEEK_MODE_NEXT_SYNC, 3000, 4, 2};
    CreateFdSource(INP_DIR_33);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9600
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, mpeg4_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_34, SEEK_MODE_PREVIOUS_SYNC, 3000, 16, 11};
    CreateFdSource(INP_DIR_34);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9700
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, msvideo1_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_35, SEEK_MODE_CLOSEST_SYNC, 3000, 28, 19};
    CreateFdSource(INP_DIR_35);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9800
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, vp9_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_36, SEEK_MODE_NEXT_SYNC, 3000, 56, 39};
    CreateFdSource(INP_DIR_36);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_9900
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, av1_mp3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_9900, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_38, 117, 185);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10000
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, DVCNTSC_720x480_25_422_dv5n.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10000, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_39, 132, 25);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10100
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, DVCPAL_720x576_25_411_dvpp.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10100, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_40, 132, 27);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10200
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, DVCPROHD_1280x1080_29_422_dvh6.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10200, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_41, 132, 30);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10300
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, h264_adpcm_ima_wav.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10300, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_42, 131, 184);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10400
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, vc1.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10400, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_43, 95, 60);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10500
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, vp8_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10500, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_44, 132, 95);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10600
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, wmv3_wmapro.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10600, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_45, 32, 120);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10700
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, av1_mp3.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10700, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_38, SEEK_MODE_PREVIOUS_SYNC, 3000, 57, 36};
    CreateFdSource(INP_DIR_38);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_10800
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, DVCNTSC_720x480_25_422_dv5n.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_39, SEEK_MODE_CLOSEST_SYNC, 3000, 25, 0};
    CreateFdSource(INP_DIR_39);
    CheckSeekMode(fileTest1);
}
/**
 * @tc.number    : DEMUXER_ASF_FUNC_10900
 * @tc.name      : demuxer ASF, Seek to middle time, next mode, DVCPAL_720x576_25_411_dvpp.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_10900, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_40, SEEK_MODE_NEXT_SYNC, 3000, 27, 0};
    CreateFdSource(INP_DIR_40);
    CheckSeekMode(fileTest1);
}
/**
 * @tc.number    : DEMUXER_ASF_FUNC_11000
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, DVCPROHD_1280x1080_29_422_dvh6.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_41, SEEK_MODE_PREVIOUS_SYNC, 3000, 30, 0};
    CreateFdSource(INP_DIR_41);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11100
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, h264_adpcm_ima_wav.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11100, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_42, SEEK_MODE_CLOSEST_SYNC, 3000, 184, 131};
    CreateFdSource(INP_DIR_42);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11200
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, vc1.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_43, SEEK_MODE_CLOSEST_SYNC, 3000, 60, 0};
    CreateFdSource(INP_DIR_43);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11300
 * @tc.name      : demuxer ASF, Seek to middle time, previous mode, vp8_aac.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11300, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_44, SEEK_MODE_PREVIOUS_SYNC, 3000, 95, 132};
    CreateFdSource(INP_DIR_44);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11400
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, wmv3_wmapro.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11400, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_45, SEEK_MODE_CLOSEST_SYNC, 3000, 106, 8};
    CreateFdSource(INP_DIR_45);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11500
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, test_mpeg4_wma.asf.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11500, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_2, 216, 300);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11600
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, test_mpeg4_wma.asf.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11600, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_2, SEEK_MODE_CLOSEST_SYNC, 9000, 36, 26};
    CreateFdSource(INP_DIR_2);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11700
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, mpeg1_dts.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11700, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_46, 262, 185);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11800
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, mpeg1_dts.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11800, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_46, SEEK_MODE_CLOSEST_SYNC, 2000, 77, 109};
    CreateFdSource(INP_DIR_46);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_11900
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, rawvideo_dts.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_11900, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_47, 130, 90);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_12000
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, rawvideo_dts.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_12000, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_47, SEEK_MODE_CLOSEST_SYNC, 1000, 36, 53};
    CreateFdSource(INP_DIR_47);
    CheckSeekMode(fileTest1);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_12100
 * @tc.name      : demuxer ASF, Create source with g_fd, Local, cinepak_dts.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_12100, TestSize.Level3)
{
    DemuxerAsfResult(INP_DIR_48, 262, 185);
}

/**
 * @tc.number    : DEMUXER_ASF_FUNC_12200
 * @tc.name      : demuxer ASF, Seek to middle time, closest mode, cinepak_dts.asf
 * @tc.desc      : function test
 */
HWTEST_F(DemuxerAsfFuncNdkTest, DEMUXER_ASF_FUNC_12200, TestSize.Level1)
{
    seekInfo fileTest1{INP_DIR_48, SEEK_MODE_CLOSEST_SYNC, 2000, 77, 107};
    CreateFdSource(INP_DIR_48);
    CheckSeekMode(fileTest1);
}