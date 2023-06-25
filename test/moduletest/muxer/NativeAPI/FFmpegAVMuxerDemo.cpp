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
#include "gtest/gtest.h"
#include "AVMuxerDemoCommon.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;


namespace {
    constexpr int64_t BIT_RATE = 32000;
    constexpr int32_t CODEC_CONFIG = 100;
    constexpr int32_t AUDIO_CHANNELS = 1;
    constexpr int32_t SAMPLE_RATE = 48000;
    constexpr int32_t SAMPLE_PER_FRAME = 480;
    constexpr int32_t AAC_PROFILE = 0;

    class FFmpegAVMuxerDemo : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void FFmpegAVMuxerDemo::SetUpTestCase() {}
    void FFmpegAVMuxerDemo::TearDownTestCase() {}
    void FFmpegAVMuxerDemo::SetUp() {}
    void FFmpegAVMuxerDemo::TearDown() {}


    void Create(AVMuxerDemo* muxerDemo)
    {
        OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
        int32_t fd = muxerDemo->getFdByMode(format);
        muxerDemo->FFmpegCreate(fd);
    }

    Status SetLocation(AVMuxerDemo* muxerDemo)
    {
        float latitude = 0;
        float longitude = 0;

        return muxerDemo->FFmpegSetLocation(latitude, longitude);
    }

    Status SetRotation(AVMuxerDemo* muxerDemo)
    {
        int32_t rotation = 0;

        return muxerDemo->FFmpegSetRotation(rotation);
    }

    Status SetParameter(AVMuxerDemo* muxerDemo)
    {
        Format generalFormat;
        generalFormat.PutStringValue(OH_AV_KEY_TITLE, "aaa");

        return muxerDemo->FFmpegSetParameter(generalFormat);
    }

    Status AddTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        Format trackFormat;
        uint8_t a[100];
        
        trackFormat.PutStringValue(OH_AV_KEY_MIME, OH_AV_MIME_AUDIO_AAC);
        trackFormat.PutLongValue(OH_AV_KEY_BIT_RATE, BIT_RATE);
        trackFormat.PutBuffer(OH_AV_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
        trackFormat.PutIntValue(OH_AV_KEY_AUDIO_SAMPLE_FORMAT, A_SAMPLE_FMT_S16);
        trackFormat.PutIntValue(OH_AV_KEY_AUDIO_CHANNELS, AUDIO_CHANNELS);
        trackFormat.PutIntValue(OH_AV_KEY_AUDIO_SAMPLE_RATE, SAMPLE_RATE);
        trackFormat.PutLongValue(OH_AV_KEY_AUDIO_CHANNEL_MASK, A_CH_MASK_STEREO);
        trackFormat.PutIntValue(OH_AV_KEY_AUDIO_SAMPLE_PER_FRAME, SAMPLE_PER_FRAME);
        trackFormat.PutIntValue(OH_AV_KEY_AUDIO_AAC_PROFILE, AAC_PROFILE);

        return muxerDemo->FFmpegAddTrack(trackIndex, trackFormat);
    }

    Status Start(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->FFmpegStart();
    }

    Status WriteSampleBuffer(AVMuxerDemo* muxerDemo, uint32_t trackIndex)
    {
        uint8_t data[100];

        AVCodecBufferInfo info;
        constexpr uint32_t INFO_SIZE = 100;
        info.size = INFO_SIZE;
        info.pts = 0;
        info.offset = 0;
        info.flags = 0;

        return muxerDemo->FFmpegWriteSampleBuffer(trackIndex, data, info);
    }

    Status Stop(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->FFmpegStop();
    }

    Status Destroy(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->FFmpegDestroy();
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001
 * @tc.name      : Create -> SetLocation -> SetRotation -> SetParameter -> AddTrack -> Start ->
 * WriteSampleBuffer -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerDemo, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;
    int32_t trackId;

    ret = SetLocation(muxerDemo);
    ASSERT_EQ(CSERR_OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(CSERR_OK, ret);

    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(CSERR_OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(CSERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(CSERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(CSERR_OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(CSERR_OK, ret);

    delete muxerDemo;
}