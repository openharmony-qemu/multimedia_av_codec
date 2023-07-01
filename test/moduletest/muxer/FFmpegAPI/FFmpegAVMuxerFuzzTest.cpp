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
#include "avcodec_info.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Plugin;
using namespace Ffmpeg;

namespace {
    class FFmpegAVMuxerFuzzTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void FFmpegAVMuxerFuzzTest::SetUpTestCase() {}
    void FFmpegAVMuxerFuzzTest::TearDownTestCase() {}
    void FFmpegAVMuxerFuzzTest::SetUp() {}
    void FFmpegAVMuxerFuzzTest::TearDown() {}

    constexpr int FUZZ_TEST_NUM = 1000000;

    int32_t getIntRand()
    {
        int32_t data = -10000 + rand() % 20001;
        return data;
    }
    
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_001
 * @tc.name      : FFmpegCreate
 * @tc.desc      : Fuzz test
 */
HWTEST_F(FFmpegAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    int32_t fd = -1;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        fd = rand();
        
        muxerDemo->FFmpegCreate(fd);
        muxerDemo->FFmpegDestroy();
    }

    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_002
 * @tc.name      : FFmpegSetRotation
 * @tc.desc      : Fuzz test
 */
HWTEST_F(FFmpegAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    int32_t rotation;
    Status ret;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        rotation = getIntRand();
        cout << "rotation is: " << rotation << endl;
        ret = muxerDemo->FFmpegSetRotation(rotation);
        cout << "ret code is: " << (int)ret << endl;
    }
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_003
 * @tc.name      : FFmpegAddTrack
 * @tc.desc      : Fuzz test
 */
HWTEST_F(FFmpegAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_003, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

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

        cout << "MediaDescriptionKey::MD_KEY_CODEC_MIME is: " << mimeType[typeIndex] << endl;
        cout << "OH_AV_KEY_BIT_RATE is: " << bitRate << endl;
        cout << "MediaDescriptionKey::MD_KEY_CHANNEL_COUNT len is: " << dataLen << endl;
        cout << "OH_AV_KEY_AUDIO_SAMPLE_FORMAT is: " << audioSampleFormat << endl;
        cout << "OH_AV_KEY_AUDIO_CHANNELS len is: " << audioChannels << endl;
        cout << "OH_AV_KEY_VIDEO_HEIGHT is: " << videoHeight << endl;
        cout << "OH_AV_KEY_VIDEO_FRAME_RATE len is: " << videoFrameRate << endl;

        mediaParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, mimeType[typeIndex].c_str());
        mediaParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, bitRate);
        mediaParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, data, dataLen);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, audioChannels);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, audioSampleRate);
        // video config
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, videoWidth);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, videoHeight);
        mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, videoFrameRate);
        
        int trackId = 0;
        muxerDemo->FFmpegAddTrack(trackId, mediaParams);
        cout << "trackId is: " << trackId << endl;
    }

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_004
 * @tc.name      : WriteSampleBuffer
 * @tc.desc      : Fuzz test
 */
HWTEST_F(FFmpegAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_004, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OutputFormat format = OUTPUT_FORMAT_M4A;
    int32_t fd = -1;
    fd = muxerDemo->FFmpeggetFdByMode(format);
    muxerDemo->FFmpegCreate(fd);

    uint8_t a[100];
    MediaDescription mediaParams;
    mediaParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
    mediaParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, 320000);
    mediaParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, 100);
    mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, 1);
    mediaParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, 48000);

    Status ret;
    int trackIndex = 0;

    ret = muxerDemo->FFmpegAddTrack(trackIndex, mediaParams);
    ASSERT_EQ(Status::OK, ret);

    ret = muxerDemo->FFmpegStart();
    ASSERT_EQ(Status::OK, ret);

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

        cout << "info.presentationTimeUs is:" << info.presentationTimeUs << endl;
        cout << "info.size is:" << info.size << endl;
        cout << "info.offset is:" << info.offset << endl;
        cout << "flag is:" << flag << endl;
        cout << "info.trackIndex is:" << trackIndex << endl;


        ret = muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
        cout << "ret code is: " << (int)ret << endl;
    }

    muxerDemo->FFmpegDestroy();
    delete muxerDemo;
}

