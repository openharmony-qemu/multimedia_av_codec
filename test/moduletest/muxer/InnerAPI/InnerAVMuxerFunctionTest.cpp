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
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
constexpr uint32_t SAMPLE_RATE_352 = 352;
constexpr uint32_t SAMPLE_RATE_288 = 288;
constexpr uint32_t CHANNEL_COUNT = 2;
constexpr uint32_t SAMPLE_RATE_44100 = 44100;
constexpr uint32_t EXTRA_SIZE_NUM = 100;
constexpr int32_t BUFFER_SIZE_NUM = 1024 * 1024;

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
int32_t g_testResult[10] = { -1 };

int32_t AddAudioTrack(AVMuxerDemo *muxerDemo, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();

    int extraSize = 0;
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(g_inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_MPEG);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);

    return ret;
}

int32_t AddAudioTrackAAC(AVMuxerDemo *muxerDemo, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();

    int extraSize = 0;
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(g_inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);

    return ret;
}

int32_t AddVideoTrack(AVMuxerDemo *muxerDemo, int32_t &trackIndex)
{
    std::shared_ptr<Meta> videoParams = std::make_shared<Meta>();

    int extraSize = 0;
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(g_inputFile, buffer.data(), extraSize);
        videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
    videoParams->Set<Tag::VIDEO_WIDTH>(SAMPLE_RATE_352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(SAMPLE_RATE_288);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, videoParams);

    return ret;
}

int32_t AddCoverTrack(AVMuxerDemo *muxerDemo, string coverType, int32_t trackIndex)
{
    std::shared_ptr<Meta> coverFormat = std::make_shared<Meta>();

    if (coverType == "jpg") {
        coverFormat->Set<Tag::MIME_TYPE>(Plugins::MimeType::IMAGE_JPG);
    } else if (coverType == "png") {
        coverFormat->Set<Tag::MIME_TYPE>(Plugins::MimeType::IMAGE_PNG);
    } else {
        coverFormat->Set<Tag::MIME_TYPE>(Plugins::MimeType::IMAGE_BMP);
    }
    coverFormat->Set<Tag::VIDEO_WIDTH>(SAMPLE_RATE_352);
    coverFormat->Set<Tag::VIDEO_HEIGHT>(SAMPLE_RATE_288);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, coverFormat);

    return ret;
}

void RemoveHeader()
{
    int extraSize = 0;
    unsigned char buffer[100] = {0};
    read(g_inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
        read(g_inputFile, buffer, extraSize);
    }
}

void WriteTrackSample(AVMuxerDemo *muxerDemo, int audioTrackIndex, int videoTrackIndex)
{
    int dataTrackId = 0;
    int dataSize = 0;
    int trackId = 0;
    uint32_t trackIndex;
    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> data = AVBuffer::CreateAVBuffer(alloc, BUFFER_SIZE_NUM);
    int ret = 0;
    do {
        ret = read(g_inputFile, static_cast<void*>(&dataTrackId), sizeof(dataTrackId));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(&data->pts_), sizeof(data->pts_));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(&dataSize), sizeof(dataSize));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(data->memory_->GetAddr()), dataSize);
        if (ret <= 0) {
            return;
        }

        data->memory_->SetSize(dataSize);
        if (dataTrackId == DATA_AUDIO_ID) {
            trackId = audioTrackIndex;
        } else if (dataTrackId == DATA_VIDEO_ID) {
            trackId = videoTrackIndex;
        } else {
            cout << "error dataTrackId : " << trackId << endl;
        }
        if (trackId >= 0) {
            trackIndex = trackId;
            int32_t result = muxerDemo->InnerWriteSample(trackIndex, data);
            if (result != AVCS_ERR_OK) {
                return;
            }
        }
    } while (ret > 0)
}

void WriteTrackSampleShort(AVMuxerDemo *muxerDemo, int audioTrackIndex, int videoTrackIndex, int audioWriteTime)
{
    int dataTrackId = 0;
    int dataSize = 0;
    int trackId = 0;
    int curTime = 0;
    uint32_t trackIndex;
    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> data = AVBuffer::CreateAVBuffer(alloc, BUFFER_SIZE_NUM);
    int ret = 0;
    do {
        ret = read(g_inputFile, static_cast<void*>(&dataTrackId), sizeof(dataTrackId));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(&data->pts_), sizeof(data->pts_));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(&dataSize), sizeof(dataSize));
        if (ret <= 0) {
            return;
        }
        ret = read(g_inputFile, static_cast<void*>(data->memory_->GetAddr()), dataSize);
        if (ret <= 0) {
            return;
        }

        data->memory_->SetSize(dataSize);
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
            int32_t result = muxerDemo->InnerWriteSample(trackIndex, data);
            if (result != AVCS_ERR_OK) {
                return;
            }
        }
    } while (ret > 0)
}

int32_t AddAudioTrackByFd(AVMuxerDemo *muxerDemo, int32_t inputFile, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::shared_ptr<Meta>();

    int extraSize = 0;
    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(inputFile, buffer.data(), extraSize);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_MPEG);
    audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(CHANNEL_COUNT);
    audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_44100);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, audioParams);

    return ret;
}

int32_t AddAudioTrackAACByFd(AVMuxerDemo *muxerDemo, int32_t inputFile, int32_t &trackIndex)
{
    std::shared_ptr<Meta> audioParams = std::shared_ptr<Meta>();

    int extraSize = 0;
    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
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
    std::shared_ptr<Meta> videoParams = std::shared_ptr<Meta>();

    int extraSize = 0;
    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(inputFile, buffer.data(), extraSize);
        videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_MPEG4);
    videoParams->Set<Tag::VIDEO_WIDTH>(SAMPLE_RATE_352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(SAMPLE_RATE_288);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, videoParams);

    return ret;
}

int32_t AddVideoTrackH264ByFd(AVMuxerDemo *muxerDemo, int32_t inputFile, int32_t &trackIndex)
{
    std::shared_ptr<Meta> videoParams = std::shared_ptr<Meta>();

    int extraSize = 0;
    read(inputFile, static_cast<void*>(&extraSize), sizeof(extraSize));
    if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
        std::vector<uint8_t> buffer(extraSize);
        read(inputFile, buffer.data(), extraSize);
        videoParams->Set<Tag::MEDIA_CODEC_CONFIG>(buffer);
    }
    videoParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::VIDEO_AVC);
    videoParams->Set<Tag::VIDEO_WIDTH>(SAMPLE_RATE_352);
    videoParams->Set<Tag::VIDEO_HEIGHT>(SAMPLE_RATE_288);

    int32_t ret = muxerDemo->InnerAddTrack(trackIndex, videoParams);

    return ret;
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
        auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        avMuxerDemoBuffer = AVBuffer::CreateAVBuffer(alloc, dataSize);
        if (avMuxerDemoBuffer == nullptr) {
            printf("error malloc memory!\n");
            return -1;
        }
    }
    return 0;
}

int WriteTrackSampleByFdGetIndex(const int *dataTrackId, const int *audioTrackIndex, int *videoTrackIndex)
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
    int ret = 0;
    do {
        ret = WriteTrackSampleByFdRead(&inputFile, &pts, &dataSize, &dataTrackId);
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
            if (result != 0) {
                cout << "    WriteSampleBuffer error! ret is: " << result << endl;
                break;
            }
        }
    }while (ret > 0)
}

void RunMuxer(string testcaseName, int threadId, Plugins::OutputFormat format)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();
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
        if (extraSize <= EXTRA_SIZE_NUM && extraSize > 0) {
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

    g_testResult[threadId] = AVCS_ERR_OK;
    close(inputFile);
    close(fd);
    delete muxerDemo;
}

int WriteSingleTrackSampleRead(int *fp, int64_t *pts, int *dataSize, int *flags)
{
    int ret = read(*fp, static_cast<void*>(pts), sizeof(*pts));
    if (ret <= 0) {
        return -1;
    }

    ret = read(*fp, static_cast<void*>(flags), sizeof(*flags));
    if (ret <= 0) {
        return -1;
    }

    ret = read(*fp, static_cast<void*>(dataSize), sizeof(*dataSize));
    if (ret <= 0 || *dataSize < 0) {
        return -1;
    }
    return 0;
}

int WriteSingleTrackSampleMem(int dataSize, std::shared_ptr<AVBuffer>& avMuxerDemoBuffer)
{
    if (avMuxerDemoBuffer != nullptr && dataSize > avMuxerDemoBuffer->memory_->GetCapacity()) {
        avMuxerDemoBuffer = nullptr;
    }
    if (avMuxerDemoBuffer == nullptr) {
        auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        avMuxerDemoBuffer = AVBuffer::CreateAVBuffer(alloc, dataSize);
        if (avMuxerDemoBuffer == nullptr) {
            printf("error malloc memory! %d\n", dataSize);
            return -1;
        }
    }
    return 0;
}

void WriteSingleTrackSample(AVMuxerDemo *muxerDemo, int trackId, int fd)
{
    int ret = 0;
    int dataSize = 0;
    int flags = 0;
    int64_t pts = 0;
    std::shared_ptr<AVBuffer> avMuxerDemoBuffer = nullptr;
    uint32_t trackIndex;
    do {
        ret = WriteSingleTrackSampleRead(&fd, &pts, &dataSize, &flags);
        if (ret != 0) {
            break;
        }
        ret = WriteSingleTrackSampleMem(dataSize, avMuxerDemoBuffer);
        if (ret != 0) {
            break;
        }
        ret = read(fd, static_cast<void*>(avMuxerDemoBuffer->memory_->GetAddr()), dataSize);
        if (ret <= 0) {
            break;
        }
        avMuxerDemoBuffer->pts_ = pts;
        avMuxerDemoBuffer->memory_->SetSize(dataSize);

        if (flags != 0) {
            avMuxerDemoBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
        }
        trackIndex = trackId;
        int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMuxerDemoBuffer);
        if (result != 0) {
            cout << "WriteSingleTrackSample error! ret is: " << result << endl;
            break;
        }
    } while (ret > 0)
}

void WriteTrackCover(AVMuxerDemo *muxerDemo, int coverTrackIndex, int fdInput)
{
    printf("WriteTrackCover\n");
    uint32_t trackIndex;
    struct stat fileStat;
    fstat(fdInput, &fileStat);
    int32_t size = fileStat.st_size;

    auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
    std::shared_ptr<AVBuffer> avMuxerDemoBuffer = AVBuffer::CreateAVBuffer(alloc, size);
    if (avMuxerDemoBuffer == nullptr) {
        printf("malloc memory error! size: %d \n", size);
        return;
    }

    int ret = read(fdInput, avMuxerDemoBuffer->memory_->GetAddr(), size);
    if (ret <= 0) {
        return;
    }
    avMuxerDemoBuffer->memory_->SetSize(size);
    trackIndex = coverTrackIndex;
    int32_t result = muxerDemo->InnerWriteSample(trackIndex, avMuxerDemoBuffer);
    if (result != 0) {
        cout << "WriteTrackCover error! ret is: " << result << endl;
        return;
    }
}
} // namespace

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001
 * @tc.name      : audio
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_001, TestSize.Level2)
{
    const Plugins::OutputFormat formatList[] = {Plugins::OutputFormat::M4A, Plugins::OutputFormat::MPEG_4};
    for (int i = 0; i < 2; i++) {
        AVMuxerDemo *muxerDemo = new AVMuxerDemo();

        Plugins::OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->InnerGetFdByName(format, "FUNCTION_001_INNER_" + to_string(i));

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        AddAudioTrackAAC(muxerDemo, audioTrackId);

        int32_t videoTrackId = -1;
        RemoveHeader();

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        ASSERT_EQ(AVCS_ERR_OK, ret);
        audioTrackId = 0;

        if (audioTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        }

        ret = muxerDemo->InnerStop();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        ret = muxerDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);

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
    const Plugins::OutputFormat formatList[] = {Plugins::OutputFormat::M4A, Plugins::OutputFormat::MPEG_4};
    for (int i = 0; i < 2; i++) {
        AVMuxerDemo *muxerDemo = new AVMuxerDemo();

        Plugins::OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->InnerGetFdByName(format, "FUNCTION_002_INNER_" + to_string(i));

        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId = -1;
        RemoveHeader();
        int32_t videoTrackId;
        AddVideoTrack(muxerDemo, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        if (videoTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->InnerStop();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        ret = muxerDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);

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
    const Plugins::OutputFormat formatList[] = {Plugins::OutputFormat::M4A, Plugins::OutputFormat::MPEG_4};
    for (int i = 0; i < 2; i++) {
        AVMuxerDemo *muxerDemo = new AVMuxerDemo();

        Plugins::OutputFormat format = formatList[i];
        int32_t fd = muxerDemo->InnerGetFdByName(format, "FUNCTION_003_INNER_" + to_string(i));

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        AddAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        int32_t videoTrackId;
        AddVideoTrackByFd(muxerDemo, videoFileFd, videoTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        if (audioTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        }
        if (videoTrackId >= 0) {
            WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);
        }

        ret = muxerDemo->InnerStop();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        ret = muxerDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);
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
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "FUNCTION_004_INNER");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);

    int32_t audioTrackId;
    AddAudioTrack(muxerDemo, audioTrackId);
    int32_t videoTrackId;
    AddVideoTrack(muxerDemo, videoTrackId);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    int32_t ret;
    ret = muxerDemo->InnerSetRotation(90);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    WriteTrackSample(muxerDemo, audioTrackId, videoTrackId);

    ret = muxerDemo->InnerStop();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = muxerDemo->InnerDestroy();
    ASSERT_EQ(AVCS_ERR_OK, ret);

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
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "FUNCTION_005_INNER");

    g_inputFile = open("avDataMpegMpeg4.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);
    int32_t ret;

    int32_t audioTrackId;
    AddAudioTrack(muxerDemo, audioTrackId);
    int32_t videoTrackId;
    AddVideoTrack(muxerDemo, videoTrackId);

    cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId << endl;

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    WriteTrackSampleShort(muxerDemo, audioTrackId, videoTrackId, 100);

    ret = muxerDemo->InnerStop();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = muxerDemo->InnerDestroy();
    ASSERT_EQ(AVCS_ERR_OK, ret);

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
    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    for (int i = 0; i < 16; i++) {
        threadVec.push_back(thread(RunMuxer, "FUNCTION_006_INNER", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++) {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AVCS_ERR_OK, g_testResult[i]);
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
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    for (int i = 0; i < 16; i++) {
        threadVec.push_back(thread(RunMuxer, "FUNCTION_007_INNER", i, format));
    }
    for (uint32_t i = 0; i < threadVec.size(); i++) {
        threadVec[i].join();
    }
    for (int32_t i = 0; i < 10; i++)
    {
        ASSERT_EQ(AVCS_ERR_OK, g_testResult[i]);
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008
 * @tc.name      : m4a(multi audio track)
 * @tc.desc      : Function test
 */
HWTEST_F(InnerAVMuxerFunctionTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_008, TestSize.Level2)
{
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "FUNCTION_008_INNER");

    int32_t audioFileFd1 = open("aac_44100_2.bin", O_RDONLY);
    int32_t audioFileFd2 = open("aac_44100_2.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);

    int32_t audioTrackId1;
    int32_t audioTrackId2;
    AddAudioTrackAACByFd(muxerDemo, audioFileFd1, audioTrackId1);
    AddAudioTrackAACByFd(muxerDemo, audioFileFd2, audioTrackId2);
    RemoveHeader();

    cout << "audiotrack id1 is: " << audioTrackId1 << ", audioTrackId2 is: " << audioTrackId2 << endl;

    int32_t ret;

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    if (audioTrackId1 >= 0) {
        WriteSingleTrackSample(muxerDemo, audioTrackId1, audioFileFd1);
    }
    if (audioTrackId2 >= 0) {
        WriteSingleTrackSample(muxerDemo, audioTrackId2, audioFileFd2);
    }

    ret = muxerDemo->InnerStop();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = muxerDemo->InnerDestroy();
    ASSERT_EQ(AVCS_ERR_OK, ret);

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
    AVMuxerDemo *muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = muxerDemo->InnerGetFdByName(format, "FUNCTION_009_INNER");

    int32_t videoFileFd1 = open("h264_640_360.bin", O_RDONLY);
    int32_t videoFileFd2 = open("h264_640_360.bin", O_RDONLY);

    muxerDemo->InnerCreate(fd, format);

    int32_t videoTrackId1;
    int32_t videoTrackId2;
    AddVideoTrackH264ByFd(muxerDemo, videoFileFd1, videoTrackId1);
    AddVideoTrackH264ByFd(muxerDemo, videoFileFd2, videoTrackId2);

    int32_t ret;

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    if (videoTrackId1 >= 0) {
        WriteSingleTrackSample(muxerDemo, videoTrackId1, videoFileFd1);
    }
    if (videoTrackId2 >= 0) {
        WriteSingleTrackSample(muxerDemo, videoTrackId2, videoFileFd2);
    }

    ret = muxerDemo->InnerStop();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = muxerDemo->InnerDestroy();
    ASSERT_EQ(AVCS_ERR_OK, ret);

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
    string coverTypeList[] = {"bmp", "jpg", "png"};
    for (int i = 0; i < 3; i++) {
        AVMuxerDemo *muxerDemo = new AVMuxerDemo();
        string outputFile = "SUB_MULTIMEDIA_MEDIA_MUXER_FUNCTION_010_INNER_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];

        Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
        int32_t fd = muxerDemo->InnerGetFdByName(format, outputFile);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("h264_640_360.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        int32_t videoTrackId;
        int32_t coverTrackId = 1;

        AddAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        AddVideoTrackH264ByFd(muxerDemo, videoFileFd, videoTrackId);
        AddCoverTrack(muxerDemo, coverTypeList[i], coverTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId
             << ", cover track id is: " << coverTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        WriteTrackCover(muxerDemo, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);

        ret = muxerDemo->InnerStop();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        ret = muxerDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);

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
    string coverTypeList[] = {"bmp", "jpg", "png"};
    for (int i = 0; i < 3; i++) {
        AVMuxerDemo *muxerDemo = new AVMuxerDemo();
        string outputFile = "FUNCTION_011_INNER_" + coverTypeList[i];
        string coverFile = "greatwall." + coverTypeList[i];

        Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
        int32_t fd = muxerDemo->InnerGetFdByName(format, outputFile);

        int32_t audioFileFd = open("aac_44100_2.bin", O_RDONLY);
        int32_t videoFileFd = open("mpeg4_720_480.bin", O_RDONLY);
        int32_t coverFileFd = open(coverFile.c_str(), O_RDONLY);

        muxerDemo->InnerCreate(fd, format);

        int32_t audioTrackId;
        int32_t videoTrackId;
        int32_t coverTrackId = 1;

        AddAudioTrackAACByFd(muxerDemo, audioFileFd, audioTrackId);
        AddVideoTrackByFd(muxerDemo, videoFileFd, videoTrackId);
        AddCoverTrack(muxerDemo, coverTypeList[i], coverTrackId);

        cout << "audio track id is: " << audioTrackId << ", video track id is: " << videoTrackId
             << ", cover track id is: " << coverTrackId << endl;

        int32_t ret;

        ret = muxerDemo->InnerStart();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        WriteTrackCover(muxerDemo, coverTrackId, coverFileFd);
        WriteSingleTrackSample(muxerDemo, audioTrackId, audioFileFd);
        WriteSingleTrackSample(muxerDemo, videoTrackId, videoFileFd);

        ret = muxerDemo->InnerStop();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        ret = muxerDemo->InnerDestroy();
        ASSERT_EQ(AVCS_ERR_OK, ret);

        close(audioFileFd);
        close(videoFileFd);
        close(coverFileFd);
        close(fd);
        delete muxerDemo;
    }
}
