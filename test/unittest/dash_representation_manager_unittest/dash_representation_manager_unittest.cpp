/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include <iostream>
#include <algorithm>
#include "dash_representation_manager_unittest.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing;
using namespace testing::ext;
const static int32_t ID_TEST = 1;
const static int32_t NUM_TEST = 0;
void DashRepresentationManagerUnittest::SetUpTestCase(void) {}
void DashRepresentationManagerUnittest::TearDownTestCase(void) {}
void DashRepresentationManagerUnittest::SetUp(void)
{
    dashPtr_ = std::make_shared<DashRepresentationManager>();
}
void DashRepresentationManagerUnittest::TearDown(void)
{
    dashPtr_ = nullptr;
}

/**
 * @tc.name  : Test Reset
 * @tc.number: Reset_001
 * @tc.desc  : Test all
 */
HWTEST_F(DashRepresentationManagerUnittest, Reset_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    auto representationInfo = new DashRepresentationInfo();
    dashPtr_->representationInfo_ = representationInfo;
    EXPECT_NE(dashPtr_->representationInfo_, nullptr);
    dashPtr_->Reset();
    EXPECT_EQ(dashPtr_->representationInfo_, nullptr);
    EXPECT_EQ(dashPtr_->segTmpltFlag_, NUM_TEST);
    delete representationInfo;
}

/**
 * @tc.name  : Test Init
 * @tc.number: Init_001
 * @tc.desc  : Test representationInfo_ == nullptr
 */
HWTEST_F(DashRepresentationManagerUnittest, Init_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    dashPtr_->segTmpltFlag_ = ID_TEST;
    dashPtr_->representationInfo_ = nullptr;
    dashPtr_->Init();
    EXPECT_EQ(dashPtr_->segTmpltFlag_, ID_TEST);

    auto representationInfo = new DashRepresentationInfo();
    dashPtr_->representationInfo_ = representationInfo;
    EXPECT_NE(dashPtr_->representationInfo_, nullptr);
    dashPtr_->Init();
    EXPECT_EQ(dashPtr_->segTmpltFlag_, NUM_TEST);
    delete representationInfo;
    dashPtr_->representationInfo_ = nullptr;
}

/**
 * @tc.name  : Test ParseInitSegment
 * @tc.number: ParseInitSegment_001
 * @tc.desc  : Test representationInfo_->representationSegTmplt_ != nullptr
 *             Test initSegment_ != nullptr
 */
HWTEST_F(DashRepresentationManagerUnittest, ParseInitSegment_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    auto representationInfo = new DashRepresentationInfo();
    dashPtr_->representationInfo_ = representationInfo;
    EXPECT_NE(dashPtr_->representationInfo_, nullptr);
    DashSegTmpltInfo *dashSegTmpltInfo = new DashSegTmpltInfo();
    dashSegTmpltInfo->segTmpltInitialization_ = "test";
    dashPtr_->representationInfo_->representationSegTmplt_ = dashSegTmpltInfo;
    EXPECT_NE(dashPtr_->representationInfo_->representationSegTmplt_, nullptr);
    dashPtr_->ParseInitSegment();
    EXPECT_NE(dashPtr_->initSegment_, nullptr);
    EXPECT_EQ(dashPtr_->segTmpltFlag_, ID_TEST);

    delete dashSegTmpltInfo;
    dashPtr_->representationInfo_->representationSegTmplt_ = nullptr;
    dashPtr_->ParseInitSegment();
    EXPECT_EQ(dashPtr_->initSegment_, nullptr);
    delete representationInfo;
    dashPtr_->representationInfo_ = nullptr;
}

/**
 * @tc.name  : Test ParseInitSegmentBySegTmplt
 * @tc.number: ParseInitSegmentBySegTmplt_001
 * @tc.desc  : Test all
 */
HWTEST_F(DashRepresentationManagerUnittest, ParseInitSegmentBySegTmplt_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    auto representationInfo = new DashRepresentationInfo();
    dashPtr_->representationInfo_ = representationInfo;
    DashSegTmpltInfo *dashSegTmpltInfo = new DashSegTmpltInfo();
    DashUrlType *dashUrlType = new DashUrlType();
    dashSegTmpltInfo->multSegBaseInfo_.segBaseInfo_.initialization_ = dashUrlType;
    dashPtr_->representationInfo_->representationSegTmplt_ = dashSegTmpltInfo;
    dashPtr_->ParseInitSegmentBySegTmplt();
    EXPECT_NE(dashPtr_->initSegment_, nullptr);


    delete dashUrlType;
    dashSegTmpltInfo->multSegBaseInfo_.segBaseInfo_.initialization_ = nullptr;
    dashSegTmpltInfo->segTmpltInitialization_ = "test";
    delete dashPtr_->initSegment_;
    dashPtr_->initSegment_ = nullptr;
    dashPtr_->ParseInitSegmentBySegTmplt();
    EXPECT_NE(dashPtr_->initSegment_, nullptr);
    EXPECT_EQ(dashPtr_->segTmpltFlag_, ID_TEST);

    dashSegTmpltInfo->segTmpltInitialization_ = "";
    dashPtr_->ParseInitSegmentBySegTmplt();
    EXPECT_EQ(dashPtr_->initSegment_, nullptr);
    delete dashSegTmpltInfo;
    dashPtr_->representationInfo_->representationSegTmplt_ = nullptr;
    delete representationInfo;
    dashPtr_->representationInfo_ = nullptr;
}

/**
 * @tc.name  : Test GetRepresentationInfo && GetPreviousRepresentationInfo
 * @tc.number: GetRepresentationInfo_001
 * @tc.desc  : Test all
 */
HWTEST_F(DashRepresentationManagerUnittest, GetRepresentationInfo_001, TestSize.Level0)
{
    ASSERT_NE(dashPtr_, nullptr);
    auto ret = dashPtr_->GetRepresentationInfo();
    EXPECT_EQ(ret, nullptr);

    ret = dashPtr_->GetPreviousRepresentationInfo();
    EXPECT_EQ(ret, nullptr);
}
}
}
}
}