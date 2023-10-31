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

#include <dirent.h>
#include <memory>
#include <string>
#include <vector>
#include <gtest/gtest.h>
#include "avbuffer.h"
#include "meta.h"
#include "meta_key.h"
#include "unittest_log.h"

using namespace std;
using namespace testing::ext;
using namespace OHOS;
using namespace OHOS::Media;

namespace OHOS {
namespace MediaAVCodec {
namespace MediaCodecFucUT {
class MediaCodecUnitTest : public testing::Test {
public:
    static void SetUpTestCase(void);
    static void TearDownTestCase(void);
    void SetUp(void);
    void TearDown(void);

private:
};

void MediaCodecUnitTest::SetUpTestCase(void) {}

void MediaCodecUnitTest::TearDownTestCase(void) {}

void MediaCodecUnitTest::SetUp(void)
{
    std::cout << "[SetUp]: SetUp!!!, test: ";
    const ::testing::TestInfo *testInfo_ = ::testing::UnitTest::GetInstance()->current_test_info();
    std::string testName = testInfo_->name();
    std::cout << testName << std::endl;
}

void MediaCodecUnitTest::TearDown(void)
{
    std::cout << "[TearDown]: over!!!" << std::endl;
}

/**
 * @tc.name: MediaCodec_Create_001
 * @tc.desc: media codec create
 * @tc.type: FUNC
 */
HWTEST_F(MediaCodecUnitTest, MediaCodec_Create_001, TestSize.Level1)
{
    EXPECT_EQ(1 - 1, 0);
}
} // namespace MediaCodecFucUT
} // namespace MediaAVCodec
} // namespace OHOS