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

#include "dash_period_manager_unit_test.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {

using namespace testing::ext;
using namespace OHOS::Media::Plugins::HttpPlugin;

void DashPeriodManagerUnitTest::SetUpTestCase(void) {}

void DashPeriodManagerUnitTest::TearDownTestCase(void) {}

void DashPeriodManagerUnitTest::SetUp(void)
{
    dashPeriodManager_ = std::make_shared<DashPeriodManager>();
    ASSERT_TRUE(dashPeriodManager_ != nullptr);
}

void DashPeriodManagerUnitTest::TearDown(void)
{
    dashPeriodManager_ = nullptr;
}

/**
 * @tc.name  : Test Reset
 * @tc.number: Reset_001
 * @tc.desc  : Test Reset return
 */
HWTEST_F(DashPeriodManagerUnitTest, Reset_001, TestSize.Level1)
{
    dashPeriodManager_->Reset();
    EXPECT_EQ(dashPeriodManager_->GetPeriod(), nullptr);
    EXPECT_EQ(dashPeriodManager_->GetPreviousPeriod(), nullptr);
}

/**
 * @tc.name  : Test Init
 * @tc.number: Init_001
 * @tc.desc  : Test this->periodInfo_ == nullptr
 */
HWTEST_F(DashPeriodManagerUnitTest, Init_001, TestSize.Level1)
{
    dashPeriodManager_->periodInfo_ = nullptr;
    dashPeriodManager_->Init();
    EXPECT_EQ(dashPeriodManager_->Empty(), true);
}

/**
 * @tc.name  : Test ParseAdptSetList
 * @tc.number: ParseAdptSetList_001
 * @tc.desc  : Test mimeType/contentType is empty and contentCompList is empty
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetList_001, TestSize.Level1)
{
    DashAdptSetInfo dashAdptSetInfoTest;
    dashAdptSetInfoTest.commonAttrsAndElements_.mimeType_ = "";
    dashAdptSetInfoTest.contentType_ = "";

    DashList<DashAdptSetInfo *> adptSetListTest;
    adptSetListTest.push_back(&dashAdptSetInfoTest);

    dashPeriodManager_->videoAdptSetsVector_.clear();

    dashPeriodManager_->ParseAdptSetList(adptSetListTest);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), false);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByMime
 * @tc.number: ParseAdptSetTypeByMime_001
 * @tc.desc  : Test mimeType.find(SUBTITLE_MIME_APPLICATION) != std::string::npos
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByMime_001, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();

    DashAdptSetInfo dashAdptSetInfoTest;
    dashAdptSetInfoTest.mimeType_ = "";
    std::string mimeTypeTest = "http://application:appTest";
    dashPeriodManager_->subtitleMimeType_ = "";
    dashPeriodManager_->ParseAdptSetTypeByMime(mimeTypeTest, &dashAdptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), false);
    EXPECT_EQ(dashPeriodManager_->subtitleMimeType_, DashPeriodManager::SUBTITLE_MIME_APPLICATION);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByMime
 * @tc.number: ParseAdptSetTypeByMime_002
 * @tc.desc  : Test mimeType.find(SUBTITLE_MIME_TEXT) != std::string::npos
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByMime_002, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();

    DashAdptSetInfo dashAdptSetInfoTest;
    dashAdptSetInfoTest.mimeType_ = "";
    std::string mimeTypeTest = "http://text:appTest";
    dashPeriodManager_->subtitleMimeType_ = "";
    dashPeriodManager_->ParseAdptSetTypeByMime(mimeTypeTest, &dashAdptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), false);
    EXPECT_EQ(dashPeriodManager_->subtitleMimeType_, DashPeriodManager::SUBTITLE_MIME_TEXT);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByMime
 * @tc.number: ParseAdptSetTypeByMime_003
 * @tc.desc  : Test mimeType.find(subtitleMimeType_) == std::string::npos
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByMime_003, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();

    DashAdptSetInfo dashAdptSetInfoTest;
    dashAdptSetInfoTest.mimeType_ = "";
    std::string mimeTypeTest = "http://text:appTest";
    dashPeriodManager_->subtitleMimeType_ = "video";
    dashPeriodManager_->ParseAdptSetTypeByMime(mimeTypeTest, &dashAdptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->subtitleMimeType_.empty(), false);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByMime
 * @tc.number: ParseAdptSetTypeByMime_004
 * @tc.desc  : Test mimeType is SUBTITLE_MIME_APPLICATION and subtitleMimeType_ is other
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByMime_004, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();

    DashAdptSetInfo dashAdptSetInfoTest;
    dashAdptSetInfoTest.mimeType_ = "";
    std::string mimeTypeTest = "http://appTest";
    dashPeriodManager_->subtitleMimeType_ = "app";
    dashPeriodManager_->ParseAdptSetTypeByMime(mimeTypeTest, &dashAdptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), false);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByMime
 * @tc.number: ParseAdptSetTypeByMime_005
 * @tc.desc  : Test mimeType is other
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByMime_005, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();

    DashAdptSetInfo dashAdptSetInfoTest;
    dashAdptSetInfoTest.mimeType_ = "";
    std::string mimeTypeTest = "http://appTest";
    dashPeriodManager_->subtitleMimeType_ = "app";
    dashPeriodManager_->ParseAdptSetTypeByMime(mimeTypeTest, &dashAdptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), false);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByComp
 * @tc.number: ParseAdptSetTypeByComp_001
 * @tc.desc  : Test ContainType(contentCompList, AUDIO_MIME_TYPE)
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByComp_001, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();
    dashPeriodManager_->audioAdptSetsVector_.clear();

    DashContentCompInfo dashContentCompInfoTest;
    dashContentCompInfoTest.contentType_ = AUDIO_MIME_TYPE;

    DashList<DashContentCompInfo *> contentCompListTest;
    contentCompListTest.push_back(&dashContentCompInfoTest);

    DashAdptSetInfo adptSetInfoTest;
    adptSetInfoTest.mimeType_ = "";

    dashPeriodManager_->ParseAdptSetTypeByComp(contentCompListTest, &adptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->audioAdptSetsVector_.empty(), false);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByComp
 * @tc.number: ParseAdptSetTypeByComp_002
 * @tc.desc  : Test ContainType(contentCompList, SUBTITLE_MIME_APPLICATION)
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByComp_002, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();
    dashPeriodManager_->audioAdptSetsVector_.clear();

    DashContentCompInfo dashContentCompInfoTest;
    dashContentCompInfoTest.contentType_ = DashPeriodManager::SUBTITLE_MIME_APPLICATION;

    DashList<DashContentCompInfo *> contentCompListTest;
    contentCompListTest.push_back(&dashContentCompInfoTest);

    DashAdptSetInfo adptSetInfoTest;
    adptSetInfoTest.mimeType_ = "http://application:appTest";
    dashPeriodManager_->subtitleMimeType_ = "";

    dashPeriodManager_->ParseAdptSetTypeByComp(contentCompListTest, &adptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), false);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->audioAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->subtitleMimeType_, DashPeriodManager::SUBTITLE_MIME_APPLICATION);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByComp
 * @tc.number: ParseAdptSetTypeByComp_003
 * @tc.desc  : Test ContainType(contentCompList, SUBTITLE_MIME_TEXT)
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByComp_003, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();
    dashPeriodManager_->audioAdptSetsVector_.clear();

    DashContentCompInfo dashContentCompInfoTest;
    dashContentCompInfoTest.contentType_ = DashPeriodManager::SUBTITLE_MIME_TEXT;

    DashList<DashContentCompInfo *> contentCompListTest;
    contentCompListTest.push_back(&dashContentCompInfoTest);

    DashAdptSetInfo adptSetInfoTest;
    adptSetInfoTest.mimeType_ = "http:://text:appTest";
    dashPeriodManager_->subtitleMimeType_ = "";

    dashPeriodManager_->ParseAdptSetTypeByComp(contentCompListTest, &adptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), false);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->audioAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->subtitleMimeType_, DashPeriodManager::SUBTITLE_MIME_TEXT);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByComp
 * @tc.number: ParseAdptSetTypeByComp_004
 * @tc.desc  : Test subtitleMimeType_.empty()
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByComp_004, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();
    dashPeriodManager_->audioAdptSetsVector_.clear();

    DashContentCompInfo dashContentCompInfoTest;
    dashContentCompInfoTest.contentType_ = DashPeriodManager::SUBTITLE_MIME_TEXT;

    DashList<DashContentCompInfo *> contentCompListTest;
    contentCompListTest.push_back(&dashContentCompInfoTest);

    DashAdptSetInfo adptSetInfoTest;
    adptSetInfoTest.mimeType_ = "http:://text:appTest111";
    dashPeriodManager_->subtitleMimeType_ = "";

    dashPeriodManager_->ParseAdptSetTypeByComp(contentCompListTest, &adptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), false);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->audioAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->subtitleMimeType_, DashPeriodManager::SUBTITLE_MIME_TEXT);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByComp
 * @tc.number: ParseAdptSetTypeByComp_005
 * @tc.desc  : Test !ContainType(contentCompList, subtitleMimeType_)
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByComp_005, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();
    dashPeriodManager_->audioAdptSetsVector_.clear();

    DashContentCompInfo dashContentCompInfoTest;
    dashContentCompInfoTest.contentType_ = DashPeriodManager::SUBTITLE_MIME_TEXT;

    DashList<DashContentCompInfo *> contentCompListTest;
    contentCompListTest.push_back(&dashContentCompInfoTest);

    DashAdptSetInfo adptSetInfoTest;
    adptSetInfoTest.mimeType_ = "http:://text:appTest111";
    dashPeriodManager_->subtitleMimeType_ = "app";

    dashPeriodManager_->ParseAdptSetTypeByComp(contentCompListTest, &adptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->audioAdptSetsVector_.empty(), true);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByComp
 * @tc.number: ParseAdptSetTypeByComp_006
 * @tc.desc  : Test ContainType(contentCompList, subtitleMimeType_)
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByComp_006, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();
    dashPeriodManager_->audioAdptSetsVector_.clear();

    DashContentCompInfo dashContentCompInfoTest;
    dashContentCompInfoTest.contentType_ = DashPeriodManager::SUBTITLE_MIME_TEXT;

    DashList<DashContentCompInfo *> contentCompListTest;
    contentCompListTest.push_back(&dashContentCompInfoTest);

    DashAdptSetInfo adptSetInfoTest;
    adptSetInfoTest.mimeType_ = "http:://text:appTest111";
    dashPeriodManager_->subtitleMimeType_ = DashPeriodManager::SUBTITLE_MIME_TEXT;

    dashPeriodManager_->ParseAdptSetTypeByComp(contentCompListTest, &adptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), false);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->audioAdptSetsVector_.empty(), true);
}

/**
 * @tc.name  : Test ParseAdptSetTypeByComp
 * @tc.number: ParseAdptSetTypeByComp_007
 * @tc.desc  : Test ContainType(contentCompList, subtitleMimeType_) is other
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseAdptSetTypeByComp_007, TestSize.Level1)
{
    dashPeriodManager_->subtitleAdptSetsVector_.clear();
    dashPeriodManager_->videoAdptSetsVector_.clear();
    dashPeriodManager_->audioAdptSetsVector_.clear();

    DashContentCompInfo dashContentCompInfoTest;
    dashContentCompInfoTest.contentType_ = "app";

    DashList<DashContentCompInfo *> contentCompListTest;
    contentCompListTest.push_back(&dashContentCompInfoTest);

    DashAdptSetInfo adptSetInfoTest;
    adptSetInfoTest.mimeType_ = "http:://text:appTest111";
    dashPeriodManager_->subtitleMimeType_ = DashPeriodManager::SUBTITLE_MIME_TEXT;

    dashPeriodManager_->ParseAdptSetTypeByComp(contentCompListTest, &adptSetInfoTest);
    EXPECT_EQ(dashPeriodManager_->subtitleAdptSetsVector_.empty(), true);
    EXPECT_EQ(dashPeriodManager_->videoAdptSetsVector_.empty(), false);
    EXPECT_EQ(dashPeriodManager_->audioAdptSetsVector_.empty(), true);
}

/**
 * @tc.name  : Test ParseInitSegment
 * @tc.number: ParseInitSegment_001
 * @tc.desc  : Test periodInfo_->periodSegBase_ && periodInfo_->periodSegBase_->initialization_
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseInitSegment_001, TestSize.Level1)
{
    DashUrlType dashUrlTypeTest;
    dashUrlTypeTest.range_ = "test_range";
    dashUrlTypeTest.sourceUrl_ = "test_sourceUrl";
    DashSegBaseInfo dashSegBaseInfoTest;
    DashPeriodInfo dashPeriodInfoTest;
    dashSegBaseInfoTest.initialization_ = &dashUrlTypeTest;
    dashPeriodInfoTest.periodSegBase_ = &dashSegBaseInfoTest;

    dashPeriodManager_->periodInfo_ = &dashPeriodInfoTest;

    dashPeriodManager_->ParseInitSegment();
    EXPECT_TRUE(dashPeriodManager_->initSegment_ != nullptr);
    EXPECT_EQ(dashPeriodManager_->initSegment_->range_, "test_range");
    EXPECT_EQ(dashPeriodManager_->initSegment_->sourceUrl_, "test_sourceUrl");
}

/**
 * @tc.name  : Test ParseInitSegment
 * @tc.number: ParseInitSegment_002
 * @tc.desc  : Test periodInfo_->periodSegList_ &&
 *    periodInfo_->periodSegList_->multSegBaseInfo_.segBaseInfo_.initialization_
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseInitSegment_002, TestSize.Level1)
{
    DashUrlType dashUrlTypeTest;
    dashUrlTypeTest.range_ = "test_range";
    dashUrlTypeTest.sourceUrl_ = "test_sourceUrl";

    DashSegListInfo periodSegListTest;
    periodSegListTest.multSegBaseInfo_.segBaseInfo_.initialization_ = &dashUrlTypeTest;

    DashPeriodInfo dashPeriodInfoTest;
    dashPeriodInfoTest.periodSegBase_ = nullptr;
    dashPeriodInfoTest.periodSegList_ = &periodSegListTest;

    dashPeriodManager_->periodInfo_ = &dashPeriodInfoTest;

    dashPeriodManager_->ParseInitSegment();
    EXPECT_TRUE(dashPeriodManager_->initSegment_ != nullptr);
    EXPECT_EQ(dashPeriodManager_->initSegment_->range_, "test_range");
    EXPECT_EQ(dashPeriodManager_->initSegment_->sourceUrl_, "test_sourceUrl");
}

/**
 * @tc.name  : Test ParseInitSegmentBySegTmplt
 * @tc.number: ParseInitSegmentBySegTmplt_001
 * @tc.desc  : Test periodInfo_->periodSegTmplt_->multSegBaseInfo_.segBaseInfo_.initialization_
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseInitSegmentBySegTmplt_001, TestSize.Level1)
{
    DashUrlType dashUrlTypeTest;
    dashUrlTypeTest.range_ = "test_range";
    dashUrlTypeTest.sourceUrl_ = "test_sourceUrl";

    DashSegTmpltInfo dashSegTmpltInfoTest;
    DashPeriodInfo dashPeriodInfoTest;

    dashSegTmpltInfoTest.multSegBaseInfo_.segBaseInfo_.initialization_ = &dashUrlTypeTest;
    dashPeriodInfoTest.periodSegTmplt_ = &dashSegTmpltInfoTest;

    dashPeriodManager_->periodInfo_ = &dashPeriodInfoTest;

    dashPeriodManager_->ParseInitSegmentBySegTmplt();
    EXPECT_TRUE(dashPeriodManager_->initSegment_ != nullptr);
    EXPECT_EQ(dashPeriodManager_->initSegment_->range_, "test_range");
    EXPECT_EQ(dashPeriodManager_->initSegment_->sourceUrl_, "test_sourceUrl");
}

/**
 * @tc.name  : Test ParseInitSegmentBySegTmplt
 * @tc.number: ParseInitSegmentBySegTmplt_002
 * @tc.desc  : Test periodInfo_->periodSegTmplt_->segTmpltInitialization_.length() > 0
 */
HWTEST_F(DashPeriodManagerUnitTest, ParseInitSegmentBySegTmplt_002, TestSize.Level1)
{
    DashSegTmpltInfo dashSegTmpltInfoTest;
    DashPeriodInfo dashPeriodInfoTest;
    dashSegTmpltInfoTest.segTmpltInitialization_ = "test1";
    dashSegTmpltInfoTest.multSegBaseInfo_.segBaseInfo_.initialization_ = nullptr;
    dashPeriodInfoTest.periodSegTmplt_ = &dashSegTmpltInfoTest;

    dashPeriodManager_->periodInfo_ = &dashPeriodInfoTest;

    dashPeriodManager_->ParseInitSegmentBySegTmplt();
    EXPECT_TRUE(dashPeriodManager_->initSegment_ != nullptr);
    EXPECT_EQ(dashPeriodManager_->segTmpltFlag_, 1);
    EXPECT_EQ(dashPeriodManager_->initSegment_->sourceUrl_, "test1");
}

/**
 * @tc.name  : Test SetPeriodInfo
 * @tc.number: SetPeriodInfo_001
 * @tc.desc  : Test previousPeriodInfo_ == period
 */
HWTEST_F(DashPeriodManagerUnitTest, SetPeriodInfo_001, TestSize.Level1)
{
    DashPeriodInfo dashPeriodInfoTest;
    dashPeriodInfoTest.bitstreamSwitching_ = true;

    dashPeriodManager_->previousPeriodInfo_ = &dashPeriodInfoTest;
    dashPeriodManager_->SetPeriodInfo(&dashPeriodInfoTest);
    EXPECT_TRUE(dashPeriodManager_->periodInfo_ != nullptr);
    EXPECT_EQ(dashPeriodManager_->periodInfo_->bitstreamSwitching_, true);
    EXPECT_EQ(dashPeriodManager_->previousPeriodInfo_->bitstreamSwitching_, true);
}

/**
 * @tc.name  : Test SetPeriodInfo
 * @tc.number: SetPeriodInfo_002
 * @tc.desc  : Test previousPeriodInfo_ != period
 */
HWTEST_F(DashPeriodManagerUnitTest, SetPeriodInfo_002, TestSize.Level1)
{
    DashPeriodInfo dashPeriodInfoTest;
    DashPeriodInfo dashPeriodInfoTest1;
    dashPeriodInfoTest.bitstreamSwitching_ = true;
    dashPeriodInfoTest1.bitstreamSwitching_ = false;

    dashPeriodManager_->previousPeriodInfo_ = &dashPeriodInfoTest1;

    dashPeriodManager_->SetPeriodInfo(&dashPeriodInfoTest);
    EXPECT_TRUE(dashPeriodManager_->periodInfo_ != nullptr);
    EXPECT_EQ(dashPeriodManager_->periodInfo_->bitstreamSwitching_, true);
    EXPECT_EQ(dashPeriodManager_->periodInfo_->bitstreamSwitching_, true);
}

/**
 * @tc.name  : Test GetBaseUrlList
 * @tc.number: GetBaseUrlList_001
 * @tc.desc  : Test initSegment_ != nullptr
 */
HWTEST_F(DashPeriodManagerUnitTest, GetBaseUrlList_001, TestSize.Level1)
{
    DashPeriodInfo dashPeriodInfoTest;
    dashPeriodInfoTest.bitstreamSwitching_ = true;
    std::string baseUrlTest = "test";
    dashPeriodInfoTest.baseUrl_.push_back(baseUrlTest);

    dashPeriodManager_->periodInfo_ = &dashPeriodInfoTest;

    std::list<std::string> baseUrlListTest;
    dashPeriodManager_->GetBaseUrlList(baseUrlListTest);

    EXPECT_TRUE(dashPeriodManager_->periodInfo_ != nullptr);
    EXPECT_TRUE(baseUrlListTest.size() > 0);
}
} //HttpPlugin
} //Plugins
} //Media
} //OHOS