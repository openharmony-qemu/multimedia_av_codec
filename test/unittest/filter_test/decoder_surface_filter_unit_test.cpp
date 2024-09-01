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
#include <sys/stat.h>
#include <fcntl.h>
#include <cinttypes>
#include "gtest/gtest.h"
#include "avcodec_errors.h"
#include "avcodec_info.h"
#include "media_description.h"
#include "decoder_surface_filter_unit_test.h"
#include "video_decoder_adapter.h"
#include "common/log.h"
#include "parameters.h"

using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing::ext;

namespace {
constexpr OHOS::HiviewDFX::HiLogLabel LABEL = { LOG_ONLY_PRERELEASE, LOG_DOMAIN_SYSTEM_PLAYER,
                                                "DecoderSurfaceFilterUnitTest" };
}

namespace OHOS {
namespace Media {
namespace Pipeline {
class FilterMediaCodecCallback : public OHOS::MediaAVCodec::MediaCodecCallback {
public:
    explicit FilterMediaCodecCallback(std::shared_ptr<DecoderSurfaceFilter> decoderSurfaceFilter)
        : decoderSurfaceFilter_(decoderSurfaceFilter)
    {
    }

    ~FilterMediaCodecCallback() = default;

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->OnError(errorType, errorCode);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->OnOutputFormatChanged(format);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override {}

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer) override
    {
        if (auto decoderSurfaceFilter = decoderSurfaceFilter_.lock()) {
            decoderSurfaceFilter->DrainOutputBuffer(index, buffer);
        } else {
            MEDIA_LOG_I("invalid decoderSurfaceFilter");
        }
    }

private:
    std::weak_ptr<DecoderSurfaceFilter> decoderSurfaceFilter_;
};

class FilterLinkCallbackMock : public FilterLinkCallback {
    void OnLinkedResult(const sptr<AVBufferQueueProducer> &queue, std::shared_ptr<Meta> &meta)
    {
        (void)queue;
        (void)meta;
        return;
    }

    void OnUnlinkedResult(std::shared_ptr<Meta> &meta)
    {
        (void)meta;
    }

    void OnUpdatedResult(std::shared_ptr<Meta> &meta)
    {
        (void)meta;
    }
};

class VideoDecoderAdapterMock : public VideoDecoderAdapter {
public:
    explicit VideoDecoderAdapterMock() : VideoDecoderAdapter() {}

    int32_t SetCallback(const std::shared_ptr<MediaAVCodec::MediaCodecCallback> &callback)
    {
        callback_ = callback;
        return 0;
    }

    void OnInputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
    {
        FALSE_RETURN(callback_ != nullptr);
        callback_->OnInputBufferAvailable(index, buffer);
    }

    void OnError(MediaAVCodec::AVCodecErrorType errorType, int32_t errorCode)
    {
        FALSE_RETURN(callback_ != nullptr);
        callback_->OnError(errorType, errorCode);
    }

    void OnOutputFormatChanged(const MediaAVCodec::Format &format)
    {
        FALSE_RETURN(callback_ != nullptr);
        callback_->OnOutputFormatChanged(format);
    }

    void OnOutputBufferAvailable(uint32_t index, std::shared_ptr<AVBuffer> buffer)
    {
        FALSE_RETURN(callback_ != nullptr);
        callback_->OnOutputBufferAvailable(index, buffer);
    }

    Status Init(MediaAVCodec::AVCodecType type, bool isMimeType, const std::string &name)
    {
        return initRes_;
    }

    Status Configure(const Format &format)
    {
        return configureRes_;
    }

protected:
    Status initRes_{ Status::OK };
    Status configureRes_{ Status::OK };
    std::shared_ptr<MediaAVCodec::MediaCodecCallback> callback_;
};

class EventReceiverMock : public EventReceiver {
public:
    void OnEvent(const Event &event)
    {
        (void)event;
    }
};

void DecoderSurfaceFilterUnitTest::SetUpTestCase(void) {}

void DecoderSurfaceFilterUnitTest::TearDownTestCase(void) {}

void DecoderSurfaceFilterUnitTest::SetUp(void)
{
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    auto eventReceiver = std::make_shared<EventReceiverMock>();
    decoderSurfaceFilter_->eventReceiver_ = eventReceiver;
}

void DecoderSurfaceFilterUnitTest::TearDown(void)
{
    decoderSurfaceFilter_ = nullptr;
}

/**
 * @tc.name: DecoderSurfaceFilter_Destructor_0100
 * @tc.desc: ~DecoderSurfaceFilter
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_Destructor_0100, TestSize.Level1)
{
    // set IS_FILTER_ASYNC false
    system::SetParameter("persist.media_service.async_filter", "0");
    // 1. common dectructor
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    EXPECT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->isThreadExit_ = true;
    // 2. isThreadExit_ is false and destructor
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    EXPECT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->isThreadExit_ = false;
    // 3. isThreadExit_ is false and readThread_ exist but not joinable
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    EXPECT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->isThreadExit_ = false;
    decoderSurfaceFilter_->readThread_ =
        std::make_unique<std::thread>(&DecoderSurfaceFilter::RenderLoop, decoderSurfaceFilter_);
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    EXPECT_NE(decoderSurfaceFilter_, nullptr);

    // set IS_FILTER_ASYNC true
    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    EXPECT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->isThreadExit_ = true;
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    EXPECT_NE(decoderSurfaceFilter_, nullptr);
    decoderSurfaceFilter_->isThreadExit_ = false;
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    EXPECT_NE(decoderSurfaceFilter_, nullptr);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoInitAfterLink_0100, TestSize.Level1)
{
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    auto res = decoderSurfaceFilter_->DoInitAfterLink();
    std::cout << "DoInitAfterLink " << static_cast<int32_t>(res) << std::endl;

    decoderSurfaceFilter_->isDrmProtected_ = true;
    decoderSurfaceFilter_->svpFlag_ = true;
    res = decoderSurfaceFilter_->DoInitAfterLink();
    std::cout << "DoInitAfterLink " << static_cast<int32_t>(res) << std::endl;

    videoDecoderMock->configureRes_ = Status::ERROR_WRONG_STATE;
    res = decoderSurfaceFilter_->DoInitAfterLink();
    std::cout << "DoInitAfterLink " << static_cast<int32_t>(res) << std::endl;

    videoDecoderMock->initRes_ = Status::ERROR_WRONG_STATE;
    res = decoderSurfaceFilter_->DoInitAfterLink();
    std::cout << "DoInitAfterLink " << static_cast<int32_t>(res) << std::endl;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoPrepareFrame_0100, TestSize.Level1)
{
    auto res = decoderSurfaceFilter_->DoPrepareFrame(true);
    EXPECT_NE(res, Status::OK);
    std::cout << "DoPrepareFrame " << static_cast<int32_t>(res) << std::endl;
    res = decoderSurfaceFilter_->DoPrepareFrame(false);
    EXPECT_NE(res, Status::OK);
    std::cout << "DoPrepareFrame2 " << static_cast<int32_t>(res) << std::endl;
    decoderSurfaceFilter_->isPaused_ = true;
    res = decoderSurfaceFilter_->DoPrepareFrame(true);
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPrepareFrame3 " << static_cast<int32_t>(res) << std::endl;
    res = decoderSurfaceFilter_->DoPrepareFrame(false);
    EXPECT_NE(res, Status::OK);
    std::cout << "DoPrepareFrame4 " << static_cast<int32_t>(res) << std::endl;
    decoderSurfaceFilter_->isPaused_ = false;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_HandleInputBuffer_0100, TestSize.Level1)
{
    auto res = decoderSurfaceFilter_->HandleInputBuffer();
    EXPECT_EQ(res, Status::OK);
    decoderSurfaceFilter_->doPrepareFrame_ = true;
    res = decoderSurfaceFilter_->HandleInputBuffer();
    EXPECT_EQ(res, Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoStart_0100, TestSize.Level1)
{
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    auto res = decoderSurfaceFilter_->DoStart();
    EXPECT_NE(res, Status::OK);
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;

    decoderSurfaceFilter_->isPaused_ = true;
    res = decoderSurfaceFilter_->DoStart();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;

    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    res = decoderSurfaceFilter_->DoStart();
    EXPECT_NE(res, Status::OK);
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;

    decoderSurfaceFilter_->isPaused_ = true;
    res = decoderSurfaceFilter_->DoStart();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoStart " << static_cast<int32_t>(res) << std::endl;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoPause_0100, TestSize.Level1)
{
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    auto res = decoderSurfaceFilter_->DoPause();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPause " << static_cast<int32_t>(res) << std::endl;

    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    res = decoderSurfaceFilter_->DoPause();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPause " << static_cast<int32_t>(res) << std::endl;

    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    res = decoderSurfaceFilter_->DoPause();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPause " << static_cast<int32_t>(res) << std::endl;

    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    res = decoderSurfaceFilter_->DoPause();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPause " << static_cast<int32_t>(res) << std::endl;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoResume_0100, TestSize.Level1)
{
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    auto res = decoderSurfaceFilter_->DoResume();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoResume " << static_cast<int32_t>(res) << std::endl;

    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    res = decoderSurfaceFilter_->DoResume();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoResume " << static_cast<int32_t>(res) << std::endl;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoResumeDragging_0100, TestSize.Level1)
{
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    auto res = decoderSurfaceFilter_->DoResumeDragging();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoResumeDragging " << static_cast<int32_t>(res) << std::endl;

    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    res = decoderSurfaceFilter_->DoResumeDragging();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoResumeDragging " << static_cast<int32_t>(res) << std::endl;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoStop_0100, TestSize.Level1)
{
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    decoderSurfaceFilter_->isThreadExit_ = true;
    auto res = decoderSurfaceFilter_->DoStop();
    EXPECT_NE(res, Status::OK);
    std::cout << "DoStop " << static_cast<int32_t>(res) << std::endl;

    decoderSurfaceFilter_->isThreadExit_ = false;
    res = decoderSurfaceFilter_->DoStop();
    EXPECT_NE(res, Status::OK);
    std::cout << "DoStop " << static_cast<int32_t>(res) << std::endl;

    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    decoderSurfaceFilter_->isThreadExit_ = true;
    res = decoderSurfaceFilter_->DoStop();
    EXPECT_NE(res, Status::OK);
    std::cout << "DoStop " << static_cast<int32_t>(res) << std::endl;

    decoderSurfaceFilter_->isThreadExit_ = false;
    res = decoderSurfaceFilter_->DoStop();
    EXPECT_NE(res, Status::OK);
    std::cout << "DoStop " << static_cast<int32_t>(res) << std::endl;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_DoPrepare_0100, TestSize.Level1)
{
    auto res = decoderSurfaceFilter_->DoPrepare();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    decoderSurfaceFilter_->onLinkedResultCallback_ = std::make_shared<FilterLinkCallbackMock>();
    res = decoderSurfaceFilter_->DoPrepare();
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPrepare " << static_cast<int32_t>(res) << std::endl;
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_SetParameter_0100, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    int32_t scaleType = 0;
    parameter->Set<Tag::VIDEO_SCALE_TYPE>(scaleType);
    double rate = -1.0;
    parameter->Set<Tag::VIDEO_FRAME_RATE>(rate);
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    decoderSurfaceFilter_->SetParameter(parameter);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_IS_FILTER_ASYNC_0100, TestSize.Level1)
{
    // set IS_FILTER_ASYNC false
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    EXPECT_EQ(decoderSurfaceFilter_->DoStart(), Status::ERROR_INVALID_STATE);
    EXPECT_EQ(decoderSurfaceFilter_->DoPause(), Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->DoResume(), Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->DoResumeDragging(), Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->DoStop(), Status::ERROR_INVALID_STATE);

    // set IS_FILTER_ASYNC true
    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    EXPECT_EQ(decoderSurfaceFilter_->DoStart(), Status::ERROR_INVALID_STATE);
    EXPECT_EQ(decoderSurfaceFilter_->DoPause(), Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->DoResume(), Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->DoResumeDragging(), Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->DoStop(), Status::ERROR_INVALID_STATE);
    EXPECT_EQ(decoderSurfaceFilter_->DoFlush(), Status::OK);
    EXPECT_EQ(decoderSurfaceFilter_->DoRelease(), Status::OK);
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_GetCodecName_0100, TestSize.Level1)
{
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    std::string mimeType = "test";
    EXPECT_EQ(decoderSurfaceFilter_->GetCodecName(mimeType), "");
}

HWTEST_F(DecoderSurfaceFilterUnitTest, DecoderSurfaceFilter_AcquireNextRenderBuffer_0100, TestSize.Level1)
{
    decoderSurfaceFilter_ =
        std::make_shared<DecoderSurfaceFilter>("testDecoderSurfaceFilter", FilterType::FILTERTYPE_VIDEODEC);
    std::string mimeType = "test";
    bool byIdx = false;
    uint32_t index = 0;
    std::shared_ptr<AVBuffer> outBuffer = std::make_shared<AVBuffer>();
    EXPECT_EQ(decoderSurfaceFilter_->AcquireNextRenderBuffer(byIdx, index, outBuffer), false);
    byIdx = true;
    EXPECT_EQ(decoderSurfaceFilter_->AcquireNextRenderBuffer(byIdx, index, outBuffer), false);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS