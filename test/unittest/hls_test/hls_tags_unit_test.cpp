/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "hls_tags_unit_test.h"
#include "gtest/gtest.h"
#include <cstring>
#include <sstream>
#include <stack>
#include <utility>


using namespace OHOS::Media::Plugins::HttpPlugin ;
using namespace testing::ext;
using namespace std;

namespace {
using namespace testing::ext;

void HlsPlayListDownloaderUnitTest::SetUpTestCase(void) {}

void HlsPlayListDownloaderUnitTest::TearDownTestCase(void) {}

HlsPlayListDownloaderUnitTest::SetUp(void) {}

HlsPlayListDownloaderUnitTest::TearDown(void) {}


HWTEST_F(AttributeUnitTest, HLS_TAGS_GetAttributeName_0001, TestSize.Level1)
{
    EXPECT_STREQ(attribute->GetName(), g_name);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_Decimal_0001, TestSize.Level1)
{
    EXPECT_STRNE(attribute_->Decimal(), 0.0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_QuotedString_0001, TestSize.Level1)
{
    EXPECT_NE(attribute_->QuotedString(), "");
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_FloatingPoint_0001, TestSize.Level1)
{
    EXPECT_NE(attribute_->FloatingPoint(), 0.0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_HexSequence_0001, TestSize.Level1)
{
    std::vector<uint8_t> res = attribute->HexSequence();
    EXPECT_GT(res.size(), 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetByteRange_0001, TestSize.Level1)
{
    std::pair<std::size_t, std::size_t> range = attribute->GetByteRange();
    EXPECT_GE(range.first, 0);
    EXPECT_GT(range.second, 0);
}

HWTEST_F(AttributeUnitTest, HLS_TAGS_GetResolution_0001, TestSize.Level1)
{
    std::pair<int, int> sol = attribute->GetResolution();
    EXPECT_GE(sol.first, 0);
    EXPECT_GT(sol.second, 0);
}


HWTEST_F(AttributeUnitTest, HLS_TAGS_GetType_0001, TestSize.Level1)
{
    EXPECT_STRNE(tag->GetType(), "");
}


HWTEST_F(AttributeUnitTest, HLS_TAGS_AddAttribute_0001, TestSize.Level1)
{
    attributeTag->AddAttribute(attribute);
    EXPECT_QE(attributeTag_->attributes.size(), 1);
}
}
