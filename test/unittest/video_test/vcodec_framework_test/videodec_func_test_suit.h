#ifndef VIDEODEC_FUNC_TEST_SUIT_H
#define VIDEODEC_FUNC_TEST_SUIT_H
#include "avcodec_log.h"

#ifdef VIDEODEC_ASYNC_UNIT_TEST
#include "vdec_async_sample.h"
#else
#include "vdec_sync_sample.h"
#endif

namespace VFTSUIT {

#ifdef VIDEODEC_CAPI_UNIT_TEST
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
#endif

class TEST_SUIT : public testing::TestWithParam<int32_t> {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

    bool CreateVideoCodecByName(const std::string &decName);
    bool CreateVideoCodecByMime(const std::string &decMime);
    void CreateByNameWithParam(int32_t param);
    void CreateByNameWithParam(std::string_view param);
    void SetFormatWithParam(int32_t param);
    void PrepareSource(int32_t param);
    void PrepareSource(int32_t param, std::string sourcePath);
    static constexpr OHOS::HiviewDFX::HiLogLabel LABEL = {LOG_CORE, LOG_DOMAIN_FRAMEWORK, STRINGFY(TEST_SUIT)};

protected:
    std::shared_ptr<OHOS::MediaAVCodec::CodecListMock> capability_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VideoDecSample> videoDec_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::FormatMock> format_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VDecCallbackTest> vdecCallback_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VDecCallbackTestExt> vdecCallbackExt_ = nullptr;
    std::shared_ptr<OHOS::MediaAVCodec::VDecSignal> vdecSignal_ = nullptr;
#ifdef VIDEODEC_CAPI_UNIT_TEST
    OH_AVCodec *codec_ = nullptr;
#endif
};

void TEST_SUIT::TearDownTestCase(void) {}

void TEST_SUIT::SetUp(void)
{
    vdecSignal_ = std::make_shared<OHOS::MediaAVCodec::VDecSignal>();
    vdecCallback_ = std::make_shared<OHOS::MediaAVCodec::VDecCallbackTest>(vdecSignal_);
    ASSERT_NE(nullptr, vdecCallback_);

    vdecCallbackExt_ = std::make_shared<OHOS::MediaAVCodec::VDecCallbackTestExt>(vdecSignal_);
    ASSERT_NE(nullptr, vdecCallbackExt_);

    videoDec_ = std::make_shared<OHOS::MediaAVCodec::VideoDecSample>(vdecSignal_);
    ASSERT_NE(nullptr, videoDec_);

    format_ = OHOS::MediaAVCodec::FormatMockFactory::CreateFormat();
    ASSERT_NE(nullptr, format_);

    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testCaseName = testInfo_->name();
    AVCODEC_LOGI("%{public}s", testCaseName.c_str());
}

void TEST_SUIT::TearDown(void)
{
    if (format_ != nullptr) {
        format_->Destroy();
    }
    videoDec_ = nullptr;
#ifdef VIDEODEC_CAPI_UNIT_TEST
    if (codec_ != nullptr) {
        EXPECT_EQ(AV_ERR_OK, OH_VideoDecoder_Destroy(codec_));
        codec_ = nullptr;
    }
#endif
}

bool TEST_SUIT::CreateVideoCodecByMime(const std::string &decMime)
{
    if (videoDec_->CreateVideoDecMockByMime(decMime) == false || videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
        return false;
    }
    return true;
}

bool TEST_SUIT::CreateVideoCodecByName(const std::string &decName)
{
    if (videoDec_->isAVBufferMode_) {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallbackExt_) != AV_ERR_OK) {
            return false;
        }
    } else {
        if (videoDec_->CreateVideoDecMockByName(decName) == false ||
            videoDec_->SetCallback(vdecCallback_) != AV_ERR_OK) {
            return false;
        }
    }
    return true;
}
}

#endif //VIDEODEC_FUNC_TEST_SUIT_H