/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include <random>
#include <gtest/gtest.h>
#include "hisysevent.h"
#include "hisysevent_manager_c.h"
#include "hisysevent_record_c.h"
#include "instance_info.h"
#include "native_averrors.h"
#include "statistics_event_handler.h"
#define TEST_SUIT DfxStatisticsEventCoverageTest

using namespace OHOS;
using namespace OHOS::Media;
using namespace OHOS::MediaAVCodec;
using namespace testing;
using namespace testing::ext;

namespace {
std::string GenerateRandomString(size_t length, std::string_view charset = "abcdefghijklmnopqrstuvwxyz")
{
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<size_t> dist(0, charset.size() - 1);
    std::string result;
    result.reserve(length);
    for (size_t i = 0; i < length; ++i) {
        result += charset[dist(gen)];
    }
    return result;
}
} // namespace

class TEST_SUIT : public testing::Test {
public:
    static void SetUpTestCase(void){};
    static void TearDownTestCase(void){};
    void SetUp(void);
    void TearDown(void);
    void SubmitEventInfo(StatisticsEventType eventType);

    std::shared_ptr<Media::Meta> meta_ = nullptr;
    static constexpr HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK,
                                                    STRINGFY(DfxStatisticsEventCoverageTest)};
};

void TEST_SUIT::SetUp(void)
{
    meta_ = std::make_shared<Media::Meta>();
    ASSERT_NE(nullptr, meta_);
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void TEST_SUIT::TearDown(void)
{
    meta_.reset();
}

void TEST_SUIT::SubmitEventInfo(StatisticsEventType eventType)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(eventType, *meta_);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: GetIndexByMeta_EmptyAppName_001
 * @tc.desc: Test GetIndexByMeta with empty app name string.
 *           Should return INVALID_APP_NAME_INDEX.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, GetIndexByMeta_EmptyAppName_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "");
    meta_->SetData("ADVANCED_FEATURE", "TEST_FEATURE");
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_QueryCapTimes_001
 * @tc.desc: Test OnSummateEventInfo aggregates queryCapTimes correctly.
 *           Verifies BASIC_QUERY_CAP_INFO event contributes to queryCapTimes counter.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_QueryCapTimes_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "query_cap_test");
    for (int i = 0; i < 3; ++i) {
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_QUERY_CAP_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnSummateEventInfo_CreateCodecTimes_001
 * @tc.desc: Test OnSummateEventInfo aggregates createCodecTimes correctly.
 *           Verifies BASIC_CREATE_CODEC_INFO event contributes to createCodecTimes counter.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_CreateCodecTimes_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "create_codec_test");
    for (int i = 0; i < 5; ++i) {
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnSummateEventInfo_VideoCodecSpecInfo_001
 * @tc.desc: Test OnSummateEventInfo aggregates video codec specification info correctly.
 *           Verifies valid video codec type, mime type, and secure flag are processed.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_VideoCodecSpecInfo_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "video_spec_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_VideoCodecSpecInfo_Invalid_001
 * @tc.desc: Test OnSummateEventInfo handles invalid video codec type correctly.
 *           Should skip invalid codec types (UNKNOWN or >= END).
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_VideoCodecSpecInfo_Invalid_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_video_spec_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::UNKNOWN));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_VideoCodecSpecInfo_EmptyMime_001
 * @tc.desc: Test OnSummateEventInfo handles empty mime type correctly.
 *           Should skip entries with empty mime type.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_VideoCodecSpecInfo_EmptyMime_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "empty_mime_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_AudioCodecSpecInfo_001
 * @tc.desc: Test OnSummateEventInfo aggregates audio codec specification info correctly.
 *           Verifies audio encoder spec info is processed.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_AudioCodecSpecInfo_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "audio_enc_test");
    meta_->SetData("CODEC_TYPE", std::to_string(AVCODEC_TYPE_AUDIO_ENCODER));
    meta_->SetData(Tag::MIME_TYPE, "audio/mp4a-latm");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_AudioCodecSpecInfo_Decoder_001
 * @tc.desc: Test OnSummateEventInfo aggregates audio decoder specification info correctly.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_AudioCodecSpecInfo_Decoder_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "audio_dec_test");
    meta_->SetData("CODEC_TYPE", std::to_string(AVCODEC_TYPE_AUDIO_DECODER));
    meta_->SetData(Tag::MIME_TYPE, "audio/mp4a-latm");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_AudioCodecSpecInfo_NoMimeType_001
 * @tc.desc: Test OnSummateEventInfo handles audio codec without mime type correctly.
 *           Should skip entries with empty mime type in OnAudioCodecSpecInfo.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_AudioCodecSpecInfo_NoMimeType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "audio_no_mime_test");
    meta_->SetData("CODEC_TYPE", std::to_string(AVCODEC_TYPE_AUDIO_ENCODER));
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_VideoCodecSpecInfo_Secure_001
 * @tc.desc: Test OnSummateEventInfo handles secure video codec correctly.
 *           Verifies isSecure flag is set to true when codec name contains "secure".
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_VideoCodecSpecInfo_Secure_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "secure_video_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData(Tag::MEDIA_CODEC_NAME, "OMX.hisi.video.decoder.avc.secure");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_VideoCodecSpecInfo_NonSecure_001
 * @tc.desc: Test OnSummateEventInfo handles non-secure video codec correctly.
 *           Verifies isSecure flag is set to false when codec name does not contain "secure".
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_VideoCodecSpecInfo_NonSecure_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "non_secure_video_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::ENCODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/hevc");
    meta_->SetData(Tag::MEDIA_CODEC_NAME, "OMX.hisi.video.encoder.hevc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_VideoCodecSpecInfo_CodecTypeEnd_001
 * @tc.desc: Test OnSummateEventInfo handles video codec type END boundary correctly.
 *           Should skip entries with codec type >= END.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_VideoCodecSpecInfo_CodecTypeEnd_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "codec_type_end_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::END));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_InvalidCodecType_001
 * @tc.desc: Test OnSummateEventInfo handles invalid codec type correctly.
 *           When CODEC_TYPE is neither audio encoder/decoder nor valid video codec type,
 *           should skip processing.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_InvalidCodecType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_codec_type_test");
    meta_->SetData("CODEC_TYPE", "999"); // invalid code type
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_NoCodecType_001
 * @tc.desc: Test OnSummateEventInfo handles missing codec type correctly.
 *           When neither CODEC_TYPE nor VIDEO_CODEC_TYPE is set,
 *           should skip processing.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_NoCodecType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "no_codec_type_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_VideoCodecSpecInfo_AllTypes_001
 * @tc.desc: Test OnSummateEventInfo handles all valid video codec types correctly.
 *           Verifies DECODER_SOFTWARE, ENCODER_SOFTWARE types are processed.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_VideoCodecSpecInfo_AllTypes_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "video_sw_dec_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_SOFTWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/av01");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);

    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "video_sw_enc_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::ENCODER_SOFTWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/av01");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_EmptyFeature_001
 * @tc.desc: Test OnAddEventInfo returns early when ADVANCED_FEATURE is empty.
 *           When feature string is empty, should return immediately without processing.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_EmptyFeature_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "empty_feature_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "");
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_NotSetFeature_001
 * @tc.desc: Test OnAddEventInfo returns early when ADVANCED_FEATURE is not set.
 *           When ADVANCED_FEATURE is not set, GetMetaString returns empty, should return.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_NotSetFeature_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "not_set_feature_test");
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_CBR_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with CBR mode correctly.
 *           Verifies RC_MODE is converted to "CBR" string.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_CBR_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_cbr_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::CBR));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_VBR_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with VBR mode correctly.
 *           Verifies RC_MODE is converted to "VBR" string.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_VBR_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_vbr_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::VBR));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_CQ_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with CQ mode correctly.
 *           Verifies RC_MODE is converted to "CQ" string.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_CQ_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_cq_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::CQ));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_SQR_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with SQR mode correctly.
 *           Verifies RC_MODE is converted to "SQR" string.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_SQR_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_sqr_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::SQR));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_CBR_HIGH_QUALITY_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with CBR_HIGH_QUALITY mode correctly.
 *           Verifies RC_MODE is converted to "CBR_HIGH_QUALITY" string.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_CBR_HIGH_QUALITY_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_cbr_hq_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(),
                   static_cast<int32_t>(VideoEncodeBitrateMode::CBR_HIGH_QUALITY));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_CBR_VIDEOCALL_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with CBR_VIDEOCALL mode correctly.
 *           Verifies RC_MODE is converted to "CBR_VIDEOCALL" string.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_CBR_VIDEOCALL_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_cbr_videocall_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::CBR_VIDEOCALL));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_CRF_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with CRF mode correctly.
 *           Verifies RC_MODE is converted to "CRF" string.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_CRF_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_crf_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::CRF));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_Unknown_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature with unknown mode value.
 *           When rcMode value is not in the enum range, should convert to "UNKNOWN".
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_Unknown_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_unknown_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(99)); // 99 is an unknown mode
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_RcMode_NotSetRcMode_001
 * @tc.desc: Test OnAddEventInfo handles RC_MODE feature without RC_MODE data.
 *           When feature is "RC_MODE" but RC_MODE data is not set, featurePending should be empty.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_RcMode_NotSetRcMode_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "rc_mode_not_set_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_NonRcMode_001
 * @tc.desc: Test OnAddEventInfo handles non-RC_MODE feature correctly.
 *           When feature is not "RC_MODE", featurePending should remain empty.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_NonRcMode_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "non_rc_mode_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "SOME_OTHER_FEATURE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::CBR));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_EmptyAppName_001
 * @tc.desc: Test OnAddEventInfo returns early when app name is empty.
 *           When AV_CODEC_CALLER_PROCESS_NAME is empty, appIndex should be INVALID_APP_NAME_INDEX.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_EmptyAppName_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::CBR));
    SubmitEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO);
}

/**
 * @tc.name: AdvancedFeatureInfo_MultipleFeatures_001
 * @tc.desc: Test OnAddEventInfo handles multiple different features correctly.
 *           Verifies different features are tracked separately.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_MultipleFeatures_001, TestSize.Level1)
{
    std::vector<std::string> features = {
        "FEATURE_A",
        "FEATURE_B",
        "FEATURE_C"
    };
    for (const auto &feature : features) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "multiple_features_test");
        meta.SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), feature);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: AdvancedFeatureInfo_IncrementCount_001
 * @tc.desc: Test OnAddEventInfo increments count correctly for same feature.
 *           When adding same feature multiple times, count should increment.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_IncrementCount_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "increment_test");
    meta_->SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
    meta_->SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::CBR));
    for (int i = 0; i < 3; ++i) {
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: AdvancedFeatureInfo_DifferentApps_001
 * @tc.desc: Test OnAddEventInfo handles features from different apps correctly.
 *           Verifies features from different apps are tracked separately.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, AdvancedFeatureInfo_DifferentApps_001, TestSize.Level1)
{
    std::vector<std::string> appNames = {
        "app_1",
        "app_2",
        "app_3"
    };
    for (const auto &appName : appNames) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, appName);
        meta.SetData(EventInfoExtentedKey::ADVANCED_FEATURE.data(), "RC_MODE");
        meta.SetData(EventInfoExtentedKey::RC_MODE.data(), static_cast<int32_t>(VideoEncodeBitrateMode::VBR));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::ADVANCED_FEATURE_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnSummateEventInfo_InstanceUsage_001
 * @tc.desc: Test OnSummateEventInfo aggregates codec instance usage info correctly.
 *           Verifies instance create action increments currentCount.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_InstanceUsage_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "instance_usage_test");
    meta_->SetData("INSTANCE_ACTION", "create");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_INSTANCE_USAGE_INFO, *meta_);
    SubmitEventInfo(StatisticsEventType::CODEC_INSTANCE_USAGE_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_InstanceUsage_Release_001
 * @tc.desc: Test OnSummateEventInfo handles instance release action correctly.
 *           Verifies release action decrements currentCount.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_InstanceUsage_Release_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "instance_release_test");
    meta_->SetData("INSTANCE_ACTION", "create");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_INSTANCE_USAGE_INFO, *meta_);
    meta_->SetData("INSTANCE_ACTION", "release");
    SubmitEventInfo(StatisticsEventType::CODEC_INSTANCE_USAGE_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_ResolutionSpec_001
 * @tc.desc: Test OnSummateEventInfo aggregates resolution specification info correctly.
 *           Verifies resolution level is resolved and recorded.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_ResolutionSpec_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "resolution_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1920);
    meta_->SetData("HEIGHT", 1080);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_ContainerFormat_Muxer_001
 * @tc.desc: Test OnSummateEventInfo aggregates muxer format info correctly.
 *           Verifies muxer format takes priority over demuxer format.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_ContainerFormat_Muxer_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "muxer_format_test");
    meta_->SetData("MUXER_FORMAT", "mp4");
    meta_->SetData("DEMUXER_FORMAT", "mkv");
    SubmitEventInfo(StatisticsEventType::CONTAINER_FORMAT_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_ContainerFormat_Demuxer_001
 * @tc.desc: Test OnSummateEventInfo aggregates demuxer format info when muxer format is empty.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_ContainerFormat_Demuxer_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "demuxer_format_test");
    meta_->SetData("DEMUXER_FORMAT", "mkv");
    SubmitEventInfo(StatisticsEventType::CONTAINER_FORMAT_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_CodecMode_Encoder_001
 * @tc.desc: Test OnSummateEventInfo aggregates codec mode info for encoder correctly.
 *           Verifies encoder mode is identified and recorded.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_CodecMode_Encoder_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "encoder_mode_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("CODEC_MODE", "HW");
    meta_->SetData("IS_ENCODER", true);
    SubmitEventInfo(StatisticsEventType::CODEC_MODE_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_CodecMode_Decoder_001
 * @tc.desc: Test OnSummateEventInfo aggregates codec mode info for decoder correctly.
 *           Verifies decoder mode is identified and recorded.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_CodecMode_Decoder_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "decoder_mode_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("CODEC_MODE", "HW");
    meta_->SetData("IS_ENCODER", false);
    SubmitEventInfo(StatisticsEventType::CODEC_MODE_INFO);
}

/**
 * @tc.name: OnSummateEventInfo_MultipleEventTypes_001
 * @tc.desc: Test OnSummateEventInfo aggregates multiple different event types correctly.
 *           Verifies all event types are processed and included in the summary.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnSummateEventInfo_MultipleEventTypes_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "multi_event_test");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_QUERY_CAP_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_INFO, *meta_);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO, *meta_);
    SubmitEventInfo(StatisticsEventType::CODEC_MODE_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_AudioDecoder_NoMimeType_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles audio decoder without mime type correctly.
 *           When CODEC_TYPE is AVCODEC_TYPE_AUDIO_DECODER but MIME_TYPE is empty,
 *           should skip processing in OnAudioCodecSpecInfo.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_AudioDecoder_NoMimeType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "audio_dec_no_mime_test");
    meta_->SetData("CODEC_TYPE", std::to_string(AVCODEC_TYPE_AUDIO_DECODER));
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_VideoCodec_InvalidType_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles invalid video codec type correctly.
 *           When VIDEO_CODEC_TYPE is negative or out of valid range,
 *           should skip processing.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_VideoCodec_InvalidType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_video_codec_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(-1));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_AllValidVideoCodecTypes_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles all valid video codec types correctly.
 *           Verifies DECODER_HARDWARE, DECODER_SOFTWARE, ENCODER_HARDWARE,
 *           ENCODER_SOFTWARE are all processed correctly.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_AllValidVideoCodecTypes_001, TestSize.Level1)
{
    Media::Meta meta1;
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "test_codec_types");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "video/av01");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);

    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_SOFTWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/hevc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "video/mp4v-es");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_BoundaryValues_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles boundary values correctly.
 *           Verifies UNKNOWN (0) and END (5) codec types are rejected.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_BoundaryValues_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "boundary_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::UNKNOWN));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);

    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::END));
    meta_->SetData(Tag::MIME_TYPE, "video/hevc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_IncrementCount_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo increments count correctly for same codec type.
 *           When creating same codec type multiple times, count should increment.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_IncrementCount_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "increment_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    for (int i = 0; i < 3; ++i) {
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeNotSet_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE is not set.
 *           When MIME_TYPE cannot be retrieved, should return immediately.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeNotSet_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "no_mime_test");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeEmpty_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE is empty string.
 *           Empty MIME_TYPE should fail IsValidMimeType length check (< 7).
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeEmpty_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "empty_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeTooShort_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE length is less than 7.
 *           MIME_TYPE shorter than minMediaMimeTypeLength should be invalid.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeTooShort_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "short_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "v/avc");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeTooLong_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE length exceeds 127.
 *           MIME_TYPE longer than maxMediaMimeTypeLength should be invalid.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeTooLong_001, TestSize.Level1)
{
    std::string longMimeType = "video/";
    while (longMimeType.size() < 128) {
        longMimeType += "a";
    }

    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "long_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, longMimeType);
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeNoSlash_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE has no '/' separator.
 *           MIME_TYPE without '/' should fail validation.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeNoSlash_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "no_slash_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "videoavc");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeSlashAtStart_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE has '/' at the start.
 *           MIME_TYPE with '/' at position 0 should fail validation.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeSlashAtStart_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "slash_start_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "/avc");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeSlashAtEnd_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE has '/' at the end.
 *           MIME_TYPE with '/' at the last position should fail validation.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeSlashAtEnd_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "slash_end_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "video/");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeInvalidPrefix_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE prefix is not "video" or "audio".
 *           MIME_TYPE with invalid top-level type should fail validation.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeInvalidPrefix_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_prefix_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "image/avc");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeInvalidChars_001
 * @tc.desc: Test OnAddEventInfo returns early when MIME_TYPE contains invalid characters.
 *           Sub-type with characters other than alnum, '-', '+', '/', '.' should fail validation.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeInvalidChars_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_chars_mime_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc#test");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_IsEncoderNotSet_001
 * @tc.desc: Test OnAddEventInfo returns early when IS_ENCODER is not set.
 *           When IS_ENCODER cannot be retrieved, should return immediately.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_IsEncoderNotSet_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "no_is_encoder_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_ValidVideoMimeType_001
 * @tc.desc: Test OnAddEventInfo processes valid video MIME types correctly.
 *           Verifies various valid video MIME types are accepted.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_ValidVideoMimeType_001, TestSize.Level1)
{
    std::vector<std::string> validVideoMimes = {
        "video/avc",
        "video/hevc",
        "video/av01",
        "video/mp4v-es",
        "video/x-vnd.on2.vp8",
        "video/x-vnd.on2.vp9"
    };
    for (const auto &mime : validVideoMimes) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "valid_video_mime_test");
        meta.SetData(Media::Tag::MIME_TYPE, mime);
        meta.SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CapUnsupportedInfo_ValidAudioMimeType_001
 * @tc.desc: Test OnAddEventInfo processes valid audio MIME types correctly.
 *           Verifies various valid audio MIME types are accepted.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_ValidAudioMimeType_001, TestSize.Level1)
{
    std::vector<std::string> validAudioMimes = {
        "audio/mp4a-latm",
        "audio/mpeg",
        "audio/vorbis",
        "audio/opus",
        "audio/flac",
        "audio/amr-wb"
    };
    for (const auto &mime : validAudioMimes) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "valid_audio_mime_test");
        meta.SetData(Media::Tag::MIME_TYPE, mime);
        meta.SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CapUnsupportedInfo_MimeTypeCaseInsensitive_001
 * @tc.desc: Test OnAddEventInfo handles case-insensitive top-level type.
 *           Verifies "VIDEO/avc" and "Video/avc" are accepted (converted to lowercase).
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_MimeTypeCaseInsensitive_001, TestSize.Level1)
{
    std::vector<std::string> mixedCaseMimes = {
        "VIDEO/avc",
        "Video/avc",
        "AUDIO/mp4a-latm",
        "Audio/mp4a-latm"
    };
    for (const auto &mime : mixedCaseMimes) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "case_insensitive_mime_test");
        meta.SetData(Media::Tag::MIME_TYPE, mime);
        meta.SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CapUnsupportedInfo_BothEventTypes_001
 * @tc.desc: Test OnAddEventInfo handles both CAP_UNSUPPORTED event types.
 *           Verifies CAP_UNSUPPORTED_QUERY_CAP_INFO and CAP_UNSUPPORTED_CREATE_CODEC_INFO.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_BothEventTypes_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "both_events_test");
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), false);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_QUERY_CAP_INFO, *meta_);

    meta_->SetData(Media::Tag::MIME_TYPE, "audio/mp4a-latm");
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_CREATE_CODEC_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_ValidCodecError_001
 * @tc.desc: Test OnSummateEventInfo correctly processes valid codec error info.
 *           Verifies valid VideoCodecType, mimeType and errorCode are added to JSON.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_ValidCodecError_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "valid_codec_error_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1001));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                  static_cast<int32_t>(VideoCodecType::ENCODER_SOFTWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/hevc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1002));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_VideoCodecTypeUnknown_001
 * @tc.desc: Test OnSummateEventInfo skips entries with VideoCodecType::UNKNOWN.
 *           Entries with UNKNOWN codec type should not be added to JSON.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_VideoCodecTypeUnknown_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "unknown_codec_type_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), static_cast<int32_t>(VideoCodecType::UNKNOWN));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1001));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_VideoCodecTypeEnd_001
 * @tc.desc: Test OnSummateEventInfo skips entries with VideoCodecType::END.
 *           Entries with END codec type should not be added to JSON.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_VideoCodecTypeEnd_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "end_codec_type_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), static_cast<int32_t>(VideoCodecType::END));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1001));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_InvalidVideoCodecType_001
 * @tc.desc: Test OnSummateEventInfo skips entries with invalid VideoCodecType.
 *           When vcodecType is not in VIDEO_CODEC_TYPE_TO_STRING map, should skip.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_InvalidVideoCodecType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_codec_type_map_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), static_cast<int32_t>(5)); // 5: invalid codec type
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1001));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_EmptyMimeType_001
 * @tc.desc: Test OnSummateEventInfo skips entries with empty mimeType.
 *           Entries with empty mime type should not be added to JSON.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_EmptyMimeType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "empty_mime_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1001));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_AllValidVideoCodecTypes_001
 * @tc.desc: Test OnSummateEventInfo handles all valid VideoCodecType values correctly.
 *           Verifies DECODER_HARDWARE, DECODER_SOFTWARE, ENCODER_HARDWARE, ENCODER_SOFTWARE.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_AllValidVideoCodecTypes_001, TestSize.Level1)
{
    std::vector<VideoCodecType> validCodecTypes = {
        VideoCodecType::DECODER_HARDWARE,
        VideoCodecType::DECODER_SOFTWARE,
        VideoCodecType::ENCODER_HARDWARE,
        VideoCodecType::ENCODER_SOFTWARE
    };
    for (const auto &codecType : validCodecTypes) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "all_valid_codec_types_test");
        meta.SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), static_cast<int32_t>(codecType));
        meta.SetData(Media::Tag::MIME_TYPE, "video/avc");
        meta.SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-2001));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_DifferentMimeTypes_001
 * @tc.desc: Test OnSummateEventInfo handles different mime types correctly.
 *           Verifies different mime types generate different JSON keys.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_DifferentMimeTypes_001, TestSize.Level1)
{
    std::vector<std::string> mimeTypes = {
        "video/avc",
        "video/hevc",
        "video/av01",
        "video/mp4v-es"
    };
    for (const auto &mime : mimeTypes) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "different_mime_test");
        meta.SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                     static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
        meta.SetData(Media::Tag::MIME_TYPE, mime);
        meta.SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-3001));
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_PositiveErrorCode_001
 * @tc.desc: Test OnSummateEventInfo handles different error codes correctly.
 *           Verifies different error codes generate different JSON keys.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_DifferentErrorCodes_001, TestSize.Level1)
{
    std::vector<int32_t> errorCodes = {
        -1001, -1002, -2001, -3001, -10100
    };
    for (const auto &errorCode : errorCodes) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "different_error_code_test");
        meta.SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                     static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
        meta.SetData(Media::Tag::MIME_TYPE, "video/avc");
        meta.SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), errorCode);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_IncrementCount_001
 * @tc.desc: Test OnSummateEventInfo increments count correctly for same codec error.
 *           When adding same codec error multiple times, count should increment.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_IncrementCount_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "increment_count_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1001));
    for (int i = 0; i < 5; ++i) {
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_PositiveErrorCode_001
 * @tc.desc: Test OnSummateEventInfo handles positive error code correctly.
 *           Verifies positive error codes are processed correctly.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_PositiveErrorCode_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "positive_error_code_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::ENCODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/hevc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(1001));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_ZeroErrorCode_001
 * @tc.desc: Test OnSummateEventInfo handles zero error code correctly.
 *           Verifies zero error code is processed correctly.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_ZeroErrorCode_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "zero_error_code_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::ENCODER_SOFTWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/av01");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(0));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}

/**
 * @tc.name: CodecErrorInfo_OnSummate_MixedScenarios_001
 * @tc.desc: Test OnSummateEventInfo handles mixed valid and invalid scenarios.
 *           Verifies only valid entries are added to JSON.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CodecErrorInfo_OnSummate_MixedScenarios_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "mixed_scenarios_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-1001));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CODEC_ERROR_INFO, *meta_);

    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::ENCODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/hevc");
    meta_->SetData(EventInfoExtentedKey::CODEC_ERROR_CODE.data(), static_cast<int32_t>(-2001));
    SubmitEventInfo(StatisticsEventType::CODEC_ERROR_INFO);
}


/**
 * @tc.name: OnBasicCreateCodecSpecInfo_VideoEncoder_Secure_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles secure video encoder correctly.
 *           Verifies isSecure flag is set to true for encoder with "secure" in codec name.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_VideoEncoder_Secure_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "secure_encoder_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::ENCODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/hevc");
    meta_->SetData(Tag::MEDIA_CODEC_NAME, "OMX.hisi.video.encoder.hevc.secure");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_NoCodecName_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles missing codec name correctly.
 *           When MEDIA_CODEC_NAME is not set, isSecure should be false.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_NoCodecName_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "no_codec_name_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_AudioEncoder_NoMimeType_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles audio encoder without mime type correctly.
 *           When CODEC_TYPE is AVCODEC_TYPE_AUDIO_ENCODER but MIME_TYPE is empty,
 *           should skip processing in OnAudioCodecSpecInfo.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_AudioEncoder_NoMimeType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "audio_enc_no_mime_test");
    meta_->SetData("CODEC_TYPE", std::to_string(AVCODEC_TYPE_AUDIO_ENCODER));
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_VideoCodec_NoMimeType_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles video codec without mime type correctly.
 *           When VIDEO_CODEC_TYPE is valid but MIME_TYPE is empty,
 *           should skip processing.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_VideoCodec_NoMimeType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "video_no_mime_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_VideoCodec_NoVideoCodecType_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles missing video codec type correctly.
 *           When VIDEO_CODEC_TYPE is not set, should skip processing.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_VideoCodec_NoVideoCodecType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "no_video_codec_type_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnBasicCreateCodecSpecInfo_DifferentMimeTypes_001
 * @tc.desc: Test OnBasicCreateCodecSpecInfo handles different mime types correctly.
 *           Verifies different mime types are tracked separately.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnBasicCreateCodecSpecInfo_DifferentMimeTypes_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "different_mime_test");
    meta_->SetData("VIDEO_CODEC_TYPE", static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "video/hevc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "video/av01");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_InvalidWidth_001
 * @tc.desc: Test ResolveResolutionLevel returns "OTHER" when width is not set.
 *           Should return "OTHER" when width data cannot be retrieved.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_InvalidWidth_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_width_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("HEIGHT", 1080);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_InvalidHeight_001
 * @tc.desc: Test ResolveResolutionLevel returns "OTHER" when height is not set.
 *           Should return "OTHER" when height data cannot be retrieved.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_InvalidHeight_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "invalid_height_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1920);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_NegativeWidth_001
 * @tc.desc: Test ResolveResolutionLevel returns "OTHER" when width is negative.
 *           Should return "OTHER" when width <= 0.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_NegativeWidth_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "negative_width_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", -1920);
    meta_->SetData("HEIGHT", 1080);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_NegativeHeight_001
 * @tc.desc: Test ResolveResolutionLevel returns "OTHER" when height is negative.
 *           Should return "OTHER" when height <= 0.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_NegativeHeight_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "negative_height_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1920);
    meta_->SetData("HEIGHT", -1080);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_ZeroWidth_001
 * @tc.desc: Test ResolveResolutionLevel returns "OTHER" when width is zero.
 *           Should return "OTHER" when width <= 0.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_ZeroWidth_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "zero_width_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 0);
    meta_->SetData("HEIGHT", 1080);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_ZeroHeight_001
 * @tc.desc: Test ResolveResolutionLevel returns "OTHER" when height is zero.
 *           Should return "OTHER" when height <= 0.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_ZeroHeight_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "zero_height_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1920);
    meta_->SetData("HEIGHT", 0);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: OnAddEventInfo_ExistingEventType_001
 * @tc.desc: Test OnAddEventInfo processes event when eventType exists in eventInfoMap_.
 *           Verifies event handler's OnAddEventInfo is called for valid event types.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_ExistingEventType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "existing_event_type_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnAddEventInfo_NonExistingEventType_001
 * @tc.desc: Test OnAddEventInfo handles non-existing event type gracefully.
 *           When eventType is not in eventInfoMap_, no handler should be called.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_NonExistingEventType_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "non_existing_event_type_test");
    SubmitEventInfo(static_cast<StatisticsEventType>(0xFFFF));
}

/**
 * @tc.name: OnAddEventInfo_EventHookNotExist_001
 * @tc.desc: Test OnAddEventInfo handles case when no event hook is registered.
 *           When eventHooks_ doesn't contain the eventType, should continue normally.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_EventHookNotExist_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "no_event_hook_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: OnAddEventInfo_EventHookReturnsTrue_001
 * @tc.desc: Test OnAddEventInfo removes event hook when hook returns true.
 *           When EventHook returns true, the hook should be erased from eventHooks_.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_EventHookReturnsTrue_001, TestSize.Level1)
{
    bool hookCalled = false;
    StatisticsEventType testEventType = static_cast<StatisticsEventType>(0x1234);
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        testEventType, [&hookCalled](const Media::Meta &meta) -> bool {
            hookCalled = true;
            return true;
        });

    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "hook_returns_true_test");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(testEventType, *meta_);
    ASSERT_EQ(true, hookCalled);
    hookCalled = false;
    StatisticsEventInfo::GetInstance().OnAddEventInfo(testEventType, *meta_);
    ASSERT_EQ(false, hookCalled);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnAddEventInfo_EventHookReturnsFalse_001
 * @tc.desc: Test OnAddEventInfo keeps event hook when hook returns false.
 *           When EventHook returns false, the hook should remain in eventHooks_.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_EventHookReturnsFalse_001, TestSize.Level1)
{
    int hookCallCount = 0;
    StatisticsEventType testEventType = static_cast<StatisticsEventType>(0x2345);
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        testEventType, [&hookCallCount](const Media::Meta &meta) -> bool {
            hookCallCount++;
            return false;
        });

    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "hook_returns_false_test");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(testEventType, *meta_);
    ASSERT_EQ(1, hookCallCount);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(testEventType, *meta_);
    ASSERT_EQ(2, hookCallCount);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(testEventType, *meta_);
    ASSERT_EQ(3, hookCallCount);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnAddEventInfo_MultipleEventHooks_001
 * @tc.desc: Test OnAddEventInfo handles multiple event hooks for different event types.
 *           Verifies each event type has its own independent hook.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_MultipleEventHooks_001, TestSize.Level1)
{
    bool hook1Called = false;
    bool hook2Called = false;
    StatisticsEventType eventType1 = static_cast<StatisticsEventType>(0x3456);
    StatisticsEventType eventType2 = static_cast<StatisticsEventType>(0x4567);
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        eventType1, [&hook1Called](const Media::Meta &meta) -> bool {
            hook1Called = true;
            return true;
        });
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        eventType2, [&hook2Called](const Media::Meta &meta) -> bool {
            hook2Called = true;
            return true;
        });

    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "multiple_hooks_test");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(eventType1, *meta_);
    ASSERT_EQ(true, hook1Called);
    ASSERT_EQ(false, hook2Called);
    hook1Called = false;
    hook2Called = false;
    StatisticsEventInfo::GetInstance().OnAddEventInfo(eventType2, *meta_);
    ASSERT_EQ(false, hook1Called);
    ASSERT_EQ(true, hook2Called);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnAddEventInfo_EventHookWithMetaData_001
 * @tc.desc: Test OnAddEventInfo passes Meta data to event hook correctly.
 *           Verifies event hook receives the correct Meta object.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_EventHookWithMetaData_001, TestSize.Level1)
{
    std::string receivedProcessName;
    StatisticsEventType testEventType = static_cast<StatisticsEventType>(0x5678);
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        testEventType, [&receivedProcessName](const Media::Meta &meta) -> bool {
            meta.GetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, receivedProcessName);
            return true;
        });

    std::string expectedProcessName = "test_process_for_hook";
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, expectedProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(testEventType, *meta_);
    ASSERT_EQ(expectedProcessName, receivedProcessName);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnAddEventInfo_EventHookAndEventHandler_001
 * @tc.desc: Test OnAddEventInfo processes both event handler and event hook.
 *           Verifies both eventInfoMap_ handler and eventHooks_ hook are executed.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_EventHookAndEventHandler_001, TestSize.Level1)
{
    bool hookCalled = false;
    StatisticsEventType testEventType = static_cast<StatisticsEventType>(0x6789);
    StatisticsEventInfo::GetInstance().RegisterEventHook(
        testEventType, [&hookCalled](const Media::Meta &meta) -> bool {
            hookCalled = true;
            return true;
        });

    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "both_handler_and_hook_test");
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                   static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
    meta_->SetData(Media::Tag::MIME_TYPE, "video/avc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(testEventType, *meta_);
    ASSERT_EQ(true, hookCalled);
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: OnAddEventInfo_DifferentMainEventTypes_001
 * @tc.desc: Test OnAddEventInfo handles different main event types correctly.
 *           Verifies event type masking works properly with MAIN_EVENT_TYPE_MASK.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, OnAddEventInfo_DifferentMainEventTypes_001, TestSize.Level1)
{
    std::vector<StatisticsEventType> eventTypes = {
        StatisticsEventType::BASIC_INFO,
        StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO,
        StatisticsEventType::CODEC_ERROR_INFO,
        StatisticsEventType::ADVANCED_FEATURE_INFO
    };
    for (const auto &eventType : eventTypes) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "different_main_event_types_test");
        meta.SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(),
                     static_cast<int32_t>(VideoCodecType::DECODER_HARDWARE));
        meta.SetData(Media::Tag::MIME_TYPE, "video/avc");
        StatisticsEventInfo::GetInstance().OnAddEventInfo(eventType, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: ResolveResolutionLevel_360P_001
 * @tc.desc: Test ResolveResolutionLevel returns "360P" for 360P resolution.
 *           Verifies 640x360 (16:9) resolution is classified as 360P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_360P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "360p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 640);
    meta_->SetData("HEIGHT", 360);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_480P_001
 * @tc.desc: Test ResolveResolutionLevel returns "480P" for 480P resolution.
 *           Verifies 854x480 (16:9) resolution is classified as 480P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_480P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "480p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 854);
    meta_->SetData("HEIGHT", 480);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_576P_001
 * @tc.desc: Test ResolveResolutionLevel returns "576P" for 576P resolution.
 *           Verifies 1024x576 (16:9) resolution is classified as 576P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_576P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "576p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1024);
    meta_->SetData("HEIGHT", 576);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_720P_001
 * @tc.desc: Test ResolveResolutionLevel returns "720P" for 720P resolution.
 *           Verifies 1280x720 (16:9) resolution is classified as 720P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_720P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "720p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1280);
    meta_->SetData("HEIGHT", 720);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_1080P_001
 * @tc.desc: Test ResolveResolutionLevel returns "1080P" for 1080P resolution.
 *           Verifies 1920x1080 (16:9) resolution is classified as 1080P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_1080P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "1080p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1920);
    meta_->SetData("HEIGHT", 1080);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_1440P_001
 * @tc.desc: Test ResolveResolutionLevel returns "1440P" for 1440P resolution.
 *           Verifies 2560x1440 (16:9) resolution is classified as 1440P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_1440P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "1440p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 2560);
    meta_->SetData("HEIGHT", 1440);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_2160P_001
 * @tc.desc: Test ResolveResolutionLevel returns "2160P" for 2160P (4K) resolution.
 *           Verifies 3840x2160 (16:9) resolution is classified as 2160P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_2160P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "2160p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 3840);
    meta_->SetData("HEIGHT", 2160);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_4320P_001
 * @tc.desc: Test ResolveResolutionLevel returns "4320P" for 4320P (8K) resolution.
 *           Verifies 7680x4320 (16:9) resolution is classified as 4320P.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_4320P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "4320p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 7680);
    meta_->SetData("HEIGHT", 4320);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_Exceeds4320P_001
 * @tc.desc: Test ResolveResolutionLevel returns "OTHER" when resolution exceeds 4320P.
 *           Verifies resolution larger than 8K is classified as "OTHER".
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_Exceeds4320P_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "exceeds_4320p_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 15360);
    meta_->SetData("HEIGHT", 8640);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_BoundaryValues_001
 * @tc.desc: Test ResolveResolutionLevel handles boundary values between resolution levels.
 *           Verifies exact boundary values are classified correctly.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_BoundaryValues_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "boundary_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 640);  // width: 640
    meta_->SetData("HEIGHT", 360); // height: 360
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO, *meta_);
    meta_->SetData("WIDTH", 1280); // width: 1280
    meta_->SetData("HEIGHT", 720); // height: 720
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO, *meta_);
    meta_->SetData("WIDTH", 1920);  // width: 1920
    meta_->SetData("HEIGHT", 1080); // height: 1080
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_AllResolutionLevels_001
 * @tc.desc: Test ResolveResolutionLevel handles all resolution levels correctly.
 *           Verifies all standard resolution levels are processed.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_AllResolutionLevels_001, TestSize.Level1)
{
    struct ResolutionTestCase {
        int32_t width;
        int32_t height;
        std::string testName;
    };
    std::vector<ResolutionTestCase> testCases = {
        {640, 360, "360p"},
        {854, 480, "480p"},
        {1024, 576, "576p"},
        {1280, 720, "720p"},
        {1920, 1080, "1080p"},
        {2560, 1440, "1440p"},
        {3840, 2160, "2160p"},
        {7680, 4320, "4320p"}
    };
    for (const auto &testCase : testCases) {
        Media::Meta meta;
        meta.SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "all_resolutions_test");
        meta.SetData(Tag::MIME_TYPE, "video/avc");
        meta.SetData("WIDTH", testCase.width);
        meta.SetData("HEIGHT", testCase.height);
        StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO, meta);
    }
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: ResolveResolutionLevel_Encoder_001
 * @tc.desc: Test ResolveResolutionLevel works correctly for encoder scenarios.
 *           Verifies encoder resolution spec info is processed with "Enc" prefix.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_Encoder_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "encoder_resolution_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1920);
    meta_->SetData("HEIGHT", 1080);
    meta_->SetData("IS_ENCODER", true);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_Decoder_001
 * @tc.desc: Test ResolveResolutionLevel works correctly for decoder scenarios.
 *           Verifies decoder resolution spec info is processed with "Dec" prefix.
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_Decoder_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "decoder_resolution_test");
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData("WIDTH", 1920);
    meta_->SetData("HEIGHT", 1080);
    meta_->SetData("IS_ENCODER", false);
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

/**
 * @tc.name: GetIndexByMeta_Param_Test_001
 * @tc.desc: only caller process param test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, GetIndexByMeta_Param_Test_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO, *meta_);
    std::string callerProcessName = "callerProcess";
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO, *meta_);
    std::string forwardProcessName = "forwardcallerProcess";
    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    SubmitEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO);
}

/**
 * @tc.name: GetIndexByMeta_Param_Test_002
 * @tc.desc: only forward caller process param test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, GetIndexByMeta_Param_Test_002, TestSize.Level1)
{
    std::string forwardProcessName = "forwardcallerProcess";
    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    SubmitEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO);
}

/**
 * @tc.name: GetIndexByMeta_Param_Test_003
 * @tc.desc: caller process and forward caller process param test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, GetIndexByMeta_Param_Test_003, TestSize.Level1)
{
    constexpr int32_t pid = 123;
    const std::string callerProcessName = GenerateRandomString(237);
    const std::string forwardProcessName = "forwardcallerProcess";

    meta_->SetData(Tag::AV_CODEC_CALLER_PID, pid);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO, *meta_);
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, callerProcessName);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO, *meta_);
    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    SubmitEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO);
}

/**
 * @tc.name: GetIndexByMeta_Param_Test_004
 * @tc.desc: only forward caller process param test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, GetIndexByMeta_Param_Test_004, TestSize.Level1)
{
    constexpr int32_t pid = 123;
    const std::string forwardProcessName = "forwardcallerProcess";
    meta_->SetData(Tag::AV_CODEC_CALLER_PID, pid);
    meta_->SetData(Tag::AV_CODEC_FORWARD_CALLER_PROCESS_NAME, forwardProcessName);
    SubmitEventInfo(StatisticsEventType::SECURE_DEC_ALLOC_BUF_FAILED_INFO);
}

/**
 * @tc.name: BasicInfo_Submit_Test_001
 * @tc.desc: OnSubmitEventInfo coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, BasicInfo_Submit_Test_001, TestSize.Level1)
{
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData(Tag::MEDIA_CODEC_NAME, "OMX.hisi.video.encoder.avc.secure");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: BasicInfo_Submit_Test_002
 * @tc.desc: OnSubmitEventInfo coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, BasicInfo_Submit_Test_002, TestSize.Level1)
{
    meta_->SetData(EventInfoExtentedKey::VIDEO_CODEC_TYPE.data(), VideoCodecType::ENCODER_HARDWARE);
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    meta_->SetData(Tag::MEDIA_CODEC_NAME, "OMX.hisi.video.encoder.avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: BasicCreateCodecSpec_Test_001
 * @tc.desc: OnBasicCreateCodecSpec coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, BasicCreateCodecSpec_Test_001, TestSize.Level1)
{
    meta_->SetData(EventInfoExtentedKey::CODEC_TYPE.data(), std::to_string(AVCodecType::AVCODEC_TYPE_AUDIO_ENCODER));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    meta_->SetData(EventInfoExtentedKey::CODEC_TYPE.data(), std::to_string(AVCodecType::AVCODEC_TYPE_VIDEO_ENCODER));
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO, *meta_);
    meta_->SetData(EventInfoExtentedKey::CODEC_TYPE.data(), std::to_string(AVCodecType::AVCODEC_TYPE_AUDIO_DECODER));
    meta_->SetData(Tag::MIME_TYPE, "audio/avc");
    SubmitEventInfo(StatisticsEventType::BASIC_CREATE_CODEC_SPEC_INFO);
}

/**
 * @tc.name: ResolveResolutionLevel_Test_001
 * @tc.desc: ResolveResolutionLevel coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_Test_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO, *meta_);
    meta_->SetData(EventInfoExtentedKey::RESOLUTION_LEVEL.data(), "resolution_level");
    SubmitEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO);
}

void AddSpecInfo(std::shared_ptr<Media::Meta> meta, int32_t param)
{
    meta->SetData(EventInfoExtentedKey::WIDTH.data(), param);
    meta->SetData(EventInfoExtentedKey::HEIGHT.data(), param);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::RESOLUTION_SPEC_INFO, *meta);
}

/**
 * @tc.name: ResolveResolutionLevel_Test_002
 * @tc.desc: ResolveResolutionLevel coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, ResolveResolutionLevel_Test_002, TestSize.Level1)
{
    AddSpecInfo(meta_, 360);  // 360: less than resolution360P
    AddSpecInfo(meta_, 500);  // 500: less than resolution480P
    AddSpecInfo(meta_, 700);  // 700: less than resolution576P
    AddSpecInfo(meta_, 800);  // 800: less than resolution720P
    AddSpecInfo(meta_, 1000); // 1000: less than resolution1080P
    AddSpecInfo(meta_, 1500); // 1500: less than resolution1440P
    AddSpecInfo(meta_, 2000); // 2000: less than resolution2160P
    AddSpecInfo(meta_, 3000); // 3000: less than resolution4320P
    AddSpecInfo(meta_, 6000); // 6000: greater than resolution4320P
    StatisticsEventInfo::GetInstance().OnSubmitEventInfo();
}

/**
 * @tc.name: CapUnsupportedInfo_AddEventInfo_Test_001
 * @tc.desc: AddEventInfo coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_AddEventInfo_Test_001, TestSize.Level1)
{
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "invalid_type");
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO);
}

/**
 * @tc.name: CapUnsupportedInfo_AddEventInfo_Test_002
 * @tc.desc: AddEventInfo coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, CapUnsupportedInfo_AddEventInfo_Test_002, TestSize.Level1)
{
    meta_->SetData(EventInfoExtentedKey::IS_ENCODER.data(), true);
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "video/avc");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO, *meta_);
    meta_->SetData(Tag::MIME_TYPE, "invalid_type");
    SubmitEventInfo(StatisticsEventType::CAP_UNSUPPORTED_INFO);
}

/**
 * @tc.name: IsEventValid_Test_001
 * @tc.desc: IsEventValid coverage test
 * @tc.type: FUNC
 */
HWTEST_F(TEST_SUIT, IsEventValid_Test_001, TestSize.Level1)
{
    meta_->SetData(Tag::AV_CODEC_CALLER_PROCESS_NAME, "is_event_valid_test");
    StatisticsEventInfo::GetInstance().OnAddEventInfo(StatisticsEventType::BASIC_QUERY_CAP_INFO, *meta_);
    ASSERT_TRUE(StatisticsEventInfo::GetInstance().IsEventValid());
    StatisticsEventInfo::GetInstance().eventInfoMap_.clear();
    StatisticsEventInfo::GetInstance().eventInfoMap_.emplace(StatisticsEventType::BASIC_INFO, nullptr);
    ASSERT_FALSE(StatisticsEventInfo::GetInstance().IsEventValid());
}