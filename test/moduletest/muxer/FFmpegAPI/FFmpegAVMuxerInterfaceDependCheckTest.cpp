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
#include "avcodec_info.h"
#include "avcodec_errors.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace Plugin;
using namespace Ffmpeg;


namespace {
    class FFmpegAVMuxerInterfaceDependCheckTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void FFmpegAVMuxerInterfaceDependCheckTest::SetUpTestCase() {}
    void FFmpegAVMuxerInterfaceDependCheckTest::TearDownTestCase() {}
    void FFmpegAVMuxerInterfaceDependCheckTest::SetUp() {}
    void FFmpegAVMuxerInterfaceDependCheckTest::TearDown() {}

    void Create(AVMuxerDemo* muxerDemo)
    {
        OutputFormat format = OUTPUT_FORMAT_MPEG_4;
        int32_t fd = muxerDemo->FFmpeggetFdByMode(format);
        muxerDemo->FFmpegCreate(fd);
    }

    Status SetRotation(AVMuxerDemo* muxerDemo)
    {
        int32_t rotation = 0;

        return muxerDemo->FFmpegSetRotation(rotation);
    }

    Status AddTrack(AVMuxerDemo* muxerDemo, int32_t& trackIndex)
    {
        MediaDescription audioParams;
        uint8_t a[100];
        uint32_t CHANNEL_COUNT = 1;
        uint32_t SAMPLE_RATE = 48000;
        uint32_t BITS_RATE = 320000;
        uint32_t CODEC_CONFIG_SIZE = 100;
        audioParams.PutStringValue(MediaDescriptionKey::MD_KEY_CODEC_MIME, CodecMimeType::AUDIO_AAC);
        audioParams.PutLongValue(MediaDescriptionKey::MD_KEY_BITRATE, BITS_RATE);
        audioParams.PutBuffer(MediaDescriptionKey::MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG_SIZE);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_CHANNEL_COUNT, CHANNEL_COUNT);
        audioParams.PutIntValue(MediaDescriptionKey::MD_KEY_SAMPLE_RATE, SAMPLE_RATE);

        return muxerDemo->FFmpegAddTrack(trackIndex, audioParams);
    }

    Status Start(AVMuxerDemo* muxerDemo)
    {
        return muxerDemo->FFmpegStart();
    }

    Status WriteSampleBuffer(AVMuxerDemo* muxerDemo, uint32_t trackIndex)
    {
        uint8_t data[100];

        AVCodecBufferInfo info;
        AVCodecBufferFlag flag;
        uint32_t DEAULT_SIZE = 100;
        info.size = DEAULT_SIZE;
        info.presentationTimeUs = 0;
        flag = AVCODEC_BUFFER_FLAG_NONE;
        info.offset = 0;

        return muxerDemo->FFmpegWriteSample(trackIndex, data, info, flag);
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
 * @tc.name      : Create -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_002
 * @tc.name      : Create -> SetRotation -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_002, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_003
 * @tc.name      : Create -> AddTrack -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_004
 * @tc.name      : Create -> AddTrack -> Start -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_005
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::ERROR_MISMATCHED_TYPE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_006
 * @tc.name      : Create -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);


    Destroy(muxerDemo);
    delete muxerDemo;
}
/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_007
 * @tc.name      : Create -> AddTrack -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);


    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_008
 * @tc.name      : Create -> SetRotation -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_009
 * @tc.name      : Create -> AddTrack -> Start -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_010
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_010, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
 
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_011
 * @tc.name      : Create -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_011, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::ERROR_UNKNOWN, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_012
 * @tc.name      : Create -> AddTrack -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_012, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_013
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_013, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_014
 * @tc.name      : Create -> AddTrack -> Start -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_014, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_015
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_015, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_016
 * @tc.name      : Create -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_016, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;
    int32_t trackId = -1;

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_017
 * @tc.name      : Create -> AddTrack -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_017, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;
    int32_t trackId = -1;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_018
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_018, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;
    int32_t trackId = -1;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_019
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_019, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;
    int32_t trackId = -1;


    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}

/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_020
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_020, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_021
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_021, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;
    int32_t trackId = -1;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_022
 * @tc.name      : Create -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_022, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    

    Status ret;

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_023
 * @tc.name      : Create -> AddTrack -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_023, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    
    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_024
 * @tc.name      : Create -> AddTrack -> Start -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_024, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_025
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_025, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_026
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> Stop -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_026, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::ERROR_WRONG_STATE, ret);

    Destroy(muxerDemo);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_027
 * @tc.name      : Create -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_027, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    ret = Destroy(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_028
 * @tc.name      : Create -> SetRotation -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_028, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);
    

    Status ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_029
 * @tc.name      : Create -> AddTrack -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_029, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_030
 * @tc.name      : Create -> AddTrack -> Start -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_030, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_031
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_031, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_032
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_032, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_033
 * @tc.name      : Create -> SetRotation ->  AddTrack -> Start -> WriteSampleBuffer -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(FFmpegAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_033, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    Create(muxerDemo);

    Status ret;

    ret = SetRotation(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    int32_t trackId;
    ret = AddTrack(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = WriteSampleBuffer(muxerDemo, trackId);
    ASSERT_EQ(Status::OK, ret);

    ret = Stop(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    ret = Destroy(muxerDemo);
    ASSERT_EQ(Status::OK, ret);

    delete muxerDemo;
}