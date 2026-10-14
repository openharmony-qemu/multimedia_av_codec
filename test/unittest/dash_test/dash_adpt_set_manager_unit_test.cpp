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

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_Init_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashRepresentationInfo rep1 {.bandwidth_ = 2000};
    DashRepresentationInfo rep2 {.bandwidth_ = 1000};
    info.representationList_.emplace_back(&rep1);
    info.representationList_.emplace_back(&rep2);
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(2, setManager.representationList_.size());
    auto it = setManager.representationList_.begin();
    ASSERT_EQ(1000, (*it)->bandwidth_);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_Init_003, TestSize.Level1)
{
    DashAdptSetInfo info;
    info.representationList_.emplace_back(nullptr);
    info.representationList_.emplace_back(nullptr);
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(2, setManager.representationList_.size());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_Reset_001, TestSize.Level1)
{
    DashUrlType init {.sourceUrl_ = "mock url"};
    DashSegBaseInfo baseInfo {.initialization_ = &init};
    DashAdptSetInfo info {.adptSetSegBase_ = &baseInfo};
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.initSegment_);
    ASSERT_NE(nullptr, setManager.adptSetInfo_);
    setManager.Reset();
    ASSERT_EQ(nullptr, setManager.initSegment_);
    ASSERT_EQ(nullptr, setManager.adptSetInfo_);
    ASSERT_EQ(nullptr, setManager.previousAdptSetInfo_);
    ASSERT_EQ(0, setManager.representationList_.size());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_ParseInitSegment_003, TestSize.Level1)
{
    DashUrlType init {.sourceUrl_ = "mock url", .range_ = "mock range"};
    DashSegListInfo segListInfo;
    segListInfo.multSegBaseInfo_.segBaseInfo_.initialization_ = &init;
    DashAdptSetInfo info {.adptSetSegList_ = &segListInfo};
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.initSegment_);
    ASSERT_EQ("mock url", setManager.initSegment_->sourceUrl_);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_ParseInitSegmentBySegTmplt_002, TestSize.Level1)
{
    DashSegTmpltInfo tmpltInfo;
    tmpltInfo.segTmpltInitialization_ = "init.mp4";
    DashAdptSetInfo info {.adptSetSegTmplt_ = &tmpltInfo};
    DashAdptSetManager setManager {&info};
    ASSERT_NE(nullptr, setManager.initSegment_);
    ASSERT_EQ("init.mp4", setManager.initSegment_->sourceUrl_);
    ASSERT_EQ(1, setManager.segTmpltFlag_);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_ParseInitSegmentBySegTmplt_003, TestSize.Level1)
{
    DashSegTmpltInfo tmpltInfo;
    DashAdptSetInfo info {.adptSetSegTmplt_ = &tmpltInfo};
    DashUrlType* oldInit = new DashUrlType();
    oldInit->sourceUrl_ = "old_init";
    DashAdptSetManager setManager;
    setManager.adptSetInfo_ = &info;
    setManager.initSegment_ = oldInit;
    ASSERT_NE(nullptr, setManager.initSegment_);
    setManager.ParseInitSegmentBySegTmplt();
    ASSERT_EQ(nullptr, setManager.initSegment_);
    ASSERT_EQ(0, setManager.segTmpltFlag_);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_SetAdptSetInfo_001, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(&info, setManager.previousAdptSetInfo_);
    setManager.SetAdptSetInfo(&info);
    ASSERT_EQ(&info, setManager.previousAdptSetInfo_);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetBaseUrlList_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    info.baseUrl_.push_back("http://test.com/");
    DashAdptSetManager setManager {&info};
    std::list<std::string> baseUrlList;
    setManager.GetBaseUrlList(baseUrlList);
    ASSERT_EQ(1, baseUrlList.size());
    ASSERT_EQ("http://test.com/", baseUrlList.front());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetBandwidths_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashRepresentationInfo rep1 {.bandwidth_ = 1000};
    DashRepresentationInfo rep2 {.bandwidth_ = 2000};
    info.representationList_.emplace_back(&rep1);
    info.representationList_.emplace_back(&rep2);
    DashAdptSetManager setManager {&info};
    DashVector<uint32_t> bandwidths;
    setManager.GetBandwidths(bandwidths);
    ASSERT_EQ(2, bandwidths.size());
    ASSERT_EQ(1000, bandwidths[0]);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetRepresentationByBandwidth_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashRepresentationInfo rep1 {.bandwidth_ = 1000};
    DashRepresentationInfo rep2 {.bandwidth_ = 2000};
    info.representationList_.emplace_back(&rep1);
    info.representationList_.emplace_back(&rep2);
    DashAdptSetManager setManager {&info};
    auto result = setManager.GetRepresentationByBandwidth(1000);
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(1000, result->bandwidth_);
    result = setManager.GetRepresentationByBandwidth(999);
    ASSERT_EQ(nullptr, result);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetHighRepresentation_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashRepresentationInfo rep1 {.bandwidth_ = 1000};
    DashRepresentationInfo rep2 {.bandwidth_ = 2000};
    info.representationList_.emplace_back(&rep1);
    info.representationList_.emplace_back(&rep2);
    DashAdptSetManager setManager {&info};
    auto result = setManager.GetHighRepresentation();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(2000, result->bandwidth_);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetLowRepresentation_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashRepresentationInfo rep1 {.bandwidth_ = 1000};
    DashRepresentationInfo rep2 {.bandwidth_ = 2000};
    info.representationList_.emplace_back(&rep1);
    info.representationList_.emplace_back(&rep2);
    DashAdptSetManager setManager {&info};
    auto result = setManager.GetLowRepresentation();
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(1000, result->bandwidth_);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetMime_005, TestSize.Level1)
{
    DashAdptSetInfo info;
    info.contentType_ = "video/mp4";
    DashAdptSetManager setManager {&info};
    ASSERT_EQ("video/mp4", setManager.GetMime());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetInitSegment_001, TestSize.Level1)
{
    DashUrlType init {.sourceUrl_ = "mock url"};
    DashSegBaseInfo baseInfo {.initialization_ = &init};
    DashAdptSetInfo info {.adptSetSegBase_ = &baseInfo};
    DashAdptSetManager setManager {&info};
    int32_t flag = -1;
    auto result = setManager.GetInitSegment(flag);
    ASSERT_NE(nullptr, result);
    ASSERT_EQ(0, flag);
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetAdptSetInfo_001, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(&info, setManager.GetAdptSetInfo());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_GetPreviousAdptSetInfo_001, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(&info, setManager.GetPreviousAdptSetInfo());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_007, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashSegBaseInfo repSegBase {.indexRange_ = "0-1000"};
    DashRepresentationInfo rep {.representationSegBase_ = &repSegBase};
    info.representationList_.emplace_back(&rep);
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(true, setManager.IsOnDemand());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_008, TestSize.Level1)
{
    DashAdptSetInfo info;
    info.representationList_.emplace_back(nullptr);
    DashRepresentationInfo rep;
    info.representationList_.emplace_back(&rep);
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(false, setManager.IsOnDemand());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsOnDemand_009, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashRepresentationInfo rep;
    info.representationList_.emplace_back(&rep);
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(false, setManager.IsOnDemand());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsHdr_001, TestSize.Level1)
{
    DashAdptSetManager setManager;
    ASSERT_EQ(false, setManager.IsHdr());
}

HWTEST_F(DashAdptSetManagerUnitTest, Dash_Adpt_SetManager_IsHdr_002, TestSize.Level1)
{
    DashAdptSetInfo info;
    DashDescriptor desc;
    desc.schemeIdUrl_ = "urn:uuid:6d82888e-3c57-4a0d-b5e1-9fcb490e3e55";
    info.commonAttrsAndElements_.essentialPropertyList_.push_back(&desc);
    DashAdptSetManager setManager {&info};
    ASSERT_EQ(false, setManager.IsHdr());
}
}
}
}
}