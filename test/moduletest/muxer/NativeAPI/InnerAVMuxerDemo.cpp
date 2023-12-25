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
#include "AVMuxerDemo.h"

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

    class InnerAVMuxerDemo : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void InnerAVMuxerDemo::SetUpTestCase() {}
    void InnerAVMuxerDemo::TearDownTestCase() {}
    void InnerAVMuxerDemo::SetUp() {}
    void InnerAVMuxerDemo::TearDown() {}


    void Create(AVMuxerDemo* muxerDemo)
    {
        Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
        int32_t fd = muxerDemo->InnerGetFdByMode(format);
        muxerDemo->InnerCreate(fd, format);
    }

    int32_t SetRotation(AVMuxerDemo* muxerDemo)
    {
        int32_t rotation = 0;
        return muxerDemo->InnerSetRotation(rotation);
    }

    int32_t AddTrack(AVMuxerDemo* muxerDemo)
    {
        std::shared_ptr<Meta> trackFormat = std::make_shared<Meta>();
        std::vector<uint8_t> a(CODEC_CONFIG);
        trackFormat->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
        trackFormat->Set<Tag::MEDIA_BITRATE>(BIT_RATE);
        trackFormat->Set<Tag::MEDIA_CODEC_CONFIG>(a);
        trackFormat->Set<Tag::AUDIO_CHANNEL_COUNT>(AUDIO_CHANNELS);
        trackFormat->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE);
        trackFormat->Set<Tag::AUDIO_SAMPLE_PER_FRAME>(SAMPLE_PER_FRAME);
        return muxerDemo->InnerAddTrack(trackFormat);
    }

    int32_t Start(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->InnerStart();
    }

    int32_t WriteSampleBuffer(AVMuxerDemo* muxerDemo, uint32_t trackIndex)
    {
        uint8_t data[100];

        AVCodecBufferInfo info;
        constexpr uint32_t INFO_SIZE = 100;
        info.size = INFO_SIZE;
        info.pts = 0;
        info.offset = 0;
        info.flags = 0;

        return muxerDemo->InnerWriteSampleBuffer(trackIndex, data, info);
    }

    int32_t Stop(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->InnerStop();
    }

    int32_t Destroy(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->InnerDestroy();
    }
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001
 * @tc.name      : Create -> SetLocation -> SetRotation -> SetParameter -> AddTrack -> Start ->
 * WriteSampleBuffer -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerDemo, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;
    int32_t trackId;

    ret = SetLocation(muxerDemo);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AV_ERR_OK, ret);

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}