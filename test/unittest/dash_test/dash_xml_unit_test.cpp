/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "dash_xml_unit_test.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
using namespace testing::ext;

void DashXmlUnitTest::SetUpTestCase(void) {}

void DashXmlUnitTest::TearDownTestCase(void) {}

void DashXmlUnitTest::SetUp(void) {}

void DashXmlUnitTest::TearDown(void) {}

HWTEST_F(DashXmlUnitTest, Test_ParseFromString_Failed_001, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromString("");
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromString_Failed_002, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromString("<test><11111>");
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromString_Success_001, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromString("<test>123</test>");
    EXPECT_GE(ret, 0);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Failed_001, TestSize.Level1)
{
    double ret = xmlParser_->ParseFromBuffer(nullptr, 1);
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Failed_002, TestSize.Level1)
{
    std::string xml = "";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Failed_003, TestSize.Level1)
{
    std::string xml = "<test><11111>";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    EXPECT_GE(ret, -1);
}

HWTEST_F(DashXmlUnitTest, Test_ParseFromBuffer_Success_001, TestSize.Level1)
{
    std::string xml = "<test>123</test>";
    double ret = xmlParser_->ParseFromBuffer(xml.c_str(), xml.length());
    EXPECT_GE(ret, 0);
}

}
}
}
}