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

#include "sidx_box_parser.h"
#include "gtest/gtest.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class SidxBoxParserUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void) {}
    static void TearDownTestCase(void) {}
    void SetUp(void) {}
    void TearDown(void) {}
};

/**
 * @tc.name  : Test BuildSubSegmentIndexes API
 * @tc.number: BuildSubSegmentIndexes_001
 * @tc.desc  : Test version != 0 && (referenceCount * SIDX_REFERENCE_ITEM_SIZE + currPos) > streamSize
 */
HWTEST_F(SidxBoxParserUnitTest, BuildSubSegmentIndexes_001, TestSize.Level1)
{
    char mockStr[24] {-1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1};
    DashList<std::shared_ptr<SubSegmentIndex>> mockList;
    uint32_t currPos {0};
    ASSERT_EQ(false, SidxBoxParser::BuildSubSegmentIndexes(mockStr, 0, 0, mockList, currPos));
}

/**
 * @tc.name  : Test ParseSidxBox API
 * @tc.number: ParseSidxBox_001
 * @tc.desc  : Test boxType != BEM_SIDX
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_001, TestSize.Level1)
{
    char mockStr[24] {-1, -1, -1, -1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1};
    DashList<std::shared_ptr<SubSegmentIndex>> mockList;
    ASSERT_EQ(-1, SidxBoxParser::ParseSidxBox(mockStr, 24, 0, mockList));
}
}
}
}
}