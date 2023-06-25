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
#include "gtest/gtest.h"
#include "AVMuxerDemo.h"
#include "fcntl.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Plugin;
using namespace Ffmpeg;
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t INFO_SIZE = 100;
constexpr uint32_t BIG_EXTRA_SIZE = 100;
constexpr uint32_t WIDTH = 352;
constexpr uint32_t HEIGHT = 288;

namespace {
    class FFmpegAVMuxerStablityTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void FFmpegAVMuxerStablityTest::SetUpTestCase() {}
    void FFmpegAVMuxerStablityTest::TearDownTestCase() {}
    void FFmpegAVMuxerStablityTest::SetUp() {}
    void FFmpegAVMuxerStablityTest::TearDown() {}

    static int g_inputFile = -1;
    static const int DATA_AUDIO_ID = 0;
    static const int DATA_VIDEO_ID = 1;

    constexpr int RUN_TIMES = 1000;
    constexpr int RUN_TIME = 8 * 3600;


    Status SetRotation(AVMuxerDemo* muxerDemo)
    {
        int32_t rotation = 0;
        return muxerDemo->FFmpegSetRotation(rotation);
    }

    Status AddTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription audioParams;
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);

        return muxerDemo->FFmpegAddTrack(trackIndex, audioParams);
    }

    Status WriteSample(AVMuxerDemo* muxerDemo, uint32_t trackIndex)
    {
        uint8_t data[100];

        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        info.presentationTimeUs= 0;
        info.size = INFO_SIZE;
        flag = AVCODEC_BUFFER_FLAG_NONE;
        info.offset = 0;

        return muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
    }

    Status addAudioTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription audioParams;
       
        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
  
        Status ret = muxerDemo->FFmpegAddTrack(trackIndex, audioParams);
        
        return ret;
    }

    Status addAudioTrackAAC(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);

        Status ret = muxerDemo->FFmpegAddTrack(trackIndex, audioParams);
        
        return ret;
    }

    Status addVideoTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription videoParams;
  
        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HEIGHT);

        Status ret = muxerDemo->FFmpegAddTrack(trackIndex, videoParams);

        return ret;
    }


    void removeHeader()
    {
        int extraSize = 0;
        unsigned char buffer[100] = { 0 };
        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
        }
    }

    void WriteTrackSample(AVMuxerDemo* muxerDemo,  int audioTrackIndex, int videoTrackIndex)
    {
        int dataTrackId = 0;
        int dataSize = 0;
        int ret = 0;
        int trackId = 0;
        AVCodecBufferInfo info {0, 0, 0};
        uint32_t trackIndex;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
        uint8_t data[1024 * 1024] = { 0 };
        while (1) {
            ret = read(g_inputFile, (void*)&dataTrackId, sizeof(dataTrackId));
            if (ret <= 0) {
                cout << "read dataTrackId error, ret is: " << (int)ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)&info.presentationTimeUs, sizeof(info.presentationTimeUs));
            if (ret <= 0) {
                cout << "read info.presentationTimeUs error, ret is: " << (int)ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)&dataSize, sizeof(dataSize));
            if (ret <= 0) {
                cout << "read dataSize error, ret is: " << (int)ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)data, dataSize);
            if (ret <= 0) {
                cout << "read data error, ret is: " << (int)ret << endl;
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
                trackIndex = trackId;
                Status result = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
                if (result != Status::OK) {
                    cout << "Status_WriteSampleBuffer error! ret is: " << (int)result << endl;
                    return;
                }
            }
        }
    }


    Status addAudioTrackByFd(AVMuxerDemo* muxerDemo, int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);

        Status ret = muxerDemo->FFmpegAddTrack(trackIndex, audioParams);

        return ret;
    }

    Status addAudioTrackAACByFd(AVMuxerDemo* muxerDemo,  int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);

        Status ret = muxerDemo->FFmpegAddTrack(trackIndex, audioParams);

        return ret;
    }

    Status addVideoTrackByFd(AVMuxerDemo* muxerDemo,  int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription videoParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HEIGHT);

        Status ret = muxerDemo->FFmpegAddTrack(trackIndex, videoParams);

        return ret;
    }

    void WriteTrackSampleByFd(AVMuxerDemo* muxerDemo,  int audioTrackIndex, int videoTrackIndex, int32_t inputFile)
    {
        int dataTrackId = 0;
        int dataSize = 0;
        int ret = 0;
        int trackId = 0;
        AVCodecBufferInfo info {0, 0, 0};
        uint32_t trackIndex;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
        unsigned char* avMuxerDemoBuffer = nullptr;
        int avMuxerDemoBufferSize = 0;
        string resultStr = "";
        while (1) {
            ret = read(inputFile, (void*)&dataTrackId, sizeof(dataTrackId));
            if (ret <= 0) {
                cout << "read dataTrackId error, ret is: " << (int)ret << endl;
                return;
            }
            ret = read(inputFile, (void*)&info.presentationTimeUs, sizeof(info.presentationTimeUs));
            if (ret <= 0) {
                cout << "read info.presentationTimeUs error, ret is: " << (int)ret << endl;
                return;
            }
            ret = read(inputFile, (void*)&dataSize, sizeof(dataSize));
            if (ret <= 0) {
                cout << "read dataSize error, ret is: " << (int)ret << endl;
                return;
            }

            if (avMuxerDemoBuffer != nullptr && dataSize > avMuxerDemoBufferSize) {
                free(avMuxerDemoBuffer);
                avMuxerDemoBufferSize = 0;
                avMuxerDemoBuffer = nullptr;
            }
            if (avMuxerDemoBuffer == nullptr) {
                avMuxerDemoBuffer = (unsigned char*)malloc(dataSize);
                avMuxerDemoBufferSize = dataSize;
                if (avMuxerDemoBuffer == nullptr) {
                    printf("error malloc memory!\n");
                    break;
                }
            }
            resultStr = "inputFile is: " + to_string(inputFile) + ", avMuxerDemoBufferSize is " +
                to_string(avMuxerDemoBufferSize);
            cout << resultStr << endl;

            ret = read(inputFile, (void*)avMuxerDemoBuffer, dataSize);
            if (ret <= 0) {
                cout << "read data error, ret is: " << (int)ret << endl;
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
                trackIndex = trackId;
                Status result = muxerDemo->FFmpegWriteSample(trackIndex, avMuxerDemoBuffer, info, flag);
                if (result != Status::OK) {
                    cout << "OH_AVMuxer_WriteSampleBuffer error! ret is: " << (int)result << endl;
                    break;
                }
            }
        }
        if (avMuxerDemoBuffer != nullptr) {
            free(avMuxerDemoBuffer);
        }
    }

    void runMuxer(string testcaseName, int threadId, OutputFormat format)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        time_t startTime = time(nullptr);
        time_t curTime = time(nullptr);

        while (difftime(curTime, startTime) < RUN_TIME) {
            cout << "thread id is: " << threadId << ", run time : " << difftime(curTime, startTime) <<
                " seconds" << endl;
            string fileName = testcaseName + "_" + to_string(threadId);

            cout << "thread id is: " << threadId << ", cur file name is: " << fileName << endl;
            int32_t fd = muxerDemo->FFmpeggetFdByName(format, fileName);

            int32_t inputFile;
            int32_t audioTrackId;
            int32_t videoTrackId;

            cout << "thread id is: " << threadId << ", fd is: " << fd << endl;
            muxerDemo->FFmpegCreate(fd);

            Status ret;

            if (format == OUTPUT_FORMAT_MPEG_4) {
                cout << "thread id is: " << threadId << ", format is: " << format << endl;
                inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);
                addAudioTrackByFd(muxerDemo, inputFile, audioTrackId);
                addVideoTrackByFd(muxerDemo, inputFile, videoTrackId);
            } else {
                cout << "thread id is: " << threadId << ", format is: " << format << endl;
                inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);
                addAudioTrackAACByFd(muxerDemo, inputFile, audioTrackId);
                videoTrackId = -1;
                int extraSize = 0;
                unsigned char buffer[100] = { 0 };
                read(inputFile, (void*)&extraSize, sizeof(extraSize));
                if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
                    read(inputFile, buffer, extraSize);
                }
            }

            cout << "thread id is: " << threadId << ", audio track id is: " << audioTrackId <<
                ", video track id is: " << videoTrackId << endl;

            ret = muxerDemo->FFmpegStart();
            cout << "thread id is: " << threadId << ", Start ret is:" << (int)ret << endl;

            WriteTrackSampleByFd(muxerDemo, audioTrackId, videoTrackId, inputFile);

            ret = muxerDemo->FFmpegStop();
            cout << "thread id is: " << threadId << ", Stop ret is:" << (int)ret << endl;

            ret = muxerDemo->FFmpegDestroy();
            cout << "thread id is: " << threadId << ", Destroy ret is:" << (int)ret << endl;

            close(inputFile);
            close(fd);
            curTime = time(nullptr);
        }
        delete muxerDemo;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001
 * @tc.name      : Create(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001");

    g_inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);
    
    for (int i = 0; i < RUN_TIMES; i++)
    {
        muxerDemo->FFmpegCreate(fd);
        cout << "run time is: " << i << endl;
        muxerDemo->FFmpegDestroy();
    }

    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002
 * @tc.name      : SetRotation(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002");

    muxerDemo->FFmpegCreate(fd);
    

    for (int i = 0; i < RUN_TIMES; i++)
    {
        Status ret = SetRotation(muxerDemo);
        cout << "run time is: " << i << ", ret is:" << (int)ret << endl;
    }

    muxerDemo->FFmpegDestroy();

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003
 * @tc.name      : AddTrack(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003");

    muxerDemo->FFmpegCreate(fd);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        int32_t trackId;
        Status ret = AddTrack(muxerDemo, trackId);
        cout << "run time is: " << i << ", ret  is:" << (int)ret << endl;
    }

    muxerDemo->FFmpegDestroy();

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004
 * @tc.name      : Start(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004");

    muxerDemo->FFmpegCreate(fd);
    

    int32_t trackId;
    Status ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = muxerDemo->FFmpegStart();
        cout << "run time is: " << i << ", ret is:" << (int)ret << endl;
    }

    muxerDemo->FFmpegDestroy();

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005
 * @tc.name      : WriteSample(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005");

    muxerDemo->FFmpegCreate(fd);
    

    int32_t trackId;
    Status ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = WriteSample(muxerDemo, trackId);
        cout << "run time is: " << i << ", ret is:" << (int)ret << endl;
    }

    muxerDemo->FFmpegDestroy();

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006
 * @tc.name      : Stop(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006");

    muxerDemo->FFmpegCreate(fd);
    

    int32_t trackId;
    Status ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    for (int i = 0; i < RUN_TIMES; i++)
    {
        ret = muxerDemo->FFmpegStop();
        cout << "run time is: " << i << ", ret is:" << (int)ret << endl;
    }

    muxerDemo->FFmpegDestroy();

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007
 * @tc.name      : Destroy(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007");

    muxerDemo->FFmpegCreate(fd);
    

    for (int i = 0; i < RUN_TIMES; i++)
    {
        Status ret = muxerDemo->FFmpegDestroy();
        cout << "run time is: " << i << ", ret is:" << (int)ret << endl;
    }

    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008
 * @tc.name      : m4a(long time)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    time_t startTime = time(nullptr);
    time_t curTime = time(nullptr);

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        OutputFormat format = OUTPUT_FORMAT_M4A;
        int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008");

        g_inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);

        muxerDemo->FFmpegCreate(fd);

        int32_t audioTrackId;
        Status ret = addAudioTrackAAC(muxerDemo, audioTrackId);
        int32_t videoTrackId = -1;
        removeHeader();

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        ret = muxerDemo->FFmpegStart();
        cout << "Start ret is:" << (int)ret << endl;

        WriteTrackSample(muxerDemo, audioTrackId, videoTrackId);

        ret = muxerDemo->FFmpegStop();
        cout << "Stop ret is:" << (int)ret << endl;

        ret = muxerDemo->FFmpegDestroy();
        cout << "Destroy ret is:" << (int)ret << endl;

        close(g_inputFile);
        close(fd);
        curTime = time(nullptr);
    }
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_009
 * @tc.name      : mp4(long time)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    time_t startTime = time(nullptr);
    time_t curTime = time(nullptr);

    while (difftime(curTime, startTime) < RUN_TIME)
    {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;

        OutputFormat format = OUTPUT_FORMAT_MPEG_4;
        int32_t fd = muxerDemo->FFmpeggetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_009");

        g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

        muxerDemo->FFmpegCreate(fd);

        int32_t audioTrackId;
        Status ret = addAudioTrack(muxerDemo, audioTrackId);
        int32_t videoTrackId;
        ret= addVideoTrack(muxerDemo, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        ret = muxerDemo->FFmpegStart();
        cout << "Start ret is:" << (int)ret << endl;

        WriteTrackSample(muxerDemo, audioTrackId, videoTrackId);

        ret = muxerDemo->FFmpegStop();
        cout << "Stop ret is:" << (int)ret << endl;

        ret = muxerDemo->FFmpegDestroy();
        cout << "Destroy ret is:" << (int)ret << endl;

        close(g_inputFile);
        close(fd);
        curTime = time(nullptr);
    }
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_010
 * @tc.name      : m4a(thread long time)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_010, TestSize.Level2)
{
    vector<thread> threadVec;
    OutputFormat format = OUTPUT_FORMAT_M4A;
    for (int i = 0; i < 16; i++)
    {
        threadVec.push_back(thread(runMuxer, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_010", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011
 * @tc.name      : mp4(thread long time)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011, TestSize.Level2)
{
    vector<thread> threadVec;
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    for (int i = 0; i < 16; i++)
    {
        threadVec.push_back(thread(runMuxer, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
}