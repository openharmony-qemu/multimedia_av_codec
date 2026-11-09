/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#include <map>
#include <memory>
#include <thread>
#include <mutex>
#include <queue>
#include <string>

#include "gtest/gtest.h"
#include "serverdec_sample.h"
#include "avcodec_video_decoder.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "avcodec_common.h"
#include "native_avcapability.h"

using namespace std;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace testing::ext;

namespace {
class HevcswdecInnerApiNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test case
    void SetUp() override;
    // TearDown: Called after each test case
    void TearDown() override;
protected:
    const char *INP_DIR_720_30 = "/data/test/media/720_1280_25_avcc.h265";
};

std::shared_ptr<AVCodecVideoDecoder> vdec_ = nullptr;
std::shared_ptr<VDecInnerSignal> signal_ = nullptr;
std::string g_invalidCodecMime = "avdec_h265";
std::string g_codecMime = "video/hevc";
std::string g_codecName = "";

void HevcswdecInnerApiNdkTest::SetUpTestCase()
{
    OH_AVCapability *cap = OH_AVCodec_GetCapabilityByCategory(g_codecMime.c_str(), false, SOFTWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    g_codecName = tmpCodecName;
    cout << "g_codecName: " << g_codecName << endl;
}

void HevcswdecInnerApiNdkTest::TearDownTestCase() {}

void HevcswdecInnerApiNdkTest::SetUp()
{
    signal_ = make_shared<VDecInnerSignal>();
}

void HevcswdecInnerApiNdkTest::TearDown()
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
 * @tc.number    : VIDEO_MEMORYRECYCLE_0100
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0100, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->inputDir = INP_DIR_720_30;
        vDecSample->defaultWidth = 1920;
        vDecSample->defaultHeight = 1080;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sfOutput = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunHEVCVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0200
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0200, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->inputDir = INP_DIR_720_30;
        vDecSample->defaultWidth = 1920;
        vDecSample->defaultHeight = 1080;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sfOutput = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunHEVCErrorVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0300
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0300, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->inputDir = INP_DIR_720_30;
        vDecSample->defaultWidth = 1920;
        vDecSample->defaultHeight = 1080;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sfOutput = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunFcodecVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0400
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0400, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->inputDir = INP_DIR_720_30;
        vDecSample->defaultWidth = 1920;
        vDecSample->defaultHeight = 1080;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sfOutput = false;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunFcodecVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

/**
 * @tc.number    : VIDEO_MEMORYRECYCLE_0500
 * @tc.name      : Normal decoding process
 * @tc.desc      : api test
 */
HWTEST_F(HevcswdecInnerApiNdkTest, VIDEO_MEMORYRECYCLE_0500, TestSize.Level1)
{
    if (!access("/system/lib64/media/", 0)) {
        shared_ptr<VDecNdkInnerSample> vDecSample = make_shared<VDecNdkInnerSample>();
        vDecSample->inputDir = INP_DIR_720_30;
        vDecSample->defaultWidth = 1920;
        vDecSample->defaultHeight = 1080;
        vDecSample->defaultFrameRate = 30;
        vDecSample->sfOutput = true;
        ASSERT_EQ(AVCS_ERR_OK, vDecSample->RunFcodecErrorVideoDecoder(g_codecName));
        vDecSample->WaitForEOS();
    }
}

} // namespace

namespace {
const char *HEVC_TAKEOVER_INPUT_DIR = "/data/test/media/720_1280_25_avcc.h265";
const char *HEVC_TAKEOVER_CODEC_NAME = "OH.Media.Codec.Decoder.Video.HEVC";
constexpr uint32_t HEVC_TAKEOVER_WIDTH = 1920;
constexpr uint32_t HEVC_TAKEOVER_HEIGHT = 1080;
constexpr double HEVC_TAKEOVER_FRAME_RATE = 30.0;
constexpr uint32_t HEVC_TAKEOVER_INPUT_BUFFER_COUNT = 4;
constexpr uint32_t HEVC_TAKEOVER_TIMEOUT_MS = 60000;

/**
 * Fixture of the hard-to-software switch input buffer takeover cases.Every case owns one
 * in-process HEVC decoder and the same external input pool, so cases stay independent.
 */
class HevcSetInputBuffersTest : public testing::Test {
public:
    void SetUp() override
    {
        sample_ = make_shared<VDecNdkInnerSample>();
        if (sample_ != nullptr) {
            sample_->inputDir = HEVC_TAKEOVER_INPUT_DIR;
            sample_->defaultWidth = HEVC_TAKEOVER_WIDTH;
            sample_->defaultHeight = HEVC_TAKEOVER_HEIGHT;
            sample_->defaultFrameRate = HEVC_TAKEOVER_FRAME_RATE;
        }
    }
    void TearDown() override
    {
        if (sample_ != nullptr) {
            sample_->ReleaseInFile();
            sample_->Release();
            sample_ = nullptr;
        }
    }

protected:
    std::shared_ptr<VDecNdkInnerSample> sample_;
};

/**
 * @tc.number    : VIDEO_SET_INPUT_BUFFERS_0100
 * @tc.name      : SetInputBuffers rejected in INITIALIZED state
 * @tc.desc      : hard-to-software switch input buffer takeover test
 */
HWTEST_F(HevcSetInputBuffersTest, HEVC_SET_INPUT_BUFFERS_REJECT_IN_INIT_STATE_0100, TestSize.Level1)
{
    if (sample_ == nullptr) {
        return;
    }

    ASSERT_EQ(AVCS_ERR_OK, sample_->CreateHevcDecoderByNameAndInit(HEVC_TAKEOVER_CODEC_NAME));
    ASSERT_EQ(AVCS_ERR_OK, sample_->PrepareTakeoverInputPool());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, sample_->SetTakeoverInputBuffers(sample_->takeoverPool_));
}

/**
 * @tc.number    : VIDEO_SET_INPUT_BUFFERS_0200
 * @tc.name      : SetInputBuffers rejected for empty input pool
 * @tc.desc      : hard-to-software switch input buffer takeover test
 */
HWTEST_F(HevcSetInputBuffersTest, HEVC_SET_INPUT_BUFFERS_REJECT_EMPTY_POOL_0200, TestSize.Level1)
{
    if (sample_ == nullptr) {
        return;
    }

    ASSERT_EQ(AVCS_ERR_OK, sample_->CreateHevcDecoderByNameAndInit(HEVC_TAKEOVER_CODEC_NAME));
    ASSERT_EQ(AVCS_ERR_OK, sample_->ConfigureDecoder());
    std::map<uint32_t, std::shared_ptr<OHOS::Media::AVBuffer>> emptyPool;
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, sample_->SetTakeoverInputBuffers(emptyPool));
}

/**
 * @tc.number    : VIDEO_SET_INPUT_BUFFERS_0300
 * @tc.name      : SetInputBuffers rejected for buffer indexes ranging from 2 to 10
 * @tc.desc      : hard-to-software switch input buffer takeover test
 */
HWTEST_F(HevcSetInputBuffersTest, HEVC_SET_INPUT_BUFFERS_REJECT_INDEXES_NOT_RANGE_0300, TestSize.Level1)
{
    if (sample_ == nullptr) {
        return;
    }

    ASSERT_EQ(AVCS_ERR_OK, sample_->CreateHevcDecoderByNameAndInit(HEVC_TAKEOVER_CODEC_NAME));
    ASSERT_EQ(AVCS_ERR_OK, sample_->PrepareTakeoverInputPool());
    ASSERT_EQ(AVCS_ERR_OK, sample_->ConfigureDecoder());
    std::map<uint32_t, std::shared_ptr<OHOS::Media::AVBuffer>> brokenPool;
    brokenPool[2] = sample_->takeoverPool_[2];
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, sample_->SetTakeoverInputBuffers(brokenPool));
}

/**
 * @tc.number    : VIDEO_SET_INPUT_BUFFERS_0400
 * @tc.name      : SetInputBuffers rejected in RUNNING state
 * @tc.desc      : hard-to-software switch input buffer takeover test
 */
HWTEST_F(HevcSetInputBuffersTest, HEVC_SET_INPUT_BUFFERS_REJECT_IN_RUNNING_STATE_0400, TestSize.Level1)
{
    if (sample_ == nullptr) {
        return;
    }

    ASSERT_EQ(AVCS_ERR_OK, sample_->CreateHevcDecoderByNameAndInit(HEVC_TAKEOVER_CODEC_NAME));
    ASSERT_EQ(AVCS_ERR_OK, sample_->PrepareTakeoverInputPool());
    ASSERT_EQ(AVCS_ERR_OK, sample_->ConfigureDecoder());
    ASSERT_EQ(AVCS_ERR_OK, sample_->SetCallback());
    ASSERT_EQ(AVCS_ERR_OK, sample_->SetTakeoverInputBuffers(sample_->takeoverPool_));
    ASSERT_EQ(AVCS_ERR_OK, sample_->Start());
    ASSERT_EQ(AVCS_ERR_INVALID_STATE, sample_->SetTakeoverInputBuffers(sample_->takeoverPool_));
}

/**
 * @tc.number    : VIDEO_SET_INPUT_BUFFERS_0500
 * @tc.name      : Pending takeover buffers returned with the same index/buffer/address
 * @tc.desc      : hard-to-software switch input buffer takeover test
 */
HWTEST_F(HevcSetInputBuffersTest, HEVC_SET_INPUT_BUFFERS_PENDING_RETURN_SAME_BINDING_0500, TestSize.Level1)
{
    if (sample_ == nullptr) {
        return;
    }

    ASSERT_EQ(AVCS_ERR_OK, sample_->CreateHevcDecoderByNameAndInit(HEVC_TAKEOVER_CODEC_NAME));
    ASSERT_EQ(AVCS_ERR_OK, sample_->PrepareTakeoverInputPool());
    ASSERT_EQ(AVCS_ERR_OK, sample_->ConfigureDecoder());
    ASSERT_EQ(AVCS_ERR_OK, sample_->SetCallback());
    ASSERT_EQ(AVCS_ERR_OK, sample_->SetTakeoverInputBuffers(sample_->takeoverPool_));
    ASSERT_EQ(AVCS_ERR_OK, sample_->Start());
    ASSERT_EQ(AVCS_ERR_OK, sample_->WaitTakeoverPendingReturned(HEVC_TAKEOVER_TIMEOUT_MS));
    ASSERT_EQ(HEVC_TAKEOVER_INPUT_BUFFER_COUNT, sample_->takeoverReturnedCount_);
    ASSERT_EQ(true, sample_->takeoverAddrCheckPass_);
}

/**
 * @tc.number    : VIDEO_SET_INPUT_BUFFERS_0600
 * @tc.name      : Takeover session keeps decoding from file until EOS
 * @tc.desc      : hard-to-software switch input buffer takeover test
 */
HWTEST_F(HevcSetInputBuffersTest, HEVC_SET_INPUT_BUFFERS_DECODE_UNTIL_EOS_0600, TestSize.Level1)
{
    if (sample_ == nullptr) {
        return;
    }

    ASSERT_EQ(AVCS_ERR_OK, sample_->CreateHevcDecoderByNameAndInit(HEVC_TAKEOVER_CODEC_NAME));
    ASSERT_EQ(AVCS_ERR_OK, sample_->PrepareTakeoverInputPool());
    ASSERT_EQ(AVCS_ERR_OK, sample_->ConfigureDecoder());
    ASSERT_EQ(AVCS_ERR_OK, sample_->SetCallback());
    ASSERT_EQ(AVCS_ERR_OK, sample_->SetTakeoverInputBuffers(sample_->takeoverPool_));
    ASSERT_EQ(AVCS_ERR_OK, sample_->Start());
    ASSERT_EQ(AVCS_ERR_OK, sample_->FeedTakeoverUntilEos(HEVC_TAKEOVER_TIMEOUT_MS));
    ASSERT_GE(sample_->takeoverOutCount_, 1u);
}

} // namespace