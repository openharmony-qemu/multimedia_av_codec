/*
 * Copyright (C) 2026 Huawei Device Co., Ltd.
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

#include "gmock/gmock.h"
#include "sei_parser_helper_unittest.h"

namespace OHOS {
namespace Media {
namespace Test {}
namespace Pipeline {
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;
const static int32_t NUM_TEST = 0;
const static int32_t TIME_TEST = 10;
const static std::string NAME_TEST = "name/test";
constexpr uint16_t AVC_SEI_TYPE = 0x06;
constexpr uint16_t HEVC_SEI_TYPE_ONE = 0x4E;
constexpr uint8_t SEI_ASSEMBLE_BYTE = 0xFF;
const uint8_t VALID_SEI_DATA[] = {0x00, 0x00, 0x00, 0x01, 0x86, 0x00, 0x00};

class MockEventReceiver : public EventReceiver {
public:
    MockEventReceiver() = default;
    void OnEvent(const Event &event){};
};

class TestSeiParserHelper : public SeiParserHelper {
public:
    TestSeiParserHelper() = default;
    ~TestSeiParserHelper() = default;
    
    bool IsSeiNalu(uint8_t *&headerPtr) override
    {
        return false;
    }
};

void AvcSeiParserHelperUnittest::SetUpTestCase(void) {}

void AvcSeiParserHelperUnittest::TearDownTestCase(void) {}

void SeiParserHelperUnittest::SetUpTestCase(void) {}

void SeiParserHelperUnittest::TearDownTestCase(void) {}

void SeiParserListenerUnittest::SetUpTestCase(void) {}

void SeiParserListenerUnittest::TearDownTestCase(void) {}

void HevcSeiParserHelperUnittest::SetUpTestCase(void) {}

void HevcSeiParserHelperUnittest::TearDownTestCase(void) {}

void AvcSeiParserHelperUnittest::SetUp(void)
{
    avcSeiParserHelper_ = std::make_shared<AvcSeiParserHelper>();
}

void AvcSeiParserHelperUnittest::TearDown(void)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    avcSeiParserHelper_ = nullptr;
}

void SeiParserHelperUnittest::SetUp(void)
{
    SeiParserHelper_ = std::make_shared<TestSeiParserHelper>();
}

void SeiParserHelperUnittest::TearDown(void)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    SeiParserHelper_ = nullptr;
}

void SeiParserListenerUnittest::SetUp(void)
{
    sptr<AVBufferQueueProducer> producer = nullptr;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver = std::make_shared<MockEventReceiver>();
    SeiParserListener_ = std::make_shared<SeiParserListener>(
        "video/avc", producer, eventReceiver, false);
}

void SeiParserListenerUnittest::TearDown(void)
{
    ASSERT_NE(SeiParserListener_, nullptr);
    SeiParserListener_ = nullptr;
}

void HevcSeiParserHelperUnittest::SetUp(void)
{
    HevcSeiParserHelper_ = std::make_shared<HevcSeiParserHelper>();
}

void HevcSeiParserHelperUnittest::TearDown(void)
{
    ASSERT_NE(HevcSeiParserHelper_, nullptr);
    HevcSeiParserHelper_ = nullptr;
}

/**
 * @tc.name: Test ParseSeiRbsp  API
 * @tc.number: ParseSeiRbsp_001
 * @tc.desc: Test bodyPtr + SEI_UUID_LEN >= maxPtr
 */
HWTEST_F(AvcSeiParserHelperUnittest, ParseSeiRbsp_001, TestSize.Level0)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    uint8_t buffer[10];
    uint8_t* bodyPtr = buffer + sizeof(buffer);
    const uint8_t* const maxPtr = buffer;
    std::shared_ptr<SeiPayloadInfoGroup> group = std::make_shared<SeiPayloadInfoGroup>();
    auto ret = avcSeiParserHelper_->ParseSeiRbsp(bodyPtr, maxPtr, group);
    EXPECT_EQ(ret, Status::ERROR_UNSUPPORTED_FORMAT);
}

/**
 * @tc.name: Test SeiParserListener Constructor&FlowLimit  API
 * @tc.number: SeiParserListenerConstructor_001
 * @tc.desc: Test Constructor all
 *           Test FlowLimit all
 */
HWTEST_F(AvcSeiParserHelperUnittest, SeiParserListenerConstructor_001, TestSize.Level0)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    auto mockProducer = new MockAVBufferQueueProducer();
    EXPECT_CALL(*(mockProducer), SetBufferFilledListener(_)).WillRepeatedly(Return(Status::OK));
    sptr<AVBufferQueueProducer> producer = mockProducer;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver = std::make_shared<MockEventReceiver>();
    bool isFlowLimited = false;
    auto seiParserListener = std::make_shared<SeiParserListener>(NAME_TEST, producer, eventReceiver, isFlowLimited);

    seiParserListener->seiParserHelper_ = avcSeiParserHelper_;
    std::shared_ptr<AVBuffer> avBuffer = std::make_shared<AVBuffer>();
    auto mockSyncCenter = std::make_shared<MediaSyncManager>();
    EXPECT_CALL(*(mockSyncCenter), GetMediaTimeNow()).WillRepeatedly(Return(NUM_TEST));
    seiParserListener->syncCenter_ = mockSyncCenter;
    seiParserListener->startPts_ = NUM_TEST;
    avBuffer->pts_ = TIME_TEST;
    seiParserListener->FlowLimit(avBuffer);
    EXPECT_NE(seiParserListener->startPts_, avBuffer->pts_);
}

/**
 * @tc.name: Test SetPayloadTypeVec  API
 * @tc.number: SeiParserListenerSetPayloadTypeVec_001
 * @tc.desc: Test all
 */
HWTEST_F(AvcSeiParserHelperUnittest, SeiParserListenerSetPayloadTypeVec_001, TestSize.Level0)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    auto mockProducer = new MockAVBufferQueueProducer();
    EXPECT_CALL(*(mockProducer), SetBufferFilledListener(_)).WillRepeatedly(Return(Status::OK));
    sptr<AVBufferQueueProducer> producer = mockProducer;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver = std::make_shared<MockEventReceiver>();
    bool isFlowLimited = false;
    auto seiParserListener = std::make_shared<SeiParserListener>(NAME_TEST, producer, eventReceiver, isFlowLimited);
    seiParserListener->seiParserHelper_ = avcSeiParserHelper_;
    std::vector<int32_t> vector;
    seiParserListener->SetPayloadTypeVec(vector);
    EXPECT_EQ(avcSeiParserHelper_->payloadTypeVec_, vector);
}

/**
 * @tc.name: Test OnInterrupted  API
 * @tc.number: SeiParserListenerOnInterrupted_001
 * @tc.desc: Test all
 */
HWTEST_F(AvcSeiParserHelperUnittest, SeiParserListenerOnInterrupted_001, TestSize.Level0)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    auto mockProducer = new MockAVBufferQueueProducer();
    EXPECT_CALL(*(mockProducer), SetBufferFilledListener(_)).WillRepeatedly(Return(Status::OK));
    sptr<AVBufferQueueProducer> producer = mockProducer;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver = std::make_shared<MockEventReceiver>();
    bool isFlowLimited = false;
    auto seiParserListener = std::make_shared<SeiParserListener>(NAME_TEST, producer, eventReceiver, isFlowLimited);
    seiParserListener->seiParserHelper_ = avcSeiParserHelper_;
    bool isInterruptNeeded = true;
    seiParserListener->OnInterrupted(isInterruptNeeded);
    EXPECT_EQ(seiParserListener->isInterruptNeeded_, isInterruptNeeded);
}

/**
 * @tc.name: Test SetSeiMessageCbStatus  API
 * @tc.number: SeiParserListenerSetSeiMessageCbStatus_001
 * @tc.desc: Test status == true
 *           Test status == false && payloadTypes.empty() == true
 *           Test status == false && payloadTypes.empty() == false
 */
HWTEST_F(AvcSeiParserHelperUnittest, SeiParserListenerSetSeiMessageCbStatus_001, TestSize.Level0)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    auto mockProducer = new MockAVBufferQueueProducer();
    EXPECT_CALL(*(mockProducer), SetBufferFilledListener(_)).WillRepeatedly(Return(Status::OK));
    sptr<AVBufferQueueProducer> producer = mockProducer;
    std::shared_ptr<Pipeline::EventReceiver> eventReceiver = std::make_shared<MockEventReceiver>();
    bool isFlowLimited = false;
    auto seiParserListener = std::make_shared<SeiParserListener>(NAME_TEST, producer, eventReceiver, isFlowLimited);
    seiParserListener->seiParserHelper_ = avcSeiParserHelper_;
    bool status = true;
    std::vector<int32_t> payloadTypes;
    auto ret = seiParserListener->SetSeiMessageCbStatus(status, payloadTypes);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(seiParserListener->payloadTypes_, payloadTypes);
    EXPECT_EQ(avcSeiParserHelper_->payloadTypeVec_, payloadTypes);

    status = false;
    ret = seiParserListener->SetSeiMessageCbStatus(status, payloadTypes);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_EQ(seiParserListener->payloadTypes_, payloadTypes);
    EXPECT_EQ(avcSeiParserHelper_->payloadTypeVec_, payloadTypes);

    payloadTypes.push_back(NUM_TEST);
    payloadTypes.push_back(NUM_TEST);
    ret = seiParserListener->SetSeiMessageCbStatus(status, payloadTypes);
    EXPECT_EQ(ret, Status::OK);
    EXPECT_NE(seiParserListener->payloadTypes_, payloadTypes);
    EXPECT_EQ(seiParserListener->payloadTypes_.size(), 0);
    EXPECT_EQ(avcSeiParserHelper_->payloadTypeVec_, seiParserListener->payloadTypes_);
}

/**
 * @tc.name: Test ParseSeiPayload  API
 * @tc.number: ParseSeiPayload_001
 * @tc.desc: return Status::ERROR_INVALID_DATA;
 */
HWTEST_F(SeiParserHelperUnittest, ParseSeiPayload_001, TestSize.Level1)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    std::shared_ptr<AVBuffer> buffer = std::make_shared<AVBuffer>();
    auto mockMemory = std::make_shared<MockMemory>(static_cast<size_t>(32));
    uint8_t testMemory[32] = {0};
    std::copy_n(VALID_SEI_DATA, std::min(sizeof(testMemory), sizeof(VALID_SEI_DATA)), testMemory);
    EXPECT_CALL(*mockMemory, GetRealAddr()).WillRepeatedly(Return(testMemory));
    std::shared_ptr<SeiPayloadInfoGroup> group = nullptr;
    auto ret = SeiParserHelper_->ParseSeiPayload(buffer, group);
    EXPECT_EQ(ret, Status::ERROR_INVALID_DATA);
}

/**
 * @tc.name: Test SetPayloadTypeVec  API
 * @tc.number: SetPayloadTypeVec_001
 * @tc.desc: vector<int32_t> testVec = {1, 5, 8, 10};
 */
HWTEST_F(SeiParserHelperUnittest, SetPayloadTypeVec_001, TestSize.Level1)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    std::vector<int32_t> testVec = {1, 5, 8, 10};
    SeiParserHelper_->SetPayloadTypeVec(testVec);
    const auto& resultVec = SeiParserHelper_->payloadTypeVec_;
    EXPECT_EQ(resultVec.size(), testVec.size());
    for (size_t i = 0; i < testVec.size(); ++i) {
        EXPECT_EQ(resultVec[i], testVec[i]);
    }
}

/**
 * @tc.name: Test SetPayloadTypeVec  API
 * @tc.number: SetPayloadTypeVec_002
 * @tc.desc: vector<int32_t> emptyVec;
 */
HWTEST_F(SeiParserHelperUnittest, SetPayloadTypeVec_002, TestSize.Level1)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    std::vector<int32_t> initVec = {2, 4};
    SeiParserHelper_->SetPayloadTypeVec(initVec);
    EXPECT_EQ(SeiParserHelper_->payloadTypeVec_.size(), 2);
    std::vector<int32_t> emptyVec;
    SeiParserHelper_->SetPayloadTypeVec(emptyVec);
    EXPECT_TRUE(SeiParserHelper_->payloadTypeVec_.empty());
}

/**
 * @tc.name: Test FindNextSeiNaluPos  API
 * @tc.number: FindNextSeiNaluPos_001
 * @tc.desc: startPtr >= maxPtr
 */
HWTEST_F(SeiParserHelperUnittest, FindNextSeiNaluPos_001, TestSize.Level1)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    uint8_t startByte = 0x02;
    uint8_t maxByte = 0x08;
    uint8_t* startPtr = &startByte;
    const uint8_t* maxPtr = &maxByte;
    bool result = SeiParserHelper_->FindNextSeiNaluPos(startPtr, maxPtr);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: Test FindNextSeiNaluPos  API
 * @tc.number: FindNextSeiNaluPos_002
 * @tc.desc: startPtr < maxPtr
 *           *startPtr & SEI_BYTE_MASK_HIGH_7BITS
 */
HWTEST_F(SeiParserHelperUnittest, FindNextSeiNaluPos_002, TestSize.Level1)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    uint8_t testBuffer[32] = {0};
    testBuffer[0] = 0x86;
    testBuffer[1] = 0x00;
    testBuffer[2] = 0x00;
    testBuffer[3] = 0x01;
    uint8_t* startPtr = testBuffer;
    const uint8_t* maxPtr = testBuffer + 10;
    bool result = SeiParserHelper_->FindNextSeiNaluPos(startPtr, maxPtr);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: Test FindNextSeiNaluPos  API
 * @tc.number: FindNextSeiNaluPos_003
 * @tc.desc: startPtr < maxPtr
 *           *startPtr == 0
 */
HWTEST_F(SeiParserHelperUnittest, FindNextSeiNaluPos_003, TestSize.Level1)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    uint8_t testBuffer[32] = {0};
    testBuffer[0] = 0x00;
    testBuffer[1] = 0x00;
    testBuffer[2] = 0x01;
    testBuffer[3] = 0x86;
    uint8_t* startPtr = testBuffer;
    const uint8_t* maxPtr = testBuffer + 6;
    bool result = SeiParserHelper_->FindNextSeiNaluPos(startPtr, maxPtr);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: Test IsSeiNalu  API
 * @tc.number: IsSeiNalu_001
 * @tc.desc: return true
 */
HWTEST_F(AvcSeiParserHelperUnittest, IsSeiNalu_001, TestSize.Level1)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    uint8_t seiHeader = AVC_SEI_TYPE;
    uint8_t* headerPtr = &seiHeader;
    bool result = avcSeiParserHelper_->IsSeiNalu(headerPtr);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: Test IsSeiNalu  API
 * @tc.number: IsSeiNalu_002
 * @tc.desc: return false
 */
HWTEST_F(AvcSeiParserHelperUnittest, IsSeiNalu_002, TestSize.Level1)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    uint8_t seiHeader = 0x01;
    uint8_t* headerPtr = &seiHeader;
    bool result = avcSeiParserHelper_->IsSeiNalu(headerPtr);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: Test IsSeiNalu  API
 * @tc.number: IsSeiNalu_001
 * @tc.desc: return true
 */
HWTEST_F(HevcSeiParserHelperUnittest, IsSeiNalu_001, TestSize.Level1)
{
    ASSERT_NE(HevcSeiParserHelper_, nullptr);
    uint8_t seiHeader = HEVC_SEI_TYPE_ONE;
    uint8_t* headerPtr = &seiHeader;
    bool result = HevcSeiParserHelper_->IsSeiNalu(headerPtr);
    EXPECT_EQ(result, true);
}

/**
 * @tc.name: Test IsSeiNalu  API
 * @tc.number: IsSeiNalu_002
 * @tc.desc: return false
 */
HWTEST_F(HevcSeiParserHelperUnittest, IsSeiNalu_002, TestSize.Level1)
{
    ASSERT_NE(HevcSeiParserHelper_, nullptr);
    uint8_t seiHeader = 0x06;
    uint8_t* headerPtr = &seiHeader;
    bool result = HevcSeiParserHelper_->IsSeiNalu(headerPtr);
    EXPECT_EQ(result, false);
}

/**
 * @tc.name: Test GetSeiTypeOrSize  API
 * @tc.number: GetSeiTypeOrSize_001
 * @tc.desc: Test all
 */
HWTEST_F(SeiParserHelperUnittest, GetSeiTypeOrSize_001, TestSize.Level1)
{
    ASSERT_NE(SeiParserHelper_, nullptr);
    const int expectedValue = SEI_ASSEMBLE_BYTE;
    uint8_t testData[] = {SEI_ASSEMBLE_BYTE, 0x01};
    uint8_t* dataPtr = testData;
    uint8_t*& dataPtrRef = dataPtr;
    const uint8_t* maxDataPtr = testData + sizeof(testData);
    int32_t res = SeiParserHelper_->GetSeiTypeOrSize(dataPtrRef, maxDataPtr);
    EXPECT_EQ(res, expectedValue);
}

/**
 * @tc.name: Test OnInterrupted  API
 * @tc.number: OnInterrupted_001
 * @tc.desc: Test all
 */
HWTEST_F(SeiParserListenerUnittest, OnInterrupted_001, TestSize.Level1)
{
    ASSERT_NE(SeiParserListener_, nullptr);
    SeiParserListener_->OnInterrupted(true);
    EXPECT_TRUE(SeiParserListener_->isInterruptNeeded_.load());
}

/**
 * @tc.name: Test SetSeiMessageCbStatus  API
 * @tc.number: SetSeiMessageCbStatus_001
 * @tc.desc: status == true
 */
HWTEST_F(SeiParserListenerUnittest, SetSeiMessageCbStatus_001, TestSize.Level1)
{
    ASSERT_NE(SeiParserListener_, nullptr);
    std::vector<int32_t> testVec = {1, 2, 3};
    SeiParserListener_->SetPayloadTypeVec(testVec);
    auto result = SeiParserListener_->SetSeiMessageCbStatus(true, testVec);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: Test SetSeiMessageCbStatus  API
 * @tc.number: SetSeiMessageCbStatus_002
 * @tc.desc: status == false, payloadTypes is empty
 */
HWTEST_F(SeiParserListenerUnittest, SetSeiMessageCbStatus_002, TestSize.Level1)
{
    ASSERT_NE(SeiParserListener_, nullptr);
    std::vector<int32_t> testVec = {};
    SeiParserListener_->SetPayloadTypeVec(testVec);
    auto result = SeiParserListener_->SetSeiMessageCbStatus(false, testVec);
    EXPECT_EQ(result, Status::OK);
}

/**
 * @tc.name: Test SetSeiMessageCbStatus  API
 * @tc.number: SetSeiMessageCbStatus_003
 * @tc.desc: status == false, payloadTypes is not empty
 */
HWTEST_F(SeiParserListenerUnittest, SetSeiMessageCbStatus_003, TestSize.Level1)
{
    ASSERT_NE(SeiParserListener_, nullptr);
    std::vector<int32_t> testVec = {1, 2, 3};
    SeiParserListener_->SetPayloadTypeVec(testVec);
    auto result = SeiParserListener_->SetSeiMessageCbStatus(false, testVec);
    EXPECT_EQ(result, Status::OK);
}
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS