#include "gtest/gtest.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "surface.h"
#include "hevc_decoder.h"

using namespace testing;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::MediaAVCodec;
using namespace OHOS::MediaAVCodec::Codec;

class TestConsumerListener : public IBufferConsumerListener {
public:
    TestConsumerListener(sptr<Surface> cs, std::string_view name);
    ~TestConsumerListener();
    void OnBufferAvailable() override;

private:
    int64_t  timestamp_ = 0;
    OHOS::Rect damage_ = {};
    sptr<Surface> cs_ = nullptr;
    std::unique_ptr<std::ofstream> outFile_;
};

TestConsumerListener::TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs_(cs)
{
    outFile_ = std::make_unique<std::ofstream>();
    outFile_->open(name.data(), std::ios::out | std::ios::binary);
}

TestConsumerListener::~TestConsumerListener()
{
    if (outFile_ != nullptr) {
        outFile_->close();
    }
}

void TestConsumerListener::OnBufferAvailable()
{
    sptr<SurfaceBuffer> buffer;
    int32_t flushFence;

    cs_->AcquireBuffer(buffer, flushFence, timestamp_, damage_);

    (void)outFile_->write(reinterpret_cast<char *>(buffer->GetVirAddr()), buffer->GetSize());
    cs_->ReleaseBuffer(buffer, -1);
}

static sptr<Surface> GetSurface()
{
    sptr<Surface> cs = Surface::CreateSurfaceAsConsumer();
    sptr<IBufferConsumerListener> listener = new TestConsumerListener(cs, "./surface");
    cs->RegisterConsumerListener(listener);
    auto p = cs->GetProducer();
    sptr<Surface> ps = Surface::CreateSurfaceAsProducer(p);
    return ps;
}

class HevcDecoderTest : public ::testing::Test {
protected:
    void SetUUp()
    {
        std::cout << "[SetUp]: SetUp!!!" << std::endl;
        outputSurface = GetSurface();
    }

    void TearDown()
    {
        std::cout << "[TearDown]: over!!!" << std::endl;
        outputSurface = nullptr;
    }

    sptr<Surface> outputSurface;
};

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: HevcDecoderTest_001
 * @tc.desc  : Test NotifyMemoryRecycle when surface is null.
 */
HWTEST_F(HevcDecoderTest, NotifyMemoryRecycle_ReturnError_HEVC_01, TestSize.Level0)
{
    HevcDecoder hevcDecoder("hevc_decoder");
    Format format;
    EXPECT_EQ(hevcDecoder.Configure(format), AVCS_ERR_OK);
    hevcDecoder.state_ = HevcDecoder::State::RUNNING;
    EXPECT_EQ(hevcDecoder.AllocateBuffers(), AVCS_ERR_OK);
    EXPECT_EQ(hevcDecoder.NotifyMemoryRecycle(), AVCS_ERR_UNKNOWN);
    hevcDecoder.Release();
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: HevcDecoderTest_002
 * @tc.desc  : Test NotifyMemoryWriteBack when surface is null.
 */
HWTEST_F(HevcDecoderTest, NotifyMemoryRecycle_ReturnError_HEVC_02, TestSize.Level0)
{
    HevcDecoder hevcDecoder("hevc_decoder");
    EXPECT_EQ(hevcDecoder.NotifyMemoryWriteBack(), AVCS_ERR_UNKNOWN);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: HevcDecoderTest_003
 * @tc.desc  : Test NotifyMemoryRecycle when state is not RUNNING, FLUSHED or EOS.
 */
HWTEST_F(HevcDecoderTest, NotifyMemoryRecycle_ReturnError_HEVC_03, TestSize.Level0)
{
    HevcDecoder hevcDecoder("hevc_decoder");
    hevcDecoder.sInfo_.surface = outputSurface;
    EXPECT_EQ(hevcDecoder.NotifyMemoryRecycle(), AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: HevcDecoderTest_004
 * @tc.desc  : Test NotifyMemoryWriteBack when state is FREEZEING or FROZEN.
 */
HWTEST_F(HevcDecoderTest, NotifyMemoryRecycle_ReturnError_HEVC_04, TestSize.Level0)
{
    HevcDecoder hevcDecoder("hevc_decoder");
    hevcDecoder.sInfo_.surface = outputSurface;
    hevcDecoder.state_ = HevcDecoder::State::RUNNING;
    EXPECT_EQ(hevcDecoder.NotifyMemoryWriteBack(), AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: HevcDecoderTest_005
 * @tc.desc  : Test NotifyMemoryRecycle when state is RUNNING, FLUSHED or EOS.
 */
HWTEST_F(HevcDecoderTest, NotifyMemoryRecycle_ReturnError_HEVC_05, TestSize.Level0)
{
    HevcDecoder hevcDecoder("hevc_decoder");
    hevcDecoder.sInfo_.surface = outputSurface;
    hevcDecoder.state_ = HevcDecoder::State::RUNNING;
    EXPECT_EQ(hevcDecoder.NotifyMemoryRecycle(), AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: HevcDecoderTest_006
 * @tc.desc  : Test NotifyMemoryRecycle when state is RUNNING, FLUSHED or EOS.
 */
HWTEST_F(HevcDecoderTest, NotifyMemoryRecycle_ReturnError_HEVC_06, TestSize.Level0)
{
    HevcDecoder hevcDecoder("hevc_decoder");
    hevcDecoder.sInfo_.surface = outputSurface;
    hevcDecoder.state_ = HevcDecoder::State::RUNNING;
    EXPECT_EQ(hevcDecoder.NotifyMemoryRecycle(), AVCS_ERR_INVALID_STATE);
    EXPECT_EQ(hevcDecoder.NotifyMemoryWriteBack(), AVCS_ERR_INVALID_STATE);
}