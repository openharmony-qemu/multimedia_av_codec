/*
 * Copyright (C) 2022 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <iostream>
#include <thread>
#include <vector>
#include <sys/stat.h>
#include "gtest/gtest.h"
#include "AVMuxerDemo.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;

namespace {
class NativeAVMuxerFunctionTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void NativeAVMuxerFunctionTest::SetUpTestCase() {}
void NativeAVMuxerFunctionTest::TearDownTestCase() {}
void NativeAVMuxerFunctionTest::SetUp() {}
void NativeAVMuxerFunctionTest::TearDown() {}

static int g_inputFile = -1;
static const int DATA_AUDIO_ID = 0;
static const int DATA_VIDEO_ID = 1;

constexpr int32_t BIG_EXTRA_SIZE = 100;
constexpr int32_t SMALL_EXTRA_SIZE = 0;

constexpr int32_t CHANNEL_COUNT = 2;
constexpr int32_t SAMPLE_RATE = 44100;
constexpr int64_t AUDIO_BITRATE = 320000;

constexpr int32_t WIDTH = 352;
constexpr int32_t HEIGHT = 288;
constexpr int64_t VIDEO_BITRATE = 524569;

constexpr int32_t WIDTH_640 = 640;
constexpr int32_t WIDTH_720 = 720;
constexpr int32_t HEIGHT_360 = 360;
constexpr int32_t HEIGHT_480 = 480;

int32_t testResult[10] = { -1 };

int32_t AddAudioTrack(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
{
    OH_AVFormat* audioFormat = OH_AVFormat_Create();
    if (audioFormat == NULL) {
        printf("audio format failed!");
        return -1;
    }
    int extraSize = 0;
    
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
        unsigned char buffer[100] = { 0 };
        read(g_inputFile, buffer, extraSize);
        OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }

    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetLongValue(audioFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);

    int32_t trackId;
    muxerDemo->NativeAddTrack(handle, &trackId, audioFormat);
    OH_AVFormat_Destroy(audioFormat);
    return trackId;
}


int32_t AddVideoTrack(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
{
    OH_AVFormat* videoFormat = OH_AVFormat_Create();
    if (videoFormat == NULL) {
        printf("video format failed!");
        return -1;
    }
    int extraSize = 0;
    
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
        unsigned char buffer[100] = { 0 };
        read(g_inputFile, buffer, extraSize);
        OH_AVFormat_SetBuffer(videoFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }
    OH_AVFormat_SetStringValue(videoFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_MPEG4);
    OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_WIDTH, WIDTH);
    OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_HEIGHT, HEIGHT);
    OH_AVFormat_SetLongValue(videoFormat, OH_MD_KEY_BITRATE, VIDEO_BITRATE);

    int32_t trackId;
    muxerDemo->NativeAddTrack(handle, &trackId, videoFormat);
    OH_AVFormat_Destroy(videoFormat);
    return trackId;
}


int32_t AddCoverTrack(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, const string coverType)
{
    OH_AVFormat* coverFormat = OH_AVFormat_Create();
    if (coverFormat == NULL) {
        printf("cover format failed!");
        return -1;
    }

    if (coverType == "jpg") {
        OH_AVFormat_SetStringValue(coverFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_IMAGE_JPG);
    } else if (coverType == "png") {
        OH_AVFormat_SetStringValue(coverFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_IMAGE_PNG);
    } else {
        OH_AVFormat_SetStringValue(coverFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_IMAGE_BMP);
    }

    OH_AVFormat_SetIntValue(coverFormat, OH_MD_KEY_WIDTH, WIDTH);
    OH_AVFormat_SetIntValue(coverFormat, OH_MD_KEY_HEIGHT, HEIGHT);

    int32_t trackId;
    muxerDemo->NativeAddTrack(handle, &trackId, coverFormat);
    OH_AVFormat_Destroy(coverFormat);
    return trackId;
}

bool ReadFile(int& dataTrackId, int64_t& pts, int& dataSize)
{
    int ret = 0;
    ret = read(g_inputFile, static_cast<void*>(&dataTrackId), sizeof(dataTrackId));
    if (ret <= 0) {
        cout << "read dataTrackId error, ret is: " << ret << endl;
        return false;
    }
    ret = read(g_inputFile, static_cast<void*>(&pts), sizeof(pts));
    if (ret <= 0) {
        cout << "read info.pts error, ret is: " << ret << endl;
        return false;
    }
    ret = read(g_inputFile, static_cast<void*>(&dataSize), sizeof(dataSize));
    if (ret <= 0) {
        cout << "read dataSize error, ret is: " << ret << endl;
        return false;
    }
    return true;
}

void WriteTrackSample(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int audioTrackIndex, int videoTrackIndex)
{
    OH_AVCodecBufferAttr info {0, 0, 0, 0};
    OH_AVMemory* avMemBuffer = nullptr;
    uint8_t* data = nullptr;
    bool readRet;
    do {
        int ret = 0;
        int dataTrackId = 0;
        int dataSize = 0;
        int trackId = 0;
        readRet = ReadFile(dataTrackId, info.pts, dataSize);
        if (!readRet) {
            return;
        }
        avMemBuffer = OH_AVMemory_Create(dataSize);
        data = OH_AVMemory_GetAddr(avMemBuffer);
        ret = read(g_inputFile, static_cast<void*>(data), dataSize);
        if (ret <= 0) {
            cout << "read data error, ret is: " << ret << endl;
            return;
        }

        info.size = dataSize;
        if (dataTrackId == DATA_AUDIO_ID) {
            trackId = audioTrackIndex;
        } else if (dataTrackId == DATA_VIDEO_ID) {
            trackId = videoTrackIndex;
        } else {
            cout << "error dataTrackId : " << trackId << endl;
        }
        if (trackId >= 0) {
            OH_AVErrCode result = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
            if (result != AV_ERR_OK) {
                cout << "OH_AVMuxer_WriteSampleBuffer error! ret is: " << result << endl;
                return;
            }
        }
        if (avMemBuffer != nullptr) {
            OH_AVMemory_Destroy(avMemBuffer);
            avMemBuffer = nullptr;
        }
    } while (readRet)
}

void WriteTrackSampleShort(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int audioTrackIndex,
    int videoTrackIndex, int audioWriteTime)
{
    OH_AVCodecBufferAttr info {0, 0, 0, 0};
    OH_AVMemory* avMemBuffer = nullptr;
    uint8_t* data = nullptr;
    bool readRet;
    do {
        int dataTrackId = 0;
        int dataSize = 0;
        int ret = 0;
        int trackId = 0;
        int curTime = 0;
        readRet = ReadFile(dataTrackId, info.pts, dataSize);
        if (!readRet) {
            return;
        }

        avMemBuffer = OH_AVMemory_Create(dataSize);
        data = OH_AVMemory_GetAddr(avMemBuffer);
        ret = read(g_inputFile, static_cast<void*>(data), dataSize);
        if (ret <= 0) { return; }

        info.size = dataSize;
        if (dataTrackId == DATA_AUDIO_ID) {
            trackId = audioTrackIndex;
        } else if (dataTrackId == DATA_VIDEO_ID) {
            trackId = videoTrackIndex;
        } else {
            printf("error dataTrackId : %d", trackId);
        }
        if (trackId >= 0) {
            if (trackId == audioTrackIndex && curTime > audioWriteTime) {
                continue;
            } else if (trackId == audioTrackIndex) {
                curTime++;
            }
            OH_AVErrCode result = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
            if (result != AV_ERR_OK) {
                printf("OH_AVMuxer_WriteSampleBuffer error!");
                return;
            }
        }
        if (avMemBuffer != nullptr) {
            OH_AVMemory_Destroy(avMemBuffer);
            avMemBuffer = nullptr;
        }
    }while (readRet)
}

int32_t AddAudioTrackByFd(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int32_t inputFile)
{
    OH_AVFormat* audioFormat = OH_AVFormat_Create();
    if (audioFormat == NULL) {
        printf("audio format failed!");
        return -1;
    }
    int extraSize = 0;

    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
        unsigned char buffer[100] = { 0 };
        read(inputFile, buffer, extraSize);
        OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }

    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetLongValue(audioFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);

    int32_t trackId;
    muxerDemo->NativeAddTrack(handle, &trackId, audioFormat);
    OH_AVFormat_Destroy(audioFormat);
    return trackId;
}

int32_t AddAudioTrackAACByFd(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int32_t inputFile)
{
    OH_AVFormat* audioFormat = OH_AVFormat_Create();
    if (audioFormat == NULL) {
        printf("audio format failed!");
        return -1;
    }
    int extraSize = 0;

    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
        unsigned char buffer[100] = { 0 };
        read(inputFile, buffer, extraSize);
        OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }
    OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
    OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
    OH_AVFormat_SetLongValue(audioFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);

    int32_t trackId;
    muxerDemo->NativeAddTrack(handle, &trackId, audioFormat);
    OH_AVFormat_Destroy(audioFormat);
    return trackId;
}

int32_t AddVideoTrackByFd(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int32_t inputFile)
{
    OH_AVFormat* videoFormat = OH_AVFormat_Create();
    if (videoFormat == NULL) {
        printf("video format failed!");
        return -1;
    }
    int extraSize = 0;

    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
        unsigned char buffer[100] = { 0 };
        read(inputFile, buffer, extraSize);
        OH_AVFormat_SetBuffer(videoFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }

    OH_AVFormat_SetStringValue(videoFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_MPEG4);
    OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_WIDTH, WIDTH_720);
    OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_HEIGHT, HEIGHT_480);
    OH_AVFormat_SetLongValue(videoFormat, OH_MD_KEY_BITRATE, VIDEO_BITRATE);

    int32_t trackId;
    muxerDemo->NativeAddTrack(handle, &trackId, videoFormat);
    OH_AVFormat_Destroy(videoFormat);
    return trackId;
}

bool ReadFileByFd(int& dataTrackId, int64_t& pts, int& dataSize, int32_t inputFile)
{
    int ret = 0;
    ret = read(inputFile, static_cast<void*>(&dataTrackId), sizeof(dataTrackId));
    if (ret <= 0) {
        cout << "read dataTrackId error, ret is: " << ret << endl;
        return false;
    }
    ret = read(inputFile, static_cast<void*>(&pts), sizeof(pts));
    if (ret <= 0) {
        cout << "read info.pts error, ret is: " << ret << endl;
        return false;
    }
    ret = read(inputFile, static_cast<void*>(&dataSize), sizeof(dataSize));
    if (ret <= 0) {
        cout << "read dataSize error, ret is: " << ret << endl;
        return false;
    }
    return true;
}

void WriteTrackSampleByFd(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int audioTrackIndex,
    int videoTrackIndex, int32_t inputFile)
{
    OH_AVCodecBufferAttr info {0, 0, 0, 0};
    OH_AVMemory* avMemBuffer = nullptr;
    uint8_t* data = nullptr;
    string resultStr = "";
    bool readRet;
    do {
        int dataTrackId = 0;
        int dataSize = 0;
        int ret = 0;
        int trackId = 0;
        readRet = ReadFileByFd(dataTrackId, info.pts, dataSize, inputFile);
        if (!readRet) {
            return;
        }

        avMemBuffer = OH_AVMemory_Create(dataSize);
        data = OH_AVMemory_GetAddr(avMemBuffer);
        cout << resultStr << endl;

        ret = read(inputFile, static_cast<void*>(data), dataSize);
        if (ret <= 0) {
            cout << "read data error, ret is: " << ret << endl;
            continue;
        }

        info.size = dataSize;
        if (dataTrackId == DATA_AUDIO_ID) {
            trackId = audioTrackIndex;
        } else if (dataTrackId == DATA_VIDEO_ID) {
            trackId = videoTrackIndex;
        } else {
            cout << "error dataTrackId : " << dataTrackId << endl;
        }
        if (trackId >= 0) {
            OH_AVErrCode result = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
            if (result != AV_ERR_OK) {
                cout << "OH_AVMuxer_WriteSampleBuffer error! ret is: " << result << endl;
                break;
            }
        }

        if (avMemBuffer != nullptr) {
            OH_AVMemory_Destroy(avMemBuffer);
            avMemBuffer = nullptr;
        }
    } while (readRet)
}

void RunMuxer(const string testcaseName, int threadId, OH_AVOutputFormat format)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    string fileName = testcaseName + "_" + to_string(threadId);

    cout << "thread id is: " << threadId << ", cur file name is: " << fileName << endl;
    int32_t fd = muxerDemo->GetFdByName(format, fileName);

    int32_t inputFile;
    int32_t audioTrackId;
    int32_t videoTrackId;

    cout << "thread id is: " << threadId << ", fd is: " << fd << endl;
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);

    cout << "thread id is: " << threadId << ", handle is: " << handle << endl;
    OH_AVErrCode ret;

    if (format == AV_OUTPUT_FORMAT_MPEG_4) {
        cout << "thread id is: " << threadId << ", format is: " << format << endl;
        inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);
        audioTrackId = AddAudioTrackByFd(muxerDemo, handle, inputFile);
        videoTrackId = AddVideoTrackByFd(muxerDemo, handle, inputFile);
    } else {
        cout << "thread id is: " << threadId << ", format is: " << format << endl;
        inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);
        audioTrackId = AddAudioTrackAACByFd(muxerDemo, handle, inputFile);
        videoTrackId = AddVideoTrackByFd(muxerDemo, handle, inputFile);
    }

    cout << "thread id is: " << threadId  << ", audio track id is: " << audioTrackId <<
        ", video track id is: " << videoTrackId << endl;

    ret = muxerDemo->NativeStart(handle);
    cout << "thread id is: " << threadId << ", Start ret is:" << ret << endl;

    WriteTrackSampleByFd(muxerDemo, handle, audioTrackId, videoTrackId, inputFile);

    ret = muxerDemo->NativeStop(handle);
    cout << "thread id is: " << threadId << ", Stop ret is:" << ret << endl;

    ret = muxerDemo->NativeDestroy(handle);
    cout << "thread id is: " << threadId << ", Destroy ret is:" << ret << endl;

    testResult[threadId] = AV_ERR_OK;
    close(inputFile);
    close(fd);
    delete muxerDemo;
}

void FreeBuffer(OH_AVMemory* avMemBuffer)
{
    if (avMemBuffer != nullptr) {
        OH_AVMemory_Destroy(avMemBuffer);
        avMemBuffer = nullptr;
    }
}

void WriteSingleTrackSample(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int trackId, int fd)
{
    OH_AVMemory* avMemBuffer = nullptr;
    uint8_t* data = nullptr;
    OH_AVCodecBufferAttr info;
    memset_s(&info, sizeof(info), 0, sizeof(info));
    int ret = 0;
    do {
        int dataSize = 0;
        int flags = 0;
        ret = read(fd, static_cast<void*>(&info.pts), sizeof(info.pts));
        if (ret <= 0) {
            break;
        }

        ret = read(fd, static_cast<void*>(&flags), sizeof(flags));
        if (ret <= 0) {
            break;
        }

        // read frame buffer
        ret = read(fd, static_cast<void*>(&dataSize), sizeof(dataSize));
        if (ret <= 0 || dataSize < 0) {
            break;
        }

        avMemBuffer = OH_AVMemory_Create(dataSize);
        data = OH_AVMemory_GetAddr(avMemBuffer);
        ret = read(fd, static_cast<void*>(data), dataSize);
        if (ret <= 0) {
            break;
        }
        info.size = dataSize;

        info.flags = 0;
        if (flags != 0) {
            info.flags |= AVCODEC_BUFFER_FLAGS_SYNC_FRAME;
        }

        OH_AVErrCode result = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
        if (result != AV_ERR_OK) {
            cout << "WriteSingleTrackSample error! ret is: " << result << endl;
            break;
        }
        FreeBuffer(avMemBuffer);
    } while (ret > 0)
    FreeBuffer(avMemBuffer);
}

void WriteTrackCover(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int coverTrackIndex, int fdInput)
{
    printf("WriteTrackCover\n");
    OH_AVCodecBufferAttr info;
    memset_s(&info, sizeof(info), 0, sizeof(info));
    stat fileStat;
    fstat(fdInput, &fileStat);
    info.size = fileStat.st_size;
    OH_AVMemory* avMemBuffer = OH_AVMemory_Create(info.size);
    uint8_t* data = OH_AVMemory_GetAddr(avMemBuffer);

    int ret = read(fdInput, static_cast<void *>data, info.size);
    if (ret <= 0) {
        OH_AVMemory_Destroy(avMemBuffer);
        return;
    }

    OH_AVErrCode result = muxerDemo->NativeWriteSampleBuffer(handle, coverTrackIndex, avMemBuffer, info);
    if (result != AV_ERR_OK) {
        OH_AVMemory_Destroy(avMemBuffer);
        cout << "WriteTrackCover error! ret is: " << result << endl;
        return;
    }
    if (avMemBuffer != nullptr) {
        OH_AVMemory_Destroy(avMemBuffer);
        avMemBuffer = nullptr;
    }
}

int32_t AddVideoTrackH264ByFd(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int32_t inputFile)
{
    OH_AVFormat* videoFormat = OH_AVFormat_Create();
    if (videoFormat == NULL) {
        printf("video format failed!");
        return -1;
    }
    int extraSize = 0;

    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
        unsigned char buffer[100] = { 0 };
        read(inputFile, buffer, extraSize);
        OH_AVFormat_SetBuffer(videoFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
    }

    OH_AVFormat_SetStringValue(videoFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_WIDTH, WIDTH_640);
    OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_HEIGHT, HEIGHT_360);
    OH_AVFormat_SetLongValue(videoFormat, OH_MD_KEY_BITRATE, VIDEO_BITRATE);

    int32_t trackId;
    muxerDemo->NativeAddTrack(handle, &trackId, videoFormat);
    OH_AVFormat_Destroy(videoFormat);
    return trackId;
}
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001
 * @tc.name      : audio
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001, TestSize.Level2)
{
    OH_AVOutputFormat formatList[] = { AV_OUTPUT_FORMAT_M4A, AV_OUTPUT_FORMAT_MPEG_4 };
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OH_AVOutputFormat format = formatList[i];
        string fileName = "FUNCTION_001_" + to_string(i);
        int32_t fd = muxerDemo->GetFdByName(format, fileName);

        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);

        int32_t audioTrackId = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd);

        OH_AVErrCode ret;

        ret = muxerDemo->NativeStart(handle);
        cout << "Start ret is:" << ret << endl;

        if (audioTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, handle, audioTrackId, audioFileFd);
        }

        ret = muxerDemo->NativeStop(handle);
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is:" << ret << endl;

        close(audioFileFd);
        close(fd);
        delete muxerDemo;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_002
 * @tc.name      : video
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_002, TestSize.Level2)
{
    OH_AVOutputFormat formatList[] = {AV_OUTPUT_FORMAT_M4A, AV_OUTPUT_FORMAT_MPEG_4};
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OH_AVOutputFormat format = formatList[i];
        string fileName = "FUNCTION_002_" + to_string(i);
        int32_t fd = muxerDemo->GetFdByName(format, fileName);

        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);
        int32_t videoTrackId = AddVideoTrackH264ByFd(muxerDemo, handle, videoFileFd);

        OH_AVErrCode ret;

        ret = muxerDemo->NativeStart(handle);
        cout << "Start ret is:" << ret << endl;

        if (videoTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, handle, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->NativeStop(handle);
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is:" << ret << endl;

        close(videoFileFd);
        close(fd);
        delete muxerDemo;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_003
 * @tc.name      : audio and video
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_003, TestSize.Level2)
{
    OH_AVOutputFormat formatList[] = {AV_OUTPUT_FORMAT_M4A, AV_OUTPUT_FORMAT_MPEG_4};
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OH_AVOutputFormat format = formatList[i];
        string fileName = "FUNCTION_003_" + to_string(i);
        int32_t fd = muxerDemo->GetFdByName(format, fileName);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);

        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t audioTrackId = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd);
        int32_t videoTrackId = AddVideoTrackByFd(muxerDemo, handle, videoFileFd);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        OH_AVErrCode ret;

        ret = muxerDemo->NativeStart(handle);
        cout << "Start ret is:" << ret << endl;

        if (audioTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, handle, audioTrackId, audioFileFd);
        }
        if (videoTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, handle, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->NativeStop(handle);
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is:" << ret << endl;

        close(audioFileFd);
        close(videoFileFd);
        close(fd);
        delete muxerDemo;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_004
 * @tc.name      : mp4(SetRotation)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByName(format, "FUNCTION_004");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t audioTrackId = AddAudioTrack(muxerDemo, handle);
    int32_t videoTrackId = AddVideoTrack(muxerDemo, handle);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    OH_AVErrCode ret;
    ret = muxerDemo->NativeSetRotation(handle, 90);
    cout << "SetRotation ret is:" << ret << endl;

    ret = muxerDemo->NativeStart(handle);
    cout << "Start ret is:" << ret << endl;

    WriteTrackSample(muxerDemo, handle, audioTrackId, videoTrackId);

    ret = muxerDemo->NativeStop(handle);
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->NativeDestroy(handle);
    cout << "Destroy ret is:" << ret << endl;

    close(g_inputFile);
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_005
 * @tc.name      : mp4(video audio length not equal)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByName(format, "FUNCTION_005");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t audioTrackId = AddAudioTrack(muxerDemo, handle);
    int32_t videoTrackId = AddVideoTrack(muxerDemo, handle);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    ret = muxerDemo->NativeStart(handle);
    cout << "Start ret is:" << ret << endl;

    WriteTrackSampleShort(muxerDemo, handle, audioTrackId, videoTrackId, 100);

    ret = muxerDemo->NativeStop(handle);
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->NativeDestroy(handle);
    cout << "Destroy ret is:" << ret << endl;

    close(g_inputFile);
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_006
 * @tc.name      : m4a(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_006, TestSize.Level2)
{
    vector<thread> threadVec;
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    for (int i = 0; i < 16; i++)
    {
        threadVec.push_back(thread(RunMuxer, "FUNCTION_006", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AV_ERR_OK, testResult[i]);
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_007
 * @tc.name      : mp4(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_007, TestSize.Level2)
{
    vector<thread> threadVec;
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    for (int i = 0; i < 16; i++)
    {
        threadVec.push_back(thread(RunMuxer, "FUNCTION_007", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AV_ERR_OK, testResult[i]);
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008
 * @tc.name      : m4a(multi audio track)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "FUNCTION_008");

    int32_t audioFileFd1 = open("aac_44100_2.bin", O_RDONLY);
    int32_t audioFileFd2 = open("aac_44100_2.bin", O_RDONLY);

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t audioTrackId1 = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd1);
    int32_t audioTrackId2 = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd2);

    cout << "audio track 1 id is: " << audioTrackId1 << ", audio track 2 id is: " << audioTrackId2 << endl;

    OH_AVErrCode ret;

    ret = muxerDemo->NativeStart(handle);
    cout << "Start ret is:" << ret << endl;

    if (audioTrackId1 >= 0) {
        WriteSingleTrackSample(muxerDemo, handle, audioTrackId1, audioFileFd1);
    }
    if (audioTrackId2 >= 0) {
        WriteSingleTrackSample(muxerDemo, handle, audioTrackId2, audioFileFd2);
    }

    ret = muxerDemo->NativeStop(handle);
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->NativeDestroy(handle);
    cout << "Destroy ret is:" << ret << endl;

    close(audioFileFd1);
    close(audioFileFd2);
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_009
 * @tc.name      : mp4(multi video track)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByName(format, "FUNCTION_009");

    int32_t videoFileFd1 = open("h264_640_360.bin", O_RDONLY);
    int32_t videoFileFd2 = open("h264_640_360.bin", O_RDONLY);

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t videoTrackId1 = AddVideoTrackH264ByFd(muxerDemo, handle, videoFileFd1);
    int32_t videoTrackId2 = AddVideoTrackH264ByFd(muxerDemo, handle, videoFileFd2);

    cout << "video track 1 id is: " << videoTrackId1 << ", video track 2 id is: " << videoTrackId2 << endl;

    OH_AVErrCode ret;

    ret = muxerDemo->NativeStart(handle);
    cout << "Start ret is:" << ret << endl;

    if (videoTrackId1 >= 0) {
        WriteSingleTrackSample(muxerDemo, handle, videoTrackId1, videoFileFd1);
    }
    if (videoTrackId2 >= 0) {
        WriteSingleTrackSample(muxerDemo, handle, videoTrackId2, videoFileFd2);
    }


    ret = muxerDemo->NativeStop(handle);
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->NativeDestroy(handle);
    cout << "Destroy ret is:" << ret << endl;

    close(videoFileFd1);
    close(videoFileFd2);
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_010
 * @tc.name      : m4a(auido video with cover)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_010, TestSize.Level2)
{
    string coverTypeList[] = {"bmp", "jpg", "png"};
    for (int i = 0; i < 3; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        string outputFile = "FUNCTION_010_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];
        OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
        int32_t fd = muxerDemo->GetFdByName(format, outputFile);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t audioTrackId = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd);
        int32_t videoTrackId = AddVideoTrackH264ByFd(muxerDemo, handle, videoFileFd);
        int32_t coverTrackId = AddCoverTrack(muxerDemo, handle, coverTypeList[i]);

        cout << "audio track id is: " << audioTrackId << ", cover track id is: " << coverTrackId << endl;

        OH_AVErrCode ret;

        ret = muxerDemo->NativeStart(handle);
        cout << "Start ret is:" << ret << endl;

        WriteTrackCover(muxerDemo, handle, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, handle, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, handle, videoTrackId, videoFileFd);

        ret = muxerDemo->NativeStop(handle);
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is:" << ret << endl;

        close(audioFileFd);
        close(videoFileFd);
        close(coverFileFd);
        close(fd);
        delete muxerDemo;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_011
 * @tc.name      : mp4(audio video with cover)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_011, TestSize.Level2)
{
    string coverTypeList[] = { "bmp", "jpg", "png" };
    for (int i = 0; i < 3; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        string outputFile = "FUNCTION_011_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];

        OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
        int32_t fd = muxerDemo->GetFdByName(format, outputFile);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t audioTrackId = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd);
        int32_t videoTrackId = AddVideoTrackByFd(muxerDemo, handle, videoFileFd);
        int32_t coverTrackId = AddCoverTrack(muxerDemo, handle, coverTypeList[i]);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId <<
            ", cover track id is: " << coverTrackId << endl;

        OH_AVErrCode ret;

        ret = muxerDemo->NativeStart(handle);
        cout << "Start ret is:" << ret << endl;

        WriteTrackCover(muxerDemo, handle, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, handle, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, handle, videoTrackId, videoFileFd);

        ret = muxerDemo->NativeStop(handle);
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is:" << ret << endl;

        close(audioFileFd);
        close(videoFileFd);
        close(coverFileFd);
        close(fd);
        delete muxerDemo;
    }
}