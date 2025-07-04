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

#include "base64_utils.h"
#include "dash_segment_downloader.h"
#include "gtest/gtest.h"
#include <iostream>
#include "mpd_parser/dash_adpt_set_manager.h"
#include "mpd_parser/dash_mpd_parser.h"
#include "mpd_parser/dash_mpd_manager.h"
#include "mpd_parser/dash_period_manager.h"
#include "mpd_parser/dash_seg_tmline_node.h"
#include "mpd_parser/i_dash_mpd_node.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class DashMpdNodeParserUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void) {}
    void TearDown(void);
private:
    IDashMpdNode* node_ {nullptr};
    std::shared_ptr<XmlParser> parser_;
    std::shared_ptr<XmlElement> element_;
};

void DashMpdNodeParserUnitTest::TearDown()
{
    IDashMpdNode::DestroyNode(node_);
}
/**
 * @tc.name  : Test CreateNode API
 * @tc.number: CreateNode_001
 * @tc.desc  : Test nodeName == "mock"
 */
HWTEST_F(DashMpdNodeParserUnitTest, CreateNode_001, TestSize.Level1)
{
    node_ = IDashMpdNode::CreateNode("mock");
    ASSERT_EQ(nullptr, node_);
}

/**
 * @tc.name  : Test DashSegTmlineNode API
 * @tc.number: DashSegTmlineNode_001
 * @tc.desc  : Test GetAttr interface
 */
HWTEST_F(DashMpdNodeParserUnitTest, DashSegTmlineNode_001, TestSize.Level1)
{
    node_ = IDashMpdNode::CreateNode("SegmentTimeline");
    ASSERT_NE(nullptr, node_);
    node_->ParseNode(parser_, element_);
    parser_ = std::make_shared<XmlParser>();
    ASSERT_NE(nullptr, parser_);
    node_->ParseNode(parser_, element_);
    std::string attr;
    node_->GetAttr("mock", attr);
    ASSERT_EQ("", attr);
    DashSegTmlineNode* segNode = reinterpret_cast<DashSegTmlineNode*>(node_);
    ASSERT_NE(nullptr, segNode);
    segNode->segTmlineAttr_[0].val_ = "mockData";
    node_->GetAttr("t", attr);
    ASSERT_EQ("mockData", attr);
}
}
}
}
}