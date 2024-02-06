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

#ifndef M3U8_UNIT_TEST_H
#define M3U8_UNIT_TEST_H

#include "gtest/gtest.h"
#include "hls/m3u8.h"

namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class M3u8UnitTest : public testing::Test {
public:
    // SetUpTestCase: Called before all test cases
    static void SetUpTestCase(void);
    // TearDownTestCase: Called after all test case
    static void TearDownTestCase(void);
    // SetUp: Called before each test cases
    void SetUp(void);
    // TearDown: Called after each test cases
    void TearDown(void);
};

//解密播放測試url
constexpr std::string BASE_URI = "https://" + "116.205" + ".147.170/video/play/8a821e166409455f0164d4118f30115c/"
    "8a821e156beb885d016c231871c40c01";

constexpr std::string TEST_URI = BASE_URI + "/28.m3u8?schemeSecret=1&t=1692345456402";

// 测试链接 tagAttribute
constexpr std::string TAG_ATTRIBUTE =
    "METHOD=AES-128,URI=\"" + BASE_URI + "/28\",IV=0x00000000000000000000000000000000";

constexpr std::string TEST_NAME = "test.m3u8";

std::shared_ptr<M3U8> testM3u8 = std::make_shared<M3U8>(TEST_URI, TEST_NAME);

}
}
}
}
#endif