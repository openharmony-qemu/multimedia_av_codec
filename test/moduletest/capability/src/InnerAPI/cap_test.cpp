/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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
#include <securec.h>
#include "gtest/gtest.h"
#include "native_avcodec_videoencoder.h"
#include "native_averrors.h"
#include "native_avcodec_base.h"
#include "avcodec_codec_name.h"
#include "native_avcapability.h"
#include "avcodec_info.h"
#include "avcodec_list.h"
#include "meta/meta_key.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "av_common.h"
#include <typeinfo>

namespace {
OH_AVCodec *venc_ = NULL;
OH_AVCapability *cap = nullptr;
OH_AVCapability *cap_hevc = nullptr;
constexpr uint32_t CODEC_NAME_SIZE = 128;
char g_codecName[CODEC_NAME_SIZE] = {};
char g_codecNameHEVC[CODEC_NAME_SIZE] = {};
OH_AVFormat *format;
} // namespace
namespace OHOS {
namespace Media {
class HwCapabilityInnerNdkTest : public testing::Test {
public:
    static void SetUpTestCase();
    static void TearDownTestCase();
    void SetUp() override;
    void TearDown() override;
    void InputFunc();
    void OutputFunc();
    void Release();
    int32_t Stop();
};
} // namespace Media
} // namespace OHOS

using namespace std;
using namespace OHOS;
using namespace OHOS::Media;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

void HwCapabilityInnerNdkTest::SetUpTestCase()
{
    cap = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_AVC, true, HARDWARE);
    const char *tmpCodecName = OH_AVCapability_GetName(cap);
    if (memcpy_s(g_codecName, sizeof(g_codecName), tmpCodecName, strlen(tmpCodecName)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname: " << g_codecName << endl;
    cap_hevc = OH_AVCodec_GetCapabilityByCategory(OH_AVCODEC_MIMETYPE_VIDEO_HEVC, true, HARDWARE);
    const char *tmpCodecNameHevc = OH_AVCapability_GetName(cap_hevc);
    if (memcpy_s(g_codecNameHEVC, sizeof(g_codecNameHEVC), tmpCodecNameHevc, strlen(tmpCodecNameHevc)) != 0)
        cout << "memcpy failed" << endl;
    cout << "codecname_hevc: " << g_codecNameHEVC << endl;
}
void HwCapabilityInnerNdkTest::TearDownTestCase() {}
void HwCapabilityInnerNdkTest::SetUp() {}
void HwCapabilityInnerNdkTest::TearDown()
{
    if (venc_ != NULL) {
        OH_VideoEncoder_Destroy(venc_);
        venc_ = nullptr;
    }
    if (format != nullptr) {
        OH_AVFormat_Destroy(format);
        format = nullptr;
    }
}
namespace {
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0010
 * @tc.name      : IsFeatureSupported para error
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0010, TestSize.Level2)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(false, codecInfo->IsFeatureSupported(static_cast<AVCapabilityFeature>(4)));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0011
 * @tc.name      : 解码，是否支持分层编码
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0011, TestSize.Level2)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0012
 * @tc.name      : 编码，是否支持分层编码
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0012, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    if (!access("/system/lib64/media/", 0)) {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    } else {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0013
 * @tc.name      : 解码，是否支持LTR
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0013, TestSize.Level2)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0014
 * @tc.name      : 编码，是否支持LTR
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0014, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    if (!access("/system/lib64/media/", 0)) {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
    } else {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0015
 * @tc.name      : 软解，是否支持低时延
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0015, TestSize.Level2)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        false, AVCodecCategory::AVCODEC_SOFTWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0016
 * @tc.name      : 硬解，是否支持低时延
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0016, TestSize.Level2)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    if (!access("/system/lib64/media/", 0)) {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            false, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    } else {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            false, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0017
 * @tc.name      : 编码，是否支持低时延
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0017, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    if (!access("/system/lib64/media/", 0)) {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    } else {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0018
 * @tc.name      : GetFeatureProperties para error
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0018, TestSize.Level2)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION, codecInfo->GetFeatureProperties(static_cast<AVCapabilityFeature>(4),
        featureFormat));
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, codecInfo->GetFeatureProperties(static_cast<AVCapabilityFeature>(-1),
        featureFormat));
    ASSERT_EQ(AVCS_ERR_INVALID_VAL, codecInfo->GetFeatureProperties(static_cast<AVCapabilityFeature>(100),
        featureFormat));
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY,
            featureFormat));
    } else {
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0019
 * @tc.name      : 解码，查询分层编码的能力值
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0019, TestSize.Level2)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
        codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0020
 * @tc.name      : 编码，查询分层编码的能力值
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0020, TestSize.Level1)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY,
            featureFormat));
    } else {
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0021
 * @tc.name      : 解码，查询LTR能力值
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0021, TestSize.Level2)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);    
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
        codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0022
 * @tc.name      : 编码，查询LTR的能力值
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0022, TestSize.Level1)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);     
    if (!access("/system/lib64/media/", 0)) {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(AVCS_ERR_OK, codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE,
            featureFormat));
        int ltrnum = 0;
        EXPECT_EQ(featureFormat.GetIntValue(OH_FEATURE_PROPERTY_KEY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT, ltrnum), true);
        EXPECT_EQ(ltrnum, 10);
    } else {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE, featureFormat));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0023
 * @tc.name      : 软解，查询低时延的能力值
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0023, TestSize.Level2)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        false, AVCodecCategory::AVCODEC_SOFTWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
        codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0024
 * @tc.name      : 硬解，查询低时延的能力值
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0024, TestSize.Level2)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);    
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
        codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0025
 * @tc.name      : 编码，查询低时延的能力值
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0025, TestSize.Level1)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_AVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY,
            featureFormat));
    } else {
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0026
 * @tc.name      : 能力查询是否支持LTRH265
 * @tc.desc      : function test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0026, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist); 
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
    } else {
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0027
 * @tc.name      : 能力查询是否支持LTRH265
 * @tc.desc      : function test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0027, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist); 
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE));
}
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0028
 * @tc.name      : 能力查询是否支持低时延H265
 * @tc.desc      : function test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0028, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);     
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    } else {
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0029
 * @tc.name      : 能力查询是否支持低时延H265
 * @tc.desc      : function test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0029, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);      
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    } else {
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_LOW_LATENCY));
    }
}
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0030
 * @tc.name      : 能力查询是否支持分层编码H265
 * @tc.desc      : function test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0030, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData); 
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(true, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    } else {
        ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
    }
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0031
 * @tc.name      : 能力查询是否支持分层编码H265
 * @tc.desc      : function test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0031, TestSize.Level1)
{
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);    
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(false, codecInfo->IsFeatureSupported(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY));
}
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0032
 * @tc.name      : 编码,查询低时延的能力值H265
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0032, TestSize.Level1)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);     
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY,
            featureFormat));
    } else {
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
    }
}
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0033
 * @tc.name      : 解码,查询低时延的能力值H265
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0033, TestSize.Level1)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist); 
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
        codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
}
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0034
 * @tc.name      : 解码，查询分层编码的能力值H265
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0034, TestSize.Level2)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);     
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
        codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0035
 * @tc.name      : 编码，查询分层编码的能力值H265
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0035, TestSize.Level1)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);     
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        true, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    if (!access("/system/lib64/media/", 0)) {
        ASSERT_EQ(AVCS_ERR_OK, codecInfo->GetFeatureProperties(
            AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
    } else {
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
    }
}
/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0036
 * @tc.name      : 解码，查询LTR能力值H265
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0036, TestSize.Level2)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);
    CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
        false, AVCodecCategory::AVCODEC_HARDWARE);
    ASSERT_NE(nullptr, capabilityData);
    std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
    ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
        codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_TEMPORAL_SCALABILITY, featureFormat));
}

/**
 * @tc.number    : VIDEO_TEMPORAL_ENCODE_INNER_API_0037
 * @tc.name      : 编码，查询LTR的能力值H265
 * @tc.desc      : api test
 */
HWTEST_F(HwCapabilityInnerNdkTest, VIDEO_TEMPORAL_ENCODE_INNER_API_0037, TestSize.Level1)
{
    Format featureFormat;
    auto codeclist = AVCodecListFactory::CreateAVCodecList();
    ASSERT_NE(nullptr, codeclist);    
    if (!access("/system/lib64/media/", 0)) {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(AVCS_ERR_OK,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE, featureFormat));
        int ltrnum = 0;
        EXPECT_EQ(featureFormat.GetIntValue(OH_FEATURE_PROPERTY_KEY_VIDEO_ENCODER_MAX_LTR_FRAME_COUNT, ltrnum), true);
        EXPECT_EQ(ltrnum, 10);
    } else {
        CapabilityData *capabilityData = codeclist->GetCapability(string(CodecMimeType::VIDEO_HEVC),
            true, AVCodecCategory::AVCODEC_HARDWARE);
        ASSERT_NE(nullptr, capabilityData);
        std::shared_ptr<AVCodecInfo> codecInfo = std::make_shared<AVCodecInfo>(capabilityData);
        ASSERT_EQ(AVCS_ERR_INVALID_OPERATION,
            codecInfo->GetFeatureProperties(AVCapabilityFeature::VIDEO_ENCODER_LONG_TERM_REFERENCE, featureFormat));
    }
}
} // namespace