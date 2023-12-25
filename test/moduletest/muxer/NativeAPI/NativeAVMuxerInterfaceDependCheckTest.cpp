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
    class NativeAVMuxerInterfaceDependCheckTest : public testing::Test {
    public:
        static void SetUpTestCase();
        static void TearDownTestCase();
        void SetUp() override;
        void TearDown() override;
    };

    void NativeAVMuxerInterfaceDependCheckTest::SetUpTestCase() {}
    void NativeAVMuxerInterfaceDependCheckTest::TearDownTestCase() {}
    void NativeAVMuxerInterfaceDependCheckTest::SetUp() {}
    void NativeAVMuxerInterfaceDependCheckTest::TearDown() {}

    constexpr int64_t BITRATE = 32000;
    constexpr int32_t CODEC_CONFIG = 100;
    constexpr int32_t CHANNEL_COUNT = 1;
    constexpr int32_t SAMPLE_RATE = 48000;
    constexpr int32_t PROFILE = 0;
    constexpr int32_t INFO_SIZE = 100;

    OH_AVMuxer* Create(AVMuxerDemo* muxerDemo)
    {
        OH_AVOutputFormat format = AV_OUTPUT_FORMAT_MPEG_4;
        OH_AVMuxer* handle = nullptr;
        int32_t fd = muxerDemo->GetFdByMode(format);
        handle = muxerDemo->NativeCreate(fd, format);

        return handle;
    }

    OH_AVErrCode SetRotation(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
    {
        int32_t rotation = 0;

        OH_AVErrCode ret = muxerDemo->NativeSetRotation(handle, rotation);

        return ret;
    }

    OH_AVErrCode AddTrack(AVMuxerDemo* muxerDemo, int32_t* trackId, OH_AVMuxer* handle)
    {
        uint8_t a[100];

        OH_AVFormat* trackFormat = OH_AVFormat_Create();
        OH_AVFormat_SetStringValue(trackFormat, OH_MD_KEY_CODEC_MIME, OH_AVCODEC_MIMETYPE_AUDIO_AAC);
        OH_AVFormat_SetLongValue(trackFormat, OH_MD_KEY_BITRATE, BITRATE);
        OH_AVFormat_SetBuffer(trackFormat, OH_MD_KEY_CODEC_CONFIG, a, CODEC_CONFIG);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_CHANNEL_COUNT, CHANNEL_COUNT);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_AUD_SAMPLE_RATE, SAMPLE_RATE);
        OH_AVFormat_SetIntValue(trackFormat, OH_MD_KEY_PROFILE, PROFILE);

        OH_AVErrCode ret = muxerDemo->NativeAddTrack(handle, trackId, trackFormat);
        OH_AVFormat_Destroy(trackFormat);
        return ret;
    }

    OH_AVErrCode Start(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
    {
        OH_AVErrCode ret = muxerDemo->NativeStart(handle);

        return ret;
    }

    OH_AVErrCode WriteSampleBuffer(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle, uint32_t trackIndex)
    {
        OH_AVMemory* avMemBuffer = OH_AVMemory_Create(100);

        OH_AVCodecBufferAttr info;
        info.size = INFO_SIZE;
        info.pts = 0;
        info.offset = 0;
        info.flags = 0;

        OH_AVErrCode ret = muxerDemo->NativeWriteSampleBuffer(handle, trackIndex, avMemBuffer, info);
        OH_AVMemory_Destroy(avMemBuffer);
        return ret;
    }

    OH_AVErrCode Stop(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
    {
        OH_AVErrCode ret = muxerDemo->NativeStop(handle);

        return ret;
    }

    OH_AVErrCode Destroy(AVMuxerDemo* muxerDemo, OH_AVMuxer* handle)
    {
        OH_AVErrCode ret = muxerDemo->NativeDestroy(handle);

        return ret;
    }
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001
 * @tc.name      : Create -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_001, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_002
 * @tc.name      : Create -> SetRotation -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_002, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_003
 * @tc.name      : Create -> AddTrack -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_003, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_004
 * @tc.name      : Create -> AddTrack -> Start -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_004, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_005
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> SetRotation
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_005, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_006
 * @tc.name      : Create -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_006, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    int32_t trackId;
    OH_AVErrCode ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_007
 * @tc.name      : Create -> AddTrack -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_007, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    int32_t trackId;
    OH_AVErrCode ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(0, trackId);

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(1, trackId);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_008
 * @tc.name      : Create -> SetRotation -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_008, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_009
 * @tc.name      : Create -> AddTrack -> Start -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_009, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_010
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> AddTrack
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_010, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_011
 * @tc.name      : Create -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_011, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_012
 * @tc.name      : Create -> AddTrack -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_012, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_013
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_013, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_014
 * @tc.name      : Create -> AddTrack -> Start -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_014, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_015
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> Start
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_015, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_016
 * @tc.name      : Create -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_016, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_017
 * @tc.name      : Create -> AddTrack -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_017, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_018
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_018, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_019
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_019, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_020
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_020, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_021
 * @tc.name      : Create -> AddTrack -> Start -> Stop -> WriteSampleBuffer
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_021, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_022
 * @tc.name      : Create -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_022, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_023
 * @tc.name      : Create -> AddTrack -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_023, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_024
 * @tc.name      : Create -> AddTrack -> Start -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_024, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_025
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_025, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    int32_t trackId = -1;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_026
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> Stop -> Stop
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_026, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    int32_t trackId = -1;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OPERATE_NOT_PERMIT, ret);

    Destroy(muxerDemo, handle);
    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_027
 * @tc.name      : Create -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_027, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = Destroy(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_028
 * @tc.name      : Create -> SetRotation -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_028, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Destroy(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_029
 * @tc.name      : Create -> AddTrack -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_029, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Destroy(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_030
 * @tc.name      : Create -> AddTrack -> Start -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_030, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Destroy(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_031
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_031, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Destroy(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_032
 * @tc.name      : Create -> AddTrack -> Start -> WriteSampleBuffer -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_032, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;
    int32_t trackId = -1;

    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Destroy(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}


/**
 * @tc.number    : SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_033
 * @tc.name      : Create -> SetRotation -> AddTrack -> Start -> WriteSampleBuffer -> Stop -> Destroy
 * @tc.desc      : interface depend check
 */
HWTEST_F(NativeAVMuxerInterfaceDependCheckTest, SUB_MULTIMEDIA_MEDIA_MUXER_INTERFACE_DEPEND_CHECK_033, TestSize.Level2)
{
    AVMuxerDemo* muxerDemo = new AVMuxerDemo();
    OH_AVMuxer* handle = Create(muxerDemo);
    ASSERT_NE(nullptr, handle);

    OH_AVErrCode ret;

    ret = SetRotation(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    int32_t trackId = -1;
    ret = AddTrack(muxerDemo, &trackId, handle);
    ASSERT_EQ(0, trackId);

    ret = Start(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = WriteSampleBuffer(muxerDemo, handle, trackId);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Stop(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    ret = Destroy(muxerDemo, handle);
    ASSERT_EQ(AV_ERR_OK, ret);

    delete muxerDemo;
}