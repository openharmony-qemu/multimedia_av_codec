/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AVMUXER_UNIT_TEST_H
#define AVMUXER_UNIT_TEST_H

#include "gtest/gtest.h"
#include <fstream>
#include "avmuxer_sample.h"

namespace OHOS {
namespace MediaAVCodec {
class AVMuxerUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);

    int32_t WriteSample(int32_t trackId, std::shared_ptr<std::ifstream> file, bool &eosFlag, uint32_t flag);

protected:
    std::shared_ptr<AVMuxerSample> avmuxer_ {nullptr};
    std::shared_ptr<std::ifstream> inputFile_ = nullptr;
    int32_t fd_ {-1};
    uint8_t buffer_[26] = {
        'a', 'b', 'c', 'd', 'e', 'f', 'g',
        'h', 'i', 'j', 'k', 'l', 'm', 'n',
        'o', 'p', 'q', 'r', 's', 't',
        'u', 'v', 'w', 'x', 'y', 'z'
    };
    uint8_t annexBuffer_[72] = {
        0x00, 0x00, 0x00, 0x01,
        0x40, 0x01, 0x0c, 0x01, 0xff, 0xff, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03,
        0x00, 0x00, 0x03, 0x00, 0x5d, 0x95, 0x98, 0x09,
        0x00, 0x00, 0x00, 0x01,
        0x42, 0x01, 0x01, 0x01, 0x60, 0x00, 0x00, 0x03, 0x00, 0x90, 0x00, 0x00, 0x03, 0x00, 0x00, 0x03,
        0x00, 0x5d, 0xa0, 0x05, 0xa2, 0x01, 0xe1, 0x65, 0x95, 0x9a, 0x49, 0x32, 0xb9, 0xa0, 0x20, 0x00,
        0x00, 0x03, 0x00, 0x20, 0x00, 0x00, 0x07, 0x81
    };
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVMUXER_UNIT_TEST_H