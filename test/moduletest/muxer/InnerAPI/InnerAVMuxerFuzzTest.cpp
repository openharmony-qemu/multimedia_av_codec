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
#include <ctime>
#include "gtest/gtest.h"
#include "AVMuxerDemo.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;

namespace {
    class InnerAVMuxerFuzzTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };
    void InnerAVMuxerFuzzTest::SetUpTestCase() {}
    void InnerAVMuxerFuzzTest::TearDownTestCase() {}
    void InnerAVMuxerFuzzTest::SetUp() {}
    void InnerAVMuxerFuzzTest::TearDown() {}

    constexpr int FUZZ_TEST_NUM = 1000000;
    int32_t GetIntRand()
    {
        int32_t data = -10000 + rand() % 20001;
        return data;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_001
 * @tc.name      : Create
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = -1;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        std::cout << "current run time is: " << i << std::endl;
        fd = rand();

        muxerDemo->InnerCreate(fd, format);
        muxerDemo->InnerDestroy();
    }

    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_002
 * @tc.name      : SetRotation
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->InnerGetFdByMode(format);
    muxerDemo->InnerCreate(fd, format);


    int32_t rotation;
    int32_t ret;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        rotation = GetIntRand();
        cout << "rotation is: " << rotation << endl;
        ret = muxerDemo->InnerSetRotation(rotation);
        cout << "ret code is: " << ret << endl;
    }

    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_003
 * @tc.name      : AddTrack
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_003, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->InnerGetFdByMode(format);
    muxerDemo->InnerCreate(fd, format);


    string mimeType[] = {"audio/mp4a-latm", "audio/mpeg", "video/avc", "video/mp4v-es"};
    std::shared_ptr<Meta> mediaParams = std::make_shared<memset_sOptAsm>();

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int typeIndex = rand() % 4;
        int bitRate = GetIntRand();
        int dataLen = rand() % 65536;
        vector<uint8_t> data(dataLen);
        int audioSampleFormat = GetIntRand();
        int audioChannels = GetIntRand();
        int audioSampleRate = GetIntRand();

        int videoWidth = GetIntRand();
        int videoHeight = GetIntRand();
        double videoFrameRate = GetIntRand();

        cout << "OH_AV_KEY_MIME is: " << mimeType[typeIndex] << endl;
        cout << "OH_AV_KEY_BIT_RATE is: " << bitRate << ", OH_AV_KEY_CODEC_CONFIG len is: " << dataLen << endl;
        cout << "OH_AV_KEY_AUDIO_SAMPLE_FORMAT is: " << audioSampleFormat <<
        ", OH_AV_KEY_AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << "OH_AV_KEY_VIDEO_HEIGHT is: " << videoHeight <<
        ", OH_AV_KEY_VIDEO_FRAME_RATE len is: " << videoFrameRate << endl;

        mediaParams->Set<Tag::MIME_TYPE>(mimeType[typeIndex].c_str());
        mediaParams->Set<Tag::MEDIA_BITRATE>(bitRate);
        mediaParams->Set<Tag::MEDIA_CODEC_CONFIG>(data);
        mediaParams->Set<Tag::AUDIO_CHANNEL_COUNT>(audioChannels);
        mediaParams->Set<Tag::AUDIO_SAMPLE_RATE>(audioSampleRate);

        // video config
        mediaParams->Set<Tag::VIDEO_WIDTH>(videoWidth);
        mediaParams->Set<Tag::VIDEO_HEIGHT>(videoHeight);
        mediaParams->Set<Tag::VIDEO_FRAME_RATE>(videoFrameRate);

        int trackIndex = 0;
        muxerDemo->InnerAddTrack(trackIndex, mediaParams);
    }

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_004
 * @tc.name      : WriteSampleBuffer
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_004, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::M4A;
    int32_t fd = -1;
    fd = muxerDemo->InnerGetFdByMode(format);
    muxerDemo->InnerCreate(fd, format);

    std::vector<uint8_t> a(100);
    std::shared_ptr<Meta> mediaParams = std::make_shared<Meta>();
    mediaParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
    mediaParams->Set<Tag::MEDIA_BITRATE>(320000);
    mediaParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
    mediaParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
    mediaParams->Set<Tag::AUDIO_SAMPLE_RATE>(48000);

    int32_t trackId;
    int32_t ret;
    int trackIndex = 0;
    int64_t pts = 0;

    trackId = muxerDemo->InnerAddTrack(trackIndex, mediaParams);

    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int dataLen = rand() % 65536;
        uint8_t data[dataLen];
        cout << "data len is:" << dataLen << endl;

        pts += 21;
        trackIndex = trackId;

        cout << "pts is:" << pts << endl;
        cout << "size is:" << dataLen << endl;
        cout << "trackIndex is:" << trackIndex << endl;
        auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(alloc, dataLen);
        avMemBuffer->memory_->Write(data, dataLen);
        avMemBuffer->pts_ = pts;
        ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
        cout << "ret code is: " << ret << endl;
    }

    muxerDemo->InnerDestroy();
    delete muxerDemo;
}

static int HwTest_AddTrack(std::shared_ptr<Meta> mediaParams, int64_t *pts, int32_t *size, AVMuxerDemo *muxerDemo)
{
    string mimeType[] = { "audio/mp4a-latm", "audio/mpeg", "video/avc", "video/mp4v-es" };
    // AddTrack
    int typeIndex = rand() % 4;
    int bitRate = GetIntRand();
    int configLen = rand() % 65536;
    std::vector<uint8_t> config(configLen);
    int audioSampleFormat = GetIntRand();
    int audioChannels = GetIntRand();
    int audioSampleRate = GetIntRand();

    int videoWidth = GetIntRand();
    int videoHeight = GetIntRand();
    double videoFrameRate = GetIntRand();

    cout << "OH_AV_KEY_MIME is: " << mimeType[typeIndex] << endl;
    cout << "OH_AV_KEY_BIT_RATE is: " << bitRate << ", OH_AV_KEY_CODEC_CONFIG len is: " << configLen << endl;
    cout << "OH_AV_KEY_AUDIO_SAMPLE_FORMAT is: " << audioSampleFormat
        << ", OH_AV_KEY_AUDIO_CHANNELS len is: " << audioChannels << endl;
    cout << "OH_AV_KEY_VIDEO_HEIGHT is: " << videoHeight <<
        ", OH_AV_KEY_VIDEO_FRAME_RATE len is: " << videoFrameRate << endl;

    // audio config
    mediaParams->Set<Tag::MIME_TYPE>(mimeType[typeIndex].c_str());
    mediaParams->Set<Tag::MEDIA_BITRATE>(bitRate);
    mediaParams->Set<Tag::MEDIA_CODEC_CONFIG>(config);
    mediaParams->Set<Tag::AUDIO_CHANNEL_COUNT>(audioChannels);
    mediaParams->Set<Tag::AUDIO_SAMPLE_RATE>(audioSampleRate);

    // video config
    mediaParams->Set<Tag::VIDEO_WIDTH>(videoWidth);
    mediaParams->Set<Tag::VIDEO_HEIGHT>(videoHeight);
    mediaParams->Set<Tag::VIDEO_FRAME_RATE>(videoFrameRate);

    int trackIndex = 0;
    int32_t trackId;
    int32_t ret;
    trackId = muxerDemo->InnerAddTrack(trackIndex, mediaParams);
    cout << "trackId is: " << trackId << endl;

    ret = muxerDemo->InnerStart();
    cout << "Start ret is:" << ret << endl;

    int dataLen = rand() % 0x10000;

    constexpr int64_t PTS = 21;
    *pts += PTS;
    *size = dataLen;
    trackIndex = trackId;

    cout << "pts is:" << *pts << endl;
    cout << "size is:" << *size << endl;

    return trackIndex;
}
/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_005
 * @tc.name      : WriteSample
 * @tc.desc      : Fuzz test
 */
HWTEST_F(InnerAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_005, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
    int32_t fd = -1;


    string test_key = "";
    string test_value = "";

    std::shared_ptr<Meta> mediaParams = std::make_shared<Meta>();
    string mimeType[] = { "audio/mp4a-latm", "audio/mpeg", "video/avc", "video/mp4v-es" };

    int32_t ret;
    int64_t pts = 0;
    int32_t size = 0;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;

        // Create
        fd = rand();
        format = Plugins::OutputFormat(rand() % 3);
        cout << "fd is: " << fd << ", format is: " << static_cast<int32_t>(format) << endl;
        muxerDemo->InnerCreate(fd, format);
        cout << "Create ret code is: " << ret << endl;

        // SetRotation
        float rotation = GetIntRand();
        cout << "rotation is: " << rotation << endl;
        ret = muxerDemo->InnerSetRotation(rotation);
        cout << "SetRotation ret code is: " << ret << endl;

        // AddTrack
        int trackIndex = HwTest_AddTrack(mediaParams, &pts, &size, muxerDemo);

        auto alloc = AVAllocatorFactory::CreateSharedAllocator(MemoryFlag::MEMORY_READ_WRITE);
        std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(alloc, size);
        avMemBuffer->memory_->SetSize(size);

        ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
        cout << "WriteSample ret code is: " << ret << endl;

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
        cout << "Destroy ret is:" << ret << endl;
    }

    delete muxerDemo;
}