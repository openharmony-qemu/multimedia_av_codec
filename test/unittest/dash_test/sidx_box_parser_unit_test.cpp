/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "sidx_box_parser_unit_test.h"
#include "sidx_box_parser.h"
#include "mpd_parser_def.h"

using namespace std;
using namespace testing::ext;

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
void SidxBoxParserUnitTest::SetUpTestCase(void) {}
void SidxBoxParserUnitTest::TearDownTestCase(void) {}
void SidxBoxParserUnitTest::SetUp(void) {}
void SidxBoxParserUnitTest::TearDown(void) {}

/**
 * @tc.name  : Test ParseSidxBox with null bitStream
 * @tc.number: ParseSidxBox_001
 * @tc.desc  : Test bitStream == nullptr
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_001, TestSize.Level0)
{
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(nullptr, 0, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test ParseSidxBox with zero streamSize
 * @tc.number: ParseSidxBox_002
 * @tc.desc  : Test streamSize == 0
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_002, TestSize.Level0)
{
    char buffer[100] = {0};
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, 0, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test ParseSidxBox with insufficient buffer size
 * @tc.number: ParseSidxBox_003
 * @tc.desc  : Test streamSize < BASE_BOX_HEAD_SIZE (8 bytes)
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_003, TestSize.Level0)
{
    char buffer[4] = {0};
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, 4, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test ParseSidxBox with invalid box type
 * @tc.number: ParseSidxBox_004
 * @tc.desc  : Test boxType != 'sidx'
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_004, TestSize.Level0)
{
    char buffer[20] = {0};
    buffer[4] = 'f';
    buffer[5] = 'r';
    buffer[6] = 'e';
    buffer[7] = 'e';
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, 20, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with version 0 and single reference
 * @tc.number: ParseSidxBox_005
 * @tc.desc  : Test version 0 sidx box with one reference
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_005, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
}

/**
 * @tc.name  : Test ParseSidxBox with version 0 and multiple references
 * @tc.number: ParseSidxBox_006
 * @tc.desc  : Test version 0 sidx box with multiple references
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_006, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x38,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0x60,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x60,
        0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x60,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 2);
}

/**
 * @tc.name  : Test ParseSidxBox with version 1 and single reference
 * @tc.number: ParseSidxBox_007
 * @tc.desc  : Test version 1 sidx box with one reference
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_007, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x34,
        0x73, 0x69, 0x64, 0x78,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
}

/**
 * @tc.name  : Test ParseSidxBox with version 1 and multiple references
 * @tc.number: ParseSidxBox_008
 * @tc.desc  : Test version 1 sidx box with multiple references
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_008, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x40,
        0x73, 0x69, 0x64, 0x78,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0x60,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x60,
        0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0x60,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 2);
}

/**
 * @tc.name  : Test ParseSidxBox with positive sidxEndOffset
 * @tc.number: ParseSidxBox_009
 * @tc.desc  : Test sidxEndOffset > 0
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_009, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 1000, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
    if (!subSegIndexTable.empty()) {
        EXPECT_EQ(subSegIndexTable.front()->startPos_, 1001);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with negative sidxEndOffset
 * @tc.number: ParseSidxBox_010
 * @tc.desc  : Test sidxEndOffset < 0
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_010, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, -500, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
}

/**
 * @tc.name  : Test ParseSidxBox with zero reference count
 * @tc.number: ParseSidxBox_011
 * @tc.desc  : Test referenceCount == 0
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_011, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);

    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 0);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete buffer for box header
 * @tc.number: ParseSidxBox_012
 * @tc.desc  : Test buffer size exactly 7 bytes (less than box header)
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_012, TestSize.Level0)
{
    char buffer[7] = {0};
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, 7, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete reference data
 * @tc.number: ParseSidxBox_013
 * @tc.desc  : Test buffer size insufficient for reference data
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_013, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with reference count overflow
 * @tc.number: ParseSidxBox_014
 * @tc.desc  : Test referenceCount causing buffer overflow
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_014, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete vFlag field
 * @tc.number: ParseSidxBox_015
 * @tc.desc  : Test buffer size insufficient for vFlag
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_015, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete timescale field
 * @tc.number: ParseSidxBox_016
 * @tc.desc  : Test buffer size insufficient for timescale
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_016, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete version 0 earliestPresentationTime
 * @tc.number: ParseSidxBox_017
 * @tc.desc  : Test buffer size insufficient for version 0 earliestPresentationTime
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_017, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete version 0 firstOffset
 * @tc.number: ParseSidxBox_018
 * @tc.desc  : Test buffer size insufficient for version 0 firstOffset
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_018, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete version 1 earliestPresentationTime
 * @tc.number: ParseSidxBox_019
 * @tc.desc  : Test buffer size insufficient for version 1 earliestPresentationTime
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_019, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x34,
        0x73, 0x69, 0x64, 0x78,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete version 1 firstOffset
 * @tc.number: ParseSidxBox_020
 * @tc.desc  : Test buffer size insufficient for version 1 firstOffset
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_020, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x34,
        0x73, 0x69, 0x64, 0x78,
        0x01, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete referenceCount field
 * @tc.number: ParseSidxBox_021
 * @tc.desc  : Test buffer size insufficient for referenceCount
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_021, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with different timescale values
 * @tc.number: ParseSidxBox_022
 * @tc.desc  : Test timescale = 1000
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_022, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
    if (!subSegIndexTable.empty()) {
        EXPECT_EQ(subSegIndexTable.front()->timeScale_, 1000);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with media reference type
 * @tc.number: ParseSidxBox_023
 * @tc.desc  : Test referenceType = 0 (media)
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_023, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
    if (!subSegIndexTable.empty()) {
        EXPECT_EQ(subSegIndexTable.front()->referenceType_, 0);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with index reference type
 * @tc.number: ParseSidxBox_024
 * @tc.desc  : Test referenceType = 1 (index)
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_024, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x80, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
    if (!subSegIndexTable.empty()) {
        EXPECT_EQ(subSegIndexTable.front()->referenceType_, 1);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with large referencedSize
 * @tc.number: ParseSidxBox_025
 * @tc.desc  : Test referencedSize with maximum 31-bit value
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_025, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x7F, 0xFF, 0xFF, 0xFF,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
    if (!subSegIndexTable.empty()) {
        EXPECT_EQ(subSegIndexTable.front()->referencedSize_, 0x7FFFFFFF);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with large firstOffset
 * @tc.number: ParseSidxBox_026
 * @tc.desc  : Test firstOffset with large value
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_026, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0x7FFFFFFF, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
    if (!subSegIndexTable.empty()) {
        EXPECT_EQ(subSegIndexTable.front()->startPos_, 0x80000000);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with duration and timescale mapping
 * @tc.number: ParseSidxBox_027
 * @tc.desc  : Test correct duration and timescale mapping
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_027, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x01, 0x86, 0xA0,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x01, 0x86,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
    if (!subSegIndexTable.empty()) {
        EXPECT_EQ(subSegIndexTable.front()->duration_, 0x186);
        EXPECT_EQ(subSegIndexTable.front()->timeScale_, 0x186A0);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with SAP info parsing
 * @tc.number: ParseSidxBox_028
 * @tc.desc  : Test SAP info is parsed correctly
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_028, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
}

/**
 * @tc.name  : Test ParseSidxBox with multiple segments position calculation
 * @tc.number: ParseSidxBox_029
 * @tc.desc  : Test correct startPos and endPos calculation for multiple segments
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_029, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x38,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0x60,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x02,
        0x00, 0x00, 0x00, 0x0A,
        0x00, 0x00, 0x03, 0x60,
        0x10, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x0B,
        0x00, 0x00, 0x03, 0x60,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 2);
    if (subSegIndexTable.size() >= 2) {
        auto it = subSegIndexTable.begin();
        EXPECT_EQ((*it)->startPos_, 1);
        EXPECT_EQ((*it)->endPos_, 10);
        ++it;
        EXPECT_EQ((*it)->startPos_, 11);
        EXPECT_EQ((*it)->endPos_, 21);
    }
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete typeAndSize field
 * @tc.number: ParseSidxBox_030
 * @tc.desc  : Test buffer size insufficient for typeAndSize in reference
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_030, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete duration field
 * @tc.number: ParseSidxBox_031
 * @tc.desc  : Test buffer size insufficient for duration in reference
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_031, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with incomplete SAP info field
 * @tc.number: ParseSidxBox_032
 * @tc.desc  : Test buffer size insufficient for SAP info in reference
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_032, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, -1);
}

/**
 * @tc.name  : Test ParseSidxBox with buffer at exact boundary
 * @tc.number: ParseSidxBox_033
 * @tc.desc  : Test buffer size exactly matches required size
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_033, TestSize.Level0)
{
    char buffer[] = {
        0x00, 0x00, 0x00, 0x2C,
        0x73, 0x69, 0x64, 0x78,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x03, 0xE8,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x00, 0x01,
        0x00, 0x00, 0x00, 0x00,
        0x00, 0x00, 0x03, 0xE8,
        0x10, 0x00, 0x00, 0x00
    };
    uint32_t len = sizeof(buffer);
    DashList<std::shared_ptr<SubSegmentIndex>> subSegIndexTable;
    int32_t ret = SidxBoxParser::ParseSidxBox(buffer, len, 0, subSegIndexTable);
    EXPECT_EQ(ret, 0);
    EXPECT_EQ(subSegIndexTable.size(), 1);
}

/**
 * @tc.name  : Test ParseSidxBox destructor
 * @tc.number: ParseSidxBox_034
 * @tc.desc  : Test SidxBoxParser destructor
 */
HWTEST_F(SidxBoxParserUnitTest, ParseSidxBox_034, TestSize.Level0)
{
    auto parser = std::make_shared<SidxBoxParser>();
    parser.reset();
    EXPECT_EQ(parser, nullptr);
}

} // namespace HttpPlugin
} // namespace Plugins
} // namespace Media
} // namespace OHOS
