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

#include "dash_adpt_set_manager_unit_test.h"
#include "dash_adpt_set_manager.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing::ext;

void DashAdptSetManagerUnitTest::SetUpTestCase(void) {}

void DashAdptSetManagerUnitTest::TearDownTestCase(void) {}

void DashAdptSetManagerUnitTest::SetUp(void) {}

void DashAdptSetManagerUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test Init API
 * @tc.number: Dash_Adpt_SetManager_Init_001
 * @tc.desc  : Test Dash Adpt Set Manager Init interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_Init_001, TestSize.Level1)
{
    auto setManager = std::make_shared<DashAdptSetManager>(nullptr);
    ASSERT_NE(nullptr, setManager);
    ASSERT_EQ(nullptr, setManager->adptSetInfo_);
    ASSERT_EQ(nullptr, setManager->previousAdptSetInfo_);
}

/**
 * @tc.name  : Test ParseInitSegment API
 * @tc.number: Dash_Adpt_SetManager_ParseInitSegment_001
 * @tc.desc  : Test Dash Adpt Set Manager ParseInitSegment interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_ParseInitSegment_001, TestSize.Level1)
{
    DashUrlType init {.sourceUrl_ = "mock url", .range_ = "mock range"};
    DashSegBaseInfo baseInfo {.initialization_ = &init};
    DashAdptSetInfo info {.adptSetSegBase_ = &baseInfo};
    DashAdptSetManager setManager;
    setManager.adptSetInfo_ = &info;
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    setManager.ParseInitSegment();
    ASSERT_NE(nullptr, setManager.initSegment_);
}

/**
 * @tc.name  : Test ParseInitSegment API
 * @tc.number: Dash_Adpt_SetManager_ParseInitSegment_002
 * @tc.desc  : Test Dash Adpt Set Manager ParseInitSegment interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_ParseInitSegment_002, TestSize.Level1)
{
    DashUrlType *init = new DashUrlType();
    DashAdptSetInfo info;
    DashAdptSetManager setManager;
    setManager.adptSetInfo_ = &info;
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    setManager.initSegment_ = init;
    ASSERT_EQ(nullptr, setManager.adptSetInfo_->adptSetSegBase_);
    ASSERT_EQ(nullptr, setManager.adptSetInfo_->adptSetSegList_);
    ASSERT_EQ(nullptr, setManager.adptSetInfo_->adptSetSegTmplt_);
    ASSERT_NE(nullptr, setManager.initSegment_);
    setManager.ParseInitSegment();
    ASSERT_EQ(nullptr, setManager.initSegment_);
}

/**
 * @tc.name  : Test ParseInitSegmentBySegTmplt API
 * @tc.number: Dash_Adpt_SetManager_ParseInitSegmentBySegTmplt_001
 * @tc.desc  : Test Dash Adpt Set Manager ParseInitSegmentBySegTmplt interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_ParseInitSegmentBySegTmplt_001, TestSize.Level1)
{
    DashUrlType init {.sourceUrl_ = "mock url", .range_ = "mock range"};
    DashSegTmpltInfo tmpltInfo;
    tmpltInfo.multSegBaseInfo_.segBaseInfo_.initialization_ = &init;
    DashAdptSetInfo info {.adptSetSegTmplt_ = &tmpltInfo};
    DashAdptSetManager setManager;
    setManager.adptSetInfo_ = &info;
    setManager.ParseInitSegmentBySegTmplt();
    ASSERT_NE(nullptr, setManager.initSegment_);
    tmpltInfo.multSegBaseInfo_.segBaseInfo_.initialization_ = nullptr;
    setManager.ParseInitSegmentBySegTmplt();
    ASSERT_EQ(nullptr, setManager.initSegment_);
}

/**
 * @tc.name  : Test GetBaseUrlList API
 * @tc.number: Dash_Adpt_SetManager_GetBaseUrlList_001
 * @tc.desc  : Test Dash Adpt Set Manager GetBaseUrlList interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetBaseUrlList_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(nullptr, setManager.adptSetInfo_);
    std::list<std::string> baseUrlList;
    setManager.GetBaseUrlList(baseUrlList);
    ASSERT_EQ(0, baseUrlList.size());
}

/**
 * @tc.name  : Test GetBandwidths API
 * @tc.number: Dash_Adpt_SetManager_GetBandwidths_001
 * @tc.desc  : Test Dash Adpt Set Manager GetBandwidths interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetBandwidths_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(0, setManager.representationList_.size());
    DashVector<uint32_t> bandwidths;
    setManager.GetBandwidths(bandwidths);
    ASSERT_EQ(0, bandwidths.size());
    DashRepresentationInfo info;
    setManager.representationList_.emplace_back(&info);
    setManager.representationList_.emplace_back(nullptr);
    setManager.GetBandwidths(bandwidths);
    ASSERT_EQ(1, bandwidths.size());
}

/**
 * @tc.name  : Test GetRepresentationByBandwidth API
 * @tc.number: Dash_Adpt_SetManager_GetRepresentationByBandwidth_001
 * @tc.desc  : Test Dash Adpt Set Manager GetRepresentationByBandwidth interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetRepresentationByBandwidth_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(0, setManager.representationList_.size());
    ASSERT_EQ(nullptr, setManager.GetRepresentationByBandwidth(0));
    DashRepresentationInfo info {.bandwidth_ = 1};
    setManager.representationList_.emplace_back(&info);
    ASSERT_EQ(nullptr, setManager.GetRepresentationByBandwidth(0));
}

/**
 * @tc.name  : Test GetHighRepresentation API
 * @tc.number: Dash_Adpt_SetManager_GetHighRepresentation_001
 * @tc.desc  : Test Dash Adpt Set Manager GetHighRepresentation interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetHighRepresentation_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(0, setManager.representationList_.size());
    ASSERT_EQ(nullptr, setManager.GetHighRepresentation());
}

/**
 * @tc.name  : Test GetLowRepresentation API
 * @tc.number: Dash_Adpt_SetManager_GetLowRepresentation_001
 * @tc.desc  : Test Dash Adpt Set Manager GetLowRepresentation interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetLowRepresentation_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(0, setManager.representationList_.size());
    ASSERT_EQ(nullptr, setManager.GetLowRepresentation());
}

/**
 * @tc.name  : Test GetMime API
 * @tc.number: Dash_Adpt_SetManager_GetMime_001
 * @tc.desc  : Test Dash Adpt Set Manager GetMime interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetMime_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(true, setManager.GetMime().empty());
}

/**
 * @tc.name  : Test GetMime API
 * @tc.number: Dash_Adpt_SetManager_GetMime_002
 * @tc.desc  : Test Dash Adpt Set Manager GetMime interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetMime_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    std::string mimeType = "mockMimeType";
    info.commonAttrsAndElements_.mimeType_ = mimeType;
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(mimeType, setManager.GetMime());
}

/**
 * @tc.name  : Test GetMime API
 * @tc.number: Dash_Adpt_SetManager_GetMime_003
 * @tc.desc  : Test Dash Adpt Set Manager GetMime interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetMime_003, TestSize.Level1)
{
    DashAdptSetInfo info;
    std::string mimeType = "mockMimeType";
    DashRepresentationInfo reInfo;
    reInfo.commonAttrsAndElements_.mimeType_ = mimeType;
    info.representationList_.emplace_back(&reInfo);
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(mimeType, setManager.GetMime());
}

/**
 * @tc.name  : Test GetMime API
 * @tc.number: Dash_Adpt_SetManager_GetMime_004
 * @tc.desc  : Test Dash Adpt Set Manager GetMime interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetMime_004, TestSize.Level1)
{
    DashAdptSetInfo info;
    std::string mimeType = "";
    DashRepresentationInfo reInfo;
    reInfo.commonAttrsAndElements_.mimeType_ = mimeType;
    info.representationList_.emplace_back(nullptr);
    info.representationList_.emplace_back(&reInfo);
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(mimeType, setManager.GetMime());
}

/**
 * @tc.name  : Test IsOnDemand API
 * @tc.number: Dash_Adpt_SetManager_IsOnDemand_001
 * @tc.desc  : Test Dash Adpt Set Manager IsOnDemand interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(false, setManager.IsOnDemand());
}

/**
 * @tc.name  : Test IsOnDemand API
 * @tc.number: Dash_Adpt_SetManager_IsOnDemand_002
 * @tc.desc  : Test Dash Adpt Set Manager IsOnDemand interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(0, setManager.adptSetInfo_->representationList_.size());
    ASSERT_EQ(nullptr, setManager.adptSetInfo_->adptSetSegBase_);
    ASSERT_EQ(false, setManager.IsOnDemand());
}

/**
 * @tc.name  : Test IsOnDemand API
 * @tc.number: Dash_Adpt_SetManager_IsOnDemand_003
 * @tc.desc  : Test Dash Adpt Set Manager IsOnDemand interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_003, TestSize.Level1)
{
    DashSegBaseInfo baseInfo;
    DashAdptSetInfo info {.adptSetSegBase_ = &baseInfo};
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(0, setManager.adptSetInfo_->representationList_.size());
    ASSERT_NE(nullptr, setManager.adptSetInfo_->adptSetSegBase_);
    ASSERT_EQ(false, setManager.adptSetInfo_->adptSetSegBase_->indexRange_.size() > 0);
    ASSERT_EQ(false, setManager.IsOnDemand());
}

/**
 * @tc.name  : Test IsOnDemand API
 * @tc.number: Dash_Adpt_SetManager_IsOnDemand_004
 * @tc.desc  : Test Dash Adpt Set Manager IsOnDemand interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_004, TestSize.Level1)
{
    DashSegBaseInfo baseInfo {.indexRange_ = "mock index range"};
    DashAdptSetInfo info {.adptSetSegBase_ = &baseInfo};
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(0, setManager.adptSetInfo_->representationList_.size());
    ASSERT_NE(nullptr, setManager.adptSetInfo_->adptSetSegBase_);
    ASSERT_EQ(true, setManager.adptSetInfo_->adptSetSegBase_->indexRange_.size() > 0);
    ASSERT_EQ(true, setManager.IsOnDemand());
}

/**
 * @tc.name  : Test IsOnDemand API
 * @tc.number: Dash_Adpt_SetManager_IsOnDemand_005
 * @tc.desc  : Test Dash Adpt Set Manager IsOnDemand interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_005, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashSegListInfo segInfo;
    DashRepresentationInfo reInfo {.representationSegList_ = &segInfo};
    info.representationList_.emplace_back(&reInfo);
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(1, setManager.adptSetInfo_->representationList_.size());
    ASSERT_EQ(false, setManager.IsOnDemand());
}

/**
 * @tc.name  : Test IsOnDemand API
 * @tc.number: Dash_Adpt_SetManager_IsOnDemand_006
 * @tc.desc  : Test Dash Adpt Set Manager IsOnDemand interface
 */
HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_006, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashSegTmpltInfo segInfo;
    DashRepresentationInfo reInfo {.representationSegTmplt_ = &segInfo};
    info.representationList_.emplace_back(&reInfo);
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(1, setManager.adptSetInfo_->representationList_.size());
    ASSERT_EQ(false, setManager.IsOnDemand());
}
}
}
}
}