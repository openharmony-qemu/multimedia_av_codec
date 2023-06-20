/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <string>
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "videoenc_ndk_sample.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"

#define MAX_THREAD 16

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;

namespace OHOS {
namespace Media {
class EncPerfNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();

protected:
    const char *CODEC_NAME_AVC = "OMX.hisi.video.encoder.avc";
    const char *CODEC_NAME_HEVC = "OMX.hisi.video.encoder.hevc";
    const char *INP_DIR_720 = "/data/test/media/1280_720_nv.yuv";
    const char *INP_DIR_1080 = "/data/test/media/1920_1080_nv.yuv";
    const char *INP_DIR_2160 = "/data/test/media/3840_2160_nv.yuv";
};
} // namespace Media
} // namespace OHOS

void EncPerfNdkTest::SetUpTestCase() {}
void EncPerfNdkTest::TearDownTestCase() {}
void EncPerfNdkTest::SetUp() {}
void EncPerfNdkTest::TearDown() {}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0100, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0200, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0300, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0400, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0500, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0600, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0700, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0800, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_0900, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_1000, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_1100, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_BUFFER_1200, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0100, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0200, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0300, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0400, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0500, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0600, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_AVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0700, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0800, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_0900, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 30;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_1000, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_720;
    vEncSample->DEFAULT_WIDTH = 1280;
    vEncSample->DEFAULT_HEIGHT = 720;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 10000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1280_720_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_1100, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_1080;
    vEncSample->DEFAULT_WIDTH = 1920;
    vEncSample->DEFAULT_HEIGHT = 1088;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 20000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/1920_1080_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_SURFACE_1200, TestSize.Level1)
{
    VEncNdkSample *vEncSample = new VEncNdkSample();
    vEncSample->INP_DIR = INP_DIR_2160;
    vEncSample->DEFAULT_WIDTH = 3840;
    vEncSample->DEFAULT_HEIGHT = 2160;
    vEncSample->DEFAULT_FRAME_RATE = 60;
    vEncSample->DEFAULT_BITRATE = 30000000;
    vEncSample->SURFACE_INPUT = true;
    vEncSample->OUT_DIR = "/data/test/media/3840_2160_buffer.h264";
    ASSERT_EQ(AV_ERR_OK, vEncSample->CreateVideoEncoder(CODEC_NAME_HEVC));
    ASSERT_EQ(AV_ERR_OK, vEncSample->SetVideoEncoderCallback());
    ASSERT_EQ(AV_ERR_OK, vEncSample->ConfigureVideoEncoder());
    ASSERT_EQ(AV_ERR_OK, vEncSample->StartVideoEncoder());
    vEncSample->WaitForEOS();
    ASSERT_EQ(AV_ERR_OK, vEncSample->errCount);
}

HWTEST_F(EncPerfNdkTest, VIDEO_ENCODE_FUNCTION_2200, TestSize.Level1)
{
    for (int i = 0; i < 2000; i++) {
        VEncNdkSample *vEncSample = new VEncNdkSample();
        vEncSample->INP_DIR = INP_DIR_1080;
        vEncSample->DEFAULT_WIDTH = 1920;
        vEncSample->DEFAULT_HEIGHT = 1080;
        vEncSample->DEFAULT_FRAME_RATE = 30;
        vEncSample->CreateVideoEncoder(CODEC_NAME_AVC);
        vEncSample->SetVideoEncoderCallback();
        vEncSample->ConfigureVideoEncoder();
        vEncSample->testApi();
        delete vEncSample;
        cout << i << " ";
    }
}
