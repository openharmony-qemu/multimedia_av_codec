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
        return Status::ERROR_UNKNOWN;
    }

    Status Configure(const Format &format)
    {
        return configureRes_;
    }
    int32_t SetOutputSurface(sptr<Surface> videoSurface)
    {
        return 0;
    }
    Status Stop()
    {
        return configureRes_;
    }
    int32_t SetParameter(const Format &format)
    {
        return MediaAVCodec::AVCS_ERR_INVALID_STATE;
    }
    void OnDumpInfo(int32_t fd)
    {
        return;
    }
    void SetCallingInfo(int32_t appUid, int32_t appPid, const std::string& bundleName, uint64_t instanceId)
    {
        return;
    }
    int32_t SetDecryptConfig(const sptr<DrmStandard::IMediaKeySessionService> &keySession,
        const bool svpFlag)
    {
        return 0;
    }
    int32_t ReleaseOutputBuffer(uint32_t index, bool render)
    {
        return 0;
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
    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    auto res = decoderSurfaceFilter_->DoPrepareFrame(true);
    EXPECT_NE(res, Status::OK);
    std::cout << "DoPrepareFrame " << static_cast<int32_t>(res) << std::endl;
    decoderSurfaceFilter_->isPaused_ = true;
    res = decoderSurfaceFilter_->DoPrepareFrame(false);
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPrepareFrame2 " << static_cast<int32_t>(res) << std::endl;
    decoderSurfaceFilter_->isPaused_ = true;
    res = decoderSurfaceFilter_->DoPrepareFrame(true);
    EXPECT_EQ(res, Status::OK);
    std::cout << "DoPrepareFrame3 " << static_cast<int32_t>(res) << std::endl;
    decoderSurfaceFilter_->isPaused_ = true;
    res = decoderSurfaceFilter_->DoPrepareFrame(false);
    EXPECT_EQ(res, Status::OK);
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

/**
 * @tc.name: DoPrepare_001
 * @tc.desc: DoPrepare
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DoPrepare_001, TestSize.Level1)
{
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    decoderSurfaceFilter_->meta_ = std::make_shared<Meta>();
    decoderSurfaceFilter_->videoSurface_ = nullptr;
    decoderSurfaceFilter_->onLinkedResultCallback_ = nullptr;
    Status ret = decoderSurfaceFilter_->DoPrepare();
    decoderSurfaceFilter_->onLinkedResultCallback_ = std::make_shared<FilterLinkCallbackMock>();
    ret = decoderSurfaceFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoStart_001
 * @tc.desc: DoStart
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DoStart_001, TestSize.Level1)
{
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    decoderSurfaceFilter_->meta_ = std::make_shared<Meta>();
    decoderSurfaceFilter_->videoSurface_ = nullptr;
    decoderSurfaceFilter_->onLinkedResultCallback_ = nullptr;
    Status ret = decoderSurfaceFilter_->DoPrepare();
    decoderSurfaceFilter_->onLinkedResultCallback_ = std::make_shared<FilterLinkCallbackMock>();
    ret = decoderSurfaceFilter_->DoPrepare();
    EXPECT_EQ(ret, Status::OK);
    system::SetParameter("persist.media_service.async_filter", "0");
    ret = decoderSurfaceFilter_->DoPrepare();
    system::SetParameter("persist.media_service.async_filter", "1");
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoPause_001
 * @tc.desc: DoPause
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DoPause_001, TestSize.Level1)
{
    decoderSurfaceFilter_->videoSink_ = std::make_shared<VideoSink>();
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    system::SetParameter("persist.media_service.async_filter", "0");
    Status ret = decoderSurfaceFilter_->DoPause();
    system::SetParameter("persist.media_service.async_filter", "1");
    ret = decoderSurfaceFilter_->DoPause();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoResume_001
 * @tc.desc: DoResume
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DoResume_001, TestSize.Level1)
{
    decoderSurfaceFilter_->videoSink_ = std::make_shared<VideoSink>();
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    system::SetParameter("persist.media_service.async_filter", "0");
    Status ret = decoderSurfaceFilter_->DoResume();
    system::SetParameter("persist.media_service.async_filter", "1");
    ret = decoderSurfaceFilter_->DoResume();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoResumeDragging_001
 * @tc.desc: DoResumeDragging
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DoResumeDragging_001, TestSize.Level1)
{
    decoderSurfaceFilter_->videoSink_ = std::make_shared<VideoSink>();
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    system::SetParameter("persist.media_service.async_filter", "0");
    Status ret = decoderSurfaceFilter_->DoResumeDragging();
    system::SetParameter("persist.media_service.async_filter", "1");
    ret = decoderSurfaceFilter_->DoResumeDragging();
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoStop_001
 * @tc.desc: DoStop
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DoStop_001, TestSize.Level1)
{
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    decoderSurfaceFilter_->meta_ = std::make_shared<Meta>();
    decoderSurfaceFilter_->videoSurface_ = nullptr;
    decoderSurfaceFilter_->onLinkedResultCallback_ = nullptr;
    decoderSurfaceFilter_->isThreadExit_ = false;
    system::SetParameter("persist.media_service.async_filter", "1");
    Status ret = decoderSurfaceFilter_->DoStop();
    decoderSurfaceFilter_->isThreadExit_ = true;
    system::SetParameter("persist.media_service.async_filter", "0");
    ret = decoderSurfaceFilter_->DoStop();
    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_->isThreadExit_ = true;
    ret = decoderSurfaceFilter_->DoStop();
    EXPECT_NE(ret, Status::OK);
}

/**
 * @tc.name: SetParameter_001
 * @tc.desc: SetParameter
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, SetParameter_001, TestSize.Level1)
{
    std::shared_ptr<Meta> parameter = std::make_shared<Meta>();
    parameter->SetData(Tag::VIDEO_SCALE_TYPE, 1);
    decoderSurfaceFilter_->SetParameter(parameter);
    parameter->SetData(Tag::VIDEO_FRAME_RATE, -1);
    decoderSurfaceFilter_->SetParameter(parameter);
    parameter->SetData(Tag::VIDEO_FRAME_RATE, 1);
    decoderSurfaceFilter_->SetParameter(parameter);
    parameter->SetData(Tag::VIDEO_FRAME_RATE, 0);
    decoderSurfaceFilter_->isThreadExit_ = true;
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_->SetParameter(parameter);
    decoderSurfaceFilter_->isThreadExit_ = false;
    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_->SetParameter(parameter);
    decoderSurfaceFilter_->isThreadExit_ = true;
    system::SetParameter("persist.media_service.async_filter", "1");
    decoderSurfaceFilter_->SetParameter(parameter);
    decoderSurfaceFilter_->isDrmProtected_ = true;
    decoderSurfaceFilter_->SetParameter(parameter);
    decoderSurfaceFilter_->isDrmProtected_ = false;
    decoderSurfaceFilter_->SetParameter(parameter);
    EXPECT_EQ(decoderSurfaceFilter_->stopTime_, 0);
}

/**
 * @tc.name: GetCodecName_001
 * @tc.desc: GetCodecName
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, GetCodecName_001, TestSize.Level1)
{
    decoderSurfaceFilter_->GetCodecName("test");
    EXPECT_EQ(decoderSurfaceFilter_->renderFrameCnt_, 0);
}

/**
 * @tc.name: AcquireNextRenderBuffer_001
 * @tc.desc: AcquireNextRenderBuffer
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, AcquireNextRenderBuffer_001, TestSize.Level1)
{
    uint32_t index = 0;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> outBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    bool ret = decoderSurfaceFilter_->AcquireNextRenderBuffer(true, index, outBuffer);
    ret = decoderSurfaceFilter_->AcquireNextRenderBuffer(false, index, outBuffer);
    EXPECT_NE(ret, true);
}

/**
 * @tc.name: CalculateNextRender_002
 * @tc.desc: CalculateNextRender
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, CalculateNextRender_002, TestSize.Level1)
{
    uint32_t index = 0;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> outBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    decoderSurfaceFilter_->isSeek_ = true;
    outBuffer->pts_ = 1;
    decoderSurfaceFilter_->CalculateNextRender(index, outBuffer);
    decoderSurfaceFilter_->isSeek_ = true;
    outBuffer->pts_ = -1;
    decoderSurfaceFilter_->CalculateNextRender(index, outBuffer);
    decoderSurfaceFilter_->isSeek_ = false;
    decoderSurfaceFilter_->CalculateNextRender(index, outBuffer);
    EXPECT_EQ(decoderSurfaceFilter_->seekTimeUs_, 0);
}

/**
 * @tc.name: RenderNextOutput_002
 * @tc.desc: RenderNextOutput
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, RenderNextOutput_002, TestSize.Level1)
{
    uint32_t index = 0;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> outBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    decoderSurfaceFilter_->isInSeekContinous_ = false;
    decoderSurfaceFilter_->RenderNextOutput(index, outBuffer);
    decoderSurfaceFilter_->isInSeekContinous_ = true;
    decoderSurfaceFilter_->RenderNextOutput(index, outBuffer);
    EXPECT_EQ(decoderSurfaceFilter_->seekTimeUs_, 0);
}

/**
 * @tc.name: DrainOutputBuffer_001
 * @tc.desc: DrainOutputBuffer
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DrainOutputBuffer_001, TestSize.Level1)
{
    uint32_t index = 0;
    uint8_t data[100];
    std::shared_ptr<AVBuffer> outBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    decoderSurfaceFilter_->isInSeekContinous_ = true;
    cout << "1" <<endl;
    decoderSurfaceFilter_->DrainOutputBuffer(index, outBuffer);
    decoderSurfaceFilter_->isInSeekContinous_ = false;
    system::SetParameter("persist.media_service.async_filter", "0");
    decoderSurfaceFilter_->outputBuffers_.push_back(make_pair(1, outBuffer));
    cout << "2" <<endl;
    decoderSurfaceFilter_->DrainOutputBuffer(index, outBuffer);
    EXPECT_EQ(decoderSurfaceFilter_->seekTimeUs_, 0);
}

/**
 * @tc.name: RenderLoop_001
 * @tc.desc: RenderLoop
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, RenderLoop_001, TestSize.Level1)
{
    uint8_t data[100];
    std::shared_ptr<AVBuffer> outBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    decoderSurfaceFilter_->isThreadExit_ = true;
    decoderSurfaceFilter_->RenderLoop();
    EXPECT_EQ(decoderSurfaceFilter_->stopTime_, 0);
}

/**
 * @tc.name: SetVideoSurface_001
 * @tc.desc: SetVideoSurface
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, SetVideoSurface_001, TestSize.Level1)
{
    sptr<Surface> videoSurface = nullptr;
    Status ret = decoderSurfaceFilter_->SetVideoSurface(videoSurface);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: SetDecryptConfig_001
 * @tc.desc: SetDecryptConfig
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, SetDecryptConfig_001, TestSize.Level1)
{
    sptr<DrmStandard::IMediaKeySessionService> keySessionProxy = nullptr;
    Status ret = decoderSurfaceFilter_->SetDecryptConfig(keySessionProxy, true);
    EXPECT_EQ(ret, Status::ERROR_INVALID_PARAMETER);
}

/**
 * @tc.name: OnOutputFormatChanged_001
 * @tc.desc: OnOutputFormatChanged
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, OnOutputFormatChanged_001, TestSize.Level1)
{
    MediaAVCodec::Format format;
    format.PutIntValue(Tag::VIDEO_WIDTH, 0);
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    format.PutIntValue(Tag::VIDEO_WIDTH, 1);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 0);
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 1);
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    EXPECT_EQ(decoderSurfaceFilter_->stopTime_, 0);
}

/**
 * @tc.name: OnOutputFormatChanged_002
 * @tc.desc: OnOutputFormatChanged
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, OnOutputFormatChanged_002, TestSize.Level1)
{
    MediaAVCodec::Format format;
    decoderSurfaceFilter_->surfaceWidth_ = 1;
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    decoderSurfaceFilter_->surfaceWidth_ = 0;
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    decoderSurfaceFilter_->surfaceWidth_ = 1;
    decoderSurfaceFilter_->surfaceHeight_ = 1;
    format.PutIntValue(Tag::VIDEO_WIDTH, 0);
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    format.PutIntValue(Tag::VIDEO_WIDTH, 1);
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 0);
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    format.PutIntValue(Tag::VIDEO_HEIGHT, 1);
    decoderSurfaceFilter_->OnOutputFormatChanged(format);
    EXPECT_EQ(decoderSurfaceFilter_->stopTime_, 0);
}

/**
 * @tc.name: ParseDecodeRateLimit_001
 * @tc.desc: ParseDecodeRateLimit
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, ParseDecodeRateLimit_001, TestSize.Level1)
{
    decoderSurfaceFilter_->meta_ = std::make_shared<Meta>();
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_HEIGHT, 1);
    decoderSurfaceFilter_->meta_->SetData(Tag::VIDEO_WIDTH, 1);
    decoderSurfaceFilter_->ParseDecodeRateLimit();
    decoderSurfaceFilter_->rateUpperLimit_ = 1;
    decoderSurfaceFilter_->ParseDecodeRateLimit();
    EXPECT_EQ(decoderSurfaceFilter_->stopTime_, 0);
}

/**
 * @tc.name: OnDumpInfo_001
 * @tc.desc: OnDumpInfo
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, OnDumpInfo_001, TestSize.Level1)
{
    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->OnDumpInfo(32);
    EXPECT_EQ(decoderSurfaceFilter_->stopTime_, 0);
}

/**
 * @tc.name: ReleaseOutputBuffer_001
 * @tc.desc: ReleaseOutputBuffer
 * @tc.type: FUNC
 */

HWTEST_F(DecoderSurfaceFilterUnitTest, ReleaseOutputBuffer_001, TestSize.Level1)
{
    decoderSurfaceFilter_->videoDecoder_ = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoSink_ = std::make_shared<VideoSink>();
    uint8_t data[100];
    std::shared_ptr<AVBuffer> outBuffer = AVBuffer::CreateAVBuffer(data, sizeof(data), sizeof(data));
    decoderSurfaceFilter_->playRangeEndTime_ = 1;
    outBuffer->pts_ = 2000;
    decoderSurfaceFilter_->isRenderStarted_ = true;
    Status ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, outBuffer, 0L);
    decoderSurfaceFilter_->isRenderStarted_ = false;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, false, outBuffer, 0L);
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, outBuffer, 0L);
    outBuffer->flag_ = true;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, outBuffer, 0L);
    outBuffer->flag_ = false;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, outBuffer, 0L);
    decoderSurfaceFilter_->isInSeekContinous_ = true;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, outBuffer, 0L);
    decoderSurfaceFilter_->isInSeekContinous_ = false;
    ret = decoderSurfaceFilter_->ReleaseOutputBuffer(0, true, outBuffer, 0L);
    EXPECT_EQ(ret, Status::OK);
}

/**
 * @tc.name: DoInitAfterLink_001
 * @tc.desc: DoInitAfterLink
 * @tc.type: FUNC
 */
HWTEST_F(DecoderSurfaceFilterUnitTest, DoInitAfterLink_001, TestSize.Level1)
{
    auto videoDecoderMock = std::make_shared<VideoDecoderAdapterMock>();
    decoderSurfaceFilter_->videoDecoder_ = videoDecoderMock;
    decoderSurfaceFilter_->meta_ = std::make_shared<Meta>();
    decoderSurfaceFilter_->configureParameter_ = std::make_shared<Meta>();
    decoderSurfaceFilter_->videoSink_ = std::make_shared<VideoSink>();
    EXPECT_NE(decoderSurfaceFilter_->eventReceiver_, nullptr);
    decoderSurfaceFilter_->isDrmProtected_ = true;
    decoderSurfaceFilter_->svpFlag_ = false;
    decoderSurfaceFilter_->codecMimeType_ = "test";
    Status ret = decoderSurfaceFilter_->DoInitAfterLink();
    EXPECT_NE(ret, Status::OK);
    decoderSurfaceFilter_->isDrmProtected_ = false;
    decoderSurfaceFilter_->svpFlag_ = true;
    ret = decoderSurfaceFilter_->DoInitAfterLink();
    EXPECT_NE(ret, Status::OK);
    decoderSurfaceFilter_->isDrmProtected_ = true;
    decoderSurfaceFilter_->svpFlag_ = true;
    ret = decoderSurfaceFilter_->DoInitAfterLink();
    EXPECT_NE(ret, Status::OK);
    ret = decoderSurfaceFilter_->DoInitAfterLink();
    EXPECT_NE(ret, Status::OK);
    decoderSurfaceFilter_->isDrmProtected_ = false;
    ret = decoderSurfaceFilter_->DoInitAfterLink();
    decoderSurfaceFilter_->isDrmProtected_ = true;
    decoderSurfaceFilter_->svpFlag_ = false;
    ret = decoderSurfaceFilter_->DoInitAfterLink();
    EXPECT_NE(ret, Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS