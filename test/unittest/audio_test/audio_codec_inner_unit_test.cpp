#include "gtest/gtest.h"
#include "gmock/gmock.h"
#define private public
#include "avcodec_audio_codec_inner_impl.h"
#undef private
#include "audio_codec_server.h"
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

class MockCodecService : public AudioCodecServer {
public:
    MOCK_METHOD(int32_t, Init,
        (AVCodecType type, bool isMimeType, const std::string &name, Meta &callerInfo, API_VERSION apiVersion));
    MOCK_METHOD(int32_t, Configure, (const Format &format));
    MOCK_METHOD(int32_t, SetCustomBuffer, (std::shared_ptr<AVBuffer> buffer));
    MOCK_METHOD(int32_t, Start, ());
    MOCK_METHOD(int32_t, Stop, ());
    MOCK_METHOD(int32_t, Flush, ());
    MOCK_METHOD(int32_t, Reset, ());
    MOCK_METHOD(int32_t, Release, ());
    MOCK_METHOD(int32_t, NotifyEos, ());
    MOCK_METHOD(sptr<Surface>, CreateInputSurface, ());
    MOCK_METHOD(int32_t, SetOutputSurface, (sptr<Surface> surface));
    MOCK_METHOD(
        int32_t, QueueInputBuffer, (uint32_t index, AVCodecBufferInfo info, AVCodecBufferFlag flag));
    MOCK_METHOD(int32_t, QueueInputBuffer, (uint32_t index));
    MOCK_METHOD(int32_t, QueueInputParameter, (uint32_t index));
    MOCK_METHOD(int32_t, GetOutputFormat, (Format & format));
    MOCK_METHOD(int32_t, ReleaseOutputBuffer, (uint32_t index, bool render));
    MOCK_METHOD(int32_t, RenderOutputBufferAtTime, (uint32_t index, int64_t renderTimestampNs));
    MOCK_METHOD(int32_t, SetParameter, (const Format &format));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<AVCodecCallback> &callback));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaCodecCallback> &callback));
    MOCK_METHOD(int32_t, SetCallback, (const std::shared_ptr<MediaCodecParameterCallback> &callback));
    MOCK_METHOD(
        int32_t, SetCallback, (const std::shared_ptr<MediaCodecParameterWithAttrCallback> &callback));
    MOCK_METHOD(int32_t, ChangePlugin, (const std::string &mime, bool isEncoder, const std::shared_ptr<Meta> &meta));
    MOCK_METHOD(int32_t, SetCodecCallback, (const std::shared_ptr<MediaCodecCallback> &codecCallback));
    MOCK_METHOD(void, SetDumpInfo, (bool isDump, uint64_t instanceId));
    MOCK_METHOD(int32_t, GetInputFormat, (Format & format));
    MOCK_METHOD(int32_t, SetDecryptConfig,
        (const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag));
    MOCK_METHOD(int32_t, CreateCodecByName, (const std::string &name));
    MOCK_METHOD(int32_t, Configure, (const std::shared_ptr<Meta> &meta));
    MOCK_METHOD(int32_t, SetParameter, (const std::shared_ptr<Meta> &parameter));
    MOCK_METHOD(int32_t, GetOutputFormat, (std::shared_ptr<Meta> & parameter));
    MOCK_METHOD(int32_t, SetOutputBufferQueue, (const sptr<AVBufferQueueProducer> &bufferQueueProducer));
    MOCK_METHOD(int32_t, Prepare, ());
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetInputBufferQueue, ());
    MOCK_METHOD(void, ProcessInputBuffer, ());
    MOCK_METHOD(bool, CheckRunning, ());
    MOCK_METHOD(int32_t, SetAudioDecryptionConfig,
        (const sptr<DrmStandard::IMediaKeySessionService> &keySession, const bool svpFlag));
    MOCK_METHOD(sptr<AVBufferQueueConsumer>, GetInputBufferQueueConsumer, ());
    MOCK_METHOD(sptr<AVBufferQueueProducer>, GetOutputBufferQueueProducer, ());
    MOCK_METHOD(
        void, ProcessInputBufferInner, (bool isTriggeredByOutPort, bool isFlushed, uint32_t &bufferStatus));

    MOCK_METHOD(int32_t, GetChannelId, (int32_t & channelId));
    MOCK_METHOD(int32_t, SetLowPowerPlayerMode, (bool isLpp));
};

class AudioCodecInnerTest : public ::testing::Test {
protected:
    void SetUp() override
    {
        mockCodecService_ = std::make_shared<MockCodecService>();
        avCodecAudioCodecInnerImpl_ = std::make_unique<AVCodecAudioCodecInnerImpl>();
        avCodecAudioCodecInnerImpl_->codecService_ = mockCodecService_;
        innerBufferQueue_ =
            AVBufferQueue::Create(DEFAULT_BUFFER_SIZE, MemoryType::SHARED_MEMORY, "AudioCodecInnerTest");
    }

    void TearDown() override
    {}

    std::shared_ptr<MockCodecService> mockCodecService_;
    std::unique_ptr<AVCodecAudioCodecInnerImpl> avCodecAudioCodecInnerImpl_;
    std::shared_ptr<AVBufferQueue> innerBufferQueue_;
};

HWTEST_F(AudioCodecInnerTest, ATC_Configure_ShouldReturnInvalidOperation_WhenCodecServiceIsNull, TestSize.Level0)
{
    avCodecAudioCodecInnerImpl_->codecService_.reset();
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    int32_t result = avCodecAudioCodecInnerImpl_->Configure(meta);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION), result);
}

HWTEST_F(AudioCodecInnerTest, ATC_Configure_ShouldCreateSyncCodecAdapter_WhenSyncModeIsEnabled, TestSize.Level0)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::AV_CODEC_ENABLE_SYNC_MODE>(true);
    EXPECT_CALL(*mockCodecService_, SetOutputBufferQueue(_))
        .WillOnce(Return(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK)));

    int32_t result = avCodecAudioCodecInnerImpl_->Configure(meta);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
    EXPECT_NE(nullptr, avCodecAudioCodecInnerImpl_->syncCodecAdapter_);
}

HWTEST_F(AudioCodecInnerTest, ATC_Configure_ShouldResetSyncCodecAdapter_WhenSetProducerFails, TestSize.Level0)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    meta->Set<Tag::AV_CODEC_ENABLE_SYNC_MODE>(true);
    EXPECT_CALL(*mockCodecService_, SetOutputBufferQueue(_))
        .WillOnce(Return(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_UNKNOWN)));
    int32_t result = avCodecAudioCodecInnerImpl_->Configure(meta);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_UNKNOWN), result);
    EXPECT_EQ(nullptr, avCodecAudioCodecInnerImpl_->syncCodecAdapter_);
}

HWTEST_F(AudioCodecInnerTest, ATC_Configure_ShouldCallCodecServiceConfigure_WhenSyncModeIsDisabled, TestSize.Level0)
{
    std::shared_ptr<Meta> meta = std::make_shared<Meta>();
    EXPECT_CALL(*mockCodecService_, Configure(meta))
        .WillOnce(Return(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK)));

    int32_t result = avCodecAudioCodecInnerImpl_->Configure(meta);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
    EXPECT_EQ(nullptr, avCodecAudioCodecInnerImpl_->syncCodecAdapter_);
}

HWTEST_F(
    AudioCodecInnerTest, ATC_SetOutputBufferQueue_ShouldReturnInvalidOperation_WhenCodecServiceIsNull, TestSize.Level0)
{
    avCodecAudioCodecInnerImpl_->codecService_.reset();
    int32_t result = avCodecAudioCodecInnerImpl_->SetOutputBufferQueue(nullptr);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_OPERATION), result);
}

HWTEST_F(AudioCodecInnerTest, ATC_SetOutputBufferQueue_ShouldReturnOk_WhenSyncCodecAdapterIsNotNull, TestSize.Level0)
{
    avCodecAudioCodecInnerImpl_->syncCodecAdapter_ = std::make_shared<AVCodecAudioCodecInnerImpl::SyncCodecAdapter>(4);
    int32_t result = avCodecAudioCodecInnerImpl_->SetOutputBufferQueue(innerBufferQueue_->GetProducer());
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
}

HWTEST_F(AudioCodecInnerTest,
    ATC_SetOutputBufferQueue_ShouldCallCodecServiceSetOutputBufferQueue_WhenSyncCodecAdapterIsNull, TestSize.Level0)
{
    EXPECT_CALL(*mockCodecService_, SetOutputBufferQueue(innerBufferQueue_->GetProducer()))
        .WillOnce(Return(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK)));
    int32_t result = avCodecAudioCodecInnerImpl_->SetOutputBufferQueue(innerBufferQueue_->GetProducer());
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_OK), result);
}

HWTEST_F(AudioCodecInnerTest, ATC_QueryInputBuffer_ShouldReturnInvalid_WhenSyncModeIsDisabled, TestSize.Level0)
{
    uint32_t index = 0;
    const size_t bufferSize = 0;
    const int64_t timeoutUs = 0;
    int32_t result = avCodecAudioCodecInnerImpl_->QueryInputBuffer(&index, bufferSize, timeoutUs);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL), result);
}

HWTEST_F(AudioCodecInnerTest, ATC_GetInputBuffer_ShouldReturnInvalid_WhenSyncModeIsDisabled, TestSize.Level0)
{
    const uint32_t index = 0;
    auto result = avCodecAudioCodecInnerImpl_->GetInputBuffer(index);
    EXPECT_EQ(nullptr, result);
}

HWTEST_F(AudioCodecInnerTest, ATC_PushInputBuffer_ShouldReturnInvalid_WhenSyncModeIsDisabled, TestSize.Level0)
{
    const uint32_t index = 0;
    int32_t result = avCodecAudioCodecInnerImpl_->PushInputBuffer(index, true);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL), result);
}

HWTEST_F(AudioCodecInnerTest, ATC_GetOutputBuffer_ShouldReturnInvalid_WhenSyncModeIsDisabled, TestSize.Level0)
{
    const int64_t timeoutUs = 0;
    auto result = avCodecAudioCodecInnerImpl_->GetOutputBuffer(timeoutUs);
    EXPECT_EQ(nullptr, result);
}

HWTEST_F(AudioCodecInnerTest, ATC_ReleaseOutputBuffer_ShouldReturnInvalid_WhenSyncModeIsDisabled, TestSize.Level0)
{
    std::shared_ptr<AVBuffer> buffer;
    int32_t result = avCodecAudioCodecInnerImpl_->ReleaseOutputBuffer(buffer);
    EXPECT_EQ(static_cast<int32_t>(AVCodecServiceErrCode::AVCS_ERR_INVALID_VAL), result);
}
}  // namespace Media
}  // namespace OHOS