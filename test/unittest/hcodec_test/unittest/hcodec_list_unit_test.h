/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef HCODEC_LIST_UNIT_TEST_H
#define HCODEC_LIST_UNIT_TEST_H

#include <vector>
#include <string>
#include "gtest/gtest.h"
#include "avcodec_info.h"

namespace OHOS {
namespace MediaAVCodec {
class HCodecListUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    std::string GetPrintInfo(const Range&);
    std::string GetPrintInfo(const ImgSize&);
    std::string GetPrintInfo(const std::vector<int32_t>&);
    std::string GetPrintInfo(const std::map<int32_t, std::vector<int32_t>>&);
    std::string GetPrintInfo(const std::map<ImgSize, Range>&);
    static void CheckRange(const Range& range);
    static void CheckImgSize(const ImgSize& size);
};
} // namespace MediaAVCodec
} // namespace OHOS

#endif