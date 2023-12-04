/*
 * Copyright (C) 2023 Huawei Device Co., Ltd.
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

#ifndef AVSOURCE_UNIT_TEST_H
#define AVSOURCE_UNIT_TEST_H

#include "gtest/gtest.h"
#include "avsource_mock.h"

namespace OHOS {
namespace MediaAVCodec {
class AVSourceUnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
    int64_t GetFileSize(const std::string &fileName);
    int32_t OpenFile(const std::string &fileName);

    void InitResource(const std::string &path, bool local);
    void CheckHevcInfo(const std::string &path, const std::string resName, bool local);

protected:
    std::shared_ptr<AVSourceMock> source_ = nullptr;
    std::shared_ptr<FormatMock> format_ = nullptr;
    int32_t fd_ = -1;
    int64_t size_ = 0;
    uint32_t trackIndex_ = 0;
    int32_t streamsCount_ = 0;
    int32_t vTrackIdx_ = 0;
    int32_t aTrackIdx_ = 0;
};
} // namespace MediaAVCodec
} // namespace OHOS
#endif // AVSOURCE_UNIT_TEST_H