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

#include <iostream>
#include <cstdio>

#include <atomic>
#include <fstream>
#include <thread>
#include <mutex>
#include <queue>
#include <string>

#include "gtest/gtest.h"
#include "videodec_ndk_inner_sample.h"
#include "avcodec_video_decoder.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
class SwdecInnerApiNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void) override;
    // TearDown: Called after each test cases
    void TearDown(void) override;
};

std::shared_ptr<AVCodecVideoDecoder> vdec_ = nullptr;
std::shared_ptr<VDecInnerSignal> signal_ = nullptr;
std::string g_invalidCodecMime = "avdec_h264";
std::string g_codecMime = "video/avc";
std::string g_codecName = "OH.Media.Codec.Decoder.Video.AVC";
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
constexpr uint32_t DEFAULT_FRAME_RATE = 30;

void SwdecInnerApiNdkTest::SetUpTestCase() {}
void SwdecInnerApiNdkTest::TearDownTestCase() {}
void SwdecInnerApiNdkTest::SetUp()
{
    signal_ = make_shared<VDecInnerSignal>();
}

void SwdecInnerApiNdkTest::TearDown()
{
    if (signal_) {
        signal_ = nullptr;
    }
    
    if (vdec_ != nullptr) {
        vdec_->Release();
        vdec_ = nullptr;
    }
}
} // namespace

namespace {
/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0100
 * @tc.name      : CreateByMime para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0100, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByMime("");
    ASSERT_EQ(nullptr, vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0200
 * @tc.name      : CreateByName para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0200, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName("");
    ASSERT_EQ(nullptr, vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0300
 * @tc.name      : SetCallback para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0300, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    std::shared_ptr<VDecInnerCallback> cb_ = make_unique<VDecInnerCallback>(nullptr);
    int32_t ret = vdec_->SetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0400
 * @tc.name      : SetCallback para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0400, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    std::shared_ptr<VDecInnerCallback> cb_ = nullptr;
    int32_t ret = vdec_->SetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0500
 * @tc.name      : SetOutputSurface para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0500, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);
    ASSERT_EQ(AVCS_ERR_NO_MEMORY, vdec_->SetOutputSurface(nullptr));
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0600
 * @tc.name      : CreateByName para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0600, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_invalidCodecMime);
    ASSERT_EQ(nullptr, vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0700
 * @tc.name      : CreateByMime para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0700, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByMime(g_invalidCodecMime);
    ASSERT_EQ(nullptr, vdec_);
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0800
 * @tc.name      : ReleaseOutputBuffer para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0800, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->ReleaseOutputBuffer(0, true));
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_0900
 * @tc.name      : ReleaseOutputBuffer para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_0900, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->ReleaseOutputBuffer(0, false));
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_1000
 * @tc.name      : ReleaseOutputBuffer para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_1000, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->ReleaseOutputBuffer(-1, false));
}

/**
 * @tc.number    : VIDEO_SWDEC_ILLEGAL_PARA_1100
 * @tc.name      : QueueInputBuffer para error
 * @tc.desc      : param test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_ILLEGAL_PARA_1100, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    AVCodecBufferInfo info;
    info.presentationTimeUs = -1;
    info.size = -1;
    info.offset = -1;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->QueueInputBuffer(0, info, flag));
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0100
 * @tc.name      : repeat CreateByName CreateByName
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0100, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    std::shared_ptr<AVCodecVideoDecoder> vdec_2 = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_2);
    ASSERT_EQ(AVCS_ERR_OK, vdec_2->Release());
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0200
 * @tc.name      : create configure configure
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0200, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_OK, vdec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->Configure(format));
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0300
 * @tc.name      : create configure start start
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0300, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_OK, vdec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Start());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->Start());
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0400
 * @tc.name      : create configure start stop stop
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0400, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_OK, vdec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Start());
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Stop());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->Stop());
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0500
 * @tc.name      : create configure start stop reset reset
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0500, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_OK, vdec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Start());
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Stop());
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Reset());
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Reset());
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0600
 * @tc.name      : create configure start EOS EOS
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0600, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_OK, vdec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Start());

    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 0;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->QueueInputBuffer(0, info, flag));
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->QueueInputBuffer(0, info, flag));
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0700
 * @tc.name      : create configure start flush flush
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0700, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_OK, vdec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Start());
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Flush());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->Flush());
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0800
 * @tc.name      : create configure start stop release
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0800, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_OK, vdec_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Start());
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Stop());
    ASSERT_EQ(AVCS_ERR_OK, vdec_->Release());
    vdec_ = nullptr;
}

/**
 * @tc.number    : VIDEO_SWDEC_API_0900
 * @tc.name      : create create
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_0900, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, vdec_);

    std::shared_ptr<AVCodecVideoDecoder> vdec_2 = VideoDecoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, vdec_2);
    ASSERT_EQ(AVCS_ERR_OK, vdec_2->Release());
    vdec_2 = nullptr;
}

/**
 * @tc.number    : VIDEO_SWDEC_API_1000
 * @tc.name      : repeat SetCallback
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_1000, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    std::shared_ptr<VDecInnerCallback> cb_ = make_unique<VDecInnerCallback>(nullptr);
    ASSERT_EQ(AVCS_ERR_OK, vdec_->SetCallback(cb_));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->SetCallback(cb_));
}

/**
 * @tc.number    : VIDEO_SWDEC_API_1100
 * @tc.name      : repeat GetOutputFormat
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_1100, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    ASSERT_EQ(AVCS_ERR_OK, vdec_->GetOutputFormat(format));
    ASSERT_EQ(AVCS_ERR_OK, vdec_->GetOutputFormat(format));
}

/**
 * @tc.number    : VIDEO_SWDEC_API_1200
 * @tc.name      : repeat SetParameter
 * @tc.desc      : api test
 */
HWTEST_F(SwdecInnerApiNdkTest, VIDEO_SWDEC_API_1200, TestSize.Level2)
{
    vdec_ = VideoDecoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, vdec_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, DEFAULT_WIDTH);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, DEFAULT_HEIGHT);
    format.PutDoubleValue(MediaDescriptionKey::MD_KEY_FRAME_RATE, DEFAULT_FRAME_RATE);

    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->SetParameter(format));
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, vdec_->SetParameter(format));
}
} // namespace