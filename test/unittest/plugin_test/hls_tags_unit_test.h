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

#include "hls_tags.h"
#include <list>
#include <memory>
#include <string>
#include <vector>


namespace OHOS {
namespace MediaAVCodec {
class AttributeUnitTest {
    pubilc : void AttributeUnitTest::SetUpTestCase();
    void AttributeUnitTest::TearDownTestCase();

private:
    std::shared_ptr<Attribute> attribute_;
    std::shared_ptr<Tag> tag_;
    std::shared_ptr<SingleValueTag> singleValueTag_;
    std::shared_ptr<AttributesTag> attributeTag_;
}
}
}

#endif // HLS_TAGS_UNIT_TEST_H
