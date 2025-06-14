#ifndef VIDEOENC_FUNC_TEST_SUIT_H
#define VIDEOENC_FUNC_TEST_SUIT_H
#include "avcodec_log.h"
#include <iostream>
#ifdef VIDEOENC_ASYNC_UNIT_TEST
#include "venc_async_sample.h"
#else
#include "venc_sync_sample.h"
#endif

namespace VFTSUIT {
class TEST_SUIT : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    bool CreateVideoCodecByName(const std::string &decName);
    bool CreateVideoCodecByMime(const std::string &decMime);
    void CreateByNameWithParam(int32_t param);
    void SetFormatWithParam(int32_t param);
    void PrepareSource(int32_t param);
    bool ReadCustomDataToAVBuffer(const std::string &fileName, std::shared_ptr<AVBuffer> buffer);
    bool GetWaterMarkCapability(int32_t param);
    bool GetTemporalScalabilityCapability(int32_t param);
    bool GetTemporalScalabilityCapability(int32_t param, bool isTemporalScalability);
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_TEST, STRINGFY(TEST_SUIT)};

protected:
    std::shared_ptr<OHOS::MediaAVCodec::CodecListMock> capability_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VideoEncSample> videoEnc_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> format_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VEncCallbackTest> vencCallback_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VEncCallbackTestExt> vencCallbackExt_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VEncParamCallbackTest> vencParamCallback_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VEncParamWithAttrCallbackTest> vencParamWithAttrCallback_ = nullptr;
#ifdef VIDEOENC_CAPI_UNIT_TEST
    OH_AVCodec *codec_ = nullptr;
#endif
};

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    std::shared_ptr<OHOS::MediaAVCodec::VEncSignal> vencSignal = std::make_shared<OHOS::MediaAVCodec::VEncSignal>();
    vencCallback_ = std::make_shared<OHOS::MediaAVCodec::VEncCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencCallback_);

    vencCallbackExt_ = std::make_shared<OHOS::MediaAVCodec::VEncCallbackTestExt>(vencSignal);
    ASSERT_NE(nullptr, vencCallbackExt_);

    vencParamCallback_ = std::make_shared<OHOS::MediaAVCodec::VEncParamCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencParamCallback_);

    vencParamWithAttrCallback_ = std::make_shared<OHOS::MediaAVCodec::VEncParamWithAttrCallbackTest>(vencSignal);
    ASSERT_NE(nullptr, vencParamWithAttrCallback_);

    videoEnc_ = std::make_shared<OHOS::MediaAVCodec::VideoEncSample>(vencSignal);
    ASSERT_NE(nullptr, videoEnc_);

    format_ = OHOS::MediaAVCodec::FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
    std::cout << "testCaseName" << std::endl;
}

void TEST_SUIT::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoEnc_ = nullptr;
#ifdef VIDEOENC_CAPI_UNIT_TEST
    if (codec_ != nullptr) {
        EXPECT_EQ(AV_ERR_OK, OH_VideoEncoder_Destroy(codec_));
        codec_ = nullptr;
    }
#endif
}

bool TEST_SUIT::CreateVideoCodecByMime(const std::string &encMime)
{
    if (videoEnc_->CreateVideoEncMockByMime(encMime) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool TEST_SUIT::CreateVideoCodecByName(const std::string &name)
{
    if (videoEnc_->isAVBufferMode_) {
        if (videoEnc_->CreateVideoEncMockByName(name) == false ||
            videoEnc_->SetCallback(vencCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoEnc_->CreateVideoEncMockByName(name) == false || videoEnc_->SetCallback(vencCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}
} // namespace
#endif //VIDEOENC_FUNC_TEST_SUIT_H