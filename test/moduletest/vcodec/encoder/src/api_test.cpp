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
#include "native_avcodec_base.h"
#include "native_avformat.h"
#include "native_avcodec_videoencoder.h"
#include "videoenc_ndk_sample.h"
#include "native_avcapability.h"
using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
namespace {
OH_AVCodec *venc_ = NULL;
constexpr uint32_t DEFAULT_WIDTH = 1920;
constexpr uint32_t DEFAULT_HEIGHT = 1080;
const char *codecMime = "video/avc";
constexpr uint32_t CODEC_NAME_SIZE = 128;
char codecName[CODEC_NAME_SIZE] = {};
OH_AVCapability *cap = nullptr;
OHOS::Media::VEncSignal *signal_ = nullptr;
OH_AVFormat *format;
void onError(OH_AVCodec *codec, int32_t errorCode, void *userData)
{
    cout << "Error errorCode=" << errorCode << endl;
};

void onStreamChanged(OH_AVCodec *codec, OH_AVFormat *fmt, void *userData)
{
    cout << "stream Changed" << endl;
};

void onNeedInputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, void *userData)
{
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal->inMutex_);
    signal->inIdxQueue_.push(index);
    signal->inBufferQueue_.push(data);
    signal->inCond_.notify_all();
    cout << "need input data" << endl;
};

void onNewOutputData(OH_AVCodec *codec, uint32_t index, OH_AVMemory *data, OH_AVCodecBufferAttr *attr, void *userData)
{
    cout << "output data" << endl;
    VEncSignal *signal = static_cast<VEncSignal *>(userData);
    if (signal == nullptr) {
        return;
    }
    unique_lock<mutex> lock(signal->outMutex_);
    signal->outIdxQueue_.push(index);
    signal->attrQueue_.push(*attr);
    signal->outBufferQueue_.push(data);
    signal->outCond_.notify_all();
};
} // namespace

namespace OHOS {
namespace Media {
class HwEncApiNdkTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};

void HwEncApiNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    const char *TMP_CODEC_NAME = OH_AVCapability_GetName(cap);
    if (memcpy_s(codecName, sizeof(codecName), TMP_CODEC_NAME, strlen(TMP_CODEC_NAME)) != 0) {
        cout << "memcpy failed" << endl;
    }
    cout << "codecname: " << codecName << endl;
}
void HwEncApiNdkTest::TearDownTestCase() {}
void HwEncApiNdkTest::SetUp()
{
    signal_ = new VEncSignal();
}
void HwEncApiNdkTest::TearDown()
{
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
    if (signal_) {
        delete signal_;
        signal_ = nullptr;
    }
}
} // namespace Media
} // namespace OHOS

namespace {
/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0100
 * @tc.name      : OH_VideoEncoder_CreateByMime para1 error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0100, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime(nullptr);
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0200
 * @tc.name      : OH_VideoEncoder_CreateByMime para2 error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0200, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime("");
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0300
 * @tc.name      : OH_VideoEncoder_CreateByMime para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0300, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(nullptr);
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0400
 * @tc.name      : OH_VideoEncoder_CreateByMime para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0400, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName("");
    ASSERT_EQ(nullptr, venc_);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0500
 * @tc.name      : OH_VideoEncoder_CreateByMime para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_Destroy(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0600
 * @tc.name      : OH_VideoEncoder_SetCallback para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0600, TestSize.Level2)
{
    OH_AVCodecAsyncCallback cb_;
    cb_.onError = onError;
    cb_.onStreamChanged = onStreamChanged;
    cb_.onNeedInputData = onNeedInputData;
    cb_.onNeedOutputData = onNewOutputData;

    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_SetCallback(NULL, cb_, static_cast<void *>(signal_)));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0700
 * @tc.name      : OH_VideoEncoder_SetCallback para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0700, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(NULL, venc_);

    OH_AVCodecAsyncCallback cb2_;
    cb2_.onError = NULL;
    cb2_.onStreamChanged = NULL;
    cb2_.onNeedInputData = NULL;
    cb2_.onNeedOutputData = NULL;
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(venc_, cb2_, static_cast<void *>(signal_)));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0800
 * @tc.name      : OH_VideoEncoder_SetCallback para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0800, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    OH_AVCodecAsyncCallback cb_;
    cb_.onError = onError;
    cb_.onStreamChanged = onStreamChanged;
    cb_.onNeedInputData = onNeedInputData;
    cb_.onNeedOutputData = onNewOutputData;
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(venc_, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_0900
 * @tc.name      : OH_VideoEncoder_Configure para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_0900, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(nullptr, venc_);
    ret = OH_VideoEncoder_Configure(venc_, nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_1100
 * @tc.name      : OH_VideoEncoder_Configure para not enough
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_1100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_BITRATE, 100000);
    ret = OH_VideoEncoder_Configure(venc_, format);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_1400
 * @tc.name      : OH_VideoEncoder_Start para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_1400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_Start(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_1500
 * @tc.name      : OH_VideoEncoder_Stop para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_1500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_Stop(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_1600
 * @tc.name      : OH_VideoEncoder_Flush para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_1600, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_Flush(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_1700
 * @tc.name      : OH_VideoEncoder_Reset para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_1700, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_Reset(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_1800
 * @tc.name      : OH_VideoEncoder_Reset para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_1800, TestSize.Level2)
{
    format = OH_VideoEncoder_GetOutputDescription(nullptr);
    ASSERT_EQ(format, nullptr);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_1900
 * @tc.name      : OH_VideoEncoder_SetParameter para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_1900, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_SetParameter(venc_, nullptr));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2000
 * @tc.name      : OH_VideoEncoder_SetParameter para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2000, TestSize.Level2)
{
    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_SetParameter(NULL, format));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2100
 * @tc.name      : OH_VideoEncoder_GetSurface para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(nullptr, venc_);
    ret = OH_VideoEncoder_GetSurface(venc_, nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2200
 * @tc.name      : OH_VideoEncoder_FreeOutputData para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2200, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_FreeOutputData(nullptr, 0);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2300
 * @tc.name      : OH_VideoEncoder_FreeOutputData para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2300, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(nullptr, venc_);
    format = OH_AVFormat_Create();
    ASSERT_NE(nullptr, format);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_HEIGHT, 1080);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_WIDTH, 1080);
    OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_YUVI420);

    ret = OH_VideoEncoder_Configure(venc_, format);
    ASSERT_EQ(ret, AV_ERR_OK);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(venc_));

    ret = OH_VideoEncoder_FreeOutputData(venc_, 9999999);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2400
 * @tc.name      : OH_VideoEncoder_NotifyEndOfStream para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_NotifyEndOfStream(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}
/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2500
 * @tc.name      : OH_VideoEncoder_NotifyEndOfStream para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_VideoEncoder_NotifyEndOfStream(nullptr);
    ASSERT_EQ(ret, AV_ERR_INVALID_VAL);
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2600
 * @tc.name      : OH_VideoEncoder_PushInputData para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2600, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVCodecBufferAttr attr;
    attr.pts = -1;
    attr.size = -1;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;

    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_PushInputData(venc_, 0, attr));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2700
 * @tc.name      : OH_VideoEncoder_PushInputData para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2700, TestSize.Level2)
{
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_PushInputData(NULL, 0, attr));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2800
 * @tc.name      : OH_VideoEncoder_PushInputData para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2800, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(nullptr, venc_);
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_PushInputData(NULL, 99999, attr));
}

/**
 * @tc.number    : VIDEO_ENCODE_ILLEGAL_PARA_2900
 * @tc.name      : OH_VideoEncoder_GetInputDescription para error
 * * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_ILLEGAL_PARA_2900, TestSize.Level2)
{
    ASSERT_EQ(nullptr, OH_VideoEncoder_GetInputDescription(nullptr));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0100
 * @tc.name      : OH_AVCodec_GetCapability para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0100, TestSize.Level2)
{
    const char *p = nullptr;
    ASSERT_EQ(nullptr, OH_AVCodec_GetCapability(p, true));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0200
 * @tc.name      : OH_AVCodec_GetCapability para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0200, TestSize.Level2)
{
    ASSERT_EQ(nullptr, OH_AVCodec_GetCapability("", true));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0300
 * @tc.name      : OH_AVCodec_GetCapability para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0300, TestSize.Level2)
{
    ASSERT_EQ(nullptr, OH_AVCodec_GetCapability("notexist", true));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0400
 * @tc.name      : OH_AVCodec_GetCapability
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0400, TestSize.Level2)
{
    ASSERT_NE(nullptr, OH_AVCodec_GetCapability(codecMime, true));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0500
 * @tc.name      : OH_AVCodec_GetCapabilityByCategory para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0500, TestSize.Level2)
{
    ASSERT_EQ(nullptr, OH_AVCodec_GetCapabilityByCategory("", true, HARDWARE));
}
/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_9900
 * @tc.name      : OH_AVCodec_GetCapabilityByCategory para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_9900, TestSize.Level2)
{
    ASSERT_EQ(nullptr, OH_AVCodec_GetCapabilityByCategory(nullptr, true, HARDWARE));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0600
 * @tc.name      : OH_AVCodec_GetCapabilityByCategory para error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0600, TestSize.Level2)
{
    ASSERT_EQ(nullptr, OH_AVCodec_GetCapabilityByCategory("notexist", true, HARDWARE));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0700
 * @tc.name      : OH_AVCodec_GetCapabilityByCategory param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0700, TestSize.Level2)
{
    ASSERT_NE(nullptr, OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0800
 * @tc.name      : OH_AVCapability_IsHardware param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0800, TestSize.Level2)
{
    ASSERT_EQ(false, OH_AVCapability_IsHardware(nullptr));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_0900
 * @tc.name      : OH_AVCapability_IsHardware param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_0900, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_IsHardware(capability));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1000
 * @tc.name      : OH_AVCapability_GetName param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1000, TestSize.Level2)
{
    const char *name = OH_AVCapability_GetName(nullptr);
    ASSERT_NE(name, nullptr);
    ASSERT_EQ(strlen(name), 0);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1100
 * @tc.name      : OH_AVCapability_GetName param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1100, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    const char *name = OH_AVCapability_GetName(capability);
    ASSERT_NE(name, nullptr);
    ASSERT_EQ(strlen(name) > 0, true);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1200
 * @tc.name      : OH_AVCapability_GetMaxSupportedInstances param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1200, TestSize.Level2)
{
    int32_t maxSupportedInstance = OH_AVCapability_GetMaxSupportedInstances(nullptr);
    ASSERT_EQ(maxSupportedInstance, 0);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1300
 * @tc.name      : OH_AVCapability_GetMaxSupportedInstances param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1300, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);

    string codec_name = OH_AVCapability_GetName(capability);
    if (codec_name == "OMX.hisi.video.encoder.avc") {
        ASSERT_EQ(16, OH_AVCapability_GetMaxSupportedInstances(capability));
    } else {
        ASSERT_EQ(4, OH_AVCapability_GetMaxSupportedInstances(capability));
    }
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1400
 * @tc.name      : OH_AVCapability_GetEncoderBitrateRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    ret = OH_AVCapability_GetEncoderBitrateRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1500
 * @tc.name      : OH_AVCapability_GetEncoderBitrateRange param error
 * @tc.desc      : api test
 */

HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;

    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);

    ret = OH_AVCapability_GetEncoderBitrateRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1700
 * @tc.name      : OH_AVCapability_GetEncoderBitrateRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1700, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetEncoderBitrateRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "max val " << range.maxVal << "  min val " << range.minVal << endl;
    ASSERT_EQ(true, (range.minVal > 0));
    ASSERT_EQ(true, (range.maxVal > 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1800
 * @tc.name      : OH_AVCapability_IsEncoderBitrateModeSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1800, TestSize.Level2)
{
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(nullptr, BITRATE_MODE_CBR);
    ASSERT_EQ(false, isSupported);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_1900
 * @tc.name      : OH_AVCapability_IsEncoderBitrateModeSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_1900, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    bool isSupported = OH_AVCapability_IsEncoderBitrateModeSupported(capability, BITRATE_MODE_CBR);
    ASSERT_EQ(isSupported, true);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2000
 * @tc.name      : OH_AVCapability_GetEncoderQualityRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2000, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    ret = OH_AVCapability_GetEncoderQualityRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2100
 * @tc.name      : OH_AVCapability_GetEncoderBitrateRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_AVCapability_GetEncoderQualityRange(nullptr, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2200
 * @tc.name      : OH_AVCapability_GetEncoderQualityRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2200, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetEncoderQualityRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2300
 * @tc.name      : OH_AVCapability_GetEncoderQualityRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2300, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetEncoderQualityRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2400
 * @tc.name      : OH_AVCapability_GetEncoderComplexityRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    ret = OH_AVCapability_GetEncoderComplexityRange(nullptr, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2500
 * @tc.name      : OH_AVCapability_GetEncoderComplexityRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    ret = OH_AVCapability_GetEncoderComplexityRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2700
 * @tc.name      : OH_AVCapability_GetEncoderComplexityRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2700, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetEncoderComplexityRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_2800
 * @tc.name      : OH_AVCapability_GetEncoderComplexityRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_2800, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetEncoderComplexityRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3100
 * @tc.name      : OH_AVCapability_GetVideoWidthAlignment param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3200
 * @tc.name      : OH_AVCapability_GetVideoWidthAlignment param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3200, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoWidthAlignment(nullptr, &alignment);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3300
 * @tc.name      : OH_AVCapability_GetVideoWidthAlignment param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3300, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoWidthAlignment(capability, &alignment);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment >= 0, true);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3400
 * @tc.name      : OH_AVCapability_GetVideoHeightAlignment param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3500
 * @tc.name      : OH_AVCapability_GetVideoHeightAlignment param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoHeightAlignment(nullptr, &alignment);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3600
 * @tc.name      : OH_AVCapability_GetVideoHeightAlignment param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3600, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    int32_t alignment = 0;
    ret = OH_AVCapability_GetVideoHeightAlignment(capability, &alignment);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(alignment >= 0, true);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3700
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3700, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(nullptr, 1920, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3800
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3800, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 1920, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_3900
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_3900, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4000
 * @tc.name      : OH_AVCapability_GetVideoWidthRangeForHeight param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4000, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRangeForHeight(capability, 1080, &range);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(true, (range.minVal >= 0));
    ASSERT_EQ(true, (range.maxVal > 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4100
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(nullptr, 1080, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4200
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4200, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 1080, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4300
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4300, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4400
 * @tc.name      : OH_AVCapability_GetVideoHeightRangeForWidth param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRangeForWidth(capability, 1920, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(true, (range.minVal >= 0));
    ASSERT_EQ(true, (range.maxVal > 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4500
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoWidthRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4600
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4600, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4700
 * @tc.name      : OH_AVCapability_GetVideoWidthRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4700, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoWidthRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(true, (range.minVal >= 0));
    ASSERT_EQ(true, (range.maxVal > 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4800
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4800, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoHeightRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_4900
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_4900, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5000
 * @tc.name      : OH_AVCapability_GetVideoHeightRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5000, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoHeightRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(true, (range.minVal >= 0));
    ASSERT_EQ(true, (range.maxVal > 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5100
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5100, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, 0, 1080));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5200
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5200, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(capability, 1920, 0));
}
/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5300
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5300, TestSize.Level2)
{
    ASSERT_EQ(false, OH_AVCapability_IsVideoSizeSupported(nullptr, 1920, 1080));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5400
 * @tc.name      : OH_AVCapability_IsVideoSizeSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5400, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_IsVideoSizeSupported(capability, 1920, 1080));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5500
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoFrameRateRange(nullptr, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5600
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRange param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5600, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5700
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRange param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5700, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRange(capability, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    ASSERT_EQ(true, (range.minVal >= 0));
    ASSERT_EQ(true, (range.maxVal > 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5800
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5800, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(nullptr, 1920, 1080, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_5900
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_5900, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 1920, 1080, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6000
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6000, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 0, 1080, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6100
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 1920, 0, &range);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6200
 * @tc.name      : OH_AVCapability_GetVideoFrameRateRangeForSize param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6200, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    OH_AVRange range;
    memset_s(&range, sizeof(OH_AVRange), 0, sizeof(OH_AVRange));
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoFrameRateRangeForSize(capability, 1920, 1080, &range);
    ASSERT_EQ(AV_ERR_OK, ret);
    cout << "minval=" << range.minVal << "  maxval=" << range.maxVal << endl;
    ASSERT_EQ(true, (range.minVal >= 0));
    ASSERT_EQ(true, (range.maxVal > 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6300
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6300, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, 0, 1080, 30));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6400
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6400, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, 1920, 0, 30));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6500
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6500, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, 1920, 1080, 0));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6600
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6600, TestSize.Level2)
{
    ASSERT_EQ(false, OH_AVCapability_AreVideoSizeAndFrameRateSupported(nullptr, 1920, 1080, 30));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6700
 * @tc.name      : OH_AVCapability_AreVideoSizeAndFrameRateSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6700, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_AreVideoSizeAndFrameRateSupported(capability, 1920, 1080, 30));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6800
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6800, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    uint32_t pixelFormatNum = 0;
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(nullptr, &pixelFormat, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_6900
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_6900, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t pixelFormatNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, nullptr, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7000
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7000, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7100
 * @tc.name      : OH_AVCapability_GetVideoSupportedPixelFormats param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7100, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *pixelFormat = nullptr;
    uint32_t pixelFormatNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetVideoSupportedPixelFormats(capability, &pixelFormat, &pixelFormatNum);
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7200
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7200, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
    ret = OH_AVCapability_GetSupportedProfiles(nullptr, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7300
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7300, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, nullptr, &profileNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7400
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7400, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7500
 * @tc.name      : OH_AVCapability_GetSupportedProfiles param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7500, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *profiles = nullptr;
    uint32_t profileNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedProfiles(capability, &profiles, &profileNum);
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7600
 * @tc.name      : OH_AVCapability_GetSupportedLevelsForProfile param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7600, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *levels = nullptr;
    uint32_t levelNum = 0;
    ret = OH_AVCapability_GetSupportedLevelsForProfile(nullptr, AVC_PROFILE_BASELINE, &levels, &levelNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7700
 * @tc.name      : OH_AVCapability_GetSupportedLevelsForProfile param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7700, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *levels = nullptr;
    uint32_t levelNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedLevelsForProfile(capability, 1, &levels, &levelNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7800
 * @tc.name      : OH_AVCapability_GetSupportedLevelsForProfile param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7800, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    uint32_t levelNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedLevelsForProfile(capability, AVC_PROFILE_BASELINE, nullptr, &levelNum);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_7900
 * @tc.name      : OH_AVCapability_GetSupportedLevelsForProfile param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_7900, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *levels = nullptr;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedLevelsForProfile(capability, AVC_PROFILE_BASELINE, &levels, nullptr);
    ASSERT_EQ(AV_ERR_INVALID_VAL, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_8000
 * @tc.name      : OH_AVCapability_GetSupportedLevelsForProfile param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_8000, TestSize.Level2)
{
    OH_AVErrCode ret = AV_ERR_OK;
    const int32_t *levels = nullptr;
    uint32_t levelNum = 0;
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ret = OH_AVCapability_GetSupportedLevelsForProfile(capability, AVC_PROFILE_BASELINE, &levels, &levelNum);
    ASSERT_EQ(AV_ERR_OK, ret);
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_8100
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_8100, TestSize.Level2)
{
    ASSERT_EQ(false, OH_AVCapability_AreProfileAndLevelSupported(nullptr, AVC_PROFILE_BASELINE, 1));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_8200
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param error
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_8200, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(false, OH_AVCapability_AreProfileAndLevelSupported(capability, 1, 1));
}

/**
 * @tc.number    : VIDEO_ENCODE_CAPABILITY_8300
 * @tc.name      : OH_AVCapability_AreProfileAndLevelSupported param correct
 * @tc.desc      : api test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_CAPABILITY_8300, TestSize.Level2)
{
    OH_AVCapability *capability = OH_AVCodec_GetCapabilityByCategory(codecMime, true, HARDWARE);
    ASSERT_NE(nullptr, capability);
    ASSERT_EQ(true, OH_AVCapability_AreProfileAndLevelSupported(capability, AVC_PROFILE_BASELINE, 1));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0100
 * @tc.name      : create create
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0100, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(venc_, NULL);
    OH_AVCodec *venc_2 = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(venc_2, NULL);
    OH_VideoEncoder_Destroy(venc_2);
}

/**
 * @tc.number    : VIDEO_ENCODE_API_3100
 * @tc.name      : create create
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_3100, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(venc_, NULL);
    OH_AVCodec *venc_2 = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(venc_2, NULL);
    OH_VideoEncoder_Destroy(venc_2);
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0200
 * @tc.name      : create configure configure
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0200, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Configure(venc_, format));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0300
 * @tc.name      : create configure start start
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0300, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(venc_));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Start(venc_));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0400
 * @tc.name      : create configure start stop stop
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0400, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(venc_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Stop(venc_));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Stop(venc_));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0500
 * @tc.name      : create configure start stop reset reset
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0500, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(venc_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Stop(venc_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(venc_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Reset(venc_));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0600
 * @tc.name      : create configure start EOS EOS
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0600, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    OH_AVCodecAsyncCallback cb_;
    cb_.onError = onError;
    cb_.onStreamChanged = onStreamChanged;
    cb_.onNeedInputData = onNeedInputData;
    cb_.onNeedOutputData = onNewOutputData;
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(venc_, cb_, static_cast<void *>(signal_)));

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(venc_));
    unique_lock<mutex> lock(signal_->inMutex_);
    signal_->inCond_.wait(lock, [] { return signal_->inIdxQueue_.size() > 1; });
    uint32_t index = signal_->inIdxQueue_.front();
    OH_AVCodecBufferAttr attr;
    attr.pts = 0;
    attr.size = 0;
    attr.offset = 0;
    attr.flags = AVCODEC_BUFFER_FLAGS_EOS;

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_PushInputData(venc_, index, attr));
    signal_->inIdxQueue_.pop();
    index = signal_->inIdxQueue_.front();
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_PushInputData(venc_, index, attr));
    signal_->inIdxQueue_.pop();
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0700
 * @tc.name      : create configure start flush flush
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0700, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(venc_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Flush(venc_));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_Flush(venc_));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0800
 * @tc.name      : create configure start stop release release
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0800, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Configure(venc_, format));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Start(venc_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Stop(venc_));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(venc_));
    venc_ = nullptr;
    ASSERT_EQ(AV_ERR_INVALID_VAL, OH_VideoEncoder_Destroy(venc_));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_0900
 * @tc.name      : create create
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_0900, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(venc_, NULL);
    venc_ = OH_VideoEncoder_CreateByMime(codecMime);
    ASSERT_NE(venc_, NULL);
}

/**
 * @tc.number    : VIDEO_ENCODE_API_1000
 * @tc.name      : repeat OH_VideoEncoder_SetCallback
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_1000, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    OH_AVCodecAsyncCallback cb_;
    cb_.onError = onError;
    cb_.onStreamChanged = onStreamChanged;
    cb_.onNeedInputData = onNeedInputData;
    cb_.onNeedOutputData = onNewOutputData;
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(venc_, cb_, NULL));
    ASSERT_EQ(AV_ERR_OK, OH_VideoEncoder_SetCallback(venc_, cb_, NULL));
}

/**
 * @tc.number    : VIDEO_ENCODE_API_1100
 * @tc.name      : repeat OH_VideoEncoder_GetOutputDescription
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_1100, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    format = OH_VideoEncoder_GetOutputDescription(venc_);
    ASSERT_NE(NULL, format);
    OH_AVFormat_Destroy(format);
    format = OH_VideoEncoder_GetOutputDescription(venc_);
    ASSERT_NE(NULL, format);
}

/**
 * @tc.number    : VIDEO_ENCODE_API_1200
 * @tc.name      : repeat OH_VideoEncoder_SetParameter
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_1200, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);

    format = OH_AVFormat_Create();
    ASSERT_NE(NULL, format);

    string widthStr = "width";
    string heightStr = "height";
    string frameRateStr = "frame_rate";
    (void)OH_AVFormat_SetIntValue(format, widthStr.c_str(), DEFAULT_WIDTH);
    (void)OH_AVFormat_SetIntValue(format, heightStr.c_str(), DEFAULT_HEIGHT);
    (void)OH_AVFormat_SetIntValue(format, OH_MD_KEY_PIXEL_FORMAT, AV_PIXEL_FORMAT_NV12);

    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_SetParameter(venc_, format));
    ASSERT_EQ(AV_ERR_INVALID_STATE, OH_VideoEncoder_SetParameter(venc_, format));
}
/**
 * @tc.number    : VIDEO_ENCODE_API_1200
 * @tc.name      : repeat OH_VideoEncoder_GetInputDescription
 * @tc.desc      : function test
 */
HWTEST_F(HwEncApiNdkTest, VIDEO_ENCODE_API_1300, TestSize.Level2)
{
    venc_ = OH_VideoEncoder_CreateByName(codecName);
    ASSERT_NE(NULL, venc_);
    format = OH_VideoEncoder_GetInputDescription(venc_);
    ASSERT_NE(NULL, format);
    OH_AVFormat_Destroy(format);
    format = OH_VideoEncoder_GetInputDescription(venc_);
    ASSERT_NE(NULL, format);
}
} // namespace