#include "gtest/gtest.h"
#include "meta/format.h"
#include "avcodec_errors.h"
#include "surface.h"
#include "fcodec.h"

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

TestConsumerListener::TestConsumerListener(sptr<Surface> cs, std::string_view name) : cs_(cs) {
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

class FCodecTest : public ::testing::Test {
protected:
    void SetUp()
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
 * @tc.number: FCodecTest_001
 * @tc.desc  : Test NotifyMemoryRecycle when surface is null.
 */
HWTEST_F(FCodecTest, NotifyMemoryRecycle_ReturnError_FCodec_01, TestSize.Level0)
{
    FCodec fCodec("Fcodec");
    Format format;
    EXPECT_EQ(fCodec.Configure(format), AVCS_ERR_OK);
    fCodec.state_ = FCodec::State::RUNNING;
    EXPECT_EQ(fCodec.AllocateBuffers(), AVCS_ERR_OK);
    EXPECT_EQ(fCodec.NotifyMemoryRecycle(), AVCS_ERR_UNKNOWN);
    fCodec.Release();
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: FCodecTest_002
 * @tc.desc  : Test NotifyMemoryWriteBack when surface is null.
 */
HWTEST_F(FCodecTest, NotifyMemoryRecycle_ReturnError_FCodec_02, TestSize.Level0)
{
    FCodec fCodec("Fcodec");
    EXPECT_EQ(fCodec.NotifyMemoryWriteBack(), AVCS_ERR_UNKNOWN);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: FCodecTest_003
 * @tc.desc  : Test NotifyMemoryRecycle when state is not RUNNING, FLUSHED or EOS.
 */
HWTEST_F(FCodecTest, NotifyMemoryRecycle_ReturnError_FCodec_03, TestSize.Level0)
{
    FCodec fCodec("Fcodec");
    fCodec.sInfo_.surface = outputSurface;
    EXPECT_EQ(fCodec.NotifyMemoryRecycle(), AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: FCodecTest_004
 * @tc.desc  : Test NotifyMemoryWriteBack when state is FREEZEING or FROZEN.
 */
HWTEST_F(FCodecTest, NotifyMemoryRecycle_ReturnError_FCodec_04, TestSize.Level0)
{
    FCodec fCodec("Fcodec");
    fCodec.sInfo_.surface = outputSurface;
    fCodec.state_ = FCodec::State::RUNNING;
    EXPECT_EQ(fCodec.NotifyMemoryWriteBack(), AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: FCodecTest_005
 * @tc.desc  : Test NotifyMemoryRecycle when state is RUNNING, FLUSHED or EOS.
 */
HWTEST_F(FCodecTest, NotifyMemoryRecycle_ReturnError_FCodec_05, TestSize.Level0)
{
    FCodec fCodec("Fcodec");
    fCodec.sInfo_.surface = outputSurface;
    fCodec.state_ = FCodec::State::RUNNING;
    EXPECT_EQ(fCodec.NotifyMemoryRecycle(), AVCS_ERR_INVALID_STATE);
}

/**
 * @tc.name  : MemoryRecycleTest
 * @tc.number: FCodecTest_006
 * @tc.desc  : Test NotifyMemoryRecycle when state is RUNNING, FLUSHED or EOS.
 */
HWTEST_F(FCodecTest, NotifyMemoryRecycle_ReturnError_FCodec_06, TestSize.Level0)
{
    FCodec fCodec("Fcodec");
    fCodec.sInfo_.surface = outputSurface;
    fCodec.state_ = FCodec::State::RUNNING;
    EXPECT_EQ(fCodec.NotifyMemoryRecycle(), AVCS_ERR_INVALID_STATE);
    EXPECT_EQ(fCodec.NotifyMemoryWriteBack(), AVCS_ERR_INVALID_STATE);
}