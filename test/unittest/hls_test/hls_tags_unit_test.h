/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd.
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

#ifndef HLS_TAGS_UNIT_TEST_H
#define HLS_TAGS_UNIT_TEST_H

#include "hls/hls_tags.h"
#include <list>
#include <memory>
#include <string>
#include <vector>


namespace OHOS {
namespace Media {
namespace Plugins {
namespace HttpPlugin {
class AttributeUnitTest {
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
std::string g_tag_name = "#EXT-X-VERSION";
std::string g_value = "3";
std::string g_key = "METHOD";
std::string g_key_value = "AES-128";
std::shared_ptr<Attribute> attribute = std::make_shared<Attribute>(g_tag_name, g_value);
std::shared_ptr<Tag> tag = std::make_shared<Tag>(HlsTag::EXTXKEY);
std::shared_ptr<SingleValueTag> singleValueTag = std::make_shared<SingleValueTag>(HlsTag::EXTXKEY, g_value);
std::shared_ptr<AttributesTag> attributeTag = std::make_shared<AttributesTag>(HlsTag::EXTXKEY, g_key_value);
}
}
}
}
#endif // HLS_TAGS_UNIT_TEST_H
