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
#include <gtest/gtest.h>
#include <gtest/hwext/gtest-multithread.h>
#include "unittest_utils.h"
#include "status.h"
#include "native_avcodec_base.h"
#include "native_avcodec_videoencoder.h"
#include "native_avformat.h"
#include "native_window.h"
#include <fstream>
#include <iostream>
#include <string>
#include "avcodec_errors.h"


using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;
using namespace testing::mt;

namespace {
class VideoStateTest : public testing::TestWithParam<std::string> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

private:  
};
void VideoStateTest::SetUpTestCase(void) {}
void VideoStateTest::TearDownTestCase(void) {}
void VideoStateTest::SetUp(void) {}
void VideoStateTest::TearDown(void) {}
std::string mimeDec = OH_AVCODEC_MIMETYPE_VIDEO_AVC;
uint32_t DEFAULT_WIDTH = 320;
uint32_t DEFAULT_HEIGHT = 240;
uint32_t DECODER_PIXEL_FORMAT = AV_PIXEL_FORMAT_SURFACE_FORMAT;
//string_view outputSurfacePath = "/data/test/media/out_320_240_10s.rgba";

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

int32_t SetCallback(OH_AVCodec *videoEnc)
{
    struct OH_AVCodecAsyncCallback callback;
    callback.onError = OnError;
    callback.onStreamChanged = OnStreamChanged;
    callback.onNeedInputData = OnNeedInputData;
    callback.onNeedOutputData = OnNewOutputData;
    return OH_VideoEncoder_SetCallback(videoEnc, callback, NULL);
}

void OnNeedInputParameter(OH_AVCodec *codec, uint32_t index, OH_AVFormat *parameter, void *userData)
{
    (void)codec;
    (void)index;
    (void)userData;
    OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_QP_MAX, 30);
    OH_AVFormat_SetIntValue(parameter, OH_MD_KEY_VIDEO_ENCODER_QP_MIN, 20);
}

int32_t SetParameterCallback(OH_AVCodec *videoEnc)
{
    OH_VideoEncoder_OnNeedInputParameter callback = OnNeedInputParameter;
    return OH_VideoEncoder_RegisterParameterCallback(videoEnc, callback, NULL);
}

//异步
void SetSync0(OH_AVFormat * format)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, 0);
}

//同步
void SetSync1(OH_AVFormat * format)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, DEFAULT_WIDTH);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, DECODER_PIXEL_FORMAT);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_ENABLE_SYNC_MODE, 1);
}

void SetParamSync0(OH_AVFormat * format)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_INPUT_PARAMETER_SYNC_MODE, 0);  
}

void SetParamSync1(OH_AVFormat * format)
{
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_VIDEO_ENCODER_ENABLE_INPUT_PARAMETER_SYNC_MODE, 1);  
}


int32_t GetInputSurface(OH_AVCodec *videoEnc)
{
    OHNativeWindow *nativeWindow;
    return OH_VideoEncoder_GetSurface(videoEnc, &nativeWindow);
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_001
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_001, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    ASSERT_EQ(AV_ERR_OK,OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_002
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_002, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_003
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_003, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_004
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_004, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_005
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_005, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_006
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_006, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_007
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_007, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_008
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_008, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_009
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_009, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);

    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_010
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_SetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_010, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_011
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_SetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_011, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_012
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_SetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_012, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_013
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_SetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_013, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_014
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_SetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_014, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_015
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_015, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc)); 
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_016
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_016, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_017
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_017, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));

    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_018
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_018, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_019
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_019, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Initialized_Verify_020
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Initialized_Verify_020, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_001
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_001, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_002
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_002, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_003
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_003, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_004
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_004, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_005
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_005, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_006
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_006, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_007
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_007, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_008
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_008, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_009
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_009, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_010
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_010, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_011
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_011, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_012
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetCb
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_012, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_013
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_013, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_014
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_014, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_015
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_015, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_016
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_016, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_017
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_017, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_018
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_018, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));    
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_019
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_019, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_020
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_020, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_021
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_021, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_022
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_022, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_023
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_023, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_024
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_024, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_025
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_025, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));

    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_026
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_026, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_027
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_027, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_028
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_028, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_029
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_029, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_030
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_030, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_031
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_031, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_032
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_032, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_033
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_033, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_034
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_034, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_035
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_035, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_036
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_036, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_037
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_037, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_038
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_038, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_039
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_039, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_040
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_040, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_041
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_041, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_042
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_042, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_043
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_043, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_044
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_044, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_045
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_045, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_046
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_046, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Configured_Verify_047
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Configured_Verify_047, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));

    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_001
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_001, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_002
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_002, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_003
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_003, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_004
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_004, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_005
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_005, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_006
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsBufferMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_006, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_007
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsBufferMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_007, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_008
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsBufferMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_008, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_009
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_009, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_010
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_010, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_011
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_011, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_012
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_012, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_013
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_013, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_014
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_014, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_015
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_015, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_016
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_016, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_017
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_017, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_018
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_018, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_019
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_019, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Running_Verify_020
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Running_Verify_020, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));

    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(videoEnc));
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_001
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_001, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_002
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_002, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_003
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_003, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_004
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_004, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_005
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsBufferMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_005, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_006
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsBufferMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_006, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
} 
    
/**.
* @tc.name: VideoEncoder_Flushed_Verify_007
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsBufferMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_007, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_008
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsBufferMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_008, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_009
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsBufferMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_009, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_010
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_010, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_011
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_011, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));   

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_012
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_012, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc)); 

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_013
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_013, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc)); 

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_014
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_014, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc)); 

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_015
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_015, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_016
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_016, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_017
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_017, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_018
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_018, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_019
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsAsync|E_IsSetParamCb|E_IsSurfaceMode(|E_IsSetCb)
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_019, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    EXPECT_EQ(AV_ERR_OK, SetParameterCallback(videoEnc));
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, SetCallback(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_020
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_020, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_021
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_021, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_022
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_022, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_023
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_023, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_024
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_024, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}

/**.
* @tc.name: VideoEncoder_Flushed_Verify_025
* @tc.desc: Encoder state machine verify;
*           flag:E_IsVideoEncoder|E_IsParamSync|E_IsSurfaceMode
*/
HWTEST_F(VideoStateTest, VideoEncoder_Flushed_Verify_025, TestSize.Level1)
{
    OH_AVCodec *videoEnc = OH_VideoEncoder_CreateByMime(OH_AVCODEC_MIMETYPE_VIDEO_AVC);
    ASSERT_NE(nullptr, videoEnc);
    OH_AVFormat *format = OH_AVFormat_Create();
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OK, GetInputSurface(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Prepare(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(videoEnc));
    EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(videoEnc));

    SetSync0(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync0(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync0(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    SetSync1(format);
    SetParamSync1(format);
    EXPECT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(videoEnc, format));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, SetCallback(videoEnc));
    EXPECT_EQ(AV_ERR_INVALID_STATE, SetParameterCallback(videoEnc));
    EXPECT_EQ(AV_ERR_OPERATE_NOT_PERMIT, GetInputSurface(videoEnc));
    OH_AVFormat_Destroy(format);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(videoEnc));
}
}