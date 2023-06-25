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
#include "AVMuxerDemoCommon.h"
#include "fcntl.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
constexpr uint32_t SAMPLE_RATE_352 = 352;
constexpr uint32_t SAMPLE_RATE_288 = 288;
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE_44100 = 44100;
constexpr uint32_t extraSize_num = 100;

namespace {
    class InnerAVMuxerFunctionTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void InnerAVMuxerFunctionTest::SetUpTestCase() {}
    void InnerAVMuxerFunctionTest::TearDownTestCase() {}
    void InnerAVMuxerFunctionTest::SetUp() {}
    void InnerAVMuxerFunctionTest::TearDown() {}

    static int g_inputFile = -1;
    static const int DATA_AUDIO_ID = 0;
    static const int DATA_VIDEO_ID = 1;

    int32_t addAudioTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);

        return ret;
    }

    int32_t addAudioTrackAAC(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);

        return ret;
    }


    int32_t addVideoTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription videoParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
            read(g_inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, SAMPLE_RATE_352);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, SAMPLE_RATE_288);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, videoParams);

        return ret;
    }


    int32_t addCoverTrack(AVMuxerDemo* muxerDemo, string coverType, int32_t trackIndex)
    {
        MediaDescription coverFormat;

        if (coverType == "jpg") {
            coverFormat.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_JPG);
        } else if (coverType == "png") {
            coverFormat.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_PNG);
        } else {
            coverFormat.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::IMAGE_BMP);
        }
        coverFormat.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, SAMPLE_RATE_352);
        coverFormat.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, SAMPLE_RATE_288);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, coverFormat);
        return ret;
    }

    void removeHeader()
    {
        int extraSize = 0;
        unsigned char buffer[100] = { 0 };
        read(g_inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
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
                cout << "read dataTrackId error, ret is: " << ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)&info.presentationTimeUs, sizeof(info.presentationTimeUs));
            if (ret <= 0) {
                cout << "read info.presentationTimeUs error, ret is: " << ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)&dataSize, sizeof(dataSize));
            if (ret <= 0) {
                cout << "read dataSize error, ret is: " << ret << endl;
                return;
            }
            ret = read(g_inputFile, (void*)data, dataSize);
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
                trackIndex = trackId;
                std::shared_ptr<AVSharedMemoryBase> avMemBuffer =
                std::make_shared<AVSharedMemoryBase>(info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");

                avMemBuffer->Init();
                (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
                int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
                if (result != AVCS_ERR_OK) {
                    cout << "int32_t_WriteSample error! ret is: " << result << endl;
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
                trackIndex = trackId;
                std::shared_ptr<AVSharedMemoryBase> avMemBuffer =
                std::make_shared<AVSharedMemoryBase>(info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
                avMemBuffer->Init();
                (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
                int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
                if (result != AVCS_ERR_OK) {
                    printf("    WriteSample error!");
                    return;
                }
            }
        }
    }

    
    int32_t addAudioTrackByFd(AVMuxerDemo* muxerDemo, int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_MPEG);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);

        return ret;
    }

    int32_t addAudioTrackAACByFd(AVMuxerDemo* muxerDemo, int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription audioParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE_44100);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);

        return ret;
    }

    int32_t addVideoTrackByFd(AVMuxerDemo* muxerDemo, int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription videoParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_MPEG4);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, SAMPLE_RATE_352);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, SAMPLE_RATE_288);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, videoParams);

        return ret;
    }
    int32_t addVideoTrackH264ByFd(AVMuxerDemo* muxerDemo, int32_t inputFile, int32_t& trackIndex)
    {
        MediaDescription videoParams;

        int extraSize = 0;
        unsigned char buffer[100] = { 0 };

        read(inputFile, (void*)&extraSize, sizeof(extraSize));
        if (extraSize <= extraSize_num && extraSize > 0) {
            read(inputFile, buffer, extraSize);
            videoParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, (uint8_t*)buffer, extraSize);
        }
        videoParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::VIDEO_AVC);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, SAMPLE_RATE_352);
        videoParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, SAMPLE_RATE_288);

        int32_t ret = muxerDemo->InnerAddTrack(trackIndex, videoParams);

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
                cout << "read dataTrackId error, ret is: " << ret << endl;
                return;
            }
            ret = read(inputFile, (void*)&info.presentationTimeUs, sizeof(info.presentationTimeUs));
            if (ret <= 0) {
                cout << "read info.presentationTimeUs error, ret is: " << ret << endl;
                return;
            }
            ret = read(inputFile, (void*)&dataSize, sizeof(dataSize));
            if (ret <= 0) {
                cout << "read dataSize error, ret is: " << ret << endl;
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
            resultStr = "inputFile is: " + to_string(inputFile) + ", avMuxerDemoBufferSize is "
             + to_string(avMuxerDemoBufferSize);
            cout << resultStr << endl;

            ret = read(inputFile, (void*)avMuxerDemoBuffer, dataSize);
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
                trackIndex = trackId;
                std::shared_ptr<AVSharedMemoryBase> avMemBuffer =
                std::make_shared<AVSharedMemoryBase>(info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
                avMemBuffer->Init();
                (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), avMuxerDemoBuffer, info.size);
                int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
                if (result != 0) {
                    cout << "    WriteSampleBuffer error! ret is: " << result << endl;
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
        string fileName = testcaseName + "_" + to_string(threadId);

        cout << "thread id is: " << threadId << ", cur file name is: " << fileName << endl;
        int32_t fd = muxerDemo->InnergetFdByName(format, fileName);

        int32_t inputFile;
        int32_t audioTrackId;
        int32_t videoTrackId;

        cout << "thread id is: " << threadId << ", fd is: " << fd << endl;
        muxerDemo->InnerCreate(fd, format);
        int32_t ret;

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
            if (extraSize <= extraSize_num && extraSize > 0) {
                read(inputFile, buffer, extraSize);
            }
        }

        cout << "thread id is: " << threadId << ", audio track id is: "
         << audioTrackId << ", video track id is: " << videoTrackId << endl;

        ret = muxerDemo->InnerStart();
        cout << "thread id is: " << threadId << ", Start ret is:" << ret << endl;

        WriteTrackSampleByFd(muxerDemo, audioTrackId, videoTrackId, inputFile);

        ret = muxerDemo->InnerStop();
        cout << "thread id is: " << threadId << ", Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
        cout << "thread id is: " << threadId << ", Destroy ret is:" << ret << endl;

        close(inputFile);
        close(fd);
        delete muxerDemo;
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
            std::shared_ptr<AVSharedMemoryBase> avMemBuffer = std::make_shared
            <AVSharedMemoryBase>(info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
            avMemBuffer->Init();
            auto ret = memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), avMuxerDemoBuffer, info.size);
            if (ret != EOK) {
                printf("WriteSingleTrackSample memcpy_s failed, ret:%d\n", ret);
                break;
            }
            int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
            if (result != 0) {
                cout << "WriteSingleTrackSample error! ret is: " << result << endl;
                break;
            }
        }

        if (avMuxerDemoBuffer != nullptr) {
            free(avMuxerDemoBuffer);
        }
    }

    void WriteTrackCover(AVMuxerDemo* muxerDemo, int coverTrackIndex, int fdInput)
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
        std::shared_ptr<AVSharedMemoryBase> avMemBuffer = std::make_shared<AVSharedMemoryBase>
        (info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
        avMemBuffer->Init();
        auto ret = memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), avMuxerDemoBuffer, info.size);
        if (ret != EOK) {
            printf("WriteTrackCover memcpy_s failed, ret:%d\n", ret);
            return;
        }
        int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
        if (result != 0) {
            free(avMuxerDemoBuffer);
            cout << "WriteTrackCover error! ret is: " << result << endl;
            return;
        }
        free(avMuxerDemoBuffer);
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001
 * @tc.name      : audio
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001, TestSize.Level2)
{
    OutputFormat formatList[] = { OUTPUT_FORMAT_M4A, OUTPUT_FORMAT_MPEG_4 };
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->InnergetFdByName(format, "FUNCTION_001_INNER_" + to_string(i));

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        addAudioTrackAAC(muxerDemo, audioTrackId);

        int32_t videoTrackId = -1;
        removeHeader();

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;
        audioTrackId = 0;

        if (audioTrackId >= 0)
        {
            WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        }

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_002, TestSize.Level2)
{
    OutputFormat formatList[] = {OUTPUT_FORMAT_M4A, OUTPUT_FORMAT_MPEG_4};
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->InnergetFdByName(format, "FUNCTION_002_INNER_" + to_string(i));

        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId = -1;
        removeHeader();
        int32_t videoTrackId;
        addVideoTrack(muxerDemo, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;

        if (videoTrackId >= 0)
        {
            WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_003, TestSize.Level2)
{
    OutputFormat formatList[] = {OUTPUT_FORMAT_M4A, OUTPUT_FORMAT_MPEG_4};
    for (int i = 0; i < 2; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();

        OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->InnergetFdByName(format, "FUNCTION_003_INNER_" + to_string(i));

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        addAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        int32_t videoTrackId;
        addVideoTrackByFd(muxerDemo, videoFileFd, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;

        if (audioTrackId >= 0)
        {
            WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        }
        if (videoTrackId >= 0)
        {
            WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByName(format, "FUNCTION_004_INNER");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);

    int32_t audioTrackId;
    addAudioTrack(muxerDemo, audioTrackId);
    int32_t videoTrackId;
    addVideoTrack(muxerDemo, videoTrackId);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    int32_t ret;
    ret = muxerDemo->InnerSetRotation(90);
    cout << "SetRotation ret is:" << ret << endl;

    ret = muxerDemo->InnerStart();
    cout << "Start ret is:" << ret << endl;

    WriteTrackSample(muxerDemo, audioTrackId, videoTrackId);

    ret = muxerDemo->InnerStop();
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByName(format, "FUNCTION_005_INNER");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);
    int32_t ret;

    int32_t audioTrackId;
    addAudioTrack(muxerDemo, audioTrackId);
    int32_t videoTrackId;
    addVideoTrack(muxerDemo, videoTrackId);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    ret = muxerDemo->InnerStart();
    cout << "Start ret is:" << ret << endl;

    WriteTrackSampleShort(muxerDemo, audioTrackId, videoTrackId, 100);

    ret = muxerDemo->InnerStop();
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_006, TestSize.Level2)
{
    vector<thread> threadVec;
    OutputFormat format = OUTPUT_FORMAT_M4A;
    for (int i = 0; i < 16; i++)
    {
        threadVec.push_back(thread(runMuxer, "FUNCTION_006_INNER", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_007
 * @tc.name      : mp4(thread)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_007, TestSize.Level2)
{
    vector<thread> threadVec;
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    for (int i = 0; i < 16; i++)
    {
        threadVec.push_back(thread(runMuxer, "FUNCTION_007_INNER", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++)
    {
        threadVec[i].join();
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008
 * @tc.name      : m4a(multi audio track)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = muxerDemo->InnergetFdByName(format, "FUNCTION_008_INNER");

    int32_t audioFileFd1 = open("aac_44100_2.bin", O_RDONLY);
    int32_t audioFileFd2 = open("aac_44100_2.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);

    int32_t audioTrackId1;
    int32_t audioTrackId2;
    addAudioTrackAACByFd(muxerDemo, audioFileFd1, audioTrackId1);
    addAudioTrackAACByFd(muxerDemo, audioFileFd2, audioTrackId2);
    removeHeader();

    cout << "audiotrack id1 is: " << audioTrackId1 << ", audioTrackId2 is: " << audioTrackId2 << endl;

    int32_t ret;

    ret = muxerDemo->InnerStart();
    cout << "Start ret is:" << ret << endl;

    if (audioTrackId1 >= 0)
    {
        WriteSingleTrackSample(muxerDemo, audioTrackId1, audioFileFd1);
    }
    if (audioTrackId2 >= 0)
    {
        WriteSingleTrackSample(muxerDemo, audioTrackId2, audioFileFd2);
    }

    ret = muxerDemo->InnerStop();
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = muxerDemo->InnergetFdByName(format, "FUNCTION_009_INNER");

    int32_t videoFileFd1 = open("h264_640_360.bin", O_RDONLY);
    int32_t videoFileFd2 = open("h264_640_360.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);


    int32_t videoTrackId1;
    int32_t videoTrackId2;
    addVideoTrackH264ByFd(muxerDemo, videoFileFd1, videoTrackId1);
    addVideoTrackH264ByFd(muxerDemo, videoFileFd2, videoTrackId2);

    int32_t ret;

    ret = muxerDemo->InnerStart();
    cout << "Start ret is:" << ret << endl;

    if (videoTrackId1 >= 0)
    {
        WriteSingleTrackSample(muxerDemo, videoTrackId1, videoFileFd1);
    }
    if (videoTrackId2 >= 0)
    {
        WriteSingleTrackSample(muxerDemo, videoTrackId2, videoFileFd2);
    }

    ret = muxerDemo->InnerStop();
    cout << "Stop ret is:" << ret << endl;

    ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_010, TestSize.Level2)
{
    string coverTypeList[] = { "bmp", "jpg", "png" };
    for (int i = 0; i < 3; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        string outputFile = "SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_010_INNER_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];

        OutputFormat format = OUTPUT_FORMAT_M4A;
        int32_t fd = muxerDemo->InnergetFdByName(format, outputFile);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        int32_t videoTrackId;
        int32_t coverTrackId = 1;

        addAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        addVideoTrackH264ByFd(muxerDemo, videoFileFd, videoTrackId);
        addCoverTrack(muxerDemo, coverTypeList[i], coverTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: "
         << videoTrackId<< ", cover track id is: " << coverTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;

        WriteTrackCover(muxerDemo, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
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
 * @tc.name      : mp4(auido video with cover)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_011, TestSize.Level2)
{
    string coverTypeList[] = { "bmp", "jpg", "png" };
    for (int i = 0; i < 3; i++)
    {
        AVMuxerDemo* muxerDemo = new AVMuxerDemo();
        string outputFile = "FUNCTION_011_INNER_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];

        OutputFormat format = OUTPUT_FORMAT_MPEG_4;
        int32_t fd = muxerDemo->InnergetFdByName(format, outputFile);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        int32_t videoTrackId;
        int32_t coverTrackId = 1;

        addAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        addVideoTrackByFd(muxerDemo, videoFileFd, videoTrackId);
        addCoverTrack(muxerDemo, coverTypeList[i], coverTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: "
         << videoTrackId << ", cover track id is: " << coverTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;

        WriteTrackCover(muxerDemo, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
        cout << "Destroy ret is:" << ret << endl;

        close(audioFileFd);
        close(videoFileFd);
        close(coverFileFd);
        close(fd);
        delete muxerDemo;
    }
}
