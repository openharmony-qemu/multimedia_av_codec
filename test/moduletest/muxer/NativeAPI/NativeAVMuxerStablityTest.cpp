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
#include <ctime>
#include <sys/time.h>
#include "gtest/gtest.h"
#include "AVMuxerDemo.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;


namespace {
    class NativeAVMuxerStablityTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeAVMuxerStablityTest::SetUpTestCase() {}
    void NativeAVMuxerStablityTest::TearDownTestCase() {}
    void NativeAVMuxerStablityTest::SetUp() {}
    void NativeAVMuxerStablityTest::TearDown() {}

    static int g_inputFile = -1;
    static const int DATA_AUDIO_ID = 0;
    static const int DATA_VIDEO_ID = 1;

    constexpr int RUN_TIMES = 2000;
    constexpr int RUN_TIME = 12 * 3600;
    constexpr int32_t BIG_EXTRA_SIZE = 100;
    constexpr int32_t SMALL_EXTRA_SIZE = 0;

    constexpr int32_t CHANNEL_COUNT_STEREO = 2;
    constexpr int32_t SAMPLE_RATE_441K = 44100;
    constexpr int64_t AUDIO_BITRATE = 320000;
    constexpr int64_t VIDEO_BITRATE = 524569;

    constexpr int32_t CODEC_CONFIG = 100;
    constexpr int32_t CHANNEL_COUNT_MONO = 1;
    constexpr int32_t SAMPLE_RATE_48K = 48000;
    constexpr int32_t PROFILE = 0;

    constexpr int32_t INFO_SIZE = 100;
    constexpr int32_t WIDTH = 352;
    constexpr int32_t HEIGHT = 288;

    constexpr int32_t WIDTH_640 = 640;
    constexpr int32_t WIDTH_720 = 720;
    constexpr int32_t HEIGHT_360 = 360;
    constexpr int32_t HEIGHT_480 = 480;

    int32_t testResult[10] = { -1 };

    OH_AVErrCode SetRotation(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
    {
        int32_t rotation = 0;

        OH_AVErrCode ret = muxerDemo->NativeSetRotation(handle, rotation);

        return ret;
    }

    int32_t AddTrack(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
    {
        uint8_t a[100];

        OH_AVFormat* trackFormat = OH_AVFormat_Create();
        OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
        OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, AUDIO_BITRATE);
        OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S16LE);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_MONO);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_48K);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

        int32_t trackId;
        muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
        OH_AVFormat_Destroy(trackFormat);
        return trackId;
    }

    OH_AVErrCode WriteSampleBuffer(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, uint32_t trackIndex)
    {
        OH_AVMemory* avMemBuffer = OH_AVMemory_Create(INFO_SIZE);

        OH_AVCodecBufferAttr info;
        info.size = INFO_SIZE;
        info.pts = 0;
        info.pts = 0;
        info.offset = 0;
        info.flags = 0;

        OH_AVErrCode ret = muxerDemo->NativeWriteSampleBuffer(handle, trackIndex, avMemBuffer, info);

        OH_AVMemory_Destroy(avMemBuffer);
        return ret;
    }

    OH_AVErrCode WriteSampleBufferNew(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, uint32_t trackIndex)
    {
        OH_AVBuffer* avBuffer = OH_AVBuffer_Create(INFO_SIZE);

        OH_AVCodecBufferAttr info;
        info.size = INFO_SIZE;
        info.pts = 0;
        info.offset = 0;
        info.flags = 0;
        OH_AVBuffer_SetBufferAttr(avBuffer, &info);

        OH_AVErrCode ret = muxerDemo->NativeWriteSampleBuffer(handle, trackIndex, avBuffer);

        OH_AVBuffer_Destroy(avBuffer);
        return ret;
    }

    int32_t AddAudioTrack(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
    {
        OH_AVFormat* audioFormat = OH_AVFormat_Create();
        if (audioFormat == NULL) {
            printf("audio format failed!");
            return -1;
        }
        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
            read(g_inputFile, buffer, extraSize);
            OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }

        OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S32P);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_STEREO);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_441K);
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

        unsigned char buffer[100] = { 0 };

        read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
            read(g_inputFile, buffer, extraSize);
            OH_AVFormat_SetBuffer(videoFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }

        OH_AVFormat_SetStringValue(videoFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_MPEG4);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_WIDTH, WIDTH);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_HEIGHT, HEIGHT);
        OH_AVFormat_SetLongValue(videoFormat, OH_MD_KEY_BITRATE, VIDEO_BITRATE);

        int32_t trackId;
        muxerDemo->NativeAddTrack(handle, &trackId, videoFormat);
        OH_AVFormat_Destroy(videoFormat);
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
        int dataTrackId = 0;
        int dataSize = 0;
        int ret = 0;
        int trackId = 0;
        OH_AVCodecBufferAttr info { 0, 0, 0, 0 };

        OH_AVMemory* avMemBuffer = nullptr;
        uint8_t* data = nullptr;
        bool readRet;
        do {
            readRet = ReadFile(dataTrackId, info.pts, dataSize);
            if (!readRet) {
                return;
            }

            avMemBuffer = OH_AVMemory_Create(dataSize);
            data = OH_AVMemory_GetAddr(avMemBuffer);
            ret = read(g_inputFile, static_cast<void*>(data), dataSize);
            if (ret <= 0) {
                cout << "read data error, ret is: " << ret << endl;
                break;
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
                    break;
                }
            }

            if (avMemBuffer != nullptr) {
                OH_AVMemory_Destroy(avMemBuffer);
                avMemBuffer = nullptr;
            }
        }while (ret > 0)
        if (avMemBuffer != nullptr) {
            OH_AVMemory_Destroy(avMemBuffer);
            avMemBuffer = nullptr;
        }
    }

    int32_t AddAudioTrackByFd(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int32_t inputFile)
    {
        OH_AVFormat* audioFormat = OH_AVFormat_Create();
        if (audioFormat == NULL) {
            printf("audio format failed!");
            return -1;
        }
        int extraSize = 0;

        unsigned char buffer[100] = { 0 };

        read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
            read(inputFile, buffer, extraSize);
            OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }

        OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_MPEG);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S32P);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_STEREO);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_441K);
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
        unsigned char buffer[100] = { 0 };

        read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
            read(inputFile, buffer, extraSize);
            OH_AVFormat_SetBuffer(audioFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }

        OH_AVFormat_SetStringValue(audioFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUDIO_SAMPLE_FORMAT, SAMPLE_S32P);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT_STEREO);
        OH_AVFormat_SetIntValue(audioFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE_441K);
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
        unsigned char buffer[100] = { 0 };

        read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
            read(inputFile, buffer, extraSize);
            OH_AVFormat_SetBuffer(videoFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }

        OH_AVFormat_SetStringValue(videoFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_MPEG4);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_WIDTH, WIDTH_720);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_HEIGHT, HEIGHT_480);
        OH_AVFormat_SetLongValue(videoFormat, OH_MD_KEY_BITRATE, VIDEO_BITRATE);

        int32_t trackId;
        muxerDemo->NativeAddTrack(handle, &trackId, videoFormat);
        OH_AVFormat_Destroy(videoFormat);
        return trackId;
    }

    int32_t AddVideoTrackH264ByFd(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int32_t inputFile)
    {
        OH_AVFormat* videoFormat = OH_AVFormat_Create();
        if (videoFormat == NULL) {
            printf("video format failed!");
            return -1;
        }
        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > SMALL_EXTRA_SIZE) {
            read(inputFile, buffer, extraSize);
            OH_AVFormat_SetBuffer(videoFormat, OH_MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }

        OH_AVFormat_SetStringValue(videoFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_VIDEO_AVC);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_WIDTH, WIDTH_640);
        OH_AVFormat_SetIntValue(videoFormat, OH_MD_KEY_HEIGHT, HEIGHT_360);
        OH_AVFormat_SetLongValue(videoFormat, OH_MD_KEY_BITRATE, VIDEO_BITRATE);

        int32_t trackId;
        muxerDemo->NativeAddTrack(handle, &trackId, videoFormat);
        OH_AVFormat_Destroy(videoFormat);
        return trackId;
    }


    int32_t AddCoverTrack(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, string coverType)
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

    void FreeBuffer(OH_AVMemory** avMemBuffer)
    {
        if (*avMemBuffer != nullptr) {
            OH_AVMemory_Destroy(*avMemBuffer);
            *avMemBuffer = nullptr;
        }
    }

    void WriteSingleTrackSample(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int trackId, int fd)
    {
        int ret = 0;
        int dataSize = 0;
        int flags = 0;
        OH_AVMemory* avMemBuffer = nullptr;
        uint8_t* data = nullptr;
        OH_AVCodecBufferAttr info;
        memset_s(&info, sizeof(info), 0, sizeof(info));
        do {
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

            FreeBuffer(&avMemBuffer);
        }while (ret > 0)
        FreeBuffer(&avMemBuffer);
    }

    void WriteTrackCover(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, int coverTrackIndex, int fdInput)
    {
        printf("WriteTrackCover\n");
        OH_AVCodecBufferAttr info;
        memset_s(&info, sizeof(info), 0, sizeof(info));
        struct stat fileStat;
        fstat(fdInput, &fileStat);
        info.size = fileStat.st_size;
        OH_AVMemory* avMemBuffer = OH_AVMemory_Create(info.size);
        uint8_t* data = OH_AVMemory_GetAddr(avMemBuffer);

        int ret = read(fdInput, (void *)data, info.size);
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
        OH_AVMemory_Destroy(avMemBuffer);
    }

    void WriteByFormat(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, OH_AVOutputFormat format)
    {
        OH_AVErrCode ret;
        int32_t audioTrackId = -1;
        int32_t videoTrackId = -1;
        int32_t coverTrackId = -1;

        int32_t audioFileFd;
        int32_t videoFileFd;
        int32_t coverFileFd;

        if (format == AV_OUTPUT_FORMAT_MPEG_4) {
            audioFileFd = open("mpeg_44100_2.bin", O_RDONLY);
            videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);
            coverFileFd = open("greatwall.jpg", O_RDONLY);

            audioTrackId = AddAudioTrackByFd(muxerDemo, handle, audioFileFd);
            videoTrackId = AddVideoTrackByFd(muxerDemo, handle, videoFileFd);
            coverTrackId = AddCoverTrack(muxerDemo, handle, "jpg");
        } else {
            audioFileFd = open("aac_44100_2.bin", O_RDONLY);
            videoFileFd = open("h264_640_360.bin", O_RDONLY);
            coverFileFd = open("greatwall.jpg", O_RDONLY);

            audioTrackId = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd);
            videoTrackId = AddVideoTrackH264ByFd(muxerDemo, handle, videoFileFd);
            coverTrackId = AddCoverTrack(muxerDemo, handle, "jpg");
        }

        ret = muxerDemo->NativeStart(handle);

        if (coverTrackId >= 0) {
            WriteTrackCover(muxerDemo, handle, coverTrackId, coverFileFd);
        }
        if (audioTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, handle, audioTrackId, audioFileFd);
        }
        if (videoTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, handle, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->NativeStop(handle);
        cout << "Stop ret is " << ret << endl;

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is " << ret << endl;

        close(audioFileFd);
        close(videoFileFd);
        close(coverFileFd);
    }

    void RunMuxer(string testcaseName, int threadId, OH_AVOutputFormat format)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        time_t startTime = time(nullptr);
        ASSERT_NE(startTime, -1);
        time_t curTime = startTime;

        while (difftime(curTime, startTime) < RUN_TIME) {
            string fileName = testcaseName + "_" + to_string(threadId);
            int32_t fd = muxerDemo->GetFdByName(format, fileName);

            OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
            WriteByFormat(muxerDemo, handle, format);

            close(fd);
            curTime = time(nullptr);
            ASSERT_NE(curTime, -1);
        }
        testResult[threadId] = AV_ERR_OK;
        delete muxerDemo;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001
 * @tc.name      : Create(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_001");

    g_inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", handle is:" << handle << endl;
        muxerDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002
 * @tc.name      : SetRotation(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_002");

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        OH_AVErrCode ret = SetRotation(muxerDemo, handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    muxerDemo->NativeDestroy(handle);
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003
 * @tc.name      : AddTrack(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_003");

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        int32_t trackId = AddTrack(muxerDemo, handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", track id is:" << trackId << endl;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    muxerDemo->NativeDestroy(handle);

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004
 * @tc.name      : Start(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_004");

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t trackId = AddTrack(muxerDemo, handle);
        ASSERT_EQ(0, trackId);

        gettimeofday(&start, nullptr);
        OH_AVErrCode ret = muxerDemo->NativeStart(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", ret is:" << ret << endl;

        muxerDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005
 * @tc.name      : WriteSampleBuffer(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_005");

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = AddTrack(muxerDemo, handle);
    ASSERT_EQ(0, trackId);

    OH_AVErrCode ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        ret = WriteSampleBuffer(muxerDemo, handle, trackId);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    muxerDemo->NativeDestroy(handle);

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005_1
 * @tc.name      : WriteSampleBuffer(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005_1, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_005");

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t trackId = AddTrack(muxerDemo, handle);
    ASSERT_EQ(0, trackId);

    OH_AVErrCode ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        gettimeofday(&start, nullptr);
        ret = WriteSampleBufferNew(muxerDemo, handle, trackId);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;
    muxerDemo->NativeDestroy(handle);

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006
 * @tc.name      : Stop(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_006");

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t trackId = AddTrack(muxerDemo, handle);
        ASSERT_EQ(0, trackId);

        OH_AVErrCode ret = muxerDemo->NativeStart(handle);
        ASSERT_EQ(AV_ERR_OK, ret);

        ret = WriteSampleBuffer(muxerDemo, handle, trackId);
        ASSERT_EQ(AV_ERR_OK, ret);

        gettimeofday(&start, nullptr);
        ret = muxerDemo->NativeStop(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", ret is:" << ret << endl;

        muxerDemo->NativeDestroy(handle);
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007
 * @tc.name      : Destroy(2000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_007");

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++)
    {
        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        gettimeofday(&start, nullptr);
        OH_AVErrCode ret = muxerDemo->NativeDestroy(handle);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;

        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "2000 times finish, run time is " << totalTime << endl;

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008
 * @tc.name      : m4a(long time)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
        int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_008");

        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
        ASSERT_NE(nullptr, handle);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);
        int32_t coverFileFd = open("greatwall.jpg", O_RDONLY);

        int32_t audioTrackId = AddAudioTrackAACByFd(muxerDemo, handle, audioFileFd);
        int32_t videoTrackId = AddVideoTrackH264ByFd(muxerDemo, handle, videoFileFd);
        int32_t coverTrackId = AddCoverTrack(muxerDemo, handle, "jpg");

        OH_AVErrCode ret;

        ret = muxerDemo->NativeStart(handle);
        cout << "Start ret is:" << ret << endl;

        if (coverTrackId >= 0) {
            WriteTrackCover(muxerDemo, handle, coverTrackId, coverFileFd);
        }
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
        close(coverFileFd);
        close(fd);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_009
 * @tc.name      : mp4(long time)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;

        OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
        int32_t fd = muxerDemo->GetFdByName(format, "STABILITY_009");

        if (fd < 0) {
            cout << "open file failed !!! fd is " << fd << endl;
            continue;
        }

        g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

        OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);

        int32_t audioTrackId = AddAudioTrack(muxerDemo, handle);
        int32_t videoTrackId = AddVideoTrack(muxerDemo, handle);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        OH_AVErrCode ret;

        ret = muxerDemo->NativeStart(handle);
        cout << "Start ret is:" << ret << endl;

        WriteTrackSample(muxerDemo, handle, audioTrackId, videoTrackId);

        ret = muxerDemo->NativeStop(handle);
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is:" << ret << endl;

        close(g_inputFile);
        close(fd);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_010
 * @tc.name      : m4a(thread long time)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_010, TestSize.Level2)
{
    vector<thread> threadVec;
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    for (int i = 0; i < 10; i++)
    {
        threadVec.push_back(thread(RunMuxer, "STABILITY_010", i, format));
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
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011
 * @tc.name      : mp4(thread long time)
 * @tc.desc      : Function test
 */
HWTEST_F(NativeAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011, TestSize.Level2)
{
    vector<thread> threadVec;
    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    for (int i = 0; i < 10; i++)
    {
        threadVec.push_back(thread(RunMuxer, "STABILITY_011", i, format));
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
