/*
 * Copyright (C) 2024 Huawei Device Co., Ltd.
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

#include <memory>
#include <iostream>
#include "gtest/gtest.h"
#include "avcodec_dump_utils.h"
#include "avcodec_errors.h"

using namespace testing::ext;
using namespace OHOS::Media;

namespace {
constexpr uint32_t DUMP_INVALID_INDEX = 0x00'00'00'00;
constexpr uint32_t DUMP_VALID_INDEX_LEVEL_1   = 0x01'00'00'00;
constexpr uint32_t DUMP_VALID_INDEX_LEVEL_2   = 0x01'01'00'00;
constexpr uint32_t DUMP_VALID_INDEX_LEVEL_3   = 0x01'01'01'00;
constexpr uint32_t DUMP_VALID_INDEX_LEVEL_4   = 0x01'01'01'01;
constexpr char     DUMP_INT32_KEY[]           = "Int32_key";
constexpr char     DUMP_INT64_KEY[]           = "Int64_key";
constexpr char     DUMP_FLOAT_KEY[]           = "Float_key";
constexpr char     DUMP_DOUBLE_KEY[]          = "Double_key";
constexpr char     DUMP_STRING_KEY[]          = "String_key";
constexpr char     DUMP_ADDR_KEY[]            = "Addr_key";
constexpr char     DUMP_UNEXIST_KEY[]         = "Unexist_key";
}

namespace OHOS::MediaAVCodec {
class DumpUtilsTestSuilt : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
    void TearDown(void) {};
};

int32_t DumpString(AVCodecDumpControler &controler)
{
    std::string dumpString;
    int32_t ret = controler.GetDumpString(dumpString);
    std::cout << dumpString;
    return ret;
}

HWTEST_F(DumpUtilsTestSuilt, ADD_INFO_TEST, TestSize.Level1)
{
    AVCodecDumpControler controler;

    // Test for normal usage
    int32_t ret = controler.AddInfo(DUMP_VALID_INDEX_LEVEL_1, "Name", "Value");
    ASSERT_EQ(ret, AVCS_ERR_OK);
    
    // Test for invalid dump index
    ret = controler.AddInfo(DUMP_INVALID_INDEX, "Name", "Value");
    ASSERT_EQ(ret, AVCS_ERR_INVALID_VAL);

    // Test for invalid dump name
    ret = controler.AddInfo(DUMP_VALID_INDEX_LEVEL_1, "", "Value");
    ASSERT_EQ(ret, AVCS_ERR_INVALID_VAL);

    // Test for reset a same index to controler
    ret = controler.AddInfo(DUMP_VALID_INDEX_LEVEL_1, "Name", "Value");
    ASSERT_EQ(ret, AVCS_ERR_OK);

    ret = DumpString(controler);
    ASSERT_EQ(ret, AVCS_ERR_OK);
}

HWTEST_F(DumpUtilsTestSuilt, ADD_INFO_FROM_FORMAT_TEST, TestSize.Level1)
{
    AVCodecDumpControler controler;
    Format format;
    format.PutIntValue(DUMP_INT32_KEY, 0);
    format.PutLongValue(DUMP_INT64_KEY, 0);
    format.PutFloatValue(DUMP_FLOAT_KEY, 0);
    format.PutDoubleValue(DUMP_DOUBLE_KEY, 0);
    format.PutStringValue(DUMP_STRING_KEY, "Value");
    format.PutBuffer(DUMP_ADDR_KEY, reinterpret_cast<uint8_t *>(&controler), sizeof(AVCodecDumpControler));
    
    // Test for set invalid key
    int32_t ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, "", "Name");
    ASSERT_EQ(ret, AVCS_ERR_INVALID_VAL);

    // Test for set int32 value
    ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, DUMP_INT32_KEY, "Name");
    ASSERT_EQ(ret, AVCS_ERR_OK);

    // Test for set int64 value
    ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, DUMP_INT64_KEY, "Name");
    ASSERT_EQ(ret, AVCS_ERR_OK);

    // Test for set float value
    ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, DUMP_FLOAT_KEY, "Name");
    ASSERT_EQ(ret, AVCS_ERR_OK);

    // Test for set double value
    ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, DUMP_DOUBLE_KEY, "Name");
    ASSERT_EQ(ret, AVCS_ERR_OK);

    // Test for set string value
    ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, DUMP_STRING_KEY, "Name");
    ASSERT_EQ(ret, AVCS_ERR_OK);

    // Test for set Addr value
    ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, DUMP_ADDR_KEY, "Name");
    ASSERT_EQ(ret, AVCS_ERR_INVALID_VAL);

    // Test for set unexist key
    ret = controler.AddInfoFromFormat(DUMP_VALID_INDEX_LEVEL_2, format, DUMP_UNEXIST_KEY, "Name");
    ASSERT_EQ(ret, AVCS_ERR_INVALID_VAL);

    ret = DumpString(controler);
    ASSERT_EQ(ret, AVCS_ERR_OK);
}

HWTEST_F(DumpUtilsTestSuilt, ADD_INFO_FROM_FORMAT_WITH_MAPPING_TEST, TestSize.Level1)
{
    AVCodecDumpControler controler;
    Format format;
    format.PutIntValue(DUMP_INT32_KEY, 0);
    std::map<int32_t, const std::string> valueMap = {
        {0, "Value"}
    };

    // Test for set normal usage
    int32_t ret =
        controler.AddInfoFromFormatWithMapping(DUMP_VALID_INDEX_LEVEL_3, format, DUMP_INT32_KEY, "Name", valueMap);
    ASSERT_EQ(ret, AVCS_ERR_OK);

    // Test for set unexist value
    ret = controler.AddInfoFromFormatWithMapping(DUMP_VALID_INDEX_LEVEL_3, format, DUMP_INT32_KEY, "Name", valueMap);
    ASSERT_EQ(ret, AVCS_ERR_OK);

    // Test for set unexist key
    ret = controler.AddInfoFromFormatWithMapping(DUMP_VALID_INDEX_LEVEL_3, format, DUMP_UNEXIST_KEY, "Name", valueMap);
    ASSERT_EQ(ret, AVCS_ERR_INVALID_VAL);

    ret = DumpString(controler);
    ASSERT_EQ(ret, AVCS_ERR_OK);
}

HWTEST_F(DumpUtilsTestSuilt, GET_DUMP_STRING_TEST, TestSize.Level1)
{
    AVCodecDumpControler controler;

    int32_t ret = controler.AddInfo(DUMP_VALID_INDEX_LEVEL_4, "Name", "Value");
    ASSERT_EQ(ret, AVCS_ERR_OK);

    ret = DumpString(controler);
    ASSERT_EQ(ret, AVCS_ERR_OK);
}
}