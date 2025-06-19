/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include <fstream>
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include <iostream>
#include <string>
#include "avcodec_errors.h"
#include "media_description.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videodecoder.h"
#include "native_avformat.h"
#include "native_avmagic.h"
#include "native_window.h"
#include "status.h"
#include "unittest_utils.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;

namespace {
struct OH_AVCodecCallback GetVoidCallback()
{
    struct OH_AVCodecCallback cb;
    cb.onError = [](OH_AVCodec *codec, int32_t errorCode, void *userData) {
        (void)codec;
        (void)errorCode;
        (void)userData;
    };
    cb.onStreamChanged = [](OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
        (void)codec;
        (void)format;
        (void)userData;
    };
    cb.onNeedInputBuffer = [](OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
        (void)codec;
        (void)index;
        (void)buffer;
        (void)userData;
    };
    cb.onNewOutputBuffer = [](OH_AVCodec *codec, uint32_t index, OH_AVBuffer *buffer, void *userData) {
        (void)codec;
        (void)index;
        (void)buffer;
        (void)userData;
    };
    return cb;
}

struct OH_AVCodecAsyncCallback GetVoidAsyncCallback()
{
    struct OH_AVCodecAsyncCallback cb;
    cb.onError = [](OH_AVCodec *codec, int32_t errorCode, void *userData) {
        (void)codec;
        (void)errorCode;
        (void)userData;
    };
    cb.onStreamChanged = [](OH_AVCodec *codec, OH_AVFormat *format, void *userData) {
        (void)codec;
        (void)format;
        (void)userData;
    };
    cb.onNeedInputData = [](OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData) {
        (void)codec;
        (void)index;
        (void)data;
        (void)userData;
    };
    cb.onNeedOutputData = [](OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr,
                             void *userData) {
        (void)codec;
        (void)index;
        (void)data;
        (void)attr;
        (void)userData;
    };
    return cb;
}

class VideoStateTest : public testing::TestWithParam<std::string> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);
    OH_AVCodec *videoDec = nullptr;
    OH_AVFormat *format = nullptr;

private:
};
void VideoStateTest::SetUpTestCase(void) {}
void VideoStateTest::TearDownTestCase(void) {}
void VideoStateTest::SetUp(void)
{
    videoDec = OH_VideoDecoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoDec);
    format = OH_AVFormat_Create();
}

void VideoStateTest::TearDown(void)
{
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(videoDec));
    OH_AVFormat_Destroy(format);
}

std::string mimeDec = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
uint32_t DEFAULT_WIDTH = 320;
uint32_t DEFAULT_HEIGHT = 240;
uint32_t DECODER_PIXEL_FORMAT = AV_PIXEL_FORMAT_SURFACE_FORMAT;
string_view outputSurfacePath = "/data/test/media/out_320_240_10s.rgba";

void SetSync0(OH_AVFormat *format)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, 0);
}

void SetSync1(OH_AVFormat *format)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
}

void OnError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    (void)codec;
    (void)errorCode;
    (void)userData;
}

void OnStreamChanged(OH_AVCodec *codec, OH_AVFormat *format, void *userData)
{
    (void)codec;
    (void)userData;
    int32_t width = 0, height = 0;
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_WIDTH, &width);
    OH_AVFormat_GetIntValue(format, OH_MD_KEY_VIDEO_PIC_HEIGHT, &height);
}

void OnNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)userData;
}

void OnNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    (void)codec;
    (void)index;
    (void)data;
    (void)attr;
    (void)userData;
}

int32_t SetCallback(OH_AVCodec *videoDec)
{
    struct OH_AVCodecAsyncCallback callback;
    callback.onError = OnError;
    callback.onStreamChanged = OnStreamChanged;
    callback.onNeedInputData = OnNeedInputData;
    callback.onNeedOutputData = OnNewOutputData;
    return OH_VideoDecoder_SetCallback(videoDec, callback, NULL);
}

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t timestamp_ = 0;
    OHOS::Rect damage_ = {};
    sptr<Surface> cs_ = nullptr;
    std::unique_ptr<std::ofstream> outFile_;
};

TestConsumerListener::TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs_(cs)
{
    outFile_ = std::make_unique<std::ofstream>();
    outFile_->open(name.data(), std::ios::out | std::ios::binary);
}

TestConsumerListener::~TestConsumerListener()
{
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;
    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);
    (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

OH_AVErrCode SetOutputSurface(OH_AVCodec *videoDec)
{
    sptr<Surface> cs = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs, outputSurfacePath);
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    sptr<Surface> ps = Surface::CreateSurfaceAsProducer(p);
    EXPECT_NE(nullptr, ps);
    OHNativeWindow *nativeWindow = CreateNativeWindowFromSurface(&ps);
    return OH_VideoDecoder_SetSurface(videoDec, nativeWindow);
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_001
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_001, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Flush(videoDec));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetOutputSurface(videoDec));

    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_002
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_002, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_003
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_003, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_004
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_004, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_005
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_005, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Flush(videoDec));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_006
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_006, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_007
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_007, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    SetSync1(format);
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_008
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_008, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Initialized_Verify_009
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Initialized_Verify_009, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_001
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_001, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_002
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_002, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_003
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_003, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_004
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_004, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_005
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_005, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_006
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_006, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_007
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_007, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_008
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_008, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_009
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_009, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_010
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsAsync|E_IsSetCb
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_010, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_011
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_011, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_012
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_012, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    SetSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Flush(videoDec));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_013
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_013, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_014
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_014, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_015
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsASync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_015, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_016
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsASync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_016, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_017
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsASync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_017, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_018
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsASync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_018, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_019
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsASync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_019, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_020
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_020, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_021
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_021, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    SetSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Flush(videoDec));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_022
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_022, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Configured_Verify_023
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecoder|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configured_Verify_023, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_001
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_001, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_002
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_002, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_003
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_003, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_004
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_004, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_005
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsBufferMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_005, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(videoDec));
    SetSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoDec));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_006
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsBufferMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_006, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_007
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsBufferMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_007, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_008
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_008, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_009
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_009, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_010
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_010, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_011
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_011, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_012
 * @tc.desc: Decoder state machine verify；
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_012, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_013
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_013, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Start(videoDec));
    SetSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_014
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_014, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_015
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_015, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Running_Verify_016
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Running_Verify_016, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_001
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_001, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_002
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_002, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_003
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_003, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_004
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_004, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_005
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsBufferMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_005, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_006
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsBufferMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_006, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    SetSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoDec));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_007
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsBufferMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_007, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_008
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_008, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_009
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_009, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_010
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_010, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_011
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_011, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_012
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_012, TestSize.Level1)
{
    SetSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_013
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_013, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_014
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_014, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    SetSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    SetSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_Flushed_Verify_015
 * @tc.desc: Decoder state machine verify;
 *           flag:E_IsVideoDecode|E_IsSurfaceMode
 */
HWTEST_F(VideoStateTest, VideoDecoder_Flushed_Verify_015, TestSize.Level1)
{
    SetSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Prepare(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Start(videoDec));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Flush(videoDec));

    EXPECT_EQ(AV_ERR_OK, SetOutputSurface(videoDec));
}

/**
 * @tc.name: VideoDecoder_CreateWithNull_001
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_CreateWithNull_001, TestSize.Level1)
{
    ASSERT_EQ(nullptr, OH_VideoDecoder_CreateByName(""));
}

/**
 * @tc.name: VideoDecoder_CreateWithNull_002
 * @tc.desc: video create
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_CreateWithNull_002, TestSize.Level1)
{
    ASSERT_EQ(nullptr, OH_VideoDecoder_CreateByMime(""));
}

/**
 * @tc.name: VideoDecoder_SetCallback_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_001, TestSize.Level1)
{
    ASSERT_EQ(AV_ERR_OK, SetCallback(videoDec));
    ASSERT_EQ(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_SetCallback_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_002, TestSize.Level1)
{
    OH_AVCodecCallback callback = GetVoidCallback();
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec, callback, NULL));
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec, callback, NULL));
}

/**
 * @tc.name: VideoDecoder_SetCallback_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_003, TestSize.Level1)
{
    OH_AVCodecCallback callback = GetVoidCallback();
    ASSERT_EQ(AV_ERR_OK, SetCallback(videoDec));
    ASSERT_NE(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec, callback, NULL));
}

/**
 * @tc.name: VideoDecoder_SetCallback_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_004, TestSize.Level1)
{
    OH_AVCodecCallback callback = GetVoidCallback();
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec, callback, NULL));
    ASSERT_NE(AV_ERR_OK, SetCallback(videoDec));
}

/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_001
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_Invalid_001, TestSize.Level1)
{
    OH_AVCodecCallback callback = GetVoidCallback();
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(nullptr, callback, nullptr));
}

/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_002
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_Invalid_002, TestSize.Level1)
{
    OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNeedInputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(videoDec, cb, nullptr));
}

/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_003
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_Invalid_003, TestSize.Level1)
{
    OH_AVCodecCallback cb = GetVoidCallback();
    cb.onNewOutputBuffer = nullptr;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(videoDec, cb, nullptr));
}

/**
 * @tc.name: VideoDecoder_SetCallback_Invalid_004
 * @tc.desc: video setcallback
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_SetCallback_Invalid_004, TestSize.Level1)
{
    OH_AVCodecCallback cb = GetVoidCallback();
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RegisterCallback(videoDec, cb, nullptr));
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_001
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_PushInputBuffer_Invalid_001, TestSize.Level1)
{
    struct OH_AVCodecCallback cb = GetVoidCallback();
    OH_AVCodecBufferAttr attr = {0, 0, 0, 0};
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputData(videoDec, 0, attr));
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_002
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_PushInputBuffer_Invalid_002, TestSize.Level1)
{
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(videoDec, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_PushInputBuffer(videoDec, 0));
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_001
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Free_Buffer_Invalid_001, TestSize.Level1)
{
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_FreeOutputData(videoDec, 0));
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_002
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Free_Buffer_Invalid_002, TestSize.Level1)
{
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(videoDec, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_FreeOutputBuffer(videoDec, 0));
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_001
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Render_Buffer_Invalid_001, TestSize.Level1)
{
    struct OH_AVCodecCallback cb = GetVoidCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_RegisterCallback(videoDec, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_RenderOutputData(videoDec, 0));
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_002
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Render_Buffer_Invalid_002, TestSize.Level1)
{
    struct OH_AVCodecAsyncCallback cb = GetVoidAsyncCallback();
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_SetCallback(videoDec, cb, nullptr));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoDecoder_RenderOutputBuffer(videoDec, 0));
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_003
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_PushInputBuffer_Invalid_003, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_PushInputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoDecoder_PushInputBuffer_Invalid_004
 * @tc.desc: video push input buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_PushInputBuffer_Invalid_004, TestSize.Level1)
{
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_PushInputBuffer(videoDec, 0));
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_003
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Free_Buffer_Invalid_003, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_FreeOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoDecoder_Free_Buffer_Invalid_004
 * @tc.desc: video free buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Free_Buffer_Invalid_004, TestSize.Level1)
{
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_FreeOutputBuffer(videoDec, 0));
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_003
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Render_Buffer_Invalid_003, TestSize.Level1)
{
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RenderOutputBuffer(nullptr, 0));
}

/**
 * @tc.name: VideoDecoder_Render_Buffer_Invalid_004
 * @tc.desc: video render buffer
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Render_Buffer_Invalid_004, TestSize.Level1)
{
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_ENCODER;
    EXPECT_EQ(AV_ERR_INVALID_VAL, OH_VideoDecoder_RenderOutputBuffer(videoDec, 0));
    videoDec->magic_ = AVMagic::AVCODEC_MAGIC_VIDEO_DECODER;
}

/**
 * @tc.name: VideoDecoder_Configure_001
 * @tc.desc: correct key input.
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configure_001, TestSize.Level1)
{
    SetSync0(format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_ROTATION_ANGLE.data(), 0); // set rotation_angle 0
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE.data(),
                            15000); // set max input size 15000
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Configure_002
 * @tc.desc: correct key input with redundancy key input
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configure_002, TestSize.Level1)
{
    SetSync0(format);
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_ROTATION_ANGLE.data(), 0); // set rotation_angle 0
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_MAX_INPUT_SIZE.data(),
                            15000);                                                     // set max input size 15000
    OH_AVFormat_SetIntValue(format, MediaDescriptionKey::MD_KEY_AAC_IS_ADTS.data(), 1); // redundancy key
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Configure_003
 * @tc.desc: correct key input with wrong value type input
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configure_003, TestSize.Level1)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, -2);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Configure_004
 * @tc.desc: correct key input with wrong value type input
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configure_004, TestSize.Level1)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, -2);
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Configure_005
 * @tc.desc: empty format input
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Configure_005, TestSize.Level1)
{
    EXPECT_NE(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
}

/**
 * @tc.name: VideoDecoder_Reset_001
 * @tc.desc: correct flow 1
 * @tc.type: FUNC
 */
HWTEST_F(VideoStateTest, VideoDecoder_Reset_001, TestSize.Level1)
{
    SetSync0(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoDecoder_Configure(videoDec, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Reset(videoDec));
}
} // namespace