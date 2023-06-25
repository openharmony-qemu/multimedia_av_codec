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
#include "gtest/gtest.h"
#include "AVMuxerDemo.h"
#include "fcntl.h"
#include "avcodec_info.h"
#include "securec.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SAMPLE_RATE = 44100;
constexpr uint32_t CHANNEL_COUNT = 2;

namespace {
    class FFmpegAVMuxerFunctionTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void FFmpegAVMuxerFunctionTest::SetUpTestCase() {}
    void FFmpegAVMuxerFunctionTest::TearDownTestCase() {}
    void FFmpegAVMuxerFunctionTest::SetUp() {}
    void FFmpegAVMuxerFunctionTest::TearDown() {}

    static int g_inputFile = -1;
    static const int DATA_AUDIO_ID = 0;
    static const int DATA_VIDEO_ID = 1;

    Plugin::Status addAudioTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription audioParams;
       
        int extraSize = 0;
        constexpr uint32_t BIG_EXTRA_SIZE = 100;
        unsigned char buffer[100] = { 0 };
        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);

        Plugin::Status ret = muxerDemo->FFmpegAddTrack(trackIndex, audioParams);
        
        return ret;
    }


    Plugin::Status addVideoTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription videoParams;
  
        int extraSize = 0;
        constexpr uint32_t BIG_EXTRA_SIZE = 100;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extraSize);
        }
        constexpr uint32_t WIDTH = 352;
        constexpr uint32_t HEIGHT = 288;
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HEIGHT);

        Plugin::Status ret = muxerDemo->FFmpegAddTrack(trackIndex, videoParams);

        return ret;
    }


    Plugin::Status addVideoTrackH264ByFd(AVMuxerDemo* muxerDemo, int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription videoParams;
   
        int extraSize = 0;
        constexpr uint32_t BIG_EXTRA_SIZE = 100;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extraSize);
        }
        constexpr uint32_t WIDTH = 352;
        constexpr uint32_t HEIGHT = 288;
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HEIGHT);

        Plugin::Status ret = muxerDemo->FFmpegAddTrack(trackIndex, videoParams);

        return ret;
    }
    Plugin::Status addCoverTrack(AVMuxerDemo* muxerDemo, string coverType, int32_t trackIndex)
    {
        MediaDescription coverParams;

        if (coverType == "jpg") {
            coverParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_JPG);
        } else if (coverType == "png") {
            coverParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_PNG);
        } else {
            coverParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_BMP);
        }
        constexpr uint32_t WIDTH = 352;
        constexpr uint32_t HEIGHT = 288;
        coverParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
        coverParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HEIGHT);

        Plugin::Status ret = muxerDemo->FFmpegAddTrack(trackIndex, coverParams);
        
        return ret;
    }

    void removeHeader()
    {
        int extraSize = 0;
        constexpr uint32_t BIG_EXTRA_SIZE = 100;
        unsigned char buffer[100] = { 0 };
        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
        }
    }

    void WriteTrackSample(AVMuxerDemo* muxerDemo, int audioTrackIndex, int videoTrackIndex)
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
                cout << "read nfo.presentationTimeUs error, ret is: " << (int)ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)&dataSize, sizeof(dataSize));
            if (ret <= 0) {
                cout << "read nfo.size error, ret is: " << (int)ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)data, dataSize);
            if (ret <= 0) {
                cout << "read info.flags error, ret is: " << (int)ret << endl;
                return;
            }
            info.size = dataSize;
            if (dataTrackId == DATA_AUDIO_ID) {
                trackIndex = audioTrackIndex;
            } else if (dataTrackId == DATA_VIDEO_ID) {
                trackIndex = videoTrackIndex;
            } else {
                cout << "error trackIndex : " << trackId << endl;
            }
            if (dataTrackId >= 0) {
                Plugin::Status result = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
                if (result != Plugin::Status::NO_ERROR) {
                    cout << "Status_WriteSampleBuffer error! result is: " << (int)result << endl;
                    return;
                }
            }
        }
    }

    void WriteTrackSampleShort(AVMuxerDemo* muxerDemo, int audioTrackIndex, int videoTrackIndex, int audioWriteTime)
    {
        int dataTrackId = 0;
        int dataSize = 0;
        int ret = 0;
        int trackId = 0;
        int curTime = 0;
        AVCodecBufferInfo info {0, 0, 0};
        uint32_t trackIndex;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
        uint8_t data[1024 * 1024] = { 0 };
        while (1) {
            ret = read(g_inputFile, (void*)&dataTrackId, sizeof(dataTrackId));
            if (ret <= 0) { return; }
            ret = read(g_inputFile, (void*)&info.presentationTimeUs, sizeof(info.presentationTimeUs));
            if (ret <= 0) { return; }
            ret = read(g_inputFile, (void*)&dataSize, sizeof(dataSize));
            if (ret <= 0) { return; }
            ret = read(g_inputFile, (void*)data, dataSize);
            if (ret <= 0) { return; }

            info.size = dataSize;
            if (dataTrackId == DATA_AUDIO_ID) {
                trackIndex = audioTrackIndex;
            } else if (dataTrackId == DATA_VIDEO_ID) {
                trackIndex = videoTrackIndex;
            } else {
                printf("error dataTrackId : %d", trackId);
            }
            if (trackId >= 0) {
                if (trackId == audioTrackIndex && curTime > audioWriteTime) {
                    continue;
                } else if (trackId == audioTrackIndex) {
                    curTime++;
                }
                Plugin::Status result = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
                if (result != Plugin::Status::NO_ERROR) {
                    printf("OH_AVMuxer_WriteSampleBuffer error!");
                    return;
                }
            }
        }
    }


    Plugin::Status addAudioTrackByFd(AVMuxerDemo* muxerDemo, int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        constexpr uint32_t BIG_EXTRA_SIZE = 100;
        unsigned char buffer[100] = { 0 };
        
        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extraSize);
        }
    
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        Plugin::Status ret = muxerDemo->FFmpegAddTrack(trackIndex, audioParams);

        return ret;
    }

    Plugin::Status addAudioTrackAACByFd(AVMuxerDemo* muxerDemo,  int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        constexpr uint32_t BIG_EXTRA_SIZE = 100;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);

        Plugin::Status ret = muxerDemo->FFmpegAddTrack(trackIndex, audioParams);

        return ret;
    }

    Plugin::Status addVideoTrackByFd(AVMuxerDemo* muxerDemo,  int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription videoParams;

        int extraSize = 0;
        constexpr uint32_t BIG_EXTRA_SIZE = 100;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= BIG_EXTRA_SIZE && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t *)buffer, extraSize);
        }
        constexpr uint32_t WIDTH = 352;
        constexpr uint32_t HEIGHT = 288;
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, WIDTH);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, HEIGHT);

        Plugin::Status ret = muxerDemo->FFmpegAddTrack(trackIndex, videoParams);

        return ret;
    }

    void WriteTrackSampleByFd(AVMuxerDemo* muxerDemo, int audioTrackIndex, int videoTrackIndex, int32_t inputFile)
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
                cout << "read info.pts error, ret is: " << (int)ret << endl;
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
                trackIndex = audioTrackIndex;
            } else if (dataTrackId == DATA_VIDEO_ID) {
                trackIndex = videoTrackIndex;
            } else {
                cout << "error dataTrackId : " << trackId << endl;
            }
            if (trackId >= 0) {
                Plugin::Status result = muxerDemo->FFmpegWriteSample(trackIndex, avMuxerDemoBuffer, info, flag);
                if (result != Plugin::Status::NO_ERROR) {
                    cout << "OH_AVMuxer_WriteSampleBuffer error! ret is: " << (int)result << endl;
                    break;
                }
            }
        }
        if (avMuxerDemoBuffer != nullptr) {
            free(avMuxerDemoBuffer);
        }
    }
    void WriteTrackCover(AVMuxerDemo* muxerDemo, int coverTrackIndex, int32_t fdInput)
    {
        printf("WriteTrackCover\n");
        AVCodecBufferInfo info;
        uint32_t trackIndex;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
        memset_s(&info, sizeof(info), 0, sizeof(info));
        struct stat fileStat;
        fstat(fdInput, &fileStat);
        info.size = fileStat.st_size;
        unsigned char* avMuxerDemoBuffer = (unsigned char*)malloc(info.size);
        if (avMuxerDemoBuffer == nullptr) {
            printf("malloc memory error! size: %d \n", info.size);
            return;
        }

        int ret = read(fdInput, avMuxerDemoBuffer, info.size);
        if (ret <= 0) {
            free(avMuxerDemoBuffer);
            return;
        }
        trackIndex = coverTrackIndex;
        Plugin::Status result = muxerDemo->FFmpegWriteSample(trackIndex, avMuxerDemoBuffer, info, flag);
        if (result != Plugin::Status::NO_ERROR) {
            free(avMuxerDemoBuffer);
            cout << "WriteTrackCover error!  " << endl;
            return;
        }
        free(avMuxerDemoBuffer);
    }

    void WriteSingleTrackSample(AVMuxerDemo* muxerDemo, int trackId, int fd)
    {
        int ret = 0;
        int dataSize = 0;
        int flags = 0;
        unsigned char* avMuxerDemoBuffer = nullptr;
        int avMuxerDemoBufferSize = 0;
        AVCodecBufferInfo info;
        uint32_t trackIndex;
        AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
        memset_s(&info, sizeof(info), 0, sizeof(info));
        while (1) {
            ret = read(fd, (void*)&info.presentationTimeUs, sizeof(info.presentationTimeUs));
            if (ret <= 0) {
                break;
            }

            ret = read(fd, (void*)&flags, sizeof(flags));
            if (ret <= 0) {
                break;
            }

            // read frame buffer
            ret = read(fd, (void*)&dataSize, sizeof(dataSize));
            if (ret <= 0 || dataSize < 0) {
                break;
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
                    printf("error malloc memory! %d\n", dataSize);
                    break;
                }
            }
            ret = read(fd, (void*)avMuxerDemoBuffer, dataSize);
            if (ret <= 0) {
                break;
            }
            info.size = dataSize;

            if (flag != 0) {
                flag = AVCODEC_BUFFER_FLAG_SYNC_FRAME;
            }
            trackIndex = trackId;
            Plugin::Status result = muxerDemo->FFmpegWriteSample(trackIndex, avMuxerDemoBuffer, info, flag);
            if (result != Plugin::Status::NO_ERROR) {
                cout << "WriteSingleTrackSample error!  "  << endl;
                break;
            }
        }

        if (avMuxerDemoBuffer != nullptr) {
            free(avMuxerDemoBuffer);
        }
    }

    void runMuxer(string testcaseName, int threadId, OutputFormat format)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        string fileName = testcaseName + "_" + to_string(threadId);

        cout << "thread id is: " << threadId << ", cur file name is: " << fileName << endl;
        int32_t fd = muxerDemo->FFmpeggetFdByName(format, fileName);

        int32_t inputFile;
        int32_t audioTrackId;
        int32_t videoTrackId;

        cout << "thread id is: " << threadId << ", fd is: " << fd << endl;
        muxerDemo->FFmpegCreate(fd);

        Plugin::Status ret;

        if (format == OUTPUT_FORMAT_MPEG_4) {
            cout << "thread id is: " << threadId << ", format is: " << format << endl;
            inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);
            addAudioTrackByFd(muxerDemo, inputFile, audioTrackId);
            addVideoTrackByFd(muxerDemo, inputFile, videoTrackId);
        } else {
            cout << "thread id is: " << threadId << ", format is: " << format << endl;
            inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);
            addAudioTrackAACByFd(muxerDemo, inputFile, audioTrackId);
            addVideoTrackByFd(muxerDemo, inputFile, videoTrackId);
        }

        cout << "thread id is: " << threadId  << ", audio track id is: " << audioTrackId <<
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
        delete muxerDemo;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001
 * @tc.name      : audio
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001, TestSize.Level2)
{
    OutputFormat formatList[] = { OUTPUT_FORMAT_M4A, OUTPUT_FORMAT_MPEG_4 };
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->FFmpeggetFdByName(format, "FUNCTION_001");

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        muxerDemo->FFmpegCreate(fd);

        int32_t audioTrackId;
        addAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);

        int32_t videoTrackId = -1;
        removeHeader();

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        Plugin::Status ret;

        ret = muxerDemo->FFmpegStart();
        cout << "Start ret is:" << (int)ret << endl;

        if (audioTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        }

        ret = muxerDemo->FFmpegStop();
        cout << "Stop ret is:" << (int)ret << endl;

        ret = muxerDemo->FFmpegDestroy();
        cout << "Destroy ret is:" << (int)ret << endl;

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
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_002, TestSize.Level2)
{
    OutputFormat formatList[] = {OUTPUT_FORMAT_M4A, OUTPUT_FORMAT_MPEG_4};
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->FFmpeggetFdByName(format, "FUNCTION_002");

        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);

        muxerDemo->FFmpegCreate(fd);

        int32_t audioTrackId = -1;
        int32_t videoTrackId;
        addVideoTrackByFd(muxerDemo, videoFileFd, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        Plugin::Status ret;


        ret = muxerDemo->FFmpegStart();
        cout << "Start ret is:" << (int)ret << endl;

        if (videoTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->FFmpegStop();
        cout << "Stop ret is:" << (int)ret << endl;

        ret = muxerDemo->FFmpegDestroy();
        cout << "Destroy ret is:" << (int)ret << endl;

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
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_003, TestSize.Level2)
{
    OutputFormat formatList[] = {OUTPUT_FORMAT_M4A, OUTPUT_FORMAT_MPEG_4};
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->FFmpeggetFdByName(format, "FUNCTION_003");

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);

        muxerDemo->FFmpegCreate(fd);

        int32_t audioTrackId;
        addAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        int32_t videoTrackId;
        addVideoTrackByFd(muxerDemo, videoFileFd, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        Plugin::Status ret;

        ret = muxerDemo->FFmpegStart();
        cout << "Start ret is:" << (int)ret << endl;

        if (audioTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        }
        if (videoTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->FFmpegStop();
        cout << "Stop ret is:" << (int)ret << endl;

        ret = muxerDemo->FFmpegDestroy();
        cout << "Destroy ret is:" << (int)ret << endl;

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
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "FUNCTION_004");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    muxerDemo->FFmpegCreate(fd);

    int32_t audioTrackId;
    addAudioTrack(muxerDemo, audioTrackId);
    int32_t videoTrackId;
    addVideoTrack(muxerDemo, videoTrackId);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    Plugin::Status ret;
    ret = muxerDemo->FFmpegSetRotation(90);
    cout << "FFmpegSetRotation ret is:" << (int)ret << endl;

    ret = muxerDemo->FFmpegStart();
    cout << "Start ret is:" << (int)ret << endl;

    WriteTrackSample(muxerDemo, audioTrackId, videoTrackId);

    ret = muxerDemo->FFmpegStop();
    cout << "Stop ret is:" << (int)ret << endl;

    ret = muxerDemo->FFmpegDestroy();
    cout << "Destroy ret is:" << (int)ret << endl;

    close(g_inputFile);
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_005
 * @tc.name      : mp4(video audio length not equal)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "FUNCTION_005");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    muxerDemo->FFmpegCreate(fd);
    Plugin::Status ret;

    int32_t audioTrackId;
    addAudioTrack(muxerDemo, audioTrackId);
    int32_t videoTrackId;
    addVideoTrack(muxerDemo, videoTrackId);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    ret = muxerDemo->FFmpegStart();
    cout << "Start ret is:" << (int)ret << endl;

    WriteTrackSampleShort(muxerDemo, audioTrackId, videoTrackId, 100);

    ret = muxerDemo->FFmpegStop();
    cout << "Stop ret is:" << (int)ret << endl;

    ret = muxerDemo->FFmpegDestroy();
    cout << "Destroy ret is:" << (int)ret << endl;

    close(g_inputFile);
    close(fd);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_006
 * @tc.name      : m4a(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_006, TestSize.Level2)
{
    vector<thread> threadVec;
    OutputFormat format = OUTPUT_FORMAT_M4A;
    for (int i = 0; i < 10; i++) {
        threadVec.push_back(thread(runMuxer, "FUNCTION_006", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++) {
        threadVec[i].join();
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_007
 * @tc.name      : mp4(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_007, TestSize.Level2)
{
    vector<thread> threadVec;
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    for (int i = 0; i < 10; i++) {
        threadVec.push_back(thread(runMuxer, "SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_007", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++) {
        threadVec[i].join();
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008
 * @tc.name      : m4a(multi audio track)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "FUNCTION_008");

    int32_t audioFileFd1 = open("aac_44100_2.bin", O_RDONLY);
    int32_t audioFileFd2 = open("aac_44100_2.bin", O_RDONLY);

    muxerDemo->FFmpegCreate(fd);

    int32_t audioTrackId1;
    int32_t audioTrackId2;
    addAudioTrackAACByFd(muxerDemo, audioFileFd1, audioTrackId1);
    addAudioTrackAACByFd(muxerDemo, audioFileFd2, audioTrackId2);

    cout << "audiotrack id1 is: " << audioTrackId1 << ", audioTrackId2 is: " << audioTrackId2 << endl;

    Plugin::Status ret;

    ret = muxerDemo->FFmpegStart();
    cout << "Start ret is:" << (int)ret << endl;

    if (audioTrackId1 >= 0) {
        WriteSingleTrackSample(muxerDemo, audioTrackId1, audioFileFd1);
    }
    if (audioTrackId2 >= 0) {
        WriteSingleTrackSample(muxerDemo, audioTrackId2, audioFileFd2);
    }

    ret = muxerDemo->FFmpegStop();
    cout << "Stop ret is:" << (int)ret << endl;

    ret = muxerDemo->FFmpegDestroy();
    cout << "Destroy ret is:" << (int)ret << endl;

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
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->FFmpeggetFdByName(format, "FUNCTION_009");

    int32_t videoFileFd1 = open("h264_640_360.bin", O_RDONLY);
    int32_t videoFileFd2 = open("h264_640_360.bin", O_RDONLY);


    muxerDemo->FFmpegCreate(fd);

    int32_t videoTrackId1;
    int32_t videoTrackId2;
    addVideoTrackH264ByFd(muxerDemo, videoFileFd1, videoTrackId1);
    addVideoTrackH264ByFd(muxerDemo, videoFileFd2, videoTrackId2);

    Plugin::Status ret;

    ret = muxerDemo->FFmpegStart();
    cout << "Start ret is:" << (int)ret << endl;

    if (videoTrackId1 >= 0) {
        WriteSingleTrackSample(muxerDemo, videoTrackId1, videoFileFd1);
    }
    if (videoTrackId2 >= 0) {
        WriteSingleTrackSample(muxerDemo, videoTrackId2, videoFileFd2);
    }

    ret = muxerDemo->FFmpegStop();
    cout << "Stop ret is:" << (int)ret << endl;

    ret = muxerDemo->FFmpegDestroy();
    cout << "Destroy ret is:" << (int)ret << endl;

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
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_010, TestSize.Level2)
{
    string coverTypeList[] = { "bmp", "jpg", "png" };
    for (int i = 0; i < 3; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        string outputFile = "FUNCTION_010_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];
        
        OutputFormat format = OUTPUT_FORMAT_M4A;
        int32_t fd = muxerDemo->FFmpeggetFdByMode(format);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        muxerDemo->FFmpegCreate(fd);
        
        int32_t audioTrackId;
        int32_t videoTrackId;
        int32_t coverTrackId = 1;

        addAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        addVideoTrackH264ByFd(muxerDemo, videoFileFd, videoTrackId);
        addCoverTrack(muxerDemo, coverTypeList[i], coverTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId<<
            ", cover track id is: " << coverTrackId << endl;

        Plugin::Status ret;

        ret = muxerDemo->FFmpegStart();
        cout << "Start ret is:" << (int)ret << endl;

        WriteTrackCover(muxerDemo, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);

        ret = muxerDemo->FFmpegStop();
        cout << "Stop ret is:" << (int)ret << endl;

        ret = muxerDemo->FFmpegDestroy();
        cout << "Destroy ret is:" << (int)ret << endl;

        close(audioFileFd);
        close(videoFileFd);
        close(coverFileFd);
        close(fd);
        delete muxerDemo;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_011
 * @tc.name      : mp4(auido video with cover)
 * @tc.desc      : Function test
 */
HWTEST_F(FFmpegAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_011, TestSize.Level2)
{
    string coverTypeList[] = { "bmp", "jpg", "png" };
    for (int i = 0; i < 3; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        string outputFile = "FUNCTION_011_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];

        OutputFormat format = OUTPUT_FORMAT_MPEG_4;
        int32_t fd = muxerDemo->FFmpeggetFdByMode(format);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        muxerDemo->FFmpegCreate(fd);
        int32_t audioTrackId;
        addAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        int32_t videoTrackId;
        addVideoTrackByFd(muxerDemo, videoFileFd, videoTrackId);
        int32_t coverTrackId = 1;
        addCoverTrack(muxerDemo, coverTypeList[i], coverTrackId);

        cout << "audio track id is: " << audioTrackId << ", cover track id is: " << coverTrackId << endl;

        Plugin::Status ret;

        ret = muxerDemo->FFmpegStart();
        cout << "Start ret is:" << (int)ret << endl;

        WriteTrackCover(muxerDemo, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);

        ret = muxerDemo->FFmpegStop();
        cout << "Stop ret is:" << (int)ret << endl;

        ret = muxerDemo->FFmpegDestroy();
        cout << "Destroy ret is:" << (int)ret << endl;

        close(audioFileFd);
        close(videoFileFd);
        close(coverFileFd);
        close(fd);
        delete muxerDemo;
    }
}
