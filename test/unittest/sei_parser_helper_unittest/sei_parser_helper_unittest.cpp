/*
 * Copyright (C) 2025 Huawei Device Co., Ltd.
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
namespace Pipeline {
using namespace OHOS;
using namespace OHOS::Media;
using namespace std;
using namespace testing;
using namespace testing::ext;
// const static int32_t INVALID_NUM = -1;
const static int32_t NUM_TEST = 0;
// const static int32_t ID_TEST = 0;
const static int32_t TIME_TEST = 10;
const static std::string NAME_TEST = "name/test";
class MockEventReceiver : public EventReceiver {
public:
    MockEventReceiver() = default;
    void OnEvent(const Event &event){};
};

void AvcSeiParserHelperUnittest::SetUpTestCase(void) {}

void AvcSeiParserHelperUnittest::TearDownTestCase(void) {}

void AvcSeiParserHelperUnittest::SetUp(void)
{
    avcSeiParserHelper_ = std::make_shared<AvcSeiParserHelper>();
}

void AvcSeiParserHelperUnittest::TearDown(void)
{
    ASSERT_NE(avcSeiParserHelper_, nullptr);
    avcSeiParserHelper_ = nullptr;
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
}  // namespace Pipeline
}  // namespace Media
}  // namespace OHOS
