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
#include <string>

#include "gtest/gtest.h"
#include "avcodec_common.h"
#include "meta/format.h"
#include "avcodec_video_encoder.h"
#include "videoenc_ndk_inner_sample.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
class HwEncInnerApiNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp() override;
    // TearDown: Called after each test cases
    void TearDown() override;
};

constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
std::string g_codecMime = "video/avc";
std::string g_codecName = "";
std::shared_ptr<AVCodecVideoEncoder> venc_ = nullptr;
std::shared_ptr<VEncInnerSignal> signal_ = nullptr;

void HwEncInnerApiNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(g_codecMime.c_str(), true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    g_codecName = tmpCodecName;
    cout << "g_codecName: " << g_codecName << endl;
}

void HwEncInnerApiNdkTest::TearDownTestCase() {}

void HwEncInnerApiNdkTest::SetUp()
{
    signal_ = make_shared<VEncInnerSignal>();
}

void HwEncInnerApiNdkTest::TearDown()
{
    if (venc_ != nullptr) {
        venc_->Release();
        venc_ = nullptr;
    }

    if (signal_) {
        signal_ = nullptr;
    }
}
} // namespace

namespace {
/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0100
 * @tc.name      : CreateByMime para1 error
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0100, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByMime("");
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0200
 * @tc.name      : CreateByMime para2 error
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0200, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByMime("");
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0300
 * @tc.name      : CreateByName para1 error
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0300, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName("");
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0400
 * @tc.name      : CreateByName para2 error
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0400, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName("");
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0500
 * @tc.name      : SetCallback para error
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0500, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);

    std::shared_ptr<VEncInnerCallback> cb_ = make_shared<VEncInnerCallback>(nullptr);
    int32_t ret = venc_->SetCallback(cb_);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0600
 * @tc.name      : Configure para not enough
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0600, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_BITRATE, 100000);
    int32_t ret = venc_->Configure(format);
    ASSERT_EQ(ret, AVCS_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0700
 * @tc.name      : ReleaseOutputBuffer para error
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0700, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);

    Format format;
    format.PutIntValue(MediaDescriptionKey::MD_KEY_HEIGHT, 1080);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_WIDTH, 1080);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::YUVI420));

    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Start());
    usleep(1000000);
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->ReleaseOutputBuffer(9999999));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0800
 * @tc.name      : QueueInputBuffer para error
 * @tc.desc      : param test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0800, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);

    AVCodecBufferInfo info;
    info.presentationTimeUs = -1;
    info.size = -1;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->QueueInputBuffer(0, info, flag));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0100
 * @tc.name      : CreateByName CreateByName
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0100, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    std::shared_ptr<AVCodecVideoEncoder> venc2_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);
    ASSERT_EQ(AVCS_ERR_OK, venc2_->Release());
    venc2_ = nullptr;
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0200
 * @tc.name      : CreateByName configure configure
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0200, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->Configure(format));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0300
 * @tc.name      : CreateByName configure start start
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0300, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Start());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->Start());
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0400
 * @tc.name      : CreateByName configure start stop stop
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0400, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Start());
    ASSERT_EQ(AVCS_ERR_OK, venc_->Stop());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->Stop());
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0500
 * @tc.name      : CreateByName configure start stop reset reset
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0500, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Start());
    ASSERT_EQ(AVCS_ERR_OK, venc_->Stop());
    ASSERT_EQ(AVCS_ERR_OK, venc_->Reset());
    ASSERT_EQ(AVCS_ERR_OK, venc_->Reset());
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0600
 * @tc.name      : CreateByName configure start EOS EOS
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0600, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));

    std::shared_ptr<VEncInnerCallback> cb_ = make_shared<VEncInnerCallback>(signal_);
    ASSERT_EQ(AVCS_ERR_OK, venc_->SetCallback(cb_));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Start());

    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [] { return signal_->inIdxQueue_.size() > 1; });
    uint32_t index = signal_->inIdxQueue_.front();
    AVCodecBufferInfo info;
    info.presentationTimeUs = 0;
    info.size = 0;
    info.offset = 0;
    AVCodecBufferFlag flag = AVCODEC_BUFFER_FLAG_EOS;

    ASSERT_EQ(AVCS_ERR_OK, venc_->QueueInputBuffer(index, info, flag));
    signal_->inIdxQueue_.pop();
    index = signal_->inIdxQueue_.front();
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->QueueInputBuffer(index, info, flag));
    signal_->inIdxQueue_.pop();
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0700
 * @tc.name      : CreateByName configure start flush flush
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0700, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Start());
    ASSERT_EQ(AVCS_ERR_OK, venc_->Flush());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->Flush());
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0800
 * @tc.name      : CreateByName configure start stop release
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0800, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    ASSERT_EQ(AVCS_ERR_OK, venc_->Configure(format));
    ASSERT_EQ(AVCS_ERR_OK, venc_->Start());
    ASSERT_EQ(AVCS_ERR_OK, venc_->Stop());
    ASSERT_EQ(AVCS_ERR_OK, venc_->Release());
    venc_ = nullptr;
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0900
 * @tc.name      : CreateByMime CreateByMime
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_0900, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);

    std::shared_ptr<AVCodecVideoEncoder> venc2_ = VideoEncoderFactory::CreateByMime(g_codecMime);
    ASSERT_NE(nullptr, venc_);
    
    ASSERT_EQ(AVCS_ERR_OK, venc2_->Release());
    venc2_ = nullptr;
}

/**
 * @tc.number    : VIDEO_ENCODE_API_1000
 * @tc.name      : repeat SetCallback
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_1000, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    std::shared_ptr<VEncInnerCallback> cb_ = make_shared<VEncInnerCallback>(nullptr);
    ASSERT_EQ(AVCS_ERR_OK, venc_->SetCallback(cb_));
    ASSERT_EQ(AVCS_ERR_OK, venc_->SetCallback(cb_));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_1100
 * @tc.name      : repeat GetOutputFormat
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_1100, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    ASSERT_EQ(AVCS_ERR_OK, venc_->GetOutputFormat(format));
    ASSERT_EQ(AVCS_ERR_OK, venc_->GetOutputFormat(format));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_1200
 * @tc.name      : repeat SetParameter
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_1200, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    format.PutIntValue(widthStr.c_str(), DEFAULT_WIDTH);
    format.PutIntValue(heightStr.c_str(), DEFAULT_HEIGHT);
    format.PutIntValue(MediaDescriptionKey::MD_KEY_PIXEL_FORMAT, static_cast<int32_t>(VideoPixelFormat::NV12));

    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->SetParameter(format));
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, venc_->SetParameter(format));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_1300
 * @tc.name      : repeat GetInputFormat
 * @tc.desc      : api test
 */
HWTEST_F(HwEncInnerApiNdkTest, VIDEO_ENCODE_API_1300, TestSize.Level2)
{
    venc_ = VideoEncoderFactory::CreateByName(g_codecName);
    ASSERT_NE(nullptr, venc_);

    Format format;
    int32_t ret = venc_->GetInputFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);

    ret = venc_->GetInputFormat(format);
    ASSERT_EQ(AVCS_ERR_OK, ret);
}
} // namespace