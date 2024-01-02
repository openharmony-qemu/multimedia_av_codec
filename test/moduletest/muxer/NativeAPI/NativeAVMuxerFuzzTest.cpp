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

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;


namespace {
    class NativeAVMuxerFuzzTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeAVMuxerFuzzTest::SetUpTestCase() {}
    void NativeAVMuxerFuzzTest::TearDownTestCase() {}
    void NativeAVMuxerFuzzTest::SetUp() {}
    void NativeAVMuxerFuzzTest::TearDown() {}

    constexpr int FUZZ_TEST_NUM = 1000000;
    constexpr int AUDIO_PTS_ADD = 21;

    int32_t getIntRand()
    {
        int32_t data = -10000 + rand() % 20001;
        return data;
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_001
 * @tc.name      : OH_AVMuxer_Create
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_001, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;

    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        fd = rand();
        format = OH_AVOutputFormat(rand() % 3);
        handle = muxerDemo->NativeCreate(fd, format);
        if (handle != nullptr) {
            muxerDemo->NativeDestroy(handle);
        }
    }

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_002
 * @tc.name      : OH_AVMuxer_SetRotation
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_002, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    int32_t rotation;
    OH_AVErrCode ret;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        rotation = getIntRand();
        cout << "rotation is: " << rotation << endl;
        ret = muxerDemo->NativeSetRotation(handle, rotation);
        cout << "ret code is: " << ret << endl;
    }

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_003
 * @tc.name      : OH_AVMuxer_AddTrack
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_003, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    string mimeType[] = {"audio/mp4a-latm", "audio/mpeg", "video/avc", "video/mp4v-es"};
    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    int32_t trackId;
    OH_AVErrCode ret;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int typeIndex = rand() % 4;
        int audioChannels = getIntRand();
        int audioSampleRate = getIntRand();

        int videoWidth = getIntRand();
        int videoHeight = getIntRand();

        // audio config
        OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, mimeType[typeIndex].c_str());
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, audioChannels);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, audioSampleRate);

        // video config
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, videoWidth);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, videoHeight);

        ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
        cout << "trackId is: " << trackId << ", ret is: " << ret << endl;
    }

    OH_AVFormat_Destroy(trackFormat);
    muxerDemo->NativeDestroy(handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_004
 * @tc.name      : WriteSampleBuffer
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_004, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_M4A;
    int32_t fd = -1;
    fd = muxerDemo->GetFdByMode(format);
    OH_AVMuxer* handle = muxerDemo->NativeCreate(fd, format);
    ASSERT_NE(nullptr, handle);

    uint8_t a[100];
    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
    OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, 320000);
    OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, 100);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, 1);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, 48000);
    OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, 0);

    int32_t trackId;
    OH_AVErrCode ret;

    ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
    ASSERT_EQ(0, trackId);

    ret = muxerDemo->NativeStart(handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    OH_AVCodecBufferAttr info;
    info.pts = 0;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        int dataLen = rand() % 65536;
        OH_AVMemory* avMemBuffer = OH_AVMemory_Create(dataLen);

        cout << "data len is:" << dataLen << endl;

        info.pts += AUDIO_PTS_ADD;
        info.size = dataLen;
        info.offset = getIntRand();
        info.flags = getIntRand();

        cout << "info.pts is:" << info.pts << endl;
        cout << "info.size is:" << info.size << endl;
        cout << "info.offset is:" << info.offset << endl;
        cout << "info.flags is:" << info.flags << endl;

        ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);
        cout << "ret code is: " << ret << endl;
        OH_AVMemory_Destroy(avMemBuffer);
    }

    muxerDemo->NativeDestroy(handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_005
 * @tc.name      : WriteSampleBuffer
 * @tc.desc      : Fuzz test
 */
HWTEST_F(NativeAVMuxerFuzzTest, SUB_MULTIMEDIA_MEDIA_MUXER_FUZZ_005, TestSize.Level2)
{
    srand(time(nullptr) * 10);
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();

    OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
    int32_t fd = -1;
    OH_AVMuxer* handle;

    string test_key = "";
    string test_value = "";

    OH_AVFormat* trackFormat = OH_AVFormat_Create();
    string mimeType[] = {
        "audio/mp4a-latm", "audio/mpeg", "video/avc", "video/mp4v-es","image/jpeg", "image/png", "image/bmp"
    };

    int32_t trackId;
    OH_AVErrCode ret;

    OH_AVCodecBufferAttr info;
    info.pts = 0;

    for (int i = 0; i < FUZZ_TEST_NUM; i++) {
        cout << "current run time is: " << i << endl;
        // Create
        fd = rand();
        format = OH_AVOutputFormat(rand() % 3);
        handle = muxerDemo->NativeCreate(fd, format);

        // SetRotation
        float rotation = getIntRand();
        ret = muxerDemo->NativeSetRotation(handle, rotation);

        // AddTrack
        int typeIndex = rand() % 4;
        int audioChannels = getIntRand();
        int audioSampleRate = getIntRand();

        int videoWidth = getIntRand();
        int videoHeight = getIntRand();

        // audio config
        OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, mimeType[typeIndex].c_str());
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, audioChannels);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, audioSampleRate);

        // video config
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_WIDTH, videoWidth);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_HEIGHT, videoHeight);

        ret = muxerDemo->NativeAddTrack(handle, &trackId, trackFormat);
        ret = muxerDemo->NativeStart(handle);

        int dataLen = rand() % 65536;
        OH_AVMemory* avMemBuffer = OH_AVMemory_Create(dataLen);

        info.pts += AUDIO_PTS_ADD;
        info.size = dataLen;
        info.offset = getIntRand();
        info.flags = getIntRand();

        ret = muxerDemo->NativeWriteSampleBuffer(handle, trackId, avMemBuffer, info);

        ret = muxerDemo->NativeStop(handle);

        ret = muxerDemo->NativeDestroy(handle);
        cout << "Destroy ret is:" << ret << endl;
        OH_AVMemory_Destroy(avMemBuffer);
    }

    delete muxerDemo;
}