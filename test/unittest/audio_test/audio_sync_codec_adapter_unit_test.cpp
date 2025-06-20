#include "gtest/gtest.h"
#include "gmock/gmock.h"
#define private public
#include "avcodec_audio_codec_inner_impl.h"
#undef private
#include "i_avcodec_service.h"
#include "avcodec_log.h"
#include "avcodec_errors.h"
#include "avcodec_trace.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr int32_t DEFAULT_BUFFER_SIZE = 4;
}

namespace OHOS {
namespace Media {

class MockAVBufferQueueConsumer : public AVBufferQueueConsumer {
public:
    MockAVBufferQueueConsumer() = default;
    ~MockAVBufferQueueConsumer() = default;

    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(bool, IsBufferInQueue, (const std::shared_ptr<AVBuffer> &buffer), (override));
    MOCK_METHOD(Status, AcquireBuffer, (std::shared_ptr<AVBuffer> & outBuffer), (override));
    MOCK_METHOD(Status, ReleaseBuffer, (const std::shared_ptr<AVBuffer> &inBuffer), (override));
    MOCK_METHOD(Status, AttachBuffer, (std::shared_ptr<AVBuffer> & inBuffer, bool isFilled), (override));
    MOCK_METHOD(Status, DetachBuffer, (const std::shared_ptr<AVBuffer> &outBuffer), (override));
    MOCK_METHOD(Status, SetBufferAvailableListener, (sptr<IConsumerListener> & listener), (override));
    MOCK_METHOD(Status, SetQueueSizeAndAttachBuffer, (uint32_t size, std::shared_ptr<AVBuffer> &buffer, bool isFilled),
        (override));
};

class MockAVBufferQueueProducer : public IRemoteStub<AVBufferQueueProducer> {
public:
    MockAVBufferQueueProducer() = default;
    ~MockAVBufferQueueProducer() = default;

    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));

    MOCK_METHOD(Status, RequestBuffer,
        (std::shared_ptr<AVBuffer> & outBuffer, const AVBufferConfig &config, int32_t timeoutMs), (override));
    MOCK_METHOD(Status, RequestBufferWaitUs,
        (std::shared_ptr<AVBuffer> & outBuffer, const AVBufferConfig &config, int64_t timeoutUs), (override));

    MOCK_METHOD(Status, PushBuffer, (const std::shared_ptr<AVBuffer> &inBuffer, bool available), (override));
    MOCK_METHOD(Status, ReturnBuffer, (const std::shared_ptr<AVBuffer> &inBuffer, bool available), (override));

    MOCK_METHOD(Status, AttachBuffer, (std::shared_ptr<AVBuffer> & inBuffer, bool isFilled), (override));
    MOCK_METHOD(Status, DetachBuffer, (const std::shared_ptr<AVBuffer> &outBuffer), (override));

    MOCK_METHOD(Status, SetBufferFilledListener, (sptr<IBrokerListener> & listener), (override));
    MOCK_METHOD(Status, RemoveBufferFilledListener, (sptr<IBrokerListener> & listener), (override));
    MOCK_METHOD(Status, SetBufferAvailableListener, (sptr<IProducerListener> & listener), (override));
    MOCK_METHOD(Status, Clear, (), (override));
    MOCK_METHOD(Status, ClearBufferIf, (std::function<bool(const std::shared_ptr<AVBuffer> &)> pred), (override));
};

class MockAVBufferQueue : public AVBufferQueue {
public:
    MOCK_METHOD(std::shared_ptr<AVBufferQueueProducer>, GetLocalProducer, (), (override));
    MOCK_METHOD(std::shared_ptr<AVBufferQueueConsumer>, GetLocalConsumer, (), (override));

    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetProducer, (), (override));
    MOCK_METHOD(sptr<AVBufferQueueConsumer>, GetConsumer, (), (override));

    MOCK_METHOD(sptr<Surface>, GetSurfaceAsProducer, (), (override));
    MOCK_METHOD(sptr<Surface>, GetSurfaceAsConsumer, (), (override));

    MOCK_METHOD(uint32_t, GetQueueSize, (), (override));
    MOCK_METHOD(Status, SetQueueSize, (uint32_t size), (override));
    MOCK_METHOD(Status, SetLargerQueueSize, (uint32_t size), (override));
    MOCK_METHOD(bool, IsBufferInQueue, (const std::shared_ptr<AVBuffer> &buffer), (override));
    MOCK_METHOD(Status, Clear, (), (override));
    MOCK_METHOD(Status, ClearBufferIf, (std::function<bool(const std::shared_ptr<AVBuffer> &)> pred), (override));
    MOCK_METHOD(Status, SetQueueSizeAndAttachBuffer, (uint32_t size, std::shared_ptr<AVBuffer> &buffer, bool isFilled),
        (override));
    MOCK_METHOD(uint32_t, GetFilledBufferSize, (), (override));
};

class SyncCodecAdapterTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        syncCodecAdapter_ = std::make_shared<AVCodecAudioCodecInnerImpl::SyncCodecAdapter>(DEFAULT_BUFFER_SIZE);
        mockBufferQueueConsumer_ = new MockAVBufferQueueConsumer();
        mockBufferQueueProducer_ = new MockAVBufferQueueProducer();
        mockBufferQueue_.reset(new MockAVBufferQueue());
    }

    void TearDown() override
    {}

    std::shared_ptr<AVCodecAudioCodecInnerImpl::SyncCodecAdapter> syncCodecAdapter_;
    sptr<MockAVBufferQueueConsumer> mockBufferQueueConsumer_;
    sptr<MockAVBufferQueueProducer> mockBufferQueueProducer_;
    std::shared_ptr<MockAVBufferQueue> mockBufferQueue_;
};

HWTEST_F(SyncCodecAdapterTest, ATC_ReleaseOutputBuffer_ShouldReturnInvalidState_WhenNotInitialized, TestSize.Level0)
{
    syncCodecAdapter_->init_ = false;

    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    EXPECT_EQ(syncCodecAdapter_->ReleaseOutputBuffer(buffer),
        static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE));
}

HWTEST_F(SyncCodecAdapterTest, ATC_ReleaseOutputBuffer_ShouldReturnInputDataError_WhenBufferNotFound, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;

    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INPUT_DATA_ERROR),
        syncCodecAdapter_->ReleaseOutputBuffer(buffer));
}

HWTEST_F(SyncCodecAdapterTest, ATC_ReleaseOutputBuffer_ShouldReleaseBuffer_WhenBufferFound, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;

    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    syncCodecAdapter_->outputBuffers_.insert(std::make_pair(buffer.get(), buffer));
    syncCodecAdapter_->bufferQueueConsumer_ = mockBufferQueueConsumer_;

    EXPECT_CALL(*mockBufferQueueConsumer_, ReleaseBuffer(buffer)).Times(1);
    EXPECT_EQ(AVCodecServiceErrCode::AVCS_ERR_OK, syncCodecAdapter_->ReleaseOutputBuffer(buffer));
    EXPECT_EQ(syncCodecAdapter_->outputBuffers_.find(buffer.get()), syncCodecAdapter_->outputBuffers_.end());
}

HWTEST_F(SyncCodecAdapterTest, ATC_Prepare_ShouldReturnOk_WhenInitIsFalseAndBuffersAreNotNull, TestSize.Level0)
{
    syncCodecAdapter_->bufferQueueConsumer_ = mockBufferQueueConsumer_;
    syncCodecAdapter_->init_ = false;
    EXPECT_CALL(*mockBufferQueueConsumer_, SetBufferAvailableListener(_)).WillOnce(Return(Status::OK));
    int32_t result = syncCodecAdapter_->Prepare(mockBufferQueueProducer_);

    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
    EXPECT_TRUE(syncCodecAdapter_->init_);
    EXPECT_NE(nullptr, syncCodecAdapter_->bufferQueueProducer_.GetRefPtr());
}

HWTEST_F(SyncCodecAdapterTest, ATC_Prepare_ShouldReturnFailed_WhenInitIsFalseAndSetBufferAvailableListenerFailed,
    TestSize.Level0)
{
    syncCodecAdapter_->bufferQueueConsumer_ = mockBufferQueueConsumer_;
    syncCodecAdapter_->init_ = false;
    EXPECT_CALL(*mockBufferQueueConsumer_, SetBufferAvailableListener(_)).WillOnce(Return(Status::ERROR_UNKNOWN));
    int32_t result = syncCodecAdapter_->Prepare(mockBufferQueueProducer_);

    EXPECT_EQ(static_cast<int32_t>(StatusToAVCodecServiceErrCode(Status::ERROR_UNKNOWN)), result);
    EXPECT_FALSE(syncCodecAdapter_->init_);
    EXPECT_NE(nullptr, syncCodecAdapter_->bufferQueueProducer_.GetRefPtr());
}

HWTEST_F(SyncCodecAdapterTest, ATC_Prepare_ShouldReturnUnknown_WhenInitIsFalseAndBuffersAreNull, TestSize.Level0)
{
    int32_t result = syncCodecAdapter_->Prepare(nullptr);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_UNKNOWN), result);
    EXPECT_FALSE(syncCodecAdapter_->init_);
}

HWTEST_F(SyncCodecAdapterTest, ATC_Prepare_ShouldReturnOk_WhenInitIsTrue, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;
    int32_t result = syncCodecAdapter_->Prepare(nullptr);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
    EXPECT_TRUE(syncCodecAdapter_->init_);
}

HWTEST_F(SyncCodecAdapterTest, ATC_Prepare_ShouldReturnOk_WhenInitIsFalseButPrepareInputIsVaildAndConsumereIsNotNull,
    TestSize.Level0)
{
    const size_t bufferSize = 10;
    syncCodecAdapter_->init_ = false;
    syncCodecAdapter_->bufferQueueConsumer_ = mockBufferQueueConsumer_;

    EXPECT_CALL(*mockBufferQueueProducer_, GetQueueSize()).WillOnce(Return(bufferSize));
    EXPECT_CALL(*mockBufferQueueConsumer_, SetBufferAvailableListener(_)).WillOnce(Return(Status::OK));
    int32_t result = syncCodecAdapter_->Prepare(mockBufferQueueProducer_);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
    EXPECT_TRUE(syncCodecAdapter_->init_);
    EXPECT_EQ(bufferSize, syncCodecAdapter_->inputBuffers_.size());
}

HWTEST_F(SyncCodecAdapterTest, ATC_GetInputBuffer_ShouldReturnBuffer_WhenQueryInputBufferSucceeds, TestSize.Level0)
{
    const size_t bufferBytes = 1024;
    const int64_t timeoutUs = 1000;
    const size_t outputBufferSize = 4;
    uint32_t index;

    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->bufferQueueProducer_ = mockBufferQueueProducer_;
    syncCodecAdapter_->inputBuffers_.resize(outputBufferSize);

    EXPECT_CALL(*mockBufferQueueProducer_, RequestBufferWaitUs(_, _, _))
        .WillOnce([](std::shared_ptr<AVBuffer> &outBuffer, const AVBufferConfig &config, int32_t timeoutMs) {
            outBuffer = std::make_shared<AVBuffer>();
            return Status::OK;
        });
    int32_t result = syncCodecAdapter_->QueryInputBuffer(&index, bufferBytes, timeoutUs);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
    EXPECT_EQ(syncCodecAdapter_->inputIndex_ - 1, index);

    std::shared_ptr<AVBuffer> buffer = syncCodecAdapter_->GetInputBuffer(index);
    EXPECT_NE(nullptr, buffer);
}

HWTEST_F(SyncCodecAdapterTest, ATC_GetProducer_ShouldReturnProducer_WhenInnerBufferQueueIsNotNull, TestSize.Level0)
{
    syncCodecAdapter_->innerBufferQueue_ = mockBufferQueue_;

    EXPECT_CALL(*mockBufferQueue_, GetProducer()).WillOnce(Return(mockBufferQueueProducer_));
    auto result = syncCodecAdapter_->GetProducer();
    EXPECT_NE(nullptr, result.GetRefPtr());
}

HWTEST_F(SyncCodecAdapterTest, ATC_GetProducer_ShouldReturnNull_WhenInnerBufferQueueIsNull, TestSize.Level0)
{
    syncCodecAdapter_->innerBufferQueue_ = nullptr;
    auto result = syncCodecAdapter_->GetProducer();
    EXPECT_EQ(nullptr, result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_GetOutputBuffer_ShouldReturnNull_WhenNotInitialized, TestSize.Level0)
{
    const int64_t timeoutUs = 1000;

    syncCodecAdapter_->init_ = false;
    auto result = syncCodecAdapter_->GetOutputBuffer(timeoutUs);
    EXPECT_EQ(nullptr, result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_GetOutputBuffer_ShouldReturnNull_WhenWaitForFails, TestSize.Level0)
{
    const int64_t timeoutUs = 1000;

    syncCodecAdapter_->init_ = true;
    auto result = syncCodecAdapter_->GetOutputBuffer(timeoutUs);
    EXPECT_EQ(nullptr, result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_GetOutputBuffer_ShouldReturnBuffer_WhenBufferAvailable, TestSize.Level0)
{
    const int64_t timeoutUs = 1000;

    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->bufferQueueConsumer_ = mockBufferQueueConsumer_;
    syncCodecAdapter_->OnBufferAvailable();

    EXPECT_CALL(*mockBufferQueueConsumer_, AcquireBuffer(_)).WillOnce([](std::shared_ptr<AVBuffer> &outBuffer) {
        outBuffer = std::make_shared<AVBuffer>();
        return Status::OK;
    });

    auto result = syncCodecAdapter_->GetOutputBuffer(timeoutUs);
    EXPECT_NE(nullptr, result.get());
}

HWTEST_F(
    SyncCodecAdapterTest, ATC_GetOutputBuffer_ShouldReturnBufferInOrder_WhenBufferAvailableMoreThanOne, TestSize.Level0)
{
    const int64_t timeoutUs = 1000;

    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->bufferQueueConsumer_ = mockBufferQueueConsumer_;
    syncCodecAdapter_->OnBufferAvailable();
    syncCodecAdapter_->OnBufferAvailable();

    std::shared_ptr buffer1 = std::make_shared<AVBuffer>();
    std::shared_ptr buffer2 = std::make_shared<AVBuffer>();
    EXPECT_CALL(*mockBufferQueueConsumer_, AcquireBuffer(_)).WillOnce([buffer1](std::shared_ptr<AVBuffer> &outBuffer) {
        outBuffer = buffer1;
        return Status::OK;
    });
    auto result = syncCodecAdapter_->GetOutputBuffer(timeoutUs);
    EXPECT_EQ(buffer1.get(), result.get());
    EXPECT_CALL(*mockBufferQueueConsumer_, AcquireBuffer(_)).WillOnce([buffer2](std::shared_ptr<AVBuffer> &outBuffer) {
        outBuffer = buffer2;
        return Status::OK;
    });
    result = syncCodecAdapter_->GetOutputBuffer(timeoutUs);
    EXPECT_EQ(buffer2.get(), result.get());
}

HWTEST_F(SyncCodecAdapterTest, ATC_PushInputBuffer_ShouldReturnInvalidState_WhenNotInitialized, TestSize.Level0)
{
    syncCodecAdapter_->init_ = false;
    int32_t result = syncCodecAdapter_->PushInputBuffer(0, true);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE), result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_PushInputBuffer_ShouldReturnInputDataError_WhenIndexOutOfRange, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;
    int32_t result = syncCodecAdapter_->PushInputBuffer(syncCodecAdapter_->inputBuffers_.size(), true);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INPUT_DATA_ERROR), result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_PushInputBuffer_ShouldReturnInputDataError_WhenBufferIsNull, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->inputBuffers_.resize(1);
    syncCodecAdapter_->inputBuffers_[0] = nullptr;
    int32_t result = syncCodecAdapter_->PushInputBuffer(0, true);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INPUT_DATA_ERROR), result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_PushInputBuffer_ShouldPushBufferSuccessfully_WhenAllConditionsMet, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->bufferQueueProducer_ = mockBufferQueueProducer_;
    syncCodecAdapter_->inputBuffers_.resize(1);
    syncCodecAdapter_->inputBuffers_[0] = std::make_shared<AVBuffer>();
    EXPECT_CALL(*mockBufferQueueProducer_, PushBuffer(_, true)).WillOnce(Return(Status::OK));

    int32_t result = syncCodecAdapter_->PushInputBuffer(0, true);
    EXPECT_EQ(StatusToAVCodecServiceErrCode(Status::OK), result);
    EXPECT_EQ(nullptr, syncCodecAdapter_->inputBuffers_[0]);
}

HWTEST_F(SyncCodecAdapterTest, ATC_PushInputBuffer_ShouldReturnError_WhenPushBufferFails, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->bufferQueueProducer_ = mockBufferQueueProducer_;
    syncCodecAdapter_->inputBuffers_.resize(1);
    syncCodecAdapter_->inputBuffers_[0] = std::make_shared<AVBuffer>();
    EXPECT_CALL(*mockBufferQueueProducer_, PushBuffer(_, _)).WillOnce(Return(Status::ERROR_UNKNOWN));

    int32_t result = syncCodecAdapter_->PushInputBuffer(0, true);
    EXPECT_EQ(StatusToAVCodecServiceErrCode(Status::ERROR_UNKNOWN), result);
    EXPECT_EQ(nullptr, syncCodecAdapter_->inputBuffers_[0]);
}

HWTEST_F(SyncCodecAdapterTest, ATC_QueryInputBuffer_ShouldReturnInvalidState_WhenNotInitialized, TestSize.Level0)
{
    syncCodecAdapter_->init_ = false;
    uint32_t index = 0;
    const size_t bufferSize = 1024;
    const int64_t timeoutUs = 1000;
    int32_t result = syncCodecAdapter_->QueryInputBuffer(&index, bufferSize, timeoutUs);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_STATE), result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_QueryInputBuffer_ShouldReturnInputDataError_WhenIndexIsNull, TestSize.Level0)
{
    syncCodecAdapter_->init_ = true;
    uint32_t *index = nullptr;
    const size_t bufferSize = 1024;
    const int64_t timeoutUs = 1000;
    int32_t result = syncCodecAdapter_->QueryInputBuffer(index, bufferSize, timeoutUs);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INPUT_DATA_ERROR), result);
}

HWTEST_F(SyncCodecAdapterTest, ATC_QueryInputBuffer_ShouldReturnOK_WhenRequestBufferSucceeds, TestSize.Level0)
{
    uint32_t index = syncCodecAdapter_->inputIndex_ + 100;
    const size_t bufferSize = 1024;
    const int64_t timeoutUs = 1000;

    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->inputBuffers_.resize(2);
    syncCodecAdapter_->inputBuffers_[0] = nullptr;
    syncCodecAdapter_->bufferQueueProducer_ = mockBufferQueueProducer_;

    EXPECT_CALL(*mockBufferQueueProducer_, RequestBufferWaitUs(_, _, _)).WillOnce(Return(Status::OK));
    int32_t result = syncCodecAdapter_->QueryInputBuffer(&index, bufferSize, timeoutUs);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
    EXPECT_EQ(syncCodecAdapter_->inputIndex_ - 1, index);
}

HWTEST_F(SyncCodecAdapterTest, ATC_QueryInputBuffer_ShouldReturnOK_WhenRequestBufferFails, TestSize.Level0)
{
    uint32_t index = syncCodecAdapter_->inputIndex_ + 100;
    const size_t bufferSize = 1024;
    const int64_t timeoutUs = 1000;

    syncCodecAdapter_->init_ = true;
    syncCodecAdapter_->inputBuffers_.resize(1);
    syncCodecAdapter_->inputBuffers_[0] = nullptr;
    syncCodecAdapter_->bufferQueueProducer_ = mockBufferQueueProducer_;

    EXPECT_CALL(*mockBufferQueueProducer_, RequestBufferWaitUs(_, _, _)).WillOnce(Return(Status::ERROR_UNKNOWN));
    int32_t result = syncCodecAdapter_->QueryInputBuffer(&index, bufferSize, timeoutUs);
    EXPECT_EQ(static_cast<int32_t>(StatusToAVCodecServiceErrCode(Status::ERROR_UNKNOWN)), result);
    EXPECT_EQ(syncCodecAdapter_->inputIndex_ + 100, index);
}

HWTEST_F(SyncCodecAdapterTest, ATC_WaitFor_ShouldReturnTrue_WhenOutputAvailable, TestSize.Level0)
{
    syncCodecAdapter_->outputAvaliableNum_ = 1;
    std::mutex lock;
    std::unique_lock<std::mutex> uniqueLock(lock);
    EXPECT_TRUE(syncCodecAdapter_->WaitFor(uniqueLock, 0));
}

HWTEST_F(SyncCodecAdapterTest, ATC_WaitFor_ShouldReturnTrue_WhenTimeoutIsNegative, TestSize.Level0)
{
    syncCodecAdapter_->outputAvaliableNum_ = 1;
    std::mutex lock;
    std::unique_lock<std::mutex> uniqueLock(lock);
    EXPECT_TRUE(syncCodecAdapter_->WaitFor(uniqueLock, -1));
}

HWTEST_F(SyncCodecAdapterTest, ATC_WaitFor_ShouldReturnFalse_WhenTimeoutIsZero, TestSize.Level0)
{
    syncCodecAdapter_->outputAvaliableNum_ = 0;
    std::mutex lock;
    std::unique_lock<std::mutex> uniqueLock(lock);
    EXPECT_FALSE(syncCodecAdapter_->WaitFor(uniqueLock, 0));
}

HWTEST_F(SyncCodecAdapterTest, ATC_WaitFor_ShouldReturnFalse_WhenTimeoutIsPositiveAndNoData, TestSize.Level0)
{
    syncCodecAdapter_->outputAvaliableNum_ = 0;
    std::mutex lock;
    std::unique_lock<std::mutex> uniqueLock(lock);
    EXPECT_FALSE(syncCodecAdapter_->WaitFor(uniqueLock, 1000));
}

HWTEST_F(
    SyncCodecAdapterTest, ATC_syncCodecAdapterShouldInitializeCorrectly_WhenInputBufferSizeIsValid, TestSize.Level0)
{
    const size_t inputBufferSize = 1024;
    AVCodecAudioCodecInnerImpl::SyncCodecAdapter adapter(inputBufferSize);

    EXPECT_FALSE(adapter.init_);
    EXPECT_EQ(0, adapter.inputIndex_);
    EXPECT_EQ(0, adapter.inputBuffers_.size());
    EXPECT_EQ(0, adapter.outputAvaliableNum_);
    EXPECT_NE(nullptr, adapter.innerBufferQueue_);
    EXPECT_NE(nullptr, adapter.bufferQueueConsumer_.GetRefPtr());
}

HWTEST_F(SyncCodecAdapterTest, ATC_OnBufferAvailable_ShouldIncreaseOutputAvaliableNum_WhenCalled, TestSize.Level0)
{
    int initialOutputAvaliableNum = syncCodecAdapter_->outputAvaliableNum_;
    syncCodecAdapter_->OnBufferAvailable();
    EXPECT_EQ(initialOutputAvaliableNum + 1, syncCodecAdapter_->outputAvaliableNum_);
}
}  // namespace Media
}  // namespace OHOS