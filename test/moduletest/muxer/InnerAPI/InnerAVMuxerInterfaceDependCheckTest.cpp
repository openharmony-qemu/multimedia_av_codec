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
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::Media;
constexpr uint32_t SAMPLE_RATE_48000 = 48000;
constexpr uint32_t Buffer_Size = 100;
constexpr uint32_t BITRATE_320000 = 320000;


namespace {
    class InnerAVMuxerInterfaceDependCheckTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void InnerAVMuxerInterfaceDependCheckTest::SetUpTestCase() {}
    void InnerAVMuxerInterfaceDependCheckTest::TearDownTestCase() {}
    void InnerAVMuxerInterfaceDependCheckTest::SetUp() {}
    void InnerAVMuxerInterfaceDependCheckTest::TearDown() {}

    int32_t Create(AVMuxerDemo* muxerDemo)
    {
        Plugins::OutputFormat format = Plugins::OutputFormat::MPEG_4;
        int32_t fd = muxerDemo->InnerGetFdByMode(format);
        return muxerDemo->InnerCreate(fd, format);
    }

    int32_t SetRotation(AVMuxerDemo* muxerDemo)
    {
        int32_t rotation = 0;

        return muxerDemo->InnerSetRotation(rotation);
    }

    int32_t AddTrack(AVMuxerDemo* muxerDemo)
    {
        constexpr int32_t size = 100;
        std::shared_ptr<Meta> audioParams = std::make_shared<Meta>();
        std::vector<uint8_t> a(size);
        int32_t trackIndex = 0;

        audioParams->Set<Tag::MIME_TYPE>(Plugins::MimeType::AUDIO_AAC);
        audioParams->Set<Tag::MEDIA_BITRATE>(BITRATE_320000);
        audioParams->Set<Tag::MEDIA_CODEC_CONFIG>(a);
        audioParams->Set<Tag::AUDIO_CHANNEL_COUNT>(1);
        audioParams->Set<Tag::AUDIO_SAMPLE_RATE>(SAMPLE_RATE_48000);

        return muxerDemo->InnerAddTrack(trackIndex, audioParams);
    }

    int32_t Start(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->InnerStart();
    }

    int32_t WriteSample(AVMuxerDemo* muxerDemo, uint32_t trackIndex)
    {
        uint8_t data[Buffer_Size];
        std::shared_ptr<AVBuffer> avMemBuffer = AVBuffer::CreateAVBuffer(data, Buffer_Size, Buffer_Size);
        avMemBuffer->pts_ = 0;
        avMemBuffer->flag_ = static_cast<uint32_t>(Plugins::AVBufferFlag::SYNC_FRAME);
        return muxerDemo->InnerWriteSample(trackIndex, avMemBuffer);
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
 * @tc.name      : Create -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_002
 * @tc.name      : Create -> SetRotation -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_002, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_003
 * @tc.name      : Create -> AddTrack -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_004
 * @tc.name      : Create -> AddTrack -> Start -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_005
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_006
 * @tc.name      : Create -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_007
 * @tc.name      : Create -> AddTrack -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_008
 * @tc.name      : Create -> SetRotation -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_009
 * @tc.name      : Create -> AddTrack -> Start -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, trackId);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_010
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_010, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, trackId);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_011
 * @tc.name      : Create -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_011, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_012
 * @tc.name      : Create -> AddTrack -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_012, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_013
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_013, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_014
 * @tc.name      : Create -> AddTrack -> Start -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_014, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_015
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_015, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_016
 * @tc.name      : Create -> WriteSample
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_016, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;
    int32_t trackId = -1;

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_017
 * @tc.name      : Create -> AddTrack -> WriteSample
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_017, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;
    int32_t trackId = -1;

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_018
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_018, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;
    int32_t trackId = -1;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_019
 * @tc.name      : Create -> AddTrack -> Start -> WriteSample
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_019, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;
    int32_t trackId = -1;

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_020
 * @tc.name      : Create -> AddTrack -> Start -> WriteSample -> WriteSample
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_020, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;
    int32_t trackId = -1;

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_021
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> WriteSample
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_021, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;
    int32_t trackId = -1;

    trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_022
 * @tc.name      : Create -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_022, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_023
 * @tc.name      : Create -> AddTrack -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_023, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_024
 * @tc.name      : Create -> AddTrack -> Start -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_024, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_025
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_025, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_026
 * @tc.name      : Create -> AddTrack -> Start -> WriteSample -> Stop -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_026, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_027
 * @tc.name      : Create -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_027, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_028
 * @tc.name      : Create -> SetRotation -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_028, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_029
 * @tc.name      : Create -> AddTrack -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_029, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_030
 * @tc.name      : Create -> AddTrack -> Start -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_030, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_031
 * @tc.name      : Create -> AddTrack -> Start -> WriteSample -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_031, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_032
 * @tc.name      : Create -> AddTrack -> Start -> WriteSample -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_032, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_033
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> WriteSample -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(InnerAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_033, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    int32_t ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    int32_t trackId = AddTrack(muxerDemo);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = WriteSample(muxerDemo, trackId);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    delete muxerDemo;
}