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

#include <thread>
#include "gtest/gtest.h"
#include "avcodec_xcollie.h"
#include "avcodec_errors.h"

using namespace testing::ext;
using namespace OHOS::MediaAVCodec;

namespace {
constexpr uint32_t SLEEP_TIME = 1'000'000; // 1s
}

namespace OHOS::MediaAVCodec {
class XCollieTestSuilt : public testing::Test {
public:
    static void SetUpTestCase(void) {};
    static void TearDownTestCase(void) {};
    void SetUp(void) {};
    void TearDown(void) {};
};

HWTEST_F(XCollieTestSuilt, INTERFACE_TEST, TestSize.Level1)
{
    COLLIE_LISTEN(usleep(SLEEP_TIME), "CollieTest", true, false, 5);
    CLIENT_COLLIE_LISTEN(usleep(SLEEP_TIME), "CollieTest");
}

HWTEST_F(XCollieTestSuilt, DUMP_TEST, TestSize.Level1)
{
    // Test for no dumper
    int32_t ret = AVCodecXCollie::GetInstance().Dump(0);
    ASSERT_EQ(ret, AVCS_ERR_OK);

    std::thread task([] COLLIE_LISTEN(usleep(SLEEP_TIME), "CollieTest", true, false, 5));
    usleep(20'000); // 20'000: 20ms
    ret = AVCodecXCollie::GetInstance().Dump(0);
    ASSERT_EQ(ret, AVCS_ERR_OK);

    if (task.joinable()) {
        task.join();
    }
}
}