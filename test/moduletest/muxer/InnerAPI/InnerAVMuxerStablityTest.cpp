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
#include "avcodec_errors.h"
#include "securec.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
constexpr int32_t SAMPLE_RATE_44100 = 44100;
constexpr int32_t CHANNEL_COUNT = 2;
constexpr int32_t BUFFER_SIZE = 100;
constexpr int32_t SAMPLE_RATE_352 = 352;
constexpr int32_t SAMPLE_RATE_288 = 288;
constexpr int32_t BUFFER_SIZE_NUM = 1024 * 1024;

namespace {
class InnerAVMuxerStablityTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
};

void InnerAVMuxerStablityTest::SetUpTestCase() {}
void InnerAVMuxerStablityTest::TearDownTestCase() {}
void InnerAVMuxerStablityTest::SetUp() {}
void InnerAVMuxerStablityTest::TearDown() {}

static int g_inputFile = -1;
static const int DATA_AUDIO_ID = 0;
static const int DATA_VIDEO_ID = 1;

constexpr int RUN_TIMES = 1000;
constexpr int RUN_TIME = 8 * 3600;

int32_t g_testResult[10] = { -1 };

int32_t SetRotation(AVMuxerDemo *muxerDemo)
{
    int32_t rotation = 0;
    int32_t ret = muxerDemo->InnerSetRotation(rotation);
    return ret;
}

int32_t AddTrack(AVMuxerDemo *muxerDemo, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    int extraSize = 0;
    read(g_inputFile, static_cast<void *>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(g_inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);

    int32_t trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    return trackId;
}

int32_t WriteSample(AVMuxerDemo *muxerDemo)
{
    uint32_t trackIndex = 0;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    avMemBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::NONE);

    int32_t ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);

    return ret;
}

int32_t AddAudioTrack(AVMuxerDemo *muxerDemo, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    int extraSize = 0;
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(g_inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_MPEG);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);

    int32_t trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    return trackId;
}

int32_t AddAudioTrackAAC(AVMuxerDemo *muxerDemo, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    int extraSize = 0;
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(g_inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);
    audioParams->Set<Tag::MEDIA_PROFILE>(AAC_PROFILE_LC);

    int32_t trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    return trackId;
}

int32_t AddVideoTrack(AVMuxerDemo *muxerDemo, int32_t &trackIndex)
{
    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    int extraSize = 0;
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(g_inputFile, buffer.data(), extraSize);
        videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
    videoParams->Set<Tag::VIDEO_WIDTH>(SAMPLE_RATE_352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(SAMPLE_RATE_288);

    int32_t trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    return trackId;
}

void RemoveHeader()
{
    int extraSize = 0;
    unsigned char buffer[100] = {0};
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        read(g_inputFile, buffer, extraSize);
    }
}

void WriteTrackSample(AVMuxerDemo *muxerDemo, int audioTrackIndex, int videoTrackIndex)
{
    int dataTrackId = 0;
    int dataSize = 0;
    int ret = 0;
    int trackId = 0;
    uint32_t trackIndex;

    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(alloc, BUFFER_SIZE_NUM);
    do {
        ret = read(g_inputFile, static_cast<void*>(&dataTrackId), sizeof(dataTrackId));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(&avMemBuffer->pts_), sizeof(avMemBuffer->pts_));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(&dataSize), sizeof(dataSize));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(avMemBuffer->memory_->GetAddr()), dataSize);
        if (ret <= 0) {
            return;
        }

        avMemBuffer->memory_->SetSize(dataSize);
        if (dataTrackId == DATA_AUDIO_ID) {
            trackId = audioTrackIndex;
        } else if (dataTrackId == DATA_VIDEO_ID) {
            trackId = videoTrackIndex;
        } else {
            cout << "error dataTrackId : " << trackId << endl;
        }
        if (trackId >= 0) {
            trackIndex = trackId;
            int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
            if (result != AVCS_ERR_OK) {
                return;
            }
        }
    } while (ret > 0)
}

int32_t AddAudioTrackByFd(AVMuxerDemo *muxerDemo, int32_t inputFile, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    int extraSize = 0;
    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_MPEG);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);

    int32_t trackId = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    return trackId;
}

int32_t AddAudioTrackAACByFd(AVMuxerDemo *muxerDemo, int32_t inputFile, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
    int extraSize = 0;
    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);
    return ret;
}

int32_t AddVideoTrackByFd(AVMuxerDemo *muxerDemo, int32_t inputFile, int32_t &trackIndex)
{
    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();
    int extraSize = 0;
    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= BUFFER_SIZE && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(inputFile, buffer.data(), extraSize);
        videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
    videoParams->Set<Tag::VIDEO_WIDTH>(SAMPLE_RATE_352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(SAMPLE_RATE_288);

    int32_t trackId = muxerDemo->InnerAddTrack(trackIndex, videoParams);
    return trackId;
}

int WriteTrackSampleByFdRead(int *inputFile, int64_t *pts, int *dataSize, int *dataTrackId)
{
    int ret = read(*inputFile, static_cast<void*>(dataTrackId), sizeof(*dataTrackId));
    if (ret <= 0) {
        cout << "read dataTrackId error, ret is: " << ret << endl;
        return -1;
    }
    ret = read(*inputFile, static_cast<void*>(pts), sizeof(*pts));
    if (ret <= 0) {
        cout << "read info.presentationTimeUs error, ret is: " << ret << endl;
        return -1;
    }
    ret = read(*inputFile, static_cast<void*>(dataSize), sizeof(*dataSize));
    if (ret <= 0) {
        cout << "read dataSize error, ret is: " << ret << endl;
        return -1;
    }
    return 0;
}

int WriteTrackSampleByFdMem(int dataSize, std::shared_ptr<AVBuffer> &avMuxerDemoBuffer)
{
    if (avMuxerDemoBuffer != nullptr && dataSize > avMuxerDemoBuffer->memory_->GetCapacity()) {
        avMuxerDemoBuffer = nullptr;
    }
    if (avMuxerDemoBuffer == nullptr) {
        auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_ONLY);
        avMuxerDemoBuffer = AVBuffer::CreateAVBuffer(alloc, dataSize);
        if (avMuxerDemoBuffer == nullptr) {
            printf("error malloc memory!\n");
            return -1;
        }
    }
    return 0;
}

int WriteTrackSampleByFdGetIndex(const int *dataTrackId, const int *audioTrackIndex,
                                 int *videoTrackIndex)
{
    int trackId = 0;
    if (*dataTrackId == DATA_AUDIO_ID) {
        trackId = *audioTrackIndex;
    } else if (*dataTrackId == DATA_VIDEO_ID) {
        trackId = *videoTrackIndex;
    } else {
        cout << "error dataTrackId : " << *dataTrackId << endl;
    }

    return trackId;
}

void WriteTrackSampleByFd(AVMuxerDemo *muxerDemo, int audioTrackIndex, int videoTrackIndex, int32_t inputFile)
{
    int dataTrackId = 0;
    int dataSize = 0;
    int trackId = 0;
    int64_t pts = 0;
    uint32_t trackIndex;
    std::shared_ptr<AVBuffer> avMuxerDemoBuffer = nullptr;
    string resultStr = "";
    do {
        int ret = WriteTrackSampleByFdRead(&inputFile, &pts, &dataSize, &dataTrackId);
        if (ret != 0) {
            return;
        }

        ret = WriteTrackSampleByFdMem(dataSize, avMuxerDemoBuffer);
        if (ret != 0) {
            break;
        }

        resultStr =
            "inputFile is: " + to_string(inputFile) + ", avMuxerDemoBufferSize is " + to_string(dataSize);
        cout << resultStr << endl;

        ret = read(inputFile, static_cast<void*>(avMuxerDemoBuffer->memory_->GetAddr()), dataSize);
        if (ret <= 0) {
            cout << "read data error, ret is: " << ret << endl;
            continue;
        }
        avMuxerDemoBuffer->pts_ = pts;
        avMuxerDemoBuffer->memory_->SetSize(dataSize);
        trackId = WriteTrackSampleByFdGetIndex(&dataTrackId, &audioTrackIndex, &videoTrackIndex);
        if (trackId >= 0) {
            trackIndex = trackId;
            int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMuxerDemoBuffer);
            if (result != AVCS_ERR_OK) {
                cout << "InnerWriteSample error! ret is: " << result << endl;
                break;
            }
        }
    } while (ret >0)
}

void RunMuxer(string testcaseName, int threadId, Plugins::OutputFormat format)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME) {
        cout << "thread id is: " << threadId << ", run time : " << difftime(curTime, startTime) << " seconds" << endl;
        string fileName = testcaseName + "_" + to_string(threadId);

        cout << "thread id is: " << threadId << ", cur file name is: " << fileName << endl;
        int32_t fd = muxerDemo->InnerGetFdByName(format, fileName);

        int32_t inputFile;
        int32_t audioTrackId;
        int32_t videoTrackId;

        cout << "thread id is: " << threadId << ", fd is: " << fd << endl;
        muxerDemo->InnerCreate(fd, format);

        int32_t ret;

        if (format == Plugins::OutputFormat::MPEG_4) {
            cout << "thread id is: " << threadId << ", format is: " << static_cast<int32_t>(format) << endl;
            inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);
            AddAudioTrackByFd(muxerDemo, inputFile, audioTrackId);
            AddVideoTrackByFd(muxerDemo, inputFile, videoTrackId);
        } else {
            cout << "thread id is: " << threadId << ", format is: " << static_cast<int32_t>(format) << endl;
            inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);
            AddAudioTrackAACByFd(muxerDemo, inputFile, audioTrackId);
            videoTrackId = -1;
            int extraSize = 0;
            unsigned char buffer[100] = {0};
            read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
            if (extraSize <= BUFFER_SIZE && extraSize > 0) {
                read(inputFile, buffer, extraSize);
            }
        }

        cout << "thread id is: " << threadId << ", audio track id is: " << audioTrackId
             << ", video track id is: " << videoTrackId << endl;

        ret = muxerDemo->InnerStart();
        cout << "thread id is: " << threadId << ", Start ret is:" << ret << endl;

        WriteTrackSampleByFd(muxerDemo, audioTrackId, videoTrackId, inputFile);

        ret = muxerDemo->InnerStop();
        cout << "thread id is: " << threadId << ", Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
        cout << "thread id is: " << threadId << ", Destroy ret is:" << ret << endl;

        close(inputFile);
        close(fd);
        curTime = time(nullptr);
        ASSERT_NE(curTime, -1);
    }
    g_testResult[threadId] = AVCS_ERR_OK;
    delete muxerDemo;
}
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001
 * @tc.name      : Create(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_001");

    g_inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);
    struct timeval start, end;
    double totalTime = 0;
    for (int i = 0; i < RUN_TIMES; i++) {
        gettimeofday(&start, nullptr);
        muxerDemo->InnerCreate(fd, format);
        gettimeofday(&end, nullptr);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << endl;
        int32_t ret = muxerDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);
    }
    cout << "1000 times finish, run time is " << totalTime << endl;
    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002
 * @tc.name      : SetRotation(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_002");

    muxerDemo->InnerCreate(fd, format);
    double totalTime = 0;
    struct timeval start, end;

    for (int i = 0; i < RUN_TIMES; i++) {
        gettimeofday(&start, nullptr);
        int32_t ret = SetRotation(muxerDemo);
        gettimeofday(&end, nullptr);
        ASSERT_EQ(AVCS_ERR_OK, ret);
        totalTime += (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "1000 times finish, run time is " << totalTime << endl;
    muxerDemo->InnerDestroy();

    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003
 * @tc.name      : AddTrack(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_003");

    muxerDemo->InnerCreate(fd, format);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++) {
        int32_t trackId = -1;
        gettimeofday(&start, nullptr);
        AddTrack(muxerDemo, trackId);
        gettimeofday(&end, nullptr);
        ASSERT_EQ(-1, trackId);
        cout << "run time is: " << i << ", track id is:" << trackId << endl;
    }
    cout << "1000 times finish, run time is " << totalTime << endl;
    muxerDemo->InnerDestroy();

    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004
 * @tc.name      : Start(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_004");

    muxerDemo->InnerCreate(fd, format);
    int32_t audioTrackId;
    int32_t trackId = AddTrack(muxerDemo, audioTrackId);
    ASSERT_EQ(0, trackId);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++) {
        gettimeofday(&start, nullptr);
        int32_t ret = muxerDemo->InnerStart();
        gettimeofday(&end, nullptr);
        ASSERT_EQ(AVCS_ERR_OK, ret);
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "1000 times finish, run time is " << totalTime << endl;
    muxerDemo->InnerDestroy();

    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005
 * @tc.name      : WriteSample(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_005");

    muxerDemo->InnerCreate(fd, format);

    int32_t audioTrackId;
    int32_t trackId = AddTrack(muxerDemo, audioTrackId);
    ASSERT_EQ(0, trackId);

    int32_t ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++) {
        gettimeofday(&start, nullptr);
        ret = WriteSample(muxerDemo);
        gettimeofday(&end, nullptr);
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "1000 times finish, run time is " << totalTime << endl;
    muxerDemo->InnerDestroy();

    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006
 * @tc.name      : Stop(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_006");

    muxerDemo->InnerCreate(fd, format);

    int32_t audioTrackId;
    int32_t trackId = AddTrack(muxerDemo, audioTrackId);
    ASSERT_EQ(0, trackId);

    int32_t ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++) {
        gettimeofday(&start, nullptr);
        ret = muxerDemo->InnerStop();
        gettimeofday(&end, nullptr);
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "1000 times finish, run time is " << totalTime << endl;
    muxerDemo->InnerDestroy();

    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007
 * @tc.name      : Destroy(1000 times)
 * @tc.desc      : Stability test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_007");

    double totalTime = 0;
    struct timeval start, end;
    for (int i = 0; i < RUN_TIMES; i++) {
        muxerDemo->InnerCreate(fd, format);

        gettimeofday(&start, nullptr);
        int32_t ret = muxerDemo->InnerDestroy();
        gettimeofday(&end, nullptr);
        ASSERT_EQ(AVCS_ERR_OK, ret);
        cout << "run time is: " << i << ", ret is:" << ret << endl;
    }
    cout << "1000 times finish, run time is " << totalTime << endl;
    close(fd);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008
 * @tc.name      : m4a(long time)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME) {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;
        Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
        int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_008");

        g_inputFile = open("avData_mpeg4_aac_2.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        AddAudioTrackAAC(muxerDemo, audioTrackId);
        int32_t videoTrackId = -1;
        RemoveHeader();

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;

        WriteTrackSample(muxerDemo, audioTrackId, videoTrackId);

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
        cout << "Destroy ret is:" << ret << endl;

        close(g_inputFile);
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
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_009, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
    time_t startTime = time(nullptr);
    ASSERT_NE(startTime, -1);
    time_t curTime = startTime;

    while (difftime(curTime, startTime) < RUN_TIME) {
        cout << "run time: " << difftime(curTime, startTime) << " seconds" << endl;

        Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
        int32_t fd = muxerDemo->InnerGetFdByName(format, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_009");

        g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        AddAudioTrack(muxerDemo, audioTrackId);
        int32_t videoTrackId;
        AddVideoTrack(muxerDemo, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;

        WriteTrackSample(muxerDemo, audioTrackId, videoTrackId);

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
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
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_010, TestSize.Level2)
{
    vector<thread> threadVec;
    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    for (int i = 0; i < 10; i++) {
        threadVec.push_back(thread(RunMuxer, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_010", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++) {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AV_ERR_OK, g_testResult[i]);
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011
 * @tc.name      : mp4(thread long time)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerStablityTest, SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011, TestSize.Level2)
{
    vector<thread> threadVec;
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    for (int i = 0; i < 10; i++) {
        threadVec.push_back(thread(RunMuxer, "SUB_MULTIMEDIA_MEDIA_MUXER_STABILITY_011", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++) {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AV_ERR_OK, g_testResult[i]);
    }
}