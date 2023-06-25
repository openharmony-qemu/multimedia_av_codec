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
#include "AVMuxerDemoCommon.h"
#include "avcodec_info.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;


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
    int32_t getIntRand()
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
    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
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

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->InnergetFdByMode(format);
    muxerDemo->InnerCreate(fd, format);
    

    int32_t rotation;
    int32_t ret;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        rotation = getIntRand();
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

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->InnergetFdByMode(format);
    muxerDemo->InnerCreate(fd, format);
    

    string mimeType[] = {"audio/mp4a-latm", "audio/mpeg", "video/avc", "video/mp4v-es"};
    MediaDescription mediaParams;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int typeIndex = rand() % 4;
        int bitRate = getIntRand();
        int dataLen = rand() % 65536;
        uint8_t data[dataLen];
        int audioSampleFormat = getIntRand();
        int audioChannels = getIntRand();
        int audioSampleRate = getIntRand();

        int videoWidth = getIntRand();
        int videoHeight = getIntRand();
        int videoFrameRate = getIntRand();

        cout << "OH_AV_KEY_MIME is: " << mimeType[typeIndex] << endl;
        cout << "OH_AV_KEY_BIT_RATE is: " << bitRate << ", OH_AV_KEY_CODEC_CONFIG len is: " << dataLen << endl;
        cout << "OH_AV_KEY_AUDIO_SAMPLE_FORMAT is: " << audioSampleFormat <<
        ", OH_AV_KEY_AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << "OH_AV_KEY_VIDEO_HEIGHT is: " << videoHeight <<
        ", OH_AV_KEY_VIDEO_FRAME_RATE len is: " << videoFrameRate << endl;

        mediaParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType[typeIndex].c_str());
        mediaParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
        mediaParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, data, dataLen);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, audioChannels);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, audioSampleRate);

        // video config
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, videoWidth);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, videoHeight);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, videoFrameRate);
        
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

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = -1;
    fd = muxerDemo->InnergetFdByMode(format);
    muxerDemo->InnerCreate(fd, format);

    uint8_t a[100];
    MediaDescription mediaParams;
    mediaParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    mediaParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    mediaParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    int32_t trackId;
    int32_t ret;
    int trackIndex = 0;
    
    trackId = muxerDemo->InnerAddTrack(trackIndex, mediaParams);


    ret = muxerDemo->InnerStart();
    ASSERT_EQ(AVCS_ERR_OK, ret);

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.presentationTimeUs = 0;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int dataLen = rand() % 65536;
        uint8_t data[dataLen];
        cout << "data len is:" << dataLen << endl;

        info.presentationTimeUs += 21;
        info.size = dataLen;
        info.offset = getIntRand();
        trackIndex = trackId;

        cout << "info.presentationTimeUs is:" << info.presentationTimeUs << endl;
        cout << "info.size is:" << info.size << endl;
        cout << "info.offset is:" << info.offset << endl;
        cout << "flag is:" << flag << endl;
        cout << "trackIndex is:" << trackIndex << endl;
        std::shared_ptr<AVSharedMemoryBase> avMemBuffer = std::make_shared<AVSharedMemoryBase>
        (info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");
        avMemBuffer->Init();
        (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
        ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
        cout << "ret code is: " << ret << endl;
    }

    muxerDemo->InnerDestroy();
    delete muxerDemo;
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

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    

    string test_key = "";
    string test_value = "";

    MediaDescription mediaParams;
    string mimeType[] = { "audio/mp4a-latm", "audio/mpeg", "video/avc", "video/mp4v-es" };

    int32_t trackId;
    int32_t ret;

    AVCodecBufferInfo info;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_NONE;
    info.presentationTimeUs = 0;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;

        // Create
        fd = rand();
        format = OutputFormat(rand() % 3);
        cout << "fd is: " << fd << ", format is: " << format << endl;
        muxerDemo->InnerCreate(fd, format);
        cout << "Create ret code is: " << ret << endl;

        // SetRotation
        float rotation = getIntRand();
        cout << "rotation is: " << rotation << endl;
        ret = muxerDemo->InnerSetRotation(rotation);
        cout << "SetRotation ret code is: " << ret << endl;


        // AddTrack
        int typeIndex = rand() % 4;
        int bitRate = getIntRand();
        int configLen = rand() % 65536;
        uint8_t config[configLen];
        int audioSampleFormat = getIntRand();
        int audioChannels = getIntRand();
        int audioSampleRate = getIntRand();


        int videoWidth = getIntRand();
        int videoHeight = getIntRand();
        int videoFrameRate = getIntRand();

        cout << "OH_AV_KEY_MIME is: " << mimeType[typeIndex] << endl;
        cout << "OH_AV_KEY_BIT_RATE is: " << bitRate << ", OH_AV_KEY_CODEC_CONFIG len is: " << configLen << endl;
        cout << "OH_AV_KEY_AUDIO_SAMPLE_FORMAT is: " << audioSampleFormat
         << ", OH_AV_KEY_AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << "OH_AV_KEY_VIDEO_HEIGHT is: " << videoHeight <<
         ", OH_AV_KEY_VIDEO_FRAME_RATE len is: " << videoFrameRate << endl;

        // audio config
        mediaParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType[typeIndex].c_str());
        mediaParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
        mediaParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, config, configLen);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, audioChannels);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, audioSampleRate);


        // video config
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, videoWidth);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, videoHeight);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, videoFrameRate);

        int trackIndex = 0;
        trackId = muxerDemo->InnerAddTrack(trackIndex, mediaParams);
        cout << "trackId is: " << trackId << endl;

        ret = muxerDemo->InnerStart();
        cout << "Start ret is:" << ret << endl;

        int dataLen = rand() % 65536;
        uint8_t data[dataLen];
        cout << "data len is:" << dataLen << endl;

        info.presentationTimeUs += 21;
        info.size = dataLen;
        trackIndex = trackId;
        
        cout << "info.presentationTimeUs is:" << info.presentationTimeUs << endl;
        cout << "info.size is:" << info.size << endl;
        cout << "flag is:" << flag << endl;

        std::shared_ptr<AVSharedMemoryBase> avMemBuffer =
         std::make_shared<AVSharedMemoryBase>(info.size, AVSharedMemory::FLAGS_READ_ONLY, "sampleData");

        avMemBuffer->Init();
        (void)memcpy_s(avMemBuffer->GetBase(), avMemBuffer->GetSize(), data, info.size);
        ret = muxerDemo->InnerWriteSample(trackIndex, avMemBuffer, info, flag);
        cout << "WriteSample ret code is: " << ret << endl;

        ret = muxerDemo->InnerStop();
        cout << "Stop ret is:" << ret << endl;

        ret = muxerDemo->InnerDestroy();
        cout << "Destroy ret is:" << ret << endl;
    }

    delete muxerDemo;
}